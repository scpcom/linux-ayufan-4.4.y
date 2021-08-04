/*
 * net/key/af_key.c	An implementation of PF_KEYv2 sockets.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Maxim Giryaev	<gem@asplinux.ru>
 *		David S. Miller	<davem@redhat.com>
 *		Alexey Kuznetsov <kuznet@ms2.inr.ac.ru>
 *		Kunihiro Ishiguro <kunihiro@ipinfusion.com>
 *		Kazunori MIYAZAWA / USAGI Project <miyazawa@linux-ipv6.org>
 *		Derek Atkins <derek@ihtfp.com>
 */

#include <linux/capability.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/pfkeyv2.h>
#include <linux/ipsec.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <net/xfrm.h>
#include <net/netlink.h>

#include <net/sock.h>
#include <net/ip6_route.h>

#if defined(CONFIG_INET_IPSEC_OFFLOAD)|| defined(CONFIG_INET6_IPSEC_OFFLOAD)
#define NLKEY_SUPPORT 1
#else 
#undef NLKEY_SUPPORT
#endif 

#ifdef NLKEY_SUPPORT
#include <net/dsfield.h>
#include <net/inet_ecn.h>
#include <net/ipv6.h>


extern struct dst_entry *xfrm_dst_lookup(struct xfrm_state *x, int tos,
						xfrm_address_t *prev_saddr,
						xfrm_address_t *prev_daddr,
						int family);
extern int xfrm_get_tos(struct flowi *fl, int family);
extern int ipsec_nlkey_flow(u16 xfrm_nr, u16 *xfrm_handle, 
		const struct flowi *fl, u16 family, u16 dir);


#define	NLKEY_SA_CREATE		0x0A01
#define NLKEY_SA_DELETE		0x0A02
#define NLKEY_SA_FLUSH 		0x0A03
#define NLKEY_SA_SET_KEYS	0x0A04
#define NLKEY_SA_SET_TUNNEL	0x0A05
#define NLKEY_SA_SET_NATT	0x0A06
#define	NLKEY_SA_SET_STATE	0x0A07
#define	NLKEY_SA_SET_LIFETIME	0x0A08
#define	NLKEY_SA_NOTIFY		0x0A09
#define NLKEY_SA_INFO_UPDATE	0x0A0C
#define	NLKEY_FLOW_ADD		0x0A11
#define NLKEY_FLOW_REMOVE	0x0A12
#define NLKEY_FLOW_NOTIFY	0x0A13
#define NLKEY_NULL_MSG		0x0000

#define NLKEY_HDR_LEN		4
#define NLKEY_MSG_LEN 		256

#define NLKEY_MAX_NUM_KEYS	2
#define NLKEY_MAX_KEY_LEN	(256 / 8)

struct nlkey_msg {
	/* message data */
	unsigned short fcode;
	unsigned short length;
	unsigned short payload[(NLKEY_MSG_LEN /sizeof(unsigned short))];
};
/* sizeof(nlkey_msg) = 4 + 256 */

struct nlkey_sa_id {
	unsigned int spi;
	unsigned char sa_type;
	unsigned char proto_family;
	unsigned char replay_window;
#define NLKEY_SAFLAGS_ESN	0x1
	unsigned char flags;
	unsigned int dst_ip[4];
	unsigned int src_ip[4];
	unsigned short mtu;
	unsigned short dev_mtu;

};
/* sizeof(nlkey_sa_id) = 24 */

struct nlkey_sa_create {
	unsigned short sagd;
	unsigned short rsvd;
	struct nlkey_sa_id said;
};
/* sizeof(nlkey_sa_delete) = 28 */

struct nlkey_sa_delete {
	unsigned short sagd;
	unsigned short rsvd;
};
/* sizeof(nlkey_sa_delete) = 4 */

struct nlkey_sa_set_tunnel {
	unsigned short sagd;
	unsigned char rsvd;
	unsigned char proto_family;
	union {
		struct iphdr 	 ipv4h;
		struct ipv6hdr ipv6h;
	} h;
};
/* sizeof(nlkey_sa_set_tunnel) = 36 */

struct nlkey_sa_set_natt {
	unsigned short sagd;
	unsigned short sport;
	unsigned short dport;
	unsigned short rsvd;
};
/* sizeof(nlkey_sa_set_natt) = 4 */

struct nlkey_sa_set_state {
	unsigned short sagd;
	unsigned short rsvd;
	unsigned short state;
	unsigned short rsvd2;
};
/* sizeof(nlkey_sa_set_natt) = 8 */

struct nlkey_key_desc {
	unsigned short key_bits;
	unsigned char key_alg;
	unsigned char  key_type;
	unsigned char key[NLKEY_MAX_KEY_LEN]; 
};
/* sizeof(nlkey_key_desc) =  36 */

struct nlkey_sa_set_keys {
	unsigned short sagd;
	unsigned short rsvd;	
	unsigned short num_keys;
	unsigned short rsvd2;
	struct nlkey_key_desc keys[NLKEY_MAX_NUM_KEYS];
};
/* sizeof(nlkey_sa_set_keys) =  80 */

struct nlkey_lifetime_desc {
	unsigned int allocations;
	unsigned int bytes[2];
};
/* sizeof(nlkey_sa_set_lifetime) =  12 */

struct nlkey_sa_set_lifetime {
	unsigned short sagd;
	unsigned short rsvd;
	struct nlkey_lifetime_desc hard_time;
	struct nlkey_lifetime_desc soft_time;
	struct nlkey_lifetime_desc current_time;
};
/* sizeof(nlkey_sa_set_lifetime) =  40 */

/* SA notifications */
#define IPSEC_SOFT_EXPIRE 0
#define IPSEC_HARD_EXPIRE 1

struct nlkey_sa_notify {
	unsigned short sagd;
	unsigned short rsvd;
	unsigned int  action;
};
/* sizeof(nlkey_sa_notify) = 8 */

/* SA Info update */

struct nlkey_sa_info {
        unsigned short sagd;
        unsigned short rsvd;
        unsigned long long bytes;
        unsigned long long packets;
};
/* sizeof(nlkey_sa_info) =  */


static int ipsec_nlkey_send(struct net *net, struct xfrm_state *x, const struct km_event *c);
static void ipsec_nlkey_rcv(struct sk_buff *skb);
static void ipsec_nlkey_init(void);
static unsigned short ipsec_sacode_to_nlkeycode(unsigned short sa_code);
static struct sk_buff * ipsec_xfrm2nlkey (struct net *net, struct xfrm_state *x, 
					const struct km_event *c, unsigned short *msg_id);
static int ipsec_nlkey_set_said(struct net *net, struct xfrm_state *x, const struct km_event *c, struct nlkey_sa_id *said);

/* netlink NETLINK_KEY socket */
struct sock *nlkey_socket = NULL;

#endif
/************************************************************************************/


#define _X2KEY(x) ((x) == XFRM_INF ? 0 : (x))
#define _KEY2X(x) ((x) == 0 ? XFRM_INF : (x))

static int pfkey_net_id __read_mostly;
struct netns_pfkey {
	/* List of all pfkey sockets. */
	struct hlist_head table;
	atomic_t socks_nr;
};
static DEFINE_MUTEX(pfkey_mutex);

#define DUMMY_MARK 0
static struct xfrm_mark dummy_mark = {0, 0};
struct pfkey_sock {
	/* struct sock must be the first member of struct pfkey_sock */
	struct sock	sk;
	int		registered;
	int		promisc;

	struct {
		uint8_t		msg_version;
		uint32_t	msg_pid;
		int		(*dump)(struct pfkey_sock *sk);
		void		(*done)(struct pfkey_sock *sk);
		union {
			struct xfrm_policy_walk	policy;
			struct xfrm_state_walk	state;
		} u;
		struct sk_buff	*skb;
	} dump;
};

static inline struct pfkey_sock *pfkey_sk(struct sock *sk)
{
	return (struct pfkey_sock *)sk;
}

static int pfkey_can_dump(const struct sock *sk)
{
	if (3 * atomic_read(&sk->sk_rmem_alloc) <= 2 * sk->sk_rcvbuf)
		return 1;
	return 0;
}

static void pfkey_terminate_dump(struct pfkey_sock *pfk)
{
	if (pfk->dump.dump) {
		if (pfk->dump.skb) {
			kfree_skb(pfk->dump.skb);
			pfk->dump.skb = NULL;
		}
		pfk->dump.done(pfk);
		pfk->dump.dump = NULL;
		pfk->dump.done = NULL;
	}
}

static void pfkey_sock_destruct(struct sock *sk)
{
	struct net *net = sock_net(sk);
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);

	pfkey_terminate_dump(pfkey_sk(sk));
	skb_queue_purge(&sk->sk_receive_queue);

	if (!sock_flag(sk, SOCK_DEAD)) {
		pr_err("Attempt to release alive pfkey socket: %p\n", sk);
		return;
	}

	WARN_ON(atomic_read(&sk->sk_rmem_alloc));
	WARN_ON(atomic_read(&sk->sk_wmem_alloc));

	atomic_dec(&net_pfkey->socks_nr);
}

static const struct proto_ops pfkey_ops;

static void pfkey_insert(struct sock *sk)
{
	struct net *net = sock_net(sk);
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);

	mutex_lock(&pfkey_mutex);
	sk_add_node_rcu(sk, &net_pfkey->table);
	mutex_unlock(&pfkey_mutex);
}

static void pfkey_remove(struct sock *sk)
{
	mutex_lock(&pfkey_mutex);
	sk_del_node_init_rcu(sk);
	mutex_unlock(&pfkey_mutex);
}

static struct proto key_proto = {
	.name	  = "KEY",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct pfkey_sock),
};

static int pfkey_create(struct net *net, struct socket *sock, int protocol,
			int kern)
{
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);
	struct sock *sk;
	int err;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	if (sock->type != SOCK_RAW)
		return -ESOCKTNOSUPPORT;
	if (protocol != PF_KEY_V2)
		return -EPROTONOSUPPORT;

	err = -ENOMEM;
	sk = sk_alloc(net, PF_KEY, GFP_KERNEL, &key_proto);
	if (sk == NULL)
		goto out;

	sock->ops = &pfkey_ops;
	sock_init_data(sock, sk);

	sk->sk_family = PF_KEY;
	sk->sk_destruct = pfkey_sock_destruct;

	atomic_inc(&net_pfkey->socks_nr);

	pfkey_insert(sk);

	return 0;
out:
	return err;
}

static int pfkey_release(struct socket *sock)
{
	struct sock *sk = sock->sk;

	if (!sk)
		return 0;

	pfkey_remove(sk);

	sock_orphan(sk);
	sock->sk = NULL;
	skb_queue_purge(&sk->sk_write_queue);

	synchronize_rcu();
	sock_put(sk);

	return 0;
}

static int pfkey_broadcast_one(struct sk_buff *skb, struct sk_buff **skb2,
			       gfp_t allocation, struct sock *sk)
{
	int err = -ENOBUFS;

	sock_hold(sk);
	if (*skb2 == NULL) {
		if (atomic_read(&skb->users) != 1) {
			*skb2 = skb_clone(skb, allocation);
		} else {
			*skb2 = skb;
			atomic_inc(&skb->users);
		}
	}
	if (*skb2 != NULL) {
		if (atomic_read(&sk->sk_rmem_alloc) <= sk->sk_rcvbuf) {
			skb_orphan(*skb2);
			skb_set_owner_r(*skb2, sk);
			skb_queue_tail(&sk->sk_receive_queue, *skb2);
			sk->sk_data_ready(sk, (*skb2)->len);
			*skb2 = NULL;
			err = 0;
		}
	}
	sock_put(sk);
	return err;
}

/* Send SKB to all pfkey sockets matching selected criteria.  */
#define BROADCAST_ALL		0
#define BROADCAST_ONE		1
#define BROADCAST_REGISTERED	2
#define BROADCAST_PROMISC_ONLY	4
static int pfkey_broadcast(struct sk_buff *skb, gfp_t allocation,
			   int broadcast_flags, struct sock *one_sk,
			   struct net *net)
{
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);
	struct sock *sk;
	struct hlist_node *node;
	struct sk_buff *skb2 = NULL;
	int err = -ESRCH;

	/* XXX Do we need something like netlink_overrun?  I think
	 * XXX PF_KEY socket apps will not mind current behavior.
	 */
	if (!skb)
		return -ENOMEM;

	rcu_read_lock();
	sk_for_each_rcu(sk, node, &net_pfkey->table) {
		struct pfkey_sock *pfk = pfkey_sk(sk);
		int err2;

		/* Yes, it means that if you are meant to receive this
		 * pfkey message you receive it twice as promiscuous
		 * socket.
		 */
		if (pfk->promisc)
			pfkey_broadcast_one(skb, &skb2, allocation, sk);

		/* the exact target will be processed later */
		if (sk == one_sk)
			continue;
		if (broadcast_flags != BROADCAST_ALL) {
			if (broadcast_flags & BROADCAST_PROMISC_ONLY)
				continue;
			if ((broadcast_flags & BROADCAST_REGISTERED) &&
			    !pfk->registered)
				continue;
			if (broadcast_flags & BROADCAST_ONE)
				continue;
		}

		err2 = pfkey_broadcast_one(skb, &skb2, allocation, sk);

		/* Error is cleare after succecful sending to at least one
		 * registered KM */
		if ((broadcast_flags & BROADCAST_REGISTERED) && err)
			err = err2;
	}
	rcu_read_unlock();

	if (one_sk != NULL)
		err = pfkey_broadcast_one(skb, &skb2, allocation, one_sk);

	kfree_skb(skb2);
	kfree_skb(skb);
	return err;
}

static int pfkey_do_dump(struct pfkey_sock *pfk)
{
	struct sadb_msg *hdr;
	int rc;

	rc = pfk->dump.dump(pfk);
	if (rc == -ENOBUFS)
		return 0;

	if (pfk->dump.skb) {
		if (!pfkey_can_dump(&pfk->sk))
			return 0;

		hdr = (struct sadb_msg *) pfk->dump.skb->data;
		hdr->sadb_msg_seq = 0;
		hdr->sadb_msg_errno = rc;
		pfkey_broadcast(pfk->dump.skb, GFP_ATOMIC, BROADCAST_ONE,
				&pfk->sk, sock_net(&pfk->sk));
		pfk->dump.skb = NULL;
	}

	pfkey_terminate_dump(pfk);
	return rc;
}

static inline void pfkey_hdr_dup(struct sadb_msg *new,
				 const struct sadb_msg *orig)
{
	*new = *orig;
}

static int pfkey_error(const struct sadb_msg *orig, int err, struct sock *sk)
{
	struct sk_buff *skb = alloc_skb(sizeof(struct sadb_msg) + 16, GFP_KERNEL);
	struct sadb_msg *hdr;

	if (!skb)
		return -ENOBUFS;

	/* Woe be to the platform trying to support PFKEY yet
	 * having normal errnos outside the 1-255 range, inclusive.
	 */
	err = -err;
	if (err == ERESTARTSYS ||
	    err == ERESTARTNOHAND ||
	    err == ERESTARTNOINTR)
		err = EINTR;
	if (err >= 512)
		err = EINVAL;
	BUG_ON(err <= 0 || err >= 256);

	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	pfkey_hdr_dup(hdr, orig);
	hdr->sadb_msg_errno = (uint8_t) err;
	hdr->sadb_msg_len = (sizeof(struct sadb_msg) /
			     sizeof(uint64_t));

	pfkey_broadcast(skb, GFP_KERNEL, BROADCAST_ONE, sk, sock_net(sk));

	return 0;
}

static u8 sadb_ext_min_len[] = {
	[SADB_EXT_RESERVED]		= (u8) 0,
	[SADB_EXT_SA]			= (u8) sizeof(struct sadb_sa),
	[SADB_EXT_LIFETIME_CURRENT]	= (u8) sizeof(struct sadb_lifetime),
	[SADB_EXT_LIFETIME_HARD]	= (u8) sizeof(struct sadb_lifetime),
	[SADB_EXT_LIFETIME_SOFT]	= (u8) sizeof(struct sadb_lifetime),
	[SADB_EXT_ADDRESS_SRC]		= (u8) sizeof(struct sadb_address),
	[SADB_EXT_ADDRESS_DST]		= (u8) sizeof(struct sadb_address),
	[SADB_EXT_ADDRESS_PROXY]	= (u8) sizeof(struct sadb_address),
	[SADB_EXT_KEY_AUTH]		= (u8) sizeof(struct sadb_key),
	[SADB_EXT_KEY_ENCRYPT]		= (u8) sizeof(struct sadb_key),
	[SADB_EXT_IDENTITY_SRC]		= (u8) sizeof(struct sadb_ident),
	[SADB_EXT_IDENTITY_DST]		= (u8) sizeof(struct sadb_ident),
	[SADB_EXT_SENSITIVITY]		= (u8) sizeof(struct sadb_sens),
	[SADB_EXT_PROPOSAL]		= (u8) sizeof(struct sadb_prop),
	[SADB_EXT_SUPPORTED_AUTH]	= (u8) sizeof(struct sadb_supported),
	[SADB_EXT_SUPPORTED_ENCRYPT]	= (u8) sizeof(struct sadb_supported),
	[SADB_EXT_SPIRANGE]		= (u8) sizeof(struct sadb_spirange),
	[SADB_X_EXT_KMPRIVATE]		= (u8) sizeof(struct sadb_x_kmprivate),
	[SADB_X_EXT_POLICY]		= (u8) sizeof(struct sadb_x_policy),
	[SADB_X_EXT_SA2]		= (u8) sizeof(struct sadb_x_sa2),
	[SADB_X_EXT_NAT_T_TYPE]		= (u8) sizeof(struct sadb_x_nat_t_type),
	[SADB_X_EXT_NAT_T_SPORT]	= (u8) sizeof(struct sadb_x_nat_t_port),
	[SADB_X_EXT_NAT_T_DPORT]	= (u8) sizeof(struct sadb_x_nat_t_port),
	[SADB_X_EXT_NAT_T_OA]		= (u8) sizeof(struct sadb_address),
	[SADB_X_EXT_SEC_CTX]		= (u8) sizeof(struct sadb_x_sec_ctx),
	[SADB_X_EXT_KMADDRESS]		= (u8) sizeof(struct sadb_x_kmaddress),
};

/* Verify sadb_address_{len,prefixlen} against sa_family.  */
static int verify_address_len(const void *p)
{
	const struct sadb_address *sp = p;
	const struct sockaddr *addr = (const struct sockaddr *)(sp + 1);
	const struct sockaddr_in *sin;
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	const struct sockaddr_in6 *sin6;
#endif
	int len;

	switch (addr->sa_family) {
	case AF_INET:
		len = DIV_ROUND_UP(sizeof(*sp) + sizeof(*sin), sizeof(uint64_t));
		if (sp->sadb_address_len != len ||
		    sp->sadb_address_prefixlen > 32)
			return -EINVAL;
		break;
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
		len = DIV_ROUND_UP(sizeof(*sp) + sizeof(*sin6), sizeof(uint64_t));
		if (sp->sadb_address_len != len ||
		    sp->sadb_address_prefixlen > 128)
			return -EINVAL;
		break;
#endif
	default:
		/* It is user using kernel to keep track of security
		 * associations for another protocol, such as
		 * OSPF/RSVP/RIPV2/MIP.  It is user's job to verify
		 * lengths.
		 *
		 * XXX Actually, association/policy database is not yet
		 * XXX able to cope with arbitrary sockaddr families.
		 * XXX When it can, remove this -EINVAL.  -DaveM
		 */
		return -EINVAL;
		break;
	}

	return 0;
}

static inline int pfkey_sec_ctx_len(const struct sadb_x_sec_ctx *sec_ctx)
{
	return DIV_ROUND_UP(sizeof(struct sadb_x_sec_ctx) +
			    sec_ctx->sadb_x_ctx_len,
			    sizeof(uint64_t));
}

static inline int verify_sec_ctx_len(const void *p)
{
	const struct sadb_x_sec_ctx *sec_ctx = p;
	int len = sec_ctx->sadb_x_ctx_len;

	if (len > PAGE_SIZE)
		return -EINVAL;

	len = pfkey_sec_ctx_len(sec_ctx);

	if (sec_ctx->sadb_x_sec_len != len)
		return -EINVAL;

	return 0;
}

static inline struct xfrm_user_sec_ctx *pfkey_sadb2xfrm_user_sec_ctx(const struct sadb_x_sec_ctx *sec_ctx)
{
	struct xfrm_user_sec_ctx *uctx = NULL;
	int ctx_size = sec_ctx->sadb_x_ctx_len;

	uctx = kmalloc((sizeof(*uctx)+ctx_size), GFP_KERNEL);

	if (!uctx)
		return NULL;

	uctx->len = pfkey_sec_ctx_len(sec_ctx);
	uctx->exttype = sec_ctx->sadb_x_sec_exttype;
	uctx->ctx_doi = sec_ctx->sadb_x_ctx_doi;
	uctx->ctx_alg = sec_ctx->sadb_x_ctx_alg;
	uctx->ctx_len = sec_ctx->sadb_x_ctx_len;
	memcpy(uctx + 1, sec_ctx + 1,
	       uctx->ctx_len);

	return uctx;
}

