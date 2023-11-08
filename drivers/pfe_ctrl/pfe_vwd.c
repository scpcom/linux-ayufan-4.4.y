#include <linux/version.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include <net/pkt_sched.h>
#include <linux/rcupdate.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter_bridge.h>
#include <linux/irqnr.h>
#include <linux/ppp_defs.h>

#include <linux/rculist.h>
#include <../../../net/bridge/br_private.h>

#include "config.h"
#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
#include <net/xfrm.h>
#endif

#include "pfe_mod.h"
#include "pfe_tso.h"
#include "pfe_vwd.h"

#ifdef CFG_WIFI_OFFLOAD

//#define VWD_DEBUG

static int pfe_vwd_rx_low_poll(struct napi_struct *napi, int budget);
static int pfe_vwd_rx_high_poll(struct napi_struct *napi, int budget);
static void pfe_vwd_sysfs_exit(void);
static void pfe_vwd_vap_down(struct vap_desc_s *vap);
static unsigned int pfe_vwd_nf_route_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *));
static unsigned int pfe_vwd_nf_bridge_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *));
static unsigned pfe_vwd_wifi_local_tx( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *));
static int pfe_vwd_handle_vap( struct pfe_vwd_priv_s *vwd, struct vap_cmd_s *cmd );
static int pfe_vwd_event_handler(void *data, int event, int qno);
extern int comcerto_wifi_rx_fastpath_register(int (*hdlr)(struct sk_buff *skb));
extern void comcerto_wifi_rx_fastpath_unregister(void);
static int pfe_vwd_handle_vap( struct pfe_vwd_priv_s *vwd, struct vap_cmd_s *cmd );

extern unsigned int page_mode;
#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
extern struct xfrm_state *xfrm_state_lookup_byhandle(struct net *net, u16 handle);
#endif


/* IPV4 route hook , recieve the packet and forward to VWD driver*/
static struct nf_hook_ops vwd_hook = {
	.hook = pfe_vwd_nf_route_hook_fn,
	.pf = PF_INET,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

/* IPV6 route hook , recieve the packet and forward to VWD driver*/
static struct nf_hook_ops vwd_hook_ipv6 = {
	.hook = pfe_vwd_nf_route_hook_fn,
	.pf = PF_INET6,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP6_PRI_FIRST,
};

/* Bridge hook , recieve the packet and forward to VWD driver*/
static struct nf_hook_ops vwd_hook_bridge = {
	.hook = pfe_vwd_nf_bridge_hook_fn,
	.pf = PF_BRIDGE,
	.hooknum = NF_BR_PRE_ROUTING,
	.priority = NF_BR_PRI_FIRST,
};

/* FIXME : Bridge hook , recieve the wifi local packets. This is temporary need to find efficient
           way to capture wifi TX packets */

static struct nf_hook_ops vwd_hook_wifi_tx = {
	.hook = pfe_vwd_wifi_local_tx,
	.pf = PF_BRIDGE,
	.hooknum = NF_BR_LOCAL_OUT,
	.priority = NF_BR_PRI_LAST,
};

struct pfe_vwd_priv_s glbl_pfe_vwd_priv;

#ifdef VWD_DEBUG
static void pfe_vwd_dump_skb( struct sk_buff *skb )
{
	int i;

	for (i = 0; i < skb->len; i++)
	{
		if (!(i % 16))
			printk("\n");

		printk(" %02x", skb->data[i]);
	}
}
#endif

/** get_vap_by_name
 *
 */
static struct vap_desc_s *get_vap_by_name(struct pfe_vwd_priv_s *priv, const char *name)
{
	int ii;
	struct vap_desc_s *vap = NULL;

	for (ii = 0; ii < MAX_VAP_SUPPORT; ii++)
		if (priv->vaps[ii].ifindex && (!strcmp(priv->vaps[ii].ifname, name))) {
			vap = &priv->vaps[ii];
			break;
		}

	return vap;
}

/** pfe_vwd_vap_create
 *
 */
static int pfe_vwd_vap_create(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	struct net_device *wifi_dev;
	struct vap_cmd_s vap_cmd;
	char name[IFNAMSIZ];
	char tmp_name[IFNAMSIZ];
	int ii, len;

	len = IFNAMSIZ - 1;

	if (len > count)
		len = count;

	memcpy(tmp_name, buf, len);
	tmp_name[len] = '\n';
	sscanf(tmp_name, "%s", name);

	wifi_dev = dev_get_by_name(&init_net, name);
	if(wifi_dev) {
		spin_lock_bh(&priv->vaplock);
		for (ii = 0; ii < MAX_VAP_SUPPORT; ii++) {
			if (!priv->vaps[ii].ifindex)
				break;
		}

		if (ii < MAX_VAP_SUPPORT) {
			vap_cmd.action = ADD;
			vap_cmd.vapid = ii;
			vap_cmd.ifindex = wifi_dev->ifindex;
			strcpy(vap_cmd.ifname, name);
			memcpy(vap_cmd.macaddr, wifi_dev->dev_addr, 6);
			pfe_vwd_handle_vap(priv, &vap_cmd);
		}
		else
			printk("%s: All VAPs are used.. No space.\n",__func__);

		spin_unlock_bh(&priv->vaplock);

		dev_put(wifi_dev);
	}
	else {
		printk(KERN_ERR "%s: %s is invalid interface Or not created...\n",__func__, name);
	}

	return count;
}

/** pfe_vwd_vap_remove
 *
 */
static int pfe_vwd_vap_remove(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	struct vap_desc_s *vap;
	struct vap_cmd_s vap_cmd;
	char name[IFNAMSIZ];
	char tmp_name[IFNAMSIZ];
	int len;

	len = IFNAMSIZ - 1;

	if (len > count)
		len = count;

	memcpy(tmp_name, buf, len);
	tmp_name[len] = '\n';

	sscanf(tmp_name, "%s", name);

	spin_lock_bh(&priv->vaplock);
	vap = get_vap_by_name(priv, name);

	if (!vap) {
		printk(KERN_ERR "%s: %s is not valid VAP\n", __func__, name);
		goto done;
	}

	vap_cmd.action = REMOVE;
	vap_cmd.vapid = vap->vapid;
	pfe_vwd_handle_vap(priv, &vap_cmd);

done:
	spin_unlock_bh(&priv->vaplock);

	return count;
}


/** pfe_vwd_show_rx_csum_enable
 *
 */
static int pfe_vwd_show_rx_csum_enable(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap;
	int idx = 0;

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	idx += sprintf(buf + idx, "%s : %d \n", vap->ifname, (vap->dev->features & NETIF_F_RXCSUM) ? 1:0);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return idx;
}

/** pfe_vwd_set_rx_csum_enable
 *
 */
static int pfe_vwd_set_rx_csum_enable(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	int enable;
	struct net_device *wifi_dev;

	sscanf(buf, "%d", &enable);

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	wifi_dev = dev_get_by_name(&init_net, vap->ifname);
	if(wifi_dev) {
		if ((wifi_dev->features & NETIF_F_RXCSUM) && !enable) {
			wifi_dev->hw_features &= ~(NETIF_F_RXCSUM);
			wifi_dev->features &= ~(NETIF_F_RXCSUM);
		}
		else if (!(wifi_dev->features & NETIF_F_RXCSUM) && enable) {
			wifi_dev->hw_features |= (NETIF_F_RXCSUM);
			wifi_dev->features |= (NETIF_F_RXCSUM);

		}

		dev_put(wifi_dev);
	}
	else
		printk("%s: %s interface not exist ...\n",__func__, vap->ifname);

	spin_unlock_bh(&pfe->vwd.vaplock);
	return count;
}

/** pfe_vwd_show_lro_enable
 *
 */
static int pfe_vwd_show_lro_enable(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap;
	int idx = 0;

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	idx += sprintf(buf + idx, "%s : %d \n", vap->ifname, (vap->dev->features & NETIF_F_LRO) ? 1:0);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return idx;
}

/** pfe_vwd_set_lro_enable
 *
 */
static int pfe_vwd_set_lro_enable(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	int enable;
	struct net_device *wifi_dev;

	sscanf(buf, "%d", &enable);

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	wifi_dev = dev_get_by_name(&init_net, vap->ifname);
	if(wifi_dev) {
		if ((wifi_dev->features & NETIF_F_LRO) && !enable) {
			wifi_dev->hw_features &= ~(NETIF_F_LRO|NETIF_F_RXCSUM);
			wifi_dev->features &= ~(NETIF_F_LRO|NETIF_F_RXCSUM);

		}
		else if (!(wifi_dev->features & NETIF_F_LRO) && enable) {
			wifi_dev->hw_features |= (NETIF_F_LRO|NETIF_F_RXCSUM);
			wifi_dev->features |= (NETIF_F_LRO|NETIF_F_RXCSUM);

		}

		dev_put(wifi_dev);
	}
	else
		printk("%s: %s interface not exist ...\n",__func__, vap->ifname);

	spin_unlock_bh(&pfe->vwd.vaplock);
	return count;
}

#ifdef PFE_VWD_LRO_STATS
/*
 * pfe_vwd_show_lro_nb_stats
 */
static ssize_t pfe_vwd_show_lro_nb_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	ssize_t len = 0;
	int i;

	for (i = 0; i < LRO_NB_COUNT_MAX; i++)
		len += sprintf(buf + len, "%d fragments packets = %d\n", i, priv->lro_nb_counters[i]);

	return len;
}

/*
 * pfe_vwd_set_lro_nb_stats
 */
static ssize_t pfe_vwd_set_lro_nb_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	memset(priv->lro_nb_counters, 0, sizeof(priv->lro_nb_counters));

	return count;
}

/*
 * pfe_vwd_show_lro_len_stats
 */
