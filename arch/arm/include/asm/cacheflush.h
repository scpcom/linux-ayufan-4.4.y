 
#ifndef _ASMARM_CACHEFLUSH_H
#define _ASMARM_CACHEFLUSH_H

#include <linux/mm.h>

#include <asm/glue.h>
#include <asm/shmparam.h>
#include <asm/cachetype.h>

#define CACHE_COLOUR(vaddr)	((vaddr & (SHMLBA - 1)) >> PAGE_SHIFT)

#undef _CACHE
#undef MULTI_CACHE

#if defined(CONFIG_CPU_CACHE_V3)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE v3
# endif
#endif

#if defined(CONFIG_CPU_CACHE_V4)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE v4
# endif
#endif

#if defined(CONFIG_CPU_ARM920T) || defined(CONFIG_CPU_ARM922T) || \
    defined(CONFIG_CPU_ARM925T) || defined(CONFIG_CPU_ARM1020)
# define MULTI_CACHE 1
#endif

#if defined(CONFIG_CPU_FA526)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE fa
# endif
#endif

#if defined(CONFIG_CPU_ARM926T)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE arm926
# endif
#endif

#if defined(CONFIG_CPU_ARM940T)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE arm940
# endif
#endif

#if defined(CONFIG_CPU_ARM946E)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE arm946
# endif
#endif

#if defined(CONFIG_CPU_CACHE_V4WB)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE v4wb
# endif
#endif

#if defined(CONFIG_CPU_XSCALE)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE xscale
# endif
#endif

#if defined(CONFIG_CPU_XSC3)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE xsc3
# endif
#endif

#if defined(CONFIG_CPU_MOHAWK)
# ifdef _CACHE
#  define MULTI_CACHE 1
# else
#  define _CACHE mohawk
# endif
#endif

#if defined(CONFIG_CPU_FEROCEON)
# define MULTI_CACHE 1
#endif

#if defined(CONFIG_CPU_V6)
 
#  define MULTI_CACHE 1
 
#endif

#if defined(CONFIG_CPU_V7)
 
#  define MULTI_CACHE 1
 
#endif

#if !defined(_CACHE) && !defined(MULTI_CACHE)
#error Unknown cache maintainence model
#endif

#ifdef CONFIG_SYNO_PLX_PORTING
#define PG_dcache_clean PG_arch_1
#else
#define PG_dcache_dirty PG_arch_1
#endif

struct cpu_cache_fns {
	void (*flush_kern_all)(void);
	void (*flush_user_all)(void);
	void (*flush_user_range)(unsigned long, unsigned long, unsigned int);

	void (*coherent_kern_range)(unsigned long, unsigned long);
	void (*coherent_user_range)(unsigned long, unsigned long);
#ifdef CONFIG_SYNO_PLX_PORTING
	void (*flush_kern_dcache_area)(void *, size_t);
#else
	void (*flush_kern_dcache_page)(void *);
#endif

	void (*dma_inv_range)(const void *, const void *);
	void (*dma_clean_range)(const void *, const void *);
	void (*dma_flush_range)(const void *, const void *);
};

struct outer_cache_fns {
	void (*inv_range)(unsigned long, unsigned long);
	void (*clean_range)(unsigned long, unsigned long);
	void (*flush_range)(unsigned long, unsigned long);
};

#ifdef CONFIG_MV_XOR_NET_DMA
 
static inline void flush_user_range_fast(unsigned long start,
										 unsigned long end, unsigned int vmflags)
{
	unsigned long flags;

	raw_local_irq_save(flags);
	__asm__("mcr p15, 5, %0, c15, c15, 0" : : "r" (start));
	__asm__("mcr p15, 5, %0, c15, c15, 1" : : "r" (end-1));
	raw_local_irq_restore(flags);

}
#endif
 
#ifdef MULTI_CACHE

extern struct cpu_cache_fns cpu_cache;

#define __cpuc_flush_kern_all		cpu_cache.flush_kern_all
#define __cpuc_flush_user_all		cpu_cache.flush_user_all
#define __cpuc_flush_user_range		cpu_cache.flush_user_range
#define __cpuc_coherent_kern_range	cpu_cache.coherent_kern_range
#define __cpuc_coherent_user_range	cpu_cache.coherent_user_range
#ifdef CONFIG_SYNO_PLX_PORTING
#define __cpuc_flush_dcache_area	cpu_cache.flush_kern_dcache_area
#else
#define __cpuc_flush_dcache_page	cpu_cache.flush_kern_dcache_page
#endif

#define dmac_inv_range			cpu_cache.dma_inv_range
#define dmac_clean_range		cpu_cache.dma_clean_range
#define dmac_flush_range		cpu_cache.dma_flush_range

#else

#define __cpuc_flush_kern_all		__glue(_CACHE,_flush_kern_cache_all)
#define __cpuc_flush_user_all		__glue(_CACHE,_flush_user_cache_all)
#define __cpuc_flush_user_range		__glue(_CACHE,_flush_user_cache_range)
#define __cpuc_coherent_kern_range	__glue(_CACHE,_coherent_kern_range)
#define __cpuc_coherent_user_range	__glue(_CACHE,_coherent_user_range)
#ifdef CONFIG_SYNO_PLX_PORTING
#define __cpuc_flush_dcache_area	__glue(_CACHE,_flush_kern_dcache_area)
#else
#define __cpuc_flush_dcache_page	__glue(_CACHE,_flush_kern_dcache_page)
#endif

