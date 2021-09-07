#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/errno.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string_helpers.h>
#include <linux/async.h>
#include <linux/ata.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/scsicam.h>
#ifdef SYNO_SAS_DISK_NAME
#include <scsi/scsi_transport_sas.h>
#endif

#include "sd.h"
#include "scsi_logging.h"

MODULE_AUTHOR("Eric Youngdale");
MODULE_DESCRIPTION("SCSI disk (sd) driver");
MODULE_LICENSE("GPL");

MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK0_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK1_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK2_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK3_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK4_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK5_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK6_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK7_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK8_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK9_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK10_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK11_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK12_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK13_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK14_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK15_MAJOR);
MODULE_ALIAS_SCSI_DEVICE(TYPE_DISK);
MODULE_ALIAS_SCSI_DEVICE(TYPE_MOD);
MODULE_ALIAS_SCSI_DEVICE(TYPE_RBC);

#if !defined(CONFIG_DEBUG_BLOCK_EXT_DEVT)
#define SD_MINORS	16
#else
#define SD_MINORS	0
#endif

#ifdef MY_ABC_HERE
extern int gSynoHasDynModule;
#endif

static int  sd_revalidate_disk(struct gendisk *);
static int  sd_probe(struct device *);
static int  sd_remove(struct device *);
static void sd_shutdown(struct device *);
static int sd_suspend(struct device *, pm_message_t state);
static int sd_resume(struct device *);
static void sd_rescan(struct device *);
static int sd_done(struct scsi_cmnd *);
static void sd_read_capacity(struct scsi_disk *sdkp, unsigned char *buffer);
static void scsi_disk_release(struct device *cdev);
static void sd_print_sense_hdr(struct scsi_disk *, struct scsi_sense_hdr *);
static void sd_print_result(struct scsi_disk *, int);

static DEFINE_SPINLOCK(sd_index_lock);
static DEFINE_IDA(sd_index_ida);
#ifdef MY_ABC_HERE
#include <linux/libata.h>
#include <linux/usb.h>
#include "../usb/storage/usb.h"

#ifdef SYNO_SAS_DISK_NAME
static DEFINE_IDA(usb_index_ida);
static DEFINE_IDA(iscsi_index_ida);
static DEFINE_IDA(sas_index_ida);
extern int g_is_sas_model;
#endif
#ifdef MY_ABC_HERE
extern u8 syno_is_synology_pm(const struct ata_port *ap);
#endif
#endif

static DEFINE_MUTEX(sd_ref_mutex);

struct kmem_cache *sd_cdb_cache;
mempool_t *sd_cdb_pool;

static const char *sd_cache_types[] = {
	"write through", "none", "write back",
	"write back, no read (daft)"
};

static ssize_t
sd_store_cache_type(struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int i, ct = -1, rcd, wce, sp;
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;
	char buffer[64];
	char *buffer_data;
	struct scsi_mode_data data;
	struct scsi_sense_hdr sshdr;
	int len;

	if (sdp->type != TYPE_DISK)
		 
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(sd_cache_types); i++) {
		const int len = strlen(sd_cache_types[i]);
		if (strncmp(sd_cache_types[i], buf, len) == 0 &&
		    buf[len] == '\n') {
			ct = i;
			break;
		}
	}
	if (ct < 0)
		return -EINVAL;
	rcd = ct & 0x01 ? 1 : 0;
	wce = ct & 0x02 ? 1 : 0;
	if (scsi_mode_sense(sdp, 0x08, 8, buffer, sizeof(buffer), SD_TIMEOUT,
			    SD_MAX_RETRIES, &data, NULL))
		return -EINVAL;
	len = min_t(size_t, sizeof(buffer), data.length - data.header_length -
		  data.block_descriptor_length);
	buffer_data = buffer + data.header_length +
		data.block_descriptor_length;
	buffer_data[2] &= ~0x05;
	buffer_data[2] |= wce << 2 | rcd;
	sp = buffer_data[0] & 0x80 ? 1 : 0;

	if (scsi_mode_select(sdp, 1, sp, 8, buffer_data, len, SD_TIMEOUT,
			     SD_MAX_RETRIES, &data, &sshdr)) {
		if (scsi_sense_valid(&sshdr))
			sd_print_sense_hdr(sdkp, &sshdr);
		return -EINVAL;
	}
	revalidate_disk(sdkp->disk);
	return count;
}

static ssize_t
sd_store_manage_start_stop(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	sdp->manage_start_stop = simple_strtoul(buf, NULL, 10);

	return count;
}

static ssize_t
sd_store_allow_restart(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	if (sdp->type != TYPE_DISK)
		return -EINVAL;

	sdp->allow_restart = simple_strtoul(buf, NULL, 10);

	return count;
}

static ssize_t
sd_show_cache_type(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	int ct = sdkp->RCD + 2*sdkp->WCE;

	return snprintf(buf, 40, "%s\n", sd_cache_types[ct]);
}

static ssize_t
sd_show_fua(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->DPOFUA);
}

static ssize_t
sd_show_manage_start_stop(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;

	return snprintf(buf, 20, "%u\n", sdp->manage_start_stop);
}

static ssize_t
sd_show_allow_restart(struct device *dev, struct device_attribute *attr,
		      char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 40, "%d\n", sdkp->device->allow_restart);
}

static ssize_t
sd_show_protection_type(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->protection_type);
}

static ssize_t
sd_show_app_tag_own(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->ATO);
}

static struct device_attribute sd_disk_attrs[] = {
	__ATTR(cache_type, S_IRUGO|S_IWUSR, sd_show_cache_type,
	       sd_store_cache_type),
	__ATTR(FUA, S_IRUGO, sd_show_fua, NULL),
	__ATTR(allow_restart, S_IRUGO|S_IWUSR, sd_show_allow_restart,
	       sd_store_allow_restart),
	__ATTR(manage_start_stop, S_IRUGO|S_IWUSR, sd_show_manage_start_stop,
	       sd_store_manage_start_stop),
	__ATTR(protection_type, S_IRUGO, sd_show_protection_type, NULL),
	__ATTR(app_tag_own, S_IRUGO, sd_show_app_tag_own, NULL),
	__ATTR_NULL,
};

static struct class sd_disk_class = {
	.name		= "scsi_disk",
	.owner		= THIS_MODULE,
	.dev_release	= scsi_disk_release,
	.dev_attrs	= sd_disk_attrs,
};

static struct scsi_driver sd_template = {
	.owner			= THIS_MODULE,
	.gendrv = {
		.name		= "sd",
		.probe		= sd_probe,
		.remove		= sd_remove,
		.suspend	= sd_suspend,
		.resume		= sd_resume,
		.shutdown	= sd_shutdown,
	},
	.rescan			= sd_rescan,
	.done			= sd_done,
};

static int sd_major(int major_idx)
{
	switch (major_idx) {
	case 0:
		return SCSI_DISK0_MAJOR;
	case 1 ... 7:
		return SCSI_DISK1_MAJOR + major_idx - 1;
	case 8 ... 15:
		return SCSI_DISK8_MAJOR + major_idx - 8;
	default:
		BUG();
		return 0;	 
	}
}

static struct scsi_disk *__scsi_disk_get(struct gendisk *disk)
{
	struct scsi_disk *sdkp = NULL;

	if (disk->private_data) {
		sdkp = scsi_disk(disk);
		if (scsi_device_get(sdkp->device) == 0)
			get_device(&sdkp->dev);
		else
			sdkp = NULL;
	}
	return sdkp;
}

static struct scsi_disk *scsi_disk_get(struct gendisk *disk)
{
	struct scsi_disk *sdkp;

	mutex_lock(&sd_ref_mutex);
	sdkp = __scsi_disk_get(disk);
	mutex_unlock(&sd_ref_mutex);
	return sdkp;
}

static struct scsi_disk *scsi_disk_get_from_dev(struct device *dev)
{
	struct scsi_disk *sdkp;

	mutex_lock(&sd_ref_mutex);
	sdkp = dev_get_drvdata(dev);
	if (sdkp)
		sdkp = __scsi_disk_get(sdkp->disk);
	mutex_unlock(&sd_ref_mutex);
	return sdkp;
}

static void scsi_disk_put(struct scsi_disk *sdkp)
{
	struct scsi_device *sdev = sdkp->device;

	mutex_lock(&sd_ref_mutex);
	put_device(&sdkp->dev);
	scsi_device_put(sdev);
	mutex_unlock(&sd_ref_mutex);
}

static void sd_prot_op(struct scsi_cmnd *scmd, unsigned int dif)
{
	unsigned int prot_op = SCSI_PROT_NORMAL;
	unsigned int dix = scsi_prot_sg_count(scmd);

	if (scmd->sc_data_direction == DMA_FROM_DEVICE) {
		if (dif && dix)
			prot_op = SCSI_PROT_READ_PASS;
		else if (dif && !dix)
			prot_op = SCSI_PROT_READ_STRIP;
		else if (!dif && dix)
			prot_op = SCSI_PROT_READ_INSERT;
	} else {
		if (dif && dix)
			prot_op = SCSI_PROT_WRITE_PASS;
		else if (dif && !dix)
			prot_op = SCSI_PROT_WRITE_INSERT;
		else if (!dif && dix)
			prot_op = SCSI_PROT_WRITE_STRIP;
	}

	scsi_set_prot_op(scmd, prot_op);
	scsi_set_prot_type(scmd, dif);
}

