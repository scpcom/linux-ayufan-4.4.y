#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/synobios.h>
#include "../i2c/i2c-ppc.h"
#include "rtc.h"

#define SEIKO_RTC_STATUS1_ADDR   0x30
#define SEIKO_RTC_STATUS2_ADDR   0x31
#define SEIKO_RTC_REALTIME1_ADDR 0x32
#define SEIKO_RTC_REALTIME2_ADDR 0x33
#define SEIKO_RTC_INT1_ADDR      0x34
#define SEIKO_RTC_INT2_ADDR      0x35
#define SEIKO_RTC_FREE_ADDR      0x37

#define DEC2BCD(dec) (((dec/10)*16)+(dec%10))
#define BCD2DEC(val) ((val)=((val)&15) + ((val)>>4)*10)

#define RB(v) (reverse_bits(v))
#define CHAR_BIT 8
static unsigned char
reverse_bits(unsigned char v) {
	unsigned char r = v;               
	int s = sizeof(v) * CHAR_BIT - 1;  

	for( v >>= 1; v; v >>= 1 ) {
		r <<= 1;
		r |= v & 1;
		s--;
	}
	return r <<= s;  
}

static
unsigned char Hour_Ricoh_to_Seiko(const unsigned char hour)
{
	if( hour == 0x12 ) {
		return 0x00;
	} else if( hour == 0x32 ) {
		return 0x40;
	} else if( hour > 0x20 ) {
		return hour - 0x20 + 0x40;
	}
	return hour;
}

static
unsigned char Hour_Seiko_to_Ricoh(const unsigned char hour)
{
	if( hour == 0x00 ) {
		return 0x12;
	} else if( hour == 0x40 ) {
		return 0x32;
	} else if( hour > 0x40 ) {
		return hour - 0x40 + 0x20;
	}
	return hour;
}

#if 0
static
void rtc_dump_interrupt_register(void)
{
	unsigned char data[3] = {0, 0, 0};

	mpc_i2c_read(SEIKO_RTC_INT1_ADDR, (u8*)data, 3, 0, -1);
	printk("[INT1] weekday: %02X, hour: %02X, min: %02X, (%1X%1X%1X)\n", 
			RB(data[0]&0xFE), Hour_Seiko_to_Ricoh(RB(data[1]&0xFE)), RB(data[2]&0xFE),
			data[0]&0x1, data[1]&0x1, data[2]&0x1);

	mpc_i2c_read(SEIKO_RTC_INT2_ADDR, (u8*)data, 3, 0, -1);
	printk("[INT2] weekday: %02X, hour: %02X, min: %02X, (%1X%1X%1X)\n", 
			RB(data[0]&0xFE), Hour_Seiko_to_Ricoh(RB(data[1]&0xFE)), RB(data[2]&0xFE),
			data[0]&0x1, data[1]&0x1, data[2]&0x1);

	mpc_i2c_read(SEIKO_RTC_FREE_ADDR, (u8*)data, 1, 0, -1);
	printk("[FREE] weekdays: %02X\n", data[0]);
}
#endif

int rtc_seiko_get_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn)
{
	int iRet = -1;
	unsigned char rgIntReg[3] = {0, 0, 0};

	if( NULL == pAutoPowerOn || pAutoPowerOn->num != 1 ) {
		goto End;
	}

	if( (iRet = mpc_i2c_read(SEIKO_RTC_INT1_ADDR, (u8 *)rgIntReg,
					sizeof(rgIntReg)/sizeof(unsigned char), 0, -1) ) < 0 ) {
		goto End;
	}
	pAutoPowerOn->enabled = rgIntReg[2] & 0x01;
	pAutoPowerOn->RtcAlarmPkt.min = RB(rgIntReg[2] & 0xFE);	 
	pAutoPowerOn->RtcAlarmPkt.hour = Hour_Seiko_to_Ricoh(RB(rgIntReg[1] & 0xFE));

	if( (iRet = mpc_i2c_read(SEIKO_RTC_FREE_ADDR, (u8 *)&(pAutoPowerOn->RtcAlarmPkt.weekdays),
					sizeof(unsigned char)/sizeof(u8), 0, -1) ) < 0 ) {
		goto End;
	}

	iRet = 0;
End:
	return 0;
}

static
unsigned char rtc_get_num_weekdays(const SYNO_AUTO_POWERON* pAutoPowerOn)
{
	unsigned char u8Num = 0;
	unsigned char u8WeekDays;

	if( NULL == pAutoPowerOn ) {
		goto End;
	}

	u8WeekDays = pAutoPowerOn->RtcAlarmPkt.weekdays & AUTO_POWERON_WEEKDAY_MASK;
	for( ; u8WeekDays > 0; u8WeekDays >>= 1 ) {
		if( u8WeekDays & 0x01 ) {
			u8Num++;
		}
	}

End:
	return u8Num;
}

static
unsigned char rtc_get_int1_weekday(void)
{
	unsigned char data[3] = {0, 0, 0};
	mpc_i2c_read(SEIKO_RTC_INT1_ADDR, (u8*)data, 3, 0, -1);
	return RB(data[0] & 0xFE);  
}

