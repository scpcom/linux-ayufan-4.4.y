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
**  FILE        : ponOnuMngrStateMachine.c                                   **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON Manager state machine        **
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
#include "gbe/mvNeta.h"
#include "net_dev/mv_netdev.h"

/* Local Constant
------------------------------------------------------------------------------*/                                               
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuStateMachine.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/
MV_BOOL            g_overheadManualMode = MV_FALSE;
DISABLESTATSETFUNC g_onuGponDisableFunc = NULL;
MV_BOOL            g_periodicTimerState = MV_FALSE; 

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
MV_STATUS onuGponPonMngrUpdateState(MV_U32 newState);
MV_STATUS onuGponPonMngClearOnuInfo(void);
MV_STATUS onuGponPonMngClearOnuTconts(void);
MV_STATUS onuGponPonMngClearOnuPorts(void);
MV_STATUS onuGponPonMngClearOnuId(void);
MV_STATUS onuGponPonMngClearOnuDelay(void);
MV_STATUS onuGponPonMngClearOnuOverhead(void);
MV_STATUS onuGponPonMngClearBerInterval(void);
MV_STATUS onuGponPonMngPreambleSet(MV_U32 onuState);

/******************************************************************************/
/* ========================================================================== */
/*                         Messages Section                                   */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponPonMngUpdateState
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when a change of state in the ONU
**               state machine is needed.
**               
**  PARAMETERS:  MV_U32 newState
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponPonMngrUpdateState(MV_U32 newState)
{
  MV_STATUS rcode;
  MV_U32    currentState;
  MV_U32    asicNewState = newState;

  /* get onu state */
  currentState = onuGponDbOnuStateGet();

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) ONT STATE CHANGING: FROM STATE[%d] TO STATE [%d]\n",
             __FILE_DESC__, __LINE__, currentState, newState);
