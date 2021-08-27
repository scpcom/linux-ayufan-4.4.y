/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <asm/smp_plat.h>
#include <linux/io.h>

#include <asm/unified.h>
#include <asm/smp_scu.h>

#include <linux/delay.h>
#include <linux/jiffies.h>
#include <asm/cacheflush.h>
#include <asm/hardware/gic.h>
#include "ca9x2.h"
#include "ctrlEnv/sys/mvCpuIfRegs.h"

#include "core.h"

unsigned int group_cpu_mask = ((1 << NR_CPUS) - 1);
static void __iomem *scu_base = (void __iomem *)(INTER_REGS_VIRT_BASE + A9_MPCORE_SCU);

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
{
	int i, ncores = scu_get_core_count(scu_base);

	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; ++i)
		set_cpu_possible(i, true);

	set_smp_cross_call(gic_raise_softirq);
}

void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	scu_enable(scu_base);

}

void __cpuinit platform_secondary_init(unsigned int cpu)
{
	struct irq_data *irqd;
	gic_secondary_init(0);

	/*
	 * Unmask SOC Private Peripheral Interrupt here as it
	 * cannot be enabled from another CPU.
	 */
	irqd = irq_get_irq_data(IRQ_PRIV_MPIC_PPI_IRQ);
	if (irqd && irqd->chip && irqd->chip->irq_unmask)
		irqd->chip->irq_unmask(irqd);
}

int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	/* Open windows to bootROM - need to fix */
	writel(0xf1d11, 0x20098 + INTER_REGS_VIRT_BASE);
	writel(0xfff00000, 0x2009c + INTER_REGS_VIRT_BASE);

	/*
	* Write the address of secondary startup into the
	* system-wide flags register. The boot monitor waits
	* until it receives a soft interrupt, and then the
	* secondary CPU branches to this address.
	*/
	writel(virt_to_phys(a38x_secondary_startup),
	       INTER_REGS_VIRT_BASE + CPU_RESUME_ADDR_REG(cpu));
	/*
	 * Get CPU out of software reset state.
	 */
	writel(0, CPU_SOFT_RESET_REG(cpu_logical_map(cpu)) +
	       INTER_REGS_VIRT_BASE);

	return 0;
}
