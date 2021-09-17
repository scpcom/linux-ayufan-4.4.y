#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include <linux/platform_device.h>
#include <linux/module.h>

#ifdef CONFIG_ARCH_MVEBU
#include "mvNetConfig.h"
#else
#include "ctrlEnv/mvCtrlEnvSpec.h"
#endif

#include "mvDebug.h"
#include "mv_switch.h"

#include "mv_mux_netdev.h"
#include "mv_mux_tool.h"

static struct notifier_block mux_notifier_block __read_mostly;
static const struct net_device_ops mv_mux_netdev_ops;
static struct  mv_mux_switch_port  mux_switch_shadow;
struct  mv_mux_eth_port mux_eth_shadow[MV_ETH_MAX_PORTS];

static const struct  mv_mux_switch_ops *switch_ops;

static int mux_init_cnt;

static struct  mv_mux_eth_ops	*eth_ops;

static const struct  mv_switch_mux_ops mux_ops;

static inline struct net_device *mv_mux_rx_netdev_get(int port, struct sk_buff *skb);
static inline int mv_mux_rx_tag_remove(struct net_device *dev, struct sk_buff *skb);
static inline int mv_mux_tx_skb_tag_add(struct net_device *dev, struct sk_buff *skb);
static int mv_mux_netdev_delete_all(int port);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
void netif_stacked_transfer_operstate(const struct net_device *rootdev,
										struct net_device *dev)
{
	if (rootdev->operstate == IF_OPER_DORMANT)
		netif_dormant_on(dev);
	else
		netif_dormant_off(dev);

	if (netif_carrier_ok(rootdev)) {
		if (!netif_carrier_ok(dev))
			netif_carrier_on(dev);
	} else {
		if (netif_carrier_ok(dev))
			netif_carrier_off(dev);
	}
}
#endif
 
