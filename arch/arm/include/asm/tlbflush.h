#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _ASMARM_TLBFLUSH_H
#define _ASMARM_TLBFLUSH_H

#ifdef CONFIG_MMU

#include <asm/glue.h>

#define TLB_V3_PAGE	(1 << 0)
#define TLB_V4_U_PAGE	(1 << 1)
#define TLB_V4_D_PAGE	(1 << 2)
#define TLB_V4_I_PAGE	(1 << 3)
#define TLB_V6_U_PAGE	(1 << 4)
#define TLB_V6_D_PAGE	(1 << 5)
#define TLB_V6_I_PAGE	(1 << 6)

#define TLB_V3_FULL	(1 << 8)
#define TLB_V4_U_FULL	(1 << 9)
#define TLB_V4_D_FULL	(1 << 10)
#define TLB_V4_I_FULL	(1 << 11)
#define TLB_V6_U_FULL	(1 << 12)
#define TLB_V6_D_FULL	(1 << 13)
#define TLB_V6_I_FULL	(1 << 14)

#define TLB_V6_U_ASID	(1 << 16)
#define TLB_V6_D_ASID	(1 << 17)
#define TLB_V6_I_ASID	(1 << 18)

#define TLB_V7_UIS_PAGE	(1 << 19)
#define TLB_V7_UIS_FULL (1 << 20)
#define TLB_V7_UIS_ASID (1 << 21)

#define TLB_BARRIER	(1 << 28)
#define TLB_L2CLEAN_FR	(1 << 29)		 
#define TLB_DCLEAN	(1 << 30)
#define TLB_WB		(1 << 31)

#undef _TLB
#undef MULTI_TLB

#ifdef CONFIG_SMP_ON_UP
#define MULTI_TLB 1
#endif

#define v3_tlb_flags	(TLB_V3_FULL | TLB_V3_PAGE)

#ifdef CONFIG_CPU_TLB_V3
# define v3_possible_flags	v3_tlb_flags
# define v3_always_flags	v3_tlb_flags
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v3
# endif
#else
# define v3_possible_flags	0
# define v3_always_flags	(-1UL)
#endif

#define v4_tlb_flags	(TLB_V4_U_FULL | TLB_V4_U_PAGE)

#ifdef CONFIG_CPU_TLB_V4WT
# define v4_possible_flags	v4_tlb_flags
# define v4_always_flags	v4_tlb_flags
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v4
# endif
#else
# define v4_possible_flags	0
# define v4_always_flags	(-1UL)
#endif

#define fa_tlb_flags	(TLB_WB | TLB_DCLEAN | TLB_BARRIER | \
			 TLB_V4_U_FULL | TLB_V4_U_PAGE)

#ifdef CONFIG_CPU_TLB_FA
# define fa_possible_flags	fa_tlb_flags
# define fa_always_flags	fa_tlb_flags
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB fa
# endif
#else
# define fa_possible_flags	0
# define fa_always_flags	(-1UL)
#endif

#define v4wbi_tlb_flags	(TLB_WB | TLB_DCLEAN | \
			 TLB_V4_I_FULL | TLB_V4_D_FULL | \
			 TLB_V4_I_PAGE | TLB_V4_D_PAGE)

#ifdef CONFIG_CPU_TLB_V4WBI
# define v4wbi_possible_flags	v4wbi_tlb_flags
# define v4wbi_always_flags	v4wbi_tlb_flags
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v4wbi
# endif
#else
# define v4wbi_possible_flags	0
# define v4wbi_always_flags	(-1UL)
#endif

#define fr_tlb_flags	(TLB_WB | TLB_DCLEAN | TLB_L2CLEAN_FR | \
			 TLB_V4_I_FULL | TLB_V4_D_FULL | \
			 TLB_V4_I_PAGE | TLB_V4_D_PAGE)

#ifdef CONFIG_CPU_TLB_FEROCEON
# define fr_possible_flags	fr_tlb_flags
# define fr_always_flags	fr_tlb_flags
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v4wbi
# endif
#else
# define fr_possible_flags	0
# define fr_always_flags	(-1UL)
#endif

#define v4wb_tlb_flags	(TLB_WB | TLB_DCLEAN | \
			 TLB_V4_I_FULL | TLB_V4_D_FULL | \
			 TLB_V4_D_PAGE)

#ifdef CONFIG_CPU_TLB_V4WB
# define v4wb_possible_flags	v4wb_tlb_flags
# define v4wb_always_flags	v4wb_tlb_flags
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v4wb
# endif
#else
# define v4wb_possible_flags	0
# define v4wb_always_flags	(-1UL)
#endif

