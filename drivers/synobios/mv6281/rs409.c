#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/string.h>
#include "../i2c/i2c-mv.h"
#include "mv6281_common.h"

#ifdef MY_ABC_HERE
#include <linux/synobios.h>
#endif

int GetModel(void)
{
	return MODEL_RS409;
}

int 
InitModuleType(struct synobios_ops *ops)
{
	PRODUCT_MODEL model = ops->get_model();
	module_t type_rs409v1 = MODULE_T_RS409v1;
	module_t type_rs409v2 = MODULE_T_RS409v2;
	module_t *pType = NULL;

	switch (model) {
	case MODEL_RS409:
#ifdef MY_ABC_HERE
		if (syno_is_hw_version(HW_RS409v20)) {
			pType = &type_rs409v2;
		} else {
			pType = &type_rs409v1;
		}
#endif
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
	return 0;
}
