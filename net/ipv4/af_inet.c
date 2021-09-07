 
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/sockios.h>
#include <linux/net.h>
#include <linux/capability.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/netfilter_ipv4.h>
#include <linux/random.h>

#include <asm/uaccess.h>
#include <asm/system.h>

#include <linux/inet.h>
#include <linux/igmp.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <net/checksum.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/arp.h>
#include <net/route.h>
#include <net/ip_fib.h>
#include <net/inet_connection_sock.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/udplite.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/raw.h>
#include <net/icmp.h>
#include <net/ipip.h>
#include <net/inet_common.h>
#include <net/xfrm.h>
#include <net/net_namespace.h>
#ifdef CONFIG_IP_MROUTE
#include <linux/mroute.h>
#endif

static struct list_head inetsw[SOCK_MAX];
static DEFINE_SPINLOCK(inetsw_lock);

struct ipv4_config ipv4_config;
EXPORT_SYMBOL(ipv4_config);

#ifdef CONFIG_SYNO_PLX_PORTING
int sk_wrong_state_for_release(struct sock *sk)
{
	return (sk->sk_type == SOCK_STREAM && sk->sk_state != TCP_CLOSE);
}
#endif

void inet_sock_destruct(struct sock *sk)
{
	struct inet_sock *inet = inet_sk(sk);

	__skb_queue_purge(&sk->sk_receive_queue);
	__skb_queue_purge(&sk->sk_error_queue);

	sk_mem_reclaim(sk);

#ifdef CONFIG_SYNO_PLX_PORTING
 	if (sk_wrong_state_for_release(sk)) {
#else
	if (sk->sk_type == SOCK_STREAM && sk->sk_state != TCP_CLOSE) {
#endif
		pr_err("Attempt to release TCP socket in state %d %p\n",
		       sk->sk_state, sk);
		return;
	}
	if (!sock_flag(sk, SOCK_DEAD)) {
		pr_err("Attempt to release alive inet socket %p\n", sk);
		return;
	}

	WARN_ON(atomic_read(&sk->sk_rmem_alloc));
	WARN_ON(atomic_read(&sk->sk_wmem_alloc));
	WARN_ON(sk->sk_wmem_queued);
	WARN_ON(sk->sk_forward_alloc);

	kfree(inet->opt);
	dst_release(sk->sk_dst_cache);
	sk_refcnt_debug_dec(sk);
}
EXPORT_SYMBOL(inet_sock_destruct);

static int inet_autobind(struct sock *sk)
{
	struct inet_sock *inet;
	 
	lock_sock(sk);
	inet = inet_sk(sk);
	if (!inet->num) {
		if (sk->sk_prot->get_port(sk, 0)) {
			release_sock(sk);
			return -EAGAIN;
		}
		inet->sport = htons(inet->num);
	}
	release_sock(sk);
	return 0;
}

int inet_listen(struct socket *sock, int backlog)
{
	struct sock *sk = sock->sk;
	unsigned char old_state;
	int err;

	lock_sock(sk);

	err = -EINVAL;
	if (sock->state != SS_UNCONNECTED || sock->type != SOCK_STREAM)
		goto out;

	old_state = sk->sk_state;
	if (!((1 << old_state) & (TCPF_CLOSE | TCPF_LISTEN)))
		goto out;

	if (old_state != TCP_LISTEN) {
		err = inet_csk_listen_start(sk, backlog);
		if (err)
			goto out;
	}
	sk->sk_max_ack_backlog = backlog;
	err = 0;

out:
	release_sock(sk);
	return err;
}
EXPORT_SYMBOL(inet_listen);

u32 inet_ehash_secret __read_mostly;
EXPORT_SYMBOL(inet_ehash_secret);

void build_ehash_secret(void)
{
	u32 rnd;
	do {
		get_random_bytes(&rnd, sizeof(rnd));
	} while (rnd == 0);
	spin_lock_bh(&inetsw_lock);
	if (!inet_ehash_secret)
		inet_ehash_secret = rnd;
	spin_unlock_bh(&inetsw_lock);
}
EXPORT_SYMBOL(build_ehash_secret);

static inline int inet_netns_ok(struct net *net, int protocol)
{
	int hash;
	const struct net_protocol *ipprot;

	if (net_eq(net, &init_net))
		return 1;

	hash = protocol & (MAX_INET_PROTOS - 1);
	ipprot = rcu_dereference(inet_protos[hash]);

	if (ipprot == NULL)
		 
		return 1;
	return ipprot->netns_ok;
}

static int inet_create(struct net *net, struct socket *sock, int protocol)
{
	struct sock *sk;
	struct inet_protosw *answer;
	struct inet_sock *inet;
	struct proto *answer_prot;
	unsigned char answer_flags;
	char answer_no_check;
	int try_loading_module = 0;
	int err;

	if (unlikely(!inet_ehash_secret))
		if (sock->type != SOCK_RAW && sock->type != SOCK_DGRAM)
			build_ehash_secret();

	sock->state = SS_UNCONNECTED;

lookup_protocol:
	err = -ESOCKTNOSUPPORT;
	rcu_read_lock();
	list_for_each_entry_rcu(answer, &inetsw[sock->type], list) {

		err = 0;
		 
		if (protocol == answer->protocol) {
			if (protocol != IPPROTO_IP)
				break;
		} else {
			 
			if (IPPROTO_IP == protocol) {
				protocol = answer->protocol;
				break;
			}
			if (IPPROTO_IP == answer->protocol)
				break;
		}
		err = -EPROTONOSUPPORT;
	}

	if (unlikely(err)) {
		if (try_loading_module < 2) {
			rcu_read_unlock();
			 
			if (++try_loading_module == 1)
				request_module("net-pf-%d-proto-%d-type-%d",
					       PF_INET, protocol, sock->type);
			 
			else
				request_module("net-pf-%d-proto-%d",
					       PF_INET, protocol);
			goto lookup_protocol;
		} else
			goto out_rcu_unlock;
	}

	err = -EPERM;
	if (answer->capability > 0 && !capable(answer->capability))
		goto out_rcu_unlock;

	err = -EAFNOSUPPORT;
	if (!inet_netns_ok(net, protocol))
		goto out_rcu_unlock;

	sock->ops = answer->ops;
	answer_prot = answer->prot;
	answer_no_check = answer->no_check;
	answer_flags = answer->flags;
	rcu_read_unlock();

	WARN_ON(answer_prot->slab == NULL);

	err = -ENOBUFS;
	sk = sk_alloc(net, PF_INET, GFP_KERNEL, answer_prot);
	if (sk == NULL)
		goto out;

	err = 0;
	sk->sk_no_check = answer_no_check;
	if (INET_PROTOSW_REUSE & answer_flags)
		sk->sk_reuse = 1;

	inet = inet_sk(sk);
	inet->is_icsk = (INET_PROTOSW_ICSK & answer_flags) != 0;

	if (SOCK_RAW == sock->type) {
		inet->num = protocol;
		if (IPPROTO_RAW == protocol)
			inet->hdrincl = 1;
	}

	if (ipv4_config.no_pmtu_disc)
		inet->pmtudisc = IP_PMTUDISC_DONT;
	else
		inet->pmtudisc = IP_PMTUDISC_WANT;

	inet->id = 0;

	sock_init_data(sock, sk);

	sk->sk_destruct	   = inet_sock_destruct;
	sk->sk_protocol	   = protocol;
	sk->sk_backlog_rcv = sk->sk_prot->backlog_rcv;

	inet->uc_ttl	= -1;
	inet->mc_loop	= 1;
	inet->mc_ttl	= 1;
	inet->mc_all	= 1;
	inet->mc_index	= 0;
	inet->mc_list	= NULL;

	sk_refcnt_debug_inc(sk);

	if (inet->num) {
		 
		inet->sport = htons(inet->num);
		 
		sk->sk_prot->hash(sk);
	}

	if (sk->sk_prot->init) {
		err = sk->sk_prot->init(sk);
		if (err)
			sk_common_release(sk);
	}
out:
	return err;
out_rcu_unlock:
	rcu_read_unlock();
	goto out;
}

int inet_release(struct socket *sock)
{
	struct sock *sk = sock->sk;

	if (sk) {
		long timeout;

		ip_mc_drop_socket(sk);

		timeout = 0;
		if (sock_flag(sk, SOCK_LINGER) &&
		    !(current->flags & PF_EXITING))
			timeout = sk->sk_lingertime;
		sock->sk = NULL;
		sk->sk_prot->close(sk, timeout);
	}
	return 0;
}
EXPORT_SYMBOL(inet_release);

int sysctl_ip_nonlocal_bind __read_mostly;
EXPORT_SYMBOL(sysctl_ip_nonlocal_bind);

int inet_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
	struct sockaddr_in *addr = (struct sockaddr_in *)uaddr;
	struct sock *sk = sock->sk;
	struct inet_sock *inet = inet_sk(sk);
	unsigned short snum;
	int chk_addr_ret;
	int err;

	if (sk->sk_prot->bind) {
		err = sk->sk_prot->bind(sk, uaddr, addr_len);
		goto out;
	}
	err = -EINVAL;
	if (addr_len < sizeof(struct sockaddr_in))
		goto out;

	chk_addr_ret = inet_addr_type(sock_net(sk), addr->sin_addr.s_addr);

	err = -EADDRNOTAVAIL;
	if (!sysctl_ip_nonlocal_bind &&
	    !(inet->freebind || inet->transparent) &&
	    addr->sin_addr.s_addr != htonl(INADDR_ANY) &&
	    chk_addr_ret != RTN_LOCAL &&
	    chk_addr_ret != RTN_MULTICAST &&
	    chk_addr_ret != RTN_BROADCAST)
		goto out;

	snum = ntohs(addr->sin_port);
	err = -EACCES;
	if (snum && snum < PROT_SOCK && !capable(CAP_NET_BIND_SERVICE))
		goto out;

	lock_sock(sk);

	err = -EINVAL;
	if (sk->sk_state != TCP_CLOSE || inet->num)
		goto out_release_sock;

	inet->rcv_saddr = inet->saddr = addr->sin_addr.s_addr;
	if (chk_addr_ret == RTN_MULTICAST || chk_addr_ret == RTN_BROADCAST)
		inet->saddr = 0;   

	if (sk->sk_prot->get_port(sk, snum)) {
		inet->saddr = inet->rcv_saddr = 0;
		err = -EADDRINUSE;
		goto out_release_sock;
	}

	if (inet->rcv_saddr)
		sk->sk_userlocks |= SOCK_BINDADDR_LOCK;
	if (snum)
		sk->sk_userlocks |= SOCK_BINDPORT_LOCK;
	inet->sport = htons(inet->num);
	inet->daddr = 0;
	inet->dport = 0;
	sk_dst_reset(sk);
	err = 0;
out_release_sock:
	release_sock(sk);
out:
	return err;
}
EXPORT_SYMBOL(inet_bind);

