#include <linux/smp.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>
#include <asm/smp_scu.h>
#include <asm/unified.h>

#include <linux/delay.h>

#include "common.h"

#define JUMP_TO_KERNEL_START_1		0xe3a00020 	/* mov	r0, #32 */
#define JUMP_TO_KERNEL_START_2		0xe590f000 	/* ldr	pc, [r0] */

/* SCU base address */
static void __iomem *scu_base;

static DEFINE_SPINLOCK(boot_lock);

void __iomem *ls1024a_get_scu_base(void)
{
	return scu_base;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init ls1024a_smp_init_cpus(void)
{
	int i, ncores;

	scu_base = (void *) COMCERTO_SCU_VADDR;
	ncores = scu_get_core_count(scu_base);

	/* sanity check */
	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

static void __init ls1024a_smp_prepare_cpus(unsigned int max_cpus)
{
	scu_enable(scu_base);
}

static int ls1024a_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * Install the comcerto_secondary_startup pointer at 0x20
	 * Physical Address
	 */
	__raw_writel(virt_to_phys(ls1024a_secondary_startup), phys_to_virt(0x20));
	__raw_writel((unsigned int)JUMP_TO_KERNEL_START_1 , phys_to_virt(0x00));
	__raw_writel((unsigned int)JUMP_TO_KERNEL_START_2 , phys_to_virt(0x04));
	smp_wmb();
	__cpuc_flush_dcache_area((void *)phys_to_virt(0x00), 0x24);
	outer_clean_range(__pa(phys_to_virt(0x00)), __pa(phys_to_virt(0x24)));

	/* Get CPU 1 out of reset */
	__raw_writel((__raw_readl(APB_VADDR(A9DP_CPU_RESET)) & ~CPU1_RST), APB_VADDR(A9DP_CPU_RESET));
	__raw_writel((__raw_readl(APB_VADDR(A9DP_PWR_CNTRL)) & ~CLAMP_CORE1), APB_VADDR(A9DP_PWR_CNTRL));
	__raw_writel((__raw_readl(APB_VADDR(A9DP_CPU_CLK_CNTRL)) | CPU1_CLK_ENABLE), APB_VADDR(A9DP_CPU_CLK_CNTRL));

#ifdef CONFIG_NEON
	/* Get NEON 1 out of reset */
	__raw_writel((__raw_readl(APB_VADDR(A9DP_CPU_RESET)) & ~NEON1_RST), APB_VADDR(A9DP_CPU_RESET));
	__raw_writel((__raw_readl(APB_VADDR(A9DP_CPU_CLK_CNTRL)) | NEON1_CLK_ENABLE), APB_VADDR(A9DP_CPU_CLK_CNTRL));
#endif

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return 0;
}

struct smp_operations ls1024a_smp_ops __initdata = {
	.smp_init_cpus		= ls1024a_smp_init_cpus,
	.smp_prepare_cpus	= ls1024a_smp_prepare_cpus,
	//.smp_secondary_init	= ls1024a_secondary_init,
	.smp_boot_secondary	= ls1024a_boot_secondary,
#if 0
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= ls1024a_cpu_die,
#endif
#endif
};

CPU_METHOD_OF_DECLARE(fsl_ls1024a_smp, "fsl,ls1024a-smp", &ls1024a_smp_ops);
