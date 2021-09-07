/*
 * Copyright (C) 2005-2010 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xiaobo Xie <r63061@freescale.com>
 *	Roy Zang <tie-fei.zang@freescale.com>
 *	Jason Jin <jason.jin@freescale.com>
 *
 * Description:
 * PCI Agent/EP Demo Driver for Freescale MPC85xx Processor(Host/RC side)
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
#include <linux/pci.h>
#include <linux/of_platform.h>

#include <asm/fsl_msg.h>

#include "pci_veth.h"

#define HAVE_TX_TIMEOUT

#undef BOARDNET_NDEBUG

#ifdef BOARDNET_NDEBUG
# define assert(expr) do {} while (0)
#else
# define assert(expr) \
	if (unlikely(!(expr))) {					\
		printk(KERN_ERR "Assertion failed! %s,%s,%s,line=%d\n",	\
		#expr, __FILE__, __func__, __LINE__);			\
	}
#endif

struct share_mem {
	u32	share_flag;
	u32	msg_group_host2ep;
	u32 	msg_num_host2ep;
	u32	msg_group_ep2host;
	u32 	msg_num_ep2host;
	u32	hstatus;
	u32	astatus;

	u32	tx_flags;
	u32	tx_packetlen;
	u8	txbuf[2*1024 - 22];

	u32	rx_flags;
	u32	rx_packetlen;
	u8	rxbuf[2*1024 - 22];
};

struct boardnet_private {
	u32 m_immrbar;
	void *m_ioaddr;
	struct net_device_stats stats;
	struct pci_dev *pci_dev;

	int irq;
	spinlock_t lock; /* lock for set private data */

	struct sk_buff *skb;
};

static int boardnet_open(struct net_device *dev);
static int boardnet_release(struct net_device *dev);
static int boardnet_config(struct net_device *dev, struct ifmap *map);
static void boardnet_hw_tx(char *buf, int len, struct net_device *dev);
static int boardnet_start_xmit(struct sk_buff *skb, struct net_device *dev);
static void boardnet_rx(struct net_device *dev);
static void boardnet_tx_timeout(struct net_device *dev);
static int boardnet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static struct net_device_stats *boardnet_get_stats(struct net_device *dev);
static int boardnet_rebuild_header(struct sk_buff *skb);
static int boardnet_header(struct sk_buff *skb, struct net_device *dev,
			unsigned short type, const void *daddr,
			const void *saddr, unsigned int len);
static int boardnet_change_mtu(struct net_device *dev, int new_mtu);
static irqreturn_t boardnet_interrupt(int irq, void *dev_id);

static int eth;
static struct fsl_msg_unit msg_unit_host2ep;
static struct fsl_msg_unit msg_unit_ep2host;
static int link_up;
static int agent_open;

static const struct header_ops boardnet_header_ops = {
	.create		= boardnet_header,
	.rebuild	= boardnet_rebuild_header,
	.cache  	= NULL,
	.cache_update	= NULL,
	.parse		= NULL,
};

static const struct net_device_ops boardnet_netdev_ops = {
	.ndo_open = boardnet_open,
	.ndo_start_xmit = boardnet_start_xmit,
	.ndo_stop = boardnet_release,
	.ndo_change_mtu = boardnet_change_mtu,
	.ndo_tx_timeout = boardnet_tx_timeout,
	.ndo_do_ioctl = boardnet_ioctl,
	.ndo_set_config = boardnet_config,
	.ndo_get_stats = boardnet_get_stats,
};

