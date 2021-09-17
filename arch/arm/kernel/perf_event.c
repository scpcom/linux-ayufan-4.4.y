#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#undef DEBUG

#define pr_fmt(fmt) "hw perfevents: " fmt

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>
#if defined(MY_ABC_HERE)
#include <linux/irq.h>
#include <linux/irqdesc.h>
#endif  

#include <asm/irq_regs.h>
#include <asm/pmu.h>
#include <asm/stacktrace.h>

static int
armpmu_map_cache_event(const unsigned (*cache_map)
				      [PERF_COUNT_HW_CACHE_MAX]
				      [PERF_COUNT_HW_CACHE_OP_MAX]
				      [PERF_COUNT_HW_CACHE_RESULT_MAX],
		       u64 config)
{
	unsigned int cache_type, cache_op, cache_result, ret;

	cache_type = (config >>  0) & 0xff;
	if (cache_type >= PERF_COUNT_HW_CACHE_MAX)
		return -EINVAL;

	cache_op = (config >>  8) & 0xff;
	if (cache_op >= PERF_COUNT_HW_CACHE_OP_MAX)
		return -EINVAL;

	cache_result = (config >> 16) & 0xff;
	if (cache_result >= PERF_COUNT_HW_CACHE_RESULT_MAX)
		return -EINVAL;

	ret = (int)(*cache_map)[cache_type][cache_op][cache_result];

	if (ret == CACHE_OP_UNSUPPORTED)
		return -ENOENT;

	return ret;
}

static int
armpmu_map_hw_event(const unsigned (*event_map)[PERF_COUNT_HW_MAX], u64 config)
{
	int mapping;

	if (config >= PERF_COUNT_HW_MAX)
		return -ENOENT;

	mapping = (*event_map)[config];
	return mapping == HW_OP_UNSUPPORTED ? -ENOENT : mapping;
}

static int
armpmu_map_raw_event(u32 raw_event_mask, u64 config)
{
	return (int)(config & raw_event_mask);
}

int
armpmu_map_event(struct perf_event *event,
		 const unsigned (*event_map)[PERF_COUNT_HW_MAX],
		 const unsigned (*cache_map)
				[PERF_COUNT_HW_CACHE_MAX]
				[PERF_COUNT_HW_CACHE_OP_MAX]
				[PERF_COUNT_HW_CACHE_RESULT_MAX],
		 u32 raw_event_mask)
{
	u64 config = event->attr.config;

	switch (event->attr.type) {
	case PERF_TYPE_HARDWARE:
		return armpmu_map_hw_event(event_map, config);
	case PERF_TYPE_HW_CACHE:
		return armpmu_map_cache_event(cache_map, config);
	case PERF_TYPE_RAW:
		return armpmu_map_raw_event(raw_event_mask, config);
	}

	return -ENOENT;
}

int armpmu_event_set_period(struct perf_event *event)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	s64 left = local64_read(&hwc->period_left);
	s64 period = hwc->sample_period;
	int ret = 0;

	if (unlikely(period != hwc->last_period))
		left = period - (hwc->last_period - left);

	if (unlikely(left <= -period)) {
		left = period;
		local64_set(&hwc->period_left, left);
		hwc->last_period = period;
		ret = 1;
	}

	if (unlikely(left <= 0)) {
		left += period;
		local64_set(&hwc->period_left, left);
		hwc->last_period = period;
		ret = 1;
	}

	if (left > (s64)armpmu->max_period)
		left = armpmu->max_period;

	local64_set(&hwc->prev_count, (u64)-left);

	armpmu->write_counter(event, (u64)(-left) & 0xffffffff);

	perf_event_update_userpage(event);

	return ret;
}

u64 armpmu_event_update(struct perf_event *event)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u64 delta, prev_raw_count, new_raw_count;

again:
	prev_raw_count = local64_read(&hwc->prev_count);
	new_raw_count = armpmu->read_counter(event);

	if (local64_cmpxchg(&hwc->prev_count, prev_raw_count,
			     new_raw_count) != prev_raw_count)
		goto again;

	delta = (new_raw_count - prev_raw_count) & armpmu->max_period;

	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

static void
armpmu_read(struct perf_event *event)
{
	armpmu_event_update(event);
}

static void
armpmu_stop(struct perf_event *event, int flags)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	if (!(hwc->state & PERF_HES_STOPPED)) {
		armpmu->disable(event);
		armpmu_event_update(event);
		hwc->state |= PERF_HES_STOPPED | PERF_HES_UPTODATE;
	}
}

