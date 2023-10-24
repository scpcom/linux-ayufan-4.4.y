/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


/* $id$ */
#ifndef _MULTICAST_H_
#define	_MULTICAST_H_

#include "system.h"
#include "fpp.h"
#include "module_expt.h"
#include "module_qm.h"
#ifdef COMCERTO_1000
#include "xdma.h"
#endif
#include "channels.h"
#if defined(COMCERTO_1000)||defined(COMCERTO_2000_CLASS)
#include "layer2.h"
#endif
#include "module_ipv4.h"
#include "module_ipv6.h"
#ifdef COMCERTO_2000_CLASS
#include "module_tx.h"
#include "pfe/class.h"
#endif
#define MCx_MAX_LISTENERS_PER_GROUP	4
#define MC_MODE_BRIDGED 0x0001
#define MC_MODE_ROUTED 0x0000
#define MC_MODE_MASK 0x0001
#define MC_ACP_LISTENER 0x2

#define MC4_MAC_CHECKER 0x0001005E // in big-endian format

#define MC4_MAC_DEST_MARKER1 ntohs(0x0100)
#define MC4_MAC_DEST_MARKER2 ntohs(0x5E00)

#define MC6_MAC_DEST_MARKER 0x3333

#if defined(COMCERTO_2000)
#define HASH_ENTRY_VALID		(1 << 0)
#define HASH_ENTRY_USED		(1 << 1)
#define HASH_ENTRY_UPDATING	(1 << 2)
#endif

#ifdef COMCERTO_100
typedef struct _tMCListener {
	//Make L2 header aligned the same way as BaseOffset
#if ((BaseOffset & 0x3) == 0)
	U8 L2_header[ETH_HDR_SIZE + VLAN_HDR_SIZE];
	U8 L2_header_size;
	U8 output_index;
#else
	U8 L2_header_size;
	U8 output_index;
	U8 L2_header[ETH_HDR_SIZE + VLAN_HDR_SIZE];
#endif
	int timer;
	U8 output_port;
  	U8 shaper_mask;
	} __attribute__((aligned(4))) MCListener, *PMCListener, *PMC4Listener, *PMC6Listener;

#else
typedef struct _tMCListener {
	struct itf *itf;
	int timer;
	U8 output_index;
	U8 output_port;
  	U8 shaper_mask;
  	U8 family;
} __attribute__((aligned(4))) MCListener, *PMCListener, *PMC4Listener, *PMC6Listener;
#endif

typedef struct _tMCxEntry {
	U8 src_mask_len;
	U8 num_listeners;
	U8 queue_base;
	U8 flags;
	int wifi_listener_timer;
	MCListener listeners[MCx_MAX_LISTENERS_PER_GROUP];
}MCxEntry, *PMCxEntry;

int multicast_init(void);

static __inline void mc_to_expt(PMetadata mtd)
{
	mtd->queue = DSCP_to_Q[mtd->priv_mcast_dscp];
	SEND_TO(EXPT, mtd);
}

/** Computes a multicast MAC address based on the IP (v4 or v6) header.
 * Only assumption is that variables are 16-bit aligned.
 * C2k PFE class version differs in the handling of endianness in the IPv4 case,
 * and is also a bit optimized for the PE CPU (this is the reason it doesn't use
 * READ_UNALIGNED_INT/WRITE_UNALIGNED_INT macros).
 * @param[out]	dstmac 	Pointer to destination buffer
 * @param[in]	iphdr 	IP (v4 or v6) header to be used to generate the MAC address.
 * @param[in]	family	Either PROTO_IPV4 or PROTO_IPV6, depending on IP version.
 */
