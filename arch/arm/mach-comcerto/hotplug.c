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

#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>   // for using jiffies
#include <linux/timer.h>

#include <linux/wait.h>
#include <linux/workqueue.h>



extern volatile int pen_release;

extern cpu1_hotplug;
extern u32 cpu1_hotplug_done;

int platform_cpu_kill(unsigned int cpu)
{
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
	
         /* Entering to LOW power state. 
	  * Go to Low power ,Configure the CPU to reset mode.
	  */

	if(cpu) {
		/* Put A9 CPU-1 to reset */
		cpu1_hotplug_done = 1;
		wake_up(&cpu1_hotplug);
		while(1);
	}
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
