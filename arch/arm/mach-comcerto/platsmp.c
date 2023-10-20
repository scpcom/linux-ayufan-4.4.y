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
#include <asm/hardware/gic.h>
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

static void __iomem *scu_base_addr(void)
{
	return (void *)COMCERTO_SCU_VADDR;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
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

	set_smp_cross_call(gic_raise_softirq);
}

#define JUMP_TO_KERNEL_START_1		0xe3a00020 	/* mov	r0, #32 */
#define JUMP_TO_KERNEL_START_2		0xe590f000 	/* ldr	pc, [r0] */


void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;

	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	scu_enable(scu_base_addr());

	return;
}

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 */
volatile int __cpuinitdata pen_release = -1;

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void __cpuinit write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

static DEFINE_SPINLOCK(boot_lock);

void __cpuinit platform_secondary_init(unsigned int cpu)
{
	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(0);

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;
	u32 cpu_reset, mpu_reset, clk_cntrl, pwr_cntrl;

	/*
	 * Install the comcerto_secondary_startup pointer at 0x20
	 * Physical Address
	 */
	writel_relaxed(virt_to_phys(comcerto_secondary_startup), phys_to_virt(0x20));
	writel_relaxed(JUMP_TO_KERNEL_START_1, phys_to_virt(0x00));
	writel_relaxed(JUMP_TO_KERNEL_START_2, phys_to_virt(0x04));
	smp_wmb();
	__cpuc_flush_dcache_area(phys_to_virt(0x00), 0x24);
	outer_clean_range(__pa(phys_to_virt(0x00)), __pa(phys_to_virt(0x24)));

	/* Get CPU 1 out of reset */
	pwr_cntrl = readl_relaxed(A9DP_PWR_CNTRL);
	pwr_cntrl &= ~CORE_PWRDWN1;
	writel_relaxed(pwr_cntrl, A9DP_PWR_CNTRL);

	cpu_reset = readl_relaxed(A9DP_CPU_RESET);
	mpu_reset = readl_relaxed(A9DP_MPU_RESET);

	cpu_reset |= CPU1_RST;
#ifdef CONFIG_NEON
	cpu_reset |= NEON1_RST;
#endif
	writel_relaxed(cpu_reset, A9DP_CPU_RESET);

	mpu_reset |= CPU1_DBG_RST;
	writel_relaxed(mpu_reset, A9DP_MPU_RESET);

	clk_cntrl = readl_relaxed(A9DP_CPU_CLK_CNTRL);

	clk_cntrl |= CPU1_CLK_ENABLE;
#ifdef CONFIG_NEON
	clk_cntrl |= NEON1_CLK_ENABLE;
#endif
	writel_relaxed(clk_cntrl, A9DP_CPU_CLK_CNTRL);

	udelay(5); /* tPC = 5us */

	clk_cntrl &= ~CPU1_CLK_ENABLE;
#ifdef CONFIG_NEON
	clk_cntrl &= ~NEON1_CLK_ENABLE;
#endif
	writel_relaxed(clk_cntrl, A9DP_CPU_CLK_CNTRL);

	ndelay(10); /* tCR = 10ns */

	mpu_reset &= ~CPU1_DBG_RST;
	writel_relaxed(mpu_reset, A9DP_MPU_RESET);

	cpu_reset &= ~CPU1_RST;
#ifdef CONFIG_NEON
	cpu_reset &= ~NEON1_RST;
#endif
	writel_relaxed(cpu_reset, A9DP_CPU_RESET);

	pwr_cntrl &= ~CLAMP_CORE1;
	writel_relaxed(pwr_cntrl, A9DP_PWR_CNTRL);

	ndelay(20); /* tRC2 = 20ns */

	clk_cntrl |= CPU1_CLK_ENABLE;
#ifdef CONFIG_NEON
	clk_cntrl |= NEON1_CLK_ENABLE;
#endif
	writel_relaxed(clk_cntrl, A9DP_CPU_CLK_CNTRL);

	pr_info("CPU%d powered up\n", cpu);

	__scu_power_mode(scu_base_addr(), cpu, SCU_PM_NORMAL);

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(cpu);

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}
