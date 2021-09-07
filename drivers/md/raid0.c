#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/blkdev.h>
#include <linux/seq_file.h>
#include "md.h"
#include "raid0.h"
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID                        
#include "hwraid.h"
#endif

static void raid0_unplug(struct request_queue *q)
{
	mddev_t *mddev = q->queuedata;
	raid0_conf_t *conf = mddev->private;
	mdk_rdev_t **devlist = conf->devlist;
	int i;

#ifndef MY_ABC_HERE
	for (i=0; i<mddev->raid_disks; i++) {
		struct request_queue *r_queue = bdev_get_queue(devlist[i]->bdev);

		blk_unplug(r_queue);
	}
#else
	rcu_read_lock();
	for (i=0; i<mddev->raid_disks; i++) {
		 
		mdk_rdev_t *rdev = rcu_dereference(devlist[i]);
		struct request_queue *r_queue = NULL;

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
	rcu_read_unlock();
#endif
}

static int raid0_congested(void *data, int bits)
{
	mddev_t *mddev = data;
	raid0_conf_t *conf = mddev->private;
	mdk_rdev_t **devlist = conf->devlist;
	int i, ret = 0;

#ifdef MY_ABC_HERE
	 
	if(mddev->degraded) {
		 
		return ret;
	}
#endif

	if (mddev_congested(mddev, bits))
		return 1;

	for (i = 0; i < mddev->raid_disks && !ret ; i++) {
		struct request_queue *q = bdev_get_queue(devlist[i]->bdev);

		ret |= bdi_congested(&q->backing_dev_info, bits);
	}
	return ret;
}

static void dump_zones(mddev_t *mddev)
{
	int j, k, h;
	sector_t zone_size = 0;
	sector_t zone_start = 0;
	char b[BDEVNAME_SIZE];
	raid0_conf_t *conf = mddev->private;
	printk(KERN_INFO "******* %s configuration *********\n",
		mdname(mddev));
	h = 0;
	for (j = 0; j < conf->nr_strip_zones; j++) {
		printk(KERN_INFO "zone%d=[", j);
		for (k = 0; k < conf->strip_zone[j].nb_dev; k++)
			printk("%s/",
			bdevname(conf->devlist[j*mddev->raid_disks
						+ k]->bdev, b));
		printk("]\n");

		zone_size  = conf->strip_zone[j].zone_end - zone_start;
		printk(KERN_INFO "        zone offset=%llukb "
				"device offset=%llukb size=%llukb\n",
			(unsigned long long)zone_start>>1,
			(unsigned long long)conf->strip_zone[j].dev_start>>1,
			(unsigned long long)zone_size>>1);
		zone_start = conf->strip_zone[j].zone_end;
	}
	printk(KERN_INFO "**********************************\n\n");
}

static int create_strip_zones(mddev_t *mddev)
{
	int i, c, err;
	sector_t curr_zone_end, sectors;
	mdk_rdev_t *smallest, *rdev1, *rdev2, *rdev, **dev;
	struct strip_zone *zone;
	int cnt;
	char b[BDEVNAME_SIZE];
	raid0_conf_t *conf = kzalloc(sizeof(*conf), GFP_KERNEL);

	if (!conf)
		return -ENOMEM;
	list_for_each_entry(rdev1, &mddev->disks, same_set) {
		printk(KERN_INFO "raid0: looking at %s\n",
			bdevname(rdev1->bdev,b));
		c = 0;

		sectors = rdev1->sectors;
		sector_div(sectors, mddev->chunk_sectors);
		rdev1->sectors = sectors * mddev->chunk_sectors;

		list_for_each_entry(rdev2, &mddev->disks, same_set) {
			printk(KERN_INFO "raid0:   comparing %s(%llu)",
			       bdevname(rdev1->bdev,b),
			       (unsigned long long)rdev1->sectors);
			printk(KERN_INFO " with %s(%llu)\n",
			       bdevname(rdev2->bdev,b),
			       (unsigned long long)rdev2->sectors);
			if (rdev2 == rdev1) {
				printk(KERN_INFO "raid0:   END\n");
				break;
			}
			if (rdev2->sectors == rdev1->sectors) {
				 
				printk(KERN_INFO "raid0:   EQUAL\n");
				c = 1;
				break;
			}
			printk(KERN_INFO "raid0:   NOT EQUAL\n");
		}
		if (!c) {
			printk(KERN_INFO "raid0:   ==> UNIQUE\n");
			conf->nr_strip_zones++;
			printk(KERN_INFO "raid0: %d zones\n",
				conf->nr_strip_zones);
		}
	}
	printk(KERN_INFO "raid0: FINAL %d zones\n", conf->nr_strip_zones);
	err = -ENOMEM;
	conf->strip_zone = kzalloc(sizeof(struct strip_zone)*
				conf->nr_strip_zones, GFP_KERNEL);
	if (!conf->strip_zone)
		goto abort;
	conf->devlist = kzalloc(sizeof(mdk_rdev_t*)*
				conf->nr_strip_zones*mddev->raid_disks,
				GFP_KERNEL);
	if (!conf->devlist)
		goto abort;

	zone = &conf->strip_zone[0];
	cnt = 0;
	smallest = NULL;
	dev = conf->devlist;
	err = -EINVAL;
	list_for_each_entry(rdev1, &mddev->disks, same_set) {
		int j = rdev1->raid_disk;

		if (j < 0 || j >= mddev->raid_disks) {
			printk(KERN_ERR "raid0: bad disk number %d - "
				"aborting!\n", j);
			goto abort;
		}
		if (dev[j]) {
			printk(KERN_ERR "raid0: multiple devices for %d - "
				"aborting!\n", j);
			goto abort;
		}
		dev[j] = rdev1;

		disk_stack_limits(mddev->gendisk, rdev1->bdev,
				  rdev1->data_offset << 9);
		 
		if (rdev1->bdev->bd_disk->queue->merge_bvec_fn) {
			blk_queue_max_phys_segments(mddev->queue, 1);
			blk_queue_segment_boundary(mddev->queue,
						   PAGE_CACHE_SIZE - 1);
		}
		if (!smallest || (rdev1->sectors < smallest->sectors))
			smallest = rdev1;
		cnt++;
	}
	if (cnt != mddev->raid_disks) {
		printk(KERN_ERR "raid0: too few disks (%d of %d) - "
			"aborting!\n", cnt, mddev->raid_disks);
#ifdef MY_ABC_HERE
		 
		mddev->degraded = mddev->raid_disks - cnt;
		zone->nb_dev = mddev->raid_disks;
		mddev->private = conf;
		return -ENOMEM;
#else
		goto abort;
#endif
	}
	zone->nb_dev = cnt;
	zone->zone_end = smallest->sectors * cnt;

	curr_zone_end = zone->zone_end;

	for (i = 1; i < conf->nr_strip_zones; i++)
	{
		int j;

		zone = conf->strip_zone + i;
		dev = conf->devlist + i * mddev->raid_disks;

		printk(KERN_INFO "raid0: zone %d\n", i);
		zone->dev_start = smallest->sectors;
		smallest = NULL;
		c = 0;

		for (j=0; j<cnt; j++) {
			rdev = conf->devlist[j];
			printk(KERN_INFO "raid0: checking %s ...",
				bdevname(rdev->bdev, b));
			if (rdev->sectors <= zone->dev_start) {
				printk(KERN_INFO " nope.\n");
				continue;
			}
			printk(KERN_INFO " contained as device %d\n", c);
			dev[c] = rdev;
			c++;
			if (!smallest || rdev->sectors < smallest->sectors) {
				smallest = rdev;
				printk(KERN_INFO "  (%llu) is smallest!.\n",
					(unsigned long long)rdev->sectors);
			}
		}

		zone->nb_dev = c;
		sectors = (smallest->sectors - zone->dev_start) * c;
		printk(KERN_INFO "raid0: zone->nb_dev: %d, sectors: %llu\n",
			zone->nb_dev, (unsigned long long)sectors);

		curr_zone_end += sectors;
		zone->zone_end = curr_zone_end;

		printk(KERN_INFO "raid0: current zone start: %llu\n",
			(unsigned long long)smallest->sectors);
	}
	mddev->queue->unplug_fn = raid0_unplug;
	mddev->queue->backing_dev_info.congested_fn = raid0_congested;
	mddev->queue->backing_dev_info.congested_data = mddev;

	if ((mddev->chunk_sectors << 9) % queue_logical_block_size(mddev->queue)) {
		printk(KERN_ERR "%s chunk_size of %d not valid\n",
		       mdname(mddev),
		       mddev->chunk_sectors << 9);
		goto abort;
	}

#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID                        
    if (hwraid0_compatible(mddev, conf)) {
	     
	    err = hwraid0_assemble(mddev, conf);
	    if (err) {
	        goto abort;
	    }
	} else
#endif  	    
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
	{
#endif
	blk_queue_io_min(mddev->queue, mddev->chunk_sectors << 9);
	blk_queue_io_opt(mddev->queue,
			 (mddev->chunk_sectors << 9) * mddev->raid_disks);
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
    }
#endif

	printk(KERN_INFO "raid0: done.\n");
	mddev->private = conf;
	return 0;
abort:
	kfree(conf->strip_zone);
	kfree(conf->devlist);
	kfree(conf);
	mddev->private = NULL;
	return err;
}

static int raid0_mergeable_bvec(struct request_queue *q,
				struct bvec_merge_data *bvm,
				struct bio_vec *biovec)
{
	mddev_t *mddev = q->queuedata;
	sector_t sector = bvm->bi_sector + get_start_sect(bvm->bi_bdev);
	int max;
	unsigned int chunk_sectors = mddev->chunk_sectors;
	unsigned int bio_sectors = bvm->bi_size >> 9;

	if (is_power_of_2(chunk_sectors))
		max =  (chunk_sectors - ((sector & (chunk_sectors-1))
						+ bio_sectors)) << 9;
	else
		max =  (chunk_sectors - (sector_div(sector, chunk_sectors)
						+ bio_sectors)) << 9;
	if (max < 0) max = 0;  
	if (max <= biovec->bv_len && bio_sectors == 0)
		return biovec->bv_len;
	else 
		return max;
}

static sector_t raid0_size(mddev_t *mddev, sector_t sectors, int raid_disks)
{
	sector_t array_sectors = 0;
	mdk_rdev_t *rdev;

	WARN_ONCE(sectors || raid_disks,
		  "%s does not support generic reshape\n", __func__);

	list_for_each_entry(rdev, &mddev->disks, same_set)
		array_sectors += rdev->sectors;

	return array_sectors;
}

static int raid0_run(mddev_t *mddev)
{
	int ret;

#ifdef MY_ABC_HERE
	mddev->degraded = 0;
#endif

	if (mddev->chunk_sectors == 0) {
		printk(KERN_ERR "md/raid0: chunk size must be set.\n");
		return -EINVAL;
	}
	if (md_check_no_bitmap(mddev))
		return -EINVAL;
	blk_queue_max_sectors(mddev->queue, mddev->chunk_sectors);
	mddev->queue->queue_lock = &mddev->queue->__queue_lock;

	ret = create_strip_zones(mddev);
#ifdef MY_ABC_HERE
	if (ret < 0) {
#ifdef MY_ABC_HERE
		if (MD_CRASHED_ASSEMBLE != mddev->nodev_and_crashed) {
			mddev->nodev_and_crashed = MD_CRASHED;
		}
#endif
			 
			mddev->array_sectors = raid0_size(mddev, 0, 0);

			return 0;
	}
#else  
	if (ret < 0)
		return ret;
#endif  

	md_set_array_sectors(mddev, raid0_size(mddev, 0, 0));

	printk(KERN_INFO "raid0 : md_size is %llu sectors.\n",
		(unsigned long long)mddev->array_sectors);
	 
	{
		int stripe = mddev->raid_disks *
			(mddev->chunk_sectors << 9) / PAGE_SIZE;
		if (mddev->queue->backing_dev_info.ra_pages < 2* stripe)
			mddev->queue->backing_dev_info.ra_pages = 2* stripe;
	}

	blk_queue_merge_bvec(mddev->queue, raid0_mergeable_bvec);
	dump_zones(mddev);
	md_integrity_register(mddev);
	return 0;
}

static int raid0_stop(mddev_t *mddev)
{
	raid0_conf_t *conf = mddev->private;

#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID                        
	if (mddev->hw_raid) {
	    hwraid0_stop(mddev, conf);
	} else
#endif	    
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
	{
#endif
	blk_sync_queue(mddev->queue);  
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
	}
#endif
	kfree(conf->strip_zone);
	kfree(conf->devlist);
	kfree(conf);
	mddev->private = NULL;
	return 0;
}

#ifdef MY_ABC_HERE
 
static void Raid0EndRequest(struct bio *bio, int error)
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
		if (-ENODEV == error) {
			syno_md_error(mddev, rdev);
		}else{
			 
#ifdef MY_ABC_HERE
			SynoReportBadSector(bio->bi_sector, bio->bi_rw, mddev->md_minor, bio->bi_bdev, __FUNCTION__);
#endif
			md_error(mddev, rdev);
		}
	}

	atomic_dec(&rdev->nr_pending);
	bio_put(data_bio);
	 
	bio_endio(bio, 0);
}
#endif  

