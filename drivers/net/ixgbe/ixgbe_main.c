#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <scsi/fc/fc_fcoe.h>

#include "ixgbe.h"
#include "ixgbe_common.h"
#include "ixgbe_dcb_82599.h"

char ixgbe_driver_name[] = "ixgbe";
static const char ixgbe_driver_string[] =
                              "Intel(R) 10 Gigabit PCI Express Network Driver";

#define DRV_VERSION "2.0.44-k2"
const char ixgbe_driver_version[] = DRV_VERSION;
static char ixgbe_copyright[] = "Copyright (c) 1999-2009 Intel Corporation.";

static const struct ixgbe_info *ixgbe_info_tbl[] = {
	[board_82598] = &ixgbe_82598_info,
	[board_82599] = &ixgbe_82599_info,
};

static struct pci_device_id ixgbe_pci_tbl[] = {
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598AF_DUAL_PORT),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598AF_SINGLE_PORT),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598AT),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598AT2),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598EB_CX4),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598_CX4_DUAL_PORT),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598_DA_DUAL_PORT),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598_SR_DUAL_PORT_EM),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598EB_XF_LR),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598EB_SFP_LOM),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82598_BX),
	 board_82598 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_KX4),
	 board_82599 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_XAUI_LOM),
	 board_82599 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_KR),
	 board_82599 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_SFP),
	 board_82599 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_KX4_MEZZ),
	 board_82599 },
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_CX4),
	 board_82599 },
#ifdef MY_ABC_HERE
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_T3_LOM),
	 board_82599 },
#endif  
	{PCI_VDEVICE(INTEL, IXGBE_DEV_ID_82599_COMBO_BACKPLANE),
	 board_82599 },

	{0, }
};
MODULE_DEVICE_TABLE(pci, ixgbe_pci_tbl);

#ifdef CONFIG_IXGBE_DCA
static int ixgbe_notify_dca(struct notifier_block *, unsigned long event,
                            void *p);
static struct notifier_block dca_notifier = {
	.notifier_call = ixgbe_notify_dca,
	.next          = NULL,
	.priority      = 0
};
#endif

MODULE_AUTHOR("Intel Corporation, <linux.nics@intel.com>");
MODULE_DESCRIPTION("Intel(R) 10 Gigabit PCI Express Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

#define DEFAULT_DEBUG_LEVEL_SHIFT 3

static void ixgbe_release_hw_control(struct ixgbe_adapter *adapter)
{
	u32 ctrl_ext;

	ctrl_ext = IXGBE_READ_REG(&adapter->hw, IXGBE_CTRL_EXT);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_CTRL_EXT,
	                ctrl_ext & ~IXGBE_CTRL_EXT_DRV_LOAD);
}

static void ixgbe_get_hw_control(struct ixgbe_adapter *adapter)
{
	u32 ctrl_ext;

	ctrl_ext = IXGBE_READ_REG(&adapter->hw, IXGBE_CTRL_EXT);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_CTRL_EXT,
	                ctrl_ext | IXGBE_CTRL_EXT_DRV_LOAD);
}

static void ixgbe_set_ivar(struct ixgbe_adapter *adapter, s8 direction,
	                   u8 queue, u8 msix_vector)
{
	u32 ivar, index;
	struct ixgbe_hw *hw = &adapter->hw;
	switch (hw->mac.type) {
	case ixgbe_mac_82598EB:
		msix_vector |= IXGBE_IVAR_ALLOC_VAL;
		if (direction == -1)
			direction = 0;
		index = (((direction * 64) + queue) >> 2) & 0x1F;
		ivar = IXGBE_READ_REG(hw, IXGBE_IVAR(index));
		ivar &= ~(0xFF << (8 * (queue & 0x3)));
		ivar |= (msix_vector << (8 * (queue & 0x3)));
		IXGBE_WRITE_REG(hw, IXGBE_IVAR(index), ivar);
		break;
	case ixgbe_mac_82599EB:
		if (direction == -1) {
			 
			msix_vector |= IXGBE_IVAR_ALLOC_VAL;
			index = ((queue & 1) * 8);
			ivar = IXGBE_READ_REG(&adapter->hw, IXGBE_IVAR_MISC);
			ivar &= ~(0xFF << index);
			ivar |= (msix_vector << index);
			IXGBE_WRITE_REG(&adapter->hw, IXGBE_IVAR_MISC, ivar);
			break;
		} else {
			 
			msix_vector |= IXGBE_IVAR_ALLOC_VAL;
			index = ((16 * (queue & 1)) + (8 * direction));
			ivar = IXGBE_READ_REG(hw, IXGBE_IVAR(queue >> 1));
			ivar &= ~(0xFF << index);
			ivar |= (msix_vector << index);
			IXGBE_WRITE_REG(hw, IXGBE_IVAR(queue >> 1), ivar);
			break;
		}
	default:
		break;
	}
}

static inline void ixgbe_irq_rearm_queues(struct ixgbe_adapter *adapter,
                                          u64 qmask)
{
	u32 mask;

	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		mask = (IXGBE_EIMS_RTX_QUEUE & qmask);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS, mask);
	} else {
		mask = (qmask & 0xFFFFFFFF);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(0), mask);
		mask = (qmask >> 32);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EICS_EX(1), mask);
	}
}

static void ixgbe_unmap_and_free_tx_resource(struct ixgbe_adapter *adapter,
                                             struct ixgbe_tx_buffer
                                             *tx_buffer_info)
{
	tx_buffer_info->dma = 0;
	if (tx_buffer_info->skb) {
		skb_dma_unmap(&adapter->pdev->dev, tx_buffer_info->skb,
		              DMA_TO_DEVICE);
		dev_kfree_skb_any(tx_buffer_info->skb);
		tx_buffer_info->skb = NULL;
	}
	tx_buffer_info->time_stamp = 0;
	 
}

static inline bool ixgbe_tx_is_paused(struct ixgbe_adapter *adapter,
                                      struct ixgbe_ring *tx_ring)
{
	u32 txoff = IXGBE_TFCS_TXOFF;

#ifdef CONFIG_IXGBE_DCB
	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
		int tc;
		int reg_idx = tx_ring->reg_idx;
		int dcb_i = adapter->ring_feature[RING_F_DCB].indices;

		if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
			tc = reg_idx >> 2;
			txoff = IXGBE_TFCS_TXOFF0;
		} else if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
			tc = 0;
			txoff = IXGBE_TFCS_TXOFF;
			if (dcb_i == 8) {
				 
				tc = reg_idx >> 5;
				if (tc == 2)  
					tc += (reg_idx - 64) >> 4;
				else if (tc == 3)  
					tc += 1 + ((reg_idx - 96) >> 3);
			} else if (dcb_i == 4) {
				 
				tc = reg_idx >> 6;
				if (tc == 1) {
					tc += (reg_idx - 64) >> 5;
					if (tc == 2)  
						tc += (reg_idx - 96) >> 4;
				}
			}
		}
		txoff <<= tc;
	}
#endif
	return IXGBE_READ_REG(&adapter->hw, IXGBE_TFCS) & txoff;
}

static inline bool ixgbe_check_tx_hang(struct ixgbe_adapter *adapter,
                                       struct ixgbe_ring *tx_ring,
                                       unsigned int eop)
{
	struct ixgbe_hw *hw = &adapter->hw;

	adapter->detect_tx_hung = false;
	if (tx_ring->tx_buffer_info[eop].time_stamp &&
	    time_after(jiffies, tx_ring->tx_buffer_info[eop].time_stamp + HZ) &&
	    !ixgbe_tx_is_paused(adapter, tx_ring)) {
		 
		union ixgbe_adv_tx_desc *tx_desc;
		tx_desc = IXGBE_TX_DESC_ADV(*tx_ring, eop);
		DPRINTK(DRV, ERR, "Detected Tx Unit Hang\n"
			"  Tx Queue             <%d>\n"
			"  TDH, TDT             <%x>, <%x>\n"
			"  next_to_use          <%x>\n"
			"  next_to_clean        <%x>\n"
			"tx_buffer_info[next_to_clean]\n"
			"  time_stamp           <%lx>\n"
			"  jiffies              <%lx>\n",
			tx_ring->queue_index,
			IXGBE_READ_REG(hw, tx_ring->head),
			IXGBE_READ_REG(hw, tx_ring->tail),
			tx_ring->next_to_use, eop,
			tx_ring->tx_buffer_info[eop].time_stamp, jiffies);
		return true;
	}

	return false;
}

#define IXGBE_MAX_TXD_PWR       14
#define IXGBE_MAX_DATA_PER_TXD  (1 << IXGBE_MAX_TXD_PWR)

#define TXD_USE_COUNT(S) (((S) >> IXGBE_MAX_TXD_PWR) + \
			 (((S) & (IXGBE_MAX_DATA_PER_TXD - 1)) ? 1 : 0))
#define DESC_NEEDED (TXD_USE_COUNT(IXGBE_MAX_DATA_PER_TXD)   + \
	MAX_SKB_FRAGS * TXD_USE_COUNT(PAGE_SIZE) + 1)  

static void ixgbe_tx_timeout(struct net_device *netdev);

static bool ixgbe_clean_tx_irq(struct ixgbe_q_vector *q_vector,
                               struct ixgbe_ring *tx_ring)
{
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct net_device *netdev = adapter->netdev;
	union ixgbe_adv_tx_desc *tx_desc, *eop_desc;
	struct ixgbe_tx_buffer *tx_buffer_info;
	unsigned int i, eop, count = 0;
	unsigned int total_bytes = 0, total_packets = 0;

	i = tx_ring->next_to_clean;
	eop = tx_ring->tx_buffer_info[i].next_to_watch;
	eop_desc = IXGBE_TX_DESC_ADV(*tx_ring, eop);

	while ((eop_desc->wb.status & cpu_to_le32(IXGBE_TXD_STAT_DD)) &&
	       (count < tx_ring->work_limit)) {
		bool cleaned = false;
		for ( ; !cleaned; count++) {
			struct sk_buff *skb;
			tx_desc = IXGBE_TX_DESC_ADV(*tx_ring, i);
			tx_buffer_info = &tx_ring->tx_buffer_info[i];
			cleaned = (i == eop);
			skb = tx_buffer_info->skb;

			if (cleaned && skb) {
				unsigned int segs, bytecount;
				unsigned int hlen = skb_headlen(skb);

				segs = skb_shinfo(skb)->gso_segs ?: 1;
#ifdef IXGBE_FCOE
				 
				if ((adapter->flags & IXGBE_FLAG_FCOE_ENABLED)
				    && (skb->protocol == htons(ETH_P_FCOE)) &&
				    skb_is_gso(skb)) {
					hlen = skb_transport_offset(skb) +
						sizeof(struct fc_frame_header) +
						sizeof(struct fcoe_crc_eof);
					segs = DIV_ROUND_UP(skb->len - hlen,
						skb_shinfo(skb)->gso_size);
				}
#endif  
				 
				bytecount = ((segs - 1) * hlen) + skb->len;
				total_packets += segs;
				total_bytes += bytecount;
			}

			ixgbe_unmap_and_free_tx_resource(adapter,
			                                 tx_buffer_info);

			tx_desc->wb.status = 0;

			i++;
			if (i == tx_ring->count)
				i = 0;
		}

		eop = tx_ring->tx_buffer_info[i].next_to_watch;
		eop_desc = IXGBE_TX_DESC_ADV(*tx_ring, eop);
	}

	tx_ring->next_to_clean = i;

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (unlikely(count && netif_carrier_ok(netdev) &&
	             (IXGBE_DESC_UNUSED(tx_ring) >= TX_WAKE_THRESHOLD))) {
		 
		smp_mb();
		if (__netif_subqueue_stopped(netdev, tx_ring->queue_index) &&
		    !test_bit(__IXGBE_DOWN, &adapter->state)) {
			netif_wake_subqueue(netdev, tx_ring->queue_index);
			++adapter->restart_queue;
		}
	}

	if (adapter->detect_tx_hung) {
		if (ixgbe_check_tx_hang(adapter, tx_ring, i)) {
			 
			DPRINTK(PROBE, INFO,
			        "tx hang %d detected, resetting adapter\n",
			        adapter->tx_timeout_count + 1);
			ixgbe_tx_timeout(adapter->netdev);
		}
	}

	if (count >= tx_ring->work_limit)
		ixgbe_irq_rearm_queues(adapter, ((u64)1 << q_vector->v_idx));

	tx_ring->total_bytes += total_bytes;
	tx_ring->total_packets += total_packets;
	tx_ring->stats.packets += total_packets;
	tx_ring->stats.bytes += total_bytes;
	adapter->net_stats.tx_bytes += total_bytes;
	adapter->net_stats.tx_packets += total_packets;
	return (count < tx_ring->work_limit);
}

#ifdef CONFIG_IXGBE_DCA
static void ixgbe_update_rx_dca(struct ixgbe_adapter *adapter,
                                struct ixgbe_ring *rx_ring)
{
	u32 rxctrl;
	int cpu = get_cpu();
	int q = rx_ring - adapter->rx_ring;

	if (rx_ring->cpu != cpu) {
		rxctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_DCA_RXCTRL(q));
		if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
			rxctrl &= ~IXGBE_DCA_RXCTRL_CPUID_MASK;
			rxctrl |= dca3_get_tag(&adapter->pdev->dev, cpu);
		} else if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
			rxctrl &= ~IXGBE_DCA_RXCTRL_CPUID_MASK_82599;
			rxctrl |= (dca3_get_tag(&adapter->pdev->dev, cpu) <<
			           IXGBE_DCA_RXCTRL_CPUID_SHIFT_82599);
		}
		rxctrl |= IXGBE_DCA_RXCTRL_DESC_DCA_EN;
		rxctrl |= IXGBE_DCA_RXCTRL_HEAD_DCA_EN;
		rxctrl &= ~(IXGBE_DCA_RXCTRL_DESC_RRO_EN);
		rxctrl &= ~(IXGBE_DCA_RXCTRL_DESC_WRO_EN |
		            IXGBE_DCA_RXCTRL_DESC_HSRO_EN);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_DCA_RXCTRL(q), rxctrl);
		rx_ring->cpu = cpu;
	}
	put_cpu();
}

static void ixgbe_update_tx_dca(struct ixgbe_adapter *adapter,
                                struct ixgbe_ring *tx_ring)
{
	u32 txctrl;
	int cpu = get_cpu();
	int q = tx_ring - adapter->tx_ring;
	struct ixgbe_hw *hw = &adapter->hw;

	if (tx_ring->cpu != cpu) {
		if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL(q));
			txctrl &= ~IXGBE_DCA_TXCTRL_CPUID_MASK;
			txctrl |= dca3_get_tag(&adapter->pdev->dev, cpu);
			txctrl |= IXGBE_DCA_TXCTRL_DESC_DCA_EN;
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL(q), txctrl);
		} else if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL_82599(q));
			txctrl &= ~IXGBE_DCA_TXCTRL_CPUID_MASK_82599;
			txctrl |= (dca3_get_tag(&adapter->pdev->dev, cpu) <<
			          IXGBE_DCA_TXCTRL_CPUID_SHIFT_82599);
			txctrl |= IXGBE_DCA_TXCTRL_DESC_DCA_EN;
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL_82599(q), txctrl);
		}
		tx_ring->cpu = cpu;
	}
	put_cpu();
}

static void ixgbe_setup_dca(struct ixgbe_adapter *adapter)
{
	int i;

	if (!(adapter->flags & IXGBE_FLAG_DCA_ENABLED))
		return;

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_DCA_CTRL, 2);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		adapter->tx_ring[i].cpu = -1;
		ixgbe_update_tx_dca(adapter, &adapter->tx_ring[i]);
	}
	for (i = 0; i < adapter->num_rx_queues; i++) {
		adapter->rx_ring[i].cpu = -1;
		ixgbe_update_rx_dca(adapter, &adapter->rx_ring[i]);
	}
}

