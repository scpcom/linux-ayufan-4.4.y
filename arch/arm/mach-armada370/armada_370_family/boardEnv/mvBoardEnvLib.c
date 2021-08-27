#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
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
 
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPex.h"
#include "device/mvDevice.h"
#include "neta/gbe/mvEthRegs.h"
#include "neta/gbe/mvNeta.h"
#include "gpp/mvGppRegs.h"

/* defines  */
#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)	x
#define DB1(x)	x
#else
#define DB(x)
#define DB1(x)
#endif

#define CODE_IN_ROM		MV_FALSE
#define CODE_IN_RAM		MV_TRUE

#define FILL_TWSI_SLAVE(slv, addr)				\
{								\
	slv.slaveAddr.address = addr;				\
	slv.slaveAddr.type = MV_BOARD_MODULES_ADDR_TYPE;	\
	slv.validOffset = MV_TRUE;				\
	slv.offset = 0;						\
	slv.moreThen256 = MV_FALSE;				\
}

extern MV_BOARD_INFO *boardInfoTbl[];
#define BOARD_INFO(boardId)	boardInfoTbl[boardId - BOARD_ID_BASE]

/* Locals */
static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);

MV_U32 tClkRate = -1;

MV_U32 gBoardMppType2Index[] = {MV_BOARD_AUTO, MV_BOARD_TDM, MV_BOARD_I2S, MV_BOARD_GMII0, MV_BOARD_SDIO,
	MV_BOARD_RGMII0, MV_BOARD_RGMII1};

/*******************************************************************************
* mvBoardEnvInit - Init board
*
* DESCRIPTION:
*		In this function the board environment take care of device bank
*		initialization.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvBoardEnvInit(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 norDev;
	MV_U32 i, gppMask;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardEnvInit:Board unknown.\n");
		return;
	}

#if defined(CONFIG_SYNO_ARMADA_ARCH)
#else
/* MPP setting only in uboot, so we don't double set again */
	norDev = boardGetDevCSNum(0, BOARD_DEV_NOR_FLASH);
	if (norDev != 0xFFFFFFFF) {
		/* Set NOR interface access parameters */
		MV_REG_WRITE(DEV_BANK_PARAM_REG(norDev), BOARD_INFO(boardId)->norFlashReadParams);
		MV_REG_WRITE(DEV_BANK_PARAM_REG_WR(norDev), BOARD_INFO(boardId)->norFlashWriteParams);
		MV_REG_WRITE(DEV_BUS_SYNC_CTRL, 0x11);
	}

	/* Device Bus or NAND Controller selection */
#ifdef MV_INCLUDE_NOR
	MV_REG_BIT_RESET(SOC_DEVICE_MUX_REG, BIT0);
#else
	MV_REG_BIT_SET(SOC_DEVICE_MUX_REG, BIT0);
#endif

	MV_REG_WRITE(MV_RUNIT_PMU_REGS_OFFSET + 0x4, BOARD_INFO(boardId)->pmuPwrUpPolarity);
	MV_REG_WRITE(MV_RUNIT_PMU_REGS_OFFSET + 0x14, BOARD_INFO(boardId)->pmuPwrUpDelay);

	/* Set GPP Out value */
	MV_REG_WRITE(GPP_DATA_OUT_REG(0), BOARD_INFO(boardId)->gppOutValLow);
	MV_REG_WRITE(GPP_DATA_OUT_REG(1), BOARD_INFO(boardId)->gppOutValMid);
	MV_REG_WRITE(GPP_DATA_OUT_REG(2), BOARD_INFO(boardId)->gppOutValHigh);

	/* set GPP polarity */
	mvGppPolaritySet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValLow);
	mvGppPolaritySet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValMid);
	mvGppPolaritySet(2, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValHigh);

	/* Set GPP Out Enable */
	mvGppTypeSet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValLow);
	mvGppTypeSet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValMid);
	mvGppTypeSet(2, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValHigh);

	/* Set GPIO interrupts type & polarity as needed */
	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		gppMask = mvBoardGpioIntMaskGet(i);
		mvGppTypeSet(i, gppMask , (MV_GPP_IN & gppMask));
		mvGppPolaritySet(i, gppMask , (MV_GPP_IN_INVERT & gppMask));
	}
#endif
}

/*******************************************************************************
* mvBoardModelGet - Get Board model
*
* DESCRIPTION:
*       This function returns 16bit describing board model.
*       Board model is constructed of one byte major and minor numbers in the
*       following manner:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       String describing board model.
*
*******************************************************************************/
MV_U16 mvBoardModelGet(MV_VOID)
{
	return (mvBoardIdGet() >> 16);
}

/*******************************************************************************
* mbBoardRevlGet - Get Board revision
*
* DESCRIPTION:
*       This function returns a 32bit describing the board revision.
*       Board revision is constructed of 4bytes. 2bytes describes major number
*       and the other 2bytes describes minor munber.
*       For example for board revision 3.4 the function will return
*       0x00030004.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       String describing board model.
*
*******************************************************************************/
MV_U16 mvBoardRevGet(MV_VOID)
{
	return (mvBoardIdGet() & 0xFFFF);
}

/*******************************************************************************
* mvBoardNameGet - Get Board name
*
* DESCRIPTION:
*       This function returns a string describing the board model and revision.
*       String is extracted from board I2C EEPROM.
*
* INPUT:
*       None.
*
* OUTPUT:
*       pNameBuff - Buffer to contain board name string. Minimum size 32 chars.
*
* RETURN:
*
*       MV_ERROR if informantion can not be read.
*******************************************************************************/
MV_STATUS mvBoardNameGet(char *pNameBuff)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsSPrintf(pNameBuff, "Board unknown.\n");
		return MV_ERROR;
	}

	mvOsSPrintf(pNameBuff, "%s", BOARD_INFO(boardId)->boardName);

	return MV_OK;
}

/*******************************************************************************
* mvBoardIsPortInSgmii -
*
* DESCRIPTION:
*       This routine returns MV_TRUE for port number works in SGMII or MV_FALSE
*	For all other options.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - port in SGMII.
*       MV_FALSE - other.
*
*******************************************************************************/
MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
	MV_U32 serdesMode = mvBoardSerdesModeGet();

	if (ethPortNum == 0) {
		if (serdesMode & (SRDS_MOD_SGMII0_LANE1 | SRDS_MOD_SGMII0_LANE2))
			return MV_TRUE;
	}

	if (ethPortNum == 1) {
		if (serdesMode & (SRDS_MOD_SGMII1_LANE0 | SRDS_MOD_SGMII1_LANE3))
			return MV_TRUE;
	}

	return MV_FALSE;
}

/*******************************************************************************
* mvBoardIsPortInGmii -
*
* DESCRIPTION:
*       This routine returns MV_TRUE for port number works in GMII or MV_FALSE
*	For all other options.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - port in GMII.
*       MV_FALSE - other.
*
*******************************************************************************/
MV_BOOL mvBoardIsPortInGmii(MV_U32 ethPortNum)
{
	if (mvBoardIsGMIIConnected() && (ethPortNum ==0))
		return MV_TRUE;
	else
		return MV_FALSE;
}

