/*
 *  linux/drivers/spi2/spi-core.c
 *
 *  Copyright (C) 2006 Mindspeed Technologies, Inc.
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
 

#if !defined (AUTOCONF_INCLUDED)
#if 0
	#include <linux/config.h>
#endif
#endif
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/delay.h>

#include <linux/spi2/spi.h>

static LIST_HEAD(spi_adapters);
static LIST_HEAD(spi_drivers);

/**
 * spi_write_mem -
 *
 *
 */
int spi_write_mem(struct spi_client *client, u8 fs, u8 *buffer, int len)
{
	struct spi_transfer transfer;
	struct spi_adapter *adapter = client->adapter;
	unsigned long flags;
	int rc;

	memset(&transfer, 0, sizeof (struct spi_transfer));

	transfer.fs = fs;
	transfer.mode = SPI_TRANSFER_MODE_WRITE_ONLY;
	transfer.wbuf = buffer;
	transfer.wlen = len;

	spin_lock_irqsave(&adapter->lock, flags);
	rc = adapter->do_transfer(adapter, &transfer, &client->config);
	spin_unlock_irqrestore(&adapter->lock, flags);

	/* deassert the chip select at least for this long */
	udelay (1 + ((1000000 * client->config.cs_delay) / client->config.sc_rate));

	return rc;
}


/**
 * spi_write_single -
 *
 *
 */
int spi_write_single(struct spi_client *client, u8 fs, u16 value)
{
	return spi_write_mem(client, fs, (u8 *)&value, 1);
}

/**
 * spi_writen -
 *
 *
 */
int spi_writen(struct spi_client *client, u4 value)
{
	return spi_write_mem(client, 4, (u8 *)&value, 1);
}


/**
 * spi_writeb -
 *
 *
 */
int spi_writeb(struct spi_client *client, u8 value)
{
	return spi_write_mem(client, 8, &value, 1);
}


/**
 * spi_writew -
 *
 *
 */
int spi_writew(struct spi_client *client, u16 value)
{
	return spi_write_mem(client, 16, (u8 *)&value, 1);
}

/**
 * spi_read_mem -
 *
 *
 */
int spi_read_mem(struct spi_client *client, u8 fs, u8 *buffer, int len)
{
	struct spi_transfer transfer;
	struct spi_adapter *adapter = client->adapter;
	unsigned long flags;
	int rc;

	memset(&transfer, 0, sizeof (struct spi_transfer));

	transfer.fs = fs;
	transfer.mode = SPI_TRANSFER_MODE_READ_ONLY;
	transfer.rbuf = buffer;
	transfer.rlen = len;

	spin_lock_irqsave(&adapter->lock, flags);
	rc = adapter->do_transfer(adapter, &transfer, &client->config);
	spin_unlock_irqrestore(&adapter->lock, flags);

	/* deassert the chip select at least for this long */
	udelay (1 + ((1000000 * client->config.cs_delay) / client->config.sc_rate));

	return rc;
}


/**
 * spi_read_single -
 *
 *
 */
int spi_read_single(struct spi_client *client, u8 fs, u16 *value)
{
	return spi_read_mem(client, fs, (u8 *) value, 1);
}

/**
 * spi_readn -
 *
 *
 */
int spi_readn(struct spi_client *client, u4 *value)
{
	return spi_read_mem(client, 4, (u8 *)value, 1);
}


/**
 * spi_readb -
 *
 *
 */
int spi_readb(struct spi_client *client, u8 *value)
{
	return spi_read_mem(client, 8, value, 1);
}


/**
 * spi_readw -
 *
 *
 */
int spi_readw(struct spi_client *client, u16 *value)
{
	return spi_read_mem(client, 16, (u8 *)value, 1);
}


/**
 * spi_read_mem -
 *
 *
 */
