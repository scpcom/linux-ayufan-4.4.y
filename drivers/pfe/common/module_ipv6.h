/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_IPV6_H_
#define _MODULE_IPV6_H_

#include "types.h"
#include "modules.h"
#include "fpart.h"
#include "fe.h"
#include "channels.h"
#ifndef COMCERTO_2000_UTIL
#include "layer2.h"
#endif
#include "module_ipv4.h"
#include "common_hdrs.h"


typedef struct IPv6_FRAG_HDR
{
	U8 NextHeader;
	U8 rsvd;
	U16 FragOffset;
	U32 Identification;
} ipv6_frag_hdr_t;


typedef struct IPv6_ROUTING_HDR
{
	U8 NextHeader;
	U8 HdrExtLen;
	U8 RoutingType;
	U8 SegmentsLeft;
	U8 TypeSpecificData[0];
} ipv6_routing_hdr_t;


typedef struct tIpv6Stat
{
	U32 IPv6_Emitted_Frames;
	U32 IPv6_Received_Frames;
	U32 IPv6_Received_UnknownIPaddress;
	U32 IPv6_Received_UnknownProtocol;
	U32 IPv6_Received_FragmentedPacket;
	U32 IPv6_Received_UnknownPort;
	U32 IPv6_Received_BadUDPChecksum;
} Ipv6Stat, *PIpv6Stat;

typedef struct tIPV6_context {

	U32 fragmentation_id;
	struct tIpv6Stat IPV6Stats;
} IPV6_context;

/* IPv6 Conntrack entry */
#if !defined(COMCERTO_2000)
typedef struct _tCtEntryIPv6{
	// start of common header -- must match IPv4
	struct slist_entry list;
	U32 last_ct_timer;
	union {
	    U16 fwmark;
	    struct {
		U16 queue : 5;
		U16 vlan_pbits : 3;
		U16 dscp_mark_flag : 1;
		U16 dscp_mark_value : 6;
		U16 set_vlan_pbits : 1;
	    } __attribute__((packed));
	};
	U16 status;
	union {
		PRouteEntry pRtEntry;
		U32 route_id;
	};
	// end of common header
	U16	Sport;
	U16	Dport;
	U16 hSAEntry[SA_MAX_OP];
	PRouteEntry tnl_route;
	U32	unused1;
// 1st DC line
	U32 Saddr_v6[4];
	U32 Daddr_v6[4];
}CtEntryIPv6, *PCtEntryIPv6;

static inline PRouteEntry ct6_tnl_route(PCtEntryIPv6 pCtEntry)
{
	return pCtEntry->tnl_route;
}

#else

typedef CtEntry CtEntryIPv6, *PCtEntryIPv6;

#define ct6_tnl_route(entry)	ct_tnl_route((PCtEntry)(entry))

#endif

#define ct6_route(entry)	ct_route((PCtEntry)(entry))


#if defined(COMCERTO_2000_CONTROL) || !defined(COMCERTO_2000)
int IPv6_delete_CTpair(PCtEntryIPv6 pCtEntry);
#endif

int ipv6_init(void);
void ipv6_exit(void);

#if !defined(COMCERTO_2000)
void M_ipv6_entry(void) __attribute__((section ("fast_path")));
PCtEntryIPv6 CT6_lookup(PMetadata mtd, ipv6_hdr_t *ipv6_hdr, U32 L4_offset, U8 Proto) __attribute__((section("fast_path"))) ;
PCtEntryIPv6 IPv6_get_ctentry(U32 *saddr, U32 *daddr, U16 sport, U16 dport, U16 proto)  __attribute__ ((noinline));
#endif


void M_IPV6_process_packet(PMetadata mtd) __attribute__((section ("fast_path")));
PMetadata IPv6_fragment_packet(PMetadata mtd, ipv6_hdr_t *pIp6h, U8 *phdr, U32 mtu, U32 hlen, U32 preL2_len, U32 if_type);
int IPv6_send_rtp_packet(PMetadata mtd, PSock6Entry pSocket, BOOL ipv6_update, U32 payload_diff);
void IPv6_tcp_termination(PCtEntryIPv6 pCtEntry);

int IPv6_handle_RESET(void);

extern struct tIPV6_context gIPv6Ctx;

#if defined(COMCERTO_2000)
static __inline U32 HASH_CT6(U32 *Saddr, U32 *Daddr, U32 Sport, U32 Dport, U16 Proto)
{
	int i;
	U32 sum;

	sum = 0;
	for (i = 0; i < 4; i++)
		sum += ntohl(READ_UNALIGNED_INT(Saddr[i]));
	sum = htonl(sum) ^ htonl(ntohs(Sport));
	sum = crc32_be((u8 *)&sum);

	for (i = 0; i < 4; i++)
		sum += ntohl(READ_UNALIGNED_INT(Daddr[i]));
	sum += Proto;
	sum += ntohs(Dport);
//	sum += phy_no;

	return sum & CT_TABLE_HASH_MASK;
}
#else
static __inline U32 HASH_CT6(U32 *Saddr, U32 *Daddr, U32 Sport, U32 Dport, U16 Proto)
{
	U32 sum;
	sum = Saddr[IP6_LO_ADDR] + ((Daddr[IP6_LO_ADDR] << 7) | (Daddr[IP6_LO_ADDR] >> 25));
	sum ^= Sport + ((Dport << 11) | (Dport >> 21));
	sum ^= (sum >> 16);
	sum ^= (sum >> 8);
	return (sum ^ Proto) & CT_TABLE_HASH_MASK;
}
#endif

