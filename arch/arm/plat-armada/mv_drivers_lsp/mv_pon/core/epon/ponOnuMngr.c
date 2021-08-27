/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuMngr.c                                              **/
/**                                                                          **/
/**  DESCRIPTION : This file implements ONU EPON Manager functionality       **/
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
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuMngr.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
/* Interrupt handler Functions */
void      onuEponPonMngIntrAlarmHandler(MV_U32 alarm, MV_BOOL alarmStatus); 
void      onuEponPonMngIntrMessageHandler(MV_U32 msg);
MV_STATUS onuEponPonMngIntrCtrlMessageHandler(S_OnuEponCtrlBuffer *ctrlBuf);
MV_STATUS onuEponPonMngIntrRprtMessageHandler(S_OnuEponCtrlBuffer *ctrlBuf);

/* State Machine Functions */
MV_STATUS onuEponPonMngRegMsgFlagAck(S_OnuEponRegMpcFrame *mpcFrame);
MV_STATUS onuEponPonMngRegMsgFlagNack(S_OnuEponRegMpcFrame *mpcFrame);
MV_STATUS onuEponPonMngRegMsgFlagReReg(S_OnuEponRegMpcFrame *mpcFrame);
MV_STATUS onuEponPonMngRegMsgFlagDeReg(S_OnuEponRegMpcFrame *mpcFrame);

/* MPC Message Handling Functions */
MV_STATUS onuEponReadCtrlFrameData(S_OnuEponCtrlBuffer *ctrlBuf);                                                                                 
MV_STATUS onuEponRetrieveCtrlFrameData(S_OnuEponCtrlBuffer *ctrlBuf);
MV_STATUS onuEponSendCtrlFrameData(S_OnuEponCtrlBuffer *ctrlBuf, MV_U32 macId);         
MV_STATUS onuEponReadReportFrameData(S_OnuEponCtrlBuffer *ctrlBuf);                                                                                 
MV_STATUS onuEponRetrieveReportFrameData(S_OnuEponCtrlBuffer *ctrlBuf);   

void      onuEponConvertN2HShort(void *srcValPtr, void *destValPtr);
void      onuEponConvertN2HLong(void *srcValPtr, void *destValPtr);
void      onuEponConvertH2NShort(void *srcValPtr, void *destValPtr);
void      onuEponConvertH2NLong(void *srcValPtr, void *destValPtr);

/******************************************************************************/
/* ========================================================================== */
/*                         Alarm Section                                      */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponPonMngIntrAlarmHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt alarm 
**               functionality 
**               
**  PARAMETERS:  MV_U32 alarm    
**               MV_U32 alarmStatus
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponPonMngIntrAlarmHandler(MV_U32 alarm, MV_BOOL alarmStatus)
{
  MV_STATUS status;
  MV_U32    macId;

  /* alarm is ON */
  if (alarmStatus != MV_FALSE) 
  {
    for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
    {
      /* re-init onu database */
      status = onuEponDbReInit(macId);
      if (status != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Failed to re-init onu database\n\r", __FILE_DESC__, __LINE__);
        return;
      }

      /* init onu Asic */
      status = onuEponAsicReInit(macId);
      if (status != MV_OK)
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Failed to re-init onu EPON MAC\n\r", __FILE_DESC__, __LINE__);
        return;
      }
    }

    /* update alarm */
    onuEponAlarmSet(alarm, alarmStatus);
  }

  /* alarm is OFF */
  else
  {
    /* update alarm */
    onuEponAlarmSet(alarm, alarmStatus);
  }
}

/******************************************************************************/
/* ========================================================================== */
/*                         Message Section                                    */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponPonMngIntrMessageHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt MPC message 
**               functionality 
**               
**  PARAMETERS:  MV_U32 msg    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponPonMngIntrMessageHandler(MV_U32 msg)
{
  MV_STATUS           status;  
  S_OnuEponCtrlBuffer ctrlBuf;

  memset((void*)&ctrlBuf, 0, sizeof (S_OnuEponCtrlBuffer));

  if (msg == ONU_EPON_RX_CTRL_MSG) 
  {
    status = onuEponRetrieveCtrlFrameData(&ctrlBuf);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Read Rx Ctrl Frame\n\r", __FILE_DESC__, __LINE__);
      onuEponPmSwRxCountersAdd(TOTAL_MPCP_RX_ERROR_FRAME_CNT, 0);
      return;
    }

    status = onuEponPonMngIntrCtrlMessageHandler(&ctrlBuf);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Handle Rx Ctrl Frame\n\r", __FILE_DESC__, __LINE__);
      return;
    }
  }
  else if (msg == ONU_EPON_RX_RPRT_MSG) 
  {
    status = onuEponRetrieveReportFrameData(&ctrlBuf);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Read Rx Report Frame\n\r", __FILE_DESC__, __LINE__);
      return;
    }

    status = onuEponPonMngIntrRprtMessageHandler(&ctrlBuf);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Handle Rx Report Frame\n\r", __FILE_DESC__, __LINE__);
      return;
    }
  }
  else
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Unsupported Frame\n\r", __FILE_DESC__, __LINE__);
    return;
  }
}

