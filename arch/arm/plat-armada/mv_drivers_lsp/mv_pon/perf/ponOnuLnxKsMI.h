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
**  FILE        : ponOnuLnxKsMI.h                                            **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON Management Interface           **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_LINUX_KS_MNG_INTERFACE_H
#define _ONU_GPON_LINUX_KS_MNG_INTERFACE_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
#define PON_NUM_DEVICES     (1)
#define PON_DEV_NAME        ("pon")
#define MVPON_IOCTL_MAGIC   ('P')

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

#define MVGPON_IOCTL_INIT               _IOW(MVPON_IOCTL_MAGIC, 1, unsigned int)
#define MVGPON_IOCTL_BEN_INIT           _IOW(MVPON_IOCTL_MAGIC, 2, unsigned int)
#define MVGPON_IOCTL_DATA_TCONT_CONFIG	_IOW(MVPON_IOCTL_MAGIC, 3, unsigned int)
#define MVGPON_IOCTL_DATA_TCONT_RESET	 _IO(MVPON_IOCTL_MAGIC, 4)
#define MVGPON_IOCTL_INFO               _IOR(MVPON_IOCTL_MAGIC, 5, unsigned int)
#define MVGPON_IOCTL_ALARM              _IOR(MVPON_IOCTL_MAGIC, 6, unsigned int)
#define MVGPON_IOCTL_PM                 _IOR(MVPON_IOCTL_MAGIC, 7, unsigned int)

/* Enums                              
------------------------------------------------------------------------------*/ 
typedef enum
{
  E_GPON_IOCTL_PM_PLOAM_RX = 1,
  E_GPON_IOCTL_PM_PLOAM_TX = 2,
  E_GPON_IOCTL_PM_BW_MAP   = 3,
  E_GPON_IOCTL_PM_FEC      = 4,
  E_GPON_IOCTL_PM_GEM_RX   = 5,
  E_GPON_IOCTL_PM_GEM_TX   = 6
}E_GponIoctlPmSection;

/* Typedefs
------------------------------------------------------------------------------*/
typedef struct                                                                                                                                
{                                                                                                                                             
  MV_U32 rxIdlePloam;                                                                                                                       
  MV_U32 rxCrcErrorPloam;                                                                                                                   
  MV_U32 rxFifoOverErrorPloam;                                                                                                              
  MV_U32 rxBroadcastPloam;                                                                                                                  
  MV_U32 rxOnuIdPloam;                                                                                                                      
  MV_U32 rxMsgIdPloam[ONU_GPON_DS_MSG_LAST+1];                                                                                              
  MV_U32 rxMsgTotalPloam;                                                                                                                   
}S_GponIoctlPloamRxPm;                                                                                                                            

typedef struct                                                                                                                                
{                                                                                                                                             
  MV_U32 txErrMsgIdPloam[ONU_GPON_US_MSG_LAST+1];                                                                                           
  MV_U32 txMsgIdPloam[ONU_GPON_US_MSG_LAST+1];                                                                                                                                  
  MV_U32 txMsgTotalPloam;                                                                    
}S_GponIoctlPloamTxPm;                                                                             
                                                                                               
typedef struct                                                                                 
{                                                                                              
  MV_U32 allocCrcErr;                                                                        
  MV_U32 allocCorrectableCrcErr;                                                                                                                                                  
  MV_U32 allocUnCorrectableCrcErr;                                                                                                             
  MV_U32 allocCorrec;                                                                                                                          
  MV_U32 totalReceivedAllocBytes;                                                                                                              
}S_GponIoctlBwMapPm;                                                                                                                                 
                                                                                                                                                 
typedef struct                                                                                                                                   
{                                                                                                          
  MV_U32 receivedBytes;                                                                                  
  MV_U32 correctedBytes;                                                                                                                    
  MV_U32 correctedBits;                                                                                                                     
  MV_U32 receivedCodeWords;                                                                                                                 
  MV_U32 uncorrectedCodeWords;                                                                                                              
}S_GponIoctlFecPm;                                                                                                                                
                                                                                                                                              
