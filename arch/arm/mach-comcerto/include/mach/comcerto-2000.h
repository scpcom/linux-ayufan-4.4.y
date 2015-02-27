/*
 *  arch/arm/arch-comcerto/include/mach/comcerto-2000.h
 *
 *  Copyright (C) 2011 Mindspeed Technologies, Inc.
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

#ifndef __ASM_ARCH_HARDWARE_H
#error "Do not include this directly, instead #include <asm/hardware.h>"
#endif

#ifndef __ASM_ARCH_COMCERTO2000_H__
#define __ASM_ARCH_COMCERTO2000_H__


/*
  * System Clock Frequencies
  */
#ifdef CONFIG_RTSM_C2K
#define COMCERTO_DEFAULTAXICLK          (600000000/3) /* Hz */
#else
#define COMCERTO_DEFAULTAXICLK          200000000 /* Hz */
#endif


/*
 * SCU registers 
 */
#define COMCERTO_SCU_BASE					0xFFF00000			
#define COMCERTO_GIC_CPU_BASE				(COMCERTO_SCU_BASE + 0x100)
#define COMCERTO_TWD_BASE					(COMCERTO_SCU_BASE + 0x600)
#define COMCERTO_TWD_PERCPU_BASE			(COMCERTO_SCU_BASE + 0x700)
#define COMCERTO_GIC_DIST_BASE				(COMCERTO_SCU_BASE + 0x1000)
#define COMCERTO_L310_BASE					(COMCERTO_SCU_BASE + 0x10000)

#define COMCERTO_L2CC_ASSOCIATIVITY_SHIFT	16
#define COMCERTO_L2CC_ASSOCIATIVITY_MASK	0x00010000
#define COMCERTO_L2CC_ASSOCIATIVITY_8WAY	0x0
#define COMCERTO_L2CC_ASSOCIATIVITY_16WAY	0x1

#define COMCERTO_L2CC_WAYSIZE_SHIFT			17
#define COMCERTO_L2CC_WAYSIZE_MASK			0x000E0000
#define COMCERTO_L2CC_ASSOCIATIVITY_32KB	0x2

#define COMCERTO_L2CC_WR_LAT_SHIFT			8
#define COMCERTO_L2CC_RD_LAT_SHIFT			4


/*
  * Physical address of IO on APB Bus
  */

#define COMCERTO_APB_TDM_BASE			0x90400000
#define COMCERTO_APB_USBPHY_SERDES_STAT_BASE	0x90410000
#define COMCERTO_APB_TDMA_BASE			0x90420000
#define COMCERTO_APB_TIMER_BASE			0x90450000
#define COMCERTO_APB_PCI_SATA_USB_CTRL_BASE	0x90460000
#define COMCERTO_APB_GPIO_BASE			0x90470000
#define COMCERTO_APB_UART0_BASE			0x90490000
#define COMCERTO_APB_SPI_BASE			0x90498000
#define COMCERTO_APB_I2C_BASE			0x9049C000
#define COMCERTO_APB_USB3_BASE			0x904A0000
#define COMCERTO_APB_CLK_BASE			0x904B0000
#define COMCERTO_APB_RTC_BASE			0x904E0000
#define COMCERTO_APB_OTP_BASE			0x904F0000
#define COMCERTO_APB_PFE_BASE			0x90500000
#define COMCERTO_APB_SERDES_CONF_BASE		0x90590000
#define COMCERTO_APB_EXPBUS_BASE		0x905A0000
#define COMCERTO_APB_DDRPHY_BASE		0x905B0000
#define COMCERTO_APB_TDMA2_BASE			0x905D0000
#define COMCERTO_APB_MDMA_BASE			0x905E0000
#define COMCERTO_APB_A9CS_BASE			0x90600000


/*
  * Physical address on AXI Bus
  */
