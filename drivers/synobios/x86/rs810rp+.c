// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include "bandon_common.h"

BANDON_FAN_SPEED_MAPPING gRS810rppSpeedMapping[] = {
	{ .fanSpeed = FAN_SPEED_STOP,       .iDutyCycle = 0  },
	{ .fanSpeed = FAN_SPEED_ULTRA_LOW,  .iDutyCycle = 15 },
	{ .fanSpeed = FAN_SPEED_VERY_LOW,   .iDutyCycle = 20 },
	{ .fanSpeed = FAN_SPEED_LOW,        .iDutyCycle = 25 },
	{ .fanSpeed = FAN_SPEED_MIDDLE,     .iDutyCycle = 35 },
	{ .fanSpeed = FAN_SPEED_HIGH,       .iDutyCycle = 45 },
	{ .fanSpeed = FAN_SPEED_VERY_HIGH,  .iDutyCycle = 55 },
	{ .fanSpeed = FAN_SPEED_ULTRA_HIGH, .iDutyCycle = 65 },
	{ .fanSpeed = FAN_SPEED_FULL,       .iDutyCycle = 99 },
};

BANDON_FAN_SPEED_MAPPING gRS810rppCPUFanSpeedMapping[] = {
	{ .fanSpeed = FAN_SPEED_STOP,       .iDutyCycle = 0  },
	{ .fanSpeed = FAN_SPEED_ULTRA_LOW,  .iDutyCycle = 15 },
	{ .fanSpeed = FAN_SPEED_VERY_LOW,   .iDutyCycle = 20 },
	{ .fanSpeed = FAN_SPEED_LOW,        .iDutyCycle = 25 },
	{ .fanSpeed = FAN_SPEED_MIDDLE,     .iDutyCycle = 35 },
	{ .fanSpeed = FAN_SPEED_HIGH,       .iDutyCycle = 45 },
	{ .fanSpeed = FAN_SPEED_VERY_HIGH,  .iDutyCycle = 55 },
	{ .fanSpeed = FAN_SPEED_ULTRA_HIGH, .iDutyCycle = 65 },
	{ .fanSpeed = FAN_SPEED_FULL,       .iDutyCycle = 99 },
};

static
int RS810rppInitModuleType(struct synobios_ops *ops)
{
	module_t type_rs810rpp = MODULE_T_RS810rpp;
	module_t *pType = &type_rs810rpp;

	module_type_set(pType);
	return 0;
}

static
int RS810rppFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gRS810rppSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gRS810rppSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gRS810rppSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

static
int RS810rppGetBuzzerCleared(unsigned char *buzzer_cleared)
{
    GPIO_PIN Pin;
    int ret = -1;

	if ( NULL == buzzer_cleared ) {
		goto End;
	}

	*buzzer_cleared = 0;

	Pin.pin = SYNO_GPP_RS_BUZZER_OFF;
    if ( 0 > GetGpioPin( &Pin ) ) {
        goto End;
    }

    if ( 0 == Pin.value ) {
        *buzzer_cleared = 1;
    }

    ret = 0;
End:
    return ret;
}

static
int RS810rppCPUFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gRS810rppCPUFanSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gRS810rppCPUFanSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gRS810rppCPUFanSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

#define GPIO_POWER_GOOD	1
static
int RS810rppGetPowerStatus(POWER_INFO *power_info)
{
	int err = -1;
	int pin15_Value = 0, pin25_Value = 0;

	if ( 0 != syno_ich9_lpc_gpio_pin(15 , &pin15_Value, 0) ) {
		goto FAIL;
	}

	if ( 0 != syno_ich9_lpc_gpio_pin(25 , &pin25_Value, 0) ) {
		goto FAIL;
	}

	if (pin15_Value == GPIO_POWER_GOOD) {
		power_info->power_1 = POWER_STATUS_GOOD;
	}else{
		power_info->power_1 = POWER_STATUS_BAD;
	}

	if (pin25_Value == GPIO_POWER_GOOD) {
		power_info->power_2 = POWER_STATUS_GOOD;
	}else{
		power_info->power_2 = POWER_STATUS_BAD;
	}

	err = 0;

FAIL:
	return err;
}

struct model_ops rs810rpp_ops = {
	.x86_init_module_type = RS810rppInitModuleType,
	.x86_fan_speed_mapping = RS810rppFanSpeedMapping,
	.x86_set_esata_led_status = NULL,
	.x86_cpufan_speed_mapping = RS810rppCPUFanSpeedMapping,
	.x86_get_buzzer_cleared = RS810rppGetBuzzerCleared,
	.x86_get_power_status    = RS810rppGetPowerStatus,
};
