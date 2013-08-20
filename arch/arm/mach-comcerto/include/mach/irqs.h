/*
 *  arch/arm/mach-comcerto/include/mach/irqs.h
 *
 *  Copyright (C) 2011 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#if defined(CONFIG_ARCH_M86XXX)
	#include <mach/comcerto-2000/irqs.h>
#else
	#error "Unsupported CPU"
#endif

#endif  /* __ASM_ARCH_IRQS_H */
