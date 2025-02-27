#include <net/mac80211_xr.h>
#include <net/rtnetlink.h>
#include <linux/pm_qos.h>

#include "ieee80211_i.h"
#include "mesh.h"
#include "driver-ops.h"
#include "led.h"

/* return value indicates whether the driver should be further notified */
static bool ieee80211_quiesce(struct ieee80211_sub_if_data *sdata)
{
	switch (sdata->vif.type) {
	case NL80211_IFTYPE_STATION:
		mac80211_sta_quiesce(sdata);
		return true;
	case NL80211_IFTYPE_ADHOC:
		mac80211_ibss_quiesce(sdata);
		return true;
	case NL80211_IFTYPE_MESH_POINT:
		mac80211_mesh_quiesce(sdata);
		return true;
	case NL80211_IFTYPE_AP_VLAN:
	case NL80211_IFTYPE_MONITOR:
		/* don't tell driver about this */
		return false;
	default:
		return true;
	}
}

int __mac80211_suspend(struct ieee80211_hw *hw, struct cfg80211_wowlan *wowlan)
{
	struct ieee80211_local *local = hw_to_local(hw);
	struct ieee80211_sub_if_data *sdata;
	struct sta_info *sta;

	/* PM code has a watchdog to trigger a BUG when
	 * suspend callback is not returning in several seconds.
	 * Some WLAN hardware has longer timeouts for non-interruptible
	 * configuration-related operations, leading to the watchdog
	 * timeout while mac80211_scan_cancel is waiting on the mutex.
	 *
	 * The code below checks if interface mutex is already held
	 * and rejects suspend if there is a possibility of locking.
	 *
	 * It's a bit racy, but handles most of cases.
	 */
	if (mutex_trylock(&local->mtx))
		mutex_unlock(&local->mtx);
	else {
		wiphy_warn(hw->wiphy, "Suspend when operation "
			"is in progress. Suspend aborted.\n");
		return -EBUSY;
	}

/*
	if (!local->open_count)
		goto suspend;
*/
	mac80211_scan_cancel(local);

	if (hw->flags & IEEE80211_HW_AMPDU_AGGREGATION) {
		mutex_lock(&local->sta_mtx);
		list_for_each_entry(sta, &local->sta_list, list) {
			set_sta_flag(sta, WLAN_STA_BLOCK_BA);
			mac80211_sta_tear_down_BA_sessions(sta, true);
		}
		mutex_unlock(&local->sta_mtx);
	}

	xr_mac80211_stop_queues_by_reason(hw,
			IEEE80211_QUEUE_STOP_REASON_SUSPEND);

	/* flush out all packets */
	synchronize_net();
	list_for_each_entry(sdata, &local->interfaces, list)
		drv_flush(local, sdata, false);

	local->quiescing = true;
	/* make quiescing visible to timers everywhere */
	mb();

	flush_workqueue(local->workqueue);

	/* Don't try to run timers while suspended. */
	del_timer_sync(&local->sta_cleanup);

	 /*
	 * Note that this particular timer doesn't need to be
	 * restarted at resume.
	 */
	list_for_each_entry(sdata, &local->interfaces, list) {
		cancel_work_sync(&sdata->dynamic_ps_enable_work);
		del_timer_sync(&sdata->dynamic_ps_timer);
	}

	/*wowlan is useless to xradio_wow_suspend.*/
	local->wowlan = true;
	if (local->wowlan) {
		int err = drv_suspend(local, wowlan);
		if (err < 0) {
			local->quiescing = false;
			xr_mac80211_wake_queues_by_reason(hw,
					IEEE80211_QUEUE_STOP_REASON_SUSPEND);
			return err;
		} else if (err > 0) {
			WARN_ON(err != 1);
			local->wowlan = false;
		} else {
			list_for_each_entry(sdata, &local->interfaces, list) {
				cancel_work_sync(&sdata->work);
				ieee80211_quiesce(sdata);
			}
			goto suspend;
		}
		list_for_each_entry(sdata, &local->interfaces, list) {
			cancel_work_sync(&sdata->work);
		}
		goto suspend;
	}

	/* disable keys */
	list_for_each_entry(sdata, &local->interfaces, list)
		mac80211_disable_keys(sdata);

	/* tear down aggregation sessions and remove STAs */
	mutex_lock(&local->sta_mtx);
	list_for_each_entry(sta, &local->sta_list, list) {
		if (sta->uploaded) {
			sdata = sta->sdata;
			if (sdata->vif.type == NL80211_IFTYPE_AP_VLAN)
				sdata = container_of(sdata->bss,
					     struct ieee80211_sub_if_data,
					     u.ap);

			drv_sta_remove(local, sdata, &sta->sta);
		}

		xrmac_mesh_plink_quiesce(sta);
	}
	mutex_unlock(&local->sta_mtx);

	/* remove all interfaces */
	list_for_each_entry(sdata, &local->interfaces, list) {
		cancel_work_sync(&sdata->work);

		if (!ieee80211_quiesce(sdata))
			continue;

		if (!ieee80211_sdata_running(sdata))
			continue;

		/* disable beaconing */
		mac80211_bss_info_change_notify(sdata,
			BSS_CHANGED_BEACON_ENABLED);

		drv_remove_interface(local, &sdata->vif);
	}

	/* stop hardware - this must stop RX */
	if (local->open_count)
		mac80211_stop_device(local);

 suspend:
	local->suspended = true;
	/* need suspended to be visible before quiescing is false */
	barrier();
	local->quiescing = false;

#ifdef ROAM_OFFLOAD
	local->sched_scanning = true;
#endif /*ROAM_OFFLOAD*/

	return 0;
}

