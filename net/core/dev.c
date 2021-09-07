#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/bitops.h>
#include <linux/capability.h>
#include <linux/cpu.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/notifier.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <linux/rtnetlink.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/if_bridge.h>
#include <linux/if_macvlan.h>
#include <net/dst.h>
#include <net/pkt_sched.h>
#include <net/checksum.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/netpoll.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <net/wext.h>
#include <net/iw_handler.h>
#include <asm/current.h>
#include <linux/audit.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/if_arp.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <linux/jhash.h>
#include <linux/random.h>
#include <trace/events/napi.h>

#include "net-sysfs.h"

#if defined(MY_ABC_HERE) && defined(MY_ABC_HERE)
#include <linux/synobios.h>
#include <linux/pci.h>
extern unsigned int gSwitchDev;
extern char gDevPCIName[SYNO_MAX_SWITCHABLE_NET_DEVICE][SYNO_NET_DEVICE_ENCODING_LENGTH];
#endif

#define MAX_GRO_SKBS 8

#define GRO_MAX_HEAD (MAX_HEADER + 128)

#ifdef MY_ABC_HERE
static unsigned int str_to_hex( char ch )
{
	if( (ch >= '0') && (ch <= '9') )
		return( ch - '0' );

	if( (ch >= 'a') && (ch <= 'f') )
		return( ch - 'a' + 10 );

	if( (ch >= 'A') && (ch <= 'F') )
		return( ch - 'A' + 10 );

	return 0;
}

void convert_str_to_mac( char *source , char *dest )
{
	dest[0] = (str_to_hex( source[0] ) << 4) + str_to_hex( source[1] );
	dest[1] = (str_to_hex( source[2] ) << 4) + str_to_hex( source[3] );
	dest[2] = (str_to_hex( source[4] ) << 4) + str_to_hex( source[5] );
	dest[3] = (str_to_hex( source[6] ) << 4) + str_to_hex( source[7] );
	dest[4] = (str_to_hex( source[8] ) << 4) + str_to_hex( source[9] );
	dest[5] = (str_to_hex( source[10] ) << 4) + str_to_hex( source[11] );
}

#define SYNO_VENDOR_MAC_SUCCESS     0
#define SYNO_VENDOR_MAC_EMPTY       1
#define SYNO_VENDOR_MAC_FAIL        2
int syno_get_dev_vendor_mac(const char *szDev, char *szMac)
{
	extern unsigned char grgbLanMac[4][16];
	int err = SYNO_VENDOR_MAC_FAIL;

	if (!szMac || !szDev)
		goto ERR;

	if (!memcmp(szDev, "eth0", 4)) {
		if (!strcmp(grgbLanMac[0], "")) {
			err = SYNO_VENDOR_MAC_EMPTY;
			goto ERR;
		}
		convert_str_to_mac(grgbLanMac[0], szMac);
	} else if ( !memcmp(szDev, "eth1", 4) ) {
		if (!strcmp(grgbLanMac[1], "")) {
			err = SYNO_VENDOR_MAC_EMPTY;
			goto ERR;
		}
		convert_str_to_mac(grgbLanMac[1], szMac);
	} else if ( !memcmp(szDev, "eth2", 4) ) {
		if (!strcmp(grgbLanMac[2], "")) {
			err = SYNO_VENDOR_MAC_EMPTY;
			goto ERR;
		}
		convert_str_to_mac(grgbLanMac[2], szMac);
	} else if ( !memcmp(szDev, "eth3", 4) ) {
		if (!strcmp(grgbLanMac[3], "")) {
			err = SYNO_VENDOR_MAC_EMPTY;
			goto ERR;
		}
		convert_str_to_mac(grgbLanMac[3], szMac);
	} else {
		goto ERR;
	}

	err = SYNO_VENDOR_MAC_SUCCESS;
ERR:
	return err;
}
EXPORT_SYMBOL(syno_get_dev_vendor_mac);
#endif

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
static void dev_do_clear_fastroute(struct net_device *dev);
#endif
#endif
 
#define PTYPE_HASH_SIZE	(16)
#define PTYPE_HASH_MASK	(PTYPE_HASH_SIZE - 1)

static DEFINE_SPINLOCK(ptype_lock);
static struct list_head ptype_base[PTYPE_HASH_SIZE] __read_mostly;
static struct list_head ptype_all __read_mostly;	 

DEFINE_RWLOCK(dev_base_lock);
EXPORT_SYMBOL(dev_base_lock);

#define NETDEV_HASHBITS	8
#define NETDEV_HASHENTRIES (1 << NETDEV_HASHBITS)

static inline struct hlist_head *dev_name_hash(struct net *net, const char *name)
{
	unsigned hash = full_name_hash(name, strnlen(name, IFNAMSIZ));
	return &net->dev_name_head[hash & ((1 << NETDEV_HASHBITS) - 1)];
}

static inline struct hlist_head *dev_index_hash(struct net *net, int ifindex)
{
	return &net->dev_index_head[ifindex & ((1 << NETDEV_HASHBITS) - 1)];
}

static int list_netdevice(struct net_device *dev)
{
	struct net *net = dev_net(dev);

	ASSERT_RTNL();

	write_lock_bh(&dev_base_lock);
	list_add_tail(&dev->dev_list, &net->dev_base_head);
	hlist_add_head(&dev->name_hlist, dev_name_hash(net, dev->name));
	hlist_add_head(&dev->index_hlist, dev_index_hash(net, dev->ifindex));
	write_unlock_bh(&dev_base_lock);
	return 0;
}

static void unlist_netdevice(struct net_device *dev)
{
	ASSERT_RTNL();

	write_lock_bh(&dev_base_lock);
	list_del(&dev->dev_list);
	hlist_del(&dev->name_hlist);
	hlist_del(&dev->index_hlist);
	write_unlock_bh(&dev_base_lock);
}

static RAW_NOTIFIER_HEAD(netdev_chain);

DEFINE_PER_CPU(struct softnet_data, softnet_data);
EXPORT_PER_CPU_SYMBOL(softnet_data);

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
int netdev_fastroute;
EXPORT_SYMBOL(netdev_fastroute);

int netdev_fastroute_obstacles;
EXPORT_SYMBOL(netdev_fastroute_obstacles);
#endif

#ifdef CONFIG_GFAR_SW_PKT_STEERING
int rcv_pkt_steering;
EXPORT_SYMBOL(rcv_pkt_steering);
#endif
#endif

#ifdef CONFIG_LOCKDEP
 
static const unsigned short netdev_lock_type[] =
	{ARPHRD_NETROM, ARPHRD_ETHER, ARPHRD_EETHER, ARPHRD_AX25,
	 ARPHRD_PRONET, ARPHRD_CHAOS, ARPHRD_IEEE802, ARPHRD_ARCNET,
	 ARPHRD_APPLETLK, ARPHRD_DLCI, ARPHRD_ATM, ARPHRD_METRICOM,
	 ARPHRD_IEEE1394, ARPHRD_EUI64, ARPHRD_INFINIBAND, ARPHRD_SLIP,
	 ARPHRD_CSLIP, ARPHRD_SLIP6, ARPHRD_CSLIP6, ARPHRD_RSRVD,
	 ARPHRD_ADAPT, ARPHRD_ROSE, ARPHRD_X25, ARPHRD_HWX25,
	 ARPHRD_PPP, ARPHRD_CISCO, ARPHRD_LAPB, ARPHRD_DDCMP,
	 ARPHRD_RAWHDLC, ARPHRD_TUNNEL, ARPHRD_TUNNEL6, ARPHRD_FRAD,
	 ARPHRD_SKIP, ARPHRD_LOOPBACK, ARPHRD_LOCALTLK, ARPHRD_FDDI,
	 ARPHRD_BIF, ARPHRD_SIT, ARPHRD_IPDDP, ARPHRD_IPGRE,
	 ARPHRD_PIMREG, ARPHRD_HIPPI, ARPHRD_ASH, ARPHRD_ECONET,
	 ARPHRD_IRDA, ARPHRD_FCPP, ARPHRD_FCAL, ARPHRD_FCPL,
	 ARPHRD_FCFABRIC, ARPHRD_IEEE802_TR, ARPHRD_IEEE80211,
	 ARPHRD_IEEE80211_PRISM, ARPHRD_IEEE80211_RADIOTAP, ARPHRD_PHONET,
	 ARPHRD_PHONET_PIPE, ARPHRD_IEEE802154,
	 ARPHRD_VOID, ARPHRD_NONE};

static const char *const netdev_lock_name[] =
	{"_xmit_NETROM", "_xmit_ETHER", "_xmit_EETHER", "_xmit_AX25",
	 "_xmit_PRONET", "_xmit_CHAOS", "_xmit_IEEE802", "_xmit_ARCNET",
	 "_xmit_APPLETLK", "_xmit_DLCI", "_xmit_ATM", "_xmit_METRICOM",
	 "_xmit_IEEE1394", "_xmit_EUI64", "_xmit_INFINIBAND", "_xmit_SLIP",
	 "_xmit_CSLIP", "_xmit_SLIP6", "_xmit_CSLIP6", "_xmit_RSRVD",
	 "_xmit_ADAPT", "_xmit_ROSE", "_xmit_X25", "_xmit_HWX25",
	 "_xmit_PPP", "_xmit_CISCO", "_xmit_LAPB", "_xmit_DDCMP",
	 "_xmit_RAWHDLC", "_xmit_TUNNEL", "_xmit_TUNNEL6", "_xmit_FRAD",
	 "_xmit_SKIP", "_xmit_LOOPBACK", "_xmit_LOCALTLK", "_xmit_FDDI",
	 "_xmit_BIF", "_xmit_SIT", "_xmit_IPDDP", "_xmit_IPGRE",
	 "_xmit_PIMREG", "_xmit_HIPPI", "_xmit_ASH", "_xmit_ECONET",
	 "_xmit_IRDA", "_xmit_FCPP", "_xmit_FCAL", "_xmit_FCPL",
	 "_xmit_FCFABRIC", "_xmit_IEEE802_TR", "_xmit_IEEE80211",
	 "_xmit_IEEE80211_PRISM", "_xmit_IEEE80211_RADIOTAP", "_xmit_PHONET",
	 "_xmit_PHONET_PIPE", "_xmit_IEEE802154",
	 "_xmit_VOID", "_xmit_NONE"};

static struct lock_class_key netdev_xmit_lock_key[ARRAY_SIZE(netdev_lock_type)];
static struct lock_class_key netdev_addr_lock_key[ARRAY_SIZE(netdev_lock_type)];

static inline unsigned short netdev_lock_pos(unsigned short dev_type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(netdev_lock_type); i++)
		if (netdev_lock_type[i] == dev_type)
			return i;
	 
	return ARRAY_SIZE(netdev_lock_type) - 1;
}

static inline void netdev_set_xmit_lockdep_class(spinlock_t *lock,
						 unsigned short dev_type)
{
	int i;

	i = netdev_lock_pos(dev_type);
	lockdep_set_class_and_name(lock, &netdev_xmit_lock_key[i],
				   netdev_lock_name[i]);
}

static inline void netdev_set_addr_lockdep_class(struct net_device *dev)
{
	int i;

	i = netdev_lock_pos(dev->type);
	lockdep_set_class_and_name(&dev->addr_list_lock,
				   &netdev_addr_lock_key[i],
				   netdev_lock_name[i]);
}
#else
static inline void netdev_set_xmit_lockdep_class(spinlock_t *lock,
						 unsigned short dev_type)
{
}
static inline void netdev_set_addr_lockdep_class(struct net_device *dev)
{
}
#endif

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
static void dev_do_clear_fastroute(struct net_device *dev)
{
	if (dev->netdev_ops->ndo_accept_fastpath) {
		int i;

		for (i = 0; i <= NETDEV_FASTROUTE_HMASK; i++) {
			struct dst_entry *dst;

			write_lock_irq(&dev->fastpath_lock);
			dst = dev->fastpath[i];
			dev->fastpath[i] = NULL;
			write_unlock_irq(&dev->fastpath_lock);

			dst_release(dst);
		}
	}
}

void dev_clear_fastroute(struct net_device *dev)
{
	if (dev) {
		dev_do_clear_fastroute(dev);
	} else {
		read_lock(&dev_base_lock);
		for_each_netdev(dev_net(dev), dev)
			dev_do_clear_fastroute(dev);
		read_unlock(&dev_base_lock);
	}
}
#endif
#endif

void dev_add_pack(struct packet_type *pt)
{
	int hash;

	spin_lock_bh(&ptype_lock);
#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
	if (pt->af_packet_priv) {
		netdev_fastroute_obstacles++;
		dev_clear_fastroute(pt->dev);
	}
#endif
#endif
	if (pt->type == htons(ETH_P_ALL))
		list_add_rcu(&pt->list, &ptype_all);
	else {
		hash = ntohs(pt->type) & PTYPE_HASH_MASK;
		list_add_rcu(&pt->list, &ptype_base[hash]);
	}
	spin_unlock_bh(&ptype_lock);
}
EXPORT_SYMBOL(dev_add_pack);

void __dev_remove_pack(struct packet_type *pt)
{
	struct list_head *head;
	struct packet_type *pt1;

	spin_lock_bh(&ptype_lock);

	if (pt->type == htons(ETH_P_ALL))
		head = &ptype_all;
	else
		head = &ptype_base[ntohs(pt->type) & PTYPE_HASH_MASK];

	list_for_each_entry(pt1, head, list) {
		if (pt == pt1) {
#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
			if (pt->af_packet_priv)
				netdev_fastroute_obstacles--;
#endif
#endif
			list_del_rcu(&pt->list);
			goto out;
		}
	}

	printk(KERN_WARNING "dev_remove_pack: %p not found.\n", pt);
out:
	spin_unlock_bh(&ptype_lock);
}
EXPORT_SYMBOL(__dev_remove_pack);

void dev_remove_pack(struct packet_type *pt)
{
	__dev_remove_pack(pt);

	synchronize_net();
}
EXPORT_SYMBOL(dev_remove_pack);

static struct netdev_boot_setup dev_boot_setup[NETDEV_BOOT_SETUP_MAX];

static int netdev_boot_setup_add(char *name, struct ifmap *map)
{
	struct netdev_boot_setup *s;
	int i;

	s = dev_boot_setup;
	for (i = 0; i < NETDEV_BOOT_SETUP_MAX; i++) {
		if (s[i].name[0] == '\0' || s[i].name[0] == ' ') {
			memset(s[i].name, 0, sizeof(s[i].name));
			strlcpy(s[i].name, name, IFNAMSIZ);
			memcpy(&s[i].map, map, sizeof(s[i].map));
			break;
		}
	}

	return i >= NETDEV_BOOT_SETUP_MAX ? 0 : 1;
}

int netdev_boot_setup_check(struct net_device *dev)
{
	struct netdev_boot_setup *s = dev_boot_setup;
	int i;

	for (i = 0; i < NETDEV_BOOT_SETUP_MAX; i++) {
		if (s[i].name[0] != '\0' && s[i].name[0] != ' ' &&
		    !strcmp(dev->name, s[i].name)) {
			dev->irq 	= s[i].map.irq;
			dev->base_addr 	= s[i].map.base_addr;
			dev->mem_start 	= s[i].map.mem_start;
			dev->mem_end 	= s[i].map.mem_end;
			return 1;
		}
	}
	return 0;
}
EXPORT_SYMBOL(netdev_boot_setup_check);

unsigned long netdev_boot_base(const char *prefix, int unit)
{
	const struct netdev_boot_setup *s = dev_boot_setup;
	char name[IFNAMSIZ];
	int i;

	sprintf(name, "%s%d", prefix, unit);

	if (__dev_get_by_name(&init_net, name))
		return 1;

	for (i = 0; i < NETDEV_BOOT_SETUP_MAX; i++)
		if (!strcmp(name, s[i].name))
			return s[i].map.base_addr;
	return 0;
}

int __init netdev_boot_setup(char *str)
{
	int ints[5];
	struct ifmap map;

	str = get_options(str, ARRAY_SIZE(ints), ints);
	if (!str || !*str)
		return 0;

	memset(&map, 0, sizeof(map));
	if (ints[0] > 0)
		map.irq = ints[1];
	if (ints[0] > 1)
		map.base_addr = ints[2];
	if (ints[0] > 2)
		map.mem_start = ints[3];
	if (ints[0] > 3)
		map.mem_end = ints[4];

	return netdev_boot_setup_add(str, &map);
}

__setup("netdev=", netdev_boot_setup);

