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
**  FILE        : ponOnuPm.h                                                 **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON Alarm and Statistics           **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_PM_H
#define _ONU_GPON_PM_H

/* Definitions
------------------------------------------------------------------------------*/ 

/* Enums                              
------------------------------------------------------------------------------*/ 
#ifdef MV_GPON_PERFORMANCE_CHECK
typedef enum
{
  PON_ISR_PERFORMANCE              = 0, 
  PON_MNG1_PERFORMANCE             = 1,  
  PON_MNG2_PERFORMANCE             = 2,
  PON_MNG3_PERFORMANCE             = 3,
  PON_EXT_BURST_PLOAM_PERFORMANCE  = 4,
  PON_KEY_SWITCH_PLOAM_PERFORMANCE = 5,
  PON_CFG_PORT_PLOAM_PERFORMANCE   = 6,
  PON_REQ_KEY_PLOAM_PERFORMANCE    = 7,
  PON_ALLOC_ID_PLOAM_PERFORMANCE   = 8,
  PON_PASS_PLOAM_PERFORMANCE       = 9,
  PON_ENC_PORT_PLOAM_PERFORMANCE   = 10,
  PON_DIS_PLOAM_PERFORMANCE        = 11,
  PON_DACT_PLOAM_PERFORMANCE       = 12,
  PON_RNG_PLOAM_PERFORMANCE        = 13,
  PON_ONU_ID_PLOAM_PERFORMANCE     = 14,
  PON_OVERHEAD_PLOAM_PERFORMANCE   = 15,
  PON_MAX_PERFORMANCE              = 16
}E_GponPerformanceCheck;
#endif /* MV_GPON_PERFORMANCE_CHECK */

/* Typedefs
------------------------------------------------------------------------------*/
typedef struct
{
  MV_U32 rxMsgIdPloamCounter [ONU_GPON_DS_MSG_LAST+1];
  MV_U32 rxMsgTotalPloamCounter;
  MV_U32 txErrMsgIdPloamCounter [ONU_GPON_US_MSG_LAST+1];
  MV_U32 txMsgIdPloamCounter[ONU_GPON_US_MSG_LAST+1];
  MV_U32 txMsgTotalPloamCounter;
}S_PloamSwPm;

typedef struct
{
  MV_U32 idlePloamCounter;
  MV_U32 crcErrorPloamCounter;
  MV_U32 fifoOverErrorPloamCounter;
  MV_U32 receivedBroadcastPloamCounter;
  MV_U32 receivedOnuIdPloamCounter;
  MV_U32 rxMsgIdPloamCounter [ONU_GPON_DS_MSG_LAST+1];
  MV_U32 rxMsgTotalPloamCounter;
}S_RxPloamPm;

typedef struct
{
  MV_U32 txErrMsgIdPloamCounter [ONU_GPON_US_MSG_LAST+1];
  MV_U32 txMsgIdPloamCounter    [ONU_GPON_US_MSG_LAST+1];
  MV_U32 txMsgTotalPloamCounter;
}S_TxPloamPm;

typedef struct                                       
{
  MV_U32 allocCrcErr;
  MV_U32 allocCorrectableCrcErr;
  MV_U32 allocUnCorrectableCrcErr;
  MV_U32 allocCorrec;
  MV_U32 totalReceivedAllocBytes;
}S_RxBwMapPm;

typedef struct
{
  MV_U32 receivedBytes;
  MV_U32 correctedBytes;
  MV_U32 correctedBits;
  MV_U32 receivedCodeWords;
  MV_U32 uncorrectedCodeWords;
}S_RxFecPm;

typedef struct
{
  MV_U32 plend;
}S_RxPlendPm;

typedef struct
{
  MV_U32 bip8;
}S_RxBip8Pm;

