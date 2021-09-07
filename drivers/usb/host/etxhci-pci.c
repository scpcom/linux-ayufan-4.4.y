 
#include <linux/pci.h>

#include "etxhci.h"

#define PCI_VENDOR_ID_ETRON		0x1b6f
#define PCI_DEVICE_ID_ETRON_EJ168	0x7023
#define PCI_DEVICE_ID_ETRON_EJ188	0x7052

#ifdef SYNO_USB3_PCI_ID_DEFINE
extern unsigned short xhci_vendor;
#endif

static const char hcd_name[] = "etxhci_hcd_130207";

static int xhci_pci_reinit(struct xhci_hcd *xhci, struct pci_dev *pdev)
{
	 
	if (!pci_set_mwi(pdev))
		xhci_dbg(xhci, "MWI active\n");

	xhci_dbg(xhci, "Finished xhci_pci_reinit\n");
	return 0;
}

static int xhci_pci_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd		*xhci = hcd_to_xhci(hcd);
	struct pci_dev		*pdev = to_pci_dev(hcd->self.controller);
	int			retval;
	u32			temp;

	hcd->self.sg_tablesize = ~0;

	hcd->chip_id = HCD_CHIP_ID_UNKNOWN;
	if (pdev->vendor == PCI_VENDOR_ID_ETRON) {
		pci_read_config_dword(pdev, 0x58, &xhci->hcc_params1);
		xhci->hcc_params1 &= 0xffff;
		xhci_init_ejxxx(xhci);

		if (pdev->device == PCI_DEVICE_ID_ETRON_EJ168)
			hcd->chip_id = HCD_CHIP_ID_ETRON_EJ168;
		else if (pdev->device == PCI_DEVICE_ID_ETRON_EJ188)
			hcd->chip_id = HCD_CHIP_ID_ETRON_EJ188;

		xhci_dbg(xhci, "Etron chip ID %02x\n", hcd->chip_id);
		xhci->quirks |= XHCI_BROKEN_MSI;
		xhci->quirks |= XHCI_HUB_INFO_QUIRK;
		xhci->quirks |= XHCI_RESET_ON_RESUME;
		xhci_dbg(xhci, "QUIRK: Resetting on resume\n");
	}

	if (((xhci->hcc_params1 & 0xff) == 0x30) ||
		((xhci->hcc_params1 & 0xff) == 0x40)) {
		xhci->quirks |= XHCI_EP_INFO_QUIRK;
	}

	xhci->cap_regs = hcd->regs;
	xhci->op_regs = hcd->regs +
		HC_LENGTH(xhci_readl(xhci, &xhci->cap_regs->hc_capbase));
	xhci->run_regs = hcd->regs +
		(xhci_readl(xhci, &xhci->cap_regs->run_regs_off) & RTSOFF_MASK);
	 
	xhci->hcs_params1 = xhci_readl(xhci, &xhci->cap_regs->hcs_params1);
	xhci->hcs_params2 = xhci_readl(xhci, &xhci->cap_regs->hcs_params2);
	xhci->hcs_params3 = xhci_readl(xhci, &xhci->cap_regs->hcs_params3);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hc_capbase);
	xhci->hci_version = HC_VERSION(xhci->hcc_params);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hcc_params);
	etxhci_print_registers(xhci);

#ifdef SYNO_USB3_PCI_ID_DEFINE
	xhci_vendor = pdev->vendor;
