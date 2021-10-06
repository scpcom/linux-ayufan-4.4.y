/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 *   redistributing this file, you may do so under either license.
 *
 *   GPL LICENSE SUMMARY
 *
 *   Copyright(c) 2012 Intel Corporation. All rights reserved.
 *   Copyright (C) 2015 EMC Corporation. All Rights Reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation.
 *
 *   BSD LICENSE
 *
 *   Copyright(c) 2012 Intel Corporation. All rights reserved.
 *   Copyright (C) 2015 EMC Corporation. All Rights Reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copy
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * PCIe NTB Network Linux driver
 *
 * Contact Information:
 * Jon Mason <jon.mason@intel.com>
 */
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ntb.h>
#include <linux/ntb_transport.h>
#ifdef CONFIG_SYNO_NTB
#include <linux/delay.h>
#include <linux/proc_fs.h>
#endif /* CONFIG_SYNO_NTB */

#define NTB_NETDEV_VER	"0.7"

MODULE_DESCRIPTION(KBUILD_MODNAME);
MODULE_VERSION(NTB_NETDEV_VER);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Intel Corporation");

/* Time in usecs for tx resource reaper */
static unsigned int tx_time = 1;

/* Number of descriptors to free before resuming tx */
static unsigned int tx_start = 10;

/* Number of descriptors still available before stop upper layer tx */
static unsigned int tx_stop = 5;

struct ntb_netdev {
	struct list_head list;
	struct pci_dev *pdev;
	struct net_device *ndev;
	struct ntb_transport_qp *qp;
	struct timer_list tx_timer;
#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */
	unsigned int count_of_rx_allocate_fail;
	struct delayed_work ntb_skb_allocate_delay_work;
#endif /* CONFIG_SYNO_NTB */
};

#ifdef CONFIG_SYNO_NTB
static void SYNONtbSkbAllocateWork(struct work_struct *work);
#endif /* CONFIG_SYNO_NTB */

#define	NTB_TX_TIMEOUT_MS	1000
#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */ 
#define NTB_RXQ_SIZE            10000
#else
#define	NTB_RXQ_SIZE		100
#endif /* CONFIG_SYNO_NTB */ 

#ifdef CONFIG_SYNO_NTB
#define NTB_SKB_ALLOCATE_INTERVAL_MS 100
#endif /* CONFIG_SYNO_NTB */

#ifdef CONFIG_SYNO_NTB
static struct ntb_netdev *gbNtbNetDev = NULL;
#endif /* CONFIG_SYNO_NTB */

static LIST_HEAD(dev_list);

static void ntb_netdev_event_handler(void *data, int link_is_up)
{
	struct net_device *ndev = data;
	struct ntb_netdev *dev = netdev_priv(ndev);

	netdev_dbg(ndev, "Event %x, Link %x\n", link_is_up,
		   ntb_transport_link_query(dev->qp));

	if (link_is_up) {
		if (ntb_transport_link_query(dev->qp))
			netif_carrier_on(ndev);
	} else {
		netif_carrier_off(ndev);
	}
}

static void ntb_netdev_rx_handler(struct ntb_transport_qp *qp, void *qp_data,
				  void *data, int len)
{
	struct net_device *ndev = qp_data;
	struct sk_buff *skb;
	int rc;
#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */
	struct ntb_netdev *dev = netdev_priv(ndev);
#endif /* CONFIG_SYNO_NTB */

	skb = data;
	if (!skb)
		return;

	netdev_dbg(ndev, "%s: %d byte payload received\n", __func__, len);

	if (len < 0) {
		ndev->stats.rx_errors++;
		ndev->stats.rx_length_errors++;
		goto enqueue_again;
	}

	skb_put(skb, len);
	skb->protocol = eth_type_trans(skb, ndev);
	skb->ip_summed = CHECKSUM_NONE;

	if (netif_rx(skb) == NET_RX_DROP) {
		ndev->stats.rx_errors++;
		ndev->stats.rx_dropped++;
	} else {
		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += len;
	}

#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */
	skb = __netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN, GFP_ATOMIC | __GFP_HIGH);
