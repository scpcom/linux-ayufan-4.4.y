#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 

#ifndef _ASMARM_PGTABLE_H
#define _ASMARM_PGTABLE_H

#include <linux/const.h>
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_ALPINE)
#else
#include <asm-generic/4level-fixup.h>
#endif
#include <asm/proc-fns.h>

#ifndef CONFIG_MMU

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_ALPINE)
#include <asm-generic/4level-fixup.h>
#endif
#include "pgtable-nommu.h"

#else

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined(CONFIG_SYNO_ALPINE)
#include <asm-generic/pgtable-nopud.h>
#endif
#include <asm/memory.h>
#include <mach/vmalloc.h>
#include <asm/pgtable-hwdef.h>

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_ARM_LPAE)
#include <asm/pgtable-3level.h>
#elif defined(CONFIG_SYNO_ALPINE) && defined(CONFIG_ARM_LPAE)
#include <asm/pgtable-3level.h>
#else
#include <asm/pgtable-2level.h>
#endif

#ifndef VMALLOC_START
#define VMALLOC_OFFSET		(8*1024*1024)
#define VMALLOC_START		(((unsigned long)high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1))
#endif

#define LIBRARY_TEXT_START	0x0c000000

#ifndef __ASSEMBLY__
extern void __pte_error(const char *file, int line, pte_t);
extern void __pmd_error(const char *file, int line, pmd_t);
extern void __pgd_error(const char *file, int line, pgd_t);

#define pte_ERROR(pte)		__pte_error(__FILE__, __LINE__, pte)
#define pmd_ERROR(pmd)		__pmd_error(__FILE__, __LINE__, pmd)
#define pgd_ERROR(pgd)		__pgd_error(__FILE__, __LINE__, pgd)

#define FIRST_USER_ADDRESS	PAGE_SIZE

#if defined(CONFIG_ARM_LPAE) && defined(CONFIG_SYNO_ALPINE_FIX_USB_HANG)
#define USER_PGTABLES_CEILING	TASK_SIZE
#endif

#define _L_PTE_DEFAULT	L_PTE_PRESENT | L_PTE_YOUNG

extern pgprot_t		pgprot_user;
extern pgprot_t		pgprot_kernel;

#define _MOD_PROT(p, b)	__pgprot(pgprot_val(p) | (b))

#define PAGE_NONE		_MOD_PROT(pgprot_user, L_PTE_XN | L_PTE_RDONLY)
#define PAGE_SHARED		_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_XN)
#define PAGE_SHARED_EXEC	_MOD_PROT(pgprot_user, L_PTE_USER)
#define PAGE_COPY		_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_RDONLY | L_PTE_XN)
#define PAGE_COPY_EXEC		_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_RDONLY)
#define PAGE_READONLY		_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_RDONLY | L_PTE_XN)
#define PAGE_READONLY_EXEC	_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_RDONLY)
#define PAGE_KERNEL		_MOD_PROT(pgprot_kernel, L_PTE_XN)
#define PAGE_KERNEL_EXEC	pgprot_kernel

#define __PAGE_NONE		__pgprot(_L_PTE_DEFAULT | L_PTE_RDONLY | L_PTE_XN)
#define __PAGE_SHARED		__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_XN)
#define __PAGE_SHARED_EXEC	__pgprot(_L_PTE_DEFAULT | L_PTE_USER)
#define __PAGE_COPY		__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY | L_PTE_XN)
#define __PAGE_COPY_EXEC	__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY)
#define __PAGE_READONLY		__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY | L_PTE_XN)
#define __PAGE_READONLY_EXEC	__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY)

#define __pgprot_modify(prot,mask,bits)		\
	__pgprot((pgprot_val(prot) & ~(mask)) | (bits))

#define pgprot_noncached(prot) \
	__pgprot_modify(prot, L_PTE_MT_MASK, L_PTE_MT_UNCACHED)

#define pgprot_writecombine(prot) \
	__pgprot_modify(prot, L_PTE_MT_MASK, L_PTE_MT_BUFFERABLE)

#define pgprot_stronglyordered(prot) \
	__pgprot_modify(prot, L_PTE_MT_MASK, L_PTE_MT_UNCACHED)

#ifdef CONFIG_ARM_DMA_MEM_BUFFERABLE
#define pgprot_dmacoherent(prot) \
	__pgprot_modify(prot, L_PTE_MT_MASK, L_PTE_MT_BUFFERABLE | L_PTE_XN)
