/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_TX_H_
#define _MODULE_TX_H_

#include "types.h"
#include "modules.h"
#include "layer2.h"

#if defined(COMCERTO_2000_CONTROL)

#define DEFAULT_NAME_0		eth0
#define DEFAULT_NAME_1		eth2
#define DEFAULT_NAME_2		eth3

typedef struct _tPortUpdateCommand {
	U16 portid;
	char ifname[IF_NAME_SIZE];
} PortUpdateCommand, *PPortUpdateCommand;


int tx_init(void);
void tx_exit(void);
#else

#include "util_rx.h"
void M_PKT_TX_process_packet(struct tMetadata *mtd);
BOOL M_tx_enable(U32 portid) __attribute__ ((noinline));
void send_to_tx_mcast(struct tMetadata *mtd, U32 dma_start);
void M_PKT_TX_util_process_packet(struct tMetadata *mtd, u8 pkt_type);
void *dmem_header_writeback(struct tMetadata *mtd);

#define	send_to_tx(pMetadata) SEND_TO_CHANNEL(M_PKT_TX_process_packet, pMetadata)

#define	send_to_tx_multiple(pBeg, pEnd) SEND_TO_CHANNEL_MULTIPLE(M_PKT_TX_process_packet, pBeg, pEnd)

#endif

#endif /* _MODULE_TX_H_ */

