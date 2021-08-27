#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Synology Armada NAS Board GPIO Setup
 *
 * Maintained by:  KueiHuan Chen <khchen@synology.com>
 *
 * Copyright 2009-2013 Synology, Inc.  All rights reserved.
 * Copyright 2009-2013 Chocoyeh
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
#if defined(CONFIG_SYNO_COMCERTO)

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/synobios.h>
#include <linux/export.h>

#define GPIO_UNDEF				0xFF

/* copied from synobios.h */
#define DISK_LED_OFF			0
#define DISK_LED_GREEN_SOLID	1
#define DISK_LED_ORANGE_SOLID	2
#define DISK_LED_ORANGE_BLINK	3
#define DISK_LED_GREEN_BLINK    4

#define SYNO_LED_OFF		0
#define SYNO_LED_ON			1
#define SYNO_LED_BLINKING	2

#ifdef  MY_ABC_HERE
extern char gszSynoHWVersion[];
#endif

typedef struct __tag_SYNO_HDD_DETECT_GPIO {
	u8 hdd1_present_detect;
	u8 hdd2_present_detect;
	u8 hdd3_present_detect;
	u8 hdd4_present_detect;
} SYNO_HDD_DETECT_GPIO;

typedef struct __tag_SYNO_HDD_PM_GPIO {
	u8 hdd1_pm;
	u8 hdd2_pm;
	u8 hdd3_pm;
	u8 hdd4_pm;
} SYNO_HDD_PM_GPIO;

typedef struct __tag_SYNO_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_fail;
	u8 fan_fail_2;
	u8 fan_fail_3;
} SYNO_FAN_GPIO;

typedef struct __tag_SYNO_MODEL_GPIO {
	u8 model_id_0;
	u8 model_id_1;
	u8 model_id_2;
	u8 model_id_3;
} SYNO_MODEL_GPIO;

typedef struct __tag_SYNO_EXT_HDD_LED_GPIO {
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
	u8 hdd_led_mask;
} SYNO_EXT_HDD_LED_GPIO;

typedef struct __tag_SYNO_MULTI_BAY_GPIO {
	u8 inter_lock;
} SYNO_MULTI_BAY_GPIO;

typedef struct __tag_SYNO_SOC_HDD_LED_GPIO {
	u8 hdd2_fail_led;
	u8 hdd1_fail_led;
	u8 hdd2_act_led;
	u8 hdd1_act_led;
} SYNO_SOC_HDD_LED_GPIO;

typedef struct __tag_SYNO_RACK_GPIO {
	u8 buzzer_mute_req;
	u8 buzzer_mute_ack;
	u8 rps1_on;
	u8 rps2_on;
} SYNO_RACK_GPIO;

typedef struct __tag_SYNO_STATUS_LED_GPIO {
	u8 alarm_led;
	u8 power_led;
} SYNO_STATUS_LED_GPIO;

typedef struct __tag_SYNO_GPIO {
	SYNO_HDD_DETECT_GPIO    hdd_detect;
	SYNO_EXT_HDD_LED_GPIO	ext_sata_led;
	SYNO_SOC_HDD_LED_GPIO	soc_sata_led;
	SYNO_MODEL_GPIO			model;
	SYNO_FAN_GPIO			fan;
	SYNO_HDD_PM_GPIO		hdd_pm;
	SYNO_RACK_GPIO			rack;
	SYNO_MULTI_BAY_GPIO		multi_bay;
	SYNO_STATUS_LED_GPIO	status;
}SYNO_GPIO;

static SYNO_GPIO generic_gpio;

unsigned int SynoModelIDGet(SYNO_GPIO *pGpio)
{
	if (GPIO_UNDEF != pGpio->model.model_id_3) {
		return (((gpio_get_value(pGpio->model.model_id_0) ? 1 : 0) << 3) | 
		        ((gpio_get_value(pGpio->model.model_id_1) ? 1 : 0) << 2) | 
		        ((gpio_get_value(pGpio->model.model_id_2) ? 1 : 0) << 1) | 
		        ((gpio_get_value(pGpio->model.model_id_3) ? 1 : 0) << 0));
	} else {
		return (((gpio_get_value(pGpio->model.model_id_0) ? 1 : 0) << 2) |
		        ((gpio_get_value(pGpio->model.model_id_1) ? 1 : 0) << 1) |
		        ((gpio_get_value(pGpio->model.model_id_2) ? 1 : 0) << 0));
	}
}

