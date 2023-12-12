/*
 * AHCI SATA platform driver
 *
 * Copyright 2004-2005  Red Hat, Inc.
 *   Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2010  MontaVista Software, LLC.
 *   Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include <linux/clk.h>
#include <mach/reset.h>
#include "ahci.h"

#ifdef CONFIG_ARCH_M86XXX 
/* SATA Clocks */
static struct clk *sata_oob_clk; /* Core clock */
static struct clk *sata_pmu_clk; /* PMU alive clock */
static struct clk *sata_clk;	/* Sata AXI ref clock */
#endif 

enum ahci_type {
	AHCI,		/* standard platform ahci */
	IMX53_AHCI,	/* ahci on i.mx53 */
};

static struct platform_device_id ahci_devtype[] = {
	{
		.name = "ahci",
		.driver_data = AHCI,
	}, {
		.name = "imx53-ahci",
		.driver_data = IMX53_AHCI,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(platform, ahci_devtype);


static const struct ata_port_info ahci_port_info[] = {
	/* by features */
	[AHCI] = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_ops,
	},
	[IMX53_AHCI] = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_pmp_retry_srst_ops,
	},
};

#ifdef CONFIG_PM
static int sata_ahci_platform_suspend(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	int ret=0;
        if (host)
		ret = ata_host_suspend(host, PMSG_SUSPEND);

#ifdef CONFIG_ARCH_M86XXX
	if (!ret) /* sucessfully done the host suspend */
	{
		/* No do the clock disable PMU,OOB,AXI here */
		clk_disable(sata_clk);
		clk_disable(sata_oob_clk);
		clk_disable(sata_pmu_clk);
	}
#endif
	
        return ret;
}

static int sata_ahci_platform_resume(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);

#ifdef CONFIG_ARCH_M86XXX
	/* Do the  clock enable here  PMU,OOB,AXI */
	clk_enable(sata_clk);
	clk_enable(sata_oob_clk);
	clk_enable(sata_pmu_clk);
#endif

        if (host) 
		ata_host_resume(host);

	return 0;
}
#else
#define sata_ahci_platform_suspend NULL
#define sata_ahci_platform_resume NULL
#endif

static int ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	const struct platform_device_id *id = platform_get_device_id(pdev);
	struct ata_port_info pi = ahci_port_info[id ? id->driver_data : 0];
	struct ahci_host_priv *hpriv;
	unsigned long hflags = 0;
	int rc;

#ifdef CONFIG_ARCH_M86XXX
	/* Get the Reference and Enable  the SATA clocks here */

	sata_clk = clk_get(NULL,"sata");
	/* Error Handling , if no SATA(AXI) clock reference: return error */
	if (IS_ERR(sata_clk)) {
		pr_err("%s: Unable to obtain SATA(AXI) clock: %ld\n",__func__,PTR_ERR(sata_clk));
		return PTR_ERR(sata_clk);
 	}

	/*Enable the SATA(AXI) clock here */
        rc = clk_enable(sata_clk);
	if (rc){
		pr_err("%s: SATA(AXI) clock enable failed \n",__func__);
                return rc;
	}
	sata_oob_clk = clk_get(NULL,"sata_oob");
	/* Error Handling , if no SATA_OOB clock reference: return error */
	if (IS_ERR(sata_oob_clk)) {
		pr_err("%s: Unable to obtain SATA_OOB clock: %ld\n",__func__,PTR_ERR(sata_oob_clk));
		return PTR_ERR(sata_oob_clk);
 	}

	sata_pmu_clk = clk_get(NULL,"sata_pmu");
	/* Error Handling , if no SATA_PMU clock reference: return error */
	if (IS_ERR(sata_pmu_clk)) {
		pr_err("%s: Unable to obtain SATA_PMU clock: %ld\n",__func__,PTR_ERR(sata_pmu_clk));
		return PTR_ERR(sata_pmu_clk);
	}
	/*Enable the SATA(PMU and OOB) clocks here */
        rc = clk_enable(sata_oob_clk);
	if (rc){
		pr_err("%s: SATA_OOB clock enable failed \n",__func__);
                return rc;
	}

        rc = clk_enable(sata_pmu_clk);
	if (rc){
		pr_err("%s: SATA_PMU clock enable failed \n",__func__);
		return rc;
	}

	/* Set the SATA PMU clock to 30 MHZ and OOB clock to 125MHZ */
	clk_set_rate(sata_oob_clk,125000000);
	clk_set_rate(sata_pmu_clk,30000000);
	
