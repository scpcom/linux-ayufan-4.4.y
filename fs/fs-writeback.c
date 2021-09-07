#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/writeback.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/buffer_head.h>
#include "internal.h"

#define inode_to_bdi(inode)	((inode)->i_mapping->backing_dev_info)

int nr_pdflush_threads;

struct wb_writeback_args {
	long nr_pages;
	struct super_block *sb;
	enum writeback_sync_modes sync_mode;
	int for_kupdate:1;
	int range_cyclic:1;
	int for_background:1;
};

struct bdi_work {
	struct list_head list;		 
	struct rcu_head rcu_head;	 

	unsigned long seen;		 
	atomic_t pending;		 

	struct wb_writeback_args args;	 

	unsigned long state;		 
};

enum {
	WS_USED_B = 0,
	WS_ONSTACK_B,
};

#define WS_USED (1 << WS_USED_B)
#define WS_ONSTACK (1 << WS_ONSTACK_B)

static inline bool bdi_work_on_stack(struct bdi_work *work)
{
	return test_bit(WS_ONSTACK_B, &work->state);
}

static inline void bdi_work_init(struct bdi_work *work,
				 struct wb_writeback_args *args)
{
	INIT_RCU_HEAD(&work->rcu_head);
	work->args = *args;
	work->state = WS_USED;
}

int writeback_in_progress(struct backing_dev_info *bdi)
{
	return !list_empty(&bdi->work_list);
}

static void bdi_work_clear(struct bdi_work *work)
{
	clear_bit(WS_USED_B, &work->state);
	smp_mb__after_clear_bit();
	 
	wake_up_bit(&work->state, WS_USED_B);
}

static void bdi_work_free(struct rcu_head *head)
{
	struct bdi_work *work = container_of(head, struct bdi_work, rcu_head);

	if (!bdi_work_on_stack(work))
		kfree(work);
	else
		bdi_work_clear(work);
}

static void wb_work_complete(struct bdi_work *work)
{
	const enum writeback_sync_modes sync_mode = work->args.sync_mode;
	int onstack = bdi_work_on_stack(work);

	if (!onstack)
		bdi_work_clear(work);
	if (sync_mode == WB_SYNC_NONE || onstack)
		call_rcu(&work->rcu_head, bdi_work_free);
}

static void wb_clear_pending(struct bdi_writeback *wb, struct bdi_work *work)
{
	 
	if (atomic_dec_and_test(&work->pending)) {
		struct backing_dev_info *bdi = wb->bdi;

		spin_lock(&bdi->wb_lock);
		list_del_rcu(&work->list);
		spin_unlock(&bdi->wb_lock);

		wb_work_complete(work);
	}
}

static void bdi_queue_work(struct backing_dev_info *bdi, struct bdi_work *work)
{
	work->seen = bdi->wb_mask;
	BUG_ON(!work->seen);
	atomic_set(&work->pending, bdi->wb_cnt);
	BUG_ON(!bdi->wb_cnt);

	spin_lock(&bdi->wb_lock);
	list_add_tail_rcu(&work->list, &bdi->work_list);
	spin_unlock(&bdi->wb_lock);

	if (unlikely(list_empty_careful(&bdi->wb_list)))
		wake_up_process(default_backing_dev_info.wb.task);
	else {
		struct bdi_writeback *wb = &bdi->wb;

		if (wb->task)
			wake_up_process(wb->task);
	}
}

static void bdi_wait_on_work_clear(struct bdi_work *work)
{
	wait_on_bit(&work->state, WS_USED_B, bdi_sched_wait,
		    TASK_UNINTERRUPTIBLE);
}

static void bdi_alloc_queue_work(struct backing_dev_info *bdi,
				 struct wb_writeback_args *args)
{
	struct bdi_work *work;

	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (work) {
		bdi_work_init(work, args);
		bdi_queue_work(bdi, work);
	} else {
		struct bdi_writeback *wb = &bdi->wb;

		if (wb->task)
			wake_up_process(wb->task);
	}
}

