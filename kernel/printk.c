#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/nmi.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>			 
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/security.h>
#include <linux/bootmem.h>
#include <linux/syscalls.h>
#include <linux/kexec.h>

#include <asm/uaccess.h>
#ifdef CONFIG_DEBUG_LL
 
#include <mach/uncompress.h>
#endif

#define for_each_console(con) \
	for (con = console_drivers; con != NULL; con = con->next)

void asmlinkage __attribute__((weak)) early_printk(const char *fmt, ...)
{
}

#define __LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)

#define DEFAULT_MESSAGE_LOGLEVEL 4  

#define MINIMUM_CONSOLE_LOGLEVEL 1  
#define DEFAULT_CONSOLE_LOGLEVEL 7  

DECLARE_WAIT_QUEUE_HEAD(log_wait);

int console_printk[4] = {
	DEFAULT_CONSOLE_LOGLEVEL,	 
	DEFAULT_MESSAGE_LOGLEVEL,	 
	MINIMUM_CONSOLE_LOGLEVEL,	 
	DEFAULT_CONSOLE_LOGLEVEL,	 
};

static int saved_console_loglevel = -1;

int oops_in_progress;
EXPORT_SYMBOL(oops_in_progress);

static DECLARE_MUTEX(console_sem);
struct console *console_drivers;
EXPORT_SYMBOL_GPL(console_drivers);

static int console_locked, console_suspended;

static DEFINE_SPINLOCK(logbuf_lock);

#define LOG_BUF_MASK (log_buf_len-1)
#define LOG_BUF(idx) (log_buf[(idx) & LOG_BUF_MASK])

static unsigned log_start;	 
static unsigned con_start;	 
static unsigned log_end;	 

struct console_cmdline
{
	char	name[8];			 
	int	index;				 
	char	*options;			 
#ifdef CONFIG_A11Y_BRAILLE_CONSOLE
	char	*brl_options;			 
#endif
};

#define MAX_CMDLINECONSOLES 8

static struct console_cmdline console_cmdline[MAX_CMDLINECONSOLES];
static int selected_console = -1;
static int preferred_console = -1;
int console_set_on_cmdline;
EXPORT_SYMBOL(console_set_on_cmdline);

static int console_may_schedule;

#ifdef CONFIG_PRINTK

static char __log_buf[__LOG_BUF_LEN];
static char *log_buf = __log_buf;
static int log_buf_len = __LOG_BUF_LEN;
static unsigned logged_chars;  

#ifdef MY_DEF_HERE
int syno_disable_kmsg = 0;
#endif

#ifdef CONFIG_KEXEC
 
void log_buf_kexec_setup(void)
{
	VMCOREINFO_SYMBOL(log_buf);
	VMCOREINFO_SYMBOL(log_end);
	VMCOREINFO_SYMBOL(log_buf_len);
	VMCOREINFO_SYMBOL(logged_chars);
}
#endif

static int __init log_buf_len_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long flags;

	if (size)
		size = roundup_pow_of_two(size);
	if (size > log_buf_len) {
		unsigned start, dest_idx, offset;
		char *new_log_buf;

		new_log_buf = alloc_bootmem(size);
		if (!new_log_buf) {
			printk(KERN_WARNING "log_buf_len: allocation failed\n");
			goto out;
		}

		spin_lock_irqsave(&logbuf_lock, flags);
		log_buf_len = size;
		log_buf = new_log_buf;

		offset = start = min(con_start, log_start);
		dest_idx = 0;
		while (start != log_end) {
			log_buf[dest_idx] = __log_buf[start & (__LOG_BUF_LEN - 1)];
			start++;
			dest_idx++;
		}
		log_start -= offset;
		con_start -= offset;
		log_end -= offset;
		spin_unlock_irqrestore(&logbuf_lock, flags);

		printk(KERN_NOTICE "log_buf_len: %d\n", log_buf_len);
	}
out:
	return 1;
}

__setup("log_buf_len=", log_buf_len_setup);

#ifdef CONFIG_BOOT_PRINTK_DELAY

