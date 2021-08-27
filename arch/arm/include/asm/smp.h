#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_ARM_SMP_H
#define __ASM_ARM_SMP_H

#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/thread_info.h>

#ifndef CONFIG_SMP
# error "<asm/smp.h> included in non-SMP build"
#endif

#define raw_smp_processor_id() (current_thread_info()->cpu)

struct seq_file;

extern void show_ipi_list(struct seq_file *, int);

asmlinkage void do_IPI(int ipinr, struct pt_regs *regs);

void handle_IPI(int ipinr, struct pt_regs *regs);

extern void smp_init_cpus(void);

extern void set_smp_cross_call(void (*)(const struct cpumask *, unsigned int));

extern int boot_secondary(unsigned int cpu, struct task_struct *);

asmlinkage void secondary_start_kernel(void);

extern void platform_secondary_init(unsigned int cpu);

extern void platform_smp_prepare_cpus(unsigned int);

extern int __cpu_logical_map[NR_CPUS];
#define cpu_logical_map(cpu)	__cpu_logical_map[cpu]

struct secondary_data {
	unsigned long pgdir;
	unsigned long swapper_pg_dir;
	void *stack;
};
extern struct secondary_data secondary_data;

extern int __cpu_disable(void);
extern int platform_cpu_disable(unsigned int cpu);

extern void __cpu_die(unsigned int cpu);
extern void cpu_die(void);
#ifdef CONFIG_SYNO_ALPINE
extern void arch_send_wakeup_ipi_mask(const struct cpumask *mask);
#endif

extern void platform_cpu_die(unsigned int cpu);
extern int platform_cpu_kill(unsigned int cpu);
extern void platform_cpu_enable(unsigned int cpu);

extern void arch_send_call_function_single_ipi(int cpu);
extern void arch_send_call_function_ipi_mask(const struct cpumask *mask);

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_ARCH_ARMADA_XP) && defined(CONFIG_PERF_EVENTS)
extern void show_local_pmu_irqs(struct seq_file *, int);
#endif

#endif  
