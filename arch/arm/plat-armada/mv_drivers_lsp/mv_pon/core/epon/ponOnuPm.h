/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU GPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuPm.h                                                **/
/**                                                                          **/
/**  DESCRIPTION : This file contains ONU EPON Alarm and Statistics          **/
/**                                                                          **/
/******************************************************************************
 *                                                                            *                              
 *  MODIFICATION HISTORY:                                                     *
 *                                                                            *
 *   26Jan10  oren_ben_hayun    created                                       *  
 * ========================================================================== *  
 *                                                                            
 ******************************************************************************/
#ifndef _ONU_EPON_PM_H
#define _ONU_EPON_PM_H

/* Definitions
------------------------------------------------------------------------------*/ 

/* Enums                              
------------------------------------------------------------------------------*/ 
typedef enum
{
  TOTAL_MPCP_TX_FRAME_CNT             = 0,
  TOTAL_MPCP_TX_ERROR_FRAME_CNT       = 1,
  MAX_EPON_TX_SW_CNT
}E_EponTxSwCnt;

typedef enum
{
  TOTAL_MPCP_RX_FRAME_CNT             = 0,
  TOTAL_MPCP_RX_ERROR_FRAME_CNT       = 1,
  TOTAL_MPCP_REGISTER_ACK_CNT         = 2,
  TOTAL_MPCP_REGISTER_NACK_CNT        = 3,
  TOTAL_MPCP_REGISTER_DEREG_FRAME_CNT = 4,
  TOTAL_MPCP_REGISTER_REREG_FRAME_CNT = 5,
  MAX_EPON_RX_SW_CNT
}E_EponRxSwCnt;

/* Typedefs
------------------------------------------------------------------------------*/
typedef struct
{
  MV_U32 ctrlRegReqFramesCnt; /* Count number of register request frames transmitted     */  
  MV_U32 ctrlRegAckFramesCnt; /* Count number of register acknowledge frames transmitted */  
  MV_U32 reportFramesCnt;     /* Count number of report frames transmitted               */
  MV_U32 dataFramesCnt;       /* Count number of data frames transmitted                 */
  MV_U32 txAllowedBytesCnt;   /* Count number of Tx Byte Allow counter                   */
}S_TxPm;

typedef struct
{
  MV_U32 fcsErrorFramesCnt;   /* Count number of received frames with FCS errors */
  MV_U32 shortFramesCnt;      /* Count number of short frames received           */
  MV_U32 longFramesCnt;       /* Count number of long frames received            */
  MV_U32 dataFramesCnt;       /* Count number of data frames received            */
  MV_U32 ctrlFramesCnt;       /* Count number of control frames received         */
  MV_U32 reportFramesCnt;     /* Count number of report frames received          */
  MV_U32 gateFramesCnt;       /* Count number of gate frames received            */
}S_RxPm;

typedef struct
{
  MV_U32 grantValidCnt;                 /* Count number of valid grant                          */
  MV_U32 grantMaxFutureTimeErrorCnt;    /* Count number of grant max future time error          */
  MV_U32 minProcTimeErrorCnt;           /* Count number of min proc time error                  */
  MV_U32 lengthErrorCnt;                /* Count number of length error                         */
  MV_U32 discoveryAndRegCnt;            /* Count number of discovery & register                 */
  MV_U32 fifoFullErrorCnt;              /* Count number of fifo full error                      */
  MV_U32 opcDiscoveryNotRegBcastCnt;    /* Count number of opc discoveryNotRegBcastCnt          */
  MV_U32 opcRegisterNotDiscoveryCnt;    /* Count number of opc register not discovery           */ 
  MV_U32 opcDiscoveryNotRegNotBcastCnt; /* Count number of opc discovery not register not bcast */ 
  MV_U32 opcDropGrantCnt;               /* Count number of opc drop grant                       */ 
  MV_U32 opcHiddenGrantCnt;             /* Count number of opc hidden grant                     */ 
  MV_U32 opcBackToBackCnt;              /* Count number of opc back to back                     */ 
}S_GpmPm;                               

typedef struct
{
  MV_U32 swTxCnt[MAX_EPON_TX_SW_CNT];
  MV_U32 swRxCnt[MAX_EPON_RX_SW_CNT];
}S_SwPm;

typedef struct
{
  S_RxPm  rx;
  S_TxPm  tx;
  S_SwPm  sw;
  S_GpmPm gpm;
}S_EponPm;

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/
void      onuEponPmInit(void);
void      onuEponPmTimerPmHndl(unsigned long data);
void      onuEponPmTimerExpireHndl(void);
MV_STATUS onuEponPmCountersAdd(void);
MV_STATUS onuEponPmSwRxCountersAdd(MV_U32 cnt, MV_U32 macId);
MV_STATUS onuEponPmSwTxCountersAdd(MV_U32 cnt, MV_U32 macId);
MV_STATUS onuEponPmSwPmGet(S_SwPm *swPm, MV_U32 macId);
MV_STATUS onuEponPmRxPmGet(S_RxPm *rxPm, MV_U32 macId);
MV_STATUS onuEponPmTxPmGet(S_TxPm *txPm, MV_U32 macId);
MV_STATUS onuEponPmGpmPmGet(S_GpmPm *gpmPm, MV_U32 macId);
MV_U32    onuEponPmGpmValidGrantGet(MV_U32 macId);
MV_U32    onuEponPmCtrlCntGet(MV_U32 macId);

MV_STATUS onuEponAlarmSet(MV_U32 alarm, MV_BOOL state);
void      onuEponAlarmGet(MV_U32 *alarm);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_EPON_PM_H */
     