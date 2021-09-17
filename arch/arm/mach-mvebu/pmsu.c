#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#if defined(MY_ABC_HERE)
#define pr_fmt(fmt) "mvebu-pmsu: " fmt

#include <linux/clk.h>
#include <linux/cpu_pm.h>
#include <linux/cpu.h>
#if defined(MY_ABC_HERE)
#include <linux/cpufreq-dt.h>
#endif  
#include <linux/delay.h>
#endif  
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#if defined(MY_ABC_HERE)
#include <linux/of_device.h>
#endif  
#include <linux/io.h>
#if defined(MY_ABC_HERE)
#include <linux/kernel.h>
#include <linux/mbus.h>
#include <linux/mvebu-v7-cpuidle.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#if defined(MY_ABC_HERE)
#include <linux/pm_opp.h>
#else  
#include <linux/opp.h>
#endif  
#endif  
#include <linux/smp.h>
#if defined(MY_ABC_HERE)
#include <linux/resource.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/cp15.h>
#include <asm/smp_scu.h>
#endif  
#include <asm/smp_plat.h>
#if defined(MY_ABC_HERE)
#include <asm/suspend.h>
#include <asm/tlbflush.h>
#include "common.h"
#include "armada-370-xp.h"

#define PMSU_BASE_OFFSET    0x100
#define PMSU_REG_SIZE	    0x1000

#define PMSU_CONTROL_AND_CONFIG(cpu)	    ((cpu * 0x100) + 0x104)
#define PMSU_CONTROL_AND_CONFIG_DFS_REQ		BIT(18)
#define PMSU_CONTROL_AND_CONFIG_PWDDN_REQ	BIT(16)
#define PMSU_CONTROL_AND_CONFIG_L2_PWDDN	BIT(20)

#define PMSU_CPU_POWER_DOWN_CONTROL(cpu)    ((cpu * 0x100) + 0x108)

#define PMSU_CPU_POWER_DOWN_DIS_SNP_Q_SKIP	BIT(0)

#define PMSU_STATUS_AND_MASK(cpu)	    ((cpu * 0x100) + 0x10c)
#define PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT	BIT(16)
#define PMSU_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT	BIT(17)
#define PMSU_STATUS_AND_MASK_IRQ_WAKEUP		BIT(20)
#define PMSU_STATUS_AND_MASK_FIQ_WAKEUP		BIT(21)
#define PMSU_STATUS_AND_MASK_DBG_WAKEUP		BIT(22)
#define PMSU_STATUS_AND_MASK_IRQ_MASK		BIT(24)
#define PMSU_STATUS_AND_MASK_FIQ_MASK		BIT(25)

#define PMSU_EVENT_STATUS_AND_MASK(cpu)     ((cpu * 0x100) + 0x120)
#define PMSU_EVENT_STATUS_AND_MASK_DFS_DONE        BIT(1)
#define PMSU_EVENT_STATUS_AND_MASK_DFS_DONE_MASK   BIT(17)

#define PMSU_BOOT_ADDR_REDIRECT_OFFSET(cpu) ((cpu * 0x100) + 0x124)

#define L2C_NFABRIC_PM_CTL		    0x4
#define L2C_NFABRIC_PM_CTL_PWR_DOWN		BIT(20)

#define PMSU_POWERDOWN_DELAY		    0xF04
#define PMSU_POWERDOWN_DELAY_PMU		BIT(1)
#define PMSU_POWERDOWN_DELAY_MASK		0xFFFE
#define PMSU_DFLT_ARMADA38X_DELAY	        0x64

#define MPCORE_RESET_CTL		    0x64
#define MPCORE_RESET_CTL_L2			BIT(0)
#define MPCORE_RESET_CTL_DEBUG			BIT(16)

#define SRAM_PHYS_BASE  0xFFFF0000
#define BOOTROM_BASE    0xFFF00000
#define BOOTROM_SIZE    0x100000

#define ARMADA_370_CRYPT0_ENG_TARGET   0x9
#define ARMADA_370_CRYPT0_ENG_ATTR     0x1

extern void ll_disable_coherency(void);
extern void ll_enable_coherency(void);

extern void armada_370_xp_cpu_resume(void);
extern void armada_38x_cpu_resume(void);
#if defined(MY_ABC_HERE)
 
#else  
extern struct clk *get_cpu_clk(int cpu);
#endif  

void __iomem *scu_base;

