/*
 * ODROID sysfs support for extra feature enhancement
 *
 * Copyright (C) 2014, Hardkernel Co,.Ltd
 *   Author: Charles Park <charles.park@hardkernel.com>
 *   Author: Dongjin Kim <tobetter@gmail.com>
 *
 * This driver has been modified to support ODROID-N1.
 *   Modified by Joy Cho <joycho78@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <asm/setup.h>

MODULE_AUTHOR("Hardkernel Co,.Ltd");
MODULE_DESCRIPTION("SYSFS driver for ODROID hardware");
MODULE_LICENSE("GPL");

static int boot_mode;

/*
 * Discover the boot device within MicroSD or eMMC
 * and return 1 for eMMC, otherwise 0.
 */
enum {
	BOOT_DEVICE_RESERVED = 0,
	BOOT_DEVICE_SD = 1,
	BOOT_DEVICE_EMMC = 2,
	BOOT_DEVICE_NAND = 3,
	BOOT_DEVICE_NVME = 4,
	BOOT_DEVICE_USB = 5,
	BOOT_DEVICE_SPI = 6,
	BOOT_DEVICE_MAX,
};

/*
 * if boot_mode is emmc, return 1
 * else return 0
 */
int board_boot_from_emmc(void)
{
	if (boot_mode == BOOT_DEVICE_EMMC)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(board_boot_from_emmc);

static ssize_t show_bootdev(struct class *class,
		struct class_attribute *attr, char *buf)
{
	const char *boot_dev_name[BOOT_DEVICE_MAX] = {
		"unknown", /* reserved boot device treated as 'unknown' */
		"sd",
		"emmc",
		"nand",
		"nvme",
		"usb",
		"spi"
	};

	return snprintf(buf, PAGE_SIZE, "%s\n",
			boot_dev_name[boot_mode]);
}

static int __init setup_boot_mode(char *str)
{
	if (strncmp("emmc", str, 4) == 0)
		boot_mode = BOOT_DEVICE_EMMC;
	else if (strncmp("sd", str, 2) == 0)
		boot_mode = BOOT_DEVICE_SD;
	else
		boot_mode = BOOT_DEVICE_RESERVED;

	return 1;
}
__setup("storagemedia=", setup_boot_mode);

static struct class_attribute odroid_class_attrs[] = {
	__ATTR(bootdev, 0444, show_bootdev, NULL),
	__ATTR_NULL,
};

static struct class odroid_class = {
	.name = "odroid",
	.owner = THIS_MODULE,
	.class_attrs = odroid_class_attrs,
};

static int odroid_sysfs_probe(struct platform_device *pdev)
{
#ifdef CONFIG_USE_OF
	struct device_node *node;

	if (pdev->dev.of_node)
		node = pdev->dev.of_node;
#endif
	return 0;
}

static  int odroid_sysfs_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int odroid_sysfs_suspend(struct platform_device *dev, pm_message_t state)
{
	pr_info(KERN_INFO "%s\n", __func__);

	return 0;
}

static int odroid_sysfs_resume(struct platform_device *dev)
{
	pr_info(KERN_INFO "%s\n", __func__);

	return  0;
}
#endif

static const struct of_device_id odroid_sysfs_dt[] = {
	{ .compatible = "odroid-sysfs", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, odroid_sysfs_dt);

static struct platform_driver odroid_sysfs_driver = {
	.driver = {
		.name = "odroid-sysfs",
		.owner = THIS_MODULE,
		.of_match_table = odroid_sysfs_dt,
	},
	.probe = odroid_sysfs_probe,
	.remove = odroid_sysfs_remove,
#ifdef CONFIG_PM_SLEEP
	.suspend = odroid_sysfs_suspend,
	.resume = odroid_sysfs_resume,
#endif
};

static int __init odroid_sysfs_init(void)
{
	int error = class_register(&odroid_class);
	if (0 > error)
		return error;

	return platform_driver_register(&odroid_sysfs_driver);
}

static void __exit odroid_sysfs_exit(void)
{
	platform_driver_unregister(&odroid_sysfs_driver);
	class_unregister(&odroid_class);
}

module_init(odroid_sysfs_init);
module_exit(odroid_sysfs_exit);
