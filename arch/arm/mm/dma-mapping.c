/*
 *  linux/arch/arm/mm/dma-mapping.c
 *
 *  Copyright (C) 2000-2004 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  DMA uncached mapping support.
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/slab.h>

#include <asm/memory.h>
#include <asm/highmem.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/sizes.h>
#include <asm/mach/arch.h>

#include "mm.h"
#if defined(CONFIG_COMCERTO_UNCACHED_DMA)
#include <linux/hugetlb.h>
#include <asm/pgalloc.h>
#include <asm/mach/map.h>
#endif

#ifdef CONFIG_COMCERTO_ZONE_DMA_NCNB
extern unsigned long arm_dma_zone_size;
#endif

#if defined(CONFIG_COMCERTO_UNCACHED_DMA)
static pgd_t *shadow_pg_dir;
static atomic_t *shadow_pmd_count;
static int __init init_shadow_page_table(void)
{
	pmd_t *pmd, *shadow_pmd;
	pte_t *shadow_pte, *ptep;
	unsigned long start, addr, end, pfn;
	const struct mem_type *mt;
	int count;

	shadow_pg_dir = (pgd_t *)__get_free_pages(GFP_KERNEL | GFP_ATOMIC, get_order(16384));
	if (!shadow_pg_dir)
		return -ENOMEM;
	shadow_pmd_count = (atomic_t *)__get_free_pages(GFP_KERNEL | GFP_ATOMIC, get_order(sizeof(atomic_t) * PTRS_PER_PGD));
	if (!shadow_pmd_count)
		goto err1;

	memset(shadow_pg_dir, 0, 16384);
	memset(shadow_pmd_count, 0, sizeof(atomic_t) * PTRS_PER_PGD);

	mt = get_mem_type(MT_MEMORY);
	start = 0;
	count = 0;
	do {
		pmd = pmd_off_k((unsigned long) start);
		if (!pmd_none(*pmd)) {
			if (pmd_bad(*pmd) && ((pmd_val(*pmd) & ~SECTION_MASK) == mt->prot_sect)) {  // Only do it for MT_MEMORY areas
				shadow_pmd = (pmd_t *)shadow_pg_dir + (pmd - pmd_off_k(0));
				addr = (unsigned long)start & PMD_MASK;
				end = addr + PMD_SIZE;

				shadow_pte = (pte_t *)__get_free_page(PGALLOC_GFP | GFP_ATOMIC);
				if (!shadow_pte)
					goto err2;
				pfn = __phys_to_pfn(pmd_val(*pmd) & PMD_MASK);
				ptep = shadow_pte;
				do {
					set_pte_ext(ptep, pfn_pte(pfn, __pgprot(mt->prot_pte)), 0);
					pfn++;
				} while (ptep++, addr += PAGE_SIZE, addr != end);
				__pmd_populate(shadow_pmd, __pa(shadow_pte), mt->prot_l1);
			} else {
				// Mark the shadow in use, so we never replace the already existing 2nd-level
				atomic_inc(&shadow_pmd_count[pgd_index(start)]);
			}
		}
	} while (count++, start += PMD_SIZE, count < PTRS_PER_PGD);

	return 0;

err2:
	__free_pages((struct page *)shadow_pmd_count, get_order(sizeof(u16) * PTRS_PER_PGD));
	shadow_pmd_count = NULL;
	//TODO: free already allocated shadow_pte tables
err1:
	__free_pages((struct page *)shadow_pg_dir, get_order(16384));
	return -ENOMEM;
}
core_initcall(init_shadow_page_table);
#endif

static u64 get_coherent_dma_mask(struct device *dev)
{
	u64 mask = (u64)arm_dma_limit;

	if (dev) {
		mask = dev->coherent_dma_mask;

		/*
		 * Sanity check the DMA mask - it must be non-zero, and
		 * must be able to be satisfied by a DMA allocation.
		 */
		if (mask == 0) {
			dev_warn(dev, "coherent DMA mask is unset\n");
			return 0;
		}

		if ((~mask) & (u64)arm_dma_limit) {
			dev_warn(dev, "coherent DMA mask %#llx is smaller "
				 "than system GFP_DMA mask %#llx\n",
				 mask, (u64)arm_dma_limit);
			return 0;
		}
	}

	return mask;
}