#endif /* MV_GPON_DEBUG_PRINT */

  if (newState == ONU_GPON_01_INIT)
  {
    /*Reset All Counters*/
    onuGponApiResetAllCtr();

    /* No Downstream - Turn LED OFF */
    onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_OFF);

    /* stop onu periodic timers */ 
    if (g_periodicTimerState != MV_FALSE) 
    {
      onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponEvtCleanUpTimerId));
      onuPonTimerDisable(&(onuPonResourceTbl_s.onuPonPmTimerId));
      g_periodicTimerState = MV_FALSE;
    }
  }

  if (newState == ONU_GPON_02_STANDBY)
  {
    /* Downstream ON - Blink the LED */
    onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_BLINK_SLOW);

    /* stop onu periodic timers */ 
    if (g_periodicTimerState != MV_FALSE) 
    {
      onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponEvtCleanUpTimerId));
      onuPonTimerDisable(&(onuPonResourceTbl_s.onuPonPmTimerId));
      g_periodicTimerState = MV_FALSE;
    }
  }

  rcode = onuGponPonMngPreambleSet(newState);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) Preamble Set\n", __FILE_DESC__, __LINE__);
  }

  if (newState == ONU_GPON_03_SERIAL_NUM)
  {
    /* Sync sequence - Blink the LED fast */
    onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_BLINK_FAST);

    if ((onuGponDbSerialNumberMaskMatchGet()  == MV_FALSE) && 
        (onuGponDbSerialNumberMaskEnableGet() == MV_TRUE))
    {
      asicNewState = ONU_GPON_02_STANDBY;
    }
  }

  if (newState == ONU_GPON_05_OPERATION)
  {
      /* start onu periodic timers */ 
    if (g_periodicTimerState != MV_TRUE) 
    {
      onuPonTimerEnable(&(onuPonResourceTbl_s.onuGponEvtCleanUpTimerId));
      onuPonTimerEnable(&(onuPonResourceTbl_s.onuPonPmTimerId));
      g_periodicTimerState = MV_TRUE;
    }
  }

  /* update state */
  /* ============ */

  /* update asic */
  rcode = mvOnuGponMacOnuStateSet(asicNewState);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacOnuStateSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  /* update database */
  onuGponDbOnuStateSet(newState);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearOnuInfo
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function reset onu information to its default state:
**               asic & application
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearOnuInfo(void)
{
  MV_STATUS rcode;

  rcode = onuGponPonMngClearOnuPorts();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuPorts\n", __FILE_DESC__, __LINE__);    return(rcode);
  }

  rcode = onuGponPonMngClearOnuTconts();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuTconts\n", __FILE_DESC__, __LINE__);    return(rcode);
  }

  rcode = onuGponPonMngClearOnuId();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuId\n", __FILE_DESC__, __LINE__);    return(rcode);  
  }

  rcode = onuGponPonMngClearOnuDelay();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuDelay\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  rcode = onuGponPonMngClearOnuOverhead();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuOverhead\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  rcode = onuGponPonMngClearBerInterval();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearBerInterval\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearOnuPorts
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function clear onu ports information from the 
**               asic & application
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearOnuPorts(void)
{
  /* Remove Encryption from all ports */
  onuGponDbGemPortAesClearAll();
  mvOnuGponMacAesInit();

  /* Remove all gem ports */
  onuGponDbGemPortClearAll();
  mvOnuGponMacGemInit();

  /* Set OMCC To be not Valid */
  onuGponDbOmccValidSet(MV_FALSE);

  /* Remove OMCC port */
  onuGponApiGemOmccIdConfig(0, MV_FALSE);

  onuGponDbOmccPortSet(0);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearOnuTconts
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function clear onu tcont information from the 
**               asic & application
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearOnuTconts(void)
{
  MV_STATUS rcode = MV_OK;

  rcode = onuGponAllocIdDeAssignAll();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponAllocIdDeAssignAll\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearOnuId
**  ____________________________________________________________________________
**
**  DESCRIPTION:   The function clear onu Id information from the 
**                 asic & application
**                 
**  PARAMETERS:    None
**
**  OUTPUTS:       None
**
**  RETURNS:       MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearOnuId(void)
{
  MV_STATUS rcode;

  /* update asic */
  rcode = onuGponSrvcOnuIdUpdate(ONU_GPON_UNDEFINED_ONU_ID, MV_FALSE);
  if (rcode != MV_OK)
  {
    return(rcode);
  }

  /* update database */
  onuGponDbOnuIdSet(ONU_GPON_UNDEFINED_ONU_ID);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearOnuDelay
**  ____________________________________________________________________________
**
**  DESCRIPTION:   The function clear onu delay information from the 
**                 asic & application
**                 
**  PARAMETERS:    None
**
**  OUTPUTS:       None
**
**  RETURNS:       MV_OK or error
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearOnuDelay(void)
{
  MV_STATUS rcode;

  /* update asic */
  rcode = mvOnuGponMacRxEqualizationDelaySet(GPON_TX_EQUAL_DELAY_TD);
  if (rcode != MV_OK)
  {
    return(rcode);
  }

  rcode = mvOnuGponMacTxFinalDelaySet(GPON_TX_FINAL_DELAY_FD);
  if (rcode != MV_OK)
  {
    return(rcode);
  }

  /* update database */
  onuGponDbEqualizationDelaySet(GPON_TX_EQUAL_DELAY_TD);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearOnuOverhead
**  ____________________________________________________________________________
**
**  DESCRIPTION:   The function clear onu overhead information from the 
**                 asic & application
**                 
**  PARAMETERS:    None
**
**  OUTPUTS:       None
**
**  RETURNS:       MV_OK
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearOnuOverhead(void)
{
  MV_STATUS rcode;

  if (g_overheadManualMode == MV_TRUE)
  {
    return(MV_OK);
  }

  /* update asic */
  rcode = mvOnuGponMacPreambleSet(ONU_TX_PREAMBLE_TYPE_01_P, ONU_TX_PREAMBLE_TYPE_01_PC,
                                ONU_TX_PREAMBLE_TYPE_02_P, ONU_TX_PREAMBLE_TYPE_02_PC, 
                                ONU_TX_PREAMBLE_TYPE_03_P, ONU_TX_PREAMBLE_TYPE_03_DEF_PC);
  if (rcode != MV_OK)
  {
    return(rcode);
  }

  rcode = mvOnuGponMacTxDelimiterSet(GPON_TX_DELIMITER, GPON_TX_DELIMITER_DS);
  if (rcode != MV_OK)
  {
    return(rcode);
  }

  /* update database */
  onuGponDbPreambleSet (ONU_GPON_PREM_TYPE_01, ONU_TX_PREAMBLE_TYPE_01_P, ONU_TX_PREAMBLE_TYPE_01_PC);
  onuGponDbPreambleSet (ONU_GPON_PREM_TYPE_02, ONU_TX_PREAMBLE_TYPE_02_P, ONU_TX_PREAMBLE_TYPE_02_PC);
  onuGponDbPreambleSet (ONU_GPON_PREM_TYPE_03, ONU_TX_PREAMBLE_TYPE_03_P, ONU_TX_PREAMBLE_TYPE_03_DEF_PC);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngClearBerInterval
**  ____________________________________________________________________________
**
**  DESCRIPTION:   The function clear ber interval information from the 
**                 asic & application
**                 
**  PARAMETERS:    None
**
**  OUTPUTS:       None
**
**  RETURNS:       MV_OK
**
*******************************************************************************/
MV_STATUS onuGponPonMngClearBerInterval(void)
{
  MV_STATUS rcode;

  rcode = mvOnuGponMacBipInterruptIntervalSet(GPON_BIP_PERIOD_CNTR);
  if (rcode != MV_OK)
  {
    return(rcode);
  }

  /* Update S/W Database */
  onuGponDbBerIntervalSet(GPON_BIP_PERIOD_CNTR);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPonMngIsrNotExpected
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when a combination of state and event
**               is not expected                                             
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
void onuGponPonMngIsrNotExpected(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) not expected, onuId(%d), msgId(%d), state(%d)\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet());
#endif /* MV_GPON_DEBUG_PRINT */
}

/*******************************************************************************
**
**  onuGponPonMngOverheadMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when upstream overhead message is
**               received (message 01)                                             
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
void onuGponPonMngOverheadMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;
  MV_U8     totalPlouBytes;
  MV_U8     numOfGaurdBits;
  MV_U8     preambleType1Cnt;
  MV_U8     preambleType2Cnt;
  MV_U8     preambleType3Cnt;
  MV_U8     preambleType3Pattern;
  MV_U8     delimiterByte_01;
  MV_U8     delimiterByte_02;
  MV_U8     delimiterByte_03;
  MV_U32    delimiter;
  MV_U8     overheadStatus;
  MV_U32    preAssignDelay;
  MV_U32    preAssignDelayTemp;
  MV_U32    finalDelay;       
  MV_U32    equalizationDelay;
  MV_U32    extendedPreambleSyncSize;
  MV_U32    extendedPreambleOperSize;
  MV_BOOL   extendedBurstOverride;
  MV_BOOL   delimiterOverride;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_OVERHEAD_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) OVERHEAD, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  if (g_overheadManualMode == MV_FALSE)
  {
    /* Support 1.244 G Rate */
    totalPlouBytes = GPON_US_1244_PLOU_TOTAL_BYTES_SIZE;

    /* guard handling */
    /* ============== */
    numOfGaurdBits = msgData[0];

    /* preamble handling */
    /* ================= */
    preambleType1Cnt = msgData[1];         /* ploam message byte 4 */
    preambleType2Cnt = msgData[2];         /* ploam message byte 5 */
    preambleType3Pattern = msgData[3];     /* ploam message byte 6 */

    preambleType1Cnt /= UINT8_NUM_OF_BITS; /* preamble type 1 count in bytes */      
    preambleType2Cnt /= UINT8_NUM_OF_BITS; /* preamble type 2 count in bytes */

    extendedBurstOverride = onuGponDbExtendedBurstOverrideGet();

    if (extendedBurstOverride == MV_TRUE)
    {
      extendedPreambleSyncSize = onuGponDbExtendedBurstSyncOverrideValueGet();
      extendedPreambleOperSize = onuGponDbExtendedBurstOperOverrideValueGet();

      onuGponDbExtPreambleSyncSet(extendedPreambleSyncSize);
      onuGponDbExtPreambleOperSet(extendedPreambleOperSize);

      preambleType3Cnt = ONU_TX_PREAMBLE_TYPE_03_DEF_PC;
    }
    else
    {
      /* preamble type 3 count in bytes */
      /* Total Time = Guard Time + Preamble Time(Types 1, 2, 3) + Delimiter Time */
      preambleType3Cnt = totalPlouBytes - GPON_TX_DELIMITER_DS - preambleType1Cnt - 
                         preambleType2Cnt - (numOfGaurdBits / UINT8_NUM_OF_BITS);

      onuGponDbExtPreambleOperSet(preambleType3Cnt);
      onuGponDbExtPreambleSyncSet(preambleType3Cnt);
    }

    /* update database */
    onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_01, (MV_U32)ONU_TX_PREAMBLE_TYPE_01_P, (MV_U32)preambleType1Cnt);
    onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_02, (MV_U32)ONU_TX_PREAMBLE_TYPE_02_P, (MV_U32)preambleType2Cnt);
    onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_03, (MV_U32)preambleType3Pattern, (MV_U32)preambleType3Cnt);       

    /* delimiter handling */
    /* ================== */   
     delimiterOverride = onuGponDbDelimiterOverrideGet();

    if (delimiterOverride == MV_FALSE)
    {
      delimiterByte_01 = msgData[4]; /* ploam message byte 7 */
      delimiterByte_02 = msgData[5]; /* ploam message byte 8 */
      delimiterByte_03 = msgData[6]; /* ploam message byte 9 */
      delimiter = (((MV_U32)delimiterByte_01) << 16) | (((MV_U32)delimiterByte_02) << 8) |
                  ((MV_U32)delimiterByte_03);

      /* update database */
      onuGponDbDelimiterSet (ONU_GPON_DELM_BYTE_01, (MV_U32)delimiterByte_01); 
      onuGponDbDelimiterSet (ONU_GPON_DELM_BYTE_02, (MV_U32)delimiterByte_02); 
      onuGponDbDelimiterSet (ONU_GPON_DELM_BYTE_03, (MV_U32)delimiterByte_03); 
      onuGponDbDelimiterSizeSet (GPON_TX_DELIMITER_DS);

    }
    else
    {
      delimiter = onuGponDbDelimiterOverrideValueGet();

      /* update database with override value */
      onuGponDbDelimiterSet (ONU_GPON_DELM_BYTE_01, (delimiter & 0xFF)); 
      onuGponDbDelimiterSet (ONU_GPON_DELM_BYTE_02, (delimiter >> 8)& 0xFF); 
      onuGponDbDelimiterSet (ONU_GPON_DELM_BYTE_03, (delimiter >> 16)& 0xFF);

    }

    mvOnuGponMacTxDelimiterSet(delimiter, GPON_TX_DELIMITER_DS);

    /* status handling */
    /* =============== */
    overheadStatus = msgData[7]; /* ploam message byte 10 */

    if (overheadStatus & ONU_GPON_OVER_MSG_STATUS_XX)
    {/* future use */
    }
    if (overheadStatus & ONU_GPON_OVER_MSG_STATUS_M)  /* Serial Number Mask Enabled*/
    {
      onuGponDbSerialNumberMaskEnableSet(MV_TRUE);
    }
    if (overheadStatus & ONU_GPON_OVER_MSG_STATUS_SS)
    {/* future use */
    }
    if (overheadStatus & ONU_GPON_OVER_MSG_STATUS_PP)
    {/* future use */
    }

    /* pre-assign equalization delay handling */
    /* ====================================== */
    if (overheadStatus & ONU_GPON_OVER_MSG_STATUS_E)
    {
      preAssignDelayTemp  = msgData[8] << UINT8_OFFSET; /* ploam message byte 11 */
      preAssignDelayTemp |= msgData[9];                 /* ploam message byte 12 */

      preAssignDelayTemp <<= UINT8_OFFSET;              /* align to bytes from words */

      /* save pre-assign delay */
      preAssignDelay = preAssignDelayTemp;

      /* calc delay */
      finalDelay        = M_ONU_GPON_RANG_MSG_FINAL_DELAY(preAssignDelayTemp);
      equalizationDelay = M_ONU_GPON_RANG_MSG_EQUAL_DELAY(preAssignDelayTemp);

      /* update asic */
      rcode = mvOnuGponMacRxEqualizationDelaySet(equalizationDelay);
      if (rcode != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) mvOnuGponMacRxEqualizationDelaySet equalizationDelay(%d)\n",
                    __FILE_DESC__, __LINE__, equalizationDelay);;
        return;
      }

      /* update asic */
      finalDelay |= (MV_U32)GPON_TX_FINAL_DELAY_FD;

      rcode = mvOnuGponMacTxFinalDelaySet(finalDelay);
      if (rcode != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) mvOnuGponMacTxFinalDelaySet finalDelay(%d)\n",
                   __FILE_DESC__, __LINE__, finalDelay); 
        return;
      }

      /* update database */
      onuGponDbEqualizationDelaySet(preAssignDelay);
    }
  } /* g_overheadManualMode */

  /* alarm handling */
  /* ============== */

  /* cancel deactivate onu alarm (if enabled) */
  onuGponAlarmSet(ONU_GPON_ALARM_DACT, ONU_GPON_ALARM_OFF);

  /* T01 timer handling */
  /* ================== */

  /* start onu gpon pon mng T01 timer */ 
  onuPonTimerEnable(&(onuPonResourceTbl_s.onuGponT01_TimerId));

  /* state handling */
  /* ============== */

  /* Before changing state check SN_Mask VALUE and serialNumberMaskDefaultStateFlag mode.
  ** Add Support for HW and SW state machine */
  rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_03_SERIAL_NUM);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngrUpdateState(3)\n", __FILE_DESC__, __LINE__);
    return;
  }

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngSerialNumberMaskMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when serial number mask message is
**               received (message 02)
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
void onuGponPonMngSerialNumberMaskMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;
  MV_U8     numOfValidMaskBits;
  MV_U8     numOfValidMaskBytes;
  MV_U8     serialNumber[8];
  MV_U8     maskIndex;
  MV_U8     maskShift;
  MV_U8     lastByteMask;
  MV_U8     serialNumIndex = 7;
  MV_U8     maskMsgOffset  = 8;

  if (onuGponDbSerialNumberMaskEnableGet() == MV_TRUE)
  {
    /* Get Serial Number */
    onuGponDbSerialNumGet(&(serialNumber[0]));

#ifdef MV_GPON_DEBUG_PRINT
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
               "DEBUG: (%s:%d) SN_MASK, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x  ]\n",
               __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
               msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
               msgData[5], msgData[6], msgData[7], msgData[8]);
