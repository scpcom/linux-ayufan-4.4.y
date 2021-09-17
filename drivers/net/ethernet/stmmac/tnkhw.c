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

  Authors: Dan O'Donovan <dan@emutex.com>

*******************************************************************************/

/*
 *  TNK hardware interface
 */

#include <linux/io.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "tnkct.h"
#include "tnkhw.h"
#include "tnkmem.h"
#include "tnkhw_regmap.h"
#include "tnkinfo.h"
#include "tnksysctl.h"
#include "common.h"
#include "descs.h"
#include "dwmac1000.h"
#ifdef CONFIG_ARCH_HI3536
#include "tnk_hi3536.h"
#else
#ifdef CONFIG_ARCH_HI3535
#include "tnk_hi3535.h"
#endif
#endif
#include "tnktcp.h"
/*  TODO - for debug only, remove later */
#undef TNK_TTX_FLUSH_DELAYED_RECLAIM
#define TNK_TTX_BUF_RECLAIMED 0x00000BAD

#if SWITCH_RECV_LRO
#define TNK_DMA_RX_SIZE  STMMAC_ALIGN(256)
#elif SWITCH_MULTI_INTR
#define TNK_DMA_RX_SIZE  STMMAC_ALIGN(2048)
#else
#define TNK_DMA_RX_SIZE  STMMAC_ALIGN(4096)
#endif

#define TNK_DMA_RX_RECYCLE_SIZE		(16)

#define TNK_DMA_RX_DMA_COUNT   STMMAC_ALIGN(256)
#define TNK_RX_PAGE_ORDER	0
#define TNK_RX_RESERVED_SIZE	(16 * 1024)
#define TNK_RXFETCH_MAX_TIMES	3
#define TNK_LRO_MIN_DESC_CNT	2

#define TNK_TCP_HEADER_RESERVED (32)

/* Maximum amount of outstanding data in TTX pipeline for any single connection
 * Note that the actual limit imposed by the TOE is 16MB.  For now, we limit it
 * further to just 1MB here to impose a stricter ceiling on memory usage */
#define TNK_TTX_DATA_LIMIT ((1 << 24) - 1)
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
#define TNK_IP_MAX_NUM 4
static int eth0_ipv4_cnt;
static int eth1_ipv4_cnt;
static int eth0_ipv6_cnt;
static int eth1_ipv6_cnt;
#endif

/* Abbreviated masks for interrupt source register */
#if SWITCH_RECV_LRO
#define TNK_INTR_RX_LRO0 \
	(TNK_MASK_INTR_RX_LRO0 << TNK_OFFSET_INTR_RX_LRO0)
#endif
#define TNK_INTR_TRX \
	(TNK_MASK_INTR_STAT_DMA_CH2 << TNK_OFFSET_INTR_STAT_DMA_CH2)
#define TNK_INTR_TOE \
	(TNK_MASK_INTR_STAT_TOE << TNK_OFFSET_INTR_STAT_TOE)
#define TNK_INTR_TTX_ACK \
	(TNK_MASK_INTR_STAT_DMA_TTX_ACK << TNK_OFFSET_INTR_STAT_DMA_TTX_ACK)
#define TNK_INTR_TTX_ERR \
	(TNK_MASK_INTR_STAT_DMA_TTX_ERR << TNK_OFFSET_INTR_STAT_DMA_TTX_ERR)

#if SWITCH_RECV_LRO
#define TNK_INTR_RX_LRO(i) \
	(TNK_MASK_INTR_RX_LRO1 << (TNK_OFFSET_INTR_RX_LRO1 + i - 1))
#endif
#define TNK_INTR_RX_CHANNEL(i) \
	(TNK_MASK_INTR_STAT_RX1 << (TNK_OFFSET_INTR_STAT_RX1 + i - 1))
#define TNK_INTR_TX_ACK_CHANNEL(i) \
	(TNK_MASK_INTR_STAT_TTX_ACK1 << (TNK_OFFSET_INTR_STAT_TTX_ACK1 + i - 1))

/* Macro for validating cindex parameter */
#define TNK_CINDEX_VALIDATE(cindex) \
	BUG_ON(((cindex) < TNK_TTX_CINDEX_START) \
	       || ((cindex) >= tnk_max_connections));

#define TNK_CINDEX_VALIDATE_WARN(cindex) \
	WARN_ON(((cindex) < TNK_TTX_CINDEX_START) \
	       || ((cindex) >= tnk_max_connections))

#define TNK_CINDEX_NOT_VALIDATE(cindex) \
	(((cindex) < TNK_TTX_CINDEX_START) \
	       || ((cindex) >= tnk_max_connections))

#define TNKHW_CT_LOC_RXADVWND 10
#define TNKHW_CT_LOC_TXADVWND 12

/*  io-remapped base address for GMAC and TOE register map */
static void __iomem *tnkhw_ioaddr;
static struct device *tnk_dev;
static tnkhw_poll_wakeup_cb_t tnk_pollwakeup;
static tnkhw_rx_cb_t tnk_rxcallback;
static tnkhw_txfree_cb_t tnk_txcallback;
static tnkhw_tx_max_retries_cb_t tnk_txmaxretries;
#if SWITCH_MULTI_INTR
static tnkhw_channel_poll_wakeup_cb_t tnk_channel_pollwakeup;
#endif
#if SWITCH_RECV_LRO
static tnkhw_lro_rx_cb_t tnk_lro_rxcallback;
#endif

#ifdef TNK_DBG_TTX_ERR
static struct tnkhw_connection last_conn[512];
#endif

/* Maximum number of TOE connections supported */
static unsigned int tnk_max_connections;

int tnk_tx_fifo = TNK_DMA_TX_SIZE;
module_param(tnk_tx_fifo, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tnk_tx_fifo, "min:16, max:1024, default:32");

int tnk_rx_fifo = TNK_DMA_RX_SIZE;
module_param(tnk_rx_fifo, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tnk_rx_fifo, "min:256, max:8192, default:4096");

int tnk_rx_recycle_fifo = TNK_DMA_RX_RECYCLE_SIZE;
module_param(tnk_rx_recycle_fifo, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tnk_rx_recycle_fifo, "min:16, max:4096, default:16");

#if SWITCH_RECV_LRO
int tnk_rx_dma_count = TNK_DMA_RX_DMA_COUNT;
module_param(tnk_rx_dma_count, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tnk_rx_dma_count, "min:16, max:2048, default:512");

int tnk_rx_page_order = TNK_RX_PAGE_ORDER;
module_param(tnk_rx_page_order, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tnk_rx_page_order, "min:0, max:2, default:1");
#endif

#ifdef TNK_DEBUG
struct tnkhw_rx_dma_info tnk_rx_dma;
struct tnkhw_tx_dma_info *tnk_tx_dma_list;
#else
#if SWITCH_MULTI_INTR
static struct tnkhw_rx_dma_info tnk_rx_dma[TOE_MULTI_INTR_NUM];
#else
static struct tnkhw_rx_dma_info tnk_rx_dma;
#endif
#if SWITCH_RECV_LRO
static struct tnkhw_rx_dma_channel *tnk_rx_dma_channel;
#endif
static struct tnkhw_tx_dma_info *tnk_tx_dma_list;
#endif

static struct tnkhw_stats tnk_stats;
struct tnkhw_conn_stats *tnk_conn_stats;
static struct stmmac_extra_stats tnk_stmmac_stats;

static int tnk_flush_pending;

static int count_skb;

static spinlock_t tnkhw_reg_lock;
static spinlock_t tnkhw_reg_ack_cmd_lock;

static void tnkhw_toe_interrupt(void);
static void tnkhw_toe_flush_completed(void);
static void tnkhw_dump_tx_descriptors(unsigned cindex);
static void tnk_switch_enable(void)
{
	/* init all switch on TOE,
	 * disable bit 16 for toe version 220.
	 */
	writel(0x3effd2, tnkhw_ioaddr + TNK_REG_TOE_SWITCH_CTRL);
	TNK_DBG("TOE switch reg enable\n");
}

static inline void tnkhw_ct_read_addr(uint32_t rd_addr, uint32_t *data)
{
	uint32_t rd_done;
	unsigned limit = 10000;

	/*  1. Write the CT read address */
	writel(rd_addr, tnkhw_ioaddr + TNK_REG_CT_CPU_RDADDR);

	/*  2. Write for the read to complete */
	do {
		rd_done = readl(tnkhw_ioaddr + TNK_REG_CT_CPU_ACSTAT);
		rd_done =
		    (rd_done >> TNK_OFFSET_CT_CPU_ACSTAT_RDDONE) &
		    TNK_MASK_CT_CPU_ACSTAT_RDDONE;
	} while (!rd_done && limit--);

	/*  3. Return the read result */
	*data = readl(tnkhw_ioaddr + TNK_REG_CT_CPU_RDDATA);
}

void tnkhw_ct_read(unsigned cindex, uint8_t loc, uint32_t *data)
{
	uint32_t rd_addr;

	rd_addr =
	    (cindex & TNK_MASK_CT_CPU_RDADDR_CSEL) <<
	    TNK_OFFSET_CT_CPU_RDADDR_CSEL;
	rd_addr |=
	    (loc & TNK_MASK_CT_CPU_RDADDR_LOC) << TNK_OFFSET_CT_CPU_RDADDR_LOC;

	tnkhw_ct_read_addr(rd_addr, data);
}

static inline void tnkhw_ct_write_addr(uint32_t wr_addr, uint32_t data)
{
	uint32_t wr_done;
	unsigned limit = 10000;

	/*  1. Set the data to be written */
	writel(data, tnkhw_ioaddr + TNK_REG_CT_CPU_WRDATA);

	/*  2. Set the CT write address */
	writel(wr_addr, tnkhw_ioaddr + TNK_REG_CT_CPU_WRADDR);

	/*  3. Wait for the write to complete */
	do {
		wr_done = readl(tnkhw_ioaddr + TNK_REG_CT_CPU_ACSTAT);
		wr_done =
		    (wr_done >> TNK_OFFSET_CT_CPU_ACSTAT_WRDONE) &
		    TNK_MASK_CT_CPU_ACSTAT_WRDONE;
	} while (!wr_done && limit--);
}

#define TNKHW_CT_LOC_RTT_CALC_EN 20
void tnkhw_ct_write(unsigned cindex, uint8_t loc, uint32_t data)
{
	uint32_t wr_addr;

	wr_addr =
	    (cindex & TNK_MASK_CT_CPU_WRADDR_CSEL) <<
	    TNK_OFFSET_CT_CPU_WRADDR_CSEL;
	wr_addr |=
	    (loc & TNK_MASK_CT_CPU_WRADDR_LOC) << TNK_OFFSET_CT_CPU_WRADDR_LOC;

	tnkhw_ct_write_addr(wr_addr, data);

	/* some ct items need to be read back to make sure write ok,
	 * such as cTEnRTTCalc.
	 */
	if (loc == TNKHW_CT_LOC_RTT_CALC_EN) {
		uint32_t rd_back_data, wr_mask;
		uint32_t wr_mask_bit = 0;
		uint32_t wr_mask_val = 0xFF;
		int rd_back_limit = 1000;
		wr_mask = readl(tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
		while (wr_mask) {
			if (wr_mask & 0x1)
				wr_mask_bit |= wr_mask_val;
			wr_mask >>= 1;
			wr_mask_val <<= 8;
		}
		tnkhw_ct_read_addr(wr_addr, &rd_back_data);
		while (((rd_back_data & wr_mask_bit) != (data & wr_mask_bit)) &&
			rd_back_limit--) {
			tnkhw_ct_write_addr(wr_addr, data);
			tnkhw_ct_read_addr(wr_addr, &rd_back_data);
		}
		if (unlikely(rd_back_limit < 0))
			pr_err("%s: write fail! cid=%u, loc=%u, data=%u\n",
				__func__, cindex, loc, data);
	}
}

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
static inline void ct_lookup_data_load_ipv4(uint32_t remote_ipv4_addr,
					    uint16_t remote_port,
					    uint16_t local_port,
						int local_ip_pos)
#else
static inline void ct_lookup_data_load_ipv4(uint32_t remote_ipv4_addr,
					    uint16_t remote_port,
					    uint16_t local_port)
#endif
{
	uint32_t ports;

	/*  1. Load the remote IP address */
	writel(remote_ipv4_addr, tnkhw_ioaddr + TNK_REG_CT_REMOTE_IPADDR);

	/*  2. Load the remote and local ports */
	ports =
	    (remote_port & TNK_MASK_CT_TCP_PORTS_REMOTE) <<
	    TNK_OFFSET_CT_TCP_PORTS_REMOTE;
	ports |=
	    (local_port & TNK_MASK_CT_TCP_PORTS_LOCAL) <<
	    TNK_OFFSET_CT_TCP_PORTS_LOCAL;
	writel(ports, tnkhw_ioaddr + TNK_REG_CT_TCP_PORTS);
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
	/* 3. Load the Local IP address position */
	if (local_ip_pos >= 0) {
		writel(local_ip_pos & 0x00000003,
				tnkhw_ioaddr + TNK_REG_CT_LOCAL_IPADDR_SEL);
		TNKBD_DBG("%s local_ip_pos = %d\n", __func__,
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR_SEL));
	}
#endif
}

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
static inline void ct_lookup_data_load_ipv6(uint32_t remote_ipv6_addr_31_0,
					    uint32_t remote_ipv6_addr_63_32,
					    uint32_t remote_ipv6_addr_95_64,
					    uint32_t remote_ipv6_addr_127_96,
					    uint16_t remote_port,
					    uint16_t local_port,
						int local_ip_pos)
#else
static inline void ct_lookup_data_load_ipv6(uint32_t remote_ipv6_addr_31_0,
					    uint32_t remote_ipv6_addr_63_32,
					    uint32_t remote_ipv6_addr_95_64,
					    uint32_t remote_ipv6_addr_127_96,
					    uint16_t remote_port,
					    uint16_t local_port)

#endif
{
	uint32_t ports;

	/*  1. Load the remote IP address */
	writel(remote_ipv6_addr_127_96,
	       tnkhw_ioaddr + TNK_REG_CT_REMOTE_IPV6_ADDR_W0);
	writel(remote_ipv6_addr_95_64,
	       tnkhw_ioaddr + TNK_REG_CT_REMOTE_IPV6_ADDR_W1);
	writel(remote_ipv6_addr_63_32,
	       tnkhw_ioaddr + TNK_REG_CT_REMOTE_IPV6_ADDR_W2);
	writel(remote_ipv6_addr_31_0,
	       tnkhw_ioaddr + TNK_REG_CT_REMOTE_IPV6_ADDR_W3);

	/*  2. Load the remote and local ports */
	ports =
	    (remote_port & TNK_MASK_CT_TCP_PORTS_REMOTE) <<
	    TNK_OFFSET_CT_TCP_PORTS_REMOTE;
	ports |=
	    (local_port & TNK_MASK_CT_TCP_PORTS_LOCAL) <<
	    TNK_OFFSET_CT_TCP_PORTS_LOCAL;
	writel(ports, tnkhw_ioaddr + TNK_REG_CT_TCP_PORTS);

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
       /* 3. Load the Local IP address position */
	if (local_ip_pos >= 0) {
		writel(local_ip_pos & 0x00000003,
				tnkhw_ioaddr + TNK_REG_CT_LOCAL_IPADDR_SEL);
		TNKBD_DBG("%s local_ip_pos = %d\n", __func__,
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR_SEL));
	}
#endif
}

static inline void tnkhw_ct_update(unsigned cindex, int add, int ipv6)
{
	uint32_t ctrl_val, update_pending, err;
	unsigned limit, retry = 0;

begin:
	ctrl_val =
	    (cindex & TNK_MASK_CT_CONN_CTRL_CSEL) <<
	    TNK_OFFSET_CT_CONN_CTRL_CSEL;
	ctrl_val |=
	    (add & TNK_MASK_CT_CONN_CTRL_ADD) << TNK_OFFSET_CT_CONN_CTRL_ADD;
	ctrl_val |=
	    (ipv6 & TNK_MASK_CT_CONN_CTRL_IPV6) << TNK_OFFSET_CT_CONN_CTRL_IPV6;
	ctrl_val |=
	    (0x1 & TNK_MASK_CT_CONN_CTRL_UPDATE) <<
	    TNK_OFFSET_CT_CONN_CTRL_UPDATE;

	/*  1. Write the connection control command */
	writel(ctrl_val, tnkhw_ioaddr + TNK_REG_CT_CONN_CTRL);

	/*  2. Wait for the connection table update to complete */
	limit = 100000;
	do {
		ctrl_val = readl(tnkhw_ioaddr + TNK_REG_CT_CONN_CTRL);
		update_pending =
		    (ctrl_val >> TNK_OFFSET_CT_CONN_CTRL_UPDATE) &
		    TNK_MASK_CT_CONN_CTRL_UPDATE;
	} while (update_pending && limit--);

	WARN(update_pending, "%s: pending: cindex = %u\n", __func__, cindex);

	err =
	    (ctrl_val >> TNK_OFFSET_CT_CONN_CTRL_ERR) &
	    TNK_MASK_CT_CONN_CTRL_ERR;
	if (err) {
		/*  TODO - tidy up the following error handling */
		pr_err("%s: Got error, cindex = %u, add = %d, retry = %u\n",
		       __func__, cindex, add, retry);
		if (!retry) {
			retry = 1;	/*  Retry once */
			goto begin;
		}
	}
}

static inline void tnkhw_ttx_db_write(unsigned cindex, unsigned entry_size,
				      unsigned entry, uint32_t val)
{
	unsigned db_offset = (cindex * entry_size) + (entry * sizeof(uint32_t));

	writel(val, tnkhw_ioaddr + TNK_DMA_TTX_DB_OFFSET + db_offset);
}

static inline void tnkhw_ttx_db_init(unsigned cindex, uint32_t desc_hd_ptr)
{
	/* Set tx_ch_desc_hd_ptr */
	tnkhw_ttx_db_write(cindex, TNK_TTX_DMA_DB_ENTRY_SIZE, 0, desc_hd_ptr);
	/* Clear tx_offset */
	tnkhw_ttx_db_write(cindex, TNK_TTX_DMA_DB_ENTRY_SIZE, 1, 0);
	/* Set tx_ch_desc_ack_ptr */
	tnkhw_ttx_db_write(cindex, TNK_TTX_DMA_DB_ENTRY_SIZE, 2, desc_hd_ptr);
	/* Clear tx_byte_count */
	tnkhw_ttx_db_write(cindex, TNK_TTX_DMA_DB_ENTRY_SIZE, 3, 0);
}

#if SWITCH_RECV_LRO
static inline void tnkhw_rx_db_write(unsigned cindex,
				      unsigned entry, uint32_t val)
{
	unsigned db_offset = (cindex * TNK_RX_DMA_DB_ENTRY_SIZE) +
			(entry * sizeof(uint32_t));

	writel(val, tnkhw_ioaddr + TNK_DMA_RX_DB_OFFSET + db_offset);
}

static inline unsigned int tnkhw_rx_db_read(unsigned cindex,
				      unsigned entry)
{
	unsigned db_offset = (cindex * TNK_RX_DMA_DB_ENTRY_SIZE) +
			(entry * sizeof(uint32_t));
	unsigned int val;

	val = readl(tnkhw_ioaddr + TNK_DMA_RX_DB_OFFSET + db_offset);
	return val;
}

static void tnkhw_check_rx_timer_valid(unsigned int cindex)
{
	unsigned int wr_cmd, rd_req_stat, rd_val;
	int limit = 10000;

	wr_cmd = (cindex & TNK_MASK_CPU_ACS_CID) |
			(1 << TNK_OFFSET_CPU_ACS_WR_MASK) |
			(0 << TNK_OFFSET_CPU_ACS_WR_SEL);
	writel(wr_cmd, tnkhw_ioaddr + TNK_REG_DMA_TIMER_CPU_CMD);

	do {
		rd_req_stat = readl(tnkhw_ioaddr + TNK_REG_DMA_TIMER_CPU_CMD);
		rd_req_stat =
		    (rd_req_stat >> TNK_OFFSET_CPU_ACS_REQ_STAT) &
		    TNK_MASK_CPU_ACS_REQ_STAT;
	} while (!rd_req_stat && limit--);

	if (unlikely(limit == -1 && !rd_req_stat))
		pr_warn("%s: rx timer cmd stat is always 0!, wr_cmd=0x%x\n",
			__func__, wr_cmd);

	rd_val = readl(tnkhw_ioaddr + TNK_REG_DMA_TIMER_CPU_RDATA);
	if (unlikely(rd_val >> 31)) {
		pr_warn("%s: rx timer valid!, rd_val=0x%x, wr_cmd=0x%x\n",
			__func__, rd_val, wr_cmd);
		mdelay(10);
	}
}

/* This tnkhw_rx_timer_reset function is useless,
 * because write TNK_REG_DMA_TIMER_CPU_CMD has wrong result
 */
#if 0
static void tnkhw_rx_timer_reset(unsigned int cindex)
{
	unsigned int wr_cmd, rd_req_stat;
	int limit = 10000;

	writel(0, tnkhw_ioaddr + TNK_REG_DMA_TIMER_CPU_WDATA);

	wr_cmd = (cindex & TNK_MASK_CPU_ACS_CID) |
			(1 << TNK_OFFSET_CPU_ACS_WR_MASK) |
			(1 << TNK_OFFSET_CPU_ACS_WR_SEL);
	writel(wr_cmd, tnkhw_ioaddr + TNK_REG_DMA_TIMER_CPU_CMD);

	do {
		rd_req_stat = readl(tnkhw_ioaddr + TNK_REG_DMA_TIMER_CPU_CMD);
		rd_req_stat =
		    (rd_req_stat >> TNK_OFFSET_CPU_ACS_REQ_STAT) &
		    TNK_MASK_CPU_ACS_REQ_STAT;
	} while (!rd_req_stat && limit--);

	if (unlikely(limit == -1 && !rd_req_stat))
		pr_warn("%s: rx timer cmd stat is always 0!\n", __func__);
}
#endif

static inline void tnkhw_set_rx_lro_timer(void)
{
	/* base clk: 1 us */
	writel(23, tnkhw_ioaddr + TNK_REG_DMA_TIMER_BASE_CLK);
	/* timeout value: 1ms */
	writel(0x19BB, tnkhw_ioaddr + TNK_REG_DMA_TIMER_WATCHDOG);
}

static unsigned int tnkhw_get_rcv_nxt(unsigned int cindex)
{
	unsigned int rcv_nxt, rd_req_stat;
	int limit = 10000;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(cindex & TNK_MASK_RXF_DB_RD_CID,
		tnkhw_ioaddr + TNK_REG_DMA_RXF_DB_RD_ACT);

	do {
		rd_req_stat = readl(tnkhw_ioaddr + TNK_REG_DMA_RXF_DB_RD_ACT);
		rd_req_stat =
		    (rd_req_stat >> TNK_OFFSET_RXF_DB_RD_STAT) &
		    TNK_MASK_RXF_DB_RD_STAT;
	} while (!rd_req_stat && limit--);

	if (unlikely(limit == -1 && !rd_req_stat))
		pr_warn("%s: rxf db rd stat is always 0!\n", __func__);

	rcv_nxt = readl(tnkhw_ioaddr + TNK_REG_DMA_RXF_EXP_SEQNUM);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return rcv_nxt;
}

#define TX_WND_ADD	0
#define TX_WND_SUB	1
/* Increase or decrease hardware receive window,
 * when opt_type is TX_WND_ADD, it means increase receive window,
 * when opt_type is TX_WND_SUB, it means decrease receive window.
 */
static void tnkhw_add_tx_wnd(unsigned int cindex, unsigned int wnd,
				unsigned int opt_type)
{
	unsigned int wr_cmd;

	wr_cmd = (cindex & TNK_MASK_RXF_TXWND_WR_CID) |
			((wnd & TNK_MASK_RXF_TXWND_WR_WND) <<
			TNK_OFFSET_RXF_TXWND_WR_WND) |
			(opt_type << TNK_OFFSET_RXF_TXWND_OPT_TYPE);

	writel(wr_cmd, tnkhw_ioaddr + TNK_REG_DMA_RXF_TXWND_WR);
}

static void tnkhw_rxfetch_poll(void)
{
	unsigned int wr_cmd;

	wr_cmd = (1 & TNK_MASK_RXFETCH_POLL) <<
		TNK_OFFSET_RXFETCH_POLL;

	writel(wr_cmd, tnkhw_ioaddr + TNK_REG_DMA_TTX_RXFETCH_POLL);
}
#endif

static inline void tnkhw_ttx_txstat_clear(unsigned cindex)
{
	unsigned offset = cindex * TNK_TTX_DMA_DB_TXSTAT_SIZE;
	memset(tnkhw_ioaddr + TNK_DMA_TTX_DB_TXSTAT_OFFSET + offset,
	       0, TNK_TTX_DMA_DB_TXSTAT_SIZE);
}

static inline void tnkhw_ttx_rxstat_clear(unsigned cindex)
{
	unsigned offset = cindex * TNK_TTX_DMA_DB_RXSTAT_SIZE;
	memset(tnkhw_ioaddr + TNK_DMA_TTX_DB_RXSTAT_OFFSET + offset,
	       0, TNK_TTX_DMA_DB_RXSTAT_SIZE);
}

static inline void tnkhw_ttx_txstat_get(unsigned cindex,
					uint32_t *tx_acked_count,
					uint32_t *tx_retry_count)
{
	unsigned offset = cindex * TNK_TTX_DMA_DB_TXSTAT_SIZE;
	*tx_acked_count =
	    readl(tnkhw_ioaddr + TNK_DMA_TTX_DB_TXSTAT_OFFSET + offset);
	*tx_retry_count =
	    readl(tnkhw_ioaddr + TNK_DMA_TTX_DB_TXSTAT_OFFSET + offset +
		  sizeof(uint32_t));
}

static inline void tnkhw_ttx_rxstat_get(unsigned cindex,
					uint32_t *rxed_byte_count)
{
	unsigned offset = cindex * TNK_TTX_DMA_DB_RXSTAT_SIZE;
	*rxed_byte_count =
	    readl(tnkhw_ioaddr + TNK_DMA_TTX_DB_RXSTAT_OFFSET + offset);
}

static inline void tnkhw_ttx_data_add(unsigned cindex, uint32_t size)
{
	/*  If size < 2^21, use TTX_ALT_REQ */
	if (likely((size & TNK_MASK_DMA_TTX_ALT_REQ_SIZE) == size)) {
		uint32_t regval = (size << TNK_OFFSET_DMA_TTX_ALT_REQ_SIZE) |
		    (cindex << TNK_OFFSET_DMA_TTX_ADD_CID);
		writel(regval, tnkhw_ioaddr + TNK_REG_DMA_TTX_ALT_REQ);
	}
	/*  Otherwise, use TTX_ADD_SIZE and TTX_ADD registers */
	else {
		size =
		    (size & TNK_MASK_DMA_TTX_ADD_SIZE) <<
		    TNK_OFFSET_DMA_TTX_ADD_SIZE;
		TNK_DBG("%s size=%d to %p\n", __func__, size,
			tnkhw_ioaddr + TNK_REG_DMA_TTX_ADD_SIZE);
		writel(size, tnkhw_ioaddr + TNK_REG_DMA_TTX_ADD_SIZE);

		cindex =
		    (cindex & TNK_MASK_DMA_TTX_ADD_CID) <<
		    TNK_OFFSET_DMA_TTX_ADD_CID;
		TNK_DBG("%s cindex=%d to %p\n", __func__, cindex,
			tnkhw_ioaddr + TNK_REG_DMA_TTX_ADD);
		writel(cindex, tnkhw_ioaddr + TNK_REG_DMA_TTX_ADD);
	}
}

#if !SWITCH_REMOVE_TTX_ERR
static inline void tnkhw_ttx_wait_flush(void)
{
	unsigned int debug_reg;
	unsigned char rd_ptr0, wr_ptr0;
	unsigned char rd_ptr1, wr_ptr1;
	unsigned int initial_state = 0;
	unsigned int  i = 0;

	debug_reg = readl(tnkhw_ioaddr + 0x84f0);
	rd_ptr0 = (debug_reg >> 16) & 0xff;
	wr_ptr0 = (debug_reg >> 7) & 0xff;
	wr_ptr0 = wr_ptr0 + 7;

	if (rd_ptr0 < wr_ptr0)
		initial_state = 0;
	else
		initial_state = 1;

#if 0
	while (i < 256) {
		mdelay(5);
		data_pool[i++] = readl(tnkhw_ioaddr + 0x84f0);
	}
#else
	i = 0;
	do {
		debug_reg = 0;
		debug_reg = readl(tnkhw_ioaddr + 0x84f0);
		rd_ptr1 = (debug_reg >> 16) & 0x1ff;
		wr_ptr1 = (debug_reg >> 7) & 0x1ff;

		if (!initial_state && rd_ptr1 > wr_ptr0)
			break;
		if (!initial_state && rd_ptr1 < rd_ptr0)
			break;
		if (initial_state && rd_ptr1 > wr_ptr0 && rd_ptr1 < rd_ptr0)
			break;
/*
		if (initial_state && rd_ptr1 > wr_ptr0 && rd_ptr1 < wr_ptr1)
			break;
*/

		if (rd_ptr1 == wr_ptr1) {
			mdelay(1);
			break;
		}
	} while (i++ < 100000);

	if (i >= 100000)
		pr_err("%s timeout rd0:%d wr0:%d rd1:%d wr1:%d\n",
				__func__, rd_ptr0, wr_ptr0, rd_ptr1, wr_ptr1);
#endif

#if 0
	i = 0;
	while (i < 256) {
		pr_info("i:%d %x \t", i, data_pool[i]);
		i++;
	}
#endif
}
#endif

