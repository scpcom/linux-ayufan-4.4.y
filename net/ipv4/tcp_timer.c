 
#include <linux/module.h>
#include <net/tcp.h>

int sysctl_tcp_syn_retries __read_mostly = TCP_SYN_RETRIES;
int sysctl_tcp_synack_retries __read_mostly = TCP_SYNACK_RETRIES;
int sysctl_tcp_keepalive_time __read_mostly = TCP_KEEPALIVE_TIME;
int sysctl_tcp_keepalive_probes __read_mostly = TCP_KEEPALIVE_PROBES;
int sysctl_tcp_keepalive_intvl __read_mostly = TCP_KEEPALIVE_INTVL;
int sysctl_tcp_retries1 __read_mostly = TCP_RETR1;
int sysctl_tcp_retries2 __read_mostly = TCP_RETR2;
int sysctl_tcp_orphan_retries __read_mostly;

static void tcp_write_timer(unsigned long);
static void tcp_delack_timer(unsigned long);
static void tcp_keepalive_timer (unsigned long data);

void tcp_init_xmit_timers(struct sock *sk)
{
	inet_csk_init_xmit_timers(sk, &tcp_write_timer, &tcp_delack_timer,
				  &tcp_keepalive_timer);
}

EXPORT_SYMBOL(tcp_init_xmit_timers);

static void tcp_write_err(struct sock *sk)
{
	sk->sk_err = sk->sk_err_soft ? : ETIMEDOUT;
	sk->sk_error_report(sk);

	tcp_done(sk);
	NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPABORTONTIMEOUT);
}

static int tcp_out_of_resources(struct sock *sk, int do_reset)
{
	struct tcp_sock *tp = tcp_sk(sk);
	int orphans = percpu_counter_read_positive(&tcp_orphan_count);

	if ((s32)(tcp_time_stamp - tp->lsndtime) > 2*TCP_RTO_MAX || !do_reset)
		orphans <<= 1;

	if (sk->sk_err_soft)
		orphans <<= 1;

	if (tcp_too_many_orphans(sk, orphans)) {
		if (net_ratelimit())
			printk(KERN_INFO "Out of socket memory\n");

		if ((s32)(tcp_time_stamp - tp->lsndtime) <= TCP_TIMEWAIT_LEN ||
		     
		    (!tp->snd_wnd && !tp->packets_out))
			do_reset = 1;
		if (do_reset)
			tcp_send_active_reset(sk, GFP_ATOMIC);
		tcp_done(sk);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPABORTONMEMORY);
		return 1;
	}
	return 0;
}

static int tcp_orphan_retries(struct sock *sk, int alive)
{
	int retries = sysctl_tcp_orphan_retries;  

	if (sk->sk_err_soft && !alive)
		retries = 0;

	if (retries == 0 && alive)
		retries = 8;
	return retries;
}

static void tcp_mtu_probing(struct inet_connection_sock *icsk, struct sock *sk)
{
	 
	if (sysctl_tcp_mtu_probing) {
		if (!icsk->icsk_mtup.enabled) {
			icsk->icsk_mtup.enabled = 1;
			tcp_sync_mss(sk, icsk->icsk_pmtu_cookie);
		} else {
			struct tcp_sock *tp = tcp_sk(sk);
			int mss;

			mss = tcp_mtu_to_mss(sk, icsk->icsk_mtup.search_low) >> 1;
			mss = min(sysctl_tcp_base_mss, mss);
			mss = max(mss, 68 - tp->tcp_header_len);
			icsk->icsk_mtup.search_low = tcp_mss_to_mtu(sk, mss);
			tcp_sync_mss(sk, icsk->icsk_pmtu_cookie);
		}
	}
}