static void bdi_sync_writeback(struct backing_dev_info *bdi,
			       struct super_block *sb)
{
	struct wb_writeback_args args = {
		.sb		= sb,
		.sync_mode	= WB_SYNC_ALL,
		.nr_pages	= LONG_MAX,
		.range_cyclic	= 0,
	};
	struct bdi_work work;

	bdi_work_init(&work, &args);
	work.state |= WS_ONSTACK;

	bdi_queue_work(bdi, &work);
	bdi_wait_on_work_clear(&work);
}

void bdi_start_writeback(struct backing_dev_info *bdi, struct super_block *sb,
			 long nr_pages)
{
	struct wb_writeback_args args = {
		.sb		= sb,
		.sync_mode	= WB_SYNC_NONE,
		.nr_pages	= nr_pages,
		.range_cyclic	= 1,
	};

	if (!nr_pages) {
		args.nr_pages = LONG_MAX;
		args.for_background = 1;
	}

	bdi_alloc_queue_work(bdi, &args);
}

static void redirty_tail(struct inode *inode)
{
	struct bdi_writeback *wb = &inode_to_bdi(inode)->wb;

	if (!list_empty(&wb->b_dirty)) {
		struct inode *tail;

		tail = list_entry(wb->b_dirty.next, struct inode, i_list);
		if (time_before(inode->dirtied_when, tail->dirtied_when))
			inode->dirtied_when = jiffies;
	}
	list_move(&inode->i_list, &wb->b_dirty);
}

static void requeue_io(struct inode *inode)
{
	struct bdi_writeback *wb = &inode_to_bdi(inode)->wb;

	list_move(&inode->i_list, &wb->b_more_io);
}

static void inode_sync_complete(struct inode *inode)
{
	 
	smp_mb();
	wake_up_bit(&inode->i_state, __I_SYNC);
}

static bool inode_dirtied_after(struct inode *inode, unsigned long t)
{
	bool ret = time_after(inode->dirtied_when, t);
#ifndef CONFIG_64BIT
	 
	ret = ret && time_before_eq(inode->dirtied_when, jiffies);
#endif
	return ret;
}

static void move_expired_inodes(struct list_head *delaying_queue,
			       struct list_head *dispatch_queue,
				unsigned long *older_than_this)
{
	LIST_HEAD(tmp);
	struct list_head *pos, *node;
	struct super_block *sb = NULL;
	struct inode *inode;
	int do_sb_sort = 0;

	while (!list_empty(delaying_queue)) {
		inode = list_entry(delaying_queue->prev, struct inode, i_list);
		if (older_than_this &&
		    inode_dirtied_after(inode, *older_than_this))
			break;
		if (sb && sb != inode->i_sb)
			do_sb_sort = 1;
		sb = inode->i_sb;
		list_move(&inode->i_list, &tmp);
	}

	if (!do_sb_sort) {
		list_splice(&tmp, dispatch_queue);
		return;
	}

	while (!list_empty(&tmp)) {
		inode = list_entry(tmp.prev, struct inode, i_list);
		sb = inode->i_sb;
		list_for_each_prev_safe(pos, node, &tmp) {
			inode = list_entry(pos, struct inode, i_list);
			if (inode->i_sb == sb)
				list_move(&inode->i_list, dispatch_queue);
		}
	}
}

static void queue_io(struct bdi_writeback *wb, unsigned long *older_than_this)
{
	list_splice_init(&wb->b_more_io, wb->b_io.prev);
	move_expired_inodes(&wb->b_dirty, &wb->b_io, older_than_this);
}

static int write_inode(struct inode *inode, int sync)
{
	if (inode->i_sb->s_op->write_inode && !is_bad_inode(inode))
		return inode->i_sb->s_op->write_inode(inode, sync);
	return 0;
}

