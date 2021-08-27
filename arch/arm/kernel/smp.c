#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
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
#include <linux/completion.h>

#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/exception.h>
#include <asm/topology.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/processor.h>
#include <asm/sections.h>
#include <asm/tlbflush.h>
#include <asm/ptrace.h>
#include <asm/localtimer.h>
#include <asm/smp_plat.h>


struct secondary_data secondary_data;

enum ipi_msg_type {
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	IPI_WAKE = 0,
#endif
	IPI_TIMER = 2,
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
	int ret;

	if (!idle) {
		idle = fork_idle(cpu);
		if (IS_ERR(idle)) {
			printk(KERN_ERR "CPU%u: fork() failed\n", cpu);
			return PTR_ERR(idle);
		}
		ci->idle = idle;
	} else {
		 
		init_idle(idle, cpu);
	}

	pgd = pgd_alloc(&init_mm);
	if (!pgd)
		return -ENOMEM;

	if (PHYS_OFFSET != PAGE_OFFSET) {
#ifndef CONFIG_HOTPLUG_CPU
		identity_mapping_add(pgd, __pa(__init_begin), __pa(__init_end));
#endif
		identity_mapping_add(pgd, __pa(_stext), __pa(_etext));
		identity_mapping_add(pgd, __pa(_sdata), __pa(_edata));
	}

	secondary_data.stack = task_stack_page(idle) + THREAD_START_SP;
	secondary_data.pgdir = virt_to_phys(pgd);
	secondary_data.swapper_pg_dir = virt_to_phys(swapper_pg_dir);
	__cpuc_flush_dcache_area(&secondary_data, sizeof(secondary_data));
	outer_clean_range(__pa(&secondary_data), __pa(&secondary_data + 1));

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

		if (!cpu_online(cpu)) {
			pr_crit("CPU%u: failed to come online\n", cpu);
			ret = -EIO;
		}
	} else {
		pr_err("CPU%u: failed to boot: %d\n", cpu, ret);
	}

	secondary_data.stack = NULL;
	secondary_data.pgdir = 0;

	if (PHYS_OFFSET != PAGE_OFFSET) {
#ifndef CONFIG_HOTPLUG_CPU
		identity_mapping_del(pgd, __pa(__init_begin), __pa(__init_end));
#endif
		identity_mapping_del(pgd, __pa(_stext), __pa(_etext));
		identity_mapping_del(pgd, __pa(_sdata), __pa(_edata));
	}

	pgd_free(&init_mm, pgd);

	return ret;
}

#ifdef CONFIG_HOTPLUG_CPU
static void percpu_timer_stop(void);

