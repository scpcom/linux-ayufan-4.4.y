/*
 * include/asm-arm/arch-mv78xx0/entry-macro.S
 *
 * Low-level IRQ helper macros for Marvell MV78xx0 platforms
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <mach/armadaxp.h>

	.macro	disable_fiq
	.endm

	.macro  get_irqnr_preamble, base, tmp
	ldr	\base, =AXP_IRQ_VIRT_BASE
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm
#ifndef CONFIG_ARMADAXP_USE_IRQ_INTERRUPT_ACK

#if defined(CONFIG_CPU_BIG_ENDIAN)
		.macro	HTOLL sr, tt		@ sr   = A  ,B  ,C  ,D
		rev \sr, \sr
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
	beq	1001f	
	mov 	\tmp, \irqstat

	mov 	\tmp, \tmp, lsr #29 			@ determine the irq group,  
	bics	\tmp, \tmp, #4 	 			@ clear irq_stat bit

#ifdef CONFIG_SMP
	@ if vec sel is 0 and bits 0-4 are set then it is IPI timer handled seperatly.
	bne	1000f
	ands	\tmp, \irqstat, #0x5
	beq 	1000f

	@restore tmp - should be optimized!!!!
        mov     \tmp, \irqstat

        mov     \tmp, \tmp, lsr #29                     @ determine the irq group,
        bics    \tmp, \tmp, #4                          @ clear irq_stat bit
	beq	1001f

#endif
1000:
	bic	\irqstat, \irqstat, #(0xE0000000) 	@ leave irq bits, clear the rest				
	mov    	\irqnr, #0x1F
	orrs    \irqnr, \irqnr, \tmp, lsl #5    	@ irqnr = 0x1F, 0x3F, 0x5F, 0x7f, make sure Z is off	
	clz	\irqstat, \irqstat	              	@ find first active interrupt source
	sub	\irqnr, \irqnr, \irqstat
	mov 	r1, #3	
	mul	\tmp, r1, \tmp
	sub 	\irqnr, \irqnr, \tmp
1001:	
	.endm

        /* We assume that irqstat (the raw value of the IRQ acknowledge
         * register) is preserved from the macro above.
         */

	.macro test_for_ipi, irqnr, irqstat, base, tmp
	ands 	\tmp, \irqstat, #0x80000000		@ did we get irq
	beq	1002f
	ands 	\tmp, \irqstat, #0x00000001		@ was it doorbell
	beq	1002f
	ldr	\irqnr, [\base, #AXP_IN_DOORBELL_CAUSE]
	HTOLL	\irqnr, \tmp
	ands    \irqnr, \irqnr, #0xFF		@ 8 lower doorbells are considered IPI
	beq     1002f
	clz	\irqnr, \irqnr
	mov	\tmp, #31
	sub	\irqnr, \tmp, \irqnr
	mov     \tmp, #1
	lsl     \tmp, \irqnr
	HTOLL 	\tmp, \tmp
	mvn     \tmp, \tmp
	str   	\tmp, [\base, #AXP_IN_DOORBELL_CAUSE]	@ clean irq
1002:
	.endm

	/* As above, this assumes that irqstat and base are preserved.. */

	.macro test_for_ltirq, irqnr, irqstat, base, tmp
	ands 	\tmp, \irqstat, #0x80000000		@ did we get irq
	beq	1003f
	ands 	\tmp, \irqstat, #0x00000060		@ was it timer
1003:
	.endm

	.macro test_for_ipc, irqnr, irqstat, base, tmp
	ands 	\tmp, \irqstat, #0x80000000		@ did we get irq
	beq	1006f
	ands 	\tmp, \irqstat, #0x00000001		@ was it doorbell
	beq	1006f
	ldr	\irqnr, [\base, #AXP_IN_DOORBELL_CAUSE]
	HTOLL	\irqnr, \tmp
	ands    \irqnr, \irqnr, #0xF000		@ 12-15 doorbells are considered IPC
	beq     1006f
	clz	\irqnr, \irqnr
	mov	\tmp, #31
	sub	\irqnr, \tmp, \irqnr
	mov 	\tmp, #1
	lsl	\tmp, \irqnr
	mvn	\tmp, \tmp
	strh   	\tmp, [\base, #AXP_IN_DOORBELL_CAUSE]	@ clean irq
1006:
	.endm

#else /* CONFIG_ARMADAXP_USE_IRQ_INTERRUPT_ACK */
		/*
		 * The interrupt numbering scheme is defined in the
		 * interrupt controller spec.
		 *
		 * Interrupts 0-2 are IPI
		 * 0-31 are local.
		 * 32-1022 are global
		 * 1023 is "spurious" (no interrupt)
		 */

		.macro  get_irqnr_and_base, irqnr, irqstat, base, tmp

		ldr     \irqstat, [\base, #AXP_CPU_INTACK]
		ldr	\tmp, =1023
		and     \irqnr, \irqstat, \tmp
#ifdef CONFIG_SMP
		cmp     \irqnr, #7	
		cmpcc	\irqnr, \irqnr
		cmpne	\irqnr, \tmp
		cmpcs	\irqnr, \irqnr
#else
		cmp     \irqnr, \tmp
#endif
		.endm

		/* We assume that irqstat (the raw value of the IRQ acknowledge
		 * register) is preserved from the macro above.
		 * If there is an IPI, we immediately signal end of interrupt on the
		 * controller, since this requires the original irqstat value which
		 * we won't easily be able to recreate later.
		 */

		.macro test_for_ipi, irqnr, irqstat, base, tmp
		ldr	\tmp, =1023
		and     \irqnr, \irqstat, \tmp
		cmp	\irqnr, #3
		movcc	\tmp, #0
		strcc	\tmp, [\base, #AXP_IN_DOORBELL_CAUSE]   @ clean ipi irq
		cmpcs	\irqnr, \irqnr

		.endm

		/* As above, this assumes that irqstat and base are preserved.. */

		.macro test_for_ltirq, irqnr, irqstat, base, tmp
		ldr	\tmp, =1023
		and     \irqnr, \irqstat, \tmp
		mov	\tmp, #0
		cmp	\irqnr, #5
		moveq   \tmp, #1
		cmp     \tmp, #0
		.endm

		.macro test_for_ipc, irqnr, irqstat, base, tmp @ YY - check this one again
		ldr	\tmp, =1023
		and     \irqnr, \irqstat, \tmp
		cmp	\irqnr, #3
		movcc	\tmp, #0
		strcc	\tmp, [\base, #AXP_IN_DOORBELL_CAUSE]   @ clean ipi irq
		cmpcs	\irqnr, \irqnr
		.endm
#endif
