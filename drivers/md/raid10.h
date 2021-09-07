#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _RAID10_H
#define _RAID10_H

typedef struct mirror_info mirror_info_t;

struct mirror_info {
	mdk_rdev_t	*rdev;
	sector_t	head_position;
};

typedef struct r10bio_s r10bio_t;

struct r10_private_data_s {
	mddev_t			*mddev;
	mirror_info_t		*mirrors;
	int			raid_disks;
	spinlock_t		device_lock;

	int			near_copies;   
	int 			far_copies;    
	int			far_offset;    
	int			copies;	       
	sector_t		stride;	       

	int chunk_shift;  
	sector_t chunk_mask;

	struct list_head	retry_list;
	 
	struct bio_list		pending_bio_list;

	spinlock_t		resync_lock;
	int nr_pending;
	int nr_waiting;
	int nr_queued;
	int barrier;
	sector_t		next_resync;
	int			fullsync;   

	wait_queue_head_t	wait_barrier;

	mempool_t *r10bio_pool;
	mempool_t *r10buf_pool;
	struct page		*tmppage;
};

typedef struct r10_private_data_s conf_t;

struct r10bio_s {
	atomic_t		remaining;  
	sector_t		sector;	 
	int			sectors;
	unsigned long		state;
	mddev_t			*mddev;
	 
	struct bio		*master_bio;
	 
	int			read_slot;

	struct list_head	retry_list;
	 
	struct {
		struct bio		*bio;
		sector_t addr;
		int devnum;
	} devs[0];
};

#define IO_BLOCKED ((struct bio*)1)

#define	R10BIO_Uptodate	0
#define	R10BIO_IsSync	1
#define	R10BIO_IsRecover 2
#define	R10BIO_Degraded 3
#ifdef MY_ABC_HERE
#define R10BIO_FIX_READ_ERROR 4
#endif  

#endif
