#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _RAID5_H
#define _RAID5_H

#include <linux/raid/xor.h>
#include <linux/dmaengine.h>

enum check_states {
	check_state_idle = 0,
	check_state_run,  
	check_state_run_q,  
	check_state_run_pq,  
	check_state_check_result,
	check_state_compute_run,  
	check_state_compute_result,
};

enum reconstruct_states {
	reconstruct_state_idle = 0,
	reconstruct_state_prexor_drain_run,	 
	reconstruct_state_drain_run,		 
	reconstruct_state_run,			 
	reconstruct_state_prexor_drain_result,
	reconstruct_state_drain_result,
	reconstruct_state_result,
};

struct stripe_head {
	struct hlist_node	hash;
	struct list_head	lru;	       
	struct r5conf		*raid_conf;
	short			generation;	 
	sector_t		sector;		 
	short			pd_idx;		 
	short			qd_idx;		 
	short			ddf_layout; 
	unsigned long		state;		 
	atomic_t		count;	       
#ifdef MY_ABC_HERE
	atomic_t delayed_cnt;
#endif  
#ifdef MY_ABC_HERE
	spinlock_t		lock;
#endif
	int			bm_seq;	 
	int			disks;		 
	enum check_states	check_state;
	enum reconstruct_states reconstruct_state;
	 
	struct stripe_operations {
		int 		     target, target2;
		enum sum_check_flags zero_sum_result;
		#ifdef CONFIG_MULTICORE_RAID456
		unsigned long	     request;
		wait_queue_head_t    wait_for_ops;
		#endif
	} ops;
	struct r5dev {
		struct bio	req;
		struct bio_vec	vec;
		struct page	*page;
		struct bio	*toread, *read, *towrite, *written;
		sector_t	sector;			 
		unsigned long	flags;
	} dev[1];  
};

struct stripe_head_state {
	int syncing, expanding, expanded;
	int locked, uptodate, to_read, to_write, failed, written;
	int to_fill, compute, req_compute, non_overwrite;
	int failed_num[2];
	int p_failed, q_failed;
	int dec_preread_active;
	unsigned long ops_request;

	struct bio *return_bi;
	struct md_rdev *blocked_rdev;
	int handle_bad_blocks;
};

#define	R5_UPTODATE	0	 
#define	R5_LOCKED	1	 
#define	R5_OVERWRITE	2	 
 
#define	R5_Insync	3	 
#define	R5_Wantread	4	 
#define	R5_Wantwrite	5
#define	R5_Overlap	7	 
#define	R5_ReadError	8	 
#define	R5_ReWrite	9	 

#define	R5_Expanded	10	 
#define	R5_Wantcompute	11	 
#define	R5_Wantfill	12	 
#define	R5_Wantdrain	13	 
#define	R5_WantFUA	14	 
#define	R5_WriteError	15	 
#define	R5_MadeGood	16	 
#define	R5_Discard	19	 
 
#define RECONSTRUCT_WRITE	1
#define READ_MODIFY_WRITE	2
 
#define	CHECK_PARITY		3
 
#define UPDATE_PARITY		4

enum {
#ifndef MY_ABC_HERE
	STRIPE_ACTIVE,
#endif
	STRIPE_HANDLE,
	STRIPE_SYNC_REQUESTED,
	STRIPE_SYNCING,
	STRIPE_INSYNC,
	STRIPE_PREREAD_ACTIVE,
	STRIPE_DELAYED,
	STRIPE_DEGRADED,
	STRIPE_BIT_DELAY,
	STRIPE_EXPANDING,
	STRIPE_EXPAND_SOURCE,
	STRIPE_EXPAND_READY,
	STRIPE_IO_STARTED,	 
	STRIPE_FULL_WRITE,	 
	STRIPE_BIOFILL_RUN,
	STRIPE_COMPUTE_RUN,
	STRIPE_OPS_REQ_PENDING,
	STRIPE_DISCARD,
#ifdef MY_ABC_HERE
	STRIPE_NORETRY,
#endif
#ifdef MY_ABC_HERE
	STRIPE_ACTIVATE_STABLE,
	STRIPE_CHECK_STABLE_LIST,
#endif  
};