#endif /* MV_GPON_DEBUG_PRINT */

    /* numOfValidMaskBits handling */
    numOfValidMaskBits  = msgData[0];
    numOfValidMaskBytes = (numOfValidMaskBits / UINT8_NUM_OF_BITS)+1;

    /* Creating the last byte mask -if needed*/
    maskIndex = ((maskMsgOffset-numOfValidMaskBytes)+1);
    if (numOfValidMaskBits <= UINT8_NUM_OF_BITS)
    {
      maskShift = numOfValidMaskBits;
    }
    else
    {
      maskShift = (numOfValidMaskBits % UINT8_NUM_OF_BITS);
    }
    if (maskShift != 0)
    {
      lastByteMask= ((1 << (maskShift))-1);
      msgData[maskIndex] &= lastByteMask;
      serialNumber[maskIndex-1] &= lastByteMask;
    }

    /* Compering the active bytes in the Serial number mask to ONU's serial number*/
    for (maskIndex=0;maskIndex < numOfValidMaskBytes;maskIndex++)
    {
      if (serialNumber[serialNumIndex-maskIndex] != msgData[maskMsgOffset-maskIndex])
      {
        /* updating serial number flag to - no match */
        onuGponDbSerialNumberMaskMatchSet(MV_FALSE);

        /* state handling */
        rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_03_SERIAL_NUM);
        if (rcode != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                     "ERROR: (%s:%d) onuGponPonMngrUpdateState(3)\n", __FILE_DESC__, __LINE__);
          return;
        }
        return;
      }
    }

    /* updating serial number flag to - match */
    onuGponDbSerialNumberMaskMatchSet(MV_TRUE);

    /* state handling */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_03_SERIAL_NUM);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) onuGponPonMngrUpdateState(3)\n", __FILE_DESC__, __LINE__);
      return;
    }
  }
}

