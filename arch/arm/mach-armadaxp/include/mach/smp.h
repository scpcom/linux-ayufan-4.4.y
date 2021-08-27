#ifndef ASMARM_ARCH_SMP_H
#define ASMARM_ARCH_SMP_H

#include <asm/io.h>
#include <mach/armadaxp.h>

extern unsigned int master_cpu_id;
extern unsigned int group_cpu_mask;
extern unsigned long mv_cpu_count;

#define hard_smp_processor_id()			\
	({						\
		unsigned int cpunum;			\
		__asm__("mrc p15, 0, %0, c0, c0, 5"	\
			: "=r" (cpunum));		\
		cpunum &= 0x0F;				\
	})

#define get_hw_cpu_mask(cpu_mask)	((cpu_mask << master_cpu_id) & group_cpu_mask)
#define get_hw_cpu_id(cpu)		(cpu + master_cpu_id)
#define is_primary_amp()		(master_cpu_id == 0 ? 1 : 0)

#endif //ASMARM_ARCH_SMP_H