static int tcp_write_timeout(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	int retry_until;
	bool do_reset;

	if ((1 << sk->sk_state) & (TCPF_SYN_SENT | TCPF_SYN_RECV)) {
		if (icsk->icsk_retransmits)
			dst_negative_advice(&sk->sk_dst_cache);
		retry_until = icsk->icsk_syn_retries ? : sysctl_tcp_syn_retries;
	} else {
		if (retransmits_timed_out(sk, sysctl_tcp_retries1)) {
			 
			tcp_mtu_probing(icsk, sk);

			dst_negative_advice(&sk->sk_dst_cache);
		}

		retry_until = sysctl_tcp_retries2;
		if (sock_flag(sk, SOCK_DEAD)) {
			const int alive = (icsk->icsk_rto < TCP_RTO_MAX);

			retry_until = tcp_orphan_retries(sk, alive);
			do_reset = alive ||
				   !retransmits_timed_out(sk, retry_until);

			if (tcp_out_of_resources(sk, do_reset))
				return 1;
		}
	}

	if (retransmits_timed_out(sk, retry_until)) {
		 
		tcp_write_err(sk);
		return 1;
	}
	return 0;
}

#ifdef CONFIG_SYNO_PLX_PORTING
extern int sk_wrong_state_for_release(struct sock *sk);
#endif

static void tcp_delack_timer(unsigned long data)
{
	struct sock *sk = (struct sock *)data;
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);

#ifdef CONFIG_SYNO_PLX_PORTING
	bh_lock_wsock(sk);

	if ((atomic_read(&sk->sk_refcnt) == 1) && sk_wrong_state_for_release(sk)) {
		WARN(1, "Attempt to release TCP socket %p in state %d\n", sk, sk->sk_state);
		bh_unlock_wsock(sk);
		return;
	}
#else
	bh_lock_sock(sk);
#endif

	if (sock_owned_by_user(sk)) {
		 
		icsk->icsk_ack.blocked = 1;
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_DELAYEDACKLOCKED);
		sk_reset_timer(sk, &icsk->icsk_delack_timer, jiffies + TCP_DELACK_MIN);
		goto out_unlock;
	}

	sk_mem_reclaim_partial(sk);

	if (sk->sk_state == TCP_CLOSE || !(icsk->icsk_ack.pending & ICSK_ACK_TIMER))
		goto out;

	if (time_after(icsk->icsk_ack.timeout, jiffies)) {
		sk_reset_timer(sk, &icsk->icsk_delack_timer, icsk->icsk_ack.timeout);
		goto out;
	}
	icsk->icsk_ack.pending &= ~ICSK_ACK_TIMER;

	if (!skb_queue_empty(&tp->ucopy.prequeue)) {
		struct sk_buff *skb;

		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPSCHEDULERFAILED);

		while ((skb = __skb_dequeue(&tp->ucopy.prequeue)) != NULL)
			sk_backlog_rcv(sk, skb);

		tp->ucopy.memory = 0;
	}

	if (inet_csk_ack_scheduled(sk)) {
		if (!icsk->icsk_ack.pingpong) {
			 
			icsk->icsk_ack.ato = min(icsk->icsk_ack.ato << 1, icsk->icsk_rto);
		} else {
			 
			icsk->icsk_ack.pingpong = 0;
			icsk->icsk_ack.ato      = TCP_ATO_MIN;
		}
		tcp_send_ack(sk);
		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_DELAYEDACKS);
	}
	TCP_CHECK_TIMER(sk);

out:
	if (tcp_memory_pressure)
		sk_mem_reclaim(sk);
out_unlock:
#ifdef CONFIG_SYNO_PLX_PORTING
	bh_unlock_wsock(sk);
#else
	bh_unlock_sock(sk);
#endif
	sock_put(sk);
}

static void tcp_probe_timer(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	int max_probes;

	if (tp->packets_out || !tcp_send_head(sk)) {
		icsk->icsk_probes_out = 0;
		return;
	}

	max_probes = sysctl_tcp_retries2;

	if (sock_flag(sk, SOCK_DEAD)) {
		const int alive = ((icsk->icsk_rto << icsk->icsk_backoff) < TCP_RTO_MAX);

		max_probes = tcp_orphan_retries(sk, alive);

		if (tcp_out_of_resources(sk, alive || icsk->icsk_probes_out <= max_probes))
			return;
	}

	if (icsk->icsk_probes_out > max_probes) {
		tcp_write_err(sk);
	} else {
		 
		tcp_send_probe0(sk);
	}
}

