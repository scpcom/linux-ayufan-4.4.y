/**
 * $Header:
 *
 * Copyright (c) 2014 QNAP SYSTEMS, INC.
 * All Rights Reserved.
 *
 * $Id:
 */
/**
 * @file	target_fast_clone.h
 * @brief	This file contains declaration for basic structures / functions
 *		of the QANP fast block clone
 *
 * @author	Adam Hsu
 * @date	2014/01/20
 */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include "target_core_iblock.h"
#include "target_core_file.h"
#include "vaai_target_struc.h"
#include "target_fast_clone.h"

/**/
static void __dump_fc_obj(FC_OBJ *fc_obj);
struct block_device *__get_blk_dev(LIO_SE_DEVICE *se_dev);


/**/
static void __dump_fc_obj(
	FC_OBJ *fc_obj
	)
{
	printk("[fbc] rc LBA:0x%llx\n", (unsigned long long)fc_obj->s_lba);
	printk("[fbc] dest LBA:0x%llx\n",(unsigned long long)fc_obj->d_lba);
	printk("[fbc] transfer bytes:0x%llx\n", 
			(unsigned long long)fc_obj->data_bytes);

	printk("[fbc] src dev bs:0x%x\n", fc_obj->s_dev_bs);
	printk("[fbc] dest dev bs:0x%x\n", fc_obj->d_dev_bs);
	return;
}

/*
 * @fn struct block_device *__get_blk_dev (LIO_SE_DEVICE *se_dev)
 * @brief simple function to get struct block_device from LIO se dev
 *
 * @sa 
 * @param[in] se_dev
 * @retval  pointer of block_device
 */
struct block_device *__get_blk_dev(
	LIO_SE_DEVICE *se_dev
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *inode = NULL;
	struct iblock_dev *ibd = NULL;
	struct block_device *blk_dev = NULL;
	SUBSYSTEM_TYPE type;
	int ret;

	ret = __do_get_subsystem_dev_type(se_dev, &type);
	if (ret)
		return NULL;

	if (type == SUBSYSTEM_BLOCK){
		ibd = (LIO_IBLOCK_DEV *)se_dev->dev_ptr;
		blk_dev = ibd->ibd_bd;
	}
	else{
		fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
		inode = fd_dev->fd_file->f_mapping->host;
		if (S_ISBLK(inode->i_mode))
			blk_dev = inode->i_bdev;
		else
			return NULL;
	}
	return blk_dev;
}

static int __do_flush_and_drop(
	LIO_SE_DEVICE *se_dev,
	sector_t lba,
	u32 bs_order,
	u64 data_bytes
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct file *fd = NULL;
	struct inode *inode = NULL;
	struct address_space *mapping = NULL;
	int ret = 0;
	loff_t first_page = 0, last_page = 0, start = 0, len = 0;
	loff_t first_page_offset = 0, last_page_offset = 0;

#if defined(SUPPORT_TP)
	int err_1;
#endif


	fd_dev =  (LIO_FD_DEV *)se_dev->dev_ptr;
	fd = fd_dev->fd_file;

	/**/
	inode = fd->f_mapping->host;
	mapping = inode->i_mapping;

	if (!S_ISBLK(inode->i_mode)){
		pr_debug("[fbc] front-end is file i/o but back-end is NOT block dev\n");
		return -EOPNOTSUPP;
	}

//	pr_info("[fbc] flush bdev: %s\n", inode->i_bdev->bd_disk->disk_name);
	start = (loff_t)(lba << bs_order);
	len   = (loff_t)data_bytes;

	first_page = (start >> PAGE_CACHE_SHIFT);
	last_page = ((start + len - 1) >> PAGE_CACHE_SHIFT);
	first_page_offset = first_page	<< PAGE_CACHE_SHIFT;
	last_page_offset = (last_page << PAGE_CACHE_SHIFT) + (PAGE_CACHE_SIZE - 1);

#if 0
	pr_info("start:0x%llx\n", (unsigned long long)start);
	pr_info("len:0x%llx\n", (unsigned long long)len);
	pr_info("first_page:0x%llx\n", (unsigned long long)first_page);
	pr_info("last_page:0x%llx\n", (unsigned long long)last_page);
	pr_info("first_page_offset:0x%llx\n", (unsigned long long)first_page_offset);
	pr_info("last_page_offset:0x%llx\n", (unsigned long long)last_page_offset);
#endif

	if (mapping->nrpages && mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)){
		ret = filemap_write_and_wait_range(mapping, 
			first_page_offset, last_page_offset);
		
		if (unlikely(ret)) {
			pr_err("%s: fail to exec "
				"filemap_write_and_wait_range(), ret:%d\n", 
				__func__, ret);


#if defined(SUPPORT_TP)

			if (!is_thin_lun(se_dev))
				return ret;
			if (ret != -ENOSPC) {
				err_1 = check_dm_thin_cond(inode->i_bdev);
				if (err_1 == -ENOSPC){
					pr_warn("%s: space was full\n", __func__); 
					ret = err_1;
				}
			} else
				pr_warn("%s: space was full\n", __func__); 

#endif

			return ret;
		}
	}


	truncate_pagecache_range(inode, first_page_offset, last_page_offset);

