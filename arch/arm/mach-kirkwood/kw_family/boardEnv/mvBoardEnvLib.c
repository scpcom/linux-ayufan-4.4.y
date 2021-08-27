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

#include <linux/kernel.h>
#include <linux/init.h>
#include "common.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "cpu/mvCpu.h"
#include "gpp/mvGpp.h"
#include "twsi/mvTwsi.h"
#include "eth/gbe/mvEthRegs.h"

#ifdef MV_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

extern	MV_BOARD_INFO*	boardInfoTbl[];
#define BOARD_INFO(boardId)	boardInfoTbl[boardId - BOARD_ID_BASE]

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
#define NAND_READ_PARAMS_REG		0x10418
#define NAND_WRITE_PARAMS_REG		0x1041c
#define NAND_CTRL_REG			0x10470
#define NAND_ACTCEBOOT_BIT		BIT1

MV_VOID mvBoardEnvInit(MV_VOID)
{
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardEnvInit:Board unknown.\n");
		return;

	}

	if (mvBoardIdGet() != XCAT98DX_ID) {
		/* Set NAND interface access parameters */
		MV_REG_WRITE(NAND_READ_PARAMS_REG, BOARD_INFO(boardId)->nandFlashReadParams);
		MV_REG_WRITE(NAND_WRITE_PARAMS_REG, BOARD_INFO(boardId)->nandFlashWriteParams);
		MV_REG_WRITE(NAND_CTRL_REG, BOARD_INFO(boardId)->nandFlashControl);
	}

	/* Set GPP Out value */
	MV_REG_WRITE(GPP_DATA_OUT_REG(0), BOARD_INFO(boardId)->gppOutValLow);
	MV_REG_WRITE(GPP_DATA_OUT_REG(1), BOARD_INFO(boardId)->gppOutValHigh);

	/* set GPP polarity */
	mvGppPolaritySet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValLow);
	mvGppPolaritySet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValHigh);

    /* Workaround for Erratum FE-MISC-70*/
    if(mvCtrlRevGet()==MV_88F6XXX_A0_REV)
    {
        BOARD_INFO(boardId)->gppOutEnValLow &= 0xfffffffd;
        BOARD_INFO(boardId)->gppOutEnValLow |= (BOARD_INFO(boardId)->gppOutEnValHigh) & 0x00000002;
    } /*End of WA*/

	/* Set GPP Out Enable*/
	mvGppTypeSet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValLow);
	mvGppTypeSet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValHigh);

	/* Nand CE */
	MV_REG_BIT_SET(NAND_CTRL_REG, NAND_ACTCEBOOT_BIT);
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
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsSPrintf (pNameBuff, "Board unknown.\n");
		return MV_ERROR;

	}

	mvOsSPrintf (pNameBuff, "%s",BOARD_INFO(boardId)->boardName);

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
    MV_BOOL ethPortSgmiiSupport[BOARD_ETH_PORT_NUM] = MV_ETH_PORT_SGMII;

    if(ethPortNum >= BOARD_ETH_PORT_NUM)
    {
	    mvOsPrintf ("Invalid portNo=%d\n", ethPortNum);
		return MV_FALSE;
    }
    return ethPortSgmiiSupport[ethPortNum];
}

