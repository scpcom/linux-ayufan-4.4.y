#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#if defined(MY_ABC_HERE)
#include <linux/clk.h>
#include <mach/reset.h>
#include <mach/comcerto-2000/pm.h>
#endif
#include "ahci.h"
#if defined(MY_ABC_HERE)
#include <mach/serdes-c2000.h>
#endif

#if defined(MY_ABC_HERE) && defined(CONFIG_ARCH_M86XXX)
 
static struct clk *sata_oob_clk;  
static struct clk *sata_pmu_clk;  
static struct clk *sata_clk;	 
#if defined(CONFIG_COMCERTO_SATA_OCC_CLOCK)
static struct clk *sata_occ_clk;  
#endif
#endif 

enum ahci_type {
	AHCI,		 
	IMX53_AHCI,	 
};

static struct platform_device_id ahci_devtype[] = {
	{
		.name = "ahci",
		.driver_data = AHCI,
	}, {
		.name = "imx53-ahci",
		.driver_data = IMX53_AHCI,
	}, {
		 
	}
};
MODULE_DEVICE_TABLE(platform, ahci_devtype);

static const struct ata_port_info ahci_port_info[] = {
	 
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

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT("ahci_platform"),
};

#if defined(MY_ABC_HERE) && defined(CONFIG_PM)
static int ahci_platform_suspend(struct platform_device *pdev, pm_message_t state)
{
        struct ata_host *host = platform_get_drvdata(pdev);
	int ret=0;

#ifdef CONFIG_ARCH_M86XXX
	  
	if ( !(host_utilpe_shared_pmu_bitmask & SATA_IRQ )){

		return ret;
	}
#endif

        if (host)
		ret = ata_host_suspend(host, state);

#ifdef CONFIG_ARCH_M86XXX
	if (!ret)  
	{
		 
		clk_disable(sata_clk);
		clk_disable(sata_oob_clk);
		clk_disable(sata_pmu_clk);

		if (readl(COMCERTO_GPIO_SYSTEM_CONFIG) & BOOT_SERDES1_CNF_SATA0)
			writel((readl((COMCERTO_DWC1_CFG_BASE+0x44)) | 0xCC), (COMCERTO_DWC1_CFG_BASE+0x44));
		else if (readl(COMCERTO_GPIO_SYSTEM_CONFIG) & BOOT_SERDES2_CNF_SATA1)
			writel((readl((COMCERTO_DWC1_CFG_BASE+0x54)) | 0xCC), (COMCERTO_DWC1_CFG_BASE+0x54));

	}
#endif
	
        return ret;
}

static int ahci_platform_resume(struct platform_device *pdev)
{
        struct ata_host *host = platform_get_drvdata(pdev);

#ifdef CONFIG_ARCH_M86XXX
	 
	if (readl(COMCERTO_GPIO_SYSTEM_CONFIG) & BOOT_SERDES1_CNF_SATA0)
 		writel((readl((COMCERTO_DWC1_CFG_BASE+0x44)) & ~0xCC), (COMCERTO_DWC1_CFG_BASE+0x44));
	else if (readl(COMCERTO_GPIO_SYSTEM_CONFIG) & BOOT_SERDES2_CNF_SATA1)
		writel((readl((COMCERTO_DWC1_CFG_BASE+0x54)) & ~0xCC), (COMCERTO_DWC1_CFG_BASE+0x54));

	if ( !(host_utilpe_shared_pmu_bitmask & SATA_IRQ )){

		return 0;
	}

	clk_enable(sata_clk);
	clk_enable(sata_oob_clk);
	clk_enable(sata_pmu_clk);
#endif

        if (host) 
		ata_host_resume(host);

	return 0;
}
#else
#define ahci_platform_suspend NULL
#define ahci_platform_resume NULL
#endif

