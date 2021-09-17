#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ARCH_MVEBU_COMMON_H
#define __ARCH_MVEBU_COMMON_H

#if defined(MY_ABC_HERE)
#if defined(MY_ABC_HERE)
#define LSP_VERSION    "linux-3.10.70-2015_T1.1p7"
#elif defined(MY_ABC_HERE)
#define LSP_VERSION    "linux-3.10.70-2015_T1.1p4"
#else
#define LSP_VERSION    "linux-3.10.70-2015_T1.1"
#endif
#endif  

void mvebu_restart(char mode, const char *cmd);

#if defined(MY_ABC_HERE)
void armada_xp_cpu_die(unsigned int cpu);
void mvebu_pmsu_set_cpu_boot_addr(int hw_cpu, void *boot_addr);
int mvebu_boot_cpu(int cpu);

#if defined(MY_ABC_HERE)
int mvebu_pm_suspend_init(void (*board_pm_enter)(void __iomem *sdram_reg,
							u32 srcmd));
void mvebu_pm_register_init(int (*board_pm_init)(void));
#else  
int mvebu_pm_init(void (*board_pm_enter)(void __iomem *sdram_reg, u32 srcmd));
#endif  
#else  
void armada_370_xp_init_irq(void);
void armada_370_xp_handle_irq(struct pt_regs *regs);

void armada_xp_cpu_die(unsigned int cpu);
int armada_370_xp_coherency_init(void);
int armada_370_xp_pmsu_init(void);
void armada_xp_secondary_startup(void);
extern struct smp_operations armada_xp_smp_ops;
#endif  

#endif
