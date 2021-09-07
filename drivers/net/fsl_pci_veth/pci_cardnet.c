/*
 * Copyright (C) 2005-2010 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xiaobo Xie <X.Xie@freescale.com>
 *	Roy Zang <tie-fei.zang@freescale.com>
 *	Jason Jin <jason.jin@freescale.com>
 *
 * Description:
 * PCI Agent/EP Demo Driver for Freescale MPC85xx Processor(Agent/EP side)
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <linux/moduleparam.h>
#include <linux/of_platform.h>

#include <asm/page.h>
#include <asm/fsl_msg.h>
#include <sysdev/fsl_pci.h>

#include "pci_veth.h"

static int eth;
module_param(eth, int, 0);

/*Define which pci port is configured as Agent/EP */
#if !defined(CONFIG_PCI_EP_N)
static unsigned int pci_ep_n = 3;
#else
static unsigned int pci_ep_n = CONFIG_PCI_EP_N;
#endif

static int link_up;
static int host_open;

#define HAVE_TX_TIMEOUT

struct share_mem {
	u32	share_flag;
	u32	msg_group_host2ep;
	u32 	msg_num_host2ep;
	u32	msg_group_ep2host;
	u32 	msg_num_ep2host;
	u32	hstatus;
	u32	astatus;

	u32	rx_flags;
	u32	rx_packetlen;
	u8	rxbuf[2*1024 - 22];

	u32	tx_flags;
	u32	tx_packetlen;
	u8	txbuf[2*1024 - 22];
};

struct pci_agent_dev {
	u32	local_addr;
	u32	local_mem_size;
	u32	shmem_inb_win_num;
	u32	irq;
	struct  fsl_msg_unit *msg_host2ep;
	struct  fsl_msg_unit *msg_ep2host;
	void __iomem *pci_agent_regs;
};

struct card_priv {
	struct share_mem *share_mem;

	struct sk_buff *cur_tx_skb;
	int rx_packetlen;
	int tx_packetlen;
	spinlock_t lock; /* lock for set card_priv */
	struct net_device_stats stats;
	struct pci_agent_dev *pci_agent_dev;
};
static int card_open(struct net_device *dev);
static int card_release(struct net_device *dev);
static int card_config(struct net_device *dev, struct ifmap *map);
static void card_rx(struct net_device *dev, int len, unsigned char
*buf);
static int card_tx(struct sk_buff *skb, struct net_device *dev);
static void card_tx_timeout(struct net_device *dev);
static int card_ioctl(struct net_device *dev, struct ifreq *rq, int
cmd);
static struct net_device_stats *card_stats(struct net_device *dev);
static int card_rebuild_header(struct sk_buff *skb);
static int card_header(struct sk_buff *skb, struct net_device *dev,
		unsigned short type, const void *daddr, const void
*saddr,
		unsigned int len);
static int card_change_mtu(struct net_device *dev, int new_mtu);
static irqreturn_t card_interrupt(int irq, void *dev_id);

static const struct header_ops card_header_ops = {
	.create		= card_header,
	.rebuild	= card_rebuild_header,
	.cache  	= NULL,
	.cache_update	= NULL,
	.parse		= NULL,
};

static const struct net_device_ops card_netdev_ops = {
	.ndo_open = card_open,
	.ndo_start_xmit = card_tx,
	.ndo_stop = card_release,
	.ndo_change_mtu = card_change_mtu,
	.ndo_tx_timeout = card_tx_timeout,
	.ndo_do_ioctl = card_ioctl,
	.ndo_set_config = card_config,
	.ndo_get_stats = card_stats,
};

void setup_agent_shmem_inb_win(struct pci_agent_dev *dev)
{
	struct ccsr_pci __iomem *pci;
	u32 winno = dev->shmem_inb_win_num;

	/*The inb piwbar was set the host during enum,
	 *we only need to set the piwtar
	 */
	pci = (struct ccsr_pci *)dev->pci_agent_regs;
	out_be32(&pci->piw[winno].pitar, dev->local_addr>>12);
	out_be32(&pci->piw[winno].piwar, PIWAR_EN | PIWAR_TRGT_MEM |
			PIWAR_RTT_SNOOP | PIWAR_WTT_SNOOP |
			(__ilog2(dev->local_mem_size) - 1));
}