static ssize_t pfe_vwd_show_lro_len_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	ssize_t len = 0;
	int i;

	for (i = 0; i < LRO_LEN_COUNT_MAX; i++)
		len += sprintf(buf + len, "RX packets > %dKBytes = %d\n", i * 2, priv->lro_len_counters[i]);

	return len;
}

/*
 * pfe_vwd_set_lro_len_stats
 */
static ssize_t pfe_vwd_set_lro_len_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	memset(priv->lro_len_counters, 0, sizeof(priv->lro_len_counters));

	return count;
}
#endif

#ifdef PFE_VWD_NAPI_STATS
/*
 * pfe_vwd_show_napi_stats
 */
static ssize_t pfe_vwd_show_napi_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	ssize_t len = 0;

	len += sprintf(buf + len, "sched:  %d\n", priv->napi_counters[NAPI_SCHED_COUNT]);
	len += sprintf(buf + len, "poll:   %d\n", priv->napi_counters[NAPI_POLL_COUNT]);
	len += sprintf(buf + len, "packet: %d\n", priv->napi_counters[NAPI_PACKET_COUNT]);
	len += sprintf(buf + len, "budget: %d\n", priv->napi_counters[NAPI_FULL_BUDGET_COUNT]);
	len += sprintf(buf + len, "desc:   %d\n", priv->napi_counters[NAPI_DESC_COUNT]);

	return len;
}

/*
 * pfe_vwd_set_napi_stats
 */
static ssize_t pfe_vwd_set_napi_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	memset(priv->napi_counters, 0, sizeof(priv->napi_counters));

	return count;
}
#endif

/** pfe_vwd_show_dump_stats
 *
 */
static ssize_t pfe_vwd_show_dump_stats(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

#ifdef VWD_DEBUG_STATS
	len += sprintf(buf, "\nTo PFE\n");
	len += sprintf(buf + len, "  WiFi Rx pkts : %d\n", priv->pkts_transmitted);
	len += sprintf(buf + len, "  WiFi Tx pkts : %d\n", priv->pkts_total_local_tx);
	len += sprintf(buf + len, "  WiFi Tx SG pkts : %d\n", priv->pkts_local_tx_sgs);
	len += sprintf(buf + len, "  Drops : %d\n", priv->pkts_tx_dropped);

	len += sprintf(buf + len, "From PFE\n");
	len += sprintf(buf + len, "  WiFi Rx pkts : %d %d %d\n", priv->pkts_slow_forwarded[0],
			priv->pkts_slow_forwarded[1], priv->pkts_slow_forwarded[2]);
	len += sprintf(buf + len, "  WiFi Tx pkts : %d %d %d\n", priv->pkts_rx_fast_forwarded[0],
			priv->pkts_rx_fast_forwarded[1], priv->pkts_rx_fast_forwarded[2]);
	len += sprintf(buf + len, "  Skb Alloc fails : %d\n", priv->rx_skb_alloc_fail);
#endif
	len += sprintf(buf + len, "\nStatus\n");
	len += sprintf(buf + len, "  Fast path - %s\n", priv->fast_path_enable ? "Enable" : "Disable");
	len += sprintf(buf + len, "  Route hook - %s\n", priv->fast_routing_enable ? "Enable" : "Disable");
	len += sprintf(buf + len, "  Bridge hook - %s\n", priv->fast_bridging_enable ? "Enable" : "Disable");
	len += sprintf(buf + len, "  TSO hook - %s\n", priv->tso_hook_enable ? "Enable" : "Disable");

	return len;
}


/** pfe_vwd_show_fast_path_enable
 *
 */
static ssize_t pfe_vwd_show_fast_path_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int idx;

	idx = sprintf(buf, "\n%d\n", priv->fast_path_enable);

	return idx;
}

/** pfe_vwd_set_fast_path_enable
 *
 */
static ssize_t pfe_vwd_set_fast_path_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s  *priv = &pfe->vwd;
        unsigned int fast_path = 0;

	sscanf(buf, "%d", &fast_path);

	printk("%s: Wifi fast path %d\n", __func__, fast_path);

        if (fast_path && !priv->fast_path_enable)
        {
                printk("%s: Wifi fast path enabled \n", __func__);

                priv->fast_path_enable = 1;

        }
        else if (!fast_path && priv->fast_path_enable)
        {
                printk("%s: Wifi fast path disabled \n", __func__);

                priv->fast_path_enable = 0;

        }

	return count;
}

/** pfe_vwd_show_route_hook_enable
 *
 */
static ssize_t pfe_vwd_show_route_hook_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int idx;

	idx = sprintf(buf, "\n%d\n", priv->fast_routing_enable);

	return idx;
}

/** pfe_vwd_set_route_hook_enable
 *
 */
static ssize_t pfe_vwd_set_route_hook_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
        unsigned int user_val = 0;

        sscanf(buf, "%d", &user_val);

        if (user_val && !priv->fast_routing_enable)
        {
                printk("%s: Wifi fast routing enabled \n", __func__);
                priv->fast_routing_enable = 1;

                if (priv->fast_bridging_enable)
                {
                        nf_unregister_hook(&vwd_hook_bridge);
                        priv->fast_bridging_enable = 0;
                }

                nf_register_hook(&vwd_hook);
                nf_register_hook(&vwd_hook_ipv6);


        }
        else if (!user_val && priv->fast_routing_enable)
        {
                printk("%s: Wifi fast routing disabled \n", __func__);
                priv->fast_routing_enable = 0;

                nf_unregister_hook(&vwd_hook);
                nf_unregister_hook(&vwd_hook_ipv6);

        }

	return count;
}

/** pfe_vwd_show_bridge_hook_enable
 *
 */
static int pfe_vwd_show_bridge_hook_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int idx;

	idx = sprintf(buf, "%d", priv->fast_bridging_enable);
	return idx;
}

/** pfe_vwd_set_bridge_hook_enable
 *
 */
static int pfe_vwd_set_bridge_hook_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	unsigned int user_val = 0;

        sscanf(buf, "%d", &user_val);

	if ( user_val && !priv->fast_bridging_enable )
        {
                printk("%s: Wifi fast bridging enabled \n", __func__);
                priv->fast_bridging_enable = 1;

                if(priv->fast_routing_enable)
                {
                        nf_unregister_hook(&vwd_hook);
                        nf_unregister_hook(&vwd_hook_ipv6);
                        priv->fast_routing_enable = 0;
                }

                nf_register_hook(&vwd_hook_bridge);
        }
        else if ( !user_val && priv->fast_bridging_enable )
        {
                printk("%s: Wifi fast bridging disabled \n", __func__);
                priv->fast_bridging_enable = 0;

                nf_unregister_hook(&vwd_hook_bridge);
        }

	return count;
}

/** pfe_vwd_show_direct_tx_path
 *
 */
static int pfe_vwd_show_direct_tx_path(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap;
	int rc;

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	if (vap && vap->ifindex)
		rc = sprintf(buf, "%d\n", vap->direct_tx_path);
	else
		rc = sprintf(buf, "%s: This should not happend, VAP doesn't exist\n", __func__);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return rc;
}

/** pfe_vwd_set_direct_tx_path
 *
 */
static int pfe_vwd_set_direct_tx_path(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	int enable;

	sscanf(buf, "%d", &enable);

	spin_lock_bh(&pfe->vwd.vaplock);

	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));

	if (vap && vap->ifindex) {
		printk(KERN_INFO "%s: VWD => WiFi direct path is %s for %s\n",
				__func__, enable ? "enabled":"disabled", vap->ifname);
		vap->direct_tx_path = enable;
	}
	else
		printk(KERN_ERR "%s: This should not happend, VAP doesn't exist\n", __func__);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return count;
}

/** pfe_vwd_show_direct_rx_path
 *
 */
static int pfe_vwd_show_direct_rx_path(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap;
	int rc;

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	if (vap && vap->ifindex)
		rc = sprintf(buf, "%d\n", vap->direct_rx_path);
	else
		rc = sprintf(buf, "%s: This should not happend, VAP doesn't exist\n", __func__);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return rc;
}

/** pfe_vwd_set_direct_rx_path
 *
 */
static int pfe_vwd_set_direct_rx_path(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	int enable;

	sscanf(buf, "%d", &enable);

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));

	if (vap && vap->ifindex) {
		printk(KERN_INFO "%s: WiFi => VWD direct path is %s for %s\n",
				__func__, enable ? "enabled":"disabled", vap->ifname);
		vap->direct_rx_path = enable;
	}
	else
		printk(KERN_ERR "%s: This should not happend, VAP doesn't exist\n", __func__);

	spin_unlock_bh(&pfe->vwd.vaplock);

	return count;
}

#if defined(CONFIG_SMP) && (NR_CPUS > 1)
/** pfe_vwd_show_rx_cpu_affinity
 *
 */
static int pfe_vwd_show_rx_cpu_affinity(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap;
	int rc;

	spin_lock_bh(&pfe->vwd.vaplock);
	vap =  get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	if (vap && vap->ifindex)
		rc = sprintf(buf, "%d\n", vap->cpu_id);
	else
		rc = sprintf(buf, "%s: This should not happend, VAP doesn't exist\n", __func__);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return rc;
}

/** pfe_vwd_set_rx_cpu_affinity
 *
 */
static int pfe_vwd_set_rx_cpu_affinity(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	unsigned int cpu_id;

	sscanf(buf, "%d", &cpu_id);

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));

	if (cpu_id < NR_CPUS) {
		if (vap && vap->ifindex) {
			vap->cpu_id = cpu_id;
			hif_lib_set_rx_cpu_affinity(&vap->client, cpu_id);
		}
		else
			printk(KERN_ERR "%s: This should not happend, VAP doesn't exist\n", __func__);

	}
	else
		printk(KERN_ERR "%s: Invalid cpu#%d \n", __func__, cpu_id);

	spin_unlock_bh(&pfe->vwd.vaplock);

	return count;
}
#endif

