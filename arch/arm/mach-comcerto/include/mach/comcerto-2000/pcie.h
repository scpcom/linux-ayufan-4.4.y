/*
 *  linux/include/asm-arm/arch-comcerto/comcerto-2000/pcie.h
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
 */

#ifndef __COMCERTO_PCIE_H__
#define __COMCERTO_PCIE_H__

/* CFG0 Register definitions */
#define DWC_CFG0_DEV_TYPE_MASK 		0x0F
#define 	CFG0_DEV_TYPE_EP 	0x00
#define 	CFG0_DEV_TYPE_LEP 	0x01
#define 	CFG0_DEV_TYPE_RC 	0x04
#define 	CFG0_DEV_TYPE_UP_SW 	0x05
#define 	CFG0_DEV_TYPE_DWN_SW 	0x06

/* CFG1,2 Register definitions */
/*ID definitions of ARMISC*/
#define AXI_OP_TYPE_ID_MASK		0x0000001F
#define AXI_OP_TYPE_ID_BIT		0
#define AXI_OP_BCM_ID			0x00000020
#define AXI_OP_EP_ID			0x00000040
#define AXI_OP_TD_ID			0x00000080
#define AXI_OP_ATTRIBUTE_ID_MASK	0x00000300
#define AXI_OP_ATTRIBUTE_ID_BIT		8
#define AXI_OP_TC_ID_MASK		0x00001C00
#define AXI_OP_TC_ID_BIT		10
#define AXI_OP_MSG_CODE_ID_MASK		0x001FE000
#define AXI_OP_MSG_CODE_ID_BIT		13
#define AXI_OP_DBI_ACCESS_ID		0x00200000
#define AXI_OP_TYPE_MASK	0x1F
#define AXI_OP_TYPE_MEM_RDRW	0
#define AXI_OP_TYPE_MEM_RDRW_LOCKED	1
#define AXI_OP_TYPE_IO_RDRW	2
#define AXI_OP_TYPE_CONFIG_RDRW_TYPE0	4
#define AXI_OP_TYPE_CONFIG_RDRW_TYPE1	5
#define AXI_OP_TYPE_MSG_REQ	16
#define AXI_OP_TYPE_COMPLETION	10
#define AXI_OP_TYPE_COMPLETION_LOCKED	11
#define AXI_OP_TYPE_DBI_ELBI_ENABLE	1



/* CFG5 Register definitions */
#define 	CFG5_APP_INIT_RST	0x01
#define 	CFG5_LTSSM_ENABLE	0x02
#define 	CFG5_APP_RDY_L23	0x04
#define 	CFG5_LINK_DOWN_RST	0x200


/* STS0 Register definitions */
#define 	STS0_LINK_REQ_RST_NOT	0x0001
#define 	STS0_XMLH_LINK_UP	0x8000
#define 	STS0_RDLH_LINK_UP	0x10000

/* INTR_STS and INTR_EN Register definitions */
#define 	INTR_CTRL_INTA_ASSERT	0x0001
#define 	INTR_CTRL_INTA_DEASSERT	0x0002
#define 	INTR_CTRL_INTB_ASSERT	0x0004
#define 	INTR_CTRL_INTB_DEASSERT	0x0008
#define 	INTR_CTRL_INTC_ASSERT	0x0010
#define 	INTR_CTRL_INTC_DEASSERT	0x0020
#define 	INTR_CTRL_INTD_ASSERT	0x0040
#define 	INTR_CTRL_INTD_DEASSERT	0x0080
#define 	INTR_CTRL_AER		0x0100
#define 	INTR_CTRL_PME		0x0200
#define 	INTR_CTRL_HP		0x0400
#define 	INTR_CTRL_LINK_AUTO_BW	0x0800
#define 	INTR_CTRL_MSI		0x1000

/* synopsis specific PCIE configuration registers*/

/* Port Logic Registers */
#define PCIE_ALRT_REG              0x700
#define PCIE_AFL0L1_REG            0x70C
#define PCIE_SYMNUM_REG            0x718
#define PCIE_G2CTRL_REG            0x80C

#define PCIE_CAP_BASE           0x70
#define PCIE_LCAP_REG           (PCIE_CAP_BASE + 0x0C)
#define PCIE_LCNT2_REG          (PCIE_CAP_BASE + 0x30)

	/* MSI interface registers */