static int sd_prep_fn(struct request_queue *q, struct request *rq)
{
	struct scsi_cmnd *SCpnt;
	struct scsi_device *sdp = q->queuedata;
	struct gendisk *disk = rq->rq_disk;
	struct scsi_disk *sdkp;
	sector_t block = blk_rq_pos(rq);
	sector_t threshold;
	unsigned int this_count = blk_rq_sectors(rq);
	int ret, host_dif;
	unsigned char protect;

	if (rq->cmd_type == REQ_TYPE_BLOCK_PC) {
		ret = scsi_setup_blk_pc_cmnd(sdp, rq);
		goto out;
	} else if (rq->cmd_type != REQ_TYPE_FS) {
		ret = BLKPREP_KILL;
		goto out;
	}
	ret = scsi_setup_fs_cmnd(sdp, rq);
	if (ret != BLKPREP_OK)
		goto out;
	SCpnt = rq->special;
	sdkp = scsi_disk(disk);

	ret = BLKPREP_KILL;

	SCSI_LOG_HLQUEUE(1, scmd_printk(KERN_INFO, SCpnt,
					"sd_init_command: block=%llu, "
					"count=%d\n",
					(unsigned long long)block,
					this_count));

	if (!sdp || !scsi_device_online(sdp) ||
	    block + blk_rq_sectors(rq) > get_capacity(disk)) {
		SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt,
						"Finishing %u sectors\n",
						blk_rq_sectors(rq)));
		SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt,
						"Retry with 0x%p\n", SCpnt));
		goto out;
	}

	if (sdp->changed) {
		 
		goto out;
	}

	threshold = get_capacity(disk) - SD_LAST_BUGGY_SECTORS *
		(sdp->sector_size / 512);

	if (unlikely(sdp->last_sector_bug && block + this_count > threshold)) {
		if (block < threshold) {
			 
			this_count = threshold - block;
		} else {
			 
			this_count = sdp->sector_size / 512;
		}
	}

	SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt, "block=%llu\n",
					(unsigned long long)block));

	if (sdp->sector_size == 1024) {
		if ((block & 1) || (blk_rq_sectors(rq) & 1)) {
			scmd_printk(KERN_ERR, SCpnt,
				    "Bad block number requested\n");
			goto out;
		} else {
			block = block >> 1;
			this_count = this_count >> 1;
		}
	}
	if (sdp->sector_size == 2048) {
		if ((block & 3) || (blk_rq_sectors(rq) & 3)) {
			scmd_printk(KERN_ERR, SCpnt,
				    "Bad block number requested\n");
			goto out;
		} else {
			block = block >> 2;
			this_count = this_count >> 2;
		}
	}
	if (sdp->sector_size == 4096) {
		if ((block & 7) || (blk_rq_sectors(rq) & 7)) {
			scmd_printk(KERN_ERR, SCpnt,
				    "Bad block number requested\n");
			goto out;
		} else {
			block = block >> 3;
			this_count = this_count >> 3;
		}
	}
	if (rq_data_dir(rq) == WRITE) {
		if (!sdp->writeable) {
			goto out;
		}
		SCpnt->cmnd[0] = WRITE_6;
		SCpnt->sc_data_direction = DMA_TO_DEVICE;

		if (blk_integrity_rq(rq) &&
		    sd_dif_prepare(rq, block, sdp->sector_size) == -EIO)
			goto out;

	} else if (rq_data_dir(rq) == READ) {
		SCpnt->cmnd[0] = READ_6;
		SCpnt->sc_data_direction = DMA_FROM_DEVICE;
	} else {
		scmd_printk(KERN_ERR, SCpnt, "Unknown command %x\n", rq->cmd_flags);
		goto out;
	}

	SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt,
					"%s %d/%u 512 byte blocks.\n",
					(rq_data_dir(rq) == WRITE) ?
					"writing" : "reading", this_count,
					blk_rq_sectors(rq)));

	host_dif = scsi_host_dif_capable(sdp->host, sdkp->protection_type);
	if (host_dif)
		protect = 1 << 5;
	else
		protect = 0;

	if (host_dif == SD_DIF_TYPE2_PROTECTION) {
		SCpnt->cmnd = mempool_alloc(sd_cdb_pool, GFP_ATOMIC);

		if (unlikely(SCpnt->cmnd == NULL)) {
			ret = BLKPREP_DEFER;
			goto out;
		}

		SCpnt->cmd_len = SD_EXT_CDB_SIZE;
		memset(SCpnt->cmnd, 0, SCpnt->cmd_len);
		SCpnt->cmnd[0] = VARIABLE_LENGTH_CMD;
		SCpnt->cmnd[7] = 0x18;
		SCpnt->cmnd[9] = (rq_data_dir(rq) == READ) ? READ_32 : WRITE_32;
		SCpnt->cmnd[10] = protect | (blk_fua_rq(rq) ? 0x8 : 0);

		SCpnt->cmnd[12] = sizeof(block) > 4 ? (unsigned char) (block >> 56) & 0xff : 0;
		SCpnt->cmnd[13] = sizeof(block) > 4 ? (unsigned char) (block >> 48) & 0xff : 0;
		SCpnt->cmnd[14] = sizeof(block) > 4 ? (unsigned char) (block >> 40) & 0xff : 0;
		SCpnt->cmnd[15] = sizeof(block) > 4 ? (unsigned char) (block >> 32) & 0xff : 0;
		SCpnt->cmnd[16] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[17] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[18] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[19] = (unsigned char) block & 0xff;

		SCpnt->cmnd[20] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[21] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[22] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[23] = (unsigned char) block & 0xff;

		SCpnt->cmnd[28] = (unsigned char) (this_count >> 24) & 0xff;
		SCpnt->cmnd[29] = (unsigned char) (this_count >> 16) & 0xff;
		SCpnt->cmnd[30] = (unsigned char) (this_count >> 8) & 0xff;
		SCpnt->cmnd[31] = (unsigned char) this_count & 0xff;
	} else if (block > 0xffffffff) {
		SCpnt->cmnd[0] += READ_16 - READ_6;
		SCpnt->cmnd[1] = protect | (blk_fua_rq(rq) ? 0x8 : 0);
		SCpnt->cmnd[2] = sizeof(block) > 4 ? (unsigned char) (block >> 56) & 0xff : 0;
		SCpnt->cmnd[3] = sizeof(block) > 4 ? (unsigned char) (block >> 48) & 0xff : 0;
		SCpnt->cmnd[4] = sizeof(block) > 4 ? (unsigned char) (block >> 40) & 0xff : 0;
		SCpnt->cmnd[5] = sizeof(block) > 4 ? (unsigned char) (block >> 32) & 0xff : 0;
		SCpnt->cmnd[6] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[7] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[8] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[9] = (unsigned char) block & 0xff;
		SCpnt->cmnd[10] = (unsigned char) (this_count >> 24) & 0xff;
		SCpnt->cmnd[11] = (unsigned char) (this_count >> 16) & 0xff;
		SCpnt->cmnd[12] = (unsigned char) (this_count >> 8) & 0xff;
		SCpnt->cmnd[13] = (unsigned char) this_count & 0xff;
		SCpnt->cmnd[14] = SCpnt->cmnd[15] = 0;
	} else if ((this_count > 0xff) || (block > 0x1fffff) ||
		   scsi_device_protection(SCpnt->device) ||
		   SCpnt->device->use_10_for_rw) {
		if (this_count > 0xffff)
			this_count = 0xffff;

		SCpnt->cmnd[0] += READ_10 - READ_6;
		SCpnt->cmnd[1] = protect | (blk_fua_rq(rq) ? 0x8 : 0);
		SCpnt->cmnd[2] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[3] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[4] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[5] = (unsigned char) block & 0xff;
		SCpnt->cmnd[6] = SCpnt->cmnd[9] = 0;
		SCpnt->cmnd[7] = (unsigned char) (this_count >> 8) & 0xff;
		SCpnt->cmnd[8] = (unsigned char) this_count & 0xff;
	} else {
		if (unlikely(blk_fua_rq(rq))) {
			 
			scmd_printk(KERN_ERR, SCpnt,
				    "FUA write on READ/WRITE(6) drive\n");
			goto out;
		}

		SCpnt->cmnd[1] |= (unsigned char) ((block >> 16) & 0x1f);
		SCpnt->cmnd[2] = (unsigned char) ((block >> 8) & 0xff);
		SCpnt->cmnd[3] = (unsigned char) block & 0xff;
		SCpnt->cmnd[4] = (unsigned char) this_count;
		SCpnt->cmnd[5] = 0;
	}
	SCpnt->sdb.length = this_count * sdp->sector_size;

	if (host_dif || scsi_prot_sg_count(SCpnt))
		sd_prot_op(SCpnt, host_dif);

	SCpnt->transfersize = sdp->sector_size;
	SCpnt->underflow = this_count << 9;
	SCpnt->allowed = SD_MAX_RETRIES;

	ret = BLKPREP_OK;
 out:
	return scsi_prep_return(q, rq, ret);
}

static int sd_open(struct block_device *bdev, fmode_t mode)
{
	struct scsi_disk *sdkp = scsi_disk_get(bdev->bd_disk);
	struct scsi_device *sdev;
	int retval;

	if (!sdkp)
		return -ENXIO;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp, "sd_open\n"));

	sdev = sdkp->device;

	retval = -ENXIO;
	if (!scsi_block_when_processing_errors(sdev))
		goto error_out;

	if (sdev->removable || sdkp->write_prot)
		check_disk_change(bdev);

	retval = -ENOMEDIUM;
	if (sdev->removable && !sdkp->media_present && !(mode & FMODE_NDELAY))
		goto error_out;

	retval = -EROFS;
	if (sdkp->write_prot && (mode & FMODE_WRITE))
		goto error_out;

	retval = -ENXIO;
	if (!scsi_device_online(sdev))
		goto error_out;

	if (!sdkp->openers++ && sdev->removable) {
		if (scsi_block_when_processing_errors(sdev))
			scsi_set_medium_removal(sdev, SCSI_REMOVAL_PREVENT);
	}

	return 0;

error_out:
	scsi_disk_put(sdkp);
	return retval;	
}

static int sd_release(struct gendisk *disk, fmode_t mode)
{
	struct scsi_disk *sdkp = scsi_disk(disk);
	struct scsi_device *sdev = sdkp->device;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp, "sd_release\n"));

	if (!--sdkp->openers && sdev->removable) {
		if (scsi_block_when_processing_errors(sdev))
			scsi_set_medium_removal(sdev, SCSI_REMOVAL_ALLOW);
	}

	scsi_disk_put(sdkp);
	return 0;
}

static int sd_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct scsi_disk *sdkp = scsi_disk(bdev->bd_disk);
	struct scsi_device *sdp = sdkp->device;
	struct Scsi_Host *host = sdp->host;
	int diskinfo[4];

        diskinfo[0] = 0x40;	 
       	diskinfo[1] = 0x20;	 
       	diskinfo[2] = sdkp->capacity >> 11;
	
	if (host->hostt->bios_param)
		host->hostt->bios_param(sdp, bdev, sdkp->capacity, diskinfo);
	else
		scsicam_bios_param(bdev, sdkp->capacity, diskinfo);

	geo->heads = diskinfo[0];
	geo->sectors = diskinfo[1];
	geo->cylinders = diskinfo[2];
	return 0;
}