static int mv_mux_mgr_create(char *name, int gbe_port, int group, MV_MUX_TAG *tag)
{
	struct net_device *mux_dev;
	unsigned char broadcast[MV_MAC_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	char *unicast;

	mux_dev = mv_mux_netdev_alloc(name, group, tag);
	if (mux_dev == NULL) {
		printk(KERN_ERR "%s: mv_mux_netdev_alloc falied\n", __func__);
		return MV_FAIL;
	}
	mv_mux_netdev_add(gbe_port, mux_dev);

	if (switch_ops && switch_ops->group_cookie_set)
		switch_ops->group_cookie_set(group, mux_dev);

	unicast = mv_mux_get_mac(mux_dev);

	if (switch_ops && switch_ops->mac_addr_set) {
		switch_ops->mac_addr_set(group, unicast, 1);
		switch_ops->mac_addr_set(group, broadcast, 1);
	}

	return 0;
}
 
static int mv_mux_mgr_init(MV_SWITCH_PRESET_TYPE preset, int vid, MV_TAG_TYPE tag_mode, int gbe_port)
{
	char name[7] = {0, 0, 0, 0, 0, 0, 0};
	MV_MUX_TAG tag;
	unsigned int g;

	for (g = 0; g < MV_SWITCH_DB_NUM; g++) {
		 
		if (switch_ops && switch_ops->tag_get)
			if (switch_ops->tag_get(g, tag_mode, preset, vid, &tag)) {
				 
				sprintf(name, "mux%d", g);
				 
				mv_mux_mgr_create(name, gbe_port, g, &tag);
			}

	}

	return 0;
}
 
static int mv_mux_mgr_probe(int gbe_port)
{
	MV_TAG_TYPE tag_mode = mux_switch_shadow.tag_type;
	MV_SWITCH_PRESET_TYPE preset = mux_switch_shadow.preset;
	int vid = mux_switch_shadow.vid;

	if (switch_ops && switch_ops->preset_init)
		switch_ops->preset_init(tag_mode, preset, vid);

	mv_mux_tag_type_set(gbe_port, tag_mode);

	mv_mux_mgr_init(preset, vid, tag_mode, gbe_port);

	if (tag_mode != MV_TAG_TYPE_NONE) {
		rtnl_lock();
		dev_set_promiscuity(mux_eth_shadow[gbe_port].root, 1);
		rtnl_unlock();
	}

	if (switch_ops && switch_ops->interrupt_unmask)
		switch_ops->interrupt_unmask();

	printk(KERN_ERR "port #%d establish switch connection\n\n", gbe_port);

	return 0;
}

int mv_mux_switch_ops_set(const struct mv_mux_switch_ops *switch_ops_ptr)
{
	switch_ops = switch_ops_ptr;

	return 0;
}

static inline bool mv_mux_internal_switch(int port)
{
	 
	return ((mux_switch_shadow.attach) && (mux_switch_shadow.gbe_port == port));
}

void mv_mux_shadow_print(int gbe_port)
{
	struct mv_mux_eth_port shadow;
	static const char * const tags[] = {"None", "mh", "dsa", "edas", "vlan"};

#if defined(MY_ABC_HERE)
	if (gbe_port >= MV_ETH_MAX_PORTS) {
		pr_err("gbe port %d is out of range (0..%d)\n", gbe_port, MV_ETH_MAX_PORTS-1);
		return;
	}
#endif  

	if (mux_eth_shadow[gbe_port].root == NULL)
		printk(KERN_ERR "gbe port %d is not attached.\n", gbe_port);

	shadow = mux_eth_shadow[gbe_port];
		printk(KERN_ERR "\n");
		printk(KERN_ERR "port #%d: tag type=%s, switch_dev=0x%p, root_dev = 0x%p, flags=0x%x\n",
			gbe_port, tags[shadow.tag_type],
			shadow.switch_dev, shadow.root, (unsigned int)shadow.flags);

	mv_mux_netdev_print_all(gbe_port);
}

void mv_mux_switch_attach(int gbe_port, int preset, int vid, int tag, int switch_port)
{
	 
	if (mux_switch_shadow.attach)
		return;

	mux_switch_shadow.tag_type = tag;
	mux_switch_shadow.preset = preset;
	mux_switch_shadow.vid = vid;
	mux_switch_shadow.switch_port = switch_port;
	mux_switch_shadow.gbe_port = gbe_port;
	mux_switch_shadow.attach = MV_TRUE;
	 
	mux_switch_shadow.mtu = -1;

#ifdef CONFIG_MV_INCLUDE_SWITCH
	mv_switch_mux_ops_set(&mux_ops);
#endif

	if (mux_eth_shadow[gbe_port].root)
		 
		mv_mux_mgr_probe(gbe_port);
}

void mv_mux_eth_attach(int port, struct net_device *root, struct mv_mux_eth_ops *ops)
{
	 
	if (mux_eth_shadow[port].root)
		return;

	mux_eth_shadow[port].root = root;

	eth_ops = ops;

	if (mux_switch_shadow.attach && (mux_switch_shadow.gbe_port == port))
		 
		mv_mux_mgr_probe(port);
}
EXPORT_SYMBOL(mv_mux_eth_attach);

void mv_mux_eth_detach(int port)
{
	 
	if (mux_eth_shadow[port].root == NULL)
		return;

	mv_mux_netdev_delete_all(port);

	memset(&mux_eth_shadow[port], 0, sizeof(struct mv_mux_eth_port));
}
EXPORT_SYMBOL(mv_mux_eth_detach);
 
int mv_mux_netdev_find(unsigned int dev_idx)
{
	int port;
	struct net_device *root;

	for (port = 0; port < MV_ETH_MAX_PORTS; port++) {
		root = mux_eth_shadow[port].root;

		if (root && (root->ifindex == dev_idx))
			return port;
	}
	return -1;
}
EXPORT_SYMBOL(mv_mux_netdev_find);
 
int mv_mux_update_link(void *cookie, int link_up)
{
	struct net_device *mux_dev = (struct net_device *)cookie;

	(link_up) ? netif_carrier_on(mux_dev) : netif_carrier_off(mux_dev);

	return 0;
}

static inline int mv_mux_get_tag_size(MV_TAG_TYPE type)
{
	static const int size_arr[] = {0, MV_ETH_MH_SIZE,
					MV_ETH_DSA_SIZE,
					MV_TAG_TYPE_EDSA,
					MV_TAG_TYPE_VLAN};
	return size_arr[type];
}
 
static inline int mv_mux_dsa2vlan(struct net_device *mux_dev, struct sk_buff *skb, bool is_edsa)
{
	u8 *dsa_header;
	int source_device;
	int source_port;
	int len;
#if defined(MY_ABC_HERE) && !defined(CONFIG_MV_ETH_DEBUG_CODE)
	 
#else
	struct mux_netdev *pdev = MV_MUX_PRIV(mux_dev);
#endif

	dsa_header = skb->data + (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE;
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[pdev->port].flags & MV_MUX_F_DBG_RX) {
		pr_info("dsa_header WL = 0x%.8x", ntohl(*((u32 *)dsa_header)));
		if (is_edsa)
			pr_info(" WH = 0x%.8x\n", ntohl(*(((u32 *)dsa_header) + 1)));
		else
			pr_info("\n");
	}
#endif

	if (MV_DSA_HDR_TAG_CMD_GET(dsa_header) != MV_DSA_HDR_TAG_CMD_TO_CPU &&
		MV_DSA_HDR_TAG_CMD_GET(dsa_header) != MV_DSA_HDR_TAG_CMD_FORWARD) {
		pr_err("Invalid (E)DSA type\n");
		return -1;
	}

	source_device = MV_DSA_HDR_SRC_DEV_GET(dsa_header);
	source_port = MV_DSA_HDR_SRC_PORT_GET(dsa_header);
	if (is_edsa) {
		 
		if (MV_EDSA_HDR_SRC_PORT_BIT5_GET(dsa_header))
			source_port |= 0x20;
	}

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[pdev->port].flags & MV_MUX_F_DBG_RX)
		pr_info("source_device = 0x%x, source_port=0x%x", source_device, source_port);
#endif

	if (MV_DSA_HDR_TAGGED(dsa_header)) {
		u8 new_header[4];
		u16 tpid = ETH_P_8021Q;

		new_header[0] = (tpid >> 8) & 0xff;
		new_header[1] = tpid & 0xff;
		new_header[2] = dsa_header[2] & ~0x10;
		new_header[3] = dsa_header[3];

		if (is_edsa) {
			if (dsa_header[8] & 0x40)
				new_header[2] |= 0x10;
		} else {
			if (dsa_header[1] & 0x01)
				new_header[2] |= 0x10;
		}

		if (skb->ip_summed == CHECKSUM_COMPLETE) {
			__wsum c = skb->csum;
			c = csum_add(c, csum_partial(new_header + 2, 2, 0));
			c = csum_sub(c, csum_partial(dsa_header + 2, 2, 0));
			skb->csum = c;
		}

		memcpy(dsa_header, new_header, MV_ETH_DSA_SIZE);
		len = 0;
		if (is_edsa) {
			 
			memmove(skb->data + 4, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE + 4);
			__skb_pull(skb, 4);
			len = 4;
		}

		__skb_pull(skb, MV_ETH_MH_SIZE);
		len += MV_ETH_MH_SIZE;
	} else {
		 
		len = mv_mux_rx_tag_remove(mux_dev, skb);
	}

	return len;
}

static inline int mv_mux_vlan2dsa(struct sk_buff *skb, u32 dsa)
{
	u8 *dsa_header;
	 
	u8 trg_dev = (dsa >> MV_DSA_HDR_TRG_DEV_WORD_OFF) & MV_DSA_HDR_TRG_DEV_MASK;
	u8 trg_port = (dsa >> MV_DSA_HDR_TRG_PORT_WORD_OFF) & MV_DSA_HDR_TRG_PORT_MASK;

	if (skb->protocol == htons(ETH_P_8021Q)) {
		if (skb_cow_head(skb, 0) < 0)
			return -1;

		dsa_header = skb->data + 2 * MV_MAC_ADDR_SIZE;
		dsa_header[0] = 0x60 | trg_dev;
		dsa_header[1] = trg_port << 3;

		if (dsa_header[2] & 0x10) {
			dsa_header[1] |= 0x01;
			dsa_header[2] &= ~0x10;
		}
	} else {
		if (skb_cow_head(skb, MV_ETH_DSA_SIZE) < 0)
			return -1;
		skb_push(skb, MV_ETH_DSA_SIZE);

		memmove(skb->data, skb->data + MV_ETH_DSA_SIZE, 2 * MV_MAC_ADDR_SIZE);

		dsa_header = skb->data + (2 * MV_MAC_ADDR_SIZE);
		dsa_header[0] = 0x40;
		dsa_header[1] = trg_port << 3;
		dsa_header[2] = 0x00;
		dsa_header[3] = 0x00;
	}

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	pr_info("trg_dev = 0x%x, trg_port = 0x%x, dsa header = 0x%.8x\n",
		trg_dev, trg_port, ntohl(*(u32 *)dsa_header));
#endif

	return 0;
}