int inet_dgram_connect(struct socket *sock, struct sockaddr * uaddr,
		       int addr_len, int flags)
{
	struct sock *sk = sock->sk;

	if (uaddr->sa_family == AF_UNSPEC)
		return sk->sk_prot->disconnect(sk, flags);

	if (!inet_sk(sk)->num && inet_autobind(sk))
		return -EAGAIN;
	return sk->sk_prot->connect(sk, (struct sockaddr *)uaddr, addr_len);
}
EXPORT_SYMBOL(inet_dgram_connect);

static long inet_wait_for_connect(struct sock *sk, long timeo)
{
	DEFINE_WAIT(wait);

	prepare_to_wait(sk->sk_sleep, &wait, TASK_INTERRUPTIBLE);

	while ((1 << sk->sk_state) & (TCPF_SYN_SENT | TCPF_SYN_RECV)) {
		release_sock(sk);
		timeo = schedule_timeout(timeo);
		lock_sock(sk);
		if (signal_pending(current) || !timeo)
			break;
		prepare_to_wait(sk->sk_sleep, &wait, TASK_INTERRUPTIBLE);
	}
	finish_wait(sk->sk_sleep, &wait);
	return timeo;
}

int inet_stream_connect(struct socket *sock, struct sockaddr *uaddr,
			int addr_len, int flags)
{
	struct sock *sk = sock->sk;
	int err;
	long timeo;

	lock_sock(sk);

	if (uaddr->sa_family == AF_UNSPEC) {
		err = sk->sk_prot->disconnect(sk, flags);
		sock->state = err ? SS_DISCONNECTING : SS_UNCONNECTED;
		goto out;
	}

	switch (sock->state) {
	default:
		err = -EINVAL;
		goto out;
	case SS_CONNECTED:
		err = -EISCONN;
		goto out;
	case SS_CONNECTING:
		err = -EALREADY;
		 
		break;
	case SS_UNCONNECTED:
		err = -EISCONN;
		if (sk->sk_state != TCP_CLOSE)
			goto out;

		err = sk->sk_prot->connect(sk, uaddr, addr_len);
		if (err < 0)
			goto out;

		sock->state = SS_CONNECTING;

		err = -EINPROGRESS;
		break;
	}

	timeo = sock_sndtimeo(sk, flags & O_NONBLOCK);

	if ((1 << sk->sk_state) & (TCPF_SYN_SENT | TCPF_SYN_RECV)) {
		 
		if (!timeo || !inet_wait_for_connect(sk, timeo))
			goto out;

		err = sock_intr_errno(timeo);
		if (signal_pending(current))
			goto out;
	}

	if (sk->sk_state == TCP_CLOSE)
		goto sock_error;

	sock->state = SS_CONNECTED;
	err = 0;
out:
	release_sock(sk);
	return err;

sock_error:
	err = sock_error(sk) ? : -ECONNABORTED;
	sock->state = SS_UNCONNECTED;
	if (sk->sk_prot->disconnect(sk, flags))
		sock->state = SS_DISCONNECTING;
	goto out;
}
EXPORT_SYMBOL(inet_stream_connect);

