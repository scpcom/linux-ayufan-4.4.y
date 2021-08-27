/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuDb.c                                                **/
/**                                                                          **/
/**  DESCRIPTION : This file implements ONU EPON database functionality      **/
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
#define __FILE_DESC__ "mv_pon/core/epon/ponOnuDb.c"

/* Global Variables
------------------------------------------------------------------------------*/
/* ONU GPON Database */
S_OnuEponDb onuEponDb_s;

/* Local Variables
------------------------------------------------------------------------------*/
/* ONU EPON database init function */
MV_STATUS onuEponOnuMacTblInit(void);
MV_STATUS onuEponOnuGenTblInit(void);
MV_STATUS onuEponDbOnuDatapathTblInit(void);

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/
MV_U8 fixMacAddrs[8][6] = {{0x00,0x00,0x00,0x00,0x00,0x82},
                           {0x00,0x00,0x00,0x00,0x00,0x83},
                           {0x00,0x00,0x00,0x00,0x00,0x84},
                           {0x00,0x00,0x00,0x00,0x00,0x85},
                           {0x00,0x00,0x00,0x00,0x00,0x86},
                           {0x00,0x00,0x00,0x00,0x00,0x87},
                           {0x00,0x00,0x00,0x00,0x00,0x88},
                           {0x00,0x00,0x00,0x00,0x00,0x89}};

extern u8 mvMacAddr[CONFIG_MV_ETH_PORTS_NUM][MV_MAC_ADDR_SIZE];

/******************************************************************************/
/* ========================================================================== */
/*                         Initialization Section                             */
/* ========================================================================== */
/******************************************************************************/

/*******************************************************************************
**
**  onuEponDbInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database to default values
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponDbInit(void)
{
  onuEponOnuMacTblInit();             
  onuEponOnuGenTblInit();             
  onuEponDbOnuDatapathTblInit();      

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbReInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function re-init onu database to default values
**               
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponDbReInit(MV_U32 macId)
{
  /* set onu to state 01 */
  onuEponDbOnuStateSet(ONU_EPON_02_REGISTER_PENDING, macId);

  /* set onu sync time to 0 */
  onuEponDbOnuSyncTimeSet(0, macId);

  onuEponDbPktTxLlidSet(ONU_DEF_TX_LLID, macId);
  onuEponDbPktRxLlidSet(ONU_DEF_RX_LLID, macId);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponOnuGenTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database MAC table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponOnuMacTblInit(void)
{
  MV_U32 macId;
  MV_U8  macAddrSkeleton[MV_MAC_ADDR_SIZE];

  memcpy(macAddrSkeleton, mvMacAddr[2], MV_MAC_ADDR_SIZE);
  printk("macAddrSkeleton = %x:%x:%x:%x:%x:%x\n\r", 
         macAddrSkeleton[0], macAddrSkeleton[1], macAddrSkeleton[2],
         macAddrSkeleton[3], macAddrSkeleton[4], macAddrSkeleton[5]);

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    memcpy(fixMacAddrs[macId], macAddrSkeleton, MV_MAC_ADDR_SIZE);

    macAddrSkeleton[5]++;
    if (macAddrSkeleton[5] == 0x10) 
        macAddrSkeleton[5] = 0;

    printk("mac -%d- macAddr = %x:%x:%x:%x:%x:%x\n\r", macId,
           fixMacAddrs[macId][0], fixMacAddrs[macId][1], fixMacAddrs[macId][2],
           fixMacAddrs[macId][3], fixMacAddrs[macId][4], fixMacAddrs[macId][5]);
  }

  return(MV_OK); 
}

