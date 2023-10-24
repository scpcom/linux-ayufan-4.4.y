/*
 *  Copyright (c) 2010 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#ifndef _MODULE_SOCKET_H_
#define _MODULE_SOCKET_H_

#include "types.h"
#include "fe.h"
#include "mtd.h"
#include "channels.h"
#include "layer2.h"

#if !defined(COMCERTO_2000) || defined(COMCERTO_2000_CONTROL)
extern struct slist_head sock4_cache[];
extern struct slist_head sock6_cache[];
extern struct slist_head sockid_cache[];
#else
extern PVOID sock4_cache[];
extern PVOID sock6_cache[];
extern PVOID sockid_cache[];
#endif

#if defined(COMCERTO_2000)
#define SOCK_VALID		(1 << 0)
#define SOCK_USED		(1 << 1)
#define SOCK_UPDATING	(1 << 2)
#endif

extern channel_t socket_output_channel[];

#define SOCKET_TYPE_MSP		2	// Socket to MSP
#define SOCKET_TYPE_ACP		1	// Socket to ACP
#define SOCKET_TYPE_FPP		0 	// Socket to LAN or WAN

#define SOCKET_STATS_SIZE	128	// max socket statistics (in bytes)

#define SOCK_OWNER_NONE			0
#define SOCK_OWNER_RTP_RELAY	1
#define SOCK_OWNER_NATPT		2


#if !defined(COMCERTO_2000)

// IPv4 data path socket entry
typedef struct _tSockEntry{
	struct slist_entry 	list;
	struct slist_entry 	list_id;
	struct _tSockEntry	*nextid;
	U8 					SocketFamily;
	U8 					SocketType;			// 1 -> to ACP  /  0 -> to LAN or WAN (based on pRtEntry)
	PVOID				owner;	
	PRouteEntry			pRtEntry;
	U16					queue;
	U16					dscp;
	U32					SocketStats[SOCKET_STATS_SIZE/4];
	U32					route_id;
	U8					initial_takeover_done;
	U32			next;
	U16 		SocketID;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8   		proto;
	U8   		connected;
	//end of common header
	U32 		Saddr;
	U32 		Daddr;
	U16		hash;
	U16		hash_by_id;
}SockEntry, *PSockEntry;

// IPv6 data path socket entry
typedef struct _tSock6Entry{
	struct slist_entry 	list;
	struct slist_entry 	list_id;
	struct _tSockEntry	*nextid;
	U8 					SocketFamily;
	U8 					SocketType;		/** 1 -> to ACP  /  0 -> to LAN or WAN (based on pRtEntry) */
	PVOID 				owner;
	PRouteEntry			pRtEntry;
	U16 				queue;
	U16 				dscp;
	U32 				SocketStats[SOCKET_STATS_SIZE/4];
	U32 				route_id;
	U8  				initial_takeover_done;	
	U32			next;
	U16 		SocketID;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8   		proto;
	U8   		connected;
	//end of common header
	U32 		Saddr[4];
	U32 		Daddr[4];
	U16		hash;
	U16		hash_by_id;
}Sock6Entry, *PSock6Entry;

#elif defined(COMCERTO_2000_CONTROL)

// IPv4 control path HW socket entry (componed of the infos needed by both classifier and util-pe)
typedef struct _thw_sock4 {
	/* UTIL and CLASS */
	U32 		flags;
	U32 		dma_addr;
	U32 		next;
	U32 		nextid;
	U16 		SocketID;
	U8		SocketFamily;
	U8		SocketType;
	PVOID		owner;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8			proto;
	U8			connected;
	U16 		queue;
	U16 		dscp;
	U32 		Saddr;
	U32 		Daddr;
	struct 	hw_route	route;

	/* UTIL only */
	U32		route_id;
	U32 	SocketStats[SOCKET_STATS_SIZE/4];
	U16		hash;
	U16		hash_by_id;
	U8		initial_takeover_done;

	/* HOST only , these fields are only used by host software, so keep them at the end of the structure */
	struct dlist_head	list;
	struct dlist_head	list_id;
	struct _tSockEntry *sw_sock;	/*pointer to the software socket entry */
	unsigned long		removal_time;
}hw_sock4;

