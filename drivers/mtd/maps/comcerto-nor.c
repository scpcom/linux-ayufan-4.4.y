/*
 *  linux/drivers/mtd/maps/comcerto-nor.c
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/slab.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <asm/mach/flash.h>

static const char *part_probes[] = { "cmdlinepart", NULL };

struct comcertoflash_info {
	struct mtd_info *mtd;
	struct map_info map;
	struct resource *res;
};

static int comcertoflash_probe(struct platform_device *pdev)
{
	int err;
	struct comcertoflash_info *info;
	struct flash_platform_data *pdata = pdev->dev.platform_data;
	struct resource *res = pdev->resource;
	unsigned long size = res->end - res->start + 1;


	info = kmalloc(sizeof(struct comcertoflash_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memset(info, 0, sizeof(struct comcertoflash_info));

	if (!request_mem_region(res->start, size, "flash")) {
		err = -EBUSY;
		goto out_free_info;
	}

	info->map.virt = ioremap(res->start, size);
	if (!info->map.virt) {
		err = -ENOMEM;
		goto out_release_mem_region;
	}

	info->map.name = dev_name(&pdev->dev);
	info->map.phys = res->start;
	info->map.size = size;
	info->map.bankwidth = pdata->width;
	// info->map.set_vpp  = comcerto_set_vpp;

	simple_map_init(&info->map);

	info->mtd = do_map_probe(pdata->map_name, &info->map);
	if (!info->mtd) {
		err = -EIO;
		goto out_iounmap;
	}
	info->mtd->owner = THIS_MODULE;

	platform_set_drvdata(pdev, info);

        err = mtd_device_parse_register(info->mtd, part_probes, (struct mtd_part_parser_data *)pdev->resource->start,
                        pdata->parts, pdata->nr_parts);
        if (err) {
                printk(KERN_ERR "Could not parse partitions\n");
                
        }

	return 0;

      out_iounmap:
	iounmap(info->map.virt);
      out_release_mem_region:
	release_mem_region(res->start, size);
      out_free_info:
	kfree(info);

	return err;
}

static int comcertoflash_remove(struct platform_device *pdev)
{
	struct comcertoflash_info *info = platform_get_drvdata(pdev);
        struct flash_platform_data *plat = pdev->dev.platform_data;

	platform_set_drvdata(pdev, NULL);

        if(!info)
                return 0;

        if (info->mtd) {
                mtd_device_unregister(info->mtd);
                map_destroy(info->mtd);
        }
        if (info->map.virt)
                iounmap(info->map.virt);

        if (info->res) {
                release_resource(info->res);
                kfree(info->res);
        }

        if (plat->exit)
                plat->exit();

	return 0;
}

static struct platform_driver comcertoflash_driver = {
	.probe = comcertoflash_probe,
	.remove = comcertoflash_remove,
	.driver = {
		   .name = "comcertoflash",
		   },
};

static int __init comcertoflash_init(void)
{
	return platform_driver_register(&comcertoflash_driver);
}

static void __exit comcertoflash_exit(void)
{
	platform_driver_unregister(&comcertoflash_driver);
}

module_init(comcertoflash_init);
module_exit(comcertoflash_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTD NOR map driver for Mindspeed Comcerto boards");
