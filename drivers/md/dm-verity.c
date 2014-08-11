/*
 * Originally based on dm-crypt.c,
 * Copyright (C) 2003 Christophe Saout <christophe saout de>
 * Copyright (C) 2004 Clemens Fruhwirth <clemens endorphin org>
 * Copyright (C) 2006-2008 Red Hat, Inc. All rights reserved.
 * Copyright (C) 2011 The Chromium OS Authors <chromium-os-dev chromium org>
 *                    All Rights Reserved.
 *
 * This file is released under the GPLv2.
 *
 * Implements a verifying transparent block device.
 * See Documentation/device-mapper/dm-verity.txt
 */
#include <linux/async.h>
#include <linux/atomic.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/genhd.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mempool.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/device-mapper.h>
#include <linux/dm-bht.h>

#include "dm-verity.h"

#define DM_MSG_PREFIX "verity"

/* Supports up to 512-bit digests */
#define VERITY_MAX_DIGEST_SIZE 64

/* TODO(wad) make both of these report the error line/file to a
 *           verity_bug function.
 */
#define VERITY_BUG(msg...) BUG()
#define VERITY_BUG_ON(cond, msg...) BUG_ON(cond)

/* Helper for printing sector_t */
#define ULL(x) ((unsigned long long)(x))

#define MIN_IOS 32
#define MIN_BIOS (MIN_IOS * 2)
#define VERITY_DEFAULT_BLOCK_SIZE 4096

/* Provide a lightweight means of specifying the global default for
 * error behavior: eio, reboot, or none
 * Legacy support for 0 = eio, 1 = reboot/panic, 2 = none, 3 = notify.
 * This is matched to the enum in dm-verity.h.
 */
static const char * const allowed_error_behaviors[] = { "eio", "panic", "none",
							"notify", NULL };
static char *error_behavior = "eio";
module_param(error_behavior, charp, 0644);
MODULE_PARM_DESC(error_behavior, "Behavior on error "
				 "(eio, panic, none, notify)");

/* Controls whether verity_get_device will wait forever for a device. */
static int dev_wait;
module_param(dev_wait, bool, 0444);
MODULE_PARM_DESC(dev_wait, "Wait forever for a backing device");

/* per-requested-bio private data */
enum verity_io_flags {
	VERITY_IOFLAGS_CLONED = 0x1,	/* original bio has been cloned */
};

struct dm_verity_io {
	struct dm_target *target;
	struct bio *bio;
	struct delayed_work work;
	unsigned int flags;

	int error;
	atomic_t pending;

	u64 block;  /* aligned block index */
	u64 count;  /* aligned count in blocks */
};

struct verity_config {
	struct dm_dev *dev;
	sector_t start;
	sector_t size;

	struct dm_dev *hash_dev;
	sector_t hash_start;

	struct dm_bht bht;

	/* Pool required for io contexts */
	mempool_t *io_pool;
	/* Pool and bios required for making sure that backing device reads are
	 * in PAGE_SIZE increments.
	 */
	struct bio_set *bs;

	char hash_alg[CRYPTO_MAX_ALG_NAME];

	int error_behavior;
};

static struct kmem_cache *_verity_io_pool;
static struct workqueue_struct *kveritydq, *kverityd_ioq;

static void kverityd_verify(struct work_struct *work);
static void kverityd_io(struct work_struct *work);
static void kverityd_io_bht_populate(struct dm_verity_io *io);
static void kverityd_io_bht_populate_end(struct bio *, int error);

static BLOCKING_NOTIFIER_HEAD(verity_error_notifier);

/*
 * Exported interfaces
 */

int dm_verity_register_error_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&verity_error_notifier, nb);
}
EXPORT_SYMBOL_GPL(dm_verity_register_error_notifier);

int dm_verity_unregister_error_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&verity_error_notifier, nb);
}
EXPORT_SYMBOL_GPL(dm_verity_unregister_error_notifier);

/*
 * Allocation and utility functions
 */

static void kverityd_src_io_read_end(struct bio *clone, int error);

