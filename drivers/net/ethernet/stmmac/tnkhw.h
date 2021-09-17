/*****************************************************************************
  This is the driver for the CreVinn TOE-NK-2G TCP Offload Engine.
  TOE-NK-2G incorporates a Synopsys Ethernet MAC core.

  Copyright (C) 2011 Emutex Ltd. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  Author: Dan O'Donovan <dan@emutex.com>

*******************************************************************************/

/*
 *  TNK hardware interface
 */

#ifndef __TNKHW_H__
#define __TNKHW_H__

#ifndef UNIT_TEST
#include <linux/types.h>
#include <linux/skbuff.h>
#endif

#include "common.h"
#include "tnkinfo.h"

#undef DBG_TX_DMA
#ifdef DBG_TX_DMA
#define dump_tx_dma_info(t) do { \
	int i; \
	struct tnk_ttx_dma_desc *desc; \
	pr_warn("t->head=0x%x, head_index=%d, " \
		"t->tail=0x%x, tail_index=%d, " \
		"t->head_overflow=%d, " \
		"t->rst_received=%d\n", \
		t->head, t->head % tnk_tx_fifo, \
		t->tail, t->tail % tnk_tx_fifo, \
		t->head_overflow, t->rst_received); \
	pr_warn("hw_own:"); \
	for (i = 0; i < tnk_tx_fifo; i++) { \
		desc = t->desc_list + i; \
		pr_warn("%d\t", desc->hw_own); \
	} \
	pr_warn("\n"); \
} while (0)
#endif

/* Enable the following #define for FPGA hardware platform only
 * Disable when using the ASIC hardware platform
 *
 * TODO - can we query a hardware register to find out if we're
 * running on FPGA or ASIC?
 */
#undef TNK_HW_PLATFORM_FPGA

/*  Currently, TOE-NK-2G does not support Jumbo frames */
#undef TNK_JUMBO_FRAMES_SUPPORTED

/*  TNK reserves the first 2 connections for internal use only */
#define TNK_TTX_CINDEX_START 2

#define TNK_MAX_RETRANS	(0x0FF)
#define TNK_MAX_DUPACK_CNT	(0xFF)
/* maybe need  a more bigger size for small packet sending */
#define TNK_DMA_TX_SIZE 32

/* TODO: debug switch, undefine when stable */
#undef TNK_DBG_CONN
#undef TNK_DBG_KEY_POINT
#define TNK_DBG_TTX_ERR

#if SWITCH_SEND_FIN
#define TNK_DBG_SEND_FIN
#endif

/* #define TNK_HW_DEBUG */
#undef TNK_HW_DEBUG
/*#define STMMAC_XMIT_DEBUG*/
#ifdef TNK_HW_DEBUG
#define TNK_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define TNK_DBG(fmt, args...)  do { } while (0)
#endif

#if SWITCH_RECV_LRO
/* #define TNK_RECV_LRO_DEBUG */
#undef TNK_RECV_LRO_DEBUG
#define TNK_RECV_LRO_DATA_DEBUG
#ifdef TNK_RECV_LRO_DEBUG
#define TNK_LRO_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define TNK_LRO_DBG(fmt, args...)  do { } while (0)
#endif
#endif /* SWITCH_RECV_LRO */

/* Add TNK bonjour function */
#define TNK_BONJOUR
#ifdef TNK_BONJOUR
/* #define TNK_BONJOUR_DEBUG */
#undef TNK_BONJOUR_DEBUG
#ifdef TNK_BONJOUR_DEBUG
#define TNKBD_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define TNKBD_DBG(fmt, args...)  do { } while (0)
#endif
#endif
/* Add TNK bonding function */
#define TNK_BONDING
#ifdef TNK_BONDING
/* #define TNK_BONDING_DEBUG */
#undef TNK_BONDING_DEBUG
#ifdef TNK_BONDING_DEBUG
#define TNKBD_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define TNKBD_DBG(fmt, args...)  do { } while (0)
#endif
#endif