#define v6wbi_tlb_flags (TLB_WB | TLB_DCLEAN | TLB_BARRIER | \
			 TLB_V6_I_FULL | TLB_V6_D_FULL | \
			 TLB_V6_I_PAGE | TLB_V6_D_PAGE | \
			 TLB_V6_I_ASID | TLB_V6_D_ASID)

#ifdef CONFIG_CPU_TLB_V6
#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && (defined (CONFIG_ARCH_ARMADA_XP) && !defined (CONFIG_AURORA_L2_PT_WALK))
# define v6wbi_possible_flags	(v6wbi_tlb_flags | TLB_L2CLEAN_FR)
# define v6wbi_always_flags	(v6wbi_tlb_flags | TLB_L2CLEAN_FR)
#else
# define v6wbi_possible_flags	v6wbi_tlb_flags
# define v6wbi_always_flags	v6wbi_tlb_flags
#endif
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v6wbi
# endif
#else
# define v6wbi_possible_flags	0
# define v6wbi_always_flags	(-1UL)
#endif

#define v7wbi_tlb_flags_smp	(TLB_WB | TLB_DCLEAN | TLB_BARRIER | \
			 TLB_V7_UIS_FULL | TLB_V7_UIS_PAGE | TLB_V7_UIS_ASID)
#define v7wbi_tlb_flags_up	(TLB_WB | TLB_DCLEAN | TLB_BARRIER | \
			 TLB_V6_U_FULL | TLB_V6_U_PAGE | TLB_V6_U_ASID)

#ifdef CONFIG_CPU_TLB_V7

# ifdef CONFIG_SMP_ON_UP
#  define v7wbi_possible_flags	(v7wbi_tlb_flags_smp | v7wbi_tlb_flags_up)
#  define v7wbi_always_flags	(v7wbi_tlb_flags_smp & v7wbi_tlb_flags_up)
# elif defined(CONFIG_SMP)
#  define v7wbi_possible_flags	v7wbi_tlb_flags_smp
#  define v7wbi_always_flags	v7wbi_tlb_flags_smp
# else
#  define v7wbi_possible_flags	v7wbi_tlb_flags_up
#  define v7wbi_always_flags	v7wbi_tlb_flags_up
# endif
# ifdef _TLB
#  define MULTI_TLB 1
# else
#  define _TLB v7wbi
# endif
#else
# define v7wbi_possible_flags	0
# define v7wbi_always_flags	(-1UL)
#endif

#ifndef _TLB
#error Unknown TLB model
#endif

#ifndef __ASSEMBLY__

#include <linux/sched.h>

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_CACHE_AURORA_L2)
extern void l2_clean_pa(unsigned int pa); 
#endif
struct cpu_tlb_fns {
	void (*flush_user_range)(unsigned long, unsigned long, struct vm_area_struct *);
	void (*flush_kern_range)(unsigned long, unsigned long);
	unsigned long tlb_flags;
};

#ifdef MULTI_TLB

#define __cpu_flush_user_tlb_range	cpu_tlb.flush_user_range
#define __cpu_flush_kern_tlb_range	cpu_tlb.flush_kern_range

#else

#define __cpu_flush_user_tlb_range	__glue(_TLB,_flush_user_tlb_range)
#define __cpu_flush_kern_tlb_range	__glue(_TLB,_flush_kern_tlb_range)

extern void __cpu_flush_user_tlb_range(unsigned long, unsigned long, struct vm_area_struct *);
extern void __cpu_flush_kern_tlb_range(unsigned long, unsigned long);

#endif

extern struct cpu_tlb_fns cpu_tlb;

#define __cpu_tlb_flags			cpu_tlb.tlb_flags

#define possible_tlb_flags	(v3_possible_flags | \
				 v4_possible_flags | \
				 v4wbi_possible_flags | \
				 fr_possible_flags | \
				 v4wb_possible_flags | \
				 fa_possible_flags | \
				 v6wbi_possible_flags | \
				 v7wbi_possible_flags)

#define always_tlb_flags	(v3_always_flags & \
				 v4_always_flags & \
				 v4wbi_always_flags & \
				 fr_always_flags & \
				 v4wb_always_flags & \
				 fa_always_flags & \
				 v6wbi_always_flags & \
				 v7wbi_always_flags)

#define tlb_flag(f)	((always_tlb_flags & (f)) || (__tlb_flag & possible_tlb_flags & (f)))

