#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * For boards with physically mapped flash and using
 * drivers/mtd/maps/physmap.c mapping driver.
 *
 * Copyright (C) 2003 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __LINUX_MTD_PHYSMAP__
#define __LINUX_MTD_PHYSMAP__

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#if defined(CONFIG_SYNO_COMCERTO)
#include <linux/platform_device.h>
#endif

struct map_info;
struct platform_device;

struct physmap_flash_data {
	unsigned int		width;
	int			(*init)(struct platform_device *);
	void			(*exit)(struct platform_device *);
	void			(*set_vpp)(struct platform_device *, int);
	unsigned int		nr_parts;
	unsigned int		pfow_base;
	char                    *probe_type;
	struct mtd_partition	*parts;
};

#endif /* __LINUX_MTD_PHYSMAP__ */
