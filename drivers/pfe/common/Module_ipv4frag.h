/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_IPV4FRAG_H_
#define _MODULE_IPV4FRAG_H_

#include "types.h"
#include "fpart.h"
#include "channels.h"
#include "Module_ipv6frag.h" //use common definitions from ip6 reassembly include


typedef struct _tFragIP4
{
	
	struct _tFragIP4 	*next;

	U32			id;				/* fragment id		*/
	U32 		sAddr;
	U32 		dAddr;
	
	U32			refcnt;			/* number of fragments */
	int			timer;      	/* when will this queue expire */
	PMetadata	mtd_frags;   	/* linked list of packets */
	U32			cumul_len;   	/* cumulative len of received fragments */
	U8			last_in;    	/* first/last segment arrived? */
	U8			protocol;	/* layer 4 protocol */	
	U16			end;		
	U32			hash;
	U32			seq_num;
#if defined(COMCERTO_2000)
	struct _tFragIP4	*ddr_addr;
#endif
} FragIP4 , *PFragIP4;


extern PFragIP4 frag4_cache[];
extern FASTPART frag4Part;
extern FragIP4 Frag4Storage[];



void ipv4_frag_init(void);
BOOL M_ipv4_frag_init(PModuleDesc pModule);
void M_ipv4_frag_timer(void);
void M_ipv4_frag_entry(void);
#ifdef COMCERTO_2000_CLASS
void M_FRAG4_process_packet(PMetadata mtd);
#endif

#define HASH_FRAG4	HASH_FRAG6

#if defined(COMCERTO_2000_UTIL)
#include "util_dmem_storage.h"

#define frag_write_back(frag)		\
	efet_memcpy(frag->ddr_addr, frag, sizeof(*frag));

static __inline FragIP4 *frag4_alloc_part()
{
	PFragIP4 pFrag_ddr, pFrag_dmem=NULL;

	pFrag_ddr = SFL_alloc_part(&frag4Part);
	if(pFrag_ddr) {
		pFrag_dmem = (PFragIP4)FRAG_MTD_CACHE;
		pFrag_dmem->ddr_addr = pFrag_ddr;
		pFrag_dmem->next = NULL;
	}

	return pFrag_dmem;
}

#define frag_dmem2ddr(frag) 		(frag->ddr_addr)

static __inline void frag4_free_part(PFragIP4 pFrag_q4)
{
	SFL_free_part(&frag4Part, pFrag_q4->ddr_addr);
}

static __inline FragIP4 *frag4_ddr2dmem(PFragIP4 frag_ddr)
{
	PFragIP4 frag_dmem = (PFragIP4)FRAG_MTD_CACHE;
	efet_memcpy(frag_dmem, frag_ddr, sizeof(FragIP4));
	return frag_dmem;
}

static __inline FragIP4 *frag4_search(U32 hash_key_frag, U32 saddr, U32 daddr, U32 id)
{
	PFragIP4 pFrag_ddr, pFrag_dmem = NULL;

	pFrag_ddr = frag4_cache[hash_key_frag];

	while(pFrag_ddr != NULL) {
		pFrag_dmem = frag4_ddr2dmem(pFrag_ddr);
		if ((pFrag_dmem->dAddr == daddr) && (pFrag_dmem->sAddr == saddr) && (pFrag_dmem->id == id))
			goto frag_found;
		pFrag_ddr = pFrag_dmem->next;
	}
	return NULL;

frag_found:
	return pFrag_dmem;
}

#else
#define frag_dmem2ddr(frag) 		(frag)
#define frag4_ddr2dmem(frag)		(frag)
#define frag_write_back(frag)

static __inline FragIP4 *frag4_alloc_part()
{
	return SFL_alloc_part(&frag4Part);
}
static __inline void frag4_free_part(PFragIP4 pFrag_q4)
{
	SFL_free_part(&frag4Part, pFrag_q4);
}

static __inline FragIP4 *frag4_search(U32 hash_key_frag, U32 saddr, U32 daddr, U32 id)
{
	PFragIP4 pFrag_q4;

	pFrag_q4 = frag4_cache[hash_key_frag];
	while (pFrag_q4 != NULL && ((pFrag_q4->dAddr != daddr) || (pFrag_q4->sAddr != saddr) || (pFrag_q4->id != id)))
		pFrag_q4 = pFrag_q4->next;

	return pFrag_q4;
}
#endif

#endif /* _MODULE_IPV4FRAG_H_ */
