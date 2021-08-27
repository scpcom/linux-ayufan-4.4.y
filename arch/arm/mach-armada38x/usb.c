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
#define MAX_USB_PORTS           1

static const char usb_dev_name[] = "mv_udc";
static const char usb_host_name[] = "ehci_marvell";
static const char usb_bus_name[] = "platform";

static void mv_usb_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	/* normally not freed */
	printk(KERN_INFO "mv_usb_release\n");

	kfree(pdev->resource);
	kfree(pdev->dev.dma_mask);
	kfree(pdev);
}

static int __init mv_usb2_init(void)
{
	int status, dev, num, isHost;
	char *name_ptr;
	struct platform_device *mv_usb_dev_ptr;
	int irq_num[MAX_USB_PORTS] = { IRQ_GLOBAL_USB2_IP };

	num = 1;

	for (dev = 0; dev < num; dev++) {
		if (MV_FALSE == mvCtrlPwrClckGet(USB_UNIT_ID, dev)) {
			printk(KERN_INFO "\nWarning Integrated USB %d is Powered Off\n",
			       dev);
			continue;
		}

		/* Check if this USB is mapped to this AMP group */
		if (MV_FALSE == mvUnitMapIsMine(USB0 + dev))
			continue;

		isHost = mvIsUsbHost & (1 << dev);

		if (isHost)
			name_ptr = usb_host_name;
		else
			name_ptr = usb_dev_name;

		printk(KERN_INFO "registered dev#%d asa %s\n", dev, name_ptr);
		status = mvSysUsbInit(dev, isHost, 0);

		if (status != MV_OK) {
			pr_info("Error: mvSysUsbInit failed with code %d\n", status);
			continue;
		}

		mv_usb_dev_ptr =
		    kmalloc(sizeof(struct platform_device), GFP_KERNEL);
		if (mv_usb_dev_ptr == NULL) {
			printk
			    ("Can't allocate platform_device structure - %d bytes\n",
			     sizeof(struct platform_device));
			return 1;
		}
		memset(mv_usb_dev_ptr, 0, sizeof(struct platform_device));

		mv_usb_dev_ptr->name = name_ptr;
		mv_usb_dev_ptr->id = dev;

		mv_usb_dev_ptr->num_resources = 2;

		mv_usb_dev_ptr->resource = kmalloc(2 * sizeof(struct resource),
					       GFP_KERNEL);
		if (mv_usb_dev_ptr->resource == NULL) {
			printk
			    ("Can't allocate 2 resource structure - %d bytes\n",
			     2 * sizeof(struct resource));
			kfree(mv_usb_dev_ptr);
			return 1;
		}
		memset(mv_usb_dev_ptr->resource, 0,
		       2 * sizeof(struct resource));

		mv_usb_dev_ptr->resource[0].start =
		    (INTER_REGS_VIRT_BASE | MV_USB_CORE_CAP_LENGTH_REG(dev));
		mv_usb_dev_ptr->resource[0].end =
		    ((INTER_REGS_VIRT_BASE | MV_USB_CORE_CAP_LENGTH_REG(dev)) +
		     4096);
		mv_usb_dev_ptr->resource[0].flags = IORESOURCE_DMA;

		mv_usb_dev_ptr->resource[1].start = irq_num[dev];
		mv_usb_dev_ptr->resource[1].flags = IORESOURCE_IRQ;

		mv_usb_dev_ptr->dev.dma_mask = kmalloc(sizeof(u64), GFP_KERNEL);
		*mv_usb_dev_ptr->dev.dma_mask = MV_USB_DMA_MASK;

		mv_usb_dev_ptr->dev.coherent_dma_mask = ~0;
		mv_usb_dev_ptr->dev.release = mv_usb_release;
		dev_set_name(&mv_usb_dev_ptr->dev, "%s", usb_bus_name);

		printk(KERN_INFO "Marvell USB %s controller #%d: %p\n",
		       isHost ? "EHCI Host" : "Gadget", dev, mv_usb_dev_ptr);

		status = platform_device_register(mv_usb_dev_ptr);
		if (status) {
			printk
			    ("Can't register Marvell USB EHCI controller #%d, status=%d\n",
			     dev, status);
			return status;
		}
	}
	return 0;
}

#define USB3_WIN_CTRL(w)	(0x0 + ((w) * 8))
#define USB3_WIN_BASE(w)	(0x4 + ((w) * 8))
#define USB3_MAX_WINDOWS	4
#define USB3_XHCI_REGS_SIZE	_32K

static u64 mv_usb3_dmamask = 0xffffffffUL;

static struct resource mv_usb3_resources[2];

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

void __init mv_usb3_init(struct mbus_dram_target_info *dram, int dev)
{
	int ret = -ENOMEM;
	struct platform_device	*xhci;
	u8 __iomem *usb_mac_regs;
	int reg, mask;

	if (MV_FALSE == mvCtrlPwrClckGet(USB_UNIT_ID, 0)) {
		pr_warn("Warning: Integrated USB3 is Powered Off\n");
		return;
	}

	if (MV_FALSE == mvUnitMapIsMine(USB0))
		return;

	/* Allocate an XHCI device */
	xhci = platform_device_alloc("xhci-hcd", dev);

	if (!xhci) {
		pr_err("Couldn't allocate XHCI device\n");
		goto err0;
	}

	/* Set registers and irq resources for XHCI */
	mv_usb3_resources[0].start = INTER_REGS_PHYS_BASE + MV_USB3_REGS_BASE(dev);
	mv_usb3_resources[0].end = INTER_REGS_PHYS_BASE + MV_USB3_REGS_BASE(dev) +
				   USB3_XHCI_REGS_SIZE - 1;
	mv_usb3_resources[0].flags = IORESOURCE_MEM;

	mv_usb3_resources[1].start = IRQ_GLOBAL_USB3_IP(dev);
	mv_usb3_resources[1].end = IRQ_GLOBAL_USB3_IP(dev);
	mv_usb3_resources[1].flags = IORESOURCE_IRQ;

	ret = platform_device_add_resources(xhci, mv_usb3_resources,
			ARRAY_SIZE(mv_usb3_resources));
	if (ret) {
		pr_err("Couldn't add resources to XHCI device\n");
		goto err1;
	}

	dma_set_coherent_mask(&xhci->dev, 0xffffffff);
	xhci->dev.dma_mask = &mv_usb3_dmamask;

	/* Map the DDR address space to the XHCI */
	mv_usb3_conf_mbus_windows((void *)(INTER_REGS_VIRT_BASE + MV_USB3_WIN_BASE(dev)), dram);

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
	int dev;

	/* Init the Legacy USB2 MAC + UTMI PHY 0 */
	mv_usb2_init();

	/* Init the USB3 + USB2 XHCI MAC + UTMI PHY 1+2 */
	for (dev = 0; dev < mvCtrlUsb3MaxGet(); dev++) {
		mvSysUsbInit(dev+1, 1, 1);
		mv_usb3_init(dram, dev);
	}
}
