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
**  FILE        : ponOnuIsr.h                                                **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON Interrupt implementation       **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_ISR_H
#define _ONU_GPON_ISR_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
#define ONU_GPON_LOS_ALARM_MASK                 (0x0001)
#define ONU_GPON_LOF_ALARM_MASK                 (0x0002)
#define ONU_GPON_LCDG_ALARM_MASK                (0x0004)
#define ONU_GPON_DS_PLOAM_MASK                  (0x0008)
#define ONU_GPON_CDR_LOL_MASK                   (0x0010)
#define ONU_GPON_BIP_INERVAL_MASK               (0x0020)
#define ONU_GPON_RAM_TEST_GRX_PARITY_ERR_MASK   (0x0040)
#define ONU_GPON_RAM_TEST_GEM_PARITY_ERR_MASK   (0x0080)
#define ONU_GPON_RAM_TEST_GTX_PARITY_ERR_MASK   (0x0100)
#define ONU_GPON_RAM_TEST_UTM_PARITY_ERR_MASK   (0x0200)
#define ONU_GPON_PHY_READY_STATUS_MASK          (0x0400)
#define ONU_GPON_PHY_SIGNAL_DETECT_STATUS_MASK  (0x0800)
#define ONU_GPON_XVR_SIGNAL_DETECT_STATUS_MASK  (0x1000)
                                               
#define ONU_GPON_LOS_ALARM_SHIFT                (0)
#define ONU_GPON_LOF_ALARM_SHIFT                (1)
#define ONU_GPON_LCDG_ALARM_SHIFT               (2)
#define ONU_GPON_DS_PLOAM_SHIFT                 (3)
#define ONU_GPON_CDR_LOL_SHIFT                  (4)
#define ONU_GPON_BIP_INERVAL_SHIFT              (5)
#define ONU_GPON_RAM_TEST_GRX_PARITY_ERR_SHIFT  (6)
#define ONU_GPON_RAM_TEST_GEM_PARITY_ERR_SHIFT  (7)
#define ONU_GPON_RAM_TEST_GTX_PARITY_ERR_SHIFT  (8)
#define ONU_GPON_RAM_TEST_UTM_PARITY_ERR_SHIFT  (9)
#define ONU_GPON_PHY_READY_STATUS_SHIFT         (10)
#define ONU_GPON_PHY_SIGNAL_DETECT_STATUS_SHIFT (11)
#define ONU_GPON_XVR_SIGNAL_DETECT_STATUS_SHIFT (12)

#define ONU_GPON_EVENT_SHIFT                    (16)
                                               
#define ON_GPON_MAX_ALARMS                      (3)

#define ONU_GPON_INT_ALARMS                     (ONU_GPON_LOS_ALARM_MASK |\
                                                 ONU_GPON_LOF_ALARM_MASK |\
                                                 ONU_GPON_LCDG_ALARM_MASK)

#define ONU_GPON_PARITY_ERROR                   (ONU_GPON_RAM_TEST_GRX_PARITY_ERR_MASK  |\
                                                 ONU_GPON_RAM_TEST_GEM_PARITY_ERR_MASK  |\
                                                 ONU_GPON_RAM_TEST_GTX_PARITY_ERR_MASKK |\
                                                 ONU_GPON_RAM_TEST_UTM_PARITY_ERR_MASK)
                                               
#define ONU_GPON_READY_STATUS                   (ONU_GPON_PHY_READY_STATUS_SHIFT         |\
                                                 ONU_GPON_PHY_SIGNAL_DETECT_STATUS_SHIFT |\
                                                 ONU_GPON_XVR_SIGNAL_DETECT_STATUS_SHIFT)

#if 1
#define ONU_GPON_INTERRUPTS                     (ONU_GPON_INT_ALARMS |\
                                                 ONU_GPON_DS_PLOAM_MASK |\
                                                 ONU_GPON_BIP_INERVAL_MASK |\
                                                 ONU_GPON_XVR_SIGNAL_DETECT_STATUS_MASK)
#else
#define ONU_GPON_INTERRUPTS                     (ONU_GPON_INT_ALARMS       |\
                                                 ONU_GPON_PARITY_ERROR     |\
                                                 ONU_GPON_READY_STATUS     |\
                                                 ONU_GPON_DS_PLOAM_MASK    |\
                                                 ONU_GPON_BIP_INERVAL_MASK)*/                                               
#endif

#define ONU_GPON_INTERRUPT_INTERVAL             (10)

/* Enums                              
------------------------------------------------------------------------------*/ 
typedef enum
{
  ONU_GPON_ISR_ALARM,
  ONU_GPON_ISR_PLOAM,
  ONU_GPON_ISR_BIP,
  ONU_GPON_ISR_TYPE_MAX
}E_OnuGponIsrTypes;

/* Typedefs
------------------------------------------------------------------------------*/
typedef struct
{
  MV_U32 interruptReadError;
  MV_U32 sendMessageError[ONU_GPON_ISR_TYPE_MAX];   
  MV_U32 setEventError[ONU_GPON_ISR_TYPE_MAX]; 
  MV_U32 interruptSameStatus[ON_GPON_MAX_ALARMS];
  MV_U32 ploamStatusNoInterrupt;
}S_OnuGponIsrCounters;

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/
MV_STATUS onuGponIsrInit(void);
void      onuGponIsrLowRoutine(MV_U32 *interruptEvent, MV_U32 *interruptStatus);
void      onuGponIsrRoutine(MV_U32 interruptEvent, MV_U32 interruptStatus);
void      onuGponIsrXvrResetTimerHndl(unsigned long data);
MV_STATUS onuGponIsrXvrResetStateSet(MV_BOOL mode);
MV_STATUS onuGponIsrEventCleanTimerStateSet(MV_BOOL mode);
void      onuGponPonMngDebugModeSet(MV_BOOL mode);
MV_BOOL   onuGponPonMngDebugModeGet(void);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_ISR_H */
