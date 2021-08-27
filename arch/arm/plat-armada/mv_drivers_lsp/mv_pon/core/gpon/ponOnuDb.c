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
**  FILE        : ponOnuDb.c                                                 **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON database functionality       **
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
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuDb.c"

/* Global Variables
------------------------------------------------------------------------------*/
/* ONU GPON Database */
S_OnuGponDb onuGponDb_s;

/* Local Variables
------------------------------------------------------------------------------*/
MV_U8 onuGponPassword[ONU_GPON_PASS_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05,
                                             0x0A, 0x0B, 0x0C, 0x0D, 0x0E};  

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
void      onuGponDbOnuGenTblInit(void);
void      onuGponDbOnuSyncParamTblInit(void);
void      onuGponDbOnuOperParamTblInit(void);
MV_STATUS onuGponDbGemPortAesClearAll(void);
MV_STATUS onuGponDbSerialNumberMaskEnableSet(MV_BOOL a_SerialNumberMaskEnable);
MV_BOOL   onuGponDbSerialNumberMaskEnableGet(void);

/******************************************************************************/
/* ========================================================================== */
/*                         Initialization Section                             */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponDbDbInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database to default values
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbInit (void)
{
  MV_STATUS rcode = MV_OK;

  onuGponDbOnuGenTblInit();
  onuGponDbOnuSyncParamTblInit();
  onuGponDbOnuOperParamTblInit();
  onuGponDbBwAllocInit();

  return(rcode);
}

/*******************************************************************************
**
**  onuGponDbOnuGenTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database general table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbOnuGenTblInit(void)
{
  /* clear onu database general table */
  memset((&(onuGponDb_s.onuGponGenTbl_s)), 0, sizeof (S_OnuGponGenTbl));

  /* set onu to state 01 */
  onuGponDbOnuStateSet(GPON_ONU_ID_STATE_01);

  /* set onu Id to undefined onu */
  onuGponDbOnuIdSet(GPON_ONU_ID_ONU_ID_DEF);

  /* set gpon rate to default value */
  onuGponDbRateSet(GPON_FRAME_DELINEATION_FR);

  /* set onu init status to be MV_FALSE */
  onuGponDbInitSet(MV_FALSE);

  /* set OMCC Valid to be MV_FALSE */
  onuGponDbOmccValidSet(MV_FALSE);

  /* set OnuId Override to be MV_FALSE */
  onuGponDbOnuIdOverrideSet(MV_FALSE);

  /* Set NULL in Notify functions */
  onuGponDbAlarmNotifySet(NULL);
  onuGponDbStatusNotifySet(NULL);
  onuGponDbOmccNotifySet(NULL);
  onuGponDbDisableNotifySet(NULL);
}

/*******************************************************************************
**
**  onuGponDbOnuSyncParamTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database sync params table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbOnuSyncParamTblInit(void)
{
  MV_U32 IdleMsgData[ONU_GPON_MSG_LEN];
  MV_U32 snMsgData[ONU_GPON_MSG_LEN];
  MV_U8  serialNumber[ONU_GPON_SN_LEN];

  /* clear onu database sync params table */
  memset ((&(onuGponDb_s.onuGponSyncParamsTbl_s)), 0, sizeof (S_OnuGponSyncParamsTbl));

  /* set onu guard bits */ 
  onuGponDbGuardBitsSet(0);

  /* set onu delays */ 
  onuGponDbConstDelaySet(GPON_TX_DELAY_TD_1244);
  onuGponDbEqualizationDelaySet(GPON_TX_EQUAL_DELAY_TD);

  /* set onu preamble */ 
  onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_01, ONU_TX_PREAMBLE_TYPE_01_P, ONU_TX_PREAMBLE_TYPE_01_PC);
  onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_02, ONU_TX_PREAMBLE_TYPE_02_P, ONU_TX_PREAMBLE_TYPE_02_PC);
  onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_03, ONU_TX_PREAMBLE_TYPE_03_P, ONU_TX_PREAMBLE_TYPE_03_DEF_PC);

  /* set onu delimiter */ 
  onuGponDbDelimiterSet(ONU_GPON_DELM_BYTE_01, GPON_TX_DELIMITER_D0); 
  onuGponDbDelimiterSet(ONU_GPON_DELM_BYTE_02, GPON_TX_DELIMITER_D1); 
  onuGponDbDelimiterSet(ONU_GPON_DELM_BYTE_03, GPON_TX_DELIMITER_D2); 
  onuGponDbDelimiterSizeSet(GPON_TX_DELIMITER_DS);
  onuGponDbDelimiterOverrideSet (MV_FALSE);

  /* set onu serial number */
  serialNumber[0] = ONU_GPON_SN_DEF_BYTE_1;
  serialNumber[1] = ONU_GPON_SN_DEF_BYTE_2;
  serialNumber[2] = ONU_GPON_SN_DEF_BYTE_3;
  serialNumber[3] = ONU_GPON_SN_DEF_BYTE_4;
  serialNumber[4] = ONU_GPON_SN_DEF_BYTE_5;
  serialNumber[5] = ONU_GPON_SN_DEF_BYTE_6;
  serialNumber[6] = ONU_GPON_SN_DEF_BYTE_7;
  serialNumber[7] = ONU_GPON_SN_DEF_BYTE_8;

  /* set onu serial number mask parametrs */
  onuGponDbSerialNumberMaskEnableSet(ONU_GPON_SN_MSK_GEN_ENA);
  onuGponDbSerialNumberMaskMatchSet(ONU_GPON_SN_MSK_DEF_MATCH);
  onuGponDbSnMaskSet(MV_FALSE);

  onuGponDbSerialNumSet (&(serialNumber[0]));

  IdleMsgData[0] = ((MV_U32)0) |
                   (((MV_U32)0) << 8) |
                   (((MV_U32)ONU_GPON_IDLE_MSG_DEF_BYTE_02) << 16) | 
                   (((MV_U32)ONU_GPON_IDLE_MSG_DEF_BYTE_01) << 24);
  IdleMsgData[1] = ((MV_U32)0) |
                   (((MV_U32)0) << 8) |
                   (((MV_U32)0) << 16) | 
                   (((MV_U32)0) << 24);
  IdleMsgData[2] = ((MV_U32)0) |
                   (((MV_U32)0) << 8) |
                   (((MV_U32)0) << 16) | 
                   (((MV_U32)0) << 24);

  onuGponDbIdleMsgSet(IdleMsgData);

  snMsgData[0] = ((MV_U32)serialNumber[1]) | 
                 (((MV_U32)serialNumber[0]) << 8) |
                 (((MV_U32)ONU_GPON_SN_MSG_DEF_BYTE_02) << 16) |
                 (((MV_U32)ONU_GPON_SN_MSG_DEF_BYTE_01) << 24);
  snMsgData[1] = ((MV_U32)serialNumber[5]) | 
                 (((MV_U32)serialNumber[4]) << 8) |
                 (((MV_U32)serialNumber[3]) << 16) |
                 (((MV_U32)serialNumber[2]) << 24);
  snMsgData[2] = ((MV_U32)ONU_GPON_SN_MSG_DEF_BYTE_12) | 
                 (((MV_U32)ONU_GPON_SN_MSG_DEF_BYTE_11) << 8) |
                 (((MV_U32)serialNumber[7]) << 16) |
                 (((MV_U32)serialNumber[6]) << 24);

  onuGponDbSnMsgSet(snMsgData);
}

