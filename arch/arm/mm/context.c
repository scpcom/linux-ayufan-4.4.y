#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/percpu.h>

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>

static DEFINE_RAW_SPINLOCK(cpu_asid_lock);
unsigned int cpu_last_asid = ASID_FIRST_VERSION;
#ifdef CONFIG_SMP
DEFINE_PER_CPU(struct mm_struct *, current_mm);
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)  || defined(MY_DEF_HERE)
#ifdef CONFIG_ARM_LPAE
static void cpu_set_reserved_ttbr0(void)
{
	unsigned long ttbl = __pa(swapper_pg_dir);
	unsigned long ttbh = 0;

#ifdef MY_DEF_HERE
	 
	asm volatile(
#else
	 
	asm(
#endif
	"	mcrr	p15, 0, %0, %1, c2		@ set TTBR0\n"
	:
	: "r" (ttbl), "r" (ttbh));
#ifdef MY_DEF_HERE
	isb();
#endif
}
#else
static void cpu_set_reserved_ttbr0(void)
{
	u32 ttb;

	asm volatile(
	"	mrc	p15, 0, %0, c2, c0, 1		@ read TTBR1\n"
	"	mcr	p15, 0, %0, c2, c0, 0		@ set TTBR0\n"
	: "=r" (ttb));
#ifdef MY_DEF_HERE
	isb();
#endif
}
#endif
#endif

void __init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	mm->context.id = 0;
	raw_spin_lock_init(&mm->context.id_lock);
}

static void flush_context(void)
{
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	cpu_set_reserved_ttbr0();
#else
	 
	asm("mcr	p15, 0, %0, c13, c0, 1\n" : : "r" (0));
#endif
#ifdef MY_DEF_HERE
 
#else
	isb();
#endif
	local_flush_tlb_all();
	if (icache_is_vivt_asid_tagged()) {
		__flush_icache_all();
		dsb();
	}
}

#ifdef CONFIG_SMP

static void set_mm_context(struct mm_struct *mm, unsigned int asid)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&mm->context.id_lock, flags);
	if (likely((mm->context.id ^ cpu_last_asid) >> ASID_BITS)) {
		 
		mm->context.id = asid;
		cpumask_clear(mm_cpumask(mm));
	}
	raw_spin_unlock_irqrestore(&mm->context.id_lock, flags);

	cpumask_set_cpu(smp_processor_id(), mm_cpumask(mm));
}

static void reset_context(void *info)
{
	unsigned int asid;
	unsigned int cpu = smp_processor_id();
	struct mm_struct *mm = per_cpu(current_mm, cpu);

	if (!mm)
		return;

	smp_rmb();
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	asid = cpu_last_asid + cpu;
#else
	asid = cpu_last_asid + cpu + 1;
#endif

	flush_context();
	set_mm_context(mm, asid);

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	cpu_switch_mm(mm->pgd, mm);
#else
	asm("mcr	p15, 0, %0, c13, c0, 1\n" : : "r" (mm->context.id));
	isb();
#endif
}

#else

static inline void set_mm_context(struct mm_struct *mm, unsigned int asid)
{
	mm->context.id = asid;
	cpumask_copy(mm_cpumask(mm), cpumask_of(smp_processor_id()));
}

#endif

void __new_context(struct mm_struct *mm)
{
	unsigned int asid;

	raw_spin_lock(&cpu_asid_lock);
#ifdef CONFIG_SMP
	 
	if (unlikely(((mm->context.id ^ cpu_last_asid) >> ASID_BITS) == 0)) {
		cpumask_set_cpu(smp_processor_id(), mm_cpumask(mm));
		raw_spin_unlock(&cpu_asid_lock);
		return;
	}
#endif
	 
	asid = ++cpu_last_asid;
	if (asid == 0)
		asid = cpu_last_asid = ASID_FIRST_VERSION;

	if (unlikely((asid & ~ASID_MASK) == 0)) {
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
		asid = cpu_last_asid + smp_processor_id();
#else
		asid = cpu_last_asid + smp_processor_id() + 1;
#endif
		flush_context();
#ifdef CONFIG_SMP
		smp_wmb();
		smp_call_function(reset_context, NULL, 1);
#endif
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
		cpu_last_asid += NR_CPUS - 1;
#else
		cpu_last_asid += NR_CPUS;
#endif
	}

	set_mm_context(mm, asid);
	raw_spin_unlock(&cpu_asid_lock);
}
