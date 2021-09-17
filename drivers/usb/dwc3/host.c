#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/platform_device.h>
#if defined (MY_DEF_HERE)
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/xhci_pdriver.h>
#include "../host/xhci.h"
#endif  

#include "core.h"

int dwc3_host_init(struct dwc3 *dwc)
{
	struct platform_device	*xhci;
#if defined (MY_DEF_HERE)
	struct usb_hcd *hcd;
	struct xhci_hcd *xhci_dev;
	struct usb_xhci_pdata	pdata;
#endif  
	int			ret;

	xhci = platform_device_alloc("xhci-hcd", PLATFORM_DEVID_AUTO);
	if (!xhci) {
		dev_err(dwc->dev, "couldn't allocate xHCI device\n");
		ret = -ENOMEM;
		goto err0;
	}

	dma_set_coherent_mask(&xhci->dev, dwc->dev->coherent_dma_mask);

	xhci->dev.parent	= dwc->dev;
	xhci->dev.dma_mask	= dwc->dev->dma_mask;
	xhci->dev.dma_parms	= dwc->dev->dma_parms;

	dwc->xhci = xhci;

	ret = platform_device_add_resources(xhci, dwc->xhci_resources,
						DWC3_XHCI_RESOURCES_NUM);
	if (ret) {
		dev_err(dwc->dev, "couldn't add resources to xHCI device\n");
		goto err1;
	}
#if defined (MY_DEF_HERE)

	memset(&pdata, 0, sizeof(pdata));

#ifdef CONFIG_DWC3_HOST_USB3_LPM_ENABLE
	pdata.usb3_lpm_capable = 1;
#endif

	ret = platform_device_add_data(xhci, &pdata, sizeof(pdata));
	if (ret) {
		dev_err(dwc->dev, "couldn't add platform data to xHCI device\n");
		goto err1;
	}

#endif  

	ret = platform_device_add(xhci);
	if (ret) {
		dev_err(dwc->dev, "failed to register xHCI device\n");
		goto err1;
	}
#if defined (MY_DEF_HERE)

	hcd = platform_get_drvdata(xhci);
	xhci_dev = hcd_to_xhci(hcd);
	xhci_dev->shared_hcd->phy = dwc->usb3_phy;
#endif  

	return 0;

err1:
	platform_device_put(xhci);

err0:
	return ret;
}

void dwc3_host_exit(struct dwc3 *dwc)
{
	platform_device_unregister(dwc->xhci);
}