typedef struct                                                                                                                                
{                                                                                                                                             
  MV_U32 gemRxIdleGemFrames;                                                                                
  MV_U32 gemRxValidGemFrames;                                                                               
  MV_U32 gemRxUndefinedGemFrames;                                                                                                                 
  MV_U32 gemRxOmciFrames;                                                                                                                         
  MV_U32 gemRxDroppedGemFrames;                                                                                                                   
  MV_U32 gemRxDroppedOmciFrames;                                                                                                                  
  MV_U32 gemRxGemFramesWithUncorrHecErr;                                                                                                          
  MV_U32 gemRxGemFramesWithOneFixedHecErr;                                                                                             
  MV_U32 gemRxGemFramesWithTwoFixedHecErr;                                                                                             
  MV_U32 gemRxTotalByteCountOfReceivedValidGemFrames;                                                                                  
  MV_U32 gemRxTotalByteCountOfReceivedUndefinedGemFrames;                                                                              
  MV_U32 gemRxGemReassembleMemoryFlush;                                                                                                
  MV_U32 gemRxGemSynchLost;                                                                                                            
  MV_U32 gemRxEthFramesWithCorrFcs;                                                                                                    
  MV_U32 gemRxEthFramesWithFcsError;                                                                                                   
  MV_U32 gemRxOmciFramesWithCorrCrc;                                                                                                   
  MV_U32 gemRxOmciFramesWithCrcError;                                                                                                  
}S_GponIoctlGemRxPm;              

typedef struct                                                                                                            
{                                                                                                                         
  MV_U32 gemTxGemPtiTypeOneFrames;                                                                                      
  MV_U32 gemTxGemPtiTypeZeroFrames;                                                                                     
  MV_U32 gemTxIdleGemFrames;                                                                                            
  MV_U32 gemTxEthFramesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];                                                          
  MV_U32 gemTxEthBytesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];                                                           
  MV_U32 gemTxGemFramesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];                                                           
  MV_U32 gemTxIdleGemFramesViaTconti[ONU_GPON_MAX_NUM_OF_T_CONTS];                                                      
}S_GponIoctlGemTxPm;                                                                                                          
                                                                                                                          
typedef struct                                                                                                            
{                                                                                                                         
  unsigned int  section;                                                                                                  
  union                                                                                                                   
  {                                                                                                                       
    S_GponIoctlPloamRxPm ploamRx;                                                                                             
    S_GponIoctlPloamTxPm ploamTx;                                                                                             
    S_GponIoctlBwMapPm   bwMap;                                                             
    S_GponIoctlFecPm     fec;                                                                             
    S_GponIoctlGemRxPm   gemRx;                                                                           
    S_GponIoctlGemTxPm   gemTx;                                                                           
  };                                                                                                  
}S_GponIoctlPm;                                                                                           
                                                                                                      
typedef struct                                                                                        
{                                                                                                     
  MV_U32 alarmTbl[ONU_GPON_NUM_OF_ALARMS];                                                      
}S_GponIoctlAlarm;                                                                                        
                                                                                                      
typedef struct                                                                                        
{                                                                                        
  MV_U32 onuId;      
  MV_U32 onuState;      
  MV_U32 omccPort;                                                                 
  MV_U32 omccValid;                                                                
  MV_U8  serialNum[8];                                                            
  MV_U8  password[10];     
  MV_U32 disableSn;
}S_GponIoctlInfo;                                                                            
                                                                                         
typedef struct                                                                           
{                                                                                        
  MV_U32 alloc;                                                                    
  MV_U32 tcont;                                                                    
}S_GponIoctlData;                                                                            
                                                                                         
typedef struct                                                                           
{
  MV_U32 mask;   
  MV_U32 polarity;
  MV_U32 delay;  
  MV_U32 enStop; 
  MV_U32 enStart;
}S_GponIoctlXvr;

typedef struct 
{
  S_GponIoctlInfo  info;
  S_GponIoctlAlarm alarm;
  S_GponIoctlPm    pm;
  S_GponIoctlData  data;
  struct cdev  cdev;
}S_PonModuleCdev;

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

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

