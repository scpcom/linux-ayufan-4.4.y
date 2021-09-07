 
#ifndef _ASMARM_PGTABLE_H
#define _ASMARM_PGTABLE_H

#include <asm-generic/4level-fixup.h>
#include <asm/proc-fns.h>

#ifndef CONFIG_MMU

#include "pgtable-nommu.h"

#else

#include <asm/memory.h>
#include <mach/vmalloc.h>
#include <asm/pgtable-hwdef.h>

#ifndef VMALLOC_START
#define VMALLOC_OFFSET		(8*1024*1024)
#define VMALLOC_START		(((unsigned long)high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1))
#endif

#define PTRS_PER_PTE		512
#define PTRS_PER_PMD		1
#define PTRS_PER_PGD		2048

#define PMD_SHIFT		21
#define PGDIR_SHIFT		21

#define LIBRARY_TEXT_START	0x0c000000

#ifndef __ASSEMBLY__
extern void __pte_error(const char *file, int line, unsigned long val);
extern void __pmd_error(const char *file, int line, unsigned long val);
extern void __pgd_error(const char *file, int line, unsigned long val);

#define pte_ERROR(pte)		__pte_error(__FILE__, __LINE__, pte_val(pte))
#define pmd_ERROR(pmd)		__pmd_error(__FILE__, __LINE__, pmd_val(pmd))
#define pgd_ERROR(pgd)		__pgd_error(__FILE__, __LINE__, pgd_val(pgd))
#endif  

#define PMD_SIZE		(1UL << PMD_SHIFT)
#define PMD_MASK		(~(PMD_SIZE-1))
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE-1))

#define FIRST_USER_ADDRESS	PAGE_SIZE

#define FIRST_USER_PGD_NR	1
#define USER_PTRS_PER_PGD	((TASK_SIZE/PGDIR_SIZE) - FIRST_USER_PGD_NR)

#define SECTION_SHIFT		20
#define SECTION_SIZE		(1UL << SECTION_SHIFT)
#define SECTION_MASK		(~(SECTION_SIZE-1))

#define SUPERSECTION_SHIFT	24
#define SUPERSECTION_SIZE	(1UL << SUPERSECTION_SHIFT)
#define SUPERSECTION_MASK	(~(SUPERSECTION_SIZE-1))

#define L_PTE_PRESENT		(1 << 0)
#define L_PTE_YOUNG		(1 << 1)
#define L_PTE_FILE		(1 << 2)	 
#define L_PTE_DIRTY		(1 << 6)
#define L_PTE_WRITE		(1 << 7)
#define L_PTE_USER		(1 << 8)
#define L_PTE_EXEC		(1 << 9)
#define L_PTE_SHARED		(1 << 10)	 

#define L_PTE_MT_UNCACHED	(0x00 << 2)	 
#define L_PTE_MT_BUFFERABLE	(0x01 << 2)	 
#define L_PTE_MT_WRITETHROUGH	(0x02 << 2)	 
#define L_PTE_MT_WRITEBACK	(0x03 << 2)	 
#define L_PTE_MT_MINICACHE	(0x06 << 2)	 
#define L_PTE_MT_WRITEALLOC	(0x07 << 2)	 
#define L_PTE_MT_DEV_SHARED	(0x04 << 2)	 
#define L_PTE_MT_DEV_NONSHARED	(0x0c << 2)	 
#define L_PTE_MT_DEV_WC		(0x09 << 2)	 
#define L_PTE_MT_DEV_CACHED	(0x0b << 2)	 
#define L_PTE_MT_MASK		(0x0f << 2)

#ifndef __ASSEMBLY__

#define _L_PTE_DEFAULT	L_PTE_PRESENT | L_PTE_YOUNG

extern pgprot_t		pgprot_user;
extern pgprot_t		pgprot_kernel;

#define _MOD_PROT(p, b)	__pgprot(pgprot_val(p) | (b))

