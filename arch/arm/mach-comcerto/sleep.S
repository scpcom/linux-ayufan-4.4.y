/*
 * arch/arm/mach-comcerto/sleep.S
 *
 * Author: Makarand Pawagi
 *
 * Copyright (C) 2013 Mindspeed Technologies, Inc.
 * Copyright (c) 2003 ARM Limited
 * All Rights Reserved
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

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/memory.h>

#define SCRATCHPAD_SUSPEND_INDICATOR_LOC    (0xf0800000 + 0x2400)
#define SCRATCHPAD_BASE_P       (0xf0800000+0x2400+0x100) /*IRAM_MEMORY_VADDR + Offset*/
#define SCRATCHPAD_CPU_CONTEXT_LOC_OFFSET 0x70
#define SCRATCHPAD_CPU_CONTEXT_LOC     SCRATCHPAD_BASE_P + SCRATCHPAD_CPU_CONTEXT_LOC_OFFSET


	.text
/* Function call to get the restore pointer for resume from OFF */
ENTRY(c2k_get_restore_pointer)
        stmfd   sp!, {lr}     @ save registers on stack
	ARM ( adr	r0, comcerto_cpu_restore )
        bic     r0, #PAGE_OFFSET
        ldmfd   sp!, {pc}     @ restore regs and return
ENTRY(get_restore_pointer_sz)
        .word   . - get_restore_pointer_sz




/*
 * ======================
 * == Idle entry point ==
 * ======================
 */

/*
 * Forces C2K into idle state
 *
 * comcerto_cpu_suspend() - This bit of code saves the CPU context if needed
 * and executes the WFI instruction. Calling WFI effectively changes the
 * power domains states to the desired target power states.
 */
	.align	3
ENTRY(comcerto_cpu_suspend)
	stmfd	sp!, {r4 - r11, lr}	@ save registers on stack

	/*
	 * r0 contains information about saving context:
	 *   0 - No context lost
	 *   1 - Only L1 and logic lost
	 *   2 - Only L2 lost (Even L1 is retained we clean it along with L2)
	 *   3 - Both L1 and L2 lost and logic lost
	 */

	/*
	 * For OFF mode: save context and jump to WFI (comcerto_do_wfi)
	 */
	cmp	r0, #0x0		@ If no context save required,
	beq	comcerto_do_wfi		@  jump to the WFI


	/* Otherwise fall through to the save context code */
comcerto_save_context_wfi:
	/*
	 * jump out to kernel flush routine
	 *  - reuse that code is better
	 *  - it executes in a cached space so is faster than refetch per-block
	 *  - should be faster and will change with kernel
	 *  - 'might' have to copy address, load and jump to it
	 * Flush all data from the L1 data cache before disabling
	 * SCTLR.C bit.
	 */
	ldr	r1, kernel_flush
	mov	lr, pc
	bx	r1

	/*
	 * Clear the SCTLR.C bit to prevent further data cache
	 * allocation. Clearing SCTLR.C would make all the data accesses
	 * strongly ordered and would not hit the cache.
	 */
	ARM ( mrc	p15, 0, r0, c1, c0, 0 )
	ARM ( bic	r0, r0, #(1 << 2) )	@ Disable the C bit
	ARM ( mcr	p15, 0, r0, c1, c0, 0 )
	ARM ( isb )

	/*
	 * Invalidate L1 data cache. Even though only invalidate is
	 * necessary exported flush API is used here. Doing clean
	 * on already clean cache would be almost NOP.
	 */
	ldr	r1, kernel_flush
	blx	r1
	/*
	 * The kernel doesn't interwork: v7_flush_dcache_all in particluar will
	 * always return in Thumb state when CONFIG_THUMB2_KERNEL is enabled.
	 * This sequence switches back to ARM.  Note that .align may insert a
	 * nop: bx pc needs to be word-aligned in order to work.
	 */
 THUMB(	.thumb		)
 THUMB(	.align		)
 THUMB(	bx	pc	)
 THUMB(	nop		)
	.arm

	b	comcerto_do_wfi

/*
 * Local variables
 */
kernel_flush:
	.word v7_flush_dcache_all


/* ===================================
 * == WFI instruction => Enter idle ==
 * ===================================
 */

/*
 * Do WFI instruction
 * Includes the resume path for non-OFF modes [TBD]
 */
	.align	3
ENTRY(comcerto_do_wfi)

	/* Store cpsr and spsr */
        ldr 	r7, scratchpad_cpu_context_loc

	/*mrs	r4, cpsr
	mrs	r5, spsr
	stmia	r7!, {r4-r5}*/



	/* Saving all the banked registers */
        mrs     r0, cpsr

        /* Save the Undef mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x1b
        msr     cpsr_c, r1
	ARM ( stmia	r7!, {r13-r14} )
	mrs	r13, spsr
	stmia	r7!, {r13}

        /* Save the Abort mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x17
        msr     cpsr_c, r1
	ARM ( stmia	r7!, {r13-r14} )
	mrs	r13, spsr
	stmia	r7!, {r13}

        /* Save the IRQ mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x12
        msr     cpsr_c, r1
	ARM ( stmia	r7!, {r13-r14} )
	mrs	r13, spsr
	stmia	r7!, {r13}

        /* Save the FIQ mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x11
        msr     cpsr_c, r1
	ARM ( stmia	r7!, {r8-r14} )
	THUMB ( stmia	r7!, {r8-r12} )
	mrs	r13, spsr
	stmia	r7!, {r13}

        /* Return to the original mode */
        msr     cpsr_c, r0

	/* We can Put DDR in self refresh mode here [TBD] */

	/* Pass control to UtilPE */
        ldr r4, scratchpad_reboot_indicator_loc
        mov r5, #0xFF
        str r5, [r4]

	/* Data memory barrier and Data sync barrier */
	dsb
	dmb

/*
 * ===================================
 * == WFI instruction => Enter idle ==
 * ===================================
 */
	wfi				@ wait for interrupt

/*
 * =========================================
 * ==  Resume path for non-OFF modes TBD  ==
 * =========================================
 */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

/*
 * ===================================
 * TBD
 * ===================================
 */

/*
 * ==============================
 * == Resume path for OFF mode ==
 * ==============================
 */

/*
 * The restore function is instaleed at 0x0 location
 * of DDR.
 */

ENTRY(comcerto_cpu_restore)
	ldr	r1, l2dis_3630
	cmp	r1, #0x1		/*@ Test if L2 re-enable needed on 3630*/
	bne	skipl2reen
	mrc	p15, 0, r1, c1, c0, 1
	orr	r1, r1, #2		/*@ re-enable L2 cache*/
	mcr	p15, 0, r1, c1, c0, 1
skipl2reen:


	/* Now branch to the common CPU resume function */
	blx	cpu_resume

	.ltorg

/*
 * Local variables
 */

l2dis_3630:
	.word


ENTRY(copy_words)
	cmp	r2, #0
	beq	f1
b0:
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	subs	r2, r2, #1
	bne	b0
f1:
	bx	lr




scratchpad_cpu_context_loc:
        .word   SCRATCHPAD_CPU_CONTEXT_LOC
scratchpad_reboot_indicator_loc:
        .word   SCRATCHPAD_SUSPEND_INDICATOR_LOC