#define CT_RX_ACK_NUM		6
#define CT_NEXT_TX_SEQ_NUM	7
#define	CT_NEXT_RX_SEQ_NUM	8
#define CT_KEEPALIVE_ACKED	9
/* The following structure assumes both TOE and host are
 * using the same endianness for memory accesses.
 */
struct  tnkhw_connection {
	/* Word 0 */
	uint32_t r_macaddr_u16:16;   /* Upper 16 bits of Remote MAC Address */
	uint32_t l_port:16;	     /* Local TCP Port Number */
	/* Word 1 */
	uint32_t r_macaddr_l32;	     /* Lower 32 bits of Remote MAC Address */
	/* Word 2 */
	uint32_t r_ipaddr;	     /* Remote IPv4 Address */
	/* Word 3 */
	uint32_t r_port:16;	     /* Remote TCP Port Number */
	uint32_t ttl:8;		     /* Time To Live */
	uint32_t tos:8;		     /* Type Of Service */
	/* Word 4 */
	uint32_t word4_flag:1;
	uint32_t word4_reserved3:3;
	uint32_t word4_count:8;
	uint32_t word4_reserved4:20;
	/* Word 5 */
	uint32_t word5_recoverseqnum;	     /* RESERVED */
	/* Word 6 */
	uint32_t rx_ack_num;	     /* Received Acknowledge Number */
	/* Word 7 */
	uint32_t next_tx_seq_num;    /* Next Tx Sequence Number */
	/* Word 8 */
	uint32_t next_rx_seq_num;    /* Next Rx Sequence Number */
	/* Word 9 */
	uint32_t dup_ack_cnt:8;	     /* Duplicate Acknowledge Count */
	uint32_t keepalive_acked:1;
	uint32_t reserved2:23;	     /* RESERVED */
	/* Word 10 */
	uint32_t rx_adv_wnd:16;	     /* Receive Advertised Window */
	uint32_t timer_tx_retry:1;   /* Timer Timeout Retry Flag */
	uint32_t reserved3:7;	     /* RESERVED */
	uint32_t retry_count:8;	     /* Retry Count */
	/* Word 11 */
#if SWITCH_ZERO_PROBE
	uint32_t persist_cnt_check:1;
	uint32_t persist_cnt:10;
	uint32_t probe_cnt:4;
	uint32_t probe_intr:1;
#else
	uint32_t reserved4:16;	     /* RESERVED */
#endif
	uint32_t tx_mss:13;	     /* Tx Maximum Segment Size */
	uint32_t reserved5:3;	     /* RESERVED */
	/* Word 12 */
	uint32_t adv_wnd_scale:4;    /* TX Advertised Window Scale */
	uint32_t flag_conn_enable:1; /* Enable the connection */
	uint32_t flag_mac_sel:1;     /* Select MAC 0 or 1 */
	uint32_t flag_ipv6:1;	     /* Enable IPv6 */
	uint32_t flag_disable_nagle:1; /* Disable Nagle algorithm */
#if SWITCH_THIN_STREAM
	uint32_t flag_thin_lto:1;
#else
	uint32_t flag_use_local_wnd:1; /* Use the register version of the window
					* size (not the connection table entry)
					*/
#endif
#if SWITCH_VLAN
	uint32_t flag_add_vlan_tag:1;
#else
	uint32_t flag_use_local_scl:1; /* Use the register version of the scale
					* (not the connection table entry)
					*/
#endif
	uint32_t flag_full_pkt:1;    /* Enable the full packet to be sent to the
				      * receive queue (including mgmt header) */
	uint32_t flag_cork:1;
	uint32_t flag_keepalive:1;
	uint32_t flag_fin:1;
	uint32_t tx_ack_irq:2;
	uint32_t tx_adv_wnd:16;	     /* RESERVED */
	/* Word 13 */
	uint32_t cong_wnd;	     /* Congestion Window */
	/* Word 14 */
	uint32_t timer_timestamp:16; /* Timer Timestamp */
	uint32_t timer_active:1;     /* Timer Active Flag */
	uint32_t reserved7:15;	     /* RESERVED */
	/* Word 15 */
	uint32_t timer_seq_num;	     /* Sequence Number of timed packets */
	/* Word 16 */
	uint32_t timeout_value:16;   /* Timeout Value in Milliseconds */
#if SWITCH_CORK
	uint32_t cork_timestamp:16;
#else
	uint32_t reserved8:16;	     /* RESERVED */
#endif
	/* Word 17 */
	uint32_t sampled_ack_num;    /* Timer Sampled Acknowledge Number */
	/* Word 18 */
	uint32_t timer_rx_fast_retry:1;	/* Timer Fast Retry Flag */
#if SWITCH_MULTI_INTR_RETRANS
	uint32_t reserved9_1:7;
	uint32_t retry_intr:1;
	uint32_t reserved9:23;	     /* RESERVED */
#else
	uint32_t reserved9:31;	     /* RESERVED */
#endif
	/* Word 19 */
	uint32_t retry_seq_num;	     /* Transmit Sequence Number to
				      * re-transmit */
	/* Word 20 */
	uint32_t en_rtt_calc:1;	     /* Round Trip Time Measurement Enable */
	uint32_t ss_threshold:31;    /* Slow-start threshold for congestion
				      * window calc */
	/* Word 21 */
	uint32_t tx_timestamp:16;    /* Transmit Timestamp */
	uint32_t reserved10:16;	     /* RESERVED */
	/* Word 22 */
	uint32_t timer_rtt:16;	     /* Round Trip Time */
	uint32_t timer_rtt_valid:1;  /* Round Trip Time Valid Flag */
	uint32_t reserved11:15;	     /* RESERVED */
	/* Word 23 */
	uint32_t last_retry_seq_num; /* Last Retry Sequence Number */
	/* Word 24 */
	uint32_t r_ipv6_addr_31_0;   /* Remote IPv6 address [31:0] */
	/* Word 25 */
	uint32_t r_ipv6_addr_63_32;  /* Remote IPv6 address [63:32] */
	/* Word 26 */
	uint32_t r_ipv6_addr_95_64;  /* Remote IPv6 address [95:64] */
	/* Word 27 */
	uint32_t r_ipv6_addr_127_96; /* Remote IPv6 address [127:96] */
	/* Word 28 */
	uint32_t r_ipv6_flow_label:20; /* IPv6 Flow Label */
	uint32_t reserved12:12;	     /* RESERVED */
	/* Word 29 */
#if SWITCH_NAGLE
	uint32_t nagle_seq_num;
#else
	uint32_t word29_reserved0;	     /* RESERVED */
#endif
	/* Word 30 */
	uint32_t word30_updated_tx_ack_num;  /* New rx seq num */
	/* Word 31 */
	uint32_t word31_updated_flag:1; /* Rx seq num update flag */
	uint32_t word31_reserved:31;    /* RESERVED */
};

