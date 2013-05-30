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

/* Creating Task for Cpu-1 Hotplug */
struct task_struct *thread1=NULL;

DECLARE_WAIT_QUEUE_HEAD(cpu1_hotplug);

/*
 * Hotplug signal from CPU Hotplug framework
 * to invoke the Hotplug task
 */
u32 cpu1_hotplug_done;

/* Cpu-1 Hotplug Task */
void hotplug_cpu1_die(void)
{
	unsigned int reset;
	struct cpumask in_mask;
	int cpu = 0;
	int pid = 0;

	cpumask_set_cpu(cpu, &in_mask);
	sched_setaffinity(pid, &in_mask);

wait_for_cpu1_hotplug_done:

	/* Waiting for hotplug event invoked by CPU hotplug framework */
	wait_event(cpu1_hotplug, cpu1_hotplug_done>0);

#ifdef CONFIG_NEON
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) & ~NEON1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
	__raw_writel((__raw_readl(A9DP_CPU_RESET) | NEON1_RST), A9DP_CPU_RESET);
#endif
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) & ~CPU1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
	__raw_writel((__raw_readl(A9DP_PWR_CNTRL) | CLAMP_CORE1), A9DP_PWR_CNTRL);
	__raw_writel((__raw_readl(A9DP_PWR_CNTRL) | CORE_PWRDWN1), A9DP_PWR_CNTRL);
	__raw_writel((__raw_readl(A9DP_CPU_RESET) | CPU1_RST), A9DP_CPU_RESET);
	__raw_writel((__raw_readl(A9DP_PWR_CNTRL) & ~CORE_PWRDWN1), A9DP_PWR_CNTRL);

	cpu1_hotplug_done = 0;
	goto wait_for_cpu1_hotplug_done;

	return;
}

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

        /* Create cpu1_hotplug_thread */

	printk(" Creating cpu1_hotplug_thread....\n");
	cpu1_hotplug_done = 0;
	thread1 = kthread_create(hotplug_cpu1_die,NULL,our_thread);

	if((thread1))
		{
		printk(" cpu1_hotplug_thread Created\n");
		wake_up_process(thread1);
		}

	return 0;
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
	unsigned int *loop = (unsigned int *)phys_to_virt(0x08);

	/*
	 * Install the comcerto_secondary_startup pointer at 0x20
	 * Physical Address
	 */
	__raw_writel(BSYM(virt_to_phys(comcerto_secondary_startup)), phys_to_virt(0x20));
	__raw_writel((unsigned int)JUMP_TO_KERNEL_START_1 , phys_to_virt(0x00));
	__raw_writel((unsigned int)JUMP_TO_KERNEL_START_2 , phys_to_virt(0x04));
	smp_wmb();
	__cpuc_flush_dcache_area((void *)phys_to_virt(0x00), 0x24);
	outer_clean_range(__pa(phys_to_virt(0x00)), __pa(phys_to_virt(0x24)));

	/* Get CPU 1 out of reset */
	__raw_writel((__raw_readl(A9DP_CPU_RESET) & ~CPU1_RST), A9DP_CPU_RESET);
	__raw_writel((__raw_readl(A9DP_PWR_CNTRL) & ~CLAMP_CORE1), A9DP_PWR_CNTRL);
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) | CPU1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);

#ifdef CONFIG_NEON
	/* Get NEON 1 out of reset */
	__raw_writel((__raw_readl(A9DP_CPU_RESET) & ~NEON1_RST), A9DP_CPU_RESET);
	__raw_writel((__raw_readl(A9DP_CPU_CLK_CNTRL) | NEON1_CLK_ENABLE), A9DP_CPU_CLK_CNTRL);
#endif
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
