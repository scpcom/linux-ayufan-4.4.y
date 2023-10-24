/*
*  Copyright (c) 2012 Mindspeed Technologies, Inc.
*
*  THIS FILE IS CONFIDENTIAL.
*
*  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
*
*  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
*/

#ifndef _TIMER_H_
#define _TIMER_H_


#include "system.h"

#if defined(COMCERTO_2000)

#define TIMER_CYCLES_PER_WRAP   max_cycle_count
/* AHB clock for c2k is 250MHz */
#define AHB_CLK                 4 /* one clock cycle in ns*/
#define TIMER_CYCLES_PER_USEC   (1000/AHB_CLK)
#define TIMER_CYCLES_PER_MSEC   (1000000/AHB_CLK) //(TIMER_CYCLES_PER_USEC * 1000)
#define TIMER_CYCLES_PER_SEC    (1000000000/AHB_CLK) //(TIMER_CYCLES_PER_MSEC * 1000)
#define TIMER_CYCLES_PER_TENSEC (10000000000/AHB_CLK) //(TIMER_CYCLES_PER_MSEC * 10000)

#define TIMER_WRAP_TO_MSEC	 (TIMER_CYCLES_PER_TENSEC/TIMER_CYCLES_PER_MSEC)

#define TIMER_CYCLES_PER_TICK TIMER_CYCLES_PER_MSEC

#define TIMER_TICKS_PER_SEC     1000
#define TYPE_USEC	0x1
#define TYPE_MSEC	0x2

extern U32 pending_ticks;

#else
#define TIMER_TICKS_PER_SEC     1000
#define TIMER_CYCLES_PER_MSEC   max_cycle_count

typedef union timer{
	U64 timer_u64;
	STIME timer_stime;
}timer_t;


#define timer_elapsed_time (g_timer.timer_stime.msec)
#define timer_elapsed_cycles (g_timer.timer_stime.cycles)
#define timer_u64 (g_timer.timer_u64)
extern timer_t g_timer;
#endif


void hw_timer_init(void);
void get_time(STIME *time);
void timediff(STIME *t1, STIME *t0, STIME *dt);

extern U32 max_cycle_count;

static inline U32 read_cycle_counter(void)
{
#if defined(COMCERTO_100)
        return *(V32*)TIMER3_CURR_COUNT;
#elif defined(COMCERTO_1000)
        return *(V32*)TIMER2_CURR_COUNT;
#else
        return (TIMER_CYCLES_PER_WRAP - (0xFFFFFFFF - readl(GPT_COUNTER))); /* FIXME */
#endif
}



#if defined(COMCERTO_2000)

void hw_timer_update(void);

/** Convert cycles to time.
* Divides the cycles with the cycles per msec or cycles per usec based on the 
* type passed. It converts the cycles to msec or usec.
*
* @param cycles           timer cycles in terms of system clock 
* @param time_type        Type of conversion / either msec or usec. 
*
* */
static inline U32 convert_cycles_to_time(U32 cycles,U8 time_type)
{
	/* The largest n such that cycles >= 2^n, and assign the pwr */
	U8 pwr;
	int i;
	u32 cycles_per_type, res = 0;

	if (time_type == TYPE_MSEC)
	{
		pwr = 0x11;
		cycles_per_type = TIMER_CYCLES_PER_MSEC;
	}
	else if (time_type == TYPE_USEC)
	{
		pwr = 0x7;
		cycles_per_type = TIMER_CYCLES_PER_USEC;
	}

	for (i = 31 - pwr ; i >= 0; i--)
	{
		if ((cycles_per_type << i) <= cycles){
			res += (1 << i);
			cycles -= (cycles_per_type << i);
		}
	}	

	return res;
}

static inline U32 timediff_us(STIME *t1, STIME *t0)
{
        STIME dt;

        timediff(t1, t0, &dt);

        return dt.msec * 1000 + convert_cycles_to_time(dt.cycles, TYPE_USEC);
}

static inline U32 time_us(STIME *dt)
{
        return dt->msec * 1000 + convert_cycles_to_time(dt->cycles, TYPE_USEC);
}

#else
static inline U32 timediff_cycles(STIME *t1, STIME *t0)
{
        STIME dt;

        timediff(t1, t0, &dt);

        return dt.msec * TIMER_CYCLES_PER_MSEC + dt.cycles;
}

static inline U32 timediff_us(STIME *t1, STIME *t0)
{
        STIME dt;

        timediff(t1, t0, &dt);

        return dt.msec * 1000 + (dt.cycles * 1000) / TIMER_CYCLES_PER_MSEC;
}

static inline U32 time_us(STIME *dt)
{
        return dt->msec * 1000 + (dt->cycles * 1000) / TIMER_CYCLES_PER_MSEC;
}
#endif

#endif