#else
	skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN);
#endif /* CONFIG_SYNO_NTB */
	if (!skb) {
#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */
		dev->count_of_rx_allocate_fail++;
		if (NTB_RXQ_SIZE == dev->count_of_rx_allocate_fail) {
			printk("ntb_netdev skb buffer is empty\n");
			schedule_delayed_work(&dev->ntb_skb_allocate_delay_work, 0);
		}
#else
		ndev->stats.rx_errors++;
		ndev->stats.rx_frame_errors++;
#endif /* CONFIG_SYNO_NTB */
		return;
	}

enqueue_again:
	rc = ntb_transport_rx_enqueue(qp, skb, skb->data, ndev->mtu + ETH_HLEN);
	if (rc) {
		dev_kfree_skb(skb);
		ndev->stats.rx_errors++;
		ndev->stats.rx_fifo_errors++;
	}

#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */
	while(dev->count_of_rx_allocate_fail) {
                skb = __netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN, GFP_ATOMIC | __GFP_HIGH);
                if(!skb) {
                        break;
                } else {
                        rc = ntb_transport_rx_enqueue(qp, skb, skb->data, ndev->mtu + ETH_HLEN);
                        if (rc) {
                                dev_kfree_skb(skb);
                                break;
                        }
                        dev->count_of_rx_allocate_fail--;
                }
        }
#endif /* CONFIG_SYNO_NTB */
}

static int __ntb_netdev_maybe_stop_tx(struct net_device *netdev,
				      struct ntb_transport_qp *qp, int size)
{
	struct ntb_netdev *dev = netdev_priv(netdev);

	netif_stop_queue(netdev);
	/* Make sure to see the latest value of ntb_transport_tx_free_entry()
	 * since the queue was last started.
	 */
	smp_mb();

	if (likely(ntb_transport_tx_free_entry(qp) < size)) {
		mod_timer(&dev->tx_timer, jiffies + usecs_to_jiffies(tx_time));
		return -EBUSY;
	}

	netif_start_queue(netdev);
	return 0;
}

static int ntb_netdev_maybe_stop_tx(struct net_device *ndev,
				    struct ntb_transport_qp *qp, int size)
{
	if (netif_queue_stopped(ndev) ||
	    (ntb_transport_tx_free_entry(qp) >= size))
		return 0;

	return __ntb_netdev_maybe_stop_tx(ndev, qp, size);
}

static void ntb_netdev_tx_handler(struct ntb_transport_qp *qp, void *qp_data,
				  void *data, int len)
{
	struct net_device *ndev = qp_data;
	struct sk_buff *skb;
	struct ntb_netdev *dev = netdev_priv(ndev);

	skb = data;
	if (!skb || !ndev)
		return;

	if (len > 0) {
		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += skb->len;
	} else {
		ndev->stats.tx_errors++;
		ndev->stats.tx_aborted_errors++;
	}

	dev_kfree_skb(skb);

	if (ntb_transport_tx_free_entry(dev->qp) >= tx_start) {
		/* Make sure anybody stopping the queue after this sees the new
		 * value of ntb_transport_tx_free_entry()
		 */
		smp_mb();
		if (netif_queue_stopped(ndev))
			netif_wake_queue(ndev);
	}
}

static netdev_tx_t ntb_netdev_start_xmit(struct sk_buff *skb,
					 struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	int rc;

	ntb_netdev_maybe_stop_tx(ndev, dev->qp, tx_stop);

	rc = ntb_transport_tx_enqueue(dev->qp, skb, skb->data, skb->len);
	if (rc)
		goto err;

	/* check for next submit */
	ntb_netdev_maybe_stop_tx(ndev, dev->qp, tx_stop);

	return NETDEV_TX_OK;

err:
	ndev->stats.tx_dropped++;
	ndev->stats.tx_errors++;
	return NETDEV_TX_BUSY;
}