/*
 * Allocate a DMA buffer for 'dev' of size 'size' using the
 * specified gfp mask.  Note that 'size' must be page aligned.
 */
static struct page *__dma_alloc_buffer(struct device *dev, size_t size, gfp_t gfp)
{
	unsigned long order = get_order(size);
	struct page *page, *p, *e;
	void *ptr;
	u64 mask = get_coherent_dma_mask(dev);

#ifdef CONFIG_DMA_API_DEBUG
	u64 limit = (mask + 1) & ~mask;
	if (limit && size >= limit) {
		dev_warn(dev, "coherent allocation too big (requested %#x mask %#llx)\n",
			size, mask);
		return NULL;
	}
#endif

	if (!mask)
		return NULL;

	if (mask < 0xffffffffULL)
		gfp |= GFP_DMA;

	page = alloc_pages(gfp, order);
	if (!page)
		return NULL;

	/*
	 * Now split the huge page and free the excess pages
	 */
	split_page(page, order);
	for (p = page + (size >> PAGE_SHIFT), e = page + (1 << order); p < e; p++)
		__free_page(p);

	/*
	 * Ensure that the allocated pages are zeroed, and that any data
	 * lurking in the kernel direct-mapped region is invalidated.
	 */
	ptr = page_address(page);
	memset(ptr, 0, size);
	dmac_flush_range(ptr, ptr + size);
	outer_flush_range(__pa(ptr), __pa(ptr) + size);

	return page;
}

/*
 * Free a DMA buffer.  'size' must be page aligned.
 */
static void __dma_free_buffer(struct page *page, size_t size)
{
	struct page *e = page + (size >> PAGE_SHIFT);

	while (page < e) {
		__free_page(page);
		page++;
	}
}

#ifdef CONFIG_MMU

#define CONSISTENT_OFFSET(x)	(((unsigned long)(x) - consistent_base) >> PAGE_SHIFT)
#define CONSISTENT_PTE_INDEX(x) (((unsigned long)(x) - consistent_base) >> PMD_SHIFT)

/*
 * These are the page tables (2MB each) covering uncached, DMA consistent allocations
 */
static pte_t **consistent_pte;

#define DEFAULT_CONSISTENT_DMA_SIZE SZ_2M

unsigned long consistent_base = CONSISTENT_END - DEFAULT_CONSISTENT_DMA_SIZE;

void __init init_consistent_dma_size(unsigned long size)
{
	unsigned long base = CONSISTENT_END - ALIGN(size, SZ_2M);

	BUG_ON(consistent_pte); /* Check we're called before DMA region init */
	BUG_ON(base < VMALLOC_END);

	/* Grow region to accommodate specified size  */
	if (base < consistent_base)
		consistent_base = base;
}

#include "vmregion.h"

static struct arm_vmregion_head consistent_head = {
	.vm_lock	= __SPIN_LOCK_UNLOCKED(&consistent_head.vm_lock),
	.vm_list	= LIST_HEAD_INIT(consistent_head.vm_list),
	.vm_end		= CONSISTENT_END,
};

#ifdef CONFIG_HUGETLB_PAGE
#error ARM Coherent DMA allocator does not (yet) support huge TLB
#endif

/*
 * Initialise the consistent memory allocation.
 */
