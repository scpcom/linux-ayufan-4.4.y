#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>

static DEFINE_MUTEX(reset_controller_list_mutex);
static LIST_HEAD(reset_controller_list);

struct reset_control {
	struct reset_controller_dev *rcdev;
	struct device *dev;
	unsigned int id;
};

int of_reset_simple_xlate(struct reset_controller_dev *rcdev,
			  const struct of_phandle_args *reset_spec)
{
	if (WARN_ON(reset_spec->args_count != rcdev->of_reset_n_cells))
		return -EINVAL;

	if (reset_spec->args[0] >= rcdev->nr_resets)
		return -EINVAL;

	return reset_spec->args[0];
}
EXPORT_SYMBOL_GPL(of_reset_simple_xlate);

int reset_controller_register(struct reset_controller_dev *rcdev)
{
	if (!rcdev->of_xlate) {
		rcdev->of_reset_n_cells = 1;
		rcdev->of_xlate = of_reset_simple_xlate;
	}

	mutex_lock(&reset_controller_list_mutex);
	list_add(&rcdev->list, &reset_controller_list);
	mutex_unlock(&reset_controller_list_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(reset_controller_register);

void reset_controller_unregister(struct reset_controller_dev *rcdev)
{
	mutex_lock(&reset_controller_list_mutex);
	list_del(&rcdev->list);
	mutex_unlock(&reset_controller_list_mutex);
}
EXPORT_SYMBOL_GPL(reset_controller_unregister);

int reset_control_reset(struct reset_control *rstc)
{
	if (rstc->rcdev->ops->reset)
		return rstc->rcdev->ops->reset(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_reset);

int reset_control_assert(struct reset_control *rstc)
{
	if (rstc->rcdev->ops->assert)
		return rstc->rcdev->ops->assert(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_assert);

int reset_control_deassert(struct reset_control *rstc)
{
	if (rstc->rcdev->ops->deassert)
		return rstc->rcdev->ops->deassert(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_deassert);

#if defined (MY_DEF_HERE)
 
int reset_control_is_asserted(struct reset_control *rstc)
{
	if (rstc->rcdev->ops->is_asserted)
		return rstc->rcdev->ops->is_asserted(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_is_asserted);

#endif  

#if defined(MY_ABC_HERE)
 
struct reset_control *of_reset_control_get(struct device_node *np,
					   const char *id)
#else  
 
struct reset_control *reset_control_get(struct device *dev, const char *id)
#endif  
{
	struct reset_control *rstc = ERR_PTR(-EPROBE_DEFER);
	struct reset_controller_dev *r, *rcdev;
	struct of_phandle_args args;
	int index = 0;
	int rstc_id;
	int ret;

#if defined(MY_ABC_HERE)
	if (id)
		index = of_property_match_string(np,
						 "reset-names", id);
	ret = of_parse_phandle_with_args(np, "resets", "#reset-cells",
					 index, &args);
#else  
	if (!dev)
		return ERR_PTR(-EINVAL);

	if (id)
		index = of_property_match_string(dev->of_node,
						 "reset-names", id);
	ret = of_parse_phandle_with_args(dev->of_node, "resets", "#reset-cells",
					 index, &args);
#endif  
	if (ret)
		return ERR_PTR(ret);

	mutex_lock(&reset_controller_list_mutex);
	rcdev = NULL;
	list_for_each_entry(r, &reset_controller_list, list) {
		if (args.np == r->of_node) {
			rcdev = r;
			break;
		}
	}
	of_node_put(args.np);

	if (!rcdev) {
		mutex_unlock(&reset_controller_list_mutex);
#if defined (MY_DEF_HERE)
		return ERR_PTR(-EPROBE_DEFER);
#else  
		return ERR_PTR(-ENODEV);
#endif  
	}

	rstc_id = rcdev->of_xlate(rcdev, &args);
	if (rstc_id < 0) {
		mutex_unlock(&reset_controller_list_mutex);
		return ERR_PTR(rstc_id);
	}

	try_module_get(rcdev->owner);
	mutex_unlock(&reset_controller_list_mutex);

	rstc = kzalloc(sizeof(*rstc), GFP_KERNEL);
	if (!rstc) {
		module_put(rcdev->owner);
		return ERR_PTR(-ENOMEM);
	}

#if defined(MY_ABC_HERE)
	rstc->dev = NULL;
#else  
	rstc->dev = dev;
#endif  
	rstc->rcdev = rcdev;
	rstc->id = rstc_id;

	return rstc;
}
#if defined(MY_ABC_HERE)
EXPORT_SYMBOL(of_reset_control_get);

struct reset_control *reset_control_get(struct device *dev, const char *id)
{
	struct reset_control *rstc;

	if (!dev)
		return ERR_PTR(-EINVAL);

	rstc = of_reset_control_get(dev->of_node, id);
	if (IS_ERR(rstc))
		return rstc;

	rstc->dev = dev;

	return rstc;
}
#endif  
EXPORT_SYMBOL_GPL(reset_control_get);

void reset_control_put(struct reset_control *rstc)
{
	if (IS_ERR(rstc))
		return;

	module_put(rstc->rcdev->owner);
	kfree(rstc);
}
EXPORT_SYMBOL_GPL(reset_control_put);

static void devm_reset_control_release(struct device *dev, void *res)
{
	reset_control_put(*(struct reset_control **)res);
}

struct reset_control *devm_reset_control_get(struct device *dev, const char *id)
{
	struct reset_control **ptr, *rstc;

	ptr = devres_alloc(devm_reset_control_release, sizeof(*ptr),
			   GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	rstc = reset_control_get(dev, id);
	if (!IS_ERR(rstc)) {
		*ptr = rstc;
		devres_add(dev, ptr);
	} else {
		devres_free(ptr);
	}

	return rstc;
}
EXPORT_SYMBOL_GPL(devm_reset_control_get);

static int devm_reset_control_match(struct device *dev, void *res, void *data)
{
	struct reset_control **rstc = res;
	if (WARN_ON(!rstc || !*rstc))
		return 0;
	return *rstc == data;
}

void devm_reset_control_put(struct reset_control *rstc)
{
	int ret;

	ret = devres_release(rstc->dev, devm_reset_control_release,
			     devm_reset_control_match, rstc);
	if (ret)
		WARN_ON(ret);
}
EXPORT_SYMBOL_GPL(devm_reset_control_put);

int device_reset(struct device *dev)
{
	struct reset_control *rstc;
	int ret;

	rstc = reset_control_get(dev, NULL);
	if (IS_ERR(rstc))
		return PTR_ERR(rstc);

	ret = reset_control_reset(rstc);

	reset_control_put(rstc);

	return ret;
}
EXPORT_SYMBOL_GPL(device_reset);
