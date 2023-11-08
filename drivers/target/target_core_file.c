/*******************************************************************************
 * Filename:  target_core_file.c
 *
 * This file contains the Storage Engine <-> FILEIO transport specific functions
 *
 * Copyright (c) 2005 PyX Technologies, Inc.
 * Copyright (c) 2005-2006 SBE, Inc.  All Rights Reserved.
 * Copyright (c) 2007-2010 Rising Tide Systems
 * Copyright (c) 2008-2010 Linux-iSCSI.org
 *
 * Nicholas A. Bellinger <nab@kernel.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ******************************************************************************/

#include <linux/string.h>
#include <linux/parser.h>
#include <linux/timer.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <scsi/scsi.h>
#include <scsi/scsi_host.h>

#if defined(CONFIG_MACH_QNAPTS) 
/* 2014/06/14, adamhsu, redmine 8530 (start) */
#include <asm/unaligned.h>
#include <target/target_core_fabric.h>
/* 2014/06/14, adamhsu, redmine 8530 (end) */
#endif

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include "target_core_file.h"

#if defined(CONFIG_MACH_QNAPTS) 
#include "vaai_target_struc.h"
#include "target_general.h"




#if defined(SUPPORT_FAST_BLOCK_CLONE)
#include "target_fast_clone.h"
#endif

#if defined(SUPPORT_TP)
#include "tp_def.h"
#if defined(SUPPORT_FILEIO_ON_FILE)
#include <linux/falloc.h>
#endif

/* for threshold notification */
#if defined(QNAP_HAL)
#include <qnap/hal_event.h>
extern int send_hal_netlink(NETLINK_EVT *event);
#endif

#if defined(SUPPORT_VOLUME_BASED)
#include <linux/device-mapper.h>
extern int thin_get_dmtarget(char *name, struct dm_target **result);
extern int thin_get_data_status(struct dm_target *ti, uint64_t *total_size, uint64_t *used_size);
extern int thin_set_dm_monitor(struct dm_target *ti, void *dev, void (*dm_monitor_fn)(void *dev, int));

/* 2014/06/14, adamhsu, redmine 8530 (start) */
extern int thin_get_sectors_per_block(char *name, uint32_t *result);
extern int thin_get_lba_status(char *name, uint64_t index, uint64_t len, uint8_t *result);
/* 2014/06/14, adamhsu, redmine 8530 (end) */

#endif
#endif /* defined(SUPPORT_TP) */

//George Wu, 20130721, blkdev_writepages
#ifdef USE_BLKDEV_WRITEPAGES
#include <linux/genhd.h>        /* GENHD_FL_MPAGE */
#endif
#endif /* defined(CONFIG_MACH_QNAPTS) */


static struct se_subsystem_api fileio_template;


/*	fd_attach_hba(): (Part of se_subsystem_api_t template)
 *
 *
 */
static int fd_attach_hba(struct se_hba *hba, u32 host_id)
{
	struct fd_host *fd_host;

	fd_host = kzalloc(sizeof(struct fd_host), GFP_KERNEL);
	if (!fd_host) {
		pr_err("Unable to allocate memory for struct fd_host\n");
		return -ENOMEM;
	}

	fd_host->fd_host_id = host_id;

	hba->hba_ptr = fd_host;

	pr_debug("CORE_HBA[%d] - TCM FILEIO HBA Driver %s on Generic"
		" Target Core Stack %s\n", hba->hba_id, FD_VERSION,
		TARGET_CORE_MOD_VERSION);
	pr_debug("CORE_HBA[%d] - Attached FILEIO HBA: %u to Generic"
		" MaxSectors: %u\n",
		hba->hba_id, fd_host->fd_host_id, FD_MAX_SECTORS);

	return 0;
}

static void fd_detach_hba(struct se_hba *hba)
{
	struct fd_host *fd_host = hba->hba_ptr;

	pr_debug("CORE_HBA[%d] - Detached FILEIO HBA: %u from Generic"
		" Target Core\n", hba->hba_id, fd_host->fd_host_id);

	kfree(fd_host);
	hba->hba_ptr = NULL;
}

static void *fd_allocate_virtdevice(struct se_hba *hba, const char *name)
{
	struct fd_dev *fd_dev;
	struct fd_host *fd_host = hba->hba_ptr;

	fd_dev = kzalloc(sizeof(struct fd_dev), GFP_KERNEL);
	if (!fd_dev) {
		pr_err("Unable to allocate memory for struct fd_dev\n");
		return NULL;
	}

	fd_dev->fd_host = fd_host;

	pr_debug("FILEIO: Allocated fd_dev for %p\n", name);

	return fd_dev;
}

/*	fd_create_virtdevice(): (Part of se_subsystem_api_t template)
 *
 *
 */
