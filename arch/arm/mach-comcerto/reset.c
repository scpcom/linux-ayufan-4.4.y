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
#include <asm/system_misc.h>
#include <mach/reset.h>
#include <asm/io.h>
#include <linux/spinlock.h>

#include <linux/gpio.h>

#define reset_readl(r) readl((void*)(r))
#define reset_writel(v,r) writel(v, (void*)(r))

#define reset_raw_readl(r) __raw_readl((void*)(r))
#define reset_raw_writel(v,r) __raw_writel(v, (void*)(r))

static unsigned int i2cspi_state = 0x3, dus_state = 0xf;
static spinlock_t reset_lock;
static spinlock_t gpio_lock;

void comcerto_rst_cntrl_set(unsigned int dev_rst_cntrl_bit)
{
	unsigned long flags;

	spin_lock_irqsave(&reset_lock, flags);

	reset_raw_writel((dev_rst_cntrl_bit | reset_raw_readl(DEVICE_RST_CNTRL)), DEVICE_RST_CNTRL);

	spin_unlock_irqrestore(&reset_lock,flags);
}
EXPORT_SYMBOL(comcerto_rst_cntrl_set);

void ls1024_restart(enum reboot_mode mode, const char *cmd)
{
	if (mode == REBOOT_SOFT) {
		/* Jump into ROM at address 0 */
		soft_restart(0);
	} else {
		printk(KERN_WARNING "%s():: %d UNEXPECTED reboot mode %d\n", __func__, __LINE__, mode);
		/* Use on-chip reset capability */

		/* set the "key" register to enable access to
		 * "timer" and "enable" registers
		 */
	}
}
EXPORT_SYMBOL(ls1024_restart);

/* @ int block : Id of device block to be put in reset
 * @ int state : State value 0->OUT-OF-RESET , 1->RESET.
 * API for block reset to all the device blocks
 * available for C2000 devices.
 */
