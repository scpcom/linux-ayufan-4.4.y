#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "rtc/integ_rtc/mvRtc.h"
#include "rtc/integ_rtc/mvRtcReg.h"

MV_VOID mvRtcTimeSet(MV_RTC_TIME* mvTime)
{
	MV_U32 timeVal = 0;
	MV_U32 dateVal = 0;
	MV_U32 tens;
	MV_U32 single;
	
	tens = mvTime->seconds / 10;
	single = mvTime->seconds % 10;
	timeVal |= ((tens << RTC_TIME_10SECONDS_SHF) & RTC_TIME_10SECONDS_MSK) |
		  (( single << RTC_TIME_SECONDS_SHF) & RTC_TIME_SECONDS_MSK);

	tens = mvTime->minutes / 10;
	single = mvTime->minutes % 10;
	timeVal |= ((tens  << RTC_TIME_10MINUTES_SHF) & RTC_TIME_10MINUTES_MSK) |
		  (( single << RTC_TIME_MINUTES_SHF) & RTC_TIME_MINUTES_MSK);

	tens = mvTime->hours / 10;
	single = mvTime->hours % 10;
	timeVal |= ((tens  << RTC_TIME_10HOUR_SHF) & RTC_TIME_10HOUR_MSK) |
		  (( single  << RTC_TIME_HOUR_SHF) & RTC_TIME_HOUR_MSK);
  
	single = ++(mvTime->day);
	timeVal |= ((single << RTC_TIME_DAY_SHF ) & RTC_TIME_DAY_MSK);

	MV_REG_WRITE(RTC_TIME_REG, timeVal);

	tens = mvTime->date / 10;
	single = mvTime->date % 10;
	dateVal = ((tens  << RTC_DATE_10DAY_SHF) & RTC_DATE_10DAY_MSK) |
		  (( single << RTC_DATE_DAY_SHF) & RTC_DATE_DAY_MSK);

	tens = mvTime->month / 10;
	single = mvTime->month % 10;
	dateVal |= ((tens  << RTC_DATE_10MONTH_SHF) & RTC_DATE_10MONTH_MSK) |
		  (( single << RTC_DATE_MONTH_SHF) & RTC_DATE_MONTH_MSK);
    
	tens = mvTime->year / 10;
	single = mvTime->year % 10;
	dateVal |= ((tens  << RTC_DATE_10YEAR_SHF) & RTC_DATE_10YEAR_MSK) |
		  (( single << RTC_DATE_YEAR_SHF) & RTC_DATE_YEAR_MSK);

	MV_REG_WRITE(RTC_DATE_REG, dateVal);

	return;
}

MV_VOID mvRtcTimeGet(MV_RTC_TIME* mvTime)
{
	MV_U32 timeVal;
	MV_U32 dateVal;
	MV_U8 tens;
	MV_U8 single;

	timeVal = MV_REG_READ(RTC_TIME_REG);
	
	tens = ((timeVal & RTC_TIME_10SECONDS_MSK) >> RTC_TIME_10SECONDS_SHF);
	single = ((timeVal & RTC_TIME_SECONDS_MSK) >> RTC_TIME_SECONDS_SHF);
	mvTime->seconds = 10*tens + single;

	tens = ((timeVal & RTC_TIME_10MINUTES_MSK) >> RTC_TIME_10MINUTES_SHF);
	single = ((timeVal & RTC_TIME_MINUTES_MSK) >> RTC_TIME_MINUTES_SHF);
	mvTime->minutes = 10*tens + single;

	tens = ((timeVal & RTC_TIME_10HOUR_MSK) >> RTC_TIME_10HOUR_SHF);
	single = ((timeVal & RTC_TIME_HOUR_MSK) >> RTC_TIME_HOUR_SHF);
	mvTime->hours = 10*tens + single;

	mvTime->day = ((timeVal & RTC_TIME_DAY_MSK) >> RTC_TIME_DAY_SHF);
	mvTime->day--;

	dateVal = MV_REG_READ(RTC_DATE_REG);

	tens = ((dateVal & RTC_DATE_10DAY_MSK) >> RTC_DATE_10DAY_SHF);
	single = ((dateVal & RTC_DATE_DAY_MSK) >> RTC_DATE_DAY_SHF);
	mvTime->date = 10*tens + single;

	tens = ((dateVal & RTC_DATE_10MONTH_MSK) >> RTC_DATE_10MONTH_SHF);
	single = ((dateVal & RTC_DATE_MONTH_MSK) >> RTC_DATE_MONTH_SHF);
	mvTime->month = 10*tens + single;

	tens = ((dateVal & RTC_DATE_10YEAR_MSK) >> RTC_DATE_10YEAR_SHF);
	single = ((dateVal  & RTC_DATE_YEAR_MSK) >> RTC_DATE_YEAR_SHF);
	mvTime->year = 10*tens + single;

	return;	
}

MV_VOID mvRtcInit(MV_VOID)
{
	return;
}

#ifdef MY_ABC_HERE
 
EXPORT_SYMBOL(mvRtcTimeSet);
EXPORT_SYMBOL(mvRtcTimeGet);
#endif
