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

#ifndef __ASM_ARCH_SPI_H
#define __ASM_ARCH_SPI_H

#if defined(CONFIG_ARCH_M86XX)
	#include <mach/comcerto-2000/spi.h>
#else
	#error "Unsupported CPU"
#endif

#endif  /* __ASM_ARCH_SPI_H */
