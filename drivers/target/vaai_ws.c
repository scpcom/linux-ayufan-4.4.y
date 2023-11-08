/**
 * @file 	vaai_ws.c
 * @brief	This file contains the scsi WRITE SAME command code
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

/**/
static u8* do_get_ws_act_loc(
    IN u8 *cdb
    );

int vaai_block_do_ws(
    IN LIO_SE_CMD *pSeCmd
    );

int vaai_file_do_ws(
    IN LIO_SE_CMD *pSeCmd
    );

int do_check_before_ws(
    IN LIO_SE_CMD *pSeCmd
    );

/**/
static u8* do_get_ws_act_loc(
    IN u8 *cdb
    )
{
    u8 *act_loc = NULL;

    if (!cdb)
        return NULL;

    if ((cdb[0] == VARIABLE_LENGTH_CMD) && (get_unaligned_be16(&cdb[8]) == WRITE_SAME_32))
        act_loc = &cdb[10];
    else if ((cdb[0] == WRITE_SAME) || (cdb[0] == WRITE_SAME_16))
        act_loc = &cdb[1];

    return act_loc;
}

/*
 * @fn int do_check_ws_zero_buffer (IN LIO_SE_CMD *pSeCmd)
 * @brief Function to check the contents of write same buffer is zero or not
 *
 * @param[in] pSeCmd
 * @param[in] u8WSAction
 * @retval 1 - is zero buffer / 0 - is NOT zero buffer
 */
int do_check_ws_zero_buffer(
	IN LIO_SE_CMD *pSeCmd
	)
{
	struct scatterlist *pSg = NULL;
	LIO_SE_DEVICE *pSeDev = NULL;
	u32 u32BlockSize = 0, u32Index = 0;
	u64 *pu64Src = NULL;

	/**/
	pSg           = vaai_get_task_sg_from_se_cmd(pSeCmd);
	pSeDev        = pSeCmd->se_dev;
	u32BlockSize  = pSeDev->se_sub_dev->se_dev_attrib.block_size;
	pu64Src       = (u64*)(page_address(sg_page(pSg)) + pSg->offset);

	for (u32Index= 0; u32Index < (u32BlockSize / sizeof(u64)); u32Index++){
		if (*pu64Src != 0)
			return 0;

		pu64Src = (u64*)((size_t)pu64Src + sizeof(u64));
	}

//	pr_err("[WS] write same buffer is all ZERO!\n");
	return 1;
}
EXPORT_SYMBOL(do_check_ws_zero_buffer);

/*
 * @fn int vaai_block_do_ws(IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to do write-same on block i/o device
 *
 * @param[in] pSeCmd
 * @retval 0 - success / Others - fail
 */
