/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _FPP_GLOBALS_H_
#define _FPP_GLOBALS_H_

#include "types.h"
#include "modules.h"
#include "system.h"
#include "fppdiag_lib.h"


#if defined(FPP_DIAGNOSTICS) && !defined(COMCERTO_2000)
#define DEBUG_COUNTERS	1 // Enable Debug Counters when fpp diagnostics is enabled.
#endif

typedef struct _tFppGlobals {
#if !defined(COMCERTO_2000)
	EVENTHANDLER FG_gEventDataTable[EVENT_MAX];
	U32 FG_gEventStatusReg;
	U32 FG_gIRQEventStatusReg;
	STIME FG_cpu_idle_end;
	U32 FG_cpu_busy_cycles;
	U32 FG_cpu_idle_cycles;
	HANDLE FG_hGlobalHeap;
	HANDLE FG_hGlobalAramHeap;

	U32 FG_ipv4_fill_cntr;
	U32 FG_ipv4_free_cntr;
	U32 FG_ipv6_fill_cntr;
	U32 FG_ipv6_free_cntr;
#endif
	/*TODO no more required for c2k, but things are not working if they are removed */
	U16 FG_IPv6fragTimeout;
	U16 FG_IPv4fragTimeout;
	U16 FG_IPv6fragExpiryDrop;
	U16 FG_IPv4fragExpiryDrop;

	U32 FG_ct_bin_start;
	U32 FG_udp_unidir_timeout;
	U32 FG_udp_bidir_timeout;
	U32 FG_tcp_timeout;
	U32 FG_ct_timer;
	U32 FG_identification;
	U32 FG_g_mc6_mode;
	U8 FG_tx_channel[MAX_PHY_PORTS];
	void *FG_preload_addr;
#ifdef WIFI_ENABLE
	U32 rsvd[4]; 		/* FIXME: Added padding to align with pfe_ctrl*/
#endif

#if !defined(COMCERTO_2000)
#ifdef FPP_STATISTICS
	U32 FG_statFeatureBitMask;
#endif
#endif

	channel_t FG_RxOutputChannel[MAX_L3_PROTO+1];
	U8  FG_ff_enable;
	U8  FG_ipsec_pre_frag;
	U32 FG_PPPoE_entries;

	struct {
		U8 used;
		U8 enabled;
		U16 onif_index;
	} FG_L2bridge_enabled[MAX_PHY_PORTS];

	U32 FG_L2Bridge_entries;
	U8 FG_L2Bridge_mode;
	U16 FG_L2Bridge_timeout;
	U16 FG_L2Bridge_bin_number;
	U16 FG_L2Bridge_l3_bin_number;


#if defined(COMCERTO_1000)
	U16 FG_TX_Q_Sizes[8];
	U32 POOL_A_BASE;
	U32 POOL_A_SIZE;
	U32 POOL_B_BASE;
	U32 POOL_B_SIZE;
#endif
} tFppGlobals;

typedef struct _tUtilGlobals {
	U32 FG_ipv4_fill_cntr;
	U32 FG_ipv4_free_cntr;
	U32 FG_ipv6_fill_cntr;
	U32 FG_ipv6_free_cntr;
	U16 FG_IPv6fragTimeout;
	U16 FG_IPv6fragExpiryDrop;
	U16 FG_IPv4fragTimeout;
	U16 FG_IPv4fragExpiryDrop;
	U32 res[2];
} tUtilGlobals;
extern tFppGlobals gFppGlobals __attribute__((aligned(32)));
extern tUtilGlobals gUtilGlobals __attribute__((aligned(32)));
#if defined(COMCERTO_2000_CONTROL)
extern tFppGlobals CLASS_VARNAME2(gFppGlobals) __attribute__((aligned(32)));
extern tUtilGlobals UTIL_VARNAME2(gUtilGlobals) __attribute__((aligned(32)));
#endif
extern struct physical_port phy_port[MAX_PHY_PORTS];

#if !defined(COMCERTO_2000)
#define gEventDataTable gFppGlobals.FG_gEventDataTable
#define gEventStatusReg gFppGlobals.FG_gEventStatusReg
#define gIRQEventStatusReg gFppGlobals.FG_gIRQEventStatusReg
#endif
#define tx_channel gFppGlobals.FG_tx_channel


#ifdef COMCERTO_1000
#define TX_Q_Sizes gFppGlobals.FG_TX_Q_Sizes
#define gPOOL_A_BASE (gFppGlobals.POOL_A_BASE)
#define gPOOL_A_SIZE (gFppGlobals.POOL_A_SIZE)
#define gPOOL_B_BASE (gFppGlobals.POOL_B_BASE)
#define gPOOL_B_SIZE (gFppGlobals.POOL_B_SIZE)
#endif



