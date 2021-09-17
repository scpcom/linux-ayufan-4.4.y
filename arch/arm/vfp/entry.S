#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/arch/arm/vfp/entry.S
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#if defined(MY_DEF_HERE)
#include <linux/init.h>
#include <linux/linkage.h>
#endif /* MY_DEF_HERE */
#include <asm/thread_info.h>
#include <asm/vfpmacros.h>
#if defined(MY_DEF_HERE)
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#else /* MY_DEF_HERE */
#include "../kernel/entry-header.S"
#endif /* MY_DEF_HERE */

@ VFP entry point.
@
@  r0  = instruction opcode (32-bit ARM or two 16-bit Thumb)
@  r2  = PC value to resume execution after successful emulation
@  r9  = normal "successful" return address
@  r10 = this threads thread_info structure
@  lr  = unrecognised instruction return address
#if defined(MY_DEF_HERE)
@  IRQs enabled.
#else /* MY_DEF_HERE */
@  IRQs disabled.
#endif /* MY_DEF_HERE */
@
ENTRY(do_vfp)
#if defined(MY_DEF_HERE)
	inc_preempt_count r10, r4
#else /* MY_DEF_HERE */
#ifdef CONFIG_PREEMPT_COUNT
	ldr	r4, [r10, #TI_PREEMPT]	@ get preempt count
	add	r11, r4, #1		@ increment it
	str	r11, [r10, #TI_PREEMPT]
#endif
	enable_irq
#endif /* MY_DEF_HERE */
 	ldr	r4, .LCvfp
	ldr	r11, [r10, #TI_CPU]	@ CPU number
	add	r10, r10, #TI_VFPSTATE	@ r10 = workspace
	ldr	pc, [r4]		@ call VFP entry point
ENDPROC(do_vfp)

ENTRY(vfp_null_entry)
#if defined(MY_DEF_HERE)
	dec_preempt_count_ti r10, r4
#else /* MY_DEF_HERE */
#ifdef CONFIG_PREEMPT_COUNT
	get_thread_info	r10
	ldr	r4, [r10, #TI_PREEMPT]	@ get preempt count
	sub	r11, r4, #1		@ decrement it
	str	r11, [r10, #TI_PREEMPT]
#endif
#endif /* MY_DEF_HERE */
	mov	pc, lr
ENDPROC(vfp_null_entry)

	.align	2
.LCvfp:
	.word	vfp_vector

@ This code is called if the VFP does not exist. It needs to flag the
@ failure to the VFP initialisation code.

	__INIT
ENTRY(vfp_testing_entry)
#if defined(MY_DEF_HERE)
	dec_preempt_count_ti r10, r4
#else /* MY_DEF_HERE */
#ifdef CONFIG_PREEMPT_COUNT
	get_thread_info	r10
	ldr	r4, [r10, #TI_PREEMPT]	@ get preempt count
	sub	r11, r4, #1		@ decrement it
	str	r11, [r10, #TI_PREEMPT]
#endif
#endif /* MY_DEF_HERE */
	ldr	r0, VFP_arch_address
	str	r0, [r0]		@ set to non-zero value
	mov	pc, r9			@ we have handled the fault
ENDPROC(vfp_testing_entry)

	.align	2
VFP_arch_address:
	.word	VFP_arch

	__FINIT
