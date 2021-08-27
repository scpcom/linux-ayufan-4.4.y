/*
 *  linux/arch/arm/plat-armada/hotplug.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/completion.h>

#include <asm/cacheflush.h>
#include <../cpuidle.h>

#define DEBUG

#define hard_smp_processor_id()                 \
        ({                                              \
                unsigned int cpunum;                    \
                __asm__("mrc p15, 0, %0, c0, c0, 5"     \
                        : "=r" (cpunum));               \
                cpunum &= 0x0F;                         \
        })

extern int armadaxp_snoop_dis_virt(void);

static DECLARE_COMPLETION(cpu_killed);

static inline void platform_do_lowpower(unsigned int cpu)
{

	/*
	 * there is no power-control hardware on this platform, so all
	 * we can do is put the core into WFI; this is safe as the calling
	 * code will have already disabled interrupts
	 */

	for (;;) {

		/*
		 * here's the WFI
		*/

		wfi();

		/*
		 * getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * The trouble is, letting people know about this is not really
		 * possible, since we are currently running incoherently, and
		 * therefore cannot safely call printk() or anything else
		 */
#ifdef DEBUG
		printk("CPU%u: spurious wakeup call\n", cpu);
#endif
	}
}

int platform_cpu_kill(unsigned int cpu)
{
	return wait_for_completion_timeout(&cpu_killed, 5000);
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
void __ref platform_cpu_die(unsigned int cpu)
{
#ifdef DEBUG
	unsigned int this_cpu = hard_smp_processor_id();

	if (cpu != this_cpu) {
		printk(KERN_CRIT "Eek! platform_cpu_die running on %u, should be %u\n",
			   this_cpu, cpu);
		BUG();
	}
#endif

	complete(&cpu_killed);

	/*
	 * we're ready for shutdown now, so do it
	 */

	flush_cache_all();

#ifdef CONFIG_SHEEVA_DEEP_IDLE
	armadaxp_fabric_prepare_deepIdle();
	armadaxp_fabric_prepare_hotplug();
#endif
#if defined CONFIG_AURORA_IO_CACHE_COHERENCY
	armadaxp_snoop_dis_virt();
#endif
	/* none zero means deepIdle wasn't entered and regret event happened */

	platform_do_lowpower(cpu);

	/*
	 * bring this CPU back into the world of cache
	 * coherency, and then restore interrupts - will never get here it.
	   We will do it as part of the secondary boot flow
	 */

}

int platform_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */

	return cpu == 0 ? -EPERM : 0;
}
