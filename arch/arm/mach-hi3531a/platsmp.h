/******************************************************************************
 *    COPYRIGHT (C) 2013 Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-12-19
 *
******************************************************************************/
#ifndef __PLATSMP__H__
#define __PLATSMP__H__

extern struct smp_operations hi3531a_smp_ops;

void hi3531a_scu_power_up(int cpu);
void hi3531a_secondary_startup(void);

#endif