#define MVEPON_IOCTL_INIT   	_IOW(MVPON_IOCTL_MAGIC, 1, unsigned int)
#define MVEPON_IOCTL_FEC_CONFIG	_IOW(MVPON_IOCTL_MAGIC, 2, unsigned int)
#define MVEPON_IOCTL_ENC_CONFIG	_IOW(MVPON_IOCTL_MAGIC, 3, unsigned int)
#define MVEPON_IOCTL_ENC_KEY	_IOW(MVPON_IOCTL_MAGIC, 4, unsigned int)
#define MVEPON_IOCTL_INFO       _IOR(MVPON_IOCTL_MAGIC, 5, unsigned int)
#define MVEPON_IOCTL_PM         _IOR(MVPON_IOCTL_MAGIC, 6, unsigned int)

#define EPON_API_MAX_NUM_OF_MAC (8)
#define EPON_API_MAC_LEN        (6)

/* Enums                              
------------------------------------------------------------------------------*/ 
typedef enum
{
  E_EPON_IOCTL_PM_RX  = 1,
  E_EPON_IOCTL_PM_TX  = 2,
  E_EPON_IOCTL_PM_SW  = 3,
  E_EPON_IOCTL_PM_GPM = 4
}E_EponIoctlPmSection;

typedef enum
{
  E_EPON_IOCTL_MPCP_TX_FRAME_CNT             = 0,
  E_EPON_IOCTL_MPCP_TX_ERROR_FRAME_CNT       = 1,
  E_EPON_IOCTL_MAX_TX_SW_CNT
}E_EponIoctlTxSwCnt;

typedef enum
{
  E_EPON_IOCTL_MPCP_RX_FRAME_CNT             = 0,
  E_EPON_IOCTL_MPCP_RX_ERROR_FRAME_CNT       = 1,
  E_EPON_IOCTL_MPCP_REGISTER_ACK_CNT         = 2,
  E_EPON_IOCTL_MPCP_REGISTER_NACK_CNT        = 3,
  E_EPON_IOCTL_MPCP_REGISTER_DEREG_FRAME_CNT = 4,
  E_EPON_IOCTL_MPCP_REGISTER_REREG_FRAME_CNT = 5,
  E_EPON_IOCTL_MAX_RX_SW_CNT
}E_EponIoctlRxSwCnt;

/* Typedefs
------------------------------------------------------------------------------*/
typedef struct
{
  MV_U32 ctrlRegReqFramesCnt; /* Count number of register request frames transmitted     */  
  MV_U32 ctrlRegAckFramesCnt; /* Count number of register acknowledge frames transmitted */  
  MV_U32 reportFramesCnt;     /* Count number of report frames transmitted               */
  MV_U32 dataFramesCnt;       /* Count number of data frames transmitted                 */
  MV_U32 txAllowedBytesCnt;   /* Count number of Tx Byte Allow counter                   */
}S_EponIoctlTxPm;

typedef struct
{
  MV_U32 fcsErrorFramesCnt;   /* Count number of received frames with FCS errors */
  MV_U32 shortFramesCnt;      /* Count number of short frames received           */
  MV_U32 longFramesCnt;       /* Count number of long frames received            */
  MV_U32 dataFramesCnt;       /* Count number of data frames received            */
  MV_U32 ctrlFramesCnt;       /* Count number of control frames received         */
  MV_U32 reportFramesCnt;     /* Count number of report frames received          */
  MV_U32 gateFramesCnt;       /* Count number of gate frames received            */
}S_EponIoctlRxPm;

typedef struct
{
  MV_U32 swTxCnt[E_EPON_IOCTL_MAX_TX_SW_CNT];
  MV_U32 swRxCnt[E_EPON_IOCTL_MAX_RX_SW_CNT]; 
}S_EponIoctlSwPm;

