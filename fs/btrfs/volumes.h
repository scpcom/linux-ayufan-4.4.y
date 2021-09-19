#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __BTRFS_VOLUMES_
#define __BTRFS_VOLUMES_

#include <linux/bio.h>
#include <linux/sort.h>
#include <linux/btrfs.h>
#include "async-thread.h"

#define BTRFS_STRIPE_LEN	(64 * 1024)

struct buffer_head;
struct btrfs_pending_bios {
	struct bio *head;
	struct bio *tail;
};

struct btrfs_device {
	struct list_head dev_list;
	struct list_head dev_alloc_list;
	struct btrfs_fs_devices *fs_devices;
	struct btrfs_root *dev_root;

	struct btrfs_pending_bios pending_bios;
	 
	struct btrfs_pending_bios pending_sync_bios;

	u64 generation;
	int running_pending;
	int writeable;
	int in_fs_metadata;
	int missing;
	int can_discard;
	int is_tgtdev_for_dev_replace;

	spinlock_t io_lock;
	 
	fmode_t mode;

	struct block_device *bdev;

	struct rcu_string *name;

	u64 devid;

	u64 total_bytes;

	u64 disk_total_bytes;

	u64 bytes_used;

	u32 io_align;

	u32 io_width;
	 
	u64 type;

	u32 sector_size;

	u8 uuid[BTRFS_UUID_SIZE];

	int nobarriers;
	struct bio *flush_bio;
	struct completion flush_wait;

	struct scrub_ctx *scrub_device;

	struct btrfs_work work;
	struct rcu_head rcu;
	struct work_struct rcu_work;

	spinlock_t reada_lock;
	atomic_t reada_in_flight;
	u64 reada_next;
	struct reada_zone *reada_curr_zone;
	struct radix_tree_root reada_zones;
	struct radix_tree_root reada_extents;

	int dev_stats_valid;
	int dev_stats_dirty;  
	atomic_t dev_stat_values[BTRFS_DEV_STAT_VALUES_MAX];
};

struct btrfs_fs_devices {
	u8 fsid[BTRFS_FSID_SIZE];  

	u64 latest_devid;
	u64 latest_trans;
	u64 num_devices;
	u64 open_devices;
	u64 rw_devices;
	u64 missing_devices;
	u64 total_rw_bytes;
	u64 num_can_discard;
	u64 total_devices;
	struct block_device *latest_bdev;

	struct mutex device_list_mutex;
	struct list_head devices;

	struct list_head alloc_list;
	struct list_head list;

	struct btrfs_fs_devices *seed;
	int seeding;

	int opened;

	int rotating;
};

#define BTRFS_BIO_INLINE_CSUM_SIZE	64

typedef void (btrfs_io_bio_end_io_t) (struct btrfs_io_bio *bio, int err);
struct btrfs_io_bio {
	unsigned long mirror_num;
	unsigned long stripe_index;
	u8 *csum;
	u8 csum_inline[BTRFS_BIO_INLINE_CSUM_SIZE];
	u8 *csum_allocated;
	btrfs_io_bio_end_io_t *end_io;
	struct bio bio;
};

static inline struct btrfs_io_bio *btrfs_io_bio(struct bio *bio)
{
	return container_of(bio, struct btrfs_io_bio, bio);
}

struct btrfs_bio_stripe {
	struct btrfs_device *dev;
	u64 physical;
	u64 length;  
};

struct btrfs_bio;
typedef void (btrfs_bio_end_io_t) (struct btrfs_bio *bio, int err);

struct btrfs_bio {
	atomic_t stripes_pending;
#ifdef MY_ABC_HERE
#else
	struct btrfs_fs_info *fs_info;
#endif  
	bio_end_io_t *end_io;
	struct bio *orig_bio;
	void *private;
	atomic_t error;
	int max_errors;
	int num_stripes;
	int mirror_num;
	struct btrfs_bio_stripe stripes[];
};

struct btrfs_device_info {
	struct btrfs_device *dev;
	u64 dev_offset;
	u64 max_avail;
	u64 total_avail;
};

struct btrfs_raid_attr {
	int sub_stripes;	 
	int dev_stripes;	 
	int devs_max;		 
	int devs_min;		 
	int devs_increment;	 
	int ncopies;		 
};