#define __HAVE_PHYS_MEM_ACCESS_PROT
struct file;
extern pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot);
#else
#define pgprot_dmacoherent(prot) \
	__pgprot_modify(prot, L_PTE_MT_MASK, L_PTE_MT_UNCACHED | L_PTE_XN)
#endif

#endif  

#define __P000  __PAGE_NONE
#define __P001  __PAGE_READONLY
#define __P010  __PAGE_COPY
#define __P011  __PAGE_COPY
#define __P100  __PAGE_READONLY_EXEC
#define __P101  __PAGE_READONLY_EXEC
#define __P110  __PAGE_COPY_EXEC
#define __P111  __PAGE_COPY_EXEC

#define __S000  __PAGE_NONE
#define __S001  __PAGE_READONLY
#define __S010  __PAGE_SHARED
#define __S011  __PAGE_SHARED
#define __S100  __PAGE_READONLY_EXEC
#define __S101  __PAGE_READONLY_EXEC
#define __S110  __PAGE_SHARED_EXEC
#define __S111  __PAGE_SHARED_EXEC

#ifndef __ASSEMBLY__
 
extern struct page *empty_zero_page;
#define ZERO_PAGE(vaddr)	(empty_zero_page)

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];

#define pgd_index(addr)		((addr) >> PGDIR_SHIFT)

#define pgd_offset(mm, addr)	((mm)->pgd + pgd_index(addr))


#define pgd_offset_k(addr)	pgd_offset(&init_mm, addr)

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#ifdef CONFIG_ARM_LPAE

#define pud_none(pud)		(!pud_val(pud))
#define pud_bad(pud)		(!(pud_val(pud) & 2))
#define pud_present(pud)	(pud_val(pud))

#define pud_clear(pudp)			\
	do {				\
		*pudp = __pud(0);	\
		clean_pmd_entry(pudp);	\
	} while (0)

#define set_pud(pudp, pud)		\
	do {				\
		*pudp = pud;		\
		flush_pmd_entry(pudp);	\
	} while (0)

static inline pmd_t *pud_page_vaddr(pud_t pud)
{
	return __va(pud_val(pud) & PHYS_MASK & (s32)PAGE_MASK);
}

#else	 

#define pud_none(pud)		(0)
#define pud_bad(pud)		(0)
#define pud_present(pud)	(1)
#define pud_clear(pudp)		do { } while (0)
#define set_pud(pud,pudp)	do { } while (0)

#endif	 
#elif defined(CONFIG_SYNO_ALPINE)
		 
#else
 
#define pgd_none(pgd)		(0)
#define pgd_bad(pgd)		(0)
#define pgd_present(pgd)	(1)
#define pgd_clear(pgdp)		do { } while (0)
#define set_pgd(pgd,pgdp)	do { } while (0)
#define set_pud(pud,pudp)	do { } while (0)

#endif


#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#ifdef CONFIG_ARM_LPAE
#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
static inline pmd_t *pmd_offset(pud_t *pud, unsigned long addr)
{
	return (pmd_t *)pud_page_vaddr(*pud) + pmd_index(addr);
}
#else
static inline pmd_t *pmd_offset(pud_t *pud, unsigned long addr)
{
	return (pmd_t *)pud;
}
#endif
#elif defined(CONFIG_SYNO_ALPINE)
	 
#else
#define pmd_offset(dir, addr)	((pmd_t *)(dir))
#endif

#define pmd_none(pmd)		(!pmd_val(pmd))
#define pmd_present(pmd)	(pmd_val(pmd))

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_ARM_LPAE)

#define pmd_bad(pmd)		(!(pmd_val(pmd) & 2))

#define copy_pmd(pmdpd,pmdps)		\
	do {				\
		*pmdpd = *pmdps;	\
		flush_pmd_entry(pmdpd);	\
	} while (0)

#define pmd_clear(pmdp)			\
	do {				\
		*pmdp = __pmd(0);	\
		clean_pmd_entry(pmdp);	\
	} while (0)

#elif defined(CONFIG_SYNO_ALPINE)
	 
#else	 

#define pmd_bad(pmd)		(pmd_val(pmd) & 2)