#if !defined(COMCERTO_2000_CLASS)
static __inline void mc_mac_from_ip(U8 *dstmac, void *iphdr, U8 family)
{
	if (family == PROTO_IPV4)
	{
		U32 da = READ_UNALIGNED_INT(((ipv4_hdr_t *) iphdr)->DestinationAddress);
		*(U16 *)dstmac = MC4_MAC_DEST_MARKER1;
		__WRITE_UNALIGNED_INT(dstmac + 2, MC4_MAC_DEST_MARKER2 | (da & 0xFFFF7F00));
	} else
	{
		U32 da_lo = READ_UNALIGNED_INT(((ipv6_hdr_t *) iphdr)->DestinationAddress[IP6_LO_ADDR]);
		*(U16 *)dstmac = MC6_MAC_DEST_MARKER;
		__WRITE_UNALIGNED_INT(dstmac + 2, da_lo);
	}
}
#else
static __inline void mc_mac_from_ip(U8 *dstmac, void *iphdr, U8 family)
{
	if (family == PROTO_IPV4)
	{
		U16 *da = (U16 *)&((ipv4_hdr_t *) iphdr)->DestinationAddress;
		*(U16 *)dstmac = MC4_MAC_DEST_MARKER1;
		*(U16 *)(dstmac + 2) = (U16) (MC4_MAC_DEST_MARKER2 | (da[0] & 0x007F));
		*(U16 *)(dstmac + 4) = da[1];
	} else
	{
		U16 *da = (U16 *)&((ipv6_hdr_t *) iphdr)->DestinationAddress[IP6_LO_ADDR];
		*(U16 *)dstmac = MC6_MAC_DEST_MARKER;
		*(U16 *)(dstmac + 2) = da[0];
		*(U16 *)(dstmac + 4) = da[1];
	}
}
#endif

#ifdef COMCERTO_100
static __inline void mc_prepend_header(PMetadata mtd, PMCListener pmcl)
{
	int l2len;
	U8 *p;

	l2len = pmcl->L2_header_size;
	mtd->offset -= l2len;
	mtd->length += l2len;
	p = mtd->data + mtd->offset;

	mtd->output_port = pmcl->output_port;

#if ((BaseOffset & 0x3) == 0)
	if (mtd->priv_mcast_flags & MC_BRIDGED)
	{
		// need to modify L2_header to reflect unmodified src mac
		*(U16*)(pmcl->L2_header + 6) = *(U16*)(mtd->data + BaseOffset + 6);
		*(U32*)(pmcl->L2_header + 8) = *(U32*)(mtd->data + BaseOffset + 8);
	}
	*(U32*)p = *(U32*)&pmcl->L2_header[0];
	*(U32*)(p + 4) = *(U32*)&pmcl->L2_header[4];
	*(U32*)(p + 8) = *(U32*)&pmcl->L2_header[8];
	if (l2len ==  ETH_HDR_SIZE) {
		*(U16*)(p + 12) = *(U16*)&pmcl->L2_header[12];
	} else {
		*(U32*)(p + 12) = *(U32*)&pmcl->L2_header[12];
		*(U16*)(p + 16) = *(U16*)&pmcl->L2_header[16];
	}
#else
	if (mtd->priv_mcast_flags & MC_BRIDGED)
	{
		// need to modify L2_header to reflect unmodified src mac
		*(U32*)(pmcl->L2_header + 6) = *(U32*)(mtd->data + BaseOffset + 6);
		*(U16*)(pmcl->L2_header + 10) = *(U16*)(mtd->data + BaseOffset + 10);
	}
	if (l2len ==  ETH_HDR_SIZE) {
		copy16(p,&pmcl->L2_header[0]);
	} else {
		copy20(p,&pmcl->L2_header[0]);
	}
#endif
}

/** Clone and send a packet.
 * Device-specific (C100), should only be called by mc_clone_and_send.
 * @param[in] mtd				Metadata for the packet to be sent
 * @param[in] mcdest			Multicast destination entry to send the packet to
 * @param[in] first_listener	First output destination
 */
