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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __INCmvCpuIfRegsh
#define __INCmvCpuIfRegsh

/****************************************/
/* ARM Control and Status Registers Map */
/****************************************/
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "ctrlEnv/mvCtrlEnvRegs.h"

#define MV_CPUIF_REGS_BASE			(MV_CPUIF_REGS_OFFSET(0))
#define MV_MISC_REGS_BASE			(MV_MISC_REGS_OFFSET)
#define MV_L2C_REGS_BASE			(MV_AURORA_L2_REGS_OFFSET)
#define MV_CPUIF_SHARED_REGS_BASE		(MV_MBUS_REGS_OFFSET)
#define MV_COHERENCY_FABRIC_REGS_BASE		(MV_COHERENCY_FABRIC_OFFSET)

#define CPU_CONFIG_REG				(MV_CPUIF_REGS_BASE + 0x800)
#define CPU_CTRL_STAT_REG			(MV_CPUIF_REGS_BASE + 0x808)
#define CPU_RSTOUTN_MASK_REG			(MV_MISC_REGS_BASE + 0x60)
#define CPU_SYS_SOFT_RST_REG			(MV_MISC_REGS_BASE + 0x64)
#define CPU_L2_CTRL_REG				(MV_L2C_REGS_BASE + 0x100)
#define CPU_L2_AUX_CTRL_REG			(MV_L2C_REGS_BASE + 0x104)
#define SOC_CTRL_REG				(MV_MISC_REGS_BASE + 0x4)
#define SOC_DEVICE_MUX_REG			(MV_MISC_REGS_BASE + 0x8)
#define CPU_PEX_IO_CONF_5_REG			(MV_MISC_REGS_OFFSET + 0x2F0)
#define SOC_DEVICE_MUX_NAND_SEL_OFFS		0
#define SOC_COHERENCY_FABRIC_CTRL_REG		(MV_COHERENCY_FABRIC_REGS_BASE)
#define SOC_COHERENCY_FABRIC_CFG_REG		(MV_COHERENCY_FABRIC_REGS_BASE + 0x4)
#define SOC_CIB_CTRL_CFG_REG			(MV_COHERENCY_FABRIC_REGS_BASE + 0x80)

/* ARM Configuration register */
/* CPU_CONFIG_REG (CCR) */

/* Reset vector location */
#define CCR_VEC_INIT_LOC_OFFS			1
#define CCR_VEC_INIT_LOC_MASK			(1 << CCR_VEC_INIT_LOC_OFFS)
/* reset at 0x00000000 */
#define CCR_VEC_INIT_LOC_0000			(0 << CCR_VEC_INIT_LOC_OFFS)
/* reset at 0xFFFF0000 */
#define CCR_VEC_INIT_LOC_FF00			(1 << CCR_VEC_INIT_LOC_OFFS)

#define CCR_ENDIAN_INIT_OFFS			3
#define CCR_ENDIAN_INIT_MASK			(1 << CCR_ENDIAN_INIT_OFFS)
#define CCR_ENDIAN_INIT_LITTLE			(0 << CCR_ENDIAN_INIT_OFFS)
#define CCR_ENDIAN_INIT_BIG			(1 << CCR_ENDIAN_INIT_OFFS)

#define CCR_ARM_ID_SEL_OFFS			4
#define CCR_CPU_ID_SEL_MASK			(1 << CCR_ARM_ID_SEL_OFFS)
#define CCR_CPU_ID_SEL_ARM			(0 << CCR_ARM_ID_SEL_OFFS)
#define CCR_CPU_ID_SEL_MRVL			(1 << CCR_ARM_ID_SEL_OFFS)

#define CCR_TE_INIT_OFFS			5
#define CCR_TE_INIT_MASK			(1 << CCR_TE_INIT_OFFS)
#define CCR_TE_INIT_ARM				(0 << CCR_TE_INIT_OFFS)
#define CCR_TE_INIT_THUMB			(1 << CCR_TE_INIT_OFFS)

#define CCR_NFMI_EN_OFFS			6
#define CCR_NFMI_EN_MASK			(1 << CCR_NFMI_EN_OFFS)
#define CCR_NFMI_EN_DIS				(0 << CCR_NFMI_EN_OFFS)
#define CCR_NFMI_EN_EN				(1 << CCR_NFMI_EN_OFFS)

#define CCR_CORE_MODE_OFFS			9
#define CCR_CORE_MODE_MASK			(3 << CCR_CORE_MODE_OFFS)
#define CCR_CORE_MODE_ARM1176			(0 << CCR_CORE_MODE_OFFS)
#define CCR_CORE_MODE_CORTEX_A8			(1 << CCR_CORE_MODE_OFFS)

