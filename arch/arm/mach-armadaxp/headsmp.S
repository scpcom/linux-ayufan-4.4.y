/*
 *  linux/arch/arm/mach-armadaxp/headsmp.S
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/memory.h>
#include <mach/hardware.h>

#define AXP_COHERENCY_FABRIC_CTL_REG (INTER_REGS_PHYS_BASE + 0x20200)
#define AXP_COHERENCY_FABRIC_CFG_REG (INTER_REGS_PHYS_BASE + 0x20204)

/*
 * specific entry point for secondary CPUs.  This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 */
ENTRY(axp_secondary_startup)
#ifdef CONFIG_CPU_ENDIAN_BE32
        /* convert CPU to big endian */
        .word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
        .word 0x800080e3 /* orr r0, r0, #0x80 */
        .word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */
#endif
#ifdef CONFIG_BE8_ON_LE
        setend  be
#endif

	/*
	 * Add CPU to coherency fabric
	 */
	mrc	p15, 0, r0, c0, c0, 5
	and	r0, r0, #15
	mov	r4, #1
	add	r5, r0, #24
	/* Add CPU to SMP group - Atomic */
	ldr	r2, =AXP_COHERENCY_FABRIC_CTL_REG
1:
	ldrex r3, [r2]
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	orr	r3, r3, r4, lsl r5
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	strex r1, r3, [r2]
	cmp	r1, #0
	bne 1b
	/* Enable coherency on CPU - Atomic*/
	ldr	r2, =AXP_COHERENCY_FABRIC_CFG_REG
1:
	ldrex r3, [r2]
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	orr	r3, r3, r4, lsl r5
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	strex r1, r3, [r2]
	cmp	r1, #0
	bne 1b

#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_4948
	/*
	 * Disable L0 on secondary CPU if flag set
	 */
	adr	r7, l0_disable_flag_addr
	ldr	r7, [r7]
	sub	r7, r7, #(PAGE_OFFSET - PLAT_PHYS_OFFSET)
	ldr	r7, [r7]
	cmp	r7, #0
	beq	l0_dis_skip
	mrc	p15, 1, r7, c15, c1, 0
	orr	r7, r7, #0x1
	mcr	p15, 1, r7, c15, c1, 0
l0_dis_skip:
#endif
#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_5315
	/*
	 * Speculative prefetch on secondary CPU if flag set
	 * Auxiliary Functional Modes Control 0 Register
	 * Bit[7]: 0-Enable, 1-Disable (reset default)
	 */
	adr	r7, sp_enable_flag_addr
	ldr	r7, [r7]
	sub	r7, r7, #(PAGE_OFFSET - PLAT_PHYS_OFFSET)
	ldr	r7, [r7]
	cmp	r7, #0
	beq	sp_ena_skip
	mrc	p15, 1, r7, c15, c2, 0
	bic	r7, r7, #(1 << 7)
	mcr	p15, 1, r7, c15, c2, 0	
sp_ena_skip:
#endif

	b	secondary_startup

#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_4948
l0_disable_flag_addr:
	.word	l0_disable_flag
#endif
#ifndef CONFIG_SHEEVA_ERRATA_ARM_CPU_5315
sp_enable_flag_addr:
	.word	sp_enable_flag
#endif
