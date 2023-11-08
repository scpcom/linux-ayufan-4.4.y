/**
 * @file 	tpc_rod.c
 * @brief	This file contains the 3rd-party copy (ODX function) command code
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
#include "tpc_helper.h"

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_FAST_BLOCK_CLONE)
#include "target_fast_clone.h"
#endif

/* Jonathan Ho, 20131212, monitor ODX */
#ifdef SHOW_OFFLOAD_STATS
extern u64 Xpcmd;
extern unsigned int Tpcmd;
extern unsigned long cmd_done;
extern u64 Xtotal;
#endif /* SHOW_OFFLOAD_STATS */
#endif /* defined(CONFIG_MACH_QNAPTS) */

/**/
static int __core_do_wt(WBT_OBJ *wbt_obj);

static int __do_chk_before_write_by_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err
	);

static int __do_chk_before_populate_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err
	);

static int __do_chk_max_lba_in_desc_list(
    IN LIO_SE_CMD *se_cmd,
    IN void *start,
    IN u16 counts
    );

static int __do_chk_same_lba_in_desc_list(
    IN LIO_SE_CMD *se_cmd,
    IN void *start,
    IN u16 counts
    );

static int __do_chk_overlap_lba_in_desc_list(
    IN LIO_SE_CMD *se_cmd,
    IN void *start,
    IN u16 counts
    );

static int __do_b_f_wzrt(
    IN LIO_SE_CMD *se_cmd, 
    IN bool is_thin_dev,
    IN void *new_obj
    );

static int __tpc_put_rrti_by_tdata(
    IN LIO_SE_CMD *t_cmd,
    IN ROD_TOKEN_INFO_PARAM *param,
    IN T_CMD_STATUS status
    );

static int __tpc_put_rrti_by_obj(
    IN TPC_OBJ *obj,
    IN ROD_TOKEN_INFO_PARAM *param,
    IN bool attached_rod_token,
    IN u32 alloc_len
    );

static void __tpc_free_token_timer(
    IN unsigned long arg
    );

static void __tpc_setup_token_timer(
    IN TPC_OBJ *obj,
    IN u32 timeout
    );

static int __do_wt_by_normal_rw(
	WBT_OBJ *wbt_obj,
	sector_t s_lba,
	sector_t d_lba,
	u64 *data_bytes
	);

static int __do_wt_by_normal_rw(
	WBT_OBJ *wbt_obj,
	sector_t s_lba,
	sector_t d_lba,
	u64 *data_bytes
	)
{
	int r_done_blks = 0, w_done_blks = 0, exit_loop = 0, ret = 1;
	u64 e_bytes = *data_bytes;
	GEN_RW_TASK r_task, w_task;

	/* Prepare the read task */
	e_bytes = min_t(u64, e_bytes, wbt_obj->sg_total_bytes);

	memset((void *)&r_task, 0, sizeof(GEN_RW_TASK));
	r_task.sg_list = wbt_obj->sg_list;
	r_task.sg_nents = wbt_obj->sg_nents;
	__make_rw_task(&r_task, wbt_obj->s_obj->se_lun->lun_se_dev, s_lba,
		(e_bytes >> wbt_obj->s_obj->dev_bs_order),
		wbt_obj->timeout, DMA_FROM_DEVICE
		);

	pr_debug("[wrt] r-task: r-lba:0x%llx, r-blks:0x%x, "
		"e_bytes:0x%llx, r-dev bs_order:0x%x \n", 
		(unsigned long long)r_task.lba, r_task.nr_blks, 
		e_bytes, wbt_obj->s_obj->dev_bs_order);

	/* To submit read */
	r_done_blks = __tpc_do_rw(&r_task);
	if (r_done_blks <= 0 || r_task.is_timeout){
		wbt_obj->err = ERR_3RD_PARTY_DEVICE_FAILURE;
		exit_loop = 1;
		goto _RW_ERR_;
	}

	if (r_done_blks != (e_bytes >> wbt_obj->s_obj->dev_bs_order)){
		/* if r_done_blks != excepted read blks, then to computed 
		 * the excepted byte counts again. This value will used
		 * in w task. 
		 */
		pr_err("r_done_blks != expected blks\n");
		e_bytes = ((u64)r_done_blks << wbt_obj->s_obj->dev_bs_order);
	}

	/* Prepare the write task */
	memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));
	w_task.sg_list	 = r_task.sg_list;
	w_task.sg_nents  = r_task.sg_nents;
	__make_rw_task(&w_task, wbt_obj->d_obj->se_lun->lun_se_dev, d_lba, 
		(e_bytes >> wbt_obj->d_obj->dev_bs_order),
		wbt_obj->timeout, DMA_TO_DEVICE
		);
	  
	pr_debug("[wrt] w-task: w-lba:0x%llx, w-blks:0x%x\n",
		(unsigned long long)w_task.lba, w_task.nr_blks);
	
	/* To submit write */
	w_done_blks = __tpc_do_rw(&w_task);
	if((w_done_blks <= 0) || w_task.is_timeout || w_task.ret_code != 0){
		if (w_task.ret_code == -ENOSPC)
			wbt_obj->err = ERR_NO_SPACE_WRITE_PROTECT;
		else
			wbt_obj->err = ERR_3RD_PARTY_DEVICE_FAILURE;
		exit_loop = 1;
	} else if ((e_bytes >> wbt_obj->d_obj->dev_bs_order) != (u64)w_done_blks)
		pr_err("w_done_blks != expected write blks\n");

_RW_ERR_:

	if (wbt_obj->err != ERR_NO_SPACE_WRITE_PROTECT)
		__tpc_update_t_cmd_transfer_count(wbt_obj->t_data, (sector_t)w_done_blks);

	/* To exit this loop if hit any error */
	if (r_task.is_timeout || w_task.is_timeout || exit_loop)
		goto _EXIT_;
	
	/* If this real write byet counts doesn's equal to expectation 
	 * of write byte counts at this round then to break the loop */
	if ((e_bytes >> wbt_obj->d_obj->dev_bs_order) != (u64)w_done_blks)
		goto _EXIT_;

	ret = 0;
_EXIT_:
	*data_bytes = ((u64)w_done_blks << wbt_obj->d_obj->dev_bs_order);
	return ret;

}

/*
 * @fn static int __core_do_wt (WBT_OBJ *wbt_obj, ERR_REASON_INDEX *err)
 * @brief main function to execute WRITE BY TOKEN action
 *
 * @sa 
 * @param[in] wbt_obj
 * @retval  0 - Success, 1 - Fail to execute function
 */
static int __core_do_wt(
	WBT_OBJ *wbt_obj
	)
{
	BLK_RANGE_DATA *br_d = NULL;
	u32 bdr_off;
	u64 tmp_w_bytes = wbt_obj->data_bytes, tmp, e_bytes;
	int exit_loop = 0, ret = 1;
	sector_t src_lba, dest_lba = wbt_obj->d_lba;
	u64 t_off_to_rod = wbt_obj->s_off_rod;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_FAST_BLOCK_CLONE)
	LIO_SE_DEVICE *s_se_dev = wbt_obj->s_obj->se_lun->lun_se_dev;
	LIO_SE_DEVICE *d_se_dev = wbt_obj->d_obj->se_lun->lun_se_dev;
	int is_create = 0, do_fbc = 0;
	FC_OBJ fc_obj;
	TBC_DESC_DATA tbc_desc_data;
#endif
#endif

	/* main while loop() to read(or write) data */
	while (tmp_w_bytes){
	
		/* To get the br data location by rod offset */
		spin_lock_bh(&wbt_obj->s_obj->se_tpg->tpc_obj_list_lock);
		br_d = __tpc_get_rod_loc_by_rod_off(
			wbt_obj->s_obj, &bdr_off, t_off_to_rod);
		spin_unlock_bh(&wbt_obj->s_obj->se_tpg->tpc_obj_list_lock);


		if (!br_d){
			/* FIXED ME */
			u64 tmp_nr_bytes;

			spin_lock_bh(&wbt_obj->s_obj->se_tpg->tpc_obj_list_lock);
			tmp_nr_bytes = __tpc_get_nr_bytes_by_s_obj(wbt_obj->s_obj, 1);
			spin_unlock_bh(&wbt_obj->s_obj->se_tpg->tpc_obj_list_lock);

			if (t_off_to_rod >= tmp_nr_bytes)
				goto _EXIT_; 
		}
		src_lba = (br_d->lba + bdr_off);

		/* To check source token is invalid or not before to read. If
		 * the source token is invalid, here won't update any data for 
		 * s_obj, what we will do is to udpate n_obj only */

		spin_lock_bh(&wbt_obj->s_obj->se_tpg->tpc_obj_list_lock);
		exit_loop = __tpc_is_token_invalid(wbt_obj->s_obj, 
				&wbt_obj->err);
		spin_unlock_bh(&wbt_obj->s_obj->se_tpg->tpc_obj_list_lock);

		if (exit_loop != 0)
			goto _EXIT_;

		/* To check whether the w range execeeds the  (br_d->nr_blks - bdr_off) */
		tmp = ((u64)(br_d->nr_blks - bdr_off) << wbt_obj->s_obj->dev_bs_order);

		if (tmp_w_bytes > tmp)
			e_bytes = tmp;
		else
			e_bytes = tmp_w_bytes;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_FAST_BLOCK_CLONE)

		if (!s_se_dev->fast_blk_clone)
			goto _NORMAL_RW_;
	
		if (!d_se_dev->fast_blk_clone)
			goto _NORMAL_RW_;




		if (!is_create){
			__create_fbc_obj(&fc_obj, s_se_dev, d_se_dev,
				src_lba, dest_lba, e_bytes);

			if (__check_s_d_lba_before_fbc(&fc_obj) == 0){
				if (__do_check_support_fbc(&fc_obj, 
					&tbc_desc_data) == 0)
					do_fbc = __do_update_fbc_data_bytes(
							&fc_obj, &tbc_desc_data,	
							dest_lba, &e_bytes);
			}
			is_create = 1;
		}else
			/* go further step since we create already */
			do_fbc = __do_update_fbc_data_bytes(
				&fc_obj, &tbc_desc_data, 
				dest_lba, &e_bytes);

		if (do_fbc){
			__create_fbc_obj(&fc_obj, s_se_dev, d_se_dev,
				src_lba, dest_lba, e_bytes);

			ret = __do_wt_by_fbc(wbt_obj, &fc_obj, &e_bytes);

			if (ret == 0){
				/* udpate how many blks we did successfully */
				__tpc_update_t_cmd_transfer_count(
					wbt_obj->t_data, 
					(sector_t)(e_bytes >> wbt_obj->d_obj->dev_bs_order));
				goto _GO_NEXT_;
			}

			/* if found error try to use general read / write
			 * to do copy operation except the no-space event */

			if (ERR_NO_SPACE_WRITE_PROTECT == wbt_obj->err)
				goto _EXIT_;

			/* not update how many blks we did cause of we will
			 * force rollback to nomal read / write
			 */
			pr_debug("[wrt] fail to execute "
				"fast-block-clone, try rollback to "
				"general read/write, src lba:0x%llx, "
				"dest lba:0x%llx, bytes:0x%llx\n",
				(unsigned long long)src_lba, 
				(unsigned long long)dest_lba, 
				(unsigned long long)e_bytes);

			goto _NORMAL_RW_;
		}

_NORMAL_RW_:
#endif
		ret = __do_wt_by_normal_rw(wbt_obj, src_lba, dest_lba, 
			&e_bytes);
		if (ret)
			goto _EXIT_;

_GO_NEXT_:	
/* Jonathan Ho, 20131231, change place to get data */
#ifdef SHOW_OFFLOAD_STATS
		Xtotal += e_bytes >> 10; /* bytes to KB */
#endif /* SHOW_OFFLOAD_STATS */
		tmp_w_bytes -= e_bytes;
		t_off_to_rod += e_bytes;
		dest_lba += (sector_t)(e_bytes >> wbt_obj->d_obj->dev_bs_order);
	
		pr_debug("[wrt] w_done_blks:0x%llx, tmp_w_bytes:0x%llx, "
			"t_off_to_rod (byte-based):0x%llx, dest_lba:0x%llx\n", 
			(unsigned long long)(e_bytes >> wbt_obj->d_obj->dev_bs_order), 
			(unsigned long long)tmp_w_bytes, 
			(unsigned long long)t_off_to_rod,
			(unsigned long long)dest_lba);
	}

	ret = 0;

_EXIT_:
	return ret;

}


/*
 * @fn static void __tpc_free_token_timer(IN unsigned long arg)
 * @brief toker token timer
 *
 * @sa 
 * @param[in] arg
 * @retval N/A
 */
static void __tpc_free_token_timer(
	IN unsigned long arg
	)
{
#define TOKEN_NEXT_TIME_OUT    2

	TPC_OBJ *obj = (TPC_OBJ *)arg;
	void *token = NULL;
	LIST_HEAD(tmp_data_list);

	spin_lock(&obj->se_tpg->tpc_obj_list_lock);

	/* check it was taken by someone already? */
	spin_lock(&obj->o_token_status_lock);
	if (TOKEN_STS_FREE_BY_PROC(obj->o_token_status)){
		pr_debug("%s: obj token(id:0x%x, op_sac:0x%x) status was set "
				"to O_TOKEN_STS_FREE_BY_PROC already !! "
				"skip it\n", __func__, 
				obj->list_id, obj->op_sac);

		spin_unlock(&obj->o_token_status_lock);
		spin_unlock(&obj->se_tpg->tpc_obj_list_lock);
		return;
	}

	/**/
	if (TOKEN_STS_ALIVE(obj->o_token_status))
		obj->o_token_status = O_TOKEN_STS_FREE_BY_TOKEN_TIMER;

	spin_unlock(&obj->o_token_status_lock);

	/* check whether somebody still use this token or not */
	spin_lock(&obj->o_token_ref_count_lock);
	if (atomic_read(&obj->o_token_ref_count)){
		spin_unlock(&obj->o_token_ref_count_lock);
		spin_unlock(&obj->se_tpg->tpc_obj_list_lock);

		pr_debug("%s: token is using by "
			"someone(id:0x%x, op_sac:0x%x)\n",
			__func__, obj->list_id, obj->op_sac);

	        /* Let the obj timer to be the same as token timer which
	         * will be fired later */
        	mod_timer(&obj->o_timer, 
        		obj->o_timer.expires + \
        		msecs_to_jiffies(TOKEN_NEXT_TIME_OUT * 1000));

		/* To init the token timer */
		init_timer(&obj->o_token_timer);
		obj->o_token_timer.expires = (jiffies + \
				msecs_to_jiffies(TOKEN_NEXT_TIME_OUT * 1000));
		obj->o_token_timer.data      = (unsigned long)obj;
		obj->o_token_timer.function  = __tpc_free_token_timer;
		add_timer(&obj->o_token_timer);
		return;

	}
	spin_unlock(&obj->o_token_ref_count_lock);

	spin_lock(&obj->o_token_status_lock);
	if (TOKEN_STS_FREE_BY_TOKEN_TIMER(obj->o_token_status))
		obj->o_token_status = O_TOKEN_STS_EXPIRED;
	spin_unlock(&obj->o_token_status_lock);	

	spin_lock(&obj->o_data_lock);
	token = obj->o_token_data;
	obj->o_token_data = NULL;
	list_splice_tail_init(&obj->o_data_list, &tmp_data_list);
	spin_unlock(&obj->o_data_lock);

	spin_unlock(&obj->se_tpg->tpc_obj_list_lock);

	/* start to free necessary token resource */
	pr_debug("start free token(id:0x%x, op_sac:0x%x) by token-timer\n",
			obj->list_id, obj->op_sac);

	if (!list_empty(&tmp_data_list))
		__tpc_free_obj_node_data(&tmp_data_list);

	pr_debug("to free token:0x%p\n", token);
	if (token)
    		kfree(token);

	return;
}


