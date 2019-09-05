/* 
 * drivers/rtc/rtc-c2k.c
 *
 * Copyright (c) 2010 Mindspeed Technologies Co., Ltd.
 *		http://www.mindspeed.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <linux/rtc/rtc-c2k.h>

/* Initial by default values */
#define	YEAR		112 /* 2012 */ 
#define	MONTH		6 /* 6 */
#define	DATE		12 /* 18 */ 
#define	DAY		3 //0:SUNDAY 1:MONDAY 2:TUESDAY 3:WEDNESDAY 4:THURSDAY 5:FRIDAY 6:SATURDAY
#define	HOUR		18 //24 hours time format
#define	MIN		25
#define	SEC		1
/*End*/

#define	C2K_RTC_RTCALM_SECEN	0x81
#define	C2K_RTC_RTCALM_MINEN	0x82
#define	C2K_RTC_RTCALM_HOUREN	0x84
#define	C2K_RTC_RTCALM_DATEEN	0x88
#define	C2K_RTC_RTCALM_DAYEN	0x90
#define	C2K_RTC_RTCALM_MONEN	0xA0
#define	C2K_RTC_RTCALM_YEAREN	0xC0
#define	C2K_RTC_RTCALM_GLOBALEN	0x80

#define	C2K_RTC_RTCALM_ALMEN	0xff
#define	DISABLE_ALL_ALAM	0x0

#define	C2K_RTCALM_ALMEN	(0x3<<0)

static struct resource *c2k_rtc_mem;
static volatile char __iomem *c2k_rtc_base;
static int rtc_alarmno;

#define writel_rtc(v,a)		(*(int *)(a) = (v))
#define readl_rtc(a)		(*(int *)(a))

void rtc_reg_write(unsigned int WrData, volatile char *WrAd)
{ 
	unsigned int WrAddr = (unsigned int)WrAd;
	unsigned int WriteData;

	WriteData = (0x0000FFFF & WrData);

	/* The core APB runs at 250Hz ,where as the RTC APB runs at 50MHz.
	 * So the delay need to be inserted between two APB requests, other
	 * wise transaction gets dropped.
	 * Each Core time unit = 4000ps , RTC time unit = 20000ps. 5 times 
	 * core time unit = RTC time unit. 
	 */

	writel_rtc(WriteData, WrAddr);

	udelay(5);

	return;
}

int rtc_reg_read(volatile char *RdAd)
{ 
	unsigned int *RdAddr = (unsigned int*)RdAd; 
	unsigned int WriteData,ReadData;

	WriteData = 0x00010000;

	/* The core APB runs at 250Hz ,where as the RTC APB runs at 50MHz.
	 * So the delay need to be inserted between two APB requests, other
	 * wise transaction gets dropped.
	 * Each Core time unit = 4000ps , RTC time unit = 20000ps. 5 times 
	 * core time unit = RTC time unit.
	 */

	writel_rtc(WriteData, RdAddr);

	/* The core APB runs at 250Hz ,where as the RTC APB runs at 50MHz.
	 * So the delay need to be inserted between two APB requests, other
	 * wise transaction gets dropped.
	 * Each Core time unit = 4000ps , RTC time unit = 20000ps. 5 times 
	 * core time unit = RTC time unit. 
	 */

	udelay(5);

	ReadData = readl_rtc(RdAddr);

	while ( (0x00010000 & ReadData) != 0x10000)
	{
		udelay(5);
		ReadData = readl_rtc(RdAddr);
	}

	return((0x0000FFFF & ReadData));    
}

void dbg_rtc_time(struct rtc_time *rtc_tm)
{
	printk ("\n%s: \
			\n\trtc_tm->tm_sec=%d \
			\n\trtc_tm->tm_min=%d \
			\n\trtc_tm->tm_hour=%d \
			\n\trtc_tm->tm_mday=%d \
			\n\trtc_tm->tm_wday=%d \
			\n\trtc_tm->tm_mon=%d \
			\n\trtc_tm->tm_year=%d \n\n", __func__, \
			rtc_tm->tm_sec, rtc_tm->tm_min, rtc_tm->tm_hour, rtc_tm->tm_mday,\
			rtc_tm->tm_wday, rtc_tm->tm_mon, rtc_tm->tm_year);
}

/* IRQ Handlers */

