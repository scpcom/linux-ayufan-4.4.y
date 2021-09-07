 
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "internals.h"

void synchronize_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);
	unsigned int status;

	if (!desc)
		return;

	do {
		unsigned long flags;

		while (desc->status & IRQ_INPROGRESS)
			cpu_relax();

		spin_lock_irqsave(&desc->lock, flags);
		status = desc->status;
		spin_unlock_irqrestore(&desc->lock, flags);

	} while (status & IRQ_INPROGRESS);

	wait_event(desc->wait_for_threads, !atomic_read(&desc->threads_active));
}
EXPORT_SYMBOL(synchronize_irq);

#ifdef CONFIG_SMP
cpumask_var_t irq_default_affinity;

int irq_can_set_affinity(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (CHECK_IRQ_PER_CPU(desc->status) || !desc->chip ||
	    !desc->chip->set_affinity)
		return 0;

	return 1;
}

void irq_set_thread_affinity(struct irq_desc *desc)
{
	struct irqaction *action = desc->action;

	while (action) {
		if (action->thread)
			set_bit(IRQTF_AFFINITY, &action->thread_flags);
		action = action->next;
	}
}

int irq_set_affinity(unsigned int irq, const struct cpumask *cpumask)
{
	struct irq_desc *desc = irq_to_desc(irq);
	unsigned long flags;

	if (!desc->chip->set_affinity)
		return -EINVAL;

	spin_lock_irqsave(&desc->lock, flags);

#ifdef CONFIG_GENERIC_PENDING_IRQ
	if (desc->status & IRQ_MOVE_PCNTXT) {
		if (!desc->chip->set_affinity(irq, cpumask)) {
			cpumask_copy(desc->affinity, cpumask);
			irq_set_thread_affinity(desc);
		}
	}
	else {
		desc->status |= IRQ_MOVE_PENDING;
		cpumask_copy(desc->pending_mask, cpumask);
	}
#else
	if (!desc->chip->set_affinity(irq, cpumask)) {
		cpumask_copy(desc->affinity, cpumask);
		irq_set_thread_affinity(desc);
	}
#endif
	desc->status |= IRQ_AFFINITY_SET;
	spin_unlock_irqrestore(&desc->lock, flags);
	return 0;
}
#ifdef CONFIG_SYNO_QORIQ
EXPORT_SYMBOL(irq_set_affinity);
#endif

#ifndef CONFIG_AUTO_IRQ_AFFINITY
 
static int setup_affinity(unsigned int irq, struct irq_desc *desc)
{
	if (!irq_can_set_affinity(irq))
		return 0;

	if (desc->status & (IRQ_AFFINITY_SET | IRQ_NO_BALANCING)) {
		if (cpumask_any_and(desc->affinity, cpu_online_mask)
		    < nr_cpu_ids)
			goto set_affinity;
		else
			desc->status &= ~IRQ_AFFINITY_SET;
	}

	cpumask_and(desc->affinity, cpu_online_mask, irq_default_affinity);
set_affinity:
	desc->chip->set_affinity(irq, desc->affinity);

	return 0;
}
#else
static inline int setup_affinity(unsigned int irq, struct irq_desc *d)
{
	return irq_select_affinity(irq);
}
#endif

int irq_select_affinity_usr(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&desc->lock, flags);
	ret = setup_affinity(irq, desc);
	if (!ret)
		irq_set_thread_affinity(desc);
	spin_unlock_irqrestore(&desc->lock, flags);

	return ret;
}

#else
static inline int setup_affinity(unsigned int irq, struct irq_desc *desc)
{
	return 0;
}
#endif

void __disable_irq(struct irq_desc *desc, unsigned int irq, bool suspend)
{
	if (suspend) {
		if (!desc->action || (desc->action->flags & IRQF_TIMER))
			return;
		desc->status |= IRQ_SUSPENDED;
	}

	if (!desc->depth++) {
		desc->status |= IRQ_DISABLED;
		desc->chip->disable(irq);
	}
}