static struct se_device *fd_create_virtdevice(
	struct se_hba *hba,
	struct se_subsystem_dev *se_dev,
	void *p)
{
	struct se_device *dev;
	struct se_dev_limits dev_limits;
	struct queue_limits *limits;
	struct fd_dev *fd_dev = p;
	struct fd_host *fd_host = hba->hba_ptr;
	struct file *file;
	struct inode *inode = NULL;
	int dev_flags = 0, flags, ret = -EINVAL;

	memset(&dev_limits, 0, sizeof(struct se_dev_limits));

	/*
	 * Use O_DSYNC by default instead of O_SYNC to forgo syncing
	 * of pure timestamp updates.
	 */
	flags = O_RDWR | O_CREAT | O_LARGEFILE | O_DSYNC;
	/*
	 * Optionally allow fd_buffered_io=1 to be enabled for people
	 * who know what they are doing w/o O_DSYNC.
	 */
	if (fd_dev->fbd_flags & FDBD_USE_BUFFERED_IO) {
		pr_debug("FILEIO: Disabling O_DSYNC, using buffered FILEIO\n");
		flags &= ~O_DSYNC;
	}
	file = filp_open(fd_dev->fd_dev_name, flags, 0600);
	if (IS_ERR(file)) {
		pr_err("filp_open(%s) failed\n", fd_dev->fd_dev_name);
		ret = PTR_ERR(file);
		goto fail;
	}
	fd_dev->fd_file = file;

	/*
	 * If using a block backend with this struct file, we extract
	 * fd_dev->fd_[block,dev]_size from struct block_device.
	 *
	 * Otherwise, we use the passed fd_size= from configfs
	 */
	inode = file->f_mapping->host;
	if (S_ISBLK(inode->i_mode)) {
		struct request_queue *q;
		unsigned long long dev_size;

//George Wu, 20130721, blkdev_writepages
#ifdef CONFIG_MACH_QNAPTS
#ifdef USE_BLKDEV_WRITEPAGES
		//printk(KERN_DEBUG "[BLKDEV_WRITEPAGES] " "fd_create_virtdevice()@target_core_file.c sets gendisk's flags.\n");
		inode->i_bdev->bd_disk->flags |= GENHD_FL_MPAGE;
#endif
#endif
		/*
		 * Setup the local scope queue_limits from struct request_queue->limits
		 * to pass into transport_add_device_to_core_hba() as struct se_dev_limits.
		 */
		q = bdev_get_queue(inode->i_bdev);
		limits = &dev_limits.limits;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
		/* adamhsu 2013/06/07 
		 * - Support to set the logical block size from NAS GUI.
		 */
		if ((se_dev->su_dev_flags & SDF_USING_QLBS) && se_dev->se_dev_qlbs)
			limits->logical_block_size = se_dev->se_dev_qlbs;
		else
#endif
		limits->logical_block_size = bdev_logical_block_size(inode->i_bdev);


		/* here still keep the 512b unit, we may change
		 * it in se_dev_set_default_attribs() 
		 */
		limits->max_hw_sectors = queue_max_hw_sectors(q);
		limits->max_sectors = queue_max_sectors(q);
		/*
		 * Determine the number of bytes from i_size_read() minus
		 * one (1) logical sector from underlying struct block_device
		 */

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
		/* adamhsu 2013/06/07
		 * - Support to set the logical block size from NAS GUI. 
		 */
		if ((se_dev->su_dev_flags & SDF_USING_QLBS) && se_dev->se_dev_qlbs)
			fd_dev->fd_block_size = se_dev->se_dev_qlbs;
		else
#endif
		fd_dev->fd_block_size = bdev_logical_block_size(inode->i_bdev);

		dev_size = (i_size_read(file->f_mapping->host) -
				       fd_dev->fd_block_size);

		pr_debug("FILEIO: Using size: %llu bytes from struct"
			" block_device blocks: %llu logical_block_size: %d\n",
			dev_size, div_u64(dev_size, fd_dev->fd_block_size),
			fd_dev->fd_block_size);
	} else {
		if (!(fd_dev->fbd_flags & FBDF_HAS_SIZE)) {
			pr_err("FILEIO: Missing fd_dev_size="
				" parameter, and no backing struct"
				" block_device\n");
			goto fail;
		}

		limits = &dev_limits.limits;
		limits->logical_block_size = FD_BLOCKSIZE;

		/* here still keep the 512b unit, we may change
		 * it in se_dev_set_default_attribs() 
		 */
		limits->max_hw_sectors = FD_MAX_SECTORS;
		limits->max_sectors = FD_MAX_SECTORS;
		fd_dev->fd_block_size = FD_BLOCKSIZE;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
		/* 20140421, adamhsu, redmine 8108 
		 * - overwrite original setting if we set the sector size to
		 * 4096b from NAS GUI
		 */
		if ((se_dev->su_dev_flags & SDF_USING_QLBS) && se_dev->se_dev_qlbs){
			limits->logical_block_size = se_dev->se_dev_qlbs;
			fd_dev->fd_block_size = se_dev->se_dev_qlbs;
		}
#endif
	}

	dev_limits.hw_queue_depth = FD_MAX_DEVICE_QUEUE_DEPTH;
	dev_limits.queue_depth = FD_DEVICE_QUEUE_DEPTH;

#ifdef CONFIG_MACH_QNAPTS   // "FILEIO" --> "iSCSI Storage" 
	dev = transport_add_device_to_core_hba(hba, &fileio_template,
				se_dev, dev_flags, fd_dev,
				&dev_limits, "iSCSI Storage", FD_VERSION);
#else
	dev = transport_add_device_to_core_hba(hba, &fileio_template,
				se_dev, dev_flags, fd_dev,
				&dev_limits, "FILEIO", FD_VERSION);
#endif 

	if (!dev)
		goto fail;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TP)
	dev->se_sub_dev->se_dev_attrib.gti = NULL;

	/* To register something if device is thin*/
	if(!strcmp(dev->se_sub_dev->se_dev_provision, "thin"))
		dev->se_sub_dev->se_dev_attrib.emulate_tpu = 1;


	if (S_ISBLK(inode->i_mode)) {
		struct request_queue *q;
		q = bdev_get_queue(inode->i_bdev);

		dev->dev_type = QNAP_DT_FIO_BLK;

		/* FIXED ME !!
		 *
		 * Refer the iblock_create_virtdevice() to do this currently, 
		 * it may be changed in the future
		 */
		if (blk_queue_discard(q)) {
#if defined(CONFIG_MACH_QNAPTS)
#if !defined(DISABLE_FILEIO_DISCARD)
			dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count =
				q->limits.max_discard_sectors;
			dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count <<= MAX_UNMAP_COUNT_SHIFT;
			dev->se_sub_dev->se_dev_attrib.max_unmap_block_desc_count = MAX_UNMAP_DESC_COUNT;


			dev->se_sub_dev->se_dev_attrib.unmap_granularity =
				q->limits.discard_granularity >> 9;
			dev->se_sub_dev->se_dev_attrib.unmap_granularity_alignment =
				q->limits.discard_alignment;

			pr_debug("FD: LIO(File I/O) + Block Backend Discard"
				" support available, disabled by default\n");
#endif
#endif
		}
	}else{

		dev->dev_type = QNAP_DT_FIO_FILE;

		/*  FIXED ME !!
		 *
		 * Refer the loop_config_discard() in loop.c to do this
		 * currently, it may be changed in the future
		 */
		dev->se_sub_dev->se_dev_attrib.max_unmap_lba_count = UINT_MAX >> 9;
		dev->se_sub_dev->se_dev_attrib.max_unmap_block_desc_count = 1;
		dev->se_sub_dev->se_dev_attrib.unmap_granularity = inode->i_sb->s_blocksize;
		dev->se_sub_dev->se_dev_attrib.unmap_granularity_alignment = 0;

		pr_debug("FD: LIO(File I/O) + File Backend Discard support available,"
			" disabled by default\n");
	}

#endif
#endif

	if (fd_dev->fbd_flags & FDBD_USE_BUFFERED_IO) {
		pr_debug("FILEIO: Forcing setting of emulate_write_cache=1"
			" with FDBD_USE_BUFFERED_IO\n");
		dev->se_sub_dev->se_dev_attrib.emulate_write_cache = 1;
	}

	fd_dev->fd_dev_id = fd_host->fd_host_dev_id_count++;
	fd_dev->fd_queue_depth = dev->queue_depth;

#ifdef CONFIG_MACH_QNAPTS // 2009/09/23 Nike Chen change for QNAP ID
#if defined(Athens)
	pr_debug("CORE_FILE[%u] - Added Cisco FILEIO Device ID: %u at %s,"
		" %llu total bytes\n", fd_host->fd_host_id, fd_dev->fd_dev_id,
			fd_dev->fd_dev_name, fd_dev->fd_dev_size);
#elif defined(IS_G)
	pr_debug("CORE_FILE[%u] - Added FILEIO Device ID: %u at %s,"
		" %llu total bytes\n", fd_host->fd_host_id, fd_dev->fd_dev_id,
			fd_dev->fd_dev_name, fd_dev->fd_dev_size);
#else
	pr_debug("CORE_FILE[%u] - Added QNAP FILEIO Device ID: %u at %s,"
		" %llu total bytes\n", fd_host->fd_host_id, fd_dev->fd_dev_id,
			fd_dev->fd_dev_name, fd_dev->fd_dev_size);
#endif /* #if defined(Athens) */
#else
	pr_debug("CORE_FILE[%u] - Added TCM FILEIO Device ID: %u at %s,"
		" %llu total bytes\n", fd_host->fd_host_id, fd_dev->fd_dev_id,
			fd_dev->fd_dev_name, fd_dev->fd_dev_size);
#endif /* #ifdef CONFIG_MACH_QNAPTS */
	return dev;
fail:
	if (fd_dev->fd_file) {
		filp_close(fd_dev->fd_file, NULL);
		fd_dev->fd_file = NULL;
	}
	return ERR_PTR(ret);
}

/*	fd_free_device(): (Part of se_subsystem_api_t template)
 *
 *
 */
static void fd_free_device(void *p)
{
	struct fd_dev *fd_dev = p;

	if (fd_dev->fd_file) {
		filp_close(fd_dev->fd_file, NULL);
		fd_dev->fd_file = NULL;
	}

	kfree(fd_dev);
}

static inline struct fd_request *FILE_REQ(struct se_task *task)
{
	return container_of(task, struct fd_request, fd_task);
}


static struct se_task *
fd_alloc_task(unsigned char *cdb)
{
	struct fd_request *fd_req;

	fd_req = kzalloc(sizeof(struct fd_request), GFP_KERNEL);
	if (!fd_req) {
		pr_err("Unable to allocate struct fd_request\n");
		return NULL;
	}

	return &fd_req->fd_task;
}

