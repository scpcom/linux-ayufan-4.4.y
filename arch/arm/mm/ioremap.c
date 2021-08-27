#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/io.h>

#include <asm/cputype.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/sizes.h>

#include <asm/mach/map.h>
#include "mm.h"

#define VM_ARM_SECTION_MAPPING	0x80000000

#ifdef MY_DEF_HERE
#ifdef CONFIG_PCI
int pci_ioremap_io(unsigned int offset, phys_addr_t phys_addr)
{
	BUG_ON(offset + SZ_64K > IO_SPACE_LIMIT);
	return ioremap_page_range(PCI_IO_VIRT_BASE + offset,
		   PCI_IO_VIRT_BASE + offset + SZ_64K,
		   phys_addr,
		   __pgprot(get_mem_type(MT_DEVICE)->prot_pte));
}
EXPORT_SYMBOL_GPL(pci_ioremap_io);
#endif
#endif
int ioremap_page(unsigned long virt, unsigned long phys,
		 const struct mem_type *mtype)
{
	return ioremap_page_range(virt, virt + PAGE_SIZE, phys,
				  __pgprot(mtype->prot_pte));
}
EXPORT_SYMBOL(ioremap_page);

void __check_kvm_seq(struct mm_struct *mm)
{
	unsigned int seq;

	do {
		seq = init_mm.context.kvm_seq;
		memcpy(pgd_offset(mm, VMALLOC_START),
		       pgd_offset_k(VMALLOC_START),
		       sizeof(pgd_t) * (pgd_index(VMALLOC_END) -
					pgd_index(VMALLOC_START)));
		mm->context.kvm_seq = seq;
	} while (seq != init_mm.context.kvm_seq);
}

#if !defined(CONFIG_SMP) && (!defined(MY_DEF_HERE) || (defined(MY_DEF_HERE) && !defined(CONFIG_ARM_LPAE)))
 
