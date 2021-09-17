#ifndef __HI_CHIP_REGS_H__
#define __HI_CHIP_REGS_H__

#include <mach/io.h>

/* SRAM Base Address Register */
#define SRAM_BASE_ADDRESS			0x04010000

#define REG_BASE_TIMER01			0x12000000
#define REG_BASE_TIMER23			0x12010000
#define REG_BASE_TIMER45			0x12020000
#define REG_BASE_TIMER67			0x12030000

/* -------------------------------------------------------------------- */
/* Clock and Reset Generator REG */
/* -------------------------------------------------------------------- */
#define REG_BASE_CRG				0x12040000
#define REG_PERI_CRG33				IO_ADDRESS(REG_BASE_CRG + 0x84)

#define REG_CRG32				0x0080

/* DMAC CRG register offset */
#define REG_DMAC_CRG				REG_CRG32
#define DMAC_CLK_EN				BIT(5)
#define DMAC_SRST_REQ				BIT(4)

/* Ethernet CRG register offset */
#define REG_ETH_CRG				0x0078
#define REG_ETH_MAC_IF				0x008C

#define UART_CKSEL_MASK				(0x3 << 18)
#define UART_CKSEL_24M				(0x2 << 18)

#define REG_BASE_CUR_UART			REG_BASE_UART0

#define REG_BASE_A7_PERI			0x10300000

/*CORTTX-A7 MPCORE MEMORY REGION*/
#define REG_A7_PERI_GIC_CPU			0x2000
#define REG_A7_PERI_GIC_DIST			0x1000

/* -------------------------------------------------------------------- */
/* System Control REG */
/* -------------------------------------------------------------------- */
#define SYS_CTRL_BASE				0x12050000

/* System Control register offset */
#define REG_SC_CTRL				0x0000
#define REG_BASE_SCTL				(SYS_CTRL_BASE + 0)

/* System soft reset register offset */
#define REG_SC_SYSRES                                   0x0004
#define REG_SC_XTALCTRL                                 0x0010
#define REG_SC_APLLCTRL                                 0x0014
#define REG_SC_APLLFREQCTRL0                            0x0018
#define REG_SC_APLLFREQCTRL1                            0x001C
#define REG_SC_VPLL0FREQCTRL0                           0x0020
#define REG_SC_VPLL0FREQCTRL1                           0x0024
#define REG_SC_VPLL1FREQCTRL0                           0x0028
#define REG_SC_VPLL1FREQCTRL1                           0x002C
#define REG_SC_EPLLFREQCTRL0                            0x0030
#define REG_SC_EPLLFREQCTRL1                            0x0034
#define REG_SC_QPLLFREQCTRL0                            0x0038
#define REG_SC_QPLLFREQCTRL1                            0x003C
#define REG_SC_LOW_POWER_CTRL                           0x0040
#define REG_SC_IO_REUSE_SEL                             0x0044
#define REG_SC_SRST_REQ_CTRL                            0x0048
#define REG_SC_CA_RST_CTRL                              0x004C
#define REG_SC_WDG_RST_CTRL                             0x0050
#define REG_SC_DDRC_DFI_RST_CTRL                        0x0054
#define REG_SC_PLLLOCK_STAT                             0x0070
#define REG_SC_GEN0                                     0x0080
#define REG_SC_GEN1                                     0x0084
#define REG_SC_GEN2                                     0x0088
#define REG_SC_GEN3                                     0x008C
#define REG_SC_GEN4                                     0x0090
#define REG_SC_GEN5                                     0x0094
#define REG_SC_GEN6                                     0x0098
#define REG_SC_GEN7                                     0x009C
#define REG_SC_GEN8                                     0x00A0
#define REG_SC_GEN9                                     0x00A4
#define REG_SC_GEN10                                    0x00A8
#define REG_SC_GEN11                                    0x00AC
#define REG_SC_GEN12                                    0x00B0
#define REG_SC_GEN13                                    0x00B4
#define REG_SC_GEN14                                    0x00B8
#define REG_SC_GEN15                                    0x00BC
#define REG_SC_GEN16                                    0x00C0
#define REG_SC_GEN17                                    0x00C4
#define REG_SC_GEN18                                    0x00C8
#define REG_SC_GEN19                                    0x00CC
#define REG_SC_GEN20                                    0x00D0
#define REG_SC_GEN21                                    0x00D4
#define REG_SC_GEN22                                    0x00D8
#define REG_SC_GEN23                                    0x00DC
#define REG_SC_GEN24                                    0x00E0
#define REG_SC_GEN25                                    0x00E4
#define REG_SC_GEN26                                    0x00E8
#define REG_SC_GEN27                                    0x00EC
#define REG_SC_GEN28                                    0x00F0
#define REG_SC_GEN29                                    0x00F4
#define REG_SC_GEN30                                    0x00F8
#define REG_SC_GEN31                                    0x00FC
#define REG_SC_LOCKEN                                   0x020C
#define REG_SC_SYSID0                                   0x0EE0
#define REG_SC_SYSID1                                   0x0EE4
#define REG_SC_SYSID2                                   0x0EE8
#define REG_SC_SYSID3                                   0x0EEC