typedef struct
{
  MV_U32 grantValidCnt;                 /* Count number of valid grant                          */
  MV_U32 grantMaxFutureTimeErrorCnt;    /* Count number of grant max future time error          */
  MV_U32 minProcTimeErrorCnt;           /* Count number of min proc time error                  */
  MV_U32 lengthErrorCnt;                /* Count number of length error                         */
  MV_U32 discoveryAndRegCnt;            /* Count number of discovery & register                 */
  MV_U32 fifoFullErrorCnt;              /* Count number of fifo full error                      */
  MV_U32 opcDiscoveryNotRegBcastCnt;    /* Count number of opc discoveryNotRegBcastCnt          */
  MV_U32 opcRegisterNotDiscoveryCnt;    /* Count number of opc register not discovery           */ 
  MV_U32 opcDiscoveryNotRegNotBcastCnt; /* Count number of opc discovery not register not bcast */ 
  MV_U32 opcDropGrantCnt;               /* Count number of opc drop grant                       */ 
  MV_U32 opcHiddenGrantCnt;             /* Count number of opc hidden grant                     */ 
  MV_U32 opcBackToBackCnt;              /* Count number of opc back to back                     */ 
}S_EponIoctlGpmPm;                               

typedef struct
{
  MV_U32 macId;
  MV_U32 section;
  union
  {
    S_EponIoctlRxPm  rxCnt;
    S_EponIoctlTxPm  txCnt;
    S_EponIoctlSwPm  swCnt;
    S_EponIoctlGpmPm gpmCnt;
  };
}S_EponIoctlPm;

typedef struct
{
  MV_U32 macId;
  MV_U32 onuEponState;                       /* ONU State                   */  
  MV_U32 onuEponCtrlType;                    /* ONU Control Type            */
  MV_U8  onuEponMacAddr[EPON_API_MAC_LEN];   /* ONU MAC Address             */
  MV_U8  onuEponBcastAddr[EPON_API_MAC_LEN]; /* ONU MAC Broadcast Address   */
  MV_U32 onuEponRxLLID;                      /* ONU Rx Packet Rx LLID Array */
  MV_U32 onuEponTxLLID;                      /* ONU Rx Packet Tx LLID Array */
}S_EponIoctlInfo;

typedef struct
{
  MV_U8 macAddr[EPON_API_MAX_NUM_OF_MAC][EPON_API_MAC_LEN];
}S_EponIoctlInit;

typedef struct
{
  MV_U32 rxGenFecEn;
  MV_U32 txGenFecEn;
  MV_U32 txMacFecEn[EPON_API_MAX_NUM_OF_MAC];
}S_EponIoctlFec;

typedef struct
{
  MV_U32 macId;
  MV_U32 encEnable;
  MV_U32 encKeyIndex0;
  MV_U32 encKeyIndex1;
}S_EponIoctlEnc;

typedef struct 
{
  S_EponIoctlInit  init;
  S_EponIoctlInfo  info;
  S_EponIoctlPm    pm;
  S_EponIoctlFec   fec;
  S_EponIoctlEnc   enc;
  struct cdev  cdev;
}S_PonModuleCdev;

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

#endif /* CONFIG_MV_GPON_MODULE */

/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ==                                                                      == */
/* ==  ========= ======== ==        == ==        == ======== ===       ==  == */                       
/* ==  ========= ======== ==        == ==        == ======== ====      ==  == */
/* ==  ==        ==    == ===      === ===      === ==    == == ==     ==  == */
/* ==  ==        ==    == ====    ==== ====    ==== ==    == ==  ==    ==  == */
/* ==  ==        ==    == == ==  == == == ==  == == ==    == ==   ==   ==  == */
/* ==  ==        ==    == ==  ====  == ==  ====  == ==    == ==    ==  ==  == */
/* ==  ==        ==    == ==  ====  == ==  ====  == ==    == ==     == ==  == */
/* ==  ==        ==    == ==   ==   == ==   ==   == ==    == ==      ====  == */
/* ==  ========= ======== ==   ==   == ==   ==   == ======== ==       ===  == */
/* ==  ========= ======== ==   ==   == ==   ==   == ======== ==        ==  == */
/* ==                                                                      == */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/
MV_STATUS onuPonMngInterfaceCreate(void);
MV_STATUS onuPonMngInterfaceRelease(void);
void      onuPonMiNotifyCallback(MV_U32 onuState);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_LINUX_KS_MNG_INTERFACE_H */