static int __ixgbe_notify_dca(struct device *dev, void *data)
{
	struct net_device *netdev = dev_get_drvdata(dev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	unsigned long event = *(unsigned long *)data;

	switch (event) {
	case DCA_PROVIDER_ADD:
		 
		if (adapter->flags & IXGBE_FLAG_DCA_ENABLED)
			break;
		if (dca_add_requester(dev) == 0) {
			adapter->flags |= IXGBE_FLAG_DCA_ENABLED;
			ixgbe_setup_dca(adapter);
			break;
		}
		 
	case DCA_PROVIDER_REMOVE:
		if (adapter->flags & IXGBE_FLAG_DCA_ENABLED) {
			dca_remove_requester(dev);
			adapter->flags &= ~IXGBE_FLAG_DCA_ENABLED;
			IXGBE_WRITE_REG(&adapter->hw, IXGBE_DCA_CTRL, 1);
		}
		break;
	}

	return 0;
}

#endif  
 
static void ixgbe_receive_skb(struct ixgbe_q_vector *q_vector,
                              struct sk_buff *skb, u8 status,
                              struct ixgbe_ring *ring,
                              union ixgbe_adv_rx_desc *rx_desc)
{
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct napi_struct *napi = &q_vector->napi;
	bool is_vlan = (status & IXGBE_RXD_STAT_VP);
	u16 tag = le16_to_cpu(rx_desc->wb.upper.vlan);

	skb_record_rx_queue(skb, ring->queue_index);
	if (!(adapter->flags & IXGBE_FLAG_IN_NETPOLL)) {
		if (adapter->vlgrp && is_vlan && (tag & VLAN_VID_MASK))
			vlan_gro_receive(napi, adapter->vlgrp, tag, skb);
		else
			napi_gro_receive(napi, skb);
	} else {
		if (adapter->vlgrp && is_vlan && (tag & VLAN_VID_MASK))
			vlan_hwaccel_rx(skb, adapter->vlgrp, tag);
		else
			netif_rx(skb);
	}
}

static inline void ixgbe_rx_checksum(struct ixgbe_adapter *adapter,
				     union ixgbe_adv_rx_desc *rx_desc,
				     struct sk_buff *skb)
{
	u32 status_err = le32_to_cpu(rx_desc->wb.upper.status_error);

	skb->ip_summed = CHECKSUM_NONE;

	if (!(adapter->flags & IXGBE_FLAG_RX_CSUM_ENABLED))
		return;

	if ((status_err & IXGBE_RXD_STAT_IPCS) &&
	    (status_err & IXGBE_RXDADV_ERR_IPE)) {
		adapter->hw_csum_rx_error++;
		return;
	}

	if (!(status_err & IXGBE_RXD_STAT_L4CS))
		return;

	if (status_err & IXGBE_RXDADV_ERR_TCPE) {
		u16 pkt_info = rx_desc->wb.lower.lo_dword.hs_rss.pkt_info;

		if ((pkt_info & IXGBE_RXDADV_PKTTYPE_UDP) &&
		    (adapter->hw.mac.type == ixgbe_mac_82599EB))
			return;

		adapter->hw_csum_rx_error++;
		return;
	}

	skb->ip_summed = CHECKSUM_UNNECESSARY;
	adapter->hw_csum_rx_good++;
}

static inline void ixgbe_release_rx_desc(struct ixgbe_hw *hw,
                                         struct ixgbe_ring *rx_ring, u32 val)
{
	 
	wmb();
	IXGBE_WRITE_REG(hw, IXGBE_RDT(rx_ring->reg_idx), val);
}

static void ixgbe_alloc_rx_buffers(struct ixgbe_adapter *adapter,
                                   struct ixgbe_ring *rx_ring,
                                   int cleaned_count)
{
	struct pci_dev *pdev = adapter->pdev;
	union ixgbe_adv_rx_desc *rx_desc;
	struct ixgbe_rx_buffer *bi;
	unsigned int i;

	i = rx_ring->next_to_use;
	bi = &rx_ring->rx_buffer_info[i];

	while (cleaned_count--) {
		rx_desc = IXGBE_RX_DESC_ADV(*rx_ring, i);

		if (!bi->page_dma &&
		    (rx_ring->flags & IXGBE_RING_RX_PS_ENABLED)) {
			if (!bi->page) {
				bi->page = alloc_page(GFP_ATOMIC);
				if (!bi->page) {
					adapter->alloc_rx_page_failed++;
					goto no_buffers;
				}
				bi->page_offset = 0;
			} else {
				 
				bi->page_offset ^= (PAGE_SIZE / 2);
			}

			bi->page_dma = pci_map_page(pdev, bi->page,
			                            bi->page_offset,
			                            (PAGE_SIZE / 2),
			                            PCI_DMA_FROMDEVICE);
		}

		if (!bi->skb) {
			struct sk_buff *skb;
			skb = netdev_alloc_skb(adapter->netdev,
			                       (rx_ring->rx_buf_len +
			                        NET_IP_ALIGN));

			if (!skb) {
				adapter->alloc_rx_buff_failed++;
				goto no_buffers;
			}

			skb_reserve(skb, NET_IP_ALIGN);

			bi->skb = skb;
			bi->dma = pci_map_single(pdev, skb->data,
			                         rx_ring->rx_buf_len,
			                         PCI_DMA_FROMDEVICE);
		}
		 
		if (rx_ring->flags & IXGBE_RING_RX_PS_ENABLED) {
			rx_desc->read.pkt_addr = cpu_to_le64(bi->page_dma);
			rx_desc->read.hdr_addr = cpu_to_le64(bi->dma);
		} else {
			rx_desc->read.pkt_addr = cpu_to_le64(bi->dma);
		}

		i++;
		if (i == rx_ring->count)
			i = 0;
		bi = &rx_ring->rx_buffer_info[i];
	}

no_buffers:
	if (rx_ring->next_to_use != i) {
		rx_ring->next_to_use = i;
		if (i-- == 0)
			i = (rx_ring->count - 1);

		ixgbe_release_rx_desc(&adapter->hw, rx_ring, i);
	}
}

static inline u16 ixgbe_get_hdr_info(union ixgbe_adv_rx_desc *rx_desc)
{
	return rx_desc->wb.lower.lo_dword.hs_rss.hdr_info;
}

static inline u16 ixgbe_get_pkt_info(union ixgbe_adv_rx_desc *rx_desc)
{
	return rx_desc->wb.lower.lo_dword.hs_rss.pkt_info;
}

static inline u32 ixgbe_get_rsc_count(union ixgbe_adv_rx_desc *rx_desc)
{
	return (le32_to_cpu(rx_desc->wb.lower.lo_dword.data) &
	        IXGBE_RXDADV_RSCCNT_MASK) >>
	        IXGBE_RXDADV_RSCCNT_SHIFT;
}

static inline struct sk_buff *ixgbe_transform_rsc_queue(struct sk_buff *skb)
{
	unsigned int frag_list_size = 0;

	while (skb->prev) {
		struct sk_buff *prev = skb->prev;
		frag_list_size += skb->len;
		skb->prev = NULL;
		skb = prev;
	}

	skb_shinfo(skb)->frag_list = skb->next;
	skb->next = NULL;
	skb->len += frag_list_size;
	skb->data_len += frag_list_size;
	skb->truesize += frag_list_size;
	return skb;
}

static bool ixgbe_clean_rx_irq(struct ixgbe_q_vector *q_vector,
                               struct ixgbe_ring *rx_ring,
                               int *work_done, int work_to_do)
{
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct pci_dev *pdev = adapter->pdev;
	union ixgbe_adv_rx_desc *rx_desc, *next_rxd;
	struct ixgbe_rx_buffer *rx_buffer_info, *next_buffer;
	struct sk_buff *skb;
	unsigned int i, rsc_count = 0;
	u32 len, staterr;
	u16 hdr_info;
	bool cleaned = false;
	int cleaned_count = 0;
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
#ifdef IXGBE_FCOE
	int ddp_bytes = 0;
#endif  

	i = rx_ring->next_to_clean;
	rx_desc = IXGBE_RX_DESC_ADV(*rx_ring, i);
	staterr = le32_to_cpu(rx_desc->wb.upper.status_error);
	rx_buffer_info = &rx_ring->rx_buffer_info[i];

	while (staterr & IXGBE_RXD_STAT_DD) {
		u32 upper_len = 0;
		if (*work_done >= work_to_do)
			break;
		(*work_done)++;

		if (rx_ring->flags & IXGBE_RING_RX_PS_ENABLED) {
			hdr_info = le16_to_cpu(ixgbe_get_hdr_info(rx_desc));
			len = (hdr_info & IXGBE_RXDADV_HDRBUFLEN_MASK) >>
			       IXGBE_RXDADV_HDRBUFLEN_SHIFT;
			if (hdr_info & IXGBE_RXDADV_SPH)
				adapter->rx_hdr_split++;
			if (len > IXGBE_RX_HDR_SIZE)
				len = IXGBE_RX_HDR_SIZE;
			upper_len = le16_to_cpu(rx_desc->wb.upper.length);
		} else {
			len = le16_to_cpu(rx_desc->wb.upper.length);
		}

		cleaned = true;
		skb = rx_buffer_info->skb;
		prefetch(skb->data - NET_IP_ALIGN);
		rx_buffer_info->skb = NULL;

		if (rx_buffer_info->dma) {
			pci_unmap_single(pdev, rx_buffer_info->dma,
			                 rx_ring->rx_buf_len,
			                 PCI_DMA_FROMDEVICE);
			rx_buffer_info->dma = 0;
			skb_put(skb, len);
		}

		if (upper_len) {
			pci_unmap_page(pdev, rx_buffer_info->page_dma,
			               PAGE_SIZE / 2, PCI_DMA_FROMDEVICE);
			rx_buffer_info->page_dma = 0;
			skb_fill_page_desc(skb, skb_shinfo(skb)->nr_frags,
			                   rx_buffer_info->page,
			                   rx_buffer_info->page_offset,
			                   upper_len);

			if ((rx_ring->rx_buf_len > (PAGE_SIZE / 2)) ||
			    (page_count(rx_buffer_info->page) != 1))
				rx_buffer_info->page = NULL;
			else
				get_page(rx_buffer_info->page);

			skb->len += upper_len;
			skb->data_len += upper_len;
			skb->truesize += upper_len;
		}

		i++;
		if (i == rx_ring->count)
			i = 0;

		next_rxd = IXGBE_RX_DESC_ADV(*rx_ring, i);
		prefetch(next_rxd);
		cleaned_count++;

		if (adapter->flags2 & IXGBE_FLAG2_RSC_CAPABLE)
			rsc_count = ixgbe_get_rsc_count(rx_desc);

		if (rsc_count) {
			u32 nextp = (staterr & IXGBE_RXDADV_NEXTP_MASK) >>
				     IXGBE_RXDADV_NEXTP_SHIFT;
			next_buffer = &rx_ring->rx_buffer_info[nextp];
			rx_ring->rsc_count += (rsc_count - 1);
		} else {
			next_buffer = &rx_ring->rx_buffer_info[i];
		}

		if (staterr & IXGBE_RXD_STAT_EOP) {
			if (skb->prev)
				skb = ixgbe_transform_rsc_queue(skb);
			rx_ring->stats.packets++;
			rx_ring->stats.bytes += skb->len;
		} else {
			if (rx_ring->flags & IXGBE_RING_RX_PS_ENABLED) {
				rx_buffer_info->skb = next_buffer->skb;
				rx_buffer_info->dma = next_buffer->dma;
				next_buffer->skb = skb;
				next_buffer->dma = 0;
			} else {
				skb->next = next_buffer->skb;
				skb->next->prev = skb;
			}
			adapter->non_eop_descs++;
			goto next_desc;
		}

		if (staterr & IXGBE_RXDADV_ERR_FRAME_ERR_MASK) {
			dev_kfree_skb_irq(skb);
			goto next_desc;
		}

		ixgbe_rx_checksum(adapter, rx_desc, skb);

		total_rx_bytes += skb->len;
		total_rx_packets++;

		skb->protocol = eth_type_trans(skb, adapter->netdev);
#ifdef IXGBE_FCOE
		 
		if (adapter->flags & IXGBE_FLAG_FCOE_ENABLED) {
			ddp_bytes = ixgbe_fcoe_ddp(adapter, rx_desc, skb);
			if (!ddp_bytes)
				goto next_desc;
		}
#endif  
		ixgbe_receive_skb(q_vector, skb, staterr, rx_ring, rx_desc);

next_desc:
		rx_desc->wb.upper.status_error = 0;

		if (cleaned_count >= IXGBE_RX_BUFFER_WRITE) {
			ixgbe_alloc_rx_buffers(adapter, rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = next_rxd;
		rx_buffer_info = &rx_ring->rx_buffer_info[i];

		staterr = le32_to_cpu(rx_desc->wb.upper.status_error);
	}

	rx_ring->next_to_clean = i;
	cleaned_count = IXGBE_DESC_UNUSED(rx_ring);

	if (cleaned_count)
		ixgbe_alloc_rx_buffers(adapter, rx_ring, cleaned_count);

#ifdef IXGBE_FCOE
	 
	if (ddp_bytes > 0) {
		unsigned int mss;

		mss = adapter->netdev->mtu - sizeof(struct fcoe_hdr) -
			sizeof(struct fc_frame_header) -
			sizeof(struct fcoe_crc_eof);
		if (mss > 512)
			mss &= ~511;
		total_rx_bytes += ddp_bytes;
		total_rx_packets += DIV_ROUND_UP(ddp_bytes, mss);
	}
#endif  

	rx_ring->total_packets += total_rx_packets;
	rx_ring->total_bytes += total_rx_bytes;
	adapter->net_stats.rx_bytes += total_rx_bytes;
	adapter->net_stats.rx_packets += total_rx_packets;

	return cleaned;
}

static int ixgbe_clean_rxonly(struct napi_struct *, int);
 
static void ixgbe_configure_msix(struct ixgbe_adapter *adapter)
{
	struct ixgbe_q_vector *q_vector;
	int i, j, q_vectors, v_idx, r_idx;
	u32 mask;

	q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;

	for (v_idx = 0; v_idx < q_vectors; v_idx++) {
		q_vector = adapter->q_vector[v_idx];
		 
		r_idx = find_first_bit(q_vector->rxr_idx,
		                       adapter->num_rx_queues);

		for (i = 0; i < q_vector->rxr_count; i++) {
			j = adapter->rx_ring[r_idx].reg_idx;
			ixgbe_set_ivar(adapter, 0, j, v_idx);
			r_idx = find_next_bit(q_vector->rxr_idx,
			                      adapter->num_rx_queues,
			                      r_idx + 1);
		}
		r_idx = find_first_bit(q_vector->txr_idx,
		                       adapter->num_tx_queues);

		for (i = 0; i < q_vector->txr_count; i++) {
			j = adapter->tx_ring[r_idx].reg_idx;
			ixgbe_set_ivar(adapter, 1, j, v_idx);
			r_idx = find_next_bit(q_vector->txr_idx,
			                      adapter->num_tx_queues,
			                      r_idx + 1);
		}

		if (q_vector->txr_count && !q_vector->rxr_count)
			 
			q_vector->eitr = adapter->tx_eitr_param;
		else if (q_vector->rxr_count)
			 
			q_vector->eitr = adapter->rx_eitr_param;

		ixgbe_write_eitr(q_vector);
	}

	if (adapter->hw.mac.type == ixgbe_mac_82598EB)
		ixgbe_set_ivar(adapter, -1, IXGBE_IVAR_OTHER_CAUSES_INDEX,
		               v_idx);
	else if (adapter->hw.mac.type == ixgbe_mac_82599EB)
		ixgbe_set_ivar(adapter, -1, 1, v_idx);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EITR(v_idx), 1950);

	mask = IXGBE_EIMS_ENABLE_MASK;
	mask &= ~(IXGBE_EIMS_OTHER | IXGBE_EIMS_LSC);
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIAC, mask);
}

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

static u8 ixgbe_update_itr(struct ixgbe_adapter *adapter,
                           u32 eitr, u8 itr_setting,
                           int packets, int bytes)
{
	unsigned int retval = itr_setting;
	u32 timepassed_us;
	u64 bytes_perint;

	if (packets == 0)
		goto update_itr_done;

	timepassed_us = 1000000/eitr;
	bytes_perint = bytes / timepassed_us;  

	switch (itr_setting) {
	case lowest_latency:
		if (bytes_perint > adapter->eitr_low)
			retval = low_latency;
		break;
	case low_latency:
		if (bytes_perint > adapter->eitr_high)
			retval = bulk_latency;
		else if (bytes_perint <= adapter->eitr_low)
			retval = lowest_latency;
		break;
	case bulk_latency:
		if (bytes_perint <= adapter->eitr_high)
			retval = low_latency;
		break;
	}

update_itr_done:
	return retval;
}

void ixgbe_write_eitr(struct ixgbe_q_vector *q_vector)
{
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct ixgbe_hw *hw = &adapter->hw;
	int v_idx = q_vector->v_idx;
	u32 itr_reg = EITR_INTS_PER_SEC_TO_REG(q_vector->eitr);

	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		 
		itr_reg |= (itr_reg << 16);
	} else if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
		 
		itr_reg |= IXGBE_EITR_CNT_WDIS;
	}
	IXGBE_WRITE_REG(hw, IXGBE_EITR(v_idx), itr_reg);
}

static void ixgbe_set_itr_msix(struct ixgbe_q_vector *q_vector)
{
	struct ixgbe_adapter *adapter = q_vector->adapter;
	u32 new_itr;
	u8 current_itr, ret_itr;
	int i, r_idx;
	struct ixgbe_ring *rx_ring, *tx_ring;

	r_idx = find_first_bit(q_vector->txr_idx, adapter->num_tx_queues);
	for (i = 0; i < q_vector->txr_count; i++) {
		tx_ring = &(adapter->tx_ring[r_idx]);
		ret_itr = ixgbe_update_itr(adapter, q_vector->eitr,
		                           q_vector->tx_itr,
		                           tx_ring->total_packets,
		                           tx_ring->total_bytes);
		 
		q_vector->tx_itr = ((q_vector->tx_itr > ret_itr) ?
		                    q_vector->tx_itr - 1 : ret_itr);
		r_idx = find_next_bit(q_vector->txr_idx, adapter->num_tx_queues,
		                      r_idx + 1);
	}

	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	for (i = 0; i < q_vector->rxr_count; i++) {
		rx_ring = &(adapter->rx_ring[r_idx]);
		ret_itr = ixgbe_update_itr(adapter, q_vector->eitr,
		                           q_vector->rx_itr,
		                           rx_ring->total_packets,
		                           rx_ring->total_bytes);
		 
		q_vector->rx_itr = ((q_vector->rx_itr > ret_itr) ?
		                    q_vector->rx_itr - 1 : ret_itr);
		r_idx = find_next_bit(q_vector->rxr_idx, adapter->num_rx_queues,
		                      r_idx + 1);
	}

	current_itr = max(q_vector->rx_itr, q_vector->tx_itr);

	switch (current_itr) {
	 
	case lowest_latency:
		new_itr = 100000;
		break;
	case low_latency:
		new_itr = 20000;  
		break;
	case bulk_latency:
	default:
		new_itr = 8000;
		break;
	}

	if (new_itr != q_vector->eitr) {
		 
		new_itr = ((q_vector->eitr * 90)/100) + ((new_itr * 10)/100);

		q_vector->eitr = new_itr;

		ixgbe_write_eitr(q_vector);
	}

	return;
}

#ifdef MY_ABC_HERE
 
static void ixgbe_check_overtemp_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter = container_of(work,
	                                             struct ixgbe_adapter,
	                                             check_overtemp_task);
	struct ixgbe_hw *hw = &adapter->hw;
	u32 eicr = adapter->interrupt_event;

	if (adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE) {
		switch (hw->device_id) {
		case IXGBE_DEV_ID_82599_T3_LOM: {
			u32 autoneg;
			bool link_up = false;

			if (hw->mac.ops.check_link)
				hw->mac.ops.check_link(hw, &autoneg, &link_up, false);

			if (((eicr & IXGBE_EICR_GPI_SDP0) && (!link_up)) ||
			    (eicr & IXGBE_EICR_LSC))
				 
				if (hw->phy.ops.check_overtemp(hw) == IXGBE_ERR_OVERTEMP)
					break;
			}
			return;
		default:
			if (!(eicr & IXGBE_EICR_GPI_SDP0))
				return;
			break;
		}
		DPRINTK(DRV, ERR, "Network adapter has been stopped because it "
		        "has over heated. Restart the computer. If the problem "
		        "persists, power off the system and replace the "
		        "adapter\n");
		 
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP0);
	}
}

#endif  
static void ixgbe_check_fan_failure(struct ixgbe_adapter *adapter, u32 eicr)
{
	struct ixgbe_hw *hw = &adapter->hw;

	if ((adapter->flags & IXGBE_FLAG_FAN_FAIL_CAPABLE) &&
	    (eicr & IXGBE_EICR_GPI_SDP1)) {
		DPRINTK(PROBE, CRIT, "Fan has stopped, replace the adapter\n");
		 
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1);
	}
}

static void ixgbe_check_sfp_event(struct ixgbe_adapter *adapter, u32 eicr)
{
	struct ixgbe_hw *hw = &adapter->hw;

	if (eicr & IXGBE_EICR_GPI_SDP1) {
		 
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP1);
		schedule_work(&adapter->multispeed_fiber_task);
	} else if (eicr & IXGBE_EICR_GPI_SDP2) {
		 
		IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_GPI_SDP2);
		schedule_work(&adapter->sfp_config_module_task);
	} else {
		 
		return;
	}
}

static void ixgbe_check_lsc(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;

	adapter->lsc_int++;
	adapter->flags |= IXGBE_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	if (!test_bit(__IXGBE_DOWN, &adapter->state)) {
		IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_EIMC_LSC);
		schedule_work(&adapter->watchdog_task);
	}
}

static irqreturn_t ixgbe_msix_lsc(int irq, void *data)
{
	struct net_device *netdev = data;
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;
	u32 eicr;

	eicr = IXGBE_READ_REG(hw, IXGBE_EICS);
	IXGBE_WRITE_REG(hw, IXGBE_EICR, eicr);

	if (eicr & IXGBE_EICR_LSC)
		ixgbe_check_lsc(adapter);

	if (hw->mac.type == ixgbe_mac_82598EB)
		ixgbe_check_fan_failure(adapter, eicr);

	if (hw->mac.type == ixgbe_mac_82599EB) {
		ixgbe_check_sfp_event(adapter, eicr);
#ifdef MY_ABC_HERE
		adapter->interrupt_event = eicr;
		if ((adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE) &&
		    ((eicr & IXGBE_EICR_GPI_SDP0) || (eicr & IXGBE_EICR_LSC)))
			schedule_work(&adapter->check_overtemp_task);
#endif  

		if (eicr & IXGBE_EICR_FLOW_DIR) {
			int i;
			IXGBE_WRITE_REG(hw, IXGBE_EICR, IXGBE_EICR_FLOW_DIR);
			 
			netif_tx_stop_all_queues(netdev);
			for (i = 0; i < adapter->num_tx_queues; i++) {
				struct ixgbe_ring *tx_ring =
				                           &adapter->tx_ring[i];
				if (test_and_clear_bit(__IXGBE_FDIR_INIT_DONE,
				                       &tx_ring->reinit_state))
					schedule_work(&adapter->fdir_reinit_task);
			}
		}
	}
	if (!test_bit(__IXGBE_DOWN, &adapter->state))
		IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EIMS_OTHER);

	return IRQ_HANDLED;
}

static inline void ixgbe_irq_enable_queues(struct ixgbe_adapter *adapter,
					   u64 qmask)
{
	u32 mask;

	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		mask = (IXGBE_EIMS_RTX_QUEUE & qmask);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS, mask);
	} else {
		mask = (qmask & 0xFFFFFFFF);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS_EX(0), mask);
		mask = (qmask >> 32);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS_EX(1), mask);
	}
	 
}

static inline void ixgbe_irq_disable_queues(struct ixgbe_adapter *adapter,
                                            u64 qmask)
{
	u32 mask;

	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		mask = (IXGBE_EIMS_RTX_QUEUE & qmask);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC, mask);
	} else {
		mask = (qmask & 0xFFFFFFFF);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC_EX(0), mask);
		mask = (qmask >> 32);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC_EX(1), mask);
	}
	 
}

static irqreturn_t ixgbe_msix_clean_tx(int irq, void *data)
{
	struct ixgbe_q_vector *q_vector = data;
	struct ixgbe_adapter  *adapter = q_vector->adapter;
	struct ixgbe_ring     *tx_ring;
	int i, r_idx;

	if (!q_vector->txr_count)
		return IRQ_HANDLED;

	r_idx = find_first_bit(q_vector->txr_idx, adapter->num_tx_queues);
	for (i = 0; i < q_vector->txr_count; i++) {
		tx_ring = &(adapter->tx_ring[r_idx]);
		tx_ring->total_bytes = 0;
		tx_ring->total_packets = 0;
		r_idx = find_next_bit(q_vector->txr_idx, adapter->num_tx_queues,
		                      r_idx + 1);
	}

	ixgbe_irq_disable_queues(adapter, ((u64)1 << q_vector->v_idx));
	napi_schedule(&q_vector->napi);

	return IRQ_HANDLED;
}

static irqreturn_t ixgbe_msix_clean_rx(int irq, void *data)
{
	struct ixgbe_q_vector *q_vector = data;
	struct ixgbe_adapter  *adapter = q_vector->adapter;
	struct ixgbe_ring  *rx_ring;
	int r_idx;
	int i;

	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	for (i = 0;  i < q_vector->rxr_count; i++) {
		rx_ring = &(adapter->rx_ring[r_idx]);
		rx_ring->total_bytes = 0;
		rx_ring->total_packets = 0;
		r_idx = find_next_bit(q_vector->rxr_idx, adapter->num_rx_queues,
		                      r_idx + 1);
	}

	if (!q_vector->rxr_count)
		return IRQ_HANDLED;

	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	rx_ring = &(adapter->rx_ring[r_idx]);
	 
	ixgbe_irq_disable_queues(adapter, ((u64)1 << q_vector->v_idx));
	napi_schedule(&q_vector->napi);

	return IRQ_HANDLED;
}

static irqreturn_t ixgbe_msix_clean_many(int irq, void *data)
{
	struct ixgbe_q_vector *q_vector = data;
	struct ixgbe_adapter  *adapter = q_vector->adapter;
	struct ixgbe_ring  *ring;
	int r_idx;
	int i;

	if (!q_vector->txr_count && !q_vector->rxr_count)
		return IRQ_HANDLED;

	r_idx = find_first_bit(q_vector->txr_idx, adapter->num_tx_queues);
	for (i = 0; i < q_vector->txr_count; i++) {
		ring = &(adapter->tx_ring[r_idx]);
		ring->total_bytes = 0;
		ring->total_packets = 0;
		r_idx = find_next_bit(q_vector->txr_idx, adapter->num_tx_queues,
		                      r_idx + 1);
	}

	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	for (i = 0; i < q_vector->rxr_count; i++) {
		ring = &(adapter->rx_ring[r_idx]);
		ring->total_bytes = 0;
		ring->total_packets = 0;
		r_idx = find_next_bit(q_vector->rxr_idx, adapter->num_rx_queues,
		                      r_idx + 1);
	}

	ixgbe_irq_disable_queues(adapter, ((u64)1 << q_vector->v_idx));
	napi_schedule(&q_vector->napi);

	return IRQ_HANDLED;
}

static int ixgbe_clean_rxonly(struct napi_struct *napi, int budget)
{
	struct ixgbe_q_vector *q_vector =
	                       container_of(napi, struct ixgbe_q_vector, napi);
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct ixgbe_ring *rx_ring = NULL;
	int work_done = 0;
	long r_idx;

	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	rx_ring = &(adapter->rx_ring[r_idx]);
#ifdef CONFIG_IXGBE_DCA
	if (adapter->flags & IXGBE_FLAG_DCA_ENABLED)
		ixgbe_update_rx_dca(adapter, rx_ring);
#endif

	ixgbe_clean_rx_irq(q_vector, rx_ring, &work_done, budget);

	if (work_done < budget) {
		napi_complete(napi);
		if (adapter->rx_itr_setting & 1)
			ixgbe_set_itr_msix(q_vector);
		if (!test_bit(__IXGBE_DOWN, &adapter->state))
			ixgbe_irq_enable_queues(adapter,
			                        ((u64)1 << q_vector->v_idx));
	}

	return work_done;
}

static int ixgbe_clean_rxtx_many(struct napi_struct *napi, int budget)
{
	struct ixgbe_q_vector *q_vector =
	                       container_of(napi, struct ixgbe_q_vector, napi);
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct ixgbe_ring *ring = NULL;
	int work_done = 0, i;
	long r_idx;
	bool tx_clean_complete = true;

	r_idx = find_first_bit(q_vector->txr_idx, adapter->num_tx_queues);
	for (i = 0; i < q_vector->txr_count; i++) {
		ring = &(adapter->tx_ring[r_idx]);
#ifdef CONFIG_IXGBE_DCA
		if (adapter->flags & IXGBE_FLAG_DCA_ENABLED)
			ixgbe_update_tx_dca(adapter, ring);
#endif
		tx_clean_complete &= ixgbe_clean_tx_irq(q_vector, ring);
		r_idx = find_next_bit(q_vector->txr_idx, adapter->num_tx_queues,
		                      r_idx + 1);
	}

	budget /= (q_vector->rxr_count ?: 1);
	budget = max(budget, 1);
	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	for (i = 0; i < q_vector->rxr_count; i++) {
		ring = &(adapter->rx_ring[r_idx]);
#ifdef CONFIG_IXGBE_DCA
		if (adapter->flags & IXGBE_FLAG_DCA_ENABLED)
			ixgbe_update_rx_dca(adapter, ring);
#endif
		ixgbe_clean_rx_irq(q_vector, ring, &work_done, budget);
		r_idx = find_next_bit(q_vector->rxr_idx, adapter->num_rx_queues,
		                      r_idx + 1);
	}

	r_idx = find_first_bit(q_vector->rxr_idx, adapter->num_rx_queues);
	ring = &(adapter->rx_ring[r_idx]);
	 
	if (work_done < budget) {
		napi_complete(napi);
		if (adapter->rx_itr_setting & 1)
			ixgbe_set_itr_msix(q_vector);
		if (!test_bit(__IXGBE_DOWN, &adapter->state))
			ixgbe_irq_enable_queues(adapter,
			                        ((u64)1 << q_vector->v_idx));
		return 0;
	}

	return work_done;
}

