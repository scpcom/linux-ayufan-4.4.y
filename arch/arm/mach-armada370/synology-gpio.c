#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Synology Armada NAS Board GPIO Setup
 *
 * Maintained by:  KueiHuan Chen <khchen@synology.com>
 *
 * Copyright 2009-2012 Synology, Inc.  All rights reserved.
 * Copyright 2009-2012 KueiHuan.Chen
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
#if defined(CONFIG_SYNO_ARMADA_ARCH)

#include <linux/gpio.h>
#include <linux/synobios.h>
#include <linux/export.h>

#include "boardEnv/mvBoardEnvSpec.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "config/mvSysSataConfig.h"
#include "mvOs.h"

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

typedef struct __tag_SYNO_ARMADA_HDD_PM_GPIO {
	u8 hdd1_pm;
	u8 hdd2_pm;
	u8 hdd3_pm;
	u8 hdd4_pm;
} SYNO_ARMADA_HDD_PM_GPIO;

typedef struct __tag_SYNO_ARMADA_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_fail;
	u8 fan_fail_2;
	u8 fan_fail_3;
} SYNO_ARMADA_FAN_GPIO;

typedef struct __tag_SYNO_ARMADA_MODEL_GPIO {
	u8 model_id_0;
	u8 model_id_1;
	u8 model_id_2;
	u8 model_id_3;
} SYNO_ARMADA_MODEL_GPIO;

typedef struct __tag_SYNO_ARMADA_EXT_HDD_LED_GPIO {
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
} SYNO_ARMADA_EXT_HDD_LED_GPIO;

typedef struct __tag_SYNO_ARMADA_MULTI_BAY_GPIO {
	u8 inter_lock;
}SYNO_ARMADA_MULTI_BAY_GPIO;

typedef struct __tag_SYNO_ARMADA_SOC_HDD_LED_GPIO {
	u8 hdd2_fail_led;
	u8 hdd1_fail_led;
}SYNO_ARMADA_SOC_HDD_LED_GPIO;

typedef struct __tag_SYNO_ARMADA_RACK_GPIO {
	u8 buzzer_mute_req;
	u8 buzzer_mute_ack;
	u8 rps1_on;
	u8 rps2_on;
}SYNO_ARMADA_RACK_GPIO;

typedef struct __tag_SYNO_ARMADA_STATUS_LED_GPIO {
	u8 alarm_led;
	u8 power_led;
} SYNO_ARMADA_STATUS_LED_GPIO;

typedef struct __tag_SYNO_ARMADA_USB_GPIO {
	u8 usb_power;
} SYNO_ARMADA_USB_GPIO;

typedef struct __tag_SYNO_ARMADA_GENERIC_GPIO {
	SYNO_ARMADA_EXT_HDD_LED_GPIO	ext_sata_led;
	SYNO_ARMADA_SOC_HDD_LED_GPIO	soc_sata_led;
	SYNO_ARMADA_MODEL_GPIO			model;
	SYNO_ARMADA_FAN_GPIO			fan;
	SYNO_ARMADA_HDD_PM_GPIO			hdd_pm;
	SYNO_ARMADA_RACK_GPIO			rack;
	SYNO_ARMADA_MULTI_BAY_GPIO		multi_bay;
	SYNO_ARMADA_STATUS_LED_GPIO		status;
	SYNO_ARMADA_USB_GPIO			usb;
}SYNO_ARMADA_GENERIC_GPIO;

static SYNO_ARMADA_GENERIC_GPIO generic_gpio;

unsigned int Syno6282ModelIDGet(SYNO_ARMADA_GENERIC_GPIO *pGpio)
{
	return  (((gpio_get_value(pGpio->model.model_id_0) ? 1 : 0) << 3) | 
			 ((gpio_get_value(pGpio->model.model_id_1) ? 1 : 0) << 2) | 
			 ((gpio_get_value(pGpio->model.model_id_2) ? 1 : 0) << 1) | 
			 ((gpio_get_value(pGpio->model.model_id_3) ? 1 : 0) << 0));
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

	/* there are 66 mpp pins only */
	if(66 < mppPin)
		return -EINVAL;

	/* get the group the pin belongs to, for addressing */
	/* 32 bits per register, 4 bits per pin, 8 pins in a group */
	mppGroup = mppPin / 8;
	mppVal &= 0x0F;
	origVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));

	/* get the corresponding bits */
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

