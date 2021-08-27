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

/* for putstr */
/* #include <asm/arch/uncompress.h> */

MV_CPU_DEC_WIN* mv_sys_map(void);

#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_phys_base_get(u8 chan);
u32 mv_crypto_virt_base_get(u8 chan);
#endif

struct map_desc  MEM_TABLE[] =	{
	/* no use for pex mem remap */	
	{ INTER_REGS_BASE,		__phys_to_pfn(INTER_REGS_PHYS_BASE),	SZ_1M,  	     	MT_DEVICE},
	{ PEX0_IO_VIRT_BASE,   		__phys_to_pfn(PEX0_IO_PHYS_BASE),	PEX0_IO_SIZE,  		MT_DEVICE},
	{ PEX1_IO_VIRT_BASE,   		__phys_to_pfn(PEX1_IO_PHYS_BASE),	PEX1_IO_SIZE,  		MT_DEVICE},
	{ PEX2_IO_VIRT_BASE,   		__phys_to_pfn(PEX2_IO_PHYS_BASE),	PEX2_IO_SIZE,  		MT_DEVICE},
	{ PEX3_IO_VIRT_BASE,   		__phys_to_pfn(PEX3_IO_PHYS_BASE),	PEX3_IO_SIZE,  		MT_DEVICE},
	{ PEX4_IO_VIRT_BASE,   		__phys_to_pfn(PEX4_IO_PHYS_BASE),	PEX4_IO_SIZE,  		MT_DEVICE},
	{ PEX5_IO_VIRT_BASE,   		__phys_to_pfn(PEX5_IO_PHYS_BASE),	PEX5_IO_SIZE,  		MT_DEVICE},
	{ PEX6_IO_VIRT_BASE,   		__phys_to_pfn(PEX6_IO_PHYS_BASE),	PEX6_IO_SIZE,  		MT_DEVICE},
	{ PEX7_IO_VIRT_BASE,   		__phys_to_pfn(PEX7_IO_PHYS_BASE),	PEX7_IO_SIZE,  		MT_DEVICE},
	{ PEX8_IO_VIRT_BASE,   		__phys_to_pfn(PEX8_IO_PHYS_BASE),	PEX8_IO_SIZE,  		MT_DEVICE},
	{ PEX9_IO_VIRT_BASE,   		__phys_to_pfn(PEX9_IO_PHYS_BASE),	PEX9_IO_SIZE,  		MT_DEVICE},
#ifdef MV_INCLUDE_LEGACY_NAND
	{ LEGACY_NAND_VIRT_BASE,	__phys_to_pfn(LEGACY_NAND_PHYS_BASE),	LEGACY_NAND_SIZE, 	MT_DEVICE},
#endif
	{ CRYPT_ENG_VIRT_BASE(0),	__phys_to_pfn(CRYPT_ENG_PHYS_BASE(0)),	CRYPT_ENG_SIZE,		MT_DEVICE},
#ifdef CONFIG_MV_CESA
#if (CONFIG_MV_CESA_CHANNELS > 1)
	{ CRYPT_ENG_VIRT_BASE(1),	__phys_to_pfn(CRYPT_ENG_PHYS_BASE(1)),	CRYPT_ENG_SIZE,		MT_DEVICE},
#endif
#endif
};

