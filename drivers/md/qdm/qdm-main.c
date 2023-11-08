/*
 * Copyright (C) 2012 QNAP, Inc.
 *
 * Author: CH Yang <CHYang@qnap.com.tw>
 * 	   Burton Liang <BurtonLiang@qnap.com.tw>
 *
 * This file is released under the GPLv2.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * (Release Note)
 *
 * Modification History:
 * 2012/09/21 First release.
 */

#include <linux/blkdev.h>
#include <linux/device-mapper.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/list.h>
#include <linux/mempool.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/log2.h>
#include <linux/dm-kcopyd.h>
#include <linux/random.h>

#include "../dm.h"
#include "qdm.h"

#define QDM_TARGET_MAGIC  	0x2E345678
#define RETERR 			unsigned int
#define MAX_OP_COUNT    5	//TODO SPEC

struct rw_semaphore g_qdm_origin_lock;
static struct kmem_cache *qdm_operation_tag_memcache;

// Copy from dm.c
struct dm_target_io {
	struct dm_io *io;
	struct dm_target *ti;
	union map_info info;
};

struct qdm_origin_c {
	struct rw_semaphore lock;

	struct dm_target *ti;

	/* For all origin devices linkage */
	struct list_head origin_list;
	//struct qdm_operation_c *op;   // Modified by Burton
	struct list_head op_list;

	struct dm_dev *src_dev;

	atomic_t open_count;

	sector_t capacity;
	time_t c_time;
	unsigned int status;
	unsigned int type;
	unsigned int operation;
	unsigned long long vol_id;

	unsigned long magic;	// Add by Burton
};

unsigned int get_origin_target_status(struct qdm_origin_c *qdm_org)
{
	int status;

	down_read(&qdm_org->lock);
	status = qdm_org->status;
	up_read(&qdm_org->lock);

	return status;
}

void set_origin_target_status(struct qdm_origin_c *qdm_org, int status)
{
	down_write(&qdm_org->lock);
	qdm_org->status = status;
	up_write(&qdm_org->lock);
}

unsigned int get_origin_target_type(struct qdm_origin_c *qdm_org)
{
	int type;

	down_read(&qdm_org->lock);
	type = qdm_org->type;
	up_read(&qdm_org->lock);

	return type;
}

void set_origin_target_type(struct qdm_origin_c *qdm_org, int volumn_type)
{
	down_write(&qdm_org->lock);
	qdm_org->type = volumn_type;
	up_write(&qdm_org->lock);
}

unsigned int get_origin_target_oepration(struct qdm_origin_c *qdm_org)
{
	int operation;

	down_read(&qdm_org->lock);
	operation = qdm_org->operation;
	up_read(&qdm_org->lock);

	return operation;
}

void set_origin_target_operation(struct qdm_origin_c *qdm_org, int operation)
{
	down_write(&qdm_org->lock);
	qdm_org->operation = operation;
	up_write(&qdm_org->lock);
}

unsigned int get_origin_target_vol_id(struct qdm_origin_c *qdm_org)
{
	int vol_id;

	down_read(&qdm_org->lock);
	vol_id = qdm_org->vol_id;
	up_read(&qdm_org->lock);

	return vol_id;
}

struct dm_dev *get_origin_target_src_dev(struct qdm_origin_c *qdm_org)
{
	struct dm_dev *src_dev;

	down_read(&qdm_org->lock);
	src_dev = qdm_org->src_dev;
	up_read(&qdm_org->lock);

	return src_dev;
}

/*
 * Generate an U64 volumn id
 * bit 0  ~ bit 7  :1 volume tag for chaining or dedicated volume usage
 * bit 8  ~ bit 15 :1 random
 * bit 16 ~ bit 47 :4 create time
 * bit 48 ~ bit 63 :2 0x646d (DM version)
 */
unsigned long long generate_vol_id(void)
{
	unsigned long long uuid = 0;
	unsigned long current_time = 0, random_var = 0;
	unsigned char volume_tag = 0;	// TBD

	current_time = get_seconds();
	get_random_bytes(&random_var, 4);
	random_var = random_var & 0xff;

	uuid = 0x646d;
	uuid <<= 48;		// Shift the version info first for avoiding warning
	uuid = uuid | (current_time << 16) | (random_var << 8) | volume_tag;

	return uuid;
}

sector_t device_get_capacity(struct block_device * bdev)
{
	return i_size_read(bdev->bd_inode) >> SECTOR_SHIFT;
}

int qdm_bdev_equal(struct block_device *lhs, struct block_device *rhs)
{
	/*
	 * There is only ever one instance of a particular block
	 * device so we can compare pointers safely.
	 */
	return lhs == rhs;
}