static
unsigned char rtc_get_next_weekday(const SYNO_AUTO_POWERON* pAutoPowerOn, const unsigned char weekday)
{
	unsigned char u8Nextday = 0xFF;
	unsigned int mask = 1 << weekday;
	unsigned int weekdays = pAutoPowerOn->RtcAlarmPkt.weekdays & AUTO_POWERON_WEEKDAY_MASK;

	if( weekdays == 0 ) {  
		goto End;
	}

	u8Nextday = weekday;
	weekdays |= weekdays << 7;  
	while( !(mask & weekdays) ) {
		mask <<= 1;
		u8Nextday++;
	}
	u8Nextday %= 7;

End:
	return u8Nextday;
}

static
int rtc_reset_interrupt_mode(void)
{
	int iRet = -1;
	u8 csr = 0x00;

	iRet = mpc_i2c_write(SEIKO_RTC_STATUS2_ADDR, (u8 *)&csr, 1, 0, -1);

	csr = 0x22;
	iRet = mpc_i2c_write(SEIKO_RTC_STATUS2_ADDR, (u8 *)&csr, 1, 0, -1);

	return iRet;
}

static
int rtc_set_interrupt(const unsigned char intAddr, const unsigned char data[3], const int enable)
{
	int iRet = -1;
	unsigned char rgIntReg[3] = {0, 0, 0};

	if( intAddr != SEIKO_RTC_INT1_ADDR && intAddr != SEIKO_RTC_INT2_ADDR ) {
		goto End;
	}

	rgIntReg[2] = RB(data[2]);
	rgIntReg[1] = RB(Hour_Ricoh_to_Seiko(data[1]));
	rgIntReg[0] = RB(data[0]);

	if( enable ) {
		rgIntReg[2] |= 0x01;
		rgIntReg[1] |= 0x01;
		rgIntReg[0] |= 0x01;
	}

	rtc_reset_interrupt_mode();

	if( (iRet = mpc_i2c_write(intAddr, (u8 *)rgIntReg, 3, 0, -1) ) < 0 ) {
		goto End;
	}

	iRet = 0;
End:
	return iRet;
}

static
int rtc_later_equal_than_int(unsigned char rtcHour, const unsigned char rtcMin,
                       unsigned char intHour, const unsigned char intMin)
{
	if( 0x12 == rtcHour ) {
		rtcHour = 0x00;
	} else if( 0x32 == rtcHour ) {
		rtcHour = 0x12;
	}

	if( 0x12 == intHour ) {
		intHour = 0x00;
	} else if( 0x32 == intHour ) {
		intHour = 0x12;
	}

	if( rtcHour > intHour || (rtcHour == intHour && rtcMin >= intMin) ) {
		return 1;
	}

	return 0;
}

static
int rtc_rotate_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn,
                            const char u8Today, const char u8Hour, const char u8Min)
{
	int iRet = -1;
	unsigned char u8NextDay = 0;
	unsigned char rgIntReg[3] = {0, 0, 0};
	const unsigned char u8NumWeekdays = rtc_get_num_weekdays(pAutoPowerOn);

	if( NULL == pAutoPowerOn ) {
		goto End;
	}

	rgIntReg[2] = pAutoPowerOn->RtcAlarmPkt.min;
	rgIntReg[1] = pAutoPowerOn->RtcAlarmPkt.hour;

	u8NextDay = rtc_get_next_weekday(pAutoPowerOn, u8Today);
	if( u8Today == u8NextDay && rtc_later_equal_than_int(u8Hour, u8Min, rgIntReg[1], rgIntReg[2]) ) {
		 
		u8NextDay = rtc_get_next_weekday(pAutoPowerOn, (u8NextDay+1)%7);
	}

	if( 2 <= u8NumWeekdays && rtc_get_int1_weekday() != u8NextDay ) {
		 
		rgIntReg[0] = u8NextDay;
		rtc_set_interrupt(SEIKO_RTC_INT1_ADDR, rgIntReg, 1);
		rgIntReg[0] = rtc_get_next_weekday(pAutoPowerOn, (u8NextDay+1)%7);
		rtc_set_interrupt(SEIKO_RTC_INT2_ADDR, rgIntReg, 1);
	}

	iRet = 0;
End:
	return iRet;
}