int vaai_block_do_ws(
	IN LIO_SE_CMD *pSeCmd
	)
{
	LIO_SE_DEVICE *pSeDev = NULL;
	u32 u32TotalBCs = 0, u32NumBlocks = 0, bs_order, e_bytes, done_blks;
	u64 alloc_bytes = (1 << 20);
	void *buff = NULL, *tmp_buff = NULL;
	sector_t Lba = 0, Range = 0;
	struct scatterlist *sgl = NULL;
	int ret, idx;
	GEN_RW_TASK w_task;

	/**/
	memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));

	pSeDev = pSeCmd->se_dev;
	bs_order = ilog2(pSeDev->se_sub_dev->se_dev_attrib.block_size);

	 __get_lba_and_nr_blks_ws_cdb(pSeCmd, &Lba, &u32NumBlocks);

	/*
	 * If u8WriteSameNoZero = 0 and u32NumBlocks = 0 means the
	 * device server shall write all of the logical blocks starting with
	 * the one specified in the LOGICAL BLOCK ADDRESS field to the last
	 * logical block on the medium
	 */
	Range = (sector_t)u32NumBlocks;
	if ((u8WriteSameNoZero == 0) &&  (u32NumBlocks == 0))
		Range = pSeDev->transport->get_blocks(pSeDev) - Lba + 1;

	buff = transport_kmap_data_sg(pSeCmd);
	if (!buff){
		__set_err_reason(ERR_OUT_OF_RESOURCES, 
			&pSeCmd->scsi_sense_reason);
		ret = -EIO;
		goto _EXIT_;
	}

	/* To create the sg list to carry the write-same buffer data */
	ret = __generic_alloc_sg_list(&alloc_bytes, &w_task.sg_list, 
			&w_task.sg_nents);
	if (ret != 0){
		if (ret == -ENOMEM)
			pr_err("[WS BLKIO] fail to alloc sg list\n");
		if (ret == -EINVAL)
			pr_err("[WS BLKIO] invalid arg during "
				"to alloc sg list\n");

		__set_err_reason(ERR_OUT_OF_RESOURCES, 
			&pSeCmd->scsi_sense_reason);
		goto _EXIT_;
	}

	sgl = w_task.sg_list;
	for (idx = 0; idx < w_task.sg_nents; idx++){
		/* do map for each page */
		tmp_buff = kmap(sg_page(&sgl[idx])) + sgl[idx].offset;
		pSeDev->transport->do_prepare_ws_buffer(pSeCmd, (1 << bs_order), 
			sgl[idx].length, buff, tmp_buff);
	}

	u32TotalBCs = (Range << bs_order);

	/* start to do write same */
	while (u32TotalBCs){

		e_bytes = min_t(u32, alloc_bytes, u32TotalBCs);
		__make_rw_task(&w_task, pSeDev, Lba, (e_bytes >> bs_order),
			msecs_to_jiffies(5000), DMA_TO_DEVICE);

		done_blks = __do_b_rw(&w_task);
		if (done_blks <= 0 || w_task.is_timeout || w_task.ret_code != 0){
			pr_err("[WS BLKIO] fail to write\n");
		
			if (w_task.ret_code == -ENOSPC)
				__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
					&pSeCmd->scsi_sense_reason);
			else
				__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
					&pSeCmd->scsi_sense_reason);
			ret = -EIO;
			goto _EXIT_;
		}

		if (done_blks != (e_bytes >> bs_order)){
			pr_err("[WS BLKIO] byte counts != expected after "
				"to write\n");
			__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
				&pSeCmd->scsi_sense_reason);
			ret = -EIO;
			goto _EXIT_;
		}

		Lba += (sector_t)done_blks;
		u32TotalBCs -= e_bytes;
	}

	ret = 0;

_EXIT_:
	/* do unmap for page */
	if (w_task.sg_nents){
		sgl = w_task.sg_list;
		for (idx = 0; idx < w_task.sg_nents; idx++)
			kunmap(sg_page(&sgl[idx]));
	}

	__generic_free_sg_list(w_task.sg_list, w_task.sg_nents);

	if (buff)
		transport_kunmap_data_sg(pSeCmd);

	return ret;
}

/*
 * @fn int vaai_file_do_ws(IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to do write-same on file i/o device
 *
 * @param[in] pSeCmd
 * @retval 0 - success / Others - fail
 */
