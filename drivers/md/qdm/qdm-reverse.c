/*
 * Copyright (C) 2001-2003 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/device-mapper.h>

#include "../dm.h"
#include "qdm.h"

#define MIN_IOS		16
#define DM_MSG_PREFIX	"qdm-reverse"

static int reverse_op_map(struct qdm_operation_tag *op_tag, struct bio *bio,
			  union map_info *map_context);

static void reverse_op_dtr(struct qdm_operation_c *op);

/*
 * Linear: maps a reverse range of a device.
 */
struct reverse_c {
	struct dm_dev *src_target;
	struct dm_dev *reverse_dev;
	unsigned long long capacity;
	struct qdm_operation_c *reverse_op;
	struct bio_set *bs;
};

static void reverse_clone_bio_destructor(struct bio *bio)
{
	struct reverse_c *lc;

	lc = (struct reverse_c *)bio->bi_private;

	bio_free(bio, lc->bs);
}

static void reverse_clone_endio(struct bio *bio, int error)
{
	struct reverse_c *lc;

	lc = (struct reverse_c *)bio->bi_private;

	bio_put(bio);
}

struct bio *clone_map_bio(struct reverse_c *lc, struct bio *bio)
{
	struct bio *clone;

	QDM_DBG("QDM : clone_map_bio!");
	clone = bio_alloc_bioset(GFP_NOIO, bio->bi_max_vecs, lc->bs);
	//clone = bio_clone(bio, GFP_NOIO);

	if (!clone)
		return NULL;

	__bio_clone(clone, bio);
	clone->bi_destructor = reverse_clone_bio_destructor;
	clone->bi_end_io = reverse_clone_endio;
	clone->bi_private = lc;

	return clone;
}

static void reverse_sector(struct reverse_c *lc, struct bio *bio)
{
//TODO FIXBUG: capacity and check len and sector.
//      bio->bi_sector = lc->capacity - bio->bi_sector;
	return;
}

/************** Do register QDM functions *****************/

void reverse_unhook_operation(struct qdm_operation_c *reverse_op)
{
	struct dm_target *origin_ti;
	struct reverse_c *lc;

	lc = reverse_op->private;

	if (reverse_op) {
		origin_ti = reverse_op->origin_ti;
		if (origin_ti)
			qdm_op_unregister(origin_ti, reverse_op);

		lc->reverse_op = NULL;
		qdm_free_op(reverse_op);

		if (!lc->reverse_op)
			QDM_DBG("Clear reverse_op of this operating target!");
	}
}

void reverse_hook_operation(struct dm_target *origin_ti, struct reverse_c *lc)
{
	lc->reverse_op = qdm_allocate_op();

	if (!lc->reverse_op) {
		QDM_DBG("Allocate qdm_operation_c for reverse target fail!");
		return;
	} else {
		QDM_DBG("Allocate qdm_operation_c for reverse target success!");

		if (!lc->reverse_op)
			QDM_DBG("Null reverse_op!");

		//lc->reverse_op->ctr = reverse_ctr;
		lc->reverse_op->op_dtr = reverse_op_dtr;
		lc->reverse_op->op_map = reverse_op_map;
		lc->reverse_op->op_suspend = NULL;
		lc->reverse_op->op_resume = NULL;
		lc->reverse_op->private = lc;
		lc->reverse_op->op_type = QDM_OPERATION_TYPE_REVERSE;
		lc->reverse_op->origin_ti = origin_ti;
	}

	if (qdm_op_register(origin_ti, lc->reverse_op) < 0) {
		QDM_DBG("Register qdm_operation FAIL!");
		qdm_free_op(lc->reverse_op);
	}
	return;
}

void reverse_add_operation(struct reverse_c *lc)
{
	struct mapped_device *origin_md;
	struct dm_table *origin_map;
	struct dm_target *origin_ti;

	origin_md = dm_get_md(lc->src_target->bdev->bd_dev);
	if (!origin_md) {
		QDM_DBG("Can not get mapped_device");
		return;
	}
	origin_map = dm_get_live_table(origin_md);
	if (!origin_map) {
		QDM_DBG("Can not get dm_table");
		return;
	}
	origin_ti = dm_table_find_target(origin_map, 0);
	if (!origin_ti) {
		QDM_DBG("Can not get target_device");
		return;
	}

	reverse_hook_operation(origin_ti, lc);
	dm_table_put(origin_map);
	dm_put(origin_md);

	return;
}

static void reverse_op_bio(struct reverse_c *lc, struct bio *bio)
{
	struct bio_pair *bp;
	int sector_count;
	struct bio *bio_ptr;

	bio_ptr = bio;
	bio->bi_bdev = lc->reverse_dev->bdev;
	sector_count = bio->bi_size >> 9;

	while ((sector_count > 0) && (bio_ptr)) {
		bp = bio_split(bio_ptr, bio_ptr->bi_sector + 1);
		reverse_sector(lc, &bp->bio1);
		generic_make_request(&bp->bio1);
		sector_count--;
		bio_ptr = &bp->bio2;
	}

	return;
}