static int fd_do_readv(struct se_task *task)
{
	struct fd_request *req = FILE_REQ(task);
	struct se_device *se_dev = req->fd_task.task_se_cmd->se_dev;
	struct fd_dev *dev = se_dev->dev_ptr;
	struct file *fd = dev->fd_file;
	struct scatterlist *sg = task->task_sg;
	struct iovec *iov;
	mm_segment_t old_fs;
	loff_t pos = (task->task_lba *
		      se_dev->se_sub_dev->se_dev_attrib.block_size);
	int ret = 0, i;

	iov = kzalloc(sizeof(struct iovec) * task->task_sg_nents, GFP_KERNEL);
	if (!iov) {
		pr_err("Unable to allocate fd_do_readv iov[]\n");
		return -ENOMEM;
	}

	for_each_sg(task->task_sg, sg, task->task_sg_nents, i) {
		iov[i].iov_len = sg->length;
		iov[i].iov_base = sg_virt(sg);
	}

	old_fs = get_fs();
	set_fs(get_ds());
	ret = vfs_readv(fd, &iov[0], task->task_sg_nents, &pos);
	set_fs(old_fs);

	kfree(iov);
	/*
	 * Return zeros and GOOD status even if the READ did not return
	 * the expected virt_size for struct file w/o a backing struct
	 * block_device.
	 */
	if (S_ISBLK(fd->f_dentry->d_inode->i_mode)) {
		if (ret < 0 || ret != task->task_size) {
			pr_err("vfs_readv() returned %d,"
				" expecting %d for S_ISBLK\n", ret,
				(int)task->task_size);
			return (ret < 0 ? ret : -EINVAL);
		}
	} else {
		if (ret < 0) {
			pr_err("vfs_readv() returned %d for non"
				" S_ISBLK\n", ret);
			return ret;
		}
	}

	return 1;
}

static int fd_do_writev(struct se_task *task)
{
	struct fd_request *req = FILE_REQ(task);
	struct se_device *se_dev = req->fd_task.task_se_cmd->se_dev;
	struct fd_dev *dev = se_dev->dev_ptr;
	struct file *fd = dev->fd_file;
	struct scatterlist *sg = task->task_sg;
	struct iovec *iov;
	mm_segment_t old_fs;
	loff_t pos = (task->task_lba *
		      se_dev->se_sub_dev->se_dev_attrib.block_size);
	int ret, i = 0;

	iov = kzalloc(sizeof(struct iovec) * task->task_sg_nents, GFP_KERNEL);
	if (!iov) {
		pr_err("Unable to allocate fd_do_writev iov[]\n");
		return -ENOMEM;
	}

	for_each_sg(task->task_sg, sg, task->task_sg_nents, i) {
		iov[i].iov_len = sg->length;
		iov[i].iov_base = sg_virt(sg);
	}

	old_fs = get_fs();
	set_fs(get_ds());
	ret = vfs_writev(fd, &iov[0], task->task_sg_nents, &pos);
	set_fs(old_fs);

	kfree(iov);

	if (ret < 0 || ret != task->task_size) {
		pr_err("vfs_writev() returned %d\n", ret);
		return (ret < 0 ? ret : -EINVAL);
	}

	return 1;
}

static void fd_emulate_sync_cache(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	struct fd_dev *fd_dev = dev->dev_ptr;
	int immed = (cmd->t_task_cdb[1] & 0x2);
	loff_t start, end;
	int ret;

	/*
	 * If the Immediate bit is set, queue up the GOOD response
	 * for this SYNCHRONIZE_CACHE op
	 */
	if (immed)
		transport_complete_sync_cache(cmd, 1);

	/*
	 * Determine if we will be flushing the entire device.
	 */
	if (cmd->t_task_lba == 0 && cmd->data_length == 0) {
		start = 0;
		end = LLONG_MAX;
	} else {
		start = cmd->t_task_lba * dev->se_sub_dev->se_dev_attrib.block_size;
		if (cmd->data_length)
			end = start + cmd->data_length;
		else
			end = LLONG_MAX;
	}

	ret = vfs_fsync_range(fd_dev->fd_file, start, end, 1);
	if (ret != 0)
		pr_err("FILEIO: vfs_fsync_range() failed: %d\n", ret);

	if (!immed)
		transport_complete_sync_cache(cmd, ret == 0);
}


#if defined(CONFIG_MACH_QNAPTS)
/*
 * WRITE Force Unit Access (FUA) emulation on a per struct se_task
 * LBA range basis..
 */
static int fd_emulate_write_fua(struct se_cmd *cmd, struct se_task *task)

{
	struct se_device *dev = cmd->se_dev;
	struct fd_dev *fd_dev = dev->dev_ptr;
	loff_t start = task->task_lba * dev->se_sub_dev->se_dev_attrib.block_size;
	loff_t end = start + task->task_size;
	int ret;

	pr_debug("FILEIO: FUA WRITE LBA: %llu, bytes: %u\n",
			task->task_lba, task->task_size);

	ret = vfs_fsync_range(fd_dev->fd_file, start, end, 1);
	if (ret != 0)
		pr_err("FILEIO: vfs_fsync_range() failed: %d\n", ret);

	return ret;
}

#else

/*
 * WRITE Force Unit Access (FUA) emulation on a per struct se_task
 * LBA range basis..
 */
static void fd_emulate_write_fua(struct se_cmd *cmd, struct se_task *task)
{
	struct se_device *dev = cmd->se_dev;
	struct fd_dev *fd_dev = dev->dev_ptr;
	loff_t start = task->task_lba * dev->se_sub_dev->se_dev_attrib.block_size;
	loff_t end = start + task->task_size;
	int ret;

	pr_debug("FILEIO: FUA WRITE LBA: %llu, bytes: %u\n",
			task->task_lba, task->task_size);

	ret = vfs_fsync_range(fd_dev->fd_file, start, end, 1);
	if (ret != 0)
		pr_err("FILEIO: vfs_fsync_range() failed: %d\n", ret);
}
#endif

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TP)
#if defined(SUPPORT_VOLUME_BASED)
void dm_monitor_callback_fn(void *dev, int dmstate)
{
	struct se_device *ldev = dev;

	/* to prevent thin-lun and thin-pool both be removed */
	if (ldev == NULL)
		return;
	
	if ( dmstate == 1 )
		ldev->se_sub_dev->se_dev_attrib.gti = NULL;
}

#if defined(SUPPORT_FILEIO_ON_FILE)
/* (1) the coompile path shall not go to here
 * (2) the SUPPORT_FILEIO_ON_FILE shall NOT be set when enable SUPPORT_VOLUME_BASED
 */
static int fd_calculate_threshold_elements(
	LIO_SE_DEVICE *se_dev,
	uint64_t *avail_bytes_size,
	int *soft_t_reached
	)
{
	return -ENOSYS;
}

static int fd_do_discard(
	struct se_cmd *se_cmd, 
	sector_t lba, 
	u32 range
	)
{
	return -ENOTSUPP;
}

/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __fd_get_lba_map_status(
	struct se_cmd *se_cmd, 
	sector_t lba,
	u32 desc_count, 
	u8 *param, 
	int *err
	)
{
	*err = ERR_UNKNOWN_SAM_OPCODE;
	return -ENOTSUPP;
}
/* 2014/06/14, adamhsu, redmine 8530 (end) */


#else /* !defined(SUPPORT_FILEIO_ON_FILE) */

/* (1) path for SUPPORT_VOLUME_BASED = 1 and SUPPORT_FILEIO_ON_FILE = 0 
 */