struct net_device *__dev_get_by_name(struct net *net, const char *name)
{
	struct hlist_node *p;

	hlist_for_each(p, dev_name_hash(net, name)) {
		struct net_device *dev
			= hlist_entry(p, struct net_device, name_hlist);
		if (!strncmp(dev->name, name, IFNAMSIZ))
			return dev;
	}
	return NULL;
}
EXPORT_SYMBOL(__dev_get_by_name);

struct net_device *dev_get_by_name(struct net *net, const char *name)
{
	struct net_device *dev;

	read_lock(&dev_base_lock);
	dev = __dev_get_by_name(net, name);
	if (dev)
		dev_hold(dev);
	read_unlock(&dev_base_lock);
	return dev;
}
EXPORT_SYMBOL(dev_get_by_name);

struct net_device *__dev_get_by_index(struct net *net, int ifindex)
{
	struct hlist_node *p;

	hlist_for_each(p, dev_index_hash(net, ifindex)) {
		struct net_device *dev
			= hlist_entry(p, struct net_device, index_hlist);
		if (dev->ifindex == ifindex)
			return dev;
	}
	return NULL;
}
EXPORT_SYMBOL(__dev_get_by_index);

struct net_device *dev_get_by_index(struct net *net, int ifindex)
{
	struct net_device *dev;

	read_lock(&dev_base_lock);
	dev = __dev_get_by_index(net, ifindex);
	if (dev)
		dev_hold(dev);
	read_unlock(&dev_base_lock);
	return dev;
}
EXPORT_SYMBOL(dev_get_by_index);

struct net_device *dev_getbyhwaddr(struct net *net, unsigned short type, char *ha)
{
	struct net_device *dev;

	ASSERT_RTNL();

	for_each_netdev(net, dev)
		if (dev->type == type &&
		    !memcmp(dev->dev_addr, ha, dev->addr_len))
			return dev;

	return NULL;
}
EXPORT_SYMBOL(dev_getbyhwaddr);

struct net_device *__dev_getfirstbyhwtype(struct net *net, unsigned short type)
{
	struct net_device *dev;

	ASSERT_RTNL();
	for_each_netdev(net, dev)
		if (dev->type == type)
			return dev;

	return NULL;
}
EXPORT_SYMBOL(__dev_getfirstbyhwtype);

struct net_device *dev_getfirstbyhwtype(struct net *net, unsigned short type)
{
	struct net_device *dev;

	rtnl_lock();
	dev = __dev_getfirstbyhwtype(net, type);
	if (dev)
		dev_hold(dev);
	rtnl_unlock();
	return dev;
}
EXPORT_SYMBOL(dev_getfirstbyhwtype);

struct net_device *dev_get_by_flags(struct net *net, unsigned short if_flags,
				    unsigned short mask)
{
	struct net_device *dev, *ret;

	ret = NULL;
	read_lock(&dev_base_lock);
	for_each_netdev(net, dev) {
		if (((dev->flags ^ if_flags) & mask) == 0) {
			dev_hold(dev);
			ret = dev;
			break;
		}
	}
	read_unlock(&dev_base_lock);
	return ret;
}
EXPORT_SYMBOL(dev_get_by_flags);

int dev_valid_name(const char *name)
{
	if (*name == '\0')
		return 0;
	if (strlen(name) >= IFNAMSIZ)
		return 0;
	if (!strcmp(name, ".") || !strcmp(name, ".."))
		return 0;

	while (*name) {
		if (*name == '/' || isspace(*name))
			return 0;
		name++;
	}
	return 1;
}
EXPORT_SYMBOL(dev_valid_name);

static int __dev_alloc_name(struct net *net, const char *name, char *buf)
{
	int i = 0;
	const char *p;
	const int max_netdevices = 8*PAGE_SIZE;
	unsigned long *inuse;
	struct net_device *d;

	p = strnchr(name, IFNAMSIZ-1, '%');
	if (p) {
		 
		if (p[1] != 'd' || strchr(p + 2, '%'))
			return -EINVAL;

		inuse = (unsigned long *) get_zeroed_page(GFP_ATOMIC);
		if (!inuse)
			return -ENOMEM;

		for_each_netdev(net, d) {
			if (!sscanf(d->name, name, &i))
				continue;
			if (i < 0 || i >= max_netdevices)
				continue;

			snprintf(buf, IFNAMSIZ, name, i);
			if (!strncmp(buf, d->name, IFNAMSIZ))
				set_bit(i, inuse);
		}

		i = find_first_zero_bit(inuse, max_netdevices);
		free_page((unsigned long) inuse);
	}

	snprintf(buf, IFNAMSIZ, name, i);
	if (!__dev_get_by_name(net, buf))
		return i;

	return -ENFILE;
}

int dev_alloc_name(struct net_device *dev, const char *name)
{
	char buf[IFNAMSIZ];
	struct net *net;
	int ret;

	BUG_ON(!dev_net(dev));
	net = dev_net(dev);
	ret = __dev_alloc_name(net, name, buf);
	if (ret >= 0)
		strlcpy(dev->name, buf, IFNAMSIZ);
	return ret;
}
EXPORT_SYMBOL(dev_alloc_name);

int dev_change_name(struct net_device *dev, const char *newname)
{
	char oldname[IFNAMSIZ];
	int err = 0;
	int ret;
	struct net *net;

	ASSERT_RTNL();
	BUG_ON(!dev_net(dev));

	net = dev_net(dev);
	if (dev->flags & IFF_UP)
		return -EBUSY;

	if (!dev_valid_name(newname))
		return -EINVAL;

	if (strncmp(newname, dev->name, IFNAMSIZ) == 0)
		return 0;

	memcpy(oldname, dev->name, IFNAMSIZ);

	if (strchr(newname, '%')) {
		err = dev_alloc_name(dev, newname);
		if (err < 0)
			return err;
	} else if (__dev_get_by_name(net, newname))
		return -EEXIST;
	else
		strlcpy(dev->name, newname, IFNAMSIZ);

rollback:
	 
	if (net == &init_net) {
		ret = device_rename(&dev->dev, dev->name);
		if (ret) {
			memcpy(dev->name, oldname, IFNAMSIZ);
			return ret;
		}
	}

	write_lock_bh(&dev_base_lock);
	hlist_del(&dev->name_hlist);
	hlist_add_head(&dev->name_hlist, dev_name_hash(net, dev->name));
	write_unlock_bh(&dev_base_lock);

	ret = call_netdevice_notifiers(NETDEV_CHANGENAME, dev);
	ret = notifier_to_errno(ret);

	if (ret) {
		 
		if (err >= 0) {
			err = ret;
			memcpy(dev->name, oldname, IFNAMSIZ);
			goto rollback;
		} else {
			printk(KERN_ERR
			       "%s: name change rollback failed: %d.\n",
			       dev->name, ret);
		}
	}

	return err;
}

int dev_set_alias(struct net_device *dev, const char *alias, size_t len)
{
	ASSERT_RTNL();

	if (len >= IFALIASZ)
		return -EINVAL;

	if (!len) {
		if (dev->ifalias) {
			kfree(dev->ifalias);
			dev->ifalias = NULL;
		}
		return 0;
	}

	dev->ifalias = krealloc(dev->ifalias, len + 1, GFP_KERNEL);
	if (!dev->ifalias)
		return -ENOMEM;

	strlcpy(dev->ifalias, alias, len+1);
	return len;
}

void netdev_features_change(struct net_device *dev)
{
	call_netdevice_notifiers(NETDEV_FEAT_CHANGE, dev);
}
EXPORT_SYMBOL(netdev_features_change);

void netdev_state_change(struct net_device *dev)
{
	if (dev->flags & IFF_UP) {
		call_netdevice_notifiers(NETDEV_CHANGE, dev);
		rtmsg_ifinfo(RTM_NEWLINK, dev, 0);
	}
}
EXPORT_SYMBOL(netdev_state_change);

void netdev_bonding_change(struct net_device *dev, unsigned long event)
{
	call_netdevice_notifiers(event, dev);
}
EXPORT_SYMBOL(netdev_bonding_change);

void dev_load(struct net *net, const char *name)
{
	struct net_device *dev;

	read_lock(&dev_base_lock);
	dev = __dev_get_by_name(net, name);
	read_unlock(&dev_base_lock);

	if (!dev && capable(CAP_NET_ADMIN))
		request_module("%s", name);
}
EXPORT_SYMBOL(dev_load);

int dev_open(struct net_device *dev)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int ret;

	ASSERT_RTNL();

	if (dev->flags & IFF_UP)
		return 0;

	if (!netif_device_present(dev))
		return -ENODEV;

	ret = call_netdevice_notifiers(NETDEV_PRE_UP, dev);
	ret = notifier_to_errno(ret);
	if (ret)
		return ret;

	set_bit(__LINK_STATE_START, &dev->state);

	if (ops->ndo_validate_addr)
		ret = ops->ndo_validate_addr(dev);

	if (!ret && ops->ndo_open)
		ret = ops->ndo_open(dev);

	if (ret)
		clear_bit(__LINK_STATE_START, &dev->state);
	else {
		 
		dev->flags |= IFF_UP;

		net_dmaengine_get();

		dev_set_rx_mode(dev);

		dev_activate(dev);

		call_netdevice_notifiers(NETDEV_UP, dev);
	}

	return ret;
}
EXPORT_SYMBOL(dev_open);

int dev_close(struct net_device *dev)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	ASSERT_RTNL();

	might_sleep();

	if (!(dev->flags & IFF_UP))
		return 0;

	call_netdevice_notifiers(NETDEV_GOING_DOWN, dev);

	clear_bit(__LINK_STATE_START, &dev->state);

	smp_mb__after_clear_bit();  

	dev_deactivate(dev);

	if (ops->ndo_stop)
		ops->ndo_stop(dev);

	dev->flags &= ~IFF_UP;

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
	dev_clear_fastroute(dev);
#endif
#endif
	 
	call_netdevice_notifiers(NETDEV_DOWN, dev);

	net_dmaengine_put();

	return 0;
}
EXPORT_SYMBOL(dev_close);

void dev_disable_lro(struct net_device *dev)
{
	if (dev->ethtool_ops && dev->ethtool_ops->get_flags &&
	    dev->ethtool_ops->set_flags) {
		u32 flags = dev->ethtool_ops->get_flags(dev);
		if (flags & ETH_FLAG_LRO) {
			flags &= ~ETH_FLAG_LRO;
			dev->ethtool_ops->set_flags(dev, flags);
		}
	}
	WARN_ON(dev->features & NETIF_F_LRO);
}
EXPORT_SYMBOL(dev_disable_lro);

static int dev_boot_phase = 1;

int register_netdevice_notifier(struct notifier_block *nb)
{
	struct net_device *dev;
	struct net_device *last;
	struct net *net;
	int err;

	rtnl_lock();
	err = raw_notifier_chain_register(&netdev_chain, nb);
	if (err)
		goto unlock;
	if (dev_boot_phase)
		goto unlock;
	for_each_net(net) {
		for_each_netdev(net, dev) {
			err = nb->notifier_call(nb, NETDEV_REGISTER, dev);
			err = notifier_to_errno(err);
			if (err)
				goto rollback;

			if (!(dev->flags & IFF_UP))
				continue;

			nb->notifier_call(nb, NETDEV_UP, dev);
		}
	}

unlock:
	rtnl_unlock();
	return err;

rollback:
	last = dev;
	for_each_net(net) {
		for_each_netdev(net, dev) {
			if (dev == last)
				break;

			if (dev->flags & IFF_UP) {
				nb->notifier_call(nb, NETDEV_GOING_DOWN, dev);
				nb->notifier_call(nb, NETDEV_DOWN, dev);
			}
			nb->notifier_call(nb, NETDEV_UNREGISTER, dev);
		}
	}

	raw_notifier_chain_unregister(&netdev_chain, nb);
	goto unlock;
}
EXPORT_SYMBOL(register_netdevice_notifier);

int unregister_netdevice_notifier(struct notifier_block *nb)
{
	int err;

	rtnl_lock();
	err = raw_notifier_chain_unregister(&netdev_chain, nb);
	rtnl_unlock();
	return err;
}
EXPORT_SYMBOL(unregister_netdevice_notifier);

int call_netdevice_notifiers(unsigned long val, struct net_device *dev)
{
	return raw_notifier_call_chain(&netdev_chain, val, dev);
}

static atomic_t netstamp_needed = ATOMIC_INIT(0);

void net_enable_timestamp(void)
{
	atomic_inc(&netstamp_needed);
}
EXPORT_SYMBOL(net_enable_timestamp);

void net_disable_timestamp(void)
{
	atomic_dec(&netstamp_needed);
}
EXPORT_SYMBOL(net_disable_timestamp);

static inline void net_timestamp(struct sk_buff *skb)
{
	if (atomic_read(&netstamp_needed))
		__net_timestamp(skb);
	else
		skb->tstamp.tv64 = 0;
}

static void dev_queue_xmit_nit(struct sk_buff *skb, struct net_device *dev)
{
	struct packet_type *ptype;

#ifdef CONFIG_NET_CLS_ACT
	if (!(skb->tstamp.tv64 && (G_TC_FROM(skb->tc_verd) & AT_INGRESS)))
		net_timestamp(skb);
#else
	net_timestamp(skb);
#endif

	rcu_read_lock();
	list_for_each_entry_rcu(ptype, &ptype_all, list) {
		 
		if ((ptype->dev == dev || !ptype->dev) &&
		    (ptype->af_packet_priv == NULL ||
		     (struct sock *)ptype->af_packet_priv != skb->sk)) {
			struct sk_buff *skb2 = skb_clone(skb, GFP_ATOMIC);
			if (!skb2)
				break;

			skb_reset_mac_header(skb2);

			if (skb_network_header(skb2) < skb2->data ||
			    skb2->network_header > skb2->tail) {
				if (net_ratelimit())
					printk(KERN_CRIT "protocol %04x is "
					       "buggy, dev %s\n",
					       skb2->protocol, dev->name);
				skb_reset_network_header(skb2);
			}

			skb2->transport_header = skb2->network_header;
			skb2->pkt_type = PACKET_OUTGOING;
			ptype->func(skb2, skb->dev, ptype, skb->dev);
		}
	}
	rcu_read_unlock();
}

static inline void __netif_reschedule(struct Qdisc *q)
{
	struct softnet_data *sd;
	unsigned long flags;

	local_irq_save(flags);
	sd = &__get_cpu_var(softnet_data);
	q->next_sched = sd->output_queue;
	sd->output_queue = q;
	raise_softirq_irqoff(NET_TX_SOFTIRQ);
	local_irq_restore(flags);
}

void __netif_schedule(struct Qdisc *q)
{
	if (!test_and_set_bit(__QDISC_STATE_SCHED, &q->state))
		__netif_reschedule(q);
}
EXPORT_SYMBOL(__netif_schedule);

void dev_kfree_skb_irq(struct sk_buff *skb)
{
	if (atomic_dec_and_test(&skb->users)) {
		struct softnet_data *sd;
		unsigned long flags;

		local_irq_save(flags);
		sd = &__get_cpu_var(softnet_data);
		skb->next = sd->completion_queue;
		sd->completion_queue = skb;
		raise_softirq_irqoff(NET_TX_SOFTIRQ);
		local_irq_restore(flags);
	}
}
EXPORT_SYMBOL(dev_kfree_skb_irq);

void dev_kfree_skb_any(struct sk_buff *skb)
{
	if (in_irq() || irqs_disabled())
		dev_kfree_skb_irq(skb);
	else
		dev_kfree_skb(skb);
}
EXPORT_SYMBOL(dev_kfree_skb_any);

void netif_device_detach(struct net_device *dev)
{
	if (test_and_clear_bit(__LINK_STATE_PRESENT, &dev->state) &&
	    netif_running(dev)) {
		netif_tx_stop_all_queues(dev);
	}
}
EXPORT_SYMBOL(netif_device_detach);

void netif_device_attach(struct net_device *dev)
{
	if (!test_and_set_bit(__LINK_STATE_PRESENT, &dev->state) &&
	    netif_running(dev)) {
		netif_tx_wake_all_queues(dev);
		__netdev_watchdog_up(dev);
	}
}
EXPORT_SYMBOL(netif_device_attach);

static bool can_checksum_protocol(unsigned long features, __be16 protocol)
{
	return ((features & NETIF_F_GEN_CSUM) ||
		((features & NETIF_F_IP_CSUM) &&
		 protocol == htons(ETH_P_IP)) ||
		((features & NETIF_F_IPV6_CSUM) &&
		 protocol == htons(ETH_P_IPV6)) ||
		((features & NETIF_F_FCOE_CRC) &&
		 protocol == htons(ETH_P_FCOE)));
}

