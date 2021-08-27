#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 

#ifndef _ASM_PGTABLE_2LEVEL_H
#define _ASM_PGTABLE_2LEVEL_H



#if !defined(CONFIG_SYNO_COMCERTO) || !defined(CONFIG_COMCERTO_64K_PAGES)
#ifdef CONFIG_SYNO_ARMADA_ARCH
#ifdef CONFIG_MV_SUPPORT_64KB_PAGE_SIZE
#define PTRS_PER_PTE           32       
#define PTE_HWTABLE_PTRS       (512)
#else
#define PTRS_PER_PTE           512
#define PTE_HWTABLE_PTRS       (PTRS_PER_PTE)
#endif  
#elif defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#ifdef CONFIG_MV_LARGE_PAGE_SUPPORT
#define PTRS_PER_PTE           (512 >> (PAGE_SHIFT-12))
#define PTE_HWTABLE_PTRS       (512)
#else
#define PTRS_PER_PTE           512
#define PTE_HWTABLE_PTRS       (PTRS_PER_PTE)
#endif  
#else
#define PTRS_PER_PTE		512
#endif  

#define PTRS_PER_PMD		1
#define PTRS_PER_PGD		2048

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2) 
#define PTE_HWTABLE_OFF                (512 * sizeof(pte_t))
#define PTE_HWTABLE_SIZE       (PTE_HWTABLE_PTRS * sizeof(u32))
#else
#define PTE_HWTABLE_PTRS	(PTRS_PER_PTE)
#define PTE_HWTABLE_OFF		(PTE_HWTABLE_PTRS * sizeof(pte_t))
#ifdef CONFIG_SYNO_ALPINE
#define PTE_HWTABLE_SIZE	(PTE_HWTABLE_PTRS * sizeof(u32))
#define PTE_HWTABLE_MASK	(~((PTE_HWTABLE_SIZE*2)-1))
#else
#define PTE_HWTABLE_SIZE	(PTRS_PER_PTE * sizeof(u32))
#endif
#endif

#define PMD_SHIFT		21
#define PGDIR_SHIFT		21
#else
#define LINKED_PMDS_SHIFT	4
#define LINKED_PMDS			(1 << LINKED_PMDS_SHIFT)	 
#define PTRS_PER_PGD		(4096 / LINKED_PMDS)		 
#define PGDIR_SHIFT			(20 + LINKED_PMDS_SHIFT)	 
#define PMD_SHIFT			(PGDIR_SHIFT)
#define PTET_SIZE_SHIFT		6							 
#define PTE_HWTABLE_PTRS	(1 << (10 + LINKED_PMDS_SHIFT - PTET_SIZE_SHIFT))  

#define PTRS_PER_PTE		(PTE_HWTABLE_PTRS)
#define PTRS_PER_PMD		1

#define PTE_HWTABLE_OFF		32768  
#define PTE_HWTABLE_SIZE	(1 << (10 + LINKED_PMDS_SHIFT))
#endif

#define PMD_SIZE		(1UL << PMD_SHIFT)
#define PMD_MASK		(~(PMD_SIZE-1))
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE-1))

#define SECTION_SHIFT		20
#define SECTION_SIZE		(1UL << SECTION_SHIFT)
#define SECTION_MASK		(~(SECTION_SIZE-1))

#define SUPERSECTION_SHIFT	24
#define SUPERSECTION_SIZE	(1UL << SUPERSECTION_SHIFT)
#define SUPERSECTION_MASK	(~(SUPERSECTION_SIZE-1))

#define USER_PTRS_PER_PGD	(TASK_SIZE / PGDIR_SIZE)

#define L_PTE_PRESENT		(_AT(pteval_t, 1) << 0)
#define L_PTE_YOUNG		(_AT(pteval_t, 1) << 1)
#define L_PTE_FILE		(_AT(pteval_t, 1) << 2)	 
#define L_PTE_DIRTY		(_AT(pteval_t, 1) << 6)
#define L_PTE_RDONLY		(_AT(pteval_t, 1) << 7)
#define L_PTE_USER		(_AT(pteval_t, 1) << 8)
#define L_PTE_XN		(_AT(pteval_t, 1) << 9)
#define L_PTE_SHARED		(_AT(pteval_t, 1) << 10)	 

#define L_PTE_MT_UNCACHED	(_AT(pteval_t, 0x00) << 2)	 
#define L_PTE_MT_BUFFERABLE	(_AT(pteval_t, 0x01) << 2)	 
#define L_PTE_MT_WRITETHROUGH	(_AT(pteval_t, 0x02) << 2)	 
#define L_PTE_MT_WRITEBACK	(_AT(pteval_t, 0x03) << 2)	 
#define L_PTE_MT_MINICACHE	(_AT(pteval_t, 0x06) << 2)	 
#define L_PTE_MT_WRITEALLOC	(_AT(pteval_t, 0x07) << 2)	 
#define L_PTE_MT_DEV_SHARED	(_AT(pteval_t, 0x04) << 2)	 
#define L_PTE_MT_DEV_NONSHARED	(_AT(pteval_t, 0x0c) << 2)	 
#define L_PTE_MT_DEV_WC		(_AT(pteval_t, 0x09) << 2)	 
#define L_PTE_MT_DEV_CACHED	(_AT(pteval_t, 0x0b) << 2)	 
#define L_PTE_MT_MASK		(_AT(pteval_t, 0x0f) << 2)
#ifdef CONFIG_SYNO_ALPINE
#ifndef __ASSEMBLY__


#define pud_none(pud)		(0)
#define pud_bad(pud)		(0)
#define pud_present(pud)	(1)
#define pud_clear(pudp)		do { } while (0)
#define set_pud(pud,pudp)	do { } while (0)

static inline pmd_t *pmd_offset(pud_t *pud, unsigned long addr)
{
	return (pmd_t *)pud;
}

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

#define pmd_addr_end(addr,end) (end)

#define set_pte_ext(ptep,pte,ext) cpu_set_pte_ext(ptep,pte,ext)

#endif  
#endif

#endif  