static void armpmu_start(struct perf_event *event, int flags)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	if (flags & PERF_EF_RELOAD)
		WARN_ON_ONCE(!(hwc->state & PERF_HES_UPTODATE));

	hwc->state = 0;
	 
	armpmu_event_set_period(event);
	armpmu->enable(event);
}

static void
armpmu_del(struct perf_event *event, int flags)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct pmu_hw_events *hw_events = armpmu->get_hw_events();
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	armpmu_stop(event, PERF_EF_UPDATE);
	hw_events->events[idx] = NULL;
	clear_bit(idx, hw_events->used_mask);

	perf_event_update_userpage(event);
}

static int
armpmu_add(struct perf_event *event, int flags)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct pmu_hw_events *hw_events = armpmu->get_hw_events();
	struct hw_perf_event *hwc = &event->hw;
	int idx;
	int err = 0;

	perf_pmu_disable(event->pmu);

	idx = armpmu->get_event_idx(hw_events, event);
	if (idx < 0) {
		err = idx;
		goto out;
	}

	event->hw.idx = idx;
	armpmu->disable(event);
	hw_events->events[idx] = event;

	hwc->state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
	if (flags & PERF_EF_START)
		armpmu_start(event, PERF_EF_RELOAD);

	perf_event_update_userpage(event);

out:
	perf_pmu_enable(event->pmu);
	return err;
}

static int
validate_event(struct pmu_hw_events *hw_events,
	       struct perf_event *event)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
#if defined(CONFIG_SYNO_LSP_HI3536)
	 
#else  
	struct pmu *leader_pmu = event->group_leader->pmu;
#endif  

	if (is_software_event(event))
		return 1;

#if defined(CONFIG_SYNO_LSP_HI3536)
	if (event->state < PERF_EVENT_STATE_OFF)
#else  
	if (event->pmu != leader_pmu || event->state < PERF_EVENT_STATE_OFF)
#endif  
		return 1;

	if (event->state == PERF_EVENT_STATE_OFF && !event->attr.enable_on_exec)
		return 1;

	return armpmu->get_event_idx(hw_events, event) >= 0;
}

static int
validate_group(struct perf_event *event)
{
	struct perf_event *sibling, *leader = event->group_leader;
	struct pmu_hw_events fake_pmu;
	DECLARE_BITMAP(fake_used_mask, ARMPMU_MAX_HWEVENTS);

	memset(fake_used_mask, 0, sizeof(fake_used_mask));
	fake_pmu.used_mask = fake_used_mask;

	if (!validate_event(&fake_pmu, leader))
		return -EINVAL;

	list_for_each_entry(sibling, &leader->sibling_list, group_entry) {
		if (!validate_event(&fake_pmu, sibling))
			return -EINVAL;
	}

	if (!validate_event(&fake_pmu, event))
		return -EINVAL;

	return 0;
}

#if defined(MY_DEF_HERE)
static irqreturn_t armpmu_dispatch_irq(int irq, void *dev)
{
    struct pmu_hw_events *cpuc  = (struct pmu_hw_events *)dev;
    struct arm_pmu *armpmu = to_arm_pmu(cpuc->pmu);

    struct platform_device *plat_device = armpmu->plat_device;
    struct arm_pmu_platdata *plat = dev_get_platdata(&plat_device->dev);
    int ret;

    if (plat && plat->handle_irq)
        ret = plat->handle_irq(irq, armpmu, armpmu->handle_irq);
    else
        ret = armpmu->handle_irq(irq, armpmu);

    return ret;
}
#elif defined(MY_ABC_HERE)
static irqreturn_t armpmu_dispatch_irq(int irq, void *dev)
{
	int ret;
	u64 start_clock, finish_clock;
	struct arm_pmu *armpmu;
	struct platform_device *plat_device;
	struct arm_pmu_platdata *plat;

	if (irq_is_percpu(irq))
		dev = *(void **)dev;
	armpmu = dev;
	plat_device = armpmu->plat_device;
	plat = dev_get_platdata(&plat_device->dev);

	start_clock = sched_clock();
	if (plat && plat->handle_irq)
		ret = plat->handle_irq(irq, dev, armpmu->handle_irq);
	else
		ret = armpmu->handle_irq(irq, dev);
	finish_clock = sched_clock();

	perf_sample_event_took(finish_clock - start_clock);
	return ret;
}
#else
static irqreturn_t armpmu_dispatch_irq(int irq, void *dev)
{
	struct arm_pmu *armpmu = (struct arm_pmu *) dev;
	struct platform_device *plat_device = armpmu->plat_device;
	struct arm_pmu_platdata *plat = dev_get_platdata(&plat_device->dev);
	int ret;
	u64 start_clock, finish_clock;

	start_clock = sched_clock();
	if (plat && plat->handle_irq)
		ret = plat->handle_irq(irq, dev, armpmu->handle_irq);
	else
		ret = armpmu->handle_irq(irq, dev);
	finish_clock = sched_clock();

	perf_sample_event_took(finish_clock - start_clock);
	return ret;
}
#endif

