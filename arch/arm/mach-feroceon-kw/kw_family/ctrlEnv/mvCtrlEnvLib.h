#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvCtrlEnvLibh
#define __INCmvCtrlEnvLibh

#include "mvSysHwConfig.h"
#include "mvCommon.h"
#include "mvTypes.h"
#include "mvOs.h"
#include "boardEnv/mvBoardEnvLib.h"			
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"
#include "ctrlEnv/mvCtrlEnvAddrDec.h"

typedef enum _mvCachePolicy
{
    NO_COHERENCY,    
    WT_COHERENCY,    
    WB_COHERENCY        
}MV_CACHE_POLICY;

typedef enum _mvSwapType
{
    MV_BYTE_SWAP,                     
    MV_NO_SWAP,          
    MV_BYTE_WORD_SWAP,   
    MV_WORD_SWAP,        
    SWAP_TYPE_MAX	 
}MV_SWAP_TYPE;

typedef enum _mvAccessRights
{
    	NO_ACCESS_ALLOWED = 0,   
    	READ_ONLY         = 1,   
	ACC_RESERVED	  = 2,	 
    	FULL_ACCESS       = 3,   
	MAX_ACC_RIGHTS
}MV_ACCESS_RIGHTS;

#if defined(MY_ABC_HERE)
MV_STATUS SYNOMppCtrlRegWrite(MV_U32 mppPin, MV_U32 mppVal);
#endif

MV_STATUS mvCtrlEnvInit(MV_VOID);
MV_U32    mvCtrlMppRegGet(MV_U32 mppGroup);

#if defined(MV_INCLUDE_PEX)
MV_U32	  mvCtrlPexMaxIfGet(MV_VOID);
#else
#define   mvCtrlPexMaxIfGet()	(0)
#endif

#define   mvCtrlPciIfMaxIfGet()	(0)

#if defined(MV_INCLUDE_GIG_ETH) 
MV_U32	  mvCtrlEthMaxPortGet(MV_VOID);
#endif
#if defined(MV_INCLUDE_XOR)
MV_U32 mvCtrlXorMaxChanGet(MV_VOID);
#endif
#if defined(MV_INCLUDE_USB)
MV_U32 	  mvCtrlUsbMaxGet(MV_VOID);
#endif
#if defined(MV_INCLUDE_NAND)
MV_U32	  mvCtrlNandSupport(MV_VOID);
#endif
#if defined(MV_INCLUDE_SDIO)
MV_U32	  mvCtrlSdioSupport(MV_VOID);
#endif
#if defined(MV_INCLUDE_TS)
MV_U32	  mvCtrlTsSupport(MV_VOID);
#endif
#if defined(MV_INCLUDE_AUDIO)
MV_U32	  mvCtrlAudioSupport(MV_VOID);
#endif
#if defined(MV_INCLUDE_TDM)
MV_U32	  mvCtrlTdmSupport(MV_VOID);
#endif

MV_U16    mvCtrlModelGet(MV_VOID);
MV_U8     mvCtrlRevGet(MV_VOID);
MV_STATUS mvCtrlNameGet(char *pNameBuff);
MV_U32    mvCtrlModelRevGet(MV_VOID);
MV_STATUS mvCtrlModelRevNameGet(char *pNameBuff);
MV_VOID   mvCtrlAddrDecShow(MV_VOID);
const MV_8* mvCtrlTargetNameGet(MV_TARGET target);
MV_U32	  ctrlSizeToReg(MV_U32 size, MV_U32 alignment);
MV_U32	  ctrlRegToSize(MV_U32 regSize, MV_U32 alignment);
MV_U32	  ctrlSizeRegRoundUp(MV_U32 size, MV_U32 alignment);
MV_U32	  mvCtrlSysRstLengthCounterGet(MV_VOID);
MV_STATUS ctrlWinOverlapTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2);
MV_STATUS ctrlWinWithinWinTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2);

MV_VOID   mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable);
MV_BOOL	  mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index);
MV_VOID   mvCtrlPwrMemSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable);
MV_BOOL	  mvCtrlIsBootFromSPI(MV_VOID);
MV_BOOL	  mvCtrlIsBootFromSPIUseNAND(MV_VOID);
MV_BOOL	  mvCtrlIsBootFromNAND(MV_VOID);
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
MV_VOID   mvCtrlPwrSaveOn(MV_VOID);
MV_VOID   mvCtrlPwrSaveOff(MV_VOID);
#endif
MV_BOOL	  mvCtrlPwrMemGet(MV_UNIT_ID unitId, MV_U32 index);
MV_VOID   mvMPPConfigToSPI(MV_VOID);
MV_VOID   mvMPPConfigToDefault(MV_VOID);

#endif  
