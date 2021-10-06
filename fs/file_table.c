#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/fs/file_table.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 1997 David S. Miller (davem@caip.rutgers.edu)
 */

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/security.h>
#include <linux/eventpoll.h>
#include <linux/rcupdate.h>
#include <linux/mount.h>
#include <linux/capability.h>
#include <linux/cdev.h>
#include <linux/fsnotify.h>
#include <linux/sysctl.h>
#include <linux/percpu_counter.h>
#include <linux/percpu.h>
#include <linux/hardirq.h>
#include <linux/task_work.h>
#include <linux/ima.h>
#include <linux/swap.h>

#include <linux/atomic.h>
#ifdef MY_ABC_HERE
#include <linux/lglock.h>
#include <linux/dcache.h>
#include "mount.h"
#endif /*MY_ABC_HERE*/

#include "internal.h"

/* sysctl tunables... */
struct files_stat_struct files_stat = {
	.max_files = NR_FILE
};
#ifdef MY_ABC_HERE
DEFINE_STATIC_LGLOCK(files_lglock);
#endif /* MY_ABC_HERE */

/* SLAB cache for file structures */
static struct kmem_cache *filp_cachep __read_mostly;

static struct percpu_counter nr_files __cacheline_aligned_in_smp;

static void file_free_rcu(struct rcu_head *head)
{
	struct file *f = container_of(head, struct file, f_u.fu_rcuhead);

	put_cred(f->f_cred);
	kmem_cache_free(filp_cachep, f);
}

static inline void file_free(struct file *f)
{
	percpu_counter_dec(&nr_files);
	call_rcu(&f->f_u.fu_rcuhead, file_free_rcu);
}

/*
 * Return the total number of open files in the system
 */
static long get_nr_files(void)
{
	return percpu_counter_read_positive(&nr_files);
}

/*
 * Return the maximum number of open files in the system
 */
unsigned long get_max_files(void)
{
	return files_stat.max_files;
}
EXPORT_SYMBOL_GPL(get_max_files);

/*
 * Handle nr_files sysctl
 */
#if defined(CONFIG_SYSCTL) && defined(CONFIG_PROC_FS)
int proc_nr_files(struct ctl_table *table, int write,
                     void __user *buffer, size_t *lenp, loff_t *ppos)
{
	files_stat.nr_files = get_nr_files();
	return proc_doulongvec_minmax(table, write, buffer, lenp, ppos);
}
#else
int proc_nr_files(struct ctl_table *table, int write,
                     void __user *buffer, size_t *lenp, loff_t *ppos)
{
	return -ENOSYS;
}
#endif

/* Find an unused file structure and return a pointer to it.
 * Returns an error pointer if some error happend e.g. we over file
 * structures limit, run out of memory or operation is not permitted.
 *
 * Be very careful using this.  You are responsible for
 * getting write access to any mount that you might assign
 * to this filp, if it is opened for write.  If this is not
 * done, you will imbalance int the mount's writer count
 * and a warning at __fput() time.
 */
struct file *get_empty_filp(void)
{
	const struct cred *cred = current_cred();
	static long old_max;
	struct file *f;
	int error;

	/*
	 * Privileged users can go above max_files
	 */
	if (get_nr_files() >= files_stat.max_files && !capable(CAP_SYS_ADMIN)) {
		/*
		 * percpu_counters are inaccurate.  Do an expensive check before
		 * we go and fail.
		 */
		if (percpu_counter_sum_positive(&nr_files) >= files_stat.max_files)
			goto over;
	}

	f = kmem_cache_zalloc(filp_cachep, GFP_KERNEL);
	if (unlikely(!f))
		return ERR_PTR(-ENOMEM);

	percpu_counter_inc(&nr_files);
	f->f_cred = get_cred(cred);
	error = security_file_alloc(f);
	if (unlikely(error)) {
		file_free(f);
		return ERR_PTR(error);
	}
#ifdef MY_ABC_HERE
	INIT_LIST_HEAD(&f->f_u.fu_list);
#endif /* MY_ABC_HERE */
	atomic_long_set(&f->f_count, 1);
	rwlock_init(&f->f_owner.lock);
	spin_lock_init(&f->f_lock);
	mutex_init(&f->f_pos_lock);
	eventpoll_init_file(f);
	/* f->f_version: 0 */
	return f;

over:
	/* Ran out of filps - report that */
	if (get_nr_files() > old_max) {
		pr_info("VFS: file-max limit %lu reached\n", get_max_files());
		old_max = get_nr_files();
	}
	return ERR_PTR(-ENFILE);
}
#ifdef CONFIG_AUFS_FHSM
EXPORT_SYMBOL_GPL(get_empty_filp);
#endif /* CONFIG_AUFS_FHSM */

/**
 * alloc_file - allocate and initialize a 'struct file'
 *
 * @path: the (dentry, vfsmount) pair for the new file
 * @mode: the mode with which the new file will be opened
 * @fop: the 'struct file_operations' for the new file
 */
