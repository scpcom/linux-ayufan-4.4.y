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
**  FILE        : ponOnuAlrm.c                                               **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON Alarm and Statistics         **
**                functionality                                              **
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
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuAlrm.c"

/* Global Variables
------------------------------------------------------------------------------*/
S_OnuGponApmTbl onuGponApmTbl_s;

/* Local Variables
------------------------------------------------------------------------------*/
MV_U16 l_onuGponPreviousAlarmState = ONU_GPON_ALARM_DEF_STATE;
MV_U16 l_onuGponCurrentAlarmState  = ONU_GPON_ALARM_DEF_STATE; 
MV_U32 l_onuGponDsSyncFlag         = 1;

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
void onuGponAlarmProcess(void);

/******************************************************************************/
/* ========================================================================== */
/*                         Alarm Section                                      */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponAlarmTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init the onu gpon alarm table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponAlarmTblInit (void)
{
  /* set alarm Off as default for all alarms */
  memset(&(onuGponApmTbl_s.onuGponAlarmTbl_s.onuGponAlarmTbl[0]), 
         ONU_GPON_ALARM_OFF, 
         sizeof (S_OnuGponAlarmTbl));

  /* set alarms on as default only for LOS, LOF, and LCDG */
  onuGponApmTbl_s.onuGponAlarmTbl_s.onuGponAlarmTbl[ONU_GPON_ALARM_LOS]  = ONU_GPON_ALARM_ON;
  onuGponApmTbl_s.onuGponAlarmTbl_s.onuGponAlarmTbl[ONU_GPON_ALARM_LOF]  = ONU_GPON_ALARM_ON;
}

/*******************************************************************************
**
**  onuGponAlarmSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu gpon alarm 
**               
**  PARAMETERS:  E_OnuGponAlarmType  alarm
**               E_OnuGponAlarmState status
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK 
**                   
*******************************************************************************/
MV_STATUS onuGponAlarmSet(E_OnuGponAlarmType  alarm, 
                          E_OnuGponAlarmState state)
{
  if (alarm > ONU_GPON_ALARM_RDI)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_ALARM_MODULE, 
               "ERROR: (%s:%d) invalid alarm(%d) type\n", __FILE_DESC__, __LINE__, alarm);
    return(MV_ERROR);
  }

  if (state > ONU_GPON_ALARM_ON)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_ALARM_MODULE, 
               "ERROR: (%s:%d)  invalid alarm(%d) state(%d)\n", __FILE_DESC__, __LINE__, alarm, state);
    return(MV_ERROR);
  }

  onuGponApmTbl_s.onuGponAlarmTbl_s.onuGponAlarmTbl[alarm] = state;

  if (state == ONU_GPON_ALARM_OFF)
  {
    l_onuGponCurrentAlarmState &= ~(1 << alarm);
  }
  else
  {
    l_onuGponCurrentAlarmState |= (1 << alarm);
  }

  onuGponAlarmProcess();

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAlarmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu gpon alarm state
**               
**  PARAMETERS:  E_OnuGponAlarmType alarm
**
**  OUTPUTS:     None
**
**  RETURNS:     onu gpon alarm state 
**                   
*******************************************************************************/
E_OnuGponAlarmState onuGponAlarmGet(E_OnuGponAlarmType alarm)
{
  if (alarm > ONU_GPON_ALARM_RDI)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_ALARM_MODULE, 
               "ERROR: (%s:%d) invalid alarm(%d) type\n", __FILE_DESC__, __LINE__, alarm);
    return(MV_ERROR);
  }

  return(onuGponApmTbl_s.onuGponAlarmTbl_s.onuGponAlarmTbl[alarm]);
}

/*******************************************************************************
**
**  onuGponAsicAlarmStatusGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu gpon alarm state, alarm is off if all
**               asic alarms (LOS, LOF, and LCDG) are off, else alarm is on
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu gpon alarm state 
**                   
*******************************************************************************/
E_OnuGponAlarmState onuGponAsicAlarmStatusGet(void)
{
#if 0
  if ((onuGponAlarmGet(ONU_GPON_ALARM_LOS)  == ONU_GPON_ALARM_OFF) &&
      (onuGponAlarmGet(ONU_GPON_ALARM_LOF)  == ONU_GPON_ALARM_OFF) &&
      (onuGponAlarmGet(ONU_GPON_ALARM_LCDG) == ONU_GPON_ALARM_OFF))
#else
  if (onuGponAlarmGet(ONU_GPON_ALARM_LOF) == ONU_GPON_ALARM_OFF)
#endif
  {
    return(ONU_GPON_ALARM_OFF);
  }
  else
  {
    return(ONU_GPON_ALARM_ON);
  }
}