static void ntb_netdev_tx_timer(unsigned long data)
{
	struct net_device *ndev = (struct net_device *)data;
	struct ntb_netdev *dev = netdev_priv(ndev);

	if (ntb_transport_tx_free_entry(dev->qp) < tx_stop) {
		mod_timer(&dev->tx_timer, jiffies + msecs_to_jiffies(tx_time));
	} else {
		/* Make sure anybody stopping the queue after this sees the new
		 * value of ntb_transport_tx_free_entry()
		 */
		smp_mb();
		if (netif_queue_stopped(ndev))
			netif_wake_queue(ndev);
	}
}

#ifdef CONFIG_SYNO_NTB
static void SYNONtbSkbAllocateWork(struct work_struct *work)
{
	struct ntb_netdev *dev = container_of(work,
			struct ntb_netdev, ntb_skb_allocate_delay_work.work);
	struct sk_buff *skb = NULL;
	struct ntb_transport_qp *qp = dev->qp;
	struct net_device *ndev = dev->ndev;
	int iResult = 0;
	bool blAllocSucc = false;

	skb = __netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN, GFP_ATOMIC | __GFP_HIGH);
	if(skb) {
		iResult = ntb_transport_rx_enqueue(qp, skb, skb->data, ndev->mtu + ETH_HLEN);
		if (iResult) {
			dev_kfree_skb(skb);
		} else {
			dev->count_of_rx_allocate_fail--;
			blAllocSucc = true;
		}
	}

	if (!blAllocSucc) {
		schedule_delayed_work(&dev->ntb_skb_allocate_delay_work,
			       msecs_to_jiffies(NTB_SKB_ALLOCATE_INTERVAL_MS));
	} else {
		printk("ntb_netdev workqueue success to allocte a buffer\n");
	}
}
#endif /* CONFIG_SYNO_NTB */

static int ntb_netdev_open(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int rc, i, len;

	/* Add some empty rx bufs */
	for (i = 0; i < NTB_RXQ_SIZE; i++) {
		skb = netdev_alloc_skb(ndev, ndev->mtu + ETH_HLEN);
		if (!skb) {
			rc = -ENOMEM;
			goto err;
		}

		rc = ntb_transport_rx_enqueue(dev->qp, skb, skb->data,
					      ndev->mtu + ETH_HLEN);
		if (rc) {
			dev_kfree_skb(skb);
			goto err;
		}
	}

	setup_timer(&dev->tx_timer, ntb_netdev_tx_timer, (unsigned long)ndev);

	netif_carrier_off(ndev);
	ntb_transport_link_up(dev->qp);
	netif_start_queue(ndev);
#ifdef CONFIG_SYNO_NTB
	INIT_DELAYED_WORK(&dev->ntb_skb_allocate_delay_work, SYNONtbSkbAllocateWork);
#endif /* CONFIG_SYNO_NTB */

	return 0;

err:
	while ((skb = ntb_transport_rx_remove(dev->qp, &len)))
		dev_kfree_skb(skb);
	return rc;
}

static int ntb_netdev_close(struct net_device *ndev)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int len;

	ntb_transport_link_down(dev->qp);

	while ((skb = ntb_transport_rx_remove(dev->qp, &len)))
		dev_kfree_skb(skb);

	del_timer_sync(&dev->tx_timer);

	return 0;
}