/*******************************************************************************
* mvBoardIsPortInRgmii -
*
* DESCRIPTION:
*       This routine returns MV_TRUE for port number works in RGMII or MV_FALSE
*	for all other options.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - port in RGMII.
*       MV_FALSE - other.
*
*******************************************************************************/
MV_BOOL mvBoardIsPortInRgmii(MV_U32 ethPortNum)
{

	if (ethPortNum == 0) {
		if (mvBoardMppModulesCfgGet(1) & MV_BOARD_RGMII0)
			return MV_TRUE;
	}

	if (ethPortNum == 1) {
		if (mvBoardMppModulesCfgGet(1) & MV_BOARD_RGMII1)
			return MV_TRUE;
	}

	return MV_FALSE;
}

/*******************************************************************************
* mvBoardSwitchPortGet - Get the mapping between the board connector and the
*                        Ethernet Switch port
*
* DESCRIPTION:
*       This routine returns the matching Switch port.
*
* INPUT:
*	    boardPortNum - logical number of the connector on the board
*
* OUTPUT:
*       None.
*
* RETURN:
*       the matching Switch port, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32 mvBoardSwitchPortGet(MV_U32 switchIdx, MV_U32 boardPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchPortGet: Board unknown.\n");
		return -1;
	}
	if (boardPortNum >= BOARD_ETH_SWITCH_PORT_NUM) {
		mvOsPrintf("mvBoardSwitchPortGet: Illegal board port number.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].switchPort[boardPortNum];
}

/*******************************************************************************
* mvBoardSwitchConnectedPortGet -
*
* DESCRIPTION:
*       This routine returns the switch port connected to the ethPort
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_32 mvBoardSwitchConnectedPortGet(MV_U32 ethPort)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchConnectedPortGet: Board unknown.\n");
		return -1;
	}
	if (BOARD_INFO(boardId)->switchInfoNum == 0)
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[0].connectedPort[ethPort];
}

/*******************************************************************************
* mvBoardSwitchCpuPortGet - Get the the Ethernet Switch CPU port
*
* DESCRIPTION:
*       This routine returns the Switch CPU port.
*
* INPUT:
*       switchIdx - index of the switch. Only 0 is supported.
*
* OUTPUT:
*       None.
*
* RETURN:
*       the Switch CPU port, -1 if the switch is not connected.
*
*******************************************************************************/
MV_32 mvBoardSwitchCpuPortGet(MV_U32 switchIdx)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchCpuPortGet: Board unknown.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].cpuPort;
}

/*******************************************************************************
* mvBoardSwitchIrqGet - Get the IRQ number for the link status indication
*
* DESCRIPTION:
*       This routine returns the IRQ number for the link status indication.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*	the number of the IRQ for the link status indication, -1 if the port
*	number is wrong or if not relevant.
*
*******************************************************************************/
MV_32 mvBoardSwitchIrqGet(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardLinkStatusIrqGet: Board unknown.\n");
		return -1;
	}
	if (BOARD_INFO(boardId)->switchInfoNum == 0)
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[0].switchIrq;
}

/*******************************************************************************
* mvBoardIsQsgmiiModuleConnected
*
* DESCRIPTION:
*       This routine returns whether the QSGMII module is connected or not.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if QSGMII module is connected, MV_FALSE otherwise.
*
*******************************************************************************/
MV_BOOL mvBoardIsQsgmiiModuleConnected(MV_VOID)
{
	return MV_FALSE;
}

/*******************************************************************************
* mvBoardGePhySwitchPortGet
*
* DESCRIPTION:
*       This routine returns whether the internal GE PHY is connected to
*	Switch Port 0, Switch port 5 or not connected to any Switch port.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       0 if the internal GE PHY is connected to Switch Port 0,
*	5 if the internal GE PHY is connected to Switch Port 5,
*	-1 otherwise.
*
*******************************************************************************/
MV_32 mvBoardGePhySwitchPortGet(MV_VOID)
{
	return -1;
}

/*******************************************************************************
* mvBoardRgmiiASwitchPortGet
*
* DESCRIPTION:
*       This routine returns whether RGMII-A is connected to
*	Switch Port 5, Switch port 6 or not connected to any Switch port.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       5 if the internal GE PHY is connected to Switch Port 5,
*	6 if the internal GE PHY is connected to Switch Port 6,
*	-1 otherwise.
*
*******************************************************************************/
MV_32 mvBoardRgmiiASwitchPortGet(MV_VOID)
{
	return -1;
}

/*******************************************************************************
* mvBoardSwitchPortMap
*
* DESCRIPTION:
*	Map front panel connector number to switch port number.
*
* INPUT:
*	switchIdx - The switch index.
*	switchPortNum - The switch port number to get the mapping for.
*
* OUTPUT:
*       None.
*
* RETURN:
*	The switch port mapping.
*	OR -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32 mvBoardSwitchPortMap(MV_U32 switchIdx, MV_U32 switchPortNum)
{
	int i;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchPortMap: Board unknown.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	for (i = 0; i < BOARD_ETH_SWITCH_PORT_NUM; i++) {
		if (BOARD_INFO(boardId)->pSwitchInfo[switchIdx].switchPort[i] == switchPortNum)
			return i;
	}
	return -1;
}

/*******************************************************************************
* mvBoardPhyAddrGet - Get the phy address
*
* DESCRIPTION:
*       This routine returns the Phy address of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing Phy address, -1 if the port number is wrong.
*
*******************************************************************************/
MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardPhyAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
}
/*******************************************************************************
* mvBoardQuadPhyAddr0Get - Get the phy address
*
* DESCRIPTION:
*       This routine returns the Phy address of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing Phy address, -1 if the port number is wrong.
*
*******************************************************************************/
MV_32 mvBoardQuadPhyAddr0Get(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardQuadPhyAddr0Get: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr0;
}

/*******************************************************************************
* mvBoardPhyLinkCryptPortAddrGet - Get the phy gbe address
*
* DESCRIPTION:
*       This routine returns the Phy address of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing Phy address, -1 if the port number is wrong.
*
*******************************************************************************/
MV_32 mvBoardPhyLinkCryptPortAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardPhyLinkCryptPortAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].LinkCryptPortAddr;
}