static int ixgbe_clean_txonly(struct napi_struct *napi, int budget)
{
	struct ixgbe_q_vector *q_vector =
	                       container_of(napi, struct ixgbe_q_vector, napi);
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct ixgbe_ring *tx_ring = NULL;
	int work_done = 0;
	long r_idx;

	r_idx = find_first_bit(q_vector->txr_idx, adapter->num_tx_queues);
	tx_ring = &(adapter->tx_ring[r_idx]);
#ifdef CONFIG_IXGBE_DCA
	if (adapter->flags & IXGBE_FLAG_DCA_ENABLED)
		ixgbe_update_tx_dca(adapter, tx_ring);
#endif

	if (!ixgbe_clean_tx_irq(q_vector, tx_ring))
		work_done = budget;

	if (work_done < budget) {
		napi_complete(napi);
		if (adapter->tx_itr_setting & 1)
			ixgbe_set_itr_msix(q_vector);
		if (!test_bit(__IXGBE_DOWN, &adapter->state))
			ixgbe_irq_enable_queues(adapter, ((u64)1 << q_vector->v_idx));
	}

	return work_done;
}

static inline void map_vector_to_rxq(struct ixgbe_adapter *a, int v_idx,
                                     int r_idx)
{
	struct ixgbe_q_vector *q_vector = a->q_vector[v_idx];

	set_bit(r_idx, q_vector->rxr_idx);
	q_vector->rxr_count++;
}

static inline void map_vector_to_txq(struct ixgbe_adapter *a, int v_idx,
                                     int t_idx)
{
	struct ixgbe_q_vector *q_vector = a->q_vector[v_idx];

	set_bit(t_idx, q_vector->txr_idx);
	q_vector->txr_count++;
}

static int ixgbe_map_rings_to_vectors(struct ixgbe_adapter *adapter,
                                      int vectors)
{
	int v_start = 0;
	int rxr_idx = 0, txr_idx = 0;
	int rxr_remaining = adapter->num_rx_queues;
	int txr_remaining = adapter->num_tx_queues;
	int i, j;
	int rqpv, tqpv;
	int err = 0;

	if (!(adapter->flags & IXGBE_FLAG_MSIX_ENABLED))
		goto out;

	if (vectors == adapter->num_rx_queues + adapter->num_tx_queues) {
		for (; rxr_idx < rxr_remaining; v_start++, rxr_idx++)
			map_vector_to_rxq(adapter, v_start, rxr_idx);

		for (; txr_idx < txr_remaining; v_start++, txr_idx++)
			map_vector_to_txq(adapter, v_start, txr_idx);

		goto out;
	}

	for (i = v_start; i < vectors; i++) {
		rqpv = DIV_ROUND_UP(rxr_remaining, vectors - i);
		for (j = 0; j < rqpv; j++) {
			map_vector_to_rxq(adapter, i, rxr_idx);
			rxr_idx++;
			rxr_remaining--;
		}
	}
	for (i = v_start; i < vectors; i++) {
		tqpv = DIV_ROUND_UP(txr_remaining, vectors - i);
		for (j = 0; j < tqpv; j++) {
			map_vector_to_txq(adapter, i, txr_idx);
			txr_idx++;
			txr_remaining--;
		}
	}

out:
	return err;
}

static int ixgbe_request_msix_irqs(struct ixgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	irqreturn_t (*handler)(int, void *);
	int i, vector, q_vectors, err;
	int ri=0, ti=0;

	q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;

	err = ixgbe_map_rings_to_vectors(adapter, q_vectors);
	if (err)
		goto out;

#define SET_HANDLER(_v) ((!(_v)->rxr_count) ? &ixgbe_msix_clean_tx : \
                         (!(_v)->txr_count) ? &ixgbe_msix_clean_rx : \
                         &ixgbe_msix_clean_many)
	for (vector = 0; vector < q_vectors; vector++) {
		handler = SET_HANDLER(adapter->q_vector[vector]);

		if(handler == &ixgbe_msix_clean_rx) {
			sprintf(adapter->name[vector], "%s-%s-%d",
				netdev->name, "rx", ri++);
		}
		else if(handler == &ixgbe_msix_clean_tx) {
			sprintf(adapter->name[vector], "%s-%s-%d",
				netdev->name, "tx", ti++);
		}
		else
			sprintf(adapter->name[vector], "%s-%s-%d",
				netdev->name, "TxRx", vector);

		err = request_irq(adapter->msix_entries[vector].vector,
		                  handler, 0, adapter->name[vector],
		                  adapter->q_vector[vector]);
		if (err) {
			DPRINTK(PROBE, ERR,
			        "request_irq failed for MSIX interrupt "
			        "Error: %d\n", err);
			goto free_queue_irqs;
		}
	}

	sprintf(adapter->name[vector], "%s:lsc", netdev->name);
	err = request_irq(adapter->msix_entries[vector].vector,
	                  &ixgbe_msix_lsc, 0, adapter->name[vector], netdev);
	if (err) {
		DPRINTK(PROBE, ERR,
			"request_irq for msix_lsc failed: %d\n", err);
		goto free_queue_irqs;
	}

	return 0;

free_queue_irqs:
	for (i = vector - 1; i >= 0; i--)
		free_irq(adapter->msix_entries[--vector].vector,
		         adapter->q_vector[i]);
	adapter->flags &= ~IXGBE_FLAG_MSIX_ENABLED;
	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
out:
	return err;
}

static void ixgbe_set_itr(struct ixgbe_adapter *adapter)
{
	struct ixgbe_q_vector *q_vector = adapter->q_vector[0];
	u8 current_itr;
	u32 new_itr = q_vector->eitr;
	struct ixgbe_ring *rx_ring = &adapter->rx_ring[0];
	struct ixgbe_ring *tx_ring = &adapter->tx_ring[0];

	q_vector->tx_itr = ixgbe_update_itr(adapter, new_itr,
	                                    q_vector->tx_itr,
	                                    tx_ring->total_packets,
	                                    tx_ring->total_bytes);
	q_vector->rx_itr = ixgbe_update_itr(adapter, new_itr,
	                                    q_vector->rx_itr,
	                                    rx_ring->total_packets,
	                                    rx_ring->total_bytes);

	current_itr = max(q_vector->rx_itr, q_vector->tx_itr);

	switch (current_itr) {
	 
	case lowest_latency:
		new_itr = 100000;
		break;
	case low_latency:
		new_itr = 20000;  
		break;
	case bulk_latency:
		new_itr = 8000;
		break;
	default:
		break;
	}

	if (new_itr != q_vector->eitr) {
		 
		new_itr = ((q_vector->eitr * 90)/100) + ((new_itr * 10)/100);

		q_vector->eitr = new_itr;

		ixgbe_write_eitr(q_vector);
	}

	return;
}

static inline void ixgbe_irq_enable(struct ixgbe_adapter *adapter)
{
	u32 mask;

	mask = (IXGBE_EIMS_ENABLE_MASK & ~IXGBE_EIMS_RTX_QUEUE);
#ifdef MY_ABC_HERE
	if (adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE)
		mask |= IXGBE_EIMS_GPI_SDP0;
#endif  
	if (adapter->flags & IXGBE_FLAG_FAN_FAIL_CAPABLE)
		mask |= IXGBE_EIMS_GPI_SDP1;
	if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
		mask |= IXGBE_EIMS_ECC;
		mask |= IXGBE_EIMS_GPI_SDP1;
		mask |= IXGBE_EIMS_GPI_SDP2;
	}
	if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE ||
	    adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE)
		mask |= IXGBE_EIMS_FLOW_DIR;

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMS, mask);
	ixgbe_irq_enable_queues(adapter, ~0);
	IXGBE_WRITE_FLUSH(&adapter->hw);
}

static irqreturn_t ixgbe_intr(int irq, void *data)
{
	struct net_device *netdev = data;
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_q_vector *q_vector = adapter->q_vector[0];
	u32 eicr;

	IXGBE_WRITE_REG(hw, IXGBE_EIMC, IXGBE_IRQ_CLEAR_MASK);

	eicr = IXGBE_READ_REG(hw, IXGBE_EICR);
	if (!eicr) {
		 
		ixgbe_irq_enable(adapter);
		return IRQ_NONE;	 
	}

	if (eicr & IXGBE_EICR_LSC)
		ixgbe_check_lsc(adapter);

	if (hw->mac.type == ixgbe_mac_82599EB)
		ixgbe_check_sfp_event(adapter, eicr);

	ixgbe_check_fan_failure(adapter, eicr);
#ifdef MY_ABC_HERE
	if ((adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE) &&
	    ((eicr & IXGBE_EICR_GPI_SDP0) || (eicr & IXGBE_EICR_LSC)))
		schedule_work(&adapter->check_overtemp_task);
#endif  

	if (napi_schedule_prep(&(q_vector->napi))) {
		adapter->tx_ring[0].total_packets = 0;
		adapter->tx_ring[0].total_bytes = 0;
		adapter->rx_ring[0].total_packets = 0;
		adapter->rx_ring[0].total_bytes = 0;
		 
		__napi_schedule(&(q_vector->napi));
	}

	return IRQ_HANDLED;
}

static inline void ixgbe_reset_q_vectors(struct ixgbe_adapter *adapter)
{
	int i, q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;

	for (i = 0; i < q_vectors; i++) {
		struct ixgbe_q_vector *q_vector = adapter->q_vector[i];
		bitmap_zero(q_vector->rxr_idx, MAX_RX_QUEUES);
		bitmap_zero(q_vector->txr_idx, MAX_TX_QUEUES);
		q_vector->rxr_count = 0;
		q_vector->txr_count = 0;
	}
}

static int ixgbe_request_irq(struct ixgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;

	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
		err = ixgbe_request_msix_irqs(adapter);
	} else if (adapter->flags & IXGBE_FLAG_MSI_ENABLED) {
		err = request_irq(adapter->pdev->irq, &ixgbe_intr, 0,
		                  netdev->name, netdev);
	} else {
		err = request_irq(adapter->pdev->irq, &ixgbe_intr, IRQF_SHARED,
		                  netdev->name, netdev);
	}

	if (err)
		DPRINTK(PROBE, ERR, "request_irq failed, Error %d\n", err);

	return err;
}

static void ixgbe_free_irq(struct ixgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
		int i, q_vectors;

		q_vectors = adapter->num_msix_vectors;

		i = q_vectors - 1;
		free_irq(adapter->msix_entries[i].vector, netdev);

		i--;
		for (; i >= 0; i--) {
			free_irq(adapter->msix_entries[i].vector,
			         adapter->q_vector[i]);
		}

		ixgbe_reset_q_vectors(adapter);
	} else {
		free_irq(adapter->pdev->irq, netdev);
	}
}

static inline void ixgbe_irq_disable(struct ixgbe_adapter *adapter)
{
	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC, ~0);
	} else {
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC, 0xFFFF0000);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC_EX(0), ~0);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_EIMC_EX(1), ~0);
	}
	IXGBE_WRITE_FLUSH(&adapter->hw);
	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
		int i;
		for (i = 0; i < adapter->num_msix_vectors; i++)
			synchronize_irq(adapter->msix_entries[i].vector);
	} else {
		synchronize_irq(adapter->pdev->irq);
	}
}

static void ixgbe_configure_msi_and_legacy(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;

	IXGBE_WRITE_REG(hw, IXGBE_EITR(0),
	                EITR_INTS_PER_SEC_TO_REG(adapter->rx_eitr_param));

	ixgbe_set_ivar(adapter, 0, 0, 0);
	ixgbe_set_ivar(adapter, 1, 0, 0);

	map_vector_to_rxq(adapter, 0, 0);
	map_vector_to_txq(adapter, 0, 0);

	DPRINTK(HW, INFO, "Legacy interrupt IVAR setup done\n");
}

static void ixgbe_configure_tx(struct ixgbe_adapter *adapter)
{
	u64 tdba;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 i, j, tdlen, txctrl;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct ixgbe_ring *ring = &adapter->tx_ring[i];
		j = ring->reg_idx;
		tdba = ring->dma;
		tdlen = ring->count * sizeof(union ixgbe_adv_tx_desc);
		IXGBE_WRITE_REG(hw, IXGBE_TDBAL(j),
		                (tdba & DMA_BIT_MASK(32)));
		IXGBE_WRITE_REG(hw, IXGBE_TDBAH(j), (tdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_TDLEN(j), tdlen);
		IXGBE_WRITE_REG(hw, IXGBE_TDH(j), 0);
		IXGBE_WRITE_REG(hw, IXGBE_TDT(j), 0);
		adapter->tx_ring[i].head = IXGBE_TDH(j);
		adapter->tx_ring[i].tail = IXGBE_TDT(j);
		 
		switch (hw->mac.type) {
		case ixgbe_mac_82598EB:
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL(j));
			break;
		case ixgbe_mac_82599EB:
		default:
			txctrl = IXGBE_READ_REG(hw, IXGBE_DCA_TXCTRL_82599(j));
			break;
		}
		txctrl &= ~IXGBE_DCA_TXCTRL_TX_WB_RO_EN;
		switch (hw->mac.type) {
		case ixgbe_mac_82598EB:
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL(j), txctrl);
			break;
		case ixgbe_mac_82599EB:
		default:
			IXGBE_WRITE_REG(hw, IXGBE_DCA_TXCTRL_82599(j), txctrl);
			break;
		}
	}

	if (hw->mac.type == ixgbe_mac_82599EB) {
		u32 rttdcs;

		rttdcs = IXGBE_READ_REG(hw, IXGBE_RTTDCS);
		rttdcs |= IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);

		if (adapter->flags & IXGBE_FLAG_DCB_ENABLED)
			IXGBE_WRITE_REG(hw, IXGBE_MTQC, (IXGBE_MTQC_RT_ENA |
			                IXGBE_MTQC_8TC_8TQ));
		else
			IXGBE_WRITE_REG(hw, IXGBE_MTQC, IXGBE_MTQC_64Q_1PB);

		rttdcs &= ~IXGBE_RTTDCS_ARBDIS;
		IXGBE_WRITE_REG(hw, IXGBE_RTTDCS, rttdcs);
	}
}

#define IXGBE_SRRCTL_BSIZEHDRSIZE_SHIFT 2

static void ixgbe_configure_srrctl(struct ixgbe_adapter *adapter,
                                   struct ixgbe_ring *rx_ring)
{
	u32 srrctl;
	int index;
	struct ixgbe_ring_feature *feature = adapter->ring_feature;

	index = rx_ring->reg_idx;
	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		unsigned long mask;
		mask = (unsigned long) feature[RING_F_RSS].mask;
		index = index & mask;
	}
	srrctl = IXGBE_READ_REG(&adapter->hw, IXGBE_SRRCTL(index));

	srrctl &= ~IXGBE_SRRCTL_BSIZEHDR_MASK;
	srrctl &= ~IXGBE_SRRCTL_BSIZEPKT_MASK;

	srrctl |= (IXGBE_RX_HDR_SIZE << IXGBE_SRRCTL_BSIZEHDRSIZE_SHIFT) &
		  IXGBE_SRRCTL_BSIZEHDR_MASK;

	if (rx_ring->flags & IXGBE_RING_RX_PS_ENABLED) {
#if (PAGE_SIZE / 2) > IXGBE_MAX_RXBUFFER
		srrctl |= IXGBE_MAX_RXBUFFER >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;
#else
		srrctl |= (PAGE_SIZE / 2) >> IXGBE_SRRCTL_BSIZEPKT_SHIFT;
#endif
		srrctl |= IXGBE_SRRCTL_DESCTYPE_HDR_SPLIT_ALWAYS;
	} else {
		srrctl |= ALIGN(rx_ring->rx_buf_len, 1024) >>
			  IXGBE_SRRCTL_BSIZEPKT_SHIFT;
		srrctl |= IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF;
	}

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_SRRCTL(index), srrctl);
}

static u32 ixgbe_setup_mrqc(struct ixgbe_adapter *adapter)
{
	u32 mrqc = 0;
	int mask;

	if (!(adapter->hw.mac.type == ixgbe_mac_82599EB))
		return mrqc;

	mask = adapter->flags & (IXGBE_FLAG_RSS_ENABLED
#ifdef CONFIG_IXGBE_DCB
				 | IXGBE_FLAG_DCB_ENABLED
#endif
				);

	switch (mask) {
	case (IXGBE_FLAG_RSS_ENABLED):
		mrqc = IXGBE_MRQC_RSSEN;
		break;
#ifdef CONFIG_IXGBE_DCB
	case (IXGBE_FLAG_DCB_ENABLED):
		mrqc = IXGBE_MRQC_RT8TCEN;
		break;
#endif  
	default:
		break;
	}

	return mrqc;
}

static void ixgbe_configure_rscctl(struct ixgbe_adapter *adapter, int index,
                                   int rx_buf_len)
{
	struct ixgbe_ring *rx_ring;
	struct ixgbe_hw *hw = &adapter->hw;
	int j;
	u32 rscctrl;

	rx_ring = &adapter->rx_ring[index];
	j = rx_ring->reg_idx;
	rscctrl = IXGBE_READ_REG(hw, IXGBE_RSCCTL(j));
	rscctrl |= IXGBE_RSCCTL_RSCEN;
	 
	if (rx_ring->flags & IXGBE_RING_RX_PS_ENABLED) {
#if (MAX_SKB_FRAGS > 16)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_16;
#elif (MAX_SKB_FRAGS > 8)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_8;
#elif (MAX_SKB_FRAGS > 4)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_4;
#else
		rscctrl |= IXGBE_RSCCTL_MAXDESC_1;
#endif
	} else {
		if (rx_buf_len < IXGBE_RXBUFFER_4096)
			rscctrl |= IXGBE_RSCCTL_MAXDESC_16;
		else if (rx_buf_len < IXGBE_RXBUFFER_8192)
			rscctrl |= IXGBE_RSCCTL_MAXDESC_8;
		else
			rscctrl |= IXGBE_RSCCTL_MAXDESC_4;
	}
	IXGBE_WRITE_REG(hw, IXGBE_RSCCTL(j), rscctrl);
}

static void ixgbe_configure_rx(struct ixgbe_adapter *adapter)
{
	u64 rdba;
	struct ixgbe_hw *hw = &adapter->hw;
	struct ixgbe_ring *rx_ring;
	struct net_device *netdev = adapter->netdev;
	int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN;
	int i, j;
	u32 rdlen, rxctrl, rxcsum;
	static const u32 seed[10] = { 0xE291D73D, 0x1805EC6C, 0x2A94B30D,
	                  0xA54F2BEC, 0xEA49AF7C, 0xE214AD3D, 0xB855AABE,
	                  0x6A3E67EA, 0x14364D17, 0x3BED200D};
	u32 fctrl, hlreg0;
	u32 reta = 0, mrqc = 0;
	u32 rdrxctl;
	int rx_buf_len;

	adapter->flags |= IXGBE_FLAG_RX_PS_ENABLED;

	if (adapter->flags & IXGBE_FLAG_RX_PS_ENABLED) {
		rx_buf_len = IXGBE_RX_HDR_SIZE;
		if (hw->mac.type == ixgbe_mac_82599EB) {
			 
			u32 psrtype = IXGBE_PSRTYPE_TCPHDR |
			              IXGBE_PSRTYPE_UDPHDR |
			              IXGBE_PSRTYPE_IPV4HDR |
			              IXGBE_PSRTYPE_IPV6HDR |
			              IXGBE_PSRTYPE_L2HDR;
			IXGBE_WRITE_REG(hw, IXGBE_PSRTYPE(0), psrtype);
		}
	} else {
		if (!(adapter->flags2 & IXGBE_FLAG2_RSC_ENABLED) &&
		    (netdev->mtu <= ETH_DATA_LEN))
			rx_buf_len = MAXIMUM_ETHERNET_VLAN_SIZE;
		else
			rx_buf_len = ALIGN(max_frame, 1024);
	}

	fctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_FCTRL);
	fctrl |= IXGBE_FCTRL_BAM;
	fctrl |= IXGBE_FCTRL_DPF;  
	fctrl |= IXGBE_FCTRL_PMCF;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_FCTRL, fctrl);

	hlreg0 = IXGBE_READ_REG(hw, IXGBE_HLREG0);
	if (adapter->netdev->mtu <= ETH_DATA_LEN)
		hlreg0 &= ~IXGBE_HLREG0_JUMBOEN;
	else
		hlreg0 |= IXGBE_HLREG0_JUMBOEN;
#ifdef IXGBE_FCOE
	if (netdev->features & NETIF_F_FCOE_MTU)
		hlreg0 |= IXGBE_HLREG0_JUMBOEN;
#endif
	IXGBE_WRITE_REG(hw, IXGBE_HLREG0, hlreg0);

	rdlen = adapter->rx_ring[0].count * sizeof(union ixgbe_adv_rx_desc);
	 
	rxctrl = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, rxctrl & ~IXGBE_RXCTRL_RXEN);

	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = &adapter->rx_ring[i];
		rdba = rx_ring->dma;
		j = rx_ring->reg_idx;
		IXGBE_WRITE_REG(hw, IXGBE_RDBAL(j), (rdba & DMA_BIT_MASK(32)));
		IXGBE_WRITE_REG(hw, IXGBE_RDBAH(j), (rdba >> 32));
		IXGBE_WRITE_REG(hw, IXGBE_RDLEN(j), rdlen);
		IXGBE_WRITE_REG(hw, IXGBE_RDH(j), 0);
		IXGBE_WRITE_REG(hw, IXGBE_RDT(j), 0);
		rx_ring->head = IXGBE_RDH(j);
		rx_ring->tail = IXGBE_RDT(j);
		rx_ring->rx_buf_len = rx_buf_len;

		if (adapter->flags & IXGBE_FLAG_RX_PS_ENABLED)
			rx_ring->flags |= IXGBE_RING_RX_PS_ENABLED;
		else
			rx_ring->flags &= ~IXGBE_RING_RX_PS_ENABLED;

#ifdef IXGBE_FCOE
		if (netdev->features & NETIF_F_FCOE_MTU) {
			struct ixgbe_ring_feature *f;
			f = &adapter->ring_feature[RING_F_FCOE];
			if ((i >= f->mask) && (i < f->mask + f->indices)) {
				rx_ring->flags &= ~IXGBE_RING_RX_PS_ENABLED;
				if (rx_buf_len < IXGBE_FCOE_JUMBO_FRAME_SIZE)
					rx_ring->rx_buf_len =
					        IXGBE_FCOE_JUMBO_FRAME_SIZE;
			}
		}