#if defined(CONFIG_COMCERTO_CUSTOM_SKB_LAYOUT)
/** pfe_vwd_show_custom_skb_enable
 *
 */
static int pfe_vwd_show_custom_skb_enable(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));

	if (vap && vap->ifindex)
		return sprintf(buf, "%d\n", vap->custom_skb);
	else
		return sprintf(buf, "%s: This should not happend, VAP doesn't exist\n", __func__);
}

/** pfe_vwd_set_custom_skb_enable
 *
 */
static int pfe_vwd_set_custom_skb_enable(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	int enable;

	sscanf(buf, "%d", &enable);

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));

	if (vap && vap->ifindex) {
		printk(KERN_INFO "%s: Custun skb feature is %s for %s\n", __func__, enable ? "enabled":"disabled", vap->ifname);
		vap->custom_skb = enable;
	}
	else
		printk(KERN_ERR "%s: This should not happend, VAP doesn't exist\n", __func__);

	spin_unlock_bh(&pfe->vwd.vaplock);

	return count;
}
#endif

/** pfe_vwd_show_tso_enable
 *
 */
static int pfe_vwd_show_tso_enable(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct vap_desc_s *vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	int count = 0;

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	count += sprintf(buf, "%s : %d \n", vap->ifname, (vap->dev->features & NETIF_F_TSO) ? 1:0);
	spin_unlock_bh(&pfe->vwd.vaplock);

	return count;
}

/** pfe_vwd_set_tso_enable
 *
 */
static int pfe_vwd_set_tso_enable(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct vap_desc_s *vap;
	unsigned int user_val = 0;

        sscanf(buf, "%d", &user_val);

	spin_lock_bh(&pfe->vwd.vaplock);
	vap = get_vap_by_name(&pfe->vwd, kobject_name(kobj));
	if (vap && !vap->dev) {
		printk("%s: This should not happend, VAP doesn't exist\n", __func__);
		count = -EINVAL;
		goto err_out;
	}

	if (user_val)
	{
		printk("\nTSO enabled for %s \n", vap->ifname);
		vap->dev->features |= (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_SG | NETIF_F_TSO);
		vap->dev->hw_features |= (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_SG | NETIF_F_TSO);

		if(!pfe->vwd.tso_hook_enable) {
			nf_register_hook(&vwd_hook_wifi_tx);
			pfe->vwd.tso_hook_enable = 1;
		}
        }
        else
        {
                printk("\nTSO disabled for : %s\n", vap->ifname);
		vap->dev->features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_SG | NETIF_F_TSO);
		vap->dev->hw_features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_SG | NETIF_F_TSO);
        }

err_out:
	spin_unlock_bh(&pfe->vwd.vaplock);
	return count;
}
/*
 * pfe_vwd_show_tso_stats
 */
static ssize_t pfe_vwd_show_tso_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	ssize_t len = 0;
	int i;

	for (i = 0; i < 32; i++)
		len += sprintf(buf + len, "TSO packets > %dKBytes = %u\n", i * 2, priv->tso.tso_len_counters[i]);

	return len;
}

/*
 * pfe_vwd_set_tso_stats
 */
static ssize_t pfe_vwd_set_tso_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	memset(priv->tso.tso_len_counters, 0, sizeof(priv->tso.tso_len_counters));
	return count;
}

/** pfe_vwd_classify_packet
 *
 */
static int pfe_vwd_classify_packet( struct pfe_vwd_priv_s *priv, struct sk_buff *skb,
							int bridge_hook, int route_hook, int *vapid, int *own_mac)
{
	unsigned short type;
	struct vap_desc_s *vap;
	int rc = 1, ii, length;
	unsigned char *data_ptr;
	struct ethhdr *hdr;
#if defined (CONFIG_COMCERTO_VWD_MULTI_MAC)
	struct net_bridge_fdb_entry *dst = NULL;
	struct net_bridge_port *p = NULL;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
#endif
	*own_mac = 0;

	/* Move to packet network header */
	data_ptr = skb_mac_header(skb);
	length = skb->len + (skb->data - data_ptr);

	spin_lock_bh(&priv->vaplock);
	/* Broadcasts and MC are handled by stack */
	if( (eth_hdr(skb)->h_dest[0] & 0x1) ||
			( length <= ETH_HLEN ) )
	{
		rc = 1;
		goto done;
	}

	/* FIXME: This packet is VWD slow path packet, and already seen by VWD */

	if (*(unsigned long *)skb->head == 0xdead)
	{
		//printk(KERN_INFO "%s:This is dead packet....\n", __func__);
		*(unsigned long *)skb->head = 0x0;
		rc = 1;
		goto done;
	}

#ifdef VWD_DEBUG
	printk(KERN_INFO "%s: skb cur len:%d skb orig len:%d\n", __func__, skb->len, length );
#endif

	/* FIXME: We need to check the route table for the route entry. If route
	 *  entry found for the current packet, send the packet to PFE. Otherwise
	 *  REJECT the packet.
	 */
	for ( ii = 0; ii < MAX_VAP_SUPPORT; ii++ )
	{
		vap =   &priv->vaps[ii];
#ifdef VWD_DEBUG
		printk(KERN_INFO "%s:%d: iif:%d skb iif:%d\n", __func__,ii, vap->ifindex, skb->skb_iif );
#endif
		if( ( vap->ifindex ) && ( vap->ifindex == skb->skb_iif) )
		{
			/* This interface packets need to be processed by direct API */
			if (vap->direct_rx_path) {
				rc = 1;
				goto done;
			}

			hdr = (struct ethhdr *)data_ptr;
			type = htons(hdr->h_proto);
			data_ptr += ETH_HLEN;
			length -= ETH_HLEN;
			rc = 0;

			*vapid = vap->vapid;

			/* FIXME send only IPV4 and IPV6 packets to PFE */
			//Determain final protocol type
			//FIXME : This multi level parsing is not required for
			//        Bridged packets.
			if( type == ETH_P_8021Q )
			{
				struct vlan_hdr *vhdr = (struct vlan_hdr *)data_ptr;

				data_ptr += VLAN_HLEN;
				length -= VLAN_HLEN;
				type = htons(vhdr->h_vlan_encapsulated_proto);
			}

			if( type == ETH_P_PPP_SES )
			{
				struct pppoe_hdr *phdr = (struct pppoe_hdr *)data_ptr;

				data_ptr += PPPOE_SES_HLEN;
				length -= PPPOE_SES_HLEN;

				if (htons(*(u16 *)(phdr+1)) == PPP_IP)
					type = ETH_P_IP;
				else if (htons(*(u16 *)(phdr+1)) == PPP_IPV6)
					type = ETH_P_IPV6;
			}


			if (bridge_hook)
			{
#if defined (CONFIG_COMCERTO_VWD_MULTI_MAC)
				/* check if destination MAC matches one of the interfaces attached to the bridge */
				if((p = rcu_dereference(skb->dev->br_port)) != NULL)
					dst = __br_fdb_get(p->br, dest);

				if (skb->pkt_type == PACKET_HOST || (dst && dst->is_local))
#else
					if (skb->pkt_type == PACKET_HOST)
#endif
					{
						*own_mac = 1;

						if ((type != ETH_P_IP) && (type != ETH_P_IPV6))
						{
							rc = 1;
							goto done;
						}
					}
					else if (!memcmp(vap->macaddr, eth_hdr(skb)->h_dest, ETH_ALEN))
					{
						//WiFi management packets received with dst address
						//as bssid
						rc = 1;
						goto done;
					}
			}
			else
				*own_mac = 1;

			break;
		}

	}

done:
	spin_unlock_bh(&priv->vaplock);
	return rc;

}

/** pfe_vwd_flush_txQ
 *
 */
static void pfe_vwd_flush_txQ(struct vap_desc_s *vap, int queuenum, int from_tx, int n_desc)
{
	struct sk_buff *skb;
	int count = max(TX_FREE_MAX_COUNT, n_desc);
	unsigned int flags;

	//printk(KERN_INFO "%s\n", __func__);

	if (!from_tx)
		spin_lock_bh(&vap->tx_lock[queuenum]);

	while (count && (skb = hif_lib_tx_get_next_complete(&vap->client, queuenum, &flags, count))) {

		/* FIXME : Invalid data can be skipped in hif_lib itself */
		if (flags & HIF_DATA_VALID) {
			if (flags & HIF_DONT_DMA_MAP)
				pfe_common_skb_unmap(skb);
			dev_kfree_skb_any(skb);
		}
		// When called from the timer, flush all descriptors
		if (from_tx)
			count--;
	}

	if (!from_tx)
		spin_unlock_bh(&vap->tx_lock[queuenum]);


}

/** pfe_eth_flush_tx
 */
static void pfe_vwd_flush_tx(struct vap_desc_s *vap, int force)
{
	int ii;

	for (ii = 0; ii < VWD_TXQ_CNT; ii++) {
		if (force || (time_after(jiffies, vap->client.tx_q[ii].jiffies_last_packet + (COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ)/1000)))
			pfe_vwd_flush_txQ(vap, ii, 0, 0); //We will release everything we can based on from_tx param, so the count param can be set to any value
	}
}


/** pfe_vwd_tx_timeout
 */