#define COMCERTO_AXI_HIGHMEMDDR_BASE		0xFFFF0000
#define COMCERTO_AXI_DDR_BASE			0x00000000
#define COMCERTO_AXI_ACP_BASE			0x80000000 /* 48MB */
#define COMCERTO_AXI_IRAM_BASE			0x83000000
#define COMCERTO_AXI_IBR_BASE			0x90000000
#define COMCERTO_AXI_APB_BASE			0x90400000 /* 12MB */
#define COMCERTO_AXI_SEMA_BASE			0x91000000 /* 16MB */
#define COMCERTO_AXI_USB2P0_BASE		0x92000000
#define COMCERTO_AXI_TRUSTZONE_BASE		0x93000000
#define COMCERTO_AXI_DPI0_BASE			0x94000000 /* 16MB */
#define COMCERTO_AXI_DPI1_BASE			0x95000000 /* 16MB */
#define COMCERTO_AXI_UART_SPI_BASE		0x96000000 /* 16MB */
#define COMCERTO_AXI_UART0_BASE			(COMCERTO_AXI_UART_SPI_BASE + 0x00300000)
#define COMCERTO_AXI_UART1_BASE			(COMCERTO_AXI_UART_SPI_BASE + 0x00400000)
#define COMCERTO_AXI_SPI_BASE			(COMCERTO_AXI_UART_SPI_BASE + 0x00500000)
#define COMCERTO_AXI_DDRCONFIG_BASE		0x97000000
#define COMCERTO_AXI_PCIe1_BASE			0x99000000
#define COMCERTO_AXI_PCIe0_BASE			0x98000000
#define COMCERTO_AXI_IPSEC_BASE			0x9A000000
#define COMCERTO_AXI_SPACC_PDU_BASE		0x9B000000 /* 16MB */
#define COMCERTO_AXI_PFE_BASE			0x9C000000 /* 16MB */
#define COMCERTO_AXI_SATA_BASE			0x9D000000 /* 16MB */
#define COMCERTO_AXI_DECT_BASE			0x9E000000 /* 16MB */
#define COMCERTO_AXI_USB3P0_BASE                0x9F000000 /* 16MB */
#define COMCERTO_AXI_PCIe0_SLAVE_BASE	0xA0000000
#define COMCERTO_AXI_PCIe1_SLAVE_BASE	0xB0000000
#define COMCERTO_AXI_EXP_BASE			0xC0000000
#define COMCERTO_AXI_EXP_ECC_BASE		0xCFFF0000 /* 64KB */

#define COMCERTO_AXI_ACP_SIZE			(1 << 24)

#define COMCERTO_DDR_SHARED_BASE		(COMCERTO_AXI_DDR_BASE + 0x2C00000)
#define COMCERTO_DDR_SHARED_SIZE		(SZ_16M + SZ_4M)

/* MSP memory map */
#define COMCERTO_MSP_DDR_BASE			COMCERTO_DDR_SHARED_BASE
#define COMCERTO_MSP_DDR_SIZE_CB		(SZ_1M * 5)
#define COMCERTO_MSP_DDR_SIZE_NCNB		(SZ_1M * 3)
#define COMCERTO_MSP_DDR_SIZE			(COMCERTO_MSP_DDR_SIZE_CB + COMCERTO_MSP_DDR_SIZE_NCNB)

/* PFE memory map */
#define COMCERTO_APB_PFE_SIZE			SZ_64K
#define COMCERTO_AXI_PFE_SIZE			SZ_16M
#define COMCERTO_AXI_IPSEC_SIZE			SZ_16M
#define COMCERTO_PFE_DDR_BASE			(COMCERTO_DDR_SHARED_BASE + SZ_8M)
#define COMCERTO_PFE_DDR_SIZE			(SZ_8M + SZ_4M)
#define COMCERTO_PFE_IRAM_BASE			(COMCERTO_AXI_IRAM_BASE + 0x0000)
#define COMCERTO_PFE_IRAM_SIZE			SZ_8K

#define IO_SPACE_LIMIT					0

/* MDMA memory map */
#define COMCERTO_APB_MDMA_SIZE          SZ_1K+SZ_4

/*
 * Virtual address mapping
 */
#define COMCERTO_MSP_VADDR			0xf0000000
#define COMCERTO_PFE_VADDR                     0xfa000000
#define COMCERTO_PFE_AXI_VADDR         0xfc000000
#define COMCERTO_SCU_VADDR				COMCERTO_SCU_BASE
#define COMCERTO_GIC_CPU_VADDR			(COMCERTO_SCU_VADDR + 0x100)
#define COMCERTO_GIC_GLOBAL_TIMER_VADDR	(COMCERTO_SCU_VADDR + 0x200)
#define COMCERTO_TWD_VADDR				(COMCERTO_SCU_VADDR + 0x600)
#define COMCERTO_GIC_DIST_VADDR			(COMCERTO_SCU_VADDR + 0x1000)
#define COMCERTO_L310_VADDR				(COMCERTO_SCU_VADDR + 0x10000)

#define COMCERTO_DISTR_INT_SET_PENDING                   (COMCERTO_GIC_DIST_VADDR + 0x200)
#define COMCERTO_DISTR_INT_SET_PENDING_OFFSET_4          (COMCERTO_DISTR_INT_SET_PENDING + 0x4)
#define COMCERTO_DISTR_INT_SET_PENDING_OFFSET_8          (COMCERTO_DISTR_INT_SET_PENDING + 0x8)

