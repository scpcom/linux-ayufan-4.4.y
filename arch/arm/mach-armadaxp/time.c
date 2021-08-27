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
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <asm/localtimer.h>
#include <asm/sched_clock.h>

#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "boardEnv/mvBoardEnvLib.h"
#include "cpu/mvCpu.h"

#ifdef CONFIG_SMP
static struct clock_event_device __percpu ** axp_local_clockevent;
#endif

extern void axp_irq_mask(struct irq_data *d);
extern void axp_irq_unmask(struct irq_data *d);
extern unsigned int master_cpu_id;

#define  TIMER_CTRL		(MV_CNTMR_REGS_OFFSET + 0x0000)
#define  TIMER_WD_RELOAD	(MV_CNTMR_REGS_OFFSET + 0x0020)
#define  TIMER_WD_VAL		(MV_CNTMR_REGS_OFFSET + 0x0024)
#define  TIMER_CAUSE		(MV_CNTMR_REGS_OFFSET + 0x0028)

#define  TIMER_EN(x)		(0x0001 << (2 * x))
#define  TIMER_RELOAD_EN(x)	(0x0002 << (2 * x))
#define  TIMER_RELOAD(x)	(MV_CNTMR_REGS_OFFSET + 0x0010 + (8 * x))
#define  TIMER_VAL(x)		(MV_CNTMR_REGS_OFFSET + 0x0014 + (8 * x))
#define  INT_TIMER_CLR(x)	(~(1 << (8*x)))

#define  LCL_TIMER_BASE		(0x21000 | 0x40)
#define  LCL_TIMER_CTRL		(LCL_TIMER_BASE + 0x0000)
#define    LCL_TIMER0_EN		0x0001
#define    LCL_TIMER0_RELOAD_EN		0x0002
#define    LCL_TIMER1_EN		0x0004
#define    LCL_TIMER1_RELOAD_EN		0x0008
#define  LCL_TIMER0_RELOAD	(LCL_TIMER_BASE + 0x0010)
#define  LCL_TIMER0_VAL		(LCL_TIMER_BASE + 0x0014)
#define  LCL_TIMER1_RELOAD	(LCL_TIMER_BASE + 0x0018)
#define  LCL_TIMER1_VAL		(LCL_TIMER_BASE + 0x001c)
#define  LCL_TIMER_WD_RELOAD	(LCL_TIMER_BASE + 0x0020)
#define  LCL_TIMER_WD_VAL	(LCL_TIMER_BASE + 0x0024)
#define  LCL_TIMER_CAUSE	(LCL_TIMER_BASE + 0x0028)
#define   LCL_INT_TIMER0_CLR 	~(1 << 0)
#define   LCL_INT_TIMER1_CLR	~(1 << 8)
#define LCL_TIMER_TURN_25MHZ	(1 << 11)

#define TIMER_TURN_25MHZ(x)	(1 << (11 + x))
#define BRIDGE_CAUSE		(MV_MBUS_REGS_OFFSET | 0x0260)
#define BRIDGE_MASK		(MV_MBUS_REGS_OFFSET | 0x10c4)
#define BRIDGE_INT_TIMER(x)	(1 << (24 + x))

/*
 * Number of timer ticks per jiffy.
 */
static u32 ticks_per_jiffy;
static unsigned int soc_timer_id;

static DEFINE_CLOCK_DATA(cd);

unsigned long long notrace sched_clock(void)
{
	u32 cyc = ~MV_REG_READ(TIMER_VAL(soc_timer_id));
	return cyc_to_sched_clock(&cd, cyc, (u32)~0);
}

static void notrace axp_update_sched_clock(void)
{
	u32 cyc = ~MV_REG_READ(TIMER_VAL(soc_timer_id));
	update_sched_clock(&cd, cyc, (u32)~0);
}

static void __init setup_sched_clock(unsigned long tclk)
{
	init_sched_clock(&cd, axp_update_sched_clock, 32, tclk);
}

/*
 * Clocksource handling.
 */
static cycle_t axp_clksrc_read(struct clocksource *cs)
{
	return (0xffffffff - MV_REG_READ(TIMER_VAL(soc_timer_id)));
}

static struct clocksource axp_clksrc = {
	.name		= "axp_clocksource",
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

