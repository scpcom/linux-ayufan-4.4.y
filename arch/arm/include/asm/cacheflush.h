#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _ASMARM_CACHEFLUSH_H
#define _ASMARM_CACHEFLUSH_H

#include <linux/mm.h>

#include <asm/glue-cache.h>
#include <asm/shmparam.h>
#include <asm/cachetype.h>
#include <asm/outercache.h>

#define CACHE_COLOUR(vaddr)	((vaddr & (SHMLBA - 1)) >> PAGE_SHIFT)


#define PG_dcache_clean PG_arch_1



struct cpu_cache_fns {
	void (*flush_icache_all)(void);
	void (*flush_kern_all)(void);

#if defined(CONFIG_SYNO_ALPINE) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
	void (*flush_kern_louis)(void);
#endif
	void (*flush_user_all)(void);
	void (*flush_user_range)(unsigned long, unsigned long, unsigned int);

	void (*coherent_kern_range)(unsigned long, unsigned long);
	void (*coherent_user_range)(unsigned long, unsigned long);
	void (*flush_kern_dcache_area)(void *, size_t);

	void (*dma_map_area)(const void *, size_t, int);
	void (*dma_unmap_area)(const void *, size_t, int);

	void (*dma_flush_range)(const void *, const void *);
};


#ifdef MULTI_CACHE

extern struct cpu_cache_fns cpu_cache;

#define __cpuc_flush_icache_all		cpu_cache.flush_icache_all
#define __cpuc_flush_kern_all		cpu_cache.flush_kern_all
#if defined( CONFIG_SYNO_ALPINE) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#define __cpuc_flush_kern_louis		cpu_cache.flush_kern_louis
#endif
#define __cpuc_flush_user_all		cpu_cache.flush_user_all
#define __cpuc_flush_user_range		cpu_cache.flush_user_range
#define __cpuc_coherent_kern_range	cpu_cache.coherent_kern_range
#define __cpuc_coherent_user_range	cpu_cache.coherent_user_range
#define __cpuc_flush_dcache_area	cpu_cache.flush_kern_dcache_area

#define dmac_map_area			cpu_cache.dma_map_area
#define dmac_unmap_area			cpu_cache.dma_unmap_area
#define dmac_flush_range		cpu_cache.dma_flush_range

#else

extern void __cpuc_flush_icache_all(void);
extern void __cpuc_flush_kern_all(void);
#if defined CONFIG_SYNO_ALPINE) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
extern void __cpuc_flush_kern_louis(void);
#endif
extern void __cpuc_flush_user_all(void);
extern void __cpuc_flush_user_range(unsigned long, unsigned long, unsigned int);
extern void __cpuc_coherent_kern_range(unsigned long, unsigned long);
extern void __cpuc_coherent_user_range(unsigned long, unsigned long);
extern void __cpuc_flush_dcache_area(void *, size_t);

extern void dmac_map_area(const void *, size_t, int);
extern void dmac_unmap_area(const void *, size_t, int);
extern void dmac_flush_range(const void *, const void *);

#endif

extern void copy_to_user_page(struct vm_area_struct *, struct page *,
	unsigned long, void *, const void *, unsigned long);
#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	do {							\
		memcpy(dst, src, len);				\
	} while (0)

#define __flush_icache_all_generic()					\
	asm("mcr	p15, 0, %0, c7, c5, 0"				\
	    : : "r" (0));

#define __flush_icache_all_v7_smp()					\
	asm("mcr	p15, 0, %0, c7, c1, 0"				\
	    : : "r" (0));

#if (defined(CONFIG_CPU_V7) && \
     (defined(CONFIG_CPU_V6) || defined(CONFIG_CPU_V6K))) || \
	defined(CONFIG_SMP_ON_UP)
#define __flush_icache_preferred	__cpuc_flush_icache_all
#elif __LINUX_ARM_ARCH__ >= 7 && defined(CONFIG_SMP)
#define __flush_icache_preferred	__flush_icache_all_v7_smp
#elif __LINUX_ARM_ARCH__ == 6 && defined(CONFIG_ARM_ERRATA_411920)
#define __flush_icache_preferred	__cpuc_flush_icache_all
#else
#define __flush_icache_preferred	__flush_icache_all_generic
#endif

static inline void __flush_icache_all(void)
{
	__flush_icache_preferred();
#ifdef CONFIG_SYNO_ALPINE_FIX_USB_HANG
	dsb();
#endif
}
#ifdef CONFIG_SYNO_ALPINE
 
#define flush_cache_louis()		__cpuc_flush_kern_louis()
#endif

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
 
#define flush_cache_louis()		__cpuc_flush_kern_louis()
#endif

#define flush_cache_all()		__cpuc_flush_kern_all()

static inline void vivt_flush_cache_mm(struct mm_struct *mm)
{
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(mm)))
		__cpuc_flush_user_all();
}