static phys_addr_t pmsu_mp_phys_base;
#endif  
static void __iomem *pmsu_mp_base;
#if defined(MY_ABC_HERE)

static void *mvebu_cpu_resume;
static int (*mvebu_pmsu_dfs_request_ptr)(int cpu);

static void __iomem *sram_wa_virt_base[2];
#else  
static void __iomem *pmsu_reset_base;

#define PMSU_BOOT_ADDR_REDIRECT_OFFSET(cpu)	((cpu * 0x100) + 0x24)
#define PMSU_RESET_CTL_OFFSET(cpu)		(cpu * 0x8)
#endif  

static struct of_device_id of_pmsu_table[] = {
#if defined(MY_ABC_HERE)
	{ .compatible = "marvell,armada-370-pmsu", },
	{ .compatible = "marvell,armada-370-xp-pmsu", },
	{ .compatible = "marvell,armada-380-pmsu", },
#else  
	{.compatible = "marvell,armada-370-xp-pmsu"},
#endif  
	{   },
};

#if defined(MY_ABC_HERE)
void mvebu_pmsu_set_cpu_boot_addr(int hw_cpu, void *boot_addr)
{
	writel(virt_to_phys(boot_addr), pmsu_mp_base +
		PMSU_BOOT_ADDR_REDIRECT_OFFSET(hw_cpu));
}

extern unsigned char mvebu_boot_wa_start;
extern unsigned char mvebu_boot_wa_end;
extern long sleep_save_sp[CONFIG_NR_CPUS];
 
int mvebu_setup_boot_addr_wa(unsigned int crypto_eng_target,
			     unsigned int crypto_eng_attribute,
			     phys_addr_t resume_addr_reg)
{
	void __iomem *sram_virt_base;
	u32 code_len = &mvebu_boot_wa_end - &mvebu_boot_wa_start;

	mvebu_mbus_del_window(BOOTROM_BASE, BOOTROM_SIZE);
	mvebu_mbus_add_window_by_id(crypto_eng_target, crypto_eng_attribute,
				    SRAM_PHYS_BASE, SZ_64K);

	sram_virt_base = ioremap(SRAM_PHYS_BASE, SZ_64K);
	if (!sram_virt_base) {
		pr_err("Unable to map SRAM to setup the boot address WA\n");
		return -ENOMEM;
	}

	memcpy(sram_virt_base, &mvebu_boot_wa_start, code_len);

	__raw_writel((unsigned long)resume_addr_reg,
		     sram_virt_base + code_len - 4);

	iounmap(sram_virt_base);

	return 0;
}

int armada_38x_cpuidle_wa(void)
{
	u32 *tmp;
	unsigned int hw_cpu = cpu_logical_map(smp_processor_id());

	tmp = sram_wa_virt_base[hw_cpu];
	*tmp = virt_to_phys(&sleep_save_sp[hw_cpu]);	 
	tmp++;
	*tmp = sleep_save_sp[hw_cpu];			 
	tmp++;
	*tmp = 0x2C;					 
	tmp++;
#if defined(MY_ABC_HERE)
	memcpy(tmp, (void *)(sleep_save_sp[hw_cpu] + 0xC0000000), 0x2C);  
	return 0;
#else  
	memcpy(tmp, sleep_save_sp[hw_cpu] + 0xC0000000, 0x2C);  
#endif  
}

static int __init mvebu_v7_pmsu_init(void)
{
	struct device_node *np;
	struct resource res;
	int ret = 0;

	np = of_find_matching_node(NULL, of_pmsu_table);
	if (!np)
		return 0;

	pr_info("Initializing Power Management Service Unit\n");

	if (of_address_to_resource(np, 0, &res)) {
		pr_err("unable to get resource\n");
		ret = -ENOENT;
		goto out;
	}

	if (of_device_is_compatible(np, "marvell,armada-370-xp-pmsu")) {
		pr_warn(FW_WARN "deprecated pmsu binding\n");
		res.start = res.start - PMSU_BASE_OFFSET;
		res.end = res.start + PMSU_REG_SIZE - 1;
	}

	if (!request_mem_region(res.start, resource_size(&res),
				np->full_name)) {
		pr_err("unable to request region\n");
		ret = -EBUSY;
		goto out;
	}

	pmsu_mp_phys_base = res.start;

	pmsu_mp_base = ioremap(res.start, resource_size(&res));
	if (!pmsu_mp_base) {
		pr_err("unable to map registers\n");
		release_mem_region(res.start, resource_size(&res));
		ret = -ENOMEM;
		goto out;
	}

 out:
	of_node_put(np);
	return ret;
}