struct file *alloc_file(struct path *path, fmode_t mode,
		const struct file_operations *fop)
{
	struct file *file;

	file = get_empty_filp();
	if (IS_ERR(file))
		return file;

	file->f_path = *path;
	file->f_inode = path->dentry->d_inode;
	file->f_mapping = path->dentry->d_inode->i_mapping;
	if ((mode & FMODE_READ) &&
	     likely(fop->read || fop->read_iter))
		mode |= FMODE_CAN_READ;
	if ((mode & FMODE_WRITE) &&
	     likely(fop->write || fop->write_iter))
		mode |= FMODE_CAN_WRITE;
	file->f_mode = mode;
	file->f_op = fop;
	if ((mode & (FMODE_READ | FMODE_WRITE)) == FMODE_READ)
		i_readcount_inc(path->dentry->d_inode);
	return file;
}
EXPORT_SYMBOL(alloc_file);

/* the real guts of fput() - releasing the last reference to file
 */
static void __fput(struct file *file)
{
	struct dentry *dentry = file->f_path.dentry;
	struct vfsmount *mnt = file->f_path.mnt;
	struct inode *inode = file->f_inode;

	might_sleep();

	fsnotify_close(file);
	/*
	 * The function eventpoll_release() should be the first called
	 * in the file cleanup chain.
	 */
	eventpoll_release(file);
	locks_remove_file(file);

	if (unlikely(file->f_flags & FASYNC)) {
		if (file->f_op->fasync)
			file->f_op->fasync(-1, file, 0);
	}
	ima_file_free(file);
	if (file->f_op->release)
		file->f_op->release(inode, file);
	security_file_free(file);
	if (unlikely(S_ISCHR(inode->i_mode) && inode->i_cdev != NULL &&
		     !(file->f_mode & FMODE_PATH))) {
		cdev_put(inode->i_cdev);
	}
	fops_put(file->f_op);
	put_pid(file->f_owner.pid);
	if ((file->f_mode & (FMODE_READ | FMODE_WRITE)) == FMODE_READ)
		i_readcount_dec(inode);
	if (file->f_mode & FMODE_WRITER) {
		put_write_access(inode);
		__mnt_drop_write(mnt);
	}
	file->f_path.dentry = NULL;
	file->f_path.mnt = NULL;
	file->f_inode = NULL;
	file_free(file);
	dput(dentry);
	mntput(mnt);
}

static LLIST_HEAD(delayed_fput_list);
static void delayed_fput(struct work_struct *unused)
{
	struct llist_node *node = llist_del_all(&delayed_fput_list);
	struct llist_node *next;

	for (; node; node = next) {
		next = llist_next(node);
		__fput(llist_entry(node, struct file, f_u.fu_llist));
	}
}

static void ____fput(struct callback_head *work)
{
	__fput(container_of(work, struct file, f_u.fu_rcuhead));
}

/*
 * If kernel thread really needs to have the final fput() it has done
 * to complete, call this.  The only user right now is the boot - we
 * *do* need to make sure our writes to binaries on initramfs has
 * not left us with opened struct file waiting for __fput() - execve()
 * won't work without that.  Please, don't add more callers without
 * very good reasons; in particular, never call that with locks
 * held and never call that from a thread that might need to do
 * some work on any kind of umount.
 */
void flush_delayed_fput(void)
{
	delayed_fput(NULL);
}
#ifdef CONFIG_AUFS_FHSM
EXPORT_SYMBOL_GPL(flush_delayed_fput);
#endif /* CONFIG_AUFS_FHSM */

static DECLARE_DELAYED_WORK(delayed_fput_work, delayed_fput);

void fput(struct file *file)
{
	if (atomic_long_dec_and_test(&file->f_count)) {
		struct task_struct *task = current;
#ifdef MY_ABC_HERE
		file_sb_list_del(file);
#endif /* MY_ABC_HERE */
		if (likely(!in_interrupt() && !(task->flags & PF_KTHREAD))) {
			init_task_work(&file->f_u.fu_rcuhead, ____fput);
			if (!task_work_add(task, &file->f_u.fu_rcuhead, true))
				return;
			/*
			 * After this task has run exit_task_work(),
			 * task_work_add() will fail.  Fall through to delayed
			 * fput to avoid leaking *file.
			 */
		}

		if (llist_add(&file->f_u.fu_llist, &delayed_fput_list))
			schedule_delayed_work(&delayed_fput_work, 1);
	}
}

/*
 * synchronous analog of fput(); for kernel threads that might be needed
 * in some umount() (and thus can't use flush_delayed_fput() without
 * risking deadlocks), need to wait for completion of __fput() and know
 * for this specific struct file it won't involve anything that would
 * need them.  Use only if you really need it - at the very least,
 * don't blindly convert fput() by kernel thread to that.
 */
void __fput_sync(struct file *file)
{
	if (atomic_long_dec_and_test(&file->f_count)) {
		struct task_struct *task = current;
#ifdef MY_ABC_HERE
		file_sb_list_del(file);
#endif /* MY_ABC_HERE */
		BUG_ON(!(task->flags & PF_KTHREAD));
		__fput(file);
	}
}