#define RxOutputChannel gFppGlobals.FG_RxOutputChannel
#define cpu_busy_cycles (gFppGlobals.FG_cpu_busy_cycles)
#define cpu_idle_cycles (gFppGlobals.FG_cpu_idle_cycles)
#define cpu_idle_end	gFppGlobals.FG_cpu_idle_end
#define hGlobalHeap (gFppGlobals.FG_hGlobalHeap)
#define hGlobalAramHeap (gFppGlobals.FG_hGlobalAramHeap)
#define ct_timer gFppGlobals.FG_ct_timer
#define ct_bin_start gFppGlobals.FG_ct_bin_start
#define udp_unidir_timeout gFppGlobals.FG_udp_unidir_timeout
#define udp_bidir_timeout gFppGlobals.FG_udp_bidir_timeout
#define tcp_timeout gFppGlobals.FG_tcp_timeout
#define fpp_ident gFppGlobals.FG_identification
#define ff_enable gFppGlobals.FG_ff_enable
#if defined (COMCERTO_2000)
extern tUtilGlobals gUtilGlobals __attribute__((aligned(32)));
#define ipv6_frag_timeout gUtilGlobals.FG_IPv6fragTimeout
#define ipv4_frag_timeout gUtilGlobals.FG_IPv4fragTimeout
#define ipv6_frag_expirydrop gUtilGlobals.FG_IPv6fragExpiryDrop
#define ipv4_frag_expirydrop gUtilGlobals.FG_IPv4fragExpiryDrop
#define ipv4_fill_cntr gUtilGlobals.FG_ipv4_fill_cntr
#define ipv4_free_cntr gUtilGlobals.FG_ipv4_free_cntr
#define ipv6_fill_cntr gUtilGlobals.FG_ipv6_fill_cntr
#define ipv6_free_cntr gUtilGlobals.FG_ipv6_free_cntr
#else
#define ipv6_frag_timeout gFppGlobals.FG_IPv6fragTimeout
#define ipv4_frag_timeout gFppGlobals.FG_IPv4fragTimeout
#define ipv6_frag_expirydrop gFppGlobals.FG_IPv6fragExpiryDrop
#define ipv4_frag_expirydrop gFppGlobals.FG_IPv4fragExpiryDrop
#define ipv4_fill_cntr gFppGlobals.FG_ipv4_fill_cntr
#define ipv4_free_cntr gFppGlobals.FG_ipv4_free_cntr
#define ipv6_fill_cntr gFppGlobals.FG_ipv6_fill_cntr
#define ipv6_free_cntr gFppGlobals.FG_ipv6_free_cntr
#endif
#define g_mc6_mode gFppGlobals.FG_g_mc6_mode
#define preload_addr gFppGlobals.FG_preload_addr


#define PPPoE_entries gFppGlobals.FG_PPPoE_entries

#if !defined(COMCERTO_2000)
#define L2bridge_enabled gFppGlobals.FG_L2bridge_enabled
#define L2Bridge_entries gFppGlobals.FG_L2Bridge_entries
#define L2Bridge_mode gFppGlobals.FG_L2Bridge_mode
#define L2Bridge_timeout gFppGlobals.FG_L2Bridge_timeout
#define L2Bridge_bin_number gFppGlobals.FG_L2Bridge_bin_number
#define L2Bridge_l3_bin_number gFppGlobals.FG_L2Bridge_l3_bin_number
#endif

#define ipsec_pre_frag gFppGlobals.FG_ipsec_pre_frag

