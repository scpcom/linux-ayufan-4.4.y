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
#define SCU_DIAG_CTRL		0x30
#define SCU_FILTER_START	0x40
#define SCU_FILTER_END		0x44
#define SCU_SAC			0x50
#define SCU_SNSAC		0x54

#ifdef CONFIG_SMP
/*
 * Get the number of CPU cores from the SCU configuration
 */
unsigned int __init scu_get_core_count(void __iomem *scu_base)
{
	unsigned int ncores = readl_relaxed(scu_base + SCU_CONFIG);
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
		scu_ctrl = readl_relaxed(scu_base + 0x30);
		if (!(scu_ctrl & 1))
			writel_relaxed(scu_ctrl | 0x1, scu_base + 0x30);
	}
#endif

	scu_ctrl = readl_relaxed(scu_base + SCU_CTRL);
	/* already enabled? */
	if (scu_ctrl & 1)
		return;

#ifdef CONFIG_SCU_SPECULATIVE_LINE_FILLS
	scu_ctrl |= (1 << 3);
#endif

	scu_ctrl |= 1;
	writel_relaxed(scu_ctrl, scu_base + SCU_CTRL);

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
int __scu_power_mode(void __iomem *scu_base, int cpu, unsigned int mode)
{
	unsigned int val;

	if (mode > 3 || mode == 1 || cpu > 3)
		return -EINVAL;

	val = readb_relaxed(scu_base + SCU_CPU_STATUS + cpu) & ~0x03;
	val |= mode;
	writeb_relaxed(val, scu_base + SCU_CPU_STATUS + cpu);

	return 0;
}

int scu_power_mode(void __iomem *scu_base, unsigned int mode)
{
	int cpu = smp_processor_id();

	return __scu_power_mode(scu_base, cpu, mode);
}

void scu_save(void __iomem *scu_base, struct scu_context *scu)
{
	scu->ctrl = readl_relaxed(scu_base + SCU_CTRL);
#ifdef CONFIG_ARM_ERRATA_764369
	scu->diag_ctrl = readl_relaxed(scu_base + SCU_DIAG_CTRL);
#endif
	scu->cpu_status = readl_relaxed(scu_base + SCU_CPU_STATUS);
	scu->filter_start = readl_relaxed(scu_base + SCU_FILTER_START);
	scu->filter_end = readl_relaxed(scu_base + SCU_FILTER_END);
	scu->sac = readl_relaxed(scu_base + SCU_SAC);
	scu->snsac = readl_relaxed(scu_base + SCU_SNSAC);
}

void scu_restore(void __iomem *scu_base, struct scu_context *scu)
{
	writel_relaxed(0xffff, scu_base + SCU_INVALIDATE);
	writel_relaxed(scu->filter_start, scu_base + SCU_FILTER_START);
	writel_relaxed(scu->filter_end, scu_base + SCU_FILTER_END);
	writel_relaxed(scu->sac, scu_base + SCU_SAC);
	writel_relaxed(scu->snsac, scu_base + SCU_SNSAC);
	writel_relaxed(scu->cpu_status, scu_base + SCU_CPU_STATUS);

#ifdef CONFIG_ARM_ERRATA_764369
	writel_relaxed(scu->diag_ctrl, scu_base + SCU_DIAG_CTRL);
#endif

	writel_relaxed(scu->ctrl, scu_base + SCU_CTRL);

	/*
	 * Ensure that the data accessed by CPU0 before the SCU was
	 * initialised is visible to the other CPUs.
	 */
	flush_cache_all();
}
