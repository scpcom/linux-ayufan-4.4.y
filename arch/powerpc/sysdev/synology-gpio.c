#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/synobios.h>

#define GPIO_UNDEF				0xFF

#define SATAHC_LED_CONFIG_REG	(SATA_REG_BASE | 0x2C)
#define SATAHC_LED_ACT          0x0
#define SATAHC_LED_ACT_PRESENT  0x4

#define DISK_LED_OFF			0
#define DISK_LED_GREEN_SOLID	1
#define DISK_LED_ORANGE_SOLID	2
#define DISK_LED_ORANGE_BLINK	3
#define DISK_LED_GREEN_BLINK    4

#define SYNO_LED_OFF		0
#define SYNO_LED_ON			1
#define SYNO_LED_BLINKING	2

typedef struct __tag_SYNO_QORIQ_HDD_PM_GPIO {
	u8 hdd1_pm;
	u8 hdd2_pm;
	u8 hdd3_pm;
	u8 hdd4_pm;
	u8 hdd1_present;
	u8 hdd2_present;
	u8 hdd3_present;
	u8 hdd4_present;
} SYNO_QORIQ_HDD_PM_GPIO;

typedef struct __tag_SYNO_QORIQ_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_fail;
	u8 fan_fail_2;
	u8 fan_fail_3;
} SYNO_QORIQ_FAN_GPIO;

typedef struct __tag_SYNO_QORIQ_MODEL_GPIO {
	u8 model_id_0;
	u8 model_id_1;
	u8 model_id_2;
	u8 model_id_3;
} SYNO_QORIQ_MODEL_GPIO;

typedef struct __tag_SYNO_QORIQ_EXT_HDD_LED_GPIO {
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
	u8 disable_disk_led;
} SYNO_QORIQ_EXT_HDD_LED_GPIO;

typedef struct __tag_SYNO_QORIQ_MULTI_BAY_GPIO {
	u8 inter_lock;
}SYNO_QORIQ_MULTI_BAY_GPIO;

typedef struct __tag_SYNO_QORIQ_SOC_HDD_LED_GPIO {
	u8 hdd2_fail_led;
	u8 hdd1_fail_led;
}SYNO_QORIQ_SOC_HDD_LED_GPIO;

typedef struct __tag_SYNO_QORIQ_RACK_GPIO {
	u8 buzzer_mute_req;
	u8 buzzer_mute_ack;
	u8 rps1_on;
	u8 rps2_on;
}SYNO_QORIQ_RACK_GPIO;

typedef struct __tag_SYNO_QORIQ_STATUS_LED_GPIO {
	u8 alarm_led;
	u8 power_led;
} SYNO_QORIQ_STATUS_LED_GPIO;

#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
typedef struct __tag_SYNO_QORIQ_RESET_GPIO {
	u8 deep_wake;
} SYNO_QORIQ_RESET_GPIO;
#endif

#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
typedef struct __tag_SYNO_QORIQ_WAKEUP_GPIO {
	u8 power_btn;
} SYNO_QORIQ_WAKEUP_GPIO;
#endif

typedef struct __tag_SYNO_QORIQ_GENERIC_GPIO {
	SYNO_QORIQ_EXT_HDD_LED_GPIO	ext_sata_led;
	SYNO_QORIQ_SOC_HDD_LED_GPIO	soc_sata_led;
	SYNO_QORIQ_MODEL_GPIO			model;
	SYNO_QORIQ_FAN_GPIO			fan;
	SYNO_QORIQ_HDD_PM_GPIO			hdd_pm;
	SYNO_QORIQ_RACK_GPIO			rack;
	SYNO_QORIQ_MULTI_BAY_GPIO		multi_bay;
	SYNO_QORIQ_STATUS_LED_GPIO		status;
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
	SYNO_QORIQ_RESET_GPIO			hw_reset;
#endif
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
	SYNO_QORIQ_WAKEUP_GPIO			wakeup;
#endif
}SYNO_QORIQ_GENERIC_GPIO;

static SYNO_QORIQ_GENERIC_GPIO generic_gpio;

int
SYNO_QorIQ_GPIO_PIN(int pin, int *pValue, int isWrite)
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
SYNO_CTRL_INTERNAL_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int fail_led;

#ifdef MY_ABC_HERE
	extern long g_internal_hd_num;

	if ( 1 >= g_internal_hd_num ) {
		return 0;
	}