static void mvebu_v7_pmsu_enable_l2_powerdown_onidle(void)
{
	u32 reg;

	if (pmsu_mp_base == NULL)
		return;

	reg = readl(pmsu_mp_base + L2C_NFABRIC_PM_CTL);
	reg |= L2C_NFABRIC_PM_CTL_PWR_DOWN;
	writel(reg, pmsu_mp_base + L2C_NFABRIC_PM_CTL);
}

enum pmsu_idle_prepare_flags {
	PMSU_PREPARE_NORMAL = 0,
	PMSU_PREPARE_DEEP_IDLE = BIT(0),
	PMSU_PREPARE_SNOOP_DISABLE = BIT(1),
};

static int mvebu_v7_pmsu_idle_prepare(unsigned long flags)
{
	unsigned int hw_cpu = cpu_logical_map(smp_processor_id());
	u32 reg;

	if (pmsu_mp_base == NULL)
		return -EINVAL;

	reg = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));
	reg |= PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT    |
	       PMSU_STATUS_AND_MASK_IRQ_WAKEUP       |
	       PMSU_STATUS_AND_MASK_FIQ_WAKEUP       |
	       PMSU_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT |
	       PMSU_STATUS_AND_MASK_IRQ_MASK         |
	       PMSU_STATUS_AND_MASK_FIQ_MASK;
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));

	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
	 
	if (flags & PMSU_PREPARE_DEEP_IDLE)
		reg |= PMSU_CONTROL_AND_CONFIG_L2_PWDDN;

	reg |= PMSU_CONTROL_AND_CONFIG_PWDDN_REQ;
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));

	if (flags & PMSU_PREPARE_SNOOP_DISABLE) {
		 
		reg = readl(pmsu_mp_base + PMSU_CPU_POWER_DOWN_CONTROL(hw_cpu));
		reg |= PMSU_CPU_POWER_DOWN_DIS_SNP_Q_SKIP;
		writel(reg, pmsu_mp_base + PMSU_CPU_POWER_DOWN_CONTROL(hw_cpu));
	}

	return 0;
}

int armada_370_xp_pmsu_idle_enter(unsigned long deepidle)
{
	unsigned long flags = PMSU_PREPARE_SNOOP_DISABLE;
	int ret;

	if (deepidle)
		flags |= PMSU_PREPARE_DEEP_IDLE;

	ret = mvebu_v7_pmsu_idle_prepare(flags);
	if (ret)
		return ret;

	v7_exit_coherency_flush(all);

	ll_disable_coherency();

	dsb();

	wfi();

	local_flush_tlb_all();

	ll_enable_coherency();

	asm volatile(
	"mrc	p15, 0, r0, c1, c0, 0 \n\t"
	"tst	r0, #(1 << 2) \n\t"
	"orreq	r0, r0, #(1 << 2) \n\t"
	"mcreq	p15, 0, r0, c1, c0, 0 \n\t"
	"isb	"
	: : : "r0");

	pr_debug("Failed to suspend the system\n");

	return 0;
}

static int armada_370_xp_cpu_suspend(unsigned long deepidle)
{
	return cpu_suspend(deepidle, armada_370_xp_pmsu_idle_enter);
}

int armada_38x_do_cpu_suspend(unsigned long deepidle)
{
	unsigned long flags = 0;

	armada_38x_cpuidle_wa();

	if (deepidle)
		flags |= PMSU_PREPARE_DEEP_IDLE;

	mvebu_v7_pmsu_idle_prepare(flags);
	 
	v7_exit_coherency_flush(louis);

	scu_power_mode(scu_base, SCU_PM_POWEROFF);

	cpu_do_idle();

	return 1;
}

static int armada_38x_cpu_suspend(unsigned long deepidle)
{
	return cpu_suspend(false, armada_38x_do_cpu_suspend);
}

