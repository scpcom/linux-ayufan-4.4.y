#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/of.h>
#include "clk-core.h"

struct core_ratio {
	int id;
	const char *name;
};

struct core_clocks {
	u32 (*get_tclk_freq)(void __iomem *sar);
	u32 (*get_cpu_freq)(void __iomem *sar);
	void (*get_clk_ratio)(void __iomem *sar, int id, int *mult, int *div);
	const struct core_ratio *ratios;
	int num_ratios;
};

static struct clk_onecell_data clk_data;

static void __init mvebu_clk_core_setup(struct device_node *np,
				 struct core_clocks *coreclk)
{
	const char *tclk_name = "tclk";
	const char *cpuclk_name = "cpuclk";
	void __iomem *base;
	unsigned long rate;
	int n;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return;

	clk_data.clk_num = 2 + coreclk->num_ratios;
	clk_data.clks = kzalloc(clk_data.clk_num * sizeof(struct clk *),
				GFP_KERNEL);
	if (WARN_ON(!clk_data.clks))
		return;

	of_property_read_string_index(np, "clock-output-names", 0,
				      &tclk_name);
	rate = coreclk->get_tclk_freq(base);
	clk_data.clks[0] = clk_register_fixed_rate(NULL, tclk_name, NULL,
						   CLK_IS_ROOT, rate);
	WARN_ON(IS_ERR(clk_data.clks[0]));

	of_property_read_string_index(np, "clock-output-names", 1,
				      &cpuclk_name);
	rate = coreclk->get_cpu_freq(base);
	clk_data.clks[1] = clk_register_fixed_rate(NULL, cpuclk_name, NULL,
						   CLK_IS_ROOT, rate);
	WARN_ON(IS_ERR(clk_data.clks[1]));

	for (n = 0; n < coreclk->num_ratios; n++) {
		const char *rclk_name = coreclk->ratios[n].name;
		int mult, div;

		of_property_read_string_index(np, "clock-output-names",
					      2+n, &rclk_name);
		coreclk->get_clk_ratio(base, coreclk->ratios[n].id,
				       &mult, &div);
		clk_data.clks[2+n] = clk_register_fixed_factor(NULL, rclk_name,
				       cpuclk_name, 0, mult, div);
		WARN_ON(IS_ERR(clk_data.clks[2+n]));
	};

	iounmap(base);

	of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
}

#ifdef CONFIG_MACH_ARMADA_370_XP
 
#define SARL				    0	 
#define	    SARL_AXP_PCLK_FREQ_OPT	    21
#define	    SARL_AXP_PCLK_FREQ_OPT_MASK	    0x7
#define	    SARL_A370_PCLK_FREQ_OPT	    11
#define	    SARL_A370_PCLK_FREQ_OPT_MASK    0xF
#define	    SARL_AXP_FAB_FREQ_OPT	    24
#define	    SARL_AXP_FAB_FREQ_OPT_MASK	    0xF
#define	    SARL_A370_FAB_FREQ_OPT	    15
#define	    SARL_A370_FAB_FREQ_OPT_MASK	    0x1F
#define	    SARL_A370_TCLK_FREQ_OPT	    20
#define	    SARL_A370_TCLK_FREQ_OPT_MASK    0x1
#define SARH				    4	 
#define	    SARH_AXP_PCLK_FREQ_OPT	    (52-32)
#define	    SARH_AXP_PCLK_FREQ_OPT_MASK	    0x1
#define	    SARH_AXP_PCLK_FREQ_OPT_SHIFT    3
#define	    SARH_AXP_FAB_FREQ_OPT	    (51-32)
#define	    SARH_AXP_FAB_FREQ_OPT_MASK	    0x1
#define	    SARH_AXP_FAB_FREQ_OPT_SHIFT	    4

static const u32 __initconst armada_370_tclk_frequencies[] = {
	166000000,
	200000000,
};

