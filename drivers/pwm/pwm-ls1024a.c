/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/spinlock.h>
#include <asm/div64.h>

#define NUMBER_OF_PWMS		6

#define CLK_DIV_VALUE		0xFF		/* Maximum value */

#define PWM_VALUE_MASK		GENMASK(19,0)
#define CLK_DIV_ENABLE		BIT(31)
#define PWM_ENABLE		BIT(31)

#define CLK_DIV_CTRL_REG	0
#define ENABLE_MAX(x)		(((x) * 0x8) + 0x8)
#define LOW_DUTY(x)		(((x) * 0x8) + 0xC)

struct ls1024a_pwm {
	struct pwm_chip chip;
	struct device *dev;
	unsigned long scaler;
	unsigned long enabled_pwms;
	void __iomem *base;
	struct clk *clk;
	spinlock_t lock;
};

static inline struct ls1024a_pwm *to_ls1024a_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct ls1024a_pwm, chip);
}

static int ls1024a_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			      int duty_ns, int period_ns)
{
	struct ls1024a_pwm *pc = to_ls1024a_pwm(chip);
	unsigned long flags;
	u32 value;
	unsigned long duty_ticks, period_ticks;

	if (period_ns <= (int) pc->scaler) {
		dev_err(pc->dev, "period %d not supported, minimum %lu\n",
			period_ns, pc->scaler);
		return -EINVAL;
	}

	spin_lock_irqsave(&pc->lock, flags);

	period_ticks = DIV_ROUND_CLOSEST(period_ns, pc->scaler);
	period_ticks = max(period_ticks, (unsigned long) 1) - 1;
	value = readl(pc->base + ENABLE_MAX(pwm->hwpwm));
	value &= ~PWM_VALUE_MASK;
	value |=  period_ticks & PWM_VALUE_MASK;
	writel(value , pc->base + ENABLE_MAX(pwm->hwpwm));

	duty_ticks = DIV_ROUND_CLOSEST(period_ns - duty_ns, pc->scaler);
	duty_ticks = max(duty_ticks, (unsigned long) 1) - 1;
	value = readl(pc->base + LOW_DUTY(pwm->hwpwm));
	value &= ~PWM_VALUE_MASK;
	value |=  duty_ticks & PWM_VALUE_MASK;
	writel(value, pc->base + LOW_DUTY(pwm->hwpwm));

	spin_unlock_irqrestore(&pc->lock, flags);

	return 0;
}

static int ls1024a_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ls1024a_pwm *pc = to_ls1024a_pwm(chip);
	unsigned long flags;
	u32 value;

	spin_lock_irqsave(&pc->lock, flags);

	value = readl(pc->base + ENABLE_MAX(pwm->hwpwm));
	value |= PWM_ENABLE;
	writel(value, pc->base + ENABLE_MAX(pwm->hwpwm));

	pc->enabled_pwms |= BIT(pwm->hwpwm);

	/* Enable clock divider */
	value = readl(pc->base + CLK_DIV_CTRL_REG);
	if (!(value & CLK_DIV_ENABLE)) {
		value = CLK_DIV_ENABLE | CLK_DIV_VALUE;
		writel(value, pc->base + CLK_DIV_CTRL_REG);
	}

	spin_unlock_irqrestore(&pc->lock, flags);

	return 0;
}

static void ls1024a_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ls1024a_pwm *pc = to_ls1024a_pwm(chip);
	unsigned long flags;
	u32 value;

	spin_lock_irqsave(&pc->lock, flags);

	value = readl(pc->base + ENABLE_MAX(pwm->hwpwm));
	value &= ~PWM_ENABLE;
	writel(value, pc->base + ENABLE_MAX(pwm->hwpwm));

	pc->enabled_pwms &= ~BIT(pwm->hwpwm);

	/* If we disabled the last PWM, shut down the clock divider, too. */
	if (!pc->enabled_pwms) {
		value = readl(pc->base + CLK_DIV_CTRL_REG);
		if (value & CLK_DIV_ENABLE) {
			value &= ~CLK_DIV_ENABLE;
			writel(value, pc->base + CLK_DIV_CTRL_REG);
		}
	}

	spin_unlock_irqrestore(&pc->lock, flags);
}

static const struct pwm_ops ls1024a_pwm_ops = {
	.config = ls1024a_pwm_config,
	.enable = ls1024a_pwm_enable,
	.disable = ls1024a_pwm_disable,
	.owner = THIS_MODULE,
};

static int ls1024a_pwm_probe(struct platform_device *pdev)
{
	struct ls1024a_pwm *pc;
	struct resource *res;
	int ret;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	pc->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pc->base))
		return PTR_ERR(pc->base);

	pc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pc->clk)) {
		dev_err(&pdev->dev, "clock not found: %ld\n", PTR_ERR(pc->clk));
		return PTR_ERR(pc->clk);
	}

	ret = clk_prepare_enable(pc->clk);
	if (ret)
		return ret;

	pc->scaler = DIV_ROUND_CLOSEST_ULL((u64) NSEC_PER_SEC *
			(CLK_DIV_VALUE + 1),
			clk_get_rate(pc->clk));

	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &ls1024a_pwm_ops;
	pc->chip.npwm = NUMBER_OF_PWMS;

	spin_lock_init(&pc->lock);

	platform_set_drvdata(pdev, pc);

	ret = pwmchip_add(&pc->chip);
	if (ret < 0)
		goto add_fail;

	return 0;

add_fail:
	clk_disable_unprepare(pc->clk);
	return ret;
}

static int ls1024a_pwm_remove(struct platform_device *pdev)
{
	struct ls1024a_pwm *pc = platform_get_drvdata(pdev);

	clk_disable_unprepare(pc->clk);

	return pwmchip_remove(&pc->chip);
}

static const struct of_device_id ls1024a_pwm_of_match[] = {
	{ .compatible = "fsl,ls1024a-pwm", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ls1024a_pwm_of_match);

static struct platform_driver ls1024a_pwm_driver = {
	.driver = {
		.name = "ls1024a-pwm",
		.of_match_table = ls1024a_pwm_of_match,
	},
	.probe = ls1024a_pwm_probe,
	.remove = ls1024a_pwm_remove,
};
module_platform_driver(ls1024a_pwm_driver);

MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com");
MODULE_DESCRIPTION("Freescale LS1024A PWM driver");
MODULE_LICENSE("GPL v2");
