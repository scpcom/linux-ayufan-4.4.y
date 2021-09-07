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
#include "md.h"
#include "linear.h"

/*
 * find which device holds a particular offset 
 */
static inline dev_info_t *which_dev(mddev_t *mddev, sector_t sector)
{
	int lo, mid, hi;
	linear_conf_t *conf;

	lo = 0;
	hi = mddev->raid_disks - 1;
	conf = rcu_dereference(mddev->private);

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

/**
 *	linear_mergeable_bvec -- tell bio layer if two requests can be merged
 *	@q: request queue
 *	@bvm: properties of new bio
 *	@biovec: the request that could be merged to it.
 *
 *	Return amount of bytes we can take at this offset
 */
static int linear_mergeable_bvec(struct request_queue *q,
				 struct bvec_merge_data *bvm,
				 struct bio_vec *biovec)
{
	mddev_t *mddev = q->queuedata;
	dev_info_t *dev0;
	unsigned long maxsectors, bio_sectors = bvm->bi_size >> 9;
	sector_t sector = bvm->bi_sector + get_start_sect(bvm->bi_bdev);

	rcu_read_lock();
	dev0 = which_dev(mddev, sector);
	maxsectors = dev0->end_sector - sector;
	rcu_read_unlock();

	if (maxsectors < bio_sectors)
		maxsectors = 0;
	else
		maxsectors -= bio_sectors;

	if (maxsectors <= (PAGE_SIZE >> 9 ) && bio_sectors == 0)
		return biovec->bv_len;
	/* The bytes available at this offset could be really big,
	 * so we cap at 2^31 to avoid overflow */
	if (maxsectors > (1 << (31-9)))
		return 1<<31;
	return maxsectors << 9;
}

static void linear_unplug(struct request_queue *q)
{
	mddev_t *mddev = q->queuedata;
	linear_conf_t *conf;
	int i;

	rcu_read_lock();
	conf = rcu_dereference(mddev->private);

#ifdef SYNO_RAID_DEVICE_NOTIFY
	for (i=0; i < mddev->raid_disks; i++) {
		struct request_queue *r_queue;
		mdk_rdev_t *rdev = rcu_dereference(conf->disks[i].rdev);

		if(!rdev ||
		   test_bit(Faulty, &rdev->flags) ||
		   !atomic_read(&rdev->nr_pending)) {
			continue;
		}

		r_queue = bdev_get_queue(rdev->bdev);

		atomic_inc(&rdev->nr_pending);
		rcu_read_unlock();

		blk_unplug(r_queue);
		atomic_dec(&rdev->nr_pending);
		rcu_read_lock();
	}
#else
	for (i=0; i < mddev->raid_disks; i++) {
		struct request_queue *r_queue = bdev_get_queue(conf->disks[i].rdev->bdev);
		blk_unplug(r_queue);
	}
#endif
	rcu_read_unlock();
}

static int linear_congested(void *data, int bits)
{
	mddev_t *mddev = data;
	linear_conf_t *conf;
	int i, ret = 0;

#ifdef SYNO_RAID_DEVICE_NOTIFY
	if (mddev->degraded) {
		return ret;
	}
#endif

	if (mddev_congested(mddev, bits))
		return 1;

	rcu_read_lock();
	conf = rcu_dereference(mddev->private);

#ifdef SYNO_RAID_DEVICE_NOTIFY
	for (i = 0; i < mddev->raid_disks && !ret ; i++) {
		mdk_rdev_t *rdev = rcu_dereference(conf->disks[i].rdev);
		struct request_queue *q = NULL;

		if (!rdev) {
			continue;
		}

		q = bdev_get_queue(rdev->bdev);
		ret |= bdi_congested(&q->backing_dev_info, bits);
	}
#else
	for (i = 0; i < mddev->raid_disks && !ret ; i++) {
		struct request_queue *q = bdev_get_queue(conf->disks[i].rdev->bdev);
		ret |= bdi_congested(&q->backing_dev_info, bits);
	}
#endif

	rcu_read_unlock();
	return ret;
}

static sector_t linear_size(mddev_t *mddev, sector_t sectors, int raid_disks)
{
	linear_conf_t *conf;
	sector_t array_sectors;

	rcu_read_lock();
	conf = rcu_dereference(mddev->private);
	WARN_ONCE(sectors || raid_disks,
		  "%s does not support generic reshape\n", __func__);
	array_sectors = conf->array_sectors;
	rcu_read_unlock();

	return array_sectors;
}

static linear_conf_t *linear_conf(mddev_t *mddev, int raid_disks)
{
	linear_conf_t *conf;
	mdk_rdev_t *rdev;
	int i, cnt;

	conf = kzalloc (sizeof (*conf) + raid_disks*sizeof(dev_info_t),
			GFP_KERNEL);
	if (!conf)
		return NULL;

	cnt = 0;
	conf->array_sectors = 0;

	list_for_each_entry(rdev, &mddev->disks, same_set) {
		int j = rdev->raid_disk;
		dev_info_t *disk = conf->disks + j;
		sector_t sectors;

		if (j < 0 || j >= raid_disks || disk->rdev) {
			printk("linear: disk numbering problem. Aborting!\n");
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
		/* as we don't honour merge_bvec_fn, we must never risk
		 * violating it, so limit max_phys_segments to 1 lying within
		 * a single page.
		 */
		if (rdev->bdev->bd_disk->queue->merge_bvec_fn) {
			blk_queue_max_phys_segments(mddev->queue, 1);
			blk_queue_segment_boundary(mddev->queue,
						   PAGE_CACHE_SIZE - 1);
		}

		conf->array_sectors += rdev->sectors;
		cnt++;

	}
	if (cnt != raid_disks) {
#ifdef SYNO_RAID_STATUS
		/* 
		 * for Linear status consistense to other raid type
		 * Let it can assemble.
		 */
		mddev->degraded = mddev->raid_disks - cnt;		
#ifdef SYNO_BLOCK_REQUEST_ERROR_NODEV
		mddev->nodev_and_crashed = 1;
#endif
		printk("linear: not enough drives present.\n");
		return conf;
#else
		printk("linear: not enough drives present. Aborting!\n");
		goto out;
#endif
	}

	/*
	 * Here we calculate the device offsets.
	 */
	conf->disks[0].end_sector = conf->disks[0].rdev->sectors;

	for (i = 1; i < raid_disks; i++)
		conf->disks[i].end_sector =
			conf->disks[i-1].end_sector +
			conf->disks[i].rdev->sectors;

	return conf;

out:
	kfree(conf);
	return NULL;
}

static int linear_run (mddev_t *mddev)
{
	linear_conf_t *conf;

	if (md_check_no_bitmap(mddev))
		return -EINVAL;
	mddev->queue->queue_lock = &mddev->queue->__queue_lock;
#ifdef SYNO_RAID_STATUS
	mddev->degraded = 0;
#endif
	conf = linear_conf(mddev, mddev->raid_disks);

	if (!conf)
		return 1;
	mddev->private = conf;
	md_set_array_sectors(mddev, linear_size(mddev, 0, 0));

	blk_queue_merge_bvec(mddev->queue, linear_mergeable_bvec);
	mddev->queue->unplug_fn = linear_unplug;
	mddev->queue->backing_dev_info.congested_fn = linear_congested;
	mddev->queue->backing_dev_info.congested_data = mddev;
	md_integrity_register(mddev);
	return 0;
}

static void free_conf(struct rcu_head *head)
{
	linear_conf_t *conf = container_of(head, linear_conf_t, rcu);
	kfree(conf);
}

static int linear_add(mddev_t *mddev, mdk_rdev_t *rdev)
{
	/* Adding a drive to a linear array allows the array to grow.
	 * It is permitted if the new drive has a matching superblock
	 * already on it, with raid_disk equal to raid_disks.
	 * It is achieved by creating a new linear_private_data structure
	 * and swapping it in in-place of the current one.
	 * The current one is never freed until the array is stopped.
	 * This avoids races.
	 */
	linear_conf_t *newconf, *oldconf;

	if (rdev->saved_raid_disk != mddev->raid_disks)
		return -EINVAL;

	rdev->raid_disk = rdev->saved_raid_disk;

	newconf = linear_conf(mddev,mddev->raid_disks+1);

	if (!newconf)
		return -ENOMEM;

	oldconf = rcu_dereference(mddev->private);
	mddev->raid_disks++;
	rcu_assign_pointer(mddev->private, newconf);
	md_set_array_sectors(mddev, linear_size(mddev, 0, 0));
	set_capacity(mddev->gendisk, mddev->array_sectors);
	revalidate_disk(mddev->gendisk);
	call_rcu(&oldconf->rcu, free_conf);
	return 0;
}

static int linear_stop (mddev_t *mddev)
{
	linear_conf_t *conf = mddev->private;

	/*
	 * We do not require rcu protection here since
	 * we hold reconfig_mutex for both linear_add and
	 * linear_stop, so they cannot race.
	 * We should make sure any old 'conf's are properly
	 * freed though.
	 */
	rcu_barrier();
	blk_sync_queue(mddev->queue); /* the unplug fn references 'conf'*/
	kfree(conf);

	return 0;
}

#ifdef SYNO_RAID_DEVICE_NOTIFY
/**
 * This is end_io callback function.
 * We can use this for bad sector report and device error
 * handing. Prevent umount panic from file system
 *
 * @author \$Author: ckya $
 * @version \$Revision: 1.1
 *
 * @param bio    Should not be NULL. Passing from block layer
 * @param error  error number
 */
static void
SynoLinearEndRequest(struct bio *bio, int error)
{
	int uptodate = test_bit(BIO_UPTODATE, &bio->bi_flags);
	mddev_t *mddev;
	mdk_rdev_t *rdev;
	struct bio *data_bio;

	data_bio = bio->bi_private;

	rdev = (mdk_rdev_t *)data_bio->bi_next;
	mddev = rdev->mddev;

	bio->bi_end_io = data_bio->bi_end_io;
	bio->bi_private = data_bio->bi_private;

	if (!uptodate) {
#ifdef SYNO_BLOCK_REQUEST_ERROR_NODEV
		if (IsDeviceDisappear(rdev->bdev)) {
			syno_md_error(mddev, rdev);
		}else{
#ifdef SYNO_RAID_SECTOR_STATUS_REPORT
			SynoReportBadSector(bio->bi_sector, bio->bi_rw, mddev->md_minor, bio->bi_bdev, __FUNCTION__);			
#endif
			md_error(mddev, rdev);
		}
#else
		md_error(mddev, rdev);
#endif
	}

	atomic_dec(&rdev->nr_pending);
	bio_put(data_bio);
	/* Let mount could successful and bad sector could keep accessing, no matter it success or not */
	bio_endio(bio, 0);
}
#endif

static int linear_make_request (struct request_queue *q, struct bio *bio)
{
	const int rw = bio_data_dir(bio);
	mddev_t *mddev = q->queuedata;
	dev_info_t *tmp_dev;
	sector_t start_sector;
	int cpu;
#ifdef SYNO_RAID_DEVICE_NOTIFY
	struct bio *data_bio;
#endif

	if (unlikely(bio_rw_flagged(bio, BIO_RW_BARRIER))) {
		bio_endio(bio, -EOPNOTSUPP);
		return 0;
	}

#ifdef SYNO_RAID_STATUS
	/**
	* if there has any device offline, we don't make any request to
	* our linear md array
	*/
#ifdef SYNO_BLOCK_REQUEST_ERROR_NODEV
	if (mddev->nodev_and_crashed) {
#else
	if (mddev->degraded) {
#endif
		bio_endio(bio, 0);
		return 0;
	}
#endif

	cpu = part_stat_lock();
	part_stat_inc(cpu, &mddev->gendisk->part0, ios[rw]);
	part_stat_add(cpu, &mddev->gendisk->part0, sectors[rw],
		      bio_sectors(bio));
	part_stat_unlock();

	rcu_read_lock();
	tmp_dev = which_dev(mddev, bio->bi_sector);
	start_sector = tmp_dev->end_sector - tmp_dev->rdev->sectors;


	if (unlikely(bio->bi_sector >= (tmp_dev->end_sector)
		     || (bio->bi_sector < start_sector))) {
		char b[BDEVNAME_SIZE];

		printk("linear_make_request: Sector %llu out of bounds on "
			"dev %s: %llu sectors, offset %llu\n",
			(unsigned long long)bio->bi_sector,
			bdevname(tmp_dev->rdev->bdev, b),
			(unsigned long long)tmp_dev->rdev->sectors,
			(unsigned long long)start_sector);
		rcu_read_unlock();
		bio_io_error(bio);
		return 0;
	}
	if (unlikely(bio->bi_sector + (bio->bi_size >> 9) >
		     tmp_dev->end_sector)) {
		/* This bio crosses a device boundary, so we have to
		 * split it.
		 */
		struct bio_pair *bp;
		sector_t end_sector = tmp_dev->end_sector;

		rcu_read_unlock();

		bp = bio_split(bio, end_sector - bio->bi_sector);

		if (linear_make_request(q, &bp->bio1))
			generic_make_request(&bp->bio1);
		if (linear_make_request(q, &bp->bio2))
			generic_make_request(&bp->bio2);
		bio_pair_release(bp);
		return 0;
	}
		    
	bio->bi_bdev = tmp_dev->rdev->bdev;
	bio->bi_sector = bio->bi_sector - start_sector
		+ tmp_dev->rdev->data_offset;

#ifdef SYNO_RAID_DEVICE_NOTIFY
	data_bio = bio_clone(bio, GFP_NOIO);

	if (data_bio) {
		atomic_inc(&tmp_dev->rdev->nr_pending);
		data_bio->bi_end_io = bio->bi_end_io;
		data_bio->bi_private = bio->bi_private;
		data_bio->bi_next = (void *)tmp_dev->rdev;

		bio->bi_end_io = SynoLinearEndRequest;
		bio->bi_private = data_bio;
	}
#endif

	rcu_read_unlock();

	return 1;
}

#ifdef SYNO_RAID_STATUS

static void
syno_linear_status(struct seq_file *seq, mddev_t *mddev)
{	
	linear_conf_t *conf;
	mdk_rdev_t *rdev;
	int j;

	seq_printf(seq, " %dk rounding", mddev->chunk_sectors / 2);
	seq_printf(seq, " [%d/%d] [", mddev->raid_disks, mddev->raid_disks - mddev->degraded);
	rcu_read_lock();
	conf = rcu_dereference(mddev->private);
	for (j = 0; j < mddev->raid_disks; j++)
	{
		rdev = rcu_dereference(conf->disks[j].rdev);
#ifdef SYNO_RAID_DEVICE_NOTIFY
		if(rdev &&
		   !test_bit(Faulty, &rdev->flags)) {
#else		
		if(rdev) {
#endif
#ifdef SYNO_RAID_STATUS_DISKERROR
			seq_printf (seq, "%s", 
						test_bit(In_sync, &rdev->flags) ? 
						(test_bit(DiskError, &rdev->flags) ? "E" : "U") : "_");
#else
			seq_printf (seq, "%s", "U");
#endif
		}else{
			seq_printf (seq, "%s", "_");
		}
	}
	rcu_read_unlock();
	seq_printf (seq, "]");
}
#else /* SYNO_RAID_STATUS */
static void linear_status (struct seq_file *seq, mddev_t *mddev)
{

	seq_printf(seq, " %dk rounding", mddev->chunk_sectors / 2);
}

#endif /* SYNO_RAID_STATUS */

#ifdef SYNO_RAID_DEVICE_NOTIFY
int
SynoLinearRemoveDisk(mddev_t *mddev, int number)
{
	int err = 0;
	char nm[20];
	linear_conf_t *conf = mddev->private;
	mdk_rdev_t *rdev;

	rdev = conf->disks[number].rdev;
	if (!rdev) {
		goto END;
	}

	if (atomic_read(&rdev->nr_pending)) {
		/* lost the race, try later */
		err = -EBUSY;
		goto END;
	}

	/**
	 * Linear don't has their own thread, we just remove it's sysfs
	 * when there has no other pending request
	 */
	sprintf(nm,"rd%d", rdev->raid_disk);
	sysfs_remove_link(&mddev->kobj, nm);
	rdev->raid_disk = -1;
	conf->disks[number].rdev = NULL;
END:
	return err;
}

/**
 * This is our implement for raid handler.
 * It mainly for handling device hotplug.
 * We let it look like other raid type.
 * Set it faulty could let SDK know it's status
 *
 * @author \$Author: ckya $
 * @version \$Revision: 1.1
 *
 * @param mddev  Should not be NULL. passing from md.c
 * @param rdev   Should not be NULL. passing from md.c
 */
static void
SynoLinearError(mddev_t *mddev, mdk_rdev_t *rdev)
{
	if (test_and_clear_bit(In_sync, &rdev->flags)) {
		if (mddev->degraded < mddev->raid_disks) {
			SYNO_UPDATE_SB_WORK *update_sb = NULL;
			mddev->degraded++;
#ifdef SYNO_BLOCK_REQUEST_ERROR_NODEV
			mddev->nodev_and_crashed = 1;
#endif
			set_bit(Faulty, &rdev->flags);
#ifdef SYNO_RAID_STATUS_DISKERROR
			clear_bit(DiskError, &rdev->flags);
#endif

			if (NULL == (update_sb = kzalloc(sizeof(SYNO_UPDATE_SB_WORK), GFP_ATOMIC))){
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
 * @author \$Author: ckya $
 * @version \$Revision: 1.1  *
 *
 * @param mddev  Should not be NULL. passing from md.c
 * @param rdev   Should not be NULL. passing from md.c
 */
static void
SynoLinearErrorInternal(mddev_t *mddev, mdk_rdev_t *rdev)
{
#ifdef SYNO_RAID_STATUS_DISKERROR
	if (!test_bit(DiskError, &rdev->flags)) {
		SYNO_UPDATE_SB_WORK *update_sb = NULL;

		set_bit(DiskError, &rdev->flags);
		if (NULL == (update_sb = kzalloc(sizeof(SYNO_UPDATE_SB_WORK), GFP_ATOMIC))){
			WARN_ON(!update_sb);
			goto END;
		}

		INIT_WORK(&update_sb->work, SynoUpdateSBTask);
		update_sb->mddev = mddev;
		schedule_work(&update_sb->work);
		set_bit(MD_CHANGE_DEVS, &mddev->flags);
	}

END:
#endif
	return;
}
#endif /* SYNO_RAID_DEVICE_NOTIFY */

static struct mdk_personality linear_personality =
{
	.name		= "linear",
	.level		= LEVEL_LINEAR,
	.owner		= THIS_MODULE,
	.make_request	= linear_make_request,
	.run		= linear_run,
	.stop		= linear_stop,
#ifdef SYNO_RAID_STATUS
	.status		= syno_linear_status,
#else
	.status		= linear_status,
#endif
	.hot_add_disk	= linear_add,
#ifdef SYNO_RAID_DEVICE_NOTIFY
	.hot_remove_disk	= SynoLinearRemoveDisk,
	.error_handler		= SynoLinearErrorInternal,
	.syno_error_handler	= SynoLinearError,
#endif
	.size		= linear_size,
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
MODULE_ALIAS("md-personality-1"); /* LINEAR - deprecated*/
MODULE_ALIAS("md-linear");
MODULE_ALIAS("md-level--1");
