#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#if defined(MY_ABC_HERE)
#include <linux/interrupt.h>
#endif  
#include <linux/io.h>
#if defined(MY_ABC_HERE)
 
#else  
#include <linux/spinlock.h>
#endif  
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/of.h>
#if defined(MY_ABC_HERE)
#include <linux/of_device.h>
#else  
#include <mach/bridge-regs.h>
#endif  

#if defined(MY_ABC_HERE)
 
#define ORION_RSTOUT_MASK_OFFSET	0x20108

#define INTERNAL_REGS_MASK		~(SZ_1M - 1)
#endif  

#define TIMER_CTRL		0x0000
#if defined(MY_ABC_HERE)
#define TIMER_A370_STATUS	0x04

#define WDT_MAX_CYCLE_COUNT	0xffffffff

#define WDT_A370_RATIO_MASK(v)	((v) << 16)
#define WDT_A370_RATIO_SHIFT	5
#define WDT_A370_RATIO		(1 << WDT_A370_RATIO_SHIFT)

#define WDT_AXP_FIXED_ENABLE_BIT BIT(10)
#define WDT_A370_EXPIRED	BIT(31)
#else  
#define WDT_EN			0x0010
#define WDT_VAL			0x0024

#define WDT_MAX_CYCLE_COUNT	0xffffffff
#define WDT_IN_USE		0
#define WDT_OK_TO_CLOSE		1
#endif  

static bool nowayout = WATCHDOG_NOWAYOUT;
static int heartbeat = -1;		 
#if defined(MY_ABC_HERE)
 
#else  
static unsigned int wdt_max_duration;	 
static struct clk *clk;
static unsigned int wdt_tclk;
static void __iomem *wdt_reg;
static DEFINE_SPINLOCK(wdt_lock);
#endif  

#if defined(MY_ABC_HERE)
struct orion_watchdog;

struct orion_watchdog_data {
	int wdt_counter_offset;
	int wdt_enable_bit;
	int rstout_enable_bit;
	int rstout_mask_bit;
	int (*clock_init)(struct platform_device *,
			  struct orion_watchdog *);
	int (*enabled)(struct orion_watchdog *);
	int (*start)(struct watchdog_device *);
	int (*stop)(struct watchdog_device *);
};

struct orion_watchdog {
	struct watchdog_device wdt;
	void __iomem *reg;
	void __iomem *rstout;
	void __iomem *rstout_mask;
	unsigned long clk_rate;
	struct clk *clk;
	const struct orion_watchdog_data *data;
};

static int orion_wdt_clock_init(struct platform_device *pdev,
				struct orion_watchdog *dev)
{
	int ret;

	dev->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(dev->clk))
		return PTR_ERR(dev->clk);
	ret = clk_prepare_enable(dev->clk);
	if (ret) {
		clk_put(dev->clk);
		return ret;
	}

	dev->clk_rate = clk_get_rate(dev->clk);
	return 0;
}

static int armada370_wdt_clock_init(struct platform_device *pdev,
				    struct orion_watchdog *dev)
{
	int ret;

	dev->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(dev->clk))
		return PTR_ERR(dev->clk);
	ret = clk_prepare_enable(dev->clk);
	if (ret) {
		clk_put(dev->clk);
		return ret;
	}

	atomic_io_modify(dev->reg + TIMER_CTRL,
			WDT_A370_RATIO_MASK(WDT_A370_RATIO_SHIFT),
			WDT_A370_RATIO_MASK(WDT_A370_RATIO_SHIFT));

	dev->clk_rate = clk_get_rate(dev->clk) / WDT_A370_RATIO;
	return 0;
}

static int armadaxp_wdt_clock_init(struct platform_device *pdev,
				   struct orion_watchdog *dev)
{
	int ret;

	dev->clk = of_clk_get_by_name(pdev->dev.of_node, "fixed");
	if (IS_ERR(dev->clk))
		return PTR_ERR(dev->clk);
	ret = clk_prepare_enable(dev->clk);
	if (ret) {
		clk_put(dev->clk);
		return ret;
	}

