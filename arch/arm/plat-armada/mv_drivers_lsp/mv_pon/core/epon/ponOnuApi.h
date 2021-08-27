/******************************************************************************/
/**                                                                          **/
/**  COPYRIGHT (C) 2000, 2001, Iamba Technologies Ltd.                       **/ 
/**--------------------------------------------------------------------------**/
/******************************************************************************/
/******************************************************************************/
/**                                                                          **/
/**  MODULE      : ONU EPON                                                  **/
/**                                                                          **/
/**  FILE        : ponOnuApi.h                                               **/
/**                                                                          **/
/**  DESCRIPTION : This file contains ONU EPON API definitions               **/
/**                                                                          **/
/******************************************************************************
 *                                                                            *                              
 *  MODIFICATION HISTORY:                                                     *
 *                                                                            *
 *   26Jan10  oren_ben_hayun    created                                       *  
 * ========================================================================== *      
 *                                                                          
 ******************************************************************************/
#ifndef _ONU_EPON_API_H
#define _ONU_EPON_API_H

/* Include Files
------------------------------------------------------------------------------*/
 
/* Definitions
------------------------------------------------------------------------------*/ 

/* Enums                              
------------------------------------------------------------------------------*/ 

/* Structs                              
------------------------------------------------------------------------------*/ 

/* Typedefs
------------------------------------------------------------------------------*/
typedef void (*STATUSNOTIFYFUNC)(MV_U32 status);     

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

/* Notify API
------------------------------------------------------------------------------*/
MV_STATUS onuPonApiStatusNotifyRegister(STATUSNOTIFYFUNC notifyCallBack);

/* Information API
------------------------------------------------------------------------------*/
MV_STATUS onuEponApiInformationGet(S_EponIoctlInfo *onuInfo, MV_U32 macId);
MV_STATUS onuEponApiLinkStateGet(MV_U32 *linkState, MV_U32 macId);

/* FEC API
------------------------------------------------------------------------------*/
MV_STATUS onuEponApiFecConfig(MV_U32 rxGenFecEn, MV_U32 txGenFecEn, MV_U32 *txMacFecEn);

/* Encryption API
------------------------------------------------------------------------------*/
MV_STATUS onuEponApiEncryptionConfig(MV_U32 onuEncryptCfg);

MV_STATUS onuEponApiEncryptionKeyConfig(MV_U32 encryptKeyIndex0, MV_U32 encryptKeyIndex1 , MV_U32 macId);

/* PM API
------------------------------------------------------------------------------*/
MV_STATUS onuEponApiRxPmGet(S_EponIoctlRxPm *rxPm, MV_BOOL clear, MV_U32 macId);
MV_STATUS onuEponApiTxPmGet(S_EponIoctlTxPm *txPm, MV_BOOL clear, MV_U32 macId);
MV_STATUS onuEponApiSwPmGet(S_EponIoctlSwPm *swPm, MV_BOOL clear, MV_U32 macId);
MV_STATUS onuEponApiGpmPmGet(S_EponIoctlGpmPm *gpmPm, MV_BOOL clear, MV_U32 macId);

/* Macros
------------------------------------------------------------------------------*/    

#endif /* _ONU_EPON_API_H */
