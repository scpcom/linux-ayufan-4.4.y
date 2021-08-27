/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#ifdef CONFIG_MV_DRAM_BASE
#define PLAT_PHYS_OFFSET		UL(CONFIG_MV_DRAM_BASE)
#else
#define PLAT_PHYS_OFFSET		UL(0x00000000)
#endif

#define BOOT_PARAMS_OFFSET		(PLAT_PHYS_OFFSET + 0x100)

/* Override the ARM default */
#ifdef CONFIG_SPARSEMEM
#define MAX_PHYSMEM_BITS		35
#define SECTION_SIZE_BITS		29
#endif

#ifdef CONFIG_AURORA_IO_CACHE_COHERENCY
#define arch_is_coherent()		1
#endif

#endif /* __ASM_ARCH_MEMORY_H */