struct map_lookup {
	u64 type;
	int io_align;
	int io_width;
	int stripe_len;
	int sector_size;
	int num_stripes;
	int sub_stripes;
	struct btrfs_bio_stripe stripes[];
};

#define map_lookup_size(n) (sizeof(struct map_lookup) + \
			    (sizeof(struct btrfs_bio_stripe) * (n)))

#define BTRFS_BALANCE_DATA		(1ULL << 0)
#define BTRFS_BALANCE_SYSTEM		(1ULL << 1)
#define BTRFS_BALANCE_METADATA		(1ULL << 2)

#define BTRFS_BALANCE_TYPE_MASK		(BTRFS_BALANCE_DATA |	    \
					 BTRFS_BALANCE_SYSTEM |	    \
					 BTRFS_BALANCE_METADATA)

#define BTRFS_BALANCE_FORCE		(1ULL << 3)
#define BTRFS_BALANCE_RESUME		(1ULL << 4)

#define BTRFS_BALANCE_ARGS_PROFILES	(1ULL << 0)
#define BTRFS_BALANCE_ARGS_USAGE	(1ULL << 1)
#define BTRFS_BALANCE_ARGS_DEVID	(1ULL << 2)
#define BTRFS_BALANCE_ARGS_DRANGE	(1ULL << 3)
#define BTRFS_BALANCE_ARGS_VRANGE	(1ULL << 4)
#define BTRFS_BALANCE_ARGS_LIMIT	(1ULL << 5)

#define BTRFS_BALANCE_ARGS_CONVERT	(1ULL << 8)
#define BTRFS_BALANCE_ARGS_SOFT		(1ULL << 9)

struct btrfs_balance_args;
struct btrfs_balance_progress;
struct btrfs_balance_control {
	struct btrfs_fs_info *fs_info;

	struct btrfs_balance_args data;
	struct btrfs_balance_args meta;
	struct btrfs_balance_args sys;

	u64 flags;

	struct btrfs_balance_progress stat;
};

int btrfs_account_dev_extents_size(struct btrfs_device *device, u64 start,
				   u64 end, u64 *length);

#define btrfs_bio_size(n) (sizeof(struct btrfs_bio) + \
			    (sizeof(struct btrfs_bio_stripe) * (n)))

int btrfs_map_block(struct btrfs_fs_info *fs_info, int rw,
		    u64 logical, u64 *length,
		    struct btrfs_bio **bbio_ret, int mirror_num);
int btrfs_rmap_block(struct btrfs_mapping_tree *map_tree,
		     u64 chunk_start, u64 physical, u64 devid,
		     u64 **logical, int *naddrs, int *stripe_len);
int btrfs_read_sys_array(struct btrfs_root *root);
int btrfs_read_chunk_tree(struct btrfs_root *root);
int btrfs_alloc_chunk(struct btrfs_trans_handle *trans,
		      struct btrfs_root *extent_root, u64 type);
void btrfs_mapping_init(struct btrfs_mapping_tree *tree);
void btrfs_mapping_tree_free(struct btrfs_mapping_tree *tree);
int btrfs_map_bio(struct btrfs_root *root, int rw, struct bio *bio,
		  int mirror_num, int async_submit);
int btrfs_open_devices(struct btrfs_fs_devices *fs_devices,
		       fmode_t flags, void *holder);
int btrfs_scan_one_device(const char *path, fmode_t flags, void *holder,
			  struct btrfs_fs_devices **fs_devices_ret);
int btrfs_close_devices(struct btrfs_fs_devices *fs_devices);
void btrfs_close_extra_devices(struct btrfs_fs_info *fs_info,
			       struct btrfs_fs_devices *fs_devices, int step);
int btrfs_find_device_missing_or_by_path(struct btrfs_root *root,
					 char *device_path,
					 struct btrfs_device **device);
struct btrfs_device *btrfs_alloc_device(struct btrfs_fs_info *fs_info,
					const u64 *devid,
					const u8 *uuid);
int btrfs_rm_device(struct btrfs_root *root, char *device_path);
void btrfs_cleanup_fs_uuids(void);
int btrfs_num_copies(struct btrfs_fs_info *fs_info, u64 logical, u64 len);
int btrfs_grow_device(struct btrfs_trans_handle *trans,
		      struct btrfs_device *device, u64 new_size);
