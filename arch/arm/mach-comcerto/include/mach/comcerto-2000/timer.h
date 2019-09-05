/*
 *  linux/include/asm-arm/arch-comcerto/comcerto-1000/timer.h
 *
 *  Copyright (C) 2004-2008 Mindspeed Technologies, Inc.
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

#ifndef __TIMER_H__
#define __TIMER_H__


/* Comcerto Timers  */
#define COMCERTO_TIMER0_HIGH_BOUND          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x00)
#define COMCERTO_TIMER0_CURRENT_COUNT       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x04)
#define COMCERTO_TIMER1_HIGH_BOUND          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x08)
#define COMCERTO_TIMER1_CURRENT_COUNT       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x0C)
#define COMCERTO_TIMER2_LOW_BOUND           APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x10)
#define COMCERTO_TIMER2_HIGH_BOUND          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x14)
#define COMCERTO_TIMER2_CTRL                APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x18)
#define COMCERTO_TIMER2_CURRENT_COUNT       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x1C)
#define COMCERTO_TIMER3_LOW_BOUND           APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x20)
#define COMCERTO_TIMER3_HIGH_BOUND          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x24)
#define COMCERTO_TIMER3_CTRL                APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x28)
#define COMCERTO_TIMER3_CURRENT_COUNT       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x2C)
#define COMCERTO_TIMER4_HIGH_BOUND          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x30)
#define COMCERTO_TIMER4_CURRENT_COUNT       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x34)
#define COMCERTO_TIMER5_LOW_BOUND           APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x38)
#define COMCERTO_TIMER5_HIGH_BOUND          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x3C)
#define COMCERTO_TIMER5_CURRENT_COUNT       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x40)
#define COMCERTO_TIMER5_CTRL                APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x44)
#define COMCERTO_TIMER_IRQ_MASK             APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x48)
#define COMCERTO_TIMER_STATUS               APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x50)
#define COMCERTO_TIMER_STATUS_CLR           APB_VADDR(COMCERTO_APB_TIMER_BASE + 0x50)
#define COMCERTO_TIMER_WDT_HIGH_BOUND       APB_VADDR(COMCERTO_APB_TIMER_BASE + 0xD0)
#define COMCERTO_TIMER_WDT_CONTROL          APB_VADDR(COMCERTO_APB_TIMER_BASE + 0xD4)
#define COMCERTO_TIMER_WDT_CURRENT_COUNT    APB_VADDR(COMCERTO_APB_TIMER_BASE + 0xD8)

#define COMCERTO_TIMER_WDT_CONTROL_TIMER_ENABLE		(1 << 0)

/*COMCERTO_TIMER_IRQ_MASK*/
#define COMCERTO_TIMER0     0x01
#define COMCERTO_TIMER1     0x02
#define COMCERTO_TIMER2     0x04
#define COMCERTO_TIMER3     0x08
#define COMCERTO_TIMER4     0x10
#define COMCERTO_TIMER5     0x20
#define COMCERTO_ALL        0xFF
#define COMCERTO_TIMER_CSP  (COMCERTO_TIMER1 | COMCERTO_TIMER2 | COMCERTO_TIMER3 | COMCERTO_TIMER4 | COMCERTO_TIMER5)

/*
 * TIMERS
 */


/*Hardware Timer API*/
#define COMCERTO_TIMER_RUN_ONCE		(1 << 0)
#define __comcerto_timer_enable(t)	__raw_writel(__raw_readl(COMCERTO_TIMER_IRQ_MASK) | (1 << (t)), COMCERTO_TIMER_IRQ_MASK)
#define __comcerto_timer_disable(t)	__raw_writel(__raw_readl(COMCERTO_TIMER_IRQ_MASK) & ~(1 << (t)), COMCERTO_TIMER_IRQ_MASK)
#define comcerto_timer_ack(t)		__raw_writel(1 << (t), COMCERTO_TIMER_STATUS_CLR)

#define comcerto_timer0_set(hbound)	__raw_writel((hbound), COMCERTO_TIMER0_HIGH_BOUND)
#define comcerto_timer0_get()		__raw_readl(COMCERTO_TIMER0_CURRENT_COUNT)

#define comcerto_timer1_set(hbound)	__raw_writel((hbound), COMCERTO_TIMER1_HIGH_BOUND)
#define comcerto_timer1_get()		__raw_readl(COMCERTO_TIMER1_CURRENT_COUNT)

#define comcerto_timer2_set(lbound, hbound, ctrl)  do {\
						      __raw_writel((ctrl) & 0x1, COMCERTO_TIMER2_CTRL);	\
						      __raw_writel((lbound), COMCERTO_TIMER2_LOW_BOUND);	\
						      __raw_writel((hbound), COMCERTO_TIMER2_HIGH_BOUND);	\
						   } while (0)

#define comcerto_timer2_get()		__raw_readl(COMCERTO_TIMER2_CURRENT_COUNT)


#define comcerto_timer3_set(lbound, hbound, ctrl)  do {								\
						      __raw_writel((ctrl) & 0x1, COMCERTO_TIMER3_CTRL);	\
						      __raw_writel((lbound), COMCERTO_TIMER3_LOW_BOUND);	\
						      __raw_writel((hbound), COMCERTO_TIMER3_HIGH_BOUND);	\
						   } while(0)

#define comcerto_timer3_get()		__raw_readl(COMCERTO_TIMER3_CURRENT_COUNT)

#ifndef __ASSEMBLY__
struct comcerto_timer {
	unsigned long timeout;
	void (*func) (unsigned long data);
	unsigned long data;
	unsigned char flags;
	unsigned long thw;
};

struct timer_hw {
	u8 id;
	u8 status;
	struct comcerto_timer *t;
};


typedef enum _timer_chain_id {
	timer_0_2 = 0,
	timer_1_3,
	timer_4_5,
} timer_chain_id ;


typedef struct _timer_chain_pair {
	u8 chainid_0;
	u8 chainid_1;
} timer_chain_pair;


/* exports */
extern int comcerto_timer_chain_run(timer_chain_id chain_id, u32 granularity, u32 expiry);
extern int comcerto_timer_chain_kill(timer_chain_id chain_id);

#endif
#endif