int inet_accept(struct socket *sock, struct socket *newsock, int flags)
{
	struct sock *sk1 = sock->sk;
	int err = -EINVAL;
	struct sock *sk2 = sk1->sk_prot->accept(sk1, flags, &err);

	if (!sk2)
		goto do_err;

	lock_sock(sk2);

	WARN_ON(!((1 << sk2->sk_state) &
		  (TCPF_ESTABLISHED | TCPF_CLOSE_WAIT | TCPF_CLOSE)));

	sock_graft(sk2, newsock);

	newsock->state = SS_CONNECTED;
	err = 0;
	release_sock(sk2);
do_err:
	return err;
}
EXPORT_SYMBOL(inet_accept);

int inet_getname(struct socket *sock, struct sockaddr *uaddr,
			int *uaddr_len, int peer)
{
	struct sock *sk		= sock->sk;
	struct inet_sock *inet	= inet_sk(sk);
	struct sockaddr_in *sin	= (struct sockaddr_in *)uaddr;

	sin->sin_family = AF_INET;
	if (peer) {
		if (!inet->dport ||
		    (((1 << sk->sk_state) & (TCPF_CLOSE | TCPF_SYN_SENT)) &&
		     peer == 1))
			return -ENOTCONN;
		sin->sin_port = inet->dport;
		sin->sin_addr.s_addr = inet->daddr;
	} else {
		__be32 addr = inet->rcv_saddr;
		if (!addr)
			addr = inet->saddr;
		sin->sin_port = inet->sport;
		sin->sin_addr.s_addr = addr;
	}
	memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
	*uaddr_len = sizeof(*sin);
	return 0;
}
EXPORT_SYMBOL(inet_getname);

int inet_sendmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg,
		 size_t size)
{
	struct sock *sk = sock->sk;

	if (!inet_sk(sk)->num && inet_autobind(sk))
		return -EAGAIN;

	return sk->sk_prot->sendmsg(iocb, sk, msg, size);
}
EXPORT_SYMBOL(inet_sendmsg);

static ssize_t inet_sendpage(struct socket *sock, struct page *page, int offset,
			     size_t size, int flags)
{
	struct sock *sk = sock->sk;

	if (!inet_sk(sk)->num && inet_autobind(sk))
		return -EAGAIN;

	if (sk->sk_prot->sendpage)
		return sk->sk_prot->sendpage(sk, page, offset, size, flags);
	return sock_no_sendpage(sock, page, offset, size, flags);
}

