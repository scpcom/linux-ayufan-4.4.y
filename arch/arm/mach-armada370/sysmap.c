#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvSysHwConfig.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "boardEnv/mvBoardEnvLib.h"
#include <asm/mach/map.h>

MV_CPU_DEC_WIN* mv_sys_map(void);

#if defined(CONFIG_MV_INCLUDE_CESA)
u32 mv_crypto_phys_base_get(u8 chan);
u32 mv_crypto_virt_base_get(u8 chan);
#endif

struct map_desc  MEM_TABLE[] =	{
	 
	{ INTER_REGS_BASE,		__phys_to_pfn(INTER_REGS_PHYS_BASE),	SZ_1M,  	     	MT_DEVICE},
	{ PEX0_IO_VIRT_BASE,   		__phys_to_pfn(PEX0_IO_PHYS_BASE),	PEX0_IO_SIZE,  		MT_DEVICE},
	{ PEX1_IO_VIRT_BASE,   		__phys_to_pfn(PEX1_IO_PHYS_BASE),	PEX1_IO_SIZE,  		MT_DEVICE},
#ifdef MV_INCLUDE_LEGACY_NAND
	{ LEGACY_NAND_VIRT_BASE,	__phys_to_pfn(LEGACY_NAND_PHYS_BASE),	LEGACY_NAND_SIZE, 	MT_DEVICE},
#endif
	{ SPI_CS0_VIRT_BASE,		__phys_to_pfn(SPI_CS0_PHYS_BASE),	SPI_CS0_SIZE,		MT_DEVICE},
	{ CRYPT_ENG_VIRT_BASE(0),	__phys_to_pfn(CRYPT_ENG_PHYS_BASE(0)),	CRYPT_ENG_SIZE,		MT_DEVICE}
};

MV_CPU_DEC_WIN SYSMAP_ARMADA_370[] = {
	 
	{{SDRAM_CS0_BASE,		0,	SDRAM_CS0_SIZE		},	0xFFFFFFFF,	DIS},	 
	{{SDRAM_CS1_BASE,		0,	SDRAM_CS1_SIZE		},	0xFFFFFFFF,	DIS},	 
	{{SDRAM_CS2_BASE,		0,	SDRAM_CS2_SIZE		},	0xFFFFFFFF,	DIS},	 
	{{SDRAM_CS3_BASE,		0,	SDRAM_CS3_SIZE		},	0xFFFFFFFF,	DIS},	 
        {{DEVICE_CS0_PHYS_BASE,		0,	DEVICE_CS0_SIZE,	},	0x5,		EN},	 
        {{DEVICE_CS1_PHYS_BASE,		0,	DEVICE_CS1_SIZE,	},	TBL_UNUSED,	DIS},	 
        {{DEVICE_CS2_PHYS_BASE,		0,	DEVICE_CS2_SIZE,	},	TBL_UNUSED,	DIS},	 
	{{DEVICE_CS3_PHYS_BASE,		0,	DEVICE_CS3_SIZE,	},	TBL_UNUSED,	DIS},	 
	{{PEX0_MEM_PHYS_BASE,		0,	PEX0_MEM_SIZE		},	0x0,		EN},	 
	{{PEX0_IO_PHYS_BASE,		0,	PEX0_IO_SIZE		},	0x1,		EN},	 
	{{PEX1_MEM_PHYS_BASE,		0,	PEX1_MEM_SIZE		},	0x2,		EN},	 
	{{PEX1_IO_PHYS_BASE,		0,	PEX1_IO_SIZE		},	0x3,		EN},	 
	{{INTER_REGS_PHYS_BASE,		0,	INTER_REGS_SIZE		},	0x14,		EN},	 
	{{UART_REGS_BASE,		0,	UART_SIZE		},	TBL_UNUSED,	DIS},	 
	{{SPI_CS0_PHYS_BASE,		0,	SPI_CS0_SIZE		},	0x6,		EN},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
        {{TBL_UNUSED,			0,	TBL_UNUSED,		},	TBL_UNUSED,	DIS},	 
	{{BOOTROM_PHYS_BASE,		0,	DEVICE_BOOTCS_SIZE	},	0x6,		DIS},	 
	{{DEVICE_BOOTCS_PHYS_BASE,	0,	DEVICE_BOOTCS_SIZE	},	0x7,		EN},	 
	{{PMU_SCRATCH_PHYS_BASE,	0,	PMU_SCRATCH_SIZE	},	TBL_UNUSED,	DIS},	 
	{{CRYPT_ENG_PHYS_BASE(0),	0,	CRYPT_ENG_SIZE		},	0x8,		EN},	 
	{{TBL_TERM,			TBL_TERM, TBL_TERM		},	TBL_TERM,	TBL_TERM}
};

MV_CPU_DEC_WIN* mv_sys_map(void)
{
	switch(mvBoardIdGet()) {
		case DB_88F6710_BP_ID:
		case RD_88F6710_ID:
#if defined(CONFIG_SYNO_ARMADA_ARCH)
		case SYNO_DS213j_ID:
		case SYNO_US3_ID:
		case SYNO_RS214_ID:
		case SYNO_DS214se_ID:
		case SYNO_DS414slim_ID:
		case SYNO_DS115j_ID:
		case SYNO_DS216se_ID:
#endif
			return SYSMAP_ARMADA_370;
		default:
			printk("ERROR: can't find system address map\n");
			return NULL;
        }
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

#if 0
static u32 mv_pci_mem_base[] =
{
	PEX0_MEM_PHYS_BASE,
	PEX1_MEM_PHYS_BASE
};

static u32 mv_pci_mem_size[] =
{
	PEX0_MEM_SIZE,
	PEX1_MEM_SIZE
};

static u32 mv_pci_io_base[] =
{
	PEX0_IO_PHYS_BASE,
	PEX1_IO_PHYS_BASE
};

static u32 mv_pci_io_size[] =
{
	PEX0_IO_SIZE,
	PEX1_IO_SIZE
};

static MV_TARGET mv_pci_io_target[] =
{
	PEX0_IO,
	PEX1_IO
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
	 
	return 1;
}
#endif
