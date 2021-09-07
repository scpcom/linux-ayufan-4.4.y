#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/dcache.h>
#include <linux/init.h>
#include <linux/quotaops.h>
#include <linux/slab.h>
#include <linux/writeback.h>
#include <linux/module.h>
#include <linux/backing-dev.h>
#include <linux/wait.h>
#include <linux/rwsem.h>
#include <linux/hash.h>
#include <linux/swap.h>
#include <linux/security.h>
#include <linux/ima.h>
#include <linux/pagemap.h>
#include <linux/cdev.h>
#include <linux/bootmem.h>
#include <linux/inotify.h>
#include <linux/fsnotify.h>
#include <linux/mount.h>
#include <linux/async.h>
#include <linux/posix_acl.h>

#ifdef CONFIG_FS_SYNO_ACL
#include "synoacl_int.h"
#endif
 
#include <linux/buffer_head.h>

#define I_HASHBITS	i_hash_shift
#define I_HASHMASK	i_hash_mask

static unsigned int i_hash_mask __read_mostly;
static unsigned int i_hash_shift __read_mostly;

LIST_HEAD(inode_in_use);
LIST_HEAD(inode_unused);
static struct hlist_head *inode_hashtable __read_mostly;

DEFINE_SPINLOCK(inode_lock);

static DECLARE_RWSEM(iprune_sem);

struct inodes_stat_t inodes_stat;

static struct kmem_cache *inode_cachep __read_mostly;

static void wake_up_inode(struct inode *inode)
{
	 
	smp_mb();
	wake_up_bit(&inode->i_state, __I_LOCK);
}

int inode_init_always(struct super_block *sb, struct inode *inode)
{
	static const struct address_space_operations empty_aops;
	static const struct inode_operations empty_iops;
	static const struct file_operations empty_fops;
	struct address_space *const mapping = &inode->i_data;

	inode->i_sb = sb;
	inode->i_blkbits = sb->s_blocksize_bits;
	inode->i_flags = 0;
	atomic_set(&inode->i_count, 1);
	inode->i_op = &empty_iops;
	inode->i_fop = &empty_fops;
	inode->i_nlink = 1;
	inode->i_uid = 0;
	inode->i_gid = 0;
	atomic_set(&inode->i_writecount, 0);
#ifdef CONFIG_SYNO_PLX_PORTING
	inode->space_reserve = 0;
#endif

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	spin_lock_init(&inode->fast_lock);
	inode->fast_open_count = 0;
	inode->normal_open_count = 0;
	INIT_LIST_HEAD(&inode->fast_files);
	rwlock_init(&inode->fast_files_lock);
#ifdef CONFIG_SATA_OX820
	inode->filemap_info.direct_access_context = NULL;
#endif  
	inode->filemap_info.map = NULL;
	sema_init(&inode->filemap_info.sem, 1);
	inode->fallback_in_progress = 0;
	init_waitqueue_head(&inode->fallback_wait_queue);
	inode->fast_reads_in_progress_count = 0;
	inode->filemap_update_pending = 0;
	init_waitqueue_head(&inode->filemap_wait_queue);
#ifdef CONFIG_OXNAS_FAST_WRITES
	inode->fast_writes_in_progress_count = 0;
	inode->write_error = 0;
	sema_init(&inode->writer_filemap_info.sem, 1);
	inode->writer_file_context = NULL;
	inode->filemap_locker_uid = (void*)0;
	inode->i_tent_size = 0;
#ifdef CONFIG_SATA_OX820
	inode->writer_filemap_info.direct_access_context = NULL;
#endif  
	inode->writer_filemap_info.map = NULL;
	inode->writer_filemap_dirty = 0;
#endif  
#endif  

	inode->i_size = 0;
	inode->i_blocks = 0;
	inode->i_bytes = 0;
	inode->i_generation = 0;
#ifdef CONFIG_QUOTA
	memset(&inode->i_dquot, 0, sizeof(inode->i_dquot));
#endif
	inode->i_pipe = NULL;
	inode->i_bdev = NULL;
	inode->i_cdev = NULL;
	inode->i_rdev = 0;
	inode->dirtied_when = 0;
	
#ifdef MY_ABC_HERE
	inode->i_archive_version = 0;
#endif
#ifdef MY_ABC_HERE
	inode->i_archive_bit = 0;    
#endif
	if (security_inode_alloc(inode))
		goto out;

	if (ima_inode_alloc(inode))
		goto out_free_security;

	spin_lock_init(&inode->i_lock);
	lockdep_set_class(&inode->i_lock, &sb->s_type->i_lock_key);

	mutex_init(&inode->i_mutex);
	lockdep_set_class(&inode->i_mutex, &sb->s_type->i_mutex_key);
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
	mutex_init(&inode->i_syno_mutex);
	lockdep_set_class(&inode->i_syno_mutex, &sb->s_type->i_syno_mutex_key);
#endif

	init_rwsem(&inode->i_alloc_sem);
	lockdep_set_class(&inode->i_alloc_sem, &sb->s_type->i_alloc_sem_key);

	mapping->a_ops = &empty_aops;
	mapping->host = inode;
	mapping->flags = 0;
	mapping_set_gfp_mask(mapping, GFP_HIGHUSER_MOVABLE);
	mapping->assoc_mapping = NULL;
	mapping->backing_dev_info = &default_backing_dev_info;
	mapping->writeback_index = 0;

	if (sb->s_bdev) {
		struct backing_dev_info *bdi;

		bdi = sb->s_bdev->bd_inode->i_mapping->backing_dev_info;
		mapping->backing_dev_info = bdi;
	}
	inode->i_private = NULL;
	inode->i_mapping = mapping;
#ifdef CONFIG_FS_SYNO_ACL
	inode->i_syno_acl = ACL_NOT_CACHED;
#elif defined(CONFIG_FS_POSIX_ACL)
	inode->i_acl = inode->i_default_acl = ACL_NOT_CACHED;
#endif

#ifdef CONFIG_FSNOTIFY
	inode->i_fsnotify_mask = 0;
#endif

	return 0;

out_free_security:
	security_inode_free(inode);
out:
	return -ENOMEM;
}
EXPORT_SYMBOL(inode_init_always);

