 
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
#ifdef CONFIG_SYNO_PLX_PORTING
#ifdef CONFIG_SMP_LAZY_DCACHE_FLUSH
#include <mach/lazy-flush.h>
#endif  
#endif

static unsigned long shared_pte_mask = L_PTE_MT_BUFFERABLE;

#ifdef CONFIG_SYNO_PLX_PORTING
#ifdef CONFIG_SMP_LAZY_DCACHE_FLUSH
 
void remote_flush_dcache_page(void *info) {
	struct address_space *mapping;
	struct page *page= (struct page *)info;

	mapping = page_mapping(page);
	__flush_dcache_page(mapping, page);
}
#endif
#endif

#if !defined(CONFIG_SMP) || !defined(CONFIG_SYNO_PLX_PORTING)
 
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

	ret = pte_present(entry);

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

	flush_dcache_mmap_lock(mapping);
#ifdef CONFIG_ARM_ARMV5_L2_CACHE_COHERENCY_FIX
	 
	for (run = 0; run < 3; run++) {
		vma_prio_tree_foreach(mpnt, &iter, &mapping->i_mmap,
				pgoff, pgoff) {
			if ((mpnt->vm_mm != mm || mpnt == vma) && run == 0)
				continue;
			if (!(mpnt->vm_flags & VM_MAYSHARE) &&
				run != 2)  
				continue;
			offset = (pgoff - mpnt->vm_pgoff) << PAGE_SHIFT;
			aliases += adjust_pte(mpnt, mpnt->vm_start + offset,
					 
					run == 2,
					 
					run == 1);
		}
		if (aliases == 0 && run == 1)
			break;
	}
#else
	vma_prio_tree_foreach(mpnt, &iter, &mapping->i_mmap, pgoff, pgoff) {
		 
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
#ifndef CONFIG_SYNO_PLX_PORTING
	else
		flush_cache_page(vma, addr, pfn);
#endif
}

void update_mmu_cache(struct vm_area_struct *vma, unsigned long addr, pte_t pte)
{
	unsigned long pfn = pte_pfn(pte);
	struct address_space *mapping;
	struct page *page;

	if (!pfn_valid(pfn))
		return;

	page = pfn_to_page(pfn);
	mapping = page_mapping(page);
#ifdef CONFIG_SYNO_PLX_PORTING
	if (!test_and_set_bit(PG_dcache_clean, &page->flags))
		__flush_dcache_page(mapping, page);
#else  
#ifndef CONFIG_SMP
	if (test_and_clear_bit(PG_dcache_dirty, &page->flags))
		__flush_dcache_page(mapping, page);
#endif
#endif  
	if (mapping) {
		if (cache_is_vivt())
			make_coherent(mapping, vma, addr, pfn);
		else if (vma->vm_flags & VM_EXEC)
			__flush_icache_all();
	}
}

#endif	 

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
