/*
 * include/asm-arm/arch-aurora/irqs.h
 *
 * IRQ definitions for Marvell Dove MV88F6781 SoC
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

/*
 * Aurora Low Interrupt Controller
 */

#define IRQ_AURORA_IN_DRBL_LOW	0
#define IRQ_AURORA_IN_DRBL_HIGH	1
#define IRQ_AURORA_OUT_DRBL 	2
#define IRQ_AURORA_MP		3
#define IRQ_AURORA_SOC_ERROR	4
#define IRQ_AURORA_TIMER0	5
#define IRQ_LOCALTIMER 		IRQ_AURORA_TIMER0
#define IRQ_AURORA_TIMER1	6
#define IRQ_AURORA_WD		7

#define IRQ_AURORA_GBE0_FIC	8
#define IRQ_AURORA_GBE0_SIC	9
#define IRQ_AURORA_GBE1_FIC	10
#define IRQ_AURORA_GBE1_SIC	11
#define IRQ_AURORA_GBE2_FIC     12
#define IRQ_AURORA_GBE2_SIC	13
#define IRQ_AURORA_GBE3_FIC	14
#define IRQ_AURORA_GBE3_SIC	15

#define IRQ_AURORA_LCD		29
#define IRQ_AURORA_SPI          30
#define IRQ_AURORA_I2C0		31
#define IRQ_AURORA_I2C1		32

#define IRQ_AURORA_DMA0		33
#define IRQ_AURORA_DMA1		34
#define IRQ_AURORA_DMA2		35
#define IRQ_AURORA_DMA3		36

#define IRQ_AURORA_GLOB_TIMER0	37
#define IRQ_AURORA_GLOB_TIMER1	38
#define IRQ_AURORA_GLOB_TIMER2	39
#define IRQ_AURORA_GLOB_TIMER3	40

#define IRQ_AURORA_UART0	41
#define IRQ_AURORA_UART1	42
#define IRQ_AURORA_UART2	43
#define IRQ_AURORA_UART3	44

#define IRQ_AURORA_USB0		45
#define IRQ_AURORA_USB1		46
#define IRQ_AURORA_USB2		47

#define IRQ_AURORA_CRYPTO(chan)	((chan == 0) ? 48 : 49)

#define IRQ_AURORA_RTC		50

#define IRQ_AURORA_XOR00	51
#define IRQ_AURORA_XOR01	52

#define IRQ_AURORA_BM		53
#define IRQ_AURORA_SDIO		54
#define IRQ_AURORA_SATA0	55
#define IRQ_AURORA_TDM		56
#define IRQ_AURORA_SATA1	57
	
#define IRQ_AURORA_PCIE0	58
#define IRQ_AURORA_PCIE1	59
#define IRQ_AURORA_PCIE2	60
#define IRQ_AURORA_PCIE3	61
#define IRQ_AURORA_PCIE4	62
#define IRQ_AURORA_PCIE5	63
#define IRQ_AURORA_PCI0		63	/* FPGA only */
#define IRQ_AURORA_PCIE6	64
#define IRQ_AURORA_PCIE7	65

#define IRQ_AURORA_GBE0		66
#define IRQ_AURORA_GBE0_RX	67
#define IRQ_AURORA_GBE0_TX	68
#define IRQ_AURORA_GBE0_MISC	69
#define IRQ_AURORA_GBE1		70
#define IRQ_AURORA_GBE1_RX	71
#define IRQ_AURORA_GBE1_TX	72
#define IRQ_AURORA_GBE1_MISC	73
#define IRQ_AURORA_GBE2		74
#define IRQ_AURORA_GBE2_RX	75
#define IRQ_AURORA_GBE2_TX	76
#define IRQ_AURORA_GBE2_MISC	77
#define IRQ_AURORA_GBE3		78
#define IRQ_AURORA_GBE3_RX	79
#define IRQ_AURORA_GBE3_TX	80
#define IRQ_AURORA_GBE3_MISC	81

#define IRQ_AURORA_GPIO_0_7	82
#define IRQ_AURORA_GPIO_8_15	83
#define IRQ_AURORA_GPIO_16_23	84
#define IRQ_AURORA_GPIO_24_31	85
#define IRQ_AURORA_GPIO_32_39	87
#define IRQ_AURORA_GPIO_40_47	88
#define IRQ_AURORA_GPIO_48_55	89
#define IRQ_AURORA_GPIO_56_63	90
#define IRQ_AURORA_GPIO_64_66	91

#define IRQ_AURORA_XOR10	94
#define IRQ_AURORA_XOR11	95

#define IRQ_AURORA_SHARE_INB_DB0	96
#define IRQ_AURORA_SHARE_INB_DB1	97
#define IRQ_AURORA_SHARE_INB_DB2	98

#define IRQ_AURORA_PCIE8	99
#define IRQ_AURORA_PCIE9	103

#define IRQ_AURORA_PMU		107

#define IRQ_AURORA_DRAM		108

#define IRQ_AURORA_NET_WKUP0	109
#define IRQ_AURORA_NET_WKUP1	110
#define IRQ_AURORA_NET_WKUP2	111
#define IRQ_AURORA_NET_WKUP3	112

#define IRQ_AURORA_NFC		113

#define IRQ_AURORA_MTL_FIX	114

#define IRQ_AURORA_OVRCL	115

#define IRQ_MAIN_INTS_NUM	116

#define MAX_PER_CPU_IRQ_NUMBER  28
/*
 * AURORA General Purpose Pins
 */
#define IRQ_AURORA_GPIO_START		128
#define NR_GPIO_IRQS			67

/*
 * AURORA MSI interrupts
 */
#define NR_PRIVATE_MSI_GROUP		16
#define NR_PRIVATE_MSI_IRQS		NR_PRIVATE_MSI_GROUP
#define NR_MSI_IRQS			NR_PRIVATE_MSI_IRQS
#define IRQ_AURORA_MSI_START		(IRQ_AURORA_GPIO_START + NR_GPIO_IRQS)
#define NR_IRQS				(IRQ_AURORA_GPIO_START + NR_GPIO_IRQS + NR_MSI_IRQS)
#define GPP_IRQ_TYPE_LEVEL		0
#define GPP_IRQ_TYPE_CHANGE_LEVEL	1

/*
 * Aurora Error interrupts
 */

#define INT_ERR_CESA0         		0
#define INT_ERR_DEVBUS         		1

/*
 * IRQ HAL remapping
 */
#define NET_TH_RXTX_IRQ_NUM(x)		(IRQ_AURORA_GBE0_FIC + ((x) * 2))
#define IRQ_GPP_START			IRQ_AURORA_GPIO_START
#define SATA_IRQ_NUM			(IRQ_AURORA_SATA0)
#define CESA_IRQ(chan)			IRQ_AURORA_CRYPTO(chan)
#endif
