/*
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005-2006, Devicescape Software, Inc.
 * Copyright 2006-2007	Jiri Benc <jbenc@suse.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <generated/uapi/linux/version.h>
#include <net/mac80211_xr.h>
#include <linux/module.h>
#include <linux/fips.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/bitmap.h>
#include <linux/pm_qos.h>
#include <linux/inetdevice.h>
#include <net/net_namespace.h>
#include <net/cfg80211.h>
#include <uapi/linux/netdevice.h>

#ifdef IPV6_FILTERING
#include <net/if_inet6.h>
#include <net/addrconf.h>
#endif /*IPV6_FILTERING*/

#include "ieee80211_i.h"
#include "driver-ops.h"
#include "rate.h"
#include "mesh.h"
#include "wep.h"
#include "led.h"
#include "cfg.h"
#include "debugfs.h"

static struct lock_class_key ieee80211_rx_skb_queue_class;

void mac80211_configure_filter(struct ieee80211_sub_if_data *sdata)
{
	u64 mc;
	unsigned int changed_flags;
	unsigned int new_flags = 0;
	struct ieee80211_local *local = sdata->local;

#if 0
	if (!(SDATA_STATE_RUNNING & sdata->state))
		return;
#endif

	if (sdata->flags & IEEE80211_SDATA_ALLMULTI)
		new_flags |= FIF_ALLMULTI;

	if (sdata->flags & IEEE80211_SDATA_PROMISC)
		new_flags |= FIF_PROMISC_IN_BSS;

	if (sdata->vif.type == NL80211_IFTYPE_MONITOR ||
			local->scan_sdata == sdata)
		new_flags |= FIF_BCN_PRBRESP_PROMISC;

	if (sdata->vif.type == NL80211_IFTYPE_AP ||
			sdata->vif.type == NL80211_IFTYPE_ADHOC)
		new_flags |= FIF_PROBE_REQ;

	if (sdata->vif.type == NL80211_IFTYPE_AP)
		new_flags |= FIF_PSPOLL;

	new_flags |= sdata->req_filt_flags;

	spin_lock_bh(&local->filter_lock);
	changed_flags = sdata->filter_flags ^ new_flags;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	mc = drv_prepare_multicast(local, sdata, &sdata->mc_list);
#else
	mc = drv_prepare_multicast(local, sdata, sdata->mc_count,
				   sdata->mc_list);
#endif
	spin_unlock_bh(&local->filter_lock);

	/* be a bit nasty */
	new_flags |= ((unsigned int)1 << 31);

	drv_configure_filter(local, sdata, changed_flags, &new_flags, mc);

	WARN_ON(new_flags & ((unsigned int)1 << 31));

	sdata->filter_flags = new_flags & ~((unsigned int)1 << 31);
}

void ieee80211_notify_channel_change(struct ieee80211_local *local,
			struct ieee80211_sub_if_data *sdata)
{
	if (local->hw.flags & IEEE80211_HW_SUPPORTS_MULTI_CHANNEL) {
		BUG_ON(!sdata);
		mac80211_bss_info_change_notify(sdata, BSS_CHANGED_CHANNEL);
	} else {
		mac80211_hw_config(local, IEEE80211_CONF_CHANGE_CHANNEL);
	}
}

static struct ieee80211_channel *ieee80211_recalc_channel(
			struct ieee80211_local *local,
			struct ieee80211_sub_if_data *sdata,
			u32 *changed)
{
	struct ieee80211_channel_state *chan_state;
	struct ieee80211_channel *chan, *scan_chan;
	enum nl80211_channel_type channel_type;
	u32 offchannel_flag;
	bool multi_channel;

	BUG_ON(local && sdata);
	BUG_ON(!local && !sdata);

	multi_channel = sdata ? true : false;
	if (local == NULL && sdata != NULL)
		local = sdata->local;
	scan_chan = local->scan_channel;

	if (sdata) {
		if (local->scan_sdata != sdata)
			scan_chan = NULL;
		chan_state = &sdata->chan_state;
	} else if (scan_chan) {
		BUG_ON(!local->scan_sdata);
		chan_state = &local->scan_sdata->chan_state;
	} else {
		chan_state = &local->chan_state;
	}

	offchannel_flag = chan_state->conf.offchannel;
	if (scan_chan) {
		chan = scan_chan;
		/* If scanning on oper channel, use whatever channel-type
		 * is currently in use.
		 */
		if (chan == chan_state->oper_channel)
			channel_type = chan_state->_oper_channel_type;
		else
			channel_type = NL80211_CHAN_NO_HT;
		chan_state->conf.offchannel = true;
	} else if (chan_state->tmp_channel &&
		   chan_state->oper_channel != chan_state->tmp_channel) {
		chan = scan_chan = chan_state->tmp_channel;
		channel_type = chan_state->tmp_channel_type;
		chan_state->conf.offchannel = true;
	} else {
		chan = chan_state->oper_channel;
		channel_type = chan_state->_oper_channel_type;
		chan_state->conf.offchannel = false;
	}

	offchannel_flag ^= chan_state->conf.offchannel;

	if (offchannel_flag || chan != chan_state->conf.channel ||
	    channel_type != chan_state->conf.channel_type) {
		chan_state->conf.channel = chan;
		chan_state->conf.channel_type = channel_type;

		if (multi_channel) {
			*changed |= BSS_CHANGED_CHANNEL;
		} else {
			*changed |= IEEE80211_CONF_CHANGE_CHANNEL;
		}
	}

	return chan;
}

