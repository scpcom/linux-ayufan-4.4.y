#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/arch/arm/kernel/smp_scu.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/io.h>

#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>

#define SCU_CTRL		0x00
#define SCU_CONFIG		0x04
#define SCU_CPU_STATUS		0x08
#define SCU_INVALIDATE		0x0c
#define SCU_FPGA_REVISION	0x10

#if defined(CONFIG_SMP) || defined(MY_ABC_HERE)
/*
 * Get the number of CPU cores from the SCU configuration
 */
unsigned int __init scu_get_core_count(void __iomem *scu_base)
{
#if defined(MY_ABC_HERE)
	unsigned int ncores = readl_relaxed(scu_base + SCU_CONFIG);
#else /* MY_ABC_HERE */
	unsigned int ncores = __raw_readl(scu_base + SCU_CONFIG);
#endif /* MY_ABC_HERE */
	return (ncores & 0x03) + 1;
}

/*
 * Enable the SCU
 */
void scu_enable(void __iomem *scu_base)
{
	u32 scu_ctrl;

#ifdef CONFIG_ARM_ERRATA_764369
	/* Cortex-A9 only */
	if ((read_cpuid_id() & 0xff0ffff0) == 0x410fc090) {
#if defined(MY_ABC_HERE)
		scu_ctrl = readl_relaxed(scu_base + 0x30);
		if (!(scu_ctrl & 1))
			writel_relaxed(scu_ctrl | 0x1, scu_base + 0x30);
#else /* MY_ABC_HERE */
		scu_ctrl = __raw_readl(scu_base + 0x30);
		if (!(scu_ctrl & 1))
			__raw_writel(scu_ctrl | 0x1, scu_base + 0x30);
#endif /* MY_ABC_HERE */
	}
#endif

#if defined(MY_ABC_HERE)
	scu_ctrl = readl_relaxed(scu_base + SCU_CTRL);
#else /* MY_ABC_HERE */
	scu_ctrl = __raw_readl(scu_base + SCU_CTRL);
#endif /* MY_ABC_HERE */

#if defined (MY_DEF_HERE)
	/* Enable SCU standby mode To allow L2 cache controller idle mode */
	scu_ctrl |= BIT(5);
	__raw_writel(scu_ctrl, scu_base + SCU_CTRL);
#endif /* MY_DEF_HERE */
	/* already enabled? */
	if (scu_ctrl & 1)
		return;

	scu_ctrl |= 1;
#if defined(MY_ABC_HERE)
	writel_relaxed(scu_ctrl, scu_base + SCU_CTRL);
#else /* MY_ABC_HERE */
	__raw_writel(scu_ctrl, scu_base + SCU_CTRL);
#endif /* MY_ABC_HERE */

	/*
	 * Ensure that the data accessed by CPU0 before the SCU was
	 * initialised is visible to the other CPUs.
	 */
	flush_cache_all();
}
#endif

/*
 * Set the executing CPUs power mode as defined.  This will be in
 * preparation for it executing a WFI instruction.
 *
 * This function must be called with preemption disabled, and as it
 * has the side effect of disabling coherency, caches must have been
 * flushed.  Interrupts must also have been disabled.
 */
int scu_power_mode(void __iomem *scu_base, unsigned int mode)
{
	unsigned int val;
	int cpu = MPIDR_AFFINITY_LEVEL(cpu_logical_map(smp_processor_id()), 0);

	if (mode > 3 || mode == 1 || cpu > 3)
		return -EINVAL;

#if defined(MY_ABC_HERE)
	val = readb_relaxed(scu_base + SCU_CPU_STATUS + cpu) & ~0x03;
	val |= mode;
	writeb_relaxed(val, scu_base + SCU_CPU_STATUS + cpu);
#else /* MY_ABC_HERE */
	val = __raw_readb(scu_base + SCU_CPU_STATUS + cpu) & ~0x03;
	val |= mode;
	__raw_writeb(val, scu_base + SCU_CPU_STATUS + cpu);
#endif /* MY_ABC_HERE */

	return 0;
}
