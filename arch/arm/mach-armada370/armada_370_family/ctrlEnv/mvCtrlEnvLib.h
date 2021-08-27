/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __INCmvCtrlEnvLibh
#define __INCmvCtrlEnvLibh

/* includes */
#include "mvSysHwConfig.h"
#include "mvCommon.h"
#include "mvTypes.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"
#include "ctrlEnv/mvCtrlEnvAddrDec.h"

/* 0 for Auto scan mode, 1 for manual. */
#define MV_INTERNAL_SWITCH_SMI_SCAN_MODE	0

#define MV_SPI_LOW_MPPS		2
#define MV_SPI_HIGH_MPPS	3

#define MV_NAND_NOR_BOOT_8BIT	2
#define MV_NAND_NOR_BOOT_16BIT	3

#define SRDS_MOD_NUM	9
#define SRDS_MOD_SHIFT	4
#define SRDS_AUTO_CFG	0

/* typedefs */
typedef MV_STATUS (*MV_WIN_GET_FUNC_PTR)(MV_U32, MV_U32, MV_UNIT_WIN_INFO*);

/* This enumerator describes the possible HW cache coherency policies the   */
/* controllers supports.                                                    */
typedef enum _mvCachePolicy {
	NO_COHERENCY,   /* No HW cache coherency support                        */
	WT_COHERENCY,   /* HW cache coherency supported in Write Through policy */
	WB_COHERENCY    /* HW cache coherency supported in Write Back policy    */
} MV_CACHE_POLICY;

/* The swapping is referred to a 64-bit words (as this is the controller    */
/* internal data path width). This enumerator describes the possible        */
/* data swap types. Below is an example of the data 0x0011223344556677      */
typedef enum _mvSwapType {
	MV_BYTE_SWAP,       /* Byte Swap                77 66 55 44 33 22 11 00 */
	MV_NO_SWAP,         /* No swapping              00 11 22 33 44 55 66 77 */
	MV_BYTE_WORD_SWAP,  /* Both byte and word swap  33 22 11 00 77 66 55 44 */
	MV_WORD_SWAP,       /* Word swap                44 55 66 77 00 11 22 33 */
	SWAP_TYPE_MAX	/* Delimiter for this enumerator                    */
} MV_SWAP_TYPE;

typedef enum {
	LANE0 = 0x1,
	LANE1 = 0x2,
	LANE2 = 0x4,
	LANE3 = 0x8
} MV_SERDES_LANES;

typedef enum {
	SRDS_MOD_AUTO = 0x0,
	SRDS_MOD_PCIE0_LANE0 = 0x001,
	SRDS_MOD_PCIE1_LANE1 = 0x002,
	SRDS_MOD_SATA0_LANE0 = 0x004,
	SRDS_MOD_SATA0_LANE2 = 0x008,
	SRDS_MOD_SATA1_LANE3 = 0x010,
	SRDS_MOD_SGMII0_LANE1 = 0x020,
	SRDS_MOD_SGMII0_LANE2 = 0x040,
	SRDS_MOD_SGMII1_LANE0 = 0x080,
	SRDS_MOD_SGMII1_LANE3 = 0x100,
	SRDS_MOD_ALL = 0x1FF,
	SRDS_MOD_ILLEGAL = 0xFFFFFFFF
} MV_SERDES_MODE;

typedef enum {
	SRDS_SPEED_AUTO = 0x0,
	SRDS_SPEED_PCIE0_LANE0 = 0x001,
	SRDS_SPEED_PCIE1_LANE1 = 0x002,
	SRDS_SPEED_SATA0_LANE0 = 0x004,
	SRDS_SPEED_SATA0_LANE2 = 0x008,
	SRDS_SPEED_SATA1_LANE3 = 0x010,
	SRDS_SPEED_SGMII0_LANE1 = 0x020,
	SRDS_SPEED_SGMII0_LANE2 = 0x040,
	SRDS_SPEED_SGMII1_LANE0 = 0x080,
	SRDS_SPEED_SGMII1_LANE3 = 0x100,
	SRDS_SPEED_ALL = 0x1FF,
	SRDS_SPEED_ILLEGAL = 0xFFFFFFFF
} MV_SERDES_LANE_SPEED;

typedef enum {
	SRDS_SPEED_LOW		= 0,
	SRDS_SPEED_HIGH		= 1
} MV_SERDES_SPEED;

typedef enum {
	SERDES_UNIT_UNCONNECTED	= 0x0,
	SERDES_UNIT_PEX		= 0x1,
	SERDES_UNIT_SATA	= 0x2,
	SERDES_UNIT_SGMII	= 0x3,
	SERDES_UNIT_LAST
} MV_SERDES_UNIT_INDX;