/* Flushes any data currently in the TX pipeline for a specified channel
 * Useful for resetting the TX pipeline on startup, or in case of error
 */
static inline void tnkhw_ttx_data_flush(unsigned cindex)
{
	uint32_t regval;
	int limit;
	uint32_t toe_int_en, toe_int_stat;

	TNK_DBG("%s: cindex %u\n", cindex);

#if 0
	tnkhw_ttx_data_add(cindex, 64);
#endif

	tnk_flush_pending = 1;

	toe_int_en = readl(tnkhw_ioaddr + TNK_REG_TOE_INTR_EN);
	/*  enable the flush complete interrupt */
	toe_int_en |=
	    (TNK_MASK_TOE_INTR_EN_FLUSH << TNK_OFFSET_TOE_INTR_EN_FLUSH);
	writel(toe_int_en, tnkhw_ioaddr + TNK_REG_TOE_INTR_EN);

	/*  Request the flush */
	regval =
	    (cindex & TNK_MASK_DMA_TTX_ADD_CID) << TNK_OFFSET_DMA_TTX_ADD_CID;
	regval |= (1 << TNK_OFFSET_DMA_TTX_ADD_FLUSH);
	writel(regval, tnkhw_ioaddr + TNK_REG_DMA_TTX_ADD);

	/* NOTE - this code assumes that interrupts are disabled
	 * Because the TOE_INTR_STAT register is cleared-on-read,
	 * an interrupt handler could clear this status bit before
	 * we see it
	 */

	limit = 100000;
	do {
		toe_int_stat = readl(tnkhw_ioaddr + TNK_REG_INTR_STAT);
		toe_int_stat =
		    (toe_int_stat >> TNK_OFFSET_INTR_STAT_TOE) &
		    TNK_MASK_INTR_STAT_TOE;
	} while (!toe_int_stat && limit--);

	if (toe_int_stat) {
		limit = 10000;
		do {
			/* tnkhw_toe_interrupt(); */
			/* Here we only care about flush_pending_complete
			* interrupt, so we will omit ct_max_retry interrupt.
			* We trust logic will generate ct_max_retry interrupt
			* again and again.
			* Why we omit ct_max_retry interrupt, because
			* in tnk_ct_tx_max_retries_callback(),
			* tnkhw_connection_disable() need get tnkhw_reg_lock,
			* but here we has got tnkhw_reg_lock,
			* it results dead spin_lock. */
			tnkhw_toe_flush_completed();
			mb();
		} while (tnk_flush_pending && limit--);
	}

	/* This flush mechanism isn't always reliable. A suitable fallback
	 * is to wait instead for 30us to allow for any outstanding TTX DMA
	 * engine transfers to complete (see TNK data sheet)
	 * Note that flushing the TTX channel should be a rare event
	 */
	if (tnk_flush_pending) {
		pr_warn("%s: Flush completion pending on cid %d,",
				__func__, cindex);
		pr_warn(" adding 30us delay\n");
		udelay(30);
		tnkhw_toe_flush_completed();
		pr_warn("cindex:%d after delay 30us tnk_flush_pending is %d\n",
				cindex, tnk_flush_pending);
	}

	/*  disable the flush complete interrupt */
	toe_int_en &=
	    ~(TNK_MASK_TOE_INTR_EN_FLUSH << TNK_OFFSET_TOE_INTR_EN_FLUSH);
	writel(toe_int_en, tnkhw_ioaddr + TNK_REG_TOE_INTR_EN);
}

/* Returns 1 if overflow occurred, otherwise returns 0 and ack_cid contains
 * next CID with a free TX descriptor
 */
#if SWITCH_MULTI_INTR
static inline int tnkhw_ttx_ackd_cid_read(unsigned *ack_cid,
		unsigned int channel)
#else
static inline int tnkhw_ttx_ackd_cid_read(unsigned *ack_cid)
#endif
{
#if SWITCH_MULTI_INTR
	uint32_t reg_offset =
		((channel == DMA_CHANNEL_TOE0) ?
		TNK_REG_DMA_TTX_ACKD_CID :
		(TNK_REG_DMA_TTX_ACKD1_CID + (channel - 1) * 0x4));
	uint32_t ackd_val = readl(tnkhw_ioaddr + reg_offset);
#else
	uint32_t ackd_val = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_ACKD_CID);
#endif
	int overflow =
	    (ackd_val >> TNK_OFFSET_DMA_TTX_ACKD_OVFLOW) &
	    TNK_MASK_DMA_TTX_ACKD_OVFLOW;
	int valid =
	    (ackd_val >> TNK_OFFSET_DMA_TTX_ACKD_VALID) &
	    TNK_MASK_DMA_TTX_ACKD_VALID;

	if (valid)
		*ack_cid =
		    (ackd_val >> TNK_OFFSET_DMA_TTX_ACKD_CID) &
		    TNK_MASK_DMA_TTX_ACKD_CID;
	else
		*ack_cid = 0;

	if (overflow)
		return 1;

	return 0;
}

#if SWITCH_RECV_LRO
static inline int tnkhw_rx_lro_cid_read(unsigned *rx_cid,
		unsigned int channel)
{
	uint32_t reg_offset =
		TNK_REG_DMA_RXINTR_CORE0 + channel * 0x4;
	uint32_t rx_cid_val = readl(tnkhw_ioaddr + reg_offset);
	int overflow =
	    (rx_cid_val >> TNK_OFFSET_DMA_RX_INTR_OVFLOW) &
	    TNK_MASK_DMA_RX_INTR_OVFLOW;
	int valid =
	    (rx_cid_val >> TNK_OFFSET_DMA_RX_INTR_VALID) &
	    TNK_MASK_DMA_RX_INTR_VALID;

	if (valid)
		*rx_cid =
		    (rx_cid_val >> TNK_OFFSET_DMA_RX_INTR_CID) &
		    TNK_MASK_DMA_RX_INTR_CID;
	else
		*rx_cid = 0;

	if (overflow)
		return 1;

	return 0;
}

static inline unsigned int tnkhw_rx_bitmap_read(unsigned entry)
{
	unsigned int bitmap_val;

	writel(entry, tnkhw_ioaddr + TNK_REG_DMA_BITMAP_RDCMD);
	bitmap_val = readl(tnkhw_ioaddr + TNK_REG_DMA_BITMAP_RDATA);

	return bitmap_val;
}
#endif

static inline void tnkhw_ttx_ctrl_set(unsigned axi_burst_len)
{
	uint32_t ctrl_val =
	    (axi_burst_len & TNK_MASK_TTX_CTRL0_MAXBURSTLEN) <<
	    TNK_OFFSET_TTX_CTRL0_MAXBURSTLEN;
	writel(ctrl_val, tnkhw_ioaddr + TNK_REG_DMA_TTX_CTRL0);
}

static inline void tnkhw_trx_ctrl_set(unsigned rx_buffer_len)
{
	/*  TODO - consider tying this to value of DMA_BUFFER_LEN.
	 *  For now, we're assuming 2kBytes (0x3) */
	uint32_t ctrl_val =
	    (3 & TNK_MASK_TRX_CTRL_RXBUFFERSIZE) <<
	    TNK_OFFSET_TRX_CTRL_RXBUFFERSIZE;
	/* Extra TRX status options are intentionally left as disabled
	 * However, if forwarding of error'd frames was ever enabled in the
	 * DMA engine, then we would have to enable some bits here also
	 */
	ctrl_val |= (1 & TNK_MASK_TRX_CTRL_IPC_STAT_EN) <<
		TNK_OFFSET_TRX_CTRL_IPC_STAT_EN;
	writel(ctrl_val, tnkhw_ioaddr + TNK_REG_DMA_TRX_CTRL);
}

static inline void tnkhw_trx_desc_add(uint32_t count)
{
	TNK_DBG("%s write count of %d\n", __func__, count);
	count = (count & TNK_MASK_TRX_ADD_COUNT) << TNK_OFFSET_TRX_ADD_COUNT;
	writel(count, tnkhw_ioaddr + TNK_REG_DMA_TRX_ADD_SIZE);
}

#define TNKHW_CT_LOC_FLAGS 12
static void tnkhw_ct_enable(unsigned cindex, int enable)
{
	uint32_t flags;
	/*  We only want to read/write the lower 16 bits of the CT entry */
	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_FLAGS, &flags);
	if (enable)
		flags |= 0x10;	/*  Set the enable bit */
	else
		flags &= ~0x10;	/*  Clear the enable bit */
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_FLAGS, flags & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
}

#define TNKHW_CT_LOC_TX_MSS 11
void tnkhw_set_tx_mss(unsigned int cindex, unsigned int mss_now)
{
	unsigned int tx_mss;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0xC, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_TX_MSS, &tx_mss);
	tx_mss &= 0xFFFF;
	tx_mss |= mss_now << 16;
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_TX_MSS, tx_mss);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

#if SWITCH_NAGLE || SWITCH_CORK
void tnkhw_set_nagle_cork(unsigned int cindex,
		unsigned int cork, unsigned int nagle_off)
{
	unsigned int cpu_flag;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_FLAGS, &cpu_flag);
	cpu_flag = (cpu_flag & ~(1 << TNK_OFFSET_CORK)) |
		((cork & 0x1) << TNK_OFFSET_CORK);
	cpu_flag = (cpu_flag & ~(1 << TNK_OFFSET_NONAGLE)) |
		((nagle_off & 0x1) << TNK_OFFSET_NONAGLE);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_FLAGS, cpu_flag & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

#if SWITCH_THIN_STREAM
void tnkhw_set_thin_linear_timeouts(unsigned int cindex, unsigned int thin_lto)
{
	unsigned int cpu_flag;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_FLAGS, &cpu_flag);
	cpu_flag = (cpu_flag & ~(1 << TNK_OFFSET_THIN_STREAM)) |
		((thin_lto & 0x1) << TNK_OFFSET_THIN_STREAM);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_FLAGS, cpu_flag & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

#if SWITCH_KEEPALIVE
void tnkhw_set_keepalive_flag(unsigned int cindex, unsigned int keepalive)
{
	unsigned int cpu_flag;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_FLAGS, &cpu_flag);
	cpu_flag = (cpu_flag & ~(1 << TNK_OFFSET_KEEPALIVE)) |
		((keepalive & 0x1) << TNK_OFFSET_KEEPALIVE);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_FLAGS, cpu_flag & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

