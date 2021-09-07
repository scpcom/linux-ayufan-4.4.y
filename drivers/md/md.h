#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#ifndef _MD_MD_H
#define _MD_MD_H

#include <linux/blkdev.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#ifdef MY_ABC_HERE
#include <linux/raid/libmd-report.h>
#endif

#define MaxSector (~(sector_t)0)

#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
struct raidset_s; 
#endif

typedef struct mddev_s mddev_t;
typedef struct mdk_rdev_s mdk_rdev_t;

#ifdef MY_ABC_HERE
typedef struct _tag_SYNO_UPDATE_SB_WORK{
    struct work_struct work;
    mddev_t *mddev;
}SYNO_UPDATE_SB_WORK;
#endif

#ifdef MY_ABC_HERE
typedef struct _tag_SYNO_WAKEUP_DEVICE_WORK{
    struct work_struct work;
    mddev_t *mddev;
} SYNO_WAKEUP_DEVICE_WORK;
#endif


struct mdk_rdev_s
{
	struct list_head same_set;	

	sector_t sectors;		
	mddev_t *mddev;			
	int last_events;		

	struct block_device *bdev;	

	struct page	*sb_page;
#ifdef MY_ABC_HERE
	struct page	*wakeup_page;
#endif
	int		sb_loaded;
	__u64		sb_events;
	sector_t	data_offset;	
	sector_t 	sb_start;	
	int		sb_size;	
	int		preferred_minor;	

	struct kobject	kobj;

	

	unsigned long	flags;
#define	Faulty		1		
#define	In_sync		2		
#define	WriteMostly	4		
#define	BarriersNotsupp	5		
#define	AllReserved	6		
#define	AutoDetected	7		
#define Blocked		8		
#define StateChanged	9		
#ifdef MY_ABC_HERE
#define DiskError	10		
#endif 

	wait_queue_head_t blocked_wait;

	int desc_nr;			
	int raid_disk;			
	int saved_raid_disk;		
	sector_t	recovery_offset;

	atomic_t	nr_pending;	
	atomic_t	read_errors;	
	atomic_t	corrected_errors; 
	struct work_struct del_work;	

	struct sysfs_dirent *sysfs_state; 
};

struct mddev_s
{
	void				*private;
	struct mdk_personality		*pers;
	dev_t				unit;
	int				md_minor;
	struct list_head 		disks;
	unsigned long			flags;
#define MD_CHANGE_DEVS	0	
#define MD_CHANGE_CLEAN 1	
#define MD_CHANGE_PENDING 2	

	int				suspended;
	atomic_t			active_io;
	int				ro;

	struct gendisk			*gendisk;

	struct kobject			kobj;
	int				hold_active;
#define	UNTIL_IOCTL	1
#define	UNTIL_STOP	2

	
	int				major_version,
					minor_version,
					patch_version;
	int				persistent;
	int 				external;	
	char				metadata_type[17]; 
	int				chunk_sectors;
	time_t				ctime, utime;
	int				level, layout;
	char				clevel[16];
	int				raid_disks;
	int				max_disks;
	sector_t			dev_sectors; 	
	sector_t			array_sectors; 
	int				external_size; 
	__u64				events;

	char				uuid[16];

	
	sector_t			reshape_position;
	int				delta_disks, new_level, new_layout;
	int				new_chunk_sectors;

	struct mdk_thread_s		*thread;	
	struct mdk_thread_s		*sync_thread;	
	sector_t			curr_resync;	
	
	sector_t			curr_resync_completed;
	unsigned long			resync_mark;	
	sector_t			resync_mark_cnt;
	sector_t			curr_mark_cnt; 

	sector_t			resync_max_sectors; 

	sector_t			resync_mismatches; 

	
	sector_t			suspend_lo;
	sector_t			suspend_hi;
	
	int				sync_speed_min;
	int				sync_speed_max;

	
	int				parallel_resync;

	int				ok_start_degraded;
	
#define	MD_RECOVERY_RUNNING	0
#define	MD_RECOVERY_SYNC	1
#define	MD_RECOVERY_RECOVER	2
#define	MD_RECOVERY_INTR	3
#define	MD_RECOVERY_DONE	4
#define	MD_RECOVERY_NEEDED	5
#define	MD_RECOVERY_REQUESTED	6
#define	MD_RECOVERY_CHECK	7
#define MD_RECOVERY_RESHAPE	8
#define	MD_RECOVERY_FROZEN	9

