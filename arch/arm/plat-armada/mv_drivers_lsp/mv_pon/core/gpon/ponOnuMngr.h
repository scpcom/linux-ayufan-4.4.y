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
**  FILE        : ponOnuMngr.h                                               **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON Manager definitions            **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_MNGR_H
#define _ONU_GPON_MNGR_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
/* Disable Message disable/Enable Options */
#define GPON_ONU_DISABLE		    (0xFF)
#define GPON_ONU_ENABLE_ALL	        (0x0F)
#define GPON_ONU_ENABLE_ONU	        (0x00)

/* timer sub types */
#define TIMER_T01_EXPIRE            (1)
#define TIMER_T02_EXPIRE            (2)
#define TIMER_PEE_EXPIRE            (3)
                                   
/* stats sub types */              
#define STATS_ACCMULATE             (1)
#define STATS_RESET                 (2)
#define STATS_SHOW                  (3)

/* Alarms Types */
#define ONU_PON_MNGR_LOS_ALARM      (0)
#define ONU_PON_MNGR_LOF_ALARM      (1)
#define ONU_PON_MNGR_LCDG_ALARM     (2)

/* Assign Alloc Id Types */
#define ONU_GPON_ATM_PAYLOAD        (0)
#define ONU_GPON_GEM_PAYLOAD        (1)
#define ONU_GPON_DBA_PAYLOAD        (2)
#define ONU_GPON_DEALLOCATE         (255)

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Typedefs
------------------------------------------------------------------------------*/
typedef MV_STATUS (*DISABLESTATSETFUNC)(MV_BOOL disable);

typedef struct
{
  MV_U32 msgType;
  MV_U32 subType;
  MV_U32 status;
  MV_U32 param;
}S_MngrMsg;

typedef struct
{
  MV_U32 msgType;
  MV_U32 subType;
  MV_U8  onuId;
  MV_U8  msgId;
  MV_U8  data[10];
}S_MngrDebugMsg;

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/
/* Interrupt handler Functions */
void      onuGponPonMngIntrAlarmHandler(MV_U32 alarm, MV_BOOL alarmStatus); 
void      onuGponPonMngIntrMessageHandler(void);
void      onuGponPonMngIntrBeCounterHandler(void);

/* State Machine Functions */
void      onuGponPonMngPloamProcess(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);

void      onuGponPonMngIsrNotExpected(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngOverheadMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngSerialNumberMaskMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngOnuIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngRangeTimeMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngDactOnuIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngDisSnMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngCfgVpVcMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngEncrptPortIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngAssignAllocIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngNoMsgMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngPopupMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngCfgPortIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngPhyEquErrMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngChgPwrLvlMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngPstMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngKeySwitchTimeMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngExtBurstMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);

void      onuGponPonMngTimerT01Hndl(unsigned long data);
void      onuGponPonMngTimerT02Hndl(unsigned long data);
void      onuGponPonMngTimerPeeHndl(unsigned long data);
void      onuGponPonMngTimerEvtCleanUpHndl(unsigned long data);
void      onuGponPonMngTimerT01ExpireHndl(void);
void      onuGponPonMngTimerT02ExpireHndl(void);
void      onuGponPonMngTimerPeeExpireHndl(void);

/* Alarm Functions */
void      onuGponPonMngGenCritAlarm(E_OnuGponAlarmType alarmType_e, MV_U8 dummyVal, MV_U8 *dummyPtr);
void      onuGponPonMngCanCritAlarm(E_OnuGponAlarmType alarmType_e, MV_U8 dummyVal, MV_U8 *dummyPtr);
void      onuGponPonMngGenMemAlarm(E_OnuGponAlarmType alarmType_e, MV_U8 dummyVal, MV_U8 *dummyPtr);
/* Operation State Functions */                 
void      onuGponPonMngConfigPortIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);    
void      onuGponPonMngReqPassMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);    
void      onuGponPonMngBerIntervalMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData); 
void      onuGponPonMngPhyEquipErrMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);   
void      onuGponPonMngReqKeyMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);
void      onuGponPonMngEncryptPortIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);       
void      onuGponPonMngKeySwitchTimwMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData);

void      onuGponPonMngOverheadManualModeSet(MV_BOOL mode);
MV_BOOL   onuGponPonMngOverheadManualModeGet(void);
MV_U32    onuGponPonMngrFifoErrCountersGet(MV_U32 *suspiciousCounter);
MV_STATUS onuGponPonMngDisableSetRegister(DISABLESTATSETFUNC disableFunc);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_MNGR_H */
