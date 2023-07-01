/*
 *  linux/drivers/spi2/busses/comcerto_spi.h
 *
 *  Copyright (C) Mindspeed Technologies
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _COMCERTO_SPI_H
#define _COMCERTO_SPI_H

#include <linux/version.h>
#include <linux/spi2/spi.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/ioport.h>

struct platform_device {
	char            *name;
	u32             id;
	u32             num_resources;
	struct resource *resource;

	unsigned long data;
};
#endif

#define COMCERTO_SPI_DRIVER_NAME	"Comcerto SPI"


#define COMCERTO_SPI_CTRLR0               0x00
#define COMCERTO_SPI_CTRLR1               0x04
#define COMCERTO_SPI_SSIENR               0x08
#define COMCERTO_SPI_MWCR                 0x0C
#define COMCERTO_SPI_SER                  0x10
#define COMCERTO_SPI_BAUDR                0x14
#define COMCERTO_SPI_TXFTLR               0x18
#define COMCERTO_SPI_RXFTLR               0x1C
#define COMCERTO_SPI_TXFLR                0x20
#define COMCERTO_SPI_RXFLR                0x24
#define COMCERTO_SPI_SR                   0x28
#define COMCERTO_SPI_IMR                  0x2C
#define COMCERTO_SPI_ISR                  0x30
#define COMCERTO_SPI_RISR                 0x34
#define COMCERTO_SPI_TXOICR               0x38
#define COMCERTO_SPI_RXOICR               0x3C
#define COMCERTO_SPI_RXUICR               0x40
#define COMCERTO_SPI_MSTICR               0x44
#define COMCERTO_SPI_ICR                  0x48
#define COMCERTO_SPI_IDR                  0x58
#define COMCERTO_SPI_DR                   0x60


/* SR - status register bits */
#define BUSY		(1<<0)	/* SSI busy flag, serial transfer in progress */
#define TFNF		(1<<1)	/* Transmit FIFO not full */
#define TFE		(1<<2)	/* Transmit FIFO empty */
#define RFNE		(1<<3)	/* Receive FIFO not empty */
#define RFF		(1<<4)	/* Receive FIFO full */
#define TXE		(1<<5)	/* Transmission error */
#define DCOL		(1<<6)	/* Data collision error */

/* Interrupt status after being masked */
#define TXEIS		(1<<0)	/* Transmit FIFO empty interrupt status */
#define TXOIS		(1<<1)	/* Transmit FIFO overflow interrupt status */
#define RXUIS		(1<<2)	/* Receive FIFO underflow interrupt status */
#define RXOIS		(1<<3)	/* Receive FIFO overflow interrupt status */
#define RXFIS		(1<<4)	/* Receive FIFO full interrupt status */
#define MSTIS		(1<<5)	/* Multi-Master contention interrupt status */

/* Interrupt status before being masked */
#define TXEIR		(1<<0)	/* Transmit FIFO empty interrupt status */
#define TXOIR		(1<<1)	/* Transmit FIFO overflow interrupt status */
#define RXUIR		(1<<2)	/* Receive FIFO underflow interrupt status */
#define RXOIR		(1<<3)	/* Receive FIFO overflow interrupt status */
#define RXFIR		(1<<4)	/* Receive FIFO full interrupt status */
#define MSTIR		(1<<5)	/* Multi-Master contention interrupt status */


/* Interrupt mask register */
#define TXEIM		(1<<0)	/* Transmit FIFO empty interrupt status */
#define TXOIM		(1<<1)	/* Transmit FIFO overflow interrupt status */
#define RXUIM		(1<<2)	/* Receive FIFO underflow interrupt status */
#define RXOIM		(1<<3)	/* Receive FIFO overflow interrupt status */
#define RXFIM		(1<<4)	/* Receive FIFO full interrupt status */
#define MSTIM		(1<<5)	/* Multi-Master contention interrupt status */

struct comcerto_spi
{
	struct spi_adapter *adapter;
	unsigned long membase;
	int irq;
	unsigned long clock_rate;
};


#endif /* _COMCERTO_SPI_H */
