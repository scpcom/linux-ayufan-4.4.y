// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include "mv6281_common.h"

int 
InitModuleType(struct synobios_ops *ops)
{
	PRODUCT_MODEL model = ops->get_model();
	module_t type_109 = MODULE_T_DS109;
	module_t *pType = NULL;

	switch (model) {
	case MODEL_DS109:
		pType = &type_109;
		break;
	default:
		break;
	}

	module_type_set(pType);
	return 0;
}

int GetModel(void)
{
	return MODEL_DS109;
}

int SetDiskLedStatus(int disknum, SYNO_DISK_LED status)
{
	return -EINVAL;
}

int GetSysTemperature(int *Temperature)
{
	return 0;
}

int GetFanStatus(int fanno, FAN_STATUS *pStatus)
{
	int FanStatus;
	char rgcVolt[2] = {0, 0};

	if ( 1 != fanno ) {
		return -EINVAL;
	}

	do {
		SYNO_CTRL_FAN_STATUS_GET(fanno, &FanStatus);
		rgcVolt[(int)FanStatus] ++;
		if (rgcVolt[0] && rgcVolt[1]) {
			break; 
		}       
		udelay(300);
	} while ( (rgcVolt[0] + rgcVolt[1]) < 200 );

	if ((rgcVolt[0] == 0) || (rgcVolt[1] == 0) ) {
		*pStatus = FAN_STATUS_STOP;
	} else {
		*pStatus = FAN_STATUS_RUNNING;
	}

	return 0;
}

int GetMemByte( MEMORY_BYTE *pMemory )
{
	return GetFanSpeedBits(34, 32, pMemory);
}

int SetAlarmLed(unsigned char type)
{
	return 0;
}

int GetBackPlaneStatus(BACKPLANE_STATUS *pStatus)
{
	return 0;
}
