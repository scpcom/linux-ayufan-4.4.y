/*
 *  linux/include/linux/spi.h
 *
 *  Copyright (C) Mindspeed Technologies
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _SPI_H
#define _SPI_H

#include <linux/version.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/device.h>
#endif

#define SPI_TRANSFER_MODE_WRITE_ONLY	0x01
#define SPI_TRANSFER_MODE_READ_ONLY	0x02
#define SPI_TRANSFER_MODE_WRITE_READ	0x03

typedef u8	u4;

struct spi_transfer
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
	unsigned long ba_delay; 	/* bus access delay, in us */
};

/* A SPI bus adapter instance */
struct spi_adapter
{
	char *name;

	int (*do_transfer)(struct spi_adapter *adapter, struct spi_transfer *transfer, struct spi_client_config *config);

	void *data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	struct device dev;
#endif
	struct list_head list;
	struct list_head clients;

	struct spi_adapter_caps caps;

	int bus_num;

	spinlock_t lock;
};

/* A SPI device instance */
struct spi_client
{
	struct spi_client_config config;

	struct spi_driver *driver;

	struct spi_adapter *adapter;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	struct device dev;
#else
	void *data;
#endif
	struct list_head list;
};

/* A SPI device driver */
struct spi_driver {
	char *name;

	int (*attach_adapter)(struct spi_adapter *adapter);
	int (*detach_client)(struct spi_client *client);
#ifdef CONFIG_PM
	int (*suspend)(struct spi_client *client, pm_message_t state);
	int (*resume)(struct spi_client *client);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	struct device_driver driver;
#endif
	struct list_head list;
	short bus_id;
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static inline void *spi_get_clientdata (struct spi_client *dev)
{
	return dev_get_drvdata (&dev->dev);
}

static inline void spi_set_clientdata (struct spi_client *dev, void *data)
{
	dev_set_drvdata (&dev->dev, data); 
}
#else
static inline void *spi_get_clientdata (struct spi_client *dev)
{
	return (void *) dev->data;
}

static inline void spi_set_clientdata (struct spi_client *dev, void *data)
{
	dev->data = data;
}
#endif

int spi_add_adapter(struct spi_adapter *adapter);
int spi_del_adapter(struct spi_adapter *adapter);

int spi_add_driver(struct spi_driver *driver);
int spi_del_driver(struct spi_driver *driver);

int spi_attach_client(struct spi_client *client);
int spi_detach_client(struct spi_client *client);

int spi_write_mem(struct spi_client *client, u8 fs, u8 *buffer, int len);
int spi_write_single(struct spi_client *client, u8 fs, u16 value);

int spi_writen(struct spi_client *client, u4 value);
int spi_writeb(struct spi_client *client, u8 value);
int spi_writew(struct spi_client *client, u16 value);

int spi_read_mem(struct spi_client *client, u8 fs, u8 *buffer, int len);
int spi_read_single(struct spi_client *client, u8 fs, u16 *value);
int spi_writeread_mem(struct spi_client *client, u8 fs, u8 *rbuffer, int rlen, u8 *wbuffer, int wlen);

int spi_readn(struct spi_client *client, u4 *value);
int spi_readb(struct spi_client *client, u8 *value);
int spi_readw(struct spi_client *client, u16 *value);

#endif /* _SPI_H */