/* Shared destructor for all internal bios */
static void dm_verity_bio_destructor(struct bio *bio)
{
	struct dm_verity_io *io = bio->bi_private;
	struct verity_config *vc = io->target->private;
	bio_free(bio, vc->bs);
}

static struct bio *verity_alloc_bioset(struct verity_config *vc, gfp_t gfp_mask,
				       int nr_iovecs)
{
	return bio_alloc_bioset(gfp_mask, nr_iovecs, vc->bs);
}

static struct dm_verity_io *verity_io_alloc(struct dm_target *ti,
					    struct bio *bio)
{
	struct verity_config *vc = ti->private;
	sector_t sector = bio->bi_sector - ti->begin;
	struct dm_verity_io *io;

	io = mempool_alloc(vc->io_pool, GFP_NOIO);
	if (unlikely(!io))
		return NULL;
	io->flags = 0;
	io->target = ti;
	io->bio = bio;
	io->error = 0;

	/* Adjust the sector by the virtual starting sector */
	io->block = to_bytes(sector) / vc->bht.block_size;
	io->count = bio->bi_size / vc->bht.block_size;

	atomic_set(&io->pending, 0);

	return io;
}

static struct bio *verity_bio_clone(struct dm_verity_io *io)
{
	struct verity_config *vc = io->target->private;
	struct bio *bio = io->bio;
	struct bio *clone = verity_alloc_bioset(vc, GFP_NOIO, bio->bi_max_vecs);

	if (!clone)
		return NULL;

	__bio_clone(clone, bio);
	clone->bi_private = io;
	clone->bi_end_io  = kverityd_src_io_read_end;
	clone->bi_bdev    = vc->dev->bdev;
	clone->bi_sector += vc->start - io->target->begin;
	clone->bi_destructor = dm_verity_bio_destructor;

	return clone;
}

/* If the request is not successful, this handler takes action.
 * TODO make this call a registered handler.
 */
static void verity_error(struct verity_config *vc, struct dm_verity_io *io,
			 int error)
{
	const char *message;
	int error_mode = DM_VERITY_ERROR_BEHAVIOR_PANIC;
	dev_t devt = 0;
	u64 block = ~0;
	int transient = 1;
	struct dm_verity_error_state error_state;

	if (vc) {
		devt = vc->dev->bdev->bd_dev;
		error_mode = vc->error_behavior;
	}

	if (io) {
		io->error = -EIO;
		block = io->block;
	}

	switch (error) {
	case -ENOMEM:
		message = "out of memory";
		break;
	case -EBUSY:
		message = "pending data seen during verify";
		break;
	case -EFAULT:
		message = "crypto operation failure";
		break;
	case -EACCES:
		message = "integrity failure";
		/* Image is bad. */
		transient = 0;
		break;
	case -EPERM:
		message = "hash tree population failure";
		/* Should be dm-bht specific errors */
		transient = 0;
		break;
	case -EINVAL:
		message = "unexpected missing/invalid data";
		/* The device was configured incorrectly - fallback. */
		transient = 0;
		break;
	default:
		/* Other errors can be passed through as IO errors */
		message = "unknown or I/O error";
		return;
	}

	DMERR_LIMIT("verification failure occurred: %s", message);

	if (error_mode == DM_VERITY_ERROR_BEHAVIOR_NOTIFY) {
		error_state.code = error;
		error_state.transient = transient;
		error_state.block = block;
		error_state.message = message;
		error_state.dev_start = vc->start;
		error_state.dev_len = vc->size;
		error_state.dev = vc->dev->bdev;
		error_state.hash_dev_start = vc->hash_start;
		error_state.hash_dev_len = vc->bht.sectors;
		error_state.hash_dev = vc->hash_dev->bdev;

		/* Set default fallthrough behavior. */
		error_state.behavior = DM_VERITY_ERROR_BEHAVIOR_PANIC;
		error_mode = DM_VERITY_ERROR_BEHAVIOR_PANIC;

		if (!blocking_notifier_call_chain(
		    &verity_error_notifier, transient, &error_state)) {
			error_mode = error_state.behavior;
		}
	}

	switch (error_mode) {
	case DM_VERITY_ERROR_BEHAVIOR_EIO:
		break;
	case DM_VERITY_ERROR_BEHAVIOR_NONE:
		if (error != -EIO && io)
			io->error = 0;
		break;
	default:
		goto do_panic;
	}
	return;

do_panic:
	panic("dm-verity failure: "
	      "device:%u:%u error:%d block:%llu message:%s",
	      MAJOR(devt), MINOR(devt), error, ULL(block), message);
}