int inet_shutdown(struct socket *sock, int how)
{
	struct sock *sk = sock->sk;
	int err = 0;

	how++;  
	if ((how & ~SHUTDOWN_MASK) || !how)	 
		return -EINVAL;

	lock_sock(sk);
	if (sock->state == SS_CONNECTING) {
		if ((1 << sk->sk_state) &
		    (TCPF_SYN_SENT | TCPF_SYN_RECV | TCPF_CLOSE))
			sock->state = SS_DISCONNECTING;
		else
			sock->state = SS_CONNECTED;
	}

	switch (sk->sk_state) {
	case TCP_CLOSE:
		err = -ENOTCONN;
		 
	default:
		sk->sk_shutdown |= how;
		if (sk->sk_prot->shutdown)
			sk->sk_prot->shutdown(sk, how);
		break;

	case TCP_LISTEN:
		if (!(how & RCV_SHUTDOWN))
			break;
		 
	case TCP_SYN_SENT:
		err = sk->sk_prot->disconnect(sk, O_NONBLOCK);
		sock->state = err ? SS_DISCONNECTING : SS_UNCONNECTED;
		break;
	}

	sk->sk_state_change(sk);
	release_sock(sk);
	return err;
}
EXPORT_SYMBOL(inet_shutdown);

int inet_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;
	int err = 0;
	struct net *net = sock_net(sk);

	switch (cmd) {
	case SIOCGSTAMP:
		err = sock_get_timestamp(sk, (struct timeval __user *)arg);
		break;
	case SIOCGSTAMPNS:
		err = sock_get_timestampns(sk, (struct timespec __user *)arg);
		break;
	case SIOCADDRT:
	case SIOCDELRT:
	case SIOCRTMSG:
		err = ip_rt_ioctl(net, cmd, (void __user *)arg);
		break;
	case SIOCDARP:
	case SIOCGARP:
	case SIOCSARP:
		err = arp_ioctl(net, cmd, (void __user *)arg);
		break;
	case SIOCGIFADDR:
	case SIOCSIFADDR:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFDSTADDR:
	case SIOCSIFDSTADDR:
	case SIOCSIFPFLAGS:
	case SIOCGIFPFLAGS:
	case SIOCSIFFLAGS:
		err = devinet_ioctl(net, cmd, (void __user *)arg);
		break;
	default:
		if (sk->sk_prot->ioctl)
			err = sk->sk_prot->ioctl(sk, cmd, arg);
		else
			err = -ENOIOCTLCMD;
		break;
	}
	return err;
}
EXPORT_SYMBOL(inet_ioctl);

const struct proto_ops inet_stream_ops = {
	.family		   = PF_INET,
	.owner		   = THIS_MODULE,
	.release	   = inet_release,
	.bind		   = inet_bind,
	.connect	   = inet_stream_connect,
	.socketpair	   = sock_no_socketpair,
	.accept		   = inet_accept,
	.getname	   = inet_getname,
	.poll		   = tcp_poll,
	.ioctl		   = inet_ioctl,
	.listen		   = inet_listen,
	.shutdown	   = inet_shutdown,
	.setsockopt	   = sock_common_setsockopt,
	.getsockopt	   = sock_common_getsockopt,
	.sendmsg	   = tcp_sendmsg,
	.recvmsg	   = sock_common_recvmsg,
	.mmap		   = sock_no_mmap,
	.sendpage	   = tcp_sendpage,
#ifdef CONFIG_SYNO_PLX_PORTING
	.sendpages	   = tcp_sendpages,
#endif
	.splice_read	   = tcp_splice_read,
#ifdef CONFIG_COMPAT
	.compat_setsockopt = compat_sock_common_setsockopt,
	.compat_getsockopt = compat_sock_common_getsockopt,
#endif
};
EXPORT_SYMBOL(inet_stream_ops);

const struct proto_ops inet_dgram_ops = {
	.family		   = PF_INET,
	.owner		   = THIS_MODULE,
	.release	   = inet_release,
	.bind		   = inet_bind,
	.connect	   = inet_dgram_connect,
	.socketpair	   = sock_no_socketpair,
	.accept		   = sock_no_accept,
	.getname	   = inet_getname,
	.poll		   = udp_poll,
	.ioctl		   = inet_ioctl,
	.listen		   = sock_no_listen,
	.shutdown	   = inet_shutdown,
	.setsockopt	   = sock_common_setsockopt,
	.getsockopt	   = sock_common_getsockopt,
	.sendmsg	   = inet_sendmsg,
	.recvmsg	   = sock_common_recvmsg,
	.mmap		   = sock_no_mmap,
	.sendpage	   = inet_sendpage,
#ifdef CONFIG_COMPAT
	.compat_setsockopt = compat_sock_common_setsockopt,
	.compat_getsockopt = compat_sock_common_getsockopt,
#endif
};
EXPORT_SYMBOL(inet_dgram_ops);