static struct inode *alloc_inode(struct super_block *sb)
{
	struct inode *inode;

	if (sb->s_op->alloc_inode)
		inode = sb->s_op->alloc_inode(sb);
	else
		inode = kmem_cache_alloc(inode_cachep, GFP_KERNEL);

	if (!inode)
		return NULL;

	if (unlikely(inode_init_always(sb, inode))) {
		if (inode->i_sb->s_op->destroy_inode)
			inode->i_sb->s_op->destroy_inode(inode);
		else
			kmem_cache_free(inode_cachep, inode);
		return NULL;
	}

	return inode;
}

void __destroy_inode(struct inode *inode)
{
	BUG_ON(inode_has_buffers(inode));
	ima_inode_free(inode);
	security_inode_free(inode);
	fsnotify_inode_delete(inode);
#ifdef CONFIG_FS_SYNO_ACL
	if (inode->i_syno_acl && inode->i_syno_acl != ACL_NOT_CACHED)
		syno_acl_release(inode->i_syno_acl);
#elif defined(CONFIG_FS_POSIX_ACL)
	if (inode->i_acl && inode->i_acl != ACL_NOT_CACHED)
		posix_acl_release(inode->i_acl);
	if (inode->i_default_acl && inode->i_default_acl != ACL_NOT_CACHED)
		posix_acl_release(inode->i_default_acl);
#endif
}
EXPORT_SYMBOL(__destroy_inode);

void destroy_inode(struct inode *inode)
{
	__destroy_inode(inode);
	if (inode->i_sb->s_op->destroy_inode)
		inode->i_sb->s_op->destroy_inode(inode);
	else
		kmem_cache_free(inode_cachep, (inode));
}

void inode_init_once(struct inode *inode)
{
	memset(inode, 0, sizeof(*inode));
	INIT_HLIST_NODE(&inode->i_hash);
	INIT_LIST_HEAD(&inode->i_dentry);
	INIT_LIST_HEAD(&inode->i_devices);
	INIT_RADIX_TREE(&inode->i_data.page_tree, GFP_ATOMIC);
	spin_lock_init(&inode->i_data.tree_lock);
	spin_lock_init(&inode->i_data.i_mmap_lock);
	INIT_LIST_HEAD(&inode->i_data.private_list);
	spin_lock_init(&inode->i_data.private_lock);
	INIT_RAW_PRIO_TREE_ROOT(&inode->i_data.i_mmap);
	INIT_LIST_HEAD(&inode->i_data.i_mmap_nonlinear);
	i_size_ordered_init(inode);
#ifdef CONFIG_INOTIFY
	INIT_LIST_HEAD(&inode->inotify_watches);
	mutex_init(&inode->inotify_mutex);
#endif
#ifdef CONFIG_FSNOTIFY
	INIT_HLIST_HEAD(&inode->i_fsnotify_mark_entries);
#endif
}
EXPORT_SYMBOL(inode_init_once);

