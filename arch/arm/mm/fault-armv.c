/*
 *  linux/arch/arm/mm/fault-armv.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Modifications for ARM processor (c) 1995-2002 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bitops.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/pagemap.h>

#include <asm/bugs.h>
#include <asm/cacheflush.h>
#include <asm/cachetype.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

static unsigned long shared_pte_mask = L_PTE_MT_BUFFERABLE;

/*
 * We take the easy way out of this problem - we make the
 * PTE uncacheable.  However, we leave the write buffer on.
 *
 * Note that the pte lock held when calling update_mmu_cache must also
 * guard the pte (somewhere else in the same mm) that we modify here.
 * Therefore those configurations which might call adjust_pte (those
 * without CONFIG_CPU_CACHE_VIPT) cannot support split page_table_lock.
 */
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
static int adjust_pte(struct vm_area_struct *vma, unsigned long address,
		int update, int only_shared)
#else
static int adjust_pte(struct vm_area_struct *vma, unsigned long address)
#endif
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte, entry;
	int ret;

	pgd = pgd_offset(vma->vm_mm, address);
	if (pgd_none(*pgd))
		goto no_pgd;
	if (pgd_bad(*pgd))
		goto bad_pgd;

	pmd = pmd_offset(pgd, address);
	if (pmd_none(*pmd))
		goto no_pmd;
	if (pmd_bad(*pmd))
		goto bad_pmd;

	pte = pte_offset_map(pmd, address);
	entry = *pte;

	/*
	 * If this page is present, it's actually being shared.
	 */
	ret = pte_present(entry);

	/*
	 * If this page isn't present, or is already setup to
	 * fault (ie, is old), we can safely ignore any issues.
	 */
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
	if (ret &&
	    (pte_val(entry) & L_PTE_MT_MASK) != shared_pte_mask &&
	    update) {
#else
	if (ret && (pte_val(entry) & L_PTE_MT_MASK) != shared_pte_mask) {
#endif
		unsigned long pfn = pte_pfn(entry);
		flush_cache_page(vma, address, pfn);
		outer_flush_range((pfn << PAGE_SHIFT),
				  (pfn << PAGE_SHIFT) + PAGE_SIZE);
		pte_val(entry) &= ~L_PTE_MT_MASK;
		pte_val(entry) |= shared_pte_mask;
		set_pte_at(vma->vm_mm, address, pte, entry);
		flush_tlb_page(vma, address);
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
		printk(KERN_DEBUG "Uncached vma %08x "
			"(addr %08lx flags %08lx phy %08x) from pid %d\n",
			(unsigned int) vma, vma->vm_start, vma->vm_flags,
			(unsigned int) (pfn << PAGE_SHIFT),
			current->pid);
#endif
	}
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
	if (only_shared && (pte_val(entry) & L_PTE_MT_MASK) != shared_pte_mask)
		ret = 0;
#endif
	pte_unmap(pte);
	return ret;

bad_pgd:
	pgd_ERROR(*pgd);
	pgd_clear(pgd);
no_pgd:
	return 0;

bad_pmd:
	pmd_ERROR(*pmd);
	pmd_clear(pmd);
no_pmd:
	return 0;
}

