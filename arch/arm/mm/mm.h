#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef CONFIG_MMU

/* the upper-most page table pointer */
extern pmd_t *top_pmd;

#if defined(CONFIG_SYNO_ARMADA_ARCH) && defined(CONFIG_MV_SUPPORT_64KB_PAGE_SIZE)
#define TOP_PTE(x)	pte_offset_kernel(pmd_off_k(x), x)
#else
#define TOP_PTE(x)	pte_offset_kernel(top_pmd, x)
#endif

static inline pmd_t *pmd_off_k(unsigned long virt)
{
	return pmd_offset(pud_offset(pgd_offset_k(virt), virt), virt);
}

struct mem_type {
	pteval_t prot_pte;
	pmdval_t prot_l1;
	pmdval_t prot_sect;
	unsigned int domain;
};

const struct mem_type *get_mem_type(unsigned int type);

extern void __flush_dcache_page(struct address_space *mapping, struct page *page);

#endif

#ifdef CONFIG_ZONE_DMA
extern u32 arm_dma_limit;
#else
#define arm_dma_limit ((u32)~0)
#endif

void __init bootmem_init(void);
void arm_mm_memblock_reserve(void);
