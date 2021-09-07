// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include "bandon_common.h"

BANDON_FAN_SPEED_MAPPING gDS410pSpeedMapping[] = { 
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

BANDON_FAN_SPEED_MAPPING gDS410pCPUFanSpeedMapping[] = { 
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
int DS410pInitModuleType(struct synobios_ops *ops)
{	
	module_t type_410p = MODULE_T_DS410p;
	module_t *pType = &type_410p;
	
	module_type_set(pType);
	return 0;
}

static
int DS410pFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gDS410pSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gDS410pSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gDS410pSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

static
int DS410pCPUFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gDS410pCPUFanSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gDS410pCPUFanSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gDS410pCPUFanSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

struct model_ops ds410p_ops = {
	.x86_init_module_type = DS410pInitModuleType,
	.x86_fan_speed_mapping = DS410pFanSpeedMapping,
	.x86_set_esata_led_status = NULL,
	.x86_cpufan_speed_mapping = DS410pCPUFanSpeedMapping,
	.x86_get_buzzer_cleared = NULL,
};
