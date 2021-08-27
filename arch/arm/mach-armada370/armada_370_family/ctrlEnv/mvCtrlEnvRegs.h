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

#ifndef __INCmvCtrlEnvRegsh
#define __INCmvCtrlEnvRegsh

#include "mvCtrlEnvSpec.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* CV Support */
#define PEX0_MEM0 	PEX0_MEM
#define PCI0_MEM0	PEX0_MEM

/* Controller revision info */
#define PCI_CLASS_CODE_AND_REVISION_ID		0x008
#define PCCRIR_REVID_OFFS			0		/* Revision ID */
#define PCCRIR_REVID_MASK			(0xff << PCCRIR_REVID_OFFS)

/* Controler environment registers offsets */
#define MV_TDM_IRQ_NUM				56

/* Coherent Fabric Control and Status */
#define MV_COHERENCY_FABRIC_CTRL_REG		(MV_COHERENCY_FABRIC_OFFSET + 0x0)
#define MV_COHERENCY_FABRIC_CFG_REG		(MV_COHERENCY_FABRIC_OFFSET + 0x4)

/* CIB registers offsets */
#define MV_CIB_CTRL_CFG_REG			(MV_COHERENCY_FABRIC_OFFSET + 0x80)

/* PMU_NFABRIC PMU_NFABRIC PMU_UNIT_SERVICE Units */
#define MV_L2C_NFABRIC_PM_CTRL_CFG_REG		(MV_PMU_NFABRIC_UNIT_SERV_OFFSET + 0x4)
#define MV_L2C_NFABRIC_PM_CTRL_CFG_PWR_DOWN	(1 << 20)

#define MV_L2C_NFABRIC_PWR_DOWN_FLOW_CTRL_REG	(MV_PMU_NFABRIC_UNIT_SERV_OFFSET + 0x8)

#define PM_CONTROL_AND_CONFIG_REG(cpu)		(MV_CPU_PMU_UNIT_SERV_OFFSET(cpu) + 0x4)
#define PM_CONTROL_AND_CONFIG_PWDDN_REQ		(1 << 16)
#define PM_CONTROL_AND_CONFIG_L2_PWDDN		(1 << 20)

#define PM_STATUS_AND_MASK_REG(cpu)		(MV_CPU_PMU_UNIT_SERV_OFFSET(cpu) + 0xc)
#define PM_STATUS_AND_MASK_CPU_IDLE_WAIT	(1 << 16)
#define PM_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT	(1 << 17)
#define PM_STATUS_AND_MASK_IRQ_WAKEUP		(1 << 20)
#define PM_STATUS_AND_MASK_FIQ_WAKEUP		(1 << 21)
#define PM_STATUS_AND_MASK_DBG_WAKEUP		(1 << 22)
#define PM_STATUS_AND_MASK_IRQ_MASK		(1 << 24)
#define PM_STATUS_AND_MASK_FIQ_MASK		(1 << 25)

#define PM_CPU_BOOT_ADDR_REDIRECT(cpu)		(MV_CPU_PMU_UNIT_SERV_OFFSET(cpu) + 0x24)

/* Power Management Memory Power Down Registers 1 - 6 */
#define POWER_MNG_MEM_CTRL_REG(num)		(((num) < 6) ? (0x18210 + ((num) - 2) * 4) : 0x18228)

#define PMC_MCR_NUM_PEX				2
#define PMC_MCR_NUM_CFU				3
#define PMC_MCR_NUM_L2				3
#define PMC_MCR_NUM_CIB				3
#define PMC_MCR_NUM_CPU				3
#define PMC_MCR_NUM_NCS				3
#define PMC_MCR_NUM_DUNIT			3
#define PMC_MCR_NUM_NF				4
#define PMC_MCR_NUM_XOR				4
#define PMC_MCR_NUM_DEVB			4
#define PMC_MCR_NUM_CESA			4
#define PMC_MCR_NUM_USB				4
#define PMC_MCR_NUM_SATA			5
#define PMC_MCR_NUM_AUDIO			5
#define PMC_MCR_NUM_GE				5
#define PMC_MCR_NUM_COMM			6
#define PMC_MCR_NUM_PMU				6

