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
**  FILE        : ponOnuLnxKsOs.c                                            **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON Linux OS resources             **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_LINUX_KS_OS_H
#define _ONU_GPON_LINUX_KS_OS_H

/* Include Files
------------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/timer.h> 
#include <linux/slab.h>
#include <linux/byteorder/generic.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioctl.h>

#include "mvTypes.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"

/* Definitions
------------------------------------------------------------------------------*/ 

/* Printing Definitions */
#define PON_PRINT_ERROR        (1)                                                                                   
#define PON_PRINT_INFO         (2)
#define PON_PRINT_DEBUG        (3)

#define MODULE_SHIFT           (16)
#define MODULE_MASK            (0x001F)
#define OPTIONS_MASK           (0xFFFF)

#define ONU_PON_SM_DEBUG_ATTR  (0x0001)                              
#define ONU_PON_SM_STATE_ATTR  (0x0002)
#define ONU_PON_SM_AES_ATTR    (0x0004)
#define ONU_PON_SM_ALLOC_ATTR  (0x0008)
#define ONU_PON_SM_OMCC_ATTR   (0x0010)
#define ONU_PON_SM_TIMER_ATTR  (0x0020)
#define ONU_PON_SM_ALARM_ATTR  (0x0040)

#define PON_MAC_MODULE         (1)      
#define PON_INIT_MODULE        (2)     
#define PON_ISR_MODULE         (3)      
#define PON_CLI_MODULE         (4)      
#define PON_MNG_MODULE         (5)  
#define PON_SM_MODULE          (6)
#define PON_SM_DEBUG_MODULE    (6 | (ONU_PON_SM_DEBUG_ATTR << MODULE_SHIFT))  
#define PON_SM_STATE_MODULE    (6 | (ONU_PON_SM_STATE_ATTR << MODULE_SHIFT))  
#define PON_SM_AES_MODULE      (6 | (ONU_PON_SM_AES_ATTR   << MODULE_SHIFT))  
#define PON_SM_ALLOC_MODULE    (6 | (ONU_PON_SM_ALLOC_ATTR << MODULE_SHIFT))  
#define PON_SM_OMCC_MODULE     (6 | (ONU_PON_SM_OMCC_ATTR  << MODULE_SHIFT))  
#define PON_SM_TIMER_MODULE    (6 | (ONU_PON_SM_TIMER_ATTR << MODULE_SHIFT))  
#define PON_SM_ALARM_MODULE    (6 | (ONU_PON_SM_ALARM_ATTR << MODULE_SHIFT))  
#define PON_PM_MODULE	       (7)
#define PON_ALARM_MODULE       (8)    
#define PON_BER_MODULE	       (9)
#define PON_API_MODULE         (10)      
#define PON_ALLOC_MODULE       (11)    
#define PON_LAST_MODULE        (12)    

/* Timers Definitions */
#define ONU_PON_TIMER_T01_INTERVAL           (10000) /* 10 sec */
#define ONU_PON_TIMER_T02_INTERVAL           (100)   /* 100 msec */
#define ONU_PON_TIMER_PM_INTERVAL            (1000)  /* 1 sec */
#define ONU_PON_TIMER_PEE_INTERVAL           (3000)  /* 3 sec */
#define ONU_PON_TIMER_PON_EVT_CLEAN_INTERVAL (5000)  /* 5 sec */
#define ONU_PON_TIMER_XVR_RST_INTERVAL       (10)    /* 10 msec */
#define ONU_PON_TIMER_MPCP_INTERVAL          (1000)  /* 1 sec */

#define ONU_PON_TIMER_ACTIVE                 (1)
#define ONU_PON_TIMER_NOT_ACTIVE             (0)

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Typedefs
------------------------------------------------------------------------------*/
typedef void (*PTIMER_FUNCPTR)(unsigned long data);

/* Timer Id */
typedef struct
{
  struct timer_list onuPonTimerId;
  PTIMER_FUNCPTR    onuPonTimerFunc;
  MV_U32            onuPonTimerActive;
  MV_U32            onuPonTimerInterval; 
  MV_U32            onuPonTimerPeriodic;
}S_OnuPonTimer;