static int boardnet_open(struct net_device *dev)
{
	int retval;

	/*Clear the message intr from host*/
	fsl_clear_msg(&msg_unit_ep2host);

	if (agent_open) {
		netif_carrier_on(dev);
		netif_start_queue(dev);
		link_up = 1;
		dev_info(&dev->dev, "%s is up\n", dev->name);
	} else
		/* Donot know if EP is open, Looks like a ping,
		 * If the EP is open, this will make it link_up,
		 * otherwise, both keep link_down.
		 * If the agent was removed. The host also should remove.
		 */
		fsl_send_msg(&msg_unit_host2ep, HOST_UP);


	retval = request_irq(dev->irq, boardnet_interrupt, 0,
				dev->name, dev);
	if (retval) {
		dev_err(&dev->dev, "can not request irq %d\n", dev->irq);
		return retval;
	}

	return 0;
}

static int boardnet_release(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;

	netif_stop_queue(dev); /* can't transmit any more */
	netif_carrier_off(dev);
	link_up = 0;

	synchronize_irq(dev->irq);
	free_irq(dev->irq, dev);
	fsl_clear_msg(&msg_unit_host2ep);
	shmem->astatus = 0; /*clear the pending data for agent*/
	if (agent_open) {
		fsl_send_msg(&msg_unit_host2ep, HOST_DOWN);
		agent_open = 0;
	}

	printk(KERN_INFO "%s is down\n", dev->name);
	return 0;
}

static int boardnet_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP)
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		dev_warn(&dev->dev, "Boardnet: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Allow changing the IRQ */
	if (map->irq != dev->irq)
		dev->irq = map->irq;

	return 0;
}

static void boardnet_rx(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;
	int len = 0;
	struct sk_buff *skb;
	u32 *src;
	u32 *dest;
	u32 skblen, temp;

	len = shmem->rx_packetlen;
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		dev_dbg(&dev->dev, "Packet is too short (%i octets)\n",
			shmem->rx_packetlen);
		shmem->hstatus = HOST_GET;
		priv->stats.rx_errors++;
		return;
	}

	skb = dev_alloc_skb(len + 2);
	if (!skb) {
		dev_warn(&dev->dev, "Rx: low on mem-packet dropped\n");
		priv->stats.rx_dropped++;
		return;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */

	dest = (u32 *)skb_put(skb, len);
	src = (u32 *)shmem->rxbuf;
	skblen = len;

	while (skblen > 0) {
		if (skblen < 4) {
			temp = in_be32(src);
			memcpy(dest, &(temp), skblen);
			break;
		} else {
			*dest = in_be32(src);
		}
		src++;
		dest++;
		skblen = skblen - 4;
	}

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += len;

	shmem->hstatus = HOST_GET;
	netif_rx(skb);
	return;
}

/*
 * The interrupt entry point
 */
static irqreturn_t boardnet_interrupt(int irq, void *dev_id)
{
	uint32_t statusword;

	struct net_device *dev = (struct net_device *)dev_id;
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *smem;

	if (!dev)
		return IRQ_NONE;

	smem = (struct share_mem *)priv->m_ioaddr;

	/* Lock the device */
	spin_lock(&priv->lock);

	fsl_read_msg(&msg_unit_ep2host, &statusword);

	if (statusword & AGENT_SENT) {
		if (!link_up) {
			netif_carrier_on(dev);
			netif_start_queue(dev);
			link_up = 1;
			agent_open = 1;
		}
		boardnet_rx(dev);
	} else if (statusword & AGENT_UP) {
		if (!link_up) {
			netif_carrier_on(dev);
			netif_start_queue(dev);
			link_up = 1;
			/*Sometimes, agent only send a mesage but
			 *did not really up.
			 */
			fsl_send_msg(&msg_unit_host2ep, HOST_UP);
			agent_open = 1;
		}
	} else if (statusword & AGENT_DOWN) {
		netif_carrier_off(dev);
		netif_stop_queue(dev);
		link_up = 0;
		agent_open = 0;
	} else {
		dev_info(&dev->dev, "This message is not for me! msg=0x%x\n",
			statusword);
		spin_unlock(&priv->lock);
		return IRQ_NONE;
	}

	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

/*
 * Transmit a packet (low level interface)
 */
static void boardnet_hw_tx(char *buf, int len, struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;
	u32 *src;
	u32 *dest;
	u32 skblen, temp;

	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		pr_debug("Packet is too short (%i octets)\n", len);
		priv->stats.tx_errors++;
		dev_kfree_skb(priv->skb);
		return;
	}

	if (len > 2026) {
		pr_debug(KERN_INFO "Packet too long (%i octets)\n", len);
		priv->stats.tx_dropped++;
		dev_kfree_skb(priv->skb);
		return;
	}

	/* Send out the packet */
	src = (u32 *)buf;
	dest = (u32 *)shmem->txbuf;
	skblen = len;

	while (skblen > 0) {
		if (skblen < 4) {
			memcpy(&(temp), src, skblen);
			out_be32(dest, temp);
			break;
		} else {
			out_be32(dest, *src);
		}
		src++;
		dest++;
		skblen -= 4;
	}

	shmem->tx_packetlen = len;

	/* Update the statitic data */
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += len;

	/* Set the flag, indicating the peer that the packet has been sent */
	shmem->astatus = HOST_SENT;

	fsl_send_msg(&msg_unit_host2ep, HOST_SENT);

	dev_kfree_skb(priv->skb);
	return;
}

