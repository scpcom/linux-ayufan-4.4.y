#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __MACH_STI_SMP_H
#define __MACH_STI_SMP_H

extern struct smp_operations	sti_smp_ops;
void sti_secondary_startup(void);
#ifdef MY_DEF_HERE
int sti_abap_c_start(void);
extern unsigned int sti_abap_c_size;
#endif  

#endif
