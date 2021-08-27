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
**  FILE        : ponOnuInit.c                                               **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON init sequence                **
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
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuInit.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
MV_STATUS onuGponAsicDelayInit(void);
MV_STATUS onuGponAsicOverheadInit(void);
MV_STATUS onuGponAsicPortIdsInit(void);
MV_STATUS onuGponAsicBerCounterInit(void);
MV_STATUS onuGponAsicPloamParamInit(void);
MV_STATUS onuGponAsicFrameDelineationInit(void);
MV_STATUS onuGponAsicTxInit(void);
MV_STATUS onuGponAsicBurstEnableInit(void);
MV_STATUS onuGponAsicFrameLengthInit(void);
MV_STATUS onuGponAsicPortInit(void);
MV_STATUS onuGponAsicSerdesInit(void);
MV_STATUS onuGponAsicInit(MV_BOOL initTime);
MV_STATUS onuGponAppInit(void);
void      onuGponStateAndEventTblInit(void);
void      onuGponInPmInit(void);

/*******************************************************************************
**
**  onuPonSetup
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function execute onu setup init sequence
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**               MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonSetup(void)
{
  MV_STATUS rcode;

  /* init onu base address */
  rcode = onuPonBaseAddrInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Failed to init onu base address\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* init onu database */
  rcode = onuGponDbInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Failed to init onu database\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* init onu Asic */
  rcode = onuGponAsicInit(MV_TRUE);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Failed to init asic\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

#ifdef MV_GPON_PERFORMANCE_CHECK
  rcode = onuGponPmInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Failed to init pm\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }
#endif /* MV_GPON_PERFORMANCE_CHECK */

  onuGponIsrInit();
  onuGponSrvcInit();
  onuGponAllocIdInit(0x00FF);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicLedsInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu leds 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponAsicLedsInit(void)
{
  /* Turn off Led */
  onuGponLedHandler(ONU_GPON_SYNC_LED, ACTIVE_LED_OFF);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponOnuStateAndIdInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu Id and state 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicOnuStateAndIdInit(void)
{
  MV_STATUS rcode;
  MV_U32    onuState;
  MV_U32    onuId;
  MV_BOOL   onuIdEnable;

  /* ONU State Register */
  /* get onu state */
  onuState = onuGponDbOnuStateGet();
  rcode    = mvOnuGponMacOnuStateSet(onuState); 
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacOnuStateSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  /* ONU-ID Register */
  onuId       = onuGponDbOnuIdGet();
  onuIdEnable = (onuId == ONU_GPON_UNDEFINED_ONU_ID) ? MV_FALSE : MV_TRUE;
  rcode = mvOnuGponMacOnuIdSet(onuId, onuIdEnable); 
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacOnuIdSet onu Id(%d), enable(%d)\n",
               __FILE_DESC__, __LINE__, onuId, onuIdEnable);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicDelayInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu delay: const, equilization, and
**               Final
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicDelayInit(void)
{
  MV_STATUS rcode;
  MV_U32    eqDelay;
  MV_U32    finalDelay;       
  MV_U32    equalizationDelay;
  MV_U32    constDelay;
  MV_U32    snForRandomSeed;
  MV_U8     sn[8];
  
  /* TX Delay Register - const */
  constDelay = onuGponDbConstDelayGet();
  rcode      = mvOnuGponMacRxInternalDelaySet(constDelay);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacRxInternalDelaySet, constDelay(0x%x)\n",
               __FILE_DESC__, __LINE__, constDelay);
    return(rcode);
  }

  /* calc delay */
  eqDelay           = onuGponDbEqualizationDelayGet();
  finalDelay        = M_ONU_GPON_RANG_MSG_FINAL_DELAY(eqDelay);
  equalizationDelay = M_ONU_GPON_RANG_MSG_EQUAL_DELAY(eqDelay);

  /* TX Delay Register - equilization */
  rcode = mvOnuGponMacRxEqualizationDelaySet(equalizationDelay);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacRxInternalDelaySet, constDelay(0x%x)\n",
               __FILE_DESC__, __LINE__, constDelay);
    return(rcode);
  }

  /* TX Final Delay Register */
  rcode = mvOnuGponMacTxFinalDelaySet(GPON_TX_FINAL_DELAY_FD);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacTxFinalDelaySet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  /* Update two last bytes of Serial Number -Help Asic generate Random numbers 
     for S/N Request answer delay */
  onuGponDbSerialNumGet(sn);
  snForRandomSeed = sn[7] + (MV_U32)(sn[6] << 8);
  rcode = mvOnuGponMacSerialNumberSet(snForRandomSeed);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacTxFinalDelaySet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicOverheadInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu overhead parameters: Preamble, and
