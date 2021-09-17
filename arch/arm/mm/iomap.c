#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/arch/arm/mm/iomap.c
 *
 * Map IO port and PCI memory spaces so that {read,write}[bwl] can
 * be used to access this memory.
 */
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/io.h>
#if defined (MY_DEF_HERE)
#ifdef MY_DEF_HERE
#else /* MY_DEF_HERE */
#include <linux/of.h>
#endif /* MY_DEF_HERE */
#endif /* MY_DEF_HERE */

unsigned long vga_base;
EXPORT_SYMBOL(vga_base);

#ifdef __io
void __iomem *ioport_map(unsigned long port, unsigned int nr)
{
	return __io(port);
}
EXPORT_SYMBOL(ioport_map);

void ioport_unmap(void __iomem *addr)
{
}
EXPORT_SYMBOL(ioport_unmap);
#endif

#ifdef CONFIG_PCI
unsigned long pcibios_min_io = 0x1000;
EXPORT_SYMBOL(pcibios_min_io);

unsigned long pcibios_min_mem = 0x01000000;
EXPORT_SYMBOL(pcibios_min_mem);

void pci_iounmap(struct pci_dev *dev, void __iomem *addr)
{
#if defined (MY_DEF_HERE)
#ifdef MY_DEF_HERE
#else /* MY_DEF_HERE */
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	if (of_machine_is_compatible("st,stih407")
	    || of_machine_is_compatible("st,stih410"))
		addr = __stm_unfrob(addr);
#endif
#endif /* MY_DEF_HERE */
#endif /* MY_DEF_HERE */
	if ((unsigned long)addr >= VMALLOC_START &&
	    (unsigned long)addr < VMALLOC_END)
		iounmap(addr);
}
EXPORT_SYMBOL(pci_iounmap);
#endif