	unsigned long			recovery;
	int				recovery_disabled; 

	int				in_sync;	
	
	struct mutex			open_mutex;
	struct mutex			reconfig_mutex;
	atomic_t			active;		
	atomic_t			openers;	

	int				changed;	
	int				degraded;	
	int				barriers_work;	
	struct bio			*biolist; 	

	atomic_t			recovery_active; 
	wait_queue_head_t		recovery_wait;
	sector_t			recovery_cp;
	sector_t			resync_min;	
	sector_t			resync_max;	

	struct sysfs_dirent		*sysfs_state;	
	struct sysfs_dirent		*sysfs_action;  

	struct work_struct del_work;	

	spinlock_t			write_lock;
	wait_queue_head_t		sb_wait;	
	atomic_t			pending_writes;	

	unsigned int			safemode;	 
	unsigned int			safemode_delay;
	struct timer_list		safemode_timer;
	atomic_t			writes_pending; 
	struct request_queue		*queue;	

	atomic_t                        write_behind; 
	unsigned int                    max_write_behind; 

	struct bitmap                   *bitmap; 
	struct file			*bitmap_file; 
	long				bitmap_offset; 
	long				default_bitmap_offset; 
	struct mutex			bitmap_mutex;

	struct list_head		all_mddevs;
#ifdef MY_ABC_HERE
	unsigned char			blActive;  
	spinlock_t				ActLock;   
	unsigned long			ulLastReq; 
#endif
#ifdef MY_ABC_HERE
    unsigned char           nodev_and_crashed;     
#endif
#ifdef MY_ABC_HERE
	unsigned char			auto_remap;     
#endif
#ifdef MY_ABC_HERE
	unsigned char			force_auto_remap; 
	void				*syno_private;	  
	char				lv_name[16];
#endif
#ifdef MY_ABC_HERE
	mempool_t			*syno_mdio_mempool;
#endif
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
 	struct raidset_s* hw_raid;
#endif
};

static inline void rdev_dec_pending(mdk_rdev_t *rdev, mddev_t *mddev)
{
	int faulty = test_bit(Faulty, &rdev->flags);
	if (atomic_dec_and_test(&rdev->nr_pending) && faulty)
		set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
}

static inline void md_sync_acct(struct block_device *bdev, unsigned long nr_sectors)
{
        atomic_add(nr_sectors, &bdev->bd_contains->bd_disk->sync_io);
}

struct mdk_personality
{
	char *name;
	int level;
	struct list_head list;
	struct module *owner;
	int (*make_request)(struct request_queue *q, struct bio *bio);
	int (*run)(mddev_t *mddev);
	int (*stop)(mddev_t *mddev);
	void (*status)(struct seq_file *seq, mddev_t *mddev);
	
	void (*error_handler)(mddev_t *mddev, mdk_rdev_t *rdev);
#ifdef MY_ABC_HERE
	
	void (*syno_error_handler)(mddev_t *mddev, mdk_rdev_t *rdev);
#endif 
	int (*hot_add_disk) (mddev_t *mddev, mdk_rdev_t *rdev);
	int (*hot_remove_disk) (mddev_t *mddev, int number);
	int (*spare_active) (mddev_t *mddev);
	sector_t (*sync_request)(mddev_t *mddev, sector_t sector_nr, int *skipped, int go_faster);
	int (*resize) (mddev_t *mddev, sector_t sectors);
	sector_t (*size) (mddev_t *mddev, sector_t sectors, int raid_disks);
	int (*check_reshape) (mddev_t *mddev);
	int (*start_reshape) (mddev_t *mddev);
	void (*finish_reshape) (mddev_t *mddev);
	
	void (*quiesce) (mddev_t *mddev, int state);
	
	void *(*takeover) (mddev_t *mddev);
#ifdef MY_ABC_HERE
	unsigned char (*ismaxdegrade) (mddev_t *mddev);
#endif
};

struct md_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(mddev_t *, char *);
	ssize_t (*store)(mddev_t *, const char *, size_t);
};

static inline char * mdname (mddev_t * mddev)
{
	return mddev->gendisk ? mddev->gendisk->disk_name : "mdX";
}


#define rdev_for_each_list(rdev, tmp, head)				\
	list_for_each_entry_safe(rdev, tmp, head, same_set)