static irqreturn_t c2k_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	pr_debug ("%s: irq=%d: Alaram Rang ..............!!!\n \
			\n\t*(0x%x)=0x%x \
			\n\t*(0x%x)=0x%x \n", __func__, irq, \
			(unsigned int)(c2k_rtc_base + C2K_RTC_RTCALM), rtc_reg_read(c2k_rtc_base + C2K_RTC_RTCALM),\
			(unsigned int)(c2k_rtc_base + C2K_RTC_RTCIM), rtc_reg_read(c2k_rtc_base + C2K_RTC_RTCIM));

	rtc_update_irq(rdev, 1, RTC_AF | RTC_IRQF);
	/*-------------------------
	 * DISBABLING alarm
	 *------------------------
	 */
	rtc_reg_write(DISABLE_ALL_ALAM, c2k_rtc_base + C2K_RTC_RTCALM);

	/*-------------------------
	 * DISBABLING the alarm interrupt
	 *------------------------
	 */
	rtc_reg_write(DISABLE_ALL_ALAM, c2k_rtc_base + C2K_RTC_RTCIM);

	/*-------------------------
	 * DISBABLING the alarm PENDING bit
	 *------------------------
	 */
	rtc_reg_write(DISABLE_ALL_ALAM, c2k_rtc_base + C2K_RTC_RTCPEND);

	return IRQ_HANDLED;
}

/* Time read/write */
static int c2k_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	rtc_tm->tm_year = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDYEAR);
	rtc_tm->tm_mon = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDMON);
	rtc_tm->tm_mday = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDDATE);
	rtc_tm->tm_wday = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDDAY);
	rtc_tm->tm_hour = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDHOUR);
	rtc_tm->tm_min  = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDMIN);
	rtc_tm->tm_sec  = rtc_reg_read(c2k_rtc_base + C2K_RTC_BCDSEC);

	pr_debug("%s: BCD:\n \
			\n\tyear.mon.date.day hr:min:sec\n \
			\n\t0x%x.0x%x.0x%x.0x%x 0x%x:0x%x:0x%x\n",__func__,\
			rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,rtc_tm->tm_wday,\
			rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	rtc_tm->tm_sec = bcd2bin(rtc_tm->tm_sec);
	rtc_tm->tm_min = bcd2bin(rtc_tm->tm_min);
	rtc_tm->tm_hour = bcd2bin(rtc_tm->tm_hour);
	rtc_tm->tm_mday = bcd2bin(rtc_tm->tm_mday);
	rtc_tm->tm_wday = bcd2bin(rtc_tm->tm_wday);
	rtc_tm->tm_mon = bcd2bin(rtc_tm->tm_mon);
	rtc_tm->tm_year = bcd2bin(rtc_tm->tm_year);

	rtc_tm->tm_year += 100;

	rtc_tm->tm_mon -= 1;

	return rtc_valid_tm(rtc_tm);
}

#define	C2K_RTC_RTCCON_STARTB	(0x1<<0) /* RTC Halt */
#define	C2K_RTC_RTCCON_RTCEN	(0x1<<1) /* RTC Write Enable */
#define	C2K_RTC_RTCCON_CLKRST	(0x1<<2) /* RTC RESET */

static int c2k_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	int year = tm->tm_year - 100;

	pr_debug("%s: Will set:\
			\n%04d.%02d.%02d %02d:%02d:%02d\n",__func__,\
			tm->tm_year+1900, tm->tm_mon, tm->tm_mday, \
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	if (year < 0 || year >= 100) {
		dev_err(dev, "rtc only supports 100 years\n");
		return -EINVAL;
	}

	rtc_reg_write((C2K_RTC_RTCCON_STARTB | C2K_RTC_RTCCON_RTCEN | C2K_RTC_RTCCON_CLKRST), \
			c2k_rtc_base + C2K_RTC_RTCCON);

	rtc_reg_write(bin2bcd(tm->tm_min), c2k_rtc_base + C2K_RTC_BCDMIN);
	rtc_reg_write(bin2bcd(tm->tm_hour), c2k_rtc_base + C2K_RTC_BCDHOUR);
	rtc_reg_write(bin2bcd(tm->tm_mday), c2k_rtc_base + C2K_RTC_BCDDATE);
	rtc_reg_write(bin2bcd(tm->tm_wday), c2k_rtc_base + C2K_RTC_BCDDAY);
	rtc_reg_write(bin2bcd(tm->tm_mon+1), c2k_rtc_base + C2K_RTC_BCDMON);
	rtc_reg_write(bin2bcd(year), c2k_rtc_base + C2K_RTC_BCDYEAR);
	rtc_reg_write(bin2bcd(tm->tm_sec), c2k_rtc_base + C2K_RTC_BCDSEC);

	rtc_reg_write((C2K_RTC_RTCCON_RTCEN | C2K_RTC_RTCCON_CLKRST), c2k_rtc_base + C2K_RTC_RTCCON);

	return 0;
}

