/*
 *  drivers/watchdog/ls1024a_wdt.c
 *
 *  Copyright (C) 2004,2005,2013 Mindspeed Technologies, Inc.
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

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>

#define LS1024A_TIMER_WDT_HIGH_BOUND			0x0
#define LS1024A_TIMER_WDT_CONTROL			0x4
#define  LS1024A_TIMER_WDT_CONTROL_TIMER_ENABLE		BIT(0)
#define LS1024A_TIMER_WDT_CURRENT_COUNT			0x8

#define LS1024A_DEVICE_RST_CNTRL			0x0
#define  LS1024A_WD_STATUS_CLR				BIT(6)
#define  LS1024A_AXI_WD_RST_EN				BIT(5)
#define LS1024A_GNRL_DEVICE_STATUS			0x18
#define  LS1024A_AXI_WD_RST_ACTIVATED			BIT(0)

#define WDT_NAME					"comcerto_wdt"

/* these are the actual wdt limits */
#define WDT_DEFAULT_TIMEOUT				5
#define WDT_MAX_TIMEOUT					(0xffffffff / LS1024A_AHBCLK)

/* these are for the virtual wdt */
#define WDT_DEFAULT_TIME				70	/* seconds */
#define WDT_MAX_TIME					255	/* seconds */

static unsigned long LS1024A_AHBCLK;
static int wd_heartbeat = WDT_DEFAULT_TIMEOUT;
static int wd_time = WDT_DEFAULT_TIME;
static int nowayout = WATCHDOG_NOWAYOUT;
static struct clk *axi_clk;
static struct regmap *clk_rst_map;
static void __iomem *ls1024a_wdt_base;

module_param(wd_heartbeat, int, 0);
MODULE_PARM_DESC(wd_heartbeat, "Watchdog heartbeat in seconds. (default="__MODULE_STRING(WDT_DEFAULT_TIMEOUT) ")");

module_param(wd_time, int, 0);
MODULE_PARM_DESC(wd_time, "Watchdog time in seconds. (default="__MODULE_STRING(WDT_DEFAULT_TIME) ")");

#ifdef CONFIG_WATCHDOG_NOWAYOUT
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");
#endif

static struct timer_list wdt_timer;

static unsigned long ls1024a_wdt_busy;
static char expect_close;
static spinlock_t wdt_lock;
static atomic_t ticks;

/*
 * Inform whether the boot was caused by AXI watchdog or not.
 * If the boot was caused by AXI WDT, the WD status is cleared from
 * reset control register.
 */
static int ls1024a_wdt_rst_status(void)
{
	unsigned long flags;
	u32 val;
	int ret = 0;

	spin_lock_irqsave(&wdt_lock, flags);

	regmap_read(clk_rst_map, LS1024A_GNRL_DEVICE_STATUS, &val);

	if (val & LS1024A_AXI_WD_RST_ACTIVATED) {
		regmap_update_bits(clk_rst_map, LS1024A_DEVICE_RST_CNTRL,
				   LS1024A_WD_STATUS_CLR,
				   LS1024A_WD_STATUS_CLR);
		ret = 1;
	}

	spin_unlock_irqrestore(&wdt_lock, flags);

	return ret;
}

/*
 * Set a new heartbeat value for the watchdog device. If the heartbeat value is
 * incorrect we keep the old value and return -EINVAL. If successfull we return 0.
 */
static int ls1024a_wdt_set_heartbeat(int t)
{
	if (t < 1 || t > WDT_MAX_TIMEOUT)
		return -EINVAL;

	wd_heartbeat = t;
	return 0;
}

/*
 * Write wd_heartbeat to high bound register.
 */
static void ls1024a_wdt_pet_watchdog_physical(void)
{
	__raw_writel(wd_heartbeat * LS1024A_AHBCLK,
		     ls1024a_wdt_base + LS1024A_TIMER_WDT_HIGH_BOUND);
}

/*
 * reset virtual wdt timer
 */
static void ls1024a_wdt_pet_watchdog_virtual(void)
{
	atomic_set(&ticks, wd_time);
}

/*
 * set virtual wd timeout reset value
 */
static int ls1024a_wdt_settimeout(int new_time)
{
	if ((new_time <= 0) || (new_time > WDT_MAX_TIME))
		return -EINVAL;

	wd_time = new_time;
	return 0;
}

/*
 * implement virtual wdt on physical wdt with timer
 */
static void ls1024a_timer_tick(unsigned long unused)
{
	if (!atomic_dec_and_test(&ticks)) {
		ls1024a_wdt_pet_watchdog_physical();
		mod_timer(&wdt_timer, jiffies + HZ);
	} else {
		printk(KERN_CRIT "%s: Watchdog will fire soon!!!\n", WDT_NAME);
	}
}

/*
 * Disable the watchdog.
 */
