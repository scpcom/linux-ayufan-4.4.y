#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * drivers/net/gianfar.c
 *
 * Gianfar Ethernet Driver
 * This driver is designed for the non-CPM ethernet controllers
 * on the 85xx and 83xx family of integrated processors
 * Based on 8260_io/fcc_enet.c
 *
 * Author: Andy Fleming
 * Maintainer: Kumar Gala
 * Modifier: Sandeep Gopalpet <sandeep.kumar@freescale.com>
 *
 * Copyright 2002-2011 Freescale Semiconductor, Inc.
 * Copyright (c) 2007 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 *  Gianfar:  AKA Lambda Draconis, "Dragon"
 *  RA 11 31 24.2
 *  Dec +69 19 52
 *  V 3.84
 *  B-V +1.62
 *
 *  Theory of operation
 *
 *  The driver is initialized through of_device. Configuration information
 *  is therefore conveyed through an OF-style device tree.
 *
 *  The Gianfar Ethernet Controller uses a ring of buffer
 *  descriptors.  The beginning is indicated by a register
 *  pointing to the physical address of the start of the ring.
 *  The end is determined by a "wrap" bit being set in the
 *  last descriptor of the ring.
 *
 *  When a packet is received, the RXF bit in the
 *  IEVENT register is set, triggering an interrupt when the
 *  corresponding bit in the IMASK register is also set (if
 *  interrupt coalescing is active, then the interrupt may not
 *  happen immediately, but will wait until either a set number
 *  of frames or amount of time have passed).  In NAPI, the
 *  interrupt handler will signal there is work to be done, and
 *  exit. This method will start at the last known empty
 *  descriptor, and process every subsequent descriptor until there
 *  are none left with data (NAPI will stop after a set number of
 *  packets to give time to other tasks, but will eventually
 *  process all the packets).  The data arrives inside a
 *  pre-allocated skb, and so after the skb is passed up to the
 *  stack, a new skb must be allocated, and the address field in
 *  the buffer descriptor must be updated to indicate this new
 *  skb.
 *
 *  When the kernel requests that a packet be transmitted, the
 *  driver starts where it left off last time, and points the
 *  descriptor at the buffer which was passed in.  The driver
 *  then informs the DMA engine that there are packets ready to
 *  be transmitted.  Once the controller is finished transmitting
 *  the packet, an interrupt may be triggered (under the same
 *  conditions as for reception, but depending on the TXF bit).
 *  The driver then cleans up the buffer.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/inetdevice.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/phy_fixed.h>
#include <linux/of.h>
#include <net/xfrm.h>
#include <sysdev/fsl_soc.h>
#include <linux/synobios.h>
#ifdef CONFIG_GFAR_SW_PKT_STEERING
#include <asm/fsl_msg.h>
#endif

#ifdef CONFIG_NET_GIANFAR_FP
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <net/route.h>
#include <net/ip.h>
#include <linux/jhash.h>
#endif

#include <net/tcp.h>

#include "gianfar.h"
#include "fsl_pq_mdio.h"

#define TX_TIMEOUT      (1*HZ)
#undef BRIEF_GFAR_ERRORS
#undef VERBOSE_GFAR_ERRORS

#ifdef MY_ABC_HERE
static int g_netif_count = 0;
extern long g_internal_netif_num;
#endif

extern void tcp_v4_send_reset(struct sock *sk, struct sk_buff *skb);
const char gfar_driver_name[] = "Gianfar Ethernet";
const char gfar_driver_version[] = "1.4-skbr1.1.5";

static int gfar_enet_open(struct net_device *dev);
static int gfar_start_xmit(struct sk_buff *skb, struct net_device *dev);
static void gfar_reset_task(struct work_struct *work);
static void gfar_timeout(struct net_device *dev);
static int gfar_close(struct net_device *dev);
struct sk_buff *gfar_new_skb(struct net_device *dev);
static void gfar_new_rxbdp(struct gfar_priv_rx_q *rx_queue, struct rxbd8 *bdp,
		struct sk_buff *skb);
static int gfar_set_mac_address(struct net_device *dev);
static int gfar_change_mtu(struct net_device *dev, int new_mtu);
static irqreturn_t gfar_error(int irq, void *dev_id);
static irqreturn_t gfar_transmit(int irq, void *dev_id);
static irqreturn_t gfar_interrupt(int irq, void *dev_id);
static void adjust_link(struct net_device *dev);
static void init_registers(struct net_device *dev);
static int init_phy(struct net_device *dev);
static int gfar_probe(struct of_device *ofdev,
		const struct of_device_id *match);
static int gfar_remove(struct of_device *ofdev);
static void free_skb_resources(struct gfar_private *priv);
static void gfar_set_multi(struct net_device *dev);
static void gfar_set_hash_for_addr(struct net_device *dev, u8 *addr);
static void gfar_configure_serdes(struct net_device *dev);
#ifdef CONFIG_GIANFAR_TXNAPI
static int gfar_poll_tx(struct napi_struct *napi, int budget);
static int gfar_poll_rx(struct napi_struct *napi, int budget);
#else
static int gfar_poll(struct napi_struct *napi, int budget);
#endif
#ifdef CONFIG_NET_POLL_CONTROLLER
static void gfar_netpoll(struct net_device *dev);
#endif
#ifdef CONFIG_NET_GIANFAR_FP
static int gfar_accept_fastpath(struct net_device *dev, struct dst_entry *dst);
DECLARE_PER_CPU(struct netif_rx_stats, netdev_rx_stat);
#endif
int gfar_clean_rx_ring(struct gfar_priv_rx_q *rx_queue, int rx_work_limit);
#ifdef CONFIG_GIANFAR_TXNAPI
static int gfar_clean_tx_ring(struct gfar_priv_tx_q *tx_queue, int tx_work_limit);
#else
static int gfar_clean_tx_ring(struct gfar_priv_tx_q *tx_queue);
#endif
static int gfar_process_frame(struct net_device *dev, struct sk_buff *skb,
			      int amount_pull);
static void gfar_vlan_rx_register(struct net_device *netdev,
		                struct vlan_group *grp);
void gfar_halt(struct net_device *dev);
static void gfar_halt_nodisable(struct net_device *dev);
void gfar_start(struct net_device *dev);
static void gfar_clear_exact_match(struct net_device *dev);
static void gfar_set_mac_for_addr(struct net_device *dev, int num, u8 *addr);
static int gfar_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
u16 gfar_select_queue(struct net_device *dev, struct sk_buff *skb);
static struct net_device_stats *gfar_get_stats(struct net_device *dev);

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
static unsigned int skbuff_truesize(unsigned int buffer_size);
static void gfar_skbr_register_truesize(struct gfar_private *priv);
static int gfar_kfree_skb(struct sk_buff *skb, int qindex);
static void gfar_reset_skb_handler(struct gfar_skb_handler *sh);
#endif

#ifdef CONFIG_PM
static void gfar_halt_rx(struct net_device *dev);
static void gfar_halt_tx_nodisable(struct net_device *dev);
static void gfar_rx_start(struct net_device *dev);
static void gfar_tx_start(struct net_device *dev);
static void gfar_enable_filer(struct net_device *dev);
static int gfar_get_ip(struct net_device *dev);
static void gfar_config_filer_table(struct net_device *dev);
#endif

MODULE_AUTHOR("Freescale Semiconductor, Inc");
MODULE_DESCRIPTION("Gianfar Ethernet Driver");
MODULE_LICENSE("GPL");

static const struct net_device_ops gfar_netdev_ops = {
	.ndo_open = gfar_enet_open,
	.ndo_start_xmit = gfar_start_xmit,
	.ndo_stop = gfar_close,
	.ndo_change_mtu = gfar_change_mtu,
	.ndo_set_multicast_list = gfar_set_multi,
	.ndo_tx_timeout = gfar_timeout,
	.ndo_do_ioctl = gfar_ioctl,
	.ndo_select_queue = gfar_select_queue,
	.ndo_get_stats = gfar_get_stats,
	.ndo_vlan_rx_register = gfar_vlan_rx_register,
	.ndo_set_mac_address = eth_mac_addr,
	.ndo_validate_addr = eth_validate_addr,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = gfar_netpoll,
#endif
#ifdef CONFIG_NET_GIANFAR_FP
	.ndo_accept_fastpath = gfar_accept_fastpath,
#endif
};

#ifdef CONFIG_GFAR_SW_PKT_STEERING
DEFINE_PER_CPU(struct gfar_cpu_dev, gfar_cpu_dev);
#endif

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
static int syno_gianfar_drop_caches = 0;
#endif

static struct net_device_stats *gfar_get_stats(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct netdev_queue *txq;
	unsigned long rx_packets = 0, rx_bytes = 0, rx_dropped = 0;
	unsigned long tx_packets = 0, tx_bytes = 0;
	int i = 0;

	for (i = 0; i < priv->num_rx_queues; i++) {
		rx_packets += priv->rx_queue[i]->stats.rx_packets;
		rx_bytes += priv->rx_queue[i]->stats.rx_bytes;
		rx_dropped += priv->rx_queue[i]->stats.rx_dropped;
	}

	dev->stats.rx_packets = rx_packets;
	dev->stats.rx_bytes = rx_bytes;
	dev->stats.rx_dropped = rx_dropped;

	for (i = 0; i < priv->num_tx_queues; i++) {
		txq = netdev_get_tx_queue(dev, i);
		tx_bytes += txq->tx_bytes;
		tx_packets += txq->tx_packets;
	}

	dev->stats.tx_bytes = tx_bytes;
	dev->stats.tx_packets = tx_packets;

	return &dev->stats;
}

inline void lock_rx_qs(struct gfar_private *priv)
{
	int i = 0x0;

	for (i = 0; i < priv->num_rx_queues; i++)
		spin_lock(&priv->rx_queue[i]->rxlock);
}

inline void lock_tx_qs(struct gfar_private *priv)
{
	int i = 0x0;

	for (i = 0; i < priv->num_tx_queues; i++)
		spin_lock(&priv->tx_queue[i]->txlock);
}

inline void unlock_rx_qs(struct gfar_private *priv)
{
	int i = 0x0;

	for (i = 0; i < priv->num_rx_queues; i++)
		spin_unlock(&priv->rx_queue[i]->rxlock);
}

inline void unlock_tx_qs(struct gfar_private *priv)
{
	int i = 0x0;

	for (i = 0; i < priv->num_tx_queues; i++)
		spin_unlock(&priv->tx_queue[i]->txlock);
}

/* Returns 1 if incoming frames use an FCB */
static inline int gfar_uses_fcb(struct gfar_private *priv)
{
	return priv->vlgrp || priv->rx_csum_enable;
}

static inline void gfar_rx_checksum(struct sk_buff *skb, struct rxfcb *fcb)
{
	/* If valid headers were found, and valid sums
	 * were verified, then we tell the kernel that no
	 * checksumming is necessary.  Otherwise, it is */
	if ((fcb->flags & RXFCB_CSUM_MASK) == (RXFCB_CIP | RXFCB_CTU))
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	else
		skb->ip_summed = CHECKSUM_NONE;
}

u16 gfar_select_queue(struct net_device *dev, struct sk_buff *skb)
{
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	struct gfar_private *priv = netdev_priv(dev);
	if (priv->sps)
		return smp_processor_id();
	else
#endif
		return skb_get_queue_mapping(skb);
}
static void free_tx_pointers(struct gfar_private *priv)
{
	int i = 0;

	for (i = 0; i < priv->num_tx_queues; i++)
		kfree(priv->tx_queue[i]);
}

static void free_rx_pointers(struct gfar_private *priv)
{
	int i = 0;

	for (i = 0; i < priv->num_rx_queues; i++)
		kfree(priv->rx_queue[i]);
}

static void unmap_group_regs(struct gfar_private *priv)
{
	int i = 0;

	for (i = 0; i < MAXGROUPS; i++)
		if (priv->gfargrp[i].regs)
			iounmap(priv->gfargrp[i].regs);
}

static void disable_napi(struct gfar_private *priv)
{
	int i = 0;
#ifdef CONFIG_GIANFAR_TXNAPI
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int j;
	int cpus  = num_online_cpus();
#endif
	for (i = 0; i < priv->num_grps; i++) {
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		for (j = 0; j < cpus; j++)
			napi_disable(&priv->gfargrp[i].napi_tx[j]);
#else
		napi_disable(&priv->gfargrp[i].napi_tx);
#endif
		napi_disable(&priv->gfargrp[i].napi_rx);
	}
#else
	for (i = 0; i < priv->num_grps; i++)
		napi_disable(&priv->gfargrp[i].napi);
#endif
}

static void enable_napi(struct gfar_private *priv)
{
	int i = 0;

#ifdef CONFIG_GIANFAR_TXNAPI
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int j;
	int cpus = num_online_cpus();
#endif
	for (i = 0; i < priv->num_grps; i++) {
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		for (j = 0; j < cpus; j++)
			napi_enable(&priv->gfargrp[i].napi_tx[j]);
#else
		napi_enable(&priv->gfargrp[i].napi_tx);
#endif
		napi_enable(&priv->gfargrp[i].napi_rx);
	}
#else
	for (i = 0; i < priv->num_grps; i++)
		napi_enable(&priv->gfargrp[i].napi);
#endif
}

static int gfar_parse_group(struct device_node *np,
		struct gfar_private *priv, const char *model)
{
	u32 *queue_mask;
	u64 addr, size;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int i;
	int cpus = num_online_cpus();
#endif

	addr = of_translate_address(np,
			of_get_address(np, 0, &size, NULL));
	priv->gfargrp[priv->num_grps].regs = ioremap(addr, size);

	if (!priv->gfargrp[priv->num_grps].regs)
		return -ENOMEM;

	priv->gfargrp[priv->num_grps].interruptTransmit =
			irq_of_parse_and_map(np, 0);

	/* If we aren't the FEC we have multiple interrupts */
	if (model && strcasecmp(model, "FEC")) {
		priv->gfargrp[priv->num_grps].interruptReceive =
			irq_of_parse_and_map(np, 1);
		priv->gfargrp[priv->num_grps].interruptError =
			irq_of_parse_and_map(np,2);
		if (priv->gfargrp[priv->num_grps].interruptTransmit < 0 ||
			priv->gfargrp[priv->num_grps].interruptReceive < 0 ||
			priv->gfargrp[priv->num_grps].interruptError < 0) {
			return -EINVAL;
		}
	}

	priv->gfargrp[priv->num_grps].grp_id = priv->num_grps;
	priv->gfargrp[priv->num_grps].priv = priv;
	spin_lock_init(&priv->gfargrp[priv->num_grps].grplock);
	if(priv->mode == MQ_MG_MODE) {
		queue_mask = (u32 *)of_get_property(np,
					"fsl,rx-bit-map", NULL);
		priv->gfargrp[priv->num_grps].rx_bit_map =
			queue_mask ?  *queue_mask :(DEFAULT_MAPPING >> priv->num_grps);
		queue_mask = (u32 *)of_get_property(np,
					"fsl,tx-bit-map", NULL);
		priv->gfargrp[priv->num_grps].tx_bit_map =
			queue_mask ? *queue_mask : (DEFAULT_MAPPING >> priv->num_grps);
		priv->gfargrp[priv->num_grps].rx_bit_map &= ~1;
		priv->gfargrp[priv->num_grps].tx_bit_map &= ~1;
	} else {
		priv->gfargrp[priv->num_grps].rx_bit_map = 0xFF;
		priv->gfargrp[priv->num_grps].tx_bit_map = 0xFF;
	}
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps) {
		/* register msg unit for virtual tx interrupt for each cpu */
		for (i = 0; i < cpus; i++) { /* for each cpu */
			priv->gfargrp[priv->num_grps].msg_virtual_tx[i]
				= fsl_get_msg_unit();
			if (IS_ERR
			(priv->gfargrp[priv->num_grps].msg_virtual_tx[i])) {
				priv->sps = 0;
				printk(KERN_WARNING
				"%s: unable to allocate msg interrupt for pkt"
				"steering, error = %ld!\n", __func__,
				PTR_ERR(priv->gfargrp[priv->num_grps].
				msg_virtual_tx[i]));
			}
		}
	}
#endif
	priv->num_grps++;

	return 0;
}

static int gfar_of_init(struct of_device *ofdev, struct net_device **pdev)
{
	const char *model;
	const char *ctype;
	const void *mac_addr;
	int err = 0, i, ret = 0;
	struct net_device *dev = NULL;
	struct gfar_private *priv = NULL;
	struct device_node *np = ofdev->node;
	struct device_node *child = NULL;
	struct device_node *timer_node;
	const phandle *timer_handle;
	const u32 *stash;
	const u32 *stash_len;
	const u32 *stash_idx;
	unsigned int num_tx_qs, num_rx_qs;
	u32 *tx_queues, *rx_queues;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int sps;
#endif

	if (!np || !of_device_is_available(np))
		return -ENODEV;

	/* parse the num of tx and rx queues */
	tx_queues = (u32 *)of_get_property(np, "fsl,num_tx_queues", NULL);
	num_tx_qs = tx_queues ? *tx_queues : 1;

	if (num_tx_qs > MAX_TX_QS) {
		printk(KERN_ERR "num_tx_qs(=%d) greater than MAX_TX_QS(=%d)\n",
				num_tx_qs, MAX_TX_QS);
		printk(KERN_ERR "Cannot do alloc_etherdev, aborting\n");
		return -EINVAL;
	}

#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if ((num_online_cpus() == 2) &&
		(!of_device_is_compatible(np, "fsl,etsec2"))) {
		printk(KERN_INFO "ETSEC: IPS Enabled\n");
		num_tx_qs = num_online_cpus();
		sps = 1;
	}
#endif

	rx_queues = (u32 *)of_get_property(np, "fsl,num_rx_queues", NULL);
	num_rx_qs = rx_queues ? *rx_queues : 1;

	if (num_rx_qs > MAX_RX_QS) {
		printk(KERN_ERR "num_rx_qs(=%d) greater than MAX_RX_QS(=%d)\n",
				num_tx_qs, MAX_TX_QS);
		printk(KERN_ERR "Cannot do alloc_etherdev, aborting\n");
		return -EINVAL;
	}

	*pdev = alloc_etherdev_mq(sizeof(*priv), num_tx_qs);
	dev = *pdev;
	if (NULL == dev)
		return -ENOMEM;

	priv = netdev_priv(dev);
	priv->node = ofdev->node;
	priv->ndev = dev;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	priv->sps = sps;
#endif

	dev->num_tx_queues = num_tx_qs;
	dev->real_num_tx_queues = num_tx_qs;
	priv->num_tx_queues = num_tx_qs;
	priv->num_rx_queues = num_rx_qs;
	priv->num_grps = 0x0;

	model = of_get_property(np, "model", NULL);

	for (i = 0; i < MAXGROUPS; i++)
		priv->gfargrp[i].regs = NULL;

	/* Parse and initialize group specific information */
	if (of_device_is_compatible(np, "fsl,etsec2")) {
		priv->mode = MQ_MG_MODE;
		for_each_child_of_node(np, child) {
			if (of_device_is_compatible
				(child, "fsl,etsec2-mdio") ||
				of_device_is_compatible
				(child, "fsl,etsec2-tbi"))
				continue;
			err = gfar_parse_group(child, priv, model);
			if (err)
				goto err_grp_init;
		}
		priv->gfargrp[0].rx_bit_map |= 1;
		priv->gfargrp[0].tx_bit_map |= 1;
	} else {
		priv->mode = SQ_SG_MODE;
		err = gfar_parse_group(np, priv, model);
		if(err)
			goto err_grp_init;
	}

	for (i = 0; i < priv->num_tx_queues; i++)
	       priv->tx_queue[i] = NULL;
	for (i = 0; i < priv->num_rx_queues; i++)
		priv->rx_queue[i] = NULL;

	for (i = 0; i < priv->num_tx_queues; i++) {
		priv->tx_queue[i] =  (struct gfar_priv_tx_q *)kmalloc(
				sizeof (struct gfar_priv_tx_q), GFP_KERNEL);
		if (!priv->tx_queue[i]) {
			err = -ENOMEM;
			goto tx_alloc_failed;
		}
		priv->tx_queue[i]->tx_skbuff = NULL;
		priv->tx_queue[i]->qindex = i;
		priv->tx_queue[i]->dev = dev;
		spin_lock_init(&(priv->tx_queue[i]->txlock));
	}

	for (i = 0; i < priv->num_rx_queues; i++) {
		priv->rx_queue[i] = (struct gfar_priv_rx_q *)kmalloc(
					sizeof (struct gfar_priv_rx_q), GFP_KERNEL);
		if (!priv->rx_queue[i]) {
			err = -ENOMEM;
			goto rx_alloc_failed;
		}
		priv->rx_queue[i]->rx_skbuff = NULL;
		priv->rx_queue[i]->qindex = i;
		priv->rx_queue[i]->dev = dev;
		spin_lock_init(&(priv->rx_queue[i]->rxlock));
	}


	stash = of_get_property(np, "bd-stash", NULL);

	if (stash) {
		priv->device_flags |= FSL_GIANFAR_DEV_HAS_BD_STASHING;
		priv->bd_stash_en = 1;
	}

	stash_len = of_get_property(np, "rx-stash-len", NULL);

	if (stash_len)
		priv->rx_stash_size = *stash_len;

	stash_idx = of_get_property(np, "rx-stash-idx", NULL);

	if (stash_idx)
		priv->rx_stash_index = *stash_idx;

	if (stash_len || stash_idx)
		priv->device_flags |= FSL_GIANFAR_DEV_HAS_BUF_STASHING;

	/* Handle IEEE1588 node */
	timer_handle = of_get_property(np, "ptimer-handle", NULL);
	if (timer_handle) {
		timer_node = of_find_node_by_phandle(*timer_handle);
		if (timer_node) {
			ret = of_address_to_resource(timer_node, 0,
					&priv->timer_resource);
			if (!ret) {
				priv->ptimer_present = 1;
				printk(KERN_INFO "IEEE1588: ptp-timer device"
						"present in the system\n");
			}
		}
	} else
		printk(KERN_INFO "IEEE1588: disable on the system.\n");

	mac_addr = of_get_mac_address(np);
	if (mac_addr)
		memcpy(dev->dev_addr, mac_addr, MAC_ADDR_LEN);

	if (model && !strcasecmp(model, "TSEC"))
		priv->device_flags =
			FSL_GIANFAR_DEV_HAS_GIGABIT |
			FSL_GIANFAR_DEV_HAS_COALESCE |
			FSL_GIANFAR_DEV_HAS_RMON |
			FSL_GIANFAR_DEV_HAS_MULTI_INTR;
	if (model && !strcasecmp(model, "eTSEC"))
		priv->device_flags =
			FSL_GIANFAR_DEV_HAS_GIGABIT |
			FSL_GIANFAR_DEV_HAS_COALESCE |
			FSL_GIANFAR_DEV_HAS_RMON |
			FSL_GIANFAR_DEV_HAS_MULTI_INTR |
			FSL_GIANFAR_DEV_HAS_PADDING |
			FSL_GIANFAR_DEV_HAS_CSUM |
			FSL_GIANFAR_DEV_HAS_VLAN |
			FSL_GIANFAR_DEV_HAS_EXTENDED_HASH;

	ctype = of_get_property(np, "phy-connection-type", NULL);

	/* We only care about rgmii-id.  The rest are autodetected */
	if (ctype && !strcmp(ctype, "rgmii-id"))
		priv->interface = PHY_INTERFACE_MODE_RGMII_ID;
	else
		priv->interface = PHY_INTERFACE_MODE_MII;

	if (of_get_property(np, "fsl,magic-packet", NULL))
		priv->device_flags |= FSL_GIANFAR_DEV_HAS_MAGIC_PACKET;

	if (of_get_property(np, "fsl,wake-on-filer", NULL))
		priv->device_flags |= FSL_GIANFAR_DEV_HAS_ARP_PACKET;

	priv->phy_node = of_parse_phandle(np, "phy-handle", 0);

	/* Find the TBI PHY.  If it's not there, we don't support SGMII */
	priv->tbi_node = of_parse_phandle(np, "tbi-handle", 0);

	return 0;

rx_alloc_failed:
	free_rx_pointers(priv);
tx_alloc_failed:
	free_tx_pointers(priv);
err_grp_init:
	unmap_group_regs(priv);
	free_netdev(dev);
	return err;
}

/* Ioctl MII Interface */
static int gfar_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct gfar_private *priv = netdev_priv(dev);
	int retVal = 0;

	if (!netif_running(dev))
		return -EINVAL;

	if (!priv->phydev)
		return -ENODEV;

	if ((cmd >= PTP_GET_RX_TIMESTAMP_SYNC) &&
			(cmd <= PTP_CLEANUP_TIMESTAMP_BUFFERS))
		retVal = gfar_ioctl_1588(dev, rq, cmd);
	else
		retVal = phy_mii_ioctl(priv->phydev, if_mii(rq), cmd);

	return retVal;
}

