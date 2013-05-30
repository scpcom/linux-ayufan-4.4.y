
/*
 * Copyright (C) Mindspeed Technologies, Inc. 2011. All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * @file serdes.h
 * @brief this header file will contain all required data structure
 *        and function definitions for Snowbush SerDes PHY interface.
 * @date 10/02/2011
 */

#ifndef __COMCERTO_SERDES_H__
#define __COMCERTO_SERDES_H__

/* SER-DES Address space */
#define SD_COMMON_CMU     0x000
#define SD_LANE0          0x200
#define SD_LANE1          0x400
#define SD_LANE2          0x600
#define SD_LANE3          0x800
#define SD_COMMON_LANE    0xA00

#define SD_DEV_TYPE_PCIE	0	
#define	SD_DEV_TYPE_SATA	1
#define	SD_DEV_TYPE_SGMII	2


#define COMCERTO_APB_SERDES0_BASE	APB_VADDR(COMCERTO_APB_SERDES_CONF_BASE)
#define COMCERTO_APB_SERDES1_BASE	(COMCERTO_APB_SERDES0_BASE + 0x4000)
#define COMCERTO_APB_SERDES2_BASE	(COMCERTO_APB_SERDES0_BASE + 0x8000)

#define COMCERTO_DWC1_CFG_BASE 		APB_VADDR(COMCERTO_APB_USBPHY_SERDES_STAT_BASE)
#define COMCERTO_DWC1_SERDES_CFG_BASE 	COMCERTO_DWC1_CFG_BASE + 0x2C
#define SD_PHY_STS_REG_OFST		0x00
#define SD_PHY_CTRL1_REG_OFST		0x04
#define SD_PHY_CTRL2_REG_OFST		0x08
#define SD_PHY_CTRL3_REG_OFST		0x0C


#define COMCERTO_SERDES_REG( _num, _ofst) ((COMCERTO_APB_SERDES0_BASE + (0x4000 * _num)) + _ofst)

#define COMCERTO_SERDES_DWC_CFG_REG( _num, _ofst) ((COMCERTO_DWC1_SERDES_CFG_BASE + (_num <<4)) + _ofst)

#endif //__COMCERTO_SERDES_H__