static void init_once(void *foo)
{
	struct inode *inode = (struct inode *) foo;

	inode_init_once(inode);
}

void __iget(struct inode *inode)
{
	if (atomic_read(&inode->i_count)) {
		atomic_inc(&inode->i_count);
		return;
	}
	atomic_inc(&inode->i_count);
	if (!(inode->i_state & (I_DIRTY|I_SYNC)))
		list_move(&inode->i_list, &inode_in_use);
	inodes_stat.nr_unused--;
}

void clear_inode(struct inode *inode)
{
	might_sleep();
	invalidate_inode_buffers(inode);

	BUG_ON(inode->i_data.nrpages);
	BUG_ON(!(inode->i_state & I_FREEING));
	BUG_ON(inode->i_state & I_CLEAR);
	inode_sync_wait(inode);
	vfs_dq_drop(inode);
	if (inode->i_sb->s_op->clear_inode)
		inode->i_sb->s_op->clear_inode(inode);
	if (S_ISBLK(inode->i_mode) && inode->i_bdev)
		bd_forget(inode);
	if (S_ISCHR(inode->i_mode) && inode->i_cdev)
		cd_forget(inode);
	inode->i_state = I_CLEAR;
}
EXPORT_SYMBOL(clear_inode);

static void dispose_list(struct list_head *head)
{
	int nr_disposed = 0;

	while (!list_empty(head)) {
		struct inode *inode;

		inode = list_first_entry(head, struct inode, i_list);
		list_del(&inode->i_list);

		if (inode->i_data.nrpages)
			truncate_inode_pages(&inode->i_data, 0);
		clear_inode(inode);

		spin_lock(&inode_lock);
		hlist_del_init(&inode->i_hash);
		list_del_init(&inode->i_sb_list);
		spin_unlock(&inode_lock);

		wake_up_inode(inode);
		destroy_inode(inode);
		nr_disposed++;
	}
	spin_lock(&inode_lock);
	inodes_stat.nr_inodes -= nr_disposed;
	spin_unlock(&inode_lock);
}

static int invalidate_list(struct list_head *head, struct list_head *dispose)
{
	struct list_head *next;
	int busy = 0, count = 0;

	next = head->next;
	for (;;) {
		struct list_head *tmp = next;
		struct inode *inode;

		cond_resched_lock(&inode_lock);

		next = next->next;
		if (tmp == head)
			break;
		inode = list_entry(tmp, struct inode, i_sb_list);
		if (inode->i_state & I_NEW)
			continue;
		invalidate_inode_buffers(inode);
		if (!atomic_read(&inode->i_count)) {
			list_move(&inode->i_list, dispose);
			WARN_ON(inode->i_state & I_NEW);
			inode->i_state |= I_FREEING;
			count++;
			continue;
		}
		busy = 1;
	}
	 
	inodes_stat.nr_unused -= count;
	return busy;
}

int invalidate_inodes(struct super_block *sb)
{
	int busy;
	LIST_HEAD(throw_away);

	down_write(&iprune_sem);
	spin_lock(&inode_lock);
	inotify_unmount_inodes(&sb->s_inodes);
	fsnotify_unmount_inodes(&sb->s_inodes);
	busy = invalidate_list(&sb->s_inodes, &throw_away);
	spin_unlock(&inode_lock);

	dispose_list(&throw_away);
	up_write(&iprune_sem);

	return busy;
}
EXPORT_SYMBOL(invalidate_inodes);

static int can_unuse(struct inode *inode)
{
	if (inode->i_state)
		return 0;
	if (inode_has_buffers(inode))
		return 0;
	if (atomic_read(&inode->i_count))
		return 0;
	if (inode->i_data.nrpages)
		return 0;
	return 1;
}