/*******************************************************************************
* mvBoardIsPortInGmii -
*
* DESCRIPTION:
*       This routine returns MV_TRUE for port number works in GMII or MV_FALSE
*	For all other options.
*
* INPUT:
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - port in GMII.
*       MV_FALSE - other.
*
*******************************************************************************/
MV_BOOL mvBoardIsPortInGmii(MV_VOID)
{
	MV_U32 devClassId, devClass = 0;
	if (mvBoardMppGroupTypeGet(devClass) == MV_BOARD_AUTO)
	{
		/* Get MPP module ID */
		devClassId = mvBoarModuleTypeGet(devClass);
		if (MV_BOARD_MODULE_GMII_ID == devClassId)
			return MV_TRUE;
	}
	else if (mvBoardMppGroupTypeGet(devClass) == MV_BOARD_GMII)
		return MV_TRUE;

    return MV_FALSE;
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
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardPhyAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
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
MV_BOARD_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMacSpeedGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

/*******************************************************************************
* mvBoardLinkStatusIrqGet - Get the IRQ number for the link status indication
*
* DESCRIPTION:
*       This routine returns the IRQ number for the link status indication.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*       switchIndex - Switch index.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR - if ethPortNum or switchIndex are illegal or board is unknown
*       The number of the IRQ for the link status indication, -1 if the port
*       number is wrong or if not relevant.
*
*******************************************************************************/
MV_32	mvBoardLinkStatusIrqGet(MV_U32 ethPortNum, MV_U32 switchIndex)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardLinkStatusIrqGet: Board unknown.\n");
		return (-MV_BAD_VALUE);
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardLinkStatusIrqGet: Illegal port number(%u)\n", ethPortNum);
		return (-MV_BAD_VALUE);
	}

    if (switchIndex >= BOARD_MAX_CASCADED_SWITCHES)
    {
        mvOsPrintf("mvBoardLinkStatusIrqGet: Illegal Switch index(%u)\n", switchIndex);
        return (-MV_BAD_VALUE);
    }

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
	    return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].linkStatusIrq[switchIndex];

    return (-MV_BAD_VALUE);
}

/*******************************************************************************
* mvBoardSwitchPortGet - Get the mapping between the board connector (logical port number) and the
*                        physical Switch port
*
* DESCRIPTION:
*       This routine returns the matching Switch port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*       logicalPortNum - logical number of the connector on the board
*
* OUTPUT:
*       switchIndex - Switch index.
*       portNum - Switch port number.
*       portName - Switch port name (string)
*
* RETURN:
*       MV_OK on success, MV_ERROR otherwise.
*
*******************************************************************************/
MV_32	mvBoardSwitchPortGet(MV_U32 ethPortNum, MV_U8 logicalPortNum, MV_32 *switchIndex, MV_32 *portNum, char *portName)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSwitchPortGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (logicalPortNum >= BOARD_ETH_SWITCH_PORT_NUM)
	{
		mvOsPrintf("mvBoardSwitchPortGet: Illegal board port number.\n");
		return MV_ERROR;
	}

    if ((switchIndex == NULL) || (portNum == NULL))
    {
        mvOsPrintf("mvBoardSwitchPortGet: got NULL arguments.\n");
        return MV_ERROR;
    }

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
    {
        *switchIndex    = BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdPort[logicalPortNum].qdSwitchNum;
        *portNum        = BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdPort[logicalPortNum].qdPortNum;
        memcpy(portName, BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdPort[logicalPortNum].portStr, BOARD_ETH_MAX_PORT_STRING);
        return MV_OK;
    }

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardSwitchCpuPortGet - Get the the Ethernet Switch CPU port
*
* DESCRIPTION:
*       This routine returns the Switch CPU port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*       switchIndex - Switch index.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR - if ethPortNum or switchIndex are illegal or board is unknown
*       The Switch CPU port, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32	mvBoardSwitchCpuPortGet(MV_U32 ethPortNum, MV_U32 switchIndex)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSwitchCpuPortGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardSwitchCpuPortGet: Illegal port number(%u)\n", ethPortNum);
		return MV_ERROR;
	}

    if (switchIndex >= BOARD_MAX_CASCADED_SWITCHES)
    {
        mvOsPrintf("mvBoardSwitchCpuPortGet: Illegal Switch index(%u)\n", switchIndex);
        return MV_ERROR;
    }

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
	    return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdCpuPort[switchIndex];

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardSwitchInterconnectPortGet - Get the the Ethernet Switch Interconnect port
*
* DESCRIPTION:
*       This routine returns the Switch Interconnect port - the port that is connected to another Switch (if relevant).
*
* INPUT:
*       ethPortNum - Ethernet port number.
*       switchIndex - Switch index.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR - if ethPortNum or switchIndex are illegal or board is unknown
*       The Switch Interconnect port, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32	mvBoardSwitchInterconnectPortGet(MV_U32 ethPortNum, MV_U32 switchIndex)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSwitchInterconnectPortGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardSwitchInterconnectPortGet: Illegal port number(%u)\n", ethPortNum);
		return MV_ERROR;
	}

    if (switchIndex >= BOARD_MAX_CASCADED_SWITCHES)
    {
        mvOsPrintf("mvBoardSwitchInterconnectPortGet: Illegal Switch index(%u)\n", switchIndex);
        return MV_ERROR;
    }

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdInterconnectPort[switchIndex];

    return MV_ERROR;
}