#endif

	retval = etxhci_halt(xhci);
	if (retval)
		return retval;

	xhci_dbg(xhci, "Resetting HCD\n");
	 
	retval = etxhci_reset(xhci);
	if (retval)
		return retval;
	xhci_dbg(xhci, "Reset complete\n");

	temp = xhci_readl(xhci, &xhci->cap_regs->hcc_params);
	if (HCC_64BIT_ADDR(temp)) {
		xhci_dbg(xhci, "Enabling 64-bit DMA addresses.\n");
		dma_set_mask(hcd->self.controller, DMA_BIT_MASK(64));
	} else {
		dma_set_mask(hcd->self.controller, DMA_BIT_MASK(32));
	}

	xhci_dbg(xhci, "Calling HCD init\n");
	 
	retval = etxhci_init(hcd);
	if (retval)
		return retval;
	xhci_dbg(xhci, "Called HCD init\n");

	pci_read_config_byte(pdev, XHCI_SBRN_OFFSET, &xhci->sbrn);
	xhci_dbg(xhci, "Got SBRN %u\n", (unsigned int) xhci->sbrn);

	return xhci_pci_reinit(xhci, pdev);
}

#ifdef CONFIG_PM
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static int xhci_pci_suspend(struct usb_hcd *hcd, bool do_wakeup)
#else
static int xhci_pci_suspend(struct usb_hcd *hcd)
#endif
{
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	int	retval = 0;

	if (hcd->state != HC_STATE_SUSPENDED)
		return -EINVAL;

	retval = etxhci_suspend(xhci);

	return retval;
}

static int xhci_pci_resume(struct usb_hcd *hcd, bool hibernated)
{
	struct xhci_hcd		*xhci = hcd_to_xhci(hcd);
	int			retval = 0;

	retval = etxhci_resume(xhci, hibernated);

	return retval;
}
#endif  

static int xhci_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	if (dev->vendor != 0x1b6f)
		return -ENODEV;

	return usb_hcd_pci_probe(dev, id);
}

static const struct hc_driver xhci_pci_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"Etron xHCI Host Controller",
	.hcd_priv_size =	sizeof(struct xhci_hcd),

	.irq =			etxhci_irq,
	.flags =		HCD_MEMORY | HCD_USB3,

	.reset =		xhci_pci_setup,
	.start =		etxhci_run,
#ifdef CONFIG_PM
	.pci_suspend =          xhci_pci_suspend,
	.pci_resume =           xhci_pci_resume,
#endif
	.stop =			etxhci_stop,
	.shutdown =		etxhci_shutdown,

	.urb_enqueue =		etxhci_urb_enqueue,
	.urb_dequeue =		etxhci_urb_dequeue,
	.alloc_dev =		etxhci_alloc_dev,
	.free_dev =		etxhci_free_dev,
	.alloc_streams =	etxhci_alloc_streams,
	.free_streams =		etxhci_free_streams,
	.add_endpoint =		etxhci_add_endpoint,
	.drop_endpoint =	etxhci_drop_endpoint,
	.endpoint_reset =	etxhci_endpoint_reset,
	.check_bandwidth =	etxhci_check_bandwidth,
	.reset_bandwidth =	etxhci_reset_bandwidth,
	.address_device =	etxhci_address_device,
	.update_hub_device =	etxhci_update_hub_device,
	.reset_device =		etxhci_discover_or_reset_device,

	.get_frame_number =	etxhci_get_frame,

	.hub_control =		etxhci_hub_control,
	.hub_status_data =	etxhci_hub_status_data,
	.bus_suspend =		etxhci_bus_suspend,
	.bus_resume =		etxhci_bus_resume,
};

static const struct pci_device_id pci_ids[] = { {
	 
	PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_XHCI, ~0),
	.driver_data =	(unsigned long) &xhci_pci_hc_driver,
	},
	{   }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

static struct pci_driver xhci_pci_driver = {
	.name =		(char *) hcd_name,
	.id_table =	pci_ids,

	.probe =	xhci_pci_probe,
	.remove =	usb_hcd_pci_remove,
	 
	.shutdown = 	usb_hcd_pci_shutdown,
#ifdef CONFIG_PM_SLEEP
	.driver = {
		.pm = &usb_hcd_pci_pm_ops
	},
#endif
};

int etxhci_register_pci(void)
{
	return pci_register_driver(&xhci_pci_driver);
}

void etxhci_unregister_pci(void)
{
	pci_unregister_driver(&xhci_pci_driver);
}