/*******************************************************************************
**
**  onuEponPonMngIntrCtrlMessageHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt MPC Ctrl 
**               message functionality 
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponPonMngIntrCtrlMessageHandler(S_OnuEponCtrlBuffer *ctrlBuf)
{
  MV_STATUS            status;  
  S_OnuEponGenMpcFrame *genMpcMsg;
  S_OnuEponRegMpcFrame *regMpcMsg;
  MV_U16               regOpcode;
  MV_U16               regFlags;

  genMpcMsg = (S_OnuEponGenMpcFrame *)ctrlBuf->data;
  onuEponConvertN2HShort(&genMpcMsg->genMpcPdu.opCode, &regOpcode);

  if (regOpcode == MPC_OPCODE_REGISTER)
  {
    regMpcMsg = (S_OnuEponRegMpcFrame *)ctrlBuf->data;
    regFlags  = regMpcMsg->regMpcPdu.flags;

    if (regFlags == REGISTER_FLAGS_ACK) 
    {
      onuEponPmSwRxCountersAdd(TOTAL_MPCP_REGISTER_ACK_CNT, 0);
   
      status = onuEponPonMngRegMsgFlagAck(regMpcMsg);
      if (status != MV_OK) 
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Register with flag Ack\n\r", __FILE_DESC__, __LINE__);
        return(MV_ERROR);
      }
    }
    else if (regFlags == REGISTER_FLAGS_NACK) 
    {
      onuEponPmSwRxCountersAdd(TOTAL_MPCP_REGISTER_NACK_CNT, 0);

      status = onuEponPonMngRegMsgFlagNack(regMpcMsg);
      if (status != MV_OK) 
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Register with flag Nack\n\r", __FILE_DESC__, __LINE__);
        return(MV_ERROR);
      }
    }
    else if (regFlags == REGISTER_FLAGS_REREG) 
    {
      onuEponPmSwRxCountersAdd(TOTAL_MPCP_REGISTER_REREG_FRAME_CNT, 0);
   
      status = onuEponPonMngRegMsgFlagReReg(regMpcMsg);
      if (status != MV_OK) 
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Register with flag Re-Register\n\r", __FILE_DESC__, __LINE__);
        return(MV_ERROR);
      }
    }
    else if (regFlags == REGISTER_FLAGS_DEREG) 
    {
      onuEponPmSwRxCountersAdd(TOTAL_MPCP_REGISTER_DEREG_FRAME_CNT, 0); 
   
      status = onuEponPonMngRegMsgFlagDeReg(regMpcMsg);
      if (status != MV_OK) 
      {
        mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                   "ERROR: (%s:%d) Register with flag De-Register\n\r", __FILE_DESC__, __LINE__);
        return(MV_ERROR);
      }
    }
    else
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Invalid Control message flag(%d)\n\r", __FILE_DESC__, __LINE__, regFlags);
      return(MV_ERROR);
    }
  }
      
  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPonMngIntrRprtMessageHandler
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function implements the pon manager interrupt MPC Report 
**               message functionality 
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponPonMngIntrRprtMessageHandler(S_OnuEponCtrlBuffer *ctrlBuf)
{
  mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
             "ERROR: (%s:%d) Report Messages are handled by the EPON MAC\n\r", __FILE_DESC__, __LINE__);
  return(MV_OK);
}

/******************************************************************************/
/* ========================================================================== */
/*                         MPC State Machine Handling Section                 */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponPonMngRegMsgFlagAck
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when REGISTER message with flag Ack is
**               received
**
**  PARAMETERS:  S_OnuEponRegMpcFrame *mpcFrame    
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponPonMngRegMsgFlagAck(S_OnuEponRegMpcFrame *mpcFrame)
{
  MV_STATUS status;
  MV_U32    macId;
  MV_U32    macState;
  MV_U16    msgEtype;
  MV_U16    msgSyncTime;
  MV_U16    msgLlid;
  MV_U16    msgOpcode;
  MV_U8     msgFlags;              
  MV_U8     msgEchoedPendingGrants;

  onuEponConvertN2HShort(&mpcFrame->stdEthFrame.etherType, &msgEtype);
  onuEponConvertN2HShort(&mpcFrame->genMpcPdu.opCode, &msgOpcode);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.assignedLlidPort, &msgLlid);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.syncTime, &msgSyncTime);
  msgFlags               = mpcFrame->regMpcPdu.flags;              
  msgEchoedPendingGrants = mpcFrame->regMpcPdu.echoedPendingGrants;

  mvPonPrint(PON_PRINT_INFO, PON_MNG_MODULE,
             "\n"
             "REGISTER [Flag=Ack] Message\n"
             "===========================\n"
             "DestMac[%x:%x:%x:%x:%x:%x] SrcMac[%x:%x:%x:%x:%x:%x]\n"
             "Etype[0x%x] Opcode[%x] Sync[0x%x], LLID[%x] Grant[%x] Flag[%x]\n",
             mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
             mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
             mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5],
             mpcFrame->stdEthFrame.srcAddr[0], mpcFrame->stdEthFrame.srcAddr[1],
             mpcFrame->stdEthFrame.srcAddr[2], mpcFrame->stdEthFrame.srcAddr[3],
             mpcFrame->stdEthFrame.srcAddr[4], mpcFrame->stdEthFrame.srcAddr[5], 
             msgEtype,
             msgOpcode,  
             msgSyncTime,
             msgLlid,  
             msgEchoedPendingGrants,  
             msgFlags); 

  status = matchDestAddressToMacId(mpcFrame->stdEthFrame.destAddr, &macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) matchDestAddressToMacId, invalid dest addr [%02X:%02X:%02X:%02X:%02X:%02X]\n", 
               __FILE_DESC__, __LINE__,
               mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
               mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
               mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5]);
    return(MV_ERROR);
  }

  /* Validate mac state was updated by HW */
  status = mvOnuEponMacOnuStateGet(&macState, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuStateGet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  if (macState != ONU_EPON_REGISTERED)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Mac[%d] state is not Registered!!!!\n\r", __FILE_DESC__, __LINE__, macId);
    return(MV_ERROR);
  }

  onuEponDbPktTxLlidSet(msgLlid, macId);
  onuEponDbPktRxLlidSet(msgLlid, macId);
  onuEponDbOnuStateSet(ONU_EPON_03_OPERATION, macId);

  mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
             "= ONU Mac[%d] Registered =\n\r", macId);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPonMngRegMsgFlagNack
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when REGISTER message with flag Nack
**               is received
**
**  PARAMETERS:  S_OnuEponRegMpcFrame *mpcFrame  
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponPonMngRegMsgFlagNack(S_OnuEponRegMpcFrame *mpcFrame)
{
  MV_STATUS status;
  MV_U32    macId;
  MV_U16    msgEtype;
  MV_U16    msgSyncTime;
  MV_U16    msgLlid;
  MV_U16    msgOpcode;
  MV_U8     msgFlags;              
  MV_U8     msgEchoedPendingGrants;

  onuEponConvertN2HShort(&mpcFrame->stdEthFrame.etherType, &msgEtype);
  onuEponConvertN2HShort(&mpcFrame->genMpcPdu.opCode, &msgOpcode);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.assignedLlidPort, &msgLlid);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.syncTime, &msgSyncTime);
  msgFlags               = mpcFrame->regMpcPdu.flags;              
  msgEchoedPendingGrants = mpcFrame->regMpcPdu.echoedPendingGrants;

  mvPonPrint(PON_PRINT_INFO, PON_MNG_MODULE,
             "\n"
             "REGISTER [Flag=Nack] Message\n"
             "============================\n"
             "DestMac[%x:%x:%x:%x:%x:%x] SrcMac[%x:%x:%x:%x:%x:%x]\n"
             "Etype[0x%x] Opcode[%x] Sync[0x%x], LLID[%x] Grant[%x] Flag[%x]\n",
             mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
             mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
             mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5],
             mpcFrame->stdEthFrame.srcAddr[0], mpcFrame->stdEthFrame.srcAddr[1],
             mpcFrame->stdEthFrame.srcAddr[2], mpcFrame->stdEthFrame.srcAddr[3],
             mpcFrame->stdEthFrame.srcAddr[4], mpcFrame->stdEthFrame.srcAddr[5], 
             msgEtype,
             msgOpcode,  
             msgSyncTime,
             msgLlid,  
             msgEchoedPendingGrants,  
             msgFlags); 

  status = matchDestAddressToMacId(mpcFrame->stdEthFrame.destAddr, &macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) matchDestAddressToMacId, invalid dest addr [%02X:%02X:%02X:%02X:%02X:%02X]\n", 
               __FILE_DESC__, __LINE__,
               mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
               mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
               mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5]);
    return(MV_ERROR);
  }
    /* re-init onu database */
  status = onuEponDbReInit(macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Failed to re-init onu database\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }
 
  /* init onu Asic */
  status = onuEponAsicReInit(macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Failed to re-init onu EPON MAC\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPonMngRegMsgFlagReReg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when REGISTER message with flag re-register
**               is received
**
**  PARAMETERS:  S_OnuEponRegMpcFrame *mpcFrame  
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponPonMngRegMsgFlagReReg(S_OnuEponRegMpcFrame *mpcFrame)
{
  MV_STATUS status;
  MV_U32    macId;
  MV_U32    macState;
  MV_U16    msgEtype;
  MV_U16    msgSyncTime;
  MV_U16    msgLlid;
  MV_U16    msgOpcode;
  MV_U8     msgFlags;              
  MV_U8     msgEchoedPendingGrants;

  onuEponConvertN2HShort(&mpcFrame->stdEthFrame.etherType, &msgEtype);
  onuEponConvertN2HShort(&mpcFrame->genMpcPdu.opCode, &msgOpcode);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.assignedLlidPort, &msgLlid);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.syncTime, &msgSyncTime);
  msgFlags               = mpcFrame->regMpcPdu.flags;              
  msgEchoedPendingGrants = mpcFrame->regMpcPdu.echoedPendingGrants;

  mvPonPrint(PON_PRINT_INFO, PON_MNG_MODULE,
             "\n"
             "REGISTER [Flag=ReReg] Message\n"
             "=============================\n"
             "DestMac[%x:%x:%x:%x:%x:%x] SrcMac[%x:%x:%x:%x:%x:%x]\n"
             "Etype[0x%x] Opcode[%x] Sync[0x%x], LLID[%x] Grant[%x] Flag[%x]\n",
             mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
             mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
             mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5],
             mpcFrame->stdEthFrame.srcAddr[0], mpcFrame->stdEthFrame.srcAddr[1],
             mpcFrame->stdEthFrame.srcAddr[2], mpcFrame->stdEthFrame.srcAddr[3],
             mpcFrame->stdEthFrame.srcAddr[4], mpcFrame->stdEthFrame.srcAddr[5], 
             msgEtype,
             msgOpcode,  
             msgSyncTime,
             msgLlid,  
             msgEchoedPendingGrants,  
             msgFlags); 

  status = matchDestAddressToMacId(mpcFrame->stdEthFrame.destAddr, &macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) matchDestAddressToMacId, invalid dest addr [%02X:%02X:%02X:%02X:%02X:%02X]\n", 
               __FILE_DESC__, __LINE__,
               mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
               mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
               mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5]);
    return(MV_ERROR);
  }

  /* Validate mac state was updated by HW */
  status = mvOnuEponMacOnuStateGet(&macState, macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) mvOnuEponMacOnuStateGet\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  if (macState != ONU_EPON_REGISTERED)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Mac[%d] state is not Registered!!!!\n\r", __FILE_DESC__, __LINE__, macId);
    return(MV_ERROR);
  }

  onuEponDbPktTxLlidSet(msgLlid, macId);
  onuEponDbPktRxLlidSet(msgLlid, macId);
  onuEponDbOnuStateSet(ONU_EPON_03_OPERATION, macId);

  mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
             "= ONU Mac[%d] Registered =\n\r", macId);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponPonMngRegMsgFlagDeReg
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function is called when REGISTER message with flag de-register
**               is received
**
**  PARAMETERS:  S_OnuEponRegMpcFrame *mpcFrame  
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponPonMngRegMsgFlagDeReg(S_OnuEponRegMpcFrame *mpcFrame)
{
  MV_STATUS status;
  MV_U32    macId;
  MV_U16    msgEtype;
  MV_U16    msgSyncTime;
  MV_U16    msgLlid;
  MV_U16    msgOpcode;
  MV_U8     msgFlags;              
  MV_U8     msgEchoedPendingGrants;

  onuEponConvertN2HShort(&mpcFrame->stdEthFrame.etherType, &msgEtype);
  onuEponConvertN2HShort(&mpcFrame->genMpcPdu.opCode, &msgOpcode);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.assignedLlidPort, &msgLlid);
  onuEponConvertN2HShort(&mpcFrame->regMpcPdu.syncTime, &msgSyncTime);
  msgFlags               = mpcFrame->regMpcPdu.flags;              
  msgEchoedPendingGrants = mpcFrame->regMpcPdu.echoedPendingGrants;

  mvPonPrint(PON_PRINT_INFO, PON_MNG_MODULE,
             "\n"
             "REGISTER [Flag=DeReg] Message\n"
             "=============================\n"
             "DestMac[%x:%x:%x:%x:%x:%x] SrcMac[%x:%x:%x:%x:%x:%x]\n"
             "Etype[0x%x] Opcode[%x] Sync[0x%x], LLID[%x] Grant[%x] Flag[%x]\n",
             mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
             mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
             mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5],
             mpcFrame->stdEthFrame.srcAddr[0], mpcFrame->stdEthFrame.srcAddr[1],
             mpcFrame->stdEthFrame.srcAddr[2], mpcFrame->stdEthFrame.srcAddr[3],
             mpcFrame->stdEthFrame.srcAddr[4], mpcFrame->stdEthFrame.srcAddr[5], 
             msgEtype,
             msgOpcode,  
             msgSyncTime,
             msgLlid,  
             msgEchoedPendingGrants,  
             msgFlags); 
 
  status = matchDestAddressToMacId(mpcFrame->stdEthFrame.destAddr, &macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) matchDestAddressToMacId, invalid dest addr [%02X:%02X:%02X:%02X:%02X:%02X]\n", 
               __FILE_DESC__, __LINE__,
               mpcFrame->stdEthFrame.destAddr[0], mpcFrame->stdEthFrame.destAddr[1],
               mpcFrame->stdEthFrame.destAddr[2], mpcFrame->stdEthFrame.destAddr[3],
               mpcFrame->stdEthFrame.destAddr[4], mpcFrame->stdEthFrame.destAddr[5]);
    return(MV_ERROR);
  }

  /* re-init onu database */
  status = onuEponDbReInit(macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Failed to re-init onu database\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }
 
  /* init onu Asic */
  status = onuEponAsicReInit(macId);
  if (status != MV_OK)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Failed to re-init onu EPON MAC\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }
 
  return(MV_OK);
}