/**
 * verity_parse_error_behavior - parse a behavior charp to the enum
 * @behavior:	NUL-terminated char array
 *
 * Checks if the behavior is valid either as text or as an index digit
 * and returns the proper enum value or -1 on error.
 */
static int verity_parse_error_behavior(const char *behavior)
{
	const char * const *allowed = allowed_error_behaviors;
	char index = '0';

	for (; *allowed; allowed++, index++)
		if (!strcmp(*allowed, behavior) || behavior[0] == index)
			break;

	if (!*allowed)
		return -1;

	/* Convert to the integer index matching the enum. */
	return allowed - allowed_error_behaviors;
}

/*
 * Reverse flow of requests into the device.
 *
 * (Start at the bottom with verity_map and work your way upward).
 */

static void verity_inc_pending(struct dm_verity_io *io);

static void verity_return_bio_to_caller(struct dm_verity_io *io)
{
	struct verity_config *vc = io->target->private;

	if (io->error)
		verity_error(vc, io, io->error);

	bio_endio(io->bio, io->error);
	mempool_free(io, vc->io_pool);
}

/* Check for any missing bht hashes. */
static bool verity_is_bht_populated(struct dm_verity_io *io)
{
	struct verity_config *vc = io->target->private;
	u64 block;

	for (block = io->block; block < io->block + io->count; ++block)
		if (!dm_bht_is_populated(&vc->bht, block))
			return false;

	return true;
}

/* verity_dec_pending manages the lifetime of all dm_verity_io structs.
 * Non-bug error handling is centralized through this interface and
 * all passage from workqueue to workqueue.
 */
static void verity_dec_pending(struct dm_verity_io *io)
{
	if (!atomic_dec_and_test(&io->pending))
		goto done;

	if (unlikely(io->error))
		goto io_error;

	/* I/Os that were pending may now be ready */
	if (verity_is_bht_populated(io)) {
		INIT_DELAYED_WORK(&io->work, kverityd_verify);
		queue_delayed_work(kveritydq, &io->work, 0);
	} else {
		INIT_DELAYED_WORK(&io->work, kverityd_io);
		queue_delayed_work(kverityd_ioq, &io->work, HZ/10);
	}

done:
	return;

io_error:
	verity_return_bio_to_caller(io);
}

/* Walks the data set and computes the hash of the data read from the
 * untrusted source device.  The computed hash is then passed to dm-bht
 * for verification.
 */
static int verity_verify(struct verity_config *vc,
			 struct dm_verity_io *io)
{
	unsigned int block_size = vc->bht.block_size;
	struct bio *bio = io->bio;
	u64 block = io->block;
	unsigned int idx;
	int r;

	for (idx = bio->bi_idx; idx < bio->bi_vcnt; idx++) {
		struct bio_vec *bv = bio_iovec_idx(bio, idx);
		unsigned int offset = bv->bv_offset;
		unsigned int len = bv->bv_len;

		VERITY_BUG_ON(offset % block_size);
		VERITY_BUG_ON(len % block_size);

		while (len) {
			r = dm_bht_verify_block(&vc->bht, block,
						bv->bv_page, offset);
			if (r)
				goto bad_return;

			offset += block_size;
			len -= block_size;
			block++;
			cond_resched();
		}
	}

	return 0;

bad_return:
	/* dm_bht functions aren't expected to return errno friendly
	 * values.  They are converted here for uniformity.
	 */
	if (r > 0) {
		DMERR("Pending data for block %llu seen at verify", ULL(block));
		r = -EBUSY;
	} else {
		DMERR_LIMIT("Block hash does not match!");
		r = -EACCES;
	}
	return r;
}

