/*
 *  arch/arm/mach-comcerto/msp/ved.c
 *
 *  Copyright (C) 2006 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/sched.h>

#include "ved.h"

static char ctrl_mac[6] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };

#define VED_POLL_WEIGHT (32)
#define IP_SEC_HEADROOM (40)
#define PKT_BUF_SZ (1540 + IP_SEC_HEADROOM) /* size of each rx buffer */

static int ved_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *ved_get_stats(struct net_device *dev);
static irqreturn_t ved_interrupt(int irq, void *dev_id);
static int ved_open(struct net_device *dev);
static int ved_release(struct net_device *dev);
static int ved_rebuild_header(struct sk_buff *skb);
static int ved_header(struct sk_buff *skb, struct net_device *dev, unsigned short type,
                      const void *daddr, const void *saddr, unsigned int len);
static int ved_write_packet(struct sk_buff *skb, struct net_device *dev);
static int ved_read_packet(struct net_device *dev, struct FDesc * ThisFdesc);
static int ved_poll(struct napi_struct *napi, int budget);
static int start_ved(struct net_device *dev);
static int stop_ved(struct net_device *dev);


static int ved_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ved_priv *priv = (struct ved_priv *)netdev_priv(dev);
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&priv->lock, flags);

	rc = ved_write_packet(skb, dev);

	if (rc) {
		priv->stats.tx_dropped++;
	} else {
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += skb->len;
		dev->trans_start = jiffies; /* save the timestamp */
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static inline void ved_ack_msp(void)
{
	comcerto_softirq_set(IRQ_PTP1);
}

static irqreturn_t ved_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct ved_priv *priv = netdev_priv(dev);

	spin_lock(&priv->lock);

	if (napi_schedule_prep(&priv->napi)) {
		comcerto_softirq_set(irq);

		disable_irq_nosync(irq);
		__napi_schedule(&priv->napi);
	} else {
		/* FIX by disabling interrupts */
		/* disable_irq(irq); */
	}

	/* unlock the device */
	spin_unlock(&priv->lock);

	return IRQ_HANDLED;
}

static int ved_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *)skb->data;
	struct net_device *dev = skb->dev;

	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN - 1] ^= 0x01;	/* dest is us xor 1 */

	return 0;
}

static int ved_header(struct sk_buff *skb, struct net_device *dev, unsigned short type,
                      const void *daddr, const void *saddr, unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN - 1] ^= 0x01;	/* dest is us xor 1 */

	return dev->hard_header_len;
}

static int ved_open(struct net_device *dev)
{
	struct ved_priv *priv = netdev_priv(dev);
	struct platform_device *pdev = priv->pdev;
	const struct firmware *fw_entry;
	const char fw_name[] = {"voip.axf"};
	int rc = 0;

	/* check if the interface is already opened */

	if (priv->state == 1) {
		/* interface is opened, nothing to be done */

		return rc;
	}

	if (priv->msp.info) {
		/* stop MSP */

		del_timer(&priv->msp.timer_expire);
		priv->msp.info = NULL;
		priv->msp.state = MSP_RESET;
	}

	might_sleep();

	memset((void *)COMCERTO_MSP_VADDR, 0x00, COMCERTO_MSP_DDR_SIZE);

	if (request_firmware(&fw_entry, (char *)fw_name, &pdev->dev)) {
		printk(KERN_ERR "could not find the VoIP firmware\n");
		rc = -ETIMEDOUT;

		return rc;
	}

	priv->msp.code_info.code = fw_entry->data;
	priv->msp.code_info.size = fw_entry->size;

	if (comcerto_download_to_msp(&priv->msp)) {
		printk(KERN_ERR "failed download VoIP\n");
		rc = -EREMOTEIO;

		goto out;
	}

	if (comcerto_start_msp(&priv->msp)) {
		printk(KERN_ERR "failed start VoIP\n");
		rc = -EREMOTEIO;

		goto out;
	}

	start_ved(dev);
	napi_enable(&priv->napi);

	if (request_irq(dev->irq, ved_interrupt, IRQF_SHARED, "comcerto_ved", dev)) {
		printk(KERN_ERR "%s: failed to request irq#%d\n", dev->name, dev->irq);

		comcerto_stop_msp(&priv->msp);
		napi_disable(&priv->napi);
		stop_ved(dev);

		goto out;
	}

	netif_start_queue(dev);

	priv->state = 1;


out:
	release_firmware(fw_entry);

	return rc;
}

int ved_release(struct net_device *dev)
{
	struct ved_priv *priv = netdev_priv(dev);

	/* do nothing if interface is already down */

	if (priv->state) {
		netif_stop_queue(dev); /* can't transmit any more */

		napi_disable(&priv->napi);

		/* disable irq */
		free_irq(dev->irq, dev);

		/* reset MSP device */
		comcerto_stop_msp(&priv->msp);
		stop_ved(dev);
		priv->state = 0;
	}

	return 0;
}

