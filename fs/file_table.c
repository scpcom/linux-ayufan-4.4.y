 
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/security.h>
#include <linux/ima.h>
#include <linux/eventpoll.h>
#include <linux/rcupdate.h>
#include <linux/mount.h>
#include <linux/capability.h>
#include <linux/cdev.h>
#include <linux/fsnotify.h>
#include <linux/sysctl.h>
#include <linux/percpu_counter.h>

#include <asm/atomic.h>

#ifdef SYNO_FORCE_UNMOUNT
#include <linux/fs_struct.h>
#endif

struct files_stat_struct files_stat = {
	.max_files = NR_FILE
};

__cacheline_aligned_in_smp DEFINE_SPINLOCK(files_lock);

static struct kmem_cache *filp_cachep __read_mostly;

static struct percpu_counter nr_files __cacheline_aligned_in_smp;

static inline void file_free_rcu(struct rcu_head *head)
{
	struct file *f = container_of(head, struct file, f_u.fu_rcuhead);

	put_cred(f->f_cred);
	kmem_cache_free(filp_cachep, f);
}

static inline void file_free(struct file *f)
{
	percpu_counter_dec(&nr_files);
	file_check_state(f);
	call_rcu(&f->f_u.fu_rcuhead, file_free_rcu);
}

static int get_nr_files(void)
{
	return percpu_counter_read_positive(&nr_files);
}

int get_max_files(void)
{
	return files_stat.max_files;
}
EXPORT_SYMBOL_GPL(get_max_files);

#if defined(CONFIG_SYSCTL) && defined(CONFIG_PROC_FS)
int proc_nr_files(ctl_table *table, int write,
                     void __user *buffer, size_t *lenp, loff_t *ppos)
{
	files_stat.nr_files = get_nr_files();
	return proc_dointvec(table, write, buffer, lenp, ppos);
}
#else
int proc_nr_files(ctl_table *table, int write,
                     void __user *buffer, size_t *lenp, loff_t *ppos)
{
	return -ENOSYS;
}
#endif

struct file *get_empty_filp(void)
{
	const struct cred *cred = current_cred();
	static int old_max;
	struct file * f;

	if (get_nr_files() >= files_stat.max_files && !capable(CAP_SYS_ADMIN)) {
		 
		if (percpu_counter_sum_positive(&nr_files) >= files_stat.max_files)
			goto over;
	}

	f = kmem_cache_zalloc(filp_cachep, GFP_KERNEL);
	if (f == NULL)
		goto fail;

	percpu_counter_inc(&nr_files);
	if (security_file_alloc(f))
		goto fail_sec;

	INIT_LIST_HEAD(&f->f_u.fu_list);
	atomic_long_set(&f->f_count, 1);
	rwlock_init(&f->f_owner.lock);
#ifdef SYNO_FORCE_UNMOUNT
	spin_lock_init(&f->f_synostate_lock);
	f->f_synostate = O_UNMOUNT_OK;
#endif
	f->f_cred = get_cred(cred);
	spin_lock_init(&f->f_lock);
	eventpoll_init_file(f);
	 
	return f;

over:
	 
	if (get_nr_files() > old_max) {
		printk(KERN_INFO "VFS: file-max limit %d reached\n",
					get_max_files());
		old_max = get_nr_files();
	}
	goto fail;

fail_sec:
	file_free(f);
fail:
	return NULL;
}

EXPORT_SYMBOL(get_empty_filp);

struct file *alloc_file(struct vfsmount *mnt, struct dentry *dentry,
		fmode_t mode, const struct file_operations *fop)
{
	struct file *file;

	file = get_empty_filp();
	if (!file)
		return NULL;

	init_file(file, mnt, dentry, mode, fop);
	return file;
}
EXPORT_SYMBOL(alloc_file);

int init_file(struct file *file, struct vfsmount *mnt, struct dentry *dentry,
	   fmode_t mode, const struct file_operations *fop)
{
	int error = 0;
	file->f_path.dentry = dentry;
	file->f_path.mnt = mntget(mnt);
	file->f_mapping = dentry->d_inode->i_mapping;
	file->f_mode = mode;
	file->f_op = fop;

	if ((mode & FMODE_WRITE) && !special_file(dentry->d_inode->i_mode)) {
		file_take_write(file);
		error = mnt_clone_write(mnt);
		WARN_ON(error);
	}
	return error;
}
EXPORT_SYMBOL(init_file);

#ifdef SYNO_FORCE_UNMOUNT
#include <linux/namei.h>
#endif