#endif

	WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_fail_led);
	WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd2_fail_led);

	if ( DISK_LED_OFF == status ) {
		fail_led = 1;
	} else if ( DISK_LED_GREEN_SOLID == status ) {
		fail_led = 1;
	} else if ( DISK_LED_ORANGE_SOLID == status ||
		DISK_LED_ORANGE_BLINK == status ) {
		fail_led = 0;
	} else {
		printk("Wrong HDD led status [%d]\n", status);
		goto END;
	}

	switch (index) {
		case 1:
			gpio_set_value(generic_gpio.soc_sata_led.hdd1_fail_led, fail_led);
			break;
		case 2:
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

	switch(status) {
		case DISK_LED_OFF:
			bit1 = bit2 = 1;
			break;
		case DISK_LED_GREEN_BLINK:
		case DISK_LED_GREEN_SOLID:
			bit1 = 0;
			bit2 = 1;
			break;
		case DISK_LED_ORANGE_BLINK:
		case DISK_LED_ORANGE_SOLID:
			bit1 = 1;
			bit2 = 0;
			break;
		default:
			printk("Not supported LED status\n");
			break;
	}

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
			 
			ret = 0;
			goto END;
		}
		pin1 = generic_gpio.ext_sata_led.hdd5_led_0;
		pin2 = generic_gpio.ext_sata_led.hdd5_led_1;
		break;
	case 6:
		 
		ret = 0;
		goto END;
	default:
		printk("Wrong HDD number [%d]\n", index);
		goto END;
	}

	WARN_ON(pin1 == GPIO_UNDEF);
	WARN_ON(pin2 == GPIO_UNDEF);

	if (pin1 == GPIO_UNDEF || pin2 == GPIO_UNDEF) {
		goto END;
	}

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

u8 SYNOQorIQIsBoardNeedPowerUpHDD(u32 disk_id)
{
	u8 ret = 0;

	if (syno_is_hw_version(HW_DS413)) {
		if ( 4 >= disk_id )
			ret = 1;
	}

	if (syno_is_hw_version(HW_DS213pv10)) {
		if ( 2 >= disk_id )
			ret = 1;
	}

	return ret;
}

void SYNOQorIQDiskLed(int blEnable)
{
	if (GPIO_UNDEF != generic_gpio.ext_sata_led.disable_disk_led)
		gpio_set_value(generic_gpio.ext_sata_led.disable_disk_led, blEnable ? 0 : 1);
}

int
SYNO_QORIQ_GPIO_PIN(int pin, int *pValue, int isWrite)
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

#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
int
SYNOQorIQGPIOWakeInterruptClear(void)
{
	int iRet = -1;

	if (GPIO_UNDEF == generic_gpio.wakeup.power_btn) {
		printk("error wakeup pin un-define\n");
		goto END;
	}

	iRet = iGpioInterruptClear(generic_gpio.wakeup.power_btn);

END:
	return iRet;
}
#endif

#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
int
SYNOQorIQHWReset(void)
{
	int iRet = -1;

	if (GPIO_UNDEF == generic_gpio.hw_reset.deep_wake) {
		printk("error reset pin un-define\n");
		goto END;
	}

	iRet = gpio_hw_reset(generic_gpio.hw_reset.deep_wake);

END:
	return iRet;
}
#endif

int SYNO_CHECK_HDD_PRESENT(int index)
{
	int iPrzVal = 1;  

	switch (index) {
		case 1:
			if (GPIO_UNDEF != generic_gpio.hdd_pm.hdd1_present) {
				iPrzVal = !gpio_get_value(generic_gpio.hdd_pm.hdd1_present);
			}
			break;
		case 2:
			if (GPIO_UNDEF != generic_gpio.hdd_pm.hdd2_present) {
				iPrzVal = !gpio_get_value(generic_gpio.hdd_pm.hdd2_present);
			}
			break;
		case 3:
			if (GPIO_UNDEF != generic_gpio.hdd_pm.hdd3_present) {
				iPrzVal = !gpio_get_value(generic_gpio.hdd_pm.hdd3_present);
			}
			break;
		case 4:
			if (GPIO_UNDEF != generic_gpio.hdd_pm.hdd4_present) {
				iPrzVal = !gpio_get_value(generic_gpio.hdd_pm.hdd4_present);
			}
			break;
		default:
			break;
	}

	return iPrzVal;
}

EXPORT_SYMBOL(SYNO_QORIQ_GPIO_PIN);
EXPORT_SYMBOL(SYNOQorIQDiskLed);
EXPORT_SYMBOL(SYNOQorIQIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_QorIQ_GPIO_PIN);
EXPORT_SYMBOL(SYNO_CTRL_INTERNAL_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_EXT_CHIP_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_HDD_POWERON);
EXPORT_SYMBOL(SYNO_CTRL_FAN_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_ALARM_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_BACKPLANE_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_BUZZER_CLEARED_GET);
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
EXPORT_SYMBOL(SYNOQorIQGPIOWakeInterruptClear);
#endif
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
EXPORT_SYMBOL(SYNOQorIQHWReset);
#endif
EXPORT_SYMBOL(SYNO_CHECK_HDD_PRESENT);

static void 
QORIQ_813_GPIO_init(SYNO_QORIQ_GENERIC_GPIO *global_gpio)
{
	SYNO_QORIQ_GENERIC_GPIO gpio_813 = {
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
							.disable_disk_led = 60,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
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
							.fan_fail = 28,
							.fan_fail_2 = 29,
							.fan_fail_3 = 26,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
							.hdd1_present = GPIO_UNDEF,
							.hdd2_present = GPIO_UNDEF,
							.hdd3_present = GPIO_UNDEF,
							.hdd4_present = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = 27,
							.buzzer_mute_ack = GPIO_UNDEF,
							.rps1_on = 71,
							.rps2_on = 72,
						},
		.multi_bay	  = {
							.inter_lock = GPIO_UNDEF,
						},
		.status		  = {
							.power_led = GPIO_UNDEF,
							.alarm_led = GPIO_UNDEF,
						},
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
		.hw_reset	  =  {
							.deep_wake = 62,  
						 },
#endif
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
		.wakeup		  = {
							.power_btn = 61,  
						},
#endif
	};

	*global_gpio = gpio_813;
}

