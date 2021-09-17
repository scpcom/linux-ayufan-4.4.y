#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __MACH_MVEBU_PMSU_H
#define __MACH_MVEBU_PMSU_H

int armada_xp_boot_cpu(unsigned int cpu_id, void *phys_addr);
#if defined(MY_ABC_HERE)
int mvebu_setup_boot_addr_wa(unsigned int crypto_eng_target,
				unsigned int crypto_eng_attribute,
				phys_addr_t resume_addr_reg);

void mvebu_v7_pmsu_idle_exit(void);
void armada_370_xp_cpu_resume(void);
int armada_38x_do_cpu_suspend(unsigned long deepidle);
#if defined(MY_ABC_HERE)
void mvebu_v7_pmsu_disable_dfs_cpu(int hw_cpu);
#endif  
#endif  
#endif	 
