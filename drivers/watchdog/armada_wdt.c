/*
 * drivers/watchdog/armada_wdt.c
 *
 * Watchdog driver for Marvell Armada processor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/rcupdate.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#ifdef CONFIG_ARCH_ARMADA_XP
#include <mach/armadaxp.h>
#endif
#ifdef CONFIG_ARCH_ARMADA370
#include <mach/armada370.h>
#endif
#include <plat/armada_wdt.h>

/*
 * Watchdog timer block registers.
 */
#define	MV_CNTMR_REGS		(armada_timers_base)
#define	MV_MBUS_REGS		(mbus_regs)

#define	TIMER_CTRL		(MV_CNTMR_REGS + 0x0000)
#define	 GLOBAL_WD_EN		(1 << 8)
#define	 GLOBAL_WD_25MHZ_EN	(1 << 10)
#define	 GLOBAL_WD_16_RATIO	(4 << 16)

#define	GLOBAL_WD_RELOAD	(MV_CNTMR_REGS + 0x0030)
#define	GLOBAL_WD_VAL		(MV_CNTMR_REGS + 0x0034)

#define	BRIDGE_CAUSE		(MV_MBUS_REGS + 0x0260)
#define	BRIDGE_INT_GLOBAL_WDT	(1 << 26)

#define	WD_RSTOUTn_MASK		(MV_CNTMR_REGS + 0x0404)
#define	 GLOBAL_WD_RSTOUTn_EN	0x100

#define	WDT_MAX_CYCLE_COUNT	0xffffffff
#define	WDT_IN_USE		0
#define	WDT_OK_TO_CLOSE		1

static unsigned int wdt_max_duration;	/* (max amount of seconds for wdt) */
static unsigned int wdt_tclk;
static unsigned long wdt_status;
static unsigned int armada_timers_base;
static unsigned int mbus_regs;
static unsigned int global_wd_freq_en;
static DEFINE_SPINLOCK(wdt_lock);

#define TIMER_MARGIN	60		/* Default is 60 seconds */
static int heartbeat = TIMER_MARGIN;	/* module parameter (seconds) */
module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat, "Armada Watchdog - initial heartbeat in seconds");

static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Armada Watchdog cannot be stopped "
	"once started (default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static void armada_wdt_ping(void)
{

	spin_lock(&wdt_lock);

	/* Reload watchdog counter */
	writel(wdt_tclk * heartbeat, GLOBAL_WD_VAL);

	spin_unlock(&wdt_lock);
}

static void armada_wdt_enable(void)
{
	u32 reg;

	spin_lock(&wdt_lock);

	/* Set watchdog duration */
	writel(wdt_tclk * heartbeat, GLOBAL_WD_VAL);

	/* Clear watchdog timer interrupt */
	reg = readl(BRIDGE_CAUSE);
	reg &= ~BRIDGE_INT_GLOBAL_WDT;
	writel(reg, BRIDGE_CAUSE);

	/* Enable reset on watchdog */
	reg = readl(WD_RSTOUTn_MASK);
	reg |= GLOBAL_WD_RSTOUTn_EN;
	writel(reg, WD_RSTOUTn_MASK);

	/* Enable watchdog timer */
	reg = readl(TIMER_CTRL);
	reg |= (GLOBAL_WD_EN | global_wd_freq_en);
	writel(reg, TIMER_CTRL);

	spin_unlock(&wdt_lock);
}

static void armada_wdt_disable(void)
{
	u32 reg;

	spin_lock(&wdt_lock);

	/* Disable watchdog timer */
	reg = readl(TIMER_CTRL);
	reg &= ~GLOBAL_WD_EN;
	writel(reg, TIMER_CTRL);

	/* Disable reset on watchdog */
	reg = readl(WD_RSTOUTn_MASK);
	reg &= ~GLOBAL_WD_RSTOUTn_EN;
	writel(reg, WD_RSTOUTn_MASK);

	/* Clear watchdog timer interrupt */
	reg = readl(BRIDGE_CAUSE);
	reg &= ~BRIDGE_INT_GLOBAL_WDT;
	writel(reg, BRIDGE_CAUSE);

	spin_unlock(&wdt_lock);
}

static int armada_wdt_get_timeleft(int *time_left)
{

	spin_lock(&wdt_lock);
	*time_left = readl(GLOBAL_WD_VAL) / wdt_tclk;
	spin_unlock(&wdt_lock);
	return 0;
}

static int armada_wdt_open(struct inode *inode, struct file *file)
{

	if (test_and_set_bit(WDT_IN_USE, &wdt_status))
		return -EBUSY;
	clear_bit(WDT_OK_TO_CLOSE, &wdt_status);
	armada_wdt_enable();
	return nonseekable_open(inode, file);
}

static ssize_t armada_wdt_write(struct file *file, const char *data,
	size_t len, loff_t *ppos)
{

	if (len) {
		if (!nowayout) {
			size_t i;

			clear_bit(WDT_OK_TO_CLOSE, &wdt_status);
			for (i = 0; i != len; i++) {
				char c;

				if (get_user(c, data + i))
					return -EFAULT;
				if (c == 'V')
					set_bit(WDT_OK_TO_CLOSE, &wdt_status);
			}
		}
		armada_wdt_ping();
	}
	return len;
}

static int armada_wdt_settimeout(int new_time)
{

	if ((new_time <= 0) || (new_time > wdt_max_duration))
		return -EINVAL;

	/*
	 * Set new watchdog time to be used when
	 * armada_wdt_enable() or armada_wdt_ping() is called.
	 */
	heartbeat = new_time;
	return 0;
}