/*******************************************************************************
**
**  onuEponOnuGenTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database general table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK
**                   
*******************************************************************************/
MV_STATUS onuEponOnuGenTblInit(void)
{
  MV_U32 macAddrLow;
  MV_U32 macAddrHigh;
  MV_U32 macId; 

  /* clear onu database general table */
  memset ((&(onuEponDb_s.onuEponGenTbl_s)), 0, sizeof (S_OnuEponGenTbl));

  /* set onu control type to MPCP - 8808 */
  onuEponDbOnuCtrlTypeSet(ONU_MPCP_CTRL_TYPE);

  /* set onu broadcast address */
  onuEponDbOnuBcastAddrSet(ONU_BROADCAST_ADDR_LOW, ONU_BROADCAST_ADDR_HIGH);

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    /* set onu to valid */
    onuEponDbOnuValidSet(MV_TRUE, macId);

    /* set onu to state 01 */
    onuEponDbOnuStateSet(ONU_EPON_02_REGISTER_PENDING, macId);

    /* set onu sync time to 0 */
    onuEponDbOnuSyncTimeSet(0, macId);

    /* set onu mac address */
    macAddrLow  = ((fixMacAddrs[macId][5] & 0xFF)      ) | 
                  ((fixMacAddrs[macId][4] & 0xFF) <<  8) | 
                  ((fixMacAddrs[macId][3] & 0xFF) << 16) | 
                  ((fixMacAddrs[macId][2] & 0xFF) << 24);  
    macAddrHigh = ((fixMacAddrs[macId][1] & 0xFF)      ) | 
                  ((fixMacAddrs[macId][0] & 0xFF) << 8 ); 

    onuEponDbOnuMacAddrSet(macAddrLow, macAddrHigh, macId);  
  }

  /* set onu configuration */
  onuEponDbOnuCfgSet(ONU_REG_REQ_AUTO_RES, 
                     ONU_REG_ACK_SW_RES, 
                     ONU_REPORT_AUTO_RES, 
                     ONU_RX_PCS_FEC_DIS, 
                     ONU_TX_PCS_FEC_DIS,
                     ONU_TX_FEC_DIS);  

  onuEponDbStatusNotifySet(NULL);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuDatapathTblInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu database data path table
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuDatapathTblInit(void)
{
  MV_U32 macId;

  /* clear onu database data path table */
  memset ((&(onuEponDb_s.onuEponDataPathTbl_s)), 0, sizeof (S_OnuEponDatapathTbl));

  for (macId = 0; macId < EPON_MAX_MAC_NUM; macId++) 
  {
    onuEponDb_s.onuEponDataPathTbl_s.onuEponRxLLID[macId] = ONU_UNUSED_LLID;
    onuEponDb_s.onuEponDataPathTbl_s.onuEponTxLLID[macId] = ONU_UNUSED_LLID;

    onuEponDbPktTxLlidSet(ONU_DEF_TX_LLID, macId);
    onuEponDbPktRxLlidSet(ONU_DEF_RX_LLID, macId);
  }

  onuEponDbPktForwardSet(ONU_SLOW_FRAME_TO_CTRL_QUEUE,  
                         ONU_SLOW_FRAME_TO_RPRT_QUEUE,  
                         ONU_RPRT_FRAME_TO_RPRT_QUEUE,  
                         ONU_RPRT_FRAME_TO_DATA_QUEUE,  
                         ONU_CTRL_FRAME_TO_CTRL_QUEUE,  
                         ONU_CTRL_FRAME_TO_DATA_QUEUE); 

  onuEponDbPktFilterPacketSet(ONU_FORWARD_LLID_ALL_PKT, 
                              ONU_FORWARD_LLID_XXXX_MODE_1_PKT, 
                              ONU_FORWARD_LLID_7FFF_MODE_1_PKT, 
                              ONU_FORWARD_LLID_7FFF_MODE_0_PKT, 
                              ONU_DROP_LLID_NNNN_MODE_1_PKT);

  onuEponDbPktFilterErrorSet(ONU_FORWARD_LLID_LEN_ERR_PKT, 
                             ONU_FORWARD_LLID_GMII_ERR_PKT, 
                             ONU_FORWARD_LLID_FCS_ERR_PKT, 
                             ONU_FORWARD_LLID_CRC_ERR_PKT);

  onuEponDbPktSizeSet(ONU_DEF_MIN_PKT_SIZE, ONU_DEF_MAX_PKT_SIZE);

  return(MV_OK);
}

/******************************************************************************/
/* ========================================================================== */
/*                         Interface Section                                  */
/* ========================================================================== */
/******************************************************************************/