// IPv4 control path SW socket entry
typedef struct _tSockEntry{
	struct slist_entry 	list;
	struct slist_entry 	list_id;
	U16 				hash;
	U16			 		hash_by_id;
	U32 				nextid;
	U16			 		SocketID;
	U8 					SocketFamily;
	U8 					SocketType;	/** 1 -> to ACP  /  0 -> to LAN or WAN (based on pRtEntry) */
	PVOID				owner;
	U16 				owner_type;
	PRouteEntry			pRtEntry;
	U16					queue;
	U16					dscp;
	U32					SocketStats[SOCKET_STATS_SIZE/4];
	U32					route_id;
	U8					initial_takeover_done;

	U16 		Sport;
	U16 		Dport;
	U8			proto;
	U8			connected;
	U32 		Saddr;
	U32 		Daddr;

	struct _thw_sock4   *hw_sock;	/** pointer to the hardware socket entry */
}SockEntry, *PSockEntry;

// IPv6 control path HW socket entry
typedef struct _thw_sock6 {
	/* UTIL and CLASS */
	U32 		flags;
	U32 		dma_addr;
	U32 		next;
	U32 		nextid;
	U16 		SocketID;
	U8			SocketFamily;
	U8			SocketType;
	PVOID		owner;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8			proto;
	U8			connected;
	U16 		queue;
	U16 		dscp;
	U32 		Saddr[4];
	U32 		Daddr[4];
	struct hw_route	route;
	
	/* UTIL only */
	U32 		route_id;
	U32 		SocketStats[SOCKET_STATS_SIZE/4];
	U16			hash;
	U16			hash_by_id;
	U8			initial_takeover_done;

	/* HOST only , these fields are only used by host software, so keep them at the end of the structure */
	struct dlist_head	list;
	struct dlist_head	list_id;
	struct _tSock6Entry *sw_sock;	/*pointer to the software socket entry */
	unsigned long		removal_time;
}hw_sock6;

// IPv6 control path SW socket entry
typedef struct _tSock6Entry{
	struct slist_entry 	list;
	struct slist_entry 	list_id;
	U16 				hash;
	U16			 		hash_by_id;
	U32 				nextid;
	U16			 		SocketID;
	U8 					SocketFamily;
	U8 					SocketType;	/** 1 -> to ACP  /  0 -> to LAN or WAN (based on pRtEntry) */
	PVOID				owner;
	U16 				owner_type;
	PRouteEntry			pRtEntry;
	U16					queue;
	U16					dscp;
	U32					SocketStats[SOCKET_STATS_SIZE/4];
	U32					route_id;
	U8					initial_takeover_done;

	U16 		Sport;
	U16 		Dport;
	U8			proto;
	U8			connected;
	U32 		Saddr[4];
	U32 		Daddr[4];

	struct _thw_sock6   *hw_sock;	/** pointer to the hardware socket entry */
}Sock6Entry, *PSock6Entry;

#else

#if defined (COMCERTO_2000_CLASS)

// IPv4 data path socket entry for classifier
typedef struct _tSockEntry{
	U32 		flags;
	U32 		dma_addr;
	U32 		next;
	U32 		nextid;
	U16 		SocketID;
	U8		SocketFamily;
	U8		SocketType;
	PVOID		owner;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8		proto;
	U8		connected;
	U16 		queue;
	U16 		dscp;
	U32 		Saddr;
	U32 		Daddr;
	RouteEntry	route;
}SockEntry, *PSockEntry;

// IPv6 data path socket entry for classifier
typedef struct _tSock6Entry{
	U32 		flags;
	U32 		dma_addr;
	U32 		next;
	U32 		nextid;
	U16 		SocketID;
	U8		SocketFamily;
	U8		SocketType;
	PVOID		owner;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8		proto;
	U8		connected;
	U16 		queue;
	U16 		dscp;
	U32 		Saddr[4];
	U32 		Daddr[4];
	RouteEntry	route;
}Sock6Entry, *PSock6Entry;

#else 

// IPv4 data path socket entry for util-pe
typedef struct _tSockEntry{
	U32 		flags;
	U32 		dma_addr;
	U32 		next;
	U32 		nextid;
	U16 		SocketID;
	U8			SocketFamily;
	U8			SocketType;
	PVOID		owner;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8			proto;
	U8			connected;
	U16 		queue;
	U16 		dscp;
	U32 		Saddr;
	U32 		Daddr;
	RouteEntry	route;
	U32 		route_id;
	U32 		SocketStats[SOCKET_STATS_SIZE/4];
	U16 		hash;
	U16 		hash_by_id;
	U8			initial_takeover_done;
}SockEntry, *PSockEntry;