static void prune_icache(int nr_to_scan)
{
	LIST_HEAD(freeable);
	int nr_pruned = 0;
	int nr_scanned;
	unsigned long reap = 0;

	down_read(&iprune_sem);
	spin_lock(&inode_lock);
	for (nr_scanned = 0; nr_scanned < nr_to_scan; nr_scanned++) {
		struct inode *inode;

		if (list_empty(&inode_unused))
			break;

		inode = list_entry(inode_unused.prev, struct inode, i_list);

		if (inode->i_state || atomic_read(&inode->i_count)) {
			list_move(&inode->i_list, &inode_unused);
			continue;
		}
		if (inode_has_buffers(inode) || inode->i_data.nrpages) {
			__iget(inode);
			spin_unlock(&inode_lock);
			if (remove_inode_buffers(inode))
				reap += invalidate_mapping_pages(&inode->i_data,
								0, -1);
			iput(inode);
			spin_lock(&inode_lock);

			if (inode != list_entry(inode_unused.next,
						struct inode, i_list))
				continue;	 
			if (!can_unuse(inode))
				continue;
		}
		list_move(&inode->i_list, &freeable);
		WARN_ON(inode->i_state & I_NEW);
		inode->i_state |= I_FREEING;
		nr_pruned++;
	}
	inodes_stat.nr_unused -= nr_pruned;
	if (current_is_kswapd())
		__count_vm_events(KSWAPD_INODESTEAL, reap);
	else
		__count_vm_events(PGINODESTEAL, reap);
	spin_unlock(&inode_lock);

	dispose_list(&freeable);
	up_read(&iprune_sem);
}

static int shrink_icache_memory(int nr, gfp_t gfp_mask)
{
	if (nr) {
		 
		if (!(gfp_mask & __GFP_FS))
			return -1;
		prune_icache(nr);
	}
	return (inodes_stat.nr_unused / 100) * sysctl_vfs_cache_pressure;
}

static struct shrinker icache_shrinker = {
	.shrink = shrink_icache_memory,
	.seeks = DEFAULT_SEEKS,
};

static void __wait_on_freeing_inode(struct inode *inode);
 
static struct inode *find_inode(struct super_block *sb,
				struct hlist_head *head,
				int (*test)(struct inode *, void *),
				void *data)
{
	struct hlist_node *node;
	struct inode *inode = NULL;

repeat:
	hlist_for_each_entry(inode, node, head, i_hash) {
		if (inode->i_sb != sb)
			continue;
		if (!test(inode, data))
			continue;
		if (inode->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE)) {
			__wait_on_freeing_inode(inode);
			goto repeat;
		}
		break;
	}
	return node ? inode : NULL;
}

static struct inode *find_inode_fast(struct super_block *sb,
				struct hlist_head *head, unsigned long ino)
{
	struct hlist_node *node;
	struct inode *inode = NULL;

repeat:
	hlist_for_each_entry(inode, node, head, i_hash) {
		if (inode->i_ino != ino)
			continue;
		if (inode->i_sb != sb)
			continue;
		if (inode->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE)) {
			__wait_on_freeing_inode(inode);
			goto repeat;
		}
		break;
	}
	return node ? inode : NULL;
}

static unsigned long hash(struct super_block *sb, unsigned long hashval)
{
	unsigned long tmp;

	tmp = (hashval * (unsigned long)sb) ^ (GOLDEN_RATIO_PRIME + hashval) /
			L1_CACHE_BYTES;
	tmp = tmp ^ ((tmp ^ GOLDEN_RATIO_PRIME) >> I_HASHBITS);
	return tmp & I_HASHMASK;
}

static inline void
__inode_add_to_lists(struct super_block *sb, struct hlist_head *head,
			struct inode *inode)
{
	inodes_stat.nr_inodes++;
	list_add(&inode->i_list, &inode_in_use);
	list_add(&inode->i_sb_list, &sb->s_inodes);
	if (head)
		hlist_add_head(&inode->i_hash, head);
}

void inode_add_to_lists(struct super_block *sb, struct inode *inode)
{
	struct hlist_head *head = inode_hashtable + hash(sb, inode->i_ino);

	spin_lock(&inode_lock);
	__inode_add_to_lists(sb, head, inode);
	spin_unlock(&inode_lock);
}
EXPORT_SYMBOL_GPL(inode_add_to_lists);

struct inode *new_inode(struct super_block *sb)
{
	 
	static unsigned int last_ino;
	struct inode *inode;

	spin_lock_prefetch(&inode_lock);

	inode = alloc_inode(sb);
	if (inode) {
		spin_lock(&inode_lock);
		__inode_add_to_lists(sb, NULL, inode);
		inode->i_ino = ++last_ino;
		inode->i_state = 0;
		spin_unlock(&inode_lock);
	}
	return inode;
}
EXPORT_SYMBOL(new_inode);

void unlock_new_inode(struct inode *inode)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	if (inode->i_mode & S_IFDIR) {
		struct file_system_type *type = inode->i_sb->s_type;

		if (!lockdep_match_class(&inode->i_mutex,
		    &type->i_mutex_key)) {
			 
			mutex_destroy(&inode->i_mutex);
			mutex_init(&inode->i_mutex);
			lockdep_set_class(&inode->i_mutex,
					  &type->i_mutex_dir_key);
		}
	}