**               Delimiter
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicOverheadInit(void)
{
  MV_STATUS rcode;
  MV_U32    preambleType1Ctr;
  MV_U32    preambleType1Pattern;
  MV_U32    preambleType2Ctr;
  MV_U32    preambleType2Pattern;
  MV_U32    preambleType3Ctr;
  MV_U32    preambleType3Pattern;
  MV_U32    delimiterByte1;
  MV_U32    delimiterByte2;
  MV_U32    delimiterByte3;
  MV_U32    delimiter;

  onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_01, &preambleType1Pattern, &preambleType1Ctr);
  onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_02, &preambleType2Pattern, &preambleType2Ctr);
  onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_03, &preambleType3Pattern, &preambleType3Ctr);

  /* TX Preamble Register */
  rcode = mvOnuGponMacPreambleSet(ONU_TX_PREAMBLE_TYPE_01_P, preambleType1Ctr,
                                ONU_TX_PREAMBLE_TYPE_02_P, preambleType2Ctr, 
                                preambleType3Pattern, preambleType3Ctr);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacPreambleSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  /* TX Delimiter Register */
  delimiterByte1 = onuGponDbDelimiterGet(ONU_GPON_DELM_BYTE_01);
  delimiterByte2 = onuGponDbDelimiterGet(ONU_GPON_DELM_BYTE_02);
  delimiterByte3 = onuGponDbDelimiterGet(ONU_GPON_DELM_BYTE_03);

  delimiter = (delimiterByte3 << 16) | (delimiterByte2 << 8) | delimiterByte1;
  rcode = mvOnuGponMacTxDelimiterSet(delimiter, GPON_TX_DELIMITER_DS);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacTxDelimiterSet, delimiter(0x%x)\n", __FILE_DESC__, __LINE__, delimiter);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicBerCounterInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu ber interval
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicBerCounterInit(void)
{
#if 0
  MV_U32    berInterval;
  MV_STATUS rcode;

  berInterval = onuGponDbBerIntervalGet();
  /* ONU BIP period Counter Register */
  rcode = mvOnuGponMacBerIntervalSet(berInterval);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponDbBerIntervalGet, berInterval(%d)\n", __FILE_DESC__, __LINE__, berInterval);
    return(rcode);
  }