void pfe_vwd_tx_timeout(unsigned long data )
{
	struct pfe_vwd_priv_s *priv = (struct pfe_vwd_priv_s *)data;
	int ii;

	spin_lock_bh(&priv->vaplock);
	for (ii = 0; ii < MAX_VAP_SUPPORT; ii++) {
		if (!priv->vaps[ii].ifindex)
			continue;

		pfe_vwd_flush_tx(&priv->vaps[ii], 0);
	}

	priv->tx_timer.expires = jiffies + ( COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ )/1000;
	add_timer(&priv->tx_timer);
	spin_unlock_bh(&priv->vaplock);
}

/** pfe_vwd_send_packet
 *
 */
static void pfe_vwd_send_packet( struct sk_buff *skb, struct  pfe_vwd_priv_s *priv, int queuenum, struct vap_desc_s *vap, int own_mac, u32 ctrl)
{
	void *data;
	int count;
	struct skb_shared_info *sh;
	unsigned int nr_frags;

	//printk(KERN_INFO "%s\n", __func__);

	spin_lock_bh(&vap->tx_lock[queuenum]);

	if (skb_is_gso(skb)) {
		pfe_common_tso(skb, &vap->client, &priv->tso, queuenum, ctrl);

		goto out;
	}

	if (skb_headroom(skb) < (PFE_PKT_HEADER_SZ + sizeof(unsigned long))) {

		//printk(KERN_INFO "%s: copying skb %d\n", __func__, skb_headroom(skb));

		if (pskb_expand_head(skb, (PFE_PKT_HEADER_SZ + sizeof(unsigned long)), 0, GFP_ATOMIC)) {
			kfree_skb(skb);
#ifdef VWD_DEBUG_STATS
			priv->pkts_tx_dropped += 1;
#endif
			goto out;
		}
	}


	/* Send vap_id to PFE */
	if (own_mac)
		ctrl |= ((vap->vapid << HIF_CTRL_VAPID_OFST) | HIF_CTRL_TX_OWN_MAC);
	else
		ctrl |= (vap->vapid << HIF_CTRL_VAPID_OFST);


	if ((vap->dev->features & NETIF_F_RXCSUM) && (skb->ip_summed == CHECKSUM_NONE))
		ctrl |= HIF_CTRL_TX_CSUM_VALIDATE;

	sh = skb_shinfo(skb);
	nr_frags = sh->nr_frags;

	if (nr_frags) {
		skb_frag_t *f;
		int i;

		//printk("%s: SG packet \n", __func__);
		hif_tx_lock(&pfe->hif);

		if ((__hif_tx_avail(&pfe->hif) < (nr_frags + 1)) || (hif_lib_tx_avail(&vap->client, queuenum) < (nr_frags + 1))) {

			hif_tx_unlock(&pfe->hif);

			printk(KERN_ERR "%s: __hif_lib_xmit_pkt() failed\n", __func__);

			kfree_skb(skb);
#ifdef VWD_DEBUG_STATS
			priv->pkts_tx_dropped++;
#endif
			goto out;
		}

		__hif_lib_xmit_pkt(&vap->client, queuenum, skb->data, skb_headlen(skb), ctrl, HIF_FIRST_BUFFER, skb);

		for (i = 0; i < nr_frags - 1; i++) {
			f = &sh->frags[i];

			__hif_lib_xmit_pkt(&vap->client, queuenum, skb_frag_address(f), skb_frag_size(f), 0x0, 0x0, skb);
		}

		f = &sh->frags[i];

		__hif_lib_xmit_pkt(&vap->client, queuenum, skb_frag_address(f), skb_frag_size(f), 0x0, HIF_LAST_BUFFER|HIF_DATA_VALID, skb);

		hif_tx_unlock(&pfe->hif);
		hif_tx_dma_start();

		//printk("%s: pkt sent successfully skb:%p nr_frags:%d len:%d\n", __func__, skb, nr_frags, skb->len);
#ifdef VWD_DEBUG_STATS
			priv->pkts_local_tx_sgs += 1;
#endif
	}
	else
	{
#if defined(CONFIG_COMCERTO_CUSTOM_SKB_LAYOUT)
		if (skb->mspd_data && skb->mspd_len)
		{
			int len = skb->len -  skb->mspd_len;

			//printk("%s : custom skb\n", __func__);

			data = (skb->mspd_data + skb->mspd_ofst) - len;
			memcpy(data, skb->data, len);
		}
		else
#endif
			data = skb->data;

		if (hif_lib_xmit_pkt(&vap->client, queuenum, data, skb->len, ctrl, skb)) {
			// HIF Lib unable send packet
			printk(KERN_ERR "%s: hif_lib_xmit_pkt() failed\n", __func__);
			kfree_skb(skb);

#ifdef VWD_DEBUG_STATS
			priv->pkts_tx_dropped += 1;
#endif
			goto out;
		}
	}

#ifdef VWD_DEBUG_STATS
	priv->pkts_transmitted += 1;
#endif

	//printk(KERN_INFO "%s: pkt sent successfully skb:%p len:%d\n", __func__, skb, skb->len);

out:
	// Recycle buffers if a socket's send buffer becomes half full or if the HIF client queue starts filling up
	if (((count = (hif_lib_tx_pending(&vap->client, queuenum) - HIF_CL_TX_FLUSH_MARK)) > 0)
		|| (skb && skb->sk && ((sk_wmem_alloc_get(skb->sk) << 1) > skb->sk->sk_sndbuf)))
		pfe_vwd_flush_txQ(vap, queuenum, 1, count);

	spin_unlock_bh(&vap->tx_lock[queuenum]);

	return;
}

/*
 * vwd_wifi_if_send_pkt
 */
static int vwd_wifi_if_send_pkt(struct sk_buff *skb)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int ii;
	unsigned int dst_mac[2];

	if (!priv->fast_path_enable)
		goto end;

	/* Copy destination mac into cacheable memory */
	if (!((unsigned long)skb->data & 0x3))
		__memcpy8(dst_mac, skb->data);
	else
		memcpy(dst_mac, skb->data, 6);

	if (dst_mac[0] & 0x1)
		goto end;

	spin_lock_bh(&priv->vaplock);

	for (ii = 0; ii < MAX_VAP_SUPPORT; ii++)
	{
		struct vap_desc_s *vap;

		vap =   &priv->vaps[ii];

		if (vap->ifindex == skb->dev->ifindex)
		{
			if (unlikely(!vap->direct_rx_path)) {
				spin_unlock_bh(&priv->vaplock);
				goto end;
			}

			if (!memcmp(vap->macaddr, dst_mac, ETH_ALEN))
				pfe_vwd_send_packet( skb, priv, 0, vap, 1, 0);
			else
				pfe_vwd_send_packet( skb, priv, 0, vap, 0, 0);

			break;
		}
	}

	spin_unlock_bh(&priv->vaplock);

	if (unlikely(ii == MAX_VAP_SUPPORT))
		goto end;

	return 0;

end:
	return -1;
}


/** vwd_nf_bridge_hook_fn
 *
 */
static unsigned int pfe_vwd_nf_bridge_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int vapid = -1;
	int own_mac = 0;

#ifdef VWD_DEBUG
	printk("%s: protocol : 0x%04x\n", __func__, htons(skb->protocol));
#endif

	if( !priv->fast_path_enable )
		goto done;

	if( !pfe_vwd_classify_packet(priv, skb, 1, 0, &vapid, &own_mac) )
	{
#ifdef VWD_DEBUG
		printk("%s: Accepted\n", __func__);
	//	pfe_vwd_dump_skb( skb );
#endif
		skb_push(skb, ETH_HLEN);
		pfe_vwd_send_packet( skb, priv, 0, &priv->vaps[vapid], own_mac, 0);
		return NF_STOLEN;
	}

done:

	return NF_ACCEPT;

}

/** vwd_nf_route_hook_fn
 *
 */
static unsigned int pfe_vwd_nf_route_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int vapid = -1;
	int own_mac = 0;

#ifdef VWD_DEBUG
	printk("%s: protocol : 0x%04x\n", __func__, htons(skb->protocol));
#endif

	if (!priv->fast_path_enable)
		goto done;

	if (!pfe_vwd_classify_packet(priv, skb, 0, 1, &vapid, &own_mac))
	{
#ifdef VWD_DEBUG
		printk("%s: Accepted\n", __func__);
//		pfe_vwd_dump_skb( skb );
#endif
		skb_push(skb, ETH_HLEN);
		pfe_vwd_send_packet( skb, priv, 0,  &priv->vaps[vapid], own_mac, 0);
		return NF_STOLEN;
	}

done:
	return NF_ACCEPT;

}

static DEVICE_ATTR(vwd_debug_stats, 0444, pfe_vwd_show_dump_stats, NULL);
static DEVICE_ATTR(vwd_fast_path_enable, 0644, pfe_vwd_show_fast_path_enable, pfe_vwd_set_fast_path_enable);
static DEVICE_ATTR(vwd_route_hook_enable, 0644, pfe_vwd_show_route_hook_enable, pfe_vwd_set_route_hook_enable);
static DEVICE_ATTR(vwd_bridge_hook_enable, 0644, pfe_vwd_show_bridge_hook_enable, pfe_vwd_set_bridge_hook_enable);
static DEVICE_ATTR(vwd_tso_stats, 0644, pfe_vwd_show_tso_stats, pfe_vwd_set_tso_stats);

static struct kobj_attribute direct_rx_attr =
		 __ATTR(direct_rx_path, 0644, pfe_vwd_show_direct_rx_path, pfe_vwd_set_direct_rx_path);
static struct kobj_attribute direct_tx_attr =
		 __ATTR(direct_tx_path, 0644, pfe_vwd_show_direct_tx_path, pfe_vwd_set_direct_tx_path);
#if defined(CONFIG_COMCERTO_CUSTOM_SKB_LAYOUT)
static struct kobj_attribute custom_skb_attr =
		 __ATTR(custom_skb_enable, 0644, pfe_vwd_show_custom_skb_enable, pfe_vwd_set_custom_skb_enable);
