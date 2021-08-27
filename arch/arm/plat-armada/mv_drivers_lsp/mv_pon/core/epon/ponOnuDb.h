/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuDb.h                                                **/
/**                                                                          **/
/**  DESCRIPTION : This file contains ONU EPON database definitions          **/
/**                                                                          **/
/******************************************************************************
 *                                                                            *                              
 *  MODIFICATION HISTORY:                                                     *
 *                                                                            *
 *   26Jan10  oren_ben_hayun    created                                       *  
 * ========================================================================== *      
 *                                                                     
 ******************************************************************************/
#ifndef _ONU_EPON_DB_H
#define _ONU_EPON_DB_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
/******************************************************************************/
/* ========================================================================== */
/*                               Database Definitions                         */
/* ========================================================================== */
/******************************************************************************/

/* EPON MAC State Definitions */
#define ONU_EPON_REGISTERED               (0x01) 
#define ONU_EPON_NOT_REGISTERD            (0x00)
 
/* EPON State Definitions */
#define ONU_EPON_02_REGISTER_PENDING      (0x01) 
#define ONU_EPON_03_OPERATION             (0x02) 
#define ONU_EPON_MAX_NUM_OF_STATE         (4) 

/* EPON Event Definitions */
#define ONU_EPON_REGISTER_MSG_FLAG_ACK    (0x01) 
#define ONU_EPON_REGISTER_MSG_FLAG_DEREG  (0x02) 
#define ONU_EPON_REGISTER_MSG_FLAG_REREG  (0x03) 
#define ONU_EPON_MAX_NUM_OF_EVENT         (8) 

/* EPON Configuration Definitions */
#define ONU_REG_REQ_AUTO_RES              (0x01) 
#define ONU_REG_REQ_SW_RES                (0x00) 
#define ONU_REG_ACK_AUTO_RES              (0x01) 
#define ONU_REG_ACK_SW_RES                (0x00) 
#define ONU_REPORT_AUTO_RES               (0x01) 
#define ONU_REPORT_SW_RES                 (0x00) 
#define ONU_RX_PCS_FEC_EN                 (0x01) 
#define ONU_RX_PCS_FEC_DIS                (0x00) 
#define ONU_TX_PCS_FEC_EN                 (0x01) 
#define ONU_TX_PCS_FEC_DIS                (0x00) 
#define ONU_TX_FEC_DIS                    (0x00) 
#define ONU_RX_DIS                        (0x00) 
#define ONU_RX_EN                         (0x01) 
#define ONU_TX_DIS                        (0x00) 
#define ONU_TX_EN                         (0x01) 

/* EPON Registration Definitions */
#define ONU_REGISTER                      (0x01) 
#define ONU_REGISTER_ACK                  (0x02) 
#define ONU_REGISTER_REQ                  (0x03) 

/* EPON ONU Sync Time Definitions */
#define ONU_DEF_SYNC_TIME_ADD             (0x00)
#define ONU_DEF_SYNC_TIME_DIS_DISCOVER_AUTO (0x01)
#define ONU_DEF_SYNC_TIME_DIS_GATE_AUTO   (0x00)
#define ONU_DEF_SYNC_TIME_FORCE_SW        (0x00)
#define ONU_DEF_SYNC_TIME                 (0x34)

/* EPON ONU DDM Definitions */
#define ONU_DEF_DDM_DELAY                 (0x08)

/* EPON Packet Filtering Definitions */
#define ONU_FORWARD_LLID_ALL_PKT          (0x00) 
#define ONU_FORWARD_LLID_7FFF_MODE_0_PKT  (0x00) 
#define ONU_FORWARD_LLID_7FFF_MODE_1_PKT  (0x01) 
#define ONU_FORWARD_LLID_XXXX_MODE_1_PKT  (0x00) 
#define ONU_DROP_LLID_NNNN_MODE_1_PKT     (0x00) 

