
#ifndef _COMMON_HDRS_H__
#define _COMMON_HDRS_H__

#include "types.h"

#define C2000_MAX_FRAGMENTS 5 //TODO

#define IPV6_HDR_SIZE		40
#define IPV4_HDR_SIZE		20
#define TCP_HDR_SIZE		20
#define UDP_TCP_HDR_SIZE	8
#define PPPOE_HDR_SIZE		8
#define IPSEC_ESP_HDR_SIZE	8
#define ETH_HDR_SIZE		14
#define VLAN_HDR_SIZE		4

#define MAX_L2_HEADER	18


/* 
* VLAN
*
*/


/** Structure that describes the VLAN header as it used by the code:
 * VID contains the actual VID value, while TPID contains the Ethertype of the inner data.
 * This makes the VLAN handling logic simpler and more intuitive.
 */
typedef struct VLAN_HDR_STRUCT
{
		U16 TCI;
		U16 TPID; /**< Tells the Protocol ID, like IPv4 or IPv6 */
}vlan_hdr_t;

#define VLAN_VID(vlan_hdr)	((vlan_hdr)->TCI & htons(0xfff))
#define VLAN_PCP(vlan_hdr)	(*(U8 *)&((vlan_hdr)->TCI) >> 5)

/** Structure that describes the real VLAN header format, as defined in the 802.1Q standard.
 */
typedef struct REAL_VLAN_HDR_STRUCT
{
	union {
		U32 tag;
		struct {
			U16 TPID; /**< should be 0x8100 in most VLAN tags */
			U16 TCI;
		};
	};
}real_vlan_hdr_t;


/* 
* IPV4
*
*/


/* IP flags. */
#define IP_DF		0x4000		/* Flag: "Don't Fragment"	*/
#define IP_MF		0x2000		/* Flag: "More Fragments"	*/
#define IP_OFFSET	0x1FFF		/* "Fragment Offset" part	*/

typedef struct  IPv4_HDR_STRUCT
{
        unsigned char Version_IHL;
        unsigned char TypeOfService;
        unsigned short TotalLength;
        unsigned short Identification;
        unsigned short Flags_FragmentOffset;
        unsigned char  TTL;
        unsigned char  Protocol;
        unsigned short HeaderChksum;
        unsigned int SourceAddress;
        unsigned int DestinationAddress;
}  ipv4_hdr_t;

typedef struct _tNatt_Socket_v4{
        struct _tNatt_Socket_v4 *next;
        unsigned int src_ip;
        unsigned int dst_ip;
        unsigned short sport;
        unsigned short dport;
}Natt_Socket_v4, *PNatt_Socket_v4;



/* 
* IPV6
*
*/

typedef struct IPv6_HDR_STRUCT
{
        U16 Version_TC_FLHi;
        U16 FlowLabelLo;
        U16 TotalLength;
        U8  NextHeader;
        U8  HopLimit;
        U32 SourceAddress[4];
        U32 DestinationAddress[4];
} ipv6_hdr_t;

typedef struct _tNatt_Socket_v6{
        struct _tNatt_Socket_v6 *next;
        unsigned int src_ip[4];
        unsigned int dst_ip[4];
        unsigned short sport;
        unsigned short dport;
}Natt_Socket_v6, *PNatt_Socket_v6;


/* IPv6 Next Header values	 */   	 
#define IPV6_HOP_BY_HOP		0
#define IPV6_IPIP			4
#define IPV6_TCP			6
#define IPV6_UDP			17
#define IPV6_ROUTING		43
#define IPV6_FRAGMENT		44
#define IPV6_ESP			50	
#define IPV6_AUTHENTICATE	51
#define IPV6_ICMP			58
#define IPV6_NONE			59
#define IPV6_DESTOPT		60
#define IPV6_ETHERIP		97

#define	IP6_MF				0x0001

#define IPV6_MIN_MTU		1280


#ifdef ENDIAN_LITTLE