/*******************************************************************************
* mvBoardMacSpeedGet - Get the Mac speed
*
* DESCRIPTION:
*       This routine returns the Mac speed if pre define of a given ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BOARD_MAC_SPEED, -1 if the port number is wrong.
*
*******************************************************************************/
MV_BOARD_MAC_SPEED mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMacSpeedGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo) {
		mvOsPrintf("mvBoardMacSpeedGet: illegal port number\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

/*******************************************************************************
* mvBoardSpecInitGet -
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN: Return MV_TRUE and parameters in case board need spesific phy init,
*	  otherwise return MV_FALSE.
*
*
*******************************************************************************/
MV_BOOL mvBoardSpecInitGet(MV_U32 *regOff, MV_U32 *data)
{
	return MV_FALSE;
}

/*******************************************************************************
* mvBoardTclkGet - Get the board Tclk (Controller clock)
*
* DESCRIPTION:
*       This routine extract the controller core clock.
*       This function uses the controller counters to make identification.
*		Note: In order to avoid interference, make sure task context switch
*		and interrupts will not occure during this function operation
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
*******************************************************************************/
MV_U32 mvBoardTclkGet(MV_VOID)
{
#ifdef TCLK_AUTO_DETECT
	if ((MV_REG_READ(MPP_SAMPLE_AT_RESET) & MSAR_TCLK_MASK) != 0)
		return MV_BOARD_TCLK_200MHZ;
	else
		return MV_BOARD_TCLK_166MHZ;
#else
	return MV_BOARD_TCLK_200MHZ;
#endif
}

/*******************************************************************************
* mvBoardSysClkGet - Get the board SysClk (CPU bus clock , i.e. DDR clock)
*
* DESCRIPTION:
*       This routine extract the CPU bus clock.
*
* INPUT:
*       countNum - Counter number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit clock cycles in Hertz.
*
*******************************************************************************/
MV_U32 mvBoardSysClkGet(MV_VOID)
{
#ifdef SYSCLK_AUTO_DETECT
	MV_U32 idx;
	MV_U32 cpuFreqMhz, ddrFreqMhz;
	MV_CPU_ARM_CLK_RATIO clockRatioTbl[] = MV_DDR_L2_CLK_RATIO_TBL;

	idx = MSAR_DDR_L2_CLK_RATIO_IDX(MV_REG_READ(MPP_SAMPLE_AT_RESET));

	if (clockRatioTbl[idx].vco2cpu != 0) {	/* valid ratio ? */
		cpuFreqMhz = mvCpuPclkGet() / 1000000;	/* obtain CPU freq */
		cpuFreqMhz *= clockRatioTbl[idx].vco2cpu;	/* compute VCO freq */
		ddrFreqMhz = cpuFreqMhz / clockRatioTbl[idx].vco2ddr;
		/* round up to integer MHz */
		if (((cpuFreqMhz % clockRatioTbl[idx].vco2ddr) * 10 / clockRatioTbl[idx].vco2ddr) >= 5)
			ddrFreqMhz++;

		return ddrFreqMhz * 1000000;
	} else
		return 0;
#else
	return MV_BOARD_DEFAULT_SYSCLK;
#endif
}

/*******************************************************************************
* mvBoardDebugLedNumGet - Get number of debug Leds
*
* DESCRIPTION:
* INPUT:
*       boardId
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId)
{
	return BOARD_INFO(boardId)->activeLedsNumber;
}

/*******************************************************************************
* mvBoardDebugLeg - Set the board debug Leds
*
* DESCRIPTION: turn on/off status leds.
* 	       Note: assume MPP leds are part of group 0 only.
*
* INPUT:
*       hexNum - Number to be displayed in hex by Leds.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvBoardDebugLed(MV_U32 hexNum)
{
	MV_U32 val[MV_GPP_MAX_GROUP] = {0};
	MV_U32 mask[MV_GPP_MAX_GROUP] = {0};
	MV_U32 digitMask;
	MV_U32 i, pinNum, gppGroup;
	MV_U32 boardId = mvBoardIdGet();

	if (BOARD_INFO(boardId)->pLedGppPin == NULL)
		return;

	hexNum &= (1 << BOARD_INFO(boardId)->activeLedsNumber) - 1;

	for (i = 0, digitMask = 1; i < BOARD_INFO(boardId)->activeLedsNumber; i++, digitMask <<= 1) {
			pinNum = BOARD_INFO(boardId)->pLedGppPin[i];
			gppGroup = pinNum / 32;
			if (hexNum & digitMask)
				val[gppGroup]  |= (1 << (pinNum - gppGroup * 32));
			mask[gppGroup] |= (1 << (pinNum - gppGroup * 32));
	}

	for (gppGroup = 0; gppGroup < MV_GPP_MAX_GROUP; gppGroup++) {
		/* If at least one bit is set in the mask, update the whole GPP group */
		if (mask[gppGroup])
			mvGppValueSet(gppGroup, mask[gppGroup], BOARD_INFO(boardId)->ledsPolarity == 0 ?
					val[gppGroup] : ~val[gppGroup]);
	}
}

/*******************************************************************************
* mvBoarGpioPinGet - mvBoarGpioPinGet
*
* DESCRIPTION:
*
* INPUT:
*		gppClass - MV_BOARD_GPP_CLASS enum.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS gppClass, MV_U32 index)
{
	MV_U32 boardId, i;
	MV_U32 indexFound = 0;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardRTCGpioPinGet:Board unknown.\n");
		return MV_ERROR;
	}

	for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++) {
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == gppClass) {
			if (indexFound == index)
				return (MV_U32) BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			else
				indexFound++;
		}
	}
	return MV_ERROR;
}

/*******************************************************************************
* mvBoardReset - mvBoardReset
*
* DESCRIPTION:
*			Reset the board
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*       None
*
*******************************************************************************/
MV_VOID mvBoardReset(MV_VOID)
{
	MV_32 resetPin;

	/* Get gpp reset pin if define */
	resetPin = mvBoardResetGpioPinGet();
	if (resetPin != MV_ERROR) {
		MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), (1 << resetPin));
		MV_REG_BIT_RESET(GPP_DATA_OUT_EN_REG(0), (1 << resetPin));
	} else {
		/* No gpp reset pin was found, try to reset ussing
		 ** system reset out */
		MV_REG_BIT_SET(CPU_RSTOUTN_MASK_REG, BIT0);
		MV_REG_BIT_SET(CPU_SYS_SOFT_RST_REG, BIT0);
	}
}

/*******************************************************************************
* mvBoardResetGpioPinGet - mvBoardResetGpioPinGet
*
* DESCRIPTION:
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardResetGpioPinGet(MV_VOID)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_RESET, 0);
}

/*******************************************************************************
* mvBoardSDIOGpioPinGet - mvBoardSDIOGpioPinGet
*
* DESCRIPTION:
*	used for hotswap detection
* INPUT:
*	type - Type of SDIO GPP to get.
*
* OUTPUT:
*	None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardSDIOGpioPinGet(MV_BOARD_GPP_CLASS type)
{
	if ((type != BOARD_GPP_SDIO_POWER) && (type != BOARD_GPP_SDIO_DETECT) && (type != BOARD_GPP_SDIO_WP))
		return MV_FAIL;

	return mvBoarGpioPinNumGet(type, 0);
}

/*******************************************************************************
* mvBoardUSBVbusGpioPinGet - return Vbus input GPP
*
* DESCRIPTION:
*
* INPUT:
*		int  devNo.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardUSBVbusGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS, devId);
}

/*******************************************************************************
* mvBoardUSBVbusEnGpioPinGet - return Vbus Enable output GPP
*
* DESCRIPTION:
*
* INPUT:
*		int  devNo.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoardUSBVbusEnGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS_EN, devId);
}

/*******************************************************************************
* mvBoardGpioIntMaskGet - Get GPIO mask for interrupt pins
*
* DESCRIPTION:
*		This function returns a 32-bit mask of GPP pins that connected to
*		interrupt generating sources on board.
*		For example if UART channel A is hardwired to GPP pin 8 and
*		UART channel B is hardwired to GPP pin 4 the fuinction will return
*		the value 0x000000110
*
* INPUT:
*		None.
*
* OUTPUT:
*		None.
*
* RETURN:
*		See description. The function return -1 if board is not identified.
*
*******************************************************************************/
MV_U32 mvBoardGpioIntMaskGet(MV_U32 gppGrp)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGpioIntMaskGet:Board unknown.\n");
		return MV_ERROR;
	}

	switch (gppGrp) {
	case (0):
		return BOARD_INFO(boardId)->intsGppMaskLow;
		break;
	case (1):
		return BOARD_INFO(boardId)->intsGppMaskMid;
		break;
	case (2):
		return BOARD_INFO(boardId)->intsGppMaskHigh;
		break;
	default:
		return MV_ERROR;
	}
}

