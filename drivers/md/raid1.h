#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _RAID1_H
#define _RAID1_H

typedef struct mirror_info mirror_info_t;

struct mirror_info {
	mdk_rdev_t	*rdev;
	sector_t	head_position;
};

struct pool_info {
	mddev_t *mddev;
	int	raid_disks;
};

typedef struct r1bio_s r1bio_t;

struct r1_private_data_s {
	mddev_t			*mddev;
	mirror_info_t		*mirrors;
	int			raid_disks;
	int			last_used;
	sector_t		next_seq_sect;
	spinlock_t		device_lock;

	struct list_head	retry_list;
	 
	struct bio_list		pending_bio_list;
	 
	struct bio_list		flushing_bio_list;

	spinlock_t		resync_lock;
	int			nr_pending;
	int			nr_waiting;
	int			nr_queued;
	int			barrier;
	sector_t		next_resync;
	int			fullsync;   

	wait_queue_head_t	wait_barrier;

	struct pool_info	*poolinfo;

	struct page		*tmppage;

	mempool_t *r1bio_pool;
	mempool_t *r1buf_pool;
};

typedef struct r1_private_data_s conf_t;

struct r1bio_s {
	atomic_t		remaining;  
	atomic_t		behind_remaining;  
	sector_t		sector;
	int			sectors;
	unsigned long		state;
	mddev_t			*mddev;
	 
	struct bio		*master_bio;
	 
	int			read_disk;

	struct list_head	retry_list;
	struct bitmap_update	*bitmap_update;
#ifdef MY_ABC_HERE
	unsigned int read_failed;
	int orig_disk_idx;
#endif
	 
	struct bio		*bios[0];
	 
};

#define IO_BLOCKED ((struct bio*)1)

#define	R1BIO_Uptodate	0
#define	R1BIO_IsSync	1
#define	R1BIO_Degraded	2
#define	R1BIO_BehindIO	3
#define	R1BIO_Barrier	4
#define R1BIO_BarrierRetry 5
 
#define	R1BIO_Returned 6

#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
void raid1_raise_barrier(conf_t *conf);
void raid1_lower_barrier(conf_t *conf);
void raid1_wait_barrier(conf_t *conf);
void raid1_allow_barrier(conf_t *conf);
#endif

#endif
