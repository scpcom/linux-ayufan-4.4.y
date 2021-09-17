/******************************************************************************
 *    COPYRIGHT (C) 2013 Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-12-19
 *
******************************************************************************/
#ifndef __PLATSMP__H__
#define __PLATSMP__H__

extern struct smp_operations hi3536_smp_ops;

void hi3536_scu_power_up(int cpu);
void hi3536_secondary_startup(void);
void s5_scu_power_up(int cpu);

#endif