#define TNKHW_CT_LOC_ALIVE_DONE	9
void tnkhw_write_alive_done_flag(unsigned int cindex, unsigned int val)
{
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0x2, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_ALIVE_DONE, (val << 8) & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

#if SWITCH_SEND_FIN
void tnkhw_set_fin_flag(unsigned int cindex, unsigned int fin)
{
	unsigned int cpu_flag;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_FLAGS, &cpu_flag);
	cpu_flag = (cpu_flag & ~(1 << TNK_OFFSET_FIN)) |
		((fin & 0x1) << TNK_OFFSET_FIN);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_FLAGS, cpu_flag & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

int tnkhw_get_fin_send_state(unsigned int cindex)
{
	unsigned int wr_val, rd_val;
	unsigned long flags;
	int fin_sent = 0;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	wr_val = (0 << TNK_OFFSET_TOE_FIN_OPT) |
		((cindex & TNK_MASK_TOE_FIN_CID) << TNK_OFFSET_TOE_FIN_CID);
	writel(wr_val, tnkhw_ioaddr + TNK_REG_CT_FIN_SEND_STATE);

	rd_val = readl(tnkhw_ioaddr + TNK_REG_CT_FIN_SEND_STATE);
	fin_sent = (rd_val >> TNK_OFFSET_TOE_FIN_STATE) &
		TNK_MASK_TOE_FIN_STATE;

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return fin_sent;
}

void tnkhw_set_fin_send_state(unsigned int cindex, int val)
{
	unsigned int wr_val;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	wr_val = (1 << TNK_OFFSET_TOE_FIN_OPT) |
		((cindex & TNK_MASK_TOE_FIN_CID) << TNK_OFFSET_TOE_FIN_CID) |
		(val & TNK_MASK_TOE_FIN_WDATA);
	writel(wr_val, tnkhw_ioaddr + TNK_REG_CT_FIN_SEND_STATE);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

#if SWITCH_ZERO_PROBE
void tnkhw_set_zero_probe_cnt(unsigned int cindex, unsigned int val)
{
	unsigned int wr_val;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	wr_val = (1 << TNK_OFFSET_TOE_ZERO_PROBE_OPT) |
		((cindex & TNK_MASK_TOE_ZERO_PROBE_CID) <<
			TNK_OFFSET_TOE_ZERO_PROBE_CID) |
		(val & TNK_MASK_TOE_ZERO_PROBE_CNT);
	writel(wr_val, tnkhw_ioaddr + TNK_REG_CT_ZERO_PROBE_CNT);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

unsigned int tnkhw_get_zero_probe_cnt(unsigned int cindex)
{
	unsigned int wr_val;
	unsigned long flags;
	unsigned int probe_cnt;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	wr_val = (0 << TNK_OFFSET_TOE_ZERO_PROBE_OPT) |
		((cindex & TNK_MASK_TOE_ZERO_PROBE_CID) <<
			TNK_OFFSET_TOE_ZERO_PROBE_CID);
	writel(wr_val, tnkhw_ioaddr + TNK_REG_CT_ZERO_PROBE_CNT);

	probe_cnt = readl(tnkhw_ioaddr + TNK_REG_CT_ZERO_PROBE_CNT);
	probe_cnt = (probe_cnt >> TNK_OFFSET_TOE_ZERO_PROBE_CNT) &
			TNK_MASK_TOE_ZERO_PROBE_CNT;

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return probe_cnt;
}
#endif

#if SWITCH_ZERO_PROBE
#define TNKHW_CT_LOC_PROBE	11
#define TNKHW_CT_LOC_RETRY	18
int tnkhw_check_max_retries(unsigned int cindex)
{
	unsigned int retrans;
	unsigned int probes;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	tnkhw_ct_read(cindex, TNKHW_CT_LOC_RETRY, &retrans);
	retrans = (retrans >> TNK_OFFSET_RETRY_INTR) & TNK_MASK_RETRY_INTR;

	tnkhw_ct_read(cindex, TNKHW_CT_LOC_PROBE, &probes);
	probes = (probes >> TNK_OFFSET_PROBE_INTR) & TNK_MASK_PROBE_INTR;

	ret = retrans || probes;

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
	return ret;
}
#endif

void tnkhw_dma_interrupt_disable(void)
{
	uint32_t enabled;

	enabled = readl(tnkhw_ioaddr + TNK_REG_INTR_EN);
	enabled &= ~(TNK_INTR_TRX | TNK_INTR_TTX_ACK);
#if SWITCH_RECV_LRO
	enabled &= ~TNK_INTR_RX_LRO0;
#endif
	writel(enabled, tnkhw_ioaddr + TNK_REG_INTR_EN);
}

void tnkhw_dma_interrupt_enable(void)
{
	uint32_t enabled = readl(tnkhw_ioaddr + TNK_REG_INTR_EN);
	enabled |= (TNK_INTR_TRX | TNK_INTR_TTX_ACK);
#if SWITCH_RECV_LRO
	enabled |= TNK_INTR_RX_LRO0;
#endif
	writel(enabled, tnkhw_ioaddr + TNK_REG_INTR_EN);
}

#if SWITCH_MULTI_INTR
void tnkhw_dma_channel_interrupt_disable(unsigned int channel)
{
	uint32_t enabled;

	enabled = readl(tnkhw_ioaddr + TNK_REG_INTR1_EN + (channel - 1) * 0x8);
	enabled &= ~(TNK_INTR_RX_CHANNEL(channel) |
			TNK_INTR_TX_ACK_CHANNEL(channel));
#if SWITCH_RECV_LRO
	enabled &= ~TNK_INTR_RX_LRO(channel);
#endif
	writel(enabled, tnkhw_ioaddr + TNK_REG_INTR1_EN + (channel - 1) * 0x8);
}

void tnkhw_dma_channel_interrupt_enable(unsigned int channel)
{
	uint32_t enabled;

	enabled = readl(tnkhw_ioaddr + TNK_REG_INTR1_EN + (channel - 1) * 0x8);
	enabled |= (TNK_INTR_RX_CHANNEL(channel) |
			TNK_INTR_TX_ACK_CHANNEL(channel));
#if SWITCH_RECV_LRO
	enabled |= TNK_INTR_RX_LRO(channel);
#endif
	writel(enabled, tnkhw_ioaddr + TNK_REG_INTR1_EN + (channel - 1) * 0x8);
}
#endif

static int tnkhw_map_skb(struct sk_buff *skb, dma_addr_t *dma)
{
	struct tcp_skb_cb *tcp_cb = TCP_SKB_CB(skb);
	struct tnkcb *cb = &(tcp_cb->header.tcb);

	BUG_ON(cb->magic != TNK_MAGIC);

	if (cb->dma) {
		pr_err("%s ERROR skb is already DMA mapped!\n", __func__);
		return 0;
	}

#if POISON_BUF_VAL >= 0
	memset(skb->data, POISON_BUF_VAL, DMA_BUFFER_SIZE);
#endif

	*dma = dma_map_single(tnk_dev,
			      skb->data, DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	if (dma_mapping_error(tnk_dev, *dma)) {
		pr_err("%s: DMA Mapping Error\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static int tnkhw_alloc_and_map_skb(struct sk_buff **skb, dma_addr_t *dma)
{
	struct tnkcb *cb;
	int addr;

	*skb = tnk_alloc_skb(DMA_BUFFER_SIZE + TNK_TCP_HEADER_RESERVED,
			GFP_ATOMIC | __GFP_NOWARN | __GFP_HIGH);
	if (unlikely(*skb == NULL)) {
		pr_warn("%s: Rx init fails; skb is NULL\n", __func__);
		return -ENOMEM;
	}
	skb_reserve(*skb, TNK_TCP_HEADER_RESERVED);

	addr = (uint32_t) (*skb)->data;

	if ((addr % 32) != 0) {
		/*  Assuming 2k buffer size, we have room for realigning */
		TNK_DBG("%s: Unaligned buffer\n", __func__);
		skb_reserve(*skb, 8 - (addr % 32));
	}

	/* Initialise the tnk private area */
	cb = &(TCP_SKB_CB(*skb)->header.tcb);
	cb->magic = TNK_MAGIC;
	cb->type = TNK_TYPE_FREE;
	cb->dma = 0;

	if (tnkhw_map_skb(*skb, dma) != 0) {
		tnk_free_skb(*skb, SKB_FROM_RECV);
		return -ENOMEM;
	}

	cb->dma = *dma;

	return 0;
}

static int tnkhw_unmap_skb(struct sk_buff *skb)
{
	struct tcp_skb_cb *tcp_cb = TCP_SKB_CB(skb);
	struct tnkcb *cb = &(tcp_cb->header.tcb);

	BUG_ON(cb->magic != TNK_MAGIC);

	if (!cb->dma) {
		pr_err("%s ERROR skb is not DMA mapped!\n", __func__);
		return 0;
	}

	dma_unmap_single(tnk_dev, cb->dma, DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	cb->dma = 0;
	return 0;
}

static int tnkhw_unmap_and_free_skb(struct sk_buff *skb)
{
	tnkhw_unmap_skb(skb);
	tnk_free_skb(skb, SKB_FROM_RECV);

	return 0;
}

#define TNK_RXBYPASSMODE_BIT						\
	(TNK_MASK_CONFIG0_RXBYPASSMODE << TNK_OFFSET_CONFIG0_RXBYPASSMODE)
#define TNK_IPV6_EN_BIT							\
	(TNK_MASK_CONFIG0_IPV6_EN << TNK_OFFSET_CONFIG0_IPV6_EN)

static inline void tnkhw_toe_config(void)
{
	uint32_t reg_val = readl(tnkhw_ioaddr + TNK_REG_TOE_CONFIG0);

	/*  Disable TOE By-Pass mode (enabled by default) */
	reg_val &= ~TNK_RXBYPASSMODE_BIT;

#ifdef CONFIG_IPV6
	/*  Enable IPv6 (disabled by default), if kernel support enabled */
	reg_val |= TNK_IPV6_EN_BIT;
#endif

	writel(reg_val, tnkhw_ioaddr + TNK_REG_TOE_CONFIG0);
}

static inline void tnkhw_ipv6_enable(int enabled)
{
	uint32_t reg_val = readl(tnkhw_ioaddr + TNK_REG_TOE_CONFIG0);

	if (enabled)
		reg_val |= TNK_IPV6_EN_BIT;
	else
		reg_val &= ~TNK_IPV6_EN_BIT;

	writel(reg_val, tnkhw_ioaddr + TNK_REG_TOE_CONFIG0);
}

static int tnkhw_rx_dma_init(void)
{
	int i, err;
#if SWITCH_MULTI_INTR
	int j;
	struct tnkhw_rx_dma_info *r;
#else
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma;
#endif
	struct tnk_rx_dma_desc *p = NULL;

	TNK_DBG("%s\n", __func__);

#if SWITCH_MULTI_INTR
	for (j = 0; j < TOE_MULTI_INTR_NUM; j++) {
		r = &tnk_rx_dma[j];
#endif
	r->skbuff =
	    kzalloc(sizeof(struct sk_buff *) * tnk_rx_fifo, GFP_KERNEL);

	if (!r->skbuff) {
		pr_err("%s:ERROR allocating DMA Rx skbuff pointers\n",
		       __func__);
		return -ENOMEM;
	}

	r->skbuff_dma =
	    kzalloc(sizeof(dma_addr_t) * tnk_rx_fifo, GFP_KERNEL);
	if (!r->skbuff_dma) {
		pr_err("%s:ERROR allocating DMA Rx skbuff DMA pointers\n",
		       __func__);
		return -ENOMEM;
	}

	/* TODO - Ensure that descriptors are 128-bit (16-byte) aligned
	 * dma_alloc_coherent should normally return page-aligned memory,
	 * but it may not be guaranteed.  Low priority for now.
	 */
	r->desc_list =
	    (struct tnk_rx_dma_desc *)
		dma_alloc_coherent(tnk_dev,
				   tnk_rx_fifo *
				   sizeof(struct tnk_rx_dma_desc),
				   &r->desc_list_phys,
				   GFP_KERNEL);
	if (r->desc_list == NULL) {
		pr_err("%s:ERROR allocating DMA Rx descriptors\n", __func__);
		return -ENOMEM;
	}

	TNK_DBG("%s alloc desc_list %p\n", __func__, r->desc_list);
	/* Clear the descriptors initially */
	memset(r->desc_list, 0,
	       tnk_rx_fifo * sizeof(struct tnk_rx_dma_desc));

	/*  Pre-fill the RX Descriptor ring with empty buffers */
	for (i = 0; i < tnk_rx_fifo; i++) {
		p = r->desc_list + i;

		err = tnkhw_alloc_and_map_skb(&r->skbuff[i], &r->skbuff_dma[i]);
		if (err != 0)
			return err;

		/* Assign the skb data buffer phys address to the descriptor */
		p->base.des2 = r->skbuff_dma[i];
		p->base.des01.erx.buffer1_size = DMA_BUFFER_DESC_SIZE;

		/* Assign the descriptor ownership to hardware */
		p->base.des01.erx.own = 1;
		/* Set mutipacket int,the default value is 16 packet per int*/
		if (i % TNK_RX_NAPI_LIMIT)
			p->base.des01.erx.disable_ic = 1;
	}
	/* Mark the last descriptor as the end of the ring */
	p->base.des01.erx.end_ring = 1;

	/* Reset our head/tail indices */
	r->head = 0;
	r->tail = 0;
	spin_lock_init(&r->rx_desc_lock);

	/*  Pre-fill the RX recycle queue ring with additional empty buffers */
	skb_queue_head_init(&r->free_skbs);
#ifndef TNK_RX_CHANNEL_FLOW_CONTROL
	for (i = 0; i < TNK_RX_SW_Q_SIZE; i++) {
		struct sk_buff *skb;
		dma_addr_t dma;

		err = tnkhw_alloc_and_map_skb(&skb, &dma);
		if (err != 0)
			return err;

		skb_queue_head(&r->free_skbs, skb);
	}
#endif

	TNK_DBG("%s init dma ops\n", __func__);
	/* Configure the Rx DMA queue (alternate descriptor size enabled) */
#if SWITCH_MULTI_INTR
	dwmac1000_dma_ops.init(tnkhw_ioaddr, DMA_CHANNEL_TOERX + j,
			DMA_BURST_LEN,
			0, r->desc_list_phys, 1);
#else
	dwmac1000_dma_ops.init(tnkhw_ioaddr, DMA_CHANNEL_TOERX, DMA_BURST_LEN,
			       0, r->desc_list_phys, 1);
#endif
	STMMAC_SYNC_BARRIER();
	tnkhw_trx_desc_add(tnk_rx_fifo);

#if SWITCH_MULTI_INTR
	}
#endif
	TNK_DBG("%s done\n", __func__);
	return 0;
}

static void tnkhw_rx_dma_free(void)
{
	int i;
#if SWITCH_MULTI_INTR
	int j;
	struct tnkhw_rx_dma_info *r;
#else
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma;
#endif
	struct sk_buff *skb;

#if SWITCH_MULTI_INTR
	for (j = 0; j < TOE_MULTI_INTR_NUM; j++) {
		r = &tnk_rx_dma[j];
#endif
	while ((skb = skb_dequeue(&r->free_skbs))) {
		tnk_mem_free_pool_dequeued(skb);
		tnkhw_unmap_and_free_skb(skb);
	}

	if (r->skbuff) {
		for (i = 0; i < tnk_rx_fifo; i++) {
			if (r->skbuff[i]) {
				tnkhw_unmap_and_free_skb(r->skbuff[i]);
				r->skbuff[i] = NULL;
			}
		}
	}

	/* Free the region of consistent memory previously allocated for
	 * the DMA */
	if (r->desc_list) {
		dma_free_coherent(tnk_dev,
				tnk_rx_fifo * sizeof(struct tnk_rx_dma_desc),
				r->desc_list, r->desc_list_phys);
	}
	kfree(r->skbuff_dma);
	r->skbuff_dma = NULL;
	kfree(r->skbuff);
	r->skbuff = NULL;
#if SWITCH_MULTI_INTR
	}
#endif

}

#if SWITCH_RECV_LRO
int tnkhw_rx_alloc_page(struct sk_buff *skb, dma_addr_t *dma)
{
	struct tcp_skb_cb *tcp_cb = TCP_SKB_CB(skb);
	struct tnkcb *cb = &(tcp_cb->header.tcb);
	struct page *pg;

	BUG_ON(cb->magic != TNK_MAGIC);

	if (cb->dma) {
		pr_err("%s ERROR skb is already DMA mapped!\n", __func__);
		return 0;
	}

	pg = alloc_pages(GFP_ATOMIC | __GFP_COMP, tnk_rx_page_order);
	if (unlikely(!pg)) {
		pr_err("%s: alloc pages Error\n", __func__);
		return -ENOMEM;
	}

#if POISON_BUF_VAL >= 0
	memset(page_address(pg), POISON_BUF_VAL,
		PAGE_SIZE << tnk_rx_page_order);
#endif

	*dma = dma_map_page(tnk_dev, pg, 0,
				PAGE_SIZE << tnk_rx_page_order,
				DMA_FROM_DEVICE);

	if (dma_mapping_error(tnk_dev, *dma)) {
		__free_pages(pg, tnk_rx_page_order);
		*dma = 0;
		pr_err("%s: DMA Mapping Error\n", __func__);
		return -ENOMEM;
	}

	get_page(pg);
	skb_fill_page_desc(skb, 0, pg, 0, PAGE_SIZE << tnk_rx_page_order);
	skb->truesize += PAGE_SIZE << tnk_rx_page_order;

	return 0;
}

int tnkhw_alloc_rx_channel_skb(struct sk_buff **skb, dma_addr_t *dma)
{
	struct tnkcb *cb;

	*skb = tnk_alloc_skb(TNK_TCP_HEADER_RESERVED,
			GFP_ATOMIC | __GFP_NOWARN | __GFP_HIGH);
	if (unlikely(*skb == NULL)) {
		pr_warn("%s: Rx init fails; skb is NULL\n", __func__);
		return -ENOMEM;
	}
	skb_reserve(*skb, TNK_TCP_HEADER_RESERVED);

	/* Initialise the tnk private area */
	cb = &(TCP_SKB_CB(*skb)->header.tcb);
	cb->magic = TNK_MAGIC;
	cb->type = TNK_TYPE_FREE;
	cb->dma = 0;

	if (tnkhw_rx_alloc_page(*skb, dma) != 0) {
		tnk_free_skb(*skb, SKB_FROM_RECV);
		*skb = NULL;
		return -ENOMEM;
	}

	cb->dma = *dma;

	return 0;
}

int tnkhw_rx_channel_recycle_skb(struct sk_buff *skb, dma_addr_t *dma)
{
	struct tnkcb *cb;
	skb_frag_t *frag;
	struct page *pg;

	skb->tail = skb->data = skb->head;
	skb->len = skb->data_len = 0;

	skb_reserve(skb, TNK_TCP_HEADER_RESERVED);

	/* Initialise the tnk private area */
	cb = &(TCP_SKB_CB(skb)->header.tcb);
	cb->magic = TNK_MAGIC;
	cb->type = TNK_TYPE_FREE;

	frag = &skb_shinfo(skb)->frags[0];
	pg = skb_frag_page(frag);

#if POISON_BUF_VAL >= 0
	memset(page_address(pg), POISON_BUF_VAL,
		PAGE_SIZE << tnk_rx_page_order);
#endif

	*dma = dma_map_page(tnk_dev, pg, 0,
				PAGE_SIZE << tnk_rx_page_order,
				DMA_FROM_DEVICE);

	if (dma_mapping_error(tnk_dev, *dma)) {
		pr_err("%s: DMA Mapping Error\n", __func__);
		return -ENOMEM;
	}

	get_page(pg);
	skb_fill_page_desc(skb, 0, pg, 0, PAGE_SIZE << tnk_rx_page_order);

	cb->dma = *dma;

	return 0;
}

void tnkhw_unmap_lro_skb(struct sk_buff *skb)
{
	struct tcp_skb_cb *tcp_cb = TCP_SKB_CB(skb);
	struct tnkcb *cb = &(tcp_cb->header.tcb);

	BUG_ON(cb->magic != TNK_MAGIC);

	if (!cb->dma) {
		pr_err("%s ERROR skb is not DMA mapped!\n", __func__);
		return;
	}

	dma_unmap_page(tnk_dev, cb->dma, PAGE_SIZE << tnk_rx_page_order,
		DMA_FROM_DEVICE);

	cb->dma = 0;
	return;
}

void tnkhw_unmap_and_free_lro_skb(struct sk_buff *skb)
{
	skb_frag_t *frag;

	tnkhw_unmap_lro_skb(skb);

	frag = &skb_shinfo(skb)->frags[0];

	put_page(skb_frag_page(frag));

	tnk_free_skb(skb, SKB_FROM_RECV);
}

static int tnkhw_rx_dma_channel_init(void)
{
	int i, j;
	struct tnkhw_rx_dma_channel *r;
	struct tnk_rx_desc *p = NULL;
	struct tnk_rx_desc *last = NULL;
	int desc_offset;

	tnk_rx_dma_channel = (struct tnkhw_rx_dma_channel *)
		kzalloc(sizeof(struct tnkhw_rx_dma_channel) *
				tnk_max_connections, GFP_KERNEL);

	if (!tnk_rx_dma_channel) {
		pr_err("%s:ERROR allocating tnk_rx_dma_channel\n",
				__func__);
		return -ENOMEM;
	}

	for (i = 0; i < tnk_max_connections; i++) {
		r = tnk_rx_dma_channel + i;

		r->skbuff =
			kzalloc(sizeof(struct sk_buff *) * tnk_rx_dma_count,
					GFP_KERNEL);

		if (!r->skbuff) {
			pr_err("%s:ERROR allocating r->skbuff\n",
					__func__);
			return -ENOMEM;
		}

		r->skbuff_dma =
			kzalloc(sizeof(dma_addr_t) * tnk_rx_dma_count,
				GFP_KERNEL);
		if (!r->skbuff_dma) {
			pr_err("%s:ERROR allocating r->skbuff_dma\n",
					__func__);
			return -ENOMEM;
		}

		/* TODO - Ensure that descriptors are 128-bit (16-byte) aligned
		 * dma_alloc_coherent should normally return page-aligned
		 * memory,
		 * but it may not be guaranteed.  Low priority for now.
		 */
		r->desc_list =
			(struct tnk_rx_desc *)
			dma_alloc_coherent(tnk_dev,
					tnk_rx_dma_count *
					sizeof(struct tnk_rx_desc),
					&r->desc_list_phys,
					GFP_KERNEL);
		if (r->desc_list == NULL) {
			pr_err("%s:ERROR allocating r->desc_list\n", __func__);
			return -ENOMEM;
		}

		/* Clear the descriptors initially */
		memset(r->desc_list, 0,
				tnk_rx_dma_count * sizeof(struct tnk_rx_desc));

		/* Clear the Rx descriptors, construct a circular linked list */
		last = &r->desc_list[tnk_rx_dma_count - 1];
		for (j = 0; j < tnk_rx_dma_count; j++) {
			p = &r->desc_list[j];
			desc_offset = (j * sizeof(struct tnk_rx_desc));
			last->next_desc_ptr =
				(uint32_t) r->desc_list_phys + desc_offset;
			last = p;

			/* Set mutipacket int,the default value is
			 * 16 packet per int
			 */
			if (j % TNK_RX_NAPI_LIMIT_CHANNEL)
				p->disable_ic = 1;
		}

		/* Reset our head/tail indices */
		r->head = 0;
		r->tail = 0;
		spin_lock_init(&r->rx_desc_lock);
	}
	return 0;
}

static void tnkhw_rx_dma_channel_free(void)
{
	int i, j;
	struct tnkhw_rx_dma_channel *r;

	for (i = 0; i < tnk_max_connections; i++) {
		r = tnk_rx_dma_channel + i;

		if (r->skbuff) {
			for (j = 0; j < tnk_rx_dma_count; j++) {
				if (r->skbuff[j]) {
					pr_err("%s: skbuff=%p\n",
						__func__, r->skbuff[j]);
					r->skbuff[j] = NULL;
				}
			}
			kfree(r->skbuff);
			r->skbuff = NULL;
		}

		if (r->skbuff_dma) {
			for (j = 0; j < tnk_rx_dma_count; j++) {
				if (r->skbuff_dma[j]) {
					pr_err("%s: skbuff_dma=%x\n",
						__func__, r->skbuff_dma[j]);
					r->skbuff_dma[j] = 0;
				}
			}
			kfree(r->skbuff_dma);
			r->skbuff_dma = NULL;
		}

		/* Free the region of consistent memory previously allocated for
		 * the DMA
		 */
		if (r->desc_list) {
			dma_free_coherent(tnk_dev,
					tnk_rx_dma_count *
					sizeof(struct tnk_rx_desc),
					r->desc_list, r->desc_list_phys);
			r->desc_list = NULL;
		}
	}

	kfree(tnk_rx_dma_channel);
	tnk_rx_dma_channel = NULL;
}

int tnkhw_rx_channel_create(struct sock *sk)
{
	struct tnkinfo *t = &sk->sk_tnkinfo;
	struct tnkentry *e = t->entry;
	int cid;
	int err;
	int i;
	unsigned long flags, flags_rx;

	struct tnkhw_rx_dma_channel *r;
	struct tnk_rx_desc *p;

	int desc_count;
	int desc_reserved;
	int desc_total;
	int desc_max_fill;

	WARN_ON(e == NULL);

	cid = e->index;
	TNK_CINDEX_VALIDATE_WARN(cid);

	desc_count = DIV_ROUND_UP(sk->sk_rcvbuf,
				PAGE_SIZE << tnk_rx_page_order);
	desc_reserved = DIV_ROUND_UP(TNK_RX_RESERVED_SIZE,
				PAGE_SIZE << tnk_rx_page_order);

	/* TODO: large sk_rcvbuf may need more desc count,
	 * reserve MAX_TIMES desc_reserved
	 * to make sure RX fetch err can be handled.
	 */
	desc_max_fill = tnk_rx_dma_count - 1 -
		TNK_RXFETCH_MAX_TIMES * desc_reserved;
	if (desc_count >= desc_max_fill)
		desc_count = desc_max_fill;

	desc_total = desc_count + desc_reserved;

	WARN(desc_total >= tnk_rx_dma_count,
		"%s: cid=%d, sport=%d, dport=%d, rcvbuf=%d, desc_cnt=%d, "
		"desc_res=%d, total=%d, rx_dma_cnt=%d, order=%d\n",
		__func__, cid, ntohs(inet_sk(sk)->inet_sport),
		ntohs(inet_sk(sk)->inet_dport), sk->sk_rcvbuf, desc_count,
		desc_reserved, desc_total, tnk_rx_dma_count,
		tnk_rx_page_order);

	r = tnk_rx_dma_channel + cid;
	spin_lock_irqsave(&r->rx_desc_lock, flags_rx);

	WARN(r->head != r->tail,
		"%s: cid=%d, head=%d, tail=%d\n",
		__func__, cid, r->head, r->tail);

	for (i = 0; i < desc_total; i++) {
		err = tnkhw_alloc_rx_channel_skb(&r->skbuff[i],
				&r->skbuff_dma[i]);
		if (err) {
			TNK_LRO_DBG("%s: cid=%d, alloc skb failed!\n",
					__func__, cid);
			goto alloc_err;
		}

		p = r->desc_list + i;
		p->buffer_size = PAGE_SIZE << tnk_rx_page_order;
		p->buffer_addr = r->skbuff_dma[i];

		p->own = 1;
		atomic_inc(&tnk_stats.rx_lro_pages);
	}

	r->head = 0;
	r->tail = desc_total;
	r->channel_valid = 1;
	r->sk = sk;
	r->rcvbuf_desc_cnt = desc_count;
	r->total_desc_cnt = desc_total;

	STMMAC_SYNC_BARRIER();

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	tnkhw_rx_db_write(cid, TNK_RX_DB_DESC_ADDR, r->desc_list_phys);
	tnkhw_rx_db_write(cid, TNK_RX_DB_RX_OFFSET, 0);
	tnkhw_rx_db_write(cid, TNK_RX_DB_TX_ADV_WND,
		desc_count * (PAGE_SIZE << tnk_rx_page_order));

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	spin_unlock_irqrestore(&r->rx_desc_lock, flags_rx);

	return 0;
alloc_err:
	for (i = 0; i < desc_total; i++) {
		if (r->skbuff[i] == NULL)
			break;

		tnkhw_unmap_and_free_lro_skb(r->skbuff[i]);

		r->skbuff[i] = NULL;
		r->skbuff_dma[i] = 0;

		p = r->desc_list + i;
		p->buffer_size = 0;
		p->buffer_addr = 0;

		p->own = 0;
		atomic_dec(&tnk_stats.rx_lro_pages);
	}

	spin_unlock_irqrestore(&r->rx_desc_lock, flags_rx);

	return -1;
}

void tnkhw_lro_init_rcv_nxt(unsigned int cid, unsigned int tp_rcv_nxt)
{
	struct tnkhw_rx_dma_channel *r;

	TNK_CINDEX_VALIDATE_WARN(cid);
	r = tnk_rx_dma_channel + cid;

	r->tp_rcv_nxt = tp_rcv_nxt;

	TNK_LRO_DBG("%s: cid=%d, rcvnxt=%x\n",
		__func__, cid, tp_rcv_nxt);
}

bool tnkhw_check_tx_rx_zero_wnd(unsigned int cid)
{
	unsigned long flags;
	unsigned int tx_adv_wnd_ct;
	unsigned int rx_adv_wnd_ct;
	unsigned int rx_adv_wnd;
	unsigned int timer_tx_retry;
	bool ret = false;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	TNK_CINDEX_VALIDATE_WARN(cid);

	tnkhw_ct_read(cid, TNKHW_CT_LOC_TXADVWND, &tx_adv_wnd_ct);
	tx_adv_wnd_ct >>= 16;

	tnkhw_ct_read(cid, TNKHW_CT_LOC_RXADVWND, &rx_adv_wnd_ct);
	rx_adv_wnd = rx_adv_wnd_ct & 0xFFFF;

	timer_tx_retry = (rx_adv_wnd_ct >> 16) & 0x1;

	if (tx_adv_wnd_ct == 0 && rx_adv_wnd == 0 && timer_tx_retry)
		ret = true;

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return ret;
}

void tnkhw_sync_rx_wnd_from_db_to_ct(unsigned int cid)
{
	unsigned long flags;
	unsigned int tx_adv_wnd_ct;
	unsigned int tx_adv_wnd_db;
	unsigned int rcv_wscale;
	int limit = 20;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	TNK_CINDEX_VALIDATE_WARN(cid);

	tnk_conn_stats[cid].tx_adv_wnd_flush++;

	tnkhw_get_txadvwnd_scale(cid, &rcv_wscale);
	tnk_conn_stats[cid].tx_adv_wnd_scale = rcv_wscale;

	tx_adv_wnd_db = tnkhw_rx_db_read(cid, TNK_RX_DB_TX_ADV_WND);

	tnk_conn_stats[cid].tx_adv_wnd_db = tx_adv_wnd_db;

	tx_adv_wnd_db >>= rcv_wscale;

	if (tx_adv_wnd_db > 0xFFFF)
		tx_adv_wnd_db = 0xFFFF;

	do {
		/* We only want to write
		 * the upper 16 bits of the CT entry
		 */
		writel(0xC, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
		tnkhw_ct_write(cid, TNKHW_CT_LOC_TXADVWND,
				(tx_adv_wnd_db << 16) & 0xFFFF0000);
		writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

		udelay(100);

		tnkhw_ct_read(cid, TNKHW_CT_LOC_TXADVWND, &tx_adv_wnd_ct);
		tx_adv_wnd_ct >>= 16;

		tnk_conn_stats[cid].tx_adv_wnd_ct = tx_adv_wnd_ct << rcv_wscale;
	} while ((tx_adv_wnd_ct != tx_adv_wnd_db) &&
			((tx_adv_wnd_ct << rcv_wscale) < 2560) && limit--);

	if (limit < 0)
		tnk_conn_stats[cid].tx_adv_wnd_fail++;

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

static int tnkhw_tx_dma_init(void)
{
	int cindex;

	tnk_tx_dma_list =
	    kzalloc(tnk_max_connections * sizeof(struct tnkhw_tx_dma_info),
		    GFP_KERNEL);
	TNK_DBG("%s alloc %p\n", __func__, tnk_tx_dma_list);
	if (!tnk_tx_dma_list) {
		pr_err("%s: Failed to allocate memory for TX DMA info table\n",
		       __func__);
		return -ENOMEM;
	}

	for (cindex = TNK_TTX_CINDEX_START; cindex < tnk_max_connections;
	     cindex++) {
		int i;
		struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
		struct tnk_ttx_dma_desc *last = NULL;

		t->skbuff = kzalloc(tnk_tx_fifo * sizeof(struct sk_buff *),
				    GFP_KERNEL);
		if (!t->skbuff) {
			pr_err("%s: Failed to allocate memory for TX skbuff",
					__func__);
			pr_err(" table\n");
			return -ENOMEM;
		}

		/* TODO - Ensure that descriptors are 128-bit (16-byte) aligned
		 * (in physical memory) dma_alloc_coherent should normally
		 * return page-aligned memory, but it may not be guaranteed.
		 */
		t->desc_list =
		    (struct tnk_ttx_dma_desc *)
			dma_alloc_coherent(tnk_dev,
					   tnk_tx_fifo *
					   sizeof(struct tnk_ttx_dma_desc),
					   &t->desc_list_phys,
					   GFP_KERNEL);
		if (!t->desc_list) {
			pr_err("%s: ERROR allocating DMA Tx descriptor list\n",
			       __func__);
			return -ENOMEM;
		}

		memset(t->desc_list, 0,
		       tnk_tx_fifo * sizeof(struct tnk_ttx_dma_desc));
		/* Clear the Tx descriptors, construct a circular linked list */
		last = &t->desc_list[tnk_tx_fifo - 1];
		for (i = 0; i < tnk_tx_fifo; i++) {
			struct tnk_ttx_dma_desc *desc = &t->desc_list[i];
			int desc_offset = (i * sizeof(struct tnk_ttx_dma_desc));
			last->next_desc_ptr =
			    (uint32_t) t->desc_list_phys + desc_offset;
			last = desc;
		}

		spin_lock_init(&t->tx_desc_lock);
		t->head = 0;
		t->tail = 0;
#ifdef DBG_TX_DMA
		t->head_overflow = 0;
#endif

		/* Add this descriptor ring to the TTX DB */
		tnkhw_ttx_db_init(cindex, t->desc_list_phys);

		init_waitqueue_head(&t->waitqueue);
	}

	return 0;
}

static void tnkhw_tx_dma_free(void)
{
	unsigned cindex;
	struct tnk_ttx_dma_desc *desc;

	if (tnk_tx_dma_list == NULL)
		return;

	for (cindex = TNK_TTX_CINDEX_START; cindex < tnk_max_connections;
	     cindex++) {
		int i;
		struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];

		for (i = 0; i < tnk_tx_fifo; i++) {
			if (t->desc_list) {
				desc = &t->desc_list[i];
				if (desc->buffer_ptr &&
					(desc->buffer_ptr !=
						TNK_TTX_BUF_RECLAIMED)) {
					dma_unmap_single(tnk_dev,
							desc->buffer_ptr,
							desc->buffer_size,
							DMA_TO_DEVICE);
					desc->buffer_ptr = 0;
				}
			}

			if (t->skbuff && t->skbuff[i]) {
				tnk_free_skb(t->skbuff[i], SKB_FROM_SEND);
				t->skbuff[i] = NULL;

			}
		}

		/* Free the region of consistent memory previously allocated for
		 * the DMA */
		if (t->desc_list) {
			dma_free_coherent(tnk_dev,
					tnk_tx_fifo *
					sizeof(struct tnk_ttx_dma_desc),
					t->desc_list,
					t->desc_list_phys);
		}
		kfree(t->skbuff);
		t->skbuff = NULL;
	}

	kfree(tnk_tx_dma_list);
	tnk_tx_dma_list = NULL;
}

#if SWITCH_MULTI_INTR
void tnkhw_rx_refill(unsigned int channel)
#else
void tnkhw_rx_refill(void)
#endif
{
	unsigned int rxsize = tnk_rx_fifo;
#if SWITCH_MULTI_INTR
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma[channel];
#else
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma;
#endif
	unsigned count = 0;
	unsigned long long bitwize_tnk;

	spin_lock(&r->rx_desc_lock);

	bitwize_tnk = r->head + 0x100000000ull;

	TNK_DBG("%s begin\n", __func__);

	for (; ((r->head > r->tail) || ((r->head < r->tail) &&
		(bitwize_tnk > r->tail)));  r->tail++) {
		unsigned int entry = r->tail % rxsize;
		struct tnk_rx_dma_desc *p = r->desc_list + entry;

		TNK_DBG("%s skb for entry %d is %p\n", __func__,
			entry, r->skbuff[entry]);

		if (likely(r->skbuff[entry] == NULL)) {
			struct sk_buff *skb;
			int err;

#if SWITCH_MULTI_INTR
			skb = skb_dequeue(&(tnk_rx_dma[0].free_skbs));
#else
			skb = skb_dequeue(&r->free_skbs);
#endif
			tnk_mem_free_pool_dequeued(skb);

			if (skb == NULL) {
				err =
				    tnkhw_alloc_and_map_skb(&skb,
							    &r->
							    skbuff_dma[entry]);

				if (unlikely(err != 0)) {
					pr_warn
					    ("%s: Failed to allocate new skb\n",
					     __func__);
					/* Check for buffer starvation due to
					 * lack of memory as this may stall the
					 * rx queue until memory frees up and
					 * we're (hopefully) woken up by a
					 * transmit event */
					break;
				}
				tnk_stats.rx_refills_alloc++;
			} else {
				struct tnkcb *cb
					= &(TCP_SKB_CB(skb)->header.tcb);
				r->skbuff_dma[entry] = cb->dma;
			}

			count_skb++;
			TNK_DBG("%s refill skb\n", __func__);
			r->skbuff[entry] = skb;
			p->base.des2 = r->skbuff_dma[entry];
			p->base.des01.erx.buffer1_size = DMA_BUFFER_DESC_SIZE;
		}

		/* barrier here because hardware may read this desc
		 * before desc is right configured.
		 */
		STMMAC_SYNC_BARRIER();

		p->base.des01.erx.own = 1;
		BUG_ON(p->base.des01.erx.own == 0);
		count++;
	}

	if (count > 0) {
		unsigned long flags;

		TNK_DBG("%s tell hw about %d new buffs\n", __func__, count);

		/* This barrier is important here.  It is required to ensure
		 * the ARM CPU flushes it's DMA write buffers before proceeding
		 * to the next instruction, to ensure that the TOE will see
		 * our descriptor changes in memory */
		STMMAC_SYNC_BARRIER();

		/*  Let the hardware know that new buffers are available */
		spin_lock_irqsave(&tnkhw_reg_lock, flags);
		tnkhw_trx_desc_add(count);
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

#if SWITCH_MULTI_INTR
		TNK_DBG("%s enable dma rcv on chan %d io=%p\n", __func__,
			DMA_CHANNEL_TOERX + channel, tnkhw_ioaddr);
		dwmac1000_dma_ops.enable_dma_receive(tnkhw_ioaddr,
						DMA_CHANNEL_TOERX + channel);
#else
		TNK_DBG("%s enable dma rcv on chan %d io=%p\n", __func__,
			DMA_CHANNEL_TOERX, tnkhw_ioaddr);
		dwmac1000_dma_ops.enable_dma_receive(tnkhw_ioaddr,
						     DMA_CHANNEL_TOERX);
#endif

		tnk_stats.rx_refills += count;
		TNK_DBG("%s: alloc_skb: %d\n", __func__, count_skb);
	}

	spin_unlock(&r->rx_desc_lock);

	TNK_DBG("%s end\n", __func__);
}

static void tnkhw_fix_tcp_seq(struct sk_buff *skb)
{
	struct ethhdr *eth;
	unsigned char *ip_start, *tcp_start;
	const struct iphdr *iph;
	const struct tcphdr *th;

	eth = (struct ethhdr *)skb->data;

	if (!(ntohs(eth->h_proto) >= ETH_P_802_3_MIN))
		return;

	ip_start = skb->data + ETH_HLEN;
#if SWITCH_VLAN
	if (eth->h_proto == cpu_to_be16(ETH_P_8021Q))
		ip_start += VLAN_HLEN;
#endif

	iph = (struct iphdr *)ip_start;

	tcp_start = ip_start + iph->ihl * 4;
	th = (struct tcphdr *)tcp_start;
	skb->len = ETH_HLEN + ntohs(iph->tot_len);
#if SWITCH_VLAN
	if (eth->h_proto == cpu_to_be16(ETH_P_8021Q))
		skb->len += VLAN_HLEN;
#endif

	TCP_SKB_CB(skb)->seq = ntohl(th->seq);
	TCP_SKB_CB(skb)->end_seq = TCP_SKB_CB(skb)->seq + th->syn + th->fin +
			(ntohs(iph->tot_len) - iph->ihl * 4 - th->doff * 4);
}

#if SWITCH_MULTI_INTR
int tnkhw_rx(int limit, unsigned int channel)
#else
int tnkhw_rx(int limit)
#endif
{
	unsigned int rxsize = tnk_rx_fifo;
#if SWITCH_MULTI_INTR
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma[channel];
#else
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma;
#endif
	unsigned int entry;
	unsigned int next_entry;
	unsigned int count = 0;
	unsigned int max_valid;
	struct tnk_rx_dma_desc *p;
	struct tnk_rx_dma_desc *p_next;

	TNK_DBG("%s begin\n", __func__);

	if (unlikely((limit < 0) || (limit > rxsize)))
		limit = rxsize;

	spin_lock(&r->rx_desc_lock);

	entry = r->head % rxsize;
	p = r->desc_list + entry;

	/*  Prevent overtaking the tail pointer on wrap-around */
	max_valid = r->tail + rxsize - r->head;
	if (limit > max_valid)
		limit = max_valid;

	count = 0;
	while (!p->base.des01.erx.own) {
		int status;
		int cindex = p->extra.cid;

		if (count >= limit)
			break;

		TNK_DBG("%s update counters\n", __func__);
		/*  Update counters */
		count++;

		/*  Prefetch the next entry in the queue */
		next_entry = (++r->head) % rxsize;
		p_next = r->desc_list + next_entry;
		prefetch(p_next);

#ifdef TNK_HW_DEBUG
		{
			int i;
			uint32_t *desc_ptr = (uint32_t *) p;
			TNK_DBG("Rx Descriptor contents (in words)\n");
			for (i = 0;
			     i <
			     sizeof(struct tnk_rx_dma_desc) / sizeof(uint32_t);
			     i++) {
				TNK_DBG("Offset %d, value 0x%08X\n", i,
					desc_ptr[i]);
			}
		}
#endif

		if (((cindex) < TNK_TTX_CINDEX_START)
		    || ((cindex) >= tnk_max_connections)) {
			pr_err("%s descriptor %d has invalid index %d\n",
			       __func__, entry, cindex);
			status = discard_frame;
		} else {
			tnk_conn_stats[cindex].rx_pkts++;

			/* TODO: half duplex problem, fix next version */
#if 0
			TNK_DBG("%s read status\n", __func__);
			/* read the status of the incoming frame */
			status = enh_desc_ops.rx_status(NULL,
							&tnk_stmmac_stats,
							&p->base);
			/*we do not know the exact order of the frame,
			 so all frames are theat as out of order
			 */
			if (status == csum_none)
#endif
				status = out_of_order_frame;

		}

		if (unlikely(p->extra.flags & TNKHW_CB_FLAG_TOE_ERR)) {

			tnk_conn_stats[cindex].rx_toe_err++;

			/* NOTE
			 * We don't discard the frame automatically if this
			 * flag is set because of a known issue in the TOE
			 * which can cause this flag to be set erroneously
			 * if the IPC/CSUM bit is asserted in RDES0
			 * So we only discard the frame if one of the following
			 * error flags are set, effectively ignoring the
			 * RDS0 IPC/CSUM bit, which is safe because the TOE
			 * does it's own IP and TCP checks anyway.
			 */

			if (p->extra.flags & TNKHW_CB_FLAG_PREMATURE_EOP) {
				tnk_conn_stats[cindex].rx_premature_eop++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_BAD_ETHERTYPE) {
				tnk_conn_stats[cindex].rx_bad_ethertype++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_BAD_DST_MACADDR) {
				tnk_conn_stats[cindex].rx_bad_dst_macaddr++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_TCP_CSUM_ERR) {
				tnk_conn_stats[cindex].rx_tcp_csum_err++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_IPHDR_CSUM_ERR) {
				tnk_conn_stats[cindex].rx_iphdr_csum_err++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_NON_TCP_PROT) {
				tnk_conn_stats[cindex].rx_non_tcp_prot++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_DST_IP_MISMATCH) {
				tnk_conn_stats[cindex].rx_bad_dst_ipaddr++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_CONNECTION_ERR) {
				tnk_conn_stats[cindex].rx_connection_err++;
				status = discard_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_RX_SEQNUM_ERR) {
				tnk_conn_stats[cindex].rx_bad_seq_num++;
				/* seq num erro frame is out of order frame **/
				status = out_of_order_frame;
			}

			if (p->extra.flags & TNKHW_CB_FLAG_PKT_DROPPED) {
				tnk_conn_stats[cindex].rx_busy_pkt_drop++;
				status = discard_frame;
			}
#if SWITCH_TOE_ERR_PKT
			if (p->extra.flags & TNKHW_CB_FLAG_RESERVED) {
				tnk_conn_stats[cindex].rx_out_of_window++;
				status = discard_frame;
			}
#endif

			if (status == discard_frame) {
				TNK_DBG("%s dropping rx pkt, desc %d,",
						__func__, entry);
				TNK_DBG(" cindex %d, flags=0x%x, status=%d\n",
						cindex, p->extra.flags, status);
				/* Errored frame, increment stats
				 * NOTE that the skbuff is left in the Rx DMA
				 * ring, to be reused later
				 */
				tnk_conn_stats[cindex].rx_dropped++;
			}
		}

		if (likely(status == out_of_order_frame)) {
			struct sk_buff *skb;
			int frame_len;

			skb = r->skbuff[entry];
			if (unlikely(!skb)) {
				pr_err("%s: Inconsistent Rx descriptor chain,",
						__func__);
				pr_err(" no skb at index %d\n", entry);
				tnk_conn_stats[cindex].rx_dropped++;
				break;
			}
			frame_len = p->base.des01.erx.frame_length;

			tnkhw_unmap_skb(skb);
			r->skbuff[entry] = NULL;
			r->skbuff_dma[entry] = 0;
			skb_put(skb, frame_len);

			TCP_SKB_CB(skb)->seq = p->extra.seq_num;
			TCP_SKB_CB(skb)->end_seq =
				TCP_SKB_CB(skb)->seq + frame_len;
			TCP_SKB_CB(skb)->header.tcb.flag = p->extra.flags;
			if (TCP_SKB_CB(skb)->header.tcb.flag &
				TNKHW_CB_FLAG_FULL_PKT)
				tnkhw_fix_tcp_seq(skb);
			tnk_rxcallback(skb, cindex, p->extra.flags,
					       p->extra.urg_ptr);
		}
		entry = next_entry;
		p = p_next;	/* use prefetched values */
	}

	spin_unlock(&r->rx_desc_lock);

	TNK_DBG("%s do refill\n", __func__);
#if SWITCH_MULTI_INTR
	tnkhw_rx_refill(channel);
#else
	tnkhw_rx_refill();
#endif

	TNK_DBG("%s done\n", __func__);
	return count;
}

#if SWITCH_RECV_LRO
void tnkhw_lro_free_skb(struct sk_buff *skb)
{
	struct tnkcb *cb = &(TCP_SKB_CB(skb)->header.tcb);
	skb_frag_t *frag;

	if (cb && cb->magic == TNK_MAGIC &&
			cb->type == TNK_TYPE_LRO_DATA &&
			(skb_shinfo(skb)->nr_frags == 1)) {
		frag = &skb_shinfo(skb)->frags[0];
		if ((frag->page_offset + frag->size) ==
				(PAGE_SIZE << tnk_rx_page_order)) {
			atomic_dec(&tnk_stats.rx_lro_pages);
		}
	}
}

/* Judge whether we need adjust hardware rcvbuf to socket rcvbuf.
 * If we need increase hardware rcvbuf, this function will refill new
 * buffer and return the number of buffer refilled.
 * If we need decrease hardware rcvbuf, this function just returns -1.
 */
int tnkhw_rx_lro_rcvbuf_adjust(unsigned int cid, int need_refill)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;
	unsigned long flags;
	int count = 0;
	int max_valid;
	struct tnk_rx_desc *p;
	unsigned int entry;
	int err;
	int desc_reserved;
	int sk_rcvbuf_desc_cnt;
	int ret_adjust_cnt = 0;

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	spin_lock_irqsave(&r->rx_desc_lock, flags);

	WARN(r->channel_valid == 0,
		"%s: cid=%d, head=%d, tail=%d\n",
		__func__, cid, r->head, r->tail);

	max_valid = rxsize - 1 - r->total_desc_cnt;

	/* When adjust hardware rcvbuf, reserve 2*RX_RESERVED desc */
	desc_reserved = DIV_ROUND_UP(TNK_RX_RESERVED_SIZE,
				PAGE_SIZE << tnk_rx_page_order);
	if (max_valid > TNK_RXFETCH_MAX_TIMES * desc_reserved)
		max_valid -= TNK_RXFETCH_MAX_TIMES * desc_reserved;
	else
		max_valid = 0;

	/* ret_adjust_cnt: rx desc need increased.
	 * If we should decrease rx desc, ret_adjust_cnt is -1;
	 * We should reserve at least TNK_LRO_MIN_DESC_CNT descriptor
	 * to make sure recycle skb ok and tcp window can be restored.
	 */
	sk_rcvbuf_desc_cnt = DIV_ROUND_UP(r->sk->sk_rcvbuf,
			PAGE_SIZE << tnk_rx_page_order);
	if (sk_rcvbuf_desc_cnt >= r->rcvbuf_desc_cnt)
		ret_adjust_cnt = sk_rcvbuf_desc_cnt - r->rcvbuf_desc_cnt;
	else if (r->rcvbuf_desc_cnt > TNK_LRO_MIN_DESC_CNT)
		ret_adjust_cnt = -1;

	if (ret_adjust_cnt > 0 && ret_adjust_cnt > max_valid)
		ret_adjust_cnt = max_valid;

	for (count = 0; count < ret_adjust_cnt; count++) {
		entry = r->tail;
		p = r->desc_list + entry;

		WARN_ON(r->skbuff[entry]);

		err = tnkhw_alloc_rx_channel_skb(&r->skbuff[entry],
				&r->skbuff_dma[entry]);
		if (err)
			break;

		p->buffer_size = PAGE_SIZE << tnk_rx_page_order;
		p->buffer_addr = r->skbuff_dma[entry];
		p->own = 1;

		STMMAC_SYNC_BARRIER();

		r->tail = (r->tail + 1) % rxsize;
	}

	if ((ret_adjust_cnt == -1) && (need_refill == 1)) {
		r->rcvbuf_desc_cnt--;
		r->total_desc_cnt--;
		atomic_dec(&tnk_stats.rx_lro_pages);
	} else {
		r->rcvbuf_desc_cnt += count;
		r->total_desc_cnt += count;
		atomic_add(count, &tnk_stats.rx_lro_pages);
	}

	if (ret_adjust_cnt > 0)
		ret_adjust_cnt = count;

	spin_unlock_irqrestore(&r->rx_desc_lock, flags);

	if (count > 0) {
		/* This barrier is important here.  It is required to ensure
		 * the ARM CPU flushes it's DMA write buffers before proceeding
		 * to the next instruction, to ensure that the TOE will see
		 * our descriptor changes in memory
		 */
		STMMAC_SYNC_BARRIER();

		/*  Let the hardware know that new buffers are available */
		tnkhw_add_tx_wnd(cid, count * (PAGE_SIZE << tnk_rx_page_order),
				TX_WND_ADD);
	}

	return ret_adjust_cnt;
}

void tnkhw_rx_lro_recycle_skb(unsigned int cid, struct sk_buff *skb)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;
	unsigned long flags;
	unsigned int max_valid;
	struct tnk_rx_desc *p;
	unsigned int entry;

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	spin_lock_irqsave(&r->rx_desc_lock, flags);

	WARN(r->channel_valid == 0,
		"%s: cid=%d, head=%d, tail=%d\n",
		__func__, cid, r->head, r->tail);

	/*  Prevent overtaking the tail pointer on wrap-around */
	if (r->tail >= r->head)
		max_valid = rxsize - 1 - (r->tail - r->head);
	else
		max_valid = r->head - r->tail - 1;

	WARN(max_valid == 0,
		"%s: cid=%d, head=%d, tail=%d, max_valid=%d\n",
		__func__, cid, r->head, r->tail, max_valid);
	if (unlikely(max_valid == 0)) {
		r->rcvbuf_desc_cnt--;
		r->total_desc_cnt--;
		atomic_dec(&tnk_stats.rx_lro_pages);
		goto recycle_fail;
	}

	entry = r->tail;
	p = r->desc_list + entry;

	WARN_ON(r->skbuff[entry]);
	r->skbuff[entry] = skb;

	tnkhw_rx_channel_recycle_skb(skb, &r->skbuff_dma[entry]);

	p->buffer_size = PAGE_SIZE << tnk_rx_page_order;
	p->buffer_addr = r->skbuff_dma[entry];
	p->own = 1;

	STMMAC_SYNC_BARRIER();

	r->tail = (r->tail + 1) % rxsize;

	spin_unlock_irqrestore(&r->rx_desc_lock, flags);

	/* This barrier is important here.  It is required to ensure
	 * the ARM CPU flushes it's DMA write buffers before proceeding
	 * to the next instruction, to ensure that the TOE will see
	 * our descriptor changes in memory
	 */
	STMMAC_SYNC_BARRIER();

	/*  Let the hardware know that new buffers are available */
	tnkhw_add_tx_wnd(cid, PAGE_SIZE << tnk_rx_page_order, TX_WND_ADD);

	return;

recycle_fail:
	spin_unlock_irqrestore(&r->rx_desc_lock, flags);
	tnk_free_skb(skb, SKB_FROM_RECV);
}

unsigned int tnkhw_lro_refill(unsigned int cid, unsigned int limit)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;
	unsigned long flags;
	unsigned count = 0;
	unsigned int max_valid;
	struct tnk_rx_desc *p;
	unsigned int entry;
	int err;

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	spin_lock_irqsave(&r->rx_desc_lock, flags);

	WARN(r->channel_valid == 0,
		"%s: not valid, cid=%d, limit=%d, head=%d, tail=%d\n",
		__func__, cid, limit, r->head, r->tail);
	if (r->channel_valid == 0)
		goto out;

	/*  Prevent overtaking the tail pointer on wrap-around */
	if (r->tail >= r->head)
		max_valid = rxsize - 1 - (r->tail - r->head);
	else
		max_valid = r->head - r->tail - 1;

	if (max_valid == 0) {
		pr_warn("%s: no desc, cid=%d, max_valid=%d, head=%d, tail=%d\n",
			__func__, cid, max_valid, r->head, r->tail);
		goto out;
	}

	if (limit > max_valid)
		limit = max_valid;

	TNK_LRO_DBG("%s: cid=%d, limit=%d, head=%d, tail=%d, max_valid=%d\n",
			__func__, cid, limit, r->head, r->tail, max_valid);

	for (count = 0; count < limit; count++) {
		entry = r->tail;
		p = r->desc_list + entry;

		WARN_ON(r->skbuff[entry]);

		err = tnkhw_alloc_rx_channel_skb(&r->skbuff[entry],
				&r->skbuff_dma[entry]);
		if (err) {
			pr_warn("%s: Failed to alloc skb, cid=%d, h=%d, t=%d\n",
				__func__, cid, r->head, r->tail);
			break;
		}

		p->buffer_size = PAGE_SIZE << tnk_rx_page_order;
		p->buffer_addr = r->skbuff_dma[entry];
		p->own = 1;

		STMMAC_SYNC_BARRIER();

		r->tail = (r->tail + 1) % rxsize;
	}

	r->total_desc_cnt += count;
out:
	atomic_add(count, &tnk_stats.rx_lro_pages);
	spin_unlock_irqrestore(&r->rx_desc_lock, flags);

	return count;
}

void tnkhw_rx_lro_refill(unsigned int cid, unsigned int limit)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;
	unsigned long flags;
	unsigned count = 0;
	unsigned int max_valid;
	struct tnk_rx_desc *p;
	unsigned int entry;
	int err;

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	spin_lock_irqsave(&r->rx_desc_lock, flags);

	/*  Prevent overtaking the tail pointer on wrap-around */
	if (r->tail >= r->head)
		max_valid = rxsize - 1 - (r->tail - r->head);
	else
		max_valid = r->head - r->tail - 1;

	if (limit > max_valid)
		limit = max_valid;

	TNK_LRO_DBG("%s: cid=%d, limit=%d, head=%d, tail=%d, max_valid=%d\n",
			__func__, cid, limit, r->head, r->tail, max_valid);

	for (count = 0; count < limit; count++) {
		entry = r->tail;
		p = r->desc_list + entry;

		WARN_ON(r->skbuff[entry]);

		err = tnkhw_alloc_rx_channel_skb(&r->skbuff[entry],
				&r->skbuff_dma[entry]);
		if (err) {
			pr_warn("%s: Failed to allocate new skb\n",
					__func__);
			break;
		}
		tnk_stats.rx_refills_alloc++;

		p->buffer_size = PAGE_SIZE << tnk_rx_page_order;
		p->buffer_addr = r->skbuff_dma[entry];
		p->own = 1;

		STMMAC_SYNC_BARRIER();

		r->tail = (r->tail + 1) % rxsize;
	}

	r->total_desc_cnt += count;

	spin_unlock_irqrestore(&r->rx_desc_lock, flags);

	if (count > 0) {
		TNK_LRO_DBG("%s tell hw about %d new buffs\n", __func__, count);

		/* This barrier is important here.  It is required to ensure
		 * the ARM CPU flushes it's DMA write buffers before proceeding
		 * to the next instruction, to ensure that the TOE will see
		 * our descriptor changes in memory
		 */
		STMMAC_SYNC_BARRIER();

		/*  Let the hardware know that new buffers are available */
		tnkhw_add_tx_wnd(cid, count * (PAGE_SIZE << tnk_rx_page_order),
				TX_WND_ADD);

		tnk_stats.rx_refills += count;
	}
}

int tnkhw_lro_build_skb(struct sk_buff *skb, unsigned int seq)
{
	skb_frag_t *frag;

	frag = &skb_shinfo(skb)->frags[0];
	skb->len = frag->size;
	skb->data_len = skb->len;

	tnkhw_unmap_lro_skb(skb);

	put_page(skb_frag_page(frag));

	TCP_SKB_CB(skb)->seq = seq;
	TCP_SKB_CB(skb)->end_seq = seq + skb->len;

	return 0;
}

int tnkhw_lro_build_new_skb(struct sk_buff *oldskb, struct sk_buff **nskb,
		unsigned int seq, unsigned int recv_data_size)
{
	struct sk_buff *newskb;
	skb_frag_t *frag;
	struct tnkcb *cb;
	void *pg_va;

	newskb = tnk_alloc_skb(TNK_TCP_HEADER_RESERVED +
			recv_data_size,
			GFP_ATOMIC | __GFP_NOWARN | __GFP_HIGH);
	if (unlikely(newskb == NULL)) {
		pr_warn("%s: Rx alloc skb fails\n", __func__);
		return -1;
	}

	*nskb = newskb;

	skb_reserve(newskb, TNK_TCP_HEADER_RESERVED);

	cb = &(TCP_SKB_CB(oldskb)->header.tcb);

	dma_sync_single_for_cpu(tnk_dev, cb->dma,
			PAGE_SIZE << tnk_rx_page_order,
			DMA_FROM_DEVICE);

	frag = &skb_shinfo(oldskb)->frags[0];
	pg_va = page_address(skb_frag_page(frag));
	pg_va += frag->page_offset;

	skb_copy_to_linear_data(newskb, pg_va, recv_data_size);

	dma_sync_single_for_cpu(tnk_dev, cb->dma,
			PAGE_SIZE << tnk_rx_page_order,
			DMA_FROM_DEVICE);

	/* Initialise the tnk private area */
	cb = &(TCP_SKB_CB(newskb)->header.tcb);
	cb->magic = TNK_MAGIC;
	cb->type = TNK_TYPE_LRO_DATA;

	newskb->len = recv_data_size;

	TCP_SKB_CB(newskb)->seq = seq;
	TCP_SKB_CB(newskb)->end_seq = seq + newskb->len;

	frag->page_offset += recv_data_size;
	frag->size -= recv_data_size;

	return 0;
}

#if 1
int tnkhw_rx_connection(int limit, unsigned int cid)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;
	unsigned long flags;

	unsigned int entry;
	unsigned int count = 0;
	unsigned int max_valid;
	struct tnk_rx_desc *p;

	struct sk_buff *skb;
	skb_frag_t *frag;

	unsigned int recv_data_size;
	unsigned int hw_rcv_nxt;

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	if (unlikely((limit < 0) || (limit > rxsize)))
		limit = rxsize;

	/* Notice !!!
	 * we must hold e->list_lock first and then rx_desc_lock,
	 * otherwise dead lock will happen. The possible condition:
	 * CPU0:  tasklet_action --> tnk_ct_channel_rx --> tnkhw_rx_lro -->
	 *	tnkhw_rx_connection (hold rx_desc_lock) -->
	 *	tnk_ct_lro_rx_callback --> (require list_lock);
	 * CPU1: tcp_close --> tnk_tcp_close --> tnk_ct_remove (hold list_lock)
	 *	--> tnkhw_connection_remove --> tnkhw_rx_lro_cleanup -->
	 *	(require rx_desc_lock);
	 */
	if (tnk_ct_hold_list_lock(cid))
		goto release_list_lock;

	spin_lock_irqsave(&r->rx_desc_lock, flags);

	if (!r->channel_valid)
		goto out;

	entry = r->head;
	p = r->desc_list + entry;

	/*  Prevent overtaking the tail pointer on wrap-around */
	if (r->tail >= r->head)
		max_valid = r->tail - r->head;
	else
		max_valid = r->tail + rxsize - r->head;

	if (limit > max_valid)
		limit = max_valid;

	TNK_LRO_DBG("%s: cid=%d, head=%d, tail=%d, limit=%d, valid=%d\n",
		__func__, cid, r->head, r->tail, limit, max_valid);

	if (limit == 0)
		goto out;

	/* TODO: poll limit should make sure all packet received,
	 * no irq dropped.
	 */
	hw_rcv_nxt = tnkhw_get_rcv_nxt(cid);
	WARN(before(hw_rcv_nxt, r->tp_rcv_nxt),
			"%s: hw before sw, cid=%d, hw_nxt=%x, tp_nxt=%x\n",
			__func__, cid, hw_rcv_nxt, r->tp_rcv_nxt);
	if (before(hw_rcv_nxt, r->tp_rcv_nxt)) {
		tnkhw_tx_err_dump_info(cid);
		goto out;
	}

	recv_data_size = hw_rcv_nxt - r->tp_rcv_nxt;

	count = 0;
	while (recv_data_size) {
		if (count >= limit)
			break;

		skb = r->skbuff[entry];
		if (unlikely(!skb)) {
			pr_err("%s: Inconsistent Rx descriptor chain,",
					__func__);
			pr_err(" no skb at index %d\n", entry);
			pr_err("%s: cid=%d, head=%d, tail=%d, "
				"limit=%d, valid=%d\n",
				__func__, cid, r->head, r->tail,
				limit, max_valid);
			tnk_conn_stats[cid].rx_dropped++;
			goto out;
		}

		frag = &skb_shinfo(skb)->frags[0];

		if (recv_data_size >= frag->size) {
			STMMAC_SYNC_BARRIER();
			WARN(p->own, "%s:cid=%d\n", __func__, cid);
			if (p->own) {
				int own_check_cnt = 10000;

				char *buf =
					(char *)page_address(
						skb_frag_page(frag)) +
					frag->page_offset;
				int len = frag->size;
				pr_err("%s: cid=%d, head=%d, tail=%d, "
						"limit=%d, valid=%d\n",
						__func__, cid, r->head,
						r->tail, limit, max_valid);
				pr_err("%s:cid=%d, hw_nxt=%x, tp_nxt=%x, "
						"rcv_sz=%d, off=%x, fragsize=%d\n",
						__func__, cid, hw_rcv_nxt,
						r->tp_rcv_nxt, recv_data_size,
						frag->page_offset, frag->size);

				tnkhw_tx_err_dump_info(cid);
				tnk_print_memory(buf, len);

				while (p->own && (own_check_cnt--))
					STMMAC_SYNC_BARRIER();

				pr_err("%s: cur_hw_seq=0x%x\n", __func__,
						tnkhw_rx_db_read(cid,
						TNK_RX_DB_RCV_NXT));
				tnkhw_tx_err_dump_info(cid);
			}

			tnkhw_lro_build_skb(skb, r->tp_rcv_nxt);
			r->tp_rcv_nxt += skb->len;

			r->skbuff[entry] = NULL;
			r->skbuff_dma[entry] = 0;

			r->head = (r->head + 1) % rxsize;

			entry = r->head;
			p = r->desc_list + entry;

			recv_data_size -= skb->len;

			tnk_lro_rxcallback(skb, cid);
		} else {
			struct sk_buff *newskb = NULL;
			int err;
			int try_cnt = 0;
			int try_limit = 100;

			do {
				err = tnkhw_lro_build_new_skb(skb, &newskb,
						r->tp_rcv_nxt, recv_data_size);
				try_cnt++;
				if (err)
					mdelay(1);
			} while (err && (try_cnt < try_limit));

			if (err) {
				pr_err("%s,%d: alloc fail, cid=%d, nxt=%x\n",
					__func__, __LINE__, cid, r->tp_rcv_nxt);
				goto out;
			}

			r->tp_rcv_nxt += newskb->len;
			recv_data_size = 0;

			tnk_lro_rxcallback(newskb, cid);
		}
		count++;
		tnk_conn_stats[cid].rx_pkts++;
	}
out:
	spin_unlock_irqrestore(&r->rx_desc_lock, flags);

release_list_lock:
	tnk_ct_release_list_lock(cid);

	tnk_ct_ofo_queue(cid);

	TNK_LRO_DBG("%s done\n", __func__);
	return count;
}
#else
int tnkhw_rx_connection(int limit, unsigned int cid)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;

	unsigned int entry;
	unsigned int next_entry;
	unsigned int count = 0;
	unsigned int max_valid;
	struct tnk_rx_desc *p;
	struct tnk_rx_desc *p_next;

	struct sk_buff *skb;
	skb_frag_t *frag;

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	if (unlikely((limit < 0) || (limit > rxsize)))
		limit = rxsize;

	spin_lock(&r->rx_desc_lock);

	if (!r->channel_valid)
		goto out;

	entry = r->head;
	p = r->desc_list + entry;

	/*  Prevent overtaking the tail pointer on wrap-around */
	if (r->tail >= r->head)
		max_valid = r->tail - r->head;
	else
		max_valid = r->tail + rxsize - r->head;

	if (limit > max_valid)
		limit = max_valid;

	TNK_LRO_DBG("%s: cid=%d, head=%d, tail=%d, limit=%d, valid=%d\n",
		__func__, cid, r->head, r->tail, limit, max_valid);

	if (limit == 0)
		goto out;

	/* TODO: poll limit should make sure all packet received,
	 * no irq dropped.
	 */
	count = 0;
	while (!p->own) {
		if (count >= limit)
			break;

		/*  Update counters */
		count++;

		/*  Prefetch the next entry in the queue */
		r->head = (r->head + 1) % rxsize;
		next_entry = r->head;
		p_next = r->desc_list + next_entry;
#ifdef TNK_RECV_LRO_DEBUG
		{
			int i;
			uint32_t *desc_ptr = (uint32_t *) p;
			TNK_LRO_DBG("Rx Descriptor contents (in words)\n");
			for (i = 0;
			     i <
			     sizeof(struct tnk_rx_desc) / sizeof(uint32_t);
			     i++) {
				TNK_LRO_DBG("Offset %d, value 0x%08X\n", i,
					desc_ptr[i]);
			}
		}
#endif

		tnk_conn_stats[cid].rx_pkts++;

		skb = r->skbuff[entry];
		if (unlikely(!skb)) {
			pr_err("%s: Inconsistent Rx descriptor chain,",
					__func__);
			pr_err(" no skb at index %d\n", entry);
			pr_err("%s: cid=%d, head=%d, tail=%d, lmt=%d, vld=%d\n",
				__func__, cid,
				r->head, r->tail,
				limit, max_valid);
			tnk_conn_stats[cid].rx_dropped++;
			goto out;
		}

		tnkhw_lro_build_skb(skb, r->tp_rcv_nxt);
		r->tp_rcv_nxt += skb->len;

		r->skbuff[entry] = NULL;
		r->skbuff_dma[entry] = 0;

		TNK_LRO_DBG("%s: cid=%d, skblen=%d, rcvnxt=%x\n",
			__func__, cid, skb->len, r->tp_rcv_nxt);

		tnk_lro_rxcallback(skb, cid);

		entry = next_entry;
		p = p_next;	/* use prefetched values */
	}

	if (count < limit) {
		unsigned int recv_data_size;
		unsigned int hw_rcv_nxt;

		hw_rcv_nxt = tnkhw_get_rcv_nxt(cid);

		WARN(before(hw_rcv_nxt, r->tp_rcv_nxt),
			"%s: hw before sw, cid=%d, hw_nxt=%x, tp_nxt=%x\n",
			__func__, cid, hw_rcv_nxt, r->tp_rcv_nxt);
		if (before(hw_rcv_nxt, r->tp_rcv_nxt)) {
			tnkhw_tx_err_dump_info(cid);
			goto out;
		}

		recv_data_size = hw_rcv_nxt - r->tp_rcv_nxt;

		TNK_LRO_DBG("%s:cid=%d, hw_nxt=%x, tp_nxt=%x, rcv_sz=%d\n",
			__func__, cid, hw_rcv_nxt, r->tp_rcv_nxt,
			recv_data_size);

		if (recv_data_size == 0)
			goto out;

		skb = r->skbuff[entry];
		if (unlikely(!skb)) {
			pr_err("%s: Inconsistent Rx descriptor chain,",
					__func__);
			pr_err(" no skb at index %d\n", entry);
			tnk_conn_stats[cid].rx_dropped++;
			goto out;
		}

		frag = &skb_shinfo(skb)->frags[0];

		TNK_LRO_DBG("%s: cid=%d, frag_size=%d\n",
			__func__, cid, frag->size);

		if (recv_data_size >= frag->size) {
			STMMAC_SYNC_BARRIER();
			WARN(p->own, "%s:cid=%d\n", __func__, cid);
			if (p->own) {
				int own_check_cnt = 10000;

				char *buf =
					(char *)page_address(
						skb_frag_page(frag)) +
					frag->page_offset;
				int len = frag->size;
				pr_err("%s: cid=%d, head=%d, tail=%d, "
						"limit=%d, valid=%d\n",
						__func__, cid, r->head,
						r->tail, limit, max_valid);
				pr_err("%s:cid=%d, hw_nxt=%x, tp_nxt=%x, "
						"rcv_sz=%d, off=%x, fragsize=%d\n",
						__func__, cid, hw_rcv_nxt,
						r->tp_rcv_nxt, recv_data_size,
						frag->page_offset, frag->size);

				tnkhw_tx_err_dump_info(cid);
				tnk_print_memory(buf, len);

				while (p->own && (own_check_cnt--))
					STMMAC_SYNC_BARRIER();

				pr_err("%s: cur_hw_seq=0x%x\n", __func__,
						tnkhw_rx_db_read(cid,
						TNK_RX_DB_RCV_NXT));
				tnkhw_tx_err_dump_info(cid);
			}

			tnkhw_lro_build_skb(skb, r->tp_rcv_nxt);
			r->tp_rcv_nxt += skb->len;

			r->skbuff[entry] = NULL;
			r->skbuff_dma[entry] = 0;

			r->head = (r->head + 1) % rxsize;

			tnk_lro_rxcallback(skb, cid);
		} else {
			struct sk_buff *newskb = NULL;
			int err;

			err = tnkhw_lro_build_new_skb(skb, &newskb,
					r->tp_rcv_nxt, recv_data_size);

			if (err)
				goto out;
			else
				r->tp_rcv_nxt += newskb->len;

			tnk_lro_rxcallback(newskb, cid);

			TNK_LRO_DBG("%s: cid=%d, old_pg_off=%x, old_size=%d\n",
					__func__, cid,
					frag->page_offset,
					frag->size);
		}
		count++;
	}
out:
	spin_unlock(&r->rx_desc_lock);

	tnk_ct_ofo_queue(cid);

	TNK_LRO_DBG("%s done\n", __func__);
	return count;
}
#endif

int tnkhw_rx_lro(unsigned int channel)
{
	unsigned cindex = 0;
	int overflow = 0;
	unsigned long flags;
	int poll_limit = tnk_rx_lro_poll_limit;

	/*  Read from RX INTR FIFO until empty
	 */
	do {
		overflow |= tnkhw_rx_lro_cid_read(&cindex, channel);

		/*  If overflow set, we'll check all channels below
		 */
		if (cindex > 0)
			tnkhw_rx_connection(poll_limit, cindex);
	} while (cindex > 0);

	if (overflow) {
		/*  RX INTR FIFO overflowed, so we need to scan all connection
		 * for receive RX buffers
		 */
		unsigned int bitmap_val = 0;
		unsigned int entry;

		for (cindex = 0; cindex < tnk_max_connections; cindex++) {
			if (!(cindex % TNK_RX_BITMAP_SIZE)) {
				entry = cindex / TNK_RX_BITMAP_SIZE;
				spin_lock_irqsave(&tnkhw_reg_lock, flags);
				bitmap_val = tnkhw_rx_bitmap_read(entry);
				spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
			}

			if (cindex == 0 || cindex == 1)
				continue;

			if (bitmap_val & (1 << (cindex % TNK_RX_BITMAP_SIZE)))
				tnkhw_rx_connection(poll_limit, cindex);
		}
	}

	return 0;
}

int tnkhw_rx_lro_cleanup(unsigned int cid)
{
	unsigned int rxsize = tnk_rx_dma_count;
	struct tnkhw_rx_dma_channel *r;

	unsigned int entry;
	unsigned int count = 0;
	unsigned int max_valid;
	struct tnk_rx_desc *p;

	unsigned long flags;

	TNK_LRO_DBG("%s begin\n", __func__);

	TNK_CINDEX_VALIDATE_WARN(cid);

	r = tnk_rx_dma_channel + cid;

	spin_lock_irqsave(&r->rx_desc_lock, flags);

	/*  Prevent overtaking the tail pointer on wrap-around */
	if (r->tail >= r->head)
		max_valid = r->tail - r->head;
	else
		max_valid = r->tail + rxsize - r->head;

	TNK_LRO_DBG("%s: cid=%d, head=%d, tail=%d, valid=%d\n",
		__func__, cid, r->head, r->tail, max_valid);

	count = 0;
	for (count = 0; count < max_valid; count++) {
		entry = r->head;
		p = r->desc_list + entry;

		WARN(r->skbuff[entry] == NULL,
				"%s: cid=%d, head=%d, tail=%d, valid=%d\n",
				__func__, cid, r->head, r->tail, max_valid);

		if (r->skbuff[entry])
			tnkhw_unmap_and_free_lro_skb(r->skbuff[entry]);

		r->skbuff[entry] = NULL;
		r->skbuff_dma[entry] = 0;

		p->buffer_size = 0;
		p->buffer_addr = 0;
		p->own = 0;

		r->head = (r->head + 1) % rxsize;
	}

	r->channel_valid = 0;
	r->sk = NULL;

	atomic_sub(count, &tnk_stats.rx_lro_pages);

	spin_unlock_irqrestore(&r->rx_desc_lock, flags);

	TNK_LRO_DBG("%s done\n", __func__);
	return count;
}

#endif

#ifdef CONFIG_ARCH_HI3536
void tnkhw_init_out_of_order(unsigned int cindex)
{
	unsigned long flags;
	unsigned val;
	int valid = 0;
	int count = 0;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	/*  set iso flag 1,2,3,4 to 0 */
	val = TNK_HW_OUT_OF_ORDER_IS_VALID_FLAG | (cindex << 5) | (1 << 4);

	while (count < 100000) {
		count++;
		valid = readl(tnkhw_ioaddr + TNK_HW_OUT_OF_ORDER_CRTL);
		valid = (valid >> 14) & 0x1;
		if (valid)
			break;
	}

	if (!valid)
		pr_err("wait out of order cpu operation timeout\n");
	writel(0, tnkhw_ioaddr + TNK_HW_OUT_OF_ORDER_WVAL);
	writel(val, tnkhw_ioaddr + TNK_HW_OUT_OF_ORDER_CRTL);
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#else
void tnkhw_init_out_of_order(unsigned int cindex)
{
	unsigned long flags;
	unsigned val;
	int valid = 0;
	int count = 0;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	/*  set iso flag1&&iso flag2 to 0 */
	val = TNK_HW_OUT_OF_ORDER_IS01_IS02_FLAG | (cindex << 4) | (1 << 3);

	while (count < 100000) {
		count++;
		valid = readl(tnkhw_ioaddr + TNK_HW_OUT_OF_ORDER_CRTL);
		valid = (valid >> 12) & 0x1;
		if (valid)
			break;
	}

	if (!valid)
		pr_err("wait out of order cpu operation timeout\n");
	writel(0, tnkhw_ioaddr + TNK_HW_OUT_OF_ORDER_WVAL);
	writel(val, tnkhw_ioaddr + TNK_HW_OUT_OF_ORDER_CRTL);
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

}
#endif
#if 0
void tnkhw_update_fin_seqnum(unsigned int cindex)
{
	unsigned int data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/* update seqnum when fin recieved  */
	tnkhw_ct_read(cindex, 8, &data);
	data += 1;
	tnkhw_ct_write(cindex, 8, data);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif
void tnkhw_update_rx_seqnum(unsigned int cindex, unsigned int seq_num)
{
	unsigned long flags;
	struct tnkhw_connection conn;
	int i, offset;
	uint32_t *data = (uint32_t *)&conn;
	int max_conn;

	conn.word30_updated_tx_ack_num = seq_num;
	conn.word31_updated_flag = 1;

	offset = offsetof(struct tnkhw_connection, word30_updated_tx_ack_num);
	offset = offset/sizeof(uint32_t);

	if (offset != 30)
		pr_warn("error calculate of offset :%d\n", offset);
	offset = 30;
	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/*  Write the partial connection table entry, one word at a time */
	max_conn = sizeof(struct tnkhw_connection) / sizeof(uint32_t);
	for (i = offset; i < max_conn; i++)
		tnkhw_ct_write(cindex, i, data[i]);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

/*  Return number of free Tx descriptors */
static inline int tnkhw_tx_avail_desc(unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];

#ifdef DBG_TX_DMA
	if ((t->tail + tnk_tx_fifo - t->head) > tnk_tx_fifo) {
		pr_warn("DEBUG_L:(%s,%d): wrong index, cindex=%d, ",
			__func__, __LINE__, cindex);
		dump_tx_dma_info(t);
		dump_stack();
	}
#endif
	return t->tail + tnk_tx_fifo - t->head - 1;
}

/* We have a 16MB limit on the amount of data that we can have in the pipeline
 * for any given channel
 */
int tnkhw_tx_avail_bytes(unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
	int pipelined_bytes = atomic_read(&t->pipelined_bytes);

	/* This counter should never go out of range  */
	BUG_ON(pipelined_bytes < 0);
	BUG_ON(pipelined_bytes > TNK_TTX_DATA_LIMIT);

	return TNK_TTX_DATA_LIMIT - pipelined_bytes;
}

#if SWITCH_RECV_LRO
int tnkhw_lro_check_refill(unsigned int cid, struct sk_buff *skb)
{
	struct tnkcb *cb = &(TCP_SKB_CB(skb)->header.tcb);
	skb_frag_t *frag;
	int need_refill = 0;
	int rcvbuf_adjust = 0;

	if (cb && cb->magic == TNK_MAGIC &&
		cb->type == TNK_TYPE_LRO_DATA &&
		(skb_shinfo(skb)->nr_frags == 1)) {
		frag = &skb_shinfo(skb)->frags[0];
		TNK_LRO_DBG("%s: pg_off=%x, size=%d\n",
			__func__, frag->page_offset, frag->size);
		if ((frag->page_offset + frag->size) ==
			(PAGE_SIZE << tnk_rx_page_order)) {
			need_refill = 1;
		}
	}

	rcvbuf_adjust = tnkhw_rx_lro_rcvbuf_adjust(cid, need_refill);

	if (need_refill && (rcvbuf_adjust >= 0))
		tnkhw_rx_lro_recycle_skb(cid, skb);
	else
		tnkhw_skb_recycle(skb);

	return (rcvbuf_adjust > 0) || (rcvbuf_adjust == 0 && need_refill);
}
#endif

void tnkhw_skb_recycle(struct sk_buff *skb)
{
#ifdef TNK_RX_CHANNEL_FLOW_CONTROL
	int limit = tnk_rx_recycle_fifo;
#else
	int limit = (tnk_rx_fifo + TNK_RX_SW_Q_SIZE);
#endif

#if SWITCH_MULTI_INTR
	if ((skb_queue_len(&(tnk_rx_dma[0].free_skbs)) < limit)) {
#else
	if ((skb_queue_len(&tnk_rx_dma.free_skbs) < limit)) {
#endif
		struct tnkcb *cb = &(TCP_SKB_CB(skb)->header.tcb);
		if (likely(cb && cb->magic == TNK_MAGIC &&
			cb->type == TNK_TYPE_DATA)) {
			skb->tail = skb->data = skb->head;
			skb->len = 0;

			skb_reserve(skb, TNK_TCP_HEADER_RESERVED);
			tnkhw_map_skb(skb, &cb->dma);

#if SWITCH_MULTI_INTR
			skb_queue_head(&(tnk_rx_dma[0].free_skbs), skb);
#else
			skb_queue_head(&tnk_rx_dma.free_skbs, skb);
#endif
			tnk_mem_free_pool_queued(skb);
		} else {
			tnk_free_skb(skb, SKB_FROM_RECV);
		}
	} else {
		tnk_free_skb(skb, SKB_FROM_RECV);
	}
}

void tnkhw_tx_channel_reclaim(unsigned cindex, int cleanup)
{
	struct tnkhw_tx_dma_info *t;
	unsigned int txsize = tnk_tx_fifo;
	unsigned count = 0, acked_bytes = 0;
#if SWITCH_SEND_FIN
	bool fin_acked = false;
	bool need_check_fin_send = false;
#endif
	unsigned long flags;
#ifdef DBG_TX_DMA
	unsigned int max_free_count;
#endif

	if (unlikely(tnk_tx_dma_list == NULL))
		return;

	t = &tnk_tx_dma_list[cindex];

	spin_lock_irqsave(&t->tx_desc_lock, flags);

#ifdef DBG_TX_DMA
	max_free_count = t->head - t->tail;
	if ((t->head >= t->tail) && ((t->head - t->tail) < tnk_tx_fifo) &&
		!(t->head_overflow)) {
		/* normal */
	} else if ((t->head < t->tail) &&
		(t->tail + tnk_tx_fifo - 1 >= t->head) &&
		(t->head_overflow)) {
		/* t->head has overflow */
	} else {
		pr_warn("DEBUG_L:(%s,%d): wrong index. index=%d, cleanup=%d, ",
			__func__, __LINE__, cindex, cleanup);
		dump_tx_dma_info(t);
		dump_stack();
	}
#endif

	while (t->tail != t->head) {
		unsigned int entry = t->tail % txsize;
		struct sk_buff *skb = t->skbuff[entry];
		struct tnk_ttx_dma_desc *p = t->desc_list + entry;

		/*  On shutdown/cleanup, we need to reclaim ALL buffers in the
		 *  pipeline */
		if (unlikely(cleanup))
			p->hw_own = 0;

#if SWITCH_SEND_FIN
		/* can't check fin sent state and send ACK here,
		 * because these actions need to lock tnkhw_reg_lock.
		 * But tnkhw_connection_remove may have hold this lock,
		 * we have hold tx_desc_lock, so maybe dead lock.
		 */
		if (p->hw_own && (p->buffer_size == 1) && skb &&
				(TCP_SKB_CB(skb)->tcp_flags & TCPHDR_FIN))
			need_check_fin_send = true;
#endif
		/* Check if the descriptor is owned by the DMA. */
		if (p->hw_own)
			break;

		if (likely(p->buffer_ptr)
		    && p->buffer_ptr != TNK_TTX_BUF_RECLAIMED) {
			acked_bytes += p->buffer_size;

			dma_unmap_single(tnk_dev, p->buffer_ptr,
					 p->buffer_size, DMA_TO_DEVICE);

			/*  Mark the buffer pointer as invalid */
			/*  p->buffer_ptr = 0; */
			p->buffer_ptr = TNK_TTX_BUF_RECLAIMED;

			if (likely(skb != NULL)) {
#if SWITCH_SEND_FIN
				if (p->buffer_size == 1 &&
				(TCP_SKB_CB(skb)->tcp_flags & TCPHDR_FIN) &&
					!cleanup)
					fin_acked = true;
#endif
				tnk_free_skb(skb, SKB_FROM_SEND);
				t->skbuff[entry] = NULL;
			}
		}

		entry = (++t->tail) % txsize;
	#ifdef DBG_TX_DMA
		if (!(t->tail)) {
			pr_warn("(%s,%d): t->tail will overflow.",
				__func__, __LINE__);
			pr_warn("index=%d, cleanup=%d,",
				cindex, cleanup);
			pr_warn("max_free_count=%d, count=%d\n",
				max_free_count, count);
			dump_tx_dma_info(t);
			t->head_overflow = 0;
		}
	#endif
		count++;
	#ifdef DBG_TX_DMA
		if ((t->head >= t->tail) &&
			((t->head - t->tail) < tnk_tx_fifo) &&
			!(t->head_overflow)) {
			/* normal */
		} else if ((t->head < t->tail) &&
				(t->tail + tnk_tx_fifo - 1 >= t->head) &&
				(t->head_overflow)) {
			/* t->head has overflow */
		} else {
			pr_warn("(%s,%d): wrong index. index=%d, cleanup=%d, ",
					__func__, __LINE__, cindex, cleanup);
			dump_tx_dma_info(t);
			dump_stack();
		}
	#endif
	}
	spin_unlock_irqrestore(&t->tx_desc_lock, flags);

#if SWITCH_SEND_FIN
	if (need_check_fin_send) {
		if (!tnkhw_get_fin_send_state(cindex))
			tnkhw_send_ack(cindex);
	}
#endif

	if (count) {
		TNK_DBG("%s: Reclaimed %d descriptors for channel %d\n",
			__func__, count, cindex);

		atomic_sub(acked_bytes, &t->pipelined_bytes);

		/*  TODO - remove this flag if no longer needed */
		if (t->overflow)
			t->overflow = 0;

		tnk_conn_stats[cindex].tx_desc_ackd += count;
		tnk_conn_stats[cindex].tx_bytes_ackd += acked_bytes;
		tnk_stats.tx_returns += acked_bytes;

		if (likely(!cleanup)) {
			/* Trigger a callback to let the socket know that
			 * there's room on the queue again to transmit more
			 * data */
			if (tnk_txcallback)
#if SWITCH_SEND_FIN
				tnk_txcallback(cindex, acked_bytes, fin_acked);
#else
				tnk_txcallback(cindex, acked_bytes);
#endif
			else
				pr_err("%s: ERROR: TX Callback not set\n",
				       __func__);
		}

		/* once reclaim skb, wakeup transmit */
		if (waitqueue_active(&t->waitqueue))
			wake_up_interruptible(&t->waitqueue);

		/* once data is ACKed and released, we immediately send
		 * the skb queued in transmit queue.
		 */
		if (!cleanup)
			tnk_ct_tx_backlog_flush_all(cindex);
	}
}

#if SWITCH_KEEPALIVE
int tnkhw_check_tx_dma(unsigned int cindex)
{
	struct tnkhw_tx_dma_info *t;
	unsigned long flags;
	int ret = 0;

	if (unlikely(tnk_tx_dma_list == NULL))
		return ret;

	t = &tnk_tx_dma_list[cindex];
	spin_lock_irqsave(&t->tx_desc_lock, flags);
	if (t->tail != t->head)
		ret = 1;
	spin_unlock_irqrestore(&t->tx_desc_lock, flags);

	return ret;
}
#endif

#if SWITCH_MULTI_INTR
int tnkhw_tx_reclaim(unsigned int channel)
#else
int tnkhw_tx_reclaim(void)
#endif
{
	unsigned cindex = 0;
	int overflow = 0;

	/*  Read from ACK FIFO until empty */
	do {
#if SWITCH_MULTI_INTR
		overflow |= tnkhw_ttx_ackd_cid_read(&cindex, channel);
#else
		overflow |= tnkhw_ttx_ackd_cid_read(&cindex);
#endif
		TNK_DBG("%s reclaim cindex %d\n", __func__, cindex);

		/*  If overflow set, we'll check all channels below */
		if (cindex > 0)
			tnkhw_tx_channel_reclaim(cindex, 0);

	} while (cindex > 0);

	if (overflow) {
		tnk_stats.ttx_ack_ovflow++;
		/*  ACK FIFO overflowed, so we need to scan all channels for
		 *  free TX buffers */
		return 1;
	}

	return 0;
}

int tnkhw_dma_start(tnkhw_poll_wakeup_cb_t poll_wakeup_cb,
#if SWITCH_MULTI_INTR
		tnkhw_channel_poll_wakeup_cb_t channel_poll_wakeup_cb,
#endif
#if SWITCH_RECV_LRO
		tnkhw_lro_rx_cb_t lro_rx_cb,
#endif
		    tnkhw_rx_cb_t rx_cb,
		    tnkhw_txfree_cb_t tx_cb,
		    tnkhw_tx_max_retries_cb_t tx_max_retries_cb)
{
#if SWITCH_MULTI_INTR
	int i;
	tnk_channel_pollwakeup = channel_poll_wakeup_cb;
#endif
#if SWITCH_RECV_LRO
	tnk_lro_rxcallback = lro_rx_cb;
#endif
	tnk_pollwakeup = poll_wakeup_cb;
	tnk_rxcallback = rx_cb;
	tnk_txcallback = tx_cb;
	tnk_txmaxretries = tx_max_retries_cb;

	dwmac1000_dma_ops.start_rx(tnkhw_ioaddr, DMA_CHANNEL_TOERX);
#if SWITCH_MULTI_INTR
	for (i = 1; i < TOE_MULTI_INTR_NUM; i++)
		dwmac1000_dma_ops.start_rx(tnkhw_ioaddr, DMA_CHANNEL_TOERX + i);
#endif

	return 0;
}

void tnkhw_dma_stop(void)
{
	int cindex;
#if SWITCH_MULTI_INTR
	int i;
#endif

	dwmac1000_dma_ops.stop_rx(tnkhw_ioaddr, DMA_CHANNEL_TOERX);
#if SWITCH_MULTI_INTR
	for (i = 1; i < TOE_MULTI_INTR_NUM; i++)
		dwmac1000_dma_ops.stop_rx(tnkhw_ioaddr, DMA_CHANNEL_TOERX + i);
#endif

	/* Clear out any buffers left in the DMA rings
	 * ASSUMPTION - all active connections have been removed
	 * at this point
	 * TODO - add a check for the above assumption
	 */
	for (cindex = TNK_TTX_CINDEX_START; cindex < tnk_max_connections;
	     cindex++)
		tnkhw_tx_channel_reclaim(cindex, 1);

#if SWITCH_MULTI_INTR
	tnk_channel_pollwakeup = NULL;
#endif
#if SWITCH_RECV_LRO
	tnk_lro_rxcallback = NULL;
#endif
	tnk_pollwakeup = NULL;
	tnk_rxcallback = NULL;
	tnk_txcallback = NULL;
	tnk_txmaxretries = NULL;
}

#define TNK_DMA_TTX_DFETCH0_ERR \
	(TNK_MASK_TTX_ERR_DFETCH0 << TNK_OFFSET_TTX_ERR_DFETCH0)
#define TNK_DMA_TTX_DFETCH1_ERR \
	(TNK_MASK_TTX_ERR_DFETCH1 << TNK_OFFSET_TTX_ERR_DFETCH1)
#define TNK_DMA_TTX_DFETCH2_ERR \
	(TNK_MASK_TTX_ERR_DFETCH2 << TNK_OFFSET_TTX_ERR_DFETCH2)
#define TNK_DMA_TTX_DFETCH_ERRORS \
	(TNK_DMA_TTX_DFETCH0_ERR | TNK_DMA_TTX_DFETCH1_ERR \
	 | TNK_DMA_TTX_DFETCH2_ERR)

#define TNK_DMA_TTX_RETRY0_ERR \
	(TNK_MASK_TTX_ERR_RETRY0 << TNK_OFFSET_TTX_ERR_RETRY0)
#define TNK_DMA_TTX_RETRY1_ERR \
	(TNK_MASK_TTX_ERR_RETRY1 << TNK_OFFSET_TTX_ERR_RETRY1)
#define TNK_DMA_TTX_RETRY_ERRORS \
	(TNK_DMA_TTX_RETRY0_ERR | TNK_DMA_TTX_RETRY1_ERR)

#define TNK_DMA_TTX_ACKD0_ERR \
	(TNK_MASK_TTX_ERR_ACKD0 << TNK_OFFSET_TTX_ERR_ACKD0)
#define TNK_DMA_TTX_ACKD1_ERR \
	(TNK_MASK_TTX_ERR_ACKD1 << TNK_OFFSET_TTX_ERR_ACKD1)
#define TNK_DMA_TTX_ACKD2_ERR \
	(TNK_MASK_TTX_ERR_ACKD2 << TNK_OFFSET_TTX_ERR_ACKD2)
#define TNK_DMA_TTX_ACKD_ERRORS \
	(TNK_DMA_TTX_ACKD0_ERR | TNK_DMA_TTX_ACKD1_ERR | TNK_DMA_TTX_ACKD2_ERR)

#define TNK_DMA_RX_OWN_ERR \
	(TNK_MASK_RX_OWN_ERR << TNK_OFFSET_RX_OWN_ERR)

static void tnkhw_dump_tx_db(unsigned int cindex)
{
	int i;
	unsigned int offset = cindex * TNK_TTX_DMA_DB_ENTRY_SIZE
				+ TNK_DMA_TTX_DB_OFFSET;

	pr_info("%s:\n", __func__);

	for (i = 0; i < 4; i++)
		pr_info("word[%d]:%08x\n",
			i, readl(tnkhw_ioaddr + offset + i*4));
}

#if SWITCH_RECV_LRO
static void tnkhw_dump_rx_lro_db(unsigned int cindex)
{
	unsigned int val;
	pr_info("%s:\n", __func__);

	val = tnkhw_rx_db_read(cindex, TNK_RX_DB_DESC_ADDR);
	pr_info("word[%d]:%08x\n", TNK_RX_DB_DESC_ADDR, val);

	val = tnkhw_rx_db_read(cindex, TNK_RX_DB_RCV_NXT);
	pr_info("word[%d]:%08x\n", TNK_RX_DB_RCV_NXT, val);

	val = tnkhw_rx_db_read(cindex, TNK_RX_DB_RX_OFFSET);
	pr_info("word[%d]:%08x\n", TNK_RX_DB_RX_OFFSET, val);

	val = tnkhw_rx_db_read(cindex, TNK_RX_DB_TX_ADV_WND);
	pr_info("word[%d]:%08x\n", TNK_RX_DB_TX_ADV_WND, val);
}
#endif

static void tnkhw_dump_debug_reg(void)
{
	int i = 0;

	pr_err("debug registers:\n");
	for (i = 0; i < 128; i++) {
		if ((i%4) == 0)
			pr_err("\n%x :",
				CONFIG_STMMAC_IOADDR + 0x8200 + i * 4);
		pr_err("%08x\t", readl(tnkhw_ioaddr + 0x8200 + i * 4));
	}

	pr_err("\n");

	for (i = 0; i < 128; i++) {
		if ((i%4) == 0)
			pr_err("\n%x :",
				CONFIG_STMMAC_IOADDR + 0x8400 + i * 4);
		pr_err("%08x\t", readl(tnkhw_ioaddr + 0x8400 + i * 4));
	}

	pr_err("\n");

	for (i = 0; i < 64; i++) {
		if ((i%4) == 0)
			pr_err("\n%x:",
				CONFIG_STMMAC_IOADDR + 0x1e100 + i * 4);
		pr_err("%08x\t",
			readl(tnkhw_ioaddr + 0x1e100 + i * 4));
	}

}

static void tnkhw_tx_err(void)
{
	/*  Read the DMA_TTX_ERR_STATUS register, and related CID registers */
	uint32_t err_status = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_ERR_STATUS);
	uint32_t cid;

	/*  Check for dfetch errors */
	if (err_status & TNK_DMA_TTX_DFETCH_ERRORS) {
		cid = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_DFETCH_ERR_CID);
		cid =
		    (cid >> TNK_OFFSET_TTX_DFETCH_ERR_CID) &
		    TNK_MASK_TTX_DFETCH_ERR_CID;

		pr_err("%s: TTX defetch error 0x%08x (cid %d)\n",
				__func__, err_status, cid);

		if (err_status & TNK_DMA_TTX_DFETCH0_ERR)
			tnk_conn_stats[cid].tx_dfetch_desc++;
		if (err_status & TNK_DMA_TTX_DFETCH1_ERR)
			tnk_conn_stats[cid].tx_dfetch_data++;
		if (err_status & TNK_DMA_TTX_DFETCH2_ERR) {
			pr_err("%s: TTX error 0x%08x (cid %d)\n",
					__func__, err_status, cid);
#ifdef TNK_DBG_TTX_ERR
			pr_err("%s: cid=%d, last used ct table:======\n",
				__func__, cid);
			tnkhw_connection_print(&last_conn[cid]);
#endif
			tnkhw_dump_tx_descriptors(cid);
			tnkhw_connection_dump(cid);
			tnkhw_dump_tx_db(cid);
			tnkhw_dump_debug_reg();
			tnk_conn_stats[cid].tx_dfetch_own++;
		}
	}

	/*  Check for retry errors */
	if (err_status & TNK_DMA_TTX_RETRY_ERRORS) {
		cid = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_RETRY_ERR_CID);
		cid =
		    (cid >> TNK_OFFSET_TTX_RETRY_ERR_CID) &
		    TNK_MASK_TTX_RETRY_ERR_CID;

		pr_err("%s: TTX retry error 0x%08x (cid %d)\n",
				__func__, err_status, cid);

		if (err_status & TNK_DMA_TTX_RETRY0_ERR)
			tnk_conn_stats[cid].tx_retry_desc++;
		if (err_status & TNK_DMA_TTX_RETRY1_ERR)
			tnk_conn_stats[cid].tx_retry_data++;
	}

	/*  Check for ackd errors */
	if (err_status & TNK_DMA_TTX_ACKD_ERRORS) {
		cid = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_ACKD_ERR_CID);
		cid =
		    (cid >> TNK_OFFSET_TTX_ACKD_ERR_CID) &
		    TNK_MASK_TTX_ACKD_ERR_CID;

		pr_err("%s: TTX acked error 0x%08x (cid %d)\n",
				__func__, err_status, cid);

		tnkhw_dump_tx_descriptors(cid);
		tnkhw_connection_dump(cid);
		tnkhw_dump_tx_db(cid);
		tnkhw_dump_debug_reg();

		if (err_status & TNK_DMA_TTX_ACKD0_ERR)
			tnk_conn_stats[cid].tx_ackd_read++;
		if (err_status & TNK_DMA_TTX_ACKD1_ERR)
			tnk_conn_stats[cid].tx_ackd_size++;
		if (err_status & TNK_DMA_TTX_ACKD2_ERR)
			tnk_conn_stats[cid].tx_ackd_write++;
	}

#if SWITCH_RECV_LRO
	/* Check for LRO own errors */
	if (err_status & TNK_DMA_RX_OWN_ERR) {
		unsigned int desc_reserved = 0;
		unsigned int filled = 0;
		int cnt = 0;
		int cnt_limit = 8000;

		cid = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_RXFETCH_ERR_CID);
		cid =
			(cid >> TNK_OFFSET_RXFETCH_ERR_CID) &
			TNK_MASK_RXFETCH_ERR_CID;

		pr_err("%s: LRO rxfetch error 0x%08x (cid %d)\n",
				__func__, err_status, cid);

		TNK_CINDEX_VALIDATE_WARN(cid);

		desc_reserved = DIV_ROUND_UP(TNK_RX_RESERVED_SIZE,
				PAGE_SIZE << tnk_rx_page_order);

		do {
			filled = tnkhw_lro_refill(cid, desc_reserved);
			cnt++;
			if (cnt > cnt_limit) {
				pr_err("%s: rxfetch fill fail. cid=%d\n",
					__func__, cid);
				dump_stack();
				break;
			}
		} while (filled == 0);

		if (filled)
			tnkhw_rxfetch_poll();
	}
#endif
}

void tnkhw_tx_err_dump_info(int cid)
{
	unsigned long flags;

	tnkhw_dump_tx_descriptors(cid);
	tnkhw_connection_dump(cid);

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	tnkhw_dump_tx_db(cid);
#if SWITCH_RECV_LRO
	tnkhw_dump_rx_lro_db(cid);
#endif
	tnkhw_dump_debug_reg();
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

static void tnkhw_dump_tx_descriptors_nolock(unsigned cindex);
void tnkhw_tx_err_dump_info_nolock(int cid)
{
	unsigned long flags;

	tnkhw_dump_tx_descriptors_nolock(cid);
	tnkhw_connection_dump(cid);

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	tnkhw_dump_tx_db(cid);
	tnkhw_dump_debug_reg();
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

int tnkhw_tx_desc_check(int cindex)
{
	struct tnkhw_tx_dma_info *t;
	unsigned int txsize = tnk_tx_fifo;
	unsigned long flags;
	int i;
	int ret = 0;

	if (unlikely(tnk_tx_dma_list == NULL))
		return ret;

	t = &tnk_tx_dma_list[cindex];

	spin_lock_irqsave(&t->tx_desc_lock, flags);

	for (i = 0; i < txsize; i++) {
		struct tnk_ttx_dma_desc *desc = t->desc_list + i;

		/* Check if the descriptor is owned by the DMA. */
		if (desc->hw_own) {
#ifdef TNK_DBG_KEY_POINT
			pr_err("%s: TTX Descriptor (chan %u, ring index %u)\n"
					"\tack_offset: %u\n"
					"\treserved1: %u\n"
					"\thw_own: %u\n"
					"\tbuffer_size: %u\n"
					"\treserved2: %u\n"
					"\tintr_on_completion: %u\n"
					"\tbuffer_ptr: 0x%08X\n"
					"\tnext_desc_ptr: 0x%08X\n",
					__func__,
					cindex, i,
					desc->ack_offset,
					desc->reserved1,
					desc->hw_own,
					desc->buffer_size,
					desc->reserved2,
					desc->intr_on_completion,
					desc->buffer_ptr, desc->next_desc_ptr);
#endif
			desc->hw_own = 0;
			ret = 1;
		}
	}
	spin_unlock_irqrestore(&t->tx_desc_lock, flags);
	return ret;
}

int tnkhw_tx_send(struct sk_buff *skb, unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
	unsigned int txsize = tnk_tx_fifo;
	unsigned int entry;
	unsigned int last;
	unsigned int count = 0;
	int entries_needed;
	uint32_t total_len = 0;
	int i;
	int nfrags = skb_shinfo(skb)->nr_frags;
	struct tnk_ttx_dma_desc *desc = NULL;
	unsigned int nopaged_len;
	unsigned long flags;
	unsigned long flags_tx;

	TNK_DBG("%s cindex is %d range is [%d..%d]\n",
		__func__, cindex, TNK_TTX_CINDEX_START, tnk_max_connections);

	TNK_CINDEX_VALIDATE(cindex);

	TNK_DBG("%s skb is %p len is %d\n", __func__, skb, skb->len);
	if (unlikely(!skb || !skb->len)) {
		pr_err("%s: Invalid/empty skb\n", __func__);
		return -EINVAL;
	}

	nopaged_len = skb_headlen(skb);
	TNK_DBG("%s nopaged len is %u\n", __func__, nopaged_len);

	entries_needed = nfrags + ((nopaged_len > 0) ? 1 : 0);

	spin_lock_irqsave(&t->tx_desc_lock, flags_tx);

	/* get t->head in spin_lock(tx_desc_lock) to make sure safe */
	entry = t->head % txsize;
	last = entry;

#if SWITCH_SEND_FIN
	if (TCP_SKB_CB(skb)->tcp_flags & TCPHDR_FIN) {
		WARN_ON((nopaged_len != 1) || nfrags);

		if (t->head != t->tail) {
			struct tnkhw_connection cur_conn;
			unsigned int last_seq;

			last_seq = t->tx_snd_nxt - t->last_data_size;
			tnkhw_connection_get_entry(cindex, &cur_conn,
					CT_NEXT_TX_SEQ_NUM, CT_NEXT_TX_SEQ_NUM);
			if (!after(cur_conn.next_tx_seq_num, last_seq)) {
				tnk_conn_stats[cindex].tx_fin_failed++;
				spin_unlock_irqrestore(&t->tx_desc_lock,
							flags_tx);
				return -EAGAIN;
			}
		}
	}
#endif
	/*  Ensure that we have enough descriptors to send this packet */
	if (unlikely(tnkhw_tx_avail_desc(cindex) < entries_needed)) {
		int free;
		/* Set the overflow flag to indicate that queue full condition
		 * occurred */
		t->overflow = 1;
		/* NOTE - We need to prevent against possible deadlock
		 * race-condition, where ISR may come in and reclaim ALL TX
		 * buffers BEFORE we get to set this flag, meaning the flag
		 * would never get cleared?
		 */
		free = tnkhw_tx_avail_desc(cindex);
		if (likely(free < entries_needed)) {
			TNK_DBG("%s: Need %d, got %d desc free on channel %d,",
				__func__, entries_needed, free, cindex);
			TNK_DBG(" rejecting packet\n");
			/* Overflow condition still exists, so tell the caller
			 * to try again later */
			tnk_conn_stats[cindex].tx_overflow++;
			spin_unlock_irqrestore(&t->tx_desc_lock, flags_tx);
			return -EAGAIN;
		} else {
			/* Overflow condition has been cleared */
			t->overflow = 0;
		}
	}

	/* Ensure that we have enough free space in the Tx pipeline to send
	 * this packet.  We're currently limited to 16MB due to 24-bit counters
	 * used by the TOE hardware
	 */
	if (unlikely(tnkhw_tx_avail_bytes(cindex) < skb->len)) {
		/* Set the overflow flag to indicate that queue full condition
		 * occurred */
		t->overflow = 1;
		/* NOTE - We need to prevent against possible deadlock
		 * race-condition, where ISR may come in and reclaim ALL
		 * TX buffers BEFORE we get to set this flag, meaning the
		 * flag would never get cleared?
		 */
		if (likely(tnkhw_tx_avail_bytes(cindex) < skb->len)) {
			TNK_DBG("%s: Not enough free space on channel %d,",
					__func__, cindex);

			TNK_DBG(" rejecting packet\n");

			/* Overflow condition still exists, so tell the caller
			 * to try again later */
			tnk_conn_stats[cindex].tx_overflow++;
			spin_unlock_irqrestore(&t->tx_desc_lock, flags_tx);
			return -EAGAIN;
		} else {
			/* Overflow condition has been cleared */
			t->overflow = 0;
		}
	}

	if (nopaged_len) {
		desc = t->desc_list + entry;
		if (desc->hw_own)
			tnkhw_tx_err_dump_info_nolock(cindex);
		/*  TODO lock needed?  Bug still occurs as of 1/9/11 */
		BUG_ON(desc->hw_own);

		t->skbuff[entry] = NULL;
		desc->buffer_ptr = dma_map_single(tnk_dev, skb->data,
						  nopaged_len, DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(tnk_dev, desc->buffer_ptr))) {
			pr_err("%s: DMA Mapping Error\n", __func__);
			spin_unlock_irqrestore(&t->tx_desc_lock, flags_tx);
			return -ENOMEM;
		}
		TNK_DBG("%s send skb %p, phys %x, cindex %u, len %u,",
			__func__, skb->data, desc->buffer_ptr, cindex,
			nopaged_len);
		TNK_DBG("data '%-5.5s...'\n", skb->data);

		desc->buffer_size = nopaged_len;

		desc->ack_offset = 0;
		desc->hw_own = 1;
		total_len += desc->buffer_size;
		entry = (entry + 1) % txsize;
		count++;
	}

	for (i = 0; i < nfrags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		int len = frag->size;

		TNK_DBG("%s queueing frag %d of %d (%d bytes)\n", __func__, i,
			nfrags, len);
		/*  Don't transmit 0-byte frags, for efficiency */
		if (len == 0)
			continue;

		last = entry;
		desc = t->desc_list + entry;
		if (desc->hw_own)
			tnkhw_tx_err_dump_info_nolock(cindex);
		BUG_ON(desc->hw_own);
		t->skbuff[entry] = NULL;
		desc->buffer_ptr = skb_frag_dma_map(tnk_dev, frag, 0,
				len, DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(tnk_dev, desc->buffer_ptr))) {
			pr_err("%s: DMA Mapping Error\n", __func__);
			spin_unlock_irqrestore(&t->tx_desc_lock, flags_tx);
			return -ENOMEM;
		}
		TNK_DBG("%s send frag %d, ptr %p, len %u, data '%-5.5s...'\n",
			__func__, i, addr, len,
			(char *)page_address(frag->page));

		desc->buffer_size = len;

		desc->ack_offset = 0;
		desc->hw_own = 1;
		total_len += desc->buffer_size;
		entry = (entry + 1) % txsize;
		count++;
	}

#ifdef DBG_TX_DMA
	if (count >= tnk_tx_fifo) {
		pr_warn("DEBUG_L:(%s,%d): wrong (count >= tnk_tx_fifo).",
			__func__, __LINE__);
		pr_warn("cindex=%d, ", cindex);
		dump_tx_dma_info(t);
		dump_stack();
	} else if (count == 0) {
		pr_warn("(%s,%d): wired, tnkhw_tx_send do nothing, cindex=%d,",
			__func__, __LINE__, cindex);
		dump_tx_dma_info(t);
	}
#endif

	if (total_len) {
	#ifdef DBG_TX_DMA
		unsigned int prev_t_head = t->head;
	#endif
		TNK_DBG("%s tell hw about %d bytes\n", __func__, total_len);

		/* Save the skb pointer with the last descriptor in the chain so
		 * that the skb will not be freed until the whole chain has been
		 * sent */
		t->skbuff[last] = skb;

		/* Interrupt on completition only for the latest segment */
		desc->intr_on_completion = 1;

		/* Advance head, now that the ownership bit has been set on all
		 * buffers */
		t->head += count;
	#ifdef DBG_TX_DMA
		if (t->head < prev_t_head) {
			pr_warn("(%s,%d): tx_dma_list, head will overflow.",
				__func__, __LINE__);
			pr_warn("cindex=%d, prev t->head=0x%x, ",
				cindex, prev_t_head);
			pr_warn("prev_t_head_index=%d, count=%d\n",
				prev_t_head % txsize, count);
			dump_tx_dma_info(t);
			t->head_overflow = 1;
		}
	#endif

		atomic_add(total_len, &t->pipelined_bytes);

		/* This barrier is important here.  It is required to ensure
		 * the ARM CPU flushes it's DMA write buffers before proceeding
		 * to the next instruction, to ensure that the TOE will see
		 * our descriptor changes in memory */
		STMMAC_SYNC_BARRIER();

#if SWITCH_SEND_FIN
		/* WARNING: we must send ack here!! Because if this channel
		 * nerver send data, tx_reclaim will never be called.
		 */
		if (TCP_SKB_CB(skb)->tcp_flags & TCPHDR_FIN) {
			tnkhw_set_fin_flag(cindex, 1);
			tnkhw_send_ack(cindex);
#ifdef TNK_DBG_SEND_FIN
			atomic_set(&t->fin_sent, 1);
#endif
		} else {
#endif
#ifdef TNK_DBG_SEND_FIN
		if (atomic_read(&t->fin_sent)) {
			pr_err("%s: cid=%d, fin sent, size=%d\n",
				__func__, cindex, total_len);
			dump_stack();
			tnkhw_connection_dump(cindex);
		}
#endif

		/* Let the TOE know that we've added some new TX data */
		spin_lock_irqsave(&tnkhw_reg_lock, flags);
try_again:
		if (readl(t->desc_list + (t->head - 1) % txsize) & TNK_OWN_BIT)
			tnkhw_ttx_data_add(cindex, total_len);
		else
			goto try_again;
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		TNK_DBG("%s bump tx pkt\n", __func__);
#if SWITCH_SEND_FIN
		t->last_data_size = total_len;
		t->tx_snd_nxt += total_len;
		}
#endif

		tnk_conn_stats[cindex].tx_desc_sent += count;
		tnk_conn_stats[cindex].tx_bytes_sent += total_len;
	} else {
		tnk_free_skb(skb, SKB_FROM_SEND);
	}

	spin_unlock_irqrestore(&t->tx_desc_lock, flags_tx);

	TNK_DBG("%s all done\n", __func__);

	return 0;
}

int tnkhw_dma_has_work(void)
{
	uint32_t int_status = readl(tnkhw_ioaddr + TNK_REG_INTR_STAT);
	unsigned int tx_acked_irq;
	int rx_status = 0;

	tx_acked_irq = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_ACK_IRQ);
	tx_acked_irq &= 0xFFFF;

	if (int_status & TNK_INTR_TRX)
		rx_status = dwmac1000_dma_ops.dma_interrupt(tnkhw_ioaddr,
							    DMA_CHANNEL_TOERX,
							    &tnk_stmmac_stats);
	if (likely(rx_status == handle_tx_rx) ||	/*  Rx work to do */
		likely(int_status & TNK_INTR_RX_LRO0) ||
		likely(tx_acked_irq) ||
	    likely(int_status & TNK_INTR_TTX_ACK))	/*  Tx work to do */
		return 1;
	else
		return 0;
}
#if SWITCH_MULTI_INTR
int tnkhw_dma_channel_has_work(unsigned int channel)
{
	uint32_t int_status = readl(tnkhw_ioaddr + TNK_REG_INTR1_STAT +
			(channel - 1) * 0x8);
	unsigned int tx_acked_irq;
	int rx_status = 0;

	tx_acked_irq = readl(tnkhw_ioaddr + TNK_REG_DMA_TTX_ACK_IRQ +
			channel * 0x4);
	tx_acked_irq &= 0xFFFF;

	if (int_status & TNK_INTR_RX_CHANNEL(channel))
		rx_status = dwmac1000_dma_ops.dma_interrupt(tnkhw_ioaddr,
						DMA_CHANNEL_TOERX + channel,
						&tnk_stmmac_stats);
	if (likely(rx_status == handle_tx_rx) ||	/*  Rx work to do */
		likely(int_status & TNK_INTR_RX_LRO(channel)) ||
		likely(tx_acked_irq) ||
	    likely(int_status & TNK_INTR_TX_ACK_CHANNEL(channel))) /* Tx work */
		return 1;
	else
		return 0;
}
#endif

