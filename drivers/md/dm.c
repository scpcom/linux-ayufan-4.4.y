#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "dm.h"
#include "dm-uevent.h"
#ifdef MY_ABC_HERE
#include "md.h"
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/moduleparam.h>
#include <linux/blkpg.h>
#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/mempool.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/hdreg.h>

#include <trace/events/block.h>

#define DM_MSG_PREFIX "core"

#define DM_COOKIE_ENV_VAR_NAME "DM_COOKIE"
#define DM_COOKIE_LENGTH 24
#ifdef MY_ABC_HERE
void SynoMDWakeUpDevices(void *md);
#ifdef MY_ABC_HERE
extern int SynoDebugFlag;
#endif
#endif

static const char *_name = DM_NAME;

static unsigned int major = 0;
static unsigned int _major = 0;

#ifdef MY_ABC_HERE
extern sector_t (*funcSYNOLvLgSectorCount)(void *private, sector_t sector);
sector_t SynoLvLgSectorCount(void *, sector_t);
#endif

static DEFINE_SPINLOCK(_minor_lock);
 
struct dm_io {
	struct mapped_device *md;
	int error;
	atomic_t io_count;
	struct bio *bio;
	unsigned long start_time;
	spinlock_t endio_lock;
};

struct dm_target_io {
	struct dm_io *io;
	struct dm_target *ti;
	union map_info info;
};

struct dm_rq_target_io {
	struct mapped_device *md;
	struct dm_target *ti;
	struct request *orig, clone;
	int error;
	union map_info info;
};

struct dm_rq_clone_bio_info {
	struct bio *orig;
	struct dm_rq_target_io *tio;
};

union map_info *dm_get_mapinfo(struct bio *bio)
{
	if (bio && bio->bi_private)
		return &((struct dm_target_io *)bio->bi_private)->info;
	return NULL;
}

union map_info *dm_get_rq_mapinfo(struct request *rq)
{
	if (rq && rq->end_io_data)
		return &((struct dm_rq_target_io *)rq->end_io_data)->info;
	return NULL;
}
EXPORT_SYMBOL_GPL(dm_get_rq_mapinfo);

#define MINOR_ALLOCED ((void *)-1)

#define DMF_BLOCK_IO_FOR_SUSPEND 0
#define DMF_SUSPENDED 1
#define DMF_FROZEN 2
#define DMF_FREEING 3
#define DMF_DELETING 4
#define DMF_NOFLUSH_SUSPENDING 5
#define DMF_QUEUE_IO_TO_THREAD 6

struct mapped_device {
	struct rw_semaphore io_lock;
	struct mutex suspend_lock;
	rwlock_t map_lock;
	atomic_t holders;
	atomic_t open_count;

	unsigned long flags;

	struct request_queue *queue;
	struct gendisk *disk;
	char name[16];

	void *interface_ptr;

	atomic_t pending[2];
	wait_queue_head_t wait;
	struct work_struct work;
	struct bio_list deferred;
	spinlock_t deferred_lock;

	int barrier_error;

	struct workqueue_struct *wq;

	struct dm_table *map;

	mempool_t *io_pool;
	mempool_t *tio_pool;

	struct bio_set *bs;

	atomic_t event_nr;
	wait_queue_head_t eventq;
	atomic_t uevent_seq;
	struct list_head uevent_list;
	spinlock_t uevent_lock;  

	struct super_block *frozen_sb;
	struct block_device *bdev;

	struct hd_geometry geometry;

	struct request suspend_rq;

	make_request_fn *saved_make_request_fn;

	struct kobject kobj;

	struct bio barrier_bio;
#ifdef MY_ABC_HERE
	 
	int blActive;

	spinlock_t	ActLock;

	unsigned long ulLastReq;
#endif
};

struct dm_md_mempools {
	mempool_t *io_pool;
	mempool_t *tio_pool;
	struct bio_set *bs;
};

#define MIN_IOS 256
static struct kmem_cache *_io_cache;
static struct kmem_cache *_tio_cache;
static struct kmem_cache *_rq_tio_cache;
static struct kmem_cache *_rq_bio_info_cache;

static int __init local_init(void)
{
	int r = -ENOMEM;

	_io_cache = KMEM_CACHE(dm_io, 0);
	if (!_io_cache)
		return r;

	_tio_cache = KMEM_CACHE(dm_target_io, 0);
	if (!_tio_cache)
		goto out_free_io_cache;

	_rq_tio_cache = KMEM_CACHE(dm_rq_target_io, 0);
	if (!_rq_tio_cache)
		goto out_free_tio_cache;

	_rq_bio_info_cache = KMEM_CACHE(dm_rq_clone_bio_info, 0);
	if (!_rq_bio_info_cache)
		goto out_free_rq_tio_cache;

	r = dm_uevent_init();
	if (r)
		goto out_free_rq_bio_info_cache;

	_major = major;
	r = register_blkdev(_major, _name);
	if (r < 0)
		goto out_uevent_exit;

	if (!_major)
		_major = r;

	return 0;

out_uevent_exit:
	dm_uevent_exit();
out_free_rq_bio_info_cache:
	kmem_cache_destroy(_rq_bio_info_cache);
out_free_rq_tio_cache:
	kmem_cache_destroy(_rq_tio_cache);
out_free_tio_cache:
	kmem_cache_destroy(_tio_cache);
out_free_io_cache:
	kmem_cache_destroy(_io_cache);

	return r;
}

static void local_exit(void)
{
	kmem_cache_destroy(_rq_bio_info_cache);
	kmem_cache_destroy(_rq_tio_cache);
	kmem_cache_destroy(_tio_cache);
	kmem_cache_destroy(_io_cache);
	unregister_blkdev(_major, _name);
	dm_uevent_exit();

	_major = 0;

	DMINFO("cleaned up");
}

static int (*_inits[])(void) __initdata = {
	local_init,
	dm_target_init,
	dm_linear_init,
	dm_stripe_init,
	dm_kcopyd_init,
	dm_interface_init,
};

static void (*_exits[])(void) = {
	local_exit,
	dm_target_exit,
	dm_linear_exit,
	dm_stripe_exit,
	dm_kcopyd_exit,
	dm_interface_exit,
};

static int __init dm_init(void)
{
	const int count = ARRAY_SIZE(_inits);

	int r, i;

	for (i = 0; i < count; i++) {
		r = _inits[i]();
		if (r)
			goto bad;
	}

#ifdef MY_ABC_HERE
	funcSYNOLvLgSectorCount = SynoLvLgSectorCount;
#endif

	return 0;

      bad:
	while (i--)
		_exits[i]();

	return r;
}

static void __exit dm_exit(void)
{
	int i = ARRAY_SIZE(_exits);

	while (i--)
		_exits[i]();
}

static int dm_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mapped_device *md;

	spin_lock(&_minor_lock);

	md = bdev->bd_disk->private_data;
	if (!md)
		goto out;

	if (test_bit(DMF_FREEING, &md->flags) ||
	    test_bit(DMF_DELETING, &md->flags)) {
		md = NULL;
		goto out;
	}

	dm_get(md);
	atomic_inc(&md->open_count);

out:
	spin_unlock(&_minor_lock);

	return md ? 0 : -ENXIO;
}

static int dm_blk_close(struct gendisk *disk, fmode_t mode)
{
	struct mapped_device *md = disk->private_data;
	atomic_dec(&md->open_count);
	dm_put(md);
	return 0;
}

int dm_open_count(struct mapped_device *md)
{
	return atomic_read(&md->open_count);
}

int dm_lock_for_deletion(struct mapped_device *md)
{
	int r = 0;

	spin_lock(&_minor_lock);

	if (dm_open_count(md))
		r = -EBUSY;
	else
		set_bit(DMF_DELETING, &md->flags);

	spin_unlock(&_minor_lock);

	return r;
}

static int dm_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct mapped_device *md = bdev->bd_disk->private_data;

	return dm_get_geometry(md, geo);
}

