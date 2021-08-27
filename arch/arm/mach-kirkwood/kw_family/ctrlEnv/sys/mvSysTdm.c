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
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/mbus.h>
#include "common.h"

#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"

#ifdef MV_TDM_SUPPORT
#include "voiceband/tdm/mvTdm.h"
#else
 #include "voiceband/commUnit/mvCommUnit.h"
 #include "gpp/mvGpp.h"
#endif

#include "voiceband/mvSysTdmSpi.h"
#include "spi/mvSpiCmnd.h"
#include "spi/mvSpi.h"

#define MAX_DATA_LENGTH			255
#define MV_SPI_DEFAULT_BAUD_RATE	(10 << 20)

#define WINDOW_CTRL(i)		(0x4030 + ((i) << 4))
#define WINDOW_BASE(i)		(0x4034 + ((i) << 4))

static void mv_conf_mbus_windows(void __iomem *base,
				 const struct mbus_dram_target_info *dram)
{
	int i;

	for (i = 0; i < 4; i++) {
		writel(0, base + WINDOW_CTRL(i));
		writel(0, base + WINDOW_BASE(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel(((cs->size - 1) & 0xffff0000) |
			(cs->mbus_attr << 8) |
			(dram->mbus_dram_target_id << 4) | 1,
			base + WINDOW_CTRL(i));
		writel(cs->base, base + WINDOW_BASE(i));
	}
}

MV_STATUS mvSysTdmInit(MV_TDM_PARAMS* tdmParams)
{
	MV_TDM_HAL_DATA halData;
	MV_STATUS status;
	const struct mbus_dram_target_info *dram;

#ifdef MV_TDM_SUPPORT
	dram = mv_mbus_dram_info();
	mv_conf_mbus_windows((void __iomem *)TDM_PHYS_BASE, dram);
#else
	mvCommUnitWinInit();
#endif

	halData.spiMode = mvBoardTdmSpiModeGet();
	halData.model = mvCtrlModelGet();
	halData.ctrlRev = mvCtrlRevGet();
#if defined(MV_TDM_PCM_CLK_8MHZ)
	halData.frameTs = MV_FRAME_128TS;
	halData.pcmFreq = PCM_8192KHZ;
#elif defined(MV_TDM_PCM_CLK_4MHZ)
	halData.frameTs = MV_FRAME_64TS;
	halData.pcmFreq = PCM_4096KHZ;
#else /* MV_TDM_PCM_CLK_2MHZ */
	halData.frameTs = MV_FRAME_32TS;
	halData.pcmFreq = PCM_2048KHZ;
#endif

#ifdef MV_TDM_SUPPORT
	status = mvTdmHalInit (tdmParams, &halData);
#else
	halData.maxCs = mvBoardTdmDevicesCountGet();
	status = mvCommUnitHalInit (tdmParams, &halData);

	/* Issue SLIC reset */
	mvGppValueSet(0, BIT25, 0);
	mvOsUDelay(60);
	mvGppValueSet(0, BIT25, BIT25);
#endif

	if (mvSpiInit(MV_SPI_DEFAULT_BAUD_RATE, kirkwood_tclk) != MV_OK)
		mvOsPrintf("Error, unable to initialize SPI HAL driver\n");

	return status;
}

MV_VOID mvSysTdmSpiRead(MV_U16 lineId, MV_U8* cmdBuff, MV_U8 cmdSize, MV_U8* dataBuff, MV_U8 dataSize)
{
#if defined(MV_TDM_SUPPORT) && !defined(ZARLINK_SLIC_SUPPORT) && !defined(SILABS_SLIC_SUPPORT)
	if((cmdSize > 4) || (dataSize > MAX_DATA_LENGTH))
	{
		mvOsPrintf("Error, exceeded max size of command(%d) or data(%d)\n", cmdSize, dataSize);
		return;
	}

	mvTdmSpiRead(cmdBuff, cmdSize, dataBuff, dataSize, lineId);

#else

	/* Set SPI parameters for SLIC */
	mvSpiParamsSet(SPI_TYPE_SLIC);

	if(MV_OK != mvSpiWriteThenRead (cmdBuff, cmdSize, dataBuff, dataSize, 0))
		mvOsPrintf("SPI read failed !!!\n");

	/* Restore SPI parameters to FLASH */
	mvSpiParamsSet(SPI_TYPE_FLASH);

#endif
}

/*******************************************************************************
* mvSysTdmSpiWrite - telephony register write via SPI interface
*
* DESCRIPTION:
*
* INPUT:
*       None
* OUTPUT:
*		None
* RETURN:
*       None
*
*******************************************************************************/
MV_VOID mvSysTdmSpiWrite(MV_U16 lineId, MV_U8* cmdBuff, MV_U8 cmdSize, MV_U8* dataBuff, MV_U8 dataSize)
{
#if defined(MV_TDM_SUPPORT) && !defined(ZARLINK_SLIC_SUPPORT) && !defined(SILABS_SLIC_SUPPORT)

	if((cmdSize > 3) || (dataSize > MAX_DATA_LENGTH))
	{
		mvOsPrintf("Error, exceeded max size of command(%d) or data(%d)\n", cmdSize, dataSize);
		return;
	}

	mvTdmSpiWrite(cmdBuff, cmdSize, dataBuff, dataSize, lineId);

#else /* MV_COMM_UNIT_SUPPORT || ZARLINK_SLIC_SUPPORT || SILABS_SLIC_SUPPORT*/

	/* Set SPI parameters for SLIC */
	mvSpiParamsSet(SPI_TYPE_SLIC);

	if(MV_OK != mvSpiWriteThenWrite (cmdBuff, cmdSize, dataBuff, dataSize))
		printk("SPI write failed !!!\n");

	/* Restore SPI parameters to FLASH */
	mvSpiParamsSet(SPI_TYPE_FLASH);

#endif /* MV_TDM_SUPPORT */
}

/*******************************************************************************
* mvSysTdmIntEnable - Enable CSLAC device interrupts.
*
* DESCRIPTION:
*
* INPUT:
*       Device ID
* OUTPUT:
*		None
* RETURN:
*       None
*
*******************************************************************************/
MV_VOID mvSysTdmIntEnable(MV_U8 deviceId)
{
#if defined(MV_TDM_SUPPORT)

	mvTdmIntEnable();

#else /* MV_COMM_UNIT_SUPPORT */

	mvCommUnitIntEnable(deviceId);

#endif
}

/*******************************************************************************
* mvSysTdmIntDisable - Disable CSLAC device interrupts.
*
* DESCRIPTION:
*
* INPUT:
*       Device ID
* OUTPUT:
*	None
* RETURN:
*       None
*
*******************************************************************************/
MV_VOID mvSysTdmIntDisable(MV_U8 deviceId)
{
#if defined(MV_TDM_SUPPORT)

	mvTdmIntDisable();

#else /* MV_COMM_UNIT_SUPPORT */

	mvCommUnitIntDisable(deviceId);

#endif
}