#ifdef MY_ABC_HERE
static int ScsiSetBadSector(struct gendisk *pDisk, SDBADSECTORS *pSectors)
{
	int iDrive = SynoGetInternalDiskSeq(pDisk->disk_name);
	int max_support_disk = sizeof(grgSdBadSectors)/sizeof(SDBADSECTORS);

	if (pSectors == NULL) {
		return -EINVAL;
	}
	gBadSectorTest = 1;
	if (iDrive >= 0 &&
		iDrive < max_support_disk) {

		if (copy_from_user(&grgSdBadSectors[iDrive], pSectors, sizeof(SDBADSECTORS))) {
			return -EINVAL;
		}
		if (grgSdBadSectors[iDrive].uiEnable) {
			int i;
			for (i = 0; i < 100; i++) {
				printk("%s[%d]:%s set bad sector: %u, max support disk: %d\n",
					   __FILE__, __LINE__, pDisk->disk_name,
					   grgSdBadSectors[iDrive].rgSectors[i], max_support_disk);
				if (grgSdBadSectors[iDrive].rgSectors[i] == 0xFFFFFFFF) {
					break;
				}
			}
		}
		return 0;
	} else {
		return -EINVAL;
	}
}
#endif

static int sd_ioctl(struct block_device *bdev, fmode_t mode,
		    unsigned int cmd, unsigned long arg)
{
	struct gendisk *disk = bdev->bd_disk;
	struct scsi_device *sdp = scsi_disk(disk)->device;
	void __user *p = (void __user *)arg;
	int error;
    
	SCSI_LOG_IOCTL(1, printk("sd_ioctl: disk=%s, cmd=0x%x\n",
						disk->disk_name, cmd));

	error = scsi_nonblockable_ioctl(sdp, cmd, p,
					(mode & FMODE_NDELAY) != 0);
	if (!scsi_block_when_processing_errors(sdp) || !error)
		return error;

	switch (cmd) {
		case SCSI_IOCTL_GET_IDLUN:
		case SCSI_IOCTL_GET_BUS_NUMBER:
			return scsi_ioctl(sdp, cmd, p);
#ifdef MY_ABC_HERE
		case SCSI_IOCTL_SET_BADSECTORS:
			return ScsiSetBadSector(disk, p);
#endif
#ifdef MY_ABC_HERE
		case SD_IOCTL_IDLE:
		{
			return (jiffies - sdp->idle) / HZ + 1;
		}
		case SD_IOCTL_SUPPORT_SLEEP:
		{
			int *pCanSleep = (int *)arg;
			*pCanSleep = sdp->nospindown ? 0 : 1;
			return 0;
		}
#endif  
		default:
			error = scsi_cmd_ioctl(disk->queue, disk, mode, cmd, p);
			if (error != -ENOTTY)
				return error;
	}
	return scsi_ioctl(sdp, cmd, p);
}

static void set_media_not_present(struct scsi_disk *sdkp)
{
	sdkp->media_present = 0;
	sdkp->capacity = 0;
	sdkp->device->changed = 1;
}

static int sd_media_changed(struct gendisk *disk)
{
	struct scsi_disk *sdkp = scsi_disk(disk);
	struct scsi_device *sdp = sdkp->device;
	struct scsi_sense_hdr *sshdr = NULL;
	int retval;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp, "sd_media_changed\n"));

	if (!sdp->removable)
		return 0;

	if (!scsi_device_online(sdp)) {
		set_media_not_present(sdkp);
		retval = 1;
		goto out;
	}

	retval = -ENODEV;

	if (scsi_block_when_processing_errors(sdp)) {
		sshdr  = kzalloc(sizeof(*sshdr), GFP_KERNEL);
		retval = scsi_test_unit_ready(sdp, SD_TIMEOUT, SD_MAX_RETRIES,
					      sshdr);
	}

	if (retval || (scsi_sense_valid(sshdr) &&
		        
		       sshdr->asc == 0x3a)) {
		set_media_not_present(sdkp);
		retval = 1;
		goto out;
	}

	sdkp->media_present = 1;

	retval = sdp->changed;
	sdp->changed = 0;
out:
	if (retval != sdkp->previous_state)
		sdev_evt_send_simple(sdp, SDEV_EVT_MEDIA_CHANGE, GFP_KERNEL);
	sdkp->previous_state = retval;
	kfree(sshdr);
	return retval;
}

static int sd_sync_cache(struct scsi_disk *sdkp)
{
	int retries, res;
	struct scsi_device *sdp = sdkp->device;
	struct scsi_sense_hdr sshdr;

	if (!scsi_device_online(sdp))
		return -ENODEV;

	for (retries = 3; retries > 0; --retries) {
		unsigned char cmd[10] = { 0 };

		cmd[0] = SYNCHRONIZE_CACHE;
		 
		res = scsi_execute_req(sdp, cmd, DMA_NONE, NULL, 0, &sshdr,
				       SD_TIMEOUT, SD_MAX_RETRIES, NULL);
		if (res == 0)
			break;
	}

	if (res) {
		sd_print_result(sdkp, res);
		if (driver_byte(res) & DRIVER_SENSE)
			sd_print_sense_hdr(sdkp, &sshdr);
	}

	if (res)
		return -EIO;
	return 0;
}

static void sd_prepare_flush(struct request_queue *q, struct request *rq)
{
	rq->cmd_type = REQ_TYPE_BLOCK_PC;
	rq->timeout = SD_TIMEOUT;
#ifdef CONFIG_ARCH_FEROCEON
	rq->retries = SD_MAX_RETRIES;
#endif
	rq->cmd[0] = SYNCHRONIZE_CACHE;
	rq->cmd_len = 10;
}

static void sd_rescan(struct device *dev)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);

	if (sdkp) {
		revalidate_disk(sdkp->disk);
		scsi_disk_put(sdkp);
	}
}

#ifdef CONFIG_COMPAT
 
static int sd_compat_ioctl(struct block_device *bdev, fmode_t mode,
			   unsigned int cmd, unsigned long arg)
{
	struct scsi_device *sdev = scsi_disk(bdev->bd_disk)->device;

	if (!scsi_block_when_processing_errors(sdev))
		return -ENODEV;
	       
	if (sdev->host->hostt->compat_ioctl) {
		int ret;

		ret = sdev->host->hostt->compat_ioctl(sdev, cmd, (void __user *)arg);

		return ret;
	}

	return -ENOIOCTLCMD; 
}
#endif

static const struct block_device_operations sd_fops = {
	.owner			= THIS_MODULE,
	.open			= sd_open,
	.release		= sd_release,
	.locked_ioctl		= sd_ioctl,
	.getgeo			= sd_getgeo,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= sd_compat_ioctl,
#endif
	.media_changed		= sd_media_changed,
	.revalidate_disk	= sd_revalidate_disk,
};

static unsigned int sd_completed_bytes(struct scsi_cmnd *scmd)
{
	u64 start_lba = blk_rq_pos(scmd->request);
	u64 end_lba = blk_rq_pos(scmd->request) + (scsi_bufflen(scmd) / 512);
	u64 bad_lba;
	int info_valid;

	if (!blk_fs_request(scmd->request))
		return 0;

	info_valid = scsi_get_sense_info_fld(scmd->sense_buffer,
					     SCSI_SENSE_BUFFERSIZE,
					     &bad_lba);
	if (!info_valid)
		return 0;

	if (scsi_bufflen(scmd) <= scmd->device->sector_size)
		return 0;

	if (scmd->device->sector_size < 512) {
		 
		start_lba <<= 1;
		end_lba <<= 1;
	} else {
		 
		u64 factor = scmd->device->sector_size / 512;
		do_div(start_lba, factor);
		do_div(end_lba, factor);
	}

	if (bad_lba < start_lba  || bad_lba >= end_lba)
		return 0;

	return (bad_lba - start_lba) * scmd->device->sector_size;
}

