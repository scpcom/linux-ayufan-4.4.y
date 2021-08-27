#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>

#include <asm/cacheflush.h>
#include <asm/cachetype.h>
#include <asm/highmem.h>
#include <asm/smp_plat.h>
#include <asm/system.h>
#include <asm/tlbflush.h>
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#include <asm/smp_plat.h>
#endif

#include "mm.h"

#ifdef CONFIG_CPU_CACHE_VIPT

#define ALIAS_FLUSH_START	0xffff4000

static void flush_pfn_alias(unsigned long pfn, unsigned long vaddr)
{
	unsigned long to = ALIAS_FLUSH_START + (CACHE_COLOUR(vaddr) << PAGE_SHIFT);
	const int zero = 0;

	set_pte_ext(TOP_PTE(to), pfn_pte(pfn, PAGE_KERNEL), 0);
	flush_tlb_kernel_page(to);

	asm(	"mcrr	p15, 0, %1, %0, c14\n"
	"	mcr	p15, 0, %2, c7, c10, 4"
	    :
	    : "r" (to), "r" (to + PAGE_SIZE - L1_CACHE_BYTES), "r" (zero)
	    : "cc");
}

static void flush_icache_alias(unsigned long pfn, unsigned long vaddr, unsigned long len)
{
	unsigned long colour = CACHE_COLOUR(vaddr);
	unsigned long offset = vaddr & (PAGE_SIZE - 1);
	unsigned long to;

	set_pte_ext(TOP_PTE(ALIAS_FLUSH_START) + colour, pfn_pte(pfn, PAGE_KERNEL), 0);
	to = ALIAS_FLUSH_START + (colour << PAGE_SHIFT) + offset;
	flush_tlb_kernel_page(to);
	flush_icache_range(to, to + len);
}

void flush_cache_mm(struct mm_struct *mm)
{
	if (cache_is_vivt()) {
		vivt_flush_cache_mm(mm);
		return;
	}

	if (cache_is_vipt_aliasing()) {
		asm(	"mcr	p15, 0, %0, c7, c14, 0\n"
		"	mcr	p15, 0, %0, c7, c10, 4"
		    :
		    : "r" (0)
		    : "cc");
	}
}

void flush_cache_range(struct vm_area_struct *vma, unsigned long start, unsigned long end)
{
	if (cache_is_vivt()) {
		vivt_flush_cache_range(vma, start, end);
		return;
	}

	if (cache_is_vipt_aliasing()) {
		asm(	"mcr	p15, 0, %0, c7, c14, 0\n"
		"	mcr	p15, 0, %0, c7, c10, 4"
		    :
		    : "r" (0)
		    : "cc");
	}

	if (vma->vm_flags & VM_EXEC)
		__flush_icache_all();
}

void flush_cache_page(struct vm_area_struct *vma, unsigned long user_addr, unsigned long pfn)
{
	if (cache_is_vivt()) {
		vivt_flush_cache_page(vma, user_addr, pfn);
		return;
	}

	if (cache_is_vipt_aliasing()) {
		flush_pfn_alias(pfn, user_addr);
		__flush_icache_all();
	}

	if (vma->vm_flags & VM_EXEC && icache_is_vivt_asid_tagged())
		__flush_icache_all();
}

#else
#define flush_pfn_alias(pfn,vaddr)		do { } while (0)
#define flush_icache_alias(pfn,vaddr,len)	do { } while (0)
#endif

static void flush_ptrace_access_other(void *args)
{
	__flush_icache_all();
}

static
void flush_ptrace_access(struct vm_area_struct *vma, struct page *page,
			 unsigned long uaddr, void *kaddr, unsigned long len)
{
	if (cache_is_vivt()) {
		if (cpumask_test_cpu(smp_processor_id(), mm_cpumask(vma->vm_mm))) {
			unsigned long addr = (unsigned long)kaddr;
			__cpuc_coherent_kern_range(addr, addr + len);
		}
		return;
	}

	if (cache_is_vipt_aliasing()) {
		flush_pfn_alias(page_to_pfn(page), uaddr);
		__flush_icache_all();
		return;
	}

	if (vma->vm_flags & VM_EXEC) {
		unsigned long addr = (unsigned long)kaddr;
		if (icache_is_vipt_aliasing())
			flush_icache_alias(page_to_pfn(page), uaddr, len);
		else
			__cpuc_coherent_kern_range(addr, addr + len);
		if (cache_ops_need_broadcast())
			smp_call_function(flush_ptrace_access_other,
					  NULL, 1);
	}
}

void copy_to_user_page(struct vm_area_struct *vma, struct page *page,
		       unsigned long uaddr, void *dst, const void *src,
		       unsigned long len)
{
#ifdef CONFIG_SMP
	preempt_disable();
#endif
	memcpy(dst, src, len);
	flush_ptrace_access(vma, page, uaddr, dst, len);
#ifdef CONFIG_SMP
	preempt_enable();
#endif
}

