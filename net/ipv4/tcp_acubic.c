/*
 * TCP Laminar: a variation on CUBIC but with a more aggressive RTO.
 * You shouldn't do this on the open Internet; it could cause congestion
 * collapse problems.  It might be okay in a more controlled environment.
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <net/tcp.h>

static void (*old_cubic_init)(struct sock *sk);

static void laminar_init(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	old_cubic_init(sk);
	tp->is_aggressive_rto = 1;
}

static struct tcp_congestion_ops laminartcp __read_mostly;

static int __init laminar_register(void)
{
	struct tcp_congestion_ops *ca;
	u32 key = tcp_ca_get_key_by_name("cubic");
	if (key == TCP_CA_UNSPEC)
		return -EEXIST;
	ca = tcp_ca_find_key(key);
	if (!ca)
		return -EINVAL;

	// Wrap CUBIC's implementation, but replace the name and init
	// function.  This way we can measure performance differences by
	// tracking just the name of the congestion algorithm.
	memcpy(&laminartcp, ca, sizeof(struct tcp_congestion_ops));
	strncpy(laminartcp.name, "laminar", sizeof(laminartcp.name));
	old_cubic_init = ca->init;
	laminartcp.owner = THIS_MODULE;
	laminartcp.init = laminar_init;
	return tcp_register_congestion_control(&laminartcp);
}

static void __exit laminar_unregister(void)
{
	tcp_unregister_congestion_control(&laminartcp);
}

module_init(laminar_register);
module_exit(laminar_unregister);

MODULE_AUTHOR("Avery Pennarun");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Laminar TCP");
MODULE_VERSION("1.0");