static int c2k_rtc_setaie(struct device *dev, unsigned int enabled)
{
	unsigned int tmp;

	pr_debug ("%s: aie=%d\n", __func__, enabled);

	tmp = rtc_reg_read(c2k_rtc_base + C2K_RTC_RTCIM) & ~C2K_RTCALM_ALMEN;

	if (enabled)
		tmp |= C2K_RTCALM_ALMEN;

	rtc_reg_write(tmp, c2k_rtc_base + C2K_RTC_RTCIM);

	return 0;
}

static int c2k_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;
	unsigned int alm_en;

	alm_tm->tm_sec  = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMSEC);
	alm_tm->tm_min  = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMMIN);
	alm_tm->tm_hour = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMHOUR);
	alm_tm->tm_mon  = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMMON);
	alm_tm->tm_mday = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMDATE);
	alm_tm->tm_wday = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMDAY);
	alm_tm->tm_year = rtc_reg_read(c2k_rtc_base + C2K_RTC_ALMYEAR);

	alm_en = rtc_reg_read(c2k_rtc_base + C2K_RTC_RTCALM);

	alrm->enabled = (alm_en & C2K_RTC_RTCALM_ALMEN) ? 1 : 0;

	pr_debug("%s: alm_en=%d, %04d.%02d.%02d.%02d  %02d:%02d:%02d\n",__func__,
			alm_en,
			1900 + alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
			alm_tm->tm_wday, alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);

	/* decode the alarm enable field */

	if (alm_en & C2K_RTC_RTCALM_SECEN)
		alm_tm->tm_sec = bcd2bin(alm_tm->tm_sec);
	else
		alm_tm->tm_sec = -1;

	if (alm_en & C2K_RTC_RTCALM_MINEN)
		alm_tm->tm_min = bcd2bin(alm_tm->tm_min);
	else
		alm_tm->tm_min = -1;

	if (alm_en & C2K_RTC_RTCALM_HOUREN)
		alm_tm->tm_hour = bcd2bin(alm_tm->tm_hour);
	else
		alm_tm->tm_hour = -1;

	if (alm_en & C2K_RTC_RTCALM_DAYEN)
		alm_tm->tm_mday = bcd2bin(alm_tm->tm_mday);
	else
		alm_tm->tm_mday = -1;

	if (alm_en & C2K_RTC_RTCALM_MONEN) {
		alm_tm->tm_mon = bcd2bin(alm_tm->tm_mon);
		alm_tm->tm_mon -= 1;
	} else {
		alm_tm->tm_mon = -1;
	}

	if (alm_en & C2K_RTC_RTCALM_YEAREN){
		alm_tm->tm_year = bcd2bin(alm_tm->tm_year);
		alm_tm->tm_year += 100;
	}
	else
		alm_tm->tm_year = -1;

	return 0;
}

static int c2k_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;

	pr_debug("%s: %d, %04d.%02d.%02d %02d:%02d:%02d\n",__func__,
			alrm->enabled,
			1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	rtc_reg_write(0x3, (c2k_rtc_base + C2K_RTC_RTCIM)); //enable alarm interrupt mode
	rtc_reg_write(0xff, (c2k_rtc_base + C2K_RTC_RTCALM)); //enable all alarms.

	if (tm->tm_sec < 60 && tm->tm_sec >= 0) {
		rtc_reg_write(bin2bcd(tm->tm_sec), c2k_rtc_base + C2K_RTC_ALMSEC);
	}

	if (tm->tm_min < 60 && tm->tm_min >= 0) {
		rtc_reg_write(bin2bcd(tm->tm_min), c2k_rtc_base + C2K_RTC_ALMMIN);
	}

	if (tm->tm_hour < 24 && tm->tm_hour >= 0) {
		rtc_reg_write(bin2bcd(tm->tm_hour), c2k_rtc_base + C2K_RTC_ALMHOUR);
	}

	if (tm->tm_mday < 32 && tm->tm_mday >= 1) {
		rtc_reg_write(bin2bcd(tm->tm_mday), c2k_rtc_base + C2K_RTC_ALMDATE);
	}

	if (tm->tm_wday < 7 && tm->tm_wday >= 0) {
		rtc_reg_write(bin2bcd(tm->tm_wday), c2k_rtc_base + C2K_RTC_ALMDAY);
	}

	if (tm->tm_mon < 12 && tm->tm_mon >= 0) {
		rtc_reg_write(bin2bcd(tm->tm_mon+1), c2k_rtc_base + C2K_RTC_ALMMON);
	}

	if (tm->tm_year >= 1) {
		int year = tm->tm_year - 100;

		if (year < 0 || year >= 100) {
			dev_err(dev, "rtc only supports 100 years\n");
			return -EINVAL;
		}

		rtc_reg_write(bin2bcd(year), c2k_rtc_base + C2K_RTC_ALMYEAR);
	}

	c2k_rtc_setaie(dev, alrm->enabled);

	return 0;
}