/*******************************************************************************
**
**  onuGponDbOnuOperParamTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database oper params table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbOnuOperParamTblInit(void)
{
  /* clear onu database oper params table */
  memset((&(onuGponDb_s.onuGponOperParamsTbl_s)), 0, sizeof (S_OnuGponOperParamsTbl));

  /* set ber, signal fail threshold and signal degraded threshold */
  onuGponDbBerIntervalSet(GPON_BIP_PERIOD_CNTR);
  onuGponDbBerCalcIntervalSet(ONU_GPON_DEF_INTERNAL_BER_INTERVAL);
  onuGponDbSfThresholdSet(ONU_GPON_DEF_SF_THRESHOLD);
  onuGponDbSdThresholdSet(ONU_GPON_DEF_SD_THRESHOLD);

  /* Init GEM Port AES table */
  onuGponDbGemPortAesClearAll();

  /* Init GEM Port table */
  onuGponDbGemPortClearAll();

  /* set onu REI sequence number */ 
  onuGponDbReiSeqNumSet(ONU_GPON_DEF_REI_SEQ_NUM);

  /* set onu password */ 
  onuGponDbPasswordSet(&(onuGponPassword[0]));
}

/*******************************************************************************
**
**  onuGponDbBwAllocInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database bandwidth allocation table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwAllocInit(void)
{
  MV_U32 iEntry;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwTcontSet(iEntry, MV_FALSE, PON_ONU_ALLOC_NOT_EXIST, MV_FALSE);
    onuGponDbBwAllocSet(iEntry, PON_ONU_ALLOC_NOT_EXIST, MV_FALSE);
    onuGponDbBwIdleAllocSet(iEntry, PON_ONU_ALLOC_NOT_EXIST);
  }

  return(MV_OK);
}

/******************************************************************************/
/* ========================================================================== */
/*                         Interface Section                                  */
/* ========================================================================== */
/******************************************************************************/

/********************************************/
/* ======================================== */
/*   ONU GPON General Table API Functions   */
/* ======================================== */
/********************************************/

/*******************************************************************************
**
**  onuGponDbOnuStateSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu state in the database
**               
**  PARAMETERS:  E_OnuState onuState_e - ONU_GPON_01_INIT          
**                                       ONU_GPON_02_STANDBY       
**                                       ONU_GPON_03_SERIAL_NUM    
**                                       ONU_GPON_04_RANGING       
**                                       ONU_GPON_05_OPERATION     
**  									 ONU_GPON_06_POPUP         
**  									 ONU_GPON_07_EMERGANCY_STOP
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbOnuStateSet(MV_U32 onuState)
{
  if ((onuState < ONU_GPON_01_INIT) ||
      (onuState > ONU_GPON_07_EMERGANCY_STOP))
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponGenTbl_s.onuGponOnuState = onuState;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOnuStateGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu state 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu state
**                   
*******************************************************************************/
MV_U32 onuGponDbOnuStateGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponOnuState);
}

/*******************************************************************************
**
**  onuGponDbOnuIdSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu Id in the database
**               
**  PARAMETERS:  MV_U32 onuId     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbOnuIdSet(MV_U32 onuId)
{
  MV_U32 idleMsg[3];
  MV_U32 snNumMsg[3];

  onuGponDb_s.onuGponGenTbl_s.onuGponOnuId = onuId;

  onuGponDbIdleMsgGet(idleMsg);

  idleMsg[0] &= (0x00FFFFFF);
  idleMsg[0] |=  (onuId << 24);

  onuGponDbIdleMsgSet(idleMsg);

  onuGponDbSnMsgGet(snNumMsg);

  snNumMsg[0] &= (0x00FFFFFF);
  snNumMsg[0] |=  (onuId << 24);

  onuGponDbSnMsgSet(snNumMsg);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOnuIdGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Id 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu Id  
**                   
*******************************************************************************/
MV_U32 onuGponDbOnuIdGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponOnuId);
}

/*******************************************************************************
**
**  onuGponDbOnuIdOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Id override value 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu Id  
**                   
*******************************************************************************/
MV_U32 onuGponDbOnuIdOverrideValueGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponOnuIdOverrideVal);
}

/*******************************************************************************
**
**  onuGponDbOnuIdOverrideValueSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu Id override value 
**               
**  PARAMETERS:  MV_U32 onuId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK  
**                   
*******************************************************************************/
MV_STATUS onuGponDbOnuIdOverrideValueSet(MV_U32 onuId)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponOnuIdOverrideVal = onuId;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOnuIdOverrideSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu Id override state
**               
**  PARAMETERS:  MV_BOOL enable
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK  
**                   
*******************************************************************************/
MV_STATUS onuGponDbOnuIdOverrideSet(MV_BOOL enable)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponOnuIdOverrideEn = enable;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOnuIdOverrideGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Id override state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu Id  
**                   
*******************************************************************************/
MV_BOOL onuGponDbOnuIdOverrideGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponOnuIdOverrideEn);
}

