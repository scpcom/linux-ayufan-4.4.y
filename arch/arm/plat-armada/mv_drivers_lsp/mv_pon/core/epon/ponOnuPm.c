/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU GPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuPm.c                                                **/
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
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuPm.c"

/* Global Variables
------------------------------------------------------------------------------*/
S_EponPm g_OnuEponPm[EPON_MAX_MAC_NUM];
S_SwPm   g_OnuEponSwPm[EPON_MAX_MAC_NUM];

/* Global functions
------------------------------------------------------------------------------*/
MV_STATUS onuEponPmSwCountersUpdate(S_SwPm *swPm, MV_U32 macId);

/******************************************************************************/
/* ========================================================================== */
/*                         Statistics Section                                 */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponPmInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init the onu epon pm table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**                   
*******************************************************************************/
void onuEponPmInit(void)
{
  /* reset all counters */
  memset(&g_OnuEponPm,   0, sizeof(S_EponPm) * EPON_MAX_MAC_NUM);
  memset(&g_OnuEponSwPm, 0, sizeof(S_SwPm)   * EPON_MAX_MAC_NUM);
}

/*******************************************************************************
**
**  onuEponPmTimerPmHndl
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is called by the EPON PM timer
**               
**  PARAMETERS:  unsigned long data
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponPmTimerPmHndl(unsigned long data)
{
  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);   /* lock EPON interrupt */

  onuPonResourceTbl_s.onuPonPmTimerId.onuPonTimerActive = ONU_PON_TIMER_NOT_ACTIVE;

  /* Call PM handler */
  onuEponPmTimerExpireHndl();
 
  if ((onuPonResourceTbl_s.onuPonPmTimerId.onuPonTimerPeriodic) != 0) 
    onuPonTimerEnable(&(onuPonResourceTbl_s.onuPonPmTimerId));

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId); /* unlock EPON interrupt */
}