MV_CPU_DEC_WIN SYSMAP_ARMADA_XP[] = {
	/* base low       	     base high        size       		WinNum     	enable */
	{{SDRAM_CS0_BASE,		0,	SDRAM_CS0_SIZE		},	0xFFFFFFFF,	DIS},	/* SDRAM_CS0 */
	{{SDRAM_CS1_BASE,		0,	SDRAM_CS1_SIZE		},	0xFFFFFFFF,	DIS},	/* SDRAM_CS1 */
	{{SDRAM_CS2_BASE,		0,	SDRAM_CS2_SIZE		},	0xFFFFFFFF,	DIS},	/* SDRAM_CS2 */
	{{SDRAM_CS3_BASE,		0,	SDRAM_CS3_SIZE		},	0xFFFFFFFF,	DIS},	/* SDRAM_CS3 */
	{{DEVICE_CS0_PHYS_BASE,		0,	DEVICE_CS0_SIZE,	},	0x8,		EN},	/* DEVICE_CS0 */
	{{DEVICE_CS1_PHYS_BASE,		0,	DEVICE_CS1_SIZE,	},	TBL_UNUSED,	DIS},	/* DEVICE_CS1 */
	{{DEVICE_CS2_PHYS_BASE,		0,	DEVICE_CS2_SIZE,	},	TBL_UNUSED,	DIS},	/* DEVICE_CS2 */
	{{DEVICE_CS3_PHYS_BASE,		0,	DEVICE_CS3_SIZE,	},	TBL_UNUSED,	DIS},	/* DEVICE_CS3 */
	{{PEX0_MEM_PHYS_BASE,		0,	PEX0_MEM_SIZE		},	0x0,		EN},	/* PEX0_MEM */
	{{PEX0_IO_PHYS_BASE,		0,	PEX0_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX0_IO */
	{{PEX1_MEM_PHYS_BASE,		0,	PEX1_MEM_SIZE		},	0x1,		EN},	/* PEX1_MEM */
	{{PEX1_IO_PHYS_BASE,		0,	PEX1_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX1_IO */
	{{PEX2_MEM_PHYS_BASE,		0,	PEX2_MEM_SIZE		},	0x2,		EN},	/* PEX2_MEM */
	{{PEX2_IO_PHYS_BASE,		0,	PEX2_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX2_IO */
	{{PEX3_MEM_PHYS_BASE,		0,	PEX3_MEM_SIZE		},	0x3,		EN},	/* PEX3_MEM */
	{{PEX3_IO_PHYS_BASE,		0,	PEX3_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX3_IO */
	{{PEX4_MEM_PHYS_BASE,		0,	PEX4_MEM_SIZE		},	0x4,		EN},	/* PEX4_MEM */
	{{PEX4_IO_PHYS_BASE,		0,	PEX4_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX4_IO */
	{{PEX5_MEM_PHYS_BASE,		0,	PEX5_MEM_SIZE		},	TBL_UNUSED,	DIS},	/* PEX5_MEM */
	{{PEX5_IO_PHYS_BASE,		0,	PEX5_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX5_IO */
	{{PEX6_MEM_PHYS_BASE,		0,	PEX6_MEM_SIZE		},	0x5,		EN},	/* PEX6_MEM */
	{{PEX6_IO_PHYS_BASE,		0,	PEX6_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX6_IO */
	{{PEX7_MEM_PHYS_BASE,		0,	PEX7_MEM_SIZE		},	TBL_UNUSED,	DIS},	/* PEX7_MEM */
	{{PEX7_IO_PHYS_BASE,		0,	PEX7_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX7_IO */
	{{PEX8_MEM_PHYS_BASE,		0,	PEX8_MEM_SIZE		},	0x6,		EN},	/* PEX8_MEM */
	{{PEX8_IO_PHYS_BASE,		0,	PEX8_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX8_IO */
	{{PEX9_MEM_PHYS_BASE,		0,	PEX9_MEM_SIZE		},	0x7,		EN},	/* PEX9_MEM */
	{{PEX9_IO_PHYS_BASE,		0,	PEX9_IO_SIZE		},	TBL_UNUSED,	DIS},	/* PEX9_IO */
	{{INTER_REGS_PHYS_BASE,		0,	INTER_REGS_SIZE		},	0x14,		EN},	/* INTER_REGS */
	{{UART_REGS_BASE,		0,	UART_SIZE		},	TBL_UNUSED,	DIS},	/* DMA_UART */
	{{SPI_CS0_PHYS_BASE,		0,	SPI_CS0_SIZE		},	0xe,		EN},	/* SPI_CS0 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS1 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS2 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS3 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS4 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS5 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS6 */
	{{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	/* SPI_CS7 */
	{{BOOTROM_PHYS_BASE,		0,	BOOTROM_SIZE		},	0x9,		EN},	/* BOOTROM */
	{{DEVICE_BOOTCS_PHYS_BASE,	0,	DEVICE_BOOTCS_SIZE	},	0xa,		EN},	/* DEV_BOOCS */
	{{PMU_SCRATCH_PHYS_BASE,	0,	PMU_SCRATCH_SIZE	},	TBL_UNUSED,	DIS},	/* PMU SCRATCHPAD */
	{{CRYPT_ENG_PHYS_BASE(0),	0,	CRYPT_ENG_SIZE		},	0xb,		EN},	/* CRYPT0_ENG */
	{{CRYPT_ENG_PHYS_BASE(1),	0,	CRYPT_ENG_SIZE		},	0xc,		EN},	/* CRYPT1_ENG */
	{{PNC_BM_PHYS_BASE,		0,	PNC_BM_SIZE		},	0xd,		EN},	/* PNC_BM */
	{{TBL_TERM,				TBL_TERM, TBL_TERM	},	TBL_TERM,	TBL_TERM}
};

MV_CPU_DEC_WIN* mv_sys_map(void)
{
	return SYSMAP_ARMADA_XP;
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

void __init axp_map_io(void)
{
        iotable_init(MEM_TABLE, ARRAY_SIZE(MEM_TABLE));
}

static u32 mv_pci_mem_base[] = 
{
	PEX0_MEM_PHYS_BASE,
	PEX1_MEM_PHYS_BASE,
	PEX2_MEM_PHYS_BASE,
	PEX3_MEM_PHYS_BASE,
	PEX4_MEM_PHYS_BASE,
	PEX5_MEM_PHYS_BASE,
	PEX6_MEM_PHYS_BASE,
	PEX7_MEM_PHYS_BASE,
	PEX8_MEM_PHYS_BASE,
	PEX9_MEM_PHYS_BASE,
};

static u32 mv_pci_mem_size[] = 
{
	PEX0_MEM_SIZE,
	PEX1_MEM_SIZE,
	PEX2_MEM_SIZE,
	PEX3_MEM_SIZE,
	PEX4_MEM_SIZE,
	PEX5_MEM_SIZE,
	PEX6_MEM_SIZE,
	PEX7_MEM_SIZE,
	PEX8_MEM_SIZE,
	PEX9_MEM_SIZE,
};

static u32 mv_pci_io_base[] = 
{
	PEX0_IO_PHYS_BASE,
	PEX1_IO_PHYS_BASE,
	PEX2_IO_PHYS_BASE,
	PEX3_IO_PHYS_BASE,
	PEX4_IO_PHYS_BASE,
	PEX5_IO_PHYS_BASE,
	PEX6_IO_PHYS_BASE,
	PEX7_IO_PHYS_BASE,
	PEX8_IO_PHYS_BASE,
	PEX9_IO_PHYS_BASE
};

static u32 mv_pci_io_size[] = 
{
	PEX0_IO_SIZE,
	PEX1_IO_SIZE,
	PEX2_IO_SIZE,
	PEX3_IO_SIZE,
	PEX4_IO_SIZE,
	PEX5_IO_SIZE,
	PEX6_IO_SIZE,
	PEX7_IO_SIZE,
	PEX8_IO_SIZE,
	PEX9_IO_SIZE,
};

static MV_TARGET mv_pci_io_target[] = 
{
	PEX0_IO,
	PEX1_IO,
	PEX2_IO,
	PEX3_IO,
	PEX4_IO,
	PEX5_IO,
	PEX6_IO,
	PEX7_IO,
	PEX8_IO,
	PEX9_IO,
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
	   for 8 PCIE mem BARs.
	   This is disabled as long that no more windows are available for
	   I/O BARs
	*/
	    
	return 0;
}