void tcp_retransmit_timer(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct inet_connection_sock *icsk = inet_csk(sk);

	if (!tp->packets_out)
		goto out;

	WARN_ON(tcp_write_queue_empty(sk));

	if (!tp->snd_wnd && !sock_flag(sk, SOCK_DEAD) &&
	    !((1 << sk->sk_state) & (TCPF_SYN_SENT | TCPF_SYN_RECV))) {
		 
#ifdef TCP_DEBUG
		struct inet_sock *inet = inet_sk(sk);
		if (sk->sk_family == AF_INET) {
			LIMIT_NETDEBUG(KERN_DEBUG "TCP: Peer %pI4:%u/%u unexpectedly shrunk window %u:%u (repaired)\n",
			       &inet->daddr, ntohs(inet->dport),
			       inet->num, tp->snd_una, tp->snd_nxt);
		}
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
		else if (sk->sk_family == AF_INET6) {
			struct ipv6_pinfo *np = inet6_sk(sk);
			LIMIT_NETDEBUG(KERN_DEBUG "TCP: Peer %pI6:%u/%u unexpectedly shrunk window %u:%u (repaired)\n",
			       &np->daddr, ntohs(inet->dport),
			       inet->num, tp->snd_una, tp->snd_nxt);
		}
#endif
#endif
		if (tcp_time_stamp - tp->rcv_tstamp > TCP_RTO_MAX) {
			tcp_write_err(sk);
			goto out;
		}
		tcp_enter_loss(sk, 0);
		tcp_retransmit_skb(sk, tcp_write_queue_head(sk));
		__sk_dst_reset(sk);
		goto out_reset_timer;
	}

	if (tcp_write_timeout(sk))
		goto out;

	if (icsk->icsk_retransmits == 0) {
		int mib_idx;

		if (icsk->icsk_ca_state == TCP_CA_Disorder) {
			if (tcp_is_sack(tp))
				mib_idx = LINUX_MIB_TCPSACKFAILURES;
			else
				mib_idx = LINUX_MIB_TCPRENOFAILURES;
		} else if (icsk->icsk_ca_state == TCP_CA_Recovery) {
			if (tcp_is_sack(tp))
				mib_idx = LINUX_MIB_TCPSACKRECOVERYFAIL;
			else
				mib_idx = LINUX_MIB_TCPRENORECOVERYFAIL;
		} else if (icsk->icsk_ca_state == TCP_CA_Loss) {
			mib_idx = LINUX_MIB_TCPLOSSFAILURES;
		} else {
			mib_idx = LINUX_MIB_TCPTIMEOUTS;
		}
		NET_INC_STATS_BH(sock_net(sk), mib_idx);
	}

	if (tcp_use_frto(sk)) {
		tcp_enter_frto(sk);
	} else {
		tcp_enter_loss(sk, 0);
	}

	if (tcp_retransmit_skb(sk, tcp_write_queue_head(sk)) > 0) {
		 
		if (!icsk->icsk_retransmits)
			icsk->icsk_retransmits = 1;
		inet_csk_reset_xmit_timer(sk, ICSK_TIME_RETRANS,
					  min(icsk->icsk_rto, TCP_RESOURCE_PROBE_INTERVAL),
					  TCP_RTO_MAX);
		goto out;
	}

	icsk->icsk_backoff++;
	icsk->icsk_retransmits++;

out_reset_timer:
	icsk->icsk_rto = min(icsk->icsk_rto << 1, TCP_RTO_MAX);
	inet_csk_reset_xmit_timer(sk, ICSK_TIME_RETRANS, icsk->icsk_rto, TCP_RTO_MAX);
	if (retransmits_timed_out(sk, sysctl_tcp_retries1 + 1))
		__sk_dst_reset(sk);

out:;
}

