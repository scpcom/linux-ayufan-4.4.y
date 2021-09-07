 
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/memory.h>
#include <asm/highmem.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/sizes.h>

#if (CONSISTENT_DMA_SIZE % SZ_2M)
#error "CONSISTENT_DMA_SIZE must be multiple of 2MiB"
#endif

#define CONSISTENT_END	(0xffe00000)
#define CONSISTENT_BASE	(CONSISTENT_END - CONSISTENT_DMA_SIZE)

#define CONSISTENT_OFFSET(x)	(((unsigned long)(x) - CONSISTENT_BASE) >> PAGE_SHIFT)
#define CONSISTENT_PTE_INDEX(x) (((unsigned long)(x) - CONSISTENT_BASE) >> PGDIR_SHIFT)
#define NUM_CONSISTENT_PTES (CONSISTENT_DMA_SIZE >> PGDIR_SHIFT)

static u64 get_coherent_dma_mask(struct device *dev)
{
	u64 mask = ISA_DMA_THRESHOLD;

	if (dev) {
		mask = dev->coherent_dma_mask;

		if (mask == 0) {
			dev_warn(dev, "coherent DMA mask is unset\n");
			return 0;
		}

		if ((~mask) & ISA_DMA_THRESHOLD) {
			dev_warn(dev, "coherent DMA mask %#llx is smaller "
				 "than system GFP_DMA mask %#llx\n",
				 mask, (unsigned long long)ISA_DMA_THRESHOLD);
			return 0;
		}
	}

	return mask;
}

#ifdef CONFIG_MMU
 
static pte_t *consistent_pte[NUM_CONSISTENT_PTES];
static DEFINE_SPINLOCK(consistent_lock);

struct arm_vm_region {
	struct list_head	vm_list;
	unsigned long		vm_start;
	unsigned long		vm_end;
	struct page		*vm_pages;
	int			vm_active;
};

static struct arm_vm_region consistent_head = {
	.vm_list	= LIST_HEAD_INIT(consistent_head.vm_list),
	.vm_start	= CONSISTENT_BASE,
	.vm_end		= CONSISTENT_END,
};

static struct arm_vm_region *
arm_vm_region_alloc(struct arm_vm_region *head, size_t size, gfp_t gfp)
{
	unsigned long addr = head->vm_start, end = head->vm_end - size;
	unsigned long flags;
	struct arm_vm_region *c, *new;

	new = kmalloc(sizeof(struct arm_vm_region), gfp);
	if (!new)
		goto out;

	spin_lock_irqsave(&consistent_lock, flags);

	list_for_each_entry(c, &head->vm_list, vm_list) {
		if ((addr + size) < addr)
			goto nospc;
		if ((addr + size) <= c->vm_start)
			goto found;
		addr = c->vm_end;
		if (addr > end)
			goto nospc;
	}

 found:
	 
	list_add_tail(&new->vm_list, &c->vm_list);
	new->vm_start = addr;
	new->vm_end = addr + size;
	new->vm_active = 1;

	spin_unlock_irqrestore(&consistent_lock, flags);
	return new;

 nospc:
	spin_unlock_irqrestore(&consistent_lock, flags);
	kfree(new);
 out:
	return NULL;
}

static struct arm_vm_region *arm_vm_region_find(struct arm_vm_region *head, unsigned long addr)
{
	struct arm_vm_region *c;
	
	list_for_each_entry(c, &head->vm_list, vm_list) {
		if (c->vm_active && c->vm_start == addr)
			goto out;
	}
	c = NULL;
 out:
	return c;
}

#ifdef CONFIG_HUGETLB_PAGE
#error ARM Coherent DMA allocator does not (yet) support huge TLB
#endif

