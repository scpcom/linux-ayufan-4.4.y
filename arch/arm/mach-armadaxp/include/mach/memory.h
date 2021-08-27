/*
 * include/asm-arm/arch-mv78xx0/memory.h
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#ifdef CONFIG_MV_DRAM_BASE
#define PLAT_PHYS_OFFSET		UL(CONFIG_MV_DRAM_BASE)
#else
#define PLAT_PHYS_OFFSET		UL(0x00000000)
#endif

#define BOOT_PARAMS_OFFSET      PLAT_PHYS_OFFSET + 0x100
/* #define __virt_to_bus(x)	__virt_to_phys(x) */
/* #define __bus_to_virt(x)	__phys_to_virt(x) */

/* Override the ARM default */
#ifdef CONFIG_SPARSEMEM
#define MAX_PHYSMEM_BITS       35
#define SECTION_SIZE_BITS      29
#endif

#if 0
#ifdef CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE

#if (CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE == 0)
#undef CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE
#define CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE 2
#endif

#define CONSISTENT_DMA_SIZE \
	(((CONFIG_FB_DOVE_CONSISTENT_DMA_SIZE + 1) & ~1) * 1024 * 1024)

#endif
#endif

#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY
#define arch_is_coherent()  1  
#endif

#endif
