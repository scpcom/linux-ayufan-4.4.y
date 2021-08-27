#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
 
#include "mvCommon.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>
#include <linux/mv_neta.h>
#include <linux/prefetch.h>
#ifdef MY_ABC_HERE
#include <linux/mii.h>
#endif
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/module.h>
#include "mvOs.h"
#include "mvDebug.h"
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "eth-phy/mvEthPhy.h"
#include "mvSysEthPhyApi.h"
#include "mvSysNetaApi.h"

#include "gbe/mvNeta.h"
#include "bm/mvBm.h"
#include "pnc/mvPnc.h"
#include "pnc/mvTcam.h"
#include "pmt/mvPmt.h"

#include "mv_switch.h"

#include "mv_netdev.h"
#include "mv_eth_tool.h"

#include "cpu/mvCpuCntrs.h"

#ifdef CONFIG_MV_CPU_PERF_CNTRS
MV_CPU_CNTRS_EVENT	*event0 = NULL;
MV_CPU_CNTRS_EVENT	*event1 = NULL;
MV_CPU_CNTRS_EVENT	*event2 = NULL;
MV_CPU_CNTRS_EVENT	*event3 = NULL;
MV_CPU_CNTRS_EVENT	*event4 = NULL;
MV_CPU_CNTRS_EVENT	*event5 = NULL;
#endif /* CONFIG_MV_CPU_PERF_CNTRS */

unsigned int ext_switch_port_mask = 0;

#ifdef MY_ABC_HERE
extern MV_U32 syno_wol_support(struct eth_port *pp);
#endif

void handle_group_affinity(int port);
void set_rxq_affinity(struct eth_port *pp, MV_U32 rxqAffinity, int group);
static inline int mv_eth_tx_policy(struct eth_port *pp, struct sk_buff *skb);

#ifdef MY_ABC_HERE
extern int mv_eth_tool_read_phy_reg(int phy_addr, u16 page, u16 reg, u16 *val);
DEFINE_SPINLOCK(mii_lock);
#endif

/* uncomment if you want to debug the SKB recycle feature */
/* #define ETH_SKB_DEBUG */

static int pm_flag = 0;
static int wol_ports_bmp = 0;

#ifdef CONFIG_MV_ETH_PNC
unsigned int mv_eth_pnc_ctrl_en = 1;

int mv_eth_ctrl_pnc(int en)
{
	mv_eth_pnc_ctrl_en = en;
	return MV_OK;
}
#endif /* CONFIG_MV_ETH_PNC */

#ifdef CONFIG_NET_SKB_RECYCLE
int mv_ctrl_recycle = CONFIG_NET_SKB_RECYCLE_DEF;
EXPORT_SYMBOL(mv_ctrl_recycle);

int mv_eth_ctrl_recycle(int en)
{
	mv_ctrl_recycle = en;
	return MV_OK;
}
#else
int mv_eth_ctrl_recycle(int en)
{
	printk(KERN_ERR "SKB recycle is not supported\n");
	return 1;
}
#endif /* CONFIG_NET_SKB_RECYCLE */

extern u8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][MV_MAC_ADDR_SIZE];
extern u16 mvMtu[CONFIG_MV_ETH_PORTS_NUM];

extern unsigned int switch_enabled_ports;

struct bm_pool mv_eth_pool[MV_ETH_BM_POOLS];
struct eth_port **mv_eth_ports;
struct net_device **mv_net_devs;

int mv_net_devs_num = 0;
int mv_ctrl_txdone = CONFIG_MV_ETH_TXDONE_COAL_PKTS;
EXPORT_SYMBOL(mv_ctrl_txdone);

/*
 * Static declarations
 */
static int mv_eth_ports_num = 0;
static int mv_net_devs_max = 0;

static int mv_eth_initialized = 0;

/*
 * Local functions
 */
static void mv_eth_txq_delete(struct eth_port *pp, struct tx_queue *txq_ctrl);
static void mv_eth_tx_timeout(struct net_device *dev);
static int  mv_eth_tx(struct sk_buff *skb, struct net_device *dev);
static void mv_eth_tx_frag_process(struct eth_port *pp, struct sk_buff *skb, struct tx_queue *txq_ctrl, u16 flags);
static int mv_eth_rxq_fill(struct eth_port *pp, int rxq, int num);

static void mv_eth_config_show(void);
static int  mv_eth_priv_init(struct eth_port *pp, int port);
static void mv_eth_priv_cleanup(struct eth_port *pp);
static int  mv_eth_config_get(struct eth_port *pp, u8 *mac);
static int  mv_eth_hal_init(struct eth_port *pp);
struct net_device *mv_eth_netdev_init(struct eth_port *pp, int mtu, u8 *mac,
					struct platform_device *pdev);
static void mv_eth_netdev_set_features(struct net_device *dev);
static void mv_eth_netdev_update_features(struct net_device *dev);

static MV_STATUS mv_eth_pool_create(int pool, int capacity);
static int mv_eth_pool_add(int pool, int buf_num);
static int mv_eth_pool_free(int pool, int num);
static int mv_eth_pool_destroy(int pool);

#ifdef CONFIG_MV_ETH_TSO
int mv_eth_tx_tso(struct sk_buff *skb, struct net_device *dev, struct mv_eth_tx_spec *tx_spec,
		struct tx_queue *txq_ctrl);
#endif

/* Get the configuration string from the Kernel Command Line */
static char *port0_config_str = NULL, *port1_config_str = NULL, *port2_config_str = NULL, *port3_config_str = NULL;
int mv_eth_cmdline_port0_config(char *s);
__setup("mv_port0_config=", mv_eth_cmdline_port0_config);
int mv_eth_cmdline_port1_config(char *s);
__setup("mv_port1_config=", mv_eth_cmdline_port1_config);
int mv_eth_cmdline_port2_config(char *s);
__setup("mv_port2_config=", mv_eth_cmdline_port2_config);
int mv_eth_cmdline_port3_config(char *s);
__setup("mv_port3_config=", mv_eth_cmdline_port3_config);

int mv_eth_cmdline_port0_config(char *s)
{
	port0_config_str = s;
	return 1;
}

int mv_eth_cmdline_port1_config(char *s)
{
	port1_config_str = s;
	return 1;
}

int mv_eth_cmdline_port2_config(char *s)
{
	port2_config_str = s;
	return 1;
}

int mv_eth_cmdline_port3_config(char *s)
{
	port3_config_str = s;
	return 1;
}
void mv_eth_stack_print(int port, MV_BOOL isPrintElements)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_INFO "port doens not exist (%d) in %s\n" , port, __func__);
		return;
	}

	if (pp->pool_long == NULL) {
		printk(KERN_ERR "Error - long pool is null in %s\n", __func__);
		return;
	}

	printk(KERN_INFO "Long pool (%d) stack\n", pp->pool_long->pool);
	mvStackStatus(pp->pool_long->stack, isPrintElements);

#ifdef CONFIG_MV_ETH_BM
	if (pp->pool_short == NULL) {
		printk(KERN_ERR "Error - short pool is null in %s\n", __func__);
		return;
	}

	printk(KERN_INFO "Short pool (%d) stack\n", pp->pool_short->pool);
	mvStackStatus(pp->pool_short->stack, isPrintElements);
#endif /* CONFIG_MV_ETH_BM */
}

void set_cpu_affinity(struct eth_port *pp, MV_U32 cpuAffinity, int group)
{
	int cpu;
	struct cpu_ctrl	*cpuCtrl;
	MV_U32 rxqAffinity = 0;

	/* nothing to do when cpuAffinity == 0 */
	if (cpuAffinity == 0)
		return;

	/* First, read affinity of the target group, in case it contains CPUs */
	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		cpuCtrl = pp->cpu_config[cpu];
		if (!(MV_BIT_CHECK(pp->cpuMask, cpu)))
			continue;
		if (cpuCtrl->napiCpuGroup == group) {
			rxqAffinity = MV_REG_READ(NETA_CPU_MAP_REG(pp->port, cpu)) & 0xff;
			break;
		}
	}
	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		if (cpuAffinity & 1) {
			cpuCtrl = pp->cpu_config[cpu];
			cpuCtrl->napi = pp->napiGroup[group];
			cpuCtrl->napiCpuGroup = group;
			cpuCtrl->cpuRxqMask = rxqAffinity;
			/* set rxq affinity of the target group */
			mvNetaRxqCpuMaskSet(pp->port, rxqAffinity, cpu);
		}
		cpuAffinity >>= 1;
	}
}

int group_has_cpus(struct eth_port *pp, int group)
{
	int cpu;
	struct cpu_ctrl	*cpuCtrl;

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		if (!(MV_BIT_CHECK(pp->cpuMask, cpu)))
			continue;

		cpuCtrl = pp->cpu_config[cpu];

		if (cpuCtrl->napiCpuGroup == group)
			return 1;
	}

	/* the group contains no CPU */
	return MV_OK;
}

void set_rxq_affinity(struct eth_port *pp, MV_U32 rxqAffinity, int group)
{
	int rxq, cpu;
	MV_U32 regVal;
	MV_U32 tmpRxqAffinity;
	int groupHasCpus;
	int cpuInGroup;
	struct cpu_ctrl	*cpuCtrl;

	/* nothing to do when rxqAffinity == 0 */
	if (rxqAffinity == 0)
		return;

	groupHasCpus = group_has_cpus(pp, group);

	if (!groupHasCpus) {
		printk(KERN_ERR "%s: operation not performed; group %d has no cpu \n", __func__, group);
		return;
	}

   for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		if (!(MV_BIT_CHECK(pp->cpuMask, cpu)))
			continue;
	   tmpRxqAffinity = rxqAffinity;

	   regVal = MV_REG_READ(NETA_CPU_MAP_REG(pp->port, cpu));
	   cpuCtrl = pp->cpu_config[cpu];

	   if (cpuCtrl->napiCpuGroup == group) {
		   cpuInGroup = 1;
		   /* init TXQ Access Enable bits */
		   regVal = regVal & 0xff00;
	   } else {
		   cpuInGroup = 0;
		}

	   for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		   /* set rxq affinity for this cpu */
		   if (tmpRxqAffinity & 1) {
			   if (cpuInGroup)
				   regVal |= NETA_CPU_RXQ_ACCESS_MASK(rxq);
			   else
				   regVal &= ~NETA_CPU_RXQ_ACCESS_MASK(rxq);
			}
			tmpRxqAffinity >>= 1;
	   }

	MV_REG_WRITE(NETA_CPU_MAP_REG(pp->port, cpu), regVal);
	cpuCtrl->cpuRxqMask = regVal;
   }
}

static int mv_eth_port_config_parse(struct eth_port *pp)
{
	char *str;

	printk(KERN_ERR "\n");
	if (pp == NULL) {
		printk(KERN_ERR "  o mv_eth_port_config_parse: got NULL pp\n");
		return MV_ERROR;
	}

	switch (pp->port) {
	case 0:
		str = port0_config_str;
		break;
	case 1:
		str = port1_config_str;
		break;
	case 2:
		str = port2_config_str;
		break;
	case 3:
		str = port3_config_str;
		break;
	default:
		printk(KERN_ERR "  o mv_eth_port_config_parse: got unknown port %d\n", pp->port);
		return MV_ERROR;
	}

	if (str != NULL) {
		if ((!strcmp(str, "disconnected")) || (!strcmp(str, "Disconnected"))) {
			printk(KERN_ERR "  o Port %d is disconnected from Linux netdevice\n", pp->port);
			clear_bit(MV_ETH_F_CONNECT_LINUX_BIT, &(pp->flags));
			return MV_OK;
		}
	}

	printk(KERN_ERR "  o Port %d is connected to Linux netdevice\n", pp->port);
	set_bit(MV_ETH_F_CONNECT_LINUX_BIT, &(pp->flags));
	return MV_OK;
}

#ifdef ETH_SKB_DEBUG
struct sk_buff *mv_eth_skb_debug[MV_BM_POOL_CAP_MAX * MV_ETH_BM_POOLS];
static spinlock_t skb_debug_lock;

void mv_eth_skb_check(struct sk_buff *skb)
{
	int i;
	struct sk_buff *temp;
	unsigned long flags;

	if (skb == NULL)
		printk(KERN_ERR "mv_eth_skb_check: got NULL SKB\n");

	spin_lock_irqsave(&skb_debug_lock, flags);

	i = *((u32 *)&skb->cb[0]);

	if ((i >= 0) && (i < MV_BM_POOL_CAP_MAX * MV_ETH_BM_POOLS)) {
		temp = mv_eth_skb_debug[i];
		if (mv_eth_skb_debug[i] != skb) {
			printk(KERN_ERR "mv_eth_skb_check: Unexpected skb: %p (%d) != %p (%d)\n",
			       skb, i, temp, *((u32 *)&temp->cb[0]));
		}
		mv_eth_skb_debug[i] = NULL;
	} else {
		printk(KERN_ERR "mv_eth_skb_check: skb->cb=%d is out of range\n", i);
	}

	spin_unlock_irqrestore(&skb_debug_lock, flags);
}

void mv_eth_skb_save(struct sk_buff *skb, const char *s)
{
	int i;
	int saved = 0;
	unsigned long flags;

	spin_lock_irqsave(&skb_debug_lock, flags);

	for (i = 0; i < MV_BM_POOL_CAP_MAX * MV_ETH_BM_POOLS; i++) {
		if (mv_eth_skb_debug[i] == skb) {
			printk(KERN_ERR "%s: mv_eth_skb_debug Duplicate: i=%d, skb=%p\n", s, i, skb);
			mv_eth_skb_print(skb);
		}

		if ((!saved) && (mv_eth_skb_debug[i] == NULL)) {
			mv_eth_skb_debug[i] = skb;
			*((u32 *)&skb->cb[0]) = i;
			saved = 1;
		}
	}

	spin_unlock_irqrestore(&skb_debug_lock, flags);

	if ((i == MV_BM_POOL_CAP_MAX * MV_ETH_BM_POOLS) && (!saved))
		printk(KERN_ERR "mv_eth_skb_debug is FULL, skb=%p\n", skb);
}
#endif /* ETH_SKB_DEBUG */

struct eth_port *mv_eth_port_by_id(unsigned int port)
{
	if (port < mv_eth_ports_num)
		return mv_eth_ports[port];

	return NULL;
}

struct net_device *mv_eth_netdev_by_id(unsigned int idx)
{
	if (idx < mv_net_devs_num)
		return mv_net_devs[idx];

	return NULL;
}

static inline int mv_eth_skb_mh_add(struct sk_buff *skb, u16 mh)
{
	/* sanity: Check that there is place for MH in the buffer */
	if (skb_headroom(skb) < MV_ETH_MH_SIZE) {
		printk(KERN_ERR "%s: skb (%p) doesn't have place for MH, head=%p, data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	/* Prepare place for MH header */
	skb->len += MV_ETH_MH_SIZE;
	skb->data -= MV_ETH_MH_SIZE;
	*((u16 *) skb->data) = mh;

	return MV_OK;
}

void mv_eth_ctrl_txdone(int num)
{
	mv_ctrl_txdone = num;
}

int mv_eth_ctrl_flag(int port, u32 flag, u32 val)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	u32 bit_flag = (fls(flag) - 1);

	if (!pp)
		return -ENODEV;

	if ((flag == MV_ETH_F_MH) && (pp->flags & MV_ETH_F_SWITCH)) {
		printk(KERN_ERR "Error: cannot change Marvell Header on a port used by the Gateway driver\n");
		return -EPERM;
	}

	if (val)
		set_bit(bit_flag, &(pp->flags));
	else
		clear_bit(bit_flag, &(pp->flags));

	if (flag == MV_ETH_F_MH)
		mvNetaMhSet(pp->port, val ? MV_NETA_MH : MV_NETA_MH_NONE);

	return MV_OK;
}

int mv_eth_ctrl_port_buf_num_set(int port, int long_num, int short_num)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_INFO "port doens not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}
	if (pp->pool_long != NULL) {
		/* Update number of buffers in existing pool (allocate or free) */
		if (pp->pool_long_num > long_num)
			mv_eth_pool_free(pp->pool_long->pool, pp->pool_long_num - long_num);
		else if (long_num > pp->pool_long_num)
			mv_eth_pool_add(pp->pool_long->pool, long_num - pp->pool_long_num);
	}
	pp->pool_long_num = long_num;

#ifdef CONFIG_MV_ETH_BM_CPU
	if (pp->pool_short != NULL) {
		/* Update number of buffers in existing pool (allocate or free) */
		if (pp->pool_short_num > short_num)
			mv_eth_pool_free(pp->pool_short->pool, pp->pool_short_num - short_num);
		else if (short_num > pp->pool_short_num)
			mv_eth_pool_add(pp->pool_short->pool, short_num - pp->pool_short_num);
	}
	pp->pool_short_num = short_num;
#endif /* CONFIG_MV_ETH_BM_CPU */

	return MV_OK;
}

#ifdef CONFIG_MV_ETH_BM
/* Set pkt_size for the pool. Check that pool not in use (all ports are stopped) */
/* Free all buffers from the pool */
/* Detach the pool from all ports */
int mv_eth_ctrl_pool_size_set(int pool, int pkt_size)
{
#ifdef CONFIG_MV_ETH_BM_CPU
	int port;
	struct bm_pool *ppool;
	struct eth_port *pp;

	if (mvNetaMaxCheck(pool, MV_ETH_BM_POOLS))
		return -EINVAL;

	ppool = &mv_eth_pool[pool];

	for (port = 0; port < mv_eth_ports_num; port++) {
		/* Check that all ports using this pool are stopped */
		if (ppool->port_map & (1 << port)) {
			pp = mv_eth_port_by_id(port);

			if (pp->flags & MV_ETH_F_STARTED) {
				printk(KERN_ERR "Port %d use pool #%d and must be stopped before change pkt_size\n",
					port, pool);
				return -EINVAL;
			}
		}
	}
	for (port = 0; port < mv_eth_ports_num; port++) {
		/* Free all buffers and detach pool */
		if (ppool->port_map & (1 << port)) {
			pp = mv_eth_port_by_id(port);

			if (ppool == pp->pool_long) {
				mv_eth_pool_free(pool, pp->pool_long_num);
				ppool->port_map &= ~(1 << pp->port);
				pp->pool_long = NULL;
			}
			if (ppool == pp->pool_short) {
				mv_eth_pool_free(pool, pp->pool_short_num);
				ppool->port_map &= ~(1 << pp->port);
				pp->pool_short = NULL;
			}
		}
	}
	ppool->pkt_size = pkt_size;
#endif /* CONFIG_MV_ETH_BM_CPU */

	mv_eth_bm_config_pkt_size_set(pool, pkt_size);
	if (pkt_size == 0)
		mvBmPoolBufSizeSet(pool, 0);
	else
		mvBmPoolBufSizeSet(pool, RX_BUF_SIZE(pkt_size));

	return MV_OK;
}
#endif /* CONFIG_MV_ETH_BM */

int mv_eth_ctrl_set_poll_rx_weight(int port, u32 weight)
{
	struct cpu_ctrl	*cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);
	int cpu;

	if (pp == NULL) {
		printk(KERN_INFO "port doens not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	if (weight > 255)
		weight = 255;
	pp->weight = weight;

	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		if (cpuCtrl->napi)
			cpuCtrl->napi->weight = pp->weight;
	}

	return MV_OK;
}

int mv_eth_ctrl_rxq_size_set(int port, int rxq, int value)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct rx_queue	*rxq_ctrl;

	if (pp == NULL) {
		printk(KERN_INFO "port doens not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}
	rxq_ctrl = &pp->rxq_ctrl[rxq];
	if ((rxq_ctrl->q) && (rxq_ctrl->rxq_size != value)) {
		/* Reset is required when RXQ ring size is changed */
		mv_eth_rx_reset(pp->port);

		mvNetaRxqDelete(pp->port, rxq);
		rxq_ctrl->q = NULL;
	}
	pp->rxq_ctrl[rxq].rxq_size = value;

	/* New RXQ will be created during mv_eth_start_internals */
	return MV_OK;
}

int mv_eth_ctrl_txq_size_set(int port, int txp, int txq, int value)
{
	struct tx_queue *txq_ctrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_INFO "port doens not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}
	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
	if ((txq_ctrl->q) && (txq_ctrl->txq_size != value)) {
		mv_eth_txq_delete(pp, txq_ctrl);
		/* Reset of port/txp is required when TXQ ring size is changed */
		/* Reset done before as part of stop_internals function */
	}
	txq_ctrl->txq_size = value;

	/* New TXQ will be created during mv_eth_start_internals */
	return MV_OK;
}

int mv_eth_ctrl_txq_mode_get(int port, int txp, int txq, int *value)
{
	int cpu, mode = MV_ETH_TXQ_FREE, val = 0;
	struct tx_queue *txq_ctrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
	for_each_possible_cpu(cpu)
		if (txq_ctrl->cpu_owner[cpu]) {
			mode = MV_ETH_TXQ_CPU;
			val += txq_ctrl->cpu_owner[cpu];
		}
	if ((mode == MV_ETH_TXQ_FREE) && (txq_ctrl->hwf_rxp < (MV_U8) mv_eth_ports_num)) {
		mode = MV_ETH_TXQ_HWF;
		val = txq_ctrl->hwf_rxp;
	}
	if (value)
		*value = val;

	return mode;
}

/* Increment/Decrement CPU ownership for this TXQ */
int mv_eth_ctrl_txq_cpu_own(int port, int txp, int txq, int add, int cpu)
{
	int mode;
	struct tx_queue *txq_ctrl;
	struct cpu_ctrl	*cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	/* Check that new txp/txq can be allocated for CPU */
	mode = mv_eth_ctrl_txq_mode_get(port, txp, txq, NULL);

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
	cpuCtrl = pp->cpu_config[cpu];

	if (add) {
		if ((mode != MV_ETH_TXQ_CPU) && (mode != MV_ETH_TXQ_FREE))
			return -EINVAL;

		txq_ctrl->cpu_owner[cpu]++;

	} else {
		if (mode != MV_ETH_TXQ_CPU)
			return -EINVAL;

		txq_ctrl->cpu_owner[cpu]--;
	}

	mv_eth_txq_update_shared(txq_ctrl, pp);

	return MV_OK;
}

/* Set TXQ ownership to HWF from the RX port.  rxp=-1 - free TXQ ownership */
int mv_eth_ctrl_txq_hwf_own(int port, int txp, int txq, int rxp)
{
	int mode;
	struct tx_queue *txq_ctrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	/* Check that new txp/txq can be allocated for HWF */
	mode = mv_eth_ctrl_txq_mode_get(port, txp, txq, NULL);

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

	if (rxp == -1) {
		if (mode != MV_ETH_TXQ_HWF)
			return -EINVAL;
	} else {
		if ((mode != MV_ETH_TXQ_HWF) && (mode != MV_ETH_TXQ_FREE))
			return -EINVAL;
	}

	txq_ctrl->hwf_rxp = (MV_U8) rxp;

	return MV_OK;
}

/* set or clear shared bit for this txq, txp=1 for pon , 0 for gbe */
int mv_eth_shared_set(int port, int txp, int txq, int value)
{
	struct tx_queue *txq_ctrl;
	struct eth_port *pp = mv_eth_port_by_id(port);
	if ((value < 0) || (value > 1)) {
		printk(KERN_ERR "%s:Invalid value %d , should be 0 or 1.\n \n", __func__, value);
		return -EINVAL;
	}

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp is null \n", __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

	if (txq_ctrl == NULL) {
		printk(KERN_ERR "%s: txq_ctrl is null \n", __func__);
		return -EINVAL;
	}

	value ? (txq_ctrl->flags |= MV_ETH_F_TX_SHARED) : (txq_ctrl->flags &= ~MV_ETH_F_TX_SHARED);

	return MV_OK;
}

/* Set TXQ for CPU originated packets */
int mv_eth_ctrl_txq_cpu_def(int port, int txp, int txq, int cpu)
{
	struct cpu_ctrl	*cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if ((cpu >= CONFIG_NR_CPUS) || (cpu < 0)) {
		printk(KERN_ERR "cpu #%d is out of range: from 0 to %d\n",
			cpu, CONFIG_NR_CPUS - 1);
		return -EINVAL;
	}

	if (mvNetaTxpCheck(port, txp))
		return -EINVAL;

	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	cpuCtrl = pp->cpu_config[cpu];

	/* Check that new txq can be allocated for CPU */
	if (!(MV_BIT_CHECK(cpuCtrl->cpuTxqMask, txq)))	{
		printk(KERN_ERR "Txq #%d can not allocated for cpu #%d\n", txq, cpu);
		return -EINVAL;
	}

	/* Decrement CPU ownership for old txq */
	mv_eth_ctrl_txq_cpu_own(port, pp->txp, cpuCtrl->txq, 0, cpu);

	if (txq != -1) {
		if (mvNetaMaxCheck(txq, CONFIG_MV_ETH_TXQ))
			return -EINVAL;

		/* Increment CPU ownership for new txq */
		if (mv_eth_ctrl_txq_cpu_own(port, txp, txq, 1, cpu))
			return -EINVAL;
	}
	pp->txp = txp;
	cpuCtrl->txq = txq;

	return MV_OK;
}

int	mv_eth_cpu_txq_mask_set(int port, int cpu, int txqMask)
{
	struct tx_queue *txq_ctrl;
	int i;
	struct cpu_ctrl	*cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	if ((cpu >= CONFIG_NR_CPUS) || (cpu < 0)) {
		printk(KERN_ERR "cpu #%d is out of range: from 0 to %d\n",
			cpu, CONFIG_NR_CPUS - 1);
		return -EINVAL;
	}
	if (pp == NULL) {
		printk(KERN_ERR "%s: pp is null \n", __func__);
		return MV_FAIL;
	}

	if (!(MV_BIT_CHECK(pp->cpuMask, cpu)))	{
		printk(KERN_ERR "%s:Error- Cpu #%d masked for port  #%d\n", __func__, cpu, port);
		return -EINVAL;
	}

	cpuCtrl = pp->cpu_config[cpu];

	/* validate that default txq is not masked by the new txqMask Value */
	if (!(MV_BIT_CHECK(txqMask, cpuCtrl->txq))) {
		printk(KERN_ERR "Error: port %d default txq %d can not be masked.\n", port, cpuCtrl->txq);
		return -EINVAL;
	}

	/* validate that txq values in tos map are not masked by the new txqMask Value */
	for (i = 0; i < 256; i++)
		if (cpuCtrl->txq_tos_map[i] != MV_ETH_TXQ_INVALID)
			if (!(MV_BIT_CHECK(txqMask, cpuCtrl->txq_tos_map[i]))) {
				printk(KERN_WARNING "Warning: port %d tos 0h%x mapped to txq %d ,this rule delete due to new masked value (0X%x).\n",
					port, i, cpuCtrl->txq_tos_map[i], txqMask);
				txq_ctrl = &pp->txq_ctrl[pp->txp * CONFIG_MV_ETH_TXQ + cpuCtrl->txq_tos_map[i]];
				txq_ctrl->cpu_owner[cpu]--;
				mv_eth_txq_update_shared(txq_ctrl, pp);
				cpuCtrl->txq_tos_map[i] = MV_ETH_TXQ_INVALID;
			}

	/* nfp validation - can not mask nfp rules*/
	for (i = 0; i < CONFIG_MV_ETH_TXQ; i++)
		if (!(MV_BIT_CHECK(txqMask, i))) {
			txq_ctrl = &pp->txq_ctrl[pp->txp * CONFIG_MV_ETH_TXQ + i];
			if ((txq_ctrl != NULL) && (txq_ctrl->nfpCounter != 0)) {
				printk(KERN_ERR "Error: port %d txq %d ruled by NFP, can not be masked.\n", port, i);
				return -EINVAL;
			}
		}

	mvNetaTxqCpuMaskSet(port, txqMask, cpu);
	cpuCtrl->cpuTxqMask = txqMask;

	return MV_OK;
}

int mv_eth_ctrl_tx_cmd(int port, u32 tx_cmd)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->hw_cmd = tx_cmd;

	return MV_OK;
}

int mv_eth_ctrl_tx_mh(int port, u16 mh)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_mh = mh;

	return MV_OK;
}

#ifdef CONFIG_MV_ETH_TX_SPECIAL
/* Register special transmit check function */
void mv_eth_tx_special_check_func(int port,
					int (*func)(int port, struct net_device *dev, struct sk_buff *skb,
								struct mv_eth_tx_spec *tx_spec_out))
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	pp->tx_special_check = func;
}
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#ifdef CONFIG_MV_ETH_RX_SPECIAL
/* Register special transmit check function */
void mv_eth_rx_special_proc_func(int port, void (*func)(int port, int rxq, struct net_device *dev,
							struct sk_buff *skb, struct neta_rx_desc *rx_desc))
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	pp->rx_special_proc = func;
}
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

static inline u16 mv_eth_select_txq(struct net_device *dev,
									struct sk_buff *skb)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);
	return mv_eth_tx_policy(pp, skb);
}