static const struct rtc_class_ops c2k_rtcops = {
	.read_time	= c2k_rtc_gettime,
	.set_time	= c2k_rtc_settime,
	.read_alarm	= c2k_rtc_getalarm,
	.set_alarm	= c2k_rtc_setalarm,
	.alarm_irq_enable = c2k_rtc_setaie,
};

//static int __devexit c2k_rtc_remove(struct platform_device *dev)
static int c2k_rtc_remove(struct platform_device *dev)
{
	struct rtc_device *rtc = platform_get_drvdata(dev);

	free_irq(rtc_alarmno, rtc);

	platform_set_drvdata(dev, NULL);
	rtc_device_unregister(rtc);

	c2k_rtc_setaie(&dev->dev, 0);

	iounmap(c2k_rtc_base);
	release_resource(c2k_rtc_mem);
	kfree(c2k_rtc_mem);

	return 0;
}

static char __initdata banner[] = "C2000 RTC, (c) 2012 Mindspeed Technologies\n";

//static int __devinit c2k_rtc_probe(struct platform_device *pdev)
static int c2k_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct rtc_time rtc_tm;
	struct resource *res;
	int ret;

	rtc_alarmno = platform_get_irq(pdev, 0);
	if (rtc_alarmno < 0) {
		dev_err(&pdev->dev, "no irq for alarm\n");
		return -ENOENT;
	}

	/* get the memory region */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		return -ENOENT;
	}

	pr_debug ("%s: alarm irq=%d res->start=0x%x res->end=0x%x\n", __func__, \
			rtc_alarmno, res->start, res->end);

	c2k_rtc_mem = request_mem_region(res->start, resource_size(res),
			pdev->name);
	if (c2k_rtc_mem == NULL) {
		printk("%s: failed to reserve memory region\n", __func__);
		ret = -ENOENT;
		goto err_nores;
	}

	c2k_rtc_base = ioremap(res->start, resource_size(res));
	if (c2k_rtc_base == NULL) {
		printk ("%s: failed ioremap()\n", __func__);
		ret = -EINVAL;
		goto err_nomap;
	}

	device_init_wakeup(&pdev->dev, 1);

	/* register RTC and exit */

	rtc = rtc_device_register("c2k", &pdev->dev, &c2k_rtcops,
			THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_nortc;
	} else
		printk(banner);

	/* Check RTC Time */
	c2k_rtc_gettime(NULL, &rtc_tm);

	if (rtc_valid_tm(&rtc_tm)) {
		rtc_tm.tm_year	= YEAR;
		rtc_tm.tm_mon	= MONTH;
		rtc_tm.tm_mday	= DATE;
		rtc_tm.tm_wday	= DAY;
		rtc_tm.tm_hour	= HOUR;
		rtc_tm.tm_min	= MIN;
		rtc_tm.tm_sec	= SEC;

		c2k_rtc_settime(NULL, &rtc_tm);

		dev_warn(&pdev->dev, "Warning: Invalid RTC value so initializing it\n");
	}

	platform_set_drvdata(pdev, rtc);

	ret = request_irq(rtc_alarmno, c2k_rtc_alarmirq,
			IRQF_DISABLED,  "rtc-alarm", rtc);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", rtc_alarmno, ret);
		goto err_alarm_irq;
	}

	return 0;

err_alarm_irq:
	platform_set_drvdata(pdev, NULL);
	rtc_device_unregister(rtc);

err_nortc:
	iounmap(c2k_rtc_base);

err_nomap:
	release_resource(c2k_rtc_mem);

err_nores:
	return ret;
}

#ifdef CONFIG_PM

/* RTC Power management control */
static int c2k_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int c2k_rtc_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define c2k_rtc_suspend NULL
#define c2k_rtc_resume  NULL
#endif

static struct platform_driver c2k_rtc_driver = {
	.probe		= c2k_rtc_probe,
	//.remove		= __devexit_p(c2k_rtc_remove),
	.remove		= c2k_rtc_remove,
	.suspend	= c2k_rtc_suspend,
	.resume		= c2k_rtc_resume,
	.driver		= {
		.name	= "c2k-rtc",
		.owner	= THIS_MODULE,
	},
};

static int __init c2k_rtc_init(void)
{
	return platform_driver_register(&c2k_rtc_driver);
}

static void __exit c2k_rtc_exit(void)
{
	platform_driver_unregister(&c2k_rtc_driver);
}

module_init(c2k_rtc_init);
module_exit(c2k_rtc_exit);

MODULE_DESCRIPTION("Mindspeed RTC Driver");
MODULE_AUTHOR("Satendra Pratap <satendra.pratap@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:c2k-rtc");