static u32 __init armada_370_get_tclk_freq(void __iomem *sar)
{
	u8 tclk_freq_select = 0;

	tclk_freq_select = ((readl(sar) >> SARL_A370_TCLK_FREQ_OPT) &
			    SARL_A370_TCLK_FREQ_OPT_MASK);
	return armada_370_tclk_frequencies[tclk_freq_select];
}

static const u32 __initconst armada_370_cpu_frequencies[] = {
	400000000,
	533000000,
	667000000,
	800000000,
	1000000000,
	1067000000,
	1200000000,
};

static u32 __init armada_370_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar) >> SARL_A370_PCLK_FREQ_OPT) &
			   SARL_A370_PCLK_FREQ_OPT_MASK);
	if (cpu_freq_select >= ARRAY_SIZE(armada_370_cpu_frequencies)) {
		pr_err("CPU freq select unsupported %d\n", cpu_freq_select);
		cpu_freq = 0;
	} else
		cpu_freq = armada_370_cpu_frequencies[cpu_freq_select];

	return cpu_freq;
}

enum { A370_XP_NBCLK, A370_XP_HCLK, A370_XP_DRAMCLK };

static const struct core_ratio __initconst armada_370_xp_core_ratios[] = {
	{ .id = A370_XP_NBCLK,	 .name = "nbclk" },
	{ .id = A370_XP_HCLK,	 .name = "hclk" },
	{ .id = A370_XP_DRAMCLK, .name = "dramclk" },
};

static const int __initconst armada_370_xp_nbclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 2}, {2, 2},
	{1, 2}, {1, 2}, {1, 1}, {2, 3},
	{0, 1}, {1, 2}, {2, 4}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {2, 2},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int __initconst armada_370_xp_hclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 6}, {2, 3},
	{1, 3}, {1, 4}, {1, 2}, {2, 6},
	{0, 1}, {1, 6}, {2, 10}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 2},
	{2, 6}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int __initconst armada_370_xp_dramclk_ratios[32][2] = {
	{0, 1}, {1, 2}, {2, 3}, {2, 3},
	{1, 3}, {1, 2}, {1, 2}, {2, 6},
	{0, 1}, {1, 3}, {2, 5}, {0, 1},
	{1, 4}, {0, 1}, {0, 1}, {2, 5},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{2, 3}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static void __init armada_370_xp_get_clk_ratio(u32 opt,
	void __iomem *sar, int id, int *mult, int *div)
{
	switch (id) {
	case A370_XP_NBCLK:
		*mult = armada_370_xp_nbclk_ratios[opt][0];
		*div = armada_370_xp_nbclk_ratios[opt][1];
		break;
	case A370_XP_HCLK:
		*mult = armada_370_xp_hclk_ratios[opt][0];
		*div = armada_370_xp_hclk_ratios[opt][1];
		break;
	case A370_XP_DRAMCLK:
		*mult = armada_370_xp_dramclk_ratios[opt][0];
		*div = armada_370_xp_dramclk_ratios[opt][1];
		break;
	}
}

static void __init armada_370_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar) >> SARL_A370_FAB_FREQ_OPT) &
		SARL_A370_FAB_FREQ_OPT_MASK);

	armada_370_xp_get_clk_ratio(opt, sar, id, mult, div);
}

static const struct core_clocks armada_370_core_clocks = {
	.get_tclk_freq = armada_370_get_tclk_freq,
	.get_cpu_freq = armada_370_get_cpu_freq,
	.get_clk_ratio = armada_370_get_clk_ratio,
	.ratios = armada_370_xp_core_ratios,
	.num_ratios = ARRAY_SIZE(armada_370_xp_core_ratios),
};

static const u32 __initconst armada_xp_cpu_frequencies[] = {
	1000000000,
	1066000000,
	1200000000,
	1333000000,
	1500000000,
	1666000000,
	1800000000,
	2000000000,
	667000000,
	0,
	800000000,
	1600000000,
};

static u32 __init armada_xp_get_tclk_freq(void __iomem *sar)
{
	return 250 * 1000 * 1000;
}