#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
static int mv_eth_set_features(struct net_device *dev, u32 features)
{
	u32 changed = dev->features ^ features;

	if (changed & NETIF_F_RXHASH) {
		if (features & NETIF_F_RXHASH) {
			dev->features |= NETIF_F_RXHASH;
			mvPncLbModeIp4(LB_2_TUPLE_VALUE);
			mvPncLbModeIp6(LB_2_TUPLE_VALUE);
			mvPncLbModeL4(LB_4_TUPLE_VALUE);
		} else {
			dev->features |= ~NETIF_F_RXHASH;
			mvPncLbModeIp4(LB_DISABLE_VALUE);
			mvPncLbModeIp6(LB_DISABLE_VALUE);
			mvPncLbModeL4(LB_DISABLE_VALUE);
		}
	}
	return MV_OK;
}
#endif

static const struct net_device_ops mv_eth_netdev_ops = {
	.ndo_open = mv_eth_open,
	.ndo_stop = mv_eth_stop,
	.ndo_start_xmit = mv_eth_tx,
	.ndo_set_rx_mode = mv_eth_set_multicast_list,
	.ndo_set_mac_address = mv_eth_set_mac_addr,
	.ndo_change_mtu = mv_eth_change_mtu,
	.ndo_tx_timeout = mv_eth_tx_timeout,
	.ndo_select_queue = mv_eth_select_txq,
#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
	.ndo_set_features = mv_eth_set_features,
#endif
};

#ifdef CONFIG_MV_ETH_SWITCH

static const struct net_device_ops mv_switch_netdev_ops = {
	.ndo_open = mv_eth_switch_start,
	.ndo_stop = mv_eth_switch_stop,
	.ndo_start_xmit = mv_eth_tx,
	.ndo_set_rx_mode = mv_eth_switch_set_multicast_list,
	.ndo_set_mac_address = mv_eth_switch_set_mac_addr,
	.ndo_change_mtu = mv_eth_switch_change_mtu,
	.ndo_tx_timeout = mv_eth_tx_timeout,
};

int mv_eth_switch_netdev_first = 0;
int mv_eth_switch_netdev_last = 0;

static inline struct net_device *mv_eth_switch_netdev_get(struct eth_port *pp, struct eth_pbuf *pkt)
{
	MV_U8 *data;
	int db_num;

	if (pp->flags & MV_ETH_F_SWITCH) {
		data = pkt->pBuf + pkt->offset;

		/* bits[4-7] of MSB in Marvell header */
		db_num = ((*data) >> 4);

		return mv_net_devs[mv_eth_switch_netdev_first + db_num];
	}
	return pp->dev;
}

void mv_eth_switch_priv_update(struct net_device *netdev, int i)
{
	struct eth_netdev *dev_priv;
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	int print_flag, port, switch_port;

	/* Update dev_priv structure */
	dev_priv = MV_DEV_PRIV(netdev);
	dev_priv->port_map = 0;
	dev_priv->link_map = 0;

	print_flag = 1;
	for (port = 0; port < BOARD_ETH_SWITCH_PORT_NUM; port++) {
		if (switch_net_config[pp->port].board_port_map[i] & (1 << port)) {
			if (print_flag) {
				printk(KERN_CONT ". Interface ports: ");
				print_flag = 0;
			}
			printk(KERN_CONT "%d ", port);
			switch_port = mvBoardSwitchPortGet(MV_SWITCH_ID_0, port);
			if (switch_port >= 0) {
				dev_priv->port_map |= (1 << switch_port);
				switch_enabled_ports |= (1 << switch_port);
			}
		}
	}
	printk(KERN_CONT "\n");
	dev_priv->group = i;
	dev_priv->vlan_grp_id = MV_SWITCH_GROUP_VLAN_ID(i);	/* e.g. 0x100, 0x200... */
	dev_priv->tx_vlan_mh = cpu_to_be16((i << 12) | dev_priv->port_map);
	dev_priv->cpu_port = mvBoardSwitchCpuPortGet(MV_SWITCH_ID_0);

	mv_eth_switch_vlan_set(dev_priv->vlan_grp_id, dev_priv->port_map, dev_priv->cpu_port);
}

int mv_eth_switch_netdev_init(struct eth_port *pp, int dev_i,
				struct platform_device *pdev)
{
	int i;
	struct net_device *netdev;

	switch_enabled_ports = 0;

	for (i = 0; i < switch_net_config[pp->port].netdev_max; i++) {
		netdev = mv_eth_netdev_init(pp, switch_net_config[pp->port].mtu, switch_net_config[pp->port].mac_addr[i],
						pdev);
		if (netdev == NULL) {
			printk(KERN_ERR "mv_eth_switch_netdev_init: can't create netdevice\n");
			break;
		}
		mv_net_devs[dev_i++] = netdev;

		mv_eth_switch_priv_update(netdev, i);

	}
	return dev_i;
}

#endif /* CONFIG_MV_ETH_SWITCH */

void mv_eth_link_status_print(int port)
{
	MV_ETH_PORT_STATUS link;

	mvNetaLinkStatus(port, &link);
#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(port))
		link.linkup = mv_pon_link_status();
#endif /* CONFIG_MV_PON */

	if (link.linkup) {
		printk(KERN_CONT "link up");
		printk(KERN_CONT ", %s duplex", (link.duplex == MV_ETH_DUPLEX_FULL) ? "full" : "half");
		printk(KERN_CONT ", speed ");

		if (link.speed == MV_ETH_SPEED_1000)
			printk(KERN_CONT "1 Gbps\n");
		else if (link.speed == MV_ETH_SPEED_100)
			printk(KERN_CONT "100 Mbps\n");
		else
			printk(KERN_CONT "10 Mbps\n");
	} else
		printk(KERN_CONT "link down\n");
}

static void mv_eth_rx_error(struct eth_port *pp, struct neta_rx_desc *rx_desc)
{
	STAT_ERR(pp->stats.rx_error++);

	if (pp->dev)
		pp->dev->stats.rx_errors++;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if ((pp->flags & MV_ETH_F_DBG_RX) == 0)
		return;

	if (!printk_ratelimit())
		return;

	if ((rx_desc->status & NETA_RX_FL_DESC_MASK) != NETA_RX_FL_DESC_MASK) {
		printk(KERN_ERR "giga #%d: bad rx status %08x (buffer oversize), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		return;
	}

	switch (rx_desc->status & NETA_RX_ERR_CODE_MASK) {
	case NETA_RX_ERR_CRC:
		printk(KERN_ERR "giga #%d: bad rx status %08x (crc error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	case NETA_RX_ERR_OVERRUN:
		printk(KERN_ERR "giga #%d: bad rx status %08x (overrun error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	case NETA_RX_ERR_LEN:
		printk(KERN_ERR "giga #%d: bad rx status %08x (max frame length error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	case NETA_RX_ERR_RESOURCE:
		printk(KERN_ERR "giga #%d: bad rx status %08x (resource error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	}
	mv_eth_rx_desc_print(rx_desc);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
}

void mv_eth_skb_print(struct sk_buff *skb)
{
	printk(KERN_ERR "skb=%p: head=%p, data=%p, tail=%p, end=%p\n", skb, skb->head, skb->data, skb->tail, skb->end);
	printk(KERN_ERR "\t mac=%p, network=%p, transport=%p\n",
			skb->mac_header, skb->network_header, skb->transport_header);
	printk(KERN_ERR "\t truesize=%d, len=%d, data_len=%d, mac_len=%d\n",
		skb->truesize, skb->len, skb->data_len, skb->mac_len);
	printk(KERN_ERR "\t users=%d, dataref=%d, nr_frags=%d, gso_size=%d, gso_segs=%d\n",
	       atomic_read(&skb->users), atomic_read(&skb_shinfo(skb)->dataref),
	       skb_shinfo(skb)->nr_frags, skb_shinfo(skb)->gso_size, skb_shinfo(skb)->gso_segs);
	printk(KERN_ERR "\t proto=%d, ip_summed=%d, priority=%d\n", ntohs(skb->protocol), skb->ip_summed, skb->priority);
#ifdef CONFIG_NET_SKB_RECYCLE
	printk(KERN_ERR "\t skb_recycle=%p, hw_cookie=%p\n", skb->skb_recycle, skb->hw_cookie);
#endif /* CONFIG_NET_SKB_RECYCLE */
}

void mv_eth_rx_desc_print(struct neta_rx_desc *desc)
{
	int i;
	u32 *words = (u32 *) desc;

	printk(KERN_ERR "RX desc - %p: ", desc);
	for (i = 0; i < 8; i++)
		printk(KERN_CONT "%8.8x ", *words++);
	printk(KERN_CONT "\n");

	if (desc->status & NETA_RX_IP4_FRAG_MASK)
		printk(KERN_ERR "Frag, ");

	printk(KERN_CONT "size=%d, L3_offs=%d, IP_hlen=%d, L4_csum=%s, L3=",
	       desc->dataSize,
	       (desc->status & NETA_RX_L3_OFFSET_MASK) >> NETA_RX_L3_OFFSET_OFFS,
	       (desc->status & NETA_RX_IP_HLEN_MASK) >> NETA_RX_IP_HLEN_OFFS,
	       (desc->status & NETA_RX_L4_CSUM_OK_MASK) ? "Ok" : "Bad");

	if (NETA_RX_L3_IS_IP4(desc->status))
		printk(KERN_CONT "IPv4, ");
	else if (NETA_RX_L3_IS_IP4_ERR(desc->status))
		printk(KERN_CONT "IPv4 bad, ");
	else if (NETA_RX_L3_IS_IP6(desc->status))
		printk(KERN_CONT "IPv6, ");
	else
		printk(KERN_CONT "Unknown, ");

	printk(KERN_CONT "L4=");
	if (NETA_RX_L4_IS_TCP(desc->status))
		printk(KERN_CONT "TCP");
	else if (NETA_RX_L4_IS_UDP(desc->status))
		printk(KERN_CONT "UDP");
	else
		printk(KERN_CONT "Unknown");
	printk(KERN_CONT "\n");

#ifdef CONFIG_MV_ETH_PNC
	printk(KERN_ERR "RINFO: ");
	if (desc->pncInfo & NETA_PNC_DA_MC)
		printk(KERN_CONT "DA_MC, ");
	if (desc->pncInfo & NETA_PNC_DA_BC)
		printk(KERN_CONT "DA_BC, ");
	if (desc->pncInfo & NETA_PNC_DA_UC)
		printk(KERN_CONT "DA_UC, ");
	if (desc->pncInfo & NETA_PNC_VLAN)
		printk(KERN_CONT "VLAN, ");
	if (desc->pncInfo & NETA_PNC_PPPOE)
		printk(KERN_CONT "PPPOE, ");
	if (desc->pncInfo & NETA_PNC_RX_SPECIAL)
		printk(KERN_CONT "RX_SPEC, ");
#endif /* CONFIG_MV_ETH_PNC */

	printk(KERN_CONT "\n");
}
EXPORT_SYMBOL(mv_eth_rx_desc_print);

void mv_eth_tx_desc_print(struct neta_tx_desc *desc)
{
	int i;
	u32 *words = (u32 *) desc;

	printk(KERN_ERR "TX desc - %p: ", desc);
	for (i = 0; i < 8; i++)
		printk(KERN_CONT "%8.8x ", *words++);
	printk(KERN_CONT "\n");
}
EXPORT_SYMBOL(mv_eth_tx_desc_print);

void mv_eth_pkt_print(struct eth_pbuf *pkt)
{
	printk(KERN_ERR "pkt: len=%d off=%d pool=%d "
	       "skb=%p pa=%lx buf=%p\n",
	       pkt->bytes, pkt->offset, pkt->pool,
	       pkt->osInfo, pkt->physAddr, pkt->pBuf);

	mvDebugMemDump(pkt->pBuf + pkt->offset, 64, 1);
	mvOsCacheInvalidate(NULL, pkt->pBuf + pkt->offset, 64);
}
EXPORT_SYMBOL(mv_eth_pkt_print);

static inline void mv_eth_rx_csum(struct eth_port *pp, struct neta_rx_desc *rx_desc, struct sk_buff *skb)
{
#if defined(CONFIG_MV_ETH_RX_CSUM_OFFLOAD)
	if (pp->rx_csum_offload &&
	    ((NETA_RX_L3_IS_IP4(rx_desc->status) ||
	      NETA_RX_L3_IS_IP6(rx_desc->status)) && (rx_desc->status & NETA_RX_L4_CSUM_OK_MASK))) {
		skb->csum = 0;
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		STAT_DBG(pp->stats.rx_csum_hw++);
		return;
	}
#endif /* CONFIG_MV_ETH_RX_CSUM_OFFLOAD */

	skb->ip_summed = CHECKSUM_NONE;
	STAT_DBG(pp->stats.rx_csum_sw++);
}

static inline int mv_eth_tx_done_policy(u32 cause)
{
	return fls(cause >> NETA_CAUSE_TXQ_SENT_DESC_OFFS) - 1;
}

inline int mv_eth_rx_policy(u32 cause)
{
	return fls(cause >> NETA_CAUSE_RXQ_OCCUP_DESC_OFFS) - 1;
}

static inline int mv_eth_txq_tos_map_get(struct eth_port *pp, MV_U8 tos, MV_U8 cpu)
{
	MV_U8 q = pp->cpu_config[cpu]->txq_tos_map[tos];

	if (q == MV_ETH_TXQ_INVALID)
		return pp->cpu_config[smp_processor_id()]->txq;

	return q;
}

static inline int mv_eth_tx_policy(struct eth_port *pp, struct sk_buff *skb)
{
	int txq = pp->cpu_config[smp_processor_id()]->txq;

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph = ip_hdr(skb);

		txq = mv_eth_txq_tos_map_get(pp, iph->tos, smp_processor_id());
	}
	return txq;
}

#ifdef CONFIG_NET_SKB_RECYCLE
int mv_eth_skb_recycle(struct sk_buff *skb)
{
	struct eth_pbuf *pkt = skb->hw_cookie;
	struct bm_pool  *pool;
	int             status = 0;

#if defined(CONFIG_MV_ETH_DEBUG_CODE) && defined(CONFIG_MV_ETH_BM_CPU)
	/* Check that first 4 bytes of the buffer contain hw_cookie */
	if (*((MV_U32 *) skb->head) != (MV_U32)pkt) {
		printk(KERN_ERR "%s: Wrong skb->head=%p (0x%x) != hw_cookie=%p\n",
			__func__, skb->head, *((MV_U32 *) skb->head), pkt);
		mv_eth_skb_print(skb);
		mvDebugMemDump(skb->head, (skb->data - skb->head), 1);
		printk(KERN_ERR "\n");
		goto err;
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE && CONFIG_MV_ETH_BM_CPU */

	/* Check validity of skb->head - some Linux functions (skb_expand_head) reallocate it */
	if (skb->head != pkt->pBuf) {
		printk(KERN_ERR "%s: skb=%p, pkt=%p, Wrong skb->head=%p != pkt->pBuf=%p\n",
			__func__, skb, pkt, skb->head, pkt->pBuf);
		mv_eth_pkt_print(pkt);
		mv_eth_skb_print(skb);
		printk(KERN_ERR "\n");
		goto err;
	}

	pool = &mv_eth_pool[pkt->pool];
	if (skb_recycle_check(skb, pool->pkt_size)) {

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		/* Sanity check */
		if (SKB_TRUESIZE(skb->end - skb->head) != skb->truesize) {
			printk(KERN_ERR "%s: skb=%p, Wrong SKB_TRUESIZE(end - head)=%d\n",
				__func__, skb, SKB_TRUESIZE(skb->end - skb->head));
			mv_eth_skb_print(skb);
		}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		STAT_DBG(pool->stats.skb_recycled_ok++);
		mvOsCacheInvalidate(NULL, skb->head, RX_BUF_SIZE(pool->pkt_size));

		status = mv_eth_pool_put(pool, pkt);

#ifdef ETH_SKB_DEBUG
		if (status == 0)
			mv_eth_skb_save(skb, "recycle");
#endif /* ETH_SKB_DEBUG */

		return MV_OK;
	}
	STAT_DBG(pool->stats.skb_recycled_err++);

	/* printk(KERN_ERR "mv_eth_skb_recycle failed: pool=%d, pkt=%p, skb=%p\n", pkt->pool, pkt, skb); */
err:
	mvOsFree(pkt);
	skb->hw_cookie = NULL;

	return 1;
}
EXPORT_SYMBOL(mv_eth_skb_recycle);

#endif /* CONFIG_NET_SKB_RECYCLE */

static struct sk_buff *mv_eth_skb_alloc(struct bm_pool *pool, struct eth_pbuf *pkt, gfp_t gfp_mask)
{
	struct sk_buff *skb;

	skb = __dev_alloc_skb(pool->pkt_size, gfp_mask);
	if (!skb) {
		STAT_ERR(pool->stats.skb_alloc_oom++);
		return NULL;
	}
	STAT_DBG(pool->stats.skb_alloc_ok++);

#ifdef ETH_SKB_DEBUG
	mv_eth_skb_save(skb, "alloc");
#endif /* ETH_SKB_DEBUG */

#ifdef CONFIG_MV_ETH_BM_CPU
	/* Save pkt as first 4 bytes in the buffer */
#if !defined(CONFIG_MV_ETH_BE_WA)
	*((MV_U32 *) skb->head) = MV_32BIT_LE((MV_U32)pkt);
#else
	*((MV_U32 *) skb->head) = (MV_U32)pkt;
#endif /* !CONFIG_MV_ETH_BE_WA */
	mvOsCacheLineFlush(NULL, skb->head);
#endif /* CONFIG_MV_ETH_BM_CPU */

	pkt->osInfo = (void *)skb;
	pkt->pBuf = skb->head;
	pkt->bytes = 0;
	pkt->physAddr = mvOsCacheInvalidate(NULL, skb->head, RX_BUF_SIZE(pool->pkt_size));
	pkt->offset = NET_SKB_PAD;
	pkt->pool = pool->pool;

	return skb;
}

static inline void mv_eth_txq_bufs_free(struct eth_port *pp, struct tx_queue *txq_ctrl, int num)
{
	u32 shadow;
	int i;

	/* Free buffers that was not freed automatically by BM */
	for (i = 0; i < num; i++) {
		shadow = txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_get_i];
		mv_eth_shadow_inc_get(txq_ctrl);

		if (!shadow)
			continue;

		if (shadow & MV_ETH_SHADOW_SKB) {
			shadow &= ~MV_ETH_SHADOW_SKB;
			dev_kfree_skb_any((struct sk_buff *)shadow);
			STAT_DBG(pp->stats.tx_skb_free++);
		} else {
			if (shadow & MV_ETH_SHADOW_EXT) {
				shadow &= ~MV_ETH_SHADOW_EXT;
				mv_eth_extra_pool_put(pp, (void *)shadow);
			} else {
				/* packet from NFP without BM */
				struct eth_pbuf *pkt = (struct eth_pbuf *)shadow;
				struct bm_pool *pool = &mv_eth_pool[pkt->pool];

				if (mv_eth_pool_bm(pool)) {
					/* Refill BM pool */
					STAT_DBG(pool->stats.bm_put++);
					mvBmPoolPut(pkt->pool, (MV_ULONG) pkt->physAddr);
				} else {
					mv_eth_pool_put(pool, pkt);
				}
			}
		}
	}
}

inline u32 mv_eth_txq_done(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int tx_done;

	tx_done = mvNetaTxqSentDescProc(pp->port, txq_ctrl->txp, txq_ctrl->txq);
	if (!tx_done)
		return tx_done;
/*
	printk(KERN_ERR "tx_done: txq_count=%d, port=%d, txp=%d, txq=%d, tx_done=%d\n",
			txq_ctrl->txq_count, pp->port, txq_ctrl->txp, txq_ctrl->txq, tx_done);
*/
	if (!mv_eth_txq_bm(txq_ctrl))
		mv_eth_txq_bufs_free(pp, txq_ctrl, tx_done);

	txq_ctrl->txq_count -= tx_done;
	STAT_DBG(txq_ctrl->stats.txq_txdone += tx_done);

	return tx_done;
}
EXPORT_SYMBOL(mv_eth_txq_done);

inline struct eth_pbuf *mv_eth_pool_get(struct bm_pool *pool)
{
	struct eth_pbuf *pkt = NULL;
	struct sk_buff *skb;
	unsigned long flags = 0;

	MV_ETH_LOCK(&pool->lock, flags);

	if (mvStackIndex(pool->stack) > 0) {
		STAT_DBG(pool->stats.stack_get++);
		pkt = (struct eth_pbuf *)mvStackPop(pool->stack);
	} else
		STAT_ERR(pool->stats.stack_empty++);

	MV_ETH_UNLOCK(&pool->lock, flags);
	if (pkt)
		return pkt;

	/* Try to allocate new pkt + skb */
	pkt = mvOsMalloc(sizeof(struct eth_pbuf));
	if (pkt) {
		skb = mv_eth_skb_alloc(pool, pkt, GFP_ATOMIC);
		if (!skb) {
			mvOsFree(pkt);
			pkt = NULL;
		}
	}
	return pkt;
}

/* Reuse pkt if possible, allocate new skb and move BM pool or RXQ ring */
inline int mv_eth_refill(struct eth_port *pp, int rxq,
				struct eth_pbuf *pkt, struct bm_pool *pool, struct neta_rx_desc *rx_desc)
{
	if (pkt == NULL) {
		pkt = mv_eth_pool_get(pool);
		if (pkt == NULL)
			return 1;
	} else {
		struct sk_buff *skb;

		/* No recycle -  alloc new skb */
		skb = mv_eth_skb_alloc(pool, pkt, GFP_ATOMIC);
		if (!skb) {
			mvOsFree(pkt);
			pool->missed++;
			mv_eth_add_cleanup_timer(pp->cpu_config[smp_processor_id()]);
			return 1;
		}
	}
	mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);

	return MV_OK;
}
EXPORT_SYMBOL(mv_eth_refill);

static inline MV_U32 mv_eth_skb_tx_csum(struct eth_port *pp, struct sk_buff *skb)
{
#ifdef CONFIG_MV_ETH_TX_CSUM_OFFLOAD
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		int   ip_hdr_len = 0;
		MV_U8 l4_proto;

		if (skb->protocol == htons(ETH_P_IP)) {
			struct iphdr *ip4h = ip_hdr(skb);

			/* Calculate IPv4 checksum and L4 checksum */
			ip_hdr_len = ip4h->ihl;
			l4_proto = ip4h->protocol;
		} else if (skb->protocol == htons(ETH_P_IPV6)) {
			/* If not IPv4 - must be ETH_P_IPV6 - Calculate only L4 checksum */
			struct ipv6hdr *ip6h = ipv6_hdr(skb);

			/* Read l4_protocol from one of IPv6 extra headers ?????? */
			if (skb_network_header_len(skb) > 0)
				ip_hdr_len = (skb_network_header_len(skb) >> 2);
			l4_proto = ip6h->nexthdr;
		} else {
			STAT_DBG(pp->stats.tx_csum_sw++);
			return NETA_TX_L4_CSUM_NOT;
		}
		STAT_DBG(pp->stats.tx_csum_hw++);

		return mvNetaTxqDescCsum(skb_network_offset(skb), skb->protocol, ip_hdr_len, l4_proto);
	}
#endif /* CONFIG_MV_ETH_TX_CSUM_OFFLOAD */

	STAT_DBG(pp->stats.tx_csum_sw++);
	return NETA_TX_L4_CSUM_NOT;
}

#ifdef CONFIG_MV_ETH_RX_DESC_PREFETCH
inline struct neta_rx_desc *mv_eth_rx_prefetch(struct eth_port *pp, MV_NETA_RXQ_CTRL *rx_ctrl,
									  int rx_done, int rx_todo)
{
	struct neta_rx_desc	*rx_desc, *next_desc;

	rx_desc = mvNetaRxqNextDescGet(rx_ctrl);
	if (rx_done == 0) {
		/* First descriptor in the NAPI loop */
		mvOsCacheLineInv(NULL, rx_desc);
		prefetch(rx_desc);
	}
	if ((rx_done + 1) == rx_todo) {
		/* Last descriptor in the NAPI loop - prefetch are not needed */
		return rx_desc;
	}
	/* Prefetch next descriptor */
	next_desc = mvNetaRxqDescGet(rx_ctrl);
	mvOsCacheLineInv(NULL, next_desc);
	prefetch(next_desc);

	return rx_desc;
}
#endif /* CONFIG_MV_ETH_RX_DESC_PREFETCH */

static inline int mv_eth_rx(struct eth_port *pp, int rx_todo, int rxq)
{
	struct net_device *dev;
	MV_NETA_RXQ_CTRL *rx_ctrl = pp->rxq_ctrl[rxq].q;
	int rx_done, rx_filled, err;
	struct neta_rx_desc *rx_desc;
	u32 rx_status;
	int rx_bytes;
	struct eth_pbuf *pkt;
	struct sk_buff *skb;
	struct bm_pool *pool;

	/* Get number of received packets */
	rx_done = mvNetaRxqBusyDescNumGet(pp->port, rxq);
	mvOsCacheIoSync();

	if (rx_todo > rx_done)
		rx_todo = rx_done;

	rx_done = 0;
	rx_filled = 0;

	/* Fairness NAPI loop */
	while (rx_done < rx_todo) {

#ifdef CONFIG_MV_ETH_RX_DESC_PREFETCH
		rx_desc = mv_eth_rx_prefetch(pp, rx_ctrl, rx_done, rx_todo);
#else
		rx_desc = mvNetaRxqNextDescGet(rx_ctrl);
		mvOsCacheLineInv(NULL, rx_desc);
		prefetch(rx_desc);
#endif /* CONFIG_MV_ETH_RX_DESC_PREFETCH */

		rx_done++;
		rx_filled++;

#if defined(MV_CPU_BE)
		mvNetaRxqDescSwap(rx_desc);
#endif /* MV_CPU_BE */

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->flags & MV_ETH_F_DBG_RX) {
			printk(KERN_ERR "\n%s: port=%d, cpu=%d\n", __func__, pp->port, smp_processor_id());
			mv_eth_rx_desc_print(rx_desc);
		}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		rx_status = rx_desc->status;
		pkt = (struct eth_pbuf *)rx_desc->bufCookie;
		pool = &mv_eth_pool[pkt->pool];

		if (((rx_status & NETA_RX_FL_DESC_MASK) != NETA_RX_FL_DESC_MASK) ||
			(rx_status & NETA_RX_ES_MASK)) {

			mv_eth_rx_error(pp, rx_desc);

			mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
			continue;
		}

		/* Speculative ICache prefetch WA: should be replaced with dma_unmap_single (invalidate l2) */
		mvOsCacheMultiLineInv(NULL, pkt->pBuf + pkt->offset, rx_desc->dataSize);

#ifdef CONFIG_MV_ETH_RX_PKT_PREFETCH
		prefetch(pkt->pBuf + pkt->offset);
		prefetch(pkt->pBuf + pkt->offset + CPU_D_CACHE_LINE_SIZE);
#endif /* CONFIG_MV_ETH_RX_PKT_PREFETCH */

#ifdef CONFIG_MV_ETH_SWITCH
		dev = mv_eth_switch_netdev_get(pp, pkt);
#else
		dev = pp->dev;
#endif /* CONFIG_MV_ETH_SWITCH */
		STAT_DBG(pp->stats.rxq[rxq]++);
		dev->stats.rx_packets++;

		rx_bytes = rx_desc->dataSize - (MV_ETH_CRC_SIZE + MV_ETH_MH_SIZE);
		dev->stats.rx_bytes += rx_bytes;

#ifndef CONFIG_MV_ETH_PNC
	/* Update IP offset and IP header len in RX descriptor */
	if (NETA_RX_L3_IS_IP4(rx_desc->status)) {
		int ip_offset;

		if ((rx_desc->status & ETH_RX_VLAN_TAGGED_FRAME_MASK))
			ip_offset = MV_ETH_MH_SIZE + sizeof(MV_802_3_HEADER) + MV_VLAN_HLEN;
		else
			ip_offset = MV_ETH_MH_SIZE + sizeof(MV_802_3_HEADER);

		NETA_RX_SET_IPHDR_OFFSET(rx_desc, ip_offset);
		NETA_RX_SET_IPHDR_HDRLEN(rx_desc, 5);
	}
#endif /* !CONFIG_MV_ETH_PNC */

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->flags & MV_ETH_F_DBG_RX) {
			printk(KERN_ERR "pkt=%p, pBuf=%p, ksize=%d\n", pkt, pkt->pBuf, ksize(pkt->pBuf));
			mvDebugMemDump(pkt->pBuf + pkt->offset, 64, 1);
		}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

#if defined(CONFIG_MV_ETH_PNC) && defined(CONFIG_MV_ETH_RX_SPECIAL)
		/* Special RX processing */
		if (rx_desc->pncInfo & NETA_PNC_RX_SPECIAL) {
			if (pp->rx_special_proc) {
				pp->rx_special_proc(pp->port, rxq, dev, (struct sk_buff *)(pkt->osInfo), rx_desc);
				STAT_INFO(pp->stats.rx_special++);

				/* Refill processing */
				err = mv_eth_refill(pp, rxq, pkt, pool, rx_desc);
				if (err) {
					printk(KERN_ERR "Linux processing - Can't refill\n");
					pp->rxq_ctrl[rxq].missed++;
					rx_filled--;
				}
				continue;
			}
		}
#endif /* CONFIG_MV_ETH_PNC && CONFIG_MV_ETH_RX_SPECIAL */

#if defined(CONFIG_MV_ETH_NFP)
		if (pp->flags & MV_ETH_F_NFP_EN) {
			MV_STATUS status;

			pkt->bytes = rx_bytes + MV_ETH_MH_SIZE;
			pkt->offset = NET_SKB_PAD;

			status = mv_eth_nfp(pp, rxq, rx_desc, pkt, pool);
			if (status == MV_OK)
				continue;
			if (status == MV_FAIL) {
				rx_filled--;
				continue;
			}
			/* MV_TERMINATE - packet returned to slow path */
		}
#endif /* CONFIG_MV_ETH_NFP */

		/* Linux processing */
		skb = (struct sk_buff *)(pkt->osInfo);

		skb->data += MV_ETH_MH_SIZE;
		skb->tail += (rx_bytes + MV_ETH_MH_SIZE);
		skb->len = rx_bytes;

#ifdef ETH_SKB_DEBUG
		mv_eth_skb_check(skb);
#endif /* ETH_SKB_DEBUG */

		skb->protocol = eth_type_trans(skb, dev);

#ifdef CONFIG_NET_SKB_RECYCLE
		if (mv_eth_is_recycle()) {
			skb->skb_recycle = mv_eth_skb_recycle;
			skb->hw_cookie = pkt;
			pkt = NULL;
		}
#endif /* CONFIG_NET_SKB_RECYCLE */

		if (skb)
			mv_eth_rx_csum(pp, rx_desc, skb);

#ifdef CONFIG_MV_ETH_GRO
		if (skb && (dev->features & NETIF_F_GRO)) {
			STAT_DBG(pp->stats.rx_gro++);
			STAT_DBG(pp->stats.rx_gro_bytes += skb->len);

			rx_status = napi_gro_receive(pp->cpu_config[smp_processor_id()]->napi, skb);
			skb = NULL;
		}
#endif /* CONFIG_MV_ETH_GRO */

		if (skb) {
			STAT_DBG(pp->stats.rx_netif++);
			rx_status = netif_receive_skb(skb);
			STAT_DBG((rx_status == 0) ? 0 : pp->stats.rx_drop_sw++);
		}

		/* Refill processing: */
		err = mv_eth_refill(pp, rxq, pkt, pool, rx_desc);
		if (err) {
			printk(KERN_ERR "Linux processing - Can't refill\n");
			pp->rxq_ctrl[rxq].missed++;
			mv_eth_add_cleanup_timer(pp->cpu_config[smp_processor_id()]);
			rx_filled--;
		}
	}

	/* Update RxQ management counters */
	mvOsCacheIoSync();
	mvNetaRxqDescNumUpdate(pp->port, rxq, rx_done, rx_filled);

	return rx_done;
}

static int mv_eth_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	int frags = 0;
	bool tx_spec_ready = false;
	struct mv_eth_tx_spec tx_spec;
	u32 tx_cmd;
	u16 mh;
	struct tx_queue *txq_ctrl = NULL;
	struct neta_tx_desc *tx_desc;
	unsigned long flags = 0;

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->flags & MV_ETH_F_DBG_TX)
			printk(KERN_ERR "%s: STARTED_BIT = 0, packet is dropped.\n", __func__);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
		goto out;
	}

	if (!(netif_running(dev))) {
		printk(KERN_ERR "!netif_running() in %s\n", __func__);
		goto out;
	}

