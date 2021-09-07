 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/highmem.h>

#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/fixmap.h>
#include <asm/io.h>

#include "mmu_decl.h"

unsigned long ioremap_base;
unsigned long ioremap_bot;
EXPORT_SYMBOL(ioremap_bot);	 

#if defined(CONFIG_6xx) || defined(CONFIG_POWER3)
#define HAVE_BATS	1
#endif

#if defined(CONFIG_FSL_BOOKE)
#define HAVE_TLBCAM	1
#endif

extern char etext[], _stext[];

#ifdef HAVE_BATS
extern phys_addr_t v_mapped_by_bats(unsigned long va);
extern unsigned long p_mapped_by_bats(phys_addr_t pa);
void setbat(int index, unsigned long virt, phys_addr_t phys,
	    unsigned int size, int flags);

#else  
#define v_mapped_by_bats(x)	(0UL)
#define p_mapped_by_bats(x)	(0UL)
#endif  

#ifdef HAVE_TLBCAM
extern unsigned int tlbcam_index;
extern phys_addr_t v_mapped_by_tlbcam(unsigned long va);
extern unsigned long p_mapped_by_tlbcam(phys_addr_t pa);
#else  
#define v_mapped_by_tlbcam(x)	(0UL)
#define p_mapped_by_tlbcam(x)	(0UL)
#endif  

#define PGDIR_ORDER	(32 + PGD_T_LOG2 - PGDIR_SHIFT)

pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *ret;

#ifndef CONFIG_PPC_4K_PAGES
	ret = (pgd_t *)kzalloc(1 << PGDIR_ORDER, GFP_KERNEL);
#else
	ret = (pgd_t *)__get_free_pages(GFP_KERNEL|__GFP_ZERO,
			PGDIR_ORDER - PAGE_SHIFT);
#endif
	return ret;
}

void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
#ifndef CONFIG_PPC_4K_PAGES
	kfree((void *)pgd);
#else
	free_pages((unsigned long)pgd, PGDIR_ORDER - PAGE_SHIFT);
#endif
}

__init_refok pte_t *pte_alloc_one_kernel(struct mm_struct *mm, unsigned long address)
{
	pte_t *pte;
	extern int mem_init_done;
	extern void *early_get_page(void);

	if (mem_init_done) {
		pte = (pte_t *)__get_free_page(GFP_KERNEL|__GFP_REPEAT|__GFP_ZERO);
	} else {
		pte = (pte_t *)early_get_page();
		if (pte)
			clear_page(pte);
	}
	return pte;
}

pgtable_t pte_alloc_one(struct mm_struct *mm, unsigned long address)
{
	struct page *ptepage;

#ifdef CONFIG_HIGHPTE
	gfp_t flags = GFP_KERNEL | __GFP_HIGHMEM | __GFP_REPEAT | __GFP_ZERO;
#else
	gfp_t flags = GFP_KERNEL | __GFP_REPEAT | __GFP_ZERO;
#endif

	ptepage = alloc_pages(flags, 0);
	if (!ptepage)
		return NULL;
	pgtable_page_ctor(ptepage);
	return ptepage;
}

void __iomem *
ioremap(phys_addr_t addr, unsigned long size)
{
	return __ioremap_caller(addr, size, _PAGE_NO_CACHE | _PAGE_GUARDED,
				__builtin_return_address(0));
}
EXPORT_SYMBOL(ioremap);

void __iomem *
ioremap_flags(phys_addr_t addr, unsigned long size, unsigned long flags)
{
	 
	if (flags & _PAGE_RW)
		flags |= _PAGE_DIRTY | _PAGE_HWWRITE;

	flags &= ~(_PAGE_USER | _PAGE_EXEC);

#ifdef CONFIG_SYNO_QORIQ
#if defined(CONFIG_FSL_BOOKE) && defined(CONFIG_PTE_64BIT)
	 
	flags |= _PAGE_BAP_SR;
#endif
#endif

	return __ioremap_caller(addr, size, flags, __builtin_return_address(0));
}
EXPORT_SYMBOL(ioremap_flags);

void __iomem *
__ioremap(phys_addr_t addr, unsigned long size, unsigned long flags)
{
	return __ioremap_caller(addr, size, flags, __builtin_return_address(0));
}

void __iomem *
__ioremap_caller(phys_addr_t addr, unsigned long size, unsigned long flags,
		 void *caller)
{
	unsigned long v, i;
	phys_addr_t p;
	int err;

	if ((flags & _PAGE_PRESENT) == 0)
		flags |= PAGE_KERNEL;

	if (flags & _PAGE_NO_CACHE)
		flags &= ~_PAGE_COHERENT;

	p = addr & PAGE_MASK;
	size = PAGE_ALIGN(addr + size) - p;

	if (p < 16*1024*1024)
		p += _ISA_MEM_BASE;

#ifndef CONFIG_CRASH_DUMP
	 
	if (mem_init_done && (p < virt_to_phys(high_memory))) {
		printk("__ioremap(): phys addr 0x%llx is RAM lr %p\n",
		       (unsigned long long)p, __builtin_return_address(0));
		return NULL;
	}
#endif

	if (size == 0)
		return NULL;

	if ((v = p_mapped_by_bats(p))   )
		goto out;

	if ((v = p_mapped_by_tlbcam(p)))
		goto out;

	if (mem_init_done) {
		struct vm_struct *area;
		area = get_vm_area_caller(size, VM_IOREMAP, caller);
		if (area == 0)
			return NULL;
		v = (unsigned long) area->addr;
	} else {
		v = (ioremap_bot -= size);
	}

	err = 0;
	for (i = 0; i < size && err == 0; i += PAGE_SIZE)
		err = map_page(v+i, p+i, flags);
	if (err) {
		if (mem_init_done)
			vunmap((void *)v);
		return NULL;
	}

out:
	return (void __iomem *) (v + ((unsigned long)addr & ~PAGE_MASK));
}
EXPORT_SYMBOL(__ioremap);

