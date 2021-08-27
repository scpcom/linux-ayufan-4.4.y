/*
 * arch/arm/mach-armada370/include/mach/entry-macro.S
 *
 * Low-level IRQ helper macros for Marvell Armada370 platform
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <mach/armada370.h>

	.macro  disable_fiq
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	.macro  get_irqnr_preamble, base, tmp
	ldr	\base, =AXP_IRQ_VIRT_BASE
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
	/* TBD - need to be optimized 29*(sel-1) + cls 						*/
	/* r1 - we shouldnt use it here 							*/
	/* in case of SMP we only handle bit 0,1 (doorbell) and 5,6 (timer) from cause Vec 0 	*/
	/* return value is: irqnr and the flag state!!!!!!!!!!!!				*/
	.macro  get_irqnr_and_base, irqnr, irqstat, base, tmp
	@ check low interrupts
	ldr	\irqstat, [\base, #AXP_IRQ_SEL_CAUSE_OFF]
	HTOLL	\irqstat, \tmp
	ands 	\tmp, \irqstat, #0x80000000		@ did we get irq
	beq	1004f
	mov 	\tmp, \irqstat
	mov 	\tmp, \tmp, lsr #29 			@ determine the irq group,
	bics	\tmp, \tmp, #4 	 			@ clear irq_stat bit
	bic	\irqstat, \irqstat, #(0xE0000000) 	@ leave irq bits, clear the rest
	mov    	\irqnr, #0x1F
	orrs    \irqnr, \irqnr, \tmp, lsl #5    	@ irqnr = 0x1F, 0x3F, 0x5F, 0x7f, make sure Z is off
	clz	\irqstat, \irqstat	              	@ find first active interrupt source
	sub	\irqnr, \irqnr, \irqstat
	mov 	r1, #3
	mul	\tmp, r1, \tmp
	sub 	\irqnr, \irqnr, \tmp			@ irqnr = per cpu irq number
	cmp	\irqnr, #82				@ GPP LOW 0-7
	beq	1000f
	cmp	\irqnr, #83				@ GPP LOW 8-15
	beq	1000f
	cmp	\irqnr, #84				@ GPP LOW 16-23
	beq	1000f
	cmp	\irqnr, #85				@ GPP LOW 24-31
	beq	1000f
	cmp	\irqnr, #87				@ GPP LOW 32-39
	beq	1001f
	cmp	\irqnr, #88				@ GPP LOW 40-47
	beq	1001f
	cmp	\irqnr, #89				@ GPP LOW 48-55
	beq	1001f
	cmp	\irqnr, #90				@ GPP LOW 56-63
	beq	1001f
	cmp	\irqnr, #91				@ GPP LOW 64-66
	beq	1002f
	cmp	\irqnr, #4				@ SoC Main Error Summary
	beq	1003f
	b	1004f

1000:
	ldr     \tmp, =AXP_ASM_GPP_IRQ_MASK_REG
        ldr     \tmp, [\tmp]    				@ get gpp mask
        ldr	\irqstat, =AXP_ASM_GPP_IRQ_CAUSE_REG
        ldr     \irqstat, [\irqstat]                		@ get gpp data in (cause)
        ands    \irqstat, \irqstat, \tmp                        @ and mask and cause
	beq	1004f						@ if eq to 0 jump to 1004
	HTOLL	\irqstat, \tmp
	clz     \irqnr, \irqstat
	mov     \tmp, #31
	subs    \irqnr, \tmp, \irqnr
	add	\irqnr,\irqnr, #128				@ set GPIO base irq
	teq     \irqnr, #160                                    @ if irq isn't 160 unset the flag
	b	1004f

1001:
	ldr     \tmp, =AXP_ASM_GPP_IRQ_MID_MASK_REG
        ldr     \tmp, [\tmp]    				@ get gpp mask
        ldr	\irqstat, =AXP_ASM_GPP_IRQ_MID_CAUSE_REG
        ldr     \irqstat, [\irqstat]                		@ get gpp data in (cause)
        ands    \irqstat, \irqstat, \tmp                        @ and mask and cause
	beq	1004f						@ if eq to 0 jump to 1004
	HTOLL	\irqstat, \tmp
	clz     \irqnr, \irqstat
	mov     \tmp, #31
	subs    \irqnr, \tmp, \irqnr
	add	\irqnr,\irqnr, #160				@ set GPIO base irq
	teq     \irqnr, #192                                    @ if irq isn't 192 unset the flag
	b	1004f

1002:
	ldr     \tmp, =AXP_ASM_GPP_IRQ_HIGH_MASK_REG
        ldr     \tmp, [\tmp]    				@ get gpp mask
        ldr	\irqstat, =AXP_ASM_GPP_IRQ_HIGH_CAUSE_REG
        ldr     \irqstat, [\irqstat]                		@ get gpp data in (cause)
        ands    \irqstat, \irqstat, \tmp                        @ and mask and cause
	beq	1004f						@ if eq to 0 jump to 1004
	HTOLL	\irqstat, \tmp
	clz     \irqnr, \irqstat
	mov     \tmp, #31
	subs    \irqnr, \tmp, \irqnr
	add	\irqnr,\irqnr, #192				@ set GPIO base irq
	teq     \irqnr, #224                                    @ if irq isn't 224 unset the flag
	b	1004f

1003:
	ldr     \tmp, =AXP_ASM_SOC_MAIN_ERR_MASK_REG
        ldr     \tmp, [\tmp]    				@ get SoC main error mask
        ldr	\irqstat, =AXP_ASM_SOC_MAIN_ERR_CAUSE_REG
        ldr     \irqstat, [\irqstat]                		@ get SoC main error cause
        ands    \irqstat, \irqstat, \tmp                        @ and mask and cause
	beq	1004f						@ if eq to 0 jump to 1004
	HTOLL	\irqstat, \tmp
	clz     \irqnr, \irqstat
	mov     \tmp, #31
	subs    \irqnr, \tmp, \irqnr
	add	\irqnr,\irqnr, #224				@ set GPIO base irq
	teq     \irqnr, #256                                    @ if irq isn't 256 unset the flag
	b	1004f

1004:
	.endm

	.macro test_for_pmuirq, irqnr, irqstat, base, tmp
	ands 	\tmp, \irqstat, #0x80000000		@ did we get irq
	beq	1005f
	ands 	\tmp, \irqstat, #0x00000008		@ was it mp
1005:
	.endm
