/*
 * xhci-comcerto.c - Comcerto-2000 Platform specific routienes.
 *
 * Author: Makarand Pawagi
 */


#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "xhci.h"

#include <linux/clk.h>
#include <mach/reset.h>
#include <mach/hardware.h>

#define xhci_writel(v,r) writel(v, (void*)(r))

extern int usb3_clk_internal;
/* USB 3.0 clock */
static struct clk *usb3_clk;

#ifdef CONFIG_PM

u32 usb3_suspended = 0;

int comcerto_xhci_bus_suspend(struct usb_hcd *hcd)
{
	int error_status = 0;

	if (usb3_suspended) {
		pr_err("comcerto_xhci_bus_suspend: USB 3.0 Already Suspended \n");
		return error_status;
	}

	error_status = xhci_bus_suspend(hcd);

	if (!error_status) {

		/* APPLYING THE RESET TO USB3 UTMI */
		c2000_block_reset(COMPONENT_UTMI_USB1, 1);

		/* APPLYING THE RESET TO USB3 PHY */
		c2000_block_reset(COMPONENT_USB1_PHY, 1);

		/* Disable the Clock */
		clk_disable(usb3_clk);

		usb3_suspended = 1;
	}

	return error_status;
}

int comcerto_xhci_bus_resume(struct usb_hcd *hcd)
{
	int val;
	int error_status = 0;

	if (!usb3_suspended) {
		pr_err("comcerto_xhci_bus_resume: USB 3.0 Already in Resume state \n");
		return error_status;
	}

	/* Enable the Clock */
	if (clk_enable(usb3_clk)){
		pr_err("comcerto_start_xhc:Unable to enable the usb1 clock \n");
	}

	/* Bring usb3 PHY out of reset */
	c2000_block_reset(COMPONENT_USB1_PHY, 0);

	for (val = 0 ; val < 50 ; val++)
		udelay(1000);

    /* Bring usb3 UTMI out of reset */
	c2000_block_reset(COMPONENT_UTMI_USB1, 0);

	for (val = 0 ; val < 50 ; val++)
		udelay(1000);

	error_status = xhci_bus_resume(hcd);

	if (error_status) {
		/* xhci_bus_resume is not successfull keep USB3 in suspend mode */

		/* Put USB3 PHY, UTMI and Controller in Reset */

		/* APPLYING THE RESET TO USB3 UTMI */
		c2000_block_reset(COMPONENT_UTMI_USB1, 1);

		/* APPLYING THE RESET TO USB3 PHY */
		c2000_block_reset(COMPONENT_USB1_PHY, 1);

		/* Disable the Clock */
		clk_disable(usb3_clk);

		return error_status;
	}

	usb3_suspended = 0;

	return error_status;
}
#endif

static void comcerto_usb3_phy_init(void)
{
	u32 val;

        xhci_writel(0x00E00080, USB3_PHY_BASE + 0x10);

	//Configuration for internal clock
	if(usb3_clk_internal)
	{
		printk(KERN_INFO "%s():: USB3.0 clock selected: internal\n", __func__);

		if(HAL_get_ref_clk() == REF_CLK_24MHZ)
			val = 0x420E82A8;
		else
			val = 0x420E82A9;
	}
	else
	{
		val = 0x4209927A;
		printk(KERN_INFO "%s():: USB3.0 clock selected: external\n", __func__);
	}

	xhci_writel(val, USB3_PHY_BASE + 0x20);
        xhci_writel(0x69C34F53, USB3_PHY_BASE + 0x24);
        xhci_writel(0x0005D815, USB3_PHY_BASE + 0x28);
        xhci_writel(0x00000801, USB3_PHY_BASE + 0x2C);
}


void comcerto_start_xhci(void)
{
        printk(KERN_INFO "### %s\n", __func__);

#if defined(CONFIG_C2K_MFCN_EVM)
	printk("%s: Reseting usb3...\n", __func__);
	GPIO_reset_external_device(COMPONENT_USB_HUB, 0);
#endif
		/* Enable the USB 3.0 controller clock */
		/* Get the usb3 clock structure  */
		usb3_clk = clk_get(NULL,"usb1");

		/* Enable the Clock */
		if (clk_enable(usb3_clk)){
			pr_err("comcerto_start_xhci:Unable to enable the usb1 clock \n");
		}

		/* APPLYING THE RESET TO USB3 UTMI */
		c2000_block_reset(COMPONENT_UTMI_USB1, 1);

		/* APPLYING THE RESET TO USB3 PHY */
		c2000_block_reset(COMPONENT_USB1_PHY, 1);

		/* APLLYING RESET TO USB3 AXI RESET */
		c2000_block_reset(COMPONENT_AXI_USB1, 1);

		comcerto_usb3_phy_init();


		/* Bring usb3 PHY out of reset */
		c2000_block_reset(COMPONENT_USB1_PHY, 0);

		udelay(1000);

		/* Bring usb3 UTMI out of reset */
		c2000_block_reset(COMPONENT_UTMI_USB1, 0);

		udelay(1000);

		/* Bring usb3 Controller out of reset */
		c2000_block_reset(COMPONENT_AXI_USB1, 0);

		udelay(1000);
}

void comcerto_stop_xhci(void)
{
        printk(KERN_INFO "### %s\n", __func__);

		/* APPLYING THE RESET TO USB3 UTMI */
		c2000_block_reset(COMPONENT_UTMI_USB1, 1);

		/* APPLYING THE RESET TO USB3 PHY */
		c2000_block_reset(COMPONENT_USB1_PHY, 1);

		/* APLLYING RESET TO USB3 AXI RESET */
		c2000_block_reset(COMPONENT_AXI_USB1, 1);

		/* Disable the Clock */
		clk_disable(usb3_clk);

		/* Release the clock */
		clk_put(usb3_clk);
}

