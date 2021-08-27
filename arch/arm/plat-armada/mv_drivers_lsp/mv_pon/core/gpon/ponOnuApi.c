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
**  FILE        : ponOnuApi.c                                                **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON API functionality            **
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
#define __FILE_DESC__ "mv_pon/core/gpon/onuGponApi.c"

/* Global Variables
------------------------------------------------------------------------------*/
S_GponPm g_OnuGponOutPm;

/* Local Variables
------------------------------------------------------------------------------*/
#ifdef MV_GPON_STATIC_GEM_PORT
MV_U32 staticGemPortConfig[4096];
MV_U32 staticGemPortConfigFlag  = 0;
#endif /* MV_GPON_STATIC_GEM_PORT */

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions 
------------------------------------------------------------------------------*/
MV_STATUS onuGponApiGemPortIdConfig(MV_U32 gemPortid);

/******************************************************************************/
/* ========================================================================== */
/*                         Initialization Section                             */
/* ========================================================================== */
/******************************************************************************/
/*******************************************************************************
**
**  onuGponApiInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu gpon API
**               
**  PARAMETERS:  MV_U8 *serialNumber
**               MV_U8 *password
**               MV_BOOL  disabled 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiInit(MV_U8 *serialNumber, MV_U8 *password, MV_BOOL disabled)
{
  MV_STATUS rcode;
  MV_U32    constDelay;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiInit Serial number (%s) Password (%s)\n", 
             __FILE_DESC__, __LINE__, serialNumber, password);
#endif /* MV_GPON_DEBUG_PRINT */

  constDelay = GPON_TX_DELAY_TD_1244;

  rcode  = onuGponDbPasswordSet(password);
  rcode |= onuGponSrvcSerialNumberSet(serialNumber);
  onuGponDbConstDelaySet(constDelay);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponDbConstDelaySet", __FILE_DESC__, __LINE__); 
    return rcode;
  }

  rcode = mvOnuGponMacRxInternalDelaySet(constDelay);
  if (rcode != MV_OK)
  {  
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacRxInternalDelaySet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  if (disabled == MV_TRUE)
  {
    /* update asic */
    rcode = mvOnuGponMacOnuStateSet(ONU_GPON_07_EMERGANCY_STOP);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
                 "ERROR: (%s:%d) mvOnuGponMacOnuStateSet", __FILE_DESC__, __LINE__); 
      return(rcode);
    }

    /* update database */
    onuGponDbOnuStateSet(ONU_GPON_07_EMERGANCY_STOP);
  }

  memset(&g_OnuGponOutPm, 0, sizeof (S_GponPm));

  onuGponDbInitSet(MV_TRUE);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiAlarmNotifyRegister
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function register alarm callback function
**               
**  PARAMETERS:  ALARMNOTIFYFUNC notifyCallBack 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiAlarmNotifyRegister(ALARMNOTIFYFUNC notifyCallBack)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiAlarmNotifyRegister\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponDbAlarmNotifySet(notifyCallBack);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiAlarmNotifyRegister", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuPonApiStatusNotifyRegister
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function register status callback function
**               
**  PARAMETERS:  STATUSNOTIFYFUNC notifyCallBack 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuPonApiStatusNotifyRegister(STATUSNOTIFYFUNC notifyCallBack)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiStatusNotifyRegister\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponDbStatusNotifySet(notifyCallBack);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiStatusNotifyRegister", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiOmccNotifyRegister
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function register omcc callback function
**               
**  PARAMETERS:  OMCCNOTIFYFUNC notifyCallBack 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiOmccNotifyRegister(OMCCNOTIFYFUNC notifyCallBack)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiOmccNotifyRegister\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponDbOmccNotifySet(notifyCallBack);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiOmccNotifyRegister", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiDisableNotifyRegister
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function register disable onu callback function
**               
**  PARAMETERS:  DISABLENOTIFYFUNC notifyCallBack 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiDisableNotifyRegister(DISABLENOTIFYFUNC notifyCallBack)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiDisableNotifyRegister\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponDbDisableNotifySet(notifyCallBack);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiDisableNotifyRegister", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiTcontConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function validates and configures Alloc-Id to T-cont 
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 tcontId 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiTcontConfig(MV_U32 allocId, MV_U32 tcontId)
{
  MV_STATUS  rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiTcontConfig alloc Id (%d) tcont (%d)\n", 
             __FILE_DESC__, __LINE__, allocId, tcontId);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponAllocIdTcontSet(allocId,tcontId);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiTcontConfig", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiBerThresholdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configures the SD / SF thresholds for GPON ONU 
**               
**  PARAMETERS:  MV_U32 sd
**               MV_U32 sf 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiBerThresholdConfig(MV_U32 sd, MV_U32 sf)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiBerThresholdConfig sd (%d) sf (%d)\n", 
             __FILE_DESC__, __LINE__, sd, sf);
