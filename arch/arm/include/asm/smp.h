 
#ifndef __ASM_ARM_SMP_H
#define __ASM_ARM_SMP_H

#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/thread_info.h>

#include <mach/smp.h>

#ifndef CONFIG_SMP
# error "<asm/smp.h> included in non-SMP build"
#endif

#define raw_smp_processor_id() (current_thread_info()->cpu)

#define PROC_CHANGE_PENALTY		15

struct seq_file;

extern void show_ipi_list(struct seq_file *p);

asmlinkage void do_IPI(struct pt_regs *regs);

extern void smp_init_cpus(void);

extern void smp_store_cpu_info(unsigned int cpuid);

extern void smp_cross_call(const struct cpumask *mask);

extern int boot_secondary(unsigned int cpu, struct task_struct *);

asmlinkage void secondary_start_kernel(void);

extern void platform_secondary_init(unsigned int cpu);

struct secondary_data {
	unsigned long pgdir;
	void *stack;
};
extern struct secondary_data secondary_data;

extern int __cpu_disable(void);
extern int mach_cpu_disable(unsigned int cpu);

extern void __cpu_die(unsigned int cpu);
extern void cpu_die(void);

extern void platform_cpu_die(unsigned int cpu);
extern int platform_cpu_kill(unsigned int cpu);
extern void platform_cpu_enable(unsigned int cpu);

extern void arch_send_call_function_single_ipi(int cpu);
extern void arch_send_call_function_ipi_mask(const struct cpumask *mask);

extern void show_local_irqs(struct seq_file *);

#if defined(CONFIG_SYNO_PLX_PORTING)
 
extern void smp_send_timer(void);
#endif

#endif  
