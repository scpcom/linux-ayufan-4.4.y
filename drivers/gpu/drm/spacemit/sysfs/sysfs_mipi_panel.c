// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Spacemit Co., Ltd.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <video/videomode.h>

#include "../spacemit_lib.h"
#include "../spacemit_mipi_panel.h"
#include "sysfs_display.h"


int spacemit_mipi_panel_sysfs_init(struct device *dev)
{
	return 0;
}
EXPORT_SYMBOL(spacemit_mipi_panel_sysfs_init);

MODULE_DESCRIPTION("Provide panel attribute nodes for userspace");
MODULE_LICENSE("GPL v2");