static bool dev_can_checksum(struct net_device *dev, struct sk_buff *skb)
{
	if (can_checksum_protocol(dev->features, skb->protocol))
		return true;

	if (skb->protocol == htons(ETH_P_8021Q)) {
		struct vlan_ethhdr *veh = (struct vlan_ethhdr *)skb->data;
		if (can_checksum_protocol(dev->features & dev->vlan_features,
					  veh->h_vlan_encapsulated_proto))
			return true;
	}

	return false;
}

int skb_checksum_help(struct sk_buff *skb)
{
	__wsum csum;
	int ret = 0, offset;

	if (skb->ip_summed == CHECKSUM_COMPLETE)
		goto out_set_summed;

	if (unlikely(skb_shinfo(skb)->gso_size)) {
		 
		goto out_set_summed;
	}

	offset = skb->csum_start - skb_headroom(skb);
	BUG_ON(offset >= skb_headlen(skb));
	csum = skb_checksum(skb, offset, skb->len - offset, 0);

	offset += skb->csum_offset;
	BUG_ON(offset + sizeof(__sum16) > skb_headlen(skb));

	if (skb_cloned(skb) &&
	    !skb_clone_writable(skb, offset + sizeof(__sum16))) {
		ret = pskb_expand_head(skb, 0, 0, GFP_ATOMIC);
		if (ret)
			goto out;
	}

	*(__sum16 *)(skb->data + offset) = csum_fold(csum);
out_set_summed:
	skb->ip_summed = CHECKSUM_NONE;
out:
	return ret;
}
EXPORT_SYMBOL(skb_checksum_help);

struct sk_buff *skb_gso_segment(struct sk_buff *skb, int features)
{
	struct sk_buff *segs = ERR_PTR(-EPROTONOSUPPORT);
	struct packet_type *ptype;
	__be16 type = skb->protocol;
	int err;

	skb_reset_mac_header(skb);
	skb->mac_len = skb->network_header - skb->mac_header;
	__skb_pull(skb, skb->mac_len);

	if (unlikely(skb->ip_summed != CHECKSUM_PARTIAL)) {
		struct net_device *dev = skb->dev;
		struct ethtool_drvinfo info = {};

		if (dev && dev->ethtool_ops && dev->ethtool_ops->get_drvinfo)
			dev->ethtool_ops->get_drvinfo(dev, &info);

		WARN(1, "%s: caps=(0x%lx, 0x%lx) len=%d data_len=%d "
			"ip_summed=%d",
		     info.driver, dev ? dev->features : 0L,
		     skb->sk ? skb->sk->sk_route_caps : 0L,
		     skb->len, skb->data_len, skb->ip_summed);

		if (skb_header_cloned(skb) &&
		    (err = pskb_expand_head(skb, 0, 0, GFP_ATOMIC)))
			return ERR_PTR(err);
	}

	rcu_read_lock();
	list_for_each_entry_rcu(ptype,
			&ptype_base[ntohs(type) & PTYPE_HASH_MASK], list) {
		if (ptype->type == type && !ptype->dev && ptype->gso_segment) {
			if (unlikely(skb->ip_summed != CHECKSUM_PARTIAL)) {
				err = ptype->gso_send_check(skb);
				segs = ERR_PTR(err);
				if (err || skb_gso_ok(skb, features))
					break;
				__skb_push(skb, (skb->data -
						 skb_network_header(skb)));
			}
			segs = ptype->gso_segment(skb, features);
			break;
		}
	}
	rcu_read_unlock();

	__skb_push(skb, skb->data - skb_mac_header(skb));

	return segs;
}
EXPORT_SYMBOL(skb_gso_segment);

#ifdef CONFIG_BUG
void netdev_rx_csum_fault(struct net_device *dev)
{
	if (net_ratelimit()) {
		printk(KERN_ERR "%s: hw csum failure.\n",
			dev ? dev->name : "<unknown>");
		dump_stack();
	}
}
EXPORT_SYMBOL(netdev_rx_csum_fault);
#endif

static inline int illegal_highdma(struct net_device *dev, struct sk_buff *skb)
{
#ifdef CONFIG_HIGHMEM
	int i;

	if (dev->features & NETIF_F_HIGHDMA)
		return 0;

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++)
		if (PageHighMem(skb_shinfo(skb)->frags[i].page))
			return 1;

#endif
	return 0;
}

struct dev_gso_cb {
	void (*destructor)(struct sk_buff *skb);
};

#define DEV_GSO_CB(skb) ((struct dev_gso_cb *)(skb)->cb)

static void dev_gso_skb_destructor(struct sk_buff *skb)
{
	struct dev_gso_cb *cb;

	do {
		struct sk_buff *nskb = skb->next;

		skb->next = nskb->next;
		nskb->next = NULL;
		kfree_skb(nskb);
	} while (skb->next);

	cb = DEV_GSO_CB(skb);
	if (cb->destructor)
		cb->destructor(skb);
}

static int dev_gso_segment(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct sk_buff *segs;
	int features = dev->features & ~(illegal_highdma(dev, skb) ?
					 NETIF_F_SG : 0);

	segs = skb_gso_segment(skb, features);

	if (!segs)
		return 0;

	if (IS_ERR(segs))
		return PTR_ERR(segs);

	skb->next = segs;
	DEV_GSO_CB(skb)->destructor = skb->destructor;
	skb->destructor = dev_gso_skb_destructor;

	return 0;
}

int dev_hard_start_xmit(struct sk_buff *skb, struct net_device *dev,
			struct netdev_queue *txq)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int rc;

	if (likely(!skb->next)) {
		if (!list_empty(&ptype_all))
			dev_queue_xmit_nit(skb, dev);

		if (netif_needs_gso(dev, skb)) {
			if (unlikely(dev_gso_segment(skb)))
				goto out_kfree_skb;
			if (skb->next)
				goto gso;
		}

		if (dev->priv_flags & IFF_XMIT_DST_RELEASE)
			skb_dst_drop(skb);

		rc = ops->ndo_start_xmit(skb, dev);
		if (rc == NETDEV_TX_OK)
			txq_trans_update(txq);
		 
		return rc;
	}

gso:
	do {
		struct sk_buff *nskb = skb->next;

		skb->next = nskb->next;
		nskb->next = NULL;
		rc = ops->ndo_start_xmit(nskb, dev);
		if (unlikely(rc != NETDEV_TX_OK)) {
			nskb->next = skb->next;
			skb->next = nskb;
			return rc;
		}
		txq_trans_update(txq);
		if (unlikely(netif_tx_queue_stopped(txq) && skb->next))
			return NETDEV_TX_BUSY;
	} while (skb->next);

	skb->destructor = DEV_GSO_CB(skb)->destructor;

out_kfree_skb:
	kfree_skb(skb);
	return NETDEV_TX_OK;
}

static u32 skb_tx_hashrnd;

u16 skb_tx_hash(const struct net_device *dev, const struct sk_buff *skb)
{
	u32 hash;

	if (skb_rx_queue_recorded(skb)) {
		hash = skb_get_rx_queue(skb);
		while (unlikely(hash >= dev->real_num_tx_queues))
			hash -= dev->real_num_tx_queues;
		return hash;
	}

	if (skb->sk && skb->sk->sk_hash)
		hash = skb->sk->sk_hash;
	else
		hash = skb->protocol;

	hash = jhash_1word(hash, skb_tx_hashrnd);

	return (u16) (((u64) hash * dev->real_num_tx_queues) >> 32);
}
EXPORT_SYMBOL(skb_tx_hash);

static struct netdev_queue *dev_pick_tx(struct net_device *dev,
					struct sk_buff *skb)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	u16 queue_index = 0;

	if (ops->ndo_select_queue)
		queue_index = ops->ndo_select_queue(dev, skb);
	else if (dev->real_num_tx_queues > 1)
		queue_index = skb_tx_hash(dev, skb);

	skb_set_queue_mapping(skb, queue_index);
	return netdev_get_tx_queue(dev, queue_index);
}

static inline int __dev_xmit_skb(struct sk_buff *skb, struct Qdisc *q,
				 struct net_device *dev,
				 struct netdev_queue *txq)
{
	spinlock_t *root_lock = qdisc_lock(q);
	int rc;

	spin_lock(root_lock);
	if (unlikely(test_bit(__QDISC_STATE_DEACTIVATED, &q->state))) {
		kfree_skb(skb);
		rc = NET_XMIT_DROP;
	} else if ((q->flags & TCQ_F_CAN_BYPASS) && !qdisc_qlen(q) &&
		   !test_and_set_bit(__QDISC_STATE_RUNNING, &q->state)) {
		 
		__qdisc_update_bstats(q, skb->len);
		if (sch_direct_xmit(skb, q, dev, txq, root_lock))
			__qdisc_run(q);
		else
			clear_bit(__QDISC_STATE_RUNNING, &q->state);

		rc = NET_XMIT_SUCCESS;
	} else {
		rc = qdisc_enqueue_root(skb, q);
		qdisc_run(q);
	}
	spin_unlock(root_lock);

	return rc;
}

int dev_queue_xmit(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct netdev_queue *txq;
	struct Qdisc *q;
	int rc = -ENOMEM;

	if (netif_needs_gso(dev, skb))
		goto gso;

	if (skb_has_frags(skb) &&
	    !(dev->features & NETIF_F_FRAGLIST) &&
	    __skb_linearize(skb))
		goto out_kfree_skb;

	if (skb_shinfo(skb)->nr_frags &&
	    (!(dev->features & NETIF_F_SG) || illegal_highdma(dev, skb)) &&
	    __skb_linearize(skb))
		goto out_kfree_skb;

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		skb_set_transport_header(skb, skb->csum_start -
					      skb_headroom(skb));
		if (!dev_can_checksum(dev, skb) && skb_checksum_help(skb))
			goto out_kfree_skb;
	}

gso:
	 
	rcu_read_lock_bh();

	txq = dev_pick_tx(dev, skb);
	q = rcu_dereference(txq->qdisc);

#ifdef CONFIG_NET_CLS_ACT
	skb->tc_verd = SET_TC_AT(skb->tc_verd, AT_EGRESS);
#endif
	if (q->enqueue) {
		rc = __dev_xmit_skb(skb, q, dev, txq);
		goto out;
	}

	if (dev->flags & IFF_UP) {
		int cpu = smp_processor_id();  

		if (txq->xmit_lock_owner != cpu) {

			HARD_TX_LOCK(dev, txq, cpu);

			if (!netif_tx_queue_stopped(txq)) {
				rc = NET_XMIT_SUCCESS;
				if (!dev_hard_start_xmit(skb, dev, txq)) {
					HARD_TX_UNLOCK(dev, txq);
					goto out;
				}
			}
			HARD_TX_UNLOCK(dev, txq);
			if (net_ratelimit())
				printk(KERN_CRIT "Virtual device %s asks to "
				       "queue packet!\n", dev->name);
		} else {
			 
			if (net_ratelimit())
				printk(KERN_CRIT "Dead loop on virtual device "
				       "%s, fix it urgently!\n", dev->name);
		}
	}

	rc = -ENETDOWN;
	rcu_read_unlock_bh();

out_kfree_skb:
	kfree_skb(skb);
	return rc;
out:
	rcu_read_unlock_bh();
	return rc;
}
EXPORT_SYMBOL(dev_queue_xmit);

int netdev_max_backlog __read_mostly = 1000;
int netdev_budget __read_mostly = 300;
int weight_p __read_mostly = 64;             

DEFINE_PER_CPU(struct netif_rx_stats, netdev_rx_stat) = { 0, };

int netif_rx(struct sk_buff *skb)
{
	struct softnet_data *queue;
	unsigned long flags;

	if (netpoll_rx(skb))
		return NET_RX_DROP;

	if (!skb->tstamp.tv64)
		net_timestamp(skb);

	local_irq_save(flags);
	queue = &__get_cpu_var(softnet_data);

	__get_cpu_var(netdev_rx_stat).total++;
#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
	if (skb->pkt_type == PACKET_FASTROUTE)
		return dev_queue_xmit(skb);
#endif
#endif
	if (queue->input_pkt_queue.qlen <= netdev_max_backlog) {
		if (queue->input_pkt_queue.qlen) {
enqueue:
			__skb_queue_tail(&queue->input_pkt_queue, skb);
			local_irq_restore(flags);
			return NET_RX_SUCCESS;
		}

		napi_schedule(&queue->backlog);
		goto enqueue;
	}

	__get_cpu_var(netdev_rx_stat).dropped++;
	local_irq_restore(flags);

	kfree_skb(skb);
	return NET_RX_DROP;
}
EXPORT_SYMBOL(netif_rx);

int netif_rx_ni(struct sk_buff *skb)
{
	int err;

	preempt_disable();
	err = netif_rx(skb);
	if (local_softirq_pending())
		do_softirq();
	preempt_enable();

	return err;
}
EXPORT_SYMBOL(netif_rx_ni);

static void net_tx_action(struct softirq_action *h)
{
	struct softnet_data *sd = &__get_cpu_var(softnet_data);

	if (sd->completion_queue) {
		struct sk_buff *clist;

		local_irq_disable();
		clist = sd->completion_queue;
		sd->completion_queue = NULL;
		local_irq_enable();

		while (clist) {
			struct sk_buff *skb = clist;
			clist = clist->next;

			WARN_ON(atomic_read(&skb->users));
			__kfree_skb(skb);
		}
	}

	if (sd->output_queue) {
		struct Qdisc *head;

		local_irq_disable();
		head = sd->output_queue;
		sd->output_queue = NULL;
		local_irq_enable();

		while (head) {
			struct Qdisc *q = head;
			spinlock_t *root_lock;

			head = head->next_sched;

			root_lock = qdisc_lock(q);
			if (spin_trylock(root_lock)) {
				smp_mb__before_clear_bit();
				clear_bit(__QDISC_STATE_SCHED,
					  &q->state);
				qdisc_run(q);
				spin_unlock(root_lock);
			} else {
				if (!test_bit(__QDISC_STATE_DEACTIVATED,
					      &q->state)) {
					__netif_reschedule(q);
				} else {
					smp_mb__before_clear_bit();
					clear_bit(__QDISC_STATE_SCHED,
						  &q->state);
				}
			}
		}
	}
}

static inline int deliver_skb(struct sk_buff *skb,
			      struct packet_type *pt_prev,
			      struct net_device *orig_dev)
{
	atomic_inc(&skb->users);
	return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
}

#if defined(CONFIG_BRIDGE) || defined (CONFIG_BRIDGE_MODULE)

#if defined(CONFIG_ATM_LANE) || defined(CONFIG_ATM_LANE_MODULE)
 
int (*br_fdb_test_addr_hook)(struct net_device *dev,
			     unsigned char *addr) __read_mostly;
EXPORT_SYMBOL_GPL(br_fdb_test_addr_hook);
#endif

struct sk_buff *(*br_handle_frame_hook)(struct net_bridge_port *p,
					struct sk_buff *skb) __read_mostly;
EXPORT_SYMBOL_GPL(br_handle_frame_hook);

static inline struct sk_buff *handle_bridge(struct sk_buff *skb,
					    struct packet_type **pt_prev, int *ret,
					    struct net_device *orig_dev)
{
	struct net_bridge_port *port;

	if (skb->pkt_type == PACKET_LOOPBACK ||
	    (port = rcu_dereference(skb->dev->br_port)) == NULL)
		return skb;

	if (*pt_prev) {
		*ret = deliver_skb(skb, *pt_prev, orig_dev);
		*pt_prev = NULL;
	}

	return br_handle_frame_hook(port, skb);
}
#else
#define handle_bridge(skb, pt_prev, ret, orig_dev)	(skb)
#endif

#if defined(CONFIG_MACVLAN) || defined(CONFIG_MACVLAN_MODULE)
struct sk_buff *(*macvlan_handle_frame_hook)(struct sk_buff *skb) __read_mostly;
EXPORT_SYMBOL_GPL(macvlan_handle_frame_hook);

static inline struct sk_buff *handle_macvlan(struct sk_buff *skb,
					     struct packet_type **pt_prev,
					     int *ret,
					     struct net_device *orig_dev)
{
	if (skb->dev->macvlan_port == NULL)
		return skb;

