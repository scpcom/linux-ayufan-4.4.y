#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
   linear.c : Multiple Devices driver for Linux
	      Copyright (C) 1994-96 Marc ZYNGIER
	      <zyngier@ufr-info-p7.ibp.fr> or
	      <maz@gloups.fdn.fr>

   Linear mode management functions.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   You should have received a copy of the GNU General Public License
   (for example /usr/src/linux/COPYING); if not, write to the Free
   Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/blkdev.h>
#include <linux/raid/md_u.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "md.h"
#include "linear.h"

/*
 * find which device holds a particular offset
 */
static inline struct dev_info *which_dev(struct mddev *mddev, sector_t sector)
{
	int lo, mid, hi;
	struct linear_conf *conf;

	lo = 0;
	hi = mddev->raid_disks - 1;
	conf = mddev->private;

	/*
	 * Binary Search
	 */

	while (hi > lo) {

		mid = (hi + lo) / 2;
		if (sector < conf->disks[mid].end_sector)
			hi = mid;
		else
			lo = mid + 1;
	}

	return conf->disks + lo;
}

/*
 * In linear_congested() conf->raid_disks is used as a copy of
 * mddev->raid_disks to iterate conf->disks[], because conf->raid_disks
 * and conf->disks[] are created in linear_conf(), they are always
 * consitent with each other, but mddev->raid_disks does not.
 */
static int linear_congested(struct mddev *mddev, int bits)
{
	struct linear_conf *conf;
	int i, ret = 0;

	rcu_read_lock();
	conf = rcu_dereference(mddev->private);
#ifdef MY_ABC_HERE
	if (mddev->degraded) {
		rcu_read_unlock();
		return ret;
	}

	for (i = 0; i < conf->raid_disks && !ret ; i++) {
		struct md_rdev *rdev = rcu_dereference(conf->disks[i].rdev);
		struct request_queue *q = NULL;

		if (!rdev) {
			continue;
		}

		q = bdev_get_queue(rdev->bdev);
		ret |= bdi_congested(&q->backing_dev_info, bits);
	}
#else /* MY_ABC_HERE */
	for (i = 0; i < conf->raid_disks && !ret ; i++) {
		struct request_queue *q = bdev_get_queue(conf->disks[i].rdev->bdev);
		ret |= bdi_congested(&q->backing_dev_info, bits);
	}
#endif /* MY_ABC_HERE */

	rcu_read_unlock();
	return ret;
}

static sector_t linear_size(struct mddev *mddev, sector_t sectors, int raid_disks)
{
	struct linear_conf *conf;
	sector_t array_sectors;

	conf = mddev->private;
	WARN_ONCE(sectors || raid_disks,
		  "%s does not support generic reshape\n", __func__);
	array_sectors = conf->array_sectors;

	return array_sectors;
}

static struct linear_conf *linear_conf(struct mddev *mddev, int raid_disks)
{
	struct linear_conf *conf;
	struct md_rdev *rdev;
	int i, cnt;
	bool discard_supported = false;

	conf = kzalloc (sizeof (*conf) + raid_disks*sizeof(struct dev_info),
			GFP_KERNEL);
	if (!conf)
		return NULL;

	cnt = 0;
	conf->array_sectors = 0;

	rdev_for_each(rdev, mddev) {
		int j = rdev->raid_disk;
		struct dev_info *disk = conf->disks + j;
		sector_t sectors;

		if (j < 0 || j >= raid_disks || disk->rdev) {
			printk(KERN_ERR "md/linear:%s: disk numbering problem. Aborting!\n",
			       mdname(mddev));
			goto out;
		}

		disk->rdev = rdev;
		if (mddev->chunk_sectors) {
			sectors = rdev->sectors;
			sector_div(sectors, mddev->chunk_sectors);
			rdev->sectors = sectors * mddev->chunk_sectors;
		}

		disk_stack_limits(mddev->gendisk, rdev->bdev,
				  rdev->data_offset << 9);

		conf->array_sectors += rdev->sectors;
		cnt++;

		if (blk_queue_discard(bdev_get_queue(rdev->bdev)))
			discard_supported = true;
	}
	if (cnt != raid_disks) {
#ifdef MY_ABC_HERE
		/*
		 * for Linear status consistense to other raid type
		 * Let it can assemble.
		 */
		mddev->degraded = mddev->raid_disks - cnt;
#ifdef MY_ABC_HERE
		if (MD_CRASHED_ASSEMBLE != mddev->nodev_and_crashed) {
			mddev->nodev_and_crashed = MD_CRASHED;
		}
#endif /* MY_ABC_HERE */
		printk(KERN_ERR "md/linear:%s: not enough drives present.\n",
		       mdname(mddev));
		return conf;
#else /* MY_ABC_HERE */
		printk(KERN_ERR "md/linear:%s: not enough drives present. Aborting!\n",
		       mdname(mddev));
		goto out;
#endif /* MY_ABC_HERE */
	}