static unsigned int boot_delay;  
static unsigned long long loops_per_msec;	 

static int __init boot_delay_setup(char *str)
{
	unsigned long lpj;

	lpj = preset_lpj ? preset_lpj : 1000000;	 
	loops_per_msec = (unsigned long long)lpj / 1000 * HZ;

	get_option(&str, &boot_delay);
	if (boot_delay > 10 * 1000)
		boot_delay = 0;

	pr_debug("boot_delay: %u, preset_lpj: %ld, lpj: %lu, "
		"HZ: %d, loops_per_msec: %llu\n",
		boot_delay, preset_lpj, lpj, HZ, loops_per_msec);
	return 1;
}
__setup("boot_delay=", boot_delay_setup);

static void boot_delay_msec(void)
{
	unsigned long long k;
	unsigned long timeout;

	if (boot_delay == 0 || system_state != SYSTEM_BOOTING)
		return;

	k = (unsigned long long)loops_per_msec * boot_delay;

	timeout = jiffies + msecs_to_jiffies(boot_delay);
	while (k) {
		k--;
		cpu_relax();
		 
		if (time_after(jiffies, timeout))
			break;
		touch_nmi_watchdog();
	}
}
#else
static inline void boot_delay_msec(void)
{
}
#endif

int do_syslog(int type, char __user *buf, int len)
{
	unsigned i, j, limit, count;
	int do_clear = 0;
	char c;
	int error = 0;

	error = security_syslog(type);
	if (error)
		return error;

	switch (type) {
	case 0:		 
		break;
	case 1:		 
		break;
	case 2:		 
#ifdef MY_DEF_HERE
		if (syno_disable_kmsg){
			return -EAGAIN;
		}
#endif
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		error = wait_event_interruptible(log_wait,
							(log_start - log_end));
		if (error)
			goto out;
		i = 0;
		spin_lock_irq(&logbuf_lock);
		while (!error && (log_start != log_end) && i < len) {
			c = LOG_BUF(log_start);
			log_start++;
			spin_unlock_irq(&logbuf_lock);
			error = __put_user(c,buf);
			buf++;
			i++;
			cond_resched();
			spin_lock_irq(&logbuf_lock);
		}
		spin_unlock_irq(&logbuf_lock);
		if (!error)
			error = i;
		break;
	case 4:		 
		do_clear = 1;
		 
	case 3:		 
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		count = len;
		if (count > log_buf_len)
			count = log_buf_len;
		spin_lock_irq(&logbuf_lock);
		if (count > logged_chars)
			count = logged_chars;
		if (do_clear)
			logged_chars = 0;
		limit = log_end;
		 
		for (i = 0; i < count && !error; i++) {
			j = limit-1-i;
			if (j + log_buf_len < log_end)
				break;
			c = LOG_BUF(j);
			spin_unlock_irq(&logbuf_lock);
			error = __put_user(c,&buf[count-1-i]);
			cond_resched();
			spin_lock_irq(&logbuf_lock);
		}
		spin_unlock_irq(&logbuf_lock);
		if (error)
			break;
		error = i;
		if (i != count) {
			int offset = count-error;
			 
			for (i = 0; i < error; i++) {
				if (__get_user(c,&buf[i+offset]) ||
				    __put_user(c,&buf[i])) {
					error = -EFAULT;
					break;
				}
				cond_resched();
			}
		}
		break;
	case 5:		 
		logged_chars = 0;
		break;
	case 6:		 
		if (saved_console_loglevel == -1)
			saved_console_loglevel = console_loglevel;
		console_loglevel = minimum_console_loglevel;
		break;
	case 7:		 
		if (saved_console_loglevel != -1) {
			console_loglevel = saved_console_loglevel;
			saved_console_loglevel = -1;
		}
		break;
	case 8:		 
		error = -EINVAL;
		if (len < 1 || len > 8)
			goto out;
		if (len < minimum_console_loglevel)
			len = minimum_console_loglevel;
		console_loglevel = len;
		 
		saved_console_loglevel = -1;
		error = 0;
		break;
	case 9:		 
		error = log_end - log_start;
		break;
	case 10:	 
		error = log_buf_len;
		break;
	default:
		error = -EINVAL;
		break;
	}
out:
	return error;
}