#if defined(CONFIG_MV_ETH_TX_SPECIAL)
	if (pp->tx_special_check) {

		if (pp->tx_special_check(pp->port, dev, skb, &tx_spec)) {
			STAT_INFO(pp->stats.tx_special++);
			if (tx_spec.tx_func) {
				tx_spec.tx_func(skb->data, skb->len, &tx_spec);
				goto out;
			} else {
				/* Check validity of tx_spec txp/txq must be CPU owned */
				tx_spec_ready = true;
			}
		}
	}
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

	/* Get TXQ (without BM) to send packet generated by Linux */
	if (tx_spec_ready == false) {
		tx_spec.txp = pp->txp;
		tx_spec.txq = mv_eth_tx_policy(pp, skb);
		tx_spec.hw_cmd = pp->hw_cmd;
		tx_spec.flags = pp->flags;
	}

	txq_ctrl = &pp->txq_ctrl[tx_spec.txp * CONFIG_MV_ETH_TXQ + tx_spec.txq];
	if (txq_ctrl == NULL) {
		printk(KERN_ERR "%s: invalidate txp/txq (%d/%d)\n", __func__, tx_spec.txp, tx_spec.txq);
		goto out;
	}
	mv_eth_lock(txq_ctrl, flags);

#ifdef CONFIG_MV_ETH_TSO
	/* GSO/TSO */
	if (skb_is_gso(skb)) {
		frags = mv_eth_tx_tso(skb, dev, &tx_spec, txq_ctrl);
		goto out;
	}
#endif /* CONFIG_MV_ETH_TSO */

	frags = skb_shinfo(skb)->nr_frags + 1;

	if (tx_spec.flags & MV_ETH_F_MH) {
		if (tx_spec.flags & MV_ETH_F_SWITCH)
			mh = dev_priv->tx_vlan_mh;
		else
			mh = pp->tx_mh;

		if (mv_eth_skb_mh_add(skb, mh)) {
			frags = 0;
			goto out;
		}
	}

	tx_desc = mv_eth_tx_desc_get(txq_ctrl, frags);
	if (tx_desc == NULL) {
		frags = 0;
		goto out;
	}

	/* Don't use BM for Linux packets: NETA_TX_BM_ENABLE_MASK = 0 */
	/* NETA_TX_PKT_OFFSET_MASK = 0 - for all descriptors */
	tx_cmd = mv_eth_skb_tx_csum(pp, skb);

#ifdef CONFIG_MV_PON
	tx_desc->hw_cmd = tx_spec.hw_cmd;
#endif

	/* FIXME: beware of nonlinear --BK */
	tx_desc->dataSize = skb_headlen(skb);

	tx_desc->bufPhysAddr = mvOsCacheFlush(NULL, skb->data, tx_desc->dataSize);

	if (frags == 1) {
		/*
		 * First and Last descriptor
		 */
		if (tx_spec.flags & MV_ETH_F_NO_PAD)
			tx_cmd |= NETA_TX_F_DESC_MASK | NETA_TX_L_DESC_MASK;
		else
			tx_cmd |= NETA_TX_FLZ_DESC_MASK;

		tx_desc->command = tx_cmd;
		mv_eth_tx_desc_flush(tx_desc);

		txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = ((MV_ULONG) skb | MV_ETH_SHADOW_SKB);
		mv_eth_shadow_inc_put(txq_ctrl);
	} else {

		/* First but not Last */
		tx_cmd |= NETA_TX_F_DESC_MASK;

		txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = 0;
		mv_eth_shadow_inc_put(txq_ctrl);

		tx_desc->command = tx_cmd;
		mv_eth_tx_desc_flush(tx_desc);

		/* Continue with other skb fragments */
		mv_eth_tx_frag_process(pp, skb, txq_ctrl, tx_spec.flags);
		STAT_DBG(pp->stats.tx_sg++);
	}
/*
	printk(KERN_ERR "tx: frags=%d, tx_desc[0x0]=%x [0xc]=%x, wr_id=%d, rd_id=%d, skb=%p\n",
			frags, tx_desc->command,tx_desc->hw_cmd,
			txq_ctrl->shadow_txq_put_i, txq_ctrl->shadow_txq_get_i, skb);
*/
	txq_ctrl->txq_count += frags;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->flags & MV_ETH_F_DBG_TX) {
		printk(KERN_ERR "\n");
		printk(KERN_ERR "%s - eth_tx_%lu: cpu=%d, in_intr=0x%lx, port=%d, txp=%d, txq=%d\n",
		       dev->name, dev->stats.tx_packets, smp_processor_id(),
			in_interrupt(), pp->port, tx_spec.txp, tx_spec.txq);
		printk(KERN_ERR "\t skb=%p, head=%p, data=%p, size=%d\n", skb, skb->head, skb->data, skb->len);
		mv_eth_tx_desc_print(tx_desc);
		/*mv_eth_skb_print(skb);*/
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(pp->port))
		mvNetaPonTxqBytesAdd(pp->port, tx_spec.txp, tx_spec.txq, skb->len);
#endif /* CONFIG_MV_PON */

	/* Enable transmit */
	mvNetaTxqPendDescAdd(pp->port, tx_spec.txp, tx_spec.txq, frags);

	STAT_DBG(txq_ctrl->stats.txq_tx += frags);

out:
	if (frags > 0) {
		dev->stats.tx_packets++;
		dev->stats.tx_bytes += skb->len;
	} else {
		dev->stats.tx_dropped++;
		dev_kfree_skb_any(skb);
	}

#ifndef CONFIG_MV_ETH_TXDONE_ISR
	if (txq_ctrl) {
		if (txq_ctrl->txq_count >= mv_ctrl_txdone) {
			u32 tx_done = mv_eth_txq_done(pp, txq_ctrl);

			STAT_DIST((tx_done < pp->dist_stats.tx_done_dist_size) ? pp->dist_stats.tx_done_dist[tx_done]++ : 0);

		}
		/* If after calling mv_eth_txq_done, txq_ctrl->txq_count equals frags, we need to set the timer */
		if ((txq_ctrl->txq_count == frags) && (frags > 0)) {
			struct cpu_ctrl *cpuCtrl = pp->cpu_config[smp_processor_id()];

			mv_eth_add_tx_done_timer(cpuCtrl);
		}
	}
#endif /* CONFIG_MV_ETH_TXDONE_ISR */

	if (txq_ctrl)
		mv_eth_unlock(txq_ctrl, flags);

	return NETDEV_TX_OK;
}

#ifdef CONFIG_MV_ETH_TSO
/* Validate TSO */
static inline int mv_eth_tso_validate(struct sk_buff *skb, struct net_device *dev)
{
	if (!(dev->features & NETIF_F_TSO)) {
		printk(KERN_ERR "error: (skb_is_gso(skb) returns true but features is not NETIF_F_TSO\n");
		return 1;
	}

	if (skb_shinfo(skb)->frag_list != NULL) {
		printk(KERN_ERR "***** ERROR: frag_list is not null\n");
		return 1;
	}

	if (skb_shinfo(skb)->gso_segs == 1) {
		printk(KERN_ERR "***** ERROR: only one TSO segment\n");
		return 1;
	}

	if (skb->len <= skb_shinfo(skb)->gso_size) {
		printk(KERN_ERR "***** ERROR: total_len (%d) less than gso_size (%d)\n", skb->len, skb_shinfo(skb)->gso_size);
		return 1;
	}
	if ((htons(ETH_P_IP) != skb->protocol) || (ip_hdr(skb)->protocol != IPPROTO_TCP) || (tcp_hdr(skb) == NULL)) {
		printk(KERN_ERR "***** ERROR: Protocol is not TCP over IP\n");
		return 1;
	}
	return MV_OK;
}

static inline int mv_eth_tso_build_hdr_desc(struct neta_tx_desc *tx_desc, struct eth_port *priv, struct sk_buff *skb,
					     struct tx_queue *txq_ctrl, u16 *mh, int hdr_len, int size,
					     MV_U32 tcp_seq, MV_U16 ip_id, int left_len)
{
	struct iphdr *iph;
	struct tcphdr *tcph;
	MV_U8 *data, *mac;
	int mac_hdr_len = skb_network_offset(skb);

	data = mv_eth_extra_pool_get(priv);
	if (!data)
		return MV_OK;

	txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = ((MV_ULONG)data | MV_ETH_SHADOW_EXT);

	/* Reserve 2 bytes for IP header alignment */
	mac = data + MV_ETH_MH_SIZE;
	iph = (struct iphdr *)(mac + mac_hdr_len);

	memcpy(mac, skb->data, hdr_len);

	if (iph) {
		iph->id = htons(ip_id);
		iph->tot_len = htons(size + hdr_len - mac_hdr_len);
	}

	tcph = (struct tcphdr *)(mac + skb_transport_offset(skb));
	tcph->seq = htonl(tcp_seq);

	if (left_len) {
		/* Clear all special flags for not last packet */
		tcph->psh = 0;
		tcph->fin = 0;
		tcph->rst = 0;
	}

	if (mh) {
		/* Start tarnsmit from MH - add 2 bytes to size */
		*((MV_U16 *)data) = *mh;
		/* increment ip_offset field in TX descriptor by 2 bytes */
		mac_hdr_len += MV_ETH_MH_SIZE;
		hdr_len += MV_ETH_MH_SIZE;
	} else {
		/* Start transmit from MAC */
		data = mac;
	}

	tx_desc->dataSize = hdr_len;
	tx_desc->command = mvNetaTxqDescCsum(mac_hdr_len, skb->protocol, ((u8 *)tcph - (u8 *)iph) >> 2, IPPROTO_TCP);
	tx_desc->command |= NETA_TX_F_DESC_MASK;

	tx_desc->bufPhysAddr = mvOsCacheFlush(NULL, data, tx_desc->dataSize);
	mv_eth_shadow_inc_put(txq_ctrl);

	mv_eth_tx_desc_flush(tx_desc);

	return hdr_len;
}

static inline int mv_eth_tso_build_data_desc(struct neta_tx_desc *tx_desc, struct sk_buff *skb,
					     struct tx_queue *txq_ctrl, char *frag_ptr,
					     int frag_size, int data_left, int total_left)
{
	int size;

	size = MV_MIN(frag_size, data_left);

	tx_desc->dataSize = size;
	tx_desc->bufPhysAddr = mvOsCacheFlush(NULL, frag_ptr, size);
	tx_desc->command = 0;
	txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = 0;

	if (size == data_left) {
		/* last descriptor in the TCP packet */
		tx_desc->command = NETA_TX_L_DESC_MASK;

		if (total_left == 0) {
			/* last descriptor in SKB */
			txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = ((MV_ULONG) skb | MV_ETH_SHADOW_SKB);
		}
	}
	mv_eth_shadow_inc_put(txq_ctrl);
	mv_eth_tx_desc_flush(tx_desc);

	return size;
}

/***********************************************************
 * mv_eth_tx_tso --                                        *
 *   send a packet.                                        *
 ***********************************************************/
int mv_eth_tx_tso(struct sk_buff *skb, struct net_device *dev,
		struct mv_eth_tx_spec *tx_spec, struct tx_queue *txq_ctrl)
{
	int frag = 0;
	int total_len, hdr_len, size, frag_size, data_left;
	char *frag_ptr;
	int totalDescNum, totalBytes = 0;
	struct neta_tx_desc *tx_desc;
	MV_U16 ip_id;
	MV_U32 tcp_seq = 0;
	skb_frag_t *skb_frag_ptr;
	const struct tcphdr *th = tcp_hdr(skb);
	struct eth_port *priv = MV_ETH_PRIV(dev);
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	MV_U16 *mh = NULL;
	int i;

	STAT_DBG(priv->stats.tx_tso++);
/*
	printk(KERN_ERR "mv_eth_tx_tso_%d ENTER: skb=%p, total_len=%d\n", priv->stats.tx_tso, skb, skb->len);
*/
	if (mv_eth_tso_validate(skb, dev))
		return MV_OK;

	/* Calculate expected number of TX descriptors */
	totalDescNum = skb_shinfo(skb)->gso_segs * 2 + skb_shinfo(skb)->nr_frags;

	if ((txq_ctrl->txq_count + totalDescNum) >= txq_ctrl->txq_size) {
/*
		printk(KERN_ERR "%s: no TX descriptors - txq_count=%d, len=%d, nr_frags=%d, gso_segs=%d\n",
					__func__, txq_ctrl->txq_count, skb->len, skb_shinfo(skb)->nr_frags,
					skb_shinfo(skb)->gso_segs);
*/
		STAT_ERR(txq_ctrl->stats.txq_err++);
		return MV_OK;
	}

	total_len = skb->len;
	hdr_len = (skb_transport_offset(skb) + tcp_hdrlen(skb));

	total_len -= hdr_len;
	ip_id = ntohs(ip_hdr(skb)->id);
	tcp_seq = ntohl(th->seq);

	frag_size = skb_headlen(skb);
	frag_ptr = skb->data;

	if (frag_size < hdr_len) {
		printk(KERN_ERR "***** ERROR: frag_size=%d, hdr_len=%d\n", frag_size, hdr_len);
		return MV_OK;
	}

	frag_size -= hdr_len;
	frag_ptr += hdr_len;
	if (frag_size == 0) {
		skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

		/* Move to next segment */
		frag_size = skb_frag_ptr->size;
		frag_ptr = page_address(skb_frag_ptr->page.p) + skb_frag_ptr->page_offset;
		frag++;
	}
	totalDescNum = 0;

	while (total_len > 0) {
		data_left = MV_MIN(skb_shinfo(skb)->gso_size, total_len);

		tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
		if (tx_desc == NULL)
			goto outNoTxDesc;

		totalDescNum++;
		total_len -= data_left;
		txq_ctrl->txq_count++;

		if (tx_spec->flags & MV_ETH_F_MH) {
			if (tx_spec->flags & MV_ETH_F_SWITCH)
				mh = &dev_priv->tx_vlan_mh;
			else
				mh = &priv->tx_mh;
		}

		/* prepare packet headers: MAC + IP + TCP */
		size = mv_eth_tso_build_hdr_desc(tx_desc, priv, skb, txq_ctrl, mh,
					hdr_len, data_left, tcp_seq, ip_id, total_len);
		if (size == 0)
			goto outNoTxDesc;

		totalBytes += size;
/*
		printk(KERN_ERR "Header desc: tx_desc=%p, skb=%p, hdr_len=%d, data_left=%d\n",
						tx_desc, skb, hdr_len, data_left);
*/
		ip_id++;

		while (data_left > 0) {
			tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
			if (tx_desc == NULL)
				goto outNoTxDesc;

			totalDescNum++;
			txq_ctrl->txq_count++;

			size = mv_eth_tso_build_data_desc(tx_desc, skb, txq_ctrl,
							  frag_ptr, frag_size, data_left, total_len);
			totalBytes += size;
/*
			printk(KERN_ERR "Data desc: tx_desc=%p, skb=%p, size=%d, frag_size=%d, data_left=%d\n",
							tx_desc, skb, size, frag_size, data_left);
 */
			data_left -= size;
			tcp_seq += size;

			frag_size -= size;
			frag_ptr += size;

			if ((frag_size == 0) && (frag < skb_shinfo(skb)->nr_frags)) {
				skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

				/* Move to next segment */
				frag_size = skb_frag_ptr->size;
				frag_ptr = page_address(skb_frag_ptr->page.p) + skb_frag_ptr->page_offset;
				frag++;
			}
		}		/* of while data_left > 0 */
	}			/* of while (total_len > 0) */

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(priv->port))
		mvNetaPonTxqBytesAdd(priv->port, txq_ctrl->txp, txq_ctrl->txq, totalBytes);
#endif /* CONFIG_MV_PON */

	STAT_DBG(priv->stats.tx_tso_bytes += totalBytes);
	STAT_DBG(txq_ctrl->stats.txq_tx += totalDescNum);

	mvNetaTxqPendDescAdd(priv->port, txq_ctrl->txp, txq_ctrl->txq, totalDescNum);
/*
	printk(KERN_ERR "mv_eth_tx_tso EXIT: totalDescNum=%d\n", totalDescNum);
*/
	return totalDescNum;

outNoTxDesc:
	/* No enough TX descriptors for the whole skb - rollback */
	printk(KERN_ERR "%s: No TX descriptors - rollback %d, txq_count=%d, nr_frags=%d, skb=%p, len=%d, gso_segs=%d\n",
			__func__, totalDescNum, txq_ctrl->txq_count, skb_shinfo(skb)->nr_frags,
			skb, skb->len, skb_shinfo(skb)->gso_segs);

	for (i = 0; i < totalDescNum; i++) {
		txq_ctrl->txq_count--;
		mv_eth_shadow_dec_put(txq_ctrl);
		mvNetaTxqPrevDescGet(txq_ctrl->q);
	}
	return MV_OK;
}
#endif /* CONFIG_MV_ETH_TSO */

/* Drop packets received by the RXQ and free buffers */
static void mv_eth_rxq_drop_pkts(struct eth_port *pp, int rxq)
{
	struct neta_rx_desc *rx_desc;
	struct eth_pbuf     *pkt;
	struct bm_pool      *pool;
	int	                rx_done, i;
	MV_NETA_RXQ_CTRL    *rx_ctrl = pp->rxq_ctrl[rxq].q;

	if (rx_ctrl == NULL)
		return;

	rx_done = mvNetaRxqBusyDescNumGet(pp->port, rxq);
	mvOsCacheIoSync();

	for (i = 0; i < rx_done; i++) {
		rx_desc = mvNetaRxqNextDescGet(rx_ctrl);
		mvOsCacheLineInv(NULL, rx_desc);

#if defined(MV_CPU_BE)
		mvNetaRxqDescSwap(rx_desc);
#endif /* MV_CPU_BE */

		pkt = (struct eth_pbuf *)rx_desc->bufCookie;
		pool = &mv_eth_pool[pkt->pool];
		mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
	}
	if (rx_done) {
		mvOsCacheIoSync();
		mvNetaRxqDescNumUpdate(pp->port, rxq, rx_done, rx_done);
	}
}

static void mv_eth_txq_done_force(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int tx_done = txq_ctrl->txq_count;

	mv_eth_txq_bufs_free(pp, txq_ctrl, tx_done);

	STAT_DBG(txq_ctrl->stats.txq_txdone += tx_done);

	/* reset txq */
	txq_ctrl->txq_count = 0;
	txq_ctrl->shadow_txq_put_i = 0;
	txq_ctrl->shadow_txq_get_i = 0;
}

inline u32 mv_eth_tx_done_pon(struct eth_port *pp, int *tx_todo)
{
	int txp, txq;
	struct tx_queue *txq_ctrl;
	unsigned long flags = 0;

	u32 tx_done = 0;

	*tx_todo = 0;

	STAT_INFO(pp->stats.tx_done++);

	/* simply go over all TX ports and TX queues */
	txp = pp->txp_num;
	while (txp--) {
		txq = CONFIG_MV_ETH_TXQ;

		while (txq--) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
			mv_eth_lock(txq_ctrl, flags);
			if ((txq_ctrl) && (txq_ctrl->txq_count)) {
				tx_done += mv_eth_txq_done(pp, txq_ctrl);
				*tx_todo += txq_ctrl->txq_count;
			}
			mv_eth_unlock(txq_ctrl, flags);
		}
	}

	STAT_DIST((tx_done < pp->dist_stats.tx_done_dist_size) ? pp->dist_stats.tx_done_dist[tx_done]++ : 0);

	return tx_done;
}

inline u32 mv_eth_tx_done_gbe(struct eth_port *pp, u32 cause_tx_done, int *tx_todo)
{
	int txq;
	struct tx_queue *txq_ctrl;
	unsigned long flags = 0;
	u32 tx_done = 0;

	*tx_todo = 0;

	STAT_INFO(pp->stats.tx_done++);

	while (cause_tx_done != 0) {

		/* For GbE ports we get TX Buffers Threshold Cross per queue in bits [7:0] */
		txq = mv_eth_tx_done_policy(cause_tx_done);

		if (txq == -1)
			break;

		txq_ctrl = &pp->txq_ctrl[txq];

		if (txq_ctrl == NULL) {
			printk(KERN_ERR "%s: txq_ctrl = NULL, txq=%d\n", __func__, txq);
			return -EINVAL;
		}

		mv_eth_lock(txq_ctrl, flags);

		if ((txq_ctrl) && (txq_ctrl->txq_count)) {
			tx_done += mv_eth_txq_done(pp, txq_ctrl);
			*tx_todo += txq_ctrl->txq_count;
		}
		cause_tx_done &= ~((1 << txq) << NETA_CAUSE_TXQ_SENT_DESC_OFFS);

		mv_eth_unlock(txq_ctrl, flags);
	}

	STAT_DIST((tx_done < pp->dist_stats.tx_done_dist_size) ? pp->dist_stats.tx_done_dist[tx_done]++ : 0);

	return tx_done;
}

static void mv_eth_tx_frag_process(struct eth_port *pp, struct sk_buff *skb, struct tx_queue *txq_ctrl,	u16 flags)
{
	int i;
	struct neta_tx_desc *tx_desc;

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		tx_desc = mvNetaTxqNextDescGet(txq_ctrl->q);

		/* NETA_TX_BM_ENABLE_MASK = 0 */
		/* NETA_TX_PKT_OFFSET_MASK = 0 */
		tx_desc->dataSize = frag->size;
		tx_desc->bufPhysAddr = mvOsCacheFlush(NULL, page_address(frag->page.p) + frag->page_offset,
						      tx_desc->dataSize);

		if (i == (skb_shinfo(skb)->nr_frags - 1)) {
			/* Last descriptor */
			if (flags & MV_ETH_F_NO_PAD)
				tx_desc->command = NETA_TX_L_DESC_MASK;
			else
				tx_desc->command = (NETA_TX_L_DESC_MASK | NETA_TX_Z_PAD_MASK);

			txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = ((MV_ULONG) skb | MV_ETH_SHADOW_SKB);
			mv_eth_shadow_inc_put(txq_ctrl);
		} else {
			/* Descriptor in the middle: Not First, Not Last */
			tx_desc->command = 0;

			txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = 0;
			mv_eth_shadow_inc_put(txq_ctrl);
		}

		mv_eth_tx_desc_flush(tx_desc);
	}
}

/***********************************************************
 * mv_eth_port_pools_free                                  *
 *   per port - free all the buffers from pools		   *
 *   disable pool if empty				   *
 ***********************************************************/
static int mv_eth_port_pools_free(int port)
{
	struct eth_port *pp;

	pp = mv_eth_port_by_id(port);
	if (!pp)
		return MV_OK;

	if (pp->pool_long) {
		mv_eth_pool_free(pp->pool_long->pool, pp->pool_long_num);
#ifndef CONFIG_MV_ETH_BM_CPU
	}
#else
		if (pp->pool_long->buf_num == 0)
			mvBmPoolDisable(pp->pool_long->pool);

		/*empty pools*/
		if (pp->pool_short && (pp->pool_long->pool != pp->pool_short->pool)) {
			mv_eth_pool_free(pp->pool_short->pool, pp->pool_short_num);
			if (pp->pool_short->buf_num == 0)
				mvBmPoolDisable(pp->pool_short->pool);
		}
	}
#endif /*CONFIG_MV_ETH_BM_CPU*/
	return MV_OK;
}

/* Free "num" buffers from the pool */
static int mv_eth_pool_free(int pool, int num)
{
	struct eth_pbuf *pkt;
	int i = 0;
	struct bm_pool *ppool = &mv_eth_pool[pool];
	unsigned long flags = 0;
	bool free_all = false;

	MV_ETH_LOCK(&ppool->lock, flags);

	if (num >= ppool->buf_num) {
		/* Free all buffers from the pool */
		free_all = true;
		num = ppool->buf_num;
	}

#ifdef CONFIG_MV_ETH_BM_CPU
	if (mv_eth_pool_bm(ppool)) {

		if (free_all)
			mvBmConfigSet(MV_BM_EMPTY_LIMIT_MASK);

		while (i < num) {
			MV_U32 *va;
			MV_U32 pa = mvBmPoolGet(pool);

			if (pa == 0)
				break;

			va = phys_to_virt(pa);
			pkt = (struct eth_pbuf *)*va;
#if !defined(CONFIG_MV_ETH_BE_WA)
			pkt = (struct eth_pbuf *)MV_32BIT_LE((MV_U32)pkt);
#endif /* !CONFIG_MV_ETH_BE_WA */

			if (pkt) {
				mv_eth_pkt_free(pkt);
#ifdef ETH_SKB_DEBUG
				mv_eth_skb_check((struct sk_buff *)pkt->osInfo);
#endif /* ETH_SKB_DEBUG */
			}
			i++;
		}
		printk(KERN_ERR "bm pool #%d: pkt_size=%d, buf_size=%d - %d of %d buffers free\n",
			pool, ppool->pkt_size, RX_BUF_SIZE(ppool->pkt_size), i, num);

		if (free_all)
			mvBmConfigClear(MV_BM_EMPTY_LIMIT_MASK);
	}
#endif /* CONFIG_MV_ETH_BM_CPU */

	ppool->buf_num -= num;

	/* Free buffers from the pool stack too */
	if (free_all)
		num = mvStackIndex(ppool->stack);
	else if (mv_eth_pool_bm(ppool))
		num = 0;

	i = 0;
	while (i < num) {
		/* sanity check */
		if (mvStackIndex(ppool->stack) == 0) {
			printk(KERN_ERR "%s: No more buffers in the stack\n", __func__);
			break;
		}
		pkt = (struct eth_pbuf *)mvStackPop(ppool->stack);
		if (pkt) {
			mv_eth_pkt_free(pkt);
#ifdef ETH_SKB_DEBUG
			mv_eth_skb_check((struct sk_buff *)pkt->osInfo);
#endif /* ETH_SKB_DEBUG */
		}
		i++;
	}
	if (i > 0)
		printk(KERN_ERR "stack pool #%d: pkt_size=%d, buf_size=%d - %d of %d buffers free\n",
			pool, ppool->pkt_size, RX_BUF_SIZE(ppool->pkt_size), i, num);

	MV_ETH_UNLOCK(&ppool->lock, flags);

	return i;
}

