/*
 * arch/arm/mach-armada38x/time.c
 *
 * Marvell SoC timer handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <ca9x2.h>
#include <asm/localtimer.h>
#include <asm/sched_clock.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/smp_twd.h>

#include "boardEnv/mvBoardEnvLib.h"
#include "cpu/mvCpu.h"

/* SoC global timer */
#define TIMER_CTRL		(MV_CNTMR_REGS_OFFSET + 0x0000)
#define  TIMER_EN(x)		(1 << (2 * x))
#define  TIMER_RELOAD_EN(x)	(2 << (2 * x))
#define TIMER_CAUSE		(MV_CNTMR_REGS_OFFSET + 0x0004)
#define  TIMER_INT_CLR(x)	(~(1 << (8 * x)))
#define TIMER_RELOAD(x)		(MV_CNTMR_REGS_OFFSET + 0x0010 + (8 * x))
#define TIMER_VAL(x)		(MV_CNTMR_REGS_OFFSET + 0x0014 + (8 * x))
#define TIMER_TURN_25MHZ(x)	(1 << (11 + x))

/*
 * Define SoC global timers to be used for event and source timers
 */
static int event_timer_id;
static int source_timer_id;

static u32 ticks_per_jiffy;

static cycle_t a38x_clksrc_read(struct clocksource *cs)
{
	return 0xffffffff - MV_REG_READ(TIMER_VAL(source_timer_id));
}

static struct clocksource a38x_clksrc = {
	.name = "a38x_clocksource",
	.shift = 20,
	.rating = 300,
	.read = a38x_clksrc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

int a38x_clkevt_next_event(unsigned long delta, struct clock_event_device *evt)
{
	unsigned long flags;
	u32 u, t = event_timer_id;

	if (delta == 0)
		return -ETIME;

	local_irq_save(flags);

	/* Clear and enable clockevent timer interrupt */
	MV_REG_WRITE(TIMER_CAUSE, TIMER_INT_CLR(t));

	/* Setup new clockevent timer value */
	MV_REG_WRITE(TIMER_VAL(t), delta);

	/* Enable the timer */
	u = MV_REG_READ(TIMER_CTRL);
	u = (u & ~TIMER_RELOAD_EN(t)) | TIMER_EN(t) | TIMER_TURN_25MHZ(t);
	MV_REG_WRITE(TIMER_CTRL, u);

	local_irq_restore(flags);
	return 0;
}

static void a38x_clkevt_mode(enum clock_event_mode mode,
			     struct clock_event_device *evt)
{
	unsigned long flags;
	u32 u, t = event_timer_id;
	local_irq_save(flags);

	if (mode == CLOCK_EVT_MODE_PERIODIC || mode == CLOCK_EVT_MODE_ONESHOT) {
		/* Setup timer to fire at 1/HZ intervals */
		MV_REG_WRITE(TIMER_RELOAD(t), ticks_per_jiffy - 1);
		MV_REG_WRITE(TIMER_VAL(t), ticks_per_jiffy - 1);

		/* Enable timer */
		u = MV_REG_READ(TIMER_CTRL);
		u |= TIMER_EN(t) | TIMER_RELOAD_EN(t) | TIMER_TURN_25MHZ(t);
		MV_REG_WRITE(TIMER_CTRL, u);
	} else {
		/* Disable timer */
		u = MV_REG_READ(TIMER_CTRL);
		u &= ~TIMER_EN(t);
		MV_REG_WRITE(TIMER_CTRL, u);

		/* Ack pending timer interrupt */
		MV_REG_WRITE(TIMER_CAUSE, TIMER_INT_CLR(t));
	}