void disable_irq_nosync(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);
	unsigned long flags;

	if (!desc)
		return;

	chip_bus_lock(irq, desc);
	spin_lock_irqsave(&desc->lock, flags);
	__disable_irq(desc, irq, false);
	spin_unlock_irqrestore(&desc->lock, flags);
	chip_bus_sync_unlock(irq, desc);
}
EXPORT_SYMBOL(disable_irq_nosync);

void disable_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc)
		return;

	disable_irq_nosync(irq);
	if (desc->action)
		synchronize_irq(irq);
}
EXPORT_SYMBOL(disable_irq);

void __enable_irq(struct irq_desc *desc, unsigned int irq, bool resume)
{
	if (resume)
		desc->status &= ~IRQ_SUSPENDED;

	switch (desc->depth) {
	case 0:
 err_out:
		WARN(1, KERN_WARNING "Unbalanced enable for IRQ %d\n", irq);
		break;
	case 1: {
		unsigned int status = desc->status & ~IRQ_DISABLED;

		if (desc->status & IRQ_SUSPENDED)
			goto err_out;
		 
		desc->status = status | IRQ_NOPROBE;
		check_irq_resend(desc, irq);
		 
	}
	default:
		desc->depth--;
	}
}

void enable_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);
	unsigned long flags;

	if (!desc)
		return;

	chip_bus_lock(irq, desc);
	spin_lock_irqsave(&desc->lock, flags);
	__enable_irq(desc, irq, false);
	spin_unlock_irqrestore(&desc->lock, flags);
	chip_bus_sync_unlock(irq, desc);
}
EXPORT_SYMBOL(enable_irq);

static int set_irq_wake_real(unsigned int irq, unsigned int on)
{
	struct irq_desc *desc = irq_to_desc(irq);
	int ret = -ENXIO;

	if (desc->chip->set_wake)
		ret = desc->chip->set_wake(irq, on);

	return ret;
}

int set_irq_wake(unsigned int irq, unsigned int on)
{
	struct irq_desc *desc = irq_to_desc(irq);
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&desc->lock, flags);
	if (on) {
		if (desc->wake_depth++ == 0) {
			ret = set_irq_wake_real(irq, on);
			if (ret)
				desc->wake_depth = 0;
			else
				desc->status |= IRQ_WAKEUP;
		}
	} else {
		if (desc->wake_depth == 0) {
			WARN(1, "Unbalanced IRQ %d wake disable\n", irq);
		} else if (--desc->wake_depth == 0) {
			ret = set_irq_wake_real(irq, on);
			if (ret)
				desc->wake_depth = 1;
			else
				desc->status &= ~IRQ_WAKEUP;
		}
	}

	spin_unlock_irqrestore(&desc->lock, flags);
	return ret;
}
EXPORT_SYMBOL(set_irq_wake);

int can_request_irq(unsigned int irq, unsigned long irqflags)
{
	struct irq_desc *desc = irq_to_desc(irq);
	struct irqaction *action;

	if (!desc)
		return 0;

	if (desc->status & IRQ_NOREQUEST)
		return 0;

	action = desc->action;
	if (action)
		if (irqflags & action->flags & IRQF_SHARED)
			action = NULL;

	return !action;
}

void compat_irq_chip_set_default_handler(struct irq_desc *desc)
{
	 
	if (desc->handle_irq == &handle_bad_irq)
		desc->handle_irq = NULL;
}

int __irq_set_trigger(struct irq_desc *desc, unsigned int irq,
		unsigned long flags)
{
	int ret;
	struct irq_chip *chip = desc->chip;

	if (!chip || !chip->set_type) {
		 
		pr_debug("No set_type function for IRQ %d (%s)\n", irq,
				chip ? (chip->name ? : "unknown") : "unknown");
		return 0;
	}

	ret = chip->set_type(irq, flags);

	if (ret)
		pr_err("setting trigger mode %d for irq %u failed (%pF)\n",
				(int)flags, irq, chip->set_type);
	else {
		if (flags & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH))
			flags |= IRQ_LEVEL;
		 
		desc->status &= ~(IRQ_LEVEL | IRQ_TYPE_SENSE_MASK);
		desc->status |= flags;
	}

	return ret;
}

static irqreturn_t irq_default_primary_handler(int irq, void *dev_id)
{
	return IRQ_WAKE_THREAD;
}