/*******************************************************************************
**
**  onuGponAlarmProcess
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function process the current alarm state and notify screen
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponAlarmProcess(void)
{
  MV_U32 onuGponPreviousAlarmState;
  MV_U32 onuGponCurrentAlarmState;
  MV_U32 onuGponChangeAlarm;

  /* get the ASIC prvious alarm status */
  onuGponPreviousAlarmState = l_onuGponPreviousAlarmState;

  /* get the ASIC current alarm status */
  onuGponCurrentAlarmState = l_onuGponCurrentAlarmState;

  /* alarm changed */
  if ((onuGponCurrentAlarmState ^ onuGponPreviousAlarmState) != 0)
  {
    onuGponChangeAlarm = (onuGponCurrentAlarmState ^ onuGponPreviousAlarmState);

    if ((onuGponChangeAlarm & onuGponCurrentAlarmState) != 0)
    {
      if ((onuGponAsicAlarmStatusGet() == ONU_GPON_ALARM_ON) &&
          l_onuGponDsSyncFlag == 0)
      {
        /* Turn OFF LED */
        onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_OFF);

        onuGponSrvcStatusNotify(GPON_ONU_STATUS_DISCONNECTED);
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "=========================\n");
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "== Downstream sync Off ==\n");
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "=========================\n");
        l_onuGponDsSyncFlag = 1;
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_LOS_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm LOS On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_LOS, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_LOF_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm LOF On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_LOF, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_LCDG_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm LCDG On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_LCDG, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_TF_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm TF On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_TF, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_SUF_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm SUF On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_SUF, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_MEM_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm MEM On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_MEM, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_DACT_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm DACT On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_DACT, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_DIS_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm DIS On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_DIS, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_MIS_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm MIS On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_MIS, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_PEE_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm PEE On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_PEE, GPON_ONU_ALARM_STATUS_ON );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_RDI_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm RDI On ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_RDI, GPON_ONU_ALARM_STATUS_ON );
      }
    }
    else /* ((onuGponChangeAlarm & onuGponCurrentAlarmState) == 0) */
    {
      if (onuGponChangeAlarm & ONU_GPON_ALARM_LOS_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm LOS Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_LOS, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_LOF_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm LOF Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_LOF, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_LCDG_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm LCDG Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_LCDG, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_TF_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm TF Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_TF, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_SUF_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm SUF Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_SUF, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_MEM_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm MEM Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_MEM, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_DACT_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm DACT Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_DACT, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_DIS_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm DIS Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_DIS, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_MIS_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm MIS Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_MIS, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_PEE_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm PEE Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_PEE, GPON_ONU_ALARM_STATUS_OFF );
      }

      if (onuGponChangeAlarm & ONU_GPON_ALARM_RDI_LOC)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === Alarm RDI Off ===\n", __FILE_DESC__, __LINE__);
        onuGponSrvcAlarmNotify( GPON_ONU_ALARM_RDI, GPON_ONU_ALARM_STATUS_OFF );
      }

      if ((onuGponAsicAlarmStatusGet() == ONU_GPON_ALARM_OFF) &&
          l_onuGponDsSyncFlag == 1)
      {
        /* Turn LED ON */
        onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_BLINK_SLOW);
        onuGponSrvcStatusNotify(GPON_ONU_STATUS_NOT_RANGED);

        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "========================\n");
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "== Downstream sync On ==\n");
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "========================\n");
        l_onuGponDsSyncFlag = 0;

        mvOnuGponMacXvrActivate();
      }
    }

    l_onuGponPreviousAlarmState = l_onuGponCurrentAlarmState;  
  }
}

/*******************************************************************************
**
**  onuGponOnuAlarmShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print current alarm statsus
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponOnuAlarmShow(void)
{
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE,
          "ONU Gpon alarms state\n"
          "=====================\n");

  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm LOS  - %s\n", 
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_LOS_LOC)  ? ("On") : ("Off"));
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm LOF  - %s\n", 
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_LOF_LOC)  ? ("On") : ("Off"));
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm LCDG - %s\n", 
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_LCDG_LOC) ? ("On") : ("Off"));
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm TF   - %s\n",      
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_TF_LOC)   ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm SUF  - %s\n",      
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_SUF_LOC)  ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm MEM  - %s\n", 
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_MEM_LOC)  ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm DACT - %s\n",      
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_DACT_LOC) ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm DIS  - %s\n",      
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_DIS_LOC)  ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm MIS  - %s\n", 
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_MIS_LOC)  ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm PEE  - %s\n",      
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_PEE_LOC)  ? ("On") : ("Off")); 
  mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, "Alarm RDI  - %s\n",      
          (l_onuGponCurrentAlarmState & ONU_GPON_ALARM_RDI_LOC)  ? ("On") : ("Off")); 
}