static int __init consistent_init(void)
{
	int ret = 0;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int i = 0;
	unsigned long base = consistent_base;
	unsigned long num_ptes = (CONSISTENT_END - base + PMD_SIZE -1) >> PMD_SHIFT;

	consistent_pte = kmalloc(num_ptes * sizeof(pte_t), GFP_KERNEL);
	if (!consistent_pte) {
		pr_err("%s: no memory\n", __func__);
		return -ENOMEM;
	}

	pr_debug("DMA memory: 0x%08lx - 0x%08lx:\n", base, CONSISTENT_END);
	consistent_head.vm_start = base;

	do {
		pgd = pgd_offset(&init_mm, base);

		pud = pud_alloc(&init_mm, pgd, base);
		if (!pud) {
			printk(KERN_ERR "%s: no pud tables\n", __func__);
			ret = -ENOMEM;
			break;
		}

		pmd = pmd_alloc(&init_mm, pud, base);
		if (!pmd) {
			printk(KERN_ERR "%s: no pmd tables\n", __func__);
			ret = -ENOMEM;
			break;
		}
#if !defined(CONFIG_COMCERTO_64K_PAGES)
		WARN_ON(!pmd_none(*pmd));
#endif
		pte = pte_alloc_kernel(pmd, base);
		if (!pte) {
			printk(KERN_ERR "%s: no pte tables\n", __func__);
			ret = -ENOMEM;
			break;
		}

		consistent_pte[i++] = pte;
		base = (base + PMD_SIZE) & PMD_MASK;
	} while ((base-1) < (CONSISTENT_END - 1));

	return ret;
}

core_initcall(consistent_init);

static void *
__dma_alloc_remap(struct page *page, size_t size, gfp_t gfp, pgprot_t prot)
{
	struct arm_vmregion *c;
	size_t align;
	int bit;

	if (!consistent_pte) {
		printk(KERN_ERR "%s: not initialised\n", __func__);
		dump_stack();
		return NULL;
	}

	/*
	 * Align the virtual region allocation - maximum alignment is
	 * a section size, minimum is a page size.  This helps reduce
	 * fragmentation of the DMA space, and also prevents allocations
	 * smaller than a section from crossing a section boundary.
	 */
	bit = fls(size - 1);
	if (bit > SECTION_SHIFT)
		bit = SECTION_SHIFT;
	align = 1 << bit;

	/*
	 * Allocate a virtual address in the consistent mapping region.
	 */
	c = arm_vmregion_alloc(&consistent_head, align, size,
			    gfp & ~(__GFP_DMA | __GFP_HIGHMEM));
	if (c) {
		pte_t *pte;
		int idx = CONSISTENT_PTE_INDEX(c->vm_start);
		u32 off = CONSISTENT_OFFSET(c->vm_start) & (PTRS_PER_PTE-1);

		pte = consistent_pte[idx] + off;
		c->vm_pages = page;

		do {
			BUG_ON(!pte_none(*pte));

			set_pte_ext(pte, mk_pte(page, prot), 0);
			page++;
			pte++;
			off++;
			if (off >= PTRS_PER_PTE) {
				off = 0;
				pte = consistent_pte[++idx];
			}
		} while (size -= PAGE_SIZE);

		dsb();

		return (void *)c->vm_start;
	}
	return NULL;
}

static void __dma_free_remap(void *cpu_addr, size_t size)
{
	struct arm_vmregion *c;
	unsigned long addr;
	pte_t *ptep;
	int idx;
	u32 off;

	c = arm_vmregion_find_remove(&consistent_head, (unsigned long)cpu_addr);
	if (!c) {
		printk(KERN_ERR "%s: trying to free invalid coherent area: %p\n",
		       __func__, cpu_addr);
		dump_stack();
		return;
	}

	if ((c->vm_end - c->vm_start) != size) {
		printk(KERN_ERR "%s: freeing wrong coherent size (%ld != %d)\n",
		       __func__, c->vm_end - c->vm_start, size);
		dump_stack();
		size = c->vm_end - c->vm_start;
	}

	idx = CONSISTENT_PTE_INDEX(c->vm_start);
	off = CONSISTENT_OFFSET(c->vm_start) & (PTRS_PER_PTE-1);
	ptep = consistent_pte[idx] + off;
	addr = c->vm_start;
	do {
		pte_t pte = ptep_get_and_clear(&init_mm, addr, ptep);

		ptep++;
		addr += PAGE_SIZE;
		off++;
		if (off >= PTRS_PER_PTE) {
			off = 0;
			ptep = consistent_pte[++idx];
		}

		if (pte_none(pte) || !pte_present(pte))
			printk(KERN_CRIT "%s: bad page in kernel page table\n",
			       __func__);
	} while (size -= PAGE_SIZE);

	flush_tlb_kernel_range(c->vm_start, c->vm_end);

	arm_vmregion_free(&consistent_head, c);
}