static irqreturn_t irq_nested_primary_handler(int irq, void *dev_id)
{
	WARN(1, "Primary handler called for nested irq %d\n", irq);
	return IRQ_NONE;
}

static int irq_wait_for_interrupt(struct irqaction *action)
{
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);

		if (test_and_clear_bit(IRQTF_RUNTHREAD,
				       &action->thread_flags)) {
			__set_current_state(TASK_RUNNING);
			return 0;
		}
		schedule();
	}
	return -1;
}

static void irq_finalize_oneshot(unsigned int irq, struct irq_desc *desc)
{
	chip_bus_lock(irq, desc);
	spin_lock_irq(&desc->lock);
	if (!(desc->status & IRQ_DISABLED) && (desc->status & IRQ_MASKED)) {
		desc->status &= ~IRQ_MASKED;
		desc->chip->unmask(irq);
	}
	spin_unlock_irq(&desc->lock);
	chip_bus_sync_unlock(irq, desc);
}

#ifdef CONFIG_SMP
 
static void
irq_thread_check_affinity(struct irq_desc *desc, struct irqaction *action)
{
	cpumask_var_t mask;

	if (!test_and_clear_bit(IRQTF_AFFINITY, &action->thread_flags))
		return;

	if (!alloc_cpumask_var(&mask, GFP_KERNEL)) {
		set_bit(IRQTF_AFFINITY, &action->thread_flags);
		return;
	}

	spin_lock_irq(&desc->lock);
	cpumask_copy(mask, desc->affinity);
	spin_unlock_irq(&desc->lock);

	set_cpus_allowed_ptr(current, mask);
	free_cpumask_var(mask);
}
#else
static inline void
irq_thread_check_affinity(struct irq_desc *desc, struct irqaction *action) { }
#endif

static int irq_thread(void *data)
{
	struct sched_param param = { .sched_priority = MAX_USER_RT_PRIO/2, };
	struct irqaction *action = data;
	struct irq_desc *desc = irq_to_desc(action->irq);
	int wake, oneshot = desc->status & IRQ_ONESHOT;

	sched_setscheduler(current, SCHED_FIFO, &param);
	current->irqaction = action;

	while (!irq_wait_for_interrupt(action)) {

		irq_thread_check_affinity(desc, action);

		atomic_inc(&desc->threads_active);

		spin_lock_irq(&desc->lock);
		if (unlikely(desc->status & IRQ_DISABLED)) {
			 
			desc->status |= IRQ_PENDING;
			spin_unlock_irq(&desc->lock);
		} else {
			spin_unlock_irq(&desc->lock);

			action->thread_fn(action->irq, action->dev_id);

			if (oneshot)
				irq_finalize_oneshot(action->irq, desc);
		}

		wake = atomic_dec_and_test(&desc->threads_active);

		if (wake && waitqueue_active(&desc->wait_for_threads))
			wake_up(&desc->wait_for_threads);
	}

	current->irqaction = NULL;
	return 0;
}

void exit_irq_thread(void)
{
	struct task_struct *tsk = current;

	if (!tsk->irqaction)
		return;

	printk(KERN_ERR
	       "exiting task \"%s\" (%d) is an active IRQ thread (irq %d)\n",
	       tsk->comm ? tsk->comm : "", tsk->pid, tsk->irqaction->irq);

	set_bit(IRQTF_DIED, &tsk->irqaction->flags);
}