/******************************************************************************/
/* ========================================================================== */
/*                         MPC Message Handling Section                       */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponReadCtrlFrameData
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function read control frame from the Rx Control queue
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponReadCtrlFrameData(S_OnuEponCtrlBuffer *ctrlBuf)                                       
{         
  MV_STATUS status;
  MV_U32    usedCount;
  MV_U32    data;  
  MV_U32    index;

  status = mvOnuEponMacCpqRxCtrlQueueUsedCountGet(&usedCount);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Rx Ctrl Queue Used Count\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  if (usedCount < ctrlBuf->length)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Not enough data,  Frame Length = %d, usedCount = %d\n\r", 
               __FILE_DESC__, __LINE__, ctrlBuf->length, usedCount);
    return(MV_ERROR);
  }

  for(index = 0; index < usedCount; index++)                                                         
  {             
    status = mvOnuEponMacCpqRxCtrlQueueReadData(&data);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Read Rx Ctrl Queue Data\n\r", __FILE_DESC__, __LINE__);
      return(status);
    }
    else
    {
      ctrlBuf->data[index] = (MV_U8)data;
    }
  }

  onuEponPmSwRxCountersAdd(TOTAL_MPCP_RX_FRAME_CNT, 0);
                                                                                                     
  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponRetrieveCtrlFrameData
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function retrieve control frame from the Rx Control queue
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponRetrieveCtrlFrameData(S_OnuEponCtrlBuffer *ctrlBuf)                                   
{  
  MV_STATUS status;
  MV_U32    usedHeaderCount;
  MV_U32    flagLow;
  MV_U32    flagHigh;
  MV_U32    frameLength;                                                                               

  ctrlBuf->length = 0;                                                                                                                         

  status = mvOnuEponMacCpqRxCtrlHeaderQueueUsedCountGet(&usedHeaderCount);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Rx Ctrl Header Queue Used Count\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  if (usedHeaderCount <= 0)                                                                            
  {      
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) No frames received,  Used Header Count = %d\n\r", 
               __FILE_DESC__, __LINE__, usedHeaderCount);
    return(MV_ERROR);
  }

  status = mvOnuEponMacCpqRxCtrlHeaderQueueReadData(&flagLow, &flagHigh);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Rx Ctrl Header Queue Data\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  frameLength = ((flagLow & MPC_FRAME_LENGTH_MASK) >> MPC_FRAME_LENGTH_SHIFT);
  if ((frameLength > 0) && (frameLength <= sizeof(ctrlBuf->data))) 
  {                                                                                                    
    ctrlBuf->length = frameLength;   
    status = onuEponReadCtrlFrameData(ctrlBuf);                                                        
    if (status != MV_OK) 
    {
      return(status);
    }
  }                                                                                                    
  else                                                                                                 
  {  
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Unexpected frame length %d\n\r", __FILE_DESC__, __LINE__, frameLength);
    return(status);
  }  
                                                                                                                                                  
  return(MV_OK);                                                                                       
}