static int fd_calculate_threshold_elements(
	LIO_SE_DEVICE *se_dev,
	uint64_t *avail_bytes_size,
	int *soft_t_reached
	)
{
	int ret;
	uint64_t total_size, used_size, t_min;	

	/* This function will call some functions exported by dm layer and
	 * it is used on file io + block-backend dev
	 */

	*soft_t_reached = 0;
	*avail_bytes_size = 0;

	/* the unit of total_size and used_size is sector (512b) */
	ret = thin_get_data_status(
		se_dev->se_sub_dev->se_dev_attrib.gti, 
		&total_size, &used_size);
	
	if ( ret == 0 ){

		/* Here to use div_u64() to make 64 bit division to
		 * avoid this code will be fail to build with 32bit 
		 * compiler environment
		 */
		t_min = div_u64((total_size * 512 * \
			se_dev->se_sub_dev->se_dev_attrib.tp_threshold_percent), 
			100);
	
		t_min -= (((1 << se_dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size) >> 1) * 512);
	
		/* calculate used and available resource count */
		se_dev->se_sub_dev->se_dev_attrib.used = \
			(u32)div_u64((used_size * 512), 
			((1 << (se_dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size)) * 512));
	
		se_dev->se_sub_dev->se_dev_attrib.avail = \
			(u32)div_u64(((total_size - used_size) * 512), 
			((1 << (se_dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size)) * 512));

		if ( (used_size * 512) >= t_min){
			if ( !se_dev->se_sub_dev->se_dev_attrib.tp_threshold_hit ){
				se_dev->se_sub_dev->se_dev_attrib.tp_threshold_hit++;

				/* it is time to send evt cause of the soft
				 * threshold was reached
				 */
				*avail_bytes_size =
					((total_size - used_size) << 9);

				*soft_t_reached = 1;
				return -ENOSYS;
			}
		} else 
			se_dev->se_sub_dev->se_dev_attrib.tp_threshold_hit = 0;

		return 0;
	}

	/* TODO */
	return 0;

}


/* @fn static int fd_do_discard(struct se_cmd *se_cmd, sector_t lba, u32 range)
 * @brief function to do file io discard on blk-backebd device
 */
static int fd_do_discard(
	struct se_cmd *se_cmd, 
	sector_t lba, 
	u32 range
	)
{
#define DEFAULT_ALLOC_SIZE	(1 << 20)
#define DEFAULT_ALIGN_SIZE	(1 << 20)
#define NORMAL_IO_TIMEOUT	(5)	/* unit is second */

	struct se_device *dev = se_cmd->se_dev;
	struct fd_dev *fd_dev = dev->dev_ptr;
	struct file *fd = fd_dev->fd_file;
	struct inode *pInode = fd->f_mapping->host;
	struct address_space *mapping = pInode->i_mapping;
	int Ret = 0, bs_order, barrier = 0;
	loff_t first_page = 0, last_page = 0, Start = 0, Len = 0;
	loff_t first_page_offset = 0, last_page_offset = 0;

	/* 20140626, adamhsu, redmine 8745,8777,8778 (start) */
	u64 alloc_bytes = DEFAULT_ALLOC_SIZE;
	int normal_io = 1;
	sector_t t_lba = 0, t_range = 0, real_range = 0;
	u32 tmp;
	ALIGN_DESC align_desc;
	GEN_RW_TASK w_task;
	/* 20140626, adamhsu, redmine 8745,8777,8778 (end) */

	// Check if LIO-fileio + block-based device	
	if (!S_ISBLK(fd->f_dentry->d_inode->i_mode)) {
		pr_err("%s: error!! backend type is not block dev\n", 
			__FUNCTION__);
		return -ENOSYS;
	}

	pr_debug("%s: Lba:0x%llx, range:0x%x\n", __FUNCTION__, 
		(unsigned long long)lba, (u32)range);

	bs_order = ilog2(dev->se_sub_dev->se_dev_attrib.block_size);

	/* 20140626, adamhsu, redmine 8745,8777,8778 (start) */
	__create_aligned_range_desc(&align_desc, lba, 
			range, bs_order, DEFAULT_ALIGN_SIZE);

	/* create the sg io data */
	memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));
	Ret = __generic_alloc_sg_list(&alloc_bytes, &w_task.sg_list, 
			&w_task.sg_nents);
	if (Ret != 0){
		if (Ret == -ENOMEM){
			pr_err("%s: fail to alloc sg list\n", __FUNCTION__);
			__set_err_reason(ERR_OUT_OF_RESOURCES, 
					&se_cmd->scsi_sense_reason);
		}
		if (Ret == -EINVAL){
			pr_err("%s: invalid arg during to alloc sg list\n", 
				__FUNCTION__);
			__set_err_reason(ERR_INVALID_PARAMETER_LIST, 
					&se_cmd->scsi_sense_reason);
		}
		return Ret;
	}

	while (range){
		t_lba = lba;
		t_range = real_range = min_t(sector_t, range, DEFAULT_ALIGN_SIZE);

		if (align_desc.is_aligned){
			normal_io = 0;
			if (lba == align_desc.lba)
				real_range = align_desc.nr_blks;
			else {
				if (lba < align_desc.lba)
					real_range = align_desc.lba - lba;
				normal_io = 1;
			}
		}

		pr_debug("%s: normal_io:%d, Lba:0x%llx, real_range:0x%x\n", 
			__FUNCTION__, normal_io, (unsigned long long)lba,
			(u32)real_range);

		if (unlikely(normal_io))
			goto _NORMAL_IO_;

		/* truncate LIO-fileio PageCache */
		Start = (loff_t)(lba << bs_order);
		Len = (loff_t)(range << bs_order);
		first_page = (Start) >> PAGE_CACHE_SHIFT;
		last_page = (Start + Len) >> PAGE_CACHE_SHIFT;
		first_page_offset = first_page	<< PAGE_CACHE_SHIFT;
		last_page_offset = (last_page << PAGE_CACHE_SHIFT) + \
				((PAGE_CACHE_SIZE - 1));

		if (mapping->nrpages 
		&& mapping_tagged(mapping, PAGECACHE_TAG_DIRTY))
		{
			Ret = filemap_write_and_wait_range(mapping, 
				first_page_offset, last_page_offset);
			if (unlikely(Ret)) {
				pr_err("%s: fail to exec fd_do_discard(), "
					"Ret:0x%x\n", __FUNCTION__, Ret);
				break;
			}
		}

		truncate_pagecache_range(pInode, first_page_offset, 
			last_page_offset);

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
		/* To convert the lba again cause of the blk layer
		 * only accept 512 bytes unit */
		Ret = __blkio_transfer_task_lba_to_block_lba(
				(1 << bs_order), &t_lba);
		if (Ret != 0)
			break;
		t_range = (real_range * ((1 << bs_order) >> 9));
#endif
		pr_debug("%s: t_lba:0x%llx, t_range:0x%x\n", __FUNCTION__, 
			(unsigned long long)t_lba, (u32)t_range);

		Ret = qnap_transport_blkdev_issue_discard(se_cmd,
			pInode->i_bdev, t_lba, t_range,	GFP_KERNEL, barrier);

		if (unlikely(Ret)) {
			pr_err("%s: fail to exec discard_func() Ret:0x%x\n", 
				__FUNCTION__, Ret);
			break;
		}

		if ((qnap_transport_is_dropped_by_release_conn(se_cmd) == true)
		|| (qnap_transport_is_dropped_by_tmr(se_cmd) == true)
		)
			break;

		goto _GO_NEXT_;

_NORMAL_IO_:
		/* The path for real io */
		t_lba = lba;
		t_range = real_range;

		do {
			tmp = min_t(u32, t_range, ((u32)alloc_bytes >> \
					bs_order));
			pr_debug("%s: tmp:0x%x\n", tmp);
		
			__make_rw_task(&w_task, dev, t_lba, tmp, 
				msecs_to_jiffies(NORMAL_IO_TIMEOUT*1000), 
				DMA_TO_DEVICE);
		
			Ret = __do_f_rw(&w_task);
		
			pr_debug("%s: after call __do_f_rw() Ret:%d, "
				"is_timeout:%d, ret code:%d\n",
				__FUNCTION__, Ret, w_task.is_timeout, 
				w_task.ret_code);
		
			if((Ret <= 0) || w_task.is_timeout 
			|| w_task.ret_code != 0)
			{
				Ret = w_task.ret_code;
				if (w_task.is_timeout)
					Ret = 1;
				break;
			}
			Ret = 0;
			t_lba += tmp;
			t_range -= tmp;
		} while (t_range);

		if (t_range)
			break;
_GO_NEXT_:
		lba += real_range;
		range -= real_range;
	}

	__generic_free_sg_list(w_task.sg_list, w_task.sg_nents);

	if (Ret){
		if (Ret == -ENOSPC)
			__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
				&se_cmd->scsi_sense_reason);
		else
			__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
				&se_cmd->scsi_sense_reason);
	}
	return Ret;
	/* 20140626, adamhsu, redmine 8745,8777,8778 (end) */

}