static inline int mv_mux_vlan2edsa(struct sk_buff *skb, unsigned int edsaL, unsigned int edsaH)
{
	u8 *dsa_header;
	 
	u8 trg_dev = (edsaL >> MV_DSA_HDR_TRG_DEV_WORD_OFF) & MV_DSA_HDR_TRG_DEV_MASK;
	u8 trg_port = (edsaL >> MV_DSA_HDR_TRG_PORT_WORD_OFF) & MV_DSA_HDR_TRG_PORT_MASK;

	if (skb->protocol == htons(ETH_P_8021Q)) {
		if (skb_cow_head(skb, 0) < 0)
			return -1;

		skb_push(skb, MV_ETH_DSA_SIZE);

		memmove(skb->data, skb->data + MV_ETH_DSA_SIZE, 2 * MV_MAC_ADDR_SIZE + MV_ETH_DSA_SIZE);

		dsa_header = skb->data + 2 * MV_MAC_ADDR_SIZE;
		dsa_header[0] = 0x60 | trg_dev;
		dsa_header[1] = trg_port << 3;

		if (dsa_header[2] & 0x10) {
			dsa_header[4] |= 0x40;
			dsa_header[2] &= ~0x10;
		}

		dsa_header[2] |= 0x10;
		dsa_header[4] &= 0x7f;
		 
		if (edsaH & 0x400)
			dsa_header[6] |= 0x04;
	} else {
		if (skb_cow_head(skb, MV_ETH_EDSA_SIZE) < 0)
			return -1;

		skb_push(skb, MV_ETH_EDSA_SIZE);

		memmove(skb->data, skb->data + MV_ETH_EDSA_SIZE, 2 * MV_MAC_ADDR_SIZE);

		dsa_header = skb->data + 2 * MV_MAC_ADDR_SIZE;
		dsa_header[0] = 0x40 | trg_dev;
		dsa_header[1] = trg_port << 3;
		dsa_header[2] = 0x00;
		dsa_header[3] = 0x00;

		dsa_header[2] |= 0x10;
		 
		dsa_header[4] = 0x00;
		 
		dsa_header[5] = 0x00;
		dsa_header[6] = 0x00;
		dsa_header[7] = 0x00;
		 
		if (edsaH & 0x400)
			dsa_header[6] |= 0x04;
	}

	return MV_OK;
}

int mv_mux_rx(struct sk_buff *skb, int port, struct napi_struct *napi)
{
	struct net_device *mux_dev;
	int    len;
	struct mux_netdev *pdev;
	bool is_edsa = false;

	mux_dev = mv_mux_rx_netdev_get(port, skb);

	if (mux_dev == NULL)
		goto out;

	if (!(mux_dev->flags & IFF_UP))
		goto out1;

	pdev = MV_MUX_PRIV(mux_dev);

	if (pdev->leave_tag == false) {
		 
		if (mux_eth_shadow[pdev->port].tag_type == MV_TAG_TYPE_DSA ||
		    mux_eth_shadow[pdev->port].tag_type == MV_TAG_TYPE_EDSA) {
			if (mux_eth_shadow[pdev->port].tag_type == MV_TAG_TYPE_EDSA)
				is_edsa = true;
			len = mv_mux_dsa2vlan(mux_dev, skb, is_edsa);
			 
			if (len < 0) {
				pr_err("Invalid (E)DSA mode\n");
				goto out1;
			}
		} else {
			 
			len = mv_mux_rx_tag_remove(mux_dev, skb);
		}
	} else {
		 
		__skb_pull(skb, MV_ETH_MH_SIZE);
		len = MV_ETH_MH_SIZE;
	}
	mux_dev->stats.rx_packets++;
	mux_dev->stats.rx_bytes += skb->len;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX) {
		struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);
		pr_err("\n%s - %s: port=%d, cpu=%d, pkt_size=%d, shift=%d, leave_tag=%d\n",
			mux_dev->name, __func__, pmux_priv->port, smp_processor_id(), skb->len, len, pdev->leave_tag);
		 
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif  

	skb->protocol = eth_type_trans(skb, mux_dev);

	if (pdev->leave_tag == true &&
	    (mux_eth_shadow[port].tag_type == MV_TAG_TYPE_DSA || mux_eth_shadow[port].tag_type == MV_TAG_TYPE_EDSA))
		skb->protocol = htons(pdev->proto_type);

	if (mux_dev->features & NETIF_F_GRO) {
		 
		return napi_gro_receive(napi, skb);
	}

	return netif_receive_skb(skb);

out1:
	mux_dev->stats.rx_dropped++;
out:
	kfree_skb(skb);
	return 0;
}
EXPORT_SYMBOL(mv_mux_rx);

static int mv_mux_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(dev);

	if (!(netif_running(dev))) {
		printk(KERN_ERR "!netif_running() in %s\n", __func__);
		goto out;
	}

	if (mv_mux_tx_skb_tag_add(dev, skb)) {
		printk(KERN_ERR "%s: mv_mux_tx_skb_tag_add failed.\n", __func__);
		goto out;
	}

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[pmux_priv->port].flags & MV_MUX_F_DBG_TX) {
		pr_err("\n%s - %s_%lu: port=%d, cpu=%d, in_intr=0x%lx, leave_tag=%d\n",
			dev->name, __func__, dev->stats.tx_packets, pmux_priv->port,
			smp_processor_id(), in_interrupt(), pmux_priv->leave_tag);
		 
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif  

	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	skb->dev = mux_eth_shadow[pmux_priv->port].root;

	MV_MUX_SKB_TAG_SET(skb);

	return dev_queue_xmit(skb);

out:
	dev->stats.tx_dropped++;
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

char *mv_mux_get_mac(struct net_device *mux_dev)
{

	if (!mux_dev) {
		printk(KERN_ERR "%s: mux net device is NULL.\n", __func__);
		return NULL;
	}

	return mux_dev->dev_addr;
}
 
static void mv_mux_set_rx_mode(struct net_device *mux_dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);

	if (!mv_mux_internal_switch(pmux_priv->port) || (switch_ops == NULL))
		return;

	if (switch_ops->promisc_set)
		if (switch_ops->promisc_set(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx),
			(mux_dev->flags & IFF_PROMISC) ? 1 : 0))
			pr_err("%s: Set promiscuous mode failed\n", mux_dev->name);

	 if (switch_ops->all_mcast_del)
		if (switch_ops->all_mcast_del(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx)))
			pr_err("%s: Delete all Mcast failed\n", mux_dev->name);

	if (mux_dev->flags & IFF_MULTICAST) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
		if (!netdev_mc_empty(mux_dev)) {
			struct netdev_hw_addr *ha;

			netdev_for_each_mc_addr(ha, mux_dev) {
				if (switch_ops->mac_addr_set) {
					if (switch_ops->mac_addr_set(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx),
						ha->addr, 1)) {
						pr_err("%s: Mcast init failed\n", mux_dev->name);
						break;
					}
				}
			}
		}
