/*
 * debug-macro.S
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <mach/armada38x.h>

	.macro  addruart, rp, rv, tmp
	ldr     \rp, =INTER_REGS_PHYS_BASE
	ldr	\rv, =INTER_REGS_VIRT_BASE
	orr     \rp, \rp, #0x00012000
        orr     \rv, \rv, #0x00012000
#ifdef CONFIG_MV_UART_PORT
	orr     \rp, \rp, #0x100 * CONFIG_MV_UART_PORT
        orr     \rv, \rv, #0x100 * CONFIG_MV_UART_PORT
#endif
	.endm

#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>