struct tnkhw_stats {
	/* Number of interrupts with ttx_ack interrupt status bit set */
	uint32_t dma_ttx_ack_irq;
	/* Number of interrupts with ttx_ack interrupt status bit set */
	uint32_t dma_ttx_err_irq;
	/* Number of interrupts with dma_ch2 interrupt status bit set */
	uint32_t dma_ch2_irq;
	/* Number of interrupts with toe interrupt status bit set */
	uint32_t toe_irq;
	/* Number of NAPI poll operations */
	uint32_t polls;
	/* Number of DMA_TTX_ACKD_CID FIFO overflows */
	uint32_t ttx_ack_ovflow;
	/* Number of empty buffers added to the RX DMA queue */
	uint32_t rx_refills;
	uint32_t rx_refills_alloc;
#if SWITCH_RECV_LRO
	atomic_t rx_lro_pages;
#endif
	/* Number of empty buffers returned from the TTX DMA queues */
	uint32_t tx_returns;
};

/* Per-connection statistics */
struct tnkhw_conn_stats {
	/* *** Normal stats */
	/* Number of descriptors submitted to the TOE for transmission */
	uint32_t tx_desc_sent;
	/* Number of descriptors transmitted successfully by the TOE */
	uint32_t tx_desc_ackd;
	/* Number of bytes submitted to the TOE for transmission */
	uint32_t tx_bytes_sent;
	/* Number of bytes transmitted successfully by the TOE */
	uint32_t tx_bytes_ackd;
	/* Number of packets rejected due to transmit queue overflow  */
	uint32_t tx_overflow;
#if SWITCH_SEND_FIN
	uint32_t tx_fin_failed;
#endif
	/* Number of packets received from the TOE */
	uint32_t rx_pkts;
	/* Number of packets received with errors and consequently dropped */
	uint32_t rx_dropped;