static int present_and_same_family(const struct sadb_address *src,
				   const struct sadb_address *dst)
{
	const struct sockaddr *s_addr, *d_addr;

	if (!src || !dst)
		return 0;

	s_addr = (const struct sockaddr *)(src + 1);
	d_addr = (const struct sockaddr *)(dst + 1);
	if (s_addr->sa_family != d_addr->sa_family)
		return 0;
	if (s_addr->sa_family != AF_INET
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	    && s_addr->sa_family != AF_INET6
#endif
		)
		return 0;

	return 1;
}

static int parse_exthdrs(struct sk_buff *skb, const struct sadb_msg *hdr, void **ext_hdrs)
{
	const char *p = (char *) hdr;
	int len = skb->len;

	len -= sizeof(*hdr);
	p += sizeof(*hdr);
	while (len > 0) {
		const struct sadb_ext *ehdr = (const struct sadb_ext *) p;
		uint16_t ext_type;
		int ext_len;

		ext_len  = ehdr->sadb_ext_len;
		ext_len *= sizeof(uint64_t);
		ext_type = ehdr->sadb_ext_type;
		if (ext_len < sizeof(uint64_t) ||
		    ext_len > len ||
		    ext_type == SADB_EXT_RESERVED)
			return -EINVAL;

		if (ext_type <= SADB_EXT_MAX) {
			int min = (int) sadb_ext_min_len[ext_type];
			if (ext_len < min)
				return -EINVAL;
			if (ext_hdrs[ext_type-1] != NULL)
				return -EINVAL;
			if (ext_type == SADB_EXT_ADDRESS_SRC ||
			    ext_type == SADB_EXT_ADDRESS_DST ||
			    ext_type == SADB_EXT_ADDRESS_PROXY ||
			    ext_type == SADB_X_EXT_NAT_T_OA) {
				if (verify_address_len(p))
					return -EINVAL;
			}
			if (ext_type == SADB_X_EXT_SEC_CTX) {
				if (verify_sec_ctx_len(p))
					return -EINVAL;
			}
			ext_hdrs[ext_type-1] = (void *) p;
		}
		p   += ext_len;
		len -= ext_len;
	}

	return 0;
}

static uint16_t
pfkey_satype2proto(uint8_t satype)
{
	switch (satype) {
	case SADB_SATYPE_UNSPEC:
		return IPSEC_PROTO_ANY;
	case SADB_SATYPE_AH:
		return IPPROTO_AH;
	case SADB_SATYPE_ESP:
		return IPPROTO_ESP;
	case SADB_X_SATYPE_IPCOMP:
		return IPPROTO_COMP;
		break;
	default:
		return 0;
	}
	/* NOTREACHED */
}

static uint8_t
pfkey_proto2satype(uint16_t proto)
{
	switch (proto) {
	case IPPROTO_AH:
		return SADB_SATYPE_AH;
	case IPPROTO_ESP:
		return SADB_SATYPE_ESP;
	case IPPROTO_COMP:
		return SADB_X_SATYPE_IPCOMP;
		break;
	default:
		return 0;
	}
	/* NOTREACHED */
}

/* BTW, this scheme means that there is no way with PFKEY2 sockets to
 * say specifically 'just raw sockets' as we encode them as 255.
 */

static uint8_t pfkey_proto_to_xfrm(uint8_t proto)
{
	return proto == IPSEC_PROTO_ANY ? 0 : proto;
}

static uint8_t pfkey_proto_from_xfrm(uint8_t proto)
{
	return proto ? proto : IPSEC_PROTO_ANY;
}

static inline int pfkey_sockaddr_len(sa_family_t family)
{
	switch (family) {
	case AF_INET:
		return sizeof(struct sockaddr_in);
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
#endif
	}
	return 0;
}

static
int pfkey_sockaddr_extract(const struct sockaddr *sa, xfrm_address_t *xaddr)
{
	switch (sa->sa_family) {
	case AF_INET:
		xaddr->a4 =
			((struct sockaddr_in *)sa)->sin_addr.s_addr;
		return AF_INET;
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
		memcpy(xaddr->a6,
		       &((struct sockaddr_in6 *)sa)->sin6_addr,
		       sizeof(struct in6_addr));
		return AF_INET6;
#endif
	}
	return 0;
}

static
int pfkey_sadb_addr2xfrm_addr(const struct sadb_address *addr, xfrm_address_t *xaddr)
{
	return pfkey_sockaddr_extract((struct sockaddr *)(addr + 1),
				      xaddr);
}

static struct  xfrm_state *pfkey_xfrm_state_lookup(struct net *net, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	const struct sadb_sa *sa;
	const struct sadb_address *addr;
	uint16_t proto;
	unsigned short family;
	xfrm_address_t *xaddr;

	sa = ext_hdrs[SADB_EXT_SA - 1];
	if (sa == NULL)
		return NULL;

	proto = pfkey_satype2proto(hdr->sadb_msg_satype);
	if (proto == 0)
		return NULL;

	/* sadb_address_len should be checked by caller */
	addr = ext_hdrs[SADB_EXT_ADDRESS_DST - 1];
	if (addr == NULL)
		return NULL;

	family = ((const struct sockaddr *)(addr + 1))->sa_family;
	switch (family) {
	case AF_INET:
		xaddr = (xfrm_address_t *)&((const struct sockaddr_in *)(addr + 1))->sin_addr;
		break;
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
		xaddr = (xfrm_address_t *)&((const struct sockaddr_in6 *)(addr + 1))->sin6_addr;
		break;
#endif
	default:
		xaddr = NULL;
	}

	if (!xaddr)
		return NULL;

	return xfrm_state_lookup(net, DUMMY_MARK, xaddr, sa->sadb_sa_spi, proto, family);
}

#define PFKEY_ALIGN8(a) (1 + (((a) - 1) | (8 - 1)))

static int
pfkey_sockaddr_size(sa_family_t family)
{
	return PFKEY_ALIGN8(pfkey_sockaddr_len(family));
}

static inline int pfkey_mode_from_xfrm(int mode)
{
	switch(mode) {
	case XFRM_MODE_TRANSPORT:
		return IPSEC_MODE_TRANSPORT;
	case XFRM_MODE_TUNNEL:
		return IPSEC_MODE_TUNNEL;
	case XFRM_MODE_BEET:
		return IPSEC_MODE_BEET;
	default:
		return -1;
	}
}

static inline int pfkey_mode_to_xfrm(int mode)
{
	switch(mode) {
	case IPSEC_MODE_ANY:	/*XXX*/
	case IPSEC_MODE_TRANSPORT:
		return XFRM_MODE_TRANSPORT;
	case IPSEC_MODE_TUNNEL:
		return XFRM_MODE_TUNNEL;
	case IPSEC_MODE_BEET:
		return XFRM_MODE_BEET;
	default:
		return -1;
	}
}

static unsigned int pfkey_sockaddr_fill(const xfrm_address_t *xaddr, __be16 port,
					struct sockaddr *sa,
					unsigned short family)
{
	switch (family) {
	case AF_INET:
	    {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		sin->sin_family = AF_INET;
		sin->sin_port = port;
		sin->sin_addr.s_addr = xaddr->a4;
		memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
		return 32;
	    }
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
	    {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = port;
		sin6->sin6_flowinfo = 0;
		ipv6_addr_copy(&sin6->sin6_addr, (const struct in6_addr *)xaddr->a6);
		sin6->sin6_scope_id = 0;
		return 128;
	    }
#endif
	}
	return 0;
}

