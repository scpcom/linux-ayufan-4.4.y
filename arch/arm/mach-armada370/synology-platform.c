#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/* Copyright (c) 2013 Synology Inc. */
 
#include <stdarg.h>

#include <linux/export.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>

#include "ctrlEnv/sys/mvCpuIfRegs.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "mvOs.h"

#ifdef CONFIG_SYNO_THERMAL_CTRL
unsigned long long get_cpu_time(void)
{
	int cpu = smp_processor_id();
	u64 clock;
	unsigned long flags;

	local_irq_save(flags);
	clock = sched_clock_cpu(cpu);
	local_irq_restore(flags);

	return clock;
}

int set_schedule(int policy, const struct sched_param *param) {
	return sched_setscheduler(current, policy, param);
}

unsigned long long force_cpu_idle(void)
{
	unsigned long long start, end;
	unsigned int backup[IRQ_MAIN_INTS_NUM];
	int i;

	preempt_disable();
	for (i = 0; i < IRQ_MAIN_INTS_NUM; i++) {
		switch (i) {
		case IRQ_AURORA_TIMER0:
		case IRQ_AURORA_UART0:
		case IRQ_AURORA_GLOB_TIMER0:
		case IRQ_AURORA_GLOB_TIMER1:
		case IRQ_AURORA_GLOB_TIMER2:
		case IRQ_AURORA_GLOB_TIMER3:
				break;
		default:				
			backup[i] = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(i));
			MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(i), 0);
			break;
		}
	}
	
	start = get_cpu_time();
	cpu_do_idle();
	end = get_cpu_time();
	
	for (i = 0; i < IRQ_MAIN_INTS_NUM; i++) {
		switch (i) {
		case IRQ_AURORA_TIMER0:
		case IRQ_AURORA_UART0:
		case IRQ_AURORA_GLOB_TIMER0:
		case IRQ_AURORA_GLOB_TIMER1:
		case IRQ_AURORA_GLOB_TIMER2:
		case IRQ_AURORA_GLOB_TIMER3:
				break;
		default:				
			MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(i), backup[i]);
			break;
		}
	}
	preempt_enable();

	return (end-start);
}

EXPORT_SYMBOL(force_cpu_idle);
EXPORT_SYMBOL(get_cpu_time);
EXPORT_SYMBOL(set_schedule);
#endif

