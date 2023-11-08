/**
 * @file 	tpc_helper.c
 * @brief	This file contains the 3rd-party copy command helper code
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <scsi/scsi.h>
#include "vaai_comp_opt.h"
#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include "target_core_ua.h"
#include "target_core_iblock.h"
#include "target_core_file.h"

#include "vaai_target_struc.h"
#include "target_general.h"
#include "tpc_helper.h"

/**/
static void __tpc_track_node_lock_add(
    IN LIO_SE_CMD *se_cmd
    );

static void __tpc_track_node_lock_del(
    IN LIO_SE_CMD *se_cmd
    );

static void __tpc_track_node_add(
    IN LIO_SE_CMD *se_cmd,
    IN LIO_SE_PORTAL_GROUP *se_tpg
    );

static void __tpc_track_node_del(
    IN LIO_SE_CMD *se_cmd,
    IN LIO_SE_PORTAL_GROUP *se_tpg
    );

static int __tpc_get_generic_wdir_cmd_data(
    IN LIO_SE_CMD *se_cmd,
    IN OUT sector_t *lba,
    IN OUT u32 *range
    );

static int __tpc_get_wdir_blk_desc_format_cmd_data(
    IN LIO_SE_CMD *se_cmd,
    IN OUT u8 **start,
    IN OUT u16 *count,
    IN OUT sector_t *nr_blks
    );

static int __tpc_find_conflict_lba_in_br_list(
    IN struct list_head *br_list,
    IN sector_t lba,
    IN sector_t range
    );

static TPC_TRACK_DATA *__tpc_alloc_track_data(IN void);

static int __tpc_verify_target_dev_desc_hdr(IN u8 *p);

int __tpc_is_same_initiator_isid(
    IN char *s_isid,
    IN char *d_isid
    );


/**/
static ROD_TOKEN_512B *__alloc_rod_token(
    IN u32 token_type,
    IN u16 token_size
    );

static void __create_id_cscd_desc(
    IN LIO_SE_LUN *se_lun, 
    IN LIO_SE_DEVICE *se_dev, 
    IN TPC_OBJ *obj,
    IN ID_CSCD_DESC *data
    );

static void __create_cm_rod_token_id_in_rod_token(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u8 *buffer
    );

static void __create_byte64_95(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u8 *buffer
    );

static void __create_dev_type_specific_data(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u8 *dev_type_data
    );

static void __create_512b_rod_token_after_byte128(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj, 
    IN ROD_TOKEN_512B *token_512b
    );

static u32 __create_target_dev_desc(
	IN LIO_SE_CMD *se_cmd,
	IN TPC_OBJ *obj,
	IN u8 *buffer
	);

static void __create_ext_rod_token_data(
	IN LIO_SE_CMD *se_cmd,
	IN TPC_OBJ *obj,
	IN u8 *buffer,
	IN int codeset_bin
	);


static u32 __create_rod_token_type_and_cm_data(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN ROD_TOKEN *token
    );

static sector_t __get_total_nr_blks_by_s_obj(
    IN struct list_head *br_list,
    IN bool skip_truncate
    );

/**/
ROD_TYPE_TABLE  gRodTypeTable[] = {

    /* {<type>, <end table>, <token out>, <token in>, <internal token>, <prefenence>} */

#if (SUPPORT_NONZERO_ROD_TYPE == 1)
    { ROD_TYPE_PIT_COPY_D4  , 0, 1, 1, 0, 0 },  /* point in time copy - default */   
    { ROD_TYPE_BLK_DEV_ZERO , 0, 0, 1, 0, 0 },  /* block device zero ROD token */
#endif

    /* end of table */
    { 0xffffffff , 1,  0, 0, 0, 0 },
};


/** 
 * @brief  Table for supported CSCD IDs other than 0000h to 07fffh
 */
CSCD_ID_TABLE gSupportedCSCDIdsTable[] = {
#if (SUPPORT_CSCD_ID_F800 == 1)
    /*
     * The CSCD ID 0xf800 shall be supported in the Supported CSCD IDs 
     * third-party copy descriptor if
     *
     * (1) the ROD CSCD descriptor is supported and
     * (2) any ROD type descriptor contains:
     *     a) a non-zero value in the ROD type field and
     *     b) the TOKEN_IN bit is set to one
     */
    { 0xf800, FALSE },
#endif
    { 0x0000, TRUE  }, // end of table
};

/**/
static int __tpc_verify_target_dev_desc_hdr(
	IN u8 *p
	)
{

	/* codeset is UTF-8 + designator type is SCSI name string */
	if ((((p[0] & 0x0f) == 0x03) && ((p[1] & 0x0f) == 0x8))
	)
	{
		 // association is 0x0010_0000b + designator len != 0
		if (((p[1] & 0x30) == 0x20) && (p[3] != 0))
			return 0;
	}
	return 1;

}

void __tpc_update_br_status(
	IN TPC_OBJ *obj
	)
{
	BLK_RANGE_DATA *br = NULL;


	/* Since we will refer the data list in obj, please make sure the obj
	 * ref count had been increased by 1 somewhere
	 */
	spin_lock(&obj->o_data_lock);
	list_for_each_entry(br, &obj->o_data_list, b_range_data_node)
		br->curr_status = br->next_status;
	spin_unlock(&obj->o_data_lock);
	return;
}

BLK_RANGE_DATA * __tpc_get_rod_loc_by_rod_off(
	IN TPC_OBJ *s_obj,
	IN OUT u32 *bdr_off,
	IN u64 off_to_rod
	)
{
	BLK_RANGE_DATA *br = NULL;
	bool found = 0;
	u64 tmp_off = 0, bytes = 0;

	pr_debug("(a) off_to_rod (byte-based):0x%llx\n", (unsigned long long)off_to_rod);
	tmp_off = off_to_rod;

	/* Since we will refer the data list in obj, please make sure the obj
	 * ref count had been increased by 1 somewhere
	 */
	spin_lock(&s_obj->o_data_lock);

	list_for_each_entry(br, &s_obj->o_data_list, b_range_data_node){   

		bytes = ((u64)br->nr_blks << s_obj->dev_bs_order);
		if (off_to_rod >= bytes){     
			off_to_rod -= bytes;	    
			pr_debug("(b) off_to_rod:0x%llx, bytes:0x%llx, go next\n",
				(unsigned long long)off_to_rod, (unsigned long long)bytes);
			continue;
		}

		if (br->curr_status == R_STATUS_TRUNCATE_USED){
			pr_debug("(c) found br data but status is TRUNCATED_USED\n");
			break;
		}

		/* Found the block range we want */
		pr_debug("(d) found br data: br_d->lba:0x%llx, "
			"br_d->nr_blks:0x%x, br_d off:0x%llx, "
			"curr_status:0x%x, next_status:0x%x\n", 
			(unsigned long long)br->lba, br->nr_blks,
			(unsigned long long)(off_to_rod >> s_obj->dev_bs_order),
			br->curr_status, br->next_status);

		/* FIXED ME
		 *
		 * To set the curr_status to be R_STATUS_TRANSFER_USED and
		 * next_status to be R_STATUS_NOT_USED. Actually, we will reset
		 * curr_status from next_status after this command was finished
		 * completely */
		br->curr_status = R_STATUS_TRANSFER_USED;
		br->next_status = R_STATUS_NOT_USED;
		*bdr_off = (u32)(off_to_rod >> s_obj->dev_bs_order);
		found = 1;
		break;
	}
	spin_unlock(&s_obj->o_data_lock);

	if (!found){
		pr_err("%s - off to ROD:0x%llx, (a) nr_blks:0x%llx, "
			"(b) nr_blks:0x%llx, (c)dev_bs_order:0x%x\n", 
			__func__, (unsigned long long)tmp_off, 
			(unsigned long long)__tpc_get_nr_blks_by_s_obj(s_obj, 0),
			(unsigned long long)__tpc_get_nr_blks_by_s_obj(s_obj, 1), 
			s_obj->dev_bs_order);

		return NULL;
	}
	return br;
}

static void __create_cm_rod_token_id_in_rod_token(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u8 *buffer
    )
{
    /* SPC4R36, page 249
     *
     * The COPY MANAGER ROD TOKEN IDENTIFIER field contains a value that 
     * differentiates this ROD token from all other valid ROD token created by
     * and known to a specific copy manager. No two ROD tokens knwon to a specific
     * copy manager shall have the same value in this field.
     */

    /* FIXED ME !! */
    put_unaligned_be64((u64)obj->create_time, &buffer[0]);
    return;
}

static void __create_byte64_95(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u8 *buffer
    )
{
    return;
}

static void __create_dev_type_specific_data(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u8 *dev_type_data
    )
{
	memset(dev_type_data, 0, 32);

	dev_type_data[0] = 
		((se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size >> 24) & 0xff);

	dev_type_data[1] = 
		((se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size >> 16) & 0xff);

	dev_type_data[2] = 
		((se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size >> 8) & 0xff);

	dev_type_data[3] = 
		(se_cmd->se_dev->se_sub_dev->se_dev_attrib.block_size & 0xff);
	/*
	 * Set Thin Provisioning Enable bit following sbc3r22 in section
	 * READ CAPACITY (16) byte 14 if emulate_tpu or emulate_tpws is enabled.
	 */
	if (se_cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_tpu
	||  se_cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_tpws)
		dev_type_data[6] = 0x80;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
	if (se_cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_tpu 
	||  se_cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_tpws
	)
	{
		if(bReturnZeroWhenReadUnmapLba)
			dev_type_data[6] |= 0x40;
	}
#endif
	return;
}

static void __create_512b_rod_token_after_byte128(
	IN LIO_SE_CMD *se_cmd,
	IN TPC_OBJ *obj, 
	IN ROD_TOKEN_512B *token_512b
	)
{
	int codeset_bin = 0;

	/* FIXED ME !!
	 *
	 * Go to position of byte 128 and we should consider the remain token
	 * size before to build those data where starts from byte 128.
	 */

	/* byte 128 ~ byte t (target device descriptor) */
	__create_target_dev_desc(se_cmd, obj, token_512b->target_dev_desc);


	/* byte (t+1) ~ byte n (extended rod token data) */
	__create_ext_rod_token_data(se_cmd, obj, 
		token_512b->ext_rod_token_data, codeset_bin);

	return;
}

static u32 __create_target_dev_desc(
	IN LIO_SE_CMD *se_cmd,
	IN TPC_OBJ *obj,
	IN u8 *buffer
	)
{
	int use_naa = 0;
	u32 off = 0, len = 0;

	/* SPC4R36, page 251
	 *
	 * The TARGET DEVICE DESCRIPTOR field contains desigination
	 * descriptor for the SCSI target device that contains the 
	 * logical unit indicated by the descriptor in CREATOR LOGICAL
	 * UNIT DESCRIPTOR field. The designation descriptor shall have
	 * the ASSOCIATION field set to 10b (i.e. SCSI target device) 
	 * and the DESIGNATOR TYPE field set to 
	 *
	 * a) 2h (EUI-64-based)
	 * b) 3h (NAA) or
	 * c) 8h (SCSI name string)
	 */
	buffer[off] = 
		(obj->se_tpg->se_tpg_tfo->get_fabric_proto_ident(obj->se_tpg) << 4);

	buffer[off++] |= 0x3;	/* CODE SET = UTF-8 */
	buffer[off] = 0x80;	/* Set PIV=1 */
	buffer[off] |= 0x20;	/* Set ASSOCIATION == SCSI target device: 10b */
	buffer[off++] |= 0x8;	/* DESIGNATOR TYPE = SCSI name string */
	off++;			/* Skip over Reserved */

	/* to use SCSI NAME string (only 124 bytes, 128 - 4)*/
	len = ROD_SCSI_NAME_STR_LEN;

	buffer[off] = len;
	off++;

	/* FIXED ME */
	memcpy(&buffer[off], 
		obj->se_tpg->se_tpg_tfo->tpg_get_wwn(obj->se_tpg), len);

	len = ROD_TARGET_DEV_DESC_LEN;
	return len;
}

static void __create_ext_rod_token_data(
	IN LIO_SE_CMD *se_cmd,
	IN TPC_OBJ *obj,
	IN u8 *buffer,
	IN int codeset_bin
	)
{
	EXT_ROD_TOKEN_DATA *ext_data = NULL;
	u8 p[256];

	COMPILE_ASSERT((sizeof(EXT_ROD_TOKEN_DATA) <= EXT_ROD_TOKEN_DATA_LEN));

	ext_data = (EXT_ROD_TOKEN_DATA *)buffer;

	/* SPC4R36, page 251
	 *
	 * The data in this field contains the vendor specific data that marks
	 * the entire ROD token diffcult to predict or guess. 
	 */


	if (codeset_bin == 0){
		/* if the codeset of TARGET DEVICE DESC DESIGNATOR is utf-8,
		 * to copy remain IQN string to extended data area
		 */
		memset(p, 0, sizeof(p));
		memcpy(p, obj->se_tpg->se_tpg_tfo->tpg_get_wwn(obj->se_tpg),
			strlen(obj->se_tpg->se_tpg_tfo->tpg_get_wwn(obj->se_tpg)));

		if (p[ROD_SCSI_NAME_STR_LEN] != 0x0){
			memcpy(ext_data->ext_data.remain_scsi_name_str, 
				&p[ROD_SCSI_NAME_STR_LEN], 
				strlen(&p[ROD_SCSI_NAME_STR_LEN])
				);
		}
	}
	return;
}

static u32 __create_rod_token_type_and_cm_data(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN ROD_TOKEN *token
    )
{
    u32 len = 0;
    return len;
}

u16 __tpc_get_desc_counts(
    IN u16 len
    )
{
    u16 total_len = 0;

    /* The block dev range desc length shall be a multiple of 16 */
    total_len = (len & (~(0x000f)));
    return (total_len / sizeof(BLK_DEV_RANGE_DESC));
}

sector_t __tpc_get_total_nr_blks_by_desc(
    IN BLK_DEV_RANGE_DESC *start,
    IN u16 desc_counts
    )
{
    u16 index = 0;
    sector_t nr_blks = 0;

    BUG_ON(!start);

    for(index = 0; index < desc_counts; index++)
        nr_blks += (sector_t)get_unaligned_be32(&start[index].nr_blks[0]);

    return nr_blks;
}

int __tpc_set_obj_op_counter(
    IN TPC_OBJ *obj
    )
{
    if ((obj->completion_status == SAM_STAT_GOOD)
    ||  (obj->completion_status == SAM_STAT_CONDITION_MET)
    ||  (obj->completion_status == SAM_STAT_CHECK_CONDITION)
    )
    {
        obj->op_counter = 1;
        return 0;
    }
    return 1;
}

