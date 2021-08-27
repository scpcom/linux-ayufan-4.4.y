#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#if defined(CONFIG_SYNO_ARMADA_ARCH)

#include <linux/gpio.h>
#include <linux/synobios.h>
#include <linux/export.h>

#include "boardEnv/mvBoardEnvSpec.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "config/mvSysSataConfig.h"
#include "mvOs.h"

#define GPIO_UNDEF				0xFF

#define DISK_LED_OFF			0
#define DISK_LED_GREEN_SOLID	1
#define DISK_LED_ORANGE_SOLID	2
#define DISK_LED_ORANGE_BLINK	3
#define DISK_LED_GREEN_BLINK    4

#define SYNO_LED_OFF		0
#define SYNO_LED_ON		1
#define SYNO_LED_BLINKING	2

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
	u8 phy_led;
} SYNO_STATUS_LED_GPIO;

typedef struct __tag_SYNO_GPIO {
	SYNO_EXT_HDD_LED_GPIO	ext_sata_led;
	SYNO_SOC_HDD_LED_GPIO	soc_sata_led;
	SYNO_MODEL_GPIO			model;
	SYNO_FAN_GPIO			fan;
	SYNO_HDD_PM_GPIO			hdd_pm;
	SYNO_RACK_GPIO			rack;
	SYNO_MULTI_BAY_GPIO		multi_bay;
	SYNO_STATUS_LED_GPIO		status;
	SYNO_HDD_DETECT_GPIO	hdd_detect;
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
SYNO_ARMADA_GPIO_PIN(int pin, int *pValue, int isWrite)
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

extern MV_STATUS mvGppBlinkEn(MV_U32 group, MV_U32 mask, MV_U32 value);

int
SYNO_ARMADA_GPIO_BLINK(int pin, int blink)
{
	u32 grp = pin >> 5;
	u32 mask = (1 << (pin & 0x1F));

	if (blink)
		mvGppBlinkEn(grp, mask, mask);
	else
		mvGppBlinkEn(grp, mask, 0);
	return 0;
}

MV_STATUS SYNOMppCtrlRegWrite(MV_U32 mppPin, MV_U32 mppVal)
{
	MV_U32 origVal;
	MV_U32 mppGroup;

	if(66 < mppPin)
		return -EINVAL;

	mppGroup = mppPin / 8;
	mppVal &= 0x0F;
	origVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));

	origVal &= ~(0xF << ((mppPin % 8)*4));
	origVal |= (mppVal << ((mppPin % 8)*4));

	MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), origVal);

	return MV_OK;
}

void SYNO_ENABLE_HDD_LED(int blEnable)
{
	if (GPIO_UNDEF != generic_gpio.ext_sata_led.hdd_led_mask)
		gpio_set_value(generic_gpio.ext_sata_led.hdd_led_mask, blEnable ? 0 : 1);
}

void SYNO_ENABLE_PHY_LED(int blEnable)
{
	if (GPIO_UNDEF != generic_gpio.status.phy_led)
		gpio_set_value(generic_gpio.status.phy_led, blEnable ? 1 : 0);
}

int
SYNO_SOC_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int mpp_pin;
	int mode_sata_present;
	int mode_gpio;
	int fail_led;
	int active = 1;  

	WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_fail_led);

	switch (index) {
		case 1:
			mpp_pin = 25;
			mode_sata_present = 0x01;
			mode_gpio = 0;
			fail_led = generic_gpio.soc_sata_led.hdd1_fail_led;
			break;
		case 2:
			 
			mpp_pin = 24;
			mode_sata_present = 0x01;
			mode_gpio = 0;
			fail_led = generic_gpio.soc_sata_led.hdd2_fail_led;
			break;
		default:
			printk("Wrong HDD number [%d]\n", index);
			goto END;
	}

	if ( DISK_LED_ORANGE_SOLID == status ||
		 DISK_LED_ORANGE_BLINK == status )
	{
		SYNOMppCtrlRegWrite(mpp_pin, mode_gpio);   
		gpio_set_value(mpp_pin, !active);
		gpio_set_value(fail_led, active);
	}
	else if ( DISK_LED_GREEN_SOLID == status )
	{
		SYNOMppCtrlRegWrite(mpp_pin, mode_sata_present);   
		gpio_set_value(fail_led, !active);
	}
	else if (DISK_LED_OFF == status)
	{
		SYNOMppCtrlRegWrite(mpp_pin, mode_gpio);
		gpio_set_value(mpp_pin, !active);
		gpio_set_value(fail_led, !active);
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

	gpio_set_value(pin1, bit1);
	gpio_set_value(pin2, bit2);

    ret = 0;
END:
    return ret;
}

