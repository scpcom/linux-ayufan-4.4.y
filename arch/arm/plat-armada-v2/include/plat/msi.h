#ifndef _ARMADA_MSI_H_
#define _ARMADA_MSI_H_
#ifdef CONFIG_PCI_MSI
void armada_msi_init(void);
void armada_msi_init_unmask(void);
#endif
#endif
