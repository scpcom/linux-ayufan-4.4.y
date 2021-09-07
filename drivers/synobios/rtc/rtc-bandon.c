#include <linux/syno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mc146818rtc.h>
#include <linux/bcd.h>
#include <linux/synobios.h>
#include "localtime.h"
#include "rtc.h"

#define BCD2BIN bcd2bin
#define BIN2BCD bin2bcd

#define RTC_FREE_ADDR1	0x1F /* for alarm weekday */

#define RTC_IRQMASK         (RTC_PF | RTC_AF | RTC_UF)
#define RTC_MDAY_ALARM_MASK 0x3F

static unsigned long epoch = 1900;  /* year corresponding to 0x00   */
static const unsigned char days_in_mo[] =
{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static
void rtc_bandon_dump(void)
{
	spin_lock_irq(&rtc_lock);
	printk("##################################\n");
	printk("CMOS_READ(RTC_YEAR)         =%02d\n", BCD2BIN(CMOS_READ(RTC_YEAR)));
	printk("CMOS_READ(RTC_MONTH)        =%02d\n", BCD2BIN(CMOS_READ(RTC_MONTH)));
	printk("CMOS_READ(RTC_DAY_OF_MONTH) =%02d\n", BCD2BIN(CMOS_READ(RTC_DAY_OF_MONTH)));
	printk("CMOS_READ(RTC_DAY_OF_WEEK)  =%02d\n", CMOS_READ(RTC_DAY_OF_WEEK));
	printk("CMOS_READ(RTC_HOURS)        =%02d\n", BCD2BIN(CMOS_READ(RTC_HOURS)));
	printk("CMOS_READ(RTC_MINUTES)      =%02d\n", BCD2BIN(CMOS_READ(RTC_MINUTES)));
	printk("CMOS_READ(RTC_SECONDS)      =%02d\n", BCD2BIN(CMOS_READ(RTC_SECONDS)));
	printk("CMOS_READ(RTC_HOURS_ALARM)  =%02d\n", BCD2BIN(CMOS_READ(RTC_HOURS_ALARM)));
	printk("CMOS_READ(RTC_MINUTES_ALARM)=%02d\n", BCD2BIN(CMOS_READ(RTC_MINUTES_ALARM)));
	printk("CMOS_READ(RTC_SECONDS_ALARM)=%02d\n", BCD2BIN(CMOS_READ(RTC_SECONDS_ALARM)));
	printk("CMOS_READ(RTC_FREE_ADDR1)   =%02d\n", CMOS_READ(RTC_FREE_ADDR1));
	printk("CMOS_READ(RTC_VALID)        =0x%02x\n", CMOS_READ(RTC_VALID));
	printk("CMOS_READ(RTC_CONTROL)      =0x%02x\n", CMOS_READ(RTC_CONTROL));
	printk("CMOS_READ(RTC_INTR_FLAGS)   =0x%02x\n", CMOS_READ(RTC_INTR_FLAGS));
	spin_unlock_irq(&rtc_lock);
}

static
int rtc_correct_wday(SYNORTCTIMEPKT *pRtcTime)
{
    time_t t;
    struct xtm taget_time;

    t = mktime(pRtcTime->year+1900, pRtcTime->month+1, pRtcTime->day, pRtcTime->hour, pRtcTime->min, pRtcTime->sec);
    localtime_1(&taget_time, t);
    localtime_2(&taget_time, t);
    localtime_3(&taget_time, t);

	//correct weekday
    if ( taget_time.weekday != pRtcTime->weekday ) {
		pRtcTime->weekday = taget_time.weekday; 
	}
	return pRtcTime->weekday;
}

static
unsigned char rtc_get_next_weekday(const SYNO_AUTO_POWERON* pAutoPowerOn, const unsigned char weekday)
{
    unsigned char u8Nextday = 0xFF;
    unsigned int mask = 1 << weekday;
    unsigned int weekdays = pAutoPowerOn->RtcAlarmPkt.weekdays & AUTO_POWERON_WEEKDAY_MASK;

    if( weekdays == 0 || mask == 0 ) { // no days set in weekday bitsmask
        goto End;
    }

    u8Nextday = weekday;
    weekdays |= weekdays << 7; // duplicate days into next week
    while( !(mask & weekdays) ) {
        mask <<= 1;
        u8Nextday++;
    }
    u8Nextday %= 7;

End:
    return u8Nextday;
}

static
unsigned char rtc_get_next_mday(const SYNORTCTIMEPKT *pRtcTime, const int offset)
{
    time_t t; 
    struct xtm taget_time;

    t = mktime(pRtcTime->year+1900, pRtcTime->month+1, pRtcTime->day, pRtcTime->hour, pRtcTime->min, pRtcTime->sec);
    t += 86400 * offset;
    localtime_1(&taget_time, t);
    localtime_2(&taget_time, t);
    localtime_3(&taget_time, t);

	return (unsigned char)taget_time.monthday;
}

static
int rtc_later_equal_than_int(unsigned char rtcHour, const unsigned char rtcMin,
                       unsigned char intHour, const unsigned char intMin)
{
	if( rtcHour > intHour || (rtcHour == intHour && rtcMin >= intMin) ) {
		return 1;
	}

	return 0;
}

static
int rtc_rotate_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn, const SYNORTCTIMEPKT *pRtcTime)
{
	int iRet = -1;
	unsigned char next_wday;
	unsigned char mday, hrs, min;
	unsigned char rtc_control, rtc_valid;

    if( NULL == pAutoPowerOn || NULL == pRtcTime ) {
		goto End;
	}

    next_wday = rtc_get_next_weekday(pAutoPowerOn, pRtcTime->weekday);
    if( next_wday == pRtcTime->weekday && rtc_later_equal_than_int(pRtcTime->hour,
                                                                   pRtcTime->min,
                                                                   pAutoPowerOn->RtcAlarmPkt.hour,
                                                                   pAutoPowerOn->RtcAlarmPkt.min) )
    {
        next_wday = rtc_get_next_weekday(pAutoPowerOn, (pRtcTime->weekday+1)%7);
    }

    if ( next_wday >= pRtcTime->weekday ) {
    	mday = BIN2BCD(rtc_get_next_mday(pRtcTime, next_wday - pRtcTime->weekday));
	} else {
    	mday = BIN2BCD(rtc_get_next_mday(pRtcTime, 7 - pRtcTime->weekday + next_wday));
	}
    hrs = (pAutoPowerOn->RtcAlarmPkt.hour < 24) ? BIN2BCD(pAutoPowerOn->RtcAlarmPkt.hour) : 0xff;
    min = (pAutoPowerOn->RtcAlarmPkt.min < 60) ? BIN2BCD(pAutoPowerOn->RtcAlarmPkt.min) : 0xff;

    spin_lock_irq(&rtc_lock);
    rtc_control = CMOS_READ(RTC_CONTROL);
    rtc_control &= ~RTC_AIE;
    CMOS_WRITE(rtc_control, RTC_CONTROL);
    rtc_valid = pAutoPowerOn->enabled ? ((CMOS_READ(RTC_VALID) & ~RTC_MDAY_ALARM_MASK) | mday) : 0;

    CMOS_WRITE(rtc_valid, RTC_VALID);
    CMOS_WRITE(hrs, RTC_HOURS_ALARM);
    CMOS_WRITE(min, RTC_MINUTES_ALARM);
    CMOS_WRITE(0, RTC_SECONDS_ALARM);
    CMOS_WRITE(pAutoPowerOn->RtcAlarmPkt.weekdays, RTC_FREE_ADDR1);

    if (pAutoPowerOn->enabled) {
        rtc_control |= RTC_AIE;
        CMOS_WRITE(rtc_control, RTC_CONTROL);
    }
    spin_unlock_irq(&rtc_lock);

	iRet = 0;
End:
	return iRet;
}

