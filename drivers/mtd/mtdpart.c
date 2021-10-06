#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kmod.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/err.h>
#include <linux/kconfig.h>
#ifdef MY_ABC_HERE
#include <linux/synolib.h>
#endif  

#include "mtdcore.h"

#ifdef MY_ABC_HERE
extern unsigned char grgbLanMac[SYNO_MAC_MAX_NUMBER][16];
extern int giVenderFormatVersion;
#endif  
#ifdef MY_ABC_HERE
extern char gszSerialNum[];
extern char gszCustomSerialNum[];
#define SYNO_SN_TAG "SN="
#define SYNO_CHKSUM_TAG "CHK="
#define SYNO_SN_12_SIG SYNO_SN_TAG   
#endif  

static LIST_HEAD(mtd_partitions);
static DEFINE_MUTEX(mtd_partitions_mutex);

struct mtd_part {
	struct mtd_info mtd;
	struct mtd_info *master;
	uint64_t offset;
	struct list_head list;
};

#if defined(MY_DEF_HERE)
static inline struct mtd_part *mtd_to_part(const struct mtd_info *mtd)
{
	return container_of(mtd, struct mtd_part, mtd);
}
#else  
#define PART(x)  ((struct mtd_part *)(x))
#endif  

static int part_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	struct mtd_ecc_stats stats;
	int res;

	stats = part->master->ecc_stats;
	res = part->master->_read(part->master, from + part->offset, len,
				  retlen, buf);
	if (unlikely(mtd_is_eccerr(res)))
		mtd->ecc_stats.failed +=
			part->master->ecc_stats.failed - stats.failed;
	else
		mtd->ecc_stats.corrected +=
			part->master->ecc_stats.corrected - stats.corrected;
	return res;
}

static int part_point(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, void **virt, resource_size_t *phys)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  

	return part->master->_point(part->master, from + part->offset, len,
				    retlen, virt, phys);
}

static int part_unpoint(struct mtd_info *mtd, loff_t from, size_t len)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  

	return part->master->_unpoint(part->master, from + part->offset, len);
}

static unsigned long part_get_unmapped_area(struct mtd_info *mtd,
					    unsigned long len,
					    unsigned long offset,
					    unsigned long flags)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  

	offset += part->offset;
	return part->master->_get_unmapped_area(part->master, len, offset,
						flags);
}

static int part_read_oob(struct mtd_info *mtd, loff_t from,
		struct mtd_oob_ops *ops)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	int res;

	if (from >= mtd->size)
		return -EINVAL;
	if (ops->datbuf && from + ops->len > mtd->size)
		return -EINVAL;

	if (ops->oobbuf) {
		size_t len, pages;

#if defined(MY_DEF_HERE)
		len = mtd_oobavail(mtd, ops);
#else  
		if (ops->mode == MTD_OPS_AUTO_OOB)
			len = mtd->oobavail;
		else
			len = mtd->oobsize;
#endif  
		pages = mtd_div_by_ws(mtd->size, mtd);
		pages -= mtd_div_by_ws(from, mtd);
		if (ops->ooboffs + ops->ooblen > pages * len)
			return -EINVAL;
	}

	res = part->master->_read_oob(part->master, from + part->offset, ops);
	if (unlikely(res)) {
		if (mtd_is_bitflip(res))
			mtd->ecc_stats.corrected++;
		if (mtd_is_eccerr(res))
			mtd->ecc_stats.failed++;
	}
	return res;
}

static int part_read_user_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, u_char *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_read_user_prot_reg(part->master, from, len,
						 retlen, buf);
}

static int part_get_user_prot_info(struct mtd_info *mtd, size_t len,
				   size_t *retlen, struct otp_info *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_get_user_prot_info(part->master, len, retlen,
						 buf);
}

static int part_read_fact_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, u_char *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_read_fact_prot_reg(part->master, from, len,
						 retlen, buf);
}

static int part_get_fact_prot_info(struct mtd_info *mtd, size_t len,
				   size_t *retlen, struct otp_info *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_get_fact_prot_info(part->master, len, retlen,
						 buf);
}

