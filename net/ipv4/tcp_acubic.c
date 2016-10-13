/*
 * TCP ACubic: a variation on CUBIC but with a more aggressive RTO.
 * You shouldn't do this on the open Internet; it could cause congestion
 * collapse problems.  It might be okay in a more controlled environment.
 */

#include <linux/mm.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <net/tcp.h>

static void (*old_cubic_init)(struct sock *sk);

static void acubic_init(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	old_cubic_init(sk);
	tp->is_aggressive_rto = 1;
}

static struct tcp_congestion_ops acubictcp __read_mostly;

static int __init acubic_register(void)
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
	memcpy(&acubictcp, ca, sizeof(struct tcp_congestion_ops));
	strncpy(acubictcp.name, "acubic", sizeof(acubictcp.name));
	old_cubic_init = ca->init;
	acubictcp.owner = THIS_MODULE;
	acubictcp.init = acubic_init;
	return tcp_register_congestion_control(&acubictcp);
}

static void __exit acubic_unregister(void)
{
	tcp_unregister_congestion_control(&acubictcp);
}

module_init(acubic_register);
module_exit(acubic_unregister);

MODULE_AUTHOR("Avery Pennarun");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ACubic TCP");
MODULE_VERSION("1.0");
