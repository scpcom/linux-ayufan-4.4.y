#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/async.h>
#include <linux/bug.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/atomic.h>

static async_cookie_t next_cookie = 1;

#define MAX_THREADS	256
#define MAX_WORK	32768

static LIST_HEAD(async_pending);
static LIST_HEAD(async_running);
static DEFINE_SPINLOCK(async_lock);

static int async_enabled = 0;

struct async_entry {
	struct list_head list;
	async_cookie_t   cookie;
	async_func_ptr	 *func;
	void             *data;
	struct list_head *running;
};

static DECLARE_WAIT_QUEUE_HEAD(async_done);
static DECLARE_WAIT_QUEUE_HEAD(async_new);

static atomic_t entry_count;
static atomic_t thread_count;

extern int initcall_debug;

static async_cookie_t  __lowest_in_progress(struct list_head *running)
{
	struct async_entry *entry;

	if (!list_empty(running)) {
		entry = list_first_entry(running,
			struct async_entry, list);
		return entry->cookie;
	}

	list_for_each_entry(entry, &async_pending, list)
		if (entry->running == running)
			return entry->cookie;

	return next_cookie;	 
}

static async_cookie_t  lowest_in_progress(struct list_head *running)
{
	unsigned long flags;
	async_cookie_t ret;

	spin_lock_irqsave(&async_lock, flags);
	ret = __lowest_in_progress(running);
	spin_unlock_irqrestore(&async_lock, flags);
	return ret;
}
 
static void run_one_entry(void)
{
	unsigned long flags;
	struct async_entry *entry;
	ktime_t calltime, delta, rettime;

	spin_lock_irqsave(&async_lock, flags);
	if (list_empty(&async_pending))
		goto out;
	entry = list_first_entry(&async_pending, struct async_entry, list);

	list_move_tail(&entry->list, entry->running);
	spin_unlock_irqrestore(&async_lock, flags);

	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		printk("calling  %lli_%pF @ %i\n", (long long)entry->cookie,
			entry->func, task_pid_nr(current));
		calltime = ktime_get();
	}
	entry->func(entry->data, entry->cookie);
	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		rettime = ktime_get();
		delta = ktime_sub(rettime, calltime);
		printk("initcall %lli_%pF returned 0 after %lld usecs\n",
			(long long)entry->cookie,
			entry->func,
			(long long)ktime_to_ns(delta) >> 10);
	}

	spin_lock_irqsave(&async_lock, flags);
	list_del(&entry->list);

	kfree(entry);
	atomic_dec(&entry_count);

	spin_unlock_irqrestore(&async_lock, flags);

	wake_up(&async_done);
	return;

out:
	spin_unlock_irqrestore(&async_lock, flags);
}

static async_cookie_t __async_schedule(async_func_ptr *ptr, void *data, struct list_head *running)
{
	struct async_entry *entry;
	unsigned long flags;
	async_cookie_t newcookie;
	
	entry = kzalloc(sizeof(struct async_entry), GFP_ATOMIC);

	if (!async_enabled || !entry || atomic_read(&entry_count) > MAX_WORK) {
		kfree(entry);
		spin_lock_irqsave(&async_lock, flags);
		newcookie = next_cookie++;
		spin_unlock_irqrestore(&async_lock, flags);

		ptr(data, newcookie);
		return newcookie;
	}
	entry->func = ptr;
	entry->data = data;
	entry->running = running;

	spin_lock_irqsave(&async_lock, flags);
	newcookie = entry->cookie = next_cookie++;
	list_add_tail(&entry->list, &async_pending);
	atomic_inc(&entry_count);
	spin_unlock_irqrestore(&async_lock, flags);
	wake_up(&async_new);
	return newcookie;
}

#ifdef MY_ABC_HERE
static void __syno_async_schedule_enabled_set(int iValue)
{
	async_enabled = iValue;
}

static int __syno_async_schedule_enabled_get(void)
{
	return async_enabled;
}

void syno_async_schedule_enabled_set(int iValue)
{
	__syno_async_schedule_enabled_set(iValue);
}
EXPORT_SYMBOL(syno_async_schedule_enabled_set);

int syno_async_schedule_enabled_get(void)
{
	return __syno_async_schedule_enabled_get();
}
EXPORT_SYMBOL(syno_async_schedule_enabled_get);
#endif

async_cookie_t async_schedule(async_func_ptr *ptr, void *data)
{
	return __async_schedule(ptr, data, &async_running);
}
EXPORT_SYMBOL_GPL(async_schedule);

async_cookie_t async_schedule_domain(async_func_ptr *ptr, void *data,
				     struct list_head *running)
{
	return __async_schedule(ptr, data, running);
}
EXPORT_SYMBOL_GPL(async_schedule_domain);

void async_synchronize_full(void)
{
	do {
		async_synchronize_cookie(next_cookie);
	} while (!list_empty(&async_running) || !list_empty(&async_pending));
}
EXPORT_SYMBOL_GPL(async_synchronize_full);

void async_synchronize_full_domain(struct list_head *list)
{
	async_synchronize_cookie_domain(next_cookie, list);
}
EXPORT_SYMBOL_GPL(async_synchronize_full_domain);

void async_synchronize_cookie_domain(async_cookie_t cookie,
				     struct list_head *running)
{
	ktime_t starttime, delta, endtime;

	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		printk("async_waiting @ %i\n", task_pid_nr(current));
		starttime = ktime_get();
	}

	wait_event(async_done, lowest_in_progress(running) >= cookie);

	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		endtime = ktime_get();
		delta = ktime_sub(endtime, starttime);

		printk("async_continuing @ %i after %lli usec\n",
			task_pid_nr(current),
			(long long)ktime_to_ns(delta) >> 10);
	}
}
EXPORT_SYMBOL_GPL(async_synchronize_cookie_domain);

void async_synchronize_cookie(async_cookie_t cookie)
{
	async_synchronize_cookie_domain(cookie, &async_running);
}
EXPORT_SYMBOL_GPL(async_synchronize_cookie);

static int async_thread(void *unused)
{
	DECLARE_WAITQUEUE(wq, current);
	add_wait_queue(&async_new, &wq);

	while (!kthread_should_stop()) {
		int ret = HZ;
		set_current_state(TASK_INTERRUPTIBLE);
		 
		rmb();
		if (!list_empty(&async_pending))
			run_one_entry();
		else
			ret = schedule_timeout(HZ);

		if (ret == 0) {
			 
			atomic_dec(&thread_count);
			wmb();  
			if (list_empty(&async_pending))
				break;
			 
			atomic_inc(&thread_count);
		}
	}
	remove_wait_queue(&async_new, &wq);

	return 0;
}

static int async_manager_thread(void *unused)
{
	DECLARE_WAITQUEUE(wq, current);
	add_wait_queue(&async_new, &wq);

	while (!kthread_should_stop()) {
		int tc, ec;

		set_current_state(TASK_INTERRUPTIBLE);

		tc = atomic_read(&thread_count);
		rmb();
		ec = atomic_read(&entry_count);

		while (tc < ec && tc < MAX_THREADS) {
			if (IS_ERR(kthread_run(async_thread, NULL, "async/%i",
					       tc))) {
				msleep(100);
				continue;
			}
			atomic_inc(&thread_count);
			tc++;
		}

		schedule();
	}
	remove_wait_queue(&async_new, &wq);

	return 0;
}

static int __init async_init(void)
{
	async_enabled =
		!IS_ERR(kthread_run(async_manager_thread, NULL, "async/mgr"));

	WARN_ON(!async_enabled);
	return 0;
}

core_initcall(async_init);