void tnk_hw_proc(struct seq_file *s)
{
	unsigned int rxsize = tnk_rx_fifo;
#if SWITCH_MULTI_INTR
	int i;
	struct tnkhw_rx_dma_info *r;
	unsigned int entry;
	struct tnk_rx_dma_desc *p;
#else
	struct tnkhw_rx_dma_info *r = &tnk_rx_dma;
	unsigned int entry = r->head % rxsize;
	struct tnk_rx_dma_desc *p = r->desc_list + entry;
#endif

#if SWITCH_MULTI_INTR
	for (i = 0; i < TOE_MULTI_INTR_NUM; i++) {
		r = &tnk_rx_dma[i];
		entry = r->head % rxsize;
		p = r->desc_list + entry;
#endif
#if SWITCH_MULTI_INTR
	seq_printf(s, "\n\ntnkhw[%d]:\n", i);
#else
	seq_printf(s, "\n\ntnkhw:\n");
#endif
	seq_printf(s, "rx entry=%d\n", entry);
	seq_printf(s, "rx sw-owns=%d\n", !p->base.des01.erx.own);
	seq_printf(s, "rx free=%d:\n", r->tail + tnk_rx_fifo - r->head);
#if SWITCH_MULTI_INTR
	if (i != 0)
		seq_printf(s, "haswork=%d:\n", tnkhw_dma_channel_has_work(i));
	else
#endif
	seq_printf(s, "haswork=%d:\n", tnkhw_dma_has_work());
#if SWITCH_MULTI_INTR
	}
#endif
	seq_printf(s, "\n\npolls=%d:\n", tnk_stats.polls);
	seq_printf(s, "rx refill new alloc=%d:\n", tnk_stats.rx_refills_alloc);
#if SWITCH_RECV_LRO
	seq_printf(s, "lro alloc pages=%d\n",
			atomic_read(&tnk_stats.rx_lro_pages));
#endif
}

