 
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include <linux/ata_platform.h>
#include <linux/mbus.h>
#include "ahci.h"

#define AHCI_WINDOW_CTRL(win)	(0x60 + ((win)<<4))
#define AHCI_WINDOW_BASE(win)	(0x64 + ((win)<<4))
#define AHCI_WINDOW_SIZE(win)	(0x68 + ((win)<<4))

#ifdef CONFIG_SYNO_ARMADA_V2
#define VENDOR_SPECIFIC_0_ADDR  0xa0
#define VENDOR_SPECIFIC_0_DATA  0xa4
#endif

static void ahci_mv_windows_config(struct ahci_host_priv *hpriv,
				 struct mbus_dram_target_info *dram)
{
	int i;

	for (i = 0; i < 4; i++) {
		writel(0, hpriv->mmio + AHCI_WINDOW_CTRL(i));
		writel(0, hpriv->mmio + AHCI_WINDOW_BASE(i));
		writel(0, hpriv->mmio + AHCI_WINDOW_SIZE(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->mbus_attr << 8) |
			(dram->mbus_dram_target_id << 4) | 1,
			hpriv->mmio + AHCI_WINDOW_CTRL(i));
		writel(cs->base, hpriv->mmio + AHCI_WINDOW_BASE(i));
		writel(((cs->size - 1) & 0xffff0000),
			hpriv->mmio + AHCI_WINDOW_SIZE(i));
	}
}

static const struct ata_port_info ahci_mv_port_info = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_ops,
};

static struct scsi_host_template ahci_mv_platform_sht = {
	AHCI_SHT("ahci_mv_platform"),
};

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "marvell,ahci-sata", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static int ahci_mv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mv_sata_platform_data *pdata = dev_get_platdata(dev);
	struct ata_port_info pi = ahci_mv_port_info;
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "no mmio space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "no irq\n");
		return -EINVAL;
	}

	hpriv = devm_kzalloc(dev, sizeof(*hpriv), GFP_KERNEL);
	if (!hpriv) {
		dev_err(dev, "can't alloc ahci_host_priv\n");
		return -ENOMEM;
	}

	hpriv->flags |= (unsigned long)pi.private_data;

	hpriv->mmio = devm_ioremap(dev, mem->start, resource_size(mem));
	if (!hpriv->mmio) {
		dev_err(dev, "can't map %pR\n", mem);
		return -ENOMEM;
	}

	if (pdata->dram != NULL)
		ahci_mv_windows_config(hpriv, pdata->dram);

	ahci_save_initial_config(dev, hpriv, 0, 0);

	if (hpriv->cap & HOST_CAP_NCQ)
		pi.flags |= ATA_FLAG_NCQ;

	if (hpriv->cap & HOST_CAP_PMP)
		pi.flags |= ATA_FLAG_PMP;

	ahci_set_em_messages(hpriv, &pi);

	n_ports = max(ahci_nr_ports(hpriv->cap), fls(hpriv->port_map));

	host = ata_host_alloc_pinfo(dev, ppi, n_ports);
	if (!host) {
		rc = -ENOMEM;
		goto err0;
	}

	host->private_data = hpriv;

	if (!(hpriv->cap & HOST_CAP_SSS) || ahci_ignore_sss)
		host->flags |= ATA_HOST_PARALLEL_SCAN;
	else
		printk(KERN_INFO "ahci: SSS flag set, parallel bus scan disabled\n");

	if (pi.flags & ATA_FLAG_EM)
		ahci_reset_em(host);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		ata_port_desc(ap, "mmio %pR", mem);
		ata_port_desc(ap, "port 0x%x", 0x100 + ap->port_no * 0x80);

		if (ap->flags & ATA_FLAG_EM)
			ap->em_message_type = hpriv->em_msg_type;

		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

#ifdef CONFIG_SYNO_ARMADA_V2
	 
	writel(0x4, hpriv->mmio + VENDOR_SPECIFIC_0_ADDR);
	writel(0x80, hpriv->mmio + VENDOR_SPECIFIC_0_DATA);
#endif

	rc = ahci_reset_controller(host);
	if (rc)
		goto err0;

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_mv_platform_sht);
	if (rc)
		goto err0;

	return 0;
err0:
	return rc;
}

#ifdef CONFIG_PM
static int ahci_mv_suspend(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;
	void __iomem *mmio = hpriv->mmio;
	u32 ctl;
	int rc;

	if (hpriv->flags & AHCI_HFLAG_NO_SUSPEND) {
		dev_err(dev, "firmware update required for suspend/resume\n");
		return -EIO;
	}

	ctl = readl(mmio + HOST_CTL);
	ctl &= ~HOST_IRQ_EN;
	writel(ctl, mmio + HOST_CTL);
	readl(mmio + HOST_CTL);  

	rc = ata_host_suspend(host, PMSG_SUSPEND);
	if (rc)
		return rc;

	return 0;
}

static int ahci_mv_resume(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	int rc;

	if (pdata->dram != NULL)
		ahci_mv_windows_config(hpriv, pdata->dram);

	if (dev->power.power_state.event == PM_EVENT_SUSPEND) {
		rc = ahci_reset_controller(host);
		if (rc)
			return rc;

		ahci_init_controller(host);
	}

	ata_host_resume(host);

	return 0;
}
#else
#define ahci_mv_suspend NULL
#define ahci_mv_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(ahci_mv_pm_ops, ahci_mv_suspend, ahci_mv_resume);

static int __devexit ahci_mv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ata_host *host = dev_get_drvdata(dev);

	ata_host_detach(host);

	return 0;
}

static struct platform_driver ahci_mv_driver = {
	.probe = ahci_mv_probe,
	.remove = __devexit_p(ahci_mv_remove),
	.driver = {
		.name = "ahci_mv",
		.owner = THIS_MODULE,
		.of_match_table = ahci_of_match,
		.pm = &ahci_mv_pm_ops,
	},
};

static int __init ahci_mv_init(void)
{
	return platform_driver_probe(&ahci_mv_driver, ahci_mv_probe);
}
module_init(ahci_mv_init);

static void __exit ahci_mv_exit(void)
{
	platform_driver_unregister(&ahci_mv_driver);
}
module_exit(ahci_mv_exit);

MODULE_DESCRIPTION("Marvell AHCI SATA platform driver");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci_mv");
