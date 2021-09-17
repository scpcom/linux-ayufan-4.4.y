#ifndef __HI_CHIP_REGS_H__
#define __HI_CHIP_REGS_H__

#include <mach/io.h>

#define REG_BASE_TIMER01			0x12000000
#define REG_BASE_TIMER23			0x12010000
#define REG_BASE_TIMER45			0x12020000
#define REG_BASE_TIMER67			0x12030000
#define REG_BASE_TIMER89			0x12060000

/* -------------------------------------------------------------------- */
/* Clock and Reset Generator REG */
/* -------------------------------------------------------------------- */
#define REG_BASE_CRG				0x12040000

#define REG_PERI_CRG10				0x0028
#define REG_PERI_CRG98				0x0188

#define UART_CKSEL_APB				(1 << 19)

/* -------------------------------------------------------------------- */
/* System Control REG */
/* -------------------------------------------------------------------- */
#define REG_BASE_SCTL				0x12050000

/* System Control register offset */
#define REG_SC_CTRL				0x0000

/* System soft reset register offset */
#define REG_SC_SYSRES				0x0004

/* -------------------------------------------------------------------- */
/* UART Control REG */
/* -------------------------------------------------------------------- */
#define REG_BASE_UART0				0x12080000
#define REG_BASE_UART1				0x12090000
#define REG_BASE_UART2				0x120a0000
#define REG_BASE_UART3				0x120b0000

#define REG_BASE_CUR_UART			REG_BASE_UART2

/* -------------------------------------------------------------------- */
#define REG_BASE_A17_PERI                               0x1fff0000

/*CORTTX-A17 MPCORE MEMORY REGION*/
#define REG_A17_PERI_SCU                                 0x0000
#define REG_A17_PERI_GIC_CPU                             0x2000
/* #define REG_A17_PERI_GLOBAL_TIMER                        0x0200 */
/* #define REG_A17_PERI_PRI_TIMER_WDT                       0x0600 */
#define REG_A17_PERI_GIC_DIST                            0x1000

#define CFG_GIC_CPU_BASE    (IO_ADDRESS(REG_BASE_A17_PERI) \
				+ REG_A17_PERI_GIC_CPU)
#define CFG_GIC_DIST_BASE   (IO_ADDRESS(REG_BASE_A17_PERI) \
				+ REG_A17_PERI_GIC_DIST)

#endif /* End of __HI_CHIP_REGS_H__ */
