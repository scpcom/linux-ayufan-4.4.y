/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mvSysHwConfig.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "boardEnv/mvBoardEnvLib.h"
#include <asm/mach/map.h>
#include <asm/smp_twd.h>
#include "ca9x2.h"

struct map_desc MEM_TABLE[] = {
	/* no use for pex mem remap */
	{ PEX0_IO_VIRT_BASE,		__phys_to_pfn(PEX0_IO_PHYS_BASE),	SZ_1M,		MT_MEMORY_SO },
	{ PEX1_IO_VIRT_BASE,		__phys_to_pfn(PEX1_IO_PHYS_BASE),	SZ_1M,		MT_MEMORY_SO },

	{ PEX0_MEM_VIRT_BASE,		__phys_to_pfn(PEX0_MEM_PHYS_BASE),	SZ_16M,		MT_MEMORY_SO },
	{ PEX1_MEM_VIRT_BASE,		__phys_to_pfn(PEX1_MEM_PHYS_BASE),	SZ_16M,		MT_MEMORY_SO },

	{ INTER_REGS_VIRT_BASE,		__phys_to_pfn(INTER_REGS_PHYS_BASE),	SZ_1M,		MT_DEVICE},

	{ CRYPT_ENG_VIRT_BASE(0),	__phys_to_pfn(CRYPT_ENG_PHYS_BASE(0)),	CRYPT_ENG_SIZE,	MT_DEVICE},
#ifdef CONFIG_MV_CESA
#if (CONFIG_MV_CESA_CHANNELS > 1)
	{ CRYPT_ENG_VIRT_BASE(1),       __phys_to_pfn(CRYPT_ENG_PHYS_BASE(1)),  CRYPT_ENG_SIZE, MT_DEVICE},
#endif
#endif
	{ IOCC_WA_WIN0_VIRT_BASE,	__phys_to_pfn(IOCC_WA_WIN0_PHYS_BASE),	SZ_64K,		MT_DEVICE},
};

MV_CPU_DEC_WIN SYSMAP_A375_6720[] = {
	/* base low             base high       size                            WinNum          enable */
	{{SDRAM_CS0_BASE,		0,	SDRAM_CS0_SIZE		},	0xFFFFFFFF,	DIS}, /* SDRAM_CS0 */
	{{SDRAM_CS1_BASE,		0,	SDRAM_CS1_SIZE		},	0xFFFFFFFF,	DIS}, /* SDRAM_CS1 */
	{{SDRAM_CS2_BASE,		0,	SDRAM_CS2_SIZE		},	0xFFFFFFFF,	DIS}, /* SDRAM_CS2 */
	{{SDRAM_CS3_BASE,		0,	SDRAM_CS3_SIZE		},	0xFFFFFFFF,	DIS}, /* SDRAM_CS3 */
	{{DEVICE_CS0_PHYS_BASE,		0,	DEVICE_CS0_SIZE,	},	8,		 EN}, /* DEVICE_CS0 */
	{{DEVICE_CS1_PHYS_BASE,		0,	DEVICE_CS1_SIZE,	},	TBL_UNUSED,	DIS}, /* DEVICE_CS1 */
	{{DEVICE_CS2_PHYS_BASE,		0,	DEVICE_CS2_SIZE,	},	TBL_UNUSED,	DIS}, /* DEVICE_CS2 */
	{{DEVICE_CS3_PHYS_BASE,		0,	DEVICE_CS3_SIZE,	},	TBL_UNUSED,	DIS}, /* DEVICE_CS3 */
	{{PEX0_MEM_PHYS_BASE,		0,	PEX0_MEM_SIZE		},	0,		EN}, /* PEX0_MEM */
	{{PEX0_IO_PHYS_BASE,		0,	PEX0_IO_SIZE		},	1,		EN}, /* PEX0_IO */
	{{PEX1_MEM_PHYS_BASE,		0,	PEX1_MEM_SIZE		},	2,		EN}, /* PEX1_MEM */
	{{PEX1_IO_PHYS_BASE,		0,	PEX1_IO_SIZE		},	3,		EN}, /* PEX1_IO */
	{{INTER_REGS_PHYS_BASE,		0,	INTER_REGS_SIZE		},	20,		 EN}, /* INTER_REGS */
	{{UART_REGS_BASE,		0,	UART_SIZE		},	TBL_UNUSED,	DIS}, /* DMA_UART */
	{{SPI_CS0_PHYS_BASE,		0,	SPI_CS0_SIZE		},	14,		 EN}, /* SPI_CS0 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS1 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS2 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS3 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS4 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS5 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS6 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS}, /* SPI_CS7 */
	{{BOOTROM_PHYS_BASE,		0,	BOOTROM_SIZE		},	9,		DIS}, /* BOOTROM */
	{{DEVICE_BOOTCS_PHYS_BASE,	0,	DEVICE_BOOTCS_SIZE	},	10,		 EN}, /* DEV_BOOCS */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},
	{{CRYPT_ENG_PHYS_BASE(0),	0,	CRYPT_ENG_SIZE		},	12,		 EN}, /* CRYPT_ENG0 */
	{{CRYPT_ENG_PHYS_BASE(1),	0,	CRYPT_ENG_SIZE		},	4,		 EN}, /* CRYPT_ENG1 */

	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},
	{{TBL_TERM,		 TBL_TERM,	TBL_TERM		},	TBL_TERM,  TBL_TERM}
};