int mac80211_hw_config(struct ieee80211_local *local, u32 changed)
{
	struct ieee80211_channel *chan = NULL; /* Only used when multi channel is off */
	struct ieee80211_sub_if_data *sdata;
	int ret = 0;
	int power;
	bool is_ht;

	might_sleep();

	if (local->hw.flags & IEEE80211_HW_SUPPORTS_MULTI_CHANNEL) {
		/* XXX: broken code ahead*/
		/*      we can't call bss_info_changed while in rcu section!*/
		rcu_read_lock();
		list_for_each_entry_rcu(sdata, &local->interfaces, list)
			ieee80211_recalc_channel(NULL, sdata, &changed);
		rcu_read_unlock();
		/* XXX: broken code end*/
	} else {
		chan = ieee80211_recalc_channel(local, NULL, &changed);
	}

	if (local->hw.flags & IEEE80211_HW_SUPPORTS_MULTI_CHANNEL) {
		is_ht = true;
		rcu_read_lock();
		list_for_each_entry_rcu(sdata, &local->interfaces, list) {
			if (!conf_is_ht(&sdata->chan_state.conf)) {
				is_ht = false;
				break;
			}
		}
		rcu_read_unlock();
	} else {
		is_ht = conf_is_ht(local->hw.conf.chan_conf);
	}

	if (!is_ht) {
		/*
		 * mac80211.h documents that this is only valid
		 * when the channel is set to an HT type, and
		 * that otherwise STATIC is used.
		 */
		local->hw.conf.smps_mode = IEEE80211_SMPS_STATIC;
	} else if (local->hw.conf.smps_mode != local->smps_mode) {
		local->hw.conf.smps_mode = local->smps_mode;
		changed |= IEEE80211_CONF_CHANGE_SMPS;
	}

	if (local->hw.flags & IEEE80211_HW_SUPPORTS_MULTI_CHANNEL) {
		power = 0;
		rcu_read_lock();
		list_for_each_entry_rcu(sdata, &local->interfaces, list)
			power = max(power, sdata->chan_state.conf.channel->max_power);
		list_for_each_entry_rcu(sdata, &local->interfaces, list)
			power = min(power, (sdata->chan_state.conf.channel->max_power -
						local->power_constr_level));
		rcu_read_unlock();
	} else {
		if ((local->scanning & SCAN_SW_SCANNING) ||
		    (local->scanning & SCAN_HW_SCANNING)) {
			power = chan->max_power;
		} else {
			power = local->power_constr_level ?
				(chan->max_power - local->power_constr_level) :
				chan->max_power;
		}
	}

	if (local->user_power_level >= 0)
		power = min(power, local->user_power_level);

	if (local->hw.conf.power_level != power) {
		changed |= IEEE80211_CONF_CHANGE_POWER;
		local->hw.conf.power_level = power;
	}

	if (changed && local->open_count) {
		ret = drv_config(local, changed);
		/*
		 * Goal:
		 * HW reconfiguration should never fail, the driver has told
		 * us what it can support so it should live up to that promise.
		 *
		 * Current status:
		 * rfkill is not integrated with mac80211 and a
		 * configuration command can thus fail if hardware rfkill
		 * is enabled
		 *
		 * FIXME: integrate rfkill with mac80211 and then add this
		 * WARN_ON() back
		 *
		 */
		/* WARN_ON(ret); */
	}

	return ret;
}

void mac80211_bss_info_change_notify(struct ieee80211_sub_if_data *sdata,
				      u32 changed)
{
	struct ieee80211_local *local = sdata->local;
	static const u8 zero[ETH_ALEN] = { 0 };

	if (!changed)
		return;

	if (local->hw.flags & IEEE80211_HW_SUPPORTS_MULTI_CHANNEL)
		ieee80211_recalc_channel(NULL, sdata, &changed);

	if (sdata->vif.type == NL80211_IFTYPE_STATION) {
		/*
		 * While not associated, claim a BSSID of all-zeroes
		 * so that drivers don't do any weird things with the
		 * BSSID at that time.
		 */
		if (sdata->vif.bss_conf.assoc)
			sdata->vif.bss_conf.bssid = sdata->u.mgd.bssid;
		else
			sdata->vif.bss_conf.bssid = zero;
	} else if (sdata->vif.type == NL80211_IFTYPE_ADHOC)
		sdata->vif.bss_conf.bssid = sdata->u.ibss.bssid;
	else if (sdata->vif.type == NL80211_IFTYPE_AP)
		sdata->vif.bss_conf.bssid = sdata->vif.addr;
	else if (sdata->vif.type == NL80211_IFTYPE_WDS)
		sdata->vif.bss_conf.bssid = NULL;
	else if (ieee80211_vif_is_mesh(&sdata->vif)) {
		sdata->vif.bss_conf.bssid = zero;
	} else if (sdata->vif.type == NL80211_IFTYPE_P2P_DEVICE) {
		sdata->vif.bss_conf.bssid = sdata->vif.addr;
		WARN_ONCE(changed & ~(BSS_CHANGED_IDLE),
			  "P2P Device BSS changed %#x", changed);
	} else {
		WARN_ON(1);
		return;
	}

	switch (sdata->vif.type) {
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_ADHOC:
	case NL80211_IFTYPE_WDS:
	case NL80211_IFTYPE_MESH_POINT:
		break;
	default:
		/* do not warn to simplify caller in scan.c */
		changed &= ~BSS_CHANGED_BEACON_ENABLED;
		if (WARN_ON(changed & BSS_CHANGED_BEACON))
			return;
		break;
	}

	if (changed & BSS_CHANGED_BEACON_ENABLED) {
		if (local->quiescing || !ieee80211_sdata_running(sdata) ||
		    test_bit(SCAN_SW_SCANNING, &local->scanning)) {
			sdata->vif.bss_conf.enable_beacon = false;
		} else {
			/*
			 * Beacon should be enabled, but AP mode must
			 * check whether there is a beacon configured.
			 */
			switch (sdata->vif.type) {
			case NL80211_IFTYPE_AP:
				sdata->vif.bss_conf.enable_beacon =
					!!sdata->u.ap.beacon;
				break;
			case NL80211_IFTYPE_ADHOC:
				sdata->vif.bss_conf.enable_beacon =
					!!sdata->u.ibss.presp;
				break;
#ifdef CONFIG_XRMAC_MESH
			case NL80211_IFTYPE_MESH_POINT:
				sdata->vif.bss_conf.enable_beacon =
					!!sdata->u.mesh.mesh_id_len;
				break;
#endif
			default:
				/* not reached */
				WARN_ON(1);
				break;
			}
		}
	}

	drv_bss_info_changed(local, sdata, &sdata->vif.bss_conf, changed);
}

