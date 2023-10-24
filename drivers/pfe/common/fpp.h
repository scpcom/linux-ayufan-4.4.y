/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _FPP_H_
#define _FPP_H_

#include "fpp_globals.h"
#include "modules.h"
#include "channels.h"
#include "fpart.h"
#include "fpool.h"
#include "version.h"


#define ARAM_ABI		0x0A000120
#define ARAM_HEAP_STATS		0x0A000124
#define DDR_HEAP_STATS		0x0A00012C

#define CPU_BUSY_AVG		*((V32 *)0x0A000134)
#define CPU_IDLE_AVG		*((V32 *)0x0A000138)

#define GEM0_RESET_COUNT	*((V32 *)0x0A00013C)
#define GEM1_RESET_COUNT	*((V32 *)0x0A000140)


void fp_main(void);
void fp_scheduler(void) __attribute__((section ("fast_path"))) __attribute__ ((noinline)) __attribute__((noreturn));


extern FASTPART HostmsgPart;
#ifdef COMCERTO_100
extern FASTPART QMRateLimitEntryPart;
extern FASTPOOL PktBufferPool;
extern FASTPOOL PktBufferDDRPool;
#endif

/* global heap */
extern U8 GlobalAramHeap[];
extern U8 GlobalHeap[];

extern U8	DSCP_to_Q[];
extern U8	CTRL_to_Q;

#ifdef FPP_DIAGNOSTICS
extern void fppdiag_init(void);
extern void fppdiag_print(unsigned short log_id, unsigned int mod_id, const char* fmt, ...);
extern void fppdiag_exception_dump(U32* registers);
#endif

#endif /* _FPP_H_ */
