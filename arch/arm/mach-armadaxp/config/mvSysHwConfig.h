/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

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
/*******************************************************************************
* mvSysHwCfg.h - Marvell system HW configuration file
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __INCmvSysHwConfigh
#define __INCmvSysHwConfigh

#define CONFIG_MARVELL	1

/* includes */
#define _1K         0x00000400
#define _4K         0x00001000
#define _8K         0x00002000
#define _16K        0x00004000
#define _32K        0x00008000
#define _64K        0x00010000
#define _128K       0x00020000
#define _256K       0x00040000
#define _512K       0x00080000

#define _1M         0x00100000
#define _2M         0x00200000
#define _4M         0x00400000
#define _8M         0x00800000
#define _16M        0x01000000
#define _32M        0x02000000
#define _64M        0x04000000
#define _128M       0x08000000
#define _256M       0x10000000
#define _512M       0x20000000

#define _1G         0x40000000
#define _2G         0x80000000

#ifdef CONFIG_DRAM_IO_RESERVE_BASE
#define MV_DRAM_IO_RESERVE_BASE	CONFIG_DRAM_IO_RESERVE_BASE
#endif

/****************************************/
/* Soc supporeted Units definitions	*/
/****************************************/

#ifdef CONFIG_MV_INCLUDE_PEX
#define MV_INCLUDE_PEX

#ifdef CONFIG_MV_PEX_0_4X1
#define MV_PEX_0_4X1		1
#define MV_PEX_0_1X4		0
#endif
#ifdef CONFIG_MV_PEX_0_1X4
#define MV_PEX_0_4X1		0
#define MV_PEX_0_1X4		1
#endif

#ifdef CONFIG_MV_PEX_1_4X1
#define MV_PEX_1_4X1		1
#define MV_PEX_1_1X4		0
#endif
#ifdef CONFIG_MV_PEX_1_1X4
#define MV_PEX_1_4X1		0
#define MV_PEX_1_1X4		1
#endif

#ifdef CONFIG_MV_PEX_2_4X1
#define MV_PEX_2_4X1		1
#endif

#ifdef CONFIG_MV_PEX_3_4X1
#define MV_PEX_3_4X1		1
#endif

#endif /* CONFIG_MV_INCLUDE_PEX */

#ifdef CONFIG_MV_INCLUDE_PCI
#define MV_INCLUDE_PCI

#define PCI_HOST_BUS_NUM(pciIf)		(pciIf)
#define PCI_HOST_DEV_NUM(pciIf)		0

#define PEX_HOST_BUS_NUM(pciIf)		(pciIf)
#define PEX_HOST_DEV_NUM(pciIf)		0

#endif

#ifdef CONFIG_MV_INCLUDE_TWSI
#define MV_INCLUDE_TWSI
#endif
#ifdef CONFIG_MV_INCLUDE_CESA
#define MV_INCLUDE_CESA
#endif
#ifdef CONFIG_MV_INCLUDE_GIG_ETH
#define MV_INCLUDE_GIG_ETH
#endif
#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
#define MV_INCLUDE_INTEG_SATA
#define MV_INCLUDE_SATA
#endif
#ifdef CONFIG_MV_INCLUDE_USB
#define MV_INCLUDE_USB
//#define MV_USB_VOLTAGE_FIX
#endif
#ifdef CONFIG_MV_INCLUDE_LEGACY_NAND
#define MV_INCLUDE_LEGACY_NAND
#endif
#ifdef CONFIG_MV_INCLUDE_TDM
#define MV_INCLUDE_TDM
#endif
#ifdef CONFIG_MV_INCLUDE_XOR
#define MV_INCLUDE_XOR
#endif
#ifdef CONFIG_MV_INCLUDE_TWSI
#define MV_INCLUDE_TWSI
#endif
#ifdef CONFIG_MV_INCLUDE_UART
#define MV_INCLUDE_UART
#endif
#ifdef CONFIG_MV_INCLUDE_SPI
#define MV_INCLUDE_SPI
#endif
#ifdef CONFIG_MV_INCLUDE_NOR
#define MV_INCLUDE_NOR
#endif
#ifdef CONFIG_MV_INCLUDE_SFLASH_MTD
#define MV_INCLUDE_SFLASH_MTD
#endif
#ifdef CONFIG_MV_INCLUDE_AUDIO
#define MV_INCLUDE_AUDIO
#endif
#ifdef CONFIG_MV_INCLUDE_TS
#define MV_INCLUDE_TS
#endif
#ifdef CONFIG_MV_INCLUDE_SDIO
#define MV_INCLUDE_SDIO
#endif
#ifdef CONFIG_MTD_NAND_LNC_BOOT
#define MTD_NAND_LNC_BOOT
#endif
#ifdef CONFIG_MTD_NAND_LNC
#define MTD_NAND_LNC
#endif
#ifdef CONFIG_MTD_NAND_NFC
#define MTD_NAND_NFC
#endif
#ifdef CONFIG_MTD_NAND_NFC_INIT_RESET
#define MTD_NAND_NFC_INIT_RESET
#endif
#ifdef CONFIG_MTD_NAND_NFC_GANG_SUPPORT
#define MTD_NAND_NFC_GANG_SUPPORT
#endif
#ifdef CONFIG_MTD_NAND_NFC_MLC_SUPPORT
#define MTD_NAND_NFC_MLC_SUPPORT
#endif
#ifdef CONFIG_MTD_NAND_NFC_INIT_RESET
#define MTD_NAND_NFC_INIT_RESET
#endif
#ifdef CONFIG_MTD_NAND_NFC_NEGLECT_RNB
#define MTD_NAND_NFC_NEGLECT_RNB
#endif
#ifdef CONFIG_MV_INCLUDE_PDMA
#define MV_INCLUDE_PDMA
#endif
#ifdef CONFIG_MV_SPI_BOOT
#define MV_SPI_BOOT
#endif
#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY
#define AURORA_IO_CACHE_COHERENCY
#endif

