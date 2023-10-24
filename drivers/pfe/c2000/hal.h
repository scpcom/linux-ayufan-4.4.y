/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _HAL_H_
#define _HAL_H_

#include "types.h"

#include "common_hal.h"

#define	NUM_PE_CLASS	6
#define	NUM_PE_TMU	4

#if defined(COMCERTO_2000_CONTROL)

#include "pfe_ctrl_hal.h" /* This file comes from pfe_ctrl project */
#include "pfe/pfe.h"

#undef noinline /* Linux redefines this, undo it here */

#else

#define DMEM_SH(var) __attribute__((section(".dmem_sh_" #var))) var
#define LMEM_SH(var) __attribute__((section(".lmem_sh_" #var))) var
#define DDR_SH(var) __attribute__((section(".ddr_sh_" #var))) var

typedef U64 u64;
typedef U32 u32;
typedef U16 u16;
typedef U8 u8;

#define __writel(val, addr) 	(*(volatile u32 *)(addr) = ((u32)(val)))
#define readl(addr) 		(*(volatile u32 *)(addr))

#define nop_multiple(count)	do {							\
					int i;						\
					for (i = 0; i < count; i++)			\
						asm volatile("nop" ::: "memory");	\
				} while (0)

#define nop() nop_multiple(1)

/* AXI writes immediately followed by an AXI read, cause writes to be lost,
as a workaround, add a nop after each write */
#define writel(val, addr) \
				do {				\
					__writel(val, addr);	\
					nop();			\
				} while (0)

#include "pe_status.h"
#include <stdarg.h>
#include "pfe/pe.h"

#if defined(COMCERTO_2000_CLASS)

#define PE_LMEM_SH(var) __attribute__((section(".pe_lmem_sh_" #var))) var

#include "pfe/class.h"
#elif defined(COMCERTO_2000_TMU)
#include "pfe/tmu.h"
#elif defined(COMCERTO_2000_UTIL)
#include "pfe/util.h"
#endif
#endif

#include "pe_status.h"

#endif /* _HAL_H_ */

