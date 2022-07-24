// SPDX-License-Identifier: GPL-2.0-only
/*
* linux/drivers/remoteproc/sunxi_rproc_srm_dev.c
*
* Copyright © 2020-2025, wujiayi
*              Author: wujiayi <wujiayi@allwinnertech.com>
*
* This file is provided under a dual BSD/GPL license.  When using or
* redistributing this file, you may do so under either license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/component.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/remoteproc.h>

#include "sunxi_rproc_srm_core.h"

struct rproc_srm_dev {
	struct device *dev;
	struct rproc_srm_core *core;
	struct notifier_block nb;
	bool early_boot;
};

struct rproc_srm_component {
	char *name;
	struct component_ops ops;
};

/* Core */
static int rproc_srm_dev_notify_cb(struct notifier_block *nb, unsigned long evt,
				   void *data)
{
	struct rproc_srm_dev *rproc_srm_dev =
			container_of(nb, struct rproc_srm_dev, nb);
	struct device *dev = rproc_srm_dev->dev;
	struct rpmsg_srm_msg_desc *desc;
	struct rpmsg_srm_msg *i, o;
	int ret = 0;

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);
	desc = (struct rpmsg_srm_msg_desc *)data;
	i = desc->msg;
	o = *i;

	/* Check if 'device_id' (name / addr ) matches this device */
	if (!strstr(dev_name(dev), i->device_id))
		return NOTIFY_DONE;

	switch (i->message_type) {
	case RPROC_SRM_MSG_SETCONFIG:
		switch (i->rsc_type) {
		case RPROC_SRM_RSC_CLOCK:
			break;
		case RPROC_SRM_RSC_REGU:
			break;
		default:
			dev_warn(dev, "bad rsc type (%d)\n", i->rsc_type);
			ret = -EINVAL;
			break;
		}
		break;
	case RPROC_SRM_MSG_GETCONFIG:
		switch (i->rsc_type) {
		case RPROC_SRM_RSC_CLOCK:
			break;
		case RPROC_SRM_RSC_REGU:
			break;
		default:
			dev_warn(dev, "bad rsc type (%d)\n", i->rsc_type);
			ret = -EINVAL;
			break;
		}
		break;
	default:
		dev_warn(dev, "bad msg type (%d)\n", i->message_type);
		ret = -EINVAL;
		break;
	}

	/* Send return msg */
	if (ret)
		o.message_type = RPROC_SRM_MSG_ERROR;

	ret = rpmsg_srm_send(desc->ept, &o);
	return ret ? NOTIFY_BAD : NOTIFY_STOP;
}

static int
rproc_srm_uart_dev_bind(struct device *dev, struct device *master, void *data)
{
	/* if get rproc_srm_dev msg, we can use api
	* struct rproc_srm_dev *rproc_srm_dev = dev_get_drvdata(dev);
	*/
	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);
	return 0;
}

static void
rproc_srm_uart_dev_unbind(struct device *dev, struct device *master, void *data)
{
	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);
}

static int
rproc_srm_gpio_int_dev_bind(struct device *dev, struct device *master, void *data)
{

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);
	return 0;
}

static void
rproc_srm_gpio_int_dev_unbind(struct device *dev, struct device *master, void *data)
{
	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);
}

static struct rproc_srm_component rproc_srm_component_ops[] = {
	{
		.name = "d_uart",
		.ops = {
			.bind = rproc_srm_uart_dev_bind,
			.unbind = rproc_srm_uart_dev_unbind,
		}
	},
	{
		.name = "d_gpio_interrupt",
		.ops = {
			.bind = rproc_srm_gpio_int_dev_bind,
			.unbind = rproc_srm_gpio_int_dev_unbind,
		}
	},
};

static int rproc_srm_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rproc_srm_dev *rproc_srm_dev;
	struct rproc *rproc;
	int ret, i;

	dev_dbg(dev, "(%s:%d) for node %s\n", __func__, __LINE__, dev->of_node->name);

	rproc_srm_dev = devm_kzalloc(dev, sizeof(struct rproc_srm_dev),
				     GFP_KERNEL);
	if (!rproc_srm_dev)
		return -ENOMEM;

	rproc_srm_dev->dev = dev;

	rproc = (struct rproc *)dev_get_drvdata(dev->parent->parent);

	/*Fix: Consider the dsp early boot case */
	rproc_srm_dev->early_boot = false;

	rproc_srm_dev->core = dev_get_drvdata(dev->parent);
	if (!rproc_srm_dev->core)
		return -ENOMEM;

	rproc_srm_dev->nb.notifier_call = rproc_srm_dev_notify_cb;

	ret = rproc_srm_core_register_notifier(rproc_srm_dev->core,
					       &rproc_srm_dev->nb);

	if (ret)
		goto err_register;

	dev_set_drvdata(dev, rproc_srm_dev);

	/* match rproc_srm_component_ops with dev->of_node->name */
	ret = -1;
	for (i = 0; i < ARRAY_SIZE(rproc_srm_component_ops); i++) {
		if (strcmp(dev->of_node->name, rproc_srm_component_ops[i].name) == 0) {
			ret = component_add(dev, &rproc_srm_component_ops[i].ops);
			dev_dbg(dev, "(%s:%d) match ops with %s, ret = %d\n",
					__func__, __LINE__, dev->of_node->name, ret);
			break;
		}
	}
	if (i == ARRAY_SIZE(rproc_srm_component_ops))
		dev_err(dev, "(%s:%d) fail to match ops with %s\n",
				__func__, __LINE__, dev->of_node->name);

	return ret;

err_register:
	rproc_srm_core_unregister_notifier(rproc_srm_dev->core,
					   &rproc_srm_dev->nb);

	return ret;
}

static int rproc_srm_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rproc_srm_dev *rproc_srm_dev = dev_get_drvdata(dev);
	int i = 0;
	dev_dbg(dev, "%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(rproc_srm_component_ops); i++) {
		if (strcmp(dev->of_node->name, rproc_srm_component_ops[i].name) == 0) {
			component_del(dev, &rproc_srm_component_ops[i].ops);
			dev_dbg(dev, "(%s:%d) del ops with %s\n",
					__func__, __LINE__, dev->of_node->name);
			break;
		}
	}
	rproc_srm_core_unregister_notifier(rproc_srm_dev->core,
					   &rproc_srm_dev->nb);

	return 0;
}

static const struct of_device_id rproc_srm_dev_match[] = {
	{ .compatible = "allwinner,dsp-rproc-srm-dev", },
	{},
};

MODULE_DEVICE_TABLE(of, rproc_srm_dev_match);

static struct platform_driver rproc_srm_dev_driver = {
	.probe = rproc_srm_dev_probe,
	.remove = rproc_srm_dev_remove,
	.driver = {
		.name = "dsp-rproc-srm-dev",
		.of_match_table = of_match_ptr(rproc_srm_dev_match),
	},
};

module_platform_driver(rproc_srm_dev_driver);

MODULE_DESCRIPTION("Allwinnertech Remoteproc System Resource Manager driver - dev");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