#endif  
		ixgbe_configure_srrctl(adapter, rx_ring);
	}

	if (hw->mac.type == ixgbe_mac_82598EB) {
		 
		rdrxctl = IXGBE_READ_REG(hw, IXGBE_RDRXCTL);
		rdrxctl |= IXGBE_RDRXCTL_MVMEN;
		IXGBE_WRITE_REG(hw, IXGBE_RDRXCTL, rdrxctl);
	}

	mrqc = ixgbe_setup_mrqc(adapter);

	if (adapter->flags & IXGBE_FLAG_RSS_ENABLED) {
		 
		for (i = 0, j = 0; i < 128; i++, j++) {
			if (j == adapter->ring_feature[RING_F_RSS].indices)
				j = 0;
			 
			reta = (reta << 8) | (j * 0x11);
			if ((i & 3) == 3)
				IXGBE_WRITE_REG(hw, IXGBE_RETA(i >> 2), reta);
		}

		for (i = 0; i < 10; i++)
			IXGBE_WRITE_REG(hw, IXGBE_RSSRK(i), seed[i]);

		if (hw->mac.type == ixgbe_mac_82598EB)
			mrqc |= IXGBE_MRQC_RSSEN;
		     
		mrqc |= IXGBE_MRQC_RSS_FIELD_IPV4
		      | IXGBE_MRQC_RSS_FIELD_IPV4_TCP
		      | IXGBE_MRQC_RSS_FIELD_IPV4_UDP
		      | IXGBE_MRQC_RSS_FIELD_IPV6
		      | IXGBE_MRQC_RSS_FIELD_IPV6_TCP
		      | IXGBE_MRQC_RSS_FIELD_IPV6_UDP;
	}
	IXGBE_WRITE_REG(hw, IXGBE_MRQC, mrqc);

	rxcsum = IXGBE_READ_REG(hw, IXGBE_RXCSUM);

	if (adapter->flags & IXGBE_FLAG_RSS_ENABLED ||
	    adapter->flags & IXGBE_FLAG_RX_CSUM_ENABLED) {
		 
		rxcsum |= IXGBE_RXCSUM_PCSD;
	}
	if (!(rxcsum & IXGBE_RXCSUM_PCSD)) {
		 
		rxcsum |= IXGBE_RXCSUM_IPPCSE;
	}

	IXGBE_WRITE_REG(hw, IXGBE_RXCSUM, rxcsum);

	if (hw->mac.type == ixgbe_mac_82599EB) {
		rdrxctl = IXGBE_READ_REG(hw, IXGBE_RDRXCTL);
		rdrxctl |= IXGBE_RDRXCTL_CRCSTRIP;
		rdrxctl &= ~IXGBE_RDRXCTL_RSCFRSTSIZE;
		IXGBE_WRITE_REG(hw, IXGBE_RDRXCTL, rdrxctl);
	}

	if (adapter->flags2 & IXGBE_FLAG2_RSC_ENABLED) {
		 
		for (i = 0; i < adapter->num_rx_queues; i++)
			ixgbe_configure_rscctl(adapter, i, rx_buf_len);

		IXGBE_WRITE_REG(hw, IXGBE_RSCDBU,
		   (IXGBE_RSCDBU_RSCACKDIS | IXGBE_READ_REG(hw, IXGBE_RSCDBU)));
	}
}

static void ixgbe_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;

	hw->mac.ops.set_vfta(&adapter->hw, vid, 0, true);
}

static void ixgbe_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;

	if (!test_bit(__IXGBE_DOWN, &adapter->state))
		ixgbe_irq_disable(adapter);

	vlan_group_set_device(adapter->vlgrp, vid, NULL);

	if (!test_bit(__IXGBE_DOWN, &adapter->state))
		ixgbe_irq_enable(adapter);

	hw->mac.ops.set_vfta(&adapter->hw, vid, 0, false);
}

static void ixgbe_vlan_rx_register(struct net_device *netdev,
                                   struct vlan_group *grp)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	u32 ctrl;
	int i, j;

	if (!test_bit(__IXGBE_DOWN, &adapter->state))
		ixgbe_irq_disable(adapter);
	adapter->vlgrp = grp;

	ctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_VLNCTRL);
	if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
		ctrl |= IXGBE_VLNCTRL_VME | IXGBE_VLNCTRL_VFE;
		ctrl &= ~IXGBE_VLNCTRL_CFIEN;
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_VLNCTRL, ctrl);
	} else if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
		ctrl |= IXGBE_VLNCTRL_VFE;
		 
		ctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_VLNCTRL);
		ctrl &= ~IXGBE_VLNCTRL_CFIEN;
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_VLNCTRL, ctrl);
		for (i = 0; i < adapter->num_rx_queues; i++) {
			j = adapter->rx_ring[i].reg_idx;
			ctrl = IXGBE_READ_REG(&adapter->hw, IXGBE_RXDCTL(j));
			ctrl |= IXGBE_RXDCTL_VME;
			IXGBE_WRITE_REG(&adapter->hw, IXGBE_RXDCTL(j), ctrl);
		}
	}
	ixgbe_vlan_rx_add_vid(netdev, 0);

	if (!test_bit(__IXGBE_DOWN, &adapter->state))
		ixgbe_irq_enable(adapter);
}

static void ixgbe_restore_vlan(struct ixgbe_adapter *adapter)
{
	ixgbe_vlan_rx_register(adapter->netdev, adapter->vlgrp);

	if (adapter->vlgrp) {
		u16 vid;
		for (vid = 0; vid < VLAN_GROUP_ARRAY_LEN; vid++) {
			if (!vlan_group_get_device(adapter->vlgrp, vid))
				continue;
			ixgbe_vlan_rx_add_vid(adapter->netdev, vid);
		}
	}
}

static u8 *ixgbe_addr_list_itr(struct ixgbe_hw *hw, u8 **mc_addr_ptr, u32 *vmdq)
{
	struct dev_mc_list *mc_ptr;
	u8 *addr = *mc_addr_ptr;
	*vmdq = 0;

	mc_ptr = container_of(addr, struct dev_mc_list, dmi_addr[0]);
	if (mc_ptr->next)
		*mc_addr_ptr = mc_ptr->next->dmi_addr;
	else
		*mc_addr_ptr = NULL;

	return addr;
}

static void ixgbe_set_rx_mode(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;
	u32 fctrl, vlnctrl;
	u8 *addr_list = NULL;
	int addr_count = 0;

	fctrl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
	vlnctrl = IXGBE_READ_REG(hw, IXGBE_VLNCTRL);

	if (netdev->flags & IFF_PROMISC) {
		hw->addr_ctrl.user_set_promisc = 1;
		fctrl |= (IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
		vlnctrl &= ~IXGBE_VLNCTRL_VFE;
	} else {
		if (netdev->flags & IFF_ALLMULTI) {
			fctrl |= IXGBE_FCTRL_MPE;
			fctrl &= ~IXGBE_FCTRL_UPE;
		} else {
			fctrl &= ~(IXGBE_FCTRL_UPE | IXGBE_FCTRL_MPE);
		}
		vlnctrl |= IXGBE_VLNCTRL_VFE;
		hw->addr_ctrl.user_set_promisc = 0;
	}

	IXGBE_WRITE_REG(hw, IXGBE_FCTRL, fctrl);
	IXGBE_WRITE_REG(hw, IXGBE_VLNCTRL, vlnctrl);

	hw->mac.ops.update_uc_addr_list(hw, &netdev->uc.list);

	addr_count = netdev->mc_count;
	if (addr_count)
		addr_list = netdev->mc_list->dmi_addr;
	hw->mac.ops.update_mc_addr_list(hw, addr_list, addr_count,
	                                ixgbe_addr_list_itr);
}

static void ixgbe_napi_enable_all(struct ixgbe_adapter *adapter)
{
	int q_idx;
	struct ixgbe_q_vector *q_vector;
	int q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;

	if (!(adapter->flags & IXGBE_FLAG_MSIX_ENABLED))
		q_vectors = 1;

	for (q_idx = 0; q_idx < q_vectors; q_idx++) {
		struct napi_struct *napi;
		q_vector = adapter->q_vector[q_idx];
		napi = &q_vector->napi;
		if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
			if (!q_vector->rxr_count || !q_vector->txr_count) {
				if (q_vector->txr_count == 1)
					napi->poll = &ixgbe_clean_txonly;
				else if (q_vector->rxr_count == 1)
					napi->poll = &ixgbe_clean_rxonly;
			}
		}

		napi_enable(napi);
	}
}

static void ixgbe_napi_disable_all(struct ixgbe_adapter *adapter)
{
	int q_idx;
	struct ixgbe_q_vector *q_vector;
	int q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;

	if (!(adapter->flags & IXGBE_FLAG_MSIX_ENABLED))
		q_vectors = 1;

	for (q_idx = 0; q_idx < q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
		napi_disable(&q_vector->napi);
	}
}

#ifdef CONFIG_IXGBE_DCB
 
static void ixgbe_configure_dcb(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 txdctl, vlnctrl;
	int i, j;

	ixgbe_dcb_check_config(&adapter->dcb_cfg);
	ixgbe_dcb_calculate_tc_credits(&adapter->dcb_cfg, DCB_TX_CONFIG);
	ixgbe_dcb_calculate_tc_credits(&adapter->dcb_cfg, DCB_RX_CONFIG);

	ixgbe_dcb_hw_config(&adapter->hw, &adapter->dcb_cfg);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		j = adapter->tx_ring[i].reg_idx;
		txdctl = IXGBE_READ_REG(hw, IXGBE_TXDCTL(j));
		 
		txdctl |= 32;
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(j), txdctl);
	}
	 
	vlnctrl = IXGBE_READ_REG(hw, IXGBE_VLNCTRL);
	if (hw->mac.type == ixgbe_mac_82598EB) {
		vlnctrl |= IXGBE_VLNCTRL_VME | IXGBE_VLNCTRL_VFE;
		vlnctrl &= ~IXGBE_VLNCTRL_CFIEN;
		IXGBE_WRITE_REG(hw, IXGBE_VLNCTRL, vlnctrl);
	} else if (hw->mac.type == ixgbe_mac_82599EB) {
		vlnctrl |= IXGBE_VLNCTRL_VFE;
		vlnctrl &= ~IXGBE_VLNCTRL_CFIEN;
		IXGBE_WRITE_REG(hw, IXGBE_VLNCTRL, vlnctrl);
		for (i = 0; i < adapter->num_rx_queues; i++) {
			j = adapter->rx_ring[i].reg_idx;
			vlnctrl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(j));
			vlnctrl |= IXGBE_RXDCTL_VME;
			IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(j), vlnctrl);
		}
	}
	hw->mac.ops.set_vfta(&adapter->hw, 0, 0, true);
}

#endif
static void ixgbe_configure(struct ixgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ixgbe_hw *hw = &adapter->hw;
	int i;

	ixgbe_set_rx_mode(netdev);

	ixgbe_restore_vlan(adapter);
#ifdef CONFIG_IXGBE_DCB
	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
		if (hw->mac.type == ixgbe_mac_82598EB)
			netif_set_gso_max_size(netdev, 32768);
		else
			netif_set_gso_max_size(netdev, 65536);
		ixgbe_configure_dcb(adapter);
	} else {
		netif_set_gso_max_size(netdev, 65536);
	}
#else
	netif_set_gso_max_size(netdev, 65536);
#endif

#ifdef IXGBE_FCOE
	if (adapter->flags & IXGBE_FLAG_FCOE_ENABLED)
		ixgbe_configure_fcoe(adapter);

#endif  
	if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i].atr_sample_rate =
			                               adapter->atr_sample_rate;
		ixgbe_init_fdir_signature_82599(hw, adapter->fdir_pballoc);
	} else if (adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE) {
		ixgbe_init_fdir_perfect_82599(hw, adapter->fdir_pballoc);
	}

	ixgbe_configure_tx(adapter);
	ixgbe_configure_rx(adapter);
	for (i = 0; i < adapter->num_rx_queues; i++)
		ixgbe_alloc_rx_buffers(adapter, &adapter->rx_ring[i],
		                       (adapter->rx_ring[i].count - 1));
}

static inline bool ixgbe_is_sfp(struct ixgbe_hw *hw)
{
	switch (hw->phy.type) {
	case ixgbe_phy_sfp_avago:
	case ixgbe_phy_sfp_ftl:
	case ixgbe_phy_sfp_intel:
	case ixgbe_phy_sfp_unknown:
	case ixgbe_phy_tw_tyco:
	case ixgbe_phy_tw_unknown:
		return true;
	default:
		return false;
	}
}

static void ixgbe_sfp_link_config(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;

		if (hw->phy.multispeed_fiber) {
			 
			hw->mac.ops.setup_sfp(hw);
			if (!(adapter->flags & IXGBE_FLAG_IN_SFP_LINK_TASK))
				schedule_work(&adapter->multispeed_fiber_task);
		} else {
			 
			if (!(adapter->flags & IXGBE_FLAG_IN_SFP_MOD_TASK))
				schedule_work(&adapter->sfp_config_module_task);
		}
}

static int ixgbe_non_sfp_link_config(struct ixgbe_hw *hw)
{
	u32 autoneg;
	bool negotiation, link_up = false;
	u32 ret = IXGBE_ERR_LINK_SETUP;

	if (hw->mac.ops.check_link)
		ret = hw->mac.ops.check_link(hw, &autoneg, &link_up, false);

	if (ret)
		goto link_cfg_out;

	if (hw->mac.ops.get_link_capabilities)
		ret = hw->mac.ops.get_link_capabilities(hw, &autoneg, &negotiation);
	if (ret)
		goto link_cfg_out;

	if (hw->mac.ops.setup_link)
		ret = hw->mac.ops.setup_link(hw, autoneg, negotiation, link_up);
link_cfg_out:
	return ret;
}

#define IXGBE_MAX_RX_DESC_POLL 10
static inline void ixgbe_rx_desc_queue_enable(struct ixgbe_adapter *adapter,
	                                      int rxr)
{
	int j = adapter->rx_ring[rxr].reg_idx;
	int k;

	for (k = 0; k < IXGBE_MAX_RX_DESC_POLL; k++) {
		if (IXGBE_READ_REG(&adapter->hw,
		                   IXGBE_RXDCTL(j)) & IXGBE_RXDCTL_ENABLE)
			break;
		else
			msleep(1);
	}
	if (k >= IXGBE_MAX_RX_DESC_POLL) {
		DPRINTK(DRV, ERR, "RXDCTL.ENABLE on Rx queue %d "
		        "not set within the polling period\n", rxr);
	}
	ixgbe_release_rx_desc(&adapter->hw, &adapter->rx_ring[rxr],
	                      (adapter->rx_ring[rxr].count - 1));
}

static int ixgbe_up_complete(struct ixgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ixgbe_hw *hw = &adapter->hw;
	int i, j = 0;
	int num_rx_rings = adapter->num_rx_queues;
	int err;
	int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN;
	u32 txdctl, rxdctl, mhadd;
	u32 dmatxctl;
	u32 gpie;

	ixgbe_get_hw_control(adapter);

	if ((adapter->flags & IXGBE_FLAG_MSIX_ENABLED) ||
	    (adapter->flags & IXGBE_FLAG_MSI_ENABLED)) {
		if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
			gpie = (IXGBE_GPIE_MSIX_MODE | IXGBE_GPIE_EIAME |
			        IXGBE_GPIE_PBA_SUPPORT | IXGBE_GPIE_OCD);
		} else {
			 
			gpie = 0;
		}
		 
		IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);
	}

	if (!(adapter->flags & IXGBE_FLAG_MSIX_ENABLED)) {
		 
		IXGBE_WRITE_REG(hw, IXGBE_EIAM, IXGBE_EICS_RTX_QUEUE);
	}

#ifdef MY_ABC_HERE
	 
	if (adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE) {
		gpie = IXGBE_READ_REG(hw, IXGBE_GPIE);
		gpie |= IXGBE_SDP0_GPIEN;
		IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);
	}

#endif  
	 
	if (adapter->flags & IXGBE_FLAG_FAN_FAIL_CAPABLE) {
		gpie = IXGBE_READ_REG(hw, IXGBE_GPIE);
		gpie |= IXGBE_SDP1_GPIEN;
		IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);
	}

	if (hw->mac.type == ixgbe_mac_82599EB) {
		gpie = IXGBE_READ_REG(hw, IXGBE_GPIE);
		gpie |= IXGBE_SDP1_GPIEN;
		gpie |= IXGBE_SDP2_GPIEN;
		IXGBE_WRITE_REG(hw, IXGBE_GPIE, gpie);
	}

#ifdef IXGBE_FCOE
	 
	if ((netdev->features & NETIF_F_FCOE_MTU) &&
	    (max_frame < IXGBE_FCOE_JUMBO_FRAME_SIZE))
		max_frame = IXGBE_FCOE_JUMBO_FRAME_SIZE;

#endif  
	mhadd = IXGBE_READ_REG(hw, IXGBE_MHADD);
	if (max_frame != (mhadd >> IXGBE_MHADD_MFS_SHIFT)) {
		mhadd &= ~IXGBE_MHADD_MFS_MASK;
		mhadd |= max_frame << IXGBE_MHADD_MFS_SHIFT;

		IXGBE_WRITE_REG(hw, IXGBE_MHADD, mhadd);
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		j = adapter->tx_ring[i].reg_idx;
		txdctl = IXGBE_READ_REG(hw, IXGBE_TXDCTL(j));
		 
		txdctl |= (8 << 16);
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(j), txdctl);
	}

	if (hw->mac.type == ixgbe_mac_82599EB) {
		 
		dmatxctl = IXGBE_READ_REG(hw, IXGBE_DMATXCTL);
		dmatxctl |= IXGBE_DMATXCTL_TE;
		IXGBE_WRITE_REG(hw, IXGBE_DMATXCTL, dmatxctl);
	}
	for (i = 0; i < adapter->num_tx_queues; i++) {
		j = adapter->tx_ring[i].reg_idx;
		txdctl = IXGBE_READ_REG(hw, IXGBE_TXDCTL(j));
		txdctl |= IXGBE_TXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(j), txdctl);
	}

	for (i = 0; i < num_rx_rings; i++) {
		j = adapter->rx_ring[i].reg_idx;
		rxdctl = IXGBE_READ_REG(hw, IXGBE_RXDCTL(j));
		 
		rxdctl |= 0x0020;
		rxdctl |= IXGBE_RXDCTL_ENABLE;
		IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(j), rxdctl);
		if (hw->mac.type == ixgbe_mac_82599EB)
			ixgbe_rx_desc_queue_enable(adapter, i);
	}
	 
	rxdctl = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	if (hw->mac.type == ixgbe_mac_82598EB)
		rxdctl |= (IXGBE_RXCTRL_DMBYPS | IXGBE_RXCTRL_RXEN);
	else
		rxdctl |= IXGBE_RXCTRL_RXEN;
	hw->mac.ops.enable_rx_dma(hw, rxdctl);

	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED)
		ixgbe_configure_msix(adapter);
	else
		ixgbe_configure_msi_and_legacy(adapter);

	clear_bit(__IXGBE_DOWN, &adapter->state);
	ixgbe_napi_enable_all(adapter);

	IXGBE_READ_REG(hw, IXGBE_EICR);

	ixgbe_irq_enable(adapter);

	if (adapter->flags & IXGBE_FLAG_FAN_FAIL_CAPABLE) {
		u32 esdp = IXGBE_READ_REG(hw, IXGBE_ESDP);
		if (esdp & IXGBE_ESDP_SDP1)
			DPRINTK(DRV, CRIT,
				"Fan has stopped, replace the adapter\n");
	}

	if (hw->phy.type == ixgbe_phy_unknown) {
		err = hw->phy.ops.identify(hw);
		if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
			 
			ixgbe_down(adapter);
			schedule_work(&adapter->sfp_config_module_task);
			return err;
		}
	}

	if (ixgbe_is_sfp(hw)) {
		ixgbe_sfp_link_config(adapter);
	} else {
		err = ixgbe_non_sfp_link_config(hw);
		if (err)
			DPRINTK(PROBE, ERR, "link_config FAILED %d\n", err);
	}

	for (i = 0; i < adapter->num_tx_queues; i++)
		set_bit(__IXGBE_FDIR_INIT_DONE,
		        &(adapter->tx_ring[i].reinit_state));

	netif_tx_start_all_queues(netdev);

	adapter->flags |= IXGBE_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	mod_timer(&adapter->watchdog_timer, jiffies);
	return 0;
}

void ixgbe_reinit_locked(struct ixgbe_adapter *adapter)
{
	WARN_ON(in_interrupt());
	while (test_and_set_bit(__IXGBE_RESETTING, &adapter->state))
		msleep(1);
	ixgbe_down(adapter);
	ixgbe_up(adapter);
	clear_bit(__IXGBE_RESETTING, &adapter->state);
}

int ixgbe_up(struct ixgbe_adapter *adapter)
{
	 
	ixgbe_configure(adapter);

	return ixgbe_up_complete(adapter);
}

void ixgbe_reset(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	int err;

	err = hw->mac.ops.init_hw(hw);
	switch (err) {
	case 0:
	case IXGBE_ERR_SFP_NOT_PRESENT:
		break;
	case IXGBE_ERR_MASTER_REQUESTS_PENDING:
		dev_err(&adapter->pdev->dev, "master disable timed out\n");
		break;
	case IXGBE_ERR_EEPROM_VERSION:
		 
		dev_warn(&adapter->pdev->dev, "This device is a pre-production "
		         "adapter/LOM.  Please be aware there may be issues "
		         "associated with your hardware.  If you are "
		         "experiencing problems please contact your Intel or "
		         "hardware representative who provided you with this "
		         "hardware.\n");
		break;
	default:
		dev_err(&adapter->pdev->dev, "Hardware Error: %d\n", err);
	}

	hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);
}

static void ixgbe_clean_rx_ring(struct ixgbe_adapter *adapter,
                                struct ixgbe_ring *rx_ring)
{
	struct pci_dev *pdev = adapter->pdev;
	unsigned long size;
	unsigned int i;

	for (i = 0; i < rx_ring->count; i++) {
		struct ixgbe_rx_buffer *rx_buffer_info;

		rx_buffer_info = &rx_ring->rx_buffer_info[i];
		if (rx_buffer_info->dma) {
			pci_unmap_single(pdev, rx_buffer_info->dma,
			                 rx_ring->rx_buf_len,
			                 PCI_DMA_FROMDEVICE);
			rx_buffer_info->dma = 0;
		}
		if (rx_buffer_info->skb) {
			struct sk_buff *skb = rx_buffer_info->skb;
			rx_buffer_info->skb = NULL;
			do {
				struct sk_buff *this = skb;
				skb = skb->prev;
				dev_kfree_skb(this);
			} while (skb);
		}
		if (!rx_buffer_info->page)
			continue;
		if (rx_buffer_info->page_dma) {
			pci_unmap_page(pdev, rx_buffer_info->page_dma,
			               PAGE_SIZE / 2, PCI_DMA_FROMDEVICE);
			rx_buffer_info->page_dma = 0;
		}
		put_page(rx_buffer_info->page);
		rx_buffer_info->page = NULL;
		rx_buffer_info->page_offset = 0;
	}