/*******************************************************************************
* mvBoardMppGet - Get board dependent MPP register value
*
* DESCRIPTION:
*	MPP settings are derived from board design.
*	MPP group consist of 8 MPPs. An MPP group represents MPP
*	control register.
*       This function retrieves board dependend MPP register value.
*
* INPUT:
*       mppGroupNum - MPP group number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit value describing MPP control register value.
*
*******************************************************************************/
MV_32 mvBoardMppGet(MV_U32 mppGroupNum)
{
	MV_U32 boardId;
	MV_U32 mppMod;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;
	}

	if (mppGroupNum >= BOARD_INFO(boardId)->numBoardMppConfigValue)
		mppMod = 0; /* default */

	return BOARD_INFO(boardId)->pBoardMppConfigValue[mppMod].mppGroup[mppGroupNum];
}

/*******************************************************************************
* mvBoardGppConfigGet
*
* DESCRIPTION:
*	Get board configuration according to the input configuration GPP's.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*	The value of the board configuration GPP's.
*
*******************************************************************************/
MV_U32 mvBoardGppConfigGet(void)
{
	MV_U32 boardId, i;
	MV_U32 result = 0;
	MV_U32 gpp;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGppConfigGet: Board unknown.\n");
		return 0;
	}

	for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++) {
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == BOARD_GPP_CONF) {
			gpp = BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			result <<= 1;
			result |= (mvGppValueGet(gpp >> 5, 1 << (gpp & 0x1F)) >> (gpp & 0x1F));
		}
	}
	return result;

}

/*******************************************************************************
* mvBoardTdmSpiModeGet - return SLIC/DAA connection
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_32 mvBoardTdmSpiModeGet(MV_VOID)
{
	return DUAL_CHIP_SELECT_MODE;
}

/*******************************************************************************
* mvBoardTdmDevicesCountGet
*
* DESCRIPTION:
*	Return the number of TDM devices on board.
*
* INPUT:
*	None.
*
* OUTPUT:
*       None.
*
* RETURN:
*	Number of devices.
*
*******************************************************************************/
MV_U8 mvBoardTdmDevicesCountGet(void)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_16 index;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmDevicesCountGet: Board unknown.\n");
		return 0;
	}

	index = BOARD_INFO(boardId)->boardTdmInfoIndex;
	if (index == (MV_8)-1)
		return 0;

	return BOARD_INFO(boardId)->numBoardTdmInfo[(MV_U8)index];
}

/*******************************************************************************
* mvBoardTdmSpiCsGet
*
* DESCRIPTION:
*	Return the SPI Chip-select number for a given device.
*
* INPUT:
*	devId	- The Slic device ID to get the SPI CS for.
*
* OUTPUT:
*       None.
*
* RETURN:
*	The SPI CS if found, -1 otherwise.
*
*******************************************************************************/
MV_U8 mvBoardTdmSpiCsGet(MV_U8 devId)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_16 index;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmDevicesCountGet: Board unknown.\n");
		return -1;
	}

	index = BOARD_INFO(boardId)->boardTdmInfoIndex;
	if (index == (MV_8)-1)
		return 0;

	if (devId >= BOARD_INFO(boardId)->numBoardTdmInfo[(MV_U8)index])
		return -1;

	return BOARD_INFO(boardId)->pBoardTdmInt2CsInfo[(MV_U8)index][devId].spiCs;
}

/*******************************************************************************
* mvBoardTdmSpiIdGet
*
* DESCRIPTION:
*	Return SPI port ID per board.
*
* INPUT:
*	None
*
* OUTPUT:
*       None.
*
* RETURN:
*	SPI port ID.
*
*******************************************************************************/
MV_U8 mvBoardTdmSpiIdGet(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmSpiIdGet: Board unknown.\n");
		return -1;
	}

	return BOARD_INFO(boardId)->pBoardTdmSpiInfo[0].spiId;
}

/*******************************************************************************
* mvBoardSerdesModeGet
*
* DESCRIPTION:
*	Return the Serdes lanes configuration.
*
* INPUT:
*	None.
*
* OUTPUT:
*       None.
*
* RETURN:
*	A bitmap of the serdes modes.
*
*******************************************************************************/
MV_U32 mvBoardSerdesModeGet(void)
{
	MV_U32 serdesInfo = MV_REG_READ(SERDES_LINE_MUX_REG_0_3);
	MV_U32 serdesMode = 0;

	switch (serdesInfo & 0x0f){
	case 1:
		serdesMode |= SRDS_MOD_PCIE0_LANE0;
		break;
	case 2:
		serdesMode |= SRDS_MOD_SATA0_LANE0;
		break;
	case 3:
		serdesMode |= SRDS_MOD_SGMII1_LANE0;
		break;
	case 0:
	default:
		break;
	}

	switch (serdesInfo & 0x0f0){
	case 0x10:
		serdesMode |= SRDS_MOD_PCIE1_LANE1;
		break;
	case 0x20:
		serdesMode |= SRDS_MOD_SGMII0_LANE1;
		break;
	default:
		break;
	}

	switch (serdesInfo & 0x0f00){
	case 0x100:
		serdesMode |= SRDS_MOD_SATA0_LANE2;
		break;
	case 0x200:
		serdesMode |= SRDS_MOD_SGMII0_LANE2;
		break;
	default:
		break;
	}

	switch (serdesInfo & 0xf000){
	case 0x1000:
		serdesMode |= SRDS_MOD_SATA1_LANE3;
		break;
	case 0x2000:
		serdesMode |= SRDS_MOD_SGMII1_LANE3;
		break;
	default:
		break;
	}

	return serdesMode;
}
/*******************************************************************************
* mvBoardModuleTypePrint
*
* DESCRIPTION:
*	Print on-board detected modules.
*
* INPUT:
*	None.
*
* OUTPUT:
*       None.
*
* RETURN:
*	None.
*
*******************************************************************************/
MV_VOID mvBoardMppModuleTypePrint(MV_VOID)
{
	MV_U32 mppGrp1 = mvBoardMppModulesCfgGet(1);
	MV_U32 mppGrp2 = mvBoardMppModulesCfgGet(2);

	mvOsOutput("Modules/Interfaces Detected:\n");

	/* TDM */
	if (((mppGrp1 & MV_BOARD_TDM) || (mppGrp2 & MV_BOARD_TDM)) && mvCtrlTdmSupport())
		mvOsOutput("       TDM Module\n");

	/* I2S */
	if ((mppGrp1 & MV_BOARD_I2S) || (mppGrp2 & MV_BOARD_I2S))
		mvOsOutput("       I2S Module\n");

	/* GMII0 */
	if (mppGrp1 & MV_BOARD_GMII0)
		mvOsOutput("       GMII0 Module\n");

	/* SDIO */
	if (mppGrp1 & MV_BOARD_SDIO)
		mvOsOutput("       SDIO\n");

	/* RGMII0 */
	if (mppGrp1 & MV_BOARD_RGMII0)
			mvOsOutput("       RGMII0 Phy\n");

	/* RGMII1 */
	if (mppGrp1 & MV_BOARD_RGMII1) {
		if (mvBoardIsSwitchConnected())
			mvOsOutput("       RGMII1 Switch module\n");
		else
			mvOsOutput("       RGMII1 Phy\n");
	}

	return;
}