int __tpc_set_obj_completion_status(
    IN TPC_OBJ *obj
    )
{
    int ret = 0;

    /* FIXED ME !!
     *
     * The completion_status shall be reserved if the cp_op_status is 
     * 0x10, 0x11, 0x12 
     */
    if ((obj->cp_op_status == OP_IN_PROGRESS_WITHIN_FG_OR_BG_UNKNOWN)
    ||  (obj->cp_op_status == OP_IN_PROGRESS_WITHIN_FG)
    ||  (obj->cp_op_status == OP_IN_PROGRESS_WITHIN_BG)
    )
    {
        DBG_ROD_PRINT("not need to set completion_status value\n");
        ret = 1;
    }

    /* TODO: need to check in the future */
    else if ((obj->cp_op_status == OP_COMPLETED_W_ERR)
    ||  (obj->cp_op_status == OP_COMPLETED_WO_ERR_WITH_ROD_TOKEN_USAGE)
    ||  (obj->cp_op_status == OP_COMPLETED_WO_ERR_BUT_WITH_RESIDUAL_DATA)
    )
        obj->completion_status = SAM_STAT_CHECK_CONDITION;  

    else if (obj->cp_op_status == OP_COMPLETED_WO_ERR)
        obj->completion_status = SAM_STAT_GOOD;

    else if (obj->cp_op_status == OP_TERMINATED)
        obj->completion_status = SAM_STAT_TASK_ABORTED;

    else{
        DBG_ROD_PRINT("unknown cp op value\n");
        ret = 1;
    }
    return ret;
}


int __chk_valid_supported_rod_type(
    IN u32 rod_type
    )
{
    u8 index = 0;
    int ret = 1;

    for (index = 0;; index++){
        if (gRodTypeTable[index].end_table == 1)
            break;

        if (gRodTypeTable[index].rod_type == rod_type){
            ret = 0;
            break;
        }
    }

    return ret;
}

static TPC_TRACK_DATA *__tpc_alloc_track_data(IN void)
{
    TPC_TRACK_DATA *data = NULL;

    if((data = kzalloc(sizeof(TPC_TRACK_DATA), GFP_KERNEL)) == NULL)
        return NULL;

	spin_lock_init(&data->t_cmd_asked_act_lock);
	spin_lock_init(&data->t_status_lock);
	spin_lock_init(&data->t_count_lock);
	spin_lock_init(&data->t_ref_count_lock);

    atomic_set(&data->t_ref_count, 0);

    data->t_cmd_status         = T_CMD_NOT_START;
    data->t_cmd_asked          = CMD_ASKED_NOTHING;
    return data;
}



/*
 * @fn void *__do_alloc_token_data(IN LIO_SE_CMD *se_cmd, IN u32 token_type, IN u16 token_size)
 * @brief
 *
 * @sa 
 * @param[in] se_cmd
 * @param[in] token_type
 * @param[in] token_size
 * @retval 0 -Success / Others - Fail
 */
int __tpc_do_alloc_token_data(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj,
    IN u32 token_type,
    IN u16 token_size,
    IN OUT ERR_REASON_INDEX *err
    )
{
    /* allocate the rod token data if necessary */
    if ((__chk_valid_supported_rod_type(token_type) == 1) || (token_size == 0)){
        *err = ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN;
        goto _EXIT_;
    }

    /* Don't need to increase the obj ref count cause of it is new obj and 
     * is not inserted to list yet.
     */
    obj->o_token_data = (void *)__alloc_rod_token(token_type, token_size);
    if (obj->o_token_data == NULL){
        *err = ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN;
        goto _EXIT_;
    }

    obj->o_token_status = O_TOKEN_STS_ALLOCATED_BUT_NOT_ALIVE;
    return 0;

_EXIT_:
    return 1;
}

/*
 * @fn int __alloc_tpc_obj()
 * @brief
 *
 * @sa 
 * @param[in]
 * @retval
 */
TPC_OBJ *__tpc_do_alloc_obj(
    IN LIO_SE_CMD *se_cmd
    )
{
    TPC_OBJ *obj = NULL;
    LIO_SE_DEVICE *se_dev = NULL;
    bool free_obj = 1;

    /**/
    if (!se_cmd->is_tpc || !se_cmd->se_lun)
        return NULL;

    if((obj = kzalloc(sizeof(TPC_OBJ), GFP_KERNEL)) == NULL)
        return NULL;

    /* FIXED ME !! */
    se_dev = se_cmd->se_lun->lun_se_dev;

        /**/
        INIT_LIST_HEAD(&obj->o_node);
        INIT_LIST_HEAD(&obj->o_data_list);
        init_timer(&obj->o_timer);
        init_timer(&obj->o_token_timer);

        obj->se_lun           = se_cmd->se_lun;
        obj->se_tpg           = se_cmd->se_lun->lun_sep->sep_tpg;
        obj->o_status         = O_STS_ALLOCATED_BUT_NOT_ALIVE;
        obj->o_token_status   = O_TOKEN_STS_NOT_ALLOCATED_AND_NOT_ALIVE;

	spin_lock_init(&obj->o_status_lock);
	spin_lock_init(&obj->o_token_status_lock);
	spin_lock_init(&obj->o_data_lock);
	spin_lock_init(&obj->o_transfer_count_lock);

	spin_lock_init(&obj->o_ref_count_lock);
	spin_lock_init(&obj->o_token_ref_count_lock);

        atomic_set(&obj->o_ref_count, 0);
        atomic_set(&obj->o_token_ref_count, 0);

        obj->o_token_data       = NULL;

        /* --------------------------------------------------------
         * --- Basic information for copy-operation data record ---
         * --------------------------------------------------------
         */
        obj->cp_op_status       = OP_COMPLETED_WO_ERR;
        obj->completion_status  = SAM_STAT_GOOD;

        /* ------------------------------------------
         * --- Basic information for tpc obj data ---
         * ------------------------------------------
         */
        obj->backend_type       = MAX_SUBSYSTEM_TYPE;
        obj->o_data_type        = O_DATA_TYPE_NONE;
        obj->dev_bs_order       = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
        free_obj = 0;

//        printk("dev_bs_order:0x%x\n", obj->dev_bs_order);

_EXIT_:

    if (free_obj && obj){
        kfree(obj);
        return NULL;
    }
    return obj;
}

static ROD_TOKEN_512B *__alloc_rod_token(
    IN u32 token_type,
    IN u16 token_size
    )
{
    ROD_TOKEN_512B *token_512b = NULL;

    /* SBC3R31, page 89
     *
     * The block device zero ROD token represents the use data in which all bits
     * are set to zero and protection information, if any, is set to 
     * 0xFFFF_FFFF_FFFF_FFFFULL. In response to a RECEIVE ROD TOKEN INFORMATION
     * command in which the list identifier field specifies a POPULATE TOKEN command,
     * the copy manager may or may NOT return a ROD token that is the block
     * device zero ROD token.
     *
     * Therefore, we don't support to build the block device zero ROD token here.
     */
    if (token_type == ROD_TYPE_BLK_DEV_ZERO)
        return NULL;

    if (__chk_valid_supported_rod_type(token_type) == 1)
        return NULL;

    if (token_size < ROD_TOKEN_MIN_SIZE)
        return NULL;

    /* SPC4R36, page 251 */
    if ((token_type == ROD_TYPE_ACCESS_UPON_REFERENCE)
    ||  (token_type == ROD_TYPE_PIT_COPY_D4)
    ||  (token_type == ROD_TYPE_PIT_COPY_CHANGE_VULNERABLE)
    ||  (token_type == ROD_TYPE_PIT_COPY_CHANGE_PERSISTENT)
    )
        token_size = ROD_TOKEN_MIN_SIZE;

    if ((token_512b = (ROD_TOKEN_512B *)kzalloc(token_size, GFP_KERNEL)) == NULL)
        return NULL;

    /**/
    put_unaligned_be32(token_type, &token_512b->gen_data.type[0]);
    put_unaligned_be16((token_size - 8), &token_512b->gen_data.token_len[0]);
    return token_512b;

}

void __build_big_endian_data(
    IN u8 *to,
    IN u8 *from,
    IN size_t size
    )
{
    u8 *new_to = NULL;

    if (to == NULL || from == NULL || size == 0)
        return;

    new_to = to + (size-1);
    while (new_to >= to)
        *new_to-- = *from++;

    return;
}

/*
 * @fn TPC_CMD_TYPE tpc_get_type (IN LIO_SE_CMD *se_cmd)
 *
 * @brief To get the type of 3rd-party copy command
 * @note
 * @param[in] se_cmd
 * @retval TPC_CMD_TYPE enum value
 */
TPC_CMD_TYPE tpc_get_type(
    IN LIO_SE_CMD *se_cmd
    )
{
    u8 *cdb = NULL;
    TPC_CMD_TYPE type = TPC_NOT_CP_CMD;

    if (!se_cmd)
        return type;

    cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;
    if ((cdb[0] != EXTENDED_COPY) && (cdb[0] != RECEIVE_COPY_RESULTS))
        return type;

    switch(cdb[0]){
    case EXTENDED_COPY:
        switch((cdb[1] & 0x1f)){

#if 0 // FIXED ME !! Not supported yet
        case 0x1c:  // copy operation abort
            type = TPC_CP_MANAGEMENT;
            break;
        case 0x01:  // extended copy (LID4)
#endif
        /* NOTE:
         *
         * Here we don't support to monitor the EXTENDED COPY (LID1) command
         * by list id. The reason is VMWARE use this command with SNLID = 1
         * to do VAAI XCOPY (please refer the __do_receive_xcopy_op_param().
         * Therefore, we filter this case.
         */
#if 0
        case 0x00:  // extended copy (LID1)
#endif
        case 0x11:  // write using token
        case 0x10:  // populate token
            type = TPC_CP_OP;
            break;
        default:
            break;
        }
        break;

    case RECEIVE_COPY_RESULTS:
        switch((cdb[1] & 0x1f)){
#if 0 // FIXED ME !! Not supported yet
        case 0x00:  // receive copy status (LID1)
        case 0x01:  // receive copy data (LID1)
        case 0x04:  // receive copy failure details (LID1)
        case 0x05:  // receive copy status (LID4)
        case 0x06:  // receive copy data (LID4)
        case 0x08:  // report all rod tokens
#endif
        case 0x07:  // receive rod token information
            type = TPC_CP_MONITOR;
            break;

        case 0x03:  // receive copy operating parameter
            /* FIXED ME !! this cmd doesn't have list id, so I set this NO_TYPE */
            type = TPC_CP_NO_TYPE;
            break;

        default:
            break;
        }
        break;
    default:
        break;
    }

    return type;
}


/*
 * @fn TPC_LID tpc_get_list_id (IN LIO_SE_CMD *se_cmd, IN u32 *list_id)
 *
 * @brief To get the list id from 3rd-party copy command
 * @note
 * @param[in] se_cmd
 * @param[in, out] list_id
 * @retval TPC_LID enum value
 */
