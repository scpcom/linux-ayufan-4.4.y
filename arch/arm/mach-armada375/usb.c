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

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>

#include <asm/io.h>
#include <asm/irq.h>

#include "mvCommon.h"
#include "mvDebug.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvUnitMap.h"
#include "mvSysUsbApi.h"
#include "usb/mvUsbRegs.h"
#include "usb/mvUsb.h"

u32 mvIsUsbHost = 0x03;

#define MV_USB_DMA_MASK         0xffffffff
#define MAX_USB2_PORTS           2

static void mv_usb_release(struct device *dev)
{
	struct platform_device  *pdev = to_platform_device(dev);

	/* normally not freed */
	pr_info("mv_usb_release\n");

	kfree(pdev->resource);
	kfree(pdev->dev.dma_mask);
	kfree(pdev);
}

/*
 * mv_usb2_init
 * Params:
 * start: first device number
 * num: number of device to initialize
 */
static int __init mv_usb2_init(void)
{
	int status, dev, num, id;
	struct platform_device *ehci_dev;
	int irq_num[MAX_USB2_PORTS] = { IRQ_GLOBAL_USB2_IP,  IRQ_GLOBAL_USB2_IP + 1};
	int mac_id[MAX_USB2_PORTS] = { 1, 0};
	resource_size_t ehci_regs_base;

	num = mvCtrlUsbMaxGet();
	if (num > MAX_USB2_PORTS) {
		pr_info("WARNING: Limited USB ports number to %d\n", MAX_USB2_PORTS);
		num = MAX_USB2_PORTS;
	}

	/*
	* If a single USB2 is enabled it's ID is _1_ not 0
	* So the loop iterrator can not used as device ID
	*/
	for (id = 0; id < num; id++) {
		dev = mac_id[id];

		if (MV_FALSE == mvCtrlPwrClckGet(USB_UNIT_ID, dev)) {
			pr_info("\nWarning Integrated USB %d is Powered Off\n", dev);
			continue;
		}

		/* Check if this USB is mapped to this AMP group */
		if (MV_FALSE == mvUnitMapIsMine(USB0 + dev))
			continue;

		pr_info("Initialising USB2-%d HAL\n", dev);
		status = mvSysUsbInit(dev, 1);

		if (status != MV_OK) {
			pr_info("Error: mvSysUsbInit failed with code %d\n", status);
			continue;
		}
		ehci_dev = kmalloc(sizeof(struct platform_device), GFP_KERNEL);
		if (ehci_dev == NULL) {
			pr_info("Can't allocate platform_device structure - %d bytes\n",
				sizeof(struct platform_device));
			return 1;
		}
		memset(ehci_dev, 0, sizeof(struct platform_device));

		ehci_dev->name = "ehci_marvell";
		ehci_dev->id   = id;
		ehci_dev->num_resources  = 2;

		/* Set the EHCI registers and interrupts resources */
		ehci_dev->resource = kmalloc(2 * sizeof(struct resource), GFP_KERNEL);
		if (ehci_dev->resource == NULL) {
			pr_info("Can't allocate 2 resource structure - %d bytes\n",
			       2 * sizeof(struct resource));
			kfree(ehci_dev);
			return 1;
		}
		memset(ehci_dev->resource, 0, 2 * sizeof(struct resource));

		ehci_regs_base = INTER_REGS_VIRT_BASE | MV_USB_CORE_CAP_LENGTH_REG(dev);
		ehci_dev->resource[0].start = ehci_regs_base;
		ehci_dev->resource[0].end   = ehci_regs_base + _4K;
		ehci_dev->resource[0].flags = IORESOURCE_DMA;

		ehci_dev->resource[1].start = irq_num[dev];
		ehci_dev->resource[1].flags = IORESOURCE_IRQ;

		ehci_dev->dev.dma_mask	= kmalloc(sizeof(u64), GFP_KERNEL);
		*ehci_dev->dev.dma_mask	= MV_USB_DMA_MASK;
		ehci_dev->dev.coherent_dma_mask  = ~0;

		ehci_dev->dev.release = mv_usb_release;
		dev_set_name(&ehci_dev->dev, "%s", "platform");

		/* Register the device */
		status = platform_device_register(ehci_dev);
		if (status) {
			pr_info("Failed registering Marvell2 USB EHCI controller #%d, status=%d\n",
				dev, status);
			return status;
		}
		pr_info("Registered Marvell USB2 EHCI host controller %d\n", dev);
	}
	return 0;
}

