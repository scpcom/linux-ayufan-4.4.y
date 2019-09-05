/*
 * Timer device implementation for Comcerto 2000 platform.
 *
 *  Copyright (C) 2012 Mindspeed Technologies, Inc.
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
 *
 * This driver exports  API that should be supportable by C2000 Timer
 *
 * 29/08/14 - Makarand Pawagi - initial revision
 *
 *		support via the posix timer interface
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/mmtimer.h>
#include <linux/miscdevice.h>
#include <linux/posix-timers.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/math64.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <mach/comcerto-2000/timer.h>
#include <mach/comcerto-2000/pm.h>
#include <linux/io.h>

/* name of the device, usually in /dev */
#define PMUTIMER_NAME 					"pmutimer"
#define PMUTIMER_DESC 					"C2000 PMU Timer"
#define PMUTIMER_VERSION 				"0.1"
#define PMUTIMER_CHAIN 				timer_1_3
#define PMUTIMER_CLOCK					(MAX_CLOCKS-1)

static struct k_clock pmutimer_clock;

#define COMCERTO_PMU_TIMER_MAGIC         0xADEADDAD
#define TIMER_OFF						0xbadcabLL	/* Timer is not setup */
#define TIMER_SET						0			/* Comparator is set for this timer */



struct pmutimer {
	struct k_itimer *timer;
	int cpu;
};

struct pmutimer_node {
	spinlock_t lock ____cacheline_aligned;
	struct tasklet_struct tasklet;
};

static struct pmutimer this_timer;
static struct pmutimer_node *timers;



/*
 * Posix Timer Interface
 */

static int pmu_clock_get(clockid_t clockid, struct timespec *tp)
{
	return 0;
};

static int pmu_clock_set(const clockid_t clockid, const struct timespec *tp)
{
	return 0;
}

/**
 * pmutimer_interrupt - timer interrupt handler
 * @irq: irq received
 * @dev_id: device the irq came from
 *
 * This interrupt handlar is run in an interrupt context.
 */
static irqreturn_t
pmutimer_interrupt(int irq, void *dev_id)
{
	int result = IRQ_NONE;

	printk(KERN_INFO "%s \n", __func__);

	spin_lock(&timers[0].lock);
	tasklet_schedule(&timers[0].tasklet);
	result = IRQ_HANDLED;
	spin_unlock(&timers[0].lock);
	return result;
}

static void pmutimer_tasklet(unsigned long data)
{
	struct pmutimer_node *mn = &timers[0];
	struct pmutimer *x;
	struct k_itimer *t;
	unsigned long flags;

	printk(KERN_INFO "%s data=0x%lx \n", __func__, data);

	/* Send signal and deal with periodic signals */
	spin_lock_irqsave(&mn->lock, flags);

	x = (struct pmutimer *)data;
	t = x->timer;

	if (t->it.mmtimer.clock == TIMER_OFF)
		goto out;

	t->it_overrun = 0;

	if (posix_timer_event(t, 0) != 0)
		t->it_overrun++;

out:
	spin_unlock_irqrestore(&mn->lock, flags);
}

static int pmu_timer_create(struct k_itimer *timer)
{
	printk(KERN_INFO "%s \n", __func__);

	this_timer.timer= timer;

	/* Insure that a newly created timer is off */
	timer->it.mmtimer.clock = TIMER_OFF;
	return 0;
}

static int pmu_timer_del(struct k_itimer *timr)
{
	unsigned long irqflags;

	printk(KERN_INFO "%s \n", __func__);

	spin_lock_irqsave(&timers[0].lock, irqflags);
	if (timr->it.mmtimer.clock != TIMER_OFF) {

		timr->it.mmtimer.clock = TIMER_OFF;
		timr->it.mmtimer.expires = 0;

		comcerto_timer_chain_kill(PMUTIMER_CHAIN);

		/* Clear PMU Timer Information to PMU */
		writel_relaxed(0x0, HOST_UTILPE_SHARED_TIMER_MAGIC);
	}
	spin_unlock_irqrestore(&timers[0].lock, irqflags);
	return 0;
}

static void pmu_timer_get(struct k_itimer *timr, struct itimerspec *cur_setting)
{
	return;
}

static int pmu_timer_set(struct k_itimer *timr, int flags,
	struct itimerspec * new_setting,
	struct itimerspec * old_setting)
{
	unsigned long irqflags;
	unsigned long when, period;
	int err = 0;

	printk(KERN_INFO "%s \n", __func__);

	when = new_setting->it_value.tv_sec;
	period = (new_setting->it_interval.tv_sec * USEC_PER_SEC) + (new_setting->it_interval.tv_nsec / NSEC_PER_USEC);  /* In Microsecend */

	if (when == 0)
		/* Clear timer */
		return 0;

	when = when-1;
	printk(KERN_INFO "%s when=0x%lx, period=0x%lx\n", __func__, when, period);