#endif
  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicPloamParamInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu Rx PLOAM configuration
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicPloamParamInit(void)
{
  MV_STATUS rcode;

  rcode = mvOnuGponMacRxPloamConfigSet(MV_FALSE,MV_FALSE,MV_FALSE);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacRxPloamConfigSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicFrameDelineationInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu frame delineation
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicFrameDelineationInit(void)
{
  MV_STATUS rcode;

  /* Frame Delineation Register */ 
  rcode = mvOnuGponMacRxPsaConfigSet(GPON_FRAME_DELINEATION_M1, 
                                   GPON_FRAME_DELINEATION_M2,
                                   GPON_FRAME_DELINEATION_M2, 
                                   4); 
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacRxPsaConfigSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicBurstEnableInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu burst enable
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicBurstEnableInit(void)
{
  MV_STATUS rcode;

  /* BURST enable parameter */
  rcode = mvOnuGponMacTxBurstEnParamSet(GPON_BURST_EN_MASK, 
                                        GPON_BURST_EN_P, 
                                        1, 
                                        GPON_BURST_EN_STOP, 
                                        GPON_BURST_EN_START);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacTxBurstEnParamSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicFrameLengthInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu frame lengths
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicFrameLengthInit(void)
{
  MV_STATUS  rcode;

  rcode = mvOnuGponMacGemPayloadLenSet(GPON_MAX_GEM_PAYLOAD_LEN);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacGemPayloadLenSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  rcode = mvOnuGponMacGemEthFrameLenSet(GPON_MAX_ETH_FRAME_LEN, GPON_MIN_ETH_FRAME_LEN);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacGemEthFrameLenSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  rcode = mvOnuGponMacGemOmciFrameLenSet(GPON_MAX_OMCI_FRAME_LEN, GPON_MIN_OMCI_FRAME_LEN);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacGemOmciFrameLenSet\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicPortInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu gem and aes tables
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicPortInit(void)
{
  MV_STATUS rcode;

  rcode = mvOnuGponMacAesInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacAesInit\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  rcode = mvOnuGponMacGemInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) mvOnuGponMacGemInit\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicSerdesInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu gem and aes tables
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicSerdesInit(void)
{
  MV_STATUS status;

  status = onuPonSerdesInit();
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuPonSerdesInit\n", __FILE_DESC__, __LINE__);
    return(status);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAsicInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu GPON MAC configuration 
**               
**  PARAMETERS:  MV_BOOL initTime - init indication flag, true = init sequence
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponAsicInit(MV_BOOL initTime)
{
  MV_STATUS  rcode;

  onuGponAsicLedsInit();

  rcode = onuGponAsicOnuStateAndIdInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicOnuStateAndIdInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode = onuGponAsicDelayInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicDelayInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode = onuGponAsicOverheadInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicOverheadInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode = onuGponAsicBerCounterInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicBerCounterInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode = onuGponAsicFrameDelineationInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicFrameDelineationInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode = onuGponAsicPloamParamInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicPloamParamInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* Const RAM - serial number message + Idle message */
  rcode = onuGponSrvcConstPloamFromDbInit(initTime);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponSrvcConstPloamFromDbInit, initTime (%d)\n", __FILE_DESC__, __LINE__, initTime);
    return(rcode);
  }

  rcode =  onuGponAsicBurstEnableInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicBurstEnableInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode =  onuGponAsicFrameLengthInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicFrameLengthInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode =  onuGponAsicPortInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicPortInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  rcode =  onuGponAsicSerdesInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAsicSerdesInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuPonSwitchOn
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function execute onu switchOn init sequence
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonSwitchOn(void)
{
  MV_STATUS        rcode;
  S_BerCoefficient berCoeff;

  /* init onu RTOS resources */
  rcode = onuPonRtosResourceInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuPonRtosResourceInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* init onu application strcutures */
  rcode = onuGponAppInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuGponAppInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  berCoeff.numerator   = ONU_GPON_BER_DEF_COEFF_NUMERATOR;
  berCoeff.denominator = ONU_GPON_BER_DEF_COEFF_DENOMINATOR;
  onuGponBerInit(&berCoeff, ONU_GPON_DEF_INTERNAL_BER_INTERVAL);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponAppInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu gpon application: state & event table, 
**               and alarm table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK         
**                   
*******************************************************************************/
MV_STATUS onuGponAppInit(void)
{
  /* onu gpon state & event table */ 
  onuGponStateAndEventTblInit();

  /* onu gpon alarm table */ 
  onuGponAlarmTblInit();

  /* onu gpon counters table */ 
  onuGponPmInPmInit();

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponStateAndEventTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function initialize the State and Event Table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None         
**                   
*******************************************************************************/
void onuGponStateAndEventTblInit(void) 
{
  MV_U32 event;
  MV_U32 state;
  S_OnuGponGenTbl *onuGponGenTbl_p = &(onuGponDb_s.onuGponGenTbl_s);

  /* ========================================== */
  /* Initial setup for the Status & Event table */
  /* ========================================== */

  /* set onuGponIsrNotExpected function as default */
  for (event = 0; event < ONU_GPON_NUM_OF_EVENTS; event++)
  {
      for (state = 0; state < ONU_GPON_NUM_OF_STATES; state++)
      {
          onuGponGenTbl_p->onuGponStateAndEventTbl[event][state] = (GPONFUNCPTR)onuGponPonMngIsrNotExpected;
      }
  }

  /* ================================== */
  /* Message state transittions section */
  /* ================================== */

  /* ONU GPON STANDBY STATE 02 */
  /* ------------------------- */
  state = ONU_GPON_02_STANDBY;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_OVERHEAD][state]      = (GPONFUNCPTR)onuGponPonMngOverheadMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DIS_SN][state]        = (GPONFUNCPTR)onuGponPonMngDisSnMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_PHYSICAL_EQUIP_ERR][state] = (GPONFUNCPTR)onuGponPonMngPhyEquErrMsg;

  /* ONU GPON SERIAL NUM STATE 03 */
  /* ---------------------------- */
  state = ONU_GPON_03_SERIAL_NUM;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_SN_MASK][state] = (GPONFUNCPTR)onuGponPonMngSerialNumberMaskMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_EXT_BURST_LEN][state] = (GPONFUNCPTR)onuGponPonMngExtBurstMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_ASSIGN_ONU_ID][state] = (GPONFUNCPTR)onuGponPonMngOnuIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DIS_SN][state]        = (GPONFUNCPTR)onuGponPonMngDisSnMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_PHYSICAL_EQUIP_ERR][state] = (GPONFUNCPTR)onuGponPonMngPhyEquErrMsg;

  /* ONU GPON RANGING STATE 04 */
  /* ------------------------- */
  state = ONU_GPON_04_RANGING;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_EXT_BURST_LEN][state] = (GPONFUNCPTR)onuGponPonMngExtBurstMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_RANGING_TIME][state]  = (GPONFUNCPTR)onuGponPonMngRangeTimeMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DACT_ONU_ID][state]   = (GPONFUNCPTR)onuGponPonMngDactOnuIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DIS_SN][state]        = (GPONFUNCPTR)onuGponPonMngDisSnMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_PHYSICAL_EQUIP_ERR][state] = (GPONFUNCPTR)onuGponPonMngPhyEquErrMsg;

  /* ONU GPON OPERATION STATE 05 */
  /* --------------------------- */
  state = ONU_GPON_05_OPERATION;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_EXT_BURST_LEN][state] = (GPONFUNCPTR)onuGponPonMngExtBurstMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_RANGING_TIME][state]  = (GPONFUNCPTR)onuGponPonMngRangeTimeMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DACT_ONU_ID][state]   = (GPONFUNCPTR)onuGponPonMngDactOnuIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DIS_SN][state]        = (GPONFUNCPTR)onuGponPonMngDisSnMsg;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_CONFIG_PORT_ID][state] = (GPONFUNCPTR)onuGponPonMngCfgPortIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_PHYSICAL_EQUIP_ERR][state] = (GPONFUNCPTR)onuGponPonMngPhyEquErrMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_ENCRYPT_VPI_PORT_ID][state] = (GPONFUNCPTR)onuGponPonMngEncrptPortIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_ASSIGN_ALLOC_ID][state] = (GPONFUNCPTR)onuGponPonMngAssignAllocIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_REQ_PASSWORD][state]  = (GPONFUNCPTR)onuGponPonMngReqPassMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_REQ_KEY][state]       = (GPONFUNCPTR)onuGponPonMngReqKeyMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_BER_INTERVAL][state]  = (GPONFUNCPTR)onuGponPonMngBerIntervalMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_KEY_SWITCH_TIME][state] = (GPONFUNCPTR)onuGponPonMngKeySwitchTimeMsg;

  /* ONU GPON POPUP STATE 06 */
  /* ----------------------- */
  state = ONU_GPON_06_POPUP;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_POPUP][state]         = (GPONFUNCPTR)onuGponPonMngPopupMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DACT_ONU_ID][state]   = (GPONFUNCPTR)onuGponPonMngDactOnuIdMsg;
  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DIS_SN][state]        = (GPONFUNCPTR)onuGponPonMngDisSnMsg;

  /* ONU GPON EMERGANCY STOP STATE 07 */
  /* -------------------------------- */
  state = ONU_GPON_07_EMERGANCY_STOP;

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_MSG_DIS_SN][state]        = (GPONFUNCPTR)onuGponPonMngDisSnMsg;

  /* =================== */
  /* Alarm event section */
  /* =================== */
  for (state  = ONU_GPON_01_INIT; state <= ONU_GPON_07_EMERGANCY_STOP; state++)
  {
    /* generate */
    onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_GEN_LOS][state]     = (GPONFUNCPTR)onuGponPonMngGenCritAlarm;
    onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_GEN_LOF][state]     = (GPONFUNCPTR)onuGponPonMngGenCritAlarm;
    onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_GEN_LCDG][state]    = (GPONFUNCPTR)onuGponPonMngGenCritAlarm;

    /* cancel */                                                                    
    onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_CAN_LOS][state]     = (GPONFUNCPTR)onuGponPonMngCanCritAlarm;
    onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_CAN_LOF][state]     = (GPONFUNCPTR)onuGponPonMngCanCritAlarm;
    onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_CAN_LCDG][state]    = (GPONFUNCPTR)onuGponPonMngCanCritAlarm;
  }

  /* MEM alarm */                                                                                       
  /* --------- */                                                                   
  state = ONU_GPON_05_OPERATION;                                                    

  onuGponGenTbl_p->onuGponStateAndEventTbl[ONU_GPON_EVENT_ALM_GEN_MEM][state]       = (GPONFUNCPTR)onuGponPonMngGenMemAlarm;
}                                                                                                 