static const struct proto_ops inet_sockraw_ops = {
	.family		   = PF_INET,
	.owner		   = THIS_MODULE,
	.release	   = inet_release,
	.bind		   = inet_bind,
	.connect	   = inet_dgram_connect,
	.socketpair	   = sock_no_socketpair,
	.accept		   = sock_no_accept,
	.getname	   = inet_getname,
	.poll		   = datagram_poll,
	.ioctl		   = inet_ioctl,
	.listen		   = sock_no_listen,
	.shutdown	   = inet_shutdown,
	.setsockopt	   = sock_common_setsockopt,
	.getsockopt	   = sock_common_getsockopt,
	.sendmsg	   = inet_sendmsg,
	.recvmsg	   = sock_common_recvmsg,
	.mmap		   = sock_no_mmap,
	.sendpage	   = inet_sendpage,
#ifdef CONFIG_COMPAT
	.compat_setsockopt = compat_sock_common_setsockopt,
	.compat_getsockopt = compat_sock_common_getsockopt,
#endif
};

static struct net_proto_family inet_family_ops = {
	.family = PF_INET,
	.create = inet_create,
	.owner	= THIS_MODULE,
};

static struct inet_protosw inetsw_array[] =
{
	{
		.type =       SOCK_STREAM,
		.protocol =   IPPROTO_TCP,
		.prot =       &tcp_prot,
		.ops =        &inet_stream_ops,
		.capability = -1,
		.no_check =   0,
		.flags =      INET_PROTOSW_PERMANENT |
			      INET_PROTOSW_ICSK,
	},

	{
		.type =       SOCK_DGRAM,
		.protocol =   IPPROTO_UDP,
		.prot =       &udp_prot,
		.ops =        &inet_dgram_ops,
		.capability = -1,
		.no_check =   UDP_CSUM_DEFAULT,
		.flags =      INET_PROTOSW_PERMANENT,
       },

       {
	       .type =       SOCK_RAW,
	       .protocol =   IPPROTO_IP,	 
	       .prot =       &raw_prot,
	       .ops =        &inet_sockraw_ops,
	       .capability = CAP_NET_RAW,
	       .no_check =   UDP_CSUM_DEFAULT,
	       .flags =      INET_PROTOSW_REUSE,
       }
};

#define INETSW_ARRAY_LEN ARRAY_SIZE(inetsw_array)

void inet_register_protosw(struct inet_protosw *p)
{
	struct list_head *lh;
	struct inet_protosw *answer;
	int protocol = p->protocol;
	struct list_head *last_perm;

	spin_lock_bh(&inetsw_lock);

	if (p->type >= SOCK_MAX)
		goto out_illegal;

	answer = NULL;
	last_perm = &inetsw[p->type];
	list_for_each(lh, &inetsw[p->type]) {
		answer = list_entry(lh, struct inet_protosw, list);

		if (INET_PROTOSW_PERMANENT & answer->flags) {
			if (protocol == answer->protocol)
				break;
			last_perm = lh;
		}

		answer = NULL;
	}
	if (answer)
		goto out_permanent;

	list_add_rcu(&p->list, last_perm);
out:
	spin_unlock_bh(&inetsw_lock);

	return;

out_permanent:
	printk(KERN_ERR "Attempt to override permanent protocol %d.\n",
	       protocol);
	goto out;

out_illegal:
	printk(KERN_ERR
	       "Ignoring attempt to register invalid socket type %d.\n",
	       p->type);
	goto out;
}
EXPORT_SYMBOL(inet_register_protosw);

void inet_unregister_protosw(struct inet_protosw *p)
{
	if (INET_PROTOSW_PERMANENT & p->flags) {
		printk(KERN_ERR
		       "Attempt to unregister permanent protocol %d.\n",
		       p->protocol);
	} else {
		spin_lock_bh(&inetsw_lock);
		list_del_rcu(&p->list);
		spin_unlock_bh(&inetsw_lock);

		synchronize_net();
	}
}
EXPORT_SYMBOL(inet_unregister_protosw);

int sysctl_ip_dynaddr __read_mostly;

static int inet_sk_reselect_saddr(struct sock *sk)
{
	struct inet_sock *inet = inet_sk(sk);
	int err;
	struct rtable *rt;
	__be32 old_saddr = inet->saddr;
	__be32 new_saddr;
	__be32 daddr = inet->daddr;

	if (inet->opt && inet->opt->srr)
		daddr = inet->opt->faddr;

	err = ip_route_connect(&rt, daddr, 0,
			       RT_CONN_FLAGS(sk),
			       sk->sk_bound_dev_if,
			       sk->sk_protocol,
			       inet->sport, inet->dport, sk, 0);
	if (err)
		return err;

	sk_setup_caps(sk, &rt->u.dst);

	new_saddr = rt->rt_src;

	if (new_saddr == old_saddr)
		return 0;

	if (sysctl_ip_dynaddr > 1) {
		printk(KERN_INFO "%s(): shifting inet->saddr from %pI4 to %pI4\n",
		       __func__, &old_saddr, &new_saddr);
	}

	inet->saddr = inet->rcv_saddr = new_saddr;

	__sk_prot_rehash(sk);
	return 0;
}