	if (*pt_prev) {
		*ret = deliver_skb(skb, *pt_prev, orig_dev);
		*pt_prev = NULL;
	}
	return macvlan_handle_frame_hook(skb);
}
#else
#define handle_macvlan(skb, pt_prev, ret, orig_dev)	(skb)
#endif

#ifdef CONFIG_NET_CLS_ACT
 
static int ing_filter(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	u32 ttl = G_TC_RTTL(skb->tc_verd);
	struct netdev_queue *rxq;
	int result = TC_ACT_OK;
	struct Qdisc *q;

	if (MAX_RED_LOOP < ttl++) {
		printk(KERN_WARNING
		       "Redir loop detected Dropping packet (%d->%d)\n",
		       skb->iif, dev->ifindex);
		return TC_ACT_SHOT;
	}

	skb->tc_verd = SET_TC_RTTL(skb->tc_verd, ttl);
	skb->tc_verd = SET_TC_AT(skb->tc_verd, AT_INGRESS);

	rxq = &dev->rx_queue;

	q = rxq->qdisc;
	if (q != &noop_qdisc) {
		spin_lock(qdisc_lock(q));
		if (likely(!test_bit(__QDISC_STATE_DEACTIVATED, &q->state)))
			result = qdisc_enqueue_root(skb, q);
		spin_unlock(qdisc_lock(q));
	}

	return result;
}

static inline struct sk_buff *handle_ing(struct sk_buff *skb,
					 struct packet_type **pt_prev,
					 int *ret, struct net_device *orig_dev)
{
	if (skb->dev->rx_queue.qdisc == &noop_qdisc)
		goto out;

	if (*pt_prev) {
		*ret = deliver_skb(skb, *pt_prev, orig_dev);
		*pt_prev = NULL;
	} else {
		 
		skb->tc_verd = SET_TC_OK2MUNGE(skb->tc_verd);
	}

	switch (ing_filter(skb)) {
	case TC_ACT_SHOT:
	case TC_ACT_STOLEN:
		kfree_skb(skb);
		return NULL;
	}

out:
	skb->tc_verd = 0;
	return skb;
}
#endif

void netif_nit_deliver(struct sk_buff *skb)
{
	struct packet_type *ptype;

	if (list_empty(&ptype_all))
		return;

	skb_reset_network_header(skb);
	skb_reset_transport_header(skb);
	skb->mac_len = skb->network_header - skb->mac_header;

	rcu_read_lock();
	list_for_each_entry_rcu(ptype, &ptype_all, list) {
		if (!ptype->dev || ptype->dev == skb->dev)
			deliver_skb(skb, ptype, skb->dev);
	}
	rcu_read_unlock();
}

int netif_receive_skb(struct sk_buff *skb)
{
	struct packet_type *ptype, *pt_prev;
	struct net_device *orig_dev;
	struct net_device *null_or_orig;
	int ret = NET_RX_DROP;
	__be16 type;

	if (!skb->tstamp.tv64)
		net_timestamp(skb);

	if (skb->vlan_tci && vlan_hwaccel_do_receive(skb))
		return NET_RX_SUCCESS;

	if (netpoll_receive_skb(skb))
		return NET_RX_DROP;

	if (!skb->iif)
		skb->iif = skb->dev->ifindex;

	null_or_orig = NULL;
	orig_dev = skb->dev;
	if (orig_dev->master) {
		if (skb_bond_should_drop(skb))
			null_or_orig = orig_dev;  
		else
			skb->dev = orig_dev->master;
	}

	__get_cpu_var(netdev_rx_stat).total++;

	skb_reset_network_header(skb);
	skb_reset_transport_header(skb);
	skb->mac_len = skb->network_header - skb->mac_header;

	pt_prev = NULL;

	rcu_read_lock();

#ifdef CONFIG_NET_CLS_ACT
	if (skb->tc_verd & TC_NCLS) {
		skb->tc_verd = CLR_TC_NCLS(skb->tc_verd);
		goto ncls;
	}
#endif

	list_for_each_entry_rcu(ptype, &ptype_all, list) {
		if (ptype->dev == null_or_orig || ptype->dev == skb->dev ||
		    ptype->dev == orig_dev) {
			if (pt_prev)
				ret = deliver_skb(skb, pt_prev, orig_dev);
			pt_prev = ptype;
		}
	}

#ifdef CONFIG_NET_CLS_ACT
	skb = handle_ing(skb, &pt_prev, &ret, orig_dev);
	if (!skb)
		goto out;
ncls:
#endif

	skb = handle_bridge(skb, &pt_prev, &ret, orig_dev);
	if (!skb)
		goto out;
	skb = handle_macvlan(skb, &pt_prev, &ret, orig_dev);
	if (!skb)
		goto out;

	type = skb->protocol;
	list_for_each_entry_rcu(ptype,
			&ptype_base[ntohs(type) & PTYPE_HASH_MASK], list) {
		if (ptype->type == type &&
		    (ptype->dev == null_or_orig || ptype->dev == skb->dev ||
		     ptype->dev == orig_dev)) {
			if (pt_prev)
				ret = deliver_skb(skb, pt_prev, orig_dev);
			pt_prev = ptype;
		}
	}

	if (pt_prev) {
		ret = pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
	} else {
		kfree_skb(skb);
		 
		ret = NET_RX_DROP;
	}

out:
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL(netif_receive_skb);

static void flush_backlog(void *arg)
{
	struct net_device *dev = arg;
	struct softnet_data *queue = &__get_cpu_var(softnet_data);
	struct sk_buff *skb, *tmp;

	skb_queue_walk_safe(&queue->input_pkt_queue, skb, tmp)
		if (skb->dev == dev) {
			__skb_unlink(skb, &queue->input_pkt_queue);
			kfree_skb(skb);
		}
}

static int napi_gro_complete(struct sk_buff *skb)
{
	struct packet_type *ptype;
	__be16 type = skb->protocol;
	struct list_head *head = &ptype_base[ntohs(type) & PTYPE_HASH_MASK];
	int err = -ENOENT;

	if (NAPI_GRO_CB(skb)->count == 1) {
		skb_shinfo(skb)->gso_size = 0;
		goto out;
	}

	rcu_read_lock();
	list_for_each_entry_rcu(ptype, head, list) {
		if (ptype->type != type || ptype->dev || !ptype->gro_complete)
			continue;

		err = ptype->gro_complete(skb);
		break;
	}
	rcu_read_unlock();

	if (err) {
		WARN_ON(&ptype->list == head);
		kfree_skb(skb);
		return NET_RX_SUCCESS;
	}

out:
	return netif_receive_skb(skb);
}

void napi_gro_flush(struct napi_struct *napi)
{
	struct sk_buff *skb, *next;

	for (skb = napi->gro_list; skb; skb = next) {
		next = skb->next;
		skb->next = NULL;
		napi_gro_complete(skb);
	}

	napi->gro_count = 0;
	napi->gro_list = NULL;
}
EXPORT_SYMBOL(napi_gro_flush);

int dev_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	struct sk_buff **pp = NULL;
	struct packet_type *ptype;
	__be16 type = skb->protocol;
	struct list_head *head = &ptype_base[ntohs(type) & PTYPE_HASH_MASK];
	int same_flow;
	int mac_len;
	int ret;

	if (!(skb->dev->features & NETIF_F_GRO))
		goto normal;

	if (skb_is_gso(skb) || skb_has_frags(skb))
		goto normal;

	rcu_read_lock();
	list_for_each_entry_rcu(ptype, head, list) {
		if (ptype->type != type || ptype->dev || !ptype->gro_receive)
			continue;

		skb_set_network_header(skb, skb_gro_offset(skb));
		mac_len = skb->network_header - skb->mac_header;
		skb->mac_len = mac_len;
		NAPI_GRO_CB(skb)->same_flow = 0;
		NAPI_GRO_CB(skb)->flush = 0;
		NAPI_GRO_CB(skb)->free = 0;

		pp = ptype->gro_receive(&napi->gro_list, skb);
		break;
	}
	rcu_read_unlock();

	if (&ptype->list == head)
		goto normal;

	same_flow = NAPI_GRO_CB(skb)->same_flow;
	ret = NAPI_GRO_CB(skb)->free ? GRO_MERGED_FREE : GRO_MERGED;

	if (pp) {
		struct sk_buff *nskb = *pp;

		*pp = nskb->next;
		nskb->next = NULL;
		napi_gro_complete(nskb);
		napi->gro_count--;
	}

	if (same_flow)
		goto ok;

	if (NAPI_GRO_CB(skb)->flush || napi->gro_count >= MAX_GRO_SKBS)
		goto normal;

	napi->gro_count++;
	NAPI_GRO_CB(skb)->count = 1;
	skb_shinfo(skb)->gso_size = skb_gro_len(skb);
	skb->next = napi->gro_list;
	napi->gro_list = skb;
	ret = GRO_HELD;

pull:
	if (skb_headlen(skb) < skb_gro_offset(skb)) {
		int grow = skb_gro_offset(skb) - skb_headlen(skb);

		BUG_ON(skb->end - skb->tail < grow);

		memcpy(skb_tail_pointer(skb), NAPI_GRO_CB(skb)->frag0, grow);

		skb->tail += grow;
		skb->data_len -= grow;

		skb_shinfo(skb)->frags[0].page_offset += grow;
		skb_shinfo(skb)->frags[0].size -= grow;

		if (unlikely(!skb_shinfo(skb)->frags[0].size)) {
			put_page(skb_shinfo(skb)->frags[0].page);
			memmove(skb_shinfo(skb)->frags,
				skb_shinfo(skb)->frags + 1,
				--skb_shinfo(skb)->nr_frags);
		}
	}

ok:
	return ret;

normal:
	ret = GRO_NORMAL;
	goto pull;
}
EXPORT_SYMBOL(dev_gro_receive);

static int __napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	struct sk_buff *p;

	if (netpoll_rx_on(skb))
		return GRO_NORMAL;

	for (p = napi->gro_list; p; p = p->next) {
		NAPI_GRO_CB(p)->same_flow = (p->dev == skb->dev)
			&& !compare_ether_header(skb_mac_header(p),
						 skb_gro_mac_header(skb));
		NAPI_GRO_CB(p)->flush = 0;
	}

	return dev_gro_receive(napi, skb);
}

int napi_skb_finish(int ret, struct sk_buff *skb)
{
	int err = NET_RX_SUCCESS;

	switch (ret) {
	case GRO_NORMAL:
		return netif_receive_skb(skb);

	case GRO_DROP:
		err = NET_RX_DROP;
		 
	case GRO_MERGED_FREE:
		kfree_skb(skb);
		break;
	}

	return err;
}
EXPORT_SYMBOL(napi_skb_finish);

void skb_gro_reset_offset(struct sk_buff *skb)
{
	NAPI_GRO_CB(skb)->data_offset = 0;
	NAPI_GRO_CB(skb)->frag0 = NULL;
	NAPI_GRO_CB(skb)->frag0_len = 0;

	if (skb->mac_header == skb->tail &&
	    !PageHighMem(skb_shinfo(skb)->frags[0].page)) {
		NAPI_GRO_CB(skb)->frag0 =
			page_address(skb_shinfo(skb)->frags[0].page) +
			skb_shinfo(skb)->frags[0].page_offset;
		NAPI_GRO_CB(skb)->frag0_len = skb_shinfo(skb)->frags[0].size;
	}
}
EXPORT_SYMBOL(skb_gro_reset_offset);

int napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	skb_gro_reset_offset(skb);

	return napi_skb_finish(__napi_gro_receive(napi, skb), skb);
}
EXPORT_SYMBOL(napi_gro_receive);

void napi_reuse_skb(struct napi_struct *napi, struct sk_buff *skb)
{
	__skb_pull(skb, skb_headlen(skb));
	skb_reserve(skb, NET_IP_ALIGN - skb_headroom(skb));

	napi->skb = skb;
}
EXPORT_SYMBOL(napi_reuse_skb);

struct sk_buff *napi_get_frags(struct napi_struct *napi)
{
	struct net_device *dev = napi->dev;
	struct sk_buff *skb = napi->skb;

	if (!skb) {
		skb = netdev_alloc_skb(dev, GRO_MAX_HEAD + NET_IP_ALIGN);
		if (!skb)
			goto out;

		skb_reserve(skb, NET_IP_ALIGN);

		napi->skb = skb;
	}

out:
	return skb;
}
EXPORT_SYMBOL(napi_get_frags);

int napi_frags_finish(struct napi_struct *napi, struct sk_buff *skb, int ret)
{
	int err = NET_RX_SUCCESS;

	switch (ret) {
	case GRO_NORMAL:
	case GRO_HELD:
		skb->protocol = eth_type_trans(skb, skb->dev);

		if (ret == GRO_NORMAL)
			return netif_receive_skb(skb);

		skb_gro_pull(skb, -ETH_HLEN);
		break;

	case GRO_DROP:
		err = NET_RX_DROP;
		 
	case GRO_MERGED_FREE:
		napi_reuse_skb(napi, skb);
		break;
	}

	return err;
}
EXPORT_SYMBOL(napi_frags_finish);

struct sk_buff *napi_frags_skb(struct napi_struct *napi)
{
	struct sk_buff *skb = napi->skb;
	struct ethhdr *eth;
	unsigned int hlen;
	unsigned int off;

	napi->skb = NULL;

	skb_reset_mac_header(skb);
	skb_gro_reset_offset(skb);

	off = skb_gro_offset(skb);
	hlen = off + sizeof(*eth);
	eth = skb_gro_header_fast(skb, off);
	if (skb_gro_header_hard(skb, hlen)) {
		eth = skb_gro_header_slow(skb, hlen, off);
		if (unlikely(!eth)) {
			napi_reuse_skb(napi, skb);
			skb = NULL;
			goto out;
		}
	}

	skb_gro_pull(skb, sizeof(*eth));

	skb->protocol = eth->h_proto;

out:
	return skb;
}
EXPORT_SYMBOL(napi_frags_skb);

int napi_gro_frags(struct napi_struct *napi)
{
	struct sk_buff *skb = napi_frags_skb(napi);

	if (!skb)
		return NET_RX_DROP;

	return napi_frags_finish(napi, skb, __napi_gro_receive(napi, skb));
}
EXPORT_SYMBOL(napi_gro_frags);

static int process_backlog(struct napi_struct *napi, int quota)
{
	int work = 0;
	struct softnet_data *queue = &__get_cpu_var(softnet_data);
	unsigned long start_time = jiffies;

	napi->weight = weight_p;
	do {
		struct sk_buff *skb;

		local_irq_disable();
		skb = __skb_dequeue(&queue->input_pkt_queue);
		if (!skb) {
			__napi_complete(napi);
			local_irq_enable();
			break;
		}
		local_irq_enable();

		netif_receive_skb(skb);
	} while (++work < quota && jiffies == start_time);

	return work;
}