/* Services the verify workqueue */
static void kverityd_verify(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work, struct delayed_work,
						  work);
	struct dm_verity_io *io = container_of(dwork, struct dm_verity_io,
					       work);
	struct verity_config *vc = io->target->private;

	io->error = verity_verify(vc, io);

	/* Free up the bio and tag with the return value */
	verity_return_bio_to_caller(io);
}

/* Asynchronously called upon the completion of dm-bht I/O.  The status
 * of the operation is passed back to dm-bht and the next steps are
 * decided by verity_dec_pending.
 */
static void kverityd_io_bht_populate_end(struct bio *bio, int error)
{
	struct dm_bht_entry *entry = (struct dm_bht_entry *) bio->bi_private;
	struct dm_verity_io *io = (struct dm_verity_io *) entry->io_context;

	/* Tell the tree to atomically update now that we've populated
	 * the given entry.
	 */
	dm_bht_read_completed(entry, error);

	/* Clean up for reuse when reading data to be checked */
	bio->bi_vcnt = 0;
	bio->bi_io_vec->bv_offset = 0;
	bio->bi_io_vec->bv_len = 0;
	bio->bi_io_vec->bv_page = NULL;
	/* Restore the private data to I/O so the destructor can be shared. */
	bio->bi_private = (void *) io;
	bio_put(bio);

	/* We bail but assume the tree has been marked bad. */
	if (unlikely(error)) {
		DMERR("Failed to read for sector %llu (%u)",
		      ULL(io->bio->bi_sector), io->bio->bi_size);
		io->error = error;
		/* Pass through the error to verity_dec_pending below */
	}
	/* When pending = 0, it will transition to reading real data */
	verity_dec_pending(io);
}

/* Called by dm-bht (via dm_bht_populate), this function provides
 * the message digests to dm-bht that are stored on disk.
 */
static int kverityd_bht_read_callback(void *ctx, sector_t start, u8 *dst,
				      sector_t count,
				      struct dm_bht_entry *entry)
{
	struct dm_verity_io *io = ctx;  /* I/O for this batch */
	struct verity_config *vc;
	struct bio *bio;

	vc = io->target->private;

	/* The I/O context is nested inside the entry so that we don't need one
	 * io context per page read.
	 */
	entry->io_context = ctx;

	/* We should only get page size requests at present. */
	verity_inc_pending(io);
	bio = verity_alloc_bioset(vc, GFP_NOIO, 1);
	if (unlikely(!bio)) {
		DMCRIT("Out of memory at bio_alloc_bioset");
		dm_bht_read_completed(entry, -ENOMEM);
		return -ENOMEM;
	}
	bio->bi_private = (void *) entry;
	bio->bi_idx = 0;
	bio->bi_size = vc->bht.block_size;
	bio->bi_sector = vc->hash_start + start;
	bio->bi_bdev = vc->hash_dev->bdev;
	bio->bi_end_io = kverityd_io_bht_populate_end;
	bio->bi_rw = REQ_META;
	/* Only need to free the bio since the page is managed by bht */
	bio->bi_destructor = dm_verity_bio_destructor;
	bio->bi_vcnt = 1;
	bio->bi_io_vec->bv_offset = offset_in_page(dst);
	bio->bi_io_vec->bv_len = to_bytes(count);
	/* dst is guaranteed to be a page_pool allocation */
	bio->bi_io_vec->bv_page = virt_to_page(dst);
	/* Track that this I/O is in use.  There should be no risk of the io
	 * being removed prior since this is called synchronously.
	 */
	generic_make_request(bio);
	return 0;
}

/* Submits an io request for each missing block of block hashes.
 * The last one to return will then enqueue this on the io workqueue.
 */
static void kverityd_io_bht_populate(struct dm_verity_io *io)
{
	struct verity_config *vc = io->target->private;
	u64 block;

	for (block = io->block; block < io->block + io->count; ++block) {
		int ret = dm_bht_populate(&vc->bht, io, block);

		if (ret < 0) {
			/* verity_dec_pending will handle the error case. */
			io->error = ret;
			break;
		}
	}
}

/* Asynchronously called upon the completion of I/O issued
 * from kverityd_src_io_read. verity_dec_pending() acts as
 * the scheduler/flow manager.
 */
