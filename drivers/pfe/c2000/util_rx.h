
/*
*  Copyright (c) 2009 Mindspeed Technologies, Inc.
*
*  THIS FILE IS CONFIDENTIAL.
*
*  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
*
*  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
*/


#ifndef _UTIL_RX_H_
#define _UTIL_RX_H_

#include "types.h"
#include "system.h"
#include "events.h"

/* The following structure is filled by class-pe when the packet
 * has to be sent to util-pe , by filling the required information */

#define UTIL_MAGIC_NUM	0xffd8ffe000104a46
#define MAX_UTIL_RX_FETCH_CNT	4
#define UTIL_DDRC_WA	

typedef struct {
	U32 mtd_flags : 16;
	U32 packet_type : 8;
	U32 rcv_phyno : 4;
	U32 data_offset : 4;
	u32 word[3];
#ifdef UTIL_DDRC_WA
	u64 magic_num; // magic_number to verify the data validity in utilpe
#endif
} __attribute__((aligned(8))) util_rx_hdr_t; // Size must be a multiple of 64-bit to allow copies using EFET.

/* The following values are defined for packet_type of util_rx_hdr.
 * These represent different types of packets received by utilpe
 * for processing */
#define IPSEC_OUTBOUND_PKT     	EVENT_IPS_OUT
#define IPSEC_INBOUND_PKT      	EVENT_IPS_IN
#define RTP_PKT      			EVENT_RTP_RELAY
#define FRAG_IPV4_PKT		EVENT_FRAG4
#define FRAG_IPV6_PKT		EVENT_FRAG6
#define IPV4_PKT		EVENT_MAX /*TODO give some number*/
#define UTIL_EXPT_PKT		0xFF /*TODO Till we implement direct path */

/* The following data sizes and offsets are used while sending the
 * ipsec inbound packet to classpe */ 
#define LMEM_IPSEC_IPV6_DATA_SIZE 	0x68
#define LMEM_IPSEC_IPV4_DATA_SIZE 	0x40

/* Control flag used to inform CLASS PE about the inbound packet */
#define CTL_IPSEC_INBOUND       0x00000010

/* Hardware Queue Threshold */
#define HW_QUEUE_MAX_THRESHOLD 	12
#define HW_QUEUE_MIN_THRESHOLD 	4

/* Maxmium number of packets to be handled per event */
#define MAX_PKTS_PER_EVENT 8
#define MAX_PKTS_TO_DEQUEUE MAX_PKTS_PER_EVENT

/* Error codes for debugging */
#define ERR_MTD_NOT_AVAILABLE   1
#define ERR_SA_INVALID   	2

#if defined (COMCERTO_2000_UTIL)
void util_init(void);
int M_Util_RX_process_packets(void);
int M_Util_IPSEC_post_processing(void);

/**
 * Returns the number of packets pending in the Util PE INQ FIFO.
 * @return number of pending packets in UtilPE INQ queue.
 */
static int inline util_pending_inq_packets(void)
{
	return readl(INQ_FIFO_CNT);
}
#endif

#endif /* _UTIL_RX_H_ */