/* #2 */
#define PMC_PEXSTOPMEM_OFFS(port)		(((port) == 0) ? 0 : 12)
#define PMC_PEXSTOPMEM_MASK(port)		(7 << PMC_PEXSTOPMEM_OFFS(port))
#define PMC_PEXSTOPMEM_EN(port)			(0 << PMC_PEXSTOPMEM_OFFS(port))
#define PMC_PEXSTOPMEM_STOP(port)		(1 << PMC_PEXSTOPMEM_OFFS(port))

#define PMC_CPUSTOPMEM_OFFS(id)			21
#define PMC_CPUSTOPMEM_MASK(id)			(7 << PMC_CPUSTOPMEM_OFFS(id))
#define PMC_CPUSTOPMEM_EN(id)			(0 << PMC_CPUSTOPMEM_OFFS(id))
#define PMC_CPUSTOPMEM_STOP(id)			(1 << PMC_CPUSTOPMEM_OFFS(id))

/* #3 */
#define PMC_NCSSTOPMEM_OFFS			24
#define PMC_NCSSTOPMEM_MASK			(7 << PMC_NCSSTOPMEM_OFFS)
#define PMC_NCSSTOPMEM_EN			(0 << PMC_NCSSTOPMEM_OFFS)
#define PMC_NCSSTOPMEM_STOP			(1 << PMC_NCSSTOPMEM_OFFS)

#define PMC_CFUSTOPMEM_OFFS			21
#define PMC_CFUSTOPMEM_MASK			(7 << PMC_CFUSTOPMEM_OFFS)
#define PMC_CFUSTOPMEM_EN			(0 << PMC_CFUSTOPMEM_OFFS)
#define PMC_CFUSTOPMEM_STOP			(1 << PMC_CFUSTOPMEM_OFFS)

#define PMC_L2STOPMEM_OFFS			18
#define PMC_L2STOPMEM_MASK			(7 << PMC_L2STOPMEM_OFFS)
#define PMC_L2STOPMEM_EN			(0 << PMC_L2STOPMEM_OFFS)
#define PMC_L2STOPMEM_STOP			(1 << PMC_L2STOPMEM_OFFS)

#define PMC_CIBSTOPMEM_OFFS			15
#define PMC_CIBSTOPMEM_MASK			(7 << PMC_CIBSTOPMEM_OFFS)
#define PMC_CIBSTOPMEM_EN			(0 << PMC_CIBSTOPMEM_OFFS)
#define PMC_CIBSTOPMEM_STOP			(1 << PMC_CIBSTOPMEM_OFFS)

#define PMC_DUNITSTOPMEM_OFFS			12
#define PMC_DUNITSTOPMEM_MASK			(7 << PMC_DUNITSTOPMEM_OFFS)
#define PMC_DUNITSTOPMEM_EN			(0 << PMC_DUNITSTOPMEM_OFFS)
#define PMC_DUNITSTOPMEM_STOP			(1 << PMC_DUNITSTOPMEM_OFFS)

/* #4 */
#define PMC_NFSTOPMEM_OFFS			27
#define PMC_NFSTOPMEM_MASK			(7 << PMC_NFSTOPMEM_OFFS)
#define PMC_NFSTOPMEM_EN			(0 << PMC_NFSTOPMEM_OFFS)
#define PMC_NFSTOPMEM_STOP			(1 << PMC_NFSTOPMEM_OFFS)

#define PMC_XORSTOPMEM_OFFS(port)		(((port) == 0) ? 15 : 24)
#define PMC_XORSTOPMEM_MASK(port)		(7 << PMC_XORSTOPMEM_OFFS(port))
#define PMC_XORSTOPMEM_EN(port)			(0 << PMC_XORSTOPMEM_OFFS(port))
#define PMC_XORSTOPMEM_STOP(port)		(1 << PMC_XORSTOPMEM_OFFS(port))

