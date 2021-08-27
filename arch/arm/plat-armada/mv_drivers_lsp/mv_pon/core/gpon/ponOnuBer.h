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
**  FILE        : ponOnuBer.h                                                **
**                                                                           **
**  DESCRIPTION : This is header file of external interface of OLT           **
**		    	  GPON BER mechnism                                          **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/
#ifndef PON_ONU_BER_H
#define PON_ONU_BER_H

/* Include Files
------------------------------------------------------------------------------*/

/* Definitions
------------------------------------------------------------------------------*/

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Structs
------------------------------------------------------------------------------*/
typedef struct
{
  MV_U32 periodsCounter;
  MV_U32 intervalBip8ErrorsCounter;
  MV_U32 ber;
  MV_U32 newBer;
  MV_U32 outBip8Counter;
  MV_U32 intervalBandwidth;
}S_PonBer;

typedef struct
{
  MV_U32 numerator;
  MV_U32 denominator;
}S_BerCoefficient;

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

/* Prototypes
------------------------------------------------------------------------------*/  
MV_STATUS        onuGponBerInit(S_BerCoefficient *coefficient, MV_U32 interval);
MV_STATUS        onuGponBerPeriodPass(MV_U32 inbip8Counter);
MV_STATUS        onuGponBerBerCaculate(MV_U32 inbip8Counter);
MV_STATUS        onuGponBerAlarmsCheck(MV_U32 ber);
void             onuGponBerClear(void); 
MV_U32           onuGponBerBip8CounterGet(void);
MV_U32           onuGponBerLastIntervalBip8CounterGet(void);
MV_U32           onuGponBerBerValueGet(void);
MV_U32           onuGponBerIntervalBerValueGet(void);
S_BerCoefficient onuGponBerCoefficientGet(void);
MV_U32           onuGponBerIntervalSecondsGet(void);
void             onuGponBerCoefficientSet(S_BerCoefficient *coefficient);
void             onuGponBerIntervalSet(MV_U32 interval);
MV_U32           onuGponBerPower(MV_U32 x, MV_U32 y);

#endif /* PON_ONU_BER_H */
