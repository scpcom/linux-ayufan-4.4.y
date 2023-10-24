
/*
*  Copyright (c) 2009 Mindspeed Technologies, Inc.
*
*  THIS FILE IS CONFIDENTIAL.
*
*  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
*
*  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
*/

#ifndef _UTIL_TX_H_
#define _UTIL_TX_H_

#include "types.h"
#include "hal.h"
#include "mtd.h"
#include "util_dmem_storage.h"


#define LMEM_DATA_OFFSET         	0x10
#define LMEM_BUF_LEN              	0x80
#define LMEM_MAX_PKTDATA_LEN			(LMEM_BUF_LEN - LMEM_DATA_OFFSET - UTIL_TX_TRAILER_SIZE) /**< Round it down to nearest 32-bit boundary so trailer is aligned. */


#define TRANSMIT_BUFFER (&util_buffers.transmit_buffer[0])
#define PACKET_BUFFER (&util_buffers.transmit_buffer[LMEM_DATA_OFFSET])

lmem_trailer_t* util_tx_get_lmem_trailer();

void util_send_to_outputport(u8 port, u8 queue, PMetadata mtd, u16 hdr_len);
void util_send_to_classpe(PMetadata mtd);
void util_send_to_host(PMetadata mtd);

#endif /* _UTIL_TX_H_ */