#endif
#if defined(CONFIG_SMP) && (NR_CPUS > 1)
static struct kobj_attribute rx_cpu_affinity_attr =
		 __ATTR(rx_cpu_affinity, 0644, pfe_vwd_show_rx_cpu_affinity, pfe_vwd_set_rx_cpu_affinity);
#endif
static struct kobj_attribute tso_enable_attr =
		 __ATTR(tso_enable, 0644, pfe_vwd_show_tso_enable, pfe_vwd_set_tso_enable);
static struct kobj_attribute lro_enable_attr =
		 __ATTR(lro_enable, 0644, pfe_vwd_show_lro_enable, pfe_vwd_set_lro_enable);
static struct kobj_attribute rx_csum_enable_attr =
		 __ATTR(rx_csum_enable, 0644, pfe_vwd_show_rx_csum_enable, pfe_vwd_set_rx_csum_enable);

static struct attribute *vap_attrs[] = {
	&direct_rx_attr.attr,
	&direct_tx_attr.attr,
	&lro_enable_attr.attr,
	&rx_csum_enable_attr.attr,
#if defined(CONFIG_COMCERTO_CUSTOM_SKB_LAYOUT)
	&custom_skb_attr.attr,
#endif
#if defined(CONFIG_SMP) && (NR_CPUS > 1)
	&rx_cpu_affinity_attr.attr,
#endif
	&tso_enable_attr.attr,
	NULL,
};

static struct attribute_group vap_attr_group = {
	.attrs = vap_attrs,
};

#ifdef PFE_VWD_NAPI_STATS
static DEVICE_ATTR(vwd_napi_stats, 0644, pfe_vwd_show_napi_stats, pfe_vwd_set_napi_stats);
#endif
static DEVICE_ATTR(vwd_vap_create, 0644, NULL, pfe_vwd_vap_create);
static DEVICE_ATTR(vwd_vap_remove, 0644, NULL, pfe_vwd_vap_remove);
#ifdef PFE_VWD_LRO_STATS
static DEVICE_ATTR(vwd_lro_nb_stats, 0644, pfe_vwd_show_lro_nb_stats, pfe_vwd_set_lro_nb_stats);
static DEVICE_ATTR(vwd_lro_len_stats, 0644, pfe_vwd_show_lro_len_stats, pfe_vwd_set_lro_len_stats);
#endif
/** pfe_vwd_sysfs_init
 *
 */
static int pfe_vwd_sysfs_init( struct pfe_vwd_priv_s *priv )
{
	struct pfe *pfe = priv->pfe;

	if (device_create_file(pfe->dev, &dev_attr_vwd_debug_stats))
		goto err_dbg_sts;

	if (device_create_file(pfe->dev, &dev_attr_vwd_fast_path_enable))
		goto err_fp_en;

	if (device_create_file(pfe->dev, &dev_attr_vwd_route_hook_enable))
		goto err_rt;

	if (device_create_file(pfe->dev, &dev_attr_vwd_bridge_hook_enable))
		goto err_br;

	if (device_create_file(pfe->dev, &dev_attr_vwd_vap_create))
		goto err_vap_add;

	if (device_create_file(pfe->dev, &dev_attr_vwd_vap_remove))
		goto err_vap_del;

	if (device_create_file(pfe->dev, &dev_attr_vwd_tso_stats))
		goto err_tso_stats;

#ifdef PFE_VWD_NAPI_STATS
	if (device_create_file(pfe->dev, &dev_attr_vwd_napi_stats))
		goto err_napi;
#endif

#ifdef PFE_VWD_LRO_STATS
	if (device_create_file(pfe->dev, &dev_attr_vwd_lro_nb_stats))
		goto err_lro_nb;

	if (device_create_file(pfe->dev, &dev_attr_vwd_lro_len_stats))
		goto err_lro_len;
#endif

	return 0;

#ifdef PFE_VWD_LRO_STATS
err_lro_len:
	device_remove_file(pfe->dev, &dev_attr_vwd_lro_nb_stats);
err_lro_nb:
#endif

#ifdef PFE_VWD_NAPI_STATS
	device_remove_file(pfe->dev, &dev_attr_vwd_napi_stats);
err_napi:
#endif

#if defined(PFE_VWD_LRO_STATS) || defined(PFE_VWD_NAPI_STATS)
	device_remove_file(pfe->dev, &dev_attr_vwd_tso_stats);
#endif

err_tso_stats:
	device_remove_file(pfe->dev, &dev_attr_vwd_vap_remove);
err_vap_del:
	device_remove_file(pfe->dev, &dev_attr_vwd_vap_create);
err_vap_add:
	device_remove_file(pfe->dev, &dev_attr_vwd_bridge_hook_enable);
err_br:
	device_remove_file(pfe->dev, &dev_attr_vwd_route_hook_enable);
err_rt:
	device_remove_file(pfe->dev, &dev_attr_vwd_fast_path_enable);
err_fp_en:
	device_remove_file(pfe->dev, &dev_attr_vwd_debug_stats);
err_dbg_sts:
	return -1;
}


/** pfe_vwd_sysfs_exit
 *
 */
static void pfe_vwd_sysfs_exit(void)
{
	device_remove_file(pfe->dev, &dev_attr_vwd_tso_stats);
#ifdef PFE_VWD_LRO_STATS
	device_remove_file(pfe->dev, &dev_attr_vwd_lro_len_stats);
	device_remove_file(pfe->dev, &dev_attr_vwd_lro_nb_stats);
#endif

#ifdef PFE_VWD_NAPI_STATS
	device_remove_file(pfe->dev, &dev_attr_vwd_napi_stats);
#endif
	device_remove_file(pfe->dev, &dev_attr_vwd_vap_create);
	device_remove_file(pfe->dev, &dev_attr_vwd_vap_remove);
	device_remove_file(pfe->dev, &dev_attr_vwd_bridge_hook_enable);
	device_remove_file(pfe->dev, &dev_attr_vwd_route_hook_enable);
	device_remove_file(pfe->dev, &dev_attr_vwd_fast_path_enable);
	device_remove_file(pfe->dev, &dev_attr_vwd_debug_stats);
}

/** pfe_vwd_up
 *
 */
static int pfe_vwd_up(struct pfe_vwd_priv_s *priv )
{
	printk("%s: start\n", __func__);

	nf_register_hook(&vwd_hook);
	nf_register_hook(&vwd_hook_ipv6);

	if (pfe_vwd_sysfs_init(priv))
		goto err0;

	priv->fast_path_enable = 0;
	priv->fast_bridging_enable = 0;
	priv->fast_routing_enable = 1;

	comcerto_wifi_rx_fastpath_register(vwd_wifi_if_send_pkt);
	return 0;

err0:
	nf_unregister_hook(&vwd_hook);
	nf_unregister_hook(&vwd_hook_ipv6);

	return -1;
}

/** pfe_vwd_down
 *
 */
static int pfe_vwd_down( struct pfe_vwd_priv_s *priv )
{
	int ii;

	printk(KERN_INFO "%s: %s\n", priv->name, __func__);

	comcerto_wifi_rx_fastpath_unregister();

	if( priv->fast_bridging_enable )
	{
		nf_unregister_hook(&vwd_hook_bridge);
	}

	if( priv->fast_routing_enable )
	{
		nf_unregister_hook(&vwd_hook);
		nf_unregister_hook(&vwd_hook_ipv6);
	}

	/*Stop Tx recovery timer and cleanup all vaps*/
	if (priv->vap_count) {
		printk("%s: Tx recover Timer stopped...\n", __func__);
		del_timer_sync(&priv->tx_timer);
	}

	for (ii = 0; ii < MAX_VAP_SUPPORT; ii++)
	{
		if (priv->vaps[ii].ifindex)
			pfe_vwd_vap_down(&priv->vaps[ii]);

		memset(&priv->vaps[ii], 0, sizeof(struct vap_desc_s));
	}

	priv->vap_count = 0;

	if(priv->tso_hook_enable)
		nf_unregister_hook(&vwd_hook_wifi_tx);

	pfe_vwd_sysfs_exit();

	return 0;
}

/** pfe_vwd_rx_page
 *
 */
static struct sk_buff *pfe_vwd_rx_page(struct vap_desc_s *vap, int qno, unsigned int *ctrl)
{
	struct page *p;
	void *buf_addr;
	unsigned int rx_ctrl;
	unsigned int desc_ctrl = 0;
	struct sk_buff *skb;
	int length, offset, data_offset;
	struct hif_lro_hdr *lro_hdr;
	u32 pe_id;
	struct pfe_vwd_priv_s *priv = vap->priv;


