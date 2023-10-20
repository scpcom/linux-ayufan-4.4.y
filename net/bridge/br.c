/*
 *	Generic parts
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/llc.h>
#include <net/llc.h>
#include <net/stp.h>

#include "br_private.h"

static const struct stp_proto br_stp_proto = {
	.rcv	= br_stp_rcv,
};

static struct pernet_operations br_net_ops = {
	.exit	= br_net_exit,
};

static int __init br_init(void)
{
	int err;

	err = stp_proto_register(&br_stp_proto);
	if (err < 0) {
		pr_err("bridge: can't register sap for STP\n");
		return err;
	}

	err = br_fdb_init();
	if (err)
		goto err_out;

	err = register_pernet_subsys(&br_net_ops);
	if (err)
		goto err_out1;

	err = br_netfilter_init();
	if (err)
		goto err_out2;

	err = register_netdevice_notifier(&br_device_notifier);
	if (err)
		goto err_out3;

	err = br_netlink_init();
	if (err)
		goto err_out4;

	brioctl_set(br_ioctl_deviceless_stub);

#if defined(CONFIG_ATM_LANE) || defined(CONFIG_ATM_LANE_MODULE)
	br_fdb_test_addr_hook = br_fdb_test_addr;
#endif

	return 0;
err_out4:
	unregister_netdevice_notifier(&br_device_notifier);
err_out3:
	br_netfilter_fini();
err_out2:
	unregister_pernet_subsys(&br_net_ops);
err_out1:
	br_fdb_fini();
err_out:
	stp_proto_unregister(&br_stp_proto);
	return err;
}

static void __exit br_deinit(void)
{
	stp_proto_unregister(&br_stp_proto);

	br_netlink_fini();
	unregister_netdevice_notifier(&br_device_notifier);
	brioctl_set(NULL);

	unregister_pernet_subsys(&br_net_ops);

	rcu_barrier(); /* Wait for completion of call_rcu()'s */

	br_netfilter_fini();
#if defined(CONFIG_ATM_LANE) || defined(CONFIG_ATM_LANE_MODULE)
	br_fdb_test_addr_hook = NULL;
#endif

	br_fdb_fini();
}

#if defined(CONFIG_ARCH_COMCERTO)
static ATOMIC_NOTIFIER_HEAD(brevent_notif_chain);

/**
 *	register_brevent_notifier - register a netevent notifier block
 *	@nb: notifier
 *
 *	Register a notifier to be called when a bridge event occurs.
 *	The notifier passed is linked into the kernel structures and must
 *	not be reused until it has been unregistered. A negative errno code
 *	is returned on a failure.
 */
int register_brevent_notifier(struct notifier_block *nb)
{
	int err;

	err = atomic_notifier_chain_register(&brevent_notif_chain, nb);
	return err;
}

/**
 *	unregister_brevent_notifier - unregister a netevent notifier block
 *	@nb: notifier
 *
 *	Unregister a notifier previously registered by
 *	register_neigh_notifier(). The notifier is unlinked into the
 *	kernel structures and may then be reused. A negative errno code
 *	is returned on a failure.
 */

int unregister_brevent_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&brevent_notif_chain, nb);
}

/**
 *	call_brevent_notifiers - call all netevent notifier blocks
 *      @val: value passed unmodified to notifier function
 *      @v:   pointer passed unmodified to notifier function
 *
 *	Call all neighbour notifier blocks.  Parameters and return value
 *	are as for notifier_call_chain().
 */

int call_brevent_notifiers(unsigned long val, void *v)
{
	return atomic_notifier_call_chain(&brevent_notif_chain, val, v);
}

EXPORT_SYMBOL_GPL(register_brevent_notifier);
EXPORT_SYMBOL_GPL(unregister_brevent_notifier);
EXPORT_SYMBOL_GPL(call_brevent_notifiers);
#endif

module_init(br_init)
module_exit(br_deinit)
MODULE_LICENSE("GPL");
MODULE_VERSION(BR_VERSION);
MODULE_ALIAS_RTNL_LINK("bridge");
