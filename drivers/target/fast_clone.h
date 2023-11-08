/**
 * $Header:
 *
 * Copyright (c) 2013 QNAP SYSTEMS, INC.
 * All Rights Reserved.
 *
 * $Id:
 */
/**
 * @file
 * @brief
 *
 * @author
 * @date
 */

#ifndef __FAST_CLONE_H__
#define __FAST_CLONE_H__

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/blkdev.h>

/* these structures / functions shall be export by fast clone manager layer */

typedef struct _thin_blockclone_desc{
	struct block_device *src_dev;
	struct block_device *dest_dev;
	sector_t src_block_addr;
	sector_t dest_block_addr;

	/* Since this call will be executed in kernel block layer, 
	 * so its unit is 512b */
	u32 transfer_blocks;

	/* This will be used by callers who uses this structure if they want
	 * to do something in the callback*() */
	void *private_data;
} THIN_BLOCKCLONE_DESC;


/**/
typedef void (*CLONE_CALLBACK)(
        int err_code, 
        THIN_BLOCKCLONE_DESC *desc
        );

/* 0: Success, Others: Fail */
extern int thin_support_block_cloning(
	struct block_device *s_blkdev, 
	struct block_device *d_blkdev
	);

/* 0: Success, Others: Fail */
extern int thin_do_block_cloning(
	THIN_BLOCKCLONE_DESC *cloning_desc,
	CLONE_CALLBACK *callback
	);


#endif /* __FAST_CLONE_H__ */