	if (!discard_supported)
		queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, mddev->queue);
	else
		queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, mddev->queue);

	/*
	 * Here we calculate the device offsets.
	 */
	conf->disks[0].end_sector = conf->disks[0].rdev->sectors;

	for (i = 1; i < raid_disks; i++)
		conf->disks[i].end_sector =
			conf->disks[i-1].end_sector +
			conf->disks[i].rdev->sectors;

	/*
	 * conf->raid_disks is copy of mddev->raid_disks. The reason to
	 * keep a copy of mddev->raid_disks in struct linear_conf is,
	 * mddev->raid_disks may not be consistent with pointers number of
	 * conf->disks[] when it is updated in linear_add() and used to
	 * iterate old conf->disks[] earray in linear_congested().
	 * Here conf->raid_disks is always consitent with number of
	 * pointers in conf->disks[] array, and mddev->private is updated
	 * with rcu_assign_pointer() in linear_addr(), such race can be
	 * avoided.
	 */
	conf->raid_disks = raid_disks;

	return conf;

out:
	kfree(conf);
	return NULL;
}

static int linear_run (struct mddev *mddev)
{
	struct linear_conf *conf;
	int ret;

	if (md_check_no_bitmap(mddev))
		return -EINVAL;
#ifdef MY_ABC_HERE
	mddev->degraded = 0;
#endif /* MY_ABC_HERE */
	conf = linear_conf(mddev, mddev->raid_disks);

	if (!conf)
		return 1;
	mddev->private = conf;
	md_set_array_sectors(mddev, linear_size(mddev, 0, 0));

	ret =  md_integrity_register(mddev);
	if (ret) {
		kfree(conf);
		mddev->private = NULL;
	}
	return ret;
}

static int linear_add(struct mddev *mddev, struct md_rdev *rdev)
{
	/* Adding a drive to a linear array allows the array to grow.
	 * It is permitted if the new drive has a matching superblock
	 * already on it, with raid_disk equal to raid_disks.
	 * It is achieved by creating a new linear_private_data structure
	 * and swapping it in in-place of the current one.
	 * The current one is never freed until the array is stopped.
	 * This avoids races.
	 */
	struct linear_conf *newconf, *oldconf;

	if (rdev->saved_raid_disk != mddev->raid_disks)
		return -EINVAL;

	rdev->raid_disk = rdev->saved_raid_disk;
	rdev->saved_raid_disk = -1;

	newconf = linear_conf(mddev,mddev->raid_disks+1);

	if (!newconf)
		return -ENOMEM;

	/* newconf->raid_disks already keeps a copy of * the increased
	 * value of mddev->raid_disks, WARN_ONCE() is just used to make
	 * sure of this. It is possible that oldconf is still referenced
	 * in linear_congested(), therefore kfree_rcu() is used to free
	 * oldconf until no one uses it anymore.
	 */
	mddev_suspend(mddev);
	oldconf = rcu_dereference(mddev->private);
	mddev->raid_disks++;
	WARN_ONCE(mddev->raid_disks != newconf->raid_disks,
		"copied raid_disks doesn't match mddev->raid_disks");
	rcu_assign_pointer(mddev->private, newconf);
	md_set_array_sectors(mddev, linear_size(mddev, 0, 0));
	set_capacity(mddev->gendisk, mddev->array_sectors);
	mddev_resume(mddev);
	revalidate_disk(mddev->gendisk);
	kfree_rcu(oldconf, rcu);
	return 0;
}

