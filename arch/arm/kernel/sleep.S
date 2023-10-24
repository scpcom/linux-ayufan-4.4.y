#include <linux/linkage.h>
#include <linux/threads.h>
#include <asm/asm-offsets.h>
#include <asm/assembler.h>
#include <asm/glue-cache.h>
#include <asm/glue-proc.h>
#include <asm/system.h>

#ifdef CONFIG_ARCH_M86XXX
/* 
 * IRAM Location to restore the banked registers
 */
#define SCRATCHPAD_CPU_CONTEXT_LOC_DBG	 (0xf0802570)
#endif
	.text

/*
 * Save CPU state for a suspend.  This saves the CPU general purpose
 * registers, and allocates space on the kernel stack to save the CPU
 * specific registers and some other data for resume.
 *  r0 = suspend function arg0
 *  r1 = suspend function
 */
ENTRY(__cpu_suspend)
	stmfd	sp!, {r4 - r11, lr}
#ifdef MULTI_CPU
	ldr	r10, =processor
	ldr	r4, [r10, #CPU_SLEEP_SIZE] @ size of CPU sleep state
#else
	ldr	r4, =cpu_suspend_size
#endif
	mov	r5, sp			@ current virtual SP
	add	r4, r4, #12		@ Space for pgd, virt sp, phys resume fn
	sub	sp, sp, r4		@ allocate CPU state on stack
	stmfd	sp!, {r0, r1}		@ save suspend func arg and pointer
	add	r0, sp, #8		@ save pointer to save block
	mov	r1, r4			@ size of save block
	mov	r2, r5			@ virtual SP
	ldr	r3, =sleep_save_sp
#ifdef CONFIG_SMP
	ALT_SMP(mrc p15, 0, lr, c0, c0, 5)
	ALT_UP(mov lr, #0)
	and	lr, lr, #15
	add	r3, r3, lr, lsl #2
#endif
	bl	__cpu_suspend_save
	adr	lr, BSYM(cpu_suspend_abort)
	ldmfd	sp!, {r0, pc}		@ call suspend fn
ENDPROC(__cpu_suspend)
	.ltorg

cpu_suspend_abort:
	ldmia	sp!, {r1 - r3}		@ pop phys pgd, virt SP, phys resume fn
	teq	r0, #0
	moveq	r0, #1			@ force non-zero value
	mov	sp, r2
	ldmfd	sp!, {r4 - r11, pc}
ENDPROC(cpu_suspend_abort)

/*
 * r0 = control register value
 */
	.align	5
ENTRY(cpu_resume_mmu)
	ldr	r3, =cpu_resume_after_mmu
	mcr	p15, 0, r0, c1, c0, 0	@ turn on MMU, I-cache, etc
	mrc	p15, 0, r0, c0, c0, 0	@ read id reg
	mov	r0, r0
	mov	r0, r0
	mov	pc, r3			@ jump to virtual address
ENDPROC(cpu_resume_mmu)
cpu_resume_after_mmu:
	bl	cpu_init		@ restore the und/abt/irq banked regs

#ifdef CONFIG_ARCH_M86XXX
	/* 
	 * This code is added to restore the banked registers
	 * This restore has to be kept here as it has to happen 
	 * after MMU int.
	 */

	/* Restore cpsr and spsr */
        ldr 	r7, scratchpad_cpu_context_loc_dbg

	/* Restoring all the banked registers */
        mrs     r0, cpsr

        /* Restore the Undef mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x1b
        msr     cpsr_c, r1
	ARM ( ldmia	r7!, {r13-r14} )
	ldmia	r7!, {r6}
	msr	spsr, r6

        /* Restore the Abort mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x17
        msr     cpsr_c, r1
	ARM ( ldmia	r7!, {r13-r14} )
	ldmia	r7!, {r6}
	msr	spsr, r6

        /* Restore the IRQ mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x12
        msr     cpsr_c, r1
	ARM ( ldmia	r7!, {r13-r14} )
	ldmia	r7!, {r6}
	msr	spsr, r6

        /* Restore the FIQ mode reisters */
        bic     r1, r0, #0x1f
        orr     r1, r1, #0x11
        msr     cpsr_c, r1
	ARM ( ldmia	r7!, {r8-r14} )
	THUMB ( ldmia	r7!, {r8-r12} )
	ldmia	r7!, {r6}
	msr	spsr, r6

        /* Return to the original mode */
        msr     cpsr_c, r0
#endif
	mov	r0, #0			@ return zero on success
	ldmfd	sp!, {r4 - r11, pc}
ENDPROC(cpu_resume_after_mmu)


#ifdef CONFIG_ARCH_M86XXX
scratchpad_cpu_context_loc_dbg:
        .word   SCRATCHPAD_CPU_CONTEXT_LOC_DBG
#endif

/*
 * Note: Yes, part of the following code is located into the .data section.
 *       This is to allow sleep_save_sp to be accessed with a relative load
 *       while we can't rely on any MMU translation.  We could have put
 *       sleep_save_sp in the .text section as well, but some setups might
 *       insist on it to be truly read-only.
 */
	.data
	.align
ENTRY(cpu_resume)
#ifdef CONFIG_SMP
	adr	r0, sleep_save_sp
	ALT_SMP(mrc p15, 0, r1, c0, c0, 5)
	ALT_UP(mov r1, #0)
	and	r1, r1, #15
	ldr	r0, [r0, r1, lsl #2]	@ stack phys addr
#else
	ldr	r0, sleep_save_sp	@ stack phys addr
#endif
	setmode	PSR_I_BIT | PSR_F_BIT | SVC_MODE, r1  @ set SVC, irqs off
	@ load phys pgd, stack, resume fn
  ARM(	ldmia	r0!, {r1, sp, pc}	)
THUMB(	ldmia	r0!, {r1, r2, r3}	)
THUMB(	mov	sp, r2			)
THUMB(	bx	r3			)
ENDPROC(cpu_resume)

sleep_save_sp:
	.rept	CONFIG_NR_CPUS
	.long	0				@ preserve stack phys ptr here
	.endr
