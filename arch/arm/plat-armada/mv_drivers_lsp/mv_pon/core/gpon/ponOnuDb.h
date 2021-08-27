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

******************************************************************************/

/******************************************************************************
**  FILE        : ponOnuDb.h                                                 **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON database definitions           **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_DB_H
#define _ONU_GPON_DB_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
/******************************************************************************/
/* ========================================================================== */
/*                               Database Definitions                         */
/* ========================================================================== */
/******************************************************************************/

/* GPON Definitions */
#define ONU_GPON_SN_LEN          (8)
#define ONU_GPON_PASS_LEN        (10)
#define ONU_GPON_PREAMBLE_LEN    (3)
#define ONU_GPON_DELIMITER_LEN   (3)
#define ONU_GPON_MSG_LEN         (3)  

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Typedefs
------------------------------------------------------------------------------*/
/******************************************************************************/
/* ========================================================================== */
/*                               Database Definitions                         */
/* ========================================================================== */
/******************************************************************************/

/**************************************/
/* ONU GPON functions prototype       */
/**************************************/
typedef void (*GPONFUNCPTR) (MV_U8, MV_U8, MV_U8*);

/**************************************/
/* ONU GPON general table             */
/**************************************/
typedef struct
{
  MV_U32      onuGponOnuState;                     /* ONU State */  
  MV_U32      onuGponOnuId;                        /* ONU ID */
  MV_U32      onuGponOnuIdOverrideEn;              /* ONU ID Overeide Enable*/
  MV_U32      onuGponOnuIdOverrideVal;             /* ONU ID Overeide value*/
  MV_U32	  onuGponRate;
  GPONFUNCPTR onuGponStateAndEventTbl[ONU_GPON_NUM_OF_EVENTS][ONU_GPON_NUM_OF_STATES];
  MV_BOOL	  onuGponInit;						   /* Is ONU initialized? */
  MV_BOOL     omccValid;                           
  MV_U32      omccPort;
  MV_U32      onuGponOmccPortOverrideVal;
  MV_U32      onuGponOmccPortOverrideEn;
  MV_BOOL     onuGponDyingGaspEn;

  ALARMNOTIFYFUNC   alarmCallback;
  STATUSNOTIFYFUNC  statusCallback;
  OMCCNOTIFYFUNC    omccCallback;
  DISABLENOTIFYFUNC disableCallback;
}S_OnuGponGenTbl;

/**************************************/
/* ONU GPON Sync param table          */
/**************************************/
typedef struct
{
  MV_U32  onuGponGuardBits;                         /* ONU number of guard bits */
  MV_U32  onuGponPreambleVal[ONU_GPON_PREAMBLE_LEN];/* ONU preamble value */
  MV_U32  onuGponPreambleCnt[ONU_GPON_PREAMBLE_LEN];/* ONU preamble count */
  MV_U32  onuGponDelimiter[ONU_GPON_DELIMITER_LEN]; /* ONU delimiter value */
  MV_U32  onuGponDelimiterSize;                     /* ONU delimiter size */
  MV_U32  onuGponDelimiterOverrideValue;            /* ONU delimiter value override */
  MV_BOOL onuGponDelimiterOverride;                 /* ONU delimiter override */

  MV_U32  onuGponExtPreambleStatus;                 /* ONU extended preamble status (on/off) */
  MV_U32  onuGponExtPreambleSync;                   /* ONU extended preamble type 3 - state 3/4 */
  MV_U32  onuGponExtPreambleOper;                   /* ONU extended preamble type 3 - state 5 */
  MV_U32  onuGponExtPreambleSyncOverride;           /* ONU extended preamble override type 3 - state 3/4 */
  MV_U32  onuGponExtPreambleOperOverride;           /* ONU extended preamble override type 3 - state 5 */
  MV_BOOL onuGponExtPreambleOverride;               /* ONU extended Preamble override */
  
  MV_U32  onuGponMaxExtraSnTrans;                   /* ONU max extra SN transmissions */
  
  MV_U8   onuGponSerialNum[ONU_GPON_SN_LEN];        /* ONU Serial number */  
  MV_U32  onuGponConstDelay;                        /* ONU Const Delay - cannot be changed */
  MV_U32  onuGponEqualizationDelay;                 /* ONU Equalization Delay */
  MV_U32  onuGponEqualizationDelayOverrideValue;    /* ONU delimiter override */
  MV_BOOL onuGponEqualizationDelayOverride;         /* ONU delimiter override */
  
  MV_U32  onuGponPowerLevel;                        /* ONU power level */
  
  MV_BOOL onuGponSerialNumberMaskEnable;            /* ONU Serial Number Mask */
  MV_BOOL onuGponSerialNumberMaskMatch;             /* ONU Serial Number Mask  Match Mode */
  MV_BOOL onuGponSerialNumberMaskDefEnable;         /* ONU Serial Number Mask profile override status */
  
  MV_U32  onuGponIdleMsg[ONU_GPON_MSG_LEN];         /* ONU idle message */
  MV_U32  onuGponSnMsg[ONU_GPON_MSG_LEN];           /* ONU serial number message */
}S_OnuGponSyncParamsTbl;                            