/* convert Definitions for Errata used in the HAL */
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_4413
#define SHEEVA_ERRATA_ARM_CPU_4413
#endif
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_BTS61
#define SHEEVA_ERRATA_ARM_CPU_BTS61
#endif
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_4611
#define SHEEVA_ERRATA_ARM_CPU_4611
#endif

/* convert chip revision definitions */
#define MV88F78X60
#ifdef CONFIG_ARMADA_XP_REV_Z1
#define MV88F78X60_Z1
#endif
#if defined(CONFIG_ARMADA_XP_REV_A0) || defined(CONFIG_ARMADA_XP_A0_WITH_B0)
#define MV88F78X60_A0
#endif
#if defined(CONFIG_ARMADA_XP_REV_B0) && !defined(CONFIG_ARMADA_XP_A0_WITH_B0)
#define MV88F78X60_B0
#endif
/****************************************************************/
/************* General    configuration ********************/
/****************************************************************/

/* Enable Clock Power Control */
#define MV_INCLUDE_CLK_PWR_CNTRL

/* Disable the DEVICE BAR in the PEX */
#define MV_DISABLE_PEX_DEVICE_BAR

/* Allow the usage of early printings during initialization */
#define MV_INCLUDE_EARLY_PRINTK

/****************************************************************/
/************* NFP configuration ********************************/
/****************************************************************/
#define MV_NFP_SEC_Q_SIZE		64
#define MV_NFP_SEC_REQ_Q_SIZE		1000

/****************************************************************/
/************* CESA configuration ********************/
/****************************************************************/

#ifdef MV_INCLUDE_CESA

#define MV_CESA_MAX_CHAN               4

/* Use 2K of SRAM */
#define MV_CESA_MAX_BUF_SIZE           1600

#endif /* MV_INCLUDE_CESA */

/* DRAM cache coherency configuration */
#define MV_CACHE_COHERENCY  MV_CACHE_COHER_SW

/****************************************************************/
/*************** Telephony configuration ************************/
/****************************************************************/
#if defined(CONFIG_MV_TDM_LINEAR_MODE)
 #define MV_TDM_LINEAR_MODE
#elif defined(CONFIG_MV_TDM_ULAW_MODE)
 #define MV_TDM_ULAW_MODE
#endif

#if defined(CONFIG_MV_TDM_5CHANNELS)
 #define MV_TDM_5CHANNELS 
#endif

#if defined(CONFIG_MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
 #define MV_TDM_USE_EXTERNAL_PCLK_SOURCE
#endif

/****************************************************************/
/******************* LPAE configuration *************************/
/****************************************************************/
#ifdef CONFIG_ARM_LPAE
#define ARM_LPAE_SUPPORT
#endif
/* We use the following registers to store DRAM interface pre configuration   */
/* auto-detection results													  */
/* IMPORTANT: We are using mask register for that purpose. Before writing     */
/* to units mask register, make sure main maks register is set to disable     */
/* all interrupts.                                                            */
#define DRAM_BUF_REG0   0x30810 /* sdram bank 0 size            */  
#define DRAM_BUF_REG1   0x30820 /* sdram config                 */
#define DRAM_BUF_REG2   0x30830 /* sdram mode                   */
#define DRAM_BUF_REG3   0x308c4 /* dunit control low            */          
#define DRAM_BUF_REG4   0x60a90 /* sdram address control        */
#define DRAM_BUF_REG5   0x60a94 /* sdram timing control low     */
#define DRAM_BUF_REG6   0x60a98 /* sdram timing control high    */
#define DRAM_BUF_REG7   0x60a9c /* sdram ODT control low        */
#define DRAM_BUF_REG8   0x60b90 /* sdram ODT control high       */
#define DRAM_BUF_REG9   0x60b94 /* sdram Dunit ODT control      */
#define DRAM_BUF_REG10  0x60b98 /* sdram Extended Mode          */
#define DRAM_BUF_REG11  0x60b9c /* sdram Ddr2 Time Low Reg      */
#define DRAM_BUF_REG12  0x60a00 /* sdram Ddr2 Time High Reg     */
#define DRAM_BUF_REG13  0x60a04 /* dunit Ctrl High              */
#define DRAM_BUF_REG14  0x60b00 /* sdram second DIMM exist      */

/* Following the pre-configuration registers default values restored after    */
/* auto-detection is done                                                     */
#define DRAM_BUF_REG_DV 0

/* DRAM detection stuff */
#define MV_DRAM_AUTO_SIZE

/* Default FPGA Clock */
#define MV_FPGA_CLK	25000000
#endif /* __INCmvSysHwConfigh */
