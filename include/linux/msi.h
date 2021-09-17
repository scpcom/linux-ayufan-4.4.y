#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef LINUX_MSI_H
#define LINUX_MSI_H

#include <linux/kobject.h>
#include <linux/list.h>

struct msi_msg {
	u32	address_lo;	 
	u32	address_hi;	 
	u32	data;		 
};

struct irq_data;
struct msi_desc;
void mask_msi_irq(struct irq_data *data);
void unmask_msi_irq(struct irq_data *data);
void __read_msi_msg(struct msi_desc *entry, struct msi_msg *msg);
void __get_cached_msi_msg(struct msi_desc *entry, struct msi_msg *msg);
void __write_msi_msg(struct msi_desc *entry, struct msi_msg *msg);
void read_msi_msg(unsigned int irq, struct msi_msg *msg);
void get_cached_msi_msg(unsigned int irq, struct msi_msg *msg);
void write_msi_msg(unsigned int irq, struct msi_msg *msg);

struct msi_desc {
	struct {
		__u8	is_msix	: 1;
		__u8	multiple: 3;	 
		__u8	maskbit	: 1; 	 
		__u8	is_64	: 1;	 
		__u8	pos;	 	 
		__u16	entry_nr;    	 
		unsigned default_irq;	 
	} msi_attrib;

	u32 masked;			 
	unsigned int irq;
	struct list_head list;

	union {
		void __iomem *mask_base;
		u8 mask_pos;
	};
	struct pci_dev *dev;

	struct msi_msg msg;

	struct kobject kobj;
};

#if defined (MY_DEF_HERE) || defined(MY_ABC_HERE)
 
#else  
 
#endif  
int arch_setup_msi_irq(struct pci_dev *dev, struct msi_desc *desc);
void arch_teardown_msi_irq(unsigned int irq);
int arch_setup_msi_irqs(struct pci_dev *dev, int nvec, int type);
void arch_teardown_msi_irqs(struct pci_dev *dev);
int arch_msi_check_device(struct pci_dev* dev, int nvec, int type);
#if defined (MY_DEF_HERE) || defined(MY_ABC_HERE)
void arch_restore_msi_irqs(struct pci_dev *dev, int irq);

void default_teardown_msi_irqs(struct pci_dev *dev);
void default_restore_msi_irqs(struct pci_dev *dev, int irq);

struct msi_chip {
	struct module *owner;
	struct device *dev;
	struct device_node *of_node;
	struct list_head list;

	int (*setup_irq)(struct msi_chip *chip, struct pci_dev *dev,
			 struct msi_desc *desc);
	void (*teardown_irq)(struct msi_chip *chip, unsigned int irq);
	int (*check_device)(struct msi_chip *chip, struct pci_dev *dev,
			    int nvec, int type);
};
#endif  

#endif  