TPC_LID tpc_get_list_id(
    IN LIO_SE_CMD *se_cmd,
    IN u32 *list_id
    )
{
    u8 *cdb = NULL;
    u32 get_id = 0;
    TPC_LID ret = TPC_LID_NOT_CP_CMD;
    void *param = NULL;
    LID4_XCOPY_PARAMS *lid4_param = NULL;
//    LID1_XCOPY_PARAMS *lid1_param = NULL;

    if (!se_cmd || !list_id)
        BUG_ON(TRUE);

    cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;
    if ((cdb[0] != EXTENDED_COPY) && (cdb[0] != RECEIVE_COPY_RESULTS))
        return ret;

//    pr_err("cdb[0]:0x%x, cdb[1]:0x%x\n", cdb[0], cdb[1]);

    /* FIXED ME !!
     *
     * The list id for some 3rd-party command will be in parameter data, so we
     * will use transport_kmap_data_sg() to get list id. Please take care when
     * you call this function (make sure we can get param at least).
     */
    switch(cdb[0]){
    case EXTENDED_COPY:
        switch((cdb[1] & 0x1f)){
        case 0x1c:  // copy operation abort
            get_id = get_unaligned_be32(&cdb[2]);
            ret = TPC_LID_GET_OK;
            break;

        case 0x00:  // extended copy (LID1)
            /* NOTE:
             *
             * Here we don't support to monitor the EXTENDED COPY (LID1) command
             * by list id. The reason is VMWARE use this command with SNLID = 1
             * to do VAAI XCOPY (please refer the __do_receive_xcopy_op_param().
             * Therefore, we filter this case.
             */
#if 0
            param = transport_kmap_data_sg(se_cmd);
            if (param){
                /* The parameter may NOT have valid data at this time */
                lid1_param = (LID1_XCOPY_PARAMS *)param;
                if (get_unaligned_be16(&lid1_param->CscdDescListLen[0])){
                    get_id = (u32)lid1_param->ListId;
                    ret = TPC_LID_GET_OK;
                }
                transport_kunmap_data_sg(se_cmd);
            }
#endif
            break;
        case 0x01:  // extended copy (LID4)
            param = transport_kmap_data_sg(se_cmd);
            if (param){
                /* The parameter may NOT have valid data at this time */
                lid4_param = (LID4_XCOPY_PARAMS *)param;
                if (get_unaligned_be16(&lid4_param->HdrCscdDescListLen[0]) == 0x0020){
                    get_id = get_unaligned_be32(&lid4_param->ListId[0]);
                    ret = TPC_LID_GET_OK;
                }
                transport_kunmap_data_sg(se_cmd);
            }
            break;
        case 0x10:  // populate token
        case 0x11:  // write using token
            get_id = get_unaligned_be32(&cdb[6]);
            ret = TPC_LID_GET_OK;
            break;
        default:
            break;
        }
        break;

    case RECEIVE_COPY_RESULTS:
        switch((cdb[1] & 0x1f)){
        case 0x00:  // receive copy status (LID1)
        case 0x01:  // receive copy data (LID1)
        case 0x04:  // receive copy failure details (LID1)
            get_id = (u32)cdb[2];
            ret = TPC_LID_GET_OK;
            break;
        case 0x03:  // receive copy operating parameter
        case 0x08:  // report all rod tokens
            ret = TPC_LID_NO_ID;
            break;
        case 0x05:  // receive copy status (LID4)
        case 0x06:  // receive copy data (LID4)
        case 0x07:  // receive rod token information
            get_id = get_unaligned_be32(&cdb[2]);
            ret = TPC_LID_GET_OK;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    *list_id = get_id;
    return ret;
}


int __dump_br_data(
    IN struct list_head *br_list
    )
{
    BLK_RANGE_DATA *br = NULL;

    list_for_each_entry(br, br_list, b_range_data_node){
        printk("s lba:0x%llx\n", (unsigned long long)br->lba);
        printk("e lba:0x%llx\n", (unsigned long long)(br->lba + br->nr_blks - 1));
        printk("c status:0x%x\n", br->curr_status);
        printk("n status:0x%x\n", br->next_status);
    }
	return 0;
}

static int __tpc_find_conflict_lba_in_br_list(
    IN struct list_head *br_list,
    IN sector_t lba,
    IN sector_t range
    )
{
    BLK_RANGE_DATA *br = NULL;
    bool truncate_next_br = 0, used_to_transfer = 0;
    int ret = -1;

    /* Please make sure some necessary members had been locked before call this */
    BUG_ON(!br_list);

    list_for_each_entry(br, br_list, b_range_data_node){
        if (br->curr_status == R_STATUS_TRUNCATE_USED)
            continue;

        if (truncate_next_br == 1){
            if (br->curr_status == R_STATUS_TRANSFER_USED)
                br->next_status = R_STATUS_TRUNCATE_USED;
            else
                br->next_status = br->curr_status = R_STATUS_TRUNCATE_USED;
            continue;
        }

        if (br->curr_status == R_STATUS_TRANSFER_USED)
            used_to_transfer = 1;

        if (lba >= br->lba && lba <= (br->lba + br->nr_blks - 1)){
            if (br->curr_status == R_STATUS_NOT_USED){
                br->next_status = br->curr_status = R_STATUS_TRUNCATE_USED;
                truncate_next_br = 1;
            }else if (br->curr_status == R_STATUS_TRANSFER_USED){
                br->next_status = R_STATUS_TRUNCATE_USED;
            }
        }
    }

    if (truncate_next_br){
        if (used_to_transfer)
            pr_err("warning: truncate some token ranges");
        else
            pr_err("warning: invalidate all token ranges\n");
        ret = used_to_transfer;
    }
    return ret;

}


/*
 * @fn int tpc_is_to_cancel_rod_token_func1(IN LIO_SE_CMD *se_cmd)
 * @brief To check whether need to cancel the ROD token by current command
 *
 * @sa 
 * @param[in] se_cmd
 *
 * @retval  0 : Successful to execute this function
 *          1 : Un-supported command in this function
 *         -1 : Fail to execute this function
 *
 * @note This function ONLY handle the write-direction command which its LBA and
 *       SECTORS data in the cdb field. For more details, please refer the
 *       __tpc_get_generic_wdir_cmd_data() function.
 */
int tpc_is_to_cancel_rod_token_func1(
	IN LIO_SE_CMD *se_cmd
	)
{

	TPC_OBJ *obj = NULL;
	sector_t lba = 0;
	u32 range = 0;
	LIO_SE_DEVICE *se_dev = NULL;
	LIO_SE_PORTAL_GROUP *se_tpg = NULL;
	ERR_REASON_INDEX err = MAX_ERR_REASON_INDEX;
	int ret = 0, conflict = 0;
	char *isid = NULL;
	char *tiqn = NULL;
	u16 tag = 0;


	/* FIXED ME, we may add more command in __tpc_get_generic_wdir_cmd_data() */
	if ((se_cmd->data_direction != DMA_TO_DEVICE) || !se_cmd->se_lun)
		return -1;

	ret = __tpc_get_generic_wdir_cmd_data(se_cmd, &lba, &range);
	if (ret)
		return ret;

	/* If the transfer range is zero, means no any data will be transferred.
	 * And, this is NOT error case
	 */
	if (!ret && !range)
		return 0;

	/* From this point, it belongs to write-dir command came from 
	 * __tpc_get_generic_wdir_cmd_data()
	 */
	isid = kzalloc(PR_REG_ISID_LEN, GFP_KERNEL);
	tiqn = kzalloc(TARGET_IQN_NAME_LEN, GFP_KERNEL);
	if (!isid || !tiqn){
		ret = -1;
		goto _OUT_;
	}

	if((__tpc_get_isid_by_se_cmd(se_cmd, PR_REG_ISID_LEN, isid))
	|| (__tpc_get_tiqn_and_pg_tag_by_se_cmd(se_cmd, tiqn, &tag))
	)
	{
		ret = -1;
		goto _OUT_;
	}

	/* step3 : Start to parse each tpc obj by (lba, range) data */
	se_dev = se_cmd->se_lun->lun_se_dev;
	se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;


	spin_lock_bh(&se_tpg->tpc_obj_list_lock);
        list_for_each_entry(obj, &se_tpg->tpc_obj_list, o_node){

		/* To indicate we will use the obj now */
		__tpc_obj_ref_count_lock_inc(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

		if (!IS_ROD_TYPE(obj->o_data_type))
			goto _EXIT_CHECK_OBJ_;

		/* check obj status is alive or not */
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);

		__tpc_token_ref_count_lock_inc(obj);

		if (__tpc_is_token_invalid(obj, &err) != 0){
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _EXIT_CHECK_TOKEN_;
		}

		spin_lock(&obj->o_data_lock);
		if (!obj->o_token_data){
			spin_unlock(&obj->o_data_lock);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _EXIT_CHECK_TOKEN_;
		}

		if (list_empty(&obj->o_data_list)){
			spin_unlock(&obj->o_data_lock);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _EXIT_CHECK_TOKEN_;
		}
		spin_unlock(&obj->o_data_lock);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

		/* Here to make sure the I_T_NEXUS of write command is the same as
		 * the token obj which we will get. Escpecially, for LU checking */
		if ((__tpc_is_same_initiator_isid(obj->isid, isid))
		||  (__tpc_is_same_tiqn_and_pg_tag(obj->tiqn, tiqn, obj->pg_tag, tag))
		||  (__tpc_is_same_id_cscd_desc_cmd_obj(se_cmd, obj))
		)
			goto _EXIT_CHECK_TOKEN_;


		/* Here will go through obj->o_data_list (BLK_RANGE_DATA type)
		 * to find whether the block range data need to be truncated
		 * or invalid.
		 */
		conflict = __tpc_find_conflict_lba_in_br_list(
			&obj->o_data_list, lba, range);

		if (conflict != -1){
#if 1
			pr_warn("warning(P1): cmd(op:0x%x, lba:0x%llx, range:0x%x) "
				"conflicts with obj(id:0x%x, op_sac:0x%x)\n", 
				CMD_TO_TASK_CDB(se_cmd)->t_task_cdb[0], 
				(unsigned long long)lba, range, obj->list_id, 
				obj->op_sac);
#endif

			/* To update the token status if all ranges was cancelled */
			if (conflict == 0){
				spin_lock_bh(&se_tpg->tpc_obj_list_lock);
				__tpc_update_obj_token_status_lock(
					obj, O_TOKEN_STS_CANCELLED);
				spin_unlock_bh(&se_tpg->tpc_obj_list_lock);				
			}
			/* FIXED ME 
			 *
			 * Here won't recompluted the total transfer
			 * count even if we truncate some token ranges
			 * which will be used by other command. This is
			 * for windows HCK testing.
			 *
			 * p.s. Need to change this again
			 */
		}
		/* fall-through */

_EXIT_CHECK_TOKEN_:
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
		/* To release the token ref */
		__tpc_token_ref_count_lock_dec(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

_EXIT_CHECK_OBJ_:
		/* To release the ref count for obj and go to next */
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
		__tpc_obj_ref_count_lock_dec(obj);
	}
	spin_unlock_bh(&se_tpg->tpc_obj_list_lock);


_OUT_:
    if (isid)
        kfree(isid);

    if (tiqn)
        kfree(tiqn);

    return ret;



}
EXPORT_SYMBOL(tpc_is_to_cancel_rod_token_func1);

/*
 * @fn int tpc_is_to_cancel_rod_token_func2(IN LIO_SE_CMD *se_cmd)
 * @brief To check whether need to cancel the ROD token by current command
 *
 * @sa 
 * @param[in] se_cmd
 * @retval  0 : Successful to execute this function
 *          1 : Un-supported command in this function
 *         -1 : Fail to execute this function
 *
 * @note This function ONLY handle the write-direction command which its LBA and
 *       SECTORS data in the parameter data list. For more details, please refer
 *       the __tpc_get_wdir_blk_desc_format_cmd_data() function.
 */
int tpc_is_to_cancel_rod_token_func2(
	IN LIO_SE_CMD *se_cmd
	)
{
	LIO_SE_DEVICE *se_dev = NULL;
	LIO_SE_PORTAL_GROUP *se_tpg = NULL;
	TPC_OBJ *obj = NULL;
	BLK_DEV_RANGE_DESC *start = NULL;
	sector_t total_nr_blks = 0;
	u16 desc_counts = 0, index = 0;
	ERR_REASON_INDEX err = MAX_ERR_REASON_INDEX;
	int ret = 0, conflict = 0;
	char *isid = NULL, *tiqn = NULL;
	u16 tag = 0;


	/* FIXED ME, we may add more command in __tpc_get_wdir_blk_desc_format_cmd_data() */
	if ((se_cmd->data_direction != DMA_TO_DEVICE) || !se_cmd->se_lun)
		return -1;

	/* FIXED ME
	 *
	 * For POPULATE TOKEN and WRITE USING TOKEN, windows treat them as DATA-OUT
	 * command and they all will be handled in LIO write thread procedure. 
	 * In the other words, if host send the POPULATE TOKEN and WRITE USING TOKEN
	 * by sequence, there is no any chance to truncate the token range used by 
	 * another WRITE USING TOKEN.
	 *
	 * case1:
	 *
	 * host --> POPULATE TOKEN (token A)
	 *      --> WRITE USING TOKEN (use token A)
	 *      --> POPULATE TOKEN (token B conflict with token A)
	 *      --> WRITE USING TOKEN (use token B)
	 *
	 * if WRITE USING TOKEN (use token A) is still in progress, the
	 * WRITE USING TOKEN (use token B) won't have any chance to truncate token A
	 *
	 * case2:
	 *
	 * host --> POPULATE TOKEN (token A)
	 *      --> POPULATE TOKEN (token B conflict with token A)
	 *      --> WRITE USING TOKEN (use token A)
	 *      --> WRITE USING TOKEN (use token B)
	 *
	 * For this, if WRITE USING TOKEN (use token A) is still in progress, the
	 * WRITE USING TOKEN (use token B) have chance to truncate token A. But,
	 * if want to do this, we need to truncate the token A before to add command
	 * to write therad queue
	 *
	 */
	ret = __tpc_get_wdir_blk_desc_format_cmd_data(
		se_cmd,  (u8 **)&start, &desc_counts, &total_nr_blks);

	if (ret)
		return ret;

	/* If the transfer range is zero, means no any data will be transferred.
	 * And, this is NOT error case
	 */
	if (!ret && !total_nr_blks)
		return 0;

	/* To depend on command type to get some information first */
	isid = kzalloc(PR_REG_ISID_LEN, GFP_KERNEL);
	tiqn = kzalloc(TARGET_IQN_NAME_LEN, GFP_KERNEL);
	if (!isid || !tiqn){
		ret = -1;
		goto _OUT_;
	}

	if (!se_cmd->is_tpc){
		if((__tpc_get_isid_by_se_cmd(se_cmd, PR_REG_ISID_LEN, isid))
		|| (__tpc_get_tiqn_and_pg_tag_by_se_cmd(se_cmd, tiqn, &tag))
		)
			ret = -1;

	}else{
		if((__tpc_get_isid_by_se_td(se_cmd, isid))
		|| (__tpc_get_tiqn_and_pg_tag_by_se_td(se_cmd, tiqn, &tag))
		)
			ret = -1;
	}

	if (ret == -1)
		goto _OUT_;

	/* step3 : Start to parse each tpc obj by (lba, range) data */
	se_dev = se_cmd->se_lun->lun_se_dev;
	se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;

	spin_lock_bh(&se_tpg->tpc_obj_list_lock);
        list_for_each_entry(obj, &se_tpg->tpc_obj_list, o_node){

		/* To indicate we will use the obj now */
		__tpc_obj_ref_count_lock_inc(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

		if (!IS_ROD_TYPE(obj->o_data_type))
			goto _EXIT_CHECK_OBJ_;

		/* check obj status is alive or not */
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);

		__tpc_token_ref_count_lock_inc(obj);

		if (__tpc_is_token_invalid(obj, &err) != 0){
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _EXIT_CHECK_TOKEN_;
		}

		spin_lock(&obj->o_data_lock);
		if (!obj->o_token_data){
			spin_unlock(&obj->o_data_lock);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _EXIT_CHECK_TOKEN_;
		}

		if (list_empty(&obj->o_data_list)){
			spin_unlock(&obj->o_data_lock);
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
			goto _EXIT_CHECK_TOKEN_;
		}
		spin_unlock(&obj->o_data_lock);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

		/* Here to make sure the I_T_NEXUS of write command is the same as
		 * the token obj which we will get. Escpecially, for LU checking */
		if ((__tpc_is_same_initiator_isid(obj->isid, isid))
		||  (__tpc_is_same_tiqn_and_pg_tag(obj->tiqn, tiqn, obj->pg_tag, tag))
		||  (__tpc_is_same_id_cscd_desc_cmd_obj(se_cmd, obj))
		)
			goto _EXIT_CHECK_TOKEN_;

		/* everything is fine, to parse each desc data now */
		for (index = 0; index < desc_counts; index++){
			if (get_unaligned_be32(&start[index].nr_blks[0]) == 0)
				continue;

			/* To go through obj->o_data_list (BLK_RANGE_DATA type) */
			conflict = __tpc_find_conflict_lba_in_br_list(
					&obj->o_data_list,
					get_unaligned_be64(&start[index].lba[0]),
					get_unaligned_be32(&start[index].nr_blks[0])
					);

			if (conflict != -1){
#if 1
				pr_warn("warning(P2) cmd(op:0x%x, "
				"blk_desc_index:0x%x, lba:0x%llx, range:0x%x) "
				"conflicts with obj(id:0x%x, op_sac:0x%x)\n", 
				CMD_TO_TASK_CDB(se_cmd)->t_task_cdb[0], index,
				get_unaligned_be64(&start[index].lba[0]),
				get_unaligned_be32(&start[index].nr_blks[0]),
				obj->list_id, obj->op_sac);
#endif
				/* To update the token status if all ranges was cancelled */
				if (conflict == 0){
					spin_lock_bh(&se_tpg->tpc_obj_list_lock);
					__tpc_update_obj_token_status_lock(
						obj, O_TOKEN_STS_CANCELLED);
					spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
				}
				/* FIXED ME 
				 *
				 * Here won't recompluted the total transfer
				 * count even if we truncate some token ranges
				 * which will be used by other command. This is
				 * for windows HCK testing.
				 *
				 * p.s. Need to change this again
				 */

				/* we do once only ... */
				break;
			}
		}

		/* fall-though */

_EXIT_CHECK_TOKEN_:

		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
		/* To release the token ref */
		__tpc_token_ref_count_lock_dec(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

_EXIT_CHECK_OBJ_:
		/* To release the ref count for obj and go to next */
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
		__tpc_obj_ref_count_lock_dec(obj);
	}
	spin_unlock_bh(&se_tpg->tpc_obj_list_lock);


_OUT_:
    if (isid)
        kfree(isid);

    if (tiqn)
        kfree(tiqn);

    return ret;

}
EXPORT_SYMBOL(tpc_is_to_cancel_rod_token_func2);

/*
 * @fn int __tpc_get_wdir_blk_desc_format_cmd_data(IN LIO_SE_CMD *se_cmd, IN OUT u8 **s,
 *                                                 IN OUT u16 *c, IN OUT sector_t *nr_blks)
 *
 * @brief To get the LBA and SECTORS for write-direction command from parameter
 *        list data
 * @sa 
 *
 * @param[in] se_cmd
 *
 * @param[in,out] s
 *
 * @param[in,out] c
 *
 * @param[in,out] nr_blks
 *
 * @retval  0 : Successful to execute this function
 *          1 : Un-supported command in this function
 *
 * @note This function ONLY handle the write-direction command which its LBA and
 *       SECTORS data in the parameter list data.
 */
static int __tpc_get_wdir_blk_desc_format_cmd_data(
    IN LIO_SE_CMD *se_cmd,
    IN OUT u8 **s,
    IN OUT u16 *c,
    IN OUT sector_t *nr_blks
    )
{
    u8 *p = NULL;
    u8 *cdb = NULL;
    int ret = 0;

    if((p = (u8 *)transport_kmap_data_sg(se_cmd)) == NULL)
        return 1;

    cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;

    switch(cdb[0]){

    case EXTENDED_COPY:
        switch((cdb[1] & 0x1f)){
#if 0 // FIXED ME !! Not supported now
        case 0x00:  // extended copy (LID1)
            break;
        case 0x01:  // extended copy (LID4)
            break;
#endif
        case 0x11:  // write using token
            if (get_unaligned_be32(&cdb[10]) < 552){
                pr_err("error: %s - alloc_len:0x%x is smaller than 552 !!\n", 
                            __func__, get_unaligned_be32(&cdb[10]));
                ret = 1;
                break;
            }

            if ((get_unaligned_be16(&p[0]) < 550)
            ||  (get_unaligned_be16(&p[534]) < 0x10)
            )
            {
                pr_err("error: %s - length is invalid, len0:0x%x, len1:0x%x\n",
                    __func__,  get_unaligned_be16(&p[0]), get_unaligned_be16(&p[534]));
                ret = 1;
                break;
            }

            *s = (u8 *)(p + sizeof(WRITE_BY_TOKEN_PARAM));
            *c = __tpc_get_desc_counts(get_unaligned_be16(&p[534]));
            *nr_blks = __tpc_get_total_nr_blks_by_desc((BLK_DEV_RANGE_DESC *)(*s), *c);
            break;
        default:
            /* others are NOT for write-request */
            break;
        }

        break;

    case UNMAP:

        /* FIXED ME !!
         *
         * SBC3R31, page 171, The format of UNMAP block descriptor is the same
         * as BLK_DEV_RANGE_DESC 
         */
        *s = (u8 *)(p + 8);
        *c = __tpc_get_desc_counts(get_unaligned_be16(&p[2]));
        *nr_blks = __tpc_get_total_nr_blks_by_desc((BLK_DEV_RANGE_DESC *)(*s), *c);
        break;

    default:
//        pr_err("warning!! unsupported cdb[0]:0x%x in phase 2\n", cdb[0]);
        ret = 1;
        break;
    }

    if (p)
         transport_kunmap_data_sg(se_cmd);

    return ret;

}

/*
 * @fn int __tpc_get_generic_wdir_cmd_data(IN LIO_SE_CMD *se_cmd, IN OUT sector_t *lba, IN OUT u32 *range)
 *
 * @brief To get the LBA and SECTORS for write-direction command
 *
 * @sa 
 *
 * @param[in] se_cmd
 *
 * @param[in,out] lba
 *
 * @param[in,out] range
 *
 * @retval  0 : Successful to execute this function
 *          1 : Un-supported command in this function
 *
 * @note This function ONLY handle the write-direction command which its LBA and
 *       SECTORS data in the cdb field.
 */
static int __tpc_get_generic_wdir_cmd_data(
    IN LIO_SE_CMD *se_cmd,
    IN OUT sector_t *lba,
    IN OUT u32 *range
    )
{
    u8 *cdb = CMD_TO_TASK_CDB(se_cmd)->t_task_cdb;
    u16 sac = 0;
    int ret = 0;

//    pr_err("%s - cdb[0]:0x%x\n", __func__, CMD_TO_TASK_CDB(se_cmd)->t_task_cdb[0]);

    switch(cdb[0]){
    case WRITE_6:
        *range = (sector_t)__call_transport_get_sectors_6(cdb, se_cmd, &ret);
        if (!ret)
            *lba = __call_transport_lba_21(cdb);
        break;

    case XDWRITEREAD_10:
    case WRITE_VERIFY:
    case WRITE_10:
        *range = (sector_t)__call_transport_get_sectors_10(cdb, se_cmd, &ret);
        if (!ret)
            *lba = __call_transport_lba_32(cdb);
        break;

    case WRITE_VERIFY_12:
    case WRITE_12:
        *range = (sector_t)__call_transport_get_sectors_12(cdb, se_cmd, &ret);
        if (!ret)
            *lba = __call_transport_lba_32(cdb);
        break;

//    case WRITE_VERIFY_16:
    case WRITE_16:
        *range = (sector_t)__call_transport_get_sectors_16(cdb, se_cmd, &ret);
        if (!ret)
            *lba = __call_transport_lba_64(cdb);
        break;

    case VARIABLE_LENGTH_CMD:
        sac = get_unaligned_be16(&cdb[8]);
        switch (sac) {
            case XDWRITEREAD_32:
                *range = (sector_t)__call_transport_get_sectors_32(cdb, se_cmd, &ret);
                if (!ret)
                    *lba = __call_transport_lba_64_ext(cdb);
                break;

            case WRITE_SAME_32:
                *range = (sector_t)__call_transport_get_sectors_32(cdb, se_cmd, &ret);
                if (!ret)
                    *lba = get_unaligned_be64(&cdb[12]);
                break;

            default:
                pr_err("error: %s - VARIABLE_LENGTH_CMD service action 0x%04x not supported\n",
                                __func__, sac);
                ret = 1;
                break;
        }

        break;

    case WRITE_SAME:
        *range = (sector_t)__call_transport_get_sectors_10(cdb, se_cmd, &ret);
        if (!ret)
            *lba = get_unaligned_be32(&cdb[2]);
        break;

    case WRITE_SAME_16:
        *range = (sector_t)__call_transport_get_sectors_16(cdb, se_cmd, &ret);
        if (!ret)
            *lba = get_unaligned_be64(&cdb[2]);
        break;

    case SCSI_COMPARE_AND_WRITE: /* FIXED ME !! */
        *range = (u32)cdb[13];
        *lba   = get_unaligned_be64(&cdb[2]);
        break;

    default:
#if 0
        pr_err("warning!! unsupported cdb[0]:0x%x, in phase 1. "
                "go to phase 2 again\n", cdb[0]);
#endif
        ret = 1;
        break;
    }
    return ret;
}

int __tpc_get_isid_by_se_td(
    IN LIO_SE_CMD *se_cmd,
    IN OUT char *isid
    )
{
    if (se_cmd == NULL || isid == NULL)
        return 1;

    if ((!se_cmd->is_tpc) || (se_cmd->t_isid[0] == 0x00))
        return 1;

    memcpy(isid, se_cmd->t_isid, sizeof(se_cmd->t_isid));
    return 0;
}

int __tpc_get_tiqn_and_pg_tag_by_se_td(
	IN LIO_SE_CMD *se_cmd,
	IN char *tiqn,
	IN u16 *tag
	)
{
	if (se_cmd == NULL || tiqn == NULL || tag == NULL)
		return 1;

	if (!se_cmd->is_tpc || !se_cmd->t_tiqn)
		return 1;

	memcpy(tiqn, se_cmd->t_tiqn, TARGET_IQN_NAME_LEN);
	*tag = se_cmd->t_pg_tag;
	return 0;
}

int __tpc_get_tiqn_and_pg_tag_by_se_cmd(
    IN LIO_SE_CMD *se_cmd,
    IN OUT char *tiqn,
    IN OUT u16 *tag
    )
{
    LIO_SE_DEVICE *se_dev = NULL;
    LIO_SE_PORTAL_GROUP *se_tpg = NULL;
    char *name = NULL;
    unsigned long flags = 0;
    int ret = 1;

    if (se_cmd == NULL || tiqn == NULL || tag == NULL)
        return ret;

    if (!se_cmd->se_lun)
        return ret;

    se_dev = se_cmd->se_lun->lun_se_dev;
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&se_cmd->se_lun->lun_sep_lock);

    if (se_cmd->se_lun->lun_sep){
        if (!se_cmd->se_lun->lun_sep->sep_tpg)
            goto _EXIT_;

        se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;
        if (!se_tpg->se_tpg_tfo->tpg_get_wwn || !se_tpg->se_tpg_tfo->tpg_get_tag)
            goto _EXIT_;

        /* to get the tiqn name and tpg tag value */
        name = se_tpg->se_tpg_tfo->tpg_get_wwn(se_tpg);
        if (name[0] == 0x00)
            goto _EXIT_;

        memcpy(tiqn, name, strlen(name));
        *tag = se_tpg->se_tpg_tfo->tpg_get_tag(se_tpg);

        pr_debug("%s: tiqn:%s, tag:0x%x\n",__func__, tiqn, *tag);
        ret = 0;
    }

_EXIT_:
    spin_unlock(&se_cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
    return ret;
}


int __tpc_get_isid_by_se_cmd(
    IN LIO_SE_CMD *se_cmd,
    IN u32 size,
    IN OUT char *isid
    )
{
    LIO_SE_PORTAL_GROUP *se_tpg = NULL;
    LIO_SE_DEVICE *se_dev = NULL;
    int ret = 1;
    unsigned long flags;

    /* FIXED ME !!! */
    if (!se_cmd)
        return ret;

    if (!se_cmd->se_lun || !isid || (size < PR_REG_ISID_LEN))
        return ret;

    se_dev = se_cmd->se_lun->lun_se_dev;
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&se_cmd->se_lun->lun_sep_lock);

    if (se_cmd->se_lun->lun_sep){
        if (!se_cmd->se_lun->lun_sep->sep_tpg)
            goto _EXIT_;

        se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;
        if (!se_tpg->se_tpg_tfo->sess_get_initiator_sid)
            goto _EXIT_;

        se_tpg->se_tpg_tfo->sess_get_initiator_sid(se_cmd->se_sess, isid, size);
        if (isid[0] == 0x00)
            goto _EXIT_;

//        DBG_ROD_PRINT("%s: isid:%s\n",__func__, isid);
        ret = 0;
    }

_EXIT_:
    spin_unlock(&se_cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
    return ret;

}


int __tpc_is_same_i_t_nexus_func1(
    IN TPC_OBJ *obj,
    IN LIO_SE_CMD *se_cmd
    )
{
    if (!obj || !se_cmd)
        return 1;

    if (__tpc_is_same_initiator_isid(obj->isid, se_cmd->t_isid))
        return 1;

    if (__tpc_is_same_tiqn_and_pg_tag(
                obj->tiqn, se_cmd->t_tiqn, 
                obj->pg_tag, se_cmd->t_pg_tag
                ))
        return 1;


    return 0;
}

int __tpc_is_same_i_t_nexus_func2(
    IN LIO_SE_CMD *s_se_cmd,
    IN LIO_SE_CMD *d_se_cmd
    )
{
    if (!s_se_cmd || !d_se_cmd)
        return 1;

    if (__tpc_is_same_initiator_isid(s_se_cmd->t_isid, d_se_cmd->t_isid))
        return 1;

    if (__tpc_is_same_tiqn_and_pg_tag(
                s_se_cmd->t_tiqn, d_se_cmd->t_tiqn, 
                s_se_cmd->t_pg_tag, d_se_cmd->t_pg_tag
                ))
        return 1;

    return 0;
}

int __tpc_is_same_id_cscd_desc_cmd_obj(
    IN LIO_SE_CMD *se_cmd,
    IN TPC_OBJ *obj
    )
{
    ID_CSCD_DESC data0, data1;

    /* FIXED ME , this is too ugly ... */
    if (!se_cmd || !obj)
        return 1;

    if (!obj->se_lun || !se_cmd->se_lun)
        return 1;

    if (!obj->se_lun->lun_se_dev || !se_cmd->se_lun->lun_se_dev)
        return 1;

    memset(&data0, 0, sizeof(ID_CSCD_DESC));
    memset(&data1, 0, sizeof(ID_CSCD_DESC));

    /* for se_cmd to the write destination */
    __create_id_cscd_desc(se_cmd->se_lun, se_cmd->se_lun->lun_se_dev, NULL, &data0);

    /* for source obj*/
    __create_id_cscd_desc(obj->se_lun, obj->se_lun->lun_se_dev, NULL, &data1);

    if (!memcmp(&data0, &data1, sizeof(ID_CSCD_DESC)))
        return 0;

    pr_debug("error: id cscd desc of src/dest are not the same... \n");
    return 1;

}

int __tpc_is_same_tiqn_and_pg_tag(
    IN char *s_tiqn,
    IN char *d_tiqn,
    IN u16 s_tag,
    IN u16 d_tag
    )
{
	/* FIXED ME !! This may need to change in the future */
	if (!s_tiqn || !d_tiqn)
		return 1;

	/* 2014/08/17, adamhsu, redmine 9042 */
	if ((!memcmp(s_tiqn, d_tiqn, TARGET_IQN_NAME_LEN))
	&& (s_tag == d_tag)
	)
		return 0;

#if 0
	pr_info("[%s] not same tiqn and pg_tag. s_tiqn:%s, s_tag:0x%x, ""
		d_tiqn:%s, d_tag:0x%x\n", __func__, s_tiqn, s_tag,
		d_tiqn, d_tag);
#endif
	return 1;
}


int __tpc_is_same_initiator_isid(
    IN char *s_isid,
    IN char *d_isid
    )
{

    /* FIXED ME !! This may need to change in the future. */
    if (!s_isid || !d_isid)
        return 1;

    if (s_isid[0] == 0x00 || d_isid[0] == 0x00)
        return 1;

    if (memcmp(s_isid, d_isid, PR_REG_ISID_LEN) == 0)
        return 0;

#if 0
    pr_info("[%s] not same isid. s_isid:%s, d_isid:%s\n", __func__,
    	s_isid, d_isid);
#endif

    return 1;
}

/*
 * @fn TPC_OBJ *__tpc_get_obj_by_token_data(IN u8 *token_id, IN u8 *lu_designator, IN u8 *target_dev_desc, IN OUT ERR_REASON_INDEX *err)
 * @brief This function will try to get the obj which was matched with the
 *        token data passed by upper's command from tpg_list variable
 *
 * @sa 
 * @param[in] token_id
 * @param[in] lu_designator
 * @param[in] target_dev_desc
 * @param[in,out] err
 * @retval NULL - not found any matched obj / non-NULL - found matched obj
 */
TPC_OBJ *__tpc_get_obj_by_token_data(
	IN u8 *token_id,
	IN u8 *lu_designator,
	IN u8 *target_dev_desc,
	IN OUT ERR_REASON_INDEX *err
	)
{
	TPC_OBJ *obj = NULL;
	LIO_SE_PORTAL_GROUP *o_se_tpg = NULL, *tmp_se_tpg = NULL;
	u16 len = sizeof(ROD_TOKEN);
	struct list_head *tpg_list = NULL;
	spinlock_t *tpg_lock = NULL;
	int ret = 0, use_naa = 0;
	u8 tmp_id[8];
	u8 tmp_lu_designator[20];
	u8 *ext_data = NULL, *p = NULL;

	/**/
	*err = ERR_INVALID_PARAMETER_LIST;

	/* The target device descriptor shall be SCSI name string DESIGNATOR here */
	if(__tpc_verify_target_dev_desc_hdr(target_dev_desc))
		return NULL;

	/* FIXED ME */
	tpg_list = tpc_get_tpg_list_var();
	tpg_lock = (spinlock_t *)tpc_get_tpg_lock_var();

	spin_lock_bh(tpg_lock);
	if (list_empty(tpg_list)){
		spin_unlock_bh(tpg_lock);
		return NULL;
	}
	
	/* FIXED ME */
	*err = ERR_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE;

	list_for_each_entry(o_se_tpg, tpg_list, se_tpg_node){

		if ((o_se_tpg->se_tpg_type != TRANSPORT_TPG_TYPE_NORMAL)
		|| (!(o_se_tpg->se_tpg_tfo->tpg_get_wwn))
		|| (!(o_se_tpg->se_tpg_tfo->tpg_get_tag))
		)
			continue;

		spin_unlock_bh(tpg_lock);

		/* FIXED ME:
		 * any reason to remove node from tpg_list during to
		 * process this ??
		 */
			
		/* To search obj from list by this se_tpg */
		spin_lock_bh(&o_se_tpg->tpc_obj_list_lock);
		list_for_each_entry(obj, &o_se_tpg->tpc_obj_list, o_node){

			/* To indicate we will use this obj now */
			__tpc_obj_ref_count_lock_inc(obj);
			spin_unlock_bh(&o_se_tpg->tpc_obj_list_lock);

			if (!IS_ROD_TYPE(obj->o_data_type))
				goto _EXIT_OBJ_CHECK_;

			if (use_naa == 0){
				p = obj->se_tpg->se_tpg_tfo->tpg_get_wwn(o_se_tpg);
				ext_data = (target_dev_desc + ROD_TARGET_DEV_DESC_LEN);
	
				ret = memcmp(p, &target_dev_desc[4], ROD_SCSI_NAME_STR_LEN);
				if (ret)
					goto _EXIT_OBJ_CHECK_;
	
				ret = memcmp(&p[ROD_SCSI_NAME_STR_LEN], 
					&ext_data[0], 
					strlen(&p[ROD_SCSI_NAME_STR_LEN]));

				if (ret)
					goto _EXIT_OBJ_CHECK_;
			}

			/* here to create two checking conditions
			 * (1) token id
			 * (2) lu designator (20 bytes)
			 */
			memset(&tmp_lu_designator, 0, sizeof(tmp_lu_designator));

			put_unaligned_be64(*(u64*)token_id, &tmp_id[0]);
			__make_target_naa_6h_hdr(obj->se_lun->lun_se_dev, 
				&tmp_lu_designator[0]);

			__get_target_parse_naa_6h_vendor_specific(
				obj->se_lun->lun_se_dev, 
				&tmp_lu_designator[3]);

			if (!(obj->create_time == *(u64 *)&tmp_id[0]
			&& (!memcmp(tmp_lu_designator, lu_designator, 20)))
			)
				goto _EXIT_OBJ_CHECK_;


			/* To check the status of obj and token are ALIVE or not */
			spin_lock_bh(&o_se_tpg->tpc_obj_list_lock);

			if (!OBJ_STS_ALIVE(__tpc_get_obj_status_lock(obj))){			
				spin_unlock_bh(&o_se_tpg->tpc_obj_list_lock);
				goto _EXIT_OBJ_CHECK_;	
			}

			/* next step to check token information in obj */
			__tpc_token_ref_count_lock_inc(obj);

			spin_lock(&obj->o_data_lock);
			if (!obj->o_token_data){			
				spin_unlock(&obj->o_data_lock);
				spin_unlock_bh(&o_se_tpg->tpc_obj_list_lock);
				goto _EXIT_OBJ_TOKEN_DATA_CHECK_;
			}
			spin_unlock(&obj->o_data_lock);

			/* found the obj we wanted, now to check
			 * what kind of the token status if got
			 * the valid obj by  passing token data.
			 * We do this cause of the code may
			 * change the token status before to free
			 * the token resource in token timer
			 */
			spin_lock(&obj->o_token_status_lock);
			
			if (!(__tpc_is_token_expired(obj->o_token_status)))
				*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED;
			else if (!(__tpc_is_token_cancelled(obj->o_token_status)))
				*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED;
			else if (!(__tpc_is_token_deleted(obj->o_token_status)))
				*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED;
			else if (TOKEN_STS_ALIVE(obj->o_token_status))
				*err = MAX_ERR_REASON_INDEX;
			else
				BUG_ON(1);
			
			pr_debug("%s: found matched obj(id:0x%x) by token "
				"(token status:0x%x)\n", __func__, 
				obj->list_id, obj->o_token_status);
			
			/* To free all locks since we found obj */
			spin_unlock(&obj->o_token_status_lock);
			spin_unlock_bh(&o_se_tpg->tpc_obj_list_lock);				
			
			/* The same reason as __tpc_get_obj_by_id_opsac(),
			 * we don't decrease obj ref count and let caller
			 * decide to decrease it somewhere. Moreover,we also
			 * increate the token ref count since we found it here. */
			return obj;  


_EXIT_OBJ_TOKEN_DATA_CHECK_:
			spin_lock_bh(&o_se_tpg->tpc_obj_list_lock);
			__tpc_token_ref_count_lock_dec(obj);
			spin_unlock_bh(&o_se_tpg->tpc_obj_list_lock);
_EXIT_OBJ_CHECK_:
			/* Go to next */
			spin_lock_bh(&o_se_tpg->tpc_obj_list_lock);
			__tpc_obj_ref_count_lock_dec(obj);
		}

		spin_unlock_bh(&o_se_tpg->tpc_obj_list_lock);
		spin_lock_bh(tpg_lock);
	}
	spin_unlock_bh(tpg_lock);

	return NULL;

}

TPC_OBJ *__tpc_get_obj_by_id_opsac(
	IN LIO_SE_PORTAL_GROUP *se_tpg,
	IN LIO_SE_CMD *se_cmd
	)
{
	TPC_OBJ *obj = NULL;

	/**/
	if (!se_cmd || !se_tpg)
		return NULL;

	if (!se_cmd->is_tpc)
		return NULL;


	/* search all tpc obj list in this portal group */
	spin_lock_bh(&se_tpg->tpc_obj_list_lock);
	if (list_empty(&se_tpg->tpc_obj_list)){
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
		return NULL;
	}
	
	/**/
	list_for_each_entry(obj, &se_tpg->tpc_obj_list, o_node){
	
		/* To indicate we will use this obj now */
		__tpc_obj_ref_count_lock_inc(obj);
		spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
	
		/**/
		if (SAME_ID_OP_SAC(obj->list_id, se_cmd->t_list_id, 
			obj->op_sac, se_cmd->t_op_sac)
		&&  (!(__tpc_is_same_i_t_nexus_func1(obj, se_cmd)))
		)
		{
			spin_lock_bh(&se_tpg->tpc_obj_list_lock);
			if (OBJ_STS_ALIVE(__tpc_get_obj_status_lock(obj))){
				spin_unlock_bh(&se_tpg->tpc_obj_list_lock);

				/* FIXED ME !!
				 *
				 * If found the obj, we don't decrease the obj
				 * ref count here. We let caller to do it
				 * somewhere and the reason to do this is to
				 * avoid this found obj will be free here but
				 * it will be used later ...
				 */
				return obj;
			}
			spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
		}

		/* Go to next */
		spin_lock_bh(&se_tpg->tpc_obj_list_lock);
		__tpc_obj_ref_count_lock_dec(obj);
	}
	spin_unlock_bh(&se_tpg->tpc_obj_list_lock);
	return NULL;

}

void __tpc_build_obj_sense_data(
    IN TPC_OBJ *obj,
    IN ERR_REASON_INDEX err,
    IN u8 asc,
    IN u8 ascq
    )
{
    u8 *buffer = NULL;
    u8 offset= 0;

    BUG_ON(!obj);
    BUG_ON((err > MAX_ERR_REASON_INDEX));

    if (err == MAX_ERR_REASON_INDEX)
        return;

    DBG_ROD_PRINT("%s: (err:0x%x, asc:0x%x, ascq:0x%x) in obj(id:0x%x)\n", 
                        __func__, err, asc, ascq, obj->list_id);

    buffer = &obj->sense_data[0];
    memset(buffer, 0, ROD_SENSE_DATA_LEN);

    /* FIXED ME !!
     *
     * Here refer the transport_send_check_condition_and_sense() and only handle
     * the error code about the 3rd-party copy command that originated the
     * copy operation.
     */
    switch(gErrReasonTable[err].err_reason){
    case TCM_NON_EXISTENT_LUN:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x25;
        break;

    case TCM_UNSUPPORTED_SCSI_OPCODE:
    case TCM_SECTOR_COUNT_TOO_MANY:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x20;
        break;

    case TCM_CHECK_CONDITION_NOT_READY:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = NOT_READY;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = asc;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = ascq;
        break;

    case TCM_CHECK_CONDITION_ABORT_CMD:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ABORTED_COMMAND;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x29;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x03;
        break;

    case TCM_INCORRECT_AMOUNT_OF_DATA:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ABORTED_COMMAND;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0c;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x0d;
        break;

    case TCM_INVALID_CDB_FIELD:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x24;
        break;

    case TCM_INVALID_PARAMETER_LIST:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x26;
        break;

    case TCM_UNEXPECTED_UNSOLICITED_DATA:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ABORTED_COMMAND;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0c;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x0c;
		break;

    case TCM_WRITE_PROTECTED:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = DATA_PROTECT;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x27;
        break;

    case TCM_ADDRESS_OUT_OF_RANGE:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x21;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x00;
        break;

    case TCM_PARAMETER_LIST_LEN_ERROR:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x1A;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x00;
        break;

    case TCM_UNREACHABLE_COPY_TARGET:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = COPY_ABORTED;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x08;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x04;
        break;

    case TCM_3RD_PARTY_DEVICE_FAILURE:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = COPY_ABORTED;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0D;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x01;
        break;

    case TCM_INCORRECT_COPY_TARGET_DEV_TYPE:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = COPY_ABORTED;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0D;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x03;
        break;

    case TCM_TOO_MANY_TARGET_DESCRIPTORS:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x26;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x06;
        break;

    case TCM_TOO_MANY_SEGMENT_DESCRIPTORS:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x26;
        buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x08;
        break;

    case TCM_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x05;
        break;

    case TCM_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET:
		buffer[offset]                          = 0x70;
		buffer[offset]                          |= 0x80;

        /* Information field shall be zero */
        buffer[offset+3] = 0;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x04;
        break;

    case TCM_COPY_ABORT_DATA_OVERRUN_COPY_TARGET:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = COPY_ABORTED;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x05;
        break;

    case TCM_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET:
		buffer[offset]                          = 0x70;
		buffer[offset]                          |= 0x80;

        /* FIXED ME
         *
         * Information field only has 4 bytes, but the transfer count filed
         * in specfification is 8 bytes ... Need to check again
         */
        put_unaligned_be32(obj->transfer_count, &buffer[offset+3]);
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = COPY_ABORTED;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x04;
        break;

	case TCM_INSUFFICIENT_RESOURCES:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x55;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x03;
		break;

	case TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x55;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x0C;
		break;

	case TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x55;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x0D;
		break;

	case TCM_OPERATION_IN_PROGRESS:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x00;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x16;
		break;

    case TCM_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x0A;
        break;

    case TCM_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x00;
        break;

    case TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x03;
        break;

    case TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x02;
        break;

    case TCM_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x08;
        break;

    case TCM_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x05;
        break;

    case TCM_INVALID_TOKEN_OP_AND_TOKEN_DELETED:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x09;
        break;

    case TCM_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x07;
        break;

    case TCM_INVALID_TOKEN_OP_AND_TOKEN_REVOKED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x06;
        break;

    case TCM_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x04;
        break;

    case TCM_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE:
		buffer[offset]                          = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]      = 0x01;
        break;


#if defined(SUPPORT_TP)

	case TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT:
		/* CURRENT ERROR */
		buffer[offset]				= 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;

		/* DATA_PROTECT */
		buffer[offset+SPC_SENSE_KEY_OFFSET]	= DATA_PROTECT;
		buffer[offset+SPC_ASC_KEY_OFFSET]	= 0x27;
		buffer[offset+SPC_ASCQ_KEY_OFFSET]	= 0x07;
	break;
#endif
    case TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE:
    default:
        buffer[offset]                          = 0x70;
        buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
        buffer[offset+SPC_SENSE_KEY_OFFSET]     = ILLEGAL_REQUEST;
        buffer[offset+SPC_ASC_KEY_OFFSET]       = 0x80;
        break;
    }

    return;
}


void __tpc_obj_node_lock_add(
	IN TPC_OBJ *tpc_obj
	)
{
	if (tpc_obj == NULL)
		BUG_ON(TRUE);

	spin_lock(&tpc_obj->se_tpg->tpc_obj_list_lock);
	__tpc_obj_node_add(tpc_obj);
	spin_unlock(&tpc_obj->se_tpg->tpc_obj_list_lock);
	return;
}

void __tpc_obj_node_add(
    IN TPC_OBJ *tpc_obj
    )
{

    if (tpc_obj == NULL)
        BUG_ON(TRUE);

    list_add_tail(&tpc_obj->o_node, &tpc_obj->se_tpg->tpc_obj_list);
    atomic_inc(&tpc_obj->se_tpg->tpc_obj_count);

    DBG_ROD_PRINT("to add obj node(id:0x%x, op_sac:0x%x), obj_count:0x%x\n",
                tpc_obj->list_id, tpc_obj->op_sac, 
                atomic_read(&tpc_obj->se_tpg->tpc_obj_count));

    return;
}

void __tpc_obj_node_del(
    IN TPC_OBJ *obj
    )
{
	/* Take care to lock before to call this */
	if (!list_empty(&obj->o_node)){
		list_del_init(&obj->o_node);
		atomic_dec(&obj->se_tpg->tpc_obj_count);

		DBG_ROD_PRINT("to del obj node(id:0x%x, op_sac:0x%x), obj_count:0x%x\n",
			obj->list_id, obj->op_sac, 
			atomic_read(&obj->se_tpg->tpc_obj_count));
	}
	return;

}

void __tpc_obj_ref_count_lock_dec(
	IN TPC_OBJ *obj
	)
{
	spin_lock(&obj->o_ref_count_lock);
	if (atomic_read(&obj->o_ref_count))
		atomic_dec(&obj->o_ref_count);
	spin_unlock(&obj->o_ref_count_lock);
	return;
}

void __tpc_obj_ref_count_lock_inc(
	IN TPC_OBJ *obj
	)
{
	spin_lock(&obj->o_ref_count_lock);
	atomic_inc(&obj->o_ref_count);
	spin_unlock(&obj->o_ref_count_lock);
	return;
}

void __tpc_token_ref_count_lock_inc(
	IN TPC_OBJ *obj
	)
{
	spin_lock(&obj->o_token_ref_count_lock);
	atomic_inc(&obj->o_token_ref_count);
	spin_unlock(&obj->o_token_ref_count_lock);
	return;
}

void __tpc_token_ref_count_lock_dec(
	IN TPC_OBJ *obj
	)
{
	spin_lock(&obj->o_token_ref_count_lock);
	if (atomic_read(&obj->o_token_ref_count))
		atomic_dec(&obj->o_token_ref_count);
	spin_unlock(&obj->o_token_ref_count_lock);
	return;
}


static void __tpc_track_node_add(
    IN LIO_SE_CMD *se_cmd,
    IN LIO_SE_PORTAL_GROUP *se_tpg
    )
{
    list_add_tail(&se_cmd->t_cmd_node, &se_tpg->tpc_cmd_track_list);     
    atomic_inc(&se_tpg->tpc_track_count);

    DBG_ROD_PRINT("add track node (id:0x%x, op_sac:0x%x), track_count:0x%x\n",
                        se_cmd->t_list_id, se_cmd->t_op_sac, 
                        atomic_read(&se_tpg->tpc_track_count));
    return;
}

static void __tpc_track_node_del(
    IN LIO_SE_CMD *se_cmd,
    IN LIO_SE_PORTAL_GROUP *se_tpg
    )
{
    if (!list_empty(&se_cmd->t_cmd_node)){

        list_del_init(&se_cmd->t_cmd_node);
        atomic_dec(&se_tpg->tpc_track_count);

        DBG_ROD_PRINT("del track node(id:0x%x, op_sac:0x%x), track_count:0x%x\n", 
               se_cmd->t_list_id, se_cmd->t_op_sac,  
               atomic_read(&se_tpg->tpc_track_count));

    }
    return;
}

static void __tpc_track_node_lock_add(
    IN LIO_SE_CMD *se_cmd
    )
{
    LIO_SE_DEVICE *se_dev = NULL;
    LIO_SE_PORTAL_GROUP *se_tpg = NULL;
    unsigned long flags = 0;

    /**/
    if (!se_cmd->se_lun)
        return;

    se_dev = se_cmd->se_lun->lun_se_dev;
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&se_cmd->se_lun->lun_sep_lock);

    if (se_cmd->se_lun->lun_sep){
        if (!se_cmd->se_lun->lun_sep->sep_tpg)
            goto _EXIT_;

        se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;

        spin_lock(&se_tpg->tpc_cmd_track_list_lock);
        __tpc_track_node_add(se_cmd, se_tpg);
        spin_unlock(&se_tpg->tpc_cmd_track_list_lock);
    }

_EXIT_:
    spin_unlock(&se_cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
    return;
}

static void __tpc_track_node_lock_del(
    IN LIO_SE_CMD *se_cmd
    )
{
    LIO_SE_DEVICE *se_dev = NULL;
    LIO_SE_PORTAL_GROUP *se_tpg = NULL;
    unsigned long flags = 0;

    /**/
    if (!se_cmd->se_lun)
        return;

    se_dev = se_cmd->se_lun->lun_se_dev;
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&se_cmd->se_lun->lun_sep_lock);

    if (se_cmd->se_lun->lun_sep){
        if (!se_cmd->se_lun->lun_sep->sep_tpg)
            goto _EXIT_;

        se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;

        spin_lock(&se_tpg->tpc_cmd_track_list_lock);
        __tpc_track_node_del(se_cmd, se_tpg);
        spin_unlock(&se_tpg->tpc_cmd_track_list_lock);

    }
_EXIT_:
    spin_unlock(&se_cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
    return;
}


/* 2014/08/17, adamhsu, redmine 9007 */
void __tpc_td_ref_count_lock_inc(
	IN TPC_TRACK_DATA *td
	)
{
	unsigned long flags;

	/* 2014/08/17, adamhsu, redmine 9007 */
	spin_lock_irqsave(&td->t_ref_count_lock, flags);
	atomic_inc(&td->t_ref_count);
	spin_unlock_irqrestore(&td->t_ref_count_lock, flags);
	return;
}


/* 2014/08/17, adamhsu, redmine 9007 */
void __tpc_td_ref_count_lock_dec(
	IN TPC_TRACK_DATA *td
	)
{
	unsigned long flags;

	/* 2014/08/17, adamhsu, redmine 9007 */
	spin_lock_irqsave(&td->t_ref_count_lock, flags);
	if (atomic_read(&td->t_ref_count))
		atomic_dec(&td->t_ref_count);

	spin_unlock_irqrestore(&td->t_ref_count_lock, flags);		
	return;
}


void __tpc_update_t_cmd_transfer_count(
	IN TPC_TRACK_DATA *data,
	IN sector_t nr_blks
	)
{
	unsigned long flags;

	spin_lock_irqsave(&data->t_count_lock, flags);
	data->t_transfer_count += (u64)(nr_blks);
	spin_unlock_irqrestore(&data->t_count_lock, flags);
	return;
}

sector_t __tpc_get_t_cmd_transfer_count(
    IN TPC_TRACK_DATA *data
    )
{
    unsigned long flags = 0;
    sector_t nr_blks = 0;

    spin_lock_irqsave(&data->t_count_lock, flags);
    nr_blks = (sector_t)data->t_transfer_count;
    spin_unlock_irqrestore(&data->t_count_lock, flags);
    return nr_blks;
}

void __tpc_update_t_cmd_status(
    IN TPC_TRACK_DATA *data,
    IN T_CMD_STATUS status
    )
{
    unsigned long flags = 0;

    BUG_ON(!data);

    spin_lock_irqsave(&data->t_status_lock, flags);
    data->t_cmd_status = status;
    spin_unlock_irqrestore(&data->t_status_lock, flags);    
    return;
}

T_CMD_STATUS __tpc_get_t_cmd_status(
    IN TPC_TRACK_DATA *data
    )
{
    unsigned long flags = 0;
    T_CMD_STATUS status;

    BUG_ON(!data);

    spin_lock_irqsave(&data->t_status_lock, flags);
    status = data->t_cmd_status;
    spin_unlock_irqrestore(&data->t_status_lock, flags);   
    return status;
}

static void __create_id_cscd_desc(
    IN LIO_SE_LUN *se_lun, 
    IN LIO_SE_DEVICE *se_dev, 
    IN TPC_OBJ *obj,
    IN ID_CSCD_DESC *data
    )
{
	u32 blk_size = 0;

	if (!se_lun || !se_dev || !data)
		return;

	data->GenCSCDHdr.DescTypeCode = ID_DESC;
	data->GenCSCDHdr.DevType      = se_dev->transport->get_device_type(se_dev);

	/* this will be ignored in ID CSCD desc, SPC4R36, page 288 */
	data->GenCSCDHdr.LuIdType = 0;

	/* Refer the target_emulate_evpd_83() to crete initiatior port identifier field value */
	if (se_lun->lun_sep)
		put_unaligned_be16(se_lun->lun_sep->sep_rtpi, 
			&data->InitiatorPortIdentifier[0]);
    
	data->CodeSet = 0x1; // binary
	data->DesignatorType = 0x3; // NAA identifier
	data->Assoc = 0x0; // addressed logical unit

	/* We build the 16 bytes for DESIGNATOR data only */
	data->DesignatorLen = 0x10;

	__make_target_naa_6h_hdr(se_dev, &data->Designator[0]);
	__get_target_parse_naa_6h_vendor_specific(se_dev, &data->Designator[3]);

	blk_size = se_dev->se_sub_dev->se_dev_attrib.block_size;
	data->DevTypeSpecificParam.BlockDevTypes.DiskBlkLen[0] = (u8)(blk_size >> 16);
	data->DevTypeSpecificParam.BlockDevTypes.DiskBlkLen[1] = (u8)(blk_size >> 8);
	data->DevTypeSpecificParam.BlockDevTypes.DiskBlkLen[2] = (u8)blk_size;
	return;

}

void __tpc_free_obj_node_data(
    IN struct list_head *data_list
    )
{
    BLK_RANGE_DATA *br = NULL, *tmp_br = NULL;

    if (!data_list)
        return;

    list_for_each_entry_safe(br, tmp_br, data_list, b_range_data_node)
        kfree(br);
    return;
}

void __tpc_build_512b_token_data(
	IN LIO_SE_CMD *se_cmd,
	IN TPC_OBJ *obj
	)
{
	ROD_TOKEN_512B *token_512b = NULL;
	u32 rod_type = 0;

	BUG_ON(!obj->o_token_data);

	/* NOTE:
	 * The token type (byte 0 ~ byte 3) and token size (byte 6 ~ byte 7)
	 * had been updated in __alloc_rod_token() already 
	 */
	token_512b = (ROD_TOKEN_512B *)obj->o_token_data;

	/* byte 8 ~ byte 15 (copy manager rod token identifier) */
	__create_cm_rod_token_id_in_rod_token(se_cmd, obj,  
		&token_512b->gen_data.cm_rod_token_id[0]
		);

	/* byte 16 ~ byte 47 (creator logical unit descriptor, 
	 * SPC4R36, page 249) 
	 */
	__create_id_cscd_desc(se_cmd->se_lun, se_cmd->se_lun->lun_se_dev, obj, 
		(ID_CSCD_DESC *)&token_512b->gen_data.cr_lu_desc[0]
		);

	/* byte 48 ~ byte 63 (16 bytes)(number of bytes represented) */
	/* FIXED ME !! we shall take care the case about 64bit x 32bit. */
	put_unaligned_be64(0, &token_512b->gen_data.nr_represented_bytes[0]);
	put_unaligned_be64((obj->transfer_count << obj->dev_bs_order),
		&token_512b->gen_data.nr_represented_bytes[8]
		);

	/* byte 64 ~ byte 95 (rod token type specific data) */
	rod_type = get_unaligned_be32(&token_512b->gen_data.type[0]);
	if (rod_type == ROD_TYPE_SPECIFICED_BY_ROD_TOKEN){
		__create_byte64_95(se_cmd, obj,
			&token_512b->gen_data.byte64_95.rod_token_specific_data[0]
			);
	}

	/* byte 96 ~ byte 127,
	 *
	 * device type specific data is specified by the command standard for
	 * the peripheral device type indicated by the CREATOR LOGICAL UNIT
	 * DESCRIPTOR (e.g. for peripheral device type 00h, see SBC-3) */
	__create_dev_type_specific_data(se_cmd, obj, 
		&token_512b->gen_data.dev_type_data[0]
		);

	__create_512b_rod_token_after_byte128(se_cmd, obj, token_512b);
	return;
}

u32 __get_min_rod_token_info_param_len()
{
    /* SPC4R36 , page 430 */
    return (sizeof(ROD_TOKEN_INFO_PARAM) + ROD_SENSE_DATA_LEN + 4 \
            + (ROD_TOKEN_MIN_SIZE + 2));
}

BLK_RANGE_DATA * __create_blk_range()
{
    BLK_RANGE_DATA *blk_range = NULL;

    blk_range = kzalloc(sizeof(BLK_RANGE_DATA), GFP_KERNEL);
    if (!blk_range)
        return NULL;

    INIT_LIST_HEAD(&blk_range->b_range_data_node);
    blk_range->curr_status = R_STATUS_NOT_USED;
    blk_range->next_status = R_STATUS_NOT_USED;
    return blk_range;
}

static sector_t __get_total_nr_blks_by_s_obj(
    IN struct list_head *br_list,
    IN bool skip_truncate
    )
{
    BLK_RANGE_DATA *br = NULL;
    sector_t nr_blks = 0;

    /**/
    list_for_each_entry(br, br_list, b_range_data_node){

        DBG_ROD_PRINT("%s: skip_truncate:0x%x, br->curr_status:0x%x\n",
            __func__, skip_truncate, br->curr_status);

        if(skip_truncate && br->curr_status == R_STATUS_TRUNCATE_USED)
            continue;
        nr_blks += br->nr_blks;
    }
    return nr_blks;
}

sector_t __tpc_get_nr_blks_by_s_obj(
	IN TPC_OBJ *obj,
	IN bool skip_truncate
	)    
{
	sector_t nr_blks = 0;

	spin_lock(&obj->o_data_lock);
	nr_blks  = __get_total_nr_blks_by_s_obj(&obj->o_data_list, skip_truncate);
	spin_unlock(&obj->o_data_lock);
	return nr_blks;
}

u64 __tpc_get_nr_bytes_by_s_obj(
	IN TPC_OBJ *obj,
	IN bool skip_truncate
	)
{
	u64 nr_bytes = 0;

	spin_lock(&obj->o_data_lock);
	nr_bytes = (u64)__get_total_nr_blks_by_s_obj(&obj->o_data_list, skip_truncate);
	nr_bytes <<= obj->dev_bs_order;
	spin_unlock(&obj->o_data_lock);

	return nr_bytes;
}

void tpc_free_track_data(
	IN LIO_SE_CMD *se_cmd
	)
{
	TPC_TRACK_DATA *td = NULL;
	LIO_SE_DEVICE *se_dev = NULL;
	LIO_SE_PORTAL_GROUP *se_tpg = NULL;
	unsigned long flags = 0;

	/**/
	if (!se_cmd->is_tpc || !se_cmd->se_lun)
		return;

	se_dev = se_cmd->se_lun->lun_se_dev;
	spin_lock_irqsave(&se_dev->se_port_lock, flags);
	spin_lock(&se_cmd->se_lun->lun_sep_lock);

	if (!se_cmd->se_lun->lun_sep){
		spin_unlock(&se_cmd->se_lun->lun_sep_lock);
		spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
		return;
	}

	if (!se_cmd->se_lun->lun_sep->sep_tpg){
		spin_unlock(&se_cmd->se_lun->lun_sep_lock);
		spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
		return;
	}

	/**/
	se_tpg = se_cmd->se_lun->lun_sep->sep_tpg;
	spin_lock(&se_tpg->tpc_cmd_track_list_lock);
	if (list_empty(&se_cmd->t_cmd_node)){
		spin_unlock(&se_tpg->tpc_cmd_track_list_lock);
		spin_unlock(&se_cmd->se_lun->lun_sep_lock);
		spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
		return;
	}

	/* To del node to avoid someone will use it by tpc_cmd_track_list */
	__tpc_track_node_del(se_cmd, se_tpg);

	spin_unlock(&se_tpg->tpc_cmd_track_list_lock);
	spin_unlock(&se_cmd->se_lun->lun_sep_lock);
	spin_unlock_irqrestore(&se_dev->se_port_lock, flags);

	/* To free track data if necessary */
	if (se_cmd->t_track_rec){
		td = (TPC_TRACK_DATA *)se_cmd->t_track_rec;
		pr_debug("%s: found track data, start to free it\n", __func__);

		/* to wait for complete if someone still uses the track data now */
		/* 2014/08/17, adamhsu, redmine 9007 */
		do {
			spin_lock_irqsave(&td->t_ref_count_lock, flags);
			if (!atomic_read(&td->t_ref_count)){
				spin_unlock_irqrestore(&td->t_ref_count_lock, flags);
				break;
			}
			spin_unlock_irqrestore(&td->t_ref_count_lock, flags);

			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(msecs_to_jiffies(1*1000));
		} while (1);

		kfree(se_cmd->t_track_rec);
		pr_debug("%s: finish free track data\n", __func__);
	}

	if (se_cmd->t_tiqn)
		kfree(se_cmd->t_tiqn);

	se_cmd->is_tpc = 0;
	return;
}
EXPORT_SYMBOL(tpc_free_track_data);


/*
 * @fn int tpc_is_in_progress (IN LIO_SE_CMD *cmd)
 *
 * @brief To check whether the same 3rd-party copy command is in progress or not
 * @note
 * @param[in] cmd
 * @retval 0  - 3rd-party copy cmd is in progress
 * @retval 1  - 3rd-party copy cmd is not in progress
 * @retval -1 - not 3rd-party copy command or other err after call this function
 */
int tpc_is_in_progress(
    IN LIO_SE_CMD *cmd
    )
{
    TPC_TRACK_DATA *n_td = NULL;
    TPC_CMD_TYPE type = MAX_TPC_CMD_TYPE;
    TPC_LID status = MAX_TP_LID;
    LIO_SE_DEVICE *se_dev = NULL;
    LIO_SE_CMD *o_cmd = NULL, *tmp_cmd = NULL;
    LIO_SE_PORTAL_GROUP *se_tpg = NULL;
    unsigned long flags = 0;
    u8 *cdb = NULL;

    /**/
    if (!cmd->se_lun)
        return -1;

    /* step1: Try to check tpc command type and its list id value first */
    type = tpc_get_type(cmd);
    if (!(IS_TPC_CMD_TYPE(type)))
        return -1;

    status = tpc_get_list_id(cmd, &cmd->t_list_id);
    if ((status == TPC_LID_NOT_CP_CMD) || (status == MAX_TP_LID))
        return -1;

    /* step2: Try get information first */
    if((cmd->t_tiqn = kzalloc(TARGET_IQN_NAME_LEN, GFP_KERNEL)) == NULL)
        return -1;

    if((__tpc_get_isid_by_se_cmd(cmd, PR_REG_ISID_LEN, cmd->t_isid))
    || (__tpc_get_tiqn_and_pg_tag_by_se_cmd(cmd, cmd->t_tiqn, &cmd->t_pg_tag))
    )
    {
        if(cmd->t_tiqn)
            kfree(cmd->t_tiqn);
        return -1;
    }

    /* step3: Now, to process the 3rd-party copy command */
    se_dev        = cmd->se_lun->lun_se_dev;
    cdb           = CMD_TO_TASK_CDB(cmd)->t_task_cdb;
    cmd->t_op_sac = (((u16)cdb[0] << 8) | (cdb[1] & 0x1f));       

    /* FIXED ME !! */
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&cmd->se_lun->lun_sep_lock);

    if (!cmd->se_lun->lun_sep){
        spin_unlock(&cmd->se_lun->lun_sep_lock);
        spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
        DBG_ROD_PRINT("%s: lun_sep is null\n", __func__);
        return -1;
    }

    if (!cmd->se_lun->lun_sep->sep_tpg){
        spin_unlock(&cmd->se_lun->lun_sep_lock);
        spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
        DBG_ROD_PRINT("%s: se_tpg is null\n", __func__);
        return -1;
    }

    /* search all tpc cmd track list in this portal group */
    se_tpg = cmd->se_lun->lun_sep->sep_tpg;
    spin_lock(&se_tpg->tpc_cmd_track_list_lock);    

    list_for_each_entry_safe(o_cmd, tmp_cmd, &se_tpg->tpc_cmd_track_list, t_cmd_node){
        /* 
         * For example, the list id of current passing extended copy (LID4) 
         * command can't be the same as previous extended copy (LID4) command
         * in the tpc_cmd_track_list. But, the list id of copy abort command
         * or other monitor / management command can be the same as previous
         * extended copy (LID4) command in the list.
         */
        if ((SAME_ID_OP_SAC(o_cmd->t_list_id, cmd->t_list_id, o_cmd->t_op_sac, cmd->t_op_sac))
        &&  (!(__tpc_is_same_i_t_nexus_func2(o_cmd, cmd)))
        )
        {
            spin_unlock(&se_tpg->tpc_cmd_track_list_lock);
            spin_unlock(&cmd->se_lun->lun_sep_lock);
            spin_unlock_irqrestore(&se_dev->se_port_lock, flags);

            /* If found duplicated command, current command will be abort by
             * CHECK CONDITION.
             */
            pr_err("warning: found same tpc cmd (id:0x%x, op_sac:0x%x) is "
                    "in progress, to abort new one\n", 
                    cmd->t_list_id, cmd->t_op_sac);

            /* 0: cureent tpc command is in progress */
            return 0;
        }
    }
    spin_unlock(&se_tpg->tpc_cmd_track_list_lock);
    spin_unlock(&cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);

    /* step4: If not found any duplicated command ... */
    if (type == TPC_CP_OP){
        if ((n_td = __tpc_alloc_track_data()) == NULL){
            pr_err("fail to alloc track data\n");

            if (cmd->t_tiqn)
                kfree(cmd->t_tiqn);
           /* -1: not tpc command or fatal error after call this func */
            return -1;
        }
        cmd->t_track_rec = (void *)n_td;
    }

    /* everything is fine ... */
    DBG_ROD_PRINT("cmd type:0x%x, id:0x%x, op_sac:0x%x, t_track_rec:0x%p\n",
                type, cmd->t_list_id, cmd->t_op_sac, cmd->t_track_rec);

    cmd->is_tpc = 1;
    __tpc_track_node_lock_add(cmd);

    /* 1: tpc command is not in progress */
    return 1;

}
EXPORT_SYMBOL(tpc_is_in_progress);


/*
 * @fn int tpc_get_rod_token_source(IN ROD_CSCD_DESC *desc, IN OUT ROD_TOKEN_SRC *pTokenSrc)
 * @brief
 *
 * @sa 
 * @param[in] desc
 * @param[in] pTokenSrc
 * @retval 0 - success; 1 - fail
 */
int tpc_get_rod_token_source(
    IN ROD_CSCD_DESC *desc,
    IN OUT ROD_TOKEN_SRC *pTokenSrc
    )
{
    if(desc == NULL || pTokenSrc == NULL)
        return 1;

    /* FIXED ME !! FIXED ME !! FIXED ME !! */
    *pTokenSrc = TOKEN_FROM_SAME_CPMGR_IN_SAME_SCSI_TARGET;
    return 0;

}

void __tpc_update_obj_transfer_count_lock(
    IN TPC_OBJ *obj, 
    IN u64 count
    )
{
    unsigned long flags = 0;

    spin_lock_irqsave(&obj->o_transfer_count_lock, flags);
    obj->transfer_count = count;
    spin_unlock_irqrestore(&obj->o_transfer_count_lock, flags);
    return;
}

void __tpc_update_obj_status_lock(
	IN TPC_OBJ *obj, 
	IN OBJ_STS status
	)
{

	spin_lock(&obj->o_status_lock);
	obj->o_status = status;
	spin_unlock(&obj->o_status_lock);
	return;
}

void __tpc_update_obj_token_status_lock(
	IN TPC_OBJ *obj, 
	IN OBJ_TOKEN_STS status
	)
{
	spin_lock(&obj->o_token_status_lock);
	obj->o_token_status = status;
	spin_unlock(&obj->o_token_status_lock);
	return;
}

int __tpc_is_token_cancelled(
    IN OBJ_TOKEN_STS token_status
    )
{
    if (TOKEN_STS_CANCELLED(token_status))
        return 0;

    return 1;
}

int __tpc_is_token_expired(
    IN OBJ_TOKEN_STS token_status
    )
{
    if (TOKEN_STS_EXPIRED(token_status)
    || TOKEN_STS_FREE_BY_TOKEN_TIMER(token_status)
    )
    {
        return 0;
    }
    return 1;
}

int __tpc_is_token_deleted(
    IN OBJ_TOKEN_STS token_status
    )
{
    if (TOKEN_STS_DELETED(token_status)
    || TOKEN_STS_FREE_BY_PROC(token_status)
    )
    {
        return 0;
    }
    return 1;
}

int __tpc_is_token_invalid(
	IN TPC_OBJ *obj,
	IN OUT ERR_REASON_INDEX *err
	)
{
	int ret = 1;

	/* before call this, must be make sure the lock and necessary
	 * ref count had been all setup already
	 */
	spin_lock(&obj->o_status_lock);
	if (!OBJ_STS_ALIVE(obj->o_status)){
		spin_unlock(&obj->o_status_lock);
		*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED;
		ret = -1;
		goto _EXIT_;
	}
	spin_unlock(&obj->o_status_lock);

	/* obj is alive ... */
	spin_lock(&obj->o_token_status_lock);

	if (!(__tpc_is_token_cancelled(obj->o_token_status)))
		*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED;
	else if (!(__tpc_is_token_expired(obj->o_token_status)))
		*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED;
	else if (!(__tpc_is_token_deleted(obj->o_token_status)))
		*err = ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED;
	else{
		ret = 0;
		DBG_ROD_PRINT("%s: both obj and token are alive ... \n", __func__);
	}

	 spin_unlock(&obj->o_token_status_lock);

_EXIT_:
	return ret;
}

OBJ_STS __tpc_get_obj_status_lock(
	IN TPC_OBJ *obj
	)
{
	OBJ_STS obj_status;

	spin_lock(&obj->o_status_lock);
	obj_status = obj->o_status;
	spin_unlock(&obj->o_status_lock);
	return obj_status;
}

OBJ_TOKEN_STS __tpc_get_obj_token_status_lock(
	IN TPC_OBJ *obj
	)
{
	OBJ_TOKEN_STS token_status;

	spin_lock(&obj->o_token_status_lock);
	token_status = obj->o_token_status;
	spin_unlock(&obj->o_token_status_lock);
	return token_status;
}

/*
 * @fn int __tpc_do_rw (IN GEN_RW_TASK *task)
 *
 * @brief Wrapper function to do r/w for file i/o or block i/o
 * @note
 * @param[in] task
 * @retval -1 or depend on the result of __do_b_rw() or __do_f_rw()
 */
int __tpc_do_rw(
	GEN_RW_TASK *task
	)
{
	int ret = -1;
	SUBSYSTEM_TYPE subsys_type;

	if (!task)
		return ret;

	/* try get the device backend type for this taks */
	if (__do_get_subsystem_dev_type(task->se_dev, &subsys_type))
		return ret;

	if (subsys_type == SUBSYSTEM_BLOCK)
		return __do_b_rw(task);
	else if (subsys_type == SUBSYSTEM_FILE)
		return __do_f_rw(task);

	return -1;
}


/*
 * @fn static int __tpc_do_b_zero_w (IN GEN_RW_TASK *task)
 *
 * @brief Function to do zero write for block i/o
 * @note
 * @param[in] task
 * @retval < 0  - Error during call this function
 *         >= 0 - Real write byte counts
 */
static int __tpc_do_b_zero_w(
	IN GEN_RW_TASK *task
	)
{
	LIO_IBLOCK_DEV *ib_dev = NULL; 
	LIO_FD_DEV *fd_dev = NULL;
	struct block_device *bd = NULL;
	sector_t block_lba = 0, t_lba= 0;
	u32 expected_blks = 0, real_w_blks = 0, done = 0, t_range = 0;
	u32 d_bs_order = 0;
	int ret = 0;

	/* 20140630, adamhsu, redmine 8826 (start) */
	ib_dev = (LIO_IBLOCK_DEV *)task->se_dev->dev_ptr;
	bd = ib_dev->ibd_bd;

	block_lba = task->lba;
	d_bs_order = task->dev_bs_order;


	if (task->s_nr_blks)
		expected_blks = task->s_nr_blks;
	else
		expected_blks = task->nr_blks;

	while (expected_blks){

		t_range = real_w_blks = min_t(u32, expected_blks, 
			(OPTIMAL_TRANSFER_SIZE_IN_BYTES >> d_bs_order));

		t_lba = block_lba;

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)

		ret =  __blkio_transfer_task_lba_to_block_lba(
			(1 << d_bs_order), &t_lba);
		if (ret != 0)
			break;

		t_range = (real_w_blks * ((1 << d_bs_order) >> 9));
#endif

		ret = blkdev_issue_special_discard(bd, t_lba, t_range, 
			GFP_KERNEL, 0);

		if (unlikely(ret)) {
			pr_err("%s: fail to exec blkdev_issue_special_discard "
				"ret:0x%x\n", __FUNCTION__, ret);
			break;
		}

		/* To check if timeout happens before to submit */
		if (IS_TIMEOUT(task->timeout_jiffies)){
			task->is_timeout = 1;
			break;
		}

		done += real_w_blks;
		block_lba += real_w_blks;
		expected_blks -= real_w_blks;

		pr_debug("%s: block_lba:0x%llx, done blks:0x%x, "
			"expected_blks:0x%x\n", __FUNCTION__, 
			(unsigned long long)block_lba, done, expected_blks);

	}
	/* 20140630, adamhsu, redmine 8826 (end) */

	if (task->is_timeout){
		pr_err("%s: jiffies > cmd expected-timeout value\n", 
			__FUNCTION__);
		return -1;
	}
	return done;
}

#if defined(SUPPORT_FILEIO_ON_FILE)
/* 20140630, adamhsu, redmine 8826 */
static int __tpc_fio_filebackend_zero_rod_token(
	struct file *fd,
	loff_t off,
	loff_t len
	)
{
	int ret;

	if (!fd->f_op->fallocate){
		pr_err("%s: not support fallocate()\n", __FUNCTION__);
		return -EOPNOTSUPP;
	}
	
	pr_debug("%s: off:0x%llx, len:0x%llx\n", __FUNCTION__,
			(unsigned long long)off, (unsigned long long)len);
	
	ret = fd->f_op->fallocate(fd, 
		(FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE), off, len);
	
	if (unlikely(ret != 0)){
		pr_err("%s: ret:%d after call fallocate "
			"w/ punch hole + keep size\n", __FUNCTION__, ret);
		goto _EXIT_;
	}

	ret = fd->f_op->fallocate(fd, (FALLOC_FL_KEEP_SIZE), off, len);

	if (unlikely(ret != 0)){
		pr_err("%s: ret:%d after call fallocate op "
			"w/ keep size\n", __FUNCTION__, ret);
		goto _EXIT_;
	}

	ret = 0;
_EXIT_:	
	
	return ret;

}

#else
static int __tpc_fio_filebackend_zero_rod_token(
	struct file *fd,
	loff_t off,
	loff_t len
	)
{
	pr_err("%s: not support zero rod token\n", __FUNCTION__);
	return -EOPNOTSUPP;
}
#endif

/*
 * @fn static int __tpc_do_f_zero_w (IN GEN_RW_TASK *task)
 *
 * @brief Function to do zero write for file i/o
 * @note
 * @param[in] task
 * @retval -1 or depend on the result of __tpc_do_b_zero_w()
 */
static int __tpc_do_f_zero_w(
	IN GEN_RW_TASK *task
	)
{
	LIO_FD_DEV *fd_dev = NULL;
	struct inode *inode = NULL;
	int ret;

#if defined(SUPPORT_TP)
	int err_1;
#endif
	/* 20140630, adamhsu, redmine 8826 (start) */
#define DEFAULT_ALIGN_SIZE	(1 << 20)
#define NORMAL_IO_TIMEOUT	(5)	/* unit is second */

	struct address_space *mapping = NULL;
	loff_t off, len;
	loff_t first_page = 0, last_page = 0;
	loff_t first_page_offset = 0, last_page_offset = 0;
	ALIGN_DESC align_desc;
	int normal_io = 1;
	sector_t lba, t_lba;
	u32 nr_blks, t_nr_blks, real_range, bs_order, tmp, done = 0;
	GEN_RW_TASK tmp_w_task;
	/* 20140630, adamhsu, redmine 8826 (end) */

	fd_dev = (LIO_FD_DEV *)task->se_dev->dev_ptr;
	inode  = fd_dev->fd_file->f_mapping->host;
	mapping = inode->i_mapping;

	/* 20140630, adamhsu, redmine 8826 (start) */
	lba = task->lba;
	if (task->s_nr_blks)
		nr_blks = task->s_nr_blks;
	else
		nr_blks = task->nr_blks;


	bs_order = task->dev_bs_order;

	if (!S_ISBLK(inode->i_mode)){
		/* The path is for LIO file i/o + file backend (static volume) */
		off = (loff_t)((loff_t)lba << bs_order);
		len = (loff_t)((loff_t)nr_blks << bs_order);
		
		ret = __tpc_fio_filebackend_zero_rod_token(fd_dev->fd_file, 
				off, len);
		if (ret == 0)
			done += nr_blks;
		else
			done = ret;

		task->ret_code = ret;
		return done;
	}

	/* The path is for LIO file i/o + block backend device */
	__create_aligned_range_desc(&align_desc, lba, nr_blks, 
		bs_order, DEFAULT_ALIGN_SIZE);

	while (nr_blks > 0) {

		t_lba = lba;
		t_nr_blks = real_range = min_t(sector_t, nr_blks, UINT_MAX);

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

		pr_debug("%s: normal_io:%d, lba:0x%llx, real_range:0x%x\n", 
				__FUNCTION__, normal_io, 
				(unsigned long long)lba,(u32)real_range);
	
		if (unlikely(normal_io))
			goto _NORMAL_IO_;

		off = (loff_t)(lba << bs_order);
		len = (loff_t)(real_range << bs_order);
					
		first_page = (off) >> PAGE_CACHE_SHIFT;
		last_page = (off + len - 1) >> PAGE_CACHE_SHIFT;	
		first_page_offset = first_page	<< PAGE_CACHE_SHIFT;
		last_page_offset = (last_page << PAGE_CACHE_SHIFT) + (PAGE_CACHE_SIZE - 1);
					
		if (mapping->nrpages 
		&& mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)
		)
		{
			ret = filemap_write_and_wait_range(mapping, 
					first_page_offset, last_page_offset);
					
			if (unlikely(ret)) {
				pr_err("%s: fail to exec " 
					"filemap_write_and_wait_range(), "
					"ret:%d\n", __FUNCTION__, ret);

#if defined(SUPPORT_TP)
				if (!is_thin_lun(task->se_dev))
					break;

				if (ret != -ENOSPC) {
					err_1 = check_dm_thin_cond(inode->i_bdev);
					if (err_1 == -ENOSPC){
						pr_warn("%s: thin space was full\n", 
							__func__); 
						ret = err_1;
					}
				} else
					pr_warn("%s: thin space was full\n", 
						__func__); 

#endif
				break;
			}
		}


		truncate_pagecache_range(inode, first_page_offset, 
			last_page_offset);
			
		pr_debug("%s: off=%llu,len=%llu, first_page=%llu, "
				"last_page=%llu, first_page_offset=%llu, "
				"last_page_offset=%llu.\n", __FUNCTION__, 
				off, len, first_page, last_page, 
				first_page_offset, last_page_offset);
		
#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI) 
		ret = __blkio_transfer_task_lba_to_block_lba((1 << bs_order), 
				&t_lba);
		if (ret != 0)
			break;

		t_nr_blks = (real_range * ((1 << bs_order) >> 9));
#endif
		pr_debug("%s: t_lba:0x%llx, t_nr_blks:0x%x\n", __FUNCTION__,
				(unsigned long long)t_lba, (u32)t_nr_blks);

		ret = blkdev_issue_special_discard(inode->i_bdev, t_lba, 
				t_nr_blks, GFP_KERNEL, 0);

		if (unlikely(ret)) {
			pr_err("%s: fail to exec "
				"blkdev_issue_special_discard() ret:%d\n", 
				__FUNCTION__, ret);
			break;
		} 

		done += real_range;
		goto _GO_NEXT_;

_NORMAL_IO_:
		/* The path for real io */
		t_lba = lba;
		t_nr_blks = real_range;
		do {
			tmp = min_t(u32, t_nr_blks, task->nr_blks);

			memset(&tmp_w_task, 0, sizeof(GEN_RW_TASK));

			tmp_w_task.sg_list = task->sg_list;
			tmp_w_task.sg_nents = task->sg_nents;	

			__make_rw_task(&tmp_w_task, task->se_dev, t_lba, tmp,
				msecs_to_jiffies(NORMAL_IO_TIMEOUT * 1000), 
				DMA_TO_DEVICE);
	
			ret = __do_f_rw(&tmp_w_task);

			pr_debug("%s: after call __do_f_rw() ret:%d, "
				"is_timeout:%d, ret code:%d\n", 
				__FUNCTION__, ret, tmp_w_task.is_timeout, 
				tmp_w_task.ret_code);
	
			if((ret <= 0) || tmp_w_task.is_timeout 
			|| tmp_w_task.ret_code != 0){
				ret = tmp_w_task.ret_code;
				break;
			}

			ret = 0;
			done += tmp;
			t_lba += tmp;
			t_nr_blks -= tmp;
		} while (t_nr_blks);

		/* break the loop since while (t_nr_blks) didn't completed */
		if (t_nr_blks)
			break;
_GO_NEXT_:

		if (IS_TIMEOUT(task->timeout_jiffies)){
			task->is_timeout = 1;
			ret = -1;
			break;
		}
	
		lba += real_range;
		nr_blks -= real_range;
		pr_debug("%s: lba:0x%llx, done:0x%x, nr_blks:0x%x\n", 
			__FUNCTION__, (unsigned long long)lba, 
			done, nr_blks);

	}

	task->ret_code = ret;
	return done;

	/* 20140630, adamhsu, redmine 8826 (end) */

}


