#include <linux/printk.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#define LS1024A_GPIO_OUTPUT_REG			0x00
#define LS1024A_GPIO_OE_REG			0x04
#define LS1024A_GPIO_INT_CFG_REG		0x08
#define LS1024A_GPIO_INPUT_REG			0x10
#define LS1024A_GPIO_63_32_PIN_OUTPUT		0xD0
#define LS1024A_GPIO_63_32_PIN_OUTPUT_EN	0xD4
#define LS1024A_GPIO_63_32_PIN_INPUT		0xD8

#define LS1024A_GPIO_NUM_IRQS	8

struct ls1024a_gpio_chip {
	struct gpio_chip gc;
	struct irq_chip	*irqchip;
	struct irq_domain *irq_domain;

	void __iomem *reg;

	struct of_phandle_args parent_phandle_args;

	/*
	 * Used to lock ls1024a_gpio_chip->data. Also, this is needed to keep
	 * shadowed and real data registers writes together.
	 */
	spinlock_t lock;

	/* Shadowed data register to clear/set bits safely. */
	u32 data[2];

	/* Shadowed direction registers to clear/set direction safely. */
	u32 dir[2];
};

static inline struct ls1024a_gpio_chip *to_ls1024a_gpio_chip(struct gpio_chip *gc)
{
	return container_of(gc, struct ls1024a_gpio_chip, gc);
}

static int ls1024a_gpio_get(struct gpio_chip *gc, unsigned pin)
{
	u32 val;
	struct ls1024a_gpio_chip *lgc = to_ls1024a_gpio_chip(gc);

	if (pin >= gc->ngpio)
		return -EINVAL;

	if (pin < 32) {
		val = readl(lgc->reg + LS1024A_GPIO_INPUT_REG);
	} else {
		val = readl(lgc->reg + LS1024A_GPIO_63_32_PIN_INPUT);
	}

	if (val & BIT_MASK(pin))
		return 1;

	return 0;
}

static void ls1024a_gpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct ls1024a_gpio_chip *lgc = to_ls1024a_gpio_chip(gc);
	unsigned long flags;

	if (gpio >= gc->ngpio)
		return;

	spin_lock_irqsave(&lgc->lock, flags);

	if (val)
		lgc->data[BIT_WORD(gpio)] |= BIT_MASK(gpio);
	else
		lgc->data[BIT_WORD(gpio)] &= ~BIT_MASK(gpio);

	writel(lgc->data[0], lgc->reg + LS1024A_GPIO_OUTPUT_REG);
	writel(lgc->data[1], lgc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT);

	spin_unlock_irqrestore(&lgc->lock, flags);
}

static int ls1024a_gpio_set_direction(struct gpio_chip *gc, unsigned int gpio, int out) {
	struct ls1024a_gpio_chip *lgc = to_ls1024a_gpio_chip(gc);
	unsigned long flags;

	if (gpio >= gc->ngpio)
		return -EINVAL;

	spin_lock_irqsave(&lgc->lock, flags);

	if (out)
		lgc->dir[BIT_WORD(gpio)] |= BIT_MASK(gpio);
	else
		lgc->dir[BIT_WORD(gpio)] &= ~BIT_MASK(gpio);

	writel(lgc->dir[0], lgc->reg + LS1024A_GPIO_OE_REG);
	/* LS1024A_GPIO_63_32_PIN_OUTPUT_EN uses inverse logic */
	writel(~lgc->dir[1], lgc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT_EN);

	spin_unlock_irqrestore(&lgc->lock, flags);

	return 0;
}

static int ls1024a_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	return ls1024a_gpio_set_direction(gc, gpio, 0);
}

static int ls1024a_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	ls1024a_gpio_set(gc, gpio, val);
	return ls1024a_gpio_set_direction(gc, gpio, 1);
}

static int ls1024a_gpio_request(struct gpio_chip *chip, unsigned gpio_pin)
{
	if (gpio_pin < chip->ngpio)
		return 0;

	return -EINVAL;
}

static int ls1024a_set_type(struct irq_data *data, unsigned int type)
{
	struct ls1024a_gpio_chip *lgc = irq_data_get_irq_chip_data(data);
	int ls1024a_type;
	int ret;
	unsigned long flags;
	u32 value;
	switch(type) {
		case IRQ_TYPE_EDGE_RISING:
			ls1024a_type = 0x2;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			ls1024a_type = 0x1;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			ls1024a_type = 0x3;
			break;
		default:
			WARN_ON(1);
			/* Fall through */
		case IRQ_TYPE_NONE:
			ls1024a_type = 0x0;
			break;
	}
	spin_lock_irqsave(&lgc->lock, flags);
	value = readl(lgc->reg + LS1024A_GPIO_INT_CFG_REG);
	value &= ~(0x3 << data->hwirq*2);
	value |= ls1024a_type << data->hwirq*2;
	writel(value, lgc->reg + LS1024A_GPIO_INT_CFG_REG);
	spin_unlock_irqrestore(&lgc->lock, flags);

	data = data->parent_data;
	ret = data->chip->irq_set_type(data, type);
	return ret;
}