#else	/* !CONFIG_MMU */

#define __dma_alloc_remap(page, size, gfp, prot)	page_address(page)
#define __dma_free_remap(addr, size)			do { } while (0)

#endif	/* CONFIG_MMU */

static void *
__dma_alloc(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp,
	    pgprot_t prot)
{
	struct page *page;
	void *addr;

	/*
	 * Following is a work-around (a.k.a. hack) to prevent pages
	 * with __GFP_COMP being passed to split_page() which cannot
	 * handle them.  The real problem is that this flag probably
	 * should be 0 on ARM as it is not supported on this
	 * platform; see CONFIG_HUGETLBFS.
	 */
	gfp &= ~(__GFP_COMP);

	*handle = ~0;
	size = PAGE_ALIGN(size);

	page = __dma_alloc_buffer(dev, size, gfp);
	if (!page)
		return NULL;

	if (!arch_is_coherent())
		addr = __dma_alloc_remap(page, size, gfp, prot);
	else
		addr = page_address(page);

	if (addr)
		*handle = pfn_to_dma(dev, page_to_pfn(page));
	else
		__dma_free_buffer(page, size);

	return addr;
}

/*
 * Allocate DMA-coherent memory space and return both the kernel remapped
 * virtual and bus address for that space.
 */
void *
dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp)
{
	void *memory;

	if (dma_alloc_from_coherent(dev, size, handle, &memory))
		return memory;

	return __dma_alloc(dev, size, handle, gfp,
			   pgprot_dmacoherent(pgprot_kernel));
}
EXPORT_SYMBOL(dma_alloc_coherent);

/*
 * Allocate a writecombining region, in much the same way as
 * dma_alloc_coherent above.
 */
void *
dma_alloc_writecombine(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp)
{
	return __dma_alloc(dev, size, handle, gfp,
			   pgprot_writecombine(pgprot_kernel));
}
EXPORT_SYMBOL(dma_alloc_writecombine);

static int dma_mmap(struct device *dev, struct vm_area_struct *vma,
		    void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	int ret = -ENXIO;
#ifdef CONFIG_MMU
	unsigned long user_size, kern_size;
	struct arm_vmregion *c;

	user_size = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	c = arm_vmregion_find(&consistent_head, (unsigned long)cpu_addr);
	if (c) {
		unsigned long off = vma->vm_pgoff;

		kern_size = (c->vm_end - c->vm_start) >> PAGE_SHIFT;

		if (off < kern_size &&
		    user_size <= (kern_size - off)) {
			ret = remap_pfn_range(vma, vma->vm_start,
					      page_to_pfn(c->vm_pages) + off,
					      user_size << PAGE_SHIFT,
					      vma->vm_page_prot);
		}
	}
#endif	/* CONFIG_MMU */

	return ret;
}

int dma_mmap_coherent(struct device *dev, struct vm_area_struct *vma,
		      void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	vma->vm_page_prot = pgprot_dmacoherent(vma->vm_page_prot);
	return dma_mmap(dev, vma, cpu_addr, dma_addr, size);
}
EXPORT_SYMBOL(dma_mmap_coherent);

int dma_mmap_writecombine(struct device *dev, struct vm_area_struct *vma,
			  void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	return dma_mmap(dev, vma, cpu_addr, dma_addr, size);
}
EXPORT_SYMBOL(dma_mmap_writecombine);

/*
 * free a page as defined by the above mapping.
 * Must not be called with IRQs disabled.
 */
void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle)
{
	WARN_ON(irqs_disabled());

	if (dma_release_from_coherent(dev, get_order(size), cpu_addr))
		return;

	size = PAGE_ALIGN(size);

	if (!arch_is_coherent())
		__dma_free_remap(cpu_addr, size);

	__dma_free_buffer(pfn_to_page(dma_to_pfn(dev, handle)), size);
}
EXPORT_SYMBOL(dma_free_coherent);