static void inode_wait_for_writeback(struct inode *inode)
{
	DEFINE_WAIT_BIT(wq, &inode->i_state, __I_SYNC);
	wait_queue_head_t *wqh;

	wqh = bit_waitqueue(&inode->i_state, __I_SYNC);
	do {
		spin_unlock(&inode_lock);
		__wait_on_bit(wqh, &wq, inode_wait, TASK_UNINTERRUPTIBLE);
		spin_lock(&inode_lock);
	} while (inode->i_state & I_SYNC);
}

static int
writeback_single_inode(struct inode *inode, struct writeback_control *wbc)
{
	struct address_space *mapping = inode->i_mapping;
	int wait = wbc->sync_mode == WB_SYNC_ALL;
	unsigned dirty;
	int ret;

	if (!atomic_read(&inode->i_count))
		WARN_ON(!(inode->i_state & (I_WILL_FREE|I_FREEING)));
	else
		WARN_ON(inode->i_state & I_WILL_FREE);

	if (inode->i_state & I_SYNC) {
		 
		if (!wait) {
			requeue_io(inode);
			return 0;
		}

		inode_wait_for_writeback(inode);
	}

	BUG_ON(inode->i_state & I_SYNC);

	dirty = inode->i_state & I_DIRTY;
	inode->i_state |= I_SYNC;
	inode->i_state &= ~I_DIRTY;

	spin_unlock(&inode_lock);

	ret = do_writepages(mapping, wbc);

	if (dirty & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		int err = write_inode(inode, wait);
		if (ret == 0)
			ret = err;
	}

	if (wait) {
		int err = filemap_fdatawait(mapping);
		if (ret == 0)
			ret = err;
	}

	spin_lock(&inode_lock);
	inode->i_state &= ~I_SYNC;
	if (!(inode->i_state & (I_FREEING | I_CLEAR))) {
		if ((inode->i_state & I_DIRTY_PAGES) && wbc->for_kupdate) {
			 
			goto select_queue;
		} else if (inode->i_state & I_DIRTY) {
			 
			redirty_tail(inode);
		} else if (mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)) {
			 
			if (wbc->for_kupdate) {
				 
				inode->i_state |= I_DIRTY_PAGES;
select_queue:
				if (wbc->nr_to_write <= 0) {
					 
					requeue_io(inode);
				} else {
					 
					redirty_tail(inode);
				}
			} else {
				 
				inode->i_state |= I_DIRTY_PAGES;
				redirty_tail(inode);
			}
		} else if (atomic_read(&inode->i_count)) {
			 
			list_move(&inode->i_list, &inode_in_use);
		} else {
			 
			list_move(&inode->i_list, &inode_unused);
		}
	}
	inode_sync_complete(inode);
	return ret;
}

static void unpin_sb_for_writeback(struct super_block **psb)
{
	struct super_block *sb = *psb;

	if (sb) {
		up_read(&sb->s_umount);
		put_super(sb);
		*psb = NULL;
	}
}

static int pin_sb_for_writeback(struct writeback_control *wbc,
				struct inode *inode, struct super_block **psb)
{
	struct super_block *sb = inode->i_sb;

	if (sb == *psb)
		return 0;
	else if (*psb)
		unpin_sb_for_writeback(psb);

	if (wbc->sync_mode == WB_SYNC_ALL) {
		WARN_ON(!rwsem_is_locked(&sb->s_umount));
		return 0;
	}

	spin_lock(&sb_lock);
	sb->s_count++;
	if (down_read_trylock(&sb->s_umount)) {
		if (sb->s_root) {
			spin_unlock(&sb_lock);
			goto pinned;
		}
		 
		up_read(&sb->s_umount);
	}

	sb->s_count--;
	spin_unlock(&sb_lock);
	return 1;
pinned:
	*psb = sb;
	return 0;
}

static void writeback_inodes_wb(struct bdi_writeback *wb,
				struct writeback_control *wbc)
{
	struct super_block *sb = wbc->sb, *pin_sb = NULL;
	const int is_blkdev_sb = sb_is_blkdev_sb(sb);
	const unsigned long start = jiffies;	 
#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
	unsigned int find_same_bdi = 0;
#endif
#endif

