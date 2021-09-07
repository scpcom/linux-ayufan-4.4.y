 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/rtnetlink.h>
#include <net/rtnetlink.h>

static int numdummies = 1;

static int dummy_set_address(struct net_device *dev, void *p)
{
	struct sockaddr *sa = p;

	if (!is_valid_ether_addr(sa->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
	return 0;
}

static void set_multicast_list(struct net_device *dev)
{
}

static netdev_tx_t dummy_xmit(struct sk_buff *skb, struct net_device *dev)
{
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_NET_GIANFAR_FP
static int dummy_accept_fastpath(struct net_device *dev, struct dst_entry *dst)
{
	return -1;
}
#endif
#endif

static const struct net_device_ops dummy_netdev_ops = {
	.ndo_start_xmit		= dummy_xmit,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_multicast_list = set_multicast_list,
	.ndo_set_mac_address	= dummy_set_address,
#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_NET_GIANFAR_FP
	.ndo_accept_fastpath 	= dummy_accept_fastpath,
#endif
#endif
};

static void dummy_setup(struct net_device *dev)
{
	ether_setup(dev);

	dev->netdev_ops = &dummy_netdev_ops;
	dev->destructor = free_netdev;

	dev->tx_queue_len = 0;
	dev->flags |= IFF_NOARP;
	dev->flags &= ~IFF_MULTICAST;
	random_ether_addr(dev->dev_addr);
}
static int dummy_validate(struct nlattr *tb[], struct nlattr *data[])
{
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	return 0;
}

static struct rtnl_link_ops dummy_link_ops __read_mostly = {
	.kind		= "dummy",
	.setup		= dummy_setup,
	.validate	= dummy_validate,
};

module_param(numdummies, int, 0);
MODULE_PARM_DESC(numdummies, "Number of dummy pseudo devices");

static int __init dummy_init_one(void)
{
	struct net_device *dev_dummy;
	int err;

	dev_dummy = alloc_netdev(0, "dummy%d", dummy_setup);
	if (!dev_dummy)
		return -ENOMEM;

	err = dev_alloc_name(dev_dummy, dev_dummy->name);
	if (err < 0)
		goto err;

	dev_dummy->rtnl_link_ops = &dummy_link_ops;
	err = register_netdevice(dev_dummy);
	if (err < 0)
		goto err;
	return 0;

err:
	free_netdev(dev_dummy);
	return err;
}

static int __init dummy_init_module(void)
{
	int i, err = 0;

	rtnl_lock();
	err = __rtnl_link_register(&dummy_link_ops);

	for (i = 0; i < numdummies && !err; i++)
		err = dummy_init_one();
	if (err < 0)
		__rtnl_link_unregister(&dummy_link_ops);
	rtnl_unlock();

	return err;
}

static void __exit dummy_cleanup_module(void)
{
	rtnl_link_unregister(&dummy_link_ops);
}

module_init(dummy_init_module);
module_exit(dummy_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("dummy");