void mvebu_v7_pmsu_idle_exit(void)
{
	unsigned int hw_cpu = cpu_logical_map(smp_processor_id());
	u32 reg;

	if (pmsu_mp_base == NULL)
		return;
	 
	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
	reg &= ~PMSU_CONTROL_AND_CONFIG_L2_PWDDN;
#if defined(MY_ABC_HERE)
	 
	reg |= (1 << 4);
#endif  
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));

	reg = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));
	reg &= ~(PMSU_STATUS_AND_MASK_IRQ_WAKEUP | PMSU_STATUS_AND_MASK_FIQ_WAKEUP);
	reg &= ~PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT;
	reg &= ~PMSU_STATUS_AND_MASK_SNP_Q_EMPTY_WAIT;
	reg &= ~(PMSU_STATUS_AND_MASK_IRQ_MASK | PMSU_STATUS_AND_MASK_FIQ_MASK);
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(hw_cpu));
}

#if defined(MY_ABC_HERE)
void mvebu_v7_pmsu_disable_dfs_cpu(int hw_cpu)
{
	u32 reg;
	if (pmsu_mp_base == NULL)
		return;
	 
	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
	reg &= ~(0xf << 4);
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(hw_cpu));
}
#endif  

static int mvebu_v7_cpu_pm_notify(struct notifier_block *self,
				    unsigned long action, void *hcpu)
{
	if (action == CPU_PM_ENTER) {
		unsigned int hw_cpu = cpu_logical_map(smp_processor_id());
		mvebu_pmsu_set_cpu_boot_addr(hw_cpu, mvebu_cpu_resume);
	} else if (action == CPU_PM_EXIT) {
		mvebu_v7_pmsu_idle_exit();
	}

	return NOTIFY_OK;
}

static struct notifier_block mvebu_v7_cpu_pm_notifier = {
	.notifier_call = mvebu_v7_cpu_pm_notify,
};

static struct mvebu_v7_cpuidle armada_370_cpuidle = {
	.type = CPUIDLE_ARMADA_370,
	.cpu_suspend = armada_370_xp_cpu_suspend,
};

static struct mvebu_v7_cpuidle armada_38x_cpuidle = {
	.type = CPUIDLE_ARMADA_38X,
	.cpu_suspend = armada_38x_cpu_suspend,
};

static struct mvebu_v7_cpuidle armada_xp_cpuidle = {
	.type = CPUIDLE_ARMADA_XP,
	.cpu_suspend = armada_370_xp_cpu_suspend,
};

static struct platform_device mvebu_v7_cpuidle_device = {
	.name = "cpuidle-mvebu-v7",
};

static __init int armada_370_cpuidle_init(void)
{
	struct device_node *np;
	phys_addr_t redirect_reg;

	np = of_find_compatible_node(NULL, NULL, "marvell,coherency-fabric");
	if (!np)
		return -ENODEV;
	of_node_put(np);

	redirect_reg = pmsu_mp_phys_base + PMSU_BOOT_ADDR_REDIRECT_OFFSET(0);
	mvebu_setup_boot_addr_wa(ARMADA_370_CRYPT0_ENG_TARGET,
				 ARMADA_370_CRYPT0_ENG_ATTR,
				 redirect_reg);

	mvebu_cpu_resume = armada_370_xp_cpu_resume;
	mvebu_v7_cpuidle_device.dev.platform_data = &armada_370_cpuidle;

	return 0;
}

int armada_38x_cpuidle_init(void)
{
	struct device_node *np;
	void __iomem *mpsoc_base;
	u32 reg;

	np = of_find_compatible_node(NULL, NULL,
				     "marvell,armada-380-coherency-fabric");
	if (!np)
		return -ENODEV;
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL,
				     "marvell,armada-380-mpcore-soc-ctrl");
	if (!np)
		return -ENODEV;
	mpsoc_base = of_iomap(np, 0);
	BUG_ON(!mpsoc_base);
	of_node_put(np);

	reg = readl(mpsoc_base + MPCORE_RESET_CTL);
	reg |= MPCORE_RESET_CTL_L2;
	reg |= MPCORE_RESET_CTL_DEBUG;
	writel(reg, mpsoc_base + MPCORE_RESET_CTL);
	iounmap(mpsoc_base);

	reg = readl(pmsu_mp_base + PMSU_POWERDOWN_DELAY);
	reg &= ~PMSU_POWERDOWN_DELAY_MASK;
	reg |= PMSU_DFLT_ARMADA38X_DELAY;
	reg |= PMSU_POWERDOWN_DELAY_PMU;
	writel(reg, pmsu_mp_base + PMSU_POWERDOWN_DELAY);

	mvebu_cpu_resume = armada_38x_cpu_resume;
	mvebu_v7_cpuidle_device.dev.platform_data = &armada_38x_cpuidle;

	sram_wa_virt_base[0] = ioremap(0xf1100000, SZ_64);
	sram_wa_virt_base[1] = ioremap(0xf1110000, SZ_64);

