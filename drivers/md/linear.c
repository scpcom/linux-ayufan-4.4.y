#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/blkdev.h>
#include <linux/raid/md_u.h>
#include <linux/seq_file.h>
#include "md.h"
#include "linear.h"

static inline dev_info_t *which_dev(mddev_t *mddev, sector_t sector)
{
	int lo, mid, hi;
	linear_conf_t *conf;

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

#ifdef MY_ABC_HERE
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
		 
		if (rdev->bdev->bd_disk->queue->merge_bvec_fn) {
			blk_queue_max_phys_segments(mddev->queue, 1);
			blk_queue_segment_boundary(mddev->queue,
						   PAGE_CACHE_SIZE - 1);
		}

		conf->array_sectors += rdev->sectors;
		cnt++;

	}
	if (cnt != raid_disks) {
#ifdef MY_ABC_HERE
		 
		mddev->degraded = mddev->raid_disks - cnt;		
#ifdef MY_ABC_HERE
		mddev->nodev_and_crashed = 1;
#endif
		printk("linear: not enough drives present.\n");
		return conf;
#else
		printk("linear: not enough drives present. Aborting!\n");
		goto out;
#endif
	}

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
#ifdef MY_ABC_HERE
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

	rcu_barrier();
	blk_sync_queue(mddev->queue);  
	kfree(conf);

	return 0;
}

#ifdef MY_ABC_HERE
 
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
#ifdef MY_ABC_HERE
		if (IsDeviceDisappear(rdev->bdev)) {
			syno_md_error(mddev, rdev);
		}else{
#ifdef MY_ABC_HERE
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
#ifdef MY_ABC_HERE
	struct bio *data_bio;
#endif

	if (unlikely(bio_rw_flagged(bio, BIO_RW_BARRIER))) {
		bio_endio(bio, -EOPNOTSUPP);
		return 0;
	}

#ifdef MY_ABC_HERE
	 
#ifdef MY_ABC_HERE
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

	return 1;
}

#ifdef MY_ABC_HERE

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
static void linear_status (struct seq_file *seq, mddev_t *mddev)
{

	seq_printf(seq, " %dk rounding", mddev->chunk_sectors / 2);
}

#endif  

#ifdef MY_ABC_HERE
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
SynoLinearError(mddev_t *mddev, mdk_rdev_t *rdev)
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
SynoLinearErrorInternal(mddev_t *mddev, mdk_rdev_t *rdev)
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

static struct mdk_personality linear_personality =
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
MODULE_ALIAS("md-personality-1");  
MODULE_ALIAS("md-linear");
MODULE_ALIAS("md-level--1");