struct btrfs_device *btrfs_find_device(struct btrfs_fs_info *fs_info, u64 devid,
				       u8 *uuid, u8 *fsid);
int btrfs_shrink_device(struct btrfs_device *device, u64 new_size);
int btrfs_init_new_device(struct btrfs_root *root, char *path);
int btrfs_init_dev_replace_tgtdev(struct btrfs_root *root, char *device_path,
				  struct btrfs_device **device_out);
int btrfs_balance(struct btrfs_balance_control *bctl,
		  struct btrfs_ioctl_balance_args *bargs);
int btrfs_resume_balance_async(struct btrfs_fs_info *fs_info);
int btrfs_recover_balance(struct btrfs_fs_info *fs_info);
int btrfs_pause_balance(struct btrfs_fs_info *fs_info);
int btrfs_cancel_balance(struct btrfs_fs_info *fs_info);
int btrfs_create_uuid_tree(struct btrfs_fs_info *fs_info);
int btrfs_check_uuid_tree(struct btrfs_fs_info *fs_info);
int btrfs_chunk_readonly(struct btrfs_root *root, u64 chunk_offset);
int find_free_dev_extent(struct btrfs_trans_handle *trans,
			 struct btrfs_device *device, u64 num_bytes,
			 u64 *start, u64 *max_avail);
void btrfs_dev_stat_inc_and_print(struct btrfs_device *dev, int index);
int btrfs_get_dev_stats(struct btrfs_root *root,
			struct btrfs_ioctl_get_dev_stats *stats);
void btrfs_init_devices_late(struct btrfs_fs_info *fs_info);
int btrfs_init_dev_stats(struct btrfs_fs_info *fs_info);
int btrfs_run_dev_stats(struct btrfs_trans_handle *trans,
			struct btrfs_fs_info *fs_info);
void btrfs_rm_dev_replace_srcdev(struct btrfs_fs_info *fs_info,
				 struct btrfs_device *srcdev);
void btrfs_destroy_dev_replace_tgtdev(struct btrfs_fs_info *fs_info,
				      struct btrfs_device *tgtdev);
void btrfs_init_dev_replace_tgtdev_for_resume(struct btrfs_fs_info *fs_info,
					      struct btrfs_device *tgtdev);
int btrfs_scratch_superblock(struct btrfs_device *device);
int btrfs_is_parity_mirror(struct btrfs_mapping_tree *map_tree,
			   u64 logical, u64 len, int mirror_num);
unsigned long btrfs_full_stripe_len(struct btrfs_root *root,
				    struct btrfs_mapping_tree *map_tree,
				    u64 logical);
int btrfs_finish_chunk_alloc(struct btrfs_trans_handle *trans,
				struct btrfs_root *extent_root,
				u64 chunk_offset, u64 chunk_size);
int btrfs_remove_chunk(struct btrfs_trans_handle *trans,
		       struct btrfs_root *root, u64 chunk_offset);

static inline void btrfs_dev_stat_inc(struct btrfs_device *dev,
				      int index)
{
	atomic_inc(dev->dev_stat_values + index);
	dev->dev_stats_dirty = 1;
}

static inline int btrfs_dev_stat_read(struct btrfs_device *dev,
				      int index)
{
	return atomic_read(dev->dev_stat_values + index);
}

static inline int btrfs_dev_stat_read_and_reset(struct btrfs_device *dev,
						int index)
{
	int ret;

	ret = atomic_xchg(dev->dev_stat_values + index, 0);
	dev->dev_stats_dirty = 1;
	return ret;
}

static inline void btrfs_dev_stat_set(struct btrfs_device *dev,
				      int index, unsigned long val)
{
	atomic_set(dev->dev_stat_values + index, val);
	dev->dev_stats_dirty = 1;
}

static inline void btrfs_dev_stat_reset(struct btrfs_device *dev,
					int index)
{
	btrfs_dev_stat_set(dev, index, 0);
}

static inline void lock_chunks(struct btrfs_root *root)
{
	mutex_lock(&root->fs_info->chunk_mutex);
}

static inline void unlock_chunks(struct btrfs_root *root)
{
	mutex_unlock(&root->fs_info->chunk_mutex);
}

#endif