static int sd_done(struct scsi_cmnd *SCpnt)
{
	int result = SCpnt->result;
	unsigned int good_bytes = result ? 0 : scsi_bufflen(SCpnt);
	struct scsi_sense_hdr sshdr;
	struct scsi_disk *sdkp = scsi_disk(SCpnt->request->rq_disk);
	int sense_valid = 0;
	int sense_deferred = 0;

	if (result) {
		sense_valid = scsi_command_normalize_sense(SCpnt, &sshdr);
		if (sense_valid)
			sense_deferred = scsi_sense_is_deferred(&sshdr);
	}
#ifdef CONFIG_SCSI_LOGGING
	SCSI_LOG_HLCOMPLETE(1, scsi_print_result(SCpnt));
	if (sense_valid) {
		SCSI_LOG_HLCOMPLETE(1, scmd_printk(KERN_INFO, SCpnt,
						   "sd_done: sb[respc,sk,asc,"
						   "ascq]=%x,%x,%x,%x\n",
						   sshdr.response_code,
						   sshdr.sense_key, sshdr.asc,
						   sshdr.ascq));
	}
#endif
	if (driver_byte(result) != DRIVER_SENSE &&
	    (!sense_valid || sense_deferred))
		goto out;

	switch (sshdr.sense_key) {
	case HARDWARE_ERROR:
	case MEDIUM_ERROR:
		good_bytes = sd_completed_bytes(SCpnt);
		break;
	case RECOVERED_ERROR:
		good_bytes = scsi_bufflen(SCpnt);
		break;
	case NO_SENSE:
		 
		scsi_print_sense("sd", SCpnt);
		SCpnt->result = 0;
		memset(SCpnt->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
		break;
	case ABORTED_COMMAND:
		if (sshdr.asc == 0x10) {  
			scsi_print_result(SCpnt);
			scsi_print_sense("sd", SCpnt);
			good_bytes = sd_completed_bytes(SCpnt);
		}
		break;
	case ILLEGAL_REQUEST:
		if (sshdr.asc == 0x10) {  
			scsi_print_result(SCpnt);
			scsi_print_sense("sd", SCpnt);
			good_bytes = sd_completed_bytes(SCpnt);
		}
		break;
	default:
		break;
	}
 out:
	if (rq_data_dir(SCpnt->request) == READ && scsi_prot_sg_count(SCpnt))
		sd_dif_complete(SCpnt, good_bytes);

	if (scsi_host_dif_capable(sdkp->device->host, sdkp->protection_type)
	    == SD_DIF_TYPE2_PROTECTION && SCpnt->cmnd != SCpnt->request->cmd)
		mempool_free(SCpnt->cmnd, sd_cdb_pool);

	return good_bytes;
}

static int media_not_present(struct scsi_disk *sdkp,
			     struct scsi_sense_hdr *sshdr)
{

	if (!scsi_sense_valid(sshdr))
		return 0;
	 
	if (sshdr->sense_key != NOT_READY &&
	    sshdr->sense_key != UNIT_ATTENTION)
		return 0;
	if (sshdr->asc != 0x3A)  
		return 0;

	set_media_not_present(sdkp);
	return 1;
}

static void
sd_spinup_disk(struct scsi_disk *sdkp)
{
	unsigned char cmd[10];
	unsigned long spintime_expire = 0;
	int retries, spintime;
	unsigned int the_result;
	struct scsi_sense_hdr sshdr;
	int sense_valid = 0;

	spintime = 0;

	do {
		retries = 0;

		do {
			cmd[0] = TEST_UNIT_READY;
			memset((void *) &cmd[1], 0, 9);

			the_result = scsi_execute_req(sdkp->device, cmd,
						      DMA_NONE, NULL, 0,
						      &sshdr, SD_TIMEOUT,
						      SD_MAX_RETRIES, NULL);

			if (media_not_present(sdkp, &sshdr))
				return;

			if (the_result)
				sense_valid = scsi_sense_valid(&sshdr);
			retries++;
		} while (retries < 3 && 
			 (!scsi_status_is_good(the_result) ||
			  ((driver_byte(the_result) & DRIVER_SENSE) &&
			  sense_valid && sshdr.sense_key == UNIT_ATTENTION)));

		if ((driver_byte(the_result) & DRIVER_SENSE) == 0) {
			 
			if(!spintime && !scsi_status_is_good(the_result)) {
				sd_printk(KERN_NOTICE, sdkp, "Unit Not Ready\n");
				sd_print_result(sdkp, the_result);
			}
			break;
		}
					
		if (sdkp->device->no_start_on_add)
			break;

		if (sense_valid && sshdr.sense_key == NOT_READY) {
			if (sshdr.asc == 4 && sshdr.ascq == 3)
				break;	 
			if (sshdr.asc == 4 && sshdr.ascq == 0xb)
				break;	 
			if (sshdr.asc == 4 && sshdr.ascq == 0xc)
				break;	 
			 
			if (!spintime) {
				sd_printk(KERN_NOTICE, sdkp, "Spinning up disk...");
				cmd[0] = START_STOP;
				cmd[1] = 1;	 
				memset((void *) &cmd[2], 0, 8);
				cmd[4] = 1;	 
				if (sdkp->device->start_stop_pwr_cond)
					cmd[4] |= 1 << 4;
				scsi_execute_req(sdkp->device, cmd, DMA_NONE,
						 NULL, 0, &sshdr,
						 SD_TIMEOUT, SD_MAX_RETRIES,
						 NULL);
				spintime_expire = jiffies + 100 * HZ;
				spintime = 1;
			}
			 
			msleep(1000);
			printk(".");

		} else if (sense_valid &&
				sshdr.sense_key == UNIT_ATTENTION &&
				sshdr.asc == 0x28) {
			if (!spintime) {
				spintime_expire = jiffies + 5 * HZ;
				spintime = 1;
			}
			 
			msleep(1000);
		} else {
			 
			if(!spintime) {
				sd_printk(KERN_NOTICE, sdkp, "Unit Not Ready\n");
				sd_print_sense_hdr(sdkp, &sshdr);
			}
			break;
		}
				
	} while (spintime && time_before_eq(jiffies, spintime_expire));

	if (spintime) {
		if (scsi_status_is_good(the_result))
			printk("ready\n");
		else
			printk("not responding...\n");
	}
}

void sd_read_protection_type(struct scsi_disk *sdkp, unsigned char *buffer)
{
	struct scsi_device *sdp = sdkp->device;
	u8 type;

	if (scsi_device_protection(sdp) == 0 || (buffer[12] & 1) == 0)
		return;

	type = ((buffer[12] >> 1) & 7) + 1;  

	if (type == sdkp->protection_type || !sdkp->first_scan)
		return;

	sdkp->protection_type = type;

	if (type > SD_DIF_TYPE3_PROTECTION) {
		sd_printk(KERN_ERR, sdkp, "formatted with unsupported "	\
			  "protection type %u. Disabling disk!\n", type);
		sdkp->capacity = 0;
		return;
	}

	if (scsi_host_dif_capable(sdp->host, type))
		sd_printk(KERN_NOTICE, sdkp,
			  "Enabling DIF Type %u protection\n", type);
	else
		sd_printk(KERN_NOTICE, sdkp,
			  "Disabling DIF Type %u protection\n", type);
}

static void read_capacity_error(struct scsi_disk *sdkp, struct scsi_device *sdp,
			struct scsi_sense_hdr *sshdr, int sense_valid,
			int the_result)
{
	sd_print_result(sdkp, the_result);
	if (driver_byte(the_result) & DRIVER_SENSE)
		sd_print_sense_hdr(sdkp, sshdr);
	else
		sd_printk(KERN_NOTICE, sdkp, "Sense not available.\n");

	if (sdp->removable &&
	    sense_valid && sshdr->sense_key == NOT_READY)
		sdp->changed = 1;

	sdkp->capacity = 0;  
}

#define RC16_LEN 32
#if RC16_LEN > SD_BUF_SIZE
#error RC16_LEN must not be more than SD_BUF_SIZE
#endif

static int read_capacity_16(struct scsi_disk *sdkp, struct scsi_device *sdp,
						unsigned char *buffer)
{
	unsigned char cmd[16];
	struct scsi_sense_hdr sshdr;
	int sense_valid = 0;
	int the_result;
	int retries = 3;
	unsigned int alignment;
	unsigned long long lba;
	unsigned sector_size;

	do {
		memset(cmd, 0, 16);
		cmd[0] = SERVICE_ACTION_IN;
		cmd[1] = SAI_READ_CAPACITY_16;
		cmd[13] = RC16_LEN;
		memset(buffer, 0, RC16_LEN);

		the_result = scsi_execute_req(sdp, cmd, DMA_FROM_DEVICE,
					buffer, RC16_LEN, &sshdr,
					SD_TIMEOUT, SD_MAX_RETRIES, NULL);

		if (media_not_present(sdkp, &sshdr))
			return -ENODEV;

		if (the_result) {
			sense_valid = scsi_sense_valid(&sshdr);
			if (sense_valid &&
			    sshdr.sense_key == ILLEGAL_REQUEST &&
			    (sshdr.asc == 0x20 || sshdr.asc == 0x24) &&
			    sshdr.ascq == 0x00)
				 
				return -EINVAL;
		}
		retries--;

	} while (the_result && retries);

	if (the_result) {
		sd_printk(KERN_NOTICE, sdkp, "READ CAPACITY(16) failed\n");
		read_capacity_error(sdkp, sdp, &sshdr, sense_valid, the_result);
		return -EINVAL;
	}

	sector_size = get_unaligned_be32(&buffer[8]);
	lba = get_unaligned_be64(&buffer[0]);

	sd_read_protection_type(sdkp, buffer);

	if ((sizeof(sdkp->capacity) == 4) && (lba >= 0xffffffffULL)) {
		sd_printk(KERN_ERR, sdkp, "Too big for this kernel. Use a "
			"kernel compiled with support for large block "
			"devices.\n");
		sdkp->capacity = 0;
		return -EOVERFLOW;
	}

	sdkp->hw_sector_size = (1 << (buffer[13] & 0xf)) * sector_size;

	alignment = ((buffer[14] & 0x3f) << 8 | buffer[15]) * sector_size;
	blk_queue_alignment_offset(sdp->request_queue, alignment);
	if (alignment && sdkp->first_scan)
		sd_printk(KERN_NOTICE, sdkp,
			  "physical block alignment offset: %u\n", alignment);

	sdkp->capacity = lba + 1;
	return sector_size;
}

static int read_capacity_10(struct scsi_disk *sdkp, struct scsi_device *sdp,
						unsigned char *buffer)
{
	unsigned char cmd[16];
	struct scsi_sense_hdr sshdr;
	int sense_valid = 0;
	int the_result;
	int retries = 3;
	sector_t lba;
	unsigned sector_size;

	do {
		cmd[0] = READ_CAPACITY;
		memset(&cmd[1], 0, 9);
		memset(buffer, 0, 8);

		the_result = scsi_execute_req(sdp, cmd, DMA_FROM_DEVICE,
					buffer, 8, &sshdr,
					SD_TIMEOUT, SD_MAX_RETRIES, NULL);

		if (media_not_present(sdkp, &sshdr))
			return -ENODEV;

		if (the_result)
			sense_valid = scsi_sense_valid(&sshdr);
		retries--;

	} while (the_result && retries);

	if (the_result) {
		sd_printk(KERN_NOTICE, sdkp, "READ CAPACITY failed\n");
		read_capacity_error(sdkp, sdp, &sshdr, sense_valid, the_result);
		return -EINVAL;
	}

	sector_size = get_unaligned_be32(&buffer[4]);
	lba = get_unaligned_be32(&buffer[0]);

	if ((sizeof(sdkp->capacity) == 4) && (lba == 0xffffffff)) {
		sd_printk(KERN_ERR, sdkp, "Too big for this kernel. Use a "
			"kernel compiled with support for large block "
			"devices.\n");
		sdkp->capacity = 0;
		return -EOVERFLOW;
	}

	sdkp->capacity = lba + 1;
	sdkp->hw_sector_size = sector_size;
	return sector_size;
}

static int sd_try_rc16_first(struct scsi_device *sdp)
{
	if (sdp->scsi_level > SCSI_SPC_2)
		return 1;
	if (scsi_device_protection(sdp))
		return 1;
	return 0;
}

static void
sd_read_capacity(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int sector_size;
	struct scsi_device *sdp = sdkp->device;
	sector_t old_capacity = sdkp->capacity;

	if (sd_try_rc16_first(sdp)) {
		sector_size = read_capacity_16(sdkp, sdp, buffer);
		if (sector_size == -EOVERFLOW)
			goto got_data;
		if (sector_size == -ENODEV)
			return;
		if (sector_size < 0)
			sector_size = read_capacity_10(sdkp, sdp, buffer);
		if (sector_size < 0)
			return;
	} else {
		sector_size = read_capacity_10(sdkp, sdp, buffer);
		if (sector_size == -EOVERFLOW)
			goto got_data;
		if (sector_size < 0)
			return;
		if ((sizeof(sdkp->capacity) > 4) &&
		    (sdkp->capacity > 0xffffffffULL)) {
			int old_sector_size = sector_size;
			sd_printk(KERN_NOTICE, sdkp, "Very big device. "
					"Trying to use READ CAPACITY(16).\n");
			sector_size = read_capacity_16(sdkp, sdp, buffer);
			if (sector_size < 0) {
				sd_printk(KERN_NOTICE, sdkp,
					"Using 0xffffffff as device size\n");
				sdkp->capacity = 1 + (sector_t) 0xffffffff;
				sector_size = old_sector_size;
				goto got_data;
			}
		}
	}

	if (sdp->fix_capacity ||
	    (sdp->guess_capacity && (sdkp->capacity & 0x01))) {
		sd_printk(KERN_INFO, sdkp, "Adjusting the sector count "
				"from its reported value: %llu\n",
				(unsigned long long) sdkp->capacity);
		--sdkp->capacity;
	}

got_data:
	if (sector_size == 0) {
		sector_size = 512;
		sd_printk(KERN_NOTICE, sdkp, "Sector size 0 reported, "
			  "assuming 512.\n");
	}

	if (sector_size != 512 &&
	    sector_size != 1024 &&
	    sector_size != 2048 &&
	    sector_size != 4096 &&
	    sector_size != 256) {
		sd_printk(KERN_NOTICE, sdkp, "Unsupported sector size %d.\n",
			  sector_size);
		 
		sdkp->capacity = 0;
		 
		sector_size = 512;
	}
	blk_queue_logical_block_size(sdp->request_queue, sector_size);

	{
		char cap_str_2[10], cap_str_10[10];
		u64 sz = (u64)sdkp->capacity << ilog2(sector_size);

		string_get_size(sz, STRING_UNITS_2, cap_str_2,
				sizeof(cap_str_2));
		string_get_size(sz, STRING_UNITS_10, cap_str_10,
				sizeof(cap_str_10));

		if (sdkp->first_scan || old_capacity != sdkp->capacity) {
			sd_printk(KERN_NOTICE, sdkp,
				  "%llu %d-byte logical blocks: (%s/%s)\n",
				  (unsigned long long)sdkp->capacity,
				  sector_size, cap_str_10, cap_str_2);

			if (sdkp->hw_sector_size != sector_size)
				sd_printk(KERN_NOTICE, sdkp,
					  "%u-byte physical blocks\n",
					  sdkp->hw_sector_size);
		}
	}

	if (sector_size == 4096)
		sdkp->capacity <<= 3;
	else if (sector_size == 2048)
		sdkp->capacity <<= 2;
	else if (sector_size == 1024)
		sdkp->capacity <<= 1;
	else if (sector_size == 256)
		sdkp->capacity >>= 1;

	blk_queue_physical_block_size(sdp->request_queue, sdkp->hw_sector_size);
	sdkp->device->sector_size = sector_size;
}

static inline int
sd_do_mode_sense(struct scsi_device *sdp, int dbd, int modepage,
		 unsigned char *buffer, int len, struct scsi_mode_data *data,
		 struct scsi_sense_hdr *sshdr)
{
	return scsi_mode_sense(sdp, dbd, modepage, buffer, len,
			       SD_TIMEOUT, SD_MAX_RETRIES, data,
			       sshdr);
}

static void
sd_read_write_protect_flag(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int res;
	struct scsi_device *sdp = sdkp->device;
	struct scsi_mode_data data;
	int old_wp = sdkp->write_prot;

	set_disk_ro(sdkp->disk, 0);
	if (sdp->skip_ms_page_3f) {
		sd_printk(KERN_NOTICE, sdkp, "Assuming Write Enabled\n");
		return;
	}

	if (sdp->use_192_bytes_for_3f) {
		res = sd_do_mode_sense(sdp, 0, 0x3F, buffer, 192, &data, NULL);
	} else {
		 
		res = sd_do_mode_sense(sdp, 0, 0x3F, buffer, 4, &data, NULL);

		if (!scsi_status_is_good(res))
			res = sd_do_mode_sense(sdp, 0, 0, buffer, 4, &data, NULL);

		if (!scsi_status_is_good(res))
			res = sd_do_mode_sense(sdp, 0, 0x3F, buffer, 255,
					       &data, NULL);
	}

	if (!scsi_status_is_good(res)) {
		sd_printk(KERN_WARNING, sdkp,
			  "Test WP failed, assume Write Enabled\n");
	} else {
		sdkp->write_prot = ((data.device_specific & 0x80) != 0);
		set_disk_ro(sdkp->disk, sdkp->write_prot);
		if (sdkp->first_scan || old_wp != sdkp->write_prot) {
			sd_printk(KERN_NOTICE, sdkp, "Write Protect is %s\n",
				  sdkp->write_prot ? "on" : "off");
			sd_printk(KERN_DEBUG, sdkp,
				  "Mode Sense: %02x %02x %02x %02x\n",
				  buffer[0], buffer[1], buffer[2], buffer[3]);
		}
	}
}

static void
sd_read_cache_type(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int len = 0, res;
	struct scsi_device *sdp = sdkp->device;

	int dbd;
	int modepage;
	struct scsi_mode_data data;
	struct scsi_sense_hdr sshdr;
	int old_wce = sdkp->WCE;
	int old_rcd = sdkp->RCD;
	int old_dpofua = sdkp->DPOFUA;

	if (sdp->skip_ms_page_8)
		goto defaults;

	if (sdp->type == TYPE_RBC) {
		modepage = 6;
		dbd = 8;
	} else {
		modepage = 8;
		dbd = 0;
	}

	res = sd_do_mode_sense(sdp, dbd, modepage, buffer, 4, &data, &sshdr);

	if (!scsi_status_is_good(res))
		goto bad_sense;

	if (!data.header_length) {
		modepage = 6;
		sd_printk(KERN_ERR, sdkp, "Missing header in MODE_SENSE response\n");
	}

	len = data.length;

	if (len < 3)
		goto bad_sense;
	if (len > 20)
		len = 20;

	len += data.header_length + data.block_descriptor_length;
	if (len > SD_BUF_SIZE)
		goto bad_sense;

	res = sd_do_mode_sense(sdp, dbd, modepage, buffer, len, &data, &sshdr);

	if (scsi_status_is_good(res)) {
		int offset = data.header_length + data.block_descriptor_length;

		if (offset >= SD_BUF_SIZE - 2) {
			sd_printk(KERN_ERR, sdkp, "Malformed MODE SENSE response\n");
			goto defaults;
		}

		if ((buffer[offset] & 0x3f) != modepage) {
			sd_printk(KERN_ERR, sdkp, "Got wrong page\n");
			goto defaults;
		}

		if (modepage == 8) {
			sdkp->WCE = ((buffer[offset + 2] & 0x04) != 0);
			sdkp->RCD = ((buffer[offset + 2] & 0x01) != 0);
		} else {
			sdkp->WCE = ((buffer[offset + 2] & 0x01) == 0);
			sdkp->RCD = 0;
		}

		sdkp->DPOFUA = (data.device_specific & 0x10) != 0;
		if (sdkp->DPOFUA && !sdkp->device->use_10_for_rw) {
			sd_printk(KERN_NOTICE, sdkp,
				  "Uses READ/WRITE(6), disabling FUA\n");
			sdkp->DPOFUA = 0;
		}

		if (sdkp->first_scan || old_wce != sdkp->WCE ||
		    old_rcd != sdkp->RCD || old_dpofua != sdkp->DPOFUA)
			sd_printk(KERN_NOTICE, sdkp,
				  "Write cache: %s, read cache: %s, %s\n",
				  sdkp->WCE ? "enabled" : "disabled",
				  sdkp->RCD ? "disabled" : "enabled",
				  sdkp->DPOFUA ? "supports DPO and FUA"
				  : "doesn't support DPO or FUA");

		return;
	}

bad_sense:
	if (scsi_sense_valid(&sshdr) &&
	    sshdr.sense_key == ILLEGAL_REQUEST &&
	    sshdr.asc == 0x24 && sshdr.ascq == 0x0)
		 
		sd_printk(KERN_NOTICE, sdkp, "Cache data unavailable\n");
	else
		sd_printk(KERN_ERR, sdkp, "Asking for cache data failed\n");

defaults:
	sd_printk(KERN_ERR, sdkp, "Assuming drive cache: write through\n");
	sdkp->WCE = 0;
	sdkp->RCD = 0;
	sdkp->DPOFUA = 0;
}

void sd_read_app_tag_own(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int res, offset;
	struct scsi_device *sdp = sdkp->device;
	struct scsi_mode_data data;
	struct scsi_sense_hdr sshdr;

	if (sdp->type != TYPE_DISK)
		return;

	if (sdkp->protection_type == 0)
		return;

	res = scsi_mode_sense(sdp, 1, 0x0a, buffer, 36, SD_TIMEOUT,
			      SD_MAX_RETRIES, &data, &sshdr);

	if (!scsi_status_is_good(res) || !data.header_length ||
	    data.length < 6) {
		sd_printk(KERN_WARNING, sdkp,
			  "getting Control mode page failed, assume no ATO\n");

		if (scsi_sense_valid(&sshdr))
			sd_print_sense_hdr(sdkp, &sshdr);

		return;
	}

	offset = data.header_length + data.block_descriptor_length;

	if ((buffer[offset] & 0x3f) != 0x0a) {
		sd_printk(KERN_ERR, sdkp, "ATO Got wrong page\n");
		return;
	}

	if ((buffer[offset + 5] & 0x80) == 0)
		return;

	sdkp->ATO = 1;

	return;
}

#if defined(MY_ABC_HERE)
 
int
syno_get_ata_identity(struct scsi_device *sdev, u16 *id)
{
	unsigned char scsi_cmd[MAX_COMMAND_SIZE] = {0};

	scsi_cmd[0] = ATA_16;
	scsi_cmd[1] = 0x08;  
	scsi_cmd[2] = 0x0e;  
	scsi_cmd[14] = ATA_CMD_ID_ATA;

	if (scsi_execute_req(sdev, scsi_cmd, DMA_FROM_DEVICE,
		id, 512, NULL, 10 * HZ, 5, NULL)) {
		return 0;
	}

	return 1;
}
EXPORT_SYMBOL(syno_get_ata_identity);
#endif

static void sd_read_block_limits(struct scsi_disk *sdkp)
{
	unsigned int sector_sz = sdkp->device->sector_size;
	char *buffer;

	buffer = scsi_get_vpd_page(sdkp->device, 0xb0);

	if (buffer == NULL)
		return;

	blk_queue_io_min(sdkp->disk->queue,
			 get_unaligned_be16(&buffer[6]) * sector_sz);
	blk_queue_io_opt(sdkp->disk->queue,
			 get_unaligned_be32(&buffer[12]) * sector_sz);

	kfree(buffer);
}

static void sd_read_block_characteristics(struct scsi_disk *sdkp)
{
	char *buffer;
	u16 rot;

	buffer = scsi_get_vpd_page(sdkp->device, 0xb1);

	if (buffer == NULL)
		return;

	rot = get_unaligned_be16(&buffer[4]);

	if (rot == 1)
		queue_flag_set_unlocked(QUEUE_FLAG_NONROT, sdkp->disk->queue);

	kfree(buffer);
}

static int sd_try_extended_inquiry(struct scsi_device *sdp)
{
	 
	if (sdp->scsi_level > SCSI_SPC_2)
		return 1;
	return 0;
}

static int sd_revalidate_disk(struct gendisk *disk)
{
	struct scsi_disk *sdkp = scsi_disk(disk);
	struct scsi_device *sdp = sdkp->device;
	unsigned char *buffer;
	unsigned ordered;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp,
				      "sd_revalidate_disk\n"));

	if (!scsi_device_online(sdp))
		goto out;

	buffer = kmalloc(SD_BUF_SIZE, GFP_KERNEL);
	if (!buffer) {
		sd_printk(KERN_WARNING, sdkp, "sd_revalidate_disk: Memory "
			  "allocation failure.\n");
		goto out;
	}

	sd_spinup_disk(sdkp);

	if (sdkp->media_present) {
		sd_read_capacity(sdkp, buffer);

		if (sd_try_extended_inquiry(sdp)) {
			sd_read_block_limits(sdkp);
			sd_read_block_characteristics(sdkp);
		}

		sd_read_write_protect_flag(sdkp, buffer);
		sd_read_cache_type(sdkp, buffer);
		sd_read_app_tag_own(sdkp, buffer);
	}

	sdkp->first_scan = 0;

	if (sdkp->WCE)
		ordered = sdkp->DPOFUA
			? QUEUE_ORDERED_DRAIN_FUA : QUEUE_ORDERED_DRAIN_FLUSH;
	else
		ordered = QUEUE_ORDERED_DRAIN;

	blk_queue_ordered(sdkp->disk->queue, ordered, sd_prepare_flush);

	set_capacity(disk, sdkp->capacity);
	kfree(buffer);

 out:
	return 0;
}

