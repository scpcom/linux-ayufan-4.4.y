#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/device.h>
#include <linux/kallsyms.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/resume-trace.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/async.h>
#include <linux/suspend.h>

#include "../base.h"
#include "power.h"

#ifdef CONFIG_ARCH_GEN3
int suspend_device(struct device *dev, pm_message_t state);
int resume_device(struct device *dev, pm_message_t state);
#endif
 
LIST_HEAD(dpm_list);
LIST_HEAD(dpm_prepared_list);
LIST_HEAD(dpm_suspended_list);
LIST_HEAD(dpm_noirq_list);

struct suspend_stats suspend_stats;
static DEFINE_MUTEX(dpm_list_mtx);
static pm_message_t pm_transition;

#ifdef CONFIG_ARCH_GEN3
int async_error;
#else
static int async_error;
#endif

void device_pm_init(struct device *dev)
{
	dev->power.is_prepared = false;
	dev->power.is_suspended = false;
	init_completion(&dev->power.completion);
	complete_all(&dev->power.completion);
	dev->power.wakeup = NULL;
	spin_lock_init(&dev->power.lock);
	pm_runtime_init(dev);
	INIT_LIST_HEAD(&dev->power.entry);
	dev->power.power_state = PMSG_INVALID;
}

void device_pm_lock(void)
{
	mutex_lock(&dpm_list_mtx);
}

void device_pm_unlock(void)
{
	mutex_unlock(&dpm_list_mtx);
}

void device_pm_add(struct device *dev)
{
	pr_debug("PM: Adding info for %s:%s\n",
		 dev->bus ? dev->bus->name : "No Bus", dev_name(dev));
	mutex_lock(&dpm_list_mtx);
	if (dev->parent && dev->parent->power.is_prepared)
		dev_warn(dev, "parent %s should not be sleeping\n",
			dev_name(dev->parent));
	list_add_tail(&dev->power.entry, &dpm_list);
	dev_pm_qos_constraints_init(dev);
	mutex_unlock(&dpm_list_mtx);
}

void device_pm_remove(struct device *dev)
{
	pr_debug("PM: Removing info for %s:%s\n",
		 dev->bus ? dev->bus->name : "No Bus", dev_name(dev));
	complete_all(&dev->power.completion);
	mutex_lock(&dpm_list_mtx);
	dev_pm_qos_constraints_destroy(dev);
	list_del_init(&dev->power.entry);
	mutex_unlock(&dpm_list_mtx);
	device_wakeup_disable(dev);
	pm_runtime_remove(dev);
}

void device_pm_move_before(struct device *deva, struct device *devb)
{
	pr_debug("PM: Moving %s:%s before %s:%s\n",
		 deva->bus ? deva->bus->name : "No Bus", dev_name(deva),
		 devb->bus ? devb->bus->name : "No Bus", dev_name(devb));
	 
	list_move_tail(&deva->power.entry, &devb->power.entry);
}

void device_pm_move_after(struct device *deva, struct device *devb)
{
	pr_debug("PM: Moving %s:%s after %s:%s\n",
		 deva->bus ? deva->bus->name : "No Bus", dev_name(deva),
		 devb->bus ? devb->bus->name : "No Bus", dev_name(devb));
	 
	list_move(&deva->power.entry, &devb->power.entry);
}

void device_pm_move_last(struct device *dev)
{
	pr_debug("PM: Moving %s:%s to end of list\n",
		 dev->bus ? dev->bus->name : "No Bus", dev_name(dev));
	list_move_tail(&dev->power.entry, &dpm_list);
}

static ktime_t initcall_debug_start(struct device *dev)
{
	ktime_t calltime = ktime_set(0, 0);

	if (initcall_debug) {
		pr_info("calling  %s+ @ %i\n",
				dev_name(dev), task_pid_nr(current));
		calltime = ktime_get();
	}

	return calltime;
}

