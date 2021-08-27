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
**  FILE        : ponOnuAlrm.h                                               **
**                                                                           **
**  DESCRIPTION : This file contains ONU GPON Alarm and Statistics           **
**                                                                           **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef _ONU_GPON_ALRM_H
#define _ONU_GPON_ALRM_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
#define ONU_GPON_ALARM_LOS_LOC   (0x0001) 
#define ONU_GPON_ALARM_LOF_LOC   (0x0002) 
#define ONU_GPON_ALARM_LCDA_LOC  (0x0004)
#define ONU_GPON_ALARM_LCDG_LOC  (0x0008)
#define ONU_GPON_ALARM_SF_LOC    (0x0010)
#define ONU_GPON_ALARM_SD_LOC    (0x0020)
#define ONU_GPON_ALARM_TF_LOC    (0x0040)
#define ONU_GPON_ALARM_SUF_LOC   (0x0080)
#define ONU_GPON_ALARM_MEM_LOC   (0x0100)
#define ONU_GPON_ALARM_DACT_LOC  (0x0200)
#define ONU_GPON_ALARM_DIS_LOC   (0x0400)
#define ONU_GPON_ALARM_MIS_LOC   (0x0800)
#define ONU_GPON_ALARM_PEE_LOC   (0x1000)
#define ONU_GPON_ALARM_RDI_LOC   (0x2000)

#define ONU_GPON_ALARM_DEF_STATE (ONU_GPON_ALARM_LOS_LOC | \
                                  ONU_GPON_ALARM_LOF_LOC | \
                                  ONU_GPON_ALARM_LCDG_LOC)

/* Enums                              
------------------------------------------------------------------------------*/ 
typedef enum 
{
  /* ASIC Alarms */
  ONU_GPON_ALARM_LOS = 0,
  ONU_GPON_ALARM_LOF,
  ONU_GPON_ALARM_LCDA,
  ONU_GPON_ALARM_LCDG,
  /* SW Alarms */
  ONU_GPON_ALARM_SF,
  ONU_GPON_ALARM_SD,
  ONU_GPON_ALARM_TF,
  ONU_GPON_ALARM_SUF,
  ONU_GPON_ALARM_MEM,
  ONU_GPON_ALARM_DACT,
  ONU_GPON_ALARM_DIS,
  ONU_GPON_ALARM_MIS,
  ONU_GPON_ALARM_PEE,
  ONU_GPON_ALARM_RDI,
  ONU_GPON_MAX_ALARMS
}E_OnuGponAlarmType;
  
typedef enum
{
  ONU_GPON_ALARM_OFF,
  ONU_GPON_ALARM_ON
}E_OnuGponAlarmState;

/* Typedefs
------------------------------------------------------------------------------*/

/* ONU GPON Alarm table */
typedef struct
{
  E_OnuGponAlarmState onuGponAlarmTbl[ONU_GPON_NUM_OF_ALARMS];
}S_OnuGponAlarmTbl;

/* ONU GPON Stats table */
typedef struct
{
  MV_U32 onuGponInComingPloamCrcErrCnt;
  MV_U32 onuGponInComingPloamOverflowCnt;
  MV_U32 onuGponOutGoingBwCrcErrCnt;
  MV_U32 onuGponOutGoingBwCrcCorrectErrCnt;
}S_OnuGponStatsTbl;

/* ONU GPON APM table */
typedef struct
{
  S_OnuGponAlarmTbl onuGponAlarmTbl_s;
  S_OnuGponStatsTbl onuGponStatsTbl_s;
}S_OnuGponApmTbl;

/* Global variables
------------------------------------------------------------------------------*/
/* ONU GPON APM table */
extern S_OnuGponApmTbl onuGponApmTbl_s;

/* Global functions
------------------------------------------------------------------------------*/
 
/* ONU GPON Alarm table init function */
void                onuGponAlarmTblInit(void);

/* ONU GPON Alarm table API functions */
MV_STATUS           onuGponAlarmSet(E_OnuGponAlarmType alarm, E_OnuGponAlarmState state);
E_OnuGponAlarmState onuGponAlarmGet(E_OnuGponAlarmType alarm);
E_OnuGponAlarmState onuGponAsicAlarmStatusGet(void);

void                onuGponOnuAlarmShow(void);

/* ONU GPON Stats API functions */
void                onuGponOnuStatsAccmulate(void);
void                onuGponOnuStatsReset(void);
void                onuGponOnuStatsShow(void);
                   
MV_STATUS           onuGponInComingPloamCrcErrSet(MV_U32 count);
MV_U32              onuGponInComingPloamCrcErrGet(void);
MV_STATUS           onuGponInComingPloamOverflowCntSet(MV_U32 count);
MV_U32              onuGponInComingPloamOverflowCntGet(void);
MV_STATUS           onuGponOutGoingBwCrcErrCntSet(MV_U32 count);
MV_U32              onuGponOutGoingBwCrcErrCntGet(void);
MV_STATUS           onuGponOutGoingBwCrcCorrErrCntSet(MV_U32 count);
MV_U32              onuGponOutGoingBwCrcCorrErrCntGet(void);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_GPON_ALRM_H */