static inline void
vivt_flush_cache_range(struct vm_area_struct *vma, unsigned long start, unsigned long end)
{
#ifdef CONFIG_SYNO_ALPINE
	struct mm_struct *mm = vma->vm_mm;

	if (!mm || cpumask_test_cpu(smp_processor_id(), mm_cpumask(mm)))
#else
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm)))
#endif
		__cpuc_flush_user_range(start & PAGE_MASK, PAGE_ALIGN(end),
					vma->vm_flags);
}

static inline void
vivt_flush_cache_page(struct vm_area_struct *vma, unsigned long user_addr, unsigned long pfn)
{
#ifdef CONFIG_SYNO_ALPINE
	struct mm_struct *mm = vma->vm_mm;

	if (!mm || cpumask_test_cpu(smp_processor_id(), mm_cpumask(mm))) {
#else
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm))) {
#endif
		unsigned long addr = user_addr & PAGE_MASK;
		__cpuc_flush_user_range(addr, addr + PAGE_SIZE, vma->vm_flags);
#ifdef CONFIG_SYNO_ALPINE
	}
#else
	}
#endif
}

#ifndef CONFIG_CPU_CACHE_VIPT
#define flush_cache_mm(mm) \
		vivt_flush_cache_mm(mm)
#define flush_cache_range(vma,start,end) \
		vivt_flush_cache_range(vma,start,end)
#define flush_cache_page(vma,addr,pfn) \
		vivt_flush_cache_page(vma,addr,pfn)
#else
extern void flush_cache_mm(struct mm_struct *mm);
extern void flush_cache_range(struct vm_area_struct *vma, unsigned long start, unsigned long end);
extern void flush_cache_page(struct vm_area_struct *vma, unsigned long user_addr, unsigned long pfn);
#endif

#define flush_cache_dup_mm(mm) flush_cache_mm(mm)


#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#define local_flush_cache_user_range(start,end) \
 	__cpuc_coherent_user_range((start) & PAGE_MASK, PAGE_ALIGN(end))
 
#if defined(CONFIG_SMP) && defined(CONFIG_CPU_V6)
extern void flush_cache_user_range(unsigned long start, unsigned long end);
#else
#define flush_cache_user_range	local_flush_cache_user_range
#endif
#else
#define flush_cache_user_range(start,end) \
	__cpuc_coherent_user_range((start) & PAGE_MASK, PAGE_ALIGN(end))
#endif


#define flush_icache_range(s,e)		__cpuc_coherent_kern_range(s,e)


#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && (defined (CONFIG_CACHE_AURORA_L2) && defined (CONFIG_AURORA_L2_OUTER) && !defined (CONFIG_AURORA_L2_PT_WALK))
 
extern void aurora_l2_flush_range(unsigned long start, unsigned long end);
#define clean_dcache_area(start,size)	do {cpu_dcache_clean_area(start, size);	\
	aurora_l2_flush_range(__pa(start), __pa(start) + size);} while (0)			
#else
#define clean_dcache_area(start,size)	cpu_dcache_clean_area(start, size)
#endif

#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
extern void flush_dcache_page(struct page *);

static inline void flush_kernel_vmap_range(void *addr, int size)
{
	if ((cache_is_vivt() || cache_is_vipt_aliasing()))
	  __cpuc_flush_dcache_area(addr, (size_t)size);
}
static inline void invalidate_kernel_vmap_range(void *addr, int size)
{
	if ((cache_is_vivt() || cache_is_vipt_aliasing()))
	  __cpuc_flush_dcache_area(addr, (size_t)size);
}

#define ARCH_HAS_FLUSH_ANON_PAGE
static inline void flush_anon_page(struct vm_area_struct *vma,
			 struct page *page, unsigned long vmaddr)
{
	extern void __flush_anon_page(struct vm_area_struct *vma,
				struct page *, unsigned long);
	if (PageAnon(page))
		__flush_anon_page(vma, page, vmaddr);
}

#define ARCH_HAS_FLUSH_KERNEL_DCACHE_PAGE
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
extern void flush_kernel_dcache_page(struct page *);
#else
static inline void flush_kernel_dcache_page(struct page *page)
{
}
#endif

#define flush_dcache_mmap_lock(mapping) \
	spin_lock_irq(&(mapping)->tree_lock)
#define flush_dcache_mmap_unlock(mapping) \
	spin_unlock_irq(&(mapping)->tree_lock)

#define flush_icache_user_range(vma,page,addr,len) \
	flush_dcache_page(page)

#define flush_icache_page(vma,page)	do { } while (0)

static inline void flush_cache_vmap(unsigned long start, unsigned long end)
{
	if (!cache_is_vipt_nonaliasing())
		flush_cache_all();
	else
		 
		dsb();
}

static inline void flush_cache_vunmap(unsigned long start, unsigned long end)
{
	if (!cache_is_vipt_nonaliasing())
		flush_cache_all();
}

#endif
