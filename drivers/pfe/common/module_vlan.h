/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#ifndef _MODULE_VLAN_H_
#define _MODULE_VLAN_H_

#include "types.h"
#include "modules.h"
#include "channels.h"
#include "fe.h"
#include "common_hdrs.h"

#define VLAN_ENABLE 1


/*Internal VLAN entry used by the VLAN engine*/
typedef struct _tVlanEntry {
	struct itf itf;

	struct slist_entry list;

	U16 VlanId;						/*In big endian format*/

#ifdef FPP_STATISTICS	
	U32 total_packets_received;  
	U32 total_packets_transmitted; 
	U64 total_bytes_received;  
	U64 total_bytes_transmitted;	
#endif		
}VlanEntry, *PVlanEntry;

/*Structure defining the VLAN ENTRY command*/
typedef struct _tVLANCommand {
	U16 action;		 	/*Action to perform*/
	U16 vlanID;
	U8 vlanifname[12];
	U8 phyifname[12];
}VlanCommand, *PVlanCommand;

#if defined(COMCERTO_2000)
#define VLAN_MAX_ITF	32
extern VlanEntry vlan_itf[VLAN_MAX_ITF];
#endif

extern struct slist_head vlan_cache[NUM_VLAN_ENTRIES];

void M_VLAN_process_packet(PMetadata mtd) __attribute__((section ("fast_path")));

int vlan_init(void);
void vlan_exit(void);

#if !defined(COMCERTO_2000)
void M_vlan_entry(void) __attribute__((section ("fast_path")));
#endif
void M_vlan_br_encapsulate(PMetadata mtd, PVlanEntry pEntry);
void M_vlan_encapsulate(PMetadata mtd, PVlanEntry pEntry, U16 ethertype, U8 update) __attribute__((section ("fast_path"), noinline));


/** Vlan entry hash calculation (based on vlan id).
*
* @param entry	vlan_id VLAN ID in network by order
*
* @return	vlan hash index
*
*/
static __inline U32 HASH_VLAN(U16 vlan_id)
{
#if defined(COMCERTO_2000)
	/* Use host endianess for the hash calculation so host and pfe arrive at the same result */
	vlan_id = ntohs(vlan_id);
#endif
	return ((vlan_id >> 12) ^ (vlan_id >> 8) ^ (vlan_id)) & (NUM_VLAN_ENTRIES - 1);
}

#endif /* _MODULE_VLAN_H_ */