static unsigned int reverse_bitmap(unsigned int bit_map, unsigned int max_qs)
{
	unsigned int new_bit_map = 0x0;
	int mask = 0x1 << (max_qs - 1), i;
	for (i = 0; i < max_qs; i++) {
		if (bit_map & mask)
			new_bit_map = new_bit_map + (1 << i);
		mask = mask >> 0x1;
	}
	return new_bit_map;
}

u32 cluster_entry_per_class(struct gfar_private *priv, u32 rqfar, u32 class)
{
	u32 rqfpr = FPR_FILER_MASK;
	u32 rqfcr = 0x0;

	rqfar--;
	rqfcr = RQFCR_CLE | RQFCR_PID_MASK | RQFCR_CMP_EXACT;
	priv->ftp_rqfpr[rqfar] = rqfpr;
	priv->ftp_rqfcr[rqfar] = rqfcr;
	gfar_write_filer(priv, rqfar, rqfcr, rqfpr);

	rqfar--;
	rqfcr = RQFCR_CMP_NOMATCH;
	priv->ftp_rqfpr[rqfar] = rqfpr;
	priv->ftp_rqfcr[rqfar] = rqfcr;
	gfar_write_filer(priv, rqfar, rqfcr, rqfpr);

	rqfar--;
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_PARSE | RQFCR_CLE | RQFCR_AND;
	rqfpr = class;
	priv->ftp_rqfcr[rqfar] = rqfcr;
	priv->ftp_rqfpr[rqfar] = rqfpr;
	gfar_write_filer(priv, rqfar, rqfcr, rqfpr);

	rqfar--;
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_MASK | RQFCR_AND;
	rqfpr = class;
	priv->ftp_rqfcr[rqfar] = rqfcr;
	priv->ftp_rqfpr[rqfar] = rqfpr;
	gfar_write_filer(priv, rqfar, rqfcr, rqfpr);

	return rqfar;
}

#ifdef CONFIG_GFAR_HW_TCP_RECEIVE_OFFLOAD
void gfar_setup_hwaccel_tcp4_receive(struct sock *sk, struct sk_buff *skb)
{

	int i = 0;
	int j = 0;
	u32 rqfcr = 0x0;
	u32 rqfpr = 0;
	struct tcphdr *th;
	struct iphdr *iph;
	struct gfar_private *priv = netdev_priv(skb->skb_owner);

	if (priv->ptimer_present || !priv->rx_csum_enable ||
		priv->num_rx_queues < (TCP_CHL_OFFSET + RESERVE_CHL_NUM))
		return;

	th = tcp_hdr(skb);
	iph = ip_hdr(skb);

	/*select next empty TCP channel*/
	for (i = (priv->empty_tcp_channel + 1) % (priv->num_rx_queues - TCP_CHL_OFFSET - 1);
		i != priv->empty_tcp_channel;
		i = (i+1) % (priv->num_rx_queues - TCP_CHL_OFFSET - 1)) {
		if (priv->tcp_hw_channel[i] == NULL)
			break;
	}

	if (i == priv->empty_tcp_channel)
		i = (i+1) % (priv->num_rx_queues - TCP_CHL_OFFSET - 1);

	priv->tcp_hw_channel[priv->empty_tcp_channel] = sk;
	sk->tcp_hw_channel = &(priv->tcp_hw_channel[priv->empty_tcp_channel]);

	j = priv->tcp_filer_idx + (priv->empty_tcp_channel << 2);

	/*setup IPv4 source address*/
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_SIA | RQFCR_AND;
	rqfpr = ntohl(iph->saddr);
	priv->ftp_rqfcr[j] = rqfcr;
	priv->ftp_rqfpr[j] = rqfpr;
	gfar_write_filer(priv, j, rqfcr, rqfpr);
	j++;
	/*setup IPv4 destination address*/
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_DIA | RQFCR_AND;
	rqfpr = ntohl(iph->daddr);
	priv->ftp_rqfcr[j] = rqfcr;
	priv->ftp_rqfpr[j] = rqfpr;
	gfar_write_filer(priv, j, rqfcr, rqfpr);
	j++;
	/*setup TCP source port*/
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_SPT | RQFCR_AND;
	rqfpr = ntohs(th->source);
	priv->ftp_rqfcr[j] = rqfcr;
	priv->ftp_rqfpr[j] = rqfpr;
	gfar_write_filer(priv, j, rqfcr, rqfpr);
	j++;
	/*setup TCP destination port*/
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_DPT | ((priv->empty_tcp_channel + TCP_CHL_OFFSET) << 10);
	rqfpr = ntohs(th->dest);
	priv->ftp_rqfcr[j] = rqfcr;
	priv->ftp_rqfpr[j] = rqfpr;
	gfar_write_filer(priv, j, rqfcr, rqfpr);

	priv->empty_tcp_channel = i;

	/*clean up tcp channel*/
	if (priv->tcp_hw_channel[i] != NULL) {
		priv->tcp_hw_channel[i]->tcp_hw_channel = NULL;
		priv->tcp_hw_channel[i] = NULL;
		j = priv->tcp_filer_idx + (i << 2);
		rqfcr = RQFCR_CMP_NOMATCH;
		rqfpr = FPR_FILER_MASK;
		priv->ftp_rqfcr[j] = rqfcr;
		priv->ftp_rqfpr[j] = rqfpr;
		gfar_write_filer(priv, j, rqfcr, rqfpr);
		j++;
		priv->ftp_rqfcr[j] = rqfcr;
		priv->ftp_rqfpr[j] = rqfpr;
		gfar_write_filer(priv, j, rqfcr, rqfpr);
		j++;
		priv->ftp_rqfcr[j] = rqfcr;
		priv->ftp_rqfpr[j] = rqfpr;
		gfar_write_filer(priv, j, rqfcr, rqfpr);
		j++;
		priv->ftp_rqfcr[j] = rqfcr;
		priv->ftp_rqfpr[j] = rqfpr;
		gfar_write_filer(priv, j, rqfcr, rqfpr);
	}
}

inline void gfar_hwaccel_tcp4_receive(struct gfar_private *priv,
		struct gfar_priv_rx_q *rx_queue, struct sk_buff *skb, int amount_pull)
{
	const struct tcphdr *th;
	const struct iphdr *iph;
	int p_len;
	int ph_len;
	struct rxfcb *fcb;
	struct sock *gfar_sk;
	struct rtable *rt;

	gfar_sk = priv->tcp_hw_channel[rx_queue->qindex - TCP_CHL_OFFSET];

	fcb = (struct rxfcb *)skb->data;

	gfar_rx_checksum(skb, fcb);
	skb->pkt_type = PACKET_HOST;

	/*set IPv4 header*/
	skb->network_header = skb->data + amount_pull + ETH_HLEN;
	iph = ip_hdr(skb);

	if (iph->ihl > 5 || (iph->frag_off & htons(IP_MF | IP_OFFSET)) ||
		(gfar_sk->sk_state != TCP_ESTABLISHED)) {
		gfar_process_frame(priv->ndev, skb, amount_pull);
		return;
	}

	/*IPv4 header length*/
	ph_len = iph->ihl << 2;
	p_len = ntohs(iph->tot_len);

	if (p_len <  (skb->len - amount_pull - ETH_HLEN)) {
		skb->tail = skb->tail - ((skb->len - amount_pull - ETH_HLEN) - p_len);
		skb->len = p_len - ph_len;
	} else
		skb->len = skb->len - (amount_pull + ETH_HLEN + ph_len);

	/*set TCP header*/
	skb->transport_header = skb->network_header + ph_len;
	skb->data = skb->transport_header;
	th = tcp_hdr(skb);
	TCP_SKB_CB(skb)->seq = ntohl(th->seq);
	TCP_SKB_CB(skb)->end_seq = (TCP_SKB_CB(skb)->seq + th->syn + th->fin +
					skb->len - (th->doff << 2));
	TCP_SKB_CB(skb)->ack_seq = ntohl(th->ack_seq);
	TCP_SKB_CB(skb)->when	 = 0;
	TCP_SKB_CB(skb)->flags	 = iph->tos;
	TCP_SKB_CB(skb)->sacked	 = 0;
	rt = (struct rtable *)__sk_dst_check(gfar_sk, 0);
	if (rt) {
		if(!skb_rtable(skb)) {
			dst_use(&rt->u.dst, jiffies);
			skb_dst_set(skb,&rt->u.dst);
		}
	}

	bh_lock_sock(gfar_sk);
	if (!sock_owned_by_user(gfar_sk)) {
		if (tcp_rcv_established(gfar_sk, skb, tcp_hdr(skb), skb->len)) {
			if(skb_rtable(skb))
				tcp_v4_send_reset(gfar_sk, skb);
			kfree_skb(skb);
		}
	} else
		sk_add_backlog(gfar_sk, skb);
	bh_unlock_sock(gfar_sk);
}

void gfar_init_tcp_filer_rule(struct gfar_private *priv, int index)
{
	int i;
	int j = 0;
	u32 rqfcr = 0x0;
	u32 rqfpr = FPR_FILER_MASK;

	i = index - 4 - (TCP_CHL_NUM << 2);
	if (i < 0)
		return;

	printk(KERN_INFO "%s: enabled hardware TCP receive offload\n",
			priv->ndev->name);

	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_MASK | RQFCR_AND;
	rqfpr = RQFPR_IPV4|RQFPR_TCP;
	priv->ftp_rqfcr[i] = rqfcr;
	priv->ftp_rqfpr[i] = rqfpr;
	gfar_write_filer(priv, i, rqfcr, rqfpr);
	i++;
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_PARSE | RQFCR_AND;
	rqfpr = RQFPR_IPV4|RQFPR_TCP;
	priv->ftp_rqfcr[i] = rqfcr;
	priv->ftp_rqfpr[i] = rqfpr;
	gfar_write_filer(priv, i, rqfcr, rqfpr);
	i++;
	rqfcr = RQFCR_CMP_EXACT | RQFCR_PID_MASK | RQFCR_CLE | RQFCR_AND;
	rqfpr = FPR_FILER_MASK;
	priv->ftp_rqfcr[i] = rqfcr;
	priv->ftp_rqfpr[i] = rqfpr;
	gfar_write_filer(priv, i, rqfcr, rqfpr);
	i++;
	rqfcr = RQFCR_CMP_NOMATCH;
	rqfpr = FPR_FILER_MASK;
	priv->tcp_filer_idx = i;
	priv->empty_tcp_channel = 1;

	for (j = 0; j < (TCP_CHL_NUM << 2); j++) {
		priv->ftp_rqfcr[i] = rqfcr;
		priv->ftp_rqfpr[i] = rqfpr;
		gfar_write_filer(priv, i, rqfcr, rqfpr);
		i++;
	}

	for (j = 0; j < TCP_CHL_NUM; j++)
		priv->tcp_hw_channel[j] = 0;

	rqfpr = FPR_FILER_MASK;
	rqfcr = RQFCR_CMP_NOMATCH | RQFCR_CLE;
	priv->ftp_rqfcr[i] = rqfcr;
	priv->ftp_rqfpr[i] = rqfpr;
	gfar_write_filer(priv, i, rqfcr, rqfpr);
}
#endif


static void gfar_init_filer_table(struct gfar_private *priv)
{
	int i = 0x0;
	u32 rqfar = MAX_FILER_IDX;
	u32 rqfcr = 0x0;
	u32 rqfpr = FPR_FILER_MASK;

	/* Default rule */
	rqfcr = RQFCR_CMP_MATCH;
	priv->ftp_rqfcr[rqfar] = rqfcr;
	priv->ftp_rqfpr[rqfar] = rqfpr;
	gfar_write_filer(priv, rqfar, rqfcr, rqfpr);

	rqfar = cluster_entry_per_class(priv, rqfar, RQFPR_IPV6);
	rqfar = cluster_entry_per_class(priv, rqfar, RQFPR_IPV6 | RQFPR_UDP);
	rqfar = cluster_entry_per_class(priv, rqfar, RQFPR_IPV6 | RQFPR_TCP);
	rqfar = cluster_entry_per_class(priv, rqfar, RQFPR_IPV4);
	rqfar = cluster_entry_per_class(priv, rqfar, RQFPR_IPV4 | RQFPR_UDP);
	rqfar = cluster_entry_per_class(priv, rqfar, RQFPR_IPV4 | RQFPR_TCP);

#ifdef CONFIG_GFAR_HW_TCP_RECEIVE_OFFLOAD
	/*init TCP filer rule table*/
	gfar_init_tcp_filer_rule(priv, rqfar);

	/* cur_filer_idx indicated the fisrt non-masked rule */
	priv->cur_filer_idx = priv->tcp_filer_idx - 3;

	/* Program the RIR0 reg with the required distribution */
	priv->gfargrp[0].regs->rir0 = TWO_QUEUE_RIR0;
#else
	/* cur_filer_idx indicated the fisrt non-masked rule */
	priv->cur_filer_idx = rqfar;

	/* Program the RIR0 reg with the required distribution */
	priv->gfargrp[0].regs->rir0 = DEFAULT_RIR0;
#endif
	/* Rest are masked rules */
	rqfcr = RQFCR_CMP_NOMATCH;
	for (i = 0; i < rqfar; i++) {
		priv->ftp_rqfcr[i] = rqfcr;
		priv->ftp_rqfpr[i] = rqfpr;
		gfar_write_filer(priv, i, rqfcr, rqfpr);
	}

}

#ifdef CONFIG_GFAR_SW_PKT_STEERING
static int get_cpu_number(unsigned char *eth_pkt, int len)
{
	u32 addr1, addr2, ports;
	struct ipv6hdr *ip6;
	struct iphdr *ip;
	u32 hash, ihl;
	u8 ip_proto;
	int cpu;
	struct ethhdr *eth;
	static u32 simple_hashrnd;
	static int simple_hashrnd_initialized;

	if (len < ETH_HLEN)
		return -1;
	else
		eth = (struct ethhdr *)eth_pkt;

	if (unlikely(!simple_hashrnd_initialized)) {
		get_random_bytes(&simple_hashrnd, 4);
		simple_hashrnd_initialized = 1;
	}

	switch (eth->h_proto) {
	case __constant_htons(ETH_P_IP):
		if (len < (ETH_HLEN + sizeof(*ip)))
			return -1;

		ip = (struct iphdr *) (eth_pkt + ETH_HLEN);
		ip_proto = ip->protocol;
		addr1 = ip->saddr;
		addr2 = ip->daddr;
		ihl = ip->ihl;
		break;
	case __constant_htons(ETH_P_IPV6):
		if (len < (ETH_HLEN + sizeof(*ip6)))
			return -1;

		ip6 = (struct ipv6hdr *)(eth_pkt + ETH_HLEN);
		ip_proto = ip6->nexthdr;
		addr1 = ip6->saddr.s6_addr32[3];
		addr2 = ip6->daddr.s6_addr32[3];
		ihl = (40 >> 2);
		break;
	default:
		return -1;
	}
	ports = 0;
	switch (ip_proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:
	case IPPROTO_AH:
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
		if (len < (ETH_HLEN + (ihl * 4) + 4))
			ports = *((u32 *) (eth_pkt + ETH_HLEN + (ihl * 4)));
		break;

	default:
		break;
	}

	hash = jhash_3words(addr1, addr2, ports, simple_hashrnd);
	cpu = hash & 0x1;

	return cpu_online(cpu) ? cpu : -1;
}

static int gfar_cpu_poll(struct napi_struct *napi, int budget)
{
	struct gfar_cpu_dev *cpu_dev = &__get_cpu_var(gfar_cpu_dev);
	struct sk_buff *skb = NULL;
	int cpu = smp_processor_id();
	int rx_cleaned = 0;
	struct net_device *dev;
	struct gfar_private *priv;
	int amount_pull;
	struct shared_buffer *buf = &per_cpu(gfar_cpu_dev, !cpu).tx_queue;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	struct gfar_skb_handler *sh = &cpu_dev->sh;
#endif

	while (budget--) {
		smp_rmb();
		if (atomic_read(&buf->buff_cnt) == 0) {
			break;
		} else {
			skb = buf->buffer[buf->out];
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
			if (sh->recycle_count > 0) {
				buf->buffer[buf->out] = sh->recycle_queue;
				sh->recycle_queue = buf->buffer[buf->out]->next;
				buf->buffer[buf->out]->next = NULL;
				sh->recycle_count--;
			} else {
				buf->buffer[buf->out] = NULL;
			}
#endif
			buf->out = (buf->out + 1) % GFAR_CPU_BUFF_SIZE;
			atomic_dec(&buf->buff_cnt);

			dev = skb->dev;
			priv = netdev_priv(dev);

			if (priv->ptimer_present)
				amount_pull =
				(gfar_uses_fcb(priv) ? GMAC_FCB_LEN : 0);
			else
				amount_pull =
				(gfar_uses_fcb(priv) ? GMAC_FCB_LEN : 0) +
					priv->padding;

			gfar_process_frame(dev, skb, amount_pull);

			rx_cleaned++;
		}
	}

	if (budget > 0)
		napi_complete(napi);

	return rx_cleaned;
}

static irqreturn_t gfar_cpu_receive(int irq, void *dev_id)
{
	unsigned long flags;
	struct gfar_cpu_dev *cpu_dev = &__get_cpu_var(gfar_cpu_dev);

	/* clear the interrupt by reading message */
	fsl_clear_msg(cpu_dev->msg_virtual_rx);

	local_irq_save(flags);
	if (napi_schedule_prep(&cpu_dev->napi))
		__napi_schedule(&cpu_dev->napi);

	local_irq_restore(flags);

	return IRQ_HANDLED;
}

void gfar_cpu_setup(struct net_device *dev)
{
	return;
}

static enum hrtimer_restart gfar_cpu_timer_handle(struct hrtimer *timer)
{
	struct gfar_cpu_dev *this_cpu_dev = &__get_cpu_var(gfar_cpu_dev);
	struct gfar_cpu_dev *other_cpu_dev =
		&per_cpu(gfar_cpu_dev, !smp_processor_id());

	if (timer == &this_cpu_dev->intr_coalesce_timer) {
		fsl_send_msg(other_cpu_dev->msg_virtual_rx, 0x1);
		this_cpu_dev->intr_coalesce_cnt = 0;
	} else {
		fsl_send_msg(this_cpu_dev->msg_virtual_rx, 0x1);
		other_cpu_dev->intr_coalesce_cnt = 0;
	}

	return HRTIMER_NORESTART;
}

void gfar_cpu_dev_init(void)
{
	int err = -1;
	int i = 0;
	int j;
	struct gfar_cpu_dev *cpu_dev;
	struct cpumask cpumask_msg_intrs;

	for_each_possible_cpu(i) {
		cpu_dev = &per_cpu(gfar_cpu_dev, i);
		cpu_dev->enabled = 0;

		init_dummy_netdev(&cpu_dev->dev);
		netif_napi_add(&cpu_dev->dev,
			&cpu_dev->napi, gfar_cpu_poll, GFAR_DEV_WEIGHT);

		cpu_dev->msg_virtual_rx = fsl_get_msg_unit();
		if (IS_ERR(cpu_dev->msg_virtual_rx)) {
			printk(KERN_WARNING
				"%s: fsl_get_msg_unit returned error %ld!\n",
				__func__, IS_ERR(cpu_dev->msg_virtual_rx));
			goto msg_fail;
		}

		sprintf(cpu_dev->int_name, "cpu%d_vrx", i);
		err = request_irq(cpu_dev->msg_virtual_rx->irq,
			gfar_cpu_receive, 0, cpu_dev->int_name, NULL);
		if (err < 0) {
			printk(KERN_WARNING "Can't request msg IRQ %d\n",
				cpu_dev->msg_virtual_rx->irq);
			goto irq_fail;
		}
		cpumask_clear(&cpumask_msg_intrs);
		cpumask_set_cpu(i, &cpumask_msg_intrs);
		irq_set_affinity(cpu_dev->msg_virtual_rx->irq,
					&cpumask_msg_intrs);
		fsl_enable_msg(cpu_dev->msg_virtual_rx);

		for (j = 0; j < GFAR_CPU_BUFF_SIZE; j++)
			cpu_dev->tx_queue.buffer[j] = NULL;

		cpu_dev->tx_queue.in = 0;
		cpu_dev->tx_queue.out = 0;
		cpu_dev->tx_queue.buff_cnt.counter = 0;

		napi_enable(&cpu_dev->napi);

		cpu_dev->intr_coalesce_cnt = 0;
		hrtimer_init(&cpu_dev->intr_coalesce_timer, CLOCK_MONOTONIC,
			 HRTIMER_MODE_ABS);
		cpu_dev->intr_coalesce_timer.function = gfar_cpu_timer_handle;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
		gfar_reset_skb_handler(&cpu_dev->sh);
#endif

		cpu_dev->enabled = 1;
	}
	return;

irq_fail:
	fsl_release_msg_unit(cpu_dev->msg_virtual_rx);

msg_fail:
	netif_napi_del(&cpu_dev->napi);

	for (j = 0; j < i; j++) {
		cpu_dev = &per_cpu(gfar_cpu_dev, j);

		cpu_dev->enabled = 0;
		napi_disable(&cpu_dev->napi);
		free_irq(cpu_dev->msg_virtual_rx->irq, NULL);
		fsl_release_msg_unit(cpu_dev->msg_virtual_rx);
		netif_napi_del(&cpu_dev->napi);
	}
}

void gfar_cpu_dev_exit(void)
{
	int i = 0;
	struct gfar_cpu_dev *cpu_dev;

	for_each_possible_cpu(i) {
		cpu_dev = &per_cpu(gfar_cpu_dev, i);

		hrtimer_cancel(&cpu_dev->intr_coalesce_timer);
		napi_disable(&cpu_dev->napi);
		free_irq(cpu_dev->msg_virtual_rx->irq, NULL);
		fsl_release_msg_unit(cpu_dev->msg_virtual_rx);
		netif_napi_del(&cpu_dev->napi);
	}
}

int distribute_packet(struct net_device *dev,
			struct sk_buff *skb,
			int amount_pull)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar_cpu_dev *cpu_dev;
	int target_cpu;
	int current_cpu = smp_processor_id();
	unsigned char *skb_data;
	unsigned int skb_len;
	unsigned int eth_hdr_offset = 0;
	unsigned char *eth;
	struct shared_buffer *buf;
	ktime_t time;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	struct gfar_skb_handler *sh;
	struct sk_buff *new_skb;
#endif

	skb_data = skb->data;
	skb_len = skb->len;

	if (amount_pull)
		eth_hdr_offset += amount_pull;
	if (priv->ptimer_present)
		eth_hdr_offset += 8;

	if (eth_hdr_offset > skb_len)
		return -1;

	eth = skb_data + eth_hdr_offset;
	target_cpu = get_cpu_number(eth, skb_len - eth_hdr_offset);
	if (-1 == target_cpu)
		return -1;

	if (target_cpu == current_cpu)
		return -1;

	cpu_dev = &__get_cpu_var(gfar_cpu_dev);
	if (!cpu_dev->enabled)
		return -1;

	buf = &cpu_dev->tx_queue;
	if (atomic_read(&buf->buff_cnt) == (GFAR_CPU_BUFF_SIZE - 1)) {
		dev_kfree_skb_any(skb);    /* buffer full, drop packet */
		return 0;
	}
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	sh = &cpu_dev->sh;
	if (sh->recycle_count < sh->recycle_max) {
		if (buf->buffer[buf->in] != NULL)
			new_skb = buf->buffer[buf->in];
		else
			new_skb = gfar_new_skb(dev);

		/* put the obtained/allocated skb into
		current cpu's recycle buffer */
		if (new_skb) {
			new_skb->next = sh->recycle_queue;
			sh->recycle_queue = new_skb;
			sh->recycle_count++;
		}
	}
#endif

	/* inform other cpu which dev this skb was received on */
	skb->dev = dev;
	buf->buffer[buf->in] = skb;
	buf->in = (buf->in + 1) % GFAR_CPU_BUFF_SIZE;
	smp_wmb();
	atomic_inc(&buf->buff_cnt);

	/* raise other core's msg intr */
	if (0 == cpu_dev->intr_coalesce_cnt++) {
		time = ktime_set(0, 0);
		time = ktime_add_ns(time, INTR_COALESCE_TIMEOUT);
		hrtimer_start(&cpu_dev->intr_coalesce_timer,
			time, HRTIMER_MODE_ABS);
	} else {
		if (cpu_dev->intr_coalesce_cnt == INTR_COALESCE_CNT) {
			cpu_dev->intr_coalesce_cnt = 0;
			hrtimer_cancel(&cpu_dev->intr_coalesce_timer);
			fsl_send_msg(per_cpu
			(gfar_cpu_dev, target_cpu).msg_virtual_rx, 0x1);
		}
	}
	return 0;
}

static irqreturn_t gfar_virtual_transmit(int irq, void *grp_id)
{
	unsigned long flags;
	int cpu = smp_processor_id();
	struct gfar_priv_grp *grp = (struct gfar_priv_grp *)grp_id;
	int cpus = num_online_cpus();
	int i;

	for (i = 0; i < cpus; i++)
		/* clear the interrupt */
		/* although only one virtual tx intr is enabled at a time,
		 * we are clearing virtual tx intr of all cpus, to ensure
		 * this intr is cleared even if user sets wrong affinity
		 */
		fsl_clear_msg(grp->msg_virtual_tx[i]);

	local_irq_save(flags);
	if (napi_schedule_prep(&grp->napi_tx[cpu]))
		__napi_schedule(&grp->napi_tx[cpu]);

	local_irq_restore(flags);

	return IRQ_HANDLED;
}
#endif