#ifdef DEBUG_COUNTERS
#ifdef COMCERTO_2000
typedef struct _tDebugCounters {
  U32	DEBUG_packets_received[5];			// offset 0x00
  U32	DEBUG_packets_received_expt[5];			// offset 0x14
  U32	DEBUG_packets_transmitted[5];			// offset 0x28
  U32	DEBUG_packets_transmitted_expt[5];		// offset 0x3c
  //U32	DEBUG_packets_dropped_qm[5];			// offset 0x50
  U32	DEBUG_packets_dropped_tx[5];			// offset 0x64
  U32	DEBUG_packets_dropped_rxerror[5];		// offset 0x78
  U32	DEBUG_packets_dropped_icc[5];			// offset 0x8c
  U32	DEBUG_packets_dropped_expt[5];			// offset 0xa0
  //U32	DEBUG_rx_no_metadata;				// offset 0xb4
  U32	DEBUG_packets_dropped_expt_invalid_port;	// offset 0xb8
  U32	DEBUG_packets_dropped_ipv4_cksum;		// offset 0xbc
  U32	DEBUG_packets_dropped_ipv4_fragmenter;		// offset 0xc0
  U32	DEBUG_packets_dropped_ipv6_fragmenter;		// offset 0xc4
  //U32	DEBUG_packets_dropped_channel_full;		// offset 0xc8
  U32	DEBUG_packets_dropped_socket_not_bound1;	// offset 0xcc
  U32	DEBUG_packets_dropped_socket_not_bound2;	// offset 0xd0
  U32	DEBUG_packets_dropped_socket_not_bound3;	// offset 0xd4
  U32	DEBUG_packets_dropped_socket_no_route;		// offset 0xd8

  U32	DEBUG_packets_dropped_rtp_relay_no_flow;	// offset 0xdc
  U32	DEBUG_packets_dropped_rtp_relay_no_socket;	// offset 0xe0
  U32	DEBUG_packets_dropped_rtp_relay_discard;	// offset 0xe4
  U32	DEBUG_packets_dropped_rtp_relay_misc;		// offset 0xe8

  U32	DEBUG_packets_dropped_ipsec_inbound;		// offset 0xec
  U32	DEBUG_packets_dropped_ipsec_outbound;		// offset 0xf0
  U32	DEBUG_packets_dropped_ipsec_rate_limiter;	// offset 0xf4

  U32	DEBUG_packets_dropped_natt;			// offset 0xf8
  U32	DEBUG_packets_dropped_natpt;			// offset 0xfc

  U32	DEBUG_packets_dropped_mc4;			// offset 0x100
  U32	DEBUG_packets_dropped_mc6;			// offset 0x104
  U32	DEBUG_packets_dropped_fragments;		// offset 0x108
  U32	DEBUG_packets_dropped_expt_rx_ipsec;		// offset 0x10c
} tDebugCounters;
#else
typedef struct _tDebugCounters {
  U32	DEBUG_packets_received[4];			// offset 0x00
  U32	DEBUG_packets_received_expt[4];			// offset 0x10
  U32	DEBUG_packets_transmitted[4];			// offset 0x20
  U32	DEBUG_packets_transmitted_expt[4];		// offset 0x30
  U32	DEBUG_packets_dropped_qm[4];			// offset 0x40
  U32	DEBUG_packets_dropped_tx[4];			// offset 0x50
  U32	DEBUG_packets_dropped_rxerror[4];		// offset 0x60
  U32	DEBUG_packets_dropped_icc[4];			// offset 0x70
  U32	DEBUG_packets_dropped_expt[4];			// offset 0x80
  U32	DEBUG_packets_dropped_poolA;			// offset 0x90
  U32	DEBUG_packets_dropped_poolB;			// offset 0x94
  U32	DEBUG_rx_no_metadata;				// offset 0x98
  U32	DEBUG_packets_dropped_expt_invalid_port;	// offset 0x9c
  U32	DEBUG_packets_dropped_ipv4_cksum;		// offset 0xa0
  U32	DEBUG_packets_dropped_ipv4_fragmenter;		// offset 0xa4
  U32	DEBUG_packets_dropped_ipv6_fragmenter;		// offset 0xa8
  U32	DEBUG_packets_dropped_channel_full;		// offset 0xac
  U32	DEBUG_packets_dropped_socket_not_bound1;	// offset 0xb0
  U32	DEBUG_packets_dropped_socket_not_bound2;	// offset 0xb4
  U32	DEBUG_packets_dropped_socket_not_bound3;	// offset 0xb8
  U32	DEBUG_packets_dropped_socket_no_route;		// offset 0xbc
  U32	DEBUG_packets_dropped_rtp_relay_no_flow;	// offset 0xc0
  U32	DEBUG_packets_dropped_rtp_relay_no_socket;	// offset 0xc4
  U32	DEBUG_packets_dropped_rtp_relay_discard;	// offset 0xc8
  U32	DEBUG_packets_dropped_rtp_relay_misc;		// offset 0xcc
  U32	DEBUG_packets_dropped_ipsec_inbound;		// offset 0xd0
  U32	DEBUG_packets_dropped_ipsec_outbound;		// offset 0xd4
  U32	DEBUG_packets_dropped_ipsec_rate_limiter;	// offset 0xd8
  U32	DEBUG_packets_dropped_natt;			// offset 0xdc
  U32	DEBUG_packets_dropped_natpt;			// offset 0xe0
  U32	DEBUG_packets_dropped_mc4;			// offset 0xe4
  U32	DEBUG_packets_dropped_mc6;			// offset 0xe8
  U32	DEBUG_packets_dropped_fragments;		// offset 0xec
  U32	DEBUG_packets_dropped_expt_rx_ipsec;		// offset 0xf0
  U32	pad[3];
  U32	DEBUG_debug[16][4];				// offset 0x100
} tDebugCounters;
#endif

extern tDebugCounters gDebugCounters __attribute__((aligned(32)));

#define COUNTER_INCREMENT(counter) do { (gDebugCounters.DEBUG_##counter) += 1; } while(0)
#define COUNTER_ADD(counter, add_amount) do { (gDebugCounters.DEBUG_##counter) += add_amount; } while(0)
#define COUNTER_SET(counter, value) do { (gDebugCounters.DEBUG_##counter) = value; } while(0)
#define COUNTER_GET(counter)  (gDebugCounters.DEBUG_##counter)
#else
#define COUNTER_INCREMENT(counter) do {} while(0)
#define COUNTER_ADD(counter, add_amount) do {} while(0)
#define COUNTER_SET(counter, value) do {} while(0)
#define COUNTER_GET(counter) do {} while {0}
#endif


#if defined(COMCERTO_2000_CLASS)
extern U32 regMask[CLASS_MAX_PBUFFERS];
extern U32 pe_id;
#endif
#endif /* _FPP_GLOBALS_H_ */