#define PMC_DEVBSTOPMEM_OFFS			21
#define PMC_DEVBSTOPMEM_MASK			(7 << PMC_DEVBSTOPMEM_OFFS)
#define PMC_DEVBSTOPMEM_EN			(0 << PMC_DEVBSTOPMEM_OFFS)
#define PMC_DEVBSTOPMEM_STOP			(1 << PMC_DEVBSTOPMEM_OFFS)

#define PMC_CESASTOPMEM_OFFS			18
#define PMC_CESASTOPMEM_MASK			(7 << PMC_CESASTOPMEM_OFFS)
#define PMC_CESASTOPMEM_EN			(0 << PMC_CESASTOPMEM_OFFS)
#define PMC_CESASTOPMEM_STOP			(1 << PMC_CESASTOPMEM_OFFS)

#define PMC_USBSTOPMEM_OFFS(port)		(((port) == 0) ? 3 : 0)
#define PMC_USBSTOPMEM_MASK(port)		(7 << PMC_USBSTOPMEM_OFFS(port))
#define PMC_USBSTOPMEM_EN(port)			(0 << PMC_USBSTOPMEM_OFFS(port))
#define PMC_USBSTOPMEM_STOP(port)		(1 << PMC_USBSTOPMEM_OFFS(port))

/* #5 */
#define PMC_SATASTOPMEM_OFFS(port)		((port) == 0 ? 18 : 24)
#define PMC_SATASTOPMEM_MASK(port)		(0x3F << PMC_SATASTOPMEM_OFFS(port))
#define PMC_SATASTOPMEM_EN(port)		(0 << PMC_SATASTOPMEM_OFFS(port))
#define PMC_SATASTOPMEM_STOP(port)		(9 << PMC_SATASTOPMEM_OFFS(port))

#define PMC_AUSTOPMEM_OFFS			12
#define PMC_AUSTOPMEM_MASK			(7 << PMC_AUSTOPMEM_OFFS)
#define PMC_AUSTOPMEM_EN			(0 << PMC_AUSTOPMEM_OFFS)
#define PMC_AUSTOPMEM_STOP			(1 << PMC_AUSTOPMEM_OFFS)

#define PMC_GESTOPMEM_OFFS(port)		(((port) == 0) ? 9 : 6)
#define PMC_GESTOPMEM_MASK(port)		(7 << PMC_GESTOPMEM_OFFS(port))
#define PMC_GESTOPMEM_EN(port)			(0 << PMC_GESTOPMEM_OFFS(port))
#define PMC_GESTOPMEM_STOP(port)		(1 << PMC_GESTOPMEM_OFFS(port))

/* #6 */
#define PMC_COMMSTOPMEM_OFFS			4
#define PMC_COMMSTOPMEM_MASK			(7 << PMC_COMMSTOPMEM_OFFS)
#define PMC_COMMSTOPMEM_EN			(0 << PMC_COMMSTOPMEM_OFFS)
#define PMC_COMMSTOPMEM_STOP			(1 << PMC_COMMSTOPMEM_OFFS)

#define PMC_PMUSTOPMEM_OFFS			0
#define PMC_PMUSTOPMEM_MASK			(7 << PMC_PMUSTOPMEM_OFFS)
#define PMC_PMUSTOPMEM_EN			(0 << PMC_PMUSTOPMEM_OFFS)
#define PMC_PMUSTOPMEM_STOP			(1 << PMC_PMUSTOPMEM_OFFS)

/*  Power Management Clock Gating Control Register	*/
#define POWER_MNG_CTRL_REG			0x18220

#define PMC_SATASTOPCLOCK_OFFS(ch)		(ch == 0 ? 14 : 29)
#define PMC_SATASTOPCLOCK_MASK(ch)		(3 << PMC_SATASTOPCLOCK_OFFS(ch))
#define PMC_SATASTOPCLOCK_EN(ch)		(3 << PMC_SATASTOPCLOCK_OFFS(ch))
#define PMC_SATASTOPCLOCK_STOP(ch)		(0 << PMC_SATASTOPCLOCK_OFFS(ch))