static int card_open(struct net_device *dev)
{
	int retval;
	struct card_priv *tp = netdev_priv(dev);

	/*Clear the message intr from host*/
	fsl_clear_msg(tp->pci_agent_dev->msg_host2ep);

	if (host_open) {
		netif_carrier_on(dev);
		netif_start_queue(dev);
		link_up = 1;
		printk(KERN_INFO"%s is up\n", dev->name);
	} else
		/*Do not know if host is Open, Ping it.*/
		fsl_send_msg(tp->pci_agent_dev->msg_ep2host, AGENT_UP);

	retval = request_irq(dev->irq, card_interrupt, 0,
					dev->name, dev);
	if (retval) {
		dev_err(&dev->dev, "Can not request irq for cardnet!\n");
		return retval;
	}

	return 0;
}

static int card_release(struct net_device *dev)
{
	struct card_priv *tp = netdev_priv(dev);
	struct pci_agent_dev *pci_dev = tp->pci_agent_dev;

	netif_stop_queue(dev); /* can't transmit any more */
	netif_carrier_off(dev);
	link_up = 0;

	synchronize_irq(dev->irq);
	free_irq(dev->irq, dev);
	tp->share_mem->hstatus = 0; /*clear the pending data for host*/
	fsl_clear_msg(pci_dev->msg_ep2host);
	if (host_open) {
		fsl_send_msg(pci_dev->msg_ep2host, AGENT_DOWN);
		host_open = 0;
	}

	printk(KERN_INFO "%s is down\n", dev->name);

	return 0;
}

static int card_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;
	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		dev_warn(&dev->dev, "cardnet: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}
	/* Allow changing the IRQ */
	if (map->irq != dev->irq)
		dev->irq = map->irq;

	/* ignore other fields */
	return 0;
}

/*
 * Receive a packet: retrieve, encapsulate and pass over to upper levels
 */
static void card_rx(struct net_device *dev, int len, unsigned char *buf)
{
	struct card_priv *priv = netdev_priv(dev);
	struct sk_buff *skb;

	skb = dev_alloc_skb(len+2);
	if (!skb) {
		dev_err(&dev->dev, "card rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		return;
	}

	skb_reserve(skb, 2);
	memcpy(skb_put(skb, len), buf, len);
	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += len;

	netif_rx(skb);
	return;
}

/*
 * The interrupt entry point
 */
static irqreturn_t card_interrupt(int irq, void *dev_id)
{
	struct card_priv *priv;
	struct share_mem *shmem;
	u32  statusword = 0;
	int len;
	struct net_device *dev = (struct net_device *)dev_id;

	if (!dev)
		return IRQ_NONE;
	priv =  netdev_priv(dev);

	spin_lock(&priv->lock);
	shmem = (struct share_mem *) priv->share_mem;

	fsl_read_msg(priv->pci_agent_dev->msg_host2ep, &statusword);

	if (statusword & HOST_SENT) {
		if (!link_up) {
			netif_carrier_on(dev);
			netif_start_queue(dev);
			link_up = 1;
			host_open = 1;
		}
		len = shmem->rx_packetlen;
		card_rx(dev, len, shmem->rxbuf);
		shmem->astatus = AGENT_GET;
	} else if (statusword & HOST_UP) {
		if (!link_up) {
			netif_carrier_on(dev);
			netif_start_queue(dev);
			link_up = 1;
			fsl_send_msg(priv->pci_agent_dev->msg_ep2host, AGENT_UP);
			host_open = 1;
		}
	} else if (statusword & HOST_DOWN) {
		if (link_up) {
			link_up = 0;
			host_open = 0;
			netif_carrier_off(dev);
			netif_stop_queue(dev);
		}
	} else {
		printk(KERN_INFO "The message is not for me!message=0x%x\n",
					statusword);
		spin_unlock(&priv->lock);
		return IRQ_NONE;
	}

	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

static int card_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	int time_out = 1000;
	int count = 0;
	char *data;
	struct card_priv *priv = netdev_priv(dev);
	struct share_mem *shmem =
				(struct share_mem *)priv->share_mem;

	while (shmem->hstatus) {
		udelay(5);
		time_out--;
		if (!time_out) {
			dev_err(&dev->dev, "Timeout, host busy!\n");
			netif_stop_queue(dev);
			priv->stats.tx_dropped++;
			dev_kfree_skb(skb);
			return 0;
		}
	}

	len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		dev_info(&dev->dev, "packet too short (%i octets)\n", len);
		priv->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return 0;
	}
	if (len > 2026) {
		dev_info(&dev->dev, "packet too long (%i octets)\n", len);
		priv->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return 0;
	}

	data = skb->data;
	dev->trans_start = jiffies; /* save the timestamp */
	priv->cur_tx_skb = skb;
	priv->tx_packetlen = len;

	shmem->tx_flags = ++count;
	shmem->tx_packetlen = len;
	memcpy(shmem->txbuf, data, len);
	shmem->hstatus = AGENT_SENT;

	priv->stats.tx_packets++;
	priv->stats.tx_bytes += priv->tx_packetlen;

	fsl_send_msg(priv->pci_agent_dev->msg_ep2host, AGENT_SENT);

	dev_kfree_skb(priv->cur_tx_skb);

	return 0;
}

