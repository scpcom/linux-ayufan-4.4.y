/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuAlrm.c                                              **/
/**                                                                          **/
/**  DESCRIPTION : This file implements ONU EPON Alarm and Statistics        **/
/**                functionality                                             **/
/**                                                                          **/
/******************************************************************************
 *                                                                            *                              
 *  MODIFICATION HISTORY:                                                     *
 *                                                                            *
 *   26Jan10  oren_ben_hayun    created                                       *  
 * ========================================================================== *      
 *                                                                     
 ******************************************************************************/

/* Include Files
------------------------------------------------------------------------------*/
#include "ponOnuHeader.h"

/* Local Constant
------------------------------------------------------------------------------*/                                               
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuAlrm.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/
MV_U32 onuEponPreviousAlarm = 0;
MV_U32 onuEponCurrentAlarm  = 0;  

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
void onuEponAlarmProcess(void);

/******************************************************************************/
/* ========================================================================== */
/*                         Alarm Section                                      */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponAlarmSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu epon alarm 
**               
**  PARAMETERS:  MV_U32  alarm
**               MV_BOOL status
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK 
**                   
*******************************************************************************/
MV_STATUS onuEponAlarmSet(MV_U32 alarm, MV_BOOL state)
{
  if ((alarm & ONU_EPON_ALARM_MASK) == 0)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_ALARM_MODULE, 
               "ERROR: (%s:%d) invalid alarm(%d) type\n\r", __FILE_DESC__, __LINE__, alarm);
    return(MV_ERROR);
  }

  if (state == MV_FALSE)
  {
    onuEponCurrentAlarm &= ~(alarm);
  }
  else
  {
    onuEponCurrentAlarm |=  (alarm);
  }

  onuEponAlarmProcess();

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponAlarmProcess
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function process the current alarm state and notify screen
**               
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuEponAlarmProcess(void)
{
  MV_U32 onuEponPreviousAlarmState;
  MV_U32 onuEponCurrentAlarmState;
  MV_U32 onuEponChangeAlarm;

  /* get the ASIC prvious alarm status */
  onuEponPreviousAlarmState = onuEponPreviousAlarm;

  /* get the ASIC current alarm status */
  onuEponCurrentAlarmState = onuEponCurrentAlarm;

  /* alarm changed */
  if ((onuEponCurrentAlarmState ^ onuEponPreviousAlarmState) != 0)
  {
    onuEponChangeAlarm = (onuEponCurrentAlarmState ^ onuEponPreviousAlarmState);

    if ((onuEponChangeAlarm & onuEponCurrentAlarmState) != 0)
    {
      if (onuEponChangeAlarm & ONU_EPON_XVR_SD_MASK)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === XVR SD On ===\n\r", __FILE_DESC__, __LINE__);
      }

      if (onuEponChangeAlarm & ONU_EPON_SERDES_SD_MASK)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === SERDES SD On ===\n\r", __FILE_DESC__, __LINE__);
      }
    }
    else /* ((onuEponChangeAlarm & onuEponCurrentAlarmState) == 0) */
    {
      if (onuEponChangeAlarm & ONU_EPON_XVR_SD_MASK)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === XVR SD Off ===\n\r", __FILE_DESC__, __LINE__);
      }

      if (onuEponChangeAlarm & ONU_EPON_SERDES_SD_MASK)
      {
        mvPonPrint(PON_PRINT_INFO, PON_ALARM_MODULE, 
                   "INFO: (%s:%d) === SERDES SD Off ===\n\r", __FILE_DESC__, __LINE__);
      }
    }

    onuEponPreviousAlarm = onuEponCurrentAlarmState;  
  }
}

/*******************************************************************************
**
**  onuEponAlarmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return current alarm status
**               
**  PARAMETERS:  MV_U32 *alarm
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuEponAlarmGet(MV_U32 *alarm)
{
  alarm[0] = ((onuEponCurrentAlarm & ONU_EPON_XVR_SD_MASK)    != 0) ? (1) : (0); 
  alarm[1] = ((onuEponCurrentAlarm & ONU_EPON_SERDES_SD_MASK) != 0) ? (1) : (0);
}