int __cpu_disable(void)
{
	unsigned int cpu = smp_processor_id();
	struct task_struct *p;
	int ret;

	ret = platform_cpu_disable(cpu);
	if (ret)
		return ret;

	set_cpu_online(cpu, false);

	migrate_irqs();

	percpu_timer_stop();

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

static DECLARE_COMPLETION(cpu_died);

void __cpu_die(unsigned int cpu)
{
	if (!wait_for_completion_timeout(&cpu_died, msecs_to_jiffies(5000))) {
		pr_err("CPU%u: cpu didn't die\n", cpu);
		return;
	}
	printk(KERN_NOTICE "CPU%u: shutdown\n", cpu);

	if (!platform_cpu_kill(cpu))
		printk("CPU%u: unable to kill\n", cpu);
}

void __ref cpu_die(void)
{
	unsigned int cpu = smp_processor_id();

	idle_task_exit();

	local_irq_disable();
	mb();

	complete(&cpu_died);

	platform_cpu_die(cpu);

	__asm__("mov	sp, %0\n"
	"	mov	fp, #0\n"
	"	b	secondary_start_kernel"
		:
		: "r" (task_stack_page(current) + THREAD_SIZE - 8));
}
#endif  

int __cpu_logical_map[NR_CPUS];

void __init smp_setup_processor_id(void)
{
	int i;
	u32 cpu = is_smp() ? read_cpuid_mpidr() & 0xff : 0;

	cpu_logical_map(0) = cpu;
	for (i = 1; i < NR_CPUS; ++i)
		cpu_logical_map(i) = i == cpu ? 0 : i;

	printk(KERN_INFO "Booting Linux on physical CPU %d\n", cpu);
}

static void __cpuinit smp_store_cpu_info(unsigned int cpuid)
{
	struct cpuinfo_arm *cpu_info = &per_cpu(cpu_data, cpuid);

	cpu_info->loops_per_jiffy = loops_per_jiffy;

	store_cpu_topology(cpuid);
}

asmlinkage void __cpuinit secondary_start_kernel(void)
{
	struct mm_struct *mm = &init_mm;
	unsigned int cpu;

	
	cpu_switch_mm(mm->pgd, mm);
	enter_lazy_tlb(mm, current);
	local_flush_tlb_all();

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#ifdef CONFIG_MACH_ARMADA_XP_FPGA
	unsigned int cpurev;

	__asm__ __volatile__("mrc p15, 1, %0, c0, c0, 7   @ read CPU ID reg\n"
		: "=r" (cpurev) :: "memory");
#endif
#endif

	
	cpu = smp_processor_id();
	atomic_inc(&mm->mm_count);
	current->active_mm = mm;
	cpumask_set_cpu(cpu, mm_cpumask(mm));

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_MACH_ARMADA_XP_FPGA)
        printk("CPU%u: FPGA Booted secondary processor (ID 0x%04x)\n", cpu, (cpurev & 0xFFFF));
#else
	printk("CPU%u: Booted secondary processor\n", cpu);
#endif

	cpu_init();
	preempt_disable();
	trace_hardirqs_off();

	platform_secondary_init(cpu);

	notify_cpu_starting(cpu);

	calibrate_delay();

	smp_store_cpu_info(cpu);

	set_cpu_online(cpu, true);

	percpu_timer_setup();

	while (!cpu_active(cpu))
		cpu_relax();

	
	local_irq_enable();

#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_MSP)
	local_fiq_enable();
#endif   

	cpu_idle();
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

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int ncores = num_possible_cpus();

	init_cpu_topology();

	smp_store_cpu_info(smp_processor_id());

	if (max_cpus > ncores)
		max_cpus = ncores;
	if (ncores > 1 && max_cpus) {
		 
		percpu_timer_setup();

		
		init_cpu_present(&cpu_possible_map);

		
		platform_smp_prepare_cpus(max_cpus);
	}
}

static void (*smp_cross_call)(const struct cpumask *, unsigned int);

#ifdef CONFIG_SYNO_ALPINE
void arch_send_wakeup_ipi_mask(const struct cpumask *mask)
{
	smp_cross_call(mask, 0);
}
#endif
void __init set_smp_cross_call(void (*fn)(const struct cpumask *, unsigned int))
{
	smp_cross_call = fn;
}

void arch_send_call_function_ipi_mask(const struct cpumask *mask)
{
	smp_cross_call(mask, IPI_CALL_FUNC);
}

void arch_send_call_function_single_ipi(int cpu)
{
	smp_cross_call(cpumask_of(cpu), IPI_CALL_FUNC_SINGLE);
}

static const char *ipi_types[NR_IPI] = {
#define S(x,s)	[x - IPI_TIMER] = s
	S(IPI_TIMER, "Timer broadcast interrupts"),
	S(IPI_RESCHEDULE, "Rescheduling interrupts"),
	S(IPI_CALL_FUNC, "Function call interrupts"),
	S(IPI_CALL_FUNC_SINGLE, "Single function call interrupts"),
	S(IPI_CPU_STOP, "CPU stop interrupts"),
};

void show_ipi_list(struct seq_file *p, int prec)
{
	unsigned int cpu, i;

	for (i = 0; i < NR_IPI; i++) {
		seq_printf(p, "%*s%u: ", prec - 1, "IPI", i);

		for_each_present_cpu(cpu)
			seq_printf(p, "%10u ",
				   __get_irq_stat(cpu, ipi_irqs[i]));

		seq_printf(p, " %s\n", ipi_types[i]);
	}
}

u64 smp_irq_stat_cpu(unsigned int cpu)
{
	u64 sum = 0;
	int i;

	for (i = 0; i < NR_IPI; i++)
		sum += __get_irq_stat(cpu, ipi_irqs[i]);

	return sum;
}