int rtc_bandon_get_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn)
{
    int iRet = -1;

    if( NULL == pAutoPowerOn || pAutoPowerOn->num != 1 ) {
        goto End;
    }

    spin_lock_irq(&rtc_lock);
    pAutoPowerOn->num = 1;
    pAutoPowerOn->enabled = CMOS_READ(RTC_CONTROL) & RTC_AIE ? 1 : 0;
    pAutoPowerOn->RtcAlarmPkt.min = BCD2BIN(CMOS_READ(RTC_MINUTES_ALARM));
    pAutoPowerOn->RtcAlarmPkt.hour = BCD2BIN(CMOS_READ(RTC_HOURS_ALARM));
    pAutoPowerOn->RtcAlarmPkt.weekdays = CMOS_READ(RTC_FREE_ADDR1);
    spin_unlock_irq(&rtc_lock);

    iRet = 0;
End:
    return iRet;
}

int rtc_bandon_set_auto_poweron(SYNO_AUTO_POWERON* pAutoPowerOn)
{
	int iRet = -1;
    SYNORTCTIMEPKT rtcTime;

	if( NULL == pAutoPowerOn || pAutoPowerOn->num != 1 ) {
        printk("Parameter Error.\n");
		goto End;
	}

    if( 0 > rtc_bandon_get_time(&rtcTime) ) {
        printk("Failed to get time from rtc.\n");
        goto End;
    }

	if ( 0 != rtc_rotate_auto_poweron(pAutoPowerOn, &rtcTime) ) {
		printk("Failed to set alarm data.\n");
		goto End;
	}

	iRet = 0;
End:
	return iRet;
}

