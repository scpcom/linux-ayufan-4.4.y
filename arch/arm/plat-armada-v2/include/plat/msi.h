#ifndef _ARMADA_MSI_H_
#define _ARMADA_MSI_H_
#ifdef CONFIG_PCI_MSI
void armada_msi_init(void);
#else
static inline void armada_msi_init(void)
{
	return;
}
#endif
#endif
