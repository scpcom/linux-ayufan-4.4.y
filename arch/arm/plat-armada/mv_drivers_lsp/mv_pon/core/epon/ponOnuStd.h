/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnustd.h                                               **/
/**                                                                          **/
/**  DESCRIPTION : This file contains ONU EPON standard definitions          **/
/**                                                                          **/
/******************************************************************************
 *                                                                            *                              
 *  MODIFICATION HISTORY:                                                     *
 *                                                                            *
 *   26Jan10  oren_ben_hayun    created                                       *  
 * ========================================================================== *      
 *                                                                          
 ******************************************************************************/
#ifndef _ONU_EPON_STD_H
#define _ONU_EPON_STD_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
#define MIN_ETH_FRAME_LEN (64)
#define MAC_ADDR_LEN      (6)
/* Enums                              
------------------------------------------------------------------------------*/ 
typedef enum
{
  MPC_OPCODE_GATE          = 2,     
  MPC_OPCODE_REPORT        = 3,       
  MPC_OPCODE_REGISTER_REQ  = 4,
  MPC_OPCODE_REGISTER      = 5, 
  MPC_OPCODE_REGISTER_ACK  = 6
}E_EponMpcOpcode;

typedef enum
{
  REGISTER_FLAGS_REREG     = 1,     
  REGISTER_FLAGS_DEREG     = 2,
  REGISTER_FLAGS_ACK       = 3,            
  REGISTER_FLAGS_NACK      = 4
}E_EponMpcRegFlag;

typedef enum
{
  REGISTER_ACK_FLAGS_NACK  = 0,     
  REGISTER_ACK_FLAGS_ACK   = 1
}E_EponMpcRegAckFlag;

typedef enum
{
  REGISTER_REQ_FLAGS_REG   = 1,     
  REGISTER_REQ_FLAGS_DEREG = 3
}E_EponMpcRegReqFlag;

/* Typedefs
------------------------------------------------------------------------------*/
#pragma pack(1)
typedef struct
{
  MV_U32 length;
  MV_U8  data[MIN_ETH_FRAME_LEN];
}S_OnuEponCtrlBuffer;
#pragma pack(0)

/******************************************************************************/
/* ========================================================================== */
/*                         General MPC Frame Definitions                      */
/* ========================================================================== */
/******************************************************************************/
#pragma pack(1)
typedef struct
{
  MV_U8  destAddr[MAC_ADDR_LEN];                      
  MV_U8  srcAddr[MAC_ADDR_LEN];                       
  MV_U16 etherType;                        
}S_OnuEponStdEthFrame;                                      
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  MV_U16 opCode;
  MV_U32 timeStamp;
}S_OnuEponGenMpcPdu;                                      
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  S_OnuEponStdEthFrame stdEthFrame;
  S_OnuEponGenMpcPdu   genMpcPdu;  
}S_OnuEponGenMpcFrame;                                      
#pragma pack(0)

/******************************************************************************/
/* ========================================================================== */
/*                         Register MPC Frame Definitions                     */
/* ========================================================================== */
/******************************************************************************/
#pragma pack(1)
typedef struct 
{
  MV_U16 assignedLlidPort;
  MV_U8  flags;              
  MV_U16 syncTime;           
  MV_U8  echoedPendingGrants;
}S_OnuEponRegMpcPdu;                                      
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  S_OnuEponStdEthFrame stdEthFrame;
  S_OnuEponGenMpcPdu   genMpcPdu;
  S_OnuEponRegMpcPdu   regMpcPdu;
}S_OnuEponRegMpcFrame;                                      
#pragma pack(0)

/******************************************************************************/
/* ========================================================================== */
/*                         Register Ack MPC Frame Definitions                 */
/* ========================================================================== */
/******************************************************************************/
#pragma pack(1)
typedef struct 
{
  MV_U8  flags;              
  MV_U16 echoedAssignedport;  
  MV_U16 echoedSyncTime;      
}S_OnuEponRegAckMpcPdu;                                      
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  S_OnuEponStdEthFrame  stdEthFrame;
  S_OnuEponGenMpcPdu    genMpcPdu;
  S_OnuEponRegAckMpcPdu regAckMpcPdu;
}S_OnuEponRegAckMpcFrame;                                      
#pragma pack(0)

/******************************************************************************/
/* ========================================================================== */
/*                         Register Request MPC Frame Definitions             */
/* ========================================================================== */
/******************************************************************************/
#pragma pack(1)
typedef struct 
{
  MV_U8  flags;              
  MV_U8  pendingGrants;  
}S_OnuEponRegReqMpcPdu;                                      
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  S_OnuEponStdEthFrame  stdEthFrame;
  S_OnuEponGenMpcPdu    genMpcPdu;
  S_OnuEponRegReqMpcPdu regReqMpcPdu;
}S_OnuEponRegReqMpcFrame;     
#pragma pack(0)

/******************************************************************************/
/* ========================================================================== */
/*                         Report MPC Frame Definitions                       */
/* ========================================================================== */
/******************************************************************************/
#pragma pack(1)
typedef struct 
{
  MV_U8  numQueueSets;
}S_OnuEponReportMpcPduStart;
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  MV_U16 queueLength;
}S_OnuEponReportQueueReport;
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  MV_U8                      bitMap;
  S_OnuEponReportQueueReport queueReport;
}S_OnuEponReportQueueSetStart;
#pragma pack(0)

#pragma pack(1)
typedef struct 
{
  S_OnuEponStdEthFrame        stdEthFrame;
  S_OnuEponGenMpcPdu          genMpcPdu;
  S_OnuEponReportMpcPduStart  reportMpcPduStart;
  MV_U8                       next;
}S_OnuEponReportMpcFrame;
#pragma pack(0)

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_EPON_STD_H */