static DEFINE_PER_CPU(struct clock_event_device, percpu_clockevent);

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#if defined(CONFIG_ARCH_ARMADA_XP) && defined(CONFIG_PERF_EVENTS)
void show_local_pmu_irqs(struct seq_file *p, int prec)
{
	 unsigned int cpu;

	 seq_printf(p, "PMU: ");
	 
	 for_each_present_cpu(cpu)
		seq_printf(p, "%10u ", irq_stat[cpu].local_pmu_irqs);
	 
	 seq_putc(p, '\n');
}
#endif
#endif

static void ipi_timer(void)
{
	struct clock_event_device *evt = &__get_cpu_var(percpu_clockevent);
	evt->event_handler(evt);
}

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
static void smp_timer_broadcast(const struct cpumask *mask)
{
	smp_cross_call(mask, IPI_TIMER);
}
#else
#define smp_timer_broadcast	NULL
#endif

static void broadcast_timer_set_mode(enum clock_event_mode mode,
	struct clock_event_device *evt)
{
}

static void __cpuinit broadcast_timer_setup(struct clock_event_device *evt)
{
	evt->name	= "dummy_timer";
	evt->features	= CLOCK_EVT_FEAT_ONESHOT |
			  CLOCK_EVT_FEAT_PERIODIC |
			  CLOCK_EVT_FEAT_DUMMY;
	evt->rating	= 400;
	evt->mult	= 1;
	evt->set_mode	= broadcast_timer_set_mode;

	clockevents_register_device(evt);
}

void __cpuinit percpu_timer_setup(void)
{
	unsigned int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(percpu_clockevent, cpu);

	evt->cpumask = cpumask_of(cpu);
	evt->broadcast = smp_timer_broadcast;

	if (local_timer_setup(evt))
		broadcast_timer_setup(evt);
}

#ifdef CONFIG_HOTPLUG_CPU
 
static void percpu_timer_stop(void)
{
	unsigned int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(percpu_clockevent, cpu);

	local_timer_stop(evt);
}
#endif

static DEFINE_RAW_SPINLOCK(stop_lock);

static void ipi_cpu_stop(unsigned int cpu)
{
	if (system_state == SYSTEM_BOOTING ||
	    system_state == SYSTEM_RUNNING) {
		raw_spin_lock(&stop_lock);
		printk(KERN_CRIT "CPU%u: stopping\n", cpu);
		dump_stack();
		raw_spin_unlock(&stop_lock);
	}

	set_cpu_online(cpu, false);

	local_fiq_disable();
	local_irq_disable();

	while (1)
		cpu_relax();
}

asmlinkage void __exception_irq_entry do_IPI(int ipinr, struct pt_regs *regs)
{
	handle_IPI(ipinr, regs);
}

void handle_IPI(int ipinr, struct pt_regs *regs)
{
	unsigned int cpu = smp_processor_id();
	struct pt_regs *old_regs = set_irq_regs(regs);

	if (ipinr >= IPI_TIMER && ipinr < IPI_TIMER + NR_IPI)
		__inc_irq_stat(cpu, ipi_irqs[ipinr - IPI_TIMER]);

	switch (ipinr) {
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	case IPI_WAKE:
		break;
#endif

	case IPI_TIMER:
		irq_enter();
		ipi_timer();
		irq_exit();
		break;

	case IPI_RESCHEDULE:
		scheduler_ipi();
		break;

	case IPI_CALL_FUNC:
		irq_enter();
		generic_smp_call_function_interrupt();
		irq_exit();
		break;

	case IPI_CALL_FUNC_SINGLE:
		irq_enter();
		generic_smp_call_function_single_interrupt();
		irq_exit();
		break;

	case IPI_CPU_STOP:
		irq_enter();
		ipi_cpu_stop(cpu);
		irq_exit();
		break;

	default:
		printk(KERN_CRIT "CPU%u: Unknown IPI message 0x%x\n",
		       cpu, ipinr);
		break;
	}
	set_irq_regs(old_regs);
}

void smp_send_reschedule(int cpu)
{
	smp_cross_call(cpumask_of(cpu), IPI_RESCHEDULE);
}

void smp_send_stop(void)
{
	unsigned long timeout;

	if (num_online_cpus() > 1) {
		cpumask_t mask = cpu_online_map;
		cpu_clear(smp_processor_id(), mask);

		smp_cross_call(&mask, IPI_CPU_STOP);
	}

	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1)
		pr_warning("SMP: failed to stop secondary CPUs\n");
}

int setup_profiling_timer(unsigned int multiplier)
{
	return -EINVAL;
}
