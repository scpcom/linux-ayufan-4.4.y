/******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

*******************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
******************************************************************************/

#ifndef __INC_xcat98dx_core_h
#define __INC_xcat98dx_core_h

#include <mach/kirkwood.h>
#include "mvTypes.h"
#ifdef CONFIG_MV_XCAT98DX_WARM_RESET
#include "mv_switch/mv_switch.h"
#endif

#define MV_PP_DEV0	(0)
#define MV_PP_DEV1	(1)
#define MV_PP_BASE	(KIRKWOOD_PP_VIRT_BASE)

void mvPpAddrComp(MV_U32 baseAddr, MV_U32 reg, MV_U32 *addr);
void mvPpWriteReg(MV_U8 dev, MV_U32 reg, MV_U32 val);
MV_U32 mvPpReadReg(MV_U8 dev, MV_U32 reg);
void mvPpReadModWriteReg(MV_U8 dev, MV_U32 reg, MV_U32 mask, MV_U32 val);

void mvPpBitSet  (MV_U8 dev, MV_U32 reg, MV_U32 bitMask);
void mvPpBitReset(MV_U8 dev, MV_U32 reg, MV_U32 bitMask);

void mvPpCpuPortConfig(MV_U8 dev);
void mvPpCpuPortCascadeModeSet(MV_U32 dev);
void mvPpCpuPortTwoBytePrependDisable(void);
void mvPpConfigNetworkPorts(void);

MV_U32 mvPpMruGet(MV_U32 dev, MV_U32 port);
MV_STATUS mvPpMruSetAllPorts(MV_U32 mruBytes);
void mvPpReadPortMibCntCpuPort(MV_U32 dev);
void mvPresteraReadPortMibCounters(int port);

#ifdef CONFIG_MV_XCAT98DX_WARM_RESET
void mvPpWarmRestart();
#endif

/*
 * VLAN Table handeling functions.
 */

#define PRESTERA_VLAN_DIRECT

#if defined PRESTERA_VLAN_INDIRECT
    #define VLT_CTRL_REG_OFFSET	0xA000118
    #define VLT_DATA_REG_OFFSET	0xA000000
    #define VLAN_ENTRY_SIZE		0x10
#elif defined PRESTERA_VLAN_DIRECT
    #define VLT_CTRL_REG_OFFSET	0xA000118 /*not in use in direct access*/
    #define VLT_DATA_REG_OFFSET	0xA200000
    #define VLAN_ENTRY_SIZE		0x20
#else
    #error "Prestera Hal: VLAN table access isn't defined!\n"
#endif

#define VLT_ENTRY_BIT               0
#define VLT_TRIGGER_BIT             15
#define VLT_OPERATION_BIT           12

#define CPU_VLAN_MEMBER_BIT         2
#define IPv4_CTRL_TO_CPU_ENABLE     18
#define UNREGISTERED_IPv4_BC        18
#define UNREGISTERED_NON_IPv4_BC    21
#define PORT24_VLAN_MEMBER_BIT      72
#define PORT25_VLAN_MEMBER_BIT      74
#define PORT26_VLAN_MEMBER_BIT      76
#define PORT27_VLAN_MEMBER_BIT      104
#define UNKNOWN_UNICATS_CMD_BIT     12

typedef struct _mvPpVlanEntry
{
    volatile MV_U32 VLTData[4];
} STRUCT_VLAN_ENTRY;

#define CONFIG_ETH_MULTI_Q

#ifdef CONFIG_ETH_MULTI_Q
    #define MV_NET_NUM_OF_RX_Q                (8)
    #define MV_NET_NUM_OF_TX_Q                (8)
    #define MV_NET_NUM_OF_RX_DESC_PER_Q       (128)
    #define MV_NET_NUM_OF_TX_DESC_PER_Q       (128)
#else
    #define MV_NET_NUM_OF_RX_Q                (1)
    #define MV_NET_NUM_OF_TX_Q                (1)
    #define MV_NET_NUM_OF_RX_DESC_PER_Q       (8)
    #define MV_NET_NUM_OF_TX_DESC_PER_Q       (8)
#endif

#define MV_NET_NUM_OF_RX_DESC_TOTAL \
	(MV_NET_NUM_OF_RX_Q*MV_NET_NUM_OF_RX_DESC_PER_Q)
#define MV_NET_NUM_OF_TX_DESC_TOTAL \
	(MV_NET_NUM_OF_TX_Q*MV_NET_NUM_OF_TX_DESC_PER_Q)

#define NUM_OF_RX_QUEUES                  MV_NET_NUM_OF_RX_Q
#define NUM_OF_TX_QUEUES                  MV_NET_NUM_OF_TX_Q

#define PRESTERA_RXQ_LEN                  MV_NET_NUM_OF_RX_DESC_PER_Q
#define PRESTERA_TXQ_LEN                  MV_NET_NUM_OF_TX_DESC_PER_Q

#define PP_NUM_OF_RX_DESC_PER_Q           (PRESTERA_RXQ_LEN)
#define PP_NUM_OF_RX_DESC_TOTAL           (PRESTERA_RXQ_LEN * NUM_OF_RX_QUEUES)
#define PP_NUM_OF_TX_DESC_PER_Q           (PRESTERA_TXQ_LEN)
#define PP_NUM_OF_TX_DESC_TOTAL           (PRESTERA_TXQ_LEN * NUM_OF_TX_QUEUES)

MV_STATUS setCpuAsVLANMember(int dev, int vlan);

#endif /* __INC_xcat98dx_core_h */