SYSCALL_DEFINE3(syslog, int, type, char __user *, buf, int, len)
{
	return do_syslog(type, buf, len);
}

static void __call_console_drivers(unsigned start, unsigned end)
{
	struct console *con;

	for_each_console(con) {
		if ((con->flags & CON_ENABLED) && con->write &&
				(cpu_online(smp_processor_id()) ||
				(con->flags & CON_ANYTIME)))
			con->write(con, &LOG_BUF(start), end - start);
	}
}

static int __read_mostly ignore_loglevel;

static int __init ignore_loglevel_setup(char *str)
{
	ignore_loglevel = 1;
	printk(KERN_INFO "debug: ignoring loglevel setting.\n");

	return 0;
}

early_param("ignore_loglevel", ignore_loglevel_setup);

static void _call_console_drivers(unsigned start,
				unsigned end, int msg_log_level)
{
	if ((msg_log_level < console_loglevel || ignore_loglevel) &&
			console_drivers && start != end) {
		if ((start & LOG_BUF_MASK) > (end & LOG_BUF_MASK)) {
			 
			__call_console_drivers(start & LOG_BUF_MASK,
						log_buf_len);
			__call_console_drivers(0, end & LOG_BUF_MASK);
		} else {
			__call_console_drivers(start, end);
		}
	}
}

static void call_console_drivers(unsigned start, unsigned end)
{
	unsigned cur_index, start_print;
	static int msg_level = -1;

	BUG_ON(((int)(start - end)) > 0);

	cur_index = start;
	start_print = start;
	while (cur_index != end) {
		if (msg_level < 0 && ((end - cur_index) > 2) &&
				LOG_BUF(cur_index + 0) == '<' &&
				LOG_BUF(cur_index + 1) >= '0' &&
				LOG_BUF(cur_index + 1) <= '7' &&
				LOG_BUF(cur_index + 2) == '>') {
			msg_level = LOG_BUF(cur_index + 1) - '0';
			cur_index += 3;
			start_print = cur_index;
		}
		while (cur_index != end) {
			char c = LOG_BUF(cur_index);

			cur_index++;
			if (c == '\n') {
				if (msg_level < 0) {
					 
					msg_level = default_message_loglevel;
				}
				_call_console_drivers(start_print, cur_index, msg_level);
				msg_level = -1;
				start_print = cur_index;
				break;
			}
		}
	}
	_call_console_drivers(start_print, end, msg_level);
}

static void emit_log_char(char c)
{
	LOG_BUF(log_end) = c;
	log_end++;
	if (log_end - log_start > log_buf_len)
		log_start = log_end - log_buf_len;
	if (log_end - con_start > log_buf_len)
		con_start = log_end - log_buf_len;
	if (logged_chars < log_buf_len)
		logged_chars++;
}

static void zap_locks(void)
{
	static unsigned long oops_timestamp;

	if (time_after_eq(jiffies, oops_timestamp) &&
			!time_after(jiffies, oops_timestamp + 30 * HZ))
		return;

	oops_timestamp = jiffies;

	spin_lock_init(&logbuf_lock);
	 
	init_MUTEX(&console_sem);
}

#if defined(CONFIG_PRINTK_TIME)
static int printk_time = 1;
#else
static int printk_time = 0;
#endif
module_param_named(time, printk_time, bool, S_IRUGO | S_IWUSR);

static int have_callable_console(void)
{
	struct console *con;

	for_each_console(con)
		if (con->flags & CON_ANYTIME)
			return 1;

	return 0;
}

asmlinkage int printk(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);

	return r;
}

#ifdef CONFIG_DEBUG_LL
int ll_debug = 0;
#endif
 
static volatile unsigned int printk_cpu = UINT_MAX;

static inline int can_use_console(unsigned int cpu)
{
	return cpu_online(cpu) || have_callable_console();
}

