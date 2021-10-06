#if defined(CONFIG_SYNO_LSP_ARMADA_16_12)
/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
 * CPU frequency scaling support for Armada 3700 platform.
#else // CONFIG_SYNO_LSP_ARMADA_17_04_02
 * CPU frequency scaling support for Armada-3700 platform.
#endif // CONFIG_SYNO_LSP_ARMADA_17_04_02
 *
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
 * Copyright (C) 2017 Marvell
#else // CONFIG_SYNO_LSP_ARMADA_17_04_02
 * Copyright (C) 2016 Marvell
#endif // CONFIG_SYNO_LSP_ARMADA_17_04_02
 *
 * Victor Gu <xigu@marvell.com>
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
 * Evan Wang <xswang@marvell.com>
#endif // CONFIG_SYNO_LSP_ARMADA_17_04_02
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
/* North bridge PM configuration registers */
#define MVEBU_PM_CONTROL_REG		(0)
#define  MVEBU_PM_DVFS_INT_SHIFT	(13)
#define MVEBU_PM_NB_L0_L1_CONFIG_REG	(0x18)
#define MVEBU_PM_NB_L2_L3_CONFIG_REG	(0x1C)
#define  MVEBU_PM_NB_TBG_DIV_LX_OFF	(13)
#define  MVEBU_PM_NB_TBG_DIV_LX_MASK	(0x7)
#define  MVEBU_PM_NB_LX_CONFIG_SHIFT	(16)
#define MVEBU_PM_NB_DYNAMIC_MODE_REG	(0x24)
#define  MVEBU_PM_NB_DFS_EN_OFF		(31)
#define  MVEBU_PM_NB_VDD_EN_OFF		(30)
#define  MVEBU_PM_NB_DIV_EN_OFF		(29)
#define  MVEBU_PM_NB_TBG_EN_OFF		(28)
#define  MVEBU_PM_NB_CLK_SEL_EN_OFF	(26)
#define MVEBU_PM_NB_CPU_LOAD_REG	(0x30)
#define  MVEBU_PM_NB_CPU_LOAD_OFF	(0)
#define  MVEBU_NB_CPU_LOAD_MASK		(0x3)