/********************************************/
/* ======================================== */
/*   ONU EPON General Table API Functions   */
/* ======================================== */
/********************************************/
MV_STATUS matchDestAddressToMacId(MV_U8 *destAddr, MV_U32 *macId)
{
  MV_U32 addrIndex;

  for (addrIndex = 0; addrIndex < EPON_MAX_MAC_NUM; addrIndex++) 
  {
    if (memcmp(fixMacAddrs[addrIndex], destAddr, 6) == 0)
    {
      *macId = addrIndex;
      return(MV_OK);
    }
  }

  return(MV_ERROR);
}

/*******************************************************************************
**
**  onuEponDbOnuValidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu valid in the database
**               
**  PARAMETERS:  MV_BOOL onuValid  
**               MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuValidSet(MV_BOOL onuValid, MV_U32 macId)
{
  onuEponDb_s.onuEponGenTbl_s.onuEponValid[macId] = onuValid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuValidGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu valid 
**               
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     onu state
**                   
*******************************************************************************/
MV_BOOL onuEponDbOnuValidGet(MV_U32 macId)
{
  return(onuEponDb_s.onuEponGenTbl_s.onuEponValid[macId]);
}

/*******************************************************************************
**
**  onuEponDbOnuStateSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu state in the database
**               
**  PARAMETERS:  MV_U32 onuState - ONU_EPON_02_REGISTER_PENDING
**                                 ONU_EPON_03_OPERATION         
**               MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuStateSet(MV_U32 onuState, MV_U32 macId)
{
  if ((onuState < ONU_EPON_02_REGISTER_PENDING) ||
      (onuState > ONU_EPON_03_OPERATION))
  {
    return(MV_ERROR);
  }

  onuEponDb_s.onuEponGenTbl_s.onuEponState[macId] = onuState;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuStateGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu state 
**               
**  PARAMETERS:  MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     onu state
**                   
*******************************************************************************/
MV_U32 onuEponDbOnuStateGet(MV_U32 macId)
{
  return(onuEponDb_s.onuEponGenTbl_s.onuEponState[macId]);
}

/*******************************************************************************
**
**  onuEponDbOnuCtrlTypeSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu ether type in the database
**               
**  PARAMETERS:  MV_U32 ctrlType 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuCtrlTypeSet(MV_U32 ctrlType)
{
  onuEponDb_s.onuEponGenTbl_s.onuEponCtrlType = ctrlType;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuCtrlTypeGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu ether type 
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     ctrl frame ether type
**                   
*******************************************************************************/
MV_U32 onuEponDbOnuCtrlTypeGet(void)
{
  return(onuEponDb_s.onuEponGenTbl_s.onuEponCtrlType);
}

/*******************************************************************************
**
**  onuEponDbOnuSyncTimeSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu sync time in the database
**               
**  PARAMETERS:  MV_U32 syncTime
**               MV_U32 macId 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuSyncTimeSet(MV_U32 syncTime, MV_U32 macId)
{
  onuEponDb_s.onuEponGenTbl_s.onuEponSyncTime[macId] = syncTime;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuSyncTimeGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu sync time
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     onu sync time
**                   
*******************************************************************************/
MV_U32 onuEponDbOnuSyncTimeGet(MV_U32 macId)
{
  return(onuEponDb_s.onuEponGenTbl_s.onuEponSyncTime[macId]);
}