static int mv_eth_pool_destroy(int pool)
{
	int num, status = 0;
	struct bm_pool *ppool = &mv_eth_pool[pool];

	num = mv_eth_pool_free(pool, ppool->buf_num);
	if (num != ppool->buf_num) {
		printk(KERN_ERR "Warning: could not free all buffers in pool %d while destroying pool\n", pool);
		return MV_ERROR;
	}

	status = mvStackDelete(ppool->stack);

#ifdef CONFIG_MV_ETH_BM_CPU
	mvBmPoolDisable(pool);

	/* Note: we don't free the bm_pool here ! */
	if (ppool->bm_pool)
		mvOsFree(ppool->bm_pool);
#endif /* CONFIG_MV_ETH_BM_CPU */

	memset(ppool, 0, sizeof(struct bm_pool));

	return status;
}

static int mv_eth_pool_add(int pool, int buf_num)
{
	struct bm_pool *bm_pool;
	struct sk_buff *skb;
	struct eth_pbuf *pkt;
	int i;
	unsigned long flags = 0;

	if ((pool < 0) || (pool >= MV_ETH_BM_POOLS)) {
		printk(KERN_ERR "%s: invalid pool number %d\n", __func__, pool);
		return MV_OK;
	}

	bm_pool = &mv_eth_pool[pool];

	/* Check buffer size */
	if (bm_pool->pkt_size == 0) {
		printk(KERN_ERR "%s: invalid pool #%d state: pkt_size=%d, buf_size=%d, buf_num=%d\n",
		       __func__, pool, bm_pool->pkt_size, RX_BUF_SIZE(bm_pool->pkt_size), bm_pool->buf_num);
		return MV_OK;
	}

	/* Insure buf_num is smaller than capacity */
	if ((buf_num < 0) || ((buf_num + bm_pool->buf_num) > (bm_pool->capacity))) {

		printk(KERN_ERR "%s: can't add %d buffers into bm_pool=%d: capacity=%d, buf_num=%d\n",
		       __func__, buf_num, pool, bm_pool->capacity, bm_pool->buf_num);
		return MV_OK;
	}

	MV_ETH_LOCK(&bm_pool->lock, flags);

	for (i = 0; i < buf_num; i++) {
		pkt = mvOsMalloc(sizeof(struct eth_pbuf));
		if (!pkt) {
			printk(KERN_ERR "%s: can't allocate %d bytes\n", __func__, sizeof(struct eth_pbuf));
			break;
		}

		skb = mv_eth_skb_alloc(bm_pool, pkt, GFP_KERNEL);
		if (!skb) {
			kfree(pkt);
			break;
		}
/*
	printk(KERN_ERR "skb_alloc_%d: pool=%d, skb=%p, pkt=%p, head=%p (%lx), skb->truesize=%d\n",
				i, bm_pool->pool, skb, pkt, pkt->pBuf, pkt->physAddr, skb->truesize);
*/
 
#ifdef CONFIG_MV_ETH_BM_CPU
		mvBmPoolPut(pool, (MV_ULONG) pkt->physAddr);
		STAT_DBG(bm_pool->stats.bm_put++);
#else
		mvStackPush(bm_pool->stack, (MV_U32) pkt);
		STAT_DBG(bm_pool->stats.stack_put++);
#endif /* CONFIG_MV_ETH_BM_CPU */
	}
	bm_pool->buf_num += i;

	printk(KERN_ERR "pool #%d: pkt_size=%d, buf_size=%d - %d of %d buffers added\n",
	       pool, bm_pool->pkt_size, RX_BUF_SIZE(bm_pool->pkt_size), i, buf_num);

	MV_ETH_UNLOCK(&bm_pool->lock, flags);

	return i;
}

#ifdef CONFIG_MV_ETH_BM
void	*mv_eth_bm_pool_create(int pool, int capacity, MV_ULONG *pPhysAddr)
{
		MV_ULONG			physAddr;
		MV_UNIT_WIN_INFO	winInfo;
		void				*pVirt;
		MV_STATUS			status;

		pVirt = mvOsIoUncachedMalloc(NULL, sizeof(MV_U32) * capacity, &physAddr, NULL);
		if (pVirt == NULL) {
			mvOsPrintf("%s: Can't allocate %d bytes for Long pool #%d\n",
					__func__, MV_BM_POOL_CAP_MAX * sizeof(MV_U32), pool);
			return NULL;
		}

		/* Pool address must be MV_BM_POOL_PTR_ALIGN bytes aligned */
		if (MV_IS_NOT_ALIGN((unsigned)pVirt, MV_BM_POOL_PTR_ALIGN)) {
			mvOsPrintf("memory allocated for BM pool #%d is not %d bytes aligned\n",
						pool, MV_BM_POOL_PTR_ALIGN);
			mvOsIoCachedFree(NULL, sizeof(MV_U32) * capacity, physAddr, pVirt, 0);
			return NULL;
		}
		status = mvBmPoolInit(pool, pVirt, physAddr, capacity);
		if (status != MV_OK) {
			mvOsPrintf("%s: Can't init #%d BM pool. status=%d\n", __func__, pool, status);
			mvOsIoCachedFree(NULL, sizeof(MV_U32) * capacity, physAddr, pVirt, 0);
			return NULL;
		}
		status = mvCtrlAddrWinInfoGet(&winInfo, physAddr);
		if (status != MV_OK) {
			printk(KERN_ERR "%s: Can't map BM pool #%d. phys_addr=0x%x, status=%d\n",
			       __func__, pool, (unsigned)physAddr, status);
			mvOsIoCachedFree(NULL, sizeof(MV_U32) * capacity, physAddr, pVirt, 0);
			return NULL;
		}
		mvBmPoolTargetSet(pool, winInfo.targetId, winInfo.attrib);
		mvBmPoolEnable(pool);

		if (pPhysAddr != NULL)
			*pPhysAddr = physAddr;

		return pVirt;
}
#endif /* CONFIG_MV_ETH_BM */

static MV_STATUS mv_eth_pool_create(int pool, int capacity)
{
	struct bm_pool *bm_pool;

	if ((pool < 0) || (pool >= MV_ETH_BM_POOLS)) {
		printk(KERN_ERR "%s: pool=%d is out of range\n", __func__, pool);
		return MV_BAD_VALUE;
	}

	bm_pool = &mv_eth_pool[pool];
	memset(bm_pool, 0, sizeof(struct bm_pool));

#ifdef CONFIG_MV_ETH_BM_CPU
	bm_pool->bm_pool = mv_eth_bm_pool_create(pool, capacity, &bm_pool->physAddr);
	if (bm_pool->bm_pool == NULL)
		return MV_FAIL;
#endif /* CONFIG_MV_ETH_BM_CPU */

	/* Create Stack as container of alloacted skbs for SKB_RECYCLE and for RXQs working without BM support */
	bm_pool->stack = mvStackCreate(capacity);

	if (bm_pool->stack == NULL) {
		printk(KERN_ERR "Can't create MV_STACK structure for %d elements\n", capacity);
		return MV_OUT_OF_CPU_MEM;
	}

	bm_pool->pool = pool;
	bm_pool->capacity = capacity;
	bm_pool->pkt_size = 0;
	bm_pool->buf_num = 0;
	spin_lock_init(&bm_pool->lock);

	return MV_OK;
}

/* Interrupt handling */
irqreturn_t mv_eth_isr(int irq, void *dev_id)
{
	struct eth_port *pp = (struct eth_port *)dev_id;
	struct napi_struct *napi = pp->cpu_config[smp_processor_id()]->napi;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->flags & MV_ETH_F_DBG_ISR) {
		printk(KERN_ERR "%s: port=%d, cpu=%d, mask=0x%x, cause=0x%x\n",
			__func__, pp->port, smp_processor_id(),
			MV_REG_READ(NETA_INTR_NEW_MASK_REG(pp->port)), MV_REG_READ(NETA_INTR_NEW_CAUSE_REG(pp->port)));
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	STAT_INFO(pp->stats.irq++);

	/* Mask all interrupts */
	MV_REG_WRITE(NETA_INTR_NEW_MASK_REG(pp->port), 0);
	/* To be sure that itterrupt already masked Dummy read is required */
	/* MV_REG_READ(NETA_INTR_NEW_MASK_REG(pp->port));*/
	 
	/* Verify that the device not already on the polling list */
	if (napi_schedule_prep(napi)) {
		/* schedule the work (rx+txdone+link) out of interrupt contxet */
		__napi_schedule(napi);
	} else {
		STAT_INFO(pp->stats.irq_err++);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		printk(KERN_ERR "%s: IRQ=%d, port=%d, cpu=%d - NAPI already scheduled\n",
			__func__, irq, pp->port, smp_processor_id());
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
	}
	return IRQ_HANDLED;
}

void mv_eth_link_event(struct eth_port *pp, int print)
{
	struct net_device *dev = pp->dev;
	bool              link_is_up;

	STAT_INFO(pp->stats.link++);

	/* Check Link status on ethernet port */
#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(pp->port))
		link_is_up = mv_pon_link_status();
	else
#endif /* CONFIG_MV_PON */
		link_is_up = mvNetaLinkIsUp(pp->port);

	if (link_is_up) {
		mvNetaPortUp(pp->port);
		set_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));

		if (mv_eth_ctrl_is_tx_enabled(pp)) {
			if (dev) {
				netif_carrier_on(dev);
				netif_tx_wake_all_queues(dev);
			}
		}
	} else {
		if (dev) {
			netif_carrier_off(dev);
			netif_tx_stop_all_queues(dev);
		}
		mvNetaPortDown(pp->port);
		clear_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));
	}

	if (print) {
#ifdef CONFIG_SYNO_ARMADA_ARCH
		/* change log level to avoid writing to syslog,
		   which will stop hibernation */
		if (dev)
			printk(KERN_NOTICE "%s: ", dev->name);
		else
			printk(KERN_NOTICE "%s: ", "none");
#else
		if (dev)
			printk(KERN_ERR "%s: ", dev->name);
		else
			printk(KERN_ERR "%s: ", "none");
#endif

		mv_eth_link_status_print(pp->port);
	}
}

/***********************************************************************************************/
int mv_eth_poll(struct napi_struct *napi, int budget)
{
	int rx_done = 0;
	MV_U32 causeRxTx;
	struct eth_port *pp = MV_ETH_PRIV(napi->dev);
	struct cpu_ctrl *cpuCtrl = pp->cpu_config[smp_processor_id()];

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->flags & MV_ETH_F_DBG_POLL) {
		printk(KERN_ERR "%s ENTER: port=%d, cpu=%d, mask=0x%x, cause=0x%x\n",
			__func__, pp->port, smp_processor_id(),
			MV_REG_READ(NETA_INTR_NEW_MASK_REG(pp->port)), MV_REG_READ(NETA_INTR_NEW_CAUSE_REG(pp->port)));
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->flags & MV_ETH_F_DBG_RX)
			printk(KERN_ERR "%s: STARTED_BIT = 0, poll completed.\n", __func__);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		napi_complete(napi);
		STAT_INFO(pp->stats.poll_exit[smp_processor_id()]++);
		return rx_done;
	}

	STAT_INFO(pp->stats.poll[smp_processor_id()]++);

	/* Read cause register */
	causeRxTx = MV_REG_READ(NETA_INTR_NEW_CAUSE_REG(pp->port)) &
	    (MV_ETH_MISC_SUM_INTR_MASK | MV_ETH_TXDONE_INTR_MASK | MV_ETH_RX_INTR_MASK);

	if (causeRxTx & MV_ETH_MISC_SUM_INTR_MASK) {
		MV_U32 causeMisc;

		/* Process MISC events - Link, etc ??? */
		causeRxTx &= ~MV_ETH_MISC_SUM_INTR_MASK;
		causeMisc = MV_REG_READ(NETA_INTR_MISC_CAUSE_REG(pp->port));

		if (causeMisc & NETA_CAUSE_LINK_CHANGE_MASK)
			mv_eth_link_event(pp, 1);

		MV_REG_WRITE(NETA_INTR_MISC_CAUSE_REG(pp->port), 0);
	}
	causeRxTx |= cpuCtrl->causeRxTx;

#ifdef CONFIG_MV_ETH_TXDONE_ISR
	if (causeRxTx & MV_ETH_TXDONE_INTR_MASK) {
		int tx_todo = 0;
		/* TX_DONE process */
		 
		if (MV_PON_PORT(pp->port))
			mv_eth_tx_done_pon(pp, &tx_todo);
		else
			mv_eth_tx_done_gbe(pp, (causeRxTx & MV_ETH_TXDONE_INTR_MASK), &tx_todo);

		causeRxTx &= ~MV_ETH_TXDONE_INTR_MASK;
	}
#endif /* CONFIG_MV_ETH_TXDONE_ISR */

#if (CONFIG_MV_ETH_RXQ > 1)
	while ((causeRxTx != 0) && (budget > 0)) {
		int count, rx_queue;

		rx_queue = mv_eth_rx_policy(causeRxTx);
		if (rx_queue == -1)
			break;

		count = mv_eth_rx(pp, budget, rx_queue);
		rx_done += count;
		budget -= count;
		if (budget > 0)
			causeRxTx &= ~((1 << rx_queue) << NETA_CAUSE_RXQ_OCCUP_DESC_OFFS);
	}
#else
	rx_done = mv_eth_rx(pp, budget, CONFIG_MV_ETH_RXQ_DEF);
	budget -= rx_done;
#endif /* (CONFIG_MV_ETH_RXQ > 1) */

	STAT_DIST((rx_done < pp->dist_stats.rx_dist_size) ? pp->dist_stats.rx_dist[rx_done]++ : 0);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->flags & MV_ETH_F_DBG_POLL) {
		printk(KERN_ERR "%s  EXIT: port=%d, cpu=%d, budget=%d, rx_done=%d\n",
			__func__, pp->port, smp_processor_id(), budget, rx_done);
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	if (budget > 0) {
		unsigned long flags;

		causeRxTx = 0;

		napi_complete(napi);

		STAT_INFO(pp->stats.poll_exit[smp_processor_id()]++);

		local_irq_save(flags);
		MV_REG_WRITE(NETA_INTR_NEW_MASK_REG(pp->port),
			     (MV_ETH_MISC_SUM_INTR_MASK | MV_ETH_TXDONE_INTR_MASK | MV_ETH_RX_INTR_MASK));

		local_irq_restore(flags);
	}
	cpuCtrl->causeRxTx = causeRxTx;
	return rx_done;
}

static void mv_eth_cpu_counters_init(void)
{
#ifdef CONFIG_MV_CPU_PERF_CNTRS

	mvCpuCntrsInitialize();

#ifdef CONFIG_PLAT_ARMADA
	/*  cycles counter via special CCNT counter */
	mvCpuCntrsProgram(0, MV_CPU_CNTRS_CYCLES, "Cycles", 13);

	/* instruction counters */
	mvCpuCntrsProgram(1, MV_CPU_CNTRS_INSTRUCTIONS, "Instr", 13);
	/* mvCpuCntrsProgram(0, MV_CPU_CNTRS_DCACHE_READ_HIT, "DcRdHit", 0); */
	 
	/* ICache misses counter */
	mvCpuCntrsProgram(2, MV_CPU_CNTRS_ICACHE_READ_MISS, "IcMiss", 0);

	/* DCache read misses counter */
	mvCpuCntrsProgram(3, MV_CPU_CNTRS_DCACHE_READ_MISS, "DcRdMiss", 0);

	/* DCache write misses counter */
	mvCpuCntrsProgram(4, MV_CPU_CNTRS_DCACHE_WRITE_MISS, "DcWrMiss", 0);

	/* DTLB Miss counter */
	mvCpuCntrsProgram(5, MV_CPU_CNTRS_DTLB_MISS, "dTlbMiss", 0);

	/* mvCpuCntrsProgram(3, MV_CPU_CNTRS_TLB_MISS, "TlbMiss", 0); */
#else /* CONFIG_FEROCEON */
	/* 0 - instruction counters */
	mvCpuCntrsProgram(0, MV_CPU_CNTRS_INSTRUCTIONS, "Instr", 16);
	/* mvCpuCntrsProgram(0, MV_CPU_CNTRS_DCACHE_READ_HIT, "DcRdHit", 0); */
	 
	/* 1 - ICache misses counter */
	mvCpuCntrsProgram(1, MV_CPU_CNTRS_ICACHE_READ_MISS, "IcMiss", 0);

	/* 2 - cycles counter */
	mvCpuCntrsProgram(2, MV_CPU_CNTRS_CYCLES, "Cycles", 18);

	/* 3 - DCache read misses counter */
	mvCpuCntrsProgram(3, MV_CPU_CNTRS_DCACHE_READ_MISS, "DcRdMiss", 0);
	/* mvCpuCntrsProgram(3, MV_CPU_CNTRS_TLB_MISS, "TlbMiss", 0); */
#endif /* CONFIG_PLAT_ARMADA */

	event0 = mvCpuCntrsEventCreate("RX_DESC_PREF", 100000);
	event1 = mvCpuCntrsEventCreate("RX_DESC_READ", 100000);
	event2 = mvCpuCntrsEventCreate("RX_BUF_INV", 100000);
	event3 = mvCpuCntrsEventCreate("RX_DESC_FILL", 100000);
	event4 = mvCpuCntrsEventCreate("TX_START", 100000);
	event5 = mvCpuCntrsEventCreate("RX_BUF_INV", 100000);
	if ((event0 == NULL) || (event1 == NULL) || (event2 == NULL) ||
		(event3 == NULL) || (event4 == NULL) || (event5 == NULL))
		printk(KERN_ERR "Can't create cpu counter events\n");
#endif /* CONFIG_MV_CPU_PERF_CNTRS */
}

void mv_eth_port_promisc_set(int port, int queue)
{
#ifdef CONFIG_MV_ETH_PNC
	/* Accept all */
	if (mv_eth_pnc_ctrl_en) {
		pnc_mac_me(port, NULL, queue);
		pnc_mcast_all(port, 1);
	} else {
		printk(KERN_ERR "%s: PNC control is disabled\n", __func__);
	}
#else /* Legacy parser */
	mvNetaRxUnicastPromiscSet(port, MV_TRUE);
	mvNetaSetUcastTable(port, queue);
	mvNetaSetSpecialMcastTable(port, queue);
	mvNetaSetOtherMcastTable(port, queue);
#endif /* CONFIG_MV_ETH_PNC */
}

void mv_eth_port_filtering_cleanup(int port)
{
#ifdef CONFIG_MV_ETH_PNC
	static bool is_first = true;

	/* clean TCAM only one, no need to do this per port. */
	if (is_first) {
		tcam_hw_init();
		is_first = false;
	}
#else
	mvNetaRxUnicastPromiscSet(port, MV_FALSE);
	mvNetaSetUcastTable(port, -1);
	mvNetaSetSpecialMcastTable(port, -1);
	mvNetaSetOtherMcastTable(port, -1);
#endif /* CONFIG_MV_ETH_PNC */
}

static MV_STATUS mv_eth_bm_pools_init(void)
{
	int i, j;
	MV_STATUS status;

	/* Get compile time configuration */
#ifdef CONFIG_MV_ETH_BM
	mvBmControl(MV_START);
	mv_eth_bm_config_get();
#endif /* CONFIG_MV_ETH_BM */

	/* Create all pools with maximum capacity */
	for (i = 0; i < MV_ETH_BM_POOLS; i++) {
		status = mv_eth_pool_create(i, MV_BM_POOL_CAP_MAX);
		if (status != MV_OK) {
			printk(KERN_ERR "%s: can't create bm_pool=%d - capacity=%d\n", __func__, i, MV_BM_POOL_CAP_MAX);
			for (j = 0; j < i; j++)
				mv_eth_pool_destroy(j);
			return status;
		}
#ifdef CONFIG_MV_ETH_BM_CPU
		mv_eth_pool[i].pkt_size = mv_eth_bm_config_pkt_size_get(i);
		if (mv_eth_pool[i].pkt_size == 0)
			mvBmPoolBufSizeSet(i, 0);
		else
			mvBmPoolBufSizeSet(i, RX_BUF_SIZE(mv_eth_pool[i].pkt_size));
#else
		mv_eth_pool[i].pkt_size = 0;
#endif /* CONFIG_MV_ETH_BM */
	}
	return MV_OK;
}

/* Note: call this function only after mv_eth_ports_num is initialized */
static int mv_eth_load_network_interfaces(MV_U32 portMask, MV_U32 cpuMask,
						struct platform_device *pdev)
{
	u32 port, dev_i = 0;
	struct eth_port *pp;
	int mtu, err;
	u8 mac[MV_MAC_ADDR_SIZE];

	printk(KERN_ERR "  o Loading network interface(s)\n");

	for (port = 0; port < mv_eth_ports_num; port++) {
		if (!(MV_BIT_CHECK(portMask, port)))
			continue;

		if (!mvCtrlPwrClckGet(ETH_GIG_UNIT_ID, port)) {
			printk(KERN_ERR "\n  o Warning: GbE port %d is powered off\n\n", port);
			continue;
		}
		if (!MV_PON_PORT(port) && !mvBoardIsGbEPortConnected(port)) {
			printk(KERN_ERR "\n  o Warning: GbE port %d is not connected to PHY/RGMII/Switch, skip initialization\n\n",
					port);
			continue;
		}

		pp = mv_eth_ports[port] = mvOsMalloc(sizeof(struct eth_port));
		if (!pp) {
			printk(KERN_ERR "Error: failed to allocate memory for port %d\n", port);
			return -ENOMEM;
		}

		err = mv_eth_priv_init(pp, port);
		pp->cpuMask = cpuMask;

		if (err)
			return err;

#ifdef CONFIG_MV_ETH_PMT
		if (MV_PON_PORT(port))
			mvNetaPmtInit(port, (MV_NETA_PMT *)ioremap(PMT_PON_PHYS_BASE, PMT_MEM_SIZE));
		else
			mvNetaPmtInit(port, (MV_NETA_PMT *)ioremap(PMT_GIGA_PHYS_BASE + port * 0x40000, PMT_MEM_SIZE));
#endif /* CONFIG_MV_ETH_PMT */

#ifdef CONFIG_MV_ETH_SWITCH
		if (pp->flags & (MV_ETH_F_SWITCH | MV_ETH_F_EXT_SWITCH)) {
			int status = mv_eth_switch_config_get(mv_eth_initialized, port);

			if (status < 0) {
				printk(KERN_ERR "\nWarning: port %d - Invalid netconfig string\n", port);
				mv_eth_priv_cleanup(pp);
				continue;
			} else if (status == 0) {	/* User selected to work with Gateway driver    */
				clear_bit(MV_ETH_F_EXT_SWITCH_BIT, &(pp->flags));
			} else if (status == 1) {
				/* User selected to work without Gateway driver */
				clear_bit(MV_ETH_F_SWITCH_BIT, &(pp->flags));
				printk(KERN_ERR "  o Working in External Switch mode\n");
				ext_switch_port_mask = mv_switch_link_detection_init();
			}
		}

		if (pp->flags & MV_ETH_F_SWITCH) {
			set_bit(MV_ETH_F_MH_BIT, &(pp->flags));
			mtu = switch_net_config[port].mtu;
			if (mv_switch_init(RX_PKT_SIZE(mtu), SWITCH_CONNECTED_PORTS_MASK)) {
				printk(KERN_ERR "\nWarning: port %d - Switch initialization failed\n", port);
				mv_eth_priv_cleanup(pp);
				continue;
			}
		} else
#endif /* CONFIG_MV_ETH_SWITCH */
			mtu = mv_eth_config_get(pp, mac);

		printk(KERN_ERR "\t%s p=%d: mtu=%d, mac=%p\n", MV_PON_PORT(port) ? "pon" : "giga", port, mtu, mac);

		if (mv_eth_hal_init(pp)) {
			printk(KERN_ERR "%s: can't init eth hal\n", __func__);
			mv_eth_priv_cleanup(pp);
			return -EIO;
		}
#ifdef CONFIG_MV_ETH_SWITCH
		if (pp->flags & MV_ETH_F_SWITCH) {
			int queue = CONFIG_MV_ETH_RXQ_DEF;

			mv_eth_switch_netdev_first = dev_i;
			dev_i = mv_eth_switch_netdev_init(pp, dev_i, pdev);
			if (dev_i < (mv_eth_switch_netdev_first + switch_net_config[port].netdev_max)) {
				printk(KERN_ERR "%s: can't create netdevice for switch\n", __func__);
				mv_eth_priv_cleanup(pp);
				return -EIO;
			}
			mv_eth_switch_netdev_last = dev_i - 1;

			/* set this port to be in promiscuous mode. MAC filtering is performed by the Switch */
			mv_eth_port_promisc_set(pp->port, queue);
			handle_group_affinity(port);

			continue;
		}
#endif /* CONFIG_MV_ETH_SWITCH */

		mv_net_devs[dev_i] = mv_eth_netdev_init(pp, mtu, mac, pdev);
		if (!mv_net_devs[dev_i]) {
			printk(KERN_ERR "%s: can't create netdevice\n", __func__);
			mv_eth_priv_cleanup(pp);
			return -EIO;
		}
		pp->dev = mv_net_devs[dev_i];
		dev_i++;
		handle_group_affinity(port);
	}

	mv_net_devs_num = dev_i;

	return MV_OK;
}

int mv_eth_resume_network_interfaces(struct eth_port *pp)
{
	int cpu;

	if (!MV_PON_PORT(pp->port)) {
		int phyAddr;
		/* Set the board information regarding PHY address */
		phyAddr = mvBoardPhyAddrGet(pp->port);
		mvNetaPhyAddrSet(pp->port, phyAddr);
	}
	mvNetaPortDisable(pp->port);
	mvNetaDefaultsSet(pp->port);

	if (pp->flags & MV_ETH_F_MH)
		mvNetaMhSet(pp->port, MV_NETA_MH);

#ifdef CONFIG_MV_ETH_SWITCH
	if (pp->flags & MV_ETH_F_SWITCH) {
		/* set this port to be in promiscuous mode. MAC filtering is performed by the Switch */
		mv_eth_port_promisc_set(pp->port, CONFIG_MV_ETH_RXQ_DEF);
	}
#endif /* CONFIG_MV_ETH_SWITCH */

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		/* set queue mask per cpu */
		mvNetaRxqCpuMaskSet(pp->port, pp->cpu_config[cpu]->cpuRxqMask, cpu);
		mvNetaTxqCpuMaskSet(pp->port, pp->cpu_config[cpu]->cpuTxqMask, cpu);
	}

	return MV_OK;
}

#ifdef CONFIG_MV_ETH_BM
int     mv_eth_bm_pool_restore(struct bm_pool *bm_pool)
{
		MV_UNIT_WIN_INFO        winInfo;
		MV_STATUS               status;
		int pool = bm_pool->pool;

		mvBmPoolInit(bm_pool->pool, bm_pool->bm_pool, bm_pool->physAddr, bm_pool->capacity);
		status = mvCtrlAddrWinInfoGet(&winInfo, bm_pool->physAddr);

		if (status != MV_OK) {
			printk(KERN_ERR "%s: Can't map BM pool #%d. phys_addr=0x%x, status=%d\n",
				__func__, bm_pool->pool, (unsigned)bm_pool->physAddr, status);
			mvOsIoCachedFree(NULL, sizeof(MV_U32) *  bm_pool->capacity, bm_pool->physAddr, bm_pool->bm_pool, 0);
			return MV_ERROR;
		}
		mvBmPoolTargetSet(pool, winInfo.targetId, winInfo.attrib);
		mvBmPoolEnable(pool);

		return MV_OK;
}
#endif /*CONFIG_MV_ETH_BM*/

/* Refill port pools */
static int mv_eth_resume_port_pools(struct eth_port *pp)
{
	int num;

	if (!pp)
		return -ENODEV;

	/* fill long pool */
	if (pp->pool_long) {
		num = mv_eth_pool_add(pp->pool_long->pool, pp->pool_long_num);

		if (num != pp->pool_long_num) {
			printk(KERN_ERR "%s FAILED long: pool=%d, pkt_size=%d, only %d of %d allocated\n",
			       __func__, pp->pool_long->pool, pp->pool_long->pkt_size, num, pp->pool_long_num);
			return MV_ERROR;
		}

#ifndef CONFIG_MV_ETH_BM_CPU
	} /*fill long pool */
#else
		mvNetaBmPoolBufSizeSet(pp->port, pp->pool_long->pool, RX_BUF_SIZE(pp->pool_long->pkt_size));
	}

	if (pp->pool_short) {
		if (pp->pool_short->pool != pp->pool_long->pool) {
				/* fill short pool */
				num = mv_eth_pool_add(pp->pool_short->pool, pp->pool_short_num);
				if (num != pp->pool_short_num) {
					printk(KERN_ERR "%s FAILED short: pool=%d, pkt_size=%d - %d of %d buffers added\n",
					   __func__, pp->pool_short->pool, pp->pool_short->pkt_size, num, pp->pool_short_num);
					return MV_ERROR;
				}

				mvNetaBmPoolBufSizeSet(pp->port, pp->pool_short->pool, RX_BUF_SIZE(pp->pool_short->pkt_size));

		} else {

			int dummy_short_pool = (pp->pool_short->pool + 1) % MV_BM_POOLS;
			/* To disable short pool we choose unused pool and set pkt size to 0 (buffer size = pkt offset) */
			mvNetaBmPoolBufSizeSet(pp->port, dummy_short_pool, NET_SKB_PAD);

		}
	}