/*******************************************************************************
* mvBoardSwitchSmiAddrGet - Get the the Ethernet Switch SMI address
*
* DESCRIPTION:
*       This routine returns the Switch SMI address.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*       switchIndex - Switch index.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR - if ethPortNum or switchIndex are illegal or board is unknown
*       The Switch SMI address, -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32	mvBoardSwitchSmiAddrGet(MV_U32 ethPortNum, MV_U32 switchIndex)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSwitchSmiAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardSwitchSmiAddrGet: Illegal port number(%u)\n", ethPortNum);
		return MV_ERROR;
	}

    if (switchIndex >= BOARD_MAX_CASCADED_SWITCHES)
    {
        mvOsPrintf("mvBoardSwitchSmiAddrGet: Illegal Switch index(%u)\n", switchIndex);
        return MV_ERROR;
    }

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].switchSmiAddr[switchIndex];

    return MV_ERROR;
}

/*******************************************************************************
 * mvBoardDsaTagLenGet
 */
MV_U32 mvBoardDsaTagLenGet(MV_U32 port)
{
	MV_BOARD_INFO *b = BOARD_INFO(mvBoardIdGet());

	if (port >= b->numBoardMacInfo)
	{
		mvOsPrintf("%s: Wrong port number (%u)\n", __func__, port);
		return MV_ERROR;
	}

	return b->portDsaInfo[port].dsaTagLen;
}

/*******************************************************************************
 * mvBoardUseDsaTagOnPort
 */
MV_BOOL mvBoardUseDsaTagOnPort(MV_U32 port)
{
	MV_BOARD_INFO *b = BOARD_INFO(mvBoardIdGet());

	if (port >= b->numBoardMacInfo)
	{
		mvOsPrintf("%s: Wrong port number (%u)\n", __func__, port);
		return MV_ERROR;
	}

	return b->portDsaInfo[port].useDsaTag;
}

/*******************************************************************************
* mvBoardNumSwitchesOnPortGet - Get number of cascaded Switches on this Ethernet port
*
* DESCRIPTION:
*       This routine returns the number of cascaded Switches on this Ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR - if ethPortNum is illegal or board is unknown
*       0 if no Switch information available
*       1 if only one Switch is connected to this Ethernet port, 2 if two Switches are cascaded together on this port.
*
*******************************************************************************/
MV_32	mvBoardNumSwitchesOnPortGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardNumSwitchesOnPortGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardNumSwitchesOnPortGet: Illegal port number(%u)\n", ethPortNum);
		return MV_ERROR;
	}

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].numSwitchesOnPort;

    return 0;
}

/*********************************************************************************
* mvBoardIsSwitchConnected - Check if a Switch is connected on this Ethernet Port
*
* DESCRIPTION:
*       This routine checks if a Switch is connected on this Ethernet port.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE - if a Switch (or more than one Switch) is connected on this Ethernet Port
*       MV_FALSE - otherwise (or in case of error)
*
*******************************************************************************/
MV_BOOL     mvBoardIsSwitchConnected(MV_U32 ethPortNum)
{
    if (mvBoardNumSwitchesOnPortGet(ethPortNum) > 0)
        return MV_TRUE;

    return MV_FALSE;
}
/*******************************************************************************
* mvBoardSmiScanModeGet - Get Switch SMI scan mode
*
* DESCRIPTION:
*       This routine returns Switch SMI scan mode.
*
* INPUT:
*       ethPortNum - Ethernet port number.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR - if ethPortNum is illegal or board is unknown
*       1 for SMI_MANUAL_MODE,
*       2 for SMI_MULTI_ADDR_MODE,
*       -1 if the port number is wrong or if not relevant.
*
*******************************************************************************/
MV_32	mvBoardSmiScanModeGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSmiScanModeGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardSmiScanModeGet: Illegal port number(%u)\n", ethPortNum);
		return MV_ERROR;
	}

    if ((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))
	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].smiScanMode;

    return MV_ERROR;
}