static void *
__dma_alloc(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp,
	    pgprot_t prot)
{
	struct page *page;
	struct arm_vm_region *c;
	unsigned long order;
	u64 mask = get_coherent_dma_mask(dev);
	u64 limit;

	if (!consistent_pte[0]) {
		printk(KERN_ERR "%s: not initialised\n", __func__);
		dump_stack();
		return NULL;
	}

	if (!mask)
		goto no_page;

	size = PAGE_ALIGN(size);
	limit = (mask + 1) & ~mask;
	if ((limit && size >= limit) ||
	    size >= (CONSISTENT_END - CONSISTENT_BASE)) {
		printk(KERN_WARNING "coherent allocation too big "
		       "(requested %#x mask %#llx)\n", size, mask);
		goto no_page;
	}

	order = get_order(size);

	if (mask < 0xffffffffULL)
		gfp |= GFP_DMA;

	page = alloc_pages(gfp, order);
	if (!page)
		goto no_page;

	{
		void *ptr = page_address(page);
		memset(ptr, 0, size);
		dmac_flush_range(ptr, ptr + size);
		outer_flush_range(__pa(ptr), __pa(ptr) + size);
	}

	c = arm_vm_region_alloc(&consistent_head, size,
			    gfp & ~(__GFP_DMA | __GFP_HIGHMEM));
	if (c) {
		pte_t *pte;
		struct page *end = page + (1 << order);
		int idx = CONSISTENT_PTE_INDEX(c->vm_start);
		u32 off = CONSISTENT_OFFSET(c->vm_start) & (PTRS_PER_PTE-1);

		pte = consistent_pte[idx] + off;
		c->vm_pages = page;

		split_page(page, order);

		*handle = page_to_dma(dev, page);

		do {
			BUG_ON(!pte_none(*pte));

			SetPageReserved(page);
			set_pte_ext(pte, mk_pte(page, prot), 0);
			page++;
			pte++;
			off++;
			if (off >= PTRS_PER_PTE) {
				off = 0;
				pte = consistent_pte[++idx];
			}
		} while (size -= PAGE_SIZE);

		while (page < end) {
			__free_page(page);
			page++;
		}

		return (void *)c->vm_start;
	}

	if (page)
		__free_pages(page, order);
 no_page:
	*handle = ~0;
	return NULL;
}
#else	 
static void *
__dma_alloc(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp,
	    pgprot_t prot)
{
	void *virt;
	u64 mask = get_coherent_dma_mask(dev);

	if (!mask)
		goto error;

	if (mask < 0xffffffffULL)
		gfp |= GFP_DMA;
	virt = kmalloc(size, gfp);
	if (!virt)
		goto error;

	*handle =  virt_to_dma(dev, virt);
	return virt;

error:
	*handle = ~0;
	return NULL;
}
#endif	 

void *
dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp)
{
	void *memory;

	if (dma_alloc_from_coherent(dev, size, handle, &memory))
		return memory;

	if (arch_is_coherent()) {
		void *virt;

		virt = kmalloc(size, gfp);
		if (!virt)
			return NULL;
		*handle =  virt_to_dma(dev, virt);

		return virt;
	}

	return __dma_alloc(dev, size, handle, gfp,
			   pgprot_noncached(pgprot_kernel));
}
EXPORT_SYMBOL(dma_alloc_coherent);

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
	unsigned long flags, user_size, kern_size;
	struct arm_vm_region *c;

	user_size = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	spin_lock_irqsave(&consistent_lock, flags);
	c = arm_vm_region_find(&consistent_head, (unsigned long)cpu_addr);
	spin_unlock_irqrestore(&consistent_lock, flags);

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
#endif	 

	return ret;
}

int dma_mmap_coherent(struct device *dev, struct vm_area_struct *vma,
		      void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
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

#ifdef CONFIG_MMU
void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle)
{
	struct arm_vm_region *c;
	unsigned long flags, addr;
	pte_t *ptep;
	int idx;
	u32 off;

	WARN_ON(irqs_disabled());

	if (dma_release_from_coherent(dev, get_order(size), cpu_addr))
		return;

	if (arch_is_coherent()) {
		kfree(cpu_addr);
		return;
	}

	size = PAGE_ALIGN(size);

	spin_lock_irqsave(&consistent_lock, flags);
	c = arm_vm_region_find(&consistent_head, (unsigned long)cpu_addr);
	if (!c)
		goto no_area;

	c->vm_active = 0;
	spin_unlock_irqrestore(&consistent_lock, flags);

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
		unsigned long pfn;

		ptep++;
		addr += PAGE_SIZE;
		off++;
		if (off >= PTRS_PER_PTE) {
			off = 0;
			ptep = consistent_pte[++idx];
		}

		if (!pte_none(pte) && pte_present(pte)) {
			pfn = pte_pfn(pte);

			if (pfn_valid(pfn)) {
				struct page *page = pfn_to_page(pfn);

				ClearPageReserved(page);

				__free_page(page);
				continue;
			}
		}

		printk(KERN_CRIT "%s: bad page in kernel page table\n",
		       __func__);
	} while (size -= PAGE_SIZE);

	flush_tlb_kernel_range(c->vm_start, c->vm_end);

	spin_lock_irqsave(&consistent_lock, flags);
	list_del(&c->vm_list);
	spin_unlock_irqrestore(&consistent_lock, flags);

	kfree(c);
	return;

 no_area:
	spin_unlock_irqrestore(&consistent_lock, flags);
	printk(KERN_ERR "%s: trying to free invalid coherent area: %p\n",
	       __func__, cpu_addr);
	dump_stack();
}
#else	 
void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle)
{
	if (dma_release_from_coherent(dev, get_order(size), cpu_addr))
		return;
	kfree(cpu_addr);
}
#endif	 
EXPORT_SYMBOL(dma_free_coherent);

