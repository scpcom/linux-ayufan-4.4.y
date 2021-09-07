#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <asm/prom.h>

int of_get_gpio_flags(struct device_node *np, int index,
		      enum of_gpio_flags *flags)
{
	int ret;
	struct device_node *gc;
	struct of_gpio_chip *of_gc = NULL;
	int size;
	const void *gpio_spec;
	const u32 *gpio_cells;

	ret = of_parse_phandles_with_args(np, "gpios", "#gpio-cells", index,
					  &gc, &gpio_spec);
	if (ret) {
		pr_debug("%s: can't parse gpios property\n", __func__);
		goto err0;
	}

	of_gc = gc->data;
	if (!of_gc) {
		pr_debug("%s: gpio controller %s isn't registered\n",
			 np->full_name, gc->full_name);
		ret = -ENODEV;
		goto err1;
	}

	gpio_cells = of_get_property(gc, "#gpio-cells", &size);
	if (!gpio_cells || size != sizeof(*gpio_cells) ||
			*gpio_cells != of_gc->gpio_cells) {
		pr_debug("%s: wrong #gpio-cells for %s\n",
			 np->full_name, gc->full_name);
		ret = -EINVAL;
		goto err1;
	}

	if (flags)
		*flags = 0;

	ret = of_gc->xlate(of_gc, np, gpio_spec, flags);
	if (ret < 0)
		goto err1;

	ret += of_gc->gc.base;
err1:
	of_node_put(gc);
err0:
	pr_debug("%s exited with status %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(of_get_gpio_flags);

unsigned int of_gpio_count(struct device_node *np)
{
	unsigned int cnt = 0;

	do {
		int ret;

		ret = of_parse_phandles_with_args(np, "gpios", "#gpio-cells",
						  cnt, NULL, NULL);
		 
		if (ret < 0 && ret != -EEXIST)
			break;
	} while (++cnt);

	return cnt;
}
EXPORT_SYMBOL(of_gpio_count);

int of_gpio_simple_xlate(struct of_gpio_chip *of_gc, struct device_node *np,
			 const void *gpio_spec, enum of_gpio_flags *flags)
{
	const u32 *gpio = gpio_spec;

	if (of_gc->gpio_cells < 2) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (*gpio > of_gc->gc.ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpio[1];

	return *gpio;
}
EXPORT_SYMBOL(of_gpio_simple_xlate);

int of_mm_gpiochip_add(struct device_node *np,
		       struct of_mm_gpio_chip *mm_gc)
{
	int ret = -ENOMEM;
	struct of_gpio_chip *of_gc = &mm_gc->of_gc;
	struct gpio_chip *gc = &of_gc->gc;

	gc->label = kstrdup(np->full_name, GFP_KERNEL);
	if (!gc->label)
		goto err0;

	mm_gc->regs = of_iomap(np, 0);
	if (!mm_gc->regs)
		goto err1;

#ifdef MY_DEF_HERE
	gc->base = 0;
#else
	gc->base = -1;
#endif

	if (!of_gc->xlate)
		of_gc->xlate = of_gpio_simple_xlate;

	if (mm_gc->save_regs)
		mm_gc->save_regs(mm_gc);

	np->data = of_gc;

	ret = gpiochip_add(gc);
	if (ret)
		goto err2;

	of_node_get(np);

	pr_debug("%s: registered as generic GPIO chip, base is %d\n",
		 np->full_name, gc->base);
	return 0;
err2:
	np->data = NULL;
	iounmap(mm_gc->regs);
err1:
	kfree(gc->label);
err0:
	pr_err("%s: GPIO chip registration failed with status %d\n",
	       np->full_name, ret);
	return ret;
}
EXPORT_SYMBOL(of_mm_gpiochip_add);
