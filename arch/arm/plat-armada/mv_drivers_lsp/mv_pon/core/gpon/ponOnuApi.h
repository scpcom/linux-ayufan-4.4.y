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
**  FILE        : ponOnuApi.h                                                **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON API definitions                **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_API_H
#define _ONU_GPON_API_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
#define GPON_ONU_ITEM_NOT_CONIGURED	  (0xFFFFFFFF)

/* Notify Types */
#define GPON_NOTIFY_OPERATION		  (0)
#define GPON_NOTIFY_ALARM			  (1)
#define GPON_NOTIFY_OMCC			  (2

/* Counter Sizes */
#define GPON_ONU_RX_PLOAM_CNT_SIZE    (21)
#define GPON_ONU_TX_PLOAM_CNT_SIZE    (10)

/* Operational Statuses */
#define GPON_ONU_STATUS_DISCONNECTED  (0)                       /* No Downstream sync */
#define GPON_ONU_STATUS_NOT_RANGED	  (1)                       /* Downstream sync, not ranged */
#define GPON_ONU_STATUS_RANGED		  (2)                       /* Ranged */
#define GPON_ONU_STATUS_MAX			  (GPON_ONU_STATUS_RANGED)

/* Alarms */
#define GPON_ONU_ALARM_LOS			  (0)
#define GPON_ONU_ALARM_LOF			  (1)
#define GPON_ONU_ALARM_LCDA			  (2)
#define GPON_ONU_ALARM_LCDG			  (3)
#define GPON_ONU_ALARM_SF			  (4)
#define GPON_ONU_ALARM_SD			  (5)
#define GPON_ONU_ALARM_TF			  (6)
#define GPON_ONU_ALARM_SUF			  (7)
#define GPON_ONU_ALARM_MEM			  (8)
#define GPON_ONU_ALARM_DACT			  (9)
#define GPON_ONU_ALARM_DIS			  (10)
#define GPON_ONU_ALARM_MIS			  (11)
#define GPON_ONU_ALARM_PEE			  (12)
#define GPON_ONU_ALARM_RDI			  (13)

#define GPON_ONU_ALARM_STATUS_OFF	  (0)
#define GPON_ONU_ALARM_STATUS_ON	  (1)

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Structs                              
------------------------------------------------------------------------------*/ 
typedef struct
{
  MV_U8   password[10];
  MV_U8   serialNumber[8];
  MV_U32  onuId;
  MV_U32  state;
  MV_U32  sdThreshold;
  MV_U32  sfThreshold;
  MV_U32  localBerInterval;
  MV_U32  remoteBerInterval;
  MV_U32  omccPortId;
  MV_U32  constDelay;
  MV_U32  eqDelay;
}S_OnuInfo;

typedef struct
{
  MV_U32  bip8;
  MV_U32  plend;
}S_RxStandardApiPm;

typedef struct
{
  MV_U32  start;
  MV_U32  stop;
  MV_U32  order;
  MV_U32  polarity;
  MV_U32  mask;
}S_apiBurstConfig;

/* Typedefs
------------------------------------------------------------------------------*/
typedef void (*ALARMNOTIFYFUNC)(MV_U32 alarm, MV_U32 status);     
typedef void (*STATUSNOTIFYFUNC)(MV_U32 status);     
typedef void (*OMCCNOTIFYFUNC)(MV_U32 omccPortId);     
typedef void (*DISABLENOTIFYFUNC)(MV_BOOL disable);     

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

/* Init API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiInit(MV_U8 *serialNumber, MV_U8 *password, MV_BOOL disabled);

/* Notify API
------------------------------------------------------------------------------*/
MV_STATUS onuPonApiStatusNotifyRegister(STATUSNOTIFYFUNC notifyCallBack);
MV_STATUS onuGponApiAlarmNotifyRegister(ALARMNOTIFYFUNC notifyCallBack);
MV_STATUS onuGponApiOmccNotifyRegister(OMCCNOTIFYFUNC notifyCallBack);
MV_STATUS onuGponApiDisableNotifyRegister(DISABLENOTIFYFUNC notifyCallBack);

/* Ber API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiBerThresholdConfig(MV_U32 sd, MV_U32 sf);
MV_STATUS onuGponApiBerIntervalConfig(MV_U32 interval);
MV_STATUS onuGponApiBerCoefficientConfig(MV_U32 denominator, MV_U32 numerator);

/* Tcont API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiTcontConfig(MV_U32 allocId, MV_U32 tcontId);
MV_STATUS onuGponApiTcontClear(MV_U32 tcontId);
MV_STATUS onuGponApiTcontsReset(void);

/* GEM API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiGemOmccIdConfig(MV_U32 gemPortid, MV_BOOL valid);
MV_STATUS onuGponApiGemPortIdConfig(MV_U32 gemPortid);
MV_STATUS onuGponApiGemPortIdClear(MV_U32 gemPortid);
#ifdef MV_GPON_STATIC_GEM_PORT
MV_STATUS onuGponApiGemPortIdStaticConfigReset(void);
MV_STATUS onuGponApiGemPortIdStaticConfigFlag(MV_U32 flag);
#endif /* MV_GPON_STATIC_GEM_PORT */

/* Information API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiInformationGet(S_OnuInfo *onuInfo);

/* Alarm API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiAlarmsGet(MV_U32 *alarms);

/* PM API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiPmFecPmGet(S_GponIoctlFecPm *fecPm, MV_BOOL a_clear);
MV_STATUS onuGponApiPmRxPloamPmGet(S_GponIoctlPloamRxPm *rxPloamPm, MV_BOOL a_clear);
MV_STATUS onuGponApiPmRxBwMapPmGet(S_GponIoctlBwMapPm *rxBwMapPm, MV_BOOL a_clear);
MV_STATUS onuGponApiAdvancedPloamsCounterGet(S_GponIoctlPloamTxPm *txPloamPm, S_GponIoctlPloamRxPm *rxPloamPm, MV_BOOL a_clear);
MV_STATUS onuGponApiGemRxCounterGet(S_GponIoctlGemRxPm *gemPm, MV_BOOL a_clear);
MV_STATUS onuGponApiGemTxCounterGet(S_GponIoctlGemTxPm *txPm, MV_BOOL a_clear);
MV_STATUS onuGponApiPmRxStandardPmGet(S_RxStandardApiPm *rxStandardPm, MV_BOOL a_clear);

/* Genral API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiBurstConfigSet(S_apiBurstConfig *burstConfigSet);
MV_STATUS onuGponApiBurstConfigGet(S_apiBurstConfig *burstConfigGet);
void      onuGponApiResetAllCtr(void);

/* Sn Mask  API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiSnMaskConfig(MV_BOOL enable, MV_BOOL match);

/* Interoperability API
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiExtendedBurstSet(MV_BOOL enable, MV_U32 range, MV_U32 oper);
MV_STATUS onuGponApiDelimiterSet(MV_BOOL enable, MV_U32 delimiter);
MV_STATUS onuGponApiOnuIdSet(MV_BOOL enable, MV_U32 onuId);
MV_STATUS onuGponApiOmccPortIdSet(MV_BOOL enable, MV_U32 portId);
MV_STATUS onuGponApiEqualizationDelaySet(MV_BOOL enable, MV_U32  eqD); 
MV_STATUS onuGponApiEqualizationDelayChange(MV_U32 direction, MV_U32 size);
                   
/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_API_H */