MV_CPU_DEC_WIN *mv_sys_map(void)
{
	MV_CPU_DEC_WIN *map;
	MV_U16 ctrlModel = mvCtrlModelGet();

	switch (ctrlModel) {
	case MV_6720_DEV_ID:
		map = SYSMAP_A375_6720;
		break;
	default:
		pr_warn("%s: Error: Wrong ctrlModel (%d)\n", __func__,
			ctrlModel);
		map = SYSMAP_A375_6720;
	}

	return map;
}

#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_phys_base_get(u8 chan)
{
	return CRYPT_ENG_PHYS_BASE(chan);
}

u32 mv_crypto_virt_base_get(u8 chan)
{
	return CRYPT_ENG_VIRT_BASE(chan);
}
#endif

void __init a375_map_io(void)
{
	iotable_init(MEM_TABLE, ARRAY_SIZE(MEM_TABLE));
}

static u32 mv_pci_mem_base[] = {
	PEX0_MEM_PHYS_BASE,
	PEX1_MEM_PHYS_BASE,
};

static u32 mv_pci_mem_size[] = {
	PEX0_MEM_SIZE,
	PEX1_MEM_SIZE,
};

static u32 mv_pci_io_base[] = {
	PEX0_IO_PHYS_BASE,
	PEX1_IO_PHYS_BASE,
};

static u32 mv_pci_io_size[] = {
	PEX0_IO_SIZE,
	PEX1_IO_SIZE,
};

static MV_TARGET mv_pci_io_target[] = {
	PEX0_IO,
	PEX1_IO,
};

u32 mv_pci_mem_base_get(int ifNum)
{
	return mv_pci_mem_base[ifNum];
}

u32 mv_pci_mem_size_get(int ifNum)
{
	return mv_pci_mem_size[ifNum];
}

u32 mv_pci_io_base_get(int ifNum)
{
	return mv_pci_io_base[ifNum];
}

u32 mv_pci_io_size_get(int ifNum)
{
	return mv_pci_io_size[ifNum];
}

MV_TARGET mv_pci_io_target_get(int ifNum)
{
	return mv_pci_io_target[ifNum];
}

int mv_is_pci_io_mapped(int ifNum)
{
	/* FIXME: First 8 address decode windows are statically assigned
	 * for 8 PCIE mem BARs.
	 * This is disabled as long that no more windows are available for
	 * I/O BARs
	 */

	return 0;
}