static struct strip_zone *find_zone(struct raid0_private_data *conf,
				    sector_t *sectorp)
{
	int i;
	struct strip_zone *z = conf->strip_zone;
	sector_t sector = *sectorp;

	for (i = 0; i < conf->nr_strip_zones; i++)
		if (sector < z[i].zone_end) {
			if (i)
				*sectorp = sector - z[i-1].zone_end;
			return z + i;
		}
	BUG();
}

static mdk_rdev_t *map_sector(mddev_t *mddev, struct strip_zone *zone,
				sector_t sector, sector_t *sector_offset)
{
	unsigned int sect_in_chunk;
	sector_t chunk;
	raid0_conf_t *conf = mddev->private;
	unsigned int chunk_sects = mddev->chunk_sectors;

	if (is_power_of_2(chunk_sects)) {
		int chunksect_bits = ffz(~chunk_sects);
		 
		sect_in_chunk  = sector & (chunk_sects - 1);
		sector >>= chunksect_bits;
		 
		chunk = *sector_offset;
		 
		sector_div(chunk, zone->nb_dev << chunksect_bits);
	} else{
		sect_in_chunk = sector_div(sector, chunk_sects);
		chunk = *sector_offset;
		sector_div(chunk, chunk_sects * zone->nb_dev);
	}
	 
	*sector_offset = (chunk * chunk_sects) + sect_in_chunk;
	return conf->devlist[(zone - conf->strip_zone)*mddev->raid_disks
			     + sector_div(sector, zone->nb_dev)];
}

