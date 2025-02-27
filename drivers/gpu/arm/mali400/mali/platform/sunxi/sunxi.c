
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/cma.h>
#include <linux/delay.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#include <linux/dma-contiguous.h>
#endif
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include <linux/clk/clk-conf.h>

#include <linux/mali/mali_utgard.h>

#include "mali_kernel_linux.h"

struct mali {
	struct clk		*bus_clk;
	struct clk		*core_clk;

	struct reset_control	*reset;

	struct platform_device	*dev;
};

struct mali *mali;

static bool mali_has_reset_line(struct device_node *np)
{
	return of_device_is_compatible(np, "allwinner,sun4i-a10-mali") ||
		of_device_is_compatible(np, "allwinner,sun7i-a20-mali") ||
		of_device_is_compatible(np, "allwinner,sun8i-r40-mali") ||
		of_device_is_compatible(np, "allwinner,sun8i-h3-mali") ||
		of_device_is_compatible(np, "allwinner,sun50i-a64-mali") ||
		of_device_is_compatible(np, "allwinner,sun50i-h5-mali");
}

struct resource *mali_create_mp1_resources(unsigned long address,
					   int irq_gp, int irq_gpmmu,
					   int irq_pp0, int irq_ppmmu0,
					   int *len)
{
	struct resource target[] = {
		MALI_GPU_RESOURCES_MALI400_MP1_PMU(address,
						   irq_gp, irq_gpmmu,
						   irq_pp0, irq_ppmmu0)
	};
	struct resource *res;

	res = kzalloc(sizeof(target), GFP_KERNEL);
	if (!res)
		return NULL;

	memcpy(res, target, sizeof(target));

	*len = ARRAY_SIZE(target);

	return res;
}

struct resource *mali_create_mp2_resources(unsigned long address,
					   int irq_gp, int irq_gpmmu,
					   int irq_pp0, int irq_ppmmu0,
					   int irq_pp1, int irq_ppmmu1,
					   int *len)
{
	struct resource target[] = {
		MALI_GPU_RESOURCES_MALI400_MP2_PMU(address,
						   irq_gp, irq_gpmmu,
						   irq_pp0, irq_ppmmu0,
						   irq_pp1, irq_ppmmu1)
	};
	struct resource *res;

	res = kzalloc(sizeof(target), GFP_KERNEL);
	if (!res)
		return NULL;

	memcpy(res, target, sizeof(target));

	*len = ARRAY_SIZE(target);

	return res;
}

struct resource *mali_create_mali450_mp4_resources(unsigned long address,
						   int irq_gp, int irq_gpmmu,
						   int irq_pp,
						   int irq_pp0, int irq_ppmmu0,
						   int irq_pp1, int irq_ppmmu1,
						   int irq_pp2, int irq_ppmmu2,
						   int irq_pp3, int irq_ppmmu3,
						   int *len)
{
	struct resource target[] = {
		MALI_GPU_RESOURCES_MALI450_MP4_PMU(address,
						   irq_gp, irq_gpmmu,
						   irq_pp0, irq_ppmmu0,
						   irq_pp1, irq_ppmmu1,
						   irq_pp2, irq_ppmmu2,
						   irq_pp3, irq_ppmmu3, irq_pp)
	};
	struct resource *res;

	res = kzalloc(sizeof(target), GFP_KERNEL);
	if (!res)
		return NULL;

	memcpy(res, target, sizeof(target));

	*len = ARRAY_SIZE(target);

	return res;
}