static void initcall_debug_report(struct device *dev, ktime_t calltime,
				  int error)
{
	ktime_t delta, rettime;

	if (initcall_debug) {
		rettime = ktime_get();
		delta = ktime_sub(rettime, calltime);
		pr_info("call %s+ returned %d after %Ld usecs\n", dev_name(dev),
			error, (unsigned long long)ktime_to_ns(delta) >> 10);
	}
}

static void dpm_wait(struct device *dev, bool async)
{
	if (!dev)
		return;

	if (async || (pm_async_enabled && dev->power.async_suspend))
		wait_for_completion(&dev->power.completion);
}

static int dpm_wait_fn(struct device *dev, void *async_ptr)
{
	dpm_wait(dev, *((bool *)async_ptr));
	return 0;
}

static void dpm_wait_for_children(struct device *dev, bool async)
{
       device_for_each_child(dev, &async, dpm_wait_fn);
}

static int pm_op(struct device *dev,
		 const struct dev_pm_ops *ops,
		 pm_message_t state)
{
	int error = 0;
	ktime_t calltime;

	calltime = initcall_debug_start(dev);

	switch (state.event) {
#ifdef CONFIG_SUSPEND
	case PM_EVENT_SUSPEND:
		if (ops->suspend) {
			error = ops->suspend(dev);
			suspend_report_result(ops->suspend, error);
		}
		break;
	case PM_EVENT_RESUME:
		if (ops->resume) {
			error = ops->resume(dev);
			suspend_report_result(ops->resume, error);
		}
		break;
#endif  
#ifdef CONFIG_HIBERNATE_CALLBACKS
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		if (ops->freeze) {
			error = ops->freeze(dev);
			suspend_report_result(ops->freeze, error);
		}
		break;
	case PM_EVENT_HIBERNATE:
		if (ops->poweroff) {
			error = ops->poweroff(dev);
			suspend_report_result(ops->poweroff, error);
		}
		break;
	case PM_EVENT_THAW:
	case PM_EVENT_RECOVER:
		if (ops->thaw) {
			error = ops->thaw(dev);
			suspend_report_result(ops->thaw, error);
		}
		break;
	case PM_EVENT_RESTORE:
		if (ops->restore) {
			error = ops->restore(dev);
			suspend_report_result(ops->restore, error);
		}
		break;
#endif  
	default:
		error = -EINVAL;
	}

	initcall_debug_report(dev, calltime, error);

	return error;
}

static int pm_noirq_op(struct device *dev,
			const struct dev_pm_ops *ops,
			pm_message_t state)
{
	int error = 0;
	ktime_t calltime = ktime_set(0, 0), delta, rettime;

	if (initcall_debug) {
		pr_info("calling  %s+ @ %i, parent: %s\n",
				dev_name(dev), task_pid_nr(current),
				dev->parent ? dev_name(dev->parent) : "none");
		calltime = ktime_get();
	}

	switch (state.event) {
#ifdef CONFIG_SUSPEND
	case PM_EVENT_SUSPEND:
		if (ops->suspend_noirq) {
			error = ops->suspend_noirq(dev);
			suspend_report_result(ops->suspend_noirq, error);
		}
		break;
	case PM_EVENT_RESUME:
		if (ops->resume_noirq) {
			error = ops->resume_noirq(dev);
			suspend_report_result(ops->resume_noirq, error);
		}
		break;
#endif  
#ifdef CONFIG_HIBERNATE_CALLBACKS
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		if (ops->freeze_noirq) {
			error = ops->freeze_noirq(dev);
			suspend_report_result(ops->freeze_noirq, error);
		}
		break;
	case PM_EVENT_HIBERNATE:
		if (ops->poweroff_noirq) {
			error = ops->poweroff_noirq(dev);
			suspend_report_result(ops->poweroff_noirq, error);
		}
		break;
	case PM_EVENT_THAW:
	case PM_EVENT_RECOVER:
		if (ops->thaw_noirq) {
			error = ops->thaw_noirq(dev);
			suspend_report_result(ops->thaw_noirq, error);
		}
		break;
	case PM_EVENT_RESTORE:
		if (ops->restore_noirq) {
			error = ops->restore_noirq(dev);
			suspend_report_result(ops->restore_noirq, error);
		}
		break;
#endif  
	default:
		error = -EINVAL;
	}

	if (initcall_debug) {
		rettime = ktime_get();
		delta = ktime_sub(rettime, calltime);
		printk("initcall %s_i+ returned %d after %Ld usecs\n",
			dev_name(dev), error,
			(unsigned long long)ktime_to_ns(delta) >> 10);
	}

	return error;
}