static void 
QORIQ_413_GPIO_init(SYNO_QORIQ_GENERIC_GPIO *global_gpio)
{
	SYNO_QORIQ_GENERIC_GPIO gpio_413 = {
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
							.disable_disk_led = 60,
						},
		.soc_sata_led = {
							.hdd2_fail_led = 22,
							.hdd1_fail_led = 23,
						},
		.model		  = {
							.model_id_0 = 28,
							.model_id_1 = 29,
							.model_id_2 = 46,
							.model_id_3 = 47,
						},
		.fan		  = {
							.fan_1 = GPIO_UNDEF,
							.fan_2 = GPIO_UNDEF,
							.fan_3 = GPIO_UNDEF,
							.fan_fail = 28,
							.fan_fail_2 = 29,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = 24,
							.hdd2_pm = 25,
							.hdd3_pm = 26,
							.hdd4_pm = 27,
							.hdd1_present = 70,
							.hdd2_present = 71,
							.hdd3_present = 72,
							.hdd4_present = 84,
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
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
		.hw_reset	  =  {
							.deep_wake = 62,  
						 },
#endif
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
		.wakeup		  = {
							.power_btn = 61,  
						},
#endif
	};

	*global_gpio = gpio_413;
}

static void
QORIQ_213p_GPIO_init(SYNO_QORIQ_GENERIC_GPIO *global_gpio)
{
	SYNO_QORIQ_GENERIC_GPIO gpio_213p = {
		.ext_sata_led = {
							.hdd1_led_0 = 75,  
							.hdd1_led_1 = 70,  
							.hdd2_led_0 = 26,  
							.hdd2_led_1 = 72,  
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
							.disable_disk_led = 60,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
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
							.fan_fail = 28,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = 24,
							.hdd2_pm = 25,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
							.hdd1_present = 71,
							.hdd2_present = 29,
							.hdd3_present = GPIO_UNDEF,
							.hdd4_present = GPIO_UNDEF,
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
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
		.hw_reset	  =  {
							.deep_wake = 62,  
						 },
#endif
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
		.wakeup		  = {
							.power_btn = 61,  
						},
#endif
	};

	*global_gpio = gpio_213p;
}

static void
QORIQ_rs213p_GPIO_init(SYNO_QORIQ_GENERIC_GPIO *global_gpio)
{
	SYNO_QORIQ_GENERIC_GPIO gpio_rs213p = {
		.ext_sata_led = {
							.hdd1_led_0 = 75,  
							.hdd1_led_1 = 70,  
							.hdd2_led_0 = 71,  
							.hdd2_led_1 = 72,  
							.hdd3_led_0 = GPIO_UNDEF,
							.hdd3_led_1 = GPIO_UNDEF,
							.hdd4_led_0 = GPIO_UNDEF,
							.hdd4_led_1 = GPIO_UNDEF,
							.hdd5_led_0 = GPIO_UNDEF,
							.hdd5_led_1 = GPIO_UNDEF,
							.disable_disk_led = 60,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
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
							.fan_fail = 28,
							.fan_fail_2 = 29,
							.fan_fail_3 = 26,
						},
		.hdd_pm		  = {
							.hdd1_pm = GPIO_UNDEF,
							.hdd2_pm = GPIO_UNDEF,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
							.hdd1_present = GPIO_UNDEF,
							.hdd2_present = GPIO_UNDEF,
							.hdd3_present = GPIO_UNDEF,
							.hdd4_present = GPIO_UNDEF,
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
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
		.hw_reset	  =  {
							.deep_wake = 62,  
						 },
#endif
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
		.wakeup		  = {
							.power_btn = 61,  
						},
#endif
	};

	*global_gpio = gpio_rs213p;
}
int __init synology_gpio_init(void)
{
	if (syno_is_hw_version(HW_DS413)) {
		printk("Apply DS 413 GPIO\n");
		QORIQ_413_GPIO_init(&generic_gpio);
	}

    if (syno_is_hw_version(HW_DS213pv10)) {
        printk("Apply DS 213+ GPIO\n");
        QORIQ_213p_GPIO_init(&generic_gpio);
    }

	if (syno_is_hw_version(HW_RS813)) {
		printk("Apply RS 813 GPIO\n");
		QORIQ_813_GPIO_init(&generic_gpio);
	}

	if (syno_is_hw_version(HW_RS213p)) {
		printk("Apply RS 213+ GPIO\n");
		QORIQ_rs213p_GPIO_init(&generic_gpio);
	}

	return 0;
}
arch_initcall(synology_gpio_init);
