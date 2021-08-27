/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU GPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuIsr.h                                               **/
/**                                                                          **/
/**  DESCRIPTION : This file contains ONU EPON Interrupt implementation      **/
/**                                                                          **/
/******************************************************************************
 *                                                                            *                              
 *  MODIFICATION HISTORY:                                                     *
 *                                                                            *
 *   26Jan10  oren_ben_hayun    created                                       *  
 * ========================================================================== *      
 *                                                                     
 ******************************************************************************/
#ifndef _ONU_EPON_ISR_H
#define _ONU_EPON_ISR_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 
#define ONU_EPON_REGISTERED_LLID_0_MASK     (0x0001)
#define ONU_EPON_REGISTERED_LLID_0_7_MASK   (0x0002)
#define ONU_EPON_TIMESTAMP_DRIFT_MASK       (0x0004)
#define ONU_EPON_TIMESTAMP_VALUE_MATCH_MASK (0x0008)
#define ONU_EPON_RX_CTRL_QUEUE_MASK         (0x0010)
#define ONU_EPON_RX_RPRT_QUEUE_MASK         (0x0020)
#define ONU_EPON_XVR_SD_MASK                (0x0100)
#define ONU_EPON_SERDES_SD_MASK             (0x0200)

#define ONU_EPON_EVENT_SHIFT                (16)
#define ONU_EPON_INTERRUPTS                 (ONU_EPON_RX_CTRL_QUEUE_MASK | \
                                             ONU_EPON_XVR_SD_MASK)
/* Alarms Mask */         
#define ONU_EPON_ALARM_MASK                 (ONU_EPON_SERDES_SD_MASK | \
                                             ONU_EPON_XVR_SD_MASK    | \
                                             ONU_EPON_TIMESTAMP_DRIFT_MASK)
/* Message Types */                       
#define ONU_EPON_RX_CTRL_MSG                (1)
#define ONU_EPON_RX_RPRT_MSG                (2)

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Typedefs
------------------------------------------------------------------------------*/

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/
void onuEponIsrInit(void);
void onuEponIsrLowRoutine(MV_U32 *interruptEvent, MV_U32 *interruptStatus);
void onuEponIsrRoutine(MV_U32 interruptEvent, MV_U32 interruptStatus);
void onuGponIsrTimerMpcpHndl(unsigned long data);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_EPON_ISR_H */