/* Set up the ethernet device structure, private data,
 * and anything else we need before we start */
static int gfar_probe(struct of_device *ofdev,
		const struct of_device_id *match)
{
	u32 tempval;
	struct net_device *dev = NULL;
	struct gfar_private *priv = NULL;
	struct gfar __iomem *regs = NULL;
	int err = 0, i, grp_idx = 0;
	int len_devname;
	u32 rstat = 0, tstat = 0, rqueue = 0, tqueue = 0;
	u32 isrg = 0;
	u32 __iomem *baddr;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int j;
	int cpus = num_online_cpus();
#endif

#ifdef MY_ABC_HERE
	g_netif_count++;
	if ( g_internal_netif_num >= 0 &&
		 g_netif_count > g_internal_netif_num )
	{
		return -ENODEV;
	}
#endif

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
	syno_gianfar_drop_caches = 1;
#endif

	err = gfar_of_init(ofdev, &dev);

	if (err)
		return err;

	priv = netdev_priv(dev);
	priv->ndev = dev;
	priv->ofdev = ofdev;
	priv->node = ofdev->node;
	SET_NETDEV_DEV(dev, &ofdev->dev);

	if (priv->ptimer_present) {
		err = gfar_ptp_init(priv);
		if (err) {
			priv->ptimer_present = 0;
			printk(KERN_ERR "IEEE1588: ptp-timer init failed\n");
		}
		priv->filer_en = 1;
		pmuxcr_guts_write();
		printk(KERN_INFO "IEEE1588: ptp-timer initialized\n");
	}

	spin_lock_init(&priv->bflock);
	INIT_WORK(&priv->reset_task, gfar_reset_task);

	dev_set_drvdata(&ofdev->dev, priv);
	regs = priv->gfargrp[0].regs;

	/* Stop the DMA engine now, in case it was running before */
	/* (The firmware could have used it, and left it running). */
	gfar_halt(dev);

	/* Reset MAC layer */
	gfar_write(&regs->maccfg1, MACCFG1_SOFT_RESET);

	/* We need to delay at least 3 TX clocks */
	udelay(2);

	tempval = (MACCFG1_TX_FLOW | MACCFG1_RX_FLOW);
	gfar_write(&regs->maccfg1, tempval);

	/* Initialize MACCFG2. */
	gfar_write(&regs->maccfg2, MACCFG2_INIT_SETTINGS);

	/* Initialize ECNTRL */
	gfar_write(&regs->ecntrl, ECNTRL_INIT_SETTINGS);

	/* Set the dev->base_addr to the gfar reg region */
	dev->base_addr = (unsigned long) regs;

	SET_NETDEV_DEV(dev, &ofdev->dev);

	/* Fill in the dev structure */
	dev->watchdog_timeo = TX_TIMEOUT;
	dev->mtu = 1500;
	dev->netdev_ops = &gfar_netdev_ops;
	dev->ethtool_ops = &gfar_ethtool_ops;

#ifdef CONFIG_GIANFAR_TXNAPI
	/* Seperate napi for tx and rx for each group */
	for (i = 0; i < priv->num_grps; i++) {
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		for (j = 0; j < cpus; j++)
			netif_napi_add(dev, &priv->gfargrp[i].napi_tx[j],
#else
			netif_napi_add(dev, &priv->gfargrp[i].napi_tx,
#endif
				gfar_poll_tx, GFAR_DEV_WEIGHT);
		netif_napi_add(dev, &priv->gfargrp[i].napi_rx, gfar_poll_rx,
				GFAR_DEV_WEIGHT);
	}
#else
	/* Register for napi ...We are registering NAPI for each grp */
	for (i = 0; i < priv->num_grps; i++)
		netif_napi_add(dev, &priv->gfargrp[i].napi, gfar_poll, GFAR_DEV_WEIGHT);
#endif

	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_CSUM) {
		priv->rx_csum_enable = 1;
		dev->features |= NETIF_F_IP_CSUM | NETIF_F_SG | NETIF_F_HIGHDMA;
	} else
		priv->rx_csum_enable = 0;

	priv->vlgrp = NULL;

	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_VLAN)
		dev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX;

	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_EXTENDED_HASH) {
		priv->extended_hash = 1;
		priv->hash_width = 9;

		priv->hash_regs[0] = &regs->igaddr0;
		priv->hash_regs[1] = &regs->igaddr1;
		priv->hash_regs[2] = &regs->igaddr2;
		priv->hash_regs[3] = &regs->igaddr3;
		priv->hash_regs[4] = &regs->igaddr4;
		priv->hash_regs[5] = &regs->igaddr5;
		priv->hash_regs[6] = &regs->igaddr6;
		priv->hash_regs[7] = &regs->igaddr7;
		priv->hash_regs[8] = &regs->gaddr0;
		priv->hash_regs[9] = &regs->gaddr1;
		priv->hash_regs[10] = &regs->gaddr2;
		priv->hash_regs[11] = &regs->gaddr3;
		priv->hash_regs[12] = &regs->gaddr4;
		priv->hash_regs[13] = &regs->gaddr5;
		priv->hash_regs[14] = &regs->gaddr6;
		priv->hash_regs[15] = &regs->gaddr7;

	} else {
		priv->extended_hash = 0;
		priv->hash_width = 8;

		priv->hash_regs[0] = &regs->gaddr0;
		priv->hash_regs[1] = &regs->gaddr1;
		priv->hash_regs[2] = &regs->gaddr2;
		priv->hash_regs[3] = &regs->gaddr3;
		priv->hash_regs[4] = &regs->gaddr4;
		priv->hash_regs[5] = &regs->gaddr5;
		priv->hash_regs[6] = &regs->gaddr6;
		priv->hash_regs[7] = &regs->gaddr7;
	}

	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_PADDING)
		priv->padding = DEFAULT_PADDING;
	else
		priv->padding = 0;

	if (dev->features & NETIF_F_IP_CSUM  || priv->ptimer_present) {
		priv->padding = 0x8;
		dev->hard_header_len += GMAC_FCB_LEN;
	}

	/* Program the isrg regs only if number of grps > 1 */
	if (priv->num_grps > 1) {
		baddr = &regs->isrg0;
		for (i = 0; i < priv->num_grps; i++) {
			isrg |= (priv->gfargrp[i].rx_bit_map << ISRG_SHIFT_RX);
			isrg |= (priv->gfargrp[i].tx_bit_map << ISRG_SHIFT_TX);
			gfar_write(baddr, isrg);
			baddr++;
			isrg = 0x0;
		}
	}

	/* Need to reverse the bit maps as  bit_map's MSB is q0
	 * but, for_each_bit parses from right to left, which
	 * basically reverses the queue numbers */
	for (i = 0; i< priv->num_grps; i++) {
		priv->gfargrp[i].tx_bit_map = reverse_bitmap(
				priv->gfargrp[i].tx_bit_map, MAX_TX_QS);
		priv->gfargrp[i].rx_bit_map = reverse_bitmap(
				priv->gfargrp[i].rx_bit_map, MAX_RX_QS);
	}

	/* Calculate RSTAT, TSTAT, RQUEUE and TQUEUE values,
	 * also assign queues to groups */
	for (grp_idx = 0; grp_idx < priv->num_grps; grp_idx++) {
		priv->gfargrp[grp_idx].num_rx_queues = 0x0;
		for_each_bit(i, &priv->gfargrp[grp_idx].rx_bit_map,
				priv->num_rx_queues) {
			priv->gfargrp[grp_idx].num_rx_queues++;
			priv->rx_queue[i]->grp = &priv->gfargrp[grp_idx];
			rstat = rstat | (RSTAT_CLEAR_RHALT >> i);
			rqueue = rqueue | ((RQUEUE_EN0 | RQUEUE_EX0) >> i);
		}
		priv->gfargrp[grp_idx].num_tx_queues = 0x0;
		for_each_bit (i, &priv->gfargrp[grp_idx].tx_bit_map,
				priv->num_tx_queues) {
			priv->gfargrp[grp_idx].num_tx_queues++;
			priv->tx_queue[i]->grp = &priv->gfargrp[grp_idx];
			tstat = tstat | (TSTAT_CLEAR_THALT >> i);
			tqueue = tqueue | (TQUEUE_EN0 >> i);
		}
		priv->gfargrp[grp_idx].rstat = rstat;
		priv->gfargrp[grp_idx].tstat = tstat;
		rstat = tstat =0;
	}

	gfar_write(&regs->rqueue, rqueue);
	gfar_write(&regs->tqueue, tqueue);

	priv->rx_buffer_size = DEFAULT_RX_BUFFER_SIZE;
	priv->wk_buffer_size = DEFAULT_WK_BUFFER_SIZE;

	/* Initializing some of the rx/tx queue level parameters */
	for (i = 0; i < priv->num_tx_queues; i++) {
		priv->tx_queue[i]->tx_ring_size = DEFAULT_TX_RING_SIZE;
		priv->tx_queue[i]->num_txbdfree = DEFAULT_TX_RING_SIZE;
		priv->tx_queue[i]->txcoalescing = DEFAULT_TX_COALESCE;
		priv->tx_queue[i]->txic = DEFAULT_TXIC;
	}
	priv->rx_queue[priv->num_rx_queues-1]->rx_ring_size = DEFAULT_WK_RING_SIZE;

	/* enable filer if we are using multiple rx queues */
	if(priv->num_rx_queues > 1)
		priv->filer_en = 1;

	for (i = 0; i < priv->num_rx_queues; i++) {
		priv->rx_queue[i]->rx_ring_size = DEFAULT_RX_RING_SIZE;
		priv->rx_queue[i]->rxcoalescing = DEFAULT_RX_COALESCE;
		priv->rx_queue[i]->rxic = DEFAULT_RXIC;
		priv->rx_queue[i]->stats.rx_dropped = 0;
		priv->rx_queue[i]->stats.rx_bytes = 0;
		priv->rx_queue[i]->stats.rx_packets = 0;
	}

	/* Enable most messages by default */
	priv->msg_enable = (NETIF_MSG_IFUP << 1 ) - 1;

	/* Carrier starts down, phylib will bring it up */
	netif_carrier_off(dev);

	err = register_netdev(dev);

	if (err) {
		printk(KERN_ERR "%s: Cannot register net device, aborting.\n",
				dev->name);
		goto register_fail;
	}

	if ((priv->device_flags & FSL_GIANFAR_DEV_HAS_MAGIC_PACKET) ||
	    (priv->device_flags & FSL_GIANFAR_DEV_HAS_ARP_PACKET)) {
		device_set_wakeup_capable(&ofdev->dev, true);
		device_set_wakeup_enable(&ofdev->dev, false);
	}

	/* fill out IRQ number and name fields */
	len_devname = strlen(dev->name);
	for (i = 0; i < priv->num_grps; i++) {
		strncpy(&priv->gfargrp[i].int_name_tx[0], dev->name,
				len_devname);
		if (priv->device_flags & FSL_GIANFAR_DEV_HAS_MULTI_INTR) {
			strncpy(&priv->gfargrp[i].int_name_tx[len_devname],
				"_g", sizeof("_g"));
			priv->gfargrp[i].int_name_tx[
				strlen(priv->gfargrp[i].int_name_tx)] = i+48;
			strncpy(&priv->gfargrp[i].int_name_tx[strlen(
				priv->gfargrp[i].int_name_tx)],
				"_tx", sizeof("_tx") + 1);

			strncpy(&priv->gfargrp[i].int_name_rx[0], dev->name,
					len_devname);
			strncpy(&priv->gfargrp[i].int_name_rx[len_devname],
					"_g", sizeof("_g"));
			priv->gfargrp[i].int_name_rx[
				strlen(priv->gfargrp[i].int_name_rx)] = i+48;
			strncpy(&priv->gfargrp[i].int_name_rx[strlen(
				priv->gfargrp[i].int_name_rx)],
				"_rx", sizeof("_rx") + 1);

			strncpy(&priv->gfargrp[i].int_name_er[0], dev->name,
					len_devname);
			strncpy(&priv->gfargrp[i].int_name_er[len_devname],
				"_g", sizeof("_g"));
			priv->gfargrp[i].int_name_er[strlen(
					priv->gfargrp[i].int_name_er)] = i+48;
			strncpy(&priv->gfargrp[i].int_name_er[strlen(\
				priv->gfargrp[i].int_name_er)],
				"_er", sizeof("_er") + 1);
		} else
			priv->gfargrp[i].int_name_tx[len_devname] = '\0';
	}

	/* Initialize the filer table */
	gfar_init_filer_table(priv);

	/* Create all the sysfs files */
	gfar_init_sysfs(dev);

	/* Print out the device info */
	printk(KERN_INFO DEVICE_NAME "%pM\n", dev->name, dev->dev_addr);

	/* Even more device info helps when determining which kernel */
	/* provided which set of benchmarks. */
	printk(KERN_INFO "%s: Running with NAPI enabled\n", dev->name);
	for (i = 0; i < priv->num_rx_queues; i++)
		printk(KERN_INFO "%s: :RX BD ring size for Q[%d]: %d\n",
			dev->name, i, priv->rx_queue[i]->rx_ring_size);
	for(i = 0; i < priv->num_tx_queues; i++)
		 printk(KERN_INFO "%s:TX BD ring size for Q[%d]: %d\n",
			dev->name, i, priv->tx_queue[i]->tx_ring_size);


	return 0;

register_fail:
	if (priv->ptimer_present)
		gfar_ptp_cleanup(priv);
	unmap_group_regs(priv);
	free_tx_pointers(priv);
	free_rx_pointers(priv);
	if (priv->phy_node)
		of_node_put(priv->phy_node);
	if (priv->tbi_node)
		of_node_put(priv->tbi_node);
	free_netdev(dev);
	return err;
}

static int gfar_remove(struct of_device *ofdev)
{
	struct gfar_private *priv = dev_get_drvdata(&ofdev->dev);

	if (priv->phy_node)
		of_node_put(priv->phy_node);
	if (priv->tbi_node)
		of_node_put(priv->tbi_node);

	dev_set_drvdata(&ofdev->dev, NULL);

	unregister_netdev(priv->ndev);
	unmap_group_regs(priv);
	free_netdev(priv->ndev);

	return 0;
}

#ifdef CONFIG_PM
static void gfar_enable_filer(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 temp;

	lock_rx_qs(priv);

	temp = gfar_read(&regs->rctrl);
	temp |= RCTRL_FILREN;
	temp &= ~RCTRL_FSQEN;
	temp &= ~RCTRL_PRSDEP_MASK;
	temp |= RCTRL_PRSDEP_L2L3;
	gfar_write(&regs->rctrl, temp);

	unlock_rx_qs(priv);
}

static int gfar_get_ip(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct in_device *in_dev = (struct in_device *)dev->ip_ptr;
	struct in_ifaddr *ifa;

	if (in_dev != NULL) {
		ifa = (struct in_ifaddr *)in_dev->ifa_list;
		if (ifa != NULL) {
			memcpy(priv->ip_addr, &ifa->ifa_address, 4);
			return 0;
		}
	}
	return -ENOENT;
}

#ifdef CONFIG_SYNO_QORIQ_IGNORE_DEFAULT_GATEWAY_ARP
extern int g_default_gateway_mac_addr_h;
extern int g_default_gateway_mac_addr_l;
#endif

static void gfar_config_filer_table(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	u8 *ip_addr;
	u32 wakeup_ip, dest_mac_addr_h, dest_mac_addr_l;
	u32 rqfpr = 0x0;
	u32 rqfcr = RQFCR_RJE | RQFCR_CMP_MATCH;
	u8  rqfcr_queue = priv->num_rx_queues-1;
	int n_rule = 0;
	int i;

	if (gfar_get_ip(dev))
		printk(KERN_ERR "WOL: get the ip address error\n");
	ip_addr = priv->ip_addr;

	wakeup_ip = (*ip_addr << 24) | (*(ip_addr + 1) << 16) | \
		    (*(ip_addr + 2) << 8) | (*(ip_addr + 3));

	dest_mac_addr_h = (dev->dev_addr[0] << 16) | \
			  (dev->dev_addr[1] << 8) | dev->dev_addr[2];
	dest_mac_addr_l = (dev->dev_addr[3] << 16) | \
			  (dev->dev_addr[4] << 8) | dev->dev_addr[5];

	lock_rx_qs(priv);

	for (i = 0; i <= MAX_FILER_IDX; i++)
		gfar_write_filer(priv, i, rqfcr, rqfpr);

	/* ARP request filer, filling the packet to queue #1 */
	rqfcr = (rqfcr_queue << 10) | RQFCR_AND | RQFCR_CMP_EXACT | RQFCR_PID_MASK;
	rqfpr = RQFPR_ARQ;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	rqfcr = (rqfcr_queue << 10) | RQFCR_AND | RQFCR_CMP_EXACT | RQFCR_PID_PARSE;
	rqfpr = RQFPR_ARQ;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	rqfcr = (rqfcr_queue << 10) | RQFCR_AND | RQFCR_CMP_EXACT | RQFCR_PID_MASK;
	rqfpr = FPR_FILER_MASK;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

#ifdef CONFIG_SYNO_QORIQ_IGNORE_DEFAULT_GATEWAY_ARP
	/* Ignore ARP packet from default gateway */
	if (g_default_gateway_mac_addr_h != 0 && g_default_gateway_mac_addr_l != 0) {
	rqfcr = (rqfcr_queue << 10) | RQFCR_CLE | RQFCR_AND | RQFCR_CMP_EXACT | RQFCR_PID_SAH;
	rqfpr = g_default_gateway_mac_addr_h;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	rqfcr = (rqfcr_queue << 10) | RQFCR_AND | RQFCR_CMP_EXACT | RQFCR_PID_SAL;
	rqfpr = g_default_gateway_mac_addr_l;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	rqfcr = (rqfcr_queue << 10) | RQFCR_CLE | RQFCR_RJE | RQFCR_CMP_EXACT;
	rqfpr = 0x0;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);
	}
#endif

	/* DEST_IP address in ARP packet, filling it to queue #1 */
	rqfcr = RQFCR_GPI | (rqfcr_queue << 10) | RQFCR_CMP_EXACT | RQFCR_PID_DIA;
	rqfpr = wakeup_ip;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	/* Unicast packet, filling it to queue #1 */
	rqfcr = (rqfcr_queue << 10) | RQFCR_AND | RQFCR_CMP_EXACT | RQFCR_PID_DAH;
	rqfpr = dest_mac_addr_h;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	rqfcr = RQFCR_GPI | (rqfcr_queue << 10) | RQFCR_CMP_EXACT | RQFCR_PID_DAL;
	mb();
	rqfpr = dest_mac_addr_l;
	gfar_write_filer(priv, n_rule++, rqfcr, rqfpr);

	unlock_rx_qs(priv);
}

static int gfar_arp_suspend(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	unsigned long flags;
	u32 tempval;

	netif_device_detach(dev);

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
	syno_gianfar_drop_caches = 0;
#endif

	if (netif_running(dev)) {
		local_irq_save(flags);
		lock_tx_qs(priv);
		lock_rx_qs(priv);

		gfar_halt_tx_nodisable(dev);

		/* Disable Tx */
		tempval = gfar_read(&regs->maccfg1);
		tempval &= ~MACCFG1_TX_EN;
		gfar_write(&regs->maccfg1, tempval);

		unlock_rx_qs(priv);
		unlock_tx_qs(priv);
		local_irq_restore(flags);

		disable_napi(priv);

		gfar_halt_rx(dev);
		gfar_config_filer_table(dev);
		gfar_enable_filer(dev);
		gfar_rx_start(dev);
		gfar_write(&regs->imask, IMASK_INIT_CLEAR |
					IMASK_FGPI | IMASK_RXFEN0);
	}

	return 0;
}

static int gfar_wakeup_event(struct of_device *ofdev)
{
	struct gfar_private *priv = dev_get_drvdata(&ofdev->dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;

	if (priv->should_wake == 1)
		return 1;

	if ((priv->wol_opts & GIANFAR_WOL_ARP) &&
		(gfar_read(&regs->ievent) & (IEVENT_FGPI | IEVENT_RXF0)))
		return 1;

	if ((priv->wol_opts & GIANFAR_WOL_MAGIC) &&
		!(gfar_read(&regs->maccfg2) & MACCFG2_MPEN))
		return 1;

	return 0;
}

static int gfar_suspend(struct of_device *ofdev, pm_message_t state)
{
	struct gfar_private *priv = dev_get_drvdata(&ofdev->dev);
	struct net_device *dev = priv->ndev;
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	unsigned long flags;
	u32 tempval;

	int magic_packet = priv->wol_en &&
		(priv->wol_opts & GIANFAR_WOL_MAGIC) &&
		(priv->device_flags & FSL_GIANFAR_DEV_HAS_MAGIC_PACKET);
	int arp_packet = priv->wol_en &&
		(priv->wol_opts & GIANFAR_WOL_ARP) &&
		(priv->device_flags & FSL_GIANFAR_DEV_HAS_ARP_PACKET);

	priv->should_wake = 0;
	if (arp_packet) {
		pmc_enable_wake(ofdev, gfar_wakeup_event, 1);
		pmc_enable_lossless(1);
		gfar_arp_suspend(dev);
		return 0;
	}

	netif_device_detach(dev);

	if (netif_running(dev)) {
		local_irq_save(flags);
		lock_tx_qs(priv);
		lock_rx_qs(priv);

		gfar_halt_nodisable(dev);

		/* Disable Tx, and Rx if wake-on-LAN is disabled. */
		tempval = gfar_read(&regs->maccfg1);

		tempval &= ~MACCFG1_TX_EN;

		if (!magic_packet)
			tempval &= ~MACCFG1_RX_EN;

		gfar_write(&regs->maccfg1, tempval);

		unlock_rx_qs(priv);
		unlock_tx_qs(priv);
		local_irq_restore(flags);

		disable_napi(priv);

		if (magic_packet) {
			pmc_enable_wake(ofdev, gfar_wakeup_event, 1);
			/* Enable interrupt on Magic Packet */
			gfar_write(&regs->imask, IMASK_MAG);

			/* Enable Magic Packet mode */
			tempval = gfar_read(&regs->maccfg2);
			tempval |= MACCFG2_MPEN;
			gfar_write(&regs->maccfg2, tempval);
		} else {
			phy_stop(priv->phydev);
		}
	}

	return 0;
}

static int gfar_arp_resume(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);

	if (!netif_running(dev)) {
		netif_device_attach(dev);
		return 0;
	}

	gfar_tx_start(dev);
	stop_gfar(dev);
	gfar_halt_rx(dev);
	gfar_init_filer_table(priv);
	startup_gfar(dev);
	gfar_rx_start(dev);

	netif_device_attach(dev);
	enable_napi(priv);

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
	syno_gianfar_drop_caches = 1;
#endif

	return 0;
}

static int gfar_resume(struct of_device *ofdev)
{
	struct gfar_private *priv = dev_get_drvdata(&ofdev->dev);
	struct net_device *dev = priv->ndev;
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	unsigned long flags;
	u32 tempval;
	int magic_packet = priv->wol_en &&
		(priv->wol_opts & GIANFAR_WOL_MAGIC) &&
		(priv->device_flags & FSL_GIANFAR_DEV_HAS_MAGIC_PACKET);
	int arp_packet = priv->wol_en &&
		(priv->wol_opts & GIANFAR_WOL_ARP) &&
		(priv->device_flags & FSL_GIANFAR_DEV_HAS_ARP_PACKET);

	if (arp_packet) {
		pmc_enable_wake(ofdev, NULL, 0);
		pmc_enable_lossless(0);
		gfar_arp_resume(dev);
		return 0;
	} else if (magic_packet) {
		pmc_enable_wake(ofdev, NULL, 0);
	}

	if (!netif_running(dev)) {
		netif_device_attach(dev);
		return 0;
	}

	if (!magic_packet && priv->phydev)
		phy_start(priv->phydev);

	/* Disable Magic Packet mode, in case something
	 * else woke us up.
	 */
	local_irq_save(flags);
	lock_tx_qs(priv);
	lock_rx_qs(priv);

	tempval = gfar_read(&regs->maccfg2);
	tempval &= ~MACCFG2_MPEN;
	gfar_write(&regs->maccfg2, tempval);

	gfar_start(dev);

	unlock_rx_qs(priv);
	unlock_tx_qs(priv);
	local_irq_restore(flags);

	netif_device_attach(dev);

	enable_napi(priv);

	return 0;
}
#else
#define gfar_suspend NULL
#define gfar_resume NULL
#endif

/* Reads the controller's registers to determine what interface
 * connects it to the PHY.
 */