#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
#define copy_pmd(pmdpd,pmdps)		\
	do {				\
		pmdpd[0] = pmdps[0];	\
		pmdpd[1] = pmdps[1];	\
		flush_pmd_entry(pmdpd);	\
	} while (0)

#define pmd_clear(pmdp)			\
	do {				\
		pmdp[0] = __pmd(0);	\
		pmdp[1] = __pmd(0);	\
		clean_pmd_entry(pmdp);	\
	} while (0)

#else
#define copy_pmd(pmdpd,pmdps)	\
	do {	\
		int i;	\
		for(i = 0; i < LINKED_PMDS; i++)	\
			pmdpd[i] = pmdps[i];	\
		flush_pmd_entry(pmdpd);	\
	} while (0)

#define pmd_clear(pmdp)	\
	do {	\
		int i;	\
		for(i = 0; i < LINKED_PMDS; i++)	\
			pmdp[i] = __pmd(0);	\
		clean_pmd_entry(pmdp);	\
	} while (0)

#endif
#endif	 
 
#if defined(CONFIG_SYNO_COMCERTO)
#define PMD_PAGE_ADDR_MASK		(~((1 << 10) - 1))
#endif
static inline pte_t *pmd_page_vaddr(pmd_t pmd)
{
#if defined(CONFIG_SYNO_COMCERTO)
	return __va((pmd_val(pmd) & PHYS_MASK & (s32)PMD_PAGE_ADDR_MASK) - PTE_HWTABLE_OFF);
#elif defined(CONFIG_SYNO_ALPINE)
	return __va(pmd_val(pmd) & PHYS_MASK & (s32)PTE_HWTABLE_MASK);
#else
	return __va(pmd_val(pmd) & PHYS_MASK & (s32)PAGE_MASK);
#endif
}

#define pmd_page(pmd)		pfn_to_page(__phys_to_pfn(pmd_val(pmd) & PHYS_MASK))

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) || defined (CONFIG_SYNO_ALPINE)
#else
 
#define pmd_addr_end(addr,end)	(end)

#endif

#ifndef CONFIG_HIGHPTE
#define __pte_map(pmd)		pmd_page_vaddr(*(pmd))
#define __pte_unmap(pte)	do { } while (0)
#else
#define __pte_map(pmd)		(pte_t *)kmap_atomic(pmd_page(*(pmd)))
#define __pte_unmap(pte)	kunmap_atomic(pte)
#endif

#define pte_index(addr)		(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

#define pte_offset_kernel(pmd,addr)	(pmd_page_vaddr(*(pmd)) + pte_index(addr))

#define pte_offset_map(pmd,addr)	(__pte_map(pmd) + pte_index(addr))
#define pte_unmap(pte)			__pte_unmap(pte)

#define pte_pfn(pte)		((pte_val(pte) & PHYS_MASK) >> PAGE_SHIFT)
#define pfn_pte(pfn,prot)	__pte(__pfn_to_phys(pfn) | pgprot_val(prot))

#define pte_page(pte)		pfn_to_page(pte_pfn(pte))
#define mk_pte(page,prot)	pfn_pte(page_to_pfn(page), prot)

#if defined(CONFIG_SYNO_COMCERTO)
#define pte_clear(mm,addr,ptep)	do {__sync_outer_cache(ptep, __pte(0)); set_pte_ext(ptep, __pte(0), 0); } while (0)
#else
#define pte_clear(mm,addr,ptep)	set_pte_ext(ptep, __pte(0), 0)
#endif

#define pte_none(pte)		(!pte_val(pte))
#define pte_present(pte)	(pte_val(pte) & L_PTE_PRESENT)
#define pte_write(pte)		(!(pte_val(pte) & L_PTE_RDONLY))
#define pte_dirty(pte)		(pte_val(pte) & L_PTE_DIRTY)
#define pte_young(pte)		(pte_val(pte) & L_PTE_YOUNG)
#define pte_exec(pte)		(!(pte_val(pte) & L_PTE_XN))
#define pte_special(pte)	(0)

#define pte_present_user(pte) \
	((pte_val(pte) & (L_PTE_PRESENT | L_PTE_USER)) == \
	 (L_PTE_PRESENT | L_PTE_USER))