static __inline void __mc_clone_and_send(PMetadata mtd, PMCxEntry mcdest, PMCListener first_listener)
{
	PMCListener this_listener;
	PMetadata mtd_new;
	U32 num_copies;
	U32 n = mcdest->num_listeners;
	int i;

	if (n == 1 && (mcdest->flags & MC_ACP_LISTENER == 0))
	{
		// here if just one listener (and no wifi)
		mc_prepend_header(mtd, (PMCListener) first_listener);
		send_to_tx(mtd);
		return;
	}

	num_copies = 1;
	// have to replicate
	// prepare mtd properties to copy to clones.
	mtd->flags |= MTD_MULTICAST_TX;

	if (mcdest->flags & MC_ACP_LISTENER)
	{
		if ((mtd_new = mtd_alloc()) != NULL) {
			*mtd_new = *mtd;
			num_copies++;
			mc_to_expt(mtd_new);
		}
	}

	for (i = 0; i < n - 1; i++)
	{
		this_listener = &mcdest->listeners[i];
		if ((mtd_new = mtd_alloc()) != NULL) {
			*mtd_new = *mtd;
			if (mcdest->flags & MC_MODE_BRIDGED) {
				// need to modify L2_header to reflect unmodified src mac
				*(U16*)(this_listener->L2_header + 6) = *(U16*)(mtd->data + BaseOffset + 6);
				*(U32*)(this_listener->L2_header + 8) = *(U32*)(mtd->data + BaseOffset + 8);
				L1_dc_clean(this_listener->L2_header,this_listener->L2_header+sizeof(this_listener->L2_header)-1);
			}
			mtd_new->priv.dword =(U64) this_listener->L2_header | (( (U64) this_listener->L2_header_size)<< 32);
			mtd_new->output_port = this_listener->output_port;
			mtd_new->repl_msk = this_listener->shaper_mask;
			send_to_tx(mtd_new);
			num_copies++;
		}
		else {
			break;
		}
	}
	// last packet left - use original mtd.
	this_listener = &mcdest->listeners[i];
	if (mcdest->flags & MC_MODE_BRIDGED) {
		// need to modify L2_header to reflect unmodified src mac
		*(U16*)(this_listener->L2_header + 6) = *(U16*)(mtd->data + BaseOffset + 6);
		*(U32*)(this_listener->L2_header + 8) = *(U32*)(mtd->data + BaseOffset + 8);
		L1_dc_clean(this_listener->L2_header,this_listener->L2_header+sizeof(this_listener->L2_header)-1);
	}
	mtd->priv.dword =(U64) this_listener->L2_header |  (( (U64) this_listener->L2_header_size)<< 32);
	mtd->output_port = this_listener->output_port;
	mtd->repl_msk = this_listener->shaper_mask;
	send_to_tx(mtd);

	// save reference count in the data buffer
	*(U32*)(mtd->data+MC_REFCOUNT_OFFSET) = num_copies;
}
#endif




#ifdef COMCERTO_1000
static __inline void mc_prepend_header(PMetadata mtd, PMCListener pmcl)
{
		RouteEntry RtEntry;

		mc_mac_from_ip(RtEntry.dstmac, mtd->data+mtd->offset, pmcl->family);
		RtEntry.itf = pmcl->itf;
		l2_prepend_header(mtd, &RtEntry, (U16) pmcl->family);
}
//
// MDMA lane descriptor

typedef struct _mlane {
	V32	*txcnt;
	V32	*rxcnt;
	MMTXdesc *tx_top;
	MMTXdesc *tx_bot;
	MMTXdesc *tx_cur;
	U32	free_tx;
	MMRXdesc *rx_top;
	MMRXdesc *rx_bot;
	MMRXdesc *rx_cur;
	U32	pending_rx;
} mdma_lane;

#if defined(SIM) 
#define MDMA0_TX_NUMDESCR 8
#else
#define MDMA0_TX_NUMDESCR 48
#endif
#define MDMA0_RX_NUMDESCR ((MCx_MAX_LISTENERS_PER_GROUP-1) *MDMA0_TX_NUMDESCR) 

extern mdma_lane mdma_lane0_cntx;

