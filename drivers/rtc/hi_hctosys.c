/*
 * RTC subsystem, initialize system time on startup
 *
 * Copyright (C) 2005 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/rtc.h>
#include "hi_rtc.h"

/* IMPORTANT: the RTC only stores whole seconds. It is arbitrary
 * whether it stores the most close value or the value with partial
 * seconds truncated. However, it is important that we use it to store
 * the truncated value. This is because otherwise it is necessary,
 * in an rtc sync function, to read both xtime.tv_sec and
 * xtime.tv_nsec. On some processors (i.e. ARM), an atomic read
 * of >32bits is not possible. So storing the most close value would
 * slow down the sync API. So here we have the truncated value and
 * the best guess is to add 0.5s.
 */

static int __init hi_rtc_hctosys(void)
{
	int err = -ENODEV;
	struct rtc_time tm;
	struct timespec tv = {
		.tv_nsec = NSEC_PER_SEC >> 1,
	};
	rtc_time_t now;

	hirtc_get_time(&now);
	tm.tm_year = now.year - 1900;
	tm.tm_mon  = now.month - 1;
	tm.tm_mday = now.date;
	tm.tm_wday = now.weekday;
	tm.tm_hour = now.hour;
	tm.tm_min  = now.minute;
	tm.tm_sec  = now.second;

	err = rtc_valid_tm(&tm);
	if (err) {
		pr_err("hctosys: invalid date/time\n");
		goto end;
	}

	rtc_tm_to_time(&tm, &tv.tv_sec);

	err = do_settimeofday(&tv);

	pr_info("Setting system clock to "
		"%d-%02d-%02d %02d:%02d:%02d UTC (%u)\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		(unsigned int) tv.tv_sec);

end:
	return err;
}

late_initcall(hi_rtc_hctosys);