static int __init consistent_init(void)
{
	int ret = 0;
#ifdef CONFIG_MMU
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	int i = 0;
	u32 base = CONSISTENT_BASE;

	do {
		pgd = pgd_offset(&init_mm, base);
		pmd = pmd_alloc(&init_mm, pgd, base);
		if (!pmd) {
			printk(KERN_ERR "%s: no pmd tables\n", __func__);
			ret = -ENOMEM;
			break;
		}
		WARN_ON(!pmd_none(*pmd));

		pte = pte_alloc_kernel(pmd, base);
		if (!pte) {
			printk(KERN_ERR "%s: no pte tables\n", __func__);
			ret = -ENOMEM;
			break;
		}

		consistent_pte[i++] = pte;
		base += (1 << PGDIR_SHIFT);
	} while (base < CONSISTENT_END);
#endif	 

	return ret;
}

core_initcall(consistent_init);

#if defined(CONFIG_SYNO_PLX_PORTING) && defined(CONFIG_ARCH_OX820) && defined(CONFIG_SMP)
#include <mach/rps-irq.h>
#include <mach/ipi.h>

void dma_cache_maint(
	const void *start,
	size_t      size,
	int         direction)
{
	unsigned long flags;
	unsigned int  cpu;
	cpumask_t     callmap;

	local_irq_save(flags);
	cpu = get_cpu();
	
	BUG_ON(cpu > 1);

	callmap = cpu_online_map;
	cpu_clear(cpu, callmap);

	BUG_ON(per_cpu(fiq_coherency_communication, cpu).nents);

	if (!cpus_empty(callmap)) {
		 
		per_cpu(fiq_coherency_communication, cpu).type = CACHE_COHERENCY;
		per_cpu(fiq_coherency_communication, cpu).message.cache_coherency.type = direction;
		per_cpu(fiq_coherency_communication, cpu).message.cache_coherency.range[0].start = start;
		per_cpu(fiq_coherency_communication, cpu).message.cache_coherency.range[0].end = start + size;
		per_cpu(fiq_coherency_communication, cpu).nents = 1;
		wmb();

		OX820_RPS_trigger_fiq(!cpu);
	}

	switch (direction) {
		case DMA_BIDIRECTIONAL:
			dmac_flush_range(start, start + size);
			break;
		case DMA_TO_DEVICE:
			dmac_clean_range(start, start + size);
			break;
		case DMA_FROM_DEVICE:
			dmac_inv_range(start, start + size);
			break;
		default:
			printk(KERN_WARNING "Unknown DMA direction %d\n", direction);
	}

	while (per_cpu(fiq_coherency_communication,cpu).nents) {
		barrier();
	}

	put_cpu();
	local_irq_restore(flags);
}
EXPORT_SYMBOL(dma_cache_maint);

static void dma_cache_maint_contiguous(
	struct page  *page,
	unsigned long offset,
	size_t        size,
	int           direction)
{
	void *vaddr;
	int   himem = 0;

	if (!PageHighMem(page)) {
		vaddr = page_address(page) + offset;
	} else {
		vaddr = kmap_high_get(page);
		if (vaddr) {
			himem = 1;
			vaddr += offset;
		}
	}

	if (vaddr) {
		unsigned long flags;
		unsigned int  cpu;
		cpumask_t     callmap;

		local_irq_save(flags);
		cpu = get_cpu();
	
		BUG_ON(cpu > 1);

		callmap = cpu_online_map;
		cpu_clear(cpu, callmap);

		BUG_ON(per_cpu(fiq_coherency_communication, cpu).nents);

		if (!cpus_empty(callmap)) {
			 
			per_cpu(fiq_coherency_communication, cpu).type = CACHE_COHERENCY;
			per_cpu(fiq_coherency_communication, cpu).message.cache_coherency.type = direction;
			per_cpu(fiq_coherency_communication, cpu).message.cache_coherency.range[0].start = vaddr;
			per_cpu(fiq_coherency_communication, cpu).message.cache_coherency.range[0].end = vaddr + size;
			per_cpu(fiq_coherency_communication, cpu).nents = 1;
			wmb();

			OX820_RPS_trigger_fiq(!cpu);
		}

		switch (direction) {
			case DMA_BIDIRECTIONAL:
				dmac_flush_range(vaddr, vaddr + size);
				break;
			case DMA_TO_DEVICE:
				dmac_clean_range(vaddr, vaddr + size);
				break;
			case DMA_FROM_DEVICE:
				dmac_inv_range(vaddr, vaddr + size);
				break;
			default:
				printk(KERN_WARNING "Unknown DMA direction %d\n", direction);
		}

		while (per_cpu(fiq_coherency_communication,cpu).nents) {
			barrier();
		}

		put_cpu();
		local_irq_restore(flags);
	}

	if (vaddr && himem) {
		kunmap_high(page);
	}
}
#else  
 
