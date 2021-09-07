// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/synobios.h>
#include "mv6281_common.h"

int 
InitModuleType(struct synobios_ops *ops)
{
	PRODUCT_MODEL model = ops->get_model();
	module_t type_110jv1 = MODULE_T_DS110jv1;
	module_t type_110jv2 = MODULE_T_DS110jv2;
	module_t type_110jv3 = MODULE_T_DS110jv3;
	module_t *pType = NULL;

	switch (model) {
	case MODEL_DS110j:
#ifdef MY_ABC_HERE
		if (syno_is_hw_version(HW_DS110jv20)) {
			pType = &type_110jv2;
		} else if (syno_is_hw_version(HW_DS110jv30)) {
			pType = &type_110jv3;
		} else {
			pType = &type_110jv1;
		}
#endif
		break;
	default:
		break;
	}

	module_type_set(pType);
	return 0;
}

int GetModel(void)
{
	return MODEL_DS110j;
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