void tnkhw_dma_interrupt(uint32_t source_mask)
{
	int rx_status = 0;

	if (source_mask & TNK_INTR_TRX)
		rx_status = dwmac1000_dma_ops.dma_interrupt(tnkhw_ioaddr,
							    DMA_CHANNEL_TOERX,
							    &tnk_stmmac_stats);

	if (likely(rx_status == handle_tx_rx) ||    /*  Rx work to do */
#if SWITCH_RECV_LRO
	    likely(source_mask & TNK_INTR_RX_LRO0) ||
#endif
	    likely(source_mask & TNK_INTR_TTX_ACK)) /*  Tx work to do */
		tnk_pollwakeup();
}

#if SWITCH_MULTI_INTR
void tnkhw_dma_channel_interrupt(int channel, uint32_t source_mask)
{
	int rx_status = 0;
	int dma_channel = DMA_CHANNEL_TOERX + channel;

	if (source_mask & TNK_INTR_RX_CHANNEL(channel))
		rx_status = dwmac1000_dma_ops.dma_interrupt(tnkhw_ioaddr,
							    dma_channel,
							    &tnk_stmmac_stats);

	if (likely(rx_status == handle_tx_rx) ||    /*  Rx work to do */
#if SWITCH_RECV_LRO
	    likely(source_mask & TNK_INTR_RX_LRO(channel)) ||
#endif
	    likely(source_mask & TNK_INTR_TX_ACK_CHANNEL(channel))) /* Tx */
		tnk_channel_pollwakeup(channel);
}
#endif

