/*
 * Copyright (C) 2009, 2010 Semihalf.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>

MODULE_AUTHOR("Semihalf, Piotr Ziecik <kosmo@semihalf.com>");
MODULE_DESCRIPTION("IP tables NFP target module");
MODULE_LICENSE("GPL");

#include <linux/mv_nfp.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/nf_conntrack_tcp.h>
#include <linux/netfilter/ipt_NFP.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>

/* for debug */
void nfp_print_info(struct ipt_nfp_info *info)
{
	int i;

	printk(KERN_INFO "info = %p: mode = %d, flags = 0x%X, burst = %d, limit = %d, entry = %p\n",
		info, info->mode, info->flags, info->burst, info->limit, info->entry);

	printk(KERN_INFO "dscp = %d, new_dscp = %d\n", info->dscp, info->new_dscp);
	for (i = 0; i < IPT_NFP_DSCP_MAP_SIZE; i++) {
		if (info->dscp_map[i].valid)
			printk(KERN_INFO "dscp_map[%d] = %d\n", i, info->dscp_map[i].new_dscp);
	}
	printk(KERN_INFO "vlanprio = %d, new_vlanprio = %d\n", info->vlanprio, info->new_vlanprio);
	for (i = 0; i < IPT_NFP_VPRI_MAP_SIZE; i++) {
		if (info->vpri_map[i].valid)
			printk(KERN_INFO "vpri_map[%d] = %d\n", i, info->vpri_map[i].new_prio);
	}
	printk(KERN_INFO "dscp_txq=%d, txq = %d, txp = %d, mh = 0x%02X\n",
			info->dscp_txq, info->txq, info->txp, info->mh);
	for (i = 0; i <= IPT_NFP_DSCP_MAP_SIZE; i++) {
		if (info->txq_map[i].valid)
			printk(KERN_INFO "txq_map[%d] = %d\n", i, info->txq_map[i].txq);
	}
}