/*
 * @fn int __tpc_do_zero_rod_token_w (IN GEN_RW_TASK *task)
 *
 * @brief Wrapper function to do zero write for block i/o or file i/o
 * @note
 * @param[in] task
 * @retval -1 or depend on the result of __tpc_do_b_zero_w() or __tpc_do_f_zero_w()
 */
int __tpc_do_zero_rod_token_w(
	IN GEN_RW_TASK *task
	)
{
	int ret = -1;
	SUBSYSTEM_TYPE subsys_type;

	if (!task)
		return ret;

	/* try get the device backend type for this taks */
	if (__do_get_subsystem_dev_type(task->se_dev, &subsys_type))
		return ret;

	if (subsys_type == SUBSYSTEM_BLOCK){
		/* block io + fbdisk block device */
		return __tpc_do_b_zero_w(task);

	} else if (subsys_type == SUBSYSTEM_FILE){
		/* (1) file io + block backend device
		 * (2) file io + file backend device (static volume)
		 */
		return __tpc_do_f_zero_w(task);
	}

	return -1;
}


/*
 * @fn int __tpc_is_tpg_v_lun0 (IN LIO_SE_CMD *se_cmd)
 *
 * @brief Patch function to check whether the se_cmd was come from virtual LUN 0
 * @note
 * @param[in] se_cmd
 * @retval 0 - Not come from v-lun0 / 1 - Come from v-lun0
 */