static void tnkhw_toe_interrupt(void)
{
	int intr_stat = readl(tnkhw_ioaddr + TNK_REG_TOE_INTR_STAT);
	int intr_tx_err, intr_tx_err_cid;

	tnk_stats.toe_irq++;

	if ((intr_stat >> TNK_OFFSET_TOE_INTR_STAT_FLUSH) &
	    TNK_MASK_TOE_INTR_STAT_FLUSH) {
		tnk_flush_pending = 0;
		TNK_DBG("%s: Got flush completion interrupt\n", __func__);
	}

	intr_tx_err = (intr_stat >> TNK_OFFSET_TOE_INTR_STAT_TX_ERR) &
			TNK_MASK_TOE_INTR_STAT_TX_ERR;
	intr_tx_err_cid = (intr_stat >> TNK_OFFSET_TOE_INTR_STAT_TX_ERR_CID) &
				TNK_MASK_TOE_INTR_STAT_TX_ERR_CID;
	WARN(intr_tx_err, "tx err cindex=%d\n", intr_tx_err_cid);

	if ((intr_stat >> TNK_OFFSET_TOE_INTR_STAT_RETRY_MAX) &
	    TNK_MASK_TOE_INTR_STAT_RETRY_MAX) {
		unsigned cindex =
		    (intr_stat >> TNK_OFFSET_TOE_INTR_STAT_RETRY_CID) &
		    TNK_MASK_TOE_INTR_STAT_RETRY_CID;

#ifdef TNK_DBG_KEY_POINT
		pr_err("%s: retrans exceed, cid=%d\n", __func__, cindex);
#endif

		tnk_txmaxretries(cindex);
	}

#if SWITCH_ZERO_PROBE
	if ((intr_stat >> TNK_OFFSET_TOE_INTR_STAT_PROBE_MAX) &
		TNK_MASK_TOE_INTR_STAT_PROBE_MAX) {
		unsigned int intr_probe;
		unsigned int probe_cindex;

		intr_probe = readl(tnkhw_ioaddr + TNK_REG_CT_PROBE_INTR);
		probe_cindex = intr_probe & TNK_MASK_TOE_INTR_PROBE_CID;

#ifdef TNK_DBG_KEY_POINT
		pr_err("%s: probe exceed, cid=%d\n", __func__, probe_cindex);
#endif

		if (!TNK_CINDEX_NOT_VALIDATE(probe_cindex))
			tnk_txmaxretries(probe_cindex);
	}
#endif
}

static void tnkhw_toe_flush_completed(void)
{
	int intr_stat = readl(tnkhw_ioaddr + TNK_REG_TOE_INTR_STAT);
	int intr_tx_err, intr_tx_err_cid;

	tnk_stats.toe_irq++;

	if ((intr_stat >> TNK_OFFSET_TOE_INTR_STAT_FLUSH) &
	    TNK_MASK_TOE_INTR_STAT_FLUSH) {
		tnk_flush_pending = 0;
		TNK_DBG("%s: Got flush completion interrupt\n", __func__);
	}

	intr_tx_err = (intr_stat >> TNK_OFFSET_TOE_INTR_STAT_TX_ERR) &
			TNK_MASK_TOE_INTR_STAT_TX_ERR;
	intr_tx_err_cid = (intr_stat >> TNK_OFFSET_TOE_INTR_STAT_TX_ERR_CID) &
				TNK_MASK_TOE_INTR_STAT_TX_ERR_CID;
	WARN(intr_tx_err, "tx err cindex=%d\n", intr_tx_err_cid);

	/* TODO: TNK_REG_TOE_INTR_STAT register is RC(read-clear),
	* we omit TOE_INTR_STAT_RETRY_MAX interrupt because we
	* trust logic will generate this interrupt again and again
	* unless we disable the connection.
	*/
}

void tnkhw_interrupt(int source_mask)
{
#if SWITCH_RECV_LRO
	if (source_mask & (TNK_INTR_TRX | TNK_INTR_TTX_ACK | TNK_INTR_RX_LRO0))
#else
	if (source_mask & (TNK_INTR_TRX | TNK_INTR_TTX_ACK))
#endif
		tnkhw_dma_interrupt(source_mask);

	if (source_mask & TNK_INTR_TTX_ERR)
		tnkhw_tx_err();

	if (source_mask & TNK_INTR_TOE)
		tnkhw_toe_interrupt();
}

#if SWITCH_MULTI_INTR
void tnkhw_channel_interrupt(int channel, unsigned int source_mask)
{
#if SWITCH_RECV_LRO
	if (source_mask & (TNK_INTR_RX_CHANNEL(channel) |
				TNK_INTR_RX_LRO(channel) |
				TNK_INTR_TX_ACK_CHANNEL(channel)))
#else
	if (source_mask & (TNK_INTR_RX_CHANNEL(channel) |
				TNK_INTR_TX_ACK_CHANNEL(channel)))
#endif
		tnkhw_dma_channel_interrupt(channel, source_mask);
}
#endif

static void tnkhw_reset(void)
{
	int cindex, i, lookup_entry_size;
	unsigned toe_int_en;
#ifndef TNK_HW_PLATFORM_FPGA
	unsigned long tnkclk;
#endif

	/*  Clear the lookup table */
	lookup_entry_size = (tnk_max_connections <= 256) ? 1 : 2;
	for (i = 0;
	     i <
	     (tnk_max_connections * lookup_entry_size * 2) / sizeof(uint32_t);
	     i++) {
		uint32_t wr_addr = 0x100000 + i;
		tnkhw_ct_write_addr(wr_addr, 0);
	}

	/*  Clear out the connection table */
	for (cindex = 0; cindex < tnk_max_connections; cindex++) {
		for (i = 0; i < sizeof(struct tnkhw_connection)
			/ sizeof(uint32_t); i++)
			tnkhw_ct_write(cindex, i, 0);

		tnkhw_ttx_db_init(cindex, 0);
		tnkhw_ttx_txstat_clear(cindex);
		tnkhw_ttx_rxstat_clear(cindex);
	}

	tnkhw_toe_config();

#if SWITCH_RECV_LRO
	tnkhw_set_rx_lro_timer();
#endif
	/*  Configure AXI burst len (using power-on default for now) */
	/*  tnkhw_ttx_ctrl_set(0x80); */

	/*  Configure the default Rx buffer size */
	tnkhw_trx_ctrl_set(DMA_BUFFER_SIZE);

	/*  Configure the TOE with a scaling factor for advertised rx window */
	/*  TODO - what is a suitable value? */
	writel(16, tnkhw_ioaddr + TNK_REG_TOE_CTRL0);

#if 1
	/*  Configure the TOE for immediate ACKs */
	writel(0x8000, tnkhw_ioaddr + TNK_REG_TOE_CTRL1);
#endif

	/*  For FPGA platform, the TOE system clock is 24MHz */
#ifdef TNK_HW_PLATFORM_FPGA
	writel(0x05DBF0FF, tnkhw_ioaddr + TNK_REG_TOE_CTRL2);
#else

	tnkclk = tnk_clk_init();
	writel(tnkclk, tnkhw_ioaddr + TNK_REG_TOE_CTRL2);

	/*  For ASIC platform, the TOE system clock is 155MHz
	writel(0x25D780FF, tnkhw_ioaddr + TNK_REG_TOE_CTRL2);
	*/
#endif
	tnkhw_config_tcp_retries2(sysctl_tcp_retries2);
#if SWITCH_DUPACK_NUM
	tnkhw_config_tcp_dupack_cnt(sysctl_tcp_reordering);
#endif
	/* TOE don't support tcp_early_retrans feature */
	sysctl_tcp_early_retrans = 0;

	/*  Enable retry_exceeded_max and flush completion TOE interrupts */
	toe_int_en =
	    (TNK_MASK_TOE_INTR_EN_RETRY_MAX <<
	     TNK_OFFSET_TOE_INTR_EN_RETRY_MAX);

	writel(toe_int_en, tnkhw_ioaddr + TNK_REG_TOE_INTR_EN);
}

inline void tnkhw_config_tcp_retries2(int tcp_retries2)
{
	int toe_ctrl2;

	if (tcp_retries2 <= 0)
		tcp_retries2 = 1;
	else if (tcp_retries2 > TNK_MAX_RETRANS)
		tcp_retries2 = TNK_MAX_RETRANS;

	toe_ctrl2 = readl(tnkhw_ioaddr + TNK_REG_TOE_CTRL2);
	toe_ctrl2 = (toe_ctrl2 & ~TNK_MASK_TCP_RETRIES2) | tcp_retries2;
	writel(toe_ctrl2, tnkhw_ioaddr + TNK_REG_TOE_CTRL2);
}

#if SWITCH_DUPACK_NUM
inline void tnkhw_config_tcp_dupack_cnt(int tcp_reordering)
{
	int toe_reordering;

	if (tcp_reordering <= 0)
		tcp_reordering = 1;
	else if (tcp_reordering > TNK_MAX_DUPACK_CNT)
		tcp_reordering = TNK_MAX_DUPACK_CNT;

	toe_reordering = readl(tnkhw_ioaddr + TNK_REG_TOE_DUP_ACK_CNT);
	toe_reordering = (toe_reordering & ~TNK_MASK_TCP_DUPACK_CNT) |
				(tcp_reordering & TNK_MASK_TCP_DUPACK_CNT);
	writel(toe_reordering, tnkhw_ioaddr + TNK_REG_TOE_DUP_ACK_CNT);
}
#endif

#define TNK_THIN_LINEAR_TIMEOUT_BIT \
	(TNK_MASK_THIN_LINEAR_TIMEOUTS << TNK_OFFSET_THIN_LINEAR_TIMEOUTS)
void tnk_set_thin_linear_timeouts(int thin_linear_timeouts)
{
#if SWITCH_THIN_STREAM
	unsigned int toe_cfg0;
	thin_linear_timeouts = !!thin_linear_timeouts;

	toe_cfg0 = readl(tnkhw_ioaddr + TNK_REG_TOE_CONFIG0);
	toe_cfg0 &= ~TNK_THIN_LINEAR_TIMEOUT_BIT;
	toe_cfg0 |= thin_linear_timeouts << TNK_OFFSET_THIN_LINEAR_TIMEOUTS;
	writel(toe_cfg0, tnkhw_ioaddr + TNK_REG_TOE_CONFIG0);
#endif
}

int tnkhw_init(void __iomem *ioaddr, unsigned int max_connections,
	       struct device *dev)
{
	int ret = 0;

	TNK_DBG("Initialising TNK driver...\n");

	/* Control Path initialisation */
	tnkhw_ioaddr = ioaddr;
	tnk_max_connections = max_connections;
	tnk_dev = dev;

	spin_lock_init(&tnkhw_reg_lock);
	spin_lock_init(&tnkhw_reg_ack_cmd_lock);

	tnkhw_reset();

	tnk_conn_stats = kzalloc(max_connections *
				 sizeof(struct tnkhw_conn_stats),
				 GFP_KERNEL);
	if (!tnk_conn_stats) {
		pr_err("%s: Failed to allocate memory for connection stats\n",
		       __func__);
		return -ENOMEM;
	}

	/* Configure DMA channels, etc */
	ret = tnkhw_rx_dma_init();
	if (ret) {
		pr_err("%s: Failed to set initialise Rx DMA channel\n",
		       __func__);
		goto out_freemem;
	}
	TNK_DBG("%s: tnkhw_rx_dma_init returned ok\n", __func__);
	ret = tnkhw_tx_dma_init();
	if (ret) {
		pr_err("%s: Failed to set initialise Tx DMA channel\n",
		       __func__);
		goto out_freerxdma;
	}
	TNK_DBG("%s: tnkhw_tx_dma_init returned ok\n", __func__);

#if SWITCH_RECV_LRO
	ret = tnkhw_rx_dma_channel_init();
	if (ret) {
		pr_err("%s: Failed to set initialise Rx LRO DMA channel\n",
		       __func__);
		goto out_freerxdma;
	}
#endif

	/* init all switch on TOE*/
	tnk_switch_enable();

	/*  If we get here, we had no errors */
	return 0;

out_freerxdma:
	/* tnkhw_rx_dma_free(); */
out_freemem:
	/* kfree(tnk_conn_stats); */

	return ret;
}

void tnkhw_shutdown(void)
{
#if SWITCH_MULTI_INTR
	int i;
	for (i = 1; i < TOE_MULTI_INTR_NUM; i++)
		tnkhw_dma_channel_interrupt_disable(i);
#endif
	tnkhw_dma_interrupt_disable();

	tnkhw_rx_dma_free();
	tnkhw_tx_dma_free();
#if SWITCH_RECV_LRO
	tnkhw_rx_dma_channel_free();
#endif

	kfree(tnk_conn_stats);
	tnk_conn_stats = NULL;
}
#ifdef TNK_BONDING
void sync_ipv4_pos_to_logic(uint32_t addr)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPADDR0,
			TNK_REG_CT_LOCAL_IPADDR0_1,
			TNK_REG_CT_LOCAL_IPADDR0_2,
			TNK_REG_CT_LOCAL_IPADDR0_3},
		{TNK_REG_CT_LOCAL_IPADDR1,
			TNK_REG_CT_LOCAL_IPADDR1_1,
			TNK_REG_CT_LOCAL_IPADDR1_2,
			TNK_REG_CT_LOCAL_IPADDR1_3} };
	int i, j;
	uint32_t addrtmp = 0;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	/* eth0 local ip position */
	for (i = 0; i < 4; i++) {
		addrtmp = readl(tnkhw_ioaddr + reg_offset[0][i]);
		if (addrtmp == addr)
			break;
	}
	/* eth1 local ip position */
	for (j = 0; j < 4; j++) {
		addrtmp = readl(tnkhw_ioaddr + reg_offset[1][j]);
		if (addrtmp == addr)
			break;
	}
	/* make eth1 and eth0 same ip same position */
	if ((i < 4) && (j < 4) && (i != j)) {
		TNKBD_DBG("eth0 local ip pos = %d, eth1 pos = %d\n",
				i, j);
		addrtmp = readl(tnkhw_ioaddr + reg_offset[1][i]);
		writel(readl(tnkhw_ioaddr + reg_offset[1][j]),
				tnkhw_ioaddr + reg_offset[1][i]);
		writel(addrtmp, tnkhw_ioaddr + reg_offset[1][j]);
	}
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
void sync_ipv6_pos_to_logic(uint32_t *ipv6_addr)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPV6_ADDR0_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_3},
		{TNK_REG_CT_LOCAL_IPV6_ADDR1_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_3} };
	int i, j, eth0_pos, eth1_pos;
	uint32_t addrtmp = 0;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	/* eth0 local ip position */
	/* input ipv6 addr compare with the logic ipv6 addr */
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			addrtmp = readl(tnkhw_ioaddr
					+ reg_offset[0][i] + (j * 4));
			if (addrtmp != ipv6_addr[j])
				break;
		}
		if (j == 4)
			break;
	}
	eth0_pos = i;
	/* eth1 local ip position */
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			addrtmp = readl(tnkhw_ioaddr
					+ reg_offset[1][i] + (j * 4));
			if (addrtmp != ipv6_addr[j])
				break;
		}
		if (j == 4)
			break;
	}
	eth1_pos = j;
	/* make eth1 and eth0 same ip same position */
	if ((eth0_pos < 4) && (eth1_pos < 4)
			&& (eth0_pos != eth1_pos)) {
		TNKBD_DBG("eth0 local ip pos = %d, eth1 pos = %d\n",
				eth0_pos, eth1_pos);
		for (i = 0; i < 4; i++) {
			addrtmp = readl(tnkhw_ioaddr
					+ reg_offset[1][eth0_pos] + (i * 4));
			writel(readl(tnkhw_ioaddr
					+ reg_offset[1][eth1_pos] + (i * 4)),
					tnkhw_ioaddr
					+ reg_offset[1][eth0_pos] + (i * 4));
			writel(addrtmp, tnkhw_ioaddr
					+ reg_offset[1][eth1_pos] + (i * 4));
		}
	}
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
uint32_t select_ipv4_pos_to_logic(int gmac, uint32_t addr)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPADDR0,
			TNK_REG_CT_LOCAL_IPADDR0_1,
			TNK_REG_CT_LOCAL_IPADDR0_2,
			TNK_REG_CT_LOCAL_IPADDR0_3},
		{TNK_REG_CT_LOCAL_IPADDR1,
			TNK_REG_CT_LOCAL_IPADDR1_1,
			TNK_REG_CT_LOCAL_IPADDR1_2,
			TNK_REG_CT_LOCAL_IPADDR1_3} };
	int i;
	uint32_t addrtmp = 0;

	for (i = 0; i < 4; i++) {
		addrtmp = readl(tnkhw_ioaddr + reg_offset[gmac][i]);
		if (addrtmp == addr)
			break;
	}
	if (i == 4) {
		TNKBD_DBG(KERN_ERR"TOE only support 4 IPaddrs on one dev\n");
		TNKBD_DBG(KERN_ERR"This ip is not compare with those\n");
		return -1;
	}
	return i;
}
uint32_t select_ipv6_pos_to_logic(int gmac, uint32_t *ipv6_addr)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPV6_ADDR0_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_3},
		{TNK_REG_CT_LOCAL_IPV6_ADDR1_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_3} };
	int i, j;
	uint32_t addrtmp = 0;

	/* input ipv6 addr compare with the logic ipv6 addr */
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			addrtmp = readl(tnkhw_ioaddr
					+ reg_offset[gmac][i] + (j * 4));
			if (addrtmp != ipv6_addr[j])
				break;
		}
		if (j == 4)
			break;
	}
	if (i == 4) {
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This ip is not compare with those\n");
		return -1;
	}
	return i;
}
void tnk_ct_ip_position_sel_extend(int index, int local_ip_pos)
{
#if 0
	uint32_t reg;
	uint32_t bits;
	uint32_t tmp;
	uint32_t postion;
	/* 1 Select reg for TOE connection */
	reg = (index / 8) * 4 + TNK_REG_CT_LOCAL_IPADDR_INDEX_SEL;

	/* 2 Select reg bits for TOE connection */
	bits = (0x00000003 & local_ip_pos) << ((index % 8) * 4);

	/* 3 Read reg and clear the two bits */
	postion = (0x00000003) << ((index % 8) * 4);
	tmp = readl(tnkhw_ioaddr + reg);
	tmp = (~postion) & tmp;

	/* 4 Load the Local IP address position for TOE connection */
	writel(bits | tmp, tnkhw_ioaddr + reg);
	TNKBD_DBG("%s  bits = %.8x\n",
			__func__, readl(tnkhw_ioaddr + reg));
#endif
	uint32_t wr_val;

	wr_val = (local_ip_pos & TNK_MASK_BONJOUR_SEL_WRDATA) |
		(1 << TNK_OFFSET_BONJOUR_WR) |
		((index & TNK_MASK_BONJOUR_CID) << TNK_OFFSET_BONJOUR_CID);

	writel(wr_val, tnkhw_ioaddr + TNK_REG_CT_LOCAL_IPADDR_INDEX_SEL);
}
#endif

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
int tnkhw_connection_add(unsigned cindex, struct tnkhw_connection *entry,
		int local_ip_pos)