#endif /* CONFIG_MV_ETH_BM_CPU */

	return MV_OK;
}

static int mv_eth_resume_rxq_txq(struct eth_port *pp, int mtu)
{
	int rxq, txp, txq = 0;

	if (mvBoardIsPortInSgmii(pp->port))
		MV_REG_WRITE(SGMII_SERDES_CFG_REG(pp->port), pp->sgmii_serdes);

	for (txp = 0; txp < pp->txp_num; txp++)
		mvNetaTxpReset(pp->port, txp);

	mvNetaRxReset(pp->port);

	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {

		if (pp->rxq_ctrl[rxq].q) {
			/* Set Rx descriptors queue starting address */
			mvNetaRxqAddrSet(pp->port, rxq,  pp->rxq_ctrl[rxq].rxq_size);

			/* Set Offset */
			mvNetaRxqOffsetSet(pp->port, rxq, NET_SKB_PAD);

			/* Set coalescing pkts and time */
			mv_eth_rx_ptks_coal_set(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_pkts_coal);
			mv_eth_rx_time_coal_set(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_time_coal);

#if defined(CONFIG_MV_ETH_BM_CPU)
			/* Enable / Disable - BM support */
			if (pp->pool_long && pp->pool_short) {

				if (pp->pool_short->pool == pp->pool_long->pool) {
					int dummy_short_pool = (pp->pool_short->pool + 1) % MV_BM_POOLS;

					/* To disable short pool we choose unused pool and set pkt size to 0 (buffer size = pkt offset) */
					mvNetaRxqBmEnable(pp->port, rxq, dummy_short_pool, pp->pool_long->pool);
				} else
					mvNetaRxqBmEnable(pp->port, rxq, pp->pool_short->pool, pp->pool_long->pool);
			}
#else
			/* Fill RXQ with buffers from RX pool */
			mvNetaRxqBufSizeSet(pp->port, rxq, RX_BUF_SIZE(pp->pool_long->pkt_size));
			mvNetaRxqBmDisable(pp->port, rxq);
#endif /* CONFIG_MV_ETH_BM_CPU */
			if (mvNetaRxqFreeDescNumGet(pp->port, rxq) == 0)
				mv_eth_rxq_fill(pp, rxq, pp->rxq_ctrl[rxq].rxq_size);
		}
	}

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			struct tx_queue *txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

			if (txq_ctrl->q != NULL) {
				mvNetaTxqAddrSet(pp->port, txq_ctrl->txp, txq_ctrl->txq, txq_ctrl->txq_size);
				mv_eth_tx_done_ptks_coal_set(pp->port, txp, txq,
							pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq].txq_done_pkts_coal);
			}
			mvNetaTxqBandwidthSet(pp->port, txp, txq);

		}
		mvNetaTxpMaxTxSizeSet(pp->port, txp, RX_PKT_SIZE(mtu));
	}

	return MV_OK;
}

/**********************************************************
 * mv_eth_pnc_resume                                      *
 **********************************************************/
#ifdef CONFIG_MV_ETH_PNC
static void mv_eth_pnc_resume(void)
{
	/* TODO - in clock standby ,DO we want to keep old pnc TCAM/SRAM entries ? */
	if (wol_ports_bmp != 0)
		return;

	/* Not in WOL, clock standby or suspend to ram mode*/
	tcam_hw_init();

	if (pnc_default_init())
		printk(KERN_ERR "%s: Warning PNC init failed\n", __func__);

	/* TODO: load balancing resume */
#if defined(MV_ETH_PNC_LB)
	/*mv_eth_set_features(pp->dev, u32 features)*/
#endif  
}
#endif /* CONFIG_MV_ETH_PNC */

/***********************************************************
 * mv_eth_port_resume                                      *
 ***********************************************************/

int mv_eth_port_resume(int port)
{
	struct eth_port *pp;
	int cpu;

	pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp == NULL, port=%d\n", __func__, port);
		return  MV_ERROR;
	}

	if (!(pp->flags & MV_ETH_F_SUSPEND)) {
		printk(KERN_ERR "%s: port %d is not suspend.\n", __func__, port);
		return MV_ERROR;
	}
	mvNetaPortPowerUp(port, mvBoardIsPortInSgmii(port), mvBoardIsPortInRgmii(port));

	mv_eth_win_init(port);

	mv_eth_resume_network_interfaces(pp);

	/* only once for all ports*/
	if (pm_flag == 0) {

#ifdef CONFIG_MV_ETH_BM
	{
		struct bm_pool *ppool;
		int pool;

		mvBmControl(MV_START);

		mvBmRegsInit();

		for (pool = 0; pool < MV_ETH_BM_POOLS; pool++) {
			ppool = &mv_eth_pool[pool];
			if (mv_eth_bm_pool_restore(ppool)) {
				printk(KERN_ERR "%s: port #%d pool #%d resrote failed.\n", __func__, port, pool);
				return MV_ERROR;
			}
		}
	}
#endif /*CONFIG_MV_ETH_BM*/

#ifdef CONFIG_MV_ETH_PNC
		mv_eth_pnc_resume();
#endif /* CONFIG_MV_ETH_PNC */

		pm_flag = 1;
	}

	if (pp->flags & MV_ETH_F_STARTED_OLD)
		(*pp->dev->netdev_ops->ndo_set_rx_mode)(pp->dev);

	for_each_possible_cpu(cpu)
		pp->cpu_config[cpu]->causeRxTx = 0;

	set_bit(MV_ETH_F_STARTED_BIT, &(pp->flags));

	mv_eth_resume_port_pools(pp);

	mv_eth_resume_rxq_txq(pp, pp->dev->mtu);

	if (pp->flags & MV_ETH_F_STARTED_OLD) {
		mv_eth_resume_internals(pp, pp->dev->mtu);
		clear_bit(MV_ETH_F_STARTED_OLD_BIT, &(pp->flags));
		if (pp->flags & MV_ETH_F_CONNECT_LINUX) {
			mv_eth_interrupts_unmask(pp);
			smp_call_function_many(cpu_online_mask, (smp_call_func_t)mv_eth_interrupts_unmask, (void *)pp, 1);
		}
	} else
		clear_bit(MV_ETH_F_STARTED_BIT, &(pp->flags));

	clear_bit(MV_ETH_F_SUSPEND_BIT, &(pp->flags));

	printk(KERN_NOTICE "Exit suspend mode on port #%d\n", port);

	return MV_OK;
}

/***********************************************************
 * mv_eth_win_init --                                      *
 *   Win initilization                                     *
 ***********************************************************/
void 	mv_eth_win_init(int port)
{

	MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];
	MV_STATUS status;
	int i;

	status = mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1);
	if (status != MV_OK)
		return;

	for (i = 0; i < MAX_TARGETS; i++) {
		if (addrWinMap[i].enable == MV_FALSE)
			continue;

#ifdef CONFIG_MV_SUPPORT_L2_DEPOSIT
		/* Setting DRAM windows attribute to :
		   0x3 - Shared transaction + L2 write allocate (L2 Deposit) */
		if (MV_TARGET_IS_DRAM(i)) {
			addrWinMap[i].attrib &= ~(0x30);
			addrWinMap[i].attrib |= 0x30;
		}
#endif
	}
	mvNetaWinInit(port, addrWinMap);
	return;
}

/***********************************************************
 * mv_eth_port_suspend                                     *
 *   main driver initialization. loading the interfaces.   *
 ***********************************************************/
int mv_eth_port_suspend(int port)
{
	struct eth_port *pp;

	pp = mv_eth_port_by_id(port);
	if (!pp)
		return MV_OK;

	if (pp->flags & MV_ETH_F_SUSPEND) {
		printk(KERN_ERR "%s: port %d is allready suspend.\n", __func__, port);
		return MV_ERROR;
	}

	if (mvBoardIsPortInSgmii(pp->port))
		pp->sgmii_serdes = MV_REG_READ(SGMII_SERDES_CFG_REG(port));

	if (pp->flags & MV_ETH_F_STARTED) {
		set_bit(MV_ETH_F_STARTED_OLD_BIT, &(pp->flags));
		clear_bit(MV_ETH_F_STARTED_BIT, &(pp->flags));
		mv_eth_suspend_internals(pp);
	} else
		clear_bit(MV_ETH_F_STARTED_OLD_BIT, &(pp->flags));

#ifdef CONFIG_MV_ETH_HWF
	mvNetaHwfEnable(pp->port, 0);
#else
	{
		int txp;
		/* Reset TX port, transmit all pending packets */
		for (txp = 0; txp < pp->txp_num; txp++)
			mv_eth_txp_reset(pp->port, txp);
	}
#endif  /* !CONFIG_MV_ETH_HWF */

	/* Reset RX port, free the empty buffers form queue */
	mv_eth_rx_reset(pp->port);

	mv_eth_port_pools_free(port);

	set_bit(MV_ETH_F_SUSPEND_BIT, &(pp->flags));

	printk(KERN_NOTICE "Enter suspend mode on port #%d\n", port);
	return MV_OK;
}

/***********************************************************
 * mv_eth_wol_mode_set --                                   *
 *   set wol_mode. (power menegment mod)		    *
 ***********************************************************/
int	mv_eth_wol_mode_set(int port, int mode)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp == NULL, port=%d\n", __func__, port);
		return -EINVAL;
	}

	if ((mode < 0) || (mode > 1)) {
		printk(KERN_ERR "%s: mode = %d, Invalid value.\n", __func__, mode);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_SUSPEND) {
		printk(KERN_ERR "Port %d must resumed before\n", port);
		return -EINVAL;
	}
	pp->wol_mode = mode;

	if (mode)
		wol_ports_bmp |= (1 << port);
	else
		wol_ports_bmp &= ~(1 << port);

	return MV_OK;
}

/***********************************************************
 * mv_eth_probe --                                         *
 *   main driver initialization. loading the interfaces.   *
 ***********************************************************/
static int mv_eth_probe(struct platform_device *pdev)
{
	u32 port;
	struct eth_port *pp;
	int size;
	MV_U32 port_mask, cpu_mask;

	if (pdev->dev.platform_data) {
		port_mask = ((struct netaSmpGroupStruct *)pdev->dev.platform_data)->portMask;
		cpu_mask =  ((struct netaSmpGroupStruct *)pdev->dev.platform_data)->cpuMask;
	} else {
		port_mask = (1 << CONFIG_MV_ETH_PORTS_NUM) - 1;
		cpu_mask = (1 << CONFIG_NR_CPUS) - 1;
	}
	printk(KERN_INFO "%s: port_mask=0x%x, cpu_mask=0x%x \n", __func__, port_mask, cpu_mask);

#ifdef ETH_SKB_DEBUG
	memset(mv_eth_skb_debug, 0, sizeof(mv_eth_skb_debug));
	spin_lock_init(&skb_debug_lock);
#endif

	if (!mv_eth_initialized) {
		mvSysNetaInit(port_mask, cpu_mask); /* init MAC Unit */

		mv_eth_ports_num = mvCtrlEthMaxPortGet();
		if (mv_eth_ports_num > CONFIG_MV_ETH_PORTS_NUM)
			mv_eth_ports_num = CONFIG_MV_ETH_PORTS_NUM;

		mv_net_devs_max = mv_eth_ports_num;

#ifdef CONFIG_MV_ETH_SWITCH
		mv_net_devs_max += (CONFIG_MV_ETH_SWITCH_NETDEV_NUM - 1);
#endif /* CONFIG_MV_ETH_SWITCH */

		mv_eth_config_show();

		size = mv_eth_ports_num * sizeof(struct eth_port *);
		mv_eth_ports = mvOsMalloc(size);
		if (!mv_eth_ports)
			goto oom;

		memset(mv_eth_ports, 0, size);

		/* Allocate array of pointers to struct net_device */
		size = mv_net_devs_max * sizeof(struct net_device *);
		mv_net_devs = mvOsMalloc(size);
		if (!mv_net_devs)
			goto oom;

		memset(mv_net_devs, 0, size);
	}

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en) {
		if (pnc_default_init())
			printk(KERN_ERR "%s: Warning PNC init failed\n", __func__);
	} else
		printk(KERN_ERR "%s: PNC control is disabled\n", __func__);
#endif /* CONFIG_MV_ETH_PNC */

	if (mv_eth_bm_pools_init())
		goto oom;

#ifdef CONFIG_MV_INCLUDE_SWITCH
	if ((mvBoardSwitchConnectedPortGet(0) != -1) || (mvBoardSwitchConnectedPortGet(1) != -1)) {
		if (mv_switch_load(SWITCH_CONNECTED_PORTS_MASK))
			printk(KERN_ERR "\nWarning: Switch load failed\n");
	}
#endif /* CONFIG_MV_INCLUDE_SWITCH */

	if (!mv_eth_initialized) {
		if (mv_eth_load_network_interfaces(port_mask, cpu_mask, pdev))
			goto oom;
	}

#ifdef CONFIG_MV_ETH_HWF
	for (port = 0; port < mv_eth_ports_num; port++) {
		if (mv_eth_ports[port])
			mvNetaHwfInit(port);
	}
#endif /* CONFIG_MV_ETH_HWF */

	/* Call mv_eth_open specifically for ports not connected to Linux netdevice */
	for (port = 0; port < mv_eth_ports_num; port++) {
		pp = mv_eth_port_by_id(port);

		if (pp) {
			if (!(pp->flags & MV_ETH_F_CONNECT_LINUX)) {
				if (pp->flags & MV_ETH_F_SWITCH)
					printk(KERN_ERR "%s: a GbE port using the Gateway driver cannot be disconnected from Linux\n",
							__func__);
				else
					mv_eth_open(pp->dev);
			}
		}
	}

	if (!mv_eth_initialized)
		mv_eth_cpu_counters_init();

	printk(KERN_ERR "\n");

	mv_eth_initialized = 1;

	return MV_OK;
oom:
	if (mv_eth_ports)
		mvOsFree(mv_eth_ports);

	if (mv_net_devs)
		mvOsFree(mv_net_devs);

	printk(KERN_ERR "%s: out of memory\n", __func__);
	return -ENOMEM;
}

static int mv_eth_config_get(struct eth_port *pp, MV_U8 *mac_addr)
{
	char *mac_str = NULL;
	u8 zero_mac[MV_MAC_ADDR_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int mtu;

	switch (pp->port) {
	case 0:
		if (mvMtu[0] != 0)
			mtu = mvMtu[0];
		else
			mtu = CONFIG_MV_ETH_0_MTU;

		/* use default MAC address from Kconfig only if the MAC address we got is all 0 */
		if (memcmp(mvMacAddr[0], zero_mac, MV_MAC_ADDR_SIZE) == 0)
			mac_str = CONFIG_MV_ETH_0_MACADDR;
		else
			memcpy(mac_addr, mvMacAddr[0], MV_MAC_ADDR_SIZE);

		break;

#if (CONFIG_MV_ETH_PORTS_NUM > 1)
	case 1:
		if (mvMtu[1] != 0)
			mtu = mvMtu[1];
		else
			mtu = CONFIG_MV_ETH_1_MTU;

		/* use default MAC address from Kconfig only if the MAC address we got is all 0 */
		if (memcmp(mvMacAddr[1], zero_mac, MV_MAC_ADDR_SIZE) == 0)
			mac_str = CONFIG_MV_ETH_1_MACADDR;
		else
			memcpy(mac_addr, mvMacAddr[1], MV_MAC_ADDR_SIZE);

		break;
#endif /* CONFIG_MV_ETH_PORTS_NUM > 1 */

#if (CONFIG_MV_ETH_PORTS_NUM > 2)
	case 2:
		if (mvMtu[2] != 0)
			mtu = mvMtu[2];
		else
			mtu = CONFIG_MV_ETH_2_MTU;

		/* use default MAC address from Kconfig only if the MAC address we got is all 0 */
		if (memcmp(mvMacAddr[2], zero_mac, MV_MAC_ADDR_SIZE) == 0)
			mac_str = CONFIG_MV_ETH_2_MACADDR;
		else
			memcpy(mac_addr, mvMacAddr[2], MV_MAC_ADDR_SIZE);
		break;
#endif /* CONFIG_MV_ETH_PORTS_NUM > 2 */

#if (CONFIG_MV_ETH_PORTS_NUM > 3)
	case 3:
		if (mvMtu[3] != 0)
			mtu = mvMtu[3];
		else
			mtu = CONFIG_MV_ETH_3_MTU;

		/* use default MAC address from Kconfig only if the MAC address we got is all 0 */
		if (memcmp(mvMacAddr[3], zero_mac, MV_MAC_ADDR_SIZE) == 0)
			mac_str = CONFIG_MV_ETH_3_MACADDR;
		else
			memcpy(mac_addr, mvMacAddr[3], MV_MAC_ADDR_SIZE);

		break;
#endif /* CONFIG_MV_ETH_PORTS_NUM > 3 */

	default:
		printk(KERN_ERR "eth_get_config: Unexpected port number %d\n", pp->port);
		return MV_ERROR;
	}
	if ((mac_str != NULL) && (mac_addr != NULL))
		mvMacStrToHex(mac_str, mac_addr);

	return mtu;
}

/***********************************************************
 * mv_eth_tx_timeout --                                    *
 *   nothing to be done (?)                                *
 ***********************************************************/
static void mv_eth_tx_timeout(struct net_device *dev)
{
#ifdef CONFIG_MV_ETH_STAT_ERR
	struct eth_port *pp = MV_ETH_PRIV(dev);

	pp->stats.tx_timeout++;
#endif /* #ifdef CONFIG_MV_ETH_STAT_ERR */

	printk(KERN_INFO "%s: tx timeout\n", dev->name);
}

/***************************************************************
 * mv_eth_netdev_init -- Allocate and initialize net_device    *
 *                   structure                                 *
 ***************************************************************/
struct net_device *mv_eth_netdev_init(struct eth_port *pp, int mtu, u8 *mac,
				struct platform_device *pdev)
{
	int cpu, i;
	struct net_device *dev;
	struct eth_dev_priv *dev_priv;
	struct cpu_ctrl	*cpuCtrl;

	dev = alloc_etherdev_mq(sizeof(struct eth_dev_priv), CONFIG_MV_ETH_TXQ);
	if (!dev)
		return NULL;

	dev_priv = (struct eth_dev_priv *)netdev_priv(dev);
	if (!dev_priv)
		return NULL;

	memset(dev_priv, 0, sizeof(struct eth_dev_priv));
	dev_priv->port_p = pp;

	dev->irq = NET_TH_RXTX_IRQ_NUM(pp->port);

	dev->mtu = mtu;
	memcpy(dev->dev_addr, mac, MV_MAC_ADDR_SIZE);
	dev->tx_queue_len = CONFIG_MV_ETH_TXQ_DESC;
	dev->watchdog_timeo = 5 * HZ;

#ifdef CONFIG_MV_ETH_SWITCH
	if (pp->flags & (MV_ETH_F_SWITCH | MV_ETH_F_EXT_SWITCH)) {

		if (pp->flags & MV_ETH_F_SWITCH)
			dev->netdev_ops = &mv_switch_netdev_ops;
		else
			dev->netdev_ops = &mv_eth_netdev_ops;

		dev_priv->netdev_p = mvOsMalloc(sizeof(struct eth_netdev));
		if (!dev_priv->netdev_p) {
			printk(KERN_ERR "failed to allocate eth_netdev\n");
			free_netdev(dev);
			return NULL;
		}
		memset(dev_priv->netdev_p, 0, sizeof(struct eth_netdev));
		/* For correct link information of Linux interface: */
		if (pp->flags & MV_ETH_F_EXT_SWITCH) {
			dev_priv->netdev_p->port_map = ext_switch_port_mask;
			dev_priv->netdev_p->link_map = 0;
		}
	} else
#endif /* CONFIG_MV_ETH_SWITCH */
		dev->netdev_ops = &mv_eth_netdev_ops;

#ifdef CONFIG_MV_ETH_TOOL
	SET_ETHTOOL_OPS(dev, &mv_eth_tool_ops);
#endif

	/* Default NAPI initialization */
	for (i = 0; i < CONFIG_MV_ETH_NAPI_GROUPS; i++) {
		pp->napiGroup[i] = kmalloc(sizeof(struct napi_struct), GFP_KERNEL);
		memset(pp->napiGroup[i], 0, sizeof(struct napi_struct));
	}

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		cpuCtrl = pp->cpu_config[cpu];
		cpuCtrl->napiCpuGroup = 0;
		cpuCtrl->napi         = NULL;
	}

	/* Add NAPI default group */
	if (pp->flags & MV_ETH_F_CONNECT_LINUX) {
		for (i = 0; i < CONFIG_MV_ETH_NAPI_GROUPS; i++)
			netif_napi_add(dev, pp->napiGroup[i], mv_eth_poll, pp->weight);
	}
	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		cpuCtrl->tx_done_timer.data = (unsigned long)dev;
		cpuCtrl->cleanup_timer.data = (unsigned long)dev;
	}

	SET_NETDEV_DEV(dev, &pdev->dev);

	dev->priv_flags |= IFF_UNICAST_FLT;

	if (pp->flags & MV_ETH_F_CONNECT_LINUX) {
		mv_eth_netdev_set_features(dev);
		if (register_netdev(dev)) {
			printk(KERN_ERR "failed to register %s\n", dev->name);
			free_netdev(dev);
			return NULL;
		} else {

			/* register_netdev() always sets NETIF_F_GRO via NETIF_F_SOFT_FEATURES */

#ifndef CONFIG_MV_ETH_GRO_DEF
			dev->features &= ~NETIF_F_GRO;
#endif /* CONFIG_MV_ETH_GRO_DEF */

			printk(KERN_ERR "    o %s, ifindex = %d, GbE port = %d", dev->name, dev->ifindex, pp->port);
#ifdef MY_ABC_HERE
			printk("\n    o %s, phy chipid = %x, Support WOL = %d\n", dev->name, pp->phy_chip, syno_wol_support(pp));
#endif
#ifdef CONFIG_MV_ETH_SWITCH
			if (!(pp->flags & MV_ETH_F_SWITCH))
				printk(KERN_CONT "\n");
#else
			printk(KERN_CONT "\n");
#endif
		}
	}
	return dev;
}

bool mv_eth_netdev_find(unsigned int dev_idx)
{
	int i;

	for (i = 0; i < mv_net_devs_num; i++) {
		if (mv_net_devs && mv_net_devs[i] && (mv_net_devs[i]->ifindex == dev_idx))
			return true;
	}
	return false;
}
EXPORT_SYMBOL(mv_eth_netdev_find);

void mv_eth_netdev_update(int dev_index, struct eth_port *pp)
{
	int i;
	struct eth_dev_priv *dev_priv;
	struct cpu_ctrl *cpuCtrl;

#ifdef CONFIG_MV_ETH_SWITCH
	struct eth_netdev *eth_netdev_priv;
#endif /* CONFIG_MV_ETH_SWITCH */
	struct net_device *dev = mv_net_devs[dev_index];

	dev_priv = (struct eth_dev_priv *)netdev_priv(dev); /* assuming dev_priv has to be valid here */

	dev_priv->port_p = pp;

	dev->irq = NET_TH_RXTX_IRQ_NUM(pp->port);

	if (pp->flags & MV_ETH_F_CONNECT_LINUX) {
		for (i = 0; i < CONFIG_MV_ETH_NAPI_GROUPS; i++)
			netif_napi_add(dev, pp->napiGroup[i], mv_eth_poll, pp->weight);
	}

	for_each_possible_cpu(i) {
		cpuCtrl = pp->cpu_config[i];
		cpuCtrl->tx_done_timer.data = (unsigned long)dev;
		cpuCtrl->cleanup_timer.data = (unsigned long)dev;
	}

	printk(KERN_ERR "    o %s, ifindex = %d, GbE port = %d", dev->name, dev->ifindex, pp->port);

#ifdef CONFIG_MV_ETH_SWITCH
	if (pp->flags & MV_ETH_F_SWITCH) {
		eth_netdev_priv = MV_DEV_PRIV(dev);
		mv_eth_switch_priv_update(dev, MV_SWITCH_VLAN_TO_GROUP(eth_netdev_priv->vlan_grp_id));
	} else {
		printk(KERN_CONT "\n");
	}
#else
	printk(KERN_CONT "\n");
#endif /* CONFIG_MV_ETH_SWITCH */
}

int mv_eth_hal_init(struct eth_port *pp)
{
	int rxq, txp, txq, size, cpu;
	struct tx_queue *txq_ctrl;
	struct rx_queue *rxq_ctrl;

	if (!MV_PON_PORT(pp->port)) {
		int phyAddr;

		/* Set the board information regarding PHY address */
		phyAddr = mvBoardPhyAddrGet(pp->port);
		mvNetaPhyAddrSet(pp->port, phyAddr);
	}

	/* Init port */
	pp->port_ctrl = mvNetaPortInit(pp->port, NULL);
	if (!pp->port_ctrl) {
		printk(KERN_ERR "%s: failed to load port=%d\n", __func__, pp->port);
		return -ENODEV;
	}

	size = pp->txp_num * CONFIG_MV_ETH_TXQ * sizeof(struct tx_queue);
	pp->txq_ctrl = mvOsMalloc(size);
	if (!pp->txq_ctrl)
		goto oom;

	memset(pp->txq_ctrl, 0, size);

	/* Create TX descriptor rings */
	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

			txq_ctrl->q = NULL;
			txq_ctrl->hwf_rxp = 0xFF;
			txq_ctrl->txp = txp;
			txq_ctrl->txq = txq;
			txq_ctrl->txq_size = CONFIG_MV_ETH_TXQ_DESC;
			txq_ctrl->txq_count = 0;
			txq_ctrl->bm_only = MV_FALSE;

			txq_ctrl->shadow_txq_put_i = 0;
			txq_ctrl->shadow_txq_get_i = 0;
			txq_ctrl->txq_done_pkts_coal = mv_ctrl_txdone;
			txq_ctrl->flags = MV_ETH_F_TX_SHARED;
			txq_ctrl->nfpCounter = 0;
			for_each_possible_cpu(cpu)
				txq_ctrl->cpu_owner[cpu] = 0;
		}
	}

	pp->rxq_ctrl = mvOsMalloc(CONFIG_MV_ETH_RXQ * sizeof(struct rx_queue));
	if (!pp->rxq_ctrl)
		goto oom;

	memset(pp->rxq_ctrl, 0, CONFIG_MV_ETH_RXQ * sizeof(struct rx_queue));

	/* Create Rx descriptor rings */
	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		rxq_ctrl = &pp->rxq_ctrl[rxq];
		rxq_ctrl->rxq_size = CONFIG_MV_ETH_RXQ_DESC;
		rxq_ctrl->rxq_pkts_coal = CONFIG_MV_ETH_RX_COAL_PKTS;
		rxq_ctrl->rxq_time_coal = CONFIG_MV_ETH_RX_COAL_USEC;
	}

	if (pp->flags & MV_ETH_F_MH)
		mvNetaMhSet(pp->port, MV_NETA_MH);

#ifdef CONFIG_MV_ETH_TOOL
	/* Configure defaults */
	pp->autoneg_cfg  = AUTONEG_ENABLE;
	pp->speed_cfg    = SPEED_1000;
	pp->duplex_cfg  = DUPLEX_FULL;
	pp->advertise_cfg = 0x2f;
#endif /* CONFIG_MV_ETH_TOOL */

	return MV_OK;
oom:
	printk(KERN_ERR "%s: port=%d: out of memory\n", __func__, pp->port);
	return -ENODEV;
}

