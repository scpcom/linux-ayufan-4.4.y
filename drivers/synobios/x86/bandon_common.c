// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>  /* error codes */
#include <linux/delay.h>
#include <linux/synobios.h>
#include <linux/fs.h>
#include <linux/cpumask.h>
#include "../rtc/rtc.h"
#include "bandon_common.h"

static struct model_ops *model_ops = NULL;

static
int GetFanStatus(int fanno, FAN_STATUS *pStatus)
{
	return 0;
}

static
int SetFanStatus(FAN_STATUS status, FAN_SPEED speed)
{
	int iRet = -1;
	int iFanDuty = -1;
	char szUartCmd[5] = {0};

	if( status == FAN_STATUS_STOP ) {
		speed = FAN_SPEED_STOP;
	}

	if( FAN_SPEED_PWM_FORMAT_SHIFT <= (int)speed ) {
		/* PWM format is only for bandon and QC test */
		iFanDuty = FAN_SPEED_SHIFT_DUTY_GET((int)speed);
		/* set fan speed hz */
		if(0 < FAN_SPEED_SHIFT_HZ_GET((int)speed)) {
			snprintf(szUartCmd, sizeof(szUartCmd), "%s%02d", SZ_UART_FAN_FREQUENCY, FAN_SPEED_SHIFT_HZ_GET((int)speed));
			if( 0 > SetUart(szUartCmd) ) {
				goto END;
			}
		}
	} else {
		if( 0 > (iFanDuty = model_ops->x86_fan_speed_mapping(speed)) ) {
			printk("No matched fan speed!\n");
			goto END;
		}
	}

	/* set fan speed duty cycle  */
	snprintf(szUartCmd, sizeof(szUartCmd), "%s%02d", SZ_UART_FAN_DUTY_CYCLE, iFanDuty);
	if( 0 > SetUart(szUartCmd) ) {
		goto END;
	}

	iRet = 0;
END:
	return iRet;
}

static
int SetCpuFanStatus(FAN_STATUS status, FAN_SPEED speed)
{
	int iRet = -1;
	int iFanDuty = -1;
	char szUartCmd[5] = {0};

	if( status == FAN_STATUS_STOP ) {
		speed = FAN_SPEED_STOP;
	}

	if( FAN_SPEED_PWM_FORMAT_SHIFT <= (int)speed ) {
		/* PWM format is only for bandon and QC test */
		iFanDuty = FAN_SPEED_SHIFT_DUTY_GET((int)speed);
		/* set fan speed hz */
		if(0 < FAN_SPEED_SHIFT_HZ_GET((int)speed)) {
			snprintf(szUartCmd, sizeof(szUartCmd), "%s%02d", SZ_UART_CPUFAN_FREQUENCY, FAN_SPEED_SHIFT_HZ_GET((int)speed));
			if( 0 > SetUart(szUartCmd) ) {
				goto END;
			}
		}
	} else {
		if( NULL ==  model_ops->x86_cpufan_speed_mapping ) {
			goto END;
		} else if( 0 > (iFanDuty = model_ops->x86_cpufan_speed_mapping(speed)) ) {
			printk("No matched fan speed!\n");
			goto END;
		}
	}

	snprintf(szUartCmd, sizeof(szUartCmd), "%s%02d", SZ_UART_CPUFAN_DUTY_CYCLE, iFanDuty);
	if( 0 > SetUart(szUartCmd) ) {
		goto END;
	}

	iRet = 0;
END:
	return iRet;
}

static
int Uninitialize(void)
{
	return 0;
}

int GetModel(void)
{
	int model = MODEL_DS710p;

	if (syno_is_hw_version(HW_DS710p)) {
		model = MODEL_DS710p;
	} else if (syno_is_hw_version(HW_DS1010p)) {
		model = MODEL_DS1010p;
	} else if (syno_is_hw_version(HW_DS410p)) {
		model = MODEL_DS410p;
	} else if (syno_is_hw_version(HW_RS810p)){
		model = MODEL_RS810p;
	} else if (syno_is_hw_version(HW_RS810rpp)) {
		model = MODEL_RS810rpp;
	}

	return model;
}

static
int GetBrand(void)
{
	return BRAND_SYNOLOGY;
}

static
int InitModuleType(struct synobios_ops *ops)
{
	int iRet = -1;

	if (model_ops && model_ops->x86_init_module_type) {
		iRet = model_ops->x86_init_module_type(ops);
	}

	return iRet;
}

int SetGpioPin( GPIO_PIN *pPin )
{
	int ret = -1;

	if ( NULL == pPin ) {
		goto End;
	}

	if ( pPin->pin < 100 ) {
		if ( 0 != syno_ich9_lpc_gpio_pin((int)pPin->pin, (int*)&pPin->value, 1) ) {
			goto End;
		}
	} else {
		if ( 0 != syno_superio_gpio_pin((int)pPin->pin - 100, (int*)&pPin->value, 1) ) {
			goto End;
		}
	}

	ret = 0;
End:
	return ret;
}

int GetGpioPin( GPIO_PIN *pPin )
{
	int ret = -1;

	if ( NULL == pPin ) {
		goto End;
	}

	if ( pPin->pin < 100 ) {
		if ( 0 != syno_ich9_lpc_gpio_pin((int)pPin->pin, (int*)&pPin->value, 0) ) {
			goto End;
		}
	} else {
		if ( 0 != syno_superio_gpio_pin((int)pPin->pin - 100, (int*)&pPin->value, 0) ) {
			goto End;
		}
	}

	ret = 0;
End:
	return ret;
}

