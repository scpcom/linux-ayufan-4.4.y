#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/platform_device.h>
#include <linux/module.h>
#if defined(MY_ABC_HERE)
#include <linux/usb/phy.h>
#endif  
#include <linux/slab.h>
#if defined (MY_DEF_HERE)
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/usb/xhci_pdriver.h>
#endif  
#if defined(MY_ABC_HERE)
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#endif  

#include "xhci.h"
#if defined(MY_ABC_HERE)
#include "xhci-mvebu.h"
#endif  

static void xhci_plat_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	 
	xhci->quirks |= XHCI_PLAT;
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
#if defined (MY_DEF_HERE)
	.enable_device =	xhci_enable_device,
#endif  
	.update_hub_device =	xhci_update_hub_device,
	.reset_device =		xhci_discover_or_reset_device,

	.get_frame_number =	xhci_get_frame,

	.hub_control =		xhci_hub_control,
	.hub_status_data =	xhci_hub_status_data,
	.bus_suspend =		xhci_bus_suspend,
	.bus_resume =		xhci_bus_resume,
#if defined (MY_DEF_HERE)

	.enable_usb3_lpm_timeout =	xhci_enable_usb3_lpm_timeout,
	.disable_usb3_lpm_timeout =	xhci_disable_usb3_lpm_timeout,
#endif  
};

#if defined(MY_ABC_HERE)
int common_xhci_plat_probe(struct platform_device *pdev,
			   void *priv)
#else  
static int xhci_plat_probe(struct platform_device *pdev)
#endif  
{
#if defined (MY_DEF_HERE)
	struct device_node	*node = pdev->dev.of_node;
	struct usb_xhci_pdata	*pdata = dev_get_platdata(&pdev->dev);
#endif  
#if defined (MY_ABC_HERE)
	struct device_node	*node = pdev->dev.of_node;
	u32 vbus_gpio_pin = 0;
#endif  
	const struct hc_driver	*driver;
	struct xhci_hcd		*xhci;
	struct resource         *res;
	struct usb_hcd		*hcd;
	int			ret;
	int			irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_plat_xhci_driver;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

#if defined (MY_DEF_HERE) || defined(MY_ABC_HERE)
	 
	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	else
		dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
#endif  

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

	hcd->regs = ioremap_nocache(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_dbg(&pdev->dev, "error mapping memory\n");
		ret = -EFAULT;
		goto release_mem_region;
	}

#if defined (MY_ABC_HERE)
	if (node) {
		if (of_property_read_bool(node, "power-control-capable")) {
			hcd->power_control_support = 1;
		} else {
			hcd->power_control_support = 0;
		}
		if (of_property_read_bool(node, "vbus-gpio")) {
			of_property_read_u32(node, "vbus-gpio", &vbus_gpio_pin);
			 
			hcd->vbus_gpio_pin = vbus_gpio_pin;
		} else {
			hcd->vbus_gpio_pin = -1;
			dev_warn(&pdev->dev, "failed to get Vbus gpio\n");
		}
	}
#endif  

#if defined(MY_ABC_HERE)
	 
	set_bit(7, hcd->regs + 0x380c);
#endif  
#if defined (MY_ABC_HERE)
	dev_info(&pdev->dev, "USB2 Vbus gpio %d\n", hcd->vbus_gpio_pin);
	dev_info(&pdev->dev, "power control %s\n", hcd->power_control_support ? "enabled" : "disabled");
#endif  
	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto unmap_registers;

#if defined (MY_DEF_HERE)
	hcd = platform_get_drvdata(pdev);
#else  
	hcd = dev_get_drvdata(&pdev->dev);
#endif  
	xhci = hcd_to_xhci(hcd);
#if defined(MY_ABC_HERE)
	xhci->priv = priv;
#endif  
	xhci->shared_hcd = usb_create_shared_hcd(driver, &pdev->dev,
			dev_name(&pdev->dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto dealloc_usb2_hcd;
	}

#if defined (MY_DEF_HERE)
	if ((node && of_property_read_bool(node, "usb3-lpm-capable")) ||
			(pdata && pdata->usb3_lpm_capable))
		xhci->quirks |= XHCI_LPM_SUPPORT;
#endif  
#if defined(MY_ABC_HERE)
	hcd->phy = devm_usb_get_phy_by_phandle(&pdev->dev, "usb-phy", 0);
	if (IS_ERR(hcd->phy)) {
		ret = PTR_ERR(hcd->phy);
		if (ret == -EPROBE_DEFER)
			goto put_usb3_hcd;
		hcd->phy = NULL;
	} else {
		ret = usb_phy_init(hcd->phy);
		if (ret)
			goto put_usb3_hcd;
	}
#endif  
#if defined (MY_ABC_HERE)
	xhci->shared_hcd->vbus_gpio_pin = hcd->vbus_gpio_pin;
	xhci->shared_hcd->power_control_support = hcd->power_control_support;
	dev_info(&pdev->dev, "USB3 Vbus gpio %d\n", xhci->shared_hcd->vbus_gpio_pin);
	dev_info(&pdev->dev, "power control %s\n", hcd->power_control_support ? "enabled" : "disabled");
#endif  
	 
	*((struct xhci_hcd **) xhci->shared_hcd->hcd_priv) = xhci;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
#if defined(MY_ABC_HERE)
		goto disable_usb_phy;
#else  
		goto put_usb3_hcd;
#endif  

	return 0;

#if defined(MY_ABC_HERE)
disable_usb_phy:
	usb_phy_shutdown(hcd->phy);
#endif  

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

#if defined(MY_ABC_HERE)
int common_xhci_plat_remove(struct platform_device *dev)
#else  
static int xhci_plat_remove(struct platform_device *dev)
#endif  
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	usb_remove_hcd(xhci->shared_hcd);
#if defined(MY_ABC_HERE)
	if (hcd->phy)
		usb_phy_shutdown(hcd->phy);
#endif  
	usb_put_hcd(xhci->shared_hcd);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	kfree(xhci);

	return 0;
}
#if defined (MY_DEF_HERE)
#ifdef MY_DEF_HERE
#ifdef CONFIG_PM_SLEEP
static int xhci_plat_suspend(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_suspend(xhci);
}

static int xhci_plat_resume(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_resume(xhci, 0);
}
#endif  

#ifdef CONFIG_PM
static const struct dev_pm_ops xhci_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_plat_suspend, xhci_plat_resume)
};
#define DEV_PM_OPS	(&xhci_plat_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif  
#else  
#ifdef CONFIG_PM
static int xhci_plat_suspend(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_suspend(xhci);
}

