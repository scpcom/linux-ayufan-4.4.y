#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvSFlashH
#define __INCmvSFlashH

#ifdef __cplusplus
extern "C" {
#endif

#include "ctrlEnv/mvCtrlEnvSpec.h"

#define MV_SFLASH_PAGE_ALLIGN_MASK(pgSz)    (pgSz-1)
#define MV_ARRAY_SIZE(a)                    ((sizeof(a)) / (sizeof(a[0])))


#define MV_INVALID_DEVICE_NUMBER            0xFFFFFFFF
 
#define MV_SFLASH_BASIC_SPI_FREQ            10000000
 
typedef enum {
	MV_WP_NONE,              
#ifdef CONFIG_SYNO_ARMADA
	MV_WP_UPR_1OF256,        
#endif
	MV_WP_UPR_1OF128,        
	MV_WP_UPR_1OF64,         
	MV_WP_UPR_1OF32,         
	MV_WP_UPR_1OF16,         
	MV_WP_UPR_1OF8,          
	MV_WP_UPR_1OF4,          
	MV_WP_UPR_1OF2,          
	MV_WP_ALL                
} MV_SFLASH_WP_REGION;

typedef struct {
    MV_U8   opcdWREN;        
    MV_U8   opcdWRDI;        
    MV_U8   opcdRDID;        
    MV_U8   opcdRDSR;        
    MV_U8   opcdWRSR;        
    MV_U8   opcdREAD;        
    MV_U8   opcdFSTRD;       
    MV_U8   opcdPP;          
    MV_U8   opcdSE;          
    MV_U8   opcdBE;          
    MV_U8   opcdRES;         
    MV_U8   opcdPwrSave;     
    MV_U32  sectorSize;      
    MV_U32  sectorNumber;    
    MV_U32  pageSize;        
    const char *deviceModel;     
    MV_U32  manufacturerId;  
    MV_U32  deviceId;        
    MV_U32  spiMaxFreq;      
    MV_U32  spiMaxFastFreq;  
    MV_U32  spiFastRdDummyBytes;  
    MV_U32  addrCycCnt;		 
} MV_SFLASH_DEVICE_PARAMS;

typedef struct {
	MV_U32					baseAddr;        
	MV_U8	                manufacturerId;	 
	MV_U16	                deviceId;	     
	MV_U32                  sectorSize;      
	MV_U32                  sectorNumber;    
	MV_U32                  pageSize;        
	MV_U32                  index;           
} MV_SFLASH_INFO;

MV_STATUS	mvSFlashInit(MV_SFLASH_INFO *pFlinfo);

MV_STATUS 	mvSFlashSectorErase(MV_SFLASH_INFO *pFlinfo, MV_U32 secNumber);
MV_STATUS 	mvSFlashChipErase(MV_SFLASH_INFO *pFlinfo);

MV_STATUS	mvSFlashBlockRd(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
						MV_U8 *pReadBuff, MV_U32 buffSize);
MV_STATUS mvSFlashFastBlockRd(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
							     MV_U8 *pReadBuff, MV_U32 buffSize);

MV_STATUS	mvSFlashBlockWr(MV_SFLASH_INFO *pFlinfo, MV_U32 offset,
							     MV_U8 *pWriteBuff, MV_U32 buffSize);
 
MV_STATUS 	mvSFlashIdGet(MV_SFLASH_INFO *pFlinfo, MV_U8 *pManId, MV_U16 *pDevId);

MV_STATUS   mvSFlashWpRegionSet(MV_SFLASH_INFO *pFlinfo, MV_SFLASH_WP_REGION wpRegion);
MV_STATUS   mvSFlashWpRegionGet(MV_SFLASH_INFO *pFlinfo, MV_SFLASH_WP_REGION *pWpRegion);

MV_STATUS   mvSFlashStatRegLock(MV_SFLASH_INFO *pFlinfo, MV_BOOL srLock);

MV_U32      mvSFlashSizeGet(MV_SFLASH_INFO *pFlinfo);

MV_STATUS   mvSFlashPowerSaveEnter(MV_SFLASH_INFO *pFlinfo);
MV_STATUS   mvSFlashPowerSaveExit(MV_SFLASH_INFO *pFlinfo);

const MV_8 *mvSFlashModelGet(MV_SFLASH_INFO *pFlinfo);

#ifdef __cplusplus
}
#endif

#endif  