static inline int is_io_in_chunk_boundary(mddev_t *mddev,
			unsigned int chunk_sects, struct bio *bio)
{
	if (likely(is_power_of_2(chunk_sects))) {
		return chunk_sects >= ((bio->bi_sector & (chunk_sects-1))
					+ (bio->bi_size >> 9));
	} else{
		sector_t sector = bio->bi_sector;
		return chunk_sects >= (sector_div(sector, chunk_sects)
						+ (bio->bi_size >> 9));
	}
}

static int raid0_make_request(struct request_queue *q, struct bio *bio)
{
	mddev_t *mddev = q->queuedata;
	unsigned int chunk_sects;
	sector_t sector_offset;
	struct strip_zone *zone;
	mdk_rdev_t *tmp_dev;
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
		bio_endio(bio, -EIO);
		return 0;
	}
#endif

	chunk_sects = mddev->chunk_sectors;
	if (unlikely(!is_io_in_chunk_boundary(mddev, chunk_sects, bio))) {
		sector_t sector = bio->bi_sector;
		struct bio_pair *bp;
		 
		if (bio->bi_vcnt != 1 ||
		    bio->bi_idx != 0)
			goto bad_map;
		 
		if (likely(is_power_of_2(chunk_sects)))
			bp = bio_split(bio, chunk_sects - (sector &
							   (chunk_sects-1)));
		else
			bp = bio_split(bio, chunk_sects -
				       sector_div(sector, chunk_sects));
		if (raid0_make_request(q, &bp->bio1))
			generic_make_request(&bp->bio1);
		if (raid0_make_request(q, &bp->bio2))
			generic_make_request(&bp->bio2);

		bio_pair_release(bp);
		return 0;
	}

	sector_offset = bio->bi_sector;
	zone =  find_zone(mddev->private, &sector_offset);
	tmp_dev = map_sector(mddev, zone, bio->bi_sector,
			     &sector_offset);
	bio->bi_bdev = tmp_dev->bdev;
	bio->bi_sector = sector_offset + zone->dev_start +
		tmp_dev->data_offset;