#define CCR_UBIT_INIT_OFFS			11
#define CCR_UBIT_INIT_MASK			(1 << CCR_UBIT_INIT_OFFS)
#define CCR_UBIT_INIT_DIS			(0 << CCR_UBIT_INIT_OFFS)
#define CCR_UBIT_INIT_EN			(1 << CCR_UBIT_INIT_OFFS)

#define CCR_PCLK_WFI_OFFS			15
#define CCR_PCLK_WFI_MASK			(1 << CCR_PCLK_WFI_OFFS)
#define CCR_PCLK_WFI_DIS			(0 << CCR_PCLK_WFI_OFFS)
#define CCR_PCLK_WFI_EN				(1 << CCR_PCLK_WFI_OFFS)

#define CCR_SRAM_LOW_LEAK_OFFS			19
#define CCR_SRAM_LOW_LEAK_MASK			(1 << CCR_SRAM_LOW_LEAK_OFFS)
#define CCR_SRAM_LOW_LEAK_EN			(0 << CCR_SRAM_LOW_LEAK_OFFS)
#define CCR_SRAM_LOW_LEAK_DIS			(1 << CCR_SRAM_LOW_LEAK_OFFS)

#define CCR_CLUSTER_ID_OFFS			24
#define CCR_CLUSTER_ID_MASK			(0xF << CCR_SRAM_LOW_LEAK_OFFS)

/* ARM Control and Status register */
/* CPU_CTRL_STAT_REG (CCSR) */
#define CCSR_SMP_N_AMP_OFFS			0
#define CCSR_SMP_N_AMP_MASK			(1 << CCSR_SMP_N_AMP_OFFS)

#define CCSR_ENDIAN_STATUS_OFFS			0
#define CCSR_ENDIAN_STATUS_MASK			(1 << CCSR_ENDIAN_STATUS_OFFS)
#define CCSR_ENDIAN_STATUS_LITTLE		(0 << CCSR_ENDIAN_STATUS_OFFS)
#define CCSR_ENDIAN_STATUS_BIG			(1 << CCSR_ENDIAN_STATUS_OFFS)

/* RSTOUTn Mask Register */
/* CPU_RSTOUTN_MASK_REG (CRMR) */

#define CRMR_SOFT_RST_OUT_OFFS			0
#define CRMR_SOFT_RST_OUT_MASK			(1 << CRMR_SOFT_RST_OUT_OFFS)
#define CRMR_SOFT_RST_OUT_ENABLE		(1 << CRMR_SOFT_RST_OUT_OFFS)
#define CRMR_SOFT_RST_OUT_DISABLE		(0 << CRMR_SOFT_RST_OUT_OFFS)

#define CRMR_PEX_SYSRST_OUT_OFFS(bus)		(1 + ((bus) & 0x3))
#define CRMR_PEX_SYSRST_OUT_MASK(bus)		(1 << CRMR_PEX_SYSRST_OUT_OFFS(bus))
#define CRMR_PEX_SYSRST_OUT_ENABLE(bus)		(1 << CRMR_PEX_SYSRST_OUT_OFFS(bus))
#define CRMR_PEX_SYSRST_OUT_DISABLE(bus)	(0 << CRMR_PEX_SYSRST_OUT_OFFS(bus))

#define CRMR_PEX_TRST_OUT_OFFS(bus)		(5 + ((bus) & 0x3))
#define CRMR_PEX_TRST_OUT_MASK(bus)		(1 << CRMR_PEX_TRST_OUT_OFFS(bus))
#define CRMR_PEX_TRST_OUT_ENABLE(bus)		(1 << CRMR_PEX_TRST_OUT_OFFS(bus))
#define CRMR_PEX_TRST_OUT_DISABLE(bus)		(0 << CRMR_PEX_TRST_OUT_OFFS(bus))

/* System Software Reset Register */
/* CPU_SYS_SOFT_RST_REG (CSSRR) */

#define CSSRR_SYSTEM_SOFT_RST			BIT0

/* CPU_L2_CTRL_REG fields */

#define CL2CR_L2_EN_OFFS			0
#define CL2CR_L2_EN_MASK			(1 << CL2CR_L2_EN_OFFS)

/* CPU_L2_AUX_CTRL_REG fields */
#define CL2ACR_WB_WT_ATTR_OFFS			0
#define CL2ACR_WB_WT_ATTR_MASK			(3 << CL2ACR_WB_WT_ATTR_OFFS)
#define CL2ACR_WB_WT_ATTR_PAGE			(0 << CL2ACR_WB_WT_ATTR_OFFS)
#define CL2ACR_WB_WT_ATTR_WB			(1 << CL2ACR_WB_WT_ATTR_OFFS)
#define CL2ACR_WB_WT_ATTR_WT			(2 << CL2ACR_WB_WT_ATTR_OFFS)

