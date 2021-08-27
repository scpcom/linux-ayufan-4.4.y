#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/arch/arm/mm/proc-sheeva_pj4bv7.S
 *
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This is the "shell" of the ARMv7 processor support.
 */
#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable.h>
#if defined(CONFIG_ARCH_ARMADA_XP)
#include <mach/armadaxp.h>
#elif defined(CONFIG_ARCH_ARMADA370)
#include <mach/armada370.h>
#include <mach/kw_macro.h>
#endif

#include "proc-macros.S"
#include "sheeva_pj4b-macros.S"

#define TTB_S		(1 << 1)
#define TTB_RGN_NC	(0 << 3)
#define TTB_RGN_OC_WBWA	(1 << 3)
#define TTB_RGN_OC_WT	(2 << 3)
#define TTB_RGN_OC_WB	(3 << 3)
#define TTB_NOS		(1 << 5)
#define TTB_IRGN_NC	((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA	((0 << 0) | (1 << 6))
#define TTB_IRGN_WT	((1 << 0) | (0 << 6))
#define TTB_IRGN_WB	((1 << 0) | (1 << 6))

/* PTWs cacheable, inner WB not shareable, outer WB not shareable */
#define TTB_FLAGS_UP	TTB_IRGN_WB|TTB_RGN_OC_WB
#define PMD_FLAGS_UP	PMD_SECT_WB

/* PTWs cacheable, inner WBWA shareable, outer WBWA not shareable */
#define TTB_FLAGS_SMP	TTB_IRGN_WBWA|TTB_S|TTB_NOS|TTB_RGN_OC_WBWA
#define PMD_FLAGS_SMP	PMD_SECT_WBWA|PMD_SECT_S

ENTRY(cpu_pj4bv7_proc_init)
	mov	pc, lr
ENDPROC(cpu_pj4bv7_proc_init)

ENTRY(cpu_pj4bv7_proc_fin)
	
#ifdef CONFIG_CACHE_AURORA_L2
	mcr	p15, 0, r0, c7, c10, 4		@ Data Synchronization Barrier
	ldr	r0, =0xffff			@ L2C clean all 16 ways
	ldr	r1, =AXP_L2_CLEAN_WAY_REG
	str	r0, [r1]
	mrc	p15, 0, r0, c0, c0, 5		@ Read CPU core number
	and	r0, r0, #0xF
	mov	r1, #0x1
	lsl	r0, r1, r0
1:	ldr	r1, =AXP_L2_MNTNC_STAT_REG	@ Read maintanence status to check done per CPU
	ldr	r1, [r1]
	tst	r0, r1
	bne	1b
	mcr	p15, 0, r0, c7, c10, 4		@ Data Synchronization Barrier
#endif
	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x0006			@ .............ca.
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	mov	pc, lr
	
ENDPROC(cpu_pj4bv7_proc_fin)

/*
++*	cpu_pj4bv7_reset(loc)
 *
 *	Perform a soft reset of the system.  Put the CPU into the
 *	same state as it would be if it had been reset, and branch
 *	to what would be the reset vector.
 *
 *	- loc   - location to jump to for soft reset
 */
	.align	5
ENTRY(cpu_pj4bv7_reset)
	mov	pc, r0
ENDPROC(cpu_pj4bv7_reset)

/*
++ *	cpu_pj4bv7_do_idle()
 *
 *	Idle the processor (eg, wait for interrupt).
 *
 *	IRQs are already disabled.
 */
ENTRY(cpu_pj4bv7_do_idle)
#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_BTS61
	dsb					@ WFI may enter a low-power mode
	wfi
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_4742
	mcr	p15, 0, r0, c7, c10, 4		@barrier
#endif
#endif	
	mov	pc, lr
ENDPROC(cpu_pj4bv7_do_idle)

ENTRY(cpu_pj4bv7_dcache_clean_area)
#ifndef TLB_CAN_READ_FROM_L1_CACHE
	dcache_line_size r2, r3
1:
#if defined CONFIG_SHEEVA_ERRATA_ARM_CPU_6043 || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6124) || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6076)
	mcr     p15, 0, r0, c7, c14, 1			@ clean & invalidate D entry
#else
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
#endif
	add	r0, r0, r2
	subs	r1, r1, r2
	bhi	1b
	dsb
#endif
	mov	pc, lr
ENDPROC(cpu_pj4bv7_dcache_clean_area)