/*******************************************************************************
**
**  onuGponPonMngOnuIdMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when onu Id message is received   
**               (message 03)                                                                                            
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
void onuGponPonMngOnuIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;
  MV_U8     serialNum[8];
  MV_U32    msgOnuId;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_ONU_ID_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) ASSIGN ONU ID, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* serial number handling */
  /* ====================== */

  /* get onu serial number */
  onuGponDbSerialNumGet(&(serialNum[0]));

  /* match onu serial number with message serial number */
  if ((msgData[1] == serialNum[0]) && 
      (msgData[2] == serialNum[1]) && 
      (msgData[3] == serialNum[2]) && 
      (msgData[4] == serialNum[3]) && 
      (msgData[5] == serialNum[4]) && 
      (msgData[6] == serialNum[5]) && 
      (msgData[7] == serialNum[6]) && 
      (msgData[8] == serialNum[7]))
  {
    /* extract onuId from the message */
    if (onuGponDbOnuIdOverrideGet() == MV_FALSE)
    {
      msgOnuId = msgData[0];
    }
    else
    {
      msgOnuId = onuGponDbOnuIdOverrideValueGet();
    }

    /* update asic */
    rcode = onuGponSrvcOnuIdUpdate(msgOnuId, MV_TRUE);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) onuGponSrvcOnuIdUpdate\n", __FILE_DESC__, __LINE__);
      return;
    }

    /* update database */
    onuGponDbOnuIdSet(msgOnuId);

    /* set Default allocId according to the onuId */
    rcode = onuGponAllocIdAssign(msgOnuId);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) onuGponAllocIdAssign\n", __FILE_DESC__, __LINE__);
      return;
    }

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_04_RANGING);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) onuGponPonMngrUpdateState(4)\n", __FILE_DESC__, __LINE__);
      return;
    }
  }

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngRangeTimeMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when ranging time message is received                                                
**               (message 04)                                               
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
void onuGponPonMngRangeTimeMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;
  MV_U8     delay[4];
  MV_U32    msgDelay;
  MV_U32    msgDelayTemp;
  MV_U32    equalizationDelay;
  MV_U32    finalDelay;
  MV_U32    currentDelay;
  MV_U32    changeDelay;
  MV_U32    currFinalDelay;
  MV_BOOL   equalizationDelayOverride;
  MV_U32    onuState = onuGponDbOnuStateGet();

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_RNG_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) RANGE TIME, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuState, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  equalizationDelayOverride = onuGponDbEqualizationDelayOverrideGet();

  if ((equalizationDelayOverride == MV_FALSE) || (onuState == ONU_GPON_05_OPERATION))
  {
    /* equalization delay handling */
    /* =========================== */
    delay[0] = msgData[1]; /* MSB */
    delay[1] = msgData[2];
    delay[2] = msgData[3];
    delay[3] = msgData[4]; /* LSB */

    msgDelayTemp  = delay[3]; 
    msgDelayTemp |= delay[2] << 8; 
    msgDelayTemp |= delay[1] << 16; 
    msgDelayTemp |= delay[0] << 24;

    msgDelay = msgDelayTemp;
  }
  else
  {
    msgDelay = onuGponDbEqualizationDelayOverrideValueGet(); 
  }

  /* calc delay */
  finalDelay        = M_ONU_GPON_RANG_MSG_FINAL_DELAY(msgDelay);
  equalizationDelay = M_ONU_GPON_RANG_MSG_EQUAL_DELAY(msgDelay);

  /* sync state */
  /* ========== */
  if (onuState != ONU_GPON_05_OPERATION)
  {
    /* stop onu gpon pon mng T01 timer */ 
    onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponT01_TimerId));

    /* update asic */
    rcode = mvOnuGponMacRxEqualizationDelaySet(equalizationDelay);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) mvOnuGponMacRxEqualizationDelaySet equalizationDelay(%d)\n",
                 __FILE_DESC__, __LINE__, equalizationDelay);
      return;
    }

    /* update asic */
    finalDelay += (MV_U32)GPON_TX_FINAL_DELAY_FD;

    rcode = mvOnuGponMacTxFinalDelaySet(finalDelay);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) mvOnuGponMacTxFinalDelaySet finalDelay(%d)\n", 
                 __FILE_DESC__, __LINE__, finalDelay);
      return;
    }

    /* update database */
    onuGponDbEqualizationDelaySet(msgDelay);

    /* alarm handling */
    /* ============== */
    onuGponAlarmSet(ONU_GPON_ALARM_SUF, ONU_GPON_ALARM_OFF);

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_05_OPERATION);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) onuGponPonMngrUpdateState(5)\n", __FILE_DESC__, __LINE__);
      return;
    }

    onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_ON);

    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "======================\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "== Upstream sync On ==\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "======================\n");
  }
  /* operational state */
  /* ================= */
  else
  {
    /* get current delay */
    currentDelay = onuGponDbEqualizationDelayGet();

    /* Reduce Equlization delay */
    if (currentDelay > msgDelay)
    {
      changeDelay = currentDelay - msgDelay;
      mvOnuGponMacTxFinalDelayGet(&currFinalDelay); 

      /* Check if can change the TX Final Delay only */
      if (changeDelay <= currFinalDelay)
      {
        finalDelay = currFinalDelay - changeDelay;
      }
      else
      {
#ifdef MV_GPON_DEBUG_PRINT
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) Change Range delay while O5 - Update EqD\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

        /* calc delay */
        finalDelay        = M_ONU_GPON_RANG_MSG_FINAL_DELAY(msgDelay) + (MV_U32)GPON_TX_FINAL_DELAY_FD;
        equalizationDelay = M_ONU_GPON_RANG_MSG_EQUAL_DELAY(msgDelay);
        /* update asic */
        rcode = mvOnuGponMacRxEqualizationDelaySet(equalizationDelay);
        if (rcode != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                     "ERROR: (%s:%d) mvOnuGponMacRxEqualizationDelaySet equalizationDelay(%d)\n",
                     __FILE_DESC__, __LINE__, equalizationDelay);
          return;
        }
      }

      rcode = mvOnuGponMacTxFinalDelaySet(finalDelay);
      if (rcode != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) mvOnuGponMacTxFinalDelaySet finalDelay(%d)\n", 
                   __FILE_DESC__, __LINE__, finalDelay);
        return;
      }
      /* update database */
      onuGponDbEqualizationDelaySet(msgDelay);
    }
    else if (currentDelay < msgDelay)
    {
      changeDelay = msgDelay - currentDelay;
      mvOnuGponMacTxFinalDelayGet(&currFinalDelay); 

      /* Check if can change the TX Final Delay only */
      if (changeDelay + currFinalDelay > GPON_TX_FINAL_DELAY_MAX)
      {
        finalDelay = currFinalDelay + changeDelay;
      }
      else
      {
#ifdef MV_GPON_DEBUG_PRINT
        mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE, 
                   "DEBUG: (%s:%d) Change Range delay while O5 - Update EqD\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */
        
        /* calc delay */
        finalDelay        = M_ONU_GPON_RANG_MSG_FINAL_DELAY(msgDelay) + (MV_U32)GPON_TX_FINAL_DELAY_FD;
        equalizationDelay = M_ONU_GPON_RANG_MSG_EQUAL_DELAY(msgDelay);
        /* update asic */
        rcode = mvOnuGponMacRxEqualizationDelaySet(equalizationDelay);
        if (rcode != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                     "ERROR: (%s:%d) mvOnuGponMacRxEqualizationDelaySet equalizationDelay(%d)\n",
                     __FILE_DESC__, __LINE__, equalizationDelay);
          return;
        }
      }

      rcode = mvOnuGponMacTxFinalDelaySet(finalDelay);
      if (rcode != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) mvOnuGponMacTxFinalDelaySet finalDelay(%d)\n", 
                   __FILE_DESC__, __LINE__, finalDelay);
        return;
      }
      /* update database */
      onuGponDbEqualizationDelaySet(msgDelay);
    }
  }

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngDactOnuIdMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when deactivate onu message is received
**               (message 05)                                                
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
void onuGponPonMngDactOnuIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;
  MV_U32    onuState = onuGponDbOnuStateGet();

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_DACT_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) DEACTIVATE, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuState, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* timer T01 is running */
  if ((onuState >= ONU_GPON_02_STANDBY) && 
      (onuState <= ONU_GPON_04_RANGING))
  {
    /* stop onu gpon pon mng T01 timer */ 
    onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponT01_TimerId));
  }

  /* clear onu information */
  /* ===================== */
  rcode = onuGponPonMngClearOnuInfo();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuInfo\n", __FILE_DESC__, __LINE__);
    return;
  }

  /* alarm handling */
  /* ============== */
  onuGponAlarmSet(ONU_GPON_ALARM_DACT, ONU_GPON_ALARM_ON);

  /* state handling */
  /* ============== */
  rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_02_STANDBY);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngrUpdateState(2)\n", __FILE_DESC__, __LINE__);
    return;
  }

  if (onuState == ONU_GPON_05_OPERATION)
  {
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "=======================\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "== Upstream sync Off ==\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "=======================\n");

    /* Send Status Notification to upper layer */
    onuGponSrvcStatusNotify(GPON_ONU_STATUS_NOT_RANGED);
  }

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngDisSnMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when disable onu message is received
**               (message 06)                                                
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
void onuGponPonMngDisSnMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;
  MV_U32    onuState = onuGponDbOnuStateGet();
  MV_U32    disableStatus;
  MV_U8     msgSerialNumber[8];
  MV_U8     onuSerialNumber[8];
  MV_BOOL   isSnMatch;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) DISABLE, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuState, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  disableStatus = msgData[0];
  memcpy(msgSerialNumber, &(msgData[1]), 8);
  onuGponDbSerialNumGet(onuSerialNumber);
  if (memcmp(msgSerialNumber,onuSerialNumber,8) == 0)
  {
    isSnMatch = MV_TRUE;
  }
  else
  {
    isSnMatch = MV_FALSE;  
  }

  /* Disable */
  if ((disableStatus == GPON_ONU_DISABLE) && (isSnMatch == MV_TRUE) && 
      (onuState != ONU_GPON_07_EMERGANCY_STOP))
  {
    /* clear onu information */
    /* ===================== */
    rcode = onuGponPonMngClearOnuInfo();
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) DISABLE: onuGponPonMngClearOnuInfo\n", __FILE_DESC__, __LINE__);
      return;
    }

    /* alarm handling */
    /* ============== */
    onuGponAlarmSet(ONU_GPON_ALARM_DIS, ONU_GPON_ALARM_ON);

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_07_EMERGANCY_STOP);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) DISABLE: onuGponPonMngrUpdateState(7)\n", __FILE_DESC__, __LINE__);
      return;
    }

    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "==================\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "== ONT DISABLED ==\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "==================\n");

    if (g_onuGponDisableFunc != NULL)
    {
      g_onuGponDisableFunc(MV_TRUE);
    }

    /* Send Disable Notification to upper layer */
    onuGponSrvcDisableMsgNotify( MV_TRUE );

    /* If was ranged then now ont is not ranged - send status notification */
    if (onuState == ONU_GPON_05_OPERATION)
    {
      onuGponSrvcStatusNotify(GPON_ONU_STATUS_NOT_RANGED);
    }
  }
  /* Enable */
  else if (((disableStatus == GPON_ONU_ENABLE_ALL) ||
            ((disableStatus == GPON_ONU_ENABLE_ONU) && (isSnMatch == MV_TRUE))) &&
           (onuState == ONU_GPON_07_EMERGANCY_STOP))
  {
    /* alarm handling */
    /* ============== */
    onuGponAlarmSet(ONU_GPON_ALARM_DIS, ONU_GPON_ALARM_OFF);

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_02_STANDBY);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE,
                 "ERROR: (%s:%d) DISABLE: onuGponPonMngrUpdateState(2)\n", __FILE_DESC__, __LINE__);
      return;
    }

    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "=================\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "== ONT ENABLED ==\n");
    mvPonPrint(PON_PRINT_INFO, PON_SM_MODULE, "=================\n");

    if (g_onuGponDisableFunc != NULL)
    {
      g_onuGponDisableFunc(MV_FALSE);
    }
     
    /* Send Disable Notification to upper layer */
    onuGponSrvcDisableMsgNotify( MV_FALSE );
  }
}

/*******************************************************************************
**
**  onuGponPonMngCfgVpVcMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when config Vp/Vc message is received                                                
**               (message 07)                                                
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
void onuGponPonMngCfgVpVcMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) config Vp/Vc, onuId(%d), msgId(%d), state(%d)\n",
             __FILE_DESC__, __LINE__, onuId, msgId,  onuGponDbOnuStateGet());

  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) config Vp/Vc, function not supported\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */
} 