int
SYNO_COMCERTO2K_GPIO_PIN(int pin, int *pValue, int isWrite)
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

void SYNO_MASK_HDD_LED(int blEnable)
{
	if (GPIO_UNDEF != generic_gpio.ext_sata_led.hdd_led_mask)
		gpio_set_value(generic_gpio.ext_sata_led.hdd_led_mask, blEnable ? 1 : 0);
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
	case 3:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd3_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd3_pm, value);
		break;
	case 4:
		WARN_ON(GPIO_UNDEF == generic_gpio.hdd_pm.hdd4_pm);
		gpio_set_value(generic_gpio.hdd_pm.hdd4_pm, value);
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
	case 3:
		pin = generic_gpio.fan.fan_3;
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
		case 3:
			WARN_ON(GPIO_UNDEF == generic_gpio.fan.fan_fail_3);
			*pValue = gpio_get_value(generic_gpio.fan.fan_fail_3);
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

int SYNO_CTRL_ALARM_LED_SET(int status)
{
	WARN_ON(GPIO_UNDEF == generic_gpio.status.alarm_led);

	gpio_set_value(generic_gpio.status.alarm_led, status);
	return 0;
}

int SYNO_CTRL_BACKPLANE_STATUS_GET(int *pStatus)
{
	WARN_ON(GPIO_UNDEF == generic_gpio.multi_bay.inter_lock);

	*pStatus = gpio_get_value(generic_gpio.multi_bay.inter_lock);
	return 0;
}

int SYNO_CTRL_BUZZER_CLEARED_GET(int *pValue)
{
	int tempVal = 0;

	WARN_ON(GPIO_UNDEF == generic_gpio.rack.buzzer_mute_req);

	tempVal = gpio_get_value(generic_gpio.rack.buzzer_mute_req);
	if ( tempVal ) {
		*pValue = 0;
	} else {
		*pValue = 1;
		tempVal = 1;
	}

	return 0;
}

unsigned char SYNOComcerto2kIsBoardNeedPowerUpHDD(u32 disk_id) {
	u8 ret = 0;

	if(0 == strncmp(gszSynoHWVersion, HW_DS414jv10, strlen(HW_DS414jv10))) {
	    if ( 4 >= disk_id )
			ret = 1;
	} else if(0 == strncmp(gszSynoHWVersion, HW_DS214airv10, strlen(HW_DS214airv10))){
	    if ( 2 >= disk_id )
			ret = 1;
	}

	return ret;
}

int SYNO_CHECK_HDD_PRESENT(int index)
{
    int iPrzVal = 1; /*defult is present*/

    switch (index) {
        case 1:
            if (GPIO_UNDEF != generic_gpio.hdd_detect.hdd1_present_detect) {
                iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd1_present_detect);
            }
            break;
        case 2:
            if (GPIO_UNDEF != generic_gpio.hdd_detect.hdd2_present_detect) {
                iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd2_present_detect);
            }
            break;
        case 3:
            if (GPIO_UNDEF != generic_gpio.hdd_detect.hdd3_present_detect) {
                iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd3_present_detect);
            }
            break;
        case 4:
            if (GPIO_UNDEF != generic_gpio.hdd_detect.hdd4_present_detect) {
                iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd4_present_detect);
            }
            break;
        default:
            break;
    }

    return iPrzVal;
}

int
SYNO_SOC_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int fail_led;
	int act_led;

	WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_fail_led);

	/* assign pin info according to hdd */
	switch (index) {
		case 1:
			act_led = generic_gpio.soc_sata_led.hdd1_act_led;
			fail_led = generic_gpio.soc_sata_led.hdd1_fail_led;
			break;
		case 2:
			act_led = generic_gpio.soc_sata_led.hdd2_act_led;
			fail_led = generic_gpio.soc_sata_led.hdd2_fail_led;
			break;
		default:
			printk("Wrong HDD number [%d]\n", index);
			goto END;
	}

	/* Since faulty led and present led are combined,
	   we need to disable present led when light on faulty's */
	if ( DISK_LED_ORANGE_SOLID == status ||
		 DISK_LED_ORANGE_BLINK == status )
	{
		gpio_set_value(act_led, 0);
		gpio_set_value(fail_led, 1);
	}
	else if ( DISK_LED_GREEN_SOLID == status )
	{
		gpio_set_value(fail_led, 0);
		gpio_set_value(act_led, 1);
	}
	else if (DISK_LED_OFF == status)
	{
		gpio_set_value(fail_led, 0);
		gpio_set_value(act_led, 0);
	}
	else
	{
		printk("Wrong HDD led status [%d]\n", status);
		goto END;
	}

	ret = 0;