int __tpc_is_tpg_v_lun0(
    IN LIO_SE_CMD *se_cmd
    )
{
    LIO_SE_SESSION *se_sess = NULL;

    if (!se_cmd)
        return 1;

    se_sess = se_cmd->se_sess;
    if (se_cmd->se_lun == &se_sess->se_tpg->tpg_virt_lun0){
        pr_err("warning: cmd was come from v-lun0\n");
        return 1;
    }
    return 0;
}


/*
 * @fn int __tpc_is_lun_receive_stop (IN LIO_SE_CMD *se_cmd)
 *
 * @brief To check whether the LU received the STOP request
 * @note
 * @param[in] se_cmd
 * @retval 0 - NOT receive STOP request / 1 - Received the STOP request
 */
int __tpc_is_lun_receive_stop(
    IN LIO_SE_CMD *se_cmd
    )
{
    unsigned long flags = 0;

    /* FIXED ME
     *
     * please refer the transport_clear_lun_from_sessions() 
     */
    spin_lock_irqsave(&se_cmd->t_state_lock, flags);
    if (se_cmd->transport_state & (CMD_T_LUN_STOP | CMD_T_LUN_FE_STOP)){
        spin_unlock_irqrestore(&se_cmd->t_state_lock, flags);
        pr_err("warning: lun(0x%x) receives stop req !!\n",se_cmd->se_lun->unpacked_lun);
        return 1;
    }
    spin_unlock_irqrestore(&se_cmd->t_state_lock, flags);
    return 0;
}


