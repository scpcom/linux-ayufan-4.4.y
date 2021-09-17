#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/io.h>
#if defined(MY_ABC_HERE)
#include <linux/clk.h>
#endif  
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/irq.h>
#include <asm/smp_scu.h>
#include "armada-380.h"
#include "common.h"

#include "pmsu.h"

#if defined(MY_ABC_HERE)
static struct clk *__init get_cpu_clk(int cpu)
{
	struct clk *cpu_clk;
	struct device_node *np = of_get_cpu_node(cpu, NULL);

	if (WARN(!np, "missing cpu node\n"))
		return NULL;

	cpu_clk = of_clk_get(np, 0);

	if (WARN_ON(IS_ERR(cpu_clk)))
		return NULL;

	return cpu_clk;
}

static void __init set_secondary_cpus_clock(void)
{
	int thiscpu;
	struct clk *cpu_clk;

	thiscpu = smp_processor_id();
	cpu_clk = get_cpu_clk(thiscpu);
	if (!cpu_clk)
		return;
	clk_prepare_enable(cpu_clk);
}
#endif  

extern void a380_secondary_startup(void);
static struct notifier_block armada_380_secondary_cpu_notifier;

static int __cpuinit armada_380_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	mvebu_pmsu_set_cpu_boot_addr(cpu, a380_secondary_startup);
	mvebu_boot_cpu(cpu);
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));
	return 0;
}

static void __init armada_380_smp_init_cpus(void)
{
	struct device_node *np;
	unsigned int i, ncores;

	np = of_find_node_by_name(NULL, "cpus");
	if (!np)
		panic("No 'cpus' node found\n");

	ncores = of_get_child_count(np);
	if (ncores == 0 || ncores > ARMADA_380_MAX_CPUS)
		panic("Invalid number of CPUs in DT\n");

	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %d CPUs physically present. Only %d configured.",
			ncores, nr_cpu_ids);
		pr_warn("Clipping CPU count to %d\n", nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; ++i)
		set_cpu_possible(i, true);
}

static void __init armada_380_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;

	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	register_cpu_notifier(&armada_380_secondary_cpu_notifier);

#if defined(MY_ABC_HERE)
	set_secondary_cpus_clock();
#endif  
}

#if defined(MY_ABC_HERE)
 
static void armada_38x_secondary_init(unsigned int cpu)
{
	mvebu_v7_pmsu_idle_exit();
}
#endif  

#ifdef CONFIG_HOTPLUG_CPU
static void armada_38x_cpu_die(unsigned int cpu)
{
	 
	armada_38x_do_cpu_suspend(true);
}

#if defined(MY_ABC_HERE)
 
static int armada_38x_cpu_kill(unsigned int cpu)
{
	return 1;
}
#endif  
#endif

struct smp_operations armada_380_smp_ops __initdata = {
	.smp_init_cpus		= armada_380_smp_init_cpus,
	.smp_prepare_cpus	= armada_380_smp_prepare_cpus,
	.smp_boot_secondary	= armada_380_boot_secondary,
#if defined(MY_ABC_HERE)
	.smp_secondary_init     = armada_38x_secondary_init,
#endif  
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= armada_38x_cpu_die,
#if defined(MY_ABC_HERE)
	.cpu_kill               = armada_38x_cpu_kill,
#endif  
#endif
};

static int __cpuinit armada_380_secondary_init(struct notifier_block *nfb,
					       unsigned long action, void *hcpu)
{
	struct irq_data *irqd;

	if (action == CPU_STARTING || action == CPU_STARTING_FROZEN) {
		irqd = irq_get_irq_data(IRQ_PRIV_MPIC_PPI_IRQ);
		if (irqd && irqd->chip && irqd->chip->irq_unmask)
			irqd->chip->irq_unmask(irqd);
	}

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata armada_380_secondary_cpu_notifier = {
	.notifier_call = armada_380_secondary_init,
	.priority = INT_MIN,
};