#endif
	 
	smp_mb();
	WARN_ON((inode->i_state & (I_LOCK|I_NEW)) != (I_LOCK|I_NEW));
	inode->i_state &= ~(I_LOCK|I_NEW);
	wake_up_inode(inode);
}
EXPORT_SYMBOL(unlock_new_inode);

static struct inode *get_new_inode(struct super_block *sb,
				struct hlist_head *head,
				int (*test)(struct inode *, void *),
				int (*set)(struct inode *, void *),
				void *data)
{
	struct inode *inode;

	inode = alloc_inode(sb);
	if (inode) {
		struct inode *old;

		spin_lock(&inode_lock);
		 
		old = find_inode(sb, head, test, data);
		if (!old) {
			if (set(inode, data))
				goto set_failed;

			__inode_add_to_lists(sb, head, inode);
			inode->i_state = I_LOCK|I_NEW;
			spin_unlock(&inode_lock);

			return inode;
		}

		__iget(old);
		spin_unlock(&inode_lock);
		destroy_inode(inode);
		inode = old;
		wait_on_inode(inode);
	}
	return inode;

set_failed:
	spin_unlock(&inode_lock);
	destroy_inode(inode);
	return NULL;
}

static struct inode *get_new_inode_fast(struct super_block *sb,
				struct hlist_head *head, unsigned long ino)
{
	struct inode *inode;

	inode = alloc_inode(sb);
	if (inode) {
		struct inode *old;

		spin_lock(&inode_lock);
		 
		old = find_inode_fast(sb, head, ino);
		if (!old) {
			inode->i_ino = ino;
			__inode_add_to_lists(sb, head, inode);
			inode->i_state = I_LOCK|I_NEW;
			spin_unlock(&inode_lock);

			return inode;
		}

		__iget(old);
		spin_unlock(&inode_lock);
		destroy_inode(inode);
		inode = old;
		wait_on_inode(inode);
	}
	return inode;
}

ino_t iunique(struct super_block *sb, ino_t max_reserved)
{
	 
	static unsigned int counter;
	struct inode *inode;
	struct hlist_head *head;
	ino_t res;

	spin_lock(&inode_lock);
	do {
		if (counter <= max_reserved)
			counter = max_reserved + 1;
		res = counter++;
		head = inode_hashtable + hash(sb, res);
		inode = find_inode_fast(sb, head, res);
	} while (inode != NULL);
	spin_unlock(&inode_lock);

	return res;
}
EXPORT_SYMBOL(iunique);

struct inode *igrab(struct inode *inode)
{
	spin_lock(&inode_lock);
	if (!(inode->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE)))
		__iget(inode);
	else
		 
		inode = NULL;
	spin_unlock(&inode_lock);
	return inode;
}
EXPORT_SYMBOL(igrab);

static struct inode *ifind(struct super_block *sb,
		struct hlist_head *head, int (*test)(struct inode *, void *),
		void *data, const int wait)
{
	struct inode *inode;

	spin_lock(&inode_lock);
	inode = find_inode(sb, head, test, data);
	if (inode) {
		__iget(inode);
		spin_unlock(&inode_lock);
		if (likely(wait))
			wait_on_inode(inode);
		return inode;
	}
	spin_unlock(&inode_lock);
	return NULL;
}

static struct inode *ifind_fast(struct super_block *sb,
		struct hlist_head *head, unsigned long ino)
{
	struct inode *inode;

	spin_lock(&inode_lock);
	inode = find_inode_fast(sb, head, ino);
	if (inode) {
		__iget(inode);
		spin_unlock(&inode_lock);
		wait_on_inode(inode);
		return inode;
	}
	spin_unlock(&inode_lock);
	return NULL;
}

struct inode *ilookup5_nowait(struct super_block *sb, unsigned long hashval,
		int (*test)(struct inode *, void *), void *data)
{
	struct hlist_head *head = inode_hashtable + hash(sb, hashval);

	return ifind(sb, head, test, data, 0);
}
EXPORT_SYMBOL(ilookup5_nowait);

struct inode *ilookup5(struct super_block *sb, unsigned long hashval,
		int (*test)(struct inode *, void *), void *data)
{
	struct hlist_head *head = inode_hashtable + hash(sb, hashval);

	return ifind(sb, head, test, data, 1);
}
EXPORT_SYMBOL(ilookup5);