	atomic_io_modify(dev->reg + TIMER_CTRL,
			 WDT_AXP_FIXED_ENABLE_BIT,
			 WDT_AXP_FIXED_ENABLE_BIT);

	dev->clk_rate = clk_get_rate(dev->clk);
	return 0;
}

static int orion_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);
	 
	writel(dev->clk_rate * wdt_dev->timeout,
	       dev->reg + dev->data->wdt_counter_offset);
	return 0;
}

static int armada375_start(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);
	u32 reg;

	writel(dev->clk_rate * wdt_dev->timeout,
	       dev->reg + dev->data->wdt_counter_offset);

	atomic_io_modify(dev->reg + TIMER_A370_STATUS, WDT_A370_EXPIRED, 0);

	atomic_io_modify(dev->reg + TIMER_CTRL, dev->data->wdt_enable_bit,
						dev->data->wdt_enable_bit);

	reg = readl(dev->rstout);
	reg |= dev->data->rstout_enable_bit;
	writel(reg, dev->rstout);

	atomic_io_modify(dev->rstout_mask, dev->data->rstout_mask_bit, 0);
	return 0;
}

static int armada370_start(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);
	u32 reg;

	writel(dev->clk_rate * wdt_dev->timeout,
	       dev->reg + dev->data->wdt_counter_offset);

	atomic_io_modify(dev->reg + TIMER_A370_STATUS, WDT_A370_EXPIRED, 0);

	atomic_io_modify(dev->reg + TIMER_CTRL, dev->data->wdt_enable_bit,
						dev->data->wdt_enable_bit);

	reg = readl(dev->rstout);
	reg |= dev->data->rstout_enable_bit;
	writel(reg, dev->rstout);
	return 0;
}

static int orion_start(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);

	writel(dev->clk_rate * wdt_dev->timeout,
	       dev->reg + dev->data->wdt_counter_offset);

	atomic_io_modify(dev->reg + TIMER_CTRL, dev->data->wdt_enable_bit,
						dev->data->wdt_enable_bit);

	atomic_io_modify(dev->rstout, dev->data->rstout_enable_bit,
				      dev->data->rstout_enable_bit);

	return 0;
}

static int orion_wdt_start(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);

	return dev->data->start(wdt_dev);
}

static int orion_stop(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);

	atomic_io_modify(dev->rstout, dev->data->rstout_enable_bit, 0);

	atomic_io_modify(dev->reg + TIMER_CTRL, dev->data->wdt_enable_bit, 0);

	return 0;
}

static int armada375_stop(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);
	u32 reg;

	atomic_io_modify(dev->rstout_mask, dev->data->rstout_mask_bit,
					   dev->data->rstout_mask_bit);
	reg = readl(dev->rstout);
	reg &= ~dev->data->rstout_enable_bit;
	writel(reg, dev->rstout);

	atomic_io_modify(dev->reg + TIMER_CTRL, dev->data->wdt_enable_bit, 0);

	return 0;
}

static int armada370_stop(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);
	u32 reg;

	reg = readl(dev->rstout);
	reg &= ~dev->data->rstout_enable_bit;
	writel(reg, dev->rstout);

	atomic_io_modify(dev->reg + TIMER_CTRL, dev->data->wdt_enable_bit, 0);

	return 0;
}

static int orion_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);

	return dev->data->stop(wdt_dev);
}

static int orion_enabled(struct orion_watchdog *dev)
{
	bool enabled, running;

	enabled = readl(dev->rstout) & dev->data->rstout_enable_bit;
	running = readl(dev->reg + TIMER_CTRL) & dev->data->wdt_enable_bit;

	return enabled && running;
}

static int armada375_enabled(struct orion_watchdog *dev)
{
	bool masked, enabled, running;

	masked = readl(dev->rstout_mask) & dev->data->rstout_mask_bit;
	enabled = readl(dev->rstout) & dev->data->rstout_enable_bit;
	running = readl(dev->reg + TIMER_CTRL) & dev->data->wdt_enable_bit;

	return !masked && enabled && running;
}

static int orion_wdt_enabled(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);

	return dev->data->enabled(dev);
}

