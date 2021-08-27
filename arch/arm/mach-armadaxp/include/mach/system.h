/*
 * include/mach/system.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_SYSTEM_H
#define __MACH_SYSTEM_H

#include <asm/proc-fns.h>
#include <mach/hardware.h>

#include "boardEnv/mvBoardEnvLib.h"

#ifdef CONFIG_MV_SUPPORT_64KB_PAGE_SIZE
#define LSP_PG_SZ_VER  " (Large Page)"
#else
#define LSP_PG_SZ_VER  ""
#endif

#define LSP_VERSION    "linux-3.2.y-2013_Q1.2p2" LSP_PG_SZ_VER

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	printk("Reseting...\n");
	mvBoardReset();
	while (1);/* This should never be reached */
}

#endif
