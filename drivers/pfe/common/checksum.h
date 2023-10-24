/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


// Note that Macros are all uppercase

#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include "types.h"

#ifdef GCC_TOOLCHAIN
U16 ip_fast_checksum(void *iph, unsigned int ihl);

U32 checksum_partial_nofold(void *buf, U32 len, U32 sum);

U16 checksum_partial(void *buf, U32 len, U32 sum);

U16 ip_fast_checksum_gen(void *iph);

U16 udpheader_checksum(void *buf, U32 sum);

U32 pseudoheader_checksum(U32 saddr, U32 daddr, U16 len, U8 proto);

#define tcpheader_checksum(buf, sum) ((U16)~checksum_partial(buf, sizeof(tcp_hdr_t), sum))

static inline U16 checksum_fold(U32 sum)
{
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ~((U16)sum);
}

#else

/********************************************************************************
*
*      NAME:        IPHeader_Checksum
*
*      USAGE:          parameter(s):
*                                        			- iph : pointer on the begin of IP header.
*								- ihl : header length in WORD (32 bit)
*
*	                      return value:                  
*								- The checksum field is the 16 bit one's complement of
*								   the one's complement sum of all 16 bit words in the
*								   header.
*
*      DESCRIPTION:	IP header must bit 32 bit aligned. ihl is a number of WORD.
*					Checksum is already complemented
*
********************************************************************************/ 
U16 ip_fast_checksum(void* iph,  unsigned int ihl);
U16 ip_fast_checksum_check(void* iph,  unsigned int ihl);
U16 ip_fast_checksum_gen(void* iph);


/********************************************************************************
*
*      NAME:        pseudoheader_checksum
*
*      USAGE:          parameter(s):
*                                        			- saddr : IP source address
*								- daddr : IP dest address
*								- len : data length
*								- proto : IP protocol
*
*	                      return value:                  
*								- 16 bit checksum of Pseudo-header, not complemented
*								   to be used as input of checksum_partial
*
********************************************************************************/ 
U16 pseudoheader_checksum(U32 saddr, U32 daddr, U16 len,  U8 proto);
U16 merge_checksum(U32 temp1, U32 Temp2);

/********************************************************************************
*
*      NAME:        udpheader_checksum
*
*      USAGE:          parameter(s):
*                                        			- udph : pointer on UDP header
*								- sum  : previous checksum
*
*	                      return value:                  
*								- 16 bit checksum of Pseudo-header, not complemented
*								   to be used as input of checksum_partial
*
********************************************************************************/ 
U16	udpheader_checksum(void* buff, U32 sum);

/********************************************************************************
*
*      NAME:        checksum_partial
*
*      USAGE:          parameter(s):
*                                        			- buff : pointer on data
*								- len : length in bytes
*								- sum : previous checksum
*
*	                      return value:                  
*								- 16 bit checksum, complemented
*								
********************************************************************************/
U16 checksum_partial(void *buff, U16 len, U32 sum);
U16 Internet_checksum(void *buff, U16 len, U32 sum);


#define tcpheader_checksum(buff, sum) ((U16)~checksum_partial(buff, sizeof(tcp_hdr_t), sum))


#endif
#endif /* _CHECKSUM_H_ */