/*******************************************************************************
**
**  onuEponDbOnuMacAddrSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu mac address in the database
**               
**  PARAMETERS:  MV_U32 lowAddr
**               MV_U32 highAddr
**               MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuMacAddrSet(MV_U32 lowAddr, MV_U32 highAddr, MV_U32 macId)
{
  onuEponDb_s.onuEponGenTbl_s.onuEponMacAddr[macId].addressLow  = lowAddr;
  onuEponDb_s.onuEponGenTbl_s.onuEponMacAddr[macId].addressHigh = highAddr;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuMacAddrGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu mac address in the database
**               
**  PARAMETERS:  MV_U32 *lowAddr
**               MV_U32 *highAddr
**               MV_U32 macId
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuMacAddrGet(MV_U32 *lowAddr, MV_U32 *highAddr, MV_U32 macId)
{
  *lowAddr  = onuEponDb_s.onuEponGenTbl_s.onuEponMacAddr[macId].addressLow; 
  *highAddr = onuEponDb_s.onuEponGenTbl_s.onuEponMacAddr[macId].addressHigh; 

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuBcastAddrSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu mac broadcast address in the database
**               
**  PARAMETERS:  MV_U32 lowAddr
**               MV_U32 highAddr
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuBcastAddrSet(MV_U32 lowAddr, MV_U32 highAddr)
{
  onuEponDb_s.onuEponGenTbl_s.onuEponBcastAddr.addressLow  = lowAddr;
  onuEponDb_s.onuEponGenTbl_s.onuEponBcastAddr.addressHigh = highAddr;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuMacAddrGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu mac broadcast address in the database
**               
**  PARAMETERS:  MV_U32 *lowAddr
**               MV_U32 *highAddr
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuBcastAddrGet(MV_U32 *lowAddr, MV_U32 *highAddr)
{
  *lowAddr  = onuEponDb_s.onuEponGenTbl_s.onuEponBcastAddr.addressLow; 
  *highAddr = onuEponDb_s.onuEponGenTbl_s.onuEponBcastAddr.addressHigh; 

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuCfgSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu mac configuration in the database
**               
**  PARAMETERS:  MV_U32 regReqAutoRes
**               MV_U32 regAckAutoRes
**               MV_U32 reportAutoRes
**               MV_U32 rxPcsFecEn        
**               MV_U32 txPcsFecEn   
**               MV_U32 txFecEn     
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuCfgSet(MV_U32 regReqAutoRes, 
                             MV_U32 regAckAutoRes, 
                             MV_U32 reportAutoRes, 
                             MV_U32 rxPcsFecEn, 
                             MV_U32 txPcsFecEn, 
                             MV_U32 txFecEn)
{
  onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponRegReqAutoRes = regReqAutoRes;
  onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponRegAckAutoRes = regAckAutoRes;
  onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponReportAutoRes = reportAutoRes;
  onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponRxPcsFecEn    = rxPcsFecEn;      
  onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponTxPcsFecEn    = txPcsFecEn;      
  onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponTxFecEn       = txFecEn;      

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbOnuCfgGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu mac configuration in the database
**               
**  PARAMETERS:  MV_U32 regReqAutoRes
**               MV_U32 regAckAutoRes
**               MV_U32 reportAutoRes
**               MV_U32 rxFec        
**               MV_U32 txFec        
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbOnuCfgGet(MV_U32 *regReqAutoRes, 
                             MV_U32 *regAckAutoRes, 
                             MV_U32 *reportAutoRes, 
                             MV_U32 *rxPcsFecEn, 
                             MV_U32 *txPcsFecEn,
                             MV_U32 *txFecEn)
{
  *regReqAutoRes = onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponRegReqAutoRes;
  *regAckAutoRes = onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponRegAckAutoRes;
  *reportAutoRes = onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponReportAutoRes;
  *rxPcsFecEn    = onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponRxPcsFecEn;      
  *txPcsFecEn    = onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponTxPcsFecEn;      
  *txFecEn       = onuEponDb_s.onuEponGenTbl_s.onuEponCfg.onuEponTxFecEn;
                  
  return(MV_OK);
}

/********************************************/
/* ======================================== */
/*   ONU EPON DataPath Table API Functions  */
/* ======================================== */
/********************************************/