/*
 *	cpu_pj4bv7_switch_mm(pgd_phys, tsk)
 *
 *	Set the translation table base pointer to be pgd_phys
 *
 *	- pgd_phys - physical address of new TTB
 *
 *	It is assumed that:
 *	- we are not using split page tables
 */
ENTRY(cpu_pj4bv7_switch_mm)
#ifdef CONFIG_MMU
	mov	r2, #0
	ldr	r1, [r1, #MM_CONTEXT_ID]	@ get mm->context.id
	ALT_SMP(orr	r0, r0, #TTB_FLAGS_SMP)
	ALT_UP(orr	r0, r0, #TTB_FLAGS_UP)
#ifdef CONFIG_ARM_ERRATA_430973
	mcr	p15, 0, r2, c7, c5, 6		@ flush BTAC/BTB
#endif
	mrc	p15, 0, r2, c2, c0, 1		@ load TTB 1
	mcr	p15, 0, r2, c2, c0, 0		@ into TTB 0
	isb
#ifdef CONFIG_ARM_ERRATA_754322
	dsb
#endif
	mcr	p15, 0, r1, c13, c0, 1		@ set context ID
	isb
	mcr	p15, 0, r0, c2, c0, 0		@ set TTB 0
	isb
#endif
	mov	pc, lr
ENDPROC(cpu_pj4bv7_switch_mm)

/*
 *	cpu_pj4bv7_set_pte_ext(ptep, pte)
 *
 *	Set a level 2 translation table entry.
 *
 *	- ptep  - pointer to level 2 translation table entry
 *		  (hardware version is stored at -1024 bytes)
 *	- pte   - PTE value to store
 *	- ext	- value for extended PTE bits
 */
ENTRY(cpu_pj4bv7_set_pte_ext)
#ifdef CONFIG_MMU
 
str	r1, [r0]			@ linux version
 
#ifdef CONFIG_MV_SUPPORT_64KB_PAGE_SIZE
	mov     r3, #0x7C
	and     r3, r3, r0
	mov     r3, r3, lsl #4
	bic     r0, r0, #0x3FC
	bic     r0, r0, #0x400
	orr     r0, r0, r3
#endif
	bic	r3, r1, #0x000003f0
#ifdef CONFIG_MV_SUPPORT_64KB_PAGE_SIZE
	bic	r3, r3, #0x00000F000
#endif
	bic	r3, r3, #PTE_TYPE_MASK
	orr	r3, r3, r2
#ifdef CONFIG_MV_SUPPORT_64KB_PAGE_SIZE
	orr	r3, r3, #PTE_EXT_AP0 | 1
#else
	orr	r3, r3, #PTE_EXT_AP0 | 2
#endif
	tst	r1, #1 << 4
	orrne	r3, r3, #PTE_EXT_TEX(1)

	eor	r1, r1, #L_PTE_DIRTY
	tst	r1, #L_PTE_RDONLY | L_PTE_DIRTY
	orrne	r3, r3, #PTE_EXT_APX

	tst	r1, #L_PTE_USER
	orrne	r3, r3, #PTE_EXT_AP1
#ifdef CONFIG_CPU_USE_DOMAINS
	@ allow kernel read/write access to read-only user pages
	tstne	r3, #PTE_EXT_APX
	bicne	r3, r3, #PTE_EXT_APX | PTE_EXT_AP0
#endif

	tst	r1, #L_PTE_XN
	orrne	r3, r3, #PTE_EXT_XN

	tst	r1, #L_PTE_YOUNG
	tstne	r1, #L_PTE_PRESENT
	moveq	r3, #0

 ARM(	str	r3, [r0, #2048]! )
 THUMB(	add	r0, r0, #2048 )
 THUMB(	str	r3, [r0] )
#ifdef CONFIG_MV_SUPPORT_64KB_PAGE_SIZE
       @ Need to duplicate the entry 16 times because of overlapping in PTE index bits.
       str     r3, [r0, #4]
       str     r3, [r0, #8]
       str     r3, [r0, #12]
       str     r3, [r0, #16]
       str     r3, [r0, #20]
       str     r3, [r0, #24]
       str     r3, [r0, #28]
#if defined CONFIG_SHEEVA_ERRATA_ARM_CPU_6043 || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6124) || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6076)
	mcr     p15, 0, r0, c7, c14, 1			@ clean & invalidate D entry
#else
        mcr     p15, 0, r0, c7, c10, 1          @ flush_pte
#endif
       add     r0, r0, #32
       str     r3, [r0]
       str     r3, [r0, #4]
       str     r3, [r0, #8]
       str     r3, [r0, #12]
       str     r3, [r0, #16]
       str     r3, [r0, #20]
       str     r3, [r0, #24]
       str     r3, [r0, #28]
#if defined CONFIG_SHEEVA_ERRATA_ARM_CPU_6043 || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6124) || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6076)
	mcr		p15, 0, r0, c7, c14, 1			@ clean & invalidate D entry
#else
	mcr		p15, 0, r0, c7, c10, 1			@ flush_pte
#endif
#else /* CONFIG_MV_SUPPORT_64KB_PAGE_SIZE */
#if defined CONFIG_SHEEVA_ERRATA_ARM_CPU_6043 || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6124) || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6076)
	mcr		p15, 0, r0, c7, c14, 1			@ clean & invalidate D entry
#else
	mcr		p15, 0, r0, c7, c10, 1			@ flush_pte
#endif
#endif /* CONFIG_MV_SUPPORT_64KB_PAGE_SIZE */
#endif
	mov	pc, lr
ENDPROC(cpu_pj4bv7_set_pte_ext)

cpu_pj4bv7_name:
	.ascii	"Marvell PJ4Bv7 Processor\0"
	.align

/*
	 * Memory region attributes with SCTLR.TRE=1
	 *
	 *   n = TEX[0],C,B
	 *   TR = PRRR[2n+1:2n]		- memory type
	 *   IR = NMRR[2n+1:2n]		- inner cacheable property
	 *   OR = NMRR[2n+17:2n+16]	- outer cacheable property
	 *
	 *			n	TR	IR	OR
	 *   UNCACHED		000	00
	 *   BUFFERABLE		001	10	00	00
	 *   WRITETHROUGH	010	10	10	10
	 *   WRITEBACK		011	10	11	11
	 *   reserved		110
	 *   WRITEALLOC		111	10	01	01
	 *   DEV_SHARED		100	01
	 *   DEV_NONSHARED	100	01
	 *   DEV_WC		001	10
	 *   DEV_CACHED		011	10
	 *
	 * Other attributes:
	 *
	 *   DS0 = PRRR[16] = 0		- device shareable property
	 *   DS1 = PRRR[17] = 1		- device shareable property
	 *   NS0 = PRRR[18] = 0		- normal shareable property
	 *   NS1 = PRRR[19] = 1		- normal shareable property
	 *   NOS = PRRR[24+n] = 1	- not outer shareable
	 */
.equ	PRRR,	0xff0a81a8
#ifdef CONFIG_AURORA_L2_OUTER_WA
.equ	NMRR,	0x40e040e0		/* Inner and outer WBWA */
#else
.equ	NMRR,	0xc0e040e0		/* Inner WBWA, outer WBnoWA*/
#endif

/* Suspend/resume support: derived from arch/arm/mach-s5pv210/sleep.S */
.globl	cpu_v7_suspend_size
.equ	cpu_v7_suspend_size, 4 * 13
#ifdef CONFIG_ARM_CPU_SUSPEND
ENTRY(cpu_v7_do_suspend)
	stmfd	sp!, {r4 - r11, lr}
	mrc	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mrc	p15, 0, r5, c13, c0, 3	@ User r/o thread ID
	stmia	r0!, {r4 - r5}
	mrc	p15, 1, r6, c15, c1, 0  @ save CP15 - extra features
	mrc	p15, 1, r7, c15, c2, 0	@ save CP15 - Aux Func Modes Ctrl 0
	mrc	p15, 1, r8, c15, c1, 2	@ save CP15 - Aux Debug Modes Ctrl 2
	mrc	p15, 1, r9, c15, c1, 1  @ save CP15 - Aux Debug Modes Ctrl 1
	mrc	p15, 0, r10, c9, c14, 0  @ save CP15 - PMC
	stmia	r0!, {r6 - r10}
	mrc	p15, 0, r6, c3, c0, 0	@ Domain ID
	mrc	p15, 0, r7, c2, c0, 1	@ TTB 1
	mrc	p15, 0, r11, c2, c0, 2	@ TTB control register
	mrc	p15, 0, r8, c1, c0, 0	@ Control register
	mrc	p15, 0, r9, c1, c0, 1	@ Auxiliary control register
	mrc	p15, 0, r10, c1, c0, 2	@ Co-processor access control
	stmia	r0, {r6 - r11}
	ldmfd	sp!, {r4 - r11, pc}
ENDPROC(cpu_v7_do_suspend)

ENTRY(cpu_v7_do_resume)
	mov	ip, #0
	mcr	p15, 0, ip, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, ip, c7, c5, 0	@ invalidate I cache
	mcr	p15, 0, ip, c13, c0, 1	@ set reserved context ID
	ldmia	r0!, {r4 - r5}
	mcr	p15, 0, r4, c13, c0, 0	@ FCSE/PID
	mcr	p15, 0, r5, c13, c0, 3	@ User r/o thread ID
	ldmia	r0!, {r6 - r10}
	mcr	p15, 1, r6, c15, c1, 0  @ save CP15 - extra features
	mcr	p15, 1, r7, c15, c2, 0	@ save CP15 - Aux Func Modes Ctrl 0
	mcr	p15, 1, r8, c15, c1, 2	@ save CP15 - Aux Debug Modes Ctrl 2
	mcr	p15, 1, r9, c15, c1, 1  @ save CP15 - Aux Debug Modes Ctrl 1
	mcr	p15, 0, r10, c9, c14, 0  @ save CP15 - PMC
	ldmia	r0, {r6 - r11}
	mcr	p15, 0, r6, c3, c0, 0	@ Domain ID
#ifndef CONFIG_ARM_LPAE
	ALT_SMP(orr	r1, r1, #TTB_FLAGS_SMP)
	ALT_UP(orr	r1, r1, #TTB_FLAGS_UP)
#endif
	mcr	p15, 0, r1, c2, c0, 0	@ TTB 0
	mcr	p15, 0, r7, c2, c0, 1	@ TTB 1
	mcr	p15, 0, r11, c2, c0, 2	@ TTB control register
	mrc	p15, 0, r4, c1, c0, 1	@ Read Auxiliary control register
	teq	r4, r9			@ Is it already set?
	mcrne	p15, 0, r9, c1, c0, 1	@ No, so write it
	mcr	p15, 0, r10, c1, c0, 2	@ Co-processor access control
	ldr	r4, =PRRR		@ PRRR
	ldr	r5, =NMRR		@ NMRR
	mcr	p15, 0, r4, c10, c2, 0	@ write PRRR
	mcr	p15, 0, r5, c10, c2, 1	@ write NMRR
	isb
	dsb
	mov	r0, r8			@ control register
	b	cpu_resume_mmu
ENDPROC(cpu_v7_do_resume)
#else
#define cpu_v7_do_suspend	0
#define cpu_v7_do_resume	0
#endif
	__CPUINIT

/*
++ *	__pj4bv7_setup
 *
 *	Initialise TLB, Caches, and MMU state ready to switch the MMU
 *	on.  Return in r0 the new CP15 C1 control register setting.
 *
 *	We automatically detect if we have a Harvard cache, and use the
 *	Harvard cache control instructions insead of the unified cache
 *	control instructions.
 *
 *	This should be able to cover all ARMv7 cores.
 *
 *	It is assumed that:
 *	- cache type register is implemented
 */
__pj4bv7_setup:
	sheeva_pj4b_config

#if defined(CONFIG_ARMADA_XP_REV_A0) || defined(CONFIG_ARMADA_XP_REV_B0)

#ifdef CONFIG_ARMADA_XP_A0_WITH_B0
/* Read and save SoC revision */
	ldr r0, =INTER_REGS_PHYS_BASE
	ldr r6, =0x18220		/* POWER_MNG_CTRL_REG */
	add r0, r0, r6
	ldr r5, [r0]
	orr r0, r5, #(1 << 5)	/* Enable PEX0 Clk */
	ldr r0, =INTER_REGS_PHYS_BASE
	ldr r6, =0x40008
	add r0, r0, r6
	ldr r0, [r0]
	and r0, r0, #0x3
	str r0, soc_revision
	ldr r0, =INTER_REGS_PHYS_BASE
	ldr r6, =0x18220		/* POWER_MNG_CTRL_REG */
	add r0, r0, r6
	str r5, [r0]
#endif

/* DSMP A0/B0 */
	/* Auxiliary Debug Modes Control 1 Register */
	mrc        p15, 1, r0, c15, c1, 1                         /* Read */
	orr        r0, r0, #0x00020                                /* BIT5 STREX backoff_disable--> '1' enable the back off of STREX instr */
	orr	   r0, r0, #0x00100                                /* BIT8 Internal Parity Handling Disable--> '1' Disable Internal Parity Handling */
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_6409
#ifdef CONFIG_ARMADA_XP_A0_WITH_B0
	skip_errata_if_axp_b0 r5
#endif
	bic        r0, r0, #0x4                                 /* Disable Static BP */
1:
#endif
	mcr        p15, 1, r0, c15, c1, 1                         /* Write */
	
/* Auxiliary Functional Modes Control Register 0 */
	mrc        p15, 1, r0, c15, c2, 0                         /* Read */
#if defined(CONFIG_SMP) || (defined(CONFIG_AURORA_IO_CACHE_COHERENCY) && \
			    defined(CONFIG_ARCH_ARMADA_XP))
	orr          r0, r0, #0x00002                                /* BIT1 SMP/nAMP --> '1' taking part in coherency */
#endif
	orr     r0, r0, #0x00004                                /* BIT2 L1 parity --> '1' Enabled */
	orr     r0, r0, #0x00100                                /* BIT8 FW --> '1' enable Cache and TLB maintenance broadcast */
	mcr        p15, 1, r0, c15, c2, 0                         /* Write */

	/* Auxiliary Debug Modes Control 0 Register */
	mrc        p15, 1, r0, c15, c1, 0                         /* Read */
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_6136
#ifdef CONFIG_ARMADA_XP_A0_WITH_B0
	skip_errata_if_axp_b0 r5
#endif
	orr          r0, r0, #0x001000                              /* BIT12 ldstm_first Two_sgl --> '1' Force first issue to be single */
	b 2f
1:
	bic          r0, r0, #0x001000                              /* BIT12 ldstm_first Two_sgl --> '0' The first issue is double word */
2:
#else
	bic          r0, r0, #0x001000                              /* BIT12 ldstm_first Two_sgl --> '0' The first issue is double word */
#endif
	orr          r0, r0, #0x400000                              /* BIT22 dvm_wakeup_dis --> '1' WFI/WFE will serve the DVM and back to idle */
	mcr        p15, 1, r0, c15, c1, 0                         /* Write */
	/* Auxiliary Debug Modes Control 2 Register */
	mrc        p15, 1, r0, c15, c1, 2                         /* Read */
	orr          r0, r0, #0x02000000                         /* BIT25 Intervention Interleave Disable--> '1'  Disable Interleaving with Intervention Data */
	orr          r0, r0, #0x08000000                         /* BIT27 CWF Disable--> '1' Disable critical word first sequencing */
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_5980
#ifdef CONFIG_ARMADA_XP_A0_WITH_B0
	skip_errata_if_axp_b0 r5
#endif
	orr          r0, r0, #0x20000000                         /* BIT29 DEV Stream Disable--> '1' Disable MO device read / write */
1:
#endif
	orr          r0, r0, #0x40000000                         /* BIT30 L1 Replacement Policy Config--> '1' Strict Round-Robin Replacement Policy  */
	mcr        p15, 1, r0, c15, c1, 2                         /* Write */

#else 
#ifdef CONFIG_SMP 
	ALT_SMP(mrc	p15, 0, r0, c1, c0, 1)
	ALT_UP(mov	r0, #(1 << 6))		@ fake it for UP
	tst	r0, #(1 << 6)			@ SMP/nAMP mode enabled?
	orreq	r0, r0, #(1 << 6) | (1 << 0)	@ Enable SMP/nAMP mode and
	mcreq	p15, 0, r0, c1, c0, 1		@ TLB ops broadcasting
#endif
#endif
#ifdef CONFIG_CPU_SHEEVA_PJ4B_PMC_ACCESS_IN_USERMODE
	@ Enable performance counters user access
	mrc     p15, 0, r0, c9, c14, 0
	orr     r0, r0, #0x1
	mcr     p15, 0, r0, c9, c14, 0
#endif /* CONFIG_CPU_SHEEVA_PJ4B_PMC_ACCESS_IN_USERMODE */
adr	r12, __pj4bv7_setup_stack		@ the local stack
	stmia	r12, {r0-r5, r7, r9, r11, lr}
	bl	v7_invalidate_dcache_all
	ldmia	r12, {r0-r5, r7, r9, r11, lr}

	mrc	p15, 0, r0, c0, c0, 0		@ read main ID register
	and	r10, r0, #0xff000000		@ ARM?
	teq	r10, #0x41000000
	bne	3f
	and	r5, r0, #0x00f00000		@ variant
	and	r6, r0, #0x0000000f		@ revision
	orr	r6, r6, r5, lsr #20-4		@ combine variant and revision
	ubfx	r0, r0, #4, #12			@ primary part number

	/* Cortex-A8 Errata */ 
	ldr	r10, =0x00000c08		@ Cortex-A8 primary part number
	teq	r0, r10
	bne	2f

#ifdef CONFIG_ARM_ERRATA_430973
	teq	r5, #0x00100000			@ only present in r1p*
	mrceq	p15, 0, r10, c1, c0, 1		@ read aux control register
	orreq	r10, r10, #(1 << 6)		@ set IBE to 1
	mcreq	p15, 0, r10, c1, c0, 1		@ write aux control register
#endif
#ifdef CONFIG_ARM_ERRATA_458693
	teq	r6, #0x20			@ only present in r2p0
	mrceq	p15, 0, r10, c1, c0, 1		@ read aux control register
	orreq	r10, r10, #(1 << 5)		@ set L1NEON to 1
	orreq	r10, r10, #(1 << 9)		@ set PLDNOP to 1
	mcreq	p15, 0, r10, c1, c0, 1		@ write aux control register
#endif
#ifdef CONFIG_ARM_ERRATA_460075
	teq	r6, #0x20			@ only present in r2p0
	mrceq	p15, 1, r10, c9, c0, 2		@ read L2 cache aux ctrl register
	tsteq	r10, #1 << 22
	orreq	r10, r10, #(1 << 22)		@ set the Write Allocate disable bit
	mcreq	p15, 1, r10, c9, c0, 2		@ write the L2 cache aux ctrl register
#endif
	b	3f
	/* Cortex-A9 Errata */
2:	ldr	r10, =0x00000c09		@ Cortex-A9 primary part number
	teq	r0, r10
	bne	3f
#ifdef CONFIG_ARM_ERRATA_742230
	cmp	r6, #0x22			@ only present up to r2p2
	mrcle	p15, 0, r10, c15, c0, 1		@ read diagnostic register
	orrle	r10, r10, #1 << 4		@ set bit #4
	mcrle	p15, 0, r10, c15, c0, 1		@ write diagnostic register
#endif
#ifdef CONFIG_ARM_ERRATA_742231
	teq	r6, #0x20			@ present in r2p0
	teqne	r6, #0x21			@ present in r2p1
	teqne	r6, #0x22			@ present in r2p2
	mrceq	p15, 0, r10, c15, c0, 1		@ read diagnostic register
	orreq	r10, r10, #1 << 12		@ set bit #12
	orreq	r10, r10, #1 << 22		@ set bit #22
	mcreq	p15, 0, r10, c15, c0, 1		@ write diagnostic register
#endif
#ifdef CONFIG_ARM_ERRATA_743622
	teq	r6, #0x20			@ present in r2p0
	teqne	r6, #0x21			@ present in r2p1
	teqne	r6, #0x22			@ present in r2p2
	mrceq	p15, 0, r10, c15, c0, 1		@ read diagnostic register
	orreq	r10, r10, #1 << 6		@ set bit #6
	mcreq	p15, 0, r10, c15, c0, 1		@ write diagnostic register
#endif
#ifdef CONFIG_ARM_ERRATA_751472
	cmp	r6, #0x30			@ present prior to r3p0
	mrclt	p15, 0, r10, c15, c0, 1		@ read diagnostic register
	orrlt	r10, r10, #1 << 11		@ set bit #11
	mcrlt	p15, 0, r10, c15, c0, 1		@ write diagnostic register
#endif

3:	mov	r10, #0
	mcr	p15, 0, r10, c7, c5, 0		@ I+BTB cache invalidate
	dsb
#ifdef CONFIG_MMU
	mcr	p15, 0, r10, c8, c7, 0		@ invalidate I + D TLBs
	mcr	p15, 0, r10, c2, c0, 2		@ TTB control register
			
	ALT_SMP(orr	r4, r4, #TTB_FLAGS_SMP)
	ALT_UP(orr	r4, r4, #TTB_FLAGS_UP)
	ALT_SMP(orr	r8, r8, #TTB_FLAGS_SMP)
	ALT_UP(orr	r8, r8, #TTB_FLAGS_UP)

	mcr	p15, 0, r8, c2, c0, 1		@ load TTB1
	
	ldr	r5, =PRRR			@ PRRR
	ldr	r6, =NMRR			@ NMRR
	mcr	p15, 0, r5, c10, c2, 0		@ write PRRR
	mcr	p15, 0, r6, c10, c2, 1		@ write NMRR
#endif
	adr	r5, v7_crval
	ldmia	r5, {r5, r6}
#ifdef CONFIG_CPU_ENDIAN_BE8
	orr	r6, r6, #1 << 25		@ big-endian page tables
#endif
#ifdef CONFIG_SWP_EMULATE
	orr     r5, r5, #(1 << 10)              @ set SW bit in "clear"
	bic     r6, r6, #(1 << 10)              @ clear it in "mmuset"
#endif
   	mrc	p15, 0, r0, c1, c0, 0		@ read control register
	bic	r0, r0, r5			@ clear bits them
	orr	r0, r0, r6			@ set them
 THUMB(	orr	r0, r0, #1 << 30	)	@ Thumb exceptions
	mov	pc, lr				@ return to head.S:__ret
ENDPROC(__pj4bv7_setup)

.global soc_revision
soc_revision:
	.word 0

	/*   AT
	 *  TFR   EV X F   I D LR    S
	 * .EEE ..EE PUI. .T.T 4RVI ZWRS BLDP WCAM
	 * rxxx rrxx xxx0 0101 xxxx xxxx x111 xxxx < forced
	 *    1    0 110       0011 1100 .111 1101 < we want
	 */
	.type	v7_crval, #object
v7_crval:
	crval	clear=0x0120c302, mmuset=0x10c03c7d, ucset=0x00c01c7c

__pj4bv7_setup_stack:
	.space	4 * 11				@ 11 registers
__INITDATA
	.type	v7_processor_functions, #object
ENTRY(v7_processor_functions)
	.word	v7_early_abort
	.word	v7_pabort
	.word	cpu_pj4bv7_proc_init
	.word	cpu_pj4bv7_proc_fin
	.word	cpu_pj4bv7_reset
	.word	cpu_pj4bv7_do_idle
	.word	cpu_pj4bv7_dcache_clean_area
	.word	cpu_pj4bv7_switch_mm
	.word	cpu_pj4bv7_set_pte_ext
	.word	cpu_v7_suspend_size
	.word	cpu_v7_do_suspend
	.word	cpu_v7_do_resume
	
	/*
	this whole 
		.word	cpu_v7_suspend_size
	.word	cpu_v7_do_suspend
	.word	cpu_v7_do_resume
	section above is a copy paste from proc-v7.s and need to be revisted
	*/
	
	.size	v7_processor_functions, . - v7_processor_functions

	.section ".rodata"
	.type	cpu_arch_name, #object
cpu_arch_name:
	.asciz	"armv7"
	.size	cpu_arch_name, . - cpu_arch_name

	.type	cpu_elf_name, #object
cpu_elf_name:
	.asciz	"v7"
	.size	cpu_elf_name, . - cpu_elf_name
	.align

	.section ".proc.info.init", #alloc, #execinstr

	/*
	 * Match any ARMv7 processor core.
	 */
	.type	__v7_proc_info, #object
__v7_proc_info:
	.long	0x000f0000		@ Required ID value
	.long	0x000f0000		@ Mask for ID

	ALT_SMP(.long \
		PMD_TYPE_SECT | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ | \
		PMD_FLAGS_SMP)
	ALT_UP(.long \
		PMD_TYPE_SECT | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ | \
		PMD_FLAGS_UP)
	.long   PMD_TYPE_SECT | \
		PMD_SECT_XN | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	W(b)	__pj4bv7_setup
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP|HWCAP_HALF|HWCAP_THUMB|HWCAP_FAST_MULT|HWCAP_EDSP|HWCAP_TLS
	.long	cpu_pj4bv7_name
	.long	v7_processor_functions
	.long	v7wbi_tlb_fns
	.long	v6_user_fns
	.long	v7_cache_fns
	.size	__v7_proc_info, . - __v7_proc_info
