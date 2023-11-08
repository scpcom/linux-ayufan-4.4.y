/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 */


#ifndef _MTD_H_
#define _MTD_H_

#include "types.h"
#include "fpart.h"
#include "gemac.h"


#define MTD_PRIV 	3

/* data buffer contains reference count */
#define MTD_REASSEMBLED_PKT		0x1
#define MTD_TNL					0x2
#define	MTD_MULTIPLE			0x4
#define MTD_MULTICAST_TX		0x8
#define MTD_EXPT_TX				0x10
#define	MTD_IPSEC_INBOUND_MASK	0x60
#	define MTD_IPSEC_INBOUND_TRANSPORT	0x20
#	define MTD_IPSEC_INBOUND_TNL4		0x40
#	define MTD_IPSEC_INBOUND_TNL6		0x60
#	define MTD_IPSEC_INBOUND_MATCH		0x80
#define MTD_MSP					0x100
#define MTD_TX_CHECKSUM				0x200
#define MTD_RX_CHECKSUMMED			0x400
#define MTD_LRO					0x800
#define	MTD_IPSEC_OUTBOUND		0x1000
#define MTD_MC6				0x2000

#if defined(COMCERTO_2000_UTIL)

#include "util_mtd.h"

#else

typedef struct tMetadata {
	struct tMetadata *next;
	U16	length;
	U16	offset;
	U8	*data;

	// The following union must be identical for this structure and for the RX_context structure
#if defined(CFG_WIFI_OFFLOAD) || defined(COMCERTO_2000)
	union {
		U64 __attribute__((packed)) zero_init_fields;
		struct {
			U32 input_port : 8;
			U32 output_port : 8;
			U32 queue : 8;
			U32 rsvd : 8;
			U32 flags : 16;
			U32 repl_msk : 4;
			U32 vlan_pbits : 3;
			U32 ddr_offset: 8;
		} __attribute__((packed));
	};
#else
	union {
		U32 zero_init_fields;
		struct {
                        U32     input_port : 1;
                        U32     output_port : 1;
                        U32     repl_msk : 4;
			U32	baseoff : 7;
                        U32     vlan_pbits : 3;
                        U32     queue : 8;
                        U32     flags : 8;
                } __attribute__((packed));
	};

#endif

#if defined(COMCERTO_100)
	union {
		U32	Fstatus;
		U32	expt_tx_system;
	};
#elif defined(COMCERTO_1000)
	U32	Fstatus;
	U32	Fextstatus;
#elif defined(COMCERTO_2000_CLASS)
	hwparse_t *hwparse;
	void	*rx_next;
	void	*rx_dmem_end;	/* dmem end address of the packet received */
	u32	timestamp;
	u32	rx_status;
	struct physical_port	*rx_phy_port;
	u16	rx_length;
	u8	pbuf_i;
#endif

	union {
		// FIXME -- is this right????  problem with c2k???
	  	struct  { U32 pad_double; U64 dword;} __attribute__((packed));
		U32	word[MTD_PRIV];
		U16	half[ MTD_PRIV*2];
		U8	byte[MTD_PRIV*4];

		struct sec_path {
#if defined (COMCERTO_2000)
			union 
			{
				void* sec_rt;	
				struct sec_pe_info
				{
					u8 l2len[SA_MAX_OP];
					u8 output_port;
					u8 queue;
				}sec_pe;
			}sec_info;
#else
			void *sec_rt;
#endif

			U8  sec_proto;
			U8  sec_L4off;
#if defined(COMCERTO_100)
			U8  sec_wq_idx;
#elif defined(COMCERTO_1000) || defined (COMCERTO_2000)
		        U8  sec_L4_proto;
#endif
			S8  sec_sa_op;
			U16 sec_sa_h[SA_MAX_OP];
		} sec;

		struct input_info {
			U8 itf_index;
		} input;

		struct tunl_path {
			void* tnl_h;
			void *tnl_route;
			U16 id; /* Used to pass NAPTd source port for fragment ID overwrite */	
		} tnl;

		struct frag_info {
			U16 offset;
			U16 l4offset;
			U16 end;
			U16 l3offset;
		} frag;

		struct socket_info {
			U16 id;
			void *l3hdr;
			void *l4hdr;
		} socket;

		struct lro_info {
			void *l3_hdr;
			void *l4_hdr;
		} lro;

		struct l2tp_info {
			void *l2tp_itf;
		} l2tp;

		struct	mdma_transit {
		  U16 flags;	// flags passed between mdma rx and mdma tx
#if defined(COMCERTO_1000) || defined(COMCERTO_2000)
		  U16 refcnt;	// number of copies requested by the mdma
		  U16 rxndx;	// current index into listener array.
#endif
		  U8 dscp;	// dscp of received packet
		} mcast;

		struct rtpqos_info {
		 	U32 client;
			BOOL udp_check;
		} rtpqos;
	} priv;	
} Metadata, *PMetadata;