/*
 * @fn static void __tpc_setup_token_timer(IN TPC_OBJ *obj, IN u32 timeout)
 * @brief To setup the tpc token timer
 *
 * @sa 
 * @param[in] obj
 * @param[in] timeout
 * @retval N/A
 */
static void __tpc_setup_token_timer(
    IN TPC_OBJ *obj,
    IN u32 timeout
    )
{
    unsigned long token_timeout = 0;

    if (timeout > MAX_INACTIVITY_TIMEOUT)
        timeout = MAX_INACTIVITY_TIMEOUT;

    if (timeout == 0)
        token_timeout = jiffies + msecs_to_jiffies(D4_INACTIVITY_TIMEOUT * 1000);
    else
        token_timeout = jiffies + msecs_to_jiffies(timeout * 1000);

    obj->o_token_timer.expires      = token_timeout;
    obj->o_token_timer.data         = (unsigned long )obj;
    obj->o_token_timer.function     = __tpc_free_token_timer;
    add_timer(&obj->o_token_timer);
    return;
}


/*
 * @fn static void __release_token_from_free_obj(IN TPC_OBJ *obj)
 * @brief To release the token resource from free obj data (deleted from list already)
 *
 * @sa 
 * @param[in] obj
 * @retval N/A
 */
static void __release_token_from_free_obj(
	IN TPC_OBJ *obj
	)
{
	void *token = NULL;
	unsigned long flags;
	LIST_HEAD(tmp_data_list);




	/* This call will be executed from tpc_release_obj_for_se_tpg(). So, it
	 * will be
	 *
	 * (1) the obj has been removed from list already
	 * (2) and, the obj status had been changed to O_STS_FREE_BY_PROC
	 */

	spin_lock_bh(&obj->se_tpg->tpc_obj_list_lock);

	spin_lock(&obj->o_data_lock);
	if (!obj->o_token_data){
		spin_unlock(&obj->o_data_lock);	
		spin_unlock_bh(&obj->se_tpg->tpc_obj_list_lock);
		return;
	}
	spin_unlock(&obj->o_data_lock);	

	spin_lock(&obj->o_token_status_lock);
	if (TOKEN_STS_FREE_BY_TOKEN_TIMER(obj->o_token_status)){
		spin_unlock(&obj->o_token_status_lock);
		spin_unlock_bh(&obj->se_tpg->tpc_obj_list_lock);
		return;
	}
	obj->o_token_status = O_TOKEN_STS_FREE_BY_PROC;
	spin_unlock(&obj->o_token_status_lock);

	/* check whether someone still use the token data */
_KEEP_WAIT_:
	spin_lock(&obj->o_token_ref_count_lock);
	if (atomic_read(&obj->o_token_ref_count)){
		spin_unlock(&obj->o_token_ref_count_lock);
		spin_unlock_bh(&obj->se_tpg->tpc_obj_list_lock);


		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(1*1000));

		spin_lock_bh(&obj->se_tpg->tpc_obj_list_lock);
		goto _KEEP_WAIT_;
	}
	spin_unlock(&obj->o_token_ref_count_lock);
	
	/**/
	__tpc_update_obj_token_status_lock(obj, O_TOKEN_STS_DELETED);

	/* to free necessary token data and data node list */
	spin_lock(&obj->o_data_lock);
	token = obj->o_token_data;
	obj->o_token_data = NULL;
	list_splice_tail_init(&obj->o_data_list, &tmp_data_list);
	spin_unlock(&obj->o_data_lock);

	spin_unlock_bh(&obj->se_tpg->tpc_obj_list_lock);
		

	/* start to free all token source */
	del_timer_sync(&obj->o_token_timer);

	if (!list_empty(&tmp_data_list))
		__tpc_free_obj_node_data(&tmp_data_list);

	pr_debug("%s: free token resource:0x%p\n", __func__, token);
	if (token)
		kfree(token);

	return;
}

/*
 * @fn void tpc_release_obj_for_se_tpg(IN LIO_SE_PORTAL_GROUP *se_tpg)
 * @brief To release all obj data in se_tpg structure
 *
 * @sa 
 * @param[in] se_tpg
 * @retval N/A
 */
void tpc_release_obj_for_se_tpg(
    IN LIO_SE_PORTAL_GROUP *se_tpg
    )
{

	TPC_OBJ *obj = NULL, *tmp_obj = NULL;
	unsigned long flags = 0;



	
	/* FIXED ME !!! */
	spin_lock_bh(&se_tpg->tpc_obj_list_lock);
	if (list_empty(&se_tpg->tpc_obj_list)){
		pr_debug("warning: %s - not found any obj, obj_count:0x%x\n", 
			__func__, atomic_read(&se_tpg->tpc_obj_count));

		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
		return;
	}

	list_for_each_entry(obj, &se_tpg->tpc_obj_list, o_node){

		/* change the status first to avoid the fimer will
		 * take everyting */
		__tpc_update_obj_status_lock(obj, O_STS_FREE_BY_PROC);

		/* If the obj is still used, to wait. This shall be safe
		 * cause of this code only be called during to delete target
		 */
_KEEP_WAIT_:
		spin_lock(&obj->o_ref_count_lock);
		if (atomic_read(&obj->o_ref_count)){
			spin_unlock(&obj->o_ref_count_lock);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(msecs_to_jiffies(1*1000));

			spin_lock_bh(&se_tpg->tpc_obj_list_lock);
			goto _KEEP_WAIT_;
		}
		spin_unlock(&obj->o_ref_count_lock);

		__tpc_update_obj_status_lock(obj, O_STS_DELETED);
		__tpc_obj_node_del(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);


		pr_debug("%s: release obj:0x%p(id:0x%x, op_sac:0x%x)\n", 
			__func__, obj, obj->list_id, obj->op_sac);

		del_timer_sync(&obj->o_timer);

		/* To release the token for this obj */
		__release_token_from_free_obj(obj);

		pr_debug("%s: to free obj:0x%p...\n", __func__, obj);
		kfree(obj);

		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
	}
	spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
	return;
}
EXPORT_SYMBOL(tpc_release_obj_for_se_tpg);


/*
 * @fn void __tpc_put_rrti_by_obj(IN TPC_OBJ *obj,
 *                                  IN ROD_TOKEN_INFO_PARAM *param, 
 *                                  IN bool attached_rod_token,
 *                                  IN u32 alloc_len)
 *  
 * @brief To create the RECEIVE ROD TOKEN INFORMATION data into parameter
 *        data from obj structure
 * @sa 
 * @param[in] obj
 * @param[in] param
 * @param[in] attached_rod_token
 * @param[in] alloc_len
 * @retval 0 - Success / Others - Fail
 */
static int __tpc_put_rrti_by_obj(
	IN TPC_OBJ *obj,
	IN ROD_TOKEN_INFO_PARAM *param,
	IN bool attached_rod_token,
	IN u32 alloc_len
	)
{
	u8 *p = NULL;

	if (obj == NULL || param == NULL)
		BUG_ON(TRUE);

	if (alloc_len == 0)
		return 1;

	/* FIXED ME !! */
	if ((obj->cp_op_status == OP_COMPLETED_WO_ERR)
	||  (obj->cp_op_status == OP_COMPLETED_W_ERR)
	||  (obj->cp_op_status == OP_COMPLETED_WO_ERR_WITH_ROD_TOKEN_USAGE)
	||  (obj->cp_op_status == OP_COMPLETED_WO_ERR_BUT_WITH_RESIDUAL_DATA)
	||  (obj->cp_op_status == OP_TERMINATED)
	)
	{
		param->res_to_sac = (u8)obj->op_sac;    
		param->cp_op_status = obj->cp_op_status;
		put_unaligned_be16(obj->op_counter, &param->op_counter[0]);
		put_unaligned_be32(0xfffffffe, 
			&param->estimated_status_update_delay[0]);

		param->extcp_completion_status = obj->completion_status;

		if (obj->cp_op_status  == OP_TERMINATED)
			param->sense_data_len_field = param->sense_data_len = 0;
		else
			param->sense_data_len_field = param->sense_data_len = ROD_SENSE_DATA_LEN;

		param->transfer_count_units = UNIT_NA;

		put_unaligned_be16(obj->segs_processed, &param->seg_processed[0]);
		put_unaligned_be64(obj->transfer_count, &param->transfer_count[0]);

		/* go to sense data field */
		p = (u8*)((size_t)param + sizeof(ROD_TOKEN_INFO_PARAM));
		if ((param->sense_data_len_field != 0)
		&& (param->sense_data_len != 0)
		)
		{
  			/* To update the sense data to parameter buffer */


			memcpy(p, &obj->sense_data[0], ROD_SENSE_DATA_LEN);

			/* go to rod token descriptor length field */
			p = (u8*)((size_t)p + ROD_SENSE_DATA_LEN);
			put_unaligned_be32(0, &p[0]);

			/* SPC4R36, page 431 */
			if ((obj->cp_op_status == OP_COMPLETED_WO_ERR)
			||  (obj->cp_op_status == OP_COMPLETED_WO_ERR_WITH_ROD_TOKEN_USAGE)
			)
			{
				if ((attached_rod_token == 1) 
				&& (obj->o_token_data != NULL)
				)
				{
					/* To set the rod token descriptor 
					 * length if cp_op_status is w/o 
					 * any error */
					put_unaligned_be32(
						(ROD_TOKEN_MIN_SIZE + 2), &p[0]);

					/* SPC4R36, page 431 
					 * If the response to service action 
					 * field is not 0x0 or 0x1, the 
					 * ID FOR CREATING ROD CSCD DESCRIPTOR 
					 * field shall be reserved.
					 */
					memcpy(&p[4 + 2], obj->o_token_data, 
						ROD_TOKEN_MIN_SIZE);
				}
			}
		}

		put_unaligned_be32((alloc_len - 4), 
			&param->avaiable_data_len[0]);
		return 0;
	}

	/* The code shall NEVER come here !! */
	pr_err("error !! wrong way in %s\n", __func__);
	BUG_ON(TRUE);
	return 1;
}

/*
 * @fn static int __tpc_put_rrti_by_tdata(IN LIO_SE_CMD *t_cmd,
 *                                  IN ROD_TOKEN_INFO_PARAM *param, 
 *                                  IN T_CMD_STATUS status)
 *  
 * @brief To create the RECEIVE ROD TOKEN INFORMATION data into parameter
 *        data from track se_cmd data
 * @sa 
 * @param[in] obj
 * @param[in] param
 * @param[in] status
 * @retval 0 - Success / Others - Fail
 */
static int __tpc_put_rrti_by_tdata(
    IN LIO_SE_CMD *t_cmd,
    IN ROD_TOKEN_INFO_PARAM *param,
    IN T_CMD_STATUS status
    )
{
    TPC_TRACK_DATA *td = NULL;
    unsigned long flags = 0;

    /* If call this function to build the RECEIVE ROD TOKEN INFORMATION parameter
     * data, it means the command matched with list id in RECEIVE ROD TOKEN INFORMATION
     * is still processing now ...
     */
    BUG_ON(!t_cmd);
    BUG_ON(!t_cmd->t_track_rec);
    BUG_ON(!param);

    td = (TPC_TRACK_DATA *)t_cmd->t_track_rec;

    if ((status == T_CMD_IS_STARTING_IN_FG)
    ||  (status == T_CMD_IS_STARTING_IN_BG)
    ||  (status == T_CMD_WAS_ABORTED)
    )
    {
        param->res_to_sac = (u8)t_cmd->t_op_sac;
    
        /* SPC4R36, page 424 */
        if (status == T_CMD_IS_STARTING_IN_FG)
            param->cp_op_status = OP_IN_PROGRESS_WITHIN_FG;
        else if (status == T_CMD_IS_STARTING_IN_BG)
            param->cp_op_status = OP_IN_PROGRESS_WITHIN_BG;
        else
            param->cp_op_status = OP_TERMINATED;

        /* FIXED ME !! 
         *
         * All data set below shall be checked again !!
         */
        put_unaligned_be16(td->t_op_counter, &param->op_counter[0]);
        put_unaligned_be32(
                    ESTIMATE_STATUS_UPDATE_DELAY,
                    &param->estimated_status_update_delay[0]
                    );

        if (param->cp_op_status == OP_TERMINATED)
            put_unaligned_be32(0xfffffffe, &param->estimated_status_update_delay[0]);

        /* the extcp_completion_status field ONLY be set if cp_op_status is
         * 0x01,0x02,0x03,0x04 or 0x60
         */
        if (param->cp_op_status == OP_TERMINATED)
            param->extcp_completion_status = SAM_STAT_TASK_ABORTED;

        param->sense_data_len_field = param->sense_data_len = 0x0;

        /* SPC4R36, page 425 */
        param->transfer_count_units = UNIT_NA;

        spin_lock_irqsave(&td->t_count_lock, flags);
        put_unaligned_be64(td->t_transfer_count, &param->transfer_count[0]);
        spin_unlock_irqrestore(&td->t_count_lock, flags);

        put_unaligned_be16(td->t_segs_processed, &param->seg_processed[0]);

        /* SPC4R36, page 430 (or SBC3R31, page 155) */
        put_unaligned_be32((32-4), &param->avaiable_data_len[0]);
        return 0;
    }

    /* The code shall NEVER come here !! */
    pr_err("error !! wrong way in %s\n", __func__);
    return 1;
}


/*
 * @fn static int __do_chk_overlap_lba_in_desc_list(IN LIO_SE_CMD *se_cmd,
 *                                  IN void *start, 
 *                                  IN u16 counts)
 *  
 * @brief To check whether there is any overlapped LBA in each block descriptor data
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] start
 * @param[in] counts
 * @retval 0 - Success / Others - Fail
 */