static __inline int mdma_request_copy(PMetadata mtd, int num_copies, PMCxEntry listeners)
{
	// returns number of copies requested by mdma
	int copies_requested = 0;
#if !defined(COMCERTO_2000)

	U64 tmpU64;
	if (num_copies == 0)
		return 0;
	mdma_lane *pcntx = &mdma_lane0_cntx;
	if (pcntx->free_tx)
	{
		copies_requested = MDMA0_RX_NUMDESCR - pcntx->pending_rx;
		if (copies_requested > num_copies)
			copies_requested = num_copies;
		if (copies_requested > 0)
		{
			// Some copies will be made. Need to flush d-cache in the "copy from" buffer.
			// Only ttl is modified - can do a small flush. 
			// 	7 is an offset to last byte of ipv6.HopLimit 
			// 	11 is offset into ipv4.checksum
			L1_dc_clean(mtd->data+mtd->offset,mtd->data+mtd->offset+11);
			// Cache will be invalidated in module_tx or free_packet

			tmpU64 = (((U64) mtd) << 32) | ((U32) listeners);
			Write64(tmpU64, ((U64*) pcntx->tx_cur)+1);
			tmpU64 =(U64) (mtd->data+mtd->offset);
			tmpU64 += (((U64)mtd->length<<MMTX_LENGTH_SHIFT) | ((U64)copies_requested<< MMTX_NUMCPY_SHIFT) | ((U64)mtd->offset <<  MMTX_OFFSET_SHIFT ))<< 32;
			if (pcntx->tx_cur == pcntx->tx_bot)
			{
				tmpU64 += ((U64) MMTX_WRAP ) << 32;
				Write64(tmpU64, (U64*) pcntx->tx_cur);
				pcntx->tx_cur = pcntx->tx_top;
			}
			else
			{
				Write64(tmpU64, (U64*) pcntx->tx_cur);
				pcntx->tx_cur += 1;
			}
			*pcntx->txcnt = 1;
			*pcntx->rxcnt =  copies_requested;
			pcntx->pending_rx += copies_requested;
			pcntx->free_tx -= 1;
			set_event(gEventStatusReg, (1 << EVENT_MC6));
		}
	}
#endif
	return copies_requested;
}

static __inline int multicast_poola_hwm() {
    // report max number of poolA buffers held in mdma_rx
    return MDMA0_RX_NUMDESCR;
}
/** Clone and send a packet.
 * Device-specific (C1000), should only be called by mc_clone_and_send.
 * @param[in] mtd				Metadata for the packet to be sent
 * @param[in] mcdest			Multicast destination entry to send the packet to
 * @param[in] first_listener	First output destination
 */
static __inline void __mc_clone_and_send(PMetadata mtd, PMCxEntry mcdest, PMCListener first_listener)
{
	U32 num_copies;
	U32 n = mcdest->num_listeners;

	// clone using mdma
	if (mcdest->flags & MC_ACP_LISTENER) {
		// At the end mtd will be redirected to exception path
		mtd->priv_mcast_flags |= MC_LASTMTD_WIFI;
	} else {
		// At the end mtd will be used for last lan transmit
		n -= 1;
	}
	if ((num_copies = mdma_request_copy(mtd, n, mcdest)) > 0) {
		mtd->priv_mcast_refcnt = num_copies;
		mtd->priv_mcast_rxndx = 0;
	}
	else
	{
		// here if just one listener, or mdma request failed
		mc_prepend_header(mtd, (PMCListener)first_listener);
	#ifdef WIFI_ENABLE
		if (IS_WIFI_PORT(first_listener->output_port))
			  // DSCP_to_Qmod could also be considered here, but since we are no longer doing full Wifi offload,
			  // the packet will go to the exception path and so DSCP_to_Q makes more sense.
			  mtd->queue = DSCP_to_Q[mtd->priv_mcast_dscp];

	#endif
		send_to_tx(mtd);
	}
}
#endif	// COMCERTO_1000


