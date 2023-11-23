/*
 *  linux/arch/arm/mach-comcerto/time.c
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
 */

#include <linux/init.h>
#include <linux/smp.h>
#include <linux/irq.h>

#include <linux/export.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <asm/irq.h>
#include <asm/smp_twd.h>
#include <linux/sched_clock.h>
//#include <asm/localtimer.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/comcerto-2000/clock.h>
#include <mach/comcerto-2000/timer.h>
#include <mach/comcerto-2000/pm.h>
#include <linux/io.h>
#include <linux/interrupt.h>

extern void comcerto_timer_init(void);


/* Kernel needs a timer cadenced to 10ms */
#define COMCERTO_KERNEL_TIMER_VALUE	(COMCERTO_AHBCLK / HZ)
#define machinecycles_to_usecs(ticks) (((ticks) * 10)/ (COMCERTO_AHBCLK/100000))

/*
 * HARDWARE TIMER
 * Can be use by any driver for its own need
 */

#define COMCERTO_TIMER_DEBUG	1
#define TIMER_STATUS_ENABLED	(1 << 0)
#define TIMER_STATUS_FREE	(1 << 1)
#define COMCERTO_MAX_TIMERS	6
#define COMCERTO_MAX_TIMER_CHAINS	3
/*
 * HARDWARE TIMER
 * Can be use by any driver for its own need
 */

struct timer_hw timer_hw [COMCERTO_MAX_TIMERS] = {
	{0, 0, NULL}, /*  MSP timer    */
	{1, TIMER_STATUS_FREE, NULL},
	{2, 0, NULL}, /*  CLock source + Sched clock */
	{3, TIMER_STATUS_FREE, NULL},
	{4, 0, NULL},	/*  Clock event  */
	{5, TIMER_STATUS_FREE, NULL}
};


/* Globals for timer chain */
struct comcerto_timer timer_c0; /* Granularity in microseced*/
struct comcerto_timer timer_c1; /* Expiry in multiples of granularity */


timer_chain_pair timer_chain_list[COMCERTO_MAX_TIMER_CHAINS] = {
	{0, 2 }, /* Timer0 + Timer2 */
	{1, 3 }, /* Timer1 + Timer3 */
	{4, 5 }  /* Timer4 + Timer5 */

};


static spinlock_t comcerto_timer_lock;

static unsigned long COMCERTO_AHBCLK;

static void comcerto_timer_disable(int t)
{
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);
	__comcerto_timer_disable(t);
	spin_unlock_irqrestore(&comcerto_timer_lock, flags);
}

static unsigned long __comcerto_timer_get(int id)
{
	/* Timer-0 is reserved for MSP, thus not made configurable by this driver */

	if (id == 1)
	{
		return comcerto_timer1_get();
	}
	else if (id == 2)
	{
		return comcerto_timer2_get();
	}
	else if (id == 3)
	{
		return comcerto_timer3_get();
	}
	else if (id == 4)
	{
		return comcerto_timer4_get();
	}
	else if (id == 5)
	{
		return comcerto_timer5_get();
	}
	else
		return 0;
}

static void __comcerto_timer_set(int id, unsigned long count)
{
	/* Timer-0 is reserved for MSP, thus not made configurable by this driver */

	if (id == 1)
	{
		comcerto_timer1_set(count);
	}
	else if (id == 2)
	{
		comcerto_timer2_set(0, count, 0);
	}
	else if (id == 3)
	{
		comcerto_timer3_set(0, count, 0);
	}
	else if (id == 4)
	{
		comcerto_timer4_set(count);
	}
	else if (id == 5)
	{
		comcerto_timer5_set(0, count, 0);
	}
}

static unsigned long __comcerto_timer_chain_get(timer_chain_id chain_id)
{
	unsigned long axi_rate = COMCERTO_KERNEL_TIMER_VALUE;
	unsigned long hbound;

	if (chain_id == timer_1_3)
	{
		hbound = timer_readl(COMCERTO_TIMER1_HIGH_BOUND) / axi_rate; /* Period In Microseconds */
		return comcerto_timer3_get() * hbound; /* Time Elasped In Microseconds */
	}
	else if (chain_id == timer_4_5)
	{
		hbound = timer_readl(COMCERTO_TIMER4_HIGH_BOUND) / axi_rate; /* Period In Microseconds */
		return comcerto_timer5_get() * hbound; /* Time Elasped In Microseconds */
	}
	else
		return 0;
}