int
SYNO_CTRL_USB_HDD_LED_SET(int status)
{
	int pin1 = GPIO_UNDEF, pin2 = GPIO_UNDEF, 
		bit1 = 0, bit2 = 0, 
		blink1 = 0, blink2 = 0;

	pin1 = generic_gpio.ext_sata_led.hdd1_led_0;
	pin2 = generic_gpio.ext_sata_led.hdd1_led_1;

	WARN_ON(pin1 == GPIO_UNDEF);
	WARN_ON(pin2 == GPIO_UNDEF);

	switch (status) {
	case DISK_LED_OFF:
		bit1 = 0;
		bit2 = 0;
		blink1 = 0;
		blink2 = 0;
		break;
	case DISK_LED_GREEN_SOLID:
		bit1 = 0;
		bit2 = 1;
		blink1 = 0;
		blink2 = 0;
		break;
	case DISK_LED_ORANGE_SOLID:
		bit1 = 1;
		bit2 = 0;
		blink1 = 0;
		blink2 = 0;
		break;
	case DISK_LED_ORANGE_BLINK:
		bit1 = 1;
		bit2 = 0;
		blink1 = 1;
		blink2 = 0;
		break;
	case DISK_LED_GREEN_BLINK:
		bit1 = 0;
		bit2 = 1;
		blink1 = 0;
		blink2 = 1;
		break;
	default:
		printk("Wrong disk led set.\n");
		break;
	}

	gpio_set_value(pin1, bit1);
	gpio_set_value(pin2, bit2);
	SYNO_ARMADA_GPIO_BLINK(pin1, blink1);
	SYNO_ARMADA_GPIO_BLINK(pin2, blink2);

	return 0;
}

int SYNO_CTRL_POWER_LED_SET(int status)
{
	int pin = GPIO_UNDEF, bit = 0, blink = 0;

	pin = generic_gpio.status.power_led;

	WARN_ON(pin == GPIO_UNDEF);

	switch (status) {
	case SYNO_LED_OFF:
		blink = 0;
		bit = 1;
		break;
	case SYNO_LED_ON:
		blink = 0;
		bit = 0;
		break;
	case SYNO_LED_BLINKING:
		blink = 1;
		bit = 0;
		break;
	}

	gpio_set_value(pin, bit);
	SYNO_ARMADA_GPIO_BLINK(pin, blink);

	return 0;
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

int SYNO_CHECK_HDD_PRESENT(int index)
{
	int iPrzVal = 1;  

	switch (index) {
		case 1:
			WARN_ON(GPIO_UNDEF == generic_gpio.hdd_detect.hdd1_present_detect);
			iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd1_present_detect);
			break;
		case 2:
			WARN_ON(GPIO_UNDEF == generic_gpio.hdd_detect.hdd2_present_detect);
			iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd2_present_detect);
			break;
		case 3:
			WARN_ON(GPIO_UNDEF == generic_gpio.hdd_detect.hdd3_present_detect);
			iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd3_present_detect);
			break;
		case 4:
			WARN_ON(GPIO_UNDEF == generic_gpio.hdd_detect.hdd4_present_detect);
			iPrzVal = !gpio_get_value(generic_gpio.hdd_detect.hdd4_present_detect);
			break;
		default:
			break;
	}

	return iPrzVal;

}