static int acquire_console_semaphore_for_printk(unsigned int cpu)
{
	int retval = 0;

	if (!try_acquire_console_sem()) {
		retval = 1;

		if (!can_use_console(cpu)) {
			console_locked = 0;
			up(&console_sem);
			retval = 0;
		}
	}
	printk_cpu = UINT_MAX;
	spin_unlock(&logbuf_lock);
	return retval;
}
static const char recursion_bug_msg [] =
		KERN_CRIT "BUG: recent printk recursion!\n";
static int recursion_bug;
static int new_text_line = 1;
static char printk_buf[1024];

int printk_delay_msec __read_mostly;

static inline void printk_delay(void)
{
	if (unlikely(printk_delay_msec)) {
		int m = printk_delay_msec;

		while (m--) {
			mdelay(1);
			touch_nmi_watchdog();
		}
	}
}

asmlinkage int vprintk(const char *fmt, va_list args)
{
	int printed_len = 0;
	int current_log_level = default_message_loglevel;
	unsigned long flags;
	int this_cpu;
	char *p;

	boot_delay_msec();
	printk_delay();

	preempt_disable();
	 
	raw_local_irq_save(flags);
	this_cpu = smp_processor_id();

	if (unlikely(printk_cpu == this_cpu)) {
		 
		if (!oops_in_progress) {
			recursion_bug = 1;
			goto out_restore_irqs;
		}
		zap_locks();
	}

	lockdep_off();
	spin_lock(&logbuf_lock);
	printk_cpu = this_cpu;

	if (recursion_bug) {
		recursion_bug = 0;
		strcpy(printk_buf, recursion_bug_msg);
		printed_len = strlen(recursion_bug_msg);
	}
	 
	printed_len += vscnprintf(printk_buf + printed_len,
				  sizeof(printk_buf) - printed_len, fmt, args);
#ifdef CONFIG_DEBUG_LL
	{ extern void printascii(const char *); printascii(printk_buf); }
#endif

	p = printk_buf;

	if (p[0] == '<') {
		unsigned char c = p[1];
		if (c && p[2] == '>') {
			switch (c) {
			case '0' ... '7':  
				current_log_level = c - '0';
			 
			case 'd':  
				if (!new_text_line) {
					emit_log_char('\n');
					new_text_line = 1;
				}
			 
			case 'c':  
				p += 3;
				break;
			}
		}
	}

	for ( ; *p; p++) {
		if (new_text_line) {
			 
			emit_log_char('<');
			emit_log_char(current_log_level + '0');
			emit_log_char('>');
			printed_len += 3;
			new_text_line = 0;

			if (printk_time) {
				 
				char tbuf[50], *tp;
				unsigned tlen;
				unsigned long long t;
				unsigned long nanosec_rem;

				t = cpu_clock(printk_cpu);
				nanosec_rem = do_div(t, 1000000000);
				tlen = sprintf(tbuf, "[%5lu.%06lu] ",
						(unsigned long) t,
						nanosec_rem / 1000);

				for (tp = tbuf; tp < tbuf + tlen; tp++)
					emit_log_char(*tp);
				printed_len += tlen;
			}

			if (!*p)
				break;
		}

		emit_log_char(*p);
		if (*p == '\n')
			new_text_line = 1;
	}

	if (acquire_console_semaphore_for_printk(this_cpu))
		release_console_sem();

	lockdep_on();
out_restore_irqs:
	raw_local_irq_restore(flags);

	preempt_enable();
	return printed_len;
}
EXPORT_SYMBOL(printk);
EXPORT_SYMBOL(vprintk);

#else

static void call_console_drivers(unsigned start, unsigned end)
{
}

#endif

static int __add_preferred_console(char *name, int idx, char *options,
				   char *brl_options)
{
	struct console_cmdline *c;
	int i;

	for (i = 0; i < MAX_CMDLINECONSOLES && console_cmdline[i].name[0]; i++)
		if (strcmp(console_cmdline[i].name, name) == 0 &&
			  console_cmdline[i].index == idx) {
				if (!brl_options)
					selected_console = i;
				return 0;
		}
	if (i == MAX_CMDLINECONSOLES)
		return -E2BIG;
	if (!brl_options)
		selected_console = i;
	c = &console_cmdline[i];
	strlcpy(c->name, name, sizeof(c->name));
	c->options = options;
#ifdef CONFIG_A11Y_BRAILLE_CONSOLE
	c->brl_options = brl_options;
#endif
	c->index = idx;
	return 0;
}
 
