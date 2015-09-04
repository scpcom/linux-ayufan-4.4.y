/**
 * dwc3-ls1024a.c Support for dwc3 platform devices on Freescale QorIQ LS1024A
 *
 * This is a small driver for the dwc3 to provide the glue logic
 * to configure the controller.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

struct ls1024a_dwc {
	struct device *dev;
	struct reset_control *rstc_utmi;
	struct reset_control *rstc_axi;
	struct clk *clk;
};

static int ls1024a_dwc_probe(struct platform_device *pdev)
{
	struct ls1024a_dwc *dwc3_data;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	int ret;

	dwc3_data = devm_kzalloc(dev, sizeof(*dwc3_data), GFP_KERNEL);
	if (!dwc3_data)
		return -ENOMEM;

	dwc3_data->dev = dev;

	dwc3_data->clk = devm_clk_get(dev, "usb");
	if (IS_ERR(dwc3_data->clk)) {
		ret = PTR_ERR(dwc3_data->clk);
		goto undo_platform_dev_alloc;
	}
	clk_prepare_enable(dwc3_data->clk);

	dwc3_data->rstc_utmi = devm_reset_control_get(dev, "utmi");
	if (IS_ERR(dwc3_data->rstc_utmi)) {
		ret = PTR_ERR(dwc3_data->rstc_utmi);
		goto undo_clk;
	}

	if (reset_control_status(dwc3_data->rstc_utmi)) {
		reset_control_deassert(dwc3_data->rstc_utmi);
		udelay(1000);
	}

	dwc3_data->rstc_axi = devm_reset_control_get(dev, "axi");
	if (IS_ERR(dwc3_data->rstc_axi)) {
		ret = PTR_ERR(dwc3_data->rstc_axi);
		goto undo_utmi;
	}

	if (reset_control_status(dwc3_data->rstc_axi)) {
		reset_control_deassert(dwc3_data->rstc_axi);
		udelay(1000);
	}

	/* Allocate and initialize the core */
	ret = of_platform_populate(node, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "failed to add dwc3 core\n");
		goto undo_axi;
	}

	platform_set_drvdata(pdev, dwc3_data);
	return 0;

undo_axi:
	reset_control_assert(dwc3_data->rstc_axi);
undo_utmi:
	reset_control_assert(dwc3_data->rstc_utmi);
undo_clk:
	clk_disable_unprepare(dwc3_data->clk);
undo_platform_dev_alloc:
	platform_device_put(pdev);
	return ret;
}

static const struct of_device_id ls1024a_dwc_match[] = {
	{ .compatible = "fsl,ls1024a-dwc3" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, ls1024a_dwc_match);

static struct platform_driver ls1024a_dwc_driver = {
	.probe = ls1024a_dwc_probe,
	.driver = {
		.name = "usb-ls1024a-dwc3",
		.of_match_table = ls1024a_dwc_match,
	},
};

module_platform_driver(ls1024a_dwc_driver);

MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_DESCRIPTION("DesignWare USB3 Freescale QorIQ LS1024a Glue Layer");
MODULE_LICENSE("GPL v2");