#if 0
	outer_cache.clean_range = NULL;
#endif
	return 0;
}

static __init int armada_xp_cpuidle_init(void)
{
	struct device_node *np;
	np = of_find_compatible_node(NULL, NULL, "marvell,coherency-fabric");
	if (!np)
		return -ENODEV;
	of_node_put(np);

	mvebu_cpu_resume = armada_370_xp_cpu_resume;
	mvebu_v7_cpuidle_device.dev.platform_data = &armada_xp_cpuidle;

	return 0;
}

static int __init mvebu_v7_cpu_pm_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_matching_node(NULL, of_pmsu_table);
	if (!np)
		return 0;
	of_node_put(np);

	if (of_machine_is_compatible("marvell,armadaxp"))
		ret = armada_xp_cpuidle_init();
	else if (of_machine_is_compatible("marvell,armada370"))
		ret = armada_370_cpuidle_init();
	else if (of_machine_is_compatible("marvell,armada38x"))
		ret = armada_38x_cpuidle_init();
	else
		return 0;

	if (ret)
		return ret;

	mvebu_v7_pmsu_enable_l2_powerdown_onidle();
	platform_device_register(&mvebu_v7_cpuidle_device);
	cpu_pm_register_notifier(&mvebu_v7_cpu_pm_notifier);

	return 0;
}

arch_initcall(mvebu_v7_cpu_pm_init);
early_initcall(mvebu_v7_pmsu_init);

static void mvebu_pmsu_dfs_request_local(void *data)
{
	u32 reg;
	u32 cpu = smp_processor_id();
	unsigned long flags;

	local_irq_save(flags);

	reg = readl(pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));
	reg &= ~PMSU_EVENT_STATUS_AND_MASK_DFS_DONE;
	reg |= PMSU_EVENT_STATUS_AND_MASK_DFS_DONE_MASK;
	writel(reg, pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));

	reg = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));
	reg |= PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT |
	       PMSU_STATUS_AND_MASK_IRQ_MASK     |
	       PMSU_STATUS_AND_MASK_FIQ_MASK;
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));

	reg = readl(pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(cpu));
	reg |= PMSU_CONTROL_AND_CONFIG_DFS_REQ;
	writel(reg, pmsu_mp_base + PMSU_CONTROL_AND_CONFIG(cpu));

	wfi();

	reg = readl(pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));
	reg &= ~PMSU_STATUS_AND_MASK_CPU_IDLE_WAIT;
	writel(reg, pmsu_mp_base + PMSU_STATUS_AND_MASK(cpu));

	reg = readl(pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));
	reg &= ~PMSU_EVENT_STATUS_AND_MASK_DFS_DONE_MASK;
	writel(reg, pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(cpu));

	local_irq_restore(flags);
}

int armada_xp_pmsu_dfs_request(int cpu)
{
	unsigned long timeout;
	int hwcpu = cpu_logical_map(cpu);
	u32 reg;

	smp_call_function_single(cpu, mvebu_pmsu_dfs_request_local,
				 NULL, false);

	timeout = jiffies + HZ;
	while (time_before(jiffies, timeout)) {
		reg = readl(pmsu_mp_base + PMSU_EVENT_STATUS_AND_MASK(hwcpu));
		if (reg & PMSU_EVENT_STATUS_AND_MASK_DFS_DONE)
			break;
		udelay(10);
	}

	if (time_after(jiffies, timeout))
		return -ETIME;

	return 0;
}

int armada_380_pmsu_dfs_request(int cpu)
{
#if defined(MY_ABC_HERE)
	 
	cpu_hotplug_disable();
#endif  

	on_each_cpu(mvebu_pmsu_dfs_request_local,
				 NULL, false);

#if defined(MY_ABC_HERE)
	cpu_hotplug_enable();
#endif  

	return 0;
}

int mvebu_pmsu_dfs_request(int cpu)
{
	return mvebu_pmsu_dfs_request_ptr(cpu);
}

#if defined(MY_ABC_HERE)
struct cpufreq_dt_platform_data armada_xp_cpufreq_dt_pd = {
	.independent_clocks = true,
};

struct cpufreq_dt_platform_data armada_380_cpufreq_dt_pd = {
	.independent_clocks = false,
};
#endif  

