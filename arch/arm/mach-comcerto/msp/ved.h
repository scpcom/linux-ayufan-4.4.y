/*
 *  arch/arm/mach-comcerto/msp/ved.h
 *
 *  Copyright (C) 2012 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _COMCERTO_VED_H
#define _COMCERTO_VED_H

#include <linux/netdevice.h>

#include "smi.h"
#include "msp.h"

/* Protocol index definition */
enum {
	PROTID_RAW,
	PROTID_ETH,
	PROTID_PPP,
	PROTID_IPV4,
	PROTID_IPV6,
	PROTID_CSME,
	PROTID_ARP,
	PROTID_VLAN,
	PROTID_UDP,
	PROTID_TCP,
	PROTID_ICMP,
	PROTID_CSM_API,
	MAX_PROTOCOL,
	PROTO_INVALID
};

#define MAX_BDESC_NUMBER (4)
#define CTRL "eth1"


/*
 * This structure is private to each device. It is used to pass
 * packets in and out, so there is place for a packet
 */

struct ved_priv {
	struct net_device_stats stats;
	struct platform_device *pdev;
	struct net_device *dev;
	struct napi_struct napi;
	spinlock_t lock;

	struct _SKB_POOL *skbpool; /* skb pool for MSP to CSP buffer (max MTU size) */

	/* smi (share memory interface) */
	struct fastpart *tx_smipart;	/* CSP to MSP Fdesc fast part */
	struct fastpart *rx_smipart;	/* MSP to CSP Fdesc fast part */
	struct smiqueue tx_smiq;
	struct smiqueue rx_smiq;
	u32 default_packet_type;
	struct comcerto_msp msp;
	int state;
};

struct BDesc {
	u8 *BPtr;
	volatile u32 BControl;
};

struct FDesc {
	struct FDesc *Next;
	volatile u32 System;
	volatile u32 FStatus;
	volatile u32 FControl;
	struct BDesc BDesc[MAX_BDESC_NUMBER];

	u16 Length;
	u16 Offset;
	u16 protocol;
	u16 protocol_hdr_len;
	struct sk_buff *skb;
	struct fastpart *fpart;
	struct FDesc *Tail;
	u32 nFDesc; /* nFdesc used by MSP or local phy addr of FDESC
	             * (only used in direct EMAC control mode) */
	void *pSFdesc; /* CSP must not touch this value */
	u8 *Payload;
};

#endif /* _COMCERTO_VED_H */