#ifdef	MY_ABC_HERE
extern int syno_ida_get_new(struct ida *idp, int starting_id, int *id);
#endif
#ifdef	MY_ABC_HERE
int (*SynoUSBDeviceIsSATA)(void *) = NULL;
EXPORT_SYMBOL(SynoUSBDeviceIsSATA);
#endif

#ifdef SYNO_SAS_DISK_NAME
 
static int sd_format_sas_disk_name(char *prefix, int synoindex, char *buf, int buflen)
{
	 
	if (buflen <= (strlen(prefix) + (synoindex + 1)/10 + 1)) {
		return -EINVAL;
	}

	if (snprintf(buf, buflen, "%s%d", prefix, synoindex + 1) <= 0) {
		return -EINVAL;
	}

	return 0;
}
#endif

static int sd_format_disk_name(char *prefix, int index, char *buf, int buflen)
{
	const int base = 'z' - 'a' + 1;
	char *begin = buf + strlen(prefix);
	char *end = buf + buflen;
	char *p;
	int unit;

	p = end - 1;
	*p = '\0';
	unit = base;
	do {
		if (p == begin)
			return -EINVAL;
		*--p = 'a' + (index % unit);
		index = (index / unit) - 1;
	} while (index >= 0);

	memmove(begin, p, end - p);
	memcpy(buf, prefix, strlen(prefix));

	return 0;
}