int inet_sk_rebuild_header(struct sock *sk)
{
	struct inet_sock *inet = inet_sk(sk);
	struct rtable *rt = (struct rtable *)__sk_dst_check(sk, 0);
	__be32 daddr;
	int err;

	if (rt)
		return 0;

	daddr = inet->daddr;
	if (inet->opt && inet->opt->srr)
		daddr = inet->opt->faddr;
{
	struct flowi fl = {
		.oif = sk->sk_bound_dev_if,
		.mark = sk->sk_mark,
		.nl_u = {
			.ip4_u = {
				.daddr	= daddr,
				.saddr	= inet->saddr,
				.tos	= RT_CONN_FLAGS(sk),
			},
		},
		.proto = sk->sk_protocol,
		.flags = inet_sk_flowi_flags(sk),
		.uli_u = {
			.ports = {
				.sport = inet->sport,
				.dport = inet->dport,
			},
		},
	};

	security_sk_classify_flow(sk, &fl);
	err = ip_route_output_flow(sock_net(sk), &rt, &fl, sk, 0);
}
	if (!err)
		sk_setup_caps(sk, &rt->u.dst);
	else {
		 
		sk->sk_route_caps = 0;
		 
		if (!sysctl_ip_dynaddr ||
		    sk->sk_state != TCP_SYN_SENT ||
		    (sk->sk_userlocks & SOCK_BINDADDR_LOCK) ||
		    (err = inet_sk_reselect_saddr(sk)) != 0)
			sk->sk_err_soft = -err;
	}

	return err;
}
EXPORT_SYMBOL(inet_sk_rebuild_header);

static int inet_gso_send_check(struct sk_buff *skb)
{
	struct iphdr *iph;
	const struct net_protocol *ops;
	int proto;
	int ihl;
	int err = -EINVAL;

	if (unlikely(!pskb_may_pull(skb, sizeof(*iph))))
		goto out;

	iph = ip_hdr(skb);
	ihl = iph->ihl * 4;
	if (ihl < sizeof(*iph))
		goto out;

	if (unlikely(!pskb_may_pull(skb, ihl)))
		goto out;

	__skb_pull(skb, ihl);
	skb_reset_transport_header(skb);
	iph = ip_hdr(skb);
	proto = iph->protocol & (MAX_INET_PROTOS - 1);
	err = -EPROTONOSUPPORT;

	rcu_read_lock();
	ops = rcu_dereference(inet_protos[proto]);
	if (likely(ops && ops->gso_send_check))
		err = ops->gso_send_check(skb);
	rcu_read_unlock();

out:
	return err;
}

static struct sk_buff *inet_gso_segment(struct sk_buff *skb, int features)
{
	struct sk_buff *segs = ERR_PTR(-EINVAL);
	struct iphdr *iph;
	const struct net_protocol *ops;
	int proto;
	int ihl;
	int id;
	unsigned int offset = 0;

	if (!(features & NETIF_F_V4_CSUM))
		features &= ~NETIF_F_SG;

	if (unlikely(skb_shinfo(skb)->gso_type &
		     ~(SKB_GSO_TCPV4 |
		       SKB_GSO_UDP |
		       SKB_GSO_DODGY |
		       SKB_GSO_TCP_ECN |
		       0)))
		goto out;

	if (unlikely(!pskb_may_pull(skb, sizeof(*iph))))
		goto out;

	iph = ip_hdr(skb);
	ihl = iph->ihl * 4;
	if (ihl < sizeof(*iph))
		goto out;

	if (unlikely(!pskb_may_pull(skb, ihl)))
		goto out;

	__skb_pull(skb, ihl);
	skb_reset_transport_header(skb);
	iph = ip_hdr(skb);
	id = ntohs(iph->id);
	proto = iph->protocol & (MAX_INET_PROTOS - 1);
	segs = ERR_PTR(-EPROTONOSUPPORT);

	rcu_read_lock();
	ops = rcu_dereference(inet_protos[proto]);
	if (likely(ops && ops->gso_segment))
		segs = ops->gso_segment(skb, features);
	rcu_read_unlock();

	if (!segs || IS_ERR(segs))
		goto out;

	skb = segs;
	do {
		iph = ip_hdr(skb);
		if (proto == IPPROTO_UDP) {
			iph->id = htons(id);
			iph->frag_off = htons(offset >> 3);
			if (skb->next != NULL)
				iph->frag_off |= htons(IP_MF);
			offset += (skb->len - skb->mac_len - iph->ihl * 4);
		} else
			iph->id = htons(id++);
		iph->tot_len = htons(skb->len - skb->mac_len);
		iph->check = 0;
		iph->check = ip_fast_csum(skb_network_header(skb), iph->ihl);
	} while ((skb = skb->next));

out:
	return segs;
}

static struct sk_buff **inet_gro_receive(struct sk_buff **head,
					 struct sk_buff *skb)
{
	const struct net_protocol *ops;
	struct sk_buff **pp = NULL;
	struct sk_buff *p;
	struct iphdr *iph;
	unsigned int hlen;
	unsigned int off;
	unsigned int id;
	int flush = 1;
	int proto;

	off = skb_gro_offset(skb);
	hlen = off + sizeof(*iph);
	iph = skb_gro_header_fast(skb, off);
	if (skb_gro_header_hard(skb, hlen)) {
		iph = skb_gro_header_slow(skb, hlen, off);
		if (unlikely(!iph))
			goto out;
	}

	proto = iph->protocol & (MAX_INET_PROTOS - 1);

	rcu_read_lock();
	ops = rcu_dereference(inet_protos[proto]);
	if (!ops || !ops->gro_receive)
		goto out_unlock;

	if (*(u8 *)iph != 0x45)
		goto out_unlock;

	if (unlikely(ip_fast_csum((u8 *)iph, iph->ihl)))
		goto out_unlock;

	id = ntohl(*(u32 *)&iph->id);
	flush = (u16)((ntohl(*(u32 *)iph) ^ skb_gro_len(skb)) | (id ^ IP_DF));
	id >>= 16;