#endif /* MV_GPON_DEBUG_PRINT */

  if ((sf >= sd) || 
      (sf > ONU_GPON_MAX_SF_THRESHOLD) || (sf < ONU_GPON_MIN_SF_THRESHOLD) ||
      (sd > ONU_GPON_MAX_SD_THRESHOLD) || (sd < ONU_GPON_MIN_SD_THRESHOLD))
  {
    /* SD threshold must be higher than SF */
    return(MV_ERROR);
  }

  rcode = onuGponDbSdThresholdSet(sd);
  rcode |= onuGponDbSfThresholdSet(sf);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiBerThresholdConfig", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiBerIntervalConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configures the ONU BER interval,
**               which defines the time the ONU send BER information
**                 
**  PARAMETERS:  MV_U32 interval 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK. 
**                   
*******************************************************************************/
MV_STATUS onuGponApiBerIntervalConfig(MV_U32 interval)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiBerIntervalConfig interval (%d)\n", 
             __FILE_DESC__, __LINE__, interval);
#endif /* MV_GPON_DEBUG_PRINT */

  onuGponBerIntervalSet(interval);

  return(MV_OK);
}
 
/*******************************************************************************
**
**  onuGponApiBerCoefficientConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configures the BER coefficient, which are used
**               in the BER Calculation formula 
**               
**  PARAMETERS:  MV_U32 denominator
**               MV_U32 numerator 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponApiBerCoefficientConfig(MV_U32 denominator, MV_U32 numerator)
{
  S_BerCoefficient coef;

  coef.denominator = denominator;
  coef.numerator   = numerator;
  onuGponBerCoefficientSet(&coef);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiTcontClear
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clears all Alloc-Ids from T-cont 
**               
**  PARAMETERS:  MV_U32 tcontId 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiTcontClear(MV_U32 tcontId)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiTcontClear tcontId (%d)\n", 
             __FILE_DESC__, __LINE__, tcontId);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponAllocIdTcontClear(tcontId);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiTcontClear", __FILE_DESC__, __LINE__); 
    return(rcode);
  }
  
  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiTcontsReset
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clears all Alloc-Ids from all T-conts 
**               
**  PARAMETERS:  void 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiTcontsReset(void)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiTcontsReset\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponAllocIdTcontClearAll();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiTcontsReset", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiGemOmccIdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure omcc port Id 
**               
**  PARAMETERS:  MV_U32 gemPortid
**               MV_U32 valid
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiGemOmccIdConfig(MV_U32 gemPortid, MV_BOOL valid)
{
  MV_STATUS rcode;
#ifdef MV_GPON_STATIC_GEM_PORT
  MV_U32 index;
#endif /* MV_GPON_STATIC_GEM_PORT */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiGemOmcctIdConfig gemPortid (%d) valid (%d)\n", 
             __FILE_DESC__, __LINE__, gemPortid, valid);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = mvOnuGponMacGemOmciPortIdSet(gemPortid, (valid == MV_TRUE) ? (1) : (0)); /* Rx */
  rcode |= mvOnuGponMacUtmOmciPortIdSet(gemPortid, (valid == MV_TRUE) ? (1) : (0)); /* Tx */
  rcode |= mvOnuGponMacPortIdValidSet(gemPortid, valid);                         /* Gem Port-Id Valid Table */

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiGemOmcctIdConfig", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

#ifdef MV_GPON_STATIC_GEM_PORT
  if (staticGemPortConfigFlag != 0)
  {
    for (index = 0; index < 4096; index++)
    {
      if (staticGemPortConfig[index] != 0)
      {
        onuGponApiGemPortIdConfig(index);
      }
    }
  }
#endif /* MV_GPON_STATIC_GEM_PORT */

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiGemPortIdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure gem port Id from MAC
**               
**  PARAMETERS:  MV_U32 gemPortid 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiGemPortIdConfig(MV_U32 gemPortid)
{
  MV_STATUS  rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiGemPortIdConfig gemPortid (%d)\n", 
             __FILE_DESC__, __LINE__, gemPortid);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = mvOnuGponMacPortIdValidSet(gemPortid, MV_TRUE);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiGemPortIdConfig", __FILE_DESC__, __LINE__); 
    return(rcode);
  }
  onuGponDbGemPortValidSet(gemPortid, MV_TRUE);
  
#ifdef MV_GPON_STATIC_GEM_PORT
  if (staticGemPortConfigFlag == 0)
  {
    if (gemPortid < 4096)
      staticGemPortConfig[gemPortid] = 1;
  }
#endif /* MV_GPON_STATIC_GEM_PORT */

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiGemPortIdClear
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clears gem port Id from MAC
**               
**  PARAMETERS:  MV_U32 gemPortid 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiGemPortIdClear(MV_U32 gemPortid)
{
  MV_STATUS  rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiGemPortIdClear gemPortid (%d)\n", 
             __FILE_DESC__, __LINE__, gemPortid);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = mvOnuGponMacPortIdValidSet(gemPortid, MV_FALSE);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiGemPortIdClear", __FILE_DESC__, __LINE__); 
    return(rcode);
  }
  onuGponDbGemPortValidSet(gemPortid, MV_FALSE);
  
  return(rcode);
}

