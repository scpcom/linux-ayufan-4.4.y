#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ARM_PMU_H__
#define __ARM_PMU_H__

#include <linux/interrupt.h>
#include <linux/perf_event.h>

#if defined(CONFIG_SYNO_LSP_HI3536)
 
enum arm_pmu_type {
	ARM_PMU_DEVICE_CPU	= 0,
	ARM_NUM_PMU_DEVICES,
};
#endif  

struct arm_pmu_platdata {
	irqreturn_t (*handle_irq)(int irq, void *dev,
				  irq_handler_t pmu_handler);
	int (*runtime_resume)(struct device *dev);
	int (*runtime_suspend)(struct device *dev);
};

#ifdef CONFIG_HW_PERF_EVENTS

struct pmu_hw_events {
	 
	struct perf_event	**events;

	unsigned long           *used_mask;

	raw_spinlock_t		pmu_lock;
#if defined (MY_DEF_HERE)

	struct pmu		*pmu;
#endif  
};

struct arm_pmu {
	struct pmu	pmu;
	cpumask_t	active_irqs;
	char		*name;
	irqreturn_t	(*handle_irq)(int irq_num, void *dev);
	void		(*enable)(struct perf_event *event);
	void		(*disable)(struct perf_event *event);
	int		(*get_event_idx)(struct pmu_hw_events *hw_events,
					 struct perf_event *event);
	int		(*set_event_filter)(struct hw_perf_event *evt,
					    struct perf_event_attr *attr);
	u32		(*read_counter)(struct perf_event *event);
	void		(*write_counter)(struct perf_event *event, u32 val);
	void		(*start)(struct arm_pmu *);
	void		(*stop)(struct arm_pmu *);
	void		(*reset)(void *);
	int		(*request_irq)(struct arm_pmu *, irq_handler_t handler);
	void		(*free_irq)(struct arm_pmu *);
	int		(*map_event)(struct perf_event *event);
	int		num_events;
	atomic_t	active_events;
	struct mutex	reserve_mutex;
	u64		max_period;
	struct platform_device	*plat_device;
	struct pmu_hw_events	*(*get_hw_events)(void);
};

#define to_arm_pmu(p) (container_of(p, struct arm_pmu, pmu))

extern const struct dev_pm_ops armpmu_dev_pm_ops;

int armpmu_register(struct arm_pmu *armpmu, int type);

u64 armpmu_event_update(struct perf_event *event);

int armpmu_event_set_period(struct perf_event *event);

int armpmu_map_event(struct perf_event *event,
		     const unsigned (*event_map)[PERF_COUNT_HW_MAX],
		     const unsigned (*cache_map)[PERF_COUNT_HW_CACHE_MAX]
						[PERF_COUNT_HW_CACHE_OP_MAX]
						[PERF_COUNT_HW_CACHE_RESULT_MAX],
		     u32 raw_event_mask);

#endif  

#endif  