END:
	return ret;
}

/* SYNO_SUPPORT_HDD_DYNAMIC_ENABLE_POWER
 * Query support HDD dynamic Power .
 * output: 0 - support, 1 - not support.
 */
int SYNO_SUPPORT_HDD_DYNAMIC_ENABLE_POWER(void)
{
	int iRet = 0;

	/* if exist at least one hdd has enable pin and present detect pin ret=1*/
	if ((GPIO_UNDEF != generic_gpio.hdd_pm.hdd1_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd1_present_detect) ||
			(GPIO_UNDEF != generic_gpio.hdd_pm.hdd2_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd2_present_detect) ||
			(GPIO_UNDEF != generic_gpio.hdd_pm.hdd3_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd3_present_detect) ||
			(GPIO_UNDEF != generic_gpio.hdd_pm.hdd4_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd4_present_detect)) {

		iRet = 1;
	}
	return iRet;
}

EXPORT_SYMBOL(SYNOComcerto2kIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_COMCERTO2K_GPIO_PIN);
EXPORT_SYMBOL(SYNO_MASK_HDD_LED);
EXPORT_SYMBOL(SYNO_CTRL_EXT_CHIP_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_HDD_POWERON);
EXPORT_SYMBOL(SYNO_CTRL_FAN_PERSISTER);
EXPORT_SYMBOL(SYNO_CTRL_FAN_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_ALARM_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_BACKPLANE_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_BUZZER_CLEARED_GET);
EXPORT_SYMBOL(SYNO_CHECK_HDD_PRESENT);
EXPORT_SYMBOL(SYNO_SUPPORT_HDD_DYNAMIC_ENABLE_POWER);
EXPORT_SYMBOL(SYNO_SOC_HDD_LED_SET);

/*
DS414J GPIO config table

Pin     In/Out    Function
00      Out       High = LED Enable , Low = LED Disable
02      Out       High = HDD power enable , Low = HDD power disable
03      Out       High = HDD power enable , Low = HDD power disable
04      Out       High = HDD power enable , Low = HDD power disable
05      Out       High = HDD power enable , Low = HDD power disable
10      Out       Active high
11      Out       Active high
12      Out       Active high
35       In       Pulse => Fan Status is good , Low => Fan Fail
36       In       High = No HDD present , Low = HDD present
37       In       High = No HDD present , Low = HDD present
38       In       High = No HDD present , Low = HDD present
39       In       High = No HDD present , Low = HDD present
40       In       Model ID [2]
41       In       Model ID [1]
42       In       Model ID [0] , DS414j = 0x0
43       In       Pulse => Fan Status is good , Low => Fan Fail
*/

static void
COMCERTO2K_414j_GPIO_init(SYNO_GPIO *global_gpio)
{
	SYNO_GPIO gpio_ds414j = {
		.hdd_detect = {
			.hdd1_present_detect = 39,
			.hdd2_present_detect = 38,
			.hdd3_present_detect = 37,
			.hdd4_present_detect = 36,
		},
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
			.hdd_led_mask = 0,
		},
		.soc_sata_led = {
			.hdd2_fail_led = GPIO_UNDEF,
			.hdd1_fail_led = GPIO_UNDEF,
			.hdd2_act_led = GPIO_UNDEF,
			.hdd1_act_led = GPIO_UNDEF,
		},
		.model		  = {
			.model_id_0 = 42,
			.model_id_1 = 41,
			.model_id_2 = 40,
			.model_id_3 = GPIO_UNDEF,
		},
		.fan		  = {
			.fan_1 = 10,
			.fan_2 = 11,
			.fan_3 = 12,
			.fan_fail = 43,
			.fan_fail_2 = 35,
			.fan_fail_3 = GPIO_UNDEF,
		},
		.hdd_pm		  = {
			.hdd1_pm = 5,
			.hdd2_pm = 4,
			.hdd3_pm = 3,
			.hdd4_pm = 2,
		},
		.rack		  = {
			.buzzer_mute_req = GPIO_UNDEF,
			.buzzer_mute_ack = GPIO_UNDEF,
			.rps1_on = GPIO_UNDEF,
			.rps2_on = GPIO_UNDEF,
		},
		.multi_bay	  = {
			.inter_lock = GPIO_UNDEF,
		},
		.status		  = {
			.power_led = GPIO_UNDEF,
			.alarm_led = GPIO_UNDEF,
		},
	};

	*global_gpio = gpio_ds414j;
}

