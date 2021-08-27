/*
 *  linux/arch/arm/mach-armadaxp/platsmp.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/unified.h>
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvSemaphore.h"
#include "include/mach/smp.h"

extern void axp_secondary_startup(void);
extern void axp_ipi_init(void);
extern void second_cpu_msi_init(void);
extern MV_CPU_DEC_WIN *mv_sys_map(void);
extern unsigned long mv_cpu_count;
extern void armadaxp_fabric_restore_deepIdle(void);

unsigned int master_cpu_id  = 0;
unsigned int group_cpu_mask = ((1 << NR_CPUS) - 1);

/*
 * We use IRQ1 as the IPI
 */
static inline void axp_smp_cross_call(const struct cpumask *mask, unsigned int irqnr)
{
	unsigned long map = *cpus_addr(*mask);
	void __iomem *addr = (void __iomem *)(AXP_SW_TRIG_IRQ);

	map = get_hw_cpu_mask(map);

	writel((((map & 0xf) << 8) | irqnr), addr);

	return;
}

static inline unsigned int get_sample_at_reset_core_count(void)
{
	/* Read the number of availabe CPUs in the SoC */
	return ((MV_REG_READ(SOC_COHERENCY_FABRIC_CFG_REG) & 0xF) + 1);
}

static unsigned int __init get_core_count(void)
{
#ifdef CONFIG_MACH_ARMADA_XP_FPGA
	return 2;
#else
	/* The number of CPUs in this SMP group is given by
	 * CMD line. The default is NR_CPUS */
	return mv_cpu_count;
#endif
}

void __init set_core_count(unsigned int cpu_count)
{
	/* Update cpu count */
	mv_cpu_count = cpu_count;

	/* Update group mask as well */
	group_cpu_mask = ((1 << cpu_count) - 1) << (hard_smp_processor_id());
}

/* 
 * Platform intialization routine for seconday CPUs
 */

void  platform_secondary_init(unsigned int cpu)
{
	trace_hardirqs_off();

#ifndef CONFIG_ARMADA_XP_REV_Z1
#ifdef	CONFIG_SHEEVA_DEEP_IDLE
	armadaxp_fabric_restore_deepIdle();
#endif
#endif

	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	axp_ipi_init();
#ifdef CONFIG_PCI_MSI
	/* Support for MSI interrupts */
	second_cpu_msi_init();
#endif

	smp_wmb();
}

int  boot_secondary(unsigned int cpu, struct task_struct *idle)
{

	MV_U32 reg;

	cpu = get_hw_cpu_id(cpu);

	printk("SMP: CPU %d Waking up CPU %d\n", master_cpu_id, cpu);

	/* send ipi to wake cpu in case it is in offline state */
	axp_smp_cross_call(cpumask_of(cpu), 0);
	/*
	 * Make memory consistent for secondary CPUs
	 * before waking them up and enabling memory coherency.
	 */
	flush_cache_all();

	/* Set resume control and address */
	MV_REG_WRITE(AXP_CPU_RESUME_CTRL_REG, 0x0);
	MV_REG_WRITE(AXP_CPU_RESUME_ADDR_REG(cpu),
			virt_to_phys(axp_secondary_startup));

	dsb();

	/* Kick secondary CPUs */
	reg = MV_REG_READ(AXP_CPU_RESET_REG(cpu));
	reg = reg & ~(1 << AXP_CPU_RESET_OFFS);
	MV_REG_WRITE(AXP_CPU_RESET_REG(cpu), reg);

	mb();
	udelay(10);

	return 0;
}

static void set_cpu_clocks(void)
{
	MV_U32 val = 0;
	MV_U32 ncores = get_core_count();
	MV_U32 cpu_id, cpu_mask;

#ifndef CONFIG_MACH_ARMADA_XP_FPGA
#ifndef CONFIG_ARMADA_XP_REV_Z1
	/* Scale up CPU#1 clock to max */
	MV_U32 divider = MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG);
	divider = (divider & 0x3F);

	pr_info("Setting Clocks for secondary CPUs\n");

	for (cpu_id = master_cpu_id + 1; cpu_id < (master_cpu_id + ncores); cpu_id++) {
		if (cpu_id == 1) {
			val = MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG);
			val &= ~(0x0000FF00); 	/* cpu1 clkdiv ratio; cpu0 based on SAR */
			val |= divider << 8;
			MV_REG_WRITE(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG, val);
		} else if (cpu_id == 2) {
			val = MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG);
			val &= ~(0x00FF0000);   /* cpu1 clkdiv ratio; cpu0 based on SAR */
			val |= divider << 16;
			MV_REG_WRITE(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG, val);
		} else if (cpu_id == 3) {
			val = MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG);
			val &= ~0xFF000000;	/* cpus 3 clkdiv ratios */
			val |= divider << 24;
			MV_REG_WRITE(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG, val);
		}
	}