/* EPON Error Filtering Definitions */
#define ONU_FORWARD_LLID_CRC_ERR_PKT      (0x00) 
#define ONU_FORWARD_LLID_FCS_ERR_PKT      (0x00) 
#define ONU_FORWARD_LLID_GMII_ERR_PKT     (0x00) 
#define ONU_FORWARD_LLID_LEN_ERR_PKT      (0x00) 

/* EPON Packet Forwarding Definitions */
#define ONU_SLOW_FRAME_TO_CTRL_QUEUE      (0x00) 
#define ONU_SLOW_FRAME_TO_RPRT_QUEUE      (0x00) 
#define ONU_RPRT_FRAME_TO_RPRT_QUEUE      (0x00) 
#define ONU_RPRT_FRAME_TO_DATA_QUEUE      (0x00) 
#define ONU_CTRL_FRAME_TO_CTRL_QUEUE      (0x01) 
#define ONU_CTRL_FRAME_TO_DATA_QUEUE      (0x00) 

/* EPON Ether Type Definitions */
#define ONU_MPCP_CTRL_TYPE                (0x8808) 
#define ONU_OAM_CTRL_TYPE                 (0x8809) 

/* EPON MAC Address Definitions */
#define ONU_BROADCAST_ADDR_LOW            (0xC2000001) 
#define ONU_BROADCAST_ADDR_HIGH           (0x00000180) 
#define ONU_MAC_ADDR_LOW                  (0x09b0302c) 
#define ONU_MAC_ADDR_HIGH                 (0x00000013) 

/* EPON LLID Definitions */
#define ONU_UNUSED_LLID                   (0xFFFF)
#define ONU_DEF_TX_LLID                   (0x07FF)
#define ONU_DEF_RX_LLID                   (0x0000)
#define ONU_LLID_VALUE_MASK               (0x7FFF)
#define ONU_LLID_INDEX_MASK               (0x000F)
#define ONU_LLID_VALID_MASK               (0x0001)
#define ONU_LLID_VALUE_SHIFT              (0)
#define ONU_LLID_INDEX_SHIFT              (15)
#define ONU_LLID_VALID_SHIFT              (19)

/* EPON Laser Definitions */
#define ONU_DEF_LASER_ON_TIME             (5)
#define ONU_DEF_LASER_OFF_TIME            (0)

/* EPON TXM Definitions */
#define ONU_DEF_TXM_CFG_MODE              (0x0)
#define ONU_DEF_TXM_CFG_ALIGNMENT         (0x1)
#define ONU_DEF_TXM_CFG_PRIORITY          (0x0)

/* EPON Packet Size */
#define ONU_DEF_MIN_PKT_SIZE              (64)
#define ONU_DEF_MAX_PKT_SIZE              (2040)

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Typedefs
------------------------------------------------------------------------------*/
typedef void (*EPONFUNCPTR) (MV_U8, MV_U8, MV_U8*);

/******************************************************************************/
/* ========================================================================== */
/*                               Database Definitions                         */
/* ========================================================================== */
/******************************************************************************/

/* ONU EPON General Tables */
typedef struct
{
  MV_U32 onuEponRegReqAutoRes;                          /* Register Request Auto Response */  
  MV_U32 onuEponRegAckAutoRes;                          /* Register Ack Auto Response  */
  MV_U32 onuEponReportAutoRes;                          /* Report Auto Response  */
  MV_U32 onuEponRxPcsFecEn;                             /* Rx PCS FEC Enable */
  MV_U32 onuEponTxPcsFecEn;                             /* Tx PCS FEC Enable */
  MV_U32 onuEponTxFecEn;                                /* Tx FEC Enable */
}S_OnuEponConfig;                                      
                                                       
typedef struct                                         
{                                                      
  MV_U32 addressLow;                                    /* MAC Address Low */  
  MV_U32 addressHigh;                                   /* MAC Address High */
}S_OnuEponMacAddr;                                     
                                                       
