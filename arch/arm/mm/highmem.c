 
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <asm/fixmap.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include "mm.h"

void *kmap(struct page *page)
{
	might_sleep();
	if (!PageHighMem(page))
		return page_address(page);
	return kmap_high(page);
}
EXPORT_SYMBOL(kmap);

void kunmap(struct page *page)
{
	BUG_ON(in_interrupt());
	if (!PageHighMem(page))
		return;
	kunmap_high(page);
}
EXPORT_SYMBOL(kunmap);

void *__kmap_atomic(struct page *page)
{
	unsigned int idx;
	unsigned long vaddr;
	void *kmap;
	int type;

	pagefault_disable();
	if (!PageHighMem(page))
		return page_address(page);

#ifdef CONFIG_DEBUG_HIGHMEM
	 
	if (!cache_is_vivt())
		kmap = NULL;
	else
#endif
		kmap = kmap_high_get(page);
	if (kmap)
		return kmap;

	type = kmap_atomic_idx_push();

	idx = type + KM_TYPE_NR * smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEBUG_HIGHMEM
	 
#ifdef CONFIG_SYNO_ALPINE
	BUG_ON(!pte_none(get_fix_pte(vaddr)));
#else
	BUG_ON(!pte_none(*(TOP_PTE(vaddr))));
#endif
#endif
#ifdef CONFIG_SYNO_ALPINE
	set_fix_pte(vaddr, mk_pte(page, kmap_prot));
#else
	set_pte_ext(TOP_PTE(vaddr), mk_pte(page, kmap_prot), 0);
	 
	local_flush_tlb_kernel_page(vaddr);
#endif

	return (void *)vaddr;
}
EXPORT_SYMBOL(__kmap_atomic);

void __kunmap_atomic(void *kvaddr)
{
	unsigned long vaddr = (unsigned long) kvaddr & PAGE_MASK;
	int idx, type;

	if (kvaddr >= (void *)FIXADDR_START) {
		type = kmap_atomic_idx();
		idx = type + KM_TYPE_NR * smp_processor_id();

		if (cache_is_vivt())
			__cpuc_flush_dcache_area((void *)vaddr, PAGE_SIZE);
#ifdef CONFIG_DEBUG_HIGHMEM
		BUG_ON(vaddr != __fix_to_virt(FIX_KMAP_BEGIN + idx));
#ifdef CONFIG_SYNO_ALPINE
		set_fix_pte(vaddr, __pte(0));
#else
		set_pte_ext(TOP_PTE(vaddr), __pte(0), 0);
		local_flush_tlb_kernel_page(vaddr);
#endif
#else
		(void) idx;   
#endif
		kmap_atomic_idx_pop();
	} else if (vaddr >= PKMAP_ADDR(0) && vaddr < PKMAP_ADDR(LAST_PKMAP)) {
		 
		kunmap_high(pte_page(pkmap_page_table[PKMAP_NR(vaddr)]));
	}
	pagefault_enable();
}
EXPORT_SYMBOL(__kunmap_atomic);

void *kmap_atomic_pfn(unsigned long pfn)
{
	unsigned long vaddr;
	int idx, type;

	pagefault_disable();

	type = kmap_atomic_idx_push();
	idx = type + KM_TYPE_NR * smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEBUG_HIGHMEM
#ifdef CONFIG_SYNO_ALPINE
	BUG_ON(!pte_none(get_fix_pte(vaddr)));
#else
	BUG_ON(!pte_none(*(TOP_PTE(vaddr))));
#endif
#endif
#ifdef CONFIG_SYNO_ALPINE
	set_fix_pte(vaddr, pfn_pte(pfn, kmap_prot));
#else
	set_pte_ext(TOP_PTE(vaddr), pfn_pte(pfn, kmap_prot), 0);
	local_flush_tlb_kernel_page(vaddr);
#endif

	return (void *)vaddr;
}

struct page *kmap_atomic_to_page(const void *ptr)
{
	unsigned long vaddr = (unsigned long)ptr;
	pte_t *pte;

	if (vaddr < FIXADDR_START)
		return virt_to_page(ptr);

#ifdef CONFIG_SYNO_ALPINE
	return pte_page(get_fix_pte(vaddr));
#else
	pte = TOP_PTE(vaddr);
	return pte_page(*pte);
#endif
}