/*
 * @fn int __is_se_tpg_actived (IN LIO_SE_CMD *se_cmd)
 *
 * @brief non-lock version to check whether the tpg is active or not
 * @note
 * @param[in] se_cmd
 * @retval 0 - tpg is TPG_STATE_ACTIVE / 1 - tpg is NOT TPG_STATE_ACTIVE
 */
int __is_se_tpg_actived(
    IN LIO_SE_PORTAL_GROUP *se_tpg
    )
{
    struct iscsi_portal_group *iscsi_tpg = NULL;
    int ret = 1;

    /**/
    iscsi_tpg = container_of(se_tpg, struct iscsi_portal_group, tpg_se_tpg);

    spin_lock(&iscsi_tpg->tpg_state_lock);

    DBG_ROD_PRINT("iscsi tpg_state:0x%x\n", iscsi_tpg->tpg_state);
    if (iscsi_tpg->tpg_state == TPG_STATE_ACTIVE)
        ret = 0;
    else
        pr_err("iscsi tpg is not active (tpg_state:0x%x)\n", iscsi_tpg->tpg_state);
    spin_unlock(&iscsi_tpg->tpg_state_lock);
    return ret;
}


/*
 * @fn int __tpc_is_se_tpg_actived (IN LIO_SE_CMD *se_cmd)
 *
 * @brief lock version to check whether the tpg is active or not
 * @note
 * @param[in] se_cmd
 * @retval Depend on the result of __is_se_tpg_actived()
 */