/*
static void flush_bios(struct bio *bio)
{
	struct bio *n;

	while (bio) {
		n = bio->bi_next;
		bio->bi_next = NULL;
		generic_make_request(bio);
		bio = n;
	}
}
*/

// Add by Burton for qdm operation tag
struct qdm_operation_tag *qdm_operation_tag_alloc(void)
{
	struct qdm_operation_tag *op_tag = NULL;

	op_tag = kmem_cache_alloc(qdm_operation_tag_memcache, GFP_NOIO);
	if (!op_tag)
		op_tag =
		    kmem_cache_alloc(qdm_operation_tag_memcache, GFP_ATOMIC);

	return op_tag;
}

void free_qdm_operation_tag(struct qdm_operation_tag *op_tag)
{
	kmem_cache_free(qdm_operation_tag_memcache, op_tag);
}

static void qdm_origin_init_list(void)
{
	init_rwsem(&g_qdm_origin_lock);
}

void qdm_map_simple(struct bio *bio, struct dm_dev *origin_dev)
{
	bio->bi_bdev = origin_dev->bdev;
}

static void origin_init_structure(struct qdm_origin_c *qdm_o)
{
	qdm_o->magic = QDM_TARGET_MAGIC;
	qdm_o->vol_id = generate_vol_id();
	qdm_o->c_time = get_seconds();
	qdm_o->capacity = device_get_capacity(qdm_o->src_dev->bdev);
	qdm_o->type = QDM_VOLUMN_TYPE_ORIGIN;
	qdm_o->operation = QDM_OPERATION_TYPE_NONE;
	qdm_o->status = QDM_STATUS_NORMAL;
	INIT_LIST_HEAD(&qdm_o->origin_list);
	INIT_LIST_HEAD(&qdm_o->op_list);
	atomic_set(&qdm_o->open_count, 0);
	init_rwsem(&qdm_o->lock);
}

/*
 * Construct a origin device for QDM snapshot by cow-attach command
 * <origin_dev>
 */
int qdm_origin_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	int r = -EINVAL;
	struct qdm_origin_c *qdm_o;
	char *origin_path;
	fmode_t origin_mode = FMODE_READ | FMODE_WRITE;

	QDM_DBG("qdm_origin_ctr %d %s", argc, argv[0]);
	if (argc != 1) {
		ti->error = "Requires exactly 1 arguments for source device.";
		goto bad_param;
	}

	qdm_o = kzalloc(sizeof(*qdm_o), GFP_KERNEL);
	if (!qdm_o) {
		ti->error = "No enough memory to create structure";
		goto bad_nomem;
	}

	origin_path = argv[0];
	if (dm_get_device(ti, origin_path, origin_mode, &qdm_o->src_dev)) {
		ti->error = "Can't origin get device";
		goto bad_origin;
	}

	origin_init_structure(qdm_o);
	ti->num_flush_requests = 1;
	ti->num_discard_requests = 1;
	ti->split_discard_requests = true;
	ti->private = qdm_o;
	QDM_DBG("qdm_origin_ctr done!");
	return 0;

bad_origin:
	kfree(qdm_o);

bad_nomem:
bad_param:
	QDM_DBG("%s\n", ti->error);
	return r;
}

static int qdm_origin_merge(struct dm_target *ti, struct bvec_merge_data *bvm,
		      struct bio_vec *biovec, int max_size)
{
	struct qdm_origin_c *qdm_o = ti->private;
	struct request_queue *q = bdev_get_queue(qdm_o->src_dev->bdev);
	//char b1[BDEVNAME_SIZE], b2[BDEVNAME_SIZE];

	//printk("qdm_origin_merge0: %s, %s, %s, %d, %u, %u\n", ti->type->name, bdevname(bvm->bi_bdev, b1), bdevname(qdm_o->src_dev->bdev, b2), max_size, bvm->bi_size, biovec->bv_offset);
	if (!q->merge_bvec_fn)
		return max_size;

	bvm->bi_bdev = qdm_o->src_dev->bdev;

	return min(max_size, q->merge_bvec_fn(q, bvm, biovec));
}

void qdm_origin_dtr(struct dm_target *ti)
{
	struct qdm_origin_c *qdm_org;	// Modified by Burton
	struct list_head *ptr, *tmp;
	struct qdm_operation_c *op_ptr;

	qdm_org = ti->private;

	list_for_each_safe(ptr, tmp, &qdm_org->op_list) {
		op_ptr = container_of(ptr, struct qdm_operation_c, op_link);
		if ((op_ptr) && (op_ptr->op_dtr)) {
			list_del(ptr);
			QDM_DBG("Delete op link from op_list");
			op_ptr->op_dtr(op_ptr);
		}
	}

	dm_put_device(ti, qdm_org->src_dev);
	kfree(qdm_org);
}