#if 0
	pr_info("[fbc] flush / drop cache, start=0x%llx, len=0x%llx, "
		"first_page=0x%llx, last_page=0x%llx, "
		"first_page_offset=0x%llx, last_page_offset=0x%llx\n", 
		(unsigned long long)start, (unsigned long long)len, 
		(unsigned long long)first_page, (unsigned long long)last_page, 
		(unsigned long long)first_page_offset, 
		(unsigned long long)last_page_offset);
#endif
	return 0;

}


/*
 * @fn static int __do_fast_block_clone(FC_OBJ *fc_obj)
 * @brief function to execute fast block cline
 * @note this function ONLY works on file i/o + block backend device NOW
 *
 * @sa 
 * @param[in] fc_obj - obj contains the fast-clone information
 * @retval - total blks which were done (the blk unit is depend on dest dev)
 */
int __do_fast_block_clone(
	FC_OBJ *fc_obj
	)
{
	LIO_FD_DEV *s_fd_dev = NULL, *d_fd_dev = NULL;
	struct inode *s_inode = NULL, *d_inode = NULL;
	struct list_head io_lists;
	int ret, ret1, done_blks = 0, curr_done_blks_512 = 0;
	CREATE_REC create_rec;
	u32 s_bs_order, d_bs_order;
	sector_t s_lba = fc_obj->s_lba, d_lba = fc_obj->d_lba;
	sector_t s_lba_512, d_lba_512;

	/* the data_bytes range will be in 
	 * 0 ~ (OPTIMAL_TRANSFER_SIZE_IN_BYTES - 1) */
	u64 data_bytes = fc_obj->data_bytes;

	/**/
//	__dump_fc_obj(fc_obj);

	s_fd_dev = (LIO_FD_DEV *)fc_obj->s_se_dev->dev_ptr;
	d_fd_dev = (LIO_FD_DEV *)fc_obj->d_se_dev->dev_ptr;
	s_inode = s_fd_dev->fd_file->f_mapping->host;
	d_inode = d_fd_dev->fd_file->f_mapping->host;
	s_bs_order = ilog2(fc_obj->s_dev_bs);
	d_bs_order = ilog2(fc_obj->d_dev_bs);

_EXEC_AGAIN:

	/* To flush and drop the cache data for src / dest first since we will
	 * ask block layer to do fast block clone
	 */
	ret1 = __do_flush_and_drop(fc_obj->s_se_dev, s_lba, s_bs_order, data_bytes);
	ret = __do_flush_and_drop(fc_obj->d_se_dev, d_lba, d_bs_order, data_bytes);

	if (ret != 0 || ret1 != 0){

		pr_warn("[fbc] fail to flush / drop cache for %s area\n",
			(((ret != 0 && ret1 != 0)) ? "src/dest" : \
			((ret1 != 0) ? "src": "dest"))
			);
		
#if defined(SUPPORT_TP)
		/* workaround to check whether thin space is full */
		if (ret == -ENOSPC)
			fc_obj->nospc_err= 1;
#endif
		return done_blks;
	}

	/* prepare the fbc io, to conver to 512b unit first */
	s_lba_512 = ((s_lba << s_bs_order) >> 9);
	d_lba_512 = ((d_lba << d_bs_order) >> 9);

	INIT_LIST_HEAD(&io_lists);

	create_rec.s_blkdev = s_inode->i_bdev;
	create_rec.d_blkdev = d_inode->i_bdev;
	create_rec.s_lba = s_lba_512;
	create_rec.d_lba = d_lba_512;
	create_rec.transfer_bytes = data_bytes;

	/* After to create io lists, the create_rec.transfer_bytes may NOT
	 * the same as data_bytes  */
	ret =__create_fast_clone_io_lists(&io_lists, &create_rec);
	if (ret != 0)
		return 0;

	ret = __submit_fast_clone_io_lists_wait(&io_lists);


	/* After to submit io, the done_blks (unit is 512b) may 
	 * NOT the same as (create_rec.transfer_bytes >> 9) */
	curr_done_blks_512 = __get_done_blks_by_fast_clone_io_lists(&io_lists);
	done_blks += ((curr_done_blks_512 << 9) >> d_bs_order);
	__free_fast_clone_io_lists(&io_lists);


	/* (ret != 0) contains the case about cb_data.io_err_count is not zero */
	if ((ret != 0) || ((curr_done_blks_512 << 9) != create_rec.transfer_bytes)){
		if (ret == -ENOSPC)
			fc_obj->nospc_err = 1;
		return done_blks;
	}

	s_lba += (sector_t)done_blks;
	d_lba += (sector_t)done_blks;

	if (create_rec.transfer_bytes != data_bytes){
		data_bytes -= create_rec.transfer_bytes;
		goto _EXEC_AGAIN;
	}
	return done_blks;

}