static void ls1024a_wdt_stop(void)
{
	unsigned long flags;
	u32 wdt_control;

	spin_lock_irqsave(&wdt_lock, flags);

	del_timer(&wdt_timer);

	wdt_control = __raw_readl(ls1024a_wdt_base + LS1024A_TIMER_WDT_CONTROL);
	wdt_control &= ~LS1024A_TIMER_WDT_CONTROL_TIMER_ENABLE;
	__raw_writel(wdt_control, ls1024a_wdt_base + LS1024A_TIMER_WDT_CONTROL);

	spin_unlock_irqrestore(&wdt_lock, flags);

	ls1024a_wdt_pet_watchdog_physical();
}

/*
 * Enable the watchdog.
 */
static void ls1024a_wdt_start(void)
{
	unsigned long flags;
	u32 wdt_control;

	spin_lock_irqsave(&wdt_lock, flags);

	wdt_control = __raw_readl(ls1024a_wdt_base + LS1024A_TIMER_WDT_CONTROL);
	wdt_control |= LS1024A_TIMER_WDT_CONTROL_TIMER_ENABLE;
	__raw_writel(wdt_control, ls1024a_wdt_base + LS1024A_TIMER_WDT_CONTROL);

	regmap_update_bits(clk_rst_map, LS1024A_DEVICE_RST_CNTRL,
			   LS1024A_AXI_WD_RST_EN, LS1024A_AXI_WD_RST_EN);

	mod_timer(&wdt_timer, jiffies + HZ);

	spin_unlock_irqrestore(&wdt_lock, flags);
}

/*
 * Disable WDT and:
 * - set max. possible timeout to avoid reset, it can occur
 * since current counter value could be bigger then
 * high bound one at the moment
 * Function is called once at start (while configuration),
 * and it's safe not to disable/enable IRQs.
 */
static void ls1024a_wdt_config(void)
{
	ls1024a_wdt_stop();

	__raw_writel(~0, ls1024a_wdt_base + LS1024A_TIMER_WDT_HIGH_BOUND);	/* write max timeout */
}

/*
 * Watchdog device is opened, and watchdog starts running.
 */
static int ls1024a_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &ls1024a_wdt_busy))
		return -EBUSY;

	ls1024a_wdt_pet_watchdog_virtual();
	ls1024a_wdt_pet_watchdog_physical();
	ls1024a_wdt_start();

	return nonseekable_open(inode, file);
}

/*
 * Release the watchdog device.
 * If CONFIG_WATCHDOG_NOWAYOUT is NOT defined and expect_close == 42
 * i.e. magic char 'V' has been passed while write() then the watchdog
 * is also disabled.
 */
static int ls1024a_wdt_release(struct inode *inode, struct file *file)
{
	if (expect_close == 42) {
		ls1024a_wdt_stop();	/* disable the watchdog when file is closed */
		clear_bit(0, &ls1024a_wdt_busy);
	} else {
		printk(KERN_CRIT "%s: closed unexpectedly. WDT will not stop!\n", WDT_NAME);
	}

	expect_close = 0;
	return 0;
}

/*
 * Handle commands from user-space.
 */
static long ls1024a_wdt_ioctl(struct file *file, uint cmd, ulong arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int new_value;
	int err;
	static struct watchdog_info ls1024a_wdt_info = {
		.options = 	WDIOF_SETTIMEOUT |
				WDIOF_MAGICCLOSE |
				WDIOF_KEEPALIVEPING,
		.firmware_version = 1,
	};

	switch(cmd) {
	case WDIOC_KEEPALIVE:
		ls1024a_wdt_pet_watchdog_virtual();
		break;

	case WDIOC_GETSUPPORT:
		strncpy(ls1024a_wdt_info.identity, WDT_NAME, sizeof(ls1024a_wdt_info.identity));
		if (copy_to_user(argp, &ls1024a_wdt_info, sizeof(ls1024a_wdt_info)) != 0) {
			err = -EFAULT;
			goto err;
		}
		break;

	case WDIOC_SETTIMEOUT:
		if (get_user(new_value, p)) {
			err = -EFAULT;
			goto err;
		}

		if (ls1024a_wdt_settimeout(new_value)) {
			err = -EINVAL;
			goto err;
		}

		ls1024a_wdt_pet_watchdog_virtual();

		return put_user(wd_time, p);
		break;

	case WDIOC_GETTIMEOUT:
		return put_user(wd_time, p);
		break;

	case WDIOC_GETSTATUS:
		return put_user(0, p);
		break;

	case WDIOC_GETBOOTSTATUS:
		return put_user(ls1024a_wdt_rst_status(), p);
		break;

	case WDIOC_SETOPTIONS:
		if (get_user(new_value, p)) {
			err = -EFAULT;
			goto err;
		}

		if (new_value & WDIOS_DISABLECARD)
			ls1024a_wdt_stop();

		if (new_value & WDIOS_ENABLECARD)
			ls1024a_wdt_start();

		break;

	default:
		err = -ENOIOCTLCMD;
		goto err;
		break;
	}

	return 0;

err:
	return err;
}

/*
 * Pat the watchdog whenever device is written to.
 */
