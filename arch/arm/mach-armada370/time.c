/*
 * arch/arm/mach-armadaxp/time.c
 *
 * Marvell Aurora SoC timer handling.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * Timer 0 is used as free-running clocksource, while timer 1 is
 * used as clock_event_device.
 */

#include <linux/kernel.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <asm/localtimer.h>
#include <asm/sched_clock.h>

#include "boardEnv/mvBoardEnvLib.h"
#include "cpu/mvCpu.h"

extern void axp_irq_mask(struct irq_data *d);
extern void axp_irq_unmask(struct irq_data *d);

#define  TIMER_CTRL		(MV_CNTMR_REGS_OFFSET + 0x0000)
#define  TIMER0_EN		0x0001
#define  TIMER0_RELOAD_EN	0x0002
#define  TIMER1_EN		0x0004
#define  TIMER1_RELOAD_EN	0x0008
#define  TIMER0_RELOAD		(MV_CNTMR_REGS_OFFSET + 0x0010)
#define  TIMER0_VAL		(MV_CNTMR_REGS_OFFSET + 0x0014)
#define  TIMER1_RELOAD		(MV_CNTMR_REGS_OFFSET + 0x0018)
#define  TIMER1_VAL		(MV_CNTMR_REGS_OFFSET + 0x001c)
#define  TIMER_WD_RELOAD	(MV_CNTMR_REGS_OFFSET + 0x0020)
#define  TIMER_WD_VAL		(MV_CNTMR_REGS_OFFSET + 0x0024)
#define  TIMER_CAUSE		(MV_CNTMR_REGS_OFFSET + 0x0028)
#define  INT_TIMER0_CLR 	~(1 << 0)
#define  INT_TIMER1_CLR 	~(1 << 8)

#define  LCL_TIMER_BASE		(0x21000 | 0x40)
#define  LCL_TIMER_CTRL		(LCL_TIMER_BASE + 0x0000)
#define  LCL_TIMER0_EN		0x0001
#define  LCL_TIMER0_RELOAD_EN	0x0002
#define  LCL_TIMER1_EN		0x0004
#define  LCL_TIMER1_RELOAD_EN	0x0008
#define  LCL_TIMER0_RELOAD	(LCL_TIMER_BASE + 0x0010)
#define  LCL_TIMER0_VAL		(LCL_TIMER_BASE + 0x0014)
#define  LCL_TIMER1_RELOAD	(LCL_TIMER_BASE + 0x0018)
#define  LCL_TIMER1_VAL		(LCL_TIMER_BASE + 0x001c)
#define  LCL_TIMER_WD_RELOAD	(LCL_TIMER_BASE + 0x0020)
#define  LCL_TIMER_WD_VAL	(LCL_TIMER_BASE + 0x0024)
#define  LCL_TIMER_CAUSE	(LCL_TIMER_BASE + 0x0028)
#define  LCL_INT_TIMER0_CLR 	~(1 << 0)
#define  LCL_INT_TIMER1_CLR	~(1 << 8)

#define BRIDGE_CAUSE		(MV_MBUS_REGS_OFFSET | 0x0260)
#define BRIDGE_INT_TIMER0	(1 << 24)
#define BRIDGE_INT_TIMER1	(1 << 25)
#define BRIDGE_MASK		(MV_MBUS_REGS_OFFSET | 0x10c4)

static DEFINE_CLOCK_DATA(cd);

/*
 * Number of timer ticks per jiffy.
 */
static u32 ticks_per_jiffy;

/*
 * Clocksource handling.
 */
static cycle_t axp_clksrc_read(struct clocksource *cs)
{
	return (0xffffffff - MV_REG_READ(TIMER0_VAL));
}

