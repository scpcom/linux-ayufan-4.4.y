/** \file hbi_common.h
 * hbi_common.h
 *
 * This file declares the VCP Host Bus Interface layer register/mail box
 * mapping.
 *
 * Copyright (c) 2010, Zarlink Semiconductor, Inc.
 *
 * $Revision: 6419 $
 * $LastChangedDate: 2010-02-12 16:40:10 -0600 (Fri, 12 Feb 2010) $
 */

#ifndef _HBI_COMMON_H
#define _HBI_COMMON_H

#include "vp_hal.h"

/******************************************************************************
 *                        	LOW_LEVEL HBI DEFINES		                      *
 ******************************************************************************/
#define VP_HBI_PAGE_SIZE            0x80

#define VP_DP 254    /* magic page number indicating direct page */

/* HBI commands: */
#define HBI_CMD_PAGED_READ(offset, length)   (0x0000 + ((uint16)offset << 8) + \
                                                length)
#define HBI_CMD_PAGED_WRITE(offset, length)  (0x0080 + ((uint16)offset << 8) + \
                                                length)
#define HBI_CMD_DIRECT_READ(offset, length)  (0x8000 + ((uint16)offset << 8) + \
                                                length)
#define HBI_CMD_DIRECT_WRITE(offset, length) (0x8080 + ((uint16)offset << 8) + \
                                                length)
#define HBI_CMD_START_MBOX_RD(length)        (0xF800 + length)
#define HBI_CMD_START_MBOX_WR(length)        (0xF900 + length)
#define HBI_CMD_CONT_MBOX_RD(length)         (0xFA00 + length)
#define HBI_CMD_CONT_MBOX_WR(length)         (0xFB00 + length)
#define HBI_CMD_CONFIGURE(options)           (0xFD00 + options)
#define HBI_CMD_SELECT_PAGE(page)            (0xFE00 + page)
#define HBI_CMD_NOP                          0xFFFF

/******************************************************************************
 *                 REGISTERS SHARED BY VCP DEVICES                            *
 ******************************************************************************/
#define HW_Reg_INTIND           VP_DP,  0x00,   0
#define HW_Reg_INTPARAM         VP_DP,  0x01,   0
#define HW_Reg_INTIND_AND_INTPARAM         VP_DP,  0x00,   1
#define HW_Reg_MBOXFLAG         VP_DP,  0x02,   0
#define HW_Reg_CRC255           VP_DP,  0x03,   0
#define HW_Reg_BASE255          VP_DP,  0x04,   0
#define HW_Reg_MBOFFSET         VP_DP,  0x05,   0
#define HW_Reg_HWRES            VP_DP,  0x06,   0
#define HW_Reg_PCLKSEL          VP_DP,  0x07,   0
#define HW_Reg_PCMCLKSLOT       VP_DP,  0x08,   0
#define HW_Reg_SYSINTSTAT       VP_DP,  0x09,   0
#define HW_Reg_SYSINTMASK       VP_DP,  0x0A,   0

/******************************************************************************
 *                        FUNCTION PROTOTYPES			                      *
 ******************************************************************************/
EXTERN bool VpHbiRd8(VpDeviceIdType deviceId, uint8 page, uint8 offset,
                        uint8 words, uint8p pDest);
EXTERN bool VpHbiWr8(VpDeviceIdType deviceId, uint8 page, uint8 offset,
                        uint8 words, uint8p pSrc);
EXTERN bool VpHbiRd(VpDeviceIdType deviceId, uint8 page, uint8 offset,
                        uint8 words, uint16p pDest);
EXTERN bool VpHbiWr(VpDeviceIdType deviceId, uint8 page, uint8 offset,
                        uint8 words, uint16p pSrc);
EXTERN bool VpHbiXfer(bool readWrite, VpDeviceIdType deviceId,
                        uint32 ambaAddr, uint32 length, uint16p data,
                        uint8 byteAddressable,
                        bool (*SetCodeLoadBase)(VpDeviceIdType deviceId, uint32 amba_addr));

EXTERN bool VpHbiVcpVppValidate(VpDeviceIdType deviceId);
EXTERN bool VpHbiVcpVppReset(VpDeviceIdType deviceId);
EXTERN bool VpHbiVcpVppClearCodeMem(VpDeviceIdType deviceId);
EXTERN bool VpHbiVcpVppHalt(VpDeviceIdType deviceId);
EXTERN bool VpHbiVcpVppSetBase255(VpDeviceIdType deviceId, uint32 amba_addr);
EXTERN bool VpHbiVcpVppAmbaRdWr(bool readWrite, VpDeviceIdType deviceId,
                        uint32 amba_addr, uint32 numwords, uint16p data);
EXTERN bool VpHbiSetBase255(VpDeviceIdType deviceId, uint32 amba_addr, uint16p base255reg);
#define VpHbiAmbaRdWr(readWrite, deviceId, amba_addr, numwords, pData) \
    VpHbiVcpVppAmbaRdWr(readWrite, deviceId, amba_addr, numwords, pData)
#define VpHbiAmbaWrite(deviceId, amba_addr, numwords, pData) \
    VpHbiVcpVppAmbaRdWr(TRUE, deviceId, amba_addr, numwords, pData)
#define VpHbiAmbaRead(deviceId, amba_addr, numwords, pData) \
    VpHbiVcpVppAmbaRdWr(FALSE, deviceId, amba_addr, numwords, pData)

#endif /* _HBI_COMMON_H */
