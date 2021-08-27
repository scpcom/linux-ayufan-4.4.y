#ifndef ASMARM_ARCH_SMP_H
#define ASMARM_ARCH_SMP_H

#include <asm/io.h>
#include <mach/armada370.h>

#define hard_smp_processor_id()			\
	({						\
		unsigned int cpunum;			\
		__asm__("mrc p15, 0, %0, c0, c0, 5"	\
			: "=r" (cpunum));		\
		cpunum &= 0x0F;				\
	})

/*
 * We use IRQ1 as the IPI
 */
static inline void smp_cross_call(const struct cpumask *mask)
{
	unsigned long map = *cpus_addr(*mask);
	void __iomem *addr = (void __iomem *)(AXP_SW_TRIG_IRQ);

	//printk("smp_cross_call %x \n",(unsigned int)( ((map & 0x3) << 8) | 0x0) );
	writel( ( ((map & 0xf) << 8) | 0x0), addr);

	return;
}

#endif
