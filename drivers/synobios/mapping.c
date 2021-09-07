// Copyright (c) 2000-2009 Synology Inc. All rights reserved.
#include <asm/uaccess.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/poll.h>
#include "mapping.h"

#if 0
#define	DBGMESG(x...)	printk(x)
#else
#define	DBGMESG(x...)
#endif

module_t syno_module = MODULE_T_UNKNOWN;

void
module_type_set(module_t *pModule)
{
	if (NULL == pModule) {
#if defined(WARN_ON)
		WARN_ON(1);
#endif
		printk("Module type init error\n");
		goto End;
	}

	syno_module = *pModule;
End:
	return;
}

module_t *
module_type_get(void)
{
	return &syno_module;
}

#if SYNO_HAVE_KERNEL_VERSION(2,6,15)
int 
GetFanNum(int *pFanNum)
{
	int iRet = -1;

	*pFanNum = syno_module.fan_number;

	iRet = 0;
	return iRet;
}
#endif // SYNO_HAVE_KERNEL_VERSION(2,6,15)

int 
GetHwCapability(CAPABILITY *pCapability)
{
	int iRet = -1;

	if ( NULL == pCapability ) {
		iRet = -EINVAL;
		goto End;
	}

	pCapability->support = 0;

	switch (pCapability->id) {
	case CAPABILITY_FANCTRL:
		if (FANCTRL_YES == syno_module.fanctrl_type) {
			pCapability->support = 1;
		}
		break;
	case CAPABILITY_THERMAL:
		if (THERMAL_YES == syno_module.thermal_type) {
			pCapability->support = 1;
		}
		break;
	case CAPABILITY_AUTO_POWERON:
		if (AUTO_POWERON_YES == syno_module.auto_poweron_type) {
			pCapability->support = 1;
		}
		break; 
	case CAPABILITY_EBOX:
		if (EBOX_SUPPORT == syno_module.ebox_type) {
			pCapability->support = 1;
		}
		break;
	case CAPABILITY_CPU_TEMP:
			if (CPUTMP_YES == syno_module.cputmp_type) {
				pCapability->support = 1;
			}
			break;
	default:
		iRet = -EINVAL;
		goto End;
	}

	iRet = 0;
End:
	return iRet;
}

int
FanStatusMappingType1(FAN_STATUS status, FAN_SPEED speed, char *pSpeed_value)
{
	int ret = -1;

	if (status == FAN_STATUS_STOP) {
		*pSpeed_value = CPLD_FAN_SPEED_0;
	} else {
		switch (speed) {
		case FAN_SPEED_STOP:
		case FAN_SPEED_TEST_0:
			*pSpeed_value = CPLD_FAN_SPEED_0;
			break;
		case FAN_SPEED_ULTRA_LOW:
		case FAN_SPEED_TEST_1:
			*pSpeed_value = CPLD_FAN_SPEED_1;
			break;
		case FAN_SPEED_VERY_LOW:
		case FAN_SPEED_TEST_2:
			*pSpeed_value = CPLD_FAN_SPEED_2;
			break;
		/* by spec. The fan speed of 3/4 is inverted. 
		 * Because the last resistance is smaller than the sum of the others.
		 */
		case FAN_SPEED_MIDDLE:
		case FAN_SPEED_TEST_3:
			*pSpeed_value = CPLD_FAN_SPEED_3;
			break;
		case FAN_SPEED_LOW:
		case FAN_SPEED_TEST_4:
			*pSpeed_value = CPLD_FAN_SPEED_4;
			break;
		case FAN_SPEED_HIGH:
		case FAN_SPEED_TEST_5:
			*pSpeed_value = CPLD_FAN_SPEED_5;
			break;
		case FAN_SPEED_VERY_HIGH:
		case FAN_SPEED_TEST_6:
			*pSpeed_value = CPLD_FAN_SPEED_6;
			break;
		case FAN_SPEED_ULTRA_HIGH:
		case FAN_SPEED_FULL:
		case FAN_SPEED_TEST_7:
			*pSpeed_value = CPLD_FAN_SPEED_7;
			break;
		default:
			goto END;
		}
	}

	ret = 0;
END:
	return ret;
}

int
FanStatusMappingType2(FAN_STATUS status, FAN_SPEED speed, char *pSpeed_value)
{
	int ret = -1;

	if (status == FAN_STATUS_STOP) {
		*pSpeed_value = CPLD_FAN_SPEED_0;
	} else {
		switch (speed) {
		case FAN_SPEED_STOP:
		case FAN_SPEED_TEST_0:
			*pSpeed_value = CPLD_FAN_SPEED_0;
			break;
		case FAN_SPEED_ULTRA_LOW:
		case FAN_SPEED_TEST_1:
			*pSpeed_value = CPLD_FAN_SPEED_1;
			break;
		case FAN_SPEED_VERY_LOW:
        case FAN_SPEED_TEST_2:
			*pSpeed_value = CPLD_FAN_SPEED_2;
			break;
		case FAN_SPEED_LOW:
		case FAN_SPEED_TEST_3:
			*pSpeed_value = CPLD_FAN_SPEED_3;
			break;
		case FAN_SPEED_MIDDLE:	
		case FAN_SPEED_TEST_4:
			*pSpeed_value = CPLD_FAN_SPEED_4;
			break;
		case FAN_SPEED_HIGH:
		case FAN_SPEED_TEST_5:
			*pSpeed_value = CPLD_FAN_SPEED_5;
			break;
		case FAN_SPEED_VERY_HIGH:
		case FAN_SPEED_TEST_6:
			*pSpeed_value = CPLD_FAN_SPEED_6;
			break;
		case FAN_SPEED_ULTRA_HIGH:
		case FAN_SPEED_FULL:
		case FAN_SPEED_TEST_7:
			*pSpeed_value = CPLD_FAN_SPEED_7;
			break;
		default:
			goto END;
		}
	}

	ret = 0;
END:
	return ret;
}
