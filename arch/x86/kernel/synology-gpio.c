#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Synology Evansport NAS Board GPIO Setup
 *
 * Maintained by: KueiHuan Chen <khchen@synology.com>
 *                Yikai Peng <ykpeng@synology.com>
 *
 * Copyright 2009-2013 Synology, Inc.  All rights reserved.
 * Copyright 2009-2013 KueiHuan.Chen
 * Copyright 2009-2013 Yikai Peng
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
#if defined(CONFIG_ARCH_GEN3)

#include <linux/gpio.h>
#include <linux/synobios.h>
#include <linux/export.h>
#include <linux/string.h>

#define GPIO_UNDEF				0xFF

/* copied from synobios.h */
#define DISK_LED_OFF		0
#define DISK_LED_GREEN_SOLID	1
#define DISK_LED_ORANGE_SOLID	2
#define DISK_LED_ORANGE_BLINK	3
#define DISK_LED_GREEN_BLINK    4

#define SYNO_LED_OFF		0
#define SYNO_LED_ON		1
#define SYNO_LED_BLINKING	2

#ifdef  MY_ABC_HERE
extern char gszSynoHWVersion[];
#endif

#define SYNO_DS713_GPP_SCHEDULE_ON		8
#define SYNO_DS713_GPP_HDD1_PWR_EN		9
#define SYNO_DS713_GPP_HDD2_PWR_EN		10
#define SYNO_DS713_GPP_HDD1_FAULTY		11
#define SYNO_DS713_GPP_HDD2_FAULTY		12
#define SYNO_DS713_GPP_HDD1_ACT			13
#define SYNO_DS713_GPP_HDD2_ACT			15
#define SYNO_DS713_GPP_EXT_FAN1_FAIL		16
#define SYNO_DS713_GPP_HDD1_ONLINE		17
#define SYNO_DS713_GPP_HDD2_ONLINE		18
#define SYNO_DS713_GPP_INTER_LOCK		19
#define SYNO_DS713_GPP_LED_EN			34

typedef struct __tag_SYNO_EVANSPORT_HDD_PM_GPIO {
	u8 hdd1_pm;
	u8 hdd2_pm;
} SYNO_EVANSPORT_HDD_PM_GPIO;

typedef struct __tag_SYNO_EVANSPORT_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_fail;
	u8 fan_fail_2;
} SYNO_EVANSPORT_FAN_GPIO;

typedef struct __tag_SYNO_EVANSPORT_EXT_HDD_LED_GPIO {
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 hdd5_led_0;
	u8 hdd5_led_1;
} SYNO_EVANSPORT_EXT_HDD_LED_GPIO;

typedef struct __tag_SYNO_EVANSPORT_SOC_HDD_LED_GPIO {
	u8 hdd1_act_led;
	u8 hdd2_act_led;
	u8 hdd1_fail_led;
	u8 hdd2_fail_led;
}SYNO_EVANSPORT_SOC_HDD_LED_GPIO;

typedef struct __tag_SYNO_EVANSPORT_MULTI_BAY_GPIO {
	u8 inter_lock;
}SYNO_EVANSPORT_MULTI_BAY_GPIO;

typedef struct __tag_SYNO_EVANSPORT_GENERIC_GPIO {
	SYNO_EVANSPORT_EXT_HDD_LED_GPIO		ext_sata_led;
	SYNO_EVANSPORT_SOC_HDD_LED_GPIO		soc_sata_led;
	SYNO_EVANSPORT_FAN_GPIO			fan;
	SYNO_EVANSPORT_HDD_PM_GPIO		hdd_pm;
	SYNO_EVANSPORT_MULTI_BAY_GPIO		multi_bay;
}SYNO_EVANSPORT_GENERIC_GPIO;

static SYNO_EVANSPORT_GENERIC_GPIO generic_gpio;