void fput(struct file *file)
{
#ifdef SYNO_FORCE_UNMOUNT
	int doForce = 0;
	struct nameidata nd;

	spin_lock(&file->f_synostate_lock);
	if (O_UNMOUNT_WAIT == file->f_synostate) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("put %s file(%ld) dentry(%d)\n",file->f_path.dentry->d_name.name, 
			   file_count(file), atomic_read(&file->f_dentry->d_count));
#endif
		file->f_synostate = O_UNMOUNT_DONE;
		doForce = 1;
	}
	spin_unlock(&file->f_synostate_lock);

	if (doForce) {
		struct dentry * dentry = file->f_dentry;
		struct vfsmount * mnt = file->f_vfsmnt;
		struct inode * inode = dentry->d_inode;

		if (file->f_op && file->f_op->flush)
			file->f_op->flush(file, NULL);
		 
		might_sleep();
	
		fsnotify_close(file);
		 
		eventpoll_release(file);
		locks_remove_flock(file);
	
		if (unlikely(file->f_flags & FASYNC)) {
			if (file->f_op && file->f_op->fasync)
				file->f_op->fasync(-1, file, 0);
		}
		if (file->f_op && file->f_op->release)
			file->f_op->release(inode, file);
		security_file_free(file);
		ima_file_free(file);
		if (unlikely(S_ISCHR(inode->i_mode) && inode->i_cdev != NULL))
			cdev_put(inode->i_cdev);
		fops_put(file->f_op);
		put_pid(file->f_owner.pid);
		if (file->f_mode & FMODE_WRITE)
			drop_file_write_access(file);

		path_lookup("/proc/invalidfile", 0, &nd);
		file->f_dentry = nd.path.dentry;
		file->f_vfsmnt = nd.path.mnt;
		file->f_op = fops_get(nd.path.dentry->d_inode->i_fop);
		file->f_mapping = nd.path.dentry->d_inode->i_mapping;

		dput(dentry);
		mntput(mnt);
	}
#endif
	if (atomic_long_dec_and_test(&file->f_count))
		__fput(file);
}

EXPORT_SYMBOL(fput);

void drop_file_write_access(struct file *file)
{
	struct vfsmount *mnt = file->f_path.mnt;
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = dentry->d_inode;

	put_write_access(inode);

	if (special_file(inode->i_mode))
		return;
	if (file_check_writeable(file) != 0)
		return;
	mnt_drop_write(mnt);
	file_release_write(file);
}
EXPORT_SYMBOL_GPL(drop_file_write_access);

#ifdef CONFIG_SYNO_PLX_PORTING
#include <mach/fast_open_filter.h>
#endif
void __fput(struct file *file)
{
	struct dentry *dentry = file->f_path.dentry;
	struct vfsmount *mnt = file->f_path.mnt;
	struct inode *inode = dentry->d_inode;

#ifdef SYNO_FORCE_UNMOUNT
	if (blSynostate(O_UNMOUNT_DONE, file)) {
		fops_put(file->f_op);
		path_put(&file->f_path);
		file_kill(file);
		file_free(file);
		return;
	}
#endif

#ifdef CONFIG_SYNO_PLX_PORTING
 	fast_close_filter(file);
#endif

	might_sleep();

	fsnotify_close(file);
	 
	eventpoll_release(file);
	locks_remove_flock(file);

	if (unlikely(file->f_flags & FASYNC)) {
		if (file->f_op && file->f_op->fasync)
			file->f_op->fasync(-1, file, 0);
	}
	if (file->f_op && file->f_op->release)
		file->f_op->release(inode, file);
	security_file_free(file);
	ima_file_free(file);
	if (unlikely(S_ISCHR(inode->i_mode) && inode->i_cdev != NULL))
		cdev_put(inode->i_cdev);
	fops_put(file->f_op);
	put_pid(file->f_owner.pid);
	file_kill(file);
	if (file->f_mode & FMODE_WRITE)
		drop_file_write_access(file);
	file->f_path.dentry = NULL;
	file->f_path.mnt = NULL;
	file_free(file);
	dput(dentry);
	mntput(mnt);
}

struct file *fget(unsigned int fd)
{
	struct file *file;
	struct files_struct *files = current->files;

	rcu_read_lock();
	file = fcheck_files(files, fd);
	if (file) {
		if (!atomic_long_inc_not_zero(&file->f_count)) {
			 
			rcu_read_unlock();
			return NULL;
		}
	}
	rcu_read_unlock();

	return file;
}

EXPORT_SYMBOL(fget);

struct file *fget_light(unsigned int fd, int *fput_needed)
{
	struct file *file;
	struct files_struct *files = current->files;

	*fput_needed = 0;
	if (likely((atomic_read(&files->count) == 1))) {
		file = fcheck_files(files, fd);
	} else {
		rcu_read_lock();
		file = fcheck_files(files, fd);
		if (file) {
			if (atomic_long_inc_not_zero(&file->f_count))
				*fput_needed = 1;
			else
				 
				file = NULL;
		}
		rcu_read_unlock();
	}