/*******************************************************************************
**
**  onuEponSendCtrlFrameData
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function send control frame to the Tx Control queue
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf
**               MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponSendCtrlFrameData(S_OnuEponCtrlBuffer *ctrlBuf, MV_U32 macId)         
{
  MV_STATUS status;
  MV_U32    numFreeFrames;
  MV_U32    numFreeData;
  MV_U32    index;

  status = mvOnuEponMacCpqTxCtrlHeaderQueueFree(&numFreeFrames, macId);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Tx Ctrl Header Queue\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  status = mvOnuEponMacCpqTxCtrlQueueFree(&numFreeData, macId);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Tx Ctrl Queue\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  if (numFreeFrames <= 0) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) No free place in Tx Ctrl Header Queue (%d), data(%d)\n\r", 
               __FILE_DESC__, __LINE__, numFreeFrames, numFreeData);
    return(MV_ERROR);
  }

  if (numFreeData < ctrlBuf->length)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) No free place in Tx Ctrl Data Queue\n\r", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  for(index = 0; index < ctrlBuf->length; index++)                                                     
  {             
    status = mvOnuEponMacCpqTxCtrlQueueWrite(ctrlBuf->data[index], macId);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Write Tx Ctrl Queue Data\n\r", __FILE_DESC__, __LINE__);
      return(status);
    }
  }

  status = mvOnuEponMacCpqTxCtrlHeaderQueueWrite(ctrlBuf->length, macId);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Write Tx Ctrl Header Queue\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  onuEponPmSwTxCountersAdd(TOTAL_MPCP_TX_FRAME_CNT, macId);

  return(MV_OK);                                                                                                                              
}

/*******************************************************************************
**
**  onuEponReadReportFrameData
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function read report frame from the Rx report queue
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponReadReportFrameData(S_OnuEponCtrlBuffer *ctrlBuf)                                     
{         
  MV_STATUS status;
  MV_U32    usedCount;
  MV_U32    data;  
  MV_U32    index;

  status = mvOnuEponMacCpqRxRprtQueueUsedCountGet(&usedCount);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Rx Report Queue Used Count\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  if (usedCount < ctrlBuf->length)
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Not enough data,  Frame Length = %i, usedCount = %i\n\r", 
               __FILE_DESC__, __LINE__, ctrlBuf->length, usedCount);
    return(MV_ERROR);
  }

  for(index = 0; index < usedCount; index++)                                                           
  {             
    status = mvOnuEponMacCpqRxRprtQueueReadData(&data);
    if (status != MV_OK) 
    {
      mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
                 "ERROR: (%s:%d) Read Rx Report Queue Data\n\r", __FILE_DESC__, __LINE__);
      return(status);
    }
    else
    {
      ctrlBuf->data[index] = (MV_U8)data;
    }
  }
                                                                                                       
  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponRetrieveReportFrameData
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function retrieve report frame from the Rx report queue
**               
**  PARAMETERS:  S_OnuEponCtrlBuffer *ctrlBuf
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuEponRetrieveReportFrameData(S_OnuEponCtrlBuffer *ctrlBuf)                                 
{  
  MV_STATUS status;
  MV_U32    usedHeaderCount;
  MV_U32    flagLow;                                                                                   
  MV_U32    flagHigh;                                                                                  
  MV_U32    frameLength;                                                                               
                                                                                                       
  ctrlBuf->length = 0;             

  status = mvOnuEponMacCpqRxRprtHeaderQueueUsedCountGet(&usedHeaderCount);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Rx Report Header Queue Used Count\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  if (usedHeaderCount <= 0)                                                                            
  {      
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) No frames received,  Used Header Count = %i\n\r", 
               __FILE_DESC__, __LINE__, usedHeaderCount);
    return(MV_ERROR);
  }       
 
  status = mvOnuEponMacCpqRxRprtHeaderQueueReadData(&flagLow, &flagHigh);
  if (status != MV_OK) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Read Rx Report Header Queue Data\n\r", __FILE_DESC__, __LINE__);
    return(status);
  }

  frameLength = flagLow & MPC_FRAME_LENGTH_MASK;                                                       
  if (frameLength > 0 && frameLength <= sizeof(ctrlBuf->data))                                         
  {                                                                                                    
    ctrlBuf->length = frameLength;                                                                     
    status = onuEponReadReportFrameData(ctrlBuf);                                                      
    if (status != MV_OK) 
    {
      return(status);
    }
  }                                                                                                    
  else                                                                                                 
  {  
    mvPonPrint(PON_PRINT_ERROR, PON_MNG_MODULE,
               "ERROR: (%s:%d) Unexpected frame length %d\n\r", __FILE_DESC__, __LINE__, frameLength);
    return(status);
  }  
                                                                                                                                                   
  return(MV_OK);                                                                                                                              
}

/*******************************************************************************
**
**  onuEponConvertN2HShort
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function convert MV_U16 from Netwotk order to Host
**               
**  PARAMETERS:  void *srcValPtr
**               void *destValPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponConvertN2HShort(void *srcValPtr, void *destValPtr)
{
  MV_U16 src;
  MV_U16 dest;
  
  memcpy(&src, srcValPtr, sizeof(src));

  dest = ((src       & 0xFF) << 8) |
         ((src >> 8) & 0xFF);

  memcpy(destValPtr, &dest, sizeof(dest));
}

/*******************************************************************************
**
**  onuEponConvertH2NShort
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function convert MV_U16 from Host order to Network
**               
**  PARAMETERS:  void *srcValPtr
**               void *destValPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponConvertH2NShort(void *srcValPtr, void *destValPtr)
{
  MV_U16 src;
  MV_U16 dest;

  memcpy(&src, srcValPtr, sizeof(src));

  dest = ((src       & 0xFF) << 8) |
         ((src >> 8) & 0xFF);

  memcpy(destValPtr, &dest, sizeof(dest));
}

/*******************************************************************************
**
**  onuEponConvertN2HLong
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function convert MV_U32 from Netwotk order to Host
**               
**  PARAMETERS:  void *srcValPtr
**               void *destValPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponConvertN2HLong(void *srcValPtr, void *destValPtr)
{
  MV_U32 src;
  MV_U32 dest;

  memcpy(&src, srcValPtr, sizeof(src));

  dest =  ((src        & 0xFF) << 24) |
         (((src >>  8) & 0xFF) << 16) |
         (((src >> 16) & 0xFF) <<  8) |
          ((src >> 24) & 0xFF);

  memcpy(destValPtr, &dest, sizeof(dest));
}

/*******************************************************************************
**
**  onuEponConvertH2NLong
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function convert MV_U32 from Host order to Network
**               
**  PARAMETERS:  void *srcValPtr
**               void *destValPtr
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuEponConvertH2NLong(void *srcValPtr, void *destValPtr)
{
  MV_U32 src;
  MV_U32 dest;

  memcpy(&src, srcValPtr, sizeof(src));

  dest =  ((src        & 0xFF) << 24) |
         (((src >>  8) & 0xFF) << 16) |
         (((src >> 16) & 0xFF) <<  8) |
          ((src >> 24) & 0xFF);

  memcpy(destValPtr, &dest, sizeof(dest));
}