static struct sk_buff *__pfkey_xfrm_state2msg(const struct xfrm_state *x,
					      int add_keys, int hsc)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;
	struct sadb_sa *sa;
	struct sadb_lifetime *lifetime;
	struct sadb_address *addr;
	struct sadb_key *key;
	struct sadb_x_sa2 *sa2;
	struct sadb_x_sec_ctx *sec_ctx;
	struct xfrm_sec_ctx *xfrm_ctx;
	int ctx_size = 0;
	int size;
	int auth_key_size = 0;
	int encrypt_key_size = 0;
	int sockaddr_size;
	struct xfrm_encap_tmpl *natt = NULL;
	int mode;

	/* address family check */
	sockaddr_size = pfkey_sockaddr_size(x->props.family);
	if (!sockaddr_size)
		return ERR_PTR(-EINVAL);

	/* base, SA, (lifetime (HSC),) address(SD), (address(P),)
	   key(AE), (identity(SD),) (sensitivity)> */
	size = sizeof(struct sadb_msg) +sizeof(struct sadb_sa) +
		sizeof(struct sadb_lifetime) +
		((hsc & 1) ? sizeof(struct sadb_lifetime) : 0) +
		((hsc & 2) ? sizeof(struct sadb_lifetime) : 0) +
			sizeof(struct sadb_address)*2 +
				sockaddr_size*2 +
					sizeof(struct sadb_x_sa2);

	if ((xfrm_ctx = x->security)) {
		ctx_size = PFKEY_ALIGN8(xfrm_ctx->ctx_len);
		size += sizeof(struct sadb_x_sec_ctx) + ctx_size;
	}

	/* identity & sensitivity */
	if (xfrm_addr_cmp(&x->sel.saddr, &x->props.saddr, x->props.family))
		size += sizeof(struct sadb_address) + sockaddr_size;

	if (add_keys) {
		if (x->aalg && x->aalg->alg_key_len) {
			auth_key_size =
				PFKEY_ALIGN8((x->aalg->alg_key_len + 7) / 8);
			size += sizeof(struct sadb_key) + auth_key_size;
		}
		if (x->ealg && x->ealg->alg_key_len) {
			encrypt_key_size =
				PFKEY_ALIGN8((x->ealg->alg_key_len+7) / 8);
			size += sizeof(struct sadb_key) + encrypt_key_size;
		}
	}
	if (x->encap)
		natt = x->encap;

	if (natt && natt->encap_type) {
		size += sizeof(struct sadb_x_nat_t_type);
		size += sizeof(struct sadb_x_nat_t_port);
		size += sizeof(struct sadb_x_nat_t_port);
	}

	skb =  alloc_skb(size + 16, GFP_ATOMIC);
	if (skb == NULL)
		return ERR_PTR(-ENOBUFS);

	/* call should fill header later */
	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	memset(hdr, 0, size);	/* XXX do we need this ? */
	hdr->sadb_msg_len = size / sizeof(uint64_t);

	/* sa */
	sa = (struct sadb_sa *)  skb_put(skb, sizeof(struct sadb_sa));
	sa->sadb_sa_len = sizeof(struct sadb_sa)/sizeof(uint64_t);
	sa->sadb_sa_exttype = SADB_EXT_SA;
	sa->sadb_sa_spi = x->id.spi;
	sa->sadb_sa_replay = x->props.replay_window;
	switch (x->km.state) {
	case XFRM_STATE_VALID:
		sa->sadb_sa_state = x->km.dying ?
			SADB_SASTATE_DYING : SADB_SASTATE_MATURE;
		break;
	case XFRM_STATE_ACQ:
		sa->sadb_sa_state = SADB_SASTATE_LARVAL;
		break;
	default:
		sa->sadb_sa_state = SADB_SASTATE_DEAD;
		break;
	}
	sa->sadb_sa_auth = 0;
	if (x->aalg) {
		struct xfrm_algo_desc *a = xfrm_aalg_get_byname(x->aalg->alg_name, 0);
		sa->sadb_sa_auth = a ? a->desc.sadb_alg_id : 0;
	}
	sa->sadb_sa_encrypt = 0;
	BUG_ON(x->ealg && x->calg);
	if (x->ealg) {
		struct xfrm_algo_desc *a = xfrm_ealg_get_byname(x->ealg->alg_name, 0);
		sa->sadb_sa_encrypt = a ? a->desc.sadb_alg_id : 0;
	}
	/* KAME compatible: sadb_sa_encrypt is overloaded with calg id */
	if (x->calg) {
		struct xfrm_algo_desc *a = xfrm_calg_get_byname(x->calg->alg_name, 0);
		sa->sadb_sa_encrypt = a ? a->desc.sadb_alg_id : 0;
	}

	sa->sadb_sa_flags = 0;
	if (x->props.flags & XFRM_STATE_NOECN)
		sa->sadb_sa_flags |= SADB_SAFLAGS_NOECN;
	if (x->props.flags & XFRM_STATE_DECAP_DSCP)
		sa->sadb_sa_flags |= SADB_SAFLAGS_DECAP_DSCP;
	if (x->props.flags & XFRM_STATE_NOPMTUDISC)
		sa->sadb_sa_flags |= SADB_SAFLAGS_NOPMTUDISC;
	if (x->props.flags & XFRM_STATE_ESN)
		sa->sadb_sa_flags |= SADB_SAFLAGS_ESN;

	/* hard time */
	if (hsc & 2) {
		lifetime = (struct sadb_lifetime *)  skb_put(skb,
							     sizeof(struct sadb_lifetime));
		lifetime->sadb_lifetime_len =
			sizeof(struct sadb_lifetime)/sizeof(uint64_t);
		lifetime->sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
		lifetime->sadb_lifetime_allocations =  _X2KEY(x->lft.hard_packet_limit);
		lifetime->sadb_lifetime_bytes = _X2KEY(x->lft.hard_byte_limit);
		lifetime->sadb_lifetime_addtime = x->lft.hard_add_expires_seconds;
		lifetime->sadb_lifetime_usetime = x->lft.hard_use_expires_seconds;
	}
	/* soft time */
	if (hsc & 1) {
		lifetime = (struct sadb_lifetime *)  skb_put(skb,
							     sizeof(struct sadb_lifetime));
		lifetime->sadb_lifetime_len =
			sizeof(struct sadb_lifetime)/sizeof(uint64_t);
		lifetime->sadb_lifetime_exttype = SADB_EXT_LIFETIME_SOFT;
		lifetime->sadb_lifetime_allocations =  _X2KEY(x->lft.soft_packet_limit);
		lifetime->sadb_lifetime_bytes = _X2KEY(x->lft.soft_byte_limit);
		lifetime->sadb_lifetime_addtime = x->lft.soft_add_expires_seconds;
		lifetime->sadb_lifetime_usetime = x->lft.soft_use_expires_seconds;
	}
	/* current time */
	lifetime = (struct sadb_lifetime *)  skb_put(skb,
						     sizeof(struct sadb_lifetime));
	lifetime->sadb_lifetime_len =
		sizeof(struct sadb_lifetime)/sizeof(uint64_t);
	lifetime->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	lifetime->sadb_lifetime_allocations = x->curlft.packets;
	lifetime->sadb_lifetime_bytes = x->curlft.bytes;
	lifetime->sadb_lifetime_addtime = x->curlft.add_time;
	lifetime->sadb_lifetime_usetime = x->curlft.use_time;
	/* src address */
	addr = (struct sadb_address*) skb_put(skb,
					      sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
	/* "if the ports are non-zero, then the sadb_address_proto field,
	   normally zero, MUST be filled in with the transport
	   protocol's number." - RFC2367 */
	addr->sadb_address_proto = 0;
	addr->sadb_address_reserved = 0;

	addr->sadb_address_prefixlen =
		pfkey_sockaddr_fill(&x->props.saddr, 0,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	if (!addr->sadb_address_prefixlen)
		BUG();

	/* dst address */
	addr = (struct sadb_address*) skb_put(skb,
					      sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
	addr->sadb_address_proto = 0;
	addr->sadb_address_reserved = 0;

	addr->sadb_address_prefixlen =
		pfkey_sockaddr_fill(&x->id.daddr, 0,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	if (!addr->sadb_address_prefixlen)
		BUG();

	if (xfrm_addr_cmp(&x->sel.saddr, &x->props.saddr,
			  x->props.family)) {
		addr = (struct sadb_address*) skb_put(skb,
			sizeof(struct sadb_address)+sockaddr_size);
		addr->sadb_address_len =
			(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
		addr->sadb_address_exttype = SADB_EXT_ADDRESS_PROXY;
		addr->sadb_address_proto =
			pfkey_proto_from_xfrm(x->sel.proto);
		addr->sadb_address_prefixlen = x->sel.prefixlen_s;
		addr->sadb_address_reserved = 0;

		pfkey_sockaddr_fill(&x->sel.saddr, x->sel.sport,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	}

	/* auth key */
	if (add_keys && auth_key_size) {
		key = (struct sadb_key *) skb_put(skb,
						  sizeof(struct sadb_key)+auth_key_size);
		key->sadb_key_len = (sizeof(struct sadb_key) + auth_key_size) /
			sizeof(uint64_t);
		key->sadb_key_exttype = SADB_EXT_KEY_AUTH;
		key->sadb_key_bits = x->aalg->alg_key_len;
		key->sadb_key_reserved = 0;
		memcpy(key + 1, x->aalg->alg_key, (x->aalg->alg_key_len+7)/8);
	}
	/* encrypt key */
	if (add_keys && encrypt_key_size) {
		key = (struct sadb_key *) skb_put(skb,
						  sizeof(struct sadb_key)+encrypt_key_size);
		key->sadb_key_len = (sizeof(struct sadb_key) +
				     encrypt_key_size) / sizeof(uint64_t);
		key->sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
		key->sadb_key_bits = x->ealg->alg_key_len;
		key->sadb_key_reserved = 0;
		memcpy(key + 1, x->ealg->alg_key,
		       (x->ealg->alg_key_len+7)/8);
	}

	/* sa */
	sa2 = (struct sadb_x_sa2 *)  skb_put(skb, sizeof(struct sadb_x_sa2));
	sa2->sadb_x_sa2_len = sizeof(struct sadb_x_sa2)/sizeof(uint64_t);
	sa2->sadb_x_sa2_exttype = SADB_X_EXT_SA2;
	if ((mode = pfkey_mode_from_xfrm(x->props.mode)) < 0) {
		kfree_skb(skb);
		return ERR_PTR(-EINVAL);
	}
	sa2->sadb_x_sa2_mode = mode;
	sa2->sadb_x_sa2_reserved1 = 0;
	sa2->sadb_x_sa2_reserved2 = 0;
	sa2->sadb_x_sa2_sequence = 0;
	sa2->sadb_x_sa2_reqid = x->props.reqid;

	if (natt && natt->encap_type) {
		struct sadb_x_nat_t_type *n_type;
		struct sadb_x_nat_t_port *n_port;

		/* type */
		n_type = (struct sadb_x_nat_t_type*) skb_put(skb, sizeof(*n_type));
		n_type->sadb_x_nat_t_type_len = sizeof(*n_type)/sizeof(uint64_t);
		n_type->sadb_x_nat_t_type_exttype = SADB_X_EXT_NAT_T_TYPE;
		n_type->sadb_x_nat_t_type_type = natt->encap_type;
		n_type->sadb_x_nat_t_type_reserved[0] = 0;
		n_type->sadb_x_nat_t_type_reserved[1] = 0;
		n_type->sadb_x_nat_t_type_reserved[2] = 0;

		/* source port */
		n_port = (struct sadb_x_nat_t_port*) skb_put(skb, sizeof (*n_port));
		n_port->sadb_x_nat_t_port_len = sizeof(*n_port)/sizeof(uint64_t);
		n_port->sadb_x_nat_t_port_exttype = SADB_X_EXT_NAT_T_SPORT;
		n_port->sadb_x_nat_t_port_port = natt->encap_sport;
		n_port->sadb_x_nat_t_port_reserved = 0;

		/* dest port */
		n_port = (struct sadb_x_nat_t_port*) skb_put(skb, sizeof (*n_port));
		n_port->sadb_x_nat_t_port_len = sizeof(*n_port)/sizeof(uint64_t);
		n_port->sadb_x_nat_t_port_exttype = SADB_X_EXT_NAT_T_DPORT;
		n_port->sadb_x_nat_t_port_port = natt->encap_dport;
		n_port->sadb_x_nat_t_port_reserved = 0;
	}

	/* security context */
	if (xfrm_ctx) {
		sec_ctx = (struct sadb_x_sec_ctx *) skb_put(skb,
				sizeof(struct sadb_x_sec_ctx) + ctx_size);
		sec_ctx->sadb_x_sec_len =
		  (sizeof(struct sadb_x_sec_ctx) + ctx_size) / sizeof(uint64_t);
		sec_ctx->sadb_x_sec_exttype = SADB_X_EXT_SEC_CTX;
		sec_ctx->sadb_x_ctx_doi = xfrm_ctx->ctx_doi;
		sec_ctx->sadb_x_ctx_alg = xfrm_ctx->ctx_alg;
		sec_ctx->sadb_x_ctx_len = xfrm_ctx->ctx_len;
		memcpy(sec_ctx + 1, xfrm_ctx->ctx_str,
		       xfrm_ctx->ctx_len);
	}

	return skb;
}


static inline struct sk_buff *pfkey_xfrm_state2msg(const struct xfrm_state *x)
{
	struct sk_buff *skb;

	skb = __pfkey_xfrm_state2msg(x, 1, 3);

	return skb;
}

static inline struct sk_buff *pfkey_xfrm_state2msg_expire(const struct xfrm_state *x,
							  int hsc)
{
	return __pfkey_xfrm_state2msg(x, 0, hsc);
}

static struct xfrm_state * pfkey_msg2xfrm_state(struct net *net,
						const struct sadb_msg *hdr,
						void * const *ext_hdrs)
{
	struct xfrm_state *x;
	const struct sadb_lifetime *lifetime;
	const struct sadb_sa *sa;
	const struct sadb_key *key;
	const struct sadb_x_sec_ctx *sec_ctx;
	uint16_t proto;
	int err;


	sa = ext_hdrs[SADB_EXT_SA - 1];
	if (!sa ||
	    !present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST-1]))
		return ERR_PTR(-EINVAL);
	if (hdr->sadb_msg_satype == SADB_SATYPE_ESP &&
	    !ext_hdrs[SADB_EXT_KEY_ENCRYPT-1])
		return ERR_PTR(-EINVAL);
	if (hdr->sadb_msg_satype == SADB_SATYPE_AH &&
	    !ext_hdrs[SADB_EXT_KEY_AUTH-1])
		return ERR_PTR(-EINVAL);
	if (!!ext_hdrs[SADB_EXT_LIFETIME_HARD-1] !=
	    !!ext_hdrs[SADB_EXT_LIFETIME_SOFT-1])
		return ERR_PTR(-EINVAL);

	proto = pfkey_satype2proto(hdr->sadb_msg_satype);
	if (proto == 0)
		return ERR_PTR(-EINVAL);

	/* default error is no buffer space */
	err = -ENOBUFS;

	/* RFC2367:

   Only SADB_SASTATE_MATURE SAs may be submitted in an SADB_ADD message.
   SADB_SASTATE_LARVAL SAs are created by SADB_GETSPI and it is not
   sensible to add a new SA in the DYING or SADB_SASTATE_DEAD state.
   Therefore, the sadb_sa_state field of all submitted SAs MUST be
   SADB_SASTATE_MATURE and the kernel MUST return an error if this is
   not true.

	   However, KAME setkey always uses SADB_SASTATE_LARVAL.
	   Hence, we have to _ignore_ sadb_sa_state, which is also reasonable.
	 */
	if (sa->sadb_sa_auth > SADB_AALG_MAX ||
	    (hdr->sadb_msg_satype == SADB_X_SATYPE_IPCOMP &&
	     sa->sadb_sa_encrypt > SADB_X_CALG_MAX) ||
	    sa->sadb_sa_encrypt > SADB_EALG_MAX)
		return ERR_PTR(-EINVAL);
	key = ext_hdrs[SADB_EXT_KEY_AUTH - 1];
	if (key != NULL &&
	    sa->sadb_sa_auth != SADB_X_AALG_NULL &&
	    ((key->sadb_key_bits+7) / 8 == 0 ||
	     (key->sadb_key_bits+7) / 8 > key->sadb_key_len * sizeof(uint64_t)))
		return ERR_PTR(-EINVAL);
	key = ext_hdrs[SADB_EXT_KEY_ENCRYPT-1];
	if (key != NULL &&
	    sa->sadb_sa_encrypt != SADB_EALG_NULL &&
	    ((key->sadb_key_bits+7) / 8 == 0 ||
	     (key->sadb_key_bits+7) / 8 > key->sadb_key_len * sizeof(uint64_t)))
		return ERR_PTR(-EINVAL);

	x = xfrm_state_alloc(net);
	if (x == NULL)
		return ERR_PTR(-ENOBUFS);

	x->id.proto = proto;
	x->id.spi = sa->sadb_sa_spi;
	x->props.replay_window = sa->sadb_sa_replay;
	if (sa->sadb_sa_flags & SADB_SAFLAGS_NOECN)
		x->props.flags |= XFRM_STATE_NOECN;
	if (sa->sadb_sa_flags & SADB_SAFLAGS_DECAP_DSCP)
		x->props.flags |= XFRM_STATE_DECAP_DSCP;
	if (sa->sadb_sa_flags & SADB_SAFLAGS_NOPMTUDISC)
		x->props.flags |= XFRM_STATE_NOPMTUDISC;
	if (sa->sadb_sa_flags & SADB_SAFLAGS_ESN)
		x->props.flags |= XFRM_STATE_ESN;

	lifetime = ext_hdrs[SADB_EXT_LIFETIME_HARD - 1];
	if (lifetime != NULL) {
		x->lft.hard_packet_limit = _KEY2X(lifetime->sadb_lifetime_allocations);
		x->lft.hard_byte_limit = _KEY2X(lifetime->sadb_lifetime_bytes);
		x->lft.hard_add_expires_seconds = lifetime->sadb_lifetime_addtime;
		x->lft.hard_use_expires_seconds = lifetime->sadb_lifetime_usetime;
	}
	lifetime = ext_hdrs[SADB_EXT_LIFETIME_SOFT - 1];
	if (lifetime != NULL) {
		x->lft.soft_packet_limit = _KEY2X(lifetime->sadb_lifetime_allocations);
		x->lft.soft_byte_limit = _KEY2X(lifetime->sadb_lifetime_bytes);
		x->lft.soft_add_expires_seconds = lifetime->sadb_lifetime_addtime;
		x->lft.soft_use_expires_seconds = lifetime->sadb_lifetime_usetime;
	}

	sec_ctx = ext_hdrs[SADB_X_EXT_SEC_CTX - 1];
	if (sec_ctx != NULL) {
		struct xfrm_user_sec_ctx *uctx = pfkey_sadb2xfrm_user_sec_ctx(sec_ctx);

		if (!uctx)
			goto out;

		err = security_xfrm_state_alloc(x, uctx);
		kfree(uctx);

		if (err)
			goto out;
	}

	key = ext_hdrs[SADB_EXT_KEY_AUTH - 1];
	if (sa->sadb_sa_auth) {
		int keysize = 0;
		struct xfrm_algo_desc *a = xfrm_aalg_get_byid(sa->sadb_sa_auth);
		if (!a) {
			err = -ENOSYS;
			goto out;
		}
		if (key)
			keysize = (key->sadb_key_bits + 7) / 8;
		x->aalg = kmalloc(sizeof(*x->aalg) + keysize, GFP_KERNEL);
		if (!x->aalg)
			goto out;
		strcpy(x->aalg->alg_name, a->name);
		x->aalg->alg_key_len = 0;
		if (key) {
			x->aalg->alg_key_len = key->sadb_key_bits;
			memcpy(x->aalg->alg_key, key+1, keysize);
		}
		x->aalg->alg_trunc_len = a->uinfo.auth.icv_truncbits;
		x->props.aalgo = sa->sadb_sa_auth;
		/* x->algo.flags = sa->sadb_sa_flags; */
	}
	if (sa->sadb_sa_encrypt) {
		if (hdr->sadb_msg_satype == SADB_X_SATYPE_IPCOMP) {
			struct xfrm_algo_desc *a = xfrm_calg_get_byid(sa->sadb_sa_encrypt);
			if (!a) {
				err = -ENOSYS;
				goto out;
			}
			x->calg = kmalloc(sizeof(*x->calg), GFP_KERNEL);
			if (!x->calg)
				goto out;
			strcpy(x->calg->alg_name, a->name);
			x->props.calgo = sa->sadb_sa_encrypt;
		} else {
			int keysize = 0;
			struct xfrm_algo_desc *a = xfrm_ealg_get_byid(sa->sadb_sa_encrypt);
			if (!a) {
				err = -ENOSYS;
				goto out;
			}
			key = (struct sadb_key*) ext_hdrs[SADB_EXT_KEY_ENCRYPT-1];
			if (key)
				keysize = (key->sadb_key_bits + 7) / 8;
			x->ealg = kmalloc(sizeof(*x->ealg) + keysize, GFP_KERNEL);
			if (!x->ealg)
				goto out;
			strcpy(x->ealg->alg_name, a->name);
			x->ealg->alg_key_len = 0;
			if (key) {
				x->ealg->alg_key_len = key->sadb_key_bits;
				memcpy(x->ealg->alg_key, key+1, keysize);
			}
			x->props.ealgo = sa->sadb_sa_encrypt;
		}
	}
	/* x->algo.flags = sa->sadb_sa_flags; */

	x->props.family = pfkey_sadb_addr2xfrm_addr((struct sadb_address *) ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
						    &x->props.saddr);
	if (!x->props.family) {
		err = -EAFNOSUPPORT;
		goto out;
	}
	pfkey_sadb_addr2xfrm_addr((struct sadb_address *) ext_hdrs[SADB_EXT_ADDRESS_DST-1],
				  &x->id.daddr);

	if (ext_hdrs[SADB_X_EXT_SA2-1]) {
		const struct sadb_x_sa2 *sa2 = ext_hdrs[SADB_X_EXT_SA2-1];
		int mode = pfkey_mode_to_xfrm(sa2->sadb_x_sa2_mode);
		if (mode < 0) {
			err = -EINVAL;
			goto out;
		}
		x->props.mode = mode;
		x->props.reqid = sa2->sadb_x_sa2_reqid;
	}

	if (ext_hdrs[SADB_EXT_ADDRESS_PROXY-1]) {
		const struct sadb_address *addr = ext_hdrs[SADB_EXT_ADDRESS_PROXY-1];

		/* Nobody uses this, but we try. */
		x->sel.family = pfkey_sadb_addr2xfrm_addr(addr, &x->sel.saddr);
		x->sel.prefixlen_s = addr->sadb_address_prefixlen;
	}

	if (!x->sel.family)
		x->sel.family = x->props.family;

	if (ext_hdrs[SADB_X_EXT_NAT_T_TYPE-1]) {
		const struct sadb_x_nat_t_type* n_type;
		struct xfrm_encap_tmpl *natt;

		x->encap = kmalloc(sizeof(*x->encap), GFP_KERNEL);
		if (!x->encap)
			goto out;

		natt = x->encap;
		n_type = ext_hdrs[SADB_X_EXT_NAT_T_TYPE-1];
		natt->encap_type = n_type->sadb_x_nat_t_type_type;

		if (ext_hdrs[SADB_X_EXT_NAT_T_SPORT-1]) {
			const struct sadb_x_nat_t_port *n_port =
				ext_hdrs[SADB_X_EXT_NAT_T_SPORT-1];
			natt->encap_sport = n_port->sadb_x_nat_t_port_port;
		}
		if (ext_hdrs[SADB_X_EXT_NAT_T_DPORT-1]) {
			const struct sadb_x_nat_t_port *n_port =
				ext_hdrs[SADB_X_EXT_NAT_T_DPORT-1];
			natt->encap_dport = n_port->sadb_x_nat_t_port_port;
		}
		memset(&natt->encap_oa, 0, sizeof(natt->encap_oa));
	}

	err = xfrm_init_state(x);
	if (err)
		goto out;

	x->km.seq = hdr->sadb_msg_seq;
	return x;

out:
	x->km.state = XFRM_STATE_DEAD;
	xfrm_state_put(x);
	return ERR_PTR(err);
}

static int pfkey_reserved(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	return -EOPNOTSUPP;
}

static int pfkey_getspi(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	struct sk_buff *resp_skb;
	struct sadb_x_sa2 *sa2;
	struct sadb_address *saddr, *daddr;
	struct sadb_msg *out_hdr;
	struct sadb_spirange *range;
	struct xfrm_state *x = NULL;
	int mode;
	int err;
	u32 min_spi, max_spi;
	u32 reqid;
	u8 proto;
	unsigned short family;
	xfrm_address_t *xsaddr = NULL, *xdaddr = NULL;

	if (!present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST-1]))
		return -EINVAL;

	proto = pfkey_satype2proto(hdr->sadb_msg_satype);
	if (proto == 0)
		return -EINVAL;

	if ((sa2 = ext_hdrs[SADB_X_EXT_SA2-1]) != NULL) {
		mode = pfkey_mode_to_xfrm(sa2->sadb_x_sa2_mode);
		if (mode < 0)
			return -EINVAL;
		reqid = sa2->sadb_x_sa2_reqid;
	} else {
		mode = 0;
		reqid = 0;
	}

	saddr = ext_hdrs[SADB_EXT_ADDRESS_SRC-1];
	daddr = ext_hdrs[SADB_EXT_ADDRESS_DST-1];

	family = ((struct sockaddr *)(saddr + 1))->sa_family;
	switch (family) {
	case AF_INET:
		xdaddr = (xfrm_address_t *)&((struct sockaddr_in *)(daddr + 1))->sin_addr.s_addr;
		xsaddr = (xfrm_address_t *)&((struct sockaddr_in *)(saddr + 1))->sin_addr.s_addr;
		break;
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
		xdaddr = (xfrm_address_t *)&((struct sockaddr_in6 *)(daddr + 1))->sin6_addr;
		xsaddr = (xfrm_address_t *)&((struct sockaddr_in6 *)(saddr + 1))->sin6_addr;
		break;
#endif
	}

	if (hdr->sadb_msg_seq) {
		x = xfrm_find_acq_byseq(net, DUMMY_MARK, hdr->sadb_msg_seq);
		if (x && xfrm_addr_cmp(&x->id.daddr, xdaddr, family)) {
			xfrm_state_put(x);
			x = NULL;
		}
	}

	if (!x)
		x = xfrm_find_acq(net, &dummy_mark, mode, reqid, proto, xdaddr, xsaddr, 1, family);

	if (x == NULL)
		return -ENOENT;

	min_spi = 0x100;
	max_spi = 0x0fffffff;

	range = ext_hdrs[SADB_EXT_SPIRANGE-1];
	if (range) {
		min_spi = range->sadb_spirange_min;
		max_spi = range->sadb_spirange_max;
	}

	err = xfrm_alloc_spi(x, min_spi, max_spi);
	resp_skb = err ? ERR_PTR(err) : pfkey_xfrm_state2msg(x);

	if (IS_ERR(resp_skb)) {
		xfrm_state_put(x);
		return  PTR_ERR(resp_skb);
	}

	out_hdr = (struct sadb_msg *) resp_skb->data;
	out_hdr->sadb_msg_version = hdr->sadb_msg_version;
	out_hdr->sadb_msg_type = SADB_GETSPI;
	out_hdr->sadb_msg_satype = pfkey_proto2satype(proto);
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_reserved = 0;
	out_hdr->sadb_msg_seq = hdr->sadb_msg_seq;
	out_hdr->sadb_msg_pid = hdr->sadb_msg_pid;

	xfrm_state_put(x);

	pfkey_broadcast(resp_skb, GFP_KERNEL, BROADCAST_ONE, sk, net);

	return 0;
}

static int pfkey_acquire(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	struct xfrm_state *x;

	if (hdr->sadb_msg_len != sizeof(struct sadb_msg)/8)
		return -EOPNOTSUPP;

	if (hdr->sadb_msg_seq == 0 || hdr->sadb_msg_errno == 0)
		return 0;

	x = xfrm_find_acq_byseq(net, DUMMY_MARK, hdr->sadb_msg_seq);
	if (x == NULL)
		return 0;

	spin_lock_bh(&x->lock);
	if (x->km.state == XFRM_STATE_ACQ) {
		x->km.state = XFRM_STATE_ERROR;
		wake_up(&net->xfrm.km_waitq);
	}
	spin_unlock_bh(&x->lock);
	xfrm_state_put(x);
	return 0;
}

static inline int event2poltype(int event)
{
	switch (event) {
	case XFRM_MSG_DELPOLICY:
		return SADB_X_SPDDELETE;
	case XFRM_MSG_NEWPOLICY:
		return SADB_X_SPDADD;
	case XFRM_MSG_UPDPOLICY:
		return SADB_X_SPDUPDATE;
	case XFRM_MSG_POLEXPIRE:
	//	return SADB_X_SPDEXPIRE;
	default:
		pr_err("pfkey: Unknown policy event %d\n", event);
		break;
	}

	return 0;
}

static inline int event2keytype(int event)
{
	switch (event) {
	case XFRM_MSG_DELSA:
		return SADB_DELETE;
	case XFRM_MSG_NEWSA:
		return SADB_ADD;
	case XFRM_MSG_UPDSA:
		return SADB_UPDATE;
	case XFRM_MSG_EXPIRE:
		return SADB_EXPIRE;
	default:
		pr_err("pfkey: Unknown SA event %d\n", event);
		break;
	}

	return 0;
}

/* ADD/UPD/DEL */
static int key_notify_sa(struct xfrm_state *x, const struct km_event *c)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;

	skb = pfkey_xfrm_state2msg(x);

	if (IS_ERR(skb))
		return PTR_ERR(skb);

	hdr = (struct sadb_msg *) skb->data;
	hdr->sadb_msg_version = PF_KEY_V2;
	hdr->sadb_msg_type = event2keytype(c->event);
	hdr->sadb_msg_satype = pfkey_proto2satype(x->id.proto);
	hdr->sadb_msg_errno = 0;
	hdr->sadb_msg_reserved = 0;
	hdr->sadb_msg_seq = c->seq;
	hdr->sadb_msg_pid = c->pid;

	pfkey_broadcast(skb, GFP_ATOMIC, BROADCAST_ALL, NULL, xs_net(x));

	return 0;
}

static int pfkey_add(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	struct xfrm_state *x;
	int err;
	struct km_event c;

	x = pfkey_msg2xfrm_state(net, hdr, ext_hdrs);
	if (IS_ERR(x))
		return PTR_ERR(x);

	xfrm_state_hold(x);
	if (hdr->sadb_msg_type == SADB_ADD)
		err = xfrm_state_add(x);
	else
		err = xfrm_state_update(x);

	xfrm_audit_state_add(x, err ? 0 : 1,
			     audit_get_loginuid(current),
			     audit_get_sessionid(current), 0);

	if (err < 0) {
		x->km.state = XFRM_STATE_DEAD;
		__xfrm_state_put(x);
		goto out;
	}

	if (hdr->sadb_msg_type == SADB_ADD)
		c.event = XFRM_MSG_NEWSA;
	else
		c.event = XFRM_MSG_UPDSA;
	c.seq = hdr->sadb_msg_seq;
	c.pid = hdr->sadb_msg_pid;
	km_state_notify(x, &c);
out:
	xfrm_state_put(x);
	return err;
}

static int pfkey_delete(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	struct xfrm_state *x;
	struct km_event c;
	int err;

	if (!ext_hdrs[SADB_EXT_SA-1] ||
	    !present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST-1]))
		return -EINVAL;

	x = pfkey_xfrm_state_lookup(net, hdr, ext_hdrs);
	if (x == NULL)
		return -ESRCH;

	if ((err = security_xfrm_state_delete(x)))
		goto out;

	if (xfrm_state_kern(x)) {
		err = -EPERM;
		goto out;
	}

	err = xfrm_state_delete(x);

	if (err < 0)
		goto out;

	c.seq = hdr->sadb_msg_seq;
	c.pid = hdr->sadb_msg_pid;
	c.event = XFRM_MSG_DELSA;
	km_state_notify(x, &c);
out:
	xfrm_audit_state_delete(x, err ? 0 : 1,
				audit_get_loginuid(current),
				audit_get_sessionid(current), 0);
	xfrm_state_put(x);

	return err;
}

static int pfkey_get(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	__u8 proto;
	struct sk_buff *out_skb;
	struct sadb_msg *out_hdr;
	struct xfrm_state *x;

	if (!ext_hdrs[SADB_EXT_SA-1] ||
	    !present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST-1]))
		return -EINVAL;

	x = pfkey_xfrm_state_lookup(net, hdr, ext_hdrs);
	if (x == NULL)
		return -ESRCH;

	out_skb = pfkey_xfrm_state2msg(x);
	proto = x->id.proto;
	xfrm_state_put(x);
	if (IS_ERR(out_skb))
		return  PTR_ERR(out_skb);

	out_hdr = (struct sadb_msg *) out_skb->data;
	out_hdr->sadb_msg_version = hdr->sadb_msg_version;
	out_hdr->sadb_msg_type = SADB_GET;
	out_hdr->sadb_msg_satype = pfkey_proto2satype(proto);
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_reserved = 0;
	out_hdr->sadb_msg_seq = hdr->sadb_msg_seq;
	out_hdr->sadb_msg_pid = hdr->sadb_msg_pid;
	pfkey_broadcast(out_skb, GFP_ATOMIC, BROADCAST_ONE, sk, sock_net(sk));

	return 0;
}

