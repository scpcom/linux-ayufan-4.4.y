/* arch/arm/mach-alpine/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/motherboard.h>
#define DEBUG_LL_UART_OFFSET	SLR_LAD_PER_OFFSET(3)

		.macro	addruart,rp,rv
		mov	\rp, #DEBUG_LL_UART_OFFSET
		orr	\rp, \rp, #SLR_SB_LAD_OFF
		orr	\rv, \rp, #SLR_SB_IREG  @ virtual base
		orr	\rp, \rp, #SLR_SB_IREG	@ physical base
		.endm

#include <asm/hardware/debug-pl01x.S>