int spi_writeread_mem(struct spi_client *client, u8 fs, u8 *rbuffer, int rlen, u8 *wbuffer, int wlen)
{
	struct spi_transfer transfer;
	struct spi_adapter *adapter = client->adapter;
	unsigned long flags;
	int rc;

	memset(&transfer, 0, sizeof (struct spi_transfer));

	transfer.fs = fs;
	transfer.mode = SPI_TRANSFER_MODE_WRITE_READ;
	transfer.rbuf = rbuffer;
	transfer.rlen = rlen;
	transfer.wbuf = wbuffer;
	transfer.wlen = wlen;

	spin_lock_irqsave(&adapter->lock, flags);
	rc = adapter->do_transfer(adapter, &transfer, &client->config);
	spin_unlock_irqrestore(&adapter->lock, flags);

	/* deassert the chip select at least for this long */
	udelay (1 + ((1000000 * client->config.cs_delay) / client->config.sc_rate));

	return rc;
}
/**
 * spi_add_adapter -
 *
 *
 */
int spi_add_adapter(struct spi_adapter *adapter)
{
	struct spi_driver *driver;
	struct list_head *item;

	printk(KERN_INFO "SPI core: add adapter %s\n", adapter->name);

#ifdef _USE_DRIVER_MODEL_
	sprintf(adapter->dev.bus_id, "spi-%d", adapter->nr);
	adapter->dev.driver = &spi_adapter_driver;
	adapter->dev.release = &spi_adapter_dev_release;
	device_register(&adapter->dev);
#endif

	list_add(&adapter->list, &spi_adapters);
	INIT_LIST_HEAD(&adapter->clients);

	//adapter->lock = SPIN_LOCK_UNLOCKED;
	//Above method is obsolete and new is used below.
	spin_lock_init(&adapter->lock); 

	list_for_each(item, &spi_drivers) {
		driver = list_entry(item, struct spi_driver, list);
		printk ("%s: Matching: adapter->bus_num=0x%x driver->bus_id 0x%x\n", __func__,\
				adapter->bus_num, driver->bus_id);
		if (driver->bus_id == adapter->bus_num)
		{
			printk ("%s: Found adapter 0x%x \n", __func__, adapter->bus_num);
			/* We ignore the return code; if it fails, too bad */
			driver->attach_adapter(adapter);
		}
	}

	return 0;
}

/**
 * spi_del_adapter -
 *
 *
 */
int spi_del_adapter(struct spi_adapter *adapter)
{
	struct spi_client *client;
	struct list_head *item, *_n;

	printk(KERN_INFO "SPI core: del adapter %s\n", adapter->name);

	list_for_each_safe(item, _n, &adapter->clients) {
		client = list_entry(item, struct spi_client, list);

		if (client->driver->detach_client(client))
			goto out;
	}

	list_del(&adapter->list);

#ifdef _USE_DRIVER_MODEL_
	device_unregister(&adapter->dev);
#endif

      out:
	return 0;
}

/**
 * spi_attach_client -
 *
 *
 */
int spi_attach_client(struct spi_client *client)
{
	struct spi_adapter *adapter = client->adapter;
	struct spi_client_config *config = &client->config;
	struct spi_adapter_caps *caps = &adapter->caps;

	printk(KERN_INFO "SPI core: attach client to adapter %s\n", client->adapter->name);

	if ((config->sc_rate >= caps->max_sc_rate) || (config->sc_rate < caps->min_sc_rate)) {
		printk(KERN_INFO "SPI core: client serial clock rate %ld out of range [%ld, %ld]", config->sc_rate,
		       caps->min_sc_rate, caps->max_sc_rate);

		goto err;
	}

	if (config->cs_msk & ~caps->cs_msk) {
		printk(KERN_INFO "SPI core: client cs mask %#x not supported %#x", config->cs_msk,
		       caps->cs_msk);

		goto err;
	}

#ifdef _USE_DRIVER_MODEL_
	device_register(&client->device);
#endif
	list_add(&client->list, &adapter->clients);

	return 0;

      err:
	return -1;
}

/**
 * spi_dettach_client -
 *
 *
 */
int spi_detach_client(struct spi_client *client)
{
	printk(KERN_INFO "SPI core: client detach from adapter %s\n", client->adapter->name);

	list_del(&client->list);

#ifdef _USE_DRIVER_MODEL_
	device_unregister(&client->dev);
#endif
	return 0;
}

/**
 * spi_add_driver -
 *
 *
 */
