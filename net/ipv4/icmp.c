 
#include <linux/module.h>
#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <linux/netfilter_ipv4.h>
#include <net/snmp.h>
#include <net/ip.h>
#include <net/route.h>
#include <net/protocol.h>
#include <net/icmp.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/raw.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <net/checksum.h>
#include <net/xfrm.h>
#include <net/inet_common.h>

struct icmp_bxm {
	struct sk_buff *skb;
	int offset;
	int data_len;

	struct {
		struct icmphdr icmph;
		__be32	       times[3];
	} data;
	int head_len;
	struct ip_options replyopts;
	unsigned char  optbuf[40];
};

struct icmp_err icmp_err_convert[] = {
	{
		.errno = ENETUNREACH,	 
		.fatal = 0,
	},
	{
		.errno = EHOSTUNREACH,	 
		.fatal = 0,
	},
	{
		.errno = ENOPROTOOPT	 ,
		.fatal = 1,
	},
	{
		.errno = ECONNREFUSED,	 
		.fatal = 1,
	},
	{
		.errno = EMSGSIZE,	 
		.fatal = 0,
	},
	{
		.errno = EOPNOTSUPP,	 
		.fatal = 0,
	},
	{
		.errno = ENETUNREACH,	 
		.fatal = 1,
	},
	{
		.errno = EHOSTDOWN,	 
		.fatal = 1,
	},
	{
		.errno = ENONET,	 
		.fatal = 1,
	},
	{
		.errno = ENETUNREACH,	 
		.fatal = 1,
	},
	{
		.errno = EHOSTUNREACH,	 
		.fatal = 1,
	},
	{
		.errno = ENETUNREACH,	 
		.fatal = 0,
	},
	{
		.errno = EHOSTUNREACH,	 
		.fatal = 0,
	},
	{
		.errno = EHOSTUNREACH,	 
		.fatal = 1,
	},
	{
		.errno = EHOSTUNREACH,	 
		.fatal = 1,
	},
	{
		.errno = EHOSTUNREACH,	 
		.fatal = 1,
	},
};

struct icmp_control {
	void (*handler)(struct sk_buff *skb);
	short   error;		 
};

static const struct icmp_control icmp_pointers[NR_ICMP_TYPES+1];

static struct sock *icmp_sk(struct net *net)
{
	return net->ipv4.icmp_sk[smp_processor_id()];
}

