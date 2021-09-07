#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#if defined(MY_DEF_HERE)
/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All rights reserved.
 *	Scott Wood <scottwood@freescale.com>
 *	Dave Liu <daveliu@freescale.com>
 * implement the L2 cache operations of e500 based L2 controller
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <asm/reg.h>
#include <asm/cputable.h>
#include <asm/ppc_asm.h>
#include <asm/asm-offsets.h>

	.section .text

	/* r3 = virtual address of L2 controller, WIMG = 01xx */
_GLOBAL(flush_disable_L2)
	/* It's a write-through cache, so only invalidation is needed. */
	mbar
	isync
	lwz	r4, 0(r3)
	li	r5, 1
	rlwimi	r4, r5, 30, 0xc0000000
	stw	r4, 0(r3)

	/* Wait for the invalidate to finish */
1:	lwz	r4, 0(r3)
	andis.	r4, r4, 0x4000
	bne	1b
	mbar

	blr

	/* r3 = virtual address of L2 controller, WIMG = 01xx */
_GLOBAL(invalidate_enable_L2)
	mbar
	isync
	lwz	r4, 0(r3)
	li	r5, 3
	rlwimi	r4, r5, 30, 0xc0000000
	stw	r4, 0(r3)

	/* Wait for the invalidate to finish */
1:	lwz	r4, 0(r3)
	andis.	r4, r4, 0x4000
	bne	1b
	mbar

	blr
#endif /* MY_DEF_HERE */