/**************************************/
/* ONU GPON Oper param table          */
/**************************************/
typedef struct
{
  MV_U32  onuGponBerInterval;                       /* ONU Ber Interval - set by the OLT */
  MV_U32  onuGponBerCalcInterval;                   /* ONU Calc Ber Interval - internal for ONU */
  MV_U32  onuGponSfThreshold;                       /* ONU Signal Fail Threshold */
  MV_U32  onuGponSdThreshold;                       /* ONU Signal degraded Threshold */
  MV_U32  onuGponBerIntervalValue;                  /* ONU Ber Interval Value of Bip8 Errors */
  MV_BOOL onuGponGemPortAes[GPON_ONU_MAX_GEM_PORTS];
  MV_BOOL onuGponGemPortValid[GPON_ONU_MAX_GEM_PORTS];
  MV_U8   onuGponPassword[ONU_GPON_PASS_LEN];       /* ONU password */
  MV_U16  onuGponReiSeqNum;                         /* ONU REI sequence number */
}S_OnuGponOperParamsTbl;

/**************************************/
/* ONU GPON BW Allocation param table */
/**************************************/
typedef struct
{
  MV_U32  allocId;
  MV_BOOL valid;
}S_OnuAllocIds;

typedef struct
{
  MV_BOOL exist;
  MV_U32  allocId;
  MV_BOOL valid;
}S_OnuTcontIds;

typedef struct
{
  S_OnuTcontIds onuTcontIds[ONU_GPON_MAX_NUM_OF_T_CONTS];
  S_OnuAllocIds onuAllocIds[ONU_GPON_MAX_NUM_OF_T_CONTS];
  MV_U32        onuIdleAllocIds[ONU_GPON_MAX_NUM_OF_T_CONTS];
}S_OnuGponBwAllocTbl;

/* ONU GPON Database */
typedef struct
{
  S_OnuGponGenTbl        onuGponGenTbl_s;
  S_OnuGponSyncParamsTbl onuGponSyncParamsTbl_s;
  S_OnuGponOperParamsTbl onuGponOperParamsTbl_s;
  S_OnuGponBwAllocTbl    onuGponBwTbl_s;
}S_OnuGponDb;

/* Global variables
------------------------------------------------------------------------------*/
/* ONU GPON Database */
extern S_OnuGponDb onuGponDb_s;

/* Global functions
------------------------------------------------------------------------------*/
/* ONU GPON database init function */
MV_STATUS onuGponDbInit(void);

void       onuGponOnuGenTblInit(void);
void       onuGponDbOnuSyncParamTblInit(void);
void       onuGponDbOnuOperParamTblInit(void);
void       onuGponDbOnuTcontTblInit(void);
void       onuGponDbOnuPortTblInit(void);
MV_STATUS onuGponDbBwAllocInit(void);