struct inode *ilookup(struct super_block *sb, unsigned long ino)
{
	struct hlist_head *head = inode_hashtable + hash(sb, ino);

	return ifind_fast(sb, head, ino);
}
EXPORT_SYMBOL(ilookup);

struct inode *iget5_locked(struct super_block *sb, unsigned long hashval,
		int (*test)(struct inode *, void *),
		int (*set)(struct inode *, void *), void *data)
{
	struct hlist_head *head = inode_hashtable + hash(sb, hashval);
	struct inode *inode;

	inode = ifind(sb, head, test, data, 1);
	if (inode)
		return inode;
	 
	return get_new_inode(sb, head, test, set, data);
}
EXPORT_SYMBOL(iget5_locked);

struct inode *iget_locked(struct super_block *sb, unsigned long ino)
{
	struct hlist_head *head = inode_hashtable + hash(sb, ino);
	struct inode *inode;

	inode = ifind_fast(sb, head, ino);
	if (inode)
		return inode;
	 
	return get_new_inode_fast(sb, head, ino);
}
EXPORT_SYMBOL(iget_locked);

int insert_inode_locked(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	ino_t ino = inode->i_ino;
	struct hlist_head *head = inode_hashtable + hash(sb, ino);

	inode->i_state |= I_LOCK|I_NEW;
	while (1) {
		struct hlist_node *node;
		struct inode *old = NULL;
		spin_lock(&inode_lock);
		hlist_for_each_entry(old, node, head, i_hash) {
			if (old->i_ino != ino)
				continue;
			if (old->i_sb != sb)
				continue;
			if (old->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE))
				continue;
			break;
		}
		if (likely(!node)) {
			hlist_add_head(&inode->i_hash, head);
			spin_unlock(&inode_lock);
			return 0;
		}
		__iget(old);
		spin_unlock(&inode_lock);
		wait_on_inode(old);
		if (unlikely(!hlist_unhashed(&old->i_hash))) {
			iput(old);
			return -EBUSY;
		}
		iput(old);
	}
}
EXPORT_SYMBOL(insert_inode_locked);

int insert_inode_locked4(struct inode *inode, unsigned long hashval,
		int (*test)(struct inode *, void *), void *data)
{
	struct super_block *sb = inode->i_sb;
	struct hlist_head *head = inode_hashtable + hash(sb, hashval);

	inode->i_state |= I_LOCK|I_NEW;

	while (1) {
		struct hlist_node *node;
		struct inode *old = NULL;

		spin_lock(&inode_lock);
		hlist_for_each_entry(old, node, head, i_hash) {
			if (old->i_sb != sb)
				continue;
			if (!test(old, data))
				continue;
			if (old->i_state & (I_FREEING|I_CLEAR|I_WILL_FREE))
				continue;
			break;
		}
		if (likely(!node)) {
			hlist_add_head(&inode->i_hash, head);
			spin_unlock(&inode_lock);
			return 0;
		}
		__iget(old);
		spin_unlock(&inode_lock);
		wait_on_inode(old);
		if (unlikely(!hlist_unhashed(&old->i_hash))) {
			iput(old);
			return -EBUSY;
		}
		iput(old);
	}
}
EXPORT_SYMBOL(insert_inode_locked4);

void __insert_inode_hash(struct inode *inode, unsigned long hashval)
{
	struct hlist_head *head = inode_hashtable + hash(inode->i_sb, hashval);
	spin_lock(&inode_lock);
	hlist_add_head(&inode->i_hash, head);
	spin_unlock(&inode_lock);
}
EXPORT_SYMBOL(__insert_inode_hash);

void remove_inode_hash(struct inode *inode)
{
	spin_lock(&inode_lock);
	hlist_del_init(&inode->i_hash);
	spin_unlock(&inode_lock);
}
EXPORT_SYMBOL(remove_inode_hash);

void generic_delete_inode(struct inode *inode)
{
	const struct super_operations *op = inode->i_sb->s_op;

	list_del_init(&inode->i_list);
	list_del_init(&inode->i_sb_list);
	WARN_ON(inode->i_state & I_NEW);
	inode->i_state |= I_FREEING;
	inodes_stat.nr_inodes--;
	spin_unlock(&inode_lock);

	security_inode_delete(inode);

	if (op->delete_inode) {
		void (*delete)(struct inode *) = op->delete_inode;
		if (!is_bad_inode(inode))
			vfs_dq_init(inode);
		 
		delete(inode);
	} else {
		truncate_inode_pages(&inode->i_data, 0);
		clear_inode(inode);
	}
	spin_lock(&inode_lock);
	hlist_del_init(&inode->i_hash);
	spin_unlock(&inode_lock);
	wake_up_inode(inode);
	BUG_ON(inode->i_state != I_CLEAR);
	destroy_inode(inode);
}
EXPORT_SYMBOL(generic_delete_inode);