// IPv6 data path socket entry for util-pe
typedef struct _tSock6Entry{
	U32 		flags;
	U32 		dma_addr;
	U32 		next;
	U32 		nextid;
	U16 		SocketID;
	U8			SocketFamily;
	U8			SocketType;
	PVOID		owner;
	U16 		owner_type;
	U16 		Sport;
	U16 		Dport;
	U8			proto;
	U8			connected;
	U16 		queue;
	U16 		dscp;
	U32 		Saddr[4];
	U32 		Daddr[4];
	RouteEntry	route;
	U32 		route_id;
	U32 		SocketStats[SOCKET_STATS_SIZE/4];
	U16 		hash;
	U16 		hash_by_id;
	U8			initial_takeover_done;
}Sock6Entry, *PSock6Entry;

#endif

#endif

static __inline U32 HASH_SOCKID(U16 id)
{
	return (id & (NUM_SOCK_ENTRIES - 1));
}

static __inline U32 HASH_SOCK(U32 Daddr, U32 Dport, U16 Proto)
{
	U32 sum;

	Daddr = ntohl(Daddr);
	Dport = ntohs(Dport);
	
	sum = ((Daddr << 7) | (Daddr >> 25));
	sum += ((Dport << 11) | (Dport >> 21));
	sum += (sum >> 16) + Proto;
	return (sum ^ (sum >> 8)) & (NUM_SOCK_ENTRIES - 1);
}

static __inline U32 HASH_SOCK6(U32 Daddr, U32 Dport, U16 Proto)
{
	U32 sum;

	Daddr = ntohl(Daddr);
	Dport = ntohs(Dport);

	sum = ((Daddr << 7) | (Daddr >> 25));
	sum += ((Dport << 11) | (Dport >> 21));
	sum += (sum >> 16) + Proto;
	return (sum ^ (sum >> 8)) & (NUM_SOCK_ENTRIES - 1);
}


int SOCKET_expt_match(PMetadata mtd);
PRouteEntry SOCKET4_get_route(PSockEntry pSocket);
PRouteEntry SOCKET6_get_route(PSock6Entry pSocket);


BOOL SOCKET4_check_route(PSockEntry pSocket)__attribute__ ((noinline));
int SOCKET4_send_rtp_packet(PMetadata mtd, PSockEntry pSocket, BOOL ipv4_update, U32 payload_diff)__attribute__ ((noinline));
BOOL SOCKET6_check_route(PSock6Entry pSocket) __attribute__ ((noinline));
int SOCKET6_send_rtp_packet(PMetadata mtd, PSock6Entry pSocket, BOOL ipv6_update, U32 payload_diff) __attribute__ ((noinline));

#if defined (COMCERTO_2000_CLASS) || defined (COMCERTO_2000_UTIL)
PSockEntry SOCKET_find_entry_by_id(PMetadata mtd, U16 socketID)__attribute__ ((noinline));
PSockEntry SOCKET4_find_entry(U32 saddr, U16 sport, U32 daddr, U16 dport, U16 proto, PMetadata mtd)__attribute__ ((noinline));
PSock6Entry SOCKET6_find_entry(U32 *saddr, U16 sport, U32 *daddr, U16 dport, U16 proto, PMetadata mtd) __attribute__ ((noinline));
#else
PSockEntry SOCKET_find_entry_by_id(U16 socketID)__attribute__ ((noinline));
PSockEntry SOCKET4_find_entry(U32 saddr, U16 sport, U32 daddr, U16 dport, U16 proto)__attribute__ ((noinline));
PSock6Entry SOCKET6_find_entry(U32 *saddr, U16 sport, U32 *daddr, U16 dport, U16 proto) __attribute__ ((noinline));
#endif


void socket4_update(PSockEntry pSocket);
void socket6_update(PSock6Entry pSocket);

BOOL socket_init(void);
void socket_exit(void);


#if defined(COMCERTO_2000_CLASS)
/* PBUF route entry memory chunk (128B) is used to allocated a socket entry */
#define SOCKET_BUFFER(mtd)\
    ((PVOID)(CLASS_ROUTE0_BASE_ADDR + mtd->pbuf_i * CLASS_ROUTE_SIZE))
	
#elif defined(COMCERTO_2000_UTIL)
#include "util_dmem_storage.h"
#endif


#endif