static int ntb_netdev_change_mtu(struct net_device *ndev, int new_mtu)
{
	struct ntb_netdev *dev = netdev_priv(ndev);
	struct sk_buff *skb;
	int len, rc;

	if (new_mtu > ntb_transport_max_size(dev->qp) - ETH_HLEN)
		return -EINVAL;

	if (!netif_running(ndev)) {
		ndev->mtu = new_mtu;
		return 0;
	}

	/* Bring down the link and dispose of posted rx entries */
	ntb_transport_link_down(dev->qp);

	if (ndev->mtu < new_mtu) {
		int i;

		for (i = 0; (skb = ntb_transport_rx_remove(dev->qp, &len)); i++)
			dev_kfree_skb(skb);

		for (; i; i--) {
			skb = netdev_alloc_skb(ndev, new_mtu + ETH_HLEN);
			if (!skb) {
				rc = -ENOMEM;
				goto err;
			}

			rc = ntb_transport_rx_enqueue(dev->qp, skb, skb->data,
						      new_mtu + ETH_HLEN);
			if (rc) {
				dev_kfree_skb(skb);
				goto err;
			}
		}
	}

	ndev->mtu = new_mtu;

	ntb_transport_link_up(dev->qp);

	return 0;

err:
	ntb_transport_link_down(dev->qp);

	while ((skb = ntb_transport_rx_remove(dev->qp, &len)))
		dev_kfree_skb(skb);

	netdev_err(ndev, "Error changing MTU, device inoperable\n");
	return rc;
}

static const struct net_device_ops ntb_netdev_ops = {
	.ndo_open = ntb_netdev_open,
	.ndo_stop = ntb_netdev_close,
	.ndo_start_xmit = ntb_netdev_start_xmit,
	.ndo_change_mtu = ntb_netdev_change_mtu,
	.ndo_set_mac_address = eth_mac_addr,
};

static void ntb_get_drvinfo(struct net_device *ndev,
			    struct ethtool_drvinfo *info)
{
	struct ntb_netdev *dev = netdev_priv(ndev);

	strlcpy(info->driver, KBUILD_MODNAME, sizeof(info->driver));
	strlcpy(info->version, NTB_NETDEV_VER, sizeof(info->version));
	strlcpy(info->bus_info, pci_name(dev->pdev), sizeof(info->bus_info));
}

static int ntb_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	cmd->supported = SUPPORTED_Backplane;
	cmd->advertising = ADVERTISED_Backplane;
	ethtool_cmd_speed_set(cmd, SPEED_UNKNOWN);
	cmd->duplex = DUPLEX_FULL;
	cmd->port = PORT_OTHER;
	cmd->phy_address = 0;
	cmd->transceiver = XCVR_DUMMY1;
	cmd->autoneg = AUTONEG_ENABLE;
	cmd->maxtxpkt = 0;
	cmd->maxrxpkt = 0;

	return 0;
}

static const struct ethtool_ops ntb_ethtool_ops = {
	.get_drvinfo = ntb_get_drvinfo,
	.get_link = ethtool_op_get_link,
	.get_settings = ntb_get_settings,
};

static const struct ntb_queue_handlers ntb_netdev_handlers = {
	.tx_handler = ntb_netdev_tx_handler,
	.rx_handler = ntb_netdev_rx_handler,
	.event_handler = ntb_netdev_event_handler,
};

static int ntb_netdev_probe(struct device *client_dev)
{
	struct ntb_dev *ntb;
	struct net_device *ndev;
	struct pci_dev *pdev;
	struct ntb_netdev *dev;
	int rc;

	ntb = dev_ntb(client_dev->parent);
	pdev = ntb->pdev;
	if (!pdev)
		return -ENODEV;
#ifdef CONFIG_SYNO_NTB /* CONFIG_SYNO_NTB */
	ndev = alloc_netdev(sizeof(*dev), "ntb_eth%d", NET_NAME_UNKNOWN, ether_setup);
#else
	ndev = alloc_etherdev(sizeof(*dev));
#endif /* CONFIG_SYNO_NTB */
	if (!ndev)
		return -ENOMEM;

	dev = netdev_priv(ndev);
#ifdef CONFIG_SYNO_NTB
	gbNtbNetDev = dev;
#endif /* CONFIG_SYNO_NTB */
	dev->ndev = ndev;
	dev->pdev = pdev;
	ndev->features = NETIF_F_HIGHDMA;

	ndev->priv_flags |= IFF_LIVE_ADDR_CHANGE;

	ndev->hw_features = ndev->features;
	ndev->watchdog_timeo = msecs_to_jiffies(NTB_TX_TIMEOUT_MS);

	random_ether_addr(ndev->perm_addr);
	memcpy(ndev->dev_addr, ndev->perm_addr, ndev->addr_len);

	ndev->netdev_ops = &ntb_netdev_ops;
	ndev->ethtool_ops = &ntb_ethtool_ops;

	dev->qp = ntb_transport_create_queue(ndev, client_dev,
					     &ntb_netdev_handlers);
	if (!dev->qp) {
		rc = -EIO;
		goto err;
	}

	ndev->mtu = ntb_transport_max_size(dev->qp) - ETH_HLEN;

	rc = register_netdev(ndev);
	if (rc)
		goto err1;

	list_add(&dev->list, &dev_list);
	dev_info(&pdev->dev, "%s created\n", ndev->name);
	return 0;

err1:
	ntb_transport_free_queue(dev->qp);
err:
	free_netdev(ndev);
	return rc;
}