	spin_lock(&inode_lock);

	if (!wbc->for_kupdate || list_empty(&wb->b_io))
		queue_io(wb, wbc->older_than_this);

	while (!list_empty(&wb->b_io)) {
		struct inode *inode = list_entry(wb->b_io.prev,
						struct inode, i_list);
#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
		struct inode *inode_tmp;
#endif
#endif
		long pages_skipped;

		if (sb && sb != inode->i_sb) {
			redirty_tail(inode);
			continue;
		}

		if (!bdi_cap_writeback_dirty(wb->bdi)) {
			redirty_tail(inode);
			if (is_blkdev_sb) {
				 
				continue;
			}
			 
			break;
		}

		if (inode->i_state & (I_NEW | I_WILL_FREE)) {
			requeue_io(inode);
			continue;
		}

		if (wbc->nonblocking && bdi_write_congested(wb->bdi)) {
			wbc->encountered_congestion = 1;
			if (!is_blkdev_sb)
				break;		 
			requeue_io(inode);
			continue;		 
		}

		if (inode_dirtied_after(inode, start))
			break;

		if (pin_sb_for_writeback(wbc, inode, &pin_sb)) {
			requeue_io(inode);
			continue;
		}

#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
		 
		if(wbc->nonblocking && imajor(inode)) {
			requeue_io(inode);
			continue;
		}

		if(imajor(inode))
			list_for_each_entry(inode_tmp, &wb->b_io, i_list)
				if((imajor(inode_tmp)==0) && (inode_tmp->i_data.backing_dev_info==inode->i_data.backing_dev_info)) {
					inode = inode_tmp;
					find_same_bdi = 1;
					break;
				}

		if(imajor(inode) && !find_same_bdi) {
			list_for_each_entry(inode_tmp, &wb->b_dirty, i_list)
				if((imajor(inode_tmp)==0) && (inode_tmp->i_data.backing_dev_info==inode->i_data.backing_dev_info)) {
					requeue_io(inode);
					continue;
				}
			list_for_each_entry(inode_tmp, &wb->b_more_io, i_list)
				if((imajor(inode_tmp)==0) && (inode_tmp->i_data.backing_dev_info==inode->i_data.backing_dev_info)) {
					requeue_io(inode);
					continue;
			}
		}
#endif
#endif
		BUG_ON(inode->i_state & (I_FREEING | I_CLEAR));
		__iget(inode);
		pages_skipped = wbc->pages_skipped;
		writeback_single_inode(inode, wbc);
#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
		find_same_bdi = 0;
#endif
#endif
		if (wbc->pages_skipped != pages_skipped) {
			 
			redirty_tail(inode);
		}
		spin_unlock(&inode_lock);
		iput(inode);
		cond_resched();
		spin_lock(&inode_lock);
		if (wbc->nr_to_write <= 0) {
			wbc->more_io = 1;
			break;
		}
		if (!list_empty(&wb->b_more_io))
			wbc->more_io = 1;
	}

	unpin_sb_for_writeback(&pin_sb);

	spin_unlock(&inode_lock);
	 
}

void writeback_inodes_wbc(struct writeback_control *wbc)
{
	struct backing_dev_info *bdi = wbc->bdi;

	writeback_inodes_wb(&bdi->wb, wbc);
}

#define MAX_WRITEBACK_PAGES     1024

static inline bool over_bground_thresh(void)
{
	unsigned long background_thresh, dirty_thresh;

	get_dirty_limits(&background_thresh, &dirty_thresh, NULL, NULL);

	return (global_page_state(NR_FILE_DIRTY) +
		global_page_state(NR_UNSTABLE_NFS) >= background_thresh);
}