static int __init console_setup(char *str)
{
	char buf[sizeof(console_cmdline[0].name) + 4];  
	char *s, *options, *brl_options = NULL;
	int idx;

#ifdef CONFIG_A11Y_BRAILLE_CONSOLE
	if (!memcmp(str, "brl,", 4)) {
		brl_options = "";
		str += 4;
	} else if (!memcmp(str, "brl=", 4)) {
		brl_options = str + 4;
		str = strchr(brl_options, ',');
		if (!str) {
			printk(KERN_ERR "need port name after brl=\n");
			return 1;
		}
		*(str++) = 0;
	}
#endif

	if (str[0] >= '0' && str[0] <= '9') {
		strcpy(buf, "ttyS");
		strncpy(buf + 4, str, sizeof(buf) - 5);
	} else {
		strncpy(buf, str, sizeof(buf) - 1);
	}
	buf[sizeof(buf) - 1] = 0;
	if ((options = strchr(str, ',')) != NULL)
		*(options++) = 0;
#ifdef __sparc__
	if (!strcmp(str, "ttya"))
		strcpy(buf, "ttyS0");
	if (!strcmp(str, "ttyb"))
		strcpy(buf, "ttyS1");
#endif
	for (s = buf; *s; s++)
		if ((*s >= '0' && *s <= '9') || *s == ',')
			break;
	idx = simple_strtoul(s, NULL, 10);
	*s = 0;

	__add_preferred_console(buf, idx, options, brl_options);
	console_set_on_cmdline = 1;
	return 1;
}
__setup("console=", console_setup);

int add_preferred_console(char *name, int idx, char *options)
{
	return __add_preferred_console(name, idx, options, NULL);
}

int update_console_cmdline(char *name, int idx, char *name_new, int idx_new, char *options)
{
	struct console_cmdline *c;
	int i;

	for (i = 0; i < MAX_CMDLINECONSOLES && console_cmdline[i].name[0]; i++)
		if (strcmp(console_cmdline[i].name, name) == 0 &&
			  console_cmdline[i].index == idx) {
				c = &console_cmdline[i];
				strlcpy(c->name, name_new, sizeof(c->name));
				c->name[sizeof(c->name) - 1] = 0;
				c->options = options;
				c->index = idx_new;
				return i;
		}
	 
	return -1;
}

int console_suspend_enabled = 1;
EXPORT_SYMBOL(console_suspend_enabled);

static int __init console_suspend_disable(char *str)
{
	console_suspend_enabled = 0;
	return 1;
}
__setup("no_console_suspend", console_suspend_disable);

void suspend_console(void)
{
	if (!console_suspend_enabled)
		return;
	printk("Suspending console(s) (use no_console_suspend to debug)\n");
	acquire_console_sem();
	console_suspended = 1;
	up(&console_sem);
}

void resume_console(void)
{
	if (!console_suspend_enabled)
		return;
	down(&console_sem);
	console_suspended = 0;
	release_console_sem();
}

void acquire_console_sem(void)
{
	BUG_ON(in_interrupt());
	down(&console_sem);
	if (console_suspended)
		return;
	console_locked = 1;
	console_may_schedule = 1;
}
EXPORT_SYMBOL(acquire_console_sem);

int try_acquire_console_sem(void)
{
	if (down_trylock(&console_sem))
		return -1;
	if (console_suspended) {
		up(&console_sem);
		return -1;
	}
	console_locked = 1;
	console_may_schedule = 0;
	return 0;
}
EXPORT_SYMBOL(try_acquire_console_sem);

int is_console_locked(void)
{
	return console_locked;
}

static DEFINE_PER_CPU(int, printk_pending);

