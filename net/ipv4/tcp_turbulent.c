/*
 * TCP Turbulent: just always use a large, fixed cwnd.  This overfills
 * buffers, causes packet loss, and is generally a disaster.  But under
 * certain very specific circumstances, this can sacrifice latency for
 * a tiny bit of improved throughput.
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <net/tcp.h>

/*
 * To handle 30 Mbps at 1000ms of latency with 1448 byte mss,
 * bandwidth-delay product = 30e6 / 8 * 1 / 1448 = 2589 packets.
 * Round to the next higher power of 2.  (This calculation assumes very low
 * packet loss, or you need more than one rtt to guarantee delivery.)
 */
#define MIN_SSTHRESH          2
#define MIN_CWND              4096

static void reset_values(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	tp->is_rate_controlled = 1;
	tp->is_aggressive_rto = 1;
	tp->snd_ssthresh = MIN_SSTHRESH;
	tp->snd_cwnd = tp->snd_cwnd_clamp = max_t(u64, MIN_CWND, tp->snd_cwnd);
}

static void turbulent_init(struct sock *sk)
{
	reset_values(sk);
}

static void turbulent_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
	reset_values(sk);
}

static u32 turbulent_recalc_ssthresh(struct sock *sk)
{
	return MIN_SSTHRESH;
}

static bool turbulent_rate_control(struct sock *sk, unsigned int current_mss)
{
	reset_values(sk);
	return true;
}

static struct tcp_congestion_ops turbulenttcp __read_mostly = {
	.init		= turbulent_init,
	.cong_avoid	= turbulent_cong_avoid,
	.ssthresh	= turbulent_recalc_ssthresh,
	.rate_control	= turbulent_rate_control,
	.owner		= THIS_MODULE,
	.name		= "turbulent",
};

static int __init turbulent_register(void)
{
	return tcp_register_congestion_control(&turbulenttcp);
}

static void __exit turbulent_unregister(void)
{
	tcp_unregister_congestion_control(&turbulenttcp);
}

module_init(turbulent_register);
module_exit(turbulent_unregister);

MODULE_AUTHOR("Avery Pennarun");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Turbulent TCP");
MODULE_VERSION("1.0");