static void
make_coherent(struct address_space *mapping, struct vm_area_struct *vma, unsigned long addr, unsigned long pfn)
{
	struct mm_struct *mm = vma->vm_mm;
	struct vm_area_struct *mpnt;
	struct prio_tree_iter iter;
	unsigned long offset;
	pgoff_t pgoff;
	int aliases = 0;
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
	int run;
#endif

	pgoff = vma->vm_pgoff + ((addr - vma->vm_start) >> PAGE_SHIFT);

	/*
	 * If we have any shared mappings that are in the same mm
	 * space, then we need to handle them specially to maintain
	 * cache coherency.
	 */
	flush_dcache_mmap_lock(mapping);
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
	/*
	 * In the first run we just check if we have to make some
	 * address space uncacheable because of L1 VIVT. In the second
	 * we check if there is an uncached map in other processes.  If
	 * one of the previous condition is true we proceed to make
	 * *all* (both in current process VMA and that of others) of
	 * them so. This should solve both cases of multiple shared
	 * memories attached in the same process but not impact the
	 * common case of just one mapping per process.
	 */
	for (run = 0; run < 3; run++) {
		vma_prio_tree_foreach(mpnt, &iter, &mapping->i_mmap,
				pgoff, pgoff) {
			if ((mpnt->vm_mm != mm || mpnt == vma) && run == 0)
				continue;
			if (!(mpnt->vm_flags & VM_MAYSHARE) &&
				run != 2) /* update all mappings */
				continue;
			offset = (pgoff - mpnt->vm_pgoff) << PAGE_SHIFT;
			aliases += adjust_pte(mpnt, mpnt->vm_start + offset,
					/* update only on the last run */
					run == 2,
					/*
					 * on the second run
					 * catch shared in other procs
					 */
					run == 1);
		}
		if (aliases == 0 && run == 1)
			break;
	}
#else
	vma_prio_tree_foreach(mpnt, &iter, &mapping->i_mmap, pgoff, pgoff) {
		/*
		 * If this VMA is not in our MM, we can ignore it.
		 * Note that we intentionally mask out the VMA
		 * that we are fixing up.
		 */
		if (mpnt->vm_mm != mm || mpnt == vma)
			continue;
		if (!(mpnt->vm_flags & VM_MAYSHARE))
			continue;
		offset = (pgoff - mpnt->vm_pgoff) << PAGE_SHIFT;
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
		aliases += adjust_pte(mpnt, mpnt->vm_start + offset, 1, 0);
#else
		aliases += adjust_pte(mpnt, mpnt->vm_start + offset);
#endif
	}
#endif
	flush_dcache_mmap_unlock(mapping);
	if (aliases)
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
		adjust_pte(vma, addr, 1, 0);
#else
		adjust_pte(vma, addr);
#endif
	else
		flush_cache_page(vma, addr, pfn);
}

/*
 * Take care of architecture specific things when placing a new PTE into
 * a page table, or changing an existing PTE.  Basically, there are two
 * things that we need to take care of:
 *
 *  1. If PG_dcache_dirty is set for the page, we need to ensure
 *     that any cache entries for the kernels virtual memory
 *     range are written back to the page.
 *  2. If we have multiple shared mappings of the same space in
 *     an object, we need to deal with the cache aliasing issues.
 *
 * Note that the pte lock will be held.
 */
void update_mmu_cache(struct vm_area_struct *vma, unsigned long addr, pte_t pte)
{
	unsigned long pfn = pte_pfn(pte);
	struct address_space *mapping;
	struct page *page;

	if (!pfn_valid(pfn))
		return;

	page = pfn_to_page(pfn);
	mapping = page_mapping(page);
#ifndef CONFIG_SMP
	if (test_and_clear_bit(PG_dcache_dirty, &page->flags))
		__flush_dcache_page(mapping, page);
#endif
	if (mapping) {
		if (cache_is_vivt())
			make_coherent(mapping, vma, addr, pfn);
		else if (vma->vm_flags & VM_EXEC)
			__flush_icache_all();
	}
}

/*
 * Check whether the write buffer has physical address aliasing
 * issues.  If it has, we need to avoid them for the case where
 * we have several shared mappings of the same object in user
 * space.
 */
static int __init check_writebuffer(unsigned long *p1, unsigned long *p2)
{
	register unsigned long zero = 0, one = 1, val;

	local_irq_disable();
	mb();
	*p1 = one;
	mb();
	*p2 = zero;
	mb();
	val = *p1;
	mb();
	local_irq_enable();
	return val != zero;
}

void __init check_writebuffer_bugs(void)
{
	struct page *page;
	const char *reason;
	unsigned long v = 1;

	printk(KERN_INFO "CPU: Testing write buffer coherency: ");

	page = alloc_page(GFP_KERNEL);
	if (page) {
		unsigned long *p1, *p2;
		pgprot_t prot = __pgprot(L_PTE_PRESENT|L_PTE_YOUNG|
					 L_PTE_DIRTY|L_PTE_WRITE|
					 L_PTE_MT_BUFFERABLE);

		p1 = vmap(&page, 1, VM_IOREMAP, prot);
		p2 = vmap(&page, 1, VM_IOREMAP, prot);

		if (p1 && p2) {
			v = check_writebuffer(p1, p2);
			reason = "enabling work-around";
		} else {
			reason = "unable to map memory\n";
		}

		vunmap(p1);
		vunmap(p2);
		put_page(page);
	} else {
		reason = "unable to grab page\n";
	}

	if (v) {
		printk("failed, %s\n", reason);
		shared_pte_mask = L_PTE_MT_UNCACHED;
	} else {
		printk("ok\n");
	}
}