/*******************************************************************************
* mvBoarGpioPinGet - mvBoarGpioPinGet
*
* DESCRIPTION:
*
* INPUT:
*		class - MV_BOARD_GPP_CLASS enum.
*
* OUTPUT:
*		None.
*
* RETURN:
*       GPIO pin number. The function return -1 for bad parameters.
*
*******************************************************************************/
MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS class, MV_U32 index)
{
	MV_U32 boardId, i;
	MV_U32 indexFound = 0;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("Board unknown.\n");
		return MV_ERROR;

	}

        for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++)
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == class) {
			if (indexFound == index)
				return (MV_U32)BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			else
				indexFound++;

		}

	return MV_ERROR;
}

/*******************************************************************************
* mvBoardMppGet - Get board dependent MPP register value
*
* DESCRIPTION:
*		MPP settings are derived from board design.
*		MPP group consist of 8 MPPs. An MPP group represent MPP
*		control register.
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

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;

	}

	return BOARD_INFO(boardId)->pBoardMppConfigValue[0].mppGroup[mppGroupNum];
}

/*******************************************************************************
* mvBoardMppGroupId - If MPP group type is AUTO then identify it using twsi
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
MV_VOID mvBoardMppGroupIdUpdate(MV_VOID)
{
	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MODULE_ID_CLASS devClassId;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U32 maxMppGrp = 1;

	devId = mvCtrlModelGet();

	switch(devId){
        case MV_6281_DEV_ID:
            maxMppGrp = MV_6281_MPP_MAX_MODULE;
            break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_MODULE;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_MODULE;
            break;
        case MV_6192_DEV_ID:
        case MV_6701_DEV_ID:
        case MV_6702_DEV_ID:
            maxMppGrp = MV_6192_MPP_MAX_MODULE;
            break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_MODULE;
            break;
        case MV_6180_DEV_ID:
            maxMppGrp = MV_6180_MPP_MAX_MODULE;
            break;
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		/* If MPP group can be defined by the module connected to it */
		if (mvBoardMppGroupTypeGet(devClass) == MV_BOARD_AUTO)
		{
			/* Get MPP module ID */
			devClassId = mvBoarModuleTypeGet(devClass);
			if (MV_ERROR != devClassId)
			{
				switch(devClassId)
				{
				case MV_BOARD_MODULE_TDM_ID:
				case MV_BOARD_MODULE_TDM_5CHAN_ID:
					mppGroupType = MV_BOARD_TDM;
					break;
				case MV_BOARD_MODULE_AUDIO_ID:
					mppGroupType = MV_BOARD_AUDIO;
					break;
				case MV_BOARD_MODULE_RGMII_ID:
					mppGroupType = MV_BOARD_RGMII;
					break;
				case MV_BOARD_MODULE_GMII_ID:
					mppGroupType = MV_BOARD_GMII;
					break;
				case MV_BOARD_MODULE_TS_ID:
					mppGroupType = MV_BOARD_TS;
					break;
				case MV_BOARD_MODULE_MII_ID:
					mppGroupType = MV_BOARD_MII;
					break;
				case MV_BOARD_MODULE_LCD_ID:
					mppGroupType = MV_BOARD_LCD;
					break;
				default:
					mppGroupType = MV_BOARD_OTHER;
					break;
				}
			}
			else
				/* The module bay is empty */
				mppGroupType = MV_BOARD_OTHER;

			/* Update MPP group type */
			mvBoardMppGroupTypeSet(devClass, mppGroupType);
		}

		/* Update MPP output voltage for RGMII 1.8V. Set port to GMII for GMII module */
		if ((mvBoardMppGroupTypeGet(devClass) == MV_BOARD_RGMII))
			MV_REG_BIT_SET(MPP_OUTPUT_DRIVE_REG,MPP_1_8_RGMII1_OUTPUT_DRIVE | MPP_1_8_RGMII0_OUTPUT_DRIVE);
		else
		{
			if ((mvBoardMppGroupTypeGet(devClass) == MV_BOARD_GMII))
			{
				MV_REG_BIT_RESET(MPP_OUTPUT_DRIVE_REG, BIT7 | BIT15);
				MV_REG_BIT_RESET(ETH_PORT_SERIAL_CTRL_1_REG(0),BIT3);
				MV_REG_BIT_RESET(ETH_PORT_SERIAL_CTRL_1_REG(1),BIT3);
			}
			else if ((mvBoardMppGroupTypeGet(devClass) == MV_BOARD_MII))
			{
				/* Assumption that the MDC & MDIO should be 3.3V */
				MV_REG_BIT_RESET(MPP_OUTPUT_DRIVE_REG, BIT7 | BIT15);
				/* Assumption that only ETH1 can be MII when using modules on DB */
				MV_REG_BIT_RESET(ETH_PORT_SERIAL_CTRL_1_REG(1),BIT3);
			}
		}
	}

	/* Update MPP when module is LCD to override both group */
	devId = mvCtrlModelGet();
	mppGroupType = mvBoardMppGroupTypeGet(0);
	if ((devId == MV_6282_DEV_ID) && (mppGroupType == MV_BOARD_LCD))
	{
	    /* LCD module include AUDIO intrface over NAND ctrl lines */
	    MV_REG_BIT_SET(NAND_AUDIO_PIN_MUX, NAND_AUDIO_PIN_MUX_SELECT_AUDIO_MODE);

	    /* Override MPP group 1 type to LCD to update all MPP for LCD */
	    mvBoardMppGroupTypeSet(1, MV_BOARD_LCD);
	}
}