static void ntb_netdev_remove(struct device *client_dev)
{
	struct ntb_dev *ntb;
	struct net_device *ndev;
	struct pci_dev *pdev;
	struct ntb_netdev *dev;
	bool found = false;

	ntb = dev_ntb(client_dev->parent);
	pdev = ntb->pdev;

	list_for_each_entry(dev, &dev_list, list) {
		if (dev->pdev == pdev) {
			found = true;
			break;
		}
	}
	if (!found)
		return;

	list_del(&dev->list);

	ndev = dev->ndev;

#ifdef CONFIG_SYNO_NTB
	cancel_delayed_work_sync(&dev->ntb_skb_allocate_delay_work);
#endif /* CONFIG_SYNO_NTB */
	unregister_netdev(ndev);
	ntb_transport_free_queue(dev->qp);
	free_netdev(ndev);
}

static struct ntb_transport_client ntb_netdev_client = {
	.driver.name = KBUILD_MODNAME,
	.driver.owner = THIS_MODULE,
	.probe = ntb_netdev_probe,
	.remove = ntb_netdev_remove,
};

#ifdef CONFIG_SYNO_NTB
static int ntb_skb_num_proc_show(struct seq_file *m, void *v)
{
	if(gbNtbNetDev) {
        	seq_printf(m, "%d\n", gbNtbNetDev->count_of_rx_allocate_fail);
	} else {
		seq_printf(m, "ntb eth not exist");
	}
        return 0;
}

static int ntb_skb_num_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, ntb_skb_num_proc_show, NULL);
}

static const struct file_operations ntb_skb_num_proc_fops = {
        .open           = ntb_skb_num_proc_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int SynoProcNtbSkbNumInit(void)
{
        int iResult = 0;
        struct proc_dir_entry *p;

        p = proc_create("ntb_skb_num", 0, NULL, &ntb_skb_num_proc_fops);
        if (NULL == p) {
                printk("Fail to create ntb skb number proc\n");
                iResult = -1;
        }

        return iResult;
}
#endif /* CONFIG_SYNO_NTB */

static int __init ntb_netdev_init_module(void)
{
	int rc;

	rc = ntb_transport_register_client_dev(KBUILD_MODNAME);
	if (rc)
		return rc;
#ifdef CONFIG_SYNO_NTB
	rc = ntb_transport_register_client(&ntb_netdev_client);
	if (0 == rc) {
		SynoProcNtbSkbNumInit();
	}
	return rc;
#else
	return ntb_transport_register_client(&ntb_netdev_client);
#endif /* CONFIG_SYNO_NTB */
}
module_init(ntb_netdev_init_module);

static void __exit ntb_netdev_exit_module(void)
{
#ifdef CONFIG_SYNO_NTB
	remove_proc_entry("ntb_skb_num", NULL);
#endif /* CONFIG_SYNO_NTB */
	ntb_transport_unregister_client(&ntb_netdev_client);
	ntb_transport_unregister_client_dev(KBUILD_MODNAME);
}
module_exit(ntb_netdev_exit_module);