static int part_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_write(part->master, to + part->offset, len,
				    retlen, buf);
}

static int part_panic_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_panic_write(part->master, to + part->offset, len,
					  retlen, buf);
}

static int part_write_oob(struct mtd_info *mtd, loff_t to,
		struct mtd_oob_ops *ops)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  

	if (to >= mtd->size)
		return -EINVAL;
	if (ops->datbuf && to + ops->len > mtd->size)
		return -EINVAL;
	return part->master->_write_oob(part->master, to + part->offset, ops);
}

static int part_write_user_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len, size_t *retlen, u_char *buf)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_write_user_prot_reg(part->master, from, len,
						  retlen, buf);
}

static int part_lock_user_prot_reg(struct mtd_info *mtd, loff_t from,
		size_t len)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_lock_user_prot_reg(part->master, from, len);
}

static int part_writev(struct mtd_info *mtd, const struct kvec *vecs,
		unsigned long count, loff_t to, size_t *retlen)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_writev(part->master, vecs, count,
				     to + part->offset, retlen);
}

static int part_erase(struct mtd_info *mtd, struct erase_info *instr)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	int ret;

	instr->addr += part->offset;
	ret = part->master->_erase(part->master, instr);
	if (ret) {
		if (instr->fail_addr != MTD_FAIL_ADDR_UNKNOWN)
			instr->fail_addr -= part->offset;
		instr->addr -= part->offset;
	}
	return ret;
}

void mtd_erase_callback(struct erase_info *instr)
{
	if (instr->mtd->_erase == part_erase) {
#if defined(MY_DEF_HERE)
		struct mtd_part *part = mtd_to_part(instr->mtd);
#else  
		struct mtd_part *part = PART(instr->mtd);
#endif  

		if (instr->fail_addr != MTD_FAIL_ADDR_UNKNOWN)
			instr->fail_addr -= part->offset;
		instr->addr -= part->offset;
	}
	if (instr->callback)
		instr->callback(instr);
}
EXPORT_SYMBOL_GPL(mtd_erase_callback);

static int part_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_lock(part->master, ofs + part->offset, len);
}

static int part_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_unlock(part->master, ofs + part->offset, len);
}

static int part_is_locked(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_is_locked(part->master, ofs + part->offset, len);
}

static void part_sync(struct mtd_info *mtd)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	part->master->_sync(part->master);
}

static int part_suspend(struct mtd_info *mtd)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return part->master->_suspend(part->master);
}

static void part_resume(struct mtd_info *mtd)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	part->master->_resume(part->master);
}

static int part_block_isreserved(struct mtd_info *mtd, loff_t ofs)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	ofs += part->offset;
	return part->master->_block_isreserved(part->master, ofs);
}

static int part_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	ofs += part->offset;
	return part->master->_block_isbad(part->master, ofs);
}

static int part_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	int res;

	ofs += part->offset;
	res = part->master->_block_markbad(part->master, ofs);
	if (!res)
		mtd->ecc_stats.badblocks++;
	return res;
}

#if defined(MY_DEF_HERE)
static int part_get_device(struct mtd_info *mtd)
{
	struct mtd_part *part = mtd_to_part(mtd);
	return part->master->_get_device(part->master);
}

static void part_put_device(struct mtd_info *mtd)
{
	struct mtd_part *part = mtd_to_part(mtd);
	part->master->_put_device(part->master);
}

static int part_ooblayout_ecc(struct mtd_info *mtd, int section,
			      struct mtd_oob_region *oobregion)
{
	struct mtd_part *part = mtd_to_part(mtd);

	return mtd_ooblayout_ecc(part->master, section, oobregion);
}

static int part_ooblayout_free(struct mtd_info *mtd, int section,
			       struct mtd_oob_region *oobregion)
{
	struct mtd_part *part = mtd_to_part(mtd);

	return mtd_ooblayout_free(part->master, section, oobregion);
}

