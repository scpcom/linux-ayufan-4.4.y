/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#ifndef __INC_xcat98dx_regs_h
#define __INC_xcat98dx_regs_h

/*
 * dev_id[15:10] bits of DeviceID register of Prestera (0x4C)
 * determine the chip type (xCat or xCat2).
 * dev_id[15:10] == 0x37 stands for xCat
 * dev_id[15:10] == 0x39 stands for xCat2
 */
#define MV_PP_CHIP_TYPE_MASK                        0x000FC000
#define MV_PP_CHIP_TYPE_OFFSET                      14
#define MV_PP_CHIP_TYPE_XCAT                        0x37
#define MV_PP_CHIP_TYPE_XCAT2                       0x39

#define PRESTERA_ADDR_COMPLETION_REG                0x0
#define PRESTERA_FTDLL_REG                          0xC
#define PRESTERA_DEV_CFG_REG                        0x28
#define PRESTERA_SAR_REG                            0x2C
#define PRESTERA_LAST_READ_TIMESTAMP_REG            0x40
#define PRESTERA_DEV_ID_REG                         0x4C
#define PRESTERA_VENDOR_ID_REG                      0x50
#define PRESTERA_EXT_GLOBAL_CFG_REG                 0x5C

#define PRESTERA_DEV_CONFIG_REG                     0x28
#define PRESTERA_SAMPLE_AT_RESET_REG                0x2C
#define PRESTERA_GLOBAL_CTRL_REG                    0x58

#define PRESTERA_DRAGONITE_CPU_CTRL_REG             0x88
#define PRESTERA_EXT_GLOBAL_CFG_2_REG               0x8C
#define PRESTERA_ANALOG_CFG_REG                     0x9C

#define MV_VENDOR_ID 0x11AB

#define PP_CPU_PORT_NUM     (63) /* = 0x3F */

#define PRESTERA_CPU_PORT_GLOBAL_CFG_REG            0xA0
#define PRESTERA_CPU_PORT_MAC_CTRL_REG(num) (0xA80FC00 + ((num==3) ? 0x48 : 4*num))
#define PP_CPU_PORT_MAC_CTRL_REG		(0xA80FC00)
#define CASCADE_AND_HEADER_CONFIG_REG               0x0F000004
#define PP_PORT_MAC_CTRL_REG0(port)         (0x0A800000 + port * 0x400)

/*
 * MIB Counters register definitions
 */
#define PRESTERA_MIB_REG_BASE(port)                 (0x04010000 +           \
                                                    ((port / 6) * 0x800000) + \
                                                    ((port % 6) * 0x80))

#define PRESTERA_MIB_GOOD_OCTETS_RECEIVED_LOW       0x0
#define PRESTERA_MIB_GOOD_OCTETS_RECEIVED_HIGH      0x4
#define PRESTERA_MIB_BAD_OCTETS_RECEIVED            0x8
#define PRESTERA_MIB_TX_FIFO_UNDERRUN_AND_CRC       0xc
#define PRESTERA_MIB_GOOD_FRAMES_RECEIVED           0x10
#define PRESTERA_MIB_SENT_DEFERRED                  0x14
#define PRESTERA_MIB_BROADCAST_FRAMES_RECEIVED      0x18
#define PRESTERA_MIB_MULTICAST_FRAMES_RECEIVED      0x1c
#define PRESTERA_MIB_FRAMES_64_OCTETS               0x20
#define PRESTERA_MIB_FRAMES_65_TO_127_OCTETS        0x24
#define PRESTERA_MIB_FRAMES_128_TO_255_OCTETS       0x28
#define PRESTERA_MIB_FRAMES_256_TO_511_OCTETS       0x2c
#define PRESTERA_MIB_FRAMES_512_TO_1023_OCTETS      0x30
#define PRESTERA_MIB_FRAMES_1024_TO_MAX_OCTETS      0x34
#define PRESTERA_MIB_GOOD_OCTETS_SENT_LOW           0x38
#define PRESTERA_MIB_GOOD_OCTETS_SENT_HIGH          0x3c
#define PRESTERA_MIB_GOOD_FRAMES_SENT               0x40
#define PRESTERA_MIB_EXCESSIVE_COLLISION            0x44
#define PRESTERA_MIB_MULTICAST_FRAMES_SENT          0x48
#define PRESTERA_MIB_BROADCAST_FRAMES_SENT          0x4c
#define PRESTERA_MIB_SENT_MULTIPLE                  0x50
#define PRESTERA_MIB_FC_SENT                        0x54
#define PRESTERA_MIB_GOOD_FC_RECEIVED               0x58
#define PRESTERA_MIB_RECEIVED_FIFO_OVERRUN          0x5c
#define PRESTERA_MIB_UNDERSIZE_RECEIVED             0x60
#define PRESTERA_MIB_FRAGMENTS_RECEIVED             0x64
#define PRESTERA_MIB_OVERSIZE_RECEIVED              0x68
#define PRESTERA_MIB_JABBER_RECEIVED                0x6c
#define PRESTERA_MIB_RX_ERROR_FRAME_RECEIVED        0x70

#endif /* __INC_xcat98dx_regs_h */