	local_irq_restore(flags);
}

static struct clock_event_device a38x_clkevt;

static irqreturn_t a38x_timer_interrupt(int irq, void *dev_id)
{
	u32 t = event_timer_id;

	/* Ack timer interrupt */
	MV_REG_WRITE(TIMER_CAUSE, TIMER_INT_CLR(t));

	a38x_clkevt.event_handler(&a38x_clkevt);
	return IRQ_HANDLED;
}

static struct irqaction a38x_timer_irq = {
	.name = "a38x_clk_evt",
	.flags = IRQF_DISABLED | IRQF_TIMER,
	.handler = a38x_timer_interrupt,
	.dev_id = &a38x_clkevt,
};

/*
 * Implement clock API.
 */
int clk_enable(struct clk *clk)
{
	return 0;
}

void clk_disable(struct clk *clk)
{
	/* Empty */
}

static DEFINE_CLOCK_DATA(cd);

unsigned long long notrace sched_clock(void)
{
	u32 cyc = ~MV_REG_READ(TIMER_VAL(source_timer_id));
	return cyc_to_sched_clock(&cd, cyc, (u32)~0);
}

static void notrace a38x_update_sched_clock(void)
{
	u32 cyc = ~MV_REG_READ(TIMER_VAL(source_timer_id));
	update_sched_clock(&cd, cyc, (u32)~0);
}

static void __init setup_sched_clock(unsigned long tclk)
{
	init_sched_clock(&cd, a38x_update_sched_clock, 32, tclk);
}

#ifdef CONFIG_MV_INCLUDE_SDIO
static struct clk_lookup clk_sdhci_lookup = {
	.dev_id = "sdhci-pxav3",
	.con_id = "PXA-SDHCLK",
};
#endif

static void __init a38x_sdhci_clk_init(void)
{
#ifdef CONFIG_MV_INCLUDE_SDIO
	clkdev_add(&clk_sdhci_lookup);
#endif
}

/* Setup free-running clocksource timer */
static void a38x_setup_clocksource(int timer, long rate)
{
	u32 i = timer, u;
	void __iomem *base =
	    (void __iomem *)(INTER_REGS_VIRT_BASE + TIMER_VAL(i));

	MV_REG_WRITE(TIMER_VAL(i), 0xffffffff);
	MV_REG_WRITE(TIMER_RELOAD(i), 0xffffffff);

	/* Config clock source for timer */
	u = MV_REG_READ(TIMER_CTRL);
	u |= TIMER_EN(i) | TIMER_RELOAD_EN(i) | TIMER_TURN_25MHZ(i);
	MV_REG_WRITE(TIMER_CTRL, u);

	clocksource_mmio_init(base, "a38x_clk_source",
			      rate, 200, 32, clocksource_mmio_readl_down);

	a38x_clksrc.mult = clocksource_hz2mult(rate, a38x_clksrc.shift);
	setup_sched_clock(rate);
	clocksource_register(&a38x_clksrc);
}

static void a38x_setup_clockevent(int irq, long rate)
{
	struct clock_event_device *evt = &a38x_clkevt;
	unsigned int cpu = smp_processor_id();

	evt->name = "a38x_clkevt";
	evt->irq = irq;
	evt->features = (CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC),
	    evt->shift = 32,
	    evt->rating = 300,
	    evt->set_next_event = a38x_clkevt_next_event,
	    evt->set_mode = a38x_clkevt_mode, evt->cpumask = cpumask_of(cpu);
	evt->mult = div_sc(rate, NSEC_PER_SEC, evt->shift);
	evt->max_delta_ns = clockevent_delta2ns(0xffffffff, evt);
	evt->min_delta_ns = clockevent_delta2ns(0x1, evt);

	setup_irq(irq, &a38x_timer_irq);
	clockevents_config_and_register(evt, rate, 0x1, 0xffffffff);
}

static void a38x_clear_timer_config(void)
{
	MV_REG_WRITE(TIMER_CTRL, 0);
	MV_REG_WRITE(TIMER_CAUSE, 0);
}

static void __init a38x_timer_init(void)
{
	u32 rate;

	rate = 25000000;

	printk(KERN_INFO "Initializing Armada-38x SoC Timers\n");
	ticks_per_jiffy = (rate + HZ / 2) / HZ;

	a38x_clear_timer_config();

	/* Define timers used for event and source */
	event_timer_id = 1;
	source_timer_id = 0;

	a38x_setup_clocksource(source_timer_id, rate);
	a38x_setup_clockevent(IRQ_GLOBAL_TIMER(event_timer_id), rate);

#ifdef CONFIG_HAVE_ARM_TWD
	twd_base = (INTER_REGS_VIRT_BASE + A9_MPCORE_TWD);
#endif
	a38x_sdhci_clk_init();
}

struct sys_timer a38x_timer = {
	.init = a38x_timer_init,
};