static struct sk_buff *compose_sadb_supported(const struct sadb_msg *orig,
					      gfp_t allocation)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;
	int len, auth_len, enc_len, i;

	auth_len = xfrm_count_auth_supported();
	if (auth_len) {
		auth_len *= sizeof(struct sadb_alg);
		auth_len += sizeof(struct sadb_supported);
	}

	enc_len = xfrm_count_enc_supported();
	if (enc_len) {
		enc_len *= sizeof(struct sadb_alg);
		enc_len += sizeof(struct sadb_supported);
	}

	len = enc_len + auth_len + sizeof(struct sadb_msg);

	skb = alloc_skb(len + 16, allocation);
	if (!skb)
		goto out_put_algs;

	hdr = (struct sadb_msg *) skb_put(skb, sizeof(*hdr));
	pfkey_hdr_dup(hdr, orig);
	hdr->sadb_msg_errno = 0;
	hdr->sadb_msg_len = len / sizeof(uint64_t);

	if (auth_len) {
		struct sadb_supported *sp;
		struct sadb_alg *ap;

		sp = (struct sadb_supported *) skb_put(skb, auth_len);
		ap = (struct sadb_alg *) (sp + 1);

		sp->sadb_supported_len = auth_len / sizeof(uint64_t);
		sp->sadb_supported_exttype = SADB_EXT_SUPPORTED_AUTH;

		for (i = 0; ; i++) {
			struct xfrm_algo_desc *aalg = xfrm_aalg_get_byidx(i);
			if (!aalg)
				break;
			if (aalg->available)
				*ap++ = aalg->desc;
		}
	}

	if (enc_len) {
		struct sadb_supported *sp;
		struct sadb_alg *ap;

		sp = (struct sadb_supported *) skb_put(skb, enc_len);
		ap = (struct sadb_alg *) (sp + 1);

		sp->sadb_supported_len = enc_len / sizeof(uint64_t);
		sp->sadb_supported_exttype = SADB_EXT_SUPPORTED_ENCRYPT;

		for (i = 0; ; i++) {
			struct xfrm_algo_desc *ealg = xfrm_ealg_get_byidx(i);
			if (!ealg)
				break;
			if (ealg->available)
				*ap++ = ealg->desc;
		}
	}

out_put_algs:
	return skb;
}

static int pfkey_register(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct pfkey_sock *pfk = pfkey_sk(sk);
	struct sk_buff *supp_skb;

	if (hdr->sadb_msg_satype > SADB_SATYPE_MAX)
		return -EINVAL;

	if (hdr->sadb_msg_satype != SADB_SATYPE_UNSPEC) {
		if (pfk->registered&(1<<hdr->sadb_msg_satype))
			return -EEXIST;
		pfk->registered |= (1<<hdr->sadb_msg_satype);
	}

	xfrm_probe_algs();

	supp_skb = compose_sadb_supported(hdr, GFP_KERNEL);
	if (!supp_skb) {
		if (hdr->sadb_msg_satype != SADB_SATYPE_UNSPEC)
			pfk->registered &= ~(1<<hdr->sadb_msg_satype);

		return -ENOBUFS;
	}

	pfkey_broadcast(supp_skb, GFP_KERNEL, BROADCAST_REGISTERED, sk, sock_net(sk));

	return 0;
}

static int unicast_flush_resp(struct sock *sk, const struct sadb_msg *ihdr)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;

	skb = alloc_skb(sizeof(struct sadb_msg) + 16, GFP_ATOMIC);
	if (!skb)
		return -ENOBUFS;

	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	memcpy(hdr, ihdr, sizeof(struct sadb_msg));
	hdr->sadb_msg_errno = (uint8_t) 0;
	hdr->sadb_msg_len = (sizeof(struct sadb_msg) / sizeof(uint64_t));

	return pfkey_broadcast(skb, GFP_ATOMIC, BROADCAST_ONE, sk, sock_net(sk));
}

static int key_notify_sa_flush(const struct km_event *c)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;

	skb = alloc_skb(sizeof(struct sadb_msg) + 16, GFP_ATOMIC);
	if (!skb)
		return -ENOBUFS;
	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	hdr->sadb_msg_satype = pfkey_proto2satype(c->data.proto);
	hdr->sadb_msg_type = SADB_FLUSH;
	hdr->sadb_msg_seq = c->seq;
	hdr->sadb_msg_pid = c->pid;
	hdr->sadb_msg_version = PF_KEY_V2;
	hdr->sadb_msg_errno = (uint8_t) 0;
	hdr->sadb_msg_len = (sizeof(struct sadb_msg) / sizeof(uint64_t));
	hdr->sadb_msg_reserved = 0;

	pfkey_broadcast(skb, GFP_ATOMIC, BROADCAST_ALL, NULL, c->net);

	return 0;
}

static int pfkey_flush(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	unsigned proto;
	struct km_event c;
	struct xfrm_audit audit_info;
	int err, err2;

	proto = pfkey_satype2proto(hdr->sadb_msg_satype);
	if (proto == 0)
		return -EINVAL;

	audit_info.loginuid = audit_get_loginuid(current);
	audit_info.sessionid = audit_get_sessionid(current);
	audit_info.secid = 0;
	err = xfrm_state_flush(net, proto, &audit_info);
	err2 = unicast_flush_resp(sk, hdr);
	if (err || err2) {
		if (err == -ESRCH) /* empty table - go quietly */
			err = 0;
		return err ? err : err2;
	}

	c.data.proto = proto;
	c.seq = hdr->sadb_msg_seq;
	c.pid = hdr->sadb_msg_pid;
	c.event = XFRM_MSG_FLUSHSA;
	c.net = net;
	km_state_notify(NULL, &c);

	return 0;
}

static int dump_sa(struct xfrm_state *x, int count, void *ptr)
{
	struct pfkey_sock *pfk = ptr;
	struct sk_buff *out_skb;
	struct sadb_msg *out_hdr;

	if (!pfkey_can_dump(&pfk->sk))
		return -ENOBUFS;

	out_skb = pfkey_xfrm_state2msg(x);
	if (IS_ERR(out_skb))
		return PTR_ERR(out_skb);

	out_hdr = (struct sadb_msg *) out_skb->data;
	out_hdr->sadb_msg_version = pfk->dump.msg_version;
	out_hdr->sadb_msg_type = SADB_DUMP;
	out_hdr->sadb_msg_satype = pfkey_proto2satype(x->id.proto);
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_reserved = 0;
	out_hdr->sadb_msg_seq = count + 1;
	out_hdr->sadb_msg_pid = pfk->dump.msg_pid;

	if (pfk->dump.skb)
		pfkey_broadcast(pfk->dump.skb, GFP_ATOMIC, BROADCAST_ONE,
				&pfk->sk, sock_net(&pfk->sk));
	pfk->dump.skb = out_skb;

	return 0;
}

static int pfkey_dump_sa(struct pfkey_sock *pfk)
{
	struct net *net = sock_net(&pfk->sk);
	return xfrm_state_walk(net, &pfk->dump.u.state, dump_sa, (void *) pfk);
}

static void pfkey_dump_sa_done(struct pfkey_sock *pfk)
{
	xfrm_state_walk_done(&pfk->dump.u.state);
}

static int pfkey_dump(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	u8 proto;
	struct pfkey_sock *pfk = pfkey_sk(sk);

	if (pfk->dump.dump != NULL)
		return -EBUSY;

	proto = pfkey_satype2proto(hdr->sadb_msg_satype);
	if (proto == 0)
		return -EINVAL;

	pfk->dump.msg_version = hdr->sadb_msg_version;
	pfk->dump.msg_pid = hdr->sadb_msg_pid;
	pfk->dump.dump = pfkey_dump_sa;
	pfk->dump.done = pfkey_dump_sa_done;
	xfrm_state_walk_init(&pfk->dump.u.state, proto);

	return pfkey_do_dump(pfk);
}

static int pfkey_promisc(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct pfkey_sock *pfk = pfkey_sk(sk);
	int satype = hdr->sadb_msg_satype;
	bool reset_errno = false;

	if (hdr->sadb_msg_len == (sizeof(*hdr) / sizeof(uint64_t))) {
		reset_errno = true;
		if (satype != 0 && satype != 1)
			return -EINVAL;
		pfk->promisc = satype;
	}
	if (reset_errno && skb_cloned(skb))
		skb = skb_copy(skb, GFP_KERNEL);
	else
		skb = skb_clone(skb, GFP_KERNEL);

	if (reset_errno && skb) {
		struct sadb_msg *new_hdr = (struct sadb_msg *) skb->data;
		new_hdr->sadb_msg_errno = 0;
	}

	pfkey_broadcast(skb, GFP_KERNEL, BROADCAST_ALL, NULL, sock_net(sk));
	return 0;
}

static int check_reqid(struct xfrm_policy *xp, int dir, int count, void *ptr)
{
	int i;
	u32 reqid = *(u32*)ptr;

	for (i=0; i<xp->xfrm_nr; i++) {
		if (xp->xfrm_vec[i].reqid == reqid)
			return -EEXIST;
	}
	return 0;
}

static u32 gen_reqid(struct net *net)
{
	struct xfrm_policy_walk walk;
	u32 start;
	int rc;
	static u32 reqid = IPSEC_MANUAL_REQID_MAX;

	start = reqid;
	do {
		++reqid;
		if (reqid == 0)
			reqid = IPSEC_MANUAL_REQID_MAX+1;
		xfrm_policy_walk_init(&walk, XFRM_POLICY_TYPE_MAIN);
		rc = xfrm_policy_walk(net, &walk, check_reqid, (void*)&reqid);
		xfrm_policy_walk_done(&walk);
		if (rc != -EEXIST)
			return reqid;
	} while (reqid != start);
	return 0;
}

static int
parse_ipsecrequest(struct xfrm_policy *xp, struct sadb_x_ipsecrequest *rq)
{
	struct net *net = xp_net(xp);
	struct xfrm_tmpl *t = xp->xfrm_vec + xp->xfrm_nr;
	int mode;

	if (xp->xfrm_nr >= XFRM_MAX_DEPTH)
		return -ELOOP;

	if (rq->sadb_x_ipsecrequest_mode == 0)
		return -EINVAL;

	t->id.proto = rq->sadb_x_ipsecrequest_proto; /* XXX check proto */
	if ((mode = pfkey_mode_to_xfrm(rq->sadb_x_ipsecrequest_mode)) < 0)
		return -EINVAL;
	t->mode = mode;
	if (rq->sadb_x_ipsecrequest_level == IPSEC_LEVEL_USE)
		t->optional = 1;
	else if (rq->sadb_x_ipsecrequest_level == IPSEC_LEVEL_UNIQUE) {
		t->reqid = rq->sadb_x_ipsecrequest_reqid;
		if (t->reqid > IPSEC_MANUAL_REQID_MAX)
			t->reqid = 0;
		if (!t->reqid && !(t->reqid = gen_reqid(net)))
			return -ENOBUFS;
	}

	/* addresses present only in tunnel mode */
	if (t->mode == XFRM_MODE_TUNNEL) {
		u8 *sa = (u8 *) (rq + 1);
		int family, socklen;

		family = pfkey_sockaddr_extract((struct sockaddr *)sa,
						&t->saddr);
		if (!family)
			return -EINVAL;

		socklen = pfkey_sockaddr_len(family);
		if (pfkey_sockaddr_extract((struct sockaddr *)(sa + socklen),
					   &t->id.daddr) != family)
			return -EINVAL;
		t->encap_family = family;
	} else
		t->encap_family = xp->family;

	/* No way to set this via kame pfkey */
	t->allalgs = 1;
	xp->xfrm_nr++;
	return 0;
}

static int
parse_ipsecrequests(struct xfrm_policy *xp, struct sadb_x_policy *pol)
{
	int err;
	int len = pol->sadb_x_policy_len*8 - sizeof(struct sadb_x_policy);
	struct sadb_x_ipsecrequest *rq = (void*)(pol+1);

	while (len >= sizeof(struct sadb_x_ipsecrequest)) {
		if ((err = parse_ipsecrequest(xp, rq)) < 0)
			return err;
		len -= rq->sadb_x_ipsecrequest_len;
		rq = (void*)((u8*)rq + rq->sadb_x_ipsecrequest_len);
	}
	return 0;
}

static inline int pfkey_xfrm_policy2sec_ctx_size(const struct xfrm_policy *xp)
{
  struct xfrm_sec_ctx *xfrm_ctx = xp->security;

	if (xfrm_ctx) {
		int len = sizeof(struct sadb_x_sec_ctx);
		len += xfrm_ctx->ctx_len;
		return PFKEY_ALIGN8(len);
	}
	return 0;
}

static int pfkey_xfrm_policy2msg_size(const struct xfrm_policy *xp)
{
	const struct xfrm_tmpl *t;
	int sockaddr_size = pfkey_sockaddr_size(xp->family);
	int socklen = 0;
	int i;

	for (i=0; i<xp->xfrm_nr; i++) {
		t = xp->xfrm_vec + i;
		socklen += pfkey_sockaddr_len(t->encap_family);
	}

	return sizeof(struct sadb_msg) +
		(sizeof(struct sadb_lifetime) * 3) +
		(sizeof(struct sadb_address) * 2) +
		(sockaddr_size * 2) +
		sizeof(struct sadb_x_policy) +
		(xp->xfrm_nr * sizeof(struct sadb_x_ipsecrequest)) +
		(socklen * 2) +
		pfkey_xfrm_policy2sec_ctx_size(xp);
}

static struct sk_buff * pfkey_xfrm_policy2msg_prep(const struct xfrm_policy *xp)
{
	struct sk_buff *skb;
	int size;

	size = pfkey_xfrm_policy2msg_size(xp);

	skb =  alloc_skb(size + 16, GFP_ATOMIC);
	if (skb == NULL)
		return ERR_PTR(-ENOBUFS);

	return skb;
}

static int pfkey_xfrm_policy2msg(struct sk_buff *skb, const struct xfrm_policy *xp, int dir)
{
	struct sadb_msg *hdr;
	struct sadb_address *addr;
	struct sadb_lifetime *lifetime;
	struct sadb_x_policy *pol;
	struct sadb_x_sec_ctx *sec_ctx;
	struct xfrm_sec_ctx *xfrm_ctx;
	int i;
	int size;
	int sockaddr_size = pfkey_sockaddr_size(xp->family);
	int socklen = pfkey_sockaddr_len(xp->family);

	size = pfkey_xfrm_policy2msg_size(xp);

	/* call should fill header later */
	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	memset(hdr, 0, size);	/* XXX do we need this ? */

	/* src address */
	addr = (struct sadb_address*) skb_put(skb,
					      sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
	addr->sadb_address_proto = pfkey_proto_from_xfrm(xp->selector.proto);
	addr->sadb_address_prefixlen = xp->selector.prefixlen_s;
	addr->sadb_address_reserved = 0;
	if (!pfkey_sockaddr_fill(&xp->selector.saddr,
				 xp->selector.sport,
				 (struct sockaddr *) (addr + 1),
				 xp->family))
		BUG();

	/* dst address */
	addr = (struct sadb_address*) skb_put(skb,
					      sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
	addr->sadb_address_proto = pfkey_proto_from_xfrm(xp->selector.proto);
	addr->sadb_address_prefixlen = xp->selector.prefixlen_d;
	addr->sadb_address_reserved = 0;

	pfkey_sockaddr_fill(&xp->selector.daddr, xp->selector.dport,
			    (struct sockaddr *) (addr + 1),
			    xp->family);

	/* hard time */
	lifetime = (struct sadb_lifetime *)  skb_put(skb,
						     sizeof(struct sadb_lifetime));
	lifetime->sadb_lifetime_len =
		sizeof(struct sadb_lifetime)/sizeof(uint64_t);
	lifetime->sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
	lifetime->sadb_lifetime_allocations =  _X2KEY(xp->lft.hard_packet_limit);
	lifetime->sadb_lifetime_bytes = _X2KEY(xp->lft.hard_byte_limit);
	lifetime->sadb_lifetime_addtime = xp->lft.hard_add_expires_seconds;
	lifetime->sadb_lifetime_usetime = xp->lft.hard_use_expires_seconds;
	/* soft time */
	lifetime = (struct sadb_lifetime *)  skb_put(skb,
						     sizeof(struct sadb_lifetime));
	lifetime->sadb_lifetime_len =
		sizeof(struct sadb_lifetime)/sizeof(uint64_t);
	lifetime->sadb_lifetime_exttype = SADB_EXT_LIFETIME_SOFT;
	lifetime->sadb_lifetime_allocations =  _X2KEY(xp->lft.soft_packet_limit);
	lifetime->sadb_lifetime_bytes = _X2KEY(xp->lft.soft_byte_limit);
	lifetime->sadb_lifetime_addtime = xp->lft.soft_add_expires_seconds;
	lifetime->sadb_lifetime_usetime = xp->lft.soft_use_expires_seconds;
	/* current time */
	lifetime = (struct sadb_lifetime *)  skb_put(skb,
						     sizeof(struct sadb_lifetime));
	lifetime->sadb_lifetime_len =
		sizeof(struct sadb_lifetime)/sizeof(uint64_t);
	lifetime->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	lifetime->sadb_lifetime_allocations = xp->curlft.packets;
	lifetime->sadb_lifetime_bytes = xp->curlft.bytes;
	lifetime->sadb_lifetime_addtime = xp->curlft.add_time;
	lifetime->sadb_lifetime_usetime = xp->curlft.use_time;

	pol = (struct sadb_x_policy *)  skb_put(skb, sizeof(struct sadb_x_policy));
	pol->sadb_x_policy_len = sizeof(struct sadb_x_policy)/sizeof(uint64_t);
	pol->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	pol->sadb_x_policy_type = IPSEC_POLICY_DISCARD;
	if (xp->action == XFRM_POLICY_ALLOW) {
		if (xp->xfrm_nr)
			pol->sadb_x_policy_type = IPSEC_POLICY_IPSEC;
		else
			pol->sadb_x_policy_type = IPSEC_POLICY_NONE;
	}
	pol->sadb_x_policy_dir = dir+1;
	pol->sadb_x_policy_reserved = 0;
	pol->sadb_x_policy_id = xp->index;
	pol->sadb_x_policy_priority = xp->priority;

	for (i=0; i<xp->xfrm_nr; i++) {
		const struct xfrm_tmpl *t = xp->xfrm_vec + i;
		struct sadb_x_ipsecrequest *rq;
		int req_size;
		int mode;

		req_size = sizeof(struct sadb_x_ipsecrequest);
		if (t->mode == XFRM_MODE_TUNNEL) {
			socklen = pfkey_sockaddr_len(t->encap_family);
			req_size += socklen * 2;
		} else {
			size -= 2*socklen;
		}
		rq = (void*)skb_put(skb, req_size);
		pol->sadb_x_policy_len += req_size/8;
		memset(rq, 0, sizeof(*rq));
		rq->sadb_x_ipsecrequest_len = req_size;
		rq->sadb_x_ipsecrequest_proto = t->id.proto;
		if ((mode = pfkey_mode_from_xfrm(t->mode)) < 0)
			return -EINVAL;
		rq->sadb_x_ipsecrequest_mode = mode;
		rq->sadb_x_ipsecrequest_level = IPSEC_LEVEL_REQUIRE;
		if (t->reqid)
			rq->sadb_x_ipsecrequest_level = IPSEC_LEVEL_UNIQUE;
		if (t->optional)
			rq->sadb_x_ipsecrequest_level = IPSEC_LEVEL_USE;
		rq->sadb_x_ipsecrequest_reqid = t->reqid;

		if (t->mode == XFRM_MODE_TUNNEL) {
			u8 *sa = (void *)(rq + 1);
			pfkey_sockaddr_fill(&t->saddr, 0,
					    (struct sockaddr *)sa,
					    t->encap_family);
			pfkey_sockaddr_fill(&t->id.daddr, 0,
					    (struct sockaddr *) (sa + socklen),
					    t->encap_family);
		}
	}

	/* security context */
	if ((xfrm_ctx = xp->security)) {
		int ctx_size = pfkey_xfrm_policy2sec_ctx_size(xp);

		sec_ctx = (struct sadb_x_sec_ctx *) skb_put(skb, ctx_size);
		sec_ctx->sadb_x_sec_len = ctx_size / sizeof(uint64_t);
		sec_ctx->sadb_x_sec_exttype = SADB_X_EXT_SEC_CTX;
		sec_ctx->sadb_x_ctx_doi = xfrm_ctx->ctx_doi;
		sec_ctx->sadb_x_ctx_alg = xfrm_ctx->ctx_alg;
		sec_ctx->sadb_x_ctx_len = xfrm_ctx->ctx_len;
		memcpy(sec_ctx + 1, xfrm_ctx->ctx_str,
		       xfrm_ctx->ctx_len);
	}

	hdr->sadb_msg_len = size / sizeof(uint64_t);
	hdr->sadb_msg_reserved = atomic_read(&xp->refcnt);

	return 0;
}

static int key_notify_policy(struct xfrm_policy *xp, int dir, const struct km_event *c)
{
	struct sk_buff *out_skb;
	struct sadb_msg *out_hdr;
	int err;

	out_skb = pfkey_xfrm_policy2msg_prep(xp);
	if (IS_ERR(out_skb))
		return PTR_ERR(out_skb);

	err = pfkey_xfrm_policy2msg(out_skb, xp, dir);
	if (err < 0)
		return err;

	out_hdr = (struct sadb_msg *) out_skb->data;
	out_hdr->sadb_msg_version = PF_KEY_V2;

	if (c->data.byid && c->event == XFRM_MSG_DELPOLICY)
		out_hdr->sadb_msg_type = SADB_X_SPDDELETE2;
	else
		out_hdr->sadb_msg_type = event2poltype(c->event);
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_seq = c->seq;
	out_hdr->sadb_msg_pid = c->pid;
	pfkey_broadcast(out_skb, GFP_ATOMIC, BROADCAST_ALL, NULL, xp_net(xp));
	return 0;

}

static int pfkey_spdadd(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	int err = 0;
	struct sadb_lifetime *lifetime;
	struct sadb_address *sa;
	struct sadb_x_policy *pol;
	struct xfrm_policy *xp;
	struct km_event c;
	struct sadb_x_sec_ctx *sec_ctx;

	if (!present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST-1]) ||
	    !ext_hdrs[SADB_X_EXT_POLICY-1])
		return -EINVAL;

	pol = ext_hdrs[SADB_X_EXT_POLICY-1];
	if (pol->sadb_x_policy_type > IPSEC_POLICY_IPSEC)
		return -EINVAL;
	if (!pol->sadb_x_policy_dir || pol->sadb_x_policy_dir >= IPSEC_DIR_MAX)
		return -EINVAL;

	xp = xfrm_policy_alloc(net, GFP_KERNEL);
	if (xp == NULL)
		return -ENOBUFS;

	xp->action = (pol->sadb_x_policy_type == IPSEC_POLICY_DISCARD ?
		      XFRM_POLICY_BLOCK : XFRM_POLICY_ALLOW);
	xp->priority = pol->sadb_x_policy_priority;

	sa = ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
	xp->family = pfkey_sadb_addr2xfrm_addr(sa, &xp->selector.saddr);
	if (!xp->family) {
		err = -EINVAL;
		goto out;
	}
	xp->selector.family = xp->family;
	xp->selector.prefixlen_s = sa->sadb_address_prefixlen;
	xp->selector.proto = pfkey_proto_to_xfrm(sa->sadb_address_proto);
	xp->selector.sport = ((struct sockaddr_in *)(sa+1))->sin_port;
	if (xp->selector.sport)
		xp->selector.sport_mask = htons(0xffff);

	sa = ext_hdrs[SADB_EXT_ADDRESS_DST-1],
	pfkey_sadb_addr2xfrm_addr(sa, &xp->selector.daddr);
	xp->selector.prefixlen_d = sa->sadb_address_prefixlen;

	/* Amusing, we set this twice.  KAME apps appear to set same value
	 * in both addresses.
	 */
	xp->selector.proto = pfkey_proto_to_xfrm(sa->sadb_address_proto);

	xp->selector.dport = ((struct sockaddr_in *)(sa+1))->sin_port;
	if (xp->selector.dport)
		xp->selector.dport_mask = htons(0xffff);

	sec_ctx = ext_hdrs[SADB_X_EXT_SEC_CTX - 1];
	if (sec_ctx != NULL) {
		struct xfrm_user_sec_ctx *uctx = pfkey_sadb2xfrm_user_sec_ctx(sec_ctx);

		if (!uctx) {
			err = -ENOBUFS;
			goto out;
		}

		err = security_xfrm_policy_alloc(&xp->security, uctx);
		kfree(uctx);

		if (err)
			goto out;
	}

	xp->lft.soft_byte_limit = XFRM_INF;
	xp->lft.hard_byte_limit = XFRM_INF;
	xp->lft.soft_packet_limit = XFRM_INF;
	xp->lft.hard_packet_limit = XFRM_INF;
	if ((lifetime = ext_hdrs[SADB_EXT_LIFETIME_HARD-1]) != NULL) {
		xp->lft.hard_packet_limit = _KEY2X(lifetime->sadb_lifetime_allocations);
		xp->lft.hard_byte_limit = _KEY2X(lifetime->sadb_lifetime_bytes);
		xp->lft.hard_add_expires_seconds = lifetime->sadb_lifetime_addtime;
		xp->lft.hard_use_expires_seconds = lifetime->sadb_lifetime_usetime;
	}
	if ((lifetime = ext_hdrs[SADB_EXT_LIFETIME_SOFT-1]) != NULL) {
		xp->lft.soft_packet_limit = _KEY2X(lifetime->sadb_lifetime_allocations);
		xp->lft.soft_byte_limit = _KEY2X(lifetime->sadb_lifetime_bytes);
		xp->lft.soft_add_expires_seconds = lifetime->sadb_lifetime_addtime;
		xp->lft.soft_use_expires_seconds = lifetime->sadb_lifetime_usetime;
	}
	xp->xfrm_nr = 0;
	if (pol->sadb_x_policy_type == IPSEC_POLICY_IPSEC &&
	    (err = parse_ipsecrequests(xp, pol)) < 0)
		goto out;

	err = xfrm_policy_insert(pol->sadb_x_policy_dir-1, xp,
				 hdr->sadb_msg_type != SADB_X_SPDUPDATE);

	xfrm_audit_policy_add(xp, err ? 0 : 1,
			      audit_get_loginuid(current),
			      audit_get_sessionid(current), 0);

	if (err)
		goto out;

	if (hdr->sadb_msg_type == SADB_X_SPDUPDATE)
		c.event = XFRM_MSG_UPDPOLICY;
	else
		c.event = XFRM_MSG_NEWPOLICY;

	c.seq = hdr->sadb_msg_seq;
	c.pid = hdr->sadb_msg_pid;

	km_policy_notify(xp, pol->sadb_x_policy_dir-1, &c);
	xfrm_pol_put(xp);
	return 0;

out:
	xp->walk.dead = 1;
	xfrm_policy_destroy(xp);
	return err;
}