#ifdef COMCERTO_2000_CLASS
/** Fill in a VLAN header structure based on given field values.
 * @param[out] pvlanHdr		pointer to VLAN header structure to fill in.
 * @param[in] ethertype		Ethertype to use, in network endianness.
 * @param[in] VlanId		VLAN id, in network endianness.
 * @param[in] vlan_pbits	VLAN priority bits (in bits [2:0]).
 * We can't use the vlan_hdr_t structure as it (purposefully) doesn't match the standard 802.1Q format.
 */
static inline void fill_real_vlan_header(real_vlan_hdr_t *pvlanHdr, U16 ethertype, U16 VlanId, U8 vlan_pbits)
{
	pvlanHdr->TPID = ethertype;
	pvlanHdr->TCI = VlanId | htons(vlan_pbits << 13);
}

/** Prepend L2 headers, excluding the outermost one in case of QinQ.
 * This behavior is needed on C2k because the hardware will take care of modifying
 * the last header, as long as it only means changing the src MAC address and/or VLAN tag.
 * @param mtd
 * @param pmcl
 */
static __inline U32 mc_prepend_upper_header(PMetadata mtd, PMCListener pmcl, U16 **srcMac, real_vlan_hdr_t *vlan_tag)
{
	U8 dstmac[ETHER_ADDR_LEN];
	U16 ethertype = l2_get_tid(pmcl->family);
	U16 vlanid = 0;
	U16 srcMac_orig[3];
	U16 *pkt_src_mac;
	struct itf *itf = pmcl->itf;
	int res = 0;

	mc_mac_from_ip(dstmac, mtd->data + mtd->offset, pmcl->family);
	pkt_src_mac = (U16 *) (mtd->data + BaseOffset + ETHER_ADDR_LEN);
	COPY_MACADDR(srcMac_orig, pkt_src_mac);

	if (itf->type & IF_TYPE_PPPOE)
		{
			M_pppoe_encapsulate(mtd, (pPPPoE_Info)itf, ethertype, 1);
			ethertype = ETHERTYPE_PPPOE_END;
			itf = itf->phys;
		}

#ifdef CFG_MACVLAN
	if (itf->type & IF_TYPE_MACVLAN)
	{
		*srcMac = ((PMacvlanEntry)itf)->MACaddr;
		itf = itf->phys;
	}
#endif

	/* We can add but not remove VLANs in hardware, so:
	 * . if we must add a VLAN tag, we store its value to program the HW later (so we'll be able to handle listeners
	 * with or without VLAN tags).
	 * . if we must add 2 VLAN tags (QinQ), we add the inner one and store the value of the second to program the HW later
	 * (so we'll be able to handle listeners with one or 2 tags).
	 * => This means we cannot handle regular ethernet and QinQ listeners in hardware for the same packet.
	 */
	if (itf->type & IF_TYPE_VLAN)
	{
		vlanid = ((PVlanEntry) itf)->VlanId;

		/* QinQ */
		if (itf->phys->type & IF_TYPE_VLAN)
		{
			res = 1;
			M_vlan_encapsulate(mtd, (PVlanEntry)itf, ethertype, 1);
			ethertype = ETHERTYPE_VLAN_END;
			itf = itf->phys;
			vlanid = ((PVlanEntry) itf)->VlanId;
		}
		itf = itf->phys;
	}

	if (vlanid)
	{
		fill_real_vlan_header(vlan_tag, ETHERTYPE_VLAN_END, vlanid, mtd->vlan_pbits);
	}

	mtd->length += ETH_HEADER_SIZE;
	mtd->offset -= ETH_HEADER_SIZE;

#ifdef CFG_MACVLAN
	if (itf->type & IF_TYPE_MACVLAN)
	{
		*srcMac = ((PMacvlanEntry)itf)->MACaddr;
		itf = itf->phys;
	}
	else
#endif
	if (!*srcMac)
		*srcMac = (U16 *) ((struct physical_port *)itf)->mac_addr;


	mtd->output_port = ((struct physical_port *)itf)->id;

	COPY_MACHEADER(mtd->data + mtd->offset, dstmac, srcMac_orig, ethertype);

	return res;
}