static void __comcerto_timer_chain_set(int id, unsigned long count)
{
	/* Timer-0 is reserved for MSP, thus not made configurable by this driver */

	if (id == 1)
	{
		comcerto_timer1_set(count);
	}
	else if (id == 2)
	{
		comcerto_timer2_set(0, count, 1);
	}
	else if (id == 3)
	{
		comcerto_timer3_set(0, count, 1);
	}
	else if (id == 4)
	{
		comcerto_timer4_set(count);
	}
	else if (id == 5)
	{
		comcerto_timer5_set(0, count, 1);
	}
}


static void __timer_start(struct timer_hw *thw, unsigned long count)
{
	thw->status |= TIMER_STATUS_ENABLED;

	__comcerto_timer_set(thw->id, count);

	__comcerto_timer_enable(thw->id);
}

static void timer_start(struct timer_hw *thw, unsigned long count)
{
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);
	__timer_start(thw, count);
	spin_unlock_irqrestore(&comcerto_timer_lock, flags);
}

static void __timer_stop(struct timer_hw *thw)
{
	__comcerto_timer_disable(thw->id);
	thw->status &= ~ TIMER_STATUS_ENABLED;
}

static void timer_stop(struct timer_hw *thw)
{
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);
	__timer_stop(thw);
	spin_unlock_irqrestore(&comcerto_timer_lock, flags);
}

static void __timer_free(struct timer_hw *thw)
{
	thw->status |= TIMER_STATUS_FREE;
	thw->t->thw = (unsigned long) NULL;
	thw->t = NULL;
}

static void timer_free(struct timer_hw *thw)
{
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);
	__timer_free(thw);
	spin_unlock_irqrestore(&comcerto_timer_lock, flags);
}


static struct timer_hw *__timer_alloc(struct comcerto_timer *t)
{
	struct timer_hw *thw;
	int i;

	for (i = 0; i < COMCERTO_MAX_TIMERS; i++)
	{
		thw = &timer_hw[i];
		if (thw->status & TIMER_STATUS_FREE)
		{
			thw->status &= ~ TIMER_STATUS_FREE;
			t->thw = (unsigned long) thw;
			thw->t = t;
			goto found;
		}
	}

	return NULL;
found:
	return thw;
}

static struct timer_hw *timer_alloc(struct comcerto_timer *t)
{
	struct timer_hw *thw;
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);
	thw = __timer_alloc(t);
	spin_unlock_irqrestore(&comcerto_timer_lock, flags);

	return thw;
}


static struct timer_hw *__timer_chain_alloc(struct comcerto_timer *t, u8 id)
{
	struct timer_hw *thw;

		thw = &timer_hw[id];
		if (thw->status & TIMER_STATUS_FREE)
		{
			thw->status &= ~ TIMER_STATUS_FREE;
			t->thw = (unsigned long) thw;
			thw->t = t;
			goto found;
		}


	return NULL;
found:
	return thw;
}

static struct timer_hw *timer_chain_alloc(struct comcerto_timer *t, u8 id)
{
	struct timer_hw *thw;
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);
	thw = __timer_chain_alloc(t, id);
	spin_unlock_irqrestore(&comcerto_timer_lock, flags);

	return thw;
}

/*
  *  Exported Timer API's:
  *  comcerto_timer_start(), comcerto_timer_stop(), comcerto_timer_read()
  */