	return file;
}

void put_filp(struct file *file)
{
	if (atomic_long_dec_and_test(&file->f_count)) {
		security_file_free(file);
		file_kill(file);
		file_free(file);
	}
}

void file_move(struct file *file, struct list_head *list)
{
	if (!list)
		return;
	file_list_lock();
	list_move(&file->f_u.fu_list, list);
	file_list_unlock();
}

void file_kill(struct file *file)
{
	if (!list_empty(&file->f_u.fu_list)) {
		file_list_lock();
		list_del_init(&file->f_u.fu_list);
		file_list_unlock();
	}
}

#ifdef SYNO_FORCE_UNMOUNT
#define MAX_FORCE_UNMOUNT_LIMIT 100000
void fs_set_all_files_umount(struct super_block *sb)
{
	struct file *file;

	file_list_lock();
	 
	list_for_each_entry(file, &sb->s_files, f_u.fu_list) {
		spin_lock(&file->f_synostate_lock);
		file->f_synostate = O_UNMOUNT_WAIT;
		spin_unlock(&file->f_synostate_lock);
	}
	file_list_unlock();
}

void fs_force_close_all_files(struct super_block *sb)
{
	int    cLimit = 0;
	int    busyFlag = 0;
	struct file *file;

	do {
		busyFlag = 0;
		file_list_lock();
		 
		list_for_each_entry(file, &sb->s_files, f_u.fu_list) {
			if(!blSynostate(O_UNMOUNT_DONE, file)) {
				busyFlag = 1;
				break;
			}
		}
		file_list_unlock();
		if (busyFlag) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
			printk("force close %s file(%ld) dentry(%d)\n",file->f_path.dentry->d_name.name, 
				   file_count(file), atomic_read(&file->f_dentry->d_count));
#endif
			file->f_dentry->d_inode->i_flags |= S_SYNO_FORCE_UMOUNT;
			get_file(file);
			fput(file);
		}
		if (cLimit > MAX_FORCE_UNMOUNT_LIMIT) {
			break;
		}
		cLimit++;
	} while (busyFlag);
}

#ifdef SYNO_DEBUG_FORCE_UNMOUNT
void fs_show_opened_file(struct super_block *sb)
{
	struct file *file;

	file_list_lock();
	 
	list_for_each_entry(file, &sb->s_files, f_u.fu_list) {
		if(!blSynostate(O_UNMOUNT_DONE, file)) {
			printk("file %s in mnt(%ld) dentry(%d) stat:%d\n",file->f_path.dentry->d_name.name, 
				   file_count(file), atomic_read(&file->f_dentry->d_count),file->f_synostate);
		}
	}
	file_list_unlock();
}
#endif
#endif

int fs_may_remount_ro(struct super_block *sb)
{
	struct file *file;

	file_list_lock();
	list_for_each_entry(file, &sb->s_files, f_u.fu_list) {
		struct inode *inode = file->f_path.dentry->d_inode;

		if (inode->i_nlink == 0)
			goto too_bad;

		if (S_ISREG(inode->i_mode) && (file->f_mode & FMODE_WRITE))
			goto too_bad;
	}
	file_list_unlock();
	return 1;  
too_bad:
	file_list_unlock();
	return 0;
}

void mark_files_ro(struct super_block *sb)
{
	struct file *f;

retry:
	file_list_lock();
	list_for_each_entry(f, &sb->s_files, f_u.fu_list) {
		struct vfsmount *mnt;
		if (!S_ISREG(f->f_path.dentry->d_inode->i_mode))
		       continue;
		if (!file_count(f))
			continue;
		if (!(f->f_mode & FMODE_WRITE))
			continue;
		spin_lock(&f->f_lock);
		f->f_mode &= ~FMODE_WRITE;
		spin_unlock(&f->f_lock);
		if (file_check_writeable(f) != 0)
			continue;
		file_release_write(f);
		mnt = mntget(f->f_path.mnt);
		file_list_unlock();
		 
		mnt_drop_write(mnt);
		mntput(mnt);
		goto retry;
	}
	file_list_unlock();
}

void __init files_init(unsigned long mempages)
{ 
	int n; 

	filp_cachep = kmem_cache_create("filp", sizeof(struct file), 0,
			SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);

	n = (mempages * (PAGE_SIZE / 1024)) / 10;
	files_stat.max_files = n; 
	if (files_stat.max_files < NR_FILE)
		files_stat.max_files = NR_FILE;
	files_defer_init();
	percpu_counter_init(&nr_files, 0);
} 