#define PMC_DDRSTOPCLOCK_OFFS			28
#define PMC_DDRSTOPCLOCK_MASK			(1 << PMC_DDRSTOPCLOCK_OFFS)
#define PMC_DDRSTOPCLOCK_EN			(1 << PMC_DDRSTOPCLOCK_OFFS)
#define PMC_DDRSTOPCLOCK_STOP			(0 << PMC_DDRSTOPCLOCK_OFFS)

#define PMC_TDMSTOPCLOCK_OFFS			25
#define PMC_TDMSTOPCLOCK_MASK			(1 << PMC_TDMSTOPCLOCK_OFFS)
#define PMC_TDMSTOPCLOCK_EN			(1 << PMC_TDMSTOPCLOCK_OFFS)
#define PMC_TDMSTOPCLOCK_STOP			(0 << PMC_TDMSTOPCLOCK_OFFS)

#define PMC_RUNITSTOPCLOCK_OFFS			24
#define PMC_RUNITSTOPCLOCK_MASK			(1 << PMC_RUNITSTOPCLOCK_OFFS)
#define PMC_RUNITSTOPCLOCK_EN			(1 << PMC_RUNITSTOPCLOCK_OFFS)
#define PMC_RUNITSTOPCLOCK_STOP			(0 << PMC_RUNITSTOPCLOCK_OFFS)

#define PMC_CESASTOPCLOCK_OFFS			23
#define PMC_CESASTOPCLOCK_MASK			(1 << PMC_CESASTOPCLOCK_OFFS)
#define PMC_CESASTOPCLOCK_EN			(1 << PMC_CESASTOPCLOCK_OFFS)
#define PMC_CESASTOPCLOCK_STOP			(0 << PMC_CESASTOPCLOCK_OFFS)

#define PMC_XORSTOPCLOCK_OFFS			22
#define PMC_XORSTOPCLOCK_MASK			(1 << PMC_XORSTOPCLOCK_OFFS)
#define PMC_XORSTOPCLOCK_EN				(1 << PMC_XORSTOPCLOCK_OFFS)
#define PMC_XORSTOPCLOCK_STOP			(0 << PMC_XORSTOPCLOCK_OFFS)

#define PMC_USBSTOPCLOCK_OFFS(port)		(18 + (port))
#define PMC_USBSTOPCLOCK_MASK(port)		(1 << PMC_USBSTOPCLOCK_OFFS(port))
#define PMC_USBSTOPCLOCK_EN(port)		(1 << PMC_USBSTOPCLOCK_OFFS(port))
#define PMC_USBSTOPCLOCK_STOP(port)		(0 << PMC_USBSTOPCLOCK_OFFS(port))

#define PMC_SDIOSTOPCLOCK_OFFS			17
#define PMC_SDIOSTOPCLOCK_MASK			(1 << PMC_SDIOSTOPCLOCK_OFFS)
#define PMC_SDIOSTOPCLOCK_EN			(1 << PMC_SDIOSTOPCLOCK_OFFS)
#define PMC_SDIOSTOPCLOCK_STOP			(0 << PMC_SDIOSTOPCLOCK_OFFS)

#define PMC_PEXSTOPCLOCK_OFFS(port)		((port == 0) ? 5 : 9)
#define PMC_PEXSTOPCLOCK_MASK(port)		(1 << PMC_PEXSTOPCLOCK_OFFS(port))
#define PMC_PEXSTOPCLOCK_EN(port)		(1 << PMC_PEXSTOPCLOCK_OFFS(port))
#define PMC_PEXSTOPCLOCK_STOP(port)		(0 << PMC_PEXSTOPCLOCK_OFFS(port))

#define PMC_GESTOPCLOCK_OFFS(port)		(4 - (port))
#define PMC_GESTOPCLOCK_MASK(port)		(1 << PMC_GESTOPCLOCK_OFFS(port))
#define PMC_GESTOPCLOCK_EN(port)		(1 << PMC_GESTOPCLOCK_OFFS(port))
#define PMC_GESTOPCLOCK_STOP(port)		(0 << PMC_GESTOPCLOCK_OFFS(port))

