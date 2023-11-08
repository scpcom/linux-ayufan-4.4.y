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

#ifndef __TARGET_FAST_CLONE_H__
#define __TARGET_FAST_CLONE_H__


#include <linux/types.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/fast_clone.h>


#include "target_general.h"
#if defined(SUPPORT_VAAI)
#include "vaai_helper.h"
#endif
#if defined(SUPPORT_TPC_CMD) 
#include "tpc_helper.h"
#endif


/* TBD, shall be multuple of 4KB(or 512b) at least */
#define POOL_BLK_SIZE_KB	POOL_BLK_SIZE_512_KB

/* TBD, the value match to ODX setting current */
#define MAX_FBC_IO		(64*1024*1024)
#define FAST_CLONE_TIMEOUT_SEC	4

/**/
typedef struct __fast_clone_io_cb_data{
	atomic_t io_done;
	atomic_t io_err_count;
	struct completion *io_wait;

	/* used on thin provisioning case */
	int nospc_err;
} FCCB_DATA;


typedef struct __fast_clone_io_rec{
	struct list_head io_node;
	THIN_BLOCKCLONE_DESC desc;

	/* 0: init value , 1: io done w/o error , -1: io done with any error */
	int io_done;
} FCIO_REC;

typedef struct __create_record{
	struct block_device *s_blkdev;
	struct block_device *d_blkdev;
	sector_t s_lba;
	sector_t d_lba;
	u64 transfer_bytes;
} CREATE_REC;

typedef struct _fast_clone_obj{
	LIO_SE_DEVICE	*s_se_dev;
	LIO_SE_DEVICE	*d_se_dev;
	sector_t	s_lba;
	sector_t	d_lba;
	u32		s_dev_bs;
	u32		d_dev_bs;
	u64		data_bytes;
	int		nospc_err;
} FC_OBJ;

typedef struct __tbc_desc_data{
	THIN_BLOCKCLONE_DESC tbc_desc;

	/* the unit for lba data bytes listed below is original unit */
	sector_t h_d_lba;	/* head */
	sector_t t_d_lba;	/* tail */

	/* align area */
	sector_t d_align_lba;
	u64 data_bytes;		/* shall be multiple by pool block size */
	int do_fbc;
} TBC_DESC_DATA;




/**/
int quick_check_support_fbc(struct block_device *bd);


int __do_fast_clone(FC_OBJ *fc_obj);

int __create_fast_clone_io_lists(
	struct list_head *io_lists, 
	CREATE_REC *create_rec
	);

int __submit_fast_clone_io_lists_wait(struct list_head *io_lists);
void __free_fast_clone_io_lists(struct list_head *io_lists);
void __fast_clone_cb(int err, THIN_BLOCKCLONE_DESC *clone_desc);
void __init_fccb_data(FCCB_DATA *fccb_data, struct completion *io_wait);
u32 __get_done_blks_by_fast_clone_io_lists(struct list_head *io_lists);


int __do_fast_block_clone(FC_OBJ *fc_obj);
void __create_fbc_obj(FC_OBJ *fc_obj, 
	LIO_SE_DEVICE *s_se_dev, LIO_SE_DEVICE *d_se_dev, 
	sector_t s_lba, sector_t d_lba, u64 data_bytes);

int __check_s_d_lba_before_fbc(FC_OBJ *fc_obj);
int __do_check_support_fbc(FC_OBJ *fc_obj, TBC_DESC_DATA *tbcd_data);
int __do_update_fbc_data_bytes(FC_OBJ *fc_obj, TBC_DESC_DATA *tbcd_data, 
	sector_t dest_lba, u64 *data_bytes);

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD) 

int __do_wt_by_fbc(WBT_OBJ *wbt_obj, FC_OBJ *fc_obj, 
	u64 *data_bytes);

#endif

#if defined(SUPPORT_VAAI) 
int __do_b2b_xcopy_by_fbc(B2B_XCOPY_OBJ *b2b_obj, FC_OBJ *fc_obj, 
	u64 *data_bytes);
#endif



#endif


#endif /* __FAST_CLONE_H__ */