static phy_interface_t gfar_get_interface(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 ecntrl;

	ecntrl = gfar_read(&regs->ecntrl);

	if (ecntrl & ECNTRL_SGMII_MODE)
		return PHY_INTERFACE_MODE_SGMII;

	if (ecntrl & ECNTRL_TBI_MODE) {
		if (ecntrl & ECNTRL_REDUCED_MODE)
			return PHY_INTERFACE_MODE_RTBI;
		else
			return PHY_INTERFACE_MODE_TBI;
	}

	if (ecntrl & ECNTRL_REDUCED_MODE) {
		if (ecntrl & ECNTRL_REDUCED_MII_MODE)
			return PHY_INTERFACE_MODE_RMII;
		else {
			phy_interface_t interface = priv->interface;

			/*
			 * This isn't autodetected right now, so it must
			 * be set by the device tree or platform code.
			 */
			if (interface == PHY_INTERFACE_MODE_RGMII_ID)
				return PHY_INTERFACE_MODE_RGMII_ID;

			return PHY_INTERFACE_MODE_RGMII;
		}
	}

	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_GIGABIT)
		return PHY_INTERFACE_MODE_GMII;

	return PHY_INTERFACE_MODE_MII;
}


/* Initializes driver's PHY state, and attaches to the PHY.
 * Returns 0 on success.
 */
static int init_phy(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	uint gigabit_support =
		priv->device_flags & FSL_GIANFAR_DEV_HAS_GIGABIT ?
		SUPPORTED_1000baseT_Full : 0;
	phy_interface_t interface;

	priv->oldlink = 0;
	priv->oldspeed = 0;
	priv->oldduplex = -1;

	interface = gfar_get_interface(dev);

	priv->phydev = of_phy_connect(dev, priv->phy_node, &adjust_link, 0,
				      interface);
	if (!priv->phydev)
		priv->phydev = of_phy_connect_fixed_link(dev, &adjust_link,
							 interface);
	if (!priv->phydev) {
		dev_err(&dev->dev, "could not attach to PHY\n");
		return -ENODEV;
	}

	if (interface == PHY_INTERFACE_MODE_SGMII)
		gfar_configure_serdes(dev);

	/* Remove any features not supported by the controller */
	priv->phydev->supported &= (GFAR_SUPPORTED | gigabit_support);
	priv->phydev->advertising = priv->phydev->supported;

	return 0;
}

/*
 * Initialize TBI PHY interface for communicating with the
 * SERDES lynx PHY on the chip.  We communicate with this PHY
 * through the MDIO bus on each controller, treating it as a
 * "normal" PHY at the address found in the TBIPA register.  We assume
 * that the TBIPA register is valid.  Either the MDIO bus code will set
 * it to a value that doesn't conflict with other PHYs on the bus, or the
 * value doesn't matter, as there are no other PHYs on the bus.
 */
static void gfar_configure_serdes(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct phy_device *tbiphy;

	if (!priv->tbi_node) {
		dev_warn(&dev->dev, "error: SGMII mode requires that the "
				    "device tree specify a tbi-handle\n");
		return;
	}

	tbiphy = of_phy_find_device(priv->tbi_node);
	if (!tbiphy) {
		dev_err(&dev->dev, "error: Could not get TBI device\n");
		return;
	}

	/*
	 * If the link is already up, we must already be ok, and don't need to
	 * configure and reset the TBI<->SerDes link.  Maybe U-Boot configured
	 * everything for us?  Resetting it takes the link down and requires
	 * several seconds for it to come back.
	 */
	if (phy_read(tbiphy, MII_BMSR) & BMSR_LSTATUS)
		return;

	/* Single clk mode, mii mode off(for serdes communication) */
	phy_write(tbiphy, MII_TBICON, TBICON_CLK_SELECT);

	phy_write(tbiphy, MII_ADVERTISE,
			ADVERTISE_1000XFULL | ADVERTISE_1000XPAUSE |
			ADVERTISE_1000XPSE_ASYM);

	phy_write(tbiphy, MII_BMCR, BMCR_ANENABLE |
			BMCR_ANRESTART | BMCR_FULLDPLX | BMCR_SPEED1000);
}

static void init_registers(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = NULL;
	int i = 0;

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Clear IEVENT */
		gfar_write(&regs->ievent, IEVENT_INIT_CLEAR);

		/* Initialize IMASK */
		gfar_write(&regs->imask, IMASK_INIT_CLEAR);
	}

	regs = priv->gfargrp[0].regs;
	/* Init hash registers to zero */
	gfar_write(&regs->igaddr0, 0);
	gfar_write(&regs->igaddr1, 0);
	gfar_write(&regs->igaddr2, 0);
	gfar_write(&regs->igaddr3, 0);
	gfar_write(&regs->igaddr4, 0);
	gfar_write(&regs->igaddr5, 0);
	gfar_write(&regs->igaddr6, 0);
	gfar_write(&regs->igaddr7, 0);

	gfar_write(&regs->gaddr0, 0);
	gfar_write(&regs->gaddr1, 0);
	gfar_write(&regs->gaddr2, 0);
	gfar_write(&regs->gaddr3, 0);
	gfar_write(&regs->gaddr4, 0);
	gfar_write(&regs->gaddr5, 0);
	gfar_write(&regs->gaddr6, 0);
	gfar_write(&regs->gaddr7, 0);

	/* Zero out the rmon mib registers if it has them */
	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_RMON) {
		memset_io(&(regs->rmon), 0, sizeof (struct rmon_mib));

		/* Mask off the CAM interrupts */
		gfar_write(&regs->rmon.cam1, 0xffffffff);
		gfar_write(&regs->rmon.cam2, 0xffffffff);
	}

	/* Initialize the max receive buffer length */
	gfar_write(&regs->mrblr, priv->rx_buffer_size);

	/* Initialize the Minimum Frame Length Register */
	gfar_write(&regs->minflr, MINFLR_INIT_SETTINGS);
}


/* Halt the receive and transmit queues */
static void gfar_halt_nodisable(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = NULL;
	u32 tempval;
	int i = 0;

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Mask all interrupts */
		gfar_write(&regs->imask, IMASK_INIT_CLEAR);

		/* Clear all interrupts */
		gfar_write(&regs->ievent, IEVENT_INIT_CLEAR);
	}

	regs = priv->gfargrp[0].regs;
	/* Stop the DMA, and wait for it to stop */
	tempval = gfar_read(&regs->dmactrl);
	if ((tempval & (DMACTRL_GRS | DMACTRL_GTS))
	    != (DMACTRL_GRS | DMACTRL_GTS)) {
		tempval |= (DMACTRL_GRS | DMACTRL_GTS);
		gfar_write(&regs->dmactrl, tempval);

		while ((gfar_read(&regs->ievent) &
			 (IEVENT_GRSC | IEVENT_GTSC))!=
			 (IEVENT_GRSC | IEVENT_GTSC))
			cpu_relax();
		gfar_write(&regs->ievent, IEVENT_GRSC | IEVENT_GTSC);
	}
}

#ifdef CONFIG_PM
/* Halt the receive queues */
static void gfar_halt_rx(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 tempval;
	int i = 0;

	/* Disable Rx in MACCFG1  */
	tempval = gfar_read(&regs->maccfg1);
	tempval &= ~MACCFG1_RX_EN;
	gfar_write(&regs->maccfg1, tempval);

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Mask all interrupts */
		gfar_write(&regs->imask, IMASK_INIT_CLEAR);

		/* Clear all interrupts */
		gfar_write(&regs->ievent, IEVENT_INIT_CLEAR);
	}

	regs = priv->gfargrp[0].regs;
	/* Stop the DMA, and wait for it to stop */
	tempval = gfar_read(&regs->dmactrl);
	if ((tempval & DMACTRL_GRS) != DMACTRL_GRS) {
		tempval |= DMACTRL_GRS;
		gfar_write(&regs->dmactrl, tempval);

		while (!(gfar_read(&regs->ievent) & IEVENT_GRSC))
			cpu_relax();
		gfar_write(&regs->ievent, IEVENT_GRSC);
	}
}

/* Halt the transmit queues */
static void gfar_halt_tx_nodisable(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = NULL;
	u32 tempval;
	int i = 0;

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Mask all interrupts */
		gfar_write(&regs->imask, IMASK_INIT_CLEAR);

		/* Clear all interrupts */
		gfar_write(&regs->ievent, IEVENT_INIT_CLEAR);
	}

	regs = priv->gfargrp[0].regs;
	/* Stop the DMA, and wait for it to stop */
	tempval = gfar_read(&regs->dmactrl);
	if ((tempval & DMACTRL_GTS) != DMACTRL_GTS) {
		tempval |= DMACTRL_GTS;
		gfar_write(&regs->dmactrl, tempval);

		while (!(gfar_read(&regs->ievent) & IEVENT_GTSC))
			cpu_relax();
	}
}
#endif

/* Halt the receive and transmit queues */
void gfar_halt(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 tempval;

	gfar_halt_nodisable(dev);

	/* Disable Rx and Tx */
	tempval = gfar_read(&regs->maccfg1);
	tempval &= ~(MACCFG1_RX_EN | MACCFG1_TX_EN);
	gfar_write(&regs->maccfg1, tempval);
}

static void free_grp_irqs(struct gfar_priv_grp *grp)
{
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int i;
	struct gfar_private *priv = grp->priv;
	int cpus = num_online_cpus();

	if (priv->sps) {
		for (i = 0; i < cpus; i++) {
			free_irq(grp->msg_virtual_tx[i]->irq, grp);
			fsl_release_msg_unit(grp->msg_virtual_tx[i]);
		}
	}
#endif
	free_irq(grp->interruptError, grp);
	free_irq(grp->interruptTransmit, grp);
	free_irq(grp->interruptReceive, grp);
}

void free_bds(struct gfar_private *priv)
{
	unsigned long region_size = 0;
	region_size = (sizeof(struct txbd8) + sizeof(struct sk_buff *)) *
			priv->total_tx_ring_size +
			(sizeof(struct rxbd8) + sizeof(struct sk_buff *)) *
			priv->total_rx_ring_size;
#ifdef CONFIG_GIANFAR_L2SRAM
	if (priv->bd_in_ram)
		dma_free_coherent(&priv->ofdev->dev,
			region_size,
			priv->tx_queue[0]->tx_bd_base,
			gfar_read(&(priv->gfargrp[0].regs)->tbase0));
	else
		mpc85xx_cache_sram_free(priv->tx_queue[0]->tx_bd_base);
#else
	dma_free_coherent(&priv->ofdev->dev,
			region_size,
			priv->tx_queue[0]->tx_bd_base,
			gfar_read(&(priv->gfargrp[0].regs)->tbase0));
#endif
}

void stop_gfar(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	unsigned long flags;
	int i = 0;

	phy_stop(priv->phydev);

	/* Lock it down */
	local_irq_save(flags);
	lock_tx_qs(priv);
	lock_rx_qs(priv);

	gfar_halt(dev);

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	priv->skbuff_truesize = 0;
#endif
	unlock_rx_qs(priv);
	unlock_tx_qs(priv);
	local_irq_restore(flags);

	if (priv->ptimer_present)
		gfar_1588_stop(dev);

	/* Free the IRQs */
	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_MULTI_INTR) {
		for (i = 0; i < priv->num_grps; i++)
			free_grp_irqs(&priv->gfargrp[i]);
	} else {
		for (i = 0; i < priv->num_grps; i++)
			free_irq(priv->gfargrp[i].interruptTransmit,
					&priv->gfargrp[i]);
	}

	free_skb_resources(priv);
	free_bds(priv);
}

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
/*
 * function: gfar_reset_skb_handler
 * Resetting skb handler spin lock entry in the driver initialization.
 * Execute only once.
 */
static void gfar_reset_skb_handler(struct gfar_skb_handler *sh)
{
	spin_lock_init(&sh->lock);
	sh->recycle_max = GFAR_DEFAULT_RECYCLE_MAX;
	sh->recycle_count = 0;
	sh->recycle_queue = NULL;
	sh->recycle_enable = 1;
}

/*
 * function: gfar_free_recycle_queue
 * Reset SKB handler struction and free existance socket buffer
 * and data buffer in the recycling queue
 */
void gfar_free_recycle_queue(struct gfar_skb_handler *sh, int lock_flag)
{
	unsigned long flags = 0;
	struct sk_buff *clist = NULL;
	struct sk_buff *skb;
	/* Get recycling queue */
	/* just for making sure there is recycle_queue */
	if (lock_flag)
		spin_lock_irqsave(&sh->lock, flags);
	if (sh->recycle_queue) {
		/* pick one from head; most recent one */
		clist = sh->recycle_queue;
		sh->recycle_enable = 0;
		sh->recycle_count = 0;
		sh->recycle_queue = NULL;
	}
	if (lock_flag)
		spin_unlock_irqrestore(&sh->lock, flags);
	while (clist) {
		skb = clist;
		clist = clist->next;
		dev_kfree_skb_any(skb);
	}
}
#endif

static void free_skb_tx_queue(struct gfar_priv_tx_q *tx_queue)
{
	struct txbd8 *txbdp;
	struct gfar_private *priv = netdev_priv(tx_queue->dev);
	int i, j;

	txbdp = tx_queue->tx_bd_base;

	for (i = 0; i < tx_queue->tx_ring_size; i++) {
		if (!tx_queue->tx_skbuff[i])
			continue;

		dma_unmap_single(&priv->ofdev->dev, txbdp->bufPtr,
				txbdp->length, DMA_TO_DEVICE);
		txbdp->lstatus = 0;
		for (j = 0; j < skb_shinfo(tx_queue->tx_skbuff[i])->nr_frags; j++) {
			txbdp++;
			dma_unmap_page(&priv->ofdev->dev, txbdp->bufPtr,
					txbdp->length, DMA_TO_DEVICE);
		}
		txbdp++;
		dev_kfree_skb_any(tx_queue->tx_skbuff[i]);
		tx_queue->tx_skbuff[i] = NULL;
	}
#ifndef CONFIG_GIANFAR_L2SRAM
	kfree(tx_queue->tx_skbuff);
#endif
}

static void free_skb_rx_queue(struct gfar_priv_rx_q *rx_queue)
{
	struct rxbd8 *rxbdp;
	struct gfar_private *priv = netdev_priv(rx_queue->dev);
	int i;

	rxbdp = rx_queue->rx_bd_base;

	/* rx_skbuff is not guaranteed to be allocated, so only
	 * free it and its contents if it is allocated */
	if (rx_queue->rx_skbuff != NULL) {
		for (i = 0; i < rx_queue->rx_ring_size; i++) {
			if (rx_queue->rx_skbuff[i]) {
				dma_unmap_single(&priv->ofdev->dev,
				rxbdp->bufPtr, priv->rx_buffer_size,
					DMA_FROM_DEVICE);

				dev_kfree_skb_any(rx_queue->rx_skbuff[i]);
				rx_queue->rx_skbuff[i] = NULL;
			}

			rxbdp->lstatus = 0;
			rxbdp->bufPtr = 0;

			rxbdp++;
		}
#ifndef CONFIG_GIANFAR_L2SRAM
		kfree(rx_queue->rx_skbuff);
#endif
	}
}

/* If there are any tx skbs or rx skbs still around, free them.
 * Then free tx_skbuff and rx_skbuff */
static void free_skb_resources(struct gfar_private *priv)
{
	struct gfar_priv_tx_q *tx_queue = NULL;
	struct gfar_priv_rx_q *rx_queue = NULL;
	int i, cpu;

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	//disable_napi(priv);
	/* 1: spinlocking of skb_handler is required */
	gfar_free_recycle_queue(&(priv->skb_handler), 1);
	for (i = 0; i < priv->num_tx_queues ; i++) {
		for_each_possible_cpu(cpu) {
			gfar_free_recycle_queue(
				per_cpu_ptr(priv->tx_queue[i]->local_sh,
								cpu), 0);
		}
		free_percpu(priv->tx_queue[i]->local_sh);
	}

	for (i = 0; i < priv->num_rx_queues ; i++) {
		/* 1: spinlocking of skb_handler is required */
		gfar_free_recycle_queue(&(priv->rx_queue[i]->skb_handler), 1);
		for_each_possible_cpu(cpu) {
			gfar_free_recycle_queue(
				per_cpu_ptr(priv->rx_queue[i]->local_sh,
								cpu), 0);
		}
		free_percpu(priv->rx_queue[i]->local_sh);
	}
#endif

	if(( priv->device_flags & FSL_GIANFAR_DEV_HAS_ARP_PACKET)) {
		rx_queue = priv->rx_queue[priv->num_rx_queues-1];
		dma_free_coherent(&priv->ofdev->dev,
			   priv->wk_buffer_size * rx_queue->rx_ring_size \
			   + RXBUF_ALIGNMENT, (void *)priv->wk_buf_vaddr,
			   priv->wk_buf_paddr);
	}
	/* Go through all the buffer descriptors and free their data buffers */
	for (i = 0; i < priv->num_tx_queues; i++) {
		tx_queue = priv->tx_queue[i];
		free_skb_tx_queue(tx_queue);
	}

	for (i = 0; i < priv->num_rx_queues; i++) {
		rx_queue = priv->rx_queue[i];
		free_skb_rx_queue(rx_queue);
	}
}

void gfar_start(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 tempval;
	int i = 0;

	/* Enable Rx and Tx in MACCFG1 */
	tempval = gfar_read(&regs->maccfg1);
	tempval |= (MACCFG1_RX_EN | MACCFG1_TX_EN);
	gfar_write(&regs->maccfg1, tempval);

	/* Initialize DMACTRL to have WWR and WOP */
	tempval = gfar_read(&regs->dmactrl);
	tempval |= DMACTRL_INIT_SETTINGS;
	gfar_write(&regs->dmactrl, tempval);

	/* Make sure we aren't stopped */
	tempval = gfar_read(&regs->dmactrl);
	tempval &= ~(DMACTRL_GRS | DMACTRL_GTS);
	gfar_write(&regs->dmactrl, tempval);

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Clear THLT/RHLT, so that the DMA starts polling now */
		gfar_write(&regs->tstat, priv->gfargrp[i].tstat);
		gfar_write(&regs->rstat, priv->gfargrp[i].rstat);
		/* Unmask the interrupts we look for */
		gfar_write(&regs->imask, IMASK_DEFAULT);
	}

	dev->trans_start = jiffies;
}

#ifdef CONFIG_PM
void gfar_rx_start(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 tempval;
	int i = 0;

	/* Enable Rx in MACCFG1 */
	tempval = gfar_read(&regs->maccfg1);
	tempval |= MACCFG1_RX_EN;
	gfar_write(&regs->maccfg1, tempval);

	/* Make sure we aren't stopped */
	tempval = gfar_read(&regs->dmactrl);
	tempval &= ~DMACTRL_GRS;
	gfar_write(&regs->dmactrl, tempval);

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Clear RHLT, so that the DMA starts polling now */
		gfar_write(&regs->rstat, priv->gfargrp[i].rstat);
	}
}

void gfar_tx_start(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 tempval;
	int i = 0;

	/* Enable Tx in MACCFG1 */
	tempval = gfar_read(&regs->maccfg1);
	tempval |= MACCFG1_TX_EN;
	gfar_write(&regs->maccfg1, tempval);

	/* Make sure we aren't stopped */
	tempval = gfar_read(&regs->dmactrl);
	tempval &= ~DMACTRL_GTS;
	gfar_write(&regs->dmactrl, tempval);

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		/* Clear THLT, so that the DMA starts polling now */
		gfar_write(&regs->rstat, priv->gfargrp[i].tstat);
	}
}
#endif

void gfar_configure_tx_coalescing(struct gfar_private *priv,
				unsigned long tx_mask)
{
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 __iomem *baddr;
	int i = 0, mask = 0x1;

	/* Backward compatible case ---- even if we enable
	 * multiple queues, there's only single reg to program
	 */
	if (priv->mode == SQ_SG_MODE) {
		gfar_write(&regs->txic, 0);
		if (likely(priv->tx_queue[0]->txcoalescing))
			gfar_write(&regs->txic, priv->tx_queue[0]->txic);
	}

	if (priv->mode == MQ_MG_MODE) {
		baddr = &regs->txic0;
		for (i = 0; i < priv->num_tx_queues; i++) {
			if (tx_mask & mask) {
				if (likely(priv->tx_queue[i]->txcoalescing)) {
					gfar_write(baddr + i, 0);
					gfar_write(baddr + i,
						 priv->tx_queue[i]->txic);
				}
			}
			mask = mask << 0x1;
		}
	}
}

void gfar_configure_rx_coalescing(struct gfar_private *priv,
				unsigned long rx_mask)
{
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 __iomem *baddr;
	int i = 0, mask = 0x1;

	/* Backward compatible case ---- even if we enable
	 * multiple queues, there's only single reg to program
	 */
	if (priv->mode == SQ_SG_MODE) {
		gfar_write(&regs->rxic, 0);
		if (unlikely(priv->rx_queue[0]->rxcoalescing))
			gfar_write(&regs->rxic, priv->rx_queue[0]->rxic);
	}

	if (priv->mode == MQ_MG_MODE) {
		baddr = &regs->rxic0;
		for (i = 0; i < priv->num_rx_queues; i++) {
			if (rx_mask & mask) {
				if (likely(priv->rx_queue[i]->rxcoalescing)) {
					gfar_write(baddr + i, 0);
					gfar_write(baddr + i,
						priv->rx_queue[i]->rxic);
				}
			}
			mask = mask << 0x1;
		}
	}
}

static int register_grp_irqs(struct gfar_priv_grp *grp)
{
	struct gfar_private *priv = grp->priv;
	struct net_device *dev = priv->ndev;
	int err;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int i, j;
	int cpus = num_online_cpus();
	struct cpumask cpumask_msg_intrs;
#endif

	/* If the device has multiple interrupts, register for
	 * them.  Otherwise, only register for the one */
	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_MULTI_INTR) {
		/* Install our interrupt handlers for Error,
		 * Transmit, and Receive */
		if ((err = request_irq(grp->interruptError, gfar_error, 0,
				grp->int_name_er,grp)) < 0) {
			if (netif_msg_intr(priv))
				printk(KERN_ERR "%s: Can't get IRQ %d\n",
					dev->name, grp->interruptError);

				goto err_irq_fail;
		}

		if ((err = request_irq(grp->interruptTransmit, gfar_transmit,
				0, grp->int_name_tx, grp)) < 0) {
			if (netif_msg_intr(priv))
				printk(KERN_ERR "%s: Can't get IRQ %d\n",
					dev->name, grp->interruptTransmit);
			goto tx_irq_fail;
		}

		if ((err = request_irq(grp->interruptReceive, gfar_receive, 0,
				grp->int_name_rx, grp)) < 0) {
			if (netif_msg_intr(priv))
				printk(KERN_ERR "%s: Can't get IRQ %d\n",
					dev->name, grp->interruptReceive);
			goto rx_irq_fail;
		}
	} else {
		if ((err = request_irq(grp->interruptTransmit, gfar_interrupt, 0,
				grp->int_name_tx, grp)) < 0) {
			if (netif_msg_intr(priv))
				printk(KERN_ERR "%s: Can't get IRQ %d\n",
					dev->name, grp->interruptTransmit);
			goto err_irq_fail;
		}

	}

#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps) {
		for (i = 0; i < cpus; i++) {
			sprintf(grp->int_name_vtx[i], "%s_g%d_vtx%d",
				priv->ndev->name, grp->grp_id, i);
			err = request_irq(grp->msg_virtual_tx[i]->irq,
						gfar_virtual_transmit, 0,
						grp->int_name_vtx[i], grp);
			if (err < 0) {
				priv->sps = 0;
				printk(KERN_WARNING
				"%s: Can't request msg IRQ %d for dev %s\n",
				__func__,
				grp->msg_virtual_tx[i]->irq, dev->name);
				for (j = 0; j < i; j++) {
					free_irq(grp->msg_virtual_tx[j]->irq,
						grp);
					clrbits32(grp->msg_virtual_tx[j]->mer,
					1 << grp->msg_virtual_tx[j]->msg_num);
				}
				goto vtx_irq_fail;
			}
			cpumask_clear(&cpumask_msg_intrs);
			cpumask_set_cpu(i, &cpumask_msg_intrs);
			irq_set_affinity(grp->msg_virtual_tx[i]->irq,
						&cpumask_msg_intrs);
			fsl_enable_msg(grp->msg_virtual_tx[i]);
		}
	}
#endif
	return 0;

#ifdef CONFIG_GFAR_SW_PKT_STEERING
vtx_irq_fail:
	free_irq(grp->interruptReceive, grp);
#endif
rx_irq_fail:
	free_irq(grp->interruptTransmit, grp);
tx_irq_fail:
	free_irq(grp->interruptError, grp);
err_irq_fail:
	return err;

}