#ifdef MV_GPON_STATIC_GEM_PORT

/*******************************************************************************
**
**  onuGponApiGemPortIdStaticConfigReset
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clears dummy gem port Id table
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiGemPortIdStaticConfigFlag(MV_U32 flag)
{
   staticGemPortConfigFlag = flag;
   return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiGemPortIdStaticConfigReset
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clears dummy gem port Id table
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiGemPortIdStaticConfigReset(void)
{
  MV_U32 index;

  for(index = 0; index < 4096; index++)
  {
    staticGemPortConfig[index] = 0;
  }

  return(MV_OK);
}
#endif /* MV_GPON_STATIC_GEM_PORT */

/*******************************************************************************
**
**  onuGponApiInformationGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu info
**               
**  PARAMETERS:  S_OnuInfo *onuInfo 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponApiInformationGet(S_OnuInfo *onuInfo)
{ 
  onuGponDbPasswordGet(onuInfo->password);  
  onuGponDbSerialNumGet(onuInfo->serialNumber);
  onuInfo->onuId             = onuGponDbOnuIdGet();         
  onuInfo->state             = onuGponDbOnuStateGet();         
  onuInfo->sdThreshold       = onuGponDbSdThresholdGet();   
  onuInfo->sfThreshold       = onuGponDbSfThresholdGet(); 
  onuInfo->localBerInterval  = onuGponDbBerCalcIntervalGet();
  onuInfo->remoteBerInterval = onuGponDbBerIntervalGet();
  onuInfo->omccPortId        = onuGponDbOmccPortGet();
  onuInfo->constDelay        = onuGponDbConstDelayGet();
  onuInfo->eqDelay           = onuGponDbEqualizationDelayGet();

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiAlarmsGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu alarms bit map
**               
**  PARAMETERS:  MV_U32 *alarms 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponApiAlarmsGet(MV_U32 *alarms)
{
  MV_U32  i;
  MV_U32  status;

  *alarms = 0;
  for (i = ONU_GPON_ALARM_LOS ; i < ONU_GPON_MAX_ALARMS ; i++)
  {
    status = onuGponAlarmGet(i);
    if (status == ONU_GPON_ALARM_ON)
    {
      *alarms |= 1 << i;
    }
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPmFecPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               FEC PM counters                           
**                
**  PARAMETERS:  S_IoctlFecPm *fecPm
**               MV_BOOL      a_clear 
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiPmFecPmGet(S_GponIoctlFecPm *fecPm, MV_BOOL a_clear)
{
  MV_STATUS rcode;
  S_RxFecPm inCounters;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiPmFecPmGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponPmFecPmGet(&inCounters);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiPmFecPmGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  fecPm->receivedBytes        = inCounters.receivedBytes        - g_OnuGponOutPm.rxFec.receivedBytes;
  fecPm->correctedBytes       = inCounters.correctedBytes       - g_OnuGponOutPm.rxFec.correctedBytes;
  fecPm->correctedBits        = inCounters.correctedBits        - g_OnuGponOutPm.rxFec.correctedBits;
  fecPm->receivedCodeWords    = inCounters.receivedCodeWords    - g_OnuGponOutPm.rxFec.receivedCodeWords;
  fecPm->uncorrectedCodeWords = inCounters.uncorrectedCodeWords - g_OnuGponOutPm.rxFec.uncorrectedCodeWords;

  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.rxFec.receivedBytes        = inCounters.receivedBytes;
    g_OnuGponOutPm.rxFec.correctedBytes       = inCounters.correctedBytes;
    g_OnuGponOutPm.rxFec.correctedBits        = inCounters.correctedBits;
    g_OnuGponOutPm.rxFec.receivedCodeWords    = inCounters.receivedCodeWords;
    g_OnuGponOutPm.rxFec.uncorrectedCodeWords = inCounters.uncorrectedCodeWords;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiPmRxPloamPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Rx Ploam counters                           
**                
**  PARAMETERS:  S_IoctlPloamRxPm *rxPloamPm
**               MV_BOOL           a_clear 
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiPmRxPloamPmGet(S_GponIoctlPloamRxPm *rxPloamPm, MV_BOOL a_clear)
{
  MV_STATUS   rcode;
  S_RxPloamPm inCounters;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiPmRxPloamPmGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponPmRxPloamPmGet(&inCounters);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiPmRxPloamPmGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  rxPloamPm->rxCrcErrorPloam      = inCounters.crcErrorPloamCounter          - g_OnuGponOutPm.rxPloam.crcErrorPloamCounter;
  rxPloamPm->rxBroadcastPloam     = inCounters.receivedBroadcastPloamCounter - g_OnuGponOutPm.rxPloam.receivedBroadcastPloamCounter;
  rxPloamPm->rxOnuIdPloam         = inCounters.receivedOnuIdPloamCounter     - g_OnuGponOutPm.rxPloam.receivedOnuIdPloamCounter;
  rxPloamPm->rxFifoOverErrorPloam = inCounters.fifoOverErrorPloamCounter     - g_OnuGponOutPm.rxPloam.fifoOverErrorPloamCounter;
  rxPloamPm->rxIdlePloam          = inCounters.idlePloamCounter              - g_OnuGponOutPm.rxPloam.idlePloamCounter;
  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.rxPloam.crcErrorPloamCounter          = inCounters.crcErrorPloamCounter;
    g_OnuGponOutPm.rxPloam.receivedBroadcastPloamCounter = inCounters.receivedBroadcastPloamCounter;
    g_OnuGponOutPm.rxPloam.receivedOnuIdPloamCounter     = inCounters.receivedOnuIdPloamCounter;
    g_OnuGponOutPm.rxPloam.fifoOverErrorPloamCounter     = inCounters.fifoOverErrorPloamCounter;
    g_OnuGponOutPm.rxPloam.idlePloamCounter              = inCounters.idlePloamCounter;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiPmTxPloamPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Tx Ploam counters                           
**                
**  PARAMETERS:  S_TxPloamAdvApiPm *txPloamPm
**               S_RxPloamAdvApiPm *rxPloamPm
**               MV_BOOL              a_clear
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiAdvancedPloamsCounterGet(S_GponIoctlPloamTxPm *txPloamPm, 
                                             S_GponIoctlPloamRxPm *rxPloamPm,
                                             MV_BOOL a_clear)
{
  MV_STATUS   status;
  S_TxPloamPm inCounters1;                      
  S_RxPloamPm inCounters2;
  MV_U32       numOfDsMsgIdIndex;
  MV_U32       numOfUsMsgIdIndex;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiAdvancedPloamsCounterGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  status  = onuGponPmTxPloamPmGet(&inCounters1);
  status |= onuGponPmRxPloamPmGet(&inCounters2);

  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiAdvancedPloamsCounterGet", __FILE_DESC__, __LINE__); 
    return status;
  }

  txPloamPm->txMsgTotalPloam = inCounters1.txMsgTotalPloamCounter - g_OnuGponOutPm.txPloam.txMsgTotalPloamCounter;

  for (numOfUsMsgIdIndex = 1 ;numOfUsMsgIdIndex <= ONU_GPON_US_MSG_LAST ;numOfUsMsgIdIndex++)
  {
    txPloamPm->txMsgIdPloam[numOfUsMsgIdIndex]    = inCounters1.txMsgIdPloamCounter[numOfUsMsgIdIndex]    - 
                                                    g_OnuGponOutPm.txPloam.txMsgIdPloamCounter[numOfUsMsgIdIndex];
    txPloamPm->txErrMsgIdPloam[numOfUsMsgIdIndex] = inCounters1.txErrMsgIdPloamCounter[numOfUsMsgIdIndex] - 
                                                    g_OnuGponOutPm.txPloam.txErrMsgIdPloamCounter[numOfUsMsgIdIndex];
  }

  rxPloamPm->rxMsgTotalPloam = inCounters2.rxMsgTotalPloamCounter - g_OnuGponOutPm.rxPloam.rxMsgTotalPloamCounter;

  for (numOfDsMsgIdIndex = 0 ;numOfDsMsgIdIndex <= ONU_GPON_DS_MSG_LAST ;numOfDsMsgIdIndex++)
  {
    rxPloamPm->rxMsgIdPloam[numOfDsMsgIdIndex] = inCounters2.rxMsgIdPloamCounter[numOfDsMsgIdIndex]-
                                                 g_OnuGponOutPm.rxPloam.rxMsgIdPloamCounter[numOfDsMsgIdIndex];
  }

  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.txPloam.txMsgTotalPloamCounter = inCounters1.txMsgTotalPloamCounter;

    for (numOfUsMsgIdIndex = 0 ;numOfUsMsgIdIndex <= ONU_GPON_US_MSG_LAST ;numOfUsMsgIdIndex++)
    {
      g_OnuGponOutPm.txPloam.txErrMsgIdPloamCounter[numOfUsMsgIdIndex] = inCounters1.txErrMsgIdPloamCounter[numOfUsMsgIdIndex];
      g_OnuGponOutPm.txPloam.txMsgIdPloamCounter[numOfUsMsgIdIndex]    = inCounters1.txMsgIdPloamCounter[numOfUsMsgIdIndex];
    }

    g_OnuGponOutPm.rxPloam.rxMsgTotalPloamCounter = inCounters2.rxMsgTotalPloamCounter;

    for (numOfDsMsgIdIndex = 0 ;numOfDsMsgIdIndex <= ONU_GPON_DS_MSG_LAST ;numOfDsMsgIdIndex++)
    {
      g_OnuGponOutPm.rxPloam.rxMsgIdPloamCounter[numOfDsMsgIdIndex] = inCounters2.rxMsgIdPloamCounter[numOfDsMsgIdIndex];

    }
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiPmRxBwMapPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Rx Bw Map counters                           
**                
**  PARAMETERS:  S_RxBwMapApiPm *rxBwMapPm
**               MV_BOOL           a_clear
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiPmRxBwMapPmGet(S_GponIoctlBwMapPm *rxBwMapPm, MV_BOOL a_clear)
{
  MV_STATUS   rcode;
  S_RxBwMapPm inCounters;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiPmRxBwMapPmGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponPmRxBwMapPmGet(&inCounters);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiPmRxBwMapPmGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  rxBwMapPm->allocCorrec              = inCounters.allocCorrec              - g_OnuGponOutPm.rxBwMap.allocCorrec;
  rxBwMapPm->allocCorrectableCrcErr   = inCounters.allocCorrectableCrcErr   - g_OnuGponOutPm.rxBwMap.allocCorrectableCrcErr;
  rxBwMapPm->allocCrcErr              = inCounters.allocCrcErr              - g_OnuGponOutPm.rxBwMap.allocCrcErr;
  rxBwMapPm->allocUnCorrectableCrcErr = inCounters.allocUnCorrectableCrcErr - g_OnuGponOutPm.rxBwMap.allocUnCorrectableCrcErr;
  rxBwMapPm->totalReceivedAllocBytes  = inCounters.totalReceivedAllocBytes  - g_OnuGponOutPm.rxBwMap.totalReceivedAllocBytes;

  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.rxBwMap.allocCorrec              = inCounters.allocCorrec;
    g_OnuGponOutPm.rxBwMap.allocCorrectableCrcErr   = inCounters.allocCorrectableCrcErr;
    g_OnuGponOutPm.rxBwMap.allocCrcErr              = inCounters.allocCrcErr;
    g_OnuGponOutPm.rxBwMap.allocUnCorrectableCrcErr = inCounters.allocUnCorrectableCrcErr;
    g_OnuGponOutPm.rxBwMap.totalReceivedAllocBytes  = inCounters.totalReceivedAllocBytes;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiPmRxStandardPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Rx Standard counters                           
**                
**  PARAMETERS:  S_RxStandardApiPm *rxStandardPm
**               MV_BOOL              a_clear
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiPmRxStandardPmGet(S_RxStandardApiPm *rxStandardPm, MV_BOOL a_clear)
{
  MV_STATUS   rcode;
  S_RxBip8Pm  inCounter1;
  S_RxPlendPm inCounter2;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiPmRxStandardPmGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = onuGponPmRxBip8PmGet(&inCounter1);
  rcode |= onuGponPmRxPlendPmGet(&inCounter2);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiPmRxStandardPmGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }
       
  rxStandardPm->bip8  = inCounter1.bip8  - g_OnuGponOutPm.rxBip8.bip8;
  rxStandardPm->plend = inCounter2.plend - g_OnuGponOutPm.rxPlend.plend;

  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.rxBip8.bip8   = inCounter1.bip8;
    g_OnuGponOutPm.rxPlend.plend = inCounter2.plend;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiGemRxCounterGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               GEM counters                           
**                
**  PARAMETERS:  S_IoctlGemRxPm *gemPm
**               MV_BOOL         a_clear
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiGemRxCounterGet(S_GponIoctlGemRxPm *gemPm, MV_BOOL a_clear)
{
  MV_STATUS rcode;
  S_GemPm   inCounters;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiGemCounterGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponPmGemPmGet(&inCounters);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiGemCounterGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  gemPm->gemRxIdleGemFrames                              = inCounters.receivedIdleGemFrames               - 
                                                           g_OnuGponOutPm.gem.receivedIdleGemFrames;
  gemPm->gemRxValidGemFrames                             = inCounters.receivedValidGemFrames              - 
                                                           g_OnuGponOutPm.gem.receivedValidGemFrames;            
  gemPm->gemRxUndefinedGemFrames                         = inCounters.receivedUndefinedGemFrames          - 
                                                           g_OnuGponOutPm.gem.receivedUndefinedGemFrames;                
  gemPm->gemRxOmciFrames                                 = inCounters.receivedOmciFrames                  - 
                                                           g_OnuGponOutPm.gem.receivedOmciFrames;                        
  gemPm->gemRxDroppedGemFrames                           = inCounters.droppedGemFrames                    - 
                                                           g_OnuGponOutPm.gem.droppedGemFrames;                          
  gemPm->gemRxDroppedOmciFrames                          = inCounters.droppedOmciFrames                   - 
                                                           g_OnuGponOutPm.gem.droppedOmciFrames;
  gemPm->gemRxGemFramesWithUncorrHecErr                  = inCounters.receivedGemFramesWithUncorrHecErr   - 
                                                           g_OnuGponOutPm.gem.receivedGemFramesWithUncorrHecErr;         
  gemPm->gemRxGemFramesWithOneFixedHecErr                = inCounters.receivedGemFramesWithOneFixedHecErr - 
                                                           g_OnuGponOutPm.gem.receivedGemFramesWithOneFixedHecErr;       
  gemPm->gemRxGemFramesWithTwoFixedHecErr                = inCounters.receivedGemFramesWithTwoFixedHecErr - 
                                                           g_OnuGponOutPm.gem.receivedGemFramesWithTwoFixedHecErr;       
  gemPm->gemRxTotalByteCountOfReceivedValidGemFrames     = inCounters.totalByteCountOfReceivedValidGemFrames     - 
                                                           g_OnuGponOutPm.gem.totalByteCountOfReceivedValidGemFrames;    
  gemPm->gemRxTotalByteCountOfReceivedUndefinedGemFrames = inCounters.totalByteCountOfReceivedUndefinedGemFrames - 
                                                           g_OnuGponOutPm.gem.totalByteCountOfReceivedUndefinedGemFrames;
  gemPm->gemRxGemReassembleMemoryFlush                   = inCounters.gemReassembleMemoryFlush            - 
                                                           g_OnuGponOutPm.gem.gemReassembleMemoryFlush;                  
  gemPm->gemRxGemSynchLost                               = inCounters.gemSynchLost                        - 
                                                           g_OnuGponOutPm.gem.gemSynchLost;                              
  gemPm->gemRxEthFramesWithCorrFcs                       = inCounters.receivedEthFramesWithCorrFcs        - 
                                                           g_OnuGponOutPm.gem.receivedEthFramesWithCorrFcs;              
  gemPm->gemRxEthFramesWithFcsError                      = inCounters.receivedEthFramesWithFcsError       - 
                                                           g_OnuGponOutPm.gem.receivedEthFramesWithFcsError;             
  gemPm->gemRxOmciFramesWithCorrCrc                      = inCounters.receivedOmciFramesWithCorrCrc       - 
                                                           g_OnuGponOutPm.gem.receivedOmciFramesWithCorrCrc;             
  gemPm->gemRxOmciFramesWithCrcError                     = inCounters.receivedOmciFramesWithCrcError      - 
                                                           g_OnuGponOutPm.gem.receivedOmciFramesWithCrcError;            
  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.gem.receivedIdleGemFrames                      = inCounters.receivedIdleGemFrames;                                                
    g_OnuGponOutPm.gem.receivedValidGemFrames                     = inCounters.receivedValidGemFrames;                    
    g_OnuGponOutPm.gem.receivedUndefinedGemFrames                 = inCounters.receivedUndefinedGemFrames;                
    g_OnuGponOutPm.gem.receivedOmciFrames                         = inCounters.receivedOmciFrames;                        
    g_OnuGponOutPm.gem.droppedGemFrames                           = inCounters.droppedGemFrames;                          
    g_OnuGponOutPm.gem.droppedOmciFrames                          = inCounters.droppedOmciFrames;                         
    g_OnuGponOutPm.gem.receivedGemFramesWithUncorrHecErr          = inCounters.receivedGemFramesWithUncorrHecErr;         
    g_OnuGponOutPm.gem.receivedGemFramesWithOneFixedHecErr        = inCounters.receivedGemFramesWithOneFixedHecErr;       
    g_OnuGponOutPm.gem.receivedGemFramesWithTwoFixedHecErr        = inCounters.receivedGemFramesWithTwoFixedHecErr;       
    g_OnuGponOutPm.gem.totalByteCountOfReceivedValidGemFrames     = inCounters.totalByteCountOfReceivedValidGemFrames;    
    g_OnuGponOutPm.gem.totalByteCountOfReceivedUndefinedGemFrames = inCounters.totalByteCountOfReceivedUndefinedGemFrames;
    g_OnuGponOutPm.gem.gemReassembleMemoryFlush                   = inCounters.gemReassembleMemoryFlush;                  
    g_OnuGponOutPm.gem.gemSynchLost                               = inCounters.gemSynchLost;                              
    g_OnuGponOutPm.gem.receivedEthFramesWithCorrFcs               = inCounters.receivedEthFramesWithCorrFcs;              
    g_OnuGponOutPm.gem.receivedEthFramesWithFcsError              = inCounters.receivedEthFramesWithFcsError;             
    g_OnuGponOutPm.gem.receivedOmciFramesWithCorrCrc              = inCounters.receivedOmciFramesWithCorrCrc;             
    g_OnuGponOutPm.gem.receivedOmciFramesWithCrcError             = inCounters.receivedOmciFramesWithCrcError;            
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiGemTxCounterGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Tx counters                           
**                
**  PARAMETERS:  S_TxApiPm *txPm
**               MV_BOOL       a_clear
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiGemTxCounterGet(S_GponIoctlGemTxPm *txPm, MV_BOOL a_clear)
{
  MV_STATUS rcode;
  S_TxPm    inCounters;
  MV_U32    tcont;
  MV_BOOL   exist;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiTxCounterGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = onuGponPmTxPmGet(&inCounters);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiTxCounterGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  txPm->gemTxGemPtiTypeOneFrames  = inCounters.transmittedGemPtiTypeOneFrames  - g_OnuGponOutPm.tx.transmittedGemPtiTypeOneFrames;                 
  txPm->gemTxGemPtiTypeZeroFrames = inCounters.transmittedGemPtiTypeZeroFrames - g_OnuGponOutPm.tx.transmittedGemPtiTypeZeroFrames;                
  txPm->gemTxIdleGemFrames        = inCounters.transmittedIdleGemFrames        - g_OnuGponOutPm.tx.transmittedIdleGemFrames;                       
  
  for (tcont = 0; tcont < ONU_GPON_MAX_NUM_OF_T_CONTS; tcont++)
  {
    onuGponDbBwTcontExist(tcont, &exist);
    if (exist != MV_FALSE)
    {
      txPm->gemTxEthFramesViaTconti[tcont]     = inCounters.transmittedEthFramesViaTconti[tcont]     - g_OnuGponOutPm.tx.transmittedEthFramesViaTconti[tcont];    
      txPm->gemTxEthBytesViaTconti[tcont]      = inCounters.transmittedEthBytesViaTconti[tcont]      - g_OnuGponOutPm.tx.transmittedEthBytesViaTconti[tcont];     
      txPm->gemTxGemFramesViaTconti[tcont]     = inCounters.transmittedGemFramesViaTconti[tcont]     - g_OnuGponOutPm.tx.transmittedGemFramesViaTconti[tcont];     
      txPm->gemTxIdleGemFramesViaTconti[tcont] = inCounters.transmittedIdleGemFramesViaTconti[tcont] - g_OnuGponOutPm.tx.transmittedIdleGemFramesViaTconti[tcont];
    }
  }
                    
  if (a_clear == MV_TRUE)
  {
    g_OnuGponOutPm.tx.transmittedGemPtiTypeOneFrames    = inCounters.transmittedGemPtiTypeOneFrames;                            
    g_OnuGponOutPm.tx.transmittedGemPtiTypeZeroFrames   = inCounters.transmittedGemPtiTypeZeroFrames;         
    g_OnuGponOutPm.tx.transmittedIdleGemFrames          = inCounters.transmittedIdleGemFrames;                       
    
    for (tcont = 0; tcont < ONU_GPON_MAX_NUM_OF_T_CONTS; tcont++)
    {
      onuGponDbBwTcontExist(tcont, &exist);
      if (exist != MV_FALSE)
      {
        g_OnuGponOutPm.tx.transmittedEthFramesViaTconti[tcont]     = inCounters.transmittedEthFramesViaTconti[tcont];
        g_OnuGponOutPm.tx.transmittedEthBytesViaTconti[tcont]      = inCounters.transmittedEthBytesViaTconti[tcont]; 
        g_OnuGponOutPm.tx.transmittedGemFramesViaTconti[tcont]     = inCounters.transmittedGemFramesViaTconti[tcont]; 
        g_OnuGponOutPm.tx.transmittedIdleGemFramesViaTconti[tcont] = inCounters.transmittedIdleGemFramesViaTconti[tcont]; 
      }
    }
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiResetAllCtr
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function reset all ONU PM information      
**                
**  PARAMETERS:  None
**
**  OUTPUTS:     None 
**
**  RETURNS:     None
**
*******************************************************************************/
void onuGponApiResetAllCtr(void)
{
  onuGponPmInPmInit();                                   /* reset pm counters */
  onuGponBerClear();                                     /* reset ber counters */
  memset(&(g_OnuGponOutPm), 0, sizeof (g_OnuGponOutPm)); /* reset api counters */
}

