#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __MACH_370_XP_COHERENCY_H
#define __MACH_370_XP_COHERENCY_H

#if defined(MY_ABC_HERE)
extern unsigned long coherency_phys_base;
extern bool coherency_hard_mode;

#define COHERENCY_FABRIC_HARD_MODE() coherency_hard_mode
int set_cpu_coherent(void);

int coherency_init(void);
int coherency_available(void);
#else  
#ifdef CONFIG_SMP
int coherency_get_cpu_count(void);
#endif

int set_cpu_coherent(int cpu_id, int smp_group_id);
int coherency_available(void);
int coherency_init(void);
#endif  

#endif	 