static void
armpmu_release_hardware(struct arm_pmu *armpmu)
{
	armpmu->free_irq(armpmu);
	pm_runtime_put_sync(&armpmu->plat_device->dev);
}

static int
armpmu_reserve_hardware(struct arm_pmu *armpmu)
{
	int err;
	struct platform_device *pmu_device = armpmu->plat_device;

	if (!pmu_device)
		return -ENODEV;

	pm_runtime_get_sync(&pmu_device->dev);
	err = armpmu->request_irq(armpmu, armpmu_dispatch_irq);
	if (err) {
		armpmu_release_hardware(armpmu);
		return err;
	}

	return 0;
}

static void
hw_perf_event_destroy(struct perf_event *event)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	atomic_t *active_events	 = &armpmu->active_events;
	struct mutex *pmu_reserve_mutex = &armpmu->reserve_mutex;

	if (atomic_dec_and_mutex_lock(active_events, pmu_reserve_mutex)) {
		armpmu_release_hardware(armpmu);
		mutex_unlock(pmu_reserve_mutex);
	}
}

static int
event_requires_mode_exclusion(struct perf_event_attr *attr)
{
	return attr->exclude_idle || attr->exclude_user ||
	       attr->exclude_kernel || attr->exclude_hv;
}

static int
__hw_perf_event_init(struct perf_event *event)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int mapping;

	mapping = armpmu->map_event(event);

	if (mapping < 0) {
		pr_debug("event %x:%llx not supported\n", event->attr.type,
			 event->attr.config);
		return mapping;
	}

	hwc->idx		= -1;
	hwc->config_base	= 0;
	hwc->config		= 0;
	hwc->event_base		= 0;

	if ((!armpmu->set_event_filter ||
	     armpmu->set_event_filter(hwc, &event->attr)) &&
	     event_requires_mode_exclusion(&event->attr)) {
		pr_debug("ARM performance counters do not support "
			 "mode exclusion\n");
		return -EOPNOTSUPP;
	}

	hwc->config_base	    |= (unsigned long)mapping;

	if (!hwc->sample_period) {
		 
		hwc->sample_period  = armpmu->max_period >> 1;
		hwc->last_period    = hwc->sample_period;
		local64_set(&hwc->period_left, hwc->sample_period);
	}

	if (event->group_leader != event) {
		if (validate_group(event) != 0)
			return -EINVAL;
	}

	return 0;
}

static int armpmu_event_init(struct perf_event *event)
{
	struct arm_pmu *armpmu = to_arm_pmu(event->pmu);
	int err = 0;
	atomic_t *active_events = &armpmu->active_events;

	if (has_branch_stack(event))
		return -EOPNOTSUPP;

	if (armpmu->map_event(event) == -ENOENT)
		return -ENOENT;

	event->destroy = hw_perf_event_destroy;

	if (!atomic_inc_not_zero(active_events)) {
		mutex_lock(&armpmu->reserve_mutex);
		if (atomic_read(active_events) == 0)
			err = armpmu_reserve_hardware(armpmu);

		if (!err)
			atomic_inc(active_events);
		mutex_unlock(&armpmu->reserve_mutex);
	}

	if (err)
		return err;

	err = __hw_perf_event_init(event);
	if (err)
		hw_perf_event_destroy(event);

	return err;
}

static void armpmu_enable(struct pmu *pmu)
{
	struct arm_pmu *armpmu = to_arm_pmu(pmu);
	struct pmu_hw_events *hw_events = armpmu->get_hw_events();
	int enabled = bitmap_weight(hw_events->used_mask, armpmu->num_events);

	if (enabled)
		armpmu->start(armpmu);
}

static void armpmu_disable(struct pmu *pmu)
{
	struct arm_pmu *armpmu = to_arm_pmu(pmu);
	armpmu->stop(armpmu);
}