/*
 * Deal with a transmit timeout.
 */
static void card_tx_timeout(struct net_device *dev)
{
	struct card_priv *priv = netdev_priv(dev);

	dev_info(&dev->dev, "Transmit timeout at %ld, latency %ld\n",
		jiffies, jiffies - dev->trans_start);

	priv->stats.tx_errors++;

	fsl_clear_msg(priv->pci_agent_dev->msg_ep2host);

	/*Try to kick the other side when timeout*/
	fsl_send_msg(priv->pci_agent_dev->msg_ep2host, AGENT_UP);

	netif_wake_queue(dev);
	return;
}

static int card_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	return 0;
}

static struct net_device_stats *card_stats(struct net_device *dev)
{
	struct card_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

static int card_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01; /* dest is us xor 1 */

	return 0;
}

static int card_header(struct sk_buff *skb, struct net_device *dev,
		unsigned short type, const void *daddr, const void *saddr,
		unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;
	return dev->hard_header_len;
}

static int card_change_mtu(struct net_device *dev, int new_mtu)
{
	spinlock_t *lock = &((struct card_priv *) netdev_priv(dev))->lock;
	unsigned long flags;

	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}

static void card_init(struct net_device *dev)
{
	ether_setup(dev); /* assign some of the fields */
	dev->netdev_ops	= &card_netdev_ops;
	dev->header_ops = &card_header_ops;

	memcpy(dev->dev_addr, "\0FSLD0", ETH_ALEN);

	dev->watchdog_timeo	= 5 * HZ;
	/* keep the default flags, just add NOARP */
	dev->flags	|= IFF_NOARP;
	dev->features	|= NETIF_F_NO_CSUM;

	return;
}