static void sd_probe_async(void *data, async_cookie_t cookie)
{
	struct scsi_disk *sdkp = data;
	struct scsi_device *sdp;
	struct gendisk *gd;
	u32 index;
	struct device *dev;

	sdp = sdkp->device;
	gd = sdkp->disk;
	index = sdkp->index;
	dev = &sdp->sdev_gendev;

	if (index < SD_MAX_DISKS) {
		gd->major = sd_major((index & 0xf0) >> 4);
		gd->first_minor = ((index & 0xf) << 4) | (index & 0xfff00);
		gd->minors = SD_MINORS;
	}
	gd->fops = &sd_fops;
	gd->private_data = &sdkp->driver;
	gd->queue = sdkp->device->request_queue;

	sdp->sector_size = 512;
	sdkp->capacity = 0;
	sdkp->media_present = 1;
	sdkp->write_prot = 0;
	sdkp->WCE = 0;
	sdkp->RCD = 0;
	sdkp->ATO = 0;
	sdkp->first_scan = 1;

	sd_revalidate_disk(gd);

	blk_queue_prep_rq(sdp->request_queue, sd_prep_fn);

	gd->driverfs_dev = &sdp->sdev_gendev;
	gd->flags = GENHD_FL_EXT_DEVT | GENHD_FL_DRIVERFS;
	if (sdp->removable)
		gd->flags |= GENHD_FL_REMOVABLE;

	dev_set_drvdata(dev, sdkp);
	add_disk(gd);
	sd_dif_config_host(sdkp);

	sd_revalidate_disk(gd);

	sd_printk(KERN_NOTICE, sdkp, "Attached SCSI %sdisk\n",
		  sdp->removable ? "removable " : "");
	put_device(&sdkp->dev);
}

#ifdef MY_ABC_HERE
#ifdef MY_ABC_HERE
static bool syno_find_synoboot(void)
{
	bool find = false;
	struct scsi_disk *sdisk = NULL;
	struct class_dev_iter iter;
	struct device *dev;

	class_dev_iter_init(&iter, &sd_disk_class, NULL, NULL);
	dev = class_dev_iter_next(&iter);
	while (dev) {
		if (!dev->parent) {
			dev = class_dev_iter_next(&iter);
			continue;
		}
		sdisk = dev_get_drvdata(dev->parent);
		if (sdisk && sdisk->disk) {
			if (0 == strcmp(SYNO_USB_FLASH_DEVICE_NAME, sdisk->disk->disk_name)) {
				find = true;
				goto OUT;
			}
		}
		dev = class_dev_iter_next(&iter);
	}
OUT:
	class_dev_iter_exit(&iter);
	return find;
}
#endif

static SYNO_DISK_TYPE syno_disk_type_get(struct device *dev)
{
	struct scsi_device *sdp = to_scsi_device(dev);

#ifdef MY_ABC_HERE
	if(strcmp(sdp->host->hostt->name, "iSCSI Initiator over TCP/IP") == 0){
		return SYNO_DISK_ISCSI;
	}
#endif
	if (SYNO_PORT_TYPE_USB == sdp->host->hostt->syno_port_type) {
#ifdef MY_ABC_HERE
		struct us_data *us = host_to_us(sdp->host);
		struct usb_device *usbdev = us->pusb_dev;
		if (IS_SYNO_USBBOOT_ID_VENDOR(le16_to_cpu(usbdev->descriptor.idVendor)) &&
			IS_SYNO_USBBOOT_ID_PRODUCT(le16_to_cpu(usbdev->descriptor.idProduct)) &&
			gSynoHasDynModule) {
			if (!syno_find_synoboot()) {
				return SYNO_DISK_SYNOBOOT;
			}
		}
#endif
		return SYNO_DISK_USB;
	}

	if (SYNO_PORT_TYPE_SATA == sdp->host->hostt->syno_port_type) {
		 
		return SYNO_DISK_SATA;
	}
	 
	if (SYNO_PORT_TYPE_SAS == sdp->host->hostt->syno_port_type) {
		return SYNO_DISK_SAS;
	}
	return SYNO_DISK_UNKNOWN;
}
#endif

