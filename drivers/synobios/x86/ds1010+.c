// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include "bandon_common.h"

BANDON_FAN_SPEED_MAPPING gDS1010pSpeedMapping[] = { 
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
int DS1010pInitModuleType(struct synobios_ops *ops)
{	
	module_t type_1010p = MODULE_T_DS1010p;
	module_t *pType = &type_1010p;
	
	module_type_set(pType);
	return 0;
}

static
int DS1010pFanSpeedMapping(FAN_SPEED speed)
{
	int iDutyCycle = -1;
	size_t i;

	for( i = 0; i < sizeof(gDS1010pSpeedMapping)/sizeof(BANDON_FAN_SPEED_MAPPING); ++i ) {
		if( gDS1010pSpeedMapping[i].fanSpeed == speed ) {
			iDutyCycle = gDS1010pSpeedMapping[i].iDutyCycle;
			break;
		}
	}

	return iDutyCycle;
}

struct model_ops ds1010p_ops = {
	.x86_init_module_type = DS1010pInitModuleType,
	.x86_fan_speed_mapping = DS1010pFanSpeedMapping,
	.x86_set_esata_led_status = NULL,
	.x86_cpufan_speed_mapping = NULL,
	.x86_get_buzzer_cleared = NULL,
};