static int
__setup_irq(unsigned int irq, struct irq_desc *desc, struct irqaction *new)
{
	struct irqaction *old, **old_ptr;
	const char *old_name = NULL;
	unsigned long flags;
	int nested, shared = 0;
	int ret;

	if (!desc)
		return -EINVAL;

	if (desc->chip == &no_irq_chip)
		return -ENOSYS;
	 
	if (new->flags & IRQF_SAMPLE_RANDOM) {
		 
		rand_initialize_irq(irq);
	}

	if ((new->flags & IRQF_ONESHOT) && (new->flags & IRQF_SHARED))
		return -EINVAL;

	nested = desc->status & IRQ_NESTED_THREAD;
	if (nested) {
		if (!new->thread_fn)
			return -EINVAL;
		 
		new->handler = irq_nested_primary_handler;
	}

	if (new->thread_fn && !nested) {
		struct task_struct *t;

		t = kthread_create(irq_thread, new, "irq/%d-%s", irq,
				   new->name);
		if (IS_ERR(t))
			return PTR_ERR(t);
		 
		get_task_struct(t);
		new->thread = t;
	}

	spin_lock_irqsave(&desc->lock, flags);
	old_ptr = &desc->action;
	old = *old_ptr;
	if (old) {
		 
		if (!((old->flags & new->flags) & IRQF_SHARED) ||
		    ((old->flags ^ new->flags) & IRQF_TRIGGER_MASK)) {
			old_name = old->name;
			goto mismatch;
		}

#if defined(CONFIG_IRQ_PER_CPU)
		 
		if ((old->flags & IRQF_PERCPU) !=
		    (new->flags & IRQF_PERCPU))
			goto mismatch;
#endif

		do {
			old_ptr = &old->next;
			old = *old_ptr;
		} while (old);
		shared = 1;
	}

	if (!shared) {
		irq_chip_set_defaults(desc->chip);

		init_waitqueue_head(&desc->wait_for_threads);

		if (new->flags & IRQF_TRIGGER_MASK) {
			ret = __irq_set_trigger(desc, irq,
					new->flags & IRQF_TRIGGER_MASK);

			if (ret)
				goto out_thread;
		} else
			compat_irq_chip_set_default_handler(desc);
#if defined(CONFIG_IRQ_PER_CPU)
		if (new->flags & IRQF_PERCPU)
			desc->status |= IRQ_PER_CPU;
#endif

		desc->status &= ~(IRQ_AUTODETECT | IRQ_WAITING | IRQ_ONESHOT |
				  IRQ_INPROGRESS | IRQ_SPURIOUS_DISABLED);

		if (new->flags & IRQF_ONESHOT)
			desc->status |= IRQ_ONESHOT;

		if (desc->msi_desc)
			new->flags |= IRQF_DISABLED;

		if (!(desc->status & IRQ_NOAUTOEN)) {
			desc->depth = 0;
			desc->status &= ~IRQ_DISABLED;
			desc->chip->startup(irq);
		} else
			 
			desc->depth = 1;

		if (new->flags & IRQF_NOBALANCING)
			desc->status |= IRQ_NO_BALANCING;

		setup_affinity(irq, desc);

	} else if ((new->flags & IRQF_TRIGGER_MASK)
			&& (new->flags & IRQF_TRIGGER_MASK)
				!= (desc->status & IRQ_TYPE_SENSE_MASK)) {
		 
		pr_warning("IRQ %d uses trigger mode %d; requested %d\n",
				irq, (int)(desc->status & IRQ_TYPE_SENSE_MASK),
				(int)(new->flags & IRQF_TRIGGER_MASK));
	}

	new->irq = irq;
	*old_ptr = new;

	desc->irq_count = 0;
	desc->irqs_unhandled = 0;

	if (shared && (desc->status & IRQ_SPURIOUS_DISABLED)) {
		desc->status &= ~IRQ_SPURIOUS_DISABLED;
		__enable_irq(desc, irq, false);
	}

	spin_unlock_irqrestore(&desc->lock, flags);

	if (new->thread)
		wake_up_process(new->thread);

	register_irq_proc(irq, desc);
	new->dir = NULL;
	register_handler_proc(irq, new);

	return 0;

mismatch:
#ifdef CONFIG_DEBUG_SHIRQ
	if (!(new->flags & IRQF_PROBE_SHARED)) {
		printk(KERN_ERR "IRQ handler type mismatch for IRQ %d\n", irq);
		if (old_name)
			printk(KERN_ERR "current handler: %s\n", old_name);
		dump_stack();
	}
#endif
	ret = -EBUSY;

out_thread:
	spin_unlock_irqrestore(&desc->lock, flags);
	if (new->thread) {
		struct task_struct *t = new->thread;

		new->thread = NULL;
		if (likely(!test_bit(IRQTF_DIED, &new->thread_flags)))
			kthread_stop(t);
		put_task_struct(t);
	}
	return ret;
}