#else
int tnkhw_connection_add(unsigned cindex, struct tnkhw_connection *entry)
#endif
{
	int i;
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
	uint32_t *data = (uint32_t *) entry;
	unsigned long flags;

	TNK_CINDEX_VALIDATE(cindex);

	TNK_DBG("%s db init and stat clear for t=%p\n", __func__, t);

#ifdef TNK_TTX_FLUSH_DELAYED_RECLAIM
	if (t->head != t->tail) {
		/* Clear out the old buffers as well */
		tnkhw_tx_channel_reclaim(cindex, 1);
	}
#endif

	if (t->head != t->tail)
		tnkhw_tx_err_dump_info_nolock(cindex);

	tnkhw_tx_desc_check(cindex);

	/* Re-initialise the channel variables */
	tnkhw_ttx_db_init(cindex, t->desc_list_phys);
	tnkhw_ttx_txstat_clear(cindex);
	tnkhw_ttx_rxstat_clear(cindex);
	memset(&tnk_conn_stats[cindex], 0, sizeof(tnk_conn_stats[0]));
	t->head = 0;
	t->tail = 0;
	atomic_set(&t->pipelined_bytes, 0);
#ifdef TNK_DBG_SEND_FIN
	atomic_set(&t->fin_sent, 0);
#endif
#if SWITCH_SEND_FIN
	t->tx_snd_nxt = entry->next_tx_seq_num;
	t->last_data_size = 0;
#endif
	t->rst_received = 0;
	t->overflow = 0;
#ifdef DBG_TX_DMA
	t->head_overflow = 0;
#endif

	TNK_DBG("%s writing conn table for cindex %d\n", __func__, cindex);

	tnkhw_init_out_of_order(cindex);
	spin_lock_irqsave(&tnkhw_reg_lock, flags);

#if SWITCH_RECV_LRO
	tnkhw_rx_db_write(cindex, TNK_RX_DB_RCV_NXT, entry->next_rx_seq_num);
	tnkhw_lro_init_rcv_nxt(cindex, entry->next_rx_seq_num);
	tnkhw_check_rx_timer_valid(cindex);
#if 0
	tnkhw_rx_timer_reset(cindex);
#endif
#endif
	/*  Write the complete connection table entry, one word at a time */
	for (i = 0; i < sizeof(struct tnkhw_connection) / sizeof(uint32_t); i++)
		tnkhw_ct_write(cindex, i, data[i]);

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
	/* One TOE connection to a 2 bits ip position */
	if (local_ip_pos >= 0) {
		/* for tx like ct table */
		tnk_ct_ip_position_sel_extend(cindex, local_ip_pos);
	}

	/*  Add this connection to the lookup table */
	if (entry->flag_ipv6)
		ct_lookup_data_load_ipv6(entry->r_ipv6_addr_31_0,
					 entry->r_ipv6_addr_63_32,
					 entry->r_ipv6_addr_95_64,
					 entry->r_ipv6_addr_127_96,
					 entry->r_port,
					 entry->l_port,
					 local_ip_pos);
	else
		ct_lookup_data_load_ipv4(entry->r_ipaddr, entry->r_port,
					 entry->l_port, local_ip_pos);
#else
	/*  Add this connection to the lookup table */
	if (entry->flag_ipv6)
		ct_lookup_data_load_ipv6(entry->r_ipv6_addr_31_0,
					 entry->r_ipv6_addr_63_32,
					 entry->r_ipv6_addr_95_64,
					 entry->r_ipv6_addr_127_96,
					 entry->r_port, entry->l_port);
	else
		ct_lookup_data_load_ipv4(entry->r_ipaddr, entry->r_port,
					 entry->l_port);
#endif
	/*  Commit the CT update by writing CT_CONN_CTRL, and wait for
	 *  operation to complete */
	tnkhw_ct_update(cindex, 1, entry->flag_ipv6);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}
#ifdef TNK_BONDING
int is_bonding_enable(void)
{
	uint32_t data;

	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	if (data & 0x00000080) {
		TNKBD_DBG("%s Bonding disable\n", __func__);
		return 0;
	} else {
		TNKBD_DBG("%s Bonding enable\n", __func__);
		return 1;
	}
}
void tnkhw_bonding_enable(int enable)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	if (enable) {
		writel((0xffffff7f & data),
				tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
		TNKBD_DBG("%s Bonding enable\n", __func__);
	} else {
		writel((0x00000080 | data),
				tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
		TNKBD_DBG("%s Bonding disable\n", __func__);
	}
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
EXPORT_SYMBOL(tnkhw_bonding_enable);
void tnkhw_bonding_setmode(int mode)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	if (mode) {
		TNKBD_DBG("%s BOND_MODE_ACTIVEBACKUP\n", __func__);
		writel((0x00000040 | data),
				tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	} else {
		TNKBD_DBG("%s BOND_MODE_ROUNDROBIN\n", __func__);
		writel((0xffffffbf & data),
				tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	}
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
EXPORT_SYMBOL(tnkhw_bonding_setmode);
int tnkhw_bonding_getmode(void)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
	if (data & 0x00000040) {
		TNKBD_DBG("%s BOND_MODE_ACTIVEBACKUP\n", __func__);
		return 1;
	} else {
		TNKBD_DBG("%s BOND_MODE_ROUNDROBIN\n", __func__);
		return 0;
	}
}
int curr_bonding_dev(void)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
	if (data & 0x00000004) {
		TNKBD_DBG("%s curr_slave = eth1\n", __func__);
		return 1;
	} else {
		TNKBD_DBG("%s curr_slave = eth0\n", __func__);
		return 0;
	}
}
int curr_active_dev(void)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
	if (0x2 == (data & 0x00000003)) {
		TNKBD_DBG("%s Only eth1 is active now\n", __func__);
		return 1;
	} else if (0x1 == (data & 0x00000003)) {
		TNKBD_DBG("%s Only eth0 is active now\n", __func__);
		return 0;
	} else {
		TNKBD_DBG("%s eth0 and eth1 are active\n", __func__);
		return 2;
	}
}
void tnkhw_bonding_setcurr_active_slave(int curr_slave)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	if (curr_slave) {
		TNKBD_DBG("%s curr_slave = eth1\n", __func__);
		writel((0x00000004 | data),
				tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	} else {
		TNKBD_DBG("%s curr_slave = eth0\n", __func__);
		writel((0xfffffffb & data),
				tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	}
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
EXPORT_SYMBOL(tnkhw_bonding_setcurr_active_slave);
void tnkhw_bonding_setstatus_slave(int gmac, int enable)
{
	uint32_t data;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	data = readl(tnkhw_ioaddr + TNK_REG_TOE_BONDING_CTRL);
	if (gmac == 0) {
		if (enable)
			writel((0x00000001 | data), tnkhw_ioaddr
					+ TNK_REG_TOE_BONDING_CTRL);
		else
			writel((0xfffffffe & data), tnkhw_ioaddr
					+ TNK_REG_TOE_BONDING_CTRL);
	} else {
		if (enable)
			writel((0x00000002 | data), tnkhw_ioaddr
					+ TNK_REG_TOE_BONDING_CTRL);
		else
			writel((0xfffffffd & data), tnkhw_ioaddr
					+ TNK_REG_TOE_BONDING_CTRL);
	}
	TNKBD_DBG("%s gmac = %d, enable = %d\n",
			__func__, gmac, enable);
	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
EXPORT_SYMBOL(tnkhw_bonding_setstatus_slave);
#endif
static void tnkhw_dump_tx_descriptors(unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
	int i;
	unsigned long flags;

	spin_lock_irqsave(&t->tx_desc_lock, flags);
	pr_info("%s cindex %d, head %d, tail %d\n",
		__func__, cindex, t->head, t->tail);

	for (i = 0; i < tnk_tx_fifo; i++) {
		struct tnk_ttx_dma_desc *desc = &t->desc_list[i];
		pr_info("TTX Descriptor (chan %u, ring index %u)\n"
			"\tack_offset: %u\n"
			"\treserved1: %u\n"
			"\thw_own: %u\n"
			"\tbuffer_size: %u\n"
			"\treserved2: %u\n"
			"\tintr_on_completion: %u\n"
			"\tbuffer_ptr: 0x%08X\n"
			"\tnext_desc_ptr: 0x%08X\n",
			cindex, i,
			desc->ack_offset,
			desc->reserved1,
			desc->hw_own,
			desc->buffer_size,
			desc->reserved2,
			desc->intr_on_completion,
			desc->buffer_ptr, desc->next_desc_ptr);
	}
	spin_unlock_irqrestore(&t->tx_desc_lock, flags);
}
static void tnkhw_dump_tx_descriptors_nolock(unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
	int i;

	pr_info("%s cindex %d, head %d, tail %d\n",
		__func__, cindex, t->head, t->tail);

	for (i = 0; i < tnk_tx_fifo; i++) {
		struct tnk_ttx_dma_desc *desc = &t->desc_list[i];
		pr_info("TTX Descriptor (chan %u, ring index %u)\n"
			"\tack_offset: %u\n"
			"\treserved1: %u\n"
			"\thw_own: %u\n"
			"\tbuffer_size: %u\n"
			"\treserved2: %u\n"
			"\tintr_on_completion: %u\n"
			"\tbuffer_ptr: 0x%08X\n"
			"\tnext_desc_ptr: 0x%08X\n",
			cindex, i,
			desc->ack_offset,
			desc->reserved1,
			desc->hw_own,
			desc->buffer_size,
			desc->reserved2,
			desc->intr_on_completion,
			desc->buffer_ptr, desc->next_desc_ptr);
	}
}

void tnkhw_connection_disable(unsigned cindex)
{
	unsigned long flags;

	TNK_CINDEX_VALIDATE(cindex);

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/* Clear the enable bit in the connection table */
	tnkhw_ct_enable(cindex, 0);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

void tnkhw_connection_enable(unsigned cindex)
{
	unsigned long flags;

	TNK_CINDEX_VALIDATE(cindex);

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/* Set the enable bit in the connection table */
	tnkhw_ct_enable(cindex, 1);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
int tnkhw_connection_remove(unsigned cindex, struct tnkhw_connection *entry,
		int local_ip_pos, int ct_hw_removed)
#else
int tnkhw_connection_remove(unsigned cindex, struct tnkhw_connection *entry,
		int ct_hw_removed)
#endif
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];
	unsigned long flags;
#ifdef TNK_DBG_TTX_ERR
	struct tnkhw_connection cur_conn;
#endif

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	TNK_CINDEX_VALIDATE(cindex);

	TNK_DBG("%s ip %x\n", __func__, entry->r_ipaddr);

	if (ct_hw_removed)
		goto tnkhw_ct_removed;

	/* 1. Remove the lookup-table entry.
	 * This stops any ACK processing or receiving packets for this
	 * connection
	 */
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
	if (entry->flag_ipv6)
		ct_lookup_data_load_ipv6(entry->r_ipv6_addr_31_0,
					 entry->r_ipv6_addr_63_32,
					 entry->r_ipv6_addr_95_64,
					 entry->r_ipv6_addr_127_96,
					 entry->r_port,
					 entry->l_port,
					 local_ip_pos);
	else
		ct_lookup_data_load_ipv4(entry->r_ipaddr, entry->r_port,
					 entry->l_port, local_ip_pos);
#else
	if (entry->flag_ipv6)
		ct_lookup_data_load_ipv6(entry->r_ipv6_addr_31_0,
				entry->r_ipv6_addr_63_32,
				entry->r_ipv6_addr_95_64,
				entry->r_ipv6_addr_127_96,
				entry->r_port, entry->l_port);
	else
		ct_lookup_data_load_ipv4(entry->r_ipaddr, entry->r_port,
				entry->l_port);
#endif

	/*  Commit the CT update by writing CT_CONN_CTRL, and wait for
	 *  operation to complete */
	tnkhw_ct_update(cindex, 0, entry->flag_ipv6);

	/* 2. Clear the enable bit in the connection table
	 * This stops any further new Tx processing, including reading
	 * descriptors, although the TOE may be in the process of
	 * completing one.
	 *
	 * It is not guaranteed that the flush will complete immediately,
	 * so we will wait until we go to re-use the channel or shut-down
	 * before reclaiming the buffers that may be sitting in the ring
	 */
	tnkhw_ct_enable(cindex, 0);

tnkhw_ct_removed:
	/* 3. Send a flush command for that connection
	 * This will clear out any accumulated count for that connection
	 */
	if (t->head != t->tail) {
#if !SWITCH_REMOVE_TTX_ERR
		mdelay(1);
#endif
		tnkhw_ttx_data_flush(cindex);
#if !SWITCH_REMOVE_TTX_ERR
		tnkhw_ttx_wait_flush();
#endif
#ifndef TNK_TTX_FLUSH_DELAYED_RECLAIM
		/* Clear out the old buffers as well */
		tnkhw_tx_channel_reclaim(cindex, 1);
#endif
	}

#ifdef TNK_DBG_CONN
	pr_err("(%s,%d) cid=%d, sport=%d, dport=%d, "
			"snd_nxt=%x, snd_una=%x, rcv_nxt=%x\n",
			__func__, __LINE__, cindex,
			entry->l_port, entry->r_port,
			entry->next_tx_seq_num, entry->rx_ack_num,
			entry->next_rx_seq_num);
#endif

#ifdef DBG_TX_DMA
	if (t->head_overflow) {
		pr_warn("DEBUG_L:(%s,%d): t->head_overflow non-zero.",
			__func__, __LINE__);
		pr_warn("cindex=%d, ", cindex);
		dump_tx_dma_info(t);
		dump_stack();
	}
#endif

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

#ifdef TNK_DBG_TTX_ERR
	tnkhw_connection_get(cindex, &cur_conn);
	memcpy((char *)(&last_conn[cindex]), (char *)(&cur_conn),
		sizeof(struct tnkhw_connection));
#endif
	return 0;
}

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
int tnkhw_connection_remove_hw(unsigned cindex, struct tnkhw_connection *entry,
		int local_ip_pos)
#else
int tnkhw_connection_remove_hw(unsigned cindex, struct tnkhw_connection *entry)
#endif
{
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	TNK_CINDEX_VALIDATE(cindex);

	TNK_DBG("%s ip %x\n", __func__, entry->r_ipaddr);

	/* 1. Remove the lookup-table entry.
	 * This stops any ACK processing or receiving packets for this
	 * connection
	 */
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
	if (entry->flag_ipv6)
		ct_lookup_data_load_ipv6(entry->r_ipv6_addr_31_0,
					 entry->r_ipv6_addr_63_32,
					 entry->r_ipv6_addr_95_64,
					 entry->r_ipv6_addr_127_96,
					 entry->r_port,
					 entry->l_port,
					 local_ip_pos);
	else
		ct_lookup_data_load_ipv4(entry->r_ipaddr, entry->r_port,
					 entry->l_port, local_ip_pos);
#else
	if (entry->flag_ipv6)
		ct_lookup_data_load_ipv6(entry->r_ipv6_addr_31_0,
				entry->r_ipv6_addr_63_32,
				entry->r_ipv6_addr_95_64,
				entry->r_ipv6_addr_127_96,
				entry->r_port, entry->l_port);
	else
		ct_lookup_data_load_ipv4(entry->r_ipaddr, entry->r_port,
				entry->l_port);
#endif

	/*  Commit the CT update by writing CT_CONN_CTRL, and wait for
	 *  operation to complete */
	tnkhw_ct_update(cindex, 0, entry->flag_ipv6);

	/* 2. Clear the enable bit in the connection table
	 * This stops any further new Tx processing, including reading
	 * descriptors, although the TOE may be in the process of
	 * completing one.
	 *
	 * It is not guaranteed that the flush will complete immediately,
	 * so we will wait until we go to re-use the channel or shut-down
	 * before reclaiming the buffers that may be sitting in the ring
	 */
	tnkhw_ct_enable(cindex, 0);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}

int tnkhw_connection_get(unsigned cindex, struct tnkhw_connection *entry)
{
	int i;
	uint32_t *data = (uint32_t *) entry;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	TNK_CINDEX_VALIDATE(cindex);

	/*  Write the complete connection table entry, one word at a time */
	for (i = 0; i < sizeof(struct tnkhw_connection) / sizeof(uint32_t); i++)
		tnkhw_ct_read(cindex, i, &data[i]);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}

#define MAX_CONNECTION_ENTRY_COUNT	32
int tnkhw_connection_get_entry(unsigned cindex, struct tnkhw_connection *entry,
				unsigned int start, unsigned int end)
{
	int i;
	uint32_t *data = (uint32_t *) entry;
	unsigned long flags;

	WARN_ON(end >= MAX_CONNECTION_ENTRY_COUNT || end < start);

	TNK_CINDEX_VALIDATE(cindex);

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/* get connection table entry, one word at a time */
	for (i = start; i < end + 1; i++)
		tnkhw_ct_read(cindex, i, &data[i]);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}

#ifdef TNK_DBG_TTX_ERR
void tnkhw_connection_print(struct tnkhw_connection *conn)
{
	pr_info("r_macaddr_u16       0x%04x\n", conn->r_macaddr_u16);
	pr_info("l_port              %u\n", conn->l_port);
	pr_info("r_macaddr_l32       0x%08x\n", conn->r_macaddr_l32);
	pr_info("r_ipaddr            0x%08x\n", conn->r_ipaddr);
	pr_info("r_port              %u\n", conn->r_port);
	pr_info("ttl                 %u\n", conn->ttl);
	pr_info("tos                 %u\n", conn->tos);
	pr_info("word4_flag: %u\n", conn->word4_flag);
	pr_info("word4_count: %u\n", conn->word4_count);
	pr_info("word5_recoverseqnum: 0x%08x\n",
			conn->word5_recoverseqnum);
	pr_info("rx_ack_num          0x%08x\n", conn->rx_ack_num);
	pr_info("next_rx_seq_num     0x%08x\n", conn->next_rx_seq_num);
	pr_info("next_tx_seq_num     0x%08x\n", conn->next_tx_seq_num);
	pr_info("dup_ack_cnt         %u\n", conn->dup_ack_cnt);
	pr_info("rx_adv_wnd          %u\n", conn->rx_adv_wnd);
	pr_info("timer_tx_retry      %u\n", conn->timer_tx_retry);
	pr_info("retry_count         %u\n", conn->retry_count);
#if SWITCH_ZERO_PROBE
	pr_info("persist_cnt_check: %u\n", conn->persist_cnt_check);
	pr_info("persist_cnt: %u\n", conn->persist_cnt);
	pr_info("probe_cnt: %u\n", conn->probe_cnt);
	pr_info("probe_intr: %u\n", conn->probe_intr);
#endif
	pr_info("tx_mss              %u\n", conn->tx_mss);
	pr_info("adv_wnd_scale       %u\n", conn->adv_wnd_scale);
	pr_info("flag_conn_enable    %u\n", conn->flag_conn_enable);
	pr_info("flag_mac_sel        %u\n", conn->flag_mac_sel);
	pr_info("flag_ipv6           %u\n", conn->flag_ipv6);
	pr_info("flag_disable_nagle  %u\n", conn->flag_disable_nagle);
#if SWITCH_NAGLE
	pr_info("nagle_seq_num: 0x%08x\n", conn->nagle_seq_num);
#endif
#if SWITCH_THIN_STREAM
	pr_info("flag_thin_lto	     %u\n", conn->flag_thin_lto);
#else
	pr_info("flag_use_local_wnd  %u\n", conn->flag_use_local_wnd);
#endif
#if SWITCH_VLAN
	pr_info("flag_add_vlan_tag  %u\n", conn->flag_add_vlan_tag);
#else
	pr_info("flag_use_local_scl  %u\n", conn->flag_use_local_scl);
#endif
	pr_info("flag_full_pkt       %u\n", conn->flag_full_pkt);
#if SWITCH_CORK
	pr_info("flag_cork: %u\n", conn->flag_cork);
	pr_info("cork_timestamp: %u\n", conn->cork_timestamp);
#endif
#if SWITCH_KEEPALIVE
	pr_info("flag_keepalive: %u\n", conn->flag_keepalive);
	pr_info("keepalive acked: %u\n", conn->keepalive_acked);
#endif
#if SWITCH_SEND_FIN
	pr_info("flag_fin: %u\n", conn->flag_fin);
#endif
#if SWITCH_MULTI_INTR
	pr_info("tx_ack_irq: %u\n", conn->tx_ack_irq);
#endif
	pr_info("tx_adv_wnd          %u\n", conn->tx_adv_wnd);
	pr_info("cong_wnd            %u\n", conn->cong_wnd);
	pr_info("timer_timestamp     %u\n", conn->timer_timestamp);
	pr_info("timer_active        %u\n", conn->timer_active);
	pr_info("timer_seq_num       0x%08x\n", conn->timer_seq_num);
	pr_info("timeout_value       %u\n", conn->timeout_value);
	pr_info("sampled_ack_num     0x%08x\n", conn->sampled_ack_num);
	pr_info("timer_rx_fast_retry %u\n", conn->timer_rx_fast_retry);
#if SWITCH_MULTI_INTR_RETRANS
	pr_info("retry_intr: %u\n", conn->retry_intr);
#endif
	pr_info("retry_seq_num       0x%08x\n", conn->retry_seq_num);
	pr_info("en_rtt_calc         %u\n", conn->en_rtt_calc);
	pr_info("ss_threshold        %u\n", conn->ss_threshold);
	pr_info("tx_timestamp        %u\n", conn->tx_timestamp);
	pr_info("timer_rtt           %u\n", conn->timer_rtt);
	pr_info("timer_rtt_valid     %u\n", conn->timer_rtt_valid);
	pr_info("last_retry_seq_num  0x%08x\n", conn->last_retry_seq_num);
	pr_info("ipv6_addr_31_0      %u\n", conn->r_ipv6_addr_31_0);
	pr_info("ipv6_addr_63_32     %u\n", conn->r_ipv6_addr_63_32);
	pr_info("ipv6_addr_95_64     %u\n", conn->r_ipv6_addr_95_64);
	pr_info("ipv6_addr_127_96    %u\n", conn->r_ipv6_addr_127_96);
	pr_info("ipv6_flow_label     %u\n", conn->r_ipv6_flow_label);
	pr_info("word30_updated_tx_ack_num: 0x%08x\n",
			conn->word30_updated_tx_ack_num);
	pr_info("word31_updated_flag: %u\n", conn->word31_updated_flag);
}
#endif

void tnkhw_connection_dump(unsigned cindex)
{
	struct tnkhw_connection conn;

	tnkhw_connection_get(cindex, &conn);

	pr_info("r_macaddr_u16       0x%04x\n", conn.r_macaddr_u16);
	pr_info("l_port              %u\n", conn.l_port);
	pr_info("r_macaddr_l32       0x%08x\n", conn.r_macaddr_l32);
	pr_info("r_ipaddr            0x%08x\n", conn.r_ipaddr);
	pr_info("r_port              %u\n", conn.r_port);
	pr_info("ttl                 %u\n", conn.ttl);
	pr_info("tos                 %u\n", conn.tos);
	pr_info("word4_flag: %u\n", conn.word4_flag);
	pr_info("word4_count: %u\n", conn.word4_count);
	pr_info("word5_recoverseqnum: 0x%08x\n",
			conn.word5_recoverseqnum);
	pr_info("rx_ack_num          0x%08x\n", conn.rx_ack_num);
	pr_info("next_rx_seq_num     0x%08x\n", conn.next_rx_seq_num);
	pr_info("next_tx_seq_num     0x%08x\n", conn.next_tx_seq_num);
	pr_info("dup_ack_cnt         %u\n", conn.dup_ack_cnt);
	pr_info("rx_adv_wnd          %u\n", conn.rx_adv_wnd);
	pr_info("timer_tx_retry      %u\n", conn.timer_tx_retry);
	pr_info("retry_count         %u\n", conn.retry_count);
#if SWITCH_ZERO_PROBE
	pr_info("persist_cnt_check: %u\n", conn.persist_cnt_check);
	pr_info("persist_cnt: %u\n", conn.persist_cnt);
	pr_info("probe_cnt: %u\n", conn.probe_cnt);
	pr_info("probe_intr: %u\n", conn.probe_intr);
#endif
	pr_info("tx_mss              %u\n", conn.tx_mss);
	pr_info("adv_wnd_scale       %u\n", conn.adv_wnd_scale);
	pr_info("flag_conn_enable    %u\n", conn.flag_conn_enable);
	pr_info("flag_mac_sel        %u\n", conn.flag_mac_sel);
	pr_info("flag_ipv6           %u\n", conn.flag_ipv6);
	pr_info("flag_disable_nagle  %u\n", conn.flag_disable_nagle);
#if SWITCH_NAGLE
	pr_info("nagle_seq_num: 0x%08x\n", conn.nagle_seq_num);
#endif
#if SWITCH_THIN_STREAM
	pr_info("flag_thin_lto	     %u\n", conn.flag_thin_lto);
#else
	pr_info("flag_use_local_wnd  %u\n", conn.flag_use_local_wnd);
#endif
#if SWITCH_VLAN
	pr_info("flag_add_vlan_tag  %u\n", conn.flag_add_vlan_tag);
#else
	pr_info("flag_use_local_scl  %u\n", conn.flag_use_local_scl);
#endif
	pr_info("flag_full_pkt       %u\n", conn.flag_full_pkt);
#if SWITCH_CORK
	pr_info("flag_cork: %u\n", conn.flag_cork);
	pr_info("cork_timestamp: %u\n", conn.cork_timestamp);
#endif
#if SWITCH_KEEPALIVE
	pr_info("flag_keepalive: %u\n", conn.flag_keepalive);
	pr_info("keepalive acked: %u\n", conn.keepalive_acked);
#endif
#if SWITCH_SEND_FIN
	pr_info("flag_fin: %u\n", conn.flag_fin);
#endif
#if SWITCH_MULTI_INTR
	pr_info("tx_ack_irq: %u\n", conn.tx_ack_irq);
#endif
	pr_info("tx_adv_wnd          %u\n", conn.tx_adv_wnd);
	pr_info("cong_wnd            %u\n", conn.cong_wnd);
	pr_info("timer_timestamp     %u\n", conn.timer_timestamp);
	pr_info("timer_active        %u\n", conn.timer_active);
	pr_info("timer_seq_num       0x%08x\n", conn.timer_seq_num);
	pr_info("timeout_value       %u\n", conn.timeout_value);
	pr_info("sampled_ack_num     0x%08x\n", conn.sampled_ack_num);
	pr_info("timer_rx_fast_retry %u\n", conn.timer_rx_fast_retry);
#if SWITCH_MULTI_INTR_RETRANS
	pr_info("retry_intr: %u\n", conn.retry_intr);
#endif
	pr_info("retry_seq_num       0x%08x\n", conn.retry_seq_num);
	pr_info("en_rtt_calc         %u\n", conn.en_rtt_calc);
	pr_info("ss_threshold        %u\n", conn.ss_threshold);
	pr_info("tx_timestamp        %u\n", conn.tx_timestamp);
	pr_info("timer_rtt           %u\n", conn.timer_rtt);
	pr_info("timer_rtt_valid     %u\n", conn.timer_rtt_valid);
	pr_info("last_retry_seq_num  0x%08x\n", conn.last_retry_seq_num);
	pr_info("ipv6_addr_31_0      %u\n", conn.r_ipv6_addr_31_0);
	pr_info("ipv6_addr_63_32     %u\n", conn.r_ipv6_addr_63_32);
	pr_info("ipv6_addr_95_64     %u\n", conn.r_ipv6_addr_95_64);
	pr_info("ipv6_addr_127_96    %u\n", conn.r_ipv6_addr_127_96);
	pr_info("ipv6_flow_label     %u\n", conn.r_ipv6_flow_label);
	pr_info("word30_updated_tx_ack_num: 0x%08x\n",
			conn.word30_updated_tx_ack_num);
	pr_info("word31_updated_flag: %u\n", conn.word31_updated_flag);
}

int tnkhw_macaddress_set(uint8_t *mac_addr, int gmac)
{
	uint32_t u16, l32;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	u16 = (mac_addr[0] << 8) | mac_addr[1];
	l32 =
	    (mac_addr[2] << 24) | (mac_addr[3] << 16) | (mac_addr[4] << 8) |
	    mac_addr[5];

	switch (gmac) {
	case 0:
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL_MACADDR0_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL_MACADDR0_L32);
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL0_MACADDR1_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL0_MACADDR1_L32);
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL0_MACADDR2_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL0_MACADDR2_L32);
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL0_MACADDR3_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL0_MACADDR3_L32);
#endif
		break;
	case 1:
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL_MACADDR1_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL_MACADDR1_L32);
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL1_MACADDR1_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL1_MACADDR1_L32);
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL1_MACADDR2_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL1_MACADDR2_L32);
		writel(u16, tnkhw_ioaddr + TNK_REG_CT_LOCAL1_MACADDR3_U16);
		writel(l32, tnkhw_ioaddr + TNK_REG_CT_LOCAL1_MACADDR3_L32);