	for (p = *head; p; p = p->next) {
		struct iphdr *iph2;

		if (!NAPI_GRO_CB(p)->same_flow)
			continue;

		iph2 = ip_hdr(p);

		if ((iph->protocol ^ iph2->protocol) |
		    (iph->tos ^ iph2->tos) |
		    (iph->saddr ^ iph2->saddr) |
		    (iph->daddr ^ iph2->daddr)) {
			NAPI_GRO_CB(p)->same_flow = 0;
			continue;
		}

		NAPI_GRO_CB(p)->flush |=
			(iph->ttl ^ iph2->ttl) |
			((u16)(ntohs(iph2->id) + NAPI_GRO_CB(p)->count) ^ id);

		NAPI_GRO_CB(p)->flush |= flush;
	}

	NAPI_GRO_CB(skb)->flush |= flush;
	skb_gro_pull(skb, sizeof(*iph));
	skb_set_transport_header(skb, skb_gro_offset(skb));

	pp = ops->gro_receive(head, skb);

out_unlock:
	rcu_read_unlock();

out:
	NAPI_GRO_CB(skb)->flush |= flush;

	return pp;
}

static int inet_gro_complete(struct sk_buff *skb)
{
	const struct net_protocol *ops;
	struct iphdr *iph = ip_hdr(skb);
	int proto = iph->protocol & (MAX_INET_PROTOS - 1);
	int err = -ENOSYS;
	__be16 newlen = htons(skb->len - skb_network_offset(skb));

	csum_replace2(&iph->check, iph->tot_len, newlen);
	iph->tot_len = newlen;

	rcu_read_lock();
	ops = rcu_dereference(inet_protos[proto]);
	if (WARN_ON(!ops || !ops->gro_complete))
		goto out_unlock;

	err = ops->gro_complete(skb);

out_unlock:
	rcu_read_unlock();

	return err;
}

int inet_ctl_sock_create(struct sock **sk, unsigned short family,
			 unsigned short type, unsigned char protocol,
			 struct net *net)
{
	struct socket *sock;
	int rc = sock_create_kern(family, type, protocol, &sock);

	if (rc == 0) {
		*sk = sock->sk;
		(*sk)->sk_allocation = GFP_ATOMIC;
		 
		(*sk)->sk_prot->unhash(*sk);

		sk_change_net(*sk, net);
	}
	return rc;
}
EXPORT_SYMBOL_GPL(inet_ctl_sock_create);

unsigned long snmp_fold_field(void *mib[], int offt)
{
	unsigned long res = 0;
	int i;

	for_each_possible_cpu(i) {
		res += *(((unsigned long *) per_cpu_ptr(mib[0], i)) + offt);
		res += *(((unsigned long *) per_cpu_ptr(mib[1], i)) + offt);
	}
	return res;
}
EXPORT_SYMBOL_GPL(snmp_fold_field);

int snmp_mib_init(void *ptr[2], size_t mibsize)
{
	BUG_ON(ptr == NULL);
	ptr[0] = __alloc_percpu(mibsize, __alignof__(unsigned long long));
	if (!ptr[0])
		goto err0;
	ptr[1] = __alloc_percpu(mibsize, __alignof__(unsigned long long));
	if (!ptr[1])
		goto err1;
	return 0;
err1:
	free_percpu(ptr[0]);
	ptr[0] = NULL;
err0:
	return -ENOMEM;
}
EXPORT_SYMBOL_GPL(snmp_mib_init);

void snmp_mib_free(void *ptr[2])
{
	BUG_ON(ptr == NULL);
	free_percpu(ptr[0]);
	free_percpu(ptr[1]);
	ptr[0] = ptr[1] = NULL;
}
EXPORT_SYMBOL_GPL(snmp_mib_free);

#ifdef CONFIG_IP_MULTICAST
static const struct net_protocol igmp_protocol = {
	.handler =	igmp_rcv,
	.netns_ok =	1,
};
#endif

static const struct net_protocol tcp_protocol = {
	.handler =	tcp_v4_rcv,
	.err_handler =	tcp_v4_err,
	.gso_send_check = tcp_v4_gso_send_check,
	.gso_segment =	tcp_tso_segment,
	.gro_receive =	tcp4_gro_receive,
	.gro_complete =	tcp4_gro_complete,
	.no_policy =	1,
	.netns_ok =	1,
};

static const struct net_protocol udp_protocol = {
	.handler =	udp_rcv,
	.err_handler =	udp_err,
	.gso_send_check = udp4_ufo_send_check,
	.gso_segment = udp4_ufo_fragment,
	.no_policy =	1,
	.netns_ok =	1,
};

static const struct net_protocol icmp_protocol = {
	.handler =	icmp_rcv,
	.no_policy =	1,
	.netns_ok =	1,
};

static __net_init int ipv4_mib_init_net(struct net *net)
{
	if (snmp_mib_init((void **)net->mib.tcp_statistics,
			  sizeof(struct tcp_mib)) < 0)
		goto err_tcp_mib;
	if (snmp_mib_init((void **)net->mib.ip_statistics,
			  sizeof(struct ipstats_mib)) < 0)
		goto err_ip_mib;
	if (snmp_mib_init((void **)net->mib.net_statistics,
			  sizeof(struct linux_mib)) < 0)
		goto err_net_mib;
	if (snmp_mib_init((void **)net->mib.udp_statistics,
			  sizeof(struct udp_mib)) < 0)
		goto err_udp_mib;
	if (snmp_mib_init((void **)net->mib.udplite_statistics,
			  sizeof(struct udp_mib)) < 0)
		goto err_udplite_mib;
	if (snmp_mib_init((void **)net->mib.icmp_statistics,
			  sizeof(struct icmp_mib)) < 0)
		goto err_icmp_mib;
	if (snmp_mib_init((void **)net->mib.icmpmsg_statistics,
			  sizeof(struct icmpmsg_mib)) < 0)
		goto err_icmpmsg_mib;

	tcp_mib_init(net);
	return 0;

err_icmpmsg_mib:
	snmp_mib_free((void **)net->mib.icmp_statistics);
err_icmp_mib:
	snmp_mib_free((void **)net->mib.udplite_statistics);
err_udplite_mib:
	snmp_mib_free((void **)net->mib.udp_statistics);
err_udp_mib:
	snmp_mib_free((void **)net->mib.net_statistics);
err_net_mib:
	snmp_mib_free((void **)net->mib.ip_statistics);
err_ip_mib:
	snmp_mib_free((void **)net->mib.tcp_statistics);
err_tcp_mib:
	return -ENOMEM;
}