typedef struct                                         
{                                                      
  MV_U32           onuEponValid[EPON_MAX_MAC_NUM];      /* ONU Valid for sync */  
  MV_U32           onuEponState[EPON_MAX_MAC_NUM];      /* ONU State */  
  MV_U32           onuEponCtrlType;                     /* ONU Control Type */
  MV_U32           onuEponSyncTime[EPON_MAX_MAC_NUM];   /* ONU Sync Time */
  S_OnuEponConfig  onuEponCfg;                          /* ONU Configuration */
  S_OnuEponMacAddr onuEponMacAddr[EPON_MAX_MAC_NUM];    /* ONU MAC Address */
  S_OnuEponMacAddr onuEponBcastAddr;                    /* ONU MAC Broadcast Address */
  STATUSNOTIFYFUNC onuStatusCallback;
}S_OnuEponGenTbl;                                     

/* ONU EPON Data Path tables */
typedef struct
{
  MV_U32 slowFrameToCtrlQueue;                     
  MV_U32 slowFrameToRprtQueue;                     
  MV_U32 rprtFrameToRprtQueue;                     
  MV_U32 rprtFrameToDataQueue;                     
  MV_U32 ctrlFrameToCtrlQueue;                     
  MV_U32 ctrlFrameToDataQueue;                     
}S_OnuEponRxPktForward;

typedef struct
{
  MV_U32 dropLlid1NNN;
  MV_U32 forwardLlidAll;                     
  MV_U32 forwardLlid1XXX;                     
  MV_U32 forwardLlid1FFF;                     
  MV_U32 forwardLlid0FFF;                     
  MV_U32 ignoreLenErr;                   
  MV_U32 ignoreGmiiErr;                    
  MV_U32 ignoreFcsErr;                    
  MV_U32 ignoreLlidCrcErr;                    
}S_OnuEponRxPktFilter;

typedef struct
{
  MV_U32                onuEponRxPktMinSize;             /* ONU Rx Packet Min Size */
  MV_U32                onuEponRxPktMaxSize;             /* ONU Rx Packet Max Size */
  S_OnuEponRxPktForward onuEponPktForward;               /* ONU Rx Packet Forward */
  S_OnuEponRxPktFilter  onuEponPktFilter;                /* ONU Rx Packet Filter */ 
  MV_U32                onuEponRxLLID[EPON_MAX_MAC_NUM]; /* ONU Rx Packet Rx LLID Array */
  MV_U32                onuEponTxLLID[EPON_MAX_MAC_NUM]; /* ONU Rx Packet Tx LLID Array */
}S_OnuEponDatapathTbl;                                     

/* ONU EPON DataBase */
typedef struct
{
  S_OnuEponGenTbl      onuEponGenTbl_s;
  S_OnuEponDatapathTbl onuEponDataPathTbl_s;
}S_OnuEponDb;

/* Global variables
------------------------------------------------------------------------------*/
/* ONU EPON Database */
extern S_OnuEponDb onuEponDb_s;

/* Global functions
------------------------------------------------------------------------------*/
/* ONU EPON database init function */
MV_STATUS onuEponDbInit(void);
MV_STATUS onuEponDbReInit(MV_U32 macId);

/* ONU EPON general table API functions */
MV_STATUS onuEponDbOnuValidSet(MV_BOOL onuValid, MV_U32 macId);
MV_BOOL   onuEponDbOnuValidGet(MV_U32 macId);
MV_STATUS onuEponDbOnuStateSet(MV_U32 onuState, MV_U32 macId);
MV_U32    onuEponDbOnuStateGet(MV_U32 macId);
MV_STATUS onuEponDbOnuCtrlTypeSet(MV_U32 ctrlType);
MV_U32    onuEponDbOnuCtrlTypeGet(void);
MV_STATUS onuEponDbOnuSyncTimeSet(MV_U32 syncTime, MV_U32 macId);
MV_U32    onuEponDbOnuSyncTimeGet(MV_U32 macId);
MV_STATUS onuEponDbOnuMacAddrSet(MV_U32 lowAddr, MV_U32 highAddr, MV_U32 macId);
MV_STATUS onuEponDbOnuMacAddrGet(MV_U32 *lowAddr, MV_U32 *highAddr, MV_U32 macId);
MV_STATUS onuEponDbOnuBcastAddrSet(MV_U32 lowAddr, MV_U32 highAddr);
MV_STATUS onuEponDbOnuBcastAddrGet(MV_U32 *lowAddr, MV_U32 *highAddr);
MV_STATUS onuEponDbOnuCfgSet(MV_U32 regReqAutoRes, MV_U32 regAckAutoRes, MV_U32 reportAutoRes, 
                             MV_U32 rxPcsFecEn, MV_U32 txPcsFecEn, MV_U32 txFecEn);