	/* *** TOE Transmit errors */
	/* Data Fetch Error.  AXI error response recieved on descriptor read */
	uint32_t tx_dfetch_desc;
	/* Data Fetch Error.  AXI error response recieved on data read */
	uint32_t tx_dfetch_data;
	/* Data Fetch Error.  Own bit not set when descriptor is read and data
	 * requested */
	uint32_t tx_dfetch_own;
	/* TCP Retry error.  AXI error response received on read of a
	 * descriptor */
	uint32_t tx_retry_desc;
	/* TCP Retry error.  No data to retry */
	uint32_t tx_retry_data;
	/* Acknowledge data error.  AXI error response received on read of
	 * a descriptor */
	uint32_t tx_ackd_read;
	/* Acknowledge data error.  Acknowledge size received is greater than
	 * the amount of data available to acknowledge */
	uint32_t tx_ackd_size;
	/* Acknowledge data error.  AXI error response received on write to a
	 * descriptor */
	uint32_t tx_ackd_write;

	/* *** TOE Receive errors */
	/* Premature end of packet detected */
	uint32_t rx_premature_eop;
	/* MAC type field doesn not indicate IPv4 */
	uint32_t rx_bad_ethertype;
	/* Destination MAC or IP addresses in packet did not match ours */
	uint32_t rx_bad_dst_macaddr;
	/* TCP checksum error */
	uint32_t rx_tcp_csum_err;
	/* IP header checksum error */
	uint32_t rx_iphdr_csum_err;
	/* Non-TCP protocol */
	uint32_t rx_non_tcp_prot;
	/* Destination IP Addresses in packet did not match ours */
	uint32_t rx_bad_dst_ipaddr;
	/* Connection error (need to check what this covers) */
	uint32_t rx_connection_err;
	/* Received TCP Sequence Number was out of order */
	uint32_t rx_bad_seq_num;
	/* Packet dropped because RxEngine was busy or FIFO full */
	uint32_t rx_busy_pkt_drop;
#if SWITCH_TOE_ERR_PKT
	/* Packet dropped because seq out of window */
	uint32_t rx_out_of_window;
#endif
	/* Errored frame. If this bit is set then the data received should be
	 * dropped by software. The data has not been acknowledged by the TCP
	 * offload engine */
	uint32_t rx_toe_err;

	/* *** Stats counters maintained by the TOE */
	/* Number of bytes acknowledged for this channel */
	uint32_t tx_acked_count;
	/* Number of bytes re-transmitted for this channel */
	uint32_t tx_retry_count;
	/* Number of bytes received for this channel */
	uint32_t rxed_byte_count;