#else
		struct dev_mc_list *curr_addr = mux_dev->mc_list;
		int                i;
		for (i = 0; i < mux_dev->mc_count; i++, curr_addr = curr_addr->next) {
			if (!curr_addr)
				break;
			if (switch_ops->mac_addr_set) {
				if (switch_ops->mac_addr_set(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx),
					curr_addr->dmi_addr, 1)) {
					pr_err("%s: Mcast init failed\n", mux_dev->name);
					break;
				}
			}
		}
#endif  
	}
}
 
static int mv_mux_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	 
	printk(KERN_ERR "Not supported yet.\n");
	return 0;
}
 
static void mv_mux_switch_mtu_update(int mtu)
{
	int pkt_size, tag_size = 0;
	MV_TAG_TYPE tag_type =  mux_switch_shadow.tag_type;

	if (tag_type == MV_TAG_TYPE_MH)
		tag_size = MV_ETH_MH_SIZE;
	else if (tag_type == MV_TAG_TYPE_DSA)
		tag_size = MV_ETH_DSA_SIZE;

	pkt_size = mtu + tag_size + MV_ETH_ALEN + MV_ETH_VLAN_SIZE + MV_ETH_CRC_SIZE;

	if (switch_ops && switch_ops->jumbo_mode_set)
		switch_ops->jumbo_mode_set(pkt_size);
}

int mv_mux_close(struct net_device *dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	netif_stacked_transfer_operstate(root, dev);
	netif_tx_stop_all_queues(dev);

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->group_disable)
			switch_ops->group_disable(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx));

	printk(KERN_NOTICE "%s: stopped\n", dev->name);

	return MV_OK;
}
 
int mv_mux_open(struct net_device *dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (!root) {
		printk(KERN_ERR "%s:Invalid operation, set master before up.\n", __func__);
		return MV_ERROR;
	}

	if (!(root->flags & IFF_UP)) {
		printk(KERN_ERR "%s:Invalid operation, port %d is down.\n", __func__, pmux_priv->port);
		return MV_ERROR;
	}

	netif_stacked_transfer_operstate(root, dev);
	netif_tx_wake_all_queues(dev);

	if (dev->mtu > root->mtu)
		dev->mtu = root->mtu;

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->group_enable)
			switch_ops->group_enable(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx));

	printk(KERN_NOTICE "%s: started\n", dev->name);

	return MV_OK;

}
 
static int mv_mux_set_mac(struct net_device *mux_dev, void *addr)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);

	u8 *mac = &(((u8 *)addr)[2]);   

	mv_mux_close(mux_dev);

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->mac_addr_set) {

			if (switch_ops->mac_addr_set(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx), mux_dev->dev_addr, 0))
				return MV_ERROR;

			if (switch_ops->mac_addr_set(MV_MUX_GROUP_IDX_2_DB(pmux_priv->idx), mac, 1))
				return MV_ERROR;
		}

	memcpy(mux_dev->dev_addr, mac, ETH_ALEN);

	mv_mux_open(mux_dev);

	return 0;
}
 
int mv_mux_mtu_change(struct net_device *mux_dev, int mtu)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (root->mtu < mtu) {
		printk(KERN_ERR "Invalid mtu value.\n");
		return MV_ERROR;
	}

	mux_dev->mtu = mtu;
	return MV_OK;
}

struct net_device *mv_mux_netdev_alloc(char *name, int idx, MV_MUX_TAG *tag_cfg)
{
	struct net_device *mux_dev;
	struct mux_netdev *pmux_priv;

	if (name == NULL) {
		printk(KERN_ERR "%s: mux net device name is missig.\n", __func__);
		return NULL;
	}

	mux_dev = dev_get_by_name(&init_net, name);

	if (!mux_dev) {
		 
		mux_dev = alloc_netdev(sizeof(struct mux_netdev), name, ether_setup);
		if (!mux_dev) {
			printk(KERN_ERR "%s: out of memory, net device allocation failed.\n", __func__);
			return NULL;
		}
		 
		mux_dev->irq = NO_IRQ;
		 
		mux_dev->netdev_ops = &mv_mux_netdev_ops;

		if (register_netdev(mux_dev)) {
			printk(KERN_ERR "%s: failed to register %s\n", __func__, mux_dev->name);
			free_netdev(mux_dev);
			return NULL;
		}

		pmux_priv = MV_MUX_PRIV(mux_dev);
		memset(pmux_priv, 0, sizeof(struct mux_netdev));
		pmux_priv->port = -1;
		pmux_priv->next = NULL;
	} else
		dev_put(mux_dev);

	pmux_priv = MV_MUX_PRIV(mux_dev);

	if (tag_cfg == NULL) {
		memset(pmux_priv, 0, sizeof(struct mux_netdev));
		pmux_priv->port = -1;
		pmux_priv->next = NULL;
	} else{
		 
		pmux_priv->tx_tag = tag_cfg->tx_tag;
		pmux_priv->rx_tag_ptrn = tag_cfg->rx_tag_ptrn;
		pmux_priv->rx_tag_mask = tag_cfg->rx_tag_mask;
		pmux_priv->leave_tag = tag_cfg->leave_tag;
		pmux_priv->proto_type = tag_cfg->proto_type;
	}
	pmux_priv->idx = idx;
	return mux_dev;
}

static inline void mv_mux_init_features(struct net_device *mux_dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	mux_dev->features = root->features;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	mux_dev->hw_features = root->hw_features & ~NETIF_F_RXCSUM;
	mux_dev->wanted_features = root->wanted_features;
#endif
	mux_dev->vlan_features = root->vlan_features;
}
 