#else /*CONFIG_ARMADA_XP_REV_Z1*/
	/* Scale up CPU#1 clock to max */
	if (ncores > 1) {
		val = MV_REG_READ(CPU_DIV_CLK_CTRL2_RATIO_FULL0_REG);
		val &= ~(0xFF000000);   /* cpu1 clkdiv ratio; cpu0 based on SAR */
		val |= 0x1 << 24;
		MV_REG_WRITE(CPU_DIV_CLK_CTRL2_RATIO_FULL0_REG, val);
	}

	/* Scale up CPU#2 clock to max */
	if (ncores > 2) {
		val = MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG);
		val &= ~0x00FF0000;     /* cpus 2 clkdiv ratios */
		val |= 0x1 << 16;
		MV_REG_WRITE(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG, val);
	}

	/* Scale up CPU#3 clock to max */
	if (ncores > 3) {
		val = MV_REG_READ(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG);
		val &= ~0xFF000000;     /* cpus 3 clkdiv ratios */
		val |= 0x1 << 24;
		MV_REG_WRITE(CPU_DIV_CLK_CTRL3_RATIO_FULL1_REG, val);
	}
#endif/*CONFIG_ARMADA_XP_REV_Z1*/

	cpu_mask = ((0x1 << (ncores-1)) - 1) << master_cpu_id;

#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaLock(MV_SEMA_CLOCK);
#endif
	/* Set clock devider reload smooth bit mask */
	val = MV_REG_READ(CPU_DIV_CLK_CTRL0_REG);
	val |= (cpu_mask) << 21;
	MV_REG_WRITE(CPU_DIV_CLK_CTRL0_REG, val);

	/* Request clock devider reload */
	val = MV_REG_READ(CPU_DIV_CLK_CTRL0_REG);
	val |= 1 << 24;
	MV_REG_WRITE(CPU_DIV_CLK_CTRL0_REG, val);

	/* Wait for clocks to settle down then release reload request */
	udelay(100);
	val &= ~(0xf << 21);
	MV_REG_WRITE(CPU_DIV_CLK_CTRL0_REG, val);
	udelay(100);
#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaUnlock(MV_SEMA_CLOCK);
#endif
#endif /*CONFIG_MACH_ARMADA_XP_FPGA*/

}

static void init_coherency(void)
{
	MV_U32 reg;
	MV_U32 core_bits;

	/* Enable the boot CPU in coherency fabric */
	core_bits = 1 << master_cpu_id;

#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaLock(MV_SEMA_BRIDGE);
#endif
	/* Associate group cores to the same SMP group */
	reg = MV_REG_READ(SOC_COHERENCY_FABRIC_CFG_REG);
	reg |= (core_bits << 24);
	MV_REG_WRITE(SOC_COHERENCY_FABRIC_CFG_REG, reg);

	/* Enable Snooping on coherency fabric */
	reg = MV_REG_READ(SOC_COHERENCY_FABRIC_CTRL_REG);
	reg |= (core_bits << 24);
	MV_REG_WRITE(SOC_COHERENCY_FABRIC_CTRL_REG, reg);

#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaUnlock(MV_SEMA_BRIDGE);
#endif
}

/*
 * Initialize the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
{
	MV_U32 i;
	MV_U32 ncores = get_core_count();

	printk("SMP: init cpus\n");

	/* Set the HW CPU id of the master core */
	master_cpu_id  = hard_smp_processor_id();

	/* Set CPU address decoding */
	if (mvCpuIfInit(mv_sys_map())) {
		printk("Cpu Interface initialization failed.\n");
		return;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	set_smp_cross_call(axp_smp_cross_call);
}

void smp_resume()
{
	if(mv_cpu_count > 1)
		set_cpu_clocks();
	init_coherency();

	axp_ipi_init();
}

void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int ncores = get_core_count();
	int i;

	printk("SMP: prepare CPUs (%d cores)\n", ncores);
	/* sanity check */
	if (ncores == 0) {
		printk(KERN_ERR
		       "strange CM count of 0? Default to 1\n");

		ncores = 1;
	}

	if (ncores > NR_CPUS) {
		printk(KERN_WARNING
		       "no. of cores (%d) greater than configured "
		       "maximum of %d - clipping\n",
		       ncores, NR_CPUS);
		ncores = NR_CPUS;
	}

	if ((ncores + master_cpu_id) > NR_CPUS) {
		printk(KERN_WARNING
		       "Bad core count (%d) for SMP group starting at cpu no (%d). There is no CPU %d. Clipping\n",
				ncores, master_cpu_id, ncores + master_cpu_id);
		ncores = (NR_CPUS - master_cpu_id);
	}

	if (ncores > get_sample_at_reset_core_count())
		ncores = get_sample_at_reset_core_count();

	/* Adjust core count in case fixing was done */
	set_core_count(ncores);

	/*
	 * are we trying to boot more cores than exist?
	 */
	if (max_cpus > ncores)
		max_cpus = ncores;

	for (i = 0; i < NR_CPUS; i++) {
		set_cpu_possible(i, false);
		set_cpu_present(i, false);
	}
	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	for (i = 0; i < max_cpus; i++) {
		set_cpu_possible(i, true);
		set_cpu_present(i, true);
	}
	if (max_cpus > 1) {
		flush_cache_all();
		set_cpu_clocks();
	}
	init_coherency();
}
