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

static inline void hi3531a_scu_power_off(int cpu)
{
	unsigned long regval;

	regval = readl(__io_address(CRG_REG_BASE + REG_A9_SRST_CRG));
	regval |= (WDG1_SRST_REQ | DBG1_SRST_REQ | CPU1_SRST_REQ);
	writel(regval, __io_address(CRG_REG_BASE + REG_A9_SRST_CRG));
}

/*****************************************************************************/

void hi3531a_cpu_die(unsigned int cpu)
{
	flush_cache_all();
	hi3531a_scu_power_off(cpu);
	BUG();
}
/*****************************************************************************/
/*
 * copy startup code to sram, and flash cache.
 * @start_addr: slave start phy address
 * @jump_addr: slave jump phy address
 */
void set_scu_boot_addr(unsigned int start_addr, unsigned int jump_addr)
{
	unsigned int *virtaddr;
	unsigned int *p_virtaddr;

	p_virtaddr = virtaddr = ioremap(start_addr, PAGE_SIZE);

	*p_virtaddr++ = 0xe51ff004; /* ldr  pc, [pc, #-4] */
	*p_virtaddr++ = jump_addr;  /* pc jump phy address */

	smp_wmb();
	__cpuc_flush_dcache_area((void *)virtaddr,
		(size_t)((char *)p_virtaddr - (char *)virtaddr));
	outer_clean_range(__pa(virtaddr), __pa(p_virtaddr));

	iounmap(virtaddr);
}
/*****************************************************************************/

void hi3531a_scu_power_up(int cpu)
{
	unsigned int regval;

	/* clear the slave cpu reset */
	regval = readl(__io_address(CRG_REG_BASE + REG_A9_SRST_CRG));
	regval &= ~CPU1_SRST_REQ;
	writel(regval, __io_address(CRG_REG_BASE + REG_A9_SRST_CRG));
}
