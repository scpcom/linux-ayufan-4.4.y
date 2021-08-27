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
**  FILE        : ponOnuMngr.c                                               **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON Manager functionality        **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/

/* Include Files
------------------------------------------------------------------------------*/
#include "ponOnuHeader.h"

/* Local Constant
------------------------------------------------------------------------------*/      
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuMngr.c"

/* Global Variables
------------------------------------------------------------------------------*/
MV_U32  g_mngrSuspiciousFifoErrCounter = 0;
MV_U32  g_mngrDecidedFifoErrCounter    = 0;

MV_U8 *dsPloamText[MAC_LAST_DOWNSTREAM_PLOAM + 1] = 
{ "UNKNOWN",
  "UPSTREAM OVERHEAD",  
  "SERIAL_NUMBER MASK",
  "ASSIGN ONU ID",      
  "RANGING TIME",
  "DEACTIVATE ONU ID",  
  "DISABLE SERIAL NUMBER",
  "CONFIGURE VP/VC",    
  "ENCRYPTED PORT-ID/VPI",
  "REQUEST PASSWORD",   
  "ASSIGN ALLOC ID",
  "NO MESSAGE",         
  "POPUP",
  "REQUEST KEY",        
  "CONFIGURE PORT-ID",
  "PEE",                
  "CHANGE POWER LEVEL",
  "PST MESSAGE",        
  "BER INTERVAL",
  "KEY SWITCHING TIME", 
  "EXTENDED BURST LENGTH"
};

/* Local Variables
------------------------------------------------------------------------------*/

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/

/******************************************************************************/
/* ========================================================================== */
/*                         Operational Section                                */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponPonMngIntrAlarmHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt alarm 
**               functionality 
**               - handle alarms (LOS, LOF, and LCDG)
**               
**  PARAMETERS:  MV_U32 alarm    
**               MV_U32 alarmStatus
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngIntrAlarmHandler(MV_U32 alarm, MV_BOOL alarmStatus)
{
  MV_U32          onuState;
  MV_U32          alarmIndex;
  MV_U32          alarmEvent;
  GPONFUNCPTR     ptrFunc;
  S_OnuGponGenTbl *onuGponGenTbl_p;
  MV_U32          eventBase;

  /* Events State Machine Table */
  onuGponGenTbl_p = &(onuGponDb_s.onuGponGenTbl_s);

  /* get onu state */
  onuState = onuGponDbOnuStateGet();

  /* Get the Alarm Index in the Event Table */
  switch (alarm)
  {
  case ONU_PON_MNGR_LOS_ALARM:
    alarmIndex = ONU_GPON_ALARM_LOS;
    alarmEvent = ONU_GPON_EVENT_ALARM_LOS;
    break;
  case ONU_PON_MNGR_LOF_ALARM: 
    alarmIndex = ONU_GPON_ALARM_LOF;
    alarmEvent = ONU_GPON_EVENT_ALARM_LOF;
    break;
  case ONU_PON_MNGR_LCDG_ALARM:
    alarmIndex = ONU_GPON_ALARM_LCDG;
    alarmEvent = ONU_GPON_EVENT_ALARM_LCDG;
    break;
  default:
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Unkown Alarm Type, alarm(%d), status(%d)\n", 
               __FILE_DESC__, __LINE__, alarm, alarmStatus);
    return;
  }

  /* Get the Base Enum in the Event Table */
  eventBase = (alarmStatus == MV_TRUE) ? ONU_GPON_ALARM_GEN_BASE : ONU_GPON_ALARM_CAN_BASE;

  /* handle alarm deactivation */
  ptrFunc = onuGponGenTbl_p->onuGponStateAndEventTbl[eventBase + alarmEvent][onuState];
  if (ptrFunc == NULL)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Cancel alarm - function is NULL, alarm(%d), status(%d)\n", 
               __FILE_DESC__, __LINE__, alarm, alarmStatus);
  }
  else
  {
    (*ptrFunc)(alarmIndex, 0, NULL);
  }
}

/*******************************************************************************
**
**  onuGponPonMngIntrMessageHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt message 
**               functionality 
**               
**  PARAMETERS:  None    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngIntrMessageHandler(void)
{
  MV_STATUS     rcode;
  MV_U32        ploamFifoSize;
  static MV_U32 ploamErrorCounter = 0;
  MV_U32        i;
  MV_U32        dummyBuffer;
  MV_U8         msgData[12];
  MV_U8         msgOnuId;
  MV_U8         msgId;
  MV_BOOL       onuValid;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_MNG1_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

  /* Get the Init value of the ONU (if it MV_FALSE - not Valid yet) */
  onuValid = onuGponDbInitGet();

  /* read fifo size */
  rcode = mvOnuGponMacRxPloamDataUsedGet(&ploamFifoSize);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read ploam message fifo size\n", __FILE_DESC__, __LINE__);
    return;
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_MNG_MODULE,
             "DEBUG: (%s:%d) PLOAM FIFO Used (%d) words\n", __FILE_DESC__, __LINE__, ploamFifoSize);
#endif /* MV_GPON_DEBUG_PRINT */

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */

  /* there is at least one full ploam message in the fifo */
  while (ploamFifoSize >= 3)
  {
#ifdef MV_GPON_PERFORMANCE_CHECK
    tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_MNG2_PERFORMANCE]);
   
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

    /* clear received message content */
    memset((void*)msgData, 0, sizeof (msgData));

    /* read message from fifo */
    rcode = mvOnuGponMacMessageReceive(&msgOnuId, &msgId, msgData);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Read ploam message\n", __FILE_DESC__, __LINE__);
      return;     
    }

