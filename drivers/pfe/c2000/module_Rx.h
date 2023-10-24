/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_RX_H_
#define _MODULE_RX_H_

#include "types.h"
#include "channels.h"



#if defined(COMCERTO_2000_CONTROL)
int rx_init(void);
void rx_exit(void);

#elif defined(COMCERTO_2000_CLASS)
#define QMOD_NONE 0
#define QMOD_DSCP 1
/*
 * Commands
 */
/* Ingress congestion control management */
typedef struct _tEthIccCommand {
	unsigned short portIdx;	/* enable and disable commands */
	unsigned short acc;	/* optional , enable only*/ 
	unsigned short onThr;	/* optional , enable only*/
	unsigned short offThr;	/* optional , enable only*/
} EthIccCommand, *PEthIccCommand;

extern U32 pbuf_i;
extern channel_t RXFromUtilChannel[NUM_CHANNEL];

static int inline Rx_packet_available(void)
{
	U32 qb_buf_status;

	/* read the hardware bitmask value for classifier buffers and check for new packets */

	/* read QB_BUF_STATUS into qbBufStatus */
	/* FIXME can this be read only when the bit is not set? */
	qb_buf_status = readl(PERG_QB_BUF_STATUS);

	return (qb_buf_status & regMask[pbuf_i]) != 0;
}

void class_init(void);
void _M_Rx_put_channel(struct tMetadata *mtd);
BOOL M_rx_enable(U8 portid);
void M_RX_process_packet(void);

#endif /* defined(COMCERTO_2000_CLASS) */


#endif /* _MODULE_RX_H_ */
