/*
 * arch/arm/plat-armada/cpuidle.c
 *
 * CPU idle implementation for Marvell ARMADA-XP SoCs
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/cpuidle.h>
#include <asm/io.h>
#include <asm/proc-fns.h>
#include <plat/cache-aurora-l2.h>
#include <mach/smp.h>
#include <asm/vfp.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>
#include <asm/sections.h>
#include <linux/export.h>
#include <asm/sections.h>
#include <asm/suspend.h>

#include <../cpuidle.h>
#include "ctrlEnv/sys/mvCpuIfRegs.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "mvOs.h"

void smp_resume(void);

/*
 * Store boot information used by bin header
 */
#define  BOOT_INFO_ADDR		(0x3000)
#define  BOOT_MAGIC_WORD	(0xDEADB002)
#define  REG_LIST_END		(0xFFFFFFFF)
#define  BOOTROM_INTER_REGS_PHYS_BASE	(0xD0000000)

#define SDRAM_WIN_BASE_REG(x)	(0x20180 + (0x8*x))
#define SDRAM_WIN_CTRL_REG(x)	(0x20184 + (0x8*x))
#define MAX_CS_COUNT		4

/*#define DEBUG_DDR_SELF_REFRESH*/

#ifdef DEBUG_DDR_SELF_REFRESH
/* 
 * Write a pattern to memory and check it after resume
 * to ensure the DDR has entered self refresh
 */
#define BASE_PATTERN 0x40000000
#define SIZE_PATTERN 0x10000000
static int  *pattern_ptr;

void write_pattern(void)
{
	int *ptr, n = 0;
	
	ptr = ioremap(BASE_PATTERN,SIZE_PATTERN);
	pattern_ptr = ptr;
	pr_info("Writing pattern to %p(%p) + %x\n", ptr,
		(void *)virt_to_phys(ptr), SIZE_PATTERN);

	if(ptr == 0)
	{
		pr_info("Failed to remap pattern space\n");
		return;
	}

	while (n < (SIZE_PATTERN / sizeof(int)))
	{
		*ptr = (int)ptr;
		ptr++;
		n++;
	}
}

void test_pattern(void)
{
	int *ptr, n = 0;

	ptr = pattern_ptr;

	if(pattern_ptr == 0)
		return;

	pr_info("Testing pattern from %p (%p) + %x\n", ptr,
		(void *)virt_to_phys(ptr), SIZE_PATTERN);

	__asm__("b .");

	while (n < (SIZE_PATTERN / sizeof(int)))
	{
		if(*ptr != (int)ptr)
		{
			pr_info("Bad pattern %x at %p\n", *ptr, ptr);
		}
		ptr++;
		n++;
	}
}
#endif

void armadaxp_store_boot_info(void)
{
	int *store_addr = (int *)BOOT_INFO_ADDR;
	int *resume_pc, win;

	store_addr = (int *)phys_to_virt((int)store_addr);
	resume_pc = (int *)virt_to_phys(armadaxp_cpu_resume);

	/*
	 * Store magic word indicating suspend to ram
	 * and return address
	 */
	*store_addr++ = (int)(BOOT_MAGIC_WORD);
	*store_addr++ = (int)resume_pc;

	/*
	 * Now store registers that need to be proggrammed before
	 * comming back to linux. format is addr->value
	 */
	/* Disable X-Bar window 12 which is opend in the bootROM
	 * by default to 0xF0000000 */
	*store_addr++ = BOOTROM_INTER_REGS_PHYS_BASE + 0x200B0;
	*store_addr++ = 0x0;

	/* Update Internal Regs offset in case UBoot is configured
	* to use a different base address.	*/
	*store_addr++ = BOOTROM_INTER_REGS_PHYS_BASE + 0x20080;
	*store_addr++ = INTER_REGS_PHYS_BASE;

	/* Save AXI windows data */
	for (win = 0; win < 4; win++) {
		*store_addr++ = INTER_REGS_PHYS_BASE + SDRAM_WIN_BASE_REG(win);
		*store_addr++ = MV_REG_READ(SDRAM_WIN_BASE_REG(win));

		*store_addr++ = INTER_REGS_PHYS_BASE + SDRAM_WIN_CTRL_REG(win);
		*store_addr++ = MV_REG_READ(SDRAM_WIN_CTRL_REG(win));
	}

	/* Mark the end of the boot info*/
	*store_addr = REG_LIST_END;
}
/*
 * Save SOC & CPU register data before powering down board
 */
void armadaxp_suspend(void)
{
#if defined(CONFIG_VFP)
	vfp_save();
#endif
	aurora_l2_pm_enter();
	armadaxp_store_boot_info();
#ifdef DEBUG_DDR_SELF_REFRESH
	write_pattern();
#endif
	/* This call will shut down the board */
	cpu_suspend(0, armadaxp_powerdown);
#ifdef DEBUG_DDR_SELF_REFRESH
	test_pattern();
#endif
	cpu_init();
	armadaxp_fabric_restore_deepIdle();
	aurora_l2_pm_exit();
#ifdef CONFIG_SMP
	smp_resume();
#endif

#if defined(CONFIG_VFP)
	vfp_restore();
#endif
}