static struct irq_chip ls1024a_gpio_irq_chip = {
	.name			= "GPIO",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_set_type		= ls1024a_set_type,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
};

static int ls1024a_domain_alloc(struct irq_domain *domain, unsigned int virq,
				   unsigned int nr_irqs, void *arg)
{
	int i, ret;
	irq_hw_number_t hwirq;
	unsigned int type;
	struct of_phandle_args *irq_data = arg;
	struct of_phandle_args gic_data = *irq_data;
	struct ls1024a_gpio_chip *lgc = domain->host_data;

	if (irq_data->args_count != 2)
		return -EINVAL;

	ret = domain->ops->xlate(domain, irq_data->np, irq_data->args,
			irq_data->args_count, &hwirq, &type);
	if (ret)
		return ret;
	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
					      &ls1024a_gpio_irq_chip,
					      domain->host_data);
	gic_data = lgc->parent_phandle_args;
	gic_data.args[1] += hwirq;
	gic_data.args[2] = type;
	return irq_domain_alloc_irqs_parent(domain, virq, nr_irqs, &gic_data);
}

static struct irq_domain_ops ls1024a_domain_ops = {
	.xlate = irq_domain_xlate_twocell,
	.alloc = ls1024a_domain_alloc,
	.free = irq_domain_free_irqs_common,
};

static int ls1024a_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *res;
	struct ls1024a_gpio_chip *lgc;
	struct irq_domain *domain_parent;
	int rc;

	lgc = devm_kzalloc(dev, sizeof(*lgc), GFP_KERNEL);
	if (!lgc)
		return -ENOMEM;

	lgc->gc.dev = dev;
#ifdef CONFIG_OF_GPIO
	lgc->gc.of_node = of_node_get(node);
#endif
	spin_lock_init(&lgc->lock);
	lgc->gc.dev = dev;
	lgc->gc.label = dev_name(dev);
	lgc->gc.base = 0;
	lgc->gc.ngpio = 64;
	lgc->gc.request = ls1024a_gpio_request;

	lgc->gc.direction_input = ls1024a_dir_in;
	lgc->gc.direction_output = ls1024a_dir_out;
	lgc->gc.set = ls1024a_gpio_set;
	lgc->gc.get = ls1024a_gpio_get;
	lgc->gc.can_sleep = false;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lgc->reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(lgc->reg)) {
		rc = PTR_ERR(lgc->reg);
		goto err;
	}

	platform_set_drvdata(pdev, lgc);

	rc = of_irq_parse_one(node, 0, &lgc->parent_phandle_args);
	if (rc) {
		goto err_iounmap;
	}
	domain_parent = irq_find_host(lgc->parent_phandle_args.np);
	if (!domain_parent) {
		dev_err(dev, "cannot find irq parent domain\n");
		rc = -EPROBE_DEFER;
		goto err_iounmap;
	}
	lgc->irq_domain = irq_domain_add_hierarchy(domain_parent, 0,
			LS1024A_GPIO_NUM_IRQS, node,
			&ls1024a_domain_ops, lgc);
	if (!lgc->irq_domain) {
		rc = -ENOMEM;
		goto err_iounmap;
	}

	lgc->dir[0] = readl(lgc->reg + LS1024A_GPIO_OE_REG);
	/* LS1024A_GPIO_63_32_PIN_OUTPUT_EN uses inverse logic */
	lgc->dir[1] = ~readl(lgc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT_EN);
	lgc->data[0] = readl(lgc->reg + LS1024A_GPIO_OUTPUT_REG);
	lgc->data[1] = readl(lgc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT);

	lgc->gc.owner = THIS_MODULE;

	rc = gpiochip_add(&lgc->gc);
	if (rc)
		goto err_remove_domain;
	return 0;

err_remove_domain:
	irq_domain_remove(lgc->irq_domain);
err_iounmap:
	devm_iounmap(dev, lgc->reg);
err:
	platform_set_drvdata(pdev, NULL);
	devm_kfree(dev, lgc);
	return rc;
}

static const struct of_device_id ls1024a_gpio_match[] = {
	{
		.compatible = "fsl,ls1024a-gpio",
		.data = NULL,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, ls1024a_gpio_match);

static struct platform_driver ls1024a_gpio_driver = {
	.probe		= ls1024a_gpio_probe,
	.driver		= {
		.name	= "ls1024a_gpio",
		.of_match_table = of_match_ptr(ls1024a_gpio_match),
	},
};

module_platform_driver(ls1024a_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_DESCRIPTION("Freescale LS1024A GPIO driver");