static int cardnet_priv_init(struct net_device *dev)
{
	struct card_priv *priv;
	struct fsl_msg_unit *msg_unit_host2ep = NULL, *msg_unit_ep2host = NULL;
	struct pci_agent_dev *pci_agent_dev;
	struct device_node *np;
	const unsigned int *cell_index = NULL;
	u32 shmem_phys_addr;

	/* Allocate the priv field. */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct card_priv));

	spin_lock_init(&((struct card_priv *) netdev_priv(dev))->lock);

	priv->share_mem = (struct share_mem *)
				__get_free_pages(GFP_ATOMIC, NEED_LOCAL_PAGE);
	if (priv->share_mem == NULL) {
		dev_err(&dev->dev, "Can not allocate pages for sharing mem!\n");
		return -ENOMEM;
	}

	shmem_phys_addr = virt_to_phys((void *)priv->share_mem);
	memset(priv->share_mem, 0, PAGE_SIZE << NEED_LOCAL_PAGE);

	priv->pci_agent_dev = (struct pci_agent_dev *)
			kmalloc(sizeof(struct pci_agent_dev), GFP_KERNEL);
	if (priv->pci_agent_dev == NULL) {
		dev_err(&dev->dev, "Can not allocate memory!\n");
		goto err_out2;
	}

	memset(priv->pci_agent_dev, 0, sizeof(struct pci_agent_dev));

	pci_agent_dev = priv->pci_agent_dev;
	pci_agent_dev->local_addr = shmem_phys_addr;

	for_each_node_by_type(np, "pci") {
		if (of_device_is_compatible(np, "fsl,mpc8540-pci") ||
		    of_device_is_compatible(np, "fsl,mpc8548-pcie") ||
		    of_device_is_compatible(np, "fsl,p1022-pcie")) {
			cell_index = of_get_property(np, "cell-index", NULL);
			if (cell_index && (*cell_index == pci_ep_n))
				break;
		}
	}
	if (!cell_index || *cell_index != pci_ep_n) {
		dev_err(&dev->dev, "Can not find the assigned EP node!\n");
		goto err_out1;
	}

	pci_agent_dev->pci_agent_regs = of_iomap(np, 0);
	of_node_put(np);
	if (!pci_agent_dev->pci_agent_regs) {
		dev_err(&dev->dev, "Can not map the pci resoure for EP!\n");
		goto err_out1;
	}

	/*inbound winno 3, is the only inbound window for 32bit mem,
	 *used to map the share mem
	 */
	pci_agent_dev->local_mem_size = AGENT_MEM_SIZE;
	pci_agent_dev->shmem_inb_win_num = 2;
	setup_agent_shmem_inb_win(pci_agent_dev);

	/*Request message interrupt resource*/
	msg_unit_host2ep = fsl_get_msg_unit();
	msg_unit_ep2host = fsl_get_msg_unit();
	if (!msg_unit_host2ep || !msg_unit_ep2host ||
		(msg_unit_host2ep->requested == false) ||
		(msg_unit_ep2host->requested == false)) {
			dev_info(&dev->dev,
					"can not allocate message interrupt!");
			goto err_out1;
	}

	fsl_enable_msg(msg_unit_host2ep);
	fsl_enable_msg(msg_unit_ep2host);

	/*Set the message for ep2host routing to irq_out*/
	fsl_msg_route_int_to_irqout(msg_unit_ep2host);

	priv->share_mem->msg_group_host2ep =
		msg_unit_host2ep->msg_group_addr_offset;
	priv->share_mem->msg_group_host2ep =
		msg_unit_host2ep->msg_group_addr_offset;

	priv->share_mem->msg_num_host2ep = msg_unit_host2ep->msg_num;
	priv->share_mem->msg_group_ep2host =
		msg_unit_ep2host->msg_group_addr_offset;
	priv->share_mem->msg_num_ep2host = msg_unit_ep2host->msg_num;

	pci_agent_dev->msg_host2ep = msg_unit_host2ep;
	pci_agent_dev->msg_ep2host = msg_unit_ep2host;

	dev->irq = msg_unit_host2ep->irq;

	/*Put a flag in the header of the share_mem*/
	priv->share_mem->share_flag = SHARE_FLAG;
	return 0;

err_out1:
	kfree(priv->pci_agent_dev);
err_out2:
	kfree(priv->share_mem);
	return -1;
}

static struct net_device *card_devs;

static __init int card_init_module(void)
{
	int result, device_present = 0;
	int card_eth;
	char interface_name[16];

	card_eth = eth; /* copy the cfg datum in the non-static place */
	if (!card_eth)
		strcpy(interface_name, "ceth%d");
	else
		strcpy(interface_name, "eth%d");

	card_devs = alloc_netdev(sizeof(struct card_priv),
					interface_name, card_init);
	if (card_devs == NULL)
		return -ENODEV;

	if (cardnet_priv_init(card_devs))
		return -ENODEV;

	netif_carrier_off(card_devs);
	result = register_netdev(card_devs);
	if (result) {
		printk(KERN_ERR "card: error %i registering device \"%s\"\n",
			result, interface_name);
		free_netdev(card_devs);
	} else
		device_present++;
	printk(KERN_INFO "register device named-----%s\n", card_devs->name);
	printk(KERN_INFO "mpc85xx agent drvier init succeed\n");

	return device_present ? 0 : -ENODEV;
}

static __exit void card_cleanup(void)
{

	struct card_priv *tp = netdev_priv(card_devs);
	struct share_mem *share_mem = (struct share_mem *)tp->share_mem;
	struct pci_agent_dev *pci_dev = tp->pci_agent_dev;

	free_pages((unsigned long)share_mem, NEED_LOCAL_PAGE);

	iounmap(pci_dev->pci_agent_regs);
	kfree(tp->pci_agent_dev);

	fsl_release_msg_unit(pci_dev->msg_ep2host);
	fsl_release_msg_unit(pci_dev->msg_host2ep);

	unregister_netdev(card_devs);
	free_netdev(card_devs);

	return;
}
module_init(card_init_module);
module_exit(card_cleanup);

MODULE_AUTHOR("Xiaobo Xie<X.Xie@freescale.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MPC85xx PCI Agent/EP Demo Driver(Agent/EP side)");
