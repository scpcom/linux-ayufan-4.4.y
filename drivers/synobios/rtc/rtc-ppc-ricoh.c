#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/synobios.h>
#include "../i2c/i2c-ppc.h"

int rtc_ricoh_get_time(struct _SynoRtcTimePkt *pRtcTimePkt)
{
    return mpc_i2c_read(0x32, (u8 *)pRtcTimePkt, 7, 0, 0);
}

int rtc_ricoh_set_time(struct _SynoRtcTimePkt *pRtcTimePkt)
{
	return mpc_i2c_write(0x32, (u8 *)pRtcTimePkt, 7, 0, 0);
}

int rtc_ricoh_get_auto_poweron(SYNO_AUTO_POWERON *pAutoPowerOn)
{
	int err = -1;    
	u8 csr;
	int offset = 0;
	
	if ( NULL == pAutoPowerOn ) {
		return -EINVAL;
	}
	
	if ( pAutoPowerOn->num < 1 || pAutoPowerOn->num > 2 ) {
		return -EINVAL;
	}
	
	if ( 0 > (err = mpc_i2c_read(I2C_RTC_ADDR, (u8 *)&csr, sizeof(csr)/sizeof(u8), 0, I2C_RTC_CONTROL1_OFFSET)) ) {
		goto End;
	}
	
	if ( pAutoPowerOn->num == 1 ) {
		pAutoPowerOn->enabled = (csr >> 7) & 0x1;
	} else {
		pAutoPowerOn->enabled = (csr >> 6) & 0x1;
	}
	
	if ( pAutoPowerOn->num == 1 ) {
		offset = I2C_RTC_ALARMA_OFFSET;
	} else {
		offset = I2C_RTC_ALARMB_OFFSET;
	}
	
	if ( 0 > (err = mpc_i2c_read(I2C_RTC_ADDR, (u8 *)&pAutoPowerOn->RtcAlarmPkt, 
	                             sizeof(pAutoPowerOn->RtcAlarmPkt)/sizeof(u8), 0, offset)) ) {
		goto End;
	}
	
	err = 0;

End:
	return err;
}

int rtc_ricoh_set_auto_poweron(SYNO_AUTO_POWERON *pAutoPowerOn)
{
	int err = -1;
	u8 csr = 0;
	int offset = 0;
	
	if ( NULL == pAutoPowerOn ) {
		return -EINVAL;
	}
	
	if ( pAutoPowerOn->num < 1 || pAutoPowerOn->num > 2 ) {
		return -EINVAL;
	}
	
	if ( 0 > (err = mpc_i2c_read(I2C_RTC_ADDR, (u8 *)&csr, sizeof(csr)/sizeof(u8), 0, I2C_RTC_CONTROL1_OFFSET)) ) {
		goto End;
	}
	
	if ( SYNO_AUTO_POWERON_DISABLE == pAutoPowerOn->enabled ) {
		 
		if ( pAutoPowerOn->num == 1 ) {
			csr &= ~I2C_RTC_ALARMA_ENABLE;
		} else {
			csr &= ~I2C_RTC_ALARMB_ENABLE;
		}
	
		if ( 0 > (err = mpc_i2c_write(I2C_RTC_ADDR, (u8 *)&csr, sizeof(csr)/sizeof(u8), 0, I2C_RTC_CONTROL1_OFFSET)) ) {
			goto End;
		}
	} else {
		 
		csr |= I2C_RTC_ALARMAB_SL;
		if ( pAutoPowerOn->num == 1 ) {
			csr |= I2C_RTC_ALARMA_ENABLE;
		} else {
			csr |= I2C_RTC_ALARMB_ENABLE;
		}
	
		if ( 0 > (err = mpc_i2c_write(I2C_RTC_ADDR, (u8 *)&csr, sizeof(csr)/sizeof(u8), 0, I2C_RTC_CONTROL1_OFFSET)) ) {
			goto End;
		}
	
                csr = 0;
		if ( 0 > (err = mpc_i2c_write(I2C_RTC_ADDR, (u8 *)&csr, sizeof(csr)/sizeof(u8), 0, I2C_RTC_CONTROL2_OFFSET)) ) {
			goto End;
		}
		
		if ( pAutoPowerOn->num == 1 ) {
			offset |= I2C_RTC_ALARMA_OFFSET;
		} else {
			offset |= I2C_RTC_ALARMB_OFFSET;
		}
		if ( 0 > (err = mpc_i2c_write(I2C_RTC_ADDR, (u8 *)&pAutoPowerOn->RtcAlarmPkt, 
		                              sizeof(pAutoPowerOn->RtcAlarmPkt)/sizeof(u8), 0, offset)) ) {
			goto End;
		}
	}
	
	err = 0;

End:
	return err;
}

int rtc_ricoh_auto_poweron_init(void)
{
	int err = -1;
	u8 csr = 0;

	if ( 0 > (err = mpc_i2c_write(I2C_RTC_ADDR, (u8 *)&csr, sizeof(csr)/sizeof(u8), 0, I2C_RTC_CONTROL2_OFFSET)) ) {
		goto End;
	}

	err = 0;

End:
	return err;
}

int rtc_ricoh_auto_poweron_uninit(void)
{
	return rtc_ricoh_auto_poweron_init();
}