static char *pm_verb(int event)
{
	switch (event) {
	case PM_EVENT_SUSPEND:
		return "suspend";
	case PM_EVENT_RESUME:
		return "resume";
	case PM_EVENT_FREEZE:
		return "freeze";
	case PM_EVENT_QUIESCE:
		return "quiesce";
	case PM_EVENT_HIBERNATE:
		return "hibernate";
	case PM_EVENT_THAW:
		return "thaw";
	case PM_EVENT_RESTORE:
		return "restore";
	case PM_EVENT_RECOVER:
		return "recover";
	default:
		return "(unknown PM event)";
	}
}

static void pm_dev_dbg(struct device *dev, pm_message_t state, char *info)
{
	dev_dbg(dev, "%s%s%s\n", info, pm_verb(state.event),
		((state.event & PM_EVENT_SLEEP) && device_may_wakeup(dev)) ?
		", may wakeup" : "");
}

static void pm_dev_err(struct device *dev, pm_message_t state, char *info,
			int error)
{
	printk(KERN_ERR "PM: Device %s failed to %s%s: error %d\n",
		dev_name(dev), pm_verb(state.event), info, error);
}

static void dpm_show_time(ktime_t starttime, pm_message_t state, char *info)
{
	ktime_t calltime;
	u64 usecs64;
	int usecs;

	calltime = ktime_get();
	usecs64 = ktime_to_ns(ktime_sub(calltime, starttime));
	do_div(usecs64, NSEC_PER_USEC);
	usecs = usecs64;
	if (usecs == 0)
		usecs = 1;
	pr_info("PM: %s%s%s of devices complete after %ld.%03ld msecs\n",
		info ?: "", info ? " " : "", pm_verb(state.event),
		usecs / USEC_PER_MSEC, usecs % USEC_PER_MSEC);
}

static int device_resume_noirq(struct device *dev, pm_message_t state)
{
	int error = 0;

	TRACE_DEVICE(dev);
	TRACE_RESUME(0);

	if (dev->pm_domain) {
		pm_dev_dbg(dev, state, "EARLY power domain ");
		error = pm_noirq_op(dev, &dev->pm_domain->ops, state);
	} else if (dev->type && dev->type->pm) {
		pm_dev_dbg(dev, state, "EARLY type ");
		error = pm_noirq_op(dev, dev->type->pm, state);
	} else if (dev->class && dev->class->pm) {
		pm_dev_dbg(dev, state, "EARLY class ");
		error = pm_noirq_op(dev, dev->class->pm, state);
	} else if (dev->bus && dev->bus->pm) {
		pm_dev_dbg(dev, state, "EARLY ");
		error = pm_noirq_op(dev, dev->bus->pm, state);
	}

	TRACE_RESUME(error);
	return error;
}

void dpm_resume_noirq(pm_message_t state)
{
	ktime_t starttime = ktime_get();

	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_noirq_list)) {
		struct device *dev = to_device(dpm_noirq_list.next);
		int error;

		get_device(dev);
		list_move_tail(&dev->power.entry, &dpm_suspended_list);
		mutex_unlock(&dpm_list_mtx);

		error = device_resume_noirq(dev, state);
		if (error) {
			suspend_stats.failed_resume_noirq++;
			dpm_save_failed_step(SUSPEND_RESUME_NOIRQ);
			dpm_save_failed_dev(dev_name(dev));
			pm_dev_err(dev, state, " early", error);
		}

		mutex_lock(&dpm_list_mtx);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	dpm_show_time(starttime, state, "early");
	resume_device_irqs();
}
EXPORT_SYMBOL_GPL(dpm_resume_noirq);