static void kverityd_src_io_read_end(struct bio *clone, int error)
{
	struct dm_verity_io *io = clone->bi_private;

	if (unlikely(!bio_flagged(clone, BIO_UPTODATE) && !error))
		error = -EIO;

	if (unlikely(error)) {
		DMERR("Error occurred: %d (%llu, %u)",
			error, ULL(clone->bi_sector), clone->bi_size);
		io->error = error;
	}

	/* Release the clone which just avoids the block layer from
	 * leaving offsets, etc in unexpected states.
	 */
	bio_put(clone);

	verity_dec_pending(io);
}

/* If not yet underway, an I/O request will be issued to the vc->dev
 * device for the data needed. It is cloned to avoid unexpected changes
 * to the original bio struct.
 */
static void kverityd_src_io_read(struct dm_verity_io *io)
{
	struct bio *clone;

	/* Check if the read is already issued. */
	if (io->flags & VERITY_IOFLAGS_CLONED)
		return;

	io->flags |= VERITY_IOFLAGS_CLONED;

	/* Clone the bio. The block layer may modify the bvec array. */
	clone = verity_bio_clone(io);
	if (unlikely(!clone)) {
		io->error = -ENOMEM;
		return;
	}

	verity_inc_pending(io);

	generic_make_request(clone);
}

/* kverityd_io services the I/O workqueue. For each pass through
 * the I/O workqueue, a call to populate both the origin drive
 * data and the hash tree data is made.
 */
static void kverityd_io(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work, struct delayed_work,
						  work);
	struct dm_verity_io *io = container_of(dwork, struct dm_verity_io,
					       work);

	/* Issue requests asynchronously. */
	verity_inc_pending(io);
	kverityd_src_io_read(io);
	kverityd_io_bht_populate(io);
	verity_dec_pending(io);
}

/* Paired with verity_dec_pending, the pending value in the io dictate the
 * lifetime of a request and when it is ready to be processed on the
 * workqueues.
 */
static void verity_inc_pending(struct dm_verity_io *io)
{
	atomic_inc(&io->pending);
}

/* Block-level requests start here. */
static int verity_map(struct dm_target *ti, struct bio *bio,
		      union map_info *map_context)
{
	struct dm_verity_io *io;
	struct verity_config *vc;
	struct request_queue *r_queue;

	if (unlikely(!ti)) {
		DMERR("dm_target was NULL");
		return -EIO;
	}

	vc = ti->private;
	r_queue = bdev_get_queue(vc->dev->bdev);

	if (bio_data_dir(bio) == WRITE) {
		/* If we silently drop writes, then the VFS layer will cache
		 * the write and persist it in memory. While it doesn't change
		 * the underlying storage, it still may be contrary to the
		 * behavior expected by a verified, read-only device.
		 */
		DMWARN_LIMIT("write request received. rejecting with -EIO.");
		verity_error(vc, NULL, -EIO);
		return -EIO;
	} else {
		/* Queue up the request to be verified */
		io = verity_io_alloc(ti, bio);
		if (!io) {
			DMERR_LIMIT("Failed to allocate and init IO data");
			return DM_MAPIO_REQUEUE;
		}
		INIT_DELAYED_WORK(&io->work, kverityd_io);
		queue_delayed_work(kverityd_ioq, &io->work, 0);
	}

	return DM_MAPIO_SUBMITTED;
}

static void splitarg(char *arg, char **key, char **val)
{
	*key = strsep(&arg, "=");
	*val = strsep(&arg, "");
}

/*
 * Non-block interfaces and device-mapper specific code
 */