static inline void local_flush_tlb_all(void)
{
	const int zero = 0;
	const unsigned int __tlb_flag = __cpu_tlb_flags;

	if (tlb_flag(TLB_WB))
		dsb();

	if (tlb_flag(TLB_V3_FULL))
		asm("mcr p15, 0, %0, c6, c0, 0" : : "r" (zero) : "cc");
	if (tlb_flag(TLB_V4_U_FULL | TLB_V6_U_FULL))
		asm("mcr p15, 0, %0, c8, c7, 0" : : "r" (zero) : "cc");
	if (tlb_flag(TLB_V4_D_FULL | TLB_V6_D_FULL))
		asm("mcr p15, 0, %0, c8, c6, 0" : : "r" (zero) : "cc");
	if (tlb_flag(TLB_V4_I_FULL | TLB_V6_I_FULL))
		asm("mcr p15, 0, %0, c8, c5, 0" : : "r" (zero) : "cc");
	if (tlb_flag(TLB_V7_UIS_FULL))
		asm("mcr p15, 0, %0, c8, c3, 0" : : "r" (zero) : "cc");

	if (tlb_flag(TLB_BARRIER)) {
		dsb();
		isb();
	}
}

static inline void local_flush_tlb_mm(struct mm_struct *mm)
{
	const int zero = 0;
	const int asid = ASID(mm);
	const unsigned int __tlb_flag = __cpu_tlb_flags;

	if (tlb_flag(TLB_WB))
		dsb();

	if (cpumask_test_cpu(get_cpu(), mm_cpumask(mm))) {
		if (tlb_flag(TLB_V3_FULL))
			asm("mcr p15, 0, %0, c6, c0, 0" : : "r" (zero) : "cc");
		if (tlb_flag(TLB_V4_U_FULL))
			asm("mcr p15, 0, %0, c8, c7, 0" : : "r" (zero) : "cc");
		if (tlb_flag(TLB_V4_D_FULL))
			asm("mcr p15, 0, %0, c8, c6, 0" : : "r" (zero) : "cc");
		if (tlb_flag(TLB_V4_I_FULL))
			asm("mcr p15, 0, %0, c8, c5, 0" : : "r" (zero) : "cc");
	}
	put_cpu();

	if (tlb_flag(TLB_V6_U_ASID))
		asm("mcr p15, 0, %0, c8, c7, 2" : : "r" (asid) : "cc");
	if (tlb_flag(TLB_V6_D_ASID))
		asm("mcr p15, 0, %0, c8, c6, 2" : : "r" (asid) : "cc");
	if (tlb_flag(TLB_V6_I_ASID))
		asm("mcr p15, 0, %0, c8, c5, 2" : : "r" (asid) : "cc");
	if (tlb_flag(TLB_V7_UIS_ASID))
#ifdef CONFIG_ARM_ERRATA_720789
		asm("mcr p15, 0, %0, c8, c3, 0" : : "r" (zero) : "cc");
#else
		asm("mcr p15, 0, %0, c8, c3, 2" : : "r" (asid) : "cc");
#endif

	if (tlb_flag(TLB_BARRIER))
		dsb();
}

static inline void
local_flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr)
{
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
#if !defined(CONFIG_MV_LARGE_PAGE_SUPPORT) || defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT)
	const int zero = 0;
#endif
#else  
	const int zero = 0;
#endif  

	const unsigned int __tlb_flag = __cpu_tlb_flags;
#ifdef CONFIG_SYNO_ALPINE
	unsigned long uaddr_last;
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2) && (defined(CONFIG_MV_LARGE_PAGE_SUPPORT) && !defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT))
	if (tlb_flag(TLB_WB))
		dsb();

	uaddr = (uaddr & PAGE_MASK);
	__cpu_flush_user_tlb_range(uaddr, uaddr + PAGE_SIZE, vma);

#else
	uaddr = (uaddr & PAGE_MASK) | ASID(vma->vm_mm);
#ifdef CONFIG_SYNO_ALPINE
	uaddr_last = uaddr+PAGE_SIZE;
#endif

	if (tlb_flag(TLB_WB))
		dsb();

