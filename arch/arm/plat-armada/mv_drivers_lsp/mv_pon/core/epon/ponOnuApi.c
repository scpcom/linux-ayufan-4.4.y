/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuApi.c                                               **/
/**                                                                          **/
/**  DESCRIPTION : This file implements ONU EPON API functionality           **/
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
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuApi.c"

/* Global Variables
------------------------------------------------------------------------------*/
S_EponPm g_OnuEponOutPm[EPON_MAX_MAC_NUM];

/* Local Variables
------------------------------------------------------------------------------*/

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions 
------------------------------------------------------------------------------*/

/* Typedefs
------------------------------------------------------------------------------*/

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

/*******************************************************************************
**
**  onuPonApiStatusNotifyRegister
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function register status callback function
**               
**  PARAMETERS:  STATUSNOTIFYFUNC notifyCallBack 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuPonApiStatusNotifyRegister(STATUSNOTIFYFUNC notifyCallBack)
{
  MV_STATUS status;

  status = onuEponDbStatusNotifySet(notifyCallBack);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) ponOnuApiStatusNotifyRegister", __FILE_DESC__, __LINE__); 
    return(status);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiInformationGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu info
**               
**  PARAMETERS:  S_IoctlInfo *onuInfo 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponApiInformationGet(S_EponIoctlInfo *onuInfo, MV_U32 macId)
{
  MV_U32 addressLow;
  MV_U32 addressHigh;

  onuInfo->onuEponState    = onuEponDbOnuStateGet(macId);      
  onuInfo->onuEponCtrlType = onuEponDbOnuCtrlTypeGet();   

  onuEponDbOnuMacAddrGet(&addressLow, &addressHigh, macId);
  onuInfo->onuEponMacAddr[5] = ((addressLow  >>  0) & 0xFF);
  onuInfo->onuEponMacAddr[4] = ((addressLow  >>  8) & 0xFF); 
  onuInfo->onuEponMacAddr[3] = ((addressLow  >> 16) & 0xFF);
  onuInfo->onuEponMacAddr[2] = ((addressLow  >> 24) & 0xFF);
  onuInfo->onuEponMacAddr[1] = ((addressHigh >>  0) & 0xFF);
  onuInfo->onuEponMacAddr[0] = ((addressHigh >>  8) & 0xFF);

  onuEponDbOnuBcastAddrGet(&addressLow, &addressHigh);
  onuInfo->onuEponBcastAddr[5] = ((addressLow  >>  0) & 0xFF);
  onuInfo->onuEponBcastAddr[4] = ((addressLow  >>  8) & 0xFF); 
  onuInfo->onuEponBcastAddr[3] = ((addressLow  >> 16) & 0xFF);
  onuInfo->onuEponBcastAddr[2] = ((addressLow  >> 24) & 0xFF);
  onuInfo->onuEponBcastAddr[1] = ((addressHigh >>  0) & 0xFF);
  onuInfo->onuEponBcastAddr[0] = ((addressHigh >>  8) & 0xFF);

  onuEponDbPktRxLlidGet(&(onuInfo->onuEponRxLLID), macId);
  onuEponDbPktTxLlidGet(&(onuInfo->onuEponTxLLID), macId);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiLinkStateGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu link state
**               
**  PARAMETERS:  MV_U32 *linkState 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponApiLinkStateGet(MV_U32 *linkState, MV_U32 macId)
{
  if (onuEponDbOnuStateGet(macId) == ONU_EPON_03_OPERATION) *linkState = 1;
  else                                                      *linkState = 0;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiFecConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu FEC
**               
**  PARAMETERS:  MV_U32 rxGenFecEn
**               MV_U32 txGenFecEn
**               MV_U32 txMacFecEn[8]
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponApiFecConfig(MV_U32 rxGenFecEn, MV_U32 txGenFecEn, MV_U32 *txMacFecEn)
{
  MV_STATUS status;
  MV_U32    regReqAutoRes;
  MV_U32    regAckAutoRes;
  MV_U32    reportAutoRes;
  MV_U32    rxGenFecEnTemp;
  MV_U32    txGenFecEnTemp;   
  MV_U32    txMacFecEnTemp;   

  onuEponDbOnuCfgGet(&regReqAutoRes, &regAckAutoRes, &reportAutoRes, &rxGenFecEnTemp, &txGenFecEnTemp, &txMacFecEnTemp);
  rxGenFecEnTemp = rxGenFecEn;
  txGenFecEnTemp = txGenFecEn;
  txMacFecEnTemp = ((txMacFecEn[0] & 0x1) << 0) |
                   ((txMacFecEn[1] & 0x1) << 1) |
                   ((txMacFecEn[2] & 0x1) << 2) |
                   ((txMacFecEn[3] & 0x1) << 3) |
                   ((txMacFecEn[4] & 0x1) << 4) |
                   ((txMacFecEn[5] & 0x1) << 5) |
                   ((txMacFecEn[6] & 0x1) << 6) |
                   ((txMacFecEn[7] & 0x1) << 7);
  onuEponDbOnuCfgSet(regReqAutoRes, regAckAutoRes, reportAutoRes, rxGenFecEnTemp, txGenFecEnTemp, txMacFecEnTemp);

  status = mvOnuEponMacGenOnuConfigSet(rxGenFecEnTemp, txGenFecEnTemp, reportAutoRes, regAckAutoRes, regReqAutoRes, txMacFecEnTemp);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiFecConfig", __FILE_DESC__, __LINE__); 
    return(status);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiEncryptionConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu Encryption
**               
**  PARAMETERS:  MV_U32 onuEncryptCfg
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponApiEncryptionConfig(MV_U32 onuEncryptCfg)
{
  MV_STATUS status;

  status = mvOnuEponMacRxpEncConfigSet(onuEncryptCfg);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiEncryptionConfig", __FILE_DESC__, __LINE__); 
    return(status);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiEncryptionKeyConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu Encryption Key
**               
**  PARAMETERS:  MV_U32 encryptKeyIndex0
**               MV_U32 encryptKeyIndex1
**              MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponApiEncryptionKeyConfig(MV_U32 encryptKeyIndex0, MV_U32 encryptKeyIndex1, MV_U32 macId)
{
  MV_STATUS status;

  status = mvOnuEponMacRxpEncKeySet(encryptKeyIndex0, 0, macId); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiEncryptionKeyConfig", __FILE_DESC__, __LINE__); 
    return(status);
  }

  status = mvOnuEponMacRxpEncKeySet(encryptKeyIndex1, 1, macId); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiEncryptionKeyConfig", __FILE_DESC__, __LINE__); 
    return(status);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiRxPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Rx counters                           
**                
**  PARAMETERS:  S_EponIoctlRxPm *rxPm
**               MV_BOOL         clear
**               MV_U32          macId
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuEponApiRxPmGet(S_EponIoctlRxPm *rxPm,  MV_BOOL clear, MV_U32 macId)
{
  MV_STATUS status;
  S_RxPm    inCounters;

  status = onuEponPmRxPmGet(&inCounters, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiRxPmGet", __FILE_DESC__, __LINE__); 
    return(status);
  }

  rxPm->fcsErrorFramesCnt = inCounters.fcsErrorFramesCnt - g_OnuEponOutPm[macId].rx.fcsErrorFramesCnt;
  rxPm->shortFramesCnt    = inCounters.shortFramesCnt    - g_OnuEponOutPm[macId].rx.shortFramesCnt;
  rxPm->longFramesCnt     = inCounters.longFramesCnt     - g_OnuEponOutPm[macId].rx.longFramesCnt; 
  rxPm->dataFramesCnt     = inCounters.dataFramesCnt     - g_OnuEponOutPm[macId].rx.dataFramesCnt; 
  rxPm->ctrlFramesCnt     = inCounters.ctrlFramesCnt     - g_OnuEponOutPm[macId].rx.ctrlFramesCnt; 
  rxPm->reportFramesCnt   = inCounters.reportFramesCnt   - g_OnuEponOutPm[macId].rx.reportFramesCnt;  
  rxPm->gateFramesCnt     = inCounters.gateFramesCnt     - g_OnuEponOutPm[macId].rx.gateFramesCnt;    

  if (clear == MV_TRUE)
  {
    g_OnuEponOutPm[macId].rx.fcsErrorFramesCnt = inCounters.fcsErrorFramesCnt;
    g_OnuEponOutPm[macId].rx.shortFramesCnt    = inCounters.shortFramesCnt;   
    g_OnuEponOutPm[macId].rx.longFramesCnt     = inCounters.longFramesCnt;    
    g_OnuEponOutPm[macId].rx.dataFramesCnt     = inCounters.dataFramesCnt;    
    g_OnuEponOutPm[macId].rx.ctrlFramesCnt     = inCounters.ctrlFramesCnt;    
    g_OnuEponOutPm[macId].rx.reportFramesCnt   = inCounters.reportFramesCnt;   
    g_OnuEponOutPm[macId].rx.gateFramesCnt     = inCounters.gateFramesCnt;     
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiTxPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Tx counters                           
**                
**  PARAMETERS:  S_EponIoctlTxPm *rxPm
**               MV_BOOL         clear
**               MV_U32          macId
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuEponApiTxPmGet(S_EponIoctlTxPm *txPm, MV_BOOL clear, MV_U32 macId)
{
  MV_STATUS status;
  S_TxPm    inCounters;

  status = onuEponPmTxPmGet(&inCounters, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiTxPmGet", __FILE_DESC__, __LINE__); 
    return(status);
  }

  txPm->ctrlRegReqFramesCnt = inCounters.ctrlRegReqFramesCnt - g_OnuEponOutPm[macId].tx.ctrlRegReqFramesCnt;
  txPm->ctrlRegAckFramesCnt = inCounters.ctrlRegAckFramesCnt - g_OnuEponOutPm[macId].tx.ctrlRegAckFramesCnt;
  txPm->reportFramesCnt     = inCounters.reportFramesCnt     - g_OnuEponOutPm[macId].tx.reportFramesCnt;    
  txPm->dataFramesCnt       = inCounters.dataFramesCnt       - g_OnuEponOutPm[macId].tx.dataFramesCnt;      
  txPm->txAllowedBytesCnt   = inCounters.txAllowedBytesCnt   - g_OnuEponOutPm[macId].tx.txAllowedBytesCnt;  

  if (clear == MV_TRUE)
  {
    g_OnuEponOutPm[macId].tx.ctrlRegReqFramesCnt = inCounters.ctrlRegReqFramesCnt;
    g_OnuEponOutPm[macId].tx.ctrlRegAckFramesCnt = inCounters.ctrlRegAckFramesCnt;
    g_OnuEponOutPm[macId].tx.reportFramesCnt     = inCounters.reportFramesCnt;   
    g_OnuEponOutPm[macId].tx.dataFramesCnt       = inCounters.dataFramesCnt;      
    g_OnuEponOutPm[macId].tx.txAllowedBytesCnt   = inCounters.txAllowedBytesCnt;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiSwPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               SW counters                           
**                
**  PARAMETERS:  S_EponIoctlSwPm *swPm
**               MV_BOOL         clear
**               MV_U32          macId
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuEponApiSwPmGet(S_EponIoctlSwPm *swPm, MV_BOOL clear, MV_U32 macId)
{
  MV_STATUS status;
  S_SwPm    inCounters;
  MV_U32    index;

  status = onuEponPmSwPmGet(&inCounters, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiSwPmGet", __FILE_DESC__, __LINE__); 
    return(status);
  }

  for (index = 0; index < MAX_EPON_RX_SW_CNT; index++)
  {
    swPm->swRxCnt[index] = inCounters.swRxCnt[index] - g_OnuEponOutPm[macId].sw.swRxCnt[index];
  }

  for (index = 0; index < MAX_EPON_TX_SW_CNT; index++)
  {
    swPm->swTxCnt[index] = inCounters.swTxCnt[index] - g_OnuEponOutPm[macId].sw.swTxCnt[index];
  }

  if (clear == MV_TRUE)
  {
    for (index = 0; index < MAX_EPON_RX_SW_CNT; index++)
    {
      g_OnuEponOutPm[macId].sw.swRxCnt[index] = inCounters.swRxCnt[index];
    }

    for (index = 0; index < MAX_EPON_TX_SW_CNT; index++)
    {
      g_OnuEponOutPm[macId].sw.swTxCnt[index] = inCounters.swTxCnt[index];
    }
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponApiGpmPmGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function is used to retrieve and clear (if requested) ONU
**               Gpm counters                           
**                
**  PARAMETERS:  S_EponIoctlGpmPm *rxPm
**               MV_BOOL          clear
**               MV_U32           macId
**
**  OUTPUTS:     None 
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuEponApiGpmPmGet(S_EponIoctlGpmPm *gpmPm, MV_BOOL clear, MV_U32 macId)
{
  MV_STATUS status;
  S_GpmPm   inCounters;

  status = onuEponPmGpmPmGet(&inCounters, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE, 
               "ERROR: (%s:%d) onuEponApiTxPmGet", __FILE_DESC__, __LINE__); 
    return(status);
  }

  gpmPm->grantValidCnt                 = inCounters.grantValidCnt                 - g_OnuEponOutPm[macId].gpm.grantValidCnt;                
  gpmPm->grantMaxFutureTimeErrorCnt    = inCounters.grantMaxFutureTimeErrorCnt    - g_OnuEponOutPm[macId].gpm.grantMaxFutureTimeErrorCnt;   
  gpmPm->minProcTimeErrorCnt           = inCounters.minProcTimeErrorCnt           - g_OnuEponOutPm[macId].gpm.minProcTimeErrorCnt;          
  gpmPm->lengthErrorCnt                = inCounters.lengthErrorCnt                - g_OnuEponOutPm[macId].gpm.lengthErrorCnt;               
  gpmPm->discoveryAndRegCnt            = inCounters.discoveryAndRegCnt            - g_OnuEponOutPm[macId].gpm.discoveryAndRegCnt;           
  gpmPm->fifoFullErrorCnt              = inCounters.fifoFullErrorCnt              - g_OnuEponOutPm[macId].gpm.fifoFullErrorCnt;             
  gpmPm->opcDiscoveryNotRegBcastCnt    = inCounters.opcDiscoveryNotRegBcastCnt    - g_OnuEponOutPm[macId].gpm.opcDiscoveryNotRegBcastCnt;   
  gpmPm->opcRegisterNotDiscoveryCnt    = inCounters.opcRegisterNotDiscoveryCnt    - g_OnuEponOutPm[macId].gpm.opcRegisterNotDiscoveryCnt;   
  gpmPm->opcDiscoveryNotRegNotBcastCnt = inCounters.opcDiscoveryNotRegNotBcastCnt - g_OnuEponOutPm[macId].gpm.opcDiscoveryNotRegNotBcastCnt;
  gpmPm->opcDropGrantCnt               = inCounters.opcDropGrantCnt               - g_OnuEponOutPm[macId].gpm.opcDropGrantCnt;              
  gpmPm->opcHiddenGrantCnt             = inCounters.opcHiddenGrantCnt             - g_OnuEponOutPm[macId].gpm.opcHiddenGrantCnt;            
  gpmPm->opcBackToBackCnt              = inCounters.opcBackToBackCnt              - g_OnuEponOutPm[macId].gpm.opcBackToBackCnt;             

  if (clear == MV_TRUE)
  {
    g_OnuEponOutPm[macId].gpm.grantValidCnt                 = inCounters.grantValidCnt;                
    g_OnuEponOutPm[macId].gpm.grantMaxFutureTimeErrorCnt    = inCounters.grantMaxFutureTimeErrorCnt;   
    g_OnuEponOutPm[macId].gpm.minProcTimeErrorCnt           = inCounters.minProcTimeErrorCnt;          
    g_OnuEponOutPm[macId].gpm.lengthErrorCnt                = inCounters.lengthErrorCnt;               
    g_OnuEponOutPm[macId].gpm.discoveryAndRegCnt            = inCounters.discoveryAndRegCnt;           
    g_OnuEponOutPm[macId].gpm.fifoFullErrorCnt              = inCounters.fifoFullErrorCnt;             
    g_OnuEponOutPm[macId].gpm.opcDiscoveryNotRegBcastCnt    = inCounters.opcDiscoveryNotRegBcastCnt;   
    g_OnuEponOutPm[macId].gpm.opcRegisterNotDiscoveryCnt    = inCounters.opcRegisterNotDiscoveryCnt;   
    g_OnuEponOutPm[macId].gpm.opcDiscoveryNotRegNotBcastCnt = inCounters.opcDiscoveryNotRegNotBcastCnt;
    g_OnuEponOutPm[macId].gpm.opcDropGrantCnt               = inCounters.opcDropGrantCnt;              
    g_OnuEponOutPm[macId].gpm.opcHiddenGrantCnt             = inCounters.opcHiddenGrantCnt;            
    g_OnuEponOutPm[macId].gpm.opcBackToBackCnt              = inCounters.opcBackToBackCnt;             
  }

  return(MV_OK);
}
