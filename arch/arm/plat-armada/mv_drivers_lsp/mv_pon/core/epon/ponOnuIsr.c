/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuIsr.c                                               **/
/**                                                                          **/
/**  DESCRIPTION : This file implements ONU EPON Interrupt handling          **/
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
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuIsr.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/
MV_U32 onuEponCurrentInterrupt = 0;
MV_U32 onuEponGpmMacCtrlPackets[EPON_MAX_MAC_NUM];
MV_U32 onuEponGpmMacState[EPON_MAX_MAC_NUM];
MV_U32 onuEponRxMacCtrlPackets;
MV_U32 onuEponRxMacState;

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/

/*******************************************************************************
**
**  onuEponIsrInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init the onu epon isr mpcp timer table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuEponIsrInit(void)
{
  memset(&onuEponGpmMacCtrlPackets[0], 0, sizeof(MV_U32) * EPON_MAX_MAC_NUM);
  memset(&onuEponGpmMacState[0],       0, sizeof(MV_U32) * EPON_MAX_MAC_NUM);

  onuEponRxMacCtrlPackets = 0;
  onuEponRxMacState       = 0;     
}

/*******************************************************************************
**
**  onuEponIsrLowRoutine
**                   
*******************************************************************************/
void onuEponIsrLowRoutine(MV_U32 *interruptEvent, MV_U32 *interruptStatus)
{
  MV_U32 interrupt;

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);    /* lock EPON interrupt from timer context */

  mvOnuEponMacPonInterruptGet(&interrupt);
  onuEponCurrentInterrupt &= 0xFFFF0000;
  onuEponCurrentInterrupt |= interrupt;
 
  *interruptEvent  = (onuEponCurrentInterrupt >> ONU_EPON_EVENT_SHIFT) & ONU_EPON_INTERRUPTS;
  *interruptStatus =  onuEponCurrentInterrupt                          & ONU_EPON_INTERRUPTS;

  onuEponCurrentInterrupt = 0;

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock EPON interrupt from timer context */
}

/*******************************************************************************
**
**  onuEponIsrRoutine
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function implements interrupt handler
**               
**  PARAMETERS:  None, void* param 
**
**  OUTPUTS:     None
**
**  RETURNS:     None, void* 
**                   
*******************************************************************************/
void onuEponIsrRoutine(MV_U32 event, MV_U32 status)
{
  MV_BOOL state;
  MV_U32  interruptEvent  = 0;
  MV_U32  interruptStatus = 0;

#ifdef MV_EPON_HW_INTERRUPT
  onuEponIsrLowRoutine(&interruptEvent, &interruptStatus);
#else
  interruptEvent  = event; 
  interruptStatus = status;
#endif /* MV_GPON_HW_INTERRUPT */

  /* Interrupt Protect */
  /* ================= */
  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);    /* lock EPON interrupt from timer context */

  /* Event is triggered for each change in the Status  */
  /* Status == 0, means there is something in the FIFO */
  /* Status == 1, means there is nothing in the FIFO   */
  /* ================================================= */
  if (interruptEvent & ONU_EPON_XVR_SD_MASK)
  {
    state = (interruptStatus & ONU_EPON_XVR_SD_MASK) ? MV_TRUE : MV_FALSE;
    onuEponPonMngIntrAlarmHandler(ONU_EPON_XVR_SD_MASK, state);
  }

  if (interruptEvent & ONU_EPON_SERDES_SD_MASK)
  {
    state = (interruptStatus & ONU_EPON_SERDES_SD_MASK) ? MV_TRUE : MV_FALSE;
    onuEponPonMngIntrAlarmHandler(ONU_EPON_SERDES_SD_MASK, state);
  }

  if (interruptEvent & ONU_EPON_TIMESTAMP_DRIFT_MASK)
  {
    state = (interruptStatus & ONU_EPON_TIMESTAMP_DRIFT_MASK) ? MV_TRUE : MV_FALSE;
    onuEponPonMngIntrAlarmHandler(ONU_EPON_TIMESTAMP_DRIFT_MASK, state);
  }

  if ((interruptEvent   & ONU_EPON_RX_CTRL_QUEUE_MASK) &&
      ((interruptStatus & ONU_EPON_RX_CTRL_QUEUE_MASK) == 0))
  {
    onuEponPonMngIntrMessageHandler(ONU_EPON_RX_CTRL_MSG);
  }

  if ((interruptEvent   & ONU_EPON_RX_RPRT_QUEUE_MASK) &&
      ((interruptStatus & ONU_EPON_RX_RPRT_QUEUE_MASK) == 0))
  {
    onuEponPonMngIntrMessageHandler(ONU_EPON_RX_RPRT_MSG);
  }

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock EPON interrupt from timer context */
}

