 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/mtd/map.h>
#include <linux/mtd/mtd.h>
#include "mvCommon.h"
#include "mvOs.h"
#include "sflash/mvSFlash.h"
#include "sflash/mvSFlashSpec.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#ifdef MTD_SFLASH_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

#define sflash_disable_irqs(flags, sflash_in_irq)	\
	sflash_in_irq = in_interrupt();			\
	if(!sflash_in_irq)	 	 		\
		local_irq_save(flags);	 

#define sflash_enable_irqs(flags, sflash_in_irq)	\
	if(!sflash_in_irq)				\
		local_irq_restore(flags);	 

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
	typedef	uint32_t 	sflash_size_t;
#else
	typedef uint64_t	sflash_size_t;
#endif

static struct mtd_info *sflash_probe(struct map_info *map);
static void sflash_destroy(struct mtd_info *mtd);
static int sflash_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
static int sflash_write(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, const u_char *buf);
static int sflash_erase(struct mtd_info *mtd, struct erase_info *instr);
static void sflash_sync(struct mtd_info *mtd);
static int sflash_suspend(struct mtd_info *mtd);
static void sflash_resume(struct mtd_info *mtd);
static int sflash_lock (struct mtd_info *mtd, loff_t ofs, sflash_size_t len);
static int sflash_unlock (struct mtd_info *mtd, loff_t ofs, sflash_size_t len);
static int sflash_block_isbad (struct mtd_info *mtd, loff_t ofs);
static int sflash_block_markbad (struct mtd_info *mtd, loff_t ofs);

static struct mtd_chip_driver sflash_chipdrv = {
	.probe		= sflash_probe,
	.destroy	= sflash_destroy,
	.name		= "sflash",
	.module		= THIS_MODULE
};

static struct mtd_info *sflash_probe(struct map_info *map)
{
	struct mtd_info *mtd = NULL;
	MV_SFLASH_INFO *sflash = NULL;
	MV_ULONG flags = 0, sflash_in_irq = 0;

#if defined(CONFIG_MV78200) || defined(CONFIG_MV632X)
	if (MV_FALSE == mvSocUnitIsMappedToThisCpu(SPI_FLASH))
	{
		printk(KERN_INFO"SPI flash is not mapped to this CPU\n");
		return -ENODEV;
	}		
#endif

	DB(printk("\nINFO: entering %s",__FUNCTION__));

	mtd = kmalloc(sizeof(*mtd), GFP_KERNEL);
	if(!mtd)
	{
		printk(KERN_NOTICE "\nERROR: %s - Failed to allocate memory for mtd structure",__FUNCTION__);
		return NULL;
	}

	sflash = kmalloc(sizeof(MV_SFLASH_INFO), GFP_KERNEL);
	if(!sflash) 
	{
		printk(KERN_NOTICE "\nERROR: %s - Failed to allocate memory for sflash structure",__FUNCTION__);
		kfree(mtd);
		return NULL;
	}
		
	memset(mtd, 0, sizeof(*mtd));
	memset(sflash, 0, sizeof(*sflash));
	    
	DB(printk("\nINFO: %s - Base address %08x\n",__FUNCTION__, map->phys));
#ifdef CONFIG_ARCH_FEROCEON_ORION	
	 
    if (mvCtrlSpiBusModeDetect() != MV_SPI_CONN_TO_EXT_FLASH)
    {
        printk(KERN_NOTICE "\nERROR: %s - SPI interface is not routed to external SPI flash!", __FUNCTION__);
		kfree(mtd);
		kfree(sflash);
		return NULL;
    }
#endif
	 	
	sflash->baseAddr         = map->phys;
	sflash->index            = MV_INVALID_DEVICE_NUMBER;  	
	sflash_disable_irqs(flags, sflash_in_irq);	
	if (mvSFlashInit(sflash) != MV_OK)
	{
		sflash_enable_irqs(flags, sflash_in_irq);
		printk(KERN_NOTICE "ERROR: %s - Failed to initialize the SFlash.", __FUNCTION__);
		kfree(mtd);
		kfree(sflash);
		return NULL;
	}
	sflash_enable_irqs(flags, sflash_in_irq);
	
	mtd->erasesize = sflash->sectorSize;
	mtd->size = sflash->sectorSize * sflash->sectorNumber;
	mtd->priv = map;  
	mtd->type = MTD_NORFLASH;
	mtd->erase = sflash_erase;
	mtd->read = sflash_read;
	mtd->write = sflash_write;
	mtd->sync = sflash_sync;
	mtd->suspend = sflash_suspend;
	mtd->resume = sflash_resume;	
	mtd->lock = sflash_lock;
	mtd->unlock = sflash_unlock;
	mtd->block_isbad = sflash_block_isbad;
	mtd->block_markbad = sflash_block_markbad;	
	mtd->flags = (MTD_WRITEABLE | MTD_BIT_WRITEABLE);  
	mtd->name = map->name;
	mtd->writesize = 1;
	mtd->writebufsize = 1;  
	