#ifdef CONFIG_SYNO_ALPINE
	 
	for (; uaddr < uaddr_last; uaddr += HW_PAGE_SIZE) {
#endif
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm))) {
		if (tlb_flag(TLB_V3_PAGE))
			asm("mcr p15, 0, %0, c6, c0, 0" : : "r" (uaddr) : "cc");
		if (tlb_flag(TLB_V4_U_PAGE))
			asm("mcr p15, 0, %0, c8, c7, 1" : : "r" (uaddr) : "cc");
		if (tlb_flag(TLB_V4_D_PAGE))
			asm("mcr p15, 0, %0, c8, c6, 1" : : "r" (uaddr) : "cc");
		if (tlb_flag(TLB_V4_I_PAGE))
			asm("mcr p15, 0, %0, c8, c5, 1" : : "r" (uaddr) : "cc");
		if (!tlb_flag(TLB_V4_I_PAGE) && tlb_flag(TLB_V4_I_FULL))
			asm("mcr p15, 0, %0, c8, c5, 0" : : "r" (zero) : "cc");
	}

	if (tlb_flag(TLB_V6_U_PAGE))
		asm("mcr p15, 0, %0, c8, c7, 1" : : "r" (uaddr) : "cc");
	if (tlb_flag(TLB_V6_D_PAGE))
		asm("mcr p15, 0, %0, c8, c6, 1" : : "r" (uaddr) : "cc");
	if (tlb_flag(TLB_V6_I_PAGE))
		asm("mcr p15, 0, %0, c8, c5, 1" : : "r" (uaddr) : "cc");
	if (tlb_flag(TLB_V7_UIS_PAGE))
#ifdef CONFIG_ARM_ERRATA_720789
		asm("mcr p15, 0, %0, c8, c3, 3" : : "r" (uaddr & PAGE_MASK) : "cc");
#else
		asm("mcr p15, 0, %0, c8, c3, 1" : : "r" (uaddr) : "cc");
#endif
#ifdef CONFIG_SYNO_ALPINE
	}
#endif

#endif  
	if (tlb_flag(TLB_BARRIER))
		dsb();
}

static inline void local_flush_tlb_kernel_page(unsigned long kaddr)
{
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
#if !defined(CONFIG_MV_LARGE_PAGE_SUPPORT) || defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT)
	const int zero = 0;
#endif
#else  
	const int zero = 0;
#endif  
	const unsigned int __tlb_flag = __cpu_tlb_flags;
#ifdef CONFIG_SYNO_ALPINE
	unsigned long kaddr_last;
#endif

	kaddr &= PAGE_MASK;
#ifdef CONFIG_SYNO_ALPINE
	kaddr_last = kaddr+PAGE_SIZE;
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH_V2) && (defined(CONFIG_MV_LARGE_PAGE_SUPPORT) && !defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT))
	if (tlb_flag(TLB_WB))
		dsb();

	__cpu_flush_kern_tlb_range(kaddr, kaddr + PAGE_SIZE);
#else

	if (tlb_flag(TLB_WB))
		dsb();

#ifdef CONFIG_SYNO_ALPINE
	for (; kaddr < kaddr_last; kaddr += HW_PAGE_SIZE) {
#endif
	if (tlb_flag(TLB_V3_PAGE))
		asm("mcr p15, 0, %0, c6, c0, 0" : : "r" (kaddr) : "cc");
	if (tlb_flag(TLB_V4_U_PAGE))
		asm("mcr p15, 0, %0, c8, c7, 1" : : "r" (kaddr) : "cc");
	if (tlb_flag(TLB_V4_D_PAGE))
		asm("mcr p15, 0, %0, c8, c6, 1" : : "r" (kaddr) : "cc");
	if (tlb_flag(TLB_V4_I_PAGE))
		asm("mcr p15, 0, %0, c8, c5, 1" : : "r" (kaddr) : "cc");
	if (!tlb_flag(TLB_V4_I_PAGE) && tlb_flag(TLB_V4_I_FULL))
		asm("mcr p15, 0, %0, c8, c5, 0" : : "r" (zero) : "cc");

	if (tlb_flag(TLB_V6_U_PAGE))
		asm("mcr p15, 0, %0, c8, c7, 1" : : "r" (kaddr) : "cc");
	if (tlb_flag(TLB_V6_D_PAGE))
		asm("mcr p15, 0, %0, c8, c6, 1" : : "r" (kaddr) : "cc");
	if (tlb_flag(TLB_V6_I_PAGE))
		asm("mcr p15, 0, %0, c8, c5, 1" : : "r" (kaddr) : "cc");
	if (tlb_flag(TLB_V7_UIS_PAGE))
		asm("mcr p15, 0, %0, c8, c3, 1" : : "r" (kaddr) : "cc");
#ifdef CONFIG_SYNO_ALPINE
	}
#endif 

#endif  

	if (tlb_flag(TLB_BARRIER)) {
		dsb();
		isb();
	}
}