static int pfkey_spddelete(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	int err;
	struct sadb_address *sa;
	struct sadb_x_policy *pol;
	struct xfrm_policy *xp;
	struct xfrm_selector sel;
	struct km_event c;
	struct sadb_x_sec_ctx *sec_ctx;
	struct xfrm_sec_ctx *pol_ctx = NULL;

	if (!present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST-1]) ||
	    !ext_hdrs[SADB_X_EXT_POLICY-1])
		return -EINVAL;

	pol = ext_hdrs[SADB_X_EXT_POLICY-1];
	if (!pol->sadb_x_policy_dir || pol->sadb_x_policy_dir >= IPSEC_DIR_MAX)
		return -EINVAL;

	memset(&sel, 0, sizeof(sel));

	sa = ext_hdrs[SADB_EXT_ADDRESS_SRC-1],
	sel.family = pfkey_sadb_addr2xfrm_addr(sa, &sel.saddr);
	sel.prefixlen_s = sa->sadb_address_prefixlen;
	sel.proto = pfkey_proto_to_xfrm(sa->sadb_address_proto);
	sel.sport = ((struct sockaddr_in *)(sa+1))->sin_port;
	if (sel.sport)
		sel.sport_mask = htons(0xffff);

	sa = ext_hdrs[SADB_EXT_ADDRESS_DST-1],
	pfkey_sadb_addr2xfrm_addr(sa, &sel.daddr);
	sel.prefixlen_d = sa->sadb_address_prefixlen;
	sel.proto = pfkey_proto_to_xfrm(sa->sadb_address_proto);
	sel.dport = ((struct sockaddr_in *)(sa+1))->sin_port;
	if (sel.dport)
		sel.dport_mask = htons(0xffff);

	sec_ctx = ext_hdrs[SADB_X_EXT_SEC_CTX - 1];
	if (sec_ctx != NULL) {
		struct xfrm_user_sec_ctx *uctx = pfkey_sadb2xfrm_user_sec_ctx(sec_ctx);

		if (!uctx)
			return -ENOMEM;

		err = security_xfrm_policy_alloc(&pol_ctx, uctx);
		kfree(uctx);
		if (err)
			return err;
	}

	xp = xfrm_policy_bysel_ctx(net, DUMMY_MARK, XFRM_POLICY_TYPE_MAIN,
				   pol->sadb_x_policy_dir - 1, &sel, pol_ctx,
				   1, &err);
	security_xfrm_policy_free(pol_ctx);
	if (xp == NULL)
		return -ENOENT;

	xfrm_audit_policy_delete(xp, err ? 0 : 1,
				 audit_get_loginuid(current),
				 audit_get_sessionid(current), 0);

	if (err)
		goto out;

	c.seq = hdr->sadb_msg_seq;
	c.pid = hdr->sadb_msg_pid;
	c.data.byid = 0;
	c.event = XFRM_MSG_DELPOLICY;
	km_policy_notify(xp, pol->sadb_x_policy_dir-1, &c);

out:
	xfrm_pol_put(xp);
	return err;
}

static int key_pol_get_resp(struct sock *sk, struct xfrm_policy *xp, const struct sadb_msg *hdr, int dir)
{
	int err;
	struct sk_buff *out_skb;
	struct sadb_msg *out_hdr;
	err = 0;

	out_skb = pfkey_xfrm_policy2msg_prep(xp);
	if (IS_ERR(out_skb)) {
		err =  PTR_ERR(out_skb);
		goto out;
	}
	err = pfkey_xfrm_policy2msg(out_skb, xp, dir);
	if (err < 0)
		goto out;

	out_hdr = (struct sadb_msg *) out_skb->data;
	out_hdr->sadb_msg_version = hdr->sadb_msg_version;
	out_hdr->sadb_msg_type = hdr->sadb_msg_type;
	out_hdr->sadb_msg_satype = 0;
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_seq = hdr->sadb_msg_seq;
	out_hdr->sadb_msg_pid = hdr->sadb_msg_pid;
	pfkey_broadcast(out_skb, GFP_ATOMIC, BROADCAST_ONE, sk, xp_net(xp));
	err = 0;

out:
	return err;
}

#ifdef CONFIG_NET_KEY_MIGRATE
static int pfkey_sockaddr_pair_size(sa_family_t family)
{
	return PFKEY_ALIGN8(pfkey_sockaddr_len(family) * 2);
}

static int parse_sockaddr_pair(struct sockaddr *sa, int ext_len,
			       xfrm_address_t *saddr, xfrm_address_t *daddr,
			       u16 *family)
{
	int af, socklen;

	if (ext_len < pfkey_sockaddr_pair_size(sa->sa_family))
		return -EINVAL;

	af = pfkey_sockaddr_extract(sa, saddr);
	if (!af)
		return -EINVAL;

	socklen = pfkey_sockaddr_len(af);
	if (pfkey_sockaddr_extract((struct sockaddr *) (((u8 *)sa) + socklen),
				   daddr) != af)
		return -EINVAL;

	*family = af;
	return 0;
}

static int ipsecrequests_to_migrate(struct sadb_x_ipsecrequest *rq1, int len,
				    struct xfrm_migrate *m)
{
	int err;
	struct sadb_x_ipsecrequest *rq2;
	int mode;

	if (len <= sizeof(struct sadb_x_ipsecrequest) ||
	    len < rq1->sadb_x_ipsecrequest_len)
		return -EINVAL;

	/* old endoints */
	err = parse_sockaddr_pair((struct sockaddr *)(rq1 + 1),
				  rq1->sadb_x_ipsecrequest_len,
				  &m->old_saddr, &m->old_daddr,
				  &m->old_family);
	if (err)
		return err;

	rq2 = (struct sadb_x_ipsecrequest *)((u8 *)rq1 + rq1->sadb_x_ipsecrequest_len);
	len -= rq1->sadb_x_ipsecrequest_len;

	if (len <= sizeof(struct sadb_x_ipsecrequest) ||
	    len < rq2->sadb_x_ipsecrequest_len)
		return -EINVAL;

	/* new endpoints */
	err = parse_sockaddr_pair((struct sockaddr *)(rq2 + 1),
				  rq2->sadb_x_ipsecrequest_len,
				  &m->new_saddr, &m->new_daddr,
				  &m->new_family);
	if (err)
		return err;

	if (rq1->sadb_x_ipsecrequest_proto != rq2->sadb_x_ipsecrequest_proto ||
	    rq1->sadb_x_ipsecrequest_mode != rq2->sadb_x_ipsecrequest_mode ||
	    rq1->sadb_x_ipsecrequest_reqid != rq2->sadb_x_ipsecrequest_reqid)
		return -EINVAL;

	m->proto = rq1->sadb_x_ipsecrequest_proto;
	if ((mode = pfkey_mode_to_xfrm(rq1->sadb_x_ipsecrequest_mode)) < 0)
		return -EINVAL;
	m->mode = mode;
	m->reqid = rq1->sadb_x_ipsecrequest_reqid;

	return ((int)(rq1->sadb_x_ipsecrequest_len +
		      rq2->sadb_x_ipsecrequest_len));
}

static int pfkey_migrate(struct sock *sk, struct sk_buff *skb,
			 const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	int i, len, ret, err = -EINVAL;
	u8 dir;
	struct sadb_address *sa;
	struct sadb_x_kmaddress *kma;
	struct sadb_x_policy *pol;
	struct sadb_x_ipsecrequest *rq;
	struct xfrm_selector sel;
	struct xfrm_migrate m[XFRM_MAX_DEPTH];
	struct xfrm_kmaddress k;

	if (!present_and_same_family(ext_hdrs[SADB_EXT_ADDRESS_SRC - 1],
				     ext_hdrs[SADB_EXT_ADDRESS_DST - 1]) ||
	    !ext_hdrs[SADB_X_EXT_POLICY - 1]) {
		err = -EINVAL;
		goto out;
	}

	kma = ext_hdrs[SADB_X_EXT_KMADDRESS - 1];
	pol = ext_hdrs[SADB_X_EXT_POLICY - 1];

	if (pol->sadb_x_policy_dir >= IPSEC_DIR_MAX) {
		err = -EINVAL;
		goto out;
	}

	if (kma) {
		/* convert sadb_x_kmaddress to xfrm_kmaddress */
		k.reserved = kma->sadb_x_kmaddress_reserved;
		ret = parse_sockaddr_pair((struct sockaddr *)(kma + 1),
					  8*(kma->sadb_x_kmaddress_len) - sizeof(*kma),
					  &k.local, &k.remote, &k.family);
		if (ret < 0) {
			err = ret;
			goto out;
		}
	}

	dir = pol->sadb_x_policy_dir - 1;
	memset(&sel, 0, sizeof(sel));

	/* set source address info of selector */
	sa = ext_hdrs[SADB_EXT_ADDRESS_SRC - 1];
	sel.family = pfkey_sadb_addr2xfrm_addr(sa, &sel.saddr);
	sel.prefixlen_s = sa->sadb_address_prefixlen;
	sel.proto = pfkey_proto_to_xfrm(sa->sadb_address_proto);
	sel.sport = ((struct sockaddr_in *)(sa + 1))->sin_port;
	if (sel.sport)
		sel.sport_mask = htons(0xffff);

	/* set destination address info of selector */
	sa = ext_hdrs[SADB_EXT_ADDRESS_DST - 1],
	pfkey_sadb_addr2xfrm_addr(sa, &sel.daddr);
	sel.prefixlen_d = sa->sadb_address_prefixlen;
	sel.proto = pfkey_proto_to_xfrm(sa->sadb_address_proto);
	sel.dport = ((struct sockaddr_in *)(sa + 1))->sin_port;
	if (sel.dport)
		sel.dport_mask = htons(0xffff);

	rq = (struct sadb_x_ipsecrequest *)(pol + 1);

	/* extract ipsecrequests */
	i = 0;
	len = pol->sadb_x_policy_len * 8 - sizeof(struct sadb_x_policy);

	while (len > 0 && i < XFRM_MAX_DEPTH) {
		ret = ipsecrequests_to_migrate(rq, len, &m[i]);
		if (ret < 0) {
			err = ret;
			goto out;
		} else {
			rq = (struct sadb_x_ipsecrequest *)((u8 *)rq + ret);
			len -= ret;
			i++;
		}
	}

	if (!i || len > 0) {
		err = -EINVAL;
		goto out;
	}

	return xfrm_migrate(&sel, dir, XFRM_POLICY_TYPE_MAIN, m, i,
			    kma ? &k : NULL);

 out:
	return err;
}
#else
static int pfkey_migrate(struct sock *sk, struct sk_buff *skb,
			 const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	return -ENOPROTOOPT;
}
#endif


static int pfkey_spdget(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	unsigned int dir;
	int err = 0, delete;
	struct sadb_x_policy *pol;
	struct xfrm_policy *xp;
	struct km_event c;

	if ((pol = ext_hdrs[SADB_X_EXT_POLICY-1]) == NULL)
		return -EINVAL;

	dir = xfrm_policy_id2dir(pol->sadb_x_policy_id);
	if (dir >= XFRM_POLICY_MAX)
		return -EINVAL;

	delete = (hdr->sadb_msg_type == SADB_X_SPDDELETE2);
	xp = xfrm_policy_byid(net, DUMMY_MARK, XFRM_POLICY_TYPE_MAIN,
			      dir, pol->sadb_x_policy_id, delete, &err);
	if (xp == NULL)
		return -ENOENT;

	if (delete) {
		xfrm_audit_policy_delete(xp, err ? 0 : 1,
				audit_get_loginuid(current),
				audit_get_sessionid(current), 0);

		if (err)
			goto out;
		c.seq = hdr->sadb_msg_seq;
		c.pid = hdr->sadb_msg_pid;
		c.data.byid = 1;
		c.event = XFRM_MSG_DELPOLICY;
		km_policy_notify(xp, dir, &c);
	} else {
		err = key_pol_get_resp(sk, xp, hdr, dir);
	}

out:
	xfrm_pol_put(xp);
	return err;
}

static int dump_sp(struct xfrm_policy *xp, int dir, int count, void *ptr)
{
	struct pfkey_sock *pfk = ptr;
	struct sk_buff *out_skb;
	struct sadb_msg *out_hdr;
	int err;

	if (!pfkey_can_dump(&pfk->sk))
		return -ENOBUFS;

	out_skb = pfkey_xfrm_policy2msg_prep(xp);
	if (IS_ERR(out_skb))
		return PTR_ERR(out_skb);

	err = pfkey_xfrm_policy2msg(out_skb, xp, dir);
	if (err < 0)
		return err;

	out_hdr = (struct sadb_msg *) out_skb->data;
	out_hdr->sadb_msg_version = pfk->dump.msg_version;
	out_hdr->sadb_msg_type = SADB_X_SPDDUMP;
	out_hdr->sadb_msg_satype = SADB_SATYPE_UNSPEC;
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_seq = count + 1;
	out_hdr->sadb_msg_pid = pfk->dump.msg_pid;

	if (pfk->dump.skb)
		pfkey_broadcast(pfk->dump.skb, GFP_ATOMIC, BROADCAST_ONE,
				&pfk->sk, sock_net(&pfk->sk));
	pfk->dump.skb = out_skb;

	return 0;
}

static int pfkey_dump_sp(struct pfkey_sock *pfk)
{
	struct net *net = sock_net(&pfk->sk);
	return xfrm_policy_walk(net, &pfk->dump.u.policy, dump_sp, (void *) pfk);
}

static void pfkey_dump_sp_done(struct pfkey_sock *pfk)
{
	xfrm_policy_walk_done(&pfk->dump.u.policy);
}

static int pfkey_spddump(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct pfkey_sock *pfk = pfkey_sk(sk);

	if (pfk->dump.dump != NULL)
		return -EBUSY;

	pfk->dump.msg_version = hdr->sadb_msg_version;
	pfk->dump.msg_pid = hdr->sadb_msg_pid;
	pfk->dump.dump = pfkey_dump_sp;
	pfk->dump.done = pfkey_dump_sp_done;
	xfrm_policy_walk_init(&pfk->dump.u.policy, XFRM_POLICY_TYPE_MAIN);

	return pfkey_do_dump(pfk);
}

static int key_notify_policy_flush(const struct km_event *c)
{
	struct sk_buff *skb_out;
	struct sadb_msg *hdr;

	skb_out = alloc_skb(sizeof(struct sadb_msg) + 16, GFP_ATOMIC);
	if (!skb_out)
		return -ENOBUFS;
	hdr = (struct sadb_msg *) skb_put(skb_out, sizeof(struct sadb_msg));
	hdr->sadb_msg_type = SADB_X_SPDFLUSH;
	hdr->sadb_msg_seq = c->seq;
	hdr->sadb_msg_pid = c->pid;
	hdr->sadb_msg_version = PF_KEY_V2;
	hdr->sadb_msg_errno = (uint8_t) 0;
	hdr->sadb_msg_satype = SADB_SATYPE_UNSPEC;
	hdr->sadb_msg_len = (sizeof(struct sadb_msg) / sizeof(uint64_t));
	hdr->sadb_msg_reserved = 0;
	pfkey_broadcast(skb_out, GFP_ATOMIC, BROADCAST_ALL, NULL, c->net);
	return 0;

}

static int pfkey_spdflush(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr, void * const *ext_hdrs)
{
	struct net *net = sock_net(sk);
	struct km_event c;
	struct xfrm_audit audit_info;
	int err, err2;

	audit_info.loginuid = audit_get_loginuid(current);
	audit_info.sessionid = audit_get_sessionid(current);
	audit_info.secid = 0;
	err = xfrm_policy_flush(net, XFRM_POLICY_TYPE_MAIN, &audit_info);
	err2 = unicast_flush_resp(sk, hdr);
	if (err || err2) {
		if (err == -ESRCH) /* empty table - old silent behavior */
			return 0;
		return err;
	}

	c.data.type = XFRM_POLICY_TYPE_MAIN;
	c.event = XFRM_MSG_FLUSHPOLICY;
	c.pid = hdr->sadb_msg_pid;
	c.seq = hdr->sadb_msg_seq;
	c.net = net;
	km_policy_notify(NULL, 0, &c);

	return 0;
}

