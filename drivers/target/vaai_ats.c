/**
 * @file 	vaai_ats.c
 * @brief	This file contains the SCSI ATS (compare and write) code
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 */

#include <linux/kernel.h>
#include <linux/module.h>
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

//
//
//
static int vaai_block_do_ats(
	IN LIO_SE_CMD *pSeCmd
	);

static int vaai_file_do_ats(
	IN LIO_SE_CMD *pSeCmd
	);

static int vaai_ats_memcmp(
	IN const u8 *pSrc, 
	IN const u8 *pDest,
	IN u32 u32Counts,
	IN OUT u32 *pu32ErrOff
	);

//
//
//

/*
 * @fn static int vaai_ats_memcmp (IN const u8 *pSrc,IN const u8 *pDest, IN u32 u32Counts, IN OUT u32 *pu32ErrOff)
 * @brief Simple memory-compare function for ATS command
 *
 * @param[in] pSrc
 * @param[in] pDest
 * @param[in] u32Counts
 * @param[in,out] pu32ErrOff
 * @retval 0 - data is the same / 1 - data is not the same
 * @retval pu32ErrOff - pointer to buffer where will be put the error location value if return 1
 */
static int vaai_ats_memcmp(
	IN const u8 *pSrc, 
	IN const u8 *pDest, 
	IN u32 u32Counts, 
	IN OUT u32 *pu32ErrOff
	)
{
	u32 u32Off = 0;
	int Result = 1;

	if ((Result = memcmp(pSrc, pDest, (size_t)u32Counts))) {
		for (u32Off = 0; u32Off < u32Counts && *pSrc++ == *pDest++; u32Off++);
	}
	// No matter how, *pu32ErrOff should be equal to u32Off.
	*pu32ErrOff = u32Off;

	return Result;
}

