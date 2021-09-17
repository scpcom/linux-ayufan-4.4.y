#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ASM_ARM_MEMORY_H
#define __ASM_ARM_MEMORY_H

#include <linux/compiler.h>
#include <linux/const.h>
#include <linux/types.h>
#include <linux/sizes.h>

#ifdef CONFIG_NEED_MACH_MEMORY_H
#include <mach/memory.h>
#endif

#define UL(x) _AC(x, UL)

#ifdef CONFIG_MMU

#define PAGE_OFFSET		UL(CONFIG_PAGE_OFFSET)
#define TASK_SIZE		(UL(CONFIG_PAGE_OFFSET) - UL(SZ_16M))
#define TASK_UNMAPPED_BASE	ALIGN(TASK_SIZE / 3, SZ_16M)

#define TASK_SIZE_26		(UL(1) << 26)

#ifndef CONFIG_THUMB2_KERNEL
#define MODULES_VADDR		(PAGE_OFFSET - SZ_16M)
#else
 
#define MODULES_VADDR		(PAGE_OFFSET - SZ_8M)
#endif

#if TASK_SIZE > MODULES_VADDR
#error Top of user space clashes with start of module space
#endif

#ifdef CONFIG_HIGHMEM
#define MODULES_END		(PAGE_OFFSET - PMD_SIZE)
#else
#define MODULES_END		(PAGE_OFFSET)
#endif

#define XIP_VIRT_ADDR(physaddr)  (MODULES_VADDR + ((physaddr) & 0x000fffff))

#define IOREMAP_MAX_ORDER	24

#if defined(MY_DEF_HERE)
 
#ifndef CONSISTENT_DMA_SIZE
#define CONSISTENT_DMA_SIZE	SZ_2M
#endif  
#endif  

#if defined(MY_ABC_HERE) && defined(CONFIG_MV_LARGE_PAGE_SUPPORT) && defined(CONFIG_HIGHMEM)
#define CONSISTENT_END		(0xffc00000UL)
#else  
#define CONSISTENT_END		(0xffe00000UL)
#endif  
#if defined(MY_DEF_HERE)
#define CONSISTENT_BASE		(CONSISTENT_END - CONSISTENT_DMA_SIZE)
#endif  

#else  

#ifndef TASK_SIZE
#define TASK_SIZE		(CONFIG_DRAM_SIZE)
#endif

#ifndef TASK_UNMAPPED_BASE
#define TASK_UNMAPPED_BASE	UL(0x00000000)
#endif

#ifndef END_MEM
#define END_MEM     		(UL(CONFIG_DRAM_BASE) + CONFIG_DRAM_SIZE)
#endif

#ifndef PAGE_OFFSET
#define PAGE_OFFSET		PLAT_PHYS_OFFSET
#endif

#define MODULES_END		(END_MEM)
#define MODULES_VADDR		PAGE_OFFSET

#define XIP_VIRT_ADDR(physaddr)  (physaddr)

#endif  

#ifdef CONFIG_HAVE_TCM
#define ITCM_OFFSET	UL(0xfffe0000)
#define DTCM_OFFSET	UL(0xfffe8000)
#endif

#define	__phys_to_pfn(paddr)	((unsigned long)((paddr) >> PAGE_SHIFT))
#define	__pfn_to_phys(pfn)	((phys_addr_t)(pfn) << PAGE_SHIFT)

#define page_to_phys(page)	(__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys)	(pfn_to_page(__phys_to_pfn(phys)))

#ifndef PLAT_PHYS_OFFSET
#define PLAT_PHYS_OFFSET	UL(CONFIG_PHYS_OFFSET)
#endif

#ifndef __ASSEMBLY__

#ifndef __virt_to_phys
#ifdef CONFIG_ARM_PATCH_PHYS_VIRT

#define __PV_BITS_31_24	0x81000000

extern unsigned long __pv_phys_offset;
#define PHYS_OFFSET __pv_phys_offset

#define __pv_stub(from,to,instr,type)			\
	__asm__("@ __pv_stub\n"				\
	"1:	" instr "	%0, %1, %2\n"		\
	"	.pushsection .pv_table,\"a\"\n"		\
	"	.long	1b\n"				\
	"	.popsection\n"				\
	: "=r" (to)					\
	: "r" (from), "I" (type))

static inline unsigned long __virt_to_phys(unsigned long x)
{
	unsigned long t;
	__pv_stub(x, t, "add", __PV_BITS_31_24);
	return t;
}

static inline unsigned long __phys_to_virt(unsigned long x)
{
	unsigned long t;
	__pv_stub(x, t, "sub", __PV_BITS_31_24);
	return t;
}
#else

#define PHYS_OFFSET	PLAT_PHYS_OFFSET

#define __virt_to_phys(x)	((x) - PAGE_OFFSET + PHYS_OFFSET)
#define __phys_to_virt(x)	((x) - PHYS_OFFSET + PAGE_OFFSET)

#endif
#endif

#define PHYS_PFN_OFFSET	((unsigned long)(PHYS_OFFSET >> PAGE_SHIFT))

static inline phys_addr_t virt_to_phys(const volatile void *x)
{
	return __virt_to_phys((unsigned long)(x));
}

static inline void *phys_to_virt(phys_addr_t x)
{
	return (void *)(__phys_to_virt((unsigned long)(x)));
}

#define __pa(x)			__virt_to_phys((unsigned long)(x))
#define __va(x)			((void *)__phys_to_virt((unsigned long)(x)))
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)

#ifndef __virt_to_bus
#define __virt_to_bus	__virt_to_phys
#define __bus_to_virt	__phys_to_virt
#define __pfn_to_bus(x)	__pfn_to_phys(x)
#define __bus_to_pfn(x)	__phys_to_pfn(x)
#endif

#ifdef CONFIG_VIRT_TO_BUS
static inline __deprecated unsigned long virt_to_bus(void *x)
{
	return __virt_to_bus((unsigned long)x);
}

static inline __deprecated void *bus_to_virt(unsigned long x)
{
	return (void *)__bus_to_virt(x);
}
#endif

#define ARCH_PFN_OFFSET		PHYS_PFN_OFFSET

#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
#define virt_addr_valid(kaddr)	(((unsigned long)(kaddr) >= PAGE_OFFSET && (unsigned long)(kaddr) < (unsigned long)high_memory) \
					&& pfn_valid(__pa(kaddr) >> PAGE_SHIFT) )

#endif

#include <asm-generic/memory_model.h>

#endif
