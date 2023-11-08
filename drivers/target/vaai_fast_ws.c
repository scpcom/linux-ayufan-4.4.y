/**
 * @file 	vaai_fast_ws.c
 * @brief	This file contains the special function to process WRITE SAME command
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/falloc.h>
#include <linux/security.h>
#include <linux/bio.h>
#include <asm/unaligned.h>
#include <scsi/scsi.h>

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include "target_core_ua.h"
#include "target_core_iblock.h"
#include "target_core_file.h"

#include "vaai_target_struc.h"
#include "target_general.h"
#include "vaai_helper.h"



/*
 * @fn static int ib_do_special_discard (IN LIO_SE_DEVICE *dev, IN sector_t lba, IN u32 range)
 * @brief Function to do special discard request (discard first then to allocate again)
 *        and this function will be used in WRITE SAME W/O UNMAP currently
 *
 * @sa 
 * @param[in] cmd   - pointer to se cmd
 * @param[in] lba   - start lba to be discard
 * @param[in] range - range to be discard
 * @retval 0 - Success / Others - Fail
 */
static int ib_do_special_discard(
	IN LIO_SE_CMD *se_cmd, 
	IN sector_t lba, 
	IN u32 range
	)
{
	/* Here refer the blkdev_issue_discard().
	 *
	 * The most difference between iblock_do_discard() and ib_do_special_discard()
	 * is this function will dicard first then to allocate space where its range
	 * was discard at previous operation.
	 */
	struct iblock_dev *ibd = se_cmd->se_dev->dev_ptr;
	struct block_device *bd = ibd->ibd_bd;
	int barrier = 0;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
	/* The sector unit is 512 in block i/o layer of kernel, 
	 * so need to transfer it again */
	if (__blkio_transfer_task_lba_to_block_lba(
		se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size, &lba) != 0)
		return -EINVAL;
        
	range *= (se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size >> 9);
#endif

	return blkdev_issue_special_discard(bd, lba, range, GFP_KERNEL, barrier);
}

/*
 * @fn int vaai_block_do_specific_fast_ws(IN LIO_SE_CMD *pSeCmd)
 * @brief Wrapper function to depend on special_ws parameter to process WRITE SAME
 *        command of fast version for LIO block i/o
 *
 * @sa 
 * @param[in]  pSeCmd
 * @param[in]  special_ws
 * @retval  0 - Success / Others - Fail
 */
int vaai_block_do_specific_fast_ws(
	IN LIO_SE_CMD *pSeCmd,
	IN bool special_ws
	)
{
	LIO_SE_DEVICE *pSeDev = NULL;
	LIO_IBLOCK_DEV *pIBD = NULL;
	int (*discard_func)(LIO_SE_CMD *, sector_t, u32) = NULL;
	u32 u32NumBlocks = 0, block_size = 0;
	sector_t Lba = 0, Range = 0,RealRange = 0;
	int Ret = 0;

	/**/
	pSeDev = pSeCmd->se_dev;
	pIBD = (LIO_IBLOCK_DEV *)(pSeDev->dev_ptr);
	block_size = pSeDev->se_sub_dev->se_dev_attrib.block_size;

	/*
	 * If u8WriteSameNoZero = 0 and u32NumBlocks = 0 means the device 
	 * server shall write all of the logical blocks starting with the one
	 * specified in the LOGICAL BLOCK ADDRESS field to the last logical
	 * block on the medium
	 */
	__get_lba_and_nr_blks_ws_cdb(pSeCmd, &Lba, &u32NumBlocks);

	Range = (sector_t)u32NumBlocks;
	if ((u8WriteSameNoZero == 0) && (u32NumBlocks == 0))
		Range = pSeDev->transport->get_blocks(pSeDev) - Lba + 1;

	/* To check what kind of discard function we will use ... */
	if (!special_ws){
		if (!pSeDev->transport->do_discard) {
			pr_err("%s: subsystem device NOT support "
				"do_discard()\n", __func__);
			Ret = -EOPNOTSUPP;
		}else
			discard_func = pSeDev->transport->do_discard;
	}else
		discard_func = ib_do_special_discard;

	if (Ret == -EOPNOTSUPP)
		return Ret;

	/* Start to process the discard request ... */
	while (Range > 0) {

		RealRange = min_t(sector_t, Range, UINT_MAX);

		Ret = discard_func(pSeCmd, Lba, (u32)RealRange);
		if (Ret != 0) {
			pr_err("%s: fail to write to device, Ret:0x%x\n", 
				__func__, Ret);
			goto _ERR_;
		}
		Lba += RealRange;
		Range -= RealRange;
	}

	// Everything is fine ...
	goto _EXIT_;

_ERR_:
	if (Ret != -ENOSPC){
		__set_err_reason(ERR_CHECK_CONDITION_NOT_READY, 
			&pSeCmd->scsi_sense_reason);
		pSeCmd->scsi_asc    = 0x6e; // command to logical unit is fail
		pSeCmd->scsi_ascq   = 0x00;
	} else
		__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
			&pSeCmd->scsi_sense_reason);

