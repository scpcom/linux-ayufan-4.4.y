 
#ifndef __MACH_STI_SMP_H
#define __MACH_STI_SMP_H

extern struct smp_operations	sti_smp_ops;
void sti_secondary_startup(void);
#ifdef CONFIG_SYNO_LSP_MONACO_SDK2_15_4
int sti_abap_c_start(void);
extern unsigned int sti_abap_c_size;
#endif  

#endif