typedef int (*pfkey_handler)(struct sock *sk, struct sk_buff *skb,
			     const struct sadb_msg *hdr, void * const *ext_hdrs);
static pfkey_handler pfkey_funcs[SADB_MAX + 1] = {
	[SADB_RESERVED]		= pfkey_reserved,
	[SADB_GETSPI]		= pfkey_getspi,
	[SADB_UPDATE]		= pfkey_add,
	[SADB_ADD]		= pfkey_add,
	[SADB_DELETE]		= pfkey_delete,
	[SADB_GET]		= pfkey_get,
	[SADB_ACQUIRE]		= pfkey_acquire,
	[SADB_REGISTER]		= pfkey_register,
	[SADB_EXPIRE]		= NULL,
	[SADB_FLUSH]		= pfkey_flush,
	[SADB_DUMP]		= pfkey_dump,
	[SADB_X_PROMISC]	= pfkey_promisc,
	[SADB_X_PCHANGE]	= NULL,
	[SADB_X_SPDUPDATE]	= pfkey_spdadd,
	[SADB_X_SPDADD]		= pfkey_spdadd,
	[SADB_X_SPDDELETE]	= pfkey_spddelete,
	[SADB_X_SPDGET]		= pfkey_spdget,
	[SADB_X_SPDACQUIRE]	= NULL,
	[SADB_X_SPDDUMP]	= pfkey_spddump,
	[SADB_X_SPDFLUSH]	= pfkey_spdflush,
	[SADB_X_SPDSETIDX]	= pfkey_spdadd,
	[SADB_X_SPDDELETE2]	= pfkey_spdget,
	[SADB_X_MIGRATE]	= pfkey_migrate,
};

static int pfkey_process(struct sock *sk, struct sk_buff *skb, const struct sadb_msg *hdr)
{
	void *ext_hdrs[SADB_EXT_MAX];
	int err;

	pfkey_broadcast(skb_clone(skb, GFP_KERNEL), GFP_KERNEL,
			BROADCAST_PROMISC_ONLY, NULL, sock_net(sk));

	memset(ext_hdrs, 0, sizeof(ext_hdrs));
	err = parse_exthdrs(skb, hdr, ext_hdrs);
	if (!err) {
		err = -EOPNOTSUPP;
		if (pfkey_funcs[hdr->sadb_msg_type])
			err = pfkey_funcs[hdr->sadb_msg_type](sk, skb, hdr, ext_hdrs);
	}
	return err;
}

static struct sadb_msg *pfkey_get_base_msg(struct sk_buff *skb, int *errp)
{
	struct sadb_msg *hdr = NULL;

	if (skb->len < sizeof(*hdr)) {
		*errp = -EMSGSIZE;
	} else {
		hdr = (struct sadb_msg *) skb->data;
		if (hdr->sadb_msg_version != PF_KEY_V2 ||
		    hdr->sadb_msg_reserved != 0 ||
		    (hdr->sadb_msg_type <= SADB_RESERVED ||
		     hdr->sadb_msg_type > SADB_MAX)) {
			hdr = NULL;
			*errp = -EINVAL;
		} else if (hdr->sadb_msg_len != (skb->len /
						 sizeof(uint64_t)) ||
			   hdr->sadb_msg_len < (sizeof(struct sadb_msg) /
						sizeof(uint64_t))) {
			hdr = NULL;
			*errp = -EMSGSIZE;
		} else {
			*errp = 0;
		}
	}
	return hdr;
}

static inline int aalg_tmpl_set(const struct xfrm_tmpl *t,
				const struct xfrm_algo_desc *d)
{
	unsigned int id = d->desc.sadb_alg_id;

	if (id >= sizeof(t->aalgos) * 8)
		return 0;

	return (t->aalgos >> id) & 1;
}

static inline int ealg_tmpl_set(const struct xfrm_tmpl *t,
				const struct xfrm_algo_desc *d)
{
	unsigned int id = d->desc.sadb_alg_id;

	if (id >= sizeof(t->ealgos) * 8)
		return 0;

	return (t->ealgos >> id) & 1;
}

static int count_ah_combs(const struct xfrm_tmpl *t)
{
	int i, sz = 0;

	for (i = 0; ; i++) {
		const struct xfrm_algo_desc *aalg = xfrm_aalg_get_byidx(i);
		if (!aalg)
			break;
		if (aalg_tmpl_set(t, aalg) && aalg->available)
			sz += sizeof(struct sadb_comb);
	}
	return sz + sizeof(struct sadb_prop);
}

static int count_esp_combs(const struct xfrm_tmpl *t)
{
	int i, k, sz = 0;

	for (i = 0; ; i++) {
		const struct xfrm_algo_desc *ealg = xfrm_ealg_get_byidx(i);
		if (!ealg)
			break;

		if (!(ealg_tmpl_set(t, ealg) && ealg->available))
			continue;

		for (k = 1; ; k++) {
			const struct xfrm_algo_desc *aalg = xfrm_aalg_get_byidx(k);
			if (!aalg)
				break;

			if (aalg_tmpl_set(t, aalg) && aalg->available)
				sz += sizeof(struct sadb_comb);
		}
	}
	return sz + sizeof(struct sadb_prop);
}

static void dump_ah_combs(struct sk_buff *skb, const struct xfrm_tmpl *t)
{
	struct sadb_prop *p;
	int i;

	p = (struct sadb_prop*)skb_put(skb, sizeof(struct sadb_prop));
	p->sadb_prop_len = sizeof(struct sadb_prop)/8;
	p->sadb_prop_exttype = SADB_EXT_PROPOSAL;
	p->sadb_prop_replay = 32;
	memset(p->sadb_prop_reserved, 0, sizeof(p->sadb_prop_reserved));

	for (i = 0; ; i++) {
		const struct xfrm_algo_desc *aalg = xfrm_aalg_get_byidx(i);
		if (!aalg)
			break;

		if (aalg_tmpl_set(t, aalg) && aalg->available) {
			struct sadb_comb *c;
			c = (struct sadb_comb*)skb_put(skb, sizeof(struct sadb_comb));
			memset(c, 0, sizeof(*c));
			p->sadb_prop_len += sizeof(struct sadb_comb)/8;
			c->sadb_comb_auth = aalg->desc.sadb_alg_id;
			c->sadb_comb_auth_minbits = aalg->desc.sadb_alg_minbits;
			c->sadb_comb_auth_maxbits = aalg->desc.sadb_alg_maxbits;
			c->sadb_comb_hard_addtime = 24*60*60;
			c->sadb_comb_soft_addtime = 20*60*60;
			c->sadb_comb_hard_usetime = 8*60*60;
			c->sadb_comb_soft_usetime = 7*60*60;
		}
	}
}

static void dump_esp_combs(struct sk_buff *skb, const struct xfrm_tmpl *t)
{
	struct sadb_prop *p;
	int i, k;

	p = (struct sadb_prop*)skb_put(skb, sizeof(struct sadb_prop));
	p->sadb_prop_len = sizeof(struct sadb_prop)/8;
	p->sadb_prop_exttype = SADB_EXT_PROPOSAL;
	p->sadb_prop_replay = 32;
	memset(p->sadb_prop_reserved, 0, sizeof(p->sadb_prop_reserved));

	for (i=0; ; i++) {
		const struct xfrm_algo_desc *ealg = xfrm_ealg_get_byidx(i);
		if (!ealg)
			break;

		if (!(ealg_tmpl_set(t, ealg) && ealg->available))
			continue;

		for (k = 1; ; k++) {
			struct sadb_comb *c;
			const struct xfrm_algo_desc *aalg = xfrm_aalg_get_byidx(k);
			if (!aalg)
				break;
			if (!(aalg_tmpl_set(t, aalg) && aalg->available))
				continue;
			c = (struct sadb_comb*)skb_put(skb, sizeof(struct sadb_comb));
			memset(c, 0, sizeof(*c));
			p->sadb_prop_len += sizeof(struct sadb_comb)/8;
			c->sadb_comb_auth = aalg->desc.sadb_alg_id;
			c->sadb_comb_auth_minbits = aalg->desc.sadb_alg_minbits;
			c->sadb_comb_auth_maxbits = aalg->desc.sadb_alg_maxbits;
			c->sadb_comb_encrypt = ealg->desc.sadb_alg_id;
			c->sadb_comb_encrypt_minbits = ealg->desc.sadb_alg_minbits;
			c->sadb_comb_encrypt_maxbits = ealg->desc.sadb_alg_maxbits;
			c->sadb_comb_hard_addtime = 24*60*60;
			c->sadb_comb_soft_addtime = 20*60*60;
			c->sadb_comb_hard_usetime = 8*60*60;
			c->sadb_comb_soft_usetime = 7*60*60;
		}
	}
}

static int key_notify_policy_expire(struct xfrm_policy *xp, const struct km_event *c)
{
	return 0;
}

static int key_notify_sa_expire(struct xfrm_state *x, const struct km_event *c)
{
	struct sk_buff *out_skb;
	struct sadb_msg *out_hdr;
	int hard;
	int hsc;

	hard = c->data.hard;
	if (hard)
		hsc = 2;
	else
		hsc = 1;

	out_skb = pfkey_xfrm_state2msg_expire(x, hsc);
	if (IS_ERR(out_skb))
		return PTR_ERR(out_skb);

	out_hdr = (struct sadb_msg *) out_skb->data;
	out_hdr->sadb_msg_version = PF_KEY_V2;
	out_hdr->sadb_msg_type = SADB_EXPIRE;
	out_hdr->sadb_msg_satype = pfkey_proto2satype(x->id.proto);
	out_hdr->sadb_msg_errno = 0;
	out_hdr->sadb_msg_reserved = 0;
	out_hdr->sadb_msg_seq = 0;
	out_hdr->sadb_msg_pid = 0;

	pfkey_broadcast(out_skb, GFP_ATOMIC, BROADCAST_REGISTERED, NULL, xs_net(x));

	return 0;
}

static int pfkey_send_notify(struct xfrm_state *x, const struct km_event *c)
{
	struct net *net = x ? xs_net(x) : c->net;
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);


#ifdef NLKEY_SUPPORT
	/* send message to the user space through NETLINK_KEY socket*/
	ipsec_nlkey_send(net, x, c);
#endif

	if (atomic_read(&net_pfkey->socks_nr) == 0)
		return 0;

	switch (c->event) {
	case XFRM_MSG_EXPIRE:
		return key_notify_sa_expire(x, c);
	case XFRM_MSG_DELSA:
	case XFRM_MSG_NEWSA:
	case XFRM_MSG_UPDSA:
		return key_notify_sa(x, c);
	case XFRM_MSG_FLUSHSA:
		return key_notify_sa_flush(c);
	case XFRM_MSG_NEWAE: /* not yet supported */
		break;
	default:
		pr_err("pfkey: Unknown SA event %d\n", c->event);
		break;
	}

	return 0;
}

static int pfkey_send_policy_notify(struct xfrm_policy *xp, int dir, const struct km_event *c)
{
	if (xp && xp->type != XFRM_POLICY_TYPE_MAIN)
		return 0;

	switch (c->event) {
	case XFRM_MSG_POLEXPIRE:
		return key_notify_policy_expire(xp, c);
	case XFRM_MSG_DELPOLICY:
	case XFRM_MSG_NEWPOLICY:
	case XFRM_MSG_UPDPOLICY:
		return key_notify_policy(xp, dir, c);
	case XFRM_MSG_FLUSHPOLICY:
		if (c->data.type != XFRM_POLICY_TYPE_MAIN)
			break;
		return key_notify_policy_flush(c);
	default:
		pr_err("pfkey: Unknown policy event %d\n", c->event);
		break;
	}

	return 0;
}

static u32 get_acqseq(void)
{
	u32 res;
	static atomic_t acqseq;

	do {
		res = atomic_inc_return(&acqseq);
	} while (!res);
	return res;
}