void __napi_schedule(struct napi_struct *n)
{
	unsigned long flags;

	local_irq_save(flags);
	list_add_tail(&n->poll_list, &__get_cpu_var(softnet_data).poll_list);
	__raise_softirq_irqoff(NET_RX_SOFTIRQ);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(__napi_schedule);

void __napi_complete(struct napi_struct *n)
{
	BUG_ON(!test_bit(NAPI_STATE_SCHED, &n->state));
	BUG_ON(n->gro_list);

	list_del(&n->poll_list);
	smp_mb__before_clear_bit();
	clear_bit(NAPI_STATE_SCHED, &n->state);
}
EXPORT_SYMBOL(__napi_complete);

void napi_complete(struct napi_struct *n)
{
	unsigned long flags;

	if (unlikely(test_bit(NAPI_STATE_NPSVC, &n->state)))
		return;

	napi_gro_flush(n);
	local_irq_save(flags);
	__napi_complete(n);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(napi_complete);

void netif_napi_add(struct net_device *dev, struct napi_struct *napi,
		    int (*poll)(struct napi_struct *, int), int weight)
{
	INIT_LIST_HEAD(&napi->poll_list);
	napi->gro_count = 0;
	napi->gro_list = NULL;
	napi->skb = NULL;
	napi->poll = poll;
	napi->weight = weight;
	list_add(&napi->dev_list, &dev->napi_list);
	napi->dev = dev;
#ifdef CONFIG_NETPOLL
	spin_lock_init(&napi->poll_lock);
	napi->poll_owner = -1;
#endif
	set_bit(NAPI_STATE_SCHED, &napi->state);
}
EXPORT_SYMBOL(netif_napi_add);

void netif_napi_del(struct napi_struct *napi)
{
	struct sk_buff *skb, *next;

	list_del_init(&napi->dev_list);
	napi_free_frags(napi);

	for (skb = napi->gro_list; skb; skb = next) {
		next = skb->next;
		skb->next = NULL;
		kfree_skb(skb);
	}

	napi->gro_list = NULL;
	napi->gro_count = 0;
}
EXPORT_SYMBOL(netif_napi_del);

static void net_rx_action(struct softirq_action *h)
{
	struct list_head *list = &__get_cpu_var(softnet_data).poll_list;
	unsigned long time_limit = jiffies + 2;
	int budget = netdev_budget;
	void *have;

	local_irq_disable();

	while (!list_empty(list)) {
		struct napi_struct *n;
		int work, weight;

		if (unlikely(budget <= 0 || time_after(jiffies, time_limit)))
			goto softnet_break;

		local_irq_enable();

		n = list_entry(list->next, struct napi_struct, poll_list);

		have = netpoll_poll_lock(n);

		weight = n->weight;

		work = 0;
		if (test_bit(NAPI_STATE_SCHED, &n->state)) {
			work = n->poll(n, weight);
			trace_napi_poll(n);
		}

		WARN_ON_ONCE(work > weight);

		budget -= work;

		local_irq_disable();

		if (unlikely(work == weight)) {
			if (unlikely(napi_disable_pending(n))) {
				local_irq_enable();
				napi_complete(n);
				local_irq_disable();
			} else
				list_move_tail(&n->poll_list, list);
		}

		netpoll_poll_unlock(have);
	}
out:
	local_irq_enable();

#ifdef CONFIG_NET_DMA
	 
	dma_issue_pending_all();
#endif

	return;

softnet_break:
	__get_cpu_var(netdev_rx_stat).time_squeeze++;
	__raise_softirq_irqoff(NET_RX_SOFTIRQ);
	goto out;
}

static gifconf_func_t *gifconf_list[NPROTO];

int register_gifconf(unsigned int family, gifconf_func_t *gifconf)
{
	if (family >= NPROTO)
		return -EINVAL;
	gifconf_list[family] = gifconf;
	return 0;
}
EXPORT_SYMBOL(register_gifconf);

static int dev_ifname(struct net *net, struct ifreq __user *arg)
{
	struct net_device *dev;
	struct ifreq ifr;

	if (copy_from_user(&ifr, arg, sizeof(struct ifreq)))
		return -EFAULT;

	read_lock(&dev_base_lock);
	dev = __dev_get_by_index(net, ifr.ifr_ifindex);
	if (!dev) {
		read_unlock(&dev_base_lock);
		return -ENODEV;
	}

	strcpy(ifr.ifr_name, dev->name);
	read_unlock(&dev_base_lock);

	if (copy_to_user(arg, &ifr, sizeof(struct ifreq)))
		return -EFAULT;
	return 0;
}

static int dev_ifconf(struct net *net, char __user *arg)
{
	struct ifconf ifc;
	struct net_device *dev;
	char __user *pos;
	int len;
	int total;
	int i;

	if (copy_from_user(&ifc, arg, sizeof(struct ifconf)))
		return -EFAULT;

	pos = ifc.ifc_buf;
	len = ifc.ifc_len;

	total = 0;
	for_each_netdev(net, dev) {
		for (i = 0; i < NPROTO; i++) {
			if (gifconf_list[i]) {
				int done;
				if (!pos)
					done = gifconf_list[i](dev, NULL, 0);
				else
					done = gifconf_list[i](dev, pos + total,
							       len - total);
				if (done < 0)
					return -EFAULT;
				total += done;
			}
		}
	}

	ifc.ifc_len = total;

	return copy_to_user(arg, &ifc, sizeof(struct ifconf)) ? -EFAULT : 0;
}

#ifdef CONFIG_PROC_FS
 
void *dev_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(dev_base_lock)
{
	struct net *net = seq_file_net(seq);
	loff_t off;
	struct net_device *dev;

	read_lock(&dev_base_lock);
	if (!*pos)
		return SEQ_START_TOKEN;

	off = 1;
	for_each_netdev(net, dev)
		if (off++ == *pos)
			return dev;

	return NULL;
}

void *dev_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct net *net = seq_file_net(seq);
	++*pos;
	return v == SEQ_START_TOKEN ?
		first_net_device(net) : next_net_device((struct net_device *)v);
}

void dev_seq_stop(struct seq_file *seq, void *v)
	__releases(dev_base_lock)
{
	read_unlock(&dev_base_lock);
}

static void dev_seq_printf_stats(struct seq_file *seq, struct net_device *dev)
{
	const struct net_device_stats *stats = dev_get_stats(dev);

	seq_printf(seq, "%6s:%8lu %7lu %4lu %4lu %4lu %5lu %10lu %9lu "
		   "%8lu %7lu %4lu %4lu %4lu %5lu %7lu %10lu\n",
		   dev->name, stats->rx_bytes, stats->rx_packets,
		   stats->rx_errors,
		   stats->rx_dropped + stats->rx_missed_errors,
		   stats->rx_fifo_errors,
		   stats->rx_length_errors + stats->rx_over_errors +
		    stats->rx_crc_errors + stats->rx_frame_errors,
		   stats->rx_compressed, stats->multicast,
		   stats->tx_bytes, stats->tx_packets,
		   stats->tx_errors, stats->tx_dropped,
		   stats->tx_fifo_errors, stats->collisions,
		   stats->tx_carrier_errors +
		    stats->tx_aborted_errors +
		    stats->tx_window_errors +
		    stats->tx_heartbeat_errors,
		   stats->tx_compressed);
}

static int dev_seq_show(struct seq_file *seq, void *v)
{
	if (v == SEQ_START_TOKEN)
		seq_puts(seq, "Inter-|   Receive                            "
			      "                    |  Transmit\n"
			      " face |bytes    packets errs drop fifo frame "
			      "compressed multicast|bytes    packets errs "
			      "drop fifo colls carrier compressed\n");
	else
		dev_seq_printf_stats(seq, v);
	return 0;
}

static struct netif_rx_stats *softnet_get_online(loff_t *pos)
{
	struct netif_rx_stats *rc = NULL;

	while (*pos < nr_cpu_ids)
		if (cpu_online(*pos)) {
			rc = &per_cpu(netdev_rx_stat, *pos);
			break;
		} else
			++*pos;
	return rc;
}

static void *softnet_seq_start(struct seq_file *seq, loff_t *pos)
{
	return softnet_get_online(pos);
}

static void *softnet_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	++*pos;
	return softnet_get_online(pos);
}

static void softnet_seq_stop(struct seq_file *seq, void *v)
{
}

static int softnet_seq_show(struct seq_file *seq, void *v)
{
	struct netif_rx_stats *s = v;

	seq_printf(seq, "%08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
		   s->total, s->dropped, s->time_squeeze, 0,
		   0, 0, 0, 0,  
		   s->cpu_collision);
	return 0;
}

static const struct seq_operations dev_seq_ops = {
	.start = dev_seq_start,
	.next  = dev_seq_next,
	.stop  = dev_seq_stop,
	.show  = dev_seq_show,
};

static int dev_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &dev_seq_ops,
			    sizeof(struct seq_net_private));
}

static const struct file_operations dev_seq_fops = {
	.owner	 = THIS_MODULE,
	.open    = dev_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};

static const struct seq_operations softnet_seq_ops = {
	.start = softnet_seq_start,
	.next  = softnet_seq_next,
	.stop  = softnet_seq_stop,
	.show  = softnet_seq_show,
};

static int softnet_seq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &softnet_seq_ops);
}

static const struct file_operations softnet_seq_fops = {
	.owner	 = THIS_MODULE,
	.open    = softnet_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release,
};

static void *ptype_get_idx(loff_t pos)
{
	struct packet_type *pt = NULL;
	loff_t i = 0;
	int t;

	list_for_each_entry_rcu(pt, &ptype_all, list) {
		if (i == pos)
			return pt;
		++i;
	}

	for (t = 0; t < PTYPE_HASH_SIZE; t++) {
		list_for_each_entry_rcu(pt, &ptype_base[t], list) {
			if (i == pos)
				return pt;
			++i;
		}
	}
	return NULL;
}

static void *ptype_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(RCU)
{
	rcu_read_lock();
	return *pos ? ptype_get_idx(*pos - 1) : SEQ_START_TOKEN;
}

static void *ptype_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct packet_type *pt;
	struct list_head *nxt;
	int hash;

	++*pos;
	if (v == SEQ_START_TOKEN)
		return ptype_get_idx(0);

	pt = v;
	nxt = pt->list.next;
	if (pt->type == htons(ETH_P_ALL)) {
		if (nxt != &ptype_all)
			goto found;
		hash = 0;
		nxt = ptype_base[0].next;
	} else
		hash = ntohs(pt->type) & PTYPE_HASH_MASK;

	while (nxt == &ptype_base[hash]) {
		if (++hash >= PTYPE_HASH_SIZE)
			return NULL;
		nxt = ptype_base[hash].next;
	}
found:
	return list_entry(nxt, struct packet_type, list);
}

static void ptype_seq_stop(struct seq_file *seq, void *v)
	__releases(RCU)
{
	rcu_read_unlock();
}

static int ptype_seq_show(struct seq_file *seq, void *v)
{
	struct packet_type *pt = v;

	if (v == SEQ_START_TOKEN)
		seq_puts(seq, "Type Device      Function\n");
	else if (pt->dev == NULL || dev_net(pt->dev) == seq_file_net(seq)) {
		if (pt->type == htons(ETH_P_ALL))
			seq_puts(seq, "ALL ");
		else
			seq_printf(seq, "%04x", ntohs(pt->type));

		seq_printf(seq, " %-8s %pF\n",
			   pt->dev ? pt->dev->name : "", pt->func);
	}

	return 0;
}

static const struct seq_operations ptype_seq_ops = {
	.start = ptype_seq_start,
	.next  = ptype_seq_next,
	.stop  = ptype_seq_stop,
	.show  = ptype_seq_show,
};

static int ptype_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &ptype_seq_ops,
			sizeof(struct seq_net_private));
}

static const struct file_operations ptype_seq_fops = {
	.owner	 = THIS_MODULE,
	.open    = ptype_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};

static int __net_init dev_proc_net_init(struct net *net)
{
	int rc = -ENOMEM;

	if (!proc_net_fops_create(net, "dev", S_IRUGO, &dev_seq_fops))
		goto out;
	if (!proc_net_fops_create(net, "softnet_stat", S_IRUGO, &softnet_seq_fops))
		goto out_dev;
	if (!proc_net_fops_create(net, "ptype", S_IRUGO, &ptype_seq_fops))
		goto out_softnet;

	if (wext_proc_init(net))
		goto out_ptype;
	rc = 0;
out:
	return rc;
out_ptype:
	proc_net_remove(net, "ptype");
out_softnet:
	proc_net_remove(net, "softnet_stat");
out_dev:
	proc_net_remove(net, "dev");
	goto out;
}

static void __net_exit dev_proc_net_exit(struct net *net)
{
	wext_proc_exit(net);

	proc_net_remove(net, "ptype");
	proc_net_remove(net, "softnet_stat");
	proc_net_remove(net, "dev");
}

static struct pernet_operations __net_initdata dev_proc_ops = {
	.init = dev_proc_net_init,
	.exit = dev_proc_net_exit,
};

static int __init dev_proc_init(void)
{
	return register_pernet_subsys(&dev_proc_ops);
}
#else
#define dev_proc_init() 0
#endif	 

int netdev_set_master(struct net_device *slave, struct net_device *master)
{
	struct net_device *old = slave->master;

	ASSERT_RTNL();

	if (master) {
		if (old)
			return -EBUSY;
		dev_hold(master);
	}

	slave->master = master;

	synchronize_net();

	if (old)
		dev_put(old);

	if (master)
		slave->flags |= IFF_SLAVE;
	else
		slave->flags &= ~IFF_SLAVE;

	rtmsg_ifinfo(RTM_NEWLINK, slave, IFF_SLAVE);
	return 0;
}
EXPORT_SYMBOL(netdev_set_master);

static void dev_change_rx_flags(struct net_device *dev, int flags)
{
	const struct net_device_ops *ops = dev->netdev_ops;

	if ((dev->flags & IFF_UP) && ops->ndo_change_rx_flags)
		ops->ndo_change_rx_flags(dev, flags);
}

static int __dev_set_promiscuity(struct net_device *dev, int inc)
{
	unsigned short old_flags = dev->flags;
	uid_t uid;
	gid_t gid;

	ASSERT_RTNL();

	dev->flags |= IFF_PROMISC;
	dev->promiscuity += inc;
	if (dev->promiscuity == 0) {
		 
		if (inc < 0)
			dev->flags &= ~IFF_PROMISC;
		else {
			dev->promiscuity -= inc;
			printk(KERN_WARNING "%s: promiscuity touches roof, "
				"set promiscuity failed, promiscuity feature "
				"of device might be broken.\n", dev->name);
			return -EOVERFLOW;
		}
	}
	if (dev->flags != old_flags) {
#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
		if (dev->flags & IFF_PROMISC) {
			netdev_fastroute_obstacles++;
			dev_clear_fastroute(dev);
		} else
			netdev_fastroute_obstacles--;
#endif
#endif
		printk(KERN_INFO "device %s %s promiscuous mode\n",
		       dev->name, (dev->flags & IFF_PROMISC) ? "entered" :
							       "left");
		if (audit_enabled) {
			current_uid_gid(&uid, &gid);
			audit_log(current->audit_context, GFP_ATOMIC,
				AUDIT_ANOM_PROMISCUOUS,
				"dev=%s prom=%d old_prom=%d auid=%u uid=%u gid=%u ses=%u",
				dev->name, (dev->flags & IFF_PROMISC),
				(old_flags & IFF_PROMISC),
				audit_get_loginuid(current),
				uid, gid,
				audit_get_sessionid(current));
		}

		dev_change_rx_flags(dev, IFF_PROMISC);
	}
	return 0;
}

int dev_set_promiscuity(struct net_device *dev, int inc)
{
	unsigned short old_flags = dev->flags;
	int err;

	err = __dev_set_promiscuity(dev, inc);
	if (err < 0)
		return err;
	if (dev->flags != old_flags)
		dev_set_rx_mode(dev);
	return err;
}
EXPORT_SYMBOL(dev_set_promiscuity);

int dev_set_allmulti(struct net_device *dev, int inc)
{
	unsigned short old_flags = dev->flags;

	ASSERT_RTNL();

	dev->flags |= IFF_ALLMULTI;
	dev->allmulti += inc;
	if (dev->allmulti == 0) {
		 
		if (inc < 0)
			dev->flags &= ~IFF_ALLMULTI;
		else {
			dev->allmulti -= inc;
			printk(KERN_WARNING "%s: allmulti touches roof, "
				"set allmulti failed, allmulti feature of "
				"device might be broken.\n", dev->name);
			return -EOVERFLOW;
		}
	}
	if (dev->flags ^ old_flags) {
		dev_change_rx_flags(dev, IFF_ALLMULTI);
		dev_set_rx_mode(dev);
	}
	return 0;
}
EXPORT_SYMBOL(dev_set_allmulti);

void __dev_set_rx_mode(struct net_device *dev)
{
	const struct net_device_ops *ops = dev->netdev_ops;

	if (!(dev->flags&IFF_UP))
		return;

	if (!netif_device_present(dev))
		return;

	if (ops->ndo_set_rx_mode)
		ops->ndo_set_rx_mode(dev);
	else {
		 
		if (dev->uc.count > 0 && !dev->uc_promisc) {
			__dev_set_promiscuity(dev, 1);
			dev->uc_promisc = 1;
		} else if (dev->uc.count == 0 && dev->uc_promisc) {
			__dev_set_promiscuity(dev, -1);
			dev->uc_promisc = 0;
		}

		if (ops->ndo_set_multicast_list)
			ops->ndo_set_multicast_list(dev);
	}
}

void dev_set_rx_mode(struct net_device *dev)
{
	netif_addr_lock_bh(dev);
	__dev_set_rx_mode(dev);
	netif_addr_unlock_bh(dev);
}