/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __fd_get_lba_map_status(
	struct se_cmd *se_cmd, 
	sector_t lba,
	u32 desc_count, 
	u8 *param, 
	int *err
	)
{
	struct se_device *dev = se_cmd->se_dev;
	struct se_subsystem_dev *se_dev = dev->se_sub_dev;
	LIO_FD_DEV *fd_dev = (LIO_FD_DEV *)dev->dev_ptr;
	struct inode *inode = fd_dev->fd_file->f_mapping->host;
	u64 lindex;
	u32 num_of_lb, sector_per_block, len;
	uint8_t *pro_status = NULL;
	u16 off = 8;
	int i, ret = -EOPNOTSUPP;
	char lvname[256];
	ERR_REASON_INDEX err_reason;

	/* here is for file i/o + block based configuration */
	if (!S_ISBLK(inode->i_mode)){
		err_reason = ERR_UNKNOWN_SAM_OPCODE;
		ret = -ENOTSUPP;
		goto EXIT;
	}

	/* get the desc counts first */
	pro_status = kzalloc(desc_count, GFP_KERNEL);
	if (!pro_status){
		err_reason = ERR_INVALID_PARAMETER_LIST;
		ret = -ENOMEM;
		goto EXIT;
	}

	if (!strcmp(se_dev->se_dev_udev_path, "")){
		err_reason = ERR_INVALID_PARAMETER_LIST;
		ret = -ENODEV;
		goto EXIT;
	}

	// volume-based device
	strcpy(lvname, se_dev->se_dev_udev_path);

	ret = thin_get_sectors_per_block(lvname, &sector_per_block);
	if (ret != 0){
		pr_debug("call thin_get_sectors_per_block error!\n");
		err_reason = ERR_INVALID_PARAMETER_LIST;
		ret = -EOPNOTSUPP;
		goto EXIT;
	}

	/* Here to use div_u64() to make 64 bit division to avoid this code
	 * will be fail to build with 32bit compiler environment
	 */
	lindex = div_u64((u64)lba, sector_per_block);
	num_of_lb =  sector_per_block;

	ret = thin_get_lba_status(lvname, lindex, desc_count, pro_status);
	if (ret != 0){
		pr_debug("call thin_get_lba_status error!\n");
		err_reason = ERR_INVALID_PARAMETER_LIST;
		ret = -EOPNOTSUPP;
		goto EXIT;
	}

	for (i = 0; i < desc_count; i++){
		/* STARTING LOGICAL BLOCK ADDRESS */
		param[off + 0] = (lba >> 56) & 0xff;
		param[off + 1] = (lba >> 48) & 0xff;
		param[off + 2] = (lba >> 40) & 0xff;
		param[off + 3] = (lba >> 32) & 0xff;
		param[off + 4] = (lba >> 24) & 0xff;
		param[off + 5] = (lba >> 16) & 0xff;
		param[off + 6] = (lba >> 8) & 0xff;
		param[off + 7] = lba & 0xff;
		off += 8;

		/* NUMBER OF LOGICAL BLOCKS */
		param[off + 0] = (num_of_lb >> 24) & 0xff;
		param[off + 1] = (num_of_lb >> 16) & 0xff;
		param[off + 2] = (num_of_lb >> 8) & 0xff;
		param[off + 3] = num_of_lb & 0xff;
		off += 4;

		/* PROVISIONING STATUS 
		 * pro_status: 0->mapped, 1->unmapped
		 */
		param[off] = pro_status[i];
		off += 4;

		lba  += num_of_lb;
	}

	/* to update PARAMETER DATA LENGTH finally */
	len = ((desc_count << 4) + 4);
	param[0] = (len >> 24) & 0xff;
	param[1] = (len >> 16) & 0xff;
	param[2] = (len >> 8) & 0xff;
	param[3] = len & 0xff;
	ret = 0;
EXIT:
	if (pro_status)
		kfree(pro_status);
	
	if (ret != 0)
		*err = (int)err_reason;

	return ret;
}
/* 2014/06/14, adamhsu, redmine 8530 (end) */


#endif

#else /* !defined(SUPPORT_VOLUME_BASED) */

#if defined(SUPPORT_FILEIO_ON_FILE)

/* (1) path for SUPPORT_VOLUME_BASED = 0 and SUPPORT_FILEIO_ON_FILE = 1
 * this is for static volume
 */
static int fd_calculate_threshold_elements(
	LIO_SE_DEVICE *se_dev,
	uint64_t *avail_bytes_size,
	int *soft_t_reached
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *inode = NULL;
	uint64_t total_size, used_size, t_min;

	/* This function will be called on file io + file-backend
	 * environment (static volume)
	 */

	*soft_t_reached = 0;
		
	fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
	inode = fd_dev->fd_file->f_mapping->host;
	total_size = (uint64_t)inode->i_size;
	used_size = (uint64_t)inode->i_blocks;

	/* Here to use div_u64() to make 64 bit division to 
	 * avoid this code will be fail to build with 32bit
	 * compiler environment
	 */
	t_min = div_u64(
		(total_size * se_dev->se_sub_dev->se_dev_attrib.tp_threshold_percent), 
		100);

	t_min -= (((1 << \
		se_dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size) >> 1) * 512);

//TODO
#if 0	/* move to function of iblock_update_allocated() */
	/* calculate used and available resource count */
	dev->se_sub_dev->se_dev_attrib.allocated = used * 512;
#endif
	se_dev->se_sub_dev->se_dev_attrib.used = 
			(u32)div_u64((u64)(used_size * 512), 
				((1 << (se_dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size)) *512));

	se_dev->se_sub_dev->se_dev_attrib.avail =
			(u32)div_u64((u64)((total_size - (used_size * 512))), 
			((1 << (se_dev->se_sub_dev->se_dev_attrib.tp_threshold_set_size)) * 512));

	if ((used_size * 512) >= t_min){
		if ( !se_dev->se_sub_dev->se_dev_attrib.tp_threshold_hit ){
			se_dev->se_sub_dev->se_dev_attrib.tp_threshold_hit++;

			/* it is time to send evt cause of the soft
			 * threshold was reached
			 */
			*avail_bytes_size =
					(total_size - (used_size << 9));

			*soft_t_reached = 1;
			return -ENOSYS;
		}
	}
	else{
		se_dev->se_sub_dev->se_dev_attrib.tp_threshold_hit = 0;
	}

	return 0;
}

/* 201404xx, adamhsu, redmine 8058
 * @fn static int fd_do_discard(struct se_cmd *se_cmd, sector_t lba, u32 range)
 * @brief function to do file io discard on file-backebd device (static volume)
 */
static int fd_do_discard(
	struct se_cmd *se_cmd, 
	sector_t lba, 
	u32 range
	)
{
	struct se_device *dev = se_cmd->se_dev;
	struct fd_dev *fd_dev = dev->dev_ptr;
	struct file *fd = fd_dev->fd_file;
	loff_t start = 0, len = 0;
	int ret = 0, bs_order;
	
	if (S_ISBLK(fd->f_dentry->d_inode->i_mode)) {
		pr_err("[UNMAP FIO FileBackend] error!! backend type "
				"is not file dev\n");
		return -ENOSYS;
	}

	if (!fd_dev->fd_file->f_op->fallocate){
		pr_err("[UNMAP FIO FileBackend] Not support fallocate op\n");
		return -ENOTSUPP;
	}
	
	bs_order = ilog2(dev->se_sub_dev->se_dev_attrib.block_size);
	start = (loff_t)(lba << bs_order);
	len = (loff_t)(range << bs_order);

	ret = fd_dev->fd_file->f_op->fallocate(fd_dev->fd_file,
			(FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE),
			start, len);

	if (ret != 0)
		pr_err("[UNMAP FIO FileBackend] ret:%d after call fallocate op "
			"with punch hole + keep size\n", ret);

	return ret;
}