static struct clocksource axp_clksrc = {
	.name		= "armada370_clocksource",
	.shift		= 20,
	.rating		= 300,
	.read		= axp_clksrc_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

/*
 * Clockevent handling.
 */
int axp_clkevt_next_event(unsigned long delta, struct clock_event_device *dev)
{
	unsigned long flags;
	u32 u;

	if (delta == 0)
		return -ETIME;

	local_irq_save(flags);

	/* Clear and enable clockevent timer interrupt */
	MV_REG_WRITE(LCL_TIMER_CAUSE, LCL_INT_TIMER0_CLR);
	/*axp_irq_unmask(IRQ_LOCALTIMER);*/
	axp_irq_unmask(irq_get_irq_data(IRQ_LOCALTIMER));

	/* Setup new clockevent timer value */
	MV_REG_WRITE(LCL_TIMER0_VAL, delta);

	/* Enable the timer */
	u = MV_REG_READ(LCL_TIMER_CTRL);
	u = (u & ~LCL_TIMER0_RELOAD_EN) | LCL_TIMER0_EN;
	MV_REG_WRITE(LCL_TIMER_CTRL, u);

	local_irq_restore(flags);

	return 0;
}

static void axp_clkevt_mode(enum clock_event_mode mode, struct clock_event_device *dev)
{
	unsigned long flags;
	u32 u;

	local_irq_save(flags);

	if (mode == CLOCK_EVT_MODE_PERIODIC) {
		/* Setup timer to fire at 1/HZ intervals */
		MV_REG_WRITE(LCL_TIMER0_RELOAD, (ticks_per_jiffy - 1));
		MV_REG_WRITE(LCL_TIMER0_VAL, (ticks_per_jiffy - 1));

		/* Enable timer interrupt */
		/*axp_irq_unmask(IRQ_LOCALTIMER);*/
		axp_irq_unmask(irq_get_irq_data(IRQ_LOCALTIMER));

		/* Enable timer */
		u = MV_REG_READ(LCL_TIMER_CTRL);
		u |= (LCL_TIMER0_EN | LCL_TIMER0_RELOAD_EN);
		MV_REG_WRITE(LCL_TIMER_CTRL, u);
	} else {
		/* Disable timer */
		u = MV_REG_READ(LCL_TIMER_CTRL);
		u &= ~LCL_TIMER0_EN;
		MV_REG_WRITE(LCL_TIMER_CTRL, u);
		/* Disable timer interrupt */
		axp_irq_mask(irq_get_irq_data(IRQ_LOCALTIMER));

		/* ACK pending timer interrupt */
		MV_REG_WRITE(LCL_TIMER_CAUSE, LCL_INT_TIMER0_CLR);
	}

	local_irq_restore(flags);
}

static struct clock_event_device axp_clkevt;
static irqreturn_t axp_timer_interrupt(int irq, void *dev_id)
{
	/* ACK timer interrupt and call event handler */
	MV_REG_WRITE(LCL_TIMER_CAUSE, LCL_INT_TIMER0_CLR);
	axp_clkevt.event_handler(&axp_clkevt);

	return IRQ_HANDLED;
}

static struct irqaction axp_timer_irq = {
	.name		= "armada370_tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= axp_timer_interrupt
};

/*
 * Setup the local clock events for a CPU.
 */
void __cpuinit mv_timer_setup(struct clock_event_device *clk, unsigned int fabric_clk)
{
	unsigned int cpu = smp_processor_id();

	clk->features		= (CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC),
	clk->shift		= 32,
	clk->rating		= 300,
	clk->set_next_event	= axp_clkevt_next_event,
	clk->set_mode		= axp_clkevt_mode,
	clk->cpumask		= cpumask_of(cpu);
	clk->mult		= div_sc(fabric_clk, NSEC_PER_SEC, clk->shift);
	clk->max_delta_ns	= clockevent_delta2ns(0xffffffff, clk);
	clk->min_delta_ns	= clockevent_delta2ns(0x1, clk);
}

unsigned long long notrace sched_clock(void)
{
	u32 cyc = ~MV_REG_READ(TIMER0_VAL);
	return cyc_to_sched_clock(&cd, cyc, (u32)~0);
}

static void notrace a370_update_sched_clock(void)
{
	u32 cyc = ~MV_REG_READ(TIMER0_VAL);
	update_sched_clock(&cd, cyc, (u32)~0);
}

static void __init setup_sched_clock(unsigned long tclk)
{
	init_sched_clock(&cd, a370_update_sched_clock, 32, tclk);
}

void __init axp_time_init(unsigned int fabric_clk)
{
	u32 u;

	printk("a370_time_init\n");

	ticks_per_jiffy = (fabric_clk + HZ/2) / HZ;

	setup_sched_clock(fabric_clk);

	/* Setup free-running clocksource timer (interrupts disabled) */
	MV_REG_WRITE(TIMER0_VAL, 0xffffffff);
	MV_REG_WRITE(TIMER0_RELOAD, 0xffffffff);
	u = MV_REG_READ(BRIDGE_MASK);
	u &= ~BRIDGE_INT_TIMER0;
	MV_REG_WRITE(BRIDGE_MASK, u);
	u = MV_REG_READ(TIMER_CTRL);
	u |= (TIMER0_EN | TIMER0_RELOAD_EN);
	MV_REG_WRITE(TIMER_CTRL, u);
	axp_clksrc.mult = clocksource_hz2mult(fabric_clk, axp_clksrc.shift);
	clocksource_register(&axp_clksrc);

	/* Setup clockevent timer (interrupt-driven) */
	axp_clkevt.name = "armada370_tick";
	axp_clkevt.irq = IRQ_LOCALTIMER;
	mv_timer_setup(&axp_clkevt, fabric_clk);
	setup_irq(IRQ_LOCALTIMER, &axp_timer_irq);
	clockevents_register_device(&axp_clkevt);
}

static void axp_timer_init(void)
{
	axp_time_init(mvCpuL2ClkGet());
}

struct sys_timer axp_timer = {
	.init = axp_timer_init,
};