/*
 * @fn int __submit_fast_clone_io_lists_wait(struct list_head *io_lists, FCCB_DATA *cb)
 * @brief create the fast clone io lists
 *
 * @sa 
 * @param[in] io_lists - fast clone io lists
 * @param[in] cb - global fast clone cb data
 * @retval  0: Success, 1: Fail
 */
int __submit_fast_clone_io_lists_wait(
	struct list_head *io_lists
	)
{
#define MAX_USEC_T	(1000)
	DECLARE_COMPLETION_ONSTACK(fast_io_wait);
	FCIO_REC *rec = NULL;
	unsigned long t0;
	int ret;
	u32 t1;
	FCCB_DATA cb;
	THIN_BLOCKCLONE_DESC *desc;

	__init_fccb_data(&cb, &fast_io_wait);
	t0 = jiffies;

	/* submit fast copy io */
	list_for_each_entry(rec, io_lists, io_node){
		pr_debug("[fbc] src_block_addr:0x%llx, dest_block_addr:0x%llx,"
			"transfer_blocks:0x%x\n",
			(unsigned long long)rec->desc.src_block_addr, 
			(unsigned long long)rec->desc.dest_block_addr,
			rec->desc.transfer_blocks
			);

		desc = &rec->desc;
		desc->private_data = &cb;
		atomic_inc(&cb.io_done);
		ret = thin_do_block_cloning(&rec->desc, (void *)__fast_clone_cb);

		/* thin_do_block_cloning() will return -ENOMEM and 0 */
		if (ret != 0){
			pr_warn("[%s]: thin_do_block_cloning() return %d\n",ret);
			__fast_clone_cb(ret, &rec->desc);
		}
	}

	/* to wait the all io if possible */
	if (!atomic_dec_and_test(&cb.io_done)){
		while (wait_for_completion_timeout(&fast_io_wait, 
				msecs_to_jiffies(FAST_CLONE_TIMEOUT_SEC * 1000)
				) == 0)
			printk("[fbc] wait fast copy io to be done\n");
	}

	/* check and print the io time */
	t1 = jiffies_to_usecs(jiffies - t0);
	if (t1 > MAX_USEC_T)
		pr_debug("[fbc] diff time: %d (usec)\n", t1);

	if (atomic_read(&cb.io_err_count)){
		pr_debug("[fbc] err in %s\n", __FUNCTION__);
		if (cb.nospc_err)
			return -ENOSPC;
		else
			return -EIO;
	}
	return 0;
}

/*
 * @fn int __create_fast_clone_io_lists(struct list_head *io_lists, CREATE_REC *create_rec)
 * @brief create the fast clone io lists
 *
 * @sa 
 * @param[in] io_lists - fast clone io lists
 * @param[in] create_rec - create record data
 * @retval  0: Success, 1: Fail
 */