#if defined(CONFIG_COMCERTO_UNCACHED_DMA)
static inline void shadow_pmd_inc(const void *kaddr, int incr)
{
	atomic_add(incr, &shadow_pmd_count[pgd_index((unsigned long) kaddr)]);
}
#endif

static inline void __dmac_map_area(const void *kaddr, size_t size,
	int dir)
{
#if defined(CONFIG_COMCERTO_UNCACHED_DMA)
	pmd_t *pmd, *shadow_pmd;
	pte_t *pte;
	const void *kaddr_page;
	const struct mem_type *mt;
	unsigned int nr_pages, nr_pages_pmd;

	if (!shadow_pmd_count)
		goto op;

	// For now, be safe and only uncache full pages so we don't have to handle
	// the case of pages being DMA-mapped multiple times
	if ((dir == DMA_FROM_DEVICE) && ((((unsigned long) kaddr|size) & ~PAGE_MASK) == 0)) {
		mt = get_mem_type(MT_MEMORY_NONCACHED);
		kaddr_page = kaddr;
		pmd = pmd_off_k((unsigned long) kaddr_page);
		shadow_pmd = (pmd_t *)shadow_pg_dir + (pmd - pmd_off_k(0));
		nr_pages = __phys_to_pfn(size);

		if (nr_pages == 1) { // Optimize for the common case
			shadow_pmd_inc(kaddr_page, 1);
			if (pmd_bad(*pmd))  // Skip pmd_bad() area - Wayne.Chou 2014/09/23
				goto op;

			pte = pte_offset_kernel(pmd, (int) kaddr_page);
			uncache_pte_ext(pte);
			flush_tlb_kernel_page((unsigned long) kaddr_page);
			goto op;
		}

		nr_pages_pmd = __phys_to_pfn(PMD_SIZE - ((unsigned long) kaddr_page & ~PMD_MASK));

		while (nr_pages) {

			nr_pages_pmd = min(nr_pages, nr_pages_pmd);
			nr_pages -= nr_pages_pmd;

			shadow_pmd_inc(kaddr_page, nr_pages_pmd);

			if (pmd_bad(*pmd)) // Skip pmd_bad() area - Wayne.Chou 2014/09/23
				goto op;

			pte = pte_offset_kernel(pmd, (int) kaddr_page);
			while (nr_pages_pmd) {
				uncache_pte_ext(pte);
				flush_tlb_kernel_page((unsigned long) kaddr_page);
				pte++;
				kaddr_page += PAGE_SIZE;
				nr_pages_pmd--;
			}

			nr_pages_pmd = PTRS_PER_PTE;
#if !defined(CONFIG_COMCERTO_64K_PAGES)
			pmd += 2;
			shadow_pmd += 2;
#else
			pmd += LINKED_PMDS;
			shadow_pmd += LINKED_PMDS;
#endif
		}
	}
	op:
#endif
	dmac_map_area(kaddr, size, dir);
}