static int boardnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;
	char *data, shortpkt[ETH_ZLEN];
	int time_out = 100;
	int len;

	while (shmem->astatus) {
		udelay(2);
		time_out--;
		if (!time_out) {
			dev_dbg(&dev->dev, "Timeout, agent busy!\n");
			netif_stop_queue(dev);
			priv->stats.tx_dropped++;
			dev_kfree_skb(skb);
			return 0;
		}
	}

	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	dev->trans_start = jiffies; /* save the timestamp */

	/* Remember the skb, so we can free it at interrupt time */
	priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	boardnet_hw_tx(data, len, dev);
	return 0;
}

/*
 * Deal with a transmit timeout.
 */
static void boardnet_tx_timeout(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);

	dev_info(&dev->dev, "Transmit timeout at %ld, latency %ld\n",
		jiffies, jiffies - dev->trans_start);

	priv->stats.tx_errors++;

	fsl_clear_msg(&msg_unit_host2ep);

	/*When timeout, try to kick the EP*/
	fsl_send_msg(&msg_unit_host2ep, HOST_UP);

	netif_wake_queue(dev);
	return;
}

static int boardnet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	pr_debug("ioctl\n");
	return 0;
}

static struct net_device_stats *boardnet_get_stats(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	return &priv->stats;
}

static int boardnet_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01; /* dest is us xor 1 */
	return 0;
}

/*
 * This function is called to fill up an eth header, since arp is not
 * available on the interface
 */
static int
boardnet_header(struct sk_buff *skb, struct net_device *dev,
		unsigned short type, const void *daddr, const void *saddr,
		unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01; /* dest is us xor 1 */

	return dev->hard_header_len;
}

static int boardnet_change_mtu(struct net_device *dev, int new_mtu)
{
	struct boardnet_private *priv = netdev_priv(dev);
	spinlock_t *lock = &priv->lock;
	unsigned long flags;

	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	/*
	 * Do anything you need, and the accept the value
	 */
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}

/*
 * Cleanup
 */
static void boardnet_cleanup(struct pci_dev *pdev)
{
	struct net_device *dev;
	struct boardnet_private *priv;

	dev = pci_get_drvdata(pdev);
	priv = netdev_priv(dev);
	/* Unmap the space address */
	iounmap((void *)(priv->m_immrbar));
	iounmap((void *)(priv->m_ioaddr));
	free_netdev(dev);
	pci_disable_msi(pdev);
	/* Release region */
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	return;
}

/*
 * Called when remove the device
 */
static __devexit void boardnet_remove(struct pci_dev *pdev)
{
	struct net_device *dev;

	dev = pci_get_drvdata(pdev);
	unregister_netdev(dev);
	boardnet_cleanup(pdev);
	/* Clear the device pointer in PCI */
	pci_set_drvdata(pdev, NULL);
}