void __flush_dcache_page(struct address_space *mapping, struct page *page)
{
	 
	if (!PageHighMem(page)) {
		__cpuc_flush_dcache_area(page_address(page), PAGE_SIZE);
	} else {
		void *addr = kmap_high_get(page);
		if (addr) {
			__cpuc_flush_dcache_area(addr, PAGE_SIZE);
			kunmap_high(page);
		} else if (cache_is_vipt()) {
			 
			addr = kmap_atomic(page);
			__cpuc_flush_dcache_area(addr, PAGE_SIZE);
			kunmap_atomic(addr);
		}
	}

	if (mapping && cache_is_vipt_aliasing())
		flush_pfn_alias(page_to_pfn(page),
				page->index << PAGE_CACHE_SHIFT);
}

static void __flush_dcache_aliases(struct address_space *mapping, struct page *page)
{
	struct mm_struct *mm = current->active_mm;
	struct vm_area_struct *mpnt;
	struct prio_tree_iter iter;
	pgoff_t pgoff;

	pgoff = page->index << (PAGE_CACHE_SHIFT - PAGE_SHIFT);

	flush_dcache_mmap_lock(mapping);
	vma_prio_tree_foreach(mpnt, &iter, &mapping->i_mmap, pgoff, pgoff) {
		unsigned long offset;

		if (mpnt->vm_mm != mm)
			continue;
		if (!(mpnt->vm_flags & VM_MAYSHARE))
			continue;
		offset = (pgoff - mpnt->vm_pgoff) << PAGE_SHIFT;
		flush_cache_page(mpnt, mpnt->vm_start + offset, page_to_pfn(page));
	}
	flush_dcache_mmap_unlock(mapping);
}

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_L2X0_INSTRUCTION_ONLY)

void __sync_outer_cache(pte_t *ptep, pte_t pteval)
{
	if (pte_present(*ptep) && pte_exec(*ptep) && (!pte_present(pteval) || !pte_exec(pteval))) {
		unsigned long phys = __pfn_to_phys(pte_pfn(*ptep));

		outer_flush_range(phys, phys + PAGE_SIZE);
	}
}

static void sync_outer_cache(struct page *page)
{
	unsigned long phys = page_to_phys(page);

	outer_flush_range(phys, phys + PAGE_SIZE);
}
#endif

#if __LINUX_ARM_ARCH__ >= 6
void __sync_icache_dcache(pte_t pteval)
{
	unsigned long pfn;
	struct page *page;
	struct address_space *mapping;

	if (cache_is_vipt_nonaliasing() && !pte_exec(pteval))
		 
		return;
	pfn = pte_pfn(pteval);
	if (!pfn_valid(pfn))
		return;

	page = pfn_to_page(pfn);
	if (cache_is_vipt_aliasing())
		mapping = page_mapping(page);
	else
		mapping = NULL;

	if (!test_and_set_bit(PG_dcache_clean, &page->flags))
		__flush_dcache_page(mapping, page);

	if (pte_exec(pteval))
		__flush_icache_all();
}
#endif

void flush_dcache_page(struct page *page)
{
	struct address_space *mapping;

	if (page == ZERO_PAGE(0))
		return;

	mapping = page_mapping(page);

	if (!cache_ops_need_broadcast() &&
	    mapping && !mapping_mapped(mapping))
		clear_bit(PG_dcache_clean, &page->flags);
	else {
		__flush_dcache_page(mapping, page);
		if (mapping && cache_is_vivt())
			__flush_dcache_aliases(mapping, page);
		else if (mapping)
			__flush_icache_all();

#if defined(CONFIG_SYNO_COMCERTO) && defined(CONFIG_L2X0_INSTRUCTION_ONLY)
		sync_outer_cache(page);
#endif
		set_bit(PG_dcache_clean, &page->flags);
	}
}
EXPORT_SYMBOL(flush_dcache_page);

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
 
void flush_kernel_dcache_page(struct page *page)
{
	if (cache_is_vivt() || cache_is_vipt_aliasing()) {
		struct address_space *mapping;

		mapping = page_mapping(page);

		if (!mapping || mapping_mapped(mapping)) {
			void *addr;

			addr = page_address(page);
			 
			if (!IS_ENABLED(CONFIG_HIGHMEM) || addr)
				__cpuc_flush_dcache_area(addr, PAGE_SIZE);
		}
	}
}
EXPORT_SYMBOL(flush_kernel_dcache_page);
#endif
 
void __flush_anon_page(struct vm_area_struct *vma, struct page *page, unsigned long vmaddr)
{
	unsigned long pfn;

	if (cache_is_vipt_nonaliasing())
		return;

	pfn = page_to_pfn(page);
	if (cache_is_vivt()) {
		flush_cache_page(vma, vmaddr, pfn);
	} else {
		 
		flush_pfn_alias(pfn, vmaddr);
		__flush_icache_all();
	}

	__cpuc_flush_dcache_area(page_address(page), PAGE_SIZE);
}
