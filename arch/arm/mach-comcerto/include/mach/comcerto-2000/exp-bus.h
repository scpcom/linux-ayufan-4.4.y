/*
 *  arch/arm/mach-comcerto/include/mach/comcerto-2000/exp-bus.h
 *
 *  Copyright (C) 2012 Mindspeed Technologies, Inc.
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

#ifndef __COMCERTO_EXP_BUS_H__
#define __COMCERTO_EXP_BUS_H__


/***** Registers address *****/

#define COMCERTO_EXP_SW_RST_R			APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x000)
#define COMCERTO_EXP_CS_EN_R			APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x004)
#define COMCERTO_EXP_CSx_BASE_R(x)		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x008 + 0x4 * (x))
#define COMCERTO_EXP_CSx_SEG_R(x)		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x01C + 0x4 * (x))
#define COMCERTO_EXP_CSx_CFG_R(x)		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x030 + 0x4 * (x))
#define COMCERTO_EXP_CSx_TMG1_R(x)		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x044 + 0x4 * (x))
#define COMCERTO_EXP_CSx_TMG2_R(x)		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x058 + 0x4 * (x))
#define COMCERTO_EXP_CSx_TMG3_R(x)		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x06C + 0x4 * (x))
#define COMCERTO_EXP_CLOCK_DIV_R		APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x080)
#define COMCERTO_EXP_MFSM_R			APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x100)
#define COMCERTO_EXP_CSFSM_R			APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x104)
#define COMCERTO_EXP_WRSM_R			APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x108)
#define COMCERTO_EXP_RDSM_R			APB_VADDR(COMCERTO_APB_EXPBUS_BASE + 0x10C)

	
/***** Masks *****/

/* EXP_SWRST_R register*/
#define EXP_SW_RST			0x00000001
	
/* EXP_CS_EN_R register*/
#define EXP_CS4_EN			0x00000020
#define EXP_CS3_EN			0x00000010
#define EXP_CS2_EN			0x00000008
#define EXP_CS1_EN			0x00000004
#define EXP_CS0_EN			0x00000002
#define EXP_CSx_EN(x)			(1 << ((x) + 1))
#define EXP_CLK_EN			0x00000001

/* EXP_CSx_CFG_R register*/
#define EXP_RDY_EDG			0x00000800
#define EXP_RDY_EN			0x00000400
#define EXP_NAND_MODE			0x00000200
#define EXP_DM_MODE			0x00000100
#define EXP_STRB_MODE			0x00000080
#define EXP_ALE_MODE			0x00000040
#define EXP_RE_CMD_LVL			0x00000020
#define EXP_WE_CMD_LVL			0x00000010
#define EXP_CS_LEVEL			0x00000008
#define EXP_MEM_BUS_SIZE		0x00000006
#define EXP_MEM_BUS_SIZE_32		0x00000004
#define EXP_MEM_BUS_SIZE_16		0x00000002
#define EXP_MEM_BUS_SIZE_8		0x00000000



#define EXP_CS0_SEG_SIZE		SZ_128M
#define EXP_CS1_SEG_SIZE		SZ_1M
#define EXP_CS2_SEG_SIZE		SZ_1M
#define EXP_CS3_SEG_SIZE		SZ_1M
#define EXP_CS4_SEG_SIZE		SZ_1M

#define EXP_CS0_AXI_BASEADDR		COMCERTO_AXI_EXP_BASE
#define EXP_CS1_AXI_BASEADDR		(EXP_CS0_AXI_BASEADDR + EXP_CS0_SEG_SIZE)
#define EXP_CS2_AXI_BASEADDR		(EXP_CS1_AXI_BASEADDR + EXP_CS1_SEG_SIZE)
#define EXP_CS3_AXI_BASEADDR		(EXP_CS2_AXI_BASEADDR + EXP_CS2_SEG_SIZE)
#define EXP_CS4_AXI_BASEADDR		(EXP_CS3_AXI_BASEADDR + EXP_CS3_SEG_SIZE)

#define EXP_BUS_REG_BASE_CS0		(EXP_CS0_AXI_BASEADDR & ~0xf0000000)
#define EXP_BUS_REG_BASE_CS1		(EXP_CS1_AXI_BASEADDR & ~0xf0000000)
#define EXP_BUS_REG_BASE_CS2		(EXP_CS2_AXI_BASEADDR & ~0xf0000000)
#define EXP_BUS_REG_BASE_CS3		(EXP_CS3_AXI_BASEADDR & ~0xf0000000)
#define EXP_BUS_REG_BASE_CS4		(EXP_CS4_AXI_BASEADDR & ~0xf0000000)

/* EXP_CSx_TMG1_R register */
/* EXP_CSx_TMG2_R register */
/* EXP_CSx_TMG3_R register */
	
/* EXP_CLOCK_DIV_R register */
	
/* EXP_MFSM_R register*/
/* EXP_CSFSM_R register*/
/* EXP_WRFSM_R register*/
/* EXP_RDFSM_R register*/
	
#endif
