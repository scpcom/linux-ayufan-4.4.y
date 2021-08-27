#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef CONFIG_MMU

/* the upper-most page table pointer */
extern pmd_t *top_pmd;

#if (defined(CONFIG_SYNO_ARMADA_ARCH__V2) && defined(CONFIG_MV_LARGE_PAGE_SUPPORT)) ||\
     (defined(CONFIG_SYNO_ARMADA_ARCH) && defined(CONFIG_MV_SUPPORT_64KB_PAGE_SIZE))
#define TOP_PTE(x)	pte_offset_kernel(pmd_off_k(x), x)
#else
#define TOP_PTE(x)	pte_offset_kernel(top_pmd, x)
#endif

static inline pmd_t *pmd_off_k(unsigned long virt)
{
	return pmd_offset(pud_offset(pgd_offset_k(virt), virt), virt);
}

#ifdef CONFIG_SYNO_ALPINE
static inline void set_fix_pte(unsigned long va, pte_t pte)
{
#if defined (CONFIG_ARM_PAGE_SIZE_LARGE) && defined(CONFIG_HIGHMEM)
	pte_t *ptep = pte_offset_kernel(pmd_off_k(va), va);
	set_pte_ext(ptep, pte, 0);
	local_flush_tlb_kernel_page(va);
#else
	pte_t *ptep = TOP_PTE(va);
	set_pte_ext(ptep, pte, 0);
	local_flush_tlb_kernel_page(va);
#endif
}

static inline pte_t get_fix_pte(unsigned long va)
{
#if defined (CONFIG_ARM_PAGE_SIZE_LARGE) && defined(CONFIG_HIGHMEM)
	pte_t *ptep = pte_offset_kernel(pmd_off_k(va), va);
	return *ptep;
#else
	pte_t *ptep = TOP_PTE(va);
	return *ptep;
#endif
}
#endif
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
#ifdef CONFIG_SYNO_ALPINE
extern phys_addr_t arm_dma_limit;
#else
extern u32 arm_dma_limit;
#endif
#else
#ifdef CONFIG_SYNO_ALPINE
#define arm_dma_limit (PHYS_MASK)
#else
#define arm_dma_limit ((u32)~0)
#endif
#endif

void __init bootmem_init(void);
void arm_mm_memblock_reserve(void);
