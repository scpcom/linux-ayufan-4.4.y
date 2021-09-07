#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/rcupdate.h>
#include <linux/rculist.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/cpu.h>

static DEFINE_PER_CPU(struct call_single_queue, call_single_queue);

static struct {
	struct list_head	queue;
	spinlock_t		lock;
} call_function __cacheline_aligned_in_smp =
	{
		.queue		= LIST_HEAD_INIT(call_function.queue),
		.lock		= __SPIN_LOCK_UNLOCKED(call_function.lock),
	};

enum {
	CSD_FLAG_LOCK		= 0x01,
};

struct call_function_data {
	struct call_single_data	csd;
	atomic_t		refs;
	cpumask_var_t		cpumask;
};

struct call_single_queue {
	struct list_head	list;
	spinlock_t		lock;
};

static DEFINE_PER_CPU(struct call_function_data, cfd_data);

static int
hotplug_cfd(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	long cpu = (long)hcpu;
	struct call_function_data *cfd = &per_cpu(cfd_data, cpu);

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		if (!zalloc_cpumask_var_node(&cfd->cpumask, GFP_KERNEL,
				cpu_to_node(cpu)))
			return NOTIFY_BAD;
		break;

#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:

	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		free_cpumask_var(cfd->cpumask);
		break;
#endif
	};

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata hotplug_cfd_notifier = {
	.notifier_call		= hotplug_cfd,
};

static int __cpuinit init_call_single_data(void)
{
	void *cpu = (void *)(long)smp_processor_id();
	int i;

	for_each_possible_cpu(i) {
		struct call_single_queue *q = &per_cpu(call_single_queue, i);

		spin_lock_init(&q->lock);
		INIT_LIST_HEAD(&q->list);
	}

	hotplug_cfd(&hotplug_cfd_notifier, CPU_UP_PREPARE, cpu);
	register_cpu_notifier(&hotplug_cfd_notifier);

	return 0;
}
early_initcall(init_call_single_data);

static void csd_lock_wait(struct call_single_data *data)
{
	while (data->flags & CSD_FLAG_LOCK)
		cpu_relax();
}

static void csd_lock(struct call_single_data *data)
{
	csd_lock_wait(data);
	data->flags = CSD_FLAG_LOCK;

	smp_mb();
}

static void csd_unlock(struct call_single_data *data)
{
	WARN_ON(!(data->flags & CSD_FLAG_LOCK));

	smp_mb();

	data->flags &= ~CSD_FLAG_LOCK;
}

static
void generic_exec_single(int cpu, struct call_single_data *data, int wait)
{
	struct call_single_queue *dst = &per_cpu(call_single_queue, cpu);
	unsigned long flags;
	int ipi;

	spin_lock_irqsave(&dst->lock, flags);
	ipi = list_empty(&dst->list);
	list_add_tail(&data->list, &dst->list);
	spin_unlock_irqrestore(&dst->lock, flags);

	if (ipi)
		arch_send_call_function_single_ipi(cpu);

	if (wait)
		csd_lock_wait(data);
}

void generic_smp_call_function_interrupt(void)
{
	struct call_function_data *data;
	int cpu = get_cpu();

	WARN_ON_ONCE(!cpu_online(cpu));

	smp_mb();

	list_for_each_entry_rcu(data, &call_function.queue, csd.list) {
		int refs;

		if (!cpumask_test_and_clear_cpu(cpu, data->cpumask))
			continue;

		data->csd.func(data->csd.info);

		refs = atomic_dec_return(&data->refs);
		WARN_ON(refs < 0);
		if (!refs) {
			spin_lock(&call_function.lock);
			list_del_rcu(&data->csd.list);
			spin_unlock(&call_function.lock);
		}

		if (refs)
			continue;

		csd_unlock(&data->csd);
	}

	put_cpu();
}