MV_VOID mvBoardOtherModuleTypePrint(MV_VOID)
{
	MV_U32 srdsCfg = mvBoardSerdesModeGet();

	/* PCI-E */
	if (srdsCfg & SRDS_MOD_PCIE0_LANE0)
		mvOsOutput("       PEX0 (Lane 0)\n");
	if (srdsCfg & SRDS_MOD_PCIE1_LANE1)
		mvOsOutput("       PEX1 (Lane 1)\n");

	/* SATA */
	if (srdsCfg & SRDS_MOD_SATA0_LANE0)
		mvOsOutput("       SATA0 (Lane 0)\n");
	if (srdsCfg & SRDS_MOD_SATA0_LANE2)
		mvOsOutput("       SATA0 (Lane 2)\n");
	if (srdsCfg & SRDS_MOD_SATA1_LANE3 && (mvCtrlSataMaxPortGet() == 2))
		mvOsOutput("       SATA1 (Lane 3)\n");

	/* SGMII */
	if (srdsCfg & SRDS_MOD_SGMII0_LANE1)
		mvOsOutput("       SGMII0 Phy module (Lane 1)\n");
	if (srdsCfg & SRDS_MOD_SGMII0_LANE2)
		mvOsOutput("       SGMII0 Phy module (Lane 2)\n");
	if (srdsCfg & SRDS_MOD_SGMII1_LANE0)
		mvOsOutput("       SGMII1 Phy module (Lane 0)\n");
	if (srdsCfg & SRDS_MOD_SGMII1_LANE3)
		mvOsOutput("       SGMII1 Phy module (Lane 3)\n");

	return;
}

/*******************************************************************************
* mvBoardIsGbEPortConnected
*
* DESCRIPTION:
*	Checks if a given GbE port is actually connected to the GE-PHY, internal Switch or any RGMII module.
*
* INPUT:
*	port - GbE port number (0 or 1).
*
* OUTPUT:
*       None.
*
* RETURN:
*	MV_TRUE if port is connected, MV_FALSE otherwise.
*
*******************************************************************************/
MV_BOOL mvBoardIsGbEPortConnected(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 mppMask;
	MV_U32 srdsMask = mvBoardSerdesModeGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsGbEPortConnected: Board unknown.\n");
		return -1;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
		return MV_FALSE;

	mppMask = BOARD_INFO(boardId)->pBoardModTypeValue->boardMppGrp1Mod;

	if ((ethPortNum == 0) && (((mppMask & (MV_BOARD_RGMII0 | MV_BOARD_GMII0))) ||
			(srdsMask & (SRDS_MOD_SGMII0_LANE1 | SRDS_MOD_SGMII0_LANE2))))
		return MV_TRUE;

	if ((ethPortNum == 1) && ((mppMask & MV_BOARD_RGMII1) ||
			(srdsMask & (SRDS_MOD_SGMII1_LANE0 | SRDS_MOD_SGMII1_LANE3))))
		return MV_TRUE;

	return MV_FALSE;
}

/* Board devices API managments */

/*******************************************************************************
* mvBoardGetDeviceNumber - Get number of device of some type on the board
*
* DESCRIPTION:
*
* INPUT:
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		number of those devices else the function returns 0
*
*
*******************************************************************************/
MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devNum;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGetDeviceNumber:Board unknown.\n");
		return 0xFFFFFFFF;
	}

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++) {
		if (BOARD_INFO(boardId)->pDevCsInfo[devNum].devClass == devClass)
			foundIndex++;
	}

	return foundIndex;
}

/*******************************************************************************
* mvBoardGetDeviceBaseAddr - Get base address of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*	Base address else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return mvCpuIfTargetWinBaseLowGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

/*******************************************************************************
* mvBoardGetDeviceBusWidth - Get Bus width of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		Bus width else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->busWidth;

	return 0xFFFFFFFF;
}

/*******************************************************************************
* mvBoardGetDeviceWidth - Get dev width of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		dev width else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->devWidth;

	return MV_ERROR;
}

/*******************************************************************************
* mvBoardGetDeviceWinSize - Get the window size of a device existing on the board
*
* DESCRIPTION:
*
* INPUT:
*       devIndex - The device sequential number on the board
*		devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*       None.
*
* RETURN:
*       If the device is found on the board the then the functions returns the
*		window size else the function returns 0xffffffff
*
*
*******************************************************************************/
MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return mvCpuIfTargetWinSizeGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

/*******************************************************************************
* boardGetDevEntry - returns the entry pointer of a device on the board
*
* DESCRIPTION:
*
* INPUT:
*	devIndex - The device sequential number on the board
*	devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*	None.
*
* RETURN:
*	If the device is found on the board the then the functions returns the
*	dev number else the function returns 0x0
*
*******************************************************************************/
static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devIndex;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("boardGetDevEntry: Board unknown.\n");
		return NULL;
	}

	for (devIndex = START_DEV_CS; devIndex < BOARD_INFO(boardId)->numBoardDeviceIf; devIndex++) {
		if (BOARD_INFO(boardId)->pDevCsInfo[devIndex].devClass == devClass) {
			if (foundIndex == devNum)
				return &(BOARD_INFO(boardId)->pDevCsInfo[devIndex]);
			foundIndex++;
		}
	}

	/* device not found */
	return NULL;
}

/*******************************************************************************
* boardGetDevCSNum
*
* DESCRIPTION:
*	Return the device's chip-select number.
*
* INPUT:
*	devIndex - The device sequential number on the board
*	devType - The device type ( Flash,RTC , etc .. )
*
* OUTPUT:
*	None.
*
* RETURN:
*	If the device is found on the board the then the functions returns the
*	dev number else the function returns 0x0
*
*******************************************************************************/
MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->deviceCS;

	return 0xFFFFFFFF;
}

/*******************************************************************************
* mvBoardTwsiAddrTypeGet -
*
* DESCRIPTION:
*	Return the TWSI address type for a given twsi device class.
*
* INPUT:
*	twsiClass - The TWSI device to return the address type for.
*	index	  - The TWSI device index (Pass 0 in case of a single
*		    device)
*
* OUTPUT:
*       None.
*
* RETURN:
*	The TWSI address type.
*
*******************************************************************************/
MV_U8 mvBoardTwsiAddrTypeGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId = mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++) {
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}
	}
	return (MV_ERROR);
}

/*******************************************************************************
* mvBoardTwsiAddrGet -
*
* DESCRIPTION:
*	Return the TWSI address for a given twsi device class.
*
* INPUT:
*	twsiClass - The TWSI device to return the address type for.
*	index	  - The TWSI device index (Pass 0 in case of a single
*		    device)
*
* OUTPUT:
*       None.
*
* RETURN:
*	The TWSI address.
*
*******************************************************************************/
MV_U8 mvBoardTwsiAddrGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId = mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++) {
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}
	}
	return (0xFF);
}

