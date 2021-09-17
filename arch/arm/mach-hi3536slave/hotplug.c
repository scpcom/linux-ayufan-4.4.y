/******************************************************************************
 *    COPYRIGHT (C) 2013 Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-12-18
 *
******************************************************************************/

#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/completion.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

/*****************************************************************************/

static inline void hi3536_scu_power_off(int cpu)
{
	unsigned long regval;

	regval = readl(__io_address(REG_BASE_CRG + REG_PERI_CRG10));
	regval |= (7 << 4);
	writel(regval, __io_address(REG_BASE_CRG + REG_PERI_CRG10));
}

void hi3536_cpu_die(unsigned int cpu)
{
	flush_cache_all();
	hi3536_scu_power_off(cpu);
	BUG();
}

void hi3536_scu_power_up(int cpu)
{
	unsigned int regval;

	/* clear the slave cpu reset */
	regval = readl(__io_address(REG_BASE_CRG + REG_PERI_CRG10));
	if (0 == cpu)
		return;
	else if (1 == cpu)
		regval &= ~(7<<4);
	else if (2 == cpu)
		regval &= ~(7<<8);
	else if (3 == cpu)
		regval &= ~(7<<12);

	writel(regval, __io_address(REG_BASE_CRG + REG_PERI_CRG10));
}
