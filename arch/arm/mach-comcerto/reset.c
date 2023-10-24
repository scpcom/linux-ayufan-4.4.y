/*
 *  linux/arch/arm/mach-comcerto/reset.c
 *
 *  driver for block reset for all the devices availble in the 
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

#include <linux/module.h>
#include <mach/reset.h> 
#include <asm/io.h>
#include <linux/spinlock.h>

static char i2cspi_state[2],dus_state[3];
spinlock_t reset_lock;

/* @ int block : Id of device block to be put in reset 
 * @ int state : State value 0->OUT-OF-RESET , 1->RESET. 
 * API for block reset to all the device blocks 
 * available for C2000 devices.
 */
void c2000_block_reset(int block,int state)
{
unsigned long flags;
spin_lock_irqsave(&reset_lock, flags);

if(state){
	/*  Code is to put the device block in RESET */
	switch (block)
	{
		case COMPONENT_AXI_RTC:
			/* Put the Timer device(AXI clock domain) in reset state */
			writel(readl(AXI_RESET_1) | RTC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_I2C:
			i2cspi_state[0]=1;
			goto i2cspi_rst;
		case COMPONENT_AXI_LEGACY_SPI:
			i2cspi_state[1]=1;
		i2cspi_rst:
			if(i2cspi_state[0] == 1 && i2cspi_state[1] == 1)
				/* Put the I2C and LEGACY SPI(AXI clock domain) device in reset state */
				writel(readl(AXI_RESET_1) | I2CSPI_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DMA:
			dus_state[0]=1;
			goto dus_rst;
		case COMPONENT_AXI_FAST_UART:
			dus_state[1]=1;
			goto dus_rst;
		case COMPONENT_AXI_FAST_SPI:
			dus_state[2]=1;
		dus_rst:
			if(dus_state[0] == 1 && dus_state[1] == 1 && dus_state[2] == 1)
				/* Put the DUS (AXI clock domain) device in reset state */
				writel(readl(AXI_RESET_1) | DUS_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_TDM:
			/* Put the TDM(AXI clock domain) device in reset state */
			writel(readl(AXI_RESET_1) | TDM_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_PFE_SYS:
			/* Put the PFE device in reset state */
			writel(readl(AXI_RESET_1) | PFE_SYS_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_EAPE:
			/* Put the IPSEC EAPE (AXI clock domain) device in reset state */
			writel(readl(AXI_RESET_1) | IPSEC_EAPE_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_SPACC:
			/* Put the IPSEC SPACC (AXI clock domain) device in reset state */
			writel(readl(AXI_RESET_1) | IPSEC_SPACC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DPI_CIE:
			/* Put the DPI CIE (AXI clock domain) device in reset state */
			writel(readl(AXI_RESET_0) | DPI_CIE_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_DPI_DECOMP:
			/* Put the DPI DECOMP (AXI clock domain) device in reset state */
			writel(readl(AXI_RESET_0) | DPI_DECOMP_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_USB0:
			/* Put the USB0 device(AXI clock domain) in reset state */
			writel(readl(AXI_RESET_2) | USB0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB0:
			/* Put the USB0 device(UTMI clock domain) in reset state */
			writel(readl(USB_RST_CNTRL) | USB0_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB0_PHY:
			/* Put the USB0_PHY device in reset state */
			writel(readl(USB_RST_CNTRL) | USB0_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_USB1:
			/* Put the USB1 device in reset state */
			writel(readl(AXI_RESET_2) | USB1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB1:
			/* Put the USB1 device(UTMI clock domain) in reset state */
			writel(readl(USB_RST_CNTRL) | USB1_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB1_PHY:
			/* Put the USB1_PHY device in reset state */
			writel(readl(USB_RST_CNTRL) | USB1_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_SATA:
			/* Put the SATA device in reset state */
			writel(readl(AXI_RESET_2) | SATA_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE0:
			/* Put the PCIE0 device in reset state */
			writel(readl(AXI_RESET_2) | PCIE0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE1:
			/* Put the PCIE1  device in reset state */
			writel(readl(AXI_RESET_2) | PCIE1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_PFE_CORE:
			/* Put the PFE core  device in reset state */
			writel(readl(PFE_RESET) | PFE_CORE_RESET_BIT, PFE_RESET);
			break;
		case COMPONENT_IPSEC_EAPE_CORE:
			/* Put the IPSEC EAPE  core  device in reset state */
			writel(readl(IPSEC_RESET) | IPSEC_EAPE_CORE_RESET_BIT, IPSEC_RESET);
			break;
		case COMPONENT_GEMTX:
			/* Put the GEMTX device in reset state */
			writel(readl(GEMTX_RESET) | GEMTX_RESET_BIT , GEMTX_RESET);
			break;
		case COMPONENT_DECT:
			/* Put the DECT device in reset state */
			writel(readl(DECT_RESET) | DECT_RESET_BIT , DECT_RESET);
			break;
		case COMPONENT_DDR_CNTLR:
			/* Put the DDR controller  device in reset state */
			writel(readl(DDR_RESET) | DDR_CNTRL_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_DDR_PHY:
			/* Put the DDR PHY  device in reset state */
			writel(readl(DDR_RESET) | DDR_PHY_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_SERDES0:
			/* Put  SERDES0 controller  in Reset state */
			writel(readl(SERDES_RST_CNTRL) | SERDES0_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE0:
			/* Put the PCIE0 SERDES controller in Reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) | SERDES_PCIE0_RESET_BIT, PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES1:
			/* Put SERDES1 contrller 1 in reset state */
			writel(readl(SERDES_RST_CNTRL) | SERDES1_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE1: 
			/* Put PCIE1 serdes controller in reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) | SERDES_PCIE1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA0: 
			/* Put SATA0 serdes controller in reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) | (SERDES_SATA0_RESET_BIT), PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES2:
			/* Put SERDES2 contrller  in reset state */
			writel(readl(SERDES_RST_CNTRL) | SERDES2_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA1: 
			/* Put SATA1 serdes controller in reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) | SERDES_SATA1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SGMII: 
			/* Put SGMII serdes controller in reset state */
			writel(readl(SGMII_OCC_RESET) | (SGMII_RESET_BIT), SGMII_OCC_RESET);
			break;
		case COMPONENT_SATA_PMU:
			/* Put the SATA PMU(Keep Alive clock) in Reset state */
			writel(readl(SATA_PMU_RESET) | SATA_PMU_RESET_BIT, SATA_PMU_RESET);
			break;
		case COMPONENT_SATA_OOB:
			/* Put the SATA OOB in Reset state */
			writel(readl(SATA_OOB_RESET) | SATA_OOB_RESET_BIT, SATA_OOB_RESET);
			break;
		case COMPONENT_TDMNTG:
			/* Put the TDMNTG  in Reset state */
			writel(readl(TDMNTG_RESET) | TDMNTG_RESET_BIT, TDMNTG_RESET);
			break;
		default :
			break;
	}	
}else{
	/* Code is to put the device block to Out of reset */
	switch (block)
	{
		case COMPONENT_AXI_RTC:
			/* Put the Timer device in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~RTC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_I2C:
			i2cspi_state[0]=0;
			goto i2cspi_outrst;
		case COMPONENT_AXI_LEGACY_SPI:
			i2cspi_state[1]=0;
		i2cspi_outrst:
			/* Put the I2C/SPI device in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~I2CSPI_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DMA:
			dus_state[0]=0;
			goto dus_outrst;
		case COMPONENT_AXI_FAST_UART:
			dus_state[1]=0;
			goto dus_outrst;
		case COMPONENT_AXI_FAST_SPI:
			dus_state[2]=0;
		dus_outrst:
			/* Put the DUS devices in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~DUS_AXI_RESET_BIT , AXI_RESET_1);
			break;
		case COMPONENT_AXI_TDM:
			/* Put the TDM device in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~TDM_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_PFE_SYS:
			/* Put the PFE System devices in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~PFE_SYS_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_EAPE:
			/* Put the IPSEC EAPE devices in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~IPSEC_EAPE_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_SPACC:
			/* Put the IPSEC SPACC devices in Out-Of-Reset state */
			writel(readl(AXI_RESET_1) & ~IPSEC_SPACC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DPI_CIE:
			/* Put the DPI CIE devices in Out-Of-Reset state */
			writel(readl(AXI_RESET_0) & ~DPI_CIE_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_DPI_DECOMP:
			/* Put the DPI DECOMP devices in Out-Of-Reset state */
			writel(readl(AXI_RESET_0) & ~DPI_DECOMP_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_USB0:
			/* Put the USB0 device in Out-Of-Reset state */
			writel(readl(AXI_RESET_2) & ~USB0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB0:
			/* Put the USB0 device(UTMI clock domain) in Out-Of-reset state */
			writel(readl(USB_RST_CNTRL) & ~USB0_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB0_PHY:
			/* Put the USB0_PHY devices in Out-Of-Reset state */
			writel(readl(USB_RST_CNTRL)& ~USB0_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_USB1:
			/* Put the USB1 device in Out-Of-Reset state */
			writel(readl(AXI_RESET_2) & ~USB1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB1:
			/* Put the USB1 device(UTMI clock domain) in Out-Of-reset state */
			writel(readl(USB_RST_CNTRL) & ~USB1_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB1_PHY:
			/* Put the USB1_PHY devices in Out-Of-Reset state */
			writel(readl(USB_RST_CNTRL)& ~USB1_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_SATA:
			/* Put the SATA device in Out-Of-Reset state */
			writel(readl(AXI_RESET_2) & ~SATA_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE0:
			/* Put the PCIE0 device in Out-Of-reset state */
			writel(readl(AXI_RESET_2) & ~PCIE0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE1:
			/* Put the PCIE1 device in Out-Of-Reset state */
			writel(readl(AXI_RESET_2) & ~PCIE1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_PFE_CORE:
			/* Put the PFE core  device Out-Of-Reset state */
			writel(readl(PFE_RESET) & ~PFE_CORE_RESET_BIT, PFE_RESET);
			break;
		case COMPONENT_IPSEC_EAPE_CORE:
			/* Put the IPSEC EAPE  core  device in Out-Of-Reset state */
			writel(readl(IPSEC_RESET) & ~IPSEC_EAPE_CORE_RESET_BIT, IPSEC_RESET);
			break;
		case COMPONENT_GEMTX:
			/* Put the GEMTX device in Out-Of-Reset state */
			writel(readl(GEMTX_RESET) & ~GEMTX_RESET_BIT , GEMTX_RESET);
			break;
		case COMPONENT_DECT:
			/* Put the DECT device in Out-Of-Reset state */
			writel(readl(DECT_RESET) & ~DECT_RESET_BIT , DECT_RESET);
			break;
		case COMPONENT_DDR_CNTLR:
			/* Put the DDR controller  device in Out-Of-Reset state */
			writel(readl(DDR_RESET) & ~DDR_CNTRL_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_DDR_PHY:
			/* Put the DDR PHY  device in Out-Of-Reset state */
			writel(readl(DDR_RESET) & ~DDR_PHY_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_SERDES0:
			/* put the SERDES0 controller  in Out-of-Reset state */
			writel(readl(SERDES_RST_CNTRL) & ~SERDES0_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE0:
			/* Put the PCIE0 SERDES controller in Out-Of-Reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) & ~SERDES_PCIE0_RESET_BIT, PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES1:
			/* Put SEDES1 controller in Out-Of-reset state */
			writel(readl(SERDES_RST_CNTRL)& ~SERDES1_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE1: 
			/* Put PCIE1 serdes controller in Out-Of-Reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) & ~SERDES_PCIE1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA0: 
			writel(readl(PCIe_SATA_RST_CNTRL) & ~SERDES_SATA0_RESET_BIT, PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES2:
			/* Put Serdes contrller 1 in Out-Of-Reset state */
			writel(readl(SERDES_RST_CNTRL) & ~SERDES2_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA1: 
			/* Put SATA1 serdes controller in Out-Of-reset state */
			writel(readl(PCIe_SATA_RST_CNTRL) & ~SERDES_SATA1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SGMII: 
			/* Put the SGMII controller in Out-Of-Reset state */
			writel(readl(SGMII_OCC_RESET) & ~SGMII_RESET_BIT, SGMII_OCC_RESET);
			break;
		case COMPONENT_SATA_PMU:
			/* Put the SATA PMU(KEEP ALIVE clock) in Out-Of-Reset state */
			writel(readl(SATA_PMU_RESET) & ~SATA_PMU_RESET_BIT , SATA_PMU_RESET);
			break;
		case COMPONENT_SATA_OOB:
			/* Put the SATA OOB  in Out-Of-Reset state */
			writel(readl(SATA_OOB_RESET) & ~SATA_OOB_RESET_BIT , SATA_OOB_RESET);
			break;
		case COMPONENT_TDMNTG:
			/* Put the TDMNTG  in Out-Of-Reset state */
			writel(readl(TDMNTG_RESET) & ~TDMNTG_RESET_BIT, TDMNTG_RESET);
			break;
		default :
			break;
	}	
}
spin_unlock_irqrestore(&reset_lock,flags);
}
EXPORT_SYMBOL(c2000_block_reset);

void reset_init(void){
	
	/*Initilize the DUS ,serde0/1/2 and I2CSPI dependancy values */
	i2cspi_state[0]=i2cspi_state[1]=dus_state[0]=dus_state[1]=dus_state[2]=0;

        spin_lock_init(&reset_lock);

	/* Do Any boottime Reset/Out-Of-Reset to devices if required*/
}
