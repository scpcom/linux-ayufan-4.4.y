/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#include "hal.h"
#include "types.h"
#include "layer2.h"
#include "module_pppoe.h"
#include "module_vlan.h"
#include "module_mc4.h"
#include "module_mc6.h"
#include "module_qm.h"
#include "module_ipsec.h"
#include "module_tunnel.h"
#include "module_bridge.h"
#include "module_hidrv.h"
#include "module_rtp_relay.h"
#include "voicebuf.h"
#include "icc.h"

TIMER_ENTRY ip_timer;
TIMER_ENTRY mc6_timer;
TIMER_ENTRY mc6_delayed_remove_timer;
TIMER_ENTRY mc4_timer;
TIMER_ENTRY mc4_delayed_remove_timer;
TIMER_ENTRY bridge_timer;
TIMER_ENTRY bridge_delayed_removal_timer;
TIMER_ENTRY socket_timer;
TIMER_ENTRY rtpflow_timer;
TIMER_ENTRY voice_source_delayed_remove_timer;
TIMER_ENTRY sa_timer;


struct physical_port CLASS_DMEM_SH(phy_port)[MAX_PHY_PORTS];

/* PPPoE session cache */
struct slist_head CLASS_DMEM_SH(pppoe_cache)[NUM_PPPOE_ENTRIES] __attribute__((aligned(32)));
PPPoE_Info CLASS_DMEM_SH(pppoe_itf)[PPPOE_MAX_ITF];

struct slist_head CLASS_DMEM_SH(vlan_cache)[NUM_VLAN_ENTRIES] __attribute__((aligned(32)));
VlanEntry CLASS_DMEM_SH(vlan_itf)[VLAN_MAX_ITF];

OnifDesc gOnif_DB[L2_MAX_ONIF+1] __attribute__((aligned(32)));

struct slist_head CLASS_DMEM_SH(mc4_table_memory)[MC4_NUM_HASH_ENTRIES] __attribute__((aligned(32)));
struct slist_head CLASS_DMEM_SH(mc6_table_memory)[MC6_NUM_HASH_ENTRIES] __attribute__((aligned(32)));
struct tMC4_context CLASS_DMEM_SH(gMC4Ctx);
struct tMC6_context CLASS_DMEM_SH(gMC6Ctx);

struct slist_head ct_cache[NUM_CT_ENTRIES] __attribute__((aligned(32)));

struct slist_head bridge_cache[NUM_BT_ENTRIES] __attribute__((aligned(32)));
struct slist_head  bridge_l3_cache[NUM_BT_L3_ENTRIES]  __attribute__((aligned(32)));
struct _tbridge_enable L2bridge_enabled[MAX_PHY_PORTS] __attribute__((aligned(32)));

struct slist_head sock4_cache[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));
struct slist_head sock6_cache[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));
struct slist_head sockid_cache[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));

struct slist_head sa_cache_by_h[NUM_SA_ENTRIES] __attribute__((aligned(32)));
struct slist_head sa_cache_by_spi[NUM_SA_ENTRIES] __attribute__((aligned(32)));
struct slist_head rt_cache[NUM_ROUTE_ENTRIES] __attribute__((aligned(32)));

struct voice_buffer UTIL_DDR_SH(voicebuf)[VOICEBUF_MAX] __attribute__((aligned(32)));
U8 UTIL_DMEM_SH(voice_buffers_loaded);

U8 CLASS_DMEM_SH(DSCP_to_Q)[NUM_DSCP_VALUES];
U8 CLASS_DMEM_SH(DSCP_to_Qmod)[NUM_DSCP_VALUES];
U8 CLASS_DMEM_SH(CTRL_to_Q);
U8 CLASS_DMEM_SH(g_qos_enable)[MAX_PHY_PORTS];

ICCTAB CLASS_DMEM_SH(icctab);
U8 CLASS_DMEM_SH(lro_enable);
PNatt_Socket_v6 gNatt_Sock_v6_cache[NUM_IPSEC_SOCK_ENTRIES] __attribute__((aligned(32))) = {0};
PNatt_Socket_v4 gNatt_Sock_v4_cache[NUM_IPSEC_SOCK_ENTRIES] __attribute__((aligned(32))) = {0};
struct tIPSec_hw_context gIpSecHWCtx __attribute__((aligned(32)));
int gIpsec_available = 0;

struct tTNL_context CLASS_DMEM_SH(gTNLCtx);

CmdProc gCmdProcTable[EVENT_MAX];

tFppGlobals gFppGlobals __attribute__((aligned(32)));
tFppGlobals CLASS_DMEM_SH2(gFppGlobals) __attribute__((aligned(32)));

tUtilGlobals gUtilGlobals __attribute__((aligned(32)));
tUtilGlobals UTIL_DMEM_SH2(gUtilGlobals) __attribute__((aligned(32)));

struct tQM_context_ctl *gQMpCtx[GEM_PORTS] __attribute__((aligned(32)));;
struct tQM_context_ctl gQMCtx[GEM_PORTS] __attribute__((aligned(32)));
struct tQM_context_ctl gQMQosOffCtx[GEM_PORTS] __attribute__((aligned(32)));
struct tQM_context_ctl gQMExptCtx __attribute__((aligned(32)));

struct tQM_context TMU_DMEM_SH(g_qm_context) __attribute__((aligned(32)));
//struct tQM_stat TMU_DMEM_SH(g_qm_stat)[4] __attribute__((aligned(32)));
//U8 TMU_DMEM_SH(g_qm_wt)[QM_WT_CMD_SIZE] __attribute__((aligned(32)));
U8 TMU_DMEM_SH(g_qm_cmd_info)[QM_CMD_SIZE] __attribute__((aligned(32)));

#ifdef FPP_STATISTICS
int gStatBridgeQueryStatus;
int gStatPPPoEQueryStatus;
int gStatIpsecQueryStatus;
int gStatVlanQueryStatus;
#endif

U8 gDTMF_PT[2];
struct slist_head rtpflow_cache[NUM_RTPFLOW_ENTRIES] __attribute__((aligned(32)));
struct slist_head rtpcall_list;