/* TBG divider */
enum mvebu_tbg_divider {
	MVEBU_TBG_DIVIDER_1 = 1,
	MVEBU_TBG_DIVIDER_2,
	MVEBU_TBG_DIVIDER_3,
	MVEBU_TBG_DIVIDER_4,
	MVEBU_TBG_DIVIDER_5,
	MVEBU_TBG_DIVIDER_6
};

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
/* CPU LOAD index */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
enum armada3700_cpu_load_index {
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
enum mvebu_cpu_load_index {
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	MVEBU_CPU_LOAD_0 = 0,
	MVEBU_CPU_LOAD_1,
	MVEBU_CPU_LOAD_2,
	MVEBU_CPU_LOAD_3,
	MVEBU_CPU_LOAD_MAX
};

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
#define MHZ_TO_HZ 1000000
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
#define CPUFREQ_NAME		"armada3700_dvfs"
#define DEF_TRANS_LATENCY	1000
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
struct a3700_cpu_freq_div_info {
	u32 cpu_freq_max;/* MHz */
	u8 divider[MVEBU_CPU_LOAD_MAX];
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
struct armada3700_dvfs_data {
	void __iomem *base;
	struct resource *mem;
	int irq;
	struct clk *cpu_clk;
	unsigned int latency;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_count;
	struct device *dev;
	bool dvfs_enabled;
	struct work_struct irq_work;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
};

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
static struct a3700_cpu_freq_div_info a3700_divider_info[] = {
	{.cpu_freq_max = 1200, .divider = {1, 2, 4, 6} },
	{.cpu_freq_max = 1000, .divider = {1, 2, 4, 5} },
	{.cpu_freq_max = 800,  .divider = {1, 2, 3, 4} },
	{.cpu_freq_max = 600,  .divider = {2, 4, 5, 6} },
};
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static struct armada3700_dvfs_data *dvfs_info;
static DEFINE_MUTEX(cpufreq_lock);
static struct cpufreq_freqs freqs;

static void armada3700_enable_dvfs(void)
{
	unsigned int reg;

	/* Disable dynamic CPU voltage and frequency scaling */
	reg = readl(dvfs_info->base + MVEBU_PM_NB_DYNAMIC_MODE_REG);
	reg &= ~(1 << MVEBU_PM_NB_DFS_EN_OFF);
	writel(reg, dvfs_info->base + MVEBU_PM_NB_DYNAMIC_MODE_REG);

	/* Set highest CPU Load 0 by default */
	reg = readl(dvfs_info->base + MVEBU_PM_NB_CPU_LOAD_REG);
	reg &= ~(MVEBU_NB_CPU_LOAD_MASK << MVEBU_PM_NB_CPU_LOAD_OFF);
	reg |= ((MVEBU_CPU_LOAD_0 & MVEBU_NB_CPU_LOAD_MASK) << MVEBU_PM_NB_CPU_LOAD_OFF);
	writel(reg, dvfs_info->base + MVEBU_PM_NB_CPU_LOAD_REG);

	/* Enable dynamic CPU voltage and frequency scaling */
	reg = readl(dvfs_info->base + MVEBU_PM_NB_DYNAMIC_MODE_REG);
	reg |= (1 << MVEBU_PM_NB_DFS_EN_OFF);
	reg |= (1 << MVEBU_PM_NB_VDD_EN_OFF);
	reg |= (1 << MVEBU_PM_NB_DIV_EN_OFF);
	reg |= (1 << MVEBU_PM_NB_TBG_EN_OFF);
	reg |= (1 << MVEBU_PM_NB_CLK_SEL_EN_OFF);
	writel(reg, dvfs_info->base + MVEBU_PM_NB_DYNAMIC_MODE_REG);
}

static int armada3700_target(struct cpufreq_policy *policy, unsigned int index)
{
	unsigned int reg;
	struct cpufreq_frequency_table *freq_table = dvfs_info->freq_table;

	/* Protect CPU DFS from changing the number of online cpus number during
	 * frequency transition by temporarily disable cpu hotplug
	 */
	cpu_hotplug_disable();

	freqs.old = policy->cur;
	freqs.new = freq_table[index].frequency;

	cpufreq_freq_transition_begin(policy, &freqs);

	/* Set the target frequency and voltage */
	reg = readl(dvfs_info->base + MVEBU_PM_NB_CPU_LOAD_REG);
	reg &= ~(MVEBU_NB_CPU_LOAD_MASK << MVEBU_PM_NB_CPU_LOAD_OFF);
	/* The index in CPU freq is inversed from small to big, need to convert it */
	index = MVEBU_CPU_LOAD_3 - index;
	reg |= ((index & MVEBU_NB_CPU_LOAD_MASK) << MVEBU_PM_NB_CPU_LOAD_OFF);
	writel(reg, dvfs_info->base + MVEBU_PM_NB_CPU_LOAD_REG);

	cpufreq_freq_transition_end(policy, &freqs, 0);

	cpu_hotplug_enable();

	return 0;
}
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
static struct a3700_cpu_freq_div_info *armada_3700_cpu_freq_info_get(u32 max_cpu_freq)
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static irqreturn_t armada3700_cpufreq_irq(int irq, void *id)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
{
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	int i;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	unsigned int reg;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	for (i = 0; i < ARRAY_SIZE(a3700_divider_info); i++) {
		if (max_cpu_freq == a3700_divider_info[i].cpu_freq_max)
			break;
	}
	if (i == ARRAY_SIZE(a3700_divider_info)) {
		pr_err("unsupported CPU frequency %d MHz\n", max_cpu_freq);
		return NULL;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = readl(dvfs_info->base + MVEBU_PM_CONTROL_REG);
	if (reg >> MVEBU_PM_DVFS_INT_SHIFT & 0x1) {
		writel(reg, dvfs_info->base + MVEBU_PM_CONTROL_REG);
		disable_irq_nosync(irq);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	}
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	return IRQ_HANDLED;
}

static int armada3700_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	policy->clk = dvfs_info->cpu_clk;
	return cpufreq_generic_init(policy, dvfs_info->freq_table,
			dvfs_info->latency);
}

static struct cpufreq_driver armada3700_driver = {
	.flags		= CPUFREQ_STICKY | CPUFREQ_ASYNC_NOTIFICATION |
				CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= armada3700_target,
	.get		= cpufreq_generic_get,
	.init		= armada3700_cpufreq_cpu_init,
	.name		= CPUFREQ_NAME,
	.attr		= cpufreq_generic_attr,
};

static const struct of_device_id armada3700_cpufreq_match[] = {
	{
		.compatible = "marvell,armada-3700-cpufreq",
	},
	{},
};
MODULE_DEVICE_TABLE(of, armada3700_cpufreq_match);

static void armada3700_cpufreq_divider_get(unsigned int *div_arr)
{
	unsigned int reg;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	return &a3700_divider_info[i];
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	reg = readl(dvfs_info->base + MVEBU_PM_NB_L0_L1_CONFIG_REG);
	div_arr[MVEBU_CPU_LOAD_0] = (reg >> (MVEBU_PM_NB_TBG_DIV_LX_OFF + MVEBU_PM_NB_LX_CONFIG_SHIFT)) &
					MVEBU_PM_NB_TBG_DIV_LX_MASK;
	div_arr[MVEBU_CPU_LOAD_1] = (reg >> (MVEBU_PM_NB_TBG_DIV_LX_OFF)) &
					MVEBU_PM_NB_TBG_DIV_LX_MASK;
	reg = readl(dvfs_info->base + MVEBU_PM_NB_L2_L3_CONFIG_REG);
	div_arr[MVEBU_CPU_LOAD_2] = (reg >> (MVEBU_PM_NB_TBG_DIV_LX_OFF + MVEBU_PM_NB_LX_CONFIG_SHIFT)) &
					MVEBU_PM_NB_TBG_DIV_LX_MASK;
	div_arr[MVEBU_CPU_LOAD_3] = (reg >> (MVEBU_PM_NB_TBG_DIV_LX_OFF)) &
					MVEBU_PM_NB_TBG_DIV_LX_MASK;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
static int __init armada3700_cpufreq_driver_init(void)
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static int armada3700_cpufreq_probe(struct platform_device *pdev)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
{
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct platform_device *pdev;
	struct device_node *node;
	struct a3700_cpu_freq_div_info *divider_info;
	struct device *cpu_dev;
	struct clk *clk;
	int load_level, ret;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	int ret = -EINVAL;
	struct device_node *np;
	struct resource res;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	unsigned int cur_frequency;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	const struct of_device_id *match;
	unsigned int div_arr[MVEBU_CPU_LOAD_MAX];
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	node = of_find_compatible_node(NULL, NULL, "marvell,armada-37xx-cpu-pm-clk");
	if (!node || !of_device_is_available(node))
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	match = of_match_device(armada3700_cpufreq_match, &pdev->dev);
	if (!match)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		return -ENODEV;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	/*
	 * On CPU 0 register the operating points
	 * supported (which are the nominal CPU frequency and full integer
	 * divisions of it).
	 */
	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		dev_err(cpu_dev, "Cannot get CPU\n");
		return -ENODEV;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	np = pdev->dev.of_node;
	dvfs_info = devm_kzalloc(&pdev->dev, sizeof(*dvfs_info), GFP_KERNEL);
	if (!dvfs_info) {
		ret = -ENOMEM;
		goto err_put_node;
	}

	dvfs_info->dev = &pdev->dev;

	ret = of_address_to_resource(np, 0, &res);
	if (ret)
		goto err_put_node;

	dvfs_info->base = devm_ioremap_resource(dvfs_info->dev, &res);
	if (IS_ERR(dvfs_info->base)) {
		ret = PTR_ERR(dvfs_info->base);
		goto err_put_node;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	clk = clk_get(cpu_dev, 0);
	if (IS_ERR(clk)) {
		dev_err(cpu_dev, "Cannot get clock for CPU0\n");
		return PTR_ERR(clk);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	dvfs_info->irq = irq_of_parse_and_map(np, 0);
	if (!dvfs_info->irq) {
		dev_err(dvfs_info->dev, "No cpufreq irq found\n");
		ret = -ENODEV;
		goto err_put_node;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	/* Get nominal (current) CPU frequency */
	cur_frequency = clk_get_rate(clk);
	if (!cur_frequency) {
		dev_err(cpu_dev, "Failed to get clock rate for CPU\n");
		return -EINVAL;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (of_property_read_u32(np, "clock-latency", &dvfs_info->latency))
		dvfs_info->latency = DEF_TRANS_LATENCY;

	/* Get CPU core clock */
	dvfs_info->cpu_clk = of_clk_get_by_name(np, NULL);
	if (IS_ERR(dvfs_info->cpu_clk)) {
		dev_err(dvfs_info->dev, "Failed to get cpu clock\n");
		ret = PTR_ERR(dvfs_info->cpu_clk);
		goto err_put_node;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	divider_info = armada_3700_cpu_freq_info_get(cur_frequency / MHZ_TO_HZ);
	if (!divider_info) {
		dev_err(cpu_dev, "Failed to get freq divider info for CPU\n");
		return -EINVAL;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Get MAX CPU frequency */
	cur_frequency = clk_get_rate(dvfs_info->cpu_clk);
	if (!cur_frequency) {
		dev_err(dvfs_info->dev, "Failed to get clock rate\n");
		ret = -EINVAL;
		goto err_put_node;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	}

	/*
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
// do nothing
#else // CONFIG_SYNO_LSP_ARMADA_17_04_02
	 * Get divider for all the four levels, then caculate
	 * corresponding CPU frequency and reigster to Linux.
#endif // CONFIG_SYNO_LSP_ARMADA_17_04_02
	 * In case of a failure of dev_pm_opp_add(), we don't
	 * bother with cleaning up the registered OPP (there's
	 * no function to do so), and simply cancel the
	 * registration of the cpufreq device.
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	 * Armada 3700 supports up to four CPU loads, but here register higher three loads,
	 * the lowest CPU load will be added by DTS.
#endif // CONFIG_SYNO_LSP_ARMADA_17_04_02
	 */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	cur_frequency *= divider_info->divider[MVEBU_CPU_LOAD_0];
	for (load_level = MVEBU_CPU_LOAD_0; load_level < MVEBU_CPU_LOAD_3; load_level++) {
		ret = dev_pm_opp_add(cpu_dev, cur_frequency / divider_info->divider[load_level], 0);
		if (ret)
			return ret;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	armada3700_cpufreq_divider_get(&div_arr[0]);
	ret = dev_pm_opp_add(dvfs_info->dev, cur_frequency, 0);
	if (ret)
		return ret;

	cur_frequency = cur_frequency * div_arr[MVEBU_CPU_LOAD_0];
	ret = dev_pm_opp_add(dvfs_info->dev, cur_frequency / div_arr[MVEBU_CPU_LOAD_1], 0);
	if (ret)
		return ret;

	ret = dev_pm_opp_add(dvfs_info->dev, cur_frequency / div_arr[MVEBU_CPU_LOAD_2], 0);
	if (ret)
		return ret;

	ret = dev_pm_opp_add(dvfs_info->dev, cur_frequency / div_arr[MVEBU_CPU_LOAD_3], 0);
	if (ret)
		return ret;

	ret = dev_pm_opp_init_cpufreq_table(dvfs_info->dev,
					    &dvfs_info->freq_table);
	if (ret) {
		dev_err(dvfs_info->dev,
			"failed to init cpufreq table: %d\n", ret);
		goto err_put_node;
	}
	dvfs_info->freq_count = dev_pm_opp_get_opp_count(dvfs_info->dev);

	ret = devm_request_irq(dvfs_info->dev, dvfs_info->irq,
				armada3700_cpufreq_irq, IRQF_TRIGGER_NONE,
				CPUFREQ_NAME, dvfs_info);
	if (ret) {
		dev_err(dvfs_info->dev, "Failed to register IRQ\n");
		goto err_put_node;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	pdev = platform_device_register_simple("cpufreq-dt", -1, NULL, 0);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* Enable DVFS */
	armada3700_enable_dvfs();

	ret = cpufreq_register_driver(&armada3700_driver);
	if (ret) {
		dev_err(dvfs_info->dev,
			"%s: failed to register cpufreq driver\n", __func__);
		goto err_put_node;
	}

	of_node_put(np);
	dvfs_info->dvfs_enabled = true;
	return 0;

err_put_node:
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	return PTR_ERR_OR_ZERO(pdev);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	of_node_put(np);
	dev_err(&pdev->dev, "%s: failed initialization\n", __func__);
	return ret;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
module_init(armada3700_cpufreq_driver_init);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

static int armada3700_cpufreq_remove(struct platform_device *pdev)
{
	cpufreq_unregister_driver(&armada3700_driver);
	dev_pm_opp_free_cpufreq_table(dvfs_info->dev, &dvfs_info->freq_table);
	clk_put(dvfs_info->cpu_clk);

	return 0;
}

static struct platform_driver armada3700_cpufreq_platdrv = {
	.driver = {
		.name	= "armada3700-cpufreq",
		.of_match_table = armada3700_cpufreq_match,
	},
	.probe		= armada3700_cpufreq_probe,
	.remove		= armada3700_cpufreq_remove,
};
module_platform_driver(armada3700_cpufreq_platdrv);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

MODULE_AUTHOR("Victor Gu <xigu@marvell.com>");
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
MODULE_AUTHOR("Evan Wang <xswang@marvell.com>");
MODULE_DESCRIPTION("Armada 3700 cpufreq driver");
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
MODULE_DESCRIPTION("Armada3700 cpufreq driver");
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
MODULE_LICENSE("GPL");
#endif /* CONFIG_SYNO_LSP_ARMADA_16_12 */