int
SYNO_EVANSPORT_GPIO_PIN(int pin, int *pValue, int isWrite)
{
	int ret = -1;

	if (!pValue)
		goto END;

	if (1 == isWrite)
		gpio_set_value(pin, *pValue);
	else
		*pValue = gpio_get_value(pin);

	ret = 0;
END:
	return 0;
}

int
SYNO_EVANSPORT_GPIO_BLINK(int pin, int blink)
{
	return 0;
}

int
SYNO_CTRL_INTERNAL_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int fail_led;
	int act_led;

#ifdef MY_ABC_HERE
	extern long g_internal_hd_num;

	if ( 1 >= g_internal_hd_num ) {
		return 0;
	}
#endif

			WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_act_led);
			WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_fail_led);
			WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd2_act_led);
			WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd2_fail_led);

	//note: hd led is active low
	if ( DISK_LED_OFF == status ) {
		fail_led = 1;
		act_led = 1;
	} else if ( DISK_LED_GREEN_SOLID == status ) {
		fail_led = 1;
		act_led = 0;
	} else if ( DISK_LED_ORANGE_SOLID == status ||
		DISK_LED_ORANGE_BLINK == status ) {
		fail_led = 0;
		act_led = 1;
	} else {
		printk("Wrong HDD led status [%d]\n", status);
		goto END;
	}

	switch (index) {
		case 1:
			gpio_set_value(generic_gpio.soc_sata_led.hdd1_act_led, act_led);
			gpio_set_value(generic_gpio.soc_sata_led.hdd1_fail_led, fail_led);
			break;
		case 2:
			gpio_set_value(generic_gpio.soc_sata_led.hdd2_act_led, act_led);
			gpio_set_value(generic_gpio.soc_sata_led.hdd2_fail_led, fail_led);
			break;
		default:
			printk("Wrong HDD number [%d]\n", index);
			goto END;
	}

	ret = 0;
END:
	return ret;
}

int
SYNO_CTRL_EXT_CHIP_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int pin1 = 0, pin2 = 0, bit1 = 0, bit2 = 0;

	bit1 = ( status >> 0 ) & 0x1;
	bit2 = ( status >> 1 ) & 0x1;

	switch (index) {
	case 1:
		pin1 = generic_gpio.ext_sata_led.hdd1_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd1_led_1;
		break;
	case 2:
		pin1 = generic_gpio.ext_sata_led.hdd2_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd2_led_1;
		break;
	case 3:
		pin1 = generic_gpio.ext_sata_led.hdd3_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd3_led_1;
		break;
	case 4:
		pin1 = generic_gpio.ext_sata_led.hdd4_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd4_led_1;
		break;
	case 5:
		if (generic_gpio.ext_sata_led.hdd5_led_0 == GPIO_UNDEF ||
			generic_gpio.ext_sata_led.hdd5_led_1 == GPIO_UNDEF) {
			//some 4 bay model don't contain such gpio.
			ret = 0;
			goto END;
		}
		pin1 = generic_gpio.ext_sata_led.hdd5_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd5_led_1;
		break;
	case 6:
		//for esata
		ret = 0;
		goto END;
	default:
		printk("Wrong HDD number [%d]\n", index);
		goto END;
	}

	WARN_ON(pin1 == GPIO_UNDEF);
	WARN_ON(pin2 == GPIO_UNDEF);

	gpio_set_value(pin1, bit1);
	gpio_set_value(pin2, bit2);

    ret = 0;
END:
    return ret;
}

int SYNO_CTRL_HDD_POWERON(int index, int value)
{
	int ret = -1;

	switch (index) {
	case 1:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd1_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd1_pm, value);
		break;
	case 2:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd2_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd2_pm, value);
		break;
	default:
		goto END;
	}

	ret = 0;
END:
	return ret;
}

