// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include <linux/fs.h>
#include "mv6281_common.h"
#include "../mapping.h"
#include "../i2c/i2c-mv.h"
#include "../rtc/rtc.h"

#ifdef MY_ABC_HERE
extern char gszSerialNum[];
#endif

static int Uninitialize(void);

static int
FanStatusMappingRS409r1(FAN_STATUS status, FAN_SPEED speed, char *pSpeed_value)
{
	int ret = -1;

	if (status == FAN_STATUS_STOP) {
		*pSpeed_value = CPLD_FAN_SPEED_0;
	} else {
		switch (speed) {
		case FAN_SPEED_STOP:
			*pSpeed_value = CPLD_FAN_SPEED_0;
			break;
		case FAN_SPEED_ULTRA_LOW:
		case FAN_SPEED_VERY_LOW:
		case FAN_SPEED_LOW:
		case FAN_SPEED_TEST_1:
			*pSpeed_value = CPLD_FAN_SPEED_2;
			break;
		case FAN_SPEED_MIDDLE:
		case FAN_SPEED_TEST_2:
			*pSpeed_value = CPLD_FAN_SPEED_4;
			break;
		case FAN_SPEED_HIGH:
		case FAN_SPEED_VERY_HIGH:
		case FAN_SPEED_ULTRA_HIGH:
		case FAN_SPEED_FULL:
		case FAN_SPEED_TEST_4:
			*pSpeed_value = CPLD_FAN_SPEED_6;
			break;
		default:
			printk("%s(%d) No such fan speed exists, speed=[%d].\n",
				__FILE__, __LINE__, speed);
			goto END;
		}
	}

	ret = 0;
END:
	return ret;
}

int GetBrand(void)
{
	int Brand = -1;

	//YMXX[Z]SSSSS
#ifdef MY_ABC_HERE
	if ( gszSerialNum[4] == 'M' ) {
		Brand = BRAND_LOGITEC;
	} else if ( gszSerialNum[4] == 'U' ) {
		Brand = BRAND_SYNOLOGY_USA;
	} else {
		Brand = BRAND_SYNOLOGY;
	}
#endif

	return Brand;
}

static int SetGpioPin( GPIO_PIN *pPin )
{
	int ret = -1;

	if ( NULL == pPin ) {
		goto End;
	}

	if ( 0 != SYNO_MV6281_GPIO_PIN((int)pPin->pin, (int*)&pPin->value, 1) ) {
		goto End;
	}

	ret = 0;
End:
	return ret;
}

static int GetGpioPin( GPIO_PIN *pPin )
{
	int ret = -1;

	if ( NULL == pPin ) {
		goto End;
	}

	if ( 0 != SYNO_MV6281_GPIO_PIN((int)pPin->pin, (int*)&pPin->value, 0) ) {
		goto End;
	}

	ret = 0;
End:
	return ret;
}

static int 
SetFanSpeedValue(char speed_value)
{
	int index = 0;
	int ret = -1;
	int status = 0;

	for (; index<3; index++) {
		if (0x01 & (speed_value>>index)) {
			status = 1;
		} else {
			status = 0;
		}

		if (SYNO_CTRL_FAN_PERSISTER(index+1, status, 1)) {
			goto End;
		}
	}

	ret = 0;
End:
	return ret;
}

int
SetFanStatus(FAN_STATUS status, FAN_SPEED speed)
{
	char speed_value;
	int res = -EINVAL;
	int model = GetModel();

	switch (model) {
		case MODEL_RS409:
			/* this is only for RS409r1, not RS409, RS409 not use FAN_MULTI_ALWAYS in mapping.h
			 * so it never go to this case even you use this function point */
			if (FanStatusMappingRS409r1(status, speed, &speed_value)) {
				goto END;
			}
			break;
		default:
			if (FanStatusMappingType1(status, speed, &speed_value)) {
				goto END;
			}
	}

	// DS2.0 #8126
	if (-1 == SetFanSpeedValue(FAN_ACTIVATION_SPEED)) {
		goto END;
	}
	mdelay(FAN_ACTIVATION_DURATION);

	if (-1 == SetFanSpeedValue(speed_value)) {
		goto END;
	}

	res = 0;
END:
	return res;
}