static void mv_mux_transfer_features(struct net_device *root, struct net_device *mux_dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	mux_dev->features &= ~NETIF_F_RXCSUM;
	mux_dev->features |=  (root->features & NETIF_F_RXCSUM);
#endif

	mux_dev->features &= ~NETIF_F_IP_CSUM;
	mux_dev->features |=  (root->features & NETIF_F_IP_CSUM);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	mux_dev->hw_features &= ~NETIF_F_IP_CSUM;
	mux_dev->hw_features |=  (root->features & NETIF_F_IP_CSUM);
#endif

	mux_dev->features &= ~NETIF_F_TSO;
	mux_dev->features |=  (root->features & NETIF_F_TSO);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	mux_dev->hw_features &= ~NETIF_F_TSO;
	mux_dev->hw_features |=  (root->features & NETIF_F_TSO);
#endif

	mux_dev->features &= ~NETIF_F_SG;
	mux_dev->features |=  (root->features & NETIF_F_SG);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	mux_dev->hw_features &= ~NETIF_F_SG;
	mux_dev->hw_features |=  (root->features & NETIF_F_SG);
#endif
}
 
static struct net_device *mv_mux_netdev_init(int port, struct net_device *mux_dev)
{
	struct mux_netdev *pmux_priv;
	struct net_device *root = mux_eth_shadow[port].root;
	int tag_type = mux_eth_shadow[port].tag_type;

	if (root == NULL)
		return NULL;
 
	if (!mux_dev) {
		printk(KERN_ERR "%s: mux net device is NULL.\n", __func__);
		return NULL;
	}

	mux_dev->hard_header_len = root->hard_header_len +
					mv_mux_get_tag_size(tag_type);

	mux_dev->mtu = root->mtu;
	pmux_priv = MV_MUX_PRIV(mux_dev);
	pmux_priv->port = port;
	memcpy(mux_dev->dev_addr, root->dev_addr, MV_MAC_ADDR_SIZE);

	mv_mux_init_features(mux_dev);

	SET_ETHTOOL_OPS(mux_dev, &mv_mux_tool_ops);

	return mux_dev;
}
 
struct net_device *mv_mux_switch_ptr_get(int port)
{
	return mux_eth_shadow[port].switch_dev;
}
 
int mv_mux_tag_type_get(int port)
{
	return mux_eth_shadow[port].tag_type;
}

struct net_device *mv_mux_netdev_add(int port, struct net_device *mux_dev)
{
	struct net_device *dev_temp;
	struct net_device *switch_dev;

	struct mux_netdev *pdev;

	if (mux_eth_shadow[port].root == NULL)
		return NULL;

	mux_dev = mv_mux_netdev_init(port, mux_dev);

	if (mux_dev == NULL)
		return NULL;

	switch_dev = mux_eth_shadow[port].switch_dev;

	if (switch_dev == NULL) {
		 
		mux_eth_shadow[port].switch_dev = mux_dev;
	} else {
		pdev = MV_MUX_PRIV(switch_dev);
		dev_temp = switch_dev;
		while ((mux_dev != dev_temp) && (pdev->next != NULL)) {
			dev_temp = pdev->next;
			pdev = MV_MUX_PRIV(dev_temp);
		}
		 
		if (mux_dev == dev_temp)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 34)
			printk(KERN_INFO "%s: this mux interface is already in port %d\n", mux_dev->name, port);
#else
			netdev_info(mux_dev, "this mux interface is already in port %d\n", port);
#endif
		else
			pdev->next = mux_dev;
	}

	if (!mux_init_cnt)
		if (register_netdevice_notifier(&mux_notifier_block) < 0)
			unregister_netdevice_notifier(&mux_notifier_block);

	mux_init_cnt++;

	return mux_dev;
}

int mv_mux_tag_type_set(int port, int type)
{
	unsigned int flgs;
	struct net_device *root;

#if defined(MY_ABC_HERE)
	if (port >= MV_ETH_MAX_PORTS) {
		pr_err("gbe port %d is out of range (0..%d)\n", port, MV_ETH_MAX_PORTS-1);
		return;
	}
#endif  

	if ((type < MV_TAG_TYPE_NONE) || (type >= MV_TAG_TYPE_LAST)) {
		printk(KERN_INFO "%s: Invalid tag type %d\n", __func__, type);
		return MV_ERROR;
	}
	root = mux_eth_shadow[port].root;
	 
	if (root == NULL)
		return MV_ERROR;

	if (mux_eth_shadow[port].tag_type == type)
		return MV_OK;

	flgs = root->flags;

	if (flgs & IFF_UP) {
		printk(KERN_ERR "%s: root device (%s) must stopped before.\n", __func__, root->name);
		return MV_ERROR;
	}

	if (mv_mux_netdev_delete_all(port))
		return MV_ERROR;

	mux_eth_shadow[port].tag_type = type;

	if (eth_ops && eth_ops->set_tag_type)
		eth_ops->set_tag_type(port, mux_eth_shadow[port].tag_type);

	return MV_OK;
}

int mv_mux_netdev_delete(struct net_device *mux_dev)
{
	struct net_device *pdev_curr, *pdev_prev = NULL;
	struct mux_netdev *pdev_tmp_curr, *pdev_tmp_prev, *pdev;
	struct net_device *root = NULL;
	int flgs, port;

	if (mux_dev == NULL) {
		printk(KERN_ERR "%s: mux net device is NULL.\n", __func__);
		return MV_ERROR;
	}
	pdev = MV_MUX_PRIV(mux_dev);
	port = pdev->port;

	if (port != -1)
		root = mux_eth_shadow[pdev->port].root;

	if (root == NULL) {
		synchronize_net();
		unregister_netdev(mux_dev);
		free_netdev(mux_dev);
		 
		return MV_OK;
	}

	flgs = mux_dev->flags;
	if (flgs & IFF_UP) {
		printk(KERN_ERR "%s: root device (%s) must stopped before.\n", __func__, root->name);
		return MV_ERROR;
	}

	pdev_curr = mux_eth_shadow[port].switch_dev;

	while (pdev_curr != NULL) {

		pdev_tmp_curr = MV_MUX_PRIV(pdev_curr);

		if (pdev_curr == mux_dev) {
			if (pdev_curr == mux_eth_shadow[port].switch_dev) {
				 
				mux_eth_shadow[port].switch_dev = pdev_tmp_curr->next;
			} else {
				pdev_tmp_prev = MV_MUX_PRIV(pdev_prev);
				pdev_tmp_prev->next = pdev_tmp_curr->next;
			}
			 
			synchronize_net();
			unregister_netdev(mux_dev);
			printk(KERN_ERR "%s has been removed.\n", mux_dev->name);
			free_netdev(mux_dev);

			mux_init_cnt--;

			if (!mux_init_cnt)
				unregister_netdevice_notifier(&mux_notifier_block);

			return MV_OK;

		} else {
			pdev_prev = pdev_curr;
			pdev_curr = pdev_tmp_curr->next;
		}
	}
	 
	return MV_ERROR;
}