int __create_fast_clone_io_lists(
	struct list_head *io_lists, 
	CREATE_REC *create_rec
	)
{
	FCIO_REC *io_rec;
	u64 tmp_bytes, buf_size, tmp_total;
	sector_t s_lba, d_lba;

	if (!io_lists || !create_rec)
		BUG_ON(TRUE);

	tmp_total = 0;
	tmp_bytes = create_rec->transfer_bytes;
	s_lba = create_rec->s_lba;
	d_lba = create_rec->d_lba;

	while (tmp_bytes){
		buf_size= min_t (u64, tmp_bytes, MAX_FBC_IO);

		io_rec = kzalloc(sizeof(FCIO_REC), GFP_KERNEL);
		if (!io_rec)
			break;

		/* prepare io_rec */
		io_rec->desc.src_dev = create_rec->s_blkdev;
		io_rec->desc.dest_dev = create_rec->d_blkdev;

		/* unit is 512b */
		io_rec->desc.src_block_addr = s_lba;
		io_rec->desc.dest_block_addr = d_lba;
		io_rec->desc.transfer_blocks = (buf_size >> 9);

		io_rec->io_done = 0;
		INIT_LIST_HEAD(&io_rec->io_node);
	
		/* put the io req to lists */
		list_add_tail(&io_rec->io_node, io_lists);		

		s_lba += (buf_size >> 9);
		d_lba += (buf_size >> 9);

		tmp_total += buf_size;
		tmp_bytes -= buf_size;
	}

	if (list_empty(io_lists))
		return 1;

	/* if not create fully, to update transfer_bytes to REAL bytes */
	if (tmp_bytes)
		create_rec->transfer_bytes = tmp_total;

	return 0;
}

/*
 * @fn void __free_fast_clone_io_lists (struct list_head *io_lists)
 * @brief free the fast clone io lists
 *
 * @sa 
 * @param[in] io_lists - fast clone io lists
 * @retval  NONE
 */
void __free_fast_clone_io_lists(
	struct list_head *io_lists
	)
{
	FCIO_REC *rec = NULL, *tmp_rec = NULL;

	list_for_each_entry_safe(rec, tmp_rec, io_lists, io_node)
		kfree(rec);
	return;
} 

/*
 * @fn void __fast_clone_cb (int err, THIN_BLOCKCLONE_DESC *clone_desc)
 * @brief callback function of fast clone
 *
 * @sa 
 * @param[in] err - err code passed by caller
 * @param[in] clone_desc - data for fast clone information
 * @retval  NONE
 */
void __fast_clone_cb(
	int err,
	THIN_BLOCKCLONE_DESC *clone_desc
	)
{
	FCIO_REC *rec = NULL;
	FCCB_DATA *cb = NULL;

	rec = container_of(clone_desc, FCIO_REC, desc);
	cb = (FCCB_DATA *)clone_desc->private_data;


	/**/
	rec->io_done = 1;

	if (err != 0){
		if (err != -EINVAL)
			pr_err("%s: err:%d\n", __func__, err);

		if (err == -ENOSPC)
			cb->nospc_err = 1;

		atomic_inc(&cb->io_err_count);
		smp_mb__after_atomic_inc();
	}
	
	/* If there is any error which were set before, here will set to done 
	 * to -1 even if current status is w/o any error */
	if (atomic_read(&cb->io_err_count))
	    rec->io_done = -1;


	if (atomic_dec_and_test(&cb->io_done)){
		pr_debug("[fbc] all io was done, to compete them\n");
		complete(cb->io_wait);
	}
	return;
	
}

/*
 * @fn void __init_fccb_data (FCCB_DATA *fccb_data, struct completion *io_wait)
 * @brief function to initialize the global fast clone cb data
 *
 * @sa 
 * @param[in] fccb_data - global fast clone cb data
 * @param[in] io_wait
 * @retval  NONE
 */
void __init_fccb_data(
	FCCB_DATA *fccb_data, 
	struct completion *io_wait
	)
{
	fccb_data->io_wait = io_wait;
	fccb_data->nospc_err = 0;
	atomic_set(&fccb_data->io_done, 1);
	atomic_set(&fccb_data->io_err_count, 0);
	return;
}

/*
 * @fn u32 __get_done_blks_by_fast_clone_io_lists (struct list_head *io_lists)
 * @brief function to return total blks which were done w/o any error
 *
 * @sa 
 * @param[in] io_lists - fast-clone io lists
 * @retval  total blks
 */