#endif
		break;
	default:
		pr_err("%s: Invalid gmac %d specified\n", __func__, gmac);
		ret = -EINVAL;
	}

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return ret;
}

void *tnkhw_ioaddr_get(void)
{
	return tnkhw_ioaddr;
}

int tnkhw_check_vlan(int gmac, int pos_id)
{
	u16 vlan_id;

	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_PORT0_VLANTAG0,
			TNK_REG_CT_PORT0_VLANTAG1,
			TNK_REG_CT_PORT0_VLANTAG2,
			TNK_REG_CT_PORT0_VLANTAG3},
		{TNK_REG_CT_PORT1_VLANTAG0,
			TNK_REG_CT_PORT1_VLANTAG1,
			TNK_REG_CT_PORT1_VLANTAG2,
			TNK_REG_CT_PORT1_VLANTAG3} };

	vlan_id = readl(tnkhw_ioaddr + reg_offset[gmac][pos_id]);
	vlan_id &= 0xFFFF;

	return !!vlan_id;
}

void tnkhw_vlan_id_set(u16 vlan_id, int gmac, int pos_id)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_PORT0_VLANTAG0,
			TNK_REG_CT_PORT0_VLANTAG1,
			TNK_REG_CT_PORT0_VLANTAG2,
			TNK_REG_CT_PORT0_VLANTAG3},
		{TNK_REG_CT_PORT1_VLANTAG0,
			TNK_REG_CT_PORT1_VLANTAG1,
			TNK_REG_CT_PORT1_VLANTAG2,
			TNK_REG_CT_PORT1_VLANTAG3} };

	writel(vlan_id, tnkhw_ioaddr + reg_offset[gmac][pos_id]);
}

void tnkhw_mac_addr_set(uint8_t *mac_addr, int gmac, int pos_id)
{
	uint32_t u16 = 0;
	uint32_t l32 = 0;

	uint32_t reg_offset_u16[2][4] = {
		{TNK_REG_CT_LOCAL_MACADDR0_U16,
			TNK_REG_CT_LOCAL0_MACADDR1_U16,
			TNK_REG_CT_LOCAL0_MACADDR2_U16,
			TNK_REG_CT_LOCAL0_MACADDR3_U16},
		{TNK_REG_CT_LOCAL_MACADDR1_U16,
			TNK_REG_CT_LOCAL1_MACADDR1_U16,
			TNK_REG_CT_LOCAL1_MACADDR2_U16,
			TNK_REG_CT_LOCAL1_MACADDR3_U16} };

	uint32_t reg_offset_l32[2][4] = {
		{TNK_REG_CT_LOCAL_MACADDR0_L32,
			TNK_REG_CT_LOCAL0_MACADDR1_L32,
			TNK_REG_CT_LOCAL0_MACADDR2_L32,
			TNK_REG_CT_LOCAL0_MACADDR3_L32},
		{TNK_REG_CT_LOCAL_MACADDR1_L32,
			TNK_REG_CT_LOCAL1_MACADDR1_L32,
			TNK_REG_CT_LOCAL1_MACADDR2_L32,
			TNK_REG_CT_LOCAL1_MACADDR3_L32} };

	if (mac_addr) {
		u16 = (mac_addr[0] << 8) | mac_addr[1];
		l32 = (mac_addr[2] << 24) | (mac_addr[3] << 16) |
			(mac_addr[4] << 8) | mac_addr[5];
	}

	writel(u16, tnkhw_ioaddr + reg_offset_u16[gmac][pos_id]);
	writel(l32, tnkhw_ioaddr + reg_offset_l32[gmac][pos_id]);
}

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
uint32_t tnkhw_ipv4select_reg_write(int gmac, int *pos_id)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPADDR0,
			TNK_REG_CT_LOCAL_IPADDR0_1,
			TNK_REG_CT_LOCAL_IPADDR0_2,
			TNK_REG_CT_LOCAL_IPADDR0_3},
		{TNK_REG_CT_LOCAL_IPADDR1,
			TNK_REG_CT_LOCAL_IPADDR1_1,
			TNK_REG_CT_LOCAL_IPADDR1_2,
			TNK_REG_CT_LOCAL_IPADDR1_3} };
	int i;
	uint32_t addr = 0;

	for (i = 0; i < 4; i++) {
		addr = readl(tnkhw_ioaddr + reg_offset[gmac][i]);
		TNKBD_DBG("%s addr[%d] = %x\n", __func__, i, addr);
		if (addr == 0)
			break;
	}
	if (i == 4) {
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This ip will not be actived for logic\n");
		return 0;
	}
	*pos_id = i;
	return reg_offset[gmac][i];
}
uint32_t tnkhw_ipv6select_reg_write(int gmac)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPV6_ADDR0_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_3},
		{TNK_REG_CT_LOCAL_IPV6_ADDR1_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_3} };
	int i;
	uint32_t addr1, addr2, addr3, addr4;

	addr1 = addr2 = addr3 = addr4 = 0;

	for (i = 0; i < 4; i++) {
		addr1 = readl(tnkhw_ioaddr + reg_offset[gmac][i]);
		addr2 = readl(tnkhw_ioaddr + reg_offset[gmac][i] + 4);
		addr3 = readl(tnkhw_ioaddr + reg_offset[gmac][i] + 8);
		addr4 = readl(tnkhw_ioaddr + reg_offset[gmac][i] + 12);
		if (!addr1 && !addr2 && !addr3 && !addr4)
			break;
	}
	if (i == 4) {
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This ip will not be actived for logic\n");
		return 0;
	}
	return reg_offset[gmac][i];
}

uint32_t tnkhw_ipv4select_reg_rst(int gmac, uint32_t addr, int *pos_id)
{
	uint32_t reg_offset[2][4] = {
		{ TNK_REG_CT_LOCAL_IPADDR0,
			TNK_REG_CT_LOCAL_IPADDR0_1,
			TNK_REG_CT_LOCAL_IPADDR0_2,
			TNK_REG_CT_LOCAL_IPADDR0_3},
		{TNK_REG_CT_LOCAL_IPADDR1,
			TNK_REG_CT_LOCAL_IPADDR1_1,
			TNK_REG_CT_LOCAL_IPADDR1_2,
			TNK_REG_CT_LOCAL_IPADDR1_3} };
	int i;
	uint32_t addrtmp = 0;

	for (i = 0; i < 4; i++) {
		addrtmp = readl(tnkhw_ioaddr + reg_offset[gmac][i]);
		if (addrtmp == addr)
			break;
	}
	if (i == 4) {
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This ip is not compare with those\n");
		return 0;
	}
	*pos_id = i;
	return reg_offset[gmac][i];
}
int tnkhw_ipv4address_rst(uint8_t *ipv4_addr, int gmac)
{
	uint32_t addr = 0, reg_offset;
	unsigned long flags;
	static int ipv4_cnt;
	int pos_id;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	if (ipv4_addr) {
		TNKBD_DBG("rst:Setting GMAC %d ipv4 address to %d.%d.%d.%d\n",
			gmac, ipv4_addr[0], ipv4_addr[1], ipv4_addr[2],
			ipv4_addr[3]);
		addr =
		    (ipv4_addr[0] << 24) | (ipv4_addr[1] << 16) | (ipv4_addr[2]
								   << 8) |
		    ipv4_addr[3];
	} else
		TNKBD_DBG("Setting GMAC %d ipv4 address to 0\n", gmac);
	/* bonjour function for TOE */
	ipv4_cnt = (gmac == 0) ? --eth0_ipv4_cnt : --eth1_ipv4_cnt;
	TNKBD_DBG("ipv4_cnt = %d\n", ipv4_cnt);

	if (ipv4_cnt < 0) {
		ipv4_cnt = (gmac == 0) ? ++eth0_ipv4_cnt : ++eth1_ipv4_cnt;
		pr_err("TOE ipv4 address are all deleted\n");
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}

	reg_offset = tnkhw_ipv4select_reg_rst(gmac, addr, &pos_id);

	if (reg_offset == 0) {
		ipv4_cnt = (gmac == 0) ? ++eth0_ipv4_cnt : ++eth1_ipv4_cnt;
		TNKBD_DBG("ipv4_cnt = %d\n", ipv4_cnt);
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
	writel(0, tnkhw_ioaddr + reg_offset);

	tnkhw_mac_addr_set(NULL, gmac, pos_id);

	tnkhw_vlan_id_set(0, gmac, pos_id);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}
uint32_t tnkhw_ipv6select_reg_rst(int gmac, uint8_t *ipv6_addr)
{
	uint32_t reg_offset[2][4] = {
		{TNK_REG_CT_LOCAL_IPV6_ADDR0_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR0_W0_3},
		{TNK_REG_CT_LOCAL_IPV6_ADDR1_W0,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_1,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_2,
			TNK_REG_CT_LOCAL_IPV6_ADDR1_W0_3} };
	int i, j;
	uint32_t addrtmp = 0;
	uint32_t addr[4] = {0};

	for (i = 0; i < 4; i++) {
		if (ipv6_addr)
			addr[i] = (ipv6_addr[0 + (i * 4)] << 24) |
			    (ipv6_addr[1 + (i * 4)] << 16) |
			    (ipv6_addr[2 + (i * 4)] << 8) |
			    ipv6_addr[3 + (i * 4)];
	}
	/* input ipv6 addr compare with the logic ipv6 addr */
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			addrtmp = readl(tnkhw_ioaddr
					+ reg_offset[gmac][i] + (j * 4));
			if (addrtmp != addr[j])
				break;
		}
		if (j == 4)
			break;
	}
	if (i == 4) {
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This ip is not compare with those\n");
		return 0;
	}
	return reg_offset[gmac][i];
}
int tnkhw_ipv6address_rst(uint8_t *ipv6_addr, int gmac)
{
	uint32_t reg_offset, i;
	unsigned long flags;
	int ipv6_cnt;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);
	/* bonjour function for TOE */
	ipv6_cnt = (gmac == 0) ? --eth0_ipv6_cnt : --eth1_ipv6_cnt;

	if (ipv6_cnt < 0) {
		ipv6_cnt = (gmac == 0) ? ++eth0_ipv6_cnt : ++eth1_ipv6_cnt;
		TNKBD_DBG("ipv6_cnt = %d\n", ipv6_cnt);
		pr_err("TOE ipv6 address are all deleted\n");
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
	reg_offset = tnkhw_ipv6select_reg_rst(gmac, ipv6_addr);
	TNKBD_DBG("ipv6_cnt = %d\n", ipv6_cnt);
	if (reg_offset == 0) {
		ipv6_cnt = (gmac == 0) ? ++eth0_ipv6_cnt : ++eth1_ipv6_cnt;
		TNKBD_DBG("ipv6_cnt = %d\n", ipv6_cnt);
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
	if (ipv6_addr) {
		TNKBD_DBG("Setting GMAC %d ipv6 address to", gmac);
		TNKBD_DBG
			(" %x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x\n",
			 ipv6_addr[0], ipv6_addr[1], ipv6_addr[2],
			 ipv6_addr[3], ipv6_addr[4], ipv6_addr[5],
			 ipv6_addr[6], ipv6_addr[7], ipv6_addr[8],
			 ipv6_addr[9], ipv6_addr[10], ipv6_addr[11],
			 ipv6_addr[12], ipv6_addr[13], ipv6_addr[14],
			 ipv6_addr[15]);
	} else
		TNKBD_DBG("Setting GMAC %d ipv6 address to 0\n", gmac);

	for (i = 0; i < 4; i++)
		writel(0, tnkhw_ioaddr + reg_offset + (i * 4));

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}
#endif

int tnkhw_ipv4address_set(uint8_t *ipv4_addr, int gmac,
		u16 vlan_id, uint8_t *mac_addr)
{
	uint32_t addr = 0, reg_offset;
	unsigned long flags;
#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
	int ipv4_cnt;
	int pos_id = 0;
#endif
	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	if (ipv4_addr) {
#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
		TNKBD_DBG("Setting GMAC %d ipv4 address to %d.%d.%d.%d\n",
			gmac, ipv4_addr[0], ipv4_addr[1], ipv4_addr[2],
			ipv4_addr[3]);
#endif
		addr =
		    (ipv4_addr[0] << 24) | (ipv4_addr[1] << 16) | (ipv4_addr[2]
								   << 8) |
		    ipv4_addr[3];
	} else
		TNK_DBG("Setting GMAC %d ipv4 address to 0\n", gmac);
#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
	/* bonjour function for TOE */
	ipv4_cnt = (gmac == 0) ? ++eth0_ipv4_cnt : ++eth1_ipv4_cnt;
	if (ipv4_cnt > TNK_IP_MAX_NUM) {
		ipv4_cnt = (gmac == 0) ? --eth0_ipv4_cnt : --eth1_ipv4_cnt;
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This IP will not be actived for logic\n");
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
	TNKBD_DBG("ipv4_cnt = %d\n", ipv4_cnt);

	reg_offset = tnkhw_ipv4select_reg_write(gmac, &pos_id);
	if (reg_offset == 0) {
		ipv4_cnt = (gmac == 0) ? --eth0_ipv4_cnt : --eth1_ipv4_cnt;
		TNKBD_DBG(" The ipv4_cnt = %d,", ipv4_cnt);
		TNKBD_DBG(" however there are not usefull Ipaddrs registors\n");
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
#else
	reg_offset = (gmac == 1) ? TNK_REG_CT_LOCAL_IPADDR1
		: TNK_REG_CT_LOCAL_IPADDR0;
	writel(addr, tnkhw_ioaddr + reg_offset);
#endif
#if defined(TNK_BONJOUR) || defined(TNK_BONDING)
	if (gmac == 0) {
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0_1 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0_1));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0_2 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0_2));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0_3 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0_3));
		writel(addr, tnkhw_ioaddr + reg_offset);
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0_1 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0_1));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0_2 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0_2));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR0_3 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR0_3));
	}
	if (gmac == 1) {
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1_1 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1_1));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1_2 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1_2));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1_3 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1_3));
		writel(addr, tnkhw_ioaddr + reg_offset);
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1_1 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1_1));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1_2 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1_2));
		TNKBD_DBG("TNK_REG_CT_LOCAL_IPADDR1_3 = %x\n",
				readl(tnkhw_ioaddr
					+ TNK_REG_CT_LOCAL_IPADDR1_3));
	}

	tnkhw_mac_addr_set(mac_addr, gmac, pos_id);

	tnkhw_vlan_id_set(vlan_id, gmac, pos_id);
#endif

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}

int tnkhw_ipv6address_set(uint8_t *ipv6_addr, int gmac)
{
	uint32_t reg_offset, i;
	unsigned long flags;
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
	int ipv6_cnt;
#endif
	spin_lock_irqsave(&tnkhw_reg_lock, flags);

#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
	/* bonjour function for TOE */
	ipv6_cnt = (gmac == 0) ? ++eth0_ipv6_cnt : ++eth1_ipv6_cnt;
	if (ipv6_cnt > TNK_IP_MAX_NUM) {
		ipv6_cnt = (gmac == 0) ? --eth0_ipv6_cnt : --eth1_ipv6_cnt;
		pr_err("TOE only support 4 IPaddrs on one dev\n");
		pr_err("This ip will not be actived for logic\n");
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
	reg_offset = tnkhw_ipv6select_reg_write(gmac);
	TNKBD_DBG("ipv6_cnt = %d\n", ipv6_cnt);
	if (reg_offset == 0) {
		ipv6_cnt = (gmac == 0) ? --eth0_ipv6_cnt : --eth1_ipv6_cnt;
		TNKBD_DBG(" The ipv6_cnt = %d,", ipv6_cnt);
		TNKBD_DBG(" however there are not usefull Ipaddrs registors\n");
		spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
		return -1;
	}
#else
	reg_offset =
	    (gmac ==
	     1) ? TNK_REG_CT_LOCAL_IPV6_ADDR1_W0 :
	    TNK_REG_CT_LOCAL_IPV6_ADDR0_W0;
#endif
	if (ipv6_addr) {
#if defined(TNK_BONDING) || defined(TNK_BONJOUR)
		TNKBD_DBG
			("Setting GMAC %d ipv6 address to",
			 gmac);
		TNKBD_DBG(" %x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x:%x%x\n",
				ipv6_addr[0], ipv6_addr[1], ipv6_addr[2],
				ipv6_addr[3], ipv6_addr[4], ipv6_addr[5],
				ipv6_addr[6], ipv6_addr[7], ipv6_addr[8],
				ipv6_addr[9], ipv6_addr[10], ipv6_addr[11],
				ipv6_addr[12], ipv6_addr[13], ipv6_addr[14],
				ipv6_addr[15]);
#endif
	} else
		TNK_DBG("Setting GMAC %d ipv6 address to 0\n", gmac);

	for (i = 0; i < 4; i++) {
		uint32_t addr = 0;
		if (ipv6_addr)
			addr = (ipv6_addr[0 + (i * 4)] << 24) |
			    (ipv6_addr[1 + (i * 4)] << 16) |
			    (ipv6_addr[2 + (i * 4)] << 8) |
			    ipv6_addr[3 + (i * 4)];

		writel(addr, tnkhw_ioaddr + reg_offset + (i * 4));
	}

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return 0;
}

void tnkhw_stats_get(struct tnkhw_stats *stats)
{
	memcpy(stats, &tnk_stats, sizeof(struct tnkhw_stats));
}

void tnkhw_conn_stats_get(unsigned cindex, struct tnkhw_conn_stats *stats)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];

	TNK_CINDEX_VALIDATE(cindex);

	memcpy(stats, &tnk_conn_stats[cindex], sizeof(struct tnkhw_conn_stats));
	tnkhw_ttx_txstat_get(cindex, &stats->tx_acked_count,
			     &stats->tx_retry_count);
	tnkhw_ttx_rxstat_get(cindex, &stats->rxed_byte_count);
	stats->ttx_pipelined_bytes = atomic_read(&t->pipelined_bytes);
}

void tnkhw_stats_clearall(void)
{
	int cindex;

	memset(&tnk_stats, 0, sizeof(struct tnkhw_stats));
	memset(tnk_conn_stats, 0,
	       tnk_max_connections * sizeof(struct tnkhw_conn_stats));

	for (cindex = TNK_TTX_CINDEX_START; cindex < tnk_max_connections;
	     cindex++) {
		tnkhw_ttx_txstat_clear(cindex);
		tnkhw_ttx_rxstat_clear(cindex);
	}
}

void tnkhw_inc_persist(void)
{
	int val;
	int count;
	int mask = TNK_MASK_CTRL1_PERSISTCOUNT << TNK_OFFSET_CTRL1_PERSISTCOUNT;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	val = readl(tnkhw_ioaddr + TNK_REG_TOE_CTRL1);
	count = (val & mask) + 1;
	val = (val & ~mask) | (count & mask);
	writel(val, tnkhw_ioaddr + TNK_REG_TOE_CTRL1);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

int tnkhw_tx_done_test(unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];

	return (t->tail == t->head);
}

int tnkhw_tx_done_wait(unsigned cindex, unsigned timeout_jiffies)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];

	while (t->tail != t->head && timeout_jiffies > 0 && !t->rst_received) {
		DEFINE_WAIT(wait);

		prepare_to_wait(&t->waitqueue, &wait, TASK_INTERRUPTIBLE);
		if (t->tail != t->head)
			timeout_jiffies = schedule_timeout(timeout_jiffies);
		finish_wait(&t->waitqueue, &wait);
	}
	return atomic_read(&t->pipelined_bytes);
}

inline void tnkhw_rcv_rst(unsigned cindex)
{
	struct tnkhw_tx_dma_info *t = &tnk_tx_dma_list[cindex];

	t->rst_received = 1;

	if (waitqueue_active(&t->waitqueue))
		wake_up_interruptible(&t->waitqueue);
}

#define TNKHW_CT_LOC_RTT_SAMPLE 22
#define TNKHW_CT_MASK_RTT_SAMPLE_VALID 0x10000
#define TNKHW_CT_MASK_RTT_SAMPLE_VALUE 0xFFFF
int tnkhw_rtt_sample(unsigned cindex)
{
	uint32_t data;
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	tnkhw_ct_read(cindex, TNKHW_CT_LOC_RTT_SAMPLE, &data);

	if (data & TNKHW_CT_MASK_RTT_SAMPLE_VALID) {
		/*  Clear the RTT sample valid bit, so that a new sample
		 *  will be collected */
		tnkhw_ct_write(cindex, TNKHW_CT_LOC_RTT_SAMPLE, 0);

		ret = data & TNKHW_CT_MASK_RTT_SAMPLE_VALUE;
		/* Assuming RTT value > TNK_TX_RTT_FILITER indicates
		 * invalid sample */
		if (ret > TNK_TX_RTT_FILITER)
			ret = -1;
	} else
		ret = -1; /*  negative number indicates invalid sample */

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	return ret;
}

#define TNKHW_CT_LOC_TXTIMEOUT 16
void tnkhw_txtimeout_update(unsigned cindex, unsigned timeout_ms)
{
	unsigned long flags;

	if (timeout_ms > TNK_TX_TIMEOUT_MAX)
		timeout_ms = TNK_TX_TIMEOUT_MAX;

	if (timeout_ms < TNK_TX_TIMEOUT_MIN)
		timeout_ms = TNK_TX_TIMEOUT_MIN;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/*  We only want to write the lower 16 bits of the CT entry */
	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_TXTIMEOUT, timeout_ms & 0xFFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

#ifdef TNK_RX_CHANNEL_FLOW_CONTROL

void tnkhw_txadvwnd_update(unsigned cindex, unsigned tx_adv_wnd)
{
	unsigned long flags;

	if (tx_adv_wnd > 0xFFFF)
		tx_adv_wnd = 0xFFFF;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	/*  We only want to write the upper 16 bits of the CT entry */
	writel(0xC, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_TXADVWND,
		       (tx_adv_wnd << 16) & 0xFFFF0000);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);

	TNK_DBG("%s: Updated tx_adv_wnd to %u for cid %u\n",
		__func__, cindex, tx_adv_wnd);
}

#endif

#if SWITCH_SEND_ACK
void tnkhw_send_ack(unsigned int cindex)
{
	unsigned int cpu_txack_stat;
	int limit = 10000;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_ack_cmd_lock, flags);

	writel(cindex, tnkhw_ioaddr + TNK_REG_CT_ACK_CMD_REQ);
	do {
		cpu_txack_stat = readl(tnkhw_ioaddr + TNK_REG_CT_ACK_CMD_REQ);
		cpu_txack_stat =
		    (cpu_txack_stat >> TNK_OFFSET_CT_CPU_TXACK_STAT) &
		    TNK_MASK_CT_CPU_TXACK_STAT;
	} while (!cpu_txack_stat && limit--);

	if (unlikely(limit == -1 && !cpu_txack_stat))
		pr_warn("%s: cpu_txack_stat is always 0!\n", __func__);

	spin_unlock_irqrestore(&tnkhw_reg_ack_cmd_lock, flags);
}
#endif

#if SWITCH_MULTI_INTR
void tnkhw_set_rx_irq(unsigned int cindex, unsigned int cpu)
{
	unsigned long flags;
	unsigned int reg_offset;
	unsigned int wr_data;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	reg_offset = TNK_OFFSET_LOOKUP_TABLE + TNK_OFFSET_AD_ENTRY +
		cindex * TNK_AD_ENTRY_SIZE + TNK_OFFSET_RX_IRQ;
	wr_data = (cpu & 0x3) << 16;
	writel(0x4, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_write_addr(reg_offset, wr_data);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

void tnkhw_set_tx_ack_irq(unsigned int cindex, unsigned int cpu)
{
	unsigned int cpu_flag;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	writel(0x3, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	tnkhw_ct_read(cindex, TNKHW_CT_LOC_FLAGS, &cpu_flag);
	cpu_flag = (cpu_flag &
		~(TNK_MASK_TX_ACK_IRQ << TNK_OFFSET_TX_ACK_IRQ)) |
		((cpu & TNK_MASK_TX_ACK_IRQ) << TNK_OFFSET_TX_ACK_IRQ);
	tnkhw_ct_write(cindex, TNKHW_CT_LOC_FLAGS, cpu_flag & 0xFFFF);
	writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

void tnkhw_get_txadvwnd_scale(unsigned int cindex, unsigned int *rcv_wscale)
{
	unsigned int wscale_wdata;
	unsigned int wscale_rdata;

	wscale_wdata = (cindex << TNK_OFFSET_WSCALE_CID) |
		(0 << TNK_OFFSET_WSCALE_WR);
	writel(wscale_wdata, tnkhw_ioaddr + TNK_REG_CT_TXADVWND_SCALE);
	wscale_rdata = readl(tnkhw_ioaddr + TNK_REG_CT_TXADVWND_SCALE);
	*rcv_wscale = (wscale_rdata >> 16) & 0xF;
}

void tnkhw_set_txadvwnd_scale(unsigned int cindex, unsigned int rcv_wscale)
{
	unsigned int wscale_wdata;
	unsigned long flags;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	wscale_wdata = (rcv_wscale & TNK_MASK_WSCALE_WDATA) |
		(cindex << TNK_OFFSET_WSCALE_CID) |
		(1 << TNK_OFFSET_WSCALE_WR);
	writel(wscale_wdata, tnkhw_ioaddr + TNK_REG_CT_TXADVWND_SCALE);

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

#ifdef TNK_DBG_HW_INTERFACE
void tnkhw_ct_access(int ct_access_val[])
{
	int cid, loc, bit_low, bit_hig, rw, val;
	unsigned int rd_val, wr_val, bit_mask;
	int i;
	unsigned long flags;

	cid	= ct_access_val[0];
	loc	= ct_access_val[1];
	bit_low = ct_access_val[2];
	bit_hig = ct_access_val[3];
	rw	= ct_access_val[4];
	val	= ct_access_val[5];

	if (TNK_CINDEX_VALIDATE_WARN(cid))
		return;
	if (WARN_ON(loc < 0 || loc > 31))
		return;
	if (WARN_ON(bit_low < 0 || bit_low > 31))
		return;
	if (WARN_ON(bit_hig < 0 || bit_hig > 31))
		return;
	if (WARN_ON(bit_low > bit_hig))
		return;
	if (WARN_ON(rw < 0 || rw > 1))
		return;

	bit_mask = 0;
	for (i = bit_low; i <= bit_hig; i++)
		bit_mask |= 1 << i;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	if (rw) {
		writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
		tnkhw_ct_read(cid, loc, &wr_val);
		wr_val = (wr_val & ~bit_mask) | ((val << bit_low) & bit_mask);
		tnkhw_ct_write(cid, loc, wr_val);
		writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	} else {
		tnkhw_ct_read(cid, loc, &rd_val);
		ct_access_val[5] = (rd_val & bit_mask) >> bit_low;
	}

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}

void tnkhw_lookup_access(int lookup_access_val[])
{
	int loc, bit_low, bit_hig, rw, val;
	unsigned int rd_val, wr_val, bit_mask;
	unsigned int reg_off;
	int i;
	unsigned long flags;

	loc	= lookup_access_val[0];
	bit_low	= lookup_access_val[1];
	bit_hig = lookup_access_val[2];
	rw	= lookup_access_val[3];
	val	= lookup_access_val[4];

	if (WARN_ON(loc < 0))
		return;
	if (WARN_ON(bit_low < 0 || bit_low > 31))
		return;
	if (WARN_ON(bit_hig < 0 || bit_hig > 31))
		return;
	if (WARN_ON(bit_low > bit_hig))
		return;
	if (WARN_ON(rw < 0 || rw > 1))
		return;

	bit_mask = 0;
	for (i = bit_low; i <= bit_hig; i++)
		bit_mask |= 1 << i;

	spin_lock_irqsave(&tnkhw_reg_lock, flags);

	reg_off = TNK_OFFSET_LOOKUP_TABLE + loc;
	if (rw) {
		writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
		tnkhw_ct_read_addr(reg_off, &wr_val);
		wr_val = (wr_val & ~bit_mask) | ((val << bit_low) & bit_mask);
		tnkhw_ct_write_addr(reg_off, wr_val);
		writel(0xF, tnkhw_ioaddr + TNK_REG_CT_CPU_WRMASK);
	} else {
		tnkhw_ct_read_addr(reg_off, &rd_val);
		lookup_access_val[4] = (rd_val & bit_mask) >> bit_low;
	}

	spin_unlock_irqrestore(&tnkhw_reg_lock, flags);
}
#endif

#ifdef TNK_RECV_LRO_DATA_DEBUG
void tnk_print_memory(char *buf, int len)
{
	int i;

	pr_err("[%s:%d]Packet buf=%p, length= %#4x\n",
		__func__, __LINE__, buf, len);
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			pr_err("%#4.4x", i);
		if (i % 2 == 0)
			pr_err(" ");
		pr_err("%2.2x", ((unsigned char *)buf)[i]);
		if (i % 16 == 15)
			pr_err("\n");
	}
	pr_err("\n\n");
}
#endif
