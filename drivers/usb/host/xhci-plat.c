#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "xhci.h"

#if defined(MY_ABC_HERE)
#include "xhci-comcerto2000.h"
#endif

static void xhci_plat_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	 
	xhci->quirks |= XHCI_BROKEN_MSI;
}

static int xhci_plat_setup(struct usb_hcd *hcd)
{
	return xhci_gen_setup(hcd, xhci_plat_quirks);
}

static const struct hc_driver xhci_plat_xhci_driver = {
	.description =		"xhci-hcd",
	.product_desc =		"xHCI Host Controller",
	.hcd_priv_size =	sizeof(struct xhci_hcd *),

	.irq =			xhci_irq,
	.flags =		HCD_MEMORY | HCD_USB3 | HCD_SHARED,

	.reset =		xhci_plat_setup,
	.start =		xhci_run,
	.stop =			xhci_stop,
	.shutdown =		xhci_shutdown,

	.urb_enqueue =		xhci_urb_enqueue,
	.urb_dequeue =		xhci_urb_dequeue,
	.alloc_dev =		xhci_alloc_dev,
	.free_dev =		xhci_free_dev,
	.alloc_streams =	xhci_alloc_streams,
	.free_streams =		xhci_free_streams,
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
#if defined(MY_ABC_HERE) && defined(CONFIG_PM)
	.bus_suspend =		comcerto_xhci_bus_suspend,
	.bus_resume =		comcerto_xhci_bus_resume,
#endif
};

static int xhci_plat_probe(struct platform_device *pdev)
{
	const struct hc_driver	*driver;
	struct xhci_hcd		*xhci;
	struct resource         *res;
	struct usb_hcd		*hcd;
	int			ret;
	int			irq;

	printk(KERN_INFO "## %s\n",__func__);

	if (usb_disabled())
		return -ENODEV;

#if defined(MY_ABC_HERE)
	 
	comcerto_start_xhci();
#endif

	driver = &xhci_plat_xhci_driver;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len,
				driver->description)) {
		dev_dbg(&pdev->dev, "controller already in use\n");
		ret = -EBUSY;
		goto put_hcd;
	}

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
#else
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
#endif
	if (!hcd->regs) {
		dev_dbg(&pdev->dev, "error mapping memory\n");
		ret = -EFAULT;
		goto release_mem_region;
	}

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#else
#if 1
	writel(readl(hcd->regs + 0xc200) & 0x7FFFFFFF, hcd->regs + 0xc200);
	writel(readl(hcd->regs + 0xc2c0) & 0x7FFFFFFF, hcd->regs + 0xc2c0);
	writel(0x5Dc11000, hcd->regs + 0xc110);
#endif

#if defined(MY_ABC_HERE) && defined(CONFIG_SYNO_C2K_USB_VBUS_CONTROL)
	 
	writel((readl(COMCERTO_GPIO_OUTPUT_REG) | (0x1 << 15)), COMCERTO_GPIO_OUTPUT_REG);
#endif
#endif

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto unmap_registers;

	hcd = dev_get_drvdata(&pdev->dev);
	xhci = hcd_to_xhci(hcd);
	xhci->shared_hcd = usb_create_shared_hcd(driver, &pdev->dev,
			dev_name(&pdev->dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto dealloc_usb2_hcd;
	}

	*((struct xhci_hcd **) xhci->shared_hcd->hcd_priv) = xhci;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;

	return 0;

put_usb3_hcd:
	usb_put_hcd(xhci->shared_hcd);

dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

unmap_registers:
	iounmap(hcd->regs);

release_mem_region:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);

put_hcd:
	usb_put_hcd(hcd);

	return ret;
}

static int xhci_plat_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	usb_remove_hcd(xhci->shared_hcd);
	usb_put_hcd(xhci->shared_hcd);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	usb_put_hcd(hcd);

#if defined(MY_ABC_HERE)
	 
	comcerto_stop_xhci();
#endif

	kfree(xhci);

	return 0;
}

static struct platform_driver usb_xhci_driver = {
	.probe	= xhci_plat_probe,
	.remove	= xhci_plat_remove,
	.driver	= {
		.name = "xhci-hcd",
	},
};
MODULE_ALIAS("platform:xhci-hcd");

int xhci_register_plat(void)
{
	return platform_driver_register(&usb_xhci_driver);
}

void xhci_unregister_plat(void)
{
	platform_driver_unregister(&usb_xhci_driver);
}
