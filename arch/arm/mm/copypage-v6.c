 
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/highmem.h>

#include <asm/pgtable.h>
#include <asm/shmparam.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <asm/cachetype.h>

#include "mm.h"

#if SHMLBA > 16384
#error FIX ME
#endif

#define from_address	(0xffff8000)
#define to_address	(0xffffc000)

static DEFINE_SPINLOCK(v6_lock);

static void v6_copy_user_highpage_nonaliasing(struct page *to,
	struct page *from, unsigned long vaddr)
{
	void *kto, *kfrom;

	kfrom = kmap_atomic(from, KM_USER0);
	kto = kmap_atomic(to, KM_USER1);
	copy_page(kto, kfrom);
#ifdef CONFIG_SYNO_PLX_PORTING
	__cpuc_flush_dcache_area(kto,PAGE_SIZE);
#endif
	kunmap_atomic(kto, KM_USER1);
	kunmap_atomic(kfrom, KM_USER0);
}

static void v6_clear_user_highpage_nonaliasing(struct page *page, unsigned long vaddr)
{
	void *kaddr = kmap_atomic(page, KM_USER0);
	clear_page(kaddr);
	kunmap_atomic(kaddr, KM_USER0);
}

static void discard_old_kernel_data(void *kto)
{
	__asm__("mcrr	p15, 0, %1, %0, c6	@ 0xec401f06"
	   :
	   : "r" (kto),
	     "r" ((unsigned long)kto + PAGE_SIZE - L1_CACHE_BYTES)
	   : "cc");
}

static void v6_copy_user_highpage_aliasing(struct page *to,
	struct page *from, unsigned long vaddr)
{
	unsigned int offset = CACHE_COLOUR(vaddr);
	unsigned long kfrom, kto;

#ifdef CONFIG_SYNO_PLX_PORTING
	if (!test_and_set_bit(PG_dcache_clean, &from->flags))
#else
	if (test_and_clear_bit(PG_dcache_dirty, &from->flags))
#endif
		__flush_dcache_page(page_mapping(from), from);

	discard_old_kernel_data(page_address(to));

	spin_lock(&v6_lock);

	set_pte_ext(TOP_PTE(from_address) + offset, pfn_pte(page_to_pfn(from), PAGE_KERNEL), 0);
	set_pte_ext(TOP_PTE(to_address) + offset, pfn_pte(page_to_pfn(to), PAGE_KERNEL), 0);

	kfrom = from_address + (offset << PAGE_SHIFT);
	kto   = to_address + (offset << PAGE_SHIFT);

	flush_tlb_kernel_page(kfrom);
	flush_tlb_kernel_page(kto);

	copy_page((void *)kto, (void *)kfrom);

	spin_unlock(&v6_lock);
}

static void v6_clear_user_highpage_aliasing(struct page *page, unsigned long vaddr)
{
	unsigned int offset = CACHE_COLOUR(vaddr);
	unsigned long to = to_address + (offset << PAGE_SHIFT);

	discard_old_kernel_data(page_address(page));

	spin_lock(&v6_lock);

	set_pte_ext(TOP_PTE(to_address) + offset, pfn_pte(page_to_pfn(page), PAGE_KERNEL), 0);
	flush_tlb_kernel_page(to);
	clear_page((void *)to);

	spin_unlock(&v6_lock);
}

struct cpu_user_fns v6_user_fns __initdata = {
	.cpu_clear_user_highpage = v6_clear_user_highpage_nonaliasing,
	.cpu_copy_user_highpage	= v6_copy_user_highpage_nonaliasing,
};

static int __init v6_userpage_init(void)
{
	if (cache_is_vipt_aliasing()) {
		cpu_user.cpu_clear_user_highpage = v6_clear_user_highpage_aliasing;
		cpu_user.cpu_copy_user_highpage = v6_copy_user_highpage_aliasing;
	}

	return 0;
}

core_initcall(v6_userpage_init);
