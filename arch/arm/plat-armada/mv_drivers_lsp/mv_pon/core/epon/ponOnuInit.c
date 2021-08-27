/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuInit.c                                              **/
/**                                                                          **/
/**  DESCRIPTION : This file contains ONU EPON init sequence definitions     **/
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
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuInit.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
MV_STATUS onuEponAsicInit(void);
MV_STATUS onuEponMacAddrGet(void);
MV_STATUS onuGponAppInit(void);
void      onuGponStateAndEventTblInit(void);
void      onuGponInPmInit(void);

/*******************************************************************************
**
**  onuPonSetup
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function execute onu setup init sequence
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**               MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonSetup(void)
{
  MV_STATUS rcode;

  /* init onu base address */
  rcode = onuPonBaseAddrInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) Failed to init onu base address\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* init onu epon mac address */
  onuEponMacAddrGet();

  /* init onu database */
  rcode = onuEponDbInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) Failed to init onu database\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* init onu Asic */
  rcode = onuEponAsicInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) Failed to init asic\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponAsicInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init default onu EPON MAC configuration 
**               
**  PARAMETERS:  MV_BOOL initTime - init indication flag, true = init sequence
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponAsicInit(void)
{
  MV_STATUS status;
  MV_U32    macAddrHigh;
  MV_U32    macAddrLow;
  MV_U32    macId;

  /* Disable MAC */
  status = mvOnuEponMacOnuEnableSet(ONU_RX_DIS, ONU_TX_DIS);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuStateSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* MAC State */
  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status = mvOnuEponMacOnuStateSet(ONU_EPON_NOT_REGISTERD, macId);
    if (status != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                 "ERROR: (%s:%d) mvOnuEponMacOnuStateSet\n\r", __FILE_DESC__, __LINE__);
      return(MV_ERROR);
    }
  }

  /* Broadcast Address */
  status = mvOnuEponMacGenBcastAddrSet(ONU_BROADCAST_ADDR_HIGH, ONU_BROADCAST_ADDR_LOW);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacGenBcastAddrSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* Unicast Address */
  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    onuEponDbOnuMacAddrGet(&macAddrLow, &macAddrHigh, macId);

    status = mvOnuEponMacGenUcastAddrSet(macAddrHigh, macAddrLow, macId); 
    if (status != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                 "ERROR: (%s:%d) mvOnuEponMacGenUcastAddrSet\n\r", __FILE_DESC__, __LINE__);
      return(MV_ERROR);
    }
  }

  status = mvOnuEponMacGenSyncTimeSet(ONU_DEF_SYNC_TIME, 
                                      ONU_DEF_SYNC_TIME_ADD, 
                                      ONU_DEF_SYNC_TIME_FORCE_SW, 
                                      ONU_DEF_SYNC_TIME_DIS_GATE_AUTO, 
                                      ONU_DEF_SYNC_TIME_DIS_DISCOVER_AUTO); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacGenSyncTimeSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacDdmDelaySet(ONU_DEF_DDM_DELAY); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvAsicReg_EPON_DDM_1814_CONFIG\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacRxpPacketForwardSet(ONU_CTRL_FRAME_TO_DATA_QUEUE,
                                           ONU_CTRL_FRAME_TO_CTRL_QUEUE,
                                           ONU_RPRT_FRAME_TO_DATA_QUEUE,
                                           ONU_RPRT_FRAME_TO_RPRT_QUEUE,
                                           ONU_SLOW_FRAME_TO_RPRT_QUEUE,
                                           ONU_SLOW_FRAME_TO_CTRL_QUEUE);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacRxpPacketForwardSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacOnuRegAutoUpdateStateSet(1);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuAutoUpdateStateSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacGenOnuConfigSet(ONU_RX_PCS_FEC_DIS, 
                                       ONU_TX_PCS_FEC_DIS, 
                                       ONU_REPORT_AUTO_RES, 
                                       ONU_REG_ACK_AUTO_RES, 
                                       ONU_REG_REQ_AUTO_RES,
                                       ONU_TX_FEC_DIS); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacGenOnuConfigSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacRxpPacketFilterSet(ONU_FORWARD_LLID_CRC_ERR_PKT, 
                                          ONU_FORWARD_LLID_FCS_ERR_PKT,
                                          ONU_FORWARD_LLID_GMII_ERR_PKT, 
                                          ONU_FORWARD_LLID_LEN_ERR_PKT,
                                          ONU_FORWARD_LLID_ALL_PKT, 
                                          ONU_FORWARD_LLID_7FFF_MODE_0_PKT,
                                          ONU_FORWARD_LLID_7FFF_MODE_1_PKT, 
                                          ONU_FORWARD_LLID_XXXX_MODE_1_PKT, 
                                          ONU_DROP_LLID_NNNN_MODE_1_PKT); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacRxpPacketFilterSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacGenLaserParamSet(ONU_DEF_LASER_ON_TIME, ONU_DEF_LASER_OFF_TIME);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacGenLaserParamSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    status = mvOnuEponMacTxmLlidSet(ONU_DEF_TX_LLID, macId); 
    if (status != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                 "ERROR: (%s:%d) mvOnuEponMacTxmLlidSet\n\r", __FILE_DESC__, __LINE__);
      return(MV_ERROR);
    }

    status = mvOnuEponMacRxpLlidDataSet(ONU_DEF_RX_LLID, macId); 
    if (status != MV_OK)
    {
      mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                 "ERROR: (%s:%d) mvOnuEponMacRxpLlidDataSet\n\r", __FILE_DESC__, __LINE__);
      return(MV_ERROR);
    }
  }
  
  status = mvOnuEponMacTxmConfigSet(ONU_DEF_TXM_CFG_MODE,     
                                    ONU_DEF_TXM_CFG_ALIGNMENT,
                                    ONU_DEF_TXM_CFG_PRIORITY);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacTxmConfigSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = onuPonSerdesInit();
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) onuPonSerdesInit\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponAsicReInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function re-init default onu EPON MAC configuration 
**               
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponAsicReInit(MV_U32 macId)
{
  MV_STATUS status;

  status = mvOnuEponMacOnuStateSet(ONU_EPON_NOT_REGISTERD, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuStateSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacTxmLlidSet(ONU_DEF_TX_LLID, macId); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacTxmLlidSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  status = mvOnuEponMacRxpLlidDataSet(ONU_DEF_RX_LLID, macId); 
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacRxpLlidDataSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponMacAddrGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return ONU EPON MAC address 
**               
**  PARAMETERS:  MV_U8 *macAddr
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponMacAddrGet(void)
{
  mvPonPrint(PON_PRINT_INFO, PON_API_MODULE,
             "!!!!!! FIXED MAC ADDRESS  SHOULD BE CHANGED!!!!!\n");

  return(MV_OK);
}

/*******************************************************************************
**
**  onuPonSwitchOn
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function execute onu switchOn init sequence
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonSwitchOn(void)
{
  MV_STATUS rcode;

  /* init onu RTOS resources */
  rcode = onuPonRtosResourceInit();
  if (rcode != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) onuPonRtosResourceInit\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  /* onu epon counters table */ 
  onuEponPmInit();
  onuEponIsrInit();

  return(MV_OK);
}

/*******************************************************************************
**
**  onuPonOperate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function execute onu operate init sequence
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**           
*******************************************************************************/
MV_STATUS onuPonOperate(void)
{
  MV_STATUS status;
  MV_U32    interruptMask;

  /* start onu pon pon pm timer */
  onuPonTimerEnable(&(onuPonResourceTbl_s.onuPonPmTimerId));

  /* start onu pon pon mpc timer */
  onuPonTimerEnable(&(onuPonResourceTbl_s.onuPonMpcpTimerId));

  /* enable onu pon interrupt mask */
  interruptMask = ONU_EPON_INTERRUPTS;
  status = mvOnuEponMacPonInterruptMaskSet(interruptMask);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) Enable PON interrupt mask\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  /* Enable MAC */
  status = mvOnuEponMacOnuRxEnableSet(ONU_RX_EN);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuRxEnableSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }
 
  status = mvOnuEponMacOnuTxEnableSet(ONU_TX_EN, 0);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuRxEnableSet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  return(MV_OK);
}
