/*
 * pm.c
 *
 * Power Management functions for Marvell ArmadaXP System On Chip
 *
 * Maintainer: Nadav Haklai <nadavh@marvell.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>

#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"

#ifdef CONFIG_SHEEVA_DEEP_IDLE
extern void armadaxp_deepidle(int power_state);
extern void armadaxp_suspend(void);
extern void axp_db_restore(void);
extern void axp_irq_restore(void);

typedef enum  {
	DISABLED,
	WFI,
	DEEP_IDLE,
	SNOOZE,
} MV_PM_STATES;

/*
 * Logical check for Armada XP valid PM states
 */
static int armadaxp_pm_valid(suspend_state_t state)
{
	return ((state == PM_SUSPEND_STANDBY) ||
		(state == PM_SUSPEND_MEM));
}

/*
 * Enter the requested PM state
 */
static int armadaxp_pm_enter(suspend_state_t state)
{

	MV_U32 reg;

	switch (state)	{
	case PM_SUSPEND_STANDBY:

		/* Reenable the Uart IRQ in order to wake from it */
		/* Enable Uart IRQ */
		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_UART0));
		reg |= (1 << CPU_INT_SOURCE_CONTROL_IRQ_OFFS);	/* Enable the IRQ */
		reg = (reg & ~0xF) | 0x1;			/* Mask all non-boot CPUs */
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_UART0), reg);

		/* Disable IPI IRQs */
		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_LOW));
		reg &= ~0x1;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_LOW), reg);

#ifdef CONFIG_MV_ETH_PNC_WOL

		printk(KERN_INFO "Entering Wol Mode (Neta IRQs 8,10,12,14 are enabled now)...\n");

		/* Reenable the NETA IRQ in order to wake from it */
		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE0_FIC));
		reg = (reg & ~0xF) | 0x1;	/* Mask all non-boot CPUs */
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE0_FIC), reg);

		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE1_FIC));
		reg = (reg & ~0xF) | 0x1;	/* Mask all non-boot CPUs */
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE1_FIC), reg);

		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE2_FIC));
		reg = (reg & ~0xF) | 0x1;	/* Mask all non-boot CPUs */
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE2_FIC), reg);

		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE3_FIC));
		reg = (reg & ~0xF) | 0x1;	/* Mask all non-boot CPUs */
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE3_FIC), reg);
#endif /* CONFIG_MV_ETH_PNC_WOL */

		armadaxp_deepidle(SNOOZE);

		/* Enable IPI IRQs - return to original state */
		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_LOW));
		reg |= 0x1;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_LOW), reg);

		/* Disable it since it will be re-enabled by the stack */
		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_UART0));
		reg &= ~(1 << CPU_INT_SOURCE_CONTROL_IRQ_OFFS);
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_UART0), reg);
#ifdef CONFIG_MV_ETH_PNC_WOL
		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE0_FIC));
		reg &= ~0x1;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE0_FIC), reg);

		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE1_FIC));
		reg &= ~0x1;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE1_FIC), reg);

		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE2_FIC));
		reg &= ~0x1;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE2_FIC), reg);

		reg = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE3_FIC));
		reg &= ~0x1;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_GBE3_FIC), reg);

		printk(KERN_INFO "Exiting Wol Mode (Neta IRQs 8,10,12,14 are disabled now)...\n");
#endif /* CONFIG_MV_ETH_PNC_WOL */
		break;

	case PM_SUSPEND_MEM:

		pr_info("Suspending Armada XP\n");
		armadaxp_suspend();

		pr_info("Restoring Armada XP\n");
		axp_db_restore();
		axp_irq_restore();

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct platform_suspend_ops armadaxp_pm_ops = {
	.valid		= armadaxp_pm_valid,
	.enter		= armadaxp_pm_enter,
};

static int __init armadaxp_pm_init(void)
{
	printk(KERN_INFO "ArmadaXP Power Managament Suspend Operations Initialized\n");
	suspend_set_ops(&armadaxp_pm_ops);
	return 0;
}

__initcall(armadaxp_pm_init);

#else

static int __init armadaxp_pm_init(void)
{
	printk(KERN_INFO "ArmadaXP Power Managament NOT Initialized (Missing Deep-Idle Support)\n");
	return 0;
}

__initcall(armadaxp_pm_init);

#endif /* CONFIG_SHEEVA_DEEP_IDLE */