#define rdev_for_each(rdev, tmp, mddev)				\
	list_for_each_entry_safe(rdev, tmp, &((mddev)->disks), same_set)

#define rdev_for_each_rcu(rdev, mddev)				\
	list_for_each_entry_rcu(rdev, &((mddev)->disks), same_set)

typedef struct mdk_thread_s {
	void			(*run) (mddev_t *mddev);
	mddev_t			*mddev;
	wait_queue_head_t	wqueue;
	unsigned long           flags;
	struct task_struct	*tsk;
	unsigned long		timeout;
} mdk_thread_t;

#define THREAD_WAKEUP  0

#define __wait_event_lock_irq(wq, condition, lock, cmd) 		\
do {									\
	wait_queue_t __wait;						\
	init_waitqueue_entry(&__wait, current);				\
									\
	add_wait_queue(&wq, &__wait);					\
	for (;;) {							\
		set_current_state(TASK_UNINTERRUPTIBLE);		\
		if (condition)						\
			break;						\
		spin_unlock_irq(&lock);					\
		cmd;							\
		schedule();						\
		spin_lock_irq(&lock);					\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)

#define wait_event_lock_irq(wq, condition, lock, cmd) 			\
do {									\
	if (condition)	 						\
		break;							\
	__wait_event_lock_irq(wq, condition, lock, cmd);		\
} while (0)

static inline void safe_put_page(struct page *p)
{
	if (p) put_page(p);
}

extern int register_md_personality(struct mdk_personality *p);
extern int unregister_md_personality(struct mdk_personality *p);
extern mdk_thread_t * md_register_thread(void (*run) (mddev_t *mddev),
				mddev_t *mddev, const char *name);
extern void md_unregister_thread(mdk_thread_t *thread);
extern void md_wakeup_thread(mdk_thread_t *thread);
extern void md_check_recovery(mddev_t *mddev);
extern void md_write_start(mddev_t *mddev, struct bio *bi);
extern void md_write_end(mddev_t *mddev);
extern void md_done_sync(mddev_t *mddev, int blocks, int ok);
extern void md_error(mddev_t *mddev, mdk_rdev_t *rdev);
#ifdef MY_ABC_HERE
extern void syno_md_error (mddev_t *mddev, mdk_rdev_t *rdev);
extern int IsDeviceDisappear(struct block_device *bdev);
#endif
#ifdef MY_ABC_HERE
extern void SynoUpdateSBTask(struct work_struct *work);
#endif

extern int mddev_congested(mddev_t *mddev, int bits);
extern void md_super_write(mddev_t *mddev, mdk_rdev_t *rdev,
			   sector_t sector, int size, struct page *page);
extern void md_super_wait(mddev_t *mddev);
extern int sync_page_io(struct block_device *bdev, sector_t sector, int size,
			struct page *page, int rw);
extern void md_do_sync(mddev_t *mddev);
extern void md_new_event(mddev_t *mddev);
extern int md_allow_write(mddev_t *mddev);
extern void md_wait_for_blocked_rdev(mdk_rdev_t *rdev, mddev_t *mddev);
extern void md_set_array_sectors(mddev_t *mddev, sector_t array_sectors);
extern int md_check_no_bitmap(mddev_t *mddev);
extern int md_integrity_register(mddev_t *mddev);
void md_integrity_add_rdev(mdk_rdev_t *rdev, mddev_t *mddev);

#ifdef MY_ABC_HERE
void SynoAutoRemapReport(mddev_t *mddev, sector_t sector, struct block_device *bdev);
#endif

#ifdef MY_ABC_HERE
void SYNORaidRdevUnplug(mddev_t *mddev, mdk_rdev_t *rdev);
#endif

#ifdef MY_ABC_HERE
void RaidRemapModeSet(struct block_device *, unsigned char);

static inline void
RaidMemberAutoRemapSet(mddev_t *mddev)
{
	mdk_rdev_t *rdev, *tmp;
	char b[BDEVNAME_SIZE];

	
	rdev_for_each(rdev, tmp, mddev) {
		bdevname(rdev->bdev,b);
		RaidRemapModeSet(rdev->bdev, mddev->auto_remap);
		printk("md: %s: set %s to auto_remap [%d]\n", mdname(mddev), b, mddev->auto_remap);
	}
}
#endif

#endif 