EXPORT_SYMBOL(fput);
#ifdef CONFIG_AUFS_FHSM
EXPORT_SYMBOL_GPL(__fput_sync);
#endif /* CONFIG_AUFS_FHSM */

void put_filp(struct file *file)
{
	if (atomic_long_dec_and_test(&file->f_count)) {
		security_file_free(file);
#ifdef MY_ABC_HERE
		file_sb_list_del(file);
#endif /* MY_ABC_HERE */
		file_free(file);
	}
}
#ifdef CONFIG_AUFS_FHSM
EXPORT_SYMBOL_GPL(put_filp);
#endif /* CONFIG_AUFS_FHSM */

void __init files_init(void)
{ 
	filp_cachep = kmem_cache_create("filp", sizeof(struct file), 0,
			SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);
#ifdef MY_ABC_HERE
	lg_lock_init(&files_lglock, "files_lglock");
#endif /* MY_ABC_HERE */
	percpu_counter_init(&nr_files, 0, GFP_KERNEL);
}

/*
 * One file with associated inode and dcache is very roughly 1K. Per default
 * do not use more than 10% of our memory for files.
 */
void __init files_maxfiles_init(void)
{
	unsigned long n;
	unsigned long memreserve = (totalram_pages - nr_free_pages()) * 3/2;

	memreserve = min(memreserve, totalram_pages - 1);
	n = ((totalram_pages - memreserve) * (PAGE_SIZE / 1024)) / 10;

	files_stat.max_files = max_t(unsigned long, n, NR_FILE);
} 

#ifdef MY_ABC_HERE
static inline int file_list_cpu(struct file *file)
{
#ifdef CONFIG_SMP
	return file->f_sb_list_cpu;
#else
	return smp_processor_id();
#endif
}

/* helper for file_sb_list_add to reduce ifdefs */
static inline void __file_sb_list_add(struct file *file, struct super_block *sb)
{
	struct list_head *list;
#ifdef CONFIG_SMP
	int cpu;
	cpu = smp_processor_id();
	file->f_sb_list_cpu = cpu;
	list = per_cpu_ptr(sb->s_files, cpu);
#else
	list = &sb->s_files;
#endif
	list_add(&file->f_u.fu_list, list);
}

/**
 * file_sb_list_add - add a file to the sb's file list
 * @file: file to add
 * @sb: sb to add it to
 *
 * Use this function to associate a file with the superblock of the inode it
 * refers to.
 */
void file_sb_list_add(struct file *file, struct super_block *sb)
{
	lg_local_lock(&files_lglock);
	__file_sb_list_add(file, sb);
	lg_local_unlock(&files_lglock);
}

/**
 * file_sb_list_del - remove a file from the sb's file list
 * @file: file to remove
 * @sb: sb to remove it from
 *
 * Use this function to remove a file from its superblock.
 */
void file_sb_list_del(struct file *file)
{
	if (!list_empty(&file->f_u.fu_list)) {
		lg_local_lock_cpu(&files_lglock, file_list_cpu(file));
		list_del_init(&file->f_u.fu_list);
		lg_local_unlock_cpu(&files_lglock, file_list_cpu(file));
	}
}

#ifdef CONFIG_SMP

/*
 * These macros iterate all files on all CPUs for a given superblock.
 * files_lglock must be held globally.
 */
#define do_file_list_for_each_entry(__sb, __file)		\
{								\
	int i;							\
	for_each_possible_cpu(i) {				\
		struct list_head *list;				\
		list = per_cpu_ptr((__sb)->s_files, i);		\
		list_for_each_entry((__file), list, f_u.fu_list)

#define while_file_list_for_each_entry				\
	}							\
}

#else

#define do_file_list_for_each_entry(__sb, __file)		\
{								\
	struct list_head *list;					\
	list = &(sb)->s_files;					\
	list_for_each_entry((__file), list, f_u.fu_list)

#define while_file_list_for_each_entry				\
}

#endif

#define MAX_SHOWN_OPENED_FILE 10
void fs_show_opened_file(struct mount *mnt,
			 const char *mnt_point_name, char *file_name_buf, int buflen)
{
	struct file *file;
	char *file_name;
	unsigned num_show = 0;

	lg_global_lock(&files_lglock);
	do_file_list_for_each_entry(mnt->mnt.mnt_sb, file) {
		file_name = dentry_path_raw(file->f_path.dentry, file_name_buf, buflen - 1);
		if (IS_ERR(file_name)) {
			continue;
		}
		printk(KERN_WARNING "VFS: opened file in mnt_point: (%s), file: (%s)\n", mnt_point_name, file_name);
		if (MAX_SHOWN_OPENED_FILE <= ++num_show) {
			break;
		}
	} while_file_list_for_each_entry;
	lg_global_unlock(&files_lglock);
}
#endif /* MY_ABC_HERE */
