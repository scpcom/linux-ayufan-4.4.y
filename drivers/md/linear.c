#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/blkdev.h>
#include <linux/raid/md_u.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "md.h"
#include "linear.h"

static inline struct dev_info *which_dev(struct mddev *mddev, sector_t sector)
{
	int lo, mid, hi;
	struct linear_conf *conf;

	lo = 0;
	hi = mddev->raid_disks - 1;
	conf = rcu_dereference(mddev->private);

	while (hi > lo) {

		mid = (hi + lo) / 2;
		if (sector < conf->disks[mid].end_sector)
			hi = mid;
		else
			lo = mid + 1;
	}

	return conf->disks + lo;
}

static int linear_mergeable_bvec(struct request_queue *q,
				 struct bvec_merge_data *bvm,
				 struct bio_vec *biovec)
{
	struct mddev *mddev = q->queuedata;
	struct dev_info *dev0;
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
	 
	if (maxsectors > (1 << (31-9)))
		return 1<<31;
	return maxsectors << 9;
}

static int linear_congested(void *data, int bits)
{
	struct mddev *mddev = data;
	struct linear_conf *conf;
	int i, ret = 0;

#ifdef MY_ABC_HERE
	if (mddev->degraded) {
		return ret;
	}
#endif

	if (mddev_congested(mddev, bits))
		return 1;

	rcu_read_lock();
	conf = rcu_dereference(mddev->private);

#ifdef MY_ABC_HERE
	for (i = 0; i < mddev->raid_disks && !ret ; i++) {
		struct md_rdev *rdev = rcu_dereference(conf->disks[i].rdev);
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

static sector_t linear_size(struct mddev *mddev, sector_t sectors, int raid_disks)
{
	struct linear_conf *conf;
	sector_t array_sectors;

	rcu_read_lock();
	conf = rcu_dereference(mddev->private);
	WARN_ONCE(sectors || raid_disks,
		  "%s does not support generic reshape\n", __func__);
	array_sectors = conf->array_sectors;
	rcu_read_unlock();

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

	list_for_each_entry(rdev, &mddev->disks, same_set) {
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
		 
		if (rdev->bdev->bd_disk->queue->merge_bvec_fn) {
			blk_queue_max_segments(mddev->queue, 1);
			blk_queue_segment_boundary(mddev->queue,
						   PAGE_CACHE_SIZE - 1);
		}

		conf->array_sectors += rdev->sectors;
		cnt++;

		if (blk_queue_discard(bdev_get_queue(rdev->bdev)))
			discard_supported = true;
	}
	if (cnt != raid_disks) {
#ifdef MY_ABC_HERE
		 
		mddev->degraded = mddev->raid_disks - cnt;		
#ifdef MY_ABC_HERE
		mddev->nodev_and_crashed = 1;
#endif
		printk(KERN_ERR "md/linear:%s: not enough drives present.\n",
		       mdname(mddev));
		return conf;
#else
		printk(KERN_ERR "md/linear:%s: not enough drives present. Aborting!\n",
		       mdname(mddev));
		goto out;
#endif
	}

	if (!discard_supported)
		queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, mddev->queue);
	else
		queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, mddev->queue);

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

static int linear_run (struct mddev *mddev)
{
	struct linear_conf *conf;

	if (md_check_no_bitmap(mddev))
		return -EINVAL;
#ifdef MY_ABC_HERE
	mddev->degraded = 0;
#endif
	conf = linear_conf(mddev, mddev->raid_disks);

	if (!conf)
		return 1;
	mddev->private = conf;
	md_set_array_sectors(mddev, linear_size(mddev, 0, 0));

	blk_queue_merge_bvec(mddev->queue, linear_mergeable_bvec);
	mddev->queue->backing_dev_info.congested_fn = linear_congested;
	mddev->queue->backing_dev_info.congested_data = mddev;
	return md_integrity_register(mddev);
}

static int linear_add(struct mddev *mddev, struct md_rdev *rdev)
{
	 
	struct linear_conf *newconf, *oldconf;

	if (rdev->saved_raid_disk != mddev->raid_disks)
		return -EINVAL;

	rdev->raid_disk = rdev->saved_raid_disk;
	rdev->saved_raid_disk = -1;

	newconf = linear_conf(mddev,mddev->raid_disks+1);

	if (!newconf)
		return -ENOMEM;

	oldconf = rcu_dereference(mddev->private);
	mddev->raid_disks++;
	rcu_assign_pointer(mddev->private, newconf);
	md_set_array_sectors(mddev, linear_size(mddev, 0, 0));
	set_capacity(mddev->gendisk, mddev->array_sectors);
	revalidate_disk(mddev->gendisk);
	kfree_rcu(oldconf, rcu);
	return 0;
}

static int linear_stop (struct mddev *mddev)
{
	struct linear_conf *conf = mddev->private;

	rcu_barrier();
	blk_sync_queue(mddev->queue);  
	kfree(conf);
	mddev->private = NULL;

	return 0;
}

#ifdef MY_ABC_HERE
 
static void
SynoLinearEndRequest(struct bio *bio, int error)
{
	int uptodate = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct mddev *mddev = NULL;
    struct md_rdev *rdev = NULL;
	struct bio *data_bio;

	data_bio = bio->bi_private;

	rdev = (struct md_rdev *)data_bio->bi_next;
	mddev = rdev->mddev;

	bio->bi_end_io = data_bio->bi_end_io;
	bio->bi_private = data_bio->bi_private;

	if (!uptodate) {
#ifdef MY_ABC_HERE
		if (IsDeviceDisappear(rdev->bdev)) {
			syno_md_error(mddev, rdev);
		}else{
#ifdef MY_ABC_HERE
#ifdef MY_ABC_HERE
			if (bio_flagged(bio, BIO_AUTO_REMAP)) {
				SynoReportBadSector(bio->bi_sector, bio->bi_rw, mddev->md_minor, bio->bi_bdev, __FUNCTION__);
			}
#else
			SynoReportBadSector(bio->bi_sector, bio->bi_rw, mddev->md_minor, bio->bi_bdev, __FUNCTION__);
#endif
#endif
			md_error(mddev, rdev);
		}
#else
		md_error(mddev, rdev);
#endif
	}

	atomic_dec(&rdev->nr_pending);
	bio_put(data_bio);
	 
	bio_endio(bio, 0);
}
#endif

static void linear_make_request(struct mddev *mddev, struct bio *bio)
{
	struct dev_info *tmp_dev;
	sector_t start_sector;
#ifdef MY_ABC_HERE
	struct bio *data_bio;
#endif

	if (unlikely(bio->bi_rw & REQ_FLUSH)) {
		md_flush_request(mddev, bio);
		return;
	}

#ifdef MY_ABC_HERE
	 
#ifdef MY_ABC_HERE
	if (mddev->nodev_and_crashed) {
#else
	if (mddev->degraded) {
#endif
		bio_endio(bio, 0);
		return;
	}
#endif

	rcu_read_lock();
	tmp_dev = which_dev(mddev, bio->bi_sector);
	start_sector = tmp_dev->end_sector - tmp_dev->rdev->sectors;

	if (unlikely(bio->bi_sector >= (tmp_dev->end_sector)
		     || (bio->bi_sector < start_sector))) {
		char b[BDEVNAME_SIZE];

		printk(KERN_ERR
		       "md/linear:%s: make_request: Sector %llu out of bounds on "
		       "dev %s: %llu sectors, offset %llu\n",
		       mdname(mddev),
		       (unsigned long long)bio->bi_sector,
		       bdevname(tmp_dev->rdev->bdev, b),
		       (unsigned long long)tmp_dev->rdev->sectors,
		       (unsigned long long)start_sector);
		rcu_read_unlock();
		bio_io_error(bio);
		return;
	}
	if (unlikely(bio->bi_sector + (bio->bi_size >> 9) >
		     tmp_dev->end_sector)) {
		 
		struct bio_pair *bp;
		sector_t end_sector = tmp_dev->end_sector;

		rcu_read_unlock();

		bp = bio_split(bio, end_sector - bio->bi_sector);

		linear_make_request(mddev, &bp->bio1);
		linear_make_request(mddev, &bp->bio2);
		bio_pair_release(bp);
		return;
	}
		    
	bio->bi_bdev = tmp_dev->rdev->bdev;
	bio->bi_sector = bio->bi_sector - start_sector
		+ tmp_dev->rdev->data_offset;

#ifdef MY_ABC_HERE
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

	if (unlikely((bio->bi_rw & REQ_DISCARD) &&
		     !blk_queue_discard(bdev_get_queue(bio->bi_bdev)))) {
		 
		bio_endio(bio, 0);
		return;
	}

	generic_make_request(bio);
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
		if(rdev &&
		   !test_bit(Faulty, &rdev->flags)) {
#else		
		if(rdev) {
#endif
#ifdef MY_ABC_HERE
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
#else  
static void linear_status (struct seq_file *seq, struct mddev *mddev)
{

	seq_printf(seq, " %dk rounding", mddev->chunk_sectors / 2);
}

#endif  

#ifdef MY_ABC_HERE
int
SynoLinearRemoveDisk(struct mddev *mddev, int number)
{
	int err = 0;
	char nm[20];
	struct linear_conf *conf = mddev->private;
	struct md_rdev *rdev = NULL;

	rdev = conf->disks[number].rdev;
	if (!rdev) {
		goto END;
	}

	if (atomic_read(&rdev->nr_pending)) {
		 
		err = -EBUSY;
		goto END;
	}

	sprintf(nm,"rd%d", rdev->raid_disk);
	sysfs_remove_link(&mddev->kobj, nm);
	rdev->raid_disk = -1;
	conf->disks[number].rdev = NULL;
END:
	return err;
}

static void
SynoLinearError(struct mddev *mddev, struct md_rdev *rdev)
{
	if (test_and_clear_bit(In_sync, &rdev->flags)) {
		if (mddev->degraded < mddev->raid_disks) {
			SYNO_UPDATE_SB_WORK *update_sb = NULL;
			mddev->degraded++;
#ifdef MY_ABC_HERE
			mddev->nodev_and_crashed = 1;
#endif
			set_bit(Faulty, &rdev->flags);
#ifdef MY_ABC_HERE
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

static void
SynoLinearErrorInternal(struct mddev *mddev, struct md_rdev *rdev)
{
#ifdef MY_ABC_HERE
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
#endif  

static struct md_personality linear_personality =
{
	.name		= "linear",
	.level		= LEVEL_LINEAR,
	.owner		= THIS_MODULE,
	.make_request	= linear_make_request,
	.run		= linear_run,
	.stop		= linear_stop,
#ifdef MY_ABC_HERE
	.status		= syno_linear_status,
#else
	.status		= linear_status,
#endif
	.hot_add_disk	= linear_add,
#ifdef MY_ABC_HERE
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
MODULE_DESCRIPTION("Linear device concatenation personality for MD");
MODULE_ALIAS("md-personality-1");  
MODULE_ALIAS("md-linear");
MODULE_ALIAS("md-level--1");