#define PCIE_MSI_ADDR_LO	0x820	/* 32 bits */
#define PCIE_MSI_ADDR_HI	0x824	/* 32 bits */
#define PCIE_MSI_INTR0_ENABLE	0x828	/* 32 bits */
#define PCIE_MSI_INTR0_MASK	0x82C	/* 32 bits */
#define PCIE_MSI_INTR0_STATUS	0x830	/* 32 bits */

/* iATU interface registers */
#define PCIE_iATU_VIEW_PORT	0x900	/* 32 bits */
#define PCIE_iATU_CTRL1		0x904   /* 32 bits */
#define PCIE_iATU_CTRL2		0x908	/* 32 bits */
#define PCIE_iATU_SRC_LOW	0x90C	/* 32 bits */
#define PCIE_iATU_SRC_HIGH	0x910	/* 32 bits */
#define PCIE_iATU_LIMIT		0x914	/* 32 bits */
#define PCIE_iATU_TRGT_LOW	0x918	/* 32 bits */
#define PCIE_iATU_TRGT_HIGH	0x91C	/* 32 bits */

/*******************************************************/
/* BAR MASK Registers (uses dbi_cs2)                   */
/*******************************************************/
#define PCIE_BAR0_MASK_REG              0x100010

/* iATU viewport register definitions */
#define iATU_VIEW_PORT_ID_MASK	0x0F
#define iATU_VIEW_PORT_ID_BIT	0
#define iATU_VIEW_PORT_IN_BOUND	0X80000000

/* iATU control1 register definitions */
#define iATU_CTRL1_TYPE_MASK	0x001F
#define iATU_CTRL1_TYPE_BIT	0
#define iATU_CTRL1_TC_MASK	0x00E0
#define iATU_CTRL1_TC_BIT	5
#define iATU_CTRL1_TD		0x100


/* iATU control2 register definitions */
#define iATU_CTRL2_ID_EN		0x80000000
#define iATU_CTRL2_IB_MEM_BAR_MATCH	0x40000000
#define iATU_CTRL2_IB_CFG0_ACCEPT	0x40000000
#define iATU_CTRL2_IB_MATCH_BAR0	0x00000000
#define iATU_CTRL2_IB_MATCH_BAR1	0x00000100
#define iATU_CTRL2_IB_MATCH_BAR2	0x00000200
#define iATU_CTRL2_IB_MATCH_BAR3	0x00000300
#define iATU_CTRL2_IB_MATCH_BAR4	0x00000400
#define iATU_CTRL2_IB_MATCH_BAR5	0x00000500
#define iATU_CTRL2_IB_MATCH_ROM		0x00000600

#define iATU_ENTRY_MAX		8
#define iATU_ENTRY_MEM		0
#define iATU_ENTRY_IO		1
#define iATU_ENTRY_CNF0		2
#define iATU_ENTRY_CNF1		3
#define iATU_ENTRY_MSG		4

/** 254MB,s used for slave memory mapped address space remaining 2MB
 *  is used for IO, CFG0/1 and MSG transalation. This value cann't be
 *  increased.
 */
#define COMCERTO_PCIe_MEM_SIZE				(254 * 1024 *1024)
#define COMCERTO_PCIe_MSG_SIZE				(1 * 1024 * 1024)
#define COMCERTO_PCIe_IO_SIZE				(64 * 1024)
#define COMCERTO_PCIe_CFG0_SIZE				(64 * 1024)
#define COMCERTO_PCIe_CFG1_SIZE				(64 * 1024)

/* Sum of all these space can maximum be 256MB */
#define iATU_MEM_SIZE	(COMCERTO_PCIe_MEM_SIZE)
#define iATU_MSG_SIZE	(COMCERTO_PCIe_MSG_SIZE)
#define iATU_IO_SIZE	(COMCERTO_PCIe_IO_SIZE)
#define iATU_CFG0_SIZE	(COMCERTO_PCIe_CFG0_SIZE)
#define iATU_CFG1_SIZE	(COMCERTO_PCIe_CFG1_SIZE)

#define iATU_GET_MEM_BASE( _base ) (_base + 0)
#define iATU_GET_MSG_BASE( _base ) ( iATU_GET_MEM_BASE ( _base ) + iATU_MEM_SIZE)
#define iATU_GET_IO_BASE( _base ) ( iATU_GET_MSG_BASE( _base ) + iATU_MSG_SIZE)
#define iATU_GET_CFG0_BASE( _base ) ( iATU_GET_IO_BASE( _base ) + iATU_IO_SIZE)
#define iATU_GET_CFG1_BASE( _base ) ( iATU_GET_CFG0_BASE( _base ) + iATU_CFG0_SIZE)

#endif /* __COMCERTO_PCIE_H__ */