/*******************************************************************************
**
**  onuGponDbdGaspEnSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set dying gasp in the database 
**               
**  PARAMETERS:  MV_BOOL dGaspEn
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK  
**                   
*******************************************************************************/
MV_STATUS onuGponDbdGaspEnSet(MV_BOOL dGaspEn)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponDyingGaspEn = dGaspEn;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbdGaspEnGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu dying gasp state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu Id  
**                   
*******************************************************************************/
MV_BOOL onuGponDbdGaspEnGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponDyingGaspEn);
}

/*******************************************************************************
**
**  onuGponDbInitSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set ONU Init status in the database
**               
**  PARAMETERS:  MV_BOOL init     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbInitSet(MV_BOOL init)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponInit = init;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbInitGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu init state 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu init state - MV_TRUE/MV_FALSE  
**                   
*******************************************************************************/
MV_BOOL onuGponDbInitGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponInit);
}

/*******************************************************************************
**
**  onuGponDbOmccValidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu OMCC state in the database 
**               
**  PARAMETERS:  MV_BOOL omccValid     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbOmccValidSet(MV_BOOL omccValid)
{
  onuGponDb_s.onuGponGenTbl_s.omccValid = omccValid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOmccValidGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu OMCC state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     OMCC - MV_TRUE/MV_FALSE  
**                   
*******************************************************************************/
MV_BOOL onuGponDbOmccValidGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.omccValid);
}

/*******************************************************************************
**
**  onuGponDbOmccPortdSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu OMCC value in the database 
**               
**  PARAMETERS:  MV_U32 omccPort     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbOmccPortSet(MV_U32 omccPort)
{
  onuGponDb_s.onuGponGenTbl_s.omccPort = omccPort;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOmccPortGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu OMCC port 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     OMCC port 
**                   
*******************************************************************************/
MV_U32 onuGponDbOmccPortGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.omccPort);
}

/*******************************************************************************
**
**  onuGponDbOnuIdOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu OMCC override value 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     OMCC override value 
**                   
*******************************************************************************/
MV_U32 onuGponDbOmccPortOverrideValueGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponOmccPortOverrideVal);
}

/*******************************************************************************
**
**  onuGponDbOnuIdOverrideValueSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu OMCC override value in the database 
**               
**  PARAMETERS:  MV_U32 OmccPort
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK  
**                   
*******************************************************************************/
MV_STATUS onuGponDbOmccPortOverrideValueSet(MV_U32 OmccPort)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponOmccPortOverrideVal = OmccPort;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOmccPortOverrideSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu OMCC override state in the database 
**               
**  PARAMETERS:  MV_BOOL enable
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK 
**                   
*******************************************************************************/
MV_STATUS onuGponDbOmccPortOverrideSet(MV_BOOL enable)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponOmccPortOverrideEn = enable;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOmccPortOverrideGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu OMCC override state 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     OMCC override state 
**                   
*******************************************************************************/
MV_BOOL onuGponDbOmccPortOverrideGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponOmccPortOverrideEn);
}

/*******************************************************************************
**
**  onuGponDbRateSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set ONU Rate in the database
**               
**  PARAMETERS:  MV_U32 rate     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbRateSet(MV_U32 rate)
{
  onuGponDb_s.onuGponGenTbl_s.onuGponRate = rate;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbRateGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Rate 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu Rate 
**                   
*******************************************************************************/
MV_U32 onuGponDbRateGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.onuGponRate);
}

/*******************************************************************************
**
**  onuGponDbAlarmNotifySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set Alarm Notify Callback in the database
**               
**  PARAMETERS:  ALARMNOTIFYFUNC alarmCallback - alarm callback     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbAlarmNotifySet(ALARMNOTIFYFUNC alarmCallback)
{
  onuGponDb_s.onuGponGenTbl_s.alarmCallback = alarmCallback;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbAlarmNotifyGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Alarm Notify Callback
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     Alarm Notify Callback
**                   
*******************************************************************************/
ALARMNOTIFYFUNC onuGponDbAlarmNotifyGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.alarmCallback);
}

/*******************************************************************************
**
**  onuGponDbStatusNotifySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set Status Notify Callback in the database
**               
**  PARAMETERS:  STATUSNOTIFYFUNC statusCallback      
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbStatusNotifySet(STATUSNOTIFYFUNC statusCallback)
{
  onuGponDb_s.onuGponGenTbl_s.statusCallback = statusCallback;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbStatusNotifyGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Status Notify Callback
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     Status Notify Callback
**                   
*******************************************************************************/
STATUSNOTIFYFUNC onuGponDbStatusNotifyGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.statusCallback);
}

/*******************************************************************************
**
**  onuGponDbOmccNotifySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu Omcc Notify Callback in the database
**               
**  PARAMETERS:  OMCCNOTIFYFUNC omccCallback     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbOmccNotifySet(OMCCNOTIFYFUNC omccCallback)
{
  onuGponDb_s.onuGponGenTbl_s.omccCallback = omccCallback;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbOmccNotifyGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Omcc Notify Callback 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     Omcc Notify Callback  
**                   
*******************************************************************************/
OMCCNOTIFYFUNC onuGponDbOmccNotifyGet(void)
{
  return(onuGponDb_s.onuGponGenTbl_s.omccCallback);
}

/*******************************************************************************
**
**  onuGponDbDisableNotifySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu disable Notify Callback in the database
**               
**  PARAMETERS:  DISABLENOTIFYFUNC disableCallback     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbDisableNotifySet(DISABLENOTIFYFUNC disableCallback)
{
  onuGponDb_s.onuGponGenTbl_s.disableCallback = disableCallback;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbDisableNotifyGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu disable Notify Callback
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     Disable Notify Callback  
**                   
*******************************************************************************/
DISABLENOTIFYFUNC onuGponDbDisableNotifyGet (void)
{
  return(onuGponDb_s.onuGponGenTbl_s.disableCallback);
}

/***********************************************/
/* =========================================== */
/*   ONU GPON Sync Params Table API Functions  */
/* =========================================== */
/***********************************************/

