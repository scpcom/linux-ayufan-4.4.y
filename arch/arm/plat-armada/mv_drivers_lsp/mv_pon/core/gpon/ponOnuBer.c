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
**  FILE        : ponOnuBer.c                                                **
**                                                                           **
**  DESCRIPTION : This file implements the OLT GPON Bit Error Rate Mechanism **
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
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuBer.c"

#define PON_BER_PERIOD_LENGTH  (ONU_PON_TIMER_PM_INTERVAL)

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/
MV_U32           g_numsOfPeriodInInterval;
MV_U32           g_intervalSeconds;
S_BerCoefficient g_berCoefficient;
S_PonBer         onuBer;

/* Local Prototypes
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/

/*******************************************************************************
**
**  onuGponBerInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu BER parameters
**                 
**  PARAMETERS:  S_BerCoefficient coefficient - the perdentage of the current BER in overall BER
**			     MV_U32			  interval    - the interval of BER measures (seconds)    
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuGponBerInit(S_BerCoefficient *coefficient, MV_U32 interval)
{
  /* Keep how many periods pass before BER is checked */
  g_numsOfPeriodInInterval     = (interval * 1000) / PON_BER_PERIOD_LENGTH;
  g_intervalSeconds            = interval;
  g_berCoefficient.numerator   = coefficient->numerator;
  g_berCoefficient.denominator = coefficient->denominator;

  onuGponBerClear();

  return(MV_OK);
}     