#define USB3_WIN_CTRL(w)	(0x0 + ((w) * 8))
#define USB3_WIN_BASE(w)	(0x4 + ((w) * 8))
#define USB3_MAX_WINDOWS	4
#define USB3_XHCI_REGS_SIZE	_16K

static u64 mv_usb3_dmamask = 0xffffffffUL;
static struct resource mv_usb3_resources[2] = {
	[0] = {
		.start	= USB3_REGS_PHYS_BASE,
		.end	= USB3_REGS_PHYS_BASE + USB3_XHCI_REGS_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_GLOBAL_USB3_IP,
		.end	= IRQ_GLOBAL_USB3_IP,
		.flags	= IORESOURCE_IRQ,
	},
};

static void __init mv_usb3_conf_mbus_windows(void __iomem *base,
			const struct mbus_dram_target_info *dram)
{
	int win;

	/* Clear all existing windows */
	for (win = 0; win < USB3_MAX_WINDOWS; win++) {
		writel(0, base + USB3_WIN_CTRL(win));
		writel(0, base + USB3_WIN_BASE(win));
	}

	/* Program each DRAM CS in a seperate window */
	for (win = 0; win < dram->num_cs; win++) {
		const struct mbus_dram_window *cs = dram->cs + win;

		writel(((cs->size - 1) & 0xffff0000) | (cs->mbus_attr << 8) |
				(dram->mbus_dram_target_id << 4) | 1,
				 base + USB3_WIN_CTRL(win));

		writel((cs->base & 0xffff0000), base + USB3_WIN_BASE(win));
	}
}

void __init mv_usb3_init(struct mbus_dram_target_info *dram)
{
	int ret = -ENOMEM;
	struct platform_device	*xhci;
	u8 __iomem *usb_mac_regs;
	int reg, mask;

	if (MV_FALSE == mvCtrlPwrClckGet(USB3_UNIT_ID, 0)) {
		pr_warn("Warning: Integrated USB3 is Powered Off\n");
		return;
	}

	if (MV_FALSE == mvUnitMapIsMine(USB0))
		return;

	/* Allocate an XHCI device */
	xhci = platform_device_alloc("xhci-hcd", -1);
	if (!xhci) {
		pr_err("Couldn't allocate XHCI device\n");
		goto err0;
	}

	ret = platform_device_add_resources(xhci, mv_usb3_resources,
					    ARRAY_SIZE(mv_usb3_resources));
	if (ret) {
		pr_err("Couldn't add resources to XHCI device\n");
		goto err1;
	}

	dma_set_coherent_mask(&xhci->dev, 0xffffffff);
	xhci->dev.dma_mask = &mv_usb3_dmamask;

	/* Map the DDR address space to the XHCI */
	mv_usb3_conf_mbus_windows((void *)(INTER_REGS_VIRT_BASE +
			MV_USB3_REGS_BASE(0)), dram);

	/* Register the device */
	ret = platform_device_add(xhci);
	if (ret) {
		pr_err("Failed to register xHCI device\n");
		goto err1;
	}

	pr_info("USB3 XHCI Device registered successfully\n");
	return;

err1:
	platform_device_put(xhci);
err0:
	return;
}

void __init mv_usb_init(struct mbus_dram_target_info *dram)
{
	int reg;

	if (mvCtrlUsb3MaxGet() > 0)
		mv_usb3_init(dram);

	if (mvCtrlUsbMaxGet() > 0)
		mv_usb2_init();

	/*
	* If 2 USB2 MACs are enabled (MAC0 & MAC1), UTMI PHY-0 should be
	* connected to MAC0. If 1 USB2 is enabled (MAC1) UTMI PHY-0
	* should be connected to USB3 MAC.
	*/
	reg = readl(INTER_REGS_VIRT_BASE + USB_CLUSTER_CONTROL);
	reg &= (~1);
	if (mvCtrlUsbMaxGet() < 2)
		reg |= 1; /* Connect UTMI to USB3 */
	pr_info("-----> 0x18400 = 0x%x\n", reg);
	writel(reg, INTER_REGS_VIRT_BASE + USB_CLUSTER_CONTROL);
}