/*******************************************************************************
**
**  onuGponApiBurstConfigSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configures ONU TX Burst enable parameters                           
**                
**  PARAMETERS:  S_apiBurstConfig *burstConfigSet 
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiBurstConfigSet(S_apiBurstConfig *burstConfigSet)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiBurstConfigSet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = mvOnuGponMacTxBurstEnParamSet(burstConfigSet->mask, 
                                      burstConfigSet->polarity,
                                      burstConfigSet->order,
                                      burstConfigSet->stop,
                                      burstConfigSet->start);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiPmRxStandardPmGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiBurstConfigGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function retrieves ONU TX Burst enable parameters                           
**                 
**  PARAMETERS:  S_apiBurstConfig *burstConfigSet 
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiBurstConfigGet(S_apiBurstConfig *burstConfigGet)
{ 
  MV_STATUS rcode;
  MV_U32    mask;
  MV_U32    polarity;
  MV_U32    order;
  MV_U32    stop;
  MV_U32    start;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiBurstConfigGet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode = mvOnuGponMacTxBurstEnParamGet(&mask, &polarity,&order, &stop, &start);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiBurstConfigGet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  burstConfigGet->start    = start;
  burstConfigGet->stop     = stop;
  burstConfigGet->order    = order;
  burstConfigGet->polarity = polarity;
  burstConfigGet->mask     = mask;

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiExtendedBurstSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable / disable to override ONU Type 3 preamble
**               bytes assigned from the OLT with input values                           
**                 
**  PARAMETERS:  MV_BOOL   enable
**               MV_U32 range
**               MV_U32 oper   
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiExtendedBurstSet(MV_BOOL enable, MV_U32 range, MV_U32 oper)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiExtendedBurstSet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = onuGponDbExtendedBurstOverrideSet(enable);
  rcode |= onuGponDbExtendedBurstOverrideValueSet(range , oper);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiExtendedBurstSet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }
  
  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiDelimiterSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable / disable to override ONU delimiter
**               assigned from the OLT with input value
**                 
**  PARAMETERS:  MV_BOOL   enable
**               MV_U32 delimiter
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiDelimiterSet(MV_BOOL enable, MV_U32 delimiter) 
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiDelimiterSet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = onuGponDbDelimiterOverrideSet(enable);
  rcode |= onuGponDbDelimiterOverrideValueSet(delimiter);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiDelimiterSet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiOnuIdSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable / disable to override ONU ID assigned
**               from the OLT with input value                            
**                 
**  PARAMETERS:  MV_BOOL   enable
**               MV_U32 onuId
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiOnuIdSet(MV_BOOL enable, MV_U32 onuId) 
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiOnuIdSet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = onuGponDbOnuIdOverrideSet(enable);
  rcode |= onuGponDbOnuIdOverrideValueSet(onuId);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiOnuIdSet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiOmccPortIdSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable / disable to override ONU OMCC assigned
**               from the OLT with input value                           
**                 
**  PARAMETERS:  MV_BOOL   enable
**               MV_U32 portId
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiOmccPortIdSet (MV_BOOL enable, MV_U32 portId) 
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiOmccPortIdSet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */
   
  rcode  = onuGponDbOmccPortOverrideSet(enable);
  rcode |= onuGponDbOmccPortOverrideValueSet(portId);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiOmccPortIdSet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiEqualizationDelaySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable / disable to override ONU Equalization