/*******************************************************************************
**
**  onuEponPmTimerExpireHndl
**  ____________________________________________________________________________
** 
**  DESCRIPTION: This routine execute PM handler functionality                          
**                
**  PARAMETERS:  None  
**                   
**  OUTPUTS:     None 
**                   
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponPmTimerExpireHndl(void)
{
  onuEponPmCountersAdd();
}

/*******************************************************************************
**
**  onuEponPmCountersAdd
**  ____________________________________________________________________________
** 
**  DESCRIPTION: This routine read statistics values from ASIC to the database                             
**                 
**  PARAMETERS:  None  
**                   
**  OUTPUTS:     None 
**                   
**  RETURNS:     MV_OK 
**
*******************************************************************************/
MV_STATUS onuEponPmCountersAdd(void)
{
  MV_U32 counter;
  MV_U32 macId;

  /* Rx Counters - One set of counters for all MACs */
  /* ============================================== */
  mvOnuEponMacRxStatsFcsErrorGet(&counter, 0);          /* Count number of received frames with FCS errors */
  g_OnuEponPm[0].rx.fcsErrorFramesCnt += counter;                                                             
  mvOnuEponMacRxStatsShortErrorGet(&counter, 0);        /* Count number of short frames received */
  g_OnuEponPm[0].rx.shortFramesCnt += counter;                                                     
  mvOnuEponMacRxStatsLongErrorGet(&counter, 0);         /* Count number of long frames received */
  g_OnuEponPm[0].rx.longFramesCnt += counter;                                                     
  mvOnuEponMacRxStatsDataFrameGet(&counter, 0);         /* Count number of data frames received */
  g_OnuEponPm[0].rx.dataFramesCnt += counter;                                                                
  mvOnuEponMacRxStatsCtrlFrameGet(&counter, 0);         /* Count number of control frames received */
  g_OnuEponPm[0].rx.ctrlFramesCnt += counter;                       
  mvOnuEponMacRxStatsReportFrameGet(&counter, 0);       /* Count number of report frames received */
  g_OnuEponPm[0].rx.reportFramesCnt += counter;                                                     
  mvOnuEponMacRxStatsGateFrameGet(&counter, 0);         /* Count number of gate frames received */
  g_OnuEponPm[0].rx.gateFramesCnt += counter;    

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {   
    /* Rx Counters */
    /* =========== */
    mvOnuEponMacTxStatsCtrlRegReqFrameGet(&counter, macId);   /* Count number of register request frames transmitted */    
    g_OnuEponPm[macId].tx.ctrlRegReqFramesCnt += counter;                                                                  
    mvOnuEponMacTxStatsCtrlRegAckFrameGet(&counter, macId);   /* Count number of register acknowledge frames transmitted */
    g_OnuEponPm[macId].tx.ctrlRegAckFramesCnt += counter;                                                                  
    mvOnuEponMacTxStatsCtrlReportFrameGet(&counter, macId);   /* Count number of report frames transmitted */              
    g_OnuEponPm[macId].tx.reportFramesCnt += counter;                                                                      
    mvOnuEponMacTxStatsDataFrameGet(&counter, macId);         /* Count number of data frames transmitted */                
    g_OnuEponPm[macId].tx.dataFramesCnt += counter;                                                                        
    mvOnuEponMacTxStatsTxAllowedByteCountGet(&counter, macId) /* Count number of Tx Byte Allow counter */                  ;
    g_OnuEponPm[macId].tx.txAllowedBytesCnt += counter;   

    /* Sw Counters */
    /* =========== */
    onuEponPmSwCountersUpdate(&g_OnuEponPm[macId].sw, macId); 

    /* Gpm Counters */
    /* ============ */
    mvOnuEponMacGpmGrantValidCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.grantValidCnt += counter;                 
    mvOnuEponMacGpmGrantMaxFutureTimeErrorCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.grantMaxFutureTimeErrorCnt += counter;    
    mvOnuEponMacGpmMinProcTimeErrorCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.minProcTimeErrorCnt += counter;          
    mvOnuEponMacGpmLengthErrorCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.lengthErrorCnt += counter;                
    mvOnuEponMacGpmDiscoveryAndRegisterCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.discoveryAndRegCnt += counter;            
    mvOnuEponMacGpmFifoFullErrorCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.fifoFullErrorCnt += counter;              
    mvOnuEponMacGpmOpcDiscoveryNotRegisterBcastCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.opcDiscoveryNotRegBcastCnt += counter;    
    mvOnuEponMacGpmOpcRegisterNotDiscoveryCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.opcRegisterNotDiscoveryCnt += counter;    
    mvOnuEponMacGpmOpcDiscoveryNotRegisterNotBcastCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.opcDiscoveryNotRegNotBcastCnt += counter; 
    mvOnuEponMacGpmOpcDropGrantCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.opcDropGrantCnt += counter;               
    mvOnuEponMacGpmOpcHiddenGrantCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.opcHiddenGrantCnt += counter;             
    mvOnuEponMacGpmOpcBackToBackCounterGet(&counter, macId);
    g_OnuEponPm[macId].gpm.opcBackToBackCnt += counter; 
  }

  return(MV_OK);
}

 /*******************************************************************************
**
**  onuEponPmSwCountersUpdate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function updates software counters
**                
**  PARAMETERS:  S_SwPm *swPm
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmSwCountersUpdate(S_SwPm *swPm, MV_U32 macId)
{
  MV_U32 index;

  /* MAC 0 counts RX statitics for all modules */
  for (index = 0; index < MAX_EPON_RX_SW_CNT; index++)
  {
    swPm->swRxCnt[index] = g_OnuEponSwPm[macId].swRxCnt[index];
  }

  for (index = 0; index < MAX_EPON_TX_SW_CNT; index++)
  {
    swPm->swTxCnt[index] = g_OnuEponSwPm[macId].swTxCnt[index];
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmSwRxCountersAdd
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function updates SW Rx counters
**                
**  PARAMETERS:  MV_U32 cnt 
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmSwRxCountersAdd(MV_U32 cnt, MV_U32 macId)
{
  if (cnt >= MAX_EPON_RX_SW_CNT) 
    return(MV_ERROR);

  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  g_OnuEponSwPm[macId].swRxCnt[cnt]++;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmSwTxCountersAdd
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function updates SW counters
**                
**  PARAMETERS:  MV_U32 cnt 
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmSwTxCountersAdd(MV_U32 cnt, MV_U32 macId)
{
  if (cnt >= MAX_EPON_TX_SW_CNT) 
    return(MV_ERROR);

  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  g_OnuEponSwPm[macId].swTxCnt[cnt]++;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmRxPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The funtion return onu Rx counters
**                 
**  PARAMETERS:  S_RxPm *rxPm
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmRxPmGet(S_RxPm *rxPm, MV_U32 macId)
{
  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  rxPm->fcsErrorFramesCnt = g_OnuEponPm[macId].rx.fcsErrorFramesCnt;
  rxPm->shortFramesCnt    = g_OnuEponPm[macId].rx.shortFramesCnt;   
  rxPm->longFramesCnt     = g_OnuEponPm[macId].rx.longFramesCnt;    
  rxPm->dataFramesCnt     = g_OnuEponPm[macId].rx.dataFramesCnt;    
  rxPm->ctrlFramesCnt     = g_OnuEponPm[macId].rx.ctrlFramesCnt;    
  rxPm->reportFramesCnt   = g_OnuEponPm[macId].rx.reportFramesCnt;  
  rxPm->gateFramesCnt     = g_OnuEponPm[macId].rx.gateFramesCnt;    

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmCtrlCntGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The funtion return onu control packet counters
**                 
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_U32 onuEponPmCtrlCntGet(MV_U32 macId)
{
  return(g_OnuEponPm[macId].rx.ctrlFramesCnt);
}

/*******************************************************************************
**
**  onuEponPmTxPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The funtion return onu Tx counters
**                 
**  PARAMETERS:  S_TxPm *txPm
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmTxPmGet(S_TxPm *txPm, MV_U32 macId)
{
  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  txPm->ctrlRegReqFramesCnt = g_OnuEponPm[macId].tx.ctrlRegReqFramesCnt;
  txPm->ctrlRegAckFramesCnt = g_OnuEponPm[macId].tx.ctrlRegAckFramesCnt;
  txPm->reportFramesCnt     = g_OnuEponPm[macId].tx.reportFramesCnt;    
  txPm->dataFramesCnt       = g_OnuEponPm[macId].tx.dataFramesCnt;      
  txPm->txAllowedBytesCnt   = g_OnuEponPm[macId].tx.txAllowedBytesCnt;  

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmSwPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The funtion return onu SW counters
**                 
**  PARAMETERS:  S_SwPm *swPm
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmSwPmGet(S_SwPm *swPm, MV_U32 macId)
{
  MV_U32 index;

  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  for (index = 0; index < MAX_EPON_RX_SW_CNT; index++)
  {
    swPm->swRxCnt[index] = g_OnuEponPm[macId].sw.swRxCnt[index];
  }

  for (index = 0; index < MAX_EPON_TX_SW_CNT; index++)
  {
    swPm->swTxCnt[index] = g_OnuEponPm[macId].sw.swTxCnt[index];
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmGpmPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The funtion return onu Gpm counters
**                 
**  PARAMETERS:  S_GpmPm *gpmPm
**               MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     MV_OK  
**
*******************************************************************************/
MV_STATUS onuEponPmGpmPmGet(S_GpmPm *gpmPm, MV_U32 macId)
{
  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  gpmPm->grantValidCnt                 = g_OnuEponPm[macId].gpm.grantValidCnt;                
  gpmPm->grantMaxFutureTimeErrorCnt    = g_OnuEponPm[macId].gpm.grantMaxFutureTimeErrorCnt;   
  gpmPm->minProcTimeErrorCnt           = g_OnuEponPm[macId].gpm.minProcTimeErrorCnt;          
  gpmPm->lengthErrorCnt                = g_OnuEponPm[macId].gpm.lengthErrorCnt;               
  gpmPm->discoveryAndRegCnt            = g_OnuEponPm[macId].gpm.discoveryAndRegCnt;           
  gpmPm->fifoFullErrorCnt              = g_OnuEponPm[macId].gpm.fifoFullErrorCnt;             
  gpmPm->opcDiscoveryNotRegBcastCnt    = g_OnuEponPm[macId].gpm.opcDiscoveryNotRegBcastCnt;   
  gpmPm->opcRegisterNotDiscoveryCnt    = g_OnuEponPm[macId].gpm.opcRegisterNotDiscoveryCnt;   
  gpmPm->opcDiscoveryNotRegNotBcastCnt = g_OnuEponPm[macId].gpm.opcDiscoveryNotRegNotBcastCnt;
  gpmPm->opcDropGrantCnt               = g_OnuEponPm[macId].gpm.opcDropGrantCnt;              
  gpmPm->opcHiddenGrantCnt             = g_OnuEponPm[macId].gpm.opcHiddenGrantCnt;            
  gpmPm->opcBackToBackCnt              = g_OnuEponPm[macId].gpm.opcBackToBackCnt;             
                                         
  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPmGpmValidGrantGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The funtion return onu Gpm valid grant counter
**                 
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None  
**
**  RETURNS:     onu Gpm valid grant counter  
**
*******************************************************************************/
MV_U32 onuEponPmGpmValidGrantGet(MV_U32 macId)
{
  if (macId >= EPON_MAX_MAC_NUM)
    return(MV_ERROR);

  return(g_OnuEponPm[macId].gpm.grantValidCnt);
}