/*******************************************************************************
**
**  onuGponPonMngEncrptPortIdMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when encrpt port Id message is received
**               (message 08)                                                                                                
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
void onuGponPonMngEncrptPortIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS status;
  MV_BOOL   encrypted;
  MV_BOOL   gem;
  MV_U32    portId;
  MV_U8     *text[] = {"NOT Encrypted","Encrypted"};

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_ENC_PORT_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) ENCRYPTED PORT, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(),
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* Send Acknowledge PLOAM */
  status = mvOnuGponMacAcknowledgeMessageSend(onuId, msgId, msgData);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacAcknowledgeMessageSend msgId(%d)\n",
               __FILE_DESC__, __LINE__, msgId);
    return;
  }
  
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "[US PLOAM] ACKNOWLEDGE, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             onuId, msgId, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  portId    = ((MV_U32)(msgData[1]) << 4) +
              ((MV_U32)(msgData[2]) >> 4);
  encrypted = (msgData[0] & 0x01) ? MV_TRUE : MV_FALSE;
  gem       = (msgData[0] & 0x02) ? MV_TRUE : MV_FALSE;
  if (gem != MV_TRUE)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) ONT Does not support ATM\n", __FILE_DESC__, __LINE__);
    return;
  }

  /* to start AES on a port Id, it should be valid */
  if (encrypted == MV_TRUE)
  {
    if (onuGponDbGemPortValidGet(portId) != MV_TRUE)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE,
                 "ERROR: (%s:%d) SM AES: PORT ID [%d], can not be encrypted, port not valid\n", 
                 __FILE_DESC__, __LINE__, portId, text[encrypted]);
      return;
    }
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_AES_MODULE,
             "DEBUG: (%s:%d) SM AES: PORT ID [%d] - %s Encrypted\n", 
             __FILE_DESC__, __LINE__, portId, text[encrypted]);
#endif /* MV_GPON_DEBUG_PRINT */

  mvOnuGponMacAesPortIdSet(portId,encrypted);
  onuGponDbGemPortAesSet(portId,encrypted);

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngReqPassMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when request password message is received 
**               (message 09)                                                                                               
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
void onuGponPonMngReqPassMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_U8 password[10];
  MV_U8 srcOnuId;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_PASS_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) PASSWORD REQUEST, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  onuGponDbPasswordGet(password);
  srcOnuId = onuGponDbOnuIdGet();
  /* Send Upstream Password message */ 
  mvOnuGponMacPasswordMessageSend(srcOnuId, password, 3);
  
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE, 
             "[US PLOAM] PASSWORD, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             srcOnuId, msgId, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngAssignAllocIdMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when assign allocId password message 
**               is received (message 10)               
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
void onuGponPonMngAssignAllocIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS status;
  MV_U32    allocId;
  MV_U32    assignType;
  MV_BOOL   curAllocStatus;
  MV_BOOL   newAllocStatus;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_ALLOC_ID_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) ASSIGN ALLOC ID, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  allocId = (((MV_U32)(msgData[0])) << 4) + (((MV_U32)(msgData[1])) >> 4);
  assignType = msgData[2]; 

  switch (assignType)
  {
    case ONU_GPON_ATM_PAYLOAD:
#ifdef MV_GPON_DEBUG_PRINT
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type ATM - Not supported, allocId (%d)\n", __FILE_DESC__, __LINE__, allocId);
#endif /* MV_GPON_DEBUG_PRINT */
      return;
      break;
    case ONU_GPON_GEM_PAYLOAD:
#ifdef MV_GPON_DEBUG_PRINT
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type GEM, allocId (%d)\n", __FILE_DESC__, __LINE__, allocId);
#endif /* MV_GPON_DEBUG_PRINT */
      newAllocStatus = MV_TRUE;
      break;
    case ONU_GPON_DBA_PAYLOAD:
#ifdef MV_GPON_DEBUG_PRINT
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type DBA - Not supported, allocId (%d)\n", __FILE_DESC__, __LINE__, allocId);
#endif /* MV_GPON_DEBUG_PRINT */
      return;
      break;
    case ONU_GPON_DEALLOCATE: 
#ifdef MV_GPON_DEBUG_PRINT
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type DEALLOCATE, allocId (%d)\n", __FILE_DESC__, __LINE__, allocId);
#endif /* MV_GPON_DEBUG_PRINT */
      newAllocStatus = MV_FALSE;
      break;
    default:
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type - Not supported, allocId (%d) type(%d)\n", __FILE_DESC__, __LINE__, allocId, assignType);
      return;
  }
  /* Scan the alloc-Id table and check if the Alloc-Id has been defined and is valid */
  curAllocStatus = onuGponDbBwAllocExist(allocId);
  if (curAllocStatus == newAllocStatus)
  {
#ifdef MV_GPON_DEBUG_PRINT
    if (assignType == ONU_GPON_GEM_PAYLOAD) 
    {
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type - exist, allocId (%d)\n", __FILE_DESC__, __LINE__, allocId);
    }
    else if (assignType == ONU_GPON_DEALLOCATE) 
    {
      mvPonPrint(PON_PRINT_DEBUG, PON_SM_ALLOC_MODULE,
                 "DEBUG: (%s:%d) Alloc Id Type - removed, allocId (%d)\n", __FILE_DESC__, __LINE__, allocId);
    }
#endif /* MV_GPON_DEBUG_PRINT */
  }
  else
  {
    /* Assign new Alloc Id */
    if (newAllocStatus == MV_TRUE)
    {
      status = onuGponAllocIdAssign(allocId);
      if (status != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) onuGponAllocIdAssign onuId(%d), allocId(%d)\n", __FILE_DESC__, __LINE__, onuId, allocId);
      }
    }
    /* De-Assign exist Alloc Id */
    else
    {
      status = onuGponAllocIdDeAssign(allocId);
      if (status != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                   "ERROR: (%s:%d) onuGponAllocIdDeAssign onuId(%d), allocId(%d)\n", __FILE_DESC__, __LINE__, onuId, allocId);
      }
    }
  }

  /* Send Acknowledge PLOAM */
  status = mvOnuGponMacAcknowledgeMessageSend(onuId, msgId, msgData);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacAcknowledgeMessageSend onuId(%d), allocId(%d)\n", __FILE_DESC__, __LINE__, onuId, allocId);
    return;
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "[US PLOAM] ACKNOWLEDGE, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             onuId, msgId, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngNoMsgMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when downstream no message is received
**               (message 11)                                                                                                
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
void onuGponPonMngNoMsgMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) no message, onuId(%d), msgId(%d), state(%d)\n",
             __FILE_DESC__, __LINE__, onuId, msgId,  onuGponDbOnuStateGet());
#endif /* MV_GPON_DEBUG_PRINT */
}

/*******************************************************************************
**
**  onuGponPonMngPopupMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when popup message is received
**               (message 12)                                                                                                
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
void onuGponPonMngPopupMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) POPUP, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* stop onu gpon pon mng T02 timer */ 
  onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponT02_TimerId));

  if (onuId == ONU_GPON_BROADCAST_ONU_ID) /* Broadcast ONU-ID */
  {
    /* T01 timer handling */
    /* ================== */
    /* start onu gpon pon mng T01 timer */ 
    onuPonTimerEnable(&(onuPonResourceTbl_s.onuGponT01_TimerId));

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_04_RANGING);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) POPUP: onuGponPonMngrUpdateState(4)\n", __FILE_DESC__, __LINE__);
      return;
    }
  }
  else  /* ONU ID is directed */
  {
    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_05_OPERATION);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) POPUP: onuGponPonMngrUpdateState(5)\n", __FILE_DESC__, __LINE__);
      return;
    }

#ifdef MV_GPON_DEBUG_PRINT
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "========================\n");
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "== Recover from POPUP ==\n");
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "========================\n");
#endif /* MV_GPON_DEBUG_PRINT */
  }
}

/*******************************************************************************
**
**  onuGponPonMngReqKeyMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when request key message is received
**               (message 13)                                                                                                
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
void onuGponPonMngReqKeyMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS     rcode;
  MV_U8         key[16];
  MV_U32        i;
  static MV_U8  keyIndex;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_REQ_KEY_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) REQUEST KEY, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  keyIndex++;

  /* Generate new AES Key */
  onuGponSrvcAesKeyGenerate(key);

  /* Write it to asic */
  rcode = mvOnuGponMacAesKeyShadowWrite(key);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacAesKeyShadowWrite\n", __FILE_DESC__, __LINE__);
    return;
  }

  /* Send the Key 3 times */
  for (i = 0 ; i < 3 ; i++)
  {
    rcode  = mvOnuGponMacEncryptionKeyMessageSend(onuId, keyIndex,0, key);
    rcode |= mvOnuGponMacEncryptionKeyMessageSend(onuId, keyIndex,1, &(key[8]));
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) mvOnuGponMacEncryptionKeyMessageSend\n", __FILE_DESC__, __LINE__);
      return;
    }
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_AES_MODULE,
             "DEBUG: (%s:%d) SM AES: INDEX [%u] KEY [%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             __FILE_DESC__, __LINE__, keyIndex,
             key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7], 
             key[8], key[9], key[10], key[11], key[12], key[13], key[14], key[15]);