/*
 * __ieee80211_resume() is a static inline which just calls
 * mac80211_reconfig(), which is also needed for hardware
 * hang/firmware failure/etc. recovery.
 */

static BLOCKING_NOTIFIER_HEAD(network_lat_notifier);
static struct pm_qos_constraints network_lat_constraints = {
	.list = PLIST_HEAD_INIT(network_lat_constraints.list),
	.target_value = PM_QOS_NETWORK_LAT_DEFAULT_VALUE,
	.default_value = PM_QOS_NETWORK_LAT_DEFAULT_VALUE,
	.no_constraint_value = PM_QOS_NETWORK_LAT_DEFAULT_VALUE,
	.type = PM_QOS_MIN,
	.notifiers = &network_lat_notifier,
};
static struct xr_pm_qos_object network_lat_pm_qos = {
	.constraints = &network_lat_constraints,
	.name = "network_latency",
};

static BLOCKING_NOTIFIER_HEAD(network_throughput_notifier);
static struct pm_qos_constraints network_tput_constraints = {
	.list = PLIST_HEAD_INIT(network_tput_constraints.list),
	.target_value = PM_QOS_NETWORK_THROUGHPUT_DEFAULT_VALUE,
	.default_value = PM_QOS_NETWORK_THROUGHPUT_DEFAULT_VALUE,
	.no_constraint_value = PM_QOS_NETWORK_THROUGHPUT_DEFAULT_VALUE,
	.type = PM_QOS_MAX,
	.notifiers = &network_throughput_notifier,
};
struct xr_pm_qos_object network_throughput_pm_qos = {
	.constraints = &network_tput_constraints,
	.name = "network_throughput",
};

#if 0
static BLOCKING_NOTIFIER_HEAD(memory_bandwidth_notifier);
static struct pm_qos_constraints memory_bw_constraints = {
	.list = PLIST_HEAD_INIT(memory_bw_constraints.list),
	.target_value = PM_QOS_MEMORY_BANDWIDTH_DEFAULT_VALUE,
	.default_value = PM_QOS_MEMORY_BANDWIDTH_DEFAULT_VALUE,
	.no_constraint_value = PM_QOS_MEMORY_BANDWIDTH_DEFAULT_VALUE,
	.type = PM_QOS_SUM,
	.notifiers = &memory_bandwidth_notifier,
};
static struct xr_pm_qos_object memory_bandwidth_pm_qos = {
	.constraints = &memory_bw_constraints,
	.name = "memory_bandwidth",
};
#endif

static struct xr_pm_qos_object null_pm_qos;
static struct xr_pm_qos_object cpu_dma_pm_qos;

static struct xr_pm_qos_object *xr_pm_qos_array[] = {
	&null_pm_qos,
	&cpu_dma_pm_qos,
	&network_lat_pm_qos,
	&network_throughput_pm_qos,
#if 0
	&memory_bandwidth_pm_qos,
#endif
};

int xr_pm_qos_request(int pm_qos_class)
{
	return xr_pm_qos_array[pm_qos_class]->constraints->target_value;
}

int xr_pm_qos_add_notifier(int pm_qos_class,
			struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_register(
			xr_pm_qos_array[pm_qos_class]->constraints->notifiers,
			notifier);

	return retval;
}

int xr_pm_qos_remove_notifier(int pm_qos_class,
				struct notifier_block *notifier)
{
	 int retval;

	retval = blocking_notifier_chain_unregister(
			xr_pm_qos_array[pm_qos_class]->constraints->notifiers,
			notifier);

	return retval;
}