int __tpc_is_se_tpg_actived(
    IN LIO_SE_CMD *se_cmd
    )
{
    LIO_SE_DEVICE *se_dev = NULL;
    unsigned long flags = 0;
    int ret = -1;

    if (!se_cmd->se_lun)
        return ret;

    /* FIXED ME !! */
    se_dev = se_cmd->se_lun->lun_se_dev;
    spin_lock_irqsave(&se_dev->se_port_lock, flags);
    spin_lock(&se_cmd->se_lun->lun_sep_lock);

    if (se_cmd->se_lun->lun_sep){
        if (!se_cmd->se_lun->lun_sep->sep_tpg)
            goto _EXIT_;
        ret = __is_se_tpg_actived(se_cmd->se_lun->lun_sep->sep_tpg);
    }
_EXIT_:

    spin_unlock(&se_cmd->se_lun->lun_sep_lock);
    spin_unlock_irqrestore(&se_dev->se_port_lock, flags);
    return ret;
}

/*
 * @fn void __tpc_setup_obj_timer(IN TPC_OBJ *obj)
 * @brief To setup the tpc obj timer
 *
 * @sa 
 * @param[in] obj
 * @retval N/A
 */
void __tpc_setup_obj_timer(
	IN TPC_OBJ *obj
	)
{
	/* FIXED ME !! 
	 * 
	 * The expire time of obj is always larger than token expire time
	 */
	obj->o_timer.expires   = jiffies + msecs_to_jiffies(TPC_LIFETIME * 1000);
	obj->o_timer.data      = (unsigned long )obj;
	obj->o_timer.function  = __tpc_free_obj_timer;
	add_timer(&obj->o_timer);
	return;
}

