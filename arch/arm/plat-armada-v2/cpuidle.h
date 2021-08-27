/*
 * arch/arm/plat-armada/cpuidle.h
 *
 * CPU power management functions
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PLAT_ARMADA_CPUIDLE_H
#define __PLAT_ARMADA_CPUIDLE_H

int armadaxp_snoop_dis_virt(void);
int armadaxp_cpu_suspend(unsigned long);
int armada_powerdown(unsigned long);
void armada_cpu_resume(void);
void armadaxp_fabric_setup_deepIdle(void);
void armadaxp_fabric_prepare_deepIdle(void);
void armadaxp_fabric_prepare_hotplug(void);
void armadaxp_fabric_restore_deepIdle(void);
void armadaxp_deepidle(int power_state);
void armadaxp_smp_prepare_idle(unsigned int processor_id);
void armadaxp_smp_restore_idle(unsigned int processor_id);

extern int axp_secondary_startup(void);
#ifdef CONFIG_ARMADA_SUPPORT_DEEP_IDLE_CESA_USE
extern int armadaxp_deep_idle_exit(void);
extern unsigned char armadaxp_deep_idle_exit_start;
extern unsigned char armadaxp_deep_idle_exit_end;
#endif

typedef enum  {
	DISABLED,
	WFI,
	DEEP_IDLE,
	SNOOZE,
} MV_PM_STATES;

#endif /* __PLAT_ARMADA_CPUIDLE_H*/