/*
 * Called to initialize the board
 */
static __devinit int
boardnet_board_init(struct pci_dev *pdev, struct net_device **dev_out)
{
	struct net_device *dev;
	struct boardnet_private *priv;
	void *mapped_immrbar;
	int retval;

	/* Enable device */
	retval = pci_enable_device(pdev);
	if (retval) {
		printk(KERN_ERR "Cannot enable device\n");
		return retval;
	}

	retval = pci_request_regions(pdev, "boardnet");
	if (retval) {
		printk(KERN_ERR "%s: Cannot reserve region, aborting\n",
			pci_name(pdev));
		pci_disable_device(pdev);
		return -ENODEV;
	}

	/* Enable PCI bus mastering */
	pci_set_master(pdev);

	*dev_out = NULL;
	/*
	 * Allocate and set up an ethernet device.
	 * dev and dev->priv zeroed in alloc_etherdev.
	 */
	dev = alloc_etherdev(sizeof(struct boardnet_private));
	if (dev == NULL) {
		printk(KERN_ERR PFX "%s: Unable to alloc new net device\n",
			pci_name(pdev));
		pci_release_regions(pdev);
		pci_disable_device(pdev);
		return -ENODEV;
	}

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct boardnet_private));

	mapped_immrbar = pci_ioremap_bar(pdev, 0);
	if (!mapped_immrbar) {
		printk(KERN_ERR "%s: Cannot remap memory, aborting\n",
			pci_name(pdev));
		pci_release_regions(pdev);
		pci_disable_device(pdev);
		retval = -EIO;
	}
	priv->m_immrbar = (uint32_t)mapped_immrbar;

	/*The BAR1, used for share mem*/
	priv->m_ioaddr = ioremap(pci_resource_start(pdev, 1), AGENT_MEM_SIZE);

	priv->pci_dev = pdev;
	*dev_out = dev;
	return 0;
}