	size = sizeof(struct ixgbe_rx_buffer) * rx_ring->count;
	memset(rx_ring->rx_buffer_info, 0, size);

	memset(rx_ring->desc, 0, rx_ring->size);

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;

	if (rx_ring->head)
		writel(0, adapter->hw.hw_addr + rx_ring->head);
	if (rx_ring->tail)
		writel(0, adapter->hw.hw_addr + rx_ring->tail);
}

static void ixgbe_clean_tx_ring(struct ixgbe_adapter *adapter,
                                struct ixgbe_ring *tx_ring)
{
	struct ixgbe_tx_buffer *tx_buffer_info;
	unsigned long size;
	unsigned int i;

	for (i = 0; i < tx_ring->count; i++) {
		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		ixgbe_unmap_and_free_tx_resource(adapter, tx_buffer_info);
	}

	size = sizeof(struct ixgbe_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);

	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;

	if (tx_ring->head)
		writel(0, adapter->hw.hw_addr + tx_ring->head);
	if (tx_ring->tail)
		writel(0, adapter->hw.hw_addr + tx_ring->tail);
}

static void ixgbe_clean_all_rx_rings(struct ixgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		ixgbe_clean_rx_ring(adapter, &adapter->rx_ring[i]);
}

static void ixgbe_clean_all_tx_rings(struct ixgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		ixgbe_clean_tx_ring(adapter, &adapter->tx_ring[i]);
}

void ixgbe_down(struct ixgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 rxctrl;
	u32 txdctl;
	int i, j;

	set_bit(__IXGBE_DOWN, &adapter->state);

	rxctrl = IXGBE_READ_REG(hw, IXGBE_RXCTRL);
	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, rxctrl & ~IXGBE_RXCTRL_RXEN);

	netif_tx_disable(netdev);

	IXGBE_WRITE_FLUSH(hw);
	msleep(10);

	netif_tx_stop_all_queues(netdev);

	ixgbe_irq_disable(adapter);

	ixgbe_napi_disable_all(adapter);

	clear_bit(__IXGBE_SFP_MODULE_NOT_FOUND, &adapter->state);
	del_timer_sync(&adapter->sfp_timer);
	del_timer_sync(&adapter->watchdog_timer);
	cancel_work_sync(&adapter->watchdog_task);

	if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE ||
	    adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE)
		cancel_work_sync(&adapter->fdir_reinit_task);

#ifdef MY_ABC_HERE
	if (adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE)
		cancel_work_sync(&adapter->check_overtemp_task);

#endif  
	 
	for (i = 0; i < adapter->num_tx_queues; i++) {
		j = adapter->tx_ring[i].reg_idx;
		txdctl = IXGBE_READ_REG(hw, IXGBE_TXDCTL(j));
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(j),
		                (txdctl & ~IXGBE_TXDCTL_ENABLE));
	}
	 
	if (hw->mac.type == ixgbe_mac_82599EB)
		IXGBE_WRITE_REG(hw, IXGBE_DMATXCTL,
		                (IXGBE_READ_REG(hw, IXGBE_DMATXCTL) &
		                 ~IXGBE_DMATXCTL_TE));

	netif_carrier_off(netdev);

	if (!pci_channel_offline(adapter->pdev))
		ixgbe_reset(adapter);
	ixgbe_clean_all_tx_rings(adapter);
	ixgbe_clean_all_rx_rings(adapter);

#ifdef CONFIG_IXGBE_DCA
	 
	ixgbe_setup_dca(adapter);
#endif
}

static int ixgbe_poll(struct napi_struct *napi, int budget)
{
	struct ixgbe_q_vector *q_vector =
	                        container_of(napi, struct ixgbe_q_vector, napi);
	struct ixgbe_adapter *adapter = q_vector->adapter;
	int tx_clean_complete, work_done = 0;

#ifdef CONFIG_IXGBE_DCA
	if (adapter->flags & IXGBE_FLAG_DCA_ENABLED) {
		ixgbe_update_tx_dca(adapter, adapter->tx_ring);
		ixgbe_update_rx_dca(adapter, adapter->rx_ring);
	}
#endif

	tx_clean_complete = ixgbe_clean_tx_irq(q_vector, adapter->tx_ring);
	ixgbe_clean_rx_irq(q_vector, adapter->rx_ring, &work_done, budget);

	if (!tx_clean_complete)
		work_done = budget;

	if (work_done < budget) {
		napi_complete(napi);
		if (adapter->rx_itr_setting & 1)
			ixgbe_set_itr(adapter);
		if (!test_bit(__IXGBE_DOWN, &adapter->state))
			ixgbe_irq_enable_queues(adapter, IXGBE_EIMS_RTX_QUEUE);
	}
	return work_done;
}

static void ixgbe_tx_timeout(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	schedule_work(&adapter->reset_task);
}

static void ixgbe_reset_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter;
	adapter = container_of(work, struct ixgbe_adapter, reset_task);

	if (test_bit(__IXGBE_DOWN, &adapter->state) ||
	    test_bit(__IXGBE_RESETTING, &adapter->state))
		return;

	adapter->tx_timeout_count++;

	ixgbe_reinit_locked(adapter);
}

#ifdef CONFIG_IXGBE_DCB
static inline bool ixgbe_set_dcb_queues(struct ixgbe_adapter *adapter)
{
	bool ret = false;
	struct ixgbe_ring_feature *f = &adapter->ring_feature[RING_F_DCB];

	if (!(adapter->flags & IXGBE_FLAG_DCB_ENABLED))
		return ret;

	f->mask = 0x7 << 3;
	adapter->num_rx_queues = f->indices;
	adapter->num_tx_queues = f->indices;
	ret = true;

	return ret;
}
#endif

static inline bool ixgbe_set_rss_queues(struct ixgbe_adapter *adapter)
{
	bool ret = false;
	struct ixgbe_ring_feature *f = &adapter->ring_feature[RING_F_RSS];

	if (adapter->flags & IXGBE_FLAG_RSS_ENABLED) {
		f->mask = 0xF;
		adapter->num_rx_queues = f->indices;
		adapter->num_tx_queues = f->indices;
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

static bool inline ixgbe_set_fdir_queues(struct ixgbe_adapter *adapter)
{
	bool ret = false;
	struct ixgbe_ring_feature *f_fdir = &adapter->ring_feature[RING_F_FDIR];

	f_fdir->indices = min((int)num_online_cpus(), f_fdir->indices);
	f_fdir->mask = 0;

	if (adapter->flags & IXGBE_FLAG_RSS_ENABLED &&
	    ((adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE ||
	     (adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE)))) {
		adapter->num_tx_queues = f_fdir->indices;
		adapter->num_rx_queues = f_fdir->indices;
		ret = true;
	} else {
		adapter->flags &= ~IXGBE_FLAG_FDIR_HASH_CAPABLE;
		adapter->flags &= ~IXGBE_FLAG_FDIR_PERFECT_CAPABLE;
	}
	return ret;
}

#ifdef IXGBE_FCOE
 
static inline bool ixgbe_set_fcoe_queues(struct ixgbe_adapter *adapter)
{
	bool ret = false;
	struct ixgbe_ring_feature *f = &adapter->ring_feature[RING_F_FCOE];

	f->indices = min((int)num_online_cpus(), f->indices);
	if (adapter->flags & IXGBE_FLAG_FCOE_ENABLED) {
		adapter->num_rx_queues = 1;
		adapter->num_tx_queues = 1;
#ifdef CONFIG_IXGBE_DCB
		if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
			DPRINTK(PROBE, INFO, "FCoE enabled with DCB \n");
			ixgbe_set_dcb_queues(adapter);
		}
#endif
		if (adapter->flags & IXGBE_FLAG_RSS_ENABLED) {
			DPRINTK(PROBE, INFO, "FCoE enabled with RSS \n");
			if ((adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE) ||
			    (adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE))
				ixgbe_set_fdir_queues(adapter);
			else
				ixgbe_set_rss_queues(adapter);
		}
		 
		f->mask = adapter->num_rx_queues;
		adapter->num_rx_queues += f->indices;
		adapter->num_tx_queues += f->indices;

		ret = true;
	}

	return ret;
}

#endif  
 
static void ixgbe_set_num_queues(struct ixgbe_adapter *adapter)
{
#ifdef IXGBE_FCOE
	if (ixgbe_set_fcoe_queues(adapter))
		goto done;

#endif  
#ifdef CONFIG_IXGBE_DCB
	if (ixgbe_set_dcb_queues(adapter))
		goto done;

#endif
	if (ixgbe_set_fdir_queues(adapter))
		goto done;

	if (ixgbe_set_rss_queues(adapter))
		goto done;

	adapter->num_rx_queues = 1;
	adapter->num_tx_queues = 1;

done:
	 
	adapter->netdev->real_num_tx_queues = adapter->num_tx_queues;
}

static void ixgbe_acquire_msix_vectors(struct ixgbe_adapter *adapter,
                                       int vectors)
{
	int err, vector_threshold;

	vector_threshold = MIN_MSIX_COUNT;

	while (vectors >= vector_threshold) {
		err = pci_enable_msix(adapter->pdev, adapter->msix_entries,
		                      vectors);
		if (!err)  
			break;
		else if (err < 0)
			vectors = 0;  
		else  
			vectors = err;
	}

	if (vectors < vector_threshold) {
		 
		DPRINTK(HW, DEBUG, "Unable to allocate MSI-X interrupts\n");
		adapter->flags &= ~IXGBE_FLAG_MSIX_ENABLED;
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	} else {
		adapter->flags |= IXGBE_FLAG_MSIX_ENABLED;  
		 
		adapter->num_msix_vectors = min(vectors,
		                   adapter->max_msix_q_vectors + NON_Q_VECTORS);
	}
}

static inline bool ixgbe_cache_ring_rss(struct ixgbe_adapter *adapter)
{
	int i;
	bool ret = false;

	if (adapter->flags & IXGBE_FLAG_RSS_ENABLED) {
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i].reg_idx = i;
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i].reg_idx = i;
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

#ifdef CONFIG_IXGBE_DCB
 
static inline bool ixgbe_cache_ring_dcb(struct ixgbe_adapter *adapter)
{
	int i;
	bool ret = false;
	int dcb_i = adapter->ring_feature[RING_F_DCB].indices;

	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
		if (adapter->hw.mac.type == ixgbe_mac_82598EB) {
			 
			for (i = 0; i < dcb_i; i++) {
				adapter->rx_ring[i].reg_idx = i << 3;
				adapter->tx_ring[i].reg_idx = i << 2;
			}
			ret = true;
		} else if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
			if (dcb_i == 8) {
				 
				for (i = 0; i < 3; i++) {
					adapter->tx_ring[i].reg_idx = i << 5;
					adapter->rx_ring[i].reg_idx = i << 4;
				}
				for ( ; i < 5; i++) {
					adapter->tx_ring[i].reg_idx =
					                         ((i + 2) << 4);
					adapter->rx_ring[i].reg_idx = i << 4;
				}
				for ( ; i < dcb_i; i++) {
					adapter->tx_ring[i].reg_idx =
					                         ((i + 8) << 3);
					adapter->rx_ring[i].reg_idx = i << 4;
				}

				ret = true;
			} else if (dcb_i == 4) {
				 
				adapter->tx_ring[0].reg_idx = 0;
				adapter->tx_ring[1].reg_idx = 64;
				adapter->tx_ring[2].reg_idx = 96;
				adapter->tx_ring[3].reg_idx = 112;
				for (i = 0 ; i < dcb_i; i++)
					adapter->rx_ring[i].reg_idx = i << 5;

				ret = true;
			} else {
				ret = false;
			}
		} else {
			ret = false;
		}
	} else {
		ret = false;
	}

	return ret;
}
#endif

static bool inline ixgbe_cache_ring_fdir(struct ixgbe_adapter *adapter)
{
	int i;
	bool ret = false;

	if (adapter->flags & IXGBE_FLAG_RSS_ENABLED &&
	    ((adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE) ||
	     (adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE))) {
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i].reg_idx = i;
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i].reg_idx = i;
		ret = true;
	}

	return ret;
}

#ifdef IXGBE_FCOE
 
static inline bool ixgbe_cache_ring_fcoe(struct ixgbe_adapter *adapter)
{
	int i, fcoe_rx_i = 0, fcoe_tx_i = 0;
	bool ret = false;
	struct ixgbe_ring_feature *f = &adapter->ring_feature[RING_F_FCOE];

	if (adapter->flags & IXGBE_FLAG_FCOE_ENABLED) {
#ifdef CONFIG_IXGBE_DCB
		if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
			struct ixgbe_fcoe *fcoe = &adapter->fcoe;

			ixgbe_cache_ring_dcb(adapter);
			 
			fcoe_rx_i = adapter->rx_ring[fcoe->tc].reg_idx + 1;
			fcoe_tx_i = adapter->tx_ring[fcoe->tc].reg_idx + 1;
			 
			if ((f->indices == IXGBE_FCRETA_SIZE) && (fcoe->tc > 3))
				fcoe_tx_i--;
		}
#endif  
		if (adapter->flags & IXGBE_FLAG_RSS_ENABLED) {
			if ((adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE) ||
			    (adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE))
				ixgbe_cache_ring_fdir(adapter);
			else
				ixgbe_cache_ring_rss(adapter);

			fcoe_rx_i = f->mask;
			fcoe_tx_i = f->mask;
		}
		for (i = 0; i < f->indices; i++, fcoe_rx_i++, fcoe_tx_i++) {
			adapter->rx_ring[f->mask + i].reg_idx = fcoe_rx_i;
			adapter->tx_ring[f->mask + i].reg_idx = fcoe_tx_i;
		}
		ret = true;
	}
	return ret;
}

#endif  
 
static void ixgbe_cache_ring_register(struct ixgbe_adapter *adapter)
{
	 
	adapter->rx_ring[0].reg_idx = 0;
	adapter->tx_ring[0].reg_idx = 0;

#ifdef IXGBE_FCOE
	if (ixgbe_cache_ring_fcoe(adapter))
		return;

#endif  
#ifdef CONFIG_IXGBE_DCB
	if (ixgbe_cache_ring_dcb(adapter))
		return;

#endif
	if (ixgbe_cache_ring_fdir(adapter))
		return;

	if (ixgbe_cache_ring_rss(adapter))
		return;
}

static int ixgbe_alloc_queues(struct ixgbe_adapter *adapter)
{
	int i;

	adapter->tx_ring = kcalloc(adapter->num_tx_queues,
	                           sizeof(struct ixgbe_ring), GFP_KERNEL);
	if (!adapter->tx_ring)
		goto err_tx_ring_allocation;

	adapter->rx_ring = kcalloc(adapter->num_rx_queues,
	                           sizeof(struct ixgbe_ring), GFP_KERNEL);
	if (!adapter->rx_ring)
		goto err_rx_ring_allocation;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		adapter->tx_ring[i].count = adapter->tx_ring_count;
		adapter->tx_ring[i].queue_index = i;
	}

	for (i = 0; i < adapter->num_rx_queues; i++) {
		adapter->rx_ring[i].count = adapter->rx_ring_count;
		adapter->rx_ring[i].queue_index = i;
	}

	ixgbe_cache_ring_register(adapter);

	return 0;

err_rx_ring_allocation:
	kfree(adapter->tx_ring);
err_tx_ring_allocation:
	return -ENOMEM;
}

static int ixgbe_set_interrupt_capability(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	int err = 0;
	int vector, v_budget;

	v_budget = min(adapter->num_rx_queues + adapter->num_tx_queues,
	               (int)(num_online_cpus() * 2)) + NON_Q_VECTORS;

	v_budget = min(v_budget, (int)hw->mac.max_msix_vectors);

	adapter->msix_entries = kcalloc(v_budget,
	                                sizeof(struct msix_entry), GFP_KERNEL);
	if (adapter->msix_entries) {
		for (vector = 0; vector < v_budget; vector++)
			adapter->msix_entries[vector].entry = vector;

		ixgbe_acquire_msix_vectors(adapter, v_budget);

		if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED)
			goto out;
	}

	adapter->flags &= ~IXGBE_FLAG_DCB_ENABLED;
	adapter->flags &= ~IXGBE_FLAG_RSS_ENABLED;
	adapter->flags &= ~IXGBE_FLAG_FDIR_HASH_CAPABLE;
	adapter->flags &= ~IXGBE_FLAG_FDIR_PERFECT_CAPABLE;
	adapter->atr_sample_rate = 0;
	ixgbe_set_num_queues(adapter);

	err = pci_enable_msi(adapter->pdev);
	if (!err) {
		adapter->flags |= IXGBE_FLAG_MSI_ENABLED;
	} else {
		DPRINTK(HW, DEBUG, "Unable to allocate MSI interrupt, "
		        "falling back to legacy.  Error: %d\n", err);
		 
		err = 0;
	}

out:
	return err;
}

static int ixgbe_alloc_q_vectors(struct ixgbe_adapter *adapter)
{
	int q_idx, num_q_vectors;
	struct ixgbe_q_vector *q_vector;
	int napi_vectors;
	int (*poll)(struct napi_struct *, int);

	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
		num_q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;
		napi_vectors = adapter->num_rx_queues;
		poll = &ixgbe_clean_rxtx_many;
	} else {
		num_q_vectors = 1;
		napi_vectors = 1;
		poll = &ixgbe_poll;
	}

	for (q_idx = 0; q_idx < num_q_vectors; q_idx++) {
		q_vector = kzalloc(sizeof(struct ixgbe_q_vector), GFP_KERNEL);
		if (!q_vector)
			goto err_out;
		q_vector->adapter = adapter;
		if (q_vector->txr_count && !q_vector->rxr_count)
			q_vector->eitr = adapter->tx_eitr_param;
		else
			q_vector->eitr = adapter->rx_eitr_param;
		q_vector->v_idx = q_idx;
		netif_napi_add(adapter->netdev, &q_vector->napi, (*poll), 64);
		adapter->q_vector[q_idx] = q_vector;
	}

	return 0;

err_out:
	while (q_idx) {
		q_idx--;
		q_vector = adapter->q_vector[q_idx];
		netif_napi_del(&q_vector->napi);
		kfree(q_vector);
		adapter->q_vector[q_idx] = NULL;
	}
	return -ENOMEM;
}

static void ixgbe_free_q_vectors(struct ixgbe_adapter *adapter)
{
	int q_idx, num_q_vectors;

	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED)
		num_q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;
	else
		num_q_vectors = 1;

	for (q_idx = 0; q_idx < num_q_vectors; q_idx++) {
		struct ixgbe_q_vector *q_vector = adapter->q_vector[q_idx];
		adapter->q_vector[q_idx] = NULL;
		netif_napi_del(&q_vector->napi);
		kfree(q_vector);
	}
}

static void ixgbe_reset_interrupt_capability(struct ixgbe_adapter *adapter)
{
	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
		adapter->flags &= ~IXGBE_FLAG_MSIX_ENABLED;
		pci_disable_msix(adapter->pdev);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	} else if (adapter->flags & IXGBE_FLAG_MSI_ENABLED) {
		adapter->flags &= ~IXGBE_FLAG_MSI_ENABLED;
		pci_disable_msi(adapter->pdev);
	}
	return;
}

int ixgbe_init_interrupt_scheme(struct ixgbe_adapter *adapter)
{
	int err;

	ixgbe_set_num_queues(adapter);

	err = ixgbe_set_interrupt_capability(adapter);
	if (err) {
		DPRINTK(PROBE, ERR, "Unable to setup interrupt capabilities\n");
		goto err_set_interrupt;
	}

	err = ixgbe_alloc_q_vectors(adapter);
	if (err) {
		DPRINTK(PROBE, ERR, "Unable to allocate memory for queue "
		        "vectors\n");
		goto err_alloc_q_vectors;
	}

	err = ixgbe_alloc_queues(adapter);
	if (err) {
		DPRINTK(PROBE, ERR, "Unable to allocate memory for queues\n");
		goto err_alloc_queues;
	}

	DPRINTK(DRV, INFO, "Multiqueue %s: Rx Queue count = %u, "
	        "Tx Queue count = %u\n",
	        (adapter->num_rx_queues > 1) ? "Enabled" :
	        "Disabled", adapter->num_rx_queues, adapter->num_tx_queues);

	set_bit(__IXGBE_DOWN, &adapter->state);

	return 0;

err_alloc_queues:
	ixgbe_free_q_vectors(adapter);
err_alloc_q_vectors:
	ixgbe_reset_interrupt_capability(adapter);
err_set_interrupt:
	return err;
}

void ixgbe_clear_interrupt_scheme(struct ixgbe_adapter *adapter)
{
	kfree(adapter->tx_ring);
	kfree(adapter->rx_ring);
	adapter->tx_ring = NULL;
	adapter->rx_ring = NULL;

	ixgbe_free_q_vectors(adapter);
	ixgbe_reset_interrupt_capability(adapter);
}

static void ixgbe_sfp_timer(unsigned long data)
{
	struct ixgbe_adapter *adapter = (struct ixgbe_adapter *)data;

	schedule_work(&adapter->sfp_task);
}

static void ixgbe_sfp_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter = container_of(work,
	                                             struct ixgbe_adapter,
	                                             sfp_task);
	struct ixgbe_hw *hw = &adapter->hw;

	if ((hw->phy.type == ixgbe_phy_nl) &&
	    (hw->phy.sfp_type == ixgbe_sfp_type_not_present)) {
		s32 ret = hw->phy.ops.identify_sfp(hw);
		if (ret == IXGBE_ERR_SFP_NOT_PRESENT)
			goto reschedule;
		ret = hw->phy.ops.reset(hw);
		if (ret == IXGBE_ERR_SFP_NOT_SUPPORTED) {
			dev_err(&adapter->pdev->dev, "failed to initialize "
				"because an unsupported SFP+ module type "
				"was detected.\n"
				"Reload the driver after installing a "
				"supported module.\n");
			unregister_netdev(adapter->netdev);
		} else {
			DPRINTK(PROBE, INFO, "detected SFP+: %d\n",
			        hw->phy.sfp_type);
		}
		 
		clear_bit(__IXGBE_SFP_MODULE_NOT_FOUND, &adapter->state);
	}
	return;
reschedule:
	if (test_bit(__IXGBE_SFP_MODULE_NOT_FOUND, &adapter->state))
		mod_timer(&adapter->sfp_timer,
		          round_jiffies(jiffies + (2 * HZ)));
}