/**
 * verity_ctr - Construct a verified mapping
 * @ti:   Target being created
 * @argc: Number of elements in argv
 * @argv: Vector of key-value pairs (see below).
 *
 * Accepts the following keys:
 * @payload:        hashed device
 * @hashtree:       device hashtree is stored on
 * @hashstart:      start address of hashes (default 0)
 * @block_size:     size of a hash block
 * @alg:            hash algorithm
 * @root_hexdigest: toplevel hash of the tree
 * @error_behavior: what to do when verification fails [optional]
 * @salt:           salt, in hex [optional]
 *
 * E.g.,
 * payload=/dev/sda2 hashtree=/dev/sda3 alg=sha256
 * root_hexdigest=f08aa4a3695290c569eb1b0ac032ae1040150afb527abbeb0a3da33d82fb2c6e
 *
 * TODO(wad):
 * - Boot time addition
 * - Track block verification to free block_hashes if memory use is a concern
 * Testing needed:
 * - Regular slub_debug tracing (on checkins)
 * - Improper block hash padding
 * - Improper bundle padding
 * - Improper hash layout
 * - Missing padding at end of device
 * - Improperly sized underlying devices
 * - Out of memory conditions (make sure this isn't too flaky under high load!)
 * - Incorrect superhash
 * - Incorrect block hashes
 * - Incorrect bundle hashes
 * - Boot-up read speed; sustained read speeds
 */
static int verity_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct verity_config *vc = NULL;
	int ret = 0;
	sector_t blocks;
	unsigned int block_size = VERITY_DEFAULT_BLOCK_SIZE;
	const char *payload = NULL;
	const char *hashtree = NULL;
	unsigned long hashstart = 0;
	const char *alg = NULL;
	const char *root_hexdigest = NULL;
	const char *dev_error_behavior = error_behavior;
	const char *hexsalt = "";
	int i;

	for (i = 0; i < argc; ++i) {
		char *key, *val;
		DMWARN("Argument %d: '%s'", i, argv[i]);
		splitarg(argv[i], &key, &val);
		if (!key) {
			DMWARN("Bad argument %d: missing key?", i);
			break;
		}
		if (!val) {
			DMWARN("Bad argument %d='%s': missing value", i, key);
			break;
		}

		if (!strcmp(key, "alg")) {
			alg = val;
		} else if (!strcmp(key, "payload")) {
			payload = val;
		} else if (!strcmp(key, "hashtree")) {
			hashtree = val;
		} else if (!strcmp(key, "root_hexdigest")) {
			root_hexdigest = val;
		} else if (!strcmp(key, "hashstart")) {
			if (strict_strtoul(val, 10, &hashstart)) {
				ti->error = "Invalid hashstart";
				return -EINVAL;
			}
		} else if (!strcmp(key, "block_size")) {
			unsigned long tmp;
			if (strict_strtoul(val, 10, &tmp) ||
			    (tmp > UINT_MAX)) {
				ti->error = "Invalid block_size";
				return -EINVAL;
			}
			block_size = (unsigned int)tmp;
		} else if (!strcmp(key, "error_behavior")) {
			dev_error_behavior = val;
		} else if (!strcmp(key, "salt")) {
			hexsalt = val;
		} else if (!strcmp(key, "error_behavior")) {
			dev_error_behavior = val;
		}
	}

#define NEEDARG(n) \
	if (!(n)) { \
		ti->error = "Missing argument: " #n; \
		return -EINVAL; \
	}

	NEEDARG(alg);
	NEEDARG(payload);
	NEEDARG(hashtree);
	NEEDARG(root_hexdigest);

