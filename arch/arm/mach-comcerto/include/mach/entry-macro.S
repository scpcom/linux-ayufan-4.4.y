/*
 * arch/arm/mach-comcerto/include/mach/entry-macro.S
 *
 * Copyright (C) 2011 Mindspeed Technologies, Inc.
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

#include <mach/hardware.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/entry-macro-gic.S>

	.macro	disable_fiq
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

#ifndef HAVE_GET_IRQNR_PREAMBLE
	.macro	get_irqnr_preamble, base, pending_intr, tmp
	ldr	\base, =gic_cpu_base_addr
	ldr	\base, [\base]
	ldr	\pending_intr, =gic_dist_base_addr
	ldr	\pending_intr, [\base_dist]
	.endm
#endif

/*
 * The interrupt numbering scheme is defined in the
 * interrupt controller spec.  To wit:
 *
 * Interrupts 0-15 are IPI
 * 16-31 are local.  We allow 30 to be used for the watchdog.
 * 32-1020 are global
 * 1021-1022 are reserved
 * 1023 is "spurious" (no interrupt)
 *
 * A simple read from the controller will tell us the number of the highest
 * priority enabled interrupt.  We then just need to check whether it is in the
 * valid range for an IRQ (30-1020 inclusive).
 */

	.macro  get_irqnr_and_base, irqnr, irqstat, base, pending_intr, tmp

#ifdef CONFIG_COMCERTO_MSP
	ldr     \irqstat, [\base, #GIC_CPU_HIGHPRI]
	bic     \irqnr, \irqstat, #0x1c00
	cmp     \irqnr, #33
	cmpne   \irqnr, #66
	cmpne   \irqnr, #87
	cmpne   \irqnr, #51
	cmpeq   \irqnr, \irqnr
	bne 1001f

	mov \irqnr, \irqnr /* breakpoint here */

	beq 1002f

1001:
#endif /* CONFIG_COMCERTO_MSP */

	ldr     \irqstat, [\base, #GIC_CPU_INTACK]
	/* bits 12-10 = src CPU, 9-0 = int # */

	bic     \irqnr, \irqstat, #0x1c00
	
#ifdef CONFIG_COMCERTO_MSP
	cmp     \irqnr, #33
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	ldreq	\tmp, =0x2	//2 on bit1
	ldreq	\pending_intr, =COMCERTO_DISTR_INT_SET_PENDING_OFFSET_4
	streq	\tmp, [\pending_intr]
	beq	1002f

	cmpne   \irqnr, #51
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	ldreq	\tmp, =0x80000	//0x80000 on bit 19
	ldreq	\pending_intr, =COMCERTO_DISTR_INT_SET_PENDING_OFFSET_4
	streq	\tmp, [\pending_intr]
	beq	1002f

	cmpne   \irqnr, #66
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	ldreq	\tmp, =0x4	//4 on bit 2
	ldreq	\pending_intr, =COMCERTO_DISTR_INT_SET_PENDING_OFFSET_8
	streq	\tmp, [\pending_intr]
	beq	1002f

	cmpne   \irqnr, #87
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	ldreq	\tmp, =0x800000	//0x800000 on bit 23
	ldreq	\pending_intr, =COMCERTO_DISTR_INT_SET_PENDING_OFFSET_8
	streq	\tmp, [\pending_intr]
	beq	1002f

#endif /* CONFIG_COMCERTO_MSP */
	ldr	\tmp, =1021
	cmp     \irqnr, #15
	cmpcc	\irqnr, \irqnr
	cmpne	\irqnr, \tmp
	cmpcs	\irqnr, \irqnr

#ifdef CONFIG_COMCERTO_MSP
1002:
#endif /* CONFIG_COMCERTO_MSP */

	.endm

/* We assume that irqstat (the raw value of the IRQ acknowledge
 * register) is preserved from the macro above.
 * If there is an IPI, we immediately signal end of interrupt on the
 * controller, since this requires the original irqstat value which
 * we won't easily be able to recreate later.
 */

	.macro test_for_ipi, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	cmp	\irqnr, #16
	strcc	\irqstat, [\base, #GIC_CPU_EOI]
	cmpcs	\irqnr, \irqnr
	.endm
