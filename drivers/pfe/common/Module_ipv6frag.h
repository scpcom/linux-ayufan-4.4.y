/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */



#ifndef _MODULE_IPV6FRAG_H_
#define _MODULE_IPV6FRAG_H_

#include "types.h"
#include "fpart.h"
#include "channels.h"
#include "module_timer.h"

#define NUM_FRAG_ENTRIES	128

#define FRAG_DROP	(1 << 2)
#define FRAG_FIRST_IN	(1 << 1)
#define FRAG_LAST_IN	(1 << 0)

#if defined (COMCERTO_2000)
#define FPP_MAX_IP_FRAG	(DDR_BUF_SIZE - (DDR_HDR_SIZE/2)) //2k DDR buffer - 128 head room
#else
#define FPP_MAX_IP_FRAG	1628 // 4Kb buffer - Linux skb_shared (164) - max stagger offset (2240)  - Baseoffset (64)
#endif
#define FPP_MAX_FRAG_NUM	2

#define IP_FRAG_TIMER_INTERVAL		(1 * TIMER_TICKS_PER_SEC)
#define FRAG_TIMEOUT			(10 * TIMER_TICKS_PER_SEC)

typedef struct _tFragIP6
{
	
	struct _tFragIP6 	*next;

	U32		id;					/* fragment id		*/
	U32 		sAddr[4];
	U32 		dAddr[4];
	
	U32				refcnt;		/* number of fragments */
	int				timer;      	/* when will this queue expire */
	PMetadata		mtd_frags;   /* linked list of packets */
	U32				cumul_len;   /* cumulative len of received fragments */
	U8				last_in;    	/* first/last segment arrived? */
	U8				protocol;	/* layer 4 protocol */	
	U16				end;		
	U32				hash;
	U32			seq_num;
} FragIP6 , *PFragIP6;


extern PFragIP6 frag6_cache[];
extern FASTPART frag6Part;
extern FragIP6 Frag6Storage[];

void ipv6_frag_init(void);
void M_ipv6_frag_timer(void);
void M_ipv6_frag_entry(void);
#if defined(COMCERTO_2000_CLASS)
void M_FRAG6_process_packet(PMetadata mtd);
#endif

static __inline U32 HASH_FRAG6(U32 Saddr, U32 Daddr, U32 id)
{
	U32 sum;
	sum = id + Saddr + ((Daddr << 7) | (Daddr >> 25));
	sum += (sum >> 16);
	return (sum ^ (sum >> 8)) & (NUM_FRAG_ENTRIES - 1);
}

#endif /* _MODULE_IPV6FRAG_H_ */