int SYNO_CTRL_FAN_PERSISTER(int index, int status, int isWrite)
{
	int ret = 0;
	u8 pin = GPIO_UNDEF;

	switch (index) {
	case 1:
		pin = generic_gpio.fan.fan_1;
		break;
	case 2:
		pin = generic_gpio.fan.fan_2;
		break;
	default:
		ret = -1;
		printk("%s fan not match\n", __FUNCTION__);
		goto END;
	}

	WARN_ON(GPIO_UNDEF == pin);
	gpio_set_value(pin, status);
END:
	return ret;
}

int SYNO_CTRL_FAN_STATUS_GET(int index, int *pValue)
{
	int ret = 0;

	switch (index) {
		case 1:
			WARN_ON(GPIO_UNDEF == generic_gpio.fan.fan_fail);
			*pValue = gpio_get_value(generic_gpio.fan.fan_fail);
			break;
		case 2:
			WARN_ON(GPIO_UNDEF == generic_gpio.fan.fan_fail_2);
			*pValue = gpio_get_value(generic_gpio.fan.fan_fail_2);
			break;
		default:
			WARN_ON(1);
			break;
	}

	if(*pValue)
		*pValue = 0;
	else
		*pValue = 1;

	return ret;
}

u8 SYNOEvansportIsBoardNeedPowerUpHDD(u32 disk_id) {
	u8 ret = 0;

#ifdef  MY_ABC_HERE
	if ( 0 == strncmp(gszSynoHWVersion, HW_DS713, sizeof(HW_DS713)) ) {
		if (2 >= disk_id ) {
			ret = 1;
		}
	}
#endif

	return ret;
}

int SYNO_CTRL_BACKPLANE_STATUS_GET(int *pStatus)
{
	WARN_ON(GPIO_UNDEF == generic_gpio.multi_bay.inter_lock);

	*pStatus = gpio_get_value(generic_gpio.multi_bay.inter_lock);
	return 0;
}

EXPORT_SYMBOL(SYNOEvansportIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_EVANSPORT_GPIO_PIN);
EXPORT_SYMBOL(SYNO_EVANSPORT_GPIO_BLINK);
EXPORT_SYMBOL(SYNO_CTRL_INTERNAL_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_EXT_CHIP_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_HDD_POWERON);
EXPORT_SYMBOL(SYNO_CTRL_FAN_PERSISTER);
EXPORT_SYMBOL(SYNO_CTRL_FAN_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_BACKPLANE_STATUS_GET);