#undef NEEDARG

	/* The device mapper device should be setup read-only */
	if ((dm_table_get_mode(ti->table) & ~FMODE_READ) != 0) {
		ti->error = "Must be created readonly.";
		return -EINVAL;
	}

	vc = kzalloc(sizeof(*vc), GFP_KERNEL);
	if (!vc) {
		/* TODO(wad) if this is called from the setup helper, then we
		 * catch these errors and do a CrOS specific thing. if not, we
		 * need to have this call the error handler.
		 */
		return -EINVAL;
	}

	/* Calculate the blocks from the given device size */
	vc->size = ti->len;
	blocks = to_bytes(vc->size) / block_size;
	if (dm_bht_create(&vc->bht, blocks, block_size, alg)) {
		DMERR("failed to create required bht");
		goto bad_bht;
	}
	if (dm_bht_set_root_hexdigest(&vc->bht, root_hexdigest)) {
		DMERR("root hexdigest error");
		goto bad_root_hexdigest;
	}
	dm_bht_set_salt(&vc->bht, hexsalt);
	vc->bht.read_cb = kverityd_bht_read_callback;

	/* payload: device to verify */
	vc->start = 0;  /* TODO: should this support a starting offset? */
	/* We only ever grab the device in read-only mode. */
	ret = dm_get_device(ti, payload,
			    dm_table_get_mode(ti->table), &vc->dev);
	if (ret) {
		DMERR("Failed to acquire device '%s': %d", payload, ret);
		ti->error = "Device lookup failed";
		goto bad_verity_dev;
	}

	if ((to_bytes(vc->start) % block_size) ||
	    (to_bytes(vc->size) % block_size)) {
		ti->error = "Device must be block_size divisble/aligned";
		goto bad_hash_start;
	}

	vc->hash_start = (sector_t)hashstart;

	/* hashtree: device with hashes.
	 * Note, payload == hashtree is okay as long as the size of
	 *       ti->len passed to device mapper does not include
	 *       the hashes.
	 */
	if (dm_get_device(ti, hashtree,
			  dm_table_get_mode(ti->table), &vc->hash_dev)) {
		ti->error = "Hash device lookup failed";
		goto bad_hash_dev;
	}

	/* arg4: cryptographic digest algorithm */
	if (snprintf(vc->hash_alg, CRYPTO_MAX_ALG_NAME, "%s", alg) >=
	    CRYPTO_MAX_ALG_NAME) {
		ti->error = "Hash algorithm name is too long";
		goto bad_hash;
	}

	/* override with optional device-specific error behavior */
	vc->error_behavior = verity_parse_error_behavior(dev_error_behavior);
	if (vc->error_behavior == -1) {
		ti->error = "Bad error_behavior supplied";
		goto bad_err_behavior;
	}

	/* TODO: Maybe issues a request on the io queue for block 0? */

	/* Argument processing is done, setup operational data */
	/* Pool for dm_verity_io objects */
	vc->io_pool = mempool_create_slab_pool(MIN_IOS, _verity_io_pool);
	if (!vc->io_pool) {
		ti->error = "Cannot allocate verity io mempool";
		goto bad_slab_pool;
	}

	/* Allocate the bioset used for request padding */
	/* TODO(wad) allocate a separate bioset for the first verify maybe */
	vc->bs = bioset_create(MIN_BIOS, 0);
	if (!vc->bs) {
		ti->error = "Cannot allocate verity bioset";
		goto bad_bs;
	}

	ti->num_flush_requests = 1;
	ti->private = vc;

	/* TODO(wad) add device and hash device names */
	{
		char hashdev[BDEVNAME_SIZE], vdev[BDEVNAME_SIZE];
		bdevname(vc->hash_dev->bdev, hashdev);
		bdevname(vc->dev->bdev, vdev);
		DMINFO("dev:%s hash:%s [sectors:%llu blocks:%llu]", vdev,
		       hashdev, ULL(vc->bht.sectors), ULL(blocks));
	}
	return 0;

bad_bs:
	mempool_destroy(vc->io_pool);
bad_slab_pool:
bad_err_behavior:
bad_hash:
	dm_put_device(ti, vc->hash_dev);
bad_hash_dev:
bad_hash_start:
	dm_put_device(ti, vc->dev);
bad_bht:
bad_root_hexdigest:
bad_verity_dev:
	kfree(vc);   /* hash is not secret so no need to zero */
	return -EINVAL;
}

static void verity_dtr(struct dm_target *ti)
{
	struct verity_config *vc = (struct verity_config *) ti->private;

	bioset_free(vc->bs);
	mempool_destroy(vc->io_pool);
	dm_bht_destroy(&vc->bht);
	dm_put_device(ti, vc->hash_dev);
	dm_put_device(ti, vc->dev);
	kfree(vc);
}

