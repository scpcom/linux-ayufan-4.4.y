/**
 * $Header: /home/cvsroot/NasX86/Kernel/linux-3.2.26/drivers/target/iscsi/iscsi_target_log.c,v 1.3 2014/06/20 09:30:56 jonathanho Exp $
 *
 * Copyright (c) 2009, 2010 QNAP SYSTEMS, INC.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * @brief	iscsi target connection log function for lio target kernel modules.
 * @author	Nike Chen
 * @date	2009/11/23
 *
 * $Id: iscsi_target_log.c,v 1.3 2014/06/20 09:30:56 jonathanho Exp $
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>

#include "iscsi_target_log.h"

static int s_pid = -1;
static struct sock *netlink_sock = NULL;

/** iscsi target log send function.
*/
extern int iscsi_log_send_msg(int conn_type, int log_type, char *init_iqn, char *ip, char *target_iqn)
{
	iscsi_conn_log* conn_logP = NULL;
	int ret = -1;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	int size;
	int skblen;

	if (s_pid < 0 || !netlink_sock) {
		// printk("iscsi netlink is not ready, abort the log!!\n");
		return ret;
	}
	
	// allocate the log buffer
	size = sizeof(*conn_logP);
	skblen = NLMSG_SPACE(size);
	skb = alloc_skb(skblen, GFP_KERNEL);
	
	if (!skb) {
		printk("Fail to allocate log buffer, abort the log!!\n");
		return ret;
	}	
	
	// fill up the buffer
	nlh = nlmsg_put(skb,
	                s_pid,
	                0,
	                0,
	                size - sizeof(*nlh),
	                0);
	if (!nlh) {
		printk("Fail to fill with the log buffer, abort the log!!\n");
		kfree_skb(skb);
	}
	
	conn_logP = (iscsi_conn_log*) NLMSG_DATA(nlh);
	conn_logP->conn_type = conn_type;
	conn_logP->log_type = log_type;
	
	if (init_iqn)
		strcpy(conn_logP->init_iqn, init_iqn);
	if (ip)
		strcpy(conn_logP->init_ip, ip);
	if (target_iqn)
		strcpy(conn_logP->target_iqn, target_iqn);
		
	//ret = netlink_broadcast(netlink_sock, skb, 0, dst_groups, GFP_KERNEL);
	ret = nlmsg_unicast(netlink_sock, skb, s_pid);
	/*
	if (ret < 0)
		printk("Fail to send iscsi connection log (%s,%s,%s) size = %d, error code = 0x%x.\n", 
			init_iqn, ip, target_iqn, size, ret);
	else
		printk("Send iscsi connection log (%s,%s,%s) size = %d, successfully.\n",
			init_iqn, ip, target_iqn, size);
	*/

	// NOTE!! the allocated skb buffer should not deallocate explicitly, system will free
	// it in the appropriate time.
	// kfree_skb(skb);
	
	return ret;
}

/** iscsi target log receive function.
*/
static int iscsi_log_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
	// just need the pid right now

#if(LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
	s_pid = NETLINK_CB(skb).portid;
#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6))
	s_pid = NETLINK_CB(skb).pid;	
#else
#error "Ooo.. what kernel version do you compile ??"
#endif

	printk("%s: get log pid = %d.\n", __func__, s_pid);
	return 0;
}

/** iscsi target log receive upper layer function.
*/
static void iscsi_log_rcv(struct sk_buff *skb)
{
	netlink_rcv_skb(skb, &iscsi_log_rcv_msg);
}

/** iscsi target log subsystem initialize function.
*/
int iscsi_log_init()
{

#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,10,20)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
	struct netlink_kernel_cfg cfg = {
		.input		= iscsi_log_rcv,
		.groups 	= 0,
		.cb_mutex	= NULL,
	};

	/* bug 47438, bug 57880
	 * Bugfix about stop iscsi service and happen kernel panic 
	 */
	netlink_sock = netlink_kernel_create(&init_net, NETLINK_ISCSI_TARGET, &cfg);

#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6))

	netlink_sock = netlink_kernel_create(NULL,
				     NETLINK_ISCSI_TARGET,
				     0,
				     iscsi_log_rcv,
				     NULL,
				     THIS_MODULE);
#else
#error "Ooo.. what kernel version do you compile ??"
#endif


	if (!netlink_sock)
		printk("Fail to initiate iscsi target log!\n");
	else
		printk("Initiate iscsi target log successfully.\n");
	return 0;
}

/** iscsi target log subsystem cleanup function.
*/
void iscsi_log_cleanup()
{
	if (netlink_sock) {
		netlink_kernel_release(netlink_sock);
		printk("iscsi target log cleanup successfully.\n");
	}
}