static inline void __dmac_unmap_area(const void *kaddr, size_t size,
	int dir)
{
#if defined(CONFIG_COMCERTO_UNCACHED_DMA)
	pmd_t *pmd;
	pte_t *pte;
	const struct mem_type *mt;
	unsigned long pa;
	const void *kaddr_page;
	unsigned int nr_pages, nr_pages_pmd, page_count;

	if (!shadow_pmd_count)
		goto op;

	if ((dir == DMA_FROM_DEVICE) && ((((unsigned long) kaddr|size) & ~PAGE_MASK) == 0)) {
		kaddr_page = kaddr;

		pmd = pmd_off_k((unsigned long) kaddr_page);
		pa = __virt_to_phys((unsigned long)kaddr_page & PMD_MASK);

		nr_pages = __phys_to_pfn(size);

		if (nr_pages == 1) { // Optimize for the common case
			if (pmd_bad(*pmd)) // No 2nd-level page table, so page was never made non-cacheable.
				goto op;
			pte = pte_offset_kernel(pmd, (int) kaddr_page);
			set_pte_ext(pte, *pte, 8);

			if (atomic_sub_and_test(1, &shadow_pmd_count[pgd_index((unsigned long) kaddr_page)])) {
#if !defined(CONFIG_COMCERTO_64K_PAGES)
				pmd++;
				pa += SECTION_SIZE;
#else
				pmd_t *orig_pmd = pmd;
				while (pmd < (orig_pmd + LINKED_PMDS)) {
					pa += SECTION_SIZE;
					pmd++;
				}
#endif
			}
			flush_tlb_kernel_page((unsigned long) kaddr_page);
			return;
		}

		nr_pages_pmd = __phys_to_pfn(PMD_SIZE - ((unsigned long) kaddr_page & ~PMD_MASK));

		while (nr_pages) {
			if (pmd_bad(*pmd)) // No 2nd-level page table, so page was never made non-cacheable.
				goto op;
			nr_pages_pmd = min(nr_pages, nr_pages_pmd);
			nr_pages -= nr_pages_pmd;

			pte = pte_offset_kernel(pmd, (int) kaddr_page);
			page_count = nr_pages_pmd;
			while (page_count) {
				set_pte_ext(pte, *pte, 8);
				pte++;
				page_count--;
			}

			if (atomic_sub_and_test( nr_pages_pmd, &shadow_pmd_count[pgd_index((unsigned long) kaddr_page)])) {
#if !defined(CONFIG_COMCERTO_64K_PAGES)
				pmd++;
				pa += SECTION_SIZE;
				pmd++;
				pa += SECTION_SIZE;
#else
				pmd_t *orig_pmd = pmd;
				while (pmd < (orig_pmd + LINKED_PMDS)) {
					pmd++;
					pa += SECTION_SIZE;
				}
#endif
			}

			while (nr_pages_pmd) {
				flush_tlb_kernel_page((unsigned long) kaddr_page);
				kaddr_page += PAGE_SIZE;
				nr_pages_pmd--;
			}

			nr_pages_pmd = PTRS_PER_PTE;
		}

		return;
	}
	op:
#endif
#if !defined(CONFIG_CPU_DMA_PARTIAL_INVALIDATES)
	dmac_unmap_area(kaddr, size, dir);
#else
	size_t size_inv = min_t(size_t, 32, size);

	dmac_unmap_area(kaddr, size_inv, dir);
	dmac_unmap_area(kaddr + size - size_inv, size_inv, dir);
#endif
}

/*
 * Make an area consistent for devices.
 * Note: Drivers should NOT use this function directly, as it will break
 * platforms with CONFIG_DMABOUNCE.
 * Use the driver DMA support - see dma-mapping.h (dma_sync_*)
 */
void ___dma_single_cpu_to_dev(const void *kaddr, size_t size,
	enum dma_data_direction dir)
{
	unsigned long paddr = __pa(kaddr);

	BUG_ON(!virt_addr_valid(kaddr) || !virt_addr_valid(kaddr + size - 1));

#ifdef CONFIG_COMCERTO_ZONE_DMA_NCNB
	if ((paddr + size) <= arm_dma_zone_size) {
		if (dir != DMA_FROM_DEVICE)
			wmb();

		return;
	}
#endif
	__dmac_map_area(kaddr, size, dir);

#if !defined(CONFIG_L2X0_INSTRUCTION_ONLY)
	if (dir == DMA_FROM_DEVICE) {
		outer_inv_range(paddr, paddr + size);
	} else {
		outer_clean_range(paddr, paddr + size);
	}
	/* FIXME: non-speculating: flush on bidirectional mappings? */
#endif
}
EXPORT_SYMBOL(___dma_single_cpu_to_dev);

void ___dma_single_dev_to_cpu(const void *kaddr, size_t size,
	enum dma_data_direction dir)
{
	unsigned long paddr = __pa(kaddr);

