#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_DEF_HERE
 
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <asm/fsl_pixis.h>
#include <asm/immap_85xx.h>
#include <sysdev/fsl_soc.h>

struct pmc_regs {
	__be32 devdisr;
	__be32:32;
	__be32:32;
	__be32:32;
	__be32 pmcsr;
	__be32:32;
#ifdef MY_DEF_HERE
	__be32 pmpdccr;
#else
	__be32:32;
#endif
	__be32 pmcdr;
};
static struct device *pmc_dev;
static struct pmc_regs __iomem *pmc_regs;
#ifdef MY_DEF_HERE
static struct ccsr_guts __iomem *guts;
static void __iomem *gpio_base;
#endif

#define PMCSR_DPSLP	0x00100000
#define PMCSR_SLP	0x00020000
#define PMCSR_LOSSLESS	0x00400000
#define PMCSR_INT_MASK	0x00000f00
static int has_deep_sleep, has_lossless;

void mpc85xx_enter_deep_sleep(phys_addr_t ccsrbar, u32 powmgtreq);
extern void flush_dcache_L1(void);
#ifdef MY_DEF_HERE
extern int SYNOQorIQHWReset(void);
#endif

static LIST_HEAD(wakeup);

struct wake_data{
	struct list_head link;
	wakeup_event_t func;
	void *data;
};

static void pmc_suspend_end(void)
{
	struct wake_data *p, *tmp;

	list_for_each_entry_safe(p, tmp, &wakeup, link) {
		list_del(&p->link);
		kfree(p);
	}
}

static int pmc_suspend_exit(void)
{
	struct wake_data *p;

	list_for_each_entry(p, &wakeup, link) {
		if (p->func(p->data))
			return 1;
	}
	return 0;
}

#ifdef MY_DEF_HERE
extern void GPIOSuspend(void);
#endif
int pmc_enable_wake(struct of_device *ofdev, wakeup_event_t func, bool enable)
{
	int ret = 0;
	struct device_node *clk_np;
	u32 *pmcdr_mask;
	struct wake_data *tmp;

	if (!pmc_regs) {
		printk(KERN_WARNING "PMC is unavailable\n");
		return -ENOMEM;
	}

	if (enable && !device_may_wakeup(&ofdev->dev))
		return -EINVAL;

	clk_np = of_parse_phandle(ofdev->node, "clk-handle", 0);
	if (!clk_np)
		return -EINVAL;

	pmcdr_mask = (u32 *)of_get_property(clk_np, "fsl,pmcdr-mask", NULL);
	if (!pmcdr_mask) {
		ret = -EINVAL;
		goto out;
	}

	if (enable)
		clrbits32(&pmc_regs->pmcdr, *pmcdr_mask);
	else
		setbits32(&pmc_regs->pmcdr, *pmcdr_mask);

#ifdef MY_DEF_HERE
	if (enable) {
		 
		setbits32(&pmc_regs->pmcdr, 0x00c00800);
		setbits32(&guts->pmuxcr, 0x20000000);
		msleep(200);
		setbits32(gpio_base + 0x100, 0xfff00000);
		GPIOSuspend();
	} else {
		 
		clrbits32(gpio_base + 0x100, 0xfff00000);
		clrbits32(&guts->pmuxcr, 0x20000000);
		clrbits32(&pmc_regs->pmcdr, 0x00c00800);
	}
#endif

	if (func != NULL) {
		tmp = kzalloc(sizeof(struct wake_data), GFP_KERNEL);
		if (!tmp) {
			dev_err(&ofdev->dev, "out of memory\n");
			ret = -ENOMEM;
			goto out;
		}
		tmp->func = func;
		tmp->data = ofdev;
		list_add_tail(&tmp->link, &wakeup);
	}
out:
	of_node_put(clk_np);
	return ret;
}
EXPORT_SYMBOL_GPL(pmc_enable_wake);

void pmc_enable_lossless(int enable)
{
	if (enable && has_lossless)
		setbits32(&pmc_regs->pmcsr, PMCSR_LOSSLESS);
	else
		clrbits32(&pmc_regs->pmcsr, PMCSR_LOSSLESS);
}
EXPORT_SYMBOL_GPL(pmc_enable_lossless);

