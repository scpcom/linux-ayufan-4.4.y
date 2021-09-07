 
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

void *kmap_atomic(struct page *page, enum km_type type)
{
	unsigned int idx;
	unsigned long vaddr;
	void *kmap;

	pagefault_disable();
	if (!PageHighMem(page))
		return page_address(page);

	debug_kmap_atomic(type);

	kmap = kmap_high_get(page);
	if (kmap)
		return kmap;

	idx = type + KM_TYPE_NR * smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEBUG_HIGHMEM
	 
	BUG_ON(!pte_none(*(TOP_PTE(vaddr))));
#endif
	set_pte_ext(TOP_PTE(vaddr), mk_pte(page, kmap_prot), 0);
	 
	local_flush_tlb_kernel_page(vaddr);

	return (void *)vaddr;
}
EXPORT_SYMBOL(kmap_atomic);

void kunmap_atomic(void *kvaddr, enum km_type type)
{
	unsigned long vaddr = (unsigned long) kvaddr & PAGE_MASK;
	unsigned int idx = type + KM_TYPE_NR * smp_processor_id();

	if (kvaddr >= (void *)FIXADDR_START) {
#ifdef CONFIG_SYNO_PLX_PORTING
		__cpuc_flush_dcache_area((void *)vaddr, PAGE_SIZE);
#else
		__cpuc_flush_dcache_page((void *)vaddr);
#endif
#ifdef CONFIG_DEBUG_HIGHMEM
		BUG_ON(vaddr != __fix_to_virt(FIX_KMAP_BEGIN + idx));
		set_pte_ext(TOP_PTE(vaddr), __pte(0), 0);
		local_flush_tlb_kernel_page(vaddr);
#else
		(void) idx;   
#endif
	} else if (vaddr >= PKMAP_ADDR(0) && vaddr < PKMAP_ADDR(LAST_PKMAP)) {
		 
		kunmap_high(pte_page(pkmap_page_table[PKMAP_NR(vaddr)]));
	}
	pagefault_enable();
}
EXPORT_SYMBOL(kunmap_atomic);

void *kmap_atomic_pfn(unsigned long pfn, enum km_type type)
{
	unsigned int idx;
	unsigned long vaddr;

	pagefault_disable();

	idx = type + KM_TYPE_NR * smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEBUG_HIGHMEM
	BUG_ON(!pte_none(*(TOP_PTE(vaddr))));
#endif
	set_pte_ext(TOP_PTE(vaddr), pfn_pte(pfn, kmap_prot), 0);
	local_flush_tlb_kernel_page(vaddr);

	return (void *)vaddr;
}

struct page *kmap_atomic_to_page(const void *ptr)
{
	unsigned long vaddr = (unsigned long)ptr;
	pte_t *pte;

	if (vaddr < FIXADDR_START)
		return virt_to_page(ptr);

	pte = TOP_PTE(vaddr);
	return pte_page(*pte);
}