/*******************************************************************************
**
**  onuGponDbSnMaskSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu SN mask in the database 
**               
**  PARAMETERS:  MV_U32 snMaskEn
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbSnMaskSet(MV_U32 snMaskEn)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNumberMaskDefEnable = snMaskEn;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbGuardBitsSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead guard bit
**               
**  PARAMETERS:  MV_U32 guardBit     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbGuardBitsSet(MV_U32 guardBit)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponGuardBits = guardBit;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbGuardBitsGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead guard bit
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     Overhead guard bit 
**                   
*******************************************************************************/
MV_U32 onuGponDbGuardBitsGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponGuardBits);
}

/*******************************************************************************
**
**  onuGponDbPreambleSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead preamble type and count
**               
**  PARAMETERS:  E_OnuOverheadPreambleType premType_e
**               MV_U32                    premVal  
**               MV_U32                    premCnt
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbPreambleSet(E_OnuOverheadPreambleType premType_e, 
                               MV_U32 premVal,
                               MV_U32 premCnt)
{
  if (premType_e > ONU_GPON_PREM_TYPE_03)
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponPreambleVal[premType_e] = premVal;
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponPreambleCnt[premType_e] = premCnt;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbPreambleGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead preamble type and count
**               
**  PARAMETERS:  E_OnuOverheadPreambleType premType_e
**               MV_U32                    *premVal  
**               MV_U32                    *premCnt
** 
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbPreambleGet(E_OnuOverheadPreambleType premType_e,
                          MV_U32 *premVal, 
                          MV_U32 *premCnt)
{
  *premVal = onuGponDb_s.onuGponSyncParamsTbl_s.onuGponPreambleVal[premType_e];
  *premCnt = onuGponDb_s.onuGponSyncParamsTbl_s.onuGponPreambleCnt[premType_e];
}

/*******************************************************************************
**
**  onuGponDbExtPreambleStatusSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead extended preamble type3 status
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbExtPreambleStatusSet(MV_U32 extendPremStatus)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleStatus = extendPremStatus;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbExtPreambleStatusGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead extended preamble type3 status
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     extended preamble type3 status
**                   
*******************************************************************************/
MV_U32 onuGponDbExtPreambleStatusGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleStatus);
}

/*******************************************************************************
**
**  onuGponDbExtPreambleSyncSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead extended preamble type3 - state 3/4
**               
**  PARAMETERS:  MV_U32 extendPremCnt
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbExtPreambleSyncSet(MV_U32 extendPremCnt)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleSync = extendPremCnt;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbExtPreambleSyncGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead extended preamble type3
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     overhead extended preamble type3 count - state 3/4
**                   
*******************************************************************************/
MV_U32 onuGponDbExtPreambleSyncGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleSync);
}

/*******************************************************************************
**
**  onuGponDbExtPreambleOperSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead extended preamble type3 - state 5
**               
**  PARAMETERS:  MV_U32 extendPremCnt
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbExtPreambleOperSet(MV_U32 extendPremCnt)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleOper = extendPremCnt;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbExtPreambleOperGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead extended preamble type3
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     overhead extended preamble type3 count - state 5
**                   
*******************************************************************************/
MV_U32 onuGponDbExtPreambleOperGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleOper);
}

/*******************************************************************************
**
**  onuGponDbDelimiterSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead delimiter byte
**               
**  PARAMETERS:  E_OnuOverheadDelimiterByte delimByte_e
**               MV_U32                     delimVal  
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbDelimiterSet(E_OnuOverheadDelimiterByte delimByte_e, 
                                MV_U32 delimVal)
{
  if (delimByte_e > ONU_GPON_DELM_BYTE_03)
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiter[delimByte_e] = delimVal;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbDelimiterGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead delimiter byte
**               
**  PARAMETERS:  E_OnuOverheadDelimiterByte delimByte_e
**
**  OUTPUTS:     None
**
**  RETURNS:     overhead delimiter byte
**                   
*******************************************************************************/
MV_U32 onuGponDbDelimiterGet(E_OnuOverheadDelimiterByte delimByte_e)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiter[delimByte_e]);
}

/*******************************************************************************
**
**  onuGponDbDelimiterSizeSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead delimiter size
**               
**  PARAMETERS:  MV_U32 delimSize  
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbDelimiterSizeSet(MV_U32 delimSize)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiterSize = delimSize;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbDelimiterSizeGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead delimiter size
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     overhead delimiter size
**                   
*******************************************************************************/
MV_U32 onuGponDbDelimiterSizeGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiterSize);
}

/*******************************************************************************
**
**  onuGponDbDelimiterOverrideValueSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead delimiter override value
**               
**  PARAMETERS:  MV_U32 delimVal
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbDelimiterOverrideValueSet(MV_U32 delimVal)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiterOverrideValue = delimVal;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDelimiterOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead delimiter override value
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu overhead delimiter override value
**                   
*******************************************************************************/
MV_U32 onuGponDbDelimiterOverrideValueGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiterOverrideValue); 
}

/*******************************************************************************
**
**  onuGponDbDelimiterOverrideSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu overhead delimiter override state
**               
**  PARAMETERS:  MV_BOOL enable
**
**  OUTPUTS:     None
**
**  RETURNS:     onu overhead delimiter state
**                   
*******************************************************************************/
MV_STATUS onuGponDbDelimiterOverrideSet(MV_BOOL enable)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiterOverride = enable;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbDelimiterOverridGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu overhead delimiter override state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     overhead delimiter override state
**                   
*******************************************************************************/
MV_U32 onuGponDbDelimiterOverrideGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponDelimiterOverride);
}

/*******************************************************************************
**
**  onuGponDbExtendedBurstOverrideSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu extended burst override state
**               
**  PARAMETERS:  MV_BOOL enable
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbExtendedBurstOverrideSet(MV_BOOL enable)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleOverride = enable;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbExtendedBurstOverrideGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu extended burst override state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu extended burst override state
**                   
*******************************************************************************/
MV_BOOL onuGponDbExtendedBurstOverrideGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleOverride);
}

/*******************************************************************************
**
**  onuGponDbExtendedBurstOverrideValueSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu extended burst override values
**               
**  PARAMETERS:  MV_U32 exBurstRange
**               MV_U32 exBurstOper
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbExtendedBurstOverrideValueSet(MV_U32 exBurstRange, MV_U32 exBurstOper)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleSyncOverride = exBurstRange;
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleOperOverride = exBurstOper;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbExtendedBurstSyncOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu extended burst override value (O3/O4)
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu extended burst override value (O3/O4)
**                   
*******************************************************************************/
MV_U32 onuGponDbExtendedBurstSyncOverrideValueGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleSyncOverride); 
}