static int __do_chk_overlap_lba_in_desc_list(
    IN LIO_SE_CMD *se_cmd,
    IN void *start,
    IN u16 counts
    )
{
    BLK_DEV_RANGE_DESC *src_desc = NULL;
    u16 index0 = 0, index1 = 0;
    sector_t lba = 0, s = 0, r = 0;

    if (se_cmd == NULL || start == NULL || counts== 0)
        BUG_ON(TRUE);

    src_desc =  (BLK_DEV_RANGE_DESC *)start;

    for (index0 = 0; index0 < counts; index0++){

        if (get_unaligned_be32(&src_desc[index0].nr_blks[0]) == 0)
            continue;

        lba = (sector_t)(get_unaligned_be64(&src_desc[index0].lba[0]));

        /**/
        for (index1 = 0; index1 < counts; index1++){

            if (get_unaligned_be32(&src_desc[index1].nr_blks[0]) == 0)
                continue;

            if (index0 == index1)
                continue;

            s = (sector_t)get_unaligned_be64(&src_desc[index1].lba[0]);
            r = (sector_t)get_unaligned_be32(&src_desc[index1].nr_blks[0]);
            if ((lba >= s) && (lba < (s + r)))
                return 1;
        }
    }

    return 0;

}


/*
 * @fn static int __do_chk_same_lba_in_desc_list(IN LIO_SE_CMD *se_cmd,
 *                                  IN void *start, 
 *                                  IN u16 counts)
 *  
 * @brief To check whether there is any same LBA in each block descriptor data
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] start
 * @param[in] counts
 * @retval 0 - Success / Others - Fail
 */
int __do_chk_same_lba_in_desc_list(
    IN LIO_SE_CMD *se_cmd,
    IN void *start,
    IN u16 counts
    )
{
    BLK_DEV_RANGE_DESC *src_desc = NULL;
    u16 index0 = 0, index1 = 0;
    sector_t lba = 0;

    if (se_cmd == NULL || start == NULL || counts== 0)
        BUG_ON(TRUE);

    src_desc =  (BLK_DEV_RANGE_DESC *)start;

    /**/
    for (index0 = 0; index0 < counts; index0++){

        if (get_unaligned_be32(&src_desc[index0].nr_blks[0]) == 0)
            continue;

        lba = (sector_t)(get_unaligned_be64(&src_desc[index0].lba[0]));

        for (index1 = 0; index1 < counts; index1++){
            if (get_unaligned_be32(&src_desc[index1].nr_blks[0]) == 0)
                continue;

            if (index0 == index1)
                continue;

            if (lba == (sector_t)get_unaligned_be64(&src_desc[index1].lba[0]))
                return 1;
        }
    }
    return 0;
}


/*
 * @fn static int __do_chk_max_lba_in_desc_list(IN LIO_SE_CMD *se_cmd,
 *                                  IN void *start, 
 *                                  IN u16 counts)
 *  
 * @brief To check whether the LBA in each block descriptor data exceeds the max LBA
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] start
 * @param[in] counts
 * @retval 0 - Success / Others - Fail
 */
static int __do_chk_max_lba_in_desc_list(
    IN LIO_SE_CMD *se_cmd,
    IN void *start,
    IN u16 counts
    )
{
    BLK_DEV_RANGE_DESC *src_desc = NULL;
    u16 index0 = 0;
    sector_t lba = 0;

    if (se_cmd == NULL || start == NULL || counts== 0)
        BUG_ON(TRUE);

    src_desc = (BLK_DEV_RANGE_DESC *)start;

    for (index0 = 0; index0 < counts ; index0++){

        lba = (sector_t)(get_unaligned_be64(&src_desc->lba[0]) + \
                                (u64)get_unaligned_be32(&src_desc->nr_blks[0]) - 1);

        if (lba > se_cmd->se_dev->transport->get_blocks(se_cmd->se_dev))
            return 1;

        src_desc  = (BLK_DEV_RANGE_DESC *)((u8*)src_desc + sizeof(BLK_DEV_RANGE_DESC));
    }
    return 0;
}

/*
 * @fn int __do_wzrt (IN LIO_SE_CMD *se_cmd, IN void *new_obj)
 * @brief wrapper function to do WRITE ZERO ROD TOKEN for block i/o or file i/o
 *
 * @param[in] se_cmd
 * @param[in] new_obj
 * @retval Depend on the result of __do_b_f_wzrt()
 */
int __do_wzrt(
	IN LIO_SE_CMD *se_cmd, 
	IN void *new_obj
	)
{
	struct request_queue *q = NULL;
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *inode = NULL;
	struct block_device *bd = NULL;
	LIO_IBLOCK_DEV *ib_dev = NULL;
	ERR_REASON_INDEX err = ERR_INVALID_CDB_FIELD;
	int ret = 1;
	SUBSYSTEM_TYPE subsys_type;

	/* 20140701, adamhsu, redmine 8826 (start) */
	int go_fio_filebackend_op = 0;

#if defined(SUPPORT_FILEIO_ON_FILE)
	go_fio_filebackend_op = 1;
#endif
	/* 20140701, adamhsu, redmine 8826 (end) */


	/* This function will write-zero to specific range indicated by
	 * device block descriptor. In the other words, it is similar to 
	 * write-same w/o unmap command. So here need to check what kind of
	 * device type (thin or thick)
	 *
	 * Note:
	 * Currently, we only support two products for this function.
	 *
	 * (1) LIO block i/o + block backend device (fbdisk)
	 * (2) LIO file i/o + block backend device
	 * (3) LIO file i/o + file backend device
	 */
	if (!se_cmd || !new_obj)
		goto _OUT_;

	if(__do_get_subsystem_dev_type(se_cmd->se_dev, &subsys_type))
		goto _OUT_;

	if (is_thin_lun(se_cmd->se_dev)){

		/* FIXED ME !!
		 *
		 * If it is thin device, to check whether the backend device
		 * supports discard function or not.
		 */
		if (subsys_type == SUBSYSTEM_FILE){

			fd_dev = (LIO_FD_DEV *)se_cmd->se_dev->dev_ptr;
			inode  = fd_dev->fd_file->f_mapping->host;

			/* 20140701, adamhsu, redmine 8826 (start) */
			if (!S_ISBLK(inode->i_mode)){
				if (go_fio_filebackend_op)
					goto _GO_FUNC_;
				else
					goto _OUT_;
			}
			bd = inode->i_bdev;

			/* 20140701, adamhsu, redmine 8826 (end) */

		} else {
			ib_dev = (LIO_IBLOCK_DEV *)se_cmd->se_dev->dev_ptr;	
			bd = ib_dev->ibd_bd;
		}

		/* TODO
		 * Here is for LIO block i/o (or file i/o) + block backend device. 
		 * To refer blkdev_issue_discard() to do pre-condition checking 
		 */
		q = bdev_get_queue(bd);
		if (!blk_queue_discard(q)){
			pr_err("[wzrt] blk dev not support discard func\n");
			goto _OUT_;
		}

		/* FIXED ME */
		if (blk_queue_secdiscard(q)){
			pr_err("[wzrt] not support secure-discard currently\n");
			goto _OUT_;
		}

		if (!q->limits.max_discard_sectors){
			pr_err("[wzrt] max_discard_sectors of blk dev is zero\n");
			goto _OUT_;
		}
	}


_GO_FUNC_:
	/* Here will return directly since this function will be called
	 * from __do_write_by_token() */
	return __do_b_f_wzrt(se_cmd, is_thin_lun(se_cmd->se_dev), new_obj);

_OUT_:
	if (((TPC_OBJ *)new_obj)->o_token_data)
		kfree(((TPC_OBJ *)new_obj)->o_token_data);

	kfree(new_obj);
	__set_err_reason(err, &se_cmd->scsi_sense_reason);
	return ret;

}


/*
 * @fn int __do_wzrt (IN LIO_SE_CMD *se_cmd, IN bool is_thin_dev, IN void *new_obj)
 *
 * @brief main function to do WRITE ZERO ROD TOKEN for block i/o or file i/o
 * @note
 * @param[in] se_cmd
 * @param[in] is_thin_dev
 * @param[in] new_obj
 * @retval 0  - Success / 1- Fail
 */
static int __do_b_f_wzrt(
	IN LIO_SE_CMD *se_cmd, 
	IN bool is_thin_dev,
	IN void *new_obj
	)
{
	WRITE_BY_TOKEN_PARAM *param = NULL;
	BLK_DEV_RANGE_DESC *start = NULL;
	TPC_OBJ *n_obj = NULL;
	TPC_TRACK_DATA *t_data = NULL;
	sector_t d_nr_blks = 0, dest_lba = 0;
	u32 w_nr_blks = 0, real_w_blks = 0, tmp_w_blks = 0;
	u32 expected_blks = 0, d_bs_order = 0;
	u16 desc_counts = 0, index = 0;
	u64 expected_bytes = 0, tmp_w_bytes = 0;
	unsigned long timeout_jiffies = 0;
	int w_done = 0, exit_loop = 0,  ret = -1, alloc_ret;
	T_CMD_STATUS cur_status = T_CMD_COMPLETED_W_ERR;
	ERR_REASON_INDEX err = ERR_INVALID_PARAMETER_LIST;
	GEN_RW_TASK w_task;

	u32 compare_done_blks;



	/* Beware this !!!
	 * This call doesn't the same as __do_write_by_token() function. For
	 * write zero ROD token, we don't need any src obj and src token data.
	 */
	if (!se_cmd || !new_obj){
		err = ERR_INVALID_CDB_FIELD;
		goto _OUT_2_;
	}

	param = (WRITE_BY_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd);
	if (!param)
		goto _OUT_2_;

	n_obj       = (TPC_OBJ *)new_obj;
	t_data      = (TPC_TRACK_DATA *)se_cmd->t_track_rec;
	d_bs_order  = n_obj->dev_bs_order;

	/* The block dev range desc length shall be a multiple of 16 */
	start  = (BLK_DEV_RANGE_DESC *)((u8*)param + sizeof(WRITE_BY_TOKEN_PARAM));
	desc_counts = __tpc_get_desc_counts(
		get_unaligned_be16(&param->blkdev_range_desc_len[0]));
	d_nr_blks = __tpc_get_total_nr_blks_by_desc(start, desc_counts);

	/* To start to process the command and indicate we will use the
	 * track data now 
	 */

	/* 2014/08/17, adamhsu, redmine 9007 */
	__tpc_td_ref_count_lock_inc(t_data);
	__tpc_update_t_cmd_status(t_data, T_CMD_IS_STARTING_IN_FG);

	/* FIXED ME !! */
	timeout_jiffies = jiffies + msecs_to_jiffies(ROD_TIMEOUT);;

	for (index = 0; index < desc_counts; index++){
		if (get_unaligned_be32(&start[index].nr_blks[0]) == 0)
			continue;

		dest_lba  = get_unaligned_be64(&start[index].lba[0]);
		w_nr_blks = get_unaligned_be32(&start[index].nr_blks[0]);

		pr_debug("[wzrt-bf] (a) index:0x%x, dest_lba:0x%llx, "
			"w_nr_blks:0x%llx, w-dev bs-order:0x%x\n", index,
			(unsigned long long)dest_lba, 
			(unsigned long long)w_nr_blks, d_bs_order);

_DO_AGAIN_:
		if (__tpc_is_lun_receive_stop(se_cmd) 
		|| (__tpc_is_se_tpg_actived(se_cmd) == 1)
		){
			if (se_cmd->cur_se_task->task_flags & TF_REQUEST_STOP)
				transport_complete_task(se_cmd->cur_se_task, 0);

			/* 2014/08/17, adamhsu, redmine 9007 */
			__tpc_td_ref_count_lock_dec(
				(TPC_TRACK_DATA *)se_cmd->t_track_rec);

			err = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			goto _OUT_2_;
		}

		/* FIXED ME !!
		 *
		 * We do this cause of the total number of blocks in single
		 * descriptor passing by host may be too large. To avoid the
		 * code will create many bios at a time, to limit the number
		 * of blocks to be processed by  OPTIMAL_TRANSFER_SIZE_IN_BYTES.
		 */
		tmp_w_blks = real_w_blks = min_t(u32, w_nr_blks, 
			(OPTIMAL_TRANSFER_SIZE_IN_BYTES >> d_bs_order));

		pr_debug("[wzrt-bf] real_w_blks:0x%x\n", real_w_blks);

		/**/
		tmp_w_bytes = ((u64)tmp_w_blks << d_bs_order);

		while (tmp_w_bytes){

			expected_bytes = tmp_w_bytes;
			memset((void *)&w_task, 0, sizeof(GEN_RW_TASK));

			/* prepare the w task */
			w_task.se_dev = n_obj->se_lun->lun_se_dev;
			w_task.lba = dest_lba;
			w_task.dev_bs_order = d_bs_order;
			w_task.dir = DMA_TO_DEVICE;
			w_task.timeout_jiffies = timeout_jiffies;
			w_task.is_timeout = 0;
			w_task.ret_code = 0;

			/* 20140630, adamhsu, redmine 8826 (start) */

			pr_debug("[wzrt-bf] w-task: dest_lba:0x%llx, "
				"expected_bytes:0x%llx\n", 
				(unsigned long long)dest_lba, expected_bytes);

			alloc_ret = __generic_alloc_sg_list(
				&expected_bytes, &w_task.sg_list, 
				&w_task.sg_nents);
			
			if (alloc_ret != 0){
				if (alloc_ret == -ENOMEM)
					pr_err("[wzrt-bf] fail to alloc sg list\n");
				if (alloc_ret == -EINVAL)
					pr_err("[wzrt-bf] invalid arg during "
						"to alloc sg list\n");
				goto _OUT_1_;
			}

			w_task.nr_blks = expected_blks = 
				(expected_bytes >> d_bs_order);

			/* For thin dev, we have chance to do fast-zero
			 * otherwise we always will NOT do it
			 */
			if (is_thin_dev){
				w_task.s_nr_blks = (tmp_w_bytes >> d_bs_order);
				compare_done_blks = (tmp_w_bytes >> d_bs_order);
			} else {
				w_task.s_nr_blks = 0;
				compare_done_blks = expected_blks;
			}


			pr_debug("[wzrt-bf] start rw for device, "
				"expected blks:0x%x, s_nr_blks:0x%x, "
				"dev type:0x%x\n", expected_blks, 
				w_task.s_nr_blks, is_thin_dev);

			if (!is_thin_dev)
				w_done = __tpc_do_rw(&w_task);
			else 
				w_done = __tpc_do_zero_rod_token_w(&w_task);


			if((w_done <= 0) || w_task.is_timeout || w_task.ret_code != 0){
				if (w_task.ret_code == -ENOSPC)
					err = ERR_NO_SPACE_WRITE_PROTECT;
				else
					err = ERR_3RD_PARTY_DEVICE_FAILURE;
				exit_loop = 1;
			} 
			else if (w_done != compare_done_blks)
				pr_err("[wzrt-bf] w_done != expected write blks\n");

			__generic_free_sg_list(w_task.sg_list, w_task.sg_nents);

			/* 20140630, adamhsu, redmine 8826 (end) */

			__tpc_update_t_cmd_transfer_count(t_data, (sector_t)w_done);

			/* To exit this loop if hit any error */
			if (w_task.is_timeout || exit_loop)
				goto _OUT_1_;

			/* If this real write byet counts doesn's equal to
			 * expectation of write byte counts at this round then
			 * to break the loop 
			 */
			if (w_done != compare_done_blks)
				goto _OUT_1_;

			tmp_w_bytes -= (compare_done_blks << d_bs_order);
			dest_lba += (sector_t)w_done;

			pr_debug("[wzrt-bf] w_done:0x%llx, "
				"tmp_w_bytes:0x%llx, dest_lba:0x%llx\n", 
				(unsigned long long)w_done, 
				(unsigned long long)tmp_w_bytes, 
				(unsigned long long)dest_lba);
		}

		w_nr_blks -= real_w_blks;
		pr_debug("[wzrt-bf] (b) w_nr_blks:0x%llx\n", 
			(unsigned long long)w_nr_blks);

		if (w_nr_blks)
			goto _DO_AGAIN_;
	}


_OUT_1_:

	/* please refer the sbc3r31, page 88 */
	if (d_nr_blks == t_data->t_transfer_count){
		cur_status = T_CMD_COMPLETED_WO_ERR;
		n_obj->cp_op_status = OP_COMPLETED_WO_ERR;
	}
	else{
		if (param->immed)
			n_obj->completion_status = SAM_STAT_CHECK_CONDITION;

		if (err == ERR_NO_SPACE_WRITE_PROTECT) {
			/* treat it as copy-error if hit no sapce event */
			n_obj->cp_op_status = OP_COMPLETED_W_ERR;
			err = ERR_NO_SPACE_WRITE_PROTECT;
			pr_warn("[write by zero token] space was full\n");
		} else {	
			n_obj->cp_op_status = OP_COMPLETED_WO_ERR_WITH_ROD_TOKEN_USAGE;

			if (t_data->t_transfer_count)
				err = ERR_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET;
			else
				err = ERR_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET;
		}
	}

	__do_get_subsystem_dev_type(se_cmd->se_dev, &n_obj->backend_type);
	__tpc_get_isid_by_se_td(se_cmd, n_obj->isid);
	__tpc_get_tiqn_and_pg_tag_by_se_td(se_cmd, n_obj->tiqn, &n_obj->pg_tag);
	__tpc_set_obj_completion_status(n_obj);
	__tpc_set_obj_op_counter(n_obj);

	/* The WRITE USING TOKEN obj won't keep anything here */
	n_obj->o_data_type     = O_DATA_TYPE_NONE;
	n_obj->list_id        = se_cmd->t_list_id;
	n_obj->op_sac         = se_cmd->t_op_sac;
	n_obj->segs_processed = t_data->t_segs_processed = 0;
	n_obj->transfer_count = t_data->t_transfer_count;

	ret = 0;
	if (n_obj->cp_op_status != OP_COMPLETED_WO_ERR){
		__tpc_build_obj_sense_data(n_obj, err, 0, 0);
		cur_status = T_CMD_COMPLETED_W_ERR;
		ret = 1;
	}

	/* not need to protect anything for new obj */
	__tpc_update_obj_status_lock(n_obj, O_STS_ALIVE);
	__tpc_update_obj_token_status_lock(n_obj, O_TOKEN_STS_NOT_ALLOCATED_AND_NOT_ALIVE);
	__tpc_setup_obj_timer(n_obj);

	spin_lock_bh(&n_obj->se_tpg->tpc_obj_list_lock);
	__tpc_obj_node_add(n_obj);
	spin_unlock_bh(&n_obj->se_tpg->tpc_obj_list_lock);


	__tpc_update_t_cmd_status(t_data, cur_status);

	/* 2014/08/17, adamhsu, redmine 9007 */
	__tpc_td_ref_count_lock_dec(
		(TPC_TRACK_DATA *)se_cmd->t_track_rec);

_OUT_:
	if (param)
		transport_kunmap_data_sg(se_cmd);

	if (ret != 0)
		__set_err_reason(err, &se_cmd->scsi_sense_reason);

	return ret;

_OUT_2_:
	if (n_obj->o_token_data)
		kfree(n_obj->o_token_data);
	kfree(n_obj);
	goto _OUT_;

}