	/* Pass PMU Timer Information to PMU */
	writel_relaxed(COMCERTO_PMU_TIMER_MAGIC, HOST_UTILPE_SHARED_TIMER_MAGIC);
	writel_relaxed(PMUTIMER_CHAIN, HOST_UTILPE_SHARED_TIMER_CHAINID);
	writel_relaxed(period, HOST_UTILPE_SHARED_TIMER_PERIOD);
	writel_relaxed(when, HOST_UTILPE_SHARED_TIMER_VALUE);

	comcerto_timer_chain_run(PMUTIMER_CHAIN, period, when);

	preempt_disable();

	/* Lock the node timer structure */
	spin_lock_irqsave(&timers[0].lock, irqflags);

	timr->it.mmtimer.clock = TIMER_SET;
	timr->it.mmtimer.node = 0;
	timr->it.mmtimer.incr = period;
	timr->it.mmtimer.expires = when;

	spin_unlock_irqrestore(&timers[0].lock, irqflags);

	preempt_enable();

	return err;
}

static int pmu_clock_getres(const clockid_t which_clock, struct timespec *tp)
{
	return 0;
}

static struct k_clock pmutimer_clock = {
	.clock_set	= pmu_clock_set,
	.clock_get	= pmu_clock_get,
	.clock_getres	= pmu_clock_getres,
	.timer_create	= pmu_timer_create,
	.timer_set	= pmu_timer_set,
	.timer_del	= pmu_timer_del,
	.timer_get	= pmu_timer_get
};

/*
  * comcerto_pmutimer_probe - PMU timer initialization routine
  */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
static int __devinit comcerto_pmutimer_probe(struct platform_device *pdev)
#else
static int comcerto_pmutimer_probe(struct platform_device *pdev)
#endif
{
	printk(KERN_INFO "%s \n", __func__);

	if (request_irq(IRQ_TIMER3, pmutimer_interrupt, (IRQF_PERCPU | IRQF_DISABLED | IRQF_TIMER), PMUTIMER_NAME, NULL)) {
		printk(KERN_WARNING "%s: unable to allocate interrupt.",
			PMUTIMER_NAME);
		goto out1;
	}

	/* Allocate node ptrs to pmutimer */
	timers = kzalloc(sizeof(struct pmutimer_node), GFP_KERNEL);
	if (timers == NULL) {
		printk(KERN_ERR "%s: failed to allocate memory for device\n",
				PMUTIMER_NAME);
		goto out2;
	}

	/* Initialize struct pmutimer for node */
	spin_lock_init(&timers[0].lock);
	tasklet_init(&timers[0].tasklet, pmutimer_tasklet, (unsigned long)&this_timer);

	posix_timers_register_clock(PMUTIMER_CLOCK, &pmutimer_clock);

	/* Clear PMU Timer Information Area in IRAM */
	writel_relaxed(0x0, HOST_UTILPE_SHARED_TIMER_MAGIC);
	writel_relaxed(0x0, HOST_UTILPE_SHARED_TIMER_CHAINID);
	writel_relaxed(0x0, HOST_UTILPE_SHARED_TIMER_PERIOD);
	writel_relaxed(0x0, HOST_UTILPE_SHARED_TIMER_VALUE);

	printk(KERN_INFO "%s: v%s Initialized.. \n", PMUTIMER_DESC, PMUTIMER_VERSION);

	return 0;

out3:
	kfree(timers);
out2:
	free_irq(PMUTIMER_CLOCK, NULL);
out1:
	return -1;
}



#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
static int __devexit comcerto_pmutimer_remove(struct platform_device *pdev)
#else
static int comcerto_pmutimer_remove(struct platform_device *pdev)
#endif
{
	printk(KERN_INFO "%s \n", __func__);

	free_irq(IRQ_TIMER3, NULL);
	kfree(timers);

	printk(KERN_INFO "%s: v%s Removed.. \n", PMUTIMER_DESC, PMUTIMER_VERSION);

	return 0;
}

static struct platform_driver comcerto_pmutimer_driver = {
	.probe        = comcerto_pmutimer_probe,
	.remove       = comcerto_pmutimer_remove,
	.driver       = {
		.owner = THIS_MODULE,
		.name  = "comcerto_pmutimer",
	},
};


static int __init pmutimer_init(void)
{
	printk(KERN_INFO "%s \n", __func__);

	return platform_driver_register(&comcerto_pmutimer_driver);
}
module_init(pmutimer_init);

static void __exit pmutimer_exit(void)
{
	printk(KERN_INFO "%s \n", __func__);

	platform_driver_unregister(&comcerto_pmutimer_driver);
}
module_exit(pmutimer_exit);


MODULE_AUTHOR("Makarand Pawagi <makarandpawagi@freescale.com>");
MODULE_DESCRIPTION("C2000 PMU Timer");
MODULE_LICENSE("GPL");