/* Interrupt */
typedef struct
{
  int                   onuPonIrqNum;
  spinlock_t            onuPonIrqLock;
  struct tasklet_struct onuPonTasklet;
}S_onuPonIrq;

/* ONU GPON resource table */
typedef struct
{
  /* Timer */
#ifdef CONFIG_MV_GPON_MODULE
  S_OnuPonTimer onuGponT01_TimerId;       /* ONU GPON T01 timer */
  S_OnuPonTimer onuGponT02_TimerId;       /* ONU GPON T02 timer */
  S_OnuPonTimer onuGponPeeTimerId;        /* ONU GPON PEE timer */
  S_OnuPonTimer onuGponEvtCleanUpTimerId; /* ONU GPON Event Clean Up timer */
  S_OnuPonTimer onuGponIsrXvrRstTimerId;  /* ONU EPON XVR Reset timer */
#else /* CONFIG_MV_EPON_MODULE */
  S_OnuPonTimer onuPonMpcpTimerId;        /* ONU EPON MPCP timer */
#endif /* CONFIG_MV_GPON_MODULE */
  S_OnuPonTimer onuPonPmTimerId;         /* ONU PON PM timer */

  /* Interrupt */
  S_onuPonIrq   onuPonIrqId;           
}S_OnuPonResourceTbl;

/* Printing Options */
typedef struct
{
  MV_U32 modulePrintLevel; /* Severity printing level per module */
  MV_U32 moduleOptions;    /* Optional module printing options */
}S_PonModulePrint;

/* Global variables
------------------------------------------------------------------------------*/
extern S_OnuPonResourceTbl onuPonResourceTbl_s;

/* Global functions
------------------------------------------------------------------------------*/
/* Init API */
extern MV_STATUS onuPonBaseAddrInit(void);
extern MV_STATUS onuPonRtosResourceInit(void);
extern MV_STATUS onuPonRtosResourceRelease(void);

/* Timer API */
extern MV_STATUS onuPonTimerCreate(S_OnuPonTimer *timerId, 
                                   MV_U8         *timerDesc, 
                                   PTIMER_FUNCPTR timerFunc, 
                                   MV_U32         timerParam, 
                                   MV_U32         timerInterval, 
                                   MV_U32         timerPeriodic);
extern int       onuPonTimerEnable(S_OnuPonTimer *timerId);
extern int       onuPonTimerDisable(S_OnuPonTimer *timerId);

/* Memory API */
extern MV_STATUS ponOnuGlbAddrInit(void);
extern MV_STATUS ponOnuGlbAddrSet(MV_U32 mmapBaseAddr);
extern void*     onuPonMemAlloc(unsigned int size); 
extern void      onuPonMemRelease(void); 

/* Interrupt API */
extern MV_STATUS onuPonIrqNumInit(void);
extern MV_STATUS onuPonIrqRegister(S_onuPonIrq *irqId); 
extern MV_STATUS onuPonIrqLockInit(S_onuPonIrq *irqId);
extern MV_STATUS onuPonIrqTaskletInit(S_onuPonIrq *irqId);
extern int       onuPonIrqEnable(S_onuPonIrq *irqId);
extern int       onuPonIrqDisable(S_onuPonIrq *irqId);
extern int       onuPonIrqLock(S_onuPonIrq irqId);
extern int       onuPonIrqUnlock(S_onuPonIrq irqId);

/* Printing API */
extern MV_STATUS mvPonPrint(MV_U32 level, MV_U32 bitMask, const char *format, ...);
extern MV_STATUS ponOnuCheckPrintStatus(MV_U32 printLevel, MV_U32 moduleOptions);
extern MV_STATUS ponOnuChangePrintStatus(MV_U32 module, MV_U32 printLevel, MV_U32 moduleOptions);
extern MV_STATUS ponOnuGetPrintStatus(MV_U32 module, MV_U32 *printLevel, MV_U32 *moduleOptions);
extern int       ponOnuPrintStatus(char* buf);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_LINUX_KS_OS_H */
