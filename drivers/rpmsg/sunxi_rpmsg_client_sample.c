// SPDX-License-Identifier: GPL-2.0-only
/*
 * Remote processor messaging - sample client driver
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 */

/*
 * This file is ported from samples/rpmsg/rpmsg_client_sample.c, added with
 * name service announcement and an example to bind extra rx callbacks to
 * customized extra endpoints.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>

struct instance_data {
	struct rpmsg_device *rpdev;
	struct rpmsg_endpoint *extra_ept;
};

static int rpmsg_sample_cb(struct rpmsg_device *rpdev, void *data, int len,
			   void *priv, u32 src)
{
	int ret;
	int i;

	/* print received data */
	printk("(%s:%d) Receive data (src: 0x%x, len: %d):\n",
			__func__, __LINE__, src, len);
	for (i = 0; i < len; ++i) {
		printk(" 0x%x,", *((u8 *)(data) + i));
	}
	printk("\n");

	/* send received data back to remote */
	ret = rpmsg_send(rpdev->ept, data, len);
	if (ret)
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);

	return 0;
}

static int rpmsg_sample_extra_cb(struct rpmsg_device *rpdev, void *data, int len,
				 void *priv, u32 src)
{
	int i;

	printk("(%s:%d) Receive data (src: 0x%x, len: %d):\n",
			__func__, __LINE__, src, len);
	for (i = 0; i < len; ++i) {
		printk(" 0x%x,", *((u8 *)(data) + i));
	}
	printk("\n");

	return 0;
}

static int rpmsg_sample_probe(struct rpmsg_device *rpdev)
{
	struct instance_data *idata;
	struct rpmsg_endpoint *extra_ept;
	/*
	 * NOTE:
	 *   In the backend of virtio_rpmsg_bus, when we pass the
	 *   rpmsg_channel_info to rpmsg_create_ept() to create endpoints,
	 *   the name and dst will be ignored, and only the src will be used.
	 *
	 *   We can use a specific src address to create endpoint, or for better
	 *   portability, use RPMSG_ADDR_ANY and then an available address will
	 *   be dynamically assigned to it.
	 */
	struct rpmsg_channel_info extra_ept_info = {
		.name = "extra_ept",	/* not used? */
		.src = RPMSG_ADDR_ANY,
		.dst = RPMSG_ADDR_ANY,	/* not used? */
	};

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
					rpdev->src, rpdev->dst);

	/*
	 * Since currently the rpmsg devices can only be dynamically created
	 * by name service announcement, we need to announce the existence of
	 * new device to remote.
	 */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	idata->rpdev = rpdev;

	/*
	 * Optional.
	 *
	 * We can create extra endpoints and bind them to extra rx callback.
	 * When the remote processor sends messages to the address of extra
	 * endpoint, its rx callback will be called.
	 *
	 * Simple rpmsg drivers may not need extra rx callbacks, just ignore this.
	 */
	extra_ept = rpmsg_create_ept(rpdev, rpmsg_sample_extra_cb, NULL, extra_ept_info);
	if (!extra_ept) {
		dev_err(&rpdev->dev, "failed to create extra_ept\n");
		return -1;
	}
	dev_info(&rpdev->dev, "extra_ept addr: 0x%x\n", extra_ept->addr);

	idata->extra_ept = extra_ept;

	dev_set_drvdata(&rpdev->dev, idata);

	return 0;
}

static void rpmsg_sample_remove(struct rpmsg_device *rpdev)
{
	struct instance_data *idata = dev_get_drvdata(&rpdev->dev);

	if (idata->extra_ept)
		rpmsg_destroy_ept(idata->extra_ept);

	dev_info(&rpdev->dev, "rpmsg sample client driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name	= "rpmsg-client-sample" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_sample_id_table,
	.probe		= rpmsg_sample_probe,
	.callback	= rpmsg_sample_cb,
	.remove		= rpmsg_sample_remove,
};
module_rpmsg_driver(rpmsg_sample_client);

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
