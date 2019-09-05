/*
 *  linux/include/asm-arm/arch-comcerto/comcerto1000/spi.h
 *
 *  Copyright (C) 2004,2008 Mindspeed Technologies, Inc.
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
#ifndef __ASM_ARCH_COMCERTO1000_SPI_H
#define __ASM_ARCH_COMCERTO1000_SPI_H

#define COMCERTO_SPI_FIFO_DEPTH		8

/* SPI core registers */
#define COMCERTO_SPI_CTRLR0		0x00
#define COMCERTO_SPI_CTRLR1		0x04
#define COMCERTO_SPI_SSIENR		0x08
#define COMCERTO_SPI_MWCR		0x0C
#define COMCERTO_SPI_SER		0x10
#define COMCERTO_SPI_BAUDR		0x14
#define COMCERTO_SPI_TXFTLR		0x18
#define COMCERTO_SPI_RXFTLR		0x1C
#define COMCERTO_SPI_TXFLR		0x20
#define COMCERTO_SPI_RXFLR		0x24
#define COMCERTO_SPI_SR			0x28
#define COMCERTO_SPI_IMR		0x2C
#define COMCERTO_SPI_ISR		0x30
#define COMCERTO_SPI_RISR		0x34
#define COMCERTO_SPI_TXOICR		0x38
#define COMCERTO_SPI_RXOICR		0x3C
#define COMCERTO_SPI_RXUICR		0x40
#define COMCERTO_SPI_MSTICR		0x44
#define COMCERTO_SPI_ICR		0x48
#define COMCERTO_SPI_IDR		0x58
#define COMCERTO_SPI_DR			0x60

/* CTRLR0 - control register 0 bits/masks (incomplete) */
#define SPI_CTRLR0_DFS_MASK		(15<<0)	/* Data frame size mask */
#define SPI_CTRLR0_SCPOL		(1<<7)	/* Serial clock polarity */
#define SPI_CTRLR0_SCPH			(1<<6)	/* Serial clock phase */
#define SPI_CTRLR0_SRL			(1<<11)	/* Shift register loop */

/* SR - status register bits */
#define SPI_SR_BUSY			(1<<0)	/* SSI busy flag, serial transfer in progress */
#define SPI_SR_TFNF			(1<<1)	/* Transmit FIFO not full */
#define SPI_SR_TFE			(1<<2)	/* Transmit FIFO empty */
#define SPI_SR_RFNE			(1<<3)	/* Receive FIFO not empty */
#define SPI_SR_RFF			(1<<4)	/* Receive FIFO full */
#define SPI_SR_TXE			(1<<5)	/* Transmission error */
#define SPI_SR_DCOL			(1<<6)	/* Data collision error */

/* IMR, ISR, RISR - interrupt mask/status bits */
#define SPI_INT_TXEI			(1<<0)	/* Transmit FIFO empty interrupt status */
#define SPI_INT_TXOI			(1<<1)	/* Transmit FIFO overflow interrupt status */
#define SPI_INT_RXUI			(1<<2)	/* Receive FIFO underflow interrupt status */
#define SPI_INT_RXOI			(1<<3)	/* Receive FIFO overflow interrupt status */
#define SPI_INT_RXFI			(1<<4)	/* Receive FIFO full interrupt status */
#define SPI_INT_MSTI			(1<<5)	/* Multi-Master contention interrupt status */

#endif /* __ASM_ARCH_COMCERTO1000_SPI_H */