/* TODO this is the length of the metadata added to packets towards the exception path
 * so that the HIF driver on the host can determine how to handle the packet
 * (wifi offload, MSP, IPsec, etc).
 */
#define ACP_MTD_LEN 8

/** Clone and send a packet.
 * Device-specific (C2000), should only be called by mc_clone_and_send.
 * @param[in] mtd				Metadata for the packet to be sent
 * @param[in] mcdest			Multicast destination entry to send the packet to
 * @param[in] first_listener	First output destination
 */
static __inline void __mc_clone_and_send(PMetadata mtd, PMCxEntry mcdest, PMCListener first_listener)
{
	U32 n = mcdest->num_listeners;
	U32 packet_start;
	U16 *src_mac = NULL;
	int i;
	int first_is_vlan = 0;
	real_vlan_hdr_t vlan_tag;
	struct itf *itf;
	class_tx_hdr_mc_t *tx_hdr;
	u16 hif_hdr_len;

	vlan_tag.tag = 0;
	// TODO We need enough headroom for additional descriptors, otherwise send to exception path.

	/* Assume all listeners will have the same L2 headers after first ETH/VLAN fields.
	 * This assumption is no longer required on the C1k with the use of itf structs, but still is on the C2k.
	 */
	if (mcdest->flags & MC_ACP_LISTENER)
	{
		// We need to keep the packet unmodified to send to the host, so we only update pointers without touching data.
		// TODO do a packet copy instead for host packets in order to handle more cases.
		mtd->length += mtd->offset - BaseOffset;
		mtd->offset = BaseOffset;
		mtd->output_port = TMU3_PORT;
		mtd->queue = DSCP_to_Q[mtd->priv_mcast_dscp];
		mtd->priv_mcast_refcnt = n+1; // For C2k, priv_mcast_refcnt will be the total number of clones, i.e. one more than the C1k value.
		if (is_vlan(mtd)) //the hwparse structure should not be corrupt yet at this stage
			first_is_vlan = 1;
		i = 0;

		M_EXPT_compute_queue(mtd);
		hif_hdr_len = M_EXPT_prepare_hif_header(mtd);

		packet_start = (U32)mtd->data + mtd->offset - hif_hdr_len;
	} else
	{
		mtd->priv_mcast_refcnt = n; // For C2k, priv_mcast_refcnt will be the total number of clones, i.e. one more than the C1k value.
		first_is_vlan = mc_prepend_upper_header(mtd, first_listener, &src_mac, &vlan_tag);
		i = 1;
		packet_start = (U32)mtd->data + mtd->offset;
	}

	// First clone
	tx_hdr = (class_tx_hdr_mc_t *)(ALIGN64(packet_start) - sizeof(class_tx_hdr_mc_t));
	tx_hdr->start_data_off = packet_start - ((U32) tx_hdr);
	/* FIXME this only works if lmem_hdr_size == ddr_hdr_size, otherwise we need to add (ddr_hdr_size - lmem_hdr_size) */
	/* (src_addr & 0xff) is the offset from the 256 byte aligned packet slot */
	tx_hdr->start_buf_off = ((U32) tx_hdr) & 0xff;
	tx_hdr->queueno = mtd->queue;
	tx_hdr->act_phyno = (0 << 4) | (mtd->output_port & 0xfU);
	if (mcdest->flags & MC_ACP_LISTENER)
	{
		tx_hdr->pkt_length = mtd->length + hif_hdr_len;
		/* Now that we created the first clone, correct offset for the following clones that
		 * do not need the ACP metadata.
		 */
		packet_start += hif_hdr_len;
	}
	else
	{
		tx_hdr->pkt_length = mtd->length;
		if ((mcdest->flags & MC_MODE_MASK) == MC_MODE_ROUTED) {
			tx_hdr->act_phyno |= ACT_SRC_MAC_REPLACE;
			//src_mac already updated by mc_prepend_upper_header
			tx_hdr->src_mac_msb = src_mac[0];
			tx_hdr->src_mac_lsb = (src_mac[1] << 16) + src_mac[2];
		}
	}

	tx_hdr->vlanid = vlan_tag.tag;
	if  (vlan_tag.tag)
	{
		tx_hdr->act_phyno |= ACT_VLAN_ADD;
	}

	while (i < n)
	{
		tx_hdr--;
		tx_hdr->start_data_off = packet_start - (U32) tx_hdr;
		tx_hdr->start_buf_off = ((U32) tx_hdr) & 0xff;
		tx_hdr->pkt_length = mtd->length;
		tx_hdr->act_phyno = 0;

		// Now get to the bottom of the itf list to retrieve MAC, output port, and VLAN (if present)
		itf = mcdest->listeners[i].itf;
		vlan_tag.tag = 0;
		src_mac = NULL;
		while (itf->phys)
		{
#ifdef CFG_MACVLAN
			if (itf->type & IF_TYPE_MACVLAN)
					src_mac = ((PMacvlanEntry)itf)->MACaddr;
			else
#endif
			if (itf->type == IF_TYPE_VLAN)
			{
				if (vlan_tag.tag) // This means we're in QinQ mode, so only solution is to add the 2nd tag
					tx_hdr->act_phyno |= ACT_VLAN_ADD;
				fill_real_vlan_header(&vlan_tag, ETHERTYPE_VLAN_END, ((VlanEntry *)itf)->VlanId, mtd->vlan_pbits);
			}
			itf = itf->phys;
		}
		tx_hdr->vlanid = vlan_tag.tag;
		if (vlan_tag.tag)
		{
			if ((tx_hdr->act_phyno & ACT_VLAN_ADD) == 0) // We're not in QinQ mode
			{
				if (first_is_vlan)
					tx_hdr->act_phyno |= ACT_VLAN_REPLACE;
				else
					tx_hdr->act_phyno |= ACT_VLAN_ADD;
			}
		}

		tx_hdr->act_phyno |= (((struct physical_port *)itf)->id & 0xfU);
		tx_hdr->queueno = mtd->queue; // the queue number is the same for all listeners.
		if (!src_mac)
			src_mac = (U16 *) ((struct physical_port *)itf)->mac_addr;

		if ((mcdest->flags & MC_MODE_MASK) == MC_MODE_ROUTED) {
			tx_hdr->act_phyno |= ACT_SRC_MAC_REPLACE;
			tx_hdr->src_mac_msb = src_mac[0];
			tx_hdr->src_mac_lsb = (src_mac[1] << 16) + src_mac[2];
		}

		i++;

		if (first_is_vlan && !vlan_tag.tag) {
					PRINTF("MCAST entry not supported, skipping\n");
					tx_hdr++;
					mtd->priv_mcast_refcnt--;
		}

	}

	send_to_tx_mcast(mtd, (U32) tx_hdr);
}
#endif


#if defined(COMCERTO_2000_CLASS)||defined(COMCERTO_100)||defined(COMCERTO_1000)
/** Clone a packet and send copies to the relevant output interfaces.
 * The actual cloning operation is heavily device-dependent, so that function simply calls
 * __mc_clone_and_send after setting up common fields in the metadata structure.
 * @param[in] mtd		Metadata for the packet to be sent
 * @param[in] mcdest	Multicast destination entry to send the packet to
 */
static __inline void mc_clone_and_send(PMetadata mtd, PMCxEntry mcdest)
{
	PMCListener first_listener;
	U32 queue_mod = DSCP_to_Qmod[mtd->priv_mcast_dscp];

	mtd->priv_mcast_flags = 0;
	mtd->queue = mcdest->queue_base + queue_mod;
	first_listener = &mcdest->listeners[0];
	mtd->repl_msk = first_listener->shaper_mask;
	if ((mcdest->flags & MC_MODE_BRIDGED) != 0)
		mtd->priv_mcast_flags |= MC_BRIDGED;

	__mc_clone_and_send(mtd, mcdest, first_listener);
}
#endif




#endif	/* _MULTICAST_H_ */
