/*
 * TCP Transonic: a congestion controller that simply activates the TCP
 * layer's built in fixed-rate congestion control tweaks.
 *
 * Only use this with fixed-rate streams that won't try to saturate the
 * network, and only use it on a LAN.  It would be very rude to use this
 * on the open Internet (and it also probably wouldn't work).
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <net/tcp.h>

static void transonic_init(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	tp->is_rate_controlled = 1;
	tp->is_aggressive_rto = 1;
}

static void transonic_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
}

static u32 transonic_recalc_ssthresh(struct sock *sk)
{
	return 0;
}

static bool transonic_rate_control(struct sock *sk, unsigned int current_mss)
{
	static DEFINE_RATELIMIT_STATE(transonic_rate, 2 * HZ, 1);
	struct tcp_sock *tp = tcp_sk(sk);
	u64 rate = 50*1000*1000/8;  // max allowed streaming rate in bytes/sec
	u32 agg_time_us = 25*1000, min_rtt;

	if (!tp->is_rate_controlled)
		return false;

	/* Goal: cwnd of at least 2 aggregates. */
	min_rtt = max_t(u32, 2 * agg_time_us, tcp_min_rtt(tp));
	tp->snd_cwnd = DIV_ROUND_UP_ULL(2 * rate * min_rtt,
					USEC_PER_SEC * current_mss);
	/* avoid cwnd=1, just in case. */
	tp->snd_cwnd = max(tp->snd_cwnd, 2U);
	return true;
}

static struct tcp_congestion_ops transonic __read_mostly = {
	.init		= transonic_init,
	.cong_avoid	= transonic_cong_avoid,
	.ssthresh	= transonic_recalc_ssthresh,
	.rate_control	= transonic_rate_control,
	.owner		= THIS_MODULE,
	.name		= "transonic",
};

static int __init transonic_register(void)
{
	return tcp_register_congestion_control(&transonic);
}

static void __exit transonic_unregister(void)
{
	tcp_unregister_congestion_control(&transonic);
}

module_init(transonic_register);
module_exit(transonic_unregister);

MODULE_AUTHOR("Avery Pennarun");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Transonic TCP");
MODULE_VERSION("0.5");