void printk_tick(void)
{
	if (__get_cpu_var(printk_pending)) {
		__get_cpu_var(printk_pending) = 0;
		wake_up_interruptible(&log_wait);
	}
}

int printk_needs_cpu(int cpu)
{
	return per_cpu(printk_pending, cpu);
}

void wake_up_klogd(void)
{
	if (waitqueue_active(&log_wait))
		__raw_get_cpu_var(printk_pending) = 1;
}

void release_console_sem(void)
{
	unsigned long flags;
	unsigned _con_start, _log_end;
	unsigned wake_klogd = 0;

	if (console_suspended) {
		up(&console_sem);
		return;
	}

	console_may_schedule = 0;

	for ( ; ; ) {
		spin_lock_irqsave(&logbuf_lock, flags);
		wake_klogd |= log_start - log_end;
		if (con_start == log_end)
			break;			 
		_con_start = con_start;
		_log_end = log_end;
		con_start = log_end;		 
		spin_unlock(&logbuf_lock);
		stop_critical_timings();	 
		call_console_drivers(_con_start, _log_end);
		start_critical_timings();
		local_irq_restore(flags);
	}
	console_locked = 0;
	up(&console_sem);
	spin_unlock_irqrestore(&logbuf_lock, flags);
	if (wake_klogd)
		wake_up_klogd();
}
EXPORT_SYMBOL(release_console_sem);

void __sched console_conditional_schedule(void)
{
	if (console_may_schedule)
		cond_resched();
}
EXPORT_SYMBOL(console_conditional_schedule);

void console_unblank(void)
{
	struct console *c;

	if (oops_in_progress) {
		if (down_trylock(&console_sem) != 0)
			return;
	} else
		acquire_console_sem();

	console_locked = 1;
	console_may_schedule = 0;
	for_each_console(c)
		if ((c->flags & CON_ENABLED) && c->unblank)
			c->unblank();
	release_console_sem();
}

struct tty_driver *console_device(int *index)
{
	struct console *c;
	struct tty_driver *driver = NULL;

	acquire_console_sem();
	for_each_console(c) {
		if (!c->device)
			continue;
		driver = c->device(c, index);
		if (driver)
			break;
	}
	release_console_sem();
	return driver;
}

void console_stop(struct console *console)
{
	acquire_console_sem();
	console->flags &= ~CON_ENABLED;
	release_console_sem();
}
EXPORT_SYMBOL(console_stop);

void console_start(struct console *console)
{
	acquire_console_sem();
	console->flags |= CON_ENABLED;
	release_console_sem();
}
EXPORT_SYMBOL(console_start);