int comcerto_timer_start(struct comcerto_timer *t)
{
	struct timer_hw *thw;

	thw = (struct timer_hw *) t->thw;
	if (!thw) {
		thw = timer_alloc(t);
		if (!thw) {
			printk (KERN_ERR "Comcerto timer: unable to allocate hardware timer\n");
			goto err;
		}
	}

#ifdef COMCERTO_TIMER_DEBUG
	if (thw->t != t) {
		printk (KERN_ERR "Comcerto timer: timer corruption %#lx %#lx %#lx\n",
					(unsigned long) thw, (unsigned long) thw->t, (unsigned long) t);

		goto err;
	}
#endif /* COMCERTO_TIMER_DEBUG */

	/* timeout in us */
	timer_start(thw, (t->timeout * (COMCERTO_AHBCLK / 100000)) / 10);

	return 0;

  err:
	return -1;
}

int comcerto_timer_stop(struct comcerto_timer *t)
{
	struct timer_hw *thw;

	thw = (struct timer_hw *) t->thw;
	if (!thw)
		goto err;

#ifdef COMCERTO_TIMER_DEBUG
	if (thw->t != t) {
		printk (KERN_ERR "Comcerto timer: timer corruption %#lx %#lx %#lx\n",
					(unsigned long) thw, (unsigned long) thw->t, (unsigned long) t);

		goto err;
	}
#endif /* COMCERTO_TIMER_DEBUG */

	timer_stop(thw);
	timer_free(thw);

	return 0;

  err:
	return -1;
}

int comcerto_timer_read(struct comcerto_timer *t)
{
	struct timer_hw *thw;

	thw = (struct timer_hw *) t->thw;
	if (!thw)
		goto err;

#ifdef COMCERTO_TIMER_DEBUG
	if (thw->t != t) {
		printk (KERN_ERR "Comcerto timer: timer corruption %#lx %#lx %#lx\n",
					(unsigned long) thw, (unsigned long) thw->t, (unsigned long) t);

		goto err;
	}
#endif /* COMCERTO_TIMER_DEBUG */

	return ((__comcerto_timer_get(thw->id) * 10) / (COMCERTO_AHBCLK / 100000));

  err:
	return -1;
}

EXPORT_SYMBOL(comcerto_timer_start);
EXPORT_SYMBOL(comcerto_timer_stop);
EXPORT_SYMBOL(comcerto_timer_read);



/*
  *  Exported Timer Chain API's:
  *  comcerto_timer_chain_start(), comcerto_timer_chain_stop(), comcerto_timer_chain_read()
  */

int comcerto_timer_chain_start(timer_chain_id chain_id, struct comcerto_timer *timer_c0, struct comcerto_timer *timer_c1)
{
	struct timer_hw *thw_c0;
	struct timer_hw *thw_c1;
	unsigned long flags;

	/* Allocate timers */

	thw_c0 = (struct timer_hw *) timer_c0->thw;
	if (!thw_c0) {
		thw_c0 = timer_chain_alloc(timer_c0, timer_chain_list[chain_id].chainid_0);
		if (!thw_c0) {
			printk (KERN_ERR "Comcerto timer: unable to allocate hardware timer\n");
			return -1;
		}
	}

	if (thw_c0->t != timer_c0) {
		printk ("Comcerto timer: timer corruption");
		return -1;
	}

	thw_c1 = (struct timer_hw *) timer_c1->thw;
	if (!thw_c1) {
		thw_c1 = timer_chain_alloc(timer_c1, timer_chain_list[chain_id].chainid_1);
		if (!thw_c1) {
			printk (KERN_ERR "Comcerto timer: unable to allocate hardware timer\n");
			return -1;
		}
	}


	if (thw_c1->t != timer_c1) {
		printk ("Comcerto timer: timer corruption");
		return -1;
	}

	/* Setting up granularity timer in chain */

	spin_lock_irqsave(&comcerto_timer_lock, flags);

	thw_c0->status |= TIMER_STATUS_ENABLED;
	__comcerto_timer_chain_set(thw_c0->id, (timer_c0->timeout * (COMCERTO_AHBCLK / 1000000)));

	thw_c1->status |= TIMER_STATUS_ENABLED;

	__comcerto_timer_chain_set(thw_c1->id, timer_c1->timeout);
	__comcerto_timer_enable(thw_c1->id);

	spin_unlock_irqrestore(&comcerto_timer_lock, flags);

	return 0;
}


