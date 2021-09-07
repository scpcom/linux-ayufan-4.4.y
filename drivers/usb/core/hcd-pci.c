#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/usb.h>

#include <asm/io.h>
#include <asm/irq.h>

#ifdef CONFIG_PPC_PMAC
#include <asm/machdep.h>
#include <asm/pmac_feature.h>
#include <asm/pci-bridge.h>
#include <asm/prom.h>
#endif

#include "usb.h"
#include "hcd.h"

int usb_hcd_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct hc_driver	*driver;
	struct usb_hcd		*hcd;
	int			retval;

	if (usb_disabled())
		return -ENODEV;

	if (!id)
		return -EINVAL;
	driver = (struct hc_driver *)id->driver_data;
	if (!driver)
		return -EINVAL;

	if (pci_enable_device(dev) < 0)
		return -ENODEV;
	dev->current_state = PCI_D0;

	if (!dev->irq) {
		dev_err(&dev->dev,
			"Found HC with no IRQ.  Check BIOS/PCI %s setup!\n",
			pci_name(dev));
		retval = -ENODEV;
		goto err1;
	}

	hcd = usb_create_hcd(driver, &dev->dev, pci_name(dev));
	if (!hcd) {
		retval = -ENOMEM;
		goto err1;
	}

	if (driver->flags & HCD_MEMORY) {
		 
		hcd->rsrc_start = pci_resource_start(dev, 0);
		hcd->rsrc_len = pci_resource_len(dev, 0);
		if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len,
				driver->description)) {
			dev_dbg(&dev->dev, "controller already in use\n");
			retval = -EBUSY;
			goto err2;
		}
		hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
		if (hcd->regs == NULL) {
			dev_dbg(&dev->dev, "error mapping memory\n");
			retval = -EFAULT;
			goto err3;
		}

	} else {
		 
		int	region;

		for (region = 0; region < PCI_ROM_RESOURCE; region++) {
			if (!(pci_resource_flags(dev, region) &
					IORESOURCE_IO))
				continue;

			hcd->rsrc_start = pci_resource_start(dev, region);
			hcd->rsrc_len = pci_resource_len(dev, region);
			if (request_region(hcd->rsrc_start, hcd->rsrc_len,
					driver->description))
				break;
		}
		if (region == PCI_ROM_RESOURCE) {
			dev_dbg(&dev->dev, "no i/o regions available\n");
			retval = -EBUSY;
			goto err1;
		}
	}

	pci_set_master(dev);

	retval = usb_add_hcd(hcd, dev->irq, IRQF_DISABLED | IRQF_SHARED);
	if (retval != 0)
		goto err4;
	return retval;

 err4:
	if (driver->flags & HCD_MEMORY) {
		iounmap(hcd->regs);
 err3:
		release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	} else
		release_region(hcd->rsrc_start, hcd->rsrc_len);
 err2:
	usb_put_hcd(hcd);
 err1:
	pci_disable_device(dev);
	dev_err(&dev->dev, "init %s fail, %d\n", pci_name(dev), retval);
	return retval;
}
EXPORT_SYMBOL_GPL(usb_hcd_pci_probe);

void usb_hcd_pci_remove(struct pci_dev *dev)
{
	struct usb_hcd		*hcd;

	hcd = pci_get_drvdata(dev);
	if (!hcd)
		return;

	usb_remove_hcd(hcd);
	if (hcd->driver->flags & HCD_MEMORY) {
		iounmap(hcd->regs);
		release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	} else {
		release_region(hcd->rsrc_start, hcd->rsrc_len);
	}
	usb_put_hcd(hcd);
	pci_disable_device(dev);
}
EXPORT_SYMBOL_GPL(usb_hcd_pci_remove);

#ifdef MY_ABC_HERE
#include <linux/kthread.h>
extern struct task_struct *khubd_task;
#endif

void usb_hcd_pci_shutdown(struct pci_dev *dev)
{
	struct usb_hcd		*hcd;

	hcd = pci_get_drvdata(dev);
	if (!hcd)
		return;

#ifdef MY_ABC_HERE
	if (khubd_task != NULL) {
		kthread_stop(khubd_task);
		khubd_task = NULL;
	}
#endif

#ifdef MY_ABC_HERE
	if (hcd->irq >= 0) {
		 
		free_irq(hcd->irq, hcd);
		hcd->irq = -1;
	}
#endif

	if (hcd->driver->shutdown) {
		hcd->driver->shutdown(hcd);
		pci_disable_device(dev);
	}
}
EXPORT_SYMBOL_GPL(usb_hcd_pci_shutdown);

#ifdef	CONFIG_PM_SLEEP