static void linear_free(struct mddev *mddev, void *priv)
{
	struct linear_conf *conf = priv;

	kfree(conf);
}

#ifdef MY_ABC_HERE
/**
 * This is end_io callback function.
 * We can use this for bad sector report and device error
 * handing. Prevent umount panic from file system
 *
 * @author \$Author: khchen $
 * @version \$Revision: 1.1
 *
 * @param bio    Should not be NULL. Passing from block layer
 * @param error  error number
 */
static void
SynoLinearEndRequest(struct bio *bio)
{
	int bio_error = bio->bi_error;
	struct mddev *mddev = NULL;
	struct md_rdev *rdev = NULL;
	struct bio *data_bio;

	data_bio = bio->bi_private;

	rdev = (struct md_rdev *)data_bio->bi_next;
	mddev = rdev->mddev;

	bio->bi_end_io = data_bio->bi_end_io;
	bio->bi_private = data_bio->bi_private;

	if (bio_error) {
#ifdef MY_ABC_HERE
		if (IsDeviceDisappear(rdev->bdev)) {
			syno_md_error(mddev, rdev);
		} else {
#ifdef MY_ABC_HERE
#ifdef MY_ABC_HERE
			if (bio_flagged(bio, BIO_AUTO_REMAP)) {
				SynoReportBadSector(bio->bi_iter.bi_sector, bio->bi_rw, mddev->md_minor, bio->bi_bdev, __FUNCTION__);
			}
#else /* MY_ABC_HERE */
			SynoReportBadSector(bio->bi_iter.bi_sector, bio->bi_rw, mddev->md_minor, bio->bi_bdev, __FUNCTION__);
#endif /* MY_ABC_HERE */
#endif /* MY_ABC_HERE */
			md_error(mddev, rdev);
		}
#else /* MY_ABC_HERE */
		md_error(mddev, rdev);
#endif /* MY_ABC_HERE */
	}

	atomic_dec(&rdev->nr_pending);
	bio_put(data_bio);
	/* Let mount could successful and bad sector could keep accessing, no matter it success or not */
	bio_endio(bio);
}
#endif /* MY_ABC_HERE */