/*******************************************************************************
**
**  onuGponBerPeriodPass
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function calculate BER if interval expired
**                 
**  PARAMETERS:  S_BerCoefficient coefficient - the perdentage of the current BER in overall BER
**			     MV_U32			  interval    - the interval of BER measures (seconds)    
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuGponBerPeriodPass(MV_U32 inbip8Counter)
{
  onuBer.periodsCounter++;
  onuBer.intervalBandwidth += ONU_GPON_DS_DEF_RATE;

  /* Check if is BER caculating Time */
  if (onuBer.periodsCounter < g_numsOfPeriodInInterval)
  {
    /* Do nothing */
    return(MV_OK);
  }
  else
  {
    /* Need to calculate BER */
    onuGponBerBerCaculate(inbip8Counter);
    onuBer.periodsCounter = 0;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponBerBerCaculate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: This routine Calculate the BER
**				 NIOS doen't support floating-point, so we need to pass that limit:
**				 We use this method OvarallBER(n) = Coefficient * IntervalBer(n) + 
**				 								  (1 - Coefficient) * OvarallBER(n-1)
**				 Coefficient is structure of ( numerator / denominator )
**				 IntervalBer is Bip8Counter(Interval)/OverallBits(Interval)
**				 Since the lowest BER we'd like to measure is 10^(-10) (standard lowest 
**				 value to Cacel SD condition), we shell multiple all values by 10^(10).
**				 The thresholds also will be multiple by 10^(10).
**				 The Ber calculation will be as the following (for 1.244 upstream rate):
**
**                  Bip8Counter(Interval) x 10^11     Bip8Counter(Interval) x 10^11
**				 IntervalBer(n) = ---------------------------- = ---------------------------------
**				                      OverallBits(Interval)        actual bits passed(Interval)
**
**				                    Bip8Counter(Interval) x 10^5
**				                = --------------------------------
**                              Mbits 
**
**				                 (IntervalBer(n) x Coef.numerator) + (OverAllBer(n-1) x (Coef.denominator - Coef.numerator))
**				 OverAllBer(n) = -------------------------------------------------------------------------------------------
**				                                                    Coef.denominator          
**
**  PARAMETERS:	 MV_U32 inbip8Counter
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuGponBerBerCaculate(MV_U32 inbip8Counter)
{
  onuBer.intervalBip8ErrorsCounter = inbip8Counter - onuBer.outBip8Counter;
  onuBer.newBer = (onuBer.intervalBip8ErrorsCounter * 100000) / onuBer.intervalBandwidth;

  onuBer.ber = ((g_berCoefficient.numerator * onuBer.newBer) +
                (( g_berCoefficient.denominator - g_berCoefficient.numerator ) * onuBer.ber))
               / g_berCoefficient.denominator;

  onuGponBerAlarmsCheck(onuBer.ber);
  onuBer.outBip8Counter = inbip8Counter;
  onuBer.intervalBandwidth = 0;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponBerAlarmsCheck
**  ____________________________________________________________________________
** 
**  DESCRIPTION: This routine check the Ber Value and decide whether there is a
**				 change in the alarms SD and SF condistion. 
**               
**  PARAMETERS:	 MV_U32 ber
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuGponBerAlarmsCheck(MV_U32 ber)
{
  MV_U32  sdThreshold;
  MV_U32  sfThreshold;
  MV_U32  sdDetectValue;
  MV_U32  sdCancelValue;
  MV_U32  sfDetectValue;
  MV_U32  sfCancelValue;
  MV_BOOL onuSd;
  MV_BOOL onuSf;
  MV_BOOL sd = MV_FALSE;
  MV_BOOL sf = MV_FALSE;

  /* Get curent alarm condition */
  onuSf         = onuGponAlarmGet(ONU_GPON_ALARM_SF);
  onuSd         = onuGponAlarmGet(ONU_GPON_ALARM_SD);
               
  sfThreshold   = onuGponDbSfThresholdGet();
  sdThreshold   = onuGponDbSdThresholdGet();

  sdDetectValue = onuGponBerPower(10 ,(11 - sdThreshold));
  sdCancelValue = onuGponBerPower(10 ,(11 - sdThreshold - 1));
  sfDetectValue = onuGponBerPower(10 ,(11 - sfThreshold));
  sfCancelValue = onuGponBerPower(10 ,(11 - sfThreshold - 1));

  /* check Detect Alarm */
  if ( ber >= sfDetectValue)
  {
    sf = MV_TRUE;
  }
  else if (ber >= sdDetectValue)
  {
    sd = MV_TRUE;
  }

  /* check Cancel Alarm */
  if ( ber < sdCancelValue)
  {
    sd = MV_FALSE;
  }
  else if (ber < sfCancelValue)
  {
    sf = MV_FALSE;
  }

  if ((onuSf == MV_FALSE) && (onuSd == MV_FALSE))
  {
    if ((sd == MV_FALSE) && (sf == MV_TRUE))
    {
      onuGponAlarmSet(ONU_GPON_ALARM_SF,MV_TRUE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SF,MV_TRUE);
    }
    else if ((sd == MV_TRUE) && (sf == MV_FALSE))
    {
      onuGponAlarmSet(ONU_GPON_ALARM_SD,MV_TRUE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SD,MV_TRUE);
    }
  }
  else if ((onuSf == MV_TRUE) && (onuSd == MV_FALSE))
  {
    if ((sd == MV_FALSE) && (sf == MV_FALSE))
    {
      onuGponAlarmSet(ONU_GPON_ALARM_SF,MV_FALSE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SF,MV_FALSE);
    }
    else if ((sd == MV_TRUE) && (sf == MV_FALSE))
    {
      onuGponAlarmSet(ONU_GPON_ALARM_SD,MV_TRUE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SD,MV_TRUE);

      onuGponAlarmSet(ONU_GPON_ALARM_SF,MV_FALSE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SF,MV_FALSE);
    }
  }
  else if ((onuSf == MV_FALSE) && (onuSd == MV_TRUE))
  {
    if ((sd == MV_FALSE) && (sf == MV_FALSE))
    {
      onuGponAlarmSet(ONU_GPON_ALARM_SD,MV_FALSE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SD,MV_FALSE);
    }
    else if ((sd == MV_FALSE) && (sf == MV_TRUE))
    {
      onuGponAlarmSet(ONU_GPON_ALARM_SD,MV_FALSE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SD,MV_FALSE);

      onuGponAlarmSet(ONU_GPON_ALARM_SF,MV_TRUE);
      onuGponSrvcAlarmNotify(ONU_GPON_ALARM_SF,MV_TRUE);
    }
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponBerClear
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function clear all BER counters and thresholds  
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     None
**
*******************************************************************************/
void onuGponBerClear(void)
{
  onuBer.periodsCounter            = 0;
  onuBer.intervalBip8ErrorsCounter = 0;
  onuBer.ber                       = 0;
  onuBer.newBer                    = 0;
  onuBer.outBip8Counter            = 0;
  onuBer.intervalBandwidth         = 0;

  onuGponAlarmSet(ONU_GPON_ALARM_SD, MV_FALSE);
  onuGponAlarmSet(ONU_GPON_ALARM_SF, MV_FALSE);
}

/*******************************************************************************
**
**  onuGponBerBip8CounterGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return bip8 counter 
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     bip8 counter
**
*******************************************************************************/
MV_U32 onuGponBerBip8CounterGet(void)
{
  MV_U32     counter;
  S_RxBip8Pm inBip8Pm;

  onuGponPmRxBip8PmGet(&inBip8Pm);
  counter = inBip8Pm.bip8 - onuBer.outBip8Counter;

  return(counter); 
}

/*******************************************************************************
**
**  onuGponBerLastIntervalBip8CounterGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return bip8 counter of the last interval
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     bip8 counter
**
*******************************************************************************/
MV_U32 onuGponBerLastIntervalBip8CounterGet(void)
{
  MV_U32 counter;

  counter = onuBer.intervalBip8ErrorsCounter;

  return(counter); 
}

/*******************************************************************************
**
**  onuGponBerBerValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu ber
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     onu ber
**
*******************************************************************************/
MV_U32 onuGponBerBerValueGet(void)
{
  return(onuBer.ber); 
}

/*******************************************************************************
**
**  onuGponBerIntervalBerValueGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return ber interval value
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     ber interval
**
*******************************************************************************/
MV_U32 onuGponBerIntervalBerValueGet(void)
{
  return(onuBer.newBer); 
}

/*******************************************************************************
**
**  onuGponBerCoefficientGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return ber coefficient
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     ber coefficient
**
*******************************************************************************/
S_BerCoefficient onuGponBerCoefficientGet(void)
{
  return(g_berCoefficient); 
}

/*******************************************************************************
**
**  onuGponBerIntervalSecondsGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return ber interval in seconds
**               
**  PARAMETERS:	 None
**
**  OUTPUTS:     None 
**
**  RETURNS:     ber interval in seconds
**
*******************************************************************************/
MV_U32 onuGponBerIntervalSecondsGet(void)
{
  return(g_intervalSeconds); 
}

/*******************************************************************************
**
**  onuGponBerCoefficientSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set ber coefficient
**               
**  PARAMETERS:	 S_BerCoefficient *coefficient
**
**  OUTPUTS:     None 
**
**  RETURNS:     None
**
*******************************************************************************/
void onuGponBerCoefficientSet(S_BerCoefficient *coefficient)
{
  g_berCoefficient.numerator   = coefficient->numerator;
  g_berCoefficient.denominator = coefficient->denominator;
}

/*******************************************************************************
**
**  onuGponBerIntervalSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set ber interval
**               
**  PARAMETERS:	 MV_U32 interval
**
**  OUTPUTS:     None 
**
**  RETURNS:     None
**
*******************************************************************************/
void onuGponBerIntervalSet(MV_U32 interval)
{
  g_intervalSeconds        = interval;
  g_numsOfPeriodInInterval = (interval * 1000) / PON_BER_PERIOD_LENGTH;
  onuGponDbBerCalcIntervalSet(interval);
}

/*******************************************************************************
**
**  onuGponBerPower
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is a BER utility function
**               
**  PARAMETERS:	 MV_U32 x
**               MV_U32 y
**
**  OUTPUTS:     None
**
**  RETURNS:     calculated value  
**
*******************************************************************************/
MV_U32 onuGponBerPower(MV_U32 x, MV_U32 y)
{
  MV_U32  i;
  MV_U32  sum = 1;

  for (i = 0 ; i < y ; i++)
  {
    sum = sum*x;
  }

  return(sum); 
}
