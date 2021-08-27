/** \file boot_common.h
 * boot_common.h
 *
 * Header file for the VP-API-II c files.
 *
 * This file contains all of the VP-API-II declarations that are Voice
 * Termination Device (VTD) family independent.	The implementation in this file
 * is applicable to VCP classes of devices.
 *
 * Copyright (c) 2010, Zarlink Semiconductor, Inc.
 *
 * $Revision: 6419 $
 * $LastChangedDate: 2010-02-12 16:40:10 -0600 (Fri, 12 Feb 2010) $
 */

#ifndef BOOT_COMMON_H
#define BOOT_COMMON_H

#include "vp_api_types.h"
#include "hbi_common.h"

/******************************************************************************
 *                              TYPE DEFINITIONS                              *
 ******************************************************************************/

typedef bool (*VpBootSupportFuncPtrType) (VpDeviceIdType deviceId);

/******************************************************************************
 *                             FUNCTION PROTOTYPES                            *
 ******************************************************************************/
/*
 * Initialization functions
 */
EXTERN VpStatusType
VpVcpVppBootLoad(
	VpDevCtxType *pDevCtx,
    VpBootStateType state,
    VpImagePtrType pImageBuffer,
    uint32 bufferSize,
    VpScratchMemType *pScratchMem,
    VpBootModeType validation);

EXTERN VpStatusType
VpBootLoadInternal(
    VpBootStateType state,
    VpImagePtrType pImageBuffer,
    uint32 bufferSize,
    VpScratchMemType *pScratchMem,
    VpBootModeType validation,
    VpDeviceIdType deviceId,
    uint16p pRecCnt,
    VpBootSupportFuncPtrType haltFunc,
    VpBootSupportFuncPtrType validateFunc);

#endif /* BOOT_COMMON_H */