/* System Status register offset */
#define SYS_CTRL_SYSSTAT			0x8c
#define GET_SPI_DEVICE_TYPE(_reg)		(((_reg) >> 8) & 0x1)
/* bit[8]=0; bit[7]:SPI nor address mode; bit[7]=(0:3-Byte | 1:4-Byte) */
#define GET_SPI_NOR_ADDRESS_MODE(_reg)		(((_reg) >> 7) & 0x1)

#define REG_PERI_SOC_FUSE                               0x840

#define REG_BASE_WDG0                                   0x12070000
#define REG_PERI_CRG10                                  0x0028

#define CFG_GIC_CPU_BASE    (IO_ADDRESS(REG_BASE_A7_PERI) \
					+ REG_A7_PERI_GIC_CPU)
#define CFG_GIC_DIST_BASE   (IO_ADDRESS(REG_BASE_A7_PERI) \
					+ REG_A7_PERI_GIC_DIST)

/* -------------------------------------------------------------------- */
/* UART Control REG */
/* -------------------------------------------------------------------- */
#define REG_BASE_UART0				0x12080000
#define REG_BASE_UART1				0x12090000
#define REG_BASE_UART2				0x120a0000

#define REG_UART_DATA				0x0000
#define REG_UART_FLAG				0x0018
#define REG_UART_CTRL				0x0030
#define REG_UART_DMA_CR				0x0048

/* -------------------------------------------------------------------- */
/* I2C Control REG */
/* -------------------------------------------------------------------- */
#define I2C_REG_BASE				0x120c0000

#define REG_I2C_DATA				0x0010

/* -------------------------------------------------------------------- */
/* SSP Control REG */
/* -------------------------------------------------------------------- */
#define SSP_REG_BASE				0x120d0000

#define REG_SSP_DATA				0x0008

/*********************************************************************/
#define A7_AXI_SCALE_REG	IO_ADDRESS(REG_BASE_CRG + 0x34)

/* -------------------------------------------------------------------- */
#define DDR_MEM_BASE			0x80000000

/* -------------------------------------------------------------------- */
#define get_bus_clk() ({ \
	unsigned long tmp_reg, busclk = 0;\
	tmp_reg = readl((void *)A7_AXI_SCALE_REG);\
	tmp_reg = (tmp_reg >> 12) & 0x3;\
	if (0x1 == tmp_reg) {\
		busclk = 250000000;\
	} else if (0x2 == tmp_reg) {\
		busclk = 202500000;\
	} \
	busclk;\
})
/*********************************************************************/
/*
 * 0x1-> init item1
 * 0x2-> init item2
 * 0x3->init item1 & item2
 */
#define INIT_REG_ITEM1               1
#define INIT_REG_ITEM2               2
#define INIT_REG_ITEM1_ITEM2         (INIT_REG_ITEM1 | INIT_REG_ITEM2)

/*-----------------------------------------------------------------------
 * PERI_CRG29 FMC REG
 * ----------------------------------------------------------------------*/
#define FMC_CRG29			0x74
#define FMC_CRG29_CLK_SEL(_clk)		(((_clk) & 0x3) << 2)
#define FMC_CRG29_CLK_EN		(1 << 1)
#define FMC_CRG29_SOFT_RST_REQ		(1 << 0)

#define FMC_CLK_SEL_MASK		(0x3 << 2)

#define CLK_24M				0x0
#define CLK_83M				0x1
#define CLK_150M			0x2

#define FMC_CLK_SEL_24M		FMC_CRG29_CLK_SEL(CLK_24M)
#define FMC_CLK_SEL_83M		FMC_CRG29_CLK_SEL(CLK_83M)
#define FMC_CLK_SEL_150M	FMC_CRG29_CLK_SEL(CLK_150M)

#endif /* End of __HI_CHIP_REGS_H__ */