/*******************************************************************************
**
**  onuGponDbExtendedBurstOperOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu extended burst override value (O5)
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu extended burst override value (O5)
**                   
*******************************************************************************/
MV_U32 onuGponDbExtendedBurstOperOverrideValueGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponExtPreambleOperOverride); 
}

/*******************************************************************************
**
**  onuGponDbMaxExtraSnTransSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set overhead max extra sn transmissions
**               
**  PARAMETERS:  MV_U32 maxExtraSnTrans
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbMaxExtraSnTransSet(MV_U32 maxExtraSnTrans)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponMaxExtraSnTrans = maxExtraSnTrans;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbMaxExtraSnTransGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return overhead max extra sn transmissions
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     overhead max extra sn transmissions
**                   
*******************************************************************************/
MV_U32 onuGponDbMaxExtraSnTransGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponMaxExtraSnTrans);
}

/*******************************************************************************
**
**  onuGponDbSerialNumSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu serial number
**               
**  PARAMETERS:  MV_U8 *serialNum
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbSerialNumSet(MV_U8 *serialNum)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_SN_LEN; index++)
  {
    onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNum[index] = *(serialNum + index);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbSerialNumGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu serial number
**               
**  PARAMETERS:  MV_U8 *serialNum
**
**  OUTPUTS:     MV_U8 *serialNum
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbSerialNumGet(MV_U8 *serialNum)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_SN_LEN; index++)
  {
    *(serialNum + index) = onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNum[index];
  }
}

/*******************************************************************************
**
**  onuGponDbSerialNumberMaskEnableGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu SN mask enable state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     SN mask enable state
**                   
*******************************************************************************/
MV_BOOL onuGponDbSerialNumberMaskEnableGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNumberMaskEnable);
}

/*******************************************************************************
**
**  onuGponDbSerialNumberMaskEnableSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu SN mask enable state
**               
**  PARAMETERS:  MV_BOOL a_SerialNumberMaskEnable
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbSerialNumberMaskEnableSet(MV_BOOL a_SerialNumberMaskEnable)
{
  MV_BOOL profileSnMaskValue;

  profileSnMaskValue = onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNumberMaskDefEnable; 

  if (profileSnMaskValue == MV_TRUE)
  {
    onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNumberMaskEnable = a_SerialNumberMaskEnable;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbSerialNumberMaskMatchGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu SN mask match state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     SN mask match state
**                   
*******************************************************************************/
MV_BOOL onuGponDbSerialNumberMaskMatchGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNumberMaskMatch);
}

/*******************************************************************************
**
**  onuGponDbSerialNumberMaskMatchSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu SN mask enable state
**               
**  PARAMETERS:  MV_BOOL a_SerialNumberMaskMatch
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbSerialNumberMaskMatchSet(MV_BOOL a_SerialNumberMaskMatch)
{    
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSerialNumberMaskMatch = a_SerialNumberMaskMatch;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbConstDelaySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu const delay
**               
**  PARAMETERS:  MV_U32 constDelay
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbConstDelaySet(MV_U32 constDelay)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponConstDelay = constDelay;
}

/*******************************************************************************
**
**  onuGponDbConstDelayGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu const delay
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu const delay
**                   
*******************************************************************************/
MV_U32 onuGponDbConstDelayGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponConstDelay);
}

/*******************************************************************************
**
**  onuGponDbEqualizationDelaySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu equalization delay
**               
**  PARAMETERS:  MV_U32 equalizationDelay
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbEqualizationDelaySet(MV_U32 equalizationDelay)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponEqualizationDelay = equalizationDelay;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbEqualizationDelayGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu equalization delay
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu equalization delay
**                   
*******************************************************************************/
MV_U32 onuGponDbEqualizationDelayGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponEqualizationDelay);
}

/*******************************************************************************
**
**  onuGponDbEqualizationDelayOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu equalization delay override value
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu equalization delay override value
**                   
*******************************************************************************/
MV_U32 onuGponDbEqualizationDelayOverrideValueGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponEqualizationDelayOverrideValue);
}

/*******************************************************************************
**
**  onuGponDbEqualizationDelayOverrideValueSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu equalization delay override value
**               
**  PARAMETERS:  MV_U32 equalizationDelayOverrideVal
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbEqualizationDelayOverrideValueSet(MV_U32 equalizationDelayOverrideVal)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponEqualizationDelayOverrideValue = equalizationDelayOverrideVal;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbEqualizationDelayOverrideValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu equalization delay override state
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu equalization delay override state
**                   
*******************************************************************************/
MV_BOOL onuGponDbEqualizationDelayOverrideGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponEqualizationDelayOverride);
}

/*******************************************************************************
**
**  onuGponDbEqualizationDelayOverrideSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu equalization delay override state
**               
**  PARAMETERS:  MV_BOOL OverrideEqualizationDelay
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbEqualizationDelayOverrideSet(MV_BOOL OverrideEqualizationDelay)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponEqualizationDelayOverride = OverrideEqualizationDelay;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbPowerLevelSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu power level
**               
**  PARAMETERS:  MV_U32 powerLevel
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbPowerLevelSet(MV_U32 powerLevel)
{
  onuGponDb_s.onuGponSyncParamsTbl_s.onuGponPowerLevel = powerLevel;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbPowerLevelGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu power level
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu power level
**                   
*******************************************************************************/
MV_U32 onuGponDbPowerLevelGet(void)
{
  return(onuGponDb_s.onuGponSyncParamsTbl_s.onuGponPowerLevel);
}

