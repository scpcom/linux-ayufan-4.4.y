/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_SYSTEM_H
#define __MACH_SYSTEM_H

#include "boardEnv/mvBoardEnvLib.h"

#ifdef CONFIG_MV_LARGE_PAGE_SUPPORT
#define LSP_PG_SZ_VER  " (" MV_PAGE_SIZE_STR ")"
#else
#define LSP_PG_SZ_VER  ""
#endif

#define LSP_VERSION "linux-3.2.58-2014_T2.0p2" LSP_PG_SZ_VER

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	printk(KERN_NOTICE "Reseting...\n");
	mvBoardReset();
	/* This should never be reached */
	while (1)
		;
}

#endif /* __MACH_SYSTEM_H */
