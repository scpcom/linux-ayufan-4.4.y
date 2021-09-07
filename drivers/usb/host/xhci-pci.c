#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/pci.h>

#include "xhci.h"

#define PCI_VENDOR_ID_FRESCO_LOGIC	0x1b73
#define PCI_DEVICE_ID_FRESCO_LOGIC_PDK	0x1000

#ifndef MY_ABC_HERE  
#define PCI_VENDOR_ID_ETRON   0x1b6f
#define PCI_DEVICE_ID_ASROCK_P67  0x7023
#else
extern unsigned short xhci_vendor;
#endif

static const char hcd_name[] = "xhci_hcd";

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

	hcd->self.sg_tablesize = TRBS_PER_SEGMENT - 2;

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
	xhci_print_registers(xhci);

	if (pdev->vendor == PCI_VENDOR_ID_FRESCO_LOGIC &&
			pdev->device == PCI_DEVICE_ID_FRESCO_LOGIC_PDK &&
			pdev->revision == 0x0) {
			xhci->quirks |= XHCI_RESET_EP_QUIRK;
			xhci_dbg(xhci, "QUIRK: Fresco Logic xHC needs configure"
					" endpoint cmd after reset endpoint\n");
	}
	if (pdev->vendor == PCI_VENDOR_ID_NEC)
		xhci->quirks |= XHCI_NEC_HOST;

#ifdef MY_ABC_HERE
	xhci_vendor = pdev->vendor;

  if (pdev->vendor == PCI_VENDOR_ID_ETRON &&
      pdev->device == PCI_DEVICE_ID_ASROCK_P67) {
    xhci_err(xhci, "Etron chip found.\n");
  }
#endif

	retval = xhci_halt(xhci);
	if (retval)
		return retval;

	xhci_dbg(xhci, "Resetting HCD\n");
	 
	retval = xhci_reset(xhci);
	if (retval)
		return retval;
	xhci_dbg(xhci, "Reset complete\n");

	xhci_dbg(xhci, "Calling HCD init\n");
	 
	retval = xhci_init(hcd);
	if (retval)
		return retval;
	xhci_dbg(xhci, "Called HCD init\n");

	pci_read_config_byte(pdev, XHCI_SBRN_OFFSET, &xhci->sbrn);
	xhci_dbg(xhci, "Got SBRN %u\n", (unsigned int) xhci->sbrn);

	return xhci_pci_reinit(xhci, pdev);
}

#ifdef CONFIG_USB_ETRON_HCD_MODULE
static int xhci_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	if (dev->vendor == 0x1b6f)
		return -ENODEV;

	return usb_hcd_pci_probe(dev, id);
}
#endif

static const struct hc_driver xhci_pci_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"xHCI Host Controller",
	.hcd_priv_size =	sizeof(struct xhci_hcd),

	.irq =			xhci_irq,
	.flags =		HCD_MEMORY | HCD_USB3,

	.reset =		xhci_pci_setup,
	.start =		xhci_run,
	 
	.stop =			xhci_stop,
	.shutdown =		xhci_shutdown,

	.urb_enqueue =		xhci_urb_enqueue,
	.urb_dequeue =		xhci_urb_dequeue,
	.alloc_dev =		xhci_alloc_dev,
	.free_dev =		xhci_free_dev,
	.add_endpoint =		xhci_add_endpoint,
	.drop_endpoint =	xhci_drop_endpoint,
	.endpoint_reset =	xhci_endpoint_reset,
	.check_bandwidth =	xhci_check_bandwidth,
	.reset_bandwidth =	xhci_reset_bandwidth,
	.address_device =	xhci_address_device,
	.update_hub_device =	xhci_update_hub_device,
	.reset_device =		xhci_discover_or_reset_device,

	.get_frame_number =	xhci_get_frame,

	.hub_control =		xhci_hub_control,
	.hub_status_data =	xhci_hub_status_data,
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

#ifdef CONFIG_USB_ETRON_HCD_MODULE
	.probe =	xhci_pci_probe,
#else
	.probe =	usb_hcd_pci_probe,
#endif
	.remove =	usb_hcd_pci_remove,
	 
	.shutdown = 	usb_hcd_pci_shutdown,
};

int xhci_register_pci(void)
{
	return pci_register_driver(&xhci_pci_driver);
}

void xhci_unregister_pci(void)
{
	pci_unregister_driver(&xhci_pci_driver);
}