/*******************************************************************************
**
**  onuGponDbIdleMsgSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu idle message
**               
**  PARAMETERS:  MV_U32 *idleMsg
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbIdleMsgSet(MV_U32 *idleMsg)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_MSG_LEN; index++)
  {
    onuGponDb_s.onuGponSyncParamsTbl_s.onuGponIdleMsg[index] = *(idleMsg + index);
  }
}

/*******************************************************************************
**
**  onuGponDbIdleMsgGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu idle message
**               
**  PARAMETERS:  MV_U32 *idleMsg
**
**  OUTPUTS:     MV_U32 *idleMsg
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbIdleMsgGet(MV_U32 *idleMsg)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_MSG_LEN; index++)
  {
    *(idleMsg + index) = onuGponDb_s.onuGponSyncParamsTbl_s.onuGponIdleMsg[index];
  }
}

/*******************************************************************************
**
**  onuGponDbSnMsgSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu serial number message
**               
**  PARAMETERS:  MV_U32 *snMsg
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbSnMsgSet(MV_U32 *snMsg)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_MSG_LEN; index++)
  {
    onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSnMsg[index] = *(snMsg + index);
  }
}

/*******************************************************************************
**
**  onuGponDbSnMsgGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu serial number message
**               
**  PARAMETERS:  MV_U32 *snMsg
**
**  OUTPUTS:     MV_U32 *snMsg
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbSnMsgGet(MV_U32 *snMsg)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_MSG_LEN; index++)
  {
    *(snMsg + index) = onuGponDb_s.onuGponSyncParamsTbl_s.onuGponSnMsg[index];
  }
}

/***********************************************/
/* =========================================== */
/*   ONU GPON Oper Params Table API Functions  */
/* =========================================== */
/***********************************************/

/*******************************************************************************
**
**  onuGponDbBerIntervalSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu ber interval
**               
**  PARAMETERS:  MV_U32 berInterval
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbBerIntervalSet(MV_U32 berInterval)
{
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponBerInterval = berInterval;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBerIntervalGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu ber interval
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu ber interval
**                   
*******************************************************************************/
MV_U32 onuGponDbBerIntervalGet(void)
{
  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponBerInterval);
}

/*******************************************************************************
**
**  onuGponDbBipInterruptStatusValueSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function update onu ber interval value
**               
**  PARAMETERS:  MV_U32 Bip8
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbBipInterruptStatusValueSet(MV_U32 Bip8)
{
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponBerIntervalValue = Bip8;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBerIntervalGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu ber interval value
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu ber interval value
**                   
*******************************************************************************/
MV_U32 onuGponDbBipInterruptStatusValueGet(void)
{
  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponBerIntervalValue);
}

/*******************************************************************************
**
**  onuGponDbBerCalcIntervalSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu internal ber interval
**               
**  PARAMETERS:  MV_U32 internalBerInterval
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbBerCalcIntervalSet(MV_U32 internalBerInterval)
{
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponBerCalcInterval = internalBerInterval;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBerCalcIntervalGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu internal ber interval
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu internal ber interval
**                   
*******************************************************************************/
MV_U32 onuGponDbBerCalcIntervalGet(void)
{
  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponBerCalcInterval);
}

/*******************************************************************************
**
**  onuGponDbSfThresholdSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu SF threshold
**               
**  PARAMETERS:  MV_U32 SF_Threshold
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbSfThresholdSet(MV_U32 SF_Threshold)
{
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponSfThreshold = SF_Threshold;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbSfThresholdGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu SF threshold
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu SF threshold
**                   
*******************************************************************************/
MV_U32 onuGponDbSfThresholdGet(void)
{
  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponSfThreshold);
}

/*******************************************************************************
**
**  onuGponDbSdThresholdSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu SD threshold
**               
**  PARAMETERS:  MV_U32 SD_Threshold
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbSdThresholdSet(MV_U32 SD_Threshold)
{
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponSdThreshold = SD_Threshold;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbSdThresholdGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu SD threshold
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu SD threshold
**                   
*******************************************************************************/
MV_U32 onuGponDbSdThresholdGet(void)
{
  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponSdThreshold);
}

/*******************************************************************************
**
**  onuGponDbGemPortValidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set gem port valid state 
**               
**  PARAMETERS:  MV_U32 gemPortId
**               MV_BOOL   valid 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbGemPortValidSet(MV_U32 gemPortId, MV_BOOL valid)
{
  if (gemPortId >= GPON_ONU_MAX_GEM_PORTS)
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponOperParamsTbl_s.onuGponGemPortValid[gemPortId] = valid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbGemPortValidGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return gem port valid state 
**               
**  PARAMETERS:  MV_U32 gemPortId
**
**  OUTPUTS:     None
**
**  RETURNS:     gem port valid state
**                   
*******************************************************************************/
MV_BOOL onuGponDbGemPortValidGet(MV_U32 gemPortId)
{
  if (gemPortId >= GPON_ONU_MAX_GEM_PORTS)
  {
    return(MV_FALSE);
  }

  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponGemPortValid[gemPortId]);
}

/*******************************************************************************
**
**  onuGponDbGemPortClearAll
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clear all gem ports
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbGemPortClearAll(void)
{
  MV_U32  iPort;

  for (iPort = 0 ; iPort < GPON_ONU_MAX_GEM_PORTS ; iPort++)
  {
    onuGponDb_s.onuGponOperParamsTbl_s.onuGponGemPortValid[iPort] = MV_FALSE;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbGemPortAesSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set gem port encryption state 
**               
**  PARAMETERS:  MV_U32 gemPortId
**               MV_BOOL mode 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbGemPortAesSet(MV_U32 gemPortId, MV_BOOL mode)
{
  if (gemPortId >= GPON_ONU_MAX_GEM_PORTS)
  {
    return(MV_ERROR);
  }
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponGemPortAes[gemPortId] = mode;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbGemPortAesGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return gem port encryption state 
**               
**  PARAMETERS:  MV_U32 gemPortId
**
**  OUTPUTS:     None
**
**  RETURNS:     gem port encryption state
**                   
*******************************************************************************/
MV_BOOL onuGponDbGemPortAesGet(MV_U32 gemPortId)
{
  if (gemPortId >= GPON_ONU_MAX_GEM_PORTS)
  {
    return(MV_FALSE);
  }

  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponGemPortAes[gemPortId]);
}