#define priv_proto		priv.sec.sec_proto
#define priv_L4_offset		priv.sec.sec_L4off
#define priv_sa_op		priv.sec.sec_sa_op
#define priv_sa_handle		priv.sec.sec_sa_h
#if defined(COMCERTO_2000)
#define priv_rt			priv.sec.sec_info.sec_rt
#define priv_pe_l2len		priv.sec.sec_info.sec_pe.l2len
#define priv_pe_queue		priv.sec.sec_info.sec_pe.queue
#define priv_pe_outport		priv.sec.sec_info.sec_pe.output_port
/* priv_proto is overloaded to send information from class to utilpe
 * in case of ipsec outbound to update the length. This field contains
 * the type of interface to update the packet length in utilpe */
#define priv_sa_outitf          priv.sec.sec_proto
#else
#define priv_rt			priv.sec.sec_rt
#endif
#define iitf_index		priv.input.itf_index
#define priv_tnl		priv.tnl.tnl_h
#define priv_tnl_rt		priv.tnl.tnl_route
#define priv_tnl_id		priv.tnl.id
#define priv_socketid		priv.socket.id
#define priv_l3hdr		priv.socket.l3hdr
#define priv_l4hdr		priv.socket.l4hdr
#define priv_lro_l3hdr		priv.lro.l3_hdr
#define priv_lro_l4hdr		priv.lro.l4_hdr
#ifdef COMCERTO_100
#define priv_wq_idx		priv.sec.sec_wq_idx
#endif
#define priv_l2tp		priv.l2tp.l2tp_itf
#define priv_L4_proto		priv.sec.sec_L4_proto

#define priv_mcast_flags	priv.mcast.flags

#if defined(COMCERTO_1000) || defined(COMCERTO_2000)
#define priv_mcast_refcnt	priv.mcast.refcnt
#define priv_mcast_rxndx	priv.mcast.rxndx
#endif	// COMCERTO_1000
#define priv_mcast_dscp		priv.mcast.dscp

#define	MC_LASTMTD_WIFI 1
#define	MC_BRIDGED	2

#if !defined(COMCERTO_2000)

extern FASTPART MetadataPart;
extern FASTPART MetadataDDRPart;
extern Metadata MetadataStorage[];
extern Metadata MetadataDDRStorage[];

static __inline PMetadata mtd_alloc(void)
{

	PMetadata mtd;

	mtd = SFL_alloc_part(&MetadataPart);
	if (mtd == NULL)
		mtd = SFL_alloc_part(&MetadataDDRPart);

	return mtd;
}

static __inline void mtd_free(PMetadata mtd)
{
	if ((PVOID)mtd < (PVOID)MetadataStorage)
		 SFL_free_part(&MetadataDDRPart, mtd);
	else
		SFL_free_part(&MetadataPart, mtd);
}
#else
static __inline PMetadata mtd_alloc(void)
{
	return NULL; /* FIXME maybe allocate a bmu1 buffer and use it for both mtd and payload... */
}

static __inline void mtd_free(PMetadata mtd)
{

}

#endif


#if defined(COMCERTO_100)

static inline int get_proto(struct tMetadata *mtd)
{
	int proto_match = mtd->Fstatus & RX_STA_TYPEID_MASK;

	/* The caller must check that proto_match != 0 */

	__asm
	{
		CLZ	proto_match, proto_match
		RSB	proto_match, proto_match, (31-RX_STA_TYPEID_POS)
	}

	if ((mtd->Fstatus & RX_STA_MCAST) && (proto_match < PROTO_PPPOE))
		proto_match += PROTO_MC4;

	return proto_match;
}


static inline BOOL is_multicast(struct tMetadata *mtd)
{
	return (mtd->Fstatus & RX_STA_MCAST) != 0;
}


static inline BOOL is_vlan(struct tMetadata *mtd)
{
	return (mtd->Fstatus & RX_STA_VLAN) != 0;
}

#elif defined(COMCERTO_1000)

static inline U32 get_proto(struct tMetadata *mtd)
{
	U32 proto_match = (mtd->Fextstatus & RX_STA_IPV4) ? PROTO_IPV4 : PROTO_IPV6;

	if (mtd->Fstatus & RX_STA_MCAST)
		proto_match += PROTO_MC4;
	else {
		if (mtd->Fextstatus & RX_STA_PPPOE) {
			// ipv4 or ipv6 is checked already
			// we could check TCP or UDP, but what about IPSec
			proto_match = PROTO_PPPOE;
		}
	}

	return proto_match;
}