int generic_detach_inode(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;

	if (!hlist_unhashed(&inode->i_hash)) {
		if (!(inode->i_state & (I_DIRTY|I_SYNC)))
			list_move(&inode->i_list, &inode_unused);
		inodes_stat.nr_unused++;
		if (sb->s_flags & MS_ACTIVE) {
			spin_unlock(&inode_lock);
			return 0;
		}
		WARN_ON(inode->i_state & I_NEW);
		inode->i_state |= I_WILL_FREE;
		spin_unlock(&inode_lock);
		write_inode_now(inode, 1);
		spin_lock(&inode_lock);
		WARN_ON(inode->i_state & I_NEW);
		inode->i_state &= ~I_WILL_FREE;
		inodes_stat.nr_unused--;
		hlist_del_init(&inode->i_hash);
	}
	list_del_init(&inode->i_list);
	list_del_init(&inode->i_sb_list);
	WARN_ON(inode->i_state & I_NEW);
	inode->i_state |= I_FREEING;
	inodes_stat.nr_inodes--;
	spin_unlock(&inode_lock);
	return 1;
}
EXPORT_SYMBOL_GPL(generic_detach_inode);

static void generic_forget_inode(struct inode *inode)
{
	if (!generic_detach_inode(inode))
		return;
	if (inode->i_data.nrpages)
		truncate_inode_pages(&inode->i_data, 0);
	clear_inode(inode);
	wake_up_inode(inode);
	destroy_inode(inode);
}

void generic_drop_inode(struct inode *inode)
{
	if (!inode->i_nlink)
		generic_delete_inode(inode);
	else
		generic_forget_inode(inode);
}
EXPORT_SYMBOL_GPL(generic_drop_inode);

static inline void iput_final(struct inode *inode)
{
	const struct super_operations *op = inode->i_sb->s_op;
	void (*drop)(struct inode *) = generic_drop_inode;

	if (op && op->drop_inode)
		drop = op->drop_inode;
	drop(inode);
}

void iput(struct inode *inode)
{
	if (inode) {
		BUG_ON(inode->i_state == I_CLEAR);

		if (atomic_dec_and_lock(&inode->i_count, &inode_lock))
			iput_final(inode);
	}
}
EXPORT_SYMBOL(iput);

sector_t bmap(struct inode *inode, sector_t block)
{
	sector_t res = 0;
	if (inode->i_mapping->a_ops->bmap)
		res = inode->i_mapping->a_ops->bmap(inode->i_mapping, block);
	return res;
}
EXPORT_SYMBOL(bmap);

static int relatime_need_update(struct vfsmount *mnt, struct inode *inode,
			     struct timespec now)
{
#ifdef MY_ABC_HERE
	long relatime_period = 1;
#endif  

	if (!(mnt->mnt_flags & MNT_RELATIME))
		return 1;
	 
	if (timespec_compare(&inode->i_mtime, &inode->i_atime) >= 0)
		return 1;
	 
	if (timespec_compare(&inode->i_ctime, &inode->i_atime) >= 0)
		return 1;

#ifdef MY_ABC_HERE
	if (inode->i_sb->relatime_period > 0)
		relatime_period = inode->i_sb->relatime_period;

	if ((long)(now.tv_sec - inode->i_atime.tv_sec) >= relatime_period*24*60*60)
		return 1;
#else
	if ((long)(now.tv_sec - inode->i_atime.tv_sec) >= 24*60*60)
		return 1;
#endif  
	 
	return 0;
}

void touch_atime(struct vfsmount *mnt, struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct timespec now;

	if (inode->i_flags & S_NOATIME)
		return;
	if (IS_NOATIME(inode))
		return;
	if ((inode->i_sb->s_flags & MS_NODIRATIME) && S_ISDIR(inode->i_mode))
		return;

	if (mnt->mnt_flags & MNT_NOATIME)
		return;
	if ((mnt->mnt_flags & MNT_NODIRATIME) && S_ISDIR(inode->i_mode))
		return;

	now = current_fs_time(inode->i_sb);

	if (!relatime_need_update(mnt, inode, now))
		return;

	if (timespec_equal(&inode->i_atime, &now))
		return;

	if (mnt_want_write(mnt))
		return;

	inode->i_atime = now;
	mark_inode_dirty_sync(inode);
	mnt_drop_write(mnt);
}
EXPORT_SYMBOL(touch_atime);