static int __hw_addr_add(struct netdev_hw_addr_list *list, unsigned char *addr,
			 int addr_len, unsigned char addr_type)
{
	struct netdev_hw_addr *ha;
	int alloc_size;

	if (addr_len > MAX_ADDR_LEN)
		return -EINVAL;

	list_for_each_entry(ha, &list->list, list) {
		if (!memcmp(ha->addr, addr, addr_len) &&
		    ha->type == addr_type) {
			ha->refcount++;
			return 0;
		}
	}

	alloc_size = sizeof(*ha);
	if (alloc_size < L1_CACHE_BYTES)
		alloc_size = L1_CACHE_BYTES;
	ha = kmalloc(alloc_size, GFP_ATOMIC);
	if (!ha)
		return -ENOMEM;
	memcpy(ha->addr, addr, addr_len);
	ha->type = addr_type;
	ha->refcount = 1;
	ha->synced = false;
	list_add_tail_rcu(&ha->list, &list->list);
	list->count++;
	return 0;
}

static void ha_rcu_free(struct rcu_head *head)
{
	struct netdev_hw_addr *ha;

	ha = container_of(head, struct netdev_hw_addr, rcu_head);
	kfree(ha);
}

static int __hw_addr_del(struct netdev_hw_addr_list *list, unsigned char *addr,
			 int addr_len, unsigned char addr_type)
{
	struct netdev_hw_addr *ha;

	list_for_each_entry(ha, &list->list, list) {
		if (!memcmp(ha->addr, addr, addr_len) &&
		    (ha->type == addr_type || !addr_type)) {
			if (--ha->refcount)
				return 0;
			list_del_rcu(&ha->list);
			call_rcu(&ha->rcu_head, ha_rcu_free);
			list->count--;
			return 0;
		}
	}
	return -ENOENT;
}

static int __hw_addr_add_multiple(struct netdev_hw_addr_list *to_list,
				  struct netdev_hw_addr_list *from_list,
				  int addr_len,
				  unsigned char addr_type)
{
	int err;
	struct netdev_hw_addr *ha, *ha2;
	unsigned char type;

	list_for_each_entry(ha, &from_list->list, list) {
		type = addr_type ? addr_type : ha->type;
		err = __hw_addr_add(to_list, ha->addr, addr_len, type);
		if (err)
			goto unroll;
	}
	return 0;

unroll:
	list_for_each_entry(ha2, &from_list->list, list) {
		if (ha2 == ha)
			break;
		type = addr_type ? addr_type : ha2->type;
		__hw_addr_del(to_list, ha2->addr, addr_len, type);
	}
	return err;
}

static void __hw_addr_del_multiple(struct netdev_hw_addr_list *to_list,
				   struct netdev_hw_addr_list *from_list,
				   int addr_len,
				   unsigned char addr_type)
{
	struct netdev_hw_addr *ha;
	unsigned char type;

	list_for_each_entry(ha, &from_list->list, list) {
		type = addr_type ? addr_type : ha->type;
		__hw_addr_del(to_list, ha->addr, addr_len, addr_type);
	}
}

static int __hw_addr_sync(struct netdev_hw_addr_list *to_list,
			  struct netdev_hw_addr_list *from_list,
			  int addr_len)
{
	int err = 0;
	struct netdev_hw_addr *ha, *tmp;

	list_for_each_entry_safe(ha, tmp, &from_list->list, list) {
		if (!ha->synced) {
			err = __hw_addr_add(to_list, ha->addr,
					    addr_len, ha->type);
			if (err)
				break;
			ha->synced = true;
			ha->refcount++;
		} else if (ha->refcount == 1) {
			__hw_addr_del(to_list, ha->addr, addr_len, ha->type);
			__hw_addr_del(from_list, ha->addr, addr_len, ha->type);
		}
	}
	return err;
}

static void __hw_addr_unsync(struct netdev_hw_addr_list *to_list,
			     struct netdev_hw_addr_list *from_list,
			     int addr_len)
{
	struct netdev_hw_addr *ha, *tmp;

	list_for_each_entry_safe(ha, tmp, &from_list->list, list) {
		if (ha->synced) {
			__hw_addr_del(to_list, ha->addr,
				      addr_len, ha->type);
			ha->synced = false;
			__hw_addr_del(from_list, ha->addr,
				      addr_len, ha->type);
		}
	}
}

static void __hw_addr_flush(struct netdev_hw_addr_list *list)
{
	struct netdev_hw_addr *ha, *tmp;

	list_for_each_entry_safe(ha, tmp, &list->list, list) {
		list_del_rcu(&ha->list);
		call_rcu(&ha->rcu_head, ha_rcu_free);
	}
	list->count = 0;
}

static void __hw_addr_init(struct netdev_hw_addr_list *list)
{
	INIT_LIST_HEAD(&list->list);
	list->count = 0;
}

static void dev_addr_flush(struct net_device *dev)
{
	 
	__hw_addr_flush(&dev->dev_addrs);
	dev->dev_addr = NULL;
}

static int dev_addr_init(struct net_device *dev)
{
	unsigned char addr[MAX_ADDR_LEN];
	struct netdev_hw_addr *ha;
	int err;

	__hw_addr_init(&dev->dev_addrs);
	memset(addr, 0, sizeof(addr));
	err = __hw_addr_add(&dev->dev_addrs, addr, sizeof(addr),
			    NETDEV_HW_ADDR_T_LAN);
	if (!err) {
		 
		ha = list_first_entry(&dev->dev_addrs.list,
				      struct netdev_hw_addr, list);
		dev->dev_addr = ha->addr;
	}
	return err;
}

int dev_addr_add(struct net_device *dev, unsigned char *addr,
		 unsigned char addr_type)
{
	int err;

	ASSERT_RTNL();

	err = __hw_addr_add(&dev->dev_addrs, addr, dev->addr_len, addr_type);
	if (!err)
		call_netdevice_notifiers(NETDEV_CHANGEADDR, dev);
	return err;
}
EXPORT_SYMBOL(dev_addr_add);

int dev_addr_del(struct net_device *dev, unsigned char *addr,
		 unsigned char addr_type)
{
	int err;
	struct netdev_hw_addr *ha;

	ASSERT_RTNL();

	ha = list_first_entry(&dev->dev_addrs.list,
			      struct netdev_hw_addr, list);
	if (ha->addr == dev->dev_addr && ha->refcount == 1)
		return -ENOENT;

	err = __hw_addr_del(&dev->dev_addrs, addr, dev->addr_len,
			    addr_type);
	if (!err)
		call_netdevice_notifiers(NETDEV_CHANGEADDR, dev);
	return err;
}
EXPORT_SYMBOL(dev_addr_del);

int dev_addr_add_multiple(struct net_device *to_dev,
			  struct net_device *from_dev,
			  unsigned char addr_type)
{
	int err;

	ASSERT_RTNL();

	if (from_dev->addr_len != to_dev->addr_len)
		return -EINVAL;
	err = __hw_addr_add_multiple(&to_dev->dev_addrs, &from_dev->dev_addrs,
				     to_dev->addr_len, addr_type);
	if (!err)
		call_netdevice_notifiers(NETDEV_CHANGEADDR, to_dev);
	return err;
}
EXPORT_SYMBOL(dev_addr_add_multiple);

int dev_addr_del_multiple(struct net_device *to_dev,
			  struct net_device *from_dev,
			  unsigned char addr_type)
{
	ASSERT_RTNL();

	if (from_dev->addr_len != to_dev->addr_len)
		return -EINVAL;
	__hw_addr_del_multiple(&to_dev->dev_addrs, &from_dev->dev_addrs,
			       to_dev->addr_len, addr_type);
	call_netdevice_notifiers(NETDEV_CHANGEADDR, to_dev);
	return 0;
}
EXPORT_SYMBOL(dev_addr_del_multiple);

int __dev_addr_delete(struct dev_addr_list **list, int *count,
		      void *addr, int alen, int glbl)
{
	struct dev_addr_list *da;

	for (; (da = *list) != NULL; list = &da->next) {
		if (memcmp(da->da_addr, addr, da->da_addrlen) == 0 &&
		    alen == da->da_addrlen) {
			if (glbl) {
				int old_glbl = da->da_gusers;
				da->da_gusers = 0;
				if (old_glbl == 0)
					break;
			}
			if (--da->da_users)
				return 0;

			*list = da->next;
			kfree(da);
			(*count)--;
			return 0;
		}
	}
	return -ENOENT;
}

int __dev_addr_add(struct dev_addr_list **list, int *count,
		   void *addr, int alen, int glbl)
{
	struct dev_addr_list *da;

	for (da = *list; da != NULL; da = da->next) {
		if (memcmp(da->da_addr, addr, da->da_addrlen) == 0 &&
		    da->da_addrlen == alen) {
			if (glbl) {
				int old_glbl = da->da_gusers;
				da->da_gusers = 1;
				if (old_glbl)
					return 0;
			}
			da->da_users++;
			return 0;
		}
	}

	da = kzalloc(sizeof(*da), GFP_ATOMIC);
	if (da == NULL)
		return -ENOMEM;
	memcpy(da->da_addr, addr, alen);
	da->da_addrlen = alen;
	da->da_users = 1;
	da->da_gusers = glbl ? 1 : 0;
	da->next = *list;
	*list = da;
	(*count)++;
	return 0;
}

int dev_unicast_delete(struct net_device *dev, void *addr)
{
	int err;

	ASSERT_RTNL();

	netif_addr_lock_bh(dev);
	err = __hw_addr_del(&dev->uc, addr, dev->addr_len,
			    NETDEV_HW_ADDR_T_UNICAST);
	if (!err)
		__dev_set_rx_mode(dev);
	netif_addr_unlock_bh(dev);
	return err;
}
EXPORT_SYMBOL(dev_unicast_delete);

int dev_unicast_add(struct net_device *dev, void *addr)
{
	int err;

	ASSERT_RTNL();

	netif_addr_lock_bh(dev);
	err = __hw_addr_add(&dev->uc, addr, dev->addr_len,
			    NETDEV_HW_ADDR_T_UNICAST);
	if (!err)
		__dev_set_rx_mode(dev);
	netif_addr_unlock_bh(dev);
	return err;
}
EXPORT_SYMBOL(dev_unicast_add);

int __dev_addr_sync(struct dev_addr_list **to, int *to_count,
		    struct dev_addr_list **from, int *from_count)
{
	struct dev_addr_list *da, *next;
	int err = 0;

	da = *from;
	while (da != NULL) {
		next = da->next;
		if (!da->da_synced) {
			err = __dev_addr_add(to, to_count,
					     da->da_addr, da->da_addrlen, 0);
			if (err < 0)
				break;
			da->da_synced = 1;
			da->da_users++;
		} else if (da->da_users == 1) {
			__dev_addr_delete(to, to_count,
					  da->da_addr, da->da_addrlen, 0);
			__dev_addr_delete(from, from_count,
					  da->da_addr, da->da_addrlen, 0);
		}
		da = next;
	}
	return err;
}
EXPORT_SYMBOL_GPL(__dev_addr_sync);

void __dev_addr_unsync(struct dev_addr_list **to, int *to_count,
		       struct dev_addr_list **from, int *from_count)
{
	struct dev_addr_list *da, *next;

	da = *from;
	while (da != NULL) {
		next = da->next;
		if (da->da_synced) {
			__dev_addr_delete(to, to_count,
					  da->da_addr, da->da_addrlen, 0);
			da->da_synced = 0;
			__dev_addr_delete(from, from_count,
					  da->da_addr, da->da_addrlen, 0);
		}
		da = next;
	}
}
EXPORT_SYMBOL_GPL(__dev_addr_unsync);

int dev_unicast_sync(struct net_device *to, struct net_device *from)
{
	int err = 0;

	if (to->addr_len != from->addr_len)
		return -EINVAL;

	netif_addr_lock_bh(to);
	err = __hw_addr_sync(&to->uc, &from->uc, to->addr_len);
	if (!err)
		__dev_set_rx_mode(to);
	netif_addr_unlock_bh(to);
	return err;
}
EXPORT_SYMBOL(dev_unicast_sync);

void dev_unicast_unsync(struct net_device *to, struct net_device *from)
{
	if (to->addr_len != from->addr_len)
		return;

	netif_addr_lock_bh(from);
	netif_addr_lock(to);
	__hw_addr_unsync(&to->uc, &from->uc, to->addr_len);
	__dev_set_rx_mode(to);
	netif_addr_unlock(to);
	netif_addr_unlock_bh(from);
}
EXPORT_SYMBOL(dev_unicast_unsync);

static void dev_unicast_flush(struct net_device *dev)
{
	netif_addr_lock_bh(dev);
	__hw_addr_flush(&dev->uc);
	netif_addr_unlock_bh(dev);
}

static void dev_unicast_init(struct net_device *dev)
{
	__hw_addr_init(&dev->uc);
}

static void __dev_addr_discard(struct dev_addr_list **list)
{
	struct dev_addr_list *tmp;

	while (*list != NULL) {
		tmp = *list;
		*list = tmp->next;
		if (tmp->da_users > tmp->da_gusers)
			printk("__dev_addr_discard: address leakage! "
			       "da_users=%d\n", tmp->da_users);
		kfree(tmp);
	}
}

static void dev_addr_discard(struct net_device *dev)
{
	netif_addr_lock_bh(dev);

	__dev_addr_discard(&dev->mc_list);
	dev->mc_count = 0;

	netif_addr_unlock_bh(dev);
}

unsigned dev_get_flags(const struct net_device *dev)
{
	unsigned flags;

	flags = (dev->flags & ~(IFF_PROMISC |
				IFF_ALLMULTI |
				IFF_RUNNING |
				IFF_LOWER_UP |
				IFF_DORMANT)) |
		(dev->gflags & (IFF_PROMISC |
				IFF_ALLMULTI));

	if (netif_running(dev)) {
		if (netif_oper_up(dev))
			flags |= IFF_RUNNING;
		if (netif_carrier_ok(dev))
			flags |= IFF_LOWER_UP;
		if (netif_dormant(dev))
			flags |= IFF_DORMANT;
	}

	return flags;
}
EXPORT_SYMBOL(dev_get_flags);

int dev_change_flags(struct net_device *dev, unsigned flags)
{
	int ret, changes;
	int old_flags = dev->flags;

	ASSERT_RTNL();

	dev->flags = (flags & (IFF_DEBUG | IFF_NOTRAILERS | IFF_NOARP |
			       IFF_DYNAMIC | IFF_MULTICAST | IFF_PORTSEL |
			       IFF_AUTOMEDIA)) |
		     (dev->flags & (IFF_UP | IFF_VOLATILE | IFF_PROMISC |
				    IFF_ALLMULTI));

	if ((old_flags ^ flags) & IFF_MULTICAST)
		dev_change_rx_flags(dev, IFF_MULTICAST);

	dev_set_rx_mode(dev);

	ret = 0;
	if ((old_flags ^ flags) & IFF_UP) {	 
		ret = ((old_flags & IFF_UP) ? dev_close : dev_open)(dev);

		if (!ret)
			dev_set_rx_mode(dev);
	}

	if (dev->flags & IFF_UP &&
	    ((old_flags ^ dev->flags) & ~(IFF_UP | IFF_PROMISC | IFF_ALLMULTI |
					  IFF_VOLATILE)))
		call_netdevice_notifiers(NETDEV_CHANGE, dev);

	if ((flags ^ dev->gflags) & IFF_PROMISC) {
		int inc = (flags & IFF_PROMISC) ? 1 : -1;

		dev->gflags ^= IFF_PROMISC;
		dev_set_promiscuity(dev, inc);
	}

	if ((flags ^ dev->gflags) & IFF_ALLMULTI) {
		int inc = (flags & IFF_ALLMULTI) ? 1 : -1;

		dev->gflags ^= IFF_ALLMULTI;
		dev_set_allmulti(dev, inc);
	}

	changes = (old_flags ^ dev->flags) & ~(IFF_UP | IFF_RUNNING);
	if (changes)
		rtmsg_ifinfo(RTM_NEWLINK, dev, changes);

	return ret;
}
EXPORT_SYMBOL(dev_change_flags);

int dev_set_mtu(struct net_device *dev, int new_mtu)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int err;

	if (new_mtu == dev->mtu)
		return 0;

	if (new_mtu < 0)
		return -EINVAL;

	if (!netif_device_present(dev))
		return -ENODEV;

	err = 0;
	if (ops->ndo_change_mtu)
		err = ops->ndo_change_mtu(dev, new_mtu);
	else
		dev->mtu = new_mtu;

	if (!err && dev->flags & IFF_UP)
		call_netdevice_notifiers(NETDEV_CHANGEMTU, dev);
	return err;
}
EXPORT_SYMBOL(dev_set_mtu);

