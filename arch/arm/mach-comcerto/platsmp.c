/*
 *  arch/arm/mach-comcerto/platsmp.c
 *
 * Copyright (C) 2011 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>
//#include <asm/hardware/gic.h>
#include <linux/irqchip/arm-gic.h>
#include <asm/mach-types.h>
#include <asm/smp_scu.h>
#include <asm/unified.h>


#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>   // for using jiffies
#include <linux/timer.h>

#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/pid.h>


extern void comcerto_secondary_startup(void);
extern void platform_cpu_die(unsigned int cpu);
extern void platform_cpu_disable(unsigned int cpu);
extern void platform_cpu_kill(unsigned int cpu);

static void __iomem *scu_base_addr(void)
{
	return (void *)COMCERTO_SCU_VADDR;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init platform_smp_init_cpus(void)
{
	void __iomem *scu_base = scu_base_addr();
	unsigned int i, ncores;

	ncores = scu_base ? scu_get_core_count(scu_base) : 1;

	if (ncores > NR_CPUS) {
		printk(KERN_WARNING
		       "Comcerto: no. of cores (%d) greater than configured "
		       "maximum of %d - clipping\n",
		       ncores, NR_CPUS);
		ncores = NR_CPUS;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	//set_smp_cross_call(gic_raise_softirq); /* MAK: FIXME */
}

#define JUMP_TO_KERNEL_START_1		0xe3a00020 	/* mov	r0, #32 */
#define JUMP_TO_KERNEL_START_2		0xe590f000 	/* ldr	pc, [r0] */


void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;
	char our_thread[25]="cpu1_hotplug_thread";

	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	scu_enable(scu_base_addr());

	return;
}

static DEFINE_SPINLOCK(boot_lock);

int __cpuinit platform_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	/*
	 * Install the comcerto_secondary_startup pointer at 0x20
	 * Physical Address
	 */
	//__raw_writel(BSYM(virt_to_phys(comcerto_secondary_startup)), phys_to_virt(0x20));
	__raw_writel(virt_to_phys(comcerto_secondary_startup), phys_to_virt(0x20));	
	__raw_writel((unsigned int)JUMP_TO_KERNEL_START_1 , phys_to_virt(0x00));
	__raw_writel((unsigned int)JUMP_TO_KERNEL_START_2 , phys_to_virt(0x04));
	smp_wmb();
	__cpuc_flush_dcache_area((void *)phys_to_virt(0x00), 0x24);
	outer_clean_range(__pa(phys_to_virt(0x00)), __pa(phys_to_virt(0x24)));

	/* Get CPU 1 out of reset */
	__raw_writel((__raw_readl(A9DP_PWR_CNTRL) & ~CORE_PWRDWN1), A9DP_PWR_CNTRL);
#ifdef CONFIG_NEON
	__raw_writel((__raw_readl(A9DP_CPU_RESET) | CPU1_RST | NEON1_RST), A9DP_CPU_RESET);
#else
	__raw_writel((__raw_readl(A9DP_CPU_RESET) | CPU1_RST), A9DP_CPU_RESET);
#endif
	__raw_writel((__raw_readl(A9DP_MPU_RESET) | CPU1_DBG_RST), A9DP_MPU_RESET);
#ifdef CONFIG_NEON
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) | CPU1_CLK_ENABLE | NEON1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
#else
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) | CPU1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
#endif
	udelay(5); /* tPC = 5us */	
#ifdef CONFIG_NEON
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) & ~CPU1_CLK_ENABLE & ~NEON1_RST), A9DP_CPU_CLK_CNTRL);
#else
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) & ~CPU1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
#endif
	ndelay(10); /* tCR = 10ns */
	__raw_writel((__raw_readl(A9DP_MPU_RESET) & ~CPU1_DBG_RST), A9DP_MPU_RESET);
#ifdef CONFIG_NEON
	__raw_writel((__raw_readl(A9DP_CPU_RESET) & ~CPU1_RST & ~NEON1_RST), A9DP_CPU_RESET);
#else
	__raw_writel((__raw_readl(A9DP_CPU_RESET) & ~CPU1_RST), A9DP_CPU_RESET);
#endif
	__raw_writel((__raw_readl(A9DP_PWR_CNTRL) & ~CORE_PWRDWN1 & ~CLAMP_CORE1), A9DP_PWR_CNTRL);
	ndelay(20); /* tRC2 = 20ns */
#ifdef CONFIG_NEON
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) | CPU1_CLK_ENABLE | NEON1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
#else
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) | CPU1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
#endif
	pr_notice("CPU%d powered up\n", cpu);

	__scu_power_mode(scu_base_addr(), cpu, SCU_PM_NORMAL);
	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);

	return 0;
}

struct smp_operations comcerto_smp_ops __initdata = {
    .smp_init_cpus      = platform_smp_init_cpus,
    .smp_prepare_cpus   = platform_smp_prepare_cpus,
    //.smp_secondary_init = platform_secondary_init,
    .smp_boot_secondary = platform_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
    .cpu_die        = platform_cpu_die,
    .cpu_disable    = platform_cpu_disable,
    .cpu_kill       = platform_cpu_kill,
#endif
};