/*******************************************************************************
**
**  onuPonOperate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function execute onu operate init sequence
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**           
*******************************************************************************/
MV_STATUS onuPonOperate(void)
{
#if 0 /* standalone bring up */

  MV_STATUS rcode;
  MV_U32    interruptMask;
  MV_BOOL   disabledSnState;
  MV_U8     serialNumber[8];
  MV_U8     password[10];
  
  /* ======================================================= */
  /* ================ Disable SN Section =================== */
  /* ======================================================= */

  /* register disable SN handler */
  onuGponPonMngDisableSetRegister(onuGponDisableSnSetHandler);
  
  /* get disable State */
  onuGponDisableSnGetHandler(&disabledSnState);
  if (disabledSnState == MV_FALSE)
    /* Send Disable Notification to upper layer */
    onuGponSrvcDisableMsgNotify(MV_FALSE);
  else
  {
    mvPonPrint(PON_PRINT_INFO, PON_INIT_MODULE, "==================\n");
    mvPonPrint(PON_PRINT_INFO, PON_INIT_MODULE, "== ONT DISABLED ==\n");
    mvPonPrint(PON_PRINT_INFO, PON_INIT_MODULE, "==================\n");
    /* Send Disable Notification to upper layer */
    onuGponSrvcDisableMsgNotify(MV_TRUE);
  }

  /* ======================================================= */
  /* ================ Serial NUmber Section ================ */
  /* ======================================================= */
  onuGponSerialNumberHandler(serialNumber);
   
  /* ======================================================= */
  /* ================ Password Section ===================== */
  /* ======================================================= */
  onuGponPasswordHandler(password);
  
  /* ======================================================= */
  /* ================ Init App Section ===================== */
  /* ======================================================= */
  onuGponApiSnMaskConfig(MV_FALSE, MV_FALSE);
  onuGponApiInit(serialNumber, password, disabledSnState);

  /* XVR reset sequence */
  mvOnuGponMacXvrReset(0);

  /* Enable AES in SoC */
  mvOnuGponMacAesEnableSet(MV_TRUE);

  /* start onu gpon pon event clean up timer */ 
  onuPonTimerEnable(&onuPonResourceTbl_s.onuGponEvtCleanUpTimerId);

  /* start onu gpon pon pm timer */
  onuPonTimerEnable(&onuPonResourceTbl_s.onuPonPmTimerId);

  /* enable onu gpon interrupt mask */
  interruptMask = 0;
  interruptMask |= ONU_GPON_INTERRUPTS << 16;
  rcode = mvOnuGponMacPonInterruptrMaskSet(interruptMask);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Enable PON interrupt mask\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  /* ======================================================= */
  /* ================ Enable MAC RX ======================== */
  /* ======================================================= */
  mvOnuGponMacRxConfigSet(MV_TRUE);

#else /* dynamic bring up */

  onuGponApiSnMaskConfig(MV_FALSE, MV_FALSE);

  /* XVR reset sequence */
  mvOnuGponMacXvrReset(0);

  /* Enable AES in SoC */
  mvOnuGponMacAesEnableSet(MV_TRUE);

#endif

  printk(KERN_INFO "= PON Module Operate ended successfully =\n");

  return(MV_OK);
}