#ifdef CONFIG_PM_RUNTIME
static int armpmu_runtime_resume(struct device *dev)
{
	struct arm_pmu_platdata *plat = dev_get_platdata(dev);

	if (plat && plat->runtime_resume)
		return plat->runtime_resume(dev);

	return 0;
}

static int armpmu_runtime_suspend(struct device *dev)
{
	struct arm_pmu_platdata *plat = dev_get_platdata(dev);

	if (plat && plat->runtime_suspend)
		return plat->runtime_suspend(dev);

	return 0;
}
#endif

const struct dev_pm_ops armpmu_dev_pm_ops = {
	SET_RUNTIME_PM_OPS(armpmu_runtime_suspend, armpmu_runtime_resume, NULL)
};

static void armpmu_init(struct arm_pmu *armpmu)
{
	atomic_set(&armpmu->active_events, 0);
	mutex_init(&armpmu->reserve_mutex);

	armpmu->pmu = (struct pmu) {
		.pmu_enable	= armpmu_enable,
		.pmu_disable	= armpmu_disable,
		.event_init	= armpmu_event_init,
		.add		= armpmu_add,
		.del		= armpmu_del,
		.start		= armpmu_start,
		.stop		= armpmu_stop,
		.read		= armpmu_read,
	};
}

int armpmu_register(struct arm_pmu *armpmu, int type)
{
	armpmu_init(armpmu);
	pm_runtime_enable(&armpmu->plat_device->dev);
	pr_info("enabled with %s PMU driver, %d counters available\n",
			armpmu->name, armpmu->num_events);
	return perf_pmu_register(&armpmu->pmu, armpmu->name, type);
}

struct frame_tail {
	struct frame_tail __user *fp;
	unsigned long sp;
	unsigned long lr;
} __attribute__((packed));

static struct frame_tail __user *
user_backtrace(struct frame_tail __user *tail,
	       struct perf_callchain_entry *entry)
{
	struct frame_tail buftail;

	if (!access_ok(VERIFY_READ, tail, sizeof(buftail)))
		return NULL;
	if (__copy_from_user_inatomic(&buftail, tail, sizeof(buftail)))
		return NULL;

	perf_callchain_store(entry, buftail.lr);

	if (tail + 1 >= buftail.fp)
		return NULL;

	return buftail.fp - 1;
}

void
perf_callchain_user(struct perf_callchain_entry *entry, struct pt_regs *regs)
{
	struct frame_tail __user *tail;

	if (perf_guest_cbs && perf_guest_cbs->is_in_guest()) {
		 
		return;
	}

	perf_callchain_store(entry, regs->ARM_pc);
	tail = (struct frame_tail __user *)regs->ARM_fp - 1;

	while ((entry->nr < PERF_MAX_STACK_DEPTH) &&
	       tail && !((unsigned long)tail & 0x3))
		tail = user_backtrace(tail, entry);
}

static int
callchain_trace(struct stackframe *fr,
		void *data)
{
	struct perf_callchain_entry *entry = data;
	perf_callchain_store(entry, fr->pc);
	return 0;
}

void
perf_callchain_kernel(struct perf_callchain_entry *entry, struct pt_regs *regs)
{
	struct stackframe fr;

	if (perf_guest_cbs && perf_guest_cbs->is_in_guest()) {
		 
		return;
	}

	fr.fp = regs->ARM_fp;
	fr.sp = regs->ARM_sp;
	fr.lr = regs->ARM_lr;
	fr.pc = regs->ARM_pc;
	walk_stackframe(&fr, callchain_trace, entry);
}

unsigned long perf_instruction_pointer(struct pt_regs *regs)
{
	if (perf_guest_cbs && perf_guest_cbs->is_in_guest())
		return perf_guest_cbs->get_guest_ip();

	return instruction_pointer(regs);
}

unsigned long perf_misc_flags(struct pt_regs *regs)
{
	int misc = 0;

	if (perf_guest_cbs && perf_guest_cbs->is_in_guest()) {
		if (perf_guest_cbs->is_user_mode())
			misc |= PERF_RECORD_MISC_GUEST_USER;
		else
			misc |= PERF_RECORD_MISC_GUEST_KERNEL;
	} else {
		if (user_mode(regs))
			misc |= PERF_RECORD_MISC_USER;
		else
			misc |= PERF_RECORD_MISC_KERNEL;
	}

	return misc;
}