static int pfkey_send_acquire(struct xfrm_state *x, struct xfrm_tmpl *t, struct xfrm_policy *xp, int dir)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;
	struct sadb_address *addr;
	struct sadb_x_policy *pol;
	int sockaddr_size;
	int size;
	struct sadb_x_sec_ctx *sec_ctx;
	struct xfrm_sec_ctx *xfrm_ctx;
	int ctx_size = 0;

	sockaddr_size = pfkey_sockaddr_size(x->props.family);
	if (!sockaddr_size)
		return -EINVAL;

	size = sizeof(struct sadb_msg) +
		(sizeof(struct sadb_address) * 2) +
		(sockaddr_size * 2) +
		sizeof(struct sadb_x_policy);

	if (x->id.proto == IPPROTO_AH)
		size += count_ah_combs(t);
	else if (x->id.proto == IPPROTO_ESP)
		size += count_esp_combs(t);

	if ((xfrm_ctx = x->security)) {
		ctx_size = PFKEY_ALIGN8(xfrm_ctx->ctx_len);
		size +=  sizeof(struct sadb_x_sec_ctx) + ctx_size;
	}

	skb =  alloc_skb(size + 16, GFP_ATOMIC);
	if (skb == NULL)
		return -ENOMEM;

	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	hdr->sadb_msg_version = PF_KEY_V2;
	hdr->sadb_msg_type = SADB_ACQUIRE;
	hdr->sadb_msg_satype = pfkey_proto2satype(x->id.proto);
	hdr->sadb_msg_len = size / sizeof(uint64_t);
	hdr->sadb_msg_errno = 0;
	hdr->sadb_msg_reserved = 0;
	hdr->sadb_msg_seq = x->km.seq = get_acqseq();
	hdr->sadb_msg_pid = 0;

	/* src address */
	addr = (struct sadb_address*) skb_put(skb,
					      sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
	addr->sadb_address_proto = 0;
	addr->sadb_address_reserved = 0;
	addr->sadb_address_prefixlen =
		pfkey_sockaddr_fill(&x->props.saddr, 0,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	if (!addr->sadb_address_prefixlen)
		BUG();

	/* dst address */
	addr = (struct sadb_address*) skb_put(skb,
					      sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
	addr->sadb_address_proto = 0;
	addr->sadb_address_reserved = 0;
	addr->sadb_address_prefixlen =
		pfkey_sockaddr_fill(&x->id.daddr, 0,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	if (!addr->sadb_address_prefixlen)
		BUG();

	pol = (struct sadb_x_policy *)  skb_put(skb, sizeof(struct sadb_x_policy));
	pol->sadb_x_policy_len = sizeof(struct sadb_x_policy)/sizeof(uint64_t);
	pol->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	pol->sadb_x_policy_type = IPSEC_POLICY_IPSEC;
	pol->sadb_x_policy_dir = dir+1;
	pol->sadb_x_policy_reserved = 0;
	pol->sadb_x_policy_id = xp->index;
	pol->sadb_x_policy_priority = xp->priority;

	/* Set sadb_comb's. */
	if (x->id.proto == IPPROTO_AH)
		dump_ah_combs(skb, t);
	else if (x->id.proto == IPPROTO_ESP)
		dump_esp_combs(skb, t);

	/* security context */
	if (xfrm_ctx) {
		sec_ctx = (struct sadb_x_sec_ctx *) skb_put(skb,
				sizeof(struct sadb_x_sec_ctx) + ctx_size);
		sec_ctx->sadb_x_sec_len =
		  (sizeof(struct sadb_x_sec_ctx) + ctx_size) / sizeof(uint64_t);
		sec_ctx->sadb_x_sec_exttype = SADB_X_EXT_SEC_CTX;
		sec_ctx->sadb_x_ctx_doi = xfrm_ctx->ctx_doi;
		sec_ctx->sadb_x_ctx_alg = xfrm_ctx->ctx_alg;
		sec_ctx->sadb_x_ctx_len = xfrm_ctx->ctx_len;
		memcpy(sec_ctx + 1, xfrm_ctx->ctx_str,
		       xfrm_ctx->ctx_len);
	}

	return pfkey_broadcast(skb, GFP_ATOMIC, BROADCAST_REGISTERED, NULL, xs_net(x));
}

static struct xfrm_policy *pfkey_compile_policy(struct sock *sk, int opt,
						u8 *data, int len, int *dir)
{
	struct net *net = sock_net(sk);
	struct xfrm_policy *xp;
	struct sadb_x_policy *pol = (struct sadb_x_policy*)data;
	struct sadb_x_sec_ctx *sec_ctx;

	switch (sk->sk_family) {
	case AF_INET:
		if (opt != IP_IPSEC_POLICY) {
			*dir = -EOPNOTSUPP;
			return NULL;
		}
		break;
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
	case AF_INET6:
		if (opt != IPV6_IPSEC_POLICY) {
			*dir = -EOPNOTSUPP;
			return NULL;
		}
		break;
#endif
	default:
		*dir = -EINVAL;
		return NULL;
	}

	*dir = -EINVAL;

	if (len < sizeof(struct sadb_x_policy) ||
	    pol->sadb_x_policy_len*8 > len ||
	    pol->sadb_x_policy_type > IPSEC_POLICY_BYPASS ||
	    (!pol->sadb_x_policy_dir || pol->sadb_x_policy_dir > IPSEC_DIR_OUTBOUND))
		return NULL;

	xp = xfrm_policy_alloc(net, GFP_ATOMIC);
	if (xp == NULL) {
		*dir = -ENOBUFS;
		return NULL;
	}

	xp->action = (pol->sadb_x_policy_type == IPSEC_POLICY_DISCARD ?
		      XFRM_POLICY_BLOCK : XFRM_POLICY_ALLOW);

	xp->lft.soft_byte_limit = XFRM_INF;
	xp->lft.hard_byte_limit = XFRM_INF;
	xp->lft.soft_packet_limit = XFRM_INF;
	xp->lft.hard_packet_limit = XFRM_INF;
	xp->family = sk->sk_family;

	xp->xfrm_nr = 0;
	if (pol->sadb_x_policy_type == IPSEC_POLICY_IPSEC &&
	    (*dir = parse_ipsecrequests(xp, pol)) < 0)
		goto out;

	/* security context too */
	if (len >= (pol->sadb_x_policy_len*8 +
	    sizeof(struct sadb_x_sec_ctx))) {
		char *p = (char *)pol;
		struct xfrm_user_sec_ctx *uctx;

		p += pol->sadb_x_policy_len*8;
		sec_ctx = (struct sadb_x_sec_ctx *)p;
		if (len < pol->sadb_x_policy_len*8 +
		    sec_ctx->sadb_x_sec_len) {
			*dir = -EINVAL;
			goto out;
		}
		if ((*dir = verify_sec_ctx_len(p)))
			goto out;
		uctx = pfkey_sadb2xfrm_user_sec_ctx(sec_ctx);
		*dir = security_xfrm_policy_alloc(&xp->security, uctx);
		kfree(uctx);

		if (*dir)
			goto out;
	}

	*dir = pol->sadb_x_policy_dir-1;
	return xp;

out:
	xp->walk.dead = 1;
	xfrm_policy_destroy(xp);
	return NULL;
}

static int pfkey_send_new_mapping(struct xfrm_state *x, xfrm_address_t *ipaddr, __be16 sport)
{
	struct sk_buff *skb;
	struct sadb_msg *hdr;
	struct sadb_sa *sa;
	struct sadb_address *addr;
	struct sadb_x_nat_t_port *n_port;
	int sockaddr_size;
	int size;
	__u8 satype = (x->id.proto == IPPROTO_ESP ? SADB_SATYPE_ESP : 0);
	struct xfrm_encap_tmpl *natt = NULL;

	sockaddr_size = pfkey_sockaddr_size(x->props.family);
	if (!sockaddr_size)
		return -EINVAL;

	if (!satype)
		return -EINVAL;

	if (!x->encap)
		return -EINVAL;

	natt = x->encap;

	/* Build an SADB_X_NAT_T_NEW_MAPPING message:
	 *
	 * HDR | SA | ADDRESS_SRC (old addr) | NAT_T_SPORT (old port) |
	 * ADDRESS_DST (new addr) | NAT_T_DPORT (new port)
	 */

	size = sizeof(struct sadb_msg) +
		sizeof(struct sadb_sa) +
		(sizeof(struct sadb_address) * 2) +
		(sockaddr_size * 2) +
		(sizeof(struct sadb_x_nat_t_port) * 2);

	skb =  alloc_skb(size + 16, GFP_ATOMIC);
	if (skb == NULL)
		return -ENOMEM;

	hdr = (struct sadb_msg *) skb_put(skb, sizeof(struct sadb_msg));
	hdr->sadb_msg_version = PF_KEY_V2;
	hdr->sadb_msg_type = SADB_X_NAT_T_NEW_MAPPING;
	hdr->sadb_msg_satype = satype;
	hdr->sadb_msg_len = size / sizeof(uint64_t);
	hdr->sadb_msg_errno = 0;
	hdr->sadb_msg_reserved = 0;
	hdr->sadb_msg_seq = x->km.seq = get_acqseq();
	hdr->sadb_msg_pid = 0;

	/* SA */
	sa = (struct sadb_sa *) skb_put(skb, sizeof(struct sadb_sa));
	sa->sadb_sa_len = sizeof(struct sadb_sa)/sizeof(uint64_t);
	sa->sadb_sa_exttype = SADB_EXT_SA;
	sa->sadb_sa_spi = x->id.spi;
	sa->sadb_sa_replay = 0;
	sa->sadb_sa_state = 0;
	sa->sadb_sa_auth = 0;
	sa->sadb_sa_encrypt = 0;
	sa->sadb_sa_flags = 0;

	/* ADDRESS_SRC (old addr) */
	addr = (struct sadb_address*)
		skb_put(skb, sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
	addr->sadb_address_proto = 0;
	addr->sadb_address_reserved = 0;
	addr->sadb_address_prefixlen =
		pfkey_sockaddr_fill(&x->props.saddr, 0,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	if (!addr->sadb_address_prefixlen)
		BUG();

	/* NAT_T_SPORT (old port) */
	n_port = (struct sadb_x_nat_t_port*) skb_put(skb, sizeof (*n_port));
	n_port->sadb_x_nat_t_port_len = sizeof(*n_port)/sizeof(uint64_t);
	n_port->sadb_x_nat_t_port_exttype = SADB_X_EXT_NAT_T_SPORT;
	n_port->sadb_x_nat_t_port_port = natt->encap_sport;
	n_port->sadb_x_nat_t_port_reserved = 0;

	/* ADDRESS_DST (new addr) */
	addr = (struct sadb_address*)
		skb_put(skb, sizeof(struct sadb_address)+sockaddr_size);
	addr->sadb_address_len =
		(sizeof(struct sadb_address)+sockaddr_size)/
			sizeof(uint64_t);
	addr->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
	addr->sadb_address_proto = 0;
	addr->sadb_address_reserved = 0;
	addr->sadb_address_prefixlen =
		pfkey_sockaddr_fill(ipaddr, 0,
				    (struct sockaddr *) (addr + 1),
				    x->props.family);
	if (!addr->sadb_address_prefixlen)
		BUG();

	/* NAT_T_DPORT (new port) */
	n_port = (struct sadb_x_nat_t_port*) skb_put(skb, sizeof (*n_port));
	n_port->sadb_x_nat_t_port_len = sizeof(*n_port)/sizeof(uint64_t);
	n_port->sadb_x_nat_t_port_exttype = SADB_X_EXT_NAT_T_DPORT;
	n_port->sadb_x_nat_t_port_port = sport;
	n_port->sadb_x_nat_t_port_reserved = 0;

	return pfkey_broadcast(skb, GFP_ATOMIC, BROADCAST_REGISTERED, NULL, xs_net(x));
}

#ifdef CONFIG_NET_KEY_MIGRATE
static int set_sadb_address(struct sk_buff *skb, int sasize, int type,
			    const struct xfrm_selector *sel)
{
	struct sadb_address *addr;
	addr = (struct sadb_address *)skb_put(skb, sizeof(struct sadb_address) + sasize);
	addr->sadb_address_len = (sizeof(struct sadb_address) + sasize)/8;
	addr->sadb_address_exttype = type;
	addr->sadb_address_proto = sel->proto;
	addr->sadb_address_reserved = 0;

	switch (type) {
	case SADB_EXT_ADDRESS_SRC:
		addr->sadb_address_prefixlen = sel->prefixlen_s;
		pfkey_sockaddr_fill(&sel->saddr, 0,
				    (struct sockaddr *)(addr + 1),
				    sel->family);
		break;
	case SADB_EXT_ADDRESS_DST:
		addr->sadb_address_prefixlen = sel->prefixlen_d;
		pfkey_sockaddr_fill(&sel->daddr, 0,
				    (struct sockaddr *)(addr + 1),
				    sel->family);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static int set_sadb_kmaddress(struct sk_buff *skb, const struct xfrm_kmaddress *k)
{
	struct sadb_x_kmaddress *kma;
	u8 *sa;
	int family = k->family;
	int socklen = pfkey_sockaddr_len(family);
	int size_req;

	size_req = (sizeof(struct sadb_x_kmaddress) +
		    pfkey_sockaddr_pair_size(family));

	kma = (struct sadb_x_kmaddress *)skb_put(skb, size_req);
	memset(kma, 0, size_req);
	kma->sadb_x_kmaddress_len = size_req / 8;
	kma->sadb_x_kmaddress_exttype = SADB_X_EXT_KMADDRESS;
	kma->sadb_x_kmaddress_reserved = k->reserved;

	sa = (u8 *)(kma + 1);
	if (!pfkey_sockaddr_fill(&k->local, 0, (struct sockaddr *)sa, family) ||
	    !pfkey_sockaddr_fill(&k->remote, 0, (struct sockaddr *)(sa+socklen), family))
		return -EINVAL;

	return 0;
}

static int set_ipsecrequest(struct sk_buff *skb,
			    uint8_t proto, uint8_t mode, int level,
			    uint32_t reqid, uint8_t family,
			    const xfrm_address_t *src, const xfrm_address_t *dst)
{
	struct sadb_x_ipsecrequest *rq;
	u8 *sa;
	int socklen = pfkey_sockaddr_len(family);
	int size_req;

	size_req = sizeof(struct sadb_x_ipsecrequest) +
		   pfkey_sockaddr_pair_size(family);

	rq = (struct sadb_x_ipsecrequest *)skb_put(skb, size_req);
	memset(rq, 0, size_req);
	rq->sadb_x_ipsecrequest_len = size_req;
	rq->sadb_x_ipsecrequest_proto = proto;
	rq->sadb_x_ipsecrequest_mode = mode;
	rq->sadb_x_ipsecrequest_level = level;
	rq->sadb_x_ipsecrequest_reqid = reqid;

	sa = (u8 *) (rq + 1);
	if (!pfkey_sockaddr_fill(src, 0, (struct sockaddr *)sa, family) ||
	    !pfkey_sockaddr_fill(dst, 0, (struct sockaddr *)(sa + socklen), family))
		return -EINVAL;

	return 0;
}
#endif

#ifdef CONFIG_NET_KEY_MIGRATE
static int pfkey_send_migrate(const struct xfrm_selector *sel, u8 dir, u8 type,
			      const struct xfrm_migrate *m, int num_bundles,
			      const struct xfrm_kmaddress *k)
{
	int i;
	int sasize_sel;
	int size = 0;
	int size_pol = 0;
	struct sk_buff *skb;
	struct sadb_msg *hdr;
	struct sadb_x_policy *pol;
	const struct xfrm_migrate *mp;

	if (type != XFRM_POLICY_TYPE_MAIN)
		return 0;

	if (num_bundles <= 0 || num_bundles > XFRM_MAX_DEPTH)
		return -EINVAL;

	if (k != NULL) {
		/* addresses for KM */
		size += PFKEY_ALIGN8(sizeof(struct sadb_x_kmaddress) +
				     pfkey_sockaddr_pair_size(k->family));
	}

	/* selector */
	sasize_sel = pfkey_sockaddr_size(sel->family);
	if (!sasize_sel)
		return -EINVAL;
	size += (sizeof(struct sadb_address) + sasize_sel) * 2;

	/* policy info */
	size_pol += sizeof(struct sadb_x_policy);

	/* ipsecrequests */
	for (i = 0, mp = m; i < num_bundles; i++, mp++) {
		/* old locator pair */
		size_pol += sizeof(struct sadb_x_ipsecrequest) +
			    pfkey_sockaddr_pair_size(mp->old_family);
		/* new locator pair */
		size_pol += sizeof(struct sadb_x_ipsecrequest) +
			    pfkey_sockaddr_pair_size(mp->new_family);
	}

	size += sizeof(struct sadb_msg) + size_pol;

	/* alloc buffer */
	skb = alloc_skb(size, GFP_ATOMIC);
	if (skb == NULL)
		return -ENOMEM;

	hdr = (struct sadb_msg *)skb_put(skb, sizeof(struct sadb_msg));
	hdr->sadb_msg_version = PF_KEY_V2;
	hdr->sadb_msg_type = SADB_X_MIGRATE;
	hdr->sadb_msg_satype = pfkey_proto2satype(m->proto);
	hdr->sadb_msg_len = size / 8;
	hdr->sadb_msg_errno = 0;
	hdr->sadb_msg_reserved = 0;
	hdr->sadb_msg_seq = 0;
	hdr->sadb_msg_pid = 0;

	/* Addresses to be used by KM for negotiation, if ext is available */
	if (k != NULL && (set_sadb_kmaddress(skb, k) < 0))
		return -EINVAL;

	/* selector src */
	set_sadb_address(skb, sasize_sel, SADB_EXT_ADDRESS_SRC, sel);

	/* selector dst */
	set_sadb_address(skb, sasize_sel, SADB_EXT_ADDRESS_DST, sel);

	/* policy information */
	pol = (struct sadb_x_policy *)skb_put(skb, sizeof(struct sadb_x_policy));
	pol->sadb_x_policy_len = size_pol / 8;
	pol->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	pol->sadb_x_policy_type = IPSEC_POLICY_IPSEC;
	pol->sadb_x_policy_dir = dir + 1;
	pol->sadb_x_policy_reserved = 0;
	pol->sadb_x_policy_id = 0;
	pol->sadb_x_policy_priority = 0;

	for (i = 0, mp = m; i < num_bundles; i++, mp++) {
		/* old ipsecrequest */
		int mode = pfkey_mode_from_xfrm(mp->mode);
		if (mode < 0)
			goto err;
		if (set_ipsecrequest(skb, mp->proto, mode,
				     (mp->reqid ?  IPSEC_LEVEL_UNIQUE : IPSEC_LEVEL_REQUIRE),
				     mp->reqid, mp->old_family,
				     &mp->old_saddr, &mp->old_daddr) < 0)
			goto err;

		/* new ipsecrequest */
		if (set_ipsecrequest(skb, mp->proto, mode,
				     (mp->reqid ? IPSEC_LEVEL_UNIQUE : IPSEC_LEVEL_REQUIRE),
				     mp->reqid, mp->new_family,
				     &mp->new_saddr, &mp->new_daddr) < 0)
			goto err;
	}

	/* broadcast migrate message to sockets */
	pfkey_broadcast(skb, GFP_ATOMIC, BROADCAST_ALL, NULL, &init_net);

	return 0;

err:
	kfree_skb(skb);
	return -EINVAL;
}
#else
static int pfkey_send_migrate(const struct xfrm_selector *sel, u8 dir, u8 type,
			      const struct xfrm_migrate *m, int num_bundles,
			      const struct xfrm_kmaddress *k)
{
	return -ENOPROTOOPT;
}
#endif

static int pfkey_sendmsg(struct kiocb *kiocb,
			 struct socket *sock, struct msghdr *msg, size_t len)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb = NULL;
	struct sadb_msg *hdr = NULL;
	int err;

	err = -EOPNOTSUPP;
	if (msg->msg_flags & MSG_OOB)
		goto out;

	err = -EMSGSIZE;
	if ((unsigned)len > sk->sk_sndbuf - 32)
		goto out;

	err = -ENOBUFS;
	skb = alloc_skb(len, GFP_KERNEL);
	if (skb == NULL)
		goto out;

	err = -EFAULT;
	if (memcpy_fromiovec(skb_put(skb,len), msg->msg_iov, len))
		goto out;

	hdr = pfkey_get_base_msg(skb, &err);
	if (!hdr)
		goto out;

	mutex_lock(&xfrm_cfg_mutex);
	err = pfkey_process(sk, skb, hdr);
	mutex_unlock(&xfrm_cfg_mutex);

out:
	if (err && hdr && pfkey_error(hdr, err, sk) == 0)
		err = 0;
	kfree_skb(skb);

	return err ? : len;
}

static int pfkey_recvmsg(struct kiocb *kiocb,
			 struct socket *sock, struct msghdr *msg, size_t len,
			 int flags)
{
	struct sock *sk = sock->sk;
	struct pfkey_sock *pfk = pfkey_sk(sk);
	struct sk_buff *skb;
	int copied, err;

	err = -EINVAL;
	if (flags & ~(MSG_PEEK|MSG_DONTWAIT|MSG_TRUNC|MSG_CMSG_COMPAT))
		goto out;

	skb = skb_recv_datagram(sk, flags, flags & MSG_DONTWAIT, &err);
	if (skb == NULL)
		goto out;

	copied = skb->len;
	if (copied > len) {
		msg->msg_flags |= MSG_TRUNC;
		copied = len;
	}

	skb_reset_transport_header(skb);
	err = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, copied);
	if (err)
		goto out_free;

	sock_recv_ts_and_drops(msg, sk, skb);

	err = (flags & MSG_TRUNC) ? skb->len : copied;

	if (pfk->dump.dump != NULL &&
	    3 * atomic_read(&sk->sk_rmem_alloc) <= sk->sk_rcvbuf)
		pfkey_do_dump(pfk);

out_free:
	skb_free_datagram(sk, skb);
out:
	return err;
}

static const struct proto_ops pfkey_ops = {
	.family		=	PF_KEY,
	.owner		=	THIS_MODULE,
	/* Operations that make no sense on pfkey sockets. */
	.bind		=	sock_no_bind,
	.connect	=	sock_no_connect,
	.socketpair	=	sock_no_socketpair,
	.accept		=	sock_no_accept,
	.getname	=	sock_no_getname,
	.ioctl		=	sock_no_ioctl,
	.listen		=	sock_no_listen,
	.shutdown	=	sock_no_shutdown,
	.setsockopt	=	sock_no_setsockopt,
	.getsockopt	=	sock_no_getsockopt,
	.mmap		=	sock_no_mmap,
	.sendpage	=	sock_no_sendpage,

	/* Now the operations that really occur. */
	.release	=	pfkey_release,
	.poll		=	datagram_poll,
	.sendmsg	=	pfkey_sendmsg,
	.recvmsg	=	pfkey_recvmsg,
};

static const struct net_proto_family pfkey_family_ops = {
	.family	=	PF_KEY,
	.create	=	pfkey_create,
	.owner	=	THIS_MODULE,
};

#ifdef CONFIG_PROC_FS
static int pfkey_seq_show(struct seq_file *f, void *v)
{
	struct sock *s = sk_entry(v);

	if (v == SEQ_START_TOKEN)
		seq_printf(f ,"sk       RefCnt Rmem   Wmem   User   Inode\n");
	else
		seq_printf(f, "%pK %-6d %-6u %-6u %-6u %-6lu\n",
			       s,
			       atomic_read(&s->sk_refcnt),
			       sk_rmem_alloc_get(s),
			       sk_wmem_alloc_get(s),
			       sock_i_uid(s),
			       sock_i_ino(s)
			       );
	return 0;
}

static void *pfkey_seq_start(struct seq_file *f, loff_t *ppos)
	__acquires(rcu)
{
	struct net *net = seq_file_net(f);
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);

	rcu_read_lock();
	return seq_hlist_start_head_rcu(&net_pfkey->table, *ppos);
}

static void *pfkey_seq_next(struct seq_file *f, void *v, loff_t *ppos)
{
	struct net *net = seq_file_net(f);
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);

	return seq_hlist_next_rcu(v, &net_pfkey->table, ppos);
}

static void pfkey_seq_stop(struct seq_file *f, void *v)
	__releases(rcu)
{
	rcu_read_unlock();
}

static const struct seq_operations pfkey_seq_ops = {
	.start	= pfkey_seq_start,
	.next	= pfkey_seq_next,
	.stop	= pfkey_seq_stop,
	.show	= pfkey_seq_show,
};

static int pfkey_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &pfkey_seq_ops,
			    sizeof(struct seq_net_private));
}

static const struct file_operations pfkey_proc_ops = {
	.open	 = pfkey_seq_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = seq_release_net,
};

static int __net_init pfkey_init_proc(struct net *net)
{
	struct proc_dir_entry *e;

	e = proc_net_fops_create(net, "pfkey", 0, &pfkey_proc_ops);
	if (e == NULL)
		return -ENOMEM;

	return 0;
}

static void __net_exit pfkey_exit_proc(struct net *net)
{
	proc_net_remove(net, "pfkey");
}
#else
static inline int pfkey_init_proc(struct net *net)
{
	return 0;
}

static inline void pfkey_exit_proc(struct net *net)
{
}
#endif

static struct xfrm_mgr pfkeyv2_mgr =
{
	.id		= "pfkeyv2",
	.notify		= pfkey_send_notify,
	.acquire	= pfkey_send_acquire,
	.compile_policy	= pfkey_compile_policy,
	.new_mapping	= pfkey_send_new_mapping,
	.notify_policy	= pfkey_send_policy_notify,
	.migrate	= pfkey_send_migrate,
};


#ifdef NLKEY_SUPPORT
extern struct xfrm_state *xfrm_state_lookup_byhandle(struct net *net, u16 handle);

static unsigned short ipsec_sacode_to_nlkeycode(unsigned short sa_code)
{
	unsigned nlkey_code;

	switch (sa_code) 
	{
		case XFRM_MSG_DELSA:
			nlkey_code = NLKEY_SA_DELETE;
			break;
		case XFRM_MSG_NEWSA:
		case XFRM_MSG_UPDSA:
			nlkey_code = NLKEY_SA_CREATE;
			break;
		case XFRM_MSG_FLUSHSA:
			nlkey_code = NLKEY_SA_FLUSH;
			break;
		case XFRM_MSG_EXPIRE:
			nlkey_code = NLKEY_SA_SET_STATE;
			break;
		default:
			nlkey_code = NLKEY_NULL_MSG;
			break;
	}

	return nlkey_code;
}

static void ipsec_nlkey_rcv(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	struct nlkey_msg *msg = NULL;
	struct flowi flow;
	unsigned short *p;
	unsigned short family, dir;
	struct xfrm_state *x;
	struct nlkey_sa_notify sa_notify_msg;
	struct nlkey_sa_info sa_info_msg;

	/* extract message from skb */
	nlh = (struct nlmsghdr *)skb->data;

	msg = (struct nlkey_msg *)NLMSG_DATA(nlh);

	//printk(KERN_INFO "ipsec_nlkey_rcv fcode: 0x%x length: %d bytes\n",msg->fcode,msg->length);

	/* process command received from user space */
	switch(msg->fcode)
	{
		case NLKEY_FLOW_REMOVE:
			//printk(KERN_INFO "ipsec_nlkey_rcv NLKEY_FLOW_REMOVE\n");
			p = msg->payload;
			memcpy(&flow, p, sizeof(struct flowi)); p += sizeof(struct flowi)/2;
			family = *p; p++;
			dir = *p; p++;
			flow_cache_remove(&flow, family, dir);
			break;

		case NLKEY_SA_NOTIFY:
			//printk(KERN_INFO "ipsec_nlkey_rcv NLKEY_SA_NOTIFY\n");
			memcpy(&sa_notify_msg, msg->payload, sizeof(struct nlkey_sa_notify));
			x = xfrm_state_lookup_byhandle(&init_net, sa_notify_msg.sagd);
			if (x) {
				spin_lock(&x->lock);

				if (sa_notify_msg.action) { 
					// hard expired
					x->km.state = XFRM_STATE_EXPIRED;
					tasklet_hrtimer_start(&x->mtimer, ktime_set(0,0), HRTIMER_MODE_REL);
				}
				else if (!x->km.dying) {
					 x->km.dying = 1;
					 km_state_expired(x, 0, 0);
				}

				spin_unlock(&x->lock);
				xfrm_state_put(x);
			}
			break;

		case NLKEY_SA_INFO_UPDATE:
			memcpy(&sa_info_msg, msg->payload, sizeof(struct nlkey_sa_info));

			x = xfrm_state_lookup_byhandle(&init_net,sa_info_msg.sagd);
			if (x) {
				spin_lock(&x->lock);

				x->curlft.bytes = sa_info_msg.bytes;
				x->curlft.packets = sa_info_msg.packets;

				spin_unlock(&x->lock);
				xfrm_state_put(x);
			}
			break;
		default:
			//printk(KERN_INFO "ipsec_nlkey_rcv fcode 0x%x not supported\n", msg->fcode);
			break;
	}

}

extern struct dst_entry *__xfrm_dst_lookup(struct net *net, int tos,
						  xfrm_address_t *saddr,
						  xfrm_address_t *daddr,
						  int family);
static int ipsec_nlkey_set_said(struct net *net, struct xfrm_state *x, 
				const struct km_event *c, struct nlkey_sa_id *said)
{

	struct flowi fl;
	int tos;
	xfrm_address_t saddr, daddr;
	struct dst_entry *dst;
	int rc = 0;

	memset(&fl, 0, sizeof(struct flowi));

	/* SPI */
	said->spi = x->id.spi;
	/* SA Type (AH or ESP) */
	said->sa_type = x->id.proto;
	/* Protocol Family (IPv4 or IPv6) */
	said->proto_family = x->props.family;
	/* Replay window */
	said->replay_window = x->props.replay_window;
	/* Destination IP Address */
	if(x->props.family == AF_INET6) {
		memcpy(&said->dst_ip, x->id.daddr.a6, sizeof(struct in6_addr));
		ipv6_addr_copy(&fl.u.ip6.daddr, (struct in6_addr *)x->id.daddr.a6);
		memcpy(&said->src_ip, x->props.saddr.a6, sizeof(struct in6_addr));
	}
	else {
		said->dst_ip[0] = x->id.daddr.a4;
		fl.u.ip4.daddr = x->id.daddr.a4;
		said->src_ip[0] = x->props.saddr.a4;
	}
	said->mtu = 0;

	if(x->props.flags & XFRM_STATE_ESN)
		said->flags = NLKEY_SAFLAGS_ESN;
	xfrm_flowi_addr_get(&fl, &saddr, &daddr, x->props.family);

	tos = xfrm_get_tos(&fl, x->props.family);
	if (tos < 0) {
		printk(KERN_ERR "%s:%d: FIXME\n",__FUNCTION__,__LINE__);	
		rc = -1;
		goto error;
	}
	
	dst = __xfrm_dst_lookup(net, tos, NULL, &daddr, x->props.family);
	if (IS_ERR(dst)) {
		printk(KERN_ERR "%s:%d: FIXME\n",__FUNCTION__,__LINE__);
		rc = -1;
		goto error;
	}
	said->dev_mtu = dst_mtu(dst);
	said->mtu = xfrm_state_mtu(x,dst_mtu(dst));	

	dst_release(dst);
error:
	return rc;
}

static struct sk_buff * ipsec_xfrm2nlkey (struct net *net, struct xfrm_state *x, 
					const struct km_event *c, unsigned short *msg_id)
{
	struct nlkey_sa_id sa_id_msg;
	struct nlkey_sa_create sa_create_msg;
	struct nlkey_sa_delete sa_delete_msg;
	struct nlkey_sa_set_keys sa_set_keys_msg;
	struct nlkey_sa_set_tunnel sa_set_tunnel_msg;
	struct nlkey_sa_set_natt sa_set_natt_msg;
	struct nlkey_sa_set_state sa_set_state_msg;
	struct nlkey_sa_set_lifetime sa_set_lifetime_msg;
	struct nlkey_msg msg;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	gfp_t allocation = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;
	unsigned char tunnel, keys, natt, state, lifetime;

	/* supported SA informations */
	keys = 1; state = 1; tunnel = 1; lifetime = 1; natt = 1; 

	/* next message to build */
	memset(&msg, 0, sizeof(struct nlkey_msg));
	msg.fcode = *msg_id;
	
	//printk(KERN_INFO "\n\nipsec_xfrm2nlkey: processing event 0x%x\n", msg.fcode);

	switch (msg.fcode)
	{
		case NLKEY_SA_CREATE:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_CREATE\n");
			if(x) {
				/* some check before builing message */
				if((x->id.proto != IPPROTO_ESP) && (x->id.proto != IPPROTO_AH)) {
					printk(KERN_ERR "ipsec_xfrm2nlkey: protocol %d not supported\n", x->id.proto);
					*msg_id = NLKEY_NULL_MSG;
					goto exit;
				}	
				memset(&sa_create_msg, 0, sizeof(struct nlkey_sa_create));	

				/* SA global handler */
				sa_create_msg.sagd = x->handle;

				/* SA identifier */
				if(ipsec_nlkey_set_said(net, x, c, &sa_create_msg.said) < 0)
				{
					printk(KERN_ERR "%s: set sa ID failed\n", __func__);
					*msg_id = NLKEY_NULL_MSG; /* next message */
					goto exit;
				}
				memcpy(msg.payload, &sa_create_msg, sizeof(struct nlkey_sa_create));
				msg.length = sizeof(struct nlkey_sa_create);
				*msg_id = NLKEY_SA_SET_KEYS; /* next message */
			} else {
				*msg_id = NLKEY_NULL_MSG; /* next message */
				goto exit;
			}
			
			break;

		case NLKEY_SA_SET_KEYS:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_SET_KEYS\n");
			if(keys) {
				memset(&sa_set_keys_msg, 0, sizeof(struct nlkey_sa_set_keys));

				/* SA global handler */
				sa_set_keys_msg.sagd = x->handle; 
				
				/* auth key */
				if(x->aalg) {
					if (x->aalg->alg_key_len) {
						sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_bits = x->aalg->alg_key_len;
						sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_alg = x->props.aalgo;
						sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_type = 0;
						memcpy(sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key, x->aalg->alg_key,(sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_bits / 8));
						//printk(KERN_INFO "ipsec_xfrm2nlkey: AUTH - algo %d key %d bits\n", sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_alg, sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_bits);
						sa_set_keys_msg.num_keys++;
					}
				}
				/* encrypt key */
				if(x->ealg) {
					if (x->ealg->alg_key_len) {

						sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_bits = x->ealg->alg_key_len;
						sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_alg = x->props.ealgo;
						sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_type = 1;
						memcpy(sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key, x->ealg->alg_key,(sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_bits / 8));
						//printk(KERN_INFO "ipsec_xfrm2nlkey: ENCRYPT - algo %d key %d bits\n", sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_alg, sa_set_keys_msg.keys[sa_set_keys_msg.num_keys].key_bits);
						sa_set_keys_msg.num_keys++;
					}
				}
				memcpy(msg.payload, &sa_set_keys_msg, sizeof(struct nlkey_sa_set_keys));
				msg.length = sizeof(struct nlkey_sa_set_keys);
				*msg_id = NLKEY_SA_SET_TUNNEL; /* next message */
			} else {
				*msg_id = NLKEY_SA_SET_TUNNEL; /* next message */
				goto exit;
			}
			break;

		case NLKEY_SA_SET_TUNNEL:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_SET_TUNNEL\n");
			if(tunnel && (x->props.mode == XFRM_MODE_TUNNEL)) {
				memset(&sa_set_tunnel_msg, 0, sizeof(struct nlkey_sa_set_tunnel));

				/* SA global handler */
				sa_set_tunnel_msg.sagd = x->handle; 

				/* Tunnel */
				sa_set_tunnel_msg.proto_family = x->props.family;
				if(x->props.family == AF_INET6) {
					struct ipv6hdr *top_iph = &sa_set_tunnel_msg.h.ipv6h;
					int dsfield;
					top_iph->version = 6;
					top_iph->priority = 0;
					top_iph->flow_lbl[0] = 0;
					top_iph->flow_lbl[1] = 0;
					top_iph->flow_lbl[2] = 0;
					top_iph->nexthdr = IPPROTO_IPIP;	
					dsfield = ipv6_get_dsfield(top_iph);
					dsfield = INET_ECN_encapsulate(dsfield, dsfield);
					if (x->props.flags & XFRM_STATE_NOECN)
						dsfield &= ~INET_ECN_MASK;
					ipv6_change_dsfield(top_iph, 0, dsfield);
					top_iph->hop_limit = 64;
					memcpy(&top_iph->daddr, x->id.daddr.a6, sizeof(struct in6_addr));
					memcpy(&top_iph->saddr, x->props.saddr.a6, sizeof(struct in6_addr));
					//printk(KERN_INFO "ipsec_xfrm2nlkey: IPv6 tunnel\n");
					//printk(KERN_INFO "dst: %x %x %x %x\n", x->id.daddr.a6[0], x->id.daddr.a6[1], x->id.daddr.a6[2], x->id.daddr.a6[3]);
					//(KERN_INFO "src: %x %x %x %x\n", x->props.saddr.a6[0], x->props.saddr.a6[1], x->props.saddr.a6[2], x->props.saddr.a6[3]);
				}
				else {
					struct iphdr *top_iph = &sa_set_tunnel_msg.h.ipv4h;
					top_iph->ihl = 5;
					top_iph->version = 4;
					top_iph->tos = 0;
					top_iph->frag_off = 0; 
					top_iph->ttl = 64;
					top_iph->saddr = x->props.saddr.a4;
					top_iph->daddr = x->id.daddr.a4;
					//printk(KERN_INFO "ipsec_xfrm2nlkey: IPv4 tunnel dst:%x - src:%x \n", x->id.daddr.a4, x->props.saddr.a4);
				}
				memcpy(msg.payload, &sa_set_tunnel_msg, sizeof(struct nlkey_sa_set_tunnel));
				msg.length = sizeof(struct nlkey_sa_set_tunnel);
				*msg_id = NLKEY_SA_SET_NATT; /* next message */
			} else {
				*msg_id = NLKEY_SA_SET_NATT; /* next message */
				goto exit;	
			} 
			break;

		case NLKEY_SA_SET_NATT:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_SET_NATT\n");
			if((natt) && (x->encap)){
				memset(&sa_set_natt_msg, 0, sizeof(struct nlkey_sa_set_natt));

				/* SA global handler */
				sa_set_natt_msg.sagd = x->handle; 
				sa_set_natt_msg.sport = x->encap->encap_sport;
				sa_set_natt_msg.dport = x->encap->encap_dport;
				//printk(KERN_INFO "src port: %d  dst port: %d \n", ntohs(sa_set_natt_msg.sport), ntohs( sa_set_natt_msg.dport));
				memcpy(msg.payload, &sa_set_natt_msg, sizeof(struct nlkey_sa_set_natt));
				msg.length = sizeof(struct nlkey_sa_set_natt);
				*msg_id = NLKEY_SA_SET_LIFETIME; /* next message */
			} else {
				*msg_id = NLKEY_SA_SET_LIFETIME; /* next message */
				goto exit;	
			}
			break;

		case NLKEY_SA_SET_LIFETIME:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_SET_LIFETIME\n");
			if(lifetime) {
				memset(&sa_set_lifetime_msg, 0, sizeof(struct nlkey_sa_set_lifetime));

				/* SA global handler */
				sa_set_lifetime_msg.sagd = x->handle;

				/* hard time */
				sa_set_lifetime_msg.hard_time.allocations =  _X2KEY(x->lft.hard_packet_limit);
				if(_X2KEY(x->lft.hard_byte_limit))
					memcpy(sa_set_lifetime_msg.hard_time.bytes, &x->lft.hard_byte_limit, sizeof(uint64_t));

				/* soft time */
				sa_set_lifetime_msg.soft_time.allocations =  _X2KEY(x->lft.soft_packet_limit);
				if(_X2KEY(x->lft.soft_byte_limit))
					memcpy(sa_set_lifetime_msg.soft_time.bytes, &x->lft.soft_byte_limit, sizeof(uint64_t));

				/* current time */
				sa_set_lifetime_msg.current_time.allocations = x->curlft.packets;
				memcpy(sa_set_lifetime_msg.current_time.bytes, &x->curlft.bytes, sizeof(uint64_t));

				memcpy(msg.payload, &sa_set_lifetime_msg, sizeof(struct nlkey_sa_set_lifetime));
				msg.length = sizeof(struct nlkey_sa_set_lifetime);
				*msg_id = NLKEY_SA_SET_STATE; /* next message */
			} else {
				*msg_id = NLKEY_SA_SET_STATE; /* next message */
				goto exit;	
			}
			break;

		case NLKEY_SA_SET_STATE:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SET_STATE\n");
			if(state) {
				memset(&sa_set_state_msg, 0, sizeof(struct nlkey_sa_set_state));
				memset(&sa_id_msg, 0, sizeof(struct nlkey_sa_id));

				/* SA global handler */
				sa_set_state_msg.sagd = x->handle; 
				/* State */
				sa_set_state_msg.state = x->km.state;
				// TODO: set the offloaded state once ack received !
				x->offloaded = 1;
				atomic_inc(&flow_cache_genid);

				memcpy(msg.payload, &sa_set_state_msg, sizeof(struct nlkey_sa_set_state));
				msg.length = sizeof(struct nlkey_sa_set_state);
				*msg_id = NLKEY_NULL_MSG; /* next message */
			} else {
				*msg_id = NLKEY_NULL_MSG; /* next message */
				goto exit;
			}
			break;
		
		case NLKEY_SA_DELETE:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_DELETE\n");
			memset(&sa_delete_msg, 0, sizeof(struct nlkey_sa_delete));
			
			/* SA global handler */
			sa_delete_msg.sagd = x->handle;
			memcpy(msg.payload, &sa_delete_msg, sizeof(struct nlkey_sa_delete));
			msg.length = sizeof(struct nlkey_sa_delete);
			atomic_inc(&flow_cache_genid);


			*msg_id = NLKEY_NULL_MSG; /* next message */
			break;

		case NLKEY_SA_FLUSH:
			//printk(KERN_INFO "ipsec_xfrm2nlkey: NLKEY_SA_FLUSH\n");
			/* No data required for flush SA command */
			atomic_inc(&flow_cache_genid);

			*msg_id = NLKEY_NULL_MSG; /* next message */
			break;

		default:
			printk(KERN_ERR "ipsec_xfrm2nlkey: event 0x%x not supported\n", c->event);
			*msg_id = NLKEY_NULL_MSG; /* next message */
			break;
	}

	/* prepare netlink message for kernel to user space direction */
	if(msg.length > NLKEY_MSG_LEN)
	{
		printk(KERN_ERR "ipsec_xfrm2nlkey: maximum message size reached (%d bytes)\n", msg.length);
		goto exit;
	}

	skb = alloc_skb(NLMSG_SPACE(NLKEY_MSG_LEN + NLKEY_HDR_LEN), allocation);
	if (skb == NULL)
		goto exit;
		
	nlh = (struct nlmsghdr *)skb_put(skb, NLMSG_SPACE(NLKEY_HDR_LEN + msg.length));
	memcpy(NLMSG_DATA(nlh), (unsigned char *)&msg, (NLKEY_HDR_LEN + msg.length));
	
	/* whole length of the message i.e. header + payload */
	nlh->nlmsg_len = NLMSG_SPACE(NLKEY_HDR_LEN + msg.length);

	/* from kernel */
	nlh->nlmsg_pid = 0;
	nlh->nlmsg_flags = 0;
        nlh->nlmsg_type = 0;
	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_group = 1;
exit:
	return skb;
}

static int ipsec_nlkey_send(struct net *net, struct xfrm_state *x, const struct km_event *c)
{
	struct sk_buff *skb;
	unsigned short msg_type;
	int rc = 0;

	/* We may generate more than one message when adding new SA (sa_create + sa_set_state + sa_set_tunnel...) */
	msg_type = ipsec_sacode_to_nlkeycode((unsigned short)c->event);

	while(msg_type != NLKEY_NULL_MSG)
	{
		/* build nlkey message */
		skb = ipsec_xfrm2nlkey(net, x, c, &msg_type);

		if(skb != NULL)
			if((rc = netlink_broadcast(nlkey_socket, skb, 0, 1, GFP_ATOMIC)) < 0)
				return rc;
	}

	return rc;
}


int ipsec_nlkey_flow(u16 xfrm_nr, u16 *xfrm_handle, const struct flowi *fl, u16 family, u16 dir)
{
	struct sk_buff *skb;
	struct nlkey_msg msg;
	struct nlmsghdr *nlh = NULL;
	unsigned short *p;
	gfp_t allocation = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;

	//printk(KERN_INFO "ipsec_nlkey_flow \n");

	/* next message to build */
	memset(&msg, 0, sizeof(struct nlkey_msg));
	msg.fcode = NLKEY_FLOW_ADD;

	// Number of SA for this flow
	p = msg.payload;
	*p++ = xfrm_nr;
	msg.length += sizeof(unsigned short);
	// SA handles list
	memcpy(p, xfrm_handle, xfrm_nr*sizeof(unsigned short));
	msg.length += xfrm_nr*sizeof(unsigned short);
	p+=xfrm_nr;
	// flow family
	*p++ = family;
	msg.length += sizeof(unsigned short);
	// flow family
	*p++ = dir;
	msg.length += sizeof(unsigned short);
	// flow descriptor
	memcpy(p, fl, sizeof(struct flowi));
	msg.length +=sizeof(struct flowi);
	p+=sizeof(struct flowi) / sizeof(u16);

	skb = alloc_skb(NLMSG_SPACE(NLKEY_MSG_LEN + NLKEY_HDR_LEN), allocation);
	if (skb == NULL)
		return -ENOMEM;

	/* prepare netlink message for kernel to user space direction */
	nlh = (struct nlmsghdr *)skb_put(skb, NLMSG_SPACE(NLKEY_HDR_LEN + msg.length));
	memcpy(NLMSG_DATA(nlh), (unsigned char *)&msg, (NLKEY_HDR_LEN + msg.length));

	/* whole length of the message i.e. header + payload */
	nlh->nlmsg_len = NLMSG_SPACE(NLKEY_HDR_LEN + msg.length);

	/* from kernel */
	nlh->nlmsg_pid = 0; 
	nlh->nlmsg_flags = 0;
        nlh->nlmsg_type = 0;
	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_group = 1;

	return(netlink_broadcast(nlkey_socket, skb, 0, 1, allocation));
}
EXPORT_SYMBOL(ipsec_nlkey_flow);


int ipsec_nlkey_flow_remove(struct flowi *fl, u16 family, u16 dir)
{
	struct sk_buff *skb;
	struct nlkey_msg msg;
	struct nlmsghdr *nlh = NULL;
	unsigned short *p;
	gfp_t allocation = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;

	
	//printk(KERN_INFO "ipsec_nlkey_flow_remove\n");

	/* next message to build */
	memset(&msg, 0, sizeof(struct nlkey_msg));
	msg.fcode = NLKEY_FLOW_REMOVE;

	p = msg.payload;
	// flow family
	*p++ = family;
	msg.length += sizeof(unsigned short);
	// flow family
	*p++ = dir;
	msg.length += sizeof(unsigned short);
	// flow descriptor
	memcpy(p, fl, sizeof(struct flowi));
	msg.length +=sizeof(struct flowi);
	p+=sizeof(struct flowi) / sizeof(u16);

	skb = alloc_skb(NLMSG_SPACE(NLKEY_MSG_LEN + NLKEY_HDR_LEN), allocation);
	if (skb == NULL)
		return -ENOMEM;

	/* prepare netlink message for kernel to user space direction */
	nlh = (struct nlmsghdr *)skb_put(skb, NLMSG_SPACE(NLKEY_HDR_LEN + msg.length));
	memcpy(NLMSG_DATA(nlh), (unsigned char *)&msg, (NLKEY_HDR_LEN + msg.length));
	
	/* whole length of the message i.e. header + payload */
	nlh->nlmsg_len = NLMSG_SPACE(NLKEY_HDR_LEN + msg.length);

	/* from kernel */
	nlh->nlmsg_pid = 0; 
	nlh->nlmsg_flags = 0;
        nlh->nlmsg_type = 0;
	NETLINK_CB(skb).pid = 0;
	NETLINK_CB(skb).dst_group = 1;	

		
        return(netlink_broadcast(nlkey_socket, skb, 0, 1, allocation));

	
}
EXPORT_SYMBOL(ipsec_nlkey_flow_remove);



static void ipsec_nlkey_init(void)
{
	printk(KERN_INFO "Initializing NETLINK_KEY socket\n");

	nlkey_socket = netlink_kernel_create(&init_net, NETLINK_KEY, 1,
				     ipsec_nlkey_rcv, NULL, THIS_MODULE);
}
#endif


static int __net_init pfkey_net_init(struct net *net)
{
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);
	int rv;

	INIT_HLIST_HEAD(&net_pfkey->table);
	atomic_set(&net_pfkey->socks_nr, 0);

	rv = pfkey_init_proc(net);

	return rv;
}