static int legacy_resume(struct device *dev, int (*cb)(struct device *dev))
{
	int error;
	ktime_t calltime;

	calltime = initcall_debug_start(dev);

	error = cb(dev);
	suspend_report_result(cb, error);

	initcall_debug_report(dev, calltime, error);

	return error;
}

static int device_resume(struct device *dev, pm_message_t state, bool async)
{
	int error = 0;
	bool put = false;

	TRACE_DEVICE(dev);
	TRACE_RESUME(0);

	dpm_wait(dev->parent, async);
	device_lock(dev);

	dev->power.is_prepared = false;

	if (!dev->power.is_suspended)
		goto Unlock;

	pm_runtime_enable(dev);
	put = true;

	if (dev->pm_domain) {
		pm_dev_dbg(dev, state, "power domain ");
		error = pm_op(dev, &dev->pm_domain->ops, state);
		goto End;
	}

	if (dev->type && dev->type->pm) {
		pm_dev_dbg(dev, state, "type ");
		error = pm_op(dev, dev->type->pm, state);
		goto End;
	}

	if (dev->class) {
		if (dev->class->pm) {
			pm_dev_dbg(dev, state, "class ");
			error = pm_op(dev, dev->class->pm, state);
			goto End;
		} else if (dev->class->resume) {
			pm_dev_dbg(dev, state, "legacy class ");
			error = legacy_resume(dev, dev->class->resume);
			goto End;
		}
	}

	if (dev->bus) {
		if (dev->bus->pm) {
			pm_dev_dbg(dev, state, "");
			error = pm_op(dev, dev->bus->pm, state);
		} else if (dev->bus->resume) {
			pm_dev_dbg(dev, state, "legacy ");
			error = legacy_resume(dev, dev->bus->resume);
		}
	}

 End:
	dev->power.is_suspended = false;

 Unlock:
	device_unlock(dev);
	complete_all(&dev->power.completion);

	TRACE_RESUME(error);

	if (put)
		pm_runtime_put_sync(dev);

	return error;
}

static void async_resume(void *data, async_cookie_t cookie)
{
	struct device *dev = (struct device *)data;
	int error;

	error = device_resume(dev, pm_transition, true);
	if (error)
		pm_dev_err(dev, pm_transition, " async", error);
	put_device(dev);
}
#ifdef CONFIG_ARCH_GEN3
int resume_device(struct device *dev, pm_message_t state)
{
        return device_resume(dev,state,false);
}
#endif

static bool is_async(struct device *dev)
{
	return dev->power.async_suspend && pm_async_enabled
		&& !pm_trace_is_enabled();
}

