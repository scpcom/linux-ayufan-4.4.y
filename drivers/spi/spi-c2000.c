/*
 * Memory-mapped interface driver for DW SPI Core
 * Author: Satendra Pratap <satendra.pratap@gmail.com>
 * Copyright (c) 2012, Mindspeed Technologies.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <mach/reset.h>

#include "spi-dw.h"

#define DRIVER_NAME "comcerto_spi"

extern int dw_spi_dma_init(struct dw_spi *dws);

#define	CLK_NAME	10
struct spi_controller_pdata {
        int use_dma;
        int num_chipselects;
	int bus_num;
	u32 max_freq;
	char clk_name[CLK_NAME];
};

struct dw_spi_c2000 {
	struct dw_spi  dws;
};

static int __devinit dw_spi_c2000_probe(struct platform_device *pdev)
{
	struct dw_spi_c2000 *dwsmmio;
	struct dw_spi *dws;
	struct resource *mem, *ioarea;
	struct spi_controller_pdata *pdata;
	struct clk *clk_spi;
	int ret;

	pdata = pdev->dev.platform_data;
	if(!pdata)
	{
		ret = -EINVAL;
		goto err_end;
	}

	dwsmmio = kzalloc(sizeof(struct dw_spi_c2000), GFP_KERNEL);
	if (!dwsmmio) {
		ret = -ENOMEM;
		goto err_end;
	}

	dws = &dwsmmio->dws;

	clk_spi = clk_get(NULL,pdata->clk_name);
	if (IS_ERR(clk_spi)) {
		ret = PTR_ERR(clk_spi);
		pr_err("%s:Unable to obtain spi clock: %d\n",\
			__func__, ret);
		goto err_kfree;
	}
	dws->clk_spi=clk_spi;

	dws->max_freq = clk_get_rate(dws->clk_spi);

	printk ("%s:Initializing SPI Controller : use_dma=%d CLK(%s)=%d Hz\n", __func__, \
			pdata->use_dma, pdata->clk_name, dws->max_freq);

	if(memcmp(pdata->clk_name, "DUS", 3))
		c2000_block_reset(COMPONENT_AXI_LEGACY_SPI, 0);
	else	
		c2000_block_reset(COMPONENT_AXI_FAST_SPI, 0);
	
	if(pdata->use_dma) /* DMA */
	{
		ret = dw_spi_dma_init(dws);
		if (ret)
			goto err_clk;
	}

	dws->bus_num = pdata->bus_num;
	dws->num_cs = pdata->num_chipselects;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		ret = -EINVAL;
		goto err_clk;
	}

	ioarea = request_mem_region(mem->start, resource_size(mem),
			pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "SPI region already claimed\n");
		ret = -EBUSY;
		goto err_clk;
	}

	dws->regs = ioremap_nocache(mem->start, resource_size(mem));
	if (!dws->regs) {
		dev_err(&pdev->dev, "SPI region already mapped\n");
		ret = -ENOMEM;
		goto err_release_reg;
	}

	dws->irq = platform_get_irq(pdev, 0);
	if (dws->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		ret = dws->irq; /* -ENXIO */
		goto err_unmap;
	}

	dws->parent_dev = &pdev->dev;

	ret = dw_spi_add_host(dws);
	if (ret)
		goto err_unmap;

	platform_set_drvdata(pdev, dwsmmio);
	return 0;

err_unmap:
	iounmap(dws->regs);
err_release_reg:
	release_mem_region(mem->start, resource_size(mem));
err_clk:
	if(memcmp(pdata->clk_name, "DUS", 3))
		c2000_block_reset(COMPONENT_AXI_LEGACY_SPI, 1);
	else	
		c2000_block_reset(COMPONENT_AXI_FAST_SPI, 1);
	
	clk_put(dws->clk_spi);
err_kfree:
	kfree(dwsmmio);
err_end:
	return ret;
}

static int __devexit dw_spi_c2000_remove(struct platform_device *pdev)
{
	struct dw_spi_c2000 *dwsmmio = platform_get_drvdata(pdev);
	struct spi_controller_pdata *pdata;
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);

	dw_spi_remove_host(&dwsmmio->dws);
	iounmap(dwsmmio->dws.regs);

	clk_put(dwsmmio->dws.clk_spi);
	kfree(dwsmmio);

	pdata = pdev->dev.platform_data;
	if(!pdata)
	{
		return -EINVAL;
	}

	if(memcmp(pdata->clk_name, "DUS", 3))
		c2000_block_reset(COMPONENT_AXI_LEGACY_SPI, 1);
	else	
		c2000_block_reset(COMPONENT_AXI_FAST_SPI, 1);
	
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, resource_size(mem));

	return 0;
}

#ifdef CONFIG_PM
static int c2000_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dw_spi_c2000 *dwsmmio = platform_get_drvdata(pdev);
	struct dw_spi *dws = &dwsmmio->dws;

	return dw_spi_suspend_host(dws);
}

static int c2000_spi_resume(struct platform_device *pdev)
{
	struct dw_spi_c2000 *dwsmmio = platform_get_drvdata(pdev);
	struct dw_spi *dws = &dwsmmio->dws;

	return dw_spi_resume_host(dws);
}
#endif

static struct platform_driver dw_spi_c2000_driver = {
	.probe		= dw_spi_c2000_probe,
	.remove		= __devexit_p(dw_spi_c2000_remove),
#ifdef CONFIG_PM
	.suspend        = c2000_spi_suspend,
	.resume         = c2000_spi_resume,
#endif
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};
module_platform_driver(dw_spi_c2000_driver);

MODULE_AUTHOR("Satendra Pratap <satendra.pratap@gmail.com>");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