static unsigned int
target(struct sk_buff *skb, const struct xt_action_param *param)
{
	struct nf_conntrack_tuple target_tuple;
	struct nf_conntrack_tuple *tuple;
	enum ip_conntrack_info ctinfo;
	enum ip_conntrack_dir dir;
	struct ipt_nfp_info *info;
	struct nf_conn *ct;
	unsigned long status;

	ct = nf_ct_get(skb, &ctinfo);
	info = (struct ipt_nfp_info *)param->targinfo;

	if (!ct || !skb->dev)
		return XT_CONTINUE;

	dir = CTINFO2DIR(ctinfo);
	tuple = &ct->tuplehash[dir].tuple;

	if (tuple->info == NULL) {
		tuple->info = kmalloc(sizeof(struct ipt_nfp_info), GFP_ATOMIC);
		if (tuple->info == NULL) {
			printk(KERN_ERR "%s: OOM\n", __func__);
			return XT_CONTINUE;
		}
		memset(tuple->info, 0, sizeof(struct ipt_nfp_info));
	}

	nfp_update_tuple_info(tuple->info, info);

	tuple->nfpCapable = true;
	tuple->ifindex = skb->dev->ifindex;

	if ((!nfp_mgr_p->nfp_ct_is_learning_enabled) || (!nfp_mgr_p->nfp_is_learning_enabled))
		return XT_CONTINUE;
	if (!nfp_mgr_p->nfp_ct_is_learning_enabled() || !nfp_mgr_p->nfp_is_learning_enabled())
		return XT_CONTINUE;

	/* NFP handles TCP or UDP packets only */
	if (tuple->dst.protonum != IPPROTO_TCP &&
	    tuple->dst.protonum != IPPROTO_UDP)
		return XT_CONTINUE;

	/* If packet should be dropped, no need for further checks */
	if (tuple->info->mode == IPT_NFP_DROP) {
		tuple->nfp = true;
		if (nfp_mgr_p->nfp_hook_ct_fwd_add)
			nfp_mgr_p->nfp_hook_ct_fwd_add(tuple, 0);
		return NF_DROP;
	}
	/* do not handle IP_CT_RELATED connections to avoid ICMP error messages */
	if ((ctinfo == IP_CT_RELATED) || (ctinfo == IP_CT_RELATED + IP_CT_IS_REPLY))
		return XT_CONTINUE;

	/* NFP handles established or reply TCP connections */
	if (tuple->dst.protonum == IPPROTO_TCP) {
		if ((ctinfo != IP_CT_ESTABLISHED) && (ctinfo != (IP_CT_ESTABLISHED + IP_CT_IS_REPLY)))
			return XT_CONTINUE;

	    if (ct->proto.tcp.state != TCP_CONNTRACK_ESTABLISHED)
			return XT_CONTINUE;
	}

	/* Make sure connection is confirmed so we have all relevant NAT information */
	if (!(ct->status & IPS_CONFIRMED))
		return XT_CONTINUE;

	/* We've already handled IPT_NFP_DROP, so continue handling only if mode is IPT_NFP_FWD */
	if (tuple->info->mode != IPT_NFP_FWD)
		return XT_CONTINUE;

	tuple->nfp = true;
	status = ct->status;

	if ((status & IPS_NAT_MASK) && nfp_mgr_p->nfp_hook_ct_nat_add) {
		/* status says if the original direction requires SNAT or DNAT (or both) */
		/* if we currently work on the reply direction, we need to "reverse" the NAT status, */
		/* e.g. if original direction was SNAT, reply should be DNAT. */
		if (dir != IP_CT_DIR_ORIGINAL)
			status ^= IPS_NAT_MASK;

		nf_ct_invert_tuplepr(&target_tuple, &ct->tuplehash[!dir].tuple);

		if (tuple->dst.protonum == IPPROTO_UDP) {
			struct iphdr *iph = (struct iphdr *)skb_network_header(skb);

			if (iph->protocol == IPPROTO_UDP) {
				struct udphdr *uh = (struct udphdr *)((u32 *)iph + iph->ihl);

				if (uh->check)
					tuple->udpCsum = 1;
				else
					tuple->udpCsum = 0;
			}
		}

		if ((status & IPS_NAT_MASK) == IPS_DST_NAT) {
			/* DNAT only */
			nfp_mgr_p->nfp_hook_ct_nat_add(tuple, &target_tuple, IP_NAT_MANIP_DST);
		} else if ((status & IPS_NAT_MASK) == IPS_SRC_NAT) {
			/* SNAT only */
			nfp_mgr_p->nfp_hook_ct_nat_add(tuple, &target_tuple, IP_NAT_MANIP_SRC);
		} else {
			/* DNAT and SNAT */
			nfp_mgr_p->nfp_hook_ct_nat_add(tuple, &target_tuple, IP_NAT_MANIP_DST);
			nfp_mgr_p->nfp_hook_ct_nat_add(tuple, &target_tuple, IP_NAT_MANIP_SRC);
		}
		return NF_ACCEPT;
	}

	/* If we got till here, it must be IPT_NFP_FWD */
	if (nfp_mgr_p->nfp_hook_ct_fwd_add)
		nfp_mgr_p->nfp_hook_ct_fwd_add(tuple, 1);

	return NF_ACCEPT;
}

static int check(const struct xt_tgchk_param *par)
{
	struct ipt_nfp_info *info = par->targinfo;

	info->entry = (struct ipt_entry *)par->entryinfo;

	return 0;
}

static struct xt_target xt_nfp_target[] = {
	{
		.name		= "NFP",
		.family		= AF_INET,
		.checkentry	= check,
		.target		= target,
		.hooks		= (1 << NF_INET_FORWARD),
		.targetsize	= XT_ALIGN(sizeof(struct ipt_nfp_info)),
		.me		= THIS_MODULE,
	},
};

static int __init xt_nfp_init(void)
{
	need_conntrack();
	return xt_register_targets(xt_nfp_target,
				   ARRAY_SIZE(xt_nfp_target));
}

static void __exit xt_nfp_fini(void)
{
	xt_unregister_targets(xt_nfp_target,
			      ARRAY_SIZE(xt_nfp_target));
}

module_init(xt_nfp_init);
module_exit(xt_nfp_fini);
