 
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/cache.h>
#include <linux/profile.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/percpu.h>
#include <linux/clockchips.h>

#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/processor.h>
#include <asm/tlbflush.h>
#include <asm/ptrace.h>
#include <asm/localtimer.h>
#include <asm/smp_plat.h>

struct secondary_data secondary_data;

struct ipi_data {
	spinlock_t lock;
	unsigned long ipi_count;
	unsigned long bits;
};

static DEFINE_PER_CPU(struct ipi_data, ipi_data) = {
	.lock	= SPIN_LOCK_UNLOCKED,
};

enum ipi_msg_type {
	IPI_TIMER,
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_CALL_FUNC_SINGLE,
	IPI_CPU_STOP,
};

int __cpuinit __cpu_up(unsigned int cpu)
{
	struct cpuinfo_arm *ci = &per_cpu(cpu_data, cpu);
	struct task_struct *idle = ci->idle;
	pgd_t *pgd;
	pmd_t *pmd;
	int ret;

	if (!idle) {
		idle = fork_idle(cpu);
		if (IS_ERR(idle)) {
			printk(KERN_ERR "CPU%u: fork() failed\n", cpu);
			return PTR_ERR(idle);
		}
		ci->idle = idle;
	}

	pgd = pgd_alloc(&init_mm);
	pmd = pmd_offset(pgd + pgd_index(PHYS_OFFSET), PHYS_OFFSET);
	*pmd = __pmd((PHYS_OFFSET & PGDIR_MASK) |
		     PMD_TYPE_SECT | PMD_SECT_AP_WRITE);
	flush_pmd_entry(pmd);

	secondary_data.stack = task_stack_page(idle) + THREAD_START_SP;
	secondary_data.pgdir = virt_to_phys(pgd);
	wmb();

	ret = boot_secondary(cpu, idle);
	if (ret == 0) {
		unsigned long timeout;

		timeout = jiffies + HZ;
		while (time_before(jiffies, timeout)) {
			if (cpu_online(cpu))
				break;

			udelay(10);
			barrier();
		}

		if (!cpu_online(cpu))
			ret = -EIO;
	}

	secondary_data.stack = NULL;
	secondary_data.pgdir = 0;

	*pmd = __pmd(0);
	clean_pmd_entry(pmd);
	pgd_free(&init_mm, pgd);

	if (ret) {
		printk(KERN_CRIT "CPU%u: processor failed to boot\n", cpu);

	}

	return ret;
}

#ifdef CONFIG_HOTPLUG_CPU
 
int __cpu_disable(void)
{
	unsigned int cpu = smp_processor_id();
	struct task_struct *p;
	int ret;

	ret = mach_cpu_disable(cpu);
	if (ret)
		return ret;

	set_cpu_online(cpu, false);

	migrate_irqs();

	local_timer_stop();

	flush_cache_all();
	local_flush_tlb_all();

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (p->mm)
			cpumask_clear_cpu(cpu, mm_cpumask(p->mm));
	}
	read_unlock(&tasklist_lock);

	return 0;
}

void __cpu_die(unsigned int cpu)
{
	if (!platform_cpu_kill(cpu))
		printk("CPU%u: unable to kill\n", cpu);
}

void __ref cpu_die(void)
{
	unsigned int cpu = smp_processor_id();

	local_irq_disable();
	idle_task_exit();

	platform_cpu_die(cpu);

	__asm__("mov	sp, %0\n"
	"	b	secondary_start_kernel"
		:
		: "r" (task_stack_page(current) + THREAD_SIZE - 8));
}
#endif  

asmlinkage void __cpuinit secondary_start_kernel(void)
{
	struct mm_struct *mm = &init_mm;
	unsigned int cpu = smp_processor_id();

	printk("CPU%u: Booted secondary processor\n", cpu);

	atomic_inc(&mm->mm_users);
	atomic_inc(&mm->mm_count);
	current->active_mm = mm;
	cpumask_set_cpu(cpu, mm_cpumask(mm));
	cpu_switch_mm(mm->pgd, mm);
	enter_lazy_tlb(mm, current);
	local_flush_tlb_all();

	cpu_init();
	preempt_disable();

	platform_secondary_init(cpu);

	notify_cpu_starting(cpu);
	local_irq_enable();
	local_fiq_enable();

	percpu_timer_setup();

	calibrate_delay();

	smp_store_cpu_info(cpu);

	set_cpu_online(cpu, true);

	cpu_idle();
}