#define PMC_AUSTOPCLOCK_OFFS			(0)
#define PMC_AUSTOPCLOCK_MASK			(1 << PMC_AUSTOPCLOCK_OFFS)
#define PMC_AUSTOPCLOCK_EN				(1 << PMC_AUSTOPCLOCK_OFFS)
#define PMC_AUSTOPCLOCK_STOP			(0 << PMC_AUSTOPCLOCK_OFFS)

#define SATA_IMP_TX_SSC_CTRL_REG(port)		(MV_SATA_REGS_BASE + 0x2810 + (port)*0x2000)
#define SATA_GEN_1_SET_0_REG(port)		(MV_SATA_REGS_BASE + 0x2834 + (port)*0x2000)
#define SATA_GEN_1_SET_1_REG(port)		(MV_SATA_REGS_BASE + 0x2838 + (port)*0x2000)
#define SATA_GEN_2_SET_0_REG(port)		(MV_SATA_REGS_BASE + 0x283C + (port)*0x2000)
#define SATA_GEN_2_SET_1_REG(port)		(MV_SATA_REGS_BASE + 0x2840 + (port)*0x2000)

#define SATA_PWR_PLL_CTRL_REG(port)		(MV_SATA_REGS_BASE + 0x2804 + (port)*0x2000)
#define SATA_DIG_LP_ENA_REG(port)		(MV_SATA_REGS_BASE + 0x288C + (port)*0x2000)
#define SATA_REF_CLK_SEL_REG(port)		(MV_SATA_REGS_BASE + 0x2918 + (port)*0x2000)
#define SATA_PHY_CONTROL_REGISTER(port)		(MV_SATA_REGS_BASE + 0x2920 + (port)*0x2000)
#define SATA_LP_PHY_EXT_CTRL_REG(port)		(MV_SATA_REGS_BASE + 0x2058 + (port)*0x2000)
#define SATA_LP_PHY_EXT_STAT_REG(port)		(MV_SATA_REGS_BASE + 0x205C + (port)*0x2000)

#define SGMII_PWR_PLL_CTRL_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0xE04)
#define SGMII_GEN_1_SET_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0xE34)
#define SGMII_DIG_LP_ENA_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0xE8C)
#define SGMII_REF_CLK_SEL_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0xF18)
#define SGMII_PHY_CTRL_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0xF20)
#define SGMII_SERDES_CFG_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0x4A0)
#define SGMII_SERDES_STAT_REG(port)		(MV_ETH_SGMII_PHY_REGS_BASE(port) + 0x4A4)

#define SERDES_LINE_MUX_REG_0_3			0x18270

/* TDM PLL Control Register */
#define TDM_PLL_CONTROL_REG			0x18770
#define TDM_CLK_ENABLE_OFFS			16
#define TDM_CLK_ENABLE_MASK			(1 << TDM_CLK_ENABLE_OFFS)
#define TDM_FULL_DIV_OFFS			0
#define TDM_FULL_DIV_MASK			(0x1fff << TDM_FULL_DIV_OFFS)
#define TDM_FULL_DIV_8M				(0xB1 << TDM_FULL_DIV_OFFS)
#define TDM_FULL_DIV_4M				(0x162 << TDM_FULL_DIV_OFFS)
#define TDM_FULL_DIV_2M				(0x2C4 << TDM_FULL_DIV_OFFS)

/* Thermal Sensor Registers */
#define TSEN_STATUS_REG				0x18300
#define	TSEN_STATUS_TEMP_OUT_OFFSET		10
#define	TSEN_STATUS_TEMP_OUT_MASK		(0x1FF << TSEN_STATUS_TEMP_OUT_OFFSET)

#define TSEN_CONF_REG				0x18304
#define	TSEN_CONF_START_CAL_MASK		(0x1 << 25)
#define	TSEN_CONF_OTF_CALIB_MASK		(0x1 << 30)
#define	TSEN_CONF_REF_CAL_MASK			(0x1FF << 11)
#define	TSEN_CONF_SOFT_RESET_MASK		(0x1 << 1)

/* Controler environment registers offsets */
#define GEN_PURP_RES_1_REG				0x182F4
#define GEN_PURP_RES_2_REG				0x182F8

#define MPP_CONTROL_REG(id)				(0x18000 + (id * 4))