/*
 * @fn static int __tpc_check_duplicated_obj_data(IN LIO_SE_CMD *se_cmd)
 * @brief To check whether any obj which its list id matches with se_cmd
 *
 * @sa 
 * @param[in] se_cmd
 * @retval -1 : Other err during call this function
 *         1  : Not found any duplicated obj
 *         0  : Found the duplicated obj
 */
static int __tpc_check_duplicated_obj_data(
	IN LIO_SE_CMD *se_cmd
	)
{
	TPC_OBJ *tpc_obj = NULL;
	LIO_SE_DEVICE *se_dev = NULL;
	LIO_SE_PORTAL_GROUP *se_tpg = NULL;
	unsigned long flags = 0;
	int ret = -1;

	/**/
	if (!se_cmd->is_tpc || !se_cmd->se_lun)
		return ret;

 	if (!se_cmd->se_lun->lun_se_dev)
		return ret;

	if (!se_cmd->se_lun->lun_sep)
		return ret;
	
	if (!se_cmd->se_lun->lun_sep->sep_tpg)
		return ret;

	se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;

	/**/
	ret = 1;
	tpc_obj = __tpc_get_obj_by_id_opsac(se_tpg, se_cmd);

	if (tpc_obj){
		/* To discard related data. SPC4R36 , page 428 
		 *
		 * Since we found the duplicated obj by same list id and op sac,
		 * so to delete its node from list and let's its obj timer call
		 * to free its resource.
		 */
		pr_warn("warning: found matched tpc_obj (id:0x%x, op_sac:0x%x), "
			" to discard the tpc_obj first ...\n", 
			tpc_obj->list_id, tpc_obj->op_sac);
	
		/* To cancel the token here since someone may use it now */
		spin_lock_bh(&tpc_obj->se_tpg->tpc_obj_list_lock);

		__tpc_token_ref_count_lock_inc(tpc_obj);

		spin_lock(&tpc_obj->o_token_status_lock);
		if (TOKEN_STS_ALIVE(tpc_obj->o_token_status))
			tpc_obj->o_token_status = O_TOKEN_STS_CANCELLED;
		spin_unlock(&tpc_obj->o_token_status_lock);

		__tpc_token_ref_count_lock_dec(tpc_obj);

		/* To match the __tpc_obj_ref_count_lock_inc() in 
		 * __tpc_get_obj_by_id_opsac() 
		 */
		__tpc_obj_ref_count_lock_dec(tpc_obj);
		__tpc_obj_node_del(tpc_obj);

		spin_unlock_bh(&tpc_obj->se_tpg->tpc_obj_list_lock);

		/* If found duplicated obj data */
		ret = 0;
	}
	
	return ret;
}


/*
 * @fn static int __do_chk_before_write_by_token(IN LIO_SE_CMD *se_cmd)
 * @brief To check some condiftions before to do WRITE USING TOKEN command
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in,out] err
 * @retval 0: Success / 1: Success but no need to transfer data / Others: Fail
 */
