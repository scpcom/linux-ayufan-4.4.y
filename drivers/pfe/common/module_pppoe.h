/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_PPPOE_H_
#define _MODULE_PPPOE_H_

#define PPP_IP		0x21	/* Internet Protocol */
#define PPP_IPV6	0x57	/* Internet Protocol Version 6 */
#define PPP_LCP		0xc021	/* Link Control Protocol */
#define PPP_IPCP	0x8021	/* IP Control Protocol */
#define PPP_IPV6CP	0x8057	/* IPv6 Control Protocol */
#define PPP_PAP		0xc023
#define PPP_CHAP	0xc223


#if !defined (COMCERTO_2000_UTIL)

#include "types.h"
#include "modules.h"
#include "channels.h"
#include "module_ethernet.h"

#define IF_NAME_SIZE    16

#define NUM_PPPOE_ENTRIES (1 << 3)
#define PPPOE_RELAY_L2HDR_SIZE 24

#define PPPOE_AUTO_MODE  	0x1

typedef struct tPPPoEFrame {
    U16        PPPoE_VerTypeCode;
    U16        PPPoE_SessionID;
    U16        PPPoE_Length;
    U16        PPPoE_Protocol;
    U8         PPPoE_Payload[];
} PPPoEFrame, *pPPPoEFrame;

typedef struct _tPPPoE_Info {
	struct itf itf;

	struct slist_entry list;
	struct _tPPPoE_Info *relay;

	/* store following two items in network order */
	U8 DstMAC[ETHER_ADDR_LEN];
	U16 sessionID;

	U8 relay_l2hdr[PPPOE_RELAY_L2HDR_SIZE]; //14 (ethernet header) +4 (vlan header) + 4(if QinQ)
	U16 relay_l2hdrsize;
	U8 relay_output_port;
	U32 ppp_flags;
	U32 last_pkt_rcvd;
	U32 last_pkt_xmit;
#ifdef FPP_STATISTICS	
	U32 total_packets_received;
	U32 total_packets_transmitted;
#endif	
} PPPoE_Info, *pPPPoE_Info; 

typedef struct _tPPPoECommand {
    U16 action;
    U16 sessionID;
    U8  macAddr[6];
    U8  phy_intf[12];
    U8  log_intf[12];
    U16 mode;
} PPPoECommand, *pPPPoECommand;

typedef struct _tPPPoERelayCommand {
    U16 action;      /*Action to perform */
    U8 peermac1[6];
    U8 peermac2[6];
    U8 ipif_mac[6];
    U8 opif_mac[6];
    U8 ipifname[IF_NAME_SIZE];
    U8 opifname[IF_NAME_SIZE];
    U16 sesID;
    U16 relaysesID;
    U16 pad;
}PPPoERelayCommand, *pPPPoERelayCommand;

typedef struct _tPPPoEIdleTimeCmd {
    U8  ppp_intf[12];
    U32  xmit_idle;
    U32  recv_idle;
  } PPPoEIdleTimeCmd, *pPPPoEIdleTimeCmd;


#if defined(COMCERTO_2000)
#define PPPOE_MAX_ITF	4
extern PPPoE_Info pppoe_itf[PPPOE_MAX_ITF];
#endif

extern struct slist_head pppoe_cache[];

int pppoe_init(void);
void pppoe_exit(void);

void M_PPPOE_process_packet(PMetadata mtd) __attribute__((section ("fast_path")));
#if !defined(COMCERTO_2000)
void M_pppoe_ingress(void) __attribute__((section ("fast_path")));
#endif
void M_pppoe_update_length(PMetadata mtd);
void M_pppoe_encapsulate(PMetadata mtd, pPPPoE_Info ppp_info, U16 ethertype, U8 update) __attribute__((section ("fast_path"), noinline));

static __inline U32 HASH_PPPOE(U16 session_id, U8 *srcmac)
{
	return ((session_id & 0xff) ^ (session_id >> 8) ^ srcmac[5]) & (NUM_PPPOE_ENTRIES - 1);
}

#endif /* !defined (COMCERTO_2000_UTIL) */

#endif /* _MODULE_PPPOE_H_ */
