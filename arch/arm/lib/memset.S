/*
 *  linux/arch/arm/lib/memset.S
 *
 *  Copyright (C) 1995-2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  ASM optimised string functions
 */
#include <linux/linkage.h>
#include <asm/assembler.h>

	.text
	.align	5
#ifdef CONFIG_SYNO_ALPINE
//do nothing
#else
	.word	0

1:	subs	r2, r2, #4		@ 1 do we have enough
	blt	5f			@ 1 bytes to align with?
	cmp	r3, #2			@ 1
	strltb	r1, [r0], #1		@ 1
	strleb	r1, [r0], #1		@ 1
	strb	r1, [r0], #1		@ 1
	add	r2, r2, r3		@ 1 (r2 = r2 - (4 - r3))
/*
 * The pointer is now aligned and the length is adjusted.  Try doing the
 * memset again.
 */
#endif

ENTRY(memset)
	ands	r3, r0, #3		@ 1 unaligned?
#ifdef CONFIG_SYNO_ALPINE
	mov	ip, r0			@ preserve r0 as return value
	bne	6f			@ 1
/*
 * we know that the pointer in ip is aligned to a word boundary.
 */
1:	orr	r1, r1, r1, lsl #8
#else
	bne	1b			@ 1
/*
 * we know that the pointer in r0 is aligned to a word boundary.
 */
	orr	r1, r1, r1, lsl #8
#endif
	orr	r1, r1, r1, lsl #16
	mov	r3, r1
	cmp	r2, #16
	blt	4f

#if ! CALGN(1)+0

#ifdef CONFIG_SYNO_ALPINE
/*
 * We need 2 extra registers for this loop - use r8 and the LR
 */
	stmfd	sp!, {r8, lr}
	mov	r8, r1
#else
/*
 * We need an extra register for this loop - save the return address and
 * use the LR
 */
	str	lr, [sp, #-4]!
	mov	ip, r1
#endif
	mov	lr, r1

2:	subs	r2, r2, #64
#ifdef CONFIG_SYNO_ALPINE
	stmgeia	ip!, {r1, r3, r8, lr}	@ 64 bytes at a time.
	stmgeia	ip!, {r1, r3, r8, lr}
	stmgeia	ip!, {r1, r3, r8, lr}
	stmgeia	ip!, {r1, r3, r8, lr}
#else
	stmgeia	r0!, {r1, r3, ip, lr}	@ 64 bytes at a time.
	stmgeia	r0!, {r1, r3, ip, lr}
	stmgeia	r0!, {r1, r3, ip, lr}
	stmgeia	r0!, {r1, r3, ip, lr}
#endif
	bgt	2b
#ifdef CONFIG_SYNO_ALPINE
	ldmeqfd	sp!, {r8, pc}		@ Now <64 bytes to go.
#else
	ldmeqfd	sp!, {pc}		@ Now <64 bytes to go.
#endif
/*
 * No need to correct the count; we're only testing bits from now on
 */
	tst	r2, #32
#ifdef CONFIG_SYNO_ALPINE
	stmneia	ip!, {r1, r3, r8, lr}
	stmneia	ip!, {r1, r3, r8, lr}
#else
	stmneia	r0!, {r1, r3, ip, lr}
	stmneia	r0!, {r1, r3, ip, lr}
#endif
	tst	r2, #16
#ifdef CONFIG_SYNO_ALPINE
	stmneia	ip!, {r1, r3, r8, lr}
	ldmfd	sp!, {r8, lr}
#else
	stmneia	r0!, {r1, r3, ip, lr}
	ldr	lr, [sp], #4
#endif

#else

/*
 * This version aligns the destination pointer in order to write
 * whole cache lines at once.
 */

#ifdef CONFIG_SYNO_ALPINE
	stmfd	sp!, {r4-r8, lr}
#else
	stmfd	sp!, {r4-r7, lr}
#endif
	mov	r4, r1
	mov	r5, r1
	mov	r6, r1
	mov	r7, r1
#ifdef CONFIG_SYNO_ALPINE
	mov	r8, r1
#else
	mov	ip, r1
#endif
	mov	lr, r1

	cmp	r2, #96
#ifdef CONFIG_SYNO_ALPINE
	tstgt	ip, #31
#else
	tstgt	r0, #31
#endif
	ble	3f

#ifdef CONFIG_SYNO_ALPINE
	and	r8, ip, #31
	rsb	r8, r8, #32
	sub	r2, r2, r8
	movs	r8, r8, lsl #(32 - 4)
	stmcsia	ip!, {r4, r5, r6, r7}
	stmmiia	ip!, {r4, r5}
	tst	r8, #(1 << 30)
	mov	r8, r1
	strne	r1, [ip], #4
#else
	and	ip, r0, #31
	rsb	ip, ip, #32
	sub	r2, r2, ip
	movs	ip, ip, lsl #(32 - 4)
	stmcsia	r0!, {r4, r5, r6, r7}
	stmmiia	r0!, {r4, r5}
	tst	ip, #(1 << 30)
	mov	ip, r1
	strne	r1, [r0], #4
#endif

3:	subs	r2, r2, #64
#ifdef CONFIG_SYNO_ALPINE
	stmgeia	ip!, {r1, r3-r8, lr}
	stmgeia	ip!, {r1, r3-r8, lr}
#else
	stmgeia	r0!, {r1, r3-r7, ip, lr}
	stmgeia	r0!, {r1, r3-r7, ip, lr}
#endif
	bgt	3b
#ifdef CONFIG_SYNO_ALPINE
	ldmeqfd	sp!, {r4-r8, pc}
#else
	ldmeqfd	sp!, {r4-r7, pc}
#endif

	tst	r2, #32
#ifdef CONFIG_SYNO_ALPINE
	stmneia	ip!, {r1, r3-r8, lr}
#else
	stmneia	r0!, {r1, r3-r7, ip, lr}
#endif
	tst	r2, #16
#ifdef CONFIG_SYNO_ALPINE
	stmneia	ip!, {r4-r7}
	ldmfd	sp!, {r4-r8, lr}
#else
	stmneia	r0!, {r4-r7}
	ldmfd	sp!, {r4-r7, lr}
#endif

#endif

4:	tst	r2, #8
#ifdef CONFIG_SYNO_ALPINE
	stmneia	ip!, {r1, r3}
#else
	stmneia	r0!, {r1, r3}
#endif
	tst	r2, #4
#ifdef CONFIG_SYNO_ALPINE
	strne	r1, [ip], #4
#else
	strne	r1, [r0], #4
#endif
/*
 * When we get here, we've got less than 4 bytes to zero.  We
 * may have an unaligned pointer as well.
 */
5:	tst	r2, #2
#ifdef CONFIG_SYNO_ALPINE
	strneb	r1, [ip], #1
	strneb	r1, [ip], #1
#else
	strneb	r1, [r0], #1
	strneb	r1, [r0], #1
#endif
	tst	r2, #1
#ifdef CONFIG_SYNO_ALPINE
	strneb	r1, [ip], #1
#else
	strneb	r1, [r0], #1
#endif
	mov	pc, lr
#ifdef CONFIG_SYNO_ALPINE
6:	subs	r2, r2, #4		@ 1 do we have enough
	blt	5b			@ 1 bytes to align with?
	cmp	r3, #2			@ 1
	strltb	r1, [ip], #1		@ 1
	strleb	r1, [ip], #1		@ 1
	strb	r1, [ip], #1		@ 1
	add	r2, r2, r3		@ 1 (r2 = r2 - (4 - r3))
	b	1b
#endif
ENDPROC(memset)