MV_STATUS onuEponDbOnuCfgGet(MV_U32 *regReqAutoRes, MV_U32 *regAckAutoRes, MV_U32 *reportAutoRes, 
                             MV_U32 *rxPcsFecEn, MV_U32 *txPcsFecEn, MV_U32 *txFecEn);

/* ONU EPON Data Path table API functions */
MV_STATUS onuEponDbPktSizeSet(MV_U32 minSize, MV_U32 maxSize);
MV_STATUS onuEponDbPktSizeGet(MV_U32 *minSize, MV_U32 *maxSize);
MV_STATUS onuEponDbPktFilterPacketSet(MV_U32 forwardLlidAll, MV_U32 forwardLlid1XXX, 
                                      MV_U32 forwardLlid1FFF, MV_U32 forwardLlid0FFF, MV_U32 dropLlid1NNN);
MV_STATUS onuEponDbPktFilterPacketGet(MV_U32 *forwardLlidAll, MV_U32 *forwardLlid1XXX, 
                                      MV_U32 *forwardLlid1FFF, MV_U32 *forwardLlid0FFF, MV_U32 *dropLlid1NNN);
MV_STATUS onuEponDbPktFilterErrorSet(MV_U32 ignoreLenErr, MV_U32 ignoreGmiiErr, 
                                     MV_U32 ignoreFcsErr, MV_U32 ignoreLlidCrcErr);
MV_STATUS onuEponDbPktFilterErrorGet(MV_U32 *ignoreLenErr, MV_U32 *ignoreGmiiErr, 
                                     MV_U32 *ignoreFcsErr, MV_U32 *ignoreLlidCrcErr);
MV_STATUS onuEponDbPktForwardSet(MV_U32 slowFrameToCtrlQueue, MV_U32 slowFrameToRprtQueue,
                                 MV_U32 rprtFrameToRprtQueue, MV_U32 rprtFrameToDataQueue,
                                 MV_U32 ctrlFrameToCtrlQueue, MV_U32 ctrlFrameToDataQueue);
MV_STATUS onuEponDbPktForwardGet(MV_U32 *slowFrameToCtrlQueue, MV_U32 *slowFrameToRprtQueue,
                                 MV_U32 *rprtFrameToRprtQueue, MV_U32 *rprtFrameToDataQueue,
                                 MV_U32 *ctrlFrameToCtrlQueue, MV_U32 *ctrlFrameToDataQueue);
MV_STATUS onuEponDbPktRxLlidSet(MV_U32 llid, MV_U32 index);
MV_STATUS onuEponDbPktRxLlidGet(MV_U32 *llid, MV_U32 index);
MV_STATUS onuEponDbPktTxLlidSet(MV_U32 llid, MV_U32 index);
MV_STATUS onuEponDbPktTxLlidGet(MV_U32 *llid, MV_U32 index);

MV_STATUS        onuEponDbStatusNotifySet(STATUSNOTIFYFUNC statusCallback);
STATUSNOTIFYFUNC onuEponDbStatusNotifyGet(void);

MV_STATUS matchDestAddressToMacId(MV_U8 *destAddr, MV_U32 *macId);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_EPON_DB_H */