static int check_root_hub_suspended(struct device *dev)
{
	struct pci_dev		*pci_dev = to_pci_dev(dev);
	struct usb_hcd		*hcd = pci_get_drvdata(pci_dev);

	if (!(hcd->state == HC_STATE_SUSPENDED ||
			hcd->state == HC_STATE_HALT)) {
		dev_warn(dev, "Root hub is not suspended\n");
		return -EBUSY;
	}
	return 0;
}

static int hcd_pci_suspend(struct device *dev)
{
	struct pci_dev		*pci_dev = to_pci_dev(dev);
	struct usb_hcd		*hcd = pci_get_drvdata(pci_dev);
	int			retval;

	retval = check_root_hub_suspended(dev);
	if (retval)
		return retval;

	if (pci_dev->current_state != PCI_D0)
		return retval;

	if (hcd->driver->pci_suspend) {
		retval = hcd->driver->pci_suspend(hcd);
		suspend_report_result(hcd->driver->pci_suspend, retval);
		if (retval)
			return retval;
	}

#ifdef CONFIG_USB_ETRON_HUB
	 
	if (!hcd->msix_enabled)
		synchronize_irq(pci_dev->irq);
#else
	synchronize_irq(pci_dev->irq);
#endif

	pci_disable_device(pci_dev);
	return retval;
}

static int hcd_pci_suspend_noirq(struct device *dev)
{
	struct pci_dev		*pci_dev = to_pci_dev(dev);
	struct usb_hcd		*hcd = pci_get_drvdata(pci_dev);
	int			retval;

	retval = check_root_hub_suspended(dev);
	if (retval)
		return retval;

	pci_save_state(pci_dev);

	if (hcd->state == HC_STATE_HALT)
		device_set_wakeup_enable(dev, 0);
	dev_dbg(dev, "wakeup: %d\n", device_may_wakeup(dev));

	retval = pci_prepare_to_sleep(pci_dev);
	if (retval == -EIO) {		 
		dev_dbg(dev, "--> PCI D0 legacy\n");
		retval = 0;
	} else if (retval == 0) {
		dev_dbg(dev, "--> PCI %s\n",
				pci_power_name(pci_dev->current_state));
	} else {
		suspend_report_result(pci_prepare_to_sleep, retval);
		return retval;
	}

#ifdef CONFIG_PPC_PMAC
	 
	if (machine_is(powermac)) {
		struct device_node	*of_node;

		of_node = pci_device_to_OF_node(pci_dev);
		if (of_node)
			pmac_call_feature(PMAC_FTR_USB_ENABLE, of_node, 0, 0);
	}
#endif
	return retval;
}

static int hcd_pci_resume_noirq(struct device *dev)
{
	struct pci_dev		*pci_dev = to_pci_dev(dev);

#ifdef CONFIG_PPC_PMAC
	 
	if (machine_is(powermac)) {
		struct device_node *of_node;

		of_node = pci_device_to_OF_node(pci_dev);
		if (of_node)
			pmac_call_feature(PMAC_FTR_USB_ENABLE,
						of_node, 0, 1);
	}
#endif

	pci_back_from_sleep(pci_dev);
	return 0;
}

static int resume_common(struct device *dev, bool hibernated)
{
	struct pci_dev		*pci_dev = to_pci_dev(dev);
	struct usb_hcd		*hcd = pci_get_drvdata(pci_dev);
	int			retval;

	if (hcd->state != HC_STATE_SUSPENDED) {
		dev_dbg(dev, "can't resume, not suspended!\n");
		return 0;
	}

	retval = pci_enable_device(pci_dev);
	if (retval < 0) {
		dev_err(dev, "can't re-enable after resume, %d!\n", retval);
		return retval;
	}

	pci_set_master(pci_dev);

	clear_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);

	if (hcd->driver->pci_resume) {
		retval = hcd->driver->pci_resume(hcd, hibernated);
		if (retval) {
			dev_err(dev, "PCI post-resume error %d!\n", retval);
			usb_hc_died(hcd);
		}
	}
	return retval;
}

static int hcd_pci_resume(struct device *dev)
{
	return resume_common(dev, false);
}

static int hcd_pci_restore(struct device *dev)
{
	return resume_common(dev, true);
}

struct dev_pm_ops usb_hcd_pci_pm_ops = {
	.suspend	= hcd_pci_suspend,
	.suspend_noirq	= hcd_pci_suspend_noirq,
	.resume_noirq	= hcd_pci_resume_noirq,
	.resume		= hcd_pci_resume,
	.freeze		= check_root_hub_suspended,
	.freeze_noirq	= check_root_hub_suspended,
	.thaw_noirq	= NULL,
	.thaw		= NULL,
	.poweroff	= hcd_pci_suspend,
	.poweroff_noirq	= hcd_pci_suspend_noirq,
	.restore_noirq	= hcd_pci_resume_noirq,
	.restore	= hcd_pci_restore,
};
EXPORT_SYMBOL_GPL(usb_hcd_pci_pm_ops);

#endif	 