void qdm_map_callback(struct qdm_operation_tag *op_tag)
{
	struct qdm_origin_c *qdm_org;
	struct dm_target *ti;
	struct bio *bio;
	struct dm_target_io *tio;
	union map_info *map_context;
	struct qdm_operation_c *op;
	struct list_head *next_op_link;

	if (!op_tag)
		ASSERT(0);

	ti = op_tag->op->origin_ti;
	qdm_org = ti->private;

	bio = op_tag->bio;
	tio = (struct dm_target_io *)bio->bi_private;
	map_context = tio->info.ptr;

	down_read(&qdm_org->lock);
	// Get next list_head vi op_link.next
	next_op_link = op_tag->op->op_link.next;
	if (next_op_link != &qdm_org->op_list) {
		// Get the qdm_operation_c structure
		op = container_of(next_op_link, struct qdm_operation_c,
				  op_link);
		op_tag->op = op;
		op_tag->private = op->private;
		op_tag->timestamp = jiffies;
		// Continue next map process if map fn exists.
		if (op->op_map) {
			op->op_map(op_tag, bio, map_context);
			up_read(&qdm_org->lock);
			return;
		}
	}
	up_read(&qdm_org->lock);

	qdm_map_simple(op_tag->bio, qdm_org->src_dev);
	generic_make_request(op_tag->bio);
	//memfree op_tag by Burton
	free_qdm_operation_tag(op_tag);

	return;
}

int qdm_do_operation(struct qdm_operation_c *op,
		     struct dm_target *ti,
		     struct bio *bio, union map_info *map_context)
{
	struct qdm_operation_tag *op_tag = NULL;
	struct qdm_origin_c *qdm_org = ti->private;
	int ret = DM_MAPIO_REMAPPED;

	QDM_DBG("qdm_do_operation");

	if (op->op_map) {
		// Allocate op_tag from mempool. Burton
		op_tag = qdm_operation_tag_alloc();
		if (op_tag) {

			op_tag->op = op;
			op_tag->bio = bio;
			op_tag->timestamp = jiffies;
			op_tag->callback = (void *)qdm_map_callback;
			op_tag->private = op->private;

			ret = op->op_map(op_tag, bio, map_context);
			return DM_MAPIO_SUBMITTED;
		}
	}

	qdm_map_simple(bio, qdm_org->src_dev);
	return DM_MAPIO_REMAPPED;

}

int qdm_origin_map(struct dm_target *ti,
		   struct bio *bio, union map_info *map_context)
{
	struct qdm_origin_c *qdm_org;
	struct list_head *ptr;
	struct qdm_operation_c *op_ptr;

	qdm_org = ti->private;

	if (!qdm_org)
		return -EIO;

	down_read(&qdm_org->lock);
	ptr = qdm_org->op_list.next;

	if ((ptr) && (ptr != &qdm_org->op_list)) {
		op_ptr = container_of(ptr, struct qdm_operation_c, op_link);
		up_read(&qdm_org->lock);
		return qdm_do_operation(op_ptr, ti, bio, map_context);
	}

	qdm_map_simple(bio, qdm_org->src_dev);
	up_read(&qdm_org->lock);

	return DM_MAPIO_REMAPPED;

}

int qdm_origin_status(struct dm_target *ti,
		      status_type_t type, char *result, unsigned int maxlen)
{
	/* TBD */
	return 0;
}

int qdm_origin_message(struct dm_target *ti, unsigned int argc, char **argv)
{
	/* TBD */
	return 0;
}

int qdm_origin_open_count(struct dm_target *ti)
{
	struct qdm_origin_c *qdm_o;

	qdm_o = ti->private;

	return atomic_read(&qdm_o->open_count);
}

static int qdm_iterate_devices(struct dm_target *ti,
			       iterate_devices_callout_fn fn, void *data)
{
	struct qdm_origin_c *qdm_o = ti->private;

	return fn(ti, qdm_o->src_dev, 0, ti->len, data);
}

struct target_type qdm_origin_target = {
	.name = "qdm_origin",
	.version = {1, 1, 0},
	.module = THIS_MODULE,
	.ctr = qdm_origin_ctr,
	.dtr = qdm_origin_dtr,
	.map = qdm_origin_map,
	.status = qdm_origin_status,
	.message = qdm_origin_message,
	.open_count = qdm_origin_open_count,
	.iterate_devices = qdm_iterate_devices,
	.merge = qdm_origin_merge,
};

int qdm_init_origin(void)
{
	int r;
	r = dm_register_target(&qdm_origin_target);
	//QDM_DBG("dm_register_target qdm_origin_target result : %d", r);
	if (r == 0) {
		qdm_origin_init_list();
	} else {
		QDM_DBG("Init origin target fail.");
	}

	//QDM_DBG("<%d>", sizeof(struct qdm_operation_c));
	return r;
}

