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
**  FILE        : ponOnuLnxKsUI.c                                            **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON CLI functionality            **
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
#include "mvSysPonConfig.h"

/* Local Constant
------------------------------------------------------------------------------*/                                               
#define __FILE_DESC__ "mv_pon/perf/ponOnuLnxKsUI.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/
MV_BOOL infoCntReadClearInd = MV_TRUE;

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/

/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ==                                                                      == */
/* ==           =========   =========   =========   ===       ==           == */                       
/* ==           =========   =========   =========   ====      ==           == */
/* ==           ==          ==     ==   ==     ==   == ==     ==           == */
/* ==           ==          ==     ==   ==     ==   ==  ==    ==           == */
/* ==           =========   =========   ==     ==   ==   ==   ==           == */
/* ==           =========   =========   ==     ==   ==    ==  ==           == */
/* ==           ==     ==   ==          ==     ==   ==     == ==           == */
/* ==           ==     ==   ==          ==     ==   ==      ====           == */
/* ==           =========   ==          =========   ==       ===           == */
/* ==           =========   ==          =========   ==        ==           == */
/* ==                                                                      == */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/

#ifdef CONFIG_MV_GPON_MODULE

/******************************************************************************/
/* ========================================================================== */
/*                            Info Commands                                   */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponUiInfoShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu information
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiInfoShow(char* buf)
{
  MV_U8   *stateText[] = {"",
                           "INITIAL",
                           "STANDBY",
                           "SERIAL NUMBER",
                           "RANGING",
                           "OPERATION",
                           "POPUP",
                           "EMERGENCY STOP"};
  MV_U8   *boolText[] = {"FALSE","TRUE"};
  MV_U8   serialNumber[8];
  MV_U8   password[10];
  MV_U32  onuId,onuState;
  MV_BOOL init,omcc,snMaskEnable,snMaskMatchMode;
  MV_U32  berInt, dummy,omccPort;
  MV_U32  guard,pre1Size;
  MV_U32  pre2Size,pre3RaSize,pre3OpSize,pre3Pat;
  MV_U32  dbDelimiter,asicDelimiter,sd,sf;
  MV_U32  intDelay,eqdDb,eqdAsic,fDelay;
  MV_U32  snMsg[3], idleMsg[3];
  MV_U8   *mchText[] = {"NO MATCH","MATCH "};
  int     off = 0;

                    onuGponDbSerialNumGet(serialNumber);
  onuState        = onuGponDbOnuStateGet();
  onuId           = onuGponDbOnuIdGet();
  init            = onuGponDbInitGet();
  omcc            = onuGponDbOmccValidGet();
  omccPort        = onuGponDbOmccPortGet();
  snMaskEnable    = onuGponDbSerialNumberMaskEnableGet();
  snMaskMatchMode = onuGponDbSerialNumberMaskMatchGet();
                    onuGponDbPasswordGet(password);
  guard           = onuGponDbGuardBitsGet();
  berInt          = onuGponDbBerIntervalGet();
                    onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_01,&dummy,&pre1Size);
                    onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_02,&dummy,&pre2Size);
                    onuGponDbPreambleGet(ONU_GPON_PREM_TYPE_03,&pre3Pat,&dummy);
  pre3OpSize      = onuGponDbExtPreambleOperGet();
  pre3RaSize      = onuGponDbExtPreambleSyncGet();
  sd              = onuGponDbSdThresholdGet();
  sf              = onuGponDbSfThresholdGet();
  intDelay        = onuGponDbConstDelayGet();
  eqdDb           = onuGponDbEqualizationDelayGet();
                    mvOnuGponMacRxEqualizationDelayGet(&eqdAsic);
                    mvOnuGponMacTxFinalDelayGet(&fDelay);
  dbDelimiter     = onuGponDbDelimiterGet(ONU_GPON_DELM_BYTE_01) | 
                   (onuGponDbDelimiterGet(ONU_GPON_DELM_BYTE_02) << 8) |
                   (onuGponDbDelimiterGet(ONU_GPON_DELM_BYTE_03) << 16);
                    mvOnuGponMacTxDelimiterGet(&asicDelimiter,&dummy); 
                    onuGponDbSnMsgGet(snMsg);
                    onuGponDbIdleMsgGet(idleMsg);

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "ONT Full Information:\n");
  off += mvOsSPrintf(buf+off, "---------------------\n");
  off += mvOsSPrintf(buf+off, "SN[VENDOR ID]:                 %02X:%02X:%02X:%02X [%c%c%c%c]\n",
                     serialNumber[0],serialNumber[1],serialNumber[2],serialNumber[3],
                     serialNumber[0],serialNumber[1],serialNumber[2],serialNumber[3]);
  off += mvOsSPrintf(buf+off, "SN[Serial Number]:             %02X:%02X:%02X:%02X\n",
                     serialNumber[4],serialNumber[5],serialNumber[6],serialNumber[7]);
  off += mvOsSPrintf(buf+off, "ONU ID:                        %d\n", onuId);
  off += mvOsSPrintf(buf+off, "ONU STATE:                     %d [%s]\n", onuState, stateText[onuState]);
  off += mvOsSPrintf(buf+off, "INIT STATE:                    %s\n", boolText[init]);
  off += mvOsSPrintf(buf+off, "OMCC Valid:                    %s\n", boolText[omcc]);
  off += mvOsSPrintf(buf+off, "OMCC Port:                     %d\n", omccPort);
  off += mvOsSPrintf(buf+off, "Password:                      %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\n",
                     password[0],password[1],password[2],password[3],password[4],
                     password[5],password[6],password[7],password[8],password[9]);
  off += mvOsSPrintf(buf+off, "BER Interval:                  %d\n", berInt);
  off += mvOsSPrintf(buf+off, "SD Threshold:                  %d\n", sd);
  off += mvOsSPrintf(buf+off, "SF Threshold:                  %d\n", sf);
  off += mvOsSPrintf(buf+off, "Guard Bits:                    %d\n", guard);
  off += mvOsSPrintf(buf+off, "Preamble Type1 Size:           %d\n", pre1Size);
  off += mvOsSPrintf(buf+off, "Preamble Type2 Size:           %d\n", pre2Size);
  off += mvOsSPrintf(buf+off, "Preamble Type3 Pattern:        0x%02X\n", pre3Pat);
  off += mvOsSPrintf(buf+off, "Preamble Type3 Range Size:     %d\n", pre3RaSize);
  off += mvOsSPrintf(buf+off, "Preamble Type3 Oper Size:      %d\n", pre3OpSize);
  off += mvOsSPrintf(buf+off, "Delimiter:                     0x%08X [0x%08X]\n", dbDelimiter, asicDelimiter);
  off += mvOsSPrintf(buf+off, "Internal Delay:                %d [0x%x]\n", intDelay, intDelay);
  off += mvOsSPrintf(buf+off, "Equalization Delay:            %d [0x%x] (HW:0x%x)\n", eqdDb, eqdDb, eqdAsic);
  off += mvOsSPrintf(buf+off, "Final Delay:                   %d\n", fDelay);
  off += mvOsSPrintf(buf+off, "Const Idle Ploam:              [0x%08X][0x%08X][0x%08X]\n", idleMsg[0],idleMsg[1],idleMsg[2]);
  off += mvOsSPrintf(buf+off, "Const Serial Number Ploam:     [0x%08X][0x%08X][0x%08X]\n", snMsg[0],snMsg[1],snMsg[2]);
  off += mvOsSPrintf(buf+off, "Serial Number Mask Enable:     %s\n", boolText[snMaskEnable]);                     
  off += mvOsSPrintf(buf+off, "Serial Number Mask Match Mode: %s\n", mchText [snMaskMatchMode]);
  off += mvOsSPrintf(buf+off, "Debug Mode:                    %s\n", boolText[onuGponPonMngDebugModeGet()]);
  off += mvOsSPrintf(buf+off, "Overhead Manual Mode:          %s\n", boolText[onuGponPonMngOverheadManualModeGet()]);

  return(off);
}

/*******************************************************************************
**
**  onuGponUiAlarmShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu alarms
**               
**  PARAMETERS:  char* buf 
**                         
**  OUTPUTS:     char* buf 
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiAlarmShow(char* buf)
{
  MV_STATUS rcode;
  MV_U32    alarms;
  MV_U32    iAlarm;
  MV_U8     *alarmText[] = {"LOS ","LOF ","LCDA","LCDG"," SF "," SD "," TF ",
                            "SUF ","MEM ","DACT","DIS ","MIS ","PEE ","RDI "};
  MV_U8     *statusText[] = {"OFF","ON "};
  MV_U32    status;
  int       off = 0;

  rcode = onuGponApiAlarmsGet(&alarms);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get alarms status, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "------------------\n");
  off += mvOsSPrintf(buf+off, "|ONU ALARMS:     |\n");
  off += mvOsSPrintf(buf+off, "------------------\n");
  off += mvOsSPrintf(buf+off, "| ALARM | STATUS |\n");
  off += mvOsSPrintf(buf+off, "------------------\n");

  for (iAlarm = 0 ; iAlarm < ONU_GPON_MAX_ALARMS ; iAlarm++)
  {
    status = (alarms >> iAlarm) & 0x01;
    off += mvOsSPrintf(buf+off, " %s   | %s    |\n", alarmText[iAlarm], statusText[status]);  
  }

  off += mvOsSPrintf(buf+off, "------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiInfoHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print info help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiInfoHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat info  - dump onu information\n");
  off += mvOsSPrintf(buf+off, " cat alarm - dump onu alarms\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/******************************************************************************/
/* ========================================================================== */
/*                            PM Commands                                     */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponUiCountersReadClearFlag
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu FEC counters
**               
**  PARAMETERS:  char* buf
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     void
**                   
*******************************************************************************/
void onuGponUiCountersReadClearFlag(MV_BOOL clear)
{
  infoCntReadClearInd = clear;
}