#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
static inline void flush_pmd_entry(void *pmd)
{
	const unsigned int __tlb_flag = __cpu_tlb_flags;

	if (tlb_flag(TLB_DCLEAN))
#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_4611)
	{
		unsigned long flags;
        	raw_local_irq_save(flags);
		dmb();
#endif
#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && (defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6043) || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6124))
		asm("mcr	p15, 0, %0, c7, c14, 1  @ flush_pmd"
			: : "r" (pmd) : "cc");
#else
		asm("mcr	p15, 0, %0, c7, c10, 1	@ flush_pmd"
			: : "r" (pmd) : "cc");
#endif
#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_4611)
		raw_local_irq_restore(flags);
	}
#endif

#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_CACHE_AURORA_L2)
	if (tlb_flag(TLB_L2CLEAN_FR)) 
        	l2_clean_pa(__pa((unsigned long)pmd));
#else
	if (tlb_flag(TLB_L2CLEAN_FR))
		asm("mcr	p15, 1, %0, c15, c9, 1  @ L2 flush_pmd"
			: : "r" (pmd) : "cc");
#endif
	if (tlb_flag(TLB_WB))
		dsb();
}

static inline void clean_pmd_entry(void *pmd)
{
	const unsigned int __tlb_flag = __cpu_tlb_flags;

	if (tlb_flag(TLB_DCLEAN))
#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_4611)
	{
		unsigned long flags;
                raw_local_irq_save(flags);
                dmb();
#endif
#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && (defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6043) || defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_6124))
		asm("mcr	p15, 0, %0, c7, c14, 1  @ flush_pmd"
			: : "r" (pmd) : "cc");
#else
		asm("mcr	p15, 0, %0, c7, c10, 1	@ flush_pmd"
			: : "r" (pmd) : "cc");
#endif
#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_SHEEVA_ERRATA_ARM_CPU_4611)
		raw_local_irq_restore(flags);
	}
#endif

#if (defined(CONFIG_SYNO_ARMADA_ARCH)||defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_CACHE_AURORA_L2)
	if (tlb_flag(TLB_L2CLEAN_FR))
		l2_clean_pa(__pa((unsigned long)pmd));
#else
	if (tlb_flag(TLB_L2CLEAN_FR))
		asm("mcr	p15, 1, %0, c15, c9, 1  @ L2 flush_pmd"
			: : "r" (pmd) : "cc");
#endif
}
#else
static inline void flush_pmd_entry(void *pmd)
{
	const unsigned int __tlb_flag = __cpu_tlb_flags;
	char *p = (char *)pmd;

	if (tlb_flag(TLB_DCLEAN)) {
		while (p < ((char *)pmd + (LINKED_PMDS * sizeof(u32)))) {  
			asm("mcr	p15, 0, %0, c7, c10, 1	@ flush_pmd"
					: : "r" (p) : "cc");
			p += 32;  
		}
	}

	if (tlb_flag(TLB_WB))
		dsb();
}

static inline void clean_pmd_entry(void *pmd)
{
	const unsigned int __tlb_flag = __cpu_tlb_flags;
	char *p = (char *)pmd;

	if (tlb_flag(TLB_DCLEAN)) {
		while (p < ((char *)pmd + (LINKED_PMDS * sizeof(u32)))) {  
			asm("mcr	p15, 0, %0, c7, c10, 1	@ flush_pmd"
					: : "r" (p) : "cc");
			p += 32;  
		}
	}
}

#endif

#undef tlb_flag
#undef always_tlb_flags
#undef possible_tlb_flags

#define local_flush_tlb_range(vma,start,end)	__cpu_flush_user_tlb_range(start,end,vma)
#define local_flush_tlb_kernel_range(s,e)	__cpu_flush_kern_tlb_range(s,e)

#ifndef CONFIG_SMP
#define flush_tlb_all		local_flush_tlb_all
#define flush_tlb_mm		local_flush_tlb_mm
#define flush_tlb_page		local_flush_tlb_page
#define flush_tlb_kernel_page	local_flush_tlb_kernel_page
#define flush_tlb_range		local_flush_tlb_range
#define flush_tlb_kernel_range	local_flush_tlb_kernel_range
#else
extern void flush_tlb_all(void);
extern void flush_tlb_mm(struct mm_struct *mm);
extern void flush_tlb_page(struct vm_area_struct *vma, unsigned long uaddr);
extern void flush_tlb_kernel_page(unsigned long kaddr);
extern void flush_tlb_range(struct vm_area_struct *vma, unsigned long start, unsigned long end);
extern void flush_tlb_kernel_range(unsigned long start, unsigned long end);
#endif

#if __LINUX_ARM_ARCH__ < 6
extern void update_mmu_cache(struct vm_area_struct *vma, unsigned long addr,
	pte_t *ptep);
#else
static inline void update_mmu_cache(struct vm_area_struct *vma,
				    unsigned long addr, pte_t *ptep)
{
}
#endif

#endif

#endif  

#endif