void dpm_resume(pm_message_t state)
{
	struct device *dev;
	ktime_t starttime = ktime_get();

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	pm_transition = state;
	async_error = 0;

	list_for_each_entry(dev, &dpm_suspended_list, power.entry) {
		INIT_COMPLETION(dev->power.completion);
		if (is_async(dev)) {
			get_device(dev);
			async_schedule(async_resume, dev);
		}
	}

	while (!list_empty(&dpm_suspended_list)) {
		dev = to_device(dpm_suspended_list.next);
		get_device(dev);
		if (!is_async(dev)) {
			int error;

			mutex_unlock(&dpm_list_mtx);

			error = device_resume(dev, state, false);
			if (error) {
				suspend_stats.failed_resume++;
				dpm_save_failed_step(SUSPEND_RESUME);
				dpm_save_failed_dev(dev_name(dev));
				pm_dev_err(dev, state, "", error);
			}

			mutex_lock(&dpm_list_mtx);
		}
		if (!list_empty(&dev->power.entry))
			list_move_tail(&dev->power.entry, &dpm_prepared_list);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	async_synchronize_full();
	dpm_show_time(starttime, state, NULL);
}

static void device_complete(struct device *dev, pm_message_t state)
{
	device_lock(dev);

	if (dev->pm_domain) {
		pm_dev_dbg(dev, state, "completing power domain ");
		if (dev->pm_domain->ops.complete)
			dev->pm_domain->ops.complete(dev);
	} else if (dev->type && dev->type->pm) {
		pm_dev_dbg(dev, state, "completing type ");
		if (dev->type->pm->complete)
			dev->type->pm->complete(dev);
	} else if (dev->class && dev->class->pm) {
		pm_dev_dbg(dev, state, "completing class ");
		if (dev->class->pm->complete)
			dev->class->pm->complete(dev);
	} else if (dev->bus && dev->bus->pm) {
		pm_dev_dbg(dev, state, "completing ");
		if (dev->bus->pm->complete)
			dev->bus->pm->complete(dev);
	}

	device_unlock(dev);
}

void dpm_complete(pm_message_t state)
{
	struct list_head list;

	might_sleep();

	INIT_LIST_HEAD(&list);
	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_prepared_list)) {
		struct device *dev = to_device(dpm_prepared_list.prev);

		get_device(dev);
		dev->power.is_prepared = false;
		list_move(&dev->power.entry, &list);
		mutex_unlock(&dpm_list_mtx);

		device_complete(dev, state);

		mutex_lock(&dpm_list_mtx);
		put_device(dev);
	}
	list_splice(&list, &dpm_list);
	mutex_unlock(&dpm_list_mtx);
}

void dpm_resume_end(pm_message_t state)
{
	dpm_resume(state);
	dpm_complete(state);
}
EXPORT_SYMBOL_GPL(dpm_resume_end);

static pm_message_t resume_event(pm_message_t sleep_state)
{
	switch (sleep_state.event) {
	case PM_EVENT_SUSPEND:
		return PMSG_RESUME;
	case PM_EVENT_FREEZE:
	case PM_EVENT_QUIESCE:
		return PMSG_RECOVER;
	case PM_EVENT_HIBERNATE:
		return PMSG_RESTORE;
	}
	return PMSG_ON;
}

static int device_suspend_noirq(struct device *dev, pm_message_t state)
{
	int error;

	if (dev->pm_domain) {
		pm_dev_dbg(dev, state, "LATE power domain ");
		error = pm_noirq_op(dev, &dev->pm_domain->ops, state);
		if (error)
			return error;
	} else if (dev->type && dev->type->pm) {
		pm_dev_dbg(dev, state, "LATE type ");
		error = pm_noirq_op(dev, dev->type->pm, state);
		if (error)
			return error;
	} else if (dev->class && dev->class->pm) {
		pm_dev_dbg(dev, state, "LATE class ");
		error = pm_noirq_op(dev, dev->class->pm, state);
		if (error)
			return error;
	} else if (dev->bus && dev->bus->pm) {
		pm_dev_dbg(dev, state, "LATE ");
		error = pm_noirq_op(dev, dev->bus->pm, state);
		if (error)
			return error;
	}

	return 0;
}

int dpm_suspend_noirq(pm_message_t state)
{
	ktime_t starttime = ktime_get();
	int error = 0;

	suspend_device_irqs();
	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_suspended_list)) {
		struct device *dev = to_device(dpm_suspended_list.prev);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);

		error = device_suspend_noirq(dev, state);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			pm_dev_err(dev, state, " late", error);
			suspend_stats.failed_suspend_noirq++;
			dpm_save_failed_step(SUSPEND_SUSPEND_NOIRQ);
			dpm_save_failed_dev(dev_name(dev));
			put_device(dev);
			break;
		}
		if (!list_empty(&dev->power.entry))
			list_move(&dev->power.entry, &dpm_noirq_list);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	if (error)
		dpm_resume_noirq(resume_event(state));
	else
		dpm_show_time(starttime, state, "late");
	return error;
}
EXPORT_SYMBOL_GPL(dpm_suspend_noirq);