void qdm_exit_origin(void)
{
	dm_unregister_target(&qdm_origin_target);
}

/************  export and operation with hook modules *************/

int qdm_op_register(struct dm_target *ti, struct qdm_operation_c *op)
{
	struct qdm_origin_c *qdm_org;
	struct list_head *ptr;
	struct qdm_operation_c *op_ptr;
	int count = 0;

	qdm_org = ti->private;

	if (qdm_org->magic != QDM_TARGET_MAGIC)
		return ERROR;

	down_write(&qdm_org->lock);
	list_for_each(ptr, &qdm_org->op_list) {
		op_ptr = container_of(ptr, struct qdm_operation_c, op_link);
		if (op == op_ptr) {
			QDM_DBG
			    ("Error! Target type is already been registered on this origin device.");
			up_write(&qdm_org->lock);	// Unlock before return
			return ERROR;
		}

		if ((count++) > MAX_OP_COUNT) {
			up_write(&qdm_org->lock);	// Unlock before return
			return ERROR;
		}
	}

	list_add_tail(&op->op_link, &qdm_org->op_list);
	up_write(&qdm_org->lock);

	return OK;

}

void qdm_op_unregister(struct dm_target *ti, struct qdm_operation_c *op)
{
	struct qdm_origin_c *qdm_org;
	struct list_head *ptr;
	struct qdm_operation_c *op_ptr;

	qdm_org = ti->private;

	if (qdm_org->magic != QDM_TARGET_MAGIC)
		return;

	down_write(&qdm_org->lock);
	list_for_each(ptr, &qdm_org->op_list) {
		op_ptr = container_of(ptr, struct qdm_operation_c, op_link);
		if (op_ptr == op) {
			up_write(&qdm_org->lock);	// Unlock before return;
			list_del(&op->op_link);
			return;
		}
	}
	up_write(&qdm_org->lock);

	ASSERT(0);
	return;
}

struct qdm_operation_c *qdm_allocate_op(void)
{
	struct qdm_operation_c *op = NULL;
	op = kzalloc(sizeof(struct qdm_operation_c), GFP_KERNEL);
	if (!op) {
		QDM_DBG("qdm_allocate_op : null op");
		return NULL;
	} else {
		op->op_version = QDM_OPERATION_VERSION;
		INIT_LIST_HEAD(&op->op_link);
		return op;
	}
}

void qdm_free_op(struct qdm_operation_c *op)
{
	kfree(op);
}

void qdm_open_origin(struct dm_target *ti)
{
	struct qdm_origin_c *qdm_org;

	qdm_org = ti->private;

	atomic_inc(&qdm_org->open_count);
}

void qdm_close_origin(struct dm_target *ti)
{
	struct qdm_origin_c *qdm_org;

	qdm_org = ti->private;

	atomic_dec(&qdm_org->open_count);
}

EXPORT_SYMBOL(qdm_op_register);
EXPORT_SYMBOL(qdm_op_unregister);
EXPORT_SYMBOL(qdm_allocate_op);
EXPORT_SYMBOL(qdm_free_op);
EXPORT_SYMBOL(qdm_open_origin);
EXPORT_SYMBOL(qdm_close_origin);

/*************** QDM Module Init ***************/

int __init dm_qdm_init(void)
{
	int r;
	r = qdm_init_origin();
	// Target registration should be 0, which means success.
	if (r < 0)
		return ERROR;
	qdm_operation_tag_memcache = KMEM_CACHE(qdm_operation_tag, 0);
	if (!qdm_operation_tag_memcache) {
		QDM_DBG("KMEM_CACHE qdm_operation_tag_memcache fail!");
		goto bad_qdm_operation_tag_memcache;
	}

	QDM_DBG("Init QDM module and register %s target success!",
		qdm_origin_target.name);
	return r;

bad_qdm_operation_tag_memcache:
	qdm_exit_origin();
	ASSERT(0);
	return r;
}

void dm_qdm_exit(void)
{
	kmem_cache_destroy(qdm_operation_tag_memcache);
	qdm_exit_origin();
	QDM_DBG("Remove QDM module and unregister %s target success!",
		qdm_origin_target.name);
}

module_init(dm_qdm_init);
module_exit(dm_qdm_exit);
MODULE_AUTHOR("CH Yang <CHYang@qnap.com.tw>");
MODULE_AUTHOR("Burton Liang <BurtonLiang@qnap.com.tw>");
MODULE_DESCRIPTION(DM_NAME "QNAP common dynamic operating target");
MODULE_LICENSE("GPL");