	while (1) {
		buf_addr = hif_lib_receive_pkt(&vap->client, qno, &length, &offset, &rx_ctrl, &desc_ctrl, (void **)&lro_hdr);

		if (!buf_addr)
			goto empty;

		if (qno == 2)
			pe_id = (rx_ctrl >> 16) & 0xf;
		else
			pe_id = 0;

		skb = vap->skb_inflight[qno + pe_id];

#ifdef PFE_VWD_NAPI_STATS
		priv->napi_counters[NAPI_DESC_COUNT]++;
#endif

		*ctrl = rx_ctrl;

		/* First frag */
		if ((desc_ctrl & CL_DESC_FIRST) && !skb) {
			p = virt_to_page(buf_addr);

			skb = dev_alloc_skb(MAX_HDR_SIZE + PFE_PKT_HEADROOM + 2);
			if (unlikely(!skb)) {
				goto pkt_drop;
			}

			skb_reserve(skb, PFE_PKT_HEADROOM + 2);

			if (lro_hdr) {
				data_offset = lro_hdr->data_offset;
				if (lro_hdr->mss)
					skb_shinfo(skb)->gso_size = lro_hdr->mss;

			//	printk(KERN_INFO "mss: %d, offset: %d, data_offset: %d, len: %d\n", lro_hdr->mss, offset, lro_hdr->data_offset, length);
			} else {
				data_offset = MAX_HDR_SIZE;
			}

			/* We don't need the fragment if the whole packet */
			/* has been copied in the first linear skb        */
			if (length <= data_offset) {
				__memcpy(skb->data, buf_addr + offset, length);
				skb_put(skb, length);
				free_page((unsigned long)buf_addr);
			} else {
				__memcpy(skb->data, buf_addr + offset, data_offset);
				skb_put(skb, data_offset);
				skb_add_rx_frag(skb, 0, p, offset + data_offset, length - data_offset);
			}

			if ((vap->dev->features & NETIF_F_RXCSUM) && (rx_ctrl & HIF_CTRL_RX_CHECKSUMMED))
			{
				skb->ip_summed = CHECKSUM_UNNECESSARY;
#ifdef VWD_DEBUG_STATS
				priv->rx_csum_correct++;
#endif
			}

		} else {
			/* Next frags */
			if (unlikely(!skb)) {
				printk(KERN_ERR "%s: NULL skb_inflight\n", __func__);
				goto pkt_drop;
			}

			p = virt_to_page(buf_addr);

			skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, p, offset, length);
		}

		/* Last buffer in a software chain */
		if ((desc_ctrl & CL_DESC_LAST) && !(rx_ctrl & HIF_CTRL_RX_CONTINUED))
			break;

		/* Keep track of skb for this queue/pe */
		vap->skb_inflight[qno + pe_id] = skb;
	}

	vap->skb_inflight[qno + pe_id] = NULL;

	return skb;

pkt_drop:
	vap->skb_inflight[qno + pe_id] = NULL;

	if (skb) {
		kfree_skb(skb);
	} else {
		free_page((unsigned long)buf_addr);
	}

	return NULL;

empty:
	return NULL;
}

/** pfe_vwd_rx_skb
 *
 */
static struct sk_buff *pfe_vwd_rx_skb(struct vap_desc_s *vap, int qno, unsigned int *ctrl)
{
	void *buf_addr;
	struct hif_ipsec_hdr *ipsec_hdr;
	unsigned int rx_ctrl;
	unsigned int desc_ctrl = 0;
	struct sk_buff *skb = NULL;
	int length = 0, offset;
	struct pfe_vwd_priv_s *priv = vap->priv;
#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
	struct timespec ktime;
#endif

	buf_addr = hif_lib_receive_pkt(&vap->client, qno, &length, &offset, &rx_ctrl, &desc_ctrl,(void **) &ipsec_hdr);
	if (!buf_addr)
		goto out;

	*ctrl = rx_ctrl;
#ifdef PFE_VWD_NAPI_STATS
		priv->napi_counters[NAPI_DESC_COUNT]++;
#endif

#if defined(CONFIG_COMCERTO_CUSTOM_SKB_LAYOUT)
	if ((vap->custom_skb) && !(rx_ctrl & HIF_CTRL_RX_WIFI_EXPT)) {
		/* Even we use smaller area allocate bigger buffer, to meet skb helper function's requirements */
		skb = dev_alloc_skb(length + offset + 32);

		if (unlikely(!skb)) {
#ifdef VWD_DEBUG_STATS
			priv->rx_skb_alloc_fail += 1;
#endif
			goto pkt_drop;
		}

		/**
		 *  __memcpy expects src and dst need to be same alignment. So make sure that
		 *  skb->data starts at same alignement as buf_addr + offset.
		 */
		skb_reserve(skb, offset);
		if (length <= MAX_WIFI_HDR_SIZE) {
			__memcpy(skb->data, buf_addr + offset, length);
			skb_put(skb, length);
			kfree(buf_addr);
		}
		else {
			__memcpy(skb->data, buf_addr + offset, MAX_WIFI_HDR_SIZE);
			skb_put(skb, length);
			skb->mspd_data = buf_addr;
			skb->mspd_len = length - MAX_WIFI_HDR_SIZE;
			skb->mspd_ofst = offset + MAX_WIFI_HDR_SIZE;
		}
	}

	else
#endif
	if (rx_ctrl & HIF_CTRL_RX_WIFI_EXPT) {
#if defined(CONFIG_COMCERTO_ZONE_DMA_NCNB)
		skb = dev_alloc_skb(length + offset + 32);
#else
		skb = alloc_skb_header(PFE_BUF_SIZE, buf_addr, GFP_ATOMIC);
#endif

		if (unlikely(!skb)) {
#ifdef VWD_DEBUG_STATS
			priv->rx_skb_alloc_fail += 1;
#endif
			goto pkt_drop;
		}

		skb_reserve(skb, offset);
#if defined(CONFIG_COMCERTO_ZONE_DMA_NCNB)
		/* Since, these packets are going to linux stack, 
                 * to avoid NCNB access overhead copy NCNB to CB buffer.
                 */
		__memcpy(skb->data, buf_addr + offset, length);
		kfree(buf_addr);
#endif
		skb_put(skb, length);


		if ((vap->dev->features & NETIF_F_RXCSUM) && (rx_ctrl & HIF_CTRL_RX_CHECKSUMMED))
		{
			skb->ip_summed = CHECKSUM_UNNECESSARY;
#ifdef VWD_DEBUG_STATS
			priv->rx_csum_correct++;
#endif
		}
#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
                        if (rx_ctrl & HIF_CTRL_RX_IPSEC_IN) {
                                if (ipsec_hdr) {
                                        struct sec_path *sp;
                                        struct xfrm_state *x;
                                        unsigned short *sah = &ipsec_hdr->sa_handle[0];
                                        int i = 0;

                                        sp = secpath_dup(skb->sp);

                                        if (!sp)
                                        {
                                                goto pkt_drop;
                                        }

                                        skb->sp = sp;

                                        /* at maximum 2 SA are expected */
                                        while (i <= 1)
                                        {
                                                if(!*sah)
                                                        break;

                                                if ((x = xfrm_state_lookup_byhandle(dev_net(vap->dev), ntohs(*sah))) == NULL)
                                                {
                                                        goto pkt_drop;
                                                }

                                                sp->xvec[i] = x;

                                                if (!x->curlft.use_time)
                                                {
                                                        ktime = current_kernel_time();
                                                        x->curlft.use_time = (unsigned long)ktime.tv_sec;
                                                }

                                                i++; sah++;
                                        }

                                        sp->len = i;
                                }
                        }
#endif

	}
	else
	{
		skb = alloc_skb_header(PFE_BUF_SIZE, buf_addr, GFP_ATOMIC);

		if (unlikely(!skb)) {
#ifdef VWD_DEBUG_STATS
			priv->rx_skb_alloc_fail += 1;
#endif
			goto pkt_drop;
		}

		skb_reserve(skb, offset);
		skb_put(skb, length);
	}


	return skb;

pkt_drop:
	if (skb) {
		kfree_skb(skb);
	} else {
		kfree(buf_addr);
	}

out:
	return NULL;
}

/** pfe_vwd_send_to_vap
 *
 */

/* The most of the logic inside this function is copied from dev_queue_xmit() in linux/net/core/dev.c.*/

static void pfe_vwd_send_to_vap(struct vap_desc_s *vap, struct sk_buff *skb, struct net_device *dev)
{
	struct netdev_queue *txq;
	int cpu, rc;

	if (!vap->direct_tx_path) {
		dev_queue_xmit(skb);
		return;
	}

	/* Disable soft irqs for various locks below. Also
         * stops preemption for RCU.
         */
        rcu_read_lock_bh();

	if (unlikely(dev->real_num_tx_queues != 1)) {
		//printk("%s : number of queues : %d\n", __func__, dev->real_num_tx_queues);
		goto deliver_slow;
	}

	if (dev->flags & IFF_UP) {
		skb_set_queue_mapping(skb, 0);
		txq = netdev_get_tx_queue(dev, 0);

		cpu = smp_processor_id();

		if (txq->xmit_lock_owner != cpu) {
 			HARD_TX_LOCK(dev, txq, cpu);

			if (unlikely(netif_tx_queue_stopped(txq))) {
				//printk("%s : stopped \n", __func__);
 				HARD_TX_UNLOCK(dev, txq);
				goto deliver_slow;
			}

			rc = dev->netdev_ops->ndo_start_xmit(skb, dev);

			if (dev_xmit_complete(rc)) {
 				HARD_TX_UNLOCK(dev, txq);
				goto done;
			}
		}
	}

	rcu_read_unlock_bh();
	kfree_skb(skb);

	return;

done:
	//printk("%s : devivered packet through fast path\n", __func__);
	rcu_read_unlock_bh();
	return;

deliver_slow:
	rcu_read_unlock_bh();

	/* deliver packet to vap through stack */
	dev_queue_xmit(skb);
	return;
}

/** pfe_vwd_rx_poll
 *
 */
