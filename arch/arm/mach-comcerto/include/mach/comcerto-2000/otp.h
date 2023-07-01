/*
 * arch/arm/mach-comcerto/include/mach/comcerto-2000/otp.h
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
 */

#ifndef __COMCERTO_OTP_H__
#define __COMCERTO_OTP_H__

#include <mach/comcerto-2000.h>

#define OTP_SIZE	8 * 1024

#define OTP_DELAY 1000

#define OTP_CONFIG_LOCK_0		APB_VADDR(COMCERTO_APB_OTP_BASE + 0x00)
#define OTP_CONFIG_LOCK_1		APB_VADDR(COMCERTO_APB_OTP_BASE + 0x04)
#define OTP_CEB_SEQUENCE_LOCKS	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x08)
#define OTP_CEB_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x0C)
#define OTP_RSTB_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x10)
#define OTP_ADDR_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x14)
#define OTP_READEN_INPUT		APB_VADDR(COMCERTO_APB_OTP_BASE + 0x18)
#define OTP_DATA_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x1C)
#define OTP_DLE_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x20)
#define OTP_WEB_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x24)
#define OTP_WEB_COUNTER			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x28)
#define OTP_PGMEN_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x2C)
#define OTP_PGM2CPUMP_COUNTER	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x30)
#define OTP_CPUMPEN_INPUT		APB_VADDR(COMCERTO_APB_OTP_BASE + 0x34)
#define OTP_CPUMP2WEB_COUNTER	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x38)
#define OTP_WEB2CPUMP_COUNTER	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x3C)
#define OTP_CPUMP2PGM_COUNTER	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x40)
#define OTP_CLE_INPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x44)
#define OTP_SECURE_LOCK_OUTPUT	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x48)
#define OTP_DATA_OUT_COUNTER	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x4C)
#define OTP_DATA_OUTPUT			APB_VADDR(COMCERTO_APB_OTP_BASE + 0x50)
#define OTP_HW_SEC_MODE_STATUS	APB_VADDR(COMCERTO_APB_OTP_BASE + 0x54)

#define DOUT_COUNTER_VALUE		0x1F

#endif