/*******************************************************************************
**
**  onuPonStart
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function start onu app
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**           
*******************************************************************************/
MV_STATUS onuPonStart(S_GponIoctlInfo *onuInit)
{
  MV_STATUS rcode;
  MV_U32    interruptMask;
  MV_BOOL   disabledSnState;

  /* ======================================================= */
  /* ================ Disable SN Section =================== */
  /* ======================================================= */
  disabledSnState = (onuInit->disableSn != 0) ? (MV_TRUE) : (MV_FALSE);
  if (disabledSnState == MV_FALSE)
    /* Send Disable Notification to upper layer */
    onuGponSrvcDisableMsgNotify(MV_FALSE);
  else
  {
    mvPonPrint(PON_PRINT_INFO, PON_INIT_MODULE, "==================\n");
    mvPonPrint(PON_PRINT_INFO, PON_INIT_MODULE, "== ONT DISABLED ==\n");
    mvPonPrint(PON_PRINT_INFO, PON_INIT_MODULE, "==================\n");
    /* Send Disable Notification to upper layer */
    onuGponSrvcDisableMsgNotify(MV_TRUE);
  }

  /* ======================================================= */
  /* ================ Init App Section ===================== */
  /* ======================================================= */
  onuGponApiInit(onuInit->serialNum, onuInit->password, disabledSnState);

  /* ======================================================= */
  /* ================ Start Timers Section ================= */
  /* ======================================================= */
  onuPonTimerEnable(&onuPonResourceTbl_s.onuGponEvtCleanUpTimerId);
  onuPonTimerEnable(&onuPonResourceTbl_s.onuPonPmTimerId);

  /* ======================================================= */
  /* ================ Enable Interrupt Section ============= */
  /* ======================================================= */
  interruptMask = 0;
  interruptMask |= ONU_GPON_INTERRUPTS << 16;
  rcode = mvOnuGponMacPonInterruptrMaskSet(interruptMask);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Enable PON interrupt mask\n", __FILE_DESC__, __LINE__);
    return(rcode);
  }

  /* ======================================================= */
  /* ================ Enable MAC RX ======================== */
  /* ======================================================= */
  mvOnuGponMacRxConfigSet(MV_TRUE);

  return(MV_OK);
}