static int vaai_block_do_ats(
    IN LIO_SE_CMD *pSeCmd
    )
{
	LIO_SE_DEVICE *pSeDev = NULL;
	GEN_RW_TASK r_task, w_task;
	sector_t lba;
	u32 u32tmp = 0, alloc_bytes = PAGE_SIZE, bs_order, done_blks;
	u8 *pu8VerifyBuf = NULL, *pu8WriteBuf = NULL, *pu8TmpBuf = NULL;
	struct scatterlist *sgl = NULL;
	struct page *page = NULL;
	int ret;

	/**/
	if(pSeCmd->t_task_cdb[13] > MAX_ATS_LEN){
		__set_err_reason(ERR_INVALID_CDB_FIELD, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	/**/
	memset((void *)&r_task, 0, sizeof(GEN_RW_TASK));
	memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));

	pSeDev = pSeCmd->se_dev;

	lba = get_unaligned_be64(&CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[2]);  
	bs_order = ilog2(pSeDev->se_sub_dev->se_dev_attrib.block_size);

	u32tmp = __call_transport_get_size(
			(u32)CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[13],
			CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb, 
			pSeCmd);

	pu8VerifyBuf  = transport_kmap_data_sg(pSeCmd);
	if (!pu8VerifyBuf){
		__set_err_reason(ERR_OUT_OF_RESOURCES, &pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	pu8WriteBuf = pu8VerifyBuf + (size_t)u32tmp;

	/* create tmp page and sg list for r_task */
	page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	sgl = kzalloc(GFP_KERNEL, sizeof(struct scatterlist));

	if (!page || !sgl){
		__set_err_reason(ERR_OUT_OF_RESOURCES, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	 }

	sg_init_table(sgl, 1);
	sg_set_page(sgl, page, PAGE_SIZE, 0);

	r_task.sg_list = sgl;
	r_task.sg_nents = 1;

	/* read first */
	__make_rw_task(&r_task, pSeDev, lba, (u32tmp >> bs_order),
		msecs_to_jiffies(5000), DMA_FROM_DEVICE);

	done_blks = __do_b_rw(&r_task);

	if (done_blks <= 0 || r_task.is_timeout || r_task.ret_code != 0){
		pr_err("[ATS BLKIO] fail to read\n");
		__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}
	if (done_blks != (u32tmp >> bs_order)){
		pr_err("[ATS BLKIO] byte counts != expected after to read\n");
		__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	/* then to compare */
	pu8TmpBuf = kmap(sg_page(sgl)) + sgl->offset;

	if (vaai_ats_memcmp(pu8VerifyBuf, pu8TmpBuf, u32tmp, 
		&pSeCmd->byte_err_offset))
	{
		pr_debug("[ATS BLKIO] compare conflict: read-lba(0x%llx), "
			"err-offset(0x%x) in verify-buffer\n",
			(unsigned long long)lba, 
			pSeCmd->byte_err_offset);
		
		__set_err_reason(ERR_MISCOMPARE_DURING_VERIFY_OP, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	/* copy the write data since we will go to next step */
	memcpy(pu8TmpBuf, pu8WriteBuf, u32tmp);

	/* write finally */
	w_task.sg_list = r_task.sg_list;
	w_task.sg_nents = r_task.sg_nents;

	__make_rw_task(&w_task, pSeDev, lba, (u32tmp >> bs_order),
		msecs_to_jiffies(5000), DMA_TO_DEVICE);

	done_blks = __do_b_rw(&w_task);
	if (done_blks <= 0 || w_task.is_timeout || w_task.ret_code != 0){
		pr_err("[ATS BLKIO] fail to write\n");

		if (w_task.ret_code == -ENOSPC)
			__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
				&pSeCmd->scsi_sense_reason);
		else
			__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
				&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	if (done_blks != (u32tmp >> bs_order)){
		pr_err("[ATS BLKIO] byte counts != expected after to write\n");
		__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	ret = ATS_RET_PASS;

_EXIT_:
	if (pu8TmpBuf)
		kunmap(sg_page(sgl));

	if (page)
		__free_page(page);

	if (sgl)
		kfree(sgl);

	if (pu8VerifyBuf)
		transport_kunmap_data_sg(pSeCmd);
	return ret;
}




/*
 * @fn static int vaai_file_do_ats (IN LIO_SE_CMD *pSeCmd)
 * @brief Wrapper function for ATS command of file i/o dvice
 *
 * @param[in] pSeCmd
 * @retval ATS_RET_PASS or ATS_RET_FAIL
 */
static int vaai_file_do_ats(
    IN LIO_SE_CMD *pSeCmd
    )
{
	u8 *read_buf = NULL, *write_buf = NULL, *tmp_buf = NULL;
	u32 total_bytes, real_btyes, bs_order;
	sector_t lba = 0;
	LIO_FD_DEV *fd_dev = NULL;
	struct file *fd_file = NULL;
	LIO_SE_DEVICE *se_dev = NULL;
	struct page *page = NULL;
	int ret;
	struct iovec iov;
	loff_t position;
	mm_segment_t old_fs;
	ssize_t rw_ret;

	/* SCF_SCSI_CONTROL_SG_IO_CDB */
	if(pSeCmd->t_task_cdb[13] > MAX_ATS_LEN){
		__set_err_reason(ERR_INVALID_CDB_FIELD, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	se_dev = pSeCmd->se_dev;
	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
	fd_file = fd_dev->fd_file;

	lba = get_unaligned_be64(&CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[2]);  

	total_bytes = __call_transport_get_size(
			(u32)CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[13],
			CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb, 
			pSeCmd);

	read_buf  = transport_kmap_data_sg(pSeCmd);
	if (!read_buf){
		__set_err_reason(ERR_OUT_OF_RESOURCES, 
		    &pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	write_buf = read_buf + (size_t)total_bytes;

	/* note:
	 * Here only use one page cause of we limit the 
	 * max ats len to 1 sector (i.e. 512b or 4096b)
	 */
	page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!page){
		__set_err_reason(ERR_OUT_OF_RESOURCES, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	 }

	/* read first */
	tmp_buf = (u8 *)page_address(page);
    
	iov.iov_base = (void *)tmp_buf;
	iov.iov_len = (__kernel_size_t)total_bytes;
	position = (loff_t)(lba << bs_order);

	old_fs = get_fs();
	set_fs(get_ds());

	rw_ret = vfs_readv(fd_file, &iov, 1, &position);
	set_fs(old_fs);

	if ((rw_ret < 0) || (rw_ret != total_bytes)){
		pr_err("[ATS FIO] read fail\n");   
		__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	/* compare */
	if (vaai_ats_memcmp(read_buf, tmp_buf, total_bytes, 
		&pSeCmd->byte_err_offset))
	{
		pr_debug("[ATS FIO] compare conflict: read-lba(0x%llx), "
			"err-offset(0x%x) in verify-buffer\n",
			(unsigned long long)lba, 
			pSeCmd->byte_err_offset);
		
		__set_err_reason(ERR_MISCOMPARE_DURING_VERIFY_OP, 
			&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	/* write finally */
	iov.iov_base = (void *)write_buf;
	iov.iov_len = (__kernel_size_t)total_bytes;
	position = (loff_t)(lba << bs_order);

	old_fs = get_fs();
	set_fs(get_ds());

	rw_ret = vfs_writev(fd_file, &iov, 1, &position);
	set_fs(old_fs);

	if ((rw_ret < 0) || (rw_ret != total_bytes)){
		pr_err("[ATS FIO] write fail\n");   

		if (rw_ret == -ENOSPC)
			__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
				&pSeCmd->scsi_sense_reason);
		else
			__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
				&pSeCmd->scsi_sense_reason);
		ret = ATS_RET_FAIL;
		goto _EXIT_;
	}

	ret = ATS_RET_PASS;

_EXIT_:
	if (read_buf)
		transport_kunmap_data_sg(pSeCmd);    

	if (page)
		__free_page(page);

	 return ret;

}

/*
 * @fn int __vaai_do_ats (IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to process ATS command
 *
 * @param[in] pSeCmd
 * @retval 0 - Success / Others - Fail
 */
int __vaai_do_ats(
	IN LIO_SE_CMD *pSeCmd
	)
{
	LIO_SE_SUBSYSTEM_API *api = NULL;
	int ret = 0;

	// If number of logical blocks is zero, should not treat it as error
	if(CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb[13] == 0)
		return 0;

	 api = pSeCmd->se_dev->transport;

	ret = api->do_check_before_ats(pSeCmd);
	if (ret != 0)
		return ret;

	return api->do_ats(pSeCmd);
}


/*
 * @fn int vaai_do_ats_v1 (IN LIO_SE_TASK *pSeTask)
 * @brief Wrapper function to process ATS command passed by target module
 *
 * @param[in] pSeTask
 * @retval 0 - Success
 * @retval Others - Fail
 */
int vaai_do_ats_v1(
	IN LIO_SE_TASK *pSeTask
	)
{
	return __vaai_do_ats(pSeTask->task_se_cmd);
}

int do_check_before_ats(
	IN LIO_SE_CMD *cmd
	)
{
	sector_t Lba = 0;
	u8 *cdb = NULL;
	u32 total_bytes, bs_order;

	/**/
	cdb = CMD_TO_TASK_CDB(cmd)->t_task_cdb;

	Lba = get_unaligned_be64(&cdb[2]);  
	if((Lba > cmd->se_dev->transport->get_blocks(cmd->se_dev))
	|| ((Lba + (sector_t)cdb[13]) > cmd->se_dev->transport->get_blocks(cmd->se_dev) + 1)
	)
	{
		/*
		 * If the specific LBA exceeds the capacity of the medium, 
		 * the device server shall terminate the command with
		 * CHECK CONDITION staus with SK to ILLEGAL REQUEST and ASC is
		 * LOGICAL BLOCK ADDRESS OUT OF RANGE
		 */
		pr_err("[ATS] error !! LBA is out of range !!\n");
		__set_err_reason(ERR_LBA_OUT_OF_RANGE, &cmd->scsi_sense_reason);
		return 1;
	}

	if(cdb[13] > MAX_ATS_LEN){
		/*
		 * If the number of logical blocks exceeds the value in 
		 * MAXIMUM COMPARE AND WRITE LENGTH field in the 
		 * Block Limits VPD page. The device server terminate the 
		 * command with CHECK CONDITION staus with SK to ILLEGAL 
		 * REQUEST and ASC is INVALID FIELD IN CDB
		 */
		pr_err("[ATS] error !! num of blocks > MAX_ATS_LEN !!\n");
		__set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
		return 1;
	}

	if (MAX_ATS_LEN == 1){
		bs_order = ilog2(
			cmd->se_dev->se_sub_dev->se_dev_attrib.block_size);

		if ((MAX_ATS_LEN << bs_order) > PAGE_SIZE){
			pr_err("[ATS] error !! max supported ats len exceeds "
				"the page size\n");
			__set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
			return 1;
		}
	}

	total_bytes = __call_transport_get_size(
			(u32)CMD_TO_TASK_CDB(cmd)->t_task_cdb[13],
			CMD_TO_TASK_CDB(cmd)->t_task_cdb, 
			cmd);

	/* the data length = (2 * sector size) */
	if (cmd->data_length != (total_bytes << 1)){
		pr_err("[ATS] cmd data len != (2 * sector size)\n");
		__set_err_reason(ERR_INVALID_CDB_FIELD, 
			&cmd->scsi_sense_reason);
		return 1;
	}

	return 0;
}
EXPORT_SYMBOL(do_check_before_ats);

int iblock_do_ats(
	IN LIO_SE_CMD *cmd
	)
{
	LIO_IBLOCK_DEV *ib_dev = NULL;
	int ret = 0;

	/**/
	ib_dev = cmd->se_dev->dev_ptr;
	if (queue_max_sectors(bdev_get_queue(ib_dev->ibd_bd)) == 0)
		return 1;

	ret = vaai_block_do_ats(cmd);
	if (ret == 0){
		cmd->cur_se_task->task_scsi_status = GOOD;
		transport_complete_task(cmd->cur_se_task, 1);
	}

	return ret;
}
EXPORT_SYMBOL(iblock_do_ats);


int fd_do_ats(
	IN LIO_SE_CMD *cmd
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct file *fd_file = NULL;
	struct inode *i_node = NULL;
	int ret = 0;

	/**/
	fd_dev  = (LIO_FD_DEV *)cmd->se_dev->dev_ptr;
	fd_file = fd_dev->fd_file;
	i_node  = fd_file->f_mapping->host;

	if (S_ISBLK(i_node->i_mode)){
		if (queue_max_sectors(bdev_get_queue(i_node->i_bdev)) == 0)
			return 1;
	}

	ret = vaai_file_do_ats(cmd);
	if (ret == 0){
		cmd->cur_se_task->task_scsi_status = GOOD;
		transport_complete_task(cmd->cur_se_task, 1);
	}

	return ret;
}
EXPORT_SYMBOL(fd_do_ats);


