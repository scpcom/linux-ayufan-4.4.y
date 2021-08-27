 
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#ifdef CONFIG_SYNO_ALPINE
#include <linux/of.h>
#endif

#include "spi-dw.h"

#define DRIVER_NAME "dw_spi_pci"

struct dw_spi_pci {
	struct pci_dev	*pdev;
	struct dw_spi	dws;
};

static int __devinit spi_pci_probe(struct pci_dev *pdev,
	const struct pci_device_id *ent)
{
	struct dw_spi_pci *dwpci;
	struct dw_spi *dws;
	int pci_bar = 0;
#ifdef CONFIG_SYNO_ALPINE
	int ret, num_cs, bus_num;
#else
	int ret;
#endif

	printk(KERN_INFO "DW: found PCI SPI controller(ID: %04x:%04x)\n",
		pdev->vendor, pdev->device);

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	dwpci = kzalloc(sizeof(struct dw_spi_pci), GFP_KERNEL);
	if (!dwpci) {
		ret = -ENOMEM;
		goto err_disable;
	}

	dwpci->pdev = pdev;
	dws = &dwpci->dws;

	dws->paddr = pci_resource_start(pdev, pci_bar);
	dws->iolen = pci_resource_len(pdev, pci_bar);

	ret = pci_request_region(pdev, pci_bar, dev_name(&pdev->dev));
	if (ret)
		goto err_kfree;

	dws->regs = ioremap_nocache((unsigned long)dws->paddr,
				pci_resource_len(pdev, pci_bar));
	if (!dws->regs) {
		ret = -ENOMEM;
		goto err_release_reg;
	}

#ifdef CONFIG_SYNO_ALPINE
	ret = of_property_read_u32(pdev->dev.of_node, "bus-num", &bus_num);
	if (ret < 0)
		dws->bus_num = 0;
	else
		dws->bus_num = bus_num;

	ret = of_property_read_u32(pdev->dev.of_node, "num-chipselect", &num_cs);
	if (ret < 0)
		dws->num_cs = 4;
	else
		dws->num_cs = num_cs;

	dws->parent_dev = &pdev->dev;
#else
	dws->parent_dev = &pdev->dev;
	dws->bus_num = 0;
	dws->num_cs = 4;
#endif
	dws->irq = pdev->irq;

	if (pdev->device == 0x0800) {
		ret = dw_spi_mid_init(dws);
		if (ret)
			goto err_unmap;
	}

	ret = dw_spi_add_host(dws);
	if (ret)
		goto err_unmap;

	pci_set_drvdata(pdev, dwpci);
	return 0;

err_unmap:
	iounmap(dws->regs);
err_release_reg:
	pci_release_region(pdev, pci_bar);
err_kfree:
	kfree(dwpci);
err_disable:
	pci_disable_device(pdev);
	return ret;
}

static void __devexit spi_pci_remove(struct pci_dev *pdev)
{
	struct dw_spi_pci *dwpci = pci_get_drvdata(pdev);

	pci_set_drvdata(pdev, NULL);
	dw_spi_remove_host(&dwpci->dws);
	iounmap(dwpci->dws.regs);
	pci_release_region(pdev, 0);
	kfree(dwpci);
	pci_disable_device(pdev);
}

#ifdef CONFIG_PM
static int spi_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct dw_spi_pci *dwpci = pci_get_drvdata(pdev);
	int ret;

	ret = dw_spi_suspend_host(&dwpci->dws);
	if (ret)
		return ret;
	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));
	return ret;
}

static int spi_resume(struct pci_dev *pdev)
{
	struct dw_spi_pci *dwpci = pci_get_drvdata(pdev);
	int ret;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	ret = pci_enable_device(pdev);
	if (ret)
		return ret;
	return dw_spi_resume_host(&dwpci->dws);
}
#else
#define spi_suspend	NULL
#define spi_resume	NULL
#endif

static const struct pci_device_id pci_ids[] __devinitdata = {
	 
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x0800) },
	{},
};

#ifdef CONFIG_SYNO_ALPINE
static struct of_device_id dw_spi_pci_of_match[] = {
		{ .compatible = "snps,dw-spi-pci", },
		{  }
};
MODULE_DEVICE_TABLE(of, dw_spi_pci_of_match);
#endif
static struct pci_driver dw_spi_driver = {
	.name =		DRIVER_NAME,
	.id_table =	pci_ids,
	.probe =	spi_pci_probe,
	.remove =	__devexit_p(spi_pci_remove),
	.suspend =	spi_suspend,
	.resume	=	spi_resume,
#ifdef CONFIG_SYNO_ALPINE
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = dw_spi_pci_of_match,
	},
#endif
};

static int __init mrst_spi_init(void)
{
	return pci_register_driver(&dw_spi_driver);
}

static void __exit mrst_spi_exit(void)
{
	pci_unregister_driver(&dw_spi_driver);
}

module_init(mrst_spi_init);
module_exit(mrst_spi_exit);

MODULE_AUTHOR("Feng Tang <feng.tang@intel.com>");
MODULE_DESCRIPTION("PCI interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