static const struct mtd_ooblayout_ops part_ooblayout_ops = {
	.ecc = part_ooblayout_ecc,
	.free = part_ooblayout_free,
};

#endif  
static inline void free_partition(struct mtd_part *p)
{
	kfree(p->mtd.name);
	kfree(p);
}

int del_mtd_partitions(struct mtd_info *master)
{
	struct mtd_part *slave, *next;
	int ret, err = 0;

	mutex_lock(&mtd_partitions_mutex);
	list_for_each_entry_safe(slave, next, &mtd_partitions, list)
		if (slave->master == master) {
			ret = del_mtd_device(&slave->mtd);
			if (ret < 0) {
				err = ret;
				continue;
			}
			list_del(&slave->list);
			free_partition(slave);
		}
	mutex_unlock(&mtd_partitions_mutex);

	return err;
}

static struct mtd_part *allocate_partition(struct mtd_info *master,
			const struct mtd_partition *part, int partno,
			uint64_t cur_offset)
{
	struct mtd_part *slave;
	char *name;

	slave = kzalloc(sizeof(*slave), GFP_KERNEL);
	name = kstrdup(part->name, GFP_KERNEL);
	if (!name || !slave) {
		printk(KERN_ERR"memory allocation error while creating partitions for \"%s\"\n",
		       master->name);
		kfree(name);
		kfree(slave);
		return ERR_PTR(-ENOMEM);
	}

	slave->mtd.type = master->type;
	slave->mtd.flags = master->flags & ~part->mask_flags;
	slave->mtd.size = part->size;
	slave->mtd.writesize = master->writesize;
	slave->mtd.writebufsize = master->writebufsize;
	slave->mtd.oobsize = master->oobsize;
	slave->mtd.oobavail = master->oobavail;
	slave->mtd.subpage_sft = master->subpage_sft;
#if defined(MY_DEF_HERE)
	slave->mtd.pairing = master->pairing;
#endif  

	slave->mtd.name = name;
	slave->mtd.owner = master->owner;

	slave->mtd.dev.parent = IS_ENABLED(CONFIG_MTD_PARTITIONED_MASTER) ?
				&master->dev :
				master->dev.parent;

	slave->mtd._read = part_read;
	slave->mtd._write = part_write;

	if (master->_panic_write)
		slave->mtd._panic_write = part_panic_write;

	if (master->_point && master->_unpoint) {
		slave->mtd._point = part_point;
		slave->mtd._unpoint = part_unpoint;
	}

	if (master->_get_unmapped_area)
		slave->mtd._get_unmapped_area = part_get_unmapped_area;
	if (master->_read_oob)
		slave->mtd._read_oob = part_read_oob;
	if (master->_write_oob)
		slave->mtd._write_oob = part_write_oob;
	if (master->_read_user_prot_reg)
		slave->mtd._read_user_prot_reg = part_read_user_prot_reg;
	if (master->_read_fact_prot_reg)
		slave->mtd._read_fact_prot_reg = part_read_fact_prot_reg;
	if (master->_write_user_prot_reg)
		slave->mtd._write_user_prot_reg = part_write_user_prot_reg;
	if (master->_lock_user_prot_reg)
		slave->mtd._lock_user_prot_reg = part_lock_user_prot_reg;
	if (master->_get_user_prot_info)
		slave->mtd._get_user_prot_info = part_get_user_prot_info;
	if (master->_get_fact_prot_info)
		slave->mtd._get_fact_prot_info = part_get_fact_prot_info;
	if (master->_sync)
		slave->mtd._sync = part_sync;
	if (!partno && !master->dev.class && master->_suspend &&
	    master->_resume) {
			slave->mtd._suspend = part_suspend;
			slave->mtd._resume = part_resume;
	}
	if (master->_writev)
		slave->mtd._writev = part_writev;
	if (master->_lock)
		slave->mtd._lock = part_lock;
	if (master->_unlock)
		slave->mtd._unlock = part_unlock;
	if (master->_is_locked)
		slave->mtd._is_locked = part_is_locked;
	if (master->_block_isreserved)
		slave->mtd._block_isreserved = part_block_isreserved;
	if (master->_block_isbad)
		slave->mtd._block_isbad = part_block_isbad;
	if (master->_block_markbad)
		slave->mtd._block_markbad = part_block_markbad;
#if defined(MY_DEF_HERE)

	if (master->_get_device)
		slave->mtd._get_device = part_get_device;
	if (master->_put_device)
		slave->mtd._put_device = part_put_device;

#endif  
	slave->mtd._erase = part_erase;
	slave->master = master;
	slave->offset = part->offset;

	if (slave->offset == MTDPART_OFS_APPEND)
		slave->offset = cur_offset;
	if (slave->offset == MTDPART_OFS_NXTBLK) {
		slave->offset = cur_offset;
		if (mtd_mod_by_eb(cur_offset, master) != 0) {
			 
			slave->offset = (mtd_div_by_eb(cur_offset, master) + 1) * master->erasesize;
			printk(KERN_NOTICE "Moving partition %d: "
			       "0x%012llx -> 0x%012llx\n", partno,
			       (unsigned long long)cur_offset, (unsigned long long)slave->offset);
		}
	}
	if (slave->offset == MTDPART_OFS_RETAIN) {
		slave->offset = cur_offset;
		if (master->size - slave->offset >= slave->mtd.size) {
			slave->mtd.size = master->size - slave->offset
							- slave->mtd.size;
		} else {
			printk(KERN_ERR "mtd partition \"%s\" doesn't have enough space: %#llx < %#llx, disabled\n",
				part->name, master->size - slave->offset,
				slave->mtd.size);
			 
			goto out_register;
		}
	}
	if (slave->mtd.size == MTDPART_SIZ_FULL)
		slave->mtd.size = master->size - slave->offset;

	printk(KERN_NOTICE "0x%012llx-0x%012llx : \"%s\"\n", (unsigned long long)slave->offset,
		(unsigned long long)(slave->offset + slave->mtd.size), slave->mtd.name);

	if (slave->offset >= master->size) {
		 
		slave->offset = 0;
		slave->mtd.size = 0;
		printk(KERN_ERR"mtd: partition \"%s\" is out of reach -- disabled\n",
			part->name);
		goto out_register;
	}
	if (slave->offset + slave->mtd.size > master->size) {
		slave->mtd.size = master->size - slave->offset;
		printk(KERN_WARNING"mtd: partition \"%s\" extends beyond the end of device \"%s\" -- size truncated to %#llx\n",
			part->name, master->name, (unsigned long long)slave->mtd.size);
	}
	if (master->numeraseregions > 1) {
		 
		int i, max = master->numeraseregions;
		u64 end = slave->offset + slave->mtd.size;
		struct mtd_erase_region_info *regions = master->eraseregions;

		for (i = 0; i < max && regions[i].offset <= slave->offset; i++)
			;
		 
		if (i > 0)
			i--;

		for (; i < max && regions[i].offset < end; i++) {
			if (slave->mtd.erasesize < regions[i].erasesize) {
				slave->mtd.erasesize = regions[i].erasesize;
			}
		}
		BUG_ON(slave->mtd.erasesize == 0);
	} else {
		 
		slave->mtd.erasesize = master->erasesize;
	}

	if ((slave->mtd.flags & MTD_WRITEABLE) &&
	    mtd_mod_by_eb(slave->offset, &slave->mtd)) {
		 
		slave->mtd.flags &= ~MTD_WRITEABLE;
		printk(KERN_WARNING"mtd: partition \"%s\" doesn't start on an erase block boundary -- force read-only\n",
			part->name);
	}
	if ((slave->mtd.flags & MTD_WRITEABLE) &&
	    mtd_mod_by_eb(slave->mtd.size, &slave->mtd)) {
		slave->mtd.flags &= ~MTD_WRITEABLE;
		printk(KERN_WARNING"mtd: partition \"%s\" doesn't end on an erase block -- force read-only\n",
			part->name);
	}

#if defined(MY_DEF_HERE)
	mtd_set_ooblayout(&slave->mtd, &part_ooblayout_ops);
#else  
	slave->mtd.ecclayout = master->ecclayout;
#endif  
	slave->mtd.ecc_step_size = master->ecc_step_size;
	slave->mtd.ecc_strength = master->ecc_strength;
	slave->mtd.bitflip_threshold = master->bitflip_threshold;

	if (master->_block_isbad) {
		uint64_t offs = 0;

		while (offs < slave->mtd.size) {
			if (mtd_block_isreserved(master, offs + slave->offset))
				slave->mtd.ecc_stats.bbtblocks++;
			else if (mtd_block_isbad(master, offs + slave->offset))
				slave->mtd.ecc_stats.badblocks++;
			offs += slave->mtd.erasesize;
		}
	}

out_register:
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
	if ((memcmp(part->name, "vender", 7)==0) ||
		(memcmp(part->name, "vendor", 7)==0)) {
		u_char rgbszBuf[128];
		int i = 0, x = 0;
		size_t retlen;
		unsigned int Sum;
		u_char ucSum;
#ifdef MY_ABC_HERE
		int n = 0;
		int MacNumber = 0;
		char rgbLanMac[SYNO_MAC_MAX_NUMBER][6];
#endif  
#ifdef MY_ABC_HERE
		char szSerialBuffer[32];
		char *ptr;
		char szSerial[32];
		char szCheckSum[32];
		unsigned long ulchksum = 0;
		unsigned long ulTemp = 0;
#endif  

		part_read(&slave->mtd, 0, 128, &retlen, rgbszBuf);

#ifdef MY_ABC_HERE
		 
		x = 0;
		switch (giVenderFormatVersion) {
		case 1:
			MacNumber = 4;
			break;
		case 2:
			MacNumber = 8;
			break;
		default:
			printk(KERN_ERR "Undefined verder version %d\n", giVenderFormatVersion);
		}

		for (n = 0; n < MacNumber; n++) {
			for (Sum=0,ucSum=0,i=0; i<6; i++) {
				Sum+=rgbszBuf[i+x];
				ucSum+=rgbszBuf[i+x];
				rgbLanMac[n][i] = rgbszBuf[i+x];
			}
			x+=6;

			if (0==Sum) {
				printk("vender Mac%d doesn't set ucSum:0x%02x Buf:0x%02x Sum:%d.\n",
						n, ucSum, rgbszBuf[x], Sum);
			} else if (ucSum!=rgbszBuf[x]) {
				printk("vender Mac%d checksum error ucSum:0x%02x Buf:0x%02x Sum:%d.\n",
						n, ucSum, rgbszBuf[x], Sum);
				grgbLanMac[n][0] = '\0';
			} else {
				printk("vender Mac%d address : %02x:%02x:%02x:%02x:%02x:%02x\n",n,rgbLanMac[n][0],
												  rgbLanMac[n][1],
												  rgbLanMac[n][2],
												  rgbLanMac[n][3],
												  rgbLanMac[n][4],
												  rgbLanMac[n][5]);
				snprintf(grgbLanMac[n], sizeof(grgbLanMac),
						"%02x%02x%02x%02x%02x%02x",
				rgbLanMac[n][0],
				rgbLanMac[n][1],
				rgbLanMac[n][2],
				rgbLanMac[n][3],
				rgbLanMac[n][4],
				rgbLanMac[n][5]);
			}

			x++;
		}
#endif  
#ifdef MY_ABC_HERE
		memset(szSerial, 0, sizeof(szSerial));
		memset(szCheckSum, 0, sizeof(szCheckSum));
		memset(gszSerialNum, 0, 32);
		memcpy(szSerialBuffer, &(rgbszBuf[32]), 32);

		if (0 == strncmp(szSerialBuffer, SYNO_SN_12_SIG,strlen(SYNO_SN_12_SIG))) {
			 
			ptr = strstr(szSerialBuffer, SYNO_SN_TAG);
			if (NULL == ptr) {
				printk("no serial tag found, serial buffer='%s'\n", szSerialBuffer);
				goto SKIP_SERIAL;
			}
			ptr += strlen(SYNO_SN_TAG);
			i = 0;
			while (0 != *ptr && ',' != *ptr) {
				szSerial[i++] = *ptr;
				ptr++;
			}
			szSerial[i] = '\0';

			ptr = strstr(szSerialBuffer, SYNO_CHKSUM_TAG);
			if (NULL == ptr) {
				printk("no checksum tag found, serial buffer='%s'\n", szSerialBuffer);
				goto SKIP_SERIAL;
			}
			ptr += strlen(SYNO_CHKSUM_TAG);
			i = 0;
			while (0 != *ptr && ',' != *ptr) {
				szCheckSum[i++] = *ptr;
				ptr++;
			}
			szCheckSum[i] = '\0';

			for (i = 0 ; i < strlen(szSerial); i++) {
				ulchksum += szSerial[i];
			}

			if (0 != kstrtoul(szCheckSum, 10, &ulTemp)) {
				printk("string conversion error: '%s'\n", szCheckSum);
				goto SKIP_SERIAL;
			} else if (ulchksum != ulTemp) {
				printk("serial number checksum error, serial='%s', checksum='%lu' not '%lu'\n", szSerial, ulchksum, ulTemp);
				goto SKIP_SERIAL;
			}
		} else {
			unsigned char ucChkSum = 0;
			 
			for (i = 0 ; i < 10; i++) {
				ucChkSum += szSerialBuffer[i];
			}
			 
			if (ucChkSum != szSerialBuffer[10]) {
				printk("serial number checksum error, serial='%s', checksum='%d' not '%d'", szSerialBuffer, ucChkSum, szSerialBuffer[10]);
				goto SKIP_SERIAL;
			} else {
				memcpy(szSerial, szSerialBuffer, 10);
			}
		}
		snprintf(gszSerialNum, 32, "%s", szSerial);
SKIP_SERIAL:
		printk("serial number='%s'", gszSerialNum);

		x = 64;
		for (Sum=0,ucSum=0,i=0; i<31; i++) {
			Sum+=rgbszBuf[i+x];
			ucSum+=rgbszBuf[i+x];
			gszCustomSerialNum[i] = rgbszBuf[i+x];
		}
		x+=31;
		if (Sum==0 || ucSum!=rgbszBuf[x]) {
			for (i=0; i<32; i++) {
				gszCustomSerialNum[i] = 0;
			}
		} else {
			printk("Custom Serial Number: %s\n", gszCustomSerialNum);
		}
#endif  
	}
#endif  
	return slave;
}

