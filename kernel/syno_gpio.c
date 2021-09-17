#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Synology NAS Board GPIO Setup
 *
 * Maintained by:  Comsumer Platform Team <cpt@synology.com>
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
#include <linux/slab.h>
#include <linux/synobios.h>
#include <linux/syno_gpio.h>

SYNO_GPIO syno_gpio = {
	.fan_ctrl =NULL,
	.fan_fail =NULL,
	.hdd_fail_led =NULL,
	.hdd_present_led =NULL,
	.hdd_act_led =NULL,
	.hdd_detect =NULL,
	.hdd_enable =NULL,
	.model_id =NULL,
	.alarm_led =NULL,
	.power_led =NULL,
	.disk_led_ctrl =NULL,
	.phy_led_ctrl =NULL,
	.copy_button_detect =NULL,
};
EXPORT_SYMBOL(syno_gpio);

int SYNO_GPIO_READ(int pin)
{
#if defined(MY_DEF_HERE)
	int iVal=0;
	syno_gpio_value_get(pin, &iVal);
	return iVal;
#else
	return gpio_get_value(pin);
#endif
}
EXPORT_SYMBOL(SYNO_GPIO_READ);

void SYNO_GPIO_WRITE(int pin, int pValue)
{
#if defined(MY_DEF_HERE)
	syno_gpio_value_set(pin, pValue);
#else
	gpio_set_value(pin, pValue);
#endif
}
EXPORT_SYMBOL(SYNO_GPIO_WRITE);