/*******************************************************************************
* mvBoardMppGroupTypeGet
*
* DESCRIPTION:
*
* INPUT:
*       mppGroupClass - MPP group number 0  for MPP[35:20] or 1 for MPP[49:36].
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_BOARD_MPP_TYPE_CLASS mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;

	}

	if (mppGroupClass == MV_BOARD_MPP_GROUP_1)
		return BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup1;
	else
		return BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup2;
}

/*******************************************************************************
* mvBoardMppGroupTypeSet
*
* DESCRIPTION:
*
* INPUT:
*       mppGroupClass - MPP group number 0  for MPP[35:20] or 1 for MPP[49:36].
*       mppGroupType - MPP group type for MPP[35:20] or for MPP[49:36].
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardMppGroupTypeSet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass,
						MV_BOARD_MPP_TYPE_CLASS mppGroupType)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
	}

	if (mppGroupClass == MV_BOARD_MPP_GROUP_1)
		BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup1 = mppGroupType;
	else
		BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup2 = mppGroupType;

}

/*******************************************************************************
* mvBoardMppMuxSet - Update MPP mux
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
MV_VOID mvBoardMppMuxSet(MV_VOID)
{
	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U8 muxVal = 0xf;
	MV_U32 maxMppGrp = 1;
    MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;

	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_MODULE;
			break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_MODULE;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_MODULE;
			break;
		case MV_6192_DEV_ID:
        case MV_6701_DEV_ID:
        case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_MODULE;
			break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_MODULE;
            break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_MODULE;
			break;
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		mppGroupType = mvBoardMppGroupTypeGet(devClass);

		switch(mppGroupType)
		{
			case MV_BOARD_TDM:
				muxVal &= ~(devClass ? (0x2 << (devClass * 2)):0x0);
				break;
			case MV_BOARD_AUDIO:
				 muxVal &= ~(devClass ? 0x7 : 0x0); /*old Z0 value 0xd:0x0*/
				break;
			case MV_BOARD_TS:
				 muxVal &= ~(devClass ? (0x2 << (devClass * 2)):0x0);
				break;
			case MV_BOARD_LCD:
				 muxVal = 0x0;
				break;
			default:
				muxVal |= (devClass ? 0xf : 0);
				break;
		}
	}

	/* TWSI init */
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, kirkwood_tclk, &slave, 0);

	/* Read MPP module ID */
	DB(mvOsPrintf("Board: twsi exp set\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(MV_BOARD_MUX_I2C_ADDR_ENTRY);
	twsiSlave.slaveAddr.type = mvBoardTwsiExpAddrTypeGet(MV_BOARD_MUX_I2C_ADDR_ENTRY);
	twsiSlave.validOffset = MV_TRUE;
	/* Offset is the first command after the address which indicate the register number to be read
	   in next operation */
	twsiSlave.offset = 2;
	twsiSlave.moreThen256 = MV_FALSE;

	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
	{
		DB(mvOsPrintf("Board: twsi exp out val fail\n"));
		return;
	}
	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));

	/* Change twsi exp to output */
	twsiSlave.offset = 6;
	muxVal = 0;
	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
	{
		DB(mvOsPrintf("Board: twsi exp change to out fail\n"));
		return;
	}
	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));

}