u32 __get_done_blks_by_fast_clone_io_lists(
	struct list_head *io_lists
	)
{
	FCIO_REC *rec = NULL;
	u32 done = 0;

	list_for_each_entry(rec, io_lists, io_node){
		if (rec->io_done == 1)
			done += rec->desc.transfer_blocks;
	}

	/* the return done blks unit is 512b */
	pr_debug("[fbc] in %s, done blks (512b):0x%x\n", __FUNCTION__, done);
	return done;
}


int __do_check_backend_blkdev(
	FC_OBJ *fc_obj,
	SUBSYSTEM_TYPE frondend_type,
	int is_blk_dev
	)
{
	LIO_FD_DEV *s_fd_dev = NULL, *d_fd_dev = NULL;
	struct inode *s_inode = NULL, *d_inode = NULL;
	SUBSYSTEM_TYPE s_type, d_type;
	int ret;

	/* (1) currently, the fast block clone work on backend block dev 
	 * (2) now is for frontend file i/o + backend block dev
	 */
	if (frondend_type > SUBSYSTEM_FILE)
		return -ENOTSUPP;

	ret = __do_get_subsystem_dev_type(fc_obj->s_se_dev, &s_type);
	if (ret)
		return -ENOTSUPP;
	 
	ret = __do_get_subsystem_dev_type(fc_obj->d_se_dev, &d_type);
	if (ret)
		return -ENOTSUPP;

	/* FIXED ME
	 *
	 * Fast clone ONLY work on dm-thin block dev now. In the other words,
	 * it works on file i/o + dm-thin block dev currently (this may be
	 * changed in the future)
	 */
	if ((s_type != frondend_type) || (d_type != frondend_type))
		return -ENOTSUPP;

	if (frondend_type == SUBSYSTEM_BLOCK){
		if (!is_blk_dev)
			return -ENOTSUPP;			
	}
	else {
		/* check the backend device type again */
		s_fd_dev = (LIO_FD_DEV *)fc_obj->s_se_dev->dev_ptr;
		d_fd_dev = (LIO_FD_DEV *)fc_obj->d_se_dev->dev_ptr;
		s_inode = s_fd_dev->fd_file->f_mapping->host;
		d_inode = d_fd_dev->fd_file->f_mapping->host;
		if (!S_ISBLK(s_inode->i_mode) || !S_ISBLK(d_inode->i_mode))
			return -ENOTSUPP;
	}
	return 0;
}

void __create_fbc_obj(
	FC_OBJ *fc_obj,
	LIO_SE_DEVICE *s_se_dev,
	LIO_SE_DEVICE *d_se_dev,
	sector_t s_lba,
	sector_t d_lba,
	u64 data_bytes
	)
{
	fc_obj->s_se_dev = s_se_dev;
	fc_obj->d_se_dev = d_se_dev;
	fc_obj->s_dev_bs = s_se_dev->se_sub_dev->se_dev_attrib.block_size;
	fc_obj->d_dev_bs = d_se_dev->se_sub_dev->se_dev_attrib.block_size;
	fc_obj->s_lba = s_lba;
	fc_obj->d_lba = d_lba;
	fc_obj->data_bytes = data_bytes;
	fc_obj->nospc_err = 0;
	return;
}

/*
 * @fn int __check_s_d_lba_before_fbc(FC_OBJ *fc_obj)
 *
 * @brief pre check whether the SRC/DEST lba are suitable for fast block clone
 *
 * @sa 
 * @param[in] fc_obj
 * @retval  0: GOOD , 1: BAD
 */
int __check_s_d_lba_before_fbc(
	FC_OBJ *fc_obj
	)
{
	sector_t s_lba, d_lba;
	u32 ps_blks, s_bs_order, d_bs_order;

	/* FIXED ME
	 *
	 * now, the BOTH src, dest LBA shall be on alignment or non-alignment
	 * address before to do fast block clone
	 */

	/* convert to 512b unit first */
	ps_blks = ((POOL_BLK_SIZE_KB << 10) >> 9);
	s_bs_order = ilog2(fc_obj->s_dev_bs);
	d_bs_order = ilog2(fc_obj->d_dev_bs);
	s_lba = ((fc_obj->s_lba << s_bs_order) >> 9);
	d_lba = ((fc_obj->d_lba << d_bs_order) >> 9);

	if (((s_lba & (ps_blks -1)) && (d_lba & (ps_blks -1)))
	|| ((!(s_lba & (ps_blks -1))) && (!(d_lba & (ps_blks -1))))
	)
		return 0;

	pr_debug("[fbc] %s - ori src lba (512b unit) :0x%llx, "
		"ori dest lba (512b unit) :0x%llx\n", __FUNCTION__, 
		(unsigned long long)s_lba, (unsigned long long)d_lba);

	pr_debug("[fbc] LBA address is bad\n");
	return 1;

}