_EXIT_:
	return Ret;
}

#if defined(SUPPORT_FILEIO_ON_FILE)
/* 201404xx, adamhsu, redmine 8043 */
static int __fio_filebackend_fast_ws(
	LIO_SE_CMD *se_cmd, 
	int special_ws
	)
{
	LIO_SE_DEVICE *se_dev = NULL;
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *i_node = NULL;
	int ret = 0;
	sector_t lba = 0, range = 0;
	loff_t off, len;
	u32 nr_blks, bs_order;

	/**/
	se_dev = se_cmd->se_dev;
	fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
	i_node = fd_dev->fd_file->f_mapping->host;
	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);

	if (!fd_dev->fd_file->f_op->fallocate){
		pr_err("[WS FIO FileBackend] Not support fallocate op\n");
		return -ENOTSUPP;
	}

	__get_lba_and_nr_blks_ws_cdb(se_cmd, &lba, &nr_blks);

	range = (sector_t)nr_blks;
	if((u8WriteSameNoZero == 0) && (nr_blks == 0))
		range = (se_dev->transport->get_blocks(se_dev) - lba) + 1;


	/* TODO
	 * (1) for UNMAP (or WRITE SAME w/ UNMAP), we do punch hole only
	 * (2) for WRITE SAME w/o UNMAP, we do punch hole first then
	 * create size again
	 */
	off = (loff_t)(lba << bs_order);
	len = (loff_t)(range << bs_order);

	pr_debug("[WS FIO FileBackend] off:0x%llx, len:0x%llx\n", 
		(unsigned long long)off, (unsigned long long)len);

	ret = fd_dev->fd_file->f_op->fallocate(fd_dev->fd_file,
		(FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE),
		off, len);

	if (ret != 0){
		pr_err("[WS FIO FileBackend] ret:%d after call fallocate op "
			"with punch hole + keep size\n", ret);
		goto _MAKE_SCSI_ERR_REASON_;
	}

	if (!special_ws)
		return ret;

	/* if it is WRITE SAME w/o UNMAP */
	ret = fd_dev->fd_file->f_op->fallocate(fd_dev->fd_file,
		(FALLOC_FL_KEEP_SIZE), off, len);

	if (ret != 0){
		pr_err("[WS FIO FileBackend] ret:%d after call fallocate op "
			"with keep size\n", ret);
		goto _MAKE_SCSI_ERR_REASON_;
	}

	return 0;

_MAKE_SCSI_ERR_REASON_:
	/* TODO */
	if (ret == -EFBIG || ret == -EINVAL)
		__set_err_reason(ERR_INVALID_CDB_FIELD, 
			&se_cmd->scsi_sense_reason);
	else if (ret == -ENOSPC)
		__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
			&se_cmd->scsi_sense_reason);
	else	
		__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&se_cmd->scsi_sense_reason);

	return ret;
}

#else

static int __fio_filebackend_fast_ws(
	LIO_SE_CMD *se_cmd, 
	int special_ws
	)
{
	/* NOT support file i/o + file backend device if NOT define
	 * SUPPORT_FILEIO_ON_FILE compile option
	 */
	pr_err("%s: Not support file i/o + file backend device\n", 
		__FUNCTION__);

	return -ENOTSUPP; 
}
#endif


/*
 * @fn int vaai_file_do_specific_fast_ws(IN LIO_SE_CMD *pSeCmd)
 * @brief Wrapper function to depend on special_ws parameter to process WRITE SAME
 *        command of fast version for LIO file i/o
 *
 * @sa 
 * @param[in]  pSeCmd
 * @param[in]  special_ws
 * @retval  0 - Success / Others - Fail
 */
