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

unsigned long long get_cpu_time(void)
{
	u64 clock = sched_clock_cpu(smp_processor_id());
	return clock;
}

int set_schedule(int policy, const struct sched_param *param) {
	return 0;
}

unsigned long long force_cpu_idle(void)
{
	unsigned long long start, end;
	start = get_cpu_time();
	end = get_cpu_time();
	
	return (end-start);
}

EXPORT_SYMBOL(force_cpu_idle);
EXPORT_SYMBOL(get_cpu_time);
EXPORT_SYMBOL(set_schedule);
