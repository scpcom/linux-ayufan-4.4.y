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

#include "mvCommon.h"
#include "mvCtrlEnvLib.h"
#include "mvDeviceId.h"

#ifdef MV_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

/*******************************************************************************
* mvCtrlEnvInit - Initialize Marvell controller environment.
*
* DESCRIPTION:
*       This function get environment information and initialize controller
*       internal/external environment. For example
*       1) MPP settings according to board MPP macros.
*		NOTE: It is the user responsibility to shut down all DMA channels
*		in device and disable controller sub units interrupts during
*		boot process.
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
MV_STATUS mvCtrlEnvInit(MV_VOID)
{
	MV_U32 mppGroup;
	MV_U32 devId;
	MV_U32 boardId;
	MV_U32 i;
	MV_U32 maxMppGrp = 1;
	MV_U32 mppVal = 0;
	MV_U32 bootVal = 0;
	MV_U32 mppGroupType = 0;
	MV_U32 mppGroup1[][3] = MPP_GROUP_1_TYPE;
	MV_U32 mppGroup2[][3] = MPP_GROUP_2_TYPE;

	devId = mvCtrlModelGet();
	boardId= mvBoardIdGet();

	switch(devId){
        case MV_6281_DEV_ID:
            maxMppGrp = MV_6281_MPP_MAX_GROUP;
            break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_GROUP;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_GROUP;
            break;
        case MV_6192_DEV_ID:
        case MV_6701_DEV_ID:
        case MV_6702_DEV_ID:
            maxMppGrp = MV_6192_MPP_MAX_GROUP;
            break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_GROUP;
            break;
        case MV_6180_DEV_ID:
            maxMppGrp = MV_6180_MPP_MAX_GROUP;
            break;
	}

	/* MPP Init */
	/* We split mpp init to 3 phases:
	 * 1. We init mpp[19:0] from the board info. mpp[23:20] will be over write
	 * in phase 2.
	 * 2. We detect the mpp group type and according the mpp values [35:20].
	 * 3. We detect the mpp group type and according the mpp values [49:36].
	 */
	/* Mpp phase 1 mpp[19:0] */
	/* Read MPP group from board level and assign to MPP register */
	for (mppGroup = 0; mppGroup < 3; mppGroup++)
	{
		mppVal = mvBoardMppGet(mppGroup);
		if (mppGroup == 0)
		{
		    bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
		    if (mvCtrlIsBootFromSPI())
		    {
			mppVal &= ~0xffff;
			bootVal &= 0xffff;
			mppVal |= bootVal;
		    }
		    else if (mvCtrlIsBootFromSPIUseNAND())
		    {
			mppVal &= ~0xf0000000;
			bootVal &= 0xf0000000;
			mppVal |= bootVal;
		    }
		    else if (mvCtrlIsBootFromNAND())
		    {
			mppVal &= ~0xffffff;
			bootVal &= 0xffffff;
			mppVal |= bootVal;
		    }
		}

		if (mppGroup == 2)
		{
		    bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
		    if (mvCtrlIsBootFromNAND())
		    {
			mppVal &= ~0xff00;
			bootVal &= 0xff00;
			mppVal |= bootVal;
		    }
		}

		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

	/* Identify MPPs group */
	mvBoardMppGroupIdUpdate();

	/* Update MPPs mux relevent only on Marvell DB */
	if ((boardId == DB_88F6281A_BP_ID) ||
        (boardId == DB_88F6282A_BP_ID) ||
		(boardId == DB_88F6180A_BP_ID))
		mvBoardMppMuxSet();

	mppGroupType = mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_1);

	/* Mpp phase 2 */
	/* Read MPP group from board level and assign to MPP register */
    if (devId != MV_6180_DEV_ID && devId != MV_6280_DEV_ID)
    {
        i = 0;
	for (mppGroup = 2; mppGroup < 5; mppGroup++)
	{
		if ((mppGroupType == MV_BOARD_OTHER) ||
			(boardId == RD_88F6281A_ID) ||
			(boardId == RD_88F6282A_ID) ||
			(boardId == RD_88F6192A_ID) ||
                (boardId == RD_88F6190A_ID) ||
                (boardId == RD_88F6281A_PCAC_ID) ||
                (boardId == SHEEVA_PLUG_ID))
			mppVal = mvBoardMppGet(mppGroup);
		else
		{
			mppVal = mppGroup1[mppGroupType][i];
			i++;
		}

		/* Group 2 is shared mpp[23:16] */
		if (mppGroup == 2)
		{
                bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
			mppVal &= ~0xffff;
			bootVal &= 0xffff;
			mppVal |= bootVal;
		}

		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}
    }

        if ((devId == MV_6192_DEV_ID) || (devId == MV_6190_DEV_ID) ||
            (devId == MV_6701_DEV_ID) || (devId == MV_6702_DEV_ID))
		return MV_OK;

	/* Mpp phase 3 */
	mppGroupType = mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_2);
	/* Read MPP group from board level and assign to MPP register */
	i = 0;
	for (mppGroup = 4; mppGroup < 7; mppGroup++)
	{
		if ((mppGroupType == MV_BOARD_OTHER) ||
			(boardId == RD_88F6281A_ID) ||
			(boardId == RD_88F6282A_ID) ||
            (boardId == RD_88F6281A_PCAC_ID) ||
            (boardId == SHEEVA_PLUG_ID))
			mppVal = mvBoardMppGet(mppGroup);
		else
		{
			mppVal = mppGroup2[mppGroupType][i];
			i++;
		}

		/* Group 4 is shared mpp[35:32] */
		if (mppGroup == 4)
		{
            bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
			mppVal &= ~0xffff;
			bootVal &= 0xffff;
			mppVal |= bootVal;
		}

		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

    /* Update SSCG configuration register*/
    if (mvBoardIdGet() == DB_88F6281A_BP_ID ||
        mvBoardIdGet() == DB_88F6282A_BP_ID ||
        mvBoardIdGet() == DB_88F6192A_BP_ID ||
        mvBoardIdGet() == DB_88F6701A_BP_ID ||
        mvBoardIdGet() == DB_88F6702A_BP_ID ||
        mvBoardIdGet() == DB_88F6190A_BP_ID ||
        mvBoardIdGet() == DB_88F6180A_BP_ID ||
        mvBoardIdGet() == DB_88F6280A_BP_ID)
        MV_REG_WRITE(0x100d8, 0x53);

    return MV_OK;
}