static int legacy_suspend(struct device *dev, pm_message_t state,
			  int (*cb)(struct device *dev, pm_message_t state))
{
	int error;
	ktime_t calltime;

	calltime = initcall_debug_start(dev);

	error = cb(dev, state);
	suspend_report_result(cb, error);

	initcall_debug_report(dev, calltime, error);

	return error;
}

static int __device_suspend(struct device *dev, pm_message_t state, bool async)
{
	int error = 0;

	dpm_wait_for_children(dev, async);

	if (async_error)
		goto Complete;

	pm_runtime_get_noresume(dev);
	if (pm_runtime_barrier(dev) && device_may_wakeup(dev))
		pm_wakeup_event(dev, 0);

	if (pm_wakeup_pending()) {
		pm_runtime_put_sync(dev);
		async_error = -EBUSY;
		goto Complete;
	}

	device_lock(dev);

	if (dev->pm_domain) {
		pm_dev_dbg(dev, state, "power domain ");
		error = pm_op(dev, &dev->pm_domain->ops, state);
		goto End;
	}

	if (dev->type && dev->type->pm) {
		pm_dev_dbg(dev, state, "type ");
		error = pm_op(dev, dev->type->pm, state);
		goto End;
	}

	if (dev->class) {
		if (dev->class->pm) {
			pm_dev_dbg(dev, state, "class ");
			error = pm_op(dev, dev->class->pm, state);
			goto End;
		} else if (dev->class->suspend) {
			pm_dev_dbg(dev, state, "legacy class ");
			error = legacy_suspend(dev, state, dev->class->suspend);
			goto End;
		}
	}

	if (dev->bus) {
		if (dev->bus->pm) {
			pm_dev_dbg(dev, state, "");
			error = pm_op(dev, dev->bus->pm, state);
		} else if (dev->bus->suspend) {
			pm_dev_dbg(dev, state, "legacy ");
			error = legacy_suspend(dev, state, dev->bus->suspend);
		}
	}

 End:
	if (!error) {
		dev->power.is_suspended = true;
		if (dev->power.wakeup_path
		    && dev->parent && !dev->parent->power.ignore_children)
			dev->parent->power.wakeup_path = true;
	}

	device_unlock(dev);

 Complete:
	complete_all(&dev->power.completion);

	if (error) {
		pm_runtime_put_sync(dev);
		async_error = error;
	} else if (dev->power.is_suspended) {
		__pm_runtime_disable(dev, false);
	}

	return error;
}

static void async_suspend(void *data, async_cookie_t cookie)
{
	struct device *dev = (struct device *)data;
	int error;

	error = __device_suspend(dev, pm_transition, true);
	if (error) {
		dpm_save_failed_dev(dev_name(dev));
		pm_dev_err(dev, pm_transition, " async", error);
	}

	put_device(dev);
}

static int device_suspend(struct device *dev)
{
	INIT_COMPLETION(dev->power.completion);

	if (pm_async_enabled && dev->power.async_suspend) {
		get_device(dev);
		async_schedule(async_suspend, dev);
		return 0;
	}

	return __device_suspend(dev, pm_transition, false);
}

#ifdef CONFIG_ARCH_GEN3
int suspend_device(struct device *dev, pm_message_t state)
{
       return  __device_suspend(dev, state, false);
}	
#endif
 
