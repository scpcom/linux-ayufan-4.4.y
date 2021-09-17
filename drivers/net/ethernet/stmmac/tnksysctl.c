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

  Authors: Mark Burkley <mark@emutex.com>

*******************************************************************************/

/*
 * tnksysctl.c: sysctl interface for TNK subsystem.
 */

#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/fs.h>
#include <net/sock.h>

#include "tnkinfo.h"
#include "tnksysctl.h"
#include "common.h"

#ifdef TNK_DBG_HW_INTERFACE
#include "tnkhw.h"
#endif

#if !SWITCH_ZERO_PROBE
int tnk_tcp_persist_time = TNK_TCP_PERSIST_TIME;
#endif
#if SWITCH_RECV_LRO
int tnk_ct_remove_time = TNK_CT_REMOVE_TIME;
#endif
int tnk_threshold = TNK_THRESHOLD;
module_param(tnk_threshold, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tnk_threshold, "min:100bytes, typical:10x1024bytes,"
			"default:10*1024bytes");
int tnk_poll_limit = TNK_POLL_LIMIT;
int tnk_rx_lro_poll_limit = TNK_RX_LRO_POLL_LIMIT;
int tnk_tcp_rtt_sample_interval = TNK_TCP_RTT_SAMPLE_INTERVAL;
#ifdef TNK_RX_CHANNEL_FLOW_CONTROL
int tnk_rx_q_limit = TNK_RX_Q_LIMIT;
#endif
int tnk_send_max_msgsize = TNK_SEND_MAX_MSGSIZE;
int tnk_tcp_gmac0_enable = 1;
int tnk_tcp_gmac1_enable = 1;

int hitoe;
EXPORT_SYMBOL(hitoe);
module_param(hitoe, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hitoe, "0:Bypass 1:TOE");

int control_dbg;
int max_mss = 999999;
int tnk_macfifo_nospace_workaround;

#ifdef TNK_DBG_HW_INTERFACE
int tnk_ct_access_val[6];

int proc_tnk_ct_access(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	if (write && ret == 0)
		tnkhw_ct_access(tnk_ct_access_val);

	return ret;
}

int tnk_lookup_access_val[5];

int proc_tnk_lookup_access(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	if (write && ret == 0)
		tnkhw_lookup_access(tnk_lookup_access_val);

	return ret;
}

int tnk_dbg_cid_val;

int proc_tnk_dbg_cid(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	if (write && ret == 0)
		tnkhw_tx_err_dump_info(tnk_dbg_cid_val);

	return ret;
}
#endif

static struct ctl_table_header *tnk_table_header;
static struct ctl_table tnk_table[] = {
	{
		.procname	= "tnk_tcp_gmac0_enable",
		.data		= &tnk_tcp_gmac0_enable,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
	{
		.procname	= "tnk_tcp_gmac1_enable",
		.data		= &tnk_tcp_gmac1_enable,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
#if !SWITCH_ZERO_PROBE
	{
		.procname	= "tnk_tcp_persist_time",
		.data		= &tnk_tcp_persist_time,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
#endif
	{
		.procname	= "tnk_tcp_rtt_sample_interval",
		.data		= &tnk_tcp_rtt_sample_interval,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "tnk_bypass",
		.data		= &hitoe,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
	{
		.procname	= "tnk_threshold",
		.data		= &tnk_threshold,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
	{
		.procname	= "tnk_poll_limit",
		.data		= &tnk_poll_limit,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
#if SWITCH_RECV_LRO
	{
		.procname	= "tnk_rx_lro_poll_limit",
		.data		= &tnk_rx_lro_poll_limit,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
#endif
#ifdef TNK_RX_CHANNEL_FLOW_CONTROL
	{
		.procname	= "tnk_rx_q_limit",
		.data		= &tnk_rx_q_limit,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
#endif
	{
		.procname	= "control_dbg",
		.data		= &control_dbg,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
	{
		.procname	= "max_mss",
		.data		= &max_mss,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
	{
		.procname	= "tnk_send_max_msgsize",
		.data		= &tnk_send_max_msgsize,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec
	},
#ifdef TNK_DBG_HW_INTERFACE
	{
		.procname	= "tnk_ct_access",
		.data		= &tnk_ct_access_val,
		.maxlen		= sizeof(tnk_ct_access_val),
		.mode		= 0644,
		.proc_handler	= proc_tnk_ct_access
	},
	{
		.procname	= "tnk_lookup_access",
		.data		= &tnk_lookup_access_val,
		.maxlen		= sizeof(tnk_lookup_access_val),
		.mode		= 0644,
		.proc_handler	= proc_tnk_lookup_access
	},
	{
		.procname	= "tnk_dbg_cid",
		.data		= &tnk_dbg_cid_val,
		.maxlen		= sizeof(tnk_dbg_cid_val),
		.mode		= 0644,
		.proc_handler	= proc_tnk_dbg_cid
	},
#endif
	{ }
};

struct ctl_table tnk_path[] = {
	{
		.procname       = "tnk",
		.mode		= 0755,
		.child          = tnk_table
	},
	{ }
};

void tnk_sysctl_init(void)
{
	if (!tnk_table_header)
		tnk_table_header = register_sysctl_table(tnk_path);
}

void tnk_sysctl_shutdown(void)
{
	if (tnk_table_header) {
		unregister_sysctl_table(tnk_table_header);
		tnk_table_header = NULL;
	}
}