static ssize_t mtd_partition_offset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mtd_info *mtd = dev_get_drvdata(dev);
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  
	return snprintf(buf, PAGE_SIZE, "%lld\n", part->offset);
}

static DEVICE_ATTR(offset, S_IRUGO, mtd_partition_offset_show, NULL);

static const struct attribute *mtd_partition_attrs[] = {
	&dev_attr_offset.attr,
	NULL
};

static int mtd_add_partition_attrs(struct mtd_part *new)
{
	int ret = sysfs_create_files(&new->mtd.dev.kobj, mtd_partition_attrs);
	if (ret)
		printk(KERN_WARNING
		       "mtd: failed to create partition attrs, err=%d\n", ret);
	return ret;
}

int mtd_add_partition(struct mtd_info *master, const char *name,
		      long long offset, long long length)
{
	struct mtd_partition part;
	struct mtd_part *new;
	int ret = 0;

	if (offset == MTDPART_OFS_APPEND ||
	    offset == MTDPART_OFS_NXTBLK)
		return -EINVAL;

	if (length == MTDPART_SIZ_FULL)
		length = master->size - offset;

	if (length <= 0)
		return -EINVAL;

#if defined(MY_DEF_HERE)
	memset(&part, 0, sizeof(part));
#endif  
	part.name = name;
	part.size = length;
	part.offset = offset;
#if defined(MY_DEF_HERE)
 
#else  
	part.mask_flags = 0;
	part.ecclayout = NULL;
#endif  

	new = allocate_partition(master, &part, -1, offset);
	if (IS_ERR(new))
		return PTR_ERR(new);

	mutex_lock(&mtd_partitions_mutex);
	list_add(&new->list, &mtd_partitions);
	mutex_unlock(&mtd_partitions_mutex);

	add_mtd_device(&new->mtd);

	mtd_add_partition_attrs(new);

	return ret;
}
EXPORT_SYMBOL_GPL(mtd_add_partition);