unsigned long alloc_bds(struct gfar_private *priv, dma_addr_t *addr)
{
	unsigned long vaddr;
	unsigned long region_size;

	region_size = (sizeof(struct txbd8) + sizeof(struct sk_buff *)) *
	               priv->total_tx_ring_size +
		      (sizeof(struct rxbd8) + sizeof(struct sk_buff *)) *
	               priv->total_rx_ring_size;

#ifdef CONFIG_GIANFAR_L2SRAM
	vaddr = (unsigned long) mpc85xx_cache_sram_alloc(region_size,
					(phys_addr_t *)addr, ALIGNMENT);
	if (vaddr == NULL) {
		/* fallback to normal memory rather than stop working */
		vaddr = (unsigned long) dma_alloc_coherent(&priv->ofdev->dev,
				region_size, addr, GFP_KERNEL);
		priv->bd_in_ram = 1;
	} else {
		priv->bd_in_ram = 0;
	}
#else
	vaddr = (unsigned long) dma_alloc_coherent(&priv->ofdev->dev,
				region_size, addr, GFP_KERNEL);
#endif
	return vaddr;
}

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
#include <linux/syno_qoriq.h>
#include <linux/syscalls.h>
extern void syno_drop_caches(unsigned int cache_type);
#endif

/* Bring the controller up and running */
int startup_gfar(struct net_device *dev)
{
	struct txbd8 *txbdp;
	struct rxbd8 *rxbdp;
	struct rxbd8 *wkbdp;
	dma_addr_t addr = 0;
	unsigned long vaddr;
	unsigned long wk_buf_paddr;
	unsigned long wk_buf_vaddr;
	int i, j, k, cpu, num;
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar_priv_tx_q *tx_queue = NULL;
	struct gfar_priv_rx_q *rx_queue = NULL;
	struct gfar __iomem *regs = NULL;
	int err = 0;
	u32 *baddr;
	u32 rctrl = 0;
	u32 tctrl = 0;
	u32 attrs = 0;

	for (i = 0; i < priv->num_grps; i++) {
		regs = priv->gfargrp[i].regs;
		gfar_write(&regs->imask, IMASK_INIT_CLEAR);
	}

	regs = priv->gfargrp[0].regs;
	priv->total_tx_ring_size = 0;
	for (i = 0; i < priv->num_tx_queues; i++)
		priv->total_tx_ring_size += priv->tx_queue[i]->tx_ring_size;

	priv->total_rx_ring_size = 0;
	for (i = 0; i < priv->num_rx_queues; i++)
		priv->total_rx_ring_size += priv->rx_queue[i]->rx_ring_size;

	/* Allocate memory for the buffer descriptors */
	vaddr = alloc_bds(priv, &addr);

	if (vaddr == 0) {
		if (netif_msg_ifup(priv))
			printk(KERN_ERR "%s: Could not allocate buffer descriptors!\n",
					dev->name);
		return -ENOMEM;
	}

	baddr = &regs->tbase0;
	for (i = 0; i < priv->num_tx_queues; i++) {
		tx_queue = priv->tx_queue[i];
		tx_queue->tx_bd_base = (struct txbd8 *) vaddr;
		/* enet DMA only understands physical addresses */
		gfar_write(baddr, addr);
#ifdef CONFIG_PHYS_64BIT
		gfar_write(&regs->tbaseh, ((addr >> 32) & GFAR_TX_BASE_H));
#endif
		printk(KERN_DEBUG "txbd[%d]: addr, vaddr = 0x%llx,0x%lx\n",
				i, (unsigned long long)addr, (unsigned long)vaddr);
		addr    += sizeof(struct txbd8) *tx_queue->tx_ring_size;
		vaddr   += sizeof(struct txbd8) *tx_queue->tx_ring_size;
		baddr   += 2;
	}

	baddr = &regs->rbase0;
	/* Start the rx descriptor ring where the tx ring leaves off */
	for (i = 0; i < priv->num_rx_queues; i++) {
		rx_queue = priv->rx_queue[i];
		rx_queue->rx_bd_base = (struct rxbd8 *) vaddr;
		gfar_write(baddr, addr);
#ifdef CONFIG_PHYS_64BIT
		gfar_write(&regs->rbaseh, ((addr >> 32) & GFAR_RX_BASE_H));
#endif
		printk(KERN_DEBUG "rxbd[%d]: addr,vaddr=0x%llx,0x%lx\n",
				i, (unsigned long long)addr, (unsigned long)vaddr);
		addr    += sizeof (struct rxbd8) * rx_queue->rx_ring_size;
		vaddr   += sizeof (struct rxbd8) * rx_queue->rx_ring_size;
		baddr   += 2;
	}

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
	/* before allocate new skbuff drop all fs cache to avoid allocate failed  */
	if (syno_gianfar_drop_caches) {
		sys_sync();
		msleep(500);
		syno_drop_caches(SYNO_DROP_CACHE_PAGE | SYNO_DROP_CACHE_SLAB);
		printk("%s: flush cache before startup\n", dev->name);
	} else {
		printk("%s: Resume gianfar device, skip flush cache\n", dev->name);
	}
#endif

	/* Setup the skbuff rings */
	for (i = 0; i < priv->num_tx_queues; i++) {
		tx_queue = priv->tx_queue[i];
#ifdef CONFIG_GIANFAR_L2SRAM
		tx_queue->tx_skbuff = (struct sk_buff **) vaddr;
		vaddr += sizeof(struct sk_buff **) * tx_queue->tx_ring_size;
#else
		tx_queue->tx_skbuff =
			(struct sk_buff **)kmalloc(sizeof
				(struct sk_buff *) * tx_queue->tx_ring_size,
				GFP_KERNEL);
#endif
		if (NULL == tx_queue->tx_skbuff) {
			if (netif_msg_ifup(priv))
				printk(KERN_ERR
					"%s: Could not allocate	tx_skbuff\n",
					dev->name);
			err = -ENOMEM;
			goto tx_skb_fail;
		}

		for (k = 0; k < tx_queue->tx_ring_size; k++)
			tx_queue->tx_skbuff[k] = NULL;
	}

	for (i = 0; i < priv->num_rx_queues; i++) {
		rx_queue = priv->rx_queue[i];
#ifdef CONFIG_GIANFAR_L2SRAM
		rx_queue->rx_skbuff = (struct sk_buff **) vaddr;
		vaddr += sizeof(struct sk_buff **) * rx_queue->rx_ring_size;
#else
		rx_queue->rx_skbuff =
			(struct sk_buff **)kmalloc(sizeof (struct sk_buff *) *
					rx_queue->rx_ring_size, GFP_KERNEL);
#endif
		if (NULL == rx_queue->rx_skbuff) {
			if (netif_msg_ifup(priv))
				printk(KERN_ERR
					"%s: Could not allocate rx_skbuff\n",
					dev->name);
			err = -ENOMEM;
			goto rx_skb_fail;
		}

		for (j = 0; j < rx_queue->rx_ring_size; j++)
			rx_queue->rx_skbuff[j] = NULL;
	}

	/* Initialize some variables in our per tx/rx queue structure */
	for (i = 0; i < priv->num_tx_queues; i++) {
		tx_queue = priv->tx_queue[i];
		tx_queue->num_txbdfree = tx_queue->tx_ring_size;
		tx_queue->dirty_tx = tx_queue->tx_bd_base;
		tx_queue->cur_tx = tx_queue->tx_bd_base;
		tx_queue->skb_curtx = 0;
		tx_queue->skb_dirtytx = 0;
		txbdp = tx_queue->tx_bd_base;

		/* Initialize Transmit Descriptor Ring */
		for (j = 0; j < tx_queue->tx_ring_size; j++) {
			txbdp->lstatus = 0;
			txbdp->bufPtr = 0;
			txbdp++;
		}

		/* Set the last descriptor in the ring to indicate wrap */
		txbdp--;
		txbdp->status |= TXBD_WRAP;
	}

	if ((priv->device_flags & FSL_GIANFAR_DEV_HAS_ARP_PACKET))
		num = priv->num_rx_queues - 1;
	else
		num = priv->num_rx_queues;

	for (i = 0; i < num; i++) {
		rx_queue = priv->rx_queue[i];
		rx_queue->cur_rx = rx_queue->rx_bd_base;
		rx_queue->skb_currx = 0;
		rxbdp = rx_queue->rx_bd_base;

		for (j = 0; j < rx_queue->rx_ring_size; j++) {
			struct sk_buff *skb;

			skb = gfar_new_skb(dev);

			if (!skb) {
				printk(KERN_ERR
					"%s: Can't allocate RX buffers\n",
					dev->name);

				goto err_rxalloc_fail;
			}

			rx_queue->rx_skbuff[j] = skb;

			gfar_new_rxbdp(rx_queue, rxbdp, skb);

			rxbdp++;
		}

		/* Set the last descriptor in the ring to wrap */
		rxbdp--;
		rxbdp->status |= RXBD_WRAP;
	}

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	priv->skbuff_truesize = GFAR_DEFAULT_RECYCLE_TRUESIZE;
	gfar_reset_skb_handler(&(priv->skb_handler));
	for (i = 0;  i < priv->num_tx_queues; i++) {
		priv->tx_queue[i]->local_sh = alloc_percpu(
						struct gfar_skb_handler);
		for_each_possible_cpu(cpu) {
			gfar_reset_skb_handler(
				per_cpu_ptr(priv->tx_queue[i]->local_sh, cpu));
		}
	}
	for (i = 0;  i < priv->num_rx_queues; i++) {
		priv->rx_queue[i]->rx_skbuff_truesize =
					GFAR_DEFAULT_RECYCLE_TRUESIZE;
		gfar_reset_skb_handler(&(priv->rx_queue[i]->skb_handler));
		priv->rx_queue[i]->local_sh = alloc_percpu(
						struct gfar_skb_handler);

		for_each_possible_cpu(cpu) {
			gfar_reset_skb_handler(
				per_cpu_ptr(priv->rx_queue[i]->local_sh, cpu));
		}
	}
#endif

	if ((priv->device_flags & FSL_GIANFAR_DEV_HAS_ARP_PACKET)) {
	/* Alloc wake up rx buffer, wake up buffer need 64 bytes aligned */
		rx_queue = priv->rx_queue[priv->num_rx_queues-1];
		rx_queue->cur_rx = rx_queue->rx_bd_base;
		vaddr = (unsigned long) dma_alloc_coherent(&priv->ofdev->dev,
				priv->wk_buffer_size * rx_queue->rx_ring_size \
				+ RXBUF_ALIGNMENT, &addr, GFP_KERNEL);
		if (vaddr == 0) {
			if (netif_msg_ifup(priv))
				printk(KERN_ERR
					"%s:Could not allocate wakeup buffer!\n"					, dev->name);
			err = -ENOMEM;
			goto wk_buf_fail;
		}

		priv->wk_buf_vaddr = vaddr;
		priv->wk_buf_paddr = addr;
		wk_buf_vaddr = (unsigned long)(vaddr + RXBUF_ALIGNMENT) \
					       & ~(RXBUF_ALIGNMENT - 1);
		wk_buf_paddr = (unsigned long)(addr + RXBUF_ALIGNMENT) \
					       & ~(RXBUF_ALIGNMENT - 1);
		priv->wk_buf_align_vaddr = wk_buf_vaddr;
		priv->wk_buf_align_paddr = wk_buf_paddr;

		/* Setup wake up rx ring and buffer */
		wkbdp = rx_queue->rx_bd_base;
		for (i = 0; i < rx_queue->rx_ring_size; i++) {
			wkbdp->status = RXBD_EMPTY | RXBD_INTERRUPT;
			wkbdp->length = 0;
			wkbdp->bufPtr = wk_buf_paddr + priv->wk_buffer_size * i;
			wkbdp++;
		}

		/* Set the last descriptor in the ring to wrap */
		wkbdp--;
		wkbdp->status |= RXBD_WRAP;
	}

	for (i = 0; i < priv->num_grps; i++) {
		err = register_grp_irqs(&priv->gfargrp[i]);
		if (err) {
			for (j = 0; j < i; j++)
				free_grp_irqs(&priv->gfargrp[j]);
			goto irq_fail;
		}
	}

	phy_start(priv->phydev);

	gfar_configure_tx_coalescing(priv, 0xFF);
	gfar_configure_rx_coalescing(priv, 0xFF);

	if (priv->rx_csum_enable)
		rctrl |= RCTRL_CHECKSUMMING;

	if (priv->filer_en)
		rctrl |= RCTRL_FILREN;

	if (priv->extended_hash) {
		rctrl |= RCTRL_EXTHASH;

		gfar_clear_exact_match(dev);
		rctrl |= RCTRL_EMEN;
	}

	if (priv->padding) {
		rctrl &= ~RCTRL_PAL_MASK;
		rctrl |= RCTRL_PADDING(priv->padding);
	}

	if (priv->ptimer_present) {

		/* Enable Filer and Rx Packet Parsing capability of eTSEC */
		/* Set Filer Table */
		gfar_1588_start(dev);
		if (priv->device_flags & FSL_GIANFAR_DEV_HAS_PADDING)
			rctrl &= RCTRL_PAL_MASK;
		/* Enable Filer for Rx Queue */
		rctrl |= RCTRL_PRSDEP_INIT |
			RCTRL_TS_ENABLE | RCTRL_PADDING(8);
		priv->padding = 0x8;
	}

	/* keep vlan related bits if it's enabled */
	if (priv->vlgrp) {
		rctrl |= RCTRL_VLEX | RCTRL_PRSDEP_INIT;
		tctrl |= TCTRL_VLINS;
	}

	/* Init rctrl based on our settings */
	gfar_write(&regs->rctrl, rctrl);

	if (dev->features & NETIF_F_IP_CSUM)
		tctrl |= TCTRL_INIT_CSUM;

	tctrl |= TCTRL_TXSCHED_WRRS;

	gfar_write(&regs->tr03wt, WRRS_TR03WT);
	gfar_write(&regs->tr47wt, WRRS_TR47WT);

	gfar_write(&regs->tctrl, tctrl);

	/* Set the extraction length and index */
	attrs = ATTRELI_EL(priv->rx_stash_size) |
		ATTRELI_EI(priv->rx_stash_index);

	gfar_write(&regs->attreli, attrs);

	/* Start with defaults, and add stashing or locking
	 * depending on the approprate variables */
	attrs = ATTR_INIT_SETTINGS;

	if (priv->bd_stash_en)
		attrs |= ATTR_BDSTASH;

	if (priv->rx_stash_size != 0)
		attrs |= ATTR_BUFSTASH;

	gfar_write(&regs->attr, attrs);

	/* Start the controller */
	gfar_start(dev);

	return 0;

irq_fail:
wk_buf_fail:
	dma_free_coherent(&priv->ofdev->dev,
			priv->wk_buffer_size * priv->rx_queue[priv->num_rx_queues-1]->rx_ring_size \
			+ RXBUF_ALIGNMENT, (void *)priv->wk_buf_vaddr,
			priv->wk_buf_paddr);
err_rxalloc_fail:
rx_skb_fail:
tx_skb_fail:
	free_skb_resources(priv);
	free_bds(priv);
	return err;
}

/* Called when something needs to use the ethernet device */
/* Returns 0 for success. */
static int gfar_enet_open(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	int err;

	enable_napi(priv);

	/* Initialize a bunch of registers */
	init_registers(dev);

	gfar_set_mac_address(dev);

	err = init_phy(dev);

	if (err) {
		disable_napi(priv);
		return err;
	}

	err = startup_gfar(dev);
	if (err) {
		disable_napi(priv);
		return err;
	}

	netif_tx_start_all_queues(dev);

	device_set_wakeup_enable(&priv->ofdev->dev, priv->wol_en);

#ifdef CONFIG_SYNO_NET_INIT_OPERATION
	linkwatch_fire_event(dev);
#endif

	return err;
}

static inline struct txfcb *gfar_add_fcb(struct sk_buff *skb)
{
	struct txfcb *fcb = (struct txfcb *)skb_push(skb, GMAC_FCB_LEN);

	memset(fcb, 0, GMAC_FCB_LEN);

	return fcb;
}

static inline void gfar_tx_checksum(struct sk_buff *skb, struct txfcb *fcb)
{
	u8 flags = 0;

	/* If we're here, it's a IP packet with a TCP or UDP
	 * payload.  We set it to checksum, using a pseudo-header
	 * we provide
	 */
	flags = TXFCB_DEFAULT;

	/* Tell the controller what the protocol is */
	/* And provide the already calculated phcs */
	if (!((ip_hdr(skb)->frag_off) & htons(IP_MF|IP_OFFSET))) {
		/* If not fragmented packet */
		if (ip_hdr(skb)->protocol == IPPROTO_UDP) {
			if (udp_hdr(skb)->check) {
				fcb->phcs = udp_hdr(skb)->check;
				flags |= TXFCB_NPH;
			}
			flags |= TXFCB_UDP | TXFCB_TUP | TXFCB_CTU;
		} else if (ip_hdr(skb)->protocol == IPPROTO_TCP) {
			if (tcp_hdr(skb)->check) {
				flags |= TXFCB_NPH;
				fcb->phcs = tcp_hdr(skb)->check;
			}
			flags |= TXFCB_TUP | TXFCB_CTU;
		}
	}

	/* l3os is the distance between the start of the
	 * frame (skb->data) and the start of the IP hdr.
	 * l4os is the distance between the start of the
	 * l3 hdr and the l4 hdr */
	fcb->l3os = (u16)(skb_network_offset(skb) - GMAC_FCB_LEN);
	fcb->l4os = skb_network_header_len(skb);

	fcb->flags = flags;
}

void inline gfar_tx_vlan(struct sk_buff *skb, struct txfcb *fcb)
{
	fcb->flags |= TXFCB_VLN;
	fcb->vlctl = vlan_tx_tag_get(skb);
}

static inline struct txbd8 *skip_txbd(struct txbd8 *bdp, int stride,
			       struct txbd8 *base, int ring_size)
{
	struct txbd8 *new_bd = bdp + stride;

	return (new_bd >= (base + ring_size)) ? (new_bd - ring_size) : new_bd;
}

static inline struct txbd8 *next_txbd(struct txbd8 *bdp, struct txbd8 *base,
		int ring_size)
{
	return skip_txbd(bdp, 1, base, ring_size);
}

static int gfar_xmit_skb(struct sk_buff *skb, struct net_device *dev, int rq)
{
		struct gfar_private *priv = netdev_priv(dev);
		struct gfar_priv_tx_q *tx_queue = NULL;
		struct netdev_queue *txq;
		struct gfar __iomem *regs = NULL;
		struct txfcb *fcb = NULL;
		struct txbd8 *txbdp, *txbdp_start, *base;
		u32 lstatus;
		int i;
		u32 bufaddr;
		unsigned long flags;
		unsigned int nr_frags, length;

		tx_queue = priv->tx_queue[rq];
		txq = netdev_get_tx_queue(dev, rq);
		base = tx_queue->tx_bd_base;
		regs = tx_queue->grp->regs;

		/* total number of fragments in the SKB */
		nr_frags = skb_shinfo(skb)->nr_frags;

#ifdef CONFIG_GFAR_SW_PKT_STEERING
		if (!priv->sps)
#endif
			spin_lock_irqsave(&tx_queue->txlock, flags);

		/* check if there is space to queue this packet */
		if ((nr_frags+1) > tx_queue->num_txbdfree) {
			/* no space, stop the queue */
			netif_tx_stop_queue(txq);
			dev->stats.tx_fifo_errors++;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
			if (!priv->sps)
#endif
				spin_unlock_irqrestore(&tx_queue->txlock, flags);
			return NETDEV_TX_BUSY;
		}

		/* Update transmit stats */
		txq->tx_bytes += skb->len;
		txq->tx_packets++;

		txbdp = txbdp_start = tx_queue->cur_tx;

		if (nr_frags == 0) {
			lstatus = txbdp->lstatus | BD_LFLAG(TXBD_LAST | TXBD_INTERRUPT);
		} else {
			/* Place the fragment addresses and lengths into the TxBDs */
			for (i = 0; i < nr_frags; i++) {
				/* Point at the next BD, wrapping as needed */
				txbdp = next_txbd(txbdp, base, tx_queue->tx_ring_size);

				length = skb_shinfo(skb)->frags[i].size;

				lstatus = txbdp->lstatus | length |
					BD_LFLAG(TXBD_READY);

				/* Handle the last BD specially */
				if (i == nr_frags - 1)
					lstatus |= BD_LFLAG(TXBD_LAST | TXBD_INTERRUPT);

				bufaddr = dma_map_page(&priv->ofdev->dev,
						skb_shinfo(skb)->frags[i].page,
						skb_shinfo(skb)->frags[i].page_offset,
						length,
						DMA_TO_DEVICE);

				/* set the TxBD length and buffer pointer */
				txbdp->bufPtr = bufaddr;
				txbdp->lstatus = lstatus;
			}

			lstatus = txbdp_start->lstatus;
		}

		/* Set up checksumming */
		if (CHECKSUM_PARTIAL == skb->ip_summed) {
			fcb = gfar_add_fcb(skb);
			lstatus |= BD_LFLAG(TXBD_TOE);
			gfar_tx_checksum(skb, fcb);
		}

		if (priv->vlgrp && vlan_tx_tag_present(skb)) {
			if (unlikely(NULL == fcb)) {
				fcb = gfar_add_fcb(skb);
				lstatus |= BD_LFLAG(TXBD_TOE);
			}

			gfar_tx_vlan(skb, fcb);
		}

		if (priv->ptimer_present) {
			/* Enable ptp flag so that Tx time stamping happens */
			if (gfar_ptp_do_txstamp(skb)) {
				if (fcb == NULL)
					fcb = gfar_add_fcb(skb);
				fcb->ptp = 0x01;
				lstatus |= BD_LFLAG(TXBD_TOE);
			}
		}

		/* setup the TxBD length and buffer pointer for the first BD */
		tx_queue->tx_skbuff[tx_queue->skb_curtx] = skb;
		txbdp_start->bufPtr = dma_map_single(&priv->ofdev->dev, skb->data,
				skb_headlen(skb), DMA_TO_DEVICE);

		lstatus |= BD_LFLAG(TXBD_CRC | TXBD_READY) | skb_headlen(skb);

		/*
		 * The powerpc-specific eieio() is used, as wmb() has too strong
		 * semantics (it requires synchronization between cacheable and
		 * uncacheable mappings, which eieio doesn't provide and which we
		 * don't need), thus requiring a more expensive sync instruction.  At
		 * some point, the set of architecture-independent barrier functions
		 * should be expanded to include weaker barriers.
		 */
		eieio();

		txbdp_start->lstatus = lstatus;

		/* Update the current skb pointer to the next entry we will use
		 * (wrapping if necessary) */
		tx_queue->skb_curtx = (tx_queue->skb_curtx + 1) &
			TX_RING_MOD_MASK(tx_queue->tx_ring_size);

		tx_queue->cur_tx = next_txbd(txbdp, base, tx_queue->tx_ring_size);

		/* reduce TxBD free count */
		tx_queue->num_txbdfree -= (nr_frags + 1);

		txq->trans_start = jiffies;

		/* If the next BD still needs to be cleaned up, then the bds
		   are full.  We need to tell the kernel to stop sending us stuff. */
		if (!tx_queue->num_txbdfree) {
			netif_stop_subqueue(dev, tx_queue->qindex);

			dev->stats.tx_fifo_errors++;
		}

		/* Tell the DMA to go go go */
		gfar_write(&regs->tstat, TSTAT_CLEAR_THALT >> tx_queue->qindex);

		/* Unlock priv */
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		if (!priv->sps)
#endif
			spin_unlock_irqrestore(&tx_queue->txlock, flags);

		return NETDEV_TX_OK;

}