static long wb_writeback(struct bdi_writeback *wb,
			 struct wb_writeback_args *args)
{
	struct writeback_control wbc = {
		.bdi			= wb->bdi,
		.sb			= args->sb,
		.sync_mode		= args->sync_mode,
		.older_than_this	= NULL,
		.for_kupdate		= args->for_kupdate,
		.range_cyclic		= args->range_cyclic,
#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
		.nonblocking		= 1,
#else
		.nonblocking		= 0,
#endif
#endif
	};
	unsigned long oldest_jif;
	long wrote = 0;
	struct inode *inode;

	if (wbc.for_kupdate) {
		wbc.older_than_this = &oldest_jif;
		oldest_jif = jiffies -
				msecs_to_jiffies(dirty_expire_interval * 10);
	}
	if (!wbc.range_cyclic) {
		wbc.range_start = 0;
		wbc.range_end = LLONG_MAX;
	}

	for (;;) {
#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
		unsigned long background_thresh;
		unsigned long dirty_thresh;
#endif
#endif
		 
		if (args->nr_pages <= 0)
			break;

		if (args->for_background && !over_bground_thresh())
			break;

#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_SD_PERFORMANCE
		 
		get_dirty_limits(&background_thresh, &dirty_thresh, NULL, NULL);
		if (global_page_state(NR_FILE_DIRTY) +
				global_page_state(NR_UNSTABLE_NFS) < background_thresh)
			wbc.nonblocking = 0;
		else
			wbc.nonblocking = 1;
#endif
#endif
		wbc.more_io = 0;
		wbc.encountered_congestion = 0;
		wbc.nr_to_write = MAX_WRITEBACK_PAGES;
		wbc.pages_skipped = 0;
		writeback_inodes_wb(wb, &wbc);
		args->nr_pages -= MAX_WRITEBACK_PAGES - wbc.nr_to_write;
		wrote += MAX_WRITEBACK_PAGES - wbc.nr_to_write;

		if (wbc.nr_to_write <= 0)
			continue;
		 
		if (!wbc.more_io)
			break;
		 
		if (wbc.nr_to_write < MAX_WRITEBACK_PAGES)
			continue;
		 
		spin_lock(&inode_lock);
		if (!list_empty(&wb->b_more_io))  {
			inode = list_entry(wb->b_more_io.prev,
						struct inode, i_list);
			inode_wait_for_writeback(inode);
		}
		spin_unlock(&inode_lock);
	}

	return wrote;
}

static struct bdi_work *get_next_work_item(struct backing_dev_info *bdi,
					   struct bdi_writeback *wb)
{
	struct bdi_work *work, *ret = NULL;

	rcu_read_lock();

	list_for_each_entry_rcu(work, &bdi->work_list, list) {
		if (!test_bit(wb->nr, &work->seen))
			continue;
		clear_bit(wb->nr, &work->seen);

		ret = work;
		break;
	}

	rcu_read_unlock();
	return ret;
}

static long wb_check_old_data_flush(struct bdi_writeback *wb)
{
	unsigned long expired;
	long nr_pages;

	expired = wb->last_old_flush +
			msecs_to_jiffies(dirty_writeback_interval * 10);
	if (time_before(jiffies, expired))
		return 0;

	wb->last_old_flush = jiffies;
	nr_pages = global_page_state(NR_FILE_DIRTY) +
			global_page_state(NR_UNSTABLE_NFS) +
			(inodes_stat.nr_inodes - inodes_stat.nr_unused);

	if (nr_pages) {
		struct wb_writeback_args args = {
			.nr_pages	= nr_pages,
			.sync_mode	= WB_SYNC_NONE,
			.for_kupdate	= 1,
			.range_cyclic	= 1,
		};

		return wb_writeback(wb, &args);
	}

	return 0;
}

long wb_do_writeback(struct bdi_writeback *wb, int force_wait)
{
	struct backing_dev_info *bdi = wb->bdi;
	struct bdi_work *work;
	long wrote = 0;

	while ((work = get_next_work_item(bdi, wb)) != NULL) {
		struct wb_writeback_args args = work->args;

		if (force_wait)
			work->args.sync_mode = args.sync_mode = WB_SYNC_ALL;

		if (args.sync_mode == WB_SYNC_NONE)
			wb_clear_pending(wb, work);

		wrote += wb_writeback(wb, &args);

		if (args.sync_mode == WB_SYNC_ALL)
			wb_clear_pending(wb, work);
	}

	wrote += wb_check_old_data_flush(wb);

	return wrote;
}