int
SYNO_SOC_HDD_LED_SET(int index, int status)
{
	int ret = -1;
	int mpp_pin;
	int mode_sata_present;
	int mode_gpio;
	int fail_led;
	int active = 0; //note: led is low active

#ifdef SYNO_HW_VERSION
	if (syno_is_hw_version(HW_RS214v10)) {
		// RS214 led is high active
		active = 1;
	}
#endif

	if (1 == index)
		WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd1_fail_led);
	else if (2 == index)
		WARN_ON(GPIO_UNDEF == generic_gpio.soc_sata_led.hdd2_fail_led);

        if (SYNO_RS214_ID == mvBoardIdGet()) {
                /* RS214 disk LED is HIGH active, so we need invert the active indication */
                MV_REG_WRITE(MV_SATA_REGS_OFFSET + 0x2c, 0xc);
        } else {
                MV_REG_WRITE(MV_SATA_REGS_OFFSET + 0x2c, 0x4);
        }

	/* assign pin info according to hdd */
	switch (index) {
		case 1:
			mpp_pin = 60;
			mode_sata_present = 0x03;
			mode_gpio = 0;
			fail_led = generic_gpio.soc_sata_led.hdd1_fail_led;
			break;
		case 2:
			mpp_pin = 48;
			mode_sata_present = 0x04;
			mode_gpio = 0;
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
		SYNOMppCtrlRegWrite(mpp_pin, mode_gpio);  // change MPP to GPIO mode
		gpio_set_value(mpp_pin, !active);
		gpio_set_value(fail_led, active);
	}
	else if ( DISK_LED_GREEN_SOLID == status ||
			  DISK_LED_GREEN_BLINK == status )
	{
		SYNOMppCtrlRegWrite(mpp_pin, mode_sata_present);  // change MPP to sata present mode
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

#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))
struct disk_info {
	char *hw_version;
	int	max_disk_id;
};
static struct disk_info ds213j_family[] = {
	{HW_DS213jv10, 2},
	{HW_DS114v10, 1},
	{HW_DS214v10, 2}
};

MV_U8 SYNOArmadaIsBoardNeedPowerUpHDD(MV_U32 disk_id) {
	u8 ret = 0;
	MV_U32 boardId = mvBoardIdGet();
	struct disk_info *table = NULL;
	int table_cnt = 0;
	int def_max_disk = 0;

	switch(boardId) {
	case SYNO_DS213j_ID:
		table = ds213j_family;
		table_cnt = ARRAY_LEN(ds213j_family);
		def_max_disk = 2;
		break;
	case SYNO_US3_ID:
		def_max_disk = 0;
		break;
	case SYNO_RS214_ID:
		def_max_disk = 2;
		break;
	case SYNO_DS214se_ID:
		def_max_disk = 2;
		break;
	case SYNO_DS414slim_ID:
		def_max_disk = 0;
		break;
	case SYNO_DS115j_ID:
		def_max_disk = 1;
		break;

	default:
		break;
	}

	/* lookup table for max disk
	   if not found, compare default */
	ret = (disk_id <= def_max_disk)? 1 : 0;
	if (table) {
		int i;
		for (i = 0; i < table_cnt; i++) {
			if (syno_is_hw_version(table[i].hw_version)) {
				if (disk_id <= table[i].max_disk_id) {
					ret = 1;
				}
				break;
			}
		}
	}

	return ret;
}

/* SYNO_CHECK_HDD_PRESENT
 * Check HDD present for evansport
 * input : index - disk index, 1-based.
 * output: 0 - HDD not present,  1 - HDD present.
 */
int SYNO_CHECK_HDD_PRESENT(int index)
{
	return 1;
}

void SYNO_ENABLE_USB_POWER(int blEnable)
{
	if (GPIO_UNDEF != generic_gpio.usb.usb_power)
		gpio_set_value(generic_gpio.usb.usb_power, blEnable ? 0 : 1);
}

EXPORT_SYMBOL(SYNOArmadaIsBoardNeedPowerUpHDD);
EXPORT_SYMBOL(SYNO_ARMADA_GPIO_PIN);
EXPORT_SYMBOL(SYNO_ARMADA_GPIO_BLINK);
EXPORT_SYMBOL(SYNO_ENABLE_HDD_LED);
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
EXPORT_SYMBOL(SYNO_CHECK_HDD_PRESENT);
EXPORT_SYMBOL(SYNO_ENABLE_USB_POWER);

/*
 Pin 		Mode	Signal select and definition	Input/output	Pull-up/pull-down
 MPP[0:1]	0x1	UART0
 MPP[2:3]	0x1	TWSI
 MPP[4]		0x1	CPU Power control		Out
 MPP[5:16]	0x0	Reserved
 MPP[17:18]	0x1	Ethernet SMI
 MPP[19:30]	0x0	Reserved
 MPP[31]		0x0	HDD 0 fail LED			Out
 MPP[32]		0x0	HDD 1 fail LED			Out
 MPP[33:36]	0x2	SPI
 MPP[37]		0x0	HDD 0 Power			Out
 MPP[38]		0x0	Fan Sense			In
 MPP[39:40]	0x0	Reserved
 MPP[41:42]	0x1	UART1
 MPP[43:47]	0x0	Reserved
 MPP[48]		0x4	HDD 1 Present			Out
 MPP[49:54]	0x0	Reserved
 MPP[55:58]	0x0	Model ID			In
 MPP[59]		0x0	Reserved
 MPP[60]		0x3	HDD 0 Present			Out
 MPP[61]		0x0	Reserved
 MPP[62]		0x0	HDD 1 Power			Out
 MPP[63]		0x0	FAN Control Max			Out
 MPP[64]		0x0	FAN Control Middle		Out
 MPP[65]		0x0	FAN Control Min			Out
*/
static void 
Armada_370_213j_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_213j = {
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
							.hdd2_fail_led = 32,
							.hdd1_fail_led = 31,
						},
		.model		  = {
							.model_id_0 = 55,
							.model_id_1 = 56,
							.model_id_2 = 57,
							.model_id_3 = 58,
						},
		.fan		  = {
							.fan_1 = 63,
							.fan_2 = 64,
							.fan_3 = 65,
							.fan_fail = 38,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = 37,
							.hdd2_pm = 62,
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
		.usb		  = {
							.usb_power = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_213j;
}

extern void (*syno_power_off_indicator)(void);
static void us3_power_off(void)
{
	/* since US3 has no microP to power off,
	 * we need an indicator for system halt */
	printk("Set US3 shutdown indicator\n");
	/* set power green off */
	gpio_set_value(42, 1);
	SYNO_ARMADA_GPIO_BLINK(42, 0);
	/* set power orange on */
	gpio_set_value(43, 0);
	SYNO_ARMADA_GPIO_BLINK(43, 0);
}

static void
Armada_370_us3_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_us3 = {
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
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = 55,
							.model_id_1 = 56,
							.model_id_2 = 57,
							.model_id_3 = 58,
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
		.usb		  = {
							.usb_power = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_us3;

	/* customize power off indicator */
	syno_power_off_indicator = us3_power_off;
}

static void
Armada_370_rs214_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_rs214 = {
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
							.hdd2_fail_led = 32,
							.hdd1_fail_led = 49,
						},
		.model		  = {
							.model_id_0 = 55,
							.model_id_1 = 56,
							.model_id_2 = 57,
							.model_id_3 = 58,
						},
		.fan		  = {
							.fan_1 = 65,
							.fan_2 = 64,
							.fan_3 = 63,
							.fan_fail = 38,
							.fan_fail_2 = 50,
							.fan_fail_3 = 51,
						},
		.hdd_pm		  = {
							.hdd1_pm = 60,
							.hdd2_pm = 48,
							.hdd3_pm = GPIO_UNDEF,
							.hdd4_pm = GPIO_UNDEF,
						},
		.rack		  = {
							.buzzer_mute_req = 52,
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
		.usb		  = {
							.usb_power = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_rs214;
}

static void 
Armada_370_214se_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_214se = {
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
							.hdd2_fail_led = 32,
							.hdd1_fail_led = 31,
						},
		.model		  = {
							.model_id_0 = 55,
							.model_id_1 = 56,
							.model_id_2 = 57,
							.model_id_3 = 58,
						},
		.fan		  = {
							.fan_1 = 63,
							.fan_2 = 64,
							.fan_3 = 65,
							.fan_fail = 38,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = 37,
							.hdd2_pm = 62,
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
		.usb		  = {
							.usb_power = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_214se;
}

static void 
Armada_370_414slim_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_414slim = {
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
							.hdd_led_mask = 39,
						},
		.soc_sata_led = {
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = GPIO_UNDEF,
						},
		.model		  = {
							.model_id_0 = 55,
							.model_id_1 = 56,
							.model_id_2 = 57,
							.model_id_3 = 58,
						},
		.fan		  = {
							.fan_1 = 65,
							.fan_2 = 64,
							.fan_3 = 63,
							.fan_fail = 38,
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
		.usb		  = {
							.usb_power = 44,
						},
	};

	*global_gpio = gpio_414slim;
}

static void 
Armada_370_115j_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_115j = {
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
							.hdd2_fail_led = GPIO_UNDEF,
							.hdd1_fail_led = 31,
						},
		.model		  = {
							.model_id_0 = 55,
							.model_id_1 = 56,
							.model_id_2 = 57,
							.model_id_3 = 58,
						},
		.fan		  = {
							.fan_1 = 63,
							.fan_2 = 64,
							.fan_3 = 65,
							.fan_fail = 38,
							.fan_fail_2 = GPIO_UNDEF,
							.fan_fail_3 = GPIO_UNDEF,
						},
		.hdd_pm		  = {
							.hdd1_pm = 37,
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
		.usb		  = {
							.usb_power = 44,
						},
	};

	*global_gpio = gpio_115j;
}

static void
ARMADA_default_GPIO_init(SYNO_ARMADA_GENERIC_GPIO *global_gpio)
{
	SYNO_ARMADA_GENERIC_GPIO gpio_default = {
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
						},
		.usb		  = {
							.usb_power = GPIO_UNDEF,
						},
	};

	*global_gpio = gpio_default;
}
void synology_gpio_init(void)
{
	MV_U32 boardId = mvBoardIdGet();

	switch(boardId) {
	case SYNO_DS213j_ID:
		Armada_370_213j_GPIO_init(&generic_gpio);
		printk("Synology Armada370 1, 2 bay GPIO Init\n");
		break;
	case SYNO_US3_ID:
		Armada_370_us3_GPIO_init(&generic_gpio);
		printk("Synology Armada370 US GPIO Init\n");
		break;
	case SYNO_RS214_ID:
		Armada_370_rs214_GPIO_init(&generic_gpio);
		printk("Synology Armada370 RS214 GPIO Init\n");
		break;
	case SYNO_DS214se_ID:
		Armada_370_214se_GPIO_init(&generic_gpio);
		printk("Synology Armada370 DS214se GPIO Init\n");
		break;
	case SYNO_DS414slim_ID:
		Armada_370_414slim_GPIO_init(&generic_gpio);
		printk("Synology Armada370 DS414slim GPIO Init\n");
		break;
	case SYNO_DS115j_ID:
		Armada_370_115j_GPIO_init(&generic_gpio);
		printk("Synology Armada370 DS115j GPIO Init\n");
		break;

	default:
		printk("%s BoardID not match\n", __FUNCTION__);
		ARMADA_default_GPIO_init(&generic_gpio);
		break;
	}
}
#endif /* CONFIG_SYNO_ARMADA */