static u32 __init armada_xp_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar) >> SARL_AXP_PCLK_FREQ_OPT) &
			   SARL_AXP_PCLK_FREQ_OPT_MASK);
	 
	cpu_freq_select |= (((readl(sar+4) >> SARH_AXP_PCLK_FREQ_OPT) &
			     SARH_AXP_PCLK_FREQ_OPT_MASK)
			    << SARH_AXP_PCLK_FREQ_OPT_SHIFT);
	if (cpu_freq_select >= ARRAY_SIZE(armada_xp_cpu_frequencies)) {
		pr_err("CPU freq select unsupported: %d\n", cpu_freq_select);
		cpu_freq = 0;
	} else
		cpu_freq = armada_xp_cpu_frequencies[cpu_freq_select];

	return cpu_freq;
}

static void __init armada_xp_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{

	u32 opt = ((readl(sar) >> SARL_AXP_FAB_FREQ_OPT) &
	      SARL_AXP_FAB_FREQ_OPT_MASK);
	 
	opt |= (((readl(sar+4) >> SARH_AXP_FAB_FREQ_OPT) &
		SARH_AXP_FAB_FREQ_OPT_MASK)
	       << SARH_AXP_FAB_FREQ_OPT_SHIFT);

	armada_370_xp_get_clk_ratio(opt, sar, id, mult, div);
}

static const struct core_clocks armada_xp_core_clocks = {
	.get_tclk_freq = armada_xp_get_tclk_freq,
	.get_cpu_freq = armada_xp_get_cpu_freq,
	.get_clk_ratio = armada_xp_get_clk_ratio,
	.ratios = armada_370_xp_core_ratios,
	.num_ratios = ARRAY_SIZE(armada_370_xp_core_ratios),
};

#endif  

#if defined(MY_ABC_HERE)
#ifdef CONFIG_MACH_ARMADA_375
 
#define	    SAR1_A375_TCLK_FREQ_OPT		22
#define	    SAR1_A375_TCLK_FREQ_OPT_MASK	0x1
#define	    SAR1_A375_CPU_DDR_L2_FREQ_OPT	17
#define	    SAR1_A375_CPU_DDR_L2_FREQ_OPT_MASK	0x1F

static const u32 __initconst armada_375_tclk_frequencies[] = {
	166000000,
	200000000,
};

static u32 __init armada_375_get_tclk_freq(void __iomem *sar)
{
	u8 tclk_freq_select = 0;

	tclk_freq_select = ((readl(sar) >> SAR1_A375_TCLK_FREQ_OPT) &
			    SAR1_A375_TCLK_FREQ_OPT_MASK);
	return armada_375_tclk_frequencies[tclk_freq_select];
}

static const u32 __initconst armada_375_cpu_frequencies[] = {
	0, 0, 0, 0, 0, 0,
	400000000,
	0, 0, 0, 0, 0, 0, 0, 0,
	600000000,
	0, 0, 0, 0, 0,
	800000000,
	0, 0, 0,
	1000000000,
};

static u32 __init armada_375_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar) >> SAR1_A375_CPU_DDR_L2_FREQ_OPT) &
			   SAR1_A375_CPU_DDR_L2_FREQ_OPT_MASK);
	if (cpu_freq_select >= ARRAY_SIZE(armada_375_cpu_frequencies)) {
		pr_err("CPU freq select unsupported %d\n", cpu_freq_select);
		cpu_freq = 0;
	} else
		cpu_freq = armada_375_cpu_frequencies[cpu_freq_select];

	return cpu_freq;
}

enum { A375_CPU_TO_DDR, A375_CPU_TO_L2};

static const struct core_ratio __initconst armada_375_core_ratios[] = {
	{ .id = A375_CPU_TO_L2,	 .name = "l2clk" },
	{ .id = A375_CPU_TO_DDR, .name = "ddrclk" },
};