#endif

	hpriv = ahci_platform_get_resources(pdev);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);

	rc = ahci_platform_enable_resources(hpriv);
	if (rc)
		return rc;

	if (pdata && pdata->ata_port_info)
		pi = *pdata->ata_port_info;

	hflags |= (unsigned long)pi.private_data;

	/*
	 * Some platforms might need to prepare for mmio region access,
	 * which could be done in the following init call. So, the mmio
	 * region shouldn't be accessed before init (if provided) has
	 * returned successfully.
	 */
	if (pdata && pdata->init) {
		rc = pdata->init(dev, hpriv->mmio);
		if (rc)
			goto disable_resources;
	}

	if (of_device_is_compatible(dev->of_node, "hisilicon,hisi-ahci"))
		hflags |= AHCI_HFLAG_NO_FBS | AHCI_HFLAG_NO_NCQ;

	rc = ahci_platform_init_host(pdev, hpriv, &pi,
				     hflags,
				     pdata ? pdata->force_port_map : 0,
				     pdata ? pdata->mask_port_map  : 0);
	if (rc)
		goto pdata_exit;

	return 0;
pdata_exit:
	if (pdata && pdata->exit)
		pdata->exit(dev);
disable_resources:
	ahci_platform_disable_resources(hpriv);
	return rc;
}

static int ahci_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	struct ata_host *host = dev_get_drvdata(dev);

	ata_host_detach(host);

	if (pdata && pdata->exit)
		pdata->exit(dev);
#ifdef CONFIG_ARCH_M86XXX
	/* Disbale the SATA clocks Here */
	clk_disable(sata_clk);
	clk_put(sata_clk);
	clk_disable(sata_oob_clk);
	clk_put(sata_oob_clk);
	clk_disable(sata_pmu_clk);
	clk_put(sata_pmu_clk);
	/*Putting  SATA in reset state 
	 * Sata axi clock domain in reset state
	 * Serdes 1/2 in reset state, this depends upon PCIE1 and SGMII 
         * sata 0/1 serdes controller in reset state
	*/
	c2000_block_reset(COMPONENT_AXI_SATA,1);

	c2000_block_reset(COMPONENT_SERDES1,1);
	c2000_block_reset(COMPONENT_SERDES_SATA0,1);

	c2000_block_reset(COMPONENT_SERDES2,1);
	c2000_block_reset(COMPONENT_SERDES_SATA1,1);
#endif

	return 0;
}

static SIMPLE_DEV_PM_OPS(ahci_pm_ops, sata_ahci_platform_suspend,
			 sata_ahci_platform_resume);

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "snps,spear-ahci", },
	{ .compatible = "snps,exynos5440-ahci", },
	{ .compatible = "ibm,476gtr-ahci", },
	{ .compatible = "snps,dwc-ahci", },
	{ .compatible = "hisilicon,hisi-ahci", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static struct platform_driver ahci_driver = {
	.probe = ahci_probe,
	.remove = ahci_remove,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = "ahci",
		.owner = THIS_MODULE,
		.of_match_table = ahci_of_match,
		.pm = &ahci_pm_ops,
	},
	.id_table = ahci_devtype,
};
module_platform_driver(ahci_driver);

MODULE_DESCRIPTION("AHCI SATA platform driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci");