/* Show network driver configuration */
void mv_eth_config_show(void)
{
	/* Check restrictions */
#if (CONFIG_MV_ETH_PORTS_NUM > MV_ETH_MAX_PORTS)
#   error "CONFIG_MV_ETH_PORTS_NUM is large than MV_ETH_MAX_PORTS"
#endif

#if (CONFIG_MV_ETH_RXQ > MV_ETH_MAX_RXQ)
#   error "CONFIG_MV_ETH_RXQ is large than MV_ETH_MAX_RXQ"
#endif

#if CONFIG_MV_ETH_TXQ > MV_ETH_MAX_TXQ
#   error "CONFIG_MV_ETH_TXQ is large than MV_ETH_MAX_TXQ"
#endif

#if defined(CONFIG_MV_ETH_TSO) && !defined(CONFIG_MV_ETH_TX_CSUM_OFFLOAD)
#   error "If GSO enabled - TX checksum offload must be enabled too"
#endif

	printk(KERN_ERR "  o %d Giga ports supported\n", CONFIG_MV_ETH_PORTS_NUM);

#ifdef CONFIG_MV_PON
	printk(KERN_ERR "  o Giga PON port is #%d: - %d TCONTs supported\n", MV_PON_PORT_ID, MV_ETH_MAX_TCONT());
#endif

#ifdef CONFIG_NET_SKB_RECYCLE
	printk(KERN_ERR "  o SKB recycle supported (%s)\n", mv_ctrl_recycle ? "Enabled" : "Disabled");
#endif

#ifdef CONFIG_MV_ETH_NETA
	printk(KERN_ERR "  o NETA acceleration mode %d\n", mvNetaAccMode());
#endif

#ifdef CONFIG_MV_ETH_BM_CPU
	printk(KERN_ERR "  o BM supported for CPU: %d BM pools\n", MV_ETH_BM_POOLS);
#endif /* CONFIG_MV_ETH_BM_CPU */

#ifdef CONFIG_MV_ETH_PNC
	printk(KERN_ERR "  o PnC supported (%s)\n", mv_eth_pnc_ctrl_en ? "Enabled" : "Disabled");
#endif

#ifdef CONFIG_MV_ETH_HWF
	printk(KERN_ERR "  o HWF supported\n");
#endif

#ifdef CONFIG_MV_ETH_PMT
	printk(KERN_ERR "  o PMT supported\n");
#endif

	printk(KERN_ERR "  o RX Queue support: %d Queues * %d Descriptors\n", CONFIG_MV_ETH_RXQ, CONFIG_MV_ETH_RXQ_DESC);

	printk(KERN_ERR "  o TX Queue support: %d Queues * %d Descriptors\n", CONFIG_MV_ETH_TXQ, CONFIG_MV_ETH_TXQ_DESC);

#if defined(CONFIG_MV_ETH_TSO)
	printk(KERN_ERR "  o GSO supported\n");
#endif /* CONFIG_MV_ETH_TSO */

#if defined(CONFIG_MV_ETH_GRO)
	printk(KERN_ERR "  o GRO supported\n");
#endif /* CONFIG_MV_ETH_GRO */

#if defined(CONFIG_MV_ETH_RX_CSUM_OFFLOAD)
	printk(KERN_ERR "  o Receive checksum offload supported\n");
#endif
#if defined(CONFIG_MV_ETH_TX_CSUM_OFFLOAD)
	printk(KERN_ERR "  o Transmit checksum offload supported\n");
#endif

#if defined(CONFIG_MV_ETH_NFP)
	printk(KERN_ERR "  o NFP is supported\n");
#endif /* CONFIG_MV_ETH_NFP */

#if defined(CONFIG_MV_ETH_NFP_HOOKS)
	printk(KERN_ERR "  o NFP Hooks are supported\n");
#endif /* CONFIG_MV_ETH_NFP_HOOKS */

#if defined(CONFIG_MV_ETH_NFP_EXT)
	printk(KERN_ERR "  o NFP External drivers supported: up to %d interfaces\n", NFP_EXT_NUM);
#endif /* CONFIG_MV_ETH_NFP_EXT */

#ifdef CONFIG_MV_ETH_STAT_ERR
	printk(KERN_ERR "  o Driver ERROR statistics enabled\n");
#endif

#ifdef CONFIG_MV_ETH_STAT_INF
	printk(KERN_ERR "  o Driver INFO statistics enabled\n");
#endif

#ifdef CONFIG_MV_ETH_STAT_DBG
	printk(KERN_ERR "  o Driver DEBUG statistics enabled\n");
#endif

#ifdef ETH_DEBUG
	printk(KERN_ERR "  o Driver debug messages enabled\n");
#endif

#if defined(CONFIG_MV_ETH_SWITCH)
	printk(KERN_ERR "  o Switch support enabled\n");

#endif /* CONFIG_MV_ETH_SWITCH */

	printk(KERN_ERR "\n");
}

/* Set network device features on initialization. Take into account default compile time configuration. */
static void mv_eth_netdev_set_features(struct net_device *dev)
{
	dev->features 	 = NETIF_F_SG  | NETIF_F_LLTX;
	dev->hw_features = NETIF_F_IP_CSUM | NETIF_F_SG;

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	dev->features |= NETIF_F_NTUPLE;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
	dev->hw_features |= NETIF_F_RXHASH;
#endif

#ifdef CONFIG_MV_ETH_TX_CSUM_OFFLOAD_DEF
	if (dev->mtu <= MV_ETH_TX_CSUM_MAX_SIZE)
		dev->features |= NETIF_F_IP_CSUM;
#endif /* CONFIG_MV_ETH_TX_CSUM_OFFLOAD_DEF */

#ifdef CONFIG_MV_ETH_TSO_DEF
	if (dev->features & NETIF_F_IP_CSUM)
		dev->features |= NETIF_F_TSO;
#endif /* CONFIG_MV_ETH_TSO_DEF */

}

/* Update network device features after changing MTU.	*/
static void mv_eth_netdev_update_features(struct net_device *dev)
{
#ifdef CONFIG_MV_ETH_TX_CSUM_OFFLOAD
	if (dev->mtu > MV_ETH_TX_CSUM_MAX_SIZE) {
		dev->features &= ~NETIF_F_IP_CSUM;
		printk(KERN_ERR "Removing NETIF_F_IP_CSUM in device %s features\n", dev->name);
	}
#endif /* CONFIG_MV_ETH_TX_CSUM_OFFLOAD */

#ifdef CONFIG_MV_ETH_TSO
	if (!(dev->features & NETIF_F_IP_CSUM)) {
		dev->features &= ~NETIF_F_TSO;
		printk(KERN_ERR "Removing NETIF_F_TSO in device %s features\n", dev->name);
	}
#endif /* CONFIG_MV_ETH_TSO */
}

int mv_eth_napi_set_cpu_affinity(int port, int group, int affinity)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (pp == NULL) {
		printk(KERN_ERR "%s: pp == NULL, port=%d\n", __func__, port);
		return MV_ERROR;
	}

	if (group >= CONFIG_MV_ETH_NAPI_GROUPS) {
		printk(KERN_ERR "%s: group number is higher than %d\n", __func__, CONFIG_MV_ETH_NAPI_GROUPS-1);
		return MV_ERROR;
		}
	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}
	set_cpu_affinity(pp, affinity, group);
	return MV_OK;

}
void handle_group_affinity(int port)
{
	int group;
	struct eth_port *pp;
	MV_U32 group_cpu_affinity[CONFIG_MV_ETH_NAPI_GROUPS];
	MV_U32 rxq_affinity[CONFIG_MV_ETH_NAPI_GROUPS];

	group_cpu_affinity[0] = CONFIG_MV_ETH_GROUP0_CPU;
	rxq_affinity[0] 	  = CONFIG_MV_ETH_GROUP0_RXQ;

#ifdef CONFIG_MV_ETH_GROUP1_CPU
		group_cpu_affinity[1] = CONFIG_MV_ETH_GROUP1_CPU;
		rxq_affinity[1] 	  = CONFIG_MV_ETH_GROUP1_RXQ;
#endif

#ifdef CONFIG_MV_ETH_GROUP2_CPU
		group_cpu_affinity[2] = CONFIG_MV_ETH_GROUP2_CPU;
		rxq_affinity[2] 	  = CONFIG_MV_ETH_GROUP2_RXQ;
#endif

#ifdef CONFIG_MV_ETH_GROUP3_CPU
		group_cpu_affinity[3] = CONFIG_MV_ETH_GROUP3_CPU;
		rxq_affinity[3] 	  = CONFIG_MV_ETH_GROUP3_RXQ;
#endif

	pp = mv_eth_port_by_id(port);
	if (pp == NULL)
		return;

	for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
		set_cpu_affinity(pp, group_cpu_affinity[group], group);
	for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
		set_rxq_affinity(pp, rxq_affinity[group], group);

}

int	mv_eth_napi_set_rxq_affinity(int port, int group, int rxqAffinity)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp is null \n", __func__);
		return MV_FAIL;
	}
	if (group >= CONFIG_MV_ETH_NAPI_GROUPS) {
		printk(KERN_ERR "%s: group number is higher than %d\n", __func__, CONFIG_MV_ETH_NAPI_GROUPS-1);
		return MV_ERROR;
		}
	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	set_rxq_affinity(pp, rxqAffinity, group);
	return MV_OK;
}

void mv_eth_napi_group_show(int port)
{
	int cpu, group;
	struct cpu_ctrl	*cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp == NULL\n", __func__);
		return;
	}
	for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++) {
		printk(KERN_INFO "group=%d:\n", group);
		for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
			cpuCtrl = pp->cpu_config[cpu];
			if (!(MV_BIT_CHECK(pp->cpuMask, cpu)))
				continue;
			if (cpuCtrl->napiCpuGroup == group) {
				printk(KERN_INFO "   CPU%d ", cpu);
				mvNetaCpuDump(port, cpu, 0);
				printk(KERN_INFO "\n");
			}
		}
		printk(KERN_INFO "\n");
	}
}

void mv_eth_priv_cleanup(struct eth_port *pp)
{
	/* TODO */
}

#ifdef CONFIG_MV_ETH_BM_CPU
static struct bm_pool *mv_eth_long_pool_get(struct eth_port *pp, int pkt_size)
{
	int             pool, i;
	struct bm_pool	*bm_pool, *temp_pool = NULL;
	unsigned long   flags = 0;

	pool = mv_eth_bm_config_long_pool_get(pp->port);
	if (pool != -1) /* constant long pool for the port */
		return &mv_eth_pool[pool];

	/* look for free pool pkt_size == 0. First check pool == pp->port */
	/* if no free pool choose larger than required */
	for (i = 0; i < MV_ETH_BM_POOLS; i++) {
		pool = (pp->port + i) % MV_ETH_BM_POOLS;
		bm_pool = &mv_eth_pool[pool];

		MV_ETH_LOCK(&bm_pool->lock, flags);

		if (bm_pool->pkt_size == 0) {
			/* found free pool */
			 
			MV_ETH_UNLOCK(&bm_pool->lock, flags);
			return bm_pool;
		}
		if (bm_pool->pkt_size >= pkt_size) {
			if (temp_pool == NULL)
				temp_pool = bm_pool;
			else if (bm_pool->pkt_size < temp_pool->pkt_size)
				temp_pool = bm_pool;
		}
		MV_ETH_UNLOCK(&bm_pool->lock, flags);
	}
	return temp_pool;
}
#else
static struct bm_pool *mv_eth_long_pool_get(struct eth_port *pp, int pkt_size)
{
	return &mv_eth_pool[pp->port];
}
#endif /* CONFIG_MV_ETH_BM_CPU */

static int mv_eth_rxq_fill(struct eth_port *pp, int rxq, int num)
{
	int i;

#ifndef CONFIG_MV_ETH_BM_CPU
	struct eth_pbuf *pkt;
	struct bm_pool *bm_pool;
	MV_NETA_RXQ_CTRL *rx_ctrl;
	struct neta_rx_desc *rx_desc;

	bm_pool = pp->pool_long;

	rx_ctrl = pp->rxq_ctrl[rxq].q;
	if (!rx_ctrl) {
		printk(KERN_ERR "%s: rxq %d is not initialized\n", __func__, rxq);
		return MV_OK;
	}

	for (i = 0; i < num; i++) {
		pkt = mv_eth_pool_get(bm_pool);
		if (pkt) {
			rx_desc = (struct neta_rx_desc *)MV_NETA_QUEUE_DESC_PTR(&rx_ctrl->queueCtrl, i);
			memset(rx_desc, 0, sizeof(struct neta_rx_desc));

			mvNetaRxDescFill(rx_desc, pkt->physAddr, (MV_U32)pkt);
		} else {
			printk(KERN_ERR "%s: rxq %d, %d of %d buffers are filled\n", __func__, rxq, i, num);
			break;
		}
	}
#else
	i = num;
#endif /* CONFIG_MV_ETH_BM_CPU */

	mvNetaRxqNonOccupDescAdd(pp->port, rxq, i);

	return i;
}

static int mv_eth_txq_create(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	txq_ctrl->q = mvNetaTxqInit(pp->port, txq_ctrl->txp, txq_ctrl->txq, txq_ctrl->txq_size);
	if (txq_ctrl->q == NULL) {
		printk(KERN_ERR "%s: can't create TxQ - port=%d, txp=%d, txq=%d, desc=%d\n",
		       __func__, pp->port, txq_ctrl->txp, txq_ctrl->txp, txq_ctrl->txq_size);
		return -ENODEV;
	}

	txq_ctrl->shadow_txq = mvOsMalloc(txq_ctrl->txq_size * sizeof(MV_ULONG));
	if (txq_ctrl->shadow_txq == NULL)
		goto no_mem;

	/* reset txq */
	txq_ctrl->txq_count = 0;
	txq_ctrl->shadow_txq_put_i = 0;
	txq_ctrl->shadow_txq_get_i = 0;

#ifdef CONFIG_MV_ETH_HWF
	mvNetaHwfTxqInit(pp->port, txq_ctrl->txp, txq_ctrl->txq);
#endif /* CONFIG_MV_ETH_HWF */

	return MV_OK;

no_mem:
	mv_eth_txq_delete(pp, txq_ctrl);
	return -ENOMEM;
}

static int mv_force_port_link_speed_fc(int port, MV_ETH_PORT_SPEED port_speed, int en_force)
{
	if (en_force) {
		if (mvNetaForceLinkModeSet(port, 1, 0)) {
			printk(KERN_ERR "mvNetaForceLinkModeSet failed\n");
			return -EIO;
		}
		if (mvNetaSpeedDuplexSet(port, port_speed, MV_ETH_DUPLEX_FULL)) {
			printk(KERN_ERR "mvNetaSpeedDuplexSet failed\n");
			return -EIO;
		}
		if (mvNetaFlowCtrlSet(port, MV_ETH_FC_ENABLE)) {
			printk(KERN_ERR "mvNetaFlowCtrlSet failed\n");
			return -EIO;
		}
	} else {
		if (mvNetaForceLinkModeSet(port, 0, 0)) {
			printk(KERN_ERR "mvNetaForceLinkModeSet failed\n");
			return -EIO;
		}
		if (mvNetaSpeedDuplexSet(port, MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN)) {
			printk(KERN_ERR "mvNetaSpeedDuplexSet failed\n");
			return -EIO;
		}
		if (mvNetaFlowCtrlSet(port, MV_ETH_FC_AN_SYM)) {
			printk(KERN_ERR "mvNetaFlowCtrlSet failed\n");
			return -EIO;
		}
	}
	return MV_OK;
}

static void mv_eth_txq_delete(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	if (txq_ctrl->shadow_txq) {
		mvOsFree(txq_ctrl->shadow_txq);
		txq_ctrl->shadow_txq = NULL;
	}

	if (txq_ctrl->q) {
		mvNetaTxqDelete(pp->port, txq_ctrl->txp, txq_ctrl->txq);
		txq_ctrl->q = NULL;
	}
}

/* Free all packets pending transmit from all TXQs and reset TX port */
int mv_eth_txp_reset(int port, int txp)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	int queue;

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	/* free the skb's in the hal tx ring */
	for (queue = 0; queue < CONFIG_MV_ETH_TXQ; queue++) {
		struct tx_queue *txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + queue];

		if (txq_ctrl->q)
			mv_eth_txq_done_force(pp, txq_ctrl);
	}
	mvNetaTxpReset(port, txp);
	return MV_OK;
}

/* Free received packets from all RXQs and reset RX of the port */
int mv_eth_rx_reset(int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

#ifndef CONFIG_MV_ETH_BM_CPU
	{
		int rxq = 0;

		for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
			struct eth_pbuf *pkt;
			struct neta_rx_desc *rx_desc;
			struct bm_pool *pool;
			int i, rx_done;
			MV_NETA_RXQ_CTRL *rx_ctrl = pp->rxq_ctrl[rxq].q;

			if (rx_ctrl == NULL)
				continue;

			rx_done = mvNetaRxqFreeDescNumGet(pp->port, rxq);
			mvOsCacheIoSync();
			for (i = 0; i < rx_done; i++) {
				rx_desc = mvNetaRxqNextDescGet(rx_ctrl);
				mvOsCacheLineInv(NULL, rx_desc);

#if defined(MV_CPU_BE)
				mvNetaRxqDescSwap(rx_desc);
#endif /* MV_CPU_BE */

				pkt = (struct eth_pbuf *)rx_desc->bufCookie;
				pool = &mv_eth_pool[pkt->pool];
				mv_eth_pool_put(pool, pkt);
			}
		}
	}
#endif /* CONFIG_MV_ETH_BM_CPU */

	mvNetaRxReset(port);
	return MV_OK;
}

/***********************************************************
 * coal set functions		                           *
 ***********************************************************/
MV_STATUS mv_eth_rx_ptks_coal_set(int port, int rxq, MV_U32 value)
{
	MV_STATUS status = mvNetaRxqPktsCoalSet(port, rxq, value);
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (status == MV_OK)
		pp->rxq_ctrl[rxq].rxq_pkts_coal = value;
	return status;
}

MV_STATUS mv_eth_rx_time_coal_set(int port, int rxq, MV_U32 value)
{
	MV_STATUS status = mvNetaRxqTimeCoalSet(port, rxq, value);
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (status == MV_OK)
		pp->rxq_ctrl[rxq].rxq_time_coal = value;
	return status;
}

MV_STATUS mv_eth_tx_done_ptks_coal_set(int port, int txp, int txq, MV_U32 value)
{
	MV_STATUS status = mvNetaTxDonePktsCoalSet(port, txp, txq, value);
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (status == MV_OK)
		pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq].txq_done_pkts_coal = value;
	return status;
}

/***********************************************************
 * mv_eth_start_internals --                               *
 *   fill rx buffers. start rx/tx activity. set coalesing. *
 *   clear and unmask interrupt bits                       *
 ***********************************************************/
int mv_eth_start_internals(struct eth_port *pp, int mtu)
{
	unsigned int status;
	struct cpu_ctrl	*cpuCtrl;
	int rxq, txp, txq, num, err = 0;
	int pkt_size = RX_PKT_SIZE(mtu);
	MV_BOARD_MAC_SPEED mac_speed;

	if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_ERR(pp->stats.state_err++);
		printk(KERN_ERR "%s: port %d, wrong state: STARTED_BIT = 1\n", __func__, pp->port);
		err = -EINVAL;
		goto out;
	}

	if (mvNetaMaxRxSizeSet(pp->port, RX_PKT_SIZE(mtu))) {
		printk(KERN_ERR "%s: can't set maxRxSize=%d for port=%d, mtu=%d\n",
		       __func__, RX_PKT_SIZE(mtu), pp->port, mtu);
		err = -EINVAL;
		goto out;
	}

	if (mv_eth_ctrl_is_tx_enabled(pp)) {
		int cpu;
		for_each_possible_cpu(cpu) {
			if (!(MV_BIT_CHECK(pp->cpuMask, cpu)))
				continue;

			cpuCtrl = pp->cpu_config[cpu];

			if (!(MV_BIT_CHECK(cpuCtrl->cpuTxqMask, cpuCtrl->txq))) {
				printk(KERN_ERR "%s: error , port #%d txq #%d is masked for cpu #%d (mask= 0X%x).\n",
					__func__, pp->port, cpuCtrl->txq, cpu, cpuCtrl->cpuTxqMask);
				err = -EINVAL;
				goto out;
			}

			if (mv_eth_ctrl_txq_cpu_own(pp->port, pp->txp, cpuCtrl->txq, 1, cpu) < 0) {
				err = -EINVAL;
				goto out;
			}
		}
	}

	/* Allocate buffers for Long buffers pool */
	if (pp->pool_long == NULL) {
		struct bm_pool *new_pool;

		new_pool = mv_eth_long_pool_get(pp, pkt_size);
		if (new_pool == NULL) {
			printk(KERN_ERR "%s FAILED: port=%d, Can't find pool for pkt_size=%d\n",
			       __func__, pp->port, pkt_size);
			err = -ENOMEM;
			goto out;
		}
		if (new_pool->pkt_size == 0) {
			new_pool->pkt_size = pkt_size;
#ifdef CONFIG_MV_ETH_BM_CPU
			mvBmPoolBufSizeSet(new_pool->pool, RX_BUF_SIZE(pkt_size));
#endif /* CONFIG_MV_ETH_BM_CPU */
		}
		if (new_pool->pkt_size < pkt_size) {
			printk(KERN_ERR "%s FAILED: port=%d, long pool #%d, pkt_size=%d less than required %d\n",
					__func__, pp->port, new_pool->pool, new_pool->pkt_size, pkt_size);
			err = -ENOMEM;
			goto out;
		}
		pp->pool_long = new_pool;
		pp->pool_long->port_map |= (1 << pp->port);

		num = mv_eth_pool_add(pp->pool_long->pool, pp->pool_long_num);
		if (num != pp->pool_long_num) {
			printk(KERN_ERR "%s FAILED: mtu=%d, pool=%d, pkt_size=%d, only %d of %d allocated\n",
			       __func__, mtu, pp->pool_long->pool, pp->pool_long->pkt_size, num, pp->pool_long_num);
			err = -ENOMEM;
			goto out;
		}
	}

#ifdef CONFIG_MV_ETH_BM_CPU
	mvNetaBmPoolBufSizeSet(pp->port, pp->pool_long->pool, RX_BUF_SIZE(pp->pool_long->pkt_size));

	if (pp->pool_short == NULL) {
		int short_pool = mv_eth_bm_config_short_pool_get(pp->port);

		/* Allocate packets for short pool */
		if (short_pool < 0) {
			err = -EINVAL;
			goto out;
		}
		pp->pool_short = &mv_eth_pool[short_pool];
		pp->pool_short->port_map |= (1 << pp->port);
		if (pp->pool_short->pool != pp->pool_long->pool) {
			num = mv_eth_pool_add(pp->pool_short->pool, pp->pool_short_num);
			if (num != pp->pool_short_num) {
				printk(KERN_ERR "%s FAILED: pool=%d, pkt_size=%d - %d of %d buffers added\n",
					   __func__, short_pool, pp->pool_short->pkt_size, num, pp->pool_short_num);
				err = -ENOMEM;
				goto out;
			}
			mvNetaBmPoolBufSizeSet(pp->port, pp->pool_short->pool, RX_BUF_SIZE(pp->pool_short->pkt_size));
		} else {
			int dummy_short_pool = (pp->pool_short->pool + 1) % MV_BM_POOLS;

			/* To disable short pool we choose unused pool and set pkt size to 0 (buffer size = pkt offset) */
			mvNetaBmPoolBufSizeSet(pp->port, dummy_short_pool, NET_SKB_PAD);
		}
	}
#endif /* CONFIG_MV_ETH_BM_CPU */

	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		if (pp->rxq_ctrl[rxq].q == NULL) {
			pp->rxq_ctrl[rxq].q = mvNetaRxqInit(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_size);
			if (!pp->rxq_ctrl[rxq].q) {
				printk(KERN_ERR "%s: can't create RxQ port=%d, rxq=%d, desc=%d\n",
				       __func__, pp->port, rxq, pp->rxq_ctrl[rxq].rxq_size);
				err = -ENODEV;
				goto out;
			}
		}

		/* Set Offset */
		mvNetaRxqOffsetSet(pp->port, rxq, NET_SKB_PAD);

		/* Set coalescing pkts and time */
		mv_eth_rx_ptks_coal_set(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_pkts_coal);
		mv_eth_rx_time_coal_set(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_time_coal);

#if defined(CONFIG_MV_ETH_BM_CPU)
		/* Enable / Disable - BM support */
		if (pp->pool_short->pool == pp->pool_long->pool) {
			int dummy_short_pool = (pp->pool_short->pool + 1) % MV_BM_POOLS;

			/* To disable short pool we choose unused pool and set pkt size to 0 (buffer size = pkt offset) */
			mvNetaRxqBmEnable(pp->port, rxq, dummy_short_pool, pp->pool_long->pool);
		} else
			mvNetaRxqBmEnable(pp->port, rxq, pp->pool_short->pool, pp->pool_long->pool);
#else
		/* Fill RXQ with buffers from RX pool */
		mvNetaRxqBufSizeSet(pp->port, rxq, RX_BUF_SIZE(pkt_size));
		mvNetaRxqBmDisable(pp->port, rxq);
#endif /* CONFIG_MV_ETH_BM_CPU */

		if (mvNetaRxqFreeDescNumGet(pp->port, rxq) == 0)
			mv_eth_rxq_fill(pp, rxq, pp->rxq_ctrl[rxq].rxq_size);
	}

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			struct tx_queue *txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

			if ((txq_ctrl->q == NULL) && (txq_ctrl->txq_size > 0)) {
				err = mv_eth_txq_create(pp, txq_ctrl);
				if (err)
					goto out;
				spin_lock_init(&txq_ctrl->queue_lock);
			}
			mv_eth_tx_done_ptks_coal_set(pp->port, txp, txq,
					pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq].txq_done_pkts_coal);
		}
		mvNetaTxpMaxTxSizeSet(pp->port, txp, RX_PKT_SIZE(mtu));
	}

#ifdef CONFIG_MV_ETH_HWF
#ifdef CONFIG_MV_ETH_BM_CPU
	mvNetaHwfBmPoolsSet(pp->port, pp->pool_short->pool, pp->pool_long->pool);
#else
	mv_eth_hwf_bm_create(pp->port, RX_PKT_SIZE(mtu));
#endif /* CONFIG_MV_ETH_BM_CPU */

	mvNetaHwfEnable(pp->port, 1);
#endif /* CONFIG_MV_ETH_HWF */

	if (!MV_PON_PORT(pp->port)) {
		/* force link, speed and duplex if necessary (e.g. Switch is connected) based on board information */
		mac_speed = mvBoardMacSpeedGet(pp->port);
		switch (mac_speed) {
		case BOARD_MAC_SPEED_10M:
			err = mv_force_port_link_speed_fc(pp->port, MV_ETH_SPEED_10, 1);
			if (err)
				goto out;
			break;
		case BOARD_MAC_SPEED_100M:
			err = mv_force_port_link_speed_fc(pp->port, MV_ETH_SPEED_100, 1);
			if (err)
				goto out;
			break;
		case BOARD_MAC_SPEED_1000M:
			err = mv_force_port_link_speed_fc(pp->port, MV_ETH_SPEED_1000, 1);
			if (err)
				goto out;
			break;
		case BOARD_MAC_SPEED_AUTO:
		default:
			/* do nothing */
			break;
		}
	}

	/* start the hal - rx/tx activity */
	status = mvNetaPortEnable(pp->port);
	if (status == MV_OK)
		set_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));
#ifdef CONFIG_MV_PON
	else if (MV_PON_PORT(pp->port) && (mv_pon_link_status() == MV_TRUE)) {
		mvNetaPortUp(pp->port);
		set_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));
	}
#endif /* CONFIG_MV_PON */

	set_bit(MV_ETH_F_STARTED_BIT, &(pp->flags));

 out:
	return err;
}

int mv_eth_resume_internals(struct eth_port *pp, int mtu)
{

	unsigned int status;

	mvNetaMaxRxSizeSet(pp->port, RX_PKT_SIZE(mtu));

#ifdef CONFIG_MV_ETH_HWF
#ifdef CONFIG_MV_ETH_BM_CPU
	if (pp->pool_long && pp->pool_short)
		mvNetaHwfBmPoolsSet(pp->port, pp->pool_short->pool, pp->pool_long->pool);
#else
	/* TODO - update func if we want to support HWF */
	mv_eth_hwf_bm_create(pp->port, RX_PKT_SIZE(mtu));
#endif /* CONFIG_MV_ETH_BM_CPU */

	mvNetaHwfEnable(pp->port, 1);

#endif /* CONFIG_MV_ETH_HWF */

	/* start the hal - rx/tx activity */
	status = mvNetaPortEnable(pp->port);
	if (status == MV_OK)
		set_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));

#ifdef CONFIG_MV_PON
	else if (MV_PON_PORT(pp->port) && (mv_pon_link_status() == MV_TRUE)) {
		mvNetaPortUp(pp->port);
		set_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));
	}
#endif /* CONFIG_MV_PON */

	return MV_OK;

}

/***********************************************************
 * mv_eth_suspend_internals --                             *
 *   stop port rx/tx activity. free skb's from rx/tx rings.*
 ***********************************************************/
int mv_eth_suspend_internals(struct eth_port *pp)
{
	int cpu;

	/* stop the port activity*/
	if (mvNetaPortDisable(pp->port) != MV_OK) {
		printk(KERN_ERR "%s: GbE port %d: mvNetaPortDisable failed\n", __func__, pp->port);
		return MV_ERROR;
	}

	/* mask all interrupts */
	mv_eth_interrupts_mask(pp);
	smp_call_function_many(cpu_online_mask, (smp_call_func_t)mv_eth_interrupts_mask, (void *)pp, 1);

	for_each_possible_cpu(cpu) {
		pp->cpu_config[cpu]->causeRxTx = 0;
		if (pp->cpu_config[cpu]->napi)
			/* TODO : check napi status, MV_ETH_F_STARTED_OLD_BIT is not exactly the bit we should look at */
			if (test_bit(MV_ETH_F_STARTED_OLD_BIT, &(pp->flags)))
				napi_synchronize(pp->cpu_config[cpu]->napi);
	}

	mdelay(10);

	return MV_OK;
}