static int __devinit ixgbe_sw_init(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	unsigned int rss;
#ifdef CONFIG_IXGBE_DCB
	int j;
	struct tc_configuration *tc;
#endif

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	hw->revision_id = pdev->revision;
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

	rss = min(IXGBE_MAX_RSS_INDICES, (int)num_online_cpus());
	adapter->ring_feature[RING_F_RSS].indices = rss;
	adapter->flags |= IXGBE_FLAG_RSS_ENABLED;
	adapter->ring_feature[RING_F_DCB].indices = IXGBE_MAX_DCB_INDICES;
	if (hw->mac.type == ixgbe_mac_82598EB) {
		if (hw->device_id == IXGBE_DEV_ID_82598AT)
			adapter->flags |= IXGBE_FLAG_FAN_FAIL_CAPABLE;
		adapter->max_msix_q_vectors = MAX_MSIX_Q_VECTORS_82598;
	} else if (hw->mac.type == ixgbe_mac_82599EB) {
		adapter->max_msix_q_vectors = MAX_MSIX_Q_VECTORS_82599;
		adapter->flags2 |= IXGBE_FLAG2_RSC_CAPABLE;
		adapter->flags2 |= IXGBE_FLAG2_RSC_ENABLED;
#ifdef MY_ABC_HERE
		if (hw->device_id == IXGBE_DEV_ID_82599_T3_LOM)
			adapter->flags2 |= IXGBE_FLAG2_TEMP_SENSOR_CAPABLE;
#endif  
		adapter->flags |= IXGBE_FLAG_FDIR_HASH_CAPABLE;
		adapter->ring_feature[RING_F_FDIR].indices =
		                                         IXGBE_MAX_FDIR_INDICES;
		adapter->atr_sample_rate = 20;
		adapter->fdir_pballoc = 0;
#ifdef IXGBE_FCOE
		adapter->flags |= IXGBE_FLAG_FCOE_CAPABLE;
		adapter->flags &= ~IXGBE_FLAG_FCOE_ENABLED;
		adapter->ring_feature[RING_F_FCOE].indices = 0;
		 
		adapter->fcoe.tc = IXGBE_FCOE_DEFTC;
#endif  
	}

#ifdef CONFIG_IXGBE_DCB
	 
	for (j = 0; j < MAX_TRAFFIC_CLASS; j++) {
		tc = &adapter->dcb_cfg.tc_config[j];
		tc->path[DCB_TX_CONFIG].bwg_id = 0;
		tc->path[DCB_TX_CONFIG].bwg_percent = 12 + (j & 1);
		tc->path[DCB_RX_CONFIG].bwg_id = 0;
		tc->path[DCB_RX_CONFIG].bwg_percent = 12 + (j & 1);
		tc->dcb_pfc = pfc_disabled;
	}
	adapter->dcb_cfg.bw_percentage[DCB_TX_CONFIG][0] = 100;
	adapter->dcb_cfg.bw_percentage[DCB_RX_CONFIG][0] = 100;
	adapter->dcb_cfg.rx_pba_cfg = pba_equal;
	adapter->dcb_cfg.pfc_mode_enable = false;
	adapter->dcb_cfg.round_robin_enable = false;
	adapter->dcb_set_bitmap = 0x00;
	ixgbe_copy_dcb_cfg(&adapter->dcb_cfg, &adapter->temp_dcb_cfg,
	                   adapter->ring_feature[RING_F_DCB].indices);

#endif

	hw->fc.requested_mode = ixgbe_fc_full;
	hw->fc.current_mode = ixgbe_fc_full;	 
#ifdef CONFIG_DCB
	adapter->last_lfc_mode = hw->fc.current_mode;
#endif
	hw->fc.high_water = IXGBE_DEFAULT_FCRTH;
	hw->fc.low_water = IXGBE_DEFAULT_FCRTL;
	hw->fc.pause_time = IXGBE_DEFAULT_FCPAUSE;
	hw->fc.send_xon = true;
	hw->fc.disable_fc_autoneg = false;

	adapter->rx_itr_setting = 1;
	adapter->rx_eitr_param = 20000;
	adapter->tx_itr_setting = 1;
	adapter->tx_eitr_param = 10000;

	adapter->eitr_low = 10;
	adapter->eitr_high = 20;

	adapter->tx_ring_count = IXGBE_DEFAULT_TXD;
	adapter->rx_ring_count = IXGBE_DEFAULT_RXD;

	if (ixgbe_init_eeprom_params_generic(hw)) {
		dev_err(&pdev->dev, "EEPROM initialization failed\n");
		return -EIO;
	}

	adapter->flags |= IXGBE_FLAG_RX_CSUM_ENABLED;

	set_bit(__IXGBE_DOWN, &adapter->state);

	return 0;
}

int ixgbe_setup_tx_resources(struct ixgbe_adapter *adapter,
                             struct ixgbe_ring *tx_ring)
{
	struct pci_dev *pdev = adapter->pdev;
	int size;

	size = sizeof(struct ixgbe_tx_buffer) * tx_ring->count;
	tx_ring->tx_buffer_info = vmalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err;
	memset(tx_ring->tx_buffer_info, 0, size);

	tx_ring->size = tx_ring->count * sizeof(union ixgbe_adv_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	tx_ring->desc = pci_alloc_consistent(pdev, tx_ring->size,
	                                     &tx_ring->dma);
	if (!tx_ring->desc)
		goto err;

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
	tx_ring->work_limit = tx_ring->count;
	return 0;

err:
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	DPRINTK(PROBE, ERR, "Unable to allocate memory for the transmit "
	                    "descriptor ring\n");
	return -ENOMEM;
}

static int ixgbe_setup_all_tx_resources(struct ixgbe_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		err = ixgbe_setup_tx_resources(adapter, &adapter->tx_ring[i]);
		if (!err)
			continue;
		DPRINTK(PROBE, ERR, "Allocation for Tx Queue %u failed\n", i);
		break;
	}

	return err;
}

int ixgbe_setup_rx_resources(struct ixgbe_adapter *adapter,
                             struct ixgbe_ring *rx_ring)
{
	struct pci_dev *pdev = adapter->pdev;
	int size;

	size = sizeof(struct ixgbe_rx_buffer) * rx_ring->count;
	rx_ring->rx_buffer_info = vmalloc(size);
	if (!rx_ring->rx_buffer_info) {
		DPRINTK(PROBE, ERR,
		        "vmalloc allocation failed for the rx desc ring\n");
		goto alloc_failed;
	}
	memset(rx_ring->rx_buffer_info, 0, size);

	rx_ring->size = rx_ring->count * sizeof(union ixgbe_adv_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	rx_ring->desc = pci_alloc_consistent(pdev, rx_ring->size, &rx_ring->dma);

	if (!rx_ring->desc) {
		DPRINTK(PROBE, ERR,
		        "Memory allocation failed for the rx desc ring\n");
		vfree(rx_ring->rx_buffer_info);
		goto alloc_failed;
	}

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;

	return 0;

alloc_failed:
	return -ENOMEM;
}

static int ixgbe_setup_all_rx_resources(struct ixgbe_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		err = ixgbe_setup_rx_resources(adapter, &adapter->rx_ring[i]);
		if (!err)
			continue;
		DPRINTK(PROBE, ERR, "Allocation for Rx Queue %u failed\n", i);
		break;
	}

	return err;
}

void ixgbe_free_tx_resources(struct ixgbe_adapter *adapter,
                             struct ixgbe_ring *tx_ring)
{
	struct pci_dev *pdev = adapter->pdev;

	ixgbe_clean_tx_ring(adapter, tx_ring);

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	pci_free_consistent(pdev, tx_ring->size, tx_ring->desc, tx_ring->dma);

	tx_ring->desc = NULL;
}

static void ixgbe_free_all_tx_resources(struct ixgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		if (adapter->tx_ring[i].desc)
			ixgbe_free_tx_resources(adapter, &adapter->tx_ring[i]);
}

void ixgbe_free_rx_resources(struct ixgbe_adapter *adapter,
                             struct ixgbe_ring *rx_ring)
{
	struct pci_dev *pdev = adapter->pdev;

	ixgbe_clean_rx_ring(adapter, rx_ring);

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	pci_free_consistent(pdev, rx_ring->size, rx_ring->desc, rx_ring->dma);

	rx_ring->desc = NULL;
}

static void ixgbe_free_all_rx_resources(struct ixgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		if (adapter->rx_ring[i].desc)
			ixgbe_free_rx_resources(adapter, &adapter->rx_ring[i]);
}

static int ixgbe_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;

	if ((new_mtu < 68) || (max_frame > IXGBE_MAX_JUMBO_FRAME_SIZE))
		return -EINVAL;

	DPRINTK(PROBE, INFO, "changing MTU from %d to %d\n",
	        netdev->mtu, new_mtu);
	 
	netdev->mtu = new_mtu;

	if (netif_running(netdev))
		ixgbe_reinit_locked(adapter);

	return 0;
}

static int ixgbe_open(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	int err;

	if (test_bit(__IXGBE_TESTING, &adapter->state))
		return -EBUSY;

	netif_carrier_off(netdev);

	err = ixgbe_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	err = ixgbe_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	ixgbe_configure(adapter);

	err = ixgbe_request_irq(adapter);
	if (err)
		goto err_req_irq;

	err = ixgbe_up_complete(adapter);
	if (err)
		goto err_up;

	netif_tx_start_all_queues(netdev);

	return 0;

err_up:
	ixgbe_release_hw_control(adapter);
	ixgbe_free_irq(adapter);
err_req_irq:
err_setup_rx:
	ixgbe_free_all_rx_resources(adapter);
err_setup_tx:
	ixgbe_free_all_tx_resources(adapter);
	ixgbe_reset(adapter);

	return err;
}

static int ixgbe_close(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	ixgbe_down(adapter);
	ixgbe_free_irq(adapter);

	ixgbe_free_all_tx_resources(adapter);
	ixgbe_free_all_rx_resources(adapter);

	ixgbe_release_hw_control(adapter);

	return 0;
}

#ifdef CONFIG_PM
static int ixgbe_resume(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	u32 err;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);

	err = pci_enable_device_mem(pdev);
	if (err) {
		printk(KERN_ERR "ixgbe: Cannot enable PCI device from "
				"suspend\n");
		return err;
	}
	pci_set_master(pdev);

	pci_wake_from_d3(pdev, false);

	err = ixgbe_init_interrupt_scheme(adapter);
	if (err) {
		printk(KERN_ERR "ixgbe: Cannot initialize interrupts for "
		                "device\n");
		return err;
	}

	ixgbe_reset(adapter);

	IXGBE_WRITE_REG(&adapter->hw, IXGBE_WUS, ~0);

	if (netif_running(netdev)) {
		err = ixgbe_open(adapter->netdev);
		if (err)
			return err;
	}

	netif_device_attach(netdev);

	return 0;
}
#endif  

static int __ixgbe_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;
	u32 ctrl, fctrl;
	u32 wufc = adapter->wol;
#ifdef CONFIG_PM
	int retval = 0;
#endif

	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		ixgbe_down(adapter);
		ixgbe_free_irq(adapter);
		ixgbe_free_all_tx_resources(adapter);
		ixgbe_free_all_rx_resources(adapter);
	}
	ixgbe_clear_interrupt_scheme(adapter);

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);
	if (retval)
		return retval;

#endif
	if (wufc) {
		ixgbe_set_rx_mode(netdev);

		if (wufc & IXGBE_WUFC_MC) {
			fctrl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
			fctrl |= IXGBE_FCTRL_MPE;
			IXGBE_WRITE_REG(hw, IXGBE_FCTRL, fctrl);
		}

		ctrl = IXGBE_READ_REG(hw, IXGBE_CTRL);
		ctrl |= IXGBE_CTRL_GIO_DIS;
		IXGBE_WRITE_REG(hw, IXGBE_CTRL, ctrl);

		IXGBE_WRITE_REG(hw, IXGBE_WUFC, wufc);
	} else {
		IXGBE_WRITE_REG(hw, IXGBE_WUC, 0);
		IXGBE_WRITE_REG(hw, IXGBE_WUFC, 0);
	}

	if (wufc && hw->mac.type == ixgbe_mac_82599EB)
		pci_wake_from_d3(pdev, true);
	else
		pci_wake_from_d3(pdev, false);

	*enable_wake = !!wufc;

	ixgbe_release_hw_control(adapter);

	pci_disable_device(pdev);

	return 0;
}

#ifdef CONFIG_PM
static int ixgbe_suspend(struct pci_dev *pdev, pm_message_t state)
{
	int retval;
	bool wake;

	retval = __ixgbe_shutdown(pdev, &wake);
	if (retval)
		return retval;

	if (wake) {
		pci_prepare_to_sleep(pdev);
	} else {
		pci_wake_from_d3(pdev, false);
		pci_set_power_state(pdev, PCI_D3hot);
	}

	return 0;
}
#endif  

static void ixgbe_shutdown(struct pci_dev *pdev)
{
	bool wake;

	__ixgbe_shutdown(pdev, &wake);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wake);
		pci_set_power_state(pdev, PCI_D3hot);
	}
}

void ixgbe_update_stats(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u64 total_mpc = 0;
	u32 i, missed_rx = 0, mpc, bprc, lxon, lxoff, xon_off_tot;

	if (hw->mac.type == ixgbe_mac_82599EB) {
		u64 rsc_count = 0;
		for (i = 0; i < 16; i++)
			adapter->hw_rx_no_dma_resources +=
			                     IXGBE_READ_REG(hw, IXGBE_QPRDC(i));
		for (i = 0; i < adapter->num_rx_queues; i++)
			rsc_count += adapter->rx_ring[i].rsc_count;
		adapter->rsc_count = rsc_count;
	}

	adapter->stats.crcerrs += IXGBE_READ_REG(hw, IXGBE_CRCERRS);
	for (i = 0; i < 8; i++) {
		 
		mpc = IXGBE_READ_REG(hw, IXGBE_MPC(i));
		missed_rx += mpc;
		adapter->stats.mpc[i] += mpc;
		total_mpc += adapter->stats.mpc[i];
		if (hw->mac.type == ixgbe_mac_82598EB)
			adapter->stats.rnbc[i] += IXGBE_READ_REG(hw, IXGBE_RNBC(i));
		adapter->stats.qptc[i] += IXGBE_READ_REG(hw, IXGBE_QPTC(i));
		adapter->stats.qbtc[i] += IXGBE_READ_REG(hw, IXGBE_QBTC(i));
		adapter->stats.qprc[i] += IXGBE_READ_REG(hw, IXGBE_QPRC(i));
		adapter->stats.qbrc[i] += IXGBE_READ_REG(hw, IXGBE_QBRC(i));
		if (hw->mac.type == ixgbe_mac_82599EB) {
			adapter->stats.pxonrxc[i] += IXGBE_READ_REG(hw,
			                                    IXGBE_PXONRXCNT(i));
			adapter->stats.pxoffrxc[i] += IXGBE_READ_REG(hw,
			                                   IXGBE_PXOFFRXCNT(i));
			adapter->stats.qprdc[i] += IXGBE_READ_REG(hw, IXGBE_QPRDC(i));
		} else {
			adapter->stats.pxonrxc[i] += IXGBE_READ_REG(hw,
			                                      IXGBE_PXONRXC(i));
			adapter->stats.pxoffrxc[i] += IXGBE_READ_REG(hw,
			                                     IXGBE_PXOFFRXC(i));
		}
		adapter->stats.pxontxc[i] += IXGBE_READ_REG(hw,
		                                            IXGBE_PXONTXC(i));
		adapter->stats.pxofftxc[i] += IXGBE_READ_REG(hw,
		                                             IXGBE_PXOFFTXC(i));
	}
	adapter->stats.gprc += IXGBE_READ_REG(hw, IXGBE_GPRC);
	 
	adapter->stats.gprc -= missed_rx;

	if (hw->mac.type == ixgbe_mac_82599EB) {
		u64 tmp;
		adapter->stats.gorc += IXGBE_READ_REG(hw, IXGBE_GORCL);
		tmp = IXGBE_READ_REG(hw, IXGBE_GORCH) & 0xF;  
		adapter->stats.gorc += (tmp << 32);
		adapter->stats.gotc += IXGBE_READ_REG(hw, IXGBE_GOTCL);
		tmp = IXGBE_READ_REG(hw, IXGBE_GOTCH) & 0xF;  
		adapter->stats.gotc += (tmp << 32);
		adapter->stats.tor += IXGBE_READ_REG(hw, IXGBE_TORL);
		IXGBE_READ_REG(hw, IXGBE_TORH);  
		adapter->stats.lxonrxc += IXGBE_READ_REG(hw, IXGBE_LXONRXCNT);
		adapter->stats.lxoffrxc += IXGBE_READ_REG(hw, IXGBE_LXOFFRXCNT);
		adapter->stats.fdirmatch += IXGBE_READ_REG(hw, IXGBE_FDIRMATCH);
		adapter->stats.fdirmiss += IXGBE_READ_REG(hw, IXGBE_FDIRMISS);
#ifdef IXGBE_FCOE
		adapter->stats.fccrc += IXGBE_READ_REG(hw, IXGBE_FCCRC);
		adapter->stats.fcoerpdc += IXGBE_READ_REG(hw, IXGBE_FCOERPDC);
		adapter->stats.fcoeprc += IXGBE_READ_REG(hw, IXGBE_FCOEPRC);
		adapter->stats.fcoeptc += IXGBE_READ_REG(hw, IXGBE_FCOEPTC);
		adapter->stats.fcoedwrc += IXGBE_READ_REG(hw, IXGBE_FCOEDWRC);
		adapter->stats.fcoedwtc += IXGBE_READ_REG(hw, IXGBE_FCOEDWTC);
#endif  
	} else {
		adapter->stats.lxonrxc += IXGBE_READ_REG(hw, IXGBE_LXONRXC);
		adapter->stats.lxoffrxc += IXGBE_READ_REG(hw, IXGBE_LXOFFRXC);
		adapter->stats.gorc += IXGBE_READ_REG(hw, IXGBE_GORCH);
		adapter->stats.gotc += IXGBE_READ_REG(hw, IXGBE_GOTCH);
		adapter->stats.tor += IXGBE_READ_REG(hw, IXGBE_TORH);
	}
	bprc = IXGBE_READ_REG(hw, IXGBE_BPRC);
	adapter->stats.bprc += bprc;
	adapter->stats.mprc += IXGBE_READ_REG(hw, IXGBE_MPRC);
	if (hw->mac.type == ixgbe_mac_82598EB)
		adapter->stats.mprc -= bprc;
	adapter->stats.roc += IXGBE_READ_REG(hw, IXGBE_ROC);
	adapter->stats.prc64 += IXGBE_READ_REG(hw, IXGBE_PRC64);
	adapter->stats.prc127 += IXGBE_READ_REG(hw, IXGBE_PRC127);
	adapter->stats.prc255 += IXGBE_READ_REG(hw, IXGBE_PRC255);
	adapter->stats.prc511 += IXGBE_READ_REG(hw, IXGBE_PRC511);
	adapter->stats.prc1023 += IXGBE_READ_REG(hw, IXGBE_PRC1023);
	adapter->stats.prc1522 += IXGBE_READ_REG(hw, IXGBE_PRC1522);
	adapter->stats.rlec += IXGBE_READ_REG(hw, IXGBE_RLEC);
	lxon = IXGBE_READ_REG(hw, IXGBE_LXONTXC);
	adapter->stats.lxontxc += lxon;
	lxoff = IXGBE_READ_REG(hw, IXGBE_LXOFFTXC);
	adapter->stats.lxofftxc += lxoff;
	adapter->stats.ruc += IXGBE_READ_REG(hw, IXGBE_RUC);
	adapter->stats.gptc += IXGBE_READ_REG(hw, IXGBE_GPTC);
	adapter->stats.mptc += IXGBE_READ_REG(hw, IXGBE_MPTC);
	 
	xon_off_tot = lxon + lxoff;
	adapter->stats.gptc -= xon_off_tot;
	adapter->stats.mptc -= xon_off_tot;
	adapter->stats.gotc -= (xon_off_tot * (ETH_ZLEN + ETH_FCS_LEN));
	adapter->stats.ruc += IXGBE_READ_REG(hw, IXGBE_RUC);
	adapter->stats.rfc += IXGBE_READ_REG(hw, IXGBE_RFC);
	adapter->stats.rjc += IXGBE_READ_REG(hw, IXGBE_RJC);
	adapter->stats.tpr += IXGBE_READ_REG(hw, IXGBE_TPR);
	adapter->stats.ptc64 += IXGBE_READ_REG(hw, IXGBE_PTC64);
	adapter->stats.ptc64 -= xon_off_tot;
	adapter->stats.ptc127 += IXGBE_READ_REG(hw, IXGBE_PTC127);
	adapter->stats.ptc255 += IXGBE_READ_REG(hw, IXGBE_PTC255);
	adapter->stats.ptc511 += IXGBE_READ_REG(hw, IXGBE_PTC511);
	adapter->stats.ptc1023 += IXGBE_READ_REG(hw, IXGBE_PTC1023);
	adapter->stats.ptc1522 += IXGBE_READ_REG(hw, IXGBE_PTC1522);
	adapter->stats.bptc += IXGBE_READ_REG(hw, IXGBE_BPTC);

	adapter->net_stats.multicast = adapter->stats.mprc;

	adapter->net_stats.rx_errors = adapter->stats.crcerrs +
	                               adapter->stats.rlec;
	adapter->net_stats.rx_dropped = 0;
	adapter->net_stats.rx_length_errors = adapter->stats.rlec;
	adapter->net_stats.rx_crc_errors = adapter->stats.crcerrs;
	adapter->net_stats.rx_missed_errors = total_mpc;
}

static void ixgbe_watchdog(unsigned long data)
{
	struct ixgbe_adapter *adapter = (struct ixgbe_adapter *)data;
	struct ixgbe_hw *hw = &adapter->hw;
	u64 eics = 0;
	int i;

	if (test_bit(__IXGBE_DOWN, &adapter->state))
		goto watchdog_short_circuit;

	if (!(adapter->flags & IXGBE_FLAG_MSIX_ENABLED)) {
		 
		IXGBE_WRITE_REG(hw, IXGBE_EICS,
			(IXGBE_EICS_TCP_TIMER | IXGBE_EICS_OTHER));
		goto watchdog_reschedule;
	}

	for (i = 0; i < adapter->num_msix_vectors - NON_Q_VECTORS; i++) {
		struct ixgbe_q_vector *qv = adapter->q_vector[i];
		if (qv->rxr_count || qv->txr_count)
			eics |= ((u64)1 << i);
	}

	ixgbe_irq_rearm_queues(adapter, eics);

watchdog_reschedule:
	 
	mod_timer(&adapter->watchdog_timer, round_jiffies(jiffies + 2 * HZ));

watchdog_short_circuit:
	schedule_work(&adapter->watchdog_task);
}

static void ixgbe_multispeed_fiber_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter = container_of(work,
	                                             struct ixgbe_adapter,
	                                             multispeed_fiber_task);
	struct ixgbe_hw *hw = &adapter->hw;
	u32 autoneg;
	bool negotiation;

	adapter->flags |= IXGBE_FLAG_IN_SFP_LINK_TASK;
	autoneg = hw->phy.autoneg_advertised;
	if ((!autoneg) && (hw->mac.ops.get_link_capabilities))
		hw->mac.ops.get_link_capabilities(hw, &autoneg, &negotiation);
	if (hw->mac.ops.setup_link)
		hw->mac.ops.setup_link(hw, autoneg, negotiation, true);
	adapter->flags |= IXGBE_FLAG_NEED_LINK_UPDATE;
	adapter->flags &= ~IXGBE_FLAG_IN_SFP_LINK_TASK;
}