#define CL2ACR_ECC_OFFS				20
#define CL2ACR_ECC_MASK				(1 << CL2ACR_ECC_OFFS)
#define CL2ACR_ECC_EN				(1 << CL2ACR_ECC_OFFS)
#define CL2ACR_ECC_DIS				(0 << CL2ACR_ECC_OFFS)

#define CL2ACR_PARITY_OFFS			21
#define CL2ACR_PARITY_MASK			(1 << CL2ACR_PARITY_OFFS)
#define CL2ACR_PARITY_EN			(1 << CL2ACR_PARITY_OFFS)
#define CL2ACR_PARITY_DIS			(0 << CL2ACR_PARITY_OFFS)

#define CL2ACR_INVAL_UCE_OFFS			22
#define CL2ACR_INVAL_UCE_MASK			(1 << CL2ACR_INVAL_UCE_OFFS)
#define CL2ACR_INVAL_UCE_EN			(1 << CL2ACR_INVAL_UCE_OFFS)
#define CL2ACR_INVAL_UCE_DIS			(0 << CL2ACR_INVAL_UCE_OFFS)

#define CL2ACR_FORCE_WA_OFFS			23
#define CL2ACR_FORCE_WA_MASK			(3 << CL2ACR_FORCE_WA_OFFS)
#define CL2ACR_FORCE_WA_DISABLE			(0 << CL2ACR_FORCE_WA_OFFS)
#define CL2ACR_FORCE_NO_WA			(1 << CL2ACR_FORCE_WA_OFFS)
#define CL2ACR_FORCE_WA				(2 << CL2ACR_FORCE_WA_OFFS)

#define CL2ACR_REP_STRGY_OFFS			27
#define CL2ACR_REP_STRGY_MASK			(3 << CL2ACR_REP_STRGY_OFFS)
#define CL2ACR_REP_STRGY_PLRU_MASK		(1 << 28)

/* SOC_CTRL_REG fields */
#define SCR_PEX_ENA_OFFS(pex)			((pex) & 0x3)
#define SCR_PEX_ENA_MASK(pex)			(1 << pex)

/*******************************************/
/* Main Interrupt Controller Registers Map */
/*******************************************/

#define CPU_MAIN_INT_CAUSE_REG(vec)			(MV_CPUIF_REGS_BASE + 0x880 + (vec * 0x4))

/* Special defines for TWSI HAL. */
#define CPU_MAIN_INT_TWSI_OFFS(i)			(2 + i)
#define CPU_MAIN_INT_CAUSE_TWSI(i)			(31 + i)

#define CPU_CF_LOCAL_MASK_REG				(MV_CPUIF_REGS_BASE + 0x8c4)
#define CPU_INT_SOURCE_CONTROL_REG(i)		(MV_CPUIF_SHARED_REGS_BASE + 0xB00 + (i * 0x4))

#define CPU_INT_SOURCE_CONTROL_ENA_OFFS		28
#define CPU_INT_SOURCE_CONTROL_ENA_MASK		(1 << CPU_INT_SOURCE_CONTROL_ENA_OFFS)

#define CPU_INT_SOURCE_CONTROL_IRQ_OFFS		0
#define CPU_INT_SOURCE_CONTROL_IRQ_MASK		(1 << CPU_INT_SOURCE_CONTROL_IRQ_OFFS)

#define MV_IRQ_NR				116

/*******************************************/
/* ARM Doorbell Registers Map		   */
/*******************************************/

#define CPU_HOST_TO_ARM_DRBL_REG		(MV_CPUIF_REGS_BASE + 0x878)
#define CPU_HOST_TO_ARM_MASK_REG		(MV_CPUIF_REGS_BASE + 0x87C)
#define CPU_ARM_TO_HOST_DRBL_REG		(MV_CPUIF_REGS_BASE + 0x870)
#define CPU_ARM_TO_HOST_MASK_REG		(MV_CPUIF_REGS_BASE + 0x874)

/* CPU control register map */
/* Set bits means value is about to change according to new value */
#define CPU_CONFIG_DEFAULT_MASK				(CCR_VEC_INIT_LOC_MASK)
#define CPU_CONFIG_DEFAULT					(CCR_VEC_INIT_LOC_FF00)

#endif /* __INCmvCpuIfRegsh */
