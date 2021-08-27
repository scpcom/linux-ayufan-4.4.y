#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
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

//#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/platform_device.h>
                                                                                                                             
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

#define MV_USB_DMA_MASK		0xffffffff
#define MAX_USB_PORTS		3

static char usb_dev_name[]  = "mv_udc";
static char usb_host_name[] = "ehci_marvell";
static char usb_bus_name[]  = "platform";

static void mv_usb_release(struct device *dev)
{
    struct platform_device  *pdev = to_platform_device(dev); 

    /* normally not freed */
    printk("mv_usb_release\n");

    kfree(pdev->resource);
    kfree(pdev->dev.dma_mask);
    kfree(pdev);
} 

int mv_usb_resume(int dev)
{
	int status, isHost;
	char *name_ptr;

	if (MV_FALSE == mvCtrlPwrClckGet(USB_UNIT_ID, dev)) {
		printk(KERN_DEBUG "\nWarning Integrated USB %d is Powered Off\n", dev);
		return -EINVAL;
	}

	/* Check if this USB is mapped to this AMP group - YY */
	if(MV_FALSE == mvUnitMapIsMine(USB0 + dev))
		return -EINVAL;

	isHost = mvIsUsbHost & (1 << dev);
	name_ptr = isHost ? usb_host_name : usb_dev_name;

	printk(KERN_DEBUG "registered dev#%d as a %s\n", dev, name_ptr);
	status = mvSysUsbInit(dev, isHost);

	return status;
}
#ifdef CONFIG_SYNO_ARMADA_ARCH
#include <linux/export.h>
EXPORT_SYMBOL(mv_usb_resume);
#endif

static int __init   mv_usb_init(void)
{
	int                     status, dev, num, isHost;
	char*                   name_ptr;
	struct platform_device* mv_usb_dev_ptr;
	int 			irq_num[3] = {	IRQ_AURORA_USB0,
						IRQ_AURORA_USB1,
						IRQ_AURORA_USB2};

	num = mvCtrlUsbMaxGet(); 
	if (num > MAX_USB_PORTS) {
		printk("WARNING: Limited USB ports number to %d\n", MAX_USB_PORTS);
		num = MAX_USB_PORTS;
	}

	for(dev=0; dev<num; dev++)
	{
		if (MV_FALSE == mvCtrlPwrClckGet(USB_UNIT_ID, dev))
		{
			printk("\nWarning Integrated USB %d is Powered Off\n",dev);
			continue;
		}

		/* Check if this USB is mapped to this AMP group - YY */
		if(MV_FALSE == mvUnitMapIsMine(USB0 + dev))
		{
			continue;
		}

		isHost = mvIsUsbHost & (1 << dev);

		if(isHost)
			name_ptr = usb_host_name;
		else
			name_ptr = usb_dev_name;

		printk("registered dev#%d asa %s\n",dev,name_ptr);
		status = mvSysUsbInit(dev, isHost);

		mv_usb_dev_ptr = kmalloc(sizeof(struct platform_device), GFP_KERNEL);
		if(mv_usb_dev_ptr == NULL)
		{
			printk("Can't allocate platform_device structure - %d bytes\n",
					sizeof(struct platform_device) );
			return 1;
		}
		memset(mv_usb_dev_ptr, 0, sizeof(struct platform_device) );

		mv_usb_dev_ptr->name               = name_ptr;
		mv_usb_dev_ptr->id                 = dev;

		mv_usb_dev_ptr->num_resources  = 2;

		mv_usb_dev_ptr->resource = (struct resource*)kmalloc(2*sizeof(struct resource), GFP_KERNEL);
		if(mv_usb_dev_ptr->resource == NULL)
		{
			printk("Can't allocate 2 resource structure - %d bytes\n",
					2*sizeof(struct resource) );
			kfree(mv_usb_dev_ptr);
			return 1;
		}
		memset(mv_usb_dev_ptr->resource, 0, 2*sizeof(struct resource));

		mv_usb_dev_ptr->resource[0].start =
			( INTER_REGS_BASE | MV_USB_CORE_CAP_LENGTH_REG(dev));
		mv_usb_dev_ptr->resource[0].end   =
			((INTER_REGS_BASE | MV_USB_CORE_CAP_LENGTH_REG(dev)) + 4096);
		mv_usb_dev_ptr->resource[0].flags = IORESOURCE_DMA;

		mv_usb_dev_ptr->resource[1].start = irq_num[dev];
		mv_usb_dev_ptr->resource[1].flags = IORESOURCE_IRQ;

		mv_usb_dev_ptr->dev.dma_mask           = kmalloc(sizeof(u64), GFP_KERNEL);
		*mv_usb_dev_ptr->dev.dma_mask          = MV_USB_DMA_MASK;

		mv_usb_dev_ptr->dev.coherent_dma_mask  = ~0;
		mv_usb_dev_ptr->dev.release            = mv_usb_release;
		dev_set_name(&mv_usb_dev_ptr->dev, "%s", usb_bus_name);

		printk("Marvell USB %s controller #%d: %p\n",
				isHost ? "EHCI Host" : "Gadget", dev, mv_usb_dev_ptr);

		status = platform_device_register(mv_usb_dev_ptr);
		if (status)
		{
			printk("Can't register Marvell USB EHCI controller #%d, status=%d\n", 
					dev, status);
			return status;
		}
	}
	return 0;
}

subsys_initcall(mv_usb_init);