int vaai_file_do_specific_fast_ws(
	IN LIO_SE_CMD *pSeCmd,
	IN bool special_ws
	)
{
#define DEFAULT_ALLOC_SIZE	(1 << 20)
#define DEFAULT_ALIGN_SIZE	(1 << 20)
#define NORMAL_IO_TIMEOUT	(5)	/* unit is second */

	LIO_SE_DEVICE *pSeDev = NULL;
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *pInode = NULL;
	struct address_space *mapping = NULL;
	sector_t t_lba = 0, t_range = 0, Lba = 0, Range = 0, RealRange = 0;
	u32 bs_order = 0, u32NumBlocks = 0, tmp;
	loff_t first_page = 0, last_page = 0, Start = 0, Len = 0;
	loff_t first_page_offset = 0, last_page_offset = 0;
	int (*discard_func)(struct block_device *, sector_t, 
		sector_t, gfp_t, unsigned long) = NULL;

	ALIGN_DESC align_desc;
	int normal_io = 1, Ret = 0;
	u64 alloc_bytes = DEFAULT_ALLOC_SIZE;
	GEN_RW_TASK w_task;

#if defined(SUPPORT_TP)
	int err_1;
#endif


	/* When code comes here, the subsystem type is file i/o, so to check
	 * what kind of backing device we need to handle ...
	 */
	pSeDev = pSeCmd->se_dev;
	fd_dev = (LIO_FD_DEV *)pSeDev->dev_ptr;
	pInode = fd_dev->fd_file->f_mapping->host;
	mapping = pInode->i_mapping;
	bs_order = ilog2(pSeDev->se_sub_dev->se_dev_attrib.block_size);

	if (!S_ISBLK(pInode->i_mode))
		return __fio_filebackend_fast_ws(pSeCmd, special_ws);

	/* To check what kind of discard function we will use ... */
	if (!special_ws){
		if (!pSeDev->transport->do_discard) {
			pr_err("%s: subsystem device NOT support"
				" discard()\n", __FUNCTION__);
			Ret = -EOPNOTSUPP;
		}else
			discard_func = blkdev_issue_discard;
	}else
		discard_func = blkdev_issue_special_discard;

	if (Ret == -EOPNOTSUPP)
		return Ret;

	/* If u8WriteSameNoZero = 0 and u32NumBlocks = 0 means the
	 * device server shall write all of the logical blocks starting
	 * with the one specified in the LOGICAL BLOCK ADDRESS field to
	 * the last logical block on the medium
	 */
	pr_debug("[fws]: LIO(file i/o) + block backend device\n");
	__get_lba_and_nr_blks_ws_cdb(pSeCmd, &Lba, &u32NumBlocks);

	Range = (sector_t)u32NumBlocks;
	if((u8WriteSameNoZero == 0) && (u32NumBlocks == 0))
		Range = (pSeDev->transport->get_blocks(pSeDev) - Lba) + 1;

	pr_debug("[fws] Lba:0x%llx, range:0x%x\n", (unsigned long long)Lba, 
			(u32)Range);

	/* To create the ws range desc. Its purpose is to make sure the
	 * ws range is aligned by specific size
	 */
	__create_aligned_range_desc(&align_desc, Lba, 
			Range, bs_order, DEFAULT_ALIGN_SIZE);

	/* create sg for normal io */
	memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));

	Ret = __generic_alloc_sg_list(&alloc_bytes, &w_task.sg_list, 
			&w_task.sg_nents);
	if (Ret != 0){
		if (Ret == -ENOMEM)
			pr_err("[fws] fail to alloc sg list\n");
		if (Ret == -EINVAL)
			pr_err("[fws] invalid arg during to alloc sg list\n");

		__set_err_reason(ERR_INSUFFICIENT_RESOURCES, 
			&pSeCmd->scsi_sense_reason);

		return Ret;
	}

	pr_debug("[fws] normal_io alloc_bytes:0x%llx\n", 
				(unsigned long long)alloc_bytes);

	/*  Start to process the discard request ... */
	while (Range > 0) {

		t_lba = Lba;
		t_range = RealRange = min_t(sector_t, Range, UINT_MAX);

		/* check which part we will process by normal io or discard io */
		if (align_desc.is_aligned){
			normal_io = 0;
			if (Lba == align_desc.lba)
				RealRange = align_desc.nr_blks;
			else {
				if (Lba < align_desc.lba)
					RealRange = align_desc.lba - Lba;
				normal_io = 1;
			}
		}

		pr_debug("[fws] normal_io:%d, Lba:0x%llx, RealRange:0x%x\n", 
			normal_io, (unsigned long long)Lba,(u32)RealRange);

		if (unlikely(normal_io))
			goto _NORMAL_IO_;

		/* If our product architecture is LIO(file i/o) + fbdisk driver
		 * (it manages the iscsi img file), the blkdev_issue_discard()
		 * function will affect the 2nd page cache layer in fbdisk.
		 *
		 * If the architecture is LIO(file i/o) + block backend device
		 * or LIO(file i/o) + file backend device, it only affect the
		 * 1st page cache layer (this combination is for original LIO
		 * architecture).
		 *
		 * Since we do discard on 2nd page cache layer in fbdisk, we need
		 * to make sure the coherence between 1st and 2nd page cache layer.
		 *
		 * The ext4_ext_punch_hole() is referenced to solve this issue. 
		 * Actually, ext4_ext_punch_hole() in kernel 3.4.6 will call 
		 * truncate_inode_pages_range() to release the pages but the latest
		 * version (3.5-rc7 or 3.7-rc8) will use truncate_pagecache_range().
		 * That's why truncate_pagecache_range() is used instead of 
		 * truncate_inode_pages_range().
		 *
		 * By the way, this solution is applied to not only the fbdisk, 
		 * but also all the block devices in the file HBA. 
		 *
		 * The verification is to write first then do write-same (zero buffer) 
		 * and read finally. The result of read should be the same as 
		 * write-same (zero buffer).
		 */
		Start = (loff_t)(Lba << bs_order);
		Len   = (loff_t)(RealRange << bs_order);
			
		first_page = (Start) >> PAGE_CACHE_SHIFT;
		last_page = (Start + Len - 1) >> PAGE_CACHE_SHIFT;	
		first_page_offset = first_page	<< PAGE_CACHE_SHIFT;
		last_page_offset = (last_page << PAGE_CACHE_SHIFT) + (PAGE_CACHE_SIZE - 1);
			
		/*
		 * Benjamin 20130206 for BUG 30207: 
		 * We should write the dirty page cache in first, then truncate the
		 * page cache.	
		 * Since the range of the page cache to be truncated may be different
		 * from that of being discarded. Write out all dirty pages to avoid 
		 * race conditions then release them.
		 */
		if (mapping->nrpages && mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)){
			Ret = filemap_write_and_wait_range(mapping, 
				first_page_offset, last_page_offset);
			
			if (unlikely(Ret)) {
				pr_err("[fws] fail to exec "
					"filemap_write_and_wait_range(), "
					"Ret:%d\n", Ret);		     

#if defined(SUPPORT_TP)
				/* 2015/01/29, adamhsu, bugzilla 48461 */
				if (special_ws){

					/* adamhsu, 2015/03/17, redmine 12305 */
					if (!is_thin_lun(pSeDev))
						break;

					if (Ret != -ENOSPC) {
						err_1 = check_dm_thin_cond(pInode->i_bdev);
						if (err_1 == -ENOSPC){
							Ret = err_1;
						}
					}
				}
#endif
				break;
			}
		}

		truncate_pagecache_range(pInode, first_page_offset, 
			last_page_offset);
	
		pr_debug("[fws] Start=%llu,Len=%llu, first_page=%llu, "
			"last_page=%llu, first_page_offset=%llu, "
			"last_page_offset=%llu.\n", Start, Len, 
			first_page, last_page, first_page_offset, last_page_offset);

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI) 
		/* the sector unit is 512 in block i/o layer o fkernel, so need
		 * to transfer it again 
		 */
		Ret = __blkio_transfer_task_lba_to_block_lba((1 << bs_order), &t_lba);
		if (Ret != 0)
			break;
		t_range = (RealRange * ((1 << bs_order) >> 9));
