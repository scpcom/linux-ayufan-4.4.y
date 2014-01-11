/*
 *  linux/include/asm-arm/arch-comcerto/comcerto1000/i2c.h
 *
 *  Copyright (C) 2008 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __ASM_ARCH_COMCERTO1000_I2C_H
#define __ASM_ARCH_COMCERTO1000_I2C_H

#define COMCERTO_I2C_ADDR		(0x00*4)
#define COMCERTO_I2C_DATA		(0x01*4)
#define COMCERTO_I2C_CNTR		(0x02*4)
#define COMCERTO_I2C_STAT		(0x03*4)
#define COMCERTO_I2C_CCRFS		(0x03*4)
#define COMCERTO_I2C_XADDR		(0x04*4)
#define COMCERTO_I2C_CCRH		(0x05*4)
#define COMCERTO_I2C_RESET		(0x07*4)

/* CNTR - Control register bits */
#define CNTR_IEN			(1<<7)
#define CNTR_ENAB			(1<<6)
#define CNTR_STA			(1<<5)
#define CNTR_STP			(1<<4)
#define CNTR_IFLG			(1<<3)
#define CNTR_AAK			(1<<2)

/* STAT - Status codes */
#define STAT_BUS_ERROR			0x00	/* Bus error in master mode only */
#define STAT_START			0x08	/* Start condition transmitted */
#define STAT_START_REPEATED		0x10	/* Repeated Start condition transmited */
#define STAT_ADDR_WR_ACK		0x18	/* Address + Write bit transmitted, ACK received */
#define STAT_ADDR_WR_NACK		0x20	/* Address + Write bit transmitted, NACK received */
#define STAT_DATA_WR_ACK		0x28	/* Data byte transmitted in master mode , ACK received */
#define STAT_DATA_WR_NACK		0x30	/* Data byte transmitted in master mode , NACK received */
#define STAT_ARBIT_LOST			0x38	/* Arbitration lost in address or data byte */
#define STAT_ADDR_RD_ACK		0x40	/* Address + Read bit transmitted, ACK received  */
#define STAT_ADDR_RD_NACK		0x48	/* Address + Read bit transmitted, NACK received  */
#define STAT_DATA_RD_ACK		0x50	/* Data byte received in master mode, ACK transmitted  */
#define STAT_DATA_RD_NACK		0x58	/* Data byte received in master mode, NACK transmitted*/
#define STAT_ARBIT_LOST_ADDR		0x68	/* Arbitration lost in address  */
#define STAT_GENERAL_CALL		0x70	/* General Call, ACK transmitted */
#define STAT_NO_RELEVANT_INFO		0xF8	/* No relevant status information, IFLF=0 */

#endif /* __ASM_ARCH_COMCERTO1000_I2C_H */