#if (defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)) && defined(CONFIG_ARM_LPAE)
#define set_pte_ext(ptep,pte,ext) cpu_set_pte_ext(ptep,__pte(pte_val(pte)|(ext)))
#elif defined(CONFIG_SYNO_COMCERTO)
#define set_pte_ext(ptep,pte,ext) cpu_set_pte_ext(ptep,pte_val(pte),ext)
#define uncache_pte_ext(ptep) cpu_uncache_pte_ext(ptep)
#elif defined(CONFIG_SYNO_ALPINE)
	 
#else
#define set_pte_ext(ptep,pte,ext) cpu_set_pte_ext(ptep,pte,ext)
#endif

#if defined(CONFIG_SYNO_COMCERTO)
#if !defined(CONFIG_L2X0_INSTRUCTION_ONLY)
static inline void __sync_outer_cache(pte_t *ptep, pte_t pteval)
{
}
#else
extern void __sync_outer_cache(pte_t *ptep, pte_t pteval);
#endif
#endif
#if __LINUX_ARM_ARCH__ < 6
static inline void __sync_icache_dcache(pte_t pteval)
{
}
#else
extern void __sync_icache_dcache(pte_t pteval);
#endif

static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pteval)
{
	unsigned long ext = 0;

#if defined(CONFIG_SYNO_COMCERTO)
	__sync_outer_cache(ptep, pteval);
#endif

	if (addr < TASK_SIZE && pte_present_user(pteval)) {
		__sync_icache_dcache(pteval);
		ext |= PTE_EXT_NG;
	}

	set_pte_ext(ptep, pteval, ext);
}

#define PTE_BIT_FUNC(fn,op) \
static inline pte_t pte_##fn(pte_t pte) { pte_val(pte) op; return pte; }

PTE_BIT_FUNC(wrprotect, |= L_PTE_RDONLY);
PTE_BIT_FUNC(mkwrite,   &= ~L_PTE_RDONLY);
PTE_BIT_FUNC(mkclean,   &= ~L_PTE_DIRTY);
PTE_BIT_FUNC(mkdirty,   |= L_PTE_DIRTY);
PTE_BIT_FUNC(mkold,     &= ~L_PTE_YOUNG);
PTE_BIT_FUNC(mkyoung,   |= L_PTE_YOUNG);

static inline pte_t pte_mkspecial(pte_t pte) { return pte; }

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	const pteval_t mask = L_PTE_XN | L_PTE_RDONLY | L_PTE_USER;
	pte_val(pte) = (pte_val(pte) & ~mask) | (pgprot_val(newprot) & mask);
	return pte;
}

#define __SWP_TYPE_SHIFT	3
#define __SWP_TYPE_BITS		5
#define __SWP_TYPE_MASK		((1 << __SWP_TYPE_BITS) - 1)
#define __SWP_OFFSET_SHIFT	(__SWP_TYPE_BITS + __SWP_TYPE_SHIFT)

#define __swp_type(x)		(((x).val >> __SWP_TYPE_SHIFT) & __SWP_TYPE_MASK)
#define __swp_offset(x)		((x).val >> __SWP_OFFSET_SHIFT)
#define __swp_entry(type,offset) ((swp_entry_t) { ((type) << __SWP_TYPE_SHIFT) | ((offset) << __SWP_OFFSET_SHIFT) })

#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#if defined(CONFIG_SYNO_COMCERTO)
#define __swp_entry_to_pte(swp)	((pte_t) { { (swp).val } })
#else
#define __swp_entry_to_pte(swp)	((pte_t) { (swp).val })
#endif

#define MAX_SWAPFILES_CHECK() BUILD_BUG_ON(MAX_SWAPFILES_SHIFT > __SWP_TYPE_BITS)

#define pte_file(pte)		(pte_val(pte) & L_PTE_FILE)
#define pte_to_pgoff(x)		(pte_val(x) >> 3)
#define pgoff_to_pte(x)		__pte(((x) << 3) | L_PTE_FILE)

#define PTE_FILE_MAX_BITS	29

#define kern_addr_valid(addr)	(1)

#include <asm-generic/pgtable.h>

#define HAVE_ARCH_UNMAPPED_AREA

#define io_remap_pfn_range(vma,from,pfn,size,prot) \
		remap_pfn_range(vma, from, pfn, size, prot)

#define pgtable_cache_init() do { } while (0)

void identity_mapping_add(pgd_t *, unsigned long, unsigned long);
void identity_mapping_del(pgd_t *, unsigned long, unsigned long);

#endif  

#endif  

#endif  