int rtc_bandon_get_time(struct _SynoRtcTimePkt* pRtcTimePkt)
{
    unsigned long flags;
    unsigned char ctrl;
	SYNO_AUTO_POWERON schedule;

    /*
     * Only the values that we read from the RTC are set. We leave
     * tm_wday, tm_yday and tm_isdst untouched. Note that while the
     * RTC has RTC_DAY_OF_WEEK, we should usually ignore it, as it is
     * only updated by the RTC when initially set to a non-zero value.
     */
    spin_lock_irqsave(&rtc_lock, flags);
    pRtcTimePkt->sec = CMOS_READ(RTC_SECONDS);
    pRtcTimePkt->min = CMOS_READ(RTC_MINUTES);
    pRtcTimePkt->hour = CMOS_READ(RTC_HOURS);
    pRtcTimePkt->day = CMOS_READ(RTC_DAY_OF_MONTH);
    pRtcTimePkt->month = CMOS_READ(RTC_MONTH);
    pRtcTimePkt->weekday = CMOS_READ(RTC_DAY_OF_WEEK);
    pRtcTimePkt->year = CMOS_READ(RTC_YEAR);
    spin_unlock_irqrestore(&rtc_lock, flags);

	rtc_correct_wday(pRtcTimePkt);

    if (!(ctrl & RTC_DM_BINARY) || RTC_ALWAYS_BCD)
    {
        BCD_TO_BIN(pRtcTimePkt->sec);
        BCD_TO_BIN(pRtcTimePkt->min);
        BCD_TO_BIN(pRtcTimePkt->hour);
        BCD_TO_BIN(pRtcTimePkt->day);
        BCD_TO_BIN(pRtcTimePkt->month);
        BCD_TO_BIN(pRtcTimePkt->year);
    }

    /*
     * Account for differences between how the RTC uses the values
     * and how they are defined in a struct rtc_time;
     */
    if ((pRtcTimePkt->year += (epoch - 1900)) <= 69)
        pRtcTimePkt->year += 100;

    pRtcTimePkt->month--;

	schedule.num = 1;
	if( 0 == rtc_bandon_get_auto_poweron(&schedule) && schedule.enabled ) {
		rtc_rotate_auto_poweron(&schedule, pRtcTimePkt);
	}

	return 0;
}

int rtc_bandon_set_time(struct _SynoRtcTimePkt* pRtcTimePkt)
{
    unsigned char save_control, save_freq_select;
    unsigned char mon, day, hrs, min, sec, leap_yr, wday;
    unsigned int yrs;
    SYNO_AUTO_POWERON schedule;

    if (!capable(CAP_SYS_TIME)) {
       return -EACCES;
    }

    yrs = pRtcTimePkt->year + 1900;
    mon = pRtcTimePkt->month + 1;   /* tm_mon starts at zero */
    day = pRtcTimePkt->day;
    hrs = pRtcTimePkt->hour;
    min = pRtcTimePkt->min;
    sec = pRtcTimePkt->sec;
    wday = rtc_correct_wday(pRtcTimePkt);

    if (yrs < 1970)
        return -EINVAL;

    leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

    if ((mon > 12) || (day == 0))
        return -EINVAL;

    if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr)))
        return -EINVAL;

    if ((hrs >= 24) || (min >= 60) || (sec >= 60))
        return -EINVAL;

    if ((yrs -= epoch) > 255)    /* They are unsigned */
        return -EINVAL;

    spin_lock_irq(&rtc_lock);

    /* These limits and adjustments are independent of
     * whether the chip is in binary mode or not.
     */
    if (yrs > 169) {
        spin_unlock_irq(&rtc_lock);
        return -EINVAL;
    }
    if (yrs >= 100)
        yrs -= 100;

    if (!(CMOS_READ(RTC_CONTROL) & RTC_DM_BINARY)
        || RTC_ALWAYS_BCD) {
        BIN_TO_BCD(sec);
        BIN_TO_BCD(min);
        BIN_TO_BCD(hrs);
        BIN_TO_BCD(day);
        BIN_TO_BCD(mon);
        BIN_TO_BCD(yrs);
    }

    save_control = CMOS_READ(RTC_CONTROL);
    CMOS_WRITE((save_control|RTC_SET), RTC_CONTROL);
    save_freq_select = CMOS_READ(RTC_FREQ_SELECT);
    CMOS_WRITE((save_freq_select|RTC_DIV_RESET2), RTC_FREQ_SELECT);
    CMOS_WRITE(yrs, RTC_YEAR);
    CMOS_WRITE(mon, RTC_MONTH);
    CMOS_WRITE(day, RTC_DAY_OF_MONTH);
    CMOS_WRITE(wday, RTC_DAY_OF_WEEK);
    CMOS_WRITE(hrs, RTC_HOURS);
    CMOS_WRITE(min, RTC_MINUTES);
    CMOS_WRITE(sec, RTC_SECONDS);
    CMOS_WRITE(save_control, RTC_CONTROL);
    CMOS_WRITE(save_freq_select, RTC_FREQ_SELECT);
    spin_unlock_irq(&rtc_lock);

    schedule.num = 1;
    if( 0 == rtc_bandon_get_auto_poweron(&schedule) && schedule.enabled ) {
        rtc_rotate_auto_poweron(&schedule, pRtcTimePkt);
    }

	return 0;
}

int rtc_bandon_auto_poweron_init(void)
{
	return 0;
}

int rtc_bandon_auto_poweron_uninit(void)
{
	SYNORTCTIMEPKT rtcTime;
    rtc_bandon_get_time(&rtcTime);

	return 0;
}
