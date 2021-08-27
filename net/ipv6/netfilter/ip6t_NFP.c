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

MODULE_AUTHOR("Semihalf, Piotr Ziecik <kosmo@semihalf.com>");
MODULE_DESCRIPTION("IP tables NFP target module");
MODULE_LICENSE("GPL");

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/nf_conntrack_tcp.h>
#include <linux/netfilter/ipt_NFP.h>
#include <linux/mv_nfp.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_conntrack.h>

static unsigned int
target(struct sk_buff *skb, const struct xt_action_param *param)
{
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

	if (status & IPS_NAT_MASK) {
		tuple->nfp = false;
		return XT_CONTINUE;
	}
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

static struct xt_target xt_nfp6_target[] = {
	{
		.name		= "NFP",
		.family		= AF_INET6,
		.checkentry	= check,
		.target		= target,
		.hooks		= (1 << NF_INET_FORWARD),
		.targetsize	= XT_ALIGN(sizeof(struct ipt_nfp_info)),
		.me		= THIS_MODULE,
	},
};

static int __init xt_nfp6_init(void)
{
	need_conntrack();
	return xt_register_targets(xt_nfp6_target,
				   ARRAY_SIZE(xt_nfp6_target));
}

static void __exit xt_nfp6_fini(void)
{
	xt_unregister_targets(xt_nfp6_target,
			      ARRAY_SIZE(xt_nfp6_target));
}

module_init(xt_nfp6_init);
module_exit(xt_nfp6_fini);