#define STRIPE_OP_BIOFILL	0
#define STRIPE_OP_COMPUTE_BLK	1
#define STRIPE_OP_PREXOR	2
#define STRIPE_OP_BIODRAIN	3
#define STRIPE_OP_RECONSTRUCT	4
#define STRIPE_OP_CHECK	5

struct disk_info {
	struct md_rdev	*rdev;
};

struct r5conf {
	struct hlist_head	*stripe_hashtbl;
	struct mddev		*mddev;
	struct disk_info	*spare;
	int			chunk_sectors;
	int			level, algorithm;
	int			max_degraded;
	int			raid_disks;
	int			max_nr_stripes;

	sector_t		reshape_progress;
	 
	sector_t		reshape_safe;
	int			previous_raid_disks;
	int			prev_chunk_sectors;
	int			prev_algo;
	short			generation;  
	unsigned long		reshape_checkpoint;  

	struct list_head	handle_list;  
	struct list_head	hold_list;  
	struct list_head	delayed_list;  
	struct list_head	bitmap_list;  
#ifdef MY_ABC_HERE
	 
	struct list_head    stable_list;
#endif  
	struct bio		*retry_read_aligned;  
	struct bio		*retry_read_aligned_list;  
	atomic_t		preread_active_stripes;  
	atomic_t		active_aligned_reads;
	atomic_t		pending_full_writes;  
	int			bypass_count;  
	int			bypass_threshold;  
	struct list_head	*last_hold;  

	atomic_t		reshape_stripes;  
	 
	int			active_name;
	char			cache_name[2][32];
	struct kmem_cache		*slab_cache;  

	int			seq_flush, seq_write;
	int			quiesce;

	int			fullsync;   
	int			recovery_disabled;
	 
	struct raid5_percpu {
		struct page	*spare_page;  
		void		*scribble;    
	} __percpu *percpu;
	size_t			scribble_len;  
#ifdef CONFIG_HOTPLUG_CPU
	struct notifier_block	cpu_notify;
#endif

	atomic_t		active_stripes;
	struct list_head	inactive_list;
	wait_queue_head_t	wait_for_stripe;
	wait_queue_head_t	wait_for_overlap;
	int			inactive_blocked;	 
	int			pool_size;  
	spinlock_t		device_lock;
	struct disk_info	*disks;

	struct md_thread	*thread;
};

#define ALGORITHM_LEFT_ASYMMETRIC	0  
#define ALGORITHM_RIGHT_ASYMMETRIC	1  
#define ALGORITHM_LEFT_SYMMETRIC	2  
#define ALGORITHM_RIGHT_SYMMETRIC	3  

#define ALGORITHM_PARITY_0		4  
#define ALGORITHM_PARITY_N		5  

#define ALGORITHM_ROTATING_ZERO_RESTART	8  
#define ALGORITHM_ROTATING_N_RESTART	9  
#define ALGORITHM_ROTATING_N_CONTINUE	10  

#define ALGORITHM_LEFT_ASYMMETRIC_6	16
#define ALGORITHM_RIGHT_ASYMMETRIC_6	17
#define ALGORITHM_LEFT_SYMMETRIC_6	18
#define ALGORITHM_RIGHT_SYMMETRIC_6	19
#define ALGORITHM_PARITY_0_6		20
#define ALGORITHM_PARITY_N_6		ALGORITHM_PARITY_N

static inline int algorithm_valid_raid5(int layout)
{
	return (layout >= 0) &&
		(layout <= 5);
}
static inline int algorithm_valid_raid6(int layout)
{
	return (layout >= 0 && layout <= 5)
		||
		(layout >= 8 && layout <= 10)
		||
		(layout >= 16 && layout <= 20);
}

static inline int algorithm_is_DDF(int layout)
{
	return layout >= 8 && layout <= 10;
}

extern int md_raid5_congested(struct mddev *mddev, int bits);
extern void md_raid5_kick_device(struct r5conf *conf);
extern int raid5_set_cache_size(struct mddev *mddev, int size);

#ifdef MY_ABC_HERE
#define sector_mod(a,b) sector_div(a,b)
#endif

#endif