static void ixgbe_sfp_config_module_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter = container_of(work,
	                                             struct ixgbe_adapter,
	                                             sfp_config_module_task);
	struct ixgbe_hw *hw = &adapter->hw;
	u32 err;

	adapter->flags |= IXGBE_FLAG_IN_SFP_MOD_TASK;

	msleep(100);
	err = hw->phy.ops.identify_sfp(hw);

	if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		dev_err(&adapter->pdev->dev, "failed to initialize because "
			"an unsupported SFP+ module type was detected.\n"
			"Reload the driver after installing a supported "
			"module.\n");
		unregister_netdev(adapter->netdev);
		return;
	}
	hw->mac.ops.setup_sfp(hw);

	if (!(adapter->flags & IXGBE_FLAG_IN_SFP_LINK_TASK))
		 
		schedule_work(&adapter->multispeed_fiber_task);
	adapter->flags &= ~IXGBE_FLAG_IN_SFP_MOD_TASK;
}

static void ixgbe_fdir_reinit_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter = container_of(work,
	                                             struct ixgbe_adapter,
	                                             fdir_reinit_task);
	struct ixgbe_hw *hw = &adapter->hw;
	int i;

	if (ixgbe_reinit_fdir_tables_82599(hw) == 0) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			set_bit(__IXGBE_FDIR_INIT_DONE,
			        &(adapter->tx_ring[i].reinit_state));
	} else {
		DPRINTK(PROBE, ERR, "failed to finish FDIR re-initialization, "
		        "ignored adding FDIR ATR filters \n");
	}
	 
	netif_tx_start_all_queues(adapter->netdev);
}

static void ixgbe_watchdog_task(struct work_struct *work)
{
	struct ixgbe_adapter *adapter = container_of(work,
	                                             struct ixgbe_adapter,
	                                             watchdog_task);
	struct net_device *netdev = adapter->netdev;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	int i;
	struct ixgbe_ring *tx_ring;
	int some_tx_pending = 0;

	adapter->flags |= IXGBE_FLAG_IN_WATCHDOG_TASK;

	if (adapter->flags & IXGBE_FLAG_NEED_LINK_UPDATE) {
		hw->mac.ops.check_link(hw, &link_speed, &link_up, false);
		if (link_up) {
#ifdef CONFIG_DCB
			if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
				for (i = 0; i < MAX_TRAFFIC_CLASS; i++)
					hw->mac.ops.fc_enable(hw, i);
			} else {
				hw->mac.ops.fc_enable(hw, 0);
			}
#else
			hw->mac.ops.fc_enable(hw, 0);
#endif
		}

		if (link_up ||
		    time_after(jiffies, (adapter->link_check_timeout +
		                         IXGBE_TRY_LINK_TIMEOUT))) {
			adapter->flags &= ~IXGBE_FLAG_NEED_LINK_UPDATE;
			IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EIMC_LSC);
		}
		adapter->link_up = link_up;
		adapter->link_speed = link_speed;
	}

	if (link_up) {
		if (!netif_carrier_ok(netdev)) {
			bool flow_rx, flow_tx;

			if (hw->mac.type == ixgbe_mac_82599EB) {
				u32 mflcn = IXGBE_READ_REG(hw, IXGBE_MFLCN);
				u32 fccfg = IXGBE_READ_REG(hw, IXGBE_FCCFG);
				flow_rx = !!(mflcn & IXGBE_MFLCN_RFCE);
				flow_tx = !!(fccfg & IXGBE_FCCFG_TFCE_802_3X);
			} else {
				u32 frctl = IXGBE_READ_REG(hw, IXGBE_FCTRL);
				u32 rmcs = IXGBE_READ_REG(hw, IXGBE_RMCS);
				flow_rx = !!(frctl & IXGBE_FCTRL_RFCE);
				flow_tx = !!(rmcs & IXGBE_RMCS_TFCE_802_3X);
			}

			printk(KERN_INFO "ixgbe: %s NIC Link is Up %s, "
			       "Flow Control: %s\n",
			       netdev->name,
			       (link_speed == IXGBE_LINK_SPEED_10GB_FULL ?
			        "10 Gbps" :
			        (link_speed == IXGBE_LINK_SPEED_1GB_FULL ?
			         "1 Gbps" : "unknown speed")),
			       ((flow_rx && flow_tx) ? "RX/TX" :
			        (flow_rx ? "RX" :
			        (flow_tx ? "TX" : "None"))));

			netif_carrier_on(netdev);
		} else {
			 
			adapter->detect_tx_hung = true;
		}
	} else {
		adapter->link_up = false;
		adapter->link_speed = 0;
		if (netif_carrier_ok(netdev)) {
			printk(KERN_INFO "ixgbe: %s NIC Link is Down\n",
			       netdev->name);
			netif_carrier_off(netdev);
		}
	}

	if (!netif_carrier_ok(netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			tx_ring = &adapter->tx_ring[i];
			if (tx_ring->next_to_use != tx_ring->next_to_clean) {
				some_tx_pending = 1;
				break;
			}
		}

		if (some_tx_pending) {
			 
			 schedule_work(&adapter->reset_task);
		}
	}

	ixgbe_update_stats(adapter);
	adapter->flags &= ~IXGBE_FLAG_IN_WATCHDOG_TASK;
}

static int ixgbe_tso(struct ixgbe_adapter *adapter,
                     struct ixgbe_ring *tx_ring, struct sk_buff *skb,
                     u32 tx_flags, u8 *hdr_len)
{
	struct ixgbe_adv_tx_context_desc *context_desc;
	unsigned int i;
	int err;
	struct ixgbe_tx_buffer *tx_buffer_info;
	u32 vlan_macip_lens = 0, type_tucmd_mlhl;
	u32 mss_l4len_idx, l4len;

	if (skb_is_gso(skb)) {
		if (skb_header_cloned(skb)) {
			err = pskb_expand_head(skb, 0, 0, GFP_ATOMIC);
			if (err)
				return err;
		}
		l4len = tcp_hdrlen(skb);
		*hdr_len += l4len;

		if (skb->protocol == htons(ETH_P_IP)) {
			struct iphdr *iph = ip_hdr(skb);
			iph->tot_len = 0;
			iph->check = 0;
			tcp_hdr(skb)->check = ~csum_tcpudp_magic(iph->saddr,
			                                         iph->daddr, 0,
			                                         IPPROTO_TCP,
			                                         0);
			adapter->hw_tso_ctxt++;
		} else if (skb_shinfo(skb)->gso_type == SKB_GSO_TCPV6) {
			ipv6_hdr(skb)->payload_len = 0;
			tcp_hdr(skb)->check =
			    ~csum_ipv6_magic(&ipv6_hdr(skb)->saddr,
			                     &ipv6_hdr(skb)->daddr,
			                     0, IPPROTO_TCP, 0);
			adapter->hw_tso6_ctxt++;
		}

		i = tx_ring->next_to_use;

		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		context_desc = IXGBE_TX_CTXTDESC_ADV(*tx_ring, i);

		if (tx_flags & IXGBE_TX_FLAGS_VLAN)
			vlan_macip_lens |=
			    (tx_flags & IXGBE_TX_FLAGS_VLAN_MASK);
		vlan_macip_lens |= ((skb_network_offset(skb)) <<
		                    IXGBE_ADVTXD_MACLEN_SHIFT);
		*hdr_len += skb_network_offset(skb);
		vlan_macip_lens |=
		    (skb_transport_header(skb) - skb_network_header(skb));
		*hdr_len +=
		    (skb_transport_header(skb) - skb_network_header(skb));
		context_desc->vlan_macip_lens = cpu_to_le32(vlan_macip_lens);
		context_desc->seqnum_seed = 0;

		type_tucmd_mlhl = (IXGBE_TXD_CMD_DEXT |
		                   IXGBE_ADVTXD_DTYP_CTXT);

		if (skb->protocol == htons(ETH_P_IP))
			type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV4;
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_TCP;
		context_desc->type_tucmd_mlhl = cpu_to_le32(type_tucmd_mlhl);

		mss_l4len_idx =
		    (skb_shinfo(skb)->gso_size << IXGBE_ADVTXD_MSS_SHIFT);
		mss_l4len_idx |= (l4len << IXGBE_ADVTXD_L4LEN_SHIFT);
		 
		mss_l4len_idx |= (1 << IXGBE_ADVTXD_IDX_SHIFT);
		context_desc->mss_l4len_idx = cpu_to_le32(mss_l4len_idx);

		tx_buffer_info->time_stamp = jiffies;
		tx_buffer_info->next_to_watch = i;

		i++;
		if (i == tx_ring->count)
			i = 0;
		tx_ring->next_to_use = i;

		return true;
	}
	return false;
}

static bool ixgbe_tx_csum(struct ixgbe_adapter *adapter,
                          struct ixgbe_ring *tx_ring,
                          struct sk_buff *skb, u32 tx_flags)
{
	struct ixgbe_adv_tx_context_desc *context_desc;
	unsigned int i;
	struct ixgbe_tx_buffer *tx_buffer_info;
	u32 vlan_macip_lens = 0, type_tucmd_mlhl = 0;

	if (skb->ip_summed == CHECKSUM_PARTIAL ||
	    (tx_flags & IXGBE_TX_FLAGS_VLAN)) {
		i = tx_ring->next_to_use;
		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		context_desc = IXGBE_TX_CTXTDESC_ADV(*tx_ring, i);

		if (tx_flags & IXGBE_TX_FLAGS_VLAN)
			vlan_macip_lens |=
			    (tx_flags & IXGBE_TX_FLAGS_VLAN_MASK);
		vlan_macip_lens |= (skb_network_offset(skb) <<
		                    IXGBE_ADVTXD_MACLEN_SHIFT);
		if (skb->ip_summed == CHECKSUM_PARTIAL)
			vlan_macip_lens |= (skb_transport_header(skb) -
			                    skb_network_header(skb));

		context_desc->vlan_macip_lens = cpu_to_le32(vlan_macip_lens);
		context_desc->seqnum_seed = 0;

		type_tucmd_mlhl |= (IXGBE_TXD_CMD_DEXT |
		                    IXGBE_ADVTXD_DTYP_CTXT);

		if (skb->ip_summed == CHECKSUM_PARTIAL) {
			switch (skb->protocol) {
			case cpu_to_be16(ETH_P_IP):
				type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV4;
				if (ip_hdr(skb)->protocol == IPPROTO_TCP)
					type_tucmd_mlhl |=
					        IXGBE_ADVTXD_TUCMD_L4T_TCP;
				else if (ip_hdr(skb)->protocol == IPPROTO_SCTP)
					type_tucmd_mlhl |=
					        IXGBE_ADVTXD_TUCMD_L4T_SCTP;
				break;
			case cpu_to_be16(ETH_P_IPV6):
				 
				if (ipv6_hdr(skb)->nexthdr == IPPROTO_TCP)
					type_tucmd_mlhl |=
					        IXGBE_ADVTXD_TUCMD_L4T_TCP;
				else if (ipv6_hdr(skb)->nexthdr == IPPROTO_SCTP)
					type_tucmd_mlhl |=
					        IXGBE_ADVTXD_TUCMD_L4T_SCTP;
				break;
			default:
				if (unlikely(net_ratelimit())) {
					DPRINTK(PROBE, WARNING,
					 "partial checksum but proto=%x!\n",
					 skb->protocol);
				}
				break;
			}
		}

		context_desc->type_tucmd_mlhl = cpu_to_le32(type_tucmd_mlhl);
		 
		context_desc->mss_l4len_idx = 0;

		tx_buffer_info->time_stamp = jiffies;
		tx_buffer_info->next_to_watch = i;

		adapter->hw_csum_tx_good++;
		i++;
		if (i == tx_ring->count)
			i = 0;
		tx_ring->next_to_use = i;

		return true;
	}

	return false;
}

static int ixgbe_tx_map(struct ixgbe_adapter *adapter,
                        struct ixgbe_ring *tx_ring,
                        struct sk_buff *skb, u32 tx_flags,
                        unsigned int first)
{
	struct ixgbe_tx_buffer *tx_buffer_info;
	unsigned int len;
	unsigned int total = skb->len;
	unsigned int offset = 0, size, count = 0, i;
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags;
	unsigned int f;
	dma_addr_t *map;

	i = tx_ring->next_to_use;

	if (skb_dma_map(&adapter->pdev->dev, skb, DMA_TO_DEVICE)) {
		dev_err(&adapter->pdev->dev, "TX DMA map failed\n");
		return 0;
	}

	map = skb_shinfo(skb)->dma_maps;

	if (tx_flags & IXGBE_TX_FLAGS_FCOE)
		 
		total -= sizeof(struct fcoe_crc_eof);

	len = min(skb_headlen(skb), total);
	while (len) {
		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		size = min(len, (uint)IXGBE_MAX_DATA_PER_TXD);

		tx_buffer_info->length = size;
		tx_buffer_info->dma = skb_shinfo(skb)->dma_head + offset;
		tx_buffer_info->time_stamp = jiffies;
		tx_buffer_info->next_to_watch = i;

		len -= size;
		total -= size;
		offset += size;
		count++;

		if (len) {
			i++;
			if (i == tx_ring->count)
				i = 0;
		}
	}

	for (f = 0; f < nr_frags; f++) {
		struct skb_frag_struct *frag;

		frag = &skb_shinfo(skb)->frags[f];
		len = min((unsigned int)frag->size, total);
		offset = 0;

		while (len) {
			i++;
			if (i == tx_ring->count)
				i = 0;

			tx_buffer_info = &tx_ring->tx_buffer_info[i];
			size = min(len, (uint)IXGBE_MAX_DATA_PER_TXD);

			tx_buffer_info->length = size;
			tx_buffer_info->dma = map[f] + offset;
			tx_buffer_info->time_stamp = jiffies;
			tx_buffer_info->next_to_watch = i;

			len -= size;
			total -= size;
			offset += size;
			count++;
		}
		if (total == 0)
			break;
	}

	tx_ring->tx_buffer_info[i].skb = skb;
	tx_ring->tx_buffer_info[first].next_to_watch = i;

	return count;
}

static void ixgbe_tx_queue(struct ixgbe_adapter *adapter,
                           struct ixgbe_ring *tx_ring,
                           int tx_flags, int count, u32 paylen, u8 hdr_len)
{
	union ixgbe_adv_tx_desc *tx_desc = NULL;
	struct ixgbe_tx_buffer *tx_buffer_info;
	u32 olinfo_status = 0, cmd_type_len = 0;
	unsigned int i;
	u32 txd_cmd = IXGBE_TXD_CMD_EOP | IXGBE_TXD_CMD_RS | IXGBE_TXD_CMD_IFCS;

	cmd_type_len |= IXGBE_ADVTXD_DTYP_DATA;

	cmd_type_len |= IXGBE_ADVTXD_DCMD_IFCS | IXGBE_ADVTXD_DCMD_DEXT;

	if (tx_flags & IXGBE_TX_FLAGS_VLAN)
		cmd_type_len |= IXGBE_ADVTXD_DCMD_VLE;

	if (tx_flags & IXGBE_TX_FLAGS_TSO) {
		cmd_type_len |= IXGBE_ADVTXD_DCMD_TSE;

		olinfo_status |= IXGBE_TXD_POPTS_TXSM <<
		                 IXGBE_ADVTXD_POPTS_SHIFT;

		olinfo_status |= (1 << IXGBE_ADVTXD_IDX_SHIFT);
		if (tx_flags & IXGBE_TX_FLAGS_IPV4)
			olinfo_status |= IXGBE_TXD_POPTS_IXSM <<
			                 IXGBE_ADVTXD_POPTS_SHIFT;

	} else if (tx_flags & IXGBE_TX_FLAGS_CSUM)
		olinfo_status |= IXGBE_TXD_POPTS_TXSM <<
		                 IXGBE_ADVTXD_POPTS_SHIFT;

	if (tx_flags & IXGBE_TX_FLAGS_FCOE) {
		olinfo_status |= IXGBE_ADVTXD_CC;
		olinfo_status |= (1 << IXGBE_ADVTXD_IDX_SHIFT);
		if (tx_flags & IXGBE_TX_FLAGS_FSO)
			cmd_type_len |= IXGBE_ADVTXD_DCMD_TSE;
	}

	olinfo_status |= ((paylen - hdr_len) << IXGBE_ADVTXD_PAYLEN_SHIFT);

	i = tx_ring->next_to_use;
	while (count--) {
		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		tx_desc = IXGBE_TX_DESC_ADV(*tx_ring, i);
		tx_desc->read.buffer_addr = cpu_to_le64(tx_buffer_info->dma);
		tx_desc->read.cmd_type_len =
		        cpu_to_le32(cmd_type_len | tx_buffer_info->length);
		tx_desc->read.olinfo_status = cpu_to_le32(olinfo_status);
		i++;
		if (i == tx_ring->count)
			i = 0;
	}

	tx_desc->read.cmd_type_len |= cpu_to_le32(txd_cmd);

	wmb();

	tx_ring->next_to_use = i;
	writel(i, adapter->hw.hw_addr + tx_ring->tail);
}

static void ixgbe_atr(struct ixgbe_adapter *adapter, struct sk_buff *skb,
	              int queue, u32 tx_flags)
{
	 
	struct ixgbe_atr_input atr_input;
	struct tcphdr *th;
	struct iphdr *iph = ip_hdr(skb);
	struct ethhdr *eth = (struct ethhdr *)skb->data;
	u16 vlan_id, src_port, dst_port, flex_bytes;
	u32 src_ipv4_addr, dst_ipv4_addr;
	u8 l4type = 0;

	if (iph->protocol == IPPROTO_TCP) {
		th = tcp_hdr(skb);
		src_port = th->source;
		dst_port = th->dest;
		l4type |= IXGBE_ATR_L4TYPE_TCP;
		 
	} else {
		 
		return;
	}

	memset(&atr_input, 0, sizeof(struct ixgbe_atr_input));

	vlan_id = (tx_flags & IXGBE_TX_FLAGS_VLAN_MASK) >>
	           IXGBE_TX_FLAGS_VLAN_SHIFT;
	src_ipv4_addr = iph->saddr;
	dst_ipv4_addr = iph->daddr;
	flex_bytes = eth->h_proto;

	ixgbe_atr_set_vlan_id_82599(&atr_input, vlan_id);
	ixgbe_atr_set_src_port_82599(&atr_input, dst_port);
	ixgbe_atr_set_dst_port_82599(&atr_input, src_port);
	ixgbe_atr_set_flex_byte_82599(&atr_input, flex_bytes);
	ixgbe_atr_set_l4type_82599(&atr_input, l4type);
	 
	ixgbe_atr_set_src_ipv4_82599(&atr_input, dst_ipv4_addr);
	ixgbe_atr_set_dst_ipv4_82599(&atr_input, src_ipv4_addr);

	ixgbe_fdir_add_signature_filter_82599(&adapter->hw, &atr_input, queue);
}

static int __ixgbe_maybe_stop_tx(struct net_device *netdev,
                                 struct ixgbe_ring *tx_ring, int size)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	netif_stop_subqueue(netdev, tx_ring->queue_index);
	 
	smp_mb();

	if (likely(IXGBE_DESC_UNUSED(tx_ring) < size))
		return -EBUSY;

	netif_start_subqueue(netdev, tx_ring->queue_index);
	++adapter->restart_queue;
	return 0;
}

static int ixgbe_maybe_stop_tx(struct net_device *netdev,
                              struct ixgbe_ring *tx_ring, int size)
{
	if (likely(IXGBE_DESC_UNUSED(tx_ring) >= size))
		return 0;
	return __ixgbe_maybe_stop_tx(netdev, tx_ring, size);
}

static u16 ixgbe_select_queue(struct net_device *dev, struct sk_buff *skb)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);

	if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE)
		return smp_processor_id();

	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED)
		return (skb->vlan_tci & IXGBE_TX_FLAGS_VLAN_PRIO_MASK) >> 13;

	return skb_tx_hash(dev, skb);
}

static netdev_tx_t ixgbe_xmit_frame(struct sk_buff *skb,
				    struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_ring *tx_ring;
	unsigned int first;
	unsigned int tx_flags = 0;
	u8 hdr_len = 0;
	int r_idx = 0, tso;
	int count = 0;
	unsigned int f;

	if (adapter->vlgrp && vlan_tx_tag_present(skb)) {
		tx_flags |= vlan_tx_tag_get(skb);
		if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
			tx_flags &= ~IXGBE_TX_FLAGS_VLAN_PRIO_MASK;
			tx_flags |= (skb->queue_mapping << 13);
		}
		tx_flags <<= IXGBE_TX_FLAGS_VLAN_SHIFT;
		tx_flags |= IXGBE_TX_FLAGS_VLAN;
	} else if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
		if (skb->priority != TC_PRIO_CONTROL) {
			tx_flags |= (skb->queue_mapping << 13);
			tx_flags <<= IXGBE_TX_FLAGS_VLAN_SHIFT;
			tx_flags |= IXGBE_TX_FLAGS_VLAN;
		} else {
			skb->queue_mapping =
				adapter->ring_feature[RING_F_DCB].indices-1;
		}
	}

	r_idx = skb->queue_mapping;
	tx_ring = &adapter->tx_ring[r_idx];

	if ((adapter->flags & IXGBE_FLAG_FCOE_ENABLED) &&
	    (skb->protocol == htons(ETH_P_FCOE))) {
		tx_flags |= IXGBE_TX_FLAGS_FCOE;
#ifdef IXGBE_FCOE
		r_idx = smp_processor_id();
		r_idx &= (adapter->ring_feature[RING_F_FCOE].indices - 1);
		r_idx += adapter->ring_feature[RING_F_FCOE].mask;
		tx_ring = &adapter->tx_ring[r_idx];
#endif
	}
	 
	if (skb_is_gso(skb) ||
	    (skb->ip_summed == CHECKSUM_PARTIAL) ||
	    (tx_flags & IXGBE_TX_FLAGS_VLAN) ||
	    (tx_flags & IXGBE_TX_FLAGS_FCOE))
		count++;

	count += TXD_USE_COUNT(skb_headlen(skb));
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++)
		count += TXD_USE_COUNT(skb_shinfo(skb)->frags[f].size);

	if (ixgbe_maybe_stop_tx(netdev, tx_ring, count)) {
		adapter->tx_busy++;
		return NETDEV_TX_BUSY;
	}

	first = tx_ring->next_to_use;
	if (tx_flags & IXGBE_TX_FLAGS_FCOE) {
#ifdef IXGBE_FCOE
		 
		tso = ixgbe_fso(adapter, tx_ring, skb, tx_flags, &hdr_len);
		if (tso < 0) {
			dev_kfree_skb_any(skb);
			return NETDEV_TX_OK;
		}
		if (tso)
			tx_flags |= IXGBE_TX_FLAGS_FSO;
#endif  
	} else {
		if (skb->protocol == htons(ETH_P_IP))
			tx_flags |= IXGBE_TX_FLAGS_IPV4;
		tso = ixgbe_tso(adapter, tx_ring, skb, tx_flags, &hdr_len);
		if (tso < 0) {
			dev_kfree_skb_any(skb);
			return NETDEV_TX_OK;
		}

		if (tso)
			tx_flags |= IXGBE_TX_FLAGS_TSO;
		else if (ixgbe_tx_csum(adapter, tx_ring, skb, tx_flags) &&
			 (skb->ip_summed == CHECKSUM_PARTIAL))
			tx_flags |= IXGBE_TX_FLAGS_CSUM;
	}

	count = ixgbe_tx_map(adapter, tx_ring, skb, tx_flags, first);
	if (count) {
		 
		if (tx_ring->atr_sample_rate) {
			++tx_ring->atr_count;
			if ((tx_ring->atr_count >= tx_ring->atr_sample_rate) &&
		             test_bit(__IXGBE_FDIR_INIT_DONE,
                                      &tx_ring->reinit_state)) {
				ixgbe_atr(adapter, skb, tx_ring->queue_index,
				          tx_flags);
				tx_ring->atr_count = 0;
			}
		}
		ixgbe_tx_queue(adapter, tx_ring, tx_flags, count, skb->len,
		               hdr_len);
		ixgbe_maybe_stop_tx(netdev, tx_ring, DESC_NEEDED);

	} else {
		dev_kfree_skb_any(skb);
		tx_ring->tx_buffer_info[first].time_stamp = 0;
		tx_ring->next_to_use = first;
	}

	return NETDEV_TX_OK;
}