/*******************************************************************************
**
**  onuEponDbPktSizeSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu mac RX packet size
**               
**  PARAMETERS:  MV_U32 minSize 
**               MV_U32 maxSize
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktSizeSet(MV_U32 minSize, MV_U32 maxSize)
{
  onuEponDb_s.onuEponDataPathTbl_s.onuEponRxPktMinSize = minSize;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponRxPktMaxSize = maxSize;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktSizeGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu mac RX packet size
**               
**  PARAMETERS:  MV_U32 *minSize 
**               MV_U32 *maxSize
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktSizeGet(MV_U32 *minSize, MV_U32 *maxSize)
{
 *minSize = onuEponDb_s.onuEponDataPathTbl_s.onuEponRxPktMinSize;  
 *maxSize = onuEponDb_s.onuEponDataPathTbl_s.onuEponRxPktMaxSize;  

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktFilterPacketSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu filter packet configuration
**               
**  PARAMETERS:  MV_U32 forwardLlidAll 
**               MV_U32 forwardLlid1XXX
**               MV_U32 forwardLlid1FFF
**               MV_U32 forwardLlid0FFF
**               MV_U32 dropLlid1NNN
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktFilterPacketSet(MV_U32 forwardLlidAll, 
                                      MV_U32 forwardLlid1XXX, 
                                      MV_U32 forwardLlid1FFF, 
                                      MV_U32 forwardLlid0FFF,
                                      MV_U32 dropLlid1NNN)
{
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlidAll  = forwardLlidAll;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlid1XXX = forwardLlid1XXX;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlid1FFF = forwardLlid1FFF;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlid0FFF = forwardLlid0FFF;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.dropLlid1NNN    = dropLlid1NNN;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktFilterPacketGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu filter packet configuration
**               
**  PARAMETERS:  MV_U32 *forwardLlidAll 
**               MV_U32 *forwardLlid1XXX
**               MV_U32 *forwardLlid1FFF
**               MV_U32 *forwardLlid0FFF
**               MV_U32 *dropLlid1NNN
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktFilterPacketGet(MV_U32 *forwardLlidAll, 
                                      MV_U32 *forwardLlid1XXX, 
                                      MV_U32 *forwardLlid1FFF, 
                                      MV_U32 *forwardLlid0FFF,
                                      MV_U32 *dropLlid1NNN)
{
  *forwardLlidAll  = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlidAll; 
  *forwardLlid1XXX = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlid1XXX;
  *forwardLlid1FFF = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlid1FFF;
  *forwardLlid0FFF = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.forwardLlid0FFF;
  *dropLlid1NNN    = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.dropLlid1NNN;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktFilterErrorSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu filter error configuration
**               
**  PARAMETERS:  MV_U32 ignoreLenErr   
**               MV_U32 ignoreGmiiErr  
**               MV_U32 ignoreFcsErr   
**               MV_U32 ignoreLlidCrcErr
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktFilterErrorSet(MV_U32 ignoreLenErr, 
                                     MV_U32 ignoreGmiiErr, 
                                     MV_U32 ignoreFcsErr, 
                                     MV_U32 ignoreLlidCrcErr)
{
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreLenErr     = ignoreLenErr;   
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreGmiiErr    = ignoreGmiiErr;  
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreFcsErr     = ignoreFcsErr;   
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreLlidCrcErr = ignoreLlidCrcErr;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktFilterErrorGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu filter error configuration
**               
**  PARAMETERS:  MV_U32 *ignoreLenErr   
**               MV_U32 *ignoreGmiiErr  
**               MV_U32 *ignoreFcsErr   
**               MV_U32 *ignoreLlidCrcErr
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktFilterErrorGet(MV_U32 *ignoreLenErr, 
                                     MV_U32 *ignoreGmiiErr, 
                                     MV_U32 *ignoreFcsErr, 
                                     MV_U32 *ignoreLlidCrcErr)
{
  *ignoreLenErr     = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreLenErr;  
  *ignoreGmiiErr    = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreGmiiErr; 
  *ignoreFcsErr     = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreFcsErr;  
  *ignoreLlidCrcErr = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktFilter.ignoreLlidCrcErr; 

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktForwardSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu forwarding configuration
**               
**  PARAMETERS:  MV_U32 slowFrameToCtrlQueue
**               MV_U32 slowFrameToRprtQueue
**               MV_U32 rprtFrameToRprtQueue
**               MV_U32 rprtFrameToDataQueue
**               MV_U32 ctrlFrameToCtrlQueue
**               MV_U32 ctrlFrameToDataQueue
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktForwardSet(MV_U32 slowFrameToCtrlQueue, 
                                 MV_U32 slowFrameToRprtQueue,
                                 MV_U32 rprtFrameToRprtQueue, 
                                 MV_U32 rprtFrameToDataQueue,
                                 MV_U32 ctrlFrameToCtrlQueue, 
                                 MV_U32 ctrlFrameToDataQueue)
{
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.slowFrameToCtrlQueue = slowFrameToCtrlQueue;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.slowFrameToRprtQueue = slowFrameToRprtQueue;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.rprtFrameToRprtQueue = rprtFrameToRprtQueue;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.rprtFrameToDataQueue = rprtFrameToDataQueue;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.ctrlFrameToCtrlQueue = ctrlFrameToCtrlQueue;
  onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.ctrlFrameToDataQueue = ctrlFrameToDataQueue;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktForwardGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu forwarding configuration
**               
**  PARAMETERS:  MV_U32 *slowFrameToCtrlQueue
**               MV_U32 *slowFrameToRprtQueue
**               MV_U32 *rprtFrameToRprtQueue
**               MV_U32 *rprtFrameToDataQueue
**               MV_U32 *ctrlFrameToCtrlQueue
**               MV_U32 *ctrlFrameToDataQueue
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktForwardGet(MV_U32 *slowFrameToCtrlQueue, 
                                 MV_U32 *slowFrameToRprtQueue,
                                 MV_U32 *rprtFrameToRprtQueue, 
                                 MV_U32 *rprtFrameToDataQueue,
                                 MV_U32 *ctrlFrameToCtrlQueue, 
                                 MV_U32 *ctrlFrameToDataQueue)
{
  *slowFrameToCtrlQueue = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.slowFrameToCtrlQueue;
  *slowFrameToRprtQueue = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.slowFrameToRprtQueue;
  *rprtFrameToRprtQueue = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.rprtFrameToRprtQueue;
  *rprtFrameToDataQueue = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.rprtFrameToDataQueue;
  *ctrlFrameToCtrlQueue = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.ctrlFrameToCtrlQueue;
  *ctrlFrameToDataQueue = onuEponDb_s.onuEponDataPathTbl_s.onuEponPktForward.ctrlFrameToDataQueue;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktRxLlidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu Rx LLID
**               
**  PARAMETERS:  MV_U32 llid 
**               MV_U32 index
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktRxLlidSet(MV_U32 llid, MV_U32 index)
{
  if (index > EPON_MAX_MAC_NUM) 
    return(MV_ERROR);

  onuEponDb_s.onuEponDataPathTbl_s.onuEponRxLLID[index] = llid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktRxLlidGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Rx LLID
**               
**  PARAMETERS:  MV_U32 *llid 
**               MV_U32 index
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktRxLlidGet(MV_U32 *llid, MV_U32 index)
{
  if (index > EPON_MAX_MAC_NUM) 
    return(MV_ERROR);

  *llid = onuEponDb_s.onuEponDataPathTbl_s.onuEponRxLLID[index];

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktTxLlidSet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set onu Tx LLID
**               
**  PARAMETERS:  MV_U32 llid 
**               MV_U32 index
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktTxLlidSet(MV_U32 llid, MV_U32 index)
{
  if (index > EPON_MAX_MAC_NUM) 
    return(MV_ERROR);

  onuEponDb_s.onuEponDataPathTbl_s.onuEponTxLLID[index] = llid;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbPktTxLlidGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Tx LLID
**               
**  PARAMETERS:  MV_U32 *llid 
**               MV_U32 index
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbPktTxLlidGet(MV_U32 *llid, MV_U32 index)
{
  if (index > EPON_MAX_MAC_NUM) 
    return(MV_ERROR);

  *llid = onuEponDb_s.onuEponDataPathTbl_s.onuEponTxLLID[index];

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbStatusNotifySet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set Status Notify Callback in the database
**               
**  PARAMETERS:  STATUSNOTIFYFUNC statusCallback      
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error
**                   
*******************************************************************************/
MV_STATUS onuEponDbStatusNotifySet(STATUSNOTIFYFUNC statusCallback)
{
  onuEponDb_s.onuEponGenTbl_s.onuStatusCallback = statusCallback;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponDbStatusNotifyGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu Status Notify Callback
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     Status Notify Callback
**                   
*******************************************************************************/
STATUSNOTIFYFUNC onuEponDbStatusNotifyGet(void)
{
  return(onuEponDb_s.onuEponGenTbl_s.onuStatusCallback);
}