#ifdef MV_GPON_DEBUG_PRINT
    mvPonPrint(PON_PRINT_DEBUG, PON_MNG_MODULE,
               "[DS PLOAM] %s, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
               dsPloamText[(msgId > 20) ? 0 : msgId], msgOnuId, msgId, 
               msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
               msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

    onuGponPmRxSwCountersAdd(msgId);

#ifdef MV_GPON_PERFORMANCE_CHECK
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
    if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */

    /* If ONU wasn't init ONU will receive Downstream PLOAMs but won't handle it */
    if (onuValid != MV_FALSE)
    {
      onuGponPonMngPloamProcess(msgOnuId, msgId, msgData);  
    }

#ifdef MV_GPON_PERFORMANCE_CHECK
    tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_MNG3_PERFORMANCE]);
   
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

    /* Check first if there are any complete message in FIFO */
    rcode = mvOnuGponMacRxPloamDataUsedGet(&ploamFifoSize);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Read ploam message\n", __FILE_DESC__, __LINE__);
      return;
    }

#ifdef MV_GPON_PERFORMANCE_CHECK
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
    if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */

  } /* while */

  /* Clean FIFO if it lost alignment */
  if ((ploamFifoSize > 0) && (ploamFifoSize < 3))
  {
    ploamErrorCounter++;
    g_mngrSuspiciousFifoErrCounter++;
    if (ploamErrorCounter == 3)
    {
      g_mngrDecidedFifoErrCounter++;
      for (i = 0; i < ploamFifoSize; i++)
      {
        mvOnuGponMacRxPloamDataGet(&dummyBuffer);
      }
    }
  }
  else
    ploamErrorCounter = 0;
}

/*******************************************************************************
**
**  onuGponPonMngPloamProcess
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function Hadnle single received PLAOM message
**               
**  PARAMETERS:  MV_U8 onuId
**               MV_U8 msgId
**               MV_U8 *msgData    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngPloamProcess(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_U32          appOnuId;
  MV_U32          onuState;
  GPONFUNCPTR     ptrFunc;
  S_OnuGponGenTbl *onuGponGenTbl_p = &(onuGponDb_s.onuGponGenTbl_s);

  /* get onu Id */
  appOnuId = onuGponDbOnuIdGet();

  /* get onu state */
  onuState = onuGponDbOnuStateGet();

  /* Filter for messages with invalid ONU Id */
  if ((onuId == ONU_GPON_BROADCAST_ONU_ID) || (onuId == appOnuId))
  {
    /* Handle valid messages */
    if ((msgId >= ONU_GPON_DS_MSG_OVERHEAD) && (msgId <= ONU_GPON_DS_MSG_EXT_BURST_LEN))
    {
      /* Call the relevant event function */
      ptrFunc = (onuGponGenTbl_p->onuGponStateAndEventTbl[(msgId)][onuState]);
      if (ptrFunc == NULL)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Msg function is NULL, onuId(%d), msgId(%d)\n", 
                   __FILE_DESC__, __LINE__, onuId, msgId);
      }
      else
      {
        (*ptrFunc)(onuId, msgId, msgData);
      }           
    }
    /* handle invalid messages */
    else
    {
      /* Call the relevant function */
      ptrFunc = (onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_GEN_MEM][onuState]);
      if (ptrFunc == NULL)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Msg function is NULL, onuId(%d), msgId(%d)\n", 
                   __FILE_DESC__, __LINE__, onuId, msgId);
      }
      else
      {
        (*ptrFunc)(ONU_GPON_ALARM_MEM, 0, NULL);
      }           
    } 
  } 
}

/*******************************************************************************
**
**  onuGponPonMngIntrBeCounterHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt BER counter 
**               expired functionality 
**               
**  PARAMETERS:  None    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngIntrBeCounterHandler(void)
{
  MV_U32 onuState;
  MV_U32 bipErrorsCounter = 0;
  MV_U8  onuId;

  onuState = onuGponDbOnuStateGet();
  onuId    = onuGponDbOnuIdGet();

  if (onuState != ONU_GPON_05_OPERATION)
  {
    return;
  }

  /* Read BIP8 errors counter in MAC */
  bipErrorsCounter = onuGponDbBipInterruptStatusValueGet();
 
  /* Send an upstream REI message */
  mvOnuGponMacReiMessageSend(onuId, bipErrorsCounter);
}

/*******************************************************************************
**
**  onuGponPonMngrFifoErrCountersGet
**  ____________________________________________________________________________
**
**  DESCRIPTION:   The function 
**                 
**  PARAMETERS:    None    
**
**  OUTPUTS:       None
**
**  RETURNS:       None 
**
*******************************************************************************/
MV_U32 onuGponPonMngrFifoErrCountersGet(MV_U32 *suspiciousCounter)
{
  MV_U32 suspected;
  MV_U32 decided;

  suspected = g_mngrSuspiciousFifoErrCounter;
  decided = g_mngrDecidedFifoErrCounter;

  g_mngrSuspiciousFifoErrCounter = 0;
  g_mngrDecidedFifoErrCounter = 0;

  *suspiciousCounter = suspected;
  return(decided);
}