/** Compares 2 IPv6 addresses based on a mask.
 * No assumptions are made on the alignment of addresses.
 * @param a1		Pointer to first IPv6 address.
 * @param a2		Pointer to second IPv6 address.
 * @param masklen	Length of mask to use during comparison (a length of 128 means the 2 addresses must be identical).
 * @return	1 if the 2 addresses match, 0 otherwise.
 */
#if defined(COMCERTO_2000_CLASS) || defined(COMCERTO_2000_UTIL)
static __inline int ipv6_cmp_mask(U32 *a1, U32 *a2, unsigned int masklen) {
	U16 * a1s = (U16 *)a1;
	U16 * a2s = (U16 *)a2;
	while(masklen>0) {
		if (masklen >= 16) {
			if (*a1s != *a2s)
				return 0;
			masklen -= 16;
			a1s++;
			a2s++;
		} else {
			if ((ntohs(*a1s ^ *a2s) >> (16 - masklen)) != 0)
				return 0;
			else
				return 1;
		}
	}
	return 1;
}

#else
static __inline int ipv6_cmp_mask(U32 *a1, U32 *a2, unsigned int masklen) {
  // when memcmp is present - can be rewritten to burst-compare words.
  while(masklen>0) {
    if (masklen >= 32) {
      if (*a1 != *a2)
	return 0;
      masklen -= 32;
      a1++;
      a2++;
    } else {
      if ((ntohl(*a1 ^ *a2) >> (32-masklen)) != 0)
	return 0;
      else
	return 1;
    }
  } 
  return 1;
} 
#endif

static __inline U32 ipv6_get_frag_id(void) {
    // return fragmentation id
  extern struct tIPV6_context gIPv6Ctx;
  U32 id = gIPv6Ctx.fragmentation_id;
  gIPv6Ctx.fragmentation_id  += 1;
  if ( gIPv6Ctx.fragmentation_id == 0)
    gIPv6Ctx.fragmentation_id = 1;
  return id;
}

static __inline U32 ipv6_find_fragopt(ipv6_hdr_t *ip6h, U8 **nhdr, U32 pktlen)
{
    // returns length of unfragmentable part of ipv6 packet and pointer to nexthdr
    // loosely similar to ip6_find_1stfragopt
  int rthdr_seen = 0; 
  U16 offset = sizeof(ipv6_hdr_t);
  U8 *this_exthdr= (U8*) (ip6h+1);
  // Compiler does not allow address of bitfields
  *nhdr = ((U8*) ip6h) + 6;/*  &(ip6h->NextHeader) // (ip6h->nexthdr) */

  while (offset + 1< pktlen) {
    switch (**nhdr) {
    case IPV6_DESTOPT:
      if (rthdr_seen) // if we saw routing header - all done
	return offset;
    case IPV6_HOP_BY_HOP:
      break;
    case IPV6_ROUTING:
      rthdr_seen = 1;
      break;
    default:
      return offset;
    }
    offset += (*(*nhdr+1)+1)<<3;
    *nhdr = this_exthdr;
    this_exthdr += (*(*nhdr+1)+1)<<3;
  }
  return offset;
}

static __inline U8 ipv6_find_proto(ipv6_hdr_t *ip6h, U8 **nhdr, U32 pktlen)
{
  // very similar to ipv6_find_firstfragopt above except returns protocol found
  // and does not maintain offset
  int rthdr_seen = 0; 
  U16 offset = sizeof(ipv6_hdr_t);
  U8 *this_exthdr= (U8*) (ip6h+1);
  // Compiler does not allow address of bitfields
  *nhdr = ((U8*) ip6h) + 6;/*  &(ip6h->NextHeader) // (ip6h->nexthdr) */

  while (offset + 1< pktlen) {
    switch (**nhdr) {
    case IPV6_DESTOPT:
      if (rthdr_seen) // if we saw routing header - all done
	return IPV6_DESTOPT;
    case IPV6_HOP_BY_HOP:
      break;
    case IPV6_ROUTING:
      rthdr_seen = 1;
      break;
    default:
      return **nhdr;
    }
    offset += (*(*nhdr+1)+1)<<3;
    *nhdr = this_exthdr;
    this_exthdr += (*(*nhdr+1)+1)<<3;
  }
  return **nhdr;
}

static __inline U8* ipv6_find_frag_header(ipv6_hdr_t *ip6h, U32 pktlen)
{
    // returns length of unfragmentable part of ipv6 packet and pointer to nexthdr
    // loosely similar to ip6_find_1stfragopt
  U8 *nhdr;
  U16 offset = sizeof(ipv6_hdr_t);
  U8 *this_exthdr= (U8*) (ip6h+1);
  nhdr = ((U8*) ip6h) + 6;/*  &(ip6h->NextHeader) // (ip6h->nexthdr) */

  while (offset + 1< pktlen) {
    if (*nhdr == IPV6_FRAGMENT) 
    	return (nhdr);

    offset += (*(nhdr+1)+1)<<3;
    nhdr = this_exthdr;
    this_exthdr += (*(nhdr+1)+1)<<3;
  }

  return  nhdr;

}

static inline U32 ipv6_pseudoheader_checksum(U32 *saddr, U32 *daddr, U16 length, U8 nexthdr)
{
	U32 sum = 0;
	int i;

	for (i = 0; i < 8; i++)
		sum += ((U16 *)saddr)[i] + ((U16 *)daddr)[i];

	sum += length;

	sum += ((U16)nexthdr) << 8;

	return sum;
}

#endif /* _MODULE_IPV6_H_ */