/*
 * @fn void __tpc_free_obj_timer(IN unsigned long arg)
 * @brief tpc obj timer
 *
 * @sa 
 * @param[in] arg
 * @retval N/A
 */
void __tpc_free_obj_timer(
	IN unsigned long arg
	)
{
#define TPC_OBJ_NEXT_TIME_OUT    2

	TPC_OBJ *obj = (TPC_OBJ *)arg;
	OBJ_TOKEN_STS token_status;

	spin_lock(&obj->se_tpg->tpc_obj_list_lock);

	spin_lock(&obj->o_status_lock);
	if (OBJ_STS_FREE_BY_PROC(obj->o_status)){
		pr_debug("%s: obj(id:0x%x, op_sac:0x%x) status was "
			"set to O_STS_FREE_BY_PROC already !! skip it\n", 
			__func__, obj->list_id, obj->op_sac);
	
		spin_unlock(&obj->o_status_lock);
		spin_unlock(&obj->se_tpg->tpc_obj_list_lock);
		return;
	}

	/* change the status first */
	if (OBJ_STS_ALIVE(obj->o_status))
		obj->o_status = O_STS_FREE_BY_TPC_TIMER;

	spin_unlock(&obj->o_status_lock);


	/* 2014/08/17, adamhsu, redmine 9007 */
	/* If someone still use this obj, to init timer again and
	 * wait next chance */
	spin_lock(&obj->o_ref_count_lock);
	if (atomic_read(&obj->o_ref_count)){
		spin_unlock(&obj->o_ref_count_lock);
		spin_unlock(&obj->se_tpg->tpc_obj_list_lock);

		pr_debug("%s: this obj(id:0x%x, op_sac:0x%x) "
			"is still using by someone\n", 
			__func__, obj->list_id, obj->op_sac);
	
		/* to init the timer again */
		init_timer(&obj->o_timer);
		obj->o_timer.expires = (jiffies + \
			msecs_to_jiffies(TPC_OBJ_NEXT_TIME_OUT * 1000));
		obj->o_timer.data = (unsigned long)obj;
		obj->o_timer.function = __tpc_free_obj_timer;
		add_timer(&obj->o_timer);
		return;
	}
	spin_unlock(&obj->o_ref_count_lock);
	/* 2014/08/17, adamhsu, redmine 9007 */

	spin_lock(&obj->o_status_lock);
	if (OBJ_STS_FREE_BY_TPC_TIMER(obj->o_status))
		obj->o_status = O_STS_DELETED;
	spin_unlock(&obj->o_status_lock);

	__tpc_obj_node_del(obj);
	spin_unlock(&obj->se_tpg->tpc_obj_list_lock);

	/* Start to free the tpc obj since nobody use it */
	pr_debug("%s: start free obj(id:0x%x, op_sac:0x%x)\n", 
		__FUNCTION__, obj->list_id, obj->op_sac);

	/* debug code ... 
	 * after comes to this code, the token status SHALL not be ALIVE
	 */
	spin_lock(&obj->o_token_status_lock);
	token_status = obj->o_token_status;
	spin_unlock(&obj->o_token_status_lock);

	if (TOKEN_STS_ALLOCATED_NOT_ALIVE(token_status)
	||  TOKEN_STS_FREE_BY_TOKEN_TIMER(token_status)
	||  TOKEN_STS_FREE_BY_PROC(token_status)
	||  TOKEN_STS_ALIVE(token_status)
	)
		BUG_ON(1);

	/* debug code ... */
	BUG_ON((obj->o_token_data));
	BUG_ON((!list_empty(&obj->o_data_list)));

	kfree(obj);
	pr_debug("tpc timer to free obj was done\n");
	return;

}