/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __fd_get_lba_map_status(
	struct se_cmd *se_cmd, 
	sector_t lba,
	u32 desc_count, 
	u8 *param, 
	int *err
	)
{
#define SIZE_ORDER	20

	LIO_SE_DEVICE *se_dev = se_cmd->se_dev;
	LIO_FD_DEV *fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
	u32 idx, count = desc_count;
	int ret;
	LBA_STATUS_DESC *desc = NULL;

	/**/
	desc = kzalloc((count * sizeof(LBA_STATUS_DESC)), GFP_KERNEL);
	if (!desc){
		*err = (int)ERR_OUT_OF_RESOURCES;
		ret = -ENOMEM;
		goto _EXIT_;
	}

	/* here is for file i/o + static volume configuration */
	if (S_ISBLK(fd_dev->fd_file->f_mapping->host->i_mode)){
		*err = (int)ERR_UNKNOWN_SAM_OPCODE;
		ret = -EOPNOTSUPP;
		goto _EXIT_;
	}

	ret = __get_file_lba_map_status(se_dev, NULL, 
			fd_dev->fd_file->f_mapping->host,
			lba, &count, (u8 *)desc);

	if (ret != 0){
		pr_debug("%s - ret:%d, after exec "
			"__get_file_lba_map_status()\n", __FUNCTION__, ret);

		*err = (int)ERR_UNKNOWN_SAM_OPCODE;
	} else {
		/* update the lba status descriptor */
		memcpy(&param[8], (u8 *)desc,
			(count* sizeof(LBA_STATUS_DESC)));

		/* to update PARAMETER DATA LENGTH finally */
		count = ((count << 4) + 4);
		put_unaligned_be32(count, &param[0]);
	}

_EXIT_:
	if (desc)
		kfree(desc);

	return ret;

}
/* 2014/06/14, adamhsu, redmine 8530 (end) */


#else

/* (1) the compile path shall NOT go here
 * (2) SUPPORT_FILEIO_ON_FILE = 0 and SUPPORT_VOLUME_BASED = 0
 * shall NOT be set
 */
static int fd_calculate_threshold_elements(
	LIO_SE_DEVICE *se_dev,
	uint64_t *avail_bytes_size,
	int *soft_t_reached
	)
{
	return -ENOSYS;
}

static int fd_do_discard(
	struct se_cmd *se_cmd, 
	sector_t lba, 
	u32 range
	)
{
	return -ENOTSUPP;
}

/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __fd_get_lba_map_status(
	struct se_cmd *se_cmd, 
	sector_t lba,
	u32 desc_count, 
	u8 *param, 
	int *err
	)
{
	*err = ERR_UNKNOWN_SAM_OPCODE;
	return -ENOTSUPP;
}
/* 2014/06/14, adamhsu, redmine 8530 (end) */


#endif
#endif

static int fd_check_before_write(
	LIO_SE_CMD *se_cmd,
	LIO_SE_DEVICE *se_dev
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *inode = NULL;
	int ret, soft_t_reached = 0;
	char lvname[FD_MAX_DEV_NAME];
	uint64_t avail_bytes_size;
	
#if defined(QNAP_HAL)
	NETLINK_EVT hal_event;
#endif

	fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
	inode = fd_dev->fd_file->f_mapping->host;

	memset(&lvname, 0, sizeof(lvname));
	strcpy(lvname, se_dev->se_sub_dev->se_dev_udev_path);

#if defined(QNAP_HAL)
	memset(&hal_event, 0, sizeof(NETLINK_EVT));
	hal_event.type = HAL_EVENT_ISCSI;
#endif
	/* TODO, 201404xx, adamhsu, redmine 8037
	 *
	 * (1) we did some modifications for fileio + file-backend configuration
	 * (2) we may change it in the future
	 */
	if (lvname[0] != 0x00){
		if (!S_ISBLK(inode->i_mode)){
			/* If now is file io and udev_path is NOT null BUT
			 * it is NOT blk-backend dev. Then to return error
			 */
			pr_err("udev_path is NOT null but it is NOT "
				"fio + blk-backend env\n");
			se_cmd->scsi_sense_reason = 
				TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			return -ENODEV;
		}

#if defined(SUPPORT_VOLUME_BASED)

		/* Case for fileio + block-backend dev */
		if (!se_dev->se_sub_dev->se_dev_attrib.gti){
			/* try to get dm target and set the dm monitor */
			ret = thin_get_dmtarget(lvname, 
				&se_dev->se_sub_dev->se_dev_attrib.gti
				);

			if (ret == 0){
				thin_set_dm_monitor(
					se_dev->se_sub_dev->se_dev_attrib.gti,
					se_dev, dm_monitor_callback_fn);
			}
			else{
				// thin_get_dmtarget fail, may retry
				pr_debug("fail to call thin_get_dmtarget(), "
					"ret:%d\n", ret);
			}			
		} else {
			/* thin-pool do not extend, do nothing if we got
			 * dm target already
			 */
			pr_debug("se_dev_attrib.gti is not null\n");
		}
#endif
	} else {
		if (S_ISBLK(inode->i_mode)){
			/* Case for file io + block-backend dev. If the 
			 * udev_path string is NULL, it MAY be the error
			 * caused by iscsi lio utility
			 */
			pr_err("udev_path is NULL on fio + blk-backend env\n");
			se_cmd->scsi_sense_reason = 
				TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			return -ENODEV;
		}
		/* Case for file io + file-backend dev. Currently, it is
		 * static volume and udev_path is null
		 */
		pr_debug("go on fio + file-backend env\n");
	}

	ret = fd_calculate_threshold_elements(se_dev,
			&avail_bytes_size, &soft_t_reached);

	if ((ret == -ENOSYS) && (soft_t_reached == 1)){
		se_cmd->scsi_sense_reason = 
			TCM_THIN_PROVISIONING_SOFT_THRESHOLD_REACHED;
			
#if defined(QNAP_HAL)
		hal_event.arg.action = HIT_LUN_THRESHOLD;

		hal_event.arg.param.iscsi_lun.lun_index = 
			se_dev->se_sub_dev->se_dev_attrib.lun_index;

		hal_event.arg.param.iscsi_lun.tp_threshold = 
			se_dev->se_sub_dev->se_dev_attrib.tp_threshold_percent;

		hal_event.arg.param.iscsi_lun.tp_avail = 
			(avail_bytes_size >> 30); //unit: GB

		send_hal_netlink(&hal_event);
#endif
		return -ENOSYS;
	}

	return 0;
}
#endif /* defined(SUPPORT_TP) */
#endif /* defined(CONFIG_MACH_QNAPTS) */

static int fd_do_task(struct se_task *task)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	int ret = 0;

#if defined(CONFIG_MACH_QNAPTS)
	struct fd_dev *fd_dev = dev->dev_ptr;
	struct file *fd = fd_dev->fd_file;
	struct inode *inode = fd->f_mapping->host;
	struct scatterlist *sg = task->task_sg;
	int idx, err_1, err_2, err_3;
	loff_t len = 0, start = 0;

#if defined(SUPPORT_TP)
	unsigned long long blocks = dev->transport->get_blocks(dev);
	/* For run-time capacity change warning and only checking
	 * for thin-lun
	 */
	if(!strcmp(dev->se_sub_dev->se_dev_provision, "thin")){
		if ( dev->se_sub_dev->se_dev_attrib.lun_blocks != blocks ){
			dev->se_sub_dev->se_dev_attrib.lun_blocks = blocks;
			cmd->scsi_sense_reason = TCM_CAPACITY_DATA_HAS_CHANGED;
			return -ENOSYS;
		}
	}