int spi_add_driver(struct spi_driver *driver)
{
	struct spi_adapter *adapter;
	struct list_head *item;

	printk(KERN_INFO "SPI core: add driver %s\n", driver->name);

#ifdef _USE_DRIVER_MODEL_
	driver->driver.name = driver->name;
	driver->driver.bus = &spi2_bus_type;
	driver->driver.probe = spi_device_probe;
	driver->driver.remove = spi_device_remove;

	if (driver_register(&driver->driver))
		goto err;
#endif
	list_add(&driver->list, &spi_drivers);

	list_for_each(item, &spi_adapters) {
		adapter = list_entry(item, struct spi_adapter, list);
		printk ("%s: Checking: adapter bus_num=0x%x driver bus_id=0x%x\n",\
				__func__, adapter->bus_num, driver->bus_id);
		if (driver->bus_id == adapter->bus_num)
		{
			printk ("%s: Found adapter 0x%x\n", __func__, adapter->bus_num);
			driver->attach_adapter(adapter);
			break;
		}
	}

	return 0;

#ifdef _USE_DRIVER_MODEL_
      err:
	return -1;
#endif
}

/**
 * spi_del_driver -
 *
 *
 */
int spi_del_driver(struct spi_driver *driver)
{
	struct list_head *item1, *item2, *_n;
	struct spi_adapter *adapter;
	struct spi_client *client;

	printk(KERN_INFO "SPI core: delete driver %s\n", driver->name);

	list_for_each(item1, &spi_adapters) {
		adapter = list_entry(item1, struct spi_adapter, list);

		list_for_each_safe(item2, _n, &adapter->clients) {
			client = list_entry(item2, struct spi_client, list);
			if (client->driver != driver)
				continue;

			if (driver->detach_client(client))
				goto err;
		}
	}
#ifdef _USE_DRIVER_MODEL_
	driver_unregister(&driver->driver);
#endif
	list_del(&driver->list);

	return 0;

      err:
	return -1;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
/* match always succeeds, as we want the probe() to tell if we really accept this match */
static int spi2_device_match(struct device *dev, struct device_driver *drv)
{
	return 1;
}

struct bus_type spi2_bus_type = {
	.name = "spi2",
	.match = spi2_device_match,
};

/**
 * spi_driver_init -
 *
 *
 */
static int __init spi_driver_init(void)
{
	int retval;

	printk(KERN_INFO "SPI core: loaded version 0.2\n");

	retval = bus_register(&spi2_bus_type);
	if (retval)
		goto err0;

#ifdef _USE_DRIVER_MODEL_
	retval = driver_register(&spi_driver);
	if (retval)
		goto err1;
#endif

	return 0;

#ifdef _USE_DRIVER_MODEL_
  err1:
	bus_unregister(&spi2_bus_type);
#endif

  err0:
	return retval;
}

/**
 * spi_driver_exit -
 *
 *
 */
static void __exit spi_driver_exit(void)
{
#ifdef _USE_DRIVER_MODEL_
	driver_unregister(&spi_driver);
#endif
	bus_unregister(&spi2_bus_type);
}

subsys_initcall(spi_driver_init);
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

static int __init spi_driver_init(void)
{
	printk(KERN_INFO "SPI core: loaded version 0.2\n");

	return 0;
}

static void __exit spi_driver_exit(void)
{

}

module_init(spi_driver_init);

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

module_exit(spi_driver_exit);

EXPORT_SYMBOL(spi_add_driver);
EXPORT_SYMBOL(spi_del_driver);

EXPORT_SYMBOL(spi_add_adapter);
EXPORT_SYMBOL(spi_del_adapter);

EXPORT_SYMBOL(spi_attach_client);
EXPORT_SYMBOL(spi_detach_client);

EXPORT_SYMBOL(spi_write_single);
EXPORT_SYMBOL(spi_write_mem);

EXPORT_SYMBOL(spi_writen);
EXPORT_SYMBOL(spi_writeb);
EXPORT_SYMBOL(spi_writew);

EXPORT_SYMBOL(spi_read_mem);
EXPORT_SYMBOL(spi_read_single);
EXPORT_SYMBOL(spi_writeread_mem);

EXPORT_SYMBOL(spi_readn);
EXPORT_SYMBOL(spi_readb);
EXPORT_SYMBOL(spi_readw);

MODULE_AUTHOR("Rui Sousa <rui.sousa@mindspeed.com>");
MODULE_DESCRIPTION("SPI core");
MODULE_LICENSE("GPL");
