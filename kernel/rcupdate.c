#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/percpu.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/hardirq.h>
#include <linux/delay.h>
#include <linux/module.h>

#define CREATE_TRACE_POINTS
#include <trace/events/rcu.h>

#include "rcu.h"

module_param(rcu_expedited, int, 0);

#ifdef CONFIG_PREEMPT_RCU

void __rcu_read_lock(void)
{
	current->rcu_read_lock_nesting++;
	barrier();   
}
EXPORT_SYMBOL_GPL(__rcu_read_lock);

void __rcu_read_unlock(void)
{
	struct task_struct *t = current;

	if (t->rcu_read_lock_nesting != 1) {
		--t->rcu_read_lock_nesting;
	} else {
		barrier();   
		t->rcu_read_lock_nesting = INT_MIN;
#ifdef CONFIG_PROVE_RCU_DELAY
		udelay(10);  
#endif  
		barrier();   
		if (unlikely(ACCESS_ONCE(t->rcu_read_unlock_special)))
			rcu_read_unlock_special(t);
		barrier();   
		t->rcu_read_lock_nesting = 0;
	}
#ifdef CONFIG_PROVE_LOCKING
	{
		int rrln = ACCESS_ONCE(t->rcu_read_lock_nesting);

		WARN_ON_ONCE(rrln < 0 && rrln > INT_MIN / 2);
	}
#endif  
}
EXPORT_SYMBOL_GPL(__rcu_read_unlock);

void exit_rcu(void)
{
	struct task_struct *t = current;

	if (likely(list_empty(&current->rcu_node_entry)))
		return;
	t->rcu_read_lock_nesting = 1;
	barrier();
	t->rcu_read_unlock_special = RCU_READ_UNLOCK_BLOCKED;
	__rcu_read_unlock();
}

#else  

void exit_rcu(void)
{
}

#endif  

#ifdef CONFIG_DEBUG_LOCK_ALLOC
static struct lock_class_key rcu_lock_key;
struct lockdep_map rcu_lock_map =
	STATIC_LOCKDEP_MAP_INIT("rcu_read_lock", &rcu_lock_key);
EXPORT_SYMBOL_GPL(rcu_lock_map);

static struct lock_class_key rcu_bh_lock_key;
struct lockdep_map rcu_bh_lock_map =
	STATIC_LOCKDEP_MAP_INIT("rcu_read_lock_bh", &rcu_bh_lock_key);
EXPORT_SYMBOL_GPL(rcu_bh_lock_map);

static struct lock_class_key rcu_sched_lock_key;
struct lockdep_map rcu_sched_lock_map =
	STATIC_LOCKDEP_MAP_INIT("rcu_read_lock_sched", &rcu_sched_lock_key);
EXPORT_SYMBOL_GPL(rcu_sched_lock_map);
#endif

#ifdef CONFIG_DEBUG_LOCK_ALLOC

int debug_lockdep_rcu_enabled(void)
{
	return rcu_scheduler_active && debug_locks &&
	       current->lockdep_recursion == 0;
}
EXPORT_SYMBOL_GPL(debug_lockdep_rcu_enabled);

int rcu_read_lock_bh_held(void)
{
	if (!debug_lockdep_rcu_enabled())
		return 1;
	if (rcu_is_cpu_idle())
		return 0;
	if (!rcu_lockdep_current_cpu_online())
		return 0;
	return in_softirq() || irqs_disabled();
}
EXPORT_SYMBOL_GPL(rcu_read_lock_bh_held);

#endif  

struct rcu_synchronize {
	struct rcu_head head;
	struct completion completion;
};

static void wakeme_after_rcu(struct rcu_head  *head)
{
	struct rcu_synchronize *rcu;

	rcu = container_of(head, struct rcu_synchronize, head);
	complete(&rcu->completion);
}

void wait_rcu_gp(call_rcu_func_t crf)
{
	struct rcu_synchronize rcu;

	init_rcu_head_on_stack(&rcu.head);
	init_completion(&rcu.completion);
	 
	crf(&rcu.head, wakeme_after_rcu);
	 
	wait_for_completion(&rcu.completion);
	destroy_rcu_head_on_stack(&rcu.head);
}
EXPORT_SYMBOL_GPL(wait_rcu_gp);

#ifdef CONFIG_PROVE_RCU
 
int rcu_my_thread_group_empty(void)
{
	return thread_group_empty(current);
}
EXPORT_SYMBOL_GPL(rcu_my_thread_group_empty);
#endif  

#ifdef CONFIG_DEBUG_OBJECTS_RCU_HEAD
static inline void debug_init_rcu_head(struct rcu_head *head)
{
	debug_object_init(head, &rcuhead_debug_descr);
}

static inline void debug_rcu_head_free(struct rcu_head *head)
{
	debug_object_free(head, &rcuhead_debug_descr);
}

