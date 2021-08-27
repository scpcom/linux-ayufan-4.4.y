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

#define IRQ_AURORA_SPI          30
#define IRQ_AURORA_I2C0		31
#define IRQ_AURORA_I2C1		32

#define IRQ_AURORA_GLOB_TIMER0	37
#define IRQ_AURORA_GLOB_TIMER1	38
#define IRQ_AURORA_GLOB_TIMER2	39
#define IRQ_AURORA_GLOB_TIMER3	40

#define IRQ_AURORA_UART0	41
#define IRQ_AURORA_UART1	42

#define IRQ_AURORA_USB0		45
#define IRQ_AURORA_USB1		46
#define IRQ_AURORA_USB(x)	(45 + x)

#define IRQ_AURORA_CRYPTO(chan)	48

#define IRQ_AURORA_RTC		50

#define IRQ_AURORA_XOR0_CH0	51
#define IRQ_AURORA_XOR0_CH1	52

#define IRQ_AURORA_SDIO		54
#define IRQ_AURORA_SATA0	55
#define IRQ_AURORA_TDM		56
#define IRQ_AURORA_SATA1	57
#define IRQ_AURORA_PCIE0	58

#define IRQ_AURORA_PCIE1	62

#define IRQ_AURORA_GBE0		66
#define IRQ_AURORA_GBE0_RX	67
#define IRQ_AURORA_GBE0_TX	68
#define IRQ_AURORA_GBE0_MISC	69
#define IRQ_AURORA_GBE1		70
#define IRQ_AURORA_GBE1_RX	71
#define IRQ_AURORA_GBE1_TX	72
#define IRQ_AURORA_GBE1_MISC	73

#define IRQ_AURORA_GPIO_0_7	82
#define IRQ_AURORA_GPIO_8_15	83
#define IRQ_AURORA_GPIO_16_23	84
#define IRQ_AURORA_GPIO_24_31	85
#define IRQ_AURORA_GPIO_32_39	87
#define IRQ_AURORA_GPIO_40_47	88
#define IRQ_AURORA_GPIO_48_55	89
#define IRQ_AURORA_GPIO_56_63	90
#define IRQ_AURORA_GPIO_64_66	91

#define IRQ_AURORA_AUDIO	93

#define IRQ_AURORA_XOR1_CH0	94
#define IRQ_AURORA_XOR1_CH1	95

#define IRQ_AURORA_OUTB_DB0	96
#define IRQ_AURORA_OUTB_DB1	97
#define IRQ_AURORA_OUTB_DB2	98

#define IRQ_AURORA_DRAM		108
#define IRQ_AURORA_NET_WKUP0	109
#define IRQ_AURORA_NET_WKUP1	110

#define IRQ_AURORA_NFC		113
#define IRQ_AURORA_MTL_FIX	114

#define IRQ_MAIN_INTS_NUM	115

/*
 * AURORA General Purpose Pins
 */
#define IRQ_AURORA_GPIO_START		128
#define NR_GPIO_IRQS			96 /* only 67 irqs are valid ,but just to be aligned */

#define GPP_IRQ_TYPE_LEVEL		0
#define GPP_IRQ_TYPE_CHANGE_LEVEL	1

/*
 * Aurora Error interrupts
 */

#define MV_SOC_MAIN_INT_ERR_MASK_REG	0x218C0
#define MV_SOC_MAIN_INT_ERR_CAUSE_REG	0x20A20

#define IRQ_AURORA_ERR_START		224
#define NR_SOC_MAIN_ERR_IRQS		32

#define INT_ERR_CESA0         		0
#define INT_ERR_DEVBUS         		1
#define INT_ERR_PCIE(unit)		((unit == 0) ? 4 : 5)

/*
 * IRQ HAL remapping
 */
#define NET_TH_RXTX_IRQ_NUM(x)		(IRQ_AURORA_GBE0_FIC + ((x) * 2))
#define SATA_IRQ_NUM			(IRQ_AURORA_SATA0)
#define CESA_IRQ(chan)			IRQ_AURORA_CRYPTO(chan)
#define IRQ_GPP_START			IRQ_AURORA_GPIO_START
#define IRQ_AURORA_SATA(x)		((x == 0) ? IRQ_AURORA_SATA0 : IRQ_AURORA_SATA1)

#define MV_PCI_MASK_REG(unit)		((unit == 0) ? 0x41910 : 0x81910)
#define MV_PCI_IRQ_CAUSE_REG(unit)    	((unit == 0) ? 0x41900 : 0x81900)
#define MV_PCI_MASK_ABCD		(BIT24 | BIT25 | BIT26 | BIT27 )

/* Description for bit from PCI Express Interrupt Mask Register
** BIT3 - Erroneous Write Attempt to Internal Register
** BIT4 - Hit Default Window Error
** BIT6 and BIT7 -Rx and Tx RAM Parity Error
** BIT9 and BIT10 - Non Fatal and Fatal Error Detected
** BIT14 - Flow Control Protocol Error
** BIT23 - Link Failure Indication
*/
#define MV_PCI_MASK_ERR                        (BIT3 | BIT4 | BIT6 | BIT7 | BIT9 | BIT10 | BIT14 | BIT23)

#define NR_IRQS			(IRQ_AURORA_ERR_START + NR_SOC_MAIN_ERR_IRQS)

/* Interrupt Macros for backward compatibility */
#define IRQ_USB_CTRL(x)			((x == 0) ? IRQ_AURORA_USB0 : IRQ_AURORA_USB1)

#endif