/*******************************************************************************
* mvCtrlMppRegGet - return reg address of mpp group
*
* DESCRIPTION:
*
* INPUT:
*       mppGroup - MPP group.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_U32 - Register address.
*
*******************************************************************************/
MV_U32 mvCtrlMppRegGet(MV_U32 mppGroup)
{
        MV_U32 ret;

        switch(mppGroup){
                case (0):       ret = MPP_CONTROL_REG0;
                                break;
                case (1):       ret = MPP_CONTROL_REG1;
                                break;
                case (2):       ret = MPP_CONTROL_REG2;
                                break;
                case (3):       ret = MPP_CONTROL_REG3;
                                break;
                case (4):       ret = MPP_CONTROL_REG4;
                                break;
                case (5):       ret = MPP_CONTROL_REG5;
                                break;
                case (6):       ret = MPP_CONTROL_REG6;
                                break;
                default:        ret = MPP_CONTROL_REG0;
                                break;
        }
        return ret;
}

#if defined(MV_INCLUDE_GIG_ETH)
/*******************************************************************************
* mvCtrlEthMaxPortGet - Get Marvell controller number of etherent ports.
*
* DESCRIPTION:
*       This function returns Marvell controller number of etherent port.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of etherent port.
*
*******************************************************************************/
MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
	MV_U32 devId;

	devId = mvCtrlModelGet();

	switch(devId){
        case MV_6281_DEV_ID:
            return MV_6281_ETH_MAX_PORTS;
            break;
        case MV_6282_DEV_ID:
            return MV_6282_ETH_MAX_PORTS;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_ETH_MAX_PORTS;
            break;
        case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
            return MV_6192_ETH_MAX_PORTS;
            break;
        case MV_6190_DEV_ID:
            return MV_6190_ETH_MAX_PORTS;
            break;
        case MV_6180_DEV_ID:
            return MV_6180_ETH_MAX_PORTS;
            break;
	}
	return 0;
}
#endif

#if defined(MV_INCLUDE_TDM)
/*******************************************************************************
* mvCtrlTdmSupport - Return if this controller has integrated TDM flash support
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if TDM is supported and MV_FALSE otherwise
*
*******************************************************************************/
MV_U32	  mvCtrlTdmSupport(MV_VOID)
{
	MV_U32 devId;

	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_TDM;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_TDM;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_TDM;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_TDM;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_TDM;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_TDM;
			break;
	}
	return 0;

}
#endif

void kirkwood_pcie_id(u32 *dev, u32 *rev);

MV_U16 mvCtrlModelGet(MV_VOID)
{
	u32 dev, rev;
	kirkwood_pcie_id(&dev, &rev);
	return dev;
}