	/* *** Live counter snapshots */
	/* Number of bytes currently in TTX pipeline for this channel */
	uint32_t ttx_pipelined_bytes;
	/*the tx block num */
	uint32_t tx_block_num;
	/*the tx max data size*/
	uint32_t tx_max_data_size;
#if SWITCH_RECV_LRO
	/* the times of flush tx_adv_wnd from db to ct */
	uint32_t tx_adv_wnd_flush;
	/* the tx_adv_wnd wscale */
	uint32_t tx_adv_wnd_scale;
	/* the tx_adv_wnd read from db */
	uint32_t tx_adv_wnd_db;
	/* the tx_adv_wnd read from ct */
	uint32_t tx_adv_wnd_ct;
	/* the tx_adv_wnd sync failed */
	uint32_t tx_adv_wnd_fail;
#endif
};

/*  TODO - the following 2 structs are here for debug only.
 *  Move back into tnkhw.c later */
struct tnkhw_rx_dma_info {
	/* DMA descriptor list (virt address) */
	struct tnk_rx_dma_desc *desc_list;
	/* DMA descriptor list (phys address) */
	dma_addr_t desc_list_phys;
	/* DMA Rx skbuff pool */
	struct sk_buff **skbuff;
	/* DMA Rx skbuff data pointers */
	dma_addr_t *skbuff_dma;
	/* Next Rx buffer to read */
	unsigned int head;
	/* Next Rx buffer to be freed */
	unsigned int tail;
	/* Spinlock to to protect head and tail pointer */
	spinlock_t rx_desc_lock;
	/* skbuff recycling list */
	struct sk_buff_head free_skbs;
};

#if SWITCH_RECV_LRO
struct tnkhw_rx_dma_channel {
	/* DMA descriptor list (virt address) */
	struct tnk_rx_desc *desc_list;
	/* DMA descriptor list (phys address) */
	dma_addr_t desc_list_phys;
	/* DMA Rx skbuff pool */
	struct sk_buff **skbuff;
	/* DMA Rx skbuff data pointers */
	dma_addr_t *skbuff_dma;
	/* Next Rx buffer to read */
	unsigned int head;
	/* Next Rx buffer to be freed */
	unsigned int tail;
	/* software rcv_nxt */
	unsigned int tp_rcv_nxt;
	/* socket */
	struct sock *sk;
	/* socket rcvbuf */
	int rcvbuf_desc_cnt;
	/* total desc count */
	int total_desc_cnt;
	/* Spinlock to to protect head and tail pointer */
	spinlock_t rx_desc_lock;
	unsigned int channel_valid;
};
#endif

struct tnkhw_tx_dma_info {
	/* DMA descriptor list (virt address) */
	struct tnk_ttx_dma_desc *desc_list;
	/* DMA descriptor list (phys address) */
	dma_addr_t desc_list_phys;
	/* DMA Tx skbuff pool */
	struct sk_buff **skbuff;
	/* DMA Tx page pool */
	struct page **page;
	/* Next Tx buffer to read */
	unsigned int head;
	/* Next Tx buffer to be freed */
	unsigned int tail;
	/* Number of bytes currently in the pipeline */
	atomic_t pipelined_bytes;
	/* Waitqueue for sleeping during graceful tx shutdown */
	wait_queue_head_t waitqueue;
	/* Flag to indicate that TX overflow occurred. */
	int overflow;
	/* Flag indicating receipt of RST packet for this connection. */
	int rst_received;
	/* Spinlock to to protect head and tail pointer */
	spinlock_t tx_desc_lock;
#ifdef DBG_TX_DMA
	int head_overflow;
#endif
#if SWITCH_SEND_FIN
	unsigned int tx_snd_nxt;
	unsigned int last_data_size;
#endif
#ifdef TNK_DBG_SEND_FIN
	atomic_t fin_sent;
#endif
};

void tnkhw_config_tcp_retries2(int tcp_retries2);
#if SWITCH_DUPACK_NUM
void tnkhw_config_tcp_dupack_cnt(int tcp_reordering);
#endif
void tnk_set_thin_linear_timeouts(int thin_linear_timeouts);
#if SWITCH_THIN_STREAM
void tnkhw_set_thin_linear_timeouts(unsigned int cindex,
		unsigned int thin_lto);