static int sd_probe(struct device *dev)
{
	struct scsi_device *sdp = to_scsi_device(dev);
	struct scsi_disk *sdkp;
	struct gendisk *gd;
	u32 index;
	int error;
#ifdef	MY_ABC_HERE
	struct ata_port *ap;
	int start_index;
	int iRetry = 0;
	u32 want_idx = 0;
#endif
#ifdef	SYNO_SAS_DISK_NAME
	u32 synoidx;
#endif

	error = -ENODEV;
	if (sdp->type != TYPE_DISK && sdp->type != TYPE_MOD && sdp->type != TYPE_RBC)
		goto out;

	SCSI_LOG_HLQUEUE(3, sdev_printk(KERN_INFO, sdp,
					"sd_attach\n"));

	error = -ENOMEM;
	sdkp = kzalloc(sizeof(*sdkp), GFP_KERNEL);
	if (!sdkp)
		goto out;

	gd = alloc_disk(SD_MINORS);
	if (!gd)
		goto out_free;

#ifdef MY_ABC_HERE
	sdkp->synodisktype = syno_disk_type_get(dev);
#endif

	do {
		if (!ida_pre_get(&sd_index_ida, GFP_KERNEL))
			goto out_put;

#ifdef SYNO_SAS_DISK_NAME
		if (1 == g_is_sas_model) {
			switch(sdkp->synodisktype) {
#ifdef MY_ABC_HERE
				case SYNO_DISK_ISCSI:
					if (!ida_pre_get(&iscsi_index_ida, GFP_KERNEL))
						goto out_put;
					break;
#endif
				case SYNO_DISK_USB:
					if (!ida_pre_get(&usb_index_ida, GFP_KERNEL))
						goto out_put;
					break;
				case SYNO_DISK_SAS:
					if (!ida_pre_get(&sas_index_ida, GFP_KERNEL))
						goto out_put;
					break;
				default:
					break;
			}
		}
#endif

#ifdef MY_ABC_HERE
		sdp->idle = jiffies;
		sdp->nospindown = 0;
		sdp->spindown = 0;
#endif  

		spin_lock(&sd_index_lock);

#ifdef MY_ABC_HERE
		switch(sdkp->synodisktype) {
#ifdef MY_ABC_HERE
			case SYNO_DISK_ISCSI:
#ifdef SYNO_SAS_DISK_NAME
				if (1 == g_is_sas_model) {
					error = syno_ida_get_new(&iscsi_index_ida, 0, &synoidx);
					want_idx = 0;
					break;
				}
#endif
					want_idx = SYNO_ISCSI_DEVICE_INDEX;
				break;
#endif
#ifdef MY_ABC_HERE
			case SYNO_DISK_SYNOBOOT:
				want_idx = SYNO_USB_FLASH_DEVICE_INDEX;
				break;
#endif
			case SYNO_DISK_USB:
#ifdef SYNO_SAS_DISK_NAME
				if (1 == g_is_sas_model) {
					error = syno_ida_get_new(&usb_index_ida, 0, &synoidx);
					want_idx = 0;
					break;
				}
#endif
					want_idx = SYNO_MAX_INTERNAL_DISK + 1;
				break;
			case SYNO_DISK_SAS:
			case SYNO_DISK_SATA:
			default:
				 
#ifdef SYNO_SAS_DISK_NAME
				if (1 == g_is_sas_model) {
					error = syno_ida_get_new(&sas_index_ida, 0, &synoidx);
					want_idx = 0;
					break;
				}
#endif
				if (sdp->host->hostt->syno_index_get) {
					want_idx = sdp->host->hostt->syno_index_get(sdp->host, sdp->channel, sdp->id, sdp->lun);
				}else{
					want_idx = sdp->host->host_no;
				}
				break;
		}

		error = syno_ida_get_new(&sd_index_ida, want_idx, &index);
#ifdef SYNO_SAS_DISK_NAME
		if (1 == g_is_sas_model) {
			sdkp->synoindex = synoidx;
		}
#endif
		 
		while (want_idx != index && 
			(SYNO_DISK_SATA == sdkp->synodisktype && iRetry < 15)) {
			 
			printk("want_idx %d index %d. delay and reget\n", want_idx, index);

			ida_remove(&sd_index_ida, index);
			spin_unlock(&sd_index_lock);

			schedule_timeout_uninterruptible(HZ);

			spin_lock(&sd_index_lock);
			error = syno_ida_get_new(&sd_index_ida, want_idx, &index);

			printk("want_idx %d index %d\n", want_idx, index);
			iRetry++;
		}

#else  
		error = ida_get_new(&sd_index_ida, &index);
#endif  
		spin_unlock(&sd_index_lock);
	} while (error == -EAGAIN);

	if (error)
		goto out_put;

#ifdef MY_ABC_HERE
	gd->systemDisk = 0;
	switch(sdkp->synodisktype) {
#ifdef MY_ABC_HERE
		case SYNO_DISK_ISCSI:
#ifdef SYNO_SAS_DISK_NAME
			if (1 == g_is_sas_model) {
				error = sd_format_sas_disk_name(SYNO_SAS_ISCSI_DEVICE_PREFIX, synoidx, gd->disk_name, DISK_NAME_LEN);
				printk("got iSCSI disk[%d]\n", synoidx);
				break;
			}
#endif
			start_index = index - SYNO_ISCSI_DEVICE_INDEX;
			error = sd_format_disk_name(SYNO_ISCSI_DEVICE_PREFIX, start_index, gd->disk_name, DISK_NAME_LEN);
			printk("got iSCSI disk[%d]\n", start_index);
			break;
#endif
#ifdef MY_ABC_HERE
		case SYNO_DISK_SYNOBOOT:
			 
			sprintf(gd->disk_name, SYNO_USB_FLASH_DEVICE_NAME);
			error = 0;
			break;
#endif

		case SYNO_DISK_SAS:
#ifdef SYNO_SAS_DISK_NAME
			error = sd_format_sas_disk_name(SYNO_SAS_DEVICE_PREFIX, synoidx, gd->disk_name, DISK_NAME_LEN);
			 
			if (NULL != dev && 
				 
				NULL != dev->parent && 
				 
				NULL != dev->parent->parent && 
				 
				NULL != dev->parent->parent->parent &&
				 
				NULL != dev->parent->parent->parent->parent && 
				 
				NULL != dev->parent->parent->parent->parent->parent &&
				 
				NULL != dev->parent->parent->parent->parent->parent->parent) {
				 
				if (scsi_is_host_device(dev->parent->parent->parent->parent->parent->parent)) {
					gd->systemDisk = 1;
				}
			}
#endif
			break;
		case SYNO_DISK_SATA:
#ifdef MY_ABC_HERE
			ap = ata_shost_to_port(sdp->host);
			 
			if (NULL != ap && !syno_is_synology_pm(ap)) {
				gd->systemDisk = 1;
			}
#else
			gd->systemDisk = 1;
#endif
			error = sd_format_disk_name(SYNO_SATA_DEVICE_PREFIX, index, gd->disk_name, DISK_NAME_LEN);
			break;
		case SYNO_DISK_USB:
		default:
#ifdef SYNO_SAS_DISK_NAME
			if (1 == g_is_sas_model) {
				error = sd_format_sas_disk_name(SYNO_SAS_USB_DEVICE_PREFIX, synoidx, gd->disk_name, DISK_NAME_LEN);
				break;
			}
#endif
			error = sd_format_disk_name(SYNO_SATA_DEVICE_PREFIX, index, gd->disk_name, DISK_NAME_LEN);
			break;
	}
#else
	error = sd_format_disk_name("sd", index, gd->disk_name, DISK_NAME_LEN);
#endif

	if (error)
		goto out_free_index;

	sdkp->device = sdp;
	sdkp->driver = &sd_template;
	sdkp->disk = gd;
	sdkp->index = index;
	sdkp->openers = 0;
	sdkp->previous_state = 1;

	if (!sdp->request_queue->rq_timeout) {
		if (sdp->type != TYPE_MOD)
			blk_queue_rq_timeout(sdp->request_queue, SD_TIMEOUT);
		else
			blk_queue_rq_timeout(sdp->request_queue,
					     SD_MOD_TIMEOUT);
	}

	device_initialize(&sdkp->dev);
	sdkp->dev.parent = &sdp->sdev_gendev;
	sdkp->dev.class = &sd_disk_class;
	dev_set_name(&sdkp->dev, dev_name(&sdp->sdev_gendev));

	if (device_add(&sdkp->dev))
		goto out_free_index;

	get_device(&sdp->sdev_gendev);

	get_device(&sdkp->dev);	 
	async_schedule(sd_probe_async, sdkp);
#if defined(MY_ABC_HERE) && defined(CONFIG_SYNO_MV88F6281_USBSTATION)
	if (SYNO_DISK_SYNOBOOT != sdkp->synodisktype) {
		extern int SYNO_CTRL_USB_HDD_LED_SET(int status);
		 
		SYNO_CTRL_USB_HDD_LED_SET(0x4);
	}
#endif
#ifdef MY_ABC_HERE
	strlcpy(sdp->syno_disk_name, gd->disk_name, BDEVNAME_SIZE);
#endif

	return 0;

 out_free_index:
	spin_lock(&sd_index_lock);
	ida_remove(&sd_index_ida, index);
#ifdef SYNO_SAS_DISK_NAME
	if (1 == g_is_sas_model) {
		switch(sdkp->synodisktype) {
#ifdef MY_ABC_HERE
			case SYNO_DISK_ISCSI:
				ida_remove(&iscsi_index_ida, synoidx);
				break;
#endif
			case SYNO_DISK_USB:
				ida_remove(&usb_index_ida, synoidx);
				break;
			case SYNO_DISK_SAS:
				ida_remove(&sas_index_ida, synoidx);
				break;
			default:
				break;
		}
	}
#endif
	spin_unlock(&sd_index_lock);
 out_put:
	put_disk(gd);
 out_free:
	kfree(sdkp);
 out:
	return error;
}

static int sd_remove(struct device *dev)
{
	struct scsi_disk *sdkp;

	async_synchronize_full();
	sdkp = dev_get_drvdata(dev);
	blk_queue_prep_rq(sdkp->device->request_queue, scsi_prep_fn);
	device_del(&sdkp->dev);
	del_gendisk(sdkp->disk);
	sd_shutdown(dev);

	mutex_lock(&sd_ref_mutex);
	dev_set_drvdata(dev, NULL);
	put_device(&sdkp->dev);
	mutex_unlock(&sd_ref_mutex);

	return 0;
}

static void scsi_disk_release(struct device *dev)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct gendisk *disk = sdkp->disk;

	spin_lock(&sd_index_lock);
	ida_remove(&sd_index_ida, sdkp->index);
#ifdef SYNO_SAS_DISK_NAME
	if (1 == g_is_sas_model) {
		switch(sdkp->synodisktype) {
#ifdef MY_ABC_HERE
			case SYNO_DISK_ISCSI:
				ida_remove(&iscsi_index_ida, sdkp->synoindex);
				break;
#endif
			case SYNO_DISK_USB:
				ida_remove(&usb_index_ida, sdkp->synoindex);
				break;
			case SYNO_DISK_SAS:
				ida_remove(&sas_index_ida, sdkp->synoindex);
				break;
			default:
				break;
		}
	}
#endif
	spin_unlock(&sd_index_lock);

	disk->private_data = NULL;
	put_disk(disk);
	put_device(&sdkp->device->sdev_gendev);

	kfree(sdkp);
}

static int sd_start_stop_device(struct scsi_disk *sdkp, int start)
{
	unsigned char cmd[6] = { START_STOP };	 
	struct scsi_sense_hdr sshdr;
	struct scsi_device *sdp = sdkp->device;
	int res;

	if (start)
		cmd[4] |= 1;	 

	if (sdp->start_stop_pwr_cond)
		cmd[4] |= start ? 1 << 4 : 3 << 4;	 

	if (!scsi_device_online(sdp))
		return -ENODEV;

	res = scsi_execute_req(sdp, cmd, DMA_NONE, NULL, 0, &sshdr,
			       SD_TIMEOUT, SD_MAX_RETRIES, NULL);
	if (res) {
		sd_printk(KERN_WARNING, sdkp, "START_STOP FAILED\n");
		sd_print_result(sdkp, res);
		if (driver_byte(res) & DRIVER_SENSE)
			sd_print_sense_hdr(sdkp, &sshdr);
	}

	return res;
}

static void sd_shutdown(struct device *dev)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);

	if (!sdkp)
		return;          

	if (sdkp->WCE) {
		sd_printk(KERN_NOTICE, sdkp, "Synchronizing SCSI cache\n");
		sd_sync_cache(sdkp);
	}

	if (system_state != SYSTEM_RESTART && sdkp->device->manage_start_stop) {
		sd_printk(KERN_NOTICE, sdkp, "Stopping disk\n");
		sd_start_stop_device(sdkp, 0);
	}

	scsi_disk_put(sdkp);
}