int comcerto_timer_chain_stop(struct comcerto_timer *timer_c0, struct comcerto_timer *timer_c1)
{
	struct timer_hw *thw_c0;
	struct timer_hw *thw_c1;

	thw_c0 = (struct timer_hw *) timer_c0->thw;
	if (!thw_c0)
		goto err;

	if (thw_c0->t != timer_c0) {
		printk ("Comcerto timer: timer corruption");
		goto err;
	}

	thw_c1 = (struct timer_hw *) timer_c1->thw;
	if (!thw_c1)
		goto err;

	if (thw_c1->t != timer_c1) {
		printk ("Comcerto timer: timer corruption");
		goto err;
	}


	timer_stop(thw_c0);
	timer_free(thw_c0);

	timer_stop(thw_c1);
	timer_free(thw_c1);

	return 0;

  err:
	return -1;
}

int comcerto_timer_chain_read(timer_chain_id chain_id)
{
	return __comcerto_timer_chain_get(chain_id) ;
}

EXPORT_SYMBOL(comcerto_timer_chain_start);
EXPORT_SYMBOL(comcerto_timer_chain_stop);
EXPORT_SYMBOL(comcerto_timer_chain_read);

/*
  *  Exported PMU-Timer API's:
  *  comcerto_timer_chain_run(), comcerto_timer_chain_kill()
  */

int comcerto_timer_chain_run(timer_chain_id chain_id, u32 granularity, u32 expiry)
{
	timer_c0.timeout=granularity;
	timer_c0.func=NULL;
	timer_c0.data=0;
	timer_c0.flags=0;
	timer_c0.thw=0;

	timer_c1.timeout=expiry;
	timer_c1.func=NULL;
	timer_c1.data=0;
	timer_c1.flags=0;
	timer_c1.thw=0;

	comcerto_timer_chain_start(chain_id, &timer_c0, &timer_c1);
	return 0;
}
EXPORT_SYMBOL(comcerto_timer_chain_run);

int comcerto_timer_chain_kill(timer_chain_id chain_id)
{
	comcerto_timer_chain_stop(&timer_c0, &timer_c1);

	/* Clear any pending PMU interrupts */
	writel(0xffffffff, (void*)COMCERTO_GPIO_PMU_INTR_CLR);

	return 0;
}
EXPORT_SYMBOL(comcerto_timer_chain_kill);



int timer_hw_handler(u8 id)
{
	struct timer_hw *thw;
	unsigned long flags;

	thw = &timer_hw[id];

	if(!thw)
		return -1;

	spin_lock_irqsave(&comcerto_timer_lock, flags);

	if (thw->status & TIMER_STATUS_ENABLED) {

		struct comcerto_timer *t = thw->t;

		if(!t) {
			spin_unlock_irqrestore(&comcerto_timer_lock, flags);
			return -1;
		}

		if (t->flags & COMCERTO_TIMER_RUN_ONCE) {
			__timer_stop(thw);
			__timer_free(thw);
		}

		spin_unlock_irqrestore(&comcerto_timer_lock, flags);

		t->func(t->data);

	} else {
		spin_unlock_irqrestore(&comcerto_timer_lock, flags);
		goto err;
	}

	return 0;

  err:
	return -1;
}


struct comcerto_clock_device
{
	int timer;
	struct clock_event_device device;
};

/* This function must be called with interrupts disabled. We need to start as close from zero
 * as possible - if we don't clear counter before setting high bound value we'll have innacurate
 * results in ONESHOT mode.
 */
static int comcerto_clock_set_next_event(unsigned long evt, struct clock_event_device *dev)
{
	struct comcerto_clock_device *clock = container_of(dev, typeof(*clock), device);
	unsigned long flags;

	spin_lock_irqsave(&comcerto_timer_lock, flags);

	/* now write correct bound and clear interrupt status.
	   Writing high bound register automatically resets count to low bound value.
	   For very small bound values it's possible that we ack the interrupt _after_ the timer has already expired,
	   this is not very serious because the interrupt will be asserted again in a very short time */
	__comcerto_timer_set(clock->timer, evt);
	comcerto_timer_ack(clock->timer);

	/* enable interrupt for ONESHOT mode */
	if (dev->mode == CLOCK_EVT_MODE_ONESHOT)
		__comcerto_timer_enable(clock->timer);

	spin_unlock_irqrestore(&comcerto_timer_lock, flags);

	return 0;
}