static void linear_make_request(struct mddev *mddev, struct bio *bio)
{
	char b[BDEVNAME_SIZE];
	struct dev_info *tmp_dev;
	struct bio *split;
	sector_t start_sector, end_sector, data_offset;
#ifdef MY_ABC_HERE
	struct bio *data_bio;
#endif /* MY_ABC_HERE */

	if (unlikely(bio->bi_rw & REQ_FLUSH)) {
		md_flush_request(mddev, bio);
		return;
	}

	do {
#ifdef MY_ABC_HERE
		/**
		 * if there has any device offline, we don't make any request to
		 * our linear md array
		 */
#ifdef MY_ABC_HERE
		if (mddev->nodev_and_crashed) {
#else /* MY_ABC_HERE */
		if (mddev->degraded) {
#endif /* MY_ABC_HERE */
			bio->bi_error = -EIO;
			bio_endio(bio);
			return;
		}
#endif /* MY_ABC_HERE */
		tmp_dev = which_dev(mddev, bio->bi_iter.bi_sector);
		start_sector = tmp_dev->end_sector - tmp_dev->rdev->sectors;
		end_sector = tmp_dev->end_sector;
		data_offset = tmp_dev->rdev->data_offset;
		bio->bi_bdev = tmp_dev->rdev->bdev;

		if (unlikely(bio->bi_iter.bi_sector >= end_sector ||
			     bio->bi_iter.bi_sector < start_sector))
			goto out_of_bounds;

		if (unlikely(bio_end_sector(bio) > end_sector)) {
			/* This bio crosses a device boundary, so we have to
			 * split it.
			 */
			split = bio_split(bio, end_sector -
					  bio->bi_iter.bi_sector,
					  GFP_NOIO, fs_bio_set);
			bio_chain(split, bio);
		} else {
			split = bio;
		}

		split->bi_iter.bi_sector = split->bi_iter.bi_sector -
			start_sector + data_offset;

#ifdef MY_ABC_HERE
		data_bio = bio_clone(split, GFP_NOIO);

		if (data_bio) {
			atomic_inc(&tmp_dev->rdev->nr_pending);
			data_bio->bi_end_io = split->bi_end_io;
			data_bio->bi_private = split->bi_private;
			data_bio->bi_next = (void *)tmp_dev->rdev;

			split->bi_end_io = SynoLinearEndRequest;
			split->bi_private = data_bio;
		}
#endif /* MY_ABC_HERE */

		if (unlikely((split->bi_rw & REQ_DISCARD) &&
			 !blk_queue_discard(bdev_get_queue(split->bi_bdev)))) {
			/* Just ignore it */
			bio_endio(split);
		} else
			generic_make_request(split);
	} while (split != bio);
	return;

out_of_bounds:
	printk(KERN_ERR
	       "md/linear:%s: make_request: Sector %llu out of bounds on "
	       "dev %s: %llu sectors, offset %llu\n",
	       mdname(mddev),
	       (unsigned long long)bio->bi_iter.bi_sector,
	       bdevname(tmp_dev->rdev->bdev, b),
	       (unsigned long long)tmp_dev->rdev->sectors,
	       (unsigned long long)start_sector);
	bio_io_error(bio);
}

#ifdef MY_ABC_HERE
static void
syno_linear_status(struct seq_file *seq, struct mddev *mddev)
{
	struct linear_conf *conf;
	struct md_rdev *rdev;
	int j;

	seq_printf(seq, " %dk rounding", mddev->chunk_sectors / 2);
	seq_printf(seq, " [%d/%d] [", mddev->raid_disks, mddev->raid_disks - mddev->degraded);
	rcu_read_lock();
	conf = rcu_dereference(mddev->private);
	for (j = 0; j < mddev->raid_disks; j++)
	{
		rdev = rcu_dereference(conf->disks[j].rdev);
#ifdef MY_ABC_HERE
		if (rdev &&
			!test_bit(Faulty, &rdev->flags)) {
#else /* MY_ABC_HERE */
		if(rdev) {
#endif /* MY_ABC_HERE */
#ifdef MY_ABC_HERE
			seq_printf (seq, "%s", 
						test_bit(In_sync, &rdev->flags) ? 
						(test_bit(DiskError, &rdev->flags) ? "E" : "U") : "_");
#else /* MY_ABC_HERE */
			seq_printf (seq, "%s", "U");
#endif /* MY_ABC_HERE */
		} else {
			seq_printf (seq, "%s", "_");
		}
	}
	rcu_read_unlock();
	seq_printf (seq, "]");
}
#else /* MY_ABC_HERE */
static void linear_status (struct seq_file *seq, struct mddev *mddev)
{

	seq_printf(seq, " %dk rounding", mddev->chunk_sectors / 2);
}
#endif /* MY_ABC_HERE */

static void linear_quiesce(struct mddev *mddev, int state)
{
}

#ifdef MY_ABC_HERE
static int
SynoLinearRemoveDisk(struct mddev *mddev, struct md_rdev *rdev)
{
	int err = 0;
	char nm[20];
	struct linear_conf *conf = mddev->private;
	int number = rdev->raid_disk;

	if (!rdev) {
		goto END;
	}

	/*
		use the same synchronize method as RAID5
		see raid5.c:raid5_remove_disk
	*/
	conf->disks[number].rdev = NULL;
	synchronize_rcu();
	if (atomic_read(&rdev->nr_pending)) {
		/* lost the race, try later */
		err = -EBUSY;
		conf->disks[number].rdev = rdev;
		goto END;
	}

	/**
	 * Linear don't has their own thread, we just remove it's sysfs
	 * when there has no other pending request
	 */
	sprintf(nm,"rd%d", number);
	sysfs_remove_link(&mddev->kobj, nm);
	rdev->raid_disk = -1;
END:
	return err;
}

/**
 * This is our implement for raid handler.
 * It mainly for handling device hotplug.
 * We let it look like other raid type.
 * Set it faulty could let SDK know it's status
 *
 * @author \$Author: khchen $
 * @version \$Revision: 1.1
 *
 * @param mddev  Should not be NULL. passing from md.c
 * @param rdev   Should not be NULL. passing from md.c
 */
static void
SynoLinearError(struct mddev *mddev, struct md_rdev *rdev)
{
	char b[BDEVNAME_SIZE];
	printk(KERN_ALERT
		"md/raid:%s: Disk failure on %s, disabling device.\n",
		mdname(mddev), bdevname(rdev->bdev, b));
	if (test_and_clear_bit(In_sync, &rdev->flags)) {
		if (mddev->degraded < mddev->raid_disks) {
			SYNO_UPDATE_SB_WORK *update_sb = NULL;
			mddev->degraded++;
#ifdef MY_ABC_HERE
			if (MD_CRASHED_ASSEMBLE != mddev->nodev_and_crashed) {
				mddev->nodev_and_crashed = MD_CRASHED;
			}
#endif /* MY_ABC_HERE */
			set_bit(Faulty, &rdev->flags);
#ifdef MY_ABC_HERE
			clear_bit(DiskError, &rdev->flags);
#endif /* MY_ABC_HERE */

			if (NULL == (update_sb = kzalloc(sizeof(SYNO_UPDATE_SB_WORK), GFP_ATOMIC))) {
				WARN_ON(!update_sb);
				goto END;
			}

			INIT_WORK(&update_sb->work, SynoUpdateSBTask);
			update_sb->mddev = mddev;
			schedule_work(&update_sb->work);
			set_bit(MD_CHANGE_DEVS, &mddev->flags);
		}
	}
END:
	return;
}

/**
 * This is our implement for raid handler.
 * It mainly for mdadm set device faulty. We let it look like
 * other raid type. Let it become read only (scemd would remount
 * if it find DiskError)
 *
 * You should not sync super block in the same thread, otherwise
 * would panic.
 *
 * @author \$Author: khchen $
 * @version \$Revision: 1.1  *
 *
 * @param mddev  Should not be NULL. passing from md.c
 * @param rdev   Should not be NULL. passing from md.c
 */
static void
SynoLinearErrorInternal(struct mddev *mddev, struct md_rdev *rdev)
{
	char b[BDEVNAME_SIZE];
	printk(KERN_ALERT
		"md/raid:%s: Disk failure on %s, disabling device.\n",
		mdname(mddev), bdevname(rdev->bdev, b));
#ifdef MY_ABC_HERE
	if (!test_bit(DiskError, &rdev->flags)) {
		SYNO_UPDATE_SB_WORK *update_sb = NULL;

		set_bit(DiskError, &rdev->flags);
		if (NULL == (update_sb = kzalloc(sizeof(SYNO_UPDATE_SB_WORK), GFP_ATOMIC))) {
			WARN_ON(!update_sb);
			goto END;
		}

		INIT_WORK(&update_sb->work, SynoUpdateSBTask);
		update_sb->mddev = mddev;
		schedule_work(&update_sb->work);
		set_bit(MD_CHANGE_DEVS, &mddev->flags);
	}

END:
#endif /* MY_ABC_HERE */
	return;
}
#endif /* MY_ABC_HERE */

static struct md_personality linear_personality =
{
	.name		= "linear",
	.level		= LEVEL_LINEAR,
	.owner		= THIS_MODULE,
	.make_request	= linear_make_request,
	.run		= linear_run,
	.free		= linear_free,
#ifdef MY_ABC_HERE
	.status		= syno_linear_status,
#else /* MY_ABC_HERE */
	.status		= linear_status,
#endif /* MY_ABC_HERE */
	.hot_add_disk	= linear_add,
#ifdef MY_ABC_HERE
	.hot_remove_disk    = SynoLinearRemoveDisk,
	.error_handler      = SynoLinearErrorInternal,
	.syno_error_handler = SynoLinearError,
#endif /* MY_ABC_HERE */
	.size		= linear_size,
	.quiesce	= linear_quiesce,
	.congested	= linear_congested,
};

static int __init linear_init (void)
{
	return register_md_personality (&linear_personality);
}

static void linear_exit (void)
{
	unregister_md_personality (&linear_personality);
}

module_init(linear_init);
module_exit(linear_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linear device concatenation personality for MD");
MODULE_ALIAS("md-personality-1"); /* LINEAR - deprecated*/
MODULE_ALIAS("md-linear");
MODULE_ALIAS("md-level--1");