static int sd_suspend(struct device *dev, pm_message_t mesg)
{
#ifdef CONFIG_SYNO_QORIQ_FIX_DISK_WAKE_BEFORE_DEEPSLEEP
	return 0;
#else
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);
	int ret = 0;

	if (!sdkp)
		return 0;	 

	if (sdkp->WCE) {
		sd_printk(KERN_NOTICE, sdkp, "Synchronizing SCSI cache\n");
		ret = sd_sync_cache(sdkp);
		if (ret)
			goto done;
	}

	if ((mesg.event & PM_EVENT_SLEEP) && sdkp->device->manage_start_stop) {
		sd_printk(KERN_NOTICE, sdkp, "Stopping disk\n");
		ret = sd_start_stop_device(sdkp, 0);
	}

done:
	scsi_disk_put(sdkp);
	return ret;
#endif
}

static int sd_resume(struct device *dev)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);
	int ret = 0;

	if (!sdkp->device->manage_start_stop)
		goto done;

	sd_printk(KERN_NOTICE, sdkp, "Starting disk\n");
	ret = sd_start_stop_device(sdkp, 1);

done:
	scsi_disk_put(sdkp);
	return ret;
}

static int __init init_sd(void)
{
	int majors = 0, i, err;

	SCSI_LOG_HLQUEUE(3, printk("init_sd: sd driver entry point\n"));

	for (i = 0; i < SD_MAJORS; i++)
		if (register_blkdev(sd_major(i), "sd") == 0)
			majors++;

#ifdef MY_ABC_HERE
	gBadSectorTest = 0;
#endif

	if (!majors)
		return -ENODEV;

	err = class_register(&sd_disk_class);
	if (err)
		goto err_out;

	err = scsi_register_driver(&sd_template.gendrv);
	if (err)
		goto err_out_class;

	sd_cdb_cache = kmem_cache_create("sd_ext_cdb", SD_EXT_CDB_SIZE,
					 0, 0, NULL);
	if (!sd_cdb_cache) {
		printk(KERN_ERR "sd: can't init extended cdb cache\n");
		goto err_out_class;
	}

	sd_cdb_pool = mempool_create_slab_pool(SD_MEMPOOL_SIZE, sd_cdb_cache);
	if (!sd_cdb_pool) {
		printk(KERN_ERR "sd: can't init extended cdb pool\n");
		goto err_out_cache;
	}

	return 0;

err_out_cache:
	kmem_cache_destroy(sd_cdb_cache);

err_out_class:
	class_unregister(&sd_disk_class);
err_out:
	for (i = 0; i < SD_MAJORS; i++)
		unregister_blkdev(sd_major(i), "sd");
	return err;
}

static void __exit exit_sd(void)
{
	int i;

	SCSI_LOG_HLQUEUE(3, printk("exit_sd: exiting sd driver\n"));

	mempool_destroy(sd_cdb_pool);
	kmem_cache_destroy(sd_cdb_cache);

	scsi_unregister_driver(&sd_template.gendrv);
	class_unregister(&sd_disk_class);

	for (i = 0; i < SD_MAJORS; i++)
		unregister_blkdev(sd_major(i), "sd");
}

module_init(init_sd);
module_exit(exit_sd);

static void sd_print_sense_hdr(struct scsi_disk *sdkp,
			       struct scsi_sense_hdr *sshdr)
{
	sd_printk(KERN_INFO, sdkp, "");
	scsi_show_sense_hdr(sshdr);
	sd_printk(KERN_INFO, sdkp, "");
	scsi_show_extd_sense(sshdr->asc, sshdr->ascq);
}

static void sd_print_result(struct scsi_disk *sdkp, int result)
{
	sd_printk(KERN_INFO, sdkp, "");
	scsi_show_result(result);
}

#ifdef MY_ABC_HERE
int SynoSCSIGetDeviceIndex(struct block_device *bdev)
{
	struct gendisk *disk = NULL;

	BUG_ON(bdev == NULL);
	disk = bdev->bd_disk;

#ifdef SYNO_SAS_DISK_NAME
	if (g_is_sas_model) {
		return container_of(disk->private_data, struct scsi_disk, driver)->synoindex;
	}
#endif
	return container_of(disk->private_data, struct scsi_disk, driver)->index;
}
EXPORT_SYMBOL(SynoSCSIGetDeviceIndex);
#endif  

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
 
static unsigned char
blIsScsiDevice(int major)
{
	unsigned char ret = 0;

	switch (major) {
	case SCSI_DISK0_MAJOR:
	case SCSI_DISK1_MAJOR ... SCSI_DISK7_MAJOR:
	case SCSI_DISK8_MAJOR ... SCSI_DISK15_MAJOR:
		ret = 1;
		break;
	default:
		break;
	}

	return ret;
}
#endif  

#ifdef MY_ABC_HERE
int
IsDeviceDisappear(struct block_device *bdev)
{
	struct gendisk *disk = NULL;
	struct scsi_disk *sdkp;
	int ret = 0;

	if (!bdev) {
		WARN_ON(bdev == NULL);
		goto END;
	}

	disk = bdev->bd_disk;
	if (!disk) {
		WARN_ON(disk == NULL);
		goto END;
	}

	if (!blIsScsiDevice(disk->major)) {
		 
		printk("This is not a kind of scsi disk %d\n", disk->major);
		goto END;
	}

	sdkp = container_of(disk->private_data, struct scsi_disk, driver);
	if (!sdkp) {
		WARN_ON(!sdkp);
		goto END;
	}

	switch (sdkp->device->sdev_state) {
	case SDEV_OFFLINE:
	case SDEV_DEL:
	case SDEV_CANCEL:
		ret = 1;
		break;
	default:
		break;
	}

END:
	return ret;
}

EXPORT_SYMBOL(IsDeviceDisappear);
#endif  

#ifdef MY_ABC_HERE
 
void
PartitionRemapModeSet(struct gendisk *gd,
					  struct hd_struct *phd,
					  unsigned char blAutoRemap)
{
	struct scsi_disk *sdkp;
	struct scsi_device *sdev;

	if (!gd || !phd) {
		goto END;
	}

	phd->auto_remap = blAutoRemap;
	if (!blAutoRemap) {
		if (!blIsScsiDevice(gd->major)) {
			 
			printk("This is not a kind of scsi disk %d\n", gd->major);
			goto END;
		}

		sdkp = container_of(gd->private_data, struct scsi_disk, driver);
		if (!sdkp) {
			printk(" sdkp is NULL\n");
			goto END;
		}

		sdev = sdkp->device;
		if(!sdev) {
			printk(" sdev is NULL\n");
			goto END;
		}
		sdev->auto_remap = 0;
	}
END:
	return;
}

void
ScsiRemapModeSet(struct scsi_device *sdev,
				 unsigned char blAutoRemap)
{
	struct scsi_disk *sdkp;
	struct gendisk *gd;
	struct hd_struct *phd;
	int i = 0;

	if (!sdev) {
		goto END;
	}

	if (TYPE_DISK != sdev->type) {
		printk("Only support scsi disk\n");
		goto END;
	}

	sdev->auto_remap = blAutoRemap;
	sdkp = dev_get_drvdata(&sdev->sdev_gendev);
	if (!sdkp) {
		goto END;
	}

	gd = sdkp->disk;
	if (!gd) {
		goto END;
	}

	for (i = 0; i < gd->minors; i++) {
		phd = disk_get_part(gd, i+1);
		if (!phd || !phd->nr_sects)
			continue;

		phd->auto_remap = blAutoRemap;
	}
END:
	return;
}

void
RaidRemapModeSet(struct block_device *bdev, unsigned char blAutoRemap)
{
	struct gendisk *disk = NULL;
	struct scsi_disk *sdkp;

	if (!bdev) {
		WARN_ON(bdev == NULL);
		return;
	}

	disk = bdev->bd_disk;
	if (!disk) {
		WARN_ON(disk == NULL);
		return;
	}

	if (!blIsScsiDevice(disk->major)) {
		 
		printk("This is not a kind of scsi disk %d\n", disk->major);
		return;
	}

	if (bdev->bd_part) {
		 
		bdev->bd_part->auto_remap = blAutoRemap;
	} else {
		 
		sdkp = container_of(disk->private_data, struct scsi_disk, driver);
		if (!sdkp) {
			WARN_ON(!sdkp);
			return;
		}
		ScsiRemapModeSet(sdkp->device, blAutoRemap);
	}
}

unsigned char
blSectorNeedAutoRemap(struct scsi_cmnd *scsi_cmd,
					  sector_t lba)
{
	struct scsi_device *sdev;
	struct scsi_disk *sdkp;
	struct gendisk *gd;
	struct hd_struct *phd;
	char szName[BDEVNAME_SIZE];
	sector_t start, end;
	u8 ret = 0;
	int i = 0;

	if (!scsi_cmd) {
		WARN_ON(1);
		goto END;
	}

	sdev = scsi_cmd->device;
	if (!sdev) {
		WARN_ON(1);
		goto END;
	}

	if (TYPE_DISK != sdev->type) {
		printk("Only support scsi disk\n");
		goto END;
	}

	if (sdev->auto_remap) {
		ret = 1;
		printk("%s auto remap is on\n", dev_name(&sdev->sdev_gendev));
		goto END;
	}

	sdkp = dev_get_drvdata(&sdev->sdev_gendev);
	if (!sdkp) {
		goto END;
	}

	gd = sdkp->disk;
	if (!gd) {
		goto END;
	}

	for (i = 0; i < gd->minors; i++) {
		phd = disk_get_part(gd, i+1);
		if (!phd || !phd->nr_sects)
			continue;

		start = phd->start_sect;
		end = phd->nr_sects + start - 1;

		if (lba >= start && lba <= end) {
			printk("lba %llu start %llu end %llu\n", (unsigned long long)lba, (unsigned long long)start, (unsigned long long)end);
			ret = phd->auto_remap;
			printk("%s auto_remap %u\n", disk_name(gd, i+1, szName), phd->auto_remap);
		}
	}
END:
	return ret;
}

EXPORT_SYMBOL(blSectorNeedAutoRemap);
EXPORT_SYMBOL(RaidRemapModeSet);
EXPORT_SYMBOL(ScsiRemapModeSet);
EXPORT_SYMBOL(PartitionRemapModeSet);
#endif  