int mtd_del_partition(struct mtd_info *master, int partno)
{
	struct mtd_part *slave, *next;
	int ret = -EINVAL;

	mutex_lock(&mtd_partitions_mutex);
	list_for_each_entry_safe(slave, next, &mtd_partitions, list)
		if ((slave->master == master) &&
		    (slave->mtd.index == partno)) {
			sysfs_remove_files(&slave->mtd.dev.kobj,
					   mtd_partition_attrs);
			ret = del_mtd_device(&slave->mtd);
			if (ret < 0)
				break;

			list_del(&slave->list);
			free_partition(slave);
			break;
		}
	mutex_unlock(&mtd_partitions_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(mtd_del_partition);

int add_mtd_partitions(struct mtd_info *master,
		       const struct mtd_partition *parts,
		       int nbparts)
{
	struct mtd_part *slave;
	uint64_t cur_offset = 0;
	int i;

	printk(KERN_NOTICE "Creating %d MTD partitions on \"%s\":\n", nbparts, master->name);

	for (i = 0; i < nbparts; i++) {
		slave = allocate_partition(master, parts + i, i, cur_offset);
		if (IS_ERR(slave)) {
			del_mtd_partitions(master);
			return PTR_ERR(slave);
		}

		mutex_lock(&mtd_partitions_mutex);
		list_add(&slave->list, &mtd_partitions);
		mutex_unlock(&mtd_partitions_mutex);

		add_mtd_device(&slave->mtd);
		mtd_add_partition_attrs(slave);

		cur_offset = slave->offset + slave->mtd.size;
	}

	return 0;
}

static DEFINE_SPINLOCK(part_parser_lock);
static LIST_HEAD(part_parsers);

#if defined(MY_DEF_HERE)
static struct mtd_part_parser *mtd_part_parser_get(const char *name)
#else  
static struct mtd_part_parser *get_partition_parser(const char *name)
#endif  
{
	struct mtd_part_parser *p, *ret = NULL;

	spin_lock(&part_parser_lock);

	list_for_each_entry(p, &part_parsers, list)
		if (!strcmp(p->name, name) && try_module_get(p->owner)) {
			ret = p;
			break;
		}

	spin_unlock(&part_parser_lock);

	return ret;
}

#if defined(MY_DEF_HERE)
static inline void mtd_part_parser_put(const struct mtd_part_parser *p)
{
	module_put(p->owner);
}

static void mtd_part_parser_cleanup_default(const struct mtd_partition *pparts,
					    int nr_parts)
{
	kfree(pparts);
}
#else  
#define put_partition_parser(p) do { module_put((p)->owner); } while (0)
#endif  

#if defined(MY_DEF_HERE)
int __register_mtd_parser(struct mtd_part_parser *p, struct module *owner)
#else  
void register_mtd_parser(struct mtd_part_parser *p)
#endif  
{
#if defined(MY_DEF_HERE)
	p->owner = owner;

	if (!p->cleanup)
		p->cleanup = &mtd_part_parser_cleanup_default;

#endif  
	spin_lock(&part_parser_lock);
	list_add(&p->list, &part_parsers);
	spin_unlock(&part_parser_lock);
#if defined(MY_DEF_HERE)

	return 0;
#endif  
}
#if defined(MY_DEF_HERE)
EXPORT_SYMBOL_GPL(__register_mtd_parser);
#else  
EXPORT_SYMBOL_GPL(register_mtd_parser);
#endif  

void deregister_mtd_parser(struct mtd_part_parser *p)
{
	spin_lock(&part_parser_lock);
	list_del(&p->list);
	spin_unlock(&part_parser_lock);
}
EXPORT_SYMBOL_GPL(deregister_mtd_parser);

static const char * const default_mtd_part_types[] = {
	"cmdlinepart",
	"ofpart",
	NULL
};

int parse_mtd_partitions(struct mtd_info *master, const char *const *types,
#if defined(MY_DEF_HERE)
			 struct mtd_partitions *pparts,
#else  
			 struct mtd_partition **pparts,
#endif  
			 struct mtd_part_parser_data *data)
{
	struct mtd_part_parser *parser;
	int ret, err = 0;

	if (!types)
		types = default_mtd_part_types;

	for ( ; *types; types++) {
		pr_debug("%s: parsing partitions %s\n", master->name, *types);
#if defined(MY_DEF_HERE)
		parser = mtd_part_parser_get(*types);
#else  
		parser = get_partition_parser(*types);
#endif  
		if (!parser && !request_module("%s", *types))
#if defined(MY_DEF_HERE)
			parser = mtd_part_parser_get(*types);
#else  
			parser = get_partition_parser(*types);
#endif  
		pr_debug("%s: got parser %s\n", master->name,
			 parser ? parser->name : NULL);
		if (!parser)
			continue;
#if defined(MY_DEF_HERE)
		ret = (*parser->parse_fn)(master, &pparts->parts, data);
#else  
		ret = (*parser->parse_fn)(master, pparts, data);
#endif  
		pr_debug("%s: parser %s: %i\n",
			 master->name, parser->name, ret);
#if defined(MY_DEF_HERE)
 
#else  
		put_partition_parser(parser);
#endif  
		if (ret > 0) {
			printk(KERN_NOTICE "%d %s partitions found on MTD device %s\n",
			       ret, parser->name, master->name);
#if defined(MY_DEF_HERE)
			pparts->nr_parts = ret;
			pparts->parser = parser;
			return 0;
#else  
			return ret;
#endif  
		}
#if defined(MY_DEF_HERE)
		mtd_part_parser_put(parser);
#endif  
		 
		if (ret < 0 && !err)
			err = ret;
	}
	return err;
}

#if defined(MY_DEF_HERE)
void mtd_part_parser_cleanup(struct mtd_partitions *parts)
{
	const struct mtd_part_parser *parser;

	if (!parts)
		return;

	parser = parts->parser;
	if (parser) {
		if (parser->cleanup)
			parser->cleanup(parts->parts, parts->nr_parts);

		mtd_part_parser_put(parser);
	}
}

#endif  
int mtd_is_partition(const struct mtd_info *mtd)
{
	struct mtd_part *part;
	int ispart = 0;

	mutex_lock(&mtd_partitions_mutex);
	list_for_each_entry(part, &mtd_partitions, list)
		if (&part->mtd == mtd) {
			ispart = 1;
			break;
		}
	mutex_unlock(&mtd_partitions_mutex);

	return ispart;
}
EXPORT_SYMBOL_GPL(mtd_is_partition);

uint64_t mtd_get_device_size(const struct mtd_info *mtd)
{
	if (!mtd_is_partition(mtd))
		return mtd->size;

#if defined(MY_DEF_HERE)
	return mtd_to_part(mtd)->master->size;
#else  
	return PART(mtd)->master->size;
#endif  
}
EXPORT_SYMBOL_GPL(mtd_get_device_size);

#ifdef MY_ABC_HERE
int SYNOMTDModifyPartInfo(struct mtd_info *mtd, unsigned long offset, unsigned long length)
{
#if defined(MY_DEF_HERE)
	struct mtd_part *part = mtd_to_part(mtd);
#else  
	struct mtd_part *part = PART(mtd);
#endif  

	part->offset = offset;
	part->offset &= part->master->size-1;

	mtd->size = length;

	if (part->offset + mtd->size > part->master->size) {
		return -EFAULT;
	}

	return 0;
}
#endif  