static struct net_device_stats *ixgbe_get_stats(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	return &adapter->net_stats;
}

static int ixgbe_set_mac(struct net_device *netdev, void *p)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);

	hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, IXGBE_RAH_AV);

	return 0;
}

static int
ixgbe_mdio_read(struct net_device *netdev, int prtad, int devad, u16 addr)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;
	u16 value;
	int rc;

	if (prtad != hw->phy.mdio.prtad)
		return -EINVAL;
	rc = hw->phy.ops.read_reg(hw, addr, devad, &value);
	if (!rc)
		rc = value;
	return rc;
}

static int ixgbe_mdio_write(struct net_device *netdev, int prtad, int devad,
			    u16 addr, u16 value)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw = &adapter->hw;

	if (prtad != hw->phy.mdio.prtad)
		return -EINVAL;
	return hw->phy.ops.write_reg(hw, addr, devad, value);
}

static int ixgbe_ioctl(struct net_device *netdev, struct ifreq *req, int cmd)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	return mdio_mii_ioctl(&adapter->hw.phy.mdio, if_mii(req), cmd);
}

static int ixgbe_add_sanmac_netdev(struct net_device *dev)
{
	int err = 0;
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	struct ixgbe_mac_info *mac = &adapter->hw.mac;

	if (is_valid_ether_addr(mac->san_addr)) {
		rtnl_lock();
		err = dev_addr_add(dev, mac->san_addr, NETDEV_HW_ADDR_T_SAN);
		rtnl_unlock();
	}
	return err;
}

static int ixgbe_del_sanmac_netdev(struct net_device *dev)
{
	int err = 0;
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	struct ixgbe_mac_info *mac = &adapter->hw.mac;

	if (is_valid_ether_addr(mac->san_addr)) {
		rtnl_lock();
		err = dev_addr_del(dev, mac->san_addr, NETDEV_HW_ADDR_T_SAN);
		rtnl_unlock();
	}
	return err;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
 
static void ixgbe_netpoll(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	int i;

	adapter->flags |= IXGBE_FLAG_IN_NETPOLL;
	if (adapter->flags & IXGBE_FLAG_MSIX_ENABLED) {
		int num_q_vectors = adapter->num_msix_vectors - NON_Q_VECTORS;
		for (i = 0; i < num_q_vectors; i++) {
			struct ixgbe_q_vector *q_vector = adapter->q_vector[i];
			ixgbe_msix_clean_many(0, q_vector);
		}
	} else {
		ixgbe_intr(adapter->pdev->irq, netdev);
	}
	adapter->flags &= ~IXGBE_FLAG_IN_NETPOLL;
}
#endif

static const struct net_device_ops ixgbe_netdev_ops = {
	.ndo_open 		= ixgbe_open,
	.ndo_stop		= ixgbe_close,
	.ndo_start_xmit		= ixgbe_xmit_frame,
	.ndo_select_queue	= ixgbe_select_queue,
	.ndo_get_stats		= ixgbe_get_stats,
	.ndo_set_rx_mode        = ixgbe_set_rx_mode,
	.ndo_set_multicast_list	= ixgbe_set_rx_mode,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= ixgbe_set_mac,
	.ndo_change_mtu		= ixgbe_change_mtu,
	.ndo_tx_timeout		= ixgbe_tx_timeout,
	.ndo_vlan_rx_register	= ixgbe_vlan_rx_register,
	.ndo_vlan_rx_add_vid	= ixgbe_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= ixgbe_vlan_rx_kill_vid,
	.ndo_do_ioctl		= ixgbe_ioctl,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ixgbe_netpoll,
#endif
#ifdef IXGBE_FCOE
	.ndo_fcoe_ddp_setup = ixgbe_fcoe_ddp_get,
	.ndo_fcoe_ddp_done = ixgbe_fcoe_ddp_put,
	.ndo_fcoe_enable = ixgbe_fcoe_enable,
	.ndo_fcoe_disable = ixgbe_fcoe_disable,
#endif  
};

static int __devinit ixgbe_probe(struct pci_dev *pdev,
                                 const struct pci_device_id *ent)
{
	struct net_device *netdev;
	struct ixgbe_adapter *adapter = NULL;
	struct ixgbe_hw *hw;
	const struct ixgbe_info *ii = ixgbe_info_tbl[ent->driver_data];
	static int cards_found;
	int i, err, pci_using_dac;
#ifdef IXGBE_FCOE
	u16 device_caps;
#endif
	u32 part_num, eec;

	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64)) &&
	    !pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64))) {
		pci_using_dac = 1;
	} else {
		err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		if (err) {
			err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
			if (err) {
				dev_err(&pdev->dev, "No usable DMA "
				        "configuration, aborting\n");
				goto err_dma;
			}
		}
		pci_using_dac = 0;
	}

	err = pci_request_selected_regions(pdev, pci_select_bars(pdev,
	                                   IORESOURCE_MEM), ixgbe_driver_name);
	if (err) {
		dev_err(&pdev->dev,
		        "pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	pci_enable_pcie_error_reporting(pdev);

	pci_set_master(pdev);
	pci_save_state(pdev);

	netdev = alloc_etherdev_mq(sizeof(struct ixgbe_adapter), MAX_TX_QUEUES);
	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);

	pci_set_drvdata(pdev, netdev);
	adapter = netdev_priv(netdev);

	adapter->netdev = netdev;
	adapter->pdev = pdev;
	hw = &adapter->hw;
	hw->back = adapter;
	adapter->msg_enable = (1 << DEFAULT_DEBUG_LEVEL_SHIFT) - 1;

	hw->hw_addr = ioremap(pci_resource_start(pdev, 0),
	                      pci_resource_len(pdev, 0));
	if (!hw->hw_addr) {
		err = -EIO;
		goto err_ioremap;
	}

	for (i = 1; i <= 5; i++) {
		if (pci_resource_len(pdev, i) == 0)
			continue;
	}

	netdev->netdev_ops = &ixgbe_netdev_ops;
	ixgbe_set_ethtool_ops(netdev);
	netdev->watchdog_timeo = 5 * HZ;
	strcpy(netdev->name, pci_name(pdev));

	adapter->bd_number = cards_found;

	memcpy(&hw->mac.ops, ii->mac_ops, sizeof(hw->mac.ops));
	hw->mac.type  = ii->mac;

	memcpy(&hw->eeprom.ops, ii->eeprom_ops, sizeof(hw->eeprom.ops));
	eec = IXGBE_READ_REG(hw, IXGBE_EEC);
	 
	if (!(eec & (1 << 8)))
		hw->eeprom.ops.read = &ixgbe_read_eeprom_bit_bang_generic;

	memcpy(&hw->phy.ops, ii->phy_ops, sizeof(hw->phy.ops));
	hw->phy.sfp_type = ixgbe_sfp_type_unknown;
	 
	hw->phy.mdio.prtad = MDIO_PRTAD_NONE;
	hw->phy.mdio.mmds = 0;
	hw->phy.mdio.mode_support = MDIO_SUPPORTS_C45 | MDIO_EMULATE_C22;
	hw->phy.mdio.dev = netdev;
	hw->phy.mdio.mdio_read = ixgbe_mdio_read;
	hw->phy.mdio.mdio_write = ixgbe_mdio_write;

	init_timer(&adapter->sfp_timer);
	adapter->sfp_timer.function = &ixgbe_sfp_timer;
	adapter->sfp_timer.data = (unsigned long) adapter;

	INIT_WORK(&adapter->sfp_task, ixgbe_sfp_task);

	INIT_WORK(&adapter->multispeed_fiber_task, ixgbe_multispeed_fiber_task);

	INIT_WORK(&adapter->sfp_config_module_task,
	          ixgbe_sfp_config_module_task);

	ii->get_invariants(hw);

	err = ixgbe_sw_init(adapter);
	if (err)
		goto err_sw_init;

	if (adapter->flags & IXGBE_FLAG_FAN_FAIL_CAPABLE) {
		u32 esdp = IXGBE_READ_REG(hw, IXGBE_ESDP);
		if (esdp & IXGBE_ESDP_SDP1)
			DPRINTK(PROBE, CRIT,
				"Fan has stopped, replace the adapter\n");
	}

#ifdef MY_ABC_HERE
	hw->phy.reset_if_overtemp = true;
#endif  
	err = hw->mac.ops.reset_hw(hw);
#ifdef MY_ABC_HERE
	hw->phy.reset_if_overtemp = false;
#endif  
	if (err == IXGBE_ERR_SFP_NOT_PRESENT &&
	    hw->mac.type == ixgbe_mac_82598EB) {
		 
		set_bit(__IXGBE_SFP_MODULE_NOT_FOUND, &adapter->state);
		mod_timer(&adapter->sfp_timer,
			  round_jiffies(jiffies + (2 * HZ)));
		err = 0;
	} else if (err == IXGBE_ERR_SFP_NOT_SUPPORTED) {
		dev_err(&adapter->pdev->dev, "failed to initialize because "
			"an unsupported SFP+ module type was detected.\n"
			"Reload the driver after installing a supported "
			"module.\n");
		goto err_sw_init;
	} else if (err) {
		dev_err(&adapter->pdev->dev, "HW Init failed: %d\n", err);
		goto err_sw_init;
	}

	netdev->features = NETIF_F_SG |
	                   NETIF_F_IP_CSUM |
	                   NETIF_F_HW_VLAN_TX |
	                   NETIF_F_HW_VLAN_RX |
	                   NETIF_F_HW_VLAN_FILTER;

	netdev->features |= NETIF_F_IPV6_CSUM;
	netdev->features |= NETIF_F_TSO;
	netdev->features |= NETIF_F_TSO6;
	netdev->features |= NETIF_F_GRO;

	if (adapter->hw.mac.type == ixgbe_mac_82599EB)
		netdev->features |= NETIF_F_SCTP_CSUM;

	netdev->vlan_features |= NETIF_F_TSO;
	netdev->vlan_features |= NETIF_F_TSO6;
	netdev->vlan_features |= NETIF_F_IP_CSUM;
	netdev->vlan_features |= NETIF_F_IPV6_CSUM;
	netdev->vlan_features |= NETIF_F_SG;

	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED)
		adapter->flags &= ~IXGBE_FLAG_RSS_ENABLED;

#ifdef CONFIG_IXGBE_DCB
	netdev->dcbnl_ops = &dcbnl_ops;
#endif

#ifdef IXGBE_FCOE
	if (adapter->flags & IXGBE_FLAG_FCOE_CAPABLE) {
		if (hw->mac.ops.get_device_caps) {
			hw->mac.ops.get_device_caps(hw, &device_caps);
			if (device_caps & IXGBE_DEVICE_CAPS_FCOE_OFFLOADS)
				adapter->flags &= ~IXGBE_FLAG_FCOE_CAPABLE;
		}
	}
#endif  
	if (pci_using_dac)
		netdev->features |= NETIF_F_HIGHDMA;

	if (adapter->flags2 & IXGBE_FLAG2_RSC_ENABLED)
		netdev->features |= NETIF_F_LRO;

	if (hw->eeprom.ops.validate_checksum(hw, NULL) < 0) {
		dev_err(&pdev->dev, "The EEPROM Checksum Is Not Valid\n");
		err = -EIO;
		goto err_eeprom;
	}

	memcpy(netdev->dev_addr, hw->mac.perm_addr, netdev->addr_len);
	memcpy(netdev->perm_addr, hw->mac.perm_addr, netdev->addr_len);

	if (ixgbe_validate_mac_addr(netdev->perm_addr)) {
		dev_err(&pdev->dev, "invalid MAC address\n");
		err = -EIO;
		goto err_eeprom;
	}

	init_timer(&adapter->watchdog_timer);
	adapter->watchdog_timer.function = &ixgbe_watchdog;
	adapter->watchdog_timer.data = (unsigned long)adapter;

	INIT_WORK(&adapter->reset_task, ixgbe_reset_task);
	INIT_WORK(&adapter->watchdog_task, ixgbe_watchdog_task);

	err = ixgbe_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

	switch (pdev->device) {
	case IXGBE_DEV_ID_82599_KX4:
		adapter->wol = (IXGBE_WUFC_MAG | IXGBE_WUFC_EX |
		                IXGBE_WUFC_MC | IXGBE_WUFC_BC);
		 
		IXGBE_WRITE_REG(hw, IXGBE_GRC,
		             (IXGBE_READ_REG(hw, IXGBE_GRC) & ~IXGBE_GRC_APME));
		break;
	default:
		adapter->wol = 0;
		break;
	}
	device_set_wakeup_enable(&adapter->pdev->dev, adapter->wol);

	hw->mac.ops.get_bus_info(hw);

	dev_info(&pdev->dev, "(PCI Express:%s:%s) %pM\n",
	        ((hw->bus.speed == ixgbe_bus_speed_5000) ? "5.0Gb/s":
	         (hw->bus.speed == ixgbe_bus_speed_2500) ? "2.5Gb/s":"Unknown"),
	        ((hw->bus.width == ixgbe_bus_width_pcie_x8) ? "Width x8" :
	         (hw->bus.width == ixgbe_bus_width_pcie_x4) ? "Width x4" :
	         (hw->bus.width == ixgbe_bus_width_pcie_x1) ? "Width x1" :
	         "Unknown"),
	        netdev->dev_addr);
	ixgbe_read_pba_num_generic(hw, &part_num);
	if (ixgbe_is_sfp(hw) && hw->phy.sfp_type != ixgbe_sfp_type_not_present)
		dev_info(&pdev->dev, "MAC: %d, PHY: %d, SFP+: %d, PBA No: %06x-%03x\n",
		         hw->mac.type, hw->phy.type, hw->phy.sfp_type,
		         (part_num >> 8), (part_num & 0xff));
	else
		dev_info(&pdev->dev, "MAC: %d, PHY: %d, PBA No: %06x-%03x\n",
		         hw->mac.type, hw->phy.type,
		         (part_num >> 8), (part_num & 0xff));

	if (hw->bus.width <= ixgbe_bus_width_pcie_x4) {
		dev_warn(&pdev->dev, "PCI-Express bandwidth available for "
		         "this card is not sufficient for optimal "
		         "performance.\n");
		dev_warn(&pdev->dev, "For optimal performance a x8 "
		         "PCI-Express slot is required.\n");
	}

	hw->eeprom.ops.read(hw, 0x29, &adapter->eeprom_version);

	err = hw->mac.ops.start_hw(hw);

	if (err == IXGBE_ERR_EEPROM_VERSION) {
		 
		dev_warn(&pdev->dev, "This device is a pre-production "
		         "adapter/LOM.  Please be aware there may be issues "
		         "associated with your hardware.  If you are "
		         "experiencing problems please contact your Intel or "
		         "hardware representative who provided you with this "
		         "hardware.\n");
	}
	strcpy(netdev->name, "eth%d");
	err = register_netdev(netdev);
	if (err)
		goto err_register;

	netif_carrier_off(netdev);

	if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE ||
	    adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE)
		INIT_WORK(&adapter->fdir_reinit_task, ixgbe_fdir_reinit_task);

#ifdef MY_ABC_HERE
	if (adapter->flags2 & IXGBE_FLAG2_TEMP_SENSOR_CAPABLE)
		INIT_WORK(&adapter->check_overtemp_task, ixgbe_check_overtemp_task);
#endif  
#ifdef CONFIG_IXGBE_DCA
	if (dca_add_requester(&pdev->dev) == 0) {
		adapter->flags |= IXGBE_FLAG_DCA_ENABLED;
		ixgbe_setup_dca(adapter);
	}
#endif
	 
	ixgbe_add_sanmac_netdev(netdev);

	dev_info(&pdev->dev, "Intel(R) 10 Gigabit Network Connection\n");
	cards_found++;
	return 0;

err_register:
	ixgbe_release_hw_control(adapter);
	ixgbe_clear_interrupt_scheme(adapter);
err_sw_init:
err_eeprom:
	clear_bit(__IXGBE_SFP_MODULE_NOT_FOUND, &adapter->state);
	del_timer_sync(&adapter->sfp_timer);
	cancel_work_sync(&adapter->sfp_task);
	cancel_work_sync(&adapter->multispeed_fiber_task);
	cancel_work_sync(&adapter->sfp_config_module_task);
	iounmap(hw->hw_addr);
err_ioremap:
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_selected_regions(pdev, pci_select_bars(pdev,
	                             IORESOURCE_MEM));
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

static void __devexit ixgbe_remove(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	set_bit(__IXGBE_DOWN, &adapter->state);
	 
	clear_bit(__IXGBE_SFP_MODULE_NOT_FOUND, &adapter->state);
	del_timer_sync(&adapter->watchdog_timer);

	del_timer_sync(&adapter->sfp_timer);
	cancel_work_sync(&adapter->watchdog_task);
	cancel_work_sync(&adapter->sfp_task);
	cancel_work_sync(&adapter->multispeed_fiber_task);
	cancel_work_sync(&adapter->sfp_config_module_task);
	if (adapter->flags & IXGBE_FLAG_FDIR_HASH_CAPABLE ||
	    adapter->flags & IXGBE_FLAG_FDIR_PERFECT_CAPABLE)
		cancel_work_sync(&adapter->fdir_reinit_task);
	flush_scheduled_work();

#ifdef CONFIG_IXGBE_DCA
	if (adapter->flags & IXGBE_FLAG_DCA_ENABLED) {
		adapter->flags &= ~IXGBE_FLAG_DCA_ENABLED;
		dca_remove_requester(&pdev->dev);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_DCA_CTRL, 1);
	}

#endif
#ifdef IXGBE_FCOE
	if (adapter->flags & IXGBE_FLAG_FCOE_ENABLED)
		ixgbe_cleanup_fcoe(adapter);

#endif  

	ixgbe_del_sanmac_netdev(netdev);

	if (netdev->reg_state == NETREG_REGISTERED)
		unregister_netdev(netdev);

	ixgbe_clear_interrupt_scheme(adapter);

	ixgbe_release_hw_control(adapter);

	iounmap(adapter->hw.hw_addr);
	pci_release_selected_regions(pdev, pci_select_bars(pdev,
	                             IORESOURCE_MEM));

	DPRINTK(PROBE, INFO, "complete\n");

	free_netdev(netdev);

	pci_disable_pcie_error_reporting(pdev);

	pci_disable_device(pdev);
}

static pci_ers_result_t ixgbe_io_error_detected(struct pci_dev *pdev,
                                                pci_channel_state_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	netif_device_detach(netdev);

	if (state == pci_channel_io_perm_failure)
		return PCI_ERS_RESULT_DISCONNECT;

	if (netif_running(netdev))
		ixgbe_down(adapter);
	pci_disable_device(pdev);

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t ixgbe_io_slot_reset(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	pci_ers_result_t result;
	int err;

	if (pci_enable_device_mem(pdev)) {
		DPRINTK(PROBE, ERR,
		        "Cannot re-enable PCI device after reset.\n");
		result = PCI_ERS_RESULT_DISCONNECT;
	} else {
		pci_set_master(pdev);
		pci_restore_state(pdev);
		pci_save_state(pdev);

		pci_wake_from_d3(pdev, false);

		ixgbe_reset(adapter);
		IXGBE_WRITE_REG(&adapter->hw, IXGBE_WUS, ~0);
		result = PCI_ERS_RESULT_RECOVERED;
	}

	err = pci_cleanup_aer_uncorrect_error_status(pdev);
	if (err) {
		dev_err(&pdev->dev,
		  "pci_cleanup_aer_uncorrect_error_status failed 0x%0x\n", err);
		 
	}

	return result;
}

static void ixgbe_io_resume(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev)) {
		if (ixgbe_up(adapter)) {
			DPRINTK(PROBE, INFO, "ixgbe_up failed after reset\n");
			return;
		}
	}

	netif_device_attach(netdev);
}

static struct pci_error_handlers ixgbe_err_handler = {
	.error_detected = ixgbe_io_error_detected,
	.slot_reset = ixgbe_io_slot_reset,
	.resume = ixgbe_io_resume,
};

static struct pci_driver ixgbe_driver = {
	.name     = ixgbe_driver_name,
	.id_table = ixgbe_pci_tbl,
	.probe    = ixgbe_probe,
	.remove   = __devexit_p(ixgbe_remove),
#ifdef CONFIG_PM
	.suspend  = ixgbe_suspend,
	.resume   = ixgbe_resume,
#endif
	.shutdown = ixgbe_shutdown,
	.err_handler = &ixgbe_err_handler
};

static int __init ixgbe_init_module(void)
{
	int ret;
	printk(KERN_INFO "%s: %s - version %s\n", ixgbe_driver_name,
	       ixgbe_driver_string, ixgbe_driver_version);

	printk(KERN_INFO "%s: %s\n", ixgbe_driver_name, ixgbe_copyright);

#ifdef CONFIG_IXGBE_DCA
	dca_register_notify(&dca_notifier);
#endif

	ret = pci_register_driver(&ixgbe_driver);
	return ret;
}

module_init(ixgbe_init_module);

static void __exit ixgbe_exit_module(void)
{
#ifdef CONFIG_IXGBE_DCA
	dca_unregister_notify(&dca_notifier);
#endif
	pci_unregister_driver(&ixgbe_driver);
}

#ifdef CONFIG_IXGBE_DCA
static int ixgbe_notify_dca(struct notifier_block *nb, unsigned long event,
                            void *p)
{
	int ret_val;

	ret_val = driver_for_each_device(&ixgbe_driver.driver, NULL, &event,
	                                 __ixgbe_notify_dca);

	return ret_val ? NOTIFY_BAD : NOTIFY_DONE;
}

#endif  
#ifdef DEBUG
 
char *ixgbe_get_hw_dev_name(struct ixgbe_hw *hw)
{
	struct ixgbe_adapter *adapter = hw->back;
	return adapter->netdev->name;
}

#endif
module_exit(ixgbe_exit_module);