#endif
#endif
	/*
	 * Call vectorized fileio functions to map struct scatterlist
	 * physical memory addresses to struct iovec virtual memory.
	 */
	if (task->task_data_direction == DMA_FROM_DEVICE) {
		ret = fd_do_readv(task);
	} else {

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TP)
		if(!strcmp(dev->se_sub_dev->se_dev_provision, "thin")){

			ret = fd_check_before_write(cmd, dev);

			/* The scsi sense reason will be set in 
			 * fd_check_before_write() 
			 */
			if (ret != 0){
				pr_debug("ret:%d after to call "
					"fd_check_before_write\n", ret);
				return ret;
			}
		}

#endif

#if defined(SUPPORT_ISCSI_ZERO_COPY)   //20121130, adam hsu support iscsi zero-copy
		if(dev->transport->name[0]=='f'
		&& !(cmd->digest_zero_copy_skip))
		{
			if(cmd->err != 0)
				ret = cmd->err;
			else
				ret = 1;
		}else
#endif
#endif
		ret = fd_do_writev(task);


#if defined(CONFIG_MACH_QNAPTS)
		/* check whether was no space already ? */
#if defined(SUPPORT_ISCSI_ZERO_COPY)
		if (!cmd->digest_zero_copy_skip)
			goto _EXIT_1_;
#endif

#if defined(SUPPORT_TP)
		if (ret == 1 && is_thin_lun(cmd->se_dev)){
			/* TODO: 
			 * To sync cache again if write ok and the sync cache
		  	 * behavior shall work for thin lun only 
		  	 */
			start = (task->task_lba *
			      dev->se_sub_dev->se_dev_attrib.block_size);

			for_each_sg(task->task_sg, sg, task->task_sg_nents, idx)
				len += sg->length;

			/* check whether was no space already ? */
			err_1 = check_dm_thin_cond(inode->i_bdev);
			if (err_1 == 0)
				goto _EXIT_1_;

			/* time to do sync i/o
			 * 1. hit the sync i/o threshold area
			 * 2. or, space is full BUT need to handle lba where was mapped
			 * or not
			 */
			if (err_1 == 1 || err_1 == -ENOSPC){
				err_2 = __do_sync_cache_range(fd, 
					start, (start + len));

				if (err_2 != 0){
					/* TODO:
					 * thin i/o may go here (lba wasn't mapped to
					 * any block) or something wrong during normal
					 * sync-cache
					 */
					if (err_2 != -ENOSPC){				
						/* call again to make sure it is
						 * no space really or not
						 */
						err_3 = check_dm_thin_cond(inode->i_bdev);
						if (err_3 == -ENOSPC){
							pr_warn("%s: space was full "
								"already\n",__func__);
							err_2 = err_3;
						}
						/* it may something wrong duing sync-cache */
					} else 
						pr_warn("%s: space was full "
							"already\n",__func__);

					ret = err_2;
				}

			}

			/* fall-through */
		}
#endif

_EXIT_1_:
#endif

		if (ret > 0 &&
		    dev->se_sub_dev->se_dev_attrib.emulate_write_cache > 0 &&
		    dev->se_sub_dev->se_dev_attrib.emulate_fua_write > 0 &&
		    (cmd->se_cmd_flags & SCF_FUA)) {
			/*
			 * We might need to be a bit smarter here
			 * and return some sense data to let the initiator
			 * know the FUA WRITE cache sync failed..?
			 */

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
			ret = fd_emulate_write_fua(cmd, task);
			if (ret != 0){
				int err_1;

				if (is_thin_lun(cmd->se_dev)){
					/* check whether was no space already ? */
					err_1 = check_dm_thin_cond(inode->i_bdev);
#if 0
					pr_info("[%s] after call fd_emulate_write_fua, "
					"ret:%d, err_1:%d\n", __func__, ret, err_1);
#endif
					if (err_1 == -ENOSPC){
						pr_warn("%s: space was full "
						"already (FUA bit = 1)\n", __func__);
						ret = err_1;
					}
				}
			} else
				ret = 1;
#else
			fd_emulate_write_fua(cmd, task);
#endif
		}

	}

	if (ret < 0){
#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
		if (ret == -ENOSPC ) { // for no space response
			cmd->scsi_sense_reason = TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT;
		}
		else 
#endif			
		cmd->scsi_sense_reason = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		return ret;
	}
	
	if (ret) {
		task->task_scsi_status = GOOD;
		transport_complete_task(task, 1);
	}
	return 0;
}


/*	fd_free_task(): (Part of se_subsystem_api_t template)
 *
 *
 */
static void fd_free_task(struct se_task *task)
{
	struct fd_request *req = FILE_REQ(task);

	kfree(req);
}

enum {
	Opt_fd_dev_name, Opt_fd_dev_size, Opt_fd_buffered_io, Opt_err
};

static match_table_t tokens = {
	{Opt_fd_dev_name, "fd_dev_name=%s"},
	{Opt_fd_dev_size, "fd_dev_size=%s"},
	{Opt_fd_buffered_io, "fd_buffered_io=%d"},
	{Opt_err, NULL}
};

static ssize_t fd_set_configfs_dev_params(
	struct se_hba *hba,
	struct se_subsystem_dev *se_dev,
	const char *page, ssize_t count)
{
	struct fd_dev *fd_dev = se_dev->se_dev_su_ptr;
	char *orig, *ptr, *arg_p, *opts;
	substring_t args[MAX_OPT_ARGS];
	int ret = 0, arg, token;

	opts = kstrdup(page, GFP_KERNEL);
	if (!opts)
		return -ENOMEM;

	orig = opts;

	while ((ptr = strsep(&opts, ",\n")) != NULL) {
		if (!*ptr)
			continue;

		token = match_token(ptr, tokens, args);
		switch (token) {
		case Opt_fd_dev_name:
			if (match_strlcpy(fd_dev->fd_dev_name, &args[0],
				FD_MAX_DEV_NAME) == 0) {
				ret = -EINVAL;
				break;
			}
			pr_debug("FILEIO: Referencing Path: %s\n",
					fd_dev->fd_dev_name);
			fd_dev->fbd_flags |= FBDF_HAS_PATH;
			break;
		case Opt_fd_dev_size:
			arg_p = match_strdup(&args[0]);
			if (!arg_p) {
				ret = -ENOMEM;
				break;
			}
			ret = strict_strtoull(arg_p, 0, &fd_dev->fd_dev_size);
			kfree(arg_p);
			if (ret < 0) {
				pr_err("strict_strtoull() failed for"
						" fd_dev_size=\n");
				goto out;
			}
			pr_debug("FILEIO: Referencing Size: %llu"
					" bytes\n", fd_dev->fd_dev_size);
			fd_dev->fbd_flags |= FBDF_HAS_SIZE;
			break;
		case Opt_fd_buffered_io:
			match_int(args, &arg);
			if (arg != 1) {
				pr_err("bogus fd_buffered_io=%d value\n", arg);
				ret = -EINVAL;
				goto out;
			}

			pr_debug("FILEIO: Using buffered I/O"
				" operations for struct fd_dev\n");

			fd_dev->fbd_flags |= FDBD_USE_BUFFERED_IO;
			break;
		default:
			break;
		}
	}

out:
	kfree(orig);
	return (!ret) ? count : ret;
}

static ssize_t fd_check_configfs_dev_params(struct se_hba *hba, struct se_subsystem_dev *se_dev)
{
	struct fd_dev *fd_dev = se_dev->se_dev_su_ptr;

	if (!(fd_dev->fbd_flags & FBDF_HAS_PATH)) {
		pr_err("Missing fd_dev_name=\n");
		return -EINVAL;
	}

	return 0;
}

