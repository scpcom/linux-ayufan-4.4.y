/*
 *  linux/drivers/spi/busses/comcerto_spi.h
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

#include <linux/list.h>
#include <linux/spi/spi.h>
//#include <asm-arm/types.h>
#include <linux/types.h>

#define comcerto_SPI_NAME "comcerto_spi"

#define CFG_HZ_CLOCK            165000000       /* 165 MHz */
#define SPI_BASEADDR            COMCERTO_APB_SPI_BASE

#define SPI_BAUDR_MIN           2
#define SPI_BAUDR_MAX           0xFFFE
#define SPI_SPEED_MAX           (4*1000*1000)
#define SPI_SPEED_MIN           0 /*need to be calculated using c2k pll bus clock*/
#define SPI_FRAME_SIZE_MIN      4
#define SPI_FRAME_SIZE_MAX      16
#define SPI_CHIP_SELECT_MAX     15

struct comcerto_spi_platform {
    int     *chipselect;
    int   num_chipselect;
    u8  bits_per_word;
    u8 num_devices;
    unsigned long min_sc_rate;      /* maximum supported serial clock rate (in MHz) */
    unsigned long max_sc_rate;      /* minimum supported serial clock rate (in MHz) */

    u8 max_fs;                      /* maximum supported frame size (in bits) */
    u8 min_fs;                      /* minimum supported frame size (in bits) */

    u16 max_nframe;                 /* maximum supported transfer frame number */
    u16 min_nframe;                 /* minimum supported transfer frame number */

    u16 cs_msk;
    unsigned long clock_rate;

    struct spi_board_info *devices;
};

struct comcerto_transfer
{
    u8 *wbuf;
    unsigned int wlen;

    u8 *rbuf;
    unsigned int rlen;

    u8 mode;

    u32 ctrlr0;

    u8 bits_per_word;
    u32 baudr;
    u32 ser;

    u8 fs;                          /* transfer frame size (in bits) */
};

struct spi_client_conf
{
    u16 cs_msk;                     /* chip select mask for this client */
    u8 sc_polarity;                 /* serial clock polarity */
    u8 sc_phase;                    /* serial clock phase */
    unsigned long sc_rate;          /* serial clock rate (in MHz)*/
    u8 cs_delay;                    /* chip select deassert time (in serial clock cycles) */
};

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


#define SPI_TRANSFER_MODE_WRITE_ONLY	0x01
#define SPI_TRANSFER_MODE_READ_ONLY	0x02
#define SPI_TRANSFER_MODE_WRITE_READ	0x03

typedef u8	u4;

struct spi_local_transfer
{
	u8 *wbuf;
	unsigned int wlen;

	u8 *rbuf;
	unsigned int rlen;

	u8 mode;

	u8 fs;				/* transfer frame size (in bits) */
};

struct spi_adapter_caps
{
	unsigned long min_sc_rate;	/* maximum supported serial clock rate (in MHz) */
	unsigned long max_sc_rate;	/* minimum supported serial clock rate (in MHz) */

	u8 max_fs;			/* maximum supported frame size (in bits) */
	u8 min_fs;			/* minimum supported frame size (in bits) */

	u16 cs_msk;			/* mask of supported chip selects */

	u16 max_nframe;			/* maximum supported transfer frame number */
	u16 min_nframe;			/* minimum supported transfer frame number */
};

struct spi_client_config
{
	u16 cs_msk;			/* chip select mask for this client */
	u8 sc_polarity;			/* serial clock polarity */
	u8 sc_phase;			/* serial clock phase */
	unsigned long sc_rate;		/* serial clock rate (in MHz)*/
	u8 cs_delay;			/* chip select deassert time (in serial clock cycles) */
};

/* A SPI bus adapter instance */
struct spi_adapter
{
	char *name;

	int (*do_transfer)(struct spi_adapter *adapter, struct spi_local_transfer *transfer, struct spi_client_config *config);

	void *data;
	unsigned long membase;
	unsigned long clock_rate;	

	struct list_head clients;

	struct spi_adapter_caps caps;

	struct spi_master       master;
};

/* A SPI device instance */
struct spi_client
{
	struct spi_client_config config;
	struct spi_adapter *adapter;
	void *data;
	struct list_head list;
};

//int spi_writeread_mem(struct spi_client *client, u8 fs, u8 *rbuffer, int rlen, u8 *wbuffer, int wlen);
int spi_writeread_mem(struct spi_adapter *adapter, u8 fs, u8 *rbuffer, int rlen, u8 *wbuffer, int wlen);


#endif /* _COMCERTO_SPI_H */