	map->fldrv = &sflash_chipdrv;
	map->fldrv_priv = sflash;
	
	DB(printk("\nINFO: %s - Detected SFlash device (size %d)", __FUNCTION__, mtd->size));
	DB(printk("\n           Base Address    : 0x%08x", sflash->baseAddr));
	DB(printk("\n           Manufacturer ID : 0x%02x", sflash->manufacturerId));
	DB(printk("\n           Device ID       : 0x%04x", sflash->deviceId));
	DB(printk("\n           Sector Size     : 0x%x", sflash->sectorSize));
	DB(printk("\n           Sector Number   : %d", sflash->sectorNumber));
	
	printk("SPI Serial flash detected @ 0x%08x, %dKB (%dsec x %dKB)\n",
	         sflash->baseAddr, ((sflash->sectorNumber * sflash->sectorSize)/1024), 
	         sflash->sectorNumber, (sflash->sectorSize/1024));
	
	__module_get(THIS_MODULE);
	return mtd;
}

static void sflash_destroy(struct mtd_info *mtd)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;

	DB(printk("\nINFO: %s called", __FUNCTION__));

	if (sflash)
		kfree(sflash);	
}

static int sflash_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;
	MV_U32 offset = ((MV_U32)from);
	MV_ULONG flags = 0, sflash_in_irq = 0;

	*retlen = 0;

	DB(printk("\nINFO: %s  - offset %08x, len %d",__FUNCTION__, offset, (int)len));

	sflash_disable_irqs(flags, sflash_in_irq);
	if (mvSFlashBlockRd(sflash, offset, buf, len) != MV_OK)
	{
		sflash_enable_irqs(flags, sflash_in_irq);
		printk(KERN_NOTICE "\nERROR: %s - Failed to read block.", __FUNCTION__);
		return -1;
	}
	sflash_enable_irqs(flags, sflash_in_irq);
	
	*retlen = len;
	
	DB(printk(" - OK"));
	return 0;	
}

static int sflash_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;
 
	MV_U32 offset = ((MV_U32)to);
	MV_ULONG flags = 0, sflash_in_irq = 0;
	
	*retlen = 0;
	
	DB(printk("\nINFO: %s - offset %08x, len %d",__FUNCTION__, offset, len));
		
	sflash_disable_irqs(flags, sflash_in_irq);
	if (mvSFlashBlockWr(sflash, offset, (MV_U8*)buf, len) != MV_OK)
	{
		sflash_enable_irqs(flags, sflash_in_irq);
		printk(KERN_NOTICE "\nERROR: %s - Failed to write block", __FUNCTION__);
		return -1;
	}
	sflash_enable_irqs(flags, sflash_in_irq);
	
	*retlen = len;
	
	DB(printk(" - OK"));
	return 0;	

}

static int sflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;
 
	MV_U32 fsec, lsec;
#ifdef CONFIG_SYNO_ARMADA
	MV_U32 count, sleep_interval;