/* ONU GPON general table API functions */
MV_STATUS onuGponDbOnuStateSet(MV_U32 onuState);
MV_U32    onuGponDbOnuStateGet(void);
MV_STATUS onuGponDbOnuIdSet(MV_U32 onuId);
MV_U32    onuGponDbOnuIdGet(void);
MV_STATUS onuGponDbOnuIdOverrideSet(MV_BOOL enable);
MV_BOOL   onuGponDbOnuIdOverrideGet(void);
MV_STATUS onuGponDbOnuIdOverrideValueSet(MV_U32 onuId);
MV_U32    onuGponDbOnuIdOverrideValueGet(void);
MV_STATUS onuGponDbInitSet(MV_BOOL init);
MV_BOOL   onuGponDbInitGet(void);
MV_STATUS onuGponDbOmccValidSet(MV_BOOL omccValid);
MV_BOOL   onuGponDbOmccValidGet(void);
MV_STATUS onuGponDbOmccPortSet(MV_U32 omccPort);
MV_U32    onuGponDbOmccPortGet (void);
MV_BOOL   onuGponDbOmccPortOverrideGet(void);
MV_STATUS onuGponDbOmccPortOverrideSet(MV_BOOL enable);
MV_U32    onuGponDbOmccPortOverrideValueGet(void);
MV_STATUS onuGponDbOmccPortOverrideValueSet(MV_U32 omccPort);
MV_STATUS onuGponDbRateSet(MV_U32 rate);
MV_U32	  onuGponDbRateGet (void);
MV_BOOL   onuGponDbdGaspEnGet(void);
MV_STATUS onuGponDbdGaspEnSet(MV_BOOL dGaspEn);
MV_STATUS onuGponDbAlarmNotifySet(ALARMNOTIFYFUNC alarmCallback);
ALARMNOTIFYFUNC   onuGponDbAlarmNotifyGet (void);
MV_STATUS         onuGponDbStatusNotifySet(STATUSNOTIFYFUNC	statusCallback);
STATUSNOTIFYFUNC  onuGponDbStatusNotifyGet (void);
MV_STATUS         onuGponDbOmccNotifySet(OMCCNOTIFYFUNC omccCallback);
OMCCNOTIFYFUNC    onuGponDbOmccNotifyGet (void);
MV_STATUS         onuGponDbDisableNotifySet(DISABLENOTIFYFUNC disableCallback);
DISABLENOTIFYFUNC onuGponDbDisableNotifyGet (void);

/* ONU GPON Sync param table API functions */
MV_STATUS onuGponDbGuardBitsSet(MV_U32 guardBit);
MV_U32    onuGponDbGuardBitsGet(void);
MV_STATUS onuGponDbPreambleSet(E_OnuOverheadPreambleType premType_e, MV_U32 premVal, MV_U32 premCnt);
void      onuGponDbPreambleGet(E_OnuOverheadPreambleType premType_e, MV_U32 *premVal, MV_U32 *premCnt);
MV_BOOL   onuGponDbExtendedBurstOverrideGet(void);
MV_STATUS onuGponDbExtendedBurstOverrideSet(MV_BOOL enable);
MV_STATUS onuGponDbExtendedBurstOverrideValueSet (MV_U32 exBurstRange , MV_U32 exBurstOper);
MV_U32    onuGponDbExtendedBurstSyncOverrideValueGet(void);
MV_U32    onuGponDbExtendedBurstOperOverrideValueGet(void);
MV_STATUS onuGponDbDelimiterSet(E_OnuOverheadDelimiterByte delimType_e, MV_U32 delimVal);
MV_U32    onuGponDbDelimiterGet(E_OnuOverheadDelimiterByte delimType_e);
MV_STATUS onuGponDbDelimiterSizeSet(MV_U32 delimSize);
MV_U32    onuGponDbDelimiterSizeGet(void);
MV_STATUS onuGponDbDelimiterOverrideSet(MV_BOOL enable);
MV_U32    onuGponDbDelimiterOverrideGet(void);
MV_STATUS onuGponDbDelimiterOverrideValueSet(MV_U32 delimVal);
MV_U32    onuGponDbDelimiterOverrideValueGet(void);
MV_STATUS onuGponDbExtPreambleSyncSet(MV_U32 extendPremCnt);
MV_U32    onuGponDbExtPreambleSyncGet(void);
MV_STATUS onuGponDbExtPreambleOperSet(MV_U32 extendPremCnt);
MV_U32    onuGponDbExtPreambleOperGet(void);
MV_STATUS onuGponDbMaxExtraSnTransSet(MV_U32 maxExtraSnTrans);
MV_U32    onuGponDbMaxExtraSnTransGet(void);
MV_STATUS onuGponDbSerialNumSet(MV_U8 *serialNum);
void      onuGponDbSerialNumGet(MV_U8 *serialNum);
void      onuGponDbConstDelaySet(MV_U32 constDelay);
MV_U32    onuGponDbConstDelayGet(void);
MV_BOOL   onuGponDbSerialNumberMaskEnableGet(void);
MV_STATUS onuGponDbSerialNumberMaskEnableSet(MV_BOOL a_SerialNumberMaskEnable);
MV_BOOL   onuGponDbSerialNumberMaskMatchGet(void);
MV_STATUS onuGponDbSerialNumberMaskMatchSet(MV_BOOL a_SerialNumberMaskMatch);
MV_STATUS onuGponDbSnMaskSet(MV_U32 snMaskEn);
MV_STATUS onuGponDbEqualizationDelaySet(MV_U32 equalizationDelay);
MV_U32    onuGponDbEqualizationDelayGet(void);
MV_STATUS onuGponDbEqualizationDelayOverrideSet (MV_BOOL OverrideEqualizationDelay);
MV_BOOL   onuGponDbEqualizationDelayOverrideGet (void);
MV_U32    onuGponDbEqualizationDelayOverrideValueGet (void);
MV_STATUS onuGponDbEqualizationDelayOverrideValueSet (MV_U32 equalizationDelayOverrideVal);
MV_STATUS onuGponDbPowerLevelSet(MV_U32 powerLevel);
MV_U32    onuGponDbPowerLevelGet(void);
void      onuGponDbIdleMsgSet(MV_U32 *idleMsg);
void      onuGponDbIdleMsgGet(MV_U32 *idleMsg);
void      onuGponDbSnMsgSet(MV_U32 *snMsg);
void      onuGponDbSnMsgGet(MV_U32 *snMsg);

