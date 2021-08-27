/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/export.h>

#include "ctrlEnv/mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "mvDebug.h"
#include "mvSysHwConfig.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mvOs.h"

/*************************************************************************************************************
 * Environment
 *************************************************************************************************************/
EXPORT_SYMBOL(mvCtrlPwrClckGet);
EXPORT_SYMBOL(mvCtrlModelRevGet);
EXPORT_SYMBOL(mvCtrlModelGet);
EXPORT_SYMBOL(mvOsIoUncachedMalloc);
EXPORT_SYMBOL(mvOsIoUncachedFree);
EXPORT_SYMBOL(mvOsIoCachedMalloc);
EXPORT_SYMBOL(mvOsIoCachedFree);
EXPORT_SYMBOL(mvDebugMemDump);
EXPORT_SYMBOL(mvHexToBin);
EXPORT_SYMBOL(mvBinToHex);
EXPORT_SYMBOL(mvSizePrint);
EXPORT_SYMBOL(mvDebugPrintMacAddr);
EXPORT_SYMBOL(mvCtrlTargetNameGet);
EXPORT_SYMBOL(mvBoardIdGet);
EXPORT_SYMBOL(mvBoardPhyAddrGet);
EXPORT_SYMBOL(mvMacStrToHex);
EXPORT_SYMBOL(mvBoardMacSpeedGet);

/*************************************************************************************************************
 * USB
 *************************************************************************************************************/
#ifdef CONFIG_MV_INCLUDE_USB
extern u32 mvIsUsbHost;

#include "usb/mvUsb.h"
EXPORT_SYMBOL(mvIsUsbHost);
EXPORT_SYMBOL(mvCtrlUsbMaxGet);
EXPORT_SYMBOL(mvUsbGetCapRegAddr);
EXPORT_SYMBOL(mvUsbGppInit);
EXPORT_SYMBOL(mvUsbBackVoltageUpdate);
#endif /* CONFIG_MV_INCLUDE_USB */

/*************************************************************************************************************
 * CESA
 *************************************************************************************************************/
#ifdef CONFIG_MV_INCLUDE_CESA
#include "cesa/mvCesa.h"
#include "cesa/mvMD5.h"
#include "cesa/mvSHA1.h"

#ifdef CONFIG_USB_MRVL
extern unsigned char*  mv_sram_usage_get(int* sram_size_ptr);
EXPORT_SYMBOL(mv_sram_usage_get);
#endif

EXPORT_SYMBOL(mvCesaHalInit);
EXPORT_SYMBOL(mvCesaSessionOpen);
EXPORT_SYMBOL(mvCesaSessionClose);
EXPORT_SYMBOL(mvCesaAction);
EXPORT_SYMBOL(mvCesaReadyGet);
EXPORT_SYMBOL(mvCesaCopyFromMbuf);
EXPORT_SYMBOL(mvCesaCopyToMbuf);
EXPORT_SYMBOL(mvCesaMbufCopy);
EXPORT_SYMBOL(mvCesaCryptoIvSet);
EXPORT_SYMBOL(mvMD5);
EXPORT_SYMBOL(mvSHA1);

EXPORT_SYMBOL(mvCesaDebugQueue);
EXPORT_SYMBOL(mvCesaDebugSram);
EXPORT_SYMBOL(mvCesaDebugSAD);
EXPORT_SYMBOL(mvCesaDebugStatus);
EXPORT_SYMBOL(mvCesaDebugMbuf);
EXPORT_SYMBOL(mvCesaDebugSA);

EXPORT_SYMBOL(cesaReqResources);
EXPORT_SYMBOL(mvCesaFinish);
#endif /* CONFIG_MV_INCLUDE_CESA */

#ifdef CONFIG_MV_CESA_OCF
extern void cesa_ocf_debug(void);
EXPORT_SYMBOL(cesa_ocf_debug);
#endif

/*************************************************************************************************************
 * Networking
 *************************************************************************************************************/
#ifdef CONFIG_MV_INCLUDE_GIG_ETH
#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"
EXPORT_SYMBOL(mvCtrlEthMaxPortGet);
EXPORT_SYMBOL(mvEthPhyRegRead);
EXPORT_SYMBOL(mvEthPhyRegWrite);
#endif

/*************************************************************************************************************
 * Marvell TRACE
 *************************************************************************************************************/
#ifdef CONFIG_MV_DBG_TRACE
#include "dbg-trace.h"
EXPORT_SYMBOL(TRC_INIT);
EXPORT_SYMBOL(TRC_REC);
EXPORT_SYMBOL(TRC_OUTPUT);
#endif