int dev_set_mac_address(struct net_device *dev, struct sockaddr *sa)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int err;

	if (!ops->ndo_set_mac_address)
		return -EOPNOTSUPP;
	if (sa->sa_family != dev->type)
		return -EINVAL;
	if (!netif_device_present(dev))
		return -ENODEV;
	err = ops->ndo_set_mac_address(dev, sa);
	if (!err)
		call_netdevice_notifiers(NETDEV_CHANGEADDR, dev);
	return err;
}
EXPORT_SYMBOL(dev_set_mac_address);

static int dev_ifsioc_locked(struct net *net, struct ifreq *ifr, unsigned int cmd)
{
	int err;
	struct net_device *dev = __dev_get_by_name(net, ifr->ifr_name);

	if (!dev)
		return -ENODEV;

	switch (cmd) {
	case SIOCGIFFLAGS:	 
		ifr->ifr_flags = (short) dev_get_flags(dev);
		return 0;

	case SIOCGIFMETRIC:	 
		ifr->ifr_metric = 0;
		return 0;

	case SIOCGIFMTU:	 
		ifr->ifr_mtu = dev->mtu;
		return 0;

	case SIOCGIFHWADDR:
		if (!dev->addr_len)
			memset(ifr->ifr_hwaddr.sa_data, 0, sizeof ifr->ifr_hwaddr.sa_data);
		else
			memcpy(ifr->ifr_hwaddr.sa_data, dev->dev_addr,
			       min(sizeof ifr->ifr_hwaddr.sa_data, (size_t) dev->addr_len));
		ifr->ifr_hwaddr.sa_family = dev->type;
		return 0;

	case SIOCGIFSLAVE:
		err = -EINVAL;
		break;

	case SIOCGIFMAP:
		ifr->ifr_map.mem_start = dev->mem_start;
		ifr->ifr_map.mem_end   = dev->mem_end;
		ifr->ifr_map.base_addr = dev->base_addr;
		ifr->ifr_map.irq       = dev->irq;
		ifr->ifr_map.dma       = dev->dma;
		ifr->ifr_map.port      = dev->if_port;
		return 0;

	case SIOCGIFINDEX:
		ifr->ifr_ifindex = dev->ifindex;
		return 0;

	case SIOCGIFTXQLEN:
		ifr->ifr_qlen = dev->tx_queue_len;
		return 0;

	default:
		 
		WARN_ON(1);
		err = -EINVAL;
		break;

	}
	return err;
}

static int dev_ifsioc(struct net *net, struct ifreq *ifr, unsigned int cmd)
{
	int err;
	struct net_device *dev = __dev_get_by_name(net, ifr->ifr_name);
	const struct net_device_ops *ops;

	if (!dev)
		return -ENODEV;

	ops = dev->netdev_ops;

	switch (cmd) {
	case SIOCSIFFLAGS:	 
		return dev_change_flags(dev, ifr->ifr_flags);

	case SIOCSIFMETRIC:	 
		return -EOPNOTSUPP;

	case SIOCSIFMTU:	 
		return dev_set_mtu(dev, ifr->ifr_mtu);

	case SIOCSIFHWADDR:
		return dev_set_mac_address(dev, &ifr->ifr_hwaddr);

	case SIOCSIFHWBROADCAST:
		if (ifr->ifr_hwaddr.sa_family != dev->type)
			return -EINVAL;
		memcpy(dev->broadcast, ifr->ifr_hwaddr.sa_data,
		       min(sizeof ifr->ifr_hwaddr.sa_data, (size_t) dev->addr_len));
		call_netdevice_notifiers(NETDEV_CHANGEADDR, dev);
		return 0;

	case SIOCSIFMAP:
		if (ops->ndo_set_config) {
			if (!netif_device_present(dev))
				return -ENODEV;
			return ops->ndo_set_config(dev, &ifr->ifr_map);
		}
		return -EOPNOTSUPP;

	case SIOCADDMULTI:
		if ((!ops->ndo_set_multicast_list && !ops->ndo_set_rx_mode) ||
		    ifr->ifr_hwaddr.sa_family != AF_UNSPEC)
			return -EINVAL;
		if (!netif_device_present(dev))
			return -ENODEV;
		return dev_mc_add(dev, ifr->ifr_hwaddr.sa_data,
				  dev->addr_len, 1);

	case SIOCDELMULTI:
		if ((!ops->ndo_set_multicast_list && !ops->ndo_set_rx_mode) ||
		    ifr->ifr_hwaddr.sa_family != AF_UNSPEC)
			return -EINVAL;
		if (!netif_device_present(dev))
			return -ENODEV;
		return dev_mc_delete(dev, ifr->ifr_hwaddr.sa_data,
				     dev->addr_len, 1);

	case SIOCSIFTXQLEN:
		if (ifr->ifr_qlen < 0)
			return -EINVAL;
		dev->tx_queue_len = ifr->ifr_qlen;
		return 0;

	case SIOCSIFNAME:
		ifr->ifr_newname[IFNAMSIZ-1] = '\0';
		return dev_change_name(dev, ifr->ifr_newname);

	default:
		if ((cmd >= SIOCDEVPRIVATE &&
		    cmd <= SIOCDEVPRIVATE + 15) ||
		    cmd == SIOCBONDENSLAVE ||
		    cmd == SIOCBONDRELEASE ||
		    cmd == SIOCBONDSETHWADDR ||
		    cmd == SIOCBONDSLAVEINFOQUERY ||
		    cmd == SIOCBONDINFOQUERY ||
		    cmd == SIOCBONDCHANGEACTIVE ||
		    cmd == SIOCGMIIPHY ||
		    cmd == SIOCGMIIREG ||
		    cmd == SIOCSMIIREG ||
		    cmd == SIOCBRADDIF ||
		    cmd == SIOCBRDELIF ||
		    cmd == SIOCSHWTSTAMP ||
		    cmd == SIOCWANDEV) {
			err = -EOPNOTSUPP;
			if (ops->ndo_do_ioctl) {
				if (netif_device_present(dev))
					err = ops->ndo_do_ioctl(dev, ifr, cmd);
				else
					err = -ENODEV;
			}
		} else
			err = -EINVAL;

	}
	return err;
}

int dev_ioctl(struct net *net, unsigned int cmd, void __user *arg)
{
	struct ifreq ifr;
	int ret;
	char *colon;

	if (cmd == SIOCGIFCONF) {
		rtnl_lock();
		ret = dev_ifconf(net, (char __user *) arg);
		rtnl_unlock();
		return ret;
	}
	if (cmd == SIOCGIFNAME)
		return dev_ifname(net, (struct ifreq __user *)arg);

	if (copy_from_user(&ifr, arg, sizeof(struct ifreq)))
		return -EFAULT;

	ifr.ifr_name[IFNAMSIZ-1] = 0;

	colon = strchr(ifr.ifr_name, ':');
	if (colon)
		*colon = 0;

	switch (cmd) {
	 
	case SIOCGIFFLAGS:
	case SIOCGIFMETRIC:
	case SIOCGIFMTU:
	case SIOCGIFHWADDR:
	case SIOCGIFSLAVE:
	case SIOCGIFMAP:
	case SIOCGIFINDEX:
	case SIOCGIFTXQLEN:
		dev_load(net, ifr.ifr_name);
		read_lock(&dev_base_lock);
		ret = dev_ifsioc_locked(net, &ifr, cmd);
		read_unlock(&dev_base_lock);
		if (!ret) {
			if (colon)
				*colon = ':';
			if (copy_to_user(arg, &ifr,
					 sizeof(struct ifreq)))
				ret = -EFAULT;
		}
		return ret;

	case SIOCETHTOOL:
		dev_load(net, ifr.ifr_name);
		rtnl_lock();
		ret = dev_ethtool(net, &ifr);
		rtnl_unlock();
		if (!ret) {
			if (colon)
				*colon = ':';
			if (copy_to_user(arg, &ifr,
					 sizeof(struct ifreq)))
				ret = -EFAULT;
		}
		return ret;

	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSIFNAME:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		dev_load(net, ifr.ifr_name);
		rtnl_lock();
		ret = dev_ifsioc(net, &ifr, cmd);
		rtnl_unlock();
		if (!ret) {
			if (colon)
				*colon = ':';
			if (copy_to_user(arg, &ifr,
					 sizeof(struct ifreq)))
				ret = -EFAULT;
		}
		return ret;

	case SIOCSIFFLAGS:
	case SIOCSIFMETRIC:
	case SIOCSIFMTU:
	case SIOCSIFMAP:
	case SIOCSIFHWADDR:
	case SIOCSIFSLAVE:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFHWBROADCAST:
	case SIOCSIFTXQLEN:
	case SIOCSMIIREG:
	case SIOCBONDENSLAVE:
	case SIOCBONDRELEASE:
	case SIOCBONDSETHWADDR:
	case SIOCBONDCHANGEACTIVE:
	case SIOCBRADDIF:
	case SIOCBRDELIF:
	case SIOCSHWTSTAMP:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		 
	case SIOCBONDSLAVEINFOQUERY:
	case SIOCBONDINFOQUERY:
		dev_load(net, ifr.ifr_name);
		rtnl_lock();
		ret = dev_ifsioc(net, &ifr, cmd);
		rtnl_unlock();
		return ret;

	case SIOCGIFMEM:
		 
	case SIOCSIFMEM:
		 
	case SIOCSIFLINK:
		return -EINVAL;

	default:
		if (cmd == SIOCWANDEV ||
		    (cmd >= SIOCDEVPRIVATE &&
		     cmd <= SIOCDEVPRIVATE + 15)) {
			dev_load(net, ifr.ifr_name);
			rtnl_lock();
			ret = dev_ifsioc(net, &ifr, cmd);
			rtnl_unlock();
			if (!ret && copy_to_user(arg, &ifr,
						 sizeof(struct ifreq)))
				ret = -EFAULT;
			return ret;
		}
		 
		if (cmd >= SIOCIWFIRST && cmd <= SIOCIWLAST)
			return wext_handle_ioctl(net, &ifr, cmd, arg);
		return -EINVAL;
	}
}

static int dev_new_index(struct net *net)
{
	static int ifindex;
	for (;;) {
		if (++ifindex <= 0)
			ifindex = 1;
		if (!__dev_get_by_index(net, ifindex))
			return ifindex;
	}
}

static LIST_HEAD(net_todo_list);

static void net_set_todo(struct net_device *dev)
{
	list_add_tail(&dev->todo_list, &net_todo_list);
}

static void rollback_registered(struct net_device *dev)
{
	BUG_ON(dev_boot_phase);
	ASSERT_RTNL();

	if (dev->reg_state == NETREG_UNINITIALIZED) {
		printk(KERN_DEBUG "unregister_netdevice: device %s/%p never "
				  "was registered\n", dev->name, dev);

		WARN_ON(1);
		return;
	}

	BUG_ON(dev->reg_state != NETREG_REGISTERED);

	dev_close(dev);

	unlist_netdevice(dev);

	dev->reg_state = NETREG_UNREGISTERING;

	synchronize_net();

	dev_shutdown(dev);

	call_netdevice_notifiers(NETDEV_UNREGISTER, dev);

	dev_unicast_flush(dev);
	dev_addr_discard(dev);

	if (dev->netdev_ops->ndo_uninit)
		dev->netdev_ops->ndo_uninit(dev);

	WARN_ON(dev->master);

	netdev_unregister_kobject(dev);

	synchronize_net();

	dev_put(dev);
}

static void __netdev_init_queue_locks_one(struct net_device *dev,
					  struct netdev_queue *dev_queue,
					  void *_unused)
{
	spin_lock_init(&dev_queue->_xmit_lock);
	netdev_set_xmit_lockdep_class(&dev_queue->_xmit_lock, dev->type);
	dev_queue->xmit_lock_owner = -1;
}

static void netdev_init_queue_locks(struct net_device *dev)
{
	netdev_for_each_tx_queue(dev, __netdev_init_queue_locks_one, NULL);
	__netdev_init_queue_locks_one(dev, &dev->rx_queue, NULL);
}

unsigned long netdev_fix_features(unsigned long features, const char *name)
{
	 
	if ((features & NETIF_F_SG) &&
	    !(features & NETIF_F_ALL_CSUM)) {
		if (name)
			printk(KERN_NOTICE "%s: Dropping NETIF_F_SG since no "
			       "checksum feature.\n", name);
		features &= ~NETIF_F_SG;
	}

	if ((features & NETIF_F_TSO) && !(features & NETIF_F_SG)) {
		if (name)
			printk(KERN_NOTICE "%s: Dropping NETIF_F_TSO since no "
			       "SG feature.\n", name);
		features &= ~NETIF_F_TSO;
	}

	if (features & NETIF_F_UFO) {
		if (!(features & NETIF_F_GEN_CSUM)) {
			if (name)
				printk(KERN_ERR "%s: Dropping NETIF_F_UFO "
				       "since no NETIF_F_HW_CSUM feature.\n",
				       name);
			features &= ~NETIF_F_UFO;
		}

		if (!(features & NETIF_F_SG)) {
			if (name)
				printk(KERN_ERR "%s: Dropping NETIF_F_UFO "
				       "since no NETIF_F_SG feature.\n", name);
			features &= ~NETIF_F_UFO;
		}
	}

	return features;
}
EXPORT_SYMBOL(netdev_fix_features);

int register_netdevice(struct net_device *dev)
{
	struct hlist_head *head;
	struct hlist_node *p;
	int ret;
	struct net *net = dev_net(dev);

	BUG_ON(dev_boot_phase);
	ASSERT_RTNL();

	might_sleep();

	BUG_ON(dev->reg_state != NETREG_UNINITIALIZED);
	BUG_ON(!net);

	spin_lock_init(&dev->addr_list_lock);
	netdev_set_addr_lockdep_class(dev);
	netdev_init_queue_locks(dev);

#ifdef MY_DEF_HERE
#ifdef CONFIG_NET_GIANFAR_FP
	dev->fastpath_lock = __RW_LOCK_UNLOCKED();
#endif
#endif
	dev->iflink = -1;

	if (dev->netdev_ops->ndo_init) {
		ret = dev->netdev_ops->ndo_init(dev);
		if (ret) {
			if (ret > 0)
				ret = -EIO;
			goto out;
		}
	}

	if (!dev_valid_name(dev->name)) {
		ret = -EINVAL;
		goto err_uninit;
	}

	dev->ifindex = dev_new_index(net);
	if (dev->iflink == -1)
		dev->iflink = dev->ifindex;

	head = dev_name_hash(net, dev->name);
	hlist_for_each(p, head) {
		struct net_device *d
			= hlist_entry(p, struct net_device, name_hlist);
		if (!strncmp(d->name, dev->name, IFNAMSIZ)) {
			ret = -EEXIST;
			goto err_uninit;
		}
	}

	if ((dev->features & NETIF_F_HW_CSUM) &&
	    (dev->features & (NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM))) {
		printk(KERN_NOTICE "%s: mixed HW and IP checksum settings.\n",
		       dev->name);
		dev->features &= ~(NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM);
	}

	if ((dev->features & NETIF_F_NO_CSUM) &&
	    (dev->features & (NETIF_F_HW_CSUM|NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM))) {
		printk(KERN_NOTICE "%s: mixed no checksumming and other settings.\n",
		       dev->name);
		dev->features &= ~(NETIF_F_IP_CSUM|NETIF_F_IPV6_CSUM|NETIF_F_HW_CSUM);
	}

	dev->features = netdev_fix_features(dev->features, dev->name);

	if (dev->features & NETIF_F_SG)
		dev->features |= NETIF_F_GSO;

	netdev_initialize_kobject(dev);
	ret = netdev_register_kobject(dev);
	if (ret)
		goto err_uninit;
	dev->reg_state = NETREG_REGISTERED;

	set_bit(__LINK_STATE_PRESENT, &dev->state);

	dev_init_scheduler(dev);
	dev_hold(dev);
	list_netdevice(dev);

	ret = call_netdevice_notifiers(NETDEV_REGISTER, dev);
	ret = notifier_to_errno(ret);
	if (ret) {
		rollback_registered(dev);
		dev->reg_state = NETREG_UNREGISTERED;
	}
	 
	rtmsg_ifinfo(RTM_NEWLINK, dev, ~0U);

out:
	return ret;

err_uninit:
	if (dev->netdev_ops->ndo_uninit)
		dev->netdev_ops->ndo_uninit(dev);
	goto out;
}
EXPORT_SYMBOL(register_netdevice);