#define PAGE_NONE		pgprot_user
#define PAGE_SHARED		_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_WRITE)
#define PAGE_SHARED_EXEC	_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_WRITE | L_PTE_EXEC)
#define PAGE_COPY		_MOD_PROT(pgprot_user, L_PTE_USER)
#define PAGE_COPY_EXEC		_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_EXEC)
#define PAGE_READONLY		_MOD_PROT(pgprot_user, L_PTE_USER)
#define PAGE_READONLY_EXEC	_MOD_PROT(pgprot_user, L_PTE_USER | L_PTE_EXEC)
#define PAGE_KERNEL		pgprot_kernel
#define PAGE_KERNEL_EXEC	_MOD_PROT(pgprot_kernel, L_PTE_EXEC)

#define __PAGE_NONE		__pgprot(_L_PTE_DEFAULT)
#define __PAGE_SHARED		__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_WRITE)
#define __PAGE_SHARED_EXEC	__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_WRITE | L_PTE_EXEC)
#define __PAGE_COPY		__pgprot(_L_PTE_DEFAULT | L_PTE_USER)
#define __PAGE_COPY_EXEC	__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_EXEC)
#define __PAGE_READONLY		__pgprot(_L_PTE_DEFAULT | L_PTE_USER)
#define __PAGE_READONLY_EXEC	__pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_EXEC)

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

#define pte_pfn(pte)		(pte_val(pte) >> PAGE_SHIFT)
#define pfn_pte(pfn,prot)	(__pte(((pfn) << PAGE_SHIFT) | pgprot_val(prot)))

#define pte_none(pte)		(!pte_val(pte))
#define pte_clear(mm,addr,ptep)	set_pte_ext(ptep, __pte(0), 0)
#define pte_page(pte)		(pfn_to_page(pte_pfn(pte)))
#define pte_offset_kernel(dir,addr)	(pmd_page_vaddr(*(dir)) + __pte_index(addr))

#define pte_offset_map(dir,addr)	(__pte_map(dir, KM_PTE0) + __pte_index(addr))
#define pte_offset_map_nested(dir,addr)	(__pte_map(dir, KM_PTE1) + __pte_index(addr))
#define pte_unmap(pte)			__pte_unmap(pte, KM_PTE0)
#define pte_unmap_nested(pte)		__pte_unmap(pte, KM_PTE1)

#ifndef CONFIG_HIGHPTE
#define __pte_map(dir,km)	pmd_page_vaddr(*(dir))
#define __pte_unmap(pte,km)	do { } while (0)
#else
#define __pte_map(dir,km)	((pte_t *)kmap_atomic(pmd_page(*(dir)), km) + PTRS_PER_PTE)
#define __pte_unmap(pte,km)	kunmap_atomic((pte - PTRS_PER_PTE), km)
#endif

#define set_pte_ext(ptep,pte,ext) cpu_set_pte_ext(ptep,pte,ext)

#if defined(CONFIG_SYNO_PLX_PORTING)
#ifndef CONFIG_SMP
static inline void __sync_icache_dcache(pte_t pteval)
{
}
#else
extern void __sync_icache_dcache(pte_t pteval);
#endif

static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pteval)
{
	if (addr >= TASK_SIZE)
		set_pte_ext(ptep, pteval, 0);
	else {
		__sync_icache_dcache(pteval);
		set_pte_ext(ptep, pteval, PTE_EXT_NG);
	}
}
#else
#define set_pte_at(mm,addr,ptep,pteval) do { \
	set_pte_ext(ptep, pteval, (addr) >= TASK_SIZE ? 0 : PTE_EXT_NG); \
 } while (0)
#endif

#define pte_present(pte)	(pte_val(pte) & L_PTE_PRESENT)
#define pte_write(pte)		(pte_val(pte) & L_PTE_WRITE)
#define pte_dirty(pte)		(pte_val(pte) & L_PTE_DIRTY)
#define pte_young(pte)		(pte_val(pte) & L_PTE_YOUNG)
#define pte_special(pte)	(0)