static __net_exit void ipv4_mib_exit_net(struct net *net)
{
	snmp_mib_free((void **)net->mib.icmpmsg_statistics);
	snmp_mib_free((void **)net->mib.icmp_statistics);
	snmp_mib_free((void **)net->mib.udplite_statistics);
	snmp_mib_free((void **)net->mib.udp_statistics);
	snmp_mib_free((void **)net->mib.net_statistics);
	snmp_mib_free((void **)net->mib.ip_statistics);
	snmp_mib_free((void **)net->mib.tcp_statistics);
}

static __net_initdata struct pernet_operations ipv4_mib_ops = {
	.init = ipv4_mib_init_net,
	.exit = ipv4_mib_exit_net,
};

static int __init init_ipv4_mibs(void)
{
	return register_pernet_subsys(&ipv4_mib_ops);
}

static int ipv4_proc_init(void);

static struct packet_type ip_packet_type __read_mostly = {
	.type = cpu_to_be16(ETH_P_IP),
	.func = ip_rcv,
	.gso_send_check = inet_gso_send_check,
	.gso_segment = inet_gso_segment,
	.gro_receive = inet_gro_receive,
	.gro_complete = inet_gro_complete,
};

static int __init inet_init(void)
{
	struct sk_buff *dummy_skb;
	struct inet_protosw *q;
	struct list_head *r;
	int rc = -EINVAL;

	BUILD_BUG_ON(sizeof(struct inet_skb_parm) > sizeof(dummy_skb->cb));

	rc = proto_register(&tcp_prot, 1);
	if (rc)
		goto out;

	rc = proto_register(&udp_prot, 1);
	if (rc)
		goto out_unregister_tcp_proto;

	rc = proto_register(&raw_prot, 1);
	if (rc)
		goto out_unregister_udp_proto;

	(void)sock_register(&inet_family_ops);

#ifdef CONFIG_SYSCTL
	ip_static_sysctl_init();
#endif

	if (inet_add_protocol(&icmp_protocol, IPPROTO_ICMP) < 0)
		printk(KERN_CRIT "inet_init: Cannot add ICMP protocol\n");
	if (inet_add_protocol(&udp_protocol, IPPROTO_UDP) < 0)
		printk(KERN_CRIT "inet_init: Cannot add UDP protocol\n");
	if (inet_add_protocol(&tcp_protocol, IPPROTO_TCP) < 0)
		printk(KERN_CRIT "inet_init: Cannot add TCP protocol\n");
#ifdef CONFIG_IP_MULTICAST
	if (inet_add_protocol(&igmp_protocol, IPPROTO_IGMP) < 0)
		printk(KERN_CRIT "inet_init: Cannot add IGMP protocol\n");
#endif

	for (r = &inetsw[0]; r < &inetsw[SOCK_MAX]; ++r)
		INIT_LIST_HEAD(r);

	for (q = inetsw_array; q < &inetsw_array[INETSW_ARRAY_LEN]; ++q)
		inet_register_protosw(q);

	arp_init();

	ip_init();

	tcp_v4_init();

	tcp_init();

	udp_init();

	udplite4_register();

	if (icmp_init() < 0)
		panic("Failed to create the ICMP control socket.\n");

#if defined(CONFIG_IP_MROUTE)
	if (ip_mr_init())
		printk(KERN_CRIT "inet_init: Cannot init ipv4 mroute\n");
#endif
	 
	if (init_ipv4_mibs())
		printk(KERN_CRIT "inet_init: Cannot init ipv4 mibs\n");

	ipv4_proc_init();

	ipfrag_init();

	dev_add_pack(&ip_packet_type);

	rc = 0;
out:
	return rc;
out_unregister_udp_proto:
	proto_unregister(&udp_prot);
out_unregister_tcp_proto:
	proto_unregister(&tcp_prot);
	goto out;
}

fs_initcall(inet_init);

#ifdef CONFIG_PROC_FS
static int __init ipv4_proc_init(void)
{
	int rc = 0;

	if (raw_proc_init())
		goto out_raw;
	if (tcp4_proc_init())
		goto out_tcp;
	if (udp4_proc_init())
		goto out_udp;
	if (ip_misc_proc_init())
		goto out_misc;
out:
	return rc;
out_misc:
	udp4_proc_exit();
out_udp:
	tcp4_proc_exit();
out_tcp:
	raw_proc_exit();
out_raw:
	rc = -ENOMEM;
	goto out;
}

#else  
static int __init ipv4_proc_init(void)
{
	return 0;
}
#endif  

MODULE_ALIAS_NETPROTO(PF_INET);