static int mv_mux_netdev_delete_all(int port)
{
	 
	struct net_device *mux_dev;

	if (mux_eth_shadow[port].root == NULL)
		return MV_ERROR;

	mux_dev = mux_eth_shadow[port].switch_dev;
	while (mux_dev) {
		if (mv_mux_netdev_delete(mux_dev))
			return MV_ERROR;

		mux_dev = mux_eth_shadow[port].switch_dev;
	}

	return MV_OK;
}

static int mux_device_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
	struct net_device *mux_dev, *dev = ptr;
	struct mux_netdev *pdev_priv;
	int tag_type;
	int port = 0;
	int flgs;

	port = mv_mux_netdev_find(dev->ifindex);

	if (port == -1)
		goto out;

	tag_type = mux_eth_shadow[port].tag_type;

	if (mv_mux_internal_switch(port) && (tag_type == MV_TAG_TYPE_NONE))
		goto out;

	switch (event) {

	case NETDEV_CHANGE:
		mux_dev = mux_eth_shadow[port].switch_dev;
		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			if (mv_mux_internal_switch(port)) {
				 
				if (switch_ops && switch_ops->link_status_get
					&& (pdev_priv->idx != MV_MUX_UNKNOWN_GROUP)) {
					int link_up;
					link_up = switch_ops->link_status_get(MV_MUX_GROUP_IDX_2_DB(pdev_priv->idx));
					mv_mux_update_link(mux_dev, link_up);
				}
			} else {
				 
				netif_stacked_transfer_operstate(dev, mux_dev);
			}
			mux_dev = pdev_priv->next;
		}
		break;

	case NETDEV_CHANGEADDR:
		 
		mux_dev = mux_eth_shadow[port].switch_dev;

		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			 
			if (!compare_ether_addr(mux_dev->dev_addr, dev->dev_addr)) {
				mux_dev = pdev_priv->next;
				continue;
			}
			memcpy(mux_dev->dev_addr, dev->dev_addr, ETH_ALEN);
			mux_dev = pdev_priv->next;
		}
		break;

	case NETDEV_CHANGEMTU:
		mux_dev = mux_eth_shadow[port].switch_dev;

		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			dev_set_mtu(mux_dev, dev->mtu);
			mux_dev = pdev_priv->next;
		}

		if (mv_mux_internal_switch(port)) {
			mux_switch_shadow.mtu = dev->mtu;
			mv_mux_switch_mtu_update(dev->mtu);
		}

		break;

	case NETDEV_DOWN:
		 
		mux_dev = mux_eth_shadow[port].switch_dev;

		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			flgs = mux_dev->flags;
			if (!(flgs & IFF_UP)) {
				mux_dev = pdev_priv->next;
				continue;
			}
			 
			dev_change_flags(mux_dev, flgs & ~IFF_UP);
			mux_dev = pdev_priv->next;
		}
		break;

	case NETDEV_UP:
		 
		if (mv_mux_internal_switch(port) &&
			((mux_switch_shadow.mtu == -1) || (mux_switch_shadow.mtu > dev->mtu))) {
				mux_switch_shadow.mtu = dev->mtu;
				mv_mux_switch_mtu_update(dev->mtu);
		}

		break;

	case NETDEV_FEAT_CHANGE:
		 
		mux_dev = mux_eth_shadow[port].switch_dev;
		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			mv_mux_transfer_features(mux_eth_shadow[port].root, mux_dev);
			mux_dev = pdev_priv->next;
		}
		break;
	}  
out:
	return NOTIFY_DONE;
}
 
static struct notifier_block mux_notifier_block __read_mostly = {
	.notifier_call = mux_device_event,
};
 
bool mv_mux_netdev_link_status(struct net_device *dev)
{
	return netif_carrier_ok(dev) ? true : false;
}

void mv_mux_vlan_set(MV_MUX_TAG *mux_cfg, unsigned int vid)
{

	mux_cfg->tx_tag.vlan = MV_32BIT_BE((MV_VLAN_TYPE << 16) | vid);
	mux_cfg->rx_tag_ptrn.vlan = MV_32BIT_BE((MV_VLAN_TYPE << 16) | vid);

	mux_cfg->rx_tag_mask.vlan = MV_32BIT_BE(0xFFFF0FFF);

	mux_cfg->tag_type = MV_TAG_TYPE_VLAN;
}

void mv_mux_cfg_get(struct net_device *mux_dev, MV_MUX_TAG *mux_cfg)
{
	if (mux_dev) {
		struct mux_netdev *pmux_priv;
		pmux_priv = MV_MUX_PRIV(mux_dev);
		mux_cfg->tx_tag = pmux_priv->tx_tag;
		mux_cfg->rx_tag_ptrn = pmux_priv->rx_tag_ptrn;
		mux_cfg->rx_tag_mask = pmux_priv->rx_tag_mask;
		mux_cfg->leave_tag = pmux_priv->leave_tag;
		mux_cfg->proto_type = pmux_priv->proto_type;
	} else
		memset(mux_cfg, 0, sizeof(MV_MUX_TAG));
}
 
static inline struct net_device *mv_mux_mh_netdev_get(int port, MV_TAG *tag)
{
	struct net_device *dev = mux_eth_shadow[port].switch_dev;
	struct mux_netdev *pdev;

	while (dev != NULL) {
		pdev = MV_MUX_PRIV(dev);
		if ((tag->mh & pdev->rx_tag_mask.mh) == pdev->rx_tag_ptrn.mh)
			return dev;

		dev = pdev->next;
	}
	printk(KERN_ERR "%s: MH=0x%04x match no interfaces\n", __func__, tag->mh);
	return NULL;
}