#endif /* MV_GPON_DEBUG_PRINT */

#ifdef MV_GPON_PERFORMANCE_CHECK
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
    if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngCfgPortIdMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when config portId message is received
**               (message 14)                                                                                                
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
void onuGponPonMngCfgPortIdMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS  status;
  MV_BOOL    active;
  MV_U32     portId;
  MV_U32     omciTxCmd = 0;
  MV_BOOL    omccValid;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_CFG_PORT_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) CONFIG PORT ID, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* Get Enable */
  active = (msgData[0] == 1)?MV_TRUE:MV_FALSE;  /* 0x00 - Activate, 0x01 - Deactivate */

  if (onuGponDbOmccPortOverrideGet() == MV_FALSE)
  {
    /* Get Port ID */
    /* PLOAM Octet 4 (msgData[1]) - Port-ID[11:4] */
    /* 4 MSB bits of PLOAM Octet 5 (msgData[2]) - Port-ID[3:0] */
    portId = (msgData[1] << 4) + (msgData[2] >> 4);
  }
  else
  {
    portId = onuGponDbOmccPortOverrideValueGet();
  }

  /* Send Acknowledge PLOAM */
  status = mvOnuGponMacAcknowledgeMessageSend(onuId, msgId, msgData);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacAcknowledgeMessageSend, msgId(%d) , active(%d)\n", 
               __FILE_DESC__, __LINE__, msgId, active);
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "[US PLOAM] ACKNOWLEDGE, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             onuId, msgId, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* Update Tx Descriptor */
  /* ==================== */

  /* Byte 0xC */
  /* +--------+--------+--------+--------+--------+--------+--------+--------+ */
  /* | 31-30  | 29-20  |  19-8  |   7    |  6-4   |   3    |  2-1   |   0    | */
  /* |  DSA   |  MOD   |  GEM   | RSRVD  |  MH    |  PKT   | ERROR  | ERROR  | */
  /* |  TAG   |  CMD   |  PORT  |        | SELECT |  COLOR | CODING | SUMMARY| */
  /* +--------+--------+--------+--------+--------+--------+--------+--------+ */
  /*     0        0     Port ID     0        1         0        0        0     */
  omciTxCmd = ((portId << 8) | 0x0010);
  mv_pon_ctrl_omci_tx_cmd(omciTxCmd);

  /* Get OMCC State */
  omccValid = onuGponDbOmccValidGet();
  if (omccValid == active)
  {
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_OMCC_MODULE,
               "DEBUG: (%s:%d) Configure Port Id - Activness not changed, portId(%d), active(%d), omccValid(%d)\n", 
               __FILE_DESC__, __LINE__, portId, active, omccValid);
    return;
  }

  /* Update the new OMCC State */
  onuGponDbOmccValidSet(active);

  /* Set OMCC Port Id */
  status = onuGponApiGemOmccIdConfig(portId, active);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponApiGemOmccIdConfig portId(%d) active(%d)\n", 
               __FILE_DESC__, __LINE__, portId, active);
  }

  onuGponDbOmccPortSet(portId);

  /* Send OMCC Notification to upper layer */
  onuGponSrvcOmccNotify(portId);

  if (active == MV_TRUE)
  {
#ifdef MV_GPON_DEBUG_PRINT
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_OMCC_MODULE,
               "DEBUG: (%s:%d) GPON SM: OMCC ACTIVE: GEM Port[%d]\n", __FILE_DESC__, __LINE__, portId);
#endif /* MV_GPON_DEBUG_PRINT */

    /* Upstream ON - Turn LED On */
    onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_ON);
  }
  else
  {
#ifdef MV_GPON_DEBUG_PRINT
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_OMCC_MODULE, 
               "DEBUG: (%s:%d) GPON SM: OMCC DEACTIVE\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

    /* Upstream OFF - Blink the LED (Downstream only) */
    onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_BLINK_SLOW);
  }
#ifdef MV_GPON_PERFORMANCE_CHECK
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
    if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngPhyEquErrMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when physical equipt error message is 
**               received (message 15)                                                                                                
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
void onuGponPonMngPhyEquErrMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) PEE, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(),
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* Stop PEE Timer */
  onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponPeeTimerId));

  /* Start PEE Timer */
  onuPonTimerEnable(&(onuPonResourceTbl_s.onuGponPeeTimerId));

  /* Generate PEE Alarm */
  onuGponAlarmSet(ONU_GPON_ALARM_PEE, ONU_GPON_ALARM_ON);
}

/*******************************************************************************
**                            
**  onuGponPonMngChgPwrLvlMsg 
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when change power level message is 
**               received (message 16)                                                                                                
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
void onuGponPonMngChgPwrLvlMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) change power level, onuId(%d), msgId(%d), state(%d)\n",
             __FILE_DESC__, __LINE__, onuId, msgId,  onuGponDbOnuStateGet());

  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) change power level, function not supported\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */
}

/*******************************************************************************
**
**  onuGponPonMngPstMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when PST message is received
**               (message 17)                                                                                                
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
void onuGponPonMngPstMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) pst, onuId(%d), msgId(%d), state(%d)\n",
             __FILE_DESC__, __LINE__, onuId, msgId,  onuGponDbOnuStateGet());

  mvPonPrint(PON_PRINT_DEBUG, PON_SM_DEBUG_MODULE,
             "DEBUG: (%s:%d) pst, function not supported\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */
}

/*******************************************************************************
**
**  onuGponPonMngBerIntervalMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when ber interval message is received
**               (message 18)                                                                                                
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
void onuGponPonMngBerIntervalMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS status;
  MV_U32    berInterval;
  MV_U32    currentBerInterval;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) BER INTERVAL, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  /* BER Interval in Downstream Superframes */
  berInterval = (MV_U32)(msgData[0] << 24) + (MV_U32)(msgData[1] << 16) + 
                (MV_U32)(msgData[2] << 8) + (MV_U32)msgData[3]; 

  /* Send Acknowledge PLOAM */
  status = mvOnuGponMacAcknowledgeMessageSend(onuId, msgId, msgData);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) BER: mvOnuGponMacAcknowledgeMessageSend onuId(%d), msgId(%d)\n",
               __FILE_DESC__, __LINE__, onuId, msgId);
    return;
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "[US PLOAM] ACKNOWLEDGE, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             onuId, msgId, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  currentBerInterval = onuGponDbBerIntervalGet();
  if (currentBerInterval == berInterval)
  {
    return;
  } 

  status = mvOnuGponMacBipInterruptIntervalSet(berInterval);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) BER: mvOnuGponMacBipInterruptIntervalSet onuId(%d), msgId(%d), interval(%d)\n",
               __FILE_DESC__, __LINE__, onuId, msgId, berInterval);
    return;
  }

  /* Update S/W Database */
  onuGponDbBerIntervalSet(berInterval);
}

/*******************************************************************************
**
**  onuGponPonMngKeySwitchTimeMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when key switch time message is received
**               (message 19)                                                                                                
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
void onuGponPonMngKeySwitchTimeMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS status;
  MV_U32    time;
  MV_U8     realOnuId;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_KEY_SWITCH_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "DEBUG: (%s:%d) KEY SWITCHING TIME, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuGponDbOnuStateGet(), 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  time = (MV_U32)(msgData[3]) + (((MV_U32)(msgData[2])) << 8) + (((MV_U32)(msgData[1])) << 16) + 
         (((MV_U32)((msgData[0]) & 0x3F)) << 24); 

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_AES_MODULE,
             "DEBUG: (%s:%d) SM AES: SWITCHING TIME [0x%x (%u)]\n",
             __FILE_DESC__, __LINE__, time, time);
#endif /* MV_GPON_DEBUG_PRINT */

  realOnuId = (MV_U8)onuGponDbOnuIdGet();

  /* Send Acknowledge PLOAM */
  status = mvOnuGponMacAcknowledgeMessageSend(realOnuId, msgId, msgData);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacAcknowledgeMessageSend\n", __FILE_DESC__, __LINE__);
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "[US PLOAM] ACKNOWLEDGE, onuId(%d), msgId(%d), msg[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n",
             realOnuId, msgId, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