int bdi_writeback_task(struct bdi_writeback *wb)
{
	unsigned long last_active = jiffies;
	unsigned long wait_jiffies = -1UL;
	long pages_written;

	while (!kthread_should_stop()) {
		pages_written = wb_do_writeback(wb, 0);

		if (pages_written)
			last_active = jiffies;
		else if (wait_jiffies != -1UL) {
			unsigned long max_idle;

			max_idle = max(5UL * 60 * HZ, wait_jiffies);
			if (time_after(jiffies, max_idle + last_active))
				break;
		}

		wait_jiffies = msecs_to_jiffies(dirty_writeback_interval * 10);
		schedule_timeout_interruptible(wait_jiffies);
		try_to_freeze();
	}

	return 0;
}

static void bdi_writeback_all(struct super_block *sb, long nr_pages)
{
	struct wb_writeback_args args = {
		.sb		= sb,
		.nr_pages	= nr_pages,
		.sync_mode	= WB_SYNC_NONE,
	};
	struct backing_dev_info *bdi;

	rcu_read_lock();

	list_for_each_entry_rcu(bdi, &bdi_list, bdi_list) {
		if (!bdi_has_dirty_io(bdi))
			continue;

		bdi_alloc_queue_work(bdi, &args);
	}

	rcu_read_unlock();
}

void wakeup_flusher_threads(long nr_pages)
{
	if (nr_pages == 0)
		nr_pages = global_page_state(NR_FILE_DIRTY) +
				global_page_state(NR_UNSTABLE_NFS);
	bdi_writeback_all(NULL, nr_pages);
}

static noinline void block_dump___mark_inode_dirty(struct inode *inode)
{
	if (inode->i_ino || strcmp(inode->i_sb->s_id, "bdev")) {
		struct dentry *dentry;
		const char *name = "?";

		dentry = d_find_alias(inode);
		if (dentry) {
			spin_lock(&dentry->d_lock);
			name = (const char *) dentry->d_name.name;
		}
		printk(KERN_DEBUG
		       "%s(%d): dirtied inode %lu (%s) on %s\n",
		       current->comm, task_pid_nr(current), inode->i_ino,
		       name, inode->i_sb->s_id);
		if (dentry) {
			spin_unlock(&dentry->d_lock);
			dput(dentry);
		}
	}
}

void __mark_inode_dirty(struct inode *inode, int flags)
{
	struct super_block *sb = inode->i_sb;

	if (flags & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		if (sb->s_op->dirty_inode)
			sb->s_op->dirty_inode(inode);
	}

	smp_mb();

	if ((inode->i_state & flags) == flags)
		return;

	if (unlikely(block_dump))
		block_dump___mark_inode_dirty(inode);

	spin_lock(&inode_lock);
	if ((inode->i_state & flags) != flags) {
		const int was_dirty = inode->i_state & I_DIRTY;

		inode->i_state |= flags;

		if (inode->i_state & I_SYNC)
			goto out;

		if (!S_ISBLK(inode->i_mode)) {
			if (hlist_unhashed(&inode->i_hash))
				goto out;
		}
		if (inode->i_state & (I_FREEING|I_CLEAR))
			goto out;

		if (!was_dirty) {
			struct bdi_writeback *wb = &inode_to_bdi(inode)->wb;
			struct backing_dev_info *bdi = wb->bdi;

			if (bdi_cap_writeback_dirty(bdi) &&
			    !test_bit(BDI_registered, &bdi->state)) {
				WARN_ON(1);
				printk(KERN_ERR "bdi-%s not registered\n",
								bdi->name);
			}

			inode->dirtied_when = jiffies;
			list_move(&inode->i_list, &wb->b_dirty);
		}
	}
out:
	spin_unlock(&inode_lock);
}
EXPORT_SYMBOL(__mark_inode_dirty);

