/*
 * comcerto SMP cpu-hotplug support
 *
 * Copyright (C) 2012 Mindspeed Technologies, LTD.
 * Author:
 *      Satyabrata sahu <satyabrat.sahu@mindspeed.com>
 *
 * Platform file needed for the comcerto A9 SMP system . This file is based on arm
 * realview smp platform.
 * Copyright (c) 2002 ARM Limited.

 * CPU-1 shutdown and reset . while makeing online , CPU-1 will be again in 
 * out of reset mode.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/io.h>

#include <asm/cacheflush.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/smp_scu.h>

#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>   // for using jiffies
#include <linux/timer.h>

#include <linux/wait.h>
#include <linux/workqueue.h>



extern volatile int pen_release;

extern u32 cpu1_hotplug_done;

int platform_cpu_kill(unsigned int cpu)
{
	int running_cpu = get_cpu();
	u32 val;

	pr_notice("Killing CPU%d from CPU%d\n", cpu, running_cpu);

	if (cpu) {
		int count = 5000;

		do {
			val = __raw_readl(A9DP_PWR_STAT);
			mdelay(1);
		} while (count-- && !(val & (1 << 1)));

		if (!(val & (1 << 1))) {
			pr_err("CPU%d not in standby\n", cpu);
			return -1;
		}

		val = __raw_readl(A9DP_CPU_CLK_CNTRL);

#ifdef CONFIG_NEON
		val &= ~NEON1_CLK_ENABLE;
#endif
		val &=  ~CPU1_CLK_ENABLE;

		__raw_writel(val, A9DP_CPU_CLK_CNTRL);

		ndelay(10); /* tCC = 10ns */

		val = __raw_readl(A9DP_PWR_CNTRL);

		__raw_writel(val | CLAMP_CORE1, A9DP_PWR_CNTRL);

		ndelay(20); /* tCP = 20ns */

		__raw_writel(val | CLAMP_CORE1 | CORE_PWRDWN1, A9DP_PWR_CNTRL);

		pr_notice("CPU%d powered down\n", cpu);
	}

	return 1;
}



/*
 * platform-specific code to shutdown a CPU
 * Called with IRQs disabled
 */
void platform_cpu_die(unsigned int cpu)
{
	/* Flush all cache  */
	flush_cache_all();
	dsb();

	/*
	 * we're ready for shutdown now, so do it
	 */
	
	scu_power_mode((void *)COMCERTO_SCU_VADDR, SCU_PM_POWEROFF);
	wfi();
}

int platform_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */

	if (!cpu)
                pr_info("We are not allowing the CPU(%d) to shutdown. \n",cpu);
	
	return cpu == 0 ? -EPERM : 0;
}
