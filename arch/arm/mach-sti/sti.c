#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/clk-provider.h>
#include <linux/of_gpio.h>
#include <linux/memblock.h>
#include <linux/slab.h>
#ifdef CONFIG_SOC_BUS
#include <linux/sys_soc.h>
#endif
#include <linux/stat.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#if defined(MY_DEF_HERE)
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#if defined(MY_DEF_HERE)
extern int (*syno_standby_power_enable)(void);
#endif

#define BAUDRATE_VAL_M1(bps, clk)  \
	((((bps * (1 << 14)) + (1 << 13)) / (clk / (1 << 6))))

static void synology_power_off(void)
{

	if ( of_aliases ) {
		int reg,size_reg;
		unsigned long microp_rate;
		void __iomem *microp_base;
		const void *prop = of_get_property(of_aliases,"ttyS1",NULL);
		struct device_node *microp;
		struct clk *clk;

		if (prop)
			microp = of_find_node_by_path(prop);
		else
			goto END;

		if  (!IS_ERR_OR_NULL(microp)) {
			if (!of_property_read_u32_index(microp, "reg", 0, &reg)) {
				if (!of_property_read_u32_index(microp, "reg", 1, &size_reg)) {
					microp_base = ioremap_nocache(reg, size_reg);
				}
			}

			clk = of_clk_get(microp, 0);
			if (clk)
				microp_rate = clk_get_rate(clk);

		} else
			goto END;

		writel(0x1189 & ~0x80, microp_base + 0x0c);  
		writel(BAUDRATE_VAL_M1(9600, microp_rate), microp_base);  
		writel(20, microp_base + 0x1c);  
		writel(1, microp_base + 0x10);  
		writel(0x1189, microp_base + 0x0c);  

#if defined(MY_DEF_HERE)
		if (NULL != syno_standby_power_enable) {
			if (syno_standby_power_enable())
				writel('l', microp_base + 0x4);
			else
				writel('k', microp_base + 0x4);
		}

		mdelay(200);
#endif

		writel('1', microp_base + 0x4);
	}
END:
	mdelay(100);
	printk(KERN_ERR "syno power off failed!\n");

}

#endif  
 
static void stid127_setup_clockgentel(void)
{
	void __iomem *base;
	int tmp;

	base = ioremap(0xfe910000, SZ_128);
	if (!base) {
		pr_err("%s: failed to map QFS_TEL base address (%ld)\n",
				__func__, PTR_ERR(base));
		return;
	}

	writel(0x208, base);
	tmp = readl(base + 0x1c);
	writel(tmp & 0x1fe, base + 0x1c);
	tmp =  readl(base + 0x20);
	writel(tmp & 0x1fe, base + 0x20);

	tmp = readl(base);
	writel(tmp | BIT(6), base);
	tmp = readl(base + 0x20);
	writel(tmp & ~BIT(3), base + 0x20);
	writel(0x125ccccd, base + 0xc);
	writel(0x124ccccd, base + 0xc);
	tmp = readl(base + 0x1c);
	writel(tmp & ~BIT(3), base + 0x1c);

	writel(0x2012d, base + 0x64);
	writel(0xc, base + 0x74);

	iounmap(base);

	return;
}

static void __init soc_info_populate(struct soc_device_attribute *soc_dev_attr)
{
	char *family;
	char *model;
	char *pch;
	int family_length;
	unsigned long dt_root;

	soc_dev_attr->machine = "STi";

	dt_root = of_get_flat_dt_root();
	model = of_get_flat_dt_prop(dt_root, "model", NULL);
	if (!model)
		return;

	pch = strchr(model, ' ');
	if (!pch)
		return;
	family_length = pch - model;

	family = kzalloc((family_length + 1) * sizeof(char), GFP_KERNEL);
	if (!family)
		return;

	strncat(family, model, family_length);
	soc_dev_attr->family = kasprintf(GFP_KERNEL, "%s", family);
}

static struct device *sti_soc_device_init(void)
{
	struct soc_device *soc_dev;
	struct soc_device_attribute *soc_dev_attr;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return NULL;

	soc_info_populate(soc_dev_attr);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		kfree(soc_dev_attr);
		return NULL;
	}

	return soc_device_to_device(soc_dev);
}

#define ST_GPIO_PINS_PER_BANK (8)
#define TSOUT1_BYTECLK_GPIO (5 * ST_GPIO_PINS_PER_BANK + 0)

void __init sti_init_machine_late(void)
{
	if (of_machine_is_compatible("st,stid127"))
		gpio_request_one(TSOUT1_BYTECLK_GPIO,
			GPIOF_OUT_INIT_LOW, "tsout1_byteclk");

#if defined(MY_DEF_HERE)
	pm_power_off = synology_power_off;
#endif
}

void __init sti_init_machine(void)
{
#ifdef MY_DEF_HERE
#else  
	struct platform_device_info devinfo = { .name = "cpufreq-cpu0", };
#endif  
	struct device *parent = NULL;

	parent = sti_soc_device_init();

	of_platform_populate(NULL, of_default_bus_match_table, NULL, parent);

	if (of_machine_is_compatible("st,stid127"))
		stid127_setup_clockgentel();
#ifdef MY_DEF_HERE
#else  

	platform_device_register_full(&devinfo);
#endif  
}
#ifdef MY_DEF_HERE
static int sti_register_cpufreqcpu0(void)
{
	struct platform_device_info devinfo = { .name = "cpufreq-cpu0", };
	platform_device_register_full(&devinfo);

	return 0;
}
late_initcall(sti_register_cpufreqcpu0);
#endif  