static struct net_device_stats *ved_get_stats(struct net_device *dev)
{
	return &(((struct ved_priv *)netdev_priv(dev))->stats);
}

/**
 *	ved_read_packet - read and process an REALPACKET (Ethernet) frame
 *	@dev: device id
 *	@ThisFdesc: Frame descriptor to parse
 *
 *	This function read and process received frame descriptor non pre-processed by MSP . (ethernet frames)
 */
static int ved_read_packet(struct net_device *dev, struct FDesc *ThisFdesc)
{
	struct ved_priv *priv = (struct ved_priv *)netdev_priv(dev);
	char *data_addr;
	int length, offset, reason;
	struct sk_buff *skb;

	skb = dev_alloc_skb(PKT_BUF_SZ);

	if (!skb) {
		printk(KERN_WARNING "no skb was allocated: packet is dropped\n");
		priv->stats.rx_dropped++;

		return 0;
	}

	length = ThisFdesc->Length;
	offset = ThisFdesc->Offset;

	ThisFdesc->FStatus = 0;
	data_addr = (char *)(ThisFdesc->Payload + offset);

	skb_reserve(skb, NET_IP_ALIGN);
	memcpy(skb_put(skb, length), data_addr, length);
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);

	if ((reason = netif_receive_skb(skb))) {
		priv->stats.rx_dropped++;
	}

	priv->stats.rx_packets++;
	priv->stats.rx_bytes += length;
	dev->last_rx = jiffies;

	return 1;
}

static int ved_poll(struct napi_struct *napi, int budget)
{

	struct ved_priv *priv = container_of(napi, struct ved_priv, napi);
	struct net_device *dev = priv->dev;
	struct FDesc * ThisFdesc = NULL;
	int done;
	int rx_work_limit = budget;
	int received = 0;

restart_poll:
	do {
		comcerto_irq_ack(dev->irq);

		while (!smi_is_queue_empty(priv->rx_smiq.fpq)) {
			if (--rx_work_limit < 0) {
				goto not_done;
			}

			ThisFdesc = smi_dequeue(&priv->rx_smiq);

			done = ved_read_packet(dev, ThisFdesc);
			smi_free_part(ThisFdesc->fpart, ThisFdesc);

			if (!done) {
				goto not_done;
			} else {
				received++;
			}

			break;
		}
	} while (comcerto_softirq_check(dev->irq));

	/* we are happy/done, no more packets on ring; put us back
	   to where we can start processing interrupts again */
	napi_complete(napi);
	/* enable_irq(dev->irq); */

	/* The last op happens after poll completion. Which means the following:
	 * 1. it can race with disabling irqs in irq handler (which are done to
	 * schedule polls)
	 * 2. it can race with dis/enabling irqs in other poll threads
	 * 3. if an irq raised after the begining of the outer  beginning
	 * loop(marked in the code above), it will be immediately
	 * triggered here.
	 *
	 * Summarizing: the logic may results in some redundant irqs both
	 * due to races in masking and due to too late acking of already
	 * processed irqs. The good news: no events are ever lost.
	 */

	/* Let's have a last chance to process any missing interrupts we may
	 * we may have missed while the IRQs line was masqued. Before leaving
	 * poll mode, we check if data have been posted in the queue from MSP.
	 * If true, then the IRQ line is disabled again and a new loop of polling
	 * is scheduled. This double check work arround is well known in NAPI
	 * based implementation (see documentations/NAPI_HOWTO.txt - appendix2)
	 */

	if (!smi_is_queue_empty(priv->rx_smiq.fpq) && napi_reschedule(napi)) {
		/* disable_irq(dev->irq); */
		goto restart_poll;
	}

	enable_irq(dev->irq);

	return 0;

not_done:
	if (!received) {
		received = 1;
	}

	return 1;
}

static int ved_write_packet(struct sk_buff *skb, struct net_device *dev)
{
	struct ved_priv *priv = (struct ved_priv *)netdev_priv(dev);
	struct FDesc *ThisFdesc;
	u8 *buf;
	u32 offset;

	ThisFdesc = smi_alloc_part(priv->tx_smipart);

	if (!ThisFdesc) {
		dev_kfree_skb(skb);

		return -ENOMEM;
	}

	offset = NET_IP_ALIGN;
	buf = ThisFdesc->Payload + offset;

	memcpy(buf, skb->data, skb->len);

	ThisFdesc->Offset = offset;

	ThisFdesc->Next = NULL;
	ThisFdesc->Length = (u32)(skb->len);
	ThisFdesc->FStatus = 0;
	ThisFdesc->protocol = priv->default_packet_type;

	/* free the original skb (local generation) */
	dev_kfree_skb(skb);

	if (!smi_enqueue(&priv->tx_smiq, ThisFdesc)) {
		printk(KERN_WARNING "could not enqueue: queue is full: %#lx\n", (unsigned long)ThisFdesc);

		smi_free_part(priv->tx_smipart, ThisFdesc);

		return -ENOMEM;
	}

	return 0;
}