#ifdef MV_GPON_PERFORMANCE_CHECK
    asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                             &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
    if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngExtBurstMsg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when ext burst message is received
**               (message 20)                                                                                                
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
void onuGponPonMngExtBurstMsg(MV_U8 onuId, MV_U8 msgId, MV_U8 *msgData)
{
  MV_STATUS  rcode;
  MV_U32     onuState = onuGponDbOnuStateGet();
  MV_U8      preambleCnt_03Oper;
  MV_U8      preambleCnt_03Sync;
  MV_U32     currentExtPreambleSync;
  MV_U32     currentExtPreambleOper;
  MV_BOOL    burstOverride;

#ifdef MV_GPON_PERFORMANCE_CHECK
  S_GponPerformanceCheckNode *tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[PON_EXT_BURST_PLOAM_PERFORMANCE]);

  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStart[tmpPmCheckNode->uSecCntIdx]), 0);
#endif /* MV_GPON_PERFORMANCE_CHECK */

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_STATE_MODULE,
             "INFO: (%s:%d) EXT BURST, onuId(%d), msgId(%d), state(%d) msg[0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
             __FILE_DESC__, __LINE__, onuId, msgId, onuState, 
             msgData[0], msgData[1], msgData[2], msgData[3], msgData[4], 
             msgData[5], msgData[6], msgData[7], msgData[8], msgData[9]);
#endif /* MV_GPON_DEBUG_PRINT */

  currentExtPreambleSync = onuGponDbExtPreambleSyncGet();
  currentExtPreambleOper = onuGponDbExtPreambleOperGet();
  if (g_overheadManualMode == MV_TRUE)                 
  {                                                     
    return;                                             
  }                                                     
  burstOverride = onuGponDbExtendedBurstOverrideGet();

  if (burstOverride == MV_TRUE)
  {
    preambleCnt_03Sync = onuGponDbExtendedBurstSyncOverrideValueGet();
    preambleCnt_03Oper = onuGponDbExtendedBurstOperOverrideValueGet();

    onuGponDbExtPreambleSyncSet(preambleCnt_03Sync);
    onuGponDbExtPreambleOperSet(preambleCnt_03Oper);
  }
  else
  {
    /* preamble handling */
    /* ================= */
    preambleCnt_03Sync = msgData[0];
    preambleCnt_03Oper = msgData[1];

    if ((currentExtPreambleSync == preambleCnt_03Sync) && 
        (currentExtPreambleOper == preambleCnt_03Oper))
    {
      return;
    }

    onuGponDbExtPreambleSyncSet(preambleCnt_03Sync);
    onuGponDbExtPreambleOperSet(preambleCnt_03Oper);
  }
  rcode = onuGponPonMngPreambleSet(onuState);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) Preamble Set\n", __FILE_DESC__, __LINE__);
  }

#ifdef MV_GPON_PERFORMANCE_CHECK
  asicOntGlbRegReadNoCheck(mvAsicReg_GPON_GEN_MICRO_SEC_CNT, 
                           &(tmpPmCheckNode->uSecCntStop[tmpPmCheckNode->uSecCntIdx]), 0);
  if(tmpPmCheckNode->uSecCntIdx < 255) tmpPmCheckNode->uSecCntIdx++;
#endif /* MV_GPON_PERFORMANCE_CHECK */
}

/*******************************************************************************
**
**  onuGponPonMngTimerT01Hndl
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when timer T01 expires                                                
**               
**  PARAMETERS:  unsigned long data
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngTimerT01Hndl(unsigned long data)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_TIMER_MODULE,
             "DEBUG: (%s:%d) onuGponPonMngTimerT01Hndl\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);   /* lock GPON interrupt */

  onuPonResourceTbl_s.onuGponT01_TimerId.onuPonTimerActive = ONU_PON_TIMER_NOT_ACTIVE;
  onuGponPonMngTimerT01ExpireHndl();
  /* stop onu gpon pon mng T01 timer */
  onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponT01_TimerId));

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock GPON interrupt */
}

/*******************************************************************************
**
**  onuGponPonMngTimerT01ExpireHndl
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function executes timer T01 functionality
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
void onuGponPonMngTimerT01ExpireHndl(void)
{
  MV_STATUS rcode;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_TIMER_MODULE,
             "DEBUG: (%s:%d) TIMER TO1 Expired, state(%d)\n", __FILE_DESC__, __LINE__, onuGponDbOnuStateGet());
#endif /* MV_GPON_DEBUG_PRINT */

  /* state handling */
  /* ============== */
  rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_02_STANDBY);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngrUpdateState(2)\n", __FILE_DESC__, __LINE__);
    return;
  }

  /* clear onu information */
  /* ===================== */
  rcode = onuGponPonMngClearOnuInfo();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuInfo\n", __FILE_DESC__, __LINE__);
    return;
  }

  /* alarm handling */
  /* ============== */
  onuGponAlarmSet(ONU_GPON_ALARM_SUF, ONU_GPON_ALARM_ON);
}

/*******************************************************************************
**
**  onuGponPonMngTimerT02Hndl
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when timer T02 expires                                                
**               
**  PARAMETERS:  unsigned long data
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngTimerT02Hndl(unsigned long data)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_TIMER_MODULE,
             "DEBUG: (%s:%d) onuGponPonMngTimerT02Hndl\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);   /* lock GPON interrupt */

  onuPonResourceTbl_s.onuGponT02_TimerId.onuPonTimerActive = ONU_PON_TIMER_NOT_ACTIVE;
  onuGponPonMngTimerT02ExpireHndl();
  /* stop onu gpon pon mng T02 timer */
  onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponT02_TimerId));

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock GPON interrupt */
}

/*******************************************************************************
**
**  onuGponPonMngTimerT02ExpireHndl
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function executes timer T02 functionality                                                
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
void onuGponPonMngTimerT02ExpireHndl(void)
{
  MV_STATUS rcode;
  MV_BOOL   downstreamSync;

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_TIMER_MODULE,
             "DEBUG: (%s:%d) TIMER TO2 Expired, state(%d)\n", __FILE_DESC__, __LINE__, onuGponDbOnuStateGet());
#endif /* MV_GPON_DEBUG_PRINT */

  /* clear onu information */
  /* ===================== */
  rcode = onuGponPonMngClearOnuInfo();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngClearOnuInfo\n", __FILE_DESC__, __LINE__);
    return;
  }

  /* state handling */
  /* ============== */
  if (onuGponAlarmGet(ONU_GPON_ALARM_LOF) == ONU_GPON_ALARM_ON)
  {
    downstreamSync = MV_FALSE;
  }
  else
  {
    downstreamSync = MV_TRUE;
  }

  if (downstreamSync == MV_FALSE)
  {
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_01_INIT);
    if (rcode != MV_OK) 
    {
      /* start xvr reset timer */
      onuGponIsrXvrResetStateSet(MV_TRUE);
    }
  }
  else
  {
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_02_STANDBY);
  }
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) onuGponPonMngrUpdateState(1|2)\n", __FILE_DESC__, __LINE__);
    return;

    /* stop xvr reset timer */
    onuGponIsrXvrResetStateSet(MV_FALSE);
  }

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "================\n");
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "== POPUP Over ==\n");
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "================\n");
#endif /* MV_GPON_DEBUG_PRINT */
}

/*******************************************************************************
**
**  onuGponPonMngTimerPeeHndl
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when timer Pee expires                                                
**               
**  PARAMETERS:  unsigned long data
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngTimerPeeHndl(unsigned long data)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_TIMER_MODULE,
             "DEBUG: (%s:%d) onuGponPonMngTimerPeeHndl\n", __FILE_DESC__, __LINE__);
#endif /* MV_GPON_DEBUG_PRINT */

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);   /* lock GPON interrupt */

  onuPonResourceTbl_s.onuGponPeeTimerId.onuPonTimerActive = ONU_PON_TIMER_NOT_ACTIVE;
  onuGponPonMngTimerPeeExpireHndl();
  /* stop onu gpon pon mng Pee timer */
  onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponPeeTimerId));

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock GPON interrupt */
}

/*******************************************************************************
**
**  onuGponPonMngTimerPeeExpireHndl
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function executes timer PEE functionality                                                
**               
**  PARAMETERS:  None  
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngTimerPeeExpireHndl(void)
{
#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_TIMER_MODULE,
            "DEBUG: (%s:%d) TIMER PEE Expired, state(%d)\n",
             __FILE_DESC__, __LINE__, onuGponDbOnuStateGet());
#endif /* MV_GPON_DEBUG_PRINT */

  /* Cancel PEE Alarm */
  onuGponAlarmSet(ONU_GPON_ALARM_PEE, ONU_GPON_ALARM_OFF);
}