int vaai_file_do_ws(
	IN LIO_SE_CMD *pSeCmd
	)
{
	LIO_SE_DEVICE *se_dev = NULL;
	u32 u32Tmp = 0, u32TotalBCs = 0, u32NumBlocks = 0, bs_order;
	sector_t Lba = 0, Range = 0;
	LIO_FD_DEV *fd_dev = NULL;
	struct file *fd_file = NULL;
	struct page *page = NULL;
	void *src_buffer = NULL, *tmp_page_buffer = NULL;
	int Ret = 0;
	
	/**/
	__get_lba_and_nr_blks_ws_cdb(pSeCmd, &Lba, &u32NumBlocks);
	se_dev = pSeCmd->se_dev;
	fd_dev = (LIO_FD_DEV *)se_dev->dev_ptr;
	fd_file = fd_dev->fd_file;
	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);

	/*
	 * If u8WriteSameNoZero = 0 and u32NumBlocks = 0 means the device server
	 * shall write all of the logical blocks starting with the one specified in
	 * the LOGICAL BLOCK ADDRESS field to the last logical block on the medium
	 */
	Range = (sector_t)u32NumBlocks;
	if ((u8WriteSameNoZero == 0) &&  (u32NumBlocks == 0))
		Range = se_dev->transport->get_blocks(pSeCmd->se_dev) - Lba + 1;

	u32Tmp = u32TotalBCs = __call_transport_get_size((u32)Range, 
			CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb, pSeCmd);

	src_buffer = transport_kmap_data_sg(pSeCmd);
	if (!src_buffer)
		goto _EXIT_;
	
	/* To create the temp write same buffer first */
	page = alloc_pages(GFP_KERNEL, 0);
	if (!page)
		goto _EXIT_;
	
	/* to prepare the write buffer contents */
	tmp_page_buffer = kmap(page);
	se_dev->transport->do_prepare_ws_buffer(pSeCmd, (1 << bs_order), 
			PAGE_SIZE, src_buffer, tmp_page_buffer);

	/* TBD */
	while (u32TotalBCs){
		u32Tmp = min_t(u32, PAGE_SIZE, u32TotalBCs);

		Ret = vaai_do_file_rw(se_dev, (void*)fd_file, DO_WRITE, 
				tmp_page_buffer, Lba, u32Tmp);

#if defined(SUPPORT_TP)
		if ((Ret == 0) && is_thin_lun(se_dev)){
			int err_1, err_2; 
			struct inode *inode = fd_file->f_mapping->host;

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
				err_1 = __do_sync_cache_range(fd_file, 
					(Lba << bs_order), 
					((Lba << bs_order) + u32Tmp));

				if (err_1 != 0){
					/* TODO:
					 * thin i/o may go here (lba wasn't
					 * mapped to any block) or something 
					 * wrong during normal sync-cache
					 */
					if (err_1 != -ENOSPC){					
						/* call again to make sure it is no space
						 * really or not
						 */
						err_2 = check_dm_thin_cond(inode->i_bdev);
						if (err_2 == -ENOSPC){
							err_1 = err_2;
						}
						/* it may something wrong duing sync-cache */
					}
					Ret = err_1;
					goto _EXIT_1_;
				}
			}

			/* fall-through */
		}
_EXIT_1_:
#endif
		if (Ret != 0){

#if defined(SUPPORT_TP)
			if (Ret == -ENOSPC){
				pr_warn("[WS FIO] space was full\n");
				__set_err_reason(ERR_NO_SPACE_WRITE_PROTECT, 
					&pSeCmd->scsi_sense_reason);
				goto _EXIT_;
			}
#endif
			pr_err("[WS FIO] fail to write data, "
				"Ret:0x%x\n", Ret);
			__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
				&pSeCmd->scsi_sense_reason);

			Ret = -EIO;
			goto _EXIT_;
		}

		Lba += (sector_t)(u32Tmp >> bs_order);
		u32TotalBCs -= u32Tmp;
	}

	Ret = 0;
_EXIT_:
	
	if (tmp_page_buffer)
		kunmap(page);
	
	if (page)
		__free_page(page);

	if (src_buffer)
		transport_kunmap_data_sg(pSeCmd);
	
	return Ret;
}


/*
 * @fn int do_check_before_ws (IN LIO_SE_CMD *pSeCmd)
 * @brief Function to do some condition-checking before to handle write-same command
 *
 * @param[in] pSeCmd
 * @retval 0 - Success / Others - Fail
 */