#endif
int tnkhw_init(void __iomem *ioaddr, unsigned int max_connections,
	       struct device *dev);
void tnkhw_shutdown(void);

void tnkhw_interrupt(int source_mask);
#if SWITCH_MULTI_INTR
extern unsigned int stmmac_irq_num[];
void tnkhw_channel_interrupt(int channel, unsigned int source_mask);
#endif

#define TNK_OWN_BIT	0x80000000

typedef void (*tnkhw_poll_wakeup_cb_t) (void);
typedef void (*tnkhw_rx_cb_t) (struct sk_buff *skb, unsigned cindex,
			       unsigned long flags, unsigned short urgent_ptr);
#if SWITCH_RECV_LRO
typedef void (*tnkhw_lro_rx_cb_t) (struct sk_buff *skb, unsigned cindex);
#endif
#if SWITCH_SEND_FIN
typedef void (*tnkhw_txfree_cb_t) (unsigned cindex,
				unsigned acked_bytes, bool fin_acked);
#else
typedef void (*tnkhw_txfree_cb_t) (unsigned cindex, unsigned acked_bytes);
#endif
typedef void (*tnkhw_tx_max_retries_cb_t) (unsigned cindex);
#if SWITCH_MULTI_INTR
typedef void (*tnkhw_channel_poll_wakeup_cb_t) (unsigned int channel);
#endif

extern struct tnkhw_conn_stats *tnk_conn_stats;

/* Register an RX event callback, which will be invoked whenever a packet is
 * received from the TOE.
 * Register an TX event callback, which will be invoked whenever a TX overflow
 * condition exists for the specified channel AND data has subsequently been
 * transmitted successfully by the TOE, thereby freeing up some space in the
 * TX queue
 * Start the DMA engine
 * This MUST be called before adding any connections
 */
int tnkhw_dma_start(tnkhw_poll_wakeup_cb_t poll_wakeup_cb,
#if SWITCH_MULTI_INTR
		tnkhw_channel_poll_wakeup_cb_t channel_poll_wakeup_cb,
#endif
#if SWITCH_RECV_LRO
		tnkhw_lro_rx_cb_t lro_rx_cb,
#endif
		    tnkhw_rx_cb_t rx_cb,
		    tnkhw_txfree_cb_t tx_cb,
		    tnkhw_tx_max_retries_cb_t tx_max_retries_cb);
void tnkhw_dma_stop(void);

int tnkhw_poll(int budget);

/* Transmit a packet on an TOE-accelerated connection */
int tnkhw_tx_send(struct sk_buff *skb, unsigned cindex);

#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
uint32_t select_ipv4_pos_to_logic(int gmac, uint32_t addr);
uint32_t select_ipv6_pos_to_logic(int gmac, uint32_t *ipv6_addr);
void tnk_ct_ip_position_sel(int local_ip_pos);
void tnk_ct_ip_position_sel_extend(int index, int local_ip_pos);
#endif
/* Add a connection for TOE acceleration.
 * Writes entry in TOE connection table, and adds entry to TOE lookup hash
 */
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
int tnkhw_connection_add(unsigned cindex, struct tnkhw_connection *entry,
		int local_ip_pos);
#else
int tnkhw_connection_add(unsigned cindex, struct tnkhw_connection *entry);
#endif

/* Withdraw a connection from TOE acceleration.
 * Deletes entry from TOE lookup hash
 */
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
int tnkhw_connection_remove(unsigned cindex, struct tnkhw_connection *entry,
		int local_ip_pos, int ct_hw_removed);
#else
int tnkhw_connection_remove(unsigned cindex, struct tnkhw_connection *entry,
		int ct_hw_removed);
