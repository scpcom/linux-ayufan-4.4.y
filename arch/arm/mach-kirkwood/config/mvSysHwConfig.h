/******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

*******************************************************************************
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

******************************************************************************/
/******************************************************************************
* mvSysHwCfg.h - Marvell system HW configuration file
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
******************************************************************************/

#ifndef __INCmvSysHwConfigh
#define __INCmvSysHwConfigh

#include "../../../../include/generated/autoconf.h"

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

/****************************************/
/* Soc supporeted Units definitions	*/
/****************************************/

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
#define MV_USB_VOLTAGE_FIX
#endif
#ifdef CONFIG_MV_INCLUDE_TDM
#define MV_INCLUDE_TDM
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
#ifdef CONFIG_MV_INCLUDE_SFLASH_MTD
#define MV_INCLUDE_SFLASH_MTD
#endif
#ifdef CONFIG_MV_INCLUDE_TS
#define MV_INCLUDE_TS
#endif

/* NAND flash stuff */
#ifdef CONFIG_MV_NAND_BOOT
#define MV_NAND_BOOT
#endif
#ifdef CONFIG_MV_NAND
#define MV_NAND
#endif

/* SPI flash stuff */
#ifdef CONFIG_MV_SPI_BOOT
#define MV_SPI_BOOT
#endif

/****************************************************************/
/************* General    configuration ********************/
/****************************************************************/

/* Enable Clock Power Control */
#define MV_INCLUDE_CLK_PWR_CNTRL

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

#if defined(CONFIG_MV_INCLUDE_GIG_ETH)

#ifdef CONFIG_MV_NFP_STATS
#define MV_FP_STATISTICS
#else
#undef MV_FP_STATISTICS
#endif

/* Default configuration for TX_EN workaround: 0 - Disabled, 1 - Enabled */
#define MV_ETH_TX_EN_DEFAULT        0

/* un-comment if you want to perform tx_done from within the poll function */
/* #define ETH_TX_DONE_ISR */

/* put descriptors in uncached memory */
/* #define ETH_DESCR_UNCACHED */

/* Descriptors location: DRAM/internal-SRAM */
#define ETH_DESCR_IN_SDRAM
#undef  ETH_DESCR_IN_SRAM    /* No integrated SRAM in 88Fxx81 devices */

#if defined(ETH_DESCR_IN_SRAM)
#if defined(ETH_DESCR_UNCACHED)
 #define ETH_DESCR_CONFIG_STR    "Uncached descriptors in integrated SRAM"
#else
 #define ETH_DESCR_CONFIG_STR    "Cached descriptors in integrated SRAM"
#endif
#elif defined(ETH_DESCR_IN_SDRAM)
#if defined(ETH_DESCR_UNCACHED)
 #define ETH_DESCR_CONFIG_STR    "Uncached descriptors in DRAM"
#else
 #define ETH_DESCR_CONFIG_STR    "Cached descriptors in DRAM"
#endif
#else
 #error "Ethernet descriptors location undefined"
#endif /* ETH_DESCR_IN_SRAM or ETH_DESCR_IN_SDRAM*/

/* SW Sync-Barrier: not relevant for 88fxx81*/
/* Reasnable to define this macro
	when descriptors in SRAM and buffers in DRAM */
/* In RX the CPU theoretically might see himself as the descriptor owner,      */
/* although the buffer hadn't been written to DRAM yet. Performance cost.      */
/* #define INCLUDE_SYNC_BARR */

/* Buffers cache coherency method (buffers in DRAM) */
#ifndef MV_CACHE_COHER_SW
/* Taken from mvCommon.h */
/* Memory uncached, HW or SW cache coherency is not needed */
#define MV_UNCACHED             0
/* Memory cached, HW cache coherency supported in WriteThrough mode */
#define MV_CACHE_COHER_HW_WT    1
/* Memory cached, HW cache coherency supported in WriteBack mode */
#define MV_CACHE_COHER_HW_WB    2
/* Memory cached, No HW cache coherency, Cache coherency must be in SW */
#define MV_CACHE_COHER_SW       3

#endif

/* DRAM cache coherency configuration */
#define MV_CACHE_COHERENCY  MV_CACHE_COHER_SW

/*No HW coherency in 88Fxx81 devices*/
#define ETHER_DRAM_COHER    MV_CACHE_COHER_SW

#if (ETHER_DRAM_COHER == MV_CACHE_COHER_HW_WB)
 #define ETH_SDRAM_CONFIG_STR    "DRAM HW cache coherency (write-back)"
#elif (ETHER_DRAM_COHER == MV_CACHE_COHER_HW_WT)
 #define ETH_SDRAM_CONFIG_STR    "DRAM HW cache coherency (write-through)"
#elif (ETHER_DRAM_COHER == MV_CACHE_COHER_SW)
 #define ETH_SDRAM_CONFIG_STR    "DRAM SW cache-coherency"
#elif (ETHER_DRAM_COHER == MV_UNCACHED)
#   define ETH_SDRAM_CONFIG_STR  "DRAM uncached"
#else
 #error "Ethernet-DRAM undefined"
#endif /* ETHER_DRAM_COHER */

/****************************************************************/
/************* Ethernet driver configuration ********************/
/****************************************************************/

/* port's default queueus */
#define ETH_DEF_TXQ         0
#define ETH_DEF_RXQ         0

