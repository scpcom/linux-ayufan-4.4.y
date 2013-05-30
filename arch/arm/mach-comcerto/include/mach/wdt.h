/*
 *  linux/include/asm-arm/arch-comcerto/irqs.h
 *
 *  Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __ASM_ARCH_WDT_H
#define __ASM_ARCH_WDT_H


#if defined(CONFIG_ARCH_M86XXX)
	#include <mach/comcerto-2000/wdt.h>
#else
	#error "mach/comcerto-2000/wdt.h :  Unknown architecture" 
#endif

#endif  /* __ASM_ARCH_WDT_H */