/*******************************************************************************
**
**  onuGponUiFecCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu FEC counters
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiFecCountersShow(char* buf)
{
  MV_STATUS        rcode;
  S_GponIoctlFecPm counters;
  int              off = 0;

  MV_U8  *counterText[] = {"FEC received Bytes       ",
                           "FEC corrected Bytes      ",
                           "FEC corrected Bits       ",
                           "FEC received CodeWords   ",
                           "FEC uncorrected CodeWords"};

  rcode = onuGponApiPmFecPmGet(&counters,infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }
  
  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "------------------\n");
  off += mvOsSPrintf(buf+off, "| FEC COUNTERS:   |\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| COUNTER                   |   VALUE      |\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[0],counters.receivedBytes);
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[1],counters.correctedBytes);
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[2],counters.correctedBits); 
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[3],counters.receivedCodeWords);
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[4],counters.uncorrectedCodeWords); 
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiRxPloamCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu Rx Ploam counters
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiRxPloamCountersShow(char* buf)
{
  MV_STATUS            rcode;
  S_GponIoctlPloamRxPm counters;
  S_GponIoctlPloamTxPm countDummy;
  MV_U32               MsgId;
  int                  off = 0;

  MV_U8 *counterText[] = {"Idle Ploam Counter                 ",
                          "CRC Error Ploam Counter            ", 
                          "FIFO Overflaw Error Ploam Counter  ",
                          "received Broadcast Ploam Counter   ",
                          "Received Onu Id Ploam Counter      "};

  MV_U8 *msgIdCntText  =  {"Total PLOAM        "};
  MV_U8 *msgIdText[]   =  {"ERROR PLOAM        ",
                           "OVERHEAD           ",
                           "SN_MASK            ", 
                           "ASSIGN_ONU_ID      ",
                           "RANGING_TIME       ",
                           "DACT_ONU_ID        ",
                           "DIS_SN             ",
                           "CONFIG_VP_VC       ",
                           "ENCRYPT_VPI_PORT_ID",
                           "REQ_PASSWORD       ",
                           "ASSIGN_ALLOC_ID    ",
                           "NO_MESSAGE         ",
                           "POPUP              ",
                           "REQ_KEY            ",
                           "CONFIG_PORT_ID     ",
                           "PHYSICAL_EQUIP_ERR ",
                           "CHANGE_POWER_LEVEL ",
                           "PST                ",
                           "BER_INTERVAL       ",
                           "KEY_SWITCH_TIME    ",
                           "EXT_BURST_LEN      "};

  rcode  = onuGponApiPmRxPloamPmGet(&counters, infoCntReadClearInd);
  rcode |= onuGponApiAdvancedPloamsCounterGet(&countDummy, &counters, infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "-------------------------\n");
  off += mvOsSPrintf(buf+off, "|RX PLOAM COUNTERS:     |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| HW COUNTERS                          |    VALUE    |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[0],counters.rxIdlePloam);         
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[1],counters.rxCrcErrorPloam);     
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[2],counters.rxFifoOverErrorPloam);
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[3],counters.rxBroadcastPloam);    
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[4],counters.rxOnuIdPloam);        
  off += mvOsSPrintf(buf+off, "------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| SW COUNTERS          |    VALUE    |\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", msgIdCntText, counters.rxMsgTotalPloam);

  for (MsgId = 0; MsgId <= ONU_GPON_DS_MSG_LAST; MsgId++)
    off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", msgIdText[MsgId], counters.rxMsgIdPloam[MsgId]);

  off += mvOsSPrintf(buf+off, "--------------------------------------\n");

  return(off);
}
                
/*******************************************************************************
**
**  onuGponUiTxPloamCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu Tx Ploam counters
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiTxPloamCountersShow(char* buf)
{
  MV_STATUS            rcode;
  S_GponIoctlPloamRxPm countDummy;
  S_GponIoctlPloamTxPm counters;
  MV_U32               MsgId;
  int                  off = 0;

  MV_U8 *msgIdText[] = {"Total Ploams Recived",
                        "SN_ONU              ",
                        "PASSWORD            ", 
                        "DYING_GASP          ",
                        "NO_MESSAGE          ",
                        "ENCRYPT_KEY         ",
                        "PHYSICAL_EQUIP_ERR  ",
                        "PST                 ",
                        "REI                 ",
                        "ACK                 "};

  rcode = onuGponApiAdvancedPloamsCounterGet(&counters, &countDummy, infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "-------------------------\n");
  off += mvOsSPrintf(buf+off, "|TX PLOAM COUNTERS:     |\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| COUNTER             |    VALUE    |\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s | %10d  |\n", msgIdText[0], counters.txMsgTotalPloam);
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| TX Error            |    VALUE    |\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");

  for (MsgId=1;MsgId <= ONU_GPON_US_MSG_LAST; MsgId++)
  {
    off += mvOsSPrintf(buf+off, " %s | %10d  |\n", msgIdText[MsgId], counters.txErrMsgIdPloam[MsgId]);

  }

  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| TX Correct          |    VALUE    |\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");

  for (MsgId=1;MsgId <= ONU_GPON_US_MSG_LAST; MsgId++)
  {
    off += mvOsSPrintf(buf+off, " %s | %10d  |\n", msgIdText[MsgId], counters.txMsgIdPloam[MsgId]);
  }

  off += mvOsSPrintf(buf+off, "-------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiBwMapCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu bw map counters
**               
**  PARAMETERS:  char* buf
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiBwMapCountersShow(char* buf)
{
  MV_STATUS          rcode;
  S_GponIoctlBwMapPm counters;
  int                off = 0;

  MV_U8 *counterText[] = {"Alloc Crc Error               ", 
                          "Alloc Correctable Crc Error   ",
                          "Alloc UnCorrectable Crc Error ",
                          "Alloc Correct                 ",
                          "Total Received Alloc Bytes    "};

  rcode = onuGponApiPmRxBwMapPmGet(&counters, infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "----------------------\n");
  off += mvOsSPrintf(buf+off, "| BWMAP COUNTERS:    |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| COUNTER                         |   VALUE    |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s   | %10u |\n", counterText[0],counters.allocCrcErr);
  off += mvOsSPrintf(buf+off, " %s   | %10u |\n", counterText[1],counters.allocCorrectableCrcErr);
  off += mvOsSPrintf(buf+off, " %s   | %10u |\n", counterText[2],counters.allocUnCorrectableCrcErr);
  off += mvOsSPrintf(buf+off, " %s   | %10u |\n", counterText[3],counters.allocCorrec);
  off += mvOsSPrintf(buf+off, " %s   | %10u |\n", counterText[4],counters.totalReceivedAllocBytes);
  off += mvOsSPrintf(buf+off, "------------------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiStandardCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu standard counters
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiStandardCountersShow(char* buf)
{
  MV_STATUS         rcode;
  S_RxStandardApiPm counters;
  int               off = 0;

  MV_U8 *counterText[] = {"bip8 Error Counter     ",
                          "plend Counter          "};

  rcode = onuGponApiPmRxStandardPmGet(&counters, infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "-------------------------\n");
  off += mvOsSPrintf(buf+off, "|STANDARD COUNTERS:     |\n");
  off += mvOsSPrintf(buf+off, "-----------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| COUNTER                  |    VALUE   |\n");
  off += mvOsSPrintf(buf+off, "-----------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s   | %8u   |\n", counterText[0],counters.bip8);
  off += mvOsSPrintf(buf+off, " %s   | %8u   |\n",counterText[1],counters.plend);
  off += mvOsSPrintf(buf+off, "-----------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiGemCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu gem counters
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiGemCountersShow(char* buf)
{
  MV_STATUS          rcode;
  S_GponIoctlGemRxPm counters;
  int                off = 0;

  MV_U8 *counterText[] = {"Received Idle Gem Frames                       ", 
                          "Received Valid Gem Frames                      ",
                          "Received Undefined Gem Frames                  ",
                          "Received Omci Frames                           ",
                          "Dropped Gem Frames                             ",
                          "Dropped Omci Frames                            ",
                          "Received Gem Frames With Uncorr Hec Error      ",
                          "Received Gem Frames With One Fixed Hec Error   ",
                          "Received Gem Frames With Two Fixed Hec Error   ",
                          "Received Valid Gem Frames Total Byte Count     ",
                          "Received Undefined Gem Frames Total Byte Count ",
                          "Gem Reassemble Memory Flush                    ",
                          "Gem Synch Lost                                 ",
                          "Received Eth Frames With Correct FCS           ",
                          "Received Eth Frames With FCS Error             ",
                          "Received Omci Frames With Correct CRC          ",
                          "Received Omci Frames With CRC Error            "};
      
  rcode = onuGponApiGemRxCounterGet(&counters, infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "----------------------\n");
  off += mvOsSPrintf(buf+off, "|GEM COUNTERS:       |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| COUNTER                                          |  VALUE      |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");
  
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[0], counters.gemRxIdleGemFrames);                             
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[1], counters.gemRxValidGemFrames);                            
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[2], counters.gemRxUndefinedGemFrames);                        
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[3], counters.gemRxOmciFrames);                                
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[4], counters.gemRxDroppedGemFrames);                          
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[5], counters.gemRxDroppedOmciFrames);                         
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[6], counters.gemRxGemFramesWithUncorrHecErr);                 
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[7], counters.gemRxGemFramesWithOneFixedHecErr);               
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[8], counters.gemRxGemFramesWithTwoFixedHecErr);               
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[9], counters.gemRxTotalByteCountOfReceivedValidGemFrames);    
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[10],counters.gemRxTotalByteCountOfReceivedUndefinedGemFrames);
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[11],counters.gemRxGemReassembleMemoryFlush);                  
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[12],counters.gemRxGemSynchLost);                              
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[13],counters.gemRxEthFramesWithCorrFcs);                      
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[14],counters.gemRxEthFramesWithFcsError);                     
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[15],counters.gemRxOmciFramesWithCorrCrc);                     
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[16],counters.gemRxOmciFramesWithCrcError);                    
  off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");

  return(off);
}
                                                                               
/*******************************************************************************
**
**  onuGponUiTxCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu tx counters
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiTxCountersShow(char* buf)
{
  MV_STATUS          rcode;
  S_GponIoctlGemTxPm counters;
  MV_BOOL            exist;
  MV_U32             tcont;
  int                off = 0;

  MV_U8 *counterText[] = {"Transmitted Gem Pti Type-1 Frames              ", 
                          "Transmitted Gem Pti Type-0 Frames              ",
                          "Transmitted Idle Gem Frames                    ",
                          "Transmitted Eth Frames Via Tcont               ",
                          "Transmitted Eth Bytes Via Tcont                ",
                          "Transmitted Gem Frames Via Tcont               ",
                          "Transmitted Idle Gem Frames Via Tcont          "};

  rcode = onuGponApiGemTxCounterGet(&counters, infoCntReadClearInd);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get counters value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "----------------------\n");
  off += mvOsSPrintf(buf+off, "|TX COUNTERS:        |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| COUNTER                                          |  VALUE      |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[0],counters.gemTxGemPtiTypeOneFrames);                         
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[1],counters.gemTxGemPtiTypeZeroFrames);                        
  off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[2],counters.gemTxIdleGemFrames);                    
   
  for (tcont = 0; tcont < ONU_GPON_MAX_NUM_OF_T_CONTS; tcont++)
  {
    onuGponDbBwTcontExist(tcont, &exist);
    if (exist != MV_FALSE)
    {
      off += mvOsSPrintf(buf+off, "T-Cont [%d] Counters                                              |\n", tcont);   
      off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");
      off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[3],counters.gemTxEthFramesViaTconti[tcont]);                              
      off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[4],counters.gemTxEthBytesViaTconti[tcont]);                             
      off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[5],counters.gemTxGemFramesViaTconti[tcont]);
      off += mvOsSPrintf(buf+off, " %s   | %10u  |\n", counterText[6],counters.gemTxIdleGemFramesViaTconti[tcont]); 
    }
  }

  off += mvOsSPrintf(buf+off, "------------------------------------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiRawCounters
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu counters
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiRaw_1Counters(char* buf)
{
  MV_U32      suspected;
  MV_U32      decided;
  S_RxPloamPm inRxPloamCounters;
  S_RxBwMapPm inRxBwMapCounters;
  S_RxBip8Pm  inBip8Counter;
  S_RxPlendPm inPlendCounter;
  S_RxFecPm   inRxFecCounters;
  S_GemPm     gemCounters;
  int         off = 0;

  decided = onuGponPonMngrFifoErrCountersGet(&suspected);
  onuGponPmFecPmGet(&inRxFecCounters);
  onuGponPmRxBwMapPmGet(&inRxBwMapCounters);
  onuGponPmRxBip8PmGet(&inBip8Counter);
  onuGponPmRxPlendPmGet(&inPlendCounter);
  onuGponPmRxPloamPmGet(&inRxPloamCounters);
  onuGponPmGemPmGet(&gemCounters);

  off += mvOsSPrintf(buf+off, "ONT GPON Counters\n");
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "PLOAM FIFO SYNC ERROR SUSPECTED:                   %u\n", suspected);
  off += mvOsSPrintf(buf+off, "PLOAM FIFO SYNC ERROR DECIDED:                     %u\n", decided);
  off += mvOsSPrintf(buf+off, "This ONU ID PLOAMs:                                %u\n", inRxPloamCounters.receivedOnuIdPloamCounter); 
  off += mvOsSPrintf(buf+off, "Broadcast PLOAMs:                                  %u\n", inRxPloamCounters.receivedBroadcastPloamCounter); 
  off += mvOsSPrintf(buf+off, "Idles PLOAMs:                                      %u\n", inRxPloamCounters.idlePloamCounter);           
  off += mvOsSPrintf(buf+off, "CRC Error PLOAMs:                                  %u\n", inRxPloamCounters.crcErrorPloamCounter);       
  off += mvOsSPrintf(buf+off, "FIFO Full Dropped PLOAMs:                          %u\n", inRxPloamCounters.fifoOverErrorPloamCounter); 
  off += mvOsSPrintf(buf+off, "BW Error Free Allocs:                              %u\n", inRxBwMapCounters.allocCorrec);              
  off += mvOsSPrintf(buf+off, "BW Uncorrectable Allocs:                           %u\n", inRxBwMapCounters.allocUnCorrectableCrcErr);
  off += mvOsSPrintf(buf+off, "BW Bad CRC Allocs:                                 %u\n", inRxBwMapCounters.allocCrcErr);             
  off += mvOsSPrintf(buf+off, "BW Error Free and Correctable Allocs:              %u\n", inRxBwMapCounters.allocCorrectableCrcErr);  
  off += mvOsSPrintf(buf+off, "BW Total Allocation:                               %u\n", inRxBwMapCounters.totalReceivedAllocBytes);
  off += mvOsSPrintf(buf+off, "Bip8:                                              %u\n", inBip8Counter.bip8);
  off += mvOsSPrintf(buf+off, "Plend:                                             %u\n", inPlendCounter.plend);
  off += mvOsSPrintf(buf+off, "FEC correctedBits:                                 %u\n", inRxFecCounters.correctedBits);        
  off += mvOsSPrintf(buf+off, "FEC correctedBytes:                                %u\n", inRxFecCounters.correctedBytes);       
  off += mvOsSPrintf(buf+off, "FEC receivedBytes:                                 %u\n", inRxFecCounters.receivedBytes);        
  off += mvOsSPrintf(buf+off, "FEC receivedCodeWords:                             %u\n", inRxFecCounters.receivedCodeWords);    
  off += mvOsSPrintf(buf+off, "FEC uncorrectedCodeWords:                          %u\n", inRxFecCounters.uncorrectedCodeWords);
  off += mvOsSPrintf(buf+off, "GEM Received Idle Gem Frames                       %u\n", gemCounters.receivedIdleGemFrames);     
  off += mvOsSPrintf(buf+off, "GEM Received Valid Gem Frames                      %u\n", gemCounters.receivedValidGemFrames);    
  off += mvOsSPrintf(buf+off, "GEM Received Undefined Gem Frames                  %u\n", gemCounters.receivedUndefinedGemFrames);
  off += mvOsSPrintf(buf+off, "GEM Received Omci Frames                           %u\n", gemCounters.receivedOmciFrames);        
  off += mvOsSPrintf(buf+off, "GEM Dropped Gem Frames                             %u\n", gemCounters.droppedGemFrames);          
  off += mvOsSPrintf(buf+off, "GEM Dropped Omci Frames                            %u\n", gemCounters.droppedOmciFrames);        
  off += mvOsSPrintf(buf+off, "GEM Received Gem Frames With Uncorr Hec Error      %u\n", gemCounters.receivedGemFramesWithUncorrHecErr);          
  off += mvOsSPrintf(buf+off, "GEM Received Gem Frames With One Fixed Hec Error   %u\n", gemCounters.receivedGemFramesWithOneFixedHecErr);        
  off += mvOsSPrintf(buf+off, "GEM Received Gem Frames With Two Fixed Hec Error   %u\n", gemCounters.receivedGemFramesWithTwoFixedHecErr);        
  off += mvOsSPrintf(buf+off, "GEM Received Valid Gem Frames Total Byte Count     %u\n", gemCounters.totalByteCountOfReceivedValidGemFrames);     
  off += mvOsSPrintf(buf+off, "GEM Received Undefined Gem Frames Total Byte Count %u\n", gemCounters.totalByteCountOfReceivedUndefinedGemFrames);
  off += mvOsSPrintf(buf+off, "GEM Gem Reassemble Memory Flush                    %u\n", gemCounters.gemReassembleMemoryFlush);       
  off += mvOsSPrintf(buf+off, "GEM Gem Synch Lost                                 %u\n", gemCounters.gemSynchLost);                   
  off += mvOsSPrintf(buf+off, "GEM Received Eth Frames With Correct FCS           %u\n", gemCounters.receivedEthFramesWithCorrFcs);   
  off += mvOsSPrintf(buf+off, "GEM Received Eth Frames With FCS Error             %u\n", gemCounters.receivedEthFramesWithFcsError);  
  off += mvOsSPrintf(buf+off, "GEM Received Omci Frames With Correct CRC          %u\n", gemCounters.receivedOmciFramesWithCorrCrc);  
  off += mvOsSPrintf(buf+off, "GEM Received Omci Frames With CRC Error            %u\n", gemCounters.receivedOmciFramesWithCrcError);

  return(off);
}

int onuGponUiRaw_2Counters(char* buf)
{
  S_TxPm txCounters;
  MV_U32 tcont;
  int    off = 0;

  onuGponPmTxPmGet(&txCounters);

  off += mvOsSPrintf(buf+off, "ONT GPON Raw Counters\n");
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
  
  off += mvOsSPrintf(buf+off, "Transmitted Gem Pti Type-1 Frames                  %u\n", txCounters.transmittedGemPtiTypeOneFrames);     
  off += mvOsSPrintf(buf+off, "Transmitted Gem Pti Type-0 Frames                  %u\n", txCounters.transmittedGemPtiTypeZeroFrames);    
  off += mvOsSPrintf(buf+off, "Transmitted Idle Gem Frames                        %u\n", txCounters.transmittedIdleGemFrames);           
  off += mvOsSPrintf(buf+off, "Transmitted Tx Enable Count                        %u\n", txCounters.transmittedTxEnableCount);           
  off += mvOsSPrintf(buf+off, "Transmitted Tx Enable Threshold Count              %u\n", txCounters.transmittedTxEnableThresholdCount); 

  for (tcont = 0; tcont < ONU_GPON_MAX_NUM_OF_T_CONTS; tcont++)
  {
    off += mvOsSPrintf(buf+off, "T-Cont [%d] Counters                                      \n", tcont);   
    off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
    off += mvOsSPrintf(buf+off, "Transmitted Eth Frames Via Tcont                   %u\n", txCounters.transmittedEthFramesViaTconti[tcont]);                              
    off += mvOsSPrintf(buf+off, "Transmitted Eth Bytes Via Tcont                    %u\n", txCounters.transmittedEthBytesViaTconti[tcont]);                             
    off += mvOsSPrintf(buf+off, "Transmitted Gem Frames Via Tcont                   %u\n", txCounters.transmittedGemFramesViaTconti[tcont]);             
    off += mvOsSPrintf(buf+off, "Transmitted Idle Gem Frames Via Tcont              %u\n", txCounters.transmittedIdleGemFramesViaTconti[tcont]); 
  }

  return(off);
}

/*******************************************************************************
**
**  onuGponUiPmHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print pm help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiPmHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[DEC]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [state] > cntRdClrState - onu counters flag, Read[0], Read Clear[1]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat rxPloamCnt               - dump Rx PLOAM counters\n");
  off += mvOsSPrintf(buf+off, " cat txPloamCnt               - dump Tx PLOAM counters\n");
  off += mvOsSPrintf(buf+off, " cat bwMapCnt                 - dump bw map counters\n");
  off += mvOsSPrintf(buf+off, " cat stdCnt                   - dump standard counters\n");
  off += mvOsSPrintf(buf+off, " cat fecCnt                   - dump fec counters\n");
  off += mvOsSPrintf(buf+off, " cat gemCnt                   - dump gem counters\n");
  off += mvOsSPrintf(buf+off, " cat txCnt                    - dump Tx counters\n");
  off += mvOsSPrintf(buf+off, " cat rawCnt                   - dump raw counters\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/******************************************************************************/
/* ========================================================================== */
/*                            Data Commands                                   */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponUiTcontConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu tcont <--> alloc
**               
**  PARAMETERS:  MV_U32 alloc
**               MV_U32 tcont 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiTcontConfig(MV_U32 alloc, MV_U32 tcont)
{
  MV_STATUS rcode;

  rcode = onuGponApiTcontConfig(alloc, tcont);
  if (rcode != MV_OK)
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "ONU Allocation Id(%d) to T-CONT(%d) Config - FAIL\n", alloc, tcont);
}