void file_update_time(struct file *file)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	struct timespec now;
	enum { S_MTIME = 1, S_CTIME = 2, S_VERSION = 4 } sync_it = 0;

	if (IS_NOCMTIME(inode))
		return;

	now = current_fs_time(inode->i_sb);
	if (!timespec_equal(&inode->i_mtime, &now))
		sync_it = S_MTIME;

	if (!timespec_equal(&inode->i_ctime, &now))
		sync_it |= S_CTIME;

	if (IS_I_VERSION(inode))
		sync_it |= S_VERSION;

	if (!sync_it)
		return;

	if (mnt_want_write_file(file))
		return;

	if (sync_it & S_VERSION)
		inode_inc_iversion(inode);
	if (sync_it & S_CTIME)
		inode->i_ctime = now;
	if (sync_it & S_MTIME)
		inode->i_mtime = now;
	mark_inode_dirty_sync(inode);
	mnt_drop_write(file->f_path.mnt);
}
EXPORT_SYMBOL(file_update_time);

int inode_needs_sync(struct inode *inode)
{
	if (IS_SYNC(inode))
		return 1;
	if (S_ISDIR(inode->i_mode) && IS_DIRSYNC(inode))
		return 1;
	return 0;
}
EXPORT_SYMBOL(inode_needs_sync);

int inode_wait(void *word)
{
	schedule();
	return 0;
}
EXPORT_SYMBOL(inode_wait);

static void __wait_on_freeing_inode(struct inode *inode)
{
	wait_queue_head_t *wq;
	DEFINE_WAIT_BIT(wait, &inode->i_state, __I_LOCK);
	wq = bit_waitqueue(&inode->i_state, __I_LOCK);
	prepare_to_wait(wq, &wait.wait, TASK_UNINTERRUPTIBLE);
	spin_unlock(&inode_lock);
	schedule();
	finish_wait(wq, &wait.wait);
	spin_lock(&inode_lock);
}

static __initdata unsigned long ihash_entries;
static int __init set_ihash_entries(char *str)
{
	if (!str)
		return 0;
	ihash_entries = simple_strtoul(str, &str, 0);
	return 1;
}
__setup("ihash_entries=", set_ihash_entries);

void __init inode_init_early(void)
{
	int loop;

	if (hashdist)
		return;

	inode_hashtable =
		alloc_large_system_hash("Inode-cache",
					sizeof(struct hlist_head),
					ihash_entries,
					14,
					HASH_EARLY,
					&i_hash_shift,
					&i_hash_mask,
					0);

	for (loop = 0; loop < (1 << i_hash_shift); loop++)
		INIT_HLIST_HEAD(&inode_hashtable[loop]);
}

void __init inode_init(void)
{
	int loop;

	inode_cachep = kmem_cache_create("inode_cache",
					 sizeof(struct inode),
					 0,
					 (SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|
					 SLAB_MEM_SPREAD),
					 init_once);
	register_shrinker(&icache_shrinker);

	if (!hashdist)
		return;

	inode_hashtable =
		alloc_large_system_hash("Inode-cache",
					sizeof(struct hlist_head),
					ihash_entries,
					14,
					0,
					&i_hash_shift,
					&i_hash_mask,
					0);

	for (loop = 0; loop < (1 << i_hash_shift); loop++)
		INIT_HLIST_HEAD(&inode_hashtable[loop]);
}

void init_special_inode(struct inode *inode, umode_t mode, dev_t rdev)
{
	inode->i_mode = mode;
	if (S_ISCHR(mode)) {
		inode->i_fop = &def_chr_fops;
		inode->i_rdev = rdev;
	} else if (S_ISBLK(mode)) {
		inode->i_fop = &def_blk_fops;
		inode->i_rdev = rdev;
	} else if (S_ISFIFO(mode))
		inode->i_fop = &def_fifo_fops;
	else if (S_ISSOCK(mode))
		inode->i_fop = &bad_sock_fops;
	else
		printk(KERN_DEBUG "init_special_inode: bogus i_mode (%o) for"
				  " inode %s:%lu\n", mode, inode->i_sb->s_id,
				  inode->i_ino);
}
EXPORT_SYMBOL(init_special_inode);