u32 mac80211_reset_erp_info(struct ieee80211_sub_if_data *sdata)
{
	sdata->vif.bss_conf.use_cts_prot = false;
	sdata->vif.bss_conf.use_short_preamble = false;
	sdata->vif.bss_conf.use_short_slot = false;
	return BSS_CHANGED_ERP_CTS_PROT |
	       BSS_CHANGED_ERP_PREAMBLE |
	       BSS_CHANGED_ERP_SLOT;
}

static void ieee80211_tasklet_handler(unsigned long data)
{
	struct ieee80211_local *local = (struct ieee80211_local *) data;
	struct sta_info *sta, *tmp;
	struct skb_eosp_msg_data *eosp_data;
	struct sk_buff *skb;

	while ((skb = skb_dequeue(&local->skb_queue)) ||
	       (skb = skb_dequeue(&local->skb_queue_unreliable))) {
		switch (skb->pkt_type) {
		case IEEE80211_RX_MSG:
			/* Clear skb->pkt_type in order to not confuse kernel
			 * netstack. */
			skb->pkt_type = 0;
			xr_mac80211_rx(local_to_hw(local), skb);
			break;
		case IEEE80211_TX_STATUS_MSG:
			skb->pkt_type = 0;
			xr_mac80211_tx_status(local_to_hw(local), skb);
			break;
		case IEEE80211_EOSP_MSG:
			eosp_data = (void *)skb->cb;
			for_each_sta_info(local, eosp_data->sta, sta, tmp) {
				/* skip wrong virtual interface */
				if (memcmp(eosp_data->iface,
					   sta->sdata->vif.addr, ETH_ALEN))
					continue;
				clear_sta_flag(sta, WLAN_STA_SP);
				break;
			}
			dev_kfree_skb(skb);
			break;
		default:
			WARN(1, "mac80211: Packet is of unknown type %d\n",
			     skb->pkt_type);
			dev_kfree_skb(skb);
			break;
		}
	}
}

static void ieee80211_restart_work(struct work_struct *work)
{
	struct ieee80211_local *local =
		container_of(work, struct ieee80211_local, restart_work);

	/* wait for scan work complete */
	flush_workqueue(local->workqueue);

	mutex_lock(&local->mtx);
	WARN(test_bit(SCAN_HW_SCANNING, &local->scanning) ||
	     local->sched_scanning,
		"%s called with hardware scan in progress\n", __func__);
	mutex_unlock(&local->mtx);

	rtnl_lock();
	mac80211_scan_cancel(local);
	mac80211_reconfig(local);
	rtnl_unlock();
}

void xr_mac80211_restart_hw(struct ieee80211_hw *hw)
{
	struct ieee80211_local *local = hw_to_local(hw);

	trace_api_restart_hw(local);

	wiphy_info(hw->wiphy,
		   "Hardware restart was requested\n");

	/* use this reason, mac80211_reconfig will unblock it */
	xr_mac80211_stop_queues_by_reason(hw,
		IEEE80211_QUEUE_STOP_REASON_SUSPEND);

	/*
	 * Stop all Rx during the reconfig. We don't want state changes
	 * or driver callbacks while this is in progress.
	 */
	local->in_reconfig = true;
	barrier();

	queue_work(system_freezable_wq, &local->restart_work);
}

int xr_mac80211_ifdev_move(struct ieee80211_hw *hw,
			struct device *new_parent, int dpm_order)
{
	struct ieee80211_local *local = hw_to_local(hw);
	struct ieee80211_sub_if_data *sdata;
	int ret;