/* Sample at Reset */
#define MPP_SAMPLE_AT_RESET				(0x18230)

/* SYSRSTn Length Counter */
#define SYSRST_LENGTH_COUNTER_REG		0x18250
#define SLCR_COUNT_OFFS				0
#define SLCR_COUNT_MASK				(0x1FFFFFFF << SLCR_COUNT_OFFS)
#define SLCR_CLR_OFFS				31
#define SLCR_CLR_MASK				(1 << SLCR_CLR_OFFS)

/* Device ID */
#define CHIP_BOND_REG				0x18238
#define PCKG_OPT_MASK				0x3

#define MPP_OUTPUT_DRIVE_REG			0x184E4
#define MPP_GE_A_OUTPUT_DRIVE_OFFS		6
#define MPP_GE_A_1_8_OUTPUT_DRIVE		(0x1 << MPP_GE_A_OUTPUT_DRIVE_OFFS)
#define MPP_GE_A_2_5_OUTPUT_DRIVE		(0x2 << MPP_GE_A_OUTPUT_DRIVE_OFFS)
#define MPP_GE_B_OUTPUT_DRIVE_OFFS		14
#define MPP_GE_B_1_8_OUTPUT_DRIVE		(0x1 << MPP_GE_B_OUTPUT_DRIVE_OFFS)
#define MPP_GE_B_2_5_OUTPUT_DRIVE		(0x2 << MPP_GE_B_OUTPUT_DRIVE_OFFS)

#define MSAR_BOOT_MODE_OFFS			1
#define MSAR_BOOT_MODE_MASK			(0x3F << MSAR_BOOT_MODE_OFFS)
#define MSAR_BOOT_NOR_LIST			{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,	\
						 0x3A, 0x3B, 0xFFFFFFFF}
#define MSAR_BOOT_SPI_LOW_LIST			{0x0, 0x1, 0xFFFFFFFF}
#define MSAR_BOOT_SPI_HIGH_LIST			{0x10, 0x14, 0xFFFFFFFF}
#define MSAR_BOOT_NAND_LIST			{0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x1A, 0x1B, 0x1C,	\
						 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,	\
						 0x27, 0x29, 0x2A, 0x2B, 0x2D, 0x2E, 0x2F, 0xFFFFFFFF}

#define MSAR_TCLK_OFFS				20
#define MSAR_TCLK_MASK				(0x1 << MSAR_TCLK_OFFS)

/* Extract CPU, L2, DDR clocks SAR value from
** SAR bits 24-27
*/
#define MSAR_CPU_CLK_IDX(sar0)			(((sar0) >> 11) & 0xF)
#define MSAR_CPU_CLK_TWSI(sar0)			((((sar0) >> 2)  & 0x7) + (((sar1) & 1) << 3))
#define MSAR_DDR_L2_CLK_RATIO_IDX(sar0)		(((sar0) >> 15) & 0x1F)
#define MSAR_DDR_L2_CLK_RATIO_TWSI(sar0)	(((sar0) >> 1)  & 0xF)

#ifndef MV_ASMLANGUAGE

#define MV_CPU_CLK_TBL { 400, 533, 667, 800, 1000, 1067, 1200, 1333, 1500, 1600, 1667,\
			1800, 2000, 333, 600, 900, 0 }

#define MV_DEFAULT_PCLK		1200000000
#define MV_BOARD_DEFAULT_SYSCLK MV_DEFAULT_PCLK
#define MV_BOARD_DEFAULT_L2CLK	600000000