static int pfe_vwd_rx_poll( struct vap_desc_s *vap, struct napi_struct *napi, int qno, int budget)
{
	struct sk_buff *skb;
	int work_done = 0;
	struct net_device *dev;
	struct pfe_vwd_priv_s *priv = vap->priv;

	//printk(KERN_INFO"%s\n", __func__);
	if ((dev = dev_get_by_index(&init_net, vap->ifindex)) == NULL) {
		printk(KERN_ERR"%s : VAP is down... This should not happend\n", __func__);
		goto done;
	}

#ifdef PFE_VWD_NAPI_STATS
	priv->napi_counters[NAPI_POLL_COUNT]++;
#endif
	do {
		unsigned int ctrl = 0;

		if (page_mode)
			skb = pfe_vwd_rx_page(vap, qno, &ctrl);
		else
			skb = pfe_vwd_rx_skb(vap, qno, &ctrl);

		if (!skb)
			break;

		skb->dev = dev;
		dev->last_rx = jiffies;

#ifdef PFE_VWD_LRO_STATS
		priv->lro_len_counters[((u32)skb->len >> 11) & (LRO_LEN_COUNT_MAX - 1)]++;
		priv->lro_nb_counters[skb_shinfo(skb)->nr_frags & (LRO_NB_COUNT_MAX - 1)]++;
#endif

		/*FIXME: Need to handle WiFi to WiFi fast path */
		if (ctrl & HIF_CTRL_RX_WIFI_EXPT) {
			//printk("%s : packet sent to expt\n", __func__);

			*(unsigned long *)skb->head = 0xdead;
			skb->protocol = eth_type_trans(skb, dev);
#ifdef VWD_DEBUG_STATS
                        priv->pkts_slow_forwarded[qno] += 1;
#endif
			netif_receive_skb(skb);
		}
		else {
			struct ethhdr *hdr;

                        hdr = (struct ethhdr *)skb->data;
                        skb->protocol = hdr->h_proto;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
                        skb->mac.raw = skb->data;
                        skb->nh.raw = skb->data + sizeof(struct ethhdr);
#else
                        skb_reset_mac_header(skb);
                        skb_set_network_header(skb, sizeof(struct ethhdr));
#endif

#ifdef VWD_DEBUG_STATS
			priv->pkts_rx_fast_forwarded[qno] += 1;
#endif
                        skb->priority = 0;


			pfe_vwd_send_to_vap(vap, skb, dev);
		}


		work_done++;
#ifdef PFE_VWD_NAPI_STATS
		priv->napi_counters[NAPI_PACKET_COUNT]++;
#endif
	} while (work_done < budget);

	dev_put(dev);

done:
	/* If no Rx receive nor cleanup work was done, exit polling mode.
	 * No more netif_running(dev) check is required here , as this is checked in
	 * net/core/dev.c ( 2.6.33.5 kernel specific).
	 */
	if (work_done < budget) {
		napi_complete(napi);

		hif_lib_event_handler_start(&vap->client, EVENT_RX_PKT_IND, qno);
	}
#ifdef PFE_VWD_NAPI_STATS
	else
		priv->napi_counters[NAPI_FULL_BUDGET_COUNT]++;
#endif

	return work_done;
}

/** pfe_vwd_lro_poll
 */
static int pfe_vwd_lro_poll(struct napi_struct *napi, int budget)
{
	struct vap_desc_s *vap = container_of(napi, struct vap_desc_s, lro_napi);


	return pfe_vwd_rx_poll(vap, napi, 2, budget);
}


/** pfe_eth_low_poll
 */
static int pfe_vwd_rx_high_poll(struct napi_struct *napi, int budget)
{
	struct vap_desc_s *vap = container_of(napi, struct vap_desc_s, high_napi);

	return pfe_vwd_rx_poll(vap, napi, 1, budget);
}

/** pfe_eth_high_poll
 */
static int pfe_vwd_rx_low_poll(struct napi_struct *napi, int budget )
{
	struct vap_desc_s *vap = container_of(napi, struct vap_desc_s, low_napi);

	return pfe_vwd_rx_poll(vap, napi, 0, budget);
}

/** pfe_vwd_event_handler
 */
static int pfe_vwd_event_handler(void *data, int event, int qno)
{
	struct vap_desc_s *vap = data;

	//printk(KERN_INFO "%s: %d\n", __func__, __LINE__);

	switch (event) {
		case EVENT_RX_PKT_IND:
			if (qno == 0) {
				if (napi_schedule_prep(&vap->low_napi)) {
					//printk(KERN_INFO "%s: schedule high prio poll\n", __func__);

					__napi_schedule(&vap->low_napi);
				}
			}
			else if (qno == 1) {
				if (napi_schedule_prep(&vap->high_napi)) {
					//printk(KERN_INFO "%s: schedule high prio poll\n", __func__);

					__napi_schedule(&vap->high_napi);
				}
			}
			else if (qno == 2) {
				if (napi_schedule_prep(&vap->lro_napi)) {
					//printk(KERN_INFO "%s: schedule lro poll\n", __func__);

					__napi_schedule(&vap->lro_napi);
				}
			}

#ifdef PFE_VWD_NAPI_STATS
			vap->priv->napi_counters[NAPI_SCHED_COUNT]++;
#endif
			break;

		case EVENT_TXDONE_IND:
		case EVENT_HIGH_RX_WM:
		default:
			break;
	}

	return 0;
}



/** pfe_vwd_driver_init
 *
 *	 PFE wifi offload:
 *	 - uses HIF functions to receive/send packets
 */

static int pfe_vwd_driver_init( struct pfe_vwd_priv_s *priv )
{
	printk("%s: start\n", __func__);

	strcpy(priv->name, "vwd");

	spin_lock_init(&priv->vaplock);
	priv->pfe = pfe;

	pfe_vwd_up(priv);
	printk("%s: end\n", __func__);
	return 0;
}


/** vwd_driver_remove
 *
 */
static int pfe_vwd_driver_remove(void)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	pfe_vwd_down(priv);

	return 0;
}

/** pfe_vwd_vap_up
 *
 */
static int pfe_vwd_vap_up(struct vap_desc_s *vap)
{
	struct pfe_vwd_priv_s *priv = vap->priv;
	struct hif_client_s *client;
	int rc = 0, ii;

	printk("%s:%d\n", __func__, __LINE__);
	/* Initilize NAPI for Rx processing */
	init_dummy_netdev(&vap->dummy_dev);
	netif_napi_add(&vap->dummy_dev, &vap->low_napi, pfe_vwd_rx_low_poll, VWD_RX_POLL_WEIGHT);
	netif_napi_add(&vap->dummy_dev, &vap->high_napi, pfe_vwd_rx_high_poll, VWD_RX_POLL_WEIGHT);
	netif_napi_add(&vap->dummy_dev, &vap->lro_napi, pfe_vwd_lro_poll, VWD_RX_POLL_WEIGHT);
	napi_enable(&vap->high_napi);
	napi_enable(&vap->low_napi);
	napi_enable(&vap->lro_napi);

	vap->vap_kobj = kobject_create_and_add(vap->ifname, &pfe->dev->kobj);
	if (!vap->vap_kobj) {
		printk(KERN_ERR "%s : Failed to create kobject\n", __func__);
		goto err0;
	}

	rc = sysfs_create_group(vap->vap_kobj, &vap_attr_group);
	if (rc) {
		printk(KERN_ERR "%s : Failed to create sysfs entries \n", __func__);
		goto err1;
	}


	/* Register VWD Client driver with HIF */
	client = &vap->client;
	memset(client, 0, sizeof(*client));
	client->id = PFE_CL_VWD0 + vap->vapid;
	client->tx_qn = VWD_TXQ_CNT;
	client->rx_qn = VWD_RXQ_CNT;
	client->priv    = vap;
	client->pfe     = priv->pfe;
	client->event_handler   = pfe_vwd_event_handler;
	client->user_cpu_id  = vap->cpu_id;

	/* FIXME : For now hif lib sets all tx and rx queues to same size */
	client->tx_qsize = EMAC_TXQ_DEPTH;
	client->rx_qsize = EMAC_RXQ_DEPTH;

	if ((rc = hif_lib_client_register(client))) {
		printk(KERN_ERR"%s: hif_lib_client_register(%d) failed\n", __func__, client->id);
		goto err2;
	}

	for (ii = 0; ii < VWD_TXQ_CNT; ii++)
		spin_lock_init(&vap->tx_lock[ii]);

	return 0;

err2:
	sysfs_remove_group(vap->vap_kobj, &vap_attr_group);

err1:
	kobject_put(vap->vap_kobj);

err0:
	napi_disable(&vap->high_napi);
	napi_disable(&vap->low_napi);
	napi_disable(&vap->lro_napi);
	return rc;
}

/** pfe_vwd_vap_down
 *
 */
static void pfe_vwd_vap_down(struct vap_desc_s *vap)
{
	printk("%s:%d\n", __func__, __LINE__);
	pfe_vwd_flush_tx(vap, 1);
	hif_lib_client_unregister(&vap->client);
	napi_disable(&vap->high_napi);
	napi_disable(&vap->low_napi);
	napi_disable(&vap->lro_napi);
	sysfs_remove_group(vap->vap_kobj, &vap_attr_group);
	kobject_put(vap->vap_kobj);
}


/** pfe_vwd_handle_vap
 *
 */