static void comcerto_clock_set_mode(enum clock_event_mode mode, struct clock_event_device *dev)
{
	struct comcerto_clock_device *clock = container_of(dev, typeof(*clock), device);
	unsigned long flags;

	/* This timer is true PERIODIC in hardware, to emulate ONESHOT we need to keep it masked
	 * until set_next_event() call. Enable interrupt only for PERIODIC mode.
	 */
	spin_lock_irqsave(&comcerto_timer_lock, flags);

	if (mode != CLOCK_EVT_MODE_PERIODIC)
		__comcerto_timer_disable(clock->timer);
	else {
		__comcerto_timer_set(clock->timer, COMCERTO_KERNEL_TIMER_VALUE);
		__comcerto_timer_enable(clock->timer);
	}

	spin_unlock_irqrestore(&comcerto_timer_lock, flags);
}

static struct comcerto_clock_device clock =
{
	.timer  = 4,
	.device =
	{
		.name           = "timer4",
		.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
		.rating         = 200,
		.shift          = 31,
		.set_mode       = comcerto_clock_set_mode,
		.set_next_event = comcerto_clock_set_next_event,
	},
};

static cycle_t comcerto_timer2_read(struct clocksource *cs)
{
	return comcerto_timer2_get();
}

static struct clocksource clocksource =
{
	.name	= "timer2",
	.rating	= 200,
	.read	= comcerto_timer2_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 28,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};


/***********************************************************
 *   KERNEL TIMER                                          *
 *   (Functions called  by the kernel)                     *
 ***********************************************************/

#define irq_to_timer(irq)	(irq - IRQ_TIMER0)
#define timer_mask(timer)	(1 << timer)


/*
 * Routine to catch timer interrupts
 */