/*		cpu	l2c	hclk	ddr	*/
#define MV_DDR_L2_CLK_RATIO_TBL    { \
/*00*/	{	1,	1,	4,	2	},\
/*01*/	{	1,	2,	2,	2	},\
/*02*/	{	2,	2,	6,	3	},\
/*03*/	{	2,	2,	3,	3	},\
/*04*/	{	1,	2,	3,	3	},\
/*05*/	{	1,	2,	4,	2	},\
/*06*/	{	1,	1,	2,	2	},\
/*07*/	{	2,	3,	6,	6	},\
/*08*/	{	2,	3,	5,	5	},\
/*09*/	{	1,	2,	6,	3	},\
/*10*/	{	2,	4,	10,	5	},\
/*11*/	{	1,	3,	6,	6	},\
/*12*/	{	1,	2,	4,	4	},\
/*13*/	{	1,	3,	6,	3	},\
/*14*/	{	1,	2,	5,	5	},\
/*15*/	{	2,	2,	5,	5	},\
/*16*/	{	1,	1,	3,	3	},\
/*17*/	{	2,	5,	10,	10	},\
/*18*/	{	1,	3,	8,	4	},\
/*19*/	{	1,	1,	2,	1	},\
/*20*/	{	2,	3,	6,	3	},\
/*21*/	{	1,	2,	8,	4	},\
/*22*/	{	0,	0,	0,	0	},\
/*23*/	{	0,	0,	0,	0	},\
/*24*/	{	0,	0,	0,	0	},\
/*25*/	{	0,	0,	0,	0	},\
/*26*/	{	0,	0,	0,	0	},\
/*27*/	{	1,	1,	1,	1	},\
/*EOT*/	{	0,	0,	0,	0	} \
}

/* These macros help units to identify a target Mport Arbiter group */
#define MV_TARGET_IS_DRAM(target)   \
		((target >= SDRAM_CS0) && (target <= SDRAM_CS3))

#define MV_TARGET_IS_PEX0(target)   \
		((target >= PEX0_MEM) && (target <= PEX0_IO))
#define MV_TARGET_IS_PEX1(target)   \
		((target >= PEX1_MEM) && (target <= PEX1_IO))

#define MV_TARGET_IS_PEX(target)	((target >= PEX0_MEM) && (target <= PEX1_IO))

#define MV_TARGET_IS_DEVICE(target)	((target >= DEVICE_CS0) && (target <= DEVICE_CS3))

#define MV_PCI_DRAM_BAR_TO_DRAM_TARGET(bar)   0

#define MV_CHANGE_BOOT_CS(target) target

#define TCLK_TO_COUNTER_RATIO   1   /* counters running in Tclk */

#define BOOT_TARGETS_NAME_ARRAY {	\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	BOOT_ROM_CS,			\
	TBL_TERM, 			\
	TBL_TERM, 			\
	TBL_TERM,			\
	TBL_TERM,			\
	TBL_TERM,			\
	TBL_TERM,			\
	TBL_TERM,			\
	BOOT_ROM_CS			\
}

#define START_DEV_CS   		DEV_CS0
#define DEV_TO_TARGET(dev)	((dev) + START_DEV_CS)

#define PCI_IF0_MEM0		PEX0_MEM
#define PCI_IF0_IO		PEX0_IO

/* This enumerator defines the Marvell controller target ID  (see Address map) */
typedef enum _mvTargetId {
    DRAM_TARGET_ID	= 0,	/* Port 0 -> DRAM interface		*/
    DEV_TARGET_ID	= 1,	/* Port 1 -> Device port, BootROM, SPI	*/
    PEX0_TARGET_ID	= 4,	/* Port 4 -> PCI Express 0 and 2	*/
    PEX1_TARGET_ID	= 8,	/* Port 4 -> PCI Express 1 and 3	*/
    CRYPT_TARGET_ID	= 9,	/* Port 9 --> Crypto Engine SRAM	*/
    MAX_TARGETS_ID
} MV_TARGET_ID;

/*
	This structure reflect registers:
	Serdes 0-3 selectors	0x18270

	Columns:
	SERDES_UNIT_UNCONNECTED	= 0x0,
	SERDES_UNIT_PEX			= 0x1,
	SERDES_UNIT_SATA		= 0x2,
	SERDES_UNIT_SGMII		= 0x3,
*/

#define SERDES_CFG {	\
	{0,  1,  2,  3}, /* Lane 0 */	\
	{0,  1, -1,  2}, /* Lane 1 */	\
	{0, -1,  1,  2}, /* Lane 2 */	\
	{0, -1,  1,  2}  /* Lane 3 */	\
}

#endif /* MV_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