/*software TCP segmentation offload*/
static int gfar_tso(struct sk_buff *skb, struct net_device *dev, int rq)
{
	struct gfar_private *priv = netdev_priv(dev);
	int i = 0;
	struct iphdr *iph;
	int ihl;
	int id;
	unsigned int offset = 0;
	struct tcphdr *th;
	unsigned thlen;
	unsigned int seq;
	__be32 delta;
	unsigned int oldlen;
	unsigned int mss;
	unsigned int doffset;
	unsigned int headroom;
	unsigned int len;
	int nfrags;
	int pos;
	int hsize;
	int ret;
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	int howmany_reuse = 0;
	struct gfar_skb_handler *sh;
	int free_skb;
	struct sk_buff *local_head;
	unsigned long flags;
	struct gfar_skb_handler *local_sh;

	local_sh = per_cpu_ptr(priv->tx_queue[rq]->local_sh,
			smp_processor_id());

	if (local_sh->recycle_queue) {
		local_head = local_sh->recycle_queue;
		free_skb = local_sh->recycle_count;
		local_sh->recycle_queue = NULL;
		local_sh->recycle_count = 0;
	} else {
		local_head = NULL;
		free_skb = 0;
	}
	/* global skb_handler for this device */
	sh = &priv->skb_handler;
#endif
	/*processing mac header*/
	skb_reset_mac_header(skb);
	skb->mac_len = skb->network_header - skb->mac_header;
	__skb_pull(skb, skb->mac_len);

	/*processing IP header*/
	iph = ip_hdr(skb);
	ihl = iph->ihl * 4;
	__skb_pull(skb, ihl);
	skb_reset_transport_header(skb);
	iph = ip_hdr(skb);
	id = ntohs(iph->id);

	/*processing TCP header*/
	th = tcp_hdr(skb);
	thlen = th->doff * 4;
	oldlen = (u16)~skb->len;
	__skb_pull(skb, thlen);
	mss = skb_shinfo(skb)->gso_size;
	seq = ntohl(th->seq);
	delta = htonl(oldlen + (thlen + mss));

	/*processing SKB*/
	doffset = skb->data - skb_mac_header(skb);
	offset = doffset;
	nfrags = skb_shinfo(skb)->nr_frags;
	__skb_push(skb, doffset);
	headroom = skb_headroom(skb);
	pos = skb_headlen(skb);

	/*duplicating SKB*/
	hsize = skb_headlen(skb) - offset;
	if (hsize < 0)
		hsize = 0;

	do {
		struct sk_buff *nskb;
		skb_frag_t *frag;
		int size;

		len = skb->len - offset;
		if (len > mss)
			len = mss;

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
		if (!free_skb && sh->recycle_count) {
			/* refill local buffer */
			spin_lock_irqsave(&sh->lock, flags);
			local_head = sh->recycle_queue;
			free_skb = sh->recycle_count;
			sh->recycle_queue = NULL;
			sh->recycle_count = 0;
			spin_unlock_irqrestore(&sh->lock, flags);
		}
		if (local_head) {
			nskb = local_head;;
			local_head = nskb->next;
			nskb->next = NULL;
			free_skb--;
			howmany_reuse++;
		} else
			nskb = gfar_new_skb(dev);
#else
		nskb = alloc_skb(hsize + doffset + headroom,
					 GFP_ATOMIC);
#endif
		skb_reserve(nskb, headroom);
		__skb_put(nskb, doffset+hsize);

		nskb->ip_summed = skb->ip_summed;
		nskb->vlan_tci = skb->vlan_tci;
		nskb->mac_len = skb->mac_len;

		skb_reset_mac_header(nskb);
		skb_set_network_header(nskb, skb->mac_len);
		nskb->transport_header = (nskb->network_header +
					  skb_network_header_len(skb));
		skb_copy_from_linear_data(skb, nskb->data, doffset+hsize);
		frag = skb_shinfo(nskb)->frags;

		/*move skb data*/
		while (pos < offset + len && i < nfrags) {
			*frag = skb_shinfo(skb)->frags[i];
			get_page(frag->page);
			size = frag->size;

			if (pos < offset) {
				frag->page_offset += offset - pos;
				frag->size -= offset - pos;
			}

			skb_shinfo(nskb)->nr_frags++;

			if (pos + size <= offset + len) {
				i++;
				pos += size;
			} else {
				frag->size -= pos + size - (offset + len);
				goto skip_fraglist;
			}

			frag++;
		}

skip_fraglist:
		nskb->data_len = len - hsize;
		nskb->len += nskb->data_len;

		/*update TCP header*/
		if ((offset + len) >= skb->len)
			delta = htonl(oldlen + (nskb->tail -
				nskb->transport_header) + nskb->data_len);

		th = tcp_hdr(nskb);
		th->fin = th->psh = 0;
		th->seq = htonl(seq);
		th->cwr = 0;
		seq += mss;
		th->check = ~csum_fold((__force __wsum)((__force u32)th->check
				+ (__force u32)delta));

		/*update IP header*/
		iph = ip_hdr(nskb);
		iph->id = htons(id++);
		iph->tot_len = htons(nskb->len - nskb->mac_len);
		iph->check = 0;
		iph->check = ip_fast_csum(skb_network_header(nskb), iph->ihl);
		ret = gfar_xmit_skb(nskb, dev, rq);
		if (unlikely(ret != NETDEV_TX_OK)) {
			skb = nskb;
			goto out_tso;
		}
	} while ((offset += len) < skb->len);

out_tso:

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	if (free_skb) {
		/* return to local_sh for next time */
		local_sh->recycle_queue = local_head;
		local_sh->recycle_count = free_skb;
	}
	priv->extra_stats.rx_skbr += howmany_reuse;
#endif
	dev_kfree_skb_any(skb);
	return ret;
}

/* This is called by the kernel when a frame is ready for transmission. */
/* It is pointed to by the dev->hard_start_xmit function pointer */
static int gfar_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar_priv_tx_q *tx_queue = NULL;
	struct netdev_queue *txq;
	struct gfar __iomem *regs = NULL;
	struct txfcb *fcb = NULL;
	struct txbd8 *txbdp, *txbdp_start, *base;
	u32 lstatus;
	int i, rq = 0;
	u32 bufaddr;
	unsigned long flags;
	unsigned int nr_frags, length;


#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps)
		rq = smp_processor_id();
	else
#endif
		rq = skb->queue_mapping;
	tx_queue = priv->tx_queue[rq];
	txq = netdev_get_tx_queue(dev, rq);
	base = tx_queue->tx_bd_base;
	regs = tx_queue->grp->regs;

	/* make space for additional header when fcb is needed */
	if (((skb->ip_summed == CHECKSUM_PARTIAL) ||
			(priv->vlgrp && vlan_tx_tag_present(skb))) &&
			(skb_headroom(skb) < GMAC_FCB_LEN)) {
		struct sk_buff *skb_new;

		skb_new = skb_realloc_headroom(skb, GMAC_FCB_LEN);
		if (!skb_new) {
			dev->stats.tx_errors++;
			kfree_skb(skb);
			return NETDEV_TX_OK;
		}
		kfree_skb(skb);
		skb = skb_new;
	}

	if (skb_shinfo(skb)->gso_size)
		return gfar_tso(skb, dev, rq);

	/* total number of fragments in the SKB */
	nr_frags = skb_shinfo(skb)->nr_frags;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (!priv->sps)
#endif
		spin_lock_irqsave(&tx_queue->txlock, flags);

	/* check if there is space to queue this packet */
	if ((nr_frags+1) > tx_queue->num_txbdfree) {
		/* no space, stop the queue */
		netif_tx_stop_queue(txq);
		dev->stats.tx_fifo_errors++;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		if (!priv->sps)
#endif
			spin_unlock_irqrestore(&tx_queue->txlock, flags);
		return NETDEV_TX_BUSY;
	}

	/* Update transmit stats */
	txq->tx_bytes += skb->len;
	txq->tx_packets++;

	txbdp = txbdp_start = tx_queue->cur_tx;

	if (nr_frags == 0) {
		lstatus = txbdp->lstatus | BD_LFLAG(TXBD_LAST | TXBD_INTERRUPT);
	} else {
		/* Place the fragment addresses and lengths into the TxBDs */
		for (i = 0; i < nr_frags; i++) {
			/* Point at the next BD, wrapping as needed */
			txbdp = next_txbd(txbdp, base, tx_queue->tx_ring_size);

			length = skb_shinfo(skb)->frags[i].size;

			lstatus = txbdp->lstatus | length |
				BD_LFLAG(TXBD_READY);

			/* Handle the last BD specially */
			if (i == nr_frags - 1)
				lstatus |= BD_LFLAG(TXBD_LAST | TXBD_INTERRUPT);

			bufaddr = dma_map_page(&priv->ofdev->dev,
					skb_shinfo(skb)->frags[i].page,
					skb_shinfo(skb)->frags[i].page_offset,
					length,
					DMA_TO_DEVICE);

			/* set the TxBD length and buffer pointer */
			txbdp->bufPtr = bufaddr;
			txbdp->lstatus = lstatus;
		}

		lstatus = txbdp_start->lstatus;
	}

	/* Set up checksumming */
	if (CHECKSUM_PARTIAL == skb->ip_summed) {
		fcb = gfar_add_fcb(skb);
		lstatus |= BD_LFLAG(TXBD_TOE);
		gfar_tx_checksum(skb, fcb);
	}

	if (priv->vlgrp && vlan_tx_tag_present(skb)) {
		if (unlikely(NULL == fcb)) {
			fcb = gfar_add_fcb(skb);
			lstatus |= BD_LFLAG(TXBD_TOE);
		}

		gfar_tx_vlan(skb, fcb);
	}

	if (priv->ptimer_present) {
		/* Enable ptp flag so that Tx time stamping happens */
		if (gfar_ptp_do_txstamp(skb)) {
			if (fcb == NULL)
				fcb = gfar_add_fcb(skb);
			fcb->ptp = 0x01;
			lstatus |= BD_LFLAG(TXBD_TOE);
		}
	}

	/* setup the TxBD length and buffer pointer for the first BD */
	tx_queue->tx_skbuff[tx_queue->skb_curtx] = skb;
	txbdp_start->bufPtr = dma_map_single(&priv->ofdev->dev, skb->data,
			skb_headlen(skb), DMA_TO_DEVICE);

	lstatus |= BD_LFLAG(TXBD_CRC | TXBD_READY) | skb_headlen(skb);

	/*
	 * The powerpc-specific eieio() is used, as wmb() has too strong
	 * semantics (it requires synchronization between cacheable and
	 * uncacheable mappings, which eieio doesn't provide and which we
	 * don't need), thus requiring a more expensive sync instruction.  At
	 * some point, the set of architecture-independent barrier functions
	 * should be expanded to include weaker barriers.
	 */
	eieio();

	txbdp_start->lstatus = lstatus;

	/* Update the current skb pointer to the next entry we will use
	 * (wrapping if necessary) */
	tx_queue->skb_curtx = (tx_queue->skb_curtx + 1) &
		TX_RING_MOD_MASK(tx_queue->tx_ring_size);

	tx_queue->cur_tx = next_txbd(txbdp, base, tx_queue->tx_ring_size);

	/* reduce TxBD free count */
	tx_queue->num_txbdfree -= (nr_frags + 1);

	txq->trans_start = jiffies;

	/* If the next BD still needs to be cleaned up, then the bds
	   are full.  We need to tell the kernel to stop sending us stuff. */
	if (!tx_queue->num_txbdfree) {
		netif_stop_subqueue(dev, tx_queue->qindex);

		dev->stats.tx_fifo_errors++;
	}

	/* Tell the DMA to go go go */
	gfar_write(&regs->tstat, TSTAT_CLEAR_THALT >> tx_queue->qindex);

	/* Unlock priv */
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (!priv->sps)
#endif
		spin_unlock_irqrestore(&tx_queue->txlock, flags);

	return NETDEV_TX_OK;
}

/* Stops the kernel queue, and halts the controller */
static int gfar_close(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);

	disable_napi(priv);

	cancel_work_sync(&priv->reset_task);
	stop_gfar(dev);

	/* Disconnect from the PHY */
	phy_disconnect(priv->phydev);
	priv->phydev = NULL;

	netif_tx_stop_all_queues(dev);

	return 0;
}

/* Changes the mac address if the controller is not running. */
static int gfar_set_mac_address(struct net_device *dev)
{
	gfar_set_mac_for_addr(dev, 0, dev->dev_addr);

	return 0;
}

/**********************************************************************
 * gfar_accept_fastpath
 *
 * Used to authenticate to the kernel that a fast path entry can be
 * added to device's routing table cache
 *
 * Input : pointer to ethernet interface network device structure and
 *         a pointer to the designated entry to be added to the cache.
 * Output : zero upon success, negative upon failure
 **********************************************************************/
#ifdef CONFIG_NET_GIANFAR_FP
static int gfar_accept_fastpath(struct net_device *dev, struct dst_entry *dst)
{
	struct net_device *odev = dst->dev;
	const struct net_device_ops *ops = odev->netdev_ops;

	if ((dst->ops->protocol != __constant_htons(ETH_P_IP))
			|| (odev->type != ARPHRD_ETHER)
			|| (ops->ndo_accept_fastpath == NULL))
		return -1;

	return 0;
}

static inline int neigh_is_valid(struct neighbour *neigh)
{
	return neigh->nud_state & NUD_VALID;
}


u32 gfar_fastroute_hash(u8 daddr, u8 saddr)
{
	u32 hash;

	hash = ((u32)daddr ^ saddr) & NETDEV_FASTROUTE_HMASK;

	return hash;
}
#endif


/* try_fastroute() -- Checks the fastroute cache to see if a given packet
 *   can be routed immediately to another device.  If it can, we send it.
 *   If we used a fastroute, we return 1.  Otherwise, we return 0.
 *   Returns 0 if CONFIG_NET_GIANFAR_FP is not on
 */
static inline int try_fastroute(struct sk_buff *skb,
				struct net_device *dev, int length)
{
#ifdef CONFIG_NET_GIANFAR_FP
	struct ethhdr *eth;
	struct iphdr *iph;
	unsigned int hash;
	struct rtable *rt;
	struct net_device *odev;
	struct gfar_private *priv = netdev_priv(dev);
	struct netdev_queue *txq = NULL;
	const struct net_device_ops *ops;
	u16 q_idx = 0;

	/* this is correct. pull padding already */
	eth = (struct ethhdr *) (skb->data);

	/* Only route ethernet IP packets */
	if (eth->h_proto != __constant_htons(ETH_P_IP))
		return 0;

	iph = (struct iphdr *)(skb->data + ETH_HLEN);

	/* Generate the hash value */
	hash = gfar_fastroute_hash((*(u8 *)&iph->daddr),
				   (*(u8 *)&iph->saddr));

#ifdef FASTPATH_DEBUG
	printk(KERN_INFO "%s:  hash = %d (%d, %d)\n",
	       __func__, hash, (*(u8 *)&iph->daddr), (*(u8 *)&iph->saddr));
#endif
	rt = (struct rtable *) (dev->fastpath[hash]);
	/* Only support big endian */
	if ((rt != NULL)
	    && ((*(u32 *)(&iph->daddr))	== (*(u32 *)(&rt->rt_dst)))
	    && ((*(u32 *)(&iph->saddr))	== (*(u32 *)(&rt->rt_src)))
	    && !(rt->u.dst.obsolete)) {
		odev = rt->u.dst.dev;  /* get output device */
		ops = odev->netdev_ops;

		/* Make sure the packet is:
		 * 1) IPv4
		 * 2) without any options (header length of 5)
		 * 3) Not a multicast packet
		 * 4) going to a valid destination
		 * 5) Not out of time-to-live
		 */
		if (iph->version == 4
		    && iph->ihl == 5
		    && (!(eth->h_dest[0] & 0x01))
		    && neigh_is_valid(rt->u.dst.neighbour)
		    && iph->ttl > 1) {

			q_idx = skb_get_queue_mapping(skb);
			txq = netdev_get_tx_queue(odev, q_idx);
			/* Fast Route Path: Taken if the outgoing
			 * device is ready to transmit the packet now */
			if ((!netif_tx_queue_stopped(txq))
			    && (!spin_is_locked(&txq->_xmit_lock))
			    && (skb->len <= (odev->mtu + ETH_HLEN + 2 + 4))) {
				skb->pkt_type = PACKET_FASTROUTE;
				skb->protocol = __constant_htons(ETH_P_IP);
				skb_set_network_header(skb, ETH_HLEN);
				ip_decrease_ttl(iph);

				memcpy(eth->h_source, odev->dev_addr,
				       MAC_ADDR_LEN);
				memcpy(eth->h_dest, rt->u.dst.neighbour->ha,
				       MAC_ADDR_LEN);
				skb->dev = odev;
				if (ops->ndo_start_xmit(skb, odev) != 0) {
					panic("%s: FastRoute path corrupted",
					      dev->name);
				}
				priv->extra_stats.rx_fast++;
			}
			/* Semi Fast Route Path: Mark the packet as needing
			 * fast routing, but let the stack handle getting it
			 * to the device */
			else {
				skb->pkt_type = PACKET_FASTROUTE;
				skb_reset_network_header(skb);
				/* Tell the skb what kind of packet this is*/
				skb->protocol = eth_type_trans(skb, dev);
				/* Prep the skb for the packet */
				if (netif_receive_skb(skb) == NET_RX_DROP)
					priv->extra_stats.kernel_dropped++;
			}
			return 1;
		}
	}
#endif /* CONFIG_NET_GIANFAR_FP */
	return 0;
}

/* Enables and disables VLAN insertion/extraction */
static void gfar_vlan_rx_register(struct net_device *dev,
		struct vlan_group *grp)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = NULL;
	unsigned long flags;
	u32 tempval;

	regs = priv->gfargrp[0].regs;
	local_irq_save(flags);
	lock_rx_qs(priv);

	priv->vlgrp = grp;

	if (grp) {
		/* Enable VLAN tag insertion */
		tempval = gfar_read(&regs->tctrl);
		tempval |= TCTRL_VLINS;

		gfar_write(&regs->tctrl, tempval);

		/* Enable VLAN tag extraction */
		tempval = gfar_read(&regs->rctrl);
		tempval |= (RCTRL_VLEX | RCTRL_PRSDEP_INIT);
		gfar_write(&regs->rctrl, tempval);
	} else {
		/* Disable VLAN tag insertion */
		tempval = gfar_read(&regs->tctrl);
		tempval &= ~TCTRL_VLINS;
		gfar_write(&regs->tctrl, tempval);

		/* Disable VLAN tag extraction */
		tempval = gfar_read(&regs->rctrl);
		tempval &= ~RCTRL_VLEX;
		/* If parse is no longer required, then disable parser */
		if (tempval & RCTRL_REQ_PARSER)
			tempval |= RCTRL_PRSDEP_INIT;
		else
			tempval &= ~RCTRL_PRSDEP_INIT;
		gfar_write(&regs->rctrl, tempval);
	}

	gfar_change_mtu(dev, dev->mtu);

	unlock_rx_qs(priv);
	local_irq_restore(flags);
}

static int gfar_change_mtu(struct net_device *dev, int new_mtu)
{
	int tempsize, tempval;
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	int oldsize = priv->rx_buffer_size;
	int frame_size = new_mtu + ETH_HLEN;

#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (rcv_pkt_steering && priv->sps) {
		printk(KERN_ERR "Can't change mtu with rcv_pkt_steering on\n");
		return -EINVAL;
	}
#endif

	if (priv->vlgrp)
		frame_size += VLAN_HLEN;

	if ((frame_size < 64) || (frame_size > JUMBO_FRAME_SIZE)) {
		if (netif_msg_drv(priv))
			printk(KERN_ERR "%s: Invalid MTU setting\n",
					dev->name);
		return -EINVAL;
	}

	if (gfar_uses_fcb(priv))
		frame_size += GMAC_FCB_LEN;

	frame_size += priv->padding;

	tempsize =
	    (frame_size & ~(INCREMENTAL_BUFFER_SIZE - 1)) +
	    INCREMENTAL_BUFFER_SIZE;

	/* Only stop and start the controller if it isn't already
	 * stopped, and we changed something */
	if ((oldsize != tempsize) && (dev->flags & IFF_UP))
		stop_gfar(dev);

	priv->rx_buffer_size = tempsize;

	dev->mtu = new_mtu;

	gfar_write(&regs->mrblr, priv->rx_buffer_size);
	gfar_write(&regs->maxfrm, priv->rx_buffer_size);

	/* If the mtu is larger than the max size for standard
	 * ethernet frames (ie, a jumbo frame), then set maccfg2
	 * to allow huge frames, and to check the length */
	tempval = gfar_read(&regs->maccfg2);

	if (priv->rx_buffer_size > DEFAULT_RX_BUFFER_SIZE)
		tempval |= (MACCFG2_HUGEFRAME | MACCFG2_LENGTHCHECK);
	else
		tempval &= ~(MACCFG2_HUGEFRAME | MACCFG2_LENGTHCHECK);

	gfar_write(&regs->maccfg2, tempval);

	if ((oldsize != tempsize) && (dev->flags & IFF_UP))
		startup_gfar(dev);

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	gfar_skbr_register_truesize(priv);
#endif /*CONFIG_GFAR_SKBUFF_RECYCLING*/

	return 0;
}

/* gfar_reset_task gets scheduled when a packet has not been
 * transmitted after a set amount of time.
 * For now, assume that clearing out all the structures, and
 * starting over will fix the problem.
 */
static void gfar_reset_task(struct work_struct *work)
{
	struct gfar_private *priv = container_of(work, struct gfar_private,
			reset_task);
	struct net_device *dev = priv->ndev;

	if (dev->flags & IFF_UP) {
		netif_tx_stop_all_queues(dev);
		stop_gfar(dev);
		startup_gfar(dev);
		netif_tx_start_all_queues(dev);
	}

	netif_tx_schedule_all(dev);
}

static void gfar_timeout(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);

	dev->stats.tx_errors++;
	schedule_work(&priv->reset_task);
}

/* Interrupt Handler for Transmit complete */
#ifdef CONFIG_GIANFAR_TXNAPI
static int gfar_clean_tx_ring(struct gfar_priv_tx_q *tx_queue, int tx_work_limit)
#else
static int gfar_clean_tx_ring(struct gfar_priv_tx_q *tx_queue)
#endif
{
	struct net_device *dev = tx_queue->dev;
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar_priv_rx_q *rx_queue = NULL;
	struct txbd8 *bdp;
	struct txbd8 *lbdp = NULL;
	struct txbd8 *base = tx_queue->tx_bd_base;
	struct sk_buff *skb;
	int skb_dirtytx;
	int tx_ring_size = tx_queue->tx_ring_size;
	int frags = 0;
	int i;
	int howmany = 0;
	u32 lstatus;
#ifdef CONFIG_TCP_FAST_ACK
	unsigned long flags = 0;
#endif

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	int howmany_recycle = 0;
#endif

	rx_queue = priv->rx_queue[tx_queue->qindex];
	bdp = tx_queue->dirty_tx;
	skb_dirtytx = tx_queue->skb_dirtytx;

	while ((skb = tx_queue->tx_skbuff[skb_dirtytx])) {
		frags = skb_shinfo(skb)->nr_frags;
		lbdp = skip_txbd(bdp, frags, base, tx_ring_size);

		lstatus = lbdp->lstatus;

		/* Only clean completed frames */
		if ((lstatus & BD_LFLAG(TXBD_READY)) &&
				(lstatus & BD_LENGTH_MASK))
			break;

		dma_unmap_single(&priv->ofdev->dev,
				bdp->bufPtr,
				bdp->length,
				DMA_TO_DEVICE);

		bdp->lstatus &= BD_LFLAG(TXBD_WRAP);
		bdp = next_txbd(bdp, base, tx_ring_size);

		for (i = 0; i < frags; i++) {
			dma_unmap_page(&priv->ofdev->dev,
					bdp->bufPtr,
					bdp->length,
					DMA_TO_DEVICE);
			bdp->lstatus &= BD_LFLAG(TXBD_WRAP);
			bdp = next_txbd(bdp, base, tx_ring_size);
		}


#ifdef CONFIG_TCP_FAST_ACK
		if (skb->sk && skb->sk->sk_state == TCP_ESTABLISHED &&
		skb->truesize == SKB_DATA_ALIGN(MAX_TCP_HEADER) + sizeof(struct sk_buff) &&
		TCP_SKB_CB(skb)->flags == TCPCB_FLAG_ACK &&
		skb_queue_len(&skb->sk->sk_ack_queue) < GFAR_DEFAULT_RECYCLE_MAX &&
		spin_trylock_irqsave(&skb->sk->sk_ack_queue.lock, flags)) {
			void (*destructor)(struct sk_buff *);
			destructor = skb->destructor;
			skb->destructor = NULL;
			__skb_queue_head(&skb->sk->sk_ack_queue, skb);
			spin_unlock_irqrestore(&skb->sk->sk_ack_queue.lock, flags);
			if (destructor)
				destructor(skb);
		} else
#endif
		{
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
			howmany_recycle += gfar_kfree_skb(skb, tx_queue->qindex);
#else
			dev_kfree_skb_any(skb);
#endif
		}
		tx_queue->tx_skbuff[skb_dirtytx] = NULL;

		skb_dirtytx = (skb_dirtytx + 1) &
			TX_RING_MOD_MASK(tx_ring_size);

		howmany++;
		tx_queue->num_txbdfree += frags + 1;
	}

	/* If we freed a buffer, we can restart transmission, if necessary */
	if (__netif_subqueue_stopped(dev, tx_queue->qindex) && tx_queue->num_txbdfree)
		netif_wake_subqueue(dev, tx_queue->qindex);

	/* Update dirty indicators */
	tx_queue->skb_dirtytx = skb_dirtytx;
	tx_queue->dirty_tx = bdp;

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	priv->extra_stats.rx_skbr_free += howmany_recycle;
#endif
	return howmany;
}

#ifdef CONFIG_GIANFAR_TXNAPI
static void gfar_schedule_cleanup_rx(struct gfar_priv_grp *gfargrp)
{
	unsigned long flags;
	u32 imask = 0;

	spin_lock_irqsave(&gfargrp->grplock, flags);
	if (napi_schedule_prep(&gfargrp->napi_rx)) {
		imask = gfar_read(&gfargrp->regs->imask);
		imask = imask & IMASK_RX_DISABLED;
		gfar_write(&gfargrp->regs->imask, imask);
		__napi_schedule(&gfargrp->napi_rx);
	} else {
		gfar_write(&gfargrp->regs->ievent, IEVENT_RX_MASK);
	}
	spin_unlock_irqrestore(&gfargrp->grplock, flags);
}