typedef struct
{
  MV_U32 receivedIdleGemFrames;
  MV_U32 receivedValidGemFrames;
  MV_U32 receivedUndefinedGemFrames;
  MV_U32 receivedOmciFrames;
  MV_U32 droppedGemFrames;
  MV_U32 droppedOmciFrames;
  MV_U32 receivedGemFramesWithUncorrHecErr;
  MV_U32 receivedGemFramesWithOneFixedHecErr;
  MV_U32 receivedGemFramesWithTwoFixedHecErr;
  MV_U32 totalByteCountOfReceivedValidGemFrames;
  MV_U32 totalByteCountOfReceivedUndefinedGemFrames;
  MV_U32 gemReassembleMemoryFlush;
  MV_U32 gemSynchLost;
  MV_U32 receivedEthFramesWithCorrFcs;
  MV_U32 receivedEthFramesWithFcsError;
  MV_U32 receivedOmciFramesWithCorrCrc;
  MV_U32 receivedOmciFramesWithCrcError;
}S_GemPm;

typedef struct
{
  MV_U32 transmittedGemPtiTypeOneFrames;   
  MV_U32 transmittedGemPtiTypeZeroFrames;  
  MV_U32 transmittedIdleGemFrames;         
  MV_U32 transmittedTxEnableCount;         
  MV_U32 transmittedTxEnableThresholdCount;         
  MV_U32 transmittedEthFramesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];    
  MV_U32 transmittedEthBytesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];     
  MV_U32 transmittedGemFramesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];     
  MV_U32 transmittedIdleGemFramesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];
}S_TxPm;

typedef struct
{
  S_RxBwMapPm rxBwMap;
  S_RxPloamPm rxPloam;
  S_TxPloamPm txPloam;
  S_RxPlendPm rxPlend;
  S_RxFecPm   rxFec;
  S_RxBip8Pm  rxBip8;
  S_GemPm     gem;
  S_TxPm      tx;
}S_GponPm;

#ifdef MV_GPON_PERFORMANCE_CHECK
typedef struct
{
  MV_U32 *uSecCntStart;
  MV_U32 *uSecCntStop;
  MV_U32  uSecCntIdx;
}S_GponPerformanceCheckNode;

typedef struct
{
  S_GponPerformanceCheckNode pmCheckNode[PON_MAX_PERFORMANCE];
}S_GponPerformanceCheck;
#endif /* MV_GPON_PERFORMANCE_CHECK */

/* Global variables
------------------------------------------------------------------------------*/
#ifdef MV_GPON_PERFORMANCE_CHECK
extern S_GponPerformanceCheck g_GponPmCheck;
#endif /* MV_GPON_PERFORMANCE_CHECK */

/* Global functions
------------------------------------------------------------------------------*/
#ifdef MV_GPON_PERFORMANCE_CHECK
MV_STATUS onuGponPmInit(void);
#endif /* MV_GPON_PERFORMANCE_CHECK */
void      onuGponPmInPmInit(void);
void      onuGponPmTimerPmHndl(unsigned long data);
void      onuGponPmTimerExpireHndl(void);
MV_STATUS onuGponPmCountersAdd(void);
MV_STATUS onuGponPmRxSwCountersAdd(MV_U8 a_msgId);
MV_STATUS onuGponPmSwCountersUpdate(S_RxPloamPm *rxPloamPm, S_TxPloamPm *txPloamPm);
MV_STATUS onuGponPmFecPmGet(S_RxFecPm *fecPm);
MV_STATUS onuGponPmRxPloamPmGet(S_RxPloamPm *rxPloamPm);
MV_STATUS onuGponPmTxPloamPmGet(S_TxPloamPm *txPloamPm);
MV_STATUS onuGponPmRxBwMapPmGet(S_RxBwMapPm *rxBwMapPm);
MV_STATUS onuGponPmRxPlendPmGet(S_RxPlendPm *rxPlendPm);
MV_STATUS onuGponPmRxBip8PmGet(S_RxBip8Pm *rxBip8Pm);
MV_STATUS onuGponPmGemPmGet(S_GemPm *gemPm);
MV_STATUS onuGponPmTxPmGet(S_TxPm *txPm);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_PM_H */
  