#endif
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
int tnkhw_connection_remove_hw(unsigned cindex, struct tnkhw_connection *entry,
		int local_ip_pos);
#else
int tnkhw_connection_remove_hw(unsigned cindex, struct tnkhw_connection *entry);
#endif
/* Read connection entry details from TOE connection table */
int tnkhw_connection_get(unsigned cindex, struct tnkhw_connection *entry);
int tnkhw_connection_get_entry(unsigned cindex, struct tnkhw_connection *entry,
		unsigned int start, unsigned int end);

/* Print connection entry on the console (for debug only) */
void tnkhw_connection_dump(unsigned cindex);
#ifdef TNK_DBG_TTX_ERR
void tnkhw_connection_print(struct tnkhw_connection *conn);
#endif

/* Set/unset the enable flag in a connection table entry */
void tnkhw_connection_enable(unsigned cindex);
void tnkhw_connection_disable(unsigned cindex);

/* Configures the local mac address for a specified GMAC
 * This MUST be called before adding any connections
 * (typically called from stmmac_main.c whenever the MAC address is updated)
 */
int tnkhw_macaddress_set(unsigned char *mac_addr, int gmac);

int tnkhw_check_vlan(int gmac, int pos_id);

/* Configures a local IPv4 address for a specified GMAC
 * This MUST be called before adding any connections, and
 * again if the local IPv4 address is subsequently changed.
 */
int tnkhw_ipv4address_set(uint8_t *ipv4_addr, int gmac,
			u16 vlan_id, uint8_t *mac_addr);
#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
int tnkhw_ipv4address_rst(uint8_t *ipv4_addr, int gmac);
#endif
/* Configures a local IPv6 address for a specified GMAC
 * This MUST be called before adding any connections, and
 * again if the local IPv6 address is subsequently changed.
 */
int tnkhw_ipv6address_set(uint8_t *ipv6_addr, int gmac);
#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
int tnkhw_ipv6address_rst(uint8_t *ipv6_addr, int gmac);
#endif
void tnkhw_stats_get(struct tnkhw_stats *stats);
void tnkhw_conn_stats_get(unsigned cindex, struct tnkhw_conn_stats *stats);
void tnkhw_stats_clearall(void);

void *tnkhw_ioaddr_get(void);

/* For use by interrupt polling mechanism only */
#if SWITCH_MULTI_INTR
int tnkhw_rx(int limit, unsigned int channel);
int tnkhw_tx_reclaim(unsigned int channel);
void tnkhw_rx_refill(unsigned int channel);
#else
int tnkhw_rx(int limit);
int tnkhw_tx_reclaim(void);
void tnkhw_rx_refill(void);
#endif
void tnkhw_tx_channel_reclaim(unsigned cindex, int shutdown);
void tnkhw_skb_recycle(struct sk_buff *skb);
void tnkhw_ctrl_interrupt(int source_mask);
int tnkhw_dma_has_work(void);
void tnkhw_dma_interrupt_disable(void);
void tnkhw_dma_interrupt_enable(void);

void tnkhw_tx_err_dump_info(int cid);

#if SWITCH_MULTI_INTR
int tnkhw_dma_channel_has_work(unsigned int channel);
void tnkhw_dma_channel_interrupt_disable(unsigned int channel);
void tnkhw_dma_channel_interrupt_enable(unsigned int channel);
#endif

#if SWITCH_KEEPALIVE
int tnkhw_check_tx_dma(unsigned int cindex);
#endif

void tnkhw_inc_persist(void);

int tnkhw_tx_done_test(unsigned cindex);
int tnkhw_tx_done_wait(unsigned cindex, unsigned timeout_jiffies);
void tnkhw_rcv_rst(unsigned cindex);

int tnkhw_rtt_sample(unsigned cindex);
void tnkhw_txtimeout_update(unsigned cindex, unsigned timeout_ms);
#ifdef TNK_RX_CHANNEL_FLOW_CONTROL
void tnkhw_txadvwnd_update(unsigned cindex, unsigned tx_adv_wnd);
#endif