#endif
	int i;
	MV_ULONG flags = 0, sflash_in_irq = 0;

	DB(printk("\nINFO: %s - Addr %08x, len %d",__FUNCTION__, instr->addr, instr->len));
	
	if(instr->addr & (mtd->erasesize - 1))
	{
		printk(KERN_NOTICE "\nError: %s - Erase address not sector alligned",__FUNCTION__);
		return -EINVAL;
	}
	if(instr->len & (mtd->erasesize - 1))
	{
		printk(KERN_NOTICE "\nError: %s - Erase length is not sector alligned",__FUNCTION__);
		return -EINVAL;
	}
	if(instr->len + instr->addr > mtd->size)
	{
		printk(KERN_NOTICE "\nError: %s - Erase exceeded flash size",__FUNCTION__);
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
	fsec = (instr->addr / mtd->erasesize);
	lsec = (fsec +(instr->len / mtd->erasesize));
#else
	fsec = instr->addr;
	do_div(fsec, mtd->erasesize);
	lsec = instr->len;
	do_div(lsec, mtd->erasesize);
	lsec = (fsec + lsec);
#endif

	DB(printk("\nINFO: %s - from sector %u to %u",__FUNCTION__, fsec, 
		  lsec-1));

#ifndef CONFIG_SYNO_ARMADA
	sflash_disable_irqs(flags, sflash_in_irq);
#endif
#ifdef CONFIG_SYNO_ARMADA
	count = lsec - fsec;
	do_div(count, 4);
	sleep_interval = fsec + count;
#endif
	for (i=fsec; i<lsec; i++)
	{
#ifdef CONFIG_SYNO_ARMADA
		if (i == sleep_interval) {
			sleep_interval += count;
			msleep(1000);
		}
		sflash_disable_irqs(flags, sflash_in_irq);
#endif
		if (mvSFlashSectorErase(sflash, i) != MV_OK)
		{
			sflash_enable_irqs(flags, sflash_in_irq);
			printk(KERN_NOTICE "\nError: %s - mvSFlashSectorErase on sector %d",__FUNCTION__, i);
			return -1;
		}
#ifdef CONFIG_SYNO_ARMADA
		sflash_enable_irqs(flags, sflash_in_irq);
#endif
	}
#ifndef CONFIG_SYNO_ARMADA
	sflash_enable_irqs(flags, sflash_in_irq);
#endif
	
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int sflash_lock (struct mtd_info *mtd, loff_t ofs, sflash_size_t len)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;
	MV_ULONG flags = 0, sflash_in_irq = 0;
 
	DB(printk("\nINFO: %s called", __FUNCTION__));
	
	sflash_disable_irqs(flags, sflash_in_irq);
	if (mvSFlashWpRegionSet(sflash, MV_WP_ALL) != MV_OK)
	{
		sflash_enable_irqs(flags, sflash_in_irq);
		printk(KERN_NOTICE "\nError: %s - mvSFlashWpRegionSet failed",__FUNCTION__);
		return -1;
	}
	sflash_enable_irqs(flags, sflash_in_irq);

#ifdef CONFIG_SYNO_ARMADA
#else
	printk("\nNotice: Serial SPI flash (%s) lock per sector is not supported!\n        Locking the whole device.", mtd->name);
#endif
		
	return 0;
}

static int sflash_unlock (struct mtd_info *mtd, loff_t ofs, sflash_size_t len)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;
	MV_ULONG flags = 0, sflash_in_irq = 0;
 
	DB(printk("\nINFO: %s called", __FUNCTION__));
	
	sflash_disable_irqs(flags, sflash_in_irq);
	if (mvSFlashWpRegionSet(sflash, MV_WP_NONE) != MV_OK)
	{
		sflash_enable_irqs(flags, sflash_in_irq);
		printk(KERN_NOTICE "\nError: %s - mvSFlashWpRegionSet failed",__FUNCTION__);
		return -1;
	}
	sflash_enable_irqs(flags, sflash_in_irq);

#ifdef CONFIG_SYNO_ARMADA
#else
	printk("\nNotice: Serial SPI flash (%s) unlock per sector is not supported!\n        Unlocking the whole device.", mtd->name);
#endif

	return 0;
}

static void sflash_sync(struct mtd_info *mtd)
{
	DB(printk("\nINFO: %s called - DUMMY", __FUNCTION__));
}

static int sflash_suspend(struct mtd_info *mtd)
{
	DB(printk("\nINFO: %s called - DUMMY()", __FUNCTION__));
	return 0;
}

static void sflash_resume(struct mtd_info *mtd)
{
	struct map_info *map = mtd->priv;
	MV_SFLASH_INFO *sflash = map->fldrv_priv;
	MV_ULONG flags = 0, sflash_in_irq = 0;

	sflash_disable_irqs(flags, sflash_in_irq);
	if (mvSFlashInit(sflash) != MV_OK)
	{
		sflash_enable_irqs(flags, sflash_in_irq);
		printk(KERN_NOTICE "ERROR: %s - Failed to initialize the SFlash.", __FUNCTION__);
		kfree(mtd);
		kfree(sflash);
		return;
	}
	sflash_enable_irqs(flags, sflash_in_irq);

	printk(KERN_NOTICE "Resuming serial Flash succeeded\n");
}

static int sflash_block_isbad (struct mtd_info *mtd, loff_t ofs)
{
	DB(printk("\nINFO: %s called - DUMMY", __FUNCTION__));
	return 0;
}

static int sflash_block_markbad (struct mtd_info *mtd, loff_t ofs)
{
	DB(printk("\nINFO: %s called - DUMMY", __FUNCTION__));
	return 0;
}

static int __init sflash_probe_init(void)
{
	DB(printk("\nINFO: %s - MTD SFlash chip driver.\n", __FUNCTION__));

	register_mtd_chip_driver(&sflash_chipdrv);

	return 0;
}

static void __exit sflash_probe_exit(void)
{
	DB(printk(KERN_ALERT "\nINFO: %s - MTD SFlash driver exit", __FUNCTION__));
	unregister_mtd_chip_driver(&sflash_chipdrv);
}

subsys_initcall(sflash_probe_init);
 
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("MTD chip driver for the SPI serial flash device");