static int pfe_vwd_handle_vap( struct pfe_vwd_priv_s *vwd, struct vap_cmd_s *cmd )
{
	struct vap_desc_s *vap;
	int rc = 0, ii;
	struct net_device *dev;


	printk("%s function called %d: %s\n", __func__, cmd->action, cmd->ifname);

	dev = dev_get_by_name(&init_net, cmd->ifname);

	if ((!dev) && ((cmd->action != REMOVE) && (cmd->action != RESET)))
		return -EINVAL;


	switch( cmd->action )
	{
		case ADD:
			/* Find free VAP */

			if( cmd->vapid >= MAX_VAP_SUPPORT )
			{
				rc = -EINVAL;
				goto done;
			}


			vap = &vwd->vaps[cmd->vapid];

			if( vap->ifindex )
			{
				rc = -EINVAL;
				break;
			}

			vap->vapid = cmd->vapid;
			vap->ifindex = cmd->ifindex;
			vap->direct_rx_path = cmd->direct_rx_path;
			vap->direct_tx_path = 0;
			memcpy(vap->ifname, cmd->ifname, 12);
			memcpy(vap->macaddr, cmd->macaddr, 6);

			vap->dev = dev;
			vap->priv = vwd;
			vap->cpu_id = -1;

			if (!pfe_vwd_vap_up(vap)) {
				printk("%s:ADD: name:%s, vapid:%d, direct_rx_path : %s, ifindex:%d, mac:%x:%x:%x:%x:%x:%x\n",
                                	        __func__, vap->ifname, vap->vapid,
						vap->direct_rx_path ? "ON":"OFF", vap->ifindex,
						vap->macaddr[0], vap->macaddr[1],
						vap->macaddr[2], vap->macaddr[3],
						vap->macaddr[4], vap->macaddr[5] );

				if (!vwd->vap_count) {
					printk("%s: Tx recover Timer started...\n", __func__);
					add_timer(&vwd->tx_timer);
				}

				vwd->vap_count++;
			}
			else {
				printk(KERN_ERR "%s: Unable to add VAP (%s)\n", __func__, cmd->ifname);
				memset(vap, 0, sizeof(*vap));
			}
			break;

		case REMOVE:
			/* Find  VAP to be removed*/
			if( cmd->vapid >= MAX_VAP_SUPPORT )
			{
				rc = -EINVAL;
				goto done;
			}

			vap = &vwd->vaps[cmd->vapid];

			if( !vap->ifindex )
			{
				rc = 0;
				goto done;
			}

			printk("%s:REMOVE: name:%s, vapid:%d ifindex:%d mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->ifname, vap->vapid, vap->ifindex,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );

			pfe_vwd_vap_down(vap);
			memset(vap, 0, sizeof(*vap));

			vwd->vap_count--;

			if (!vwd->vap_count) {
				printk("%s: Tx recover Timer stopped...\n", __func__);
				spin_unlock_bh(&vwd->vaplock);
				del_timer_sync(&vwd->tx_timer);
				spin_lock_bh(&vwd->vaplock);
			}

			break;

		case UPDATE:
			/* Find VAP to be updated */

			if( cmd->vapid >= MAX_VAP_SUPPORT )
			{
				rc = -EINVAL;
				goto done;
			}

			vap = &vwd->vaps[cmd->vapid];

			if ( !vap->ifindex )
			{
				rc = -EINVAL;
				goto done;
			}

			printk("%s:UPDATE: old mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );

			/* Not yet implemented */
			memcpy(vap->macaddr, cmd->macaddr, 6);

			printk("%s:UPDATE: name:%s, vapid:%d ifindex:%d mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->ifname, vap->vapid, vap->ifindex,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );
			break;

		case RESET:
			/* Remove all VAPs */
			printk("%s: Removing fastpath vaps\n", __func__);
			for (ii = 0; (ii < MAX_VAP_SUPPORT) && vwd->vap_count; ii++) {
				vap = &vwd->vaps[ii];
				if (vap->ifindex) {
					pfe_vwd_vap_down(vap);
					memset(vap, 0, sizeof(*vap));

					vwd->vap_count--;
				}

				if (!vwd->vap_count) {
					printk("%s: Tx recover Timer stopped...\n", __func__);
					spin_unlock_bh(&vwd->vaplock);
					del_timer_sync(&vwd->tx_timer);
					spin_lock_bh(&vwd->vaplock);
				}
			}
			break;


		default:
			rc =  -EINVAL;

	}
done:

	if(dev)
		dev_put(dev);


	return rc;

}

#define SIOCVAPUPDATE  ( 0x6401 )

/** pfe_vwd_ioctl
 *
 */
static long
pfe_vwd_ioctl(struct file * file, unsigned int cmd, unsigned long arg)
{
	struct vap_cmd_s vap_cmd;
	void __user *argp = (void __user *)arg;
	int rc;
	struct pfe_vwd_priv_s *priv = (struct pfe_vwd_priv_s *)file->private_data;

	printk("%s: start\n", __func__);
	switch(cmd) {
		case SIOCVAPUPDATE:
			if (copy_from_user(&vap_cmd, argp, sizeof(struct vap_cmd_s)))
				return -EFAULT;

			spin_lock_bh(&priv->vaplock);
			rc = pfe_vwd_handle_vap(priv, &vap_cmd);
			spin_unlock_bh(&priv->vaplock);

			return rc;
	}
	printk("%s: end\n", __func__);

	return -EOPNOTSUPP;
}


/** vwd_open
 *
 */
	static int
pfe_vwd_open(struct inode *inode, struct file *file)
{
	int result = 0;
	unsigned int dev_minor = MINOR(inode->i_rdev);

#if defined (CONFIG_COMCERTO_VWD_MULTI_MAC)
	printk( "%s :  Multi MAC mode enabled\n", __func__);
#endif
	printk( "%s :  minor device -> %d\n", __func__, dev_minor);
	if (dev_minor != 0)
	{
		printk(KERN_ERR ": trying to access unknown minor device -> %d\n", dev_minor);
		result = -ENODEV;
		goto out;
	}

	file->private_data = &pfe->vwd;

out:
	return result;
}


/** pfe_vwd_wifi_local_tx
 *
 */
static unsigned pfe_vwd_wifi_local_tx( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	u32 ctrl = HIF_CTRL_TX_WIFI;
	u8 vapid = 0, l3_proto;
	struct vap_desc_s *vap;
	u32 ret = NF_ACCEPT;

	spin_lock_bh(&priv->vaplock);

	for (vapid = 0; vapid < MAX_VAP_SUPPORT; vapid++) {

		vap = &priv->vaps[vapid];

		if( ( vap->ifindex ) && ( vap->dev == skb->dev) && (vap->dev->features & NETIF_F_TSO)) {
			/* allow only ipv4 and ipv6 packets */
			if (htons(skb->protocol) == ETH_P_IP) {
				struct iphdr *ip = (struct iphdr *)skb_network_header(skb);

				l3_proto = ip->protocol;
			} else if (htons(skb->protocol) == ETH_P_IPV6) {
				struct ipv6hdr *ipv6 = (struct ipv6hdr *)skb_network_header(skb);

				l3_proto = ipv6->nexthdr;
			} else
				goto done;

			/* FIXME allow only TCP/UDP packets */
			if ((l3_proto != IPPROTO_UDP) && (l3_proto != IPPROTO_TCP))
				goto done;

#ifdef VWD_DEBUG_STATS
			priv->pkts_total_local_tx++;
#endif
			if (skb->ip_summed == CHECKSUM_PARTIAL) {
				ctrl |= HIF_CTRL_TX_CHECKSUM;
#ifdef VWD_DEBUG_STATS
				priv->pkts_local_tx_csum++;
#endif
			}
			skb_push(skb, ETH_HLEN);

			pfe_vwd_send_packet( skb, priv, 0, &priv->vaps[vapid], 0, ctrl);

			ret = NF_STOLEN;
			goto done;
		}
	}
done:
	spin_unlock_bh(&priv->vaplock);
	return ret;
}

/** vwd_close
 *
 */
	static int
pfe_vwd_close(struct inode * inode, struct file * file)
{
	printk( "%s \n", __func__);

	return 0;
}

struct file_operations vwd_fops = {
unlocked_ioctl:	pfe_vwd_ioctl,
		open:		pfe_vwd_open,
		release:	pfe_vwd_close,
};


/** pfe_vwd_init
 *
 */
int pfe_vwd_init(struct pfe *pfe)
{
	struct pfe_vwd_priv_s	*priv ;
	int rc = 0;

	printk(KERN_INFO "%s\n", __func__);
	priv  = &pfe->vwd;
	memset(priv, 0, sizeof(*priv));


	rc = alloc_chrdev_region(&priv->char_devno,VWD_MINOR, VWD_MINOR_COUNT, VWD_DRV_NAME);
	if (rc < 0) {
		printk(KERN_ERR "%s: alloc_chrdev_region() failed\n", __func__);
		goto err0;
	}

	cdev_init(&priv->char_dev, &vwd_fops);
	priv->char_dev.owner = THIS_MODULE;

	rc = cdev_add (&priv->char_dev, priv->char_devno, VWD_DEV_COUNT);
	if (rc < 0) {
		printk(KERN_ERR "%s: cdev_add() failed\n", __func__);
		goto err1;
	}

	printk(KERN_INFO "%s: created vwd device(%d, %d)\n", __func__, MAJOR(priv->char_devno),
			MINOR(priv->char_devno));

	priv->pfe = pfe;

	priv->tx_timer.data = (unsigned long)priv;
	priv->tx_timer.function = pfe_vwd_tx_timeout;
	priv->tx_timer.expires = jiffies + ( COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ )/1000;
	init_timer(&priv->tx_timer);

	if( pfe_vwd_driver_init( priv ) )
		goto err1;

	return 0;

err1:

	unregister_chrdev_region(priv->char_devno, VWD_MINOR_COUNT);

err0:
	return rc;
}

/** pfe_vwd_exit
 *
 */
void pfe_vwd_exit(struct pfe *pfe)
{
	struct pfe_vwd_priv_s	*priv = &pfe->vwd;

	printk(KERN_INFO "%s\n", __func__);

	pfe_vwd_driver_remove();
	cdev_del(&priv->char_dev);
	unregister_chrdev_region(priv->char_devno, VWD_MINOR_COUNT);
}

#else /* !CFG_WIFI_OFFLOAD */

/** pfe_vwd_init
 *
 */
int pfe_vwd_init(struct pfe *pfe)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

/** pfe_vwd_exit
 *
 */
void pfe_vwd_exit(struct pfe *pfe)
{
	printk(KERN_INFO "%s\n", __func__);
}

#endif /* !CFG_WIFI_OFFLOAD */