void iounmap(volatile void __iomem *addr)
{
	 
	if (v_mapped_by_bats((unsigned long)addr)) return;

	if (addr > high_memory && (unsigned long) addr < ioremap_bot)
		vunmap((void *) (PAGE_MASK & (unsigned long)addr));
}
EXPORT_SYMBOL(iounmap);

int map_page(unsigned long va, phys_addr_t pa, int flags)
{
	pmd_t *pd;
	pte_t *pg;
	int err = -ENOMEM;

	pd = pmd_offset(pud_offset(pgd_offset_k(va), va), va);
	 
	pg = pte_alloc_kernel(pd, va);
	if (pg != 0) {
		err = 0;
		 
		BUG_ON((pte_val(*pg) & (_PAGE_PRESENT | _PAGE_HASHPTE)) &&
		       flags);
		set_pte_at(&init_mm, va, pg, pfn_pte(pa >> PAGE_SHIFT,
						     __pgprot(flags)));
	}
	return err;
}

void __init mapin_ram(void)
{
	unsigned long v, s, f;
	phys_addr_t p;
	int ktext;

	s = mmu_mapin_ram();
	v = PAGE_OFFSET + s;
	p = memstart_addr + s;
	for (; s < total_lowmem; s += PAGE_SIZE) {
		ktext = ((char *) v >= _stext && (char *) v < etext);
		f = ktext ? PAGE_KERNEL_TEXT : PAGE_KERNEL;
		map_page(v, p, f);
#ifdef CONFIG_PPC_STD_MMU_32
		if (ktext)
			hash_preload(&init_mm, v, 0, 0x300);
#endif
		v += PAGE_SIZE;
		p += PAGE_SIZE;
	}
}

int
get_pteptr(struct mm_struct *mm, unsigned long addr, pte_t **ptep, pmd_t **pmdp)
{
        pgd_t	*pgd;
	pud_t	*pud;
        pmd_t	*pmd;
        pte_t	*pte;
        int     retval = 0;

        pgd = pgd_offset(mm, addr & PAGE_MASK);
        if (pgd) {
		pud = pud_offset(pgd, addr & PAGE_MASK);
		if (pud && pud_present(*pud)) {
			pmd = pmd_offset(pud, addr & PAGE_MASK);
			if (pmd_present(*pmd)) {
				pte = pte_offset_map(pmd, addr & PAGE_MASK);
				if (pte) {
					retval = 1;
					*ptep = pte;
					if (pmdp)
						*pmdp = pmd;
					 
				}
			}
		}
        }
        return(retval);
}

#ifdef CONFIG_DEBUG_PAGEALLOC

static int __change_page_attr(struct page *page, pgprot_t prot)
{
	pte_t *kpte;
	pmd_t *kpmd;
	unsigned long address;

	BUG_ON(PageHighMem(page));
	address = (unsigned long)page_address(page);

	if (v_mapped_by_bats(address) || v_mapped_by_tlbcam(address))
		return 0;
	if (!get_pteptr(&init_mm, address, &kpte, &kpmd))
		return -EINVAL;
	set_pte_at(&init_mm, address, kpte, mk_pte(page, prot));
	wmb();
#ifdef CONFIG_PPC_STD_MMU
	flush_hash_pages(0, address, pmd_val(*kpmd), 1);
#else
	flush_tlb_page(NULL, address);
#endif
	pte_unmap(kpte);

	return 0;
}

static int change_page_attr(struct page *page, int numpages, pgprot_t prot)
{
	int i, err = 0;
	unsigned long flags;

	local_irq_save(flags);
	for (i = 0; i < numpages; i++, page++) {
		err = __change_page_attr(page, prot);
		if (err)
			break;
	}
	local_irq_restore(flags);
	return err;
}

void kernel_map_pages(struct page *page, int numpages, int enable)
{
	if (PageHighMem(page))
		return;

	change_page_attr(page, numpages, enable ? PAGE_KERNEL : __pgprot(0));
}
#endif  

static int fixmaps;

void __set_fixmap (enum fixed_addresses idx, phys_addr_t phys, pgprot_t flags)
{
	unsigned long address = __fix_to_virt(idx);

	if (idx >= __end_of_fixed_addresses) {
		BUG();
		return;
	}

	map_page(address, phys, pgprot_val(flags));
	fixmaps++;
}

void __this_fixmap_does_not_exist(void)
{
	WARN_ON(1);
}
