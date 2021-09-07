 
#ifndef _POWERPC_SYSDEV_FSL_MSI_H
#define _POWERPC_SYSDEV_FSL_MSI_H

#include <asm/msi_bitmap.h>

#define NR_MSI_REG		8
#define IRQS_PER_MSI_REG	32
#define NR_MSI_IRQS	(NR_MSI_REG * IRQS_PER_MSI_REG)

#define FSL_PIC_IP_MASK	0x0000000F
#define FSL_PIC_IP_MPIC	0x00000001
#define FSL_PIC_IP_IPIC	0x00000002

struct fsl_msi {
	struct irq_host *irqhost;

	unsigned long cascade_irq;

	u32 msi_addr_lo;
	u32 msi_addr_hi;
	void __iomem *msi_regs;
	u32 feature;
#ifdef CONFIG_SYNO_QORIQ
	int msi_virqs[NR_MSI_REG];
#endif

	struct msi_bitmap bitmap;
#ifdef CONFIG_SYNO_QORIQ

	struct list_head list;           
#endif
};

#endif  
