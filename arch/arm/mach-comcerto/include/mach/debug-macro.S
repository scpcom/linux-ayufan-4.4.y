/*
 * arch/arm/mach-comcerto/include/mach/debug-macro.S
 *
 * Copyright (C) 2012 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define UART_LSR	0x14


	.macro	addruart, rx, rv, tmp
	ldr     \rx, =0x96000000        @ physical base address
	ldr     \rv, =0xf1600000        @ virtual base
	#ifdef CONFIG_COMCERTO_UART1_SUPPORT
		add     \rx, \rx, #0x00400000
		add     \rv, \rv, #0x00400000
	#elif CONFIG_COMCERTO_UART0_SUPPORT
		add     \rx, \rx, #0x00300000
		add     \rv, \rv, #0x00300000
	#else
		#error no uart defined
	#endif
	.endm

	.macro	senduart,rd,rx
	strb	\rd, [\rx, #0x00]
	.endm

	.macro	waituart,rd,rx
1001:	ldrb	\rd, [\rx, #UART_LSR]
	tst	\rd, #0x20		@ wait for THRE
	beq	1001b
	.endm

	.macro	busyuart,rd,rx
1001:	ldrb	\rd, [\rx, #UART_LSR]
	and	\rd, \rd, #0x60
	teq	\rd, #0x60		@ wait for TEMT and THRE
	bne	1001b
	.endm
