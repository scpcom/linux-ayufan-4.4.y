// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include "bandon_common.h"

BANDON_FAN_SPEED_MAPPING gRS810pSpeedMapping[] = { 
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

BANDON_FAN_SPEED_MAPPING gRS810pCPUFanSpeedMapping[] = { 
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
int RS810pInitModuleType(struct synobios_ops *ops)
{	
	module_t type_rs810p = MODULE_T_RS810p;
	module_t *pType = &type_rs810p;
	
	module_type_set(pType);
	return 0;
}

static
int RS810pFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gRS810pSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gRS810pSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gRS810pSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

static
int RS810pGetBuzzerCleared(unsigned char *buzzer_cleared)
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
int RS810pCPUFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gRS810pCPUFanSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gRS810pCPUFanSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gRS810pCPUFanSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

struct model_ops rs810p_ops = {
	.x86_init_module_type = RS810pInitModuleType,
	.x86_fan_speed_mapping = RS810pFanSpeedMapping,
	.x86_set_esata_led_status = NULL,
	.x86_cpufan_speed_mapping = RS810pCPUFanSpeedMapping,
	.x86_get_buzzer_cleared = RS810pGetBuzzerCleared,
};