static void gfar_schedule_cleanup_tx(struct gfar_priv_grp *gfargrp)
{
	unsigned long flags;
	u32 imask = 0;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int cpu = smp_processor_id();
#endif

	spin_lock_irqsave(&gfargrp->grplock, flags);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (napi_schedule_prep(&gfargrp->napi_tx[cpu])) {
#else
	if (napi_schedule_prep(&gfargrp->napi_tx)) {
#endif
		imask = gfar_read(&gfargrp->regs->imask);
		imask = imask & IMASK_TX_DISABLED;
		gfar_write(&gfargrp->regs->imask, imask);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		__napi_schedule(&gfargrp->napi_tx[cpu]);
#else
		__napi_schedule(&gfargrp->napi_tx);
#endif
	} else {
		gfar_write(&gfargrp->regs->ievent, IEVENT_TX_MASK);
	}
	spin_unlock_irqrestore(&gfargrp->grplock, flags);
}
#else
static void gfar_schedule_cleanup(struct gfar_priv_grp *gfargrp)
{
	unsigned long flags;

	spin_lock_irqsave(&gfargrp->grplock, flags);
	if (napi_schedule_prep(&gfargrp->napi)) {
		gfar_write(&gfargrp->regs->imask, IMASK_RTX_DISABLED);
		__napi_schedule(&gfargrp->napi);
	} else {
		/*
		 * Clear IEVENT, so interrupts aren't called again
		 * because of the packets that have already arrived.
		 */
		gfar_write(&gfargrp->regs->ievent, IEVENT_RTX_MASK);
	}
	spin_unlock_irqrestore(&gfargrp->grplock, flags);

}
#endif

/* Interrupt Handler for Transmit complete */
static irqreturn_t gfar_transmit(int irq, void *grp_id)
{
#ifdef CONFIG_GIANFAR_TXNAPI
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	struct gfar_priv_grp *grp = (struct gfar_priv_grp *)grp_id;
	struct gfar_private *priv = grp->priv;
	unsigned int tstat  = gfar_read(&grp->regs->tstat);
	int cpu = smp_processor_id();
	unsigned long flags;

	if (priv->sps) {
		spin_lock_irqsave(&grp->grplock, flags);
		if (tstat & (0x8000 >> !cpu))
			fsl_send_msg(grp->msg_virtual_tx[!cpu], 0x1);

		if (tstat & (0x8000 >> cpu))
			if (napi_schedule_prep(&grp->napi_tx[cpu]))
				__napi_schedule(&grp->napi_tx[cpu]);

		gfar_write(&grp->regs->ievent, IEVENT_TX_MASK);

		/* clear TXF0,TXF1 in TSTAT */
		gfar_write(&grp->regs->tstat, (tstat & 0xC000));

		spin_unlock_irqrestore(&grp->grplock, flags);
	} else {
#endif
		gfar_schedule_cleanup_tx((struct gfar_priv_grp *)grp_id);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	}
#endif
#else
#ifdef CONFIG_GFAR_TX_NONAPI
	struct gfar_priv_grp *grp = (struct gfar_priv_grp *)grp_id;
	struct gfar_private *priv = priv = grp->priv;
	unsigned int mask = TSTAT_TXF0_MASK;
	unsigned int tstat  = gfar_read(&grp->regs->tstat);
	int i;
	struct gfar_priv_tx_q *tx_queue = NULL;

	tstat = gfar_read(&grp->regs->tstat);
	tstat = tstat & TSTAT_TXF_MASK_ALL;
	/* Clear IEVENT */
	gfar_write(&grp->regs->ievent, IEVENT_TX_MASK);

	for (i = 0; i < priv->num_tx_queues; i++) {
		if (tstat & mask) {
			tx_queue = priv->tx_queue[i];
			spin_lock(&tx_queue->txlock);
			gfar_clean_tx_ring(tx_queue);
			spin_unlock(&tx_queue->txlock);
		}
		mask = mask >> 0x1;
	}

	gfar_configure_tx_coalescing(priv, grp->tx_bit_map);
#else
	gfar_schedule_cleanup((struct gfar_priv_grp *)grp_id);
#endif
#endif
	return IRQ_HANDLED;
}

static void gfar_new_rxbdp(struct gfar_priv_rx_q *rx_queue, struct rxbd8 *bdp,
		struct sk_buff *skb)
{
	struct net_device *dev = rx_queue->dev;
	struct gfar_private *priv = netdev_priv(dev);
	u32 lstatus;

	bdp->bufPtr = dma_map_single(&priv->ofdev->dev, skb->data,
			priv->rx_buffer_size, DMA_FROM_DEVICE);

	lstatus = BD_LFLAG(RXBD_EMPTY | RXBD_INTERRUPT);

	if (bdp == rx_queue->rx_bd_base + rx_queue->rx_ring_size - 1)
		lstatus |= BD_LFLAG(RXBD_WRAP);

	eieio();

	bdp->lstatus = lstatus;
}

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
static unsigned int skbuff_truesize(unsigned int buffer_size)
{
	return SKB_DATA_ALIGN(buffer_size + RXBUF_ALIGNMENT +
				NET_SKB_PAD) + sizeof(struct sk_buff);
}

static void gfar_skbr_register_truesize(struct gfar_private *priv)
{
	int i = 0;

	priv->skbuff_truesize = skbuff_truesize(priv->rx_buffer_size);
	for (i = 0; i < priv->num_rx_queues; i++)
		priv->rx_queue[i]->rx_skbuff_truesize =
				skbuff_truesize(priv->rx_buffer_size);
}

static inline void gfar_clean_reclaim_skb(struct sk_buff *skb)
{
	unsigned int truesize;
	unsigned int size;
	unsigned int alignamount;
	struct net_device *owner;

	dst_release((struct dst_entry *)skb->_skb_dst);
	skb->_skb_dst = 0;
	if (skb->destructor) {
		skb->destructor(skb);
		skb->destructor = NULL;
	}
#ifdef CONFIG_XFRM
	if (skb->sp) {
		secpath_put(skb->sp);
		skb->sp = NULL;
	}
#endif
#if defined(CONFIG_NF_CONNTRACK) || defined(CONFIG_NF_CONNTRACK_MODULE)
	nf_conntrack_put(skb->nfct);
	nf_conntrack_put_reasm(skb->nfct_reasm);
	skb->nfct = NULL;
	skb->nfct_reasm = NULL;
#endif
#ifdef CONFIG_BRIDGE_NETFILTER
	nf_bridge_put(skb->nf_bridge);
	skb->nf_bridge = NULL;
#endif
#ifdef CONFIG_NET_SCHED
	skb->tc_index = 0;
#ifdef CONFIG_NET_CLS_ACT
	skb->tc_verd = 0;
#endif
#endif
	/* re-initialization
	 * We are not going to touch the buffer size, so
	 * skb->truesize can be used as the truesize again
	 */
	if (skb_shinfo(skb)->nr_frags) {
		int i;
		for (i = 0; i < skb_shinfo(skb)->nr_frags; i++)
			put_page(skb_shinfo(skb)->frags[i].page);
		skb_shinfo(skb)->nr_frags = 0;
	}
	owner = skb->skb_owner;
	truesize = skb->truesize;
	size = truesize - sizeof(struct sk_buff);
	/* clear structure by &tail */
	cacheable_memzero(skb, offsetof(struct sk_buff, tail));
	atomic_set(&skb->users, 1);
	/* reset data and tail pointers */
	skb->data = skb->head + NET_SKB_PAD;
	skb_reset_tail_pointer(skb);
	/* shared info clean up */
	atomic_set(&(skb_shinfo(skb)->dataref), 1);
	/* We need the data buffer to be aligned properly.  We will
	 * reserve as many bytes as needed to align the data properly
	 */
	alignamount = ((unsigned)skb->data) & (RXBUF_ALIGNMENT-1);
	skb_reserve(skb, RXBUF_ALIGNMENT - alignamount);
	skb->dev = owner;
	/* Keep incoming device pointer for recycling */
	skb->skb_owner = owner;

}

static int gfar_kfree_skb(struct sk_buff *skb, int qindex)
{
	struct gfar_private *priv;
	struct gfar_skb_handler *sh;

	if ((skb->skb_owner == NULL) ||
		skb_has_frags(skb) ||
		skb_cloned(skb) ||
		skb_header_cloned(skb) ||
		(atomic_read(&skb->users) > 1))
			goto _normal_free;

	priv = netdev_priv(skb->skb_owner);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps)
		qindex = 0;
#endif
	if (skb->truesize == priv->skbuff_truesize) {
		unsigned long flags = 0;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		if (rcv_pkt_steering && priv->sps)
			sh = &__get_cpu_var(gfar_cpu_dev).sh;
		else
#endif
		sh = per_cpu_ptr(priv->rx_queue[qindex]->local_sh,
						smp_processor_id());

		/* loosly checking */
		if (likely(sh->recycle_count < sh->recycle_max)) {
			gfar_clean_reclaim_skb(skb);
			skb->next = sh->recycle_queue;
			sh->recycle_queue = skb;
			sh->recycle_count++;
			return 1;
		} else {
			sh = &priv->skb_handler;
			if (likely(sh->recycle_count < sh->recycle_max)) {
				gfar_clean_reclaim_skb(skb);
				/* lock sh for add one */
				spin_lock_irqsave(&sh->lock, flags);
				if (unlikely(!sh->recycle_enable)) {
					spin_unlock_irqrestore(&sh->lock, flags);
					return 0;
				}
				skb->next = sh->recycle_queue;
				sh->recycle_queue = skb;
				sh->recycle_count++;
				spin_unlock_irqrestore(&sh->lock, flags);
				return 1;
			}
		}
	}
_normal_free:
	/* skb is not recyclable */
	dev_kfree_skb_any(skb);
	return 0;
}

int gfar_recycle_skb(struct sk_buff *skb)
{
	unsigned long int flags;
	struct gfar_private *priv;
	struct gfar_skb_handler *sh;

	if ((skb->skb_owner == NULL) ||
		skb_has_frags(skb) ||
		skb_cloned(skb) ||
		skb_header_cloned(skb) ||
		(atomic_read(&skb->users) > 1))
		return 0;
	priv = netdev_priv(skb->skb_owner);
	if (skb->truesize == priv->skbuff_truesize) {
		sh = &priv->skb_handler;
		/* loosly checking */
		if (likely(sh->recycle_count < sh->recycle_max)) {
			gfar_clean_reclaim_skb(skb);
			/* lock sh for add one */
			spin_lock_irqsave(&sh->lock, flags);
			if (unlikely(!sh->recycle_enable)) {
				spin_unlock_irqrestore(&sh->lock, flags);
				return 0;
			}
			skb->next = sh->recycle_queue;
			sh->recycle_queue = skb;
			sh->recycle_count++;
			spin_unlock_irqrestore(&sh->lock, flags);
			priv->extra_stats.rx_skbr_free++;
			return 1;
		}
	}
	/* skb is not recyclable */
	return 0;
}

#endif /* RECYCLING */

/*
 * normal new skb routine
 */
struct sk_buff * gfar_new_skb(struct net_device *dev)
{
	unsigned int alignamount;
	struct gfar_private *priv = netdev_priv(dev);
	struct sk_buff *skb = NULL;

	skb = netdev_alloc_skb(dev, priv->rx_buffer_size + RXBUF_ALIGNMENT);

	if (!skb)
		return NULL;

	alignamount = RXBUF_ALIGNMENT -
		(((unsigned long) skb->data) & (RXBUF_ALIGNMENT - 1));

	/* We need the data buffer to be aligned properly.  We will reserve
	 * as many bytes as needed to align the data properly
	 */
	skb_reserve(skb, alignamount);

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	skb->dev = dev;
#endif
	/* Keep incoming device pointer for recycling */
	skb->skb_owner = dev;

	return skb;
}

static inline void count_errors(unsigned short status, struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct gfar_extra_stats *estats = &priv->extra_stats;

	/* If the packet was truncated, none of the other errors
	 * matter */
	if (status & RXBD_TRUNCATED) {
		stats->rx_length_errors++;

		estats->rx_trunc++;

		return;
	}
	/* Count the errors, if there were any */
	if (status & (RXBD_LARGE | RXBD_SHORT)) {
		stats->rx_length_errors++;

		if (status & RXBD_LARGE)
			estats->rx_large++;
		else
			estats->rx_short++;
	}
	if (status & RXBD_NONOCTET) {
		stats->rx_frame_errors++;
		estats->rx_nonoctet++;
	}
	if (status & RXBD_CRCERR) {
		estats->rx_crcerr++;
		stats->rx_crc_errors++;
	}
	if (status & RXBD_OVERRUN) {
		estats->rx_overrun++;
		stats->rx_crc_errors++;
	}
}

static inline unsigned long __wk_phy_to_virt(struct net_device *dev,
				unsigned long phy)
{
	struct gfar_private *priv = netdev_priv(dev);
	unsigned long virt, offset;

	offset = phy - priv->wk_buf_align_paddr;
	virt = priv->wk_buf_align_vaddr + offset;
	return virt;
}

static void gfar_receive_wakeup(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar_priv_rx_q *rx_queue = priv->rx_queue[priv->num_rx_queues-1];
	struct rxbd8 *bdp = rx_queue->cur_rx;
	struct sk_buff *skb;
	unsigned char *data;
	u16 len;
	int ret;

	/* get the first full descriptor */
	while (!(bdp->status & RXBD_EMPTY)) {
		rmb();
		if (bdp->status & RXBD_ERR) {
			printk(KERN_ERR "Wake up packet error!\n");
			goto out;
		}

		data = (u8 *)__wk_phy_to_virt(dev, bdp->bufPtr);
		len = bdp->length;
		/* allocate the skb */
		skb = netdev_alloc_skb(dev, len);
		if (!skb) {
			dev->stats.rx_dropped++;
			priv->extra_stats.rx_skbmissing++;
			goto out;
		}
		/* The wake up packet has the FCB */
		data += (GMAC_FCB_LEN + priv->padding);
		len -= (GMAC_FCB_LEN + priv->padding);
		/* remove the FCS from the packet length */
		len -= 4;
		/* copy received packet to skb buffer */
		memcpy(skb->data, data, len);
		/* Prep the skb for the packet */
		skb_put(skb, len);
		/* Tell the skb what kind of packet this is */
		skb->protocol = eth_type_trans(skb, dev);

		ret = netif_rx(skb);
		if (NET_RX_DROP == ret) {
			priv->extra_stats.kernel_dropped++;
		} else {
			/* Increment the number of packets */
			dev->stats.rx_packets++;
			dev->stats.rx_bytes += len;
		}

out:
		bdp->status &= RXBD_CLEAN;
		bdp->status |= RXBD_EMPTY;
		bdp->length = 0;

		mb();
		/* Update to the next pointer */
		if (bdp->status & RXBD_WRAP)
			bdp = priv->wk_bd_base;
		else
			bdp++;

	}
	rx_queue->cur_rx = bdp;
}

irqreturn_t gfar_receive(int irq, void *grp_id)
{
	struct gfar_priv_grp *gfargrp = grp_id;
	struct gfar __iomem *regs = gfargrp->regs;
	struct gfar_private *priv = gfargrp->priv;
	struct net_device *dev = priv->ndev;
	u32 ievent;

	ievent = gfar_read(&regs->ievent);

	if ((ievent & IEVENT_FGPI) == IEVENT_FGPI) {
		priv->should_wake = 1;
		gfar_write(&regs->ievent, ievent & IEVENT_RX_MASK);
		gfar_receive_wakeup(dev);
		return IRQ_HANDLED;
	}

#ifdef CONFIG_GIANFAR_TXNAPI
	gfar_schedule_cleanup_rx((struct gfar_priv_grp *)grp_id);
#else
#ifdef CONFIG_GFAR_TX_NONAPI
	u32 tempval;

	/*
	 * Clear IEVENT, so interrupts aren't called again
	 * because of the packets that have already arrived.
	 */
	gfar_write(&regs->ievent, IEVENT_RX_MASK);

	if (napi_schedule_prep(&gfargrp->napi)) {
		tempval = gfar_read(&regs->imask);
		tempval &= IMASK_RX_DISABLED;
		gfar_write(&regs->imask, tempval);
		__napi_schedule(&gfargrp->napi);
	} else {
		if (netif_msg_rx_err(priv))
			printk(KERN_DEBUG "%s: receive called twice (%x)[%x]\n",
				dev->name, gfar_read(&regs->ievent),
				gfar_read(&regs->imask));
	}

#else
	gfar_schedule_cleanup((struct gfar_priv_grp *)grp_id);
#endif
#endif
	return IRQ_HANDLED;
}

/* gfar_process_frame() -- handle one incoming packet if skb
 * isn't NULL.  */
static int gfar_process_frame(struct net_device *dev, struct sk_buff *skb,
			      int amount_pull)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct rxfcb *fcb = NULL;

	int ret;

	/* fcb is at the beginning if exists */
	fcb = (struct rxfcb *)skb->data;

	/* Remove the FCB from the skb */
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps)
		skb_set_queue_mapping(skb, smp_processor_id());
	else
#endif
		skb_set_queue_mapping(skb, fcb->rq);

	/* Remove the padded bytes, if there are any */
	if (amount_pull)
		skb_pull(skb, amount_pull);

	if (priv->ptimer_present) {
		gfar_ptp_store_rxstamp(dev, skb);
		skb_pull(skb, 8);
	}

	if (priv->rx_csum_enable)
		gfar_rx_checksum(skb, fcb);

#ifdef CONFIG_NET_GIANFAR_FP
	if (netdev_fastroute && (try_fastroute(skb, dev, skb->len) != 0))
		return 0;
#endif
	/* Tell the skb what kind of packet this is */
	skb->protocol = eth_type_trans(skb, dev);

	/* Send the packet up the stack */
	if (unlikely(priv->vlgrp && (fcb->flags & RXFCB_VLN)))
		ret = vlan_hwaccel_receive_skb(skb, priv->vlgrp, fcb->vlctl);
	else
		ret = netif_receive_skb(skb);

	if (NET_RX_DROP == ret)
		priv->extra_stats.kernel_dropped++;

	return 0;
}

/* gfar_clean_rx_ring() -- Processes each frame in the rx ring
 *   until the budget/quota has been reached. Returns the number
 *   of frames handled
 */
int gfar_clean_rx_ring(struct gfar_priv_rx_q *rx_queue, int rx_work_limit)
{
	struct net_device *dev = rx_queue->dev;
	struct rxbd8 *bdp, *base;
	struct sk_buff *skb;
	int pkt_len;
	int amount_pull;
	int howmany = 0;
	struct gfar_private *priv = netdev_priv(dev);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int ret;
#endif
#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
	int howmany_reuse = 0;
	struct gfar_skb_handler *sh;
	int free_skb;
	struct sk_buff *local_head;
	unsigned long flags;
	struct gfar_skb_handler *local_sh;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	struct sk_buff *local_tail;
	int temp;
#endif
#endif

	/* Get the first full descriptor */
	bdp = rx_queue->cur_rx;
	base = rx_queue->rx_bd_base;

	if (priv->ptimer_present)
		amount_pull = (gfar_uses_fcb(priv) ? GMAC_FCB_LEN : 0);
	else
		amount_pull = (gfar_uses_fcb(priv) ? GMAC_FCB_LEN : 0) +
				priv->padding;

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (rcv_pkt_steering && priv->sps) {
		local_head = NULL;
		free_skb = 0;
		sh = &__get_cpu_var(gfar_cpu_dev).sh;
	} else {
#endif
		local_sh = per_cpu_ptr(rx_queue->local_sh, smp_processor_id());
		if (local_sh->recycle_queue) {
			local_head = local_sh->recycle_queue;
			free_skb = local_sh->recycle_count;
			local_sh->recycle_queue = NULL;
			local_sh->recycle_count = 0;
		} else {
			local_head = NULL;
			free_skb = 0;
		}
		/* global skb_handler for this device */
		sh = &rx_queue->skb_handler;
		if (sh->recycle_count == 0 &&
			priv->skb_handler.recycle_count > 0)
			sh = &priv->skb_handler;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	}
#endif
#endif

	while (!((bdp->status & RXBD_EMPTY) || (--rx_work_limit < 0))) {
		struct sk_buff *newskb;
		rmb();

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
		if (!free_skb && sh->recycle_count) {
			/* refill local buffer */
#ifdef CONFIG_GFAR_SW_PKT_STEERING
			if (!(rcv_pkt_steering && priv->sps))
#endif
				spin_lock_irqsave(&sh->lock, flags);
			local_head = sh->recycle_queue;
			free_skb = sh->recycle_count;
			sh->recycle_queue = NULL;
			sh->recycle_count = 0;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
			if (!(rcv_pkt_steering && priv->sps))
#endif
				spin_unlock_irqrestore(&sh->lock, flags);
		}
		if (local_head) {
			newskb = local_head;
			local_head = newskb->next;
			newskb->next = NULL;
			free_skb--;
			howmany_reuse++;
		} else
			newskb = gfar_new_skb(dev);
#else
		/* Add another skb for the future */
		newskb = gfar_new_skb(dev);
#endif

		skb = rx_queue->rx_skbuff[rx_queue->skb_currx];

		dma_unmap_single(&priv->ofdev->dev, bdp->bufPtr,
				priv->rx_buffer_size, DMA_FROM_DEVICE);

		/* We drop the frame if we failed to allocate a new buffer */
		if (unlikely(!newskb || !(bdp->status & RXBD_LAST) ||
				 bdp->status & RXBD_ERR)) {
			count_errors(bdp->status, dev);

			if (unlikely(!newskb))
				newskb = skb;
			else if (skb)
				dev_kfree_skb_any(skb);
		} else {
			/* Increment the number of packets */
			rx_queue->stats.rx_packets++;
			howmany++;

			if (likely(skb)) {
				pkt_len = bdp->length - ETH_FCS_LEN;
				/* Remove the FCS from the packet length */
				skb_put(skb, pkt_len);
				rx_queue->stats.rx_bytes += pkt_len;

				if (in_irq() || irqs_disabled())
					printk("Interrupt problem!\n");
#ifdef CONFIG_GFAR_SW_PKT_STEERING
				/* Process packet here or send it to other cpu
				   for processing based on packet headers
				   hash value
				 */
				if (rcv_pkt_steering && priv->sps) {
					ret = distribute_packet(dev,
							skb, amount_pull);
					if (ret)
						gfar_process_frame(dev,
							skb, amount_pull);
				} else {
					gfar_process_frame(dev,
						skb, amount_pull);
				}
#else
#ifdef CONFIG_GFAR_HW_TCP_RECEIVE_OFFLOAD
				if ((rx_queue->qindex >= TCP_CHL_OFFSET) &&
					priv->tcp_hw_channel[rx_queue->qindex - TCP_CHL_OFFSET]) {
					gfar_hwaccel_tcp4_receive(priv, rx_queue, skb, amount_pull);
				} else
#endif
					gfar_process_frame(dev, skb, amount_pull);
#endif
			} else {
				if (netif_msg_rx_err(priv))
					printk(KERN_WARNING
					       "%s: Missing skb!\n", dev->name);
				rx_queue->stats.rx_dropped++;
				priv->extra_stats.rx_skbmissing++;
			}

		}

		rx_queue->rx_skbuff[rx_queue->skb_currx] = newskb;

		/* Setup the new bdp */
		gfar_new_rxbdp(rx_queue, bdp, newskb);

		/* Update to the next pointer */
		bdp = next_bd(bdp, base, rx_queue->rx_ring_size);

		/* update to point at the next skb */
		rx_queue->skb_currx =
		    (rx_queue->skb_currx + 1) &
		    RX_RING_MOD_MASK(rx_queue->rx_ring_size);
	}

#ifdef CONFIG_GFAR_SKBUFF_RECYCLING
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (rcv_pkt_steering && priv->sps) {
		if (free_skb > 0) {
			/* return left over skb to cpu's recycle buffer */
			if (sh->recycle_max >= (sh->recycle_count + free_skb)) {
				temp = free_skb - 1;
				local_tail = local_head;
				while (temp--)
					local_tail = local_tail->next;

				local_tail->next = sh->recycle_queue;
				sh->recycle_queue = local_head;
				sh->recycle_count += free_skb;
			} else {
				/* free the left over skbs if recycle buffer
				cant accomodate */
				temp = free_skb;
				while (temp--) {
					local_tail = local_head;
					local_head = local_head->next;
					if (local_tail)
						dev_kfree_skb_any(local_tail);
				}
			}
		}
	} else {
#endif
		if (free_skb) {
			/* return to local_sh for next time */
			local_sh->recycle_queue = local_head;
			local_sh->recycle_count = free_skb;
		}
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	}
#endif
	priv->extra_stats.rx_skbr += howmany_reuse;
#endif

	/* Update the current rxbd pointer to be the next one */
	rx_queue->cur_rx = bdp;

	return howmany;
}