#endif

		pr_debug("[fws] t_lba:0x%llx, t_range:0x%x\n", 
			(unsigned long long)t_lba, (u32)t_range);

		Ret = discard_func(pInode->i_bdev, t_lba, (u32)t_range, 
			GFP_KERNEL, 0);
	
		if (unlikely(Ret)) {
			pr_err("%s: fail to exec discard_func() Ret:%d\n", 
				__FUNCTION__, Ret);
			break;
		} else
			goto _GO_NEXT_;

_NORMAL_IO_:
		/* The path for real io */
		t_lba = Lba;
		t_range = RealRange;
		do {
			tmp = min_t(u32, t_range,
					((u32)alloc_bytes >> bs_order));

			pr_debug("[fws] tmp for normal_io:0x%x\n", tmp);

			__make_rw_task(&w_task, pSeDev, t_lba, tmp,
				msecs_to_jiffies(NORMAL_IO_TIMEOUT * 1000), 
				DMA_TO_DEVICE);

			Ret = __do_f_rw(&w_task);

			pr_debug("[fws] after call __do_f_rw() Ret:%d, "
				"is_timeout:%d, ret code:%d\n",	Ret, 
				w_task.is_timeout, w_task.ret_code);

			if((Ret <= 0) || w_task.is_timeout || w_task.ret_code != 0){
				Ret = w_task.ret_code;
				if (w_task.is_timeout)
					Ret = 1;
				break;
			}
			Ret = 0;
			t_lba += tmp;
			t_range -= tmp;
		} while (t_range);

		/* break the loop if while (t_range) did not completed */
		if (t_range)
			break;
_GO_NEXT_:
		Lba += RealRange;
		Range -= RealRange;
	}

	__generic_free_sg_list(w_task.sg_list, w_task.sg_nents);

	if (Ret){
		if (Ret == -ENOSPC){
			pr_warn("[FAST WS FIO] thin space was full\n"); 
			__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
				&pSeCmd->scsi_sense_reason);
		} else
			__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
				&pSeCmd->scsi_sense_reason);
	}
	return Ret;

}



