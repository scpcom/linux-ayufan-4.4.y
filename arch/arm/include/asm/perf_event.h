#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ARM_PERF_EVENT_H__
#define __ARM_PERF_EVENT_H__

#define PERF_EVENT_INDEX_OFFSET 1

enum arm_perf_pmu_ids {
	ARM_PERF_PMU_ID_XSCALE1	= 0,
	ARM_PERF_PMU_ID_XSCALE2,
	ARM_PERF_PMU_ID_V6,
	ARM_PERF_PMU_ID_V6MP,
	ARM_PERF_PMU_ID_CA8,
	ARM_PERF_PMU_ID_CA9,
	ARM_PERF_PMU_ID_CA5,
	ARM_PERF_PMU_ID_CA15,
	ARM_NUM_PMU_IDS,
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	MRVL_PERF_PMU_ID_PJ4B,
#endif
};

extern enum arm_perf_pmu_ids
armpmu_get_pmu_id(void);

extern int
armpmu_get_max_events(void);

#endif  