static int xhci_plat_resume(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_resume(xhci, 0);
}

static const struct dev_pm_ops xhci_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_plat_suspend, xhci_plat_resume)
};
#define DEV_PM_OPS	(&xhci_plat_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif  
#endif  

#ifdef CONFIG_OF
static const struct of_device_id usb_xhci_of_match[] = {
	{ .compatible = "xhci-platform" },
	{ },
};
MODULE_DEVICE_TABLE(of, usb_xhci_of_match);
#endif
#endif  

#if defined(MY_ABC_HERE)
static int default_xhci_plat_probe(struct platform_device *pdev)
{
	return common_xhci_plat_probe(pdev, NULL);
}

static int default_xhci_plat_remove(struct platform_device *pdev)
{
	return common_xhci_plat_remove(pdev);
}

struct xhci_plat_ops {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*resume)(struct device *);
};

static struct xhci_plat_ops xhci_plat_default = {
	.probe = default_xhci_plat_probe,
	.remove =  default_xhci_plat_remove,
};

#ifdef CONFIG_OF
struct xhci_plat_ops xhci_plat_mvebu = {
	.probe =  xhci_mvebu_probe,
	.remove =  xhci_mvebu_remove,
	.resume =  xhci_mvebu_resume,
};

static const struct of_device_id usb_xhci_of_match[] = {
	{
		.compatible = "xhci-platform",
		.data = (void *) &xhci_plat_default,
	},
	{
		.compatible = "marvell,xhci-armada-375",
		.data = (void *) &xhci_plat_mvebu,
	},
	{
		.compatible = "marvell,xhci-armada-380",
		.data = (void *) &xhci_plat_mvebu,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, usb_xhci_of_match);
#endif

static int xhci_plat_probe(struct platform_device *pdev)
{
	const struct xhci_plat_ops *plat_of = &xhci_plat_default;

	if (pdev->dev.of_node) {
		const struct of_device_id *match =
			of_match_device(usb_xhci_of_match, &pdev->dev);
		if (!match)
			return -ENODEV;
		plat_of = match->data;
	}

	if (!plat_of || !plat_of->probe)
		return  -ENODEV;

	return plat_of->probe(pdev);
}

static int xhci_plat_remove(struct platform_device *pdev)
{
	const struct xhci_plat_ops *plat_of = &xhci_plat_default;

	if (pdev->dev.of_node) {
		const struct of_device_id *match =
			of_match_device(usb_xhci_of_match, &pdev->dev);
		if (!match)
			return -ENODEV;
		plat_of = match->data;
	}

	if (!plat_of || !plat_of->remove)
		return  -ENODEV;

	return plat_of->remove(pdev);
}

#ifdef CONFIG_PM
static int xhci_plat_suspend(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_suspend(xhci);
}

static int xhci_plat_resume(struct device *dev)
{
	const struct xhci_plat_ops *plat_of = &xhci_plat_default;
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	if (dev->of_node) {
		const struct of_device_id *match =
			of_match_device(usb_xhci_of_match, dev);
		if (!match)
			return -ENODEV;
		plat_of = match->data;
	}

	if (!plat_of)
		return -ENODEV;

	if (plat_of->resume)
		plat_of->resume(dev);

	return xhci_resume(xhci, 0);
}

static const struct dev_pm_ops xhci_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_plat_suspend, xhci_plat_resume)
};
#define DEV_PM_OPS	(&xhci_plat_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif  
#endif  

static struct platform_driver usb_xhci_driver = {
#if defined(MY_ABC_HERE)
	.probe		= xhci_plat_probe,
	.remove		= xhci_plat_remove,
	.shutdown	= xhci_plat_remove,
#else  
	.probe	= xhci_plat_probe,
	.remove	= xhci_plat_remove,
#endif  
	.driver	= {
		.name = "xhci-hcd",
#if defined (MY_DEF_HERE) || defined(MY_ABC_HERE)
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(usb_xhci_of_match),
#endif  
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
