/*
 *  arch/arm/arch-comcerto/include/mach/comcerto-2000/irqs.h
 *
 *  Copyright (C) 2011 Mindspeed Technologies, Inc.
 *
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
#ifndef __COMCERTO_IRQS_H__
#define __COMCERTO_IRQS_H__

#define FIQ_START		(0)

#define SGI_IRQ(x)		(0 + (x))	/* ID0  - ID15 */
#define PPI_IRQ(x)		(27 + (x))	/* ID27 - ID31 */
#define LSPI_IRQ(x)		(32 + (x))	/* ID32 - ID62 */
#define SPI_IRQ(x)		(63 + (x))      /* ID63 - ID1020 */

#define IRQ_LOCALTIMER		PPI_IRQ(2)
#define IRQ_LOCALWDOG		PPI_IRQ(3)

#define IRQ_PTP0		LSPI_IRQ(0)
#define IRQ_PTP1		LSPI_IRQ(1)
#define IRQ_PTP2		LSPI_IRQ(2)
#define IRQ_PTP3		LSPI_IRQ(3)
#define IRQ_PFE_HIF		LSPI_IRQ(4)
#define IRQ_PFE_HIFNCPY		LSPI_IRQ(5)
#define IRQ_PFE_GPT		LSPI_IRQ(6)
#define IRQ_FABRIC		LSPI_IRQ(7)
#define IRQ_A9_L1_PAR		LSPI_IRQ(8)
#define IRQ_A9_PMU0		LSPI_IRQ(9)
#define IRQ_A9_PMU1		LSPI_IRQ(10)
#define IRQ_L2			LSPI_IRQ(11)
#define IRQ_EAPE		LSPI_IRQ(12)
#define IRQ_SPACC		LSPI_IRQ(13)
#define IRQ_CIE			LSPI_IRQ(14)
#define IRQ_DEC			LSPI_IRQ(15)
#define IRQ_SATA		LSPI_IRQ(16)
#define IRQ_CSS0		LSPI_IRQ(17)
#define IRQ_CSS1		LSPI_IRQ(18)
#define IRQ_CSS2		LSPI_IRQ(19)
#define IRQ_CSS3		LSPI_IRQ(20)
#define IRQ_USB2		LSPI_IRQ(21)
#define IRQ_USB3		LSPI_IRQ(22)
#define IRQ_SATA_MSI		LSPI_IRQ(23)
#define IRQ_CSS			LSPI_IRQ(24)
#define IRQ_DMAC		LSPI_IRQ(25)
#define IRQ_UART0		LSPI_IRQ(26)
#define IRQ_UART1		LSPI_IRQ(27)
#define IRQ_UART_S2		LSPI_IRQ(27)
#define IRQ_SPI			LSPI_IRQ(28)
#define IRQ_SPI_LS		LSPI_IRQ(29)
#define IRQ_I2C			LSPI_IRQ(30)

#define IRQ_PFE_UPE		SPI_IRQ(0)
#define IRQ_DDRC		SPI_IRQ(1)
#define IRQ_TDMA_TX		SPI_IRQ(2)
#define IRQ_TDMA_RX		SPI_IRQ(3)
#define IRQ_TDMA_AHBERR		SPI_IRQ(4)
#define IRQ_PFE_UPE_TIMER	SPI_IRQ(5)
#define IRQ_TDMA		SPI_IRQ(6)
#define IRQ_SLIC		SPI_IRQ(7)
#define IRQ_MDMA_M2IO		SPI_IRQ(8)
#define IRQ_MDMA_IO2M		SPI_IRQ(9)
#define IRQ_MDMA_AXIW		SPI_IRQ(10)
#define IRQ_MDMA_AXIR		SPI_IRQ(11)
#define IRQ_PCIe0		SPI_IRQ(12)
#define IRQ_PCIe1		SPI_IRQ(13)
#define IRQ_G0			SPI_IRQ(14)
#define IRQ_G1			SPI_IRQ(15)
#define IRQ_G2			SPI_IRQ(16)
#define IRQ_G3			SPI_IRQ(17)
#define IRQ_G4			SPI_IRQ(18)
#define IRQ_G5			SPI_IRQ(19)
#define IRQ_G6			SPI_IRQ(20)
#define IRQ_G7			SPI_IRQ(21)
#define IRQ_WOL			SPI_IRQ(22)
#define IRQ_USB3_PME		SPI_IRQ(23)
#define IRQ_TIMER0		SPI_IRQ(24)
#define IRQ_TIMER1		SPI_IRQ(25)
#define IRQ_TIMER2		SPI_IRQ(26)
#define IRQ_TIMER3		SPI_IRQ(27)
#define IRQ_TIMER4		SPI_IRQ(28)
#define IRQ_TIMER5		SPI_IRQ(29)
#define IRQ_TIMER6		SPI_IRQ(30)
#define IRQ_RTC_ALM		SPI_IRQ(31)
#define IRQ_RTC_PRI		SPI_IRQ(32)


/* Software decoded interrupts used by PCIE */
#define COMCERTO_IRQ_MAX	SPI_IRQ(33)	

/* PCIE MSI virtual irqs */
#define PCIE_NUM_MSI_IRQS	32
#define PCIE0_MSI_INT_BASE	(COMCERTO_IRQ_MAX + 0)
#define PCIE0_MSI_INT_END	(PCIE0_MSI_INT_BASE + PCIE_NUM_MSI_IRQS)
#define PCIE1_MSI_INT_BASE	(PCIE0_MSI_INT_END + 0)
#define PCIE1_MSI_INT_END	(PCIE1_MSI_INT_BASE + PCIE_NUM_MSI_IRQS)

#define PCIE_NUM_INTX_IRQS	4
#define PCIE0_INTX_BASE		(PCIE1_MSI_INT_END + 0)
#define PCIE0_INTX_END		(PCIE0_INTX_BASE + PCIE_NUM_INTX_IRQS)
#define PCIE1_INTX_BASE		(PCIE0_INTX_END + 0)
#define PCIE1_INTX_END		(PCIE1_INTX_BASE + PCIE_NUM_INTX_IRQS)

#define VIRQ_END		PCIE1_INTX_END
#define NR_IRQS			256

#endif