int do_check_before_ws(
    IN LIO_SE_CMD *pSeCmd
    )
{
    u32 u32NumBlocks = 0;
    sector_t Lba = 0;
    u8 *ws_act_loc = NULL;

    /* 
     * FIXED ME !!
     *
     * SBC3R33 obsolete PBDATA and LBDATA two bits. This code will be removed in the future 
     */
    ws_act_loc = do_get_ws_act_loc(CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb);
    if (!ws_act_loc){
        __set_err_reason(ERR_INVALID_CDB_FIELD, &pSeCmd->scsi_sense_reason);
        return -EOPNOTSUPP;
    }

    /* To check some necessary conditions before to do specific write-same */
    __get_lba_and_nr_blks_ws_cdb(pSeCmd, &Lba, &u32NumBlocks);

    DBG_PRINT("WS: u8WriteSameNoZero:0x%x, Lba:0x%llu, u32NumBlocks:0x%x\n", 
                u8WriteSameNoZero, (unsigned long long)Lba, u32NumBlocks
                );

    if ((u8WriteSameNoZero == 1) && (u32NumBlocks == 0)){
        pr_err("WS: error!! u8WriteSameNoZero is 1 and u32NumBlocks is 0 !\n");
        __set_err_reason(ERR_INVALID_CDB_FIELD, &pSeCmd->scsi_sense_reason);
        return -EOPNOTSUPP;
    }

    /*
     * u64MaxWSLen = 0 means there is no reported limit on the number of logical
     * blocks that may be requested for a single WRITE SAME commnad from device server.
     */
    if (u8WriteSameNoZero == 1){
        if ((u64MaxWSLen != 0) && (u64MaxWSLen < (sector_t)u32NumBlocks )){
            pr_err("WS: error!! u64MaxWSLen < (sector_t)u32NumBlocks !!\n");
            __set_err_reason(ERR_INVALID_CDB_FIELD, &pSeCmd->scsi_sense_reason);
            return -EOPNOTSUPP;
        }
    }

    if ((u8WriteSameNoZero == 1) && (u32NumBlocks != 0)){
        if ((Lba + (sector_t)u32NumBlocks) > pSeCmd->se_dev->transport->get_blocks(pSeCmd->se_dev) + 1){
            pr_err("WS: error!! range of lba be written > max device capacity lba !\n");
            __set_err_reason(ERR_LBA_OUT_OF_RANGE, &pSeCmd->scsi_sense_reason);
            return -EOPNOTSUPP;
        }
    }

    return 0;
}
EXPORT_SYMBOL(do_check_before_ws);

/*
 * @fn int vaai_check_do_ws (IN u8 *pu8Flags, IN void *pDev)
 * @brief Patch function to replace original target_check_write_same_discard()
 *
 * @param[in] pu8Flags - byte 1 or byte 10 of CDB field for write-same(10/16/32)
 * @param[in] pDev
 * @retval 0 - Success / Others - Fail
 */
int vaai_check_do_ws(
	IN u8 *pu8Flags,
	IN void *pDev
	)
{
	/* Pleaes refer the p205 for sbc3r35j version */
	if (!(pu8Flags[0] & 0x08)) {
		if (pu8Flags[0] & 0x10) {
			pr_err("WS] error!! UNMAP bit is 0 but "
				"ANCHOR bit is 1\n");
			return -EOPNOTSUPP;
		}
	}else{
		/* continue to check write emulation with UNMAP bit = 1 */
		if ((pu8Flags[0] & 0x10) && (bSupportArchorLba == 0)) {
			pr_err("WS: error!! UNMAP bit is 1"
				"and ANCHOR bit is 1 but ANC_SUP bit is 0\n");
			return -EOPNOTSUPP;
		}
	}
	return 0;
}


/*
 * @fn int __vaai_do_ws (IN LIO_SE_CMD *pSeCmd)
 * @brief Main function to process write same command
 *
 * @param[in] pSeCmd
 * @retval 0 - Success / Others - Fail
 */
