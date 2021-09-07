 
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <linux/miscdevice.h>
#include <linux/notifier.h>
#include <linux/watchdog.h>
#include <linux/uaccess.h>

#include <asm/reg_booke.h>
#include <asm/system.h>
#include <asm/time.h>
#include <asm/div64.h>

#ifdef	CONFIG_FSL_BOOKE
#define WDT_PERIOD_DEFAULT 38	 
#else
#define WDT_PERIOD_DEFAULT 3	 
#endif				 

u32 booke_wdt_enabled;
u32 booke_wdt_period = WDT_PERIOD_DEFAULT;

#ifdef	CONFIG_FSL_BOOKE
#define WDTP(x)		((((x)&0x3)<<30)|(((x)&0x3c)<<15))
#ifdef CONFIG_SYNO_QORIQ
#define WDTP_MASK	(WDTP(0xff))
#else
#define WDTP_MASK	(WDTP(0))
#endif
#else
#define WDTP(x)		(TCR_WP(x))
#define WDTP_MASK	(TCR_WP_MASK)
#endif

static DEFINE_SPINLOCK(booke_wdt_lock);

static unsigned long long period_to_sec(unsigned int period)
{
	unsigned long long tmp = 1ULL << (64 - period);
	unsigned long tmp2 = ppc_tb_freq;

	tmp2 = tmp2 / 5 * 2;

	do_div(tmp, tmp2);
	return tmp;
}

static unsigned int sec_to_period(unsigned int secs)
{
	unsigned int period;
	for (period = 63; period > 0; period--) {
		if (period_to_sec(period) >= secs)
			return period;
	}
	return 0;
}

static void __booke_wdt_ping(void *data)
{
	mtspr(SPRN_TSR, TSR_ENW|TSR_WIS);
}

static void booke_wdt_ping(void)
{
	on_each_cpu(__booke_wdt_ping, NULL, 0);
}

static void __booke_wdt_enable(void *data)
{
	u32 val;

	__booke_wdt_ping(NULL);
	val = mfspr(SPRN_TCR);
	val &= ~WDTP_MASK;
	val |= (TCR_WIE|TCR_WRC(WRC_CHIP)|WDTP(booke_wdt_period));

	mtspr(SPRN_TCR, val);
}

#ifdef CONFIG_SYNO_QORIQ
static void __booke_wdt_disable(void *data)
{
	u32 val;

	val = mfspr(SPRN_TCR);
	printk(KERN_DEBUG "SPRN_TCR=%08x\n", val);
	val &= ~(TCR_WIE | TCR_WRC_MASK | WDTP_MASK);
	mtspr(SPRN_TCR, val);
	printk(KERN_DEBUG "SPRN_TCR=%08x\n", val);
}

static int booke_wdt_release(struct inode *inode, struct file *file)
{
	on_each_cpu(__booke_wdt_disable, NULL, 0);
	booke_wdt_enabled = 0;
}
#endif

static ssize_t booke_wdt_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	booke_wdt_ping();
	return count;
}

static struct watchdog_info ident = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity = "PowerPC Book-E Watchdog",
};

static long booke_wdt_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	u32 tmp = 0;
	u32 __user *p = (u32 __user *)arg;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		if (copy_to_user((void *)arg, &ident, sizeof(ident)))
			return -EFAULT;
	case WDIOC_GETSTATUS:
		return put_user(ident.options, p);
	case WDIOC_GETBOOTSTATUS:
		 
		tmp = mfspr(SPRN_TSR) & TSR_WRS(3);
		 
		return (tmp ? 1 : 0);
	case WDIOC_SETOPTIONS:
		if (get_user(tmp, p))
			return -EINVAL;
		if (tmp == WDIOS_ENABLECARD) {
			booke_wdt_ping();
			break;
		} else
			return -EINVAL;
		return 0;
	case WDIOC_KEEPALIVE:
		booke_wdt_ping();
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(tmp, p))
			return -EFAULT;
#ifdef	CONFIG_FSL_BOOKE
		 
		if (tmp > period_to_sec(1))
			return -EINVAL;
		booke_wdt_period = sec_to_period(tmp);
#else
		booke_wdt_period = tmp;
#endif
		mtspr(SPRN_TCR, (mfspr(SPRN_TCR) & ~WDTP_MASK) |
						WDTP(booke_wdt_period));
		return 0;
	case WDIOC_GETTIMEOUT:
		return put_user(booke_wdt_period, p);
	default:
		return -ENOTTY;
	}

	return 0;
}

static int booke_wdt_open(struct inode *inode, struct file *file)
{
	spin_lock(&booke_wdt_lock);
	if (booke_wdt_enabled == 0) {
		booke_wdt_enabled = 1;
		on_each_cpu(__booke_wdt_enable, NULL, 0);
		printk(KERN_INFO
		      "PowerPC Book-E Watchdog Timer Enabled (wdt_period=%d)\n",
				booke_wdt_period);
	}
	spin_unlock(&booke_wdt_lock);

	return nonseekable_open(inode, file);
}

static const struct file_operations booke_wdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.write = booke_wdt_write,
	.unlocked_ioctl = booke_wdt_ioctl,
	.open = booke_wdt_open,
#ifdef CONFIG_SYNO_QORIQ
	.release = booke_wdt_release,
#endif
};

static struct miscdevice booke_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &booke_wdt_fops,
};

static void __exit booke_wdt_exit(void)
{
	misc_deregister(&booke_wdt_miscdev);
}

static int __init booke_wdt_init(void)
{
	int ret = 0;

	printk(KERN_INFO "PowerPC Book-E Watchdog Timer Loaded\n");
	ident.firmware_version = cur_cpu_spec->pvr_value;

	ret = misc_register(&booke_wdt_miscdev);
	if (ret) {
		printk(KERN_CRIT "Cannot register miscdev on minor=%d: %d\n",
				WATCHDOG_MINOR, ret);
		return ret;
	}

	spin_lock(&booke_wdt_lock);
	if (booke_wdt_enabled == 1) {
		printk(KERN_INFO
		      "PowerPC Book-E Watchdog Timer Enabled (wdt_period=%d)\n",
				booke_wdt_period);
		on_each_cpu(__booke_wdt_enable, NULL, 0);
	}
	spin_unlock(&booke_wdt_lock);

	return ret;
}
device_initcall(booke_wdt_init);
