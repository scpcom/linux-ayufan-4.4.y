/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _PE_STATUS_H_
#define _PE_STATUS_H_

#include "types.h"
#include "hal.h"

typedef struct tPE_STATUS
{
	U32	cpu_state;
	U32	activity_counter;
	U32	rx;
	U32	tx;
	U32	drop;
#if defined(PE_DEBUG)
	U32	debug_indicator;
	U32	debug[16];
#endif
} __attribute__((aligned(16))) PE_STATUS;

extern volatile PE_STATUS pe_status;

#define PESTATUS_SETSTATE(newstate) do { pe_status.cpu_state = (U32)(newstate); } while (0)
#define PESTATUS_INCR_ACTIVE() do { pe_status.activity_counter++; } while (0)
#define PESTATUS_INCR_RX() do { pe_status.rx++; } while (0)
#define PESTATUS_INCR_TX() do { pe_status.tx++; } while (0)
#define PESTATUS_SETERROR(err) do { pe_status.activity_counter = (U32)(err); } while (0)
#define PESTATUS_INCR_DROP() do { pe_status.drop++; } while (0)
#if defined(PE_DEBUG)
#define PEDEBUG_SET(i, val) do { pe_status.debug[i] = (U32)(val); } while (0)
#define PEDEBUG_INCR(i) do { pe_status.debug[i]++; } while (0)
#else
#define PEDEBUG_SET(i, val) do { } while (0)
#define PEDEBUG_INCR(i) do { } while (0)
#endif

#endif /* _PE_STATUS_H_ */
