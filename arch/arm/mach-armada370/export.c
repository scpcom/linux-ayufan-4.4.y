 
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "mvDebug.h"
#include "mvSysHwConfig.h"
#include "pex/mvPexRegs.h"
#include "cntmr/mvCntmr.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mvOs.h"

extern u32 mvTclk;
extern u32 mvSysclk;

EXPORT_SYMBOL(mv_early_printk);
EXPORT_SYMBOL(mvCtrlPwrClckGet);
EXPORT_SYMBOL(mvCtrlModelRevGet);
EXPORT_SYMBOL(mvTclk);
EXPORT_SYMBOL(mvSysclk);
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
EXPORT_SYMBOL(mvCtrlEthMaxPortGet);
EXPORT_SYMBOL(mvCtrlTargetNameGet);
EXPORT_SYMBOL(mvBoardIdGet);
EXPORT_SYMBOL(mvBoardPhyAddrGet);
EXPORT_SYMBOL(mvCpuIfTargetWinGet);
EXPORT_SYMBOL(mvMacStrToHex);
EXPORT_SYMBOL(mvBoardTclkGet);
EXPORT_SYMBOL(mvBoardMacSpeedGet);
EXPORT_SYMBOL(mvWinOverlapTest);
EXPORT_SYMBOL(mvCtrlAddrWinMapBuild);
EXPORT_SYMBOL(mvBoardTdmSpiModeGet);
EXPORT_SYMBOL(mvBoardTdmSpiCsGet);
EXPORT_SYMBOL(mvBoardTdmDevicesCountGet);

#include "spi/mvSpiCmnd.h"
EXPORT_SYMBOL(mvSpiWriteThenWrite);
EXPORT_SYMBOL(mvSpiWriteThenRead);
#include "spi/mvSpi.h"
EXPORT_SYMBOL(mvSpiParamsSet);
#include "gpp/mvGpp.h"
EXPORT_SYMBOL(mvGppValueSet);

#if defined(MV_INCLUDE_TDM)
EXPORT_SYMBOL(mvCtrlTdmUnitIrqGet);
EXPORT_SYMBOL(mvCtrlTdmUnitTypeGet);
#endif

#ifdef CONFIG_MV_INCLUDE_AUDIO
#include "audio/mvAudio.h"
#include "mvSysAudioApi.h"
EXPORT_SYMBOL(mvSPDIFRecordTclockSet);
EXPORT_SYMBOL(mvSPDIFPlaybackCtrlSet);
EXPORT_SYMBOL(mvI2SPlaybackCtrlSet);
EXPORT_SYMBOL(mvAudioPlaybackControlSet);
EXPORT_SYMBOL(mvAudioDCOCtrlSet);
EXPORT_SYMBOL(mvI2SRecordCntrlSet);
EXPORT_SYMBOL(mvAudioRecordControlSet);
EXPORT_SYMBOL(mvSysAudioInit);
#endif

#ifdef CONFIG_MV_INCLUDE_USB
extern u32 mvIsUsbHost;

#include "usb/mvUsb.h"
EXPORT_SYMBOL(mvIsUsbHost);
EXPORT_SYMBOL(mvCtrlUsbMaxGet);
EXPORT_SYMBOL(mvUsbGetCapRegAddr);
#ifdef MV_USB_VOLTAGE_FIX
EXPORT_SYMBOL(mvUsbGppInit);
EXPORT_SYMBOL(mvUsbBackVoltageUpdate);
#endif
#endif  

#ifdef CONFIG_MV_INCLUDE_CESA
#include "mvSysCesaApi.h"
#include "cesa/mvCesa.h"
#include "cesa/mvMD5.h"
#include "cesa/mvSHA1.h"
extern unsigned char*  mv_sram_usage_get(int* sram_size_ptr);

EXPORT_SYMBOL(mvSysCesaInit);
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
EXPORT_SYMBOL(mv_sram_usage_get);

extern u32 mv_crypto_virt_base_get(void);
extern u32 mv_crypto_phys_base_get(void);
EXPORT_SYMBOL(mv_crypto_virt_base_get);
EXPORT_SYMBOL(mv_crypto_phys_base_get);
EXPORT_SYMBOL(cesaReqResources);
EXPORT_SYMBOL(mvCesaFinish);

#endif

#if defined (CONFIG_MV_INCLUDE_SPI)
#include <sflash/mvSFlash.h>
#include <sflash/mvSFlashSpec.h>
EXPORT_SYMBOL(mvSFlashInit);
EXPORT_SYMBOL(mvSFlashSectorErase);
EXPORT_SYMBOL(mvSFlashChipErase);
EXPORT_SYMBOL(mvSFlashBlockRd);
EXPORT_SYMBOL(mvSFlashBlockWr);
EXPORT_SYMBOL(mvSFlashIdGet);
EXPORT_SYMBOL(mvSFlashWpRegionSet);
EXPORT_SYMBOL(mvSFlashWpRegionGet);
EXPORT_SYMBOL(mvSFlashStatRegLock);
EXPORT_SYMBOL(mvSFlashSizeGet);
EXPORT_SYMBOL(mvSFlashPowerSaveEnter);
EXPORT_SYMBOL(mvSFlashPowerSaveExit);
EXPORT_SYMBOL(mvSFlashModelGet);
#endif

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
#include <sata/CoreDriver/mvSata.h>
EXPORT_SYMBOL(mvSataWinInit);
#endif

#if (defined (CONFIG_MV_XOR_MEMCOPY) || defined (CONFIG_MV_IDMA_MEMCOPY)) && !defined(CONFIG_SYNO_ARMADA_ARCH)
EXPORT_SYMBOL(asm_memcpy);
#endif

#ifdef CONFIG_MV_SP_I_FTCH_DB_INV
EXPORT_SYMBOL(mv_l2_inv_range);
#endif

#ifdef CONFIG_MV_DBG_TRACE
#include "dbg-trace.h"
EXPORT_SYMBOL(TRC_INIT);
EXPORT_SYMBOL(TRC_REC);
EXPORT_SYMBOL(TRC_OUTPUT);
EXPORT_SYMBOL(TRC_START);
EXPORT_SYMBOL(TRC_RELEASE);
#endif