static
int SetDiskLedStatus(int disknum, SYNO_DISK_LED status)
{
	int err = -1;
	GPIO_PIN Pin1, Pin2; 

	if ( status == DISK_LED_ORANGE_BLINK ) {
		status = DISK_LED_ORANGE_SOLID;
	}

	if ( status == DISK_LED_GREEN_SOLID ) {
		Pin1.value = 1;
		Pin2.value = 0;
	} else if ( status == DISK_LED_ORANGE_SOLID ) {
		Pin1.value = 0;
		Pin2.value = 1;
	} else if ( status == DISK_LED_OFF ) {
		Pin1.value = 0;
		Pin2.value = 0;
	}

	switch (disknum) {
	case 1:
		Pin1.pin = SYNO_GPP_HDD1_LED_0;
		Pin2.pin = SYNO_GPP_HDD1_LED_1;
		break;
	case 2:
		Pin1.pin = SYNO_GPP_HDD2_LED_0;
		Pin2.pin = SYNO_GPP_HDD2_LED_1;
		break;
	case 3:
		Pin1.pin = SYNO_GPP_HDD3_LED_0;
		Pin2.pin = SYNO_GPP_HDD3_LED_1;
		break;
	case 4:
		Pin1.pin = SYNO_GPP_HDD4_LED_0;
		Pin2.pin = SYNO_GPP_HDD4_LED_1;
		break;
	case 5:
		Pin1.pin = SYNO_GPP_HDD5_LED_0;
		Pin2.pin = SYNO_GPP_HDD5_LED_1;
		break;
	case 7:
		/* The eSata disk is /dev/sdg on DS710+,
		 * so it is the 7th disk on this model.
		 * ( 'g' - 'a' + 1 = 7 )
		 */
		if (model_ops && model_ops->x86_set_esata_led_status) {
			model_ops->x86_set_esata_led_status(status);
		}
		goto ESATA_END;
	case 6:
	case 8:
	case 9:
	case 10:
		//for esata
		err = 0;
		goto END;
	default:
		printk("Wrong HDD number [%d]\n", disknum);
		goto END;
	}

	SetGpioPin(&Pin1);
	SetGpioPin(&Pin2);

ESATA_END:
    err = 0;
END:
    return err;
}

static
int GetSysTemperature(int *Temperature)
{
	if ( NULL ) {
		return -1;
	}

	syno_sys_temperature(Temperature);

	return 0;
}

static
int GetCpuTemperature(struct _SynoCpuTemp *pCpuTemp)
{
	int iRet = -1;

	if ( NULL == pCpuTemp ) {
		goto END;
	}

	iRet = syno_cpu_temperature(pCpuTemp);

END:
	return iRet;
}

static
int SetAlarmLed(unsigned char type)
{	
	const char* cmd = NULL;

	if (type) {
		cmd = SZ_UART_ALARM_LED_BLINKING;
	}else{
		cmd = SZ_UART_ALARM_LED_OFF;
	}		
	
	return SetUart(cmd);
}

static 
int GetBuzzerCleared(unsigned char *buzzer_cleared)
{
	int ret = 0;

	if (model_ops && model_ops->x86_get_buzzer_cleared) {
		ret = model_ops->x86_get_buzzer_cleared(buzzer_cleared);
	}

	return ret;
}

static int GetPowerStatus(POWER_INFO *power_info)
{
	int ret = 0;

	if (model_ops && model_ops->x86_get_power_status) {
		ret = model_ops->x86_get_power_status(power_info);
	}else{
		ret = -1;
	}

	return ret;
}

static struct synobios_ops synobios_ops = {
	.owner               = THIS_MODULE,
	.get_brand           = GetBrand,	
	.get_model           = GetModel,
	.get_rtc_time        = rtc_bandon_get_time,
	.set_rtc_time        = rtc_bandon_set_time,
	.get_auto_poweron	 = rtc_bandon_get_auto_poweron,
	.set_auto_poweron	 = rtc_bandon_set_auto_poweron,
	.get_fan_status      = GetFanStatus,
	.set_fan_status      = SetFanStatus,
	.get_sys_temperature = GetSysTemperature,
	.get_cpu_temperature = GetCpuTemperature,
	.set_cpu_fan_status  = SetCpuFanStatus,
	.get_gpio_pin        = GetGpioPin,
	.set_gpio_pin        = SetGpioPin,
	.set_disk_led        = SetDiskLedStatus,	
	.set_alarm_led       = SetAlarmLed,		
	.module_type_init    = InitModuleType,
	.get_buzzer_cleared  = GetBuzzerCleared,
	.get_power_status    = GetPowerStatus,
	.uninitialize		 = Uninitialize,
};

int synobios_model_init(struct file_operations *fops, struct synobios_ops **ops)
{
	*ops = &synobios_ops;
	
	switch(GetModel())
	{
	case MODEL_DS710p:
		model_ops = &ds710p_ops;
		break;
	case MODEL_DS1010p:
		model_ops = &ds1010p_ops;
		break;
	case MODEL_DS410p:
		model_ops = &ds410p_ops;
		break;
	case MODEL_RS810p:
		model_ops = &rs810p_ops;
		break;
	case MODEL_RS810rpp:
		model_ops = &rs810rpp_ops;
		break;
	}

	return 0;
}

int synobios_model_cleanup(struct file_operations *fops, struct synobios_ops **ops)
{
	return 0;
}