/*******************************************************************************
**
**  onuGponIsrTimerMpcpHndl
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function validate mpcp frames arrival rate
**               
**  PARAMETERS:  unsigned long data
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuGponIsrTimerMpcpHndl(unsigned long data)
{
  MV_STATUS status;
  MV_U32    macId;
  MV_U32    currentCtrlPackets;
  MV_U32    executeMpcpTimerExpire = 0;
  MV_U32    checkGpmTimerExpire    = 0;

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId); /* lock GPON interrupt */

  onuPonResourceTbl_s.onuPonMpcpTimerId.onuPonTimerActive = ONU_PON_TIMER_NOT_ACTIVE;

  /* ==================================== */
  /* = Rx Ctrl Packet Section - All MAC = */
  /* ==================================== */
  executeMpcpTimerExpire = 0;
  currentCtrlPackets     = onuEponPmCtrlCntGet(0);

  /* No Rx packets received and state is registered */
  if ((currentCtrlPackets == onuEponRxMacCtrlPackets) &&
      (onuEponRxMacState  == 0))
  {
    onuEponRxMacState      = 1; /* set mac state as NOT Registered */
    executeMpcpTimerExpire = 1; /* set mac expire flag as active */
  }
  /* Rx packets received */
  else if (currentCtrlPackets != onuEponRxMacCtrlPackets) 
  {
    onuEponRxMacState   = 0; /* set mac state as Registered */
    checkGpmTimerExpire = 1;
  }

  /* mac expire flag is active - re-init mac */
  if (executeMpcpTimerExpire != 0)
  {
    for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
    {
      if (onuEponDbOnuStateGet(macId) == ONU_EPON_03_OPERATION)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_ISR_MODULE,
                   "= ONU mac[%d] out of Register State(Rx Ctrl) =\n\r", macId);

        /* re-init onu database */
        status = onuEponDbReInit(macId);
        if (status != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_ISR_MODULE,
                     "ERROR: (%s:%d) Failed to re-init onu database\n\r", __FILE_DESC__, __LINE__);
          return;
        }

        /* init onu Asic */
        status = onuEponAsicReInit(macId);
        if (status != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_ISR_MODULE,
                     "ERROR: (%s:%d) Failed to re-init onu EPON MAC\n\r", __FILE_DESC__, __LINE__);
          return;
        }
      }
    }
  }

  onuEponRxMacCtrlPackets = currentCtrlPackets;

#ifndef MV_EPON_SINGLE_MAC_MODE
  /* =========================================== */
  /*  = GPM ctrl packet section - MAC Specific = */
  /* =========================================== */

  /* Rx ctrl packet is valid - check GPM counter per MAC */
  if (checkGpmTimerExpire == 1)
  {
    for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
    {
      executeMpcpTimerExpire = 0;
      currentCtrlPackets     = onuEponPmGpmValidGrantGet(macId);

      /* No GPM packets received and state is registered */
      if ((currentCtrlPackets        == onuEponGpmMacCtrlPackets[macId]) &&
          (onuEponGpmMacState[macId] == 0))
      {
        onuEponGpmMacState[macId] = 1; /* set mac state as NOT Registered */
        executeMpcpTimerExpire    = 1; /* set mac expire flag as active */
      }
      /* GPM packets received */
      else if (currentCtrlPackets != onuEponGpmMacCtrlPackets[macId]) 
      {
        onuEponGpmMacState[macId] = 0; /* set mac state as Registered */
      }

      /* mac expire flag is active - re-init mac */
      if (executeMpcpTimerExpire != 0)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_ISR_MODULE,
                   "= ONU mac[%d] out of Register State (GPM Valid Grant)=\n\r", macId);

        /* re-init onu database */
        status = onuEponDbReInit(macId);
        if (status != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_ISR_MODULE,
                     "ERROR: (%s:%d) Failed to re-init onu database\n\r", __FILE_DESC__, __LINE__);
          return;
        }

        /* init onu Asic */
        status = onuEponAsicReInit(macId);
        if (status != MV_OK)
        {
          mvPonPrint(PON_PRINT_ERROR, PON_ISR_MODULE,
                     "ERROR: (%s:%d) Failed to re-init onu EPON MAC\n\r", __FILE_DESC__, __LINE__);
          return;
        }
      }

      onuEponGpmMacCtrlPackets[macId] = currentCtrlPackets;
    }
  }
#endif /* MV_EPON_SINGLE_MAC_MODE */

  if ((onuPonResourceTbl_s.onuPonMpcpTimerId.onuPonTimerPeriodic) != 0) 
    onuPonTimerEnable(&(onuPonResourceTbl_s.onuPonMpcpTimerId));

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock GPON interrupt */
}