/*
 * @fn int __do_check_support_fbc(FC_OBJ *fc_obj, TBC_DESC_DATA *tbcd_data)
 *
 * @brief function to check whether to support run fast block clone
 *
 * @sa 
 * @param[in] fc_obj
 * @param[in,out] tbcd_data
 * @retval  0: support to run fast block clone , others: Not support
 */
int __do_check_support_fbc(
	FC_OBJ *fc_obj,
	TBC_DESC_DATA *tbcd_data
	)
{
	/* when code comes here, the src and dest address shall be all aligned
	 * or not all aligned (after to call __check_s_d_lba_before_fbc func)
	 */
	sector_t o_ss_lba, o_se_lba, o_ds_lba, o_de_lba;
	sector_t n_ss_lba, n_ds_lba;
	u32 data_blks, ps_blks = ((POOL_BLK_SIZE_KB << 10) >> 9);
	u32 s_bs_order, d_bs_order, ps_blk_order, diff_blks;
	unsigned long ret_bs = 0;
	int ret = -ENOTSUPP;

	/**/
	tbcd_data->do_fbc = 0;
	ps_blk_order = ilog2(ps_blks);
	s_bs_order = ilog2(fc_obj->s_dev_bs);
	d_bs_order = ilog2(fc_obj->d_dev_bs);

	if ((fc_obj->data_bytes >> 9) < ps_blks){
		pr_debug("[fbc] total data blks (512b unit):0x%llx "
			"< ps_blks (512b unit):0x%x\n",
			(unsigned long long)(fc_obj->data_bytes >> 9),
			ps_blks
			);
		goto _EXIT_;
	}

	data_blks = (fc_obj->data_bytes >> 9);
	o_ss_lba = ((fc_obj->s_lba << s_bs_order) >> 9);
	o_se_lba = (o_ss_lba + data_blks - 1);
	o_ds_lba = ((fc_obj->d_lba << d_bs_order) >> 9);
	o_de_lba = (o_ds_lba + data_blks - 1);

	pr_debug("[fbc] === ori data ===\n");
	pr_debug("[fbc] o_ss_lba (512b unit):0x%llx\n", (unsigned long long)o_ss_lba);
	pr_debug("[fbc] o_se_lba (512b unit):0x%llx\n", (unsigned long long)o_se_lba);
	pr_debug("[fbc] o_ds_lba (512b unit):0x%llx\n", (unsigned long long)o_ds_lba);
	pr_debug("[fbc] o_de_lba (512b unit):0x%llx\n", (unsigned long long)o_de_lba);
	pr_debug("[fbc] data blks (512b unit):0x%x\n", data_blks);


	/* to convert to 512b unit first and run 1st round to get
	 * alignment address of SRC */
	n_ss_lba =  (((o_ss_lba + ps_blks - 1) >> ps_blk_order) \
		<< ps_blk_order);

	diff_blks = n_ss_lba - o_ss_lba;
	n_ds_lba = o_ds_lba + diff_blks;

	if (n_ds_lba & (ps_blks -1)){
		pr_debug("[fbc] new address doesn't aligned by 0x%x, "
			"new src:0x%llx, new dest:0x%llx, to skip fbc...\n",
			ps_blks, (unsigned long long)n_ss_lba, 
			(unsigned long long)n_ds_lba
			);
		goto _EXIT_;
	}

	/* final chance to check new data blks is multipled by ps_blks or not */
	data_blks = (o_se_lba - n_ss_lba + 1);
	if (data_blks < ps_blks){
		pr_debug("[fbc] the blks between o_se_lba and n_ss_lba is small"
			"then ps_blks\n");
		goto _EXIT_;
	}else if (data_blks & (ps_blks -1))
		data_blks = ((data_blks >> ps_blk_order) << ps_blk_order);

	/* now to ask whether we can do fast block clone or not */
	tbcd_data->tbc_desc.src_dev = __get_blk_dev(fc_obj->s_se_dev);
	tbcd_data->tbc_desc.dest_dev = __get_blk_dev(fc_obj->d_se_dev);
	tbcd_data->tbc_desc.src_block_addr = n_ss_lba;
	tbcd_data->tbc_desc.dest_block_addr = n_ds_lba;
	tbcd_data->tbc_desc.transfer_blocks = data_blks;
	tbcd_data->tbc_desc.private_data = NULL;

	if (!tbcd_data->tbc_desc.src_dev || !tbcd_data->tbc_desc.dest_dev){
		pr_debug("[fbc] not found block device for src or dest\n");
		ret = -ENODEV;
		goto _EXIT_;
	}

	pr_debug("[fbc] === new data ===\n");
	pr_debug("[fbc] n_ss_lba (512b unit):0x%llx\n", (unsigned long long)n_ss_lba);
	pr_debug("[fbc] n_ds_lba (512b unit):0x%llx\n", (unsigned long long)n_ds_lba);
	pr_debug("[fbc] new data blks (512b unit):0x%x\n", data_blks);

	/* now to ask whether we can do fast block clone or not */
	tbcd_data->tbc_desc.src_dev = __get_blk_dev(fc_obj->s_se_dev);
	tbcd_data->tbc_desc.dest_dev = __get_blk_dev(fc_obj->d_se_dev);
	tbcd_data->tbc_desc.src_block_addr = n_ss_lba;
	tbcd_data->tbc_desc.dest_block_addr = n_ds_lba;
	tbcd_data->tbc_desc.transfer_blocks = data_blks;

	if (thin_support_block_cloning(&tbcd_data->tbc_desc, &ret_bs) != 0){
		pr_debug("fail to call thin_support_block_cloning(), "
			"ret_bs:0x%llx\n", (unsigned long long)ret_bs);
		goto _EXIT_;
	}

	if (ret_bs != POOL_BLK_SIZE_KB)
		pr_debug("warning: ret_bs != POOL_BLK_SIZE_KB\n");

	tbcd_data->do_fbc = 1;

	/* convert to original unit */
	tbcd_data->h_d_lba = fc_obj->d_lba;

	tbcd_data->d_align_lba = ((n_ds_lba << 9) >> d_bs_order);
	tbcd_data->data_bytes = ((u64)data_blks << 9);

	tbcd_data->t_d_lba = (tbcd_data->d_align_lba + \
		(tbcd_data->data_bytes >> d_bs_order));

	pr_debug("[fbc] === tcbd data ===\n");
	pr_debug("[fbc] h_d_lba:0x%llx\n", (unsigned long long)tbcd_data->h_d_lba);
	pr_debug("[fbc] d_align_lba:0x%llx\n", (unsigned long long)tbcd_data->d_align_lba);
	pr_debug("[fbc] data_bytes:0x%llx\n", (unsigned long long)tbcd_data->data_bytes);
	pr_debug("[fbc] t_d_lba:0x%llx\n", (unsigned long long)tbcd_data->t_d_lba);

	ret = 0;

_EXIT_:
	if (ret != 0)
		pr_debug("[fbc] %s - ret: %d\n", __FUNCTION__, ret);
	
	return ret;
}

