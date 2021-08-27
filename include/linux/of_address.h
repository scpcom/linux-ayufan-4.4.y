#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __OF_ADDRESS_H
#define __OF_ADDRESS_H
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/of.h>

#ifdef MY_DEF_HERE
struct of_pci_range_iter {
	const __be32 *range, *end;
	int np, pna;

	u32 pci_space;
	u64 pci_addr;
	u64 cpu_addr;
	u64 size;
	u32 flags;
};

#define for_each_of_pci_range(iter, np) \
	for (memset((iter), 0, sizeof(struct of_pci_range_iter)); \
	     of_pci_process_ranges(iter, np);)

#define range_iter_fill_resource(iter, np, res) \
	do { \
		(res)->flags = (iter).flags; \
		(res)->start = (iter).cpu_addr; \
		(res)->end = (iter).cpu_addr + (iter).size - 1; \
		(res)->parent = (res)->child = (res)->sibling = NULL; \
		(res)->name = (np)->full_name; \
	} while (0)
#endif
extern u64 of_translate_address(struct device_node *np, const __be32 *addr);
extern int of_address_to_resource(struct device_node *dev, int index,
				  struct resource *r);
extern struct device_node *of_find_matching_node_by_address(
					struct device_node *from,
					const struct of_device_id *matches,
					u64 base_address);
extern void __iomem *of_iomap(struct device_node *device, int index);

extern const u32 *of_get_address(struct device_node *dev, int index,
			   u64 *size, unsigned int *flags);

#ifndef pci_address_to_pio
static inline unsigned long pci_address_to_pio(phys_addr_t addr) { return -1; }
#define pci_address_to_pio pci_address_to_pio
#endif

#ifdef CONFIG_PCI
#ifdef MY_DEF_HERE
extern struct of_pci_range_iter *of_pci_process_ranges(struct of_pci_range_iter *iter,
						struct device_node *node);
#endif
extern const __be32 *of_get_pci_address(struct device_node *dev, int bar_no,
			       u64 *size, unsigned int *flags);
extern int of_pci_address_to_resource(struct device_node *dev, int bar,
				      struct resource *r);
#else  
#ifdef MY_DEF_HERE
struct of_pci_range_iter *of_pci_process_ranges(struct of_pci_range_iter *iter,
						struct device_node *node)
{
	return NULL;
}
#endif
static inline int of_pci_address_to_resource(struct device_node *dev, int bar,
				             struct resource *r)
{
	return -ENOSYS;
}

static inline const __be32 *of_get_pci_address(struct device_node *dev,
		int bar_no, u64 *size, unsigned int *flags)
{
	return NULL;
}
#endif  

#endif  
