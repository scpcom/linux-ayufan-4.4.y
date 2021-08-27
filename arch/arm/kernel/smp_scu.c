#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/io.h>

#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>

#define SCU_CTRL		0x00
#define SCU_CONFIG		0x04
#define SCU_CPU_STATUS		0x08
#define SCU_INVALIDATE		0x0c
#define SCU_FPGA_REVISION	0x10

#ifdef CONFIG_SMP
 
unsigned int __init scu_get_core_count(void __iomem *scu_base)
{
#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	unsigned int ncores = readl_relaxed(scu_base + SCU_CONFIG);
#else
	unsigned int ncores = __raw_readl(scu_base + SCU_CONFIG);
#endif
	return (ncores & 0x03) + 1;
}
#endif

#if defined(CONFIG_SMP) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) 
 
void scu_enable(void __iomem *scu_base)
{
	u32 scu_ctrl;

#ifdef CONFIG_ARM_ERRATA_764369
	 
	if ((read_cpuid(CPUID_ID) & 0xff0ffff0) == 0x410fc090) {
#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
		scu_ctrl = readl_relaxed(scu_base + 0x30);
#else
		scu_ctrl = __raw_readl(scu_base + 0x30);
#endif
		if (!(scu_ctrl & 1))
#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
			writel_relaxed(scu_ctrl | 0x1, scu_base + 0x30);
#else
			__raw_writel(scu_ctrl | 0x1, scu_base + 0x30);
#endif
	}
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	scu_ctrl = readl_relaxed(scu_base + SCU_CTRL);
#else
	scu_ctrl = __raw_readl(scu_base + SCU_CTRL);
#endif
	 
	if (scu_ctrl & 1)
		return;

#if (defined(MY_ABC_HERE) && defined(CONFIG_SCU_SPECULATIVE_LINE_FILLS)) || \
     (defined(CONFIG_SYNO_ARMADA_ARCH_V2) && defined(CONFIG_SCU_SPECULATIVE_LINEFILLS_ENABLE))
	scu_ctrl |= (1 << 3);
#endif

	scu_ctrl |= 1;
#if defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	writel_relaxed(scu_ctrl, scu_base + SCU_CTRL);
#else
	__raw_writel(scu_ctrl, scu_base + SCU_CTRL);
#endif

	flush_cache_all();
}
#endif

int scu_power_mode(void __iomem *scu_base, unsigned int mode)
{
	unsigned int val;
	int cpu = smp_processor_id();

	if (mode > 3 || mode == 1 || cpu > 3)
		return -EINVAL;

	val = __raw_readb(scu_base + SCU_CPU_STATUS + cpu) & ~0x03;
	val |= mode;
	__raw_writeb(val, scu_base + SCU_CPU_STATUS + cpu);

	return 0;
}