void c2000_block_reset(int block,int state)
{
	unsigned long flags;

	spin_lock_irqsave(&reset_lock, flags);

	if (state) {
		/*  Code is to put the device block in RESET */
		switch (block) {
		case COMPONENT_AXI_RTC:
			/* Put the Timer device(AXI clock domain) in reset state */
			reset_writel(reset_readl(AXI_RESET_1) | RTC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_I2C:
			i2cspi_state |= (1 << 0);
			goto i2cspi_rst;
		case COMPONENT_AXI_LEGACY_SPI:
			i2cspi_state |= (1 << 1);
		i2cspi_rst:
			if ((i2cspi_state & 0x3) == 0x3)
				/* Put the I2C and LEGACY SPI(AXI clock domain) device in reset state */
				reset_writel(reset_readl(AXI_RESET_1) | I2CSPI_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DMA:
			dus_state |= (1 << 0);
			goto dus_rst;
		case COMPONENT_AXI_FAST_UART0:
			dus_state |= (1 << 1);
			goto dus_rst;
		case COMPONENT_AXI_FAST_UART1:
			dus_state |= (1 << 2);
			goto dus_rst;
		case COMPONENT_AXI_FAST_SPI:
			dus_state |= (1 << 3);
		dus_rst:
			if ((dus_state & 0xf) == 0xf)
				/* Put the DUS (AXI clock domain) device in reset state */
				reset_writel(reset_readl(AXI_RESET_1) | DUS_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_TDM:
			/* Put the TDM(AXI clock domain) device in reset state */
			reset_writel(reset_readl(AXI_RESET_1) | TDM_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_PFE_SYS:
			/* Put the PFE device in reset state */
			reset_writel(reset_readl(AXI_RESET_1) | PFE_SYS_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_EAPE:
			/* Put the IPSEC EAPE (AXI clock domain) device in reset state */
			reset_writel(reset_readl(AXI_RESET_1) | IPSEC_EAPE_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_SPACC:
			/* Put the IPSEC SPACC (AXI clock domain) device in reset state */
			reset_writel(reset_readl(AXI_RESET_1) | IPSEC_SPACC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DPI_CIE:
			/* Put the DPI CIE (AXI clock domain) device in reset state */
			reset_writel(reset_readl(AXI_RESET_0) | DPI_CIE_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_DPI_DECOMP:
			/* Put the DPI DECOMP (AXI clock domain) device in reset state */
			reset_writel(reset_readl(AXI_RESET_0) | DPI_DECOMP_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_USB0:
			/* Put the USB0 device(AXI clock domain) in reset state */
			reset_writel(reset_readl(AXI_RESET_2) | USB0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB0:
			/* Put the USB0 device(UTMI clock domain) in reset state */
			reset_writel(reset_readl(USB_RST_CNTRL) | USB0_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB0_PHY:
			/* Put the USB0_PHY device in reset state */
			reset_writel(reset_readl(USB_RST_CNTRL) | USB0_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_USB1:
			/* Put the USB1 device in reset state */
			reset_writel(reset_readl(AXI_RESET_2) | USB1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB1:
			/* Put the USB1 device(UTMI clock domain) in reset state */
			reset_writel(reset_readl(USB_RST_CNTRL) | USB1_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB1_PHY:
			/* Put the USB1_PHY device in reset state */
			reset_writel(reset_readl(USB_RST_CNTRL) | USB1_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_SATA:
			/* Put the SATA device in reset state */
			reset_writel(reset_readl(AXI_RESET_2) | SATA_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE0:
			/* Put the PCIE0 device in reset state */
			reset_writel(reset_readl(AXI_RESET_2) | PCIE0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE1:
			/* Put the PCIE1  device in reset state */
			reset_writel(reset_readl(AXI_RESET_2) | PCIE1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_PFE_CORE:
			/* Put the PFE core  device in reset state */
			reset_writel(reset_readl(PFE_RESET) | PFE_CORE_RESET_BIT, PFE_RESET);
			break;
		case COMPONENT_IPSEC_EAPE_CORE:
			/* Put the IPSEC EAPE  core  device in reset state */
			reset_writel(reset_readl(IPSEC_RESET) | IPSEC_EAPE_CORE_RESET_BIT, IPSEC_RESET);
			break;
		case COMPONENT_GEMTX:
			/* Put the GEMTX device in reset state */
			reset_writel(reset_readl(GEMTX_RESET) | GEMTX_RESET_BIT , GEMTX_RESET);
			break;
		case COMPONENT_DECT:
			/* Put the DECT device in reset state */
			reset_writel(reset_readl(DECT_RESET) | DECT_RESET_BIT , DECT_RESET);
			break;
		case COMPONENT_DDR_CNTLR:
			/* Put the DDR controller  device in reset state */
			reset_writel(reset_readl(DDR_RESET) | DDR_CNTRL_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_DDR_PHY:
			/* Put the DDR PHY  device in reset state */
			reset_writel(reset_readl(DDR_RESET) | DDR_PHY_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_SERDES0:
			/* Put  SERDES0 controller  in Reset state */
			reset_writel(reset_readl(SERDES_RST_CNTRL) | SERDES0_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE0:
			/* Put the PCIE0 SERDES controller in Reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) | SERDES_PCIE0_RESET_BIT, PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES1:
			/* Put SERDES1 contrller 1 in reset state */
			reset_writel(reset_readl(SERDES_RST_CNTRL) | SERDES1_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE1: 
			/* Put PCIE1 serdes controller in reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) | SERDES_PCIE1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA0: 
			/* Put SATA0 serdes controller in reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) | (SERDES_SATA0_RESET_BIT), PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES2:
			/* Put SERDES2 contrller  in reset state */
			reset_writel(reset_readl(SERDES_RST_CNTRL) | SERDES2_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA1: 
			/* Put SATA1 serdes controller in reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) | SERDES_SATA1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SGMII: 
			/* Put SGMII serdes controller in reset state */
			reset_writel(reset_readl(SGMII_OCC_RESET) | (SGMII_RESET_BIT), SGMII_OCC_RESET);
			break;
		case COMPONENT_SATA_PMU:
			/* Put the SATA PMU(Keep Alive clock) in Reset state */
			reset_writel(reset_readl(SATA_PMU_RESET) | SATA_PMU_RESET_BIT, SATA_PMU_RESET);
			break;
		case COMPONENT_SATA_OOB:
			/* Put the SATA OOB in Reset state */
			reset_writel(reset_readl(SATA_OOB_RESET) | SATA_OOB_RESET_BIT, SATA_OOB_RESET);
			break;
		case COMPONENT_TDMNTG:
			/* Put the TDMNTG  in Reset state */
			reset_writel(reset_readl(TDMNTG_RESET) | TDMNTG_RESET_BIT, TDMNTG_RESET);
			break;
		default :
			break;
		}
	} else {

		/* Code is to put the device block to Out of reset */
		switch (block) {
		case COMPONENT_AXI_RTC:
			/* Put the Timer device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~RTC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_I2C:
			i2cspi_state &= ~(1 << 0);
			goto i2cspi_outrst;
		case COMPONENT_AXI_LEGACY_SPI:
			i2cspi_state &= ~(1 << 1);
		i2cspi_outrst:
			/* Put the I2C/SPI device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~I2CSPI_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DMA:
			dus_state &= ~(1 << 0);
			goto dus_outrst;
		case COMPONENT_AXI_FAST_UART0:
			dus_state &= ~(1 << 1);
			goto dus_outrst;
		case COMPONENT_AXI_FAST_UART1:
			dus_state &= ~(1 << 2);
			goto dus_outrst;
		case COMPONENT_AXI_FAST_SPI:
			dus_state &= ~(1 << 3);

		dus_outrst:
			/* Put the DUS devices in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~DUS_AXI_RESET_BIT , AXI_RESET_1);
			break;
		case COMPONENT_AXI_TDM:
			/* Put the TDM device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~TDM_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_PFE_SYS:
			/* Put the PFE System devices in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~PFE_SYS_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_EAPE:
			/* Put the IPSEC EAPE devices in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~IPSEC_EAPE_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_IPSEC_SPACC:
			/* Put the IPSEC SPACC devices in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_1) & ~IPSEC_SPACC_AXI_RESET_BIT, AXI_RESET_1);
			break;
		case COMPONENT_AXI_DPI_CIE:
			/* Put the DPI CIE devices in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_0) & ~DPI_CIE_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_DPI_DECOMP:
			/* Put the DPI DECOMP devices in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_0) & ~DPI_DECOMP_AXI_RESET_BIT, AXI_RESET_0);
			break;
		case COMPONENT_AXI_USB0:
			/* Put the USB0 device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_2) & ~USB0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB0:
			/* Put the USB0 device(UTMI clock domain) in Out-Of-reset state */
			reset_writel(reset_readl(USB_RST_CNTRL) & ~USB0_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB0_PHY:
			/* Put the USB0_PHY devices in Out-Of-Reset state */
			reset_writel(reset_readl(USB_RST_CNTRL)& ~USB0_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_USB1:
			/* Put the USB1 device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_2) & ~USB1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_UTMI_USB1:
			/* Put the USB1 device(UTMI clock domain) in Out-Of-reset state */
			reset_writel(reset_readl(USB_RST_CNTRL) & ~USB1_UTMI_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_USB1_PHY:
			/* Put the USB1_PHY devices in Out-Of-Reset state */
			reset_writel(reset_readl(USB_RST_CNTRL)& ~USB1_PHY_RESET_BIT, USB_RST_CNTRL);
			break;
		case COMPONENT_AXI_SATA:
			/* Put the SATA device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_2) & ~SATA_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE0:
			/* Put the PCIE0 device in Out-Of-reset state */
			reset_writel(reset_readl(AXI_RESET_2) & ~PCIE0_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_AXI_PCIE1:
			/* Put the PCIE1 device in Out-Of-Reset state */
			reset_writel(reset_readl(AXI_RESET_2) & ~PCIE1_AXI_RESET_BIT, AXI_RESET_2);
			break;
		case COMPONENT_PFE_CORE:
			/* Put the PFE core  device Out-Of-Reset state */
			reset_writel(reset_readl(PFE_RESET) & ~PFE_CORE_RESET_BIT, PFE_RESET);
			break;
		case COMPONENT_IPSEC_EAPE_CORE:
			/* Put the IPSEC EAPE  core  device in Out-Of-Reset state */
			reset_writel(reset_readl(IPSEC_RESET) & ~IPSEC_EAPE_CORE_RESET_BIT, IPSEC_RESET);
			break;
		case COMPONENT_GEMTX:
			/* Put the GEMTX device in Out-Of-Reset state */
			reset_writel(reset_readl(GEMTX_RESET) & ~GEMTX_RESET_BIT , GEMTX_RESET);
			break;
		case COMPONENT_DECT:
			/* Put the DECT device in Out-Of-Reset state */
			reset_writel(reset_readl(DECT_RESET) & ~DECT_RESET_BIT , DECT_RESET);
			break;
		case COMPONENT_DDR_CNTLR:
			/* Put the DDR controller  device in Out-Of-Reset state */
			reset_writel(reset_readl(DDR_RESET) & ~DDR_CNTRL_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_DDR_PHY:
			/* Put the DDR PHY  device in Out-Of-Reset state */
			reset_writel(reset_readl(DDR_RESET) & ~DDR_PHY_RESET_BIT , DDR_RESET);
			break;
		case COMPONENT_SERDES0:
			/* put the SERDES0 controller  in Out-of-Reset state */
			reset_writel(reset_readl(SERDES_RST_CNTRL) & ~SERDES0_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE0:
			/* Put the PCIE0 SERDES controller in Out-Of-Reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) & ~SERDES_PCIE0_RESET_BIT, PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES1:
			/* Put SEDES1 controller in Out-Of-reset state */
			reset_writel(reset_readl(SERDES_RST_CNTRL)& ~SERDES1_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_PCIE1: 
			/* Put PCIE1 serdes controller in Out-Of-Reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) & ~SERDES_PCIE1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA0: 
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) & ~SERDES_SATA0_RESET_BIT, PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SERDES2:
			/* Put Serdes contrller 1 in Out-Of-Reset state */
			reset_writel(reset_readl(SERDES_RST_CNTRL) & ~SERDES2_RESET_BIT ,SERDES_RST_CNTRL);
			break;
		case COMPONENT_SERDES_SATA1: 
			/* Put SATA1 serdes controller in Out-Of-reset state */
			reset_writel(reset_readl(PCIe_SATA_RST_CNTRL) & ~SERDES_SATA1_RESET_BIT , PCIe_SATA_RST_CNTRL);
			break;
		case COMPONENT_SGMII: 
			/* Put the SGMII controller in Out-Of-Reset state */
			reset_writel(reset_readl(SGMII_OCC_RESET) & ~SGMII_RESET_BIT, SGMII_OCC_RESET);
			break;
		case COMPONENT_SATA_PMU:
			/* Put the SATA PMU(KEEP ALIVE clock) in Out-Of-Reset state */
			reset_writel(reset_readl(SATA_PMU_RESET) & ~SATA_PMU_RESET_BIT , SATA_PMU_RESET);
			break;
		case COMPONENT_SATA_OOB:
			/* Put the SATA OOB  in Out-Of-Reset state */
			reset_writel(reset_readl(SATA_OOB_RESET) & ~SATA_OOB_RESET_BIT , SATA_OOB_RESET);
			break;
		case COMPONENT_TDMNTG:
			/* Put the TDMNTG  in Out-Of-Reset state */
			reset_writel(reset_readl(TDMNTG_RESET) & ~TDMNTG_RESET_BIT, TDMNTG_RESET);
			break;
		default :
			break;
		}
	}

	spin_unlock_irqrestore(&reset_lock, flags);
}
EXPORT_SYMBOL(c2000_block_reset);

#if defined(CONFIG_C2K_MFCN_EVM)
void GPIO_reset_external_device(int block,int state)
{
	unsigned long flags;
	spin_lock_irqsave(&gpio_lock, flags);

	/* Blocks to be put in out of Reset and reset mode
	 * 0 ----> out of reset
	 * 1 ----> reset
	 */
	switch (block){
		case COMPONENT_ATHEROS_SWITCH:
			if(gpio_request_one(GPIO_PIN_NUM_5, GPIOF_OUT_INIT_HIGH, GPIO_PIN_DESC_5)){
				printk(KERN_ERR "%s:%d: Cannot request gpio for gpio-%d\n", \
						__func__, __LINE__, GPIO_PIN_NUM_4);
				return;
			}

			if (state){
				gpio_set_value(GPIO_PIN_NUM_5, GPIO_SET_0);

				gpio_direction_input(GPIO_PIN_NUM_5);
			}else{
				gpio_set_value(GPIO_PIN_NUM_5, GPIO_SET_0);

				gpio_direction_output(GPIO_PIN_NUM_5, GPIO_SET_0);

				gpio_set_value(GPIO_PIN_NUM_5, GPIO_SET_1);
			}

			gpio_free(GPIO_PIN_NUM_5);

			break;

		case COMPONENT_SLIC:
			if(gpio_request_one(GPIO_PIN_NUM_4, GPIOF_OUT_INIT_HIGH, GPIO_PIN_DESC_4)){
				printk(KERN_ERR "%s:%d: Cannot request gpio for gpio-%d\n", \
						__func__, __LINE__, GPIO_PIN_NUM_4);
				return;
			}

			if (state){
				gpio_set_value(GPIO_PIN_NUM_4, GPIO_SET_0);

				gpio_direction_input(GPIO_PIN_NUM_4);
			}else{
				gpio_set_value(GPIO_PIN_NUM_4, GPIO_SET_0);

				gpio_direction_output(GPIO_PIN_NUM_4, GPIO_SET_0);

				gpio_set_value(GPIO_PIN_NUM_4, GPIO_SET_1);
			}

			gpio_free(GPIO_PIN_NUM_4);

			break;

		case COMPONENT_PCIE0:
			if (state){
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_SELECT);
			}else{
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | GPIO_PIN_48, COMCERTO_GPIO_63_32_PIN_SELECT);
			}
			break;
		case COMPONENT_PCIE1:
			if (state){
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_SELECT);
			}else{
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | GPIO_PIN_47, COMCERTO_GPIO_63_32_PIN_SELECT);
			}
			break;
		case COMPONENT_USB_HUB:
			if (state){
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_SELECT);
			}else{
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | GPIO_PIN_50, COMCERTO_GPIO_63_32_PIN_SELECT);
			}
			break;
		case COMPONENT_EXP_DAUGTHER_CARD:
			if (state){
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_SELECT);
			}else{
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | GPIO_PIN_49, COMCERTO_GPIO_63_32_PIN_SELECT);
			}
			break;
		case COMPONENT_RGMII0:
			if (state){
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) & ~GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_SELECT);
			}else{
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | GPIO_PIN_46, COMCERTO_GPIO_63_32_PIN_SELECT);
			}
			break;
		case COMPONENT_RGMII1:
			if (state){
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) | GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT)& ~GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_SELECT);
			}else{
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) & ~GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel( reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT_EN) & ~GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_OUTPUT_EN);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_OUTPUT) | GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_OUTPUT);
				reset_writel(reset_readl(COMCERTO_GPIO_63_32_PIN_SELECT) | GPIO_PIN_45, COMCERTO_GPIO_63_32_PIN_SELECT);
			}
			break;
		default:
			break;
	}

	spin_unlock_irqrestore(&gpio_lock,flags);
}
EXPORT_SYMBOL(GPIO_reset_external_device);
#endif

void reset_init(void)
{
	spin_lock_init(&reset_lock);

	/* Do Any boottime Reset/Out-Of-Reset to devices if required*/
}