int SYNO_SUPPORT_HDD_DYNAMIC_ENABLE_POWER(void)
{
	int iRet = 0;

	if ((GPIO_UNDEF != generic_gpio.hdd_pm.hdd1_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd1_present_detect) ||
			(GPIO_UNDEF != generic_gpio.hdd_pm.hdd2_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd2_present_detect) ||
			(GPIO_UNDEF != generic_gpio.hdd_pm.hdd3_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd3_present_detect) ||
			(GPIO_UNDEF != generic_gpio.hdd_pm.hdd4_pm && GPIO_UNDEF != generic_gpio.hdd_detect.hdd4_present_detect)) {

		iRet = 1;
	}
	return iRet;
}

struct disk_cnt {
	char *hw_version;
	int max_disk_id;
};
static inline int MAX_DISK(struct disk_cnt *tbl)
{
	int i=0;
	while (tbl[i].hw_version) {
		if (syno_is_hw_version(tbl[i].hw_version))
			return tbl[i].max_disk_id;
		i++;
	}
	return 255;
}
static struct disk_cnt table_4bay[] = {
	{"DS414v10", 4},
	{"DS214+", 2},
	{NULL, 0}
};
MV_U8 SYNOArmadaIsBoardNeedPowerUpHDD(MV_U32 disk_id)
{
	int max_disk = 0;
	MV_U32 boardId = mvBoardIdGet();

	switch(boardId) {
	case SYNO_AXP_4BAY_2BAY:
		max_disk = MAX_DISK(table_4bay);
		break;
	case SYNO_AXP_2BAY:
		max_disk = 2;
		break;
	default:
		break;
	}

	return (disk_id <= max_disk)? 1 : 0;
}

EXPORT_SYMBOL(SYNOArmadaIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_ARMADA_GPIO_PIN);
EXPORT_SYMBOL(SYNO_ARMADA_GPIO_BLINK);
EXPORT_SYMBOL(SYNO_ENABLE_HDD_LED);
EXPORT_SYMBOL(SYNO_ENABLE_PHY_LED);
EXPORT_SYMBOL(SYNO_SOC_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_EXT_CHIP_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_USB_HDD_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_POWER_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_HDD_POWERON);
EXPORT_SYMBOL(SYNO_CTRL_FAN_PERSISTER);
EXPORT_SYMBOL(SYNO_CTRL_FAN_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_ALARM_LED_SET);
EXPORT_SYMBOL(SYNO_CTRL_BACKPLANE_STATUS_GET);
EXPORT_SYMBOL(SYNO_CTRL_BUZZER_CLEARED_GET);
EXPORT_SYMBOL(SYNO_SUPPORT_HDD_DYNAMIC_ENABLE_POWER);

static SYNO_GPIO gpio_axp_4bay = {
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
		.hdd_led_mask = 25,
	},
	.soc_sata_led = {
		.hdd2_fail_led = GPIO_UNDEF,
		.hdd1_fail_led = GPIO_UNDEF,
	},
	.model		  = {
		.model_id_0 = 26,
		.model_id_1 = 28,
		.model_id_2 = 29,
		.model_id_3 = GPIO_UNDEF,
	},
	.fan		  = {
		.fan_1 = GPIO_UNDEF,
		.fan_2 = GPIO_UNDEF,
		.fan_3 = GPIO_UNDEF,
		.fan_fail = 33,
		.fan_fail_2 = 32,
		.fan_fail_3 = GPIO_UNDEF,
	},
	.hdd_pm		  = {
		.hdd1_pm = 42,
		.hdd2_pm = 44,
		.hdd3_pm = 45,
		.hdd4_pm = 46,
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
		.phy_led = 30,
	},
	.hdd_detect	= {
		.hdd1_present_detect = 34,
		.hdd2_present_detect = 35,
		.hdd3_present_detect = 40,
		.hdd4_present_detect = 41,
	},
};

static SYNO_GPIO gpio_axp_2bay = {
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
		.hdd_led_mask = GPIO_UNDEF,
	},
	.soc_sata_led = {
		.hdd2_fail_led = 45,
		.hdd1_fail_led = 46,
	},
	.model		  = {
		.model_id_0 = 26,
		.model_id_1 = 28,
		.model_id_2 = 29,
		.model_id_3 = GPIO_UNDEF,
	},
	.fan		  = {
		.fan_1 = GPIO_UNDEF,
		.fan_2 = GPIO_UNDEF,
		.fan_3 = GPIO_UNDEF,
		.fan_fail = 33,
		.fan_fail_2 = GPIO_UNDEF,
		.fan_fail_3 = GPIO_UNDEF,
	},
	.hdd_pm		  = {
		.hdd1_pm = 42,
		.hdd2_pm = 44,
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
		.phy_led = GPIO_UNDEF,
	},
	.hdd_detect	= {
		.hdd1_present_detect = 34,
		.hdd2_present_detect = 35,
		.hdd3_present_detect = GPIO_UNDEF,
		.hdd4_present_detect = GPIO_UNDEF,
	},
};