static int verity_status(struct dm_target *ti, status_type_t type,
			char *result, unsigned int maxlen)
{
	struct verity_config *vc = (struct verity_config *) ti->private;
	unsigned int sz = 0;
	char hashdev[BDEVNAME_SIZE], vdev[BDEVNAME_SIZE];
	u8 hexdigest[VERITY_MAX_DIGEST_SIZE * 2 + 1] = { 0 };

	dm_bht_root_hexdigest(&vc->bht, hexdigest, sizeof(hexdigest));

	switch (type) {
	case STATUSTYPE_INFO:
		break;
	case STATUSTYPE_TABLE:
		bdevname(vc->hash_dev->bdev, hashdev);
		bdevname(vc->dev->bdev, vdev);
		DMEMIT("/dev/%s /dev/%s %llu %u %s %s",
			vdev,
			hashdev,
			ULL(vc->hash_start),
			vc->bht.depth,
			vc->hash_alg,
			hexdigest);
		break;
	}
	return 0;
}

static int verity_merge(struct dm_target *ti, struct bvec_merge_data *bvm,
		       struct bio_vec *biovec, int max_size)
{
	struct verity_config *vc = ti->private;
	struct request_queue *q = bdev_get_queue(vc->dev->bdev);

	if (!q->merge_bvec_fn)
		return max_size;

	bvm->bi_bdev = vc->dev->bdev;
	bvm->bi_sector = vc->start + bvm->bi_sector - ti->begin;

	/* Optionally, this could just return 0 to stick to single pages. */
	return min(max_size, q->merge_bvec_fn(q, bvm, biovec));
}

static int verity_iterate_devices(struct dm_target *ti,
				 iterate_devices_callout_fn fn, void *data)
{
	struct verity_config *vc = ti->private;

	return fn(ti, vc->dev, vc->start, ti->len, data);
}

static void verity_io_hints(struct dm_target *ti,
			    struct queue_limits *limits)
{
	struct verity_config *vc = ti->private;
	unsigned int block_size = vc->bht.block_size;

	limits->logical_block_size = block_size;
	limits->physical_block_size = block_size;
	blk_limits_io_min(limits, block_size);
}

static struct target_type verity_target = {
	.name   = "verity",
	.version = {0, 1, 0},
	.module = THIS_MODULE,
	.ctr    = verity_ctr,
	.dtr    = verity_dtr,
	.map    = verity_map,
	.merge  = verity_merge,
	.status = verity_status,
	.iterate_devices = verity_iterate_devices,
	.io_hints = verity_io_hints,
};

#define VERITY_WQ_FLAGS (WQ_CPU_INTENSIVE|WQ_HIGHPRI)

static int __init dm_verity_init(void)
{
	int r = -ENOMEM;

	_verity_io_pool = KMEM_CACHE(dm_verity_io, 0);
	if (!_verity_io_pool) {
		DMERR("failed to allocate pool dm_verity_io");
		goto bad_io_pool;
	}

	kverityd_ioq = alloc_workqueue("kverityd_io", VERITY_WQ_FLAGS, 1);
	if (!kverityd_ioq) {
		DMERR("failed to create workqueue kverityd_ioq");
		goto bad_io_queue;
	}

	kveritydq = alloc_workqueue("kverityd", VERITY_WQ_FLAGS, 1);
	if (!kveritydq) {
		DMERR("failed to create workqueue kveritydq");
		goto bad_verify_queue;
	}

	r = dm_register_target(&verity_target);
	if (r < 0) {
		DMERR("register failed %d", r);
		goto register_failed;
	}

	DMINFO("version %u.%u.%u loaded", verity_target.version[0],
	       verity_target.version[1], verity_target.version[2]);

	return r;

register_failed:
	destroy_workqueue(kveritydq);
bad_verify_queue:
	destroy_workqueue(kverityd_ioq);
bad_io_queue:
	kmem_cache_destroy(_verity_io_pool);
bad_io_pool:
	return r;
}

static void __exit dm_verity_exit(void)
{
	destroy_workqueue(kveritydq);
	destroy_workqueue(kverityd_ioq);

	dm_unregister_target(&verity_target);
	kmem_cache_destroy(_verity_io_pool);
}

module_init(dm_verity_init);
module_exit(dm_verity_exit);

MODULE_AUTHOR("The Chromium OS Authors <chromium-os-dev chromium org>");
MODULE_DESCRIPTION(DM_NAME " target for transparent disk integrity checking");
MODULE_LICENSE("GPL");