static int __do_chk_before_write_by_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err
	)
{
	BLK_DEV_RANGE_DESC *start = NULL;
	WRITE_BY_TOKEN_PARAM *param = NULL;
	ROD_TOKEN *token = NULL;
	u8 *cdb = NULL;
	u16 desc_counts = 0;
	u32 d_bs_order = 0, list_id = 0;
	sector_t all_nr_blks = 0;
	int ret = -1;

	/**/
	list_id = get_unaligned_be32(&CMD_TO_TASK_CDB(se_cmd)->t_task_cdb[6]);

	param = (WRITE_BY_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd);
	if (!param){
		pr_err("error !! fail to kmap data seg (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	/* SBC3R31, page 207, 208 */
	cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;

	if (get_unaligned_be32(&cdb[10]) < 552){
		pr_err("error !! alloc len is smaller than 552 (id:0x%x) !!\n",
			list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	if (get_unaligned_be16(&param->token_data_len[0]) < 550){
		pr_err("error !! avaiable data len in param is smaller than 550 "
			"(id:0x%x) !!\n",list_id);;
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	if (get_unaligned_be16(&param->blkdev_range_desc_len[0]) < 0x10){
		pr_err("error !! blk dev range desc length is not enougth "
			"(id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	token= (ROD_TOKEN *)&param->rod_token[0];
	if (get_unaligned_be16(&token->token_len[0]) != 0x1f8 ){
		pr_err("error !! token length is NOT 0x1f8 (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN;
		goto _EXIT_;
	}

	if (__chk_valid_supported_rod_type(get_unaligned_be32(&token->type[0])) == 1){
		pr_err("error !! unsupported token type (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE;
		goto _EXIT_;
	}

	/* The block dev range desc length shall be a multiple of 16 */
	start        = (BLK_DEV_RANGE_DESC *)((u8*)param + sizeof(WRITE_BY_TOKEN_PARAM));
	desc_counts  = __tpc_get_desc_counts(get_unaligned_be16(&param->blkdev_range_desc_len[0]));
	all_nr_blks  = __tpc_get_total_nr_blks_by_desc(start, desc_counts);

	/*
	* If the NUMBER_OF_LOGICAL_BLOCKS field is set to zero, then the copy
	* manager should perform no operation for this block device range
	* descriptor. This condition shall not be considered an error
	*/
	if (!all_nr_blks){
		ret = 1;
		goto _EXIT_;
	}

	if (__do_chk_same_lba_in_desc_list(se_cmd, (void*)start, desc_counts) == 1){
		pr_err("error !! found same LBA in blk dev range "
			"descriptor (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	if (__do_chk_overlap_lba_in_desc_list(se_cmd, start, desc_counts) != 0){
		pr_err("error !! found overlapped LBA in blk dev range "
			"descriptor (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	if (__do_chk_max_lba_in_desc_list(se_cmd, start, desc_counts) != 0){
		pr_err("error !! found LBA in blk dev range descriptor execeeds "
			"the max LBA of device (id:0x%x) !!\n",list_id);
		*err = (int)ERR_LBA_OUT_OF_RANGE;
		goto _EXIT_;
	}

	if (desc_counts > __tpc_get_max_supported_blk_dev_range(se_cmd)){
		pr_err("error !! blk dev range descriptor length exceeds "
			"the max value (id:0x%x) !!\n",list_id);
		*err = (int)ERR_TOO_MANY_SEGMENT_DESCRIPTORS;
		goto _EXIT_;
	}

	/* SBC3R31, page 209 (FIXED ME !!)
	*
	* d). To check the total sum of the NUMBER OF LOGICAL BLOCKS fields in all 
	*     of the complete block device range descriptors is larger than the
	*     max bytes in block ROD value in the BLOCK ROD device type specific 
	*     features descriptor in the ROD token features third-party copy
	*     descriptors in the third-party copy vpd page or not
	*
	* FIXED ME !!
	*
	* This setting shall be checked with __build_blkdev_rod_limits_desc()
	* and __build_rod_token_feature() again
	*/
	d_bs_order = ilog2(se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size);

	if (all_nr_blks > (MAX_TRANSFER_SIZE_IN_BYTES >> d_bs_order)){
		pr_err("error !! sum of contents in blk dev range descriptor "
			"length exceeds the max value (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	ret = 0;

_EXIT_:
	if (param)
		transport_kunmap_data_sg(se_cmd);
    
	return ret;

}


/*
 * @fn static int __do_chk_wt_rod_off_before_wt(IN LIO_SE_CMD *se_cmd)
 * @brief To check offset into ROD before to do WRITE USING TOKEN command
 *
 * @sa 
 * @param[in] se_cmd
 * @retval 0 -Success / Others - Fail
 */
static int __do_chk_wt_rod_off_before_wt(
	IN LIO_SE_CMD *se_cmd, 
	IN void *token_src_obj
	)
{
	WRITE_BY_TOKEN_PARAM *param = NULL;
	BLK_DEV_RANGE_DESC *start = NULL;
	TPC_OBJ *s_obj = NULL;
	ERR_REASON_INDEX err = MAX_ERR_REASON_INDEX;
	u64 s_nr_v = 0, d_nr_v = 0, off_to_rod_v = 0;
	u32 bs_order = 0;
	u16 desc_counts = 0;
	int ret = 1;

	/* Beware this !!!
	 *
	 * When code comes here, it means we got the src obj indicated by
	 * token data already and ref count of src-obj and token had been
	 * increased by 1. Howoever, we still need to check the status of obj
	 * and token before to use them.
	 */
	if((param = (WRITE_BY_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd)) == NULL){
		err = ERR_INVALID_PARAMETER_LIST;
		goto _OUT_;
	}

	/* The block dev range desc length shall be a multiple of 16 */
	s_obj = (TPC_OBJ *)token_src_obj;
	start = (BLK_DEV_RANGE_DESC *)((u8*)param + sizeof(WRITE_BY_TOKEN_PARAM));
	desc_counts = __tpc_get_desc_counts(
			get_unaligned_be16(&param->blkdev_range_desc_len[0]));

	d_nr_v = (u64)__tpc_get_total_nr_blks_by_desc(start, desc_counts);

	spin_lock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	s_nr_v = (u64)__tpc_get_nr_blks_by_s_obj(s_obj, 0);
	spin_unlock_bh(&s_obj->se_tpg->tpc_obj_list_lock);


	bs_order = ilog2(se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size);

	/* SBC3R31, page 208 
	 *
	 * If the computed the byte offset into the ROD is larger than or equal
	 * to the number of bytes represented by the ROD token, then the copy
	 * manager shall terminate the command with CHECK CONDITION status with
	 * IIEGAL REQUEST and INVALID FIELD IN PARAMETER LIST
	 */
	off_to_rod_v = get_unaligned_be64(&param->off_into_rod[0]);
	
	/* convert to byte count unit first */
	off_to_rod_v <<= bs_order;
	s_nr_v <<= s_obj->dev_bs_order;
	d_nr_v <<= bs_order;

	if (off_to_rod_v >= s_nr_v){
		pr_err("error !! value of offset-into-ROD(0x%llx) is larger than "
			"total blks(0x%llx) of src obj (id:0x%x) !! "
			"s-dev bs_order:0x%x, d-dev bs_order:0x%x\n",
			(unsigned long long)off_to_rod_v,
			(unsigned long long)s_nr_v, s_obj->list_id,
			s_obj->dev_bs_order, bs_order);
		err = ERR_INVALID_PARAMETER_LIST;
		goto _OUT_;    
	}

	/* SBCR31, page 209
	 *
	 * If the numberof bytes of user data represented by the sum of the 
	 * contents of the NUMBER OF LOGICAL BLOCKS fields in all of the 
	 * complete block device range descriptors is larger than the number of
	 * bytes in the data represented by the ROD token minus the computed 
	 * byte offset into the ROD (i.e. the total requested length of the 
	 * transfer exceeds the length of the data avaiable in the data 
	 * represented by the ROD token), then the copy manager
	 * shall :
	 *
	 * (1) transfer as many whole logical blocks; and
	 *
	 * (2) if any portion of a logical block that is written by the copy
	 *     manager corresponds offsets into the ROD at or beyond the length
	 *     of the data represented by the ROD token, write that portion of
	 *     the logical block with user data with all bits set to zero
	 *
	 * But, for this case, offload scsi compliance test in HCK in windows want
	 * the device server terminated with CHECK CONDITION and set the ASC, 
	 * ASCQ to ILLEGAL REQUEST and COPY TARGET DEVICE DATA UNDERRUN.
	 *
	 * p.s. We need to check this again !!
	 */
	if (d_nr_v > (s_nr_v - off_to_rod_v)){
		err = ERR_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET;
		pr_err("error !! total bytes(0x%llx) of desc-obj(0x%x) "
			"is larger than avaiable ROD data (bytes:0x%llx, "
			"offset:0x%llx) !! s-dev bs_order:0x%x, "
			"d-dev bs_order:0x%x\n",
			(unsigned long long)d_nr_v, se_cmd->t_list_id,
			(unsigned long long)s_nr_v, 
			(unsigned long long)off_to_rod_v, s_obj->dev_bs_order, 
			bs_order);
		goto _OUT_;
	}
	/**/
	ret = 0;

_OUT_:
	if (param)
		transport_kunmap_data_sg(se_cmd);

	if (ret)
		__set_err_reason(err, &se_cmd->scsi_sense_reason);

	return ret;

}

/*
 * @fn static int __do_chk_before_populate_token(IN LIO_SE_CMD *se_cmd)
 * @brief To check some condiftions before to do POPULATE TOKEN command
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in,out] err
 * @retval 0: Success / 1: Success but no need to transfer data / Others: Fail
 */
static int __do_chk_before_populate_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err
	)
{
	BLK_DEV_RANGE_DESC *start = NULL;
	POPULATE_TOKEN_PARAM *param = NULL;
	u16 desc_counts = 0;
	u32 timeout = 0, d_bs_order = 0, list_id = 0, rod_type = 0;
	int ret = -1;
	sector_t all_nr_blks = 0;

	/* check all conditions are valid or not before to execute command */
	list_id = get_unaligned_be32(&CMD_TO_TASK_CDB(se_cmd)->t_task_cdb[6]);

	param = (POPULATE_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd);
	if (!param){
		pr_err("error !! fail to kmap data seg (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	if (param->rtv){
		/* we don't support to create BLOCK ZERO ROD TOKEN */
        	rod_type = get_unaligned_be32(&param->rod_type[0]);

		if(__chk_valid_supported_rod_type(rod_type)
		|| (rod_type == ROD_TYPE_BLK_DEV_ZERO)
		)
		{
			pr_err("error !! not supported token type:0x%x "
				"in PT parameter data (id:0x%x)\n", 
				rod_type, list_id);
			*err = (int)ERR_INVALID_PARAMETER_LIST;
			goto _EXIT_;
		}
	}

	if ((get_unaligned_be16(&param->token_data_len[0]) < 0x1e)
	||  (get_unaligned_be16(&param->blkdev_range_desc_len[0]) < 0x10)
	)
	{
		pr_err("error !! token data length or blk dev range desc "
			"length is not enough (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	start         = (BLK_DEV_RANGE_DESC *)((u8*)param + sizeof(POPULATE_TOKEN_PARAM));
	desc_counts   = __tpc_get_desc_counts(get_unaligned_be16(&param->blkdev_range_desc_len[0]));
	all_nr_blks   = __tpc_get_total_nr_blks_by_desc(start, desc_counts);

#if 0
	pr_info("%s: desc_counts:0x%x\n", __func__, desc_counts);
	pr_info("%s: all_nr_blks:0x%x\n", __func__, all_nr_blks);
#endif

	/* SBC3R31, page 130
	 *
	 * If the NUMBER_OF_LOGICAL_BLOCKS field is set to zero, then the copy
	 * manager should perform no operation for this block device range
	 * descriptor. This condition shall not be considered an error
	 */
	if (!all_nr_blks){
		ret = 1;
		goto _EXIT_;
	}

	/* a). To check the LBA value, SBC3R31, page 128 */
	if (__do_chk_same_lba_in_desc_list(se_cmd, start, desc_counts) != 0){
		pr_err("error !! found same LBA in blk dev range descriptor "
			"(id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	if (__do_chk_overlap_lba_in_desc_list(se_cmd, start, desc_counts) != 0){
		pr_err("error !! found overlapped LBA in blk dev "
			"range descriptor (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	/* SBC3R31, page 130 */
	if (__do_chk_max_lba_in_desc_list(se_cmd, start, desc_counts) != 0){
		pr_err("error !! found LBA in blk dev range descriptor "
			"execeeds the max LBA of device (id:0x%x) !!\n",list_id);
		*err = (int)ERR_LBA_OUT_OF_RANGE;
		goto _EXIT_;
	}

	/* b). To check timeout value */
	timeout = get_unaligned_be32(&param->inactivity_timeout[0]);
	if (timeout > MAX_INACTIVITY_TIMEOUT) {
		pr_err("error !! timeout value is larger than max-timeout "
			"value (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	/* c). To check the block device range descriptor length */
	if (desc_counts > __tpc_get_max_supported_blk_dev_range(se_cmd)){
		pr_err("error !! blk dev range descriptor length exceeds "
			"the max value (id:0x%x) !!\n",list_id);
		*err = (int)ERR_TOO_MANY_SEGMENT_DESCRIPTORS;
		goto _EXIT_;
	}

	/* SBC3R31, page 128 (FIXED ME !!)
	 *
	 * d). To check the total sum of the NUMBER OF LOGICAL BLOCKS fields in
	 *     all of the complete block device range descriptors is larger than
	 *     the max bytes in block ROD value in the BLOCK ROD device type
	 *     specific features descriptor in the ROD token features third-party
	 *     copy descriptors in the third-party copy vpd page or not
	 *
	 * If the max bytes in block ROD token value in the Block ROD device type
	 * specific features descriptor in the ROD token features 3rd-party copy
	 * descriptor in the 3rd-party copy vpd page is not reported, then the max
	 * token transfer size value in the Block Device ROD Token Limits descriptor
	 * may indicate a different value for the max value.
	 *
	 * FIXED ME !!
	 *
	 * (1) This setting shall be checked with __build_blkdev_rod_limits_desc()
	 *     and __build_rod_token_feature() again
	 *
	 * (2) I set the max token transfer size value to the same here.
	 */
	d_bs_order = ilog2(se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size);

	DBG_ROD_PRINT("%s: max_transfer_blks:0x%x\n", 
		__func__, (MAX_TRANSFER_SIZE_IN_BYTES >> d_bs_order));

	if (all_nr_blks > (MAX_TRANSFER_SIZE_IN_BYTES >> d_bs_order)){
		pr_err("error !! sum of contents of NUMBER OF LOGICAL BLOCKS "
			"field in blk dev range descs length exceeds the "
			"max value (id:0x%x) !!\n",list_id);
		*err = (int)ERR_INVALID_PARAMETER_LIST;
		goto _EXIT_;
	}

	ret = 0;

_EXIT_:


	if (param)
		transport_kunmap_data_sg(se_cmd);

	return ret;
}


/*
 * @fn int __do_rrti_step1 (IN LIO_SE_CMD *se_cmd, IN void *param, IN OUT ERR_REASON_INDEX *err)
 *
 * @brief step1 function to do RECEIVE ROD TOKEN INFORMATION command for block i/o or file i/o
 * @note
 * @param[in] se_cmd
 * @param[in] param
 * @param[in,out] err
 * @retval  0 - Go to __do_rrti_step2()
 *          1 - Success
 *         -1 - Other error during to call function
 */
static int __do_rrti_step1(
    IN LIO_SE_CMD *se_cmd, 
    IN void *param,
    IN OUT ERR_REASON_INDEX *err
    )
{
    TPC_TRACK_DATA *td = NULL;
    LIO_SE_CMD *o_cmd = NULL, *tmp_cmd = NULL;
    LIO_SE_PORTAL_GROUP *se_tpg = NULL;
    LIO_SE_DEVICE *se_dev = NULL;
    T_CMD_STATUS status = 0;
    unsigned long flags = 0;
    u32 cur_list_id = 0;
    ROD_TOKEN_INFO_PARAM *p = NULL;
    int ret = -1;
    u8 *cdb = NULL;

    /**/
    if (!se_cmd ||!param){
        *err = ERR_INVALID_CDB_FIELD;
        goto _OUT_;
    }

    if (!se_cmd->is_tpc || !se_cmd->se_lun){
        *err = ERR_INVALID_CDB_FIELD;
        goto _OUT_;
    }

    p            = (ROD_TOKEN_INFO_PARAM *)param;
    *err         = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;
    se_dev       = se_cmd->se_lun->lun_se_dev;
    cdb          = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;
    cur_list_id  = get_unaligned_be32(&cdb[2]);

    /* If allocation len is zero, it means no any parameter data, so go to step 2 */
    if (!get_unaligned_be32(&cdb[10])){
        ret = 0;
        goto _OUT_;
    }

    /* FIXED ME !! */
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&se_cmd->se_lun->lun_sep_lock);

    if (!se_cmd->se_lun->lun_sep)
        goto _OUT_1_;

    if (!se_cmd->se_lun->lun_sep->sep_tpg)
        goto _OUT_1_;

    se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;

    /* To search the track cmd from this se_tpg */
    spin_lock(&se_tpg->tpc_cmd_track_list_lock);

    list_for_each_entry_safe(o_cmd, tmp_cmd, \
        &se_tpg->tpc_cmd_track_list, t_cmd_node)
    {
        /**/
        if (__tpc_is_lun_receive_stop(se_cmd) || (__is_se_tpg_actived(se_tpg))){
            if (se_cmd->cur_se_task->task_flags & TF_REQUEST_STOP)
                transport_complete_task(se_cmd->cur_se_task, 0);
            goto _OUT_2_;
        }

        if ((o_cmd->t_list_id == cur_list_id) && (o_cmd->t_track_rec)
        &&  (!(__tpc_is_same_i_t_nexus_func2(o_cmd, se_cmd)))
        )
        {
            /* To indicate we are using track data now */
            td = (TPC_TRACK_DATA *)o_cmd->t_track_rec;

	    /* 2014/08/17, adamhsu, redmine 9007 */
            __tpc_td_ref_count_lock_inc(td);


            /* To to check what kind of status for track data*/
            status = __tpc_get_t_cmd_status(td);

            if (T_CMD_STS_IS_NOT_START(status)){
               /* FIXED ME 
                *
                * If we found the matched list id in tracking list and
                * matched command  is not working now, then to report error
                */
                DBG_ROD_PRINT("warning: (id:0x%x, op_sac:0x%x) doesn't start "
                   "work yet\n", o_cmd->t_list_id, o_cmd->t_op_sac);

		/* 2014/08/17, adamhsu, redmine 9007 */
               	__tpc_td_ref_count_lock_dec(td);
                *err = ERR_INVALID_PARAMETER_LIST;
                goto _OUT_2_;
            }
            else if (T_CMD_STS_IS_START_FG(status)
            || T_CMD_STS_IS_START_BG(status)
            || T_CMD_STS_WAS_ABORT(status) // <-- FIXED ME !!!!
            )
            {
                /* if cmd is still in processing or has been aborted ... */
                DBG_ROD_PRINT("warning: (id:0x%x, op_sac:0x%x) is in process...\n",
                o_cmd->t_list_id, o_cmd->t_op_sac);

                ret = __tpc_put_rrti_by_tdata(o_cmd, p, status);
                if (ret != 0){
                    *err = ERR_INVALID_PARAMETER_LIST;
                    ret = -1;
                }else
                    ret = 1;

		/* 2014/08/17, adamhsu, redmine 9007 */
                __tpc_td_ref_count_lock_dec(td);
                goto _OUT_2_;
            }
            else if (T_CMD_STS_WAS_COMPLETED_W_ERR(status)
            || T_CMD_STS_WAS_COMPLETED_WO_ERR(status)
            )
            {
                /* if cmd was completed, to break loop of step 1 then
                 * go to step 2
                 */
                DBG_ROD_PRINT("(id:0x%x, op_sac:0x%x) was completed..."
                "go to step 2 !!\n", o_cmd->t_list_id, o_cmd->t_op_sac);

		/* 2014/08/17, adamhsu, redmine 9007 */
                __tpc_td_ref_count_lock_dec(td);
                ret = 0;
                goto _OUT_2_;
            }else{
                pr_err("warning: %s - unknown track cmd status\n", __func__);

		/* 2014/08/17, adamhsu, redmine 9007 */
                __tpc_td_ref_count_lock_dec(td);
                *err = ERR_INVALID_PARAMETER_LIST;
                goto _OUT_2_;
            }
        }
    }

    ret = 0;
    DBG_ROD_PRINT("no list id(0x%x) in track cmd list, go to step2\n", cur_list_id);

_OUT_2_:
    spin_unlock(&se_tpg->tpc_cmd_track_list_lock);

_OUT_1_:
    spin_unlock(&se_cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);

_OUT_:
    return ret;


}

/*
 * @fn int __do_rrti_step2 (IN LIO_SE_CMD *se_cmd, IN void *param, IN OUT ERR_REASON_INDEX *err)
 *
 * @brief step2 function to do RECEIVE ROD TOKEN INFORMATION command for block i/o or file i/o
 * @note
 * @param[in] se_cmd
 * @param[in] param
 * @param[in,out] err
 * @retval
 */
static int __do_rrti_step2(
	IN LIO_SE_CMD *se_cmd, 
	IN void *param,
	IN OUT ERR_REASON_INDEX *err
	)
{


	TPC_OBJ *obj = NULL, *tmp_obj = NULL;
	LIO_SE_PORTAL_GROUP *se_tpg = NULL;
	LIO_SE_DEVICE *se_dev = NULL;
	unsigned long flags = 0;
	u32 cur_list_id = 0, alloc_len_in_cdb = 0;
	ROD_TOKEN_INFO_PARAM *p = NULL;
	int ret = 1;
	u8 *cdb = NULL;
	bool attach = 0;

	/**/
	if (!se_cmd ||!param){
		*err = ERR_INVALID_CDB_FIELD;
		goto _OUT_;
	}

	if (!se_cmd->is_tpc || !se_cmd->se_lun){
		*err = ERR_INVALID_CDB_FIELD;
		goto _OUT_;
	}

	p = (ROD_TOKEN_INFO_PARAM *)param;
	*err = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	se_dev = se_cmd->se_lun->lun_se_dev;
	cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;
	cur_list_id = get_unaligned_be32(&cdb[2]);
	alloc_len_in_cdb = get_unaligned_be32(&cdb[10]);

	/* FIXED ME !! */
	if (!se_cmd->se_lun->lun_sep)
		goto _OUT_;

	if (!se_cmd->se_lun->lun_sep->sep_tpg)
		goto _OUT_;

	/**/
	se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;

	spin_lock_bh(&se_tpg->tpc_obj_list_lock);
	list_for_each_entry(obj, &se_tpg->tpc_obj_list, o_node){

		/* To indicate we are using obj now */
		__tpc_obj_ref_count_lock_inc(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

		if (__tpc_is_lun_receive_stop(se_cmd)
		|| (__is_se_tpg_actived(se_tpg))
		){
			if (se_cmd->cur_se_task->task_flags & TF_REQUEST_STOP)
				transport_complete_task(se_cmd->cur_se_task, 0);

			spin_lock_bh(&se_tpg->tpc_obj_list_lock);	   
			__tpc_obj_ref_count_lock_dec(obj);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _OUT_;
		}

		/* NOTE: 
		 * - the o_data_type of obj for WRITE USING TOKEN is DATA_TYPE_NONE 
		 * - the o_data_type of obj for POPULATE TOKEN is O_DATA_TYPE_ROD
		 */
		if (!((obj->list_id == cur_list_id) 
		&&  (!(IS_HOLD_DATA_TYPE(obj->o_data_type)))
		&&  (!(__tpc_is_same_i_t_nexus_func1(obj, se_cmd))))
		)
			goto _EXIT_OBJ_CHECK_;

		if (alloc_len_in_cdb == 0){
			/* FIXED ME !! 
			 * SPC4R36, page 428
			 *
			 * b)
			 * if a RECEIVE ROD TOKEN INFORMATION command
			 * has been received on the same I_T nexus with
			 * a matching list id with the ALLOCATION LENGTH
			 * field set to zero
			 */
			pr_warn("warning: discard token "
				"(id:0x%x, op_sac:0x%x) "
				"when alloc_len_in_cdb is zero ... \n", 
				obj->list_id, obj->op_sac);

			spin_lock_bh(&se_tpg->tpc_obj_list_lock);
			__tpc_obj_ref_count_lock_dec(obj);
			__tpc_obj_node_del(obj);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			
			*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED;
			goto _OUT_;
		}

		/* Since found the obj already, here to check the status
		 * of obj and token before to prepare the parameter
		 * data for RECEIVE ROD TOKEN INFORMATION command
		 */
		spin_lock_bh(&obj->se_tpg->tpc_obj_list_lock);

		__tpc_token_ref_count_lock_inc(obj);
		ret = __tpc_is_token_invalid(obj, err);

		spin_unlock_bh(&obj->se_tpg->tpc_obj_list_lock);

		/* at this time, the ref count for obj and token had
		 * been increased already
		 */
		if (ret != -1){
			/* Now, the obj is alive but the token may be
			 * expired or deleted or cancelled. So we need
			 * to check token status again.
			 * If the token is still alive, to increase the
			 * ref count to notify we will use it.
			 */
			if (IS_ROD_TYPE(obj->o_data_type) && (ret == 0))
				attach = 1;
			else
				attach = 0;

			pr_debug("%s: obj(id:0x%x, op_sac:0x%x, "
				"type:0x%x) o_status:0x%x, "
				"token_status:0x%x, "
				"attach_token:0x%x in step 2\n", 
				__func__, obj->list_id, obj->op_sac, 
				obj->o_data_type, obj->o_status, 
				obj->o_token_status, attach);
		
			ret = __tpc_put_rrti_by_obj(obj, p, attach, 
					alloc_len_in_cdb);

			/* FIXED ME !! If hit error after to build rrti, then
			 * to report error */
			if (ret != 0){
				*err = ERR_INVALID_PARAMETER_LIST;
				ret = 1;
			}
			goto _OUT_2_;
		}

		/* Case for obj is NOT alive or token is NOT alive */
		pr_err("warning: %s - obj(id:0x%x)(or token) "
			"is NOT alive\n", __func__, obj->list_id);

		*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED;

_OUT_2_:
		spin_lock_bh(&obj->se_tpg->tpc_obj_list_lock);
		__tpc_token_ref_count_lock_dec(obj);
		__tpc_obj_ref_count_lock_dec(obj);
		spin_unlock_bh(&obj->se_tpg->tpc_obj_list_lock);
		goto _OUT_;

_EXIT_OBJ_CHECK_:
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
		__tpc_obj_ref_count_lock_dec(obj);
	}

	/* if not found anything ... */
	pr_err("warning: %s - not found list id(0x%x) in obj list\n", 
		__func__, cur_list_id);
	*err = ERR_INVALID_CDB_FIELD;

_OUT_1_:
	spin_unlock_bh(&se_tpg->tpc_obj_list_lock);


_OUT_:
	return ret;
}

/*
 * @fn int __do_receive_rod_token (IN LIO_SE_CMD *se_cmd)
 *
 * @brief main function to do RECEIVE ROD TOKEN INFORMATION command for block i/o or file i/o
 * @note
 * @param[in] se_cmd
 * @retval 0  - Success / 1 - Fail for other reason
 */
int __do_receive_rod_token(
    IN LIO_SE_CMD *se_cmd
    )
{
    u8 *cdb = NULL;
    ROD_TOKEN_INFO_PARAM *param = NULL;
    int ret = 1;
    ERR_REASON_INDEX err = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;


    /*
     * SPC4R36, page 428
     *
     * The copy manager shall discard the parameter data for the created ROD tokens:
     *
     * a) after all ROD tokens created by a specific copy operation have been
     *    transferred without errors to the application client
     * b) if a RECEIVE ROD TOKEN INFORMATION command has been received on the same
     *    I_T nexus with a matching list id with the ALLOCATION LENGTH field set to zero
     * c) if another a 3rd party command that originates a copy operation is
     *    received on the same I_T nexus and the list id matches the list id 
     *    associated with the ROD tokens
     * d) if the copy manager detects a LU reset conditionor I_T nexus loss condition or
     * e) if the copy manager requires the resources used to preserve the data
     *
     */
    cdb  = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;
    if (get_unaligned_be32(&cdb[10]) < __get_min_rod_token_info_param_len()){
        pr_err("error: alloc length:0x%x < min RRTI param len...\n",
            get_unaligned_be32(&cdb[10]));

        err = ERR_PARAMETER_LIST_LEN_ERROR;
        goto _EXIT_;
    }

    param = (ROD_TOKEN_INFO_PARAM *)transport_kmap_data_sg(se_cmd);
    if (!param){
        pr_err("%s: fail to call transport_kmap_data_sg()...\n", __func__);
        err = ERR_INVALID_PARAMETER_LIST;
        goto _EXIT_;
    }

    /* step1 :
     *
     * To check whether the command we want to monitor is still
     * in processing or not
     */
    ret = __do_rrti_step1(se_cmd, param, &err);
    if (ret == -1 ||  ret== 1){
        if (ret == 1)
            ret = 0;
        goto _EXIT_;
    }

    /* step2 : 
     *
     * Try to search the obj list again if not found anything in step1
     */
    ret = __do_rrti_step2(se_cmd, param, &err);

_EXIT_:
    if (param)
        transport_kunmap_data_sg(se_cmd);

    if (ret != 0)
        __set_err_reason(err, &se_cmd->scsi_sense_reason);

    return ret;

}

/*
 * @fn int __do_write_by_token (IN LIO_SE_CMD *se_cmd, IN void *token_src_obj, IN void *new_obj)
 *
 * @brief main function to do WRITE USING TOKEN command for block i/o or file i/o
 * @note
 * @param[in] se_cmd
 * @param[in] token_src_obj
 * @param[in] new_obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int __do_write_by_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *token_src_obj,
    IN void *new_obj
    )
{
#define DEFAULT_MEM_SIZE	(1024*1024)

	WRITE_BY_TOKEN_PARAM *param = NULL;
	BLK_DEV_RANGE_DESC *s = NULL;
	TPC_OBJ *n_obj = NULL, *s_obj = NULL;
	TPC_TRACK_DATA *t_data = NULL;
	sector_t d_nr_blks = 0, dest_lba = 0;
	u64 t_off_to_rod = 0;
	u64 w_nr_bytes = 0, real_w_bytes = 0, s_nr_bytes = 0, alloc_bytes;
	u16 desc_counts = 0, index = 0;
	int ret = 1;
	unsigned long timeout_jiffies = jiffies + msecs_to_jiffies(ROD_TIMEOUT);
	ERR_REASON_INDEX err = ERR_INVALID_PARAMETER_LIST;
	T_CMD_STATUS cur_status = T_CMD_COMPLETED_W_ERR;
	WBT_OBJ wbt_obj;
/* Jonathan Ho, 20131212, monitor ODX */
#ifdef SHOW_OFFLOAD_STATS
	unsigned long start_jiffies = 0;
	u64 tmp_Xpcmd;
#endif /* SHOW_OFFLOAD_STATS */

	/* Beware this !!!
	 *
	 * When code comes here, it means we got the src obj indicated by token 
	 * data already and ref count of src-obj and token had been increased by
	 * one. Howoever, we still need to check the status of obj and token
	 * before to use them.
	 */
	BUG_ON((token_src_obj == NULL));
	BUG_ON((new_obj == NULL));
	BUG_ON((se_cmd == NULL));

	n_obj = (TPC_OBJ *)new_obj;
	s_obj = (TPC_OBJ *)token_src_obj;
	t_data = (TPC_TRACK_DATA *)se_cmd->t_track_rec;

	if((param = (WRITE_BY_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd)) == NULL)
		goto _OUT_FREE_OBJ_;

	/* pre-allocate the mem */
	memset(&wbt_obj, 0, sizeof(WBT_OBJ));
	alloc_bytes = DEFAULT_MEM_SIZE;

	ret = __generic_alloc_sg_list(&alloc_bytes, &wbt_obj.sg_list, 
			&wbt_obj.sg_nents);

	wbt_obj.sg_total_bytes = alloc_bytes;
	if (ret != 0){
		if (ret == -ENOMEM)
			pr_err("[wrt] fail to alloc sg list\n");
		if (ret == -EINVAL)
			pr_err("[wrt] invalid arg during to alloc sg list\n");
		err = ERR_INSUFFICIENT_RESOURCES;
		goto _OUT_FREE_OBJ_;
	}


	/* The block dev range desc length shall be a multiple of 16 */
	s = (BLK_DEV_RANGE_DESC *)((u8*)param + sizeof(WRITE_BY_TOKEN_PARAM));
	desc_counts = __tpc_get_desc_counts(
		get_unaligned_be16(&param->blkdev_range_desc_len[0]));

	d_nr_blks = __tpc_get_total_nr_blks_by_desc(s, desc_counts);
	t_off_to_rod = (get_unaligned_be64(&param->off_into_rod[0]) << \
		n_obj->dev_bs_order);

	/* To check src obj's health status */
	spin_lock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	ret = __tpc_is_token_invalid(s_obj, &err);
	spin_unlock_bh(&s_obj->se_tpg->tpc_obj_list_lock);

	if (ret != 0){
		ret = 1;
		goto _OUT_FREE_OBJ_;
	}

	/* To start to process the command and indicate we will use the track data now */
	ret = 1;

	/* 2014/08/17, adamhsu, redmine 9007 */
	__tpc_td_ref_count_lock_inc(t_data);
	__tpc_update_t_cmd_status(t_data, T_CMD_IS_STARTING_IN_FG);

	/* to parse each block descriptor if possible */
	for (index = 0; index < desc_counts; index++){
		if (get_unaligned_be32(&s[index].nr_blks[0]) == 0)
			continue;

		dest_lba  = get_unaligned_be64(&s[index].lba[0]);
		w_nr_bytes = ((u64)get_unaligned_be32(&s[index].nr_blks[0]) << \
			n_obj->dev_bs_order);

/* Jonathan Ho, 20131212, monitor ODX */
#ifdef SHOW_OFFLOAD_STATS
		start_jiffies = jiffies;
		tmp_Xpcmd = w_nr_bytes;
#endif /* SHOW_OFFLOAD_STATS */

	        pr_debug("[wrt] (a) index:0x%x, dest_lba:0x%llx, "
        	    "w_nr_bytes:0x%llx, w-dev bs_order:0x%x\n", index,
	            (unsigned long long)dest_lba, 
	            (unsigned long long)w_nr_bytes, n_obj->dev_bs_order);

_DO_AGAIN_:
		/**/
		if (__tpc_is_lun_receive_stop(se_cmd)
		|| (__tpc_is_se_tpg_actived(se_cmd) == 1))
		{
			if (se_cmd->cur_se_task->task_flags & TF_REQUEST_STOP)
				transport_complete_task(se_cmd->cur_se_task, 0);

			/* 2014/08/17, adamhsu, redmine 9007 */
			__tpc_td_ref_count_lock_dec(
				(TPC_TRACK_DATA *)se_cmd->t_track_rec);

			__generic_free_sg_list(wbt_obj.sg_list, wbt_obj.sg_nents);
			err = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			goto _OUT_FREE_OBJ_;
		}

		/* FIXED ME !!
		 *
		 * We do this cause of the total number of blocks in single
		 * descriptor passing by host may be too large. To avoid the 
		 * code will create many bios at a time, to limit the number of
		 * blocks to be processed by OPTIMAL_BLK_ROD_LEN_GRANULARITY.
		 */
		real_w_bytes  = min_t(u64, w_nr_bytes, 
			OPTIMAL_TRANSFER_SIZE_IN_BYTES);

		/* get some information for src obj first */
		spin_lock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
		s_nr_bytes  = __tpc_get_nr_bytes_by_s_obj(s_obj, 1);
		spin_unlock_bh(&s_obj->se_tpg->tpc_obj_list_lock);


		if (t_off_to_rod >= s_nr_bytes)
			goto _OUT_;    

		pr_debug("[wrt] real_w_bytes:0x%llx, s_nr_bytes:0x%llx, "
			"t_off_to_rod:0x%llx\n", (unsigned long long)real_w_bytes, 
			(unsigned long long)s_nr_bytes, 
			(unsigned long long)t_off_to_rod);

		real_w_bytes = min_t(u64, real_w_bytes, 
			(s_nr_bytes - t_off_to_rod));

		/* main while loop() to process read and write */
		wbt_obj.s_obj = s_obj;
		wbt_obj.d_obj = n_obj;
		wbt_obj.t_data = t_data;
		wbt_obj.timeout = timeout_jiffies;
		wbt_obj.s_off_rod = t_off_to_rod;
		wbt_obj.d_lba = dest_lba;
		wbt_obj.data_bytes = real_w_bytes;
		wbt_obj.err = MAX_ERR_REASON_INDEX;

		if(__core_do_wt(&wbt_obj) != 0){
			if (wbt_obj.err != MAX_ERR_REASON_INDEX)
				err = wbt_obj.err;
			goto _OUT_;
		}

		/* update byte offset to the src ROD */
		t_off_to_rod += real_w_bytes;
		dest_lba += (sector_t)(real_w_bytes >> n_obj->dev_bs_order);
		w_nr_bytes -= real_w_bytes;

		DBG_ROD_PRINT("[wrt] (b) w_nr_bytes:0x%llx\n", 
			(unsigned long long)w_nr_bytes);

		if (w_nr_bytes)
			goto _DO_AGAIN_;

/* Jonathan Ho, 20131212, monitor ODX */
#ifdef SHOW_OFFLOAD_STATS
		Xpcmd = tmp_Xpcmd >> 10; /* bytes to KB */
		Tpcmd = jiffies_to_msecs(jiffies - start_jiffies);
		cmd_done++;
#endif /* SHOW_OFFLOAD_STATS */
	}

/**/
_OUT_:
	__generic_free_sg_list(wbt_obj.sg_list, wbt_obj.sg_nents);

	spin_lock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	__tpc_update_br_status(s_obj);
	spin_unlock_bh(&s_obj->se_tpg->tpc_obj_list_lock);


	/* please refer the sbc3r31, page 88 */
	if (d_nr_blks == t_data->t_transfer_count){
		cur_status = T_CMD_COMPLETED_WO_ERR;
		n_obj->cp_op_status = OP_COMPLETED_WO_ERR;
	}
	else{
		if (param->immed)
			n_obj->completion_status = SAM_STAT_CHECK_CONDITION;

		if (wbt_obj.err == ERR_NO_SPACE_WRITE_PROTECT) {
			/* treat it as copy-error if hit no sapce event */
			n_obj->cp_op_status = OP_COMPLETED_W_ERR;
			err = ERR_NO_SPACE_WRITE_PROTECT;
		} else {

			n_obj->cp_op_status = OP_COMPLETED_WO_ERR_WITH_ROD_TOKEN_USAGE;

			if (t_data->t_transfer_count)
				err = ERR_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET;
			else
				err = ERR_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET;
		}

	}

	__do_get_subsystem_dev_type(se_cmd->se_dev, &n_obj->backend_type);
	__tpc_get_isid_by_se_td(se_cmd, n_obj->isid);
	__tpc_get_tiqn_and_pg_tag_by_se_td(se_cmd, n_obj->tiqn, &n_obj->pg_tag);
	__tpc_set_obj_completion_status(n_obj);
	__tpc_set_obj_op_counter(n_obj);

	/* The WRITE USING TOKEN obj won't keep anything here */
	n_obj->o_data_type    = O_DATA_TYPE_NONE;
	n_obj->list_id        = se_cmd->t_list_id;
	n_obj->op_sac         = se_cmd->t_op_sac;
	n_obj->segs_processed = t_data->t_segs_processed = 0;
	n_obj->transfer_count = t_data->t_transfer_count;

	ret = 0;
	if (n_obj->cp_op_status != OP_COMPLETED_WO_ERR){
		__tpc_build_obj_sense_data(n_obj, err, 0, 0);
		cur_status = T_CMD_COMPLETED_W_ERR;
		ret = 1;
	}


	/* To release the ref count */


	/* 2014/08/17, adamhsu, redmine 9007 */
	spin_lock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	__tpc_token_ref_count_lock_dec(s_obj);
	__tpc_obj_ref_count_lock_dec(s_obj);
	spin_unlock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	
	/* don't need to lock to protect anything for new obj */
	__tpc_update_obj_status_lock(n_obj, O_STS_ALIVE);
	__tpc_update_obj_token_status_lock(n_obj, 
        	    O_TOKEN_STS_NOT_ALLOCATED_AND_NOT_ALIVE);

	__tpc_setup_obj_timer(n_obj);

	spin_lock_bh(&n_obj->se_tpg->tpc_obj_list_lock);
	__tpc_obj_node_add(n_obj);
	spin_unlock_bh(&n_obj->se_tpg->tpc_obj_list_lock);

	__tpc_update_t_cmd_status(t_data, cur_status);

	/* 2014/08/17, adamhsu, redmine 9007 */
	__tpc_td_ref_count_lock_dec(
		(TPC_TRACK_DATA *)se_cmd->t_track_rec);

_OUT_1_:
	if (param)
		transport_kunmap_data_sg(se_cmd);

	if (ret != 0)
		__set_err_reason(err, &se_cmd->scsi_sense_reason);

	return ret;

_OUT_FREE_OBJ_:
	if (n_obj->o_token_data)
		kfree(n_obj->o_token_data);
	kfree(n_obj);

_OUT_COMPLETE_REF_:
	/* To release the ref count of token data and obj since we found
	 * the them already from __tpc_get_obj_by_token_data() */



	/* 2014/08/17, adamhsu, redmine 9007 */
	spin_lock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	__tpc_token_ref_count_lock_dec(s_obj);
	__tpc_obj_ref_count_lock_dec(s_obj);
	spin_unlock_bh(&s_obj->se_tpg->tpc_obj_list_lock);
	goto _OUT_1_;

}


/*
 * @fn int __do_populate_token (IN LIO_SE_CMD *se_cmd, IN void *obj)
 *
 * @brief main function to do POPULATE TOKEN command for block i/o or file i/o
 * @note
 * @param[in] se_cmd
 * @param[in] obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int __do_populate_token(
	IN LIO_SE_CMD *se_cmd,
	IN void *obj
	)
{
 	POPULATE_TOKEN_PARAM *param = NULL;
	TPC_TRACK_DATA *td = NULL;
	BLK_DEV_RANGE_DESC *start = NULL;
	TPC_OBJ *n_obj = NULL;
	BLK_RANGE_DATA *br = NULL;
	sector_t lba = 0;
	u32 alloc_nr_blks = 0, tmp_nr_blks = 0, c_nr_blks = 0;
	u16 desc_counts = 0, index = 0;
	int ret = 1;
	ERR_REASON_INDEX err = ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	T_CMD_STATUS cur_status = T_CMD_COMPLETED_W_ERR;
	LIST_HEAD(br_list);

	/* FIXED ME !!
	 *
	 * In this function, we do NOT read any data. We do it while
	 * the write is working.
	 */
	td = (TPC_TRACK_DATA *)se_cmd->t_track_rec;
	n_obj = (TPC_OBJ *)obj;
	if ((param = (POPULATE_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd)) == NULL){
		if (n_obj->o_token_data)
			kfree(n_obj->o_token_data);
		kfree(n_obj);
		err = ERR_INVALID_PARAMETER_LIST;
		goto _OUT_1_;
	}

	/* 2014/08/17, adamhsu, redmine 9007 */
	__tpc_td_ref_count_lock_inc(td);
	__tpc_update_t_cmd_status(td, T_CMD_IS_STARTING_IN_FG);

	start = (BLK_DEV_RANGE_DESC *)((u8*)param + sizeof(POPULATE_TOKEN_PARAM));
	desc_counts = __tpc_get_desc_counts(get_unaligned_be16(&param->blkdev_range_desc_len[0]));
	c_nr_blks = (OPTIMAL_TRANSFER_SIZE_IN_BYTES >> n_obj->dev_bs_order);

	/**/
	for (index = 0; index < desc_counts; index++){

		if (__tpc_is_lun_receive_stop(se_cmd)
		|| (__tpc_is_se_tpg_actived(se_cmd) == 1)
		)
		{
			if (se_cmd->cur_se_task->task_flags & TF_REQUEST_STOP)
				transport_complete_task(se_cmd->cur_se_task, 0);
			__tpc_free_obj_node_data(&br_list);
			INIT_LIST_HEAD(&br_list);
			goto _OUT_;
		}

		/* (SBC3R31, p130)
		 *
		 * If the NUMBER_OF_LOGICAL_BLOCKS field is set to zero, then
		 * the copy manager should perform no operation for this block
		 * device range descriptor. This condition shall not be
		 * considered an error.
		 */
		if (get_unaligned_be32(&start[index].nr_blks[0]) == 0)
			continue;

		/* To split the blocks to small chunks */
		lba = get_unaligned_be64(&start[index].lba[0]);
		alloc_nr_blks = get_unaligned_be32(&start[index].nr_blks[0]);

		while (alloc_nr_blks){
			tmp_nr_blks = min_t(u32, alloc_nr_blks, c_nr_blks);
			if ((br = __create_blk_range()) == NULL){
				pr_err("[pt] fail to create pt blk_range\n");
				err = ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN;
				__tpc_free_obj_node_data(&br_list);
				INIT_LIST_HEAD(&br_list);
				goto _OUT_;
			}

			/* To record data for each block descriptor */
			br->lba = lba;
			br->nr_blks = tmp_nr_blks;

			lba += (sector_t)tmp_nr_blks;
			alloc_nr_blks -= tmp_nr_blks;

			__tpc_update_t_cmd_transfer_count(td, (sector_t)br->nr_blks);
			list_add_tail(&br->b_range_data_node, &br_list);
		}
	}

	/**/ 
	cur_status           = T_CMD_COMPLETED_WO_ERR;
	n_obj->cp_op_status  = OP_COMPLETED_WO_ERR;
	list_splice_tail_init(&br_list, &n_obj->o_data_list);

	__tpc_get_isid_by_se_td(se_cmd, n_obj->isid);
	__tpc_get_tiqn_and_pg_tag_by_se_td(se_cmd, n_obj->tiqn, &n_obj->pg_tag);
	__tpc_set_obj_completion_status(n_obj);
	__tpc_set_obj_op_counter(n_obj);
	__do_get_subsystem_dev_type(se_cmd->se_dev, &n_obj->backend_type);

	n_obj->create_time     = jiffies;
	n_obj->list_id         = se_cmd->t_list_id;
	n_obj->op_sac          = se_cmd->t_op_sac;
	n_obj->segs_processed  = td->t_segs_processed = 0;
	n_obj->o_data_type     = O_DATA_TYPE_ROD;
	n_obj->transfer_count  = __tpc_get_t_cmd_transfer_count(td);

	__tpc_build_512b_token_data(se_cmd, n_obj);

	/* (1) To change the status of obj and obj-otken
	 * (2) To insert the obj node and fire the timer
	 *
	 * here don't need to use lock to protect everything for new obj
	 */
	__tpc_update_obj_status_lock(n_obj, O_STS_ALIVE);
	__tpc_update_obj_token_status_lock(n_obj, O_TOKEN_STS_ALIVE);

	__tpc_setup_token_timer(n_obj, 
		get_unaligned_be32(&param->inactivity_timeout[0]));

	__tpc_setup_obj_timer(n_obj);

	spin_lock_bh(&n_obj->se_tpg->tpc_obj_list_lock);
	__tpc_obj_node_add(n_obj);
	spin_unlock_bh(&n_obj->se_tpg->tpc_obj_list_lock);


	/* Always report the command was complete */
	ret = 0;

_OUT_:
	__tpc_update_t_cmd_status(td, cur_status);

	/* 2014/08/17, adamhsu, redmine 9007 */
	__tpc_td_ref_count_lock_dec(
		(TPC_TRACK_DATA *)se_cmd->t_track_rec);

_OUT_1_:
	if (param)
		transport_kunmap_data_sg(se_cmd);

	if (ret != 0)
		__set_err_reason(err, &se_cmd->scsi_sense_reason);

	return ret;
}


/*
 * @fn int tpc_receive_all_rod_token(IN LIO_SE_TASK *task)
 * @brief
 *
 * @note
 * @param[in] se_task
 * @retval 0  - Success / 1 - Fail for other reason
 */
int tpc_receive_all_rod_token(
    IN LIO_SE_TASK *se_task
   )
{
    int ret = 1;

    pr_debug("%s\n", __func__);
    return ret;
}

/*
 * @fn int tpc_write_by_token(IN LIO_SE_TASK *se_task)
 *
 * @brief main wrapper function for WRITE USING TOKEN command
 * @note
 * @param[in] se_task
 * @retval 0  - Success / 1 - Fail for other reason
 */
int tpc_write_by_token(
	IN LIO_SE_TASK *se_task
	)
{
	LIO_SE_CMD *se_cmd = NULL;
	WRITE_BY_TOKEN_PARAM *param = NULL;
	TPC_OBJ *new_obj = NULL, *token_src_obj = NULL;
	ROD_TOKEN_512B *token_512b = NULL;
	u8 *cdb = NULL; 
	int ret = 1;
	ERR_REASON_INDEX err = MAX_ERR_REASON_INDEX;

	se_cmd = se_task->task_se_cmd;

	/* this is workaround and need to removed in the future */
	if(__tpc_is_tpg_v_lun0(se_cmd))
		goto _EXIT_;

	pr_debug("%s - id:0x%x, op_sac:0x%x\n", __FUNCTION__, 
		se_cmd->t_list_id, se_cmd->t_op_sac);

	cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;

	/* if length is zero, means no data shall be sent, no treat it as error */
	if(get_unaligned_be32(&cdb[10]) == 0){
		pr_debug("%s: no any data to be transferred !!\n", __FUNCTION__);
		ret = 0;
		goto _EXIT_;
	}

	ret = se_cmd->se_dev->transport->do_chk_before_wrt(se_cmd, (int *)&err);
	if (ret == -1 ||  ret == 1){
		/* If success but not need to keep to execute cmd, 
		 * then to exit this function */
		if (ret == 1)
			ret = 0;
		else
			__set_err_reason(err, &se_cmd->scsi_sense_reason);

		goto _EXIT_;
	}


	/* reset the ret value */
	ret = 1;

	/* Check whether there is any reocrd in tpc_obj_list which was matched with
	 * list id in current passing cmd. */
	if(__tpc_check_duplicated_obj_data(se_cmd) == -1){
		__set_err_reason(ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&se_cmd->scsi_sense_reason
			);
		goto _EXIT_;
	}

	/* Create new obj again */
	new_obj = __tpc_do_alloc_obj(se_cmd);
	if (!new_obj){
		__set_err_reason(ERR_INSUFFICIENT_RESOURCES, 
			&se_cmd->scsi_sense_reason);
		goto _EXIT_;
	}

	param = (WRITE_BY_TOKEN_PARAM *)transport_kmap_data_sg(se_cmd);
	if (!param){
		__set_err_reason(ERR_INVALID_PARAMETER_LIST, 
			&se_cmd->scsi_sense_reason);
		kfree(new_obj);
		goto _EXIT_;
	}

	/* We don't need the src obj and src token if command will do 
	 * WRITE USING ZERO ROD TOKEN command
	 */
	token_512b = (ROD_TOKEN_512B *)param->rod_token;
	if (get_unaligned_be32(&token_512b->gen_data.type[0]) == ROD_TYPE_BLK_DEV_ZERO){
		transport_kunmap_data_sg(se_cmd);

		ret = se_cmd->se_dev->transport->do_wzrt(se_cmd, (void *)new_obj);
		goto _EXIT_;
	}


	/* Try to find the suitable obj we want from upper passing token data */
	token_src_obj = __tpc_get_obj_by_token_data( 
				token_512b->gen_data.cm_rod_token_id,
				&token_512b->gen_data.cr_lu_desc[8],
				&token_512b->target_dev_desc[0],
				&err);

	transport_kunmap_data_sg(se_cmd);

	/* FIXED ME */
	if (!token_src_obj){
		pr_err("warning: not found matched obj by token data. "
			"PassingCmd(id:0x%x, op sac:0x%x)\n", 
			se_cmd->t_list_id, se_cmd->t_op_sac);

		kfree(new_obj);
		__set_err_reason(err, &se_cmd->scsi_sense_reason);
		goto _EXIT_;
	}else{
		/* We found token src obj but its token may not be alive ... */
		if (err != MAX_ERR_REASON_INDEX){

			pr_err("warning: found matched obj by token data. "
				"but token health status is NG. "
				"PassingCmd(id:0x%x, op sac:0x%x), "
				"token status:0x%x\n", 
				se_cmd->t_list_id, se_cmd->t_op_sac, err);
			goto _OBJ_FAIL_;
		}

		/* Actually, NOT find any standard document indicates the token
		 * used by WRITE USING TOKEN command can work across different
		 * i_t nexus (session) ...
		 */
#if 0
		/* Found token obj, but need to check the i_t nexus of
		 * obj whether it equals to passing commannd or not
		 */
		if (__tpc_is_same_i_t_nexus_func1(token_src_obj, se_cmd) != 0){
			pr_debug("warning: found matched obj by token data. but"
				" i_t nexus is not same between obj and cmd\n");
			/* TODO: need to find suitable sense code */
			err = ERR_UNREACHABLE_COPY_TARGET;
			goto _OBJ_FAIL_;
		}
#endif
	}

	/* Before to do write by token, let's investigate some conditions again */
	if(__do_chk_wt_rod_off_before_wt(se_cmd, (void *)token_src_obj) != 0){
		spin_lock_bh(&token_src_obj->se_tpg->tpc_obj_list_lock);
		__tpc_token_ref_count_lock_dec(token_src_obj);
		__tpc_obj_ref_count_lock_dec(token_src_obj);
		spin_unlock_bh(&token_src_obj->se_tpg->tpc_obj_list_lock);
		goto _EXIT_;
	}

	pr_debug("found matched obj(id:0x%x) by token data\n", 
		token_src_obj->list_id);

	/* It is safe to return since the obj / token ref count had been
	 * processed in do_wrt() already
	 */
	ret = se_cmd->se_dev->transport->do_wrt(se_cmd, 
			(void *)token_src_obj, (void *)new_obj);

_EXIT_:
	if (ret == 0){
		se_task->task_scsi_status = GOOD;
		transport_complete_task(se_task, 1);
	}
	return ret;


_OBJ_FAIL_:
	kfree(new_obj);
	__set_err_reason(err, &se_cmd->scsi_sense_reason);

	/* To release the ref count for obj and token data since
	 * this is error condition 
	 */

	/* 2014/08/17, adamhsu, redmine 9007 */
	spin_lock_bh(&token_src_obj->se_tpg->tpc_obj_list_lock);
	__tpc_token_ref_count_lock_dec(token_src_obj);
	__tpc_obj_ref_count_lock_dec(token_src_obj);
	spin_unlock_bh(&token_src_obj->se_tpg->tpc_obj_list_lock);
	goto _EXIT_;

}

/*
 * @fn int tpc_populate_token(IN LIO_SE_TASK *task)
 *
 * @brief main wrapper function for POPULATE TOKEN command
 * @note
 * @param[in] se_task
 * @retval 0  - Success / 1 - Fail for other reason
 */
int tpc_populate_token(
	IN LIO_SE_TASK *se_task
	)
{
	LIO_SE_CMD *se_cmd = NULL;
	TPC_OBJ *obj = NULL;
	u8 *cdb = NULL;
	ERR_REASON_INDEX err = MAX_ERR_REASON_INDEX;
	int ret = 1;

	se_cmd = se_task->task_se_cmd;

	/* this is workaround and need to removed in the future */
	if(__tpc_is_tpg_v_lun0(se_cmd))
		goto _EXIT_;

	DBG_ROD_PRINT("%s - id:0x%x, op_sac:0x%x\n", __func__, se_cmd->t_list_id, se_cmd->t_op_sac);
	cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;

	/* if length is zero, means no data shall be sent, no treat it as error */
	if(get_unaligned_be32(&cdb[10]) == 0){
		DBG_ROD_PRINT("%s: no any data to be transferred !!\n", __func__);
		ret = 0;
		goto _EXIT_;
	}

	/* To check the parameter data */


	ret = se_cmd->se_dev->transport->do_chk_before_pt(se_cmd, (int *)&err);
	if (ret == -1 || ret == 1){
		/* If success but not need to keep to execute cmd, 
		 * then to exit this function */
		if (ret == 1)
			ret = 0;
		else
			__set_err_reason(err, &se_cmd->scsi_sense_reason);

		goto _EXIT_;
	}

	/* reset the ret value */
	ret = 1;

	/*
	 * Check whether there is any reocrd in tpc_obj_list which was matched with
	 * list id in current passing cmd.
	 *
	 * c) If another a 3rd party command that originates a copy operation is
	 *    received on the same I_T nexus and the list id matches the list id
	 *    associated with the ROD token,then the ROD token shall be discard.
	 */
	if(__tpc_check_duplicated_obj_data(se_cmd) == -1){
		__set_err_reason(
			ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE, 
			&se_cmd->scsi_sense_reason);
		goto _EXIT_;
	}

	/* Create new obj again */
	obj = __tpc_do_alloc_obj(se_cmd);
	if (!obj){
		__set_err_reason(
			ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD, 
			&se_cmd->scsi_sense_reason
		);
		goto _EXIT_;
	}

	/* To allocate the rod token data */
	if (__tpc_do_alloc_token_data(
		se_cmd, obj, 
		ROD_TYPE_PIT_COPY_D4, ROD_TOKEN_MIN_SIZE, &err) != 0)
	{
		__set_err_reason(err, &se_cmd->scsi_sense_reason);

		if (obj->o_token_data)
			kfree(obj->o_token_data);
		kfree(obj);
		goto _EXIT_;
	}

	/* If any error happens during to do POPULATE TOKEN, the resource allocated
	 * before will be free in the do_populate_token()
	 */
	ret = se_cmd->se_dev->transport->do_pt(se_cmd, (void *)obj);

_EXIT_:
	if (ret == 0){
		se_task->task_scsi_status = GOOD;
		transport_complete_task(se_task, 1);
	}
	return ret;


}

/*
 * @fn int tpc_receive_rod_token_info(IN LIO_SE_TASK *task)
 * @brief main wrapper function for RECEIVE ROD TOKEN INFORMATION command
 *
 * @note
 * @param[in] se_task
 * @retval 0  - Success / 1 - Fail for other reason
 */
int tpc_receive_rod_token_info(
  IN LIO_SE_TASK *se_task
  )
{
    LIO_SE_CMD *se_cmd = NULL;
    int ret = 1;

    /**/
    se_cmd = se_task->task_se_cmd;
    if(__tpc_is_tpg_v_lun0(se_cmd))
        goto _EXIT_;

    DBG_ROD_PRINT("%s - id:0x%x, op_sac:0x%x\n", __func__, 
                            se_cmd->t_list_id, se_cmd->t_op_sac);

    ret = se_cmd->se_dev->transport->do_receive_rt(se_cmd);

_EXIT_:
    if (ret == 0){
        se_task->task_scsi_status = GOOD;
        transport_complete_task(se_task, 1);
    }

    return ret;

}



/*************************************************** 
 *
 * subsystem api of ROD for block backend device 
 *
 ***************************************************/

/*
 * @fn int iblock_before_write_by_token(IN LIO_SE_CMD *se_cmd)
 * @brief function to do pre-conditions checking for WRITE USING TOKEN of iblock i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in,out] err
 * @retval 0  - Success / 1 - Fail for other reason
 */
int iblock_before_write_by_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err
	)
{
	return __do_chk_before_write_by_token(se_cmd, err);
}
EXPORT_SYMBOL(iblock_before_write_by_token);


/*
 * @fn int iblock_before_populate_token(IN LIO_SE_CMD *se_cmd)
 * @brief function to do pre-conditions checking for POPULATE TOKEN of iblock i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in,out] err
 * @retval 0  - Success / 1 - Fail for other reason
 */
int iblock_before_populate_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err	
	)
{
	return __do_chk_before_populate_token(se_cmd, err);
}
EXPORT_SYMBOL(iblock_before_populate_token);

/*
 * @fn int iblock_before_populate_token(IN LIO_SE_CMD *se_cmd, IN void *obj)
 * @brief function to do POPULATE TOKEN of iblock i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in] obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int iblock_do_populate_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *obj
    )
{
    return __do_populate_token(se_cmd, obj);
}
EXPORT_SYMBOL(iblock_do_populate_token);


/*
 * @fn int iblock_do_write_by_token(IN LIO_SE_CMD *se_cmd, IN void *token_src_obj, IN void *new_obj)
 * @brief function to do WRITE USING TOKEN of iblock i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in] obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int iblock_do_write_by_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *token_src_obj,
    IN void *new_obj
    )
{
    return __do_write_by_token(se_cmd, token_src_obj, new_obj);
}
EXPORT_SYMBOL(iblock_do_write_by_token);


/*
 * @fn int iblock_do_write_by_zero_rod_token(IN LIO_SE_CMD *se_cmd, IN void *new_obj)
 * @brief function to do WRITE ZERO ROD TOKEN of iblock i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in] new_obj
 * @retval 0  - Success / 1 - Fail for other reason
 */ 
int iblock_do_write_by_zero_rod_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *new_obj
    )
{
    return __do_wzrt(se_cmd, new_obj);
}
EXPORT_SYMBOL(iblock_do_write_by_zero_rod_token);

/*
 * @fn int iblock_receive_rod_token(IN LIO_SE_CMD *se_cmd)
 * @brief function to do RECEIVE ROD TOKEN INFORMATION of iblock i/o
 *
 * @note
 * @param[in] se_cmd
 * @retval 0  - Success / 1 - Fail for other reason
 */ 
int iblock_receive_rod_token(
    IN LIO_SE_CMD *se_cmd
    )
{
    return __do_receive_rod_token(se_cmd);
}
EXPORT_SYMBOL(iblock_receive_rod_token);


/*************************************************** 
 *
 * subsystem api of ROD for file backend device 
 *
 ***************************************************/



/*
 * @fn int fd_before_write_by_token(IN LIO_SE_CMD *se_cmd)
 * @brief function to do pre-conditions checking for WRITE USING TOKEN of file i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in,out] err
 * @retval 0  - Success / 1 - Fail for other reason
 */ 
int fd_before_write_by_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err	
	)
{
	return __do_chk_before_write_by_token(se_cmd, err);
}
EXPORT_SYMBOL(fd_before_write_by_token);


/*
 * @fn int fd_before_populate_token(IN LIO_SE_CMD *se_cmd)
 * @brief function to do pre-conditions checking for POPULATE TOKEN of file i/o
 *
 * @note
 * @param[in] se_cmd
 * @param[in,out] err
 * @retval 0  - Success / 1 - Fail for other reason
 */ 
int fd_before_populate_token(
	IN LIO_SE_CMD *se_cmd,
	IN OUT int *err	
	)
{
	return __do_chk_before_populate_token(se_cmd, err);
}
EXPORT_SYMBOL(fd_before_populate_token);


/*
 * @fn int fd_receive_rod_token(IN LIO_SE_CMD *se_cmd)
 * @brief function to do RECEIVE ROD TOKEN INFORMATION of file i/o
 *
 * @note
 * @param[in] se_cmd
 * @retval 0  - Success / 1 - Fail for other reason
 */ 
int fd_receive_rod_token(IN LIO_SE_CMD *se_cmd)
{
    return __do_receive_rod_token(se_cmd);
}
EXPORT_SYMBOL(fd_receive_rod_token);


/*
 * @fn int fd_do_populate_token(IN LIO_SE_CMD *se_cmd, IN void *obj)
 * @brief function to do POPULATE TOKEN command of file i/o
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int fd_do_populate_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *obj
    )
{
    return __do_populate_token(se_cmd, obj);
}
EXPORT_SYMBOL(fd_do_populate_token);


/*
 * @fn int fd_do_write_by_token(IN LIO_SE_CMD *se_cmd, IN void *token_src_obj, IN void *new_obj)
 * @brief function to do WRITE USING TOKEN command of file i/o
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] token_src_obj
 * @param[in] new_obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int fd_do_write_by_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *token_src_obj,
    IN void *new_obj
    )
{
    return __do_write_by_token(se_cmd, token_src_obj, new_obj);
}
EXPORT_SYMBOL(fd_do_write_by_token);

/*
 * @fn int fd_do_write_by_zero_rod_token(IN LIO_SE_CMD *se_cmd, IN void *new_obj)
 * @brief function to do WRITE ZERO ROD TOKEN of file i/o
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] new_obj
 * @retval 0  - Success / 1 - Fail for other reason
 */
int fd_do_write_by_zero_rod_token(
    IN LIO_SE_CMD *se_cmd,
    IN void *new_obj
    )
{
    return __do_wzrt(se_cmd, new_obj);
}
EXPORT_SYMBOL(fd_do_write_by_zero_rod_token);