/***********************************************************
 * mv_eth_stop_internals --                                *
 *   stop port rx/tx activity. free skb's from rx/tx rings.*
 ***********************************************************/
int mv_eth_stop_internals(struct eth_port *pp)
{

	int queue;
	struct cpu_ctrl	*cpuCtrl;

	if (!test_and_clear_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_ERR(pp->stats.state_err++);
		printk(KERN_ERR "%s: port %d, wrong state: STARTED_BIT = 0.\n", __func__, pp->port);
		goto error;
	}

	/* stop the port activity, mask all interrupts */
	if (mvNetaPortDisable(pp->port) != MV_OK) {
		printk(KERN_ERR "GbE port %d: ethPortDisable failed\n", pp->port);
		goto error;
	}

	mv_eth_interrupts_mask(pp);

	mdelay(10);

#ifdef CONFIG_MV_ETH_HWF
	mvNetaHwfEnable(pp->port, 0);
#else
	{
		int txp;
		/* Reset TX port here. If HWF is supported reset must be called externally */
		for (txp = 0; txp < pp->txp_num; txp++)
			mv_eth_txp_reset(pp->port, txp);
	}
#endif /* !CONFIG_MV_ETH_HWF */

	if (mv_eth_ctrl_is_tx_enabled(pp)) {
		int cpu;
		for_each_possible_cpu(cpu) {
			cpuCtrl = pp->cpu_config[cpu];
			if (MV_BIT_CHECK(pp->cpuMask, cpu))
				if (MV_BIT_CHECK(cpuCtrl->cpuTxqMask, cpuCtrl->txq))
					mv_eth_ctrl_txq_cpu_own(pp->port, pp->txp, cpuCtrl->txq, 0, cpu);
		}
	}

	/* free the skb's in the hal rx ring */
	for (queue = 0; queue < CONFIG_MV_ETH_RXQ; queue++)
		mv_eth_rxq_drop_pkts(pp, queue);

	return MV_OK;

error:
	printk(KERN_ERR "GbE port %d: stop internals failed\n", pp->port);
	return MV_ERROR;
}

/* return positive if MTU is valid */
int mv_eth_check_mtu_valid(struct net_device *dev, int mtu)
{
	if (mtu < 68) {
		printk(KERN_INFO "MTU must be at least 68, change mtu failed\n");
		return -EINVAL;
	}
	if (mtu > 9676 /* 9700 - 20 and rounding to 8 */) {
		printk(KERN_ERR "%s: Illegal MTU value %d, ", dev->name, mtu);
		mtu = 9676;
		printk(KERN_CONT " rounding MTU to: %d \n", mtu);
	}

	if (MV_IS_NOT_ALIGN(RX_PKT_SIZE(mtu), 8)) {
		printk(KERN_ERR "%s: Illegal MTU value %d, ", dev->name, mtu);
		mtu = MV_ALIGN_UP(RX_PKT_SIZE(mtu), 8);
		printk(KERN_CONT " rounding MTU to: %d \n", mtu);
	}
	return mtu;
}

/* Check if MTU can be changed */
int mv_eth_check_mtu_internals(struct net_device *dev, int mtu)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);
	struct bm_pool	*new_pool = NULL;

	new_pool = mv_eth_long_pool_get(pp, RX_PKT_SIZE(mtu));

	/* If no pool for new MTU or shared pool is used - MTU can be changed only when interface is stopped */
	if (new_pool == NULL) {
		printk(KERN_ERR "%s: No BM pool available for MTU=%d\n", __func__, mtu);
		return -EPERM;
	}
#ifdef CONFIG_MV_ETH_BM_CPU
	if (new_pool->pkt_size < RX_PKT_SIZE(mtu)) {
		if (mv_eth_bm_config_pkt_size_get(new_pool->pool) != 0) {
			printk(KERN_ERR "%s: BM pool #%d - pkt_size = %d less than required for MTU=%d and can't be changed\n",
						__func__, new_pool->pool, new_pool->pkt_size, mtu);
			return -EPERM;
		}
		/* Pool packet size can be changed for new MTU, but pool is shared */
		if ((new_pool == pp->pool_long) && (pp->pool_long->port_map != (1 << pp->port))) {
			/* Shared pool */
			printk(KERN_ERR "%s: bmPool=%d is shared port_map=0x%x. Stop all ports uses this pool before change MTU\n",
						__func__, pp->pool_long->pool, pp->pool_long->port_map);
			return -EPERM;
		}
	}
#endif /* CONFIG_MV_ETH_BM_CPU */
	return MV_OK;
}

/***********************************************************
 * mv_eth_change_mtu_internals --                          *
 *   stop port activity. release skb from rings. set new   *
 *   mtu in device and hw. restart port activity and       *
 *   and fill rx-buiffers with size according to new mtu.  *
 ***********************************************************/
int mv_eth_change_mtu_internals(struct net_device *dev, int mtu)
{
	struct bm_pool	*new_pool = NULL;
	struct eth_port *pp = MV_ETH_PRIV(dev);
	int             config_pkt_size;

	if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_ERR(pp->stats.state_err++);
		if (pp->flags & MV_ETH_F_DBG_RX)
			printk(KERN_ERR "%s: port %d, STARTED_BIT = 0, Invalid value.\n", __func__, pp->port);
		return MV_ERROR;
	}

	if ((mtu != dev->mtu) && (pp->pool_long)) {
		/* If long pool assigned and MTU really changed and can't use old pool - free buffers */
		 
		config_pkt_size = 0;
#ifdef CONFIG_MV_ETH_BM_CPU
		new_pool = mv_eth_long_pool_get(pp, RX_PKT_SIZE(mtu));
		if (new_pool != NULL)
			config_pkt_size = mv_eth_bm_config_pkt_size_get(new_pool->pool);
#else
		/* If BM is not used always free buffers */
		new_pool = NULL;
#endif /* CONFIG_MV_ETH_BM_CPU */

		/* Free all buffers from long pool */
		if ((new_pool == NULL) || (new_pool->pkt_size < RX_PKT_SIZE(mtu)) || (pp->pool_long != new_pool) ||
			((new_pool->pkt_size > RX_PKT_SIZE(mtu)) && (config_pkt_size == 0))) {
			mv_eth_rx_reset(pp->port);
			mv_eth_pool_free(pp->pool_long->pool, pp->pool_long_num);

#ifdef CONFIG_MV_ETH_BM_CPU
			/* redefine pool pkt_size */
			if (pp->pool_long->buf_num == 0) {
				pp->pool_long->pkt_size = config_pkt_size;

				if (pp->pool_long->pkt_size == 0)
					mvBmPoolBufSizeSet(pp->pool_long->pool, 0);
				else
					mvBmPoolBufSizeSet(pp->pool_long->pool, RX_BUF_SIZE(pp->pool_long->pkt_size));
			}
#else
			pp->pool_long->pkt_size = config_pkt_size;
#endif /* CONFIG_MV_ETH_BM_CPU */

			pp->pool_long->port_map &= ~(1 << pp->port);
			pp->pool_long = NULL;
		}

		/* DIMA debug; Free all buffers from short pool */
/*
		if (pp->pool_short) {
			mv_eth_pool_free(pp->pool_short->pool, pp->pool_short_num);
			pp->pool_short = NULL;
		}
*/
	}
	dev->mtu = mtu;

	mv_eth_netdev_update_features(dev);

	return MV_OK;
}

/***********************************************************
 * mv_eth_tx_done_timer_callback --			   *
 *   N msec periodic callback for tx_done                  *
 ***********************************************************/
static void mv_eth_tx_done_timer_callback(unsigned long data)
{
	struct cpu_ctrl *cpuCtrl;
	struct net_device *dev = (struct net_device *)data;
	struct eth_port *pp = MV_ETH_PRIV(dev);
	int tx_done = 0, tx_todo = 0;
	unsigned int txq_mask;

	STAT_INFO(pp->stats.tx_done_timer++);

	cpuCtrl = pp->cpu_config[smp_processor_id()];

	clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->flags & MV_ETH_F_DBG_TX)
			printk(KERN_ERR "%s: port #%d is stopped, STARTED_BIT = 0, exit timer.\n", __func__, pp->port);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		return;
	}

	if (MV_PON_PORT(pp->port))
		tx_done = mv_eth_tx_done_pon(pp, &tx_todo);
	else {
		/* check all possible queues, as there is no indication from interrupt */
		txq_mask = ((1 << CONFIG_MV_ETH_TXQ) - 1) & cpuCtrl->cpuTxqOwner;
		tx_done = mv_eth_tx_done_gbe(pp, txq_mask, &tx_todo);
	}

	if (tx_todo > 0)
		mv_eth_add_tx_done_timer(cpuCtrl);
}

/***********************************************************
 * mv_eth_cleanup_timer_callback --			   *
 *   N msec periodic callback for error cleanup            *
 ***********************************************************/
static void mv_eth_cleanup_timer_callback(unsigned long data)
{
	struct cpu_ctrl *cpuCtrl;
	struct net_device *dev = (struct net_device *)data;
	struct eth_port *pp = MV_ETH_PRIV(dev);

	STAT_INFO(pp->stats.cleanup_timer++);

	cpuCtrl = pp->cpu_config[smp_processor_id()];
	clear_bit(MV_ETH_F_CLEANUP_TIMER_BIT, &(cpuCtrl->flags));

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
		return;

	/* FIXME: check bm_pool->missed and pp->rxq_ctrl[rxq].missed counters and allocate */
	/* re-add timer if necessary (check bm_pool->missed and pp->rxq_ctrl[rxq].missed   */
}

void mv_eth_mac_show(int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return;
	}

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en) {
		mvOsPrintf("PnC MAC Rules - port #%d:\n", port);
		pnc_mac_show();
	} else
		mvOsPrintf("%s: PNC control is disabled\n", __func__);
#else /* Legacy parser */
	mvEthPortUcastShow(port);
	mvEthPortMcastShow(port);
#endif /* CONFIG_MV_ETH_PNC */
}

void mv_eth_vlan_prio_show(int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return;
	}

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en) {
		mvOsPrintf("PnC VLAN Priority Rules - port #%d:\n", port);
		pnc_vlan_prio_show(port);
	} else
		mvOsPrintf("%s: PNC control is disabled\n", __func__);
#else /* Legacy parser */
	{
		int prio, rxq;

		mvOsPrintf("Legacy VLAN Priority Rules - port #%d:\n", port);
		for (prio = 0; prio <= 0x7 ; prio++) {

			rxq = mvNetaVprioToRxqGet(port, prio);
			if (rxq > 0)
				printk(KERN_INFO "prio=%d: rxq=%d\n", prio, rxq);
		}
	}
#endif /* CONFIG_MV_ETH_PNC */
}

void mv_eth_tos_map_show(int port)
{
	int tos, txq, cpu;
	struct cpu_ctrl *cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return;
	}

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en) {
		mvOsPrintf("PnC    TOS (DSCP) => RXQ Mapping Rules - port #%d:\n", port);
		pnc_ipv4_dscp_show(port);
	} else
		mvOsPrintf("%s: PNC control is disabled\n", __func__);
#else
	mvOsPrintf("Legacy TOS (DSCP) => RXQ Mapping Rules - port #%d:\n", port);
	for (tos = 0; tos < 0xFF; tos += 0x4) {
		int rxq;

		rxq = mvNetaTosToRxqGet(port, tos);
		if (rxq > 0)
			printk(KERN_ERR "      0x%02x (0x%02x) => %d\n",
					tos, tos >> 2, rxq);
	}
#endif /* CONFIG_MV_ETH_PNC */
	for_each_possible_cpu(cpu) {
		printk(KERN_ERR "\n");
		printk(KERN_ERR " TOS => TXQ map for port #%d cpu #%d\n", port, cpu);
		cpuCtrl = pp->cpu_config[cpu];
		for (tos = 0; tos < sizeof(cpuCtrl->txq_tos_map); tos++) {
			txq = cpuCtrl->txq_tos_map[tos];
			if (txq != MV_ETH_TXQ_INVALID)
				printk(KERN_ERR "0x%02x => %d\n", tos, txq);
		}
	}
}

int mv_eth_rxq_tos_map_set(int port, int rxq, unsigned char tos)
{
	int status = -1;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return 1;
	}

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en)
		status = pnc_ip4_dscp(port, tos, 0xFF, rxq);
	else
		mvOsPrintf("%s: PNC control is disabled\n", __func__);
#else /* Legacy parser */
	status = mvNetaTosToRxqSet(port, tos, rxq);
#endif /* CONFIG_MV_ETH_PNC */

	if (status == 0)
		printk(KERN_ERR "Succeeded\n");
	else if (status == -1)
		printk(KERN_ERR "Not supported\n");
	else
		printk(KERN_ERR "Failed\n");

	return status;
}

int mv_eth_rxq_vlan_prio_set(int port, int rxq, unsigned char prio)
{
	int status = -1;

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en)
		status = pnc_vlan_prio_set(port, prio, rxq);
	else
		mvOsPrintf("%s: PNC control is disabled\n", __func__);
#else /* Legacy parser */
	status = mvNetaVprioToRxqSet(port, prio, rxq);
#endif /* CONFIG_MV_ETH_PNC */

	if (status == 0)
		printk(KERN_ERR "Succeeded\n");
	else if (status == -1)
		printk(KERN_ERR "Not supported\n");
	else
		printk(KERN_ERR "Failed\n");

	return status;
}

/* Set TXQ for special TOS value. txq=-1 - use default TXQ for this port */
int mv_eth_txq_tos_map_set(int port, int txq, int cpu, unsigned int tos)
{
	MV_U8 old_txq;
	struct cpu_ctrl	*cpuCtrl;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (mvNetaPortCheck(port))
		return -EINVAL;

	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	if ((cpu >= CONFIG_NR_CPUS) || (cpu < 0)) {
		printk(KERN_ERR "cpu #%d is out of range: from 0 to %d\n",
			cpu, CONFIG_NR_CPUS - 1);
		return -EINVAL;
	}

	if (!(MV_BIT_CHECK(pp->cpuMask, cpu))) {
		printk(KERN_ERR "%s:Error, cpu #%d is masked \n", __func__, cpu);
		return -EINVAL;
	}
	if ((tos > 0xFF) || (tos < 0)) {
		printk(KERN_ERR "TOS 0x%x is out of range: from 0 to 0xFF\n", tos);
		return -EINVAL;
	}
	cpuCtrl = pp->cpu_config[cpu];
	old_txq = cpuCtrl->txq_tos_map[tos];

	/* The same txq - do nothing */
	if (old_txq == (MV_U8) txq)
		return MV_OK;

	if (txq == -1) {
		/* delete tos to txq mapping - free TXQ */
		if (mv_eth_ctrl_txq_cpu_own(port, pp->txp, old_txq, 0, cpu))
			return -EINVAL;

		cpuCtrl->txq_tos_map[tos] = MV_ETH_TXQ_INVALID;
		printk(KERN_ERR "Successfully deleted\n");
		return MV_OK;
	}

	if (mvNetaMaxCheck(txq, CONFIG_MV_ETH_TXQ))
		return -EINVAL;

	/* Check that new txq can be allocated for cpu */
	if (!(MV_BIT_CHECK(cpuCtrl->cpuTxqMask, txq))) {
		printk(KERN_ERR "%s: Error, Txq #%d masked for cpu #%d\n", __func__, txq, cpu);
		return -EINVAL;
	}

	if (mv_eth_ctrl_txq_cpu_own(port, pp->txp, txq, 1, cpu))
		return -EINVAL;

	cpuCtrl->txq_tos_map[tos] = (MV_U8) txq;
	printk(KERN_ERR "Successfully added\n");
	return MV_OK;
}

static int mv_eth_priv_init(struct eth_port *pp, int port)
{
	int cpu, i;
	struct cpu_ctrl	*cpuCtrl;
	u8	*ext_buf;
#ifdef MY_ABC_HERE
	MV_U16		phy_id_0, phy_id_1;
#endif

	memset(pp, 0, sizeof(struct eth_port));

	/* Default field per cpu initialization */
	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		pp->cpu_config[i] = kmalloc(sizeof(struct cpu_ctrl), GFP_KERNEL);
		memset(pp->cpu_config[i], 0, sizeof(struct cpu_ctrl));
	}

	pp->port = port;
	pp->txp_num = 1;
	pp->txp = 0;
	pp->wol_mode = 0;
	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		cpuCtrl->txq = CONFIG_MV_ETH_TXQ_DEF;
		cpuCtrl->cpuTxqOwner = (1 << CONFIG_MV_ETH_TXQ_DEF);
		cpuCtrl->cpuTxqMask = 0xFF;
		mvNetaTxqCpuMaskSet(port, 0xFF , cpu);
	}

	pp->flags = 0;

#ifdef CONFIG_MV_ETH_BM_CPU
	pp->pool_long_num = mv_eth_bm_config_long_buf_num_get(port);
	if (pp->pool_long_num > MV_BM_POOL_CAP_MAX)
		pp->pool_long_num = MV_BM_POOL_CAP_MAX;

	pp->pool_short_num = mv_eth_bm_config_short_buf_num_get(port);
	if (pp->pool_short_num > MV_BM_POOL_CAP_MAX)
		pp->pool_short_num = MV_BM_POOL_CAP_MAX;
#else
	pp->pool_long_num = CONFIG_MV_ETH_RXQ * CONFIG_MV_ETH_RXQ_DESC * 2;
#endif /* CONFIG_MV_ETH_BM_CPU */
	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		for (i = 0; i < 256; i++) {
			cpuCtrl->txq_tos_map[i] = MV_ETH_TXQ_INVALID;
#ifdef CONFIG_MV_ETH_TX_SPECIAL
		pp->tx_special_check = NULL;
#endif /* CONFIG_MV_ETH_TX_SPECIAL */
		}
	}

	mv_eth_port_config_parse(pp);

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(port)) {
		set_bit(MV_ETH_F_MH_BIT, &(pp->flags));
		pp->txp_num = MV_ETH_MAX_TCONT();
		pp->txp = CONFIG_MV_PON_TXP_DEF;
		for_each_possible_cpu(i)
			pp->cpu_config[i]->txq = CONFIG_MV_PON_TXQ_DEF;
	}
#endif /* CONFIG_MV_PON */

#if defined(CONFIG_MV_ETH_RX_CSUM_OFFLOAD_DEF)
	pp->rx_csum_offload = 1;
#endif /* CONFIG_MV_ETH_RX_CSUM_OFFLOAD_DEF */

#ifdef CONFIG_MV_INCLUDE_SWITCH
	if (mvBoardSwitchConnectedPortGet(port) != -1) {
		set_bit(MV_ETH_F_SWITCH_BIT, &(pp->flags));
		set_bit(MV_ETH_F_EXT_SWITCH_BIT, &(pp->flags));
	}
#endif /* CONFIG_MV_INCLUDE_SWITCH */
	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		memset(&cpuCtrl->tx_done_timer, 0, sizeof(struct timer_list));
		cpuCtrl->tx_done_timer.function = mv_eth_tx_done_timer_callback;
		init_timer(&cpuCtrl->tx_done_timer);
		clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));
		memset(&cpuCtrl->cleanup_timer, 0, sizeof(struct timer_list));
		cpuCtrl->cleanup_timer.function = mv_eth_cleanup_timer_callback;
		init_timer(&cpuCtrl->cleanup_timer);
		clear_bit(MV_ETH_F_CLEANUP_TIMER_BIT, &(cpuCtrl->flags));
	}

	pp->weight = CONFIG_MV_ETH_RX_POLL_WEIGHT;

	/* Init pool of external buffers for TSO, fragmentation, etc */
	spin_lock_init(&pp->extLock);
	pp->extBufSize = CONFIG_MV_ETH_EXTRA_BUF_SIZE;
	pp->extArrStack = mvStackCreate(CONFIG_MV_ETH_EXTRA_BUF_NUM);
	if (pp->extArrStack == NULL) {
		printk(KERN_ERR "Error: failed create  extArrStack for port #%d\n", port);
		return -ENOMEM;
	}
	for (i = 0; i < CONFIG_MV_ETH_EXTRA_BUF_NUM; i++) {
		ext_buf = mvOsMalloc(CONFIG_MV_ETH_EXTRA_BUF_SIZE);
		if (ext_buf == NULL) {
			printk(KERN_WARNING "%s Warning: %d of %d extra buffers allocated\n",
				__func__, i, CONFIG_MV_ETH_EXTRA_BUF_NUM);
			break;
		}
		mvStackPush(pp->extArrStack, (MV_U32)ext_buf);
	}

#ifdef CONFIG_MV_ETH_STAT_DIST
	pp->dist_stats.rx_dist = mvOsMalloc(sizeof(u32) * (CONFIG_MV_ETH_RXQ * CONFIG_MV_ETH_RXQ_DESC + 1));
	if (pp->dist_stats.rx_dist != NULL) {
		pp->dist_stats.rx_dist_size = CONFIG_MV_ETH_RXQ * CONFIG_MV_ETH_RXQ_DESC + 1;
		memset(pp->dist_stats.rx_dist, 0, sizeof(u32) * pp->dist_stats.rx_dist_size);
	} else
		printk(KERN_ERR "ethPort #%d: Can't allocate %d bytes for rx_dist\n",
		       pp->port, sizeof(u32) * (CONFIG_MV_ETH_RXQ * CONFIG_MV_ETH_RXQ_DESC + 1));

	pp->dist_stats.tx_done_dist =
	    mvOsMalloc(sizeof(u32) * (pp->txp_num * CONFIG_MV_ETH_TXQ * CONFIG_MV_ETH_TXQ_DESC + 1));
	if (pp->dist_stats.tx_done_dist != NULL) {
		pp->dist_stats.tx_done_dist_size = pp->txp_num * CONFIG_MV_ETH_TXQ * CONFIG_MV_ETH_TXQ_DESC + 1;
		memset(pp->dist_stats.tx_done_dist, 0, sizeof(u32) * pp->dist_stats.tx_done_dist_size);
	} else
		printk(KERN_ERR "ethPort #%d: Can't allocate %d bytes for tx_done_dist\n",
		       pp->port, sizeof(u32) * (pp->txp_num * CONFIG_MV_ETH_TXQ * CONFIG_MV_ETH_TXQ_DESC + 1));
#endif /* CONFIG_MV_ETH_STAT_DIST */
#ifdef MY_ABC_HERE
	pp->phy_id = mvBoardPhyAddrGet(port);
	pp->wol = 0;
	if (mv_eth_tool_read_phy_reg(pp->phy_id, 0, MII_PHYSID1, &phy_id_0) ||
		mv_eth_tool_read_phy_reg(pp->phy_id, 0, MII_PHYSID2, &phy_id_1)) {
		printk("Synology Warning: Get PHY ID Error!\n");
		pp->phy_chip = 0;
	} else {
		/* For 151X series phy */
		if (MV_PHY_ID_151X == (phy_id_0 & 0xffff) << 16 | (phy_id_1 & 0xfff0)) {
			pp->phy_chip = (phy_id_0 & 0xffff) << 16 | (phy_id_1 & 0xfff0);
		} else {
			pp->phy_chip = (phy_id_0 & 0xffff) << 16 | (phy_id_1 & 0xffff);
		}
	}
#endif

	return MV_OK;
}

/***********************************************************************************
 ***  noqueue net device
 ***********************************************************************************/
extern struct Qdisc noop_qdisc;
void mv_eth_set_noqueue(struct net_device *dev, int enable)
{
	struct netdev_queue *txq = netdev_get_tx_queue(dev, 0);

	if (dev->flags & IFF_UP) {
		printk(KERN_ERR "%s: device or resource busy, take it down\n", dev->name);
		return;
	}
	dev->tx_queue_len = enable ? 0 : CONFIG_MV_ETH_TXQ_DESC;

	if (txq)
		txq->qdisc_sleeping = &noop_qdisc;
	else
		printk(KERN_ERR "%s: txq #0 is NULL\n", dev->name);

	printk(KERN_ERR "%s: device tx queue len is %d\n", dev->name, (int)dev->tx_queue_len);

}

/***********************************************************************************
 ***  print RX bm_pool status
 ***********************************************************************************/
void mv_eth_pool_status_print(int pool)
{
	struct bm_pool *bm_pool = &mv_eth_pool[pool];

	printk(KERN_ERR "\nRX Pool #%d: pkt_size=%d, BM-HW support - %s\n",
	       pool, bm_pool->pkt_size, mv_eth_pool_bm(bm_pool) ? "Yes" : "No");

	printk(KERN_ERR "bm_pool=%p, stack=%p, capacity=%d, buf_num=%d, port_map=0x%x missed=%d\n",
	       bm_pool->bm_pool, bm_pool->stack, bm_pool->capacity, bm_pool->buf_num,
		   bm_pool->port_map, bm_pool->missed);

#ifdef CONFIG_MV_ETH_STAT_ERR
	printk(KERN_ERR "Errors: skb_alloc_oom=%u, stack_empty=%u, stack_full=%u\n",
	       bm_pool->stats.skb_alloc_oom, bm_pool->stats.stack_empty, bm_pool->stats.stack_full);
#endif /* #ifdef CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_DBG
	printk(KERN_ERR "skb_alloc_ok=%u, bm_put=%u, stack_put=%u, stack_get=%u\n",
	       bm_pool->stats.skb_alloc_ok, bm_pool->stats.bm_put, bm_pool->stats.stack_put, bm_pool->stats.stack_get);

	printk(KERN_ERR "skb_recycled_ok=%u, skb_recycled_err=%u\n",
	       bm_pool->stats.skb_recycled_ok, bm_pool->stats.skb_recycled_err);
#endif /* CONFIG_MV_ETH_STAT_DBG */

	if (bm_pool->stack)
		mvStackStatus(bm_pool->stack, 0);

	memset(&bm_pool->stats, 0, sizeof(bm_pool->stats));
}

/***********************************************************************************
 ***  print ext pool status
 ***********************************************************************************/
void mv_eth_ext_pool_print(struct eth_port *pp)
{
	printk(KERN_ERR "\nExt Pool Stack: bufSize = %u bytes\n", pp->extBufSize);
	mvStackStatus(pp->extArrStack, 0);
}

/***********************************************************************************
 ***  print net device status
 ***********************************************************************************/
void mv_eth_netdev_print(struct net_device *dev)
{
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);

	printk(KERN_ERR "%s net_device status: dev=%p, pp=%p\n", dev->name, dev, MV_ETH_PRIV(dev));
	printk(KERN_ERR "ifIdx=%d, features=0x%x, flags=0x%x, mtu=%u, size=%d, MAC=" MV_MACQUAD_FMT "\n",
	       dev->ifindex, (unsigned int)(dev->features), (unsigned int)(dev->flags),
	       dev->mtu, RX_PKT_SIZE(dev->mtu), MV_MACQUAD(dev->dev_addr));

	if (dev_priv)
		printk(KERN_ERR "group=%d, tx_vlan_mh=0x%04x, switch_port_map=0x%x, switch_port_link_map=0x%x\n",
		       dev_priv->group, dev_priv->tx_vlan_mh, dev_priv->port_map, dev_priv->link_map);
	printk(KERN_ERR "\n");
}

void mv_eth_status_print(void)
{
	printk(KERN_ERR "totals: ports=%d, devs=%d\n", mv_eth_ports_num, mv_net_devs_num);

#ifdef CONFIG_NET_SKB_RECYCLE
	printk(KERN_ERR "SKB recycle = %s\n", mv_ctrl_recycle ? "Enabled" : "Disabled");
#endif /* CONFIG_NET_SKB_RECYCLE */

#ifdef CONFIG_MV_ETH_PNC
	printk(KERN_ERR "PnC control = %s\n", mv_eth_pnc_ctrl_en ? "Enabled" : "Disabled");
#endif /* CONFIG_MV_ETH_PNC */
}

/***********************************************************************************
 ***  print Ethernet port status
 ***********************************************************************************/