static void unmap_area_sections(unsigned long virt, unsigned long size)
{
	unsigned long addr = virt, end = virt + (size & ~(SZ_1M - 1));
	pgd_t *pgd;

	flush_cache_vunmap(addr, end);
	pgd = pgd_offset_k(addr);
	do {
		pmd_t pmd, *pmdp = pmd_offset(pgd, addr);

		pmd = *pmdp;
		if (!pmd_none(pmd)) {
			 
			pmd_clear(pmdp);
			init_mm.context.kvm_seq++;

			if ((pmd_val(pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE)
				pte_free_kernel(&init_mm, pmd_page_vaddr(pmd));
		}

		addr += PGDIR_SIZE;
		pgd++;
	} while (addr < end);

	if (current->active_mm->context.kvm_seq != init_mm.context.kvm_seq)
		__check_kvm_seq(current->active_mm);

	flush_tlb_kernel_range(virt, end);
}

static int
remap_area_sections(unsigned long virt, unsigned long pfn,
		    size_t size, const struct mem_type *type)
{
	unsigned long addr = virt, end = virt + size;
	pgd_t *pgd;

	unmap_area_sections(virt, size);

	pgd = pgd_offset_k(addr);
	do {
		pmd_t *pmd = pmd_offset(pgd, addr);

		pmd[0] = __pmd(__pfn_to_phys(pfn) | type->prot_sect);
		pfn += SZ_1M >> PAGE_SHIFT;
		pmd[1] = __pmd(__pfn_to_phys(pfn) | type->prot_sect);
		pfn += SZ_1M >> PAGE_SHIFT;
		flush_pmd_entry(pmd);

		addr += PGDIR_SIZE;
		pgd++;
	} while (addr < end);

	return 0;
}

static int
remap_area_supersections(unsigned long virt, unsigned long pfn,
			 size_t size, const struct mem_type *type)
{
	unsigned long addr = virt, end = virt + size;
	pgd_t *pgd;

	unmap_area_sections(virt, size);

	pgd = pgd_offset_k(virt);
	do {
		unsigned long super_pmd_val, i;

		super_pmd_val = __pfn_to_phys(pfn) | type->prot_sect |
				PMD_SECT_SUPER;
		super_pmd_val |= ((pfn >> (32 - PAGE_SHIFT)) & 0xf) << 20;

		for (i = 0; i < 8; i++) {
			pmd_t *pmd = pmd_offset(pgd, addr);

			pmd[0] = __pmd(super_pmd_val);
			pmd[1] = __pmd(super_pmd_val);
			flush_pmd_entry(pmd);

			addr += PGDIR_SIZE;
			pgd++;
		}

		pfn += SUPERSECTION_SIZE >> PAGE_SHIFT;
	} while (addr < end);

	return 0;
}
#endif

void __iomem * __arm_ioremap_pfn_caller(unsigned long pfn,
	unsigned long offset, size_t size, unsigned int mtype, void *caller)
{
	const struct mem_type *type;
	int err;
	unsigned long addr;
 	struct vm_struct * area;

#if (defined(MY_DEF_HERE) && !defined(CONFIG_ARM_LPAE)) || !defined(MY_DEF_HERE)
	 
	if (pfn >= 0x100000 && (__pfn_to_phys(pfn) & ~SUPERSECTION_MASK))
		return NULL;
#endif

	if (WARN_ON(pfn_valid(pfn)))
		return NULL;

	type = get_mem_type(mtype);
	if (!type)
		return NULL;

	size = PAGE_ALIGN(offset + size);

	area = get_vm_area_caller(size, VM_IOREMAP, caller);
 	if (!area)
 		return NULL;
 	addr = (unsigned long)area->addr;

#if !defined(CONFIG_SMP) && ((defined(MY_DEF_HERE) && !defined(CONFIG_ARM_LPAE)) || !defined(MY_DEF_HERE))
	if (DOMAIN_IO == 0 &&
	    (((cpu_architecture() >= CPU_ARCH_ARMv6) && (get_cr() & CR_XP)) ||
	       cpu_is_xsc3()) && pfn >= 0x100000 &&
	       !((__pfn_to_phys(pfn) | size | addr) & ~SUPERSECTION_MASK)) {
		area->flags |= VM_ARM_SECTION_MAPPING;
		err = remap_area_supersections(addr, pfn, size, type);
	} else if (!((__pfn_to_phys(pfn) | size | addr) & ~PMD_MASK)) {
		area->flags |= VM_ARM_SECTION_MAPPING;
		err = remap_area_sections(addr, pfn, size, type);
	} else
#endif
		err = ioremap_page_range(addr, addr + size, __pfn_to_phys(pfn),
					 __pgprot(type->prot_pte));

	if (err) {
 		vunmap((void *)addr);
 		return NULL;
 	}

	flush_cache_vmap(addr, addr + size);
	return (void __iomem *) (offset + addr);
}

void __iomem *__arm_ioremap_caller(unsigned long phys_addr, size_t size,
	unsigned int mtype, void *caller)
{
	unsigned long last_addr;
 	unsigned long offset = phys_addr & ~PAGE_MASK;
 	unsigned long pfn = __phys_to_pfn(phys_addr);

	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	return __arm_ioremap_pfn_caller(pfn, offset, size, mtype,
			caller);
}

void __iomem *
__arm_ioremap_pfn(unsigned long pfn, unsigned long offset, size_t size,
		  unsigned int mtype)
{
	return __arm_ioremap_pfn_caller(pfn, offset, size, mtype,
			__builtin_return_address(0));
}
EXPORT_SYMBOL(__arm_ioremap_pfn);

void __iomem *
__arm_ioremap(unsigned long phys_addr, size_t size, unsigned int mtype)
{
	return __arm_ioremap_caller(phys_addr, size, mtype,
			__builtin_return_address(0));
}
EXPORT_SYMBOL(__arm_ioremap);

void __iomem *
__arm_ioremap_exec(unsigned long phys_addr, size_t size, bool cached)
{
	unsigned int mtype;

	if (cached)
		mtype = MT_MEMORY;
	else
		mtype = MT_MEMORY_NONCACHED;

	return __arm_ioremap_caller(phys_addr, size, mtype,
			__builtin_return_address(0));
}

void __iounmap(volatile void __iomem *io_addr)
{
	void *addr = (void *)(PAGE_MASK & (unsigned long)io_addr);
#if !defined(CONFIG_SMP) && ((defined(MY_DEF_HERE) && !defined(CONFIG_ARM_LPAE)) || !defined(MY_DEF_HERE))
	struct vm_struct **p, *tmp;

	write_lock(&vmlist_lock);
	for (p = &vmlist ; (tmp = *p) ; p = &tmp->next) {
		if ((tmp->flags & VM_IOREMAP) && (tmp->addr == addr)) {
			if (tmp->flags & VM_ARM_SECTION_MAPPING) {
				unmap_area_sections((unsigned long)tmp->addr,
						    tmp->size);
			}
			break;
		}
	}
	write_unlock(&vmlist_lock);
#endif

	vunmap(addr);
}
EXPORT_SYMBOL(__iounmap);