static void tcp_write_timer(unsigned long data)
{
	struct sock *sk = (struct sock *)data;
	struct inet_connection_sock *icsk = inet_csk(sk);
	int event;

#ifdef CONFIG_SYNO_PLX_PORTING
	bh_lock_wsock(sk);

	if ((atomic_read(&sk->sk_refcnt) == 1) && sk_wrong_state_for_release(sk)) {
		WARN(1, "Attempt to release TCP socket %p in state %d\n", sk, sk->sk_state);
		bh_unlock_wsock(sk);
		return;
	}
#else
	bh_lock_sock(sk);
#endif

	if (sock_owned_by_user(sk)) {
		 
		sk_reset_timer(sk, &icsk->icsk_retransmit_timer, jiffies + (HZ / 20));
		goto out_unlock;
	}

	if (sk->sk_state == TCP_CLOSE || !icsk->icsk_pending)
		goto out;

	if (time_after(icsk->icsk_timeout, jiffies)) {
		sk_reset_timer(sk, &icsk->icsk_retransmit_timer, icsk->icsk_timeout);
		goto out;
	}

	event = icsk->icsk_pending;
	icsk->icsk_pending = 0;

	switch (event) {
	case ICSK_TIME_RETRANS:
		tcp_retransmit_timer(sk);
		break;
	case ICSK_TIME_PROBE0:
		tcp_probe_timer(sk);
		break;
	}
	TCP_CHECK_TIMER(sk);

out:
	sk_mem_reclaim(sk);
out_unlock:
#ifdef CONFIG_SYNO_PLX_PORTING
	bh_unlock_wsock(sk);
#else
	bh_unlock_sock(sk);
#endif
	sock_put(sk);
}

static void tcp_synack_timer(struct sock *sk)
{
	inet_csk_reqsk_queue_prune(sk, TCP_SYNQ_INTERVAL,
				   TCP_TIMEOUT_INIT, TCP_RTO_MAX);
}

void tcp_set_keepalive(struct sock *sk, int val)
{
	if ((1 << sk->sk_state) & (TCPF_CLOSE | TCPF_LISTEN))
		return;

	if (val && !sock_flag(sk, SOCK_KEEPOPEN))
		inet_csk_reset_keepalive_timer(sk, keepalive_time_when(tcp_sk(sk)));
	else if (!val)
		inet_csk_delete_keepalive_timer(sk);
}

static void tcp_keepalive_timer (unsigned long data)
{
	struct sock *sk = (struct sock *) data;
	struct inet_connection_sock *icsk = inet_csk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	__u32 elapsed;

#ifdef CONFIG_SYNO_PLX_PORTING
	bh_lock_wsock(sk);

	if ((atomic_read(&sk->sk_refcnt) == 1) && sk_wrong_state_for_release(sk)) {
		WARN(1, "Attempt to release TCP socket %p in state %d\n", sk, sk->sk_state);
		bh_unlock_wsock(sk);
		return;
	}
#else
	bh_lock_sock(sk);
#endif

	if (sock_owned_by_user(sk)) {
		 
		inet_csk_reset_keepalive_timer (sk, HZ/20);
		goto out;
	}

	if (sk->sk_state == TCP_LISTEN) {
		tcp_synack_timer(sk);
		goto out;
	}

	if (sk->sk_state == TCP_FIN_WAIT2 && sock_flag(sk, SOCK_DEAD)) {
		if (tp->linger2 >= 0) {
			const int tmo = tcp_fin_time(sk) - TCP_TIMEWAIT_LEN;

			if (tmo > 0) {
				tcp_time_wait(sk, TCP_FIN_WAIT2, tmo);
				goto out;
			}
		}
		tcp_send_active_reset(sk, GFP_ATOMIC);
		goto death;
	}

	if (!sock_flag(sk, SOCK_KEEPOPEN) || sk->sk_state == TCP_CLOSE)
		goto out;

	elapsed = keepalive_time_when(tp);

	if (tp->packets_out || tcp_send_head(sk))
		goto resched;

	elapsed = tcp_time_stamp - tp->rcv_tstamp;

	if (elapsed >= keepalive_time_when(tp)) {
		if (icsk->icsk_probes_out >= keepalive_probes(tp)) {
			tcp_send_active_reset(sk, GFP_ATOMIC);
			tcp_write_err(sk);
			goto out;
		}
		if (tcp_write_wakeup(sk) <= 0) {
			icsk->icsk_probes_out++;
			elapsed = keepalive_intvl_when(tp);
		} else {
			 
			elapsed = TCP_RESOURCE_PROBE_INTERVAL;
		}
	} else {
		 
		elapsed = keepalive_time_when(tp) - elapsed;
	}

	TCP_CHECK_TIMER(sk);
	sk_mem_reclaim(sk);

resched:
	inet_csk_reset_keepalive_timer (sk, elapsed);
	goto out;

death:
	tcp_done(sk);

out:
#ifdef CONFIG_SYNO_PLX_PORTING
	bh_unlock_wsock(sk);
#else
	bh_unlock_sock(sk);
#endif
	sock_put(sk);
}