/*******************************************************************************
* mvBoardTdmMppSet - set MPPs in TDM module
*
* DESCRIPTION:
*
* INPUT: type of second telephony device
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvBoardTdmMppSet(MV_32 chType)
{
	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U8 muxVal = 1;
	MV_U8 muxValMask = 1;
	MV_U8 twsiVal;
	MV_U32 maxMppGrp = 1;
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;

	devId = mvCtrlModelGet();

	switch(devId)
	{
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_MODULE;
			break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_MODULE;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_MODULE;
			break;
		case MV_6192_DEV_ID:
		case MV_6701_DEV_ID:
		case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_MODULE;
			break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_MODULE;
            break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_MODULE;
			break;
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		mppGroupType = mvBoardMppGroupTypeGet(devClass);
		if(mppGroupType == MV_BOARD_TDM)
			break;
	}

	if(devClass == maxMppGrp)
		return;		/* TDM module not found */

	/* TWSI init */
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, kirkwood_tclk, &slave, 0);

	/* Read MPP module ID */
	DB(mvOsPrintf("Board: twsi exp set\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(devClass);
	twsiSlave.slaveAddr.type = ADDR7_BIT;
	twsiSlave.validOffset = MV_TRUE;
	/* Offset is the first command after the address which indicate the register number to be read
	   in next operation */
	twsiSlave.offset = 3;
	twsiSlave.moreThen256 = MV_FALSE;

	if(mvBoardIdGet() == RD_88F6281A_ID)
	{
		muxVal = 0xc;
		muxValMask = 0xf3;
	}

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        muxVal = (twsiVal & muxValMask) | muxVal;

	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
	{
		mvOsPrintf("Board(1): twsi exp out val fail\n");
		return;
	}
	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));

	/* Change twsi exp to output */
	twsiSlave.offset = 7;
	muxVal = 0xfe;
	if(mvBoardIdGet() == RD_88F6281A_ID)
		muxVal = 0xf3;

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
	muxVal = (twsiVal & muxVal);

	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
	{
		mvOsPrintf("Board: twsi exp change to out fail\n");
		return;
	}
	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));
	/* reset the line to 0 */
	twsiSlave.offset = 3;
	muxVal = 0;
	muxValMask = 1;

	if(mvBoardIdGet() == RD_88F6281A_ID)
	{
		muxVal = 0x0;
		muxValMask = 0xf3;
	}

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        muxVal = (twsiVal & muxValMask) | muxVal;

	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
	{
		mvOsPrintf("Board(2): twsi exp out val fail\n");
		return;
	}
	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));

	mvOsDelay(20);

	/* set the line to 1 */
	twsiSlave.offset = 3;
	muxVal = 1;
	muxValMask = 1;

	if(mvBoardIdGet() == RD_88F6281A_ID)
	{
		muxVal = 0xc;
		muxValMask = 0xf3;
		if(chType) /* FXS - issue reset properly */
		{
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(1), MV_GPP12);
			mvOsDelay(50);
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(1), MV_GPP12);
		}
		else /* FXO - issue reset via TDM_CODEC_RST*/
		{
		   /* change MPP44 type to TDM_CODEC_RST(0x2) */
		   MV_REG_WRITE(MPP_CONTROL_REG5, ((MV_REG_READ(MPP_CONTROL_REG5) & 0xFFF0FFFF)  | BIT17));
		}
	}

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        muxVal = (twsiVal & muxValMask) | muxVal;

	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
	{
		mvOsPrintf("Board: twsi exp out val fail\n");
		return;
	}

	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
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
	switch(mvBoardIdGet())
	{
		case RD_88F6281A_ID:
			return DAISY_CHAIN_MODE;
		case DB_88F6281A_BP_ID:
		case RD_88F6192A_ID:
		case DB_88F6192A_BP_ID:
		case DB_88F6701A_BP_ID:
		case DB_88F6702A_BP_ID:
		case DB_88F6282A_BP_ID:
		case RD_88F6282A_ID:
			 return DUAL_CHIP_SELECT_MODE;
		default:
			mvOsPrintf("%s: unknown boardId(0x%x)\n",__FUNCTION__, mvBoardIdGet());
			return DUAL_CHIP_SELECT_MODE;
	}
}