static irqreturn_t comcerto_timer4_interrupt(int irq, void *dev_id)
{
	u32 status;
	struct comcerto_clock_device *clock = dev_id;
	struct clock_event_device *dev = &clock->device;

	status = timer_readl(COMCERTO_TIMER_STATUS) & timer_readl(COMCERTO_TIMER_IRQ_MASK);

	/* timer1 expired */
	if (status & timer_mask(clock->timer)) {
		/* we need to disable interrupt to simulate ONESHOT mode,
		   do it before clearing the interrupt to avoid race */
		if (dev->mode != CLOCK_EVT_MODE_PERIODIC)
			comcerto_timer_disable(clock->timer);

		comcerto_timer_ack(clock->timer);
		dev->event_handler(dev);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

irqreturn_t comcerto_timerN_interrupt(int irq, void *dev_id)
{
	u32 status;
	int timer = irq_to_timer(irq);

	status = timer_readl(COMCERTO_TIMER_STATUS);
	if (status & timer_mask(timer)) {
		comcerto_timer_ack(timer);
		timer_hw_handler(timer);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static struct irqaction comcerto_timer1_irq = {
	.name		= "timer1",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= comcerto_timerN_interrupt,
};

static struct irqaction comcerto_timer3_irq = {
	.name		= "timer3",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= comcerto_timerN_interrupt,
};

static struct irqaction comcerto_timer4_irq = {
	.name		= "timer4",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= comcerto_timer4_interrupt,
	.dev_id		= &clock,
};

static struct irqaction comcerto_timer5_irq = {
	.name		= "timer5",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= comcerto_timerN_interrupt,
};

#if 0
static DEFINE_CLOCK_DATA(cd);

unsigned long long notrace sched_clock(void)
{
	u32 cyc = comcerto_timer2_get();

	return cyc_to_sched_clock(&cd, cyc, (u32)~0);
}

static void notrace comcerto_update_sched_clock(void)
{
	u32 cyc;

	cyc = comcerto_timer2_get();

	update_sched_clock(&cd, cyc, (u32)~0);
}
#endif
static u64 comcerto_update_sched_clock(void)
{
	return comcerto_timer2_get();
}

void __init comcerto_hwtimer_init(void)
{
	/*
	 * DO NOT MODIFY THE CONFIGURATION OF TIMER0
	 * It is used by the MSP
	 */
	
	struct clk *clk_axi;

	/* Initializing the clock structure Tree declared/defined in clock.c */
        clk_init();

	spin_lock_init(&comcerto_timer_lock);

	/* Mask all the timers except timer0 */
	comcerto_timer_disable(1);
	comcerto_timer_disable(2);
	comcerto_timer_disable(3);
	comcerto_timer_disable(4);
	comcerto_timer_disable(5);

	/* Get the AXI clock , to be used for AHB clock value*/
	clk_axi = clk_get(NULL,"axi");
	
	if (IS_ERR(clk_axi)){
		pr_err("%s: Unable to obtain axi clock: %ld\n",__func__,PTR_ERR(clk_axi));
		/* System cannot proceed from here */
		BUG();
	}

	/* Enable the AXI clock */
	if (clk_enable(clk_axi)){
		pr_err("%s: Unable to enable axi clock\n",__func__);
		/* System cannot proceed from here */
		BUG();
	}

	/* Get the AXI clock rate value , which will assigned to AHB clock value */
	COMCERTO_AHBCLK = clk_get_rate(clk_axi);	

	__comcerto_timer_set(2, 0xffffffff);
	clocksource.mult = clocksource_hz2mult(COMCERTO_AHBCLK, clocksource.shift);
	clocksource_register(&clocksource);

	//init_sched_clock(&cd, comcerto_update_sched_clock, 32, COMCERTO_AHBCLK);
	sched_clock_register(comcerto_update_sched_clock, 32, COMCERTO_AHBCLK);
	
	clock.device.mult = div_sc(COMCERTO_AHBCLK, NSEC_PER_SEC, clock.device.shift);
	clock.device.max_delta_ns = clockevent_delta2ns(0x3fffffff, &clock.device);
	clock.device.min_delta_ns = clockevent_delta2ns(1, &clock.device);
	clock.device.cpumask = cpumask_of(0);
	clockevents_register_device(&clock.device);

	/* Clear all the timers except timer0  */
	timer_writel(COMCERTO_TIMER_CSP, COMCERTO_TIMER_STATUS);

	/* Register interrupt handler for interrupt on IRQ_TIMERB*/
	irq_set_irq_type(IRQ_TIMER1, IRQ_TYPE_EDGE_RISING);
	irq_set_irq_type(IRQ_TIMER3, IRQ_TYPE_EDGE_RISING);
	irq_set_irq_type(IRQ_TIMER4, IRQ_TYPE_EDGE_RISING);
	irq_set_irq_type(IRQ_TIMER5, IRQ_TYPE_EDGE_RISING);
#ifndef CONFIG_PMUTIMER
	setup_irq(IRQ_TIMER1, &comcerto_timer1_irq);
	setup_irq(IRQ_TIMER3, &comcerto_timer3_irq);
#endif
	setup_irq(IRQ_TIMER4, &comcerto_timer4_irq);
	setup_irq(IRQ_TIMER5, &comcerto_timer5_irq);
}

#ifdef CONFIG_HAVE_ARM_TWD
static DEFINE_TWD_LOCAL_TIMER(twd_local_timer,
			      COMCERTO_TWD_VADDR,
			      IRQ_LOCALTIMER);

static void __init comcerto_twd_init(void)
{
	int err = twd_local_timer_register(&twd_local_timer);
	if (err)
		pr_err("twd_local_timer_register failed %d\n", err);
}
#else
#define comcerto_twd_init()	do { } while(0)
#endif

void __init comcerto_timer_init(void)
{

	comcerto_hwtimer_init();
	comcerto_twd_init();
}
/*
struct sys_timer comcerto_timer = {
	.init	= comcerto_timer_init,
};*/

