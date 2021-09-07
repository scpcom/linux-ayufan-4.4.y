#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/netdevice.h>
#include <net/sock.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/icmp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/netfilter_ipv4.h>
#include <net/checksum.h>
#include <linux/route.h>
#include <net/route.h>
#include <net/xfrm.h>

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
extern int netdev_fastroute;
extern int netdev_fastroute_obstacles;

extern u32 gfar_fastroute_hash(u8 daddr, u8 saddr);
#endif
#endif

static int ip_forward_finish(struct sk_buff *skb)
{
	struct ip_options * opt	= &(IPCB(skb)->opt);

	IP_INC_STATS_BH(dev_net(skb_dst(skb)->dev), IPSTATS_MIB_OUTFORWDATAGRAMS);

	if (unlikely(opt->optlen))
		ip_forward_options(skb);

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
	else {
		struct rtable *rt = (struct rtable *)skb->_skb_dst;
#ifdef FASTPATH_DEBUG
		if (printk_ratelimit())
			printk(KERN_INFO" %s: rt = %p, rt->rt_flags = %x "
			       "(fast=%x), netdev_fastroute_ob=%d\n",
			       __func___, rt, rt ? rt->rt_flags : 0,
			       RTCF_FAST, netdev_fastroute_obstacles);
#endif
		if ((rt->rt_flags & RTCF_FAST) && !netdev_fastroute_obstacles) {
			struct dst_entry *old_dst;
			unsigned h = gfar_fastroute_hash(*(u8 *)&rt->rt_dst,
							 *(u8 *)&rt->rt_src);
#ifdef FASTPATH_DEBUG
			if (printk_ratelimit())
				printk(KERN_INFO " h = %d (%d, %d)\n",
				       h, rt->rt_dst, rt->rt_src);
#endif
			write_lock_irq(&skb->dev->fastpath_lock);
			old_dst = skb->dev->fastpath[h];
			skb->dev->fastpath[h] = dst_clone(&rt->u.dst);
			write_unlock_irq(&skb->dev->fastpath_lock);
			dst_release(old_dst);
		}
	}
#endif
#endif
	return dst_output(skb);
}

int ip_forward(struct sk_buff *skb)
{
	struct iphdr *iph;	 
	struct rtable *rt;	 
	struct ip_options * opt	= &(IPCB(skb)->opt);

	if (skb_warn_if_lro(skb))
		goto drop;

	if (!xfrm4_policy_check(NULL, XFRM_POLICY_FWD, skb))
		goto drop;

	if (IPCB(skb)->opt.router_alert && ip_call_ra_chain(skb))
		return NET_RX_SUCCESS;

	if (skb->pkt_type != PACKET_HOST)
		goto drop;

	skb_forward_csum(skb);

	if (ip_hdr(skb)->ttl <= 1)
		goto too_many_hops;

	if (!xfrm4_route_forward(skb))
		goto drop;

	rt = skb_rtable(skb);

	if (opt->is_strictroute && rt->rt_dst != rt->rt_gateway)
		goto sr_failed;

	if (unlikely(skb->len > dst_mtu(&rt->u.dst) && !skb_is_gso(skb) &&
		     (ip_hdr(skb)->frag_off & htons(IP_DF))) && !skb->local_df) {
		IP_INC_STATS(dev_net(rt->u.dst.dev), IPSTATS_MIB_FRAGFAILS);
		icmp_send(skb, ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED,
			  htonl(dst_mtu(&rt->u.dst)));
		goto drop;
	}

	if (skb_cow(skb, LL_RESERVED_SPACE(rt->u.dst.dev)+rt->u.dst.header_len))
		goto drop;
	iph = ip_hdr(skb);

	ip_decrease_ttl(iph);

	if (rt->rt_flags&RTCF_DOREDIRECT && !opt->srr && !skb_sec_path(skb))
		ip_rt_send_redirect(skb);

	skb->priority = rt_tos2priority(iph->tos);

	return NF_HOOK(PF_INET, NF_INET_FORWARD, skb, skb->dev, rt->u.dst.dev,
		       ip_forward_finish);

sr_failed:
	 
	 icmp_send(skb, ICMP_DEST_UNREACH, ICMP_SR_FAILED, 0);
	 goto drop;

too_many_hops:
	 
	IP_INC_STATS_BH(dev_net(skb_dst(skb)->dev), IPSTATS_MIB_INHDRERRORS);
	icmp_send(skb, ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, 0);
drop:
	kfree_skb(skb);
	return NET_RX_DROP;
}