int init_dummy_netdev(struct net_device *dev)
{
	 
	memset(dev, 0, sizeof(struct net_device));

	dev->reg_state = NETREG_DUMMY;

	atomic_set(&dev->refcnt, 1);

	INIT_LIST_HEAD(&dev->napi_list);

	set_bit(__LINK_STATE_PRESENT, &dev->state);
	set_bit(__LINK_STATE_START, &dev->state);

	return 0;
}
EXPORT_SYMBOL_GPL(init_dummy_netdev);

int register_netdev(struct net_device *dev)
{
	int err;
#if defined(MY_ABC_HERE) && defined(MY_ABC_HERE)
	 
	static int netdevCnt = 0;
#endif

	rtnl_lock();

	if (strchr(dev->name, '%')) {
		err = dev_alloc_name(dev, dev->name);
		if (err < 0)
			goto out;
#if defined(MY_ABC_HERE) && defined(MY_ABC_HERE)
		if (syno_is_hw_version(HW_DS508) ||
			syno_is_hw_version(HW_DS1010p) ||
			syno_is_hw_version(HW_DS1511p)) {
			static int swapped = 0;
			if ( swapped == 0 && !strcmp(dev->name, "eth0") ) {
				snprintf(dev->name, sizeof(dev->name), "eth1");
				swapped = 1;
			}
		}
		if (gSwitchDev > 0 && netdevCnt < gSwitchDev) {
			snprintf(dev->name, sizeof(dev->name), "eth%c", gDevPCIName[netdevCnt++][0]);
		}
#endif
	}

	err = register_netdevice(dev);
out:
	rtnl_unlock();
	return err;
}
EXPORT_SYMBOL(register_netdev);

static void netdev_wait_allrefs(struct net_device *dev)
{
	unsigned long rebroadcast_time, warning_time;

	rebroadcast_time = warning_time = jiffies;
	while (atomic_read(&dev->refcnt) != 0) {
		if (time_after(jiffies, rebroadcast_time + 1 * HZ)) {
			rtnl_lock();

			call_netdevice_notifiers(NETDEV_UNREGISTER, dev);

			if (test_bit(__LINK_STATE_LINKWATCH_PENDING,
				     &dev->state)) {
				 
				linkwatch_run_queue();
			}

			__rtnl_unlock();

			rebroadcast_time = jiffies;
		}

		msleep(250);

		if (time_after(jiffies, warning_time + 10 * HZ)) {
			printk(KERN_EMERG "unregister_netdevice: "
			       "waiting for %s to become free. Usage "
			       "count = %d\n",
			       dev->name, atomic_read(&dev->refcnt));
			warning_time = jiffies;
		}
	}
}

void netdev_run_todo(void)
{
	struct list_head list;

	list_replace_init(&net_todo_list, &list);

	__rtnl_unlock();

	while (!list_empty(&list)) {
		struct net_device *dev
			= list_entry(list.next, struct net_device, todo_list);
		list_del(&dev->todo_list);

		if (unlikely(dev->reg_state != NETREG_UNREGISTERING)) {
			printk(KERN_ERR "network todo '%s' but state %d\n",
			       dev->name, dev->reg_state);
			dump_stack();
			continue;
		}

		dev->reg_state = NETREG_UNREGISTERED;

		on_each_cpu(flush_backlog, dev, 1);

		netdev_wait_allrefs(dev);

		BUG_ON(atomic_read(&dev->refcnt));
		WARN_ON(dev->ip_ptr);
		WARN_ON(dev->ip6_ptr);
		WARN_ON(dev->dn_ptr);

		if (dev->destructor)
			dev->destructor(dev);

		kobject_put(&dev->dev.kobj);
	}
}

const struct net_device_stats *dev_get_stats(struct net_device *dev)
{
	const struct net_device_ops *ops = dev->netdev_ops;

	if (ops->ndo_get_stats)
		return ops->ndo_get_stats(dev);
	else {
		unsigned long tx_bytes = 0, tx_packets = 0, tx_dropped = 0;
		struct net_device_stats *stats = &dev->stats;
		unsigned int i;
		struct netdev_queue *txq;

		for (i = 0; i < dev->num_tx_queues; i++) {
			txq = netdev_get_tx_queue(dev, i);
			tx_bytes   += txq->tx_bytes;
			tx_packets += txq->tx_packets;
			tx_dropped += txq->tx_dropped;
		}
		if (tx_bytes || tx_packets || tx_dropped) {
			stats->tx_bytes   = tx_bytes;
			stats->tx_packets = tx_packets;
			stats->tx_dropped = tx_dropped;
		}
		return stats;
	}
}
EXPORT_SYMBOL(dev_get_stats);

static void netdev_init_one_queue(struct net_device *dev,
				  struct netdev_queue *queue,
				  void *_unused)
{
	queue->dev = dev;
}

static void netdev_init_queues(struct net_device *dev)
{
	netdev_init_one_queue(dev, &dev->rx_queue, NULL);
	netdev_for_each_tx_queue(dev, netdev_init_one_queue, NULL);
	spin_lock_init(&dev->tx_global_lock);
}

struct net_device *alloc_netdev_mq(int sizeof_priv, const char *name,
		void (*setup)(struct net_device *), unsigned int queue_count)
{
	struct netdev_queue *tx;
	struct net_device *dev;
	size_t alloc_size;
	struct net_device *p;

	BUG_ON(strlen(name) >= sizeof(dev->name));

	alloc_size = sizeof(struct net_device);
	if (sizeof_priv) {
		 
		alloc_size = ALIGN(alloc_size, NETDEV_ALIGN);
		alloc_size += sizeof_priv;
	}
	 
	alloc_size += NETDEV_ALIGN - 1;

	p = kzalloc(alloc_size, GFP_KERNEL);
	if (!p) {
		printk(KERN_ERR "alloc_netdev: Unable to allocate device.\n");
		return NULL;
	}

	tx = kcalloc(queue_count, sizeof(struct netdev_queue), GFP_KERNEL);
	if (!tx) {
		printk(KERN_ERR "alloc_netdev: Unable to allocate "
		       "tx qdiscs.\n");
		goto free_p;
	}

	dev = PTR_ALIGN(p, NETDEV_ALIGN);
	dev->padded = (char *)dev - (char *)p;

	if (dev_addr_init(dev))
		goto free_tx;

	dev_unicast_init(dev);

	dev_net_set(dev, &init_net);

	dev->_tx = tx;
	dev->num_tx_queues = queue_count;
	dev->real_num_tx_queues = queue_count;

	dev->gso_max_size = GSO_MAX_SIZE;

	netdev_init_queues(dev);

	INIT_LIST_HEAD(&dev->napi_list);
	dev->priv_flags = IFF_XMIT_DST_RELEASE;
	setup(dev);
	strcpy(dev->name, name);
	return dev;

free_tx:
	kfree(tx);

free_p:
	kfree(p);
	return NULL;
}
EXPORT_SYMBOL(alloc_netdev_mq);

void free_netdev(struct net_device *dev)
{
	struct napi_struct *p, *n;

	release_net(dev_net(dev));

	kfree(dev->_tx);

	dev_addr_flush(dev);

	list_for_each_entry_safe(p, n, &dev->napi_list, dev_list)
		netif_napi_del(p);

	if (dev->reg_state == NETREG_UNINITIALIZED) {
		kfree((char *)dev - dev->padded);
		return;
	}

	BUG_ON(dev->reg_state != NETREG_UNREGISTERED);
	dev->reg_state = NETREG_RELEASED;

	put_device(&dev->dev);
}
EXPORT_SYMBOL(free_netdev);

void synchronize_net(void)
{
	might_sleep();
	synchronize_rcu();
}
EXPORT_SYMBOL(synchronize_net);

void unregister_netdevice(struct net_device *dev)
{
	ASSERT_RTNL();

	rollback_registered(dev);
	 
	net_set_todo(dev);
}
EXPORT_SYMBOL(unregister_netdevice);

void unregister_netdev(struct net_device *dev)
{
	rtnl_lock();
	unregister_netdevice(dev);
	rtnl_unlock();
}
EXPORT_SYMBOL(unregister_netdev);

int dev_change_net_namespace(struct net_device *dev, struct net *net, const char *pat)
{
	char buf[IFNAMSIZ];
	const char *destname;
	int err;

	ASSERT_RTNL();

	err = -EINVAL;
	if (dev->features & NETIF_F_NETNS_LOCAL)
		goto out;

#ifdef CONFIG_SYSFS
	 
	err = -EINVAL;
	if (dev->dev.parent)
		goto out;
#endif

	err = -EINVAL;
	if (dev->reg_state != NETREG_REGISTERED)
		goto out;

	err = 0;
	if (net_eq(dev_net(dev), net))
		goto out;

	err = -EEXIST;
	destname = dev->name;
	if (__dev_get_by_name(net, destname)) {
		 
		if (!pat)
			goto out;
		if (!dev_valid_name(pat))
			goto out;
		if (strchr(pat, '%')) {
			if (__dev_alloc_name(net, pat, buf) < 0)
				goto out;
			destname = buf;
		} else
			destname = pat;
		if (__dev_get_by_name(net, destname))
			goto out;
	}

	dev_close(dev);

	err = -ENODEV;
	unlist_netdevice(dev);

	synchronize_net();

	dev_shutdown(dev);

	call_netdevice_notifiers(NETDEV_UNREGISTER, dev);

	dev_unicast_flush(dev);
	dev_addr_discard(dev);

	netdev_unregister_kobject(dev);

	dev_net_set(dev, net);

	if (destname != dev->name)
		strcpy(dev->name, destname);

	if (__dev_get_by_index(net, dev->ifindex)) {
		int iflink = (dev->iflink == dev->ifindex);
		dev->ifindex = dev_new_index(net);
		if (iflink)
			dev->iflink = dev->ifindex;
	}

	err = netdev_register_kobject(dev);
	WARN_ON(err);

	list_netdevice(dev);

	call_netdevice_notifiers(NETDEV_REGISTER, dev);

	rtmsg_ifinfo(RTM_NEWLINK, dev, ~0U);

	synchronize_net();
	err = 0;
out:
	return err;
}
EXPORT_SYMBOL_GPL(dev_change_net_namespace);

static int dev_cpu_callback(struct notifier_block *nfb,
			    unsigned long action,
			    void *ocpu)
{
	struct sk_buff **list_skb;
	struct Qdisc **list_net;
	struct sk_buff *skb;
	unsigned int cpu, oldcpu = (unsigned long)ocpu;
	struct softnet_data *sd, *oldsd;

	if (action != CPU_DEAD && action != CPU_DEAD_FROZEN)
		return NOTIFY_OK;

	local_irq_disable();
	cpu = smp_processor_id();
	sd = &per_cpu(softnet_data, cpu);
	oldsd = &per_cpu(softnet_data, oldcpu);

	list_skb = &sd->completion_queue;
	while (*list_skb)
		list_skb = &(*list_skb)->next;
	 
	*list_skb = oldsd->completion_queue;
	oldsd->completion_queue = NULL;

	list_net = &sd->output_queue;
	while (*list_net)
		list_net = &(*list_net)->next_sched;
	 
	*list_net = oldsd->output_queue;
	oldsd->output_queue = NULL;

	raise_softirq_irqoff(NET_TX_SOFTIRQ);
	local_irq_enable();

	while ((skb = __skb_dequeue(&oldsd->input_pkt_queue)))
		netif_rx(skb);

	return NOTIFY_OK;
}

unsigned long netdev_increment_features(unsigned long all, unsigned long one,
					unsigned long mask)
{
	 
	if (all & NETIF_F_NO_CSUM && !(one & NETIF_F_NO_CSUM))
		all ^= NETIF_F_NO_CSUM | (one & NETIF_F_ALL_CSUM);
	else if (mask & NETIF_F_ALL_CSUM) {
		 
		if (one & (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM) &&
		    !(all & NETIF_F_GEN_CSUM)) {
			all &= ~NETIF_F_ALL_CSUM;
			all |= one & (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
		}

		if (one & NETIF_F_GEN_CSUM && !(all & NETIF_F_GEN_CSUM)) {
			all &= ~NETIF_F_ALL_CSUM;
			all |= NETIF_F_HW_CSUM;
		}
	}

	one |= NETIF_F_ALL_CSUM;

	one |= all & NETIF_F_ONE_FOR_ALL;
	all &= one | NETIF_F_LLTX | NETIF_F_GSO;
	all |= one & mask & NETIF_F_ONE_FOR_ALL;

	return all;
}
EXPORT_SYMBOL(netdev_increment_features);

static struct hlist_head *netdev_create_hash(void)
{
	int i;
	struct hlist_head *hash;

	hash = kmalloc(sizeof(*hash) * NETDEV_HASHENTRIES, GFP_KERNEL);
	if (hash != NULL)
		for (i = 0; i < NETDEV_HASHENTRIES; i++)
			INIT_HLIST_HEAD(&hash[i]);

	return hash;
}

static int __net_init netdev_init(struct net *net)
{
	INIT_LIST_HEAD(&net->dev_base_head);

	net->dev_name_head = netdev_create_hash();
	if (net->dev_name_head == NULL)
		goto err_name;

	net->dev_index_head = netdev_create_hash();
	if (net->dev_index_head == NULL)
		goto err_idx;

	return 0;

err_idx:
	kfree(net->dev_name_head);
err_name:
	return -ENOMEM;
}

char *netdev_drivername(const struct net_device *dev, char *buffer, int len)
{
	const struct device_driver *driver;
	const struct device *parent;

	if (len <= 0 || !buffer)
		return buffer;
	buffer[0] = 0;

	parent = dev->dev.parent;

	if (!parent)
		return buffer;

	driver = parent->driver;
	if (driver && driver->name)
		strlcpy(buffer, driver->name, len);
	return buffer;
}

static void __net_exit netdev_exit(struct net *net)
{
	kfree(net->dev_name_head);
	kfree(net->dev_index_head);
}

static struct pernet_operations __net_initdata netdev_net_ops = {
	.init = netdev_init,
	.exit = netdev_exit,
};

static void __net_exit default_device_exit(struct net *net)
{
	struct net_device *dev;
	 
	rtnl_lock();
restart:
	for_each_netdev(net, dev) {
		int err;
		char fb_name[IFNAMSIZ];

		if (dev->features & NETIF_F_NETNS_LOCAL)
			continue;

		if (dev->rtnl_link_ops && dev->rtnl_link_ops->dellink) {
			dev->rtnl_link_ops->dellink(dev);
			goto restart;
		}

		snprintf(fb_name, IFNAMSIZ, "dev%d", dev->ifindex);
		err = dev_change_net_namespace(dev, &init_net, fb_name);
		if (err) {
			printk(KERN_EMERG "%s: failed to move %s to init_net: %d\n",
				__func__, dev->name, err);
			BUG();
		}
		goto restart;
	}
	rtnl_unlock();
}

static struct pernet_operations __net_initdata default_device_ops = {
	.exit = default_device_exit,
};

static int __init net_dev_init(void)
{
	int i, rc = -ENOMEM;

	BUG_ON(!dev_boot_phase);

	if (dev_proc_init())
		goto out;

	if (netdev_kobject_init())
		goto out;

	INIT_LIST_HEAD(&ptype_all);
	for (i = 0; i < PTYPE_HASH_SIZE; i++)
		INIT_LIST_HEAD(&ptype_base[i]);

	if (register_pernet_subsys(&netdev_net_ops))
		goto out;

	for_each_possible_cpu(i) {
		struct softnet_data *queue;

		queue = &per_cpu(softnet_data, i);
		skb_queue_head_init(&queue->input_pkt_queue);
		queue->completion_queue = NULL;
		INIT_LIST_HEAD(&queue->poll_list);

		queue->backlog.poll = process_backlog;
		queue->backlog.weight = weight_p;
		queue->backlog.gro_list = NULL;
		queue->backlog.gro_count = 0;
	}

	dev_boot_phase = 0;

	if (register_pernet_device(&loopback_net_ops))
		goto out;

	if (register_pernet_device(&default_device_ops))
		goto out;

	open_softirq(NET_TX_SOFTIRQ, net_tx_action);
	open_softirq(NET_RX_SOFTIRQ, net_rx_action);

	hotcpu_notifier(dev_cpu_callback, 0);
	dst_init();
	dev_mcast_init();
	rc = 0;
out:
	return rc;
}

subsys_initcall(net_dev_init);

static int __init initialize_hashrnd(void)
{
	get_random_bytes(&skb_tx_hashrnd, sizeof(skb_tx_hashrnd));
	return 0;
}

late_initcall_sync(initialize_hashrnd);