#ifdef MY_ABC_HERE
	data_bio = bio_clone(bio, GFP_NOIO);

	if (data_bio) {
		atomic_inc(&tmp_dev->nr_pending);
		data_bio->bi_end_io = bio->bi_end_io;
		data_bio->bi_private = bio->bi_private;
		data_bio->bi_next = (void *)tmp_dev;

		bio->bi_end_io = Raid0EndRequest;
		bio->bi_private = data_bio;
	}
#endif

	return 1;

bad_map:
	printk("raid0_make_request bug: can't convert block across chunks"
		" or bigger than %dk %llu %d\n", chunk_sects / 2,
		(unsigned long long)bio->bi_sector, bio->bi_size >> 10);

	bio_io_error(bio);
	return 0;
}

#ifdef MY_ABC_HERE
static void
syno_raid0_status (struct seq_file *seq, mddev_t *mddev)
{
	int k;
	raid0_conf_t *conf = mddev->private;
	mdk_rdev_t *rdev;
	
	seq_printf(seq, " %dk chunks", mddev->chunk_sectors/2);
	seq_printf(seq, " [%d/%d] [", mddev->raid_disks, mddev->raid_disks - mddev->degraded);	
	for (k = 0; k < conf->strip_zone[0].nb_dev; k++) {
		rdev = conf->devlist[k];
		if(rdev) {
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
	seq_printf (seq, "]");
}
#else  
static void raid0_status(struct seq_file *seq, mddev_t *mddev)
{
#undef MD_DEBUG
#ifdef MD_DEBUG
	int j, k, h;
	char b[BDEVNAME_SIZE];
	raid0_conf_t *conf = mddev->private;

	sector_t zone_size;
	sector_t zone_start = 0;
	h = 0;

	for (j = 0; j < conf->nr_strip_zones; j++) {
		seq_printf(seq, "      z%d", j);
		seq_printf(seq, "=[");
		for (k = 0; k < conf->strip_zone[j].nb_dev; k++)
			seq_printf(seq, "%s/", bdevname(
				conf->devlist[j*mddev->raid_disks + k]
						->bdev, b));

		zone_size  = conf->strip_zone[j].zone_end - zone_start;
		seq_printf(seq, "] ze=%lld ds=%lld s=%lld\n",
			(unsigned long long)zone_start>>1,
			(unsigned long long)conf->strip_zone[j].dev_start>>1,
			(unsigned long long)zone_size>>1);
		zone_start = conf->strip_zone[j].zone_end;
	}
#endif
	seq_printf(seq, " %dk chunks", mddev->chunk_sectors / 2);
	return;
}
#endif  

#ifdef MY_ABC_HERE
int SynoRaid0RemoveDisk(mddev_t *mddev, int number)
{
	int err = 0;
	char nm[20];
	raid0_conf_t *conf = mddev->private;
	mdk_rdev_t *rdev;

	rdev = conf->devlist[number];
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
	conf->devlist[number] = NULL;
END:
	return err;
}

static void SynoRaid0Error(mddev_t *mddev, mdk_rdev_t *rdev)
{
	if (test_and_clear_bit(In_sync, &rdev->flags)) {
		if (mddev->degraded < mddev->raid_disks) {
			SYNO_UPDATE_SB_WORK *update_sb = NULL;
			mddev->degraded++;
#ifdef MY_ABC_HERE
			if (MD_CRASHED_ASSEMBLE != mddev->nodev_and_crashed) {
				mddev->nodev_and_crashed = MD_CRASHED;
			}
#endif
			set_bit(Faulty, &rdev->flags);
#ifdef MY_ABC_HERE
			clear_bit(DiskError, &rdev->flags);
#endif
			set_bit(MD_CHANGE_DEVS, &mddev->flags);

			if (NULL == (update_sb = kzalloc(sizeof(SYNO_UPDATE_SB_WORK), GFP_ATOMIC))){
				WARN_ON(!update_sb);
				goto END;
			}

			INIT_WORK(&update_sb->work, SynoUpdateSBTask);
			update_sb->mddev = mddev;
			schedule_work(&update_sb->work);
		}
	}else{
		set_bit(Faulty, &rdev->flags);
	}
END:
	return;
}

static void SynoRaid0ErrorInternal(mddev_t *mddev, mdk_rdev_t *rdev)
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

static struct mdk_personality raid0_personality=
{
	.name		= "raid0",
	.level		= 0,
	.owner		= THIS_MODULE,
	.make_request	= raid0_make_request,
	.run		= raid0_run,
	.stop		= raid0_stop,
#ifdef MY_ABC_HERE
	.status		= syno_raid0_status,
#else
	.status		= raid0_status,
#endif
	.size		= raid0_size,
#ifdef MY_ABC_HERE
	.hot_remove_disk = SynoRaid0RemoveDisk,
	.error_handler = SynoRaid0ErrorInternal,
	.syno_error_handler	 = SynoRaid0Error,
#endif
};

static int __init raid0_init (void)
{
	return register_md_personality (&raid0_personality);
}

static void raid0_exit (void)
{
	unregister_md_personality (&raid0_personality);
}

module_init(raid0_init);
module_exit(raid0_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS("md-personality-2");  
MODULE_ALIAS("md-raid0");
MODULE_ALIAS("md-level-0");
