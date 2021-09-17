/* linux/drivers/spi/spi-hisilicon/spi-hisilicon.c
 *
 * HISILICON SPI Controller driver
 *
 * Copyright (c) 2014 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * History:
 *      26-September-2014 create this file
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>

#include <linux/spi/spi.h>

#define DEFAULT_SPEED_HZ	2000000

/* #define HI_SPIDEV_INFO_DEBUG */
#ifdef HI_SPIDEV_INFO_DEBUG
#define hi_msg(x...) \
	do { \
		printk(KERN_NOTICE "%s->%d: ", __func__, __LINE__); \
		printk(KERN_NOTICE x); \
	} while (0)
#else
#define hi_msg(x...) do { } while (0)
#endif

/*--------------------------------------------------------*/

/*
 * SPI device information
 */
static struct spi_board_info spidev_info[] = {
	{
		.modalias       = "spidev",
		.bus_num        = 0,
		.chip_select    = 0,
		.max_speed_hz   = DEFAULT_SPEED_HZ,
		.mode           = SPI_MODE_0,
	},
	{
		.modalias       = "spidev",
		.bus_num        = 0,
		.chip_select    = 1,
		.max_speed_hz   = DEFAULT_SPEED_HZ,
		.mode           = SPI_MODE_0,
	}
};

static struct spi_device *spidev_infos[ARRAY_SIZE(spidev_info)];

/*--------------------------------------------------------*/

static int __init spidev_info_init(void)
{
	int i;
	struct spi_master *master;

	hi_msg("compile time:%s  %s\n" , __DATE__, __TIME__);

	for (i = 0; i < ARRAY_SIZE(spidev_info); i++) {
		master = spi_busnum_to_master(spidev_info[i].bus_num);
		if (master) {
			spidev_infos[i] = spi_new_device(master,
					&spidev_info[i]);
			if (!spidev_infos[i]) {
				dev_err(NULL, "%s: spi_new_device() error!\n",
						__func__);
				while (--i >= 0)
					spi_unregister_device(spidev_infos[i]);

				return -ENXIO;
			}
		} else {
			dev_err(NULL, "%s: spi_busnum_to_master() error!\n",
					__func__);
			while (--i >= 0)
				spi_unregister_device(spidev_infos[i]);

			return -ENXIO;
		}
	}

	return 0;
}
module_init(spidev_info_init);

static void __exit spidev_info_exit(void)
{
	int i;

	hi_msg("\n");
	i = ARRAY_SIZE(spidev_info);

	while (--i >= 0)
		spi_unregister_device(spidev_infos[i]);
}
module_exit(spidev_info_exit);

MODULE_AUTHOR("BVT OSDRV");
MODULE_DESCRIPTION("User mode SPI device info");
MODULE_LICENSE("GPL");