void __cpuinit smp_store_cpu_info(unsigned int cpuid)
{
	struct cpuinfo_arm *cpu_info = &per_cpu(cpu_data, cpuid);

	cpu_info->loops_per_jiffy = loops_per_jiffy;
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	int cpu;
	unsigned long bogosum = 0;

	for_each_online_cpu(cpu)
		bogosum += per_cpu(cpu_data, cpu).loops_per_jiffy;

	printk(KERN_INFO "SMP: Total of %d processors activated "
	       "(%lu.%02lu BogoMIPS).\n",
	       num_online_cpus(),
	       bogosum / (500000/HZ),
	       (bogosum / (5000/HZ)) % 100);
}

void __init smp_prepare_boot_cpu(void)
{
	unsigned int cpu = smp_processor_id();

	per_cpu(cpu_data, cpu).idle = current;
}

static void send_ipi_message(const struct cpumask *mask, enum ipi_msg_type msg)
{
	unsigned long flags;
	unsigned int cpu;

	local_irq_save(flags);

	for_each_cpu(cpu, mask) {
		struct ipi_data *ipi = &per_cpu(ipi_data, cpu);

		spin_lock(&ipi->lock);
		ipi->bits |= 1 << msg;
		spin_unlock(&ipi->lock);
	}

	smp_cross_call(mask);

	local_irq_restore(flags);
}

void arch_send_call_function_ipi_mask(const struct cpumask *mask)
{
	send_ipi_message(mask, IPI_CALL_FUNC);
}

void arch_send_call_function_single_ipi(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_CALL_FUNC_SINGLE);
}

void show_ipi_list(struct seq_file *p)
{
	unsigned int cpu;

	seq_puts(p, "IPI:");

	for_each_present_cpu(cpu)
		seq_printf(p, " %10lu", per_cpu(ipi_data, cpu).ipi_count);

	seq_putc(p, '\n');
}

void show_local_irqs(struct seq_file *p)
{
	unsigned int cpu;

	seq_printf(p, "LOC: ");

	for_each_present_cpu(cpu)
		seq_printf(p, "%10u ", irq_stat[cpu].local_timer_irqs);

	seq_putc(p, '\n');
}

static DEFINE_PER_CPU(struct clock_event_device, percpu_clockevent);

static void ipi_timer(void)
{
	struct clock_event_device *evt = &__get_cpu_var(percpu_clockevent);
	irq_enter();
	evt->event_handler(evt);
	irq_exit();
}

#if defined(CONFIG_LOCAL_TIMERS) || defined(CONFIG_SYNO_PLX_PORTING)
asmlinkage void __exception do_local_timer(struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	int cpu = smp_processor_id();

	if (local_timer_ack()) {
		irq_stat[cpu].local_timer_irqs++;
		ipi_timer();
	}

	set_irq_regs(old_regs);
}
#endif

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
static void smp_timer_broadcast(const struct cpumask *mask)
{
	send_ipi_message(mask, IPI_TIMER);
}

static void broadcast_timer_set_mode(enum clock_event_mode mode,
	struct clock_event_device *evt)
{
}

static void local_timer_setup(struct clock_event_device *evt)
{
	evt->name	= "dummy_timer";
	evt->features	= CLOCK_EVT_FEAT_ONESHOT |
			  CLOCK_EVT_FEAT_PERIODIC |
			  CLOCK_EVT_FEAT_DUMMY;
	evt->rating	= 400;
	evt->mult	= 1;
	evt->set_mode	= broadcast_timer_set_mode;
	evt->broadcast	= smp_timer_broadcast;

	clockevents_register_device(evt);
}
#endif

void __cpuinit percpu_timer_setup(void)
{
	unsigned int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(percpu_clockevent, cpu);

	evt->cpumask = cpumask_of(cpu);

	local_timer_setup(evt);
}

static DEFINE_SPINLOCK(stop_lock);

static void ipi_cpu_stop(unsigned int cpu)
{
	spin_lock(&stop_lock);
	printk(KERN_CRIT "CPU%u: stopping\n", cpu);
	dump_stack();
	spin_unlock(&stop_lock);

	set_cpu_online(cpu, false);

	local_fiq_disable();
	local_irq_disable();

	while (1)
		cpu_relax();
}