/*
 Pin 		Mode	Signal select and definition	Input/output	Pull-up/pull-down
 MPP[09]		0x0	HDD 0 Power			Out
 MPP[10]		0x0	HDD 1 Power			Out
 MPP[11]		0x0	HDD 0 fail LED			Out
 MPP[12]		0x0	HDD 1 fail LED			Out
 MPP[13]		0x0	HDD 0 Act			Out
 MPP[15]		0x0	HDD 1 Act			Out
 MPP[16]		0x0	Fan Sense			In
 MPP[17]		0x0	HDD 0 Present			In
 MPP[18]		0x0	HDD 1 Present			In
 MPP[19]		0x0	Inter Lock			In
 MPP[34]		0x0	Led Enable			Out
*/
static void
EVANSPORT_713_GPIO_init(SYNO_EVANSPORT_GENERIC_GPIO *global_gpio)
{
	struct gpio gpiocfg_713[] = {
		{ SYNO_DS713_GPP_SCHEDULE_ON, GPIOF_IN, "Schedule ON" },
		{ SYNO_DS713_GPP_HDD1_PWR_EN, GPIOF_OUT_INIT_LOW, "HDD1 PWR EN" },
		{ SYNO_DS713_GPP_HDD2_PWR_EN, GPIOF_OUT_INIT_LOW, "HDD2 PWR EN" },
		{ SYNO_DS713_GPP_HDD1_FAULTY, GPIOF_OUT_INIT_HIGH, "HDD1 Faulty" },
		{ SYNO_DS713_GPP_HDD2_FAULTY, GPIOF_OUT_INIT_HIGH, "HDD2 Faulty" },
		{ SYNO_DS713_GPP_HDD1_ACT, GPIOF_OUT_INIT_HIGH, "HDD1 Act" },
		{ SYNO_DS713_GPP_HDD2_ACT, GPIOF_OUT_INIT_HIGH, "HDD2 Act" },
		{ SYNO_DS713_GPP_EXT_FAN1_FAIL, GPIOF_IN, "Ext Fan1 Fail" },
		{ SYNO_DS713_GPP_HDD1_ONLINE, GPIOF_IN, "HDD1 On-line" },
		{ SYNO_DS713_GPP_HDD2_ONLINE, GPIOF_IN, "HDD2 On-line" },
		{ SYNO_DS713_GPP_INTER_LOCK, GPIOF_IN, "Inter Lock" },
		{ SYNO_DS713_GPP_LED_EN, GPIOF_OUT_INIT_HIGH, "LED Enable" },
	};

	SYNO_EVANSPORT_GENERIC_GPIO gpio_713 = {
		.ext_sata_led	= {
							.hdd1_led_0 = GPIO_UNDEF,
							.hdd1_led_1 = GPIO_UNDEF,
							.hdd2_led_0 = GPIO_UNDEF,
							.hdd2_led_1 = GPIO_UNDEF,
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led	= {
							.hdd1_act_led = SYNO_DS713_GPP_HDD1_ACT,
							.hdd2_act_led = SYNO_DS713_GPP_HDD2_ACT,
							.hdd1_fail_led = SYNO_DS713_GPP_HDD1_FAULTY,
							.hdd2_fail_led = SYNO_DS713_GPP_HDD2_FAULTY,
						},
		.fan		= {
							.fan_1 = GPIO_UNDEF,
							.fan_2 = GPIO_UNDEF,
							.fan_fail = SYNO_DS713_GPP_EXT_FAN1_FAIL,
							.fan_fail_2 = GPIO_UNDEF,
						},
		.hdd_pm		= {
							.hdd1_pm = SYNO_DS713_GPP_HDD1_PWR_EN,
							.hdd2_pm = SYNO_DS713_GPP_HDD2_PWR_EN,
						},
		.multi_bay	= {
							.inter_lock = SYNO_DS713_GPP_INTER_LOCK,
						},
	};

	*global_gpio = gpio_713;

	gpio_request_array(gpiocfg_713, ARRAY_SIZE(gpiocfg_713));
}

static void
EVANSPORT_default_GPIO_init(SYNO_EVANSPORT_GENERIC_GPIO *global_gpio)
{
	SYNO_EVANSPORT_GENERIC_GPIO gpio_default = {
		.ext_sata_led = {
							.hdd1_led_0 = GPIO_UNDEF,
							.hdd1_led_1 = GPIO_UNDEF,
							.hdd2_led_0 = GPIO_UNDEF,
							.hdd2_led_1 = GPIO_UNDEF,
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.fan		  = {
							.fan_1 = GPIO_UNDEF,
							.fan_2 = GPIO_UNDEF,
							.fan_fail = GPIO_UNDEF,
							.fan_fail_2 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
						},
		.multi_bay	= {
							.inter_lock = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_default;
}

void synology_gpio_init(void)
{
#ifdef  MY_ABC_HERE
	if ( 0 == strncmp(gszSynoHWVersion, HW_DS713, strlen(HW_DS713)) ) {
		EVANSPORT_713_GPIO_init(&generic_gpio);
		printk("Synology Evansport 2 bay GPIO Init\n");
	} else {
#endif
		EVANSPORT_default_GPIO_init(&generic_gpio);
		printk("%s: Failed to get model id or model not supported\n", __func__);
#ifdef  MY_ABC_HERE
	}
#endif
}
EXPORT_SYMBOL(synology_gpio_init);
#endif /* CONFIG_ARCH_GEN3 */
