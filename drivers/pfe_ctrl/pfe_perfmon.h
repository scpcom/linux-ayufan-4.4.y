#ifndef _PFE_PERFMON_H_
#define _PFE_PERFMON_H_

#define	CT_CPUMON_INTERVAL	(1 * TIMER_TICKS_PER_SEC)

struct pfe_cpumon {
	u32 cpu_usage_pct[MAX_PE];
	u32 class_usage_pct;
};

struct pfe_memmon {
	u32 kernel_memory_allocated;
};

void * pfe_kmalloc(size_t size, int flags);
void * pfe_kzalloc(size_t size, int flags);
void pfe_kfree(void *ptr);

int pfe_perfmon_init(struct pfe *pfe);
void pfe_perfmon_exit(struct pfe *pfe);

#endif /* _PFE_PERFMON_H_ */
