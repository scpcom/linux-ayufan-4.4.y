#ifndef _PFE_HW_H_
#define _PFE_HW_H_

#if !defined(CONFIG_PLATFORM_PCI)
#define PE_SYS_CLK_RATIO	1	/* SYS/AXI = 250MHz, HFE = 500MHz */
#else
#define PE_SYS_CLK_RATIO	0	/* SYS = 40MHz, HFE = 40MHz */
#endif

int pfe_hw_init(struct pfe *pfe);
void pfe_hw_exit(struct pfe *pfe);

#endif /* _PFE_HW_H_ */