#ifdef CONFIG_GIANFAR_TXNAPI
static int gfar_poll_tx(struct napi_struct *napi, int budget)
{
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	int cpu = smp_processor_id();
	struct gfar_priv_grp *gfargrp = container_of(napi,
					struct gfar_priv_grp, napi_tx[cpu]);
#else
	struct gfar_priv_grp *gfargrp = container_of(napi,
					struct gfar_priv_grp, napi_tx);
#endif
	struct gfar_private *priv = gfargrp->priv;
	struct gfar __iomem *regs = gfargrp->regs;
	struct gfar_priv_tx_q *tx_queue = NULL;
	int budget_per_queue = 0, tx_cleaned = 0, i = 0, num_act_qs = 0;
	int tx_cleaned_per_queue = 0, mask = TSTAT_TXF0_MASK;
	unsigned long flags;
	u32 imask, tstat, tstat_local;

#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps) {
		tx_queue = priv->tx_queue[cpu];
		tx_cleaned = gfar_clean_tx_ring(tx_queue, budget);
	} else {
#endif
		tstat = gfar_read(&regs->tstat);
		tstat = tstat & TSTAT_TXF_MASK_ALL;
		tstat_local = tstat;

		while (tstat_local) {
			num_act_qs++;
			tstat_local &= (tstat_local - 1);
		}

		budget_per_queue = budget/num_act_qs;

		gfar_write(&regs->ievent, IEVENT_TX_MASK);

		for_each_bit(i, &gfargrp->tx_bit_map, priv->num_tx_queues) {
			mask = mask >> i;
			if (tstat & mask) {
				tx_queue = priv->tx_queue[i];
				spin_lock_irqsave(&tx_queue->txlock, flags);
				tx_cleaned_per_queue =
						gfar_clean_tx_ring(tx_queue,
							budget_per_queue);
				spin_unlock_irqrestore(&tx_queue->txlock,
								flags);
				tx_cleaned += tx_cleaned_per_queue;
				tx_cleaned_per_queue = 0;
			}
			mask = TSTAT_TXF0_MASK;
		}

		budget = (num_act_qs * DEFAULT_TX_RING_SIZE) + 1;
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	}
#endif
	if (tx_cleaned < budget) {
		napi_complete(napi);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		if (!priv->sps) {
#endif
			spin_lock_irq(&gfargrp->grplock);
			gfar_write(&regs->tstat, tstat);
			imask = gfar_read(&regs->imask);
			imask |= IMASK_DEFAULT_TX;
			gfar_write(&regs->ievent, IEVENT_TX_MASK);
			gfar_write(&regs->imask, imask);
			spin_unlock_irq(&gfargrp->grplock);
			gfar_configure_tx_coalescing(priv, gfargrp->tx_bit_map);
#ifdef CONFIG_GFAR_SW_PKT_STEERING
		} else {
			gfar_write(&regs->ievent, IEVENT_TX_MASK);
		}
#endif
		return 1;
	}

#ifdef CONFIG_GFAR_SW_PKT_STEERING
	if (priv->sps)
		return 1;
	else
#endif
		return tx_cleaned;
}

static int gfar_poll_rx(struct napi_struct *napi, int budget)
{
	struct gfar_priv_grp *gfargrp = container_of(napi,
			struct gfar_priv_grp, napi_rx);
	struct gfar_private *priv = gfargrp->priv;
	struct gfar __iomem *regs = gfargrp->regs;
	struct gfar_priv_rx_q *rx_queue = NULL;
	int rx_cleaned = 0, budget_per_queue = 0, rx_cleaned_per_queue = 0;
	int num_act_qs = 0, mask = RSTAT_RXF0_MASK, i;
	u32 imask, rstat, rstat_local, rstat_rhalt = 0;

	rstat = gfar_read(&regs->rstat);
	rstat = rstat & RSTAT_RXF_ALL_MASK;
	rstat_local = rstat;

	while (rstat_local) {
		num_act_qs++;
		rstat_local &= (rstat_local - 1);
	}

	budget_per_queue = budget/num_act_qs;

	gfar_write(&regs->ievent, IEVENT_RX_MASK);

	for_each_bit(i, &gfargrp->rx_bit_map, priv->num_rx_queues) {
		mask = mask >> i;
		if (rstat & mask) {
			rstat_rhalt |= (RSTAT_CLEAR_RHALT >> i);
			rx_queue = priv->rx_queue[i];
			rx_cleaned_per_queue = gfar_clean_rx_ring(rx_queue,
							budget_per_queue);
			rx_cleaned += rx_cleaned_per_queue;
		}
		mask = RSTAT_RXF0_MASK;
	}

	if (rx_cleaned < budget) {
		napi_complete(napi);

		/* Clear the halt bit in RSTAT */
		spin_lock_irq(&gfargrp->grplock);
		gfar_write(&regs->rstat, rstat_rhalt);
		gfar_write(&regs->rstat, rstat);
		imask = gfar_read(&regs->imask);
		imask |= IMASK_DEFAULT_RX;
		gfar_write(&regs->ievent, IEVENT_RX_MASK);
		gfar_write(&regs->imask, imask);
		spin_unlock_irq(&gfargrp->grplock);

		gfar_configure_rx_coalescing(priv, gfargrp->rx_bit_map);
	}

	return rx_cleaned;
}
#else
static int gfar_poll(struct napi_struct *napi, int budget)
{
	struct gfar_priv_grp *gfargrp = container_of(napi,
			struct gfar_priv_grp, napi);
	struct gfar_private *priv = gfargrp->priv;
	struct gfar __iomem *regs = gfargrp->regs;
	struct gfar_priv_tx_q *tx_queue = NULL;
	struct gfar_priv_rx_q *rx_queue = NULL;
	int rx_cleaned = 0, budget_per_queue = 0, rx_cleaned_per_queue = 0;
	int tx_cleaned = 0, i, left_over_budget = budget;
	unsigned long serviced_queues = 0, flags;
	int num_queues = 0;

	num_queues = gfargrp->num_rx_queues;
	budget_per_queue = budget/num_queues;

	/* Clear IEVENT, so interrupts aren't called again
	 * because of the packets that have already arrived */
#ifdef CONFIG_GFAR_TX_NONAPI
	gfar_write(&gfargrp->regs->ievent, IEVENT_RX_MASK);
#else
	gfar_write(&regs->ievent, IEVENT_RTX_MASK);
#endif

	while (num_queues && left_over_budget) {

		budget_per_queue = left_over_budget/num_queues;
		left_over_budget = 0;

		for_each_bit(i, &gfargrp->rx_bit_map, priv->num_rx_queues) {
			if (test_bit(i, &serviced_queues))
				continue;
			rx_queue = priv->rx_queue[i];

#ifndef CONFIG_GFAR_TX_NONAPI
			tx_queue = priv->tx_queue[rx_queue->qindex];

			/* If we fail to get the lock,
			 * don't bother with the TX BDs */
			if (spin_trylock_irqsave(&tx_queue->txlock, flags)) {
				tx_cleaned += gfar_clean_tx_ring(tx_queue);
				spin_unlock_irqrestore(&tx_queue->txlock,
							flags);
			}
#endif
			rx_cleaned_per_queue = gfar_clean_rx_ring(rx_queue,
							budget_per_queue);
			rx_cleaned += rx_cleaned_per_queue;
			if(rx_cleaned_per_queue < budget_per_queue) {
				left_over_budget = left_over_budget +
					(budget_per_queue - rx_cleaned_per_queue);
				set_bit(i, &serviced_queues);
				num_queues--;
			}
		}
	}

#ifndef CONFIG_GFAR_TX_NONAPI
	if (tx_cleaned)
		return budget;
#endif

	if (rx_cleaned < budget) {
		napi_complete(napi);

		/* Clear the halt bit in RSTAT */
		gfar_write(&regs->rstat, gfargrp->rstat);

		gfar_write(&regs->imask, IMASK_DEFAULT);

		/* If we are coalescing interrupts, update the timer */
		/* Otherwise, clear it */
		gfar_configure_rx_coalescing(priv, gfargrp->rx_bit_map);
#ifndef CONFIG_GFAR_TX_NONAPI
		gfar_configure_tx_coalescing(priv, gfargrp->tx_bit_map);
#endif
	}

	return rx_cleaned;
}
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void gfar_netpoll(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	int i = 0;

	/* If the device has multiple interrupts, run tx/rx */
	if (priv->device_flags & FSL_GIANFAR_DEV_HAS_MULTI_INTR) {
		for (i = 0; i < priv->num_grps; i++) {
			disable_irq(priv->gfargrp[i].interruptTransmit);
			disable_irq(priv->gfargrp[i].interruptReceive);
			disable_irq(priv->gfargrp[i].interruptError);
			gfar_interrupt(priv->gfargrp[i].interruptTransmit,
						&priv->gfargrp[i]);
			enable_irq(priv->gfargrp[i].interruptError);
			enable_irq(priv->gfargrp[i].interruptReceive);
			enable_irq(priv->gfargrp[i].interruptTransmit);
		}
	} else {
		for (i = 0; i < priv->num_grps; i++) {
			disable_irq(priv->gfargrp[i].interruptTransmit);
			gfar_interrupt(priv->gfargrp[i].interruptTransmit,
						&priv->gfargrp[i]);
			enable_irq(priv->gfargrp[i].interruptTransmit);
	}
}
#endif

/* The interrupt handler for devices with one interrupt */
static irqreturn_t gfar_interrupt(int irq, void *grp_id)
{
	struct gfar_priv_grp *gfargrp = grp_id;

	/* Save ievent for future reference */
	u32 events = gfar_read(&gfargrp->regs->ievent);

	/* Check for reception */
	if (events & IEVENT_RX_MASK)
		gfar_receive(irq, grp_id);

	/* Check for transmit completion */
	if (events & IEVENT_TX_MASK)
		gfar_transmit(irq, grp_id);

	/* Check for errors */
	if (events & IEVENT_ERR_MASK)
		gfar_error(irq, grp_id);

	return IRQ_HANDLED;
}

/* Called every time the controller might need to be made
 * aware of new link state.  The PHY code conveys this
 * information through variables in the phydev structure, and this
 * function converts those variables into the appropriate
 * register values, and can bring down the device if needed.
 */
static void adjust_link(struct net_device *dev)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	unsigned long flags;
	struct phy_device *phydev = priv->phydev;
	int new_state = 0;

	local_irq_save(flags);
	lock_tx_qs(priv);

	if (phydev->link) {
		u32 tempval = gfar_read(&regs->maccfg2);
		u32 ecntrl = gfar_read(&regs->ecntrl);

		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode. */
		if (phydev->duplex != priv->oldduplex) {
			new_state = 1;
			if (!(phydev->duplex))
				tempval &= ~(MACCFG2_FULL_DUPLEX);
			else
				tempval |= MACCFG2_FULL_DUPLEX;

			priv->oldduplex = phydev->duplex;
		}

		if (phydev->speed != priv->oldspeed) {
			new_state = 1;
			switch (phydev->speed) {
			case 1000:
				tempval =
				    ((tempval & ~(MACCFG2_IF)) | MACCFG2_GMII);

				ecntrl &= ~(ECNTRL_R100);
				break;
			case 100:
			case 10:
				tempval =
				    ((tempval & ~(MACCFG2_IF)) | MACCFG2_MII);

				/* Reduced mode distinguishes
				 * between 10 and 100 */
				if (phydev->speed == SPEED_100)
					ecntrl |= ECNTRL_R100;
				else
					ecntrl &= ~(ECNTRL_R100);
				break;
			default:
				if (netif_msg_link(priv))
					printk(KERN_WARNING
						"%s: Ack!  Speed (%d) is not 10/100/1000!\n",
						dev->name, phydev->speed);
				break;
			}

			priv->oldspeed = phydev->speed;
		}

		gfar_write(&regs->maccfg2, tempval);
		gfar_write(&regs->ecntrl, ecntrl);

		if (!priv->oldlink) {
			new_state = 1;
			priv->oldlink = 1;
		}
	} else if (priv->oldlink) {
		new_state = 1;
		priv->oldlink = 0;
		priv->oldspeed = 0;
		priv->oldduplex = -1;
	}

	if (new_state && netif_msg_link(priv))
		phy_print_status(phydev);
	unlock_tx_qs(priv);
	local_irq_restore(flags);
}

/* Update the hash table based on the current list of multicast
 * addresses we subscribe to.  Also, change the promiscuity of
 * the device based on the flags (this function is called
 * whenever dev->flags is changed */
static void gfar_set_multi(struct net_device *dev)
{
	struct dev_mc_list *mc_ptr;
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	u32 tempval;

	if (dev->flags & IFF_PROMISC) {
		/* Set RCTRL to PROM */
		tempval = gfar_read(&regs->rctrl);
		tempval |= RCTRL_PROM;
		gfar_write(&regs->rctrl, tempval);
	} else {
		/* Set RCTRL to not PROM */
		tempval = gfar_read(&regs->rctrl);
		tempval &= ~(RCTRL_PROM);
		gfar_write(&regs->rctrl, tempval);
	}

	if (dev->flags & IFF_ALLMULTI) {
		/* Set the hash to rx all multicast frames */
		gfar_write(&regs->igaddr0, 0xffffffff);
		gfar_write(&regs->igaddr1, 0xffffffff);
		gfar_write(&regs->igaddr2, 0xffffffff);
		gfar_write(&regs->igaddr3, 0xffffffff);
		gfar_write(&regs->igaddr4, 0xffffffff);
		gfar_write(&regs->igaddr5, 0xffffffff);
		gfar_write(&regs->igaddr6, 0xffffffff);
		gfar_write(&regs->igaddr7, 0xffffffff);
		gfar_write(&regs->gaddr0, 0xffffffff);
		gfar_write(&regs->gaddr1, 0xffffffff);
		gfar_write(&regs->gaddr2, 0xffffffff);
		gfar_write(&regs->gaddr3, 0xffffffff);
		gfar_write(&regs->gaddr4, 0xffffffff);
		gfar_write(&regs->gaddr5, 0xffffffff);
		gfar_write(&regs->gaddr6, 0xffffffff);
		gfar_write(&regs->gaddr7, 0xffffffff);
	} else {
		int em_num;
		int idx;

		/* zero out the hash */
		gfar_write(&regs->igaddr0, 0x0);
		gfar_write(&regs->igaddr1, 0x0);
		gfar_write(&regs->igaddr2, 0x0);
		gfar_write(&regs->igaddr3, 0x0);
		gfar_write(&regs->igaddr4, 0x0);
		gfar_write(&regs->igaddr5, 0x0);
		gfar_write(&regs->igaddr6, 0x0);
		gfar_write(&regs->igaddr7, 0x0);
		gfar_write(&regs->gaddr0, 0x0);
		gfar_write(&regs->gaddr1, 0x0);
		gfar_write(&regs->gaddr2, 0x0);
		gfar_write(&regs->gaddr3, 0x0);
		gfar_write(&regs->gaddr4, 0x0);
		gfar_write(&regs->gaddr5, 0x0);
		gfar_write(&regs->gaddr6, 0x0);
		gfar_write(&regs->gaddr7, 0x0);

		/* If we have extended hash tables, we need to
		 * clear the exact match registers to prepare for
		 * setting them */
		if (priv->extended_hash) {
			em_num = GFAR_EM_NUM + 1;
			gfar_clear_exact_match(dev);
			idx = 1;
		} else {
			idx = 0;
			em_num = 0;
		}

		if (dev->mc_count == 0)
			return;

		/* Parse the list, and set the appropriate bits */
		for(mc_ptr = dev->mc_list; mc_ptr; mc_ptr = mc_ptr->next) {
			if (idx < em_num) {
				gfar_set_mac_for_addr(dev, idx,
						mc_ptr->dmi_addr);
				idx++;
			} else
				gfar_set_hash_for_addr(dev, mc_ptr->dmi_addr);
		}
	}

	return;
}


/* Clears each of the exact match registers to zero, so they
 * don't interfere with normal reception */
static void gfar_clear_exact_match(struct net_device *dev)
{
	int idx;
	u8 zero_arr[MAC_ADDR_LEN] = {0,0,0,0,0,0};

	for(idx = 1;idx < GFAR_EM_NUM + 1;idx++)
		gfar_set_mac_for_addr(dev, idx, (u8 *)zero_arr);
}

/* Set the appropriate hash bit for the given addr */
/* The algorithm works like so:
 * 1) Take the Destination Address (ie the multicast address), and
 * do a CRC on it (little endian), and reverse the bits of the
 * result.
 * 2) Use the 8 most significant bits as a hash into a 256-entry
 * table.  The table is controlled through 8 32-bit registers:
 * gaddr0-7.  gaddr0's MSB is entry 0, and gaddr7's LSB is
 * gaddr7.  This means that the 3 most significant bits in the
 * hash index which gaddr register to use, and the 5 other bits
 * indicate which bit (assuming an IBM numbering scheme, which
 * for PowerPC (tm) is usually the case) in the register holds
 * the entry. */
static void gfar_set_hash_for_addr(struct net_device *dev, u8 *addr)
{
	u32 tempval;
	struct gfar_private *priv = netdev_priv(dev);
	u32 result = ether_crc(MAC_ADDR_LEN, addr);
	int width = priv->hash_width;
	u8 whichbit = (result >> (32 - width)) & 0x1f;
	u8 whichreg = result >> (32 - width + 5);
	u32 value = (1 << (31-whichbit));

	tempval = gfar_read(priv->hash_regs[whichreg]);
	tempval |= value;
	gfar_write(priv->hash_regs[whichreg], tempval);

	return;
}


/* There are multiple MAC Address register pairs on some controllers
 * This function sets the numth pair to a given address
 */
static void gfar_set_mac_for_addr(struct net_device *dev, int num, u8 *addr)
{
	struct gfar_private *priv = netdev_priv(dev);
	struct gfar __iomem *regs = priv->gfargrp[0].regs;
	int idx;
	char tmpbuf[MAC_ADDR_LEN];
	u32 tempval;
	u32 __iomem *macptr = &regs->macstnaddr1;

	macptr += num*2;

	/* Now copy it into the mac registers backwards, cuz */
	/* little endian is silly */
	for (idx = 0; idx < MAC_ADDR_LEN; idx++)
		tmpbuf[MAC_ADDR_LEN - 1 - idx] = addr[idx];

	gfar_write(macptr, *((u32 *) (tmpbuf)));

	tempval = *((u32 *) (tmpbuf + 4));

	gfar_write(macptr+1, tempval);
}

/* GFAR error interrupt handler */
static irqreturn_t gfar_error(int irq, void *grp_id)
{
	struct gfar_priv_grp *gfargrp = grp_id;
	struct gfar __iomem *regs = gfargrp->regs;
	struct gfar_private *priv= gfargrp->priv;
	struct net_device *dev = priv->ndev;

	/* Save ievent for future reference */
	u32 events = gfar_read(&regs->ievent);

	/* Clear IEVENT */
	gfar_write(&regs->ievent, events & IEVENT_ERR_MASK);

	/* Magic Packet is not an error. */
	if ((priv->device_flags & FSL_GIANFAR_DEV_HAS_MAGIC_PACKET) &&
	    (events & IEVENT_MAG)) {
		priv->should_wake = 1;
		events &= ~IEVENT_MAG;
	}

	/* Hmm... */
	if (netif_msg_rx_err(priv) || netif_msg_tx_err(priv))
		printk(KERN_DEBUG "%s: error interrupt (ievent=0x%08x imask=0x%08x)\n",
		       dev->name, events, gfar_read(&regs->imask));

	/* Update the error counters */
	if (events & IEVENT_TXE) {
		dev->stats.tx_errors++;

		if (events & IEVENT_LC)
			dev->stats.tx_window_errors++;
		if (events & IEVENT_CRL)
			dev->stats.tx_aborted_errors++;
		if (events & IEVENT_XFUN) {
			if (netif_msg_tx_err(priv))
				printk(KERN_DEBUG "%s: TX FIFO underrun, "
				       "packet dropped.\n", dev->name);
			dev->stats.tx_dropped++;
			priv->extra_stats.tx_underrun++;

			/* Reactivate the Tx Queues */
			gfar_write(&regs->tstat, gfargrp->tstat);
		}

		/* internal data parity error */
		if (events & IEVENT_DPE) {
			unsigned long flags;
			u32 temp;

			local_irq_save(flags);
			lock_tx_qs(priv);

			/* soft reset the controller */
			temp = gfar_read(&regs->maccfg1);
			gfar_write(&regs->maccfg1, temp & ~(MACCFG1_TX_FLOW));
			udelay(10);
			gfar_write(&regs->maccfg1, temp & ~(MACCFG1_TX_FLOW | MACCFG1_TX_EN));
			udelay(1);
			gfar_write(&regs->maccfg1, temp);

			/* Reactivate the Tx Queues */
			gfar_write(&regs->tstat, gfargrp->tstat);

			unlock_tx_qs(priv);
			local_irq_restore(flags);
		}
		if (netif_msg_tx_err(priv))
			printk(KERN_DEBUG "%s: Transmit Error\n", dev->name);
	}
	if (events & IEVENT_BSY) {
		dev->stats.rx_errors++;
		priv->extra_stats.rx_bsy++;

		gfar_receive(irq, grp_id);

		if (netif_msg_rx_err(priv))
			printk(KERN_DEBUG "%s: busy error (rstat: %x)\n",
			       dev->name, gfar_read(&regs->rstat));
	}
	if (events & IEVENT_BABR) {
		dev->stats.rx_errors++;
		priv->extra_stats.rx_babr++;

		if (netif_msg_rx_err(priv))
			printk(KERN_DEBUG "%s: babbling RX error\n", dev->name);
	}
	if (events & IEVENT_EBERR) {
		priv->extra_stats.eberr++;
		if (netif_msg_rx_err(priv))
			printk(KERN_DEBUG "%s: bus error\n", dev->name);
	}
	if ((events & IEVENT_RXC) && netif_msg_rx_status(priv))
		printk(KERN_DEBUG "%s: control frame\n", dev->name);

	if (events & IEVENT_BABT) {
		priv->extra_stats.tx_babt++;
		if (netif_msg_tx_err(priv))
			printk(KERN_DEBUG "%s: babbling TX error\n", dev->name);
	}
	return IRQ_HANDLED;
}

#ifdef MY_ABC_HERE
void SynoQorIQWOLSet(void) {
	struct device_node *pDevNode = NULL;
	struct of_device * ofdev = NULL;
	struct gfar_private *priv = NULL;
	unsigned short szMacTmp[MAC_ADDR_LEN/2] = {'0'};
	int idx = 0;

	for (pDevNode = of_find_node_by_name(NULL, "ethernet"); pDevNode;
		 pDevNode = of_find_node_by_name(pDevNode, "ethernet")) {

		if (NULL == (ofdev = of_find_device_by_node(pDevNode))) {
			printk("Cannot found ofdev\n");
			continue;
		}

		if (NULL == (priv = dev_get_drvdata(&ofdev->dev))) {
			printk("NULL == (priv = dev_get_drvdata(&ofdev->dev))");
			continue;
		}

		if (NULL == priv->ndev) {
			printk("NULL == priv->ndev\n");
			continue;
		}

		if (NULL == priv->phydev) {
			printk("no phy devices\n");
			continue;
		}

		if (!(priv->wol_opts & GIANFAR_WOL_MAGIC)) {
			continue;
		}

		printk("Wol MAC addr %pM \n", priv->ndev->dev_addr);
		for( idx = 0; idx < 3; ++idx ) {
			szMacTmp[idx] = (priv->ndev->dev_addr[idx*2] & 0xff) | (priv->ndev->dev_addr[idx*2 + 1] & 0xff) << 8;
		}

		/* Tell phy about MAC address */
		phy_write(priv->phydev, 31, 0x7);
		phy_write(priv->phydev, 30, 0x6E);
		phy_write(priv->phydev, 21, szMacTmp[0]);
		phy_write(priv->phydev, 22, szMacTmp[1]);
		phy_write(priv->phydev, 23, szMacTmp[2]);

		/* Limit packet size */
		phy_write(priv->phydev, 30, 0x6D);
		phy_write(priv->phydev, 22, 0x1FFF);

		/* enable Magic packet */
		phy_write(priv->phydev, 30, 0x6D);
		phy_write(priv->phydev, 21, 0x1000);
		phy_write(priv->phydev, 31, 0x0000); //set PHY to page 0
	}

	of_node_put(pDevNode);

END:
	return 0;
}
EXPORT_SYMBOL(SynoQorIQWOLSet);
#endif

int SynoQorIQSetPhyLed(SYNO_LED ledStatus)
{
	return 0;
}
EXPORT_SYMBOL(SynoQorIQSetPhyLed);

static struct of_device_id gfar_match[] =
{
	{
		.type = "network",
		.compatible = "gianfar",
	},
	{
		.compatible = "fsl,etsec2",
	},
	{},
};
MODULE_DEVICE_TABLE(of, gfar_match);

/* Structure for a device driver */
static struct of_platform_driver gfar_driver = {
	.name = "fsl-gianfar",
	.match_table = gfar_match,

	.probe = gfar_probe,
	.remove = gfar_remove,
	.suspend = gfar_suspend,
	.resume = gfar_resume,
};

static int __init gfar_init(void)
{
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	gfar_cpu_dev_init();
#endif
	return of_register_platform_driver(&gfar_driver);
}

static void __exit gfar_exit(void)
{
#ifdef CONFIG_GFAR_SW_PKT_STEERING
	gfar_cpu_dev_exit();
#endif
	of_unregister_platform_driver(&gfar_driver);
}

module_init(gfar_init);
module_exit(gfar_exit);

