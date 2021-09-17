#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/of_irq.h>
#include <linux/of_pci.h>

int (*ltq_pci_plat_arch_init)(struct pci_dev *dev) = NULL;
int (*ltq_pci_plat_dev_init)(struct pci_dev *dev) = NULL;

int pcibios_plat_dev_init(struct pci_dev *dev)
{
	if (ltq_pci_plat_arch_init)
		return ltq_pci_plat_arch_init(dev);

	if (ltq_pci_plat_dev_init)
		return ltq_pci_plat_dev_init(dev);

	return 0;
}

int __init pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
#if defined(MY_ABC_HERE)
	struct of_phandle_args dev_irq;
#else  
	struct of_irq dev_irq;
#endif  
	int irq;

#if defined(MY_ABC_HERE)
	if (of_irq_parse_pci(dev, &dev_irq)) {
#else  
	if (of_irq_map_pci(dev, &dev_irq)) {
#endif  
		dev_err(&dev->dev, "trying to map irq for unknown slot:%d pin:%d\n",
			slot, pin);
		return 0;
	}
#if defined(MY_ABC_HERE)
	irq = irq_create_of_mapping(&dev_irq);
#else  
	irq = irq_create_of_mapping(dev_irq.controller, dev_irq.specifier,
					dev_irq.size);
#endif  
	dev_info(&dev->dev, "SLOT:%d PIN:%d IRQ:%d\n", slot, pin, irq);
	return irq;
}