	BUG_ON(!virt_addr_valid(kaddr) || !virt_addr_valid(kaddr + size - 1));

#ifdef CONFIG_COMCERTO_ZONE_DMA_NCNB
	if ((paddr + size) <= arm_dma_zone_size)
		return;
#endif

#if !defined(CONFIG_L2X0_INSTRUCTION_ONLY)
	/* don't bother invalidating if DMA to device */
	if (dir != DMA_TO_DEVICE) {
#if !defined(CONFIG_CPU_DMA_PARTIAL_INVALIDATES)
		outer_inv_range(paddr, paddr + size);
#else
		size_t size_inv = min_t(size_t, 32, size);

		outer_inv_range(paddr, paddr + size_inv);
		outer_inv_range(paddr + size - size_inv, paddr + size);
#endif
	}
#endif

	__dmac_unmap_area(kaddr, size, dir);
}
EXPORT_SYMBOL(___dma_single_dev_to_cpu);

static void dma_cache_maint_page(struct page *page, unsigned long offset,
	size_t size, enum dma_data_direction dir,
	void (*op)(const void *, size_t, int))
{
	/*
	 * A single sg entry may refer to multiple physically contiguous
	 * pages.  But we still need to process highmem pages individually.
	 * If highmem is not configured then the bulk of this loop gets
	 * optimized out.
	 */
	size_t left = size;
	do {
		size_t len = left;
		void *vaddr;

		if (PageHighMem(page)) {
			if (len + offset > PAGE_SIZE) {
				if (offset >= PAGE_SIZE) {
					page += offset / PAGE_SIZE;
					offset %= PAGE_SIZE;
				}
				len = PAGE_SIZE - offset;
			}
			vaddr = kmap_high_get(page);
			if (vaddr) {
				vaddr += offset;
				op(vaddr, len, dir);
				kunmap_high(page);
			} else if (cache_is_vipt()) {
				/* unmapped pages might still be cached */
				vaddr = kmap_atomic(page);
				op(vaddr + offset, len, dir);
				kunmap_atomic(vaddr);
			}
		} else {
			vaddr = page_address(page) + offset;
			op(vaddr, len, dir);
		}

		offset = 0;
		page++;
		left -= len;
	} while (left);
}

void ___dma_page_cpu_to_dev(struct page *page, unsigned long off,
	size_t size, enum dma_data_direction dir)
{
	unsigned long paddr = page_to_phys(page) + off;

#ifdef CONFIG_COMCERTO_ZONE_DMA_NCNB
	if ((paddr + size) <= arm_dma_zone_size) {
		if (dir != DMA_FROM_DEVICE)
			wmb();

		return;
	}
#endif

	dma_cache_maint_page(page, off, size, dir, __dmac_map_area);

#if !defined(CONFIG_L2X0_INSTRUCTION_ONLY)
	if (dir == DMA_FROM_DEVICE) {
		outer_inv_range(paddr, paddr + size);
	} else {
		outer_clean_range(paddr, paddr + size);
	}
#endif
	/* FIXME: non-speculating: flush on bidirectional mappings? */
}
EXPORT_SYMBOL(___dma_page_cpu_to_dev);

void ___dma_page_dev_to_cpu(struct page *page, unsigned long off,
	size_t size, enum dma_data_direction dir)
{
	unsigned long paddr = page_to_phys(page) + off;

#ifdef CONFIG_COMCERTO_ZONE_DMA_NCNB
	if ((paddr + size) <= arm_dma_zone_size)
		return;
#endif

#if !defined(CONFIG_L2X0_INSTRUCTION_ONLY)
	/* don't bother invalidating if DMA to device */
	if (dir != DMA_TO_DEVICE) {
#if !defined(CONFIG_CPU_DMA_PARTIAL_INVALIDATES)
		outer_inv_range(paddr, paddr + size);
#else
		size_t size_inv = min_t(size_t, 32, size);

		outer_inv_range(paddr, paddr + size_inv);
		outer_inv_range(paddr + size - size_inv, paddr + size);
#endif
	}
#endif
	dma_cache_maint_page(page, off, size, dir, __dmac_unmap_area);

	/*
	 * Mark the D-cache clean for this page to avoid extra flushing.
	 */
	if (dir != DMA_TO_DEVICE && off == 0 && size >= PAGE_SIZE)
		set_bit(PG_dcache_clean, &page->flags);
}
EXPORT_SYMBOL(___dma_page_dev_to_cpu);