static const int __initconst armada_375_cpu_l2_ratios[32][2] = {
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {1, 2}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {1, 2},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {1, 2}, {0, 1}, {0, 1},
	{0, 1}, {1, 2}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int __initconst armada_375_cpu_ddr_ratios[32][2] = {
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {1, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {2, 3},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {2, 3}, {0, 1}, {0, 1},
	{0, 1}, {1, 2}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static void __init armada_375_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar) >> SAR1_A375_CPU_DDR_L2_FREQ_OPT) &
		SAR1_A375_CPU_DDR_L2_FREQ_OPT_MASK);

	switch (id) {
	case A375_CPU_TO_L2:
		*mult = armada_375_cpu_l2_ratios[opt][0];
		*div = armada_375_cpu_l2_ratios[opt][1];
		break;
	case A375_CPU_TO_DDR:
		*mult = armada_375_cpu_ddr_ratios[opt][0];
		*div = armada_375_cpu_ddr_ratios[opt][1];
		break;
	}
}

static const struct core_clocks armada_375_core_clocks = {
	.get_tclk_freq = armada_375_get_tclk_freq,
	.get_cpu_freq = armada_375_get_cpu_freq,
	.get_clk_ratio = armada_375_get_clk_ratio,
	.ratios = armada_375_core_ratios,
	.num_ratios = ARRAY_SIZE(armada_375_core_ratios),
};

#endif  

#ifdef CONFIG_MACH_ARMADA_380
 
#define	    SAR_A380_TCLK_FREQ_OPT              15
#define	    SAR_A380_TCLK_FREQ_OPT_MASK	        0x1
#define	    SAR_A380_CPU_DDR_L2_FREQ_OPT        10
#define	    SAR_A380_CPU_DDR_L2_FREQ_OPT_MASK	0x1F

static const u32 __initconst armada_380_tclk_frequencies[] = {
	250000000,
	200000000,
};

static u32 __init armada_380_get_tclk_freq(void __iomem *sar)
{
	u8 tclk_freq_select = 0;

	tclk_freq_select = ((readl(sar) >> SAR_A380_TCLK_FREQ_OPT) &
			    SAR_A380_TCLK_FREQ_OPT_MASK);
	return armada_380_tclk_frequencies[tclk_freq_select];
}

static const u32 __initconst armada_380_cpu_frequencies[] = {
#if defined(MY_ABC_HERE)
	666 * 1000 * 1000, 0, 800 * 1000 * 1000, 0,
	1066 * 1000 * 1000, 0, 1200 * 1000 * 1000, 0,
	1332 * 1000 * 1000, 0, 0, 0,
	1600 * 1000 * 1000, 0, 0, 0,
	1866 * 1000 * 1000, 0, 0, 2000 * 1000 * 1000,
#else  
	0, 0, 0, 0,
	1066 * 1000 * 1000, 0, 0, 0,
	1332 * 1000 * 1000, 0, 0, 0,
	1600 * 1000 * 1000,
#endif  
};

static u32 __init armada_380_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar) >> SAR_A380_CPU_DDR_L2_FREQ_OPT) &
			   SAR_A380_CPU_DDR_L2_FREQ_OPT_MASK);
	if (cpu_freq_select >= ARRAY_SIZE(armada_380_cpu_frequencies)) {
		pr_err("CPU freq select unsupported %d\n", cpu_freq_select);
		cpu_freq = 0;
	} else
		cpu_freq = armada_380_cpu_frequencies[cpu_freq_select];

	return cpu_freq;
}

enum { A380_CPU_TO_DDR, A380_CPU_TO_L2 };

static const struct core_ratio __initconst armada_380_core_ratios[] = {
	{ .id = A380_CPU_TO_L2,	 .name = "l2clk" },
	{ .id = A380_CPU_TO_DDR, .name = "ddrclk" },
};