	list_for_each_entry(sdata, &local->interfaces, list) {
		if (sdata->vif.type == NL80211_IFTYPE_P2P_DEVICE)
			continue;
		ret = device_move(&sdata->dev->dev, new_parent, dpm_order);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static void mac80211_recalc_smps_work(struct work_struct *work)
{
	struct ieee80211_local *local =
		container_of(work, struct ieee80211_local, recalc_smps);

	mutex_lock(&local->iflist_mtx);
	mac80211_recalc_smps(local);
	mutex_unlock(&local->iflist_mtx);
}

#ifdef CONFIG_INET
static int ieee80211_ifa_changed(struct notifier_block *nb,
				 unsigned long data, void *arg)
{
	struct in_ifaddr *ifa = arg;
	struct ieee80211_local *local =
		container_of(nb, struct ieee80211_local,
			     ifa_notifier);
	struct net_device *ndev = ifa->ifa_dev->dev;
	struct wireless_dev *wdev = ndev->ieee80211_ptr;
	struct in_device *idev;
	struct ieee80211_sub_if_data *sdata;
	struct ieee80211_bss_conf *bss_conf;
	struct ieee80211_if_managed *ifmgd = NULL;
	int c = 0;

	/* Make sure it's our interface that got changed */
	if (!wdev)
		return NOTIFY_DONE;

	if (wdev->wiphy != local->hw.wiphy)
		return NOTIFY_DONE;

	sdata = IEEE80211_DEV_TO_SUB_IF(ndev);
	bss_conf = &sdata->vif.bss_conf;

	if (!ieee80211_sdata_running(sdata))
		return NOTIFY_DONE;

	/* ARP filtering is only supported in managed mode */
	switch (sdata->vif.type) {
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_GO:
		break;
	default:
		return NOTIFY_DONE;
	}

	idev = __in_dev_get_rtnl(sdata->dev);
	if (!idev)
		return NOTIFY_DONE;

	if (sdata->vif.type == NL80211_IFTYPE_STATION) {
		ifmgd = &sdata->u.mgd;
		mutex_lock(&ifmgd->mtx);
	}

	/* Copy the addresses to the bss_conf list */
	ifa = idev->ifa_list;
	while (c < IEEE80211_BSS_ARP_ADDR_LIST_LEN && ifa) {
		bss_conf->arp_addr_list[c] = ifa->ifa_address;
		ifa = ifa->ifa_next;
		c++;
	}

	/* If not all addresses fit the list, disable filtering */
	if (ifa) {
		sdata->arp_filter_state = false;
		c = 0;
	} else {
		sdata->arp_filter_state = true;
	}
	bss_conf->arp_addr_cnt = c;

	if (sdata->vif.type == NL80211_IFTYPE_STATION) {
		/* Configure driver only if associated */
		if (ifmgd->associated) {
			bss_conf->arp_filter_enabled = sdata->arp_filter_state;
			mac80211_bss_info_change_notify(sdata,
				BSS_CHANGED_ARP_FILTER);
		}
		mutex_unlock(&ifmgd->mtx);
	} else {
		bss_conf->arp_filter_enabled = sdata->arp_filter_state;
		mac80211_bss_info_change_notify(sdata,
						 BSS_CHANGED_ARP_FILTER);
	}

	return NOTIFY_DONE;
}

#ifdef IPV6_FILTERING
static void ieee80211_ifa6_changed_work(struct work_struct *work)
{
	struct ieee80211_local *local =
			container_of(work, struct ieee80211_local, ifa6_changed_work);
	struct ieee80211_sub_if_data *sdata = local->ifa6_sdata;
	struct ieee80211_bss_conf *bss_conf = &sdata->vif.bss_conf;
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;

	mutex_lock(&ifmgd->mtx);
	if (ifmgd->associated) {
		bss_conf->ndp_filter_enabled = sdata->ndp_filter_state;
		mac80211_bss_info_change_notify(sdata,
							BSS_CHANGED_NDP_FILTER);
	}
	mutex_unlock(&ifmgd->mtx);
}

static int ieee80211_ifa6_changed(struct notifier_block *nb,
							    unsigned long data, void *arg)
{
	struct inet6_ifaddr *ifa = (struct inet6_ifaddr *)arg;
	struct ieee80211_local *local =
		   container_of(nb, struct ieee80211_local, ifa6_notifier);
	struct net_device *ndev = (struct net_device *)ifa->idev->dev;
	struct wireless_dev *wdev = ndev->ieee80211_ptr;
	struct inet6_dev *idev;
	struct ieee80211_sub_if_data *sdata;
	struct ieee80211_bss_conf *bss_conf;
	/*struct ieee80211_if_managed *ifmgd;*/
	int c = 0;

	/* Make sure it's our interface that got changed */
	if (!wdev)
		return NOTIFY_DONE;

	if (wdev->wiphy != local->hw.wiphy)
		return NOTIFY_DONE;

	sdata = IEEE80211_DEV_TO_SUB_IF(ndev);
	bss_conf = &sdata->vif.bss_conf;

	if (!ieee80211_sdata_running(sdata))
		return NOTIFY_DONE;

	/* NDP filtering is only supported in managed mode */
	switch (sdata->vif.type) {
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_GO:
		break;
	default:
		return NOTIFY_DONE;
	}

	idev = __in6_dev_get(sdata->dev);
	if (!idev)
		return NOTIFY_DONE;

#if 0
	if (sdata->vif.type == NL80211_IFTYPE_STATION) {
			ifmgd = &sdata->u.mgd;
			mutex_lock(&ifmgd->mtx);
	}
#endif

	list_for_each_entry(ifa, &idev->addr_list, if_list) {
		c++;
		if (c > IEEE80211_BSS_NDP_ADDR_LIST_LEN)
			break;
		memcpy(&bss_conf->ndp_addr_list[c-1],
				&ifa->addr, sizeof(struct in6_addr));
	}

	if (c > IEEE80211_BSS_NDP_ADDR_LIST_LEN) {
		sdata->ndp_filter_state = false;
		c = 0;
	} else {
		sdata->ndp_filter_state = true;
	}
	bss_conf->ndp_addr_cnt = c;

	if (sdata->vif.type == NL80211_IFTYPE_STATION) {
		local->ifa6_sdata = sdata;
		schedule_work(&local->ifa6_changed_work);
#if 0
		/* Configure driver only if associated */
		if (ifmgd->associated) {
			bss_conf->ndp_filter_enabled = sdata->ndp_filter_state;
			mac80211_bss_info_change_notify(sdata,
						BSS_CHANGED_NDP_FILTER);
		}
		mutex_unlock(&ifmgd->mtx);
#endif
	} else {
		bss_conf->ndp_filter_enabled = sdata->ndp_filter_state;
	}

	return NOTIFY_DONE;
}
#endif /*IPV6_FILTERING*/
#endif /*CONFIG_INET*/

/* There isn't a lot of sense in it, but you can transmit anything you like */
static const struct ieee80211_txrx_stypes
ieee80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			BIT(IEEE80211_STYPE_ACTION >> 4),
	},
	[NL80211_IFTYPE_MESH_POINT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_AUTH >> 4) |
			BIT(IEEE80211_STYPE_DEAUTH >> 4),
	},
	[NL80211_IFTYPE_P2P_DEVICE] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
	},

};