/*
 * @fn int __do_update_fbc_data_bytes(FC_OBJ *fc_obj, TBC_DESC_DATA *tbcd_data, 
 *		sector_t dest_lba, u64 *data_bytes)
 *
 * @brief function to update data bytes for fast block clone
 *
 * @sa 
 * @param[in] fc_obj
 * @param[in] tbcd_data
 * @param[in] dest_lba
 * @param[in,out] data_bytes
 * @retval  0: NOT support fast block clone, 1: update successfully and to
 *		run fast block clone
 */
int __do_update_fbc_data_bytes(
	FC_OBJ *fc_obj,
	TBC_DESC_DATA *tbcd_data, 
	sector_t dest_lba,
	u64 *data_bytes
	)
{
	int ret = 0;
	u32 d_bs_order;

	if (!tbcd_data->do_fbc)
		return ret;

	d_bs_order = ilog2(fc_obj->d_dev_bs);

	if (dest_lba == tbcd_data->d_align_lba){
		*data_bytes = tbcd_data->data_bytes;
		ret = 1;
	}
	else{
		/* case for normal IO. only handle the case about
		 * "dest_lba < tbcd_data.d_align_lba"
		 */
		if (dest_lba < tbcd_data->d_align_lba)
			*data_bytes = ((tbcd_data->d_align_lba - dest_lba) \
				<< d_bs_order); 
	}

	pr_debug("[fbc] %s - ret:%d, dest:0x%llx, d_align_lba:0x%llx, "
		"real data bytes:0x%llx\n", __FUNCTION__, ret,
		(unsigned long long)dest_lba,
		(unsigned long long)tbcd_data->d_align_lba,
		(unsigned long long)*data_bytes
		);

	return ret;
}