static const int __initconst armada_380_cpu_l2_ratios[32][2] = {
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static const int __initconst armada_380_cpu_ddr_ratios[32][2] = {
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {0, 1},
	{1, 2}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
	{0, 1}, {0, 1}, {0, 1}, {0, 1},
};

static void __init armada_380_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar) >> SAR_A380_CPU_DDR_L2_FREQ_OPT) &
		SAR_A380_CPU_DDR_L2_FREQ_OPT_MASK);

	switch (id) {
	case A380_CPU_TO_L2:
		*mult = armada_380_cpu_l2_ratios[opt][0];
		*div = armada_380_cpu_l2_ratios[opt][1];
		break;
	case A380_CPU_TO_DDR:
		*mult = armada_380_cpu_ddr_ratios[opt][0];
		*div = armada_380_cpu_ddr_ratios[opt][1];
		break;
	}
}

static const struct core_clocks armada_380_core_clocks = {
	.get_tclk_freq = armada_380_get_tclk_freq,
	.get_cpu_freq = armada_380_get_cpu_freq,
	.get_clk_ratio = armada_380_get_clk_ratio,
	.ratios = armada_380_core_ratios,
	.num_ratios = ARRAY_SIZE(armada_380_core_ratios),
};

#endif  
#endif  

#ifdef CONFIG_ARCH_DOVE
#define SAR_DOVE_CPU_FREQ		5
#define SAR_DOVE_CPU_FREQ_MASK		0xf
#define SAR_DOVE_L2_RATIO		9
#define SAR_DOVE_L2_RATIO_MASK		0x7
#define SAR_DOVE_DDR_RATIO		12
#define SAR_DOVE_DDR_RATIO_MASK		0xf
#define SAR_DOVE_TCLK_FREQ		23
#define SAR_DOVE_TCLK_FREQ_MASK		0x3

static const u32 __initconst dove_tclk_frequencies[] = {
	166666667,
	125000000,
	0, 0
};

static u32 __init dove_get_tclk_freq(void __iomem *sar)
{
	u32 opt = (readl(sar) >> SAR_DOVE_TCLK_FREQ) &
		SAR_DOVE_TCLK_FREQ_MASK;
	return dove_tclk_frequencies[opt];
}

static const u32 __initconst dove_cpu_frequencies[] = {
	0, 0, 0, 0, 0,
	1000000000,
	933333333, 933333333,
	800000000, 800000000, 800000000,
	1066666667,
	666666667,
	533333333,
	400000000,
	333333333
};

static u32 __init dove_get_cpu_freq(void __iomem *sar)
{
	u32 opt = (readl(sar) >> SAR_DOVE_CPU_FREQ) &
		SAR_DOVE_CPU_FREQ_MASK;
	return dove_cpu_frequencies[opt];
}

enum { DOVE_CPU_TO_L2, DOVE_CPU_TO_DDR };

static const struct core_ratio __initconst dove_core_ratios[] = {
	{ .id = DOVE_CPU_TO_L2, .name = "l2clk", },
	{ .id = DOVE_CPU_TO_DDR, .name = "ddrclk", }
};

static const int __initconst dove_cpu_l2_ratios[8][2] = {
	{ 1, 1 }, { 0, 1 }, { 1, 2 }, { 0, 1 },
	{ 1, 3 }, { 0, 1 }, { 1, 4 }, { 0, 1 }
};

static const int __initconst dove_cpu_ddr_ratios[16][2] = {
	{ 1, 1 }, { 0, 1 }, { 1, 2 }, { 2, 5 },
	{ 1, 3 }, { 0, 1 }, { 1, 4 }, { 0, 1 },
	{ 1, 5 }, { 0, 1 }, { 1, 6 }, { 0, 1 },
	{ 1, 7 }, { 0, 1 }, { 1, 8 }, { 1, 10 }
};

