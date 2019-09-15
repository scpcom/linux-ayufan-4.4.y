/*
 * Comcerto 2K AHCI SATA Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This is based on fsl ahci_platform.c patch by Mindspeed Technologies, Inc.
 *
 * In v4.4-rc1 at ecfb45, ahci_qoriq.c driver was introduced with support
 * for ls1021a, ls1043a, ls2085a, however that driver does not manipulate
 * clocks, because devicetree nodes are handling that. Sequoia is running
 * an older bootloader which does not support passing devicetree structure
 * to the kernel at boot time, so we using this driver instead.
 *
 * Power management routines are not migrated from original ahci_platform
 * driver as it is not clear how to test these.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include "ahci.h"

/* Set up SATA device clocks
 *
 * This depends on arch/mach-comcerto/comcerto-2000.c:comcerto_ahci_init() setting
 * setting up the controller
 */
static int comcerto_sata_init(struct device *dev)
{
	struct clk *sata_oob_clk;	/* Core clock */
	struct clk *sata_pmu_clk;	/* PMU alive clock */
	struct clk *sata_clk;	/* Sata AXI ref clock */
	int rc;

	sata_clk = clk_get(NULL, "sata");
	if (IS_ERR(sata_clk)) {
		dev_err(dev, "%s: Unable to obtain SATA(AXI) clock: %ld\n",
			__func__, PTR_ERR(sata_clk));
		return PTR_ERR(sata_clk);
	}

	rc = clk_enable(sata_clk);
	if (rc) {
		dev_err(dev, "%s: SATA(AXI) clock enable failed \n", __func__);
		return rc;
	}

	sata_oob_clk = clk_get(NULL, "sata_oob");
	if (IS_ERR(sata_oob_clk)) {
		dev_err(dev, "%s: Unable to obtain SATA_OOB clock: %ld\n",
			__func__, PTR_ERR(sata_oob_clk));
		return PTR_ERR(sata_oob_clk);
	}

	sata_pmu_clk = clk_get(NULL, "sata_pmu");
	if (IS_ERR(sata_pmu_clk)) {
		dev_err(dev, "%s: Unable to obtain SATA_PMU clock: %ld\n",
			__func__, PTR_ERR(sata_pmu_clk));
		return PTR_ERR(sata_pmu_clk);
	}

	rc = clk_enable(sata_oob_clk);
	if (rc) {
		dev_err(dev, "%s: SATA_OOB clock enable failed \n", __func__);
		return rc;
	}

	rc = clk_enable(sata_pmu_clk);
	if (rc) {
		dev_err(dev, "%s: SATA_PMU clock enable failed \n", __func__);
		return rc;
	}

	/* Set the SATA PMU clock to 30 MHZ and OOB clock to 125MHZ */
	clk_set_rate(sata_oob_clk, 125000000);
	clk_set_rate(sata_pmu_clk, 30000000);

	return 0;
}

/* AHCI Hard Reset for Comcerto 2000
 *
 * This is copied from libahci.c which was modified by Mindspeed to support
 * what later would be known as errata A-009042 in ahci_qoriq.c.
 */
static int ahci_comcerto_hardreset(struct ata_link *link, unsigned int *class,
				   unsigned long deadline)
{
	const unsigned long *timing = sata_ehc_deb_timing(&link->eh_context);
	struct ata_port *ap = link->ap;
	struct ahci_port_priv *pp = ap->private_data;
	struct ahci_host_priv *hpriv = ap->host->private_data;
	void __iomem *port_mmio = ahci_port_base(link->ap);
	u8 *d2h_fis = pp->rx_fis + RX_FIS_D2H_REG;
	struct ata_taskfile tf;
	bool online;
	int rc;
	u32 save_cmd, tmp;

	ahci_stop_engine(ap);

	save_cmd = readl(port_mmio + PORT_CMD);

	/* clear D2H reception area to properly wait for D2H FIS */
	ata_tf_init(link->device, &tf);
	tf.command = ATA_BUSY;
	ata_tf_to_fis(&tf, 0, 0, d2h_fis);

	rc = sata_link_hardreset(link, timing, deadline, &online,
				 ahci_check_ready);

	/* Revert the saved cmd value, if not match with original */
	tmp = readl(port_mmio + PORT_CMD);
	if (tmp != save_cmd)
		writel(save_cmd, port_mmio + PORT_CMD);

	hpriv->start_engine(ap);

	if (online)
		*class = ahci_dev_classify(ap);

	return rc;
}

static struct ata_port_operations ahci_comcerto_ops = {
	.inherits = &ahci_ops,
	.hardreset = ahci_comcerto_hardreset,
};

static const struct ata_port_info ahci_comcerto_port_info = {
	.flags = AHCI_FLAG_COMMON | ATA_FLAG_NCQ,
	.pio_mask = ATA_PIO4,
	.udma_mask = ATA_UDMA6,
	.port_ops = &ahci_comcerto_ops,
};

static int ahci_comcerto_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_host_priv *hpriv;
	int rc;

	hpriv = ahci_platform_get_resources(pdev);
	if (IS_ERR(hpriv)) {
		dev_err(dev, "%s: ahci_platform_get_resources failed\n",
			__func__);
		return PTR_ERR(hpriv);
	}

	rc = ahci_platform_enable_resources(hpriv);
	if (rc) {
		dev_err(dev, "%s: ahci_platform_enable_resources failed: %d\n",
			__func__, rc);
		return rc;
	}

	rc = comcerto_sata_init(dev);
	if (rc) {
		dev_err(dev, "%s: comcert_sata_init() failed: %d\n", __func__,
			rc);
		goto disable_resources;
	}

	rc = ahci_platform_init_host(pdev, hpriv, &ahci_comcerto_port_info);
	if (rc) {
		dev_err(dev, "%s: ahci_platform_init_host() failed: %d\n",
			__func__, rc);
		goto disable_resources;
	}

	return 0;

disable_resources:
	ahci_platform_disable_resources(hpriv);
	return rc;
}

static struct platform_driver ahci_comcerto_driver = {
	.probe = ahci_comcerto_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		   .name = "ahci_comcerto",
		   },
};

module_platform_driver(ahci_comcerto_driver);

MODULE_DESCRIPTION("Comcerto AHCI SATA platform driver");
MODULE_AUTHOR("Roman Yepishev <roman.yepishev@gmail.com>");
MODULE_LICENSE("GPL");
