#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#ifdef MY_DEF_HERE
#include <linux/reboot.h>
#include <linux/notifier.h>
#endif  

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/cp15.h>
#include <asm/idmap.h>

#include <linux/power/st_lpm_def.h>
#include "smp.h"

static void __iomem *sbc_base;
static unsigned long sbc_ph_base;
#ifdef MY_DEF_HERE
void *abap_prg_base;
static unsigned long abap_ph_base;

#ifdef CONFIG_KEXEC
static int kexec_case;

static int kexec_notify_cb(struct notifier_block *nb, ulong event, void *buf)
{
	if (event == SYS_RESTART)
		kexec_case = 1;
	return NOTIFY_DONE;
};

static struct notifier_block kexec_notifier = {
	.notifier_call = kexec_notify_cb,
};
#endif
#endif  

static void __cpuinit write_pen_release(int val)
{
	pen_release = val;
	 
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

static DEFINE_SPINLOCK(boot_lock);

void __cpuinit sti_secondary_init(unsigned int cpu)
{
	trace_hardirqs_off();

	write_pen_release(-1);

	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int __cpuinit sti_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	spin_lock(&boot_lock);

	write_pen_release(cpu_logical_map(cpu));

	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		if (pen_release == -1)
			break;

		udelay(10);
	}

	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

void __init sti_smp_prepare_cpus(unsigned int max_cpus)
{
	void __iomem *scu_base = NULL;
	struct resource res;

	struct device_node *np = of_find_compatible_node(
					NULL, NULL, "arm,cortex-a9-scu");
	void __iomem *cpu_strt_ptr =
		(void *)CONFIG_PAGE_OFFSET + 0x200;
	unsigned int sbc_offset = DATA_OFFSET + PEN_HOLD_VAR_OFFSET_4xx;

	unsigned long entry_pa = virt_to_phys(sti_secondary_startup);

	if (np) {
		scu_base = of_iomap(np, 0);
		scu_enable(scu_base);
		of_node_put(np);

		if (max_cpus > 1) {
#ifdef MY_DEF_HERE
			 
#else  
			 
#endif  
#ifdef MY_DEF_HERE
#ifdef CONFIG_KEXEC
			register_reboot_notifier(&kexec_notifier);
#endif
#endif  
			np = of_find_compatible_node(NULL, NULL, "st,lpm");

			if (IS_ERR_OR_NULL(np)) {
				__raw_writel(entry_pa, cpu_strt_ptr);
				 
				smp_wmb();
				__cpuc_flush_dcache_area(cpu_strt_ptr, 4);
				outer_clean_range(__pa(cpu_strt_ptr),
						  __pa(cpu_strt_ptr + 1));
#ifdef MY_DEF_HERE

				np = of_find_node_by_name(NULL, "abap-regs");
				if (IS_ERR_OR_NULL(np))
					return;

				abap_prg_base = of_iomap(np, 0);
				if (!of_address_to_resource(np, 0, &res))
					abap_ph_base = res.start;

				of_node_put(np);
				if (!abap_prg_base)
					return;
				 
				__raw_writel(entry_pa, abap_prg_base +
						sti_abap_c_size - sizeof(int));

#ifndef CONFIG_KEXEC
				iounmap(abap_prg_base);
#endif
#endif  
				return;
			}

			sbc_base = of_iomap(np, 0);
			if (!of_address_to_resource(np, 0, &res))
				sbc_ph_base = res.start;

			of_node_put(np);
			if (!sbc_base)
				return;

			writel(entry_pa, sbc_base + sbc_offset);
#ifdef MY_DEF_HERE
#ifndef CONFIG_KEXEC
			iounmap(sbc_base);
#endif
#endif  
		}
	}
}

#ifdef CONFIG_HOTPLUG_CPU

static inline void cpu_enter_lowpower(void)
{
	unsigned int v;

	flush_cache_louis();
	asm volatile(
	"	mcr	p15, 0, %1, c7, c5, 0\n"
	"	dsb\n"
	 
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, #0x40\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, %2\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (v)
	  : "r" (0), "Ir" (CR_C)
	  : "cc");
}

static inline void cpu_leave_lowpower(void)
{
	unsigned int v;

	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, #0x40\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	  : "=&r" (v)
	  : "Ir" (CR_C)
	  : "cc");
}

#ifdef MY_DEF_HERE
static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
#else  
static inline void platform_do_lowpower_ram(unsigned int cpu, int *spurious)
#endif  
{
	 
	for (;;) {
		dsb();
		wfi();

		if (pen_release == cpu_logical_map(cpu)) {
			 
			break;
		}

		(*spurious)++;
	}
}

#ifdef MY_DEF_HERE
#ifdef CONFIG_KEXEC
static void kexec_sleep(unsigned int cpu)
{
	unsigned long val;
	typedef void (*phys_reset_t)(unsigned long);
	phys_reset_t phys_reset;
	unsigned int rst_add;

	if (!sbc_base || !sbc_ph_base) {
		 
		if (!abap_prg_base || !abap_ph_base || sti_abap_c_size > 32)
			return;

		memcpy(abap_prg_base, sti_abap_c_start, sti_abap_c_size);
		rst_add = abap_ph_base;
	} else {
		val = readl_relaxed(sbc_base +
					DATA_OFFSET + PEN_HOLD_VAR_OFFSET_4xx);

		if (val != (~0))
			return;

		rst_add = sbc_ph_base +
				DATA_OFFSET + PEN_HOLD_VAR_OFFSET_4xx + 4 + 1;
	}

	setup_mm_for_reboot();  
	flush_cache_all();
	cpu_proc_fin();
	flush_cache_all();
	phys_reset = (phys_reset_t)virt_to_phys(cpu_reset);
	phys_reset(rst_add);
	 
}
#endif
#else  
static void platform_do_lowpower_lpm(unsigned int cpu)
{
	unsigned long val;

	typedef void (*phys_reset_t)(unsigned long);
	phys_reset_t phys_reset;

	if (!sbc_base || !sbc_ph_base)
		return;

	val = readl_relaxed(sbc_base + DATA_OFFSET + PEN_HOLD_VAR_OFFSET_4xx);

	if (val != (~0))
		return;

	setup_mm_for_reboot();  
	flush_cache_all();
	cpu_proc_fin();   
	flush_cache_all();
	phys_reset = (phys_reset_t)virt_to_phys(cpu_reset);
	phys_reset(sbc_ph_base + DATA_OFFSET + PEN_HOLD_VAR_OFFSET_4xx + 4 + 1);
	 
}
#endif  

void __ref sti_cpu_die(unsigned int cpu)
{
	int spurious = 0;

#ifdef MY_DEF_HERE
	 
#ifdef CONFIG_KEXEC
	if (kexec_case) {
		kexec_sleep(cpu);
		pr_err("kexec not handled, core%d failed to sleep\n", cpu);
		kexec_case = 0;
		BUG();
	}
#endif

	cpu_enter_lowpower();

	platform_do_lowpower(cpu, &spurious);
#else  
	 
	platform_do_lowpower_lpm(cpu);

	cpu_enter_lowpower();

	platform_do_lowpower_ram(cpu, &spurious);
#endif  

	cpu_leave_lowpower();

	if (spurious)
		pr_warn("CPU%u: %u spurious wakeup calls\n", cpu, spurious);
}

#endif  

struct smp_operations __initdata sti_smp_ops = {
	.smp_prepare_cpus	= sti_smp_prepare_cpus,
	.smp_secondary_init	= sti_secondary_init,
	.smp_boot_secondary	= sti_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= sti_cpu_die,
#endif
};