static int rcuhead_fixup_init(void *addr, enum debug_obj_state state)
{
	struct rcu_head *head = addr;

	switch (state) {
	case ODEBUG_STATE_ACTIVE:
		 
#ifndef CONFIG_PREEMPT
		WARN_ON_ONCE(1);
		return 0;
#endif
		if (rcu_preempt_depth() != 0 || preempt_count() != 0 ||
		    irqs_disabled()) {
			WARN_ON_ONCE(1);
			return 0;
		}
		rcu_barrier();
		rcu_barrier_sched();
		rcu_barrier_bh();
		debug_object_init(head, &rcuhead_debug_descr);
		return 1;
	default:
		return 0;
	}
}

static int rcuhead_fixup_activate(void *addr, enum debug_obj_state state)
{
	struct rcu_head *head = addr;

	switch (state) {

	case ODEBUG_STATE_NOTAVAILABLE:
		 
		debug_object_init(head, &rcuhead_debug_descr);
		debug_object_activate(head, &rcuhead_debug_descr);
		return 0;

	case ODEBUG_STATE_ACTIVE:
		 
#ifndef CONFIG_PREEMPT
		WARN_ON_ONCE(1);
		return 0;
#endif
		if (rcu_preempt_depth() != 0 || preempt_count() != 0 ||
		    irqs_disabled()) {
			WARN_ON_ONCE(1);
			return 0;
		}
		rcu_barrier();
		rcu_barrier_sched();
		rcu_barrier_bh();
		debug_object_activate(head, &rcuhead_debug_descr);
		return 1;
	default:
		return 0;
	}
}

static int rcuhead_fixup_free(void *addr, enum debug_obj_state state)
{
	struct rcu_head *head = addr;

	switch (state) {
	case ODEBUG_STATE_ACTIVE:
		 
#ifndef CONFIG_PREEMPT
		WARN_ON_ONCE(1);
		return 0;
#endif
		if (rcu_preempt_depth() != 0 || preempt_count() != 0 ||
		    irqs_disabled()) {
			WARN_ON_ONCE(1);
			return 0;
		}
		rcu_barrier();
		rcu_barrier_sched();
		rcu_barrier_bh();
		debug_object_free(head, &rcuhead_debug_descr);
		return 1;
	default:
		return 0;
	}
}

void init_rcu_head_on_stack(struct rcu_head *head)
{
	debug_object_init_on_stack(head, &rcuhead_debug_descr);
}
EXPORT_SYMBOL_GPL(init_rcu_head_on_stack);

void destroy_rcu_head_on_stack(struct rcu_head *head)
{
	debug_object_free(head, &rcuhead_debug_descr);
}
EXPORT_SYMBOL_GPL(destroy_rcu_head_on_stack);

struct debug_obj_descr rcuhead_debug_descr = {
	.name = "rcu_head",
	.fixup_init = rcuhead_fixup_init,
	.fixup_activate = rcuhead_fixup_activate,
	.fixup_free = rcuhead_fixup_free,
};
EXPORT_SYMBOL_GPL(rcuhead_debug_descr);
#endif  

#if defined(CONFIG_TREE_RCU) || defined(CONFIG_TREE_PREEMPT_RCU) || defined(CONFIG_RCU_TRACE)
void do_trace_rcu_torture_read(char *rcutorturename, struct rcu_head *rhp,
			       unsigned long secs,
			       unsigned long c_old, unsigned long c)
{
	trace_rcu_torture_read(rcutorturename, rhp, secs, c_old, c);
}
EXPORT_SYMBOL_GPL(do_trace_rcu_torture_read);
#else
#define do_trace_rcu_torture_read(rcutorturename, rhp, secs, c_old, c) \
	do { } while (0)
#endif

#ifdef CONFIG_RCU_STALL_COMMON

#ifdef CONFIG_PROVE_RCU
#define RCU_STALL_DELAY_DELTA	       (5 * HZ)
#else
#define RCU_STALL_DELAY_DELTA	       0
#endif

#ifdef MY_ABC_HERE
int rcu_cpu_stall_suppress __read_mostly = 1;  
#else
int rcu_cpu_stall_suppress __read_mostly;  
#endif
int rcu_cpu_stall_timeout __read_mostly = CONFIG_RCU_CPU_STALL_TIMEOUT;

module_param(rcu_cpu_stall_suppress, int, 0644);
module_param(rcu_cpu_stall_timeout, int, 0644);

int rcu_jiffies_till_stall_check(void)
{
	int till_stall_check = ACCESS_ONCE(rcu_cpu_stall_timeout);

	if (till_stall_check < 3) {
		ACCESS_ONCE(rcu_cpu_stall_timeout) = 3;
		till_stall_check = 3;
	} else if (till_stall_check > 300) {
		ACCESS_ONCE(rcu_cpu_stall_timeout) = 300;
		till_stall_check = 300;
	}
	return till_stall_check * HZ + RCU_STALL_DELAY_DELTA;
}

static int rcu_panic(struct notifier_block *this, unsigned long ev, void *ptr)
{
	rcu_cpu_stall_suppress = 1;
	return NOTIFY_DONE;
}

static struct notifier_block rcu_panic_block = {
	.notifier_call = rcu_panic,
};

static int __init check_cpu_stall_init(void)
{
	atomic_notifier_chain_register(&panic_notifier_list, &rcu_panic_block);
	return 0;
}
early_initcall(check_cpu_stall_init);

#endif  
