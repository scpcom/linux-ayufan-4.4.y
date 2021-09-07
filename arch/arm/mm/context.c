 
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#ifdef CONFIG_SYNO_PLX_PORTING
#include <linux/smp.h>
#include <linux/percpu.h>
#endif

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>

static DEFINE_SPINLOCK(cpu_asid_lock);
unsigned int cpu_last_asid = ASID_FIRST_VERSION;
#ifdef CONFIG_SYNO_PLX_PORTING
#ifdef CONFIG_SMP
DEFINE_PER_CPU(struct mm_struct *, current_mm);
#endif
#endif

void __init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	mm->context.id = 0;
#ifdef CONFIG_SYNO_PLX_PORTING
	spin_lock_init(&mm->context.id_lock);
#endif
}

#ifdef CONFIG_SYNO_PLX_PORTING
static void flush_context(void)
{
	 
	asm("mcr	p15, 0, %0, c13, c0, 1\n" : : "r" (0));
	isb();
	local_flush_tlb_all();
	if (icache_is_vivt_asid_tagged()) {
		__flush_icache_all();
		dsb();
	}
}

#ifdef CONFIG_SMP

static void set_mm_context(struct mm_struct *mm, unsigned int asid)
{
	 
	spin_lock(&mm->context.id_lock);
	if (likely((mm->context.id ^ cpu_last_asid) >> ASID_BITS)) {
		 
		mm->context.id = asid;
		cpus_clear(mm->cpu_vm_mask);
	}
	spin_unlock(&mm->context.id_lock);

	cpu_set(smp_processor_id(), mm->cpu_vm_mask);
}

static void reset_context(void *info)
{
	unsigned int asid;
	unsigned int cpu = smp_processor_id();
	struct mm_struct *mm = per_cpu(current_mm, cpu);

	if (!mm)
		return;

	smp_rmb();
	asid = cpu_last_asid + cpu + 1;

	flush_context();
	set_mm_context(mm, asid);

	asm("mcr	p15, 0, %0, c13, c0, 1\n" : : "r" (mm->context.id));
}

#else

static inline void set_mm_context(struct mm_struct *mm, unsigned int asid)
{
	mm->context.id = asid;
	mm->cpu_vm_mask = cpumask_of_cpu(smp_processor_id());
}

#endif
#endif  

void __new_context(struct mm_struct *mm)
{
	unsigned int asid;

	spin_lock(&cpu_asid_lock);
#ifdef CONFIG_SYNO_PLX_PORTING
#ifdef CONFIG_SMP
	 
	if (unlikely(((mm->context.id ^ cpu_last_asid) >> ASID_BITS) == 0)) {
		cpu_set(smp_processor_id(), mm->cpu_vm_mask);
		spin_unlock(&cpu_asid_lock);
		return;
	}
#endif
	 
#endif
	asid = ++cpu_last_asid;
	if (asid == 0)
		asid = cpu_last_asid = ASID_FIRST_VERSION;

	if (unlikely((asid & ~ASID_MASK) == 0)) {
#ifdef CONFIG_SYNO_PLX_PORTING
 		asid = cpu_last_asid + smp_processor_id() + 1;
 		flush_context();
#ifdef CONFIG_SMP
 		smp_wmb();
 		smp_call_function(reset_context, NULL, 1);
#endif
 		cpu_last_asid += NR_CPUS;
#else  
		asid = ++cpu_last_asid;
		 
		asm("mcr	p15, 0, %0, c13, c0, 1	@ set reserved context ID\n"
		    :
		    : "r" (0));
		isb();
		flush_tlb_all();
		if (icache_is_vivt_asid_tagged()) {
			__flush_icache_all();
			dsb();
		}
#endif  
	}
#ifdef CONFIG_SYNO_PLX_PORTING
 	set_mm_context(mm, asid);
 	spin_unlock(&cpu_asid_lock);
#else  
	spin_unlock(&cpu_asid_lock);

	cpumask_copy(mm_cpumask(mm), cpumask_of(smp_processor_id()));
	mm->context.id = asid;
#endif  
}