#ifdef MY_DEF_HERE
extern unsigned char GPIOShouldWake(void);
#endif
static int pmc_suspend_enter(suspend_state_t state)
{
	int ret;
	u32 powmgtreq = PMCSR_DPSLP | PMCSR_LOSSLESS | PMCSR_INT_MASK;

	switch (state) {
	case PM_SUSPEND_MEM:
#ifdef CONFIG_SPE
		enable_kernel_spe();
#endif
		pr_debug("Entering deep sleep\n");

		local_irq_disable();
#ifdef MY_DEF_HERE
		setbits32(&pmc_regs->pmcsr, PMCSR_INT_MASK);
#endif
#ifdef MY_DEF_HERE
		if (!pmc_suspend_exit() && !GPIOShouldWake()) {
#else
		if (!pmc_suspend_exit()) {
#endif
#ifdef MY_DEF_HERE
			 
			setbits32(&pmc_regs->devdisr, 0x00002000);
#else
			setbits32(&pmc_regs->pmcsr, PMCSR_INT_MASK);
#endif
#ifdef MY_DEF_HERE
			clrbits32(&pmc_regs->pmpdccr, 0x1fff0000);
			setbits32(&pmc_regs->pmpdccr, 0x16ff0000);
#endif
			mpc85xx_enter_deep_sleep(get_immrbase(),
					powmgtreq);
#ifdef MY_DEF_HERE
			 
			SYNOQorIQHWReset();
			clrbits32(&pmc_regs->devdisr, 0x00002000);
#else
			clrbits32(&pmc_regs->pmcsr, PMCSR_INT_MASK);
#endif

		}
#ifdef MY_DEF_HERE
		clrbits32(&pmc_regs->pmcsr, PMCSR_INT_MASK);
#endif
		pr_debug("Resumed from deep sleep\n");

		return 0;

	case PM_SUSPEND_STANDBY:
		local_irq_disable();
		flush_dcache_L1();

		pixis_start_pm_sleep();
		if (!pmc_suspend_exit()) {
			setbits32(&pmc_regs->pmcsr, PMCSR_INT_MASK | PMCSR_SLP);
			 
			ret = spin_event_timeout(
				(in_be32(&pmc_regs->pmcsr) & PMCSR_SLP)
				== 0, 10000, 10) ? 0 : -ETIMEDOUT;
			if (ret)
				dev_err(pmc_dev, "timeout waiting for \
						SLP bit to be cleared\n");
			clrbits32(&pmc_regs->pmcsr, PMCSR_INT_MASK);
		}
		 
		pixis_stop_pm_sleep();

		return 0;

	default:
		return -EINVAL;

	}

}

static int pmc_suspend_valid(suspend_state_t state)
{
	if (state == PM_SUSPEND_STANDBY)
		return 1;
	if (has_deep_sleep && (state == PM_SUSPEND_MEM))
		return 1;
	return 0;
}

static struct platform_suspend_ops pmc_suspend_ops = {
	.valid = pmc_suspend_valid,
	.enter = pmc_suspend_enter,
	.prepare_late = pmc_suspend_exit,
	.end = pmc_suspend_end
};

static int pmc_probe(struct of_device *ofdev, const struct of_device_id *id)
{

	struct device_node *np = ofdev->node;

	pmc_regs = of_iomap(ofdev->node, 0);
	if (!pmc_regs)
		return -ENOMEM;

	if (of_device_is_compatible(np, "fsl,mpc8536-pmc"))
		has_deep_sleep = 1;

	if (of_device_is_compatible(np, "fsl,p1022-pmc")) {
		has_lossless = 1;

		if ((mfspr(SPRN_SVR) & 0xff) == 0x11) {
			struct device_node *node;
#ifdef MY_DEF_HERE
			struct device_node *gpio_node;
#else
			struct ccsr_guts __iomem *guts;
#endif

			node = of_find_compatible_node(NULL, NULL,
					"fsl,p1022-guts");
			if (!node) {
				printk(KERN_WARNING "Not set DSCR --"
					" Could not find GUTS node\n");
				goto end;
			}

			guts = of_iomap(node, 0);
			of_node_put(node);
			if (!guts) {
				printk(KERN_WARNING "Not set DSCR --"
					" Failed to map GUTS register\n");
				goto end;
			}

#ifdef MY_DEF_HERE
			setbits32(&guts->dscr, CCSR_GUTS_DSCR_ENB_PWR_DWN|CCSR_GUTS_DSCR_TRI_MCS_B|CCSR_GUTS_DSCR_TRI_MCK|0x40000000);
#else
			setbits32(&guts->dscr, CCSR_GUTS_DSCR_ENB_PWR_DWN);
#endif

#ifdef MY_DEF_HERE
			 
			gpio_node = of_find_compatible_node(NULL, NULL,
					"fsl,mpc8572-gpio");
			if (!gpio_node) {
				printk(KERN_WARNING "Cannot find fsl,mpc8572-gpio entry\n");
				goto end;
			}

			gpio_base = of_iomap(gpio_node, 0);
			of_node_put(gpio_node);
			if (!gpio_base) {
				printk(KERN_WARNING "Cannot map gpio base of fsl,mpc8572-gpio\n");
				goto end;
			}
#endif
		}
	}

end:
	pmc_dev = &ofdev->dev;
	suspend_set_ops(&pmc_suspend_ops);
	return 0;
}

static const struct of_device_id pmc_ids[] = {
	{ .compatible = "fsl,mpc8548-pmc", },
	{ .compatible = "fsl,mpc8641d-pmc", },
	{ },
};

static struct of_platform_driver pmc_driver = {
	.driver.name = "fsl-pmc",
	.match_table = pmc_ids,
	.probe = pmc_probe,
};

static int __init pmc_init(void)
{
	return of_register_platform_driver(&pmc_driver);
}
device_initcall(pmc_init);
#endif  