/*******************************************************************************
**
**  onuGponUiClearTcontConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clear onu tcont 
**               
**  PARAMETERS:  MV_U32 tcont 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiClearTcontConfig(MV_U32 tcont)
{
  MV_STATUS rcode;

  rcode = onuGponApiTcontClear(tcont);
  if (rcode != MV_OK)
   mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "ONU T-CONT(%d) Clear - FAIL\n", tcont);
}

/*******************************************************************************
**
**  onuGponUiResetTcontsConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function reset onu bw map table
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiResetTcontsConfig(void)
{
  onuGponApiTcontsReset();
}

/*******************************************************************************
**
**  onuGponUiDebugTcont
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu tcont
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiDebugTcont(char* buf)
{
  MV_U32  allocId;  
  MV_U32  idleAllocId;
  MV_U32  validCount = 0;
  MV_BOOL valid;
  MV_BOOL exist;
  MV_U32  i;
  MV_U8   *boolText[2] = {"NO ","YES"};
  MV_U32  fAllocId;
  MV_BOOL fEna;
  MV_U32  fTcont;
  MV_U32  knowTcont;
  MV_U8   *knowTcontText[] = {"T-CONT 0  ","T-CONT 1  ","T-CONT 2  ",
                              "T-CONT 3  ","T-CONT 4  ","T-CONT 5  ",
                              "T-CONT 6  ","T-CONT 7  ","T-CONT 8  ",
                              "T-Def     ","T-Idle    "};
  int      off = 0;

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| Alloc Id Table                    |\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| Assigned             ||   Idle    |\n");
  off += mvOsSPrintf(buf+off, "|-----------------------------------|\n");
  off += mvOsSPrintf(buf+off, "| ALLOC-ID | Valid     ||           |\n");
  off += mvOsSPrintf(buf+off, "|-----------------------------------|\n");
 
  for (i = 0 ; i < ONU_GPON_MAX_NUM_OF_T_CONTS; i++)
  {
    onuGponDbBwAllocGet(i, &allocId, &valid);
    onuGponDbBwIdleAllocGet(i, &idleAllocId);
    if (idleAllocId != PON_ONU_ALLOC_NOT_EXIST) 
    {
      off += mvOsSPrintf(buf+off, "|   %04d   |  %3s      ||   %04d    |\n", 
                         allocId, boolText[valid], idleAllocId);
    }
    else
    {
      off += mvOsSPrintf(buf+off, "|   %04d   |  %3s      ||   ====    |\n", 
                         allocId, boolText[valid]);
    }
  }
                             
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| Alloc Id to T-CONT Connect  Table |\n");
  off += mvOsSPrintf(buf+off, "|-----------------------------------|\n");
  off += mvOsSPrintf(buf+off, "| T-CONT | Exist | ALLOC-ID | Valid |\n");
  off += mvOsSPrintf(buf+off, "|-----------------------------------|\n");

  for (i = 0 ; i < ONU_GPON_MAX_NUM_OF_T_CONTS; i++)
  {
    onuGponDbBwTcontGet(i,&exist,&allocId,&valid);    
    off += mvOsSPrintf(buf+off, "|   %02d   |  %s  |   %04d   |  %s  |\n", i, boolText[exist], allocId, boolText[valid]);
    if (valid != MV_FALSE) validCount++;
  }

  off += mvOsSPrintf(buf+off, "-------------------------------------\n");
  off += mvOsSPrintf(buf+off, "----------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| BW MAC - SoC Table                |              |\n");
  off += mvOsSPrintf(buf+off, "|-----------------------------------|              |\n");
  off += mvOsSPrintf(buf+off, "| Index | ALLOC-ID | T-CONT | Valid | KNOWN T-CONT |\n");
  off += mvOsSPrintf(buf+off, "|--------------------------------------------------|\n");

  for (i = 0 ; i < ONU_GPON_MAX_NUM_OF_T_CONTS ; i++)
  {
    mvOnuGponMacRxBwMapGet(i,&fAllocId,&fEna,&fTcont);
    if (fTcont == PON_ONU_DEFAULT_ALLOC_TCONT)  knowTcont = 9;   
    else
    {
        if ((fTcont == onuGponIdleAllocIdTcontGet()) && (validCount < 8)) 
         knowTcont = 10;   
        else
         knowTcont = fTcont;
    }

    off += mvOsSPrintf(buf+off, "|  %02d   |   %04d   |   %02d   |  %s  |  %s  |\n",
                       i,fAllocId, fTcont, boolText[fEna],knowTcontText[knowTcont]);
  }

  off += mvOsSPrintf(buf+off, "----------------------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiDebugGemPortidValidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable gem port Id valid configuration
**               
**  PARAMETERS:  MV_U32 portId
**               MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugGemPortidValidSet(MV_U32 portId, MV_U32 enable)
{
  if (enable != 0) onuGponApiGemPortIdConfig(portId);
  else             onuGponApiGemPortIdClear(portId);
}

/*******************************************************************************
**
**  onuGponUiDebugGemPortidValidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function enable gem port Id valid configuration
**               
**  PARAMETERS:  MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugAllGemPortidValidSet(MV_U32 enable)
{
  MV_BOOL enaBool;
  MV_U32  portId;

  enaBool = (enable == 0) ? MV_FALSE : MV_TRUE;

  for (portId = 0; portId < 4096; portId++)
    onuGponApiGemPortIdConfig(portId);
}

/*******************************************************************************
**
**  onuGponUiDebugGemPortid
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return gem port Id valid configuration
**               
**  PARAMETERS:  char* buf
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiDebugGemPortid(char* buf, int range)
{
  MV_BOOL enable;
  MV_U32  portId;
  MV_U32  startRange;
  MV_U32  endRange;
  int     off = 0;

  off += mvOsSPrintf(buf+off, "GEM port ids list\n");
  off += mvOsSPrintf(buf+off, "=================\n");   

  switch (range)
  {
    case 1:  startRange = 1000; endRange = 1999; break;
    case 2:  startRange = 2000; endRange = 2999; break;
    case 3:  startRange = 3000; endRange = 4096; break;
    default: startRange = 0;    endRange = 999;  break;
  }

  for (portId = startRange ; portId < endRange ; portId++)
  {
    enable = mvOnuGponMacPortIdValidGet(portId);
    if (enable == MV_TRUE) off += mvOsSPrintf(buf+off, "[%04d]", portId); 
    if ((portId / 14) == 0) off += mvOsSPrintf(buf+off, "\n"); 
  }

  return(off);
}

#ifdef MV_GPON_STATIC_GEM_PORT
/*******************************************************************************
**
**  onuGponUiDebugDummyGemPortFlagSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set dummy gem port flag
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugDummyGemPortFlagSet(MV_U32 flag)
{
  onuGponApiGemPortIdStaticConfigFlag(flag);
}

/*******************************************************************************
**
**  onuGponUiDebugDummyGemPortReset
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function reset dummy gem port table
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     OK or ERROR 
**                   
*******************************************************************************/
void onuGponUiDebugDummyGemPortReset(void)
{
  onuGponApiGemPortIdStaticConfigReset();
}
#endif /* MV_GPON_STATIC_GEM_PORT */

/*******************************************************************************
**
**  onuGponUiDataHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print data help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiDataHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[DEC]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [AllocId] [T-Cont] > cfgTcont        - config alloc to tcont\n");
  off += mvOsSPrintf(buf+off, " echo [T-Cont]           > clrTcont        - clear alloc Ids from T-Cont\n");
  off += mvOsSPrintf(buf+off, " echo [0]                > rstTcont        - clear T-Cont configuration\n");
  off += mvOsSPrintf(buf+off, " echo [GemPort] [Valid]  > cfgGem          - config gem port valid state, Disable[0], Enable[1]\n");  
  off += mvOsSPrintf(buf+off, " echo [Valid]            > cfgAllGem       - config all gem ports valid state, Disable[0], Enable[1]\n");  
#ifdef MV_GPON_STATIC_GEM_PORT                         
  off += mvOsSPrintf(buf+off, " echo [state]            > dummyGemState   - static gem port flag, Disable[0], Enable[1]\n");
  off += mvOsSPrintf(buf+off, " echo [0]                > dummyGemRst     - clear static gem port table\n");
#endif /* MV_GPON_STATIC_GEM_PORT */
  off += mvOsSPrintf(buf+off, " echo [PortId] [State]   > cfgAesPort      - config gem port encryption state\n");
  off += mvOsSPrintf(buf+off, " echo [State]            > cfgAllAes       - config all gem ports encryption state\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat showTcont                              - dump T-Cont configuration\n"); 
  off += mvOsSPrintf(buf+off, " cat showGem0xxx showGem1xxx showGem2xxx showGem3xxx - dump valid gem port list\n");  
  off += mvOsSPrintf(buf+off, " cat showAes0xxx showAes1xxx showAes2xxx showAes3xxx - dump encrypted gem port list\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");  
  return(off);
}

/******************************************************************************/
/* ========================================================================== */
/*                            Introp Commands                                 */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponUiOverrideShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu override parameters info
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiOverrideShow(char* buf)
{
  MV_U8 *parametersText[] = {"ExtendedBurst",
                             "Delimiter        ",
                             "OnuId            ",
                             "OmccPortId       ",
                             "EqualizationDelay"};
  MV_U8  *enableText[]    = {"disabled","enabled"};
  MV_U32 tempValue1;
  MV_U32 tempValue2;
  MV_U32 status;
  int     off = 0;

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "|  ONU OVERRIDE:                           |\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "|    PARAMETER       |  VALUE   |  STATUS  |\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");

  tempValue1 = onuGponDbExtendedBurstOperOverrideValueGet();
  tempValue2 = onuGponDbExtendedBurstSyncOverrideValueGet();
  status     = (MV_U32)onuGponDbExtendedBurstOverrideGet();
  off += mvOsSPrintf(buf+off, "| %s SYNC | %8u | %s |\n", parametersText[0], tempValue1, enableText[status]);
  off += mvOsSPrintf(buf+off, "| %s OPER | %8u | %s |\n", parametersText[0], tempValue2, enableText[status]);

  tempValue1 = onuGponDbDelimiterOverrideValueGet();
  status     = (MV_U32)onuGponDbDelimiterOverrideGet();
  off += mvOsSPrintf(buf+off, "| %s  | %8u | %s |\n", parametersText[1], tempValue1, enableText[status]);

  tempValue1 = onuGponDbOnuIdOverrideValueGet();
  status     = (MV_U32)onuGponDbOnuIdOverrideGet();
  off += mvOsSPrintf(buf+off, "| %s  | %8u | %s |\n", parametersText[2], tempValue1, enableText[status]);

  tempValue1 = onuGponDbOmccPortOverrideValueGet();
  status     = (MV_U32)onuGponDbOmccPortOverrideGet();
  off += mvOsSPrintf(buf+off, "| %s  | %8u | %s |\n", parametersText[3], tempValue1, enableText[status]);

  tempValue1 = onuGponDbEqualizationDelayOverrideValueGet();
  status     = (MV_U32)onuGponDbEqualizationDelayOverrideGet();
  off += mvOsSPrintf(buf+off, "| %s  | %8u | %s |\n", parametersText[4], tempValue1, enableText[status]);
  off += mvOsSPrintf(buf+off, "--------------------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiDelimiterConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu delimiter
**               
**  PARAMETERS:  MV_U32 enable
**               MV_U32 DelValue 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDelimiterConfig(MV_U32 enable, MV_U32 DelValue)
{
  onuGponApiDelimiterSet((MV_BOOL)enable, DelValue);
}

/*******************************************************************************
**
**  onuGponUiOnuIdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu Id
**               
**  PARAMETERS:  MV_U32 onuId
**               MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiOnuIdConfig(MV_U32 onuId, MV_U32 enable)
{
  onuGponApiOnuIdSet((MV_BOOL)enable, onuId);
}

/*******************************************************************************
**
**  onuGponUiPortIdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu omcc port Id
**               
**  PARAMETERS:  MV_U32 portId
**               MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiPortIdConfig(MV_U32 portId, MV_U32 enable)
{                                         
  onuGponApiOmccPortIdSet((MV_BOOL)enable, portId);
}

/*******************************************************************************
**
**  onuGponUiEquDelaySetConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu equalization delay
**               
**  PARAMETERS:  MV_U32 equDelayVal
**               MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiEquDelaySetConfig(MV_U32 equDelayVal, MV_U32 enable)
{
  onuGponApiEqualizationDelaySet((MV_BOOL)enable, equDelayVal);
}

/*******************************************************************************
**
**  onuGponUiEquDelayChangeConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu equalization delay
**               
**  PARAMETERS:  MV_U32 direction
**               MV_U32 size 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiEquDelayChangeConfig(MV_U32 direction, MV_U32 size)
{
  onuGponApiEqualizationDelayChange(direction, size);
}

/*******************************************************************************
**
**  onuGponUiExtendedBurstConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu extended burst
**               
**  PARAMETERS:  MV_U32 enable
**               MV_U32 rangeValue
**               MV_U32 operValue 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiExtendedBurstConfig(MV_U32 enable, MV_U32 rangeValue, MV_U32 operValue)
{
  onuGponApiExtendedBurstSet ((MV_BOOL)enable, rangeValue, operValue);
}

/*******************************************************************************
**
**  onuGponUiDebugOverheadMode
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu overhead mode
**               
**  PARAMETERS:  MV_U32 overMode 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugOverheadMode(MV_U32 overMode)
{
  MV_BOOL mode = (overMode == 0)? MV_FALSE : MV_TRUE;
  onuGponPonMngOverheadManualModeSet(mode);
}

/*******************************************************************************
**
**  onuGponUiDebugOverheadGuard
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu overhead guard bits
**               
**  PARAMETERS:  MV_U32 guard 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugOverheadGuard(MV_U32 guard)
{
  onuGponDbGuardBitsSet(guard);
}

/*******************************************************************************
**
**  onuGponUiDebugOverheadPreamble
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu overhead preamble
**               
**  PARAMETERS:  MV_U32 pream3RangeCnt
**               MV_U32 pream3OperCnt
**               MV_U32 pream3PattCnt 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugOverheadPreamble(MV_U32 pream3RangeCnt, 
                                    MV_U32 pream3OperCnt, 
                                    MV_U32 pream3PattCnt)
{
  MV_U32 onuState;

  onuGponDbPreambleSet(ONU_GPON_PREM_TYPE_03, pream3PattCnt, 0);
  onuGponDbExtPreambleSyncSet(pream3RangeCnt);
  onuGponDbExtPreambleOperSet(pream3OperCnt);
  onuState = onuGponDbOnuStateGet();
  if (onuState == ONU_GPON_05_OPERATION) mvOnuGponMacPreambleType3Set(pream3PattCnt,pream3OperCnt);
  else                                   mvOnuGponMacPreambleType3Set(pream3PattCnt,pream3RangeCnt);
}

/*******************************************************************************
**
**  onuGponUiIntropHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print introp help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiIntropHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[DEC]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [OnuId] [Enable]                   > onuIdCfg     - override configured OnuId\n");
  off += mvOsSPrintf(buf+off, " echo [PortId] [Enable]                  > portIdCfg    - override configured omcc portId\n");
  off += mvOsSPrintf(buf+off, " echo [EquDelay] [Enable]                > eqdCfg       - override configured Equ Delay\n");
  off += mvOsSPrintf(buf+off, " echo [Direction] [Size]                 > eqdChange    - change Equ Delay\n");
  off += mvOsSPrintf(buf+off, " echo [Enable] [RangeValue] [OperValue]  > extBurstCfg  - override configured external burst\n");
  off += mvOsSPrintf(buf+off, " echo [Enable] [Value]                   > delimiterCfg - override configured delimiter\n");
  off += mvOsSPrintf(buf+off, " echo [RangeValue] [OperValue] [Pattern] > preambleCfg  - override configured preamble type3\n");
  off += mvOsSPrintf(buf+off, " echo [Guard Bits]                       > guardCfg     - configure guard bits\n");
  off += mvOsSPrintf(buf+off, " echo [Mode]                             > manModeCfg   - configure override parameters mode\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat manMode                              - override parameters mode\n");          
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/******************************************************************************/
/* ========================================================================== */
/*                            Protocol Commands                               */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponUiDebugBurstConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu tx burst 
**               
**  PARAMETERS:  MV_U32 start 
**               MV_U32 stop
**               MV_U32 order
**               MV_U32 polarity
**               MV_U32 mask
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugBurstConfig(MV_U32 start, 
                               MV_U32 stop, 
                               MV_U32 order, 
                               MV_U32 polarity, 
                               MV_U32 mask)     
{
  S_apiBurstConfig burstConfig;

  burstConfig.start    = start;
  burstConfig.stop     = stop;
  burstConfig.order    = order;
  burstConfig.polarity = polarity;
  burstConfig.mask     = mask;

  onuGponApiBurstConfigSet(&burstConfig);
}

/*******************************************************************************
**
**  onuGponUiDebugBurstInfo
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu tx burst info
**               
**  PARAMETERS:  char* buf
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiDebugBurstInfo(char* buf)
{
  S_apiBurstConfig burstConfig;
  int              off = 0;

  onuGponApiBurstConfigGet(&burstConfig);

  off += mvOsSPrintf(buf+off, "----------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| Burst Parameters                           |\n");
  off += mvOsSPrintf(buf+off, "|--------------------------------------------|\n");
  off += mvOsSPrintf(buf+off, "|  Start |  Stop  | Order  |Polarity| Mask   |\n");
  off += mvOsSPrintf(buf+off, "|--------------------------------------------|\n");
  off += mvOsSPrintf(buf+off, "|   %02d   |   %02d   |   %02d   |   %02d   |   %02d   |\n",
                     burstConfig.start, burstConfig.stop, burstConfig.order, 
                     burstConfig.polarity, burstConfig.mask);
  off += mvOsSPrintf(buf+off, "|--------------------------------------------|\n");

  return(off); 
}

/*******************************************************************************
**
**  onuGponUiInfoBerDebug
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function get Global OLT BER Information
**               
**  PARAMETERS:  char* buf
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiInfoBerDebug(char* buf)
{
  S_BerCoefficient coeff;
  MV_U32           seconds;
  MV_U32           ber;
  MV_U32           intervalBer;
  MV_U32           bip,lastBip;
  MV_U32           sdThreshold;
  MV_U32           sfThreshold;
  MV_U32           sdDetectValue;
  MV_U32           sdCancelValue;
  MV_U32           sfDetectValue;
  MV_U32           sfCancelValue;
  int              off = 0;

  coeff         = onuGponBerCoefficientGet();
  seconds       = onuGponBerIntervalSecondsGet();
  sdThreshold   = onuGponDbSdThresholdGet();
  sfThreshold   = onuGponDbSfThresholdGet();

  sdDetectValue = onuGponBerPower(10 ,(11 - sdThreshold));
  sdCancelValue = onuGponBerPower(10 ,(11 - sdThreshold - 1));
  sfDetectValue = onuGponBerPower(10 ,(11 - sfThreshold));
  sfCancelValue = onuGponBerPower(10 ,(11 - sfThreshold - 1));

  ber           = onuGponBerBerValueGet();
  intervalBer   = onuGponBerIntervalBerValueGet();
  bip           = onuGponBerBip8CounterGet();
  lastBip       = onuGponBerLastIntervalBip8CounterGet();

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "----------------------------------------\n");
  off += mvOsSPrintf(buf+off, "| BER Information                      |\n");
  off += mvOsSPrintf(buf+off, "|--------------------------------------|\n");
  off += mvOsSPrintf(buf+off, "|Interval of %2d Seconds                |\n", seconds);
  off += mvOsSPrintf(buf+off, "|Coefficient is %2d/%2d                  |\n", coeff.numerator, coeff.denominator);
  off += mvOsSPrintf(buf+off, "-----------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "|Interval BER|   BER      | BIP8       | Inter.BIP8 |\n");
  off += mvOsSPrintf(buf+off, "-----------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "   %010u | %010u | %010u | %010u |\n", intervalBer, ber ,bip, lastBip);
  off += mvOsSPrintf(buf+off, "------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, " | DET SD TH  | CNC SD TH  | DET SF TH  | CNC SF TH  |\n");
  off += mvOsSPrintf(buf+off, "------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "   %010u | %010u | %010u | %010u |\n", sdDetectValue, sdCancelValue, sfDetectValue, sfCancelValue);
  off += mvOsSPrintf(buf+off, "------------------------------------------------------\n");

  return(off); 
}

/*******************************************************************************
**
**  onuGponUiDebugBerIntervalDebug
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu ber interval 
**               
**  PARAMETERS:  MV_U32 interval
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugBerIntervalDebug(MV_U32 interval)
{
  onuGponApiBerIntervalConfig(interval);
}

/*******************************************************************************
**
**  onuGponUiDebugBerCoefficientDebug
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu ber coefficient
**               
**  PARAMETERS:  MV_U32 numerator
**               MV_U32 denominator
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugBerCoefficientDebug(MV_U32 numerator, MV_U32 denominator)
{
  onuGponApiBerCoefficientConfig(denominator,numerator);
}

/*******************************************************************************
**
**  onuGponUiBerThresholdsConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu ber thresholds
**               
**  PARAMETERS:  MV_U32 sd
**               MV_U32 sf 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiBerThresholdsConfig(MV_U32 sd, MV_U32 sf)
{
  onuGponApiBerThresholdConfig(sd, sf);
}

/*******************************************************************************
**
**  onuGponUiDebugAesSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu AES port configuration
**               
**  PARAMETERS:  MV_U32 portId
**               MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugAesSet(MV_U32 portId, MV_U32 enable)
{
  MV_STATUS rcode = MV_OK;
  MV_BOOL   enaBool;
  MV_BOOL   valid;

  /* to start AES on a port Id, it should be valid */
  if (enable == 1)
  {
    valid = onuGponDbGemPortValidGet(portId);
    if (valid != MV_TRUE)
    {
      mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
                 "Set GEM Port AES: Port(%d) is not Valid\n", portId);   
      return;
    }
  }

  enaBool = (enable == 0) ? MV_FALSE : MV_TRUE;

  rcode = mvOnuGponMacAesPortIdSet(portId, enaBool);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "Set GEM Port AES: Port(%d) enable(%d) failed\n", portId, enable);   
  }

  onuGponDbGemPortAesSet(portId, enaBool);
}