static unsigned int orion_wdt_get_timeleft(struct watchdog_device *wdt_dev)
{
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);
	return readl(dev->reg + dev->data->wdt_counter_offset) / dev->clk_rate;
}
#else  
static int orion_wdt_ping(struct watchdog_device *wdt_dev)
{
	spin_lock(&wdt_lock);

	writel(wdt_tclk * wdt_dev->timeout, wdt_reg + WDT_VAL);

	spin_unlock(&wdt_lock);
	return 0;
}

static int orion_wdt_start(struct watchdog_device *wdt_dev)
{
	u32 reg;

	spin_lock(&wdt_lock);

	writel(wdt_tclk * wdt_dev->timeout, wdt_reg + WDT_VAL);

	reg = readl(BRIDGE_CAUSE);
	reg &= ~WDT_INT_REQ;
	writel(reg, BRIDGE_CAUSE);

	reg = readl(wdt_reg + TIMER_CTRL);
	reg |= WDT_EN;
	writel(reg, wdt_reg + TIMER_CTRL);

	reg = readl(RSTOUTn_MASK);
	reg |= WDT_RESET_OUT_EN;
	writel(reg, RSTOUTn_MASK);

	spin_unlock(&wdt_lock);
	return 0;
}

static int orion_wdt_stop(struct watchdog_device *wdt_dev)
{
	u32 reg;

	spin_lock(&wdt_lock);

	reg = readl(RSTOUTn_MASK);
	reg &= ~WDT_RESET_OUT_EN;
	writel(reg, RSTOUTn_MASK);

	reg = readl(wdt_reg + TIMER_CTRL);
	reg &= ~WDT_EN;
	writel(reg, wdt_reg + TIMER_CTRL);

	spin_unlock(&wdt_lock);
	return 0;
}

static unsigned int orion_wdt_get_timeleft(struct watchdog_device *wdt_dev)
{
	unsigned int time_left;

	spin_lock(&wdt_lock);
	time_left = readl(wdt_reg + WDT_VAL) / wdt_tclk;
	spin_unlock(&wdt_lock);

	return time_left;
}
#endif  

static int orion_wdt_set_timeout(struct watchdog_device *wdt_dev,
				 unsigned int timeout)
{
	wdt_dev->timeout = timeout;
	return 0;
}

static const struct watchdog_info orion_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "Orion Watchdog",
};

static const struct watchdog_ops orion_wdt_ops = {
	.owner = THIS_MODULE,
	.start = orion_wdt_start,
	.stop = orion_wdt_stop,
	.ping = orion_wdt_ping,
	.set_timeout = orion_wdt_set_timeout,
	.get_timeleft = orion_wdt_get_timeleft,
};

#if defined(MY_ABC_HERE)
static irqreturn_t orion_wdt_irq(int irq, void *devid)
{
	panic("Watchdog Timeout");
	return IRQ_HANDLED;
}

static void __iomem *orion_wdt_ioremap_rstout(struct platform_device *pdev,
					      phys_addr_t internal_regs)
{
	struct resource *res;
	phys_addr_t rstout;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res)
		return devm_ioremap(&pdev->dev, res->start,
				    resource_size(res));

	rstout = internal_regs + ORION_RSTOUT_MASK_OFFSET;

	WARN(1, FW_BUG "falling back to harcoded RSTOUT reg %pa\n", &rstout);
	return devm_ioremap(&pdev->dev, rstout, 0x4);
}

static const struct orion_watchdog_data orion_data = {
	.rstout_enable_bit = BIT(1),
	.wdt_enable_bit = BIT(4),
	.wdt_counter_offset = 0x24,
	.clock_init = orion_wdt_clock_init,
	.enabled = orion_enabled,
	.start = orion_start,
	.stop = orion_stop,
};

static const struct orion_watchdog_data armada370_data = {
	.rstout_enable_bit = BIT(8),
	.wdt_enable_bit = BIT(8),
	.wdt_counter_offset = 0x34,
	.clock_init = armada370_wdt_clock_init,
	.enabled = orion_enabled,
	.start = armada370_start,
	.stop = armada370_stop,
};