static ssize_t ls1024a_wdt_write(struct file *file, const char *buf, size_t len, loff_t *ppos)
{
	if (len) {
		if (!nowayout) {
			size_t i;
			char c;

			/* in case it was set long ago */
			expect_close = 0;

			for (i = 0; i != len; i++) {
				if (get_user(c, buf + i))
					return -EFAULT;

				if (c == 'V')
					expect_close = 42;
			}
		}

		ls1024a_wdt_pet_watchdog_virtual();
	}

	return len;
}

static const struct file_operations ls1024a_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.unlocked_ioctl	= ls1024a_wdt_ioctl,
	.open		= ls1024a_wdt_open,
	.release	= ls1024a_wdt_release,
	.write		= ls1024a_wdt_write,
};

static struct miscdevice ls1024a_wdt_miscdev = {
	.minor		= WATCHDOG_MINOR,
	.name		= WDT_NAME,
	.fops		= &ls1024a_wdt_fops,
};

static int ls1024a_wdt_probe(struct platform_device *pdev)
{
	int res;
	struct resource *iores_mem;
	struct device *dev = &pdev->dev;

	if (ls1024a_wdt_miscdev.parent)
		return -EBUSY;

	spin_lock_init(&wdt_lock);

	axi_clk = devm_clk_get(dev, NULL);
	if (IS_ERR(axi_clk)) {
		dev_err(dev, "unable to obtain AXI clock: %ld\n", PTR_ERR(axi_clk));
		return -ENODEV;
	}

	res = clk_prepare_enable(axi_clk);
	if (res) {
		dev_err(dev, "failed to enable AXI clock\n");
		goto err_clk;
	}

	LS1024A_AHBCLK = clk_get_rate(axi_clk);

	iores_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores_mem) {
		dev_err(dev, "unable to obtain IORESOURCE_MEM\n");
		goto err_mem;
	}

	ls1024a_wdt_base = devm_ioremap_resource(dev, iores_mem);
	if (IS_ERR(ls1024a_wdt_base)) {
		dev_err(dev, "devm_ioremap_resource failed: %ld\n", PTR_ERR(ls1024a_wdt_base));
		goto err_mem;
	}

	clk_rst_map = syscon_regmap_lookup_by_phandle(dev->of_node, "regmap");
	if (IS_ERR(clk_rst_map)) {
		dev_err(dev, "unable to obtain regmap: %ld\n", PTR_ERR(clk_rst_map));
		goto err_misc;
	}

	ls1024a_wdt_miscdev.parent = dev;
	ls1024a_wdt_config();

	res = misc_register(&ls1024a_wdt_miscdev);
	if (res)
		goto err_misc;

	/* check that the heartbeat value is within range; if not reset to the default */
	if (ls1024a_wdt_set_heartbeat(wd_heartbeat)) {
		ls1024a_wdt_set_heartbeat(WDT_DEFAULT_TIMEOUT);

		dev_err(dev, "wd_heartbeat value is out of range: 1..%lu, using %d\n",
			WDT_MAX_TIMEOUT, WDT_DEFAULT_TIMEOUT);
	}

	/* check that the time value is within range; if not reset to the default */
	if (ls1024a_wdt_settimeout(wd_time)) {
		ls1024a_wdt_settimeout(WDT_DEFAULT_TIMEOUT);

		dev_err(dev, "wd_time value is out of range: 1..%d, using %d\n",
			WDT_MAX_TIME, WDT_DEFAULT_TIME);
	}

	setup_timer(&wdt_timer, ls1024a_timer_tick, 0L);

	return 0;

err_misc:
	devm_iounmap(dev, ls1024a_wdt_base);
err_mem:
	clk_disable_unprepare(axi_clk);
err_clk:
	devm_clk_put(dev, axi_clk);

	return res;
}

static int ls1024a_wdt_remove(struct platform_device *pdev)
{
	int res;

	res = misc_deregister(&ls1024a_wdt_miscdev);
	if (!res)
		ls1024a_wdt_miscdev.parent = NULL;

	devm_iounmap(&pdev->dev, ls1024a_wdt_base);
	clk_disable_unprepare(axi_clk);
	devm_clk_put(&pdev->dev, axi_clk);

	return res;
}

static const struct of_device_id ls1024a_wdt_of_match[] = {
	{ .compatible = "fsl,ls1024a-wdt", },
	{},
};
MODULE_DEVICE_TABLE(of, ls1024a_wdt_of_match);

static struct platform_driver ls1024a_wdt_driver = {
	.driver	= {
		.name = WDT_NAME,
		.of_match_table = ls1024a_wdt_of_match,
	},
	.probe = ls1024a_wdt_probe,
	.remove = ls1024a_wdt_remove,
};

module_platform_driver(ls1024a_wdt_driver);

MODULE_AUTHOR("Mindspeed Technologies, Inc.");
MODULE_DESCRIPTION("Watchdog driver for Freescale QorIQ LS1024A devices");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