int __vaai_do_ws(
	IN LIO_SE_CMD *pSeCmd
	)
{
	LIO_SE_SUBSYSTEM_API *api = NULL;
	u8 *ws_act_loc = NULL;
	int ret = 0;

	api = pSeCmd->se_dev->transport;
	ws_act_loc = do_get_ws_act_loc(CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb);
	if (!ws_act_loc){
		__set_err_reason(ERR_INVALID_CDB_FIELD, 
			&pSeCmd->scsi_sense_reason);
		return -EOPNOTSUPP;
	}

	ret = api->do_check_before_ws(pSeCmd);
	if (ret !=0)
		return ret;

	if (is_thin_lun(pSeCmd->se_dev) == 0){
		/*
		 * SBC3R31, p202, 
		 *
		 * The device server shall ignore the UNMAP bit and ANCHOR bit, 
		 * or the device server shall terminate the command with
		 * CHECK CONDITION status with the sense key set to 
		 * ILLEGAL REQUEST and the additional sense code set to the
		 * INVALID FIELD IN CDB if
		 *
		 * (1) the LU is fully provisioned (i.e., the LBPME bit set to
		 * the zero in the READ CAPACITY 16 parameter data; and
		 * (2) the UNMAP bit is 1 or the ANCHOR bit is 1
		 */ 
		if ((ws_act_loc[0] & 0x08) || (ws_act_loc[0] & 0x10)) {
			pr_err("[WS] error!! either UNMAP bit is 1 or "
				"ANCHOR bit is 1 for fully provisioning LUN\n");
			__set_err_reason(ERR_INVALID_CDB_FIELD, 
				&pSeCmd->scsi_sense_reason);
			return -EOPNOTSUPP;
		}

		/* WRITE_SAME w/o UNMAP bit and w/o ANCHOR bit, to do specific
		 * write operation for thick lun 
		 */
		return api->do_ws_wo_unmap(pSeCmd);
	}

	/* case for block provisioning lun ... */

	/* 
	 * SBC3R31, p202, 
	 *
	 * If the LU supports the logical block provisioning management, then
	 * the ANCHOR bit, UNMAP bit and ANC_SUP bit in the Logical Block
	 * Provisioning VPD page determine how the device server processes the
	 * command described in
	 * table 122
	 */
	if (!(ws_act_loc[0] & 0x08)) {
		if (ws_act_loc[0] & 0x10) {
			pr_err("[WS] error!! WRITE_SAME w/o UNMAP bit and "
				"ANCHOR bit is 1 for block provisioning LUN\n");
			__set_err_reason(ERR_INVALID_CDB_FIELD, 
				&pSeCmd->scsi_sense_reason);
			return -EOPNOTSUPP;
		}

		/* WRITE_SAME w/o UNMAP bit and w/o ANCHOR bit, to do specific
		 * write operation for thin lun */
		return api->do_ws_wo_unmap(pSeCmd);        
	}

	/* continue to check write emulation with UNMAP bit = 1 */

	if (ws_act_loc[0] & 0x10) {
		if (bSupportArchorLba == 0) {
			pr_err("[WS] error!! WRITE_SAME with UNMAP bit and "
				"ANCHOR bit is 1 and ANC_SUP bit is 0 for "
				"block provisioning LUN\n");
			__set_err_reason(ERR_INVALID_CDB_FIELD, 
				&pSeCmd->scsi_sense_reason);
			return -EOPNOTSUPP;
		}

		/*
		 * WRITE_SAME with UNMAP bit and ANCHOR bit is 1 and ANC_SUP
		 * bit is 1,  to do Anchor operation
		 *
		 * SBC3R31, p202, table 122, (f)
		 *
		 * The device server should anchor each LBA specified by the
		 * command. If the device server doesn't anchor the LBA, then
		 * the device server shall perform the specified write
		 * operation.
		 */
		return api->do_ws_w_anchor(pSeCmd);        
	}

	/*
	 * WRITE_SAME with UNMAP bit and ANCHOR bit is 0 and ANC_SUP bit
	 * is N/A, to do unmap operation (here, I call native function provied
	 * by LIO module)
	 *
	 * SBC3R31, p202, table 122, (e)
	 *
	 * The device server in the thin-provisioned LU should deallocate
	 * each LBA specified by the command but may anchor each LBA specified
	 * by the command. The device server in a resource provisioned LU 
	 * should anchor each LBA specified the command. If the device server
	 * doesn't deallocate or anchor the LBA, then the device server shall
	 * perform the specified write operation.
	 */
	return api->do_ws_w_unmap(pSeCmd);        
}