/*******************************************************************************
**
**  onuGponUiDebugAesSetAll
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu AES port configuration
**               
**  PARAMETERS:  MV_U32 enable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugAesSetAll(MV_U32 enable)
{
  MV_STATUS  rcode = MV_OK;
  MV_BOOL    enaBool;
  MV_BOOL    valid;
  MV_U32     portId;

  for (portId = 0; portId < 4096; portId++)
  {
    /* to start AES on a port Id, it should be valid */
    if (enable == 1)
    {
      valid = onuGponDbGemPortValidGet(portId);
      if (valid != MV_TRUE)
      {
        mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
                   "Set GEM Port AES: Port(%d) is not Valid\n", portId);   
        return;
      }
    }
  
    enaBool = (enable == 0) ? MV_FALSE : MV_TRUE;
  
    rcode = mvOnuGponMacAesPortIdSet(portId, enaBool);
    if (rcode != MV_OK)
    {
      mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
                 "Set GEM Port AES: Port(%d) enable(%d) failed\n", portId, enable);   
    }

    onuGponDbGemPortAesSet(portId,enaBool);
  }
}

/*******************************************************************************
**
**  onuGponUiDebugAesGemAll
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return a list of all gem port AES and status
**               
**  PARAMETERS:  char* buf
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiDebugAesGemAll(char* buf, int range)
{
  MV_U32  portId;
  MV_BOOL enable;
  MV_U32  startRange;
  MV_U32  endRange;
  int     off = 0;

  off += mvOsSPrintf(buf+off, "Encrypted GEM port ids list\n");
  off += mvOsSPrintf(buf+off, "===========================\n");   

  switch (range)
  {
    case 1:  startRange = 1000; endRange = 1999; break;
    case 2:  startRange = 2000; endRange = 2999; break;
    case 3:  startRange = 3000; endRange = 4096; break;
    default: startRange = 0;    endRange = 999;  break;
  }

  for (portId = startRange ; portId < endRange ; portId++)
  {
    enable = onuGponDbGemPortAesGet(portId);
    if (enable == MV_TRUE) off += mvOsSPrintf(buf+off, "[%04d]", portId); 
    if ((portId / 14) == 0) off += mvOsSPrintf(buf+off, "\n"); 
  }

  return(off);
}

/*******************************************************************************
**
**  onuGponUiDebugManagerSendPloam
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function transmit US ploam message
**               
**  PARAMETERS:  MV_U32 msgId
**               MV_U32 onuId
**               MV_U32 data1 
**               MV_U32 data2
**               MV_U32 data3 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugManagerSendPloam(MV_U32 msgId, 
                                    MV_U32 onuId, 
                                    MV_U32 data1, 
                                    MV_U32 data2, 
                                    MV_U32 data3)
{
  MV_U8     msgData[12];
  MV_STATUS rcode;

  msgData[0]  = (MV_U8)((data1 >> 24) & 0xFF);
  msgData[1]  = (MV_U8)((data1 >> 16) & 0xFF);
  msgData[2]  = (MV_U8)((data1 >> 8) & 0xFF);
  msgData[3]  = (MV_U8)((data1) & 0xFF);
  msgData[4]  = (MV_U8)((data2 >> 24) & 0xFF);
  msgData[5]  = (MV_U8)((data2 >> 16) & 0xFF);
  msgData[6]  = (MV_U8)((data2 >> 8) & 0xFF);
  msgData[7]  = (MV_U8)((data2) & 0xFF);
  msgData[8]  = (MV_U8)((data3 >> 24) & 0xFF);
  msgData[9]  = (MV_U8)((data3 >> 16) & 0xFF);
  msgData[10] = (MV_U8)((data3 >> 8) & 0xFF);
  msgData[11] = (MV_U8)((data3) & 0xFF);

  rcode = mvOnuGponMacMessageSend(onuId, msgId, msgData);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "mvOnuGponMacMessageSend failed\n"); 
  }
}

/*******************************************************************************
**
**  onuGponUiDebugManagerReceivePloam
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function receive DS ploam message
**               
**  PARAMETERS:  MV_U32 msgId
**               MV_U32 onuId
**               MV_U32 data1 
**               MV_U32 data2
**               MV_U32 data3 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugManagerReceivePloam(MV_U32 msgId, 
                                       MV_U32 onuId, 
                                       MV_U32 data1, 
                                       MV_U32 data2, 
                                       MV_U32 data3)
{
  MV_U8 msgData[12];

  msgData[0]  = (MV_U8)((data1 >> 24) & 0xFF);
  msgData[1]  = (MV_U8)((data1 >> 16) & 0xFF);
  msgData[2]  = (MV_U8)((data1 >> 8) & 0xFF);
  msgData[3]  = (MV_U8)((data1) & 0xFF);
  msgData[4]  = (MV_U8)((data2 >> 24) & 0xFF);
  msgData[5]  = (MV_U8)((data2 >> 16) & 0xFF);
  msgData[6]  = (MV_U8)((data2 >> 8) & 0xFF);
  msgData[7]  = (MV_U8)((data2) & 0xFF);
  msgData[8]  = (MV_U8)((data3 >> 24) & 0xFF);
  msgData[9]  = (MV_U8)((data3 >> 16) & 0xFF);
  msgData[10] = (MV_U8)((data3 >> 8) & 0xFF);
  msgData[11] = (MV_U8)((data3) & 0xFF);

  onuGponPonMngPloamProcess((MV_U8)onuId, (MV_U8)msgId, msgData); 
}

/*******************************************************************************
**
**  onuGponUiGemOmciPortIdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu omci port Id
**               
**  PARAMETERS:  MV_U32 portId
**               MV_U32 valid 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiGemOmciPortIdConfig(MV_U32 portId, MV_U32 valid)
{
  mvOnuGponMacGemOmciPortIdSet(portId, valid); /* Rx */
  mvOnuGponMacUtmOmciPortIdSet(portId, valid); /* Tx */

  onuGponDbOmccPortSet(portId);
  onuGponDbOmccValidSet(valid);
}

/*******************************************************************************
**
**  onuGponUiGemOmciPortIdShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu omcc port Id
**               
**  PARAMETERS:  char* buf
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiGemOmciPortIdShow(char* buf)
{
  MV_STATUS rcode;
  MV_U32    gemPortId;                          
  MV_U32    gemPortValid;                          
  MV_U32    utmPortId;                          
  MV_U32    utmPortValid;  
  int       off = 0;

  rcode = mvOnuGponMacGemOmciPortIdGet(&gemPortId, &gemPortValid);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get Rx(gem) omci portId, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  rcode = mvOnuGponMacUtmOmciPortIdGet(&utmPortId, &utmPortValid);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get Tx(Utm) omci portId, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "OMCI Rx(Gem) PortId %8d valid %s\n", gemPortId, (gemPortValid == 0) ? ("FALSE") : ("TRUE"));                          
  off += mvOsSPrintf(buf+off, "OMCI Tx(Utm) PortId %8d valid %s\n", utmPortId, (utmPortValid == 0) ? ("FALSE") : ("TRUE"));

  return(off);
}

/*******************************************************************************
**
**  onuGponUiGemFrameLengthShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu frame lengths
**               
**  PARAMETERS:  char* buf
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiGemFrameLengthShow(char* buf)
{
  MV_STATUS rcode;
  MV_U32    payloadlen;                          
  MV_U32    maxEthFrameLen;
  MV_U32    minEthFrameLen;   
  MV_U32    maxOmciFrameLen;
  MV_U32    minOmciFrameLen;  
  int       off = 0;

  rcode = mvOnuGponMacGemPayloadLenGet(&payloadlen);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get gem payload length value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  rcode = mvOnuGponMacGemEthFrameLenGet(&maxEthFrameLen, &minEthFrameLen);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get ethernet frame length value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  rcode = mvOnuGponMacGemOmciFrameLenGet(&maxOmciFrameLen, &minOmciFrameLen);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to get omci frame value, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "----------------------\n");
  off += mvOsSPrintf(buf+off, "|Frame Length        |\n");
  off += mvOsSPrintf(buf+off, "-----------------------------------\n");
  off += mvOsSPrintf(buf+off, "| Frame              |   VALUE    |\n");
  off += mvOsSPrintf(buf+off, "-----------------------------------\n");
  off += mvOsSPrintf(buf+off, "Gem payload          | %10d |\n", payloadlen);                         
  off += mvOsSPrintf(buf+off, "Gem min eth frame    | %10d |\n", minEthFrameLen);                         
  off += mvOsSPrintf(buf+off, "Gem max eth frame    | %10d |\n", maxEthFrameLen);                         
  off += mvOsSPrintf(buf+off, "Gem min omci frame   | %10d |\n", minOmciFrameLen);                         
  off += mvOsSPrintf(buf+off, "Gem max omci frame   | %10d |\n", maxOmciFrameLen);                         
  off += mvOsSPrintf(buf+off, "-----------------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuGponUiGemPayloadLenConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu gem payload length
**               
**  PARAMETERS:  MV_U32 payloadLen 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiGemPayloadLenConfig(MV_U32 payloadLen)
{
  mvOnuGponMacGemPayloadLenSet(payloadLen);
}

/*******************************************************************************
**
**  onuGponUiGemEthFrameLenConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu ethernet min/max frame length
**               
**  PARAMETERS:  MV_U32 minFrameLenArg
**               MV_U32 maxFrameLenArg 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiGemEthFrameLenConfig(MV_U32 minFrameLenArg, MV_U32 maxFrameLenArg)
{
  mvOnuGponMacGemEthFrameLenSet(maxFrameLenArg, minFrameLenArg);
}

/*******************************************************************************
**
**  onuGponUiGemOmciFrameLenConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu omci min/max frame length
**               
**  PARAMETERS:  MV_U32 minFrameLenArg
**               MV_U32 maxFrameLenArg 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiGemOmciFrameLenConfig(MV_U32 minFrameLenArg, MV_U32 maxFrameLenArg)
{
  mvOnuGponMacGemOmciFrameLenSet(maxFrameLenArg, minFrameLenArg);
}

/*******************************************************************************
**
**  onuGponUiProtoHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print protocol help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiProtoHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[DEC]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [BER Interval]            > berIntervalCfg  - config ber interval\n");
  off += mvOsSPrintf(buf+off, " echo [Numerator] [Denominator] > berCoeffCfg     - config ber coeff numerator, denominator\n");
  off += mvOsSPrintf(buf+off, " echo [SD] [SF]                 > berThesholdCfg  - config ber SD, SF thresholds\n");
  off += mvOsSPrintf(buf+off, " echo [OmccPortId] [State]      > omciPortCfg     - config omcc gem port state\n");
  off += mvOsSPrintf(buf+off, " echo [Len]                     > gemFrameLenCfg  - config gem frame payload len\n");
  off += mvOsSPrintf(buf+off, " echo [MinLen] [MaxLen]         > ethFrameLenCfg  - config eth frame min & max lengths\n");
  off += mvOsSPrintf(buf+off, " echo [MinLen] [MaxLen]         > omciFrameLenCfg - config omci frame min & max lengths\n");
  off += mvOsSPrintf(buf+off, " echo [Start] [Stop] [Order] [Pola] [Mask] > txBurstCfg   - config tx burst\n");
  off += mvOsSPrintf(buf+off, " echo [MsgId] [OnuId] [Data] [Data] [Data] > ploamMsgSend - simulate transmit of US PLOAM msg\n");
  off += mvOsSPrintf(buf+off, " echo [MsgId] [OnuId] [Data] [Data] [Data] > ploamMsgRece - simulate reception and process of DS PLOAM msg\n"); 
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat txBurst                              - dump tx burst info\n");          
  off += mvOsSPrintf(buf+off, " cat gemFrameLen                          - dump frame lengths\n");          
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/******************************************************************************/
/* ========================================================================== */
/*                            Misc Commands                                   */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuGponUiDebugSerialNumberSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set ONU serial Number
**              
**  PARAMETERS:  ON_U32 serialNumber1
**               MV_U32 serialNumber2 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugSerialNumberSet(MV_U32 serialNumber1, MV_U32 serialNumber2)
{
  MV_STATUS rcode;
  MV_U8     serialNumber[8];

  serialNumber[0] = (MV_U8)((serialNumber1 >> 24) & 0xFF);
  serialNumber[1] = (MV_U8)((serialNumber1 >> 16) & 0xFF);
  serialNumber[2] = (MV_U8)((serialNumber1 >> 8) & 0xFF);
  serialNumber[3] = (MV_U8)((serialNumber1) & 0xFF);
  serialNumber[4] = (MV_U8)((serialNumber2 >> 24) & 0xFF);
  serialNumber[5] = (MV_U8)((serialNumber2 >> 16) & 0xFF);
  serialNumber[6] = (MV_U8)((serialNumber2 >> 8) & 0xFF); 
  serialNumber[7] = (MV_U8)((serialNumber2) & 0xFF);      

  rcode = onuGponSrvcSerialNumberSet(serialNumber);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to set Serial Number, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }
}