static inline struct net_device *mv_mux_vlan_netdev_get(int port, MV_TAG *tag)
{
	struct net_device *dev = mux_eth_shadow[port].switch_dev;
	struct mux_netdev *pdev;

	while (dev != NULL) {
		pdev = MV_MUX_PRIV(dev);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX)
			printk(KERN_ERR "pkt tag = 0x%x, rx_tag_ptrn = 0x%x, rx_tag_mask = 0x%x\n",
				 tag->vlan, pdev->rx_tag_ptrn.vlan, pdev->rx_tag_mask.vlan);
#endif
		if ((tag->vlan & pdev->rx_tag_mask.vlan) ==
			(pdev->rx_tag_ptrn.vlan & pdev->rx_tag_mask.vlan))
			return dev;

		dev = pdev->next;
	}
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	printk(KERN_ERR "%s:Error TAG=0x%08x match no interfaces\n", __func__, tag->vlan);
#endif

	return NULL;
}

static inline struct net_device *mv_mux_dsa_netdev_get(int port, MV_TAG *tag)
{
	 
	return mv_mux_vlan_netdev_get(port, tag);
}

static inline struct net_device *mv_mux_edsa_netdev_get(int port, MV_TAG *tag)
{
	struct net_device *dev = mux_eth_shadow[port].switch_dev;
	struct mux_netdev *pdev;

	while (dev != NULL) {
		pdev = MV_MUX_PRIV(dev);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX)
			printk(KERN_ERR "pkt tag = 0x%x %x, rx_tag_ptrn = 0x%x %x, rx_tag_mask = 0x%x %x\n",
				 tag->edsa[0], tag->edsa[1], pdev->rx_tag_ptrn.edsa[0], pdev->rx_tag_ptrn.edsa[1],
				 pdev->rx_tag_mask.edsa[0], pdev->rx_tag_mask.edsa[1]);
#endif
		 
		if (((tag->edsa[0] & pdev->rx_tag_mask.edsa[0]) ==
			(pdev->rx_tag_ptrn.edsa[0] & pdev->rx_tag_mask.edsa[0])) &&
			((tag->edsa[1] & pdev->rx_tag_mask.edsa[1]) ==
			(pdev->rx_tag_ptrn.edsa[1] & pdev->rx_tag_mask.edsa[1])))
				return dev;

		dev = pdev->next;
	}
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX)
		pr_err("%s:Error TAG=0x%08x, 0x%08x match no interfaces\n", __func__, tag->edsa[0], tag->edsa[1]);
#endif

	return NULL;
}

static inline struct net_device *mv_mux_rx_netdev_get(int port, struct sk_buff *skb)
{
	struct net_device *dev;
	MV_TAG tag;
	MV_U8 *data = skb->data;
	int tag_type = mux_eth_shadow[port].tag_type;

	switch (tag_type) {

	case MV_TAG_TYPE_MH:
		tag.mh = ntohs(*(MV_U16 *)data);
		dev = mv_mux_mh_netdev_get(port, &tag);
		break;

	case MV_TAG_TYPE_VLAN:
		tag.vlan = ntohl(*(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE)));
		dev = mv_mux_vlan_netdev_get(port, &tag);
		break;

	case MV_TAG_TYPE_DSA:
		tag.dsa = ntohl(*(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE)));
		dev = mv_mux_dsa_netdev_get(port, &tag);
		break;

	case MV_TAG_TYPE_EDSA:
		tag.edsa[0] = ntohl(*(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE)));
		tag.edsa[1] = ntohl(*(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE) + 4));
		dev = mv_mux_edsa_netdev_get(port, &tag);
		break;

	default:
		printk(KERN_ERR "%s: unexpected port mode = %d\n", __func__, tag_type);
		return NULL;
	}

	return dev;
}

static inline int mv_mux_mh_skb_remove(struct sk_buff *skb)
{
	__skb_pull(skb, MV_ETH_MH_SIZE);
	return MV_ETH_MH_SIZE;
}

static inline int mv_mux_vlan_skb_remove(struct sk_buff *skb)
{
	 
	memmove(skb->data + MV_VLAN_HLEN, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE);

	__skb_pull(skb, MV_VLAN_HLEN);

	return MV_ETH_VLAN_SIZE;
}
 
static inline int mv_mux_dsa_skb_remove(struct sk_buff *skb)
{
	 
	memmove(skb->data + MV_ETH_DSA_SIZE, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE);

	__skb_pull(skb, MV_ETH_DSA_SIZE);

	return MV_ETH_DSA_SIZE;
}
 
static inline int mv_mux_edsa_skb_remove(struct sk_buff *skb)
{
	 
	memmove(skb->data + MV_ETH_EDSA_SIZE, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE);

	__skb_pull(skb, MV_ETH_EDSA_SIZE);

	return MV_ETH_EDSA_SIZE;
}

static inline int mv_mux_rx_tag_remove(struct net_device *dev, struct sk_buff *skb)
{
	int shift = 0;
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);
	int tag_type = mux_eth_shadow[pdev->port].tag_type;

	if (pdev->leave_tag == true)
		return 0;

	switch (tag_type) {

	case MV_TAG_TYPE_MH:
		break;

	case MV_TAG_TYPE_VLAN:
		shift = mv_mux_vlan_skb_remove(skb);
		break;

	case MV_TAG_TYPE_DSA:
		shift = mv_mux_dsa_skb_remove(skb);
		break;

	case MV_TAG_TYPE_EDSA:
		shift = mv_mux_edsa_skb_remove(skb);
		break;

	default:
		printk(KERN_ERR "%s: unexpected port mode = %d\n", __func__, tag_type);
		return -1;
	}
	 
	shift += mv_mux_mh_skb_remove(skb);

	return shift;
}

static inline int mv_eth_skb_mh_add(struct sk_buff *skb, u16 mh)
{

	if (skb_headroom(skb) < MV_ETH_MH_SIZE) {
		printk(KERN_ERR "%s: skb (%p) doesn't have place for MH, head=%p, data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	__skb_push(skb, MV_ETH_MH_SIZE);

	*((u16 *) skb->data) = mh;

	return MV_OK;
}

static inline int mv_mux_tx_skb_mh_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);

	return mv_eth_skb_mh_add(skb, pdev->tx_tag.mh);
}

static inline int mv_mux_skb_vlan_add(struct sk_buff *skb, unsigned int vlan)
{
	unsigned char *pvlan;
 
	if (skb_cow(skb, MV_VLAN_HLEN)) {
		printk(KERN_ERR "%s: skb (%p) headroom < VLAN_HDR, skb_head=%p, skb_data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	__skb_push(skb, MV_VLAN_HLEN);

	memmove(skb->data, skb->data + MV_VLAN_HLEN, 2 * MV_MAC_ADDR_SIZE);

	pvlan = skb->data + (2 * MV_MAC_ADDR_SIZE);
	*(MV_U32 *)pvlan = vlan;

	return MV_OK;
}

static inline int mv_mux_tx_skb_vlan_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);

	return mv_mux_skb_vlan_add(skb, htonl(pdev->tx_tag.vlan));
}

