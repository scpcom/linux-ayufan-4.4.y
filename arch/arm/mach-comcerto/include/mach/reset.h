/*
 *  linux/arch/arm/mach-comcerto/include/mach/comcerto-2000/reset.h
 *
 *  Header file for block reset for all the devices availble in the 
 *  c2000 device.
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
 *
 */
#ifndef  __ARCH_COMCERTO2000_RESET_H
#define  __ARCH_COMCERTO2000_RESET_H

#define RTC_AXI_RESET_BIT		(1<<7)
#define I2CSPI_AXI_RESET_BIT		(1<<5)
#define DUS_AXI_RESET_BIT		(1<<0)
#define TDM_AXI_RESET_BIT 		(1<<4)		
#define PFE_SYS_AXI_RESET_BIT		(1<<3)
#define IPSEC_SPACC_AXI_RESET_BIT	(1<<2)
#define IPSEC_EAPE_AXI_RESET_BIT	(1<<1)
#define DPI_CIE_AXI_RESET_BIT		(1<<5)
#define DPI_DECOMP_AXI_RESET_BIT	(1<<6)
#define USB1_AXI_RESET_BIT		(1<<4)
#define USB1_PHY_RESET_BIT		(1<<4)
#define USB0_AXI_RESET_BIT		(1<<3)
#define USB0_PHY_RESET_BIT		(1<<0)
#define SATA_AXI_RESET_BIT		(1<<2)
#define SATA_PMU_RESET_BIT		(1<<0)
#define SATA_OOB_RESET_BIT		(1<<0)
#define PCIE1_AXI_RESET_BIT		(1<<1)
#define PCIE0_AXI_RESET_BIT		(1<<0)
#define PFE_CORE_RESET_BIT		(1<<0)
#define IPSEC_EAPE_CORE_RESET_BIT	(1<<0)
#define GEMTX_RESET_BIT			(1<<0)
#define L2CC_RESET_BIT			(1<<0)
#define DECT_RESET_BIT			(1<<0)
#define DDR_CNTRL_RESET_BIT		(1<<1)
#define DDR_PHY_RESET_BIT		(1<<0)
#define SERDES0_RESET_BIT		(1<<0)
#define SERDES1_RESET_BIT		(1<<1)
#define SERDES2_RESET_BIT		(1<<2)
#define SERDES_PCIE0_RESET_BIT		((1<<0)|(1<<1))
#define SERDES_PCIE1_RESET_BIT		((1<<2)|(1<<3))
#define SERDES_SATA0_RESET_BIT		((1<<4)|(1<<5))
#define SERDES_SATA1_RESET_BIT		((1<<6)|(1<<7))
#define SGMII_RESET_BIT			(1<<0)
#define USB0_UTMI_RESET_BIT		(1<<1)
#define USB1_UTMI_RESET_BIT		(1<<5)
#define TDMNTG_RESET_BIT		(1<<0)

/* C2000 device blocks which are to be put 
 * in reset.
 */
typedef enum {
	COMPONENT_AXI_RTC=0,             
	COMPONENT_AXI_LEGACY_SPI,
	COMPONENT_AXI_I2C,
	COMPONENT_AXI_DMA, 
	COMPONENT_AXI_FAST_UART0,
	COMPONENT_AXI_FAST_UART1,
	COMPONENT_AXI_FAST_SPI, 
	COMPONENT_AXI_TDM, 
	COMPONENT_PFE_SYS,
	COMPONENT_AXI_IPSEC_EAPE,
	COMPONENT_AXI_IPSEC_SPACC,
	COMPONENT_AXI_DPI_CIE,
	COMPONENT_AXI_DPI_DECOMP,
	COMPONENT_AXI_USB1 ,		/* USB controller1, AXI Clock Domain reset control */
	COMPONENT_UTMI_USB1,  		/* USB controller1,UTMI Clock Domain reset control*/
	COMPONENT_USB1_PHY,		/* USB PHY1 Reset control */
	COMPONENT_AXI_USB0,		/* USB controller0, AXI Clock Domain reset control*/
	COMPONENT_UTMI_USB0,  		/* USB controller0. UTMI Clock Domain reset control */
	COMPONENT_USB0_PHY,		/* USB PHY0 Reset Control */
	COMPONENT_AXI_SATA,		/* SATA controller AXI Clock Domain Control Both for SATA 0/1*/
	COMPONENT_SERDES_SATA0,		/* SATA serdes Controller0  TX,Core Logic and RX clock domain control*/
	COMPONENT_SERDES_SATA1,		/* SATA serdes Controller1  TX,Core Logic and RX clock domain control*/
	COMPONENT_AXI_PCIE1,		/* PCIE Controller1,AXI Clock Domain reset control*/ 
	COMPONENT_SERDES_PCIE1,		/* PCIE serdes Controller1  Striky register and power register*/
	COMPONENT_AXI_PCIE0,		/* PCIE Controller0,AXI Clock Domain reset control*/
	COMPONENT_SERDES_PCIE0,		/* PCIE Controller1,TX,Core Logic and RX clock domain control*/
	COMPONENT_PFE_CORE,
	COMPONENT_IPSEC_EAPE_CORE,
	COMPONENT_GEMTX,
	COMPONENT_L2CC,
	COMPONENT_DECT,
	COMPONENT_DDR_CNTLR,
	COMPONENT_DDR_PHY,
	COMPONENT_SERDES0,
	COMPONENT_SERDES1,
	COMPONENT_SERDES2,
	COMPONENT_SGMII,
	COMPONENT_SATA_PMU,
	COMPONENT_SATA_OOB,
	COMPONENT_TDMNTG,
}C2000_RESET_COMPONENT;

#include <linux/reboot.h>
extern void ls1024_restart(enum reboot_mode, const char *);
extern void comcerto_rst_cntrl_set(unsigned int dev_rst_cntrl_bit);
extern void c2000_block_reset(int block,int state);
extern void reset_init(void);

#if defined(CONFIG_C2K_MFCN_EVM)
/* C2000 device blocks which are to be put
 * in out of reset(GPIO).
 */
typedef enum {
	COMPONENT_ATHEROS_SWITCH=0,
	COMPONENT_SLIC,
	COMPONENT_PCIE0,
	COMPONENT_PCIE1,
	COMPONENT_USB_HUB,
	COMPONENT_EXP_DAUGTHER_CARD,
	COMPONENT_RGMII0,
	COMPONENT_RGMII1
}C2000_GEN2_GPIO_RESET_COMPONENT;

void GPIO_reset_external_device(int block,int state);
#endif
#endif