/*******************************************************************************
* mvBoardNandWidthGet -
*
* DESCRIPTION: Get the width of the first NAND device in bytes
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN: 1, 2, 4 or MV_ERROR
*
*
*******************************************************************************/
MV_32 mvBoardNandWidthGet(void)
{
	MV_U32 devNum;
	MV_U32 devWidth;
	MV_U32 boardId = mvBoardIdGet();

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++) {
		devWidth = mvBoardGetDeviceWidth(devNum, BOARD_DEV_NAND_FLASH);
		if (devWidth != MV_ERROR)
			return (devWidth / 8);
	}

	/* NAND wasn't found */
	return MV_ERROR;
}

MV_U32 gBoardId = -1;
/*******************************************************************************
* mvBoardIdSet - Set Board model
*
* DESCRIPTION:
*       This function sets the board ID.
*       Board ID is 32bit word constructed of board model (16bit) and
*       board revision (16bit) in the following way: 0xMMMMRRRR.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       void
*
*******************************************************************************/
MV_VOID mvBoardIdSet(MV_VOID)
{
	if (gBoardId == -1) {
#if defined(DB_88F6710)
		gBoardId = DB_88F6710_BP_ID;
#elif defined(DB_88F6710_PCAC)
		gBoardId = DB_88F6710_PCAC_ID;
#elif defined(RD_88F6710)
		gBoardId = RD_88F6710_ID;
#else
		mvOsPrintf("mvBoardIdSet: Board ID must be defined!\n");
		while (1) {
			continue;
		}
#endif
	}
}
/*******************************************************************************
* mvBoardIdGet - Get Board model
*
* DESCRIPTION:
*       This function returns board ID.
*       Board ID is 32bit word constructed of board model (16bit) and
*       board revision (16bit) in the following way: 0xMMMMRRRR.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit board ID number, '-1' if board is undefined.
*
*******************************************************************************/
MV_U32 mvBoardIdGet(MV_VOID)
{
	if (gBoardId == -1) {
#if defined(DB_88F6710)
		gBoardId = DB_88F6710_BP_ID;
#elif defined(DB_88F6710_PCAC)
		gBoardId = DB_88F6710_PCAC_ID;
#elif defined(RD_88F6710)
		gBoardId = RD_88F6710_ID;
#else
		mvOsWarning();
		return INVALID_BAORD_ID;
#endif
	}

	return gBoardId;
}

/*******************************************************************************
* mvBoardTwsiSatRGet -
*
* DESCRIPTION:
*
* INPUT:
*	device num - one of three devices
*	reg num - 0 or 1
*
* OUTPUT:
*		None.
*
* RETURN:
*		reg value
*
*******************************************************************************/
MV_U8 mvBoardTwsiSatRGet(MV_U8 devNum, MV_U8 regNum)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U8 data;

	/* TWSI init */
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	/* Read MPP module ID */
	DB(mvOsPrintf("Board: Read S@R device read\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_DEV_TWSI_SATR, devNum);

	twsiSlave.validOffset = MV_TRUE;
	/* Use offset as command */
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;

	if (MV_OK != mvTwsiRead(0, &twsiSlave, &data, 1)) {
		DB(mvOsPrintf("Board: Read S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Read S@R succeded\n"));

	return data;
}

/*******************************************************************************
* mvBoardTwsiSatRSet -
*
* DESCRIPTION:
*
* INPUT:
*	devNum - one of three devices
*	regNum - 0 or 1
*	regVal - value
*
*
* OUTPUT:
*		None.
*
* RETURN:
*		reg value
*
*******************************************************************************/
MV_STATUS mvBoardTwsiSatRSet(MV_U8 devNum, MV_U8 regNum, MV_U8 regVal)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;

	/* TWSI init */
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	/* Read MPP module ID */
	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.validOffset = MV_TRUE;
	DB(mvOsPrintf("Board: Write S@R device addr %x, type %x, data %x\n",
		      twsiSlave.slaveAddr.address, twsiSlave.slaveAddr.type, regVal));
	/* Use offset as command */
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;
	if (MV_OK != mvTwsiWrite(0, &twsiSlave, &regVal, 1)) {
		DB1(mvOsPrintf("Board: Write S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Write S@R succeded\n"));

	return MV_OK;
}

/*******************************************************************************
* SatR Configuration functions
*******************************************************************************/
/* Swap input data bits. */
static MV_U8 mvBoardSatrSwapBits(MV_U8 val, MV_U8 width)
{
	MV_U8 i;
	MV_U8 res = 0;

	for (i = 0; i < width; i++) {
		if ((1 << i) & val)
			res |= (1 << (width - i - 1));
	}
	return res;
}

MV_U8 mvBoardFabFreqGet(MV_VOID)
{
	MV_U8 sar0, sar1, res;

	sar0 = mvBoardTwsiSatRGet(0, 0);
	sar1 = mvBoardTwsiSatRGet(1, 0);

	if (((MV_8)MV_ERROR == (MV_8)sar0) || ((MV_8)MV_ERROR == (MV_8)sar1))
		return MV_ERROR;

	res = ((sar0 & 0x10) | (mvBoardSatrSwapBits(sar1, 4) & 0xF));
	return res;
}

/*******************************************************************************/
MV_STATUS mvBoardFabFreqSet(MV_U8 freqVal)
{
	MV_U8 sar0, sar1;

	sar0 = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar0)
		return MV_ERROR;

	sar1 = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar1)
		return MV_ERROR;

	sar0 &= ~0x10;
	sar0 |= (freqVal & 0x10);
	if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar0)) {
		DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
		return MV_ERROR;
	}

	sar1 &= ~0xF;
	sar1 |= mvBoardSatrSwapBits(freqVal, 4);
	if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar1)) {
		DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write FreqOpt S@R succeeded\n"));
	return MV_OK;
}

/*******************************************************************************/
MV_U8 mvBoardCpuFreqGet(MV_VOID)
{
	MV_U8 sar;
	MV_U8 res;

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	res = sar & 0xF;
	res = mvBoardSatrSwapBits(res, 4);
	return res;
}

/*******************************************************************************/
MV_STATUS mvBoardCpuFreqSet(MV_U8 freqVal)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	freqVal = mvBoardSatrSwapBits(freqVal, 4);
	sar &= ~0xF;
	sar |= (freqVal & 0xF);
	if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar)) {
		DB1(mvOsPrintf("Board: Write CpuFreq S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write CpuFreq S@R succeeded\n"));
	return MV_OK;
}

/*******************************************************************************/
MV_U8 mvBoardBootDevGet(MV_VOID)
{
	MV_U8 sar;
	MV_U8 result;

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	result = ((sar & 0x10) << 1);

	sar = mvBoardTwsiSatRGet(2, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	result |= mvBoardSatrSwapBits(sar, 5);

	return result;
}

/*******************************************************************************/
MV_STATUS mvBoardBootDevSet(MV_U8 val)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x10);
	sar |= ((val & 0x20) >> 1);
	if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar)) {
		DB1(mvOsPrintf("Board: Write BootDev S@R fail\n"));
		return MV_ERROR;
	}

	if (MV_OK != mvBoardTwsiSatRSet(2, 0, mvBoardSatrSwapBits(val, 5))) {
		DB1(mvOsPrintf("Board: Write BootDev S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write BootDev S@R succeeded\n"));
	return MV_OK;
}

/*******************************************************************************/
MV_STATUS mvBoardPexCapabilitySet(MV_U16 conf)
{
	if (MV_OK != mvBoardTwsiSatRSet(1, 1, conf)) {
		DB(mvOsPrintf("Board: Write confID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write confID S@R succeeded\n"));
	return MV_OK;
}

/*******************************************************************************/
MV_U16 mvBoardPexCapabilityGet(MV_VOID)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(1, 1);
	return (sar & 0xFF);
}