static const struct orion_watchdog_data armadaxp_data = {
	.rstout_enable_bit = BIT(8),
	.wdt_enable_bit = BIT(8),
	.wdt_counter_offset = 0x34,
	.clock_init = armadaxp_wdt_clock_init,
	.enabled = orion_enabled,
	.start = armada370_start,
	.stop = armada370_stop,
};

static const struct orion_watchdog_data armada375_data = {
	.rstout_enable_bit = BIT(8),
	.rstout_mask_bit = BIT(10),
	.wdt_enable_bit = BIT(8),
	.wdt_counter_offset = 0x34,
	.clock_init = armada370_wdt_clock_init,
	.enabled = armada375_enabled,
	.start = armada375_start,
	.stop = armada375_stop,
};

static const struct orion_watchdog_data armada380_data = {
	.rstout_enable_bit = BIT(8),
	.rstout_mask_bit = BIT(10),
	.wdt_enable_bit = BIT(8),
	.wdt_counter_offset = 0x34,
	.clock_init = armadaxp_wdt_clock_init,
	.enabled = armada375_enabled,
	.start = armada375_start,
	.stop = armada375_stop,
};

static const struct of_device_id orion_wdt_of_match_table[] = {
	{
		.compatible = "marvell,orion-wdt",
		.data = &orion_data,
	},
	{
		.compatible = "marvell,armada-370-wdt",
		.data = &armada370_data,
	},
	{
		.compatible = "marvell,armada-xp-wdt",
		.data = &armadaxp_data,
	},
	{
		.compatible = "marvell,armada-375-wdt",
		.data = &armada375_data,
	},
	{
		.compatible = "marvell,armada-380-wdt",
		.data = &armada380_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, orion_wdt_of_match_table);
#else  
static struct watchdog_device orion_wdt = {
	.info = &orion_wdt_info,
	.ops = &orion_wdt_ops,
	.min_timeout = 1,
};
#endif  

#if defined(MY_ABC_HERE)
static int orion_wdt_get_regs(struct platform_device *pdev,
			      struct orion_watchdog *dev)
{
	struct device_node *node = pdev->dev.of_node;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	dev->reg = devm_ioremap(&pdev->dev, res->start,
				resource_size(res));
	if (!dev->reg)
		return -ENOMEM;

	if (of_device_is_compatible(node, "marvell,orion-wdt")) {

		dev->rstout = orion_wdt_ioremap_rstout(pdev, res->start &
						       INTERNAL_REGS_MASK);
		if (!dev->rstout)
			return -ENODEV;

	} else if (of_device_is_compatible(node, "marvell,armada-370-wdt") ||
		   of_device_is_compatible(node, "marvell,armada-xp-wdt")) {

		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		dev->rstout = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(dev->rstout))
			return PTR_ERR(dev->rstout);

	} else if (of_device_is_compatible(node, "marvell,armada-375-wdt") ||
		   of_device_is_compatible(node, "marvell,armada-380-wdt")) {

		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		dev->rstout = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(dev->rstout))
			return PTR_ERR(dev->rstout);

		res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		if (!res)
			return -ENODEV;
		dev->rstout_mask = devm_ioremap(&pdev->dev, res->start,
						resource_size(res));
		if (!dev->rstout_mask)
			return -ENOMEM;

	} else {
		return -ENODEV;
	}

	return 0;
}