void tnkhw_update_fin_seqnum(unsigned int cindex);
void tnkhw_update_rx_seqnum(unsigned int cindex, unsigned int seq_num);
void tnk_hw_proc(struct seq_file *s);

void tnkhw_ct_read(unsigned cindex, uint8_t loc, uint32_t *data);
void tnkhw_ct_write(unsigned cindex, uint8_t loc, uint32_t data);
#ifdef TNK_BONDING
extern void tnkhw_bonding_enable(int enable);
extern void tnkhw_bonding_setmode(int mode);
extern void tnkhw_bonding_setcurr_active_slave(int curr_slave);
extern void tnkhw_bonding_setstatus_slave(int gmac, int enable);
void sync_ipv4_pos_to_logic(uint32_t addr);
void sync_ipv6_pos_to_logic(uint32_t *addr);
int is_bonding_enable(void);
int curr_bonding_dev(void);
int tnkhw_bonding_getmode(void);
int curr_active_dev(void);
#endif

void tnkhw_set_tx_mss(unsigned int cindex, unsigned int mss_now);

#if SWITCH_SEND_ACK
void tnkhw_send_ack(unsigned int cindex);
#endif
void tnkhw_get_txadvwnd_scale(unsigned int cindex, unsigned int *rcv_wscale);
void tnkhw_set_txadvwnd_scale(unsigned int cindex, unsigned int rcv_wscale);
#if SWITCH_NAGLE || SWITCH_CORK
void tnkhw_set_nagle_cork(unsigned int cindex,
		unsigned int cork, unsigned int nagle_off);
#endif
#if SWITCH_KEEPALIVE
void tnkhw_set_keepalive_flag(unsigned int cindex, unsigned int keepalive);
void tnkhw_write_alive_done_flag(unsigned int cindex, unsigned int val);
#endif
#if SWITCH_SEND_FIN
void tnkhw_set_fin_flag(unsigned int cindex, unsigned int fin);
int tnkhw_get_fin_send_state(unsigned int cindex);
void tnkhw_set_fin_send_state(unsigned int cindex, int val);
#endif
#if SWITCH_MULTI_INTR
void tnkhw_set_rx_irq(unsigned int cindex, unsigned int cpu);
void tnkhw_set_tx_ack_irq(unsigned int cindex, unsigned int cpu);
#endif
#if SWITCH_ZERO_PROBE
int tnkhw_check_max_retries(unsigned int cindex);
void tnkhw_set_zero_probe_cnt(unsigned int cindex, unsigned int val);
unsigned int tnkhw_get_zero_probe_cnt(unsigned int cindex);
#endif

#if SWITCH_RECV_LRO
int tnkhw_rx_channel_create(struct sock *sk);
int tnkhw_rx_lro_cleanup(unsigned int cid);
int tnkhw_rx_lro(unsigned int channel);
int tnkhw_lro_check_refill(unsigned int cid, struct sk_buff *skb);
void tnkhw_rx_lro_refill(unsigned int cid, unsigned int limit);
void tnk_ct_ofo_queue(unsigned cindex);
int tnk_ct_hold_list_lock(unsigned int cindex);
int tnk_ct_release_list_lock(unsigned int cindex);
void tnkhw_lro_free_skb(struct sk_buff *skb);
bool tnkhw_check_tx_rx_zero_wnd(unsigned int cid);
void tnkhw_sync_rx_wnd_from_db_to_ct(unsigned int cid);
#endif

#ifdef TNK_RECV_LRO_DATA_DEBUG
void tnk_print_memory(char *buf, int len);
#endif

#ifdef TNK_DBG_HW_INTERFACE
void tnkhw_ct_access(int ct_access_val[]);
void tnkhw_lookup_access(int lookup_access_val[]);
#endif
#endif /*  __TNKHW_H__ */