/*******************************************************************************
* End of SatR Configuration functions
*******************************************************************************/

/*******************************************************************************
* mvBoardMppModulesScan
*
* DESCRIPTION:
*	Scan for modules connected through MPP lines.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None
*
* RETURN:
*       MV_STATUS - MV_OK, MV_ERROR.
*
*******************************************************************************/
MV_STATUS mvBoardMppModulesScan(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave, twsiSlaveGMII;
	MV_U32 boardId = mvBoardIdGet();
	MV_BOOL scanEn = mvBoardIsModScanEnabled();
	MV_BOARD_MODULE_TYPE_INFO *modInfo;
	MV_U8 swCfg;

	/* Perform scan only for DB board */
	if (scanEn == MV_FALSE)
		return MV_OK;

	modInfo = BOARD_INFO(boardId)->pBoardModTypeValue;

	modInfo->boardMppGrp1Mod  = 0;			/* MPP Giga Site */
	modInfo->boardMppGrp2Mod  = 0;			/* MPP Device Site */

	/* Giga Site Modules: */
	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_GIGA_CON_ADDR);
	FILL_TWSI_SLAVE(twsiSlaveGMII, MV_BOARD_GIGA_CON_GMII_ADDR);

	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		switch (regVal & MV_BOARD_MODULE_ID_MASK) {
		/* Switch module: DB-SGMII_Switch_1512  */
		case (MV_BOARD_SWITCH_MODULE_ID):
			modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII0;
			modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII1;
			break;
		/* TDM GMII module:  DB-KW40_GMII_TDM_Adapter - TDM Configuration */
		case (MV_BOARD_TDM_GMII_MODULE_TDM_ID):
			modInfo->boardMppGrp1Mod |= MV_BOARD_TDM;
			break;
		/* I2S SPDIF module: DB-KW40-I2S/SPDIF */
		case (MV_BOARD_I2S_SPDIF_MODULE_ID):
			modInfo->boardMppGrp1Mod |= MV_BOARD_I2S;
			break;
		default:
			break;
		}

	} else if (mvTwsiRead(0, &twsiSlaveGMII, &regVal, 1) == MV_OK) {
		if ((regVal & MV_BOARD_MODULE_ID_MASK) == MV_BOARD_TDM_GMII_MODULE_GMII_ID)
			modInfo->boardMppGrp1Mod |= MV_BOARD_GMII0;

	} else { /* No module detected */
		/* Check if SDIO or RGMII0 */
		FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_EEPROM_MODULE_ADDR);
		mvTwsiRead(0, &twsiSlave, &swCfg, 1);
		if (MV_BOARD_CFG_SDIO_MODE(swCfg) == 1)
			modInfo->boardMppGrp1Mod |= MV_BOARD_SDIO;
		else
			modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII0;
		/* Set RGMII1 */
		modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII1;
	}

	/* Device Site Modules: */
	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_DEVICE_CON_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		switch (regVal & MV_BOARD_MODULE_ID_MASK) {
		/* TDM module: DB-78x60_4xFXS-VX880 */
		case (MV_BOARD_TDM_GMII_MODULE_TDM_ID):
			modInfo->boardMppGrp2Mod |= MV_BOARD_TDM;
			break;
		/* I2S SPDIF module: DB-KW40-I2S/SPDIF */
		case (MV_BOARD_I2S_SPDIF_MODULE_ID):
			modInfo->boardMppGrp2Mod |= MV_BOARD_I2S;
			break;
		default:
			break;
		}
	} else { /* No module detected */

	}

	/* Check if SGMII module. connected - disable RGMII/GMII (SMI is directed to serdeses) */
	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_SERDES_CON_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		modInfo->boardMppGrp1Mod &= ~MV_BOARD_RGMII0;
		modInfo->boardMppGrp1Mod &= ~MV_BOARD_RGMII1;
		modInfo->boardMppGrp1Mod &= ~MV_BOARD_GMII0;
	}

	return MV_OK;
}

/*******************************************************************************
* mvBoardMppTypeIndexGet
*
* DESCRIPTION:
*	Get MPP type index of a given MPP type.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_STATUS - MV_OK, MV_ERROR.
*
*******************************************************************************/
static MV_U8 mvBoardMppTypeIndexGet(MV_BOARD_MPP_TYPE_CLASS type)
{
	MV_U8 i = 0;

	while (gBoardMppType2Index[i] != 0xFFFFFFFF) {
		if (gBoardMppType2Index[i] == type)
			return i;
		i++;
	}

	return 0x0;
}

/*******************************************************************************
* mvBoardUpdateMppAfterScan
*
* DESCRIPTION:
*	Update board MPPs list after modules scan.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_STATUS - MV_OK, MV_ERROR.
*
*******************************************************************************/
MV_STATUS mvBoardUpdateMppAfterScan(void)
{
	MV_BOOL scanEn = mvBoardIsModScanEnabled();
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 *mppList = BOARD_INFO(boardId)->pBoardMppConfigValue->mppGroup;
	MV_U32 mppGroup1[][4][2] = MPP_GROUP_1_TYPE;
	MV_U32 mppGroup2[][4][2] = MPP_GROUP_2_TYPE;
	MV_BOARD_MODULE_TYPE_INFO *modInfo;
	MV_U32 mpp, mppIdx;
	MV_U32 bootVal, mask, width;
	MV_U8 index, i;

	modInfo = BOARD_INFO(boardId)->pBoardModTypeValue;

	/* Perform update if scan is enabled.	*/
	if (scanEn == MV_FALSE)
		return MV_OK;

	/* First group - Giga Site */
	for (i = 0; i < 32; i++) {
		if (!((1 << i) &  modInfo->boardMppGrp1Mod))
			continue;
		index = mvBoardMppTypeIndexGet((1 << i));
		for (mpp = 0; mpp < 4; mpp++) {
			if (mppGroup1[index][mpp][0] != 0x0) {
				mppList[mpp] &= ~mppGroup1[index][mpp][0];
				mppList[mpp] |= mppGroup1[index][mpp][1];
			}
		}
	}

	/* Second group - Device Site */
	for (i = 0; i < 32; i++) {
		if (!((1 << i) &  modInfo->boardMppGrp2Mod))
			continue;
		index = mvBoardMppTypeIndexGet((1 << i));
		for (mpp = 0; mpp <= 4; mpp++) {
			mppIdx = mpp + 4;
			if (mppGroup2[index][mpp][0] != 0x0) {
				mppList[mppIdx] &= ~mppGroup2[index][mpp][0];
				mppList[mppIdx] |= mppGroup2[index][mpp][1];
			}
			width = mvCtrlIsBootFromNAND() || mvCtrlIsBootFromNOR();
			mask = 0x0;
			if (mvCtrlIsBootFromSPI() == MV_SPI_LOW_MPPS) {
				if (mppIdx == 4)
					mask = 0xFFFF0;
			} else if (mvCtrlIsBootFromSPI() == MV_SPI_HIGH_MPPS) {
				switch (mppIdx) {
				case 4:
					mask = 0xF;
					break;
				case 7:
					mask = 0xF0000000;
					break;
				case 8:
					mask = 0xFF;
					break;
				default:
					mask = 0x0;
					break;
				}
			} else if (width) {
				switch (mppIdx) {
				case 4:
					mask = 0xFFFFFFF0;
					break;
				case 5:
					if (width == MV_NAND_NOR_BOOT_8BIT)
						mask = 0x0FFFFFFF;
					else
						mask = 0xFFFFFFFF;
				case 6:
					if (width == MV_NAND_NOR_BOOT_16BIT)
						mask = 0xFFFFFFFF;
					break;
				case 7:
					if (width == MV_NAND_NOR_BOOT_16BIT)
						mask = 0xFFF;
					break;
				default:
					mask = 0x0;
					break;
				}
			}

			if (mask != 0) {
				bootVal = MV_REG_READ(mvCtrlMppRegGet(mppIdx));
				mppList[mppIdx] &= ~mask;
				mppList[mppIdx] |= (bootVal & mask);
			}
		}
	}

	return MV_OK;
}