extern void __cpuc_flush_kern_all(void);
extern void __cpuc_flush_user_all(void);
extern void __cpuc_flush_user_range(unsigned long, unsigned long, unsigned int);
extern void __cpuc_coherent_kern_range(unsigned long, unsigned long);
extern void __cpuc_coherent_user_range(unsigned long, unsigned long);
#ifdef CONFIG_SYNO_PLX_PORTING
extern void __cpuc_flush_dcache_area(void *, size_t);
#else
extern void __cpuc_flush_dcache_page(void *);
#endif

#define dmac_inv_range			__glue(_CACHE,_dma_inv_range)
#define dmac_clean_range		__glue(_CACHE,_dma_clean_range)
#define dmac_flush_range		__glue(_CACHE,_dma_flush_range)

extern void dmac_inv_range(const void *, const void *);
extern void dmac_clean_range(const void *, const void *);
extern void dmac_flush_range(const void *, const void *);

#endif

#ifdef CONFIG_OUTER_CACHE

extern struct outer_cache_fns outer_cache;

static inline void outer_inv_range(unsigned long start, unsigned long end)
{
	if (outer_cache.inv_range)
		outer_cache.inv_range(start, end);
}
static inline void outer_clean_range(unsigned long start, unsigned long end)
{
	if (outer_cache.clean_range)
		outer_cache.clean_range(start, end);
}
static inline void outer_flush_range(unsigned long start, unsigned long end)
{
	if (outer_cache.flush_range)
		outer_cache.flush_range(start, end);
}

#else

static inline void outer_inv_range(unsigned long start, unsigned long end)
{ }
static inline void outer_clean_range(unsigned long start, unsigned long end)
{ }
static inline void outer_flush_range(unsigned long start, unsigned long end)
{ }

#endif

#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
	do {							\
		memcpy(dst, src, len);				\
		flush_ptrace_access(vma, page, vaddr, dst, len, 1);\
	} while (0)

#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	do {							\
		memcpy(dst, src, len);				\
	} while (0)

#define flush_cache_all()		__cpuc_flush_kern_all()
#ifndef CONFIG_CPU_CACHE_VIPT
static inline void flush_cache_mm(struct mm_struct *mm)
{
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(mm)))
		__cpuc_flush_user_all();
}

static inline void
flush_cache_range(struct vm_area_struct *vma, unsigned long start, unsigned long end)
{
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm)))
		__cpuc_flush_user_range(start & PAGE_MASK, PAGE_ALIGN(end),
					vma->vm_flags);
}

static inline void
flush_cache_page(struct vm_area_struct *vma, unsigned long user_addr, unsigned long pfn)
{
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm))) {
		unsigned long addr = user_addr & PAGE_MASK;
		__cpuc_flush_user_range(addr, addr + PAGE_SIZE, vma->vm_flags);
	}
}

static inline void
flush_ptrace_access(struct vm_area_struct *vma, struct page *page,
			 unsigned long uaddr, void *kaddr,
			 unsigned long len, int write)
{
	if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm))) {
		unsigned long addr = (unsigned long)kaddr;
		__cpuc_coherent_kern_range(addr, addr + len);
	}
}
#else
extern void flush_cache_mm(struct mm_struct *mm);
extern void flush_cache_range(struct vm_area_struct *vma, unsigned long start, unsigned long end);
extern void flush_cache_page(struct vm_area_struct *vma, unsigned long user_addr, unsigned long pfn);
extern void flush_ptrace_access(struct vm_area_struct *vma, struct page *page,
				unsigned long uaddr, void *kaddr,
				unsigned long len, int write);
#endif

#define flush_cache_dup_mm(mm) flush_cache_mm(mm)

#define flush_cache_user_range(vma,start,end) \
	__cpuc_coherent_user_range((start) & PAGE_MASK, PAGE_ALIGN(end))

#define flush_icache_range(s,e)		__cpuc_coherent_kern_range(s,e)

#define clean_dcache_area(start,size)	cpu_dcache_clean_area(start, size)

extern void flush_dcache_page(struct page *);

extern void __flush_dcache_page(struct address_space *mapping, struct page *page);

static inline void __flush_icache_all(void)
{
#ifdef CONFIG_ARM_ERRATA_411920
	extern void v6_icache_inval_all(void);
	v6_icache_inval_all();
#else
	asm("mcr	p15, 0, %0, c7, c5, 0	@ invalidate I-cache\n"
	    :
	    : "r" (0));
#endif
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
static inline void flush_kernel_dcache_page(struct page *page)
{
	 
	if ((cache_is_vivt() || cache_is_vipt_aliasing()) && !PageHighMem(page))
#ifdef CONFIG_SYNO_PLX_PORTING
		__cpuc_flush_dcache_area(page_address(page), PAGE_SIZE);
#else
		__cpuc_flush_dcache_page(page_address(page));
#endif
}

#ifdef CONFIG_ARM_MARVELL_BSP_MM_ADD_API_FOR_DMA
static inline void flush_kernel_dcache_addr(void *addr)
{
	if ((cache_is_vivt() || cache_is_vipt_aliasing()))
		__cpuc_flush_dcache_page(addr);
}
static inline void invalidate_kernel_dcache_addr(void *addr)
{
	if ((cache_is_vivt() || cache_is_vipt_aliasing()))
		__cpuc_flush_dcache_page(addr);
}
#endif

#define flush_dcache_mmap_lock(mapping) \
	spin_lock_irq(&(mapping)->tree_lock)
#define flush_dcache_mmap_unlock(mapping) \
	spin_unlock_irq(&(mapping)->tree_lock)

#define flush_icache_user_range(vma,page,addr,len) \
	flush_dcache_page(page)

#define flush_icache_page(vma,page)	do { } while (0)

static inline void flush_ioremap_region(unsigned long phys, void __iomem *virt,
	unsigned offset, size_t size)
{
	const void *start = (void __force *)virt + offset;
	dmac_inv_range(start, start + size);
}

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