static SYNO_GPIO gpio_axp_4bay_rack = {
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
		.hdd_led_mask = 47,
	},
	.soc_sata_led = {
		.hdd2_fail_led = GPIO_UNDEF,
		.hdd1_fail_led = 24,
	},
	.model		  = {
		.model_id_0 = 26,
		.model_id_1 = 28,
		.model_id_2 = 29,
		.model_id_3 = GPIO_UNDEF,
	},
	.fan		  = {
		.fan_1 = GPIO_UNDEF,
		.fan_2 = GPIO_UNDEF,
		.fan_3 = GPIO_UNDEF,
		.fan_fail = 33,
		.fan_fail_2 = 34,
		.fan_fail_3 = 35,
	},
	.hdd_pm		  = {
		.hdd1_pm = GPIO_UNDEF,
		.hdd2_pm = GPIO_UNDEF,
		.hdd3_pm = GPIO_UNDEF,
		.hdd4_pm = GPIO_UNDEF,
	},
	.rack		  = {
		.buzzer_mute_req = 44,
		.buzzer_mute_ack = 40,
		.rps1_on = GPIO_UNDEF,
		.rps2_on = GPIO_UNDEF,
	},
	.multi_bay	  = {
		.inter_lock = GPIO_UNDEF,
	},
	.status		  = {
		.power_led = GPIO_UNDEF,
		.alarm_led = GPIO_UNDEF,
		.phy_led = GPIO_UNDEF,
	},
	.hdd_detect	= {
		.hdd1_present_detect = GPIO_UNDEF,
		.hdd2_present_detect = GPIO_UNDEF,
		.hdd3_present_detect = GPIO_UNDEF,
		.hdd4_present_detect = GPIO_UNDEF,
	},
};

static void
AXP_4BAY_GPIO_init(SYNO_GPIO *global_gpio)
{
	*global_gpio = gpio_axp_4bay;
}

static void
AXP_2BAY_GPIO_init(SYNO_GPIO *global_gpio)
{
	*global_gpio = gpio_axp_2bay;
}

static void
AXP_4BAY_RACK_GPIO_init(SYNO_GPIO *global_gpio)
{
	*global_gpio = gpio_axp_4bay_rack;
}

static void
ARMADA_default_GPIO_init(SYNO_GPIO *global_gpio)
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
			.phy_led = GPIO_UNDEF
		},
		.hdd_detect	= {
			.hdd1_present_detect = GPIO_UNDEF,
			.hdd2_present_detect = GPIO_UNDEF,
			.hdd3_present_detect = GPIO_UNDEF,
			.hdd4_present_detect = GPIO_UNDEF,
		},
	};

	*global_gpio = gpio_default;
}

void synology_gpio_init(void)
{
	MV_U32 boardId = mvBoardIdGet();

	switch(boardId) {
	case SYNO_AXP_4BAY_2BAY:
		AXP_4BAY_GPIO_init(&generic_gpio);
		printk("Synology ArmadaXP 4-bay GPIO Init\n");
		break;
	case SYNO_AXP_2BAY:
		AXP_2BAY_GPIO_init(&generic_gpio);
		printk("Synology ArmadaXP 2-bay GPIO Init\n");
		break;
	case SYNO_AXP_4BAY_RACK:
		AXP_4BAY_RACK_GPIO_init(&generic_gpio);
		printk("Synology ArmadaXP 4-bay rack GPIO Init\n");
		break;
	default:
		printk("%s BoardID not match\n", __FUNCTION__);
		ARMADA_default_GPIO_init(&generic_gpio);
		break;
	}
}
#endif  