**               Delay measured and assigned from the OLT with input value                       
**                 
**  PARAMETERS:  MV_BOOL   enable
**               MV_U32 eqD
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiEqualizationDelaySet(MV_BOOL enable, MV_U32 eqD) 
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiEqualizationDelaySet\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = onuGponDbEqualizationDelayOverrideSet(enable);
  rcode |= onuGponDbEqualizationDelayOverrideValueSet(eqD);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiEqualizationDelaySet", __FILE_DESC__, __LINE__); 
    return(rcode);
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponApiEqualizationDelayChange
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enables changes in the ONU Equalization Delay                           
**                 
**  PARAMETERS:  MV_U32 direction
**               MV_U32 size  
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiEqualizationDelayChange(MV_U32 direction, MV_U32 size) 
{
  MV_U32 eqD;
  MV_U32 newEqD;
  MV_U32 newFinalEqD;
  MV_U32 onuState = onuGponDbOnuStateGet();
  MV_U8  onuId;
  MV_U8  newEqDelay[5];

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiEqualizationDelayChange\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  if (onuState != ONU_GPON_05_OPERATION)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) EqD change failed. EqD can only be changed in state 05\n", __FILE_DESC__, __LINE__); 
    return(MV_OK);
  }
  eqD    = onuGponDbEqualizationDelayGet();
  newEqD = (direction == 1) ? eqD + size : eqD - size;
  newFinalEqD = M_ONU_GPON_RANG_MSG_FINAL_DELAY(newEqD);

  if (newFinalEqD < GPON_TX_FINAL_DELAY_MIN || newFinalEqD > GPON_TX_FINAL_DELAY_MAX )
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) EqD change failed. New Final eqD value is out of range\n", __FILE_DESC__, __LINE__); 
    return(MV_OK);
  }

  newEqDelay[4] = newEqD & 0xFF;        /* LSB */
  newEqDelay[3] = (newEqD >> 8 )& 0xFF;
  newEqDelay[2] = (newEqD >> 16)& 0xFF;
  newEqDelay[1] = (newEqD >> 24)& 0xFF; /* MSB */

  onuId = onuGponDbOnuIdGet();  
  onuGponPonMngPloamProcess(onuId, ONU_GPON_DS_MSG_RANGING_TIME, newEqDelay);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponApiSnMaskConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure sn mask
**                 
**  PARAMETERS:  MV_BOOL enable
**               MV_BOOL match
**
**  OUTPUTS:     None   
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponApiSnMaskConfig(MV_BOOL enable, MV_BOOL match)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
             "DEBUG: (%s:%d) onuGponApiSnMaskConfig\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  rcode  = onuGponDbSnMaskSet(enable);
  rcode |= onuGponDbSerialNumberMaskEnableSet(enable);
  rcode |= onuGponDbSerialNumberMaskMatchSet (match);

  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuGponApiSnMaskConfig\n", __FILE_DESC__, __LINE__); 
  }

  return(rcode);
}