static int dm_blk_ioctl(struct block_device *bdev, fmode_t mode,
			unsigned int cmd, unsigned long arg)
{
	struct mapped_device *md = bdev->bd_disk->private_data;
	struct dm_table *map = dm_get_table(md);
	struct dm_target *tgt;
	int r = -ENOTTY;

	if (!map || !dm_table_get_size(map))
		goto out;

	if (dm_table_get_num_targets(map) != 1)
		goto out;

	tgt = dm_table_get_target(map, 0);

	if (dm_suspended(md)) {
		r = -EAGAIN;
		goto out;
	}

	if (tgt->type->ioctl)
		r = tgt->type->ioctl(tgt, cmd, arg);

out:
	dm_table_put(map);

	return r;
}

static struct dm_io *alloc_io(struct mapped_device *md)
{
	return mempool_alloc(md->io_pool, GFP_NOIO);
}

static void free_io(struct mapped_device *md, struct dm_io *io)
{
	mempool_free(io, md->io_pool);
}

static void free_tio(struct mapped_device *md, struct dm_target_io *tio)
{
	mempool_free(tio, md->tio_pool);
}

static struct dm_rq_target_io *alloc_rq_tio(struct mapped_device *md)
{
	return mempool_alloc(md->tio_pool, GFP_ATOMIC);
}

static void free_rq_tio(struct dm_rq_target_io *tio)
{
	mempool_free(tio, tio->md->tio_pool);
}

static struct dm_rq_clone_bio_info *alloc_bio_info(struct mapped_device *md)
{
	return mempool_alloc(md->io_pool, GFP_ATOMIC);
}

static void free_bio_info(struct dm_rq_clone_bio_info *info)
{
	mempool_free(info, info->tio->md->io_pool);
}

static void start_io_acct(struct dm_io *io)
{
	struct mapped_device *md = io->md;
	int cpu;
	int rw = bio_data_dir(io->bio);

	io->start_time = jiffies;

	cpu = part_stat_lock();
	part_round_stats(cpu, &dm_disk(md)->part0);
	part_stat_unlock();
	dm_disk(md)->part0.in_flight[rw] = atomic_inc_return(&md->pending[rw]);
}

static void end_io_acct(struct dm_io *io)
{
	struct mapped_device *md = io->md;
	struct bio *bio = io->bio;
	unsigned long duration = jiffies - io->start_time;
	int pending, cpu;
	int rw = bio_data_dir(bio);

	cpu = part_stat_lock();
	part_round_stats(cpu, &dm_disk(md)->part0);
	part_stat_add(cpu, &dm_disk(md)->part0, ticks[rw], duration);
	part_stat_unlock();

	dm_disk(md)->part0.in_flight[rw] = pending =
		atomic_dec_return(&md->pending[rw]);
	pending += atomic_read(&md->pending[rw^0x1]);

	if (!pending)
		wake_up(&md->wait);
}

static void queue_io(struct mapped_device *md, struct bio *bio)
{
	down_write(&md->io_lock);

	spin_lock_irq(&md->deferred_lock);
	bio_list_add(&md->deferred, bio);
	spin_unlock_irq(&md->deferred_lock);

	if (!test_and_set_bit(DMF_QUEUE_IO_TO_THREAD, &md->flags))
		queue_work(md->wq, &md->work);

	up_write(&md->io_lock);
}

struct dm_table *dm_get_table(struct mapped_device *md)
{
	struct dm_table *t;
	unsigned long flags;

	read_lock_irqsave(&md->map_lock, flags);
	t = md->map;
	if (t)
		dm_table_get(t);
	read_unlock_irqrestore(&md->map_lock, flags);

	return t;
}

int dm_get_geometry(struct mapped_device *md, struct hd_geometry *geo)
{
	*geo = md->geometry;

	return 0;
}

int dm_set_geometry(struct mapped_device *md, struct hd_geometry *geo)
{
	sector_t sz = (sector_t)geo->cylinders * geo->heads * geo->sectors;

	if (geo->start > sz) {
		DMWARN("Start sector is beyond the geometry limits.");
		return -EINVAL;
	}

	md->geometry = *geo;

	return 0;
}

static int __noflush_suspending(struct mapped_device *md)
{
	return test_bit(DMF_NOFLUSH_SUSPENDING, &md->flags);
}

static void dec_pending(struct dm_io *io, int error)
{
	unsigned long flags;
	int io_error;
	struct bio *bio;
	struct mapped_device *md = io->md;

	if (unlikely(error)) {
		spin_lock_irqsave(&io->endio_lock, flags);
		if (!(io->error > 0 && __noflush_suspending(md)))
			io->error = error;
		spin_unlock_irqrestore(&io->endio_lock, flags);
	}

	if (atomic_dec_and_test(&io->io_count)) {
		if (io->error == DM_ENDIO_REQUEUE) {
			 
			spin_lock_irqsave(&md->deferred_lock, flags);
			if (__noflush_suspending(md)) {
				if (!bio_rw_flagged(io->bio, BIO_RW_BARRIER))
					bio_list_add_head(&md->deferred,
							  io->bio);
			} else
				 
				io->error = -EIO;
			spin_unlock_irqrestore(&md->deferred_lock, flags);
		}

		io_error = io->error;
		bio = io->bio;

		if (bio_rw_flagged(bio, BIO_RW_BARRIER)) {
			 
			if (!md->barrier_error && io_error != -EOPNOTSUPP)
				md->barrier_error = io_error;
			end_io_acct(io);
			free_io(md, io);
		} else {
			end_io_acct(io);
			free_io(md, io);

			if (io_error != DM_ENDIO_REQUEUE) {
				trace_block_bio_complete(md->queue, bio);

				bio_endio(bio, io_error);
			}
		}
	}
}

static void clone_endio(struct bio *bio, int error)
{
	int r = 0;
	struct dm_target_io *tio = bio->bi_private;
	struct dm_io *io = tio->io;
	struct mapped_device *md = tio->io->md;
	dm_endio_fn endio = tio->ti->type->end_io;

	if (!bio_flagged(bio, BIO_UPTODATE) && !error)
		error = -EIO;

	if (endio) {
		r = endio(tio->ti, bio, error, &tio->info);
		if (r < 0 || r == DM_ENDIO_REQUEUE)
			 
			error = r;
		else if (r == DM_ENDIO_INCOMPLETE)
			 
			return;
		else if (r) {
			DMWARN("unimplemented target endio return value: %d", r);
			BUG();
		}
	}

	bio->bi_private = md->bs;

	free_tio(md, tio);
	bio_put(bio);
	dec_pending(io, error);
}

static void end_clone_bio(struct bio *clone, int error)
{
	struct dm_rq_clone_bio_info *info = clone->bi_private;
	struct dm_rq_target_io *tio = info->tio;
	struct bio *bio = info->orig;
	unsigned int nr_bytes = info->orig->bi_size;

	bio_put(clone);

	if (tio->error)
		 
		return;
	else if (error) {
		 
		tio->error = error;
		return;
	}

	if (tio->orig->bio != bio)
		DMERR("bio completion is going in the middle of the request");

	blk_update_request(tio->orig, 0, nr_bytes);
}

static void rq_completed(struct mapped_device *md, int run_queue)
{
	int wakeup_waiters = 0;
	struct request_queue *q = md->queue;
	unsigned long flags;

	spin_lock_irqsave(q->queue_lock, flags);
	if (!queue_in_flight(q))
		wakeup_waiters = 1;
	spin_unlock_irqrestore(q->queue_lock, flags);

	if (wakeup_waiters)
		wake_up(&md->wait);

	if (run_queue)
		blk_run_queue(q);

	dm_put(md);
}

static void free_rq_clone(struct request *clone)
{
	struct dm_rq_target_io *tio = clone->end_io_data;

	blk_rq_unprep_clone(clone);
	free_rq_tio(tio);
}

static void dm_unprep_request(struct request *rq)
{
	struct request *clone = rq->special;

	rq->special = NULL;
	rq->cmd_flags &= ~REQ_DONTPREP;

	free_rq_clone(clone);
}