struct ieee80211_hw *xr_mac80211_alloc_hw(size_t priv_data_len,
					const struct ieee80211_ops *ops)
{
	struct ieee80211_local *local;
	int priv_size, i;
	struct wiphy *wiphy;

	/* Ensure 32-byte alignment of our private data and hw private data.
	 * We use the wiphy priv data for both our ieee80211_local and for
	 * the driver's private data
	 *
	 * In memory it'll be like this:
	 *
	 * +-------------------------+
	 * | struct wiphy	    |
	 * +-------------------------+
	 * | struct ieee80211_local  |
	 * +-------------------------+
	 * | driver's private data   |
	 * +-------------------------+
	 *
	 */
	priv_size = ALIGN(sizeof(*local), NETDEV_ALIGN) + priv_data_len;

	wiphy = wiphy_new(&xrmac_config_ops, priv_size);

	if (!wiphy)
		return NULL;

	wiphy->mgmt_stypes = ieee80211_default_mgmt_stypes;

	wiphy->privid = xrmac_wiphy_privid;

	wiphy->flags |= WIPHY_FLAG_NETNS_OK |
			WIPHY_FLAG_4ADDR_AP |
			WIPHY_FLAG_4ADDR_STATION |
			WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;

	if (!ops->set_key)
		wiphy->flags |= WIPHY_FLAG_IBSS_RSN;

	wiphy->bss_priv_size = sizeof(struct ieee80211_bss);

	local = wiphy_priv(wiphy);

	local->hw.wiphy = wiphy;

	local->hw.priv = (char *)local + ALIGN(sizeof(*local), NETDEV_ALIGN);

	BUG_ON(!ops->tx);
	BUG_ON(!ops->start);
	BUG_ON(!ops->stop);
	BUG_ON(!ops->config);
	BUG_ON(!ops->add_interface);
	BUG_ON(!ops->remove_interface);
	BUG_ON(!ops->configure_filter);
	local->ops = ops;

	local->hw.conf.chan_conf = &local->chan_state.conf;

	/* set up some defaults */
	local->hw.queues = 1;
	local->hw.max_rates = 1;
	local->hw.max_report_rates = 0;
	local->hw.max_rx_aggregation_subframes = IEEE80211_MAX_AMPDU_BUF_HE;
	local->hw.max_tx_aggregation_subframes = IEEE80211_MAX_AMPDU_BUF_HE;
	local->hw.offchannel_tx_hw_queue = IEEE80211_INVAL_HW_QUEUE;
	local->user_power_level = -1;
	local->uapsd_queues = IEEE80211_DEFAULT_UAPSD_QUEUES;
	local->uapsd_max_sp_len = IEEE80211_DEFAULT_MAX_SP_LEN;

	INIT_LIST_HEAD(&local->interfaces);
	mutex_init(&local->iflist_mtx);
	mutex_init(&local->mtx);

	mutex_init(&local->key_mtx);
	spin_lock_init(&local->filter_lock);
	spin_lock_init(&local->queue_stop_reason_lock);

	/*
	 * The rx_skb_queue is only accessed from tasklets,
	 * but other SKB queues are used from within IRQ
	 * context. Therefore, this one needs a different
	 * locking class so our direct, non-irq-safe use of
	 * the queue's lock doesn't throw lockdep warnings.
	 */
	skb_queue_head_init_class(&local->rx_skb_queue,
				  &ieee80211_rx_skb_queue_class);

	INIT_DELAYED_WORK(&local->scan_work, mac80211_scan_work);

	mac80211_work_init(local);

	INIT_WORK(&local->restart_work, ieee80211_restart_work);

#ifdef CONFIG_INET
#ifdef IPV6_FILTERING
		INIT_WORK(&local->ifa6_changed_work, ieee80211_ifa6_changed_work);
#endif /*IPV6_FILTERING*/
#endif

	INIT_WORK(&local->recalc_smps, mac80211_recalc_smps_work);
	local->smps_mode = IEEE80211_SMPS_OFF;

	INIT_WORK(&local->sched_scan_stopped_work,
		  xr_mac80211_sched_scan_stopped_work);

	xrmac_sta_info_init(local);

	for (i = 0; i < IEEE80211_MAX_QUEUES; i++) {
		skb_queue_head_init(&local->pending[i]);
		atomic_set(&local->agg_queue_stop[i], 0);
	}
	tasklet_init(&local->tx_pending_tasklet, mac80211_tx_pending,
		     (unsigned long)local);

	tasklet_init(&local->tasklet,
		     ieee80211_tasklet_handler,
		     (unsigned long) local);

	skb_queue_head_init(&local->skb_queue);
	skb_queue_head_init(&local->skb_queue_unreliable);

	ieee80211_led_names(local);

	mac80211_hw_roc_setup(local);

	return local_to_hw(local);
}