static inline int mv_mux_tx_skb_dsa_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);
	 
	if (!pdev->leave_tag)
		return mv_mux_vlan2dsa(skb, pdev->tx_tag.dsa);
	else
		return MV_OK;
}

static inline int mv_mux_skb_edsa_add(struct sk_buff *skb, unsigned int edsaL, unsigned int edsaH)
{
	unsigned char *pedsa;

	if (skb_cow(skb, MV_ETH_EDSA_SIZE)) {
		printk(KERN_ERR "%s: skb (%p) headroom < VLAN_HDR, skb_head=%p, skb_data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	__skb_push(skb, MV_ETH_EDSA_SIZE);

	memmove(skb->data, skb->data + MV_ETH_EDSA_SIZE, 2 * MV_MAC_ADDR_SIZE);

	pedsa = skb->data + (2 * MV_MAC_ADDR_SIZE);
	*(MV_U32 *)pedsa = edsaL;
	*((MV_U32 *)pedsa + 1) = edsaH;

	return MV_OK;
}

static inline int mv_mux_tx_skb_edsa_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);
	 
	if (!pdev->leave_tag)
		return mv_mux_vlan2edsa(skb, pdev->tx_tag.edsa[0], pdev->tx_tag.edsa[1]);
	else
		return MV_OK;
}

static inline int mv_mux_tx_skb_tag_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);
	int tag_type = mux_eth_shadow[pdev->port].tag_type;
	int err = 0;

	if (pdev->leave_tag == true)
		return err;

	switch (tag_type) {

	case MV_TAG_TYPE_MH:
		err = mv_mux_tx_skb_mh_add(dev, skb);
		break;
	case MV_TAG_TYPE_VLAN:
		err = mv_mux_tx_skb_vlan_add(dev, skb);
		break;
	case MV_TAG_TYPE_DSA:
		err = mv_mux_tx_skb_dsa_add(dev, skb);
		break;
	case MV_TAG_TYPE_EDSA:
		err = mv_mux_tx_skb_edsa_add(dev, skb);
		break;
	default:
		printk(KERN_ERR "%s: unexpected port mode = %d\n", __func__, tag_type);
		err = 1;
	}
	return err;
}

void mv_mux_netdev_print(struct net_device *mux_dev)
{
	struct mux_netdev *pdev;
	int tag_type;

	if (!mux_dev) {
		pr_err("%s:device in NULL.\n", __func__);
		return;
	}

	if (mv_mux_netdev_find(mux_dev->ifindex) != -1) {
		pr_err("%s: %s is not mux device.\n", __func__, mux_dev->name);
		return;
	}

	pdev = MV_MUX_PRIV(mux_dev);

	if (!pdev || (pdev->port == -1)) {
		pr_err("%s: device must be conncted to physical port\n", __func__);
		return;
	}
	tag_type = mux_eth_shadow[pdev->port].tag_type;
	switch (tag_type) {

	case MV_TAG_TYPE_VLAN:
		pr_info("%s: port=%d, pdev=%p, tx_vlan=0x%08x, rx_vlan=0x%08x, rx_mask=0x%08x",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.vlan,
			pdev->rx_tag_ptrn.vlan, pdev->rx_tag_mask.vlan);
		break;

	case MV_TAG_TYPE_DSA:
		pr_info("%s: port=%d, pdev=%p: tx_dsa=0x%08x, rx_dsa=0x%08x, rx_mask=0x%08x",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.dsa,
			pdev->rx_tag_ptrn.dsa, pdev->rx_tag_mask.dsa);
		break;

	case MV_TAG_TYPE_MH:
		pr_info("%s: port=%d, pdev=%p: tx_mh=0x%04x, rx_mh=0x%04x, rx_mask=0x%04x",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.mh, pdev->rx_tag_ptrn.mh, pdev->rx_tag_mask.mh);
		break;

	case MV_TAG_TYPE_EDSA:
		pr_info("%s: port=%d, pdev=%p: tx_edsa=0x%08x %08x, rx_edsa=0x%08x %08x, rx_mask=0x%08x %08x",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.edsa[1], pdev->tx_tag.edsa[0],
			pdev->rx_tag_ptrn.edsa[1], pdev->rx_tag_ptrn.edsa[0],
			pdev->rx_tag_mask.edsa[1], pdev->rx_tag_mask.edsa[0]);
		break;

	default:
		pr_info("%s: Error, Unknown tag type\n", __func__);
	}
	pr_info(", leave_tag=%d\n", pdev->leave_tag);
}
EXPORT_SYMBOL(mv_mux_netdev_print);

void mv_mux_netdev_print_all(int port)
{
	struct net_device *dev;
	struct mux_netdev *dev_priv;

	dev = mux_eth_shadow[port].root;

	if (!dev)
		return;

	dev = mux_eth_shadow[port].switch_dev;

	while (dev != NULL) {
		mv_mux_netdev_print(dev);
		dev_priv = MV_MUX_PRIV(dev);
		dev = dev_priv->next;
		printk(KERN_CONT "\n");
	}
}
EXPORT_SYMBOL(mv_mux_netdev_print_all);
 
int mv_mux_ctrl_dbg_flag(int port, u32 flag, u32 val)
{
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	struct net_device *root = mux_eth_shadow[port].root;
	u32 bit_flag = (fls(flag) - 1);

	if (!root)
		return -ENODEV;

	if (val)
		set_bit(bit_flag, (unsigned long *)&(mux_eth_shadow[port].flags));
	else
		clear_bit(bit_flag, (unsigned long *)&(mux_eth_shadow[port].flags));
#endif  

	return 0;
}
 
static const struct net_device_ops mv_mux_netdev_ops = {
	.ndo_open		= mv_mux_open,
	.ndo_stop		= mv_mux_close,
	.ndo_start_xmit		= mv_mux_xmit,
	.ndo_set_mac_address	= mv_mux_set_mac,
	.ndo_do_ioctl		= mv_mux_ioctl,
	.ndo_set_rx_mode	= mv_mux_set_rx_mode,
	.ndo_change_mtu		= mv_mux_mtu_change,
};
 
static const struct mv_switch_mux_ops mux_ops =  {
	.update_link = mv_mux_update_link,
};