int dpm_suspend(pm_message_t state)
{
	ktime_t starttime = ktime_get();
	int error = 0;

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	pm_transition = state;
	async_error = 0;
	while (!list_empty(&dpm_prepared_list)) {
		struct device *dev = to_device(dpm_prepared_list.prev);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);

		error = device_suspend(dev);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			pm_dev_err(dev, state, "", error);
			dpm_save_failed_dev(dev_name(dev));
			put_device(dev);
			break;
		}
		if (!list_empty(&dev->power.entry))
			list_move(&dev->power.entry, &dpm_suspended_list);
		put_device(dev);
		if (async_error)
			break;
	}
	mutex_unlock(&dpm_list_mtx);
	async_synchronize_full();
	if (!error)
		error = async_error;
	if (error) {
		suspend_stats.failed_suspend++;
		dpm_save_failed_step(SUSPEND_SUSPEND);
	} else
		dpm_show_time(starttime, state, NULL);
	return error;
}

static int device_prepare(struct device *dev, pm_message_t state)
{
	int error = 0;

	device_lock(dev);

	dev->power.wakeup_path = device_may_wakeup(dev);

	if (dev->pm_domain) {
		pm_dev_dbg(dev, state, "preparing power domain ");
		if (dev->pm_domain->ops.prepare)
			error = dev->pm_domain->ops.prepare(dev);
		suspend_report_result(dev->pm_domain->ops.prepare, error);
		if (error)
			goto End;
	} else if (dev->type && dev->type->pm) {
		pm_dev_dbg(dev, state, "preparing type ");
		if (dev->type->pm->prepare)
			error = dev->type->pm->prepare(dev);
		suspend_report_result(dev->type->pm->prepare, error);
		if (error)
			goto End;
	} else if (dev->class && dev->class->pm) {
		pm_dev_dbg(dev, state, "preparing class ");
		if (dev->class->pm->prepare)
			error = dev->class->pm->prepare(dev);
		suspend_report_result(dev->class->pm->prepare, error);
		if (error)
			goto End;
	} else if (dev->bus && dev->bus->pm) {
		pm_dev_dbg(dev, state, "preparing ");
		if (dev->bus->pm->prepare)
			error = dev->bus->pm->prepare(dev);
		suspend_report_result(dev->bus->pm->prepare, error);
	}

 End:
	device_unlock(dev);

	return error;
}

int dpm_prepare(pm_message_t state)
{
	int error = 0;

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	while (!list_empty(&dpm_list)) {
		struct device *dev = to_device(dpm_list.next);

		get_device(dev);
		mutex_unlock(&dpm_list_mtx);

		error = device_prepare(dev, state);

		mutex_lock(&dpm_list_mtx);
		if (error) {
			if (error == -EAGAIN) {
				put_device(dev);
				error = 0;
				continue;
			}
			printk(KERN_INFO "PM: Device %s not prepared "
				"for power transition: code %d\n",
				dev_name(dev), error);
			put_device(dev);
			break;
		}
		dev->power.is_prepared = true;
		if (!list_empty(&dev->power.entry))
			list_move_tail(&dev->power.entry, &dpm_prepared_list);
		put_device(dev);
	}
	mutex_unlock(&dpm_list_mtx);
	return error;
}

int dpm_suspend_start(pm_message_t state)
{
	int error;

	error = dpm_prepare(state);
	if (error) {
		suspend_stats.failed_prepare++;
		dpm_save_failed_step(SUSPEND_PREPARE);
	} else
		error = dpm_suspend(state);
	return error;
}
EXPORT_SYMBOL_GPL(dpm_suspend_start);

void __suspend_report_result(const char *function, void *fn, int ret)
{
	if (ret)
		printk(KERN_ERR "%s(): %pF returns %d\n", function, fn, ret);
}
EXPORT_SYMBOL_GPL(__suspend_report_result);

int device_pm_wait_for_dev(struct device *subordinate, struct device *dev)
{
	dpm_wait(dev, subordinate->power.async_suspend);
	return async_error;
}
EXPORT_SYMBOL_GPL(device_pm_wait_for_dev);

#if defined(CONFIG_SYNO_COMCERTO)

 
#ifdef CONFIG_PM_SYSFS_MANUAL