int rtc_seiko_set_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn)
{
	int iRet = -1;
	unsigned char rgIntReg[3] = {0, 0, 0};
	unsigned char u8Today, u8Hour, u8Min;
	const unsigned char rgEmptyData[3] = {0, 0, 0};
	const unsigned char u8NumWeekdays = rtc_get_num_weekdays(pAutoPowerOn);
	SYNORTCTIMEPKT rtcTime;

	if( NULL == pAutoPowerOn || pAutoPowerOn->num != 1 ) {
		goto End;
	}

	if( 0 > rtc_seiko_get_time(&rtcTime) ) {
		printk("Failed to get time from rtc.\n");
		goto End;
	} else {
		u8Today = rtcTime.weekday;
		u8Hour = rtcTime.hour;
		u8Min = rtcTime.min;
	}

	rgIntReg[2] = pAutoPowerOn->RtcAlarmPkt.min;
	rgIntReg[1] = pAutoPowerOn->RtcAlarmPkt.hour;

	if( SYNO_AUTO_POWERON_DISABLE == pAutoPowerOn->enabled ) {
		rtc_set_interrupt(SEIKO_RTC_INT1_ADDR, rgEmptyData, 0);
		rtc_set_interrupt(SEIKO_RTC_INT2_ADDR, rgEmptyData, 0);
	} else if( u8NumWeekdays == 1 ) {
		rgIntReg[0] = rtc_get_next_weekday(pAutoPowerOn, u8Today);
		rtc_set_interrupt(SEIKO_RTC_INT1_ADDR, rgIntReg, 1);
		rtc_set_interrupt(SEIKO_RTC_INT2_ADDR, rgEmptyData, 0);
	} else if( u8NumWeekdays >= 2 ) {
		 
		unsigned char u8FirstDay = rtc_get_next_weekday(pAutoPowerOn, u8Today);
		if( u8Today == u8FirstDay && rtc_later_equal_than_int(u8Hour, u8Min, rgIntReg[1], rgIntReg[2]) ) {
			 
			u8FirstDay = rtc_get_next_weekday(pAutoPowerOn, (u8FirstDay+1)%7);
		}

		rgIntReg[0] = u8FirstDay;
		rtc_set_interrupt(SEIKO_RTC_INT1_ADDR, rgIntReg, 1);
		rgIntReg[0] = rtc_get_next_weekday(pAutoPowerOn, (u8FirstDay+1)%7);
		rtc_set_interrupt(SEIKO_RTC_INT2_ADDR, rgIntReg, 1);
	} else {
		goto End;
	}

	if( (iRet = mpc_i2c_write(SEIKO_RTC_FREE_ADDR, (u8*)&(pAutoPowerOn->RtcAlarmPkt.weekdays),
					sizeof(unsigned char)/sizeof(u8), 0, -1) ) < 0 ) {
		goto End;
	}

	iRet = 0;
End:
	return iRet;
}

int rtc_seiko_get_time(struct _SynoRtcTimePkt* pRtcTimePkt)
{
	int iRet = -1;
	unsigned char rgRtcTimeTemp[7];
	SYNO_AUTO_POWERON schedule;

	if( (iRet = mpc_i2c_read(SEIKO_RTC_REALTIME1_ADDR, (u8 *)rgRtcTimeTemp, 
					sizeof(rgRtcTimeTemp)/sizeof(unsigned char), 0, -1) < 0) ) {
		goto End;
	}

	pRtcTimePkt->sec     = RB(rgRtcTimeTemp[6]);
	pRtcTimePkt->min     = RB(rgRtcTimeTemp[5]);
	pRtcTimePkt->hour    = Hour_Seiko_to_Ricoh(RB(rgRtcTimeTemp[4]));
	pRtcTimePkt->weekday = RB(rgRtcTimeTemp[3]);
	pRtcTimePkt->day     = RB(rgRtcTimeTemp[2]);
	pRtcTimePkt->month   = RB(rgRtcTimeTemp[1]);
	pRtcTimePkt->year    = RB(rgRtcTimeTemp[0]);

	schedule.num = 1;
	if( 0 == rtc_seiko_get_auto_poweron(&schedule) && schedule.enabled ) {
		rtc_rotate_auto_poweron(&schedule, pRtcTimePkt->weekday, pRtcTimePkt->hour, pRtcTimePkt->min);
	}

End:
	return iRet;
}

int rtc_seiko_set_time(struct _SynoRtcTimePkt* pRtcTimePkt)
{
	int iRet = -1;
	SYNO_AUTO_POWERON schedule;
	unsigned char rgRtcTimeTemp[] = {
		RB(pRtcTimePkt->year),
		RB(pRtcTimePkt->month),
		RB(pRtcTimePkt->day),
		RB(pRtcTimePkt->weekday),
		RB(Hour_Ricoh_to_Seiko(pRtcTimePkt->hour)),
		RB(pRtcTimePkt->min),
		RB(pRtcTimePkt->sec),
	};

	if( (iRet = mpc_i2c_write(SEIKO_RTC_REALTIME1_ADDR, (u8 *)rgRtcTimeTemp, 
					sizeof(rgRtcTimeTemp)/sizeof(unsigned char), 0, -1) < 0) ) {
		goto End;
	}

	schedule.num = 1;
	if( 0 == rtc_seiko_get_auto_poweron(&schedule) && schedule.enabled ) {
		rtc_rotate_auto_poweron(&schedule, pRtcTimePkt->weekday, pRtcTimePkt->hour, pRtcTimePkt->min);
	}

End:
	return iRet;
}

int rtc_seiko_auto_poweron_init(void)
{
	return rtc_reset_interrupt_mode();
}

int rtc_seiko_auto_poweron_uninit(void)
{
	 
	SYNORTCTIMEPKT rtc_time_pkt;
	rtc_seiko_get_time(&rtc_time_pkt);

	return rtc_reset_interrupt_mode();
}