void register_console(struct console *newcon)
{
	int i;
	unsigned long flags;
	struct console *bcon = NULL;

	if (console_drivers && newcon->flags & CON_BOOT) {
		 
		for_each_console(bcon) {
			if (!(bcon->flags & CON_BOOT)) {
				printk(KERN_INFO "Too late to register bootconsole %s%d\n",
					newcon->name, newcon->index);
				return;
			}
		}
	}

	if (console_drivers && console_drivers->flags & CON_BOOT)
		bcon = console_drivers;

	if (preferred_console < 0 || bcon || !console_drivers)
		preferred_console = selected_console;

	if (newcon->early_setup)
		newcon->early_setup();

	if (preferred_console < 0) {
		if (newcon->index < 0)
			newcon->index = 0;
		if (newcon->setup == NULL ||
		    newcon->setup(newcon, NULL) == 0) {
			newcon->flags |= CON_ENABLED;
			if (newcon->device) {
				newcon->flags |= CON_CONSDEV;
				preferred_console = 0;
			}
		}
	}

	for (i = 0; i < MAX_CMDLINECONSOLES && console_cmdline[i].name[0];
			i++) {
		if (strcmp(console_cmdline[i].name, newcon->name) != 0)
			continue;
		if (newcon->index >= 0 &&
		    newcon->index != console_cmdline[i].index)
			continue;
		if (newcon->index < 0)
			newcon->index = console_cmdline[i].index;
#ifdef CONFIG_A11Y_BRAILLE_CONSOLE
		if (console_cmdline[i].brl_options) {
			newcon->flags |= CON_BRL;
			braille_register_console(newcon,
					console_cmdline[i].index,
					console_cmdline[i].options,
					console_cmdline[i].brl_options);
			return;
		}
#endif
		if (newcon->setup &&
		    newcon->setup(newcon, console_cmdline[i].options) != 0)
			break;
		newcon->flags |= CON_ENABLED;
		newcon->index = console_cmdline[i].index;
		if (i == selected_console) {
			newcon->flags |= CON_CONSDEV;
			preferred_console = selected_console;
		}
		break;
	}

	if (!(newcon->flags & CON_ENABLED))
		return;

	if (bcon && ((newcon->flags & (CON_CONSDEV | CON_BOOT)) == CON_CONSDEV))
		newcon->flags &= ~CON_PRINTBUFFER;

	acquire_console_sem();
	if ((newcon->flags & CON_CONSDEV) || console_drivers == NULL) {
		newcon->next = console_drivers;
		console_drivers = newcon;
		if (newcon->next)
			newcon->next->flags &= ~CON_CONSDEV;
	} else {
		newcon->next = console_drivers->next;
		console_drivers->next = newcon;
	}
	if (newcon->flags & CON_PRINTBUFFER) {
		 
		spin_lock_irqsave(&logbuf_lock, flags);
		con_start = log_start;
		spin_unlock_irqrestore(&logbuf_lock, flags);
	}
	release_console_sem();

	if (bcon && ((newcon->flags & (CON_CONSDEV | CON_BOOT)) == CON_CONSDEV)) {
		 
		printk(KERN_INFO "console [%s%d] enabled, bootconsole disabled\n",
			newcon->name, newcon->index);
		for_each_console(bcon)
			if (bcon->flags & CON_BOOT)
				unregister_console(bcon);
	} else {
		printk(KERN_INFO "%sconsole [%s%d] enabled\n",
			(newcon->flags & CON_BOOT) ? "boot" : "" ,
			newcon->name, newcon->index);
	}
}
EXPORT_SYMBOL(register_console);

int unregister_console(struct console *console)
{
        struct console *a, *b;
	int res = 1;

#ifdef CONFIG_A11Y_BRAILLE_CONSOLE
	if (console->flags & CON_BRL)
		return braille_unregister_console(console);
#endif

	acquire_console_sem();
	if (console_drivers == console) {
		console_drivers=console->next;
		res = 0;
	} else if (console_drivers) {
		for (a=console_drivers->next, b=console_drivers ;
		     a; b=a, a=b->next) {
			if (a == console) {
				b->next = a->next;
				res = 0;
				break;
			}
		}
	}

	if (console_drivers != NULL && console->flags & CON_CONSDEV)
		console_drivers->flags |= CON_CONSDEV;

	release_console_sem();
	return res;
}
EXPORT_SYMBOL(unregister_console);

static int __init disable_boot_consoles(void)
{
	struct console *con;

	for_each_console(con) {
		if (con->flags & CON_BOOT) {
			printk(KERN_INFO "turn off boot console %s%d\n",
				con->name, con->index);
			unregister_console(con);
		}
	}
	return 0;
}
late_initcall(disable_boot_consoles);

#if defined CONFIG_PRINTK

DEFINE_RATELIMIT_STATE(printk_ratelimit_state, 5 * HZ, 10);

int printk_ratelimit(void)
{
	return __ratelimit(&printk_ratelimit_state);
}
EXPORT_SYMBOL(printk_ratelimit);

bool printk_timed_ratelimit(unsigned long *caller_jiffies,
			unsigned int interval_msecs)
{
	if (*caller_jiffies == 0
			|| !time_in_range(jiffies, *caller_jiffies,
					*caller_jiffies
					+ msecs_to_jiffies(interval_msecs))) {
		*caller_jiffies = jiffies;
		return true;
	}
	return false;
}
EXPORT_SYMBOL(printk_timed_ratelimit);
#endif