static inline BOOL is_multicast(struct tMetadata *mtd)
{
	return (mtd->Fstatus & RX_STA_MCAST) != 0;
}


static inline BOOL is_vlan(struct tMetadata *mtd)
{
	return (mtd->Fstatus & RX_STA_VLAN) != 0;
}


static inline BOOL is_pppoe(struct tMetadata *mtd)
{
	return (mtd->Fextstatus & RX_STA_PPPOE) != 0;
}


static inline BOOL is_ipv4(struct tMetadata *mtd)
{
	return (mtd->Fextstatus & RX_STA_IPV4) != 0;
}


#elif defined(COMCERTO_2000_CLASS)

/* Returns the DDR buffer offset corresponding to the DMEM pbuf address */
#define ddr_offset(mtd, dmem_addr)	(((u32)(mtd)->data & 0xff) + ((void *)(dmem_addr) - (void *)((mtd)->data)) + (mtd)->ddr_offset)

/* Returns the DDR buffer address corresponding to the DMEM pbuf address */
#define ddr_addr(mtd, dmem_addr)	((mtd)->rx_next + ddr_offset(mtd, dmem_addr))

void class_mtd_copy_to_ddr(PMetadata mtd, void *ddr_start, u32 len, void *dmem_buf, unsigned int dmem_len);

//TODO Can the functions below be used near the end of packet processing (hwparse fields overwritten?)
static inline U32 get_proto(struct tMetadata *mtd)
{
	U32 proto_match = mtd->hwparse->parseFlags & PARSE_IPV6_TYPE ? PROTO_IPV6 : PROTO_IPV4;

	if (mtd->hwparse->parseFlags & PARSE_MCAST_TYPE)
		proto_match += PROTO_MC4;
	else {
		if (mtd->hwparse->parseFlags & PARSE_PPPOE_TYPE) {
			// ipv4 or ipv6 is checked already
			// we could check TCP or UDP, but what about IPSec
			proto_match = PROTO_PPPOE;
		}
	}

	return proto_match;
}


static inline BOOL is_mac0_hit(struct tMetadata *mtd)
{
	/* FIXME not correct, for now just answer yes for any hit on a specific mac */
	return (mtd->hwparse->parseFlags & PARSE_ARC_HIT) != 0;
}


static inline BOOL is_multicast(struct tMetadata *mtd)
{
	return (mtd->hwparse->parseFlags & PARSE_MCAST_TYPE) != 0;
}


static inline BOOL is_vlan(struct tMetadata *mtd)
{
	return (mtd->hwparse->parseFlags & PARSE_VLAN_TYPE) != 0;
}


static inline BOOL is_pppoe(struct tMetadata *mtd)
{
	return (mtd->hwparse->parseFlags & PARSE_PPPOE_TYPE) != 0;
}


static inline BOOL is_ipv4(struct tMetadata *mtd)
{
	return (mtd->hwparse->parseFlags & PARSE_IP_TYPE) != 0;
}


static inline BOOL is_ip(struct tMetadata *mtd)
{
	return (mtd->hwparse->parseFlags & (PARSE_IP_TYPE | PARSE_IPV6_TYPE)) != 0;
}

static inline BOOL is_hif(struct tMetadata *mtd)
{
	return (mtd->hwparse->parseFlags & PARSE_HIF_PKT) != 0;
}

static inline BOOL is_multiple_bmu(struct tMetadata *mtd)
{
	/* Second BMU2 will come when the length is more than 1904,
	but ipsec requires some space for its headers and data.
	So, here we alloted for safe side 112 bytes. So checking 
	the size 112 bytes before 1904. */
	return (mtd->rx_length > (DDR_BUF_SIZE - DDR_HDR_SIZE) );
}

static inline void set_vlan(struct tMetadata *mtd)
{
	mtd->hwparse->parseFlags |= PARSE_VLAN_TYPE;
}


static inline void set_pppoe(struct tMetadata *mtd)
{
	mtd->hwparse->parseFlags |= PARSE_PPPOE_TYPE;
}


static inline void set_ipv4(struct tMetadata *mtd)
{
	mtd->hwparse->parseFlags |= PARSE_IP_TYPE;
}


static inline void set_ipv6(struct tMetadata *mtd)
{
	mtd->hwparse->parseFlags |= PARSE_IPV6_TYPE;
}

static inline void mtd_truncate(struct tMetadata *mtd, u32 length)
{
	mtd->length = length;

	if ((void *)mtd->data + mtd->offset + mtd->length < mtd->rx_dmem_end)
		mtd->rx_dmem_end = mtd->data + mtd->offset + mtd->length;
}

#endif

#endif

#endif /* _MTD_H_ */