static const struct watchdog_info ident = {
	.options	= WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT |
	    WDIOF_KEEPALIVEPING,
	.identity	= "Armada Watchdog",
};

static long armada_wdt_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = -ENOTTY;
	int time;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		ret = copy_to_user((struct watchdog_info *)arg, &ident,
		    sizeof(ident)) ? -EFAULT : 0;
		break;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		ret = put_user(0, (int *)arg);
		break;

	case WDIOC_KEEPALIVE:
		armada_wdt_ping();
		ret = 0;
		break;

	case WDIOC_SETTIMEOUT:
		ret = get_user(time, (int *)arg);
		if (ret)
			break;

		if (armada_wdt_settimeout(time)) {
			ret = -EINVAL;
			break;
		}
		armada_wdt_ping();

	case WDIOC_GETTIMEOUT:
		ret = put_user(heartbeat, (int *)arg);
		break;

	case WDIOC_GETTIMELEFT:
		if (armada_wdt_get_timeleft(&time)) {
			ret = -EINVAL;
			break;
		}
		ret = put_user(time, (int *)arg);
		break;
	}
	return ret;
}

static int armada_wdt_release(struct inode *inode, struct file *file)
{

	if (test_bit(WDT_OK_TO_CLOSE, &wdt_status))
		armada_wdt_disable();
	else
		printk(KERN_CRIT "Armada Watchdog: Device closed unexpectedly - "
		    "timer will not stop\n");
	clear_bit(WDT_IN_USE, &wdt_status);
	clear_bit(WDT_OK_TO_CLOSE, &wdt_status);

	return 0;
}

/*
 *	Notifier for system down
 */
static int armada_wdt_notify_sys(struct notifier_block *this, unsigned long code,
	void *unused)
{

	if (code == SYS_DOWN || code == SYS_HALT)
		if (!nowayout)
			armada_wdt_disable();

	return NOTIFY_DONE;
}

static const struct file_operations armada_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= armada_wdt_write,
	.unlocked_ioctl	= armada_wdt_ioctl,
	.open		= armada_wdt_open,
	.release	= armada_wdt_release,
};

static struct miscdevice armada_wdt_miscdev = {
	.minor		= WATCHDOG_MINOR,
	.name		= "watchdog",
	.fops		= &armada_wdt_fops,
};

static struct notifier_block armada_wdt_notifier = {
	.notifier_call	= armada_wdt_notify_sys,
};

static char banner[] __devinitdata = KERN_INFO "Armada Watchdog Timer "
	"initialized. (nowayout = %d)\n";

static int __devinit armada_wdt_probe(struct platform_device *pdev)
{
	struct armada_wdt_platform_data *pdata = pdev->dev.platform_data;
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "Armada Watchdog - missing platform resources\n");
		return -EINVAL;
	}
	armada_timers_base = res->start;

	if (pdata) {
		wdt_tclk = pdata->tclk;
		mbus_regs = pdata->mbus_regs;
	} else {
		printk(KERN_ERR "Armada Watchdog - missing platform data\n");
		return -ENODEV;
	}

	/* Check WD timer frequency - set ratio if higher than 25MHz */
	if (wdt_tclk == 25000000)
		global_wd_freq_en = GLOBAL_WD_25MHZ_EN;
	else {
		wdt_tclk = wdt_tclk / 16;
		global_wd_freq_en = GLOBAL_WD_16_RATIO;
	}

	if (armada_wdt_miscdev.parent)
		return -EBUSY;
	armada_wdt_miscdev.parent = &pdev->dev;

	wdt_max_duration = WDT_MAX_CYCLE_COUNT / wdt_tclk;
	if (armada_wdt_settimeout(heartbeat))
		heartbeat = wdt_max_duration;

	ret = register_reboot_notifier(&armada_wdt_notifier);
	if (ret) {
		printk(KERN_ERR "Armada Watchdog - cannot register "
		    "reboot notifier\n");
		return ret;
	}
	ret = misc_register(&armada_wdt_miscdev);
	if (ret) {
		printk(KERN_ERR "Armada Watchdog - cannot register miscdev "
		    "on minor=%d\n", WATCHDOG_MINOR);
		unregister_reboot_notifier(&armada_wdt_notifier);
		return ret;
	}

	printk(banner, nowayout);
	return 0;
}

static int __devexit armada_wdt_remove(struct platform_device *pdev)
{
	int ret;

	if (test_bit(WDT_IN_USE, &wdt_status)) {
		armada_wdt_disable();
		clear_bit(WDT_IN_USE, &wdt_status);
	}

	ret = misc_deregister(&armada_wdt_miscdev);
	if (!ret)
		armada_wdt_miscdev.parent = NULL;

	return ret;
}

static void armada_wdt_shutdown(struct platform_device *pdev)
{
	if (test_bit(WDT_IN_USE, &wdt_status))
		armada_wdt_disable();
}

static struct platform_driver armada_wdt_driver = {
	.probe		= armada_wdt_probe,
	.remove		= __devexit_p(armada_wdt_remove),
	.shutdown	= armada_wdt_shutdown,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "armada_wdt",
	},
};

static int __init armada_wdt_init(void)
{

	return platform_driver_register(&armada_wdt_driver);
}

static void __exit armada_wdt_exit(void)
{

	platform_driver_unregister(&armada_wdt_driver);
}

module_init(armada_wdt_init);
module_exit(armada_wdt_exit);

MODULE_DESCRIPTION("Armada Processor Watchdog");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