static ssize_t fd_show_configfs_dev_params(
	struct se_hba *hba,
	struct se_subsystem_dev *se_dev,
	char *b)
{
	struct fd_dev *fd_dev = se_dev->se_dev_su_ptr;
	ssize_t bl = 0;

#ifdef CONFIG_MACH_QNAPTS // Benjamin 20120607 change for QNAP ID
#if defined(Athens)
	bl = sprintf(b + bl, "Cisco FILEIO ID: %u", fd_dev->fd_dev_id);
#elif defined(IS_G)
	bl = sprintf(b + bl, " FILEIO ID: %u", fd_dev->fd_dev_id);
#else
	bl = sprintf(b + bl, "QNAP FILEIO ID: %u", fd_dev->fd_dev_id);
#endif /* #if defined(Athens) */
#else
	bl = sprintf(b + bl, "TCM FILEIO ID: %u", fd_dev->fd_dev_id);
#endif /* #ifdef CONFIG_MACH_QNAPTS */    
	bl += sprintf(b + bl, "        File: %s  Size: %llu  Mode: %s\n",
		fd_dev->fd_dev_name, fd_dev->fd_dev_size,
		(fd_dev->fbd_flags & FDBD_USE_BUFFERED_IO) ?
		"Buffered" : "O_DSYNC");
	return bl;
}

/*	fd_get_device_rev(): (Part of se_subsystem_api_t template)
 *
 *
 */
static u32 fd_get_device_rev(struct se_device *dev)
{
	return SCSI_SPC_2; /* Returns SPC-3 in Initiator Data */
}

/*	fd_get_device_type(): (Part of se_subsystem_api_t template)
 *
 *
 */
static u32 fd_get_device_type(struct se_device *dev)
{
	return TYPE_DISK;
}

static sector_t fd_get_blocks(struct se_device *dev)
{
	struct fd_dev *fd_dev = dev->dev_ptr;
	struct file *f = fd_dev->fd_file;
	struct inode *i = f->f_mapping->host;
	unsigned long long dev_size;
	/*
	 * When using a file that references an underlying struct block_device,
	 * ensure dev_size is always based on the current inode size in order
	 * to handle underlying block_device resize operations.
	 */

#ifdef CONFIG_MACH_QNAPTS
	/* 2014/04/16, adamhsu, redmine 8036
	 * Modify code about the procedure to get blocks is not correct
	 * on file io + file-backend configuration
	 */
	if (S_ISBLK(i->i_mode))
		dev_size = i_size_read(i);
	else
		dev_size = fd_dev->fd_dev_size;

	return div_u64(dev_size - dev->se_sub_dev->se_dev_attrib.block_size,
		dev->se_sub_dev->se_dev_attrib.block_size);

#else
	if (S_ISBLK(i->i_mode))
		dev_size = (i_size_read(i) - fd_dev->fd_block_size);
	else
		dev_size = fd_dev->fd_dev_size;

	return div_u64(dev_size, dev->se_sub_dev->se_dev_attrib.block_size);
#endif
}

#ifdef CONFIG_MACH_QNAPTS // 2010/07/20 Nike Chen, support online lun expansion
static int fd_change_dev_size(struct se_device *dev)
{
    int ret = 0;
    struct fd_dev *fd_dev = dev->dev_ptr;
    struct file *file = fd_dev->fd_file;
    struct inode *inode = file->f_mapping->host;;

    /*
        * Determine the number of bytes from i_size_read() minus
        * one (1) logical sector from underlying struct block_device
        */

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
    /* adamhsu 2013/06/07 - Support to set the logical block size from NAS GUI. */
    if ((dev->se_sub_dev->su_dev_flags & SDF_USING_QLBS) && dev->se_sub_dev->se_dev_qlbs)
        fd_dev->fd_dev_size = (i_size_read(file->f_mapping->host) -
                                dev->se_sub_dev->se_dev_qlbs);
    else
#endif
        fd_dev->fd_dev_size = (i_size_read(file->f_mapping->host) -
                                bdev_logical_block_size(inode->i_bdev));


#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
    /* adamhsu 2013/06/07 - Support to set the logical block size from NAS GUI. */
    if ((dev->se_sub_dev->su_dev_flags & SDF_USING_QLBS) && dev->se_sub_dev->se_dev_qlbs)
        pr_debug("FILEIO: Using size: %llu bytes from struct"
            " block_device, and logical_block_size: %d\n",
            fd_dev->fd_dev_size, dev->se_sub_dev->se_dev_qlbs);
    else
#endif
        /*
         * Benjamin 20120601: 
         * (struct fd_dev *)fd_dev does not have member (struct block_device *)fd_bd anymore, 
         * and fd_bd->fd_file should be the same as fd_dev->fd_file. 
         * By the way, there is no dev->dev_sectors_total anymore.     
         * That's why I do not update the sector count (dev->dev_sectors_total) via READ_CAPACITY
         */
        pr_debug("FILEIO: Using size: %llu bytes from struct"
            " block_device, and logical_block_size: %d\n",
            fd_dev->fd_dev_size, bdev_logical_block_size(inode->i_bdev));


    return ret;
}
#endif
static struct se_subsystem_api fileio_template = {
	.name			= "fileio",
	.owner			= THIS_MODULE,
	.transport_type		= TRANSPORT_PLUGIN_VHBA_PDEV,
	.write_cache_emulated	= 1,
	.fua_write_emulated	= 1,
	.attach_hba		= fd_attach_hba,
	.detach_hba		= fd_detach_hba,
	.allocate_virtdevice	= fd_allocate_virtdevice,
	.create_virtdevice	= fd_create_virtdevice,
	.free_device		= fd_free_device,
	.alloc_task		= fd_alloc_task,
	.do_task		= fd_do_task,
#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
	.do_discard		= fd_do_discard,
#endif	
	.do_sync_cache		= fd_emulate_sync_cache,
	.free_task		= fd_free_task,
	.check_configfs_dev_params = fd_check_configfs_dev_params,
	.set_configfs_dev_params = fd_set_configfs_dev_params,
	.show_configfs_dev_params = fd_show_configfs_dev_params,
	.get_device_rev		= fd_get_device_rev,
	.get_device_type	= fd_get_device_type,
	.get_blocks		= fd_get_blocks,
#ifdef CONFIG_MACH_QNAPTS // 2010/07/20 Nike Chen, support online lun expansion
    .change_dev_size    = fd_change_dev_size,
#endif		

#if defined(CONFIG_MACH_QNAPTS)
	.qnap_sync_cache	= qnap_target_execute_sync_cache,
	.qnap_do_discard	= qnap_target_execute_discard,

#if defined(SUPPORT_VAAI)

	/* api for write same function */
	.do_prepare_ws_buffer       = do_prepare_ws_buffer,
	.do_check_before_ws         = do_check_before_ws,
	.do_check_ws_zero_buffer    = do_check_ws_zero_buffer,
	.do_ws_wo_unmap             = fd_do_ws_wo_unmap,
	.do_ws_w_anchor             = fd_do_ws_w_anchor,
	.do_ws_w_unmap              = fd_do_ws_w_unmap,

	/* api for atomic test and set (ATS) function */
	.do_check_before_ats        = do_check_before_ats,
	.do_ats                     = fd_do_ats,
#endif

#if defined(SUPPORT_TP)
/* 2014/06/14, adamhsu, redmine 8530 (start) */
	.do_get_lba_map_status = __fd_get_lba_map_status,
/* 2014/06/14, adamhsu, redmine 8530 (end) */
#endif

#if defined(SUPPORT_TPC_CMD)
	/* api for 3rd-party ROD function */    
	.do_pt                  = fd_do_populate_token,
	.do_chk_before_pt       = fd_before_populate_token,
	.do_wrt                 = fd_do_write_by_token,
	.do_wzrt                = fd_do_write_by_zero_rod_token,
	.do_chk_before_wrt      = fd_before_write_by_token,
	.do_receive_rt          = fd_receive_rod_token,
#endif
#endif /* defined(CONFIG_MACH_QNAPTS) */
};

static int __init fileio_module_init(void)
{
	return transport_subsystem_register(&fileio_template);
}

static void fileio_module_exit(void)
{
	transport_subsystem_release(&fileio_template);
}

MODULE_DESCRIPTION("TCM FILEIO subsystem plugin");
MODULE_AUTHOR("nab@Linux-iSCSI.org");
MODULE_LICENSE("GPL");

module_init(fileio_module_init);
module_exit(fileio_module_exit);