/*
 * @fn int vaai_do_ws_v1 (IN LIO_SE_TASK *pSeTask)
 * @brief Wrapper function to process write same command passed by target module
 *
 * @param[in] pSeTask
 * @retval 0 - Success / Others - Fail
 */
int vaai_do_ws_v1(
    IN LIO_SE_TASK *pSeTask
    )
{
	return __vaai_do_ws(pSeTask->task_se_cmd);
}

void do_prepare_ws_buffer(
	IN struct se_cmd *cmd,
	IN u32 blk_size,
	IN u32 total_bytes,
	IN void *src,
	IN void *dest
	)
{
	u32 copy_size;

	while (total_bytes){
		copy_size = min_t(u32, blk_size, total_bytes);
		memcpy(dest, src, copy_size);
		dest += (size_t)copy_size;
		total_bytes -= copy_size;
	}
	return;
}
EXPORT_SYMBOL(do_prepare_ws_buffer);

int iblock_do_ws_wo_unmap(
    IN LIO_SE_CMD *cmd
    )
{
	LIO_IBLOCK_DEV *ib_dev = NULL;
	LIO_SE_SUBSYSTEM_API *api = NULL;
	int ret = 0;

	/**/
	api    = cmd->se_dev->transport;
	ib_dev = cmd->se_dev->dev_ptr;

	if (queue_max_sectors(bdev_get_queue(ib_dev->ibd_bd)) == 0)
		return 1;

	/* 
	 * Here will depend on the full provisioning lun or block provisioning
	 * lun to do the specifc write-same action
	 */
	if (is_thin_lun(cmd->se_dev) == 1){
		/* special: 0 - NOT do special discard (WRITE SAME w/ UNMAP)
		 * special: 1 - do special discard (WRITE SAME w/o UNMAP)
		 *
		 * NOTE:
		 * Here is for block i/o + fbdisk configuration of the
		 * 32bit kernel product
		 */
		if (api->do_check_ws_zero_buffer(cmd) == 1)
			ret = vaai_block_do_specific_fast_ws(cmd, 1);
		else
			ret = vaai_block_do_ws(cmd);
	}
	else{
		/* this is thick ... */
		ret = vaai_block_do_ws(cmd);
	}

	if (ret == 0){
		cmd->cur_se_task->task_scsi_status = GOOD;
		transport_complete_task(cmd->cur_se_task, 1);
	}

	return ret;

}
EXPORT_SYMBOL(iblock_do_ws_wo_unmap);


int iblock_do_ws_w_unmap(
	IN LIO_SE_CMD *cmd
	)
{
	LIO_IBLOCK_DEV *ib_dev = NULL;
	LIO_SE_SUBSYSTEM_API *api = NULL;
	int ret = 0;

	/**/
	api    = cmd->se_dev->transport;
	ib_dev = cmd->se_dev->dev_ptr;

	if (queue_max_sectors(bdev_get_queue(ib_dev->ibd_bd)) == 0)
		return 1;

	if (is_thin_lun(cmd->se_dev) == 0){
		pr_err("[VAAI] WS_DBG: error !! write same w/ unmap can NOT "
				"work on thick device\n");
		__set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
		return 1;
	}

	if (api->do_check_ws_zero_buffer(cmd) == 1){
		/* This is native discard provided by linux */
		ret = vaai_block_do_specific_fast_ws(cmd, 0);
	}else
		ret = vaai_block_do_ws(cmd);

	if (ret == 0){
		cmd->cur_se_task->task_scsi_status = GOOD;
		transport_complete_task(cmd->cur_se_task, 1);
	}
	return ret;
}
EXPORT_SYMBOL(iblock_do_ws_w_unmap);

