/*
 * arch/arm/mach-kirkwood/include/mach/entry-macro.S
 *
 * Low-level IRQ helper macros for Marvell Kirkwood platforms
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <mach/bridge-regs.h>

	.macro  disable_fiq
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	.macro  get_irqnr_preamble, base, tmp
	ldr	\base, =IRQ_VIRT_BASE
	.endm

#if defined(CONFIG_CPU_BIG_ENDIAN)
	.macro	HTOLL sr, tt		@ sr   = A  ,B  ,C  ,D
	eor \tt, \sr, \sr, ror #16 ;    @ temp = A^C,B^
	bic \tt, \tt, #0xff0000 ;       @ temp = A^C,0  ,C^A,D^B
	mov \sr, \sr, ror #8 ;          @ sr   = D  ,A  ,B  ,C
	eor \sr, \sr, \tt, lsr #8       @ sr   = D  ,C  ,B  ,A
	.endm
#else
	.macro	HTOLL sr, tt
	.endm
#endif

	.macro  get_irqnr_and_base, irqnr, irqstat, base, tmp
	@ check low interrupts
	ldr	\base, =IRQ_VIRT_CAUSE_LOW_REG
	ldr	\irqstat, [\base]
	HTOLL	\irqstat, \base
	ldr	\base, =IRQ_VIRT_MASK_LOW_REG
	ldr	\tmp, [\base]
	HTOLL	\tmp, \base
	mov	\irqnr, #31
	ands	\irqstat, \irqstat, \tmp
	bne	1001f

	@ if no low interrupts set, check high interrupts
	ldr	\base, =IRQ_VIRT_CAUSE_HIGH_REG
	ldr	\irqstat, [\base]
	HTOLL	\irqstat, \base
	ldr	\base, =IRQ_VIRT_MASK_HIGH_REG
	ldr	\tmp, [\base]
	HTOLL	\tmp, \base
	mov	\irqnr, #63
	ands	\irqstat, \irqstat, \tmp

	@ find first active interrupt source
1001:	clzne	\irqstat, \irqstat
	subne	\irqnr, \irqnr, \irqstat
	.endm