static void __init dove_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	switch (id) {
	case DOVE_CPU_TO_L2:
	{
		u32 opt = (readl(sar) >> SAR_DOVE_L2_RATIO) &
			SAR_DOVE_L2_RATIO_MASK;
		*mult = dove_cpu_l2_ratios[opt][0];
		*div = dove_cpu_l2_ratios[opt][1];
		break;
	}
	case DOVE_CPU_TO_DDR:
	{
		u32 opt = (readl(sar) >> SAR_DOVE_DDR_RATIO) &
			SAR_DOVE_DDR_RATIO_MASK;
		*mult = dove_cpu_ddr_ratios[opt][0];
		*div = dove_cpu_ddr_ratios[opt][1];
		break;
	}
	}
}

static const struct core_clocks dove_core_clocks = {
	.get_tclk_freq = dove_get_tclk_freq,
	.get_cpu_freq = dove_get_cpu_freq,
	.get_clk_ratio = dove_get_clk_ratio,
	.ratios = dove_core_ratios,
	.num_ratios = ARRAY_SIZE(dove_core_ratios),
};
#endif  

#ifdef CONFIG_ARCH_KIRKWOOD
#define SAR_KIRKWOOD_CPU_FREQ(x)	\
	(((x & (1 <<  1)) >>  1) |	\
	 ((x & (1 << 22)) >> 21) |	\
	 ((x & (3 <<  3)) >>  1))
#define SAR_KIRKWOOD_L2_RATIO(x)	\
	(((x & (3 <<  9)) >> 9) |	\
	 (((x & (1 << 19)) >> 17)))
#define SAR_KIRKWOOD_DDR_RATIO		5
#define SAR_KIRKWOOD_DDR_RATIO_MASK	0xf
#define SAR_MV88F6180_CLK		2
#define SAR_MV88F6180_CLK_MASK		0x7
#define SAR_KIRKWOOD_TCLK_FREQ		21
#define SAR_KIRKWOOD_TCLK_FREQ_MASK	0x1

enum { KIRKWOOD_CPU_TO_L2, KIRKWOOD_CPU_TO_DDR };

static const struct core_ratio __initconst kirkwood_core_ratios[] = {
	{ .id = KIRKWOOD_CPU_TO_L2, .name = "l2clk", },
	{ .id = KIRKWOOD_CPU_TO_DDR, .name = "ddrclk", }
};

static u32 __init kirkwood_get_tclk_freq(void __iomem *sar)
{
	u32 opt = (readl(sar) >> SAR_KIRKWOOD_TCLK_FREQ) &
		SAR_KIRKWOOD_TCLK_FREQ_MASK;
	return (opt) ? 166666667 : 200000000;
}

static const u32 __initconst kirkwood_cpu_frequencies[] = {
	0, 0, 0, 0,
	600000000,
	0,
	800000000,
	1000000000,
	0,
	1200000000,
	0, 0,
	1500000000,
	1600000000,
	1800000000,
	2000000000
};

static u32 __init kirkwood_get_cpu_freq(void __iomem *sar)
{
	u32 opt = SAR_KIRKWOOD_CPU_FREQ(readl(sar));
	return kirkwood_cpu_frequencies[opt];
}

static const int __initconst kirkwood_cpu_l2_ratios[8][2] = {
	{ 0, 1 }, { 1, 2 }, { 0, 1 }, { 1, 3 },
	{ 0, 1 }, { 1, 4 }, { 0, 1 }, { 0, 1 }
};

static const int __initconst kirkwood_cpu_ddr_ratios[16][2] = {
	{ 0, 1 }, { 0, 1 }, { 1, 2 }, { 0, 1 },
	{ 1, 3 }, { 0, 1 }, { 1, 4 }, { 2, 9 },
	{ 1, 5 }, { 1, 6 }, { 0, 1 }, { 0, 1 },
	{ 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }
};

static void __init kirkwood_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	switch (id) {
	case KIRKWOOD_CPU_TO_L2:
	{
		u32 opt = SAR_KIRKWOOD_L2_RATIO(readl(sar));
		*mult = kirkwood_cpu_l2_ratios[opt][0];
		*div = kirkwood_cpu_l2_ratios[opt][1];
		break;
	}
	case KIRKWOOD_CPU_TO_DDR:
	{
		u32 opt = (readl(sar) >> SAR_KIRKWOOD_DDR_RATIO) &
			SAR_KIRKWOOD_DDR_RATIO_MASK;
		*mult = kirkwood_cpu_ddr_ratios[opt][0];
		*div = kirkwood_cpu_ddr_ratios[opt][1];
		break;
	}
	}
}

