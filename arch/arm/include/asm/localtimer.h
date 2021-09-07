 
#ifndef __ASM_ARM_LOCALTIMER_H
#define __ASM_ARM_LOCALTIMER_H

struct clock_event_device;

void percpu_timer_setup(void);

asmlinkage void do_local_timer(struct pt_regs *);

#ifdef CONFIG_SYNO_PLX_PORTING
#include "smp_twd.h"

#define local_timer_ack()	twd_timer_ack()
#define local_timer_stop()	twd_timer_stop()

void local_timer_setup(struct clock_event_device *);
#else

#ifdef CONFIG_LOCAL_TIMERS

#ifdef CONFIG_HAVE_ARM_TWD

#include "smp_twd.h"

#define local_timer_ack()	twd_timer_ack()
#define local_timer_stop()	twd_timer_stop()

#else

int local_timer_ack(void);

void local_timer_stop(void);

#endif

void local_timer_setup(struct clock_event_device *);

#else

static inline void local_timer_stop(void)
{
}

#endif

#endif
#endif