/*******************************************************************************
**
**  onuGponDbGemPortAesClearAll
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clear all AES gem ports
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbGemPortAesClearAll(void)
{
  MV_U32  iPort;

  for (iPort = 0 ; iPort < GPON_ONU_MAX_GEM_PORTS ; iPort++)
  {
    onuGponDb_s.onuGponOperParamsTbl_s.onuGponGemPortAes[iPort] = MV_FALSE;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbReiSeqNumSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu REI sequence number
**               
**  PARAMETERS:  MV_U32 reiSeqNum
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbReiSeqNumSet(MV_U32 reiSeqNum)
{
  onuGponDb_s.onuGponOperParamsTbl_s.onuGponReiSeqNum = reiSeqNum;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbReiSeqNumGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu REI sequence number
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu REI sequence number
**                   
*******************************************************************************/
MV_U32 onuGponDbReiSeqNumGet(void)
{
  return(onuGponDb_s.onuGponOperParamsTbl_s.onuGponReiSeqNum);
}

/*******************************************************************************
**
**  onuGponDbPasswordSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu password
**               
**  PARAMETERS:  MV_U8 *password
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbPasswordSet(MV_U8 *password)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_PASS_LEN; index++)
  {
    onuGponDb_s.onuGponOperParamsTbl_s.onuGponPassword[index] = *(password + index);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbPasswordGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu password
**               
**  PARAMETERS:  MV_U8 *password
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponDbPasswordGet(MV_U8 *password)
{
  MV_U32 index;

  for (index = 0; index < ONU_GPON_PASS_LEN; index++)
  {
    *(password + index) = onuGponDb_s.onuGponOperParamsTbl_s.onuGponPassword[index]; 
  }
}

/********************************************/
/* ======================================== */
/*   ONU GPON BW Alloc Table API Functions  */
/* ======================================== */
/********************************************/

/* ======================================== */
/*   ONU BW ALLOC SECTION                   */
/* ======================================== */

/*******************************************************************************
**
**  onuGponDbBwAllocSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function sets Alloc-Id value & state to T-Cont
**               
**  PARAMETERS:  MV_U32 entry
**               MV_U32 allocId
**               MV_BOOL valid
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwAllocSet(MV_U32 entry, MV_U32 allocId, MV_BOOL valid)
{
  if (entry >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponBwTbl_s.onuAllocIds[entry].allocId = allocId;
  onuGponDb_s.onuGponBwTbl_s.onuAllocIds[entry].valid   = valid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwAllocGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return Alloc-Id value & state to T-Cont
**               
**  PARAMETERS:  MV_U32 entry
**               MV_U32 allocId
**               MV_BOOL valid
**
**  OUTPUTS:     None
**
**  RETURNS:     Alloc-Id value & state
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwAllocGet(MV_U32 entry, MV_U32 *allocId, MV_BOOL *valid)
{
  if (entry >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    *allocId = 0;
    *valid   = MV_FALSE;
    return(MV_ERROR);
  }

  *allocId = onuGponDb_s.onuGponBwTbl_s.onuAllocIds[entry].allocId;
  *valid   = onuGponDb_s.onuGponBwTbl_s.onuAllocIds[entry].valid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwAllocExist
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function true if Alloc-Id exist in the onu
**               
**  PARAMETERS:  MV_U32 allocId
**
**  OUTPUTS:     None
**
**  RETURNS:     Alloc-Id existance
**                   
*******************************************************************************/
MV_BOOL onuGponDbBwAllocExist(MV_U32 allocId)
{
  MV_U32  iEntry;
  MV_U32  entryAllocId;
  MV_BOOL valid;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwAllocGet(iEntry, &entryAllocId, &valid);
    if ((entryAllocId == allocId) && (valid == MV_TRUE))
    {
      return(MV_TRUE);
    }
  }

  return(MV_FALSE);
}

/*******************************************************************************
**
**  onuGponDbBwAllocInsert
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function assign Alloc-Id to T-Cont
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 *entry
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwAllocInsert(MV_U32 allocId, MV_U32 *entry)
{
  MV_U32  iEntry;
  MV_U32  entryAllocId;
  MV_BOOL valid;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwAllocGet(iEntry, &entryAllocId, &valid);
    if (valid == MV_FALSE)
    {
      onuGponDbBwAllocSet(iEntry, allocId, MV_TRUE);
      *entry = iEntry;
      return(MV_OK);
    }
  }

  return(MV_ERROR);
}

/*******************************************************************************
**
**  onuGponDbBwAllocRemove
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function remove Alloc-Id from T-Cont
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 *entry
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwAllocRemove(MV_U32 allocId, MV_U32 *entry)
{
  MV_U32  iEntry;
  MV_U32  entryAllocId;
  MV_BOOL valid;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwAllocGet(iEntry, &entryAllocId, &valid);
    if ((entryAllocId == allocId) && (valid == MV_TRUE))
    {
      onuGponDbBwAllocSet(iEntry, PON_ONU_ALLOC_NOT_EXIST, MV_FALSE);
      *entry = iEntry;
      return(MV_OK);
    }
  }

  return(MV_ERROR);
}

/* ======================================== */
/*   ONU BW IDLE ALLOC SECTION              */
/* ======================================== */

/*******************************************************************************
**
**  onuGponDbBwIdleAllocSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function sets Idle Alloc-Id value 
**               
**  PARAMETERS:  MV_U32 entry
**               MV_U32 allocId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwIdleAllocSet(MV_U32 entry, MV_U32 allocId)
{
  if (entry >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponBwTbl_s.onuIdleAllocIds[entry] = allocId;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwIdleAllocGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return Idle Alloc-Id value 
**               
**  PARAMETERS:  MV_U32 entry
**               MV_U32 allocId
**
**  OUTPUTS:     None
**
**  RETURNS:     Alloc-Id value & state
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwIdleAllocGet(MV_U32 entry, MV_U32 *allocId)
{
  if (entry >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    *allocId = 0;
    return(MV_ERROR);
  }

  *allocId = onuGponDb_s.onuGponBwTbl_s.onuIdleAllocIds[entry];

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwIdleAllocExist
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function true if Idle Alloc-Id exist in the onu
**               
**  PARAMETERS:  MV_U32 allocId
**
**  OUTPUTS:     None
**
**  RETURNS:     Alloc-Id existance
**                   
*******************************************************************************/
MV_BOOL onuGponDbBwIdleAllocExist(MV_U32 allocId, MV_U32 *entry)
{
  MV_U32 iEntry;
  MV_U32 entryAllocId;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwIdleAllocGet(iEntry, &entryAllocId);
    if (entryAllocId == allocId)
    {
      *entry = entryAllocId;
      return(MV_TRUE);
    }
  }

  *entry = 0;
  return(MV_FALSE);
}