static void
COMCERTO2K_214air_GPIO_init(SYNO_GPIO *global_gpio)
{
	SYNO_GPIO gpio_ds214air = {
		.hdd_detect = {
			.hdd1_present_detect = 50,
			.hdd2_present_detect = 49,
			.hdd3_present_detect = GPIO_UNDEF,
			.hdd4_present_detect = GPIO_UNDEF,
		},
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
			.hdd2_fail_led = 52,
			.hdd1_fail_led = 53,
			.hdd2_act_led = 3,
			.hdd1_act_led = 2,
		},
		.model		  = {
			.model_id_0 = 48,
			.model_id_1 = 47,
			.model_id_2 = 46,
			.model_id_3 = GPIO_UNDEF,
		},
		.fan		  = {
			.fan_1 = 10,
			.fan_2 = 11,
			.fan_3 = 12,
			.fan_fail = 44,
			.fan_fail_2 = GPIO_UNDEF,
			.fan_fail_3 = GPIO_UNDEF,
		},
		.hdd_pm		  = {
			.hdd1_pm = 5,
			.hdd2_pm = 4,
			.hdd3_pm = GPIO_UNDEF,
			.hdd4_pm = GPIO_UNDEF,
		},
		.rack		  = {
			.buzzer_mute_req = GPIO_UNDEF,
			.buzzer_mute_ack = GPIO_UNDEF,
			.rps1_on = GPIO_UNDEF,
			.rps2_on = GPIO_UNDEF,
		},
		.multi_bay	  = {
			.inter_lock = GPIO_UNDEF,
		},
		.status		  = {
			.power_led = GPIO_UNDEF,
			.alarm_led = GPIO_UNDEF,
		},
	};

	*global_gpio = gpio_ds214air;
}
static void
COMCERTO2K_default_GPIO_init(SYNO_GPIO *global_gpio)
{
	SYNO_GPIO gpio_default = {
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
			.hdd2_act_led = GPIO_UNDEF,
			.hdd1_act_led = GPIO_UNDEF,
		},
		.model		  = {
			.model_id_0 = GPIO_UNDEF,
			.model_id_1 = GPIO_UNDEF,
			.model_id_2 = GPIO_UNDEF,
			.model_id_3 = GPIO_UNDEF,
		},
		.fan		  = {
			.fan_1 = GPIO_UNDEF,
			.fan_2 = GPIO_UNDEF,
			.fan_3 = GPIO_UNDEF,
			.fan_fail = GPIO_UNDEF,
			.fan_fail_2 = GPIO_UNDEF,
			.fan_fail_3 = GPIO_UNDEF,
		},
		.hdd_pm		  = {
			.hdd1_pm = GPIO_UNDEF,
			.hdd2_pm = GPIO_UNDEF,
			.hdd3_pm = GPIO_UNDEF,
			.hdd4_pm = GPIO_UNDEF,
		},
		.rack		  = {
			.buzzer_mute_req = GPIO_UNDEF,
			.buzzer_mute_ack = GPIO_UNDEF,
			.rps1_on = GPIO_UNDEF,
			.rps2_on = GPIO_UNDEF,
		},
		.multi_bay	  = {
			.inter_lock = GPIO_UNDEF,
		},
		.status		  = {
			.power_led = GPIO_UNDEF,
			.alarm_led = GPIO_UNDEF,
		},
	};

	*global_gpio = gpio_default;
}

void synology_gpio_init(void)
{
	if(0 == strncmp(gszSynoHWVersion, HW_DS414jv10, strlen(HW_DS414jv10))) {
		COMCERTO2K_414j_GPIO_init(&generic_gpio);
		printk("Synology %s GPIO Init\n", HW_DS414jv10);
	} else if(0 == strncmp(gszSynoHWVersion, HW_DS214airv10, strlen(HW_DS214airv10))) {
		COMCERTO2K_214air_GPIO_init(&generic_gpio);
		printk("Synology %s GPIO Init\n", HW_DS214airv10);
	} else {
		COMCERTO2K_default_GPIO_init(&generic_gpio);
		printk("Not supported hw version!\n");
	}
}
#endif /* CONFIG_SYNO_COMCERTO2K_ARCH */