void dm_requeue_unmapped_request(struct request *clone)
{
	struct dm_rq_target_io *tio = clone->end_io_data;
	struct mapped_device *md = tio->md;
	struct request *rq = tio->orig;
	struct request_queue *q = rq->q;
	unsigned long flags;

	dm_unprep_request(rq);

	spin_lock_irqsave(q->queue_lock, flags);
	if (elv_queue_empty(q))
		blk_plug_device(q);
	blk_requeue_request(q, rq);
	spin_unlock_irqrestore(q->queue_lock, flags);

	rq_completed(md, 0);
}
EXPORT_SYMBOL_GPL(dm_requeue_unmapped_request);

static void __stop_queue(struct request_queue *q)
{
	blk_stop_queue(q);
}

static void stop_queue(struct request_queue *q)
{
	unsigned long flags;

	spin_lock_irqsave(q->queue_lock, flags);
	__stop_queue(q);
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static void __start_queue(struct request_queue *q)
{
	if (blk_queue_stopped(q))
		blk_start_queue(q);
}

static void start_queue(struct request_queue *q)
{
	unsigned long flags;

	spin_lock_irqsave(q->queue_lock, flags);
	__start_queue(q);
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static void dm_end_request(struct request *clone, int error)
{
	struct dm_rq_target_io *tio = clone->end_io_data;
	struct mapped_device *md = tio->md;
	struct request *rq = tio->orig;

	if (blk_pc_request(rq)) {
		rq->errors = clone->errors;
		rq->resid_len = clone->resid_len;

		if (rq->sense)
			 
			rq->sense_len = clone->sense_len;
	}

	free_rq_clone(clone);

	blk_end_request_all(rq, error);

	rq_completed(md, 1);
}

static void dm_softirq_done(struct request *rq)
{
	struct request *clone = rq->completion_data;
	struct dm_rq_target_io *tio = clone->end_io_data;
	dm_request_endio_fn rq_end_io = tio->ti->type->rq_end_io;
	int error = tio->error;

	if (!(rq->cmd_flags & REQ_FAILED) && rq_end_io)
		error = rq_end_io(tio->ti, clone, error, &tio->info);

	if (error <= 0)
		 
		dm_end_request(clone, error);
	else if (error == DM_ENDIO_INCOMPLETE)
		 
		return;
	else if (error == DM_ENDIO_REQUEUE)
		 
		dm_requeue_unmapped_request(clone);
	else {
		DMWARN("unimplemented target endio return value: %d", error);
		BUG();
	}
}

static void dm_complete_request(struct request *clone, int error)
{
	struct dm_rq_target_io *tio = clone->end_io_data;
	struct request *rq = tio->orig;

	tio->error = error;
	rq->completion_data = clone;
	blk_complete_request(rq);
}

void dm_kill_unmapped_request(struct request *clone, int error)
{
	struct dm_rq_target_io *tio = clone->end_io_data;
	struct request *rq = tio->orig;

	rq->cmd_flags |= REQ_FAILED;
	dm_complete_request(clone, error);
}
EXPORT_SYMBOL_GPL(dm_kill_unmapped_request);

static void end_clone_request(struct request *clone, int error)
{
	 
	__blk_put_request(clone->q, clone);

	dm_complete_request(clone, error);
}

static sector_t max_io_len(struct mapped_device *md,
			   sector_t sector, struct dm_target *ti)
{
	sector_t offset = sector - ti->begin;
	sector_t len = ti->len - offset;

	if (ti->split_io) {
		sector_t boundary;
		boundary = ((offset + ti->split_io) & ~(ti->split_io - 1))
			   - offset;
		if (len > boundary)
			len = boundary;
	}

	return len;
}

#ifdef MY_ABC_HERE
sector_t SynoLvLgSectorCount(void *private, sector_t sector)
{
	struct dm_target *ti = (struct dm_target *)private;

	if (ti && ti->type->lg_sector_get) {
		return ti->type->lg_sector_get(sector, ti);
	}

	return 0;
}
EXPORT_SYMBOL(SynoLvLgSectorCount);
#endif

static void __map_bio(struct dm_target *ti, struct bio *clone,
		      struct dm_target_io *tio)
{
	int r;
	sector_t sector;
	struct mapped_device *md;

	clone->bi_end_io = clone_endio;
	clone->bi_private = tio;

	atomic_inc(&tio->io->io_count);
	sector = clone->bi_sector;
	r = ti->type->map(ti, clone, &tio->info);
	if (r == DM_MAPIO_REMAPPED) {
		 
		trace_block_remap(bdev_get_queue(clone->bi_bdev), clone,
				    tio->io->bio->bi_bdev->bd_dev, sector);

		generic_make_request(clone);
	} else if (r < 0 || r == DM_MAPIO_REQUEUE) {
		 
		md = tio->io->md;
		dec_pending(tio->io, r);
		 
		clone->bi_private = md->bs;
		bio_put(clone);
		free_tio(md, tio);
	} else if (r) {
		DMWARN("unimplemented target map return value: %d", r);
		BUG();
	}
}

struct clone_info {
	struct mapped_device *md;
	struct dm_table *map;
	struct bio *bio;
	struct dm_io *io;
	sector_t sector;
	sector_t sector_count;
	unsigned short idx;
};

static void dm_bio_destructor(struct bio *bio)
{
	struct bio_set *bs = bio->bi_private;

	bio_free(bio, bs);
}

static struct bio *split_bvec(struct bio *bio, sector_t sector,
			      unsigned short idx, unsigned int offset,
			      unsigned int len, struct bio_set *bs)
{
	struct bio *clone;
	struct bio_vec *bv = bio->bi_io_vec + idx;

	clone = bio_alloc_bioset(GFP_NOIO, 1, bs);
	clone->bi_destructor = dm_bio_destructor;
	*clone->bi_io_vec = *bv;

	clone->bi_sector = sector;
	clone->bi_bdev = bio->bi_bdev;
	clone->bi_rw = bio->bi_rw & ~(1 << BIO_RW_BARRIER);
	clone->bi_vcnt = 1;
	clone->bi_size = to_bytes(len);
	clone->bi_io_vec->bv_offset = offset;
	clone->bi_io_vec->bv_len = clone->bi_size;
	clone->bi_flags |= 1 << BIO_CLONED;

	if (bio_integrity(bio)) {
		bio_integrity_clone(clone, bio, GFP_NOIO, bs);
		bio_integrity_trim(clone,
				   bio_sector_offset(bio, idx, offset), len);
	}

	return clone;
}

static struct bio *clone_bio(struct bio *bio, sector_t sector,
			     unsigned short idx, unsigned short bv_count,
			     unsigned int len, struct bio_set *bs)
{
	struct bio *clone;

	clone = bio_alloc_bioset(GFP_NOIO, bio->bi_max_vecs, bs);
	__bio_clone(clone, bio);
	clone->bi_rw &= ~(1 << BIO_RW_BARRIER);
	clone->bi_destructor = dm_bio_destructor;
	clone->bi_sector = sector;
	clone->bi_idx = idx;
	clone->bi_vcnt = idx + bv_count;
	clone->bi_size = to_bytes(len);
	clone->bi_flags &= ~(1 << BIO_SEG_VALID);

	if (bio_integrity(bio)) {
		bio_integrity_clone(clone, bio, GFP_NOIO, bs);

		if (idx != bio->bi_idx || clone->bi_size < bio->bi_size)
			bio_integrity_trim(clone,
					   bio_sector_offset(bio, idx, 0), len);
	}

	return clone;
}

static struct dm_target_io *alloc_tio(struct clone_info *ci,
				      struct dm_target *ti)
{
	struct dm_target_io *tio = mempool_alloc(ci->md->tio_pool, GFP_NOIO);

	tio->io = ci->io;
	tio->ti = ti;
	memset(&tio->info, 0, sizeof(tio->info));

	return tio;
}

static void __flush_target(struct clone_info *ci, struct dm_target *ti,
			  unsigned flush_nr)
{
	struct dm_target_io *tio = alloc_tio(ci, ti);
	struct bio *clone;

	tio->info.flush_request = flush_nr;

	clone = bio_alloc_bioset(GFP_NOIO, 0, ci->md->bs);
	__bio_clone(clone, ci->bio);
	clone->bi_destructor = dm_bio_destructor;

	__map_bio(ti, clone, tio);
}

static int __clone_and_map_empty_barrier(struct clone_info *ci)
{
	unsigned target_nr = 0, flush_nr;
	struct dm_target *ti;

	while ((ti = dm_table_get_target(ci->map, target_nr++)))
		for (flush_nr = 0; flush_nr < ti->num_flush_requests;
		     flush_nr++)
			__flush_target(ci, ti, flush_nr);

	ci->sector_count = 0;

	return 0;
}

static int __clone_and_map(struct clone_info *ci)
{
	struct bio *clone, *bio = ci->bio;
	struct dm_target *ti;
	sector_t len = 0, max;
	struct dm_target_io *tio;

	if (unlikely(bio_empty_barrier(bio)))
		return __clone_and_map_empty_barrier(ci);

	ti = dm_table_find_target(ci->map, ci->sector);
	if (!dm_target_is_valid(ti))
		return -EIO;

	max = max_io_len(ci->md, ci->sector, ti);

	tio = alloc_tio(ci, ti);

	if (ci->sector_count <= max) {
		 
		clone = clone_bio(bio, ci->sector, ci->idx,
				  bio->bi_vcnt - ci->idx, ci->sector_count,
				  ci->md->bs);
		__map_bio(ti, clone, tio);
		ci->sector_count = 0;

	} else if (to_sector(bio->bi_io_vec[ci->idx].bv_len) <= max) {
		 
		int i;
		sector_t remaining = max;
		sector_t bv_len;

		for (i = ci->idx; remaining && (i < bio->bi_vcnt); i++) {
			bv_len = to_sector(bio->bi_io_vec[i].bv_len);

			if (bv_len > remaining)
				break;

			remaining -= bv_len;
			len += bv_len;
		}

		clone = clone_bio(bio, ci->sector, ci->idx, i - ci->idx, len,
				  ci->md->bs);
		__map_bio(ti, clone, tio);

		ci->sector += len;
		ci->sector_count -= len;
		ci->idx = i;

	} else {
		 
		struct bio_vec *bv = bio->bi_io_vec + ci->idx;
		sector_t remaining = to_sector(bv->bv_len);
		unsigned int offset = 0;

		do {
			if (offset) {
				ti = dm_table_find_target(ci->map, ci->sector);
				if (!dm_target_is_valid(ti))
					return -EIO;

				max = max_io_len(ci->md, ci->sector, ti);

				tio = alloc_tio(ci, ti);
			}

			len = min(remaining, max);

			clone = split_bvec(bio, ci->sector, ci->idx,
					   bv->bv_offset + offset, len,
					   ci->md->bs);

			__map_bio(ti, clone, tio);

			ci->sector += len;
			ci->sector_count -= len;
			offset += to_bytes(len);
		} while (remaining -= len);

		ci->idx++;
	}

	return 0;
}

static void __split_and_process_bio(struct mapped_device *md, struct bio *bio)
{
	struct clone_info ci;
	int error = 0;

	ci.map = dm_get_table(md);
	if (unlikely(!ci.map)) {
		if (!bio_rw_flagged(bio, BIO_RW_BARRIER))
			bio_io_error(bio);
		else
			if (!md->barrier_error)
				md->barrier_error = -EIO;
		return;
	}

	ci.md = md;
	ci.bio = bio;
	ci.io = alloc_io(md);
	ci.io->error = 0;
	atomic_set(&ci.io->io_count, 1);
	ci.io->bio = bio;
	ci.io->md = md;
	spin_lock_init(&ci.io->endio_lock);
	ci.sector = bio->bi_sector;
	ci.sector_count = bio_sectors(bio);
	if (unlikely(bio_empty_barrier(bio)))
		ci.sector_count = 1;
	ci.idx = bio->bi_idx;

	start_io_acct(ci.io);
	while (ci.sector_count && !error)
		error = __clone_and_map(&ci);

	dec_pending(ci.io, error);
	dm_table_put(ci.map);
}
 
static int dm_merge_bvec(struct request_queue *q,
			 struct bvec_merge_data *bvm,
			 struct bio_vec *biovec)
{
	struct mapped_device *md = q->queuedata;
	struct dm_table *map = dm_get_table(md);
	struct dm_target *ti;
	sector_t max_sectors;
	int max_size = 0;

	if (unlikely(!map))
		goto out;

	ti = dm_table_find_target(map, bvm->bi_sector);
	if (!dm_target_is_valid(ti))
		goto out_table;

	max_sectors = min(max_io_len(md, bvm->bi_sector, ti),
			  (sector_t) BIO_MAX_SECTORS);
	max_size = (max_sectors << SECTOR_SHIFT) - bvm->bi_size;
	if (max_size < 0)
		max_size = 0;

	if (max_size && ti->type->merge)
		max_size = ti->type->merge(ti, bvm, biovec, max_size);
	 
	else if (queue_max_hw_sectors(q) <= PAGE_SIZE >> 9)

		max_size = 0;

out_table:
	dm_table_put(map);

out:
	 
	if (max_size <= biovec->bv_len && !(bvm->bi_size >> SECTOR_SHIFT))
		max_size = biovec->bv_len;

	return max_size;
}

static int _dm_request(struct request_queue *q, struct bio *bio)
{
	int rw = bio_data_dir(bio);
	struct mapped_device *md = q->queuedata;
	int cpu;

	down_read(&md->io_lock);

	cpu = part_stat_lock();
	part_stat_inc(cpu, &dm_disk(md)->part0, ios[rw]);
	part_stat_add(cpu, &dm_disk(md)->part0, sectors[rw], bio_sectors(bio));
	part_stat_unlock();

	if (unlikely(test_bit(DMF_QUEUE_IO_TO_THREAD, &md->flags)) ||
	    unlikely(bio_rw_flagged(bio, BIO_RW_BARRIER))) {
		up_read(&md->io_lock);

		if (unlikely(test_bit(DMF_BLOCK_IO_FOR_SUSPEND, &md->flags)) &&
		    bio_rw(bio) == READA) {
			bio_io_error(bio);
			return 0;
		}

		queue_io(md, bio);

		return 0;
	}

	__split_and_process_bio(md, bio);
	up_read(&md->io_lock);
	return 0;
}

static int dm_make_request(struct request_queue *q, struct bio *bio)
{
	struct mapped_device *md = q->queuedata;

	if (unlikely(bio_rw_flagged(bio, BIO_RW_BARRIER))) {
		bio_endio(bio, -EOPNOTSUPP);
		return 0;
	}

	return md->saved_make_request_fn(q, bio);  
}

static int dm_request_based(struct mapped_device *md)
{
	return blk_queue_stackable(md->queue);
}

static int dm_request(struct request_queue *q, struct bio *bio)
{
	struct mapped_device *md = q->queuedata;
#ifdef MY_ABC_HERE
	struct dm_dev_internal *dd = NULL;
	struct dm_table *map = NULL;
	char b[BDEVNAME_SIZE] = {'\0'};
	unsigned char blActive = 0;

	if (time_after(jiffies, md->ulLastReq + CHECKINTERVAL)) {
		spin_lock(&md->ActLock);
		blActive = md->blActive;
		md->blActive = 1;
		spin_unlock(&md->ActLock);

		map = dm_get_table(md);
		if (map && !blActive) {
			list_for_each_entry (dd, dm_table_get_devices(map), list) {

				if (dd && dd->dm_dev.bdev && NULL != strstr(bdevname(dd->dm_dev.bdev, b), "md")) {
					if (0 < SynoDebugFlag) {
						printk("dm request get [%s], push down wakeup no work\n",
								bdevname(dd->dm_dev.bdev, b));
					}
					if (dd->dm_dev.bdev->bd_disk && dd->dm_dev.bdev->bd_disk->private_data) {
						SynoMDWakeUpDevices(dd->dm_dev.bdev->bd_disk->private_data);
					}
				}
			}
		}
		dm_table_put(map);
	}

	md->ulLastReq = jiffies;
#endif

	if (dm_request_based(md))
		return dm_make_request(q, bio);

	return _dm_request(q, bio);
}

void dm_dispatch_request(struct request *rq)
{
	int r;

	if (blk_queue_io_stat(rq->q))
		rq->cmd_flags |= REQ_IO_STAT;

	rq->start_time = jiffies;
	r = blk_insert_cloned_request(rq->q, rq);
	if (r)
		dm_complete_request(rq, r);
}
EXPORT_SYMBOL_GPL(dm_dispatch_request);

static void dm_rq_bio_destructor(struct bio *bio)
{
	struct dm_rq_clone_bio_info *info = bio->bi_private;
	struct mapped_device *md = info->tio->md;

	free_bio_info(info);
	bio_free(bio, md->bs);
}

static int dm_rq_bio_constructor(struct bio *bio, struct bio *bio_orig,
				 void *data)
{
	struct dm_rq_target_io *tio = data;
	struct mapped_device *md = tio->md;
	struct dm_rq_clone_bio_info *info = alloc_bio_info(md);

	if (!info)
		return -ENOMEM;

	info->orig = bio_orig;
	info->tio = tio;
	bio->bi_end_io = end_clone_bio;
	bio->bi_private = info;
	bio->bi_destructor = dm_rq_bio_destructor;

	return 0;
}

static int setup_clone(struct request *clone, struct request *rq,
		       struct dm_rq_target_io *tio)
{
	int r = blk_rq_prep_clone(clone, rq, tio->md->bs, GFP_ATOMIC,
				  dm_rq_bio_constructor, tio);

	if (r)
		return r;

	clone->cmd = rq->cmd;
	clone->cmd_len = rq->cmd_len;
	clone->sense = rq->sense;
	clone->buffer = rq->buffer;
	clone->end_io = end_clone_request;
	clone->end_io_data = tio;

	return 0;
}

static int dm_rq_flush_suspending(struct mapped_device *md)
{
	return !md->suspend_rq.special;
}

static int dm_prep_fn(struct request_queue *q, struct request *rq)
{
	struct mapped_device *md = q->queuedata;
	struct dm_rq_target_io *tio;
	struct request *clone;

	if (unlikely(rq == &md->suspend_rq)) {
		if (dm_rq_flush_suspending(md))
			return BLKPREP_OK;
		else
			 
			return BLKPREP_KILL;
	}

	if (unlikely(rq->special)) {
		DMWARN("Already has something in rq->special.");
		return BLKPREP_KILL;
	}

	tio = alloc_rq_tio(md);  
	if (!tio)
		 
		return BLKPREP_DEFER;

	tio->md = md;
	tio->ti = NULL;
	tio->orig = rq;
	tio->error = 0;
	memset(&tio->info, 0, sizeof(tio->info));

	clone = &tio->clone;
	if (setup_clone(clone, rq, tio)) {
		 
		free_rq_tio(tio);
		return BLKPREP_DEFER;
	}

	rq->special = clone;
	rq->cmd_flags |= REQ_DONTPREP;

	return BLKPREP_OK;
}

static int map_request(struct dm_target *ti, struct request *rq,
		       struct mapped_device *md)
{
	int r, requeued = 0;
	struct request *clone = rq->special;
	struct dm_rq_target_io *tio = clone->end_io_data;

	dm_get(md);

	tio->ti = ti;
	r = ti->type->map_rq(ti, clone, &tio->info);
	switch (r) {
	case DM_MAPIO_SUBMITTED:
		 
		break;
	case DM_MAPIO_REMAPPED:
		 
		dm_dispatch_request(clone);
		break;
	case DM_MAPIO_REQUEUE:
		 
		dm_requeue_unmapped_request(clone);
		requeued = 1;
		break;
	default:
		if (r > 0) {
			DMWARN("unimplemented target map return value: %d", r);
			BUG();
		}

		dm_kill_unmapped_request(clone, r);
		break;
	}

	return requeued;
}

static void dm_request_fn(struct request_queue *q)
{
	struct mapped_device *md = q->queuedata;
	struct dm_table *map = dm_get_table(md);
	struct dm_target *ti;
	struct request *rq;

	while (!blk_queue_plugged(q) && !blk_queue_stopped(q)) {
		rq = blk_peek_request(q);
		if (!rq)
			goto plug_and_out;

		if (unlikely(rq == &md->suspend_rq)) {  
			if (queue_in_flight(q))
				 
				goto plug_and_out;

			__stop_queue(q);
			blk_start_request(rq);
			__blk_end_request_all(rq, 0);
			wake_up(&md->wait);
			goto out;
		}

		ti = dm_table_find_target(map, blk_rq_pos(rq));
		if (ti->type->busy && ti->type->busy(ti))
			goto plug_and_out;

		blk_start_request(rq);
		spin_unlock(q->queue_lock);
		if (map_request(ti, rq, md))
			goto requeued;

		spin_lock_irq(q->queue_lock);
	}

	goto out;

requeued:
	spin_lock_irq(q->queue_lock);

plug_and_out:
	if (!elv_queue_empty(q))
		 
		blk_plug_device(q);

out:
	dm_table_put(map);

	return;
}

int dm_underlying_device_busy(struct request_queue *q)
{
	return blk_lld_busy(q);
}
EXPORT_SYMBOL_GPL(dm_underlying_device_busy);

static int dm_lld_busy(struct request_queue *q)
{
	int r;
	struct mapped_device *md = q->queuedata;
	struct dm_table *map = dm_get_table(md);

	if (!map || test_bit(DMF_BLOCK_IO_FOR_SUSPEND, &md->flags))
		r = 1;
	else
		r = dm_table_any_busy_target(map);

	dm_table_put(map);

	return r;
}

static void dm_unplug_all(struct request_queue *q)
{
	struct mapped_device *md = q->queuedata;
	struct dm_table *map = dm_get_table(md);

	if (map) {
		if (dm_request_based(md))
			generic_unplug_device(q);

		dm_table_unplug_all(map);
		dm_table_put(map);
	}
}

static int dm_any_congested(void *congested_data, int bdi_bits)
{
	int r = bdi_bits;
	struct mapped_device *md = congested_data;
	struct dm_table *map;

	if (!test_bit(DMF_BLOCK_IO_FOR_SUSPEND, &md->flags)) {
		map = dm_get_table(md);
		if (map) {
			 
			if (dm_request_based(md))
				r = md->queue->backing_dev_info.state &
				    bdi_bits;
			else
				r = dm_table_any_congested(map, bdi_bits);

			dm_table_put(map);
		}
	}

	return r;
}

static DEFINE_IDR(_minor_idr);

static void free_minor(int minor)
{
	spin_lock(&_minor_lock);
	idr_remove(&_minor_idr, minor);
	spin_unlock(&_minor_lock);
}

static int specific_minor(int minor)
{
	int r, m;

	if (minor >= (1 << MINORBITS))
		return -EINVAL;

	r = idr_pre_get(&_minor_idr, GFP_KERNEL);
	if (!r)
		return -ENOMEM;

	spin_lock(&_minor_lock);

	if (idr_find(&_minor_idr, minor)) {
		r = -EBUSY;
		goto out;
	}

	r = idr_get_new_above(&_minor_idr, MINOR_ALLOCED, minor, &m);
	if (r)
		goto out;

	if (m != minor) {
		idr_remove(&_minor_idr, m);
		r = -EBUSY;
		goto out;
	}

out:
	spin_unlock(&_minor_lock);
	return r;
}

static int next_free_minor(int *minor)
{
	int r, m;

	r = idr_pre_get(&_minor_idr, GFP_KERNEL);
	if (!r)
		return -ENOMEM;

	spin_lock(&_minor_lock);

	r = idr_get_new(&_minor_idr, MINOR_ALLOCED, &m);
	if (r)
		goto out;

	if (m >= (1 << MINORBITS)) {
		idr_remove(&_minor_idr, m);
		r = -ENOSPC;
		goto out;
	}

	*minor = m;

out:
	spin_unlock(&_minor_lock);
	return r;
}

static const struct block_device_operations dm_blk_dops;

static void dm_wq_work(struct work_struct *work);

static struct mapped_device *alloc_dev(int minor)
{
	int r;
	struct mapped_device *md = kzalloc(sizeof(*md), GFP_KERNEL);
	void *old_md;

	if (!md) {
		DMWARN("unable to allocate device, out of memory.");
		return NULL;
	}

	if (!try_module_get(THIS_MODULE))
		goto bad_module_get;

	if (minor == DM_ANY_MINOR)
		r = next_free_minor(&minor);
	else
		r = specific_minor(minor);
	if (r < 0)
		goto bad_minor;

	init_rwsem(&md->io_lock);
	mutex_init(&md->suspend_lock);
	spin_lock_init(&md->deferred_lock);
	rwlock_init(&md->map_lock);
	atomic_set(&md->holders, 1);
	atomic_set(&md->open_count, 0);
	atomic_set(&md->event_nr, 0);
	atomic_set(&md->uevent_seq, 0);
	INIT_LIST_HEAD(&md->uevent_list);
	spin_lock_init(&md->uevent_lock);

	md->queue = blk_init_queue(dm_request_fn, NULL);
	if (!md->queue)
		goto bad_queue;

	queue_flag_clear_unlocked(QUEUE_FLAG_STACKABLE, md->queue);
	md->saved_make_request_fn = md->queue->make_request_fn;
	md->queue->queuedata = md;
	md->queue->backing_dev_info.congested_fn = dm_any_congested;
	md->queue->backing_dev_info.congested_data = md;
	blk_queue_make_request(md->queue, dm_request);
	blk_queue_bounce_limit(md->queue, BLK_BOUNCE_ANY);
	md->queue->unplug_fn = dm_unplug_all;
	blk_queue_merge_bvec(md->queue, dm_merge_bvec);
	blk_queue_softirq_done(md->queue, dm_softirq_done);
	blk_queue_prep_rq(md->queue, dm_prep_fn);
	blk_queue_lld_busy(md->queue, dm_lld_busy);

	md->disk = alloc_disk(1);
	if (!md->disk)
		goto bad_disk;

	atomic_set(&md->pending[0], 0);
	atomic_set(&md->pending[1], 0);
	init_waitqueue_head(&md->wait);
	INIT_WORK(&md->work, dm_wq_work);
	init_waitqueue_head(&md->eventq);

	md->disk->major = _major;
	md->disk->first_minor = minor;
	md->disk->fops = &dm_blk_dops;
	md->disk->queue = md->queue;
	md->disk->private_data = md;
	sprintf(md->disk->disk_name, "dm-%d", minor);
	add_disk(md->disk);
	format_dev_t(md->name, MKDEV(_major, minor));

	md->wq = create_singlethread_workqueue("kdmflush");
	if (!md->wq)
		goto bad_thread;

	md->bdev = bdget_disk(md->disk, 0);
	if (!md->bdev)
		goto bad_bdev;

	spin_lock(&_minor_lock);
	old_md = idr_replace(&_minor_idr, md, minor);
	spin_unlock(&_minor_lock);

	BUG_ON(old_md != MINOR_ALLOCED);
#ifdef MY_ABC_HERE
	spin_lock_init(&md->ActLock);
	md->blActive = 0;
	md->ulLastReq = jiffies;
#endif

	return md;

bad_bdev:
	destroy_workqueue(md->wq);
bad_thread:
	del_gendisk(md->disk);
	put_disk(md->disk);
bad_disk:
	blk_cleanup_queue(md->queue);
bad_queue:
	free_minor(minor);
bad_minor:
	module_put(THIS_MODULE);
bad_module_get:
	kfree(md);
	return NULL;
}

static void unlock_fs(struct mapped_device *md);

static void free_dev(struct mapped_device *md)
{
	int minor = MINOR(disk_devt(md->disk));

	unlock_fs(md);
	bdput(md->bdev);
	destroy_workqueue(md->wq);
	if (md->tio_pool)
		mempool_destroy(md->tio_pool);
	if (md->io_pool)
		mempool_destroy(md->io_pool);
	if (md->bs)
		bioset_free(md->bs);
	blk_integrity_unregister(md->disk);
	del_gendisk(md->disk);
	free_minor(minor);

	spin_lock(&_minor_lock);
	md->disk->private_data = NULL;
	spin_unlock(&_minor_lock);

	put_disk(md->disk);
	blk_cleanup_queue(md->queue);
	module_put(THIS_MODULE);
	kfree(md);
}

static void __bind_mempools(struct mapped_device *md, struct dm_table *t)
{
	struct dm_md_mempools *p;

	if (md->io_pool && md->tio_pool && md->bs)
		 
		goto out;

	p = dm_table_get_md_mempools(t);
	BUG_ON(!p || md->io_pool || md->tio_pool || md->bs);

	md->io_pool = p->io_pool;
	p->io_pool = NULL;
	md->tio_pool = p->tio_pool;
	p->tio_pool = NULL;
	md->bs = p->bs;
	p->bs = NULL;

out:
	 
	dm_table_free_md_mempools(t);
}

static void event_callback(void *context)
{
	unsigned long flags;
	LIST_HEAD(uevents);
	struct mapped_device *md = (struct mapped_device *) context;

	spin_lock_irqsave(&md->uevent_lock, flags);
	list_splice_init(&md->uevent_list, &uevents);
	spin_unlock_irqrestore(&md->uevent_lock, flags);

	dm_send_uevents(&uevents, &disk_to_dev(md->disk)->kobj);

	atomic_inc(&md->event_nr);
	wake_up(&md->eventq);
}

static void __set_size(struct mapped_device *md, sector_t size)
{
	set_capacity(md->disk, size);

	mutex_lock(&md->bdev->bd_inode->i_mutex);
	i_size_write(md->bdev->bd_inode, (loff_t)size << SECTOR_SHIFT);
	mutex_unlock(&md->bdev->bd_inode->i_mutex);
}

static int __bind(struct mapped_device *md, struct dm_table *t,
		  struct queue_limits *limits)
{
	struct request_queue *q = md->queue;
	sector_t size;
	unsigned long flags;

	size = dm_table_get_size(t);

	if (size != get_capacity(md->disk))
		memset(&md->geometry, 0, sizeof(md->geometry));

	__set_size(md, size);

	if (!size) {
		dm_table_destroy(t);
		return 0;
	}

	dm_table_event_callback(t, event_callback, md);

	if (dm_table_request_based(t) && !blk_queue_stopped(q))
		stop_queue(q);

	__bind_mempools(md, t);

	write_lock_irqsave(&md->map_lock, flags);
	md->map = t;
	dm_table_set_restrictions(t, q, limits);
	write_unlock_irqrestore(&md->map_lock, flags);

	return 0;
}

static void __unbind(struct mapped_device *md)
{
	struct dm_table *map = md->map;
	unsigned long flags;

	if (!map)
		return;

	dm_table_event_callback(map, NULL, NULL);
	write_lock_irqsave(&md->map_lock, flags);
	md->map = NULL;
	write_unlock_irqrestore(&md->map_lock, flags);
	dm_table_destroy(map);
}

int dm_create(int minor, struct mapped_device **result)
{
	struct mapped_device *md;

	md = alloc_dev(minor);
	if (!md)
		return -ENXIO;

	dm_sysfs_init(md);

	*result = md;
	return 0;
}

static struct mapped_device *dm_find_md(dev_t dev)
{
	struct mapped_device *md;
	unsigned minor = MINOR(dev);

	if (MAJOR(dev) != _major || minor >= (1 << MINORBITS))
		return NULL;

	spin_lock(&_minor_lock);

	md = idr_find(&_minor_idr, minor);
	if (md && (md == MINOR_ALLOCED ||
		   (MINOR(disk_devt(dm_disk(md))) != minor) ||
		   test_bit(DMF_FREEING, &md->flags))) {
		md = NULL;
		goto out;
	}

out:
	spin_unlock(&_minor_lock);

	return md;
}

struct mapped_device *dm_get_md(dev_t dev)
{
	struct mapped_device *md = dm_find_md(dev);

	if (md)
		dm_get(md);

	return md;
}

void *dm_get_mdptr(struct mapped_device *md)
{
	return md->interface_ptr;
}

void dm_set_mdptr(struct mapped_device *md, void *ptr)
{
	md->interface_ptr = ptr;
}

void dm_get(struct mapped_device *md)
{
	atomic_inc(&md->holders);
}

const char *dm_device_name(struct mapped_device *md)
{
	return md->name;
}
EXPORT_SYMBOL_GPL(dm_device_name);

void dm_put(struct mapped_device *md)
{
	struct dm_table *map;

	BUG_ON(test_bit(DMF_FREEING, &md->flags));

	if (atomic_dec_and_lock(&md->holders, &_minor_lock)) {
		map = dm_get_table(md);
		idr_replace(&_minor_idr, MINOR_ALLOCED,
			    MINOR(disk_devt(dm_disk(md))));
		set_bit(DMF_FREEING, &md->flags);
		spin_unlock(&_minor_lock);
		if (!dm_suspended(md)) {
			dm_table_presuspend_targets(map);
			dm_table_postsuspend_targets(map);
		}
		dm_sysfs_exit(md);
		dm_table_put(map);
		__unbind(md);
		free_dev(md);
	}
}
EXPORT_SYMBOL_GPL(dm_put);

static int dm_wait_for_completion(struct mapped_device *md, int interruptible)
{
	int r = 0;
	DECLARE_WAITQUEUE(wait, current);
	struct request_queue *q = md->queue;
	unsigned long flags;

	dm_unplug_all(md->queue);

	add_wait_queue(&md->wait, &wait);

	while (1) {
		set_current_state(interruptible);

		smp_mb();
		if (dm_request_based(md)) {
			spin_lock_irqsave(q->queue_lock, flags);
			if (!queue_in_flight(q) && blk_queue_stopped(q)) {
				spin_unlock_irqrestore(q->queue_lock, flags);
				break;
			}
			spin_unlock_irqrestore(q->queue_lock, flags);
		} else if (!atomic_read(&md->pending[0]) &&
					!atomic_read(&md->pending[1]))
			break;

		if (interruptible == TASK_INTERRUPTIBLE &&
		    signal_pending(current)) {
			r = -EINTR;
			break;
		}

		io_schedule();
	}
	set_current_state(TASK_RUNNING);

	remove_wait_queue(&md->wait, &wait);

	return r;
}

static void dm_flush(struct mapped_device *md)
{
	dm_wait_for_completion(md, TASK_UNINTERRUPTIBLE);

	bio_init(&md->barrier_bio);
	md->barrier_bio.bi_bdev = md->bdev;
	md->barrier_bio.bi_rw = WRITE_BARRIER;
	__split_and_process_bio(md, &md->barrier_bio);

	dm_wait_for_completion(md, TASK_UNINTERRUPTIBLE);
}

static void process_barrier(struct mapped_device *md, struct bio *bio)
{
	md->barrier_error = 0;

	dm_flush(md);

	if (!bio_empty_barrier(bio)) {
		__split_and_process_bio(md, bio);
		dm_flush(md);
	}

	if (md->barrier_error != DM_ENDIO_REQUEUE)
		bio_endio(bio, md->barrier_error);
	else {
		spin_lock_irq(&md->deferred_lock);
		bio_list_add_head(&md->deferred, bio);
		spin_unlock_irq(&md->deferred_lock);
	}
}

static void dm_wq_work(struct work_struct *work)
{
	struct mapped_device *md = container_of(work, struct mapped_device,
						work);
	struct bio *c;

	down_write(&md->io_lock);

	while (!test_bit(DMF_BLOCK_IO_FOR_SUSPEND, &md->flags)) {
		spin_lock_irq(&md->deferred_lock);
		c = bio_list_pop(&md->deferred);
		spin_unlock_irq(&md->deferred_lock);

		if (!c) {
			clear_bit(DMF_QUEUE_IO_TO_THREAD, &md->flags);
			break;
		}

		up_write(&md->io_lock);

		if (dm_request_based(md))
			generic_make_request(c);
		else {
			if (bio_rw_flagged(c, BIO_RW_BARRIER))
				process_barrier(md, c);
			else
				__split_and_process_bio(md, c);
		}

		down_write(&md->io_lock);
	}

	up_write(&md->io_lock);
}

static void dm_queue_flush(struct mapped_device *md)
{
	clear_bit(DMF_BLOCK_IO_FOR_SUSPEND, &md->flags);
	smp_mb__after_clear_bit();
	queue_work(md->wq, &md->work);
}

int dm_swap_table(struct mapped_device *md, struct dm_table *table)
{
	struct queue_limits limits;
	int r = -EINVAL;

	mutex_lock(&md->suspend_lock);

	if (!dm_suspended(md))
		goto out;

	r = dm_calculate_queue_limits(table, &limits);
	if (r)
		goto out;

	if (md->map &&
	    (dm_table_get_type(md->map) != dm_table_get_type(table))) {
		DMWARN("can't change the device type after a table is bound");
		goto out;
	}

	__unbind(md);
	r = __bind(md, table, &limits);

out:
	mutex_unlock(&md->suspend_lock);
	return r;
}

static void dm_rq_invalidate_suspend_marker(struct mapped_device *md)
{
	md->suspend_rq.special = (void *)0x1;
}

static void dm_rq_abort_suspend(struct mapped_device *md, int noflush)
{
	struct request_queue *q = md->queue;
	unsigned long flags;

	spin_lock_irqsave(q->queue_lock, flags);
	if (!noflush)
		dm_rq_invalidate_suspend_marker(md);
	__start_queue(q);
	spin_unlock_irqrestore(q->queue_lock, flags);
}

static void dm_rq_start_suspend(struct mapped_device *md, int noflush)
{
	struct request *rq = &md->suspend_rq;
	struct request_queue *q = md->queue;

	if (noflush)
		stop_queue(q);
	else {
		blk_rq_init(q, rq);
		blk_insert_request(q, rq, 0, NULL);
	}
}

static int dm_rq_suspend_available(struct mapped_device *md, int noflush)
{
	int r = 1;
	struct request *rq = &md->suspend_rq;
	struct request_queue *q = md->queue;
	unsigned long flags;

	if (noflush)
		return r;

	spin_lock_irqsave(q->queue_lock, flags);
	if (unlikely(rq->ref_count)) {
		 
		BUG_ON(!rq->special);  
		DMWARN("Invalidating the previous flush suspend is still in"
		       " progress.  Please retry later.");
		r = 0;
	}
	spin_unlock_irqrestore(q->queue_lock, flags);

	return r;
}

static int lock_fs(struct mapped_device *md)
{
	int r;

	WARN_ON(md->frozen_sb);

	md->frozen_sb = freeze_bdev(md->bdev);
	if (IS_ERR(md->frozen_sb)) {
		r = PTR_ERR(md->frozen_sb);
		md->frozen_sb = NULL;
		return r;
	}

	set_bit(DMF_FROZEN, &md->flags);

	return 0;
}

static void unlock_fs(struct mapped_device *md)
{
	if (!test_bit(DMF_FROZEN, &md->flags))
		return;

	thaw_bdev(md->bdev, md->frozen_sb);
	md->frozen_sb = NULL;
	clear_bit(DMF_FROZEN, &md->flags);
}

int dm_suspend(struct mapped_device *md, unsigned suspend_flags)
{
	struct dm_table *map = NULL;
	int r = 0;
	int do_lockfs = suspend_flags & DM_SUSPEND_LOCKFS_FLAG ? 1 : 0;
	int noflush = suspend_flags & DM_SUSPEND_NOFLUSH_FLAG ? 1 : 0;

	mutex_lock(&md->suspend_lock);

	if (dm_suspended(md)) {
		r = -EINVAL;
		goto out_unlock;
	}

	if (dm_request_based(md) && !dm_rq_suspend_available(md, noflush)) {
		r = -EBUSY;
		goto out_unlock;
	}

	map = dm_get_table(md);

	if (noflush)
		set_bit(DMF_NOFLUSH_SUSPENDING, &md->flags);

	dm_table_presuspend_targets(map);

	if (!noflush && do_lockfs) {
		r = lock_fs(md);
		if (r)
			goto out;
	}

	down_write(&md->io_lock);
	set_bit(DMF_BLOCK_IO_FOR_SUSPEND, &md->flags);
	set_bit(DMF_QUEUE_IO_TO_THREAD, &md->flags);
	up_write(&md->io_lock);

	flush_workqueue(md->wq);

	if (dm_request_based(md))
		dm_rq_start_suspend(md, noflush);

	r = dm_wait_for_completion(md, TASK_INTERRUPTIBLE);

	down_write(&md->io_lock);
	if (noflush)
		clear_bit(DMF_NOFLUSH_SUSPENDING, &md->flags);
	up_write(&md->io_lock);

	if (r < 0) {
		dm_queue_flush(md);

		if (dm_request_based(md))
			dm_rq_abort_suspend(md, noflush);

		unlock_fs(md);
		goto out;  
	}

	dm_table_postsuspend_targets(map);

	set_bit(DMF_SUSPENDED, &md->flags);

out:
	dm_table_put(map);

out_unlock:
	mutex_unlock(&md->suspend_lock);
	return r;
}

int dm_resume(struct mapped_device *md)
{
	int r = -EINVAL;
	struct dm_table *map = NULL;

	mutex_lock(&md->suspend_lock);
	if (!dm_suspended(md))
		goto out;

	map = dm_get_table(md);
	if (!map || !dm_table_get_size(map))
		goto out;

	r = dm_table_resume_targets(map);
	if (r)
		goto out;

	dm_queue_flush(md);

	if (dm_request_based(md))
		start_queue(md->queue);

	unlock_fs(md);

	clear_bit(DMF_SUSPENDED, &md->flags);

	dm_table_unplug_all(map);
	r = 0;
out:
	dm_table_put(map);
	mutex_unlock(&md->suspend_lock);

	return r;
}

void dm_kobject_uevent(struct mapped_device *md, enum kobject_action action,
		       unsigned cookie)
{
	char udev_cookie[DM_COOKIE_LENGTH];
	char *envp[] = { udev_cookie, NULL };

	if (!cookie)
		kobject_uevent(&disk_to_dev(md->disk)->kobj, action);
	else {
		snprintf(udev_cookie, DM_COOKIE_LENGTH, "%s=%u",
			 DM_COOKIE_ENV_VAR_NAME, cookie);
		kobject_uevent_env(&disk_to_dev(md->disk)->kobj, action, envp);
	}
}

uint32_t dm_next_uevent_seq(struct mapped_device *md)
{
	return atomic_add_return(1, &md->uevent_seq);
}

uint32_t dm_get_event_nr(struct mapped_device *md)
{
	return atomic_read(&md->event_nr);
}

int dm_wait_event(struct mapped_device *md, int event_nr)
{
	return wait_event_interruptible(md->eventq,
			(event_nr != atomic_read(&md->event_nr)));
}

void dm_uevent_add(struct mapped_device *md, struct list_head *elist)
{
	unsigned long flags;

	spin_lock_irqsave(&md->uevent_lock, flags);
	list_add(elist, &md->uevent_list);
	spin_unlock_irqrestore(&md->uevent_lock, flags);
}

struct gendisk *dm_disk(struct mapped_device *md)
{
	return md->disk;
}

struct kobject *dm_kobject(struct mapped_device *md)
{
	return &md->kobj;
}

struct mapped_device *dm_get_from_kobject(struct kobject *kobj)
{
	struct mapped_device *md;

	md = container_of(kobj, struct mapped_device, kobj);
	if (&md->kobj != kobj)
		return NULL;

	if (test_bit(DMF_FREEING, &md->flags) ||
	    test_bit(DMF_DELETING, &md->flags))
		return NULL;

	dm_get(md);
	return md;
}

int dm_suspended(struct mapped_device *md)
{
	return test_bit(DMF_SUSPENDED, &md->flags);
}

#ifdef MY_ABC_HERE
int dm_active_get(struct mapped_device *md)
{
	unsigned char blActive = 0;

	spin_lock(&md->ActLock);
	blActive = md->blActive;
	spin_unlock(&md->ActLock);

	return blActive;
}

int dm_active_set(struct mapped_device *md, int value)
{
	struct dm_table *map = NULL;
	struct dm_dev_internal *dd = NULL;
	char b[BDEVNAME_SIZE] = {'\0'};
	int iNeedWake = 0;

	spin_lock(&md->ActLock);
	if (!(md->blActive) && value) {
		iNeedWake = 1;
	}
	md->blActive = value;
	spin_unlock(&md->ActLock);

	map = dm_get_table(md);
	if (map) {
		list_for_each_entry (dd, dm_table_get_devices(map), list) {
			if (dd && dd->dm_dev.bdev && NULL != strstr(bdevname(dd->dm_dev.bdev, b), "md")) {
				if (0 < SynoDebugFlag) {
					printk("dm active set [%s], value %d iNeedWake %d\n",
							bdevname(dd->dm_dev.bdev, b), value, iNeedWake);
				}
				if (dd->dm_dev.bdev->bd_disk && dd->dm_dev.bdev->bd_disk->private_data) {
					mddev_t *mddev = dd->dm_dev.bdev->bd_disk->private_data;
					if (iNeedWake) {
						SynoMDWakeUpDevices(mddev);
					}
					spin_lock(&mddev->ActLock);
					mddev->blActive = value;
					spin_unlock(&mddev->ActLock);
				}
			}
		}
	}
	dm_table_put(map);

	return 0;
}
#endif

int dm_noflush_suspending(struct dm_target *ti)
{
	struct mapped_device *md = dm_table_get_md(ti->table);
	int r = __noflush_suspending(md);

	dm_put(md);

	return r;
}
EXPORT_SYMBOL_GPL(dm_noflush_suspending);

struct dm_md_mempools *dm_alloc_md_mempools(unsigned type)
{
	struct dm_md_mempools *pools = kmalloc(sizeof(*pools), GFP_KERNEL);

	if (!pools)
		return NULL;

	pools->io_pool = (type == DM_TYPE_BIO_BASED) ?
			 mempool_create_slab_pool(MIN_IOS, _io_cache) :
			 mempool_create_slab_pool(MIN_IOS, _rq_bio_info_cache);
	if (!pools->io_pool)
		goto free_pools_and_out;

	pools->tio_pool = (type == DM_TYPE_BIO_BASED) ?
			  mempool_create_slab_pool(MIN_IOS, _tio_cache) :
			  mempool_create_slab_pool(MIN_IOS, _rq_tio_cache);
	if (!pools->tio_pool)
		goto free_io_pool_and_out;

	pools->bs = (type == DM_TYPE_BIO_BASED) ?
		    bioset_create(16, 0) : bioset_create(MIN_IOS, 0);
	if (!pools->bs)
		goto free_tio_pool_and_out;

	return pools;

free_tio_pool_and_out:
	mempool_destroy(pools->tio_pool);

free_io_pool_and_out:
	mempool_destroy(pools->io_pool);

free_pools_and_out:
	kfree(pools);

	return NULL;
}

void dm_free_md_mempools(struct dm_md_mempools *pools)
{
	if (!pools)
		return;

	if (pools->io_pool)
		mempool_destroy(pools->io_pool);

	if (pools->tio_pool)
		mempool_destroy(pools->tio_pool);

	if (pools->bs)
		bioset_free(pools->bs);

	kfree(pools);
}

static const struct block_device_operations dm_blk_dops = {
	.open = dm_blk_open,
	.release = dm_blk_close,
	.ioctl = dm_blk_ioctl,
	.getgeo = dm_blk_getgeo,
	.owner = THIS_MODULE
};

EXPORT_SYMBOL(dm_get_mapinfo);

module_init(dm_init);
module_exit(dm_exit);

module_param(major, uint, 0);
MODULE_PARM_DESC(major, "The major number of the device mapper");
MODULE_DESCRIPTION(DM_NAME " driver");
MODULE_AUTHOR("Joe Thornber <dm-devel@redhat.com>");
MODULE_LICENSE("GPL");