#define IRAM_MEMORY_VADDR			0xf0800000
#define COMCERTO_APB_VADDR				0xf0900000	/* VA of IO on APB bus */
#define COMCERTO_APB_SIZE				0x00C00000	
#define COMCERTO_AXI_UART_SPI_VADDR		0xF1600000 
#define COMCERTO_AXI_UART_SPI_SIZE		0x01000000 
#define COMCERTO_AXI_DMA_VADDR			(COMCERTO_AXI_UART_SPI_VADDR+0x00000000) 
#define COMCERTO_AXI_UART0_VADDR		(COMCERTO_AXI_UART_SPI_VADDR+0x00300000) 
#define COMCERTO_AXI_UART1_VADDR		(COMCERTO_AXI_UART_SPI_VADDR+0x00400000) 
#define COMCERTO_AXI_SSI_VADDR			(COMCERTO_AXI_UART_SPI_VADDR+0x00500000) 

#define COMCERTO_SEMA_VADDR			0xf4000000
#define COMCERTO_AXI_PCIe0_VADDR_BASE		0xf5000000
#define COMCERTO_AXI_PCIe1_VADDR_BASE		0xf6000000
#define COMCERTO_DECT_VADDR			0xf7000000

/* 
  * Reference Clock Option in Boot Strap Register
  *
  * BIT [9:8] System PLL Refclk Select 
  * '00' - USB XTAL
  * '01' - Serdes #0 Refclk
  * '10' - Serdes #1 Refclk
  * '11' - Serdes XTAL
  *
  * BIT[7]   Serdes OSC PAD - Reference clock frequency selection bootstrap (inverted value of SF1)
  * '0' - 30MHz ~ 50MHz
  * '1' - 15MHz ~ 30MHz
  * Note: SF0 is tied to 1 in GPIO block and might later on be controlled during DFT
  *
  * BIT[5]   USB/Sys PLL OSC PAD - Reference clock frequency selection bootstrap (inverted value of SF1)
  *'0' - 30MHz ~ 50MHz
  * '1' - 15MHz ~ 30MHz
  * Note: SF0 is tied to 1 in GPIO block and might later on be controlled during DFT
  *
  */

#define GPIO_SYS_PLL_REF_CLK_MASK			(0x00000100 | 0x00000200)
#define GPIO_SYS_PLL_REF_CLK_SHIFT			8

#define USB_XTAL_REF_CLK					0
#define SERDES_0_REF_CLK					1
#define SERDES_2_REF_CLK					2
#define SERDES_XTAL_REF_CLK					3

#define GPIO_SERDES_OSC_PAD_MASK			(0x00000080)
#define GPIO_SERDES_OSC_PAD_SHIFT			7

#define GPIO_USB_OSC_PAD_MASK				(0x00000020)
#define GPIO_USB_OSC_PAD_SHIFT				5

#define REF_CLK_24MHZ						24000000 /* 24 MHz */
#define REF_CLK_48MHZ						48000000 /* 48 MHz */


/* USB 2.0 */
#define USB2_PHY_BASE						APB_VADDR(COMCERTO_APB_PCI_SATA_USB_CTRL_BASE)

/* USB 3.0 */
#define USB3_PHY_BASE           APB_VADDR(COMCERTO_APB_USB3_BASE)

#define USBPHY_SERDES_STAT_BASE APB_VADDR(COMCERTO_APB_USBPHY_SERDES_STAT_BASE)

/* Includes */
#include <mach/comcerto-2000/clk-rst.h>
#include <mach/comcerto-2000/timer.h>
#include <mach/comcerto-2000/gpio.h>
#include <mach/comcerto-2000/exp-bus.h>
#include <mach/comcerto-2000/pcie.h>
#include <mach/comcerto-2000/serdes.h>
#include <mach/comcerto-2000/memory.h>
#include <mach/c2k_dma.h>

#ifndef __ASSEMBLY__
extern int comcerto_mac_addr_get(uint8_t mac[6], uint8_t index);
#endif

#define comcerto_timer4_set(hbound)	__raw_writel((hbound), COMCERTO_TIMER4_HIGH_BOUND)

#define comcerto_timer4_get()		__raw_readl(COMCERTO_TIMER4_CURRENT_COUNT)


#define comcerto_timer5_set(lbound, hbound, ctrl)  do {								\
						      __raw_writel((ctrl) & 0x1, COMCERTO_TIMER5_CTRL);	\
						      __raw_writel((lbound), COMCERTO_TIMER5_LOW_BOUND);	\
						      __raw_writel((hbound), COMCERTO_TIMER5_HIGH_BOUND);	\
						   } while(0)

#define comcerto_timer5_get()		__raw_readl(COMCERTO_TIMER5_CURRENT_COUNT)

/* Number of gemacs supported in comcerto 2000 */
#define NUM_GEMAC_SUPPORT	3

#endif