/*******************************************************************************
**
**  onuGponUiDebugSerialNumberSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set ONU serial Number
**              
**  PARAMETERS:  ON_U32 serialNumber1
**               MV_U32 serialNumber2 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugPasswordSet(MV_U32 passwordNumber1, 
                               MV_U32 passwordNumber2, 
                               MV_U32 passwordNumber3)
{
  MV_STATUS rcode;
  MV_U8     password[10];

  password[0] = (MV_U8)((passwordNumber1 >> 24) & 0xFF);
  password[1] = (MV_U8)((passwordNumber1 >> 16) & 0xFF);
  password[2] = (MV_U8)((passwordNumber1 >> 8) & 0xFF);
  password[3] = (MV_U8)((passwordNumber1) & 0xFF);
  password[4] = (MV_U8)((passwordNumber2 >> 24) & 0xFF);
  password[5] = (MV_U8)((passwordNumber2 >> 16) & 0xFF);
  password[6] = (MV_U8)((passwordNumber2 >> 8) & 0xFF); 
  password[7] = (MV_U8)((passwordNumber2) & 0xFF);      
  password[8] = (MV_U8)((passwordNumber3 >> 24) & 0xFF);
  password[9] = (MV_U8)((passwordNumber3 >> 16) & 0xFF);

  rcode = onuGponDbPasswordSet(password);
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "%s:%d, failed to set password, rcode(%d)\n", __FILE_DESC__, __LINE__, rcode);
  }
}

/*******************************************************************************
**
**  onuGponUiTxEnThresholdConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu tx enable threshold
**               
**  PARAMETERS:  MV_U32 threshold 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiTxEnThresholdConfig(MV_U32 threshold)
{
  mvOnuGponMacTxTxEnableCounterThresholdSet(threshold);
}

/*******************************************************************************
**
**  onuGponUiUtmTcPeriodConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu utm tc period
**                 
**  PARAMETERS:  MV_U32 period 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiUtmTcPeriodConfig(MV_U32 period)
{
  mvOnuGponMacUtmTcPeriodSet(period); 
}

/*******************************************************************************
**
**  onuGponUiUtmTcValidConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu utm tc valid
**                 
**  PARAMETERS:  MV_U32 valid 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiUtmTcValidConfig(MV_U32 valid)
{
  mvOnuGponMacUtmTcValidSet(valid); 
}

/*******************************************************************************
**
**  onuGponUitxDbrBlockSizeConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function change onu dbr block size
**                 
**  PARAMETERS:  MV_U32 blockSize 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUitxDbrBlockSizeConfig(MV_U32 blockSize)
{
  MV_U32 configuredBlockSize;

  onuGponSrvcDbrBlockSizeSet(blockSize, &configuredBlockSize); 
}

/*******************************************************************************
**
**  onuGponUiDebugEventCleanTimerStateSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handle onu event clean up
**               
**  PARAMETERS:  MV_U32 eventCleanTimer 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugEventCleanTimerStateSet(MV_U32 eventCleanTimer)
{
  MV_BOOL mode;

  mode = (eventCleanTimer == 0)? MV_FALSE : MV_TRUE;
  if (onuGponIsrEventCleanTimerStateSet(mode) != MV_OK)
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
               "Event CleanUp Timer Enable Failed\n");
}

/*******************************************************************************
**
**  onuGponUiDebugMemReadReg
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function test requested address space
**               
**  PARAMETERS:  MV_U32 readOffset
**               MV_U32 readEntry 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugReadReg(MV_U32 readOffset, MV_U32 readEntry)
{
  MV_U32 regValue;

  regValue = MV_REG_READ((MV_GPON_MAC_REGS_BASE) + readOffset + (readEntry * sizeof (MV_U32))); 

  mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
             "Read register offset 0x%x, entry %d = [0x%08X]\n\r", readOffset, readEntry, regValue);
}

/*******************************************************************************
**
**  onuGponUiDebugWriteReg
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function test requested address space
**               
**  PARAMETERS:  MV_U32 writeOffset
**               MV_U32 writeEntry
**               MV_U32 regValue 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponUiDebugWriteReg(MV_U32 writeOffset, MV_U32 writeEntry, MV_U32 regValue)
{
  MV_REG_WRITE((MV_GPON_MAC_REGS_BASE) + writeOffset + (writeEntry * sizeof (MV_U32)), regValue); 

  mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
             "Write  register offset 0x%x, entry %d = [0x%08X]\n\r", writeOffset, writeEntry, regValue);
}

/*******************************************************************************
**
**  onuGponUiDebugDebugModeSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu denug mode
**               
**  PARAMETERS:  MV_U32 debugMode 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponUiDebugDebugModeSet(MV_U32 debugMode)
{
  MV_BOOL mode;

  mode = (debugMode == 0)? MV_FALSE : MV_TRUE;
  onuGponPonMngDebugModeSet(mode);
}

#ifdef MV_GPON_PERFORMANCE_CHECK

/*******************************************************************************
**
**  onuGponCliDebugPerformanceCheck
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function 
**               
**  PARAMETERS:  MV_U32 step 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponCliDebugPerformanceCheck(MV_U32 step)
{
  MV_U32 minCheck   = 0;
  MV_U32 maxCheck   = 0;
  MV_U32 aveCheck   = 0;
  MV_U32 aveCount   = 0;
  MV_U32 tempCheck  = 0;
  MV_U32 minMaxTemp = 0;
  MV_U32 index;

  S_GponPerformanceCheckNode *tmpPmCheckNode;

  char* pmStepDesc[] = 
  {
    "ISR        ",  
    "MNG1       ",  
    "MNG2       ",  
    "MNG3       ",  
    "ExtB       ",  
    "Key Switch ",  
    "Cfg Port   ",  
    "Req Key    ",  
    "AllocId    ",  
    "Password   ",  
    "Enc Port   ",  
    "Dis        ",  
    "Dact       ",  
    "Rng        ",  
    "OnuId      ",  
    "Over       "   
  };                

  if (step >= PON_MAX_PERFORMANCE) 
  {
    mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE, 
               "Invalid performance step(%d)\n", step);  
    return;
  }

  tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[step]);

  mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE, 
             "             Interval    Minimum     Maximum     Average\n");   

  aveCount = 0;
  for (index = 0; index < tmpPmCheckNode->uSecCntIdx; index++)
  {
    tempCheck = tmpPmCheckNode->uSecCntStop[index] - 
                tmpPmCheckNode->uSecCntStart[index];
  
    if (tempCheck < 1000) 
    {
      aveCheck += tempCheck;
      aveCount++;

      if (minMaxTemp == 0)
      {
        minCheck = maxCheck = tempCheck;
        minMaxTemp = 1;
      }
      else
      {
        if      (tempCheck > maxCheck) maxCheck = tempCheck;
        else if (tempCheck < minCheck) minCheck = tempCheck;
      }
    }
  }
  
  if (aveCount > 0) aveCheck = aveCheck / aveCount;

  mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE, 
             "%11s  %8u    %7lu     %7lu     %7lu\n",
             pmStepDesc[step], tmpPmCheckNode->uSecCntIdx, minCheck, maxCheck, aveCheck);  
  
  if (tmpPmCheckNode->uSecCntIdx > 0) 
  {
    for (index = 0; index < tmpPmCheckNode->uSecCntIdx - 1; index++)
    {
      mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE, 
                 "%11s   Start %u Stop %u diff %u\n",
                 pmStepDesc[step],
                 tmpPmCheckNode->uSecCntStart[index], 
                 tmpPmCheckNode->uSecCntStop[index],
                 (tmpPmCheckNode->uSecCntStop[index] - tmpPmCheckNode->uSecCntStart[index]));  
    }
  }
}

/*******************************************************************************
**
**  onuGponCliDebugPerformanceClear
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function 
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
void onuGponCliDebugPerformanceClear(void)
{
  MV_U32 step;
  MV_U32 index;
  S_GponPerformanceCheckNode *tmpPmCheckNode;

  for (step = 0; step < PON_MAX_PERFORMANCE; step++) 
  {
    tmpPmCheckNode = &(g_GponPmCheck.pmCheckNode[step]);

    for (index = 0; index < tmpPmCheckNode->uSecCntIdx; index++)
    {
      tmpPmCheckNode->uSecCntStop[index]  = 0; 
      tmpPmCheckNode->uSecCntStart[index] = 0;
    }

    tmpPmCheckNode->uSecCntIdx = 0;
  }
}

#endif /* MV_GPON_PERFORMANCE_CHECK */

/*******************************************************************************
**
**  onuGponUiMiscHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print misc help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuGponUiMiscHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[HEX]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [SN1] [SN2]                > serialNumCfg    - config serial number\n");
  off += mvOsSPrintf(buf+off, " echo [TxEnThreshold]            > txEnThresCfg    - config tx enable threshold\n");
  off += mvOsSPrintf(buf+off, " echo [TC Period]                > utmTxPeriodCfg  - config utm tc period\n");
  off += mvOsSPrintf(buf+off, " echo [TC Valid]                 > utmTcValidCfg   - config utm tc valid\n");
  off += mvOsSPrintf(buf+off, " echo [DBR Block Size]           > txDbrBlkSizeCfg - config dbr block size\n");
  off += mvOsSPrintf(buf+off, " echo [ISR CleanUp timer]        > cleanUpTimerCfg - config isr cleanup timer\n");
  off += mvOsSPrintf(buf+off, " echo [Offset] [Entry]           > readReg         - read onu reg [Id from mvPonOnuRegs.h]\n");
  off += mvOsSPrintf(buf+off, " echo [Offset] [Entry] [Value]   > writeReg        - write onu reg [Id from mvPonOnuRegs.h]\n");         
  off += mvOsSPrintf(buf+off, " echo [Mode]                     > debugModeCfg    - config debug mode\n");         
  off += mvOsSPrintf(buf+off, " echo [Section]                  > pmCheck         - performance check section\n");         
  off += mvOsSPrintf(buf+off, " echo [0]                        > pmClear         - performance check reset\n");         
  off += mvOsSPrintf(buf+off, " echo [module] [Level] [Options] > printMask       - change printing options\n");         
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat printMask                                     - dump printing options\n");          
  return(off);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/*                            ONU SYS FS Definition                           */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* ========================================================================== */
/*                            Misc Commands                                   */
/* ========================================================================== */
static ssize_t misc_show(struct device *dev, 
                         struct device_attribute *attr, 
                         char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if      (!strcmp(name, "printMask")) 
  	return ponOnuPrintStatus(buf);
  else if (!strcmp(name, "helpMisc")) 
  	return onuGponUiMiscHelpShow(buf);

  return 0;
}

