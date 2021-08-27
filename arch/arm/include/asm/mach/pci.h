 
#ifndef __ASM_MACH_PCI_H
#define __ASM_MACH_PCI_H

struct pci_sys_data;
struct pci_bus;

struct hw_pci {
#ifdef CONFIG_PCI_DOMAINS
	int		domain;
#endif
	struct list_head buses;
#ifdef CONFIG_SYNO_ALPINE
	void	**private_data;
#endif
	int		nr_controllers;
	int		(*setup)(int nr, struct pci_sys_data *);
	struct pci_bus *(*scan)(int nr, struct pci_sys_data *);
	void		(*preinit)(void);
	void		(*postinit)(void);
	u8		(*swizzle)(struct pci_dev *dev, u8 *pin);
	int		(*map_irq)(const struct pci_dev *dev, u8 slot, u8 pin);
};

struct pci_sys_data {
#ifdef CONFIG_PCI_DOMAINS
	int		domain;
#endif
	struct list_head node;
	int		busnr;		 
	u64		mem_offset;	 
	unsigned long	io_offset;	 
	struct pci_bus	*bus;		 
	struct resource *resource[3];	 
					 
	u8		(*swizzle)(struct pci_dev *, u8 *);
					 
	int		(*map_irq)(const struct pci_dev *, u8, u8);
	struct hw_pci	*hw;
#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
	int		mv_controller_num;
#endif
	void		*private_data;	 
};

#define pci_std_swizzle pci_common_swizzle

void pci_common_init(struct hw_pci *);

extern int iop3xx_pci_setup(int nr, struct pci_sys_data *);
extern struct pci_bus *iop3xx_pci_scan_bus(int nr, struct pci_sys_data *);
extern void iop3xx_pci_preinit(void);
extern void iop3xx_pci_preinit_cond(void);

extern int dc21285_setup(int nr, struct pci_sys_data *);
extern struct pci_bus *dc21285_scan_bus(int nr, struct pci_sys_data *);
extern void dc21285_preinit(void);
extern void dc21285_postinit(void);

extern int via82c505_setup(int nr, struct pci_sys_data *);
extern struct pci_bus *via82c505_scan_bus(int nr, struct pci_sys_data *);
extern void via82c505_init(void *sysdata);

extern int pci_v3_setup(int nr, struct pci_sys_data *);
extern struct pci_bus *pci_v3_scan_bus(int nr, struct pci_sys_data *);
extern void pci_v3_preinit(void);
extern void pci_v3_postinit(void);

#endif  