static const struct core_clocks kirkwood_core_clocks = {
	.get_tclk_freq = kirkwood_get_tclk_freq,
	.get_cpu_freq = kirkwood_get_cpu_freq,
	.get_clk_ratio = kirkwood_get_clk_ratio,
	.ratios = kirkwood_core_ratios,
	.num_ratios = ARRAY_SIZE(kirkwood_core_ratios),
};

static const u32 __initconst mv88f6180_cpu_frequencies[] = {
	0, 0, 0, 0, 0,
	600000000,
	800000000,
	1000000000
};

static u32 __init mv88f6180_get_cpu_freq(void __iomem *sar)
{
	u32 opt = (readl(sar) >> SAR_MV88F6180_CLK) & SAR_MV88F6180_CLK_MASK;
	return mv88f6180_cpu_frequencies[opt];
}

static const int __initconst mv88f6180_cpu_ddr_ratios[8][2] = {
	{ 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 },
	{ 0, 1 }, { 1, 3 }, { 1, 4 }, { 1, 5 }
};

static void __init mv88f6180_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	switch (id) {
	case KIRKWOOD_CPU_TO_L2:
	{
		 
		*mult = 1;
		*div = 2;
		break;
	}
	case KIRKWOOD_CPU_TO_DDR:
	{
		u32 opt = (readl(sar) >> SAR_MV88F6180_CLK) &
			SAR_MV88F6180_CLK_MASK;
		*mult = mv88f6180_cpu_ddr_ratios[opt][0];
		*div = mv88f6180_cpu_ddr_ratios[opt][1];
		break;
	}
	}
}

static const struct core_clocks mv88f6180_core_clocks = {
	.get_tclk_freq = kirkwood_get_tclk_freq,
	.get_cpu_freq = mv88f6180_get_cpu_freq,
	.get_clk_ratio = mv88f6180_get_clk_ratio,
	.ratios = kirkwood_core_ratios,
	.num_ratios = ARRAY_SIZE(kirkwood_core_ratios),
};
#endif  

static const __initdata struct of_device_id clk_core_match[] = {
#ifdef CONFIG_MACH_ARMADA_370_XP
	{
		.compatible = "marvell,armada-370-core-clock",
		.data = &armada_370_core_clocks,
	},
	{
		.compatible = "marvell,armada-xp-core-clock",
		.data = &armada_xp_core_clocks,
	},
#endif
#if defined(MY_ABC_HERE)
#ifdef CONFIG_MACH_ARMADA_375
	{
		.compatible = "marvell,armada-375-core-clock",
		.data = &armada_375_core_clocks,
	},
#endif
#ifdef CONFIG_MACH_ARMADA_380
	{
		.compatible = "marvell,armada-380-core-clock",
		.data = &armada_380_core_clocks,
	},
#endif
#endif  
#ifdef CONFIG_ARCH_DOVE
	{
		.compatible = "marvell,dove-core-clock",
		.data = &dove_core_clocks,
	},
#endif

#ifdef CONFIG_ARCH_KIRKWOOD
	{
		.compatible = "marvell,kirkwood-core-clock",
		.data = &kirkwood_core_clocks,
	},
	{
		.compatible = "marvell,mv88f6180-core-clock",
		.data = &mv88f6180_core_clocks,
	},
#endif

	{ }
};

void __init mvebu_core_clk_init(void)
{
	struct device_node *np;

	for_each_matching_node(np, clk_core_match) {
		const struct of_device_id *match =
			of_match_node(clk_core_match, np);
		mvebu_clk_core_setup(np, (struct core_clocks *)match->data);
	}
}