static int ieee80211_init_cipher_suites(struct ieee80211_local *local)
{
	bool have_wep = !fips_enabled; /* FIPS does not permit the use of RC4 */
	bool have_mfp = local->hw.flags & IEEE80211_HW_MFP_CAPABLE;
	const struct ieee80211_cipher_scheme *cs = local->hw.cipher_schemes;
	int n_suites = 0, r = 0, w = 0;
	u32 *suites;
	static const u32 cipher_suites[] = {
		/* keep WEP first, it may be removed below */
		WLAN_CIPHER_SUITE_WEP40,
		WLAN_CIPHER_SUITE_WEP104,
		WLAN_CIPHER_SUITE_TKIP,
		WLAN_CIPHER_SUITE_CCMP,

		/* keep last -- depends on hw flags! */
		WLAN_CIPHER_SUITE_AES_CMAC
	};

	/* Driver specifies the ciphers, we have nothing to do... */
	if (local->hw.wiphy->cipher_suites && have_wep)
		return 0;

	/* Set up cipher suites if driver relies on mac80211 cipher defs */
	if (!local->hw.wiphy->cipher_suites && !cs) {
		local->hw.wiphy->cipher_suites = cipher_suites;
		local->hw.wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);

		if (!have_mfp)
			local->hw.wiphy->n_cipher_suites--;

		if (!have_wep) {
			local->hw.wiphy->cipher_suites += 2;
			local->hw.wiphy->n_cipher_suites -= 2;
		}

		return 0;
	}

	if (!local->hw.wiphy->cipher_suites) {
		/*
		 * Driver specifies cipher schemes only
		 * We start counting ciphers defined by schemes, TKIP and CCMP
		 */
		n_suites = local->hw.n_cipher_schemes + 2;

		/* check if we have WEP40 and WEP104 */
		if (have_wep)
			n_suites += 2;

		/* check if we have AES_CMAC */
		if (have_mfp)
			n_suites++;

		suites = kmalloc(sizeof(u32) * n_suites, GFP_KERNEL);
		if (!suites)
			return -ENOMEM;

		suites[w++] = WLAN_CIPHER_SUITE_CCMP;
		suites[w++] = WLAN_CIPHER_SUITE_TKIP;

		if (have_wep) {
			suites[w++] = WLAN_CIPHER_SUITE_WEP40;
			suites[w++] = WLAN_CIPHER_SUITE_WEP104;
		}

		if (have_mfp)
			suites[w++] = WLAN_CIPHER_SUITE_AES_CMAC;

		for (r = 0; r < local->hw.n_cipher_schemes; r++)
			suites[w++] = cs[r].cipher;
	} else {
		/* Driver provides cipher suites, but we need to exclude WEP */
		suites = kmemdup(local->hw.wiphy->cipher_suites,
				 sizeof(u32) * local->hw.wiphy->n_cipher_suites,
				 GFP_KERNEL);
		if (!suites)
			return -ENOMEM;

		for (r = 0; r < local->hw.wiphy->n_cipher_suites; r++) {
			u32 suite = local->hw.wiphy->cipher_suites[r];

			if (suite == WLAN_CIPHER_SUITE_WEP40 ||
			    suite == WLAN_CIPHER_SUITE_WEP104)
				continue;
			suites[w++] = suite;
		}
	}

	local->hw.wiphy->cipher_suites = suites;
	local->hw.wiphy->n_cipher_suites = w;
	local->wiphy_ciphers_allocated = true;

	return 0;
}

int xr_mac80211_register_hw(struct ieee80211_hw *hw)
{
	struct ieee80211_local *local = hw_to_local(hw);
	struct ieee80211_channel_state *chan_state = &local->chan_state;
	int result, i;
	enum nl80211_band band;
	int channels, max_bitrates;
	bool supp_ht;

	if (hw->flags & IEEE80211_HW_QUEUE_CONTROL &&
	    (local->hw.offchannel_tx_hw_queue == IEEE80211_INVAL_HW_QUEUE ||
	     local->hw.offchannel_tx_hw_queue >= local->hw.queues))
		return -EINVAL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
		if ((hw->wiphy->wowlan->flags || hw->wiphy->wowlan->n_patterns)
#else
		if ((hw->wiphy->wowlan.flags || hw->wiphy->wowlan.n_patterns)
#endif

#ifdef CONFIG_PM
	    && (!local->ops->suspend || !local->ops->resume)
#endif
	    )
		return -EINVAL;

	if (hw->max_report_rates == 0)
		hw->max_report_rates = hw->max_rates;

	/*
	 * generic code guarantees at least one band,
	 * set this very early because much code assumes
	 * that chan_state->conf.channel is assigned
	 */
	channels = 0;
	max_bitrates = 0;
	supp_ht = false;
	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		struct ieee80211_supported_band *sband;

		sband = local->hw.wiphy->bands[band];
		if (!sband)
			continue;
		if (!chan_state->oper_channel) {
			/* init channel we're on */
			/* soumik: Set default channel to a non-social channel */
			chan_state->conf.channel =
			/* chan_state->oper_channel = &sband->channels[0]; */
			chan_state->oper_channel = &sband->channels[2];
			chan_state->conf.channel_type = NL80211_CHAN_NO_HT;
		}
		channels += sband->n_channels;

		if (max_bitrates < sband->n_bitrates)
			max_bitrates = sband->n_bitrates;
		supp_ht = supp_ht || sband->ht_cap.ht_supported;
	}

	local->int_scan_req = kzalloc(sizeof(*local->int_scan_req) +
				      sizeof(void *) * channels, GFP_KERNEL);
	if (!local->int_scan_req)
		return -ENOMEM;

	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		if (!local->hw.wiphy->bands[band])
			continue;
		local->int_scan_req->rates[band] = (u32) -1;
	}

	/* if low-level driver supports AP, we also support VLAN */
	if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_AP)) {
		hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_AP_VLAN);
		hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_AP_VLAN);
	}

	/* mac80211 always supports monitor */
	hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_MONITOR);
	hw->wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);

	/*
	 * mac80211 doesn't support more than 1 channel, and also not more
	 * than one IBSS interface
	 */
	for (i = 0; i < hw->wiphy->n_iface_combinations; i++) {
		const struct ieee80211_iface_combination *c;
		int j;

		c = &hw->wiphy->iface_combinations[i];

		if (c->num_different_channels > 1)
			return -EINVAL;

		for (j = 0; j < c->n_limits; j++)
			if ((c->limits[j].types & BIT(NL80211_IFTYPE_ADHOC)) &&
			    c->limits[j].max > 1)
				return -EINVAL;
	}