int setup_irq(unsigned int irq, struct irqaction *act)
{
	struct irq_desc *desc = irq_to_desc(irq);

	return __setup_irq(irq, desc, act);
}
EXPORT_SYMBOL_GPL(setup_irq);

static struct irqaction *__free_irq(unsigned int irq, void *dev_id)
{
	struct irq_desc *desc = irq_to_desc(irq);
	struct irqaction *action, **action_ptr;
	unsigned long flags;

	WARN(in_interrupt(), "Trying to free IRQ %d from IRQ context!\n", irq);

	if (!desc)
		return NULL;

	spin_lock_irqsave(&desc->lock, flags);

	action_ptr = &desc->action;
	for (;;) {
		action = *action_ptr;

		if (!action) {
			WARN(1, "Trying to free already-free IRQ %d\n", irq);
			spin_unlock_irqrestore(&desc->lock, flags);

			return NULL;
		}

		if (action->dev_id == dev_id)
			break;
		action_ptr = &action->next;
	}

	*action_ptr = action->next;

#ifdef CONFIG_IRQ_RELEASE_METHOD
	if (desc->chip->release)
		desc->chip->release(irq, dev_id);
#endif

	if (!desc->action) {
		desc->status |= IRQ_DISABLED;
		if (desc->chip->shutdown)
			desc->chip->shutdown(irq);
		else
			desc->chip->disable(irq);
	}

	spin_unlock_irqrestore(&desc->lock, flags);

	unregister_handler_proc(irq, action);

	synchronize_irq(irq);

#ifdef CONFIG_DEBUG_SHIRQ
	 
	if (action->flags & IRQF_SHARED) {
		local_irq_save(flags);
		action->handler(irq, dev_id);
		local_irq_restore(flags);
	}
#endif

	if (action->thread) {
		if (!test_bit(IRQTF_DIED, &action->thread_flags))
			kthread_stop(action->thread);
		put_task_struct(action->thread);
	}

	return action;
}

void remove_irq(unsigned int irq, struct irqaction *act)
{
	__free_irq(irq, act->dev_id);
}
EXPORT_SYMBOL_GPL(remove_irq);

void free_irq(unsigned int irq, void *dev_id)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc)
		return;

	chip_bus_lock(irq, desc);
	kfree(__free_irq(irq, dev_id));
	chip_bus_sync_unlock(irq, desc);
}
EXPORT_SYMBOL(free_irq);

int request_threaded_irq(unsigned int irq, irq_handler_t handler,
			 irq_handler_t thread_fn, unsigned long irqflags,
			 const char *devname, void *dev_id)
{
	struct irqaction *action;
	struct irq_desc *desc;
	int retval;

	if ((irqflags & (IRQF_SHARED|IRQF_DISABLED)) ==
					(IRQF_SHARED|IRQF_DISABLED)) {
		pr_warning(
		  "IRQ %d/%s: IRQF_DISABLED is not guaranteed on shared IRQs\n",
			irq, devname);
	}

#ifdef CONFIG_LOCKDEP
	 
	irqflags |= IRQF_DISABLED;
#endif
	 
	if ((irqflags & IRQF_SHARED) && !dev_id)
		return -EINVAL;

	desc = irq_to_desc(irq);
	if (!desc)
		return -EINVAL;

	if (desc->status & IRQ_NOREQUEST)
		return -EINVAL;

	if (!handler) {
		if (!thread_fn)
			return -EINVAL;
		handler = irq_default_primary_handler;
	}

	action = kzalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->handler = handler;
	action->thread_fn = thread_fn;
	action->flags = irqflags;
	action->name = devname;
	action->dev_id = dev_id;

	chip_bus_lock(irq, desc);
	retval = __setup_irq(irq, desc, action);
	chip_bus_sync_unlock(irq, desc);

	if (retval)
		kfree(action);

#ifdef CONFIG_DEBUG_SHIRQ
	if (irqflags & IRQF_SHARED) {
		 
		unsigned long flags;

		disable_irq(irq);
		local_irq_save(flags);

		handler(irq, dev_id);

		local_irq_restore(flags);
		enable_irq(irq);
	}
#endif
	return retval;
}
EXPORT_SYMBOL(request_threaded_irq);