static int __init ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	const struct platform_device_id *id = platform_get_device_id(pdev);
	struct ata_port_info pi = ahci_port_info[id ? id->driver_data : 0];
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;
#if defined(MY_ABC_HERE) && defined(CONFIG_ARCH_M86XXX)
	 
	sata_clk = clk_get(NULL,"sata");
	 
	if (IS_ERR(sata_clk)) {
		pr_err("%s: Unable to obtain SATA(AXI) clock: %ld\n",__func__,PTR_ERR(sata_clk));
		return PTR_ERR(sata_clk);
 	}

        rc = clk_enable(sata_clk);
	if (rc){
		pr_err("%s: SATA(AXI) clock enable failed \n",__func__);
                return rc;
	}
	sata_oob_clk = clk_get(NULL,"sata_oob");
	 
	if (IS_ERR(sata_oob_clk)) {
		pr_err("%s: Unable to obtain SATA_OOB clock: %ld\n",__func__,PTR_ERR(sata_oob_clk));
		return PTR_ERR(sata_oob_clk);
 	}

	sata_pmu_clk = clk_get(NULL,"sata_pmu");
	 
	if (IS_ERR(sata_pmu_clk)) {
		pr_err("%s: Unable to obtain SATA_PMU clock: %ld\n",__func__,PTR_ERR(sata_pmu_clk));
		return PTR_ERR(sata_pmu_clk);
	}
	 
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
#if defined(CONFIG_COMCERTO_SATA_OCC_CLOCK)
	sata_occ_clk = clk_get(NULL,"sata_occ");
	 
	if (IS_ERR(sata_occ_clk)) {
		pr_err("%s: Unable to obtain sata occ clock: %ld\n",__func__,PTR_ERR(sata_occ_clk));
		return PTR_ERR(sata_occ_clk);
 	}
	 
        rc = clk_enable(sata_occ_clk);
	if (rc){
		pr_err("%s: sata occ clock enable failed \n",__func__);
		return rc;
	}
#endif
	 
	clk_set_rate(sata_oob_clk,125000000);
	clk_set_rate(sata_pmu_clk,30000000);
	
#endif

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

	if (pdata && pdata->ata_port_info)
		pi = *pdata->ata_port_info;

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

	if (pdata && pdata->init) {
		rc = pdata->init(dev, hpriv->mmio);
		if (rc)
			return rc;
	}

	ahci_save_initial_config(dev, hpriv,
		pdata ? pdata->force_port_map : 0,
		pdata ? pdata->mask_port_map  : 0);

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

#if defined(MY_ABC_HERE) && defined(CONFIG_ARCH_M86XXX)
		 
		writel(0x41, ahci_port_base(ap) + 0x70);
#endif

		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

	rc = ahci_reset_controller(host);
	if (rc)
		goto err0;

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_platform_sht);
	if (rc)
		goto err0;

	return 0;
err0:
	if (pdata && pdata->exit)
		pdata->exit(dev);
	return rc;
}

static int __devexit ahci_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	struct ata_host *host = dev_get_drvdata(dev);

	ata_host_detach(host);

	if (pdata && pdata->exit)
		pdata->exit(dev);
#if defined(MY_ABC_HERE) && defined(CONFIG_ARCH_M86XXX)
	 
	clk_disable(sata_clk);
	clk_put(sata_clk);
	clk_disable(sata_oob_clk);
	clk_put(sata_oob_clk);
	clk_disable(sata_pmu_clk);
	clk_put(sata_pmu_clk);
#if defined(CONFIG_COMCERTO_SATA_OCC_CLOCK)
	clk_disable(sata_occ_clk);
	clk_put(sata_occ_clk);
#endif
	 
	c2000_block_reset(COMPONENT_AXI_SATA,1);

	c2000_block_reset(COMPONENT_SERDES1,1);
	c2000_block_reset(COMPONENT_SERDES_SATA0,1);

	c2000_block_reset(COMPONENT_SERDES2,1);
	c2000_block_reset(COMPONENT_SERDES_SATA1,1);
#endif

	return 0;
}

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "calxeda,hb-ahci", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static struct platform_driver ahci_driver = {
	.remove  = __devexit_p(ahci_remove),
#if defined(MY_ABC_HERE) && defined(CONFIG_PM)
	.suspend = ahci_platform_suspend,
	.resume  = ahci_platform_resume,
#endif
	.driver  = {
		 .name = "ahci",
		 .owner = THIS_MODULE,
		 .of_match_table = ahci_of_match,
	},
	.id_table = ahci_devtype,
};

static int __init ahci_init(void)
{
	return platform_driver_probe(&ahci_driver, ahci_probe);
}
module_init(ahci_init);

static void __exit ahci_exit(void)
{
	platform_driver_unregister(&ahci_driver);
}
module_exit(ahci_exit);

MODULE_DESCRIPTION("AHCI SATA platform driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci");