/* ONU GPON Oper param table API functions */
MV_STATUS onuGponDbBerIntervalSet(MV_U32 berInterval);
MV_U32    onuGponDbBerIntervalGet(void);
MV_STATUS onuGponDbBerCalcIntervalSet(MV_U32 internalBerInterval);
MV_U32    onuGponDbBerCalcIntervalGet(void);
MV_STATUS onuGponDbSfThresholdSet(MV_U32 SF_Threshold);
MV_U32    onuGponDbSfThresholdGet(void);
MV_STATUS onuGponDbSdThresholdSet(MV_U32 SD_Threshold);
MV_U32    onuGponDbSdThresholdGet(void);
MV_STATUS onuGponDbGemPortValidSet(MV_U32 gemPortId, MV_BOOL mode);
MV_BOOL   onuGponDbGemPortValidGet(MV_U32 gemPortId);
MV_STATUS onuGponDbGemPortClearAll(void);
MV_STATUS onuGponDbGemPortAesSet(MV_U32 gemPortId, MV_BOOL mode);
MV_BOOL   onuGponDbGemPortAesGet(MV_U32 gemPortId);
MV_STATUS onuGponDbGemPortAesClearAll(void);
MV_STATUS onuGponDbReiSeqNumSet(MV_U32 reiSeqNum);
MV_U32    onuGponDbReiSeqNumGet(void);
MV_STATUS onuGponDbPasswordSet(MV_U8 *password);
void      onuGponDbPasswordGet(MV_U8 *password);
MV_STATUS onuGponDbBipInterruptStatusValueSet(MV_U32 Bip8);
MV_U32    onuGponDbBipInterruptStatusValueGet(void);

/* ONU GPON BW Alloc param table API functions */
MV_STATUS onuGponDbBwAllocSet(MV_U32 entry, MV_U32 allocId, MV_BOOL valid);
MV_STATUS onuGponDbBwAllocGet(MV_U32 entry, MV_U32 *allocId, MV_BOOL *valid);
MV_BOOL   onuGponDbBwAllocExist(MV_U32 allocId);
MV_STATUS onuGponDbBwAllocInsert(MV_U32 allocId, MV_U32 *entry);
MV_STATUS onuGponDbBwAllocRemove(MV_U32 allocId, MV_U32 *entry);

MV_STATUS onuGponDbBwIdleAllocSet(MV_U32 entry, MV_U32 allocId);
MV_STATUS onuGponDbBwIdleAllocGet(MV_U32 entry, MV_U32 *allocId);
MV_BOOL   onuGponDbBwIdleAllocExist(MV_U32 allocId, MV_U32 *entry);
MV_STATUS onuGponDbBwIdleAllocInsert(MV_U32 allocId, MV_U32 *entry);
MV_STATUS onuGponDbBwIdleAllocRemove(MV_U32 allocId, MV_U32 *entry);

MV_STATUS onuGponDbBwTcontSet(MV_U32 tcontNum, MV_BOOL exist, MV_U32 allocId, MV_BOOL valid);
MV_STATUS onuGponDbBwTcontGet(MV_U32 tcontNum, MV_BOOL *exist, MV_U32 *allocId, MV_BOOL *valid);
MV_STATUS onuGponDbBwTcontExist(MV_U32 tcontNum, MV_BOOL *exist);
MV_STATUS onuGponDbBwTcontClear(MV_U32 tcontNum);
MV_STATUS onuGponDbBwTcontAlloc(MV_U32 tcontNum, MV_U32 allocId);
MV_STATUS onuGponDbBwTcontConnectCheck(MV_U32 allocId, MV_U32 *tcontNum, MV_BOOL *valid);
MV_STATUS onuGponDbBwTcontFreeGet(MV_U32 allocId, MV_U32 *tcontNum);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_DB_H */