typedef enum {
	PEX_BUS_DISABLED	= 0,
	PEX_BUS_MODE_X1		= 1
} MV_PEX_UNIT_CFG;

typedef struct {
	MV_U32 serdesMode;		/* Bitmap of MV_SERDES_MODE - one bit per SERDES line	*/
	MV_U32 serdesSpeed;		/* Bitmap of SRDS_LANE_SPEED - one bit per SERDES line 		*/
} MV_SERDES_CFG;

typedef struct {
	MV_U32 serdesMode;
	/* register setup for MV_SERDES_MODE - one nibble per SERDES line (serdes register format)	*/
	MV_U32 serdesSpeed;		/* Bitmap of SRDS_LANE_SPEED - one bit per SERDES line 		*/
} MV_SERDES_REG_CFG;

MV_U32 mvCtrlGetCpuNum(MV_VOID);
MV_BOOL mvCtrlIsValidSatR(MV_VOID);
void mvCtrlSetSkipSerdesPhyConfig(MV_VOID);
MV_STATUS mvCtrlEnvInit(MV_VOID);
MV_U32 mvCtrlMppRegGet(MV_U32 mppGroup);
#if defined(MV_INCLUDE_PEX)
MV_U32 mvCtrlPexMaxIfGet(MV_VOID);
MV_U32 mvCtrlPexMaxUnitGet(MV_VOID);
#endif
#if defined(MV_INCLUDE_PCI)
MV_U32	mvCtrlPciMaxIfGet(MV_VOID);
#else
#define mvCtrlPciIfMaxIfGet()	(mvCtrlPexMaxIfGet())
#endif
MV_U32 mvCtrlEthMaxPortGet(MV_VOID);
MV_U8 mvCtrlEthMaxCPUsGet(MV_VOID);
#if defined(MV_INCLUDE_SATA)
MV_U32 mvCtrlSataMaxPortGet(MV_VOID);
#endif
#if defined(MV_INCLUDE_XOR)
MV_U32 mvCtrlXorMaxChanGet(MV_VOID);
MV_U32 mvCtrlXorMaxUnitGet(MV_VOID);
#endif
#if defined(MV_INCLUDE_USB)
MV_U32 mvCtrlUsbMaxGet(void);
#endif
#if defined(MV_INCLUDE_LEGACY_NAND)
MV_U32 mvCtrlNandSupport(MV_VOID);
#endif
#if defined(MV_INCLUDE_SDIO)
MV_U32 mvCtrlSdioSupport(MV_VOID);
#endif
MV_U32 mvCtrlTdmSupport(MV_VOID);
#if defined(MV_INCLUDE_TDM)
MV_U32 mvCtrlTdmMaxGet(MV_VOID);
MV_UNIT_ID mvCtrlTdmUnitTypeGet(MV_VOID);
MV_U32 mvCtrlTdmUnitIrqGet(MV_VOID);
#endif /* if defined(MV_INCLUDE_TDM) */
MV_U32 mvCtrlDevFamilyIdGet(MV_U16 ctrlModel);
MV_U16 mvCtrlModelGet(MV_VOID);
MV_U8 mvCtrlRevGet(MV_VOID);
MV_STATUS mvCtrlNameGet(char *pNameBuff);
MV_U32 mvCtrlModelRevGet(MV_VOID);
MV_STATUS mvCtrlModelRevNameGet(char *pNameBuff);
const MV_8 *mvCtrlTargetNameGet(MV_TARGET target);
MV_VOID mvCtrlAddrDecShow(MV_VOID);
MV_U32 ctrlSizeToReg(MV_U32 size, MV_U32 alignment);
MV_U32 ctrlRegToSize(MV_U32 regSize, MV_U32 alignment);
MV_U32 ctrlSizeRegRoundUp(MV_U32 size, MV_U32 alignment);
MV_U32 mvCtrlIsBootFromNOR(MV_VOID);
MV_U32 mvCtrlIsBootFromSPI(MV_VOID);
MV_U32 mvCtrlIsBootFromNAND(MV_VOID);
MV_U32 mvCtrlAudioSupport(MV_VOID);
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable);
MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index);
MV_VOID mvCtrlPwrMemSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable);
MV_BOOL mvCtrlPwrMemGet(MV_UNIT_ID unitId, MV_U32 index);
#endif /* #if defined(MV_INCLUDE_CLK_PWR_CNTRL) */
MV_U32 mvCtrlSerdesMaxLinesGet(MV_VOID);
MV_STATUS mvCtrlSerdesPhyConfig(MV_VOID);
MV_U32 mvCtrlDDRBudWidth(MV_VOID);
MV_BOOL mvCtrlDDRThruXbar(MV_VOID);

#endif /* __INCmvCtrlEnvLibh */