static int __devinit boardnet_probe(struct pci_dev *pdev,
					const struct pci_device_id *id)
{
	struct net_device *dev = NULL;
	struct boardnet_private *priv;
	int boardnet_eth = eth;
	int retval;
	struct share_mem *shmem;

	assert(pdev != NULL);
	assert(id != NULL);

	retval = boardnet_board_init(pdev, &dev);
	if (retval)
		return retval;

	assert(dev != NULL);
	priv = netdev_priv(dev);
	assert(priv != NULL);

	if (!boardnet_eth)
		strcpy(dev->name, "beth%d");
	else
		strcpy(dev->name, "eth%d");
	dev->watchdog_timeo	= 5 * HZ;
	dev->netdev_ops		= &boardnet_netdev_ops;
	dev->header_ops		= &boardnet_header_ops;
	dev->addr_len		= ETH_ALEN;
	dev->hard_header_len 	= ETH_HLEN;

	/* keep the default flags, just add NOARP */
	dev->flags		|= IFF_NOARP;
	dev->features		|= NETIF_F_NO_CSUM;

	/* For PCIE EP, we should enable the MSI support in the system
	 * For PCI agent, the agent irq_out line should connect to the
	 * host intA~intD.
	 */
#ifdef CONFIG_PCI_MSI
	if (pci_enable_msi(pdev)) {
		printk(KERN_ERR "Can not enable MSI for PCIE endpoint device!");
		iounmap((void *)(priv->m_immrbar));
		iounmap((void *)(priv->m_ioaddr));
		free_netdev(dev);
		pci_release_regions(pdev);
		return -1;
	}
#endif

	dev->irq		= pdev->irq;
	memcpy(dev->dev_addr, "\0FSLD1", ETH_ALEN);

	/* initialize lock */
	spin_lock_init(&(priv->lock));

	pci_set_drvdata(pdev, dev);

	shmem = (struct share_mem *)priv->m_ioaddr;
	/*If can not get the share flag, must be something error*/
	if (shmem->share_flag != SHARE_FLAG) {
		printk(KERN_INFO "Something error in share memory! "
				"Please check the RC and EP settings\n");
		boardnet_cleanup(pdev);
		return -1;
	}

	netif_carrier_off(dev);
	retval = register_netdev(dev);
	if (retval) {
		printk(KERN_ERR "%s: Cannot register device, aborting\n",
			dev->name);
		boardnet_cleanup(pdev);
		return retval;
	}

	/*The message unit host2ep used to trig the message to ep
	 * when sending the packet to ep.
	 */
	memset(&msg_unit_host2ep, 0, sizeof(msg_unit_host2ep));
	msg_unit_host2ep.msg_addr = (u32 __iomem *)(priv->m_immrbar
				 + shmem->msg_group_host2ep
				 + 0x10 * shmem->msg_num_host2ep);
	msg_unit_host2ep.msg_num = shmem->msg_num_host2ep;
	msg_unit_host2ep.mer = (u32 __iomem *) (priv->m_immrbar +
			shmem->msg_group_host2ep + 0x100);
	msg_unit_host2ep.msr = (u32 __iomem *) (priv->m_immrbar +
			shmem->msg_group_host2ep + 0x110);

	/*We need the message unit ep2host to read the message which
	 * written by the ep
	 */
	memset(&msg_unit_ep2host, 0, sizeof(msg_unit_ep2host));
	msg_unit_ep2host.msg_addr = (u32 __iomem *)(priv->m_immrbar
				 + shmem->msg_group_ep2host
				 + 0x10 * shmem->msg_num_ep2host);
	msg_unit_ep2host.msg_num = shmem->msg_num_ep2host;
	msg_unit_ep2host.mer = (u32 __iomem *) (priv->m_immrbar +
			shmem->msg_group_ep2host + 0x100);
	msg_unit_ep2host.msr = (u32 __iomem *) (priv->m_immrbar +
			shmem->msg_group_ep2host + 0x110);

	printk(KERN_INFO "register device named-----%s\n", dev->name);

	return 0;
}

static int boardnet_suspend(struct pci_dev *pdev, pm_message_t state)
{
	return 0;
}

static int boardnet_resume(struct pci_dev *pdev)
{
	return 0;
}

/*
 * List devices that this driver support
 */
static struct pci_device_id boardnet_id_table[] = {
	/* Vendor_id, Device_id
	* Subvendor_id, Subdevice_id
	* Class_id, Class
	* Driver_data
	*/
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_P1021E, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_MPC8568E, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_MPC8536E, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_P1022E, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_P1013E, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{0,}
};
MODULE_DEVICE_TABLE(pci, boardnet_id_table);

/*
 * PCI Device info
 */
static struct pci_driver boardnet_pci_driver = {
	.name		= PPC85XX_NETDRV_NAME,
	.id_table	= boardnet_id_table,
	.probe		= boardnet_probe,
	.remove		= __devexit_p(boardnet_remove),
	.suspend	= boardnet_suspend,
	.resume		= boardnet_resume
};

/*
 * Entry to insmod this driver module
 */
static int __init boardnet_pci_init(void)
{
	int retval;

	retval = pci_register_driver(&boardnet_pci_driver);
	if (retval) {
		printk(KERN_ERR "MPC85xx agent-net drvier init fail."
				"ret = %d\n", retval);
		return retval;
	}
	return 0;
}

/*
 * Entry to rmmod this driver module
 */
static void __exit boardnet_pci_exit(void)
{
	pci_unregister_driver(&boardnet_pci_driver);
	return;
}

module_init(boardnet_pci_init);
module_exit(boardnet_pci_exit);

MODULE_AUTHOR("Xiaobo Xie<X.Xie@freescale.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MPC85xx PCI EP/Agent Demo Driver(Host/RC side)");