/*******************************************************************************
**
**  onuGponDbBwIdleAllocInsert
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function add Idle Alloc-Id
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 *entry
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwIdleAllocInsert(MV_U32 allocId, MV_U32 *entry)
{
  MV_U32  iEntry;
  MV_U32  entryAllocId;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwIdleAllocGet(iEntry, &entryAllocId);
    if (entryAllocId == PON_ONU_ALLOC_NOT_EXIST)
    {
      onuGponDbBwIdleAllocSet(iEntry, allocId);
      *entry = iEntry;
      return(MV_OK);
    }
  }

  return(MV_ERROR);
}

/*******************************************************************************
**
**  onuGponDbBwIdleAllocRemove
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function remove Idle Alloc-Id
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 *entry
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwIdleAllocRemove(MV_U32 allocId, MV_U32 *entry)
{
  MV_U32 iEntry;
  MV_U32 entryAllocId;

  for (iEntry = 0 ; iEntry < ONU_GPON_MAX_NUM_OF_T_CONTS ; iEntry++)
  {
    onuGponDbBwIdleAllocGet(iEntry, &entryAllocId);
    if (entryAllocId == allocId) 
    {
      onuGponDbBwIdleAllocSet(iEntry, PON_ONU_ALLOC_NOT_EXIST);
      *entry = iEntry;
      return(MV_OK);
    }
  }

  return(MV_ERROR);
}

/* ======================================== */
/*   ONU BW TCONT SECTION                   */
/* ======================================== */

/*******************************************************************************
**
**  onuGponDbBwTcontSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set an entry in the T-Cont table
**               
**  PARAMETERS:  MV_U32  tcontNum
**               MV_BOOL exist
**               MV_U32  allocId
**               MV_BOOL valid
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontSet(MV_U32  tcontNum, 
                              MV_BOOL exist, 
                              MV_U32  allocId, 
                              MV_BOOL valid)
{
  if (tcontNum >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    return(MV_ERROR);
  }

  onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].exist   = exist;
  onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].allocId = allocId;
  onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].valid   = valid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwTcontGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return an entry from the T-Cont table
**               
**  PARAMETERS:  MV_U32  tcontNum
**               MV_BOOL *exist
**               MV_U32  *allocId
**               MV_BOOL *valid
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontGet(MV_U32  tcontNum, 
                              MV_BOOL *exist, 
                              MV_U32  *allocId, 
                              MV_BOOL *valid)
{
  if (tcontNum >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    return(MV_ERROR);
  }

  *exist   = onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].exist;
  *allocId = onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].allocId;
  *valid   = onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].valid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwTcontExist
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return true if T-Cont entry is valid
**               
**  PARAMETERS:  MV_U32 tcontNum
**               MV_BOOL   *exist
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontExist(MV_U32 tcontNum, MV_BOOL *exist)
{
  if (tcontNum >= ONU_GPON_MAX_NUM_OF_T_CONTS)
  {
    return(MV_ERROR);
  }

  *exist = onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].exist;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwTcontClear
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function reset T-Cont table entry
**               
**  PARAMETERS:  MV_U32 tcontNum
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontClear(MV_U32 tcontNum)
{
  if ((tcontNum >= ONU_GPON_MAX_NUM_OF_T_CONTS) || 
      (onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].exist != MV_TRUE))
  {
    return(MV_ERROR);
  }

  onuGponDbBwTcontSet(tcontNum, MV_TRUE, PON_ONU_ALLOC_NOT_EXIST, MV_FALSE);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwTcontAlloc
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set T-Cont table entry
**               
**  PARAMETERS:  MV_U32 tcontNum
**               MV_U32 allocId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontAlloc(MV_U32 tcontNum, MV_U32 allocId)
{
  if ((tcontNum >= ONU_GPON_MAX_NUM_OF_T_CONTS) || 
      (onuGponDb_s.onuGponBwTbl_s.onuTcontIds[tcontNum].exist != MV_TRUE))
  {
    return(MV_ERROR);
  }

  onuGponDbBwTcontSet(tcontNum, MV_TRUE, allocId, MV_TRUE);
  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwTcontConnectCheck
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return T-Cont table entry according to Alloc-Id
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 *tcontNum
**               MV_BOOL   *valid
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontConnectCheck(MV_U32 allocId, MV_U32 *tcontNum, MV_BOOL *valid)
{
  MV_BOOL exist;
  MV_U32  dbAllocId;
  MV_BOOL dbValid;
  MV_U32  iTcont;

  for (iTcont = 0 ; iTcont < ONU_GPON_MAX_NUM_OF_T_CONTS ; iTcont++)
  {
    onuGponDbBwTcontGet(iTcont, &exist, &dbAllocId, &dbValid);
    if ((exist == MV_TRUE) && (dbValid == MV_TRUE) && (allocId == dbAllocId))
    {
      *valid    = dbValid;
      *tcontNum = iTcont;
      return(MV_OK);
    }
  }

  *valid    = MV_FALSE;
  *tcontNum = 0;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDbBwTcontFreeGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return free T-Cont table entry according to Alloc-Id
**               
**  PARAMETERS:  MV_U32 allocId
**               MV_U32 *tcontNum
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuGponDbBwTcontFreeGet(MV_U32 allocId, MV_U32 *tcontNum)
{
  MV_BOOL exist;
  MV_U32  dbAllocId;
  MV_BOOL dbValid;
  MV_U32  iTcont;
  MV_U32  tempTcont = 0;

  for (iTcont = 0 ; iTcont < ONU_GPON_MAX_NUM_OF_T_CONTS ; iTcont++)
  {
    onuGponDbBwTcontGet(iTcont, &exist, &dbAllocId, &dbValid);
    if ((exist == MV_TRUE) && (dbValid == MV_FALSE))
    {
      *tcontNum = iTcont;
      return(MV_OK);
    }

    if (dbAllocId == allocId) 
    {
      tempTcont = iTcont;
    }
  }

  *tcontNum = tempTcont;

  return(MV_ERROR);
}