#define MV_DSA_TAG_SIZE		4
#define MV_EDSA_TAG_SIZE	8

#ifndef CONFIG_MV_ETH_RXQ
#define CONFIG_MV_ETH_RXQ
#endif
#define MV_ETH_RX_Q_NUM     CONFIG_MV_ETH_RXQ

#ifndef CONFIG_MV_ETH_TXQ
#define CONFIG_MV_ETH_TXQ   1
#endif
#define MV_ETH_TX_Q_NUM     CONFIG_MV_ETH_TXQ

/* interrupt coalescing setting */
#define ETH_TX_COAL		    200
#define ETH_RX_COAL		    200

/* Checksum offloading */
#define TX_CSUM_OFFLOAD
#define RX_CSUM_OFFLOAD

#endif /* CONFIG_MV_INCLUDE_GIG_ETH */

/****************************************************************/
/*************** Telephony configuration ************************/
/****************************************************************/
#if defined(CONFIG_MV_TDM_SUPPORT)
	#define MV_TDM_SUPPORT
	#define MV_TDM_REGS_BASE	0xD0000
#elif defined(CONFIG_MV_COMM_UNIT_SUPPORT)
	#define MV_COMM_UNIT_SUPPORT
	#define MV_COMM_UNIT_REGS_BASE	MV_COMM_UNIT_REGS_OFFSET
#endif

#ifdef CONFIG_MV_TDM_USE_DCO
	#define MV_TDM_USE_DCO
#endif

/* SLIC vendor */
#if defined(CONFIG_SILABS_SLIC_SUPPORT)
	#define SILABS_SLIC_SUPPORT
	#if defined(CONFIG_SILABS_SLIC_3215)
		#define SILABS_SLIC_3215
	#elif defined(CONFIG_SILABS_SLIC_3217)
		#define SILABS_SLIC_3217
	#elif defined(CONFIG_SILAB_SLIC_SI3226)
		#define SI3226X
	#endif
#else /* CONFIG_ZARLINK_SLIC_SUPPORT) */
	#define ZARLINK_SLIC_SUPPORT
	#define SLIC_TIMER_EVENT_SUPPORT
	#if defined(CONFIG_ZARLINK_SLIC_VE880)
		#define ZARLINK_SLIC_VE880
	#elif defined(CONFIG_ZARLINK_SLIC_VE792)
		#define ZARLINK_SLIC_VE792
	#endif
#endif

#if defined(CONFIG_MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
	#define MV_TDM_USE_EXTERNAL_PCLK_SOURCE
#endif

#if defined(CONFIG_MV_TDM_PCM_CLK_8MHZ)
	#define MV_TDM_PCM_CLK_8MHZ
#elif defined(CONFIG_MV_TDM_PCM_CLK_4MHZ)
	#define MV_TDM_PCM_CLK_4MHZ
#elif defined(CONFIG_MV_TDM_PCM_CLK_2MHZ)
	#define MV_TDM_PCM_CLK_2MHZ
#endif

/* We use the following registers to store DRAM interface pre config */
/* auto-detection results */
/* IMPORTANT: We are using mask register for that purpose. Before writing */
/* to units mask register, make sure main maks register is set to disable */
/* all interrupts */
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

/* Following the pre-configuration registers default values restored after   */
/* auto-detection is done                                                    */
#define DRAM_BUF_REG_DV 0

/* System Mapping */
#define SDRAM_CS0_BASE  0x00000000
#define SDRAM_CS0_SIZE  _256M

#define SDRAM_CS1_BASE  0x10000000
#define SDRAM_CS1_SIZE  _256M

#define SDRAM_CS2_BASE  0x20000000
#define SDRAM_CS2_SIZE  _256M

#define SDRAM_CS3_BASE  0x30000000
#define SDRAM_CS3_SIZE  _256M

/* Device Chip Selects */
#define NFLASH_CS_BASE 0xfa000000
#define NFLASH_CS_SIZE _2M

#define SPI_CS_BASE 0xf4000000
#define SPI_CS_SIZE _16M

#define CRYPT_ENG_BASE	0xf0000000
#define CRYPT_ENG_SIZE	_2M

#define BOOTDEV_CS_BASE	0xff800000
#define BOOTDEV_CS_SIZE _8M

/* CS2 - BOOTROM */
#define DEVICE_CS2_BASE 0xff900000
#define DEVICE_CS2_SIZE _1M

/* Internal registers: size is defined in Controllerenvironment */
#ifdef CONFIG_MV_XCAT98DX_WARM_RESET
/* Warm reset requires default base for internal registers mapping
	(U-Boot have to use same base) */
#define INTER_REGS_BASE	0xD0000000
#else
#define INTER_REGS_BASE	0xF1000000
#endif

/* #define DDR_OPERATION_BASE	(INTER_REGS_BASE | 0x1418) */

/* DRAM detection stuff */
#define MV_DRAM_AUTO_SIZE

/* Board clock detection */
#define TCLK_AUTO_DETECT	/* Use Tclk auto detection   */
#define SYSCLK_AUTO_DETECT	/* Use SysClk auto detection */
#define PCLCK_AUTO_DETECT	/* Use PClk auto detection   */
#define L2CLK_AUTO_DETECT	/* Use L2Clk auto detection   */

#endif /* __INCmvSysHwConfigh */