int quick_check_support_fbc(
	struct block_device *bd
	)
{

	THIN_BLOCKCLONE_DESC tbc_desc;
	unsigned long ret_bs = 0;
	sector_t s_lba, d_lba;
	u32 data_blks;
	

	if (!bd)
		return -EINVAL;

	/* quick check for file-based lun (fbdisk) */
	if (!strncmp(bd->bd_disk->disk_name, "fbdisk", 6)){
		pr_warn("not support fast clone on fbdisk dev\n");
		return -ENOTSUPP;
	}

	/* unit is 512b */
	data_blks = ((POOL_BLK_SIZE_KB << 10) >> 9);
	s_lba = 0;
	d_lba = s_lba + data_blks;

	tbc_desc.src_dev = bd;
	tbc_desc.dest_dev = bd;
	tbc_desc.src_block_addr = s_lba;
	tbc_desc.dest_block_addr = d_lba;
	tbc_desc.transfer_blocks = data_blks;

	if (thin_support_block_cloning(&tbc_desc, &ret_bs) != 0){
		pr_warn("not support fast clone on dev: %s\n",
			bd->bd_disk->disk_name);
		return -ENOTSUPP;
	}

	return 0;
}
EXPORT_SYMBOL(quick_check_support_fbc);
#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TPC_CMD) 

/*
 * @fn int __do_wt_by_fbc(WBT_OBJ *wbt_obj, FC_OBJ *fc_obj, 
 *		u64 *data_bytes, ERR_REASON_INDEX *err )
 *
 * @brief function to do WRITE by TOKEN  via fast block clone
 *
 * @sa 
 * @param[in] wbt_obj
 * @param[in] fc_obj
 * @param[in] data_bytes
 * @retval  0: Success, 1: Fail
 */
int __do_wt_by_fbc(
	WBT_OBJ *wbt_obj,
	FC_OBJ *fc_obj,
	u64 *data_bytes
	)
{
	int w_done_blks;
	u64 e_bytes = *data_bytes;

	/* FIXED ME */
	w_done_blks = __do_fast_block_clone(fc_obj);

	if ((e_bytes >> wbt_obj->d_obj->dev_bs_order) != (u64)w_done_blks){
		pr_debug("[fbc] tpc - w_done_blks != expected write blks\n");
		if (fc_obj->nospc_err){
			wbt_obj->err = ERR_NO_SPACE_WRITE_PROTECT;

			pr_err("[fbc] %s: space was full\n", __func__);
		}
		else
			wbt_obj->err = ERR_3RD_PARTY_DEVICE_FAILURE;

		return 1;
	}

	return 0;
}
#endif

#if defined(SUPPORT_VAAI)
int __do_b2b_xcopy_by_fbc(
	B2B_XCOPY_OBJ *b2b_obj,
	FC_OBJ *fc_obj,
	u64 *data_bytes
	)
{
	int w_done_blks;
	u64 e_bytes = *data_bytes;

	w_done_blks = __do_fast_block_clone(fc_obj);

	if ((e_bytes >> b2b_obj->d_bs_order) != (u64)w_done_blks){
		pr_debug("[fbc] b2b xcopy - w_done_blks != expected write blks\n");

		if (fc_obj->nospc_err){
			b2b_obj->err = ERR_NO_SPACE_WRITE_PROTECT;
			pr_err("[fbc] %s: space was full\n", __func__);
		}
		else
			b2b_obj->err = ERR_3RD_PARTY_DEVICE_FAILURE;

		return 1;
	}

	return 0;

}
#endif




