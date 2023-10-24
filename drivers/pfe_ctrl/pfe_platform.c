#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include "pfe_mod.h"

/**
 * pfe_platform_probe -
 *
 *
 */
static int pfe_platform_probe(struct platform_device *pdev)
{
	struct resource *r;
	int rc;
	struct clk *clk_axi;

	printk(KERN_INFO "%s\n", __func__);

	pfe = kzalloc(sizeof(struct pfe), GFP_KERNEL);
	if (!pfe) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	platform_set_drvdata(pdev, pfe);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ddr");
	if (!r) {
		printk(KERN_INFO "platform_get_resource_byname(ddr) failed\n");
		rc = -ENXIO;
		goto err_ddr;
	}
	
	pfe->ddr_phys_baseaddr = r->start;
	pfe->ddr_size = resource_size(r);

	pfe->ddr_baseaddr = ioremap(r->start, resource_size(r));
	if (!pfe->ddr_baseaddr) {
		printk(KERN_INFO "ioremap() ddr failed\n");
		rc = -ENOMEM;
		goto err_ddr;
	}
	
	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "axi");
	if (!r) {
		printk(KERN_INFO "platform_get_resource_byname(axi) failed\n");
		rc = -ENXIO;
		goto err_axi;
	}

	pfe->cbus_baseaddr = ioremap(r->start, resource_size(r));
	if (!pfe->cbus_baseaddr) {
		printk(KERN_INFO "ioremap() axi failed\n");
		rc = -ENOMEM;
		goto err_axi;
	}

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apb");
	if (!r) {
		printk(KERN_INFO "platform_get_resource_byname(apb) failed\n");
		rc = -ENXIO;
		goto err_apb;
	}

	pfe->apb_baseaddr = ioremap(r->start, resource_size(r));
	if (!pfe->apb_baseaddr) {
		printk(KERN_INFO "ioremap() apb failed\n");
		rc = -ENOMEM;
		goto err_apb;
	}

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "iram");
	if (!r) {
		printk(KERN_INFO "platform_get_resource_byname(iram) failed\n");
		rc = -ENXIO;
		goto err_iram;
	}

	pfe->iram_phys_baseaddr = r->start;
	pfe->iram_baseaddr = ioremap(r->start, resource_size(r));
	if (!pfe->iram_baseaddr) {
		printk(KERN_INFO "ioremap() iram failed\n");
		rc = -ENOMEM;
		goto err_iram;
	}

        r = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ipsec");
        if (!r) {
                printk(KERN_INFO "platform_get_resource_byname(ipsec) failed\n");
                rc = -ENXIO;
                goto err_ipsec;
        }

        pfe->ipsec_phys_baseaddr = r->start;
	/* Just map only initial 1MB , as its enough to access espah engine
	*/
        //pfe->ipsec_baseaddr = ioremap(r->start, resource_size(r));
        pfe->ipsec_baseaddr = ioremap(r->start, 1*1024*1024);
        if (!pfe->ipsec_baseaddr) {
                printk(KERN_INFO "ioremap() ipsec failed\n");
                rc = -ENOMEM;
                goto err_ipsec;
        }

        printk(KERN_INFO "ipsec: baseaddr :%x --- %x\n",  (u32)pfe->ipsec_phys_baseaddr,  (u32)pfe->ipsec_baseaddr);

	pfe->hif_irq = platform_get_irq_byname(pdev, "hif");
	if (pfe->hif_irq < 0) {
		printk(KERN_INFO "platform_get_irq_byname(hif) failed\n");
		rc = pfe->hif_irq;
		goto err_hif_irq;
	}

#if 0
	pfe->hif_client_irq = platform_get_irq_byname(pdev, "hif_client");
	if (pfe->hif_client_irq < 0) {
		printk(KERN_INFO "platform_get_irq_byname(hif_client) failed\n");
		rc = pfe->hif_client_irq;
		goto err_hif_irq;
	}
#endif

	pfe->dev = &pdev->dev;

	/* FIXME this needs to be done at the BSP level with proper locking */
	writel(readl(AXI_RESET_1) | (1 << 3), AXI_RESET_1);
	mdelay(1);
	writel(readl(AXI_RESET_1) & ~(1 << 3), AXI_RESET_1);

	/* Get the system clock */
	clk_axi = clk_get(NULL,"axi");
	if (IS_ERR(clk_axi)) {
		printk(KERN_INFO "clk_get call failed\n");
		goto err_clk;
	}
	pfe->ctrl.clk_axi = clk_axi;
	pfe->ctrl.sys_clk = clk_get_rate(clk_axi) / 1000;  // save sys_clk value as KHz

	rc = pfe_probe(pfe);
	if (rc < 0)
		goto err_probe;

	return 0;

err_probe:
	clk_put(clk_axi);

err_clk:
err_hif_irq:
        iounmap(pfe->ipsec_baseaddr);
err_ipsec:
	iounmap(pfe->iram_baseaddr);
err_iram:
	iounmap(pfe->apb_baseaddr);

err_apb:
	iounmap(pfe->cbus_baseaddr);

err_axi:
	iounmap(pfe->ddr_baseaddr);

err_ddr:
	platform_set_drvdata(pdev, NULL);

	kfree(pfe);

err_alloc:
	return rc;
}


/**
 * pfe_platform_remove -
 *
 *
 */
static int pfe_platform_remove(struct platform_device *pdev)
{
	struct pfe *pfe = platform_get_drvdata(pdev);
	int rc;
	
	printk(KERN_INFO "%s\n", __func__);

	rc = pfe_remove(pfe);

	/* FIXME this needs to be done at the BSP level with proper locking */
	writel(readl(AXI_RESET_1) | (1 << 3), AXI_RESET_1);

	clk_put(pfe->ctrl.clk_axi);
        iounmap(pfe->ipsec_baseaddr);
	iounmap(pfe->iram_baseaddr);
	iounmap(pfe->apb_baseaddr);
	iounmap(pfe->cbus_baseaddr);
	iounmap(pfe->ddr_baseaddr);

	platform_set_drvdata(pdev, NULL);

	kfree(pfe);

	return rc;
}


static struct platform_driver pfe_platform_driver = {
	.probe = pfe_platform_probe,
	.remove = pfe_platform_remove,
	.driver = {
		.name = "pfe",
	},
};


static int __init pfe_module_init(void)
{
	printk(KERN_INFO "%s\n", __func__);

	return platform_driver_register(&pfe_platform_driver);
}


static void __exit pfe_module_exit(void)
{
	printk(KERN_INFO "here before:%s\n", __func__);
	platform_driver_unregister(&pfe_platform_driver);
	printk(KERN_INFO "here after:%s\n", __func__);
}

MODULE_LICENSE("GPL");
module_init(pfe_module_init);
module_exit(pfe_module_exit);
