/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_NATPT_H_
#define _MODULE_NATPT_H_

#include "types.h"
#include "modules.h"
#include "mtd.h"

#define NUM_NATPT_ENTRIES 	2048

#define NATPT_CONTROL_6to4	0x0001
#define NATPT_CONTROL_4to6	0x0002
#define NATPT_CONTROL_TCPFIN	0x0100


typedef struct _tNATPT_Entry {
	struct _tNATPT_Entry *next;
	U16	socketA;
	U16	socketB;
	U16	control;
	U8	protocol;
	U64	stat_v6_received;
	U64	stat_v6_transmitted;
	U64	stat_v6_dropped;
	U64	stat_v6_sent_to_ACP;
	U64	stat_v4_received;
	U64	stat_v4_transmitted;
	U64	stat_v4_dropped;
	U64	stat_v4_sent_to_ACP;
} NATPT_Entry, *PNATPT_Entry;


typedef struct _tNATPTOpenCommand {
	U16	socketA;
	U16	socketB;
	U16	control;
	U16	reserved;
}NATPTOpenCommand, *PNATPTOpenCommand;

typedef struct _tNATPTCloseCommand {
	U16	socketA;
	U16	socketB;
}NATPTCloseCommand, *PNATPTCloseCommand;

typedef struct _tNATPTQueryCommand {
	U16	reserved1;
	U16	socketA;
	U16	socketB;
	U16	reserved2;
}NATPTQueryCommand, *PNATPTQueryCommand;

typedef struct _tNATPTQueryResponse {
	U16	retcode;
	U16	socketA;
	U16	socketB;
	U16	control;
	U64	stat_v6_received;
	U64	stat_v6_transmitted;
	U64	stat_v6_dropped;
	U64	stat_v6_sent_to_ACP;
	U64	stat_v4_received;
	U64	stat_v4_transmitted;
	U64	stat_v4_dropped;
	U64	stat_v4_sent_to_ACP;
}NATPTQueryResponse, *PNATPTQueryResponse;

BOOL M_natpt_init(PModuleDesc pModule);

void M_natpt_receive(void) __attribute__((section ("fast_path")));

void M_NATPT_process_packet(PMetadata mtd);


extern PNATPT_Entry natpt_cache[];

static __inline U32 HASH_NATPT(U16 socketA, U16 socketB)
{
	U32 hash;
	// This hash function is designed so that socketA and socketB can be reversed.
	hash = socketA ^ socketB;
	hash ^= hash >> 8;
	return hash & (NUM_NATPT_ENTRIES - 1);
}

#endif /* _MODULE_NATPT_H_ */