static int stop_ved(struct net_device *dev)
{
	return 0;
}

static int start_ved(struct net_device *dev)
{
	struct ved_priv *priv = (struct ved_priv *)netdev_priv(dev);
	struct msp_info *info = (struct msp_info *)priv->msp.info;

	if (!info) {
		printk(KERN_ERR "no VoIP info found\n");

		return -1;
	}

	printk(KERN_INFO "Device: %lx\n", info->device);
	printk(KERN_INFO "Revision: %lx\n", info->revision);
	printk(KERN_INFO "CSPtoMSPQueuePhyaddr: %#lx\n", info->CSPtoMSPQueuePhyaddr);
	printk(KERN_INFO "MSPtoCSPQueuePhyaddr: %#lx\n", info->MSPtoCSPQueuePhyaddr);
	printk(KERN_INFO "SMRXCSPhyaddr: %#lx\n", info->SMRXCSPhyaddr);
	printk(KERN_INFO "SMTXCSPhyaddr: %#lx\n", info->SMTXCSPhyaddr);

	printk(KERN_INFO "DSP version: %s\n", info->spu_version);
	printk(KERN_INFO "VoIP version: %s\n", info->msp_version);

	priv->tx_smipart = (struct fastpart *)(info->SMTXCSPhyaddr);
	priv->rx_smipart = (struct fastpart *)(info->SMRXCSPhyaddr);

	smi_queue_init(&priv->tx_smiq, info->CSPtoMSPQueuePhyaddr, smi_gen_msp_irq);
	smi_queue_init(&priv->rx_smiq, info->MSPtoCSPQueuePhyaddr, NULL);

	ved_ack_msp();

	return 0;
}

static const struct net_device_ops ctrl_netdev_ops = {
	.ndo_open = ved_open,
	.ndo_stop = ved_release,
	.ndo_start_xmit = ved_xmit,
	.ndo_get_stats = ved_get_stats,
};

static const struct header_ops ctrl_header_ops = {
	.create = ved_header,
	.rebuild = ved_rebuild_header,
};

static int ved_probe(struct platform_device *pdev)
{
	struct net_device *dev = NULL;
	struct ved_priv *priv= NULL;
	int rc;

	/* create an ethernet device instance */

	dev = alloc_etherdev(sizeof (*priv));

	if (!dev) {
		printk(KERN_ERR "gemac %d device allocation failed\n", pdev->id);
		rc = -ENOMEM;

		goto err0;
	}

	priv = netdev_priv(dev);

	memset(priv, 0, sizeof(struct ved_priv));

	spin_lock_init(&priv->lock);

	/* init private section */
	priv->default_packet_type = PROTID_ETH;
	priv->state = 0;		  /* closed */

	priv->pdev = pdev;
	priv->dev = dev;
	platform_set_drvdata(pdev, dev);

	/* copy the station address into the dev structure */
	memcpy(dev->dev_addr, ctrl_mac, ETH_ALEN);

	if (dev_alloc_name(dev, CTRL)) {
		printk(KERN_ERR "could not allocate net device name %s\n", CTRL);
		rc = -EINVAL;

		goto err2;
	}

	SET_NETDEV_DEV(dev, &pdev->dev);

	/* fill in device structure with ethernet-generic values */
	ether_setup(dev);

	/* initialize the device structure */
	dev->netdev_ops = &ctrl_netdev_ops;
	dev->header_ops = &ctrl_header_ops;
	dev->destructor = free_netdev;

	/* fill in device structure with ethernet-generic values */
	dev->tx_queue_len = 0;
	dev->flags |= IFF_NOARP;
	dev->flags &= ~IFF_MULTICAST;
	dev->irq = platform_get_irq_byname(pdev, "irq");

	netif_napi_add(dev, &priv->napi, ved_poll, VED_POLL_WEIGHT);

	skb_queue_head_init(&priv->msp.msp_list);

	rc = register_netdev(dev);

	if (rc) {
		printk(KERN_ERR "could not register net device\n");

		goto err0;
	}

	/* create all the sysfs files */
	msp_init_sysfs(pdev);

	return 0;


err2:
	platform_set_drvdata(pdev, NULL);
	free_netdev(dev);

err0:
	return rc;
}

static int ved_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);

	unregister_netdevice(dev);
	platform_set_drvdata(pdev, NULL);
	free_netdev(dev);

	return 0;
}

/* Structure for a device driver */
static struct platform_driver ved_driver = {
	.probe = ved_probe,
	.remove = ved_remove,
	.driver = {
		.name = "ved",
	},
};

static int __init ved_init(void)
{
	return platform_driver_register(&ved_driver);
}

static void __exit ved_exit(void)
{
	platform_driver_unregister(&ved_driver);
}


module_init(ved_init);
module_exit(ved_exit);