void generic_smp_call_function_single_interrupt(void)
{
	struct call_single_queue *q = &__get_cpu_var(call_single_queue);
	unsigned int data_flags;
	LIST_HEAD(list);

	WARN_ON_ONCE(!cpu_online(smp_processor_id()));

	spin_lock(&q->lock);
	list_replace_init(&q->list, &list);
	spin_unlock(&q->lock);

	while (!list_empty(&list)) {
		struct call_single_data *data;

		data = list_entry(list.next, struct call_single_data, list);
		list_del(&data->list);

		data_flags = data->flags;

		data->func(data->info);

		if (data_flags & CSD_FLAG_LOCK)
			csd_unlock(data);
	}
}

static DEFINE_PER_CPU(struct call_single_data, csd_data);

int smp_call_function_single(int cpu, void (*func) (void *info), void *info,
			     int wait)
{
	struct call_single_data d = {
		.flags = 0,
	};
	unsigned long flags;
	int this_cpu;
	int err = 0;

	this_cpu = get_cpu();

	WARN_ON_ONCE(cpu_online(this_cpu) && irqs_disabled()
		     && !oops_in_progress);

	if (cpu == this_cpu) {
		local_irq_save(flags);
		func(info);
		local_irq_restore(flags);
	} else {
		if ((unsigned)cpu < nr_cpu_ids && cpu_online(cpu)) {
			struct call_single_data *data = &d;

			if (!wait)
				data = &__get_cpu_var(csd_data);

			csd_lock(data);

			data->func = func;
			data->info = info;
			generic_exec_single(cpu, data, wait);
		} else {
			err = -ENXIO;	 
		}
	}

	put_cpu();

	return err;
}
EXPORT_SYMBOL(smp_call_function_single);

void __smp_call_function_single(int cpu, struct call_single_data *data,
				int wait)
{
	csd_lock(data);

	WARN_ON_ONCE(cpu_online(smp_processor_id()) && wait && irqs_disabled()
		     && !oops_in_progress);

	generic_exec_single(cpu, data, wait);
}

void smp_call_function_many(const struct cpumask *mask,
			    void (*func)(void *), void *info, bool wait)
{
	struct call_function_data *data;
	unsigned long flags;
	int cpu, next_cpu, this_cpu = smp_processor_id();

#ifndef MY_DEF_HERE
	WARN_ON_ONCE(cpu_online(this_cpu) && irqs_disabled()
		     && !oops_in_progress);
#endif

	cpu = cpumask_first_and(mask, cpu_online_mask);
	if (cpu == this_cpu)
		cpu = cpumask_next_and(cpu, mask, cpu_online_mask);

	if (cpu >= nr_cpu_ids)
		return;

	next_cpu = cpumask_next_and(cpu, mask, cpu_online_mask);
	if (next_cpu == this_cpu)
		next_cpu = cpumask_next_and(next_cpu, mask, cpu_online_mask);

	if (next_cpu >= nr_cpu_ids) {
		smp_call_function_single(cpu, func, info, wait);
		return;
	}

	data = &__get_cpu_var(cfd_data);
	csd_lock(&data->csd);

	data->csd.func = func;
	data->csd.info = info;
	cpumask_and(data->cpumask, mask, cpu_online_mask);
	cpumask_clear_cpu(this_cpu, data->cpumask);
	atomic_set(&data->refs, cpumask_weight(data->cpumask));

	spin_lock_irqsave(&call_function.lock, flags);
	 
	list_add_rcu(&data->csd.list, &call_function.queue);
	spin_unlock_irqrestore(&call_function.lock, flags);

	smp_mb();

	arch_send_call_function_ipi_mask(data->cpumask);

	if (wait)
		csd_lock_wait(&data->csd);
}
EXPORT_SYMBOL(smp_call_function_many);

int smp_call_function(void (*func)(void *), void *info, int wait)
{
	preempt_disable();
	smp_call_function_many(cpu_online_mask, func, info, wait);
	preempt_enable();

	return 0;
}
EXPORT_SYMBOL(smp_call_function);

void ipi_call_lock(void)
{
	spin_lock(&call_function.lock);
}

void ipi_call_unlock(void)
{
	spin_unlock(&call_function.lock);
}

void ipi_call_lock_irq(void)
{
	spin_lock_irq(&call_function.lock);
}

void ipi_call_unlock_irq(void)
{
	spin_unlock_irq(&call_function.lock);
}
