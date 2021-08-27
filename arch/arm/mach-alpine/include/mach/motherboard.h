#ifndef __MACH_MOTHERBOARD_H
#define __MACH_MOTHERBOARD_H

/*
 * Physical addresses, offsets
 */
#define SLR_NOR0		0xf4000000
#define SLR_MSIX_SPACE_START_HI	0x00000000
#define SLR_MSIX_SPACE_START_LO	0xfbe00000
#define SLR_SB_IREG		0xfc000000
#define SLR_NB_IREG		0xfb000000
#define SLR_NB_IREG_SIZE	SZ_2M
#define SLR_SB_LAD_OFF		0x01880000
#define SLR_SB_LAD		((SLR_SB_IREG) + SLR_SB_LAD_OFF) /* Misc UART (depend boot.S) Sys (boot.S), Timer, Wdog etc */
#define SLR_INT_PCI_ECAM_BASE 	0xfbc00000
#define SLR_INT_PCI_ECAM_SIZE 	SZ_1M
#define SLR_INT_PCI_MEM_BASE  	0xfe000000 /* registers space */
#define SLR_INT_PCI_MEM_SIZE  	SZ_4M

/*
 * CTA15 - Physical base addresses
 */
#define CA15X4_MPIC          	(SLR_NB_IREG)

#define A15_MPCORE_GIC_DIST     (CA15X4_MPIC + 0x1000)
#define A15_MPCORE_GIC_CPU      (CA15X4_MPIC + 0x2000)
#define SLR_NB_IREG_SERVICE	(SLR_NB_IREG + 0x70000)

/* NB service registers */
#define SLR_NB_INIT_CONTROL		(SLR_NB_IREG_SERVICE + 0x8)
#define SLR_NB_CPUN_CFG_STAT_BASE	(SLR_NB_IREG_SERVICE + 0x2000)
#define SLR_NB_CPUN_CFG_STAT_SIZE	0x100
#define SLR_NB_CPUN_RESUME_ADDR		(SLR_NB_CPUN_CFG_STAT_BASE + 0x28)

#define SLR_NB_CPU_RESUME_ADDR(cpu)	\
	(SLR_NB_CPUN_RESUME_ADDR + ( (cpu) * SLR_NB_CPUN_CFG_STAT_SIZE) )

/* Common peripherals relative to LAD. */
#define SLR_LAD_PER_OFFSET(x)	(x << 12)
#define SLR_SYSREGS		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(1))
#define SLR_UART0		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(3))
#define SLR_UART1		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(4))
#define SLR_UART2		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(5))
#define SLR_UART3		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(6))
#define SLR_WDT			(SLR_SB_LAD + SLR_LAD_PER_OFFSET(12))
#define SLR_TIMER01		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(16))
#define SLR_TIMER23		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(17))
#define SLR_MMCI		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(0))
#define SLR_LAN9118		(SLR_SB_LAD + SLR_LAD_PER_OFFSET(2)) /* Eth0*/

#define SLR_SB_LAD_SIZE		SLR_LAD_PER_OFFSET(32)

/* System register offsets. */
#define SLR_SYS_24MHZ		(SLR_SYSREGS + 0x05c)

#define SLR_TIMER0		(SLR_TIMER01 + 0x000)
#define SLR_TIMER1		(SLR_TIMER01 + 0x020)

#define SLR_TIMER2		(SLR_TIMER23 + 0x000)
#define SLR_TIMER3		(SLR_TIMER23 + 0x020)

/*
 * Interrupts.  Those in {} are for AMBA devices
 */
#define IRQ_SLR_WDT		{ (32 + 0) }
#define IRQ_SLR_TIMER0		(32 + 2)
#define IRQ_SLR_TIMER1		(32 + 2)
#define IRQ_SLR_TIMER2		(32 + 3)
#define IRQ_SLR_TIMER3		(32 + 3)
#define IRQ_SLR_RTC		{ (32 + 4) }
#define IRQ_SLR_UART0		{ (32 + 5) }
#define IRQ_SLR_UART1		{ (32 + 6) }
#define IRQ_SLR_UART2		{ (32 + 7) }
#define IRQ_SLR_UART3		{ (32 + 8) }
#define IRQ_SLR_MMCI		{ (32 + 9), (32 + 10) }
#define IRQ_SLR_LAN9118		(32 + 15)
#define IRQ_SLR_PCIE		(32 + 17)
#define IRQ_SLR_PMU_CPU(x)	(50 + (x))
#endif
