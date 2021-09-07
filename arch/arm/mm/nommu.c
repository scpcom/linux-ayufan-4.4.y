 
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/bootmem.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <asm/sections.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>

#include "mm.h"

void __init reserve_node_zero(pg_data_t *pgdat)
{
	 
#ifdef CONFIG_XIP_KERNEL
	reserve_bootmem_node(pgdat, __pa(_data), _end - _data,
			BOOTMEM_DEFAULT);
#else
	reserve_bootmem_node(pgdat, __pa(_stext), _end - _stext,
			BOOTMEM_DEFAULT);
#endif

	reserve_bootmem_node(pgdat, CONFIG_VECTORS_BASE, PAGE_SIZE,
			BOOTMEM_DEFAULT);
}

void __init paging_init(struct machine_desc *mdesc)
{
	bootmem_init();
}

void setup_mm_for_reboot(char mode)
{
}

void flush_dcache_page(struct page *page)
{
#ifdef CONFIG_SYNO_PLX_PORTING
	__cpuc_flush_dcache_area(page_address(page), PAGE_SIZE);
#else
	__cpuc_flush_dcache_page(page_address(page));
#endif
}
EXPORT_SYMBOL(flush_dcache_page);

void __iomem *__arm_ioremap_pfn(unsigned long pfn, unsigned long offset,
				size_t size, unsigned int mtype)
{
	if (pfn >= (0x100000000ULL >> PAGE_SHIFT))
		return NULL;
	return (void __iomem *) (offset + (pfn << PAGE_SHIFT));
}
EXPORT_SYMBOL(__arm_ioremap_pfn);

void __iomem *__arm_ioremap(unsigned long phys_addr, size_t size,
			    unsigned int mtype)
{
	return (void __iomem *)phys_addr;
}
EXPORT_SYMBOL(__arm_ioremap);

void __iounmap(volatile void __iomem *addr)
{
}
EXPORT_SYMBOL(__iounmap);