MV_U8 mvCtrlRevGet(MV_VOID)
{
	u32 dev, rev;
	kirkwood_pcie_id(&dev, &rev);
	return rev;
}

MV_U32	mvCtrlModelRevGet(MV_VOID)
{
	return ((mvCtrlModelGet() << 16) | mvCtrlRevGet());
}

static const char* cntrlName[] = TARGETS_NAME_ARRAY;

/*******************************************************************************
* mvCtrlTargetNameGet - Get Marvell controller target name
*
* DESCRIPTION:
*       This function convert the trget enumeration to string.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Target name (const MV_8 *)
*******************************************************************************/
const MV_8* mvCtrlTargetNameGet( MV_TARGET target )
{

	if (target >= MAX_TARGETS)
	{
		return "target unknown";
	}

	return cntrlName[target];
}

MV_BOOL	  mvCtrlIsBootFromSPI(MV_VOID)
{
    MV_U32 satr = 0;
    satr = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
    {
        if (MSAR_BOOT_MODE_6180(satr) == MSAR_BOOT_SPI_WITH_BOOTROM_6180)
            return MV_TRUE;
        else
            return MV_FALSE;
    }
    satr = satr & MSAR_BOOT_MODE_MASK;
    if (satr == MSAR_BOOT_SPI_WITH_BOOTROM)
        return MV_TRUE;
    else
        return MV_FALSE;
}

MV_BOOL	  mvCtrlIsBootFromSPIUseNAND(MV_VOID)
{
    MV_U32 satr = 0;
    if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
        return MV_FALSE;
    satr = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    satr = satr & MSAR_BOOT_MODE_MASK;

    if (satr == MSAR_BOOT_SPI_USE_NAND_WITH_BOOTROM)
        return MV_TRUE;
    else
        return MV_FALSE;
}

MV_BOOL	  mvCtrlIsBootFromNAND(MV_VOID)
{
    MV_U32 satr = 0;
    satr = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
    {
        if (MSAR_BOOT_MODE_6180(satr) == MSAR_BOOT_NAND_WITH_BOOTROM_6180)
            return MV_TRUE;
        else
            return MV_FALSE;
    }
    satr = satr & MSAR_BOOT_MODE_MASK;
    if ((satr == MSAR_BOOT_NAND_WITH_BOOTROM))
        return MV_TRUE;
    else
        return MV_FALSE;
}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
/*******************************************************************************
* mvCtrlPwrClckGet - Get Power State of specific Unit
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
******************************************************************************/
MV_BOOL		mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	switch (unitId)
    {
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & PMC_GESTOPCLOCK_MASK(index)) == PMC_GESTOPCLOCK_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_SESTOPCLOCK_MASK) == PMC_SESTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TS)
	case TS_UNIT_ID:
		if ((reg & PMC_TSSTOPCLOCK_MASK) == PMC_TSSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_UNIT_ID:
		if ((reg & PMC_TDMSTOPCLOCK_MASK) == PMC_TDMSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif

	default:
		state = MV_TRUE;
		break;
	}

	return state;
}

#else
MV_BOOL	  mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index) {return MV_TRUE;}
#endif /* #if defined(MV_INCLUDE_CLK_PWR_CNTRL) */

/*******************************************************************************
* mvMPPConfigToSPI - Change MPP[3:0] configuration to SPI mode
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
******************************************************************************/
MV_VOID   mvMPPConfigToSPI(MV_VOID)
{
	MV_U32 mppVal = 0;
	MV_U32 bootVal = 0;

    if(!mvCtrlIsBootFromSPIUseNAND())
        return;
    mppVal = 0x00002220; /* Set MPP [3:1] to SPI mode */
    bootVal = MV_REG_READ(mvCtrlMppRegGet(0));
    bootVal &= 0xffff000f;
        mppVal |= bootVal;

    MV_REG_WRITE(mvCtrlMppRegGet(0), mppVal);
}

/*******************************************************************************
* mvMPPConfigToDefault - Change MPP[7:0] configuration to default configuration
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
******************************************************************************/
MV_VOID mvMPPConfigToDefault(MV_VOID)
{
	MV_U32 mppVal = 0;
	MV_U32 bootVal = 0;

    if(!mvCtrlIsBootFromSPIUseNAND())
        return;
    mppVal = mvBoardMppGet(0);
    bootVal = MV_REG_READ(mvCtrlMppRegGet(0));
    mppVal &= ~0xffff000f;
    bootVal &= 0xffff000f;
        mppVal |= bootVal;

    MV_REG_WRITE(mvCtrlMppRegGet(0), mppVal);
}