#if defined(CONFIG_SYNO_PLX_PORTING)
#define pte_present_exec_user(pte) \
	((pte_val(pte) & (L_PTE_PRESENT | L_PTE_EXEC | L_PTE_USER)) == \
	 (L_PTE_PRESENT | L_PTE_EXEC | L_PTE_USER))
#endif

#define PTE_BIT_FUNC(fn,op) \
static inline pte_t pte_##fn(pte_t pte) { pte_val(pte) op; return pte; }

PTE_BIT_FUNC(wrprotect, &= ~L_PTE_WRITE);
PTE_BIT_FUNC(mkwrite,   |= L_PTE_WRITE);
PTE_BIT_FUNC(mkclean,   &= ~L_PTE_DIRTY);
PTE_BIT_FUNC(mkdirty,   |= L_PTE_DIRTY);
PTE_BIT_FUNC(mkold,     &= ~L_PTE_YOUNG);
PTE_BIT_FUNC(mkyoung,   |= L_PTE_YOUNG);

static inline pte_t pte_mkspecial(pte_t pte) { return pte; }

#define pgprot_noncached(prot) \
	__pgprot((pgprot_val(prot) & ~L_PTE_MT_MASK) | L_PTE_MT_UNCACHED)
#define pgprot_writecombine(prot) \
	__pgprot((pgprot_val(prot) & ~L_PTE_MT_MASK) | L_PTE_MT_BUFFERABLE)

#define pmd_none(pmd)		(!pmd_val(pmd))
#define pmd_present(pmd)	(pmd_val(pmd))
#define pmd_bad(pmd)		(pmd_val(pmd) & 2)

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

static inline pte_t *pmd_page_vaddr(pmd_t pmd)
{
	unsigned long ptr;

	ptr = pmd_val(pmd) & ~(PTRS_PER_PTE * sizeof(void *) - 1);
	ptr += PTRS_PER_PTE * sizeof(void *);

	return __va(ptr);
}

#define pmd_page(pmd)		pfn_to_page(__phys_to_pfn(pmd_val(pmd)))

#define mk_pte(page,prot)	pfn_pte(page_to_pfn(page),prot)

#define pgd_none(pgd)		(0)
#define pgd_bad(pgd)		(0)
#define pgd_present(pgd)	(1)
#define pgd_clear(pgdp)		do { } while (0)
#define set_pgd(pgd,pgdp)	do { } while (0)

#define pgd_index(addr)		((addr) >> PGDIR_SHIFT)

#define pgd_offset(mm, addr)	((mm)->pgd+pgd_index(addr))

#define pgd_offset_k(addr)	pgd_offset(&init_mm, addr)

#define pmd_offset(dir, addr)	((pmd_t *)(dir))

#define __pte_index(addr)	(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	const unsigned long mask = L_PTE_EXEC | L_PTE_WRITE | L_PTE_USER;
	pte_val(pte) = (pte_val(pte) & ~mask) | (pgprot_val(newprot) & mask);
	return pte;
}

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];

#define __SWP_TYPE_SHIFT	3
#define __SWP_TYPE_BITS		6
#define __SWP_TYPE_MASK		((1 << __SWP_TYPE_BITS) - 1)
#define __SWP_OFFSET_SHIFT	(__SWP_TYPE_BITS + __SWP_TYPE_SHIFT)

#define __swp_type(x)		(((x).val >> __SWP_TYPE_SHIFT) & __SWP_TYPE_MASK)
#define __swp_offset(x)		((x).val >> __SWP_OFFSET_SHIFT)
#define __swp_entry(type,offset) ((swp_entry_t) { ((type) << __SWP_TYPE_SHIFT) | ((offset) << __SWP_OFFSET_SHIFT) })

#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(swp)	((pte_t) { (swp).val })

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

#endif  

#endif  

#endif  