#ifndef CONFIG_XRMAC_MESH
	/* mesh depends on Kconfig, but drivers should set it if they want */
	local->hw.wiphy->interface_modes &= ~BIT(NL80211_IFTYPE_MESH_POINT);
#endif

	/* if the underlying driver supports mesh, mac80211 will (at least)
	 * provide routing of mesh authentication frames to userspace */
	if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_MESH_POINT))
		local->hw.wiphy->flags |= WIPHY_FLAG_MESH_AUTH;

	/* mac80211 supports control port protocol changing */
	local->hw.wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;

	if (local->hw.flags & IEEE80211_HW_SIGNAL_DBM)
		local->hw.wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	else if (local->hw.flags & IEEE80211_HW_SIGNAL_UNSPEC)
		local->hw.wiphy->signal_type = CFG80211_SIGNAL_TYPE_UNSPEC;

	WARN((local->hw.flags & IEEE80211_HW_SUPPORTS_UAPSD)
	     && (local->hw.flags & IEEE80211_HW_PS_NULLFUNC_STACK),
	     "U-APSD not supported with HW_PS_NULLFUNC_STACK\n");

	/*
	 * Calculate scan IE length -- we need this to alloc
	 * memory and to subtract from the driver limit. It
	 * includes the DS Params, (extended) supported rates, and HT
	 * information -- SSID is the driver's responsibility.
	 */
	local->scan_ies_len = 4 + max_bitrates /* (ext) supp rates */ +
		3 /* DS Params */;
	if (supp_ht)
		local->scan_ies_len += 2 + sizeof(struct ieee80211_ht_cap);

	if (!local->ops->hw_scan) {
		/* For hw_scan, driver needs to set these up. */
		local->hw.wiphy->max_scan_ssids = 4;
		local->hw.wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;
	}

	/*
	 * If the driver supports any scan IEs, then assume the
	 * limit includes the IEs mac80211 will add, otherwise
	 * leave it at zero and let the driver sort it out; we
	 * still pass our IEs to the driver but userspace will
	 * not be allowed to in that case.
	 */
	if (local->hw.wiphy->max_scan_ie_len)
		local->hw.wiphy->max_scan_ie_len -= local->scan_ies_len;

	/* Set up cipher suites unless driver already did */
	WARN_ON(!ieee80211_cs_list_valid(local->hw.cipher_schemes,
					 local->hw.n_cipher_schemes));

	result = ieee80211_init_cipher_suites(local);
	if (result < 0)
		goto fail_wiphy_register;

	if (!local->ops->remain_on_channel)
		local->hw.wiphy->max_remain_on_channel_duration = 5000;

	/* mac80211 based drivers don't support internal TDLS setup */
	if (local->hw.wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS)
		local->hw.wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;

	result = wiphy_register(local->hw.wiphy);
	if (result < 0)
		goto fail_wiphy_register;

	/*
	 * We use the number of queues for feature tests (QoS, HT) internally
	 * so restrict them appropriately.
	 */
	if (hw->queues > IEEE80211_MAX_QUEUES)
		hw->queues = IEEE80211_MAX_QUEUES;

	local->workqueue =
		alloc_ordered_workqueue(wiphy_name(local->hw.wiphy), 0);
	if (!local->workqueue) {
		result = -ENOMEM;
		goto fail_workqueue;
	}

	/*
	 * The hardware needs headroom for sending the frame,
	 * and we need some headroom for passing the frame to monitor
	 * interfaces, but never both at the same time.
	 */
	local->tx_headroom = max_t(unsigned int, local->hw.extra_tx_headroom,
				   IEEE80211_TX_STATUS_HEADROOM);

#ifdef CONFIG_XRMAC_DEBUGFS
	xrmac_debugfs_hw_add(local);
#endif /* CONFIG_XRMAC_DEBUGFS */
	/*
	 * if the driver doesn't specify a max listen interval we
	 * use 5 which should be a safe default
	 */
	if (local->hw.max_listen_interval == 0)
		local->hw.max_listen_interval = 5;

	result = mac80211_wep_init(local);
	if (result < 0)
		wiphy_debug(local->hw.wiphy, "Failed to initialize wep: %d\n",
			    result);

	rtnl_lock();

	result = mac80211_init_rate_ctrl_alg(local,
					      hw->rate_control_algorithm);
	if (result < 0) {
		wiphy_debug(local->hw.wiphy,
			    "Failed to initialize rate control algorithm\n");
		goto fail_rate;
	}

	/* add one default STA interface if supported */
	if (local->hw.wiphy->interface_modes & BIT(NL80211_IFTYPE_STATION)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
		result = mac80211_if_add(local, "wlan%d", NET_NAME_ENUM, NULL,
					  NL80211_IFTYPE_STATION, NULL);
#else
		result = mac80211_if_add(local, "wlan%d", NULL,
					  NL80211_IFTYPE_STATION, NULL);
#endif
		if (result)
			wiphy_warn(local->hw.wiphy,
				   "Failed to add default virtual iface\n");
#ifdef OLD_P2P_MODE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
		result = mac80211_if_add(local, "p2p%d", NET_NAME_ENUM, NULL,
							  NL80211_IFTYPE_STATION, NULL);
#else
		result = mac80211_if_add(local, "p2p%d", NULL,
					  NL80211_IFTYPE_STATION, NULL);
#endif
		if (result)
			wiphy_warn(local->hw.wiphy,
				   "Failed to add default virtual p2p iface\n");
#endif

	}

	rtnl_unlock();

	ieee80211_led_init(local);

	local->network_latency_notifier.notifier_call =
		mac80211_max_network_latency;
	result = xr_pm_qos_add_notifier(PM_QOS_NETWORK_LATENCY,
				     &local->network_latency_notifier);
	if (result) {
		rtnl_lock();
		goto fail_pm_qos;
	}