int iblock_do_ws_w_anchor(
    IN LIO_SE_CMD *cmd
    )
{
	int Ret = -EOPNOTSUPP;

	/* FIXED ME */
	pr_debug("[VAAI] WS_DBG: %s, Ret:0x%x\n",__FUNCTION__, Ret);
	__set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
	return Ret;
}
EXPORT_SYMBOL(iblock_do_ws_w_anchor);


int fd_do_ws_wo_unmap(
    IN LIO_SE_CMD *cmd
    )
{
	LIO_FD_DEV *fd_dev = NULL;
	struct file *fd_file = NULL;
	struct inode *i_node = NULL;
	LIO_SE_SUBSYSTEM_API *api = NULL;
	int ret = 0;

	/**/
	api = cmd->se_dev->transport;
	fd_dev  = (LIO_FD_DEV *)cmd->se_dev->dev_ptr;
	fd_file = fd_dev->fd_file;
	i_node  = fd_file->f_mapping->host;

	if (S_ISBLK(i_node->i_mode)){
		if (queue_max_sectors(bdev_get_queue(i_node->i_bdev)) == 0)
			return 1;
	}

	/* 
	 * Here will depend on the full provisioning lun or block provisioning
	 * lun to do the specifc write-same action
	 */
	if (is_thin_lun(cmd->se_dev) == 1){
		if (api->do_check_ws_zero_buffer(cmd) == 1){
			/* special: 0 - NOT do special discard (WRITE SAME w/ UNMAP)
			 * special: 1 - do special discard (WRITE SAME w/o UNMAP)
			 *
			 * NOTE:
			 * Here is for file i/o + block-backend configuration
			 * of 32bit kernel product
			 */
			/* 2014/06/26, adamhsu, redmine 8794 */
			if (__IS_32BIT_ARCH(ARCH_P_LEN))
				ret = vaai_file_do_ws(cmd);
			else
				ret = vaai_file_do_specific_fast_ws(cmd, 1);
		}else
			ret = vaai_file_do_ws(cmd);
	}else
		ret = vaai_file_do_ws(cmd);

	if (ret == 0){
		cmd->cur_se_task->task_scsi_status = GOOD;
		transport_complete_task(cmd->cur_se_task, 1);
	}
	return ret;
}
EXPORT_SYMBOL(fd_do_ws_wo_unmap);

int fd_do_ws_w_anchor(
    IN LIO_SE_CMD *cmd
    )
{
	int Ret = -EOPNOTSUPP;

	/* FIXED ME */
	pr_debug("[VAAI] WS_DBG: %s, Ret:0x%x\n",__FUNCTION__, Ret);
	__set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
	return Ret;
}
EXPORT_SYMBOL(fd_do_ws_w_anchor);

int fd_do_ws_w_unmap(
	IN LIO_SE_CMD *cmd
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct file *fd_file = NULL;
	struct inode *i_node = NULL;
	LIO_SE_SUBSYSTEM_API *api = NULL;
	int ret = 0;

	api = cmd->se_dev->transport;
	fd_dev  = (LIO_FD_DEV *)cmd->se_dev->dev_ptr;
	fd_file = fd_dev->fd_file;
	i_node  = fd_file->f_mapping->host;

	if (S_ISBLK(i_node->i_mode)){
		if (queue_max_sectors(bdev_get_queue(i_node->i_bdev)) == 0)
			return 1;
	}

	if (is_thin_lun(cmd->se_dev) == 0){
		pr_err("[VAAI] WS_DBG: error !! write same w/ unmap can NOT "
				"work on thick device\n");
		__set_err_reason(ERR_INVALID_CDB_FIELD, &cmd->scsi_sense_reason);
		return 1;
	}

	if (api->do_check_ws_zero_buffer(cmd) == 1)
		ret = vaai_file_do_specific_fast_ws(cmd, 0);
	else
		ret = vaai_file_do_ws(cmd);

	if (ret == 0){
		cmd->cur_se_task->task_scsi_status = GOOD;
		transport_complete_task(cmd->cur_se_task, 1);
	}
	return ret;
}
EXPORT_SYMBOL(fd_do_ws_w_unmap);