void dma_cache_maint(const void *start, size_t size, int direction)
{
	void (*inner_op)(const void *, const void *);
	void (*outer_op)(unsigned long, unsigned long);

	BUG_ON(!virt_addr_valid(start) || !virt_addr_valid(start + size - 1));

	switch (direction) {
	case DMA_FROM_DEVICE:		 
		inner_op = dmac_inv_range;
		outer_op = outer_inv_range;
		break;
	case DMA_TO_DEVICE:		 
		inner_op = dmac_clean_range;
		outer_op = outer_clean_range;
		break;
	case DMA_BIDIRECTIONAL:		 
		inner_op = dmac_flush_range;
		outer_op = outer_flush_range;
		break;
	default:
		BUG();
	}

	inner_op(start, start + size);
	outer_op(__pa(start), __pa(start) + size);
}
EXPORT_SYMBOL(dma_cache_maint);

static void dma_cache_maint_contiguous(struct page *page, unsigned long offset,
				       size_t size, int direction)
{
	void *vaddr;
	unsigned long paddr;
	void (*inner_op)(const void *, const void *);
	void (*outer_op)(unsigned long, unsigned long);
#ifdef CONFIG_SYNO_PLX_PORTING
panic("Not expecting dma_cache_maint_contiguous() to be invoked\n");
#endif

	switch (direction) {
	case DMA_FROM_DEVICE:		 
		inner_op = dmac_inv_range;
		outer_op = outer_inv_range;
		break;
	case DMA_TO_DEVICE:		 
		inner_op = dmac_clean_range;
		outer_op = outer_clean_range;
		break;
	case DMA_BIDIRECTIONAL:		 
		inner_op = dmac_flush_range;
		outer_op = outer_flush_range;
		break;
	default:
		BUG();
	}

	if (!PageHighMem(page)) {
		vaddr = page_address(page) + offset;
		inner_op(vaddr, vaddr + size);
	} else {
		vaddr = kmap_high_get(page);
		if (vaddr) {
			vaddr += offset;
			inner_op(vaddr, vaddr + size);
			kunmap_high(page);
		}
	}

	paddr = page_to_phys(page) + offset;
	outer_op(paddr, paddr + size);
}
#endif  

void dma_cache_maint_page(struct page *page, unsigned long offset,
			  size_t size, int dir)
{
	 
	size_t left = size;
	do {
		size_t len = left;
		if (PageHighMem(page) && len + offset > PAGE_SIZE) {
			if (offset >= PAGE_SIZE) {
				page += offset / PAGE_SIZE;
				offset %= PAGE_SIZE;
			}
			len = PAGE_SIZE - offset;
		}
		dma_cache_maint_contiguous(page, offset, len, dir);
		offset = 0;
		page++;
		left -= len;
	} while (left);
}
EXPORT_SYMBOL(dma_cache_maint_page);

#if !defined(CONFIG_ARCH_OX820) || !defined(CONFIG_SMP)  
 
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i, j;

	for_each_sg(sg, s, nents, i) {
		s->dma_address = dma_map_page(dev, sg_page(s), s->offset,
						s->length, dir);
		if (dma_mapping_error(dev, s->dma_address))
			goto bad_mapping;
	}
	return nents;

 bad_mapping:
	for_each_sg(sg, s, i, j)
		dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
	return 0;
}
EXPORT_SYMBOL(dma_map_sg);
#endif

void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents,
		enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i)
		dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
}
EXPORT_SYMBOL(dma_unmap_sg);

void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		dmabounce_sync_for_cpu(dev, sg_dma_address(s), 0,
					sg_dma_len(s), dir);
	}
}
EXPORT_SYMBOL(dma_sync_sg_for_cpu);

void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		if (!dmabounce_sync_for_device(dev, sg_dma_address(s), 0,
					sg_dma_len(s), dir))
			continue;

		if (!arch_is_coherent())
			dma_cache_maint_page(sg_page(s), s->offset,
					     s->length, dir);
	}
}
EXPORT_SYMBOL(dma_sync_sg_for_device);