static void wait_sb_inodes(struct super_block *sb)
{
	struct inode *inode, *old_inode = NULL;

	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	spin_lock(&inode_lock);

	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
		struct address_space *mapping;

		if (inode->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE|I_NEW))
			continue;
		mapping = inode->i_mapping;
		if (mapping->nrpages == 0)
			continue;
		__iget(inode);
		spin_unlock(&inode_lock);
		 
		iput(old_inode);
		old_inode = inode;

		filemap_fdatawait(mapping);

		cond_resched();

		spin_lock(&inode_lock);
	}
	spin_unlock(&inode_lock);
	iput(old_inode);
}

void writeback_inodes_sb(struct super_block *sb)
{
	unsigned long nr_dirty = global_page_state(NR_FILE_DIRTY);
	unsigned long nr_unstable = global_page_state(NR_UNSTABLE_NFS);
	long nr_to_write;

	nr_to_write = nr_dirty + nr_unstable +
			(inodes_stat.nr_inodes - inodes_stat.nr_unused);

	bdi_start_writeback(sb->s_bdi, sb, nr_to_write);
}
EXPORT_SYMBOL(writeback_inodes_sb);

int writeback_inodes_sb_if_idle(struct super_block *sb)
{
	if (!writeback_in_progress(sb->s_bdi)) {
		writeback_inodes_sb(sb);
		return 1;
	} else
		return 0;
}
EXPORT_SYMBOL(writeback_inodes_sb_if_idle);

void sync_inodes_sb(struct super_block *sb)
{
	bdi_sync_writeback(sb->s_bdi, sb);
	wait_sb_inodes(sb);
}
EXPORT_SYMBOL(sync_inodes_sb);

int write_inode_now(struct inode *inode, int sync)
{
	int ret;
	struct writeback_control wbc = {
		.nr_to_write = LONG_MAX,
		.sync_mode = sync ? WB_SYNC_ALL : WB_SYNC_NONE,
		.range_start = 0,
		.range_end = LLONG_MAX,
	};

	if (!mapping_cap_writeback_dirty(inode->i_mapping))
		wbc.nr_to_write = 0;

	might_sleep();
	spin_lock(&inode_lock);
	ret = writeback_single_inode(inode, &wbc);
	spin_unlock(&inode_lock);
	if (sync)
		inode_sync_wait(inode);
	return ret;
}
EXPORT_SYMBOL(write_inode_now);

int sync_inode(struct inode *inode, struct writeback_control *wbc)
{
	int ret;

	spin_lock(&inode_lock);
	ret = writeback_single_inode(inode, wbc);
	spin_unlock(&inode_lock);
	return ret;
}
EXPORT_SYMBOL(sync_inode);

#ifdef CONFIG_SYNO_PLX_PORTING
 
int generic_osync_inode(struct inode *inode, struct address_space *mapping, int what)
{
 int err = 0;
 int need_write_inode_now = 0;
 int err2;

 if (what & OSYNC_DATA)
 err = filemap_fdatawrite(mapping);
 if (what & (OSYNC_METADATA|OSYNC_DATA)) {
 err2 = sync_mapping_buffers(mapping);
 if (!err)
 err = err2;
 }
 if (what & OSYNC_DATA) {
 err2 = filemap_fdatawait(mapping);
 if (!err)
 err = err2;
 }

 spin_lock(&inode_lock);
 if ((inode->i_state & I_DIRTY) &&
 ((what & OSYNC_INODE) || (inode->i_state & I_DIRTY_DATASYNC)))
 need_write_inode_now = 1;
 spin_unlock(&inode_lock);

 if (need_write_inode_now) {
 err2 = write_inode_now(inode, 1);
 if (!err)
 err = err2;
 }
 else
 inode_sync_wait(inode);

 return err;
}
EXPORT_SYMBOL(generic_osync_inode);
#endif