	if ((mode == CLOCK_EVT_MODE_PERIODIC) ||
	    (mode == CLOCK_EVT_MODE_ONESHOT)) {
		/* Setup timer to fire at 1/HZ intervals */
		MV_REG_WRITE(LCL_TIMER0_RELOAD, (ticks_per_jiffy - 1));
		MV_REG_WRITE(LCL_TIMER0_VAL, (ticks_per_jiffy - 1));

		/* Enable timer interrupt */
		axp_irq_unmask(irq_get_irq_data(IRQ_LOCALTIMER));

		/* Enable timer */
		u = MV_REG_READ(LCL_TIMER_CTRL);
#if !defined (CONFIG_ARMADA_XP_REV_Z1) && !defined (CONFIG_MACH_ARMADA_XP_FPGA)
		if(mode == CLOCK_EVT_MODE_PERIODIC)
			u |= (LCL_TIMER0_EN | LCL_TIMER0_RELOAD_EN | LCL_TIMER_TURN_25MHZ);
		else
			u |= (LCL_TIMER0_EN | LCL_TIMER_TURN_25MHZ);
#else
		u |= (LCL_TIMER0_EN | LCL_TIMER0_RELOAD_EN);
#endif
		MV_REG_WRITE(LCL_TIMER_CTRL, u);
	} else {
		/* Disable timer */
		u = MV_REG_READ(LCL_TIMER_CTRL);
		u &= ~LCL_TIMER0_EN;
		MV_REG_WRITE(LCL_TIMER_CTRL, u);

		/* Disable timer interrupt */
		//axp_irq_mask(IRQ_LOCALTIMER);
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
	.name		= "axp_tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= axp_timer_interrupt,
	.dev_id         = &axp_clkevt,
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

/*
 * Resume timer from suspend to RAM
 * TODO - need to implement kernel hooks for suspend/resume
 */
void axp_timer_resume(void)
{
	u32 u;

	pr_info("Resuming ArmadaXP SOC Timer %d\n", soc_timer_id);

	MV_REG_WRITE(TIMER_VAL(soc_timer_id), 0xffffffff);
	MV_REG_WRITE(TIMER_RELOAD(soc_timer_id), 0xffffffff);

	u = MV_REG_READ(BRIDGE_MASK);
	u &= ~BRIDGE_INT_TIMER(soc_timer_id);
	MV_REG_WRITE(BRIDGE_MASK, u);

	u = MV_REG_READ(TIMER_CTRL);
	u |= (TIMER_EN(soc_timer_id) | TIMER_RELOAD_EN(soc_timer_id) |
			TIMER_TURN_25MHZ(soc_timer_id));
	MV_REG_WRITE(TIMER_CTRL, u);
}

void __init axp_time_init(unsigned int fabric_clk)
{
	u32 u;

#ifdef CONFIG_MV_AMP_ENABLE
	soc_timer_id = (master_cpu_id == 0 ? 0 : 3);
#else
	soc_timer_id = 0;
#endif

	printk("Initializing ArmadaXP SOC Timer %d\n", soc_timer_id);

	ticks_per_jiffy = (fabric_clk + HZ/2) / HZ;
	
	setup_sched_clock(fabric_clk);

	/* Setup free-running clocksource timer (interrupts disabled) */
	MV_REG_WRITE(TIMER_VAL(soc_timer_id), 0xffffffff);
	MV_REG_WRITE(TIMER_RELOAD(soc_timer_id), 0xffffffff);
	u = MV_REG_READ(BRIDGE_MASK);
	u &= ~BRIDGE_INT_TIMER(soc_timer_id);
	MV_REG_WRITE(BRIDGE_MASK, u);
	u = MV_REG_READ(TIMER_CTRL);
#if !defined (CONFIG_ARMADA_XP_REV_Z1) && !defined (CONFIG_MACH_ARMADA_XP_FPGA)
	u |= (TIMER_EN(soc_timer_id) | TIMER_RELOAD_EN(soc_timer_id) | TIMER_TURN_25MHZ(soc_timer_id));
#else
	u |= (TIMER_EN(soc_timer_id) | TIMER_RELOAD_EN(soc_timer_id));
#endif
	MV_REG_WRITE(TIMER_CTRL, u);
	axp_clksrc.mult = clocksource_hz2mult(fabric_clk, axp_clksrc.shift);
	clocksource_register(&axp_clksrc);

#ifdef CONFIG_SMP
	{
		percpu_timer_setup();
	        return;
	}
#endif
	/* Setup clockevent timer (interrupt-driven) */
	axp_clkevt.name = "axp_tick";
	axp_clkevt.irq = IRQ_LOCALTIMER;
	mv_timer_setup(&axp_clkevt, fabric_clk);
	setup_irq(IRQ_LOCALTIMER, &axp_timer_irq);
	clockevents_register_device(&axp_clkevt);
}

static void axp_timer_init(void)
{
#if !defined (CONFIG_ARMADA_XP_REV_Z1) || defined (CONFIG_MACH_ARMADA_XP_FPGA)
	/* FPGA is hardcoded to 25Mhx and DSMP-A0 ref clock for the timers is 25MHz */
	axp_time_init(25000000);
#else
	axp_time_init(mvCpuL2ClkGet());  /* DSMP-Z1 clock is taken from Fabric */
#endif
}

struct sys_timer axp_timer = {
	.init = axp_timer_init,
};

#if defined (CONFIG_SMP) && defined (CONFIG_LOCAL_TIMERS)
/*
 * Used on SMP for either the local timer or IPI_TIMER
 */
/*void local_timer_interrupt(void)
{
	struct clock_event_device *clk = &__get_cpu_var(axp_local_clockevent);

	clk->event_handler(clk);
}
*/

/*
 * local_timer_ack: checks for a local timer interrupt.
 *
 * If a local timer interrupt has occurred, acknowledge and return 1.
 * Otherwise, return 0.
 */

int local_timer_ack(void)
{
	if(MV_REG_READ(LCL_TIMER_CAUSE) & ~LCL_INT_TIMER0_CLR) {
		MV_REG_WRITE(LCL_TIMER_CAUSE, LCL_INT_TIMER0_CLR);
		return 1;
	}
	return 0;
}

static irqreturn_t axp_localtimer_handler(int irq, void *dev_id)
{

	struct clock_event_device *evt = *(struct clock_event_device **)dev_id;
	if (local_timer_ack()) {
		evt->event_handler(evt);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/*
 * Setup the local clock events for a CPU.
 */
int __cpuinit local_timer_setup(struct clock_event_device *clk)
{
#if !defined (CONFIG_ARMADA_XP_REV_Z1) || defined (CONFIG_MACH_ARMADA_XP_FPGA)
	/* FPGA hardcoded to 25Mhz and in DSMP-A0 the referance clock for the timers is 25MHz */
	unsigned int fabric_clk = 25000000;
#else
	unsigned int fabric_clk = mvCpuL2ClkGet(); /* DSMP-Z1 clock is taken from Fabric */
#endif
	static int cpu0_flag = 0;
	int cpu = smp_processor_id();
	struct clock_event_device **this_cpu_clk;

	if (!axp_local_clockevent) {
		int err;

		axp_local_clockevent = alloc_percpu(struct clock_event_device *);
		if (!axp_local_clockevent) {
			pr_err("axp_local_clockevent: can't allocate memory\n");
			return 0;
		}
		err = request_percpu_irq(IRQ_LOCALTIMER, axp_localtimer_handler,
				"axp_local_clockevent", axp_local_clockevent);
		if (err) {
			pr_err("axp_local_clockevent: can't register interrupt %d (%d)\n",
				IRQ_LOCALTIMER, err);
			return 0;
		}
	}

	if((cpu) || (!cpu && !cpu0_flag)){
		ticks_per_jiffy = (fabric_clk + HZ/2) / HZ;
		clk->name = "local_timer";
		clk->irq = IRQ_LOCALTIMER;
		mv_timer_setup(clk, fabric_clk);
		this_cpu_clk = __this_cpu_ptr(axp_local_clockevent);
		*this_cpu_clk = clk;
		clockevents_register_device(clk);
	 if(!cpu)
		cpu0_flag++;
	}
	enable_percpu_irq(clk->irq, 0);
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
/*
 * take a local timer down
 */
void  __cpuexit local_timer_stop(struct clock_event_device * evt)
{
	unsigned long flags;
	u32 u;
	local_irq_save(flags);

	/* Disable timer */
	u = MV_REG_READ(LCL_TIMER_CTRL);
	u &= ~LCL_TIMER0_EN;
	MV_REG_WRITE(LCL_TIMER_CTRL, u);
	MV_REG_WRITE(LCL_TIMER_CAUSE, LCL_INT_TIMER0_CLR);
	/* Disable timer interrupt */
	/*axp_irq_mask(IRQ_LOCALTIMER);*/
	axp_irq_mask(irq_get_irq_data(IRQ_LOCALTIMER));

	local_irq_restore(flags);
}
#endif
#endif	/* CONFIG_LOCAL_TIMERS && CONFIG_SMP */
