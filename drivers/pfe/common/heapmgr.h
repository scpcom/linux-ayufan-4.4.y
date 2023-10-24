/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _HEAPMGR_H_
#define _HEAPMGR_H_

#include "types.h"


#define HEAP_MINSIZE		16
#define HEAP_MINSIZE_LOG2	4
#define	HEAP_MAXSIZE		(64*K)
#define	HEAP_MAXSIZE_LOG2	16

typedef struct HEAPBLOCK_POINTER_t {
	struct HEAPBLOCK_POINTER_t *linkF;
	struct HEAPBLOCK_POINTER_t *linkB;
} HEAPBLOCK_POINTER, *PHEAPBLOCK_POINTER;


typedef struct HEAPSTATS_t {
	U32 size;
	U32 free;
} HEAPSTATS;

typedef struct HEAPDESC_t {
	HEAPBLOCK_POINTER avail[HEAP_MAXSIZE_LOG2 - HEAP_MINSIZE_LOG2 + 1];
	U8	*heapstart;
	U32	heapsize;
	U32	heap_minsize;
	U32	heap_minsize_log2;
	U32	m;
	U8	*tag;
	U32	free;
	volatile HEAPSTATS	*stats;
} HEAPDESC, *PHEAPDESC;


HANDLE __Heap_Init(PHEAPDESC phd, PVOID heapstart, U32 heapsize, U32 maxblocksize, U32 minblocksize, void *heap_stats_baseaddr, U8 *ptags) __attribute__ ((noinline));
PVOID __Heap_Alloc(HANDLE heap_handle, U32 size) __attribute__ ((noinline));
void __Heap_Free(HANDLE heap_handle, PVOID blockp) __attribute__ ((noinline));

PVOID Heap_Alloc(U32 size)  __attribute__((section ("fast_path"))) __attribute__ ((noinline));
PVOID Heap_Alloc_ARAM(U32 size) __attribute__((section ("fast_path")))  __attribute__ ((noinline));
void Heap_Free(PVOID p) __attribute__((section ("fast_path")))  __attribute__ ((noinline));

#endif /* _HEAPMGR_H_ */