#ifdef CONFIG_INET
	local->ifa_notifier.notifier_call = ieee80211_ifa_changed;
	result = register_inetaddr_notifier(&local->ifa_notifier);
	if (result)
		goto fail_ifa;

#ifdef IPV6_FILTERING
	local->ifa6_notifier.notifier_call = ieee80211_ifa6_changed;
	result = register_inet6addr_notifier(&local->ifa6_notifier);
	if (result)
		goto fail_ifa;
#endif /*IPV6_FILTERING*/
#endif

	return 0;

#ifdef CONFIG_INET
 fail_ifa:
	xr_pm_qos_remove_notifier(PM_QOS_NETWORK_LATENCY,
			       &local->network_latency_notifier);
	rtnl_lock();
#endif
 fail_pm_qos:
	ieee80211_led_exit(local);
	mac80211_remove_interfaces(local);
 fail_rate:
	rtnl_unlock();
	xrmac_sta_info_stop(local);
	destroy_workqueue(local->workqueue);
 fail_workqueue:
	wiphy_unregister(local->hw.wiphy);
 fail_wiphy_register:
	if (local->wiphy_ciphers_allocated)
		kfree(local->hw.wiphy->cipher_suites);
	kfree(local->int_scan_req);
	return result;
}

void xr_mac80211_unregister_hw(struct ieee80211_hw *hw)
{
	struct ieee80211_local *local = hw_to_local(hw);

	tasklet_kill(&local->tx_pending_tasklet);
	tasklet_kill(&local->tasklet);

	xr_pm_qos_remove_notifier(PM_QOS_NETWORK_LATENCY,
			       &local->network_latency_notifier);
#ifdef CONFIG_INET
	unregister_inetaddr_notifier(&local->ifa_notifier);
#ifdef IPV6_FILTERING
	unregister_inet6addr_notifier(&local->ifa6_notifier);
#endif /*IPV6_FILTERING*/
#endif

	rtnl_lock();

	/*
	 * At this point, interface list manipulations are fine
	 * because the driver cannot be handing us frames any
	 * more and the tasklet is killed.
	 */
	mac80211_remove_interfaces(local);

	rtnl_unlock();

	/*
	 * Now all work items will be gone, but the
	 * timer might still be armed, so delete it
	 */
	del_timer_sync(&local->work_timer);

	cancel_work_sync(&local->restart_work);

#ifdef IPV6_FILTERING
#ifdef CONFIG_INET
	cancel_work_sync(&local->ifa6_changed_work);
#endif
#endif /*IPV6_FILTERING*/

	mac80211_clear_tx_pending(local);
	xrmac_rate_control_deinitialize(local);

	if (skb_queue_len(&local->skb_queue) ||
	    skb_queue_len(&local->skb_queue_unreliable))
		wiphy_warn(local->hw.wiphy, "skb_queue not empty\n");
	skb_queue_purge(&local->skb_queue);
	skb_queue_purge(&local->skb_queue_unreliable);
	skb_queue_purge(&local->rx_skb_queue);

#ifdef CONFIG_XRMAC_XR_ROAMING_CHANGES
	/*
	 * Work can also be sheduled in call_rcu callback.
	 * Wait for all rcu callbacks to finish.
	 */
	rcu_barrier();
#endif
	destroy_workqueue(local->workqueue);
	wiphy_unregister(local->hw.wiphy);
	xrmac_sta_info_stop(local);
	ieee80211_led_exit(local);
	kfree(local->int_scan_req);
}

void xr_mac80211_free_hw(struct ieee80211_hw *hw)
{
	struct ieee80211_local *local = hw_to_local(hw);

	mutex_destroy(&local->iflist_mtx);
	mutex_destroy(&local->mtx);

	if (local->wiphy_ciphers_allocated)
		kfree(local->hw.wiphy->cipher_suites);

	wiphy_free(local->hw.wiphy);
}

int __init ieee80211_init(void)
{
	struct sk_buff *skb;
	int ret;

	BUILD_BUG_ON(sizeof(struct ieee80211_tx_info) > sizeof(skb->cb));
	BUILD_BUG_ON(offsetof(struct ieee80211_tx_info, driver_data) +
		     IEEE80211_TX_INFO_DRIVER_DATA_SIZE > sizeof(skb->cb));

	ret = xrmac_rc80211_minstrel_init();
	if (ret)
		return ret;

	ret = xrmac_rc80211_minstrel_ht_init();
	if (ret)
		goto err_minstrel;

	ret = xrmac_rc80211_pid_init();
	if (ret)
		goto err_pid;

	ret = mac80211_iface_init();
	if (ret)
		goto err_netdev;

	return 0;
 err_netdev:
	xrmac_rc80211_pid_exit();
 err_pid:
	xrmac_rc80211_minstrel_ht_exit();
 err_minstrel:
	xrmac_rc80211_minstrel_exit();

	return ret;
}

void ieee80211_exit(void)
{
	xrmac_rc80211_pid_exit();
	xrmac_rc80211_minstrel_ht_exit();
	xrmac_rc80211_minstrel_exit();

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37))
	flush_scheduled_work();
#endif

	if (xrmac_mesh_allocated)
		mac80211s_stop();

	mac80211_iface_exit();

	rcu_barrier();
}

MODULE_DESCRIPTION("IEEE 802.11 subsystem");
MODULE_LICENSE("GPL");