static int reverse_op_map(struct qdm_operation_tag *op_tag, struct bio *bio,
			  union map_info *map_context)
{
	struct bio *clone_bio;
	struct reverse_c *lc;

	lc = op_tag->private;

	if (bio_rw(bio) == WRITE) {
		clone_bio = clone_map_bio(lc, bio);
		if (clone_bio) {
			reverse_op_bio(lc, clone_bio);
			return QDM_MAPOP_CONTINUE;
		}
	}

	return QDM_MAPOP_CONTINUE;
}

static void reverse_op_dtr(struct qdm_operation_c *op)
{
	reverse_unhook_operation(op);
	return;
}

/**************  Reverse Main function *******************/
static int reverse_status(struct dm_target *ti, status_type_t type,
			  char *result, unsigned int maxlen)
{
	QDM_DBG("QDM : reverse_status!");
	return 0;
}

static int reverse_map(struct dm_target *ti, struct bio *bio,
		       union map_info *map_context)
{
	//struct bio *clone_bio;
	struct reverse_c *lc;

	QDM_DBG("QDM : reverse_map");

	lc = ti->private;
	bio->bi_bdev = lc->reverse_dev->bdev;

	return DM_MAPIO_REMAPPED;
}

/*
 * Construct a reverse mapping: <org_dev_path> <reverse_dev_path>
 */
static int reverse_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct reverse_c *lc;
	fmode_t origin_mode = FMODE_READ | FMODE_WRITE;

	//unsigned long long tmp;
	//char dummy;

	// Test
	unsigned char counter = 0;
	for (counter = 0; counter < argc; counter++) {
		QDM_DBG("%s", argv[counter]);
	}
	// Test

	if (argc != 2) {
		ti->error = "Invalid argument count!";
		return -EINVAL;
	}

	lc = kmalloc(sizeof(*lc), GFP_KERNEL);
	if (lc == NULL) {
		ti->error = "qdm-reverse: Cannot allocate reverse context";
		return -ENOMEM;
	}

	if (dm_get_device(ti, argv[0], origin_mode, &lc->src_target)) {
		ti->error = "qdm-reverse: Device lookup failed";
		goto bad_origin;
	}

	if (dm_get_device
	    (ti, argv[1], dm_table_get_mode(ti->table), &lc->reverse_dev)) {
		ti->error = "qdm-reverse: Device lookup failed";
		goto bad_reverse;
	}

	ti->num_flush_requests = 1;
	ti->num_discard_requests = 1;
	ti->split_discard_requests = true;
	ti->private = lc;
	lc->capacity = ti->len;	//TODO CHECK, CHYANG
	lc->bs = bioset_create(MIN_IOS, 0);

	reverse_add_operation(lc);
	lc->reverse_op->private = lc;
	dm_put_device(ti, lc->src_target);	//CHYANG TEST FOR LOCK
	QDM_DBG("Reverse_ctr Done!");
	return 0;

bad_reverse:
	dm_put_device(ti, lc->src_target);

bad_origin:
	kfree(lc);
	return -EINVAL;
}

static void reverse_dtr(struct dm_target *ti)
{
	struct reverse_c *lc = ti->private;
	//struct dm_target *origin_ti;

	if (lc->reverse_op)
		reverse_unhook_operation(lc->reverse_op);

	if (lc->bs)
		bioset_free(lc->bs);

	dm_put_device(ti, lc->src_target);	//CHYANG TEST FOR LOCK
	dm_put_device(ti, lc->reverse_dev);

	kfree(lc);
}

static int reverse_iterate_devices(struct dm_target *ti,
				   iterate_devices_callout_fn fn, void *data)
{
	struct reverse_c *lc = ti->private;

	return fn(ti, lc->reverse_dev, 0, ti->len, data);
}

static struct target_type reverse_target = {
	.name = "reverse",
	.version = {1, 1, 0},
	.module = THIS_MODULE,
	.ctr = reverse_ctr,
	.dtr = reverse_dtr,
	.map = reverse_map,
	.status = reverse_status,
	.iterate_devices = reverse_iterate_devices,
};

static int __init dm_reverse_init(void)
{
	int r = dm_register_target(&reverse_target);

	if (r < 0)
		DMERR("register failed %d", r);

	QDM_DBG("Register %s target success!", reverse_target.name);

	return r;
}

static void __exit dm_reverse_exit(void)
{
	dm_unregister_target(&reverse_target);

	QDM_DBG("Unregister %s target success!", reverse_target.name);
}

module_init(dm_reverse_init);
module_exit(dm_reverse_exit);
MODULE_AUTHOR("CH Yang <CHYang@qnap.com.tw>");
MODULE_AUTHOR("Burton Liang <BurtonLiang@qnap.com.tw>");
MODULE_DESCRIPTION(DM_NAME "QNAP operating target sample");
MODULE_LICENSE("GPL");