#define IPV6_GET_TRAFFIC_CLASS(phdr) ((((phdr)->Version_TC_FLHi & 0x000F) << 4) | (phdr)->Version_TC_FLHi >> 12)
#define IPV6_SET_TRAFFIC_CLASS(phdr, tc) do { \
		U16 temp = (phdr)->Version_TC_FLHi & 0x0FF0; \
		temp |= (tc) >> 4; \
		temp |= ((tc) & 0xF) << 12; \
		(phdr)->Version_TC_FLHi = temp; \
		} while (0)
#define IPV6_GET_VERSION(phdr) (((phdr)->Version_TC_FLHi >> 4) & 0xF)
#define IPV6_SET_VERSION(phdr, vers) do { \
		U16 temp = (phdr)->Version_TC_FLHi & 0xFF0F; \
		temp |= (vers) << 4; \
		(phdr)->Version_TC_FLHi = temp; \
		} while (0)
#define IPV6_GET_FLOW_LABEL_HI(phdr) (((phdr)->Version_TC_FLHi >> 8) & 0x000F)
#define IPV6_SET_FLOW_LABEL_HI(phdr, flhi) do { \
		U16 temp = (phdr)->Version_TC_FLHi & 0xF0FF; \
		temp |= (flhi) << 8; \
		(phdr)->Version_TC_FLHi = temp; \
		} while (0)
#define IPV6_SET_FLOW_LABEL(phdr, fl) do { \
		U16 flhi = ((fl) >> 16) & 0x000f; \
		IPV6_SET_FLOW_LABEL_HI((phdr), flhi); \
		(phdr)->FlowLabelLo = htons((fl) & 0xffff); \
		} while (0)
#define IPV6_COPY_FLOW_LABEL(phdr_to, phdr_from) do { \
		IPV6_SET_FLOW_LABEL_HI((phdr_to), IPV6_GET_FLOW_LABEL_HI(phdr_from)); \
		(phdr_to)->FlowLabelLo = (phdr_from)->FlowLabelLo; \
		} while (0)

#else

#define IPV6_GET_TRAFFIC_CLASS(phdr) (((phdr)->Version_TC_FLHi >> 4) & 0xFF)
#define IPV6_SET_TRAFFIC_CLASS(phdr, tc) do { \
		U16 temp = (phdr)->Version_TC_FLHi & 0xF00F; \
		temp |= (tc) << 4; \
		(phdr)->Version_TC_FLHi = temp; \
		} while (0)
#define IPV6_GET_VERSION(phdr) ((phdr)->Version_TC_FLHi >> 12)
#define IPV6_SET_VERSION(phdr, vers) do { \
		U16 temp = (phdr)->Version_TC_FLHi & 0x0FFF; \
		temp |= (vers) << 12; \
		(phdr)->Version_TC_FLHi = temp; \
		} while (0)
#define IPV6_GET_FLOW_LABEL_HI(phdr) ((phdr)->Version_TC_FLHi & 0x000F)
#define IPV6_SET_FLOW_LABEL_HI(phdr, flhi) do { \
		U16 temp = (phdr)->Version_TC_FLHi & 0xFFF0; \
		temp |= (flhi); \
		(phdr)->Version_TC_FLHi = temp; \
		} while (0)
#define IPV6_SET_FLOW_LABEL(phdr, fl) do { \
		U16 flhi = ((fl) >> 16) & 0x000f; \
		IPV6_SET_FLOW_LABEL_HI((phdr), flhi); \
		(phdr)->FlowLabelLo = htons((fl) & 0xffff); \
		} while (0)
#define IPV6_COPY_FLOW_LABEL(phdr_to, phdr_from) do { \
		IPV6_SET_FLOW_LABEL_HI((phdr_to), IPV6_GET_FLOW_LABEL_HI(phdr_from)); \
		(phdr_to)->FlowLabelLo = (phdr_from)->FlowLabelLo; \
		} while (0)

#endif



#endif