/**
 * dma_map_sg - map a set of SG buffers for streaming mode DMA
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to map
 * @dir: DMA transfer direction
 *
 * Map a set of buffers described by scatterlist in streaming mode for DMA.
 * This is the scatter-gather version of the dma_map_single interface.
 * Here the scatter gather list elements are each tagged with the
 * appropriate dma address and length.  They are obtained via
 * sg_dma_{address,length}.
 *
 * Device ownership issues as mentioned for dma_map_single are the same
 * here.
 */
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i, j;

	BUG_ON(!valid_dma_direction(dir));

	for_each_sg(sg, s, nents, i) {
		s->dma_address = __dma_map_page(dev, sg_page(s), s->offset,
						s->length, dir);
		if (dma_mapping_error(dev, s->dma_address))
			goto bad_mapping;
	}
	debug_dma_map_sg(dev, sg, nents, nents, dir);
	return nents;

 bad_mapping:
	for_each_sg(sg, s, i, j)
		__dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
	return 0;
}
EXPORT_SYMBOL(dma_map_sg);

/**
 * dma_unmap_sg - unmap a set of SG buffers mapped by dma_map_sg
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to unmap (same as was passed to dma_map_sg)
 * @dir: DMA transfer direction (same as was passed to dma_map_sg)
 *
 * Unmap a set of streaming mode DMA translations.  Again, CPU access
 * rules concerning calls here are the same as for dma_unmap_single().
 */
void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	debug_dma_unmap_sg(dev, sg, nents, dir);

	for_each_sg(sg, s, nents, i)
		__dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
}
EXPORT_SYMBOL(dma_unmap_sg);

/**
 * dma_sync_sg_for_cpu
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to map (returned from dma_map_sg)
 * @dir: DMA transfer direction (same as was passed to dma_map_sg)
 */
void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		if (!dmabounce_sync_for_cpu(dev, sg_dma_address(s), 0,
					    sg_dma_len(s), dir))
			continue;

		__dma_page_dev_to_cpu(sg_page(s), s->offset,
				      s->length, dir);
	}

	debug_dma_sync_sg_for_cpu(dev, sg, nents, dir);
}
EXPORT_SYMBOL(dma_sync_sg_for_cpu);

/**
 * dma_sync_sg_for_device
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @sg: list of buffers
 * @nents: number of buffers to map (returned from dma_map_sg)
 * @dir: DMA transfer direction (same as was passed to dma_map_sg)
 */
void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		if (!dmabounce_sync_for_device(dev, sg_dma_address(s), 0,
					sg_dma_len(s), dir))
			continue;

		__dma_page_cpu_to_dev(sg_page(s), s->offset,
				      s->length, dir);
	}

	debug_dma_sync_sg_for_device(dev, sg, nents, dir);
}
EXPORT_SYMBOL(dma_sync_sg_for_device);

/*
 * Return whether the given device DMA address mask can be supported
 * properly.  For example, if your device can only drive the low 24-bits
 * during bus mastering, then you would pass 0x00ffffff as the mask
 * to this function.
 */
int dma_supported(struct device *dev, u64 mask)
{
	if (mask < (u64)arm_dma_limit)
		return 0;
	return 1;
}
EXPORT_SYMBOL(dma_supported);

int dma_set_mask(struct device *dev, u64 dma_mask)
{
	if (!dev->dma_mask || !dma_supported(dev, dma_mask))
		return -EIO;

#ifndef CONFIG_DMABOUNCE
	*dev->dma_mask = dma_mask;
#endif

	return 0;
}
EXPORT_SYMBOL(dma_set_mask);

#define PREALLOC_DMA_DEBUG_ENTRIES	4096

static int __init dma_debug_do_init(void)
{
	dma_debug_init(PREALLOC_DMA_DEBUG_ENTRIES);
	return 0;
}
fs_initcall(dma_debug_do_init);