asmlinkage void __exception do_IPI(struct pt_regs *regs)
{
	unsigned int cpu = smp_processor_id();
	struct ipi_data *ipi = &per_cpu(ipi_data, cpu);
	struct pt_regs *old_regs = set_irq_regs(regs);

	ipi->ipi_count++;

	for (;;) {
		unsigned long msgs;

		spin_lock(&ipi->lock);
		msgs = ipi->bits;
		ipi->bits = 0;
		spin_unlock(&ipi->lock);

		if (!msgs)
			break;

		do {
			unsigned nextmsg;

			nextmsg = msgs & -msgs;
			msgs &= ~nextmsg;
			nextmsg = ffz(~nextmsg);

			switch (nextmsg) {
			case IPI_TIMER:
				ipi_timer();
				break;

			case IPI_RESCHEDULE:
				 
				break;

			case IPI_CALL_FUNC:
				generic_smp_call_function_interrupt();
				break;

			case IPI_CALL_FUNC_SINGLE:
				generic_smp_call_function_single_interrupt();
				break;

			case IPI_CPU_STOP:
				ipi_cpu_stop(cpu);
				break;

			default:
				printk(KERN_CRIT "CPU%u: Unknown IPI message 0x%x\n",
				       cpu, nextmsg);
				break;
			}
		} while (msgs);
	}

	set_irq_regs(old_regs);
}

void smp_send_reschedule(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_RESCHEDULE);
}

#ifdef CONFIG_SYNO_PLX_PORTING
 
void smp_send_timer(void)
{
	cpumask_t mask = cpu_online_map;
	cpu_clear(smp_processor_id(), mask);
	send_ipi_message(&mask, IPI_TIMER);
}
#endif

void smp_send_stop(void)
{
	cpumask_t mask = cpu_online_map;
	cpu_clear(smp_processor_id(), mask);
	send_ipi_message(&mask, IPI_CPU_STOP);
}

int setup_profiling_timer(unsigned int multiplier)
{
	return -EINVAL;
}

static void
on_each_cpu_mask(void (*func)(void *), void *info, int wait,
		const struct cpumask *mask)
{
	preempt_disable();

	smp_call_function_many(mask, func, info, wait);
	if (cpumask_test_cpu(smp_processor_id(), mask))
		func(info);

	preempt_enable();
}

#if !defined(CONFIG_ARCH_OX820) || !defined(CONFIG_SMP)  

struct tlb_args {
	struct vm_area_struct *ta_vma;
	unsigned long ta_start;
	unsigned long ta_end;
};

static inline void ipi_flush_tlb_all(void *ignored)
{
	local_flush_tlb_all();
}

static inline void ipi_flush_tlb_mm(void *arg)
{
	struct mm_struct *mm = (struct mm_struct *)arg;

	local_flush_tlb_mm(mm);
}

static inline void ipi_flush_tlb_page(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_page(ta->ta_vma, ta->ta_start);
}

static inline void ipi_flush_tlb_kernel_page(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_kernel_page(ta->ta_start);
}

static inline void ipi_flush_tlb_range(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_range(ta->ta_vma, ta->ta_start, ta->ta_end);
}

static inline void ipi_flush_tlb_kernel_range(void *arg)
{
	struct tlb_args *ta = (struct tlb_args *)arg;

	local_flush_tlb_kernel_range(ta->ta_start, ta->ta_end);
}

void flush_tlb_all(void)
{
	if (tlb_ops_need_broadcast())
		on_each_cpu(ipi_flush_tlb_all, NULL, 1);
	else
		local_flush_tlb_all();
}

void flush_tlb_mm(struct mm_struct *mm)
{
	if (tlb_ops_need_broadcast())
		on_each_cpu_mask(ipi_flush_tlb_mm, mm, 1, mm_cpumask(mm));
	else
		local_flush_tlb_mm(mm);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_vma = vma;
		ta.ta_start = uaddr;
		on_each_cpu_mask(ipi_flush_tlb_page, &ta, 1, mm_cpumask(vma->vm_mm));
	} else
		local_flush_tlb_page(vma, uaddr);
}

void flush_tlb_kernel_page(unsigned long kaddr)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_start = kaddr;
		on_each_cpu(ipi_flush_tlb_kernel_page, &ta, 1);
	} else
		local_flush_tlb_kernel_page(kaddr);
}

void flush_tlb_range(struct vm_area_struct *vma,
                     unsigned long start, unsigned long end)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_vma = vma;
		ta.ta_start = start;
		ta.ta_end = end;
		on_each_cpu_mask(ipi_flush_tlb_range, &ta, 1, mm_cpumask(vma->vm_mm));
	} else
		local_flush_tlb_range(vma, start, end);
}

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	if (tlb_ops_need_broadcast()) {
		struct tlb_args ta;
		ta.ta_start = start;
		ta.ta_end = end;
		on_each_cpu(ipi_flush_tlb_kernel_range, &ta, 1);
	} else
		local_flush_tlb_kernel_range(start, end);
}
#endif  