void mv_eth_port_status_print(unsigned int port)
{
	int txp, q;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct tx_queue *txq_ctrl;
	struct cpu_ctrl	*cpuCtrl;

	if (!pp)
		return;

	printk(KERN_ERR "\n");
	printk(KERN_ERR "port=%d, flags=0x%lx, rx_weight=%d\n", port, pp->flags, pp->weight);
	if ((!(pp->flags & MV_ETH_F_SWITCH)) && (pp->flags & MV_ETH_F_CONNECT_LINUX))
		printk(KERN_ERR "%s: ", pp->dev->name);
	else
		printk(KERN_ERR "port %d: ", port);

	mv_eth_link_status_print(port);

#ifdef CONFIG_MV_ETH_NFP
	printk(KERN_ERR "NFP = ");
	if (pp->flags & MV_ETH_F_NFP_EN)
		printk(KERN_CONT "Enabled\n");
	else
		printk(KERN_CONT "Disabled\n");
#endif /* CONFIG_MV_ETH_NFP */
	if (pp->wol_mode == 1)
		printk(KERN_CONT "pm - wol\n");
	else
		printk(KERN_CONT "pm - suspend\n");

	printk(KERN_ERR "rxq_coal(pkts)[ q]   = ");
	for (q = 0; q < CONFIG_MV_ETH_RXQ; q++)
		printk(KERN_CONT "%3d ", mvNetaRxqPktsCoalGet(port, q));

	printk(KERN_CONT "\n");
	printk(KERN_ERR "rxq_coal(usec)[ q]   = ");
	for (q = 0; q < CONFIG_MV_ETH_RXQ; q++)
		printk(KERN_CONT "%3d ", mvNetaRxqTimeCoalGet(port, q));

	printk(KERN_CONT "\n");
	printk(KERN_ERR "rxq_desc(num)[ q]    = ");
	for (q = 0; q < CONFIG_MV_ETH_RXQ; q++)
		printk(KERN_CONT "%3d ", pp->rxq_ctrl[q].rxq_size);

	printk(KERN_CONT "\n");
	for (txp = 0; txp < pp->txp_num; txp++) {
		printk(KERN_ERR "txq_coal(pkts)[%2d.q] = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++)
			printk(KERN_CONT "%3d ", mvNetaTxDonePktsCoalGet(port, txp, q));
		printk(KERN_CONT "\n");

		printk(KERN_ERR "txq_mod(F,C,H)[%2d.q] = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
			int val, mode;

			mode = mv_eth_ctrl_txq_mode_get(port, txp, q, &val);
			if (mode == MV_ETH_TXQ_CPU)
				printk(KERN_CONT " C%-d ", val);
			else if (mode == MV_ETH_TXQ_HWF)
				printk(KERN_CONT " H%-d ", val);
			else
				printk(KERN_CONT "  F ");
		}
		printk(KERN_CONT "\n");

		printk(KERN_ERR "txq_desc(num) [%2d.q] = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
			struct tx_queue *txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + q];
			printk(KERN_CONT "%3d ", txq_ctrl->txq_size);
		}
		printk(KERN_CONT "\n");
	}
	printk(KERN_ERR "\n");

#ifdef CONFIG_MV_ETH_TXDONE_ISR
	printk(KERN_ERR "Do tx_done in NAPI context triggered by ISR\n");
	for (txp = 0; txp < pp->txp_num; txp++) {
		printk(KERN_ERR "txcoal(pkts)[%2d.q] = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++)
			printk(KERN_CONT "%3d ", mvNetaTxDonePktsCoalGet(port, txp, q));
		printk(KERN_CONT "\n");
	}
	printk(KERN_ERR "\n");
#else
	printk(KERN_ERR "Do tx_done in TX or Timer context: tx_done_threshold=%d\n", mv_ctrl_txdone);
#endif /* CONFIG_MV_ETH_TXDONE_ISR */

	printk(KERN_ERR "txp=%d, zero_pad=%s, mh_en=%s (0x%04x), tx_cmd=0x%08x\n",
	       pp->txp, (pp->flags & MV_ETH_F_NO_PAD) ? "Disabled" : "Enabled",
	       (pp->flags & MV_ETH_F_MH) ? "Enabled" : "Disabled", pp->tx_mh, pp->hw_cmd);

	printk(KERN_CONT "\n");
	printk(KERN_CONT "CPU:  txq  causeRxTx   napi txqMask txqOwner flags  timer\n");
	{
		int cpu;
		for_each_possible_cpu(cpu) {
			cpuCtrl = pp->cpu_config[cpu];
			if (MV_BIT_CHECK(pp->cpuMask, cpu))
				printk(KERN_ERR "  %d:   %d   0x%08x   %d    0x%02x    0x%02x    0x%02x    %d\n",
					cpu, cpuCtrl->txq, cpuCtrl->causeRxTx, test_bit(NAPI_STATE_SCHED, &cpuCtrl->napi->state),
					cpuCtrl->cpuTxqMask, cpuCtrl->cpuTxqOwner,
					(unsigned)cpuCtrl->flags, timer_pending(&cpuCtrl->tx_done_timer));
		}
	}

	printk(KERN_CONT "\n");
	printk(KERN_CONT "TXQ: SharedFlag  nfpCounter   cpu_owner\n");

	for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
		int cpu;

		txq_ctrl = &pp->txq_ctrl[pp->txp * CONFIG_MV_ETH_TXQ + q];
		if (txq_ctrl != NULL)
			printk(KERN_CONT " %d:     %2lu        %d",
				q, (txq_ctrl->flags & MV_ETH_F_TX_SHARED), txq_ctrl->nfpCounter);

			printk(KERN_CONT "        [");
			for_each_possible_cpu(cpu)
				printk(KERN_CONT "%2d ", txq_ctrl->cpu_owner[cpu]);

			printk(KERN_CONT "]\n");
	}
	printk(KERN_CONT "\n");

	if (pp->dev)
		mv_eth_netdev_print(pp->dev);

#ifdef CONFIG_MV_ETH_SWITCH
	if (pp->flags & MV_ETH_F_SWITCH)
		mv_eth_switch_status_print(port);
#endif /* CONFIG_MV_ETH_SWITCH */
}

/***********************************************************************************
 ***  print port statistics
 ***********************************************************************************/

void mv_eth_port_stats_print(unsigned int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct port_stats *stat = NULL;
	struct tx_queue *txq_ctrl;
	int txp, queue;
	u32 total_rx_ok, total_rx_fill_ok;
#ifdef CONFIG_MV_ETH_STAT_INF
	int i;
#endif

	if (pp == NULL) {
		printk(KERN_ERR "eth_stats_print: wrong port number %d\n", port);
		return;
	}
	stat = &(pp->stats);

#ifdef CONFIG_MV_ETH_STAT_ERR
	printk(KERN_ERR "\n====================================================\n");
	printk(KERN_ERR "ethPort_%d: Errors", port);
	printk(KERN_CONT "\n-------------------------------\n");
	printk(KERN_ERR "rx_error......................%10u\n", stat->rx_error);
	printk(KERN_ERR "tx_timeout....................%10u\n", stat->tx_timeout);
	printk(KERN_ERR "tx_netif_stop.................%10u\n", stat->netif_stop);
	printk(KERN_ERR "netif_wake....................%10u\n", stat->netif_wake);
	printk(KERN_ERR "ext_stack_empty...............%10u\n", stat->ext_stack_empty);
	printk(KERN_ERR "ext_stack_full ...............%10u\n", stat->ext_stack_full);
	printk(KERN_ERR "state_err.....................%10u\n", stat->state_err);
#endif /* CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_INF
	printk(KERN_ERR "\n====================================================\n");
	printk(KERN_ERR "ethPort_%d: interrupt statistics", port);
	printk(KERN_CONT "\n-------------------------------\n");
	printk(KERN_ERR "irq...........................%10u\n", stat->irq);
	printk(KERN_ERR "irq_err.......................%10u\n", stat->irq_err);

	printk(KERN_ERR "\n====================================================\n");
	printk(KERN_ERR "ethPort_%d: Events", port);
	printk(KERN_CONT "\n-------------------------------\n");
	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		printk(KERN_ERR "poll[%d]......................%10u\n", i, stat->poll[i]);
		printk(KERN_ERR "poll_exit[%d].................%10u\n", i, stat->poll_exit[i]);
	}
	printk(KERN_ERR "tx_fragmentation..............%10u\n", stat->tx_fragment);
	printk(KERN_ERR "tx_done_event.................%10u\n", stat->tx_done);
	printk(KERN_ERR "tx_done_timer_event...........%10u\n", stat->tx_done_timer);
	printk(KERN_ERR "cleanup_timer_event...........%10u\n", stat->cleanup_timer);
	printk(KERN_ERR "link..........................%10u\n", stat->link);
	printk(KERN_ERR "netdev_stop...................%10u\n", stat->netdev_stop);
#ifdef CONFIG_MV_ETH_RX_SPECIAL
	printk(KERN_ERR "rx_special....................%10u\n", stat->rx_special);
#endif /* CONFIG_MV_ETH_RX_SPECIAL */
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	printk(KERN_ERR "tx_special....................%10u\n", stat->tx_special);
#endif /* CONFIG_MV_ETH_TX_SPECIAL */
#endif /* CONFIG_MV_ETH_STAT_INF */

	printk(KERN_ERR "\n");
	total_rx_ok = total_rx_fill_ok = 0;
	printk(KERN_ERR "RXQ:       rx_ok      rx_fill_ok     missed\n\n");
	for (queue = 0; queue < CONFIG_MV_ETH_RXQ; queue++) {
		u32 rxq_ok = 0, rxq_fill = 0;

#ifdef CONFIG_MV_ETH_STAT_DBG
		rxq_ok = stat->rxq[queue];
		rxq_fill = stat->rxq_fill[queue];
#endif /* CONFIG_MV_ETH_STAT_DBG */

		printk(KERN_ERR "%3d:  %10u    %10u          %d\n",
			queue, rxq_ok, rxq_fill,
			pp->rxq_ctrl[queue].missed);
		total_rx_ok += rxq_ok;
		total_rx_fill_ok += rxq_fill;
	}
	printk(KERN_ERR "SUM:  %10u    %10u\n", total_rx_ok, total_rx_fill_ok);

#ifdef CONFIG_MV_ETH_STAT_DBG
	{
		printk(KERN_ERR "\n====================================================\n");
		printk(KERN_ERR "ethPort_%d: Debug statistics", port);
		printk(KERN_CONT "\n-------------------------------\n");

		printk(KERN_ERR "\n");

		printk(KERN_ERR "rx_nfp....................%10u\n", stat->rx_nfp);
		printk(KERN_ERR "rx_nfp_drop...............%10u\n", stat->rx_nfp_drop);

		printk(KERN_ERR "rx_gro....................%10u\n", stat->rx_gro);
		printk(KERN_ERR "rx_gro_bytes .............%10u\n", stat->rx_gro_bytes);

		printk(KERN_ERR "tx_tso....................%10u\n", stat->tx_tso);
		printk(KERN_ERR "tx_tso_bytes .............%10u\n", stat->tx_tso_bytes);

		printk(KERN_ERR "rx_netif..................%10u\n", stat->rx_netif);
		printk(KERN_ERR "rx_drop_sw................%10u\n", stat->rx_drop_sw);
		printk(KERN_ERR "rx_csum_hw................%10u\n", stat->rx_csum_hw);
		printk(KERN_ERR "rx_csum_sw................%10u\n", stat->rx_csum_sw);

		printk(KERN_ERR "tx_skb_free...............%10u\n", stat->tx_skb_free);
		printk(KERN_ERR "tx_sg.....................%10u\n", stat->tx_sg);
		printk(KERN_ERR "tx_csum_hw................%10u\n", stat->tx_csum_hw);
		printk(KERN_ERR "tx_csum_sw................%10u\n", stat->tx_csum_sw);

		printk(KERN_ERR "ext_stack_get.............%10u\n", stat->ext_stack_get);
		printk(KERN_ERR "ext_stack_put ............%10u\n", stat->ext_stack_put);

		printk(KERN_ERR "\n");
	}
#endif /* CONFIG_MV_ETH_STAT_DBG */

	printk(KERN_ERR "\n");
	printk(KERN_ERR "TXP-TXQ:  count        send          done      no_resource\n\n");

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (queue = 0; queue < CONFIG_MV_ETH_TXQ; queue++) {
			u32 txq_tx = 0, txq_txdone = 0, txq_err = 0;

			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + queue];
#ifdef CONFIG_MV_ETH_STAT_DBG
			txq_tx = txq_ctrl->stats.txq_tx;
			txq_txdone =  txq_ctrl->stats.txq_txdone;
#endif /* CONFIG_MV_ETH_STAT_DBG */
#ifdef CONFIG_MV_ETH_STAT_ERR
			txq_err = txq_ctrl->stats.txq_err;
#endif /* CONFIG_MV_ETH_STAT_ERR */

			printk(KERN_ERR "%d-%d:      %3d    %10u    %10u    %10u\n",
			       txp, queue, txq_ctrl->txq_count, txq_tx,
			       txq_txdone, txq_err);

			memset(&txq_ctrl->stats, 0, sizeof(txq_ctrl->stats));
		}
	}
	printk(KERN_ERR "\n\n");

	memset(stat, 0, sizeof(struct port_stats));

	/* RX pool statistics */
#ifdef CONFIG_MV_ETH_BM_CPU
	if (pp->pool_short)
		mv_eth_pool_status_print(pp->pool_short->pool);
#endif /* CONFIG_MV_ETH_BM_CPU */

	if (pp->pool_long)
		mv_eth_pool_status_print(pp->pool_long->pool);

		mv_eth_ext_pool_print(pp);

#ifdef CONFIG_MV_ETH_STAT_DIST
	{
		int i;
		struct dist_stats *dist_stats = &(pp->dist_stats);

		if (dist_stats->rx_dist) {
			printk(KERN_ERR "\n      Linux Path RX distribution\n");
			for (i = 0; i < dist_stats->rx_dist_size; i++) {
				if (dist_stats->rx_dist[i] != 0) {
					printk(KERN_ERR "%3d RxPkts - %u times\n", i, dist_stats->rx_dist[i]);
					dist_stats->rx_dist[i] = 0;
				}
			}
		}

		if (dist_stats->tx_done_dist) {
			printk(KERN_ERR "\n      tx-done distribution\n");
			for (i = 0; i < dist_stats->tx_done_dist_size; i++) {
				if (dist_stats->tx_done_dist[i] != 0) {
					printk(KERN_ERR "%3d TxDoneDesc - %u times\n", i, dist_stats->tx_done_dist[i]);
					dist_stats->tx_done_dist[i] = 0;
				}
			}
		}
#ifdef CONFIG_MV_ETH_TSO
		if (dist_stats->tx_tso_dist) {
			printk(KERN_ERR "\n      TSO stats\n");
			for (i = 0; i < dist_stats->tx_tso_dist_size; i++) {
				if (dist_stats->tx_tso_dist[i] != 0) {
					printk(KERN_ERR "%3d KBytes - %u times\n", i, dist_stats->tx_tso_dist[i]);
					dist_stats->tx_tso_dist[i] = 0;
				}
			}
		}
#endif /* CONFIG_MV_ETH_TSO */
	}
#endif /* CONFIG_MV_ETH_STAT_DIST */
}

static int mv_eth_port_cleanup(int port)
{
	int txp, txq, rxq, i;
	struct eth_port *pp;
	struct tx_queue *txq_ctrl;
	struct rx_queue *rxq_ctrl;

	pp = mv_eth_port_by_id(port);

	if (pp == NULL)
		return MV_ERROR;

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "%s: port %d is started, cannot cleanup\n", __func__, port);
		return MV_ERROR;
	}

	/* Reset Tx ports */
	for (txp = 0; txp < pp->txp_num; txp++) {
		if (mv_eth_txp_reset(port, txp))
			printk(KERN_ERR "Warning: Port %d Tx port %d reset failed\n", port, txp);
	}

	/* Delete Tx queues */
	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
			mv_eth_txq_delete(pp, txq_ctrl);
		}
	}

	mvOsFree(pp->txq_ctrl);
	pp->txq_ctrl = NULL;

#ifdef CONFIG_MV_ETH_STAT_DIST
	/* Free Tx Done distribution statistics */
	mvOsFree(pp->dist_stats.tx_done_dist);
#endif

	/* Reset RX ports */
	if (mv_eth_rx_reset(port))
		printk(KERN_ERR "Warning: Rx port %d reset failed\n", port);

	/* Delete Rx queues */
	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		rxq_ctrl = &pp->rxq_ctrl[rxq];
		mvNetaRxqDelete(pp->port, rxq);
		rxq_ctrl->q = NULL;
	}

	mvOsFree(pp->rxq_ctrl);
	pp->rxq_ctrl = NULL;

#ifdef CONFIG_MV_ETH_STAT_DIST
	/* Free Rx distribution statistics */
	mvOsFree(pp->dist_stats.rx_dist);
#endif

	/* Free buffer pools */
	if (pp->pool_long) {
		mv_eth_pool_free(pp->pool_long->pool, pp->pool_long_num);
		pp->pool_long->port_map &= ~(1 << pp->port);
		pp->pool_long = NULL;
	}
#ifdef CONFIG_MV_ETH_BM_CPU
	if (pp->pool_short) {
		mv_eth_pool_free(pp->pool_short->pool, pp->pool_short_num);
		pp->pool_short->port_map &= ~(1 << pp->port);
		pp->pool_short = NULL;
	}
#endif /* CONFIG_MV_ETH_BM_CPU */

	/* Clear Marvell Header related modes - will be set again if needed on re-init */
	mvNetaMhSet(port, MV_NETA_MH_NONE);

	/* Clear any forced link, speed and duplex */
	mv_force_port_link_speed_fc(port, MV_ETH_SPEED_AN, 0);

	mvNetaPortDestroy(port);

	if (pp->flags & MV_ETH_F_CONNECT_LINUX)
		for (i = 0; i < CONFIG_MV_ETH_NAPI_GROUPS; i++)
			netif_napi_del(pp->napiGroup[i]);

	return MV_OK;
}

int mv_eth_all_ports_cleanup(void)
{
	int port, pool, status = 0;

	for (port = 0; port < mv_eth_ports_num; port++) {
		status = mv_eth_port_cleanup(port);
		if (status != 0) {
			printk(KERN_ERR "Error: mv_eth_port_cleanup failed on port %d, stopping all ports cleanup\n", port);
			return status;
		}
	}

	for (pool = 0; pool < MV_ETH_BM_POOLS; pool++)
		mv_eth_pool_destroy(pool);

	for (port = 0; port < mv_eth_ports_num; port++) {
		if (mv_eth_ports[port])
			mvOsFree(mv_eth_ports[port]);
	}

	memset(mv_eth_ports, 0, (mv_eth_ports_num * sizeof(struct eth_port *)));
	/* Note: not freeing mv_eth_ports - we will reuse them */
	 
	return MV_OK;
}

#ifdef CONFIG_MV_ETH_PNC_WOL

#define DEF_WOL_SIZE	42
MV_U8	wol_data[DEF_WOL_SIZE] = { 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x08, 0x00,
				   0x45, 0x00, 0x00, 0x4E, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0x11, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0xFA,
				   0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x00, 0x89,
				   0x00, 0x3A };

MV_U8	wol_mask[DEF_WOL_SIZE] = { 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				   0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
				   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
				   0xff, 0xff };

int mv_eth_wol_pkts_check(int port)
{
	struct eth_port	    *pp = mv_eth_port_by_id(port);
	struct neta_rx_desc *rx_desc;
	struct eth_pbuf     *pkt;
	struct bm_pool      *pool;
	int                 rxq, rx_done, i, wakeup, ruleId;
	MV_NETA_RXQ_CTRL    *rx_ctrl;

	wakeup = 0;
	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		rx_ctrl = pp->rxq_ctrl[rxq].q;

		if (rx_ctrl == NULL)
			continue;

		rx_done = mvNetaRxqBusyDescNumGet(pp->port, rxq);

		for (i = 0; i < rx_done; i++) {
			rx_desc = mvNetaRxqNextDescGet(rx_ctrl);
			mvOsCacheLineInv(NULL, rx_desc);

#if defined(MV_CPU_BE)
			mvNetaRxqDescSwap(rx_desc);
#endif /* MV_CPU_BE */

			pkt = (struct eth_pbuf *)rx_desc->bufCookie;
			mvOsCacheInvalidate(NULL, pkt->pBuf + pkt->offset, rx_desc->dataSize);

			if (mv_pnc_wol_pkt_match(pp->port, pkt->pBuf + pkt->offset, rx_desc->dataSize, &ruleId))
				wakeup = 1;

			pool = &mv_eth_pool[pkt->pool];
			mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);

			if (wakeup) {
				printk(KERN_INFO "packet match WoL rule=%d found on port=%d, rxq=%d\n",
						ruleId, port, rxq);
				i++;
				break;
			}
		}
		if (i) {
			mvNetaRxqDescNumUpdate(pp->port, rxq, i, i);
			printk(KERN_INFO "port=%d, rxq=%d: %d of %d packets dropped\n", port, rxq, i, rx_done);
		}
		if (wakeup) {
			/* Failed enter WoL mode */
			return 1;
		}
	}
	return MV_OK;
}

void mv_eth_wol_wakeup(int port)
{
	int rxq;

	/* Restore RXQ coalescing */
	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		mvNetaRxqPktsCoalSet(port, rxq, CONFIG_MV_ETH_RX_COAL_PKTS);
		mvNetaRxqTimeCoalSet(port, rxq, CONFIG_MV_ETH_RX_COAL_USEC);
	}

	/* Set PnC to Active filtering mode */
	mv_pnc_wol_wakeup(port);
	printk(KERN_INFO "Exit wakeOnLan mode on port #%d\n", port);

}

int mv_eth_wol_sleep(int port)
{
	int rxq, cpu;
	struct eth_port *pp;

	/* Set PnC to WoL filtering mode */
	mv_pnc_wol_sleep(port);

	pp = mv_eth_port_by_id(port);
	if (pp == NULL) {
		printk(KERN_INFO "Failed to fined pp struct on port #%d\n", port);
		return 1;
	}

	mv_eth_interrupts_mask(pp);

	/* wait until all napi stop transmit */
	for_each_possible_cpu(cpu) {
		if (pp->cpu_config[cpu]->napi)
			napi_synchronize(pp->cpu_config[cpu]->napi);
	}

	/* Check received packets in all RXQs */
	/* If match one of WoL pattern - wakeup, not match - drop */
	if (mv_eth_wol_pkts_check(port)) {
		/* Set PNC to Active filtering mode */
		mv_pnc_wol_wakeup(port);
		printk(KERN_INFO "Failed to enter wakeOnLan mode on port #%d\n", port);
		return 1;
	}
	printk(KERN_INFO "Enter wakeOnLan mode on port #%d\n", port);

	/* Set RXQ coalescing to minimum */
	for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
		mvNetaRxqPktsCoalSet(port, rxq, 0);
		mvNetaRxqTimeCoalSet(port, rxq, 0);
	}

	mv_eth_interrupts_unmask(pp);

	return MV_OK;
}
#endif /* CONFIG_MV_ETH_PNC_WOL */

#ifdef CONFIG_MV_PON
/* PON link status api */
PONLINKSTATUSPOLLFUNC pon_link_status_polling_func;

void pon_link_status_notify_func(MV_BOOL link_state)
{
	struct eth_port *pon_port = mv_eth_port_by_id(MV_PON_PORT_ID_GET());
	mv_eth_link_event(pon_port, 1);
}

/* called by PON module */
void mv_pon_link_state_register(PONLINKSTATUSPOLLFUNC poll_func, PONLINKSTATUSNOTIFYFUNC *notify_func)
{
	pon_link_status_polling_func = poll_func;
	*notify_func = pon_link_status_notify_func;
}

MV_BOOL mv_pon_link_status(void)
{
	if (pon_link_status_polling_func != NULL)
		return pon_link_status_polling_func();
	printk(KERN_ERR "pon_link_status_polling_func is uninitialized\n");
	return MV_FALSE;
}
#endif /* CONFIG_MV_PON */

/* Support for platform driver */

#ifdef CONFIG_CPU_IDLE

int mv_eth_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct eth_port *pp;
	int port;

	pm_flag = 0;

	for (port = 0 ; port < CONFIG_MV_ETH_PORTS_NUM ; port++) {
		pp = mv_eth_port_by_id(port);
		if (!pp)
			continue;
		if (pp->wol_mode == 0) {
			if (mv_eth_port_suspend(port)) {
				printk(KERN_ERR "%s: port #%d suspend failed.\n", __func__, port);
				return MV_ERROR;
			}

			/* BUG WA - if port 0 clock is down, we cant interrupt by magic packet */
			if ((port != 0) || (wol_ports_bmp == 0))
				/* Set Port Power State to 0 */
				mvCtrlPwrClckSet(ETH_GIG_UNIT_ID, port, 0);
		}

		else {

#ifdef CONFIG_MV_ETH_PNC_WOL
			if (pp->flags & MV_ETH_F_STARTED)
				if (mv_eth_wol_sleep(port)) {
					printk(KERN_ERR "%s: port #%d  WOL failed.\n", __func__, port);
					return MV_ERROR;
				}
#else
			printk(KERN_INFO "%s:WARNING port #%d in WOL mode but PNC WOL is not defined.\n", __func__, port);

#endif /*CONFIG_MV_ETH_PNC_WOL*/
		}
	}
	return MV_OK;
}

int mv_eth_resume(struct platform_device *pdev)
{
	struct eth_port *pp;
	int port;

	pm_flag = 0;

	for (port = 0 ; port < CONFIG_MV_ETH_PORTS_NUM ; port++) {
		pp = mv_eth_port_by_id(port);
		if (!pp)
			continue;

		if (pp->wol_mode == 0) {
			/* Set Port Power State to 1 */
			mvCtrlPwrClckSet(ETH_GIG_UNIT_ID, port, 1);
			mdelay(10);
			if (mv_eth_port_resume(port)) {
				printk(KERN_ERR "%s: port #%d resume failed.\n", __func__, port);
				return MV_ERROR;
			}
		} else
#ifdef CONFIG_MV_ETH_PNC_WOL
			mv_eth_wol_wakeup(port);
#else
			printk(KERN_ERR "%s:WARNING port #%d in WOL mode but PNC WOL is not defined.\n", __func__, port);
#endif /*CONFIG_MV_ETH_PNC_WOL*/

	}

	return MV_OK;
}

#endif	/*CONFIG_CPU_IDLE*/

static int mv_eth_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "Removing Marvell Ethernet Driver\n");
    return MV_OK;
}

static void mv_eth_shutdown(struct platform_device *pdev)
{
    printk(KERN_INFO "Shutting Down Marvell Ethernet Driver\n");
}

static struct platform_driver mv_eth_driver = {
	.probe = mv_eth_probe,
	.remove = mv_eth_remove,
	.shutdown = mv_eth_shutdown,
#ifdef CONFIG_CPU_IDLE
	.suspend = mv_eth_suspend,
	.resume = mv_eth_resume,
#endif /* CONFIG_CPU_IDLE */
	.driver = {
		.name = "mv88fx_neta",
	},
};

#ifdef MY_ABC_HERE
void syno_mv_net_shutdown()
{
	int i = 0, j = 0;
	MV_U16 macTmp[3];
	MV_U16 phyTmp;
#ifdef MY_ABC_HERE
	extern long g_egiga;
	if ( 0 == g_egiga ) {
		return;
	}
#endif

	for(i=0; i<mv_eth_ports_num; i++) {
		struct eth_port *pp = mv_eth_port_by_id(i);

		if (NULL == pp) {
			continue;
		}
	
		if (!syno_wol_support(pp)) {
			continue;
		}

		if (MV_PHY_ID_151X == pp->phy_chip) {
			/* Step 1: clear interrupt no matter enable or disable */
			mvEthPhyRegWrite(pp->phy_id, 0x16, 0x11);
			mvEthPhyRegWrite(pp->phy_id, 0x10, 0x1000);
			mvEthPhyRegWrite(pp->phy_id, 0x16, 0x0);

			/* Step 2: enalbe */
			if (pp->wol & WAKE_MAGIC) {
				for( j = 0; j < 3; ++j ) {
					macTmp[j] = (pp->dev->dev_addr[j*2] & 0xff) | (pp->dev->dev_addr[j*2 + 1] & 0xff) << 8;
					printk("MAC 0x%x\n", macTmp[j]);
				}

				mvEthPhyRegWrite(pp->phy_id, 0x16, 0x0);
				mvEthPhyRegRead(pp->phy_id, 0x12, &phyTmp);
				mvEthPhyRegWrite(pp->phy_id, 0x12, phyTmp | 0x80);
				mvEthPhyRegWrite(pp->phy_id, 0x16, 0x3);
				mvEthPhyRegRead(pp->phy_id,0x12,  &phyTmp); 
				mvEthPhyRegWrite(pp->phy_id, 0x12, (phyTmp & 0x7fff) | 0x4880);
				mvEthPhyRegWrite(pp->phy_id, 0x16, 0x11);
				mvEthPhyRegWrite(pp->phy_id, 0x17, macTmp[2]);
				mvEthPhyRegWrite(pp->phy_id, 0x18, macTmp[1]);
				mvEthPhyRegWrite(pp->phy_id, 0x19, macTmp[0]);
				mvEthPhyRegWrite(pp->phy_id, 0x10, 0x4000);
				mvEthPhyRegWrite(pp->phy_id, 0x16, 0x0);
			}
		}
	}
}
#endif
static int __init mv_eth_init_module(void)
{
#ifdef MY_ABC_HERE
	spin_lock_init( &mii_lock );
#endif
	return platform_driver_register(&mv_eth_driver);
}
module_init(mv_eth_init_module);

static void __exit mv_eth_cleanup_module(void)
{
	platform_driver_unregister(&mv_eth_driver);
}
module_exit(mv_eth_cleanup_module);

MODULE_DESCRIPTION("Marvell Ethernet Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");