static void __net_exit pfkey_net_exit(struct net *net)
{
	struct netns_pfkey *net_pfkey = net_generic(net, pfkey_net_id);

	pfkey_exit_proc(net);
	BUG_ON(!hlist_empty(&net_pfkey->table));
}

static struct pernet_operations pfkey_net_ops = {
	.init = pfkey_net_init,
	.exit = pfkey_net_exit,
	.id   = &pfkey_net_id,
	.size = sizeof(struct netns_pfkey),
};

static void __exit ipsec_pfkey_exit(void)
{
	xfrm_unregister_km(&pfkeyv2_mgr);
	sock_unregister(PF_KEY);
	unregister_pernet_subsys(&pfkey_net_ops);
	proto_unregister(&key_proto);

#ifdef NLKEY_SUPPORT
	/* release NETLINK_KEY socket */
	sock_release(nlkey_socket->sk_socket);
#endif
}

static int __init ipsec_pfkey_init(void)
{
	int err = proto_register(&key_proto, 0);

	if (err != 0)
		goto out;

	err = register_pernet_subsys(&pfkey_net_ops);
	if (err != 0)
		goto out_unregister_key_proto;
	err = sock_register(&pfkey_family_ops);
	if (err != 0)
		goto out_unregister_pernet;
	err = xfrm_register_km(&pfkeyv2_mgr);
	if (err != 0)
		goto out_sock_unregister;

#ifdef NLKEY_SUPPORT
	/* create NETLINK_KEY socket for IPSec offload on Comcerto */
	ipsec_nlkey_init();
#endif

out:
	return err;

out_sock_unregister:
	sock_unregister(PF_KEY);
out_unregister_pernet:
	unregister_pernet_subsys(&pfkey_net_ops);
out_unregister_key_proto:
	proto_unregister(&key_proto);
	goto out;
}

module_init(ipsec_pfkey_init);
module_exit(ipsec_pfkey_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS_NETPROTO(PF_KEY);
