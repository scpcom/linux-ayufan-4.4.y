// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include "../i2c/i2c-mv.h"
#include "mv6281_common.h"

int GetModel(void)
{
	return MODEL_DS409;
}

int 
InitModuleType(struct synobios_ops *ops)
{
	PRODUCT_MODEL model = ops->get_model();
	module_t type_409 = MODULE_T_DS409;
	module_t *pType = NULL;

	switch (model) {
		case MODEL_DS409:
			pType = &type_409;
			break;
		default:
			break;
	}

	module_type_set(pType);
	return 0;
}

int SetDiskLedStatus(int disknum, SYNO_DISK_LED status)
{
	return SYNO_CTRL_EXT_CHIP_HDD_LED_SET(disknum, status);
}

int GetSysTemperature(int *Temperature)
{
	int ret = 0;
	u16 data = 0;

#ifdef CONFIG_MACH_SYNOLOGY_6281
	ret = mvI2CCharRead(0x48, (u8 *)&data, 2, -1);
#else
	ret = mvI2CCharRead(0x48, (u8 *)&data, 2, 0);
#endif
	if (ret != 0) {
		printk("Failed to read temperature from i2c. ret: %d, data:0x%x\n", ret, data);
		return -1;
	}

#if defined (__LITTLE_ENDIAN)
	data = __swab16(data);
#endif

	/* The temperature data only 9 bits */
	data = data >> 7;

	if (data >> 8) { /* bit 9 is minus sign */
		/* subzero */
		*Temperature = -1 * (0x100 - ((u8 *)&data)[1]);
	} else {
		*Temperature = data;
	}

	return 0;
}

int GetFanStatus(int fanno, FAN_STATUS *pStatus)
{
	int FanStatus;

	if (pStatus == NULL) {
		return -EINVAL;
	}

	SYNO_CTRL_FAN_STATUS_GET(fanno, &FanStatus);
	if ( FanStatus ) {
		*pStatus = FAN_STATUS_RUNNING;
	} else {		
		*pStatus = FAN_STATUS_STOP;
	}

	return 0;
}

int SetAlarmLed(unsigned char type)
{
	return 0;
}

int GetBackPlaneStatus(BACKPLANE_STATUS *pStatus)
{
	return 0;
}

int GetMemByte( MEMORY_BYTE *pMemory )
{
	return GetFanSpeedBits(17, 15, pMemory);
}
