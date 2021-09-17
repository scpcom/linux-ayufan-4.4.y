/*
 * Synology Monaco NAS Board GPIO Setup
 *
 * Maintained by: Comsumer Platform Team <cpt@synology.com>
 *
 * Copyright 2009-2015 Synology, Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/gpio.h>
#include <linux/synobios.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/syno_gpio.h>

extern SYNO_GPIO syno_gpio;
void gpio_info_init(SYNO_GPIO_INFO **gpio_info, SYNO_GPIO_INFO *init_info);
/*
 *  DS216play GPIO config table
 *
 *  Pin     In/Out    Function
 *  114       In       Fan 1 fail
 *  125       In       Model ID 0
 *  126       In       Model ID 1
 *  127       In       Model ID 2
 *  112      Out       HDD 1 power enable
 *  113      Out       PHY LED
 *  115      Out       HDD 2 power enable
 *  118      Out       HDD LED
 *  120      Out       Fan Low
 *  121      Out       Fan Mid
 *  122      Out       Fan High
 */

static SYNO_GPIO_INFO fan_ctrl = {
	.name				= "fan ctrl",
	.nr_gpio			= 3,
	.gpio_port			= {122, 121, 120}, /* MAX MID LOW */
	.gpio_direction		= OUTPUT,
	.gpio_init_value	= INIT_KEEP_VALUE,
	.gpio_polarity		= ACTIVE_HIGH,
};
static SYNO_GPIO_INFO fan_fail = {
	.name				= "fan sense",
	.nr_gpio			= 1,
	.gpio_port			= {114},
	.gpio_direction		= INPUT,
	.gpio_polarity		= ACTIVE_HIGH,
};
static SYNO_GPIO_INFO hdd_enable = {
	.name				= "sata power",
	.nr_gpio			= 2,
	.gpio_port			= {112, 115},
	.gpio_direction		= OUTPUT,
	.gpio_init_value	= INIT_KEEP_VALUE,
	.gpio_polarity		= ACTIVE_HIGH,
};
static SYNO_GPIO_INFO model_id = {
	.name				= "model id",
	.nr_gpio			= 3,
	.gpio_port			= {125, 126, 127},
	.gpio_direction		= INPUT,
	.gpio_polarity		= ACTIVE_HIGH,
};
static SYNO_GPIO_INFO disk_led_ctrl = {
	.name				= "disk led ctrl",
	.nr_gpio			= 1,
	.gpio_port			= {118},
	.gpio_direction		= OUTPUT,
	.gpio_init_value	= INIT_LOW,
	.gpio_polarity		= ACTIVE_LOW,
};
static SYNO_GPIO_INFO phy_led_ctrl = {
	.name               = "phy led ctrl",
	.nr_gpio			= 1,
	.gpio_port			= {113},
	.gpio_direction		= OUTPUT,
	.gpio_init_value	= INIT_HIGH,
	.gpio_polarity		= ACTIVE_HIGH,
};

void MONACO_ds216play_GPIO_init(void)
{
	syno_gpio.fan_ctrl 		= &fan_ctrl;
	syno_gpio.fan_fail 		= &fan_fail;
	syno_gpio.hdd_enable 	= &hdd_enable;
	syno_gpio.model_id 		= &model_id;
	syno_gpio.disk_led_ctrl = &disk_led_ctrl;
	syno_gpio.phy_led_ctrl 	= &phy_led_ctrl;
}

static int syno_gpio_device_init(SYNO_GPIO_INFO *gpio_dev,
		struct platform_device *pdev)
{
	u8 i = 0, value = -1;
	int ret = -1;
	const SYNO_GPIO_INFO *gd = gpio_dev;

	if (NULL == gd)
		goto ERROR;

	if ((gd->nr_gpio > 4) || (gd->nr_gpio == 0))
		goto ERROR;

	for(i = 0; i < gd->nr_gpio; i++) {
		ret = devm_gpio_request(&pdev->dev, gd->gpio_port[i], gd->name);
		if (ret)
			goto ERROR;

		if (INIT_KEEP_VALUE == gd->gpio_init_value)
			value = gpio_get_value(gd->gpio_port[i]);
		else
			value = gd->gpio_init_value;

		if (OUTPUT == gd->gpio_direction)
			ret = gpio_direction_output(gd->gpio_port[i], value);
		else if (INPUT == gd->gpio_direction)
			ret = gpio_direction_input(gd->gpio_port[i]);

		if (ret)
			goto ERROR;
	}

	printk(KERN_NOTICE "synology : %16s initialized.\n", gd->name);
	return 0;

ERROR:
	return -1;
}

static int synology_gpio_probe(struct platform_device *pdev)
{
	MONACO_ds216play_GPIO_init();
	syno_gpio_device_init(syno_gpio.fan_ctrl, pdev);
	syno_gpio_device_init(syno_gpio.fan_fail, pdev);
	syno_gpio_device_init(syno_gpio.hdd_enable, pdev);
	syno_gpio_device_init(syno_gpio.model_id, pdev);
	syno_gpio_device_init(syno_gpio.disk_led_ctrl, pdev);
	syno_gpio_device_init(syno_gpio.phy_led_ctrl, pdev);
	return 0;
}
static int synology_gpio_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id of_synology_gpio_match[] = {
       { .compatible = "syno, fan-ctrl", },
       { .compatible = "syno, fan-sense", },
       { .compatible = "syno, soc-hdd-pm", },
       { .compatible = "syno, gpio", },
       {},
};

static struct platform_driver synology_gpio_driver = {
	.probe      = synology_gpio_probe,
	.remove     = synology_gpio_remove,
	.driver     = {
		.name   = "synology-gpio",
		.owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(of_synology_gpio_match),
	},
};

static int __init synology_gpio_init(void)
{
	return platform_driver_register(&synology_gpio_driver);
}
device_initcall(synology_gpio_init);