static inline struct sock *icmp_xmit_lock(struct net *net)
{
	struct sock *sk;

	local_bh_disable();

	sk = icmp_sk(net);

#ifdef CONFIG_SYNO_PLX_PORTING
	if (unlikely(!wspin_trylock(&sk->sk_lock.slock))) {
#else
	if (unlikely(!spin_trylock(&sk->sk_lock.slock))) {
#endif
		 
		local_bh_enable();
		return NULL;
	}
	return sk;
}

static inline void icmp_xmit_unlock(struct sock *sk)
{
#ifdef CONFIG_SYNO_PLX_PORTING
	wspin_unlock_bh(&sk->sk_lock.slock);
#else
	spin_unlock_bh(&sk->sk_lock.slock);
#endif
}

#define XRLIM_BURST_FACTOR 6
int xrlim_allow(struct dst_entry *dst, int timeout)
{
	unsigned long now, token = dst->rate_tokens;
	int rc = 0;

	now = jiffies;
	token += now - dst->rate_last;
	dst->rate_last = now;
	if (token > XRLIM_BURST_FACTOR * timeout)
		token = XRLIM_BURST_FACTOR * timeout;
	if (token >= timeout) {
		token -= timeout;
		rc = 1;
	}
	dst->rate_tokens = token;
	return rc;
}

static inline int icmpv4_xrlim_allow(struct net *net, struct rtable *rt,
		int type, int code)
{
	struct dst_entry *dst = &rt->u.dst;
	int rc = 1;

	if (type > NR_ICMP_TYPES)
		goto out;

	if (type == ICMP_DEST_UNREACH && code == ICMP_FRAG_NEEDED)
		goto out;

	if (dst->dev && (dst->dev->flags&IFF_LOOPBACK))
		goto out;

	if ((1 << type) & net->ipv4.sysctl_icmp_ratemask)
		rc = xrlim_allow(dst, net->ipv4.sysctl_icmp_ratelimit);
out:
	return rc;
}

void icmp_out_count(struct net *net, unsigned char type)
{
	ICMPMSGOUT_INC_STATS(net, type);
	ICMP_INC_STATS(net, ICMP_MIB_OUTMSGS);
}

static int icmp_glue_bits(void *from, char *to, int offset, int len, int odd,
			  struct sk_buff *skb)
{
	struct icmp_bxm *icmp_param = (struct icmp_bxm *)from;
	__wsum csum;

	csum = skb_copy_and_csum_bits(icmp_param->skb,
				      icmp_param->offset + offset,
				      to, len, 0);

	skb->csum = csum_block_add(skb->csum, csum, odd);
	if (icmp_pointers[icmp_param->data.icmph.type].error)
		nf_ct_attach(skb, icmp_param->skb);
	return 0;
}

static void icmp_push_reply(struct icmp_bxm *icmp_param,
			    struct ipcm_cookie *ipc, struct rtable **rt)
{
	struct sock *sk;
	struct sk_buff *skb;

	sk = icmp_sk(dev_net((*rt)->u.dst.dev));
	if (ip_append_data(sk, icmp_glue_bits, icmp_param,
			   icmp_param->data_len+icmp_param->head_len,
			   icmp_param->head_len,
			   ipc, rt, MSG_DONTWAIT) < 0)
		ip_flush_pending_frames(sk);
	else if ((skb = skb_peek(&sk->sk_write_queue)) != NULL) {
		struct icmphdr *icmph = icmp_hdr(skb);
		__wsum csum = 0;
		struct sk_buff *skb1;

		skb_queue_walk(&sk->sk_write_queue, skb1) {
			csum = csum_add(csum, skb1->csum);
		}
		csum = csum_partial_copy_nocheck((void *)&icmp_param->data,
						 (char *)icmph,
						 icmp_param->head_len, csum);
		icmph->checksum = csum_fold(csum);
		skb->ip_summed = CHECKSUM_NONE;
		ip_push_pending_frames(sk);
	}
}

static void icmp_reply(struct icmp_bxm *icmp_param, struct sk_buff *skb)
{
	struct ipcm_cookie ipc;
	struct rtable *rt = skb_rtable(skb);
	struct net *net = dev_net(rt->u.dst.dev);
	struct sock *sk;
	struct inet_sock *inet;
	__be32 daddr;

	if (ip_options_echo(&icmp_param->replyopts, skb))
		return;

	sk = icmp_xmit_lock(net);
	if (sk == NULL)
		return;
	inet = inet_sk(sk);

	icmp_param->data.icmph.checksum = 0;

	inet->tos = ip_hdr(skb)->tos;
	daddr = ipc.addr = rt->rt_src;
	ipc.opt = NULL;
	ipc.shtx.flags = 0;
	if (icmp_param->replyopts.optlen) {
		ipc.opt = &icmp_param->replyopts;
		if (ipc.opt->srr)
			daddr = icmp_param->replyopts.faddr;
	}
	{
		struct flowi fl = { .nl_u = { .ip4_u =
					      { .daddr = daddr,
						.saddr = rt->rt_spec_dst,
						.tos = RT_TOS(ip_hdr(skb)->tos) } },
				    .proto = IPPROTO_ICMP };
		security_skb_classify_flow(skb, &fl);
		if (ip_route_output_key(net, &rt, &fl))
			goto out_unlock;
	}
	if (icmpv4_xrlim_allow(net, rt, icmp_param->data.icmph.type,
			       icmp_param->data.icmph.code))
		icmp_push_reply(icmp_param, &ipc, &rt);
	ip_rt_put(rt);
out_unlock:
	icmp_xmit_unlock(sk);
}

void icmp_send(struct sk_buff *skb_in, int type, int code, __be32 info)
{
	struct iphdr *iph;
	int room;
	struct icmp_bxm icmp_param;
	struct rtable *rt = skb_rtable(skb_in);
	struct ipcm_cookie ipc;
	__be32 saddr;
	u8  tos;
	struct net *net;
	struct sock *sk;

	if (!rt)
		goto out;
	net = dev_net(rt->u.dst.dev);

	iph = ip_hdr(skb_in);

	if ((u8 *)iph < skb_in->head ||
	    (skb_in->network_header + sizeof(*iph)) > skb_in->tail)
		goto out;

	if (skb_in->pkt_type != PACKET_HOST)
		goto out;

	if (rt->rt_flags & (RTCF_BROADCAST | RTCF_MULTICAST))
		goto out;

	if (iph->frag_off & htons(IP_OFFSET))
		goto out;

	if (icmp_pointers[type].error) {
		 
		if (iph->protocol == IPPROTO_ICMP) {
			u8 _inner_type, *itp;

			itp = skb_header_pointer(skb_in,
						 skb_network_header(skb_in) +
						 (iph->ihl << 2) +
						 offsetof(struct icmphdr,
							  type) -
						 skb_in->data,
						 sizeof(_inner_type),
						 &_inner_type);
			if (itp == NULL)
				goto out;

			if (*itp > NR_ICMP_TYPES ||
			    icmp_pointers[*itp].error)
				goto out;
		}
	}

	sk = icmp_xmit_lock(net);
	if (sk == NULL)
		return;

	saddr = iph->daddr;
	if (!(rt->rt_flags & RTCF_LOCAL)) {
		struct net_device *dev = NULL;

		if (rt->fl.iif &&
			net->ipv4.sysctl_icmp_errors_use_inbound_ifaddr)
			dev = dev_get_by_index(net, rt->fl.iif);

		if (dev) {
			saddr = inet_select_addr(dev, 0, RT_SCOPE_LINK);
			dev_put(dev);
		} else
			saddr = 0;
	}

	tos = icmp_pointers[type].error ? ((iph->tos & IPTOS_TOS_MASK) |
					   IPTOS_PREC_INTERNETCONTROL) :
					  iph->tos;

	if (ip_options_echo(&icmp_param.replyopts, skb_in))
		goto out_unlock;

	icmp_param.data.icmph.type	 = type;
	icmp_param.data.icmph.code	 = code;
	icmp_param.data.icmph.un.gateway = info;
	icmp_param.data.icmph.checksum	 = 0;
	icmp_param.skb	  = skb_in;
	icmp_param.offset = skb_network_offset(skb_in);
	inet_sk(sk)->tos = tos;
	ipc.addr = iph->saddr;
	ipc.opt = &icmp_param.replyopts;
	ipc.shtx.flags = 0;

	{
		struct flowi fl = {
			.nl_u = {
				.ip4_u = {
					.daddr = icmp_param.replyopts.srr ?
						icmp_param.replyopts.faddr :
						iph->saddr,
					.saddr = saddr,
					.tos = RT_TOS(tos)
				}
			},
			.proto = IPPROTO_ICMP,
			.uli_u = {
				.icmpt = {
					.type = type,
					.code = code
				}
			}
		};
		int err;
		struct rtable *rt2;

		security_skb_classify_flow(skb_in, &fl);
		if (__ip_route_output_key(net, &rt, &fl))
			goto out_unlock;

		rt2 = rt;

		err = xfrm_lookup(net, (struct dst_entry **)&rt, &fl, NULL, 0);
		switch (err) {
		case 0:
			if (rt != rt2)
				goto route_done;
			break;
		case -EPERM:
			rt = NULL;
			break;
		default:
			goto out_unlock;
		}

		if (xfrm_decode_session_reverse(skb_in, &fl, AF_INET))
			goto relookup_failed;

		if (inet_addr_type(net, fl.fl4_src) == RTN_LOCAL)
			err = __ip_route_output_key(net, &rt2, &fl);
		else {
			struct flowi fl2 = {};
			struct dst_entry *odst;

			fl2.fl4_dst = fl.fl4_src;
			if (ip_route_output_key(net, &rt2, &fl2))
				goto relookup_failed;

			odst = skb_dst(skb_in);
			err = ip_route_input(skb_in, fl.fl4_dst, fl.fl4_src,
					     RT_TOS(tos), rt2->u.dst.dev);

			dst_release(&rt2->u.dst);
			rt2 = skb_rtable(skb_in);
			skb_dst_set(skb_in, odst);
		}

		if (err)
			goto relookup_failed;

		err = xfrm_lookup(net, (struct dst_entry **)&rt2, &fl, NULL,
				  XFRM_LOOKUP_ICMP);
		switch (err) {
		case 0:
			dst_release(&rt->u.dst);
			rt = rt2;
			break;
		case -EPERM:
			goto ende;
		default:
relookup_failed:
			if (!rt)
				goto out_unlock;
			break;
		}
	}

route_done:
	if (!icmpv4_xrlim_allow(net, rt, type, code))
		goto ende;

	room = dst_mtu(&rt->u.dst);
	if (room > 576)
		room = 576;
	room -= sizeof(struct iphdr) + icmp_param.replyopts.optlen;
	room -= sizeof(struct icmphdr);

	icmp_param.data_len = skb_in->len - icmp_param.offset;
	if (icmp_param.data_len > room)
		icmp_param.data_len = room;
	icmp_param.head_len = sizeof(struct icmphdr);

	icmp_push_reply(&icmp_param, &ipc, &rt);
ende:
	ip_rt_put(rt);
out_unlock:
	icmp_xmit_unlock(sk);
out:;
}

static void icmp_unreach(struct sk_buff *skb)
{
	struct iphdr *iph;
	struct icmphdr *icmph;
	int hash, protocol;
	const struct net_protocol *ipprot;
	u32 info = 0;
	struct net *net;

	net = dev_net(skb_dst(skb)->dev);

	if (!pskb_may_pull(skb, sizeof(struct iphdr)))
		goto out_err;

	icmph = icmp_hdr(skb);
	iph   = (struct iphdr *)skb->data;

	if (iph->ihl < 5)  
		goto out_err;

	if (icmph->type == ICMP_DEST_UNREACH) {
		switch (icmph->code & 15) {
		case ICMP_NET_UNREACH:
		case ICMP_HOST_UNREACH:
		case ICMP_PROT_UNREACH:
		case ICMP_PORT_UNREACH:
			break;
		case ICMP_FRAG_NEEDED:
			if (ipv4_config.no_pmtu_disc) {
				LIMIT_NETDEBUG(KERN_INFO "ICMP: %pI4: fragmentation needed and DF set.\n",
					       &iph->daddr);
			} else {
				info = ip_rt_frag_needed(net, iph,
							 ntohs(icmph->un.frag.mtu),
							 skb->dev);
				if (!info)
					goto out;
			}
			break;
		case ICMP_SR_FAILED:
			LIMIT_NETDEBUG(KERN_INFO "ICMP: %pI4: Source Route Failed.\n",
				       &iph->daddr);
			break;
		default:
			break;
		}
		if (icmph->code > NR_ICMP_UNREACH)
			goto out;
	} else if (icmph->type == ICMP_PARAMETERPROB)
		info = ntohl(icmph->un.gateway) >> 24;

	if (!net->ipv4.sysctl_icmp_ignore_bogus_error_responses &&
	    inet_addr_type(net, iph->daddr) == RTN_BROADCAST) {
		if (net_ratelimit())
			printk(KERN_WARNING "%pI4 sent an invalid ICMP "
					    "type %u, code %u "
					    "error to a broadcast: %pI4 on %s\n",
			       &ip_hdr(skb)->saddr,
			       icmph->type, icmph->code,
			       &iph->daddr,
			       skb->dev->name);
		goto out;
	}

	if (!pskb_may_pull(skb, iph->ihl * 4 + 8))
		goto out;

	iph = (struct iphdr *)skb->data;
	protocol = iph->protocol;

	raw_icmp_error(skb, protocol, info);

	hash = protocol & (MAX_INET_PROTOS - 1);
	rcu_read_lock();
	ipprot = rcu_dereference(inet_protos[hash]);
	if (ipprot && ipprot->err_handler)
		ipprot->err_handler(skb, info);
	rcu_read_unlock();

out:
	return;
out_err:
	ICMP_INC_STATS_BH(net, ICMP_MIB_INERRORS);
	goto out;
}

static void icmp_redirect(struct sk_buff *skb)
{
	struct iphdr *iph;

	if (skb->len < sizeof(struct iphdr))
		goto out_err;

	if (!pskb_may_pull(skb, sizeof(struct iphdr)))
		goto out;

	iph = (struct iphdr *)skb->data;

	switch (icmp_hdr(skb)->code & 7) {
	case ICMP_REDIR_NET:
	case ICMP_REDIR_NETTOS:
		 
	case ICMP_REDIR_HOST:
	case ICMP_REDIR_HOSTTOS:
		ip_rt_redirect(ip_hdr(skb)->saddr, iph->daddr,
			       icmp_hdr(skb)->un.gateway,
			       iph->saddr, skb->dev);
		break;
	}
out:
	return;
out_err:
	ICMP_INC_STATS_BH(dev_net(skb->dev), ICMP_MIB_INERRORS);
	goto out;
}

static void icmp_echo(struct sk_buff *skb)
{
	struct net *net;

	net = dev_net(skb_dst(skb)->dev);
	if (!net->ipv4.sysctl_icmp_echo_ignore_all) {
		struct icmp_bxm icmp_param;

		icmp_param.data.icmph	   = *icmp_hdr(skb);
		icmp_param.data.icmph.type = ICMP_ECHOREPLY;
		icmp_param.skb		   = skb;
		icmp_param.offset	   = 0;
		icmp_param.data_len	   = skb->len;
		icmp_param.head_len	   = sizeof(struct icmphdr);
		icmp_reply(&icmp_param, skb);
	}
}

static void icmp_timestamp(struct sk_buff *skb)
{
	struct timespec tv;
	struct icmp_bxm icmp_param;
	 
	if (skb->len < 4)
		goto out_err;

	getnstimeofday(&tv);
	icmp_param.data.times[1] = htonl((tv.tv_sec % 86400) * MSEC_PER_SEC +
					 tv.tv_nsec / NSEC_PER_MSEC);
	icmp_param.data.times[2] = icmp_param.data.times[1];
	if (skb_copy_bits(skb, 0, &icmp_param.data.times[0], 4))
		BUG();
	icmp_param.data.icmph	   = *icmp_hdr(skb);
	icmp_param.data.icmph.type = ICMP_TIMESTAMPREPLY;
	icmp_param.data.icmph.code = 0;
	icmp_param.skb		   = skb;
	icmp_param.offset	   = 0;
	icmp_param.data_len	   = 0;
	icmp_param.head_len	   = sizeof(struct icmphdr) + 12;
	icmp_reply(&icmp_param, skb);
out:
	return;
out_err:
	ICMP_INC_STATS_BH(dev_net(skb_dst(skb)->dev), ICMP_MIB_INERRORS);
	goto out;
}

static void icmp_address(struct sk_buff *skb)
{
#if 0
	if (net_ratelimit())
		printk(KERN_DEBUG "a guy asks for address mask. Who is it?\n");
#endif
}

static void icmp_address_reply(struct sk_buff *skb)
{
	struct rtable *rt = skb_rtable(skb);
	struct net_device *dev = skb->dev;
	struct in_device *in_dev;
	struct in_ifaddr *ifa;

	if (skb->len < 4 || !(rt->rt_flags&RTCF_DIRECTSRC))
		goto out;

	in_dev = in_dev_get(dev);
	if (!in_dev)
		goto out;
	rcu_read_lock();
	if (in_dev->ifa_list &&
	    IN_DEV_LOG_MARTIANS(in_dev) &&
	    IN_DEV_FORWARD(in_dev)) {
		__be32 _mask, *mp;

		mp = skb_header_pointer(skb, 0, sizeof(_mask), &_mask);
		BUG_ON(mp == NULL);
		for (ifa = in_dev->ifa_list; ifa; ifa = ifa->ifa_next) {
			if (*mp == ifa->ifa_mask &&
			    inet_ifa_match(rt->rt_src, ifa))
				break;
		}
		if (!ifa && net_ratelimit()) {
			printk(KERN_INFO "Wrong address mask %pI4 from %s/%pI4\n",
			       mp, dev->name, &rt->rt_src);
		}
	}
	rcu_read_unlock();
	in_dev_put(in_dev);
out:;
}

static void icmp_discard(struct sk_buff *skb)
{
}

int icmp_rcv(struct sk_buff *skb)
{
	struct icmphdr *icmph;
	struct rtable *rt = skb_rtable(skb);
	struct net *net = dev_net(rt->u.dst.dev);

	if (!xfrm4_policy_check(NULL, XFRM_POLICY_IN, skb)) {
		struct sec_path *sp = skb_sec_path(skb);
		int nh;

		if (!(sp && sp->xvec[sp->len - 1]->props.flags &
				 XFRM_STATE_ICMP))
			goto drop;

		if (!pskb_may_pull(skb, sizeof(*icmph) + sizeof(struct iphdr)))
			goto drop;

		nh = skb_network_offset(skb);
		skb_set_network_header(skb, sizeof(*icmph));

		if (!xfrm4_policy_check_reverse(NULL, XFRM_POLICY_IN, skb))
			goto drop;

		skb_set_network_header(skb, nh);
	}

	ICMP_INC_STATS_BH(net, ICMP_MIB_INMSGS);

	switch (skb->ip_summed) {
	case CHECKSUM_COMPLETE:
		if (!csum_fold(skb->csum))
			break;
		 
	case CHECKSUM_NONE:
		skb->csum = 0;
		if (__skb_checksum_complete(skb))
			goto error;
	}

	if (!pskb_pull(skb, sizeof(*icmph)))
		goto error;

	icmph = icmp_hdr(skb);

	ICMPMSGIN_INC_STATS_BH(net, icmph->type);
	 
	if (icmph->type > NR_ICMP_TYPES)
		goto error;

	if (rt->rt_flags & (RTCF_BROADCAST | RTCF_MULTICAST)) {
		 
		if ((icmph->type == ICMP_ECHO ||
		     icmph->type == ICMP_TIMESTAMP) &&
		    net->ipv4.sysctl_icmp_echo_ignore_broadcasts) {
			goto error;
		}
		if (icmph->type != ICMP_ECHO &&
		    icmph->type != ICMP_TIMESTAMP &&
		    icmph->type != ICMP_ADDRESS &&
		    icmph->type != ICMP_ADDRESSREPLY) {
			goto error;
		}
	}

	icmp_pointers[icmph->type].handler(skb);

drop:
	kfree_skb(skb);
	return 0;
error:
	ICMP_INC_STATS_BH(net, ICMP_MIB_INERRORS);
	goto drop;
}

static const struct icmp_control icmp_pointers[NR_ICMP_TYPES + 1] = {
	[ICMP_ECHOREPLY] = {
		.handler = icmp_discard,
	},
	[1] = {
		.handler = icmp_discard,
		.error = 1,
	},
	[2] = {
		.handler = icmp_discard,
		.error = 1,
	},
	[ICMP_DEST_UNREACH] = {
		.handler = icmp_unreach,
		.error = 1,
	},
	[ICMP_SOURCE_QUENCH] = {
		.handler = icmp_unreach,
		.error = 1,
	},
	[ICMP_REDIRECT] = {
		.handler = icmp_redirect,
		.error = 1,
	},
	[6] = {
		.handler = icmp_discard,
		.error = 1,
	},
	[7] = {
		.handler = icmp_discard,
		.error = 1,
	},
	[ICMP_ECHO] = {
		.handler = icmp_echo,
	},
	[9] = {
		.handler = icmp_discard,
		.error = 1,
	},
	[10] = {
		.handler = icmp_discard,
		.error = 1,
	},
	[ICMP_TIME_EXCEEDED] = {
		.handler = icmp_unreach,
		.error = 1,
	},
	[ICMP_PARAMETERPROB] = {
		.handler = icmp_unreach,
		.error = 1,
	},
	[ICMP_TIMESTAMP] = {
		.handler = icmp_timestamp,
	},
	[ICMP_TIMESTAMPREPLY] = {
		.handler = icmp_discard,
	},
	[ICMP_INFO_REQUEST] = {
		.handler = icmp_discard,
	},
	[ICMP_INFO_REPLY] = {
		.handler = icmp_discard,
	},
	[ICMP_ADDRESS] = {
		.handler = icmp_address,
	},
	[ICMP_ADDRESSREPLY] = {
		.handler = icmp_address_reply,
	},
};

static void __net_exit icmp_sk_exit(struct net *net)
{
	int i;

	for_each_possible_cpu(i)
		inet_ctl_sock_destroy(net->ipv4.icmp_sk[i]);
	kfree(net->ipv4.icmp_sk);
	net->ipv4.icmp_sk = NULL;
}

static int __net_init icmp_sk_init(struct net *net)
{
	int i, err;

	net->ipv4.icmp_sk =
		kzalloc(nr_cpu_ids * sizeof(struct sock *), GFP_KERNEL);
	if (net->ipv4.icmp_sk == NULL)
		return -ENOMEM;

	for_each_possible_cpu(i) {
		struct sock *sk;

		err = inet_ctl_sock_create(&sk, PF_INET,
					   SOCK_RAW, IPPROTO_ICMP, net);
		if (err < 0)
			goto fail;

		net->ipv4.icmp_sk[i] = sk;

		sk->sk_sndbuf =
			(2 * ((64 * 1024) + sizeof(struct sk_buff)));

		inet_sk(sk)->pmtudisc = IP_PMTUDISC_DONT;
	}

	net->ipv4.sysctl_icmp_echo_ignore_all = 0;
	net->ipv4.sysctl_icmp_echo_ignore_broadcasts = 1;

	net->ipv4.sysctl_icmp_ignore_bogus_error_responses = 1;

	net->ipv4.sysctl_icmp_ratelimit = 1 * HZ;
	net->ipv4.sysctl_icmp_ratemask = 0x1818;
	net->ipv4.sysctl_icmp_errors_use_inbound_ifaddr = 0;

	return 0;

fail:
	for_each_possible_cpu(i)
		inet_ctl_sock_destroy(net->ipv4.icmp_sk[i]);
	kfree(net->ipv4.icmp_sk);
	return err;
}

static struct pernet_operations __net_initdata icmp_sk_ops = {
       .init = icmp_sk_init,
       .exit = icmp_sk_exit,
};

int __init icmp_init(void)
{
	return register_pernet_subsys(&icmp_sk_ops);
}

EXPORT_SYMBOL(icmp_err_convert);
EXPORT_SYMBOL(icmp_send);
EXPORT_SYMBOL(xrlim_allow);