static int orion_wdt_probe(struct platform_device *pdev)
{
	struct orion_watchdog *dev;
	const struct of_device_id *match;
	unsigned int wdt_max_duration;	 
	int ret, irq;

	dev = devm_kzalloc(&pdev->dev, sizeof(struct orion_watchdog),
			   GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	match = of_match_device(orion_wdt_of_match_table, &pdev->dev);
	if (!match)
		 
		match = &orion_wdt_of_match_table[0];

	dev->wdt.info = &orion_wdt_info;
	dev->wdt.ops = &orion_wdt_ops;
	dev->wdt.min_timeout = 1;
	dev->data = match->data;

	ret = orion_wdt_get_regs(pdev, dev);
	if (ret)
		return ret;

	ret = dev->data->clock_init(pdev, dev);
	if (ret) {
		dev_err(&pdev->dev, "cannot initialize clock\n");
		return ret;
	}

	wdt_max_duration = WDT_MAX_CYCLE_COUNT / dev->clk_rate;

	dev->wdt.timeout = wdt_max_duration;
	dev->wdt.max_timeout = wdt_max_duration;
	watchdog_init_timeout(&dev->wdt, heartbeat, &pdev->dev);

	platform_set_drvdata(pdev, &dev->wdt);
	watchdog_set_drvdata(&dev->wdt, dev);

	if (!orion_wdt_enabled(&dev->wdt))
		orion_wdt_stop(&dev->wdt);

	irq = platform_get_irq(pdev, 0);
	if (irq > 0) {
		 
		ret = devm_request_irq(&pdev->dev, irq, orion_wdt_irq, 0,
				       pdev->name, dev);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request IRQ\n");
			goto disable_clk;
		}
	}

	watchdog_set_nowayout(&dev->wdt, nowayout);
	ret = watchdog_register_device(&dev->wdt);
	if (ret)
		goto disable_clk;

	pr_info("Initial timeout %d sec%s\n",
		dev->wdt.timeout, nowayout ? ", nowayout" : "");
	return 0;

disable_clk:
	clk_disable_unprepare(dev->clk);
	clk_put(dev->clk);
	return ret;
}

static int orion_wdt_remove(struct platform_device *pdev)
{
	struct watchdog_device *wdt_dev = platform_get_drvdata(pdev);
	struct orion_watchdog *dev = watchdog_get_drvdata(wdt_dev);

	watchdog_unregister_device(wdt_dev);
	clk_disable_unprepare(dev->clk);
	clk_put(dev->clk);
	return 0;
}

static void orion_wdt_shutdown(struct platform_device *pdev)
{
	struct watchdog_device *wdt_dev = platform_get_drvdata(pdev);
	orion_wdt_stop(wdt_dev);
}
#else  
static int orion_wdt_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Orion Watchdog missing clock\n");
		return -ENODEV;
	}
	clk_prepare_enable(clk);
	wdt_tclk = clk_get_rate(clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	wdt_reg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!wdt_reg)
		return -ENOMEM;

	wdt_max_duration = WDT_MAX_CYCLE_COUNT / wdt_tclk;

	orion_wdt.timeout = wdt_max_duration;
	orion_wdt.max_timeout = wdt_max_duration;
	watchdog_init_timeout(&orion_wdt, heartbeat, &pdev->dev);

	watchdog_set_nowayout(&orion_wdt, nowayout);
	ret = watchdog_register_device(&orion_wdt);
	if (ret) {
		clk_disable_unprepare(clk);
		return ret;
	}

	pr_info("Initial timeout %d sec%s\n",
		orion_wdt.timeout, nowayout ? ", nowayout" : "");
	return 0;
}

static int orion_wdt_remove(struct platform_device *pdev)
{
	watchdog_unregister_device(&orion_wdt);
	clk_disable_unprepare(clk);
	return 0;
}

static void orion_wdt_shutdown(struct platform_device *pdev)
{
	orion_wdt_stop(&orion_wdt);
}

static const struct of_device_id orion_wdt_of_match_table[] = {
	{ .compatible = "marvell,orion-wdt", },
	{},
};
MODULE_DEVICE_TABLE(of, orion_wdt_of_match_table);
#endif  

static struct platform_driver orion_wdt_driver = {
	.probe		= orion_wdt_probe,
	.remove		= orion_wdt_remove,
	.shutdown	= orion_wdt_shutdown,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "orion_wdt",
#if defined(MY_ABC_HERE)
		.of_match_table = orion_wdt_of_match_table,
#else  
		.of_match_table = of_match_ptr(orion_wdt_of_match_table),
#endif  
	},
};

module_platform_driver(orion_wdt_driver);

MODULE_AUTHOR("Sylver Bruneau <sylver.bruneau@googlemail.com>");
MODULE_DESCRIPTION("Orion Processor Watchdog");

module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat, "Initial watchdog heartbeat in seconds");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:orion_wdt");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