/******************************************************************************/
/* ========================================================================== */
/*                         Alarm Section                                      */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponPonMngGenCritAlarm
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function generate critical alarm                                                
**               
**  PARAMETERS:  E_OnuGponAlarmType alarmType_e
**               MV_U8              dummyVal 
**               MV_U8              *dummyPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngGenCritAlarm(E_OnuGponAlarmType alarmType_e, 
                               MV_U8 dummyVal,
                               MV_U8 *dummyPtr)
{
  MV_STATUS rcode;
  MV_U32    currentState;
  MV_U32    onuId;

  onuId        = onuGponDbOnuIdGet();
  currentState = onuGponDbOnuStateGet();

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE,
             "DEBUG: (%s:%d) ALARM ON - (%d), onuId(%d), state(%d)\n", 
             __FILE_DESC__, __LINE__, alarmType_e, onuId, currentState);
#endif /* MV_GPON_DEBUG_PRINT */

  /* Operation State */
  /* =============== */
  if (currentState == ONU_GPON_05_OPERATION)
  {
    /* special care for operation state */
    /* move to popup state - start T02 timer */

    /* T02 timer handling */
    /* ================== */

    /* start onu gpon pon mng T02 timer */ 
    onuPonTimerEnable(&(onuPonResourceTbl_s.onuGponT02_TimerId));

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_06_POPUP);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) Set onu state to state 06\n", __FILE_DESC__, __LINE__);
      return;
    }

#ifdef MV_GPON_DEBUG_PRINT        
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "===========\n");
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "== POPUP ==\n");
    mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, "===========\n");
#endif /* MV_GPON_DEBUG_PRINT */  
  }

  /* Ranging States */
  /* ============== */
  else if ((currentState >= ONU_GPON_01_INIT) &&
           (currentState <= ONU_GPON_04_RANGING))
  {
    /* timer T01 is running */
    if ((currentState >= ONU_GPON_02_STANDBY) && 
        (currentState <= ONU_GPON_04_RANGING))
    {
      /* stop onu gpon pon mng T01 timer */ 
      onuPonTimerDisable(&(onuPonResourceTbl_s.onuGponT01_TimerId));
    }

    /* clear onu information */
    /* ===================== */
    rcode = onuGponPonMngClearOnuInfo();
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) Clear onu information\n", __FILE_DESC__, __LINE__);
      return;
    }

    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_01_INIT);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) Set onu state to state 01\n",__FILE_DESC__, __LINE__);
      return;
    }

    /* start xvr reset timer */
    onuGponIsrXvrResetStateSet(MV_TRUE);
  }

  /* alarm handling */
  /* ============== */
  onuGponAlarmSet(alarmType_e, ONU_GPON_ALARM_ON);
}

/*******************************************************************************
**
**  onuGponPonMngCanCritAlarm
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function cancel critical alarm                                                
**               
**  PARAMETERS:  E_OnuGponAlarmType alarmType_e
**               MV_U8              dummyVal 
**               MV_U8              *dummyPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngCanCritAlarm(E_OnuGponAlarmType alarmType_e, 
                               MV_U8 dummyVal,
                               MV_U8 *dummyPtr)
{
  MV_STATUS rcode;
  MV_U32    currentState;
  MV_U32    onuId;

  onuId        = onuGponDbOnuIdGet();
  currentState = onuGponDbOnuStateGet();

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE,
             "DEBUG: (%s:%d) ALARM OFF - (%d), onuId(%d), state(%d)\n", 
             __FILE_DESC__, __LINE__, alarmType_e, onuId, currentState);
#endif /* MV_GPON_DEBUG_PRINT */

  /* alarm handling */
  /* ============== */
  onuGponAlarmSet(alarmType_e, ONU_GPON_ALARM_OFF);

  if ((onuGponAsicAlarmStatusGet() == ONU_GPON_ALARM_OFF) &&
      currentState == ONU_GPON_01_INIT)
  {
    /* state handling */
    /* ============== */
    rcode = onuGponPonMngrUpdateState((MV_U32)ONU_GPON_02_STANDBY);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
                 "ERROR: (%s:%d) Set onu state to state 02\n", __FILE_DESC__, __LINE__);
      return;
    }

    /* stop xvr reset timer */
    onuGponIsrXvrResetStateSet(MV_FALSE);
  }
}

/*******************************************************************************
**
**  onuGponPonMngGenMemAlarm
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function handle mem alarm (received unsupported message)
**               
**  PARAMETERS:  E_OnuGponAlarmType alarmType_e
**               MV_U8              dummyVal 
**               MV_U8              *dummyPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngGenMemAlarm(E_OnuGponAlarmType alarmType_e, 
                              MV_U8 dummyVal,
                              MV_U8 *dummyPtr)
{
  MV_U32 currentState;
  MV_U32 onuId;

  onuId        = onuGponDbOnuIdGet();
  currentState = onuGponDbOnuStateGet();

#ifdef MV_GPON_DEBUG_PRINT
  mvPonPrint(PON_PRINT_DEBUG, PON_SM_MODULE, 
             "DEBUG: (%s:%d) generate mem, onuId(%d), state(%d)\n", 
             __FILE_DESC__, __LINE__, onuId, currentState);
#endif /* MV_GPON_DEBUG_PRINT */

  /* alarm handling */
  /* ============== */
  onuGponAlarmSet(alarmType_e, ONU_GPON_ALARM_ON);
}

/******************************************************************************/
/* ========================================================================== */
/*                         UTILS Routines                                     */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponPonMngPreambleSet
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function update onu preamble according to onu state
**               
**  PARAMETERS:  MV_U32 onuState
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error 
**
*******************************************************************************/
MV_STATUS onuGponPonMngPreambleSet(MV_U32 onuState)
{
  MV_STATUS rcode;
  MV_U32    type1Size;
  MV_U32    type2Size;
  MV_U32    type3Size = 0;
  MV_U32    type3Pattern;
  MV_U32    dummy;

  /* Get Preambles Parameters */
  onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_01, &dummy, &type1Size);
  onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_02, &dummy, &type2Size);
  onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_03, &type3Pattern, &dummy);
  switch (onuState)
  {
  case ONU_GPON_01_INIT:          
  case ONU_GPON_02_STANDBY: 
    break;     
  case ONU_GPON_03_SERIAL_NUM:   
  case ONU_GPON_04_RANGING:    
    type3Size = onuGponDbExtPreambleSyncGet(); 
    break;
  case ONU_GPON_05_OPERATION:     
    type3Size = onuGponDbExtPreambleOperGet();
    break;
  case ONU_GPON_06_POPUP:         
  case ONU_GPON_07_EMERGANCY_STOP: 
  default:
    break;
  }

  /* update asic */
  rcode = mvOnuGponMacPreambleSet(ONU_TX_PREAMBLE_TYPE_01_P, type1Size, 
                                  ONU_TX_PREAMBLE_TYPE_02_P, type2Size, 
                                  type3Pattern, type3Size);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_SM_MODULE, 
               "ERROR: (%s:%d) mvOnuGponMacPreambleSet type3 size(%d) type3 pattern(%d)\n",
               __FILE_DESC__, __LINE__, type3Size, type3Pattern); 
  }

  return(rcode);
}

/*******************************************************************************
**
**  onuGponPonMngOverheadManualModeSet
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function set onu overhead manual mode
**                 
**  PARAMETERS:  MV_BOOL mode
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponPonMngOverheadManualModeSet(MV_BOOL mode)
{
  g_overheadManualMode = mode;
}

/*******************************************************************************
**
**  onuGponPonMngOverheadManualModeGet
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function return onu overhead manual mode
**                 
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu overhead manual mode 
**
*******************************************************************************/
MV_BOOL onuGponPonMngOverheadManualModeGet(void)
{
  return(g_overheadManualMode);
}

/*******************************************************************************
**
**  onuGponPonMngDisableSetRegister
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu disable function
**               
**  PARAMETERS:  DISABLESTATSETFUNC disableFunc
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**           
*******************************************************************************/
MV_STATUS onuGponPonMngDisableSetRegister(DISABLESTATSETFUNC disableFunc)
{
  g_onuGponDisableFunc = disableFunc;

  return(MV_OK);
}