static int
GetGpioBits(int start, int end, unsigned char *pValue)
{
	int i = start;
	int iRet = -1;
	GPIO_PIN pin;

	if (NULL == pValue ||
			start < end) {
		goto End;
	}

	for (; i > end-1; i--) {
		*pValue <<= 1;
		pin.pin = i;

		if (-1 == GetGpioPin( &pin )) {
			goto End;
		}

		*pValue |= pin.value;
	}

	iRet = 0;
End:
	return iRet;
}

int
GetFanSpeedBits(int start, int end, MEMORY_BYTE *pMemory)
{
	int iRet = -1;
	unsigned char value = 0;

	if ( NULL == pMemory || 
			0x3 != pMemory->offset ){
		goto End;
	}

	if (-1 == GetGpioBits(start, end, &value)) {
		goto End;
	}

	pMemory->value = value;

	iRet = 0;
End:
	return iRet;
}

static int GetBuzzerCleared(unsigned char *buzzer_cleared)
{
	int value;
	int model = GetModel();

	if (model != MODEL_RS409) {
		goto END;
	}

	SYNO_CTRL_BUZZER_CLEARED_GET(&value);
	if(value) {
		*buzzer_cleared = 1;
	} else {
		*buzzer_cleared = 0;
	}

END:
	return 0;
}

static struct synobios_ops synobios_ops = {
	.owner                = THIS_MODULE,
	.get_brand            = GetBrand,
	.get_model            = GetModel,
	.get_rtc_time         = rtc_ricoh_get_time,
	.set_rtc_time         = rtc_ricoh_set_time,
	.get_fan_status       = GetFanStatus,
	.set_fan_status       = SetFanStatus,
	.get_gpio_pin         = GetGpioPin,
	.set_gpio_pin         = SetGpioPin,
	.set_disk_led         = SetDiskLedStatus,
	.get_sys_temperature  = GetSysTemperature,
	.get_auto_poweron     = rtc_ricoh_get_auto_poweron,
	.set_auto_poweron     = rtc_ricoh_set_auto_poweron,
	.init_auto_poweron    = rtc_ricoh_auto_poweron_init,
	.uninit_auto_poweron  = rtc_ricoh_auto_poweron_uninit,
	.set_alarm_led        = SetAlarmLed,
	.get_backplane_status = GetBackPlaneStatus,
	.get_mem_byte         = GetMemByte,
	.get_buzzer_cleared   = GetBuzzerCleared,
	.module_type_init     = InitModuleType,
	.uninitialize         = Uninitialize,
};

int synobios_model_init(struct file_operations *fops, struct synobios_ops **ops)
{
	module_t* pSynoModule = NULL;

	if (synobios_ops.module_type_init) {
		synobios_ops.module_type_init(&synobios_ops);
	}

	pSynoModule = module_type_get();
	if( pSynoModule && RTC_SEIKO == pSynoModule->rtc_type ) {
		synobios_ops.get_rtc_time        = rtc_seiko_get_time;
		synobios_ops.set_rtc_time        = rtc_seiko_set_time;
		synobios_ops.get_auto_poweron    = rtc_seiko_get_auto_poweron;
		synobios_ops.set_auto_poweron    = rtc_seiko_set_auto_poweron;
		synobios_ops.init_auto_poweron   = rtc_seiko_auto_poweron_init;
		synobios_ops.uninit_auto_poweron = rtc_seiko_auto_poweron_uninit;
	}

	*ops = &synobios_ops;

	if( synobios_ops.init_auto_poweron ) {
		synobios_ops.init_auto_poweron();
	}

	return 0;
}

static int Uninitialize(void)
{
	if( synobios_ops.uninit_auto_poweron ) {
		synobios_ops.uninit_auto_poweron();
	}

	return 0;
}

int synobios_model_cleanup(struct file_operations *fops, struct synobios_ops **ops)
{
	return 0;
}

