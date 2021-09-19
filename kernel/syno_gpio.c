#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/synobios.h>
#include <linux/syno_gpio.h>

#ifdef MY_DEF_HERE
static SYNO_GPIO_INFO hdd_detect = {
	.name           = "hdd detect",
	.nr_gpio        = 4,
	.gpio_port      = {56, 59, 63, 57},
	.gpio_polarity  = ACTIVE_LOW,
};
static SYNO_GPIO_INFO hdd_enable = {
	.name           = "hdd enable",
	.nr_gpio        = 4,
	.gpio_port      = {61, 60, 71, 58},
	.gpio_polarity  = ACTIVE_HIGH,
};
#endif

SYNO_GPIO syno_gpio = {
	.fan_ctrl =NULL,
	.fan_fail =NULL,
	.hdd_fail_led =NULL,
	.hdd_present_led =NULL,
	.hdd_act_led =NULL,
#ifdef MY_DEF_HERE
	.hdd_detect = &hdd_detect,
	.hdd_enable = &hdd_enable,
#else
	.hdd_detect =NULL,
	.hdd_enable =NULL,
#endif
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