static int mvebu_v7_pmsu_register_cpufreq(int cpu)
{
	struct device *cpu_dev;
	struct clk *clk;
	int ret;
#if defined(MY_ABC_HERE)
	struct device_node *np;
#endif  

#if defined(MY_ABC_HERE)
	for_each_child_of_node(of_find_node_by_path("/cpus"), np)
		if (of_get_property(np, "clock-names", NULL))
			break;

	if (!np) {
		pr_err("failed to find cpufreq node\n");
		return -ENOENT;
	}
#endif  

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		pr_err("Cannot get CPU %d\n", cpu);
		return 0;
	}

#if defined(MY_ABC_HERE)
	cpu_dev->of_node = np;
#endif  

	clk = clk_get(cpu_dev, 0);
	if (!clk) {
		pr_err("Cannot get clock for CPU %d\n", cpu);
		return -ENODEV;
	}

#if defined(MY_ABC_HERE)
		ret = dev_pm_opp_add(cpu_dev, clk_get_rate(clk), 0);
#else  
		ret = opp_add(cpu_dev, clk_get_rate(clk), 0);
#endif  
	if (ret) {
		clk_put(clk);
		return ret;
	}

#if defined(MY_ABC_HERE)
		ret = dev_pm_opp_add(cpu_dev, clk_get_rate(clk) / 2, 0);
#else  
		ret = opp_add(cpu_dev, clk_get_rate(clk) / 2, 0);
#endif  
	if (ret) {
		clk_put(clk);
		return ret;
	}

	return 0;

}

static int __init mvebu_v7_pmsu_cpufreq_init(void)
{
	struct device_node *np;
	struct resource res;
	int ret, cpu;

	np = of_find_compatible_node(NULL, NULL, "marvell,armada-xp-cpu-clock");
	if (!np)
		return 0;

	ret = of_address_to_resource(np, 1, &res);
	if (ret) {
		pr_warn(FW_WARN "not enabling cpufreq, deprecated armada-xp-cpu-clock binding\n");
		of_node_put(np);
		return 0;
	}

	of_node_put(np);

	for_each_possible_cpu(cpu) {
		ret = mvebu_v7_pmsu_register_cpufreq(cpu);
		if (ret)
			return ret;
	}

#if defined(MY_ABC_HERE)
	if (of_machine_is_compatible("marvell,armada38x")) {
		if (num_online_cpus() == 1)
			mvebu_v7_pmsu_disable_dfs_cpu(1);
		mvebu_pmsu_dfs_request_ptr = armada_380_pmsu_dfs_request;
		platform_device_register_data(NULL, "cpufreq-dt", -1,
					&armada_380_cpufreq_dt_pd, sizeof(armada_380_cpufreq_dt_pd));
	} else if (of_machine_is_compatible("marvell,armadaxp")) {
		mvebu_pmsu_dfs_request_ptr = armada_xp_pmsu_dfs_request;
		platform_device_register_data(NULL, "cpufreq-dt", -1,
					&armada_xp_cpufreq_dt_pd, sizeof(armada_xp_cpufreq_dt_pd));

	} else
		return 0;
#endif  

	return 0;
}

device_initcall(mvebu_v7_pmsu_cpufreq_init);
#else  
#ifdef CONFIG_SMP
int armada_xp_boot_cpu(unsigned int cpu_id, void *boot_addr)
{
	int reg, hw_cpu;

	if (!pmsu_mp_base || !pmsu_reset_base) {
		pr_warn("Can't boot CPU. PMSU is uninitialized\n");
		return 1;
	}

	hw_cpu = cpu_logical_map(cpu_id);

	writel(virt_to_phys(boot_addr), pmsu_mp_base +
			PMSU_BOOT_ADDR_REDIRECT_OFFSET(hw_cpu));

	reg = readl(pmsu_reset_base + PMSU_RESET_CTL_OFFSET(hw_cpu));
	reg &= (~0x1);
	writel(reg, pmsu_reset_base + PMSU_RESET_CTL_OFFSET(hw_cpu));

	return 0;
}
#endif

int __init armada_370_xp_pmsu_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_pmsu_table);
	if (np) {
		pr_info("Initializing Power Management Service Unit\n");
		pmsu_mp_base = of_iomap(np, 0);
		pmsu_reset_base = of_iomap(np, 1);
	}

	return 0;
}

early_initcall(armada_370_xp_pmsu_init);
#endif  