static DEFINE_MUTEX(dpm_lock);

void dpm_manual_resume(struct device *dev,pm_message_t state)
{
	int error;
	struct list_head list;
	ktime_t starttime = ktime_get();

	might_sleep();

	mutex_lock(&dpm_list_mtx);
        pm_transition = state;
	INIT_COMPLETION(dev->power.completion);
	mutex_unlock(&dpm_list_mtx);

	error = device_resume(dev, state, false);
	if (error) {
		suspend_stats.failed_resume++;
		dpm_save_failed_step(SUSPEND_RESUME);
		dpm_save_failed_dev(dev_name(dev));
		pm_dev_err(dev, state, "", error);
	}

	mutex_lock(&dpm_list_mtx);
	if (!list_empty(&dev->power.entry))
		list_move_tail(&dev->power.entry, &dpm_prepared_list);
	mutex_unlock(&dpm_list_mtx);
	
	INIT_LIST_HEAD(&list);
	mutex_lock(&dpm_list_mtx);
	dev->power.is_prepared = false;
	list_move(&dev->power.entry, &list);
	mutex_unlock(&dpm_list_mtx);

	device_complete(dev, state);
	dev->power.power_state=state;
	dpm_show_time(starttime, state, NULL);
}

void dpm_manual_resume_start(struct device * dev,pm_message_t state)
{
	mutex_lock(&dpm_lock);
	if (dev->power.power_state.event == state.event){
		printk(KERN_ERR "PM: We are already in the resume state \n");
		goto done;
        }
	 
	dpm_manual_resume(dev,state);
done:
	mutex_unlock(&dpm_lock);

}

static int dpm_manual_prepare(struct device * dev , pm_message_t state)
{
	 
	int error = 0;
	might_sleep();
	
	error = device_prepare(dev, state);

	mutex_lock(&dpm_list_mtx);
	if (error){
		printk(KERN_INFO "PM: Device %s not prepared " "for power transition: code %d\n",
			dev_name(dev), error);
		goto done;
	}
	dev->power.is_prepared = true;
	if (!list_empty(&dev->power.entry))
		list_move_tail(&dev->power.entry, &dpm_prepared_list);

done:
	mutex_unlock(&dpm_list_mtx);	
	return error;
}

static int dpm_manual_suspend(struct device * dev, pm_message_t state)
{
	ktime_t starttime;
	int error=0;

	might_sleep();

	mutex_lock(&dpm_list_mtx);
	pm_transition = state;
	mutex_unlock(&dpm_list_mtx);
	
	error = device_suspend(dev);
	
	mutex_lock(&dpm_list_mtx);
	if (error){
		pm_dev_err(dev, state, "", error);
                dpm_save_failed_dev(dev_name(dev));
	}	
	if (!list_empty(&dev->power.entry))
		list_move(&dev->power.entry, &dpm_suspended_list);
	mutex_unlock(&dpm_list_mtx);

	dev->power.power_state=state;
	dpm_show_time(starttime, state, NULL);
	return error;
}

int dpm_manual_suspend_start(struct device * dev, pm_message_t state)
{
	int error=0;

	mutex_lock(&dpm_lock);

	if (dev->power.power_state.event == state.event){
		if ( state.event == PM_EVENT_SUSPEND )
			printk(KERN_ERR "PM: We are already in the suspend (power off L1) state \n");
#if 0
		else if ( state.event == PM_EVENT_SUSPEND_L2)
			printk(KERN_ERR "PM: We are already in the suspend (Power off L2) state \n");
#endif
		goto done;
        }

	error=dpm_manual_prepare(dev,state);
	
	if (error){
		suspend_stats.failed_prepare++;
		dpm_save_failed_step(SUSPEND_PREPARE);
		goto done;
	}else
		error = dpm_manual_suspend(dev,state);	
done:
	mutex_unlock(&dpm_lock);
	return error;
}
#endif
#endif
