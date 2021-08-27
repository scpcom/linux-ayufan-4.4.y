/*
 * debug-macro.S
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <mach/armada370.h>

	.macro  addruart, rp, rv, tmp
	ldr	\rp, =INTER_REGS_PHYS_BASE
	ldr	\rv, =INTER_REGS_BASE
	orr	\rp, \rp, #0x00012000
	orr	\rv, \rv, #0x00012000
	.endm

#if 0
	.macro	addruart,rx, tmp
	mrc	p15, 0, \rx, c1, c0
	tst	\rx, #1					@ MMU enabled?
	ldreq	\rx, =INTER_REGS_PHYS_BASE
	ldrne	\rx, =INTER_REGS_BASE
	orr	\rx, \rx, #0x00012000
	.endm
#endif
#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>
