/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_MC4_H_
#define _MODULE_MC4_H_

#include "types.h"
#include "modules.h"
#include "channels.h"
#include "multicast.h"

#define MC4_MAX_LISTENERS_PER_GROUP MCx_MAX_LISTENERS_PER_GROUP 

#define MC4_NUM_HASH_ENTRIES 16

#define INVALID_IDX (-1)
typedef struct tMC4_context {

  	unsigned char ttl_check_rule;
}MC4_context;


#if defined (COMCERTO_2000)

#if defined (COMCERTO_2000_CONTROL)

/** Control path MC4 HW entry */
typedef struct _thw_MC4Entry {
	U32 	flags;
	U32 	dma_addr;
	U32 next;
	U32 src_addr;
	U32 dst_addr;
	/* common portion */
	MCxEntry mcdest;
	U16 status;
	U8 padding[2];  //structure size must be a multiple of 8 bytes for EFET transfers

	/* These fields are only used by host software, so keep them at the end of the structure */
	struct dlist_head 	list;
	struct _tMC4Entry	*sw_entry;	/**< pointer to the software flow entry */
	unsigned long		removal_time;
}hw_MC4Entry, *Phw_MC4Entry;

/** Control path MC4 SW entry */
typedef struct _tMC4Entry {
	struct slist_entry list;
	U32 src_addr;
	U32 dst_addr;
	/* common portion */
	MCxEntry mcdest;
	U16 status;
	U8 padding[2];  //structure size must be a multiple of 8 bytes for EFET transfers

	hw_MC4Entry *hw_entry;
}MC4Entry, *PMC4Entry;


#else
/** Data path MC4 entry */
typedef struct _tMC4Entry {
	U32 	flags;
	U32 	dma_addr;
	U32 	next;
	U32 src_addr;
	U32 dst_addr;
	/* common portion */
	MCxEntry mcdest;
	U16 status;
	U8 padding[2];  //structure size must be a multiple of 8 bytes for EFET transfers
}MC4Entry, *PMC4Entry;
#endif

#else /* COMCERTO 1000 */
typedef struct _tMC4Entry {
	struct slist_entry list;
	U32 src_addr;
	U32 dst_addr;
	/* common portion */
	MCxEntry mcdest;
	U16 status;
	U8 padding[2];  //structure size must be a multiple of 8 bytes for EFET transfers
}MC4Entry, *PMC4Entry;
#endif



/***********************************
* MC4 API Command and Entry structures
*
************************************/
typedef struct _tMC4Output {
	U32		timer;
	U8		output_device_str[11];
	U8 		shaper_mask;
}MC4Output, *PMC4Output;

typedef struct _tMC4Command {
	U16		action;
	U8		src_addr_mask;
	U8 		mode : 1,
	     		queue : 5,
	     		rsvd : 2;
	U32		src_addr;
	U32		dst_addr;
	U32		num_output;
	MC4Output output_list[MC4_MAX_LISTENERS_PER_GROUP];
}MC4Command, *PMC4Command;
#define MC4_MIN_COMMAND_SIZE	32 /* with one listener entry using 1 interface name */


extern struct tMC4_context gMC4Ctx;
#if !defined(COMCERTO_2000) || defined(COMCERTO_2000_CONTROL)
extern struct slist_head mc4_table_memory[MC4_NUM_HASH_ENTRIES];
#else
extern PVOID mc4_table_memory[MC4_NUM_HASH_ENTRIES];
#endif

BOOL mc4_init(void);
void mc4_exit(void);
void M_MC4_process_packet(PMetadata mtd) __attribute__((section ("fast_path")));
#if !defined(COMCERTO_2000)
void M_mc4_entry(void) __attribute__((section ("fast_path")));
#endif

void MC4_interface_purge(U32 if_index);

int MC4_handle_MULTICAST(U16 *p, U16 Length);
int MC4_handle_RESET (void);
int MC4_check_entry(struct tMetadata *mtd);

/* TODO use a better hash function
 */
#define HASH_MC4(x)  (ntohl(x)&(MC4_NUM_HASH_ENTRIES -1))

#ifdef COMCERTO_2000_CLASS
/** Searches multicast forwarding table for match based on source and destination IPv4 addresses.
 * @param[in] srca	IPv4 source address
 * @param[in] dsta	IPv4 destination address
 * @param[in] mtd	pointer to metadata structure for packet (needed to determine route entry location in DMEM)
 * @return pointer to matching entry, or NULL if no entry found.
 */
static __inline MC4Entry *MC4_rule_search_class(PMetadata mtd, ipv4_hdr_t *ipv4_hdr)
{
	PMC4Entry dmem_entry;
	volatile PMC4Entry ddr_entry;
	U32 srca = READ_UNALIGNED_INT(ipv4_hdr->SourceAddress);
	U32 dsta = READ_UNALIGNED_INT(ipv4_hdr->DestinationAddress);
	U32 hash_key = HASH_MC4(dsta);

	ddr_entry = mc4_table_memory[hash_key];
	// TODO define global variable in class PE code pointing to route entry location in DMEM, to avoid passing mtd as argument
	dmem_entry = (PVOID)(CLASS_ROUTE0_BASE_ADDR + mtd->pbuf_i * CLASS_ROUTE_SIZE);
	while (ddr_entry) {
		efet_memcpy(dmem_entry, ddr_entry, sizeof(MC4Entry));
		while (dmem_entry->flags & HASH_ENTRY_UPDATING)
		{
			while (ddr_entry->flags & HASH_ENTRY_UPDATING) ;
			efet_memcpy(dmem_entry, ddr_entry, sizeof(MC4Entry));
		}

		if (dsta == dmem_entry->dst_addr)
		{
			// We have to check for a null mask because the shift instruction is limited to 31 bits on esiRISC
			if ((dmem_entry->mcdest.src_mask_len == 0) || !(ntohl(srca ^  dmem_entry->src_addr)>>(32 - dmem_entry->mcdest.src_mask_len))) {
				return dmem_entry;
			}
		}
		ddr_entry = (PMC4Entry) dmem_entry->next;
	}

	return NULL;
}
#else
/** Searches multicast forwarding table for match based on source and destination IPv4 addresses.
 * @param[in] srca	IPv4 source address
 * @param[in] dsta	IPv4 destination address
 * @return pointer to matching entry, or NULL if no entry found.
 */
static __inline MC4Entry *MC4_rule_search(U32 srca, U32 dsta)
{
	PMC4Entry this_entry;
	struct slist_entry *entry;
	U32 hash_key;

	hash_key = HASH_MC4(dsta);
	slist_for_each(this_entry, entry, &mc4_table_memory[hash_key], list)
	{
	  	if (dsta == this_entry->dst_addr)
	    	{
	      		if (!(ntohl(srca ^  this_entry->src_addr)>>(32 - this_entry->mcdest.src_mask_len))) {
				return this_entry;
	      		}
	    	}
	}

  	return NULL;
}
#endif

#endif /* _MODULE_MC4_H_ */
