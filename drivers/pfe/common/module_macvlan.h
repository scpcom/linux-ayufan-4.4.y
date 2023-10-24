/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#ifdef CFG_MACVLAN

#ifndef _MODULE_MACVLAN_H_
#define _MODULE_MACVLAN_H_

#include "types.h"
#include "modules.h"
//#include "layer2.h" /* FIXME recursive include */


#define NUM_MACVLAN_HASH_ENTRIES		16

typedef struct _tMacvlanEntry {
	struct itf itf;

	struct _tMacvlanEntry *next;
	struct _tMacvlanEntry *prev;

	U8 MACaddr[ETHER_ADDR_LEN];
}MacvlanEntry , *PMacvlanEntry;


typedef struct _tMacvlanCmd{
		unsigned short action;
		unsigned char macaddr[ETHER_ADDR_LEN];
		unsigned char macvlan_ifname[12];
		unsigned char phys_ifname[12];
}MacvlanCmd, *PMacvlanCmd;

extern PMacvlanEntry macvlan_cache[NUM_MACVLAN_HASH_ENTRIES];

BOOL M_macvlan_init(PModuleDesc pModule);

void M_MACVLAN_process_packet(PMetadata mtd);
#if !defined(COMCERTO_2000)
void M_macvlan_entry(void);
#endif

static __inline U8 HASH_MACVLAN(void *pmacaddr)
{
	U8  hash;
	U16 sum;
	U16  *macAddr  = (U16 *)pmacaddr;

	sum = ( macAddr[0] ^ macAddr[1] ^ macAddr[2]);
	hash = (sum >> 8) ^ (sum & 0xFF);

	return ((hash ^ (hash >> 4)) & (NUM_MACVLAN_HASH_ENTRIES - 1));
}

#endif /* _MODULE_MACVLAN_H_ */
#endif /* CFG_MACVLAN */