/*******************************************************************************
* mvBoardIsModScanEnabled
*
* DESCRIPTION:
*	Check if modules scanning is enabled on this board.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_STATUS - MV_OK, MV_ERROR.
*
*******************************************************************************/
MV_BOOL mvBoardIsModScanEnabled(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsModScanEnabled:Board unknown.\n");
		return MV_FALSE;
	}

	return BOARD_INFO(boardId)->enableModuleScan;
}

/*******************************************************************************
* mvBoardIsSwitchConnected
*
* DESCRIPTION:
*	Check if switch module is connected to giga site.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_BOOL - MV_TRUE, MV_FALSE.
*
*******************************************************************************/
MV_BOOL mvBoardIsSwitchConnected(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave;
	MV_U32 boardId = mvBoardIdGet();
	MV_BOOL scanEn = mvBoardIsModScanEnabled();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsSwitchConnected:Board unknown.\n");
		return MV_FALSE;
	}

	if (scanEn == MV_FALSE) {
		if (BOARD_INFO(boardId)->switchInfoNum > 0)
			return MV_TRUE;
		return MV_FALSE;
	}

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_GIGA_CON_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		if ((regVal & MV_BOARD_MODULE_ID_MASK) == MV_BOARD_SWITCH_MODULE_ID)
			return MV_TRUE;
	}

	return MV_FALSE;
}

/*******************************************************************************
* mvBoardIsGMIIConnected
*
* DESCRIPTION:
*	Check if GMII module is connected to giga site.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_BOOL - MV_TRUE, MV_FALSE.
*
*******************************************************************************/
MV_BOOL mvBoardIsGMIIConnected(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave;
	MV_U32 boardId = mvBoardIdGet();
	MV_BOOL scanEn = mvBoardIsModScanEnabled();

	/* Perform update if scan is enabled.	*/
	if (scanEn == MV_FALSE)
		return MV_FALSE;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsGMIIConnected:Board unknown.\n");
		return MV_FALSE;
	}

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_GIGA_CON_GMII_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		if ((regVal & MV_BOARD_MODULE_ID_MASK) == MV_BOARD_TDM_GMII_MODULE_GMII_ID)
			return MV_TRUE;
	}

	return MV_FALSE;
}

/*******************************************************************************
* mvBoardSmiScanModeGet - Get Switch SMI scan mode
*
* DESCRIPTION:
*       This routine returns Switch SMI scan mode.
*
* INPUT:
*       switchIdx - index of the switch. Only 0 is supported.
*
* OUTPUT:
*       None.
*
* RETURN:
*       1 for SMI_MANUAL_MODE, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32 mvBoardSmiScanModeGet(MV_U32 switchIdx)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSmiScanModeGet: Board unknown.\n");
		return -1;
	}

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].smiScanMode;
}

/*******************************************************************************
* mvBoardUpdateEthAfterScan
*
* DESCRIPTION:
*	Update board MACs parameters after modules scan.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*       MV_STATUS - MV_OK, MV_ERROR.
*
*******************************************************************************/
MV_STATUS mvBoardUpdateEthAfterScan(void)
{
	MV_BOOL scanEn = mvBoardIsModScanEnabled();
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardUpdateEthAfterScan:Board unknown.\n");
		return MV_ERROR;
	}

	/* Perform update if scan is enabled.	*/
	if (scanEn == MV_FALSE)
		return MV_OK;

	/* Check if switch is connected. */
	if (MV_TRUE == mvBoardIsSwitchConnected()) {
		/* Switch is connected - set mac speed manually to 1000 */
		BOARD_INFO(boardId)->pBoardMacInfo[1].boardMacSpeed = BOARD_MAC_SPEED_1000M;
		BOARD_INFO(boardId)->pBoardMacInfo[1].boardEthSmiAddr = 0x10;
	} else {
		BOARD_INFO(boardId)->pSwitchInfo = NULL;
		BOARD_INFO(boardId)->switchInfoNum = 0;
	}

	if (MV_TRUE == mvBoardIsGMIIConnected())
		BOARD_INFO(boardId)->pBoardMacInfo[0].boardEthSmiAddr = 0x8;

	return MV_OK;
}

/*******************************************************************************
* mvBoardMppModulesCfgGet
*
* DESCRIPTION:
*	Get board MPP options configuration.
*
* INPUT:
*	group - MPP group to get the configuration for.
*
* OUTPUT:
*	None.
*
* RETURN:
*	Bitmap of the MPP configuration.
*
*******************************************************************************/
MV_U32 mvBoardMppModulesCfgGet(MV_U8 group)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMppModulesCfgGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (group == 1)
		return BOARD_INFO(boardId)->pBoardModTypeValue->boardMppGrp1Mod;
	else
		return BOARD_INFO(boardId)->pBoardModTypeValue->boardMppGrp2Mod;
}

/*******************************************************************************
* mvBoardPexInfoGet - Get board PEX Info
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*******************************************************************************/
MV_BOARD_PEX_INFO *mvBoardPexInfoGet(void)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	switch (boardId) {
	case DB_88F6710_BP_ID:
	case DB_88F6710_PCAC_ID:
	case RD_88F6710_ID:
#if defined(CONFIG_SYNO_ARMADA_ARCH)
	case SYNO_DS213j_ID:
	case SYNO_US3_ID:
	case SYNO_RS214_ID:
	case SYNO_DS214se_ID:
	case SYNO_DS414slim_ID:
	case SYNO_DS115j_ID:
	case SYNO_DS216se_ID:
#endif
		return &BOARD_INFO(boardId)->boardPexInfo;
		break;
	default:
		DB(mvOsPrintf("mvBoardPexInfoGet: Unsupported board!\n"));
		return NULL;
	}
}

/*******************************************************************************
* mvBoardBitMaskConfigSet - Set any configuration according to bit mask passed
* 			    from U-Boot.
*
* DESCRIPTION:
*
* INPUT:
*		config - 32 bit mask.
* OUTPUT:
*       None.
*
* RETURN:
*******************************************************************************/
MV_VOID mvBoardBitMaskConfigSet(MV_U32 config)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (boardId == RD_88F6710_ID) {
		/* HDD Select */
		if (config & BIT0) {
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(1), BIT31);
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(2), BIT0);
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(2), BIT1);
		} else {
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(1), BIT31);
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(2), BIT0);
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(2), BIT1);
		}
	}
}