/*******************************************************************************
* mvBoardTwsiExpAddrTypeGet -
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
*
*******************************************************************************/
MV_U8 mvBoardTwsiExpAddrTypeGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_EXP)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}

	return (MV_ERROR);
}

/*******************************************************************************
* mvBoardTwsiExpAddrGet -
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
*
*******************************************************************************/
MV_U8 mvBoardTwsiExpAddrGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_EXP)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}

	return (0xFF);
}

/*******************************************************************************
* mvBoardTwsiSatRAddrTypeGet -
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
*
*******************************************************************************/
MV_U8 mvBoardTwsiSatRAddrTypeGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_SATR)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}

	return (MV_ERROR);
}

/*******************************************************************************
* mvBoardTwsiSatRAddrGet -
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
*
*******************************************************************************/
MV_U8 mvBoardTwsiSatRAddrGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_SATR)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}

	return (0xFF);
}

MV_U32 gBoardId;

/*******************************************************************************
* mvBoardIdSet - Set Board model
*
* DESCRIPTION:
*       Board ID is 32bit word constructed of board model (16bit) and
*       board revision (16bit) in the following way: 0xMMMMRRRR.
*
* INPUT:
*       Board ID.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvBoardIdSet(MV_U32 boardId)
{
	gBoardId = boardId;
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
	MV_U32 tmpBoardId = -1;

	if(gBoardId == -1)
        {
		#if defined(DB_88F6281A)
		tmpBoardId = DB_88F6281A_BP_ID;
		#elif defined(DB_88F6282A)
		tmpBoardId = DB_88F6282A_BP_ID;
		#elif defined(DB_88F6280A)
		tmpBoardId = DB_88F6280A_BP_ID;
		#elif defined(RD_88F6281A)
		tmpBoardId = RD_88F6281A_ID;
		#elif defined(RD_88F6282A)
		tmpBoardId = RD_88F6282A_ID;
		#elif defined(DB_88F6192A)
		tmpBoardId = DB_88F6192A_BP_ID;
		#elif defined(DB_88F6701A)
		tmpBoardId = DB_88F6701A_BP_ID;
		#elif defined(DB_88F6702A)
		tmpBoardId = DB_88F6702A_BP_ID;
		#elif defined(DB_88F6190A)
		tmpBoardId = DB_88F6190A_BP_ID;
		#elif defined(RD_88F6192A)
		tmpBoardId = RD_88F6192A_ID;
		#elif defined(RD_88F6190A)
		tmpBoardId = RD_88F6190A_ID;
		#elif defined(DB_88F6180A)
		tmpBoardId = DB_88F6180A_BP_ID;
		#elif defined(RD_88F6281A_PCAC)
		tmpBoardId = RD_88F6281A_PCAC_ID;
		#elif defined(RD_88F6281A_SHEEVA_PLUG)
		tmpBoardId = SHEEVA_PLUG_ID;
		#elif defined(DB_CUSTOMER)
		tmpBoardId = DB_CUSTOMER_ID;
		#endif
		gBoardId = tmpBoardId;
	}

	return gBoardId;
}

/*******************************************************************************
* mvBoarModuleTypeGet - mvBoarModuleTypeGet
*
* DESCRIPTION:
*
* INPUT:
*		group num - MV_BOARD_MPP_GROUP_CLASS enum
*
* OUTPUT:
*		None.
*
* RETURN:
*		module num - MV_BOARD_MODULE_CLASS enum
*
*******************************************************************************/
MV_BOARD_MODULE_ID_CLASS mvBoarModuleTypeGet(MV_BOARD_MPP_GROUP_CLASS devClass)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U8 data;

	/* TWSI init */
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, kirkwood_tclk, &slave, 0);

	/* Read MPP module ID */
	DB(mvOsPrintf("Board: Read MPP module ID\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(devClass);
	twsiSlave.slaveAddr.type = mvBoardTwsiExpAddrTypeGet(devClass);
	twsiSlave.validOffset = MV_TRUE;
	/* Offset is the first command after the address which indicate the register number to be read
	   in next operation */
	twsiSlave.offset = 0;
	twsiSlave.moreThen256 = MV_FALSE;

	if( MV_OK != mvTwsiRead (0, &twsiSlave, &data, 1) )
	{
		DB(mvOsPrintf("Board: Read MPP module ID fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Read MPP module ID succeded\n"));

	return data;
}
