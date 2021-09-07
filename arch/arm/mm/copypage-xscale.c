 
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/highmem.h>

#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

#include "mm.h"

#define COPYPAGE_MINICACHE	0xffff8000

#define minicache_pgprot __pgprot(L_PTE_PRESENT | L_PTE_YOUNG | \
				  L_PTE_MT_MINICACHE)

static DEFINE_SPINLOCK(minicache_lock);

static void __naked
mc_copy_user_page(void *from, void *to)
{
	 
	asm volatile(
	"stmfd	sp!, {r4, r5, lr}		\n\
	mov	lr, %2				\n\
	pld	[r0, #0]			\n\
	pld	[r0, #32]			\n\
	pld	[r1, #0]			\n\
	pld	[r1, #32]			\n\
1:	pld	[r0, #64]			\n\
	pld	[r0, #96]			\n\
	pld	[r1, #64]			\n\
	pld	[r1, #96]			\n\
2:	ldrd	r2, [r0], #8			\n\
	ldrd	r4, [r0], #8			\n\
	mov	ip, r1				\n\
	strd	r2, [r1], #8			\n\
	ldrd	r2, [r0], #8			\n\
	strd	r4, [r1], #8			\n\
	ldrd	r4, [r0], #8			\n\
	strd	r2, [r1], #8			\n\
	strd	r4, [r1], #8			\n\
	mcr	p15, 0, ip, c7, c10, 1		@ clean D line\n\
	ldrd	r2, [r0], #8			\n\
	mcr	p15, 0, ip, c7, c6, 1		@ invalidate D line\n\
	ldrd	r4, [r0], #8			\n\
	mov	ip, r1				\n\
	strd	r2, [r1], #8			\n\
	ldrd	r2, [r0], #8			\n\
	strd	r4, [r1], #8			\n\
	ldrd	r4, [r0], #8			\n\
	strd	r2, [r1], #8			\n\
	strd	r4, [r1], #8			\n\
	mcr	p15, 0, ip, c7, c10, 1		@ clean D line\n\
	subs	lr, lr, #1			\n\
	mcr	p15, 0, ip, c7, c6, 1		@ invalidate D line\n\
	bgt	1b				\n\
	beq	2b				\n\
	ldmfd	sp!, {r4, r5, pc}		"
	:
	: "r" (from), "r" (to), "I" (PAGE_SIZE / 64 - 1));
}

void xscale_mc_copy_user_highpage(struct page *to, struct page *from,
	unsigned long vaddr)
{
	void *kto = kmap_atomic(to, KM_USER1);

#ifdef CONFIG_SYNO_PLX_PORTING
	if (!test_and_set_bit(PG_dcache_clean, &from->flags))
#else
	if (test_and_clear_bit(PG_dcache_dirty, &from->flags))
#endif
		__flush_dcache_page(page_mapping(from), from);

	spin_lock(&minicache_lock);

	set_pte_ext(TOP_PTE(COPYPAGE_MINICACHE), pfn_pte(page_to_pfn(from), minicache_pgprot), 0);
	flush_tlb_kernel_page(COPYPAGE_MINICACHE);

	mc_copy_user_page((void *)COPYPAGE_MINICACHE, kto);

	spin_unlock(&minicache_lock);

	kunmap_atomic(kto, KM_USER1);
}

void
xscale_mc_clear_user_highpage(struct page *page, unsigned long vaddr)
{
	void *ptr, *kaddr = kmap_atomic(page, KM_USER0);
	asm volatile(
	"mov	r1, %2				\n\
	mov	r2, #0				\n\
	mov	r3, #0				\n\
1:	mov	ip, %0				\n\
	strd	r2, [%0], #8			\n\
	strd	r2, [%0], #8			\n\
	strd	r2, [%0], #8			\n\
	strd	r2, [%0], #8			\n\
	mcr	p15, 0, ip, c7, c10, 1		@ clean D line\n\
	subs	r1, r1, #1			\n\
	mcr	p15, 0, ip, c7, c6, 1		@ invalidate D line\n\
	bne	1b"
	: "=r" (ptr)
	: "0" (kaddr), "I" (PAGE_SIZE / 32)
	: "r1", "r2", "r3", "ip");
	kunmap_atomic(kaddr, KM_USER0);
}

struct cpu_user_fns xscale_mc_user_fns __initdata = {
	.cpu_clear_user_highpage = xscale_mc_clear_user_highpage,
	.cpu_copy_user_highpage	= xscale_mc_copy_user_highpage,
};