static const struct of_device_id mali_dt_ids[] = {
	{ .compatible = "allwinner,sun4i-a10-mali" },
	{ .compatible = "allwinner,sun7i-a20-mali" },
	{ .compatible = "allwinner,sun8i-r40-mali" },
	{ .compatible = "allwinner,sun8i-h3-mali" },
	{ .compatible = "allwinner,sun50i-a64-mali" },
	{ .compatible = "allwinner,sun50i-h5-mali" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mali_dt_ids);

int mali_platform_device_register(void)
{
	struct mali_gpu_device_data mali_gpu_data = {
		.fb_start		= 0,
		.fb_size		= 0xfffff000,
	};
	int irq_gp, irq_gpmmu, irq_pp;
	int irq_pp0, irq_ppmmu0;
	int irq_pp1 = -EINVAL, irq_ppmmu1 = -EINVAL;
	int irq_pp2 = -EINVAL, irq_ppmmu2 = -EINVAL;
	int irq_pp3 = -EINVAL, irq_ppmmu3 = -EINVAL;
	int irq_pmu;
	struct resource *mali_res = NULL, res;
	struct device_node *np;
	struct device *dev;
	int ret, len;

	np = of_find_matching_node(NULL, mali_dt_ids);
	if (!np) {
		pr_err("Couldn't find the mali node\n");
		return -ENODEV;
	}

	mali = kzalloc(sizeof(*mali), GFP_KERNEL);
	if (!mali) {
		ret = -ENOMEM;
		goto err_put_node;
	}

	ret = of_clk_set_defaults(np, false);
	if (ret) {
		pr_err("Couldn't set clock defaults\n");
		goto err_free_mem;
	}

	mali->bus_clk = of_clk_get_by_name(np, "bus");
	if (IS_ERR(mali->bus_clk)) {
		pr_err("Couldn't retrieve our bus clock\n");
		ret = PTR_ERR(mali->bus_clk);
		goto err_free_mem;
	}
	clk_prepare_enable(mali->bus_clk);

	mali->core_clk = of_clk_get_by_name(np, "core");
	if (IS_ERR(mali->core_clk)) {
		pr_err("Couldn't retrieve our module clock\n");
		ret = PTR_ERR(mali->core_clk);
		goto err_put_bus;
	}
	clk_prepare_enable(mali->core_clk);

	if (mali_has_reset_line(np)) {
		mali->reset = of_reset_control_get(np, NULL);
		if (IS_ERR(mali->reset)) {
			pr_err("Couldn't retrieve our reset handle\n");
			ret = PTR_ERR(mali->reset);
			goto err_put_mod;
		}
		reset_control_deassert(mali->reset);
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		pr_err("Couldn't retrieve our base address\n");
		goto err_put_reset;
	}

	irq_gp = of_irq_get_byname(np, "gp");
	if (irq_gp < 0) {
		pr_err("Couldn't retrieve our GP interrupt\n");
		ret = irq_gp;
		goto err_put_reset;
	}

	irq_pp = of_irq_get_byname(np, "pp");
	if (of_device_is_compatible(np, "arm,mali-450") && (irq_pp < 0)) {
		pr_err("Couldn't retrieve our PP interrupt\n");
		ret = irq_pp;
		goto err_put_reset;
	}

	irq_gpmmu = of_irq_get_byname(np, "gpmmu");
	if (irq_gpmmu < 0) {
		pr_err("Couldn't retrieve our GP MMU interrupt\n");
		ret = irq_gpmmu;
		goto err_put_reset;
	}

	irq_pp0 = of_irq_get_byname(np, "pp0");
	if (irq_pp0 < 0) {
		pr_err("Couldn't retrieve our PP0 interrupt %d\n", irq_pp0);
		ret = irq_pp0;
		goto err_put_reset;
	}

	irq_ppmmu0 = of_irq_get_byname(np, "ppmmu0");
	if (irq_ppmmu0 < 0) {
		pr_err("Couldn't retrieve our PP0 MMU interrupt\n");
		ret = irq_ppmmu0;
		goto err_put_reset;
	}

	irq_pp1 = of_irq_get_byname(np, "pp1");
	irq_ppmmu1 = of_irq_get_byname(np, "ppmmu1");
	if ((irq_pp1 < 0) ^ (irq_ppmmu1 < 0 )) {
		pr_err("Couldn't retrieve our PP1 interrupts\n");
		ret = (irq_pp1 < 0) ? irq_pp1 : irq_ppmmu1;
		goto err_put_reset;
	}

	irq_pp2 = of_irq_get_byname(np, "pp2");
	irq_ppmmu2 = of_irq_get_byname(np, "ppmmu2");
	if ((irq_pp2 < 0) ^ (irq_ppmmu2 < 0 )) {
		pr_err("Couldn't retrieve our PP2 interrupts\n");
		ret = (irq_pp2 < 0) ? irq_pp2 : irq_ppmmu2;
		goto err_put_reset;
	}

	irq_pp3 = of_irq_get_byname(np, "pp3");
	irq_ppmmu3 = of_irq_get_byname(np, "ppmmu3");
	if ((irq_pp3 < 0) ^ (irq_ppmmu3 < 0 )) {
		pr_err("Couldn't retrieve our PP3 interrupts\n");
		ret = (irq_pp3 < 0) ? irq_pp3 : irq_ppmmu3;
		goto err_put_reset;
	}

	irq_pmu = of_irq_get_byname(np, "pmu");
	if (irq_pmu < 0) {
		pr_err("Couldn't retrieve our PMU interrupt\n");
		ret = irq_pmu;
		goto err_put_reset;
	}

	mali->dev = platform_device_alloc("mali-utgard", 0);
	if (!mali->dev) {
		pr_err("Couldn't create platform device\n");
		ret = -EINVAL;
		goto err_put_reset;
	}
	dev = &mali->dev->dev;
	dev_set_name(dev, "mali-utgard");
	dev->of_node = np;
	dev->dma_mask = &dev->coherent_dma_mask;
	dev->coherent_dma_mask = DMA_BIT_MASK(32);
	dev->bus = &platform_bus_type;

	ret = of_reserved_mem_device_init(&mali->dev->dev);
	if (ret && ret != -ENODEV) {
		pr_err("Couldn't claim our reserved memory region\n");
		goto err_free_mem_region;
	}

	if (of_device_is_compatible(np, "arm,mali-450") &&
	    (irq_pp >= 0) &&
	    (irq_pp2 >= 0) && (irq_ppmmu2 >= 0) &&
	    (irq_pp3 >= 0) && (irq_ppmmu3 >= 0))
		mali_res = mali_create_mali450_mp4_resources(res.start,
							     irq_gp, irq_gpmmu,
							     irq_pp,
							     irq_pp0, irq_ppmmu0,
							     irq_pp1, irq_ppmmu1,
							     irq_pp2, irq_ppmmu2,
							     irq_pp3, irq_ppmmu3,
							     &len);
	else if ((irq_pp1 >= 0) && (irq_ppmmu1 >= 0))
		mali_res = mali_create_mp2_resources(res.start,
						     irq_gp, irq_gpmmu,
						     irq_pp0, irq_ppmmu0,
						     irq_pp1, irq_ppmmu1,
						     &len);
	else
		mali_res = mali_create_mp1_resources(res.start,
						     irq_gp, irq_gpmmu,
						     irq_pp0, irq_ppmmu0,
						     &len);

	if (!mali_res) {
		pr_err("Couldn't create target resources\n");
		ret = -EINVAL;
		goto err_free_mem_region;
	}

	if (of_device_is_compatible(np, "allwinner,sun8i-h3-mali") ||
	    of_device_is_compatible(np, "allwinner,sun50i-h5-mali"))
		clk_set_rate(mali->core_clk, 576000000);
	else if (of_device_is_compatible(np, "allwinner,sun7i-a20-mali") ||
		 of_device_is_compatible(np, "allwinner,sun8i-a23-mali") ||
		 of_device_is_compatible(np, "allwinner,sun8i-r40-mali"))
		clk_set_rate(mali->core_clk, 384000000);
	else if (of_device_is_compatible(np, "allwinner,sun50i-a64-mali"))
		clk_set_rate(mali->core_clk, 432000000);

	clk_register_clkdev(mali->core_clk, "clk_mali", NULL);

	ret = platform_device_add_resources(mali->dev, mali_res, len);
	kfree(mali_res);
	if (ret) {
		pr_err("Couldn't add our resources\n");
		goto err_free_mem_region;
	}

	ret = platform_device_add_data(mali->dev, &mali_gpu_data,
				       sizeof(mali_gpu_data));
	if (ret) {
		pr_err("Couldn't add our platform data\n");
		goto err_free_mem_region;
	}

	ret = platform_device_add(mali->dev);
	if (ret) {
		pr_err("Couldn't add our device\n");
		goto err_free_mem_region;
	}

#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	pm_runtime_set_autosuspend_delay(&mali->dev->dev, 1000);
	pm_runtime_use_autosuspend(&mali->dev->dev);
#endif
	pm_runtime_enable(&mali->dev->dev);
#endif

	pr_info("Allwinner sunXi mali glue initialized\n");

	return 0;

err_free_mem_region:
	of_reserved_mem_device_release(dev);
	platform_device_put(mali->dev);
err_put_reset:
	if (!IS_ERR_OR_NULL(mali->reset)) {
		reset_control_assert(mali->reset);
		reset_control_put(mali->reset);
	}
err_put_mod:
	clk_disable_unprepare(mali->core_clk);
	clk_put(mali->core_clk);
err_put_bus:
	clk_disable_unprepare(mali->bus_clk);
	clk_put(mali->bus_clk);
err_free_mem:
	kfree(mali);
err_put_node:
	of_node_put(np);
	return ret;
}

int mali_platform_device_unregister(void)
{
	struct device *dev = &mali->dev->dev;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(dev);
#endif

	of_reserved_mem_device_release(dev);

	platform_device_del(mali->dev);
	of_node_put(dev->of_node);
	platform_device_put(mali->dev);

	if (!IS_ERR_OR_NULL(mali->reset)) {
		reset_control_assert(mali->reset);
		reset_control_put(mali->reset);
	}

	clk_disable_unprepare(mali->core_clk);
	clk_put(mali->core_clk);

	clk_disable_unprepare(mali->bus_clk);
	clk_put(mali->bus_clk);

	kfree(mali);

	return 0;
}