static ssize_t misc_store(struct device *dev, 
                          struct device_attribute *attr, 
                          const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param1 = 0;
  unsigned int  param2 = 0;
  unsigned int  param3 = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%x %x %x", &param1, &param2, &param3);

  raw_local_irq_save(flags);

  if      (!strcmp(name, "serialNumCfg")) 
  	onuGponUiDebugSerialNumberSet((MV_U32)param1, (MV_U32)param2);             /* serialNumber1, serialNumber2 */
  else if (!strcmp(name, "passwordCfg")) 
  	onuGponUiDebugPasswordSet((MV_U32)param1, (MV_U32)param2, (MV_U32)param3); /* serialNumber1, serialNumber2 */
  else if (!strcmp(name, "txEnThresCfg"))                            
    onuGponUiTxEnThresholdConfig((MV_U32)param1);                              /* threshold */
  else if (!strcmp(name, "utmTxPeriodCfg"))                                       
    onuGponUiUtmTcPeriodConfig((MV_U32)param1);                                /* period */                              
  else if (!strcmp(name, "utmTcValidCfg"))                                    
    onuGponUiUtmTcValidConfig((MV_U32)param1);                                 /* valid */
  else if (!strcmp(name, "txDbrBlkSizeCfg"))                                  
    onuGponUitxDbrBlockSizeConfig((MV_U32)param1);                             /* blockSize */
  else if (!strcmp(name, "cleanUpTimerCfg"))                                  
    onuGponUiDebugEventCleanTimerStateSet((MV_U32)param1);                     /* eventCleanTimer */
  else if (!strcmp(name, "readReg"))
    onuGponUiDebugReadReg((MV_U32)param1, (MV_U32)param2);                     /* readOffset, readEntry */
  else if (!strcmp(name, "writeReg"))
    onuGponUiDebugWriteReg((MV_U32)param1, (MV_U32)param2, (MV_U32)param3);    /*  writeOffset, writeEntry, regValue */
  else if (!strcmp(name, "debugModeCfg"))
    onuGponUiDebugDebugModeSet((MV_U32)param1);                                /* debugMode */
#ifdef MV_GPON_PERFORMANCE_CHECK
  else if (!strcmp(name, "pmCheck"))
    onuGponCliDebugPerformanceCheck((MV_U32)param1);                           /* step */
  else if (!strcmp(name, "pmClear"))
    onuGponCliDebugPerformanceClear();
#endif /* MV_GPON_PERFORMANCE_CHECK */
  else if (!strcmp(name, "printMask"))
    ponOnuChangePrintStatus((MV_U32)param1, (MV_U32)param2, (MV_U32)param3);   /* module, print level, options */
  else
 	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            Misc Attributes                                 */
/* ========================================================================== */
static DEVICE_ATTR(serialNumCfg,          S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(passwordCfg,           S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(txEnThresCfg,          S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(utmTxPeriodCfg,        S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(utmTcValidCfg,         S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(txDbrBlkSizeCfg,       S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(cleanUpTimerCfg,       S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(readReg,               S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(writeReg,              S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(debugModeCfg,          S_IWUSR, misc_show, misc_store);
#ifdef MV_GPON_PERFORMANCE_CHECK                 
static DEVICE_ATTR(pmCheck,               S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(pmClear,               S_IWUSR, misc_show, misc_store);
#endif /* MV_GPON_PERFORMANCE_CHECK */
static DEVICE_ATTR(printMask,             S_IRUSR | S_IWUSR, misc_show, misc_store);
static DEVICE_ATTR(helpMisc,              S_IRUSR, misc_show, misc_store);
                                                                    
static struct attribute *misc_attrs[] = {                       
    &dev_attr_serialNumCfg.attr,       
    &dev_attr_passwordCfg.attr,       
	&dev_attr_txEnThresCfg.attr,                                   
	&dev_attr_utmTxPeriodCfg.attr,                              
    &dev_attr_utmTcValidCfg.attr,              
	&dev_attr_txDbrBlkSizeCfg.attr,            
    &dev_attr_cleanUpTimerCfg.attr,         
    &dev_attr_readReg.attr,                    
    &dev_attr_writeReg.attr,                   
    &dev_attr_debugModeCfg.attr,    
#ifdef MV_GPON_PERFORMANCE_CHECK
    &dev_attr_pmCheck.attr,                    
    &dev_attr_pmClear.attr,                    
#endif /* MV_GPON_PERFORMANCE_CHECK */
    &dev_attr_printMask.attr,
    &dev_attr_helpMisc.attr,
	NULL
};

static struct attribute_group misc_group = 
{
  .name = "misc",
  .attrs = misc_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Protocol Group                                 */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Protocol Commands                               */
/* ========================================================================== */
static ssize_t protocol_show(struct device *dev, 
                             struct device_attribute *attr, 
                             char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if      (!strcmp(name, "txBurst")) 	
  	return (onuGponUiDebugBurstInfo(buf));
  else if (!strcmp(name, "gemFrameLen")) 	
  	return (onuGponUiGemFrameLengthShow(buf));
  else if (!strcmp(name, "helpProto")) 	
  	return (onuGponUiProtoHelpShow(buf));
  return 0;
}

static ssize_t protocol_store(struct device *dev, 
                              struct device_attribute *attr, 
                              const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param1 = 0;
  unsigned int  param2 = 0;
  unsigned int  param3 = 0;
  unsigned int  param4 = 0;
  unsigned int  param5 = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%d %d %d %d %d", &param1, &param2, &param3, &param4, &param5);

  raw_local_irq_save(flags);

  if      (!strcmp(name, "txBurstCfg")) 
  	onuGponUiDebugBurstConfig((MV_U32)param1, (MV_U32)param2, (MV_U32)param3,
                              (MV_U32)param4, (MV_U32)param5);            /* start, stop, order, polarity, mask */
  else if (!strcmp(name, "berIntervalCfg"))                              
    onuGponUiDebugBerIntervalDebug((MV_U32)param1);                       /* interval */
  else if (!strcmp(name, "berCoeffCfg"))                                  
    onuGponUiDebugBerCoefficientDebug((MV_U32)param1, (MV_U32)param2);    /* numerator, denominator*/                              
  else if (!strcmp(name, "berThesholdCfg"))                              
    onuGponUiBerThresholdsConfig((MV_U32)param1, (MV_U32)param2);         /* sd, sf */
  else if (!strcmp(name, "omciPortCfg"))
    onuGponUiGemOmciPortIdConfig((MV_U32)param1, (MV_U32)param2);         /* portId, valid */
  else if (!strcmp(name, "gemFrameLenCfg"))
    onuGponUiGemPayloadLenConfig((MV_U32)param1);                         /* payloadLen */
  else if (!strcmp(name, "ethFrameLenCfg"))                              
    onuGponUiGemEthFrameLenConfig((MV_U32)param1, (MV_U32)param2);        /* minFrameLenArg, maxFrameLenArg */
  else if (!strcmp(name, "omciFrameLenCfg"))                             
    onuGponUiGemOmciFrameLenConfig((MV_U32)param1, (MV_U32)param2);       /* minFrameLenArg, maxFrameLenArg */
  else if (!strcmp(name, "ploamMsgSend"))
    onuGponUiDebugManagerSendPloam((MV_U32)param1, (MV_U32)param2, (MV_U32)param3,
                                   (MV_U32)param4, (MV_U32)param5);       /* msgId, onuId, data1, data2, data3 */
  else if (!strcmp(name, "ploamMsgRece"))
    onuGponUiDebugManagerReceivePloam((MV_U32)param1, (MV_U32)param2, (MV_U32)param3,
                                      (MV_U32)param4, (MV_U32)param5);    /* msgId, onuId, data1, data2, data3 */
  else
 	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            Protocol Attributes                             */
/* ========================================================================== */
static DEVICE_ATTR(txBurstCfg,       S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(txBurst,          S_IRUSR, protocol_show, protocol_store);
static DEVICE_ATTR(berIntervalCfg,   S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(berCoeffCfg,      S_IRUSR, protocol_show, protocol_store);
static DEVICE_ATTR(berThesholdCfg,   S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(omciPortCfg,      S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(gemFrameLen,      S_IRUSR, protocol_show, protocol_store);
static DEVICE_ATTR(gemFrameLenCfg,   S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(ethFrameLenCfg,   S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(omciFrameLenCfg,  S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(ploamMsgSend,     S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(ploamMsgRece,     S_IWUSR, protocol_show, protocol_store);
static DEVICE_ATTR(helpProto,        S_IRUSR, protocol_show, protocol_store);
                                                                    
static struct attribute *protocol_attrs[] = {                       
    &dev_attr_txBurstCfg.attr,                                      
	&dev_attr_txBurst.attr,                                     
	&dev_attr_berIntervalCfg.attr,                                  
    &dev_attr_berCoeffCfg.attr,
	&dev_attr_berThesholdCfg.attr,
    &dev_attr_omciPortCfg.attr,
    &dev_attr_gemFrameLen.attr,
    &dev_attr_gemFrameLenCfg.attr,
    &dev_attr_ethFrameLenCfg.attr,
    &dev_attr_omciFrameLenCfg.attr,
    &dev_attr_ploamMsgSend.attr,
    &dev_attr_ploamMsgRece.attr,
    &dev_attr_helpProto.attr,
	NULL
};

static struct attribute_group protocol_group = 
{
  .name = "protocol",
  .attrs = protocol_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Introp Group                                    */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Introp Commands                                 */
/* ========================================================================== */
static ssize_t introp_show(struct device *dev, 
                           struct device_attribute *attr, 
                           char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if      (!strcmp(name, "manMode")) 	
  	return (onuGponUiOverrideShow(buf));
  else if (!strcmp(name, "helpIot")) 
    return (onuGponUiIntropHelpShow(buf));

  return 0;
}

static ssize_t introp_store(struct device *dev, 
                            struct device_attribute *attr, 
                            const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param1 = 0;
  unsigned int  param2 = 0;
  unsigned int  param3 = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%d %d %d", &param1, &param2, &param3);

  raw_local_irq_save(flags);

  if      (!strcmp(name, "onuIdCfg")) 
  	onuGponUiOnuIdConfig((MV_U32)param1, (MV_U32)param2);                           /* onuId, enable */
  else if (!strcmp(name, "portIdCfg"))                                              
    onuGponUiPortIdConfig((MV_U32)param1, (MV_U32)param2);                          /* portId, enable */
  else if (!strcmp(name, "eqdCfg"))                                                 
    onuGponUiEquDelaySetConfig((MV_U32)param1, (MV_U32)param2);                     /* equDelayVal, enable*/                              
  else if (!strcmp(name, "eqdChange"))                                              
    onuGponUiEquDelayChangeConfig((MV_U32)param1, (MV_U32)param2);                  /* direction, size */
  else if (!strcmp(name, "extBurstCfg"))                                            
    onuGponUiExtendedBurstConfig((MV_U32)param1, (MV_U32)param2, (MV_U32)param3);   /* enable, rangeValue, operValue */
  else if (!strcmp(name, "delimiterCfg"))
    onuGponUiDelimiterConfig((MV_U32)param1, (MV_U32)param2);                       /* enable , value*/
  else if (!strcmp(name, "preambleCfg"))
    onuGponUiDebugOverheadPreamble((MV_U32)param1, (MV_U32)param2, (MV_U32)param3); /* pream3RangeCnt, pream3OperCnt, pream3PattCnt */
  else if (!strcmp(name, "guardCfg"))
    onuGponUiDebugOverheadGuard((MV_U32)param1);                                    /* guard */
  else if (!strcmp(name, "manModeCfg"))                                            
    onuGponUiDebugOverheadMode((MV_U32)param1);                                     /* overMode */
  else
 	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            Introp Attributes                                 */
/* ========================================================================== */
static DEVICE_ATTR(onuIdCfg,        S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(portIdCfg,       S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(eqdCfg,          S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(eqdChange,       S_IRUSR, introp_show, introp_store);
static DEVICE_ATTR(extBurstCfg,     S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(delimiterCfg,    S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(preambleCfg,     S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(guardCfg,        S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(manModeCfg,      S_IWUSR, introp_show, introp_store);
static DEVICE_ATTR(manMode,         S_IRUSR, introp_show, introp_store);
static DEVICE_ATTR(helpIot,         S_IRUSR, introp_show, introp_store);

static struct attribute *introp_attrs[] = {
    &dev_attr_onuIdCfg.attr,
	&dev_attr_portIdCfg.attr,
	&dev_attr_eqdCfg.attr,
    &dev_attr_eqdChange.attr,
	&dev_attr_extBurstCfg.attr,
    &dev_attr_delimiterCfg.attr,
    &dev_attr_preambleCfg.attr,
    &dev_attr_guardCfg.attr,
    &dev_attr_manModeCfg.attr,
	&dev_attr_manMode.attr,
    &dev_attr_helpIot.attr,
	NULL
};

static struct attribute_group introp_group = 
{
  .name = "introp",
  .attrs = introp_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Data Group                                      */
/* ========================================================================== */
/******************************************************************************/
/* ========================================================================== */
/*                            Data Commands                                   */
/* ========================================================================== */
static ssize_t data_show(struct device *dev, 
                         struct device_attribute *attr, 
                         char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if      (!strcmp(name, "showTcont")) 	
  	return (onuGponUiDebugTcont(buf));
  else if (!strcmp(name, "showGem0xxx")) 
    return (onuGponUiDebugGemPortid(buf, 0));
  else if (!strcmp(name, "showGem1xxx")) 
    return (onuGponUiDebugGemPortid(buf, 1));
  else if (!strcmp(name, "showGem2xxx")) 
    return (onuGponUiDebugGemPortid(buf, 2));
  else if (!strcmp(name, "showGem3xxx")) 
    return (onuGponUiDebugGemPortid(buf, 3));
  else if (!strcmp(name, "showAes0xxx")) 	
  	return (onuGponUiDebugAesGemAll(buf, 0));
  else if (!strcmp(name, "showAes1xxx")) 	
  	return (onuGponUiDebugAesGemAll(buf, 1));
  else if (!strcmp(name, "showAes2xxx")) 	
  	return (onuGponUiDebugAesGemAll(buf, 2));
  else if (!strcmp(name, "showAes3xxx")) 	
  	return (onuGponUiDebugAesGemAll(buf, 3));
  else if (!strcmp(name, "helpData")) 
    return (onuGponUiDataHelpShow(buf));

  return 0;
}

static ssize_t data_store(struct device *dev, 
                          struct device_attribute *attr, 
                          const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param1 = 0;
  unsigned int  param2 = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%d %d", &param1, &param2);

  raw_local_irq_save(flags);

  if      (!strcmp(name, "cfgTcont")) 
  	onuGponUiTcontConfig((MV_U32)param1, (MV_U32)param2);             /* alloc, tcont */
  else if (!strcmp(name, "clrTcont"))
    onuGponUiClearTcontConfig((MV_U32)param1);                        /* tcont */
  else if (!strcmp(name, "rstTcont"))                                
    onuGponUiResetTcontsConfig();                                    
  else if (!strcmp(name, "cfgGem"))
    onuGponUiDebugGemPortidValidSet((MV_U32)param1, (MV_U32)param2);  /* portId, enable */
  else if (!strcmp(name, "cfgAllGem"))
    onuGponUiDebugAllGemPortidValidSet((MV_U32)param1);               /* enable */
#ifdef MV_GPON_STATIC_GEM_PORT                                        
  else if (!strcmp(name, "dummyGemState"))                           
    onuGponUiDebugDummyGemPortFlagSet((MV_U32)param1);                /* flag */
  else if (!strcmp(name, "dummyGemRst"))
    onuGponUiDebugDummyGemPortReset(); 
#endif /* MV_GPON_STATIC_GEM_PORT */
  else if (!strcmp(name, "cfgAesPort"))                                  
    onuGponUiDebugAesSet((MV_U32)param1, (MV_U32)param2);                 /* portId, enable */
  else if (!strcmp(name, "cfgAllAes"))                               
    onuGponUiDebugAesSetAll((MV_U32)param1);                              /* enable */
  else
 	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            Data Attributes                                 */
/* ========================================================================== */
static DEVICE_ATTR(cfgTcont,          S_IWUSR, data_show, data_store);
static DEVICE_ATTR(clrTcont,          S_IWUSR, data_show, data_store);
static DEVICE_ATTR(rstTcont,          S_IWUSR, data_show, data_store);
static DEVICE_ATTR(showTcont,         S_IRUSR, data_show, data_store);
static DEVICE_ATTR(cfgGem,            S_IWUSR, data_show, data_store);
static DEVICE_ATTR(cfgAllGem,         S_IWUSR, data_show, data_store);
static DEVICE_ATTR(showGem0xxx,       S_IRUSR, data_show, data_store);
static DEVICE_ATTR(showGem1xxx,       S_IRUSR, data_show, data_store);
static DEVICE_ATTR(showGem2xxx,       S_IRUSR, data_show, data_store);
static DEVICE_ATTR(showGem3xxx,       S_IRUSR, data_show, data_store);
#ifdef MV_GPON_STATIC_GEM_PORT                             
static DEVICE_ATTR(dummyGemState,     S_IWUSR, data_show, data_store);
static DEVICE_ATTR(dummyGemRst,       S_IWUSR, data_show, data_store);
#endif /* MV_GPON_STATIC_GEM_PORT */
static DEVICE_ATTR(cfgAesPort,        S_IWUSR, data_show, data_store);
static DEVICE_ATTR(cfgAllAes,         S_IWUSR, data_show, data_store);
static DEVICE_ATTR(showAes0xxx,       S_IRUSR, data_show, data_store);
static DEVICE_ATTR(showAes1xxx,       S_IRUSR, data_show, data_store);
static DEVICE_ATTR(showAes2xxx,       S_IRUSR, data_show, data_store);
static DEVICE_ATTR(showAes3xxx,       S_IRUSR, data_show, data_store);                                   
static DEVICE_ATTR(helpData,          S_IRUSR, data_show, data_store);

static struct attribute *data_attrs[] = {
	&dev_attr_cfgTcont.attr,
	&dev_attr_clrTcont.attr,
	&dev_attr_rstTcont.attr,
	&dev_attr_showTcont.attr,
	&dev_attr_cfgGem.attr,
	&dev_attr_cfgAllGem.attr,
	&dev_attr_showGem0xxx.attr,
    &dev_attr_showGem1xxx.attr,
    &dev_attr_showGem2xxx.attr,
    &dev_attr_showGem3xxx.attr,
#ifdef MV_GPON_STATIC_GEM_PORT
	&dev_attr_dummyGemState.attr,
	&dev_attr_dummyGemRst.attr,
#endif /* MV_GPON_STATIC_GEM_PORT */
	&dev_attr_cfgAesPort.attr,
    &dev_attr_cfgAllAes.attr,
    &dev_attr_showAes0xxx.attr,
    &dev_attr_showAes1xxx.attr,
    &dev_attr_showAes2xxx.attr,
    &dev_attr_showAes3xxx.attr,
	&dev_attr_helpData.attr,
	NULL
};

static struct attribute_group data_group = 
{
  .name = "data",
  .attrs = data_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Pm Group                                        */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Pm Commands                                     */
/* ========================================================================== */
static ssize_t pm_show(struct device *dev, 
                       struct device_attribute *attr, 
                       char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if      (!strcmp(name, "fecCnt")) 	
  	return (onuGponUiFecCountersShow(buf));
  else if (!strcmp(name, "rxPloamCnt")) 	
    return (onuGponUiRxPloamCountersShow(buf));
  else if (!strcmp(name, "txPloamCnt")) 
    return (onuGponUiTxPloamCountersShow(buf));
  else if (!strcmp(name, "bwMapCnt")) 
    return (onuGponUiBwMapCountersShow(buf));
  else if (!strcmp(name, "stdCnt")) 
    return (onuGponUiStandardCountersShow(buf));
  else if (!strcmp(name, "gemCnt")) 
    return (onuGponUiGemCountersShow(buf));
  else if (!strcmp(name, "txCnt")) 
    return (onuGponUiTxCountersShow(buf));
  else if (!strcmp(name, "rawCnt_1")) 
    return (onuGponUiRaw_1Counters(buf));
  else if (!strcmp(name, "rawCnt_2")) 
    return (onuGponUiRaw_2Counters(buf));
  else if (!strcmp(name, "helpPm")) 	
    return (onuGponUiPmHelpShow(buf));

  return 0;
}

static ssize_t pm_store(struct device *dev, 
                        struct device_attribute *attr, 
                        const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%d", &param);

  raw_local_irq_save(flags);

  if (!strcmp(name, "cntRdClrState")) 
  	onuGponUiCountersReadClearFlag((MV_BOOL)param);
  else 
  	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            PM Attributes                                   */
/* ========================================================================== */
static DEVICE_ATTR(fecCnt,        S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(rxPloamCnt,    S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(txPloamCnt,    S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(bwMapCnt,      S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(stdCnt,        S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(gemCnt,        S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(txCnt,         S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(rawCnt_1,      S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(rawCnt_2,      S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(cntRdClrState, S_IWUSR, pm_show, pm_store);
static DEVICE_ATTR(helpPm,        S_IRUSR, pm_show, pm_store);

static struct attribute *pm_attrs[] = {
    &dev_attr_fecCnt.attr,
	&dev_attr_rxPloamCnt.attr,
	&dev_attr_txPloamCnt.attr,
	&dev_attr_bwMapCnt.attr,
	&dev_attr_stdCnt.attr,
	&dev_attr_gemCnt.attr,
	&dev_attr_txCnt.attr,
	&dev_attr_rawCnt_1.attr,
    &dev_attr_rawCnt_2.attr,
	&dev_attr_cntRdClrState.attr,
    &dev_attr_helpPm.attr,
	NULL
};

static struct attribute_group pm_group = 
{
  .name = "pm",
  .attrs = pm_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Info Group                                      */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Info Commands                                   */
/* ========================================================================== */
static ssize_t info_show(struct device *dev, 
                         struct device_attribute *attr, 
                         char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))  
  	return -EPERM;              

  if      (!strcmp(name, "info")) 
  	return(onuGponUiInfoShow(buf));
  else if (!strcmp(name, "alarm")) 
    return(onuGponUiAlarmShow(buf));
  else if (!strcmp(name, "helpInfo")) 
    return(onuGponUiInfoHelpShow(buf));

  return 0;
}

static ssize_t info_store(struct device *dev, 
                          struct device_attribute *attr, 
                          const char *buf, size_t len)
{
  return(0);
}

/* ========================================================================== */
/*                            Info Attributes                                 */
/* ========================================================================== */
static DEVICE_ATTR(info,     S_IRUSR, info_show, info_store);
static DEVICE_ATTR(alarm,    S_IRUSR, info_show, info_store);
static DEVICE_ATTR(helpInfo, S_IRUSR, info_show, info_store);

static struct attribute *info_attrs[] = {
	&dev_attr_info.attr,
	&dev_attr_alarm.attr,
    &dev_attr_helpInfo.attr,
	NULL
};

static struct attribute_group info_group = 
{
  .name = "info",
  .attrs = info_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            ONU SYS FS                                      */
/* ========================================================================== */
/******************************************************************************/
int __devinit pon_sysfs_init(void)
{
  int err;
  struct device *pd;

  pd = bus_find_device_by_name(&platform_bus_type, NULL, "gpon");
  if (!pd) {
  	platform_device_register_simple("gpon", -1, NULL, 0);
  	pd = bus_find_device_by_name(&platform_bus_type, NULL, "gpon");
  }

  if (!pd) {
  	printk(KERN_ERR"%s: cannot find gpon device\n", __FUNCTION__);
  	pd = &platform_bus;
  }

  err = sysfs_create_group(&pd->kobj, &info_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &pm_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &data_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &introp_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &protocol_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &misc_group);
  if (err) {
      printk(KERN_INFO "sysfs group failed %d\n", err);
      goto out;
  }

  printk(KERN_INFO "= PON Module SYS FS Init ended successfully =\n");
out:
  return err;
}

#else /* CONFIG_MV_EPON_MODULE */

/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ==                                                                      == */
/* ==           =========   =========   =========   ===       ==           == */                       
/* ==           =========   =========   =========   ====      ==           == */
/* ==           ==          ==     ==   ==     ==   == ==     ==           == */
/* ==           ==          ==     ==   ==     ==   ==  ==    ==           == */
/* ==           =========   =========   ==     ==   ==   ==   ==           == */
/* ==           =========   =========   ==     ==   ==    ==  ==           == */
/* ==           ==          ==          ==     ==   ==     == ==           == */
/* ==           ==          ==          ==     ==   ==      ====           == */
/* ==           =========   ==          =========   ==       ===           == */
/* ==           =========   ==          =========   ==        ==           == */
/* ==                                                                      == */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/* ========================================================================== */
/*                            Info Commands                                   */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponUiInfoShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu information
**               
**  PARAMETERS:  char* buf 
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiInfoShow(char* buf)
{
  MV_STATUS status;
  S_EponIoctlInfo info[EPON_MAX_MAC_NUM];
  MV_U8           *stateDesc[] = {"", "Pending", "Active"};
  int             off = 0;
  int             macId;

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status = onuEponApiInformationGet(&info[macId], macId);
    if (status != MV_OK) 
    {
      off += mvOsSPrintf(buf+off, "Failed read mac information!!!\n");
      return(off);
    }
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "ONT Information:\n");
  off += mvOsSPrintf(buf+off, "---------------------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     State    Ether    Unicast             Bcast               Tx LLId  Tx LLId  \n"); 
  off += mvOsSPrintf(buf+off, "              Type     Mac Addr            Mac Addr                              \n");
  off += mvOsSPrintf(buf+off, " MAC                                                                             \n");  
  off += mvOsSPrintf(buf+off, "---------------------------------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
      off += mvOsSPrintf(buf+off, "[%02d] %-8s %8x %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x %8u %8u\n", 
                        macId,
                        stateDesc[info[macId].onuEponState],
                        info[macId].onuEponCtrlType,
                        info[macId].onuEponMacAddr[0], info[macId].onuEponMacAddr[1], info[macId].onuEponMacAddr[2], 
                        info[macId].onuEponMacAddr[3], info[macId].onuEponMacAddr[4], info[macId].onuEponMacAddr[5],
                        info[macId].onuEponBcastAddr[0], info[macId].onuEponBcastAddr[1], info[macId].onuEponBcastAddr[2],
                        info[macId].onuEponBcastAddr[3], info[macId].onuEponBcastAddr[4], info[macId].onuEponBcastAddr[5],
                        info[macId].onuEponTxLLID, 
                        info[macId].onuEponRxLLID);
  }
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n\r");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiAlarmShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu alarms
**               
**  PARAMETERS:  char* buf 
**                         
**  OUTPUTS:     char* buf 
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiAlarmShow(char* buf)
{
  MV_U32 alarm[2];
  MV_U8  *statusText[] = {"OFF", "ON "};
  int    off = 0;

  onuEponAlarmGet(alarm);

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "-------------\n");
  off += mvOsSPrintf(buf+off, "|ONU ALARMS |\n");
  off += mvOsSPrintf(buf+off, "-------------------------\n");
  off += mvOsSPrintf(buf+off, "| XVR SD    | SERDES SD |\n");
  off += mvOsSPrintf(buf+off, "-------------------------\n");
  off += mvOsSPrintf(buf+off, "| %9s | %9s |\n", statusText[alarm[0]], statusText[alarm[1]]);
  off += mvOsSPrintf(buf+off, "-------------------------\n");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiDebugReadReg
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function test requested address space
**               
**  PARAMETERS:  MV_U32 readOffset
**               MV_U32 readEntry 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuEponUiDebugReadReg(MV_U32 readOffset, MV_U32 readEntry)
{
  MV_U32 regValue;

  regValue = MV_REG_READ((MV_EPON_MAC_REGS_BASE) + readOffset + (readEntry * sizeof (MV_U32))); 

  mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
             "Read register offset 0x%x, entry %d = [0x%08X]\n\r", readOffset, readEntry, regValue);
}

/*******************************************************************************
**
**  onuEponUiDebugWriteReg
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function test requested address space
**               
**  PARAMETERS:  MV_U32 writeOffset
**               MV_U32 writeEntry
**               MV_U32 regValue 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuEponUiDebugWriteReg(MV_U32 writeOffset, MV_U32 writeEntry, MV_U32 regValue)
{
  MV_REG_WRITE((MV_EPON_MAC_REGS_BASE) + writeOffset + (writeEntry * sizeof (MV_U32)), regValue); 

  mvPonPrint(PON_PRINT_INFO, PON_CLI_MODULE,
             "Write  register offset 0x%x, entry %d = [0x%08X]\n\r", writeOffset, writeEntry, regValue);
}

/*******************************************************************************
**
**  onuEponUiInfoHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print info help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiInfoHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[HEX]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [Offset] [Entry]           > readReg   - read onu reg\n");
  off += mvOsSPrintf(buf+off, " echo [Offset] [Entry] [Value]   > writeReg  - write onu reg\n");         
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat info  - dump onu information\n");
  off += mvOsSPrintf(buf+off, " cat alarm - dump onu alarms\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/******************************************************************************/
/* ========================================================================== */
/*                            Pm Commands                                     */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponUiRxCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu rx counters
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiRxCountersShow(char* buf)
{
  MV_STATUS       status;
  S_EponIoctlRxPm rxCounters;
  int             off = 0;

  status = onuEponApiRxPmGet(&rxCounters, infoCntReadClearInd, 0);
  if (status != MV_OK) 
  {
    off += mvOsSPrintf(buf+off, "Failed read Rx pm information!!!\n");
    return(off);
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "Rx Counters\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     FCS      Short    Long     Data     Ctrl     Rprt     Gate    \n"); 
  off += mvOsSPrintf(buf+off, "     Frames   Frames   Frames   Frames   Frames   Frames   Frames  \n");
  off += mvOsSPrintf(buf+off, " MAC Rece     Rece     Rece     Rece     Rece     Rece     Rece    \n");  
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "[00] %8u %8u %8u %8u %8u %8u %8u\n", 
                     rxCounters.fcsErrorFramesCnt,
                     rxCounters.shortFramesCnt, 
                     rxCounters.longFramesCnt,   
                     rxCounters.dataFramesCnt,    
                     rxCounters.ctrlFramesCnt,    
                     rxCounters.reportFramesCnt,
                     rxCounters.gateFramesCnt);    
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n\r");

  return(off);
} 

/*******************************************************************************
**
**  onuEponUiTxCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu tx counters
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiTxCountersShow(char* buf)
{
  MV_STATUS       status;
  S_EponIoctlTxPm txCounters[EPON_MAX_MAC_NUM];
  int             macId;
  int             off = 0;

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status = onuEponApiTxPmGet(&txCounters[macId], infoCntReadClearInd, macId);
    if (status != MV_OK) 
    {
      off += mvOsSPrintf(buf+off, "Failed read Tx pm information!!!\n");
      return(off);
    }
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "Tx Counters\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     Register Register Report   Data     Tx Byte  \n"); 
  off += mvOsSPrintf(buf+off, "     Request  Ack      Frames   Frames   Allow    \n");
  off += mvOsSPrintf(buf+off, " MAC Frames   Frames                              \n");  
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
      off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u\n", 
                        macId,
                        txCounters[macId].ctrlRegReqFramesCnt,
                        txCounters[macId].ctrlRegAckFramesCnt,
                        txCounters[macId].reportFramesCnt,
                        txCounters[macId].dataFramesCnt,
                        txCounters[macId].txAllowedBytesCnt);
  }
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n\r");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiSwCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu sw counters
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiSwCountersShow(char* buf)
{
  MV_STATUS       status;
  S_EponIoctlSwPm swCounters[EPON_MAX_MAC_NUM];
  int             macId;
  int             off = 0;

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status = onuEponApiSwPmGet(&swCounters[macId], infoCntReadClearInd, macId);
    if (status != MV_OK) 
    {
      off += mvOsSPrintf(buf+off, "Failed read Sw pm information!!!\n");
      return(off);
    }
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "Sw Counters\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     Total    Total    Total    Total    Total    Total    Total   \n"); 
  off += mvOsSPrintf(buf+off, "     MPC Tx   MPC Tx   MPC Rx   MPC Rx   MPC Reg  MPC Reg  MPC Reg \n");
  off += mvOsSPrintf(buf+off, "     Frames   Error    Frames   Error    Ack      De-Reg   Re-Reg  \n");  
  off += mvOsSPrintf(buf+off, " MAC          Frames            Frames   Frames   Frames   Frames  \n");  
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
     off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u %8u %8u\n", 
                        macId,
                        swCounters[macId].swTxCnt[0],
                        swCounters[macId].swTxCnt[1],
                        swCounters[macId].swRxCnt[0],
                        swCounters[macId].swRxCnt[1],
                        swCounters[macId].swRxCnt[2],
                        swCounters[macId].swRxCnt[3],
                        swCounters[macId].swRxCnt[4]);
  }
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n\r");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiGpmCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print onu gpm counters
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiGpmCountersShow(char* buf)
{
  MV_STATUS        status;
  S_EponIoctlGpmPm gpmCounters[EPON_MAX_MAC_NUM];
  int              macId;
  int              off = 0;

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status = onuEponApiGpmPmGet(&gpmCounters[macId], infoCntReadClearInd, macId);
    if (status != MV_OK) 
    {
      off += mvOsSPrintf(buf+off, "Failed read Gpm pm information!!!\n");
      return(off);
    }
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "GPM Counters\n");
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     Valid    Grant    Min Proc Length   Discover Fifo    \n"); 
  off += mvOsSPrintf(buf+off, "     Grant    Max Time Time     Error    And      Full    \n");
  off += mvOsSPrintf(buf+off, " MAC          Error    Error             Register Error   \n");  
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
     off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u %8u\n", 
                        macId,
                        gpmCounters[macId].grantValidCnt,                
                        gpmCounters[macId].grantMaxFutureTimeErrorCnt,
                        gpmCounters[macId].minProcTimeErrorCnt,   
                        gpmCounters[macId].lengthErrorCnt,
                        gpmCounters[macId].discoveryAndRegCnt,
                        gpmCounters[macId].fifoFullErrorCnt);
  }
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     OPC      OPC      OPC      OPC      OPC      OPC     \n"); 
  off += mvOsSPrintf(buf+off, "     Discover Register Discover Drop     Hidden   Back to \n");
  off += mvOsSPrintf(buf+off, "     Not      Not      Not      Grant    Grant    Back    \n");  
  off += mvOsSPrintf(buf+off, "     Register Discover Register                           \n");  
  off += mvOsSPrintf(buf+off, "     Bcast             Or Bcast                           \n");  
  off += mvOsSPrintf(buf+off, " MAC                                                      \n");  
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
     off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u %8u\n", 
                        macId,
                        gpmCounters[macId].opcDiscoveryNotRegBcastCnt,
                        gpmCounters[macId].opcRegisterNotDiscoveryCnt,
                        gpmCounters[macId].opcDiscoveryNotRegNotBcastCnt,
                        gpmCounters[macId].opcDropGrantCnt,
                        gpmCounters[macId].opcHiddenGrantCnt,
                        gpmCounters[macId].opcBackToBackCnt);
  }
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n\r");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiAllCountersShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print all onu counters
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiAllCountersShow(char* buf)
{
  MV_STATUS        status;
  S_EponIoctlGpmPm gpmCounters[EPON_MAX_MAC_NUM];
  S_EponIoctlRxPm  rxCounters;
  S_EponIoctlTxPm  txCounters[EPON_MAX_MAC_NUM];
  S_EponIoctlSwPm  swCounters[EPON_MAX_MAC_NUM];

  int              macId;
  int              off = 0;

  status = onuEponApiRxPmGet(&rxCounters, infoCntReadClearInd, 0);

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status |= onuEponApiTxPmGet(&txCounters[macId],   infoCntReadClearInd, macId);
    status |= onuEponApiSwPmGet(&swCounters[macId],   infoCntReadClearInd, macId);
    status |= onuEponApiGpmPmGet(&gpmCounters[macId], infoCntReadClearInd, macId);
    if (status != MV_OK) 
    {
      off += mvOsSPrintf(buf+off, "Failed read pm information!!!\n");
      return(off);
    }
  }

  off += mvOsSPrintf(buf+off, "\n");
  off += mvOsSPrintf(buf+off, "Rx Counters\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     FCS      Short    Long     Data     Ctrl     Rprt     Gate    \n"); 
  off += mvOsSPrintf(buf+off, "     Frames   Frames   Frames   Frames   Frames   Frames   Frames  \n");
  off += mvOsSPrintf(buf+off, " MAC Rece     Rece     Rece     Rece     Rece     Rece     Rece    \n");  
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "[00] %8u %8u %8u %8u %8u %8u %8u\n", 
                     rxCounters.fcsErrorFramesCnt,
                     rxCounters.shortFramesCnt, 
                     rxCounters.longFramesCnt,   
                     rxCounters.dataFramesCnt,    
                     rxCounters.ctrlFramesCnt,    
                     rxCounters.reportFramesCnt,
                     rxCounters.gateFramesCnt);    
  
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n\r");
  off += mvOsSPrintf(buf+off, "Tx Counters\n");
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     Register Register Report   Data     Tx Byte  \n"); 
  off += mvOsSPrintf(buf+off, "     Request  Ack      Frames   Frames   Allow    \n");
  off += mvOsSPrintf(buf+off, " MAC Frames   Frames                              \n");  
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
      off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u\n", 
                        macId,
                        txCounters[macId].ctrlRegReqFramesCnt,
                        txCounters[macId].ctrlRegAckFramesCnt,
                        txCounters[macId].reportFramesCnt,
                        txCounters[macId].dataFramesCnt,
                        txCounters[macId].txAllowedBytesCnt);
  }
  off += mvOsSPrintf(buf+off, "--------------------------------------------------\n\r");
  off += mvOsSPrintf(buf+off, "Sw Counters\n");
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     Total    Total    Total    Total    Total    Total    Total   \n"); 
  off += mvOsSPrintf(buf+off, "     MPC Tx   MPC Tx   MPC Rx   MPC Rx   MPC Reg  MPC Reg  MPC Reg \n");
  off += mvOsSPrintf(buf+off, "     Frames   Error    Frames   Error    Ack      De-Reg   Re-Reg  \n");  
  off += mvOsSPrintf(buf+off, " MAC          Frames            Frames   Frames   Frames   Frames  \n");  
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
     off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u %8u %8u\n", 
                        macId,
                        swCounters[macId].swTxCnt[0],
                        swCounters[macId].swTxCnt[1],
                        swCounters[macId].swRxCnt[0],
                        swCounters[macId].swRxCnt[1],
                        swCounters[macId].swRxCnt[2],
                        swCounters[macId].swRxCnt[3],
                        swCounters[macId].swRxCnt[4]);
  }
  off += mvOsSPrintf(buf+off, "-------------------------------------------------------------------\n\r");
  off += mvOsSPrintf(buf+off, "GPM Counters\n");
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     Valid    Grant    Min Proc Length   Discover Fifo    \n"); 
  off += mvOsSPrintf(buf+off, "     Grant    Max Time Time     Error    And      Full    \n");
  off += mvOsSPrintf(buf+off, " MAC          Error    Error             Register Error   \n");  
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
     off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u %8u\n", 
                        macId,
                        gpmCounters[macId].grantValidCnt,                
                        gpmCounters[macId].grantMaxFutureTimeErrorCnt,
                        gpmCounters[macId].minProcTimeErrorCnt,   
                        gpmCounters[macId].lengthErrorCnt,
                        gpmCounters[macId].discoveryAndRegCnt,
                        gpmCounters[macId].fifoFullErrorCnt);
  }
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");
  off += mvOsSPrintf(buf+off, "     OPC      OPC      OPC      OPC      OPC      OPC     \n"); 
  off += mvOsSPrintf(buf+off, "     Discover Register Discover Drop     Hidden   Back to \n");
  off += mvOsSPrintf(buf+off, "     Not      Not      Not      Grant    Grant    Back    \n");  
  off += mvOsSPrintf(buf+off, "     Register Discover Register                           \n");  
  off += mvOsSPrintf(buf+off, "     Bcast             Or Bcast                           \n");  
  off += mvOsSPrintf(buf+off, " MAC                                                      \n");  
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n");

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
     off += mvOsSPrintf(buf+off, "[%02d] %8u %8u %8u %8u %8u %8u\n", 
                        macId,
                        gpmCounters[macId].opcDiscoveryNotRegBcastCnt,
                        gpmCounters[macId].opcRegisterNotDiscoveryCnt,
                        gpmCounters[macId].opcDiscoveryNotRegNotBcastCnt,
                        gpmCounters[macId].opcDropGrantCnt,
                        gpmCounters[macId].opcHiddenGrantCnt,
                        gpmCounters[macId].opcBackToBackCnt);
  }
  off += mvOsSPrintf(buf+off, "----------------------------------------------------------\n\r");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiPmHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print pm help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiPmHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[DEC]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [state] > cntRdClrState - onu counters flag, Read[0], Read Clear[1]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " cat rxCnt               - dump Rx counters\n");
  off += mvOsSPrintf(buf+off, " cat txCnt               - dump Tx counters\n");
  off += mvOsSPrintf(buf+off, " cat swCnt               - dump Sw counters\n");
  off += mvOsSPrintf(buf+off, " cat gpmCnt              - dump GPM counters\n");
  off += mvOsSPrintf(buf+off, " cat allCnt              - dump All counters\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiCountersReadClearFlag
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function config onu clear counters flag
**               
**  PARAMETERS:  char* buf
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     void
**                   
*******************************************************************************/
void onuEponUiCountersReadClearFlag(MV_BOOL clear)
{
  infoCntReadClearInd = clear;
}

/*******************************************************************************
**
**  onuEponUiCfgHelpShow
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function print cfg help
**               
**  PARAMETERS:  char* buf 
**
**  OUTPUTS:     char* buf
**
**  RETURNS:     message length 
**                   
*******************************************************************************/
int onuEponUiCfgHelpShow(char* buf)
{
  int off = 0;

  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Configuration Commands[DEC]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, " echo [enable]        > rxEn - onu RX enbale, Disable[0], Enable[1]\n");
  off += mvOsSPrintf(buf+off, " echo [enable][macId] > txEn - onu mac TX enbale, Disable[0], Enable[1]\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "Display Commands: cat <file>\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");
  off += mvOsSPrintf(buf+off, "============================================================================\n");

  return(off);
}

/*******************************************************************************
**
**  onuEponUiCountersReadClearFlag
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function config onu RX enable
**               
**  PARAMETERS:  char* buf
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     void
**                   
*******************************************************************************/
void onuEponUiCfgMacRxEn(MV_U32 enable)
{
  mvOnuEponMacOnuRxEnableSet(enable);
}

/*******************************************************************************
**
**  onuEponUiCfgMacTxEn
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function config onu mac TX enable
**               
**  PARAMETERS:  char* buf
**                        
**  OUTPUTS:     char* buf
**
**  RETURNS:     void
**                   
*******************************************************************************/
void onuEponUiCfgMacTxEn(MV_U32 enable, MV_U32 macId)
{
  mvOnuEponMacOnuTxEnableSet(enable, macId);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/*                            ONU SYS FS Definition                           */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/* ========================================================================== */
/*                            Info Group                                      */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Info Commands                                   */
/* ========================================================================== */
static ssize_t info_show(struct device *dev, 
                         struct device_attribute *attr, 
                         char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))  
  	return -EPERM;              

  if      (!strcmp(name, "info")) 
  	return(onuEponUiInfoShow(buf));
  else if (!strcmp(name, "alarm")) 
    return(onuEponUiAlarmShow(buf));
  else if (!strcmp(name, "helpInfo")) 
    return(onuEponUiInfoHelpShow(buf));

  return 0;
}

static ssize_t info_store(struct device *dev, 
                          struct device_attribute *attr, 
                          const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param1 = 0;
  unsigned int  param2 = 0;
  unsigned int  param3 = 0;

  if (!capable(CAP_NET_ADMIN))
    return -EPERM;

  sscanf(buf, "%x %x %x", &param1, &param2, &param3);

  raw_local_irq_save(flags);

  if      (!strcmp(name, "readReg"))
    onuEponUiDebugReadReg((MV_U32)param1, (MV_U32)param2);                     /* readOffset, readEntry */
  else if (!strcmp(name, "writeReg"))
    onuEponUiDebugWriteReg((MV_U32)param1, (MV_U32)param2, (MV_U32)param3);    /*  writeOffset, writeEntry, regValue */
  else 
    printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            Info Attributes                                 */
/* ========================================================================== */
static DEVICE_ATTR(info,     S_IRUSR, info_show, info_store);
static DEVICE_ATTR(alarm,    S_IRUSR, info_show, info_store);
static DEVICE_ATTR(helpInfo, S_IRUSR, info_show, info_store);
static DEVICE_ATTR(readReg,  S_IWUSR, info_show, info_store);
static DEVICE_ATTR(writeReg, S_IWUSR, info_show, info_store);

static struct attribute *info_attrs[] = {
	&dev_attr_info.attr,
	&dev_attr_alarm.attr,
    &dev_attr_readReg.attr,                    
    &dev_attr_writeReg.attr,                   
    &dev_attr_helpInfo.attr,
	NULL
};

static struct attribute_group info_group = 
{
  .name = "info",
  .attrs = info_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Pm Group                                        */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Pm Commands                                     */
/* ========================================================================== */
static ssize_t pm_show(struct device *dev, 
                       struct device_attribute *attr, 
                       char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if      (!strcmp(name, "rxCnt")) 
    return (onuEponUiRxCountersShow(buf));
  else if (!strcmp(name, "txCnt")) 
    return (onuEponUiTxCountersShow(buf));
  else if (!strcmp(name, "swCnt")) 
    return (onuEponUiSwCountersShow(buf));
  else if (!strcmp(name, "gpmCnt")) 
    return (onuEponUiGpmCountersShow(buf));
  else if (!strcmp(name, "allCnt")) 
    return (onuEponUiAllCountersShow(buf));
  else if (!strcmp(name, "helpPm")) 	
    return (onuEponUiPmHelpShow(buf));

  return 0;
}

static ssize_t pm_store(struct device *dev, 
                        struct device_attribute *attr, 
                        const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%d", &param);

  raw_local_irq_save(flags);

  if (!strcmp(name, "cntRdClrState")) 
  	onuEponUiCountersReadClearFlag((MV_BOOL)param);
  else 
  	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            PM Attributes                                   */
/* ========================================================================== */
static DEVICE_ATTR(rxCnt,         S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(txCnt,         S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(swCnt,         S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(gpmCnt,        S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(allCnt,        S_IRUSR, pm_show, pm_store);
static DEVICE_ATTR(cntRdClrState, S_IWUSR, pm_show, pm_store);
static DEVICE_ATTR(helpPm,        S_IRUSR, pm_show, pm_store);

static struct attribute *pm_attrs[] = {
	&dev_attr_rxCnt.attr,
	&dev_attr_txCnt.attr,
	&dev_attr_swCnt.attr,
    &dev_attr_gpmCnt.attr,
    &dev_attr_allCnt.attr,
	&dev_attr_cntRdClrState.attr,
    &dev_attr_helpPm.attr,
	NULL
};

static struct attribute_group pm_group = 
{
  .name = "pm",
  .attrs = pm_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            Cfg Group                                       */
/* ========================================================================== */
/******************************************************************************/

/* ========================================================================== */
/*                            Cfg Commands                                    */
/* ========================================================================== */
static ssize_t cfg_show(struct device *dev, 
                        struct device_attribute *attr, 
                        char *buf)
{
  const char* name = attr->attr.name;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  if (!strcmp(name, "helpCfg")) 	
    return (onuEponUiCfgHelpShow(buf));

  return 0;
}

static ssize_t cfg_store(struct device *dev, 
                         struct device_attribute *attr, 
                         const char *buf, size_t len)
{
  const char* name = attr->attr.name;
  unsigned long flags;
  unsigned int  param1 = 0;
  unsigned int  param2 = 0;

  if (!capable(CAP_NET_ADMIN))
  	return -EPERM;

  sscanf(buf, "%d %d", &param1, &param2);

  raw_local_irq_save(flags);

  if (!strcmp(name, "rxEn")) 
  	onuEponUiCfgMacRxEn((MV_U32)param1);
  if (!strcmp(name, "txEn")) 
  	onuEponUiCfgMacTxEn((MV_U32)param1, (MV_U32)param2);
  else 
  	printk("%s: illegal operation <%s>\n", __FUNCTION__, attr->attr.name);

  raw_local_irq_restore(flags);

  return(len);
}

/* ========================================================================== */
/*                            Cfg Attributes                                  */
/* ========================================================================== */
static DEVICE_ATTR(rxEn,          S_IWUSR, cfg_show, cfg_store);
static DEVICE_ATTR(txEn,          S_IWUSR, cfg_show, cfg_store);
static DEVICE_ATTR(helpCfg,       S_IRUSR, cfg_show, cfg_store);

static struct attribute *cfg_attrs[] = {
	&dev_attr_rxEn.attr,
	&dev_attr_txEn.attr,
    &dev_attr_helpCfg.attr,
	NULL
};

static struct attribute_group cfg_group = 
{
  .name = "cfg",
  .attrs = cfg_attrs,
};

/******************************************************************************/
/* ========================================================================== */
/*                            ONU SYS FS                                      */
/* ========================================================================== */
/******************************************************************************/
int pon_sysfs_init(void)
{
  int err;
  struct device *pd;

  pd = bus_find_device_by_name(&platform_bus_type, NULL, "epon");
  if (!pd) {
  	platform_device_register_simple("epon", -1, NULL, 0);
  	pd = bus_find_device_by_name(&platform_bus_type, NULL, "epon");
  }

  if (!pd) {
  	printk(KERN_ERR"%s: cannot find epon device\n", __FUNCTION__);
  	pd = &platform_bus;
  }

  err = sysfs_create_group(&pd->kobj, &info_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &pm_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  err = sysfs_create_group(&pd->kobj, &cfg_group);
  if (err) {
  	printk(KERN_INFO "sysfs group failed %d\n", err);
  	goto out;
  }

  printk(KERN_INFO "= PON Module SYS FS Init ended successfully =\n");
out:
  return err;
}

#endif /* CONFIG_MV_GPON_MODULE */

/*******************************************************************************
**
**  onuPonUsrInterfaceCreate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function create user interface - sys fs
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonUsrInterfaceCreate(void)
{
  if (pon_sysfs_init() != 0)
    return(MV_ERROR);

  return(MV_OK); 
}
