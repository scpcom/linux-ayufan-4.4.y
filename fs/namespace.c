#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/acct.h>
#include <linux/capability.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/seq_file.h>
#include <linux/mnt_namespace.h>
#include <linux/namei.h>
#include <linux/nsproxy.h>
#include <linux/security.h>
#include <linux/mount.h>
#include <linux/ramfs.h>
#include <linux/log2.h>
#include <linux/idr.h>
#include <linux/fs_struct.h>
#include <linux/fsnotify.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#ifdef MY_ABC_HERE
#include <linux/string.h>
#endif  
#include "pnode.h"
#include "internal.h"

#define HASH_SHIFT ilog2(PAGE_SIZE / sizeof(struct list_head))
#define HASH_SIZE (1UL << HASH_SHIFT)

#ifdef MY_ABC_HERE
extern int gSynoHasDynModule;
#endif

#ifdef MY_ABC_HERE
extern void ext4_fill_mount_path(struct super_block *sb, const char *szPath);
#endif

#ifdef SYNO_MD_RESHAPE_AND_MOUNT_DEADLOCK_WORKAROUND
extern struct rw_semaphore s_reshape_mount_key;
#endif  

static int event;
static DEFINE_IDA(mnt_id_ida);
static DEFINE_IDA(mnt_group_ida);
static DEFINE_SPINLOCK(mnt_id_lock);
static int mnt_id_start = 0;
static int mnt_group_start = 1;

static struct list_head *mount_hashtable __read_mostly;
static struct kmem_cache *mnt_cache __read_mostly;
static struct rw_semaphore namespace_sem;

struct kobject *fs_kobj;
EXPORT_SYMBOL_GPL(fs_kobj);

DEFINE_BRLOCK(vfsmount_lock);

static inline unsigned long hash(struct vfsmount *mnt, struct dentry *dentry)
{
	unsigned long tmp = ((unsigned long)mnt / L1_CACHE_BYTES);
	tmp += ((unsigned long)dentry / L1_CACHE_BYTES);
	tmp = tmp + (tmp >> HASH_SHIFT);
	return tmp & (HASH_SIZE - 1);
}

#define MNT_WRITER_UNDERFLOW_LIMIT -(1<<16)

static int mnt_alloc_id(struct vfsmount *mnt)
{
	int res;

retry:
	ida_pre_get(&mnt_id_ida, GFP_KERNEL);
	spin_lock(&mnt_id_lock);
	res = ida_get_new_above(&mnt_id_ida, mnt_id_start, &mnt->mnt_id);
	if (!res)
		mnt_id_start = mnt->mnt_id + 1;
	spin_unlock(&mnt_id_lock);
	if (res == -EAGAIN)
		goto retry;

	return res;
}

static void mnt_free_id(struct vfsmount *mnt)
{
	int id = mnt->mnt_id;
	spin_lock(&mnt_id_lock);
	ida_remove(&mnt_id_ida, id);
	if (mnt_id_start > id)
		mnt_id_start = id;
	spin_unlock(&mnt_id_lock);
}

static int mnt_alloc_group_id(struct vfsmount *mnt)
{
	int res;

	if (!ida_pre_get(&mnt_group_ida, GFP_KERNEL))
		return -ENOMEM;

	res = ida_get_new_above(&mnt_group_ida,
				mnt_group_start,
				&mnt->mnt_group_id);
	if (!res)
		mnt_group_start = mnt->mnt_group_id + 1;

	return res;
}

void mnt_release_group_id(struct vfsmount *mnt)
{
	int id = mnt->mnt_group_id;
	ida_remove(&mnt_group_ida, id);
	if (mnt_group_start > id)
		mnt_group_start = id;
	mnt->mnt_group_id = 0;
}

static inline void mnt_add_count(struct vfsmount *mnt, int n)
{
#ifdef CONFIG_SMP
	this_cpu_add(mnt->mnt_pcp->mnt_count, n);
#else
	preempt_disable();
	mnt->mnt_count += n;
	preempt_enable();
#endif
}

static inline void mnt_set_count(struct vfsmount *mnt, int n)
{
#ifdef CONFIG_SMP
	this_cpu_write(mnt->mnt_pcp->mnt_count, n);
#else
	mnt->mnt_count = n;
#endif
}

static inline void mnt_inc_count(struct vfsmount *mnt)
{
	mnt_add_count(mnt, 1);
}

static inline void mnt_dec_count(struct vfsmount *mnt)
{
	mnt_add_count(mnt, -1);
}

unsigned int mnt_get_count(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	unsigned int count = 0;
	int cpu;

	for_each_possible_cpu(cpu) {
		count += per_cpu_ptr(mnt->mnt_pcp, cpu)->mnt_count;
	}

	return count;
#else
	return mnt->mnt_count;
#endif
}

static struct vfsmount *alloc_vfsmnt(const char *name)
{
	struct vfsmount *mnt = kmem_cache_zalloc(mnt_cache, GFP_KERNEL);
	if (mnt) {
		int err;

		err = mnt_alloc_id(mnt);
		if (err)
			goto out_free_cache;

		if (name) {
			mnt->mnt_devname = kstrdup(name, GFP_KERNEL);
			if (!mnt->mnt_devname)
				goto out_free_id;
		}

#ifdef CONFIG_SMP
		mnt->mnt_pcp = alloc_percpu(struct mnt_pcp);
		if (!mnt->mnt_pcp)
			goto out_free_devname;

		this_cpu_add(mnt->mnt_pcp->mnt_count, 1);
#else
		mnt->mnt_count = 1;
		mnt->mnt_writers = 0;
#endif

		INIT_LIST_HEAD(&mnt->mnt_hash);
		INIT_LIST_HEAD(&mnt->mnt_child);
		INIT_LIST_HEAD(&mnt->mnt_mounts);
		INIT_LIST_HEAD(&mnt->mnt_list);
		INIT_LIST_HEAD(&mnt->mnt_expire);
		INIT_LIST_HEAD(&mnt->mnt_share);
		INIT_LIST_HEAD(&mnt->mnt_slave_list);
		INIT_LIST_HEAD(&mnt->mnt_slave);
#ifdef CONFIG_FSNOTIFY
		INIT_HLIST_HEAD(&mnt->mnt_fsnotify_marks);
#endif
	}
	return mnt;

#ifdef CONFIG_SMP
out_free_devname:
	kfree(mnt->mnt_devname);
#endif
out_free_id:
	mnt_free_id(mnt);
out_free_cache:
	kmem_cache_free(mnt_cache, mnt);
	return NULL;
}

int __mnt_is_readonly(struct vfsmount *mnt)
{
	if (mnt->mnt_flags & MNT_READONLY)
		return 1;
	if (mnt->mnt_sb->s_flags & MS_RDONLY)
		return 1;
	return 0;
}
EXPORT_SYMBOL_GPL(__mnt_is_readonly);

static inline void mnt_inc_writers(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	this_cpu_inc(mnt->mnt_pcp->mnt_writers);
#else
	mnt->mnt_writers++;
#endif
}

static inline void mnt_dec_writers(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	this_cpu_dec(mnt->mnt_pcp->mnt_writers);
#else
	mnt->mnt_writers--;
#endif
}

static unsigned int mnt_get_writers(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	unsigned int count = 0;
	int cpu;

	for_each_possible_cpu(cpu) {
		count += per_cpu_ptr(mnt->mnt_pcp, cpu)->mnt_writers;
	}

	return count;
#else
	return mnt->mnt_writers;
#endif
}

int __mnt_want_write(struct vfsmount *m)
{
	int ret = 0;

	preempt_disable();
	mnt_inc_writers(m);
	 
	smp_mb();
	while (m->mnt_flags & MNT_WRITE_HOLD)
		cpu_relax();
	 
	smp_rmb();
	if (__mnt_is_readonly(m)) {
		mnt_dec_writers(m);
		ret = -EROFS;
		goto out;
	}
out:
	preempt_enable();

	return ret;
}

int mnt_want_write(struct vfsmount *m)
{
	int ret;

	sb_start_write(m->mnt_sb);
	ret = __mnt_want_write(m);
	if (ret)
		sb_end_write(m->mnt_sb);
	return ret;
}
EXPORT_SYMBOL_GPL(mnt_want_write);

int mnt_clone_write(struct vfsmount *mnt)
{
	 
	if (__mnt_is_readonly(mnt))
		return -EROFS;
	preempt_disable();
	mnt_inc_writers(mnt);
	preempt_enable();
	return 0;
}
EXPORT_SYMBOL_GPL(mnt_clone_write);

int __mnt_want_write_file(struct file *file)
{
	struct inode *inode = file->f_dentry->d_inode;

	if (!(file->f_mode & FMODE_WRITE) || special_file(inode->i_mode))
		return __mnt_want_write(file->f_path.mnt);
	else
		return mnt_clone_write(file->f_path.mnt);
}

int mnt_want_write_file(struct file *file)
{
	int ret;

	sb_start_write(file->f_path.mnt->mnt_sb);
	ret = __mnt_want_write_file(file);
	if (ret)
		sb_end_write(file->f_path.mnt->mnt_sb);
	return ret;
}
EXPORT_SYMBOL_GPL(mnt_want_write_file);

void __mnt_drop_write(struct vfsmount *mnt)
{
	preempt_disable();
	mnt_dec_writers(mnt);
	preempt_enable();
}

void mnt_drop_write(struct vfsmount *mnt)
{
	__mnt_drop_write(mnt);
	sb_end_write(mnt->mnt_sb);
}
EXPORT_SYMBOL_GPL(mnt_drop_write);

void __mnt_drop_write_file(struct file *file)
{
	__mnt_drop_write(file->f_path.mnt);
}

void mnt_drop_write_file(struct file *file)
{
	mnt_drop_write(file->f_path.mnt);
}
EXPORT_SYMBOL(mnt_drop_write_file);

static int mnt_make_readonly(struct vfsmount *mnt)
{
	int ret = 0;

	br_write_lock(vfsmount_lock);
	mnt->mnt_flags |= MNT_WRITE_HOLD;
	 
	smp_mb();

	if (mnt_get_writers(mnt) > 0)
		ret = -EBUSY;
	else
		mnt->mnt_flags |= MNT_READONLY;
	 
	smp_wmb();
	mnt->mnt_flags &= ~MNT_WRITE_HOLD;
	br_write_unlock(vfsmount_lock);
	return ret;
}

static void __mnt_unmake_readonly(struct vfsmount *mnt)
{
	br_write_lock(vfsmount_lock);
	mnt->mnt_flags &= ~MNT_READONLY;
	br_write_unlock(vfsmount_lock);
}

static void free_vfsmnt(struct vfsmount *mnt)
{
	kfree(mnt->mnt_devname);
	mnt_free_id(mnt);
#ifdef CONFIG_SMP
	free_percpu(mnt->mnt_pcp);
#endif
	kmem_cache_free(mnt_cache, mnt);
}

struct vfsmount *__lookup_mnt(struct vfsmount *mnt, struct dentry *dentry,
			      int dir)
{
	struct list_head *head = mount_hashtable + hash(mnt, dentry);
	struct list_head *tmp = head;
	struct vfsmount *p, *found = NULL;

	for (;;) {
		tmp = dir ? tmp->next : tmp->prev;
		p = NULL;
		if (tmp == head)
			break;
		p = list_entry(tmp, struct vfsmount, mnt_hash);
		if (p->mnt_parent == mnt && p->mnt_mountpoint == dentry) {
			found = p;
			break;
		}
	}
	return found;
}

struct vfsmount *lookup_mnt(struct path *path)
{
	struct vfsmount *child_mnt;

	br_read_lock(vfsmount_lock);
	if ((child_mnt = __lookup_mnt(path->mnt, path->dentry, 1)))
		mntget(child_mnt);
	br_read_unlock(vfsmount_lock);
	return child_mnt;
}

static inline int check_mnt(struct vfsmount *mnt)
{
	return mnt->mnt_ns == current->nsproxy->mnt_ns;
}

static void touch_mnt_namespace(struct mnt_namespace *ns)
{
	if (ns) {
		ns->event = ++event;
		wake_up_interruptible(&ns->poll);
	}
}

static void __touch_mnt_namespace(struct mnt_namespace *ns)
{
	if (ns && ns->event != event) {
		ns->event = event;
		wake_up_interruptible(&ns->poll);
	}
}

static void dentry_reset_mounted(struct vfsmount *mnt, struct dentry *dentry)
{
	unsigned u;

	for (u = 0; u < HASH_SIZE; u++) {
		struct vfsmount *p;

		list_for_each_entry(p, &mount_hashtable[u], mnt_hash) {
			if (p->mnt_mountpoint == dentry)
				return;
		}
	}
	spin_lock(&dentry->d_lock);
	dentry->d_flags &= ~DCACHE_MOUNTED;
	spin_unlock(&dentry->d_lock);
}

static void detach_mnt(struct vfsmount *mnt, struct path *old_path)
{
	old_path->dentry = mnt->mnt_mountpoint;
	old_path->mnt = mnt->mnt_parent;
	mnt->mnt_parent = mnt;
	mnt->mnt_mountpoint = mnt->mnt_root;
	list_del_init(&mnt->mnt_child);
	list_del_init(&mnt->mnt_hash);
	dentry_reset_mounted(old_path->mnt, old_path->dentry);
}

void mnt_set_mountpoint(struct vfsmount *mnt, struct dentry *dentry,
			struct vfsmount *child_mnt)
{
	child_mnt->mnt_parent = mntget(mnt);
	child_mnt->mnt_mountpoint = dget(dentry);
	spin_lock(&dentry->d_lock);
	dentry->d_flags |= DCACHE_MOUNTED;
	spin_unlock(&dentry->d_lock);
}

static void attach_mnt(struct vfsmount *mnt, struct path *path)
{
	mnt_set_mountpoint(path->mnt, path->dentry, mnt);
	list_add_tail(&mnt->mnt_hash, mount_hashtable +
			hash(path->mnt, path->dentry));
	list_add_tail(&mnt->mnt_child, &path->mnt->mnt_mounts);
}

static inline void __mnt_make_longterm(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	atomic_inc(&mnt->mnt_longterm);
#endif
}

static inline void __mnt_make_shortterm(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	atomic_dec(&mnt->mnt_longterm);
#endif
}

static void commit_tree(struct vfsmount *mnt)
{
	struct vfsmount *parent = mnt->mnt_parent;
	struct vfsmount *m;
	LIST_HEAD(head);
	struct mnt_namespace *n = parent->mnt_ns;

	BUG_ON(parent == mnt);

	list_add_tail(&head, &mnt->mnt_list);
	list_for_each_entry(m, &head, mnt_list) {
		m->mnt_ns = n;
		__mnt_make_longterm(m);
	}

	list_splice(&head, n->list.prev);

	list_add_tail(&mnt->mnt_hash, mount_hashtable +
				hash(parent, mnt->mnt_mountpoint));
	list_add_tail(&mnt->mnt_child, &parent->mnt_mounts);
	touch_mnt_namespace(n);
}

static struct vfsmount *next_mnt(struct vfsmount *p, struct vfsmount *root)
{
	struct list_head *next = p->mnt_mounts.next;
	if (next == &p->mnt_mounts) {
		while (1) {
			if (p == root)
				return NULL;
			next = p->mnt_child.next;
			if (next != &p->mnt_parent->mnt_mounts)
				break;
			p = p->mnt_parent;
		}
	}
	return list_entry(next, struct vfsmount, mnt_child);
}

static struct vfsmount *skip_mnt_tree(struct vfsmount *p)
{
	struct list_head *prev = p->mnt_mounts.prev;
	while (prev != &p->mnt_mounts) {
		p = list_entry(prev, struct vfsmount, mnt_child);
		prev = p->mnt_mounts.prev;
	}
	return p;
}

struct vfsmount *
vfs_kern_mount(struct file_system_type *type, int flags, const char *name, void *data)
{
	struct vfsmount *mnt;
	struct dentry *root;

	if (!type)
		return ERR_PTR(-ENODEV);

	mnt = alloc_vfsmnt(name);
	if (!mnt)
		return ERR_PTR(-ENOMEM);

	if (flags & MS_KERNMOUNT)
		mnt->mnt_flags = MNT_INTERNAL;

	root = mount_fs(type, flags, name, data);
	if (IS_ERR(root)) {
		free_vfsmnt(mnt);
		return ERR_CAST(root);
	}

	mnt->mnt_root = root;
	mnt->mnt_sb = root->d_sb;
	mnt->mnt_mountpoint = mnt->mnt_root;
	mnt->mnt_parent = mnt;
	return mnt;
}
EXPORT_SYMBOL_GPL(vfs_kern_mount);

static struct vfsmount *clone_mnt(struct vfsmount *old, struct dentry *root,
					int flag)
{
	struct super_block *sb = old->mnt_sb;
	struct vfsmount *mnt = alloc_vfsmnt(old->mnt_devname);

	if (mnt) {
		if (flag & (CL_SLAVE | CL_PRIVATE))
			mnt->mnt_group_id = 0;  
		else
			mnt->mnt_group_id = old->mnt_group_id;

		if ((flag & CL_MAKE_SHARED) && !mnt->mnt_group_id) {
			int err = mnt_alloc_group_id(mnt);
			if (err)
				goto out_free;
		}

		mnt->mnt_flags = old->mnt_flags & ~MNT_WRITE_HOLD;
		atomic_inc(&sb->s_active);
		mnt->mnt_sb = sb;
		mnt->mnt_root = dget(root);
		mnt->mnt_mountpoint = mnt->mnt_root;
		mnt->mnt_parent = mnt;

		if (flag & CL_SLAVE) {
			list_add(&mnt->mnt_slave, &old->mnt_slave_list);
			mnt->mnt_master = old;
			CLEAR_MNT_SHARED(mnt);
		} else if (!(flag & CL_PRIVATE)) {
			if ((flag & CL_MAKE_SHARED) || IS_MNT_SHARED(old))
				list_add(&mnt->mnt_share, &old->mnt_share);
			if (IS_MNT_SLAVE(old))
				list_add(&mnt->mnt_slave, &old->mnt_slave);
			mnt->mnt_master = old->mnt_master;
		}
		if (flag & CL_MAKE_SHARED)
			set_mnt_shared(mnt);

		if (flag & CL_EXPIRE) {
			if (!list_empty(&old->mnt_expire))
				list_add(&mnt->mnt_expire, &old->mnt_expire);
		}
	}
	return mnt;

 out_free:
	free_vfsmnt(mnt);
	return NULL;
}

static inline void mntfree(struct vfsmount *mnt)
{
	struct super_block *sb = mnt->mnt_sb;

	WARN_ON(mnt_get_writers(mnt));
	fsnotify_vfsmount_delete(mnt);
	dput(mnt->mnt_root);
	free_vfsmnt(mnt);
	deactivate_super(sb);
}

static void mntput_no_expire(struct vfsmount *mnt)
{
put_again:
#ifdef CONFIG_SMP
	br_read_lock(vfsmount_lock);
	if (likely(atomic_read(&mnt->mnt_longterm))) {
		mnt_dec_count(mnt);
		br_read_unlock(vfsmount_lock);
		return;
	}
	br_read_unlock(vfsmount_lock);

	br_write_lock(vfsmount_lock);
	mnt_dec_count(mnt);
	if (mnt_get_count(mnt)) {
		br_write_unlock(vfsmount_lock);
		return;
	}
#else
	mnt_dec_count(mnt);
	if (likely(mnt_get_count(mnt)))
		return;
	br_write_lock(vfsmount_lock);
#endif
	if (unlikely(mnt->mnt_pinned)) {
		mnt_add_count(mnt, mnt->mnt_pinned + 1);
		mnt->mnt_pinned = 0;
		br_write_unlock(vfsmount_lock);
		acct_auto_close_mnt(mnt);
		goto put_again;
	}
	br_write_unlock(vfsmount_lock);
	mntfree(mnt);
}

void mntput(struct vfsmount *mnt)
{
	if (mnt) {
		 
		if (unlikely(mnt->mnt_expiry_mark))
			mnt->mnt_expiry_mark = 0;
		mntput_no_expire(mnt);
	}
}
EXPORT_SYMBOL(mntput);

struct vfsmount *mntget(struct vfsmount *mnt)
{
	if (mnt)
		mnt_inc_count(mnt);
	return mnt;
}
EXPORT_SYMBOL(mntget);

void mnt_pin(struct vfsmount *mnt)
{
	br_write_lock(vfsmount_lock);
	mnt->mnt_pinned++;
	br_write_unlock(vfsmount_lock);
}
EXPORT_SYMBOL(mnt_pin);

void mnt_unpin(struct vfsmount *mnt)
{
	br_write_lock(vfsmount_lock);
	if (mnt->mnt_pinned) {
		mnt_inc_count(mnt);
		mnt->mnt_pinned--;
	}
	br_write_unlock(vfsmount_lock);
}
EXPORT_SYMBOL(mnt_unpin);

static inline void mangle(struct seq_file *m, const char *s)
{
	seq_escape(m, s, " \t\n\\");
}

int generic_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	const char *options;

	rcu_read_lock();
	options = rcu_dereference(mnt->mnt_sb->s_options);

	if (options != NULL && options[0]) {
		seq_putc(m, ',');
		mangle(m, options);
	}
	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(generic_show_options);

void save_mount_options(struct super_block *sb, char *options)
{
	BUG_ON(sb->s_options);
	rcu_assign_pointer(sb->s_options, kstrdup(options, GFP_KERNEL));
}
EXPORT_SYMBOL(save_mount_options);

void replace_mount_options(struct super_block *sb, char *options)
{
	char *old = sb->s_options;
	rcu_assign_pointer(sb->s_options, options);
	if (old) {
		synchronize_rcu();
		kfree(old);
	}
}
EXPORT_SYMBOL(replace_mount_options);

#ifdef CONFIG_PROC_FS
 
static void *m_start(struct seq_file *m, loff_t *pos)
{
	struct proc_mounts *p = m->private;

	down_read(&namespace_sem);
	return seq_list_start(&p->ns->list, *pos);
}

static void *m_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct proc_mounts *p = m->private;

	return seq_list_next(v, &p->ns->list, pos);
}

static void m_stop(struct seq_file *m, void *v)
{
	up_read(&namespace_sem);
}

int mnt_had_events(struct proc_mounts *p)
{
	struct mnt_namespace *ns = p->ns;
	int res = 0;

	br_read_lock(vfsmount_lock);
	if (p->m.poll_event != ns->event) {
		p->m.poll_event = ns->event;
		res = 1;
	}
	br_read_unlock(vfsmount_lock);

	return res;
}

struct proc_fs_info {
	int flag;
	const char *str;
};

static int show_sb_opts(struct seq_file *m, struct super_block *sb)
{
	static const struct proc_fs_info fs_info[] = {
		{ MS_SYNCHRONOUS, ",sync" },
		{ MS_DIRSYNC, ",dirsync" },
		{ MS_MANDLOCK, ",mand" },
#ifdef MY_ABC_HERE
		{ MS_CORRUPT, ",corrupt" },
#endif
		{ 0, NULL }
	};
	const struct proc_fs_info *fs_infop;

	for (fs_infop = fs_info; fs_infop->flag; fs_infop++) {
		if (sb->s_flags & fs_infop->flag)
			seq_puts(m, fs_infop->str);
	}

	return security_sb_show_options(m, sb);
}

static void show_mnt_opts(struct seq_file *m, struct vfsmount *mnt)
{
	static const struct proc_fs_info mnt_info[] = {
		{ MNT_NOSUID, ",nosuid" },
		{ MNT_NODEV, ",nodev" },
		{ MNT_NOEXEC, ",noexec" },
		{ MNT_NOATIME, ",noatime" },
		{ MNT_NODIRATIME, ",nodiratime" },
		{ MNT_RELATIME, ",relatime" },
		{ 0, NULL }
	};
	const struct proc_fs_info *fs_infop;

	for (fs_infop = mnt_info; fs_infop->flag; fs_infop++) {
		if (mnt->mnt_flags & fs_infop->flag)
			seq_puts(m, fs_infop->str);
	}
#ifdef MY_ABC_HERE
	if ((mnt->mnt_flags & MNT_RELATIME) && mnt->mnt_root->d_sb->relatime_period > 1) {
		seq_printf(m, ",relatime_period=%ld", mnt->mnt_root->d_sb->relatime_period);
	}
#endif  
}

static void show_type(struct seq_file *m, struct super_block *sb)
{
	mangle(m, sb->s_type->name);
	if (sb->s_subtype && sb->s_subtype[0]) {
		seq_putc(m, '.');
		mangle(m, sb->s_subtype);
	}
}

static int show_vfsmnt(struct seq_file *m, void *v)
{
	struct vfsmount *mnt = list_entry(v, struct vfsmount, mnt_list);
	int err = 0;
	struct path mnt_path = { .dentry = mnt->mnt_root, .mnt = mnt };

	if (mnt->mnt_sb->s_op->show_devname) {
		err = mnt->mnt_sb->s_op->show_devname(m, mnt);
		if (err)
			goto out;
	} else {
		mangle(m, mnt->mnt_devname ? mnt->mnt_devname : "none");
	}
	seq_putc(m, ' ');
	seq_path(m, &mnt_path, " \t\n\\");
	seq_putc(m, ' ');
	show_type(m, mnt->mnt_sb);
	seq_puts(m, __mnt_is_readonly(mnt) ? " ro" : " rw");
	err = show_sb_opts(m, mnt->mnt_sb);
	if (err)
		goto out;
	show_mnt_opts(m, mnt);
	if (mnt->mnt_sb->s_op->show_options)
		err = mnt->mnt_sb->s_op->show_options(m, mnt);
	seq_puts(m, " 0 0\n");
out:
	return err;
}

const struct seq_operations mounts_op = {
	.start	= m_start,
	.next	= m_next,
	.stop	= m_stop,
	.show	= show_vfsmnt
};

static int show_mountinfo(struct seq_file *m, void *v)
{
	struct proc_mounts *p = m->private;
	struct vfsmount *mnt = list_entry(v, struct vfsmount, mnt_list);
	struct super_block *sb = mnt->mnt_sb;
	struct path mnt_path = { .dentry = mnt->mnt_root, .mnt = mnt };
	struct path root = p->root;
	int err = 0;

	seq_printf(m, "%i %i %u:%u ", mnt->mnt_id, mnt->mnt_parent->mnt_id,
		   MAJOR(sb->s_dev), MINOR(sb->s_dev));
	if (sb->s_op->show_path)
		err = sb->s_op->show_path(m, mnt);
	else
		seq_dentry(m, mnt->mnt_root, " \t\n\\");
	if (err)
		goto out;
	seq_putc(m, ' ');

	err = seq_path_root(m, &mnt_path, &root, " \t\n\\");
	if (err)
		goto out;

	seq_puts(m, mnt->mnt_flags & MNT_READONLY ? " ro" : " rw");
	show_mnt_opts(m, mnt);

	if (IS_MNT_SHARED(mnt))
		seq_printf(m, " shared:%i", mnt->mnt_group_id);
	if (IS_MNT_SLAVE(mnt)) {
		int master = mnt->mnt_master->mnt_group_id;
		int dom = get_dominating_id(mnt, &p->root);
		seq_printf(m, " master:%i", master);
		if (dom && dom != master)
			seq_printf(m, " propagate_from:%i", dom);
	}
	if (IS_MNT_UNBINDABLE(mnt))
		seq_puts(m, " unbindable");

	seq_puts(m, " - ");
	show_type(m, sb);
	seq_putc(m, ' ');
	if (sb->s_op->show_devname)
		err = sb->s_op->show_devname(m, mnt);
	else
		mangle(m, mnt->mnt_devname ? mnt->mnt_devname : "none");
	if (err)
		goto out;
	seq_puts(m, sb->s_flags & MS_RDONLY ? " ro" : " rw");
	err = show_sb_opts(m, sb);
	if (err)
		goto out;
	if (sb->s_op->show_options)
		err = sb->s_op->show_options(m, mnt);
	seq_putc(m, '\n');
out:
	return err;
}

const struct seq_operations mountinfo_op = {
	.start	= m_start,
	.next	= m_next,
	.stop	= m_stop,
	.show	= show_mountinfo,
};

static int show_vfsstat(struct seq_file *m, void *v)
{
	struct vfsmount *mnt = list_entry(v, struct vfsmount, mnt_list);
	struct path mnt_path = { .dentry = mnt->mnt_root, .mnt = mnt };
	int err = 0;

	if (mnt->mnt_sb->s_op->show_devname) {
		seq_puts(m, "device ");
		err = mnt->mnt_sb->s_op->show_devname(m, mnt);
	} else {
		if (mnt->mnt_devname) {
			seq_puts(m, "device ");
			mangle(m, mnt->mnt_devname);
		} else
			seq_puts(m, "no device");
	}

	seq_puts(m, " mounted on ");
	seq_path(m, &mnt_path, " \t\n\\");
	seq_putc(m, ' ');

	seq_puts(m, "with fstype ");
	show_type(m, mnt->mnt_sb);

	if (mnt->mnt_sb->s_op->show_stats) {
		seq_putc(m, ' ');
		if (!err)
			err = mnt->mnt_sb->s_op->show_stats(m, mnt);
	}

	seq_putc(m, '\n');
	return err;
}

const struct seq_operations mountstats_op = {
	.start	= m_start,
	.next	= m_next,
	.stop	= m_stop,
	.show	= show_vfsstat,
};
#endif   

int may_umount_tree(struct vfsmount *mnt)
{
	int actual_refs = 0;
	int minimum_refs = 0;
	struct vfsmount *p;

	br_write_lock(vfsmount_lock);
	for (p = mnt; p; p = next_mnt(p, mnt)) {
		actual_refs += mnt_get_count(p);
		minimum_refs += 2;
	}
	br_write_unlock(vfsmount_lock);

	if (actual_refs > minimum_refs)
		return 0;

	return 1;
}

EXPORT_SYMBOL(may_umount_tree);

int may_umount(struct vfsmount *mnt)
{
	int ret = 1;
	down_read(&namespace_sem);
	br_write_lock(vfsmount_lock);
	if (propagate_mount_busy(mnt, 2))
		ret = 0;
	br_write_unlock(vfsmount_lock);
	up_read(&namespace_sem);
	return ret;
}

EXPORT_SYMBOL(may_umount);

void release_mounts(struct list_head *head)
{
	struct vfsmount *mnt;
	while (!list_empty(head)) {
		mnt = list_first_entry(head, struct vfsmount, mnt_hash);
		list_del_init(&mnt->mnt_hash);
		if (mnt->mnt_parent != mnt) {
			struct dentry *dentry;
			struct vfsmount *m;

			br_write_lock(vfsmount_lock);
			dentry = mnt->mnt_mountpoint;
			m = mnt->mnt_parent;
			mnt->mnt_mountpoint = mnt->mnt_root;
			mnt->mnt_parent = mnt;
			m->mnt_ghosts--;
			br_write_unlock(vfsmount_lock);
			dput(dentry);
			mntput(m);
		}
		mntput(mnt);
	}
}

void umount_tree(struct vfsmount *mnt, int propagate, struct list_head *kill)
{
	LIST_HEAD(tmp_list);
	struct vfsmount *p;

	for (p = mnt; p; p = next_mnt(p, mnt))
		list_move(&p->mnt_hash, &tmp_list);

	if (propagate)
		propagate_umount(&tmp_list);

	list_for_each_entry(p, &tmp_list, mnt_hash) {
		list_del_init(&p->mnt_expire);
		list_del_init(&p->mnt_list);
		__touch_mnt_namespace(p->mnt_ns);
		if (p->mnt_ns)
			__mnt_make_shortterm(p);
		p->mnt_ns = NULL;
		list_del_init(&p->mnt_child);
		if (p->mnt_parent != p) {
			p->mnt_parent->mnt_ghosts++;
			dentry_reset_mounted(p->mnt_parent, p->mnt_mountpoint);
		}
		change_mnt_propagation(p, MS_PRIVATE);
	}
	list_splice(&tmp_list, kill);
}

static void shrink_submounts(struct vfsmount *mnt, struct list_head *umounts);

static int do_umount(struct vfsmount *mnt, int flags)
{
	struct super_block *sb = mnt->mnt_sb;
	int retval;
	LIST_HEAD(umount_list);

	retval = security_sb_umount(mnt, flags);
	if (retval)
		return retval;

	if (flags & MNT_EXPIRE) {
		if (mnt == current->fs->root.mnt ||
		    flags & (MNT_FORCE | MNT_DETACH))
			return -EINVAL;

		br_write_lock(vfsmount_lock);
		if (mnt_get_count(mnt) != 2) {
			br_write_unlock(vfsmount_lock);
			return -EBUSY;
		}
		br_write_unlock(vfsmount_lock);

		if (!xchg(&mnt->mnt_expiry_mark, 1))
			return -EAGAIN;
	}

	if (flags & MNT_FORCE && sb->s_op->umount_begin) {
		sb->s_op->umount_begin(sb);
	}

	if (mnt == current->fs->root.mnt && !(flags & MNT_DETACH)) {
		 
		down_write(&sb->s_umount);
		if (!(sb->s_flags & MS_RDONLY))
			retval = do_remount_sb(sb, MS_RDONLY, NULL, 0);
		up_write(&sb->s_umount);
		return retval;
	}

	down_write(&namespace_sem);
	br_write_lock(vfsmount_lock);
	event++;

	if (!(flags & MNT_DETACH))
		shrink_submounts(mnt, &umount_list);

	retval = -EBUSY;
	if (flags & MNT_DETACH || !propagate_mount_busy(mnt, 2)) {
		if (!list_empty(&mnt->mnt_list))
			umount_tree(mnt, 1, &umount_list);
		retval = 0;
	}
	br_write_unlock(vfsmount_lock);
	up_write(&namespace_sem);
	release_mounts(&umount_list);
	return retval;
}

SYSCALL_DEFINE2(umount, char __user *, name, int, flags)
{
	struct path path;
	int retval;
	int lookup_flags = 0;

	if (flags & ~(MNT_FORCE | MNT_DETACH | MNT_EXPIRE | UMOUNT_NOFOLLOW))
		return -EINVAL;

	if (!(flags & UMOUNT_NOFOLLOW))
		lookup_flags |= LOOKUP_FOLLOW;

	retval = user_path_at(AT_FDCWD, name, lookup_flags, &path);
	if (retval)
		goto out;
	retval = -EINVAL;
	if (path.dentry != path.mnt->mnt_root)
		goto dput_and_out;
	if (!check_mnt(path.mnt))
		goto dput_and_out;

	retval = -EPERM;
	if (!capable(CAP_SYS_ADMIN))
		goto dput_and_out;

	retval = do_umount(path.mnt, flags);
dput_and_out:
	 
	dput(path.dentry);
	mntput_no_expire(path.mnt);
out:
	return retval;
}

#ifdef __ARCH_WANT_SYS_OLDUMOUNT

SYSCALL_DEFINE1(oldumount, char __user *, name)
{
	return sys_umount(name, 0);
}

#endif

static int mount_is_safe(struct path *path)
{
	if (capable(CAP_SYS_ADMIN))
		return 0;
	return -EPERM;
#ifdef notyet
	if (S_ISLNK(path->dentry->d_inode->i_mode))
		return -EPERM;
	if (path->dentry->d_inode->i_mode & S_ISVTX) {
		if (current_uid() != path->dentry->d_inode->i_uid)
			return -EPERM;
	}
	if (inode_permission(path->dentry->d_inode, MAY_WRITE))
		return -EPERM;
	return 0;
#endif
}

struct vfsmount *copy_tree(struct vfsmount *mnt, struct dentry *dentry,
					int flag)
{
	struct vfsmount *res, *p, *q, *r, *s;
	struct path path;

	if (!(flag & CL_COPY_ALL) && IS_MNT_UNBINDABLE(mnt))
		return NULL;

	res = q = clone_mnt(mnt, dentry, flag);
	if (!q)
		goto Enomem;
	q->mnt_mountpoint = mnt->mnt_mountpoint;

	p = mnt;
	list_for_each_entry(r, &mnt->mnt_mounts, mnt_child) {
		if (!is_subdir(r->mnt_mountpoint, dentry))
			continue;

		for (s = r; s; s = next_mnt(s, r)) {
			if (!(flag & CL_COPY_ALL) && IS_MNT_UNBINDABLE(s)) {
				s = skip_mnt_tree(s);
				continue;
			}
			while (p != s->mnt_parent) {
				p = p->mnt_parent;
				q = q->mnt_parent;
			}
			p = s;
			path.mnt = q;
			path.dentry = p->mnt_mountpoint;
			q = clone_mnt(p, p->mnt_root, flag);
			if (!q)
				goto Enomem;
			br_write_lock(vfsmount_lock);
			list_add_tail(&q->mnt_list, &res->mnt_list);
			attach_mnt(q, &path);
			br_write_unlock(vfsmount_lock);
		}
	}
	return res;
Enomem:
	if (res) {
		LIST_HEAD(umount_list);
		br_write_lock(vfsmount_lock);
		umount_tree(res, 0, &umount_list);
		br_write_unlock(vfsmount_lock);
		release_mounts(&umount_list);
	}
	return NULL;
}

struct vfsmount *collect_mounts(struct path *path)
{
	struct vfsmount *tree;
	down_write(&namespace_sem);
	tree = copy_tree(path->mnt, path->dentry, CL_COPY_ALL | CL_PRIVATE);
	up_write(&namespace_sem);
	return tree;
}

void drop_collected_mounts(struct vfsmount *mnt)
{
	LIST_HEAD(umount_list);
	down_write(&namespace_sem);
	br_write_lock(vfsmount_lock);
	umount_tree(mnt, 0, &umount_list);
	br_write_unlock(vfsmount_lock);
	up_write(&namespace_sem);
	release_mounts(&umount_list);
}

int iterate_mounts(int (*f)(struct vfsmount *, void *), void *arg,
		   struct vfsmount *root)
{
	struct vfsmount *mnt;
	int res = f(root, arg);
	if (res)
		return res;
	list_for_each_entry(mnt, &root->mnt_list, mnt_list) {
		res = f(mnt, arg);
		if (res)
			return res;
	}
	return 0;
}

static void cleanup_group_ids(struct vfsmount *mnt, struct vfsmount *end)
{
	struct vfsmount *p;

	for (p = mnt; p != end; p = next_mnt(p, mnt)) {
		if (p->mnt_group_id && !IS_MNT_SHARED(p))
			mnt_release_group_id(p);
	}
}

static int invent_group_ids(struct vfsmount *mnt, bool recurse)
{
	struct vfsmount *p;

	for (p = mnt; p; p = recurse ? next_mnt(p, mnt) : NULL) {
		if (!p->mnt_group_id && !IS_MNT_SHARED(p)) {
			int err = mnt_alloc_group_id(p);
			if (err) {
				cleanup_group_ids(mnt, p);
				return err;
			}
		}
	}

	return 0;
}

static int attach_recursive_mnt(struct vfsmount *source_mnt,
			struct path *path, struct path *parent_path)
{
	LIST_HEAD(tree_list);
	struct vfsmount *dest_mnt = path->mnt;
	struct dentry *dest_dentry = path->dentry;
	struct vfsmount *child, *p;
	int err;

	if (IS_MNT_SHARED(dest_mnt)) {
		err = invent_group_ids(source_mnt, true);
		if (err)
			goto out;
	}
	err = propagate_mnt(dest_mnt, dest_dentry, source_mnt, &tree_list);
	if (err)
		goto out_cleanup_ids;

	br_write_lock(vfsmount_lock);

	if (IS_MNT_SHARED(dest_mnt)) {
		for (p = source_mnt; p; p = next_mnt(p, source_mnt))
			set_mnt_shared(p);
	}
	if (parent_path) {
		detach_mnt(source_mnt, parent_path);
		attach_mnt(source_mnt, path);
		touch_mnt_namespace(parent_path->mnt->mnt_ns);
	} else {
		mnt_set_mountpoint(dest_mnt, dest_dentry, source_mnt);
		commit_tree(source_mnt);
	}

	list_for_each_entry_safe(child, p, &tree_list, mnt_hash) {
		list_del_init(&child->mnt_hash);
		commit_tree(child);
	}
	br_write_unlock(vfsmount_lock);

	return 0;

 out_cleanup_ids:
	if (IS_MNT_SHARED(dest_mnt))
		cleanup_group_ids(source_mnt, NULL);
 out:
	return err;
}

static int lock_mount(struct path *path)
{
	struct vfsmount *mnt;
retry:
	mutex_lock(&path->dentry->d_inode->i_mutex);
	if (unlikely(cant_mount(path->dentry))) {
		mutex_unlock(&path->dentry->d_inode->i_mutex);
		return -ENOENT;
	}
	down_write(&namespace_sem);
	mnt = lookup_mnt(path);
	if (likely(!mnt))
		return 0;
	up_write(&namespace_sem);
	mutex_unlock(&path->dentry->d_inode->i_mutex);
	path_put(path);
	path->mnt = mnt;
	path->dentry = dget(mnt->mnt_root);
	goto retry;
}

static void unlock_mount(struct path *path)
{
	up_write(&namespace_sem);
	mutex_unlock(&path->dentry->d_inode->i_mutex);
}

static int graft_tree(struct vfsmount *mnt, struct path *path)
{
	if (mnt->mnt_sb->s_flags & MS_NOUSER)
		return -EINVAL;

	if (S_ISDIR(path->dentry->d_inode->i_mode) !=
	      S_ISDIR(mnt->mnt_root->d_inode->i_mode))
		return -ENOTDIR;

	if (d_unlinked(path->dentry))
		return -ENOENT;

	return attach_recursive_mnt(mnt, path, NULL);
}

static int flags_to_propagation_type(int flags)
{
	int type = flags & ~(MS_REC | MS_SILENT);

	if (type & ~(MS_SHARED | MS_PRIVATE | MS_SLAVE | MS_UNBINDABLE))
		return 0;
	 
	if (!is_power_of_2(type))
		return 0;
	return type;
}

static int do_change_type(struct path *path, int flag)
{
	struct vfsmount *m, *mnt = path->mnt;
	int recurse = flag & MS_REC;
	int type;
	int err = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (path->dentry != path->mnt->mnt_root)
		return -EINVAL;

	type = flags_to_propagation_type(flag);
	if (!type)
		return -EINVAL;

	down_write(&namespace_sem);
	if (type == MS_SHARED) {
		err = invent_group_ids(mnt, recurse);
		if (err)
			goto out_unlock;
	}

	br_write_lock(vfsmount_lock);
	for (m = mnt; m; m = (recurse ? next_mnt(m, mnt) : NULL))
		change_mnt_propagation(m, type);
	br_write_unlock(vfsmount_lock);

 out_unlock:
	up_write(&namespace_sem);
	return err;
}

static int do_loopback(struct path *path, char *old_name,
				int recurse)
{
	LIST_HEAD(umount_list);
	struct path old_path;
	struct vfsmount *mnt = NULL;
	int err = mount_is_safe(path);
	if (err)
		return err;
	if (!old_name || !*old_name)
		return -EINVAL;
	err = kern_path(old_name, LOOKUP_FOLLOW|LOOKUP_AUTOMOUNT, &old_path);
	if (err)
		return err;

	err = lock_mount(path);
	if (err)
		goto out;

	err = -EINVAL;
	if (IS_MNT_UNBINDABLE(old_path.mnt))
		goto out2;

	if (!check_mnt(path->mnt) || !check_mnt(old_path.mnt))
		goto out2;

	err = -ENOMEM;
	if (recurse)
		mnt = copy_tree(old_path.mnt, old_path.dentry, 0);
	else
		mnt = clone_mnt(old_path.mnt, old_path.dentry, 0);

	if (!mnt)
		goto out2;

	err = graft_tree(mnt, path);
	if (err) {
		br_write_lock(vfsmount_lock);
		umount_tree(mnt, 0, &umount_list);
		br_write_unlock(vfsmount_lock);
	}
out2:
	unlock_mount(path);
	release_mounts(&umount_list);
out:
	path_put(&old_path);
	return err;
}

static int change_mount_flags(struct vfsmount *mnt, int ms_flags)
{
	int error = 0;
	int readonly_request = 0;

	if (ms_flags & MS_RDONLY)
		readonly_request = 1;
	if (readonly_request == __mnt_is_readonly(mnt))
		return 0;

	if (readonly_request)
		error = mnt_make_readonly(mnt);
	else
		__mnt_unmake_readonly(mnt);
	return error;
}

#ifdef MY_ABC_HERE
struct syno_mnt_options {
	long relatime_period;
};

static void option_erase(char *str) {
	char *next = strchr(str, ',');

	if (next) {
		next++;
		while (*next) {
			*str = *next;
			str++;
			next++;
		}
	}
	while (*str) {
		*str = '\0';
		str++;
	}
}

enum {
	Opt_relatime_period,
	Opt_err,
};

static struct option_table {
	int token;
	const char *name;
	const char *pattern;
} tokens[] = {
	{Opt_relatime_period, "relatime_period", "relatime_period=%ld"},
	{Opt_err, NULL, NULL},
};

static int syno_parse_options(struct syno_mnt_options *options, char *data) {
	struct option_table *p;
	char *str;

	if (!data)
		return 0;

	for (p = tokens; p->token != Opt_err; p++) {
		while (NULL != (str = strstr(data, p->name))) {
			switch (p->token) {
				case Opt_relatime_period:
					if (1 != sscanf(str, p->pattern, &(options->relatime_period))
						|| options->relatime_period <= 0 || options->relatime_period > 365*10)
						return -EINVAL;
					break;
				default:
					break;
			}
			option_erase(str);
		}
	}
	return 0;
}
#endif  

static int do_remount(struct path *path, int flags, int mnt_flags,
		      void *data)
{
	int err;
	struct super_block *sb = path->mnt->mnt_sb;
#ifdef MY_ABC_HERE
	struct syno_mnt_options options;
#endif  

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!check_mnt(path->mnt))
		return -EINVAL;

	if (path->dentry != path->mnt->mnt_root)
		return -EINVAL;

#ifdef MY_ABC_HERE
	memset(&options, 0, sizeof(struct syno_mnt_options));
	err = syno_parse_options(&options, data);
	if (err)
		return -EINVAL;
#endif  

	err = security_sb_remount(sb, data);
	if (err)
		return err;

	down_write(&sb->s_umount);
	if (flags & MS_BIND)
		err = change_mount_flags(path->mnt, flags);
	else
		err = do_remount_sb(sb, flags, data, 0);
	if (!err) {
		br_write_lock(vfsmount_lock);
		mnt_flags |= path->mnt->mnt_flags & MNT_PROPAGATION_MASK;
		path->mnt->mnt_flags = mnt_flags;
		br_write_unlock(vfsmount_lock);
	}
#ifdef MY_ABC_HERE
	if (options.relatime_period > 0)
		sb->relatime_period = options.relatime_period;
#endif  
	up_write(&sb->s_umount);
	if (!err) {
		br_write_lock(vfsmount_lock);
		touch_mnt_namespace(path->mnt->mnt_ns);
		br_write_unlock(vfsmount_lock);
	}
	return err;
}

static inline int tree_contains_unbindable(struct vfsmount *mnt)
{
	struct vfsmount *p;
	for (p = mnt; p; p = next_mnt(p, mnt)) {
		if (IS_MNT_UNBINDABLE(p))
			return 1;
	}
	return 0;
}

static int do_move_mount(struct path *path, char *old_name)
{
	struct path old_path, parent_path;
	struct vfsmount *p;
	int err = 0;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (!old_name || !*old_name)
		return -EINVAL;
	err = kern_path(old_name, LOOKUP_FOLLOW, &old_path);
	if (err)
		return err;

	err = lock_mount(path);
	if (err < 0)
		goto out;

	err = -EINVAL;
	if (!check_mnt(path->mnt) || !check_mnt(old_path.mnt))
		goto out1;

	if (d_unlinked(path->dentry))
		goto out1;

	err = -EINVAL;
	if (old_path.dentry != old_path.mnt->mnt_root)
		goto out1;

	if (old_path.mnt == old_path.mnt->mnt_parent)
		goto out1;

	if (S_ISDIR(path->dentry->d_inode->i_mode) !=
	      S_ISDIR(old_path.dentry->d_inode->i_mode))
		goto out1;
	 
	if (old_path.mnt->mnt_parent &&
	    IS_MNT_SHARED(old_path.mnt->mnt_parent))
		goto out1;
	 
	if (IS_MNT_SHARED(path->mnt) &&
	    tree_contains_unbindable(old_path.mnt))
		goto out1;
	err = -ELOOP;
	for (p = path->mnt; p->mnt_parent != p; p = p->mnt_parent)
		if (p == old_path.mnt)
			goto out1;

	err = attach_recursive_mnt(old_path.mnt, path, &parent_path);
	if (err)
		goto out1;

	list_del_init(&old_path.mnt->mnt_expire);
out1:
	unlock_mount(path);
out:
	if (!err)
		path_put(&parent_path);
	path_put(&old_path);
	return err;
}

static struct vfsmount *fs_set_subtype(struct vfsmount *mnt, const char *fstype)
{
	int err;
	const char *subtype = strchr(fstype, '.');
	if (subtype) {
		subtype++;
		err = -EINVAL;
		if (!subtype[0])
			goto err;
	} else
		subtype = "";

	mnt->mnt_sb->s_subtype = kstrdup(subtype, GFP_KERNEL);
	err = -ENOMEM;
	if (!mnt->mnt_sb->s_subtype)
		goto err;
	return mnt;

 err:
	mntput(mnt);
	return ERR_PTR(err);
}

struct vfsmount *
do_kern_mount(const char *fstype, int flags, const char *name, void *data)
{
	struct file_system_type *type = get_fs_type(fstype);
	struct vfsmount *mnt;
	if (!type)
		return ERR_PTR(-ENODEV);
	mnt = vfs_kern_mount(type, flags, name, data);
	if (!IS_ERR(mnt) && (type->fs_flags & FS_HAS_SUBTYPE) &&
	    !mnt->mnt_sb->s_subtype)
		mnt = fs_set_subtype(mnt, fstype);
	put_filesystem(type);
	return mnt;
}
EXPORT_SYMBOL_GPL(do_kern_mount);

static int do_add_mount(struct vfsmount *newmnt, struct path *path, int mnt_flags)
{
	int err;

	mnt_flags &= ~(MNT_SHARED | MNT_WRITE_HOLD | MNT_INTERNAL);

	err = lock_mount(path);
	if (err)
		return err;

	err = -EINVAL;
	if (!(mnt_flags & MNT_SHRINKABLE) && !check_mnt(path->mnt))
		goto unlock;

	err = -EBUSY;
	if (path->mnt->mnt_sb == newmnt->mnt_sb &&
	    path->mnt->mnt_root == path->dentry)
		goto unlock;

	err = -EINVAL;
	if (S_ISLNK(newmnt->mnt_root->d_inode->i_mode))
		goto unlock;

	newmnt->mnt_flags = mnt_flags;
	err = graft_tree(newmnt, path);

unlock:
	unlock_mount(path);
	return err;
}

static int do_new_mount(struct path *path, char *type, int flags,
			int mnt_flags, char *name, void *data)
{
	struct vfsmount *mnt;
	int err;
#ifdef MY_ABC_HERE
	struct syno_mnt_options options;
#endif  

	if (!type)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

#ifdef MY_ABC_HERE
	memset(&options, 0, sizeof(struct syno_mnt_options));
	err = syno_parse_options(&options, data);
	if (err)
		return -EINVAL;
#endif  

	mnt = do_kern_mount(type, flags, name, data);
	if (IS_ERR(mnt))
		return PTR_ERR(mnt);

	err = do_add_mount(mnt, path, mnt_flags);
#ifdef MY_ABC_HERE
	if (!strcmp(type, "ext4")) {
		char buf[SYNO_EXT4_MOUNT_PATH_LEN] = {'\0'};
		ext4_fill_mount_path(mnt->mnt_sb, d_path(path, buf, sizeof(buf)));
	}
#endif

#ifdef MY_ABC_HERE
	if (options.relatime_period > 0)
		mnt->mnt_sb->relatime_period = options.relatime_period;
#endif  

	if (err)
		mntput(mnt);
	return err;
}

int finish_automount(struct vfsmount *m, struct path *path)
{
	int err;
	 
	BUG_ON(mnt_get_count(m) < 2);

	if (m->mnt_sb == path->mnt->mnt_sb &&
	    m->mnt_root == path->dentry) {
		err = -ELOOP;
		goto fail;
	}

	err = do_add_mount(m, path, path->mnt->mnt_flags | MNT_SHRINKABLE);
	if (!err)
		return 0;
fail:
	 
	if (!list_empty(&m->mnt_expire)) {
		down_write(&namespace_sem);
		br_write_lock(vfsmount_lock);
		list_del_init(&m->mnt_expire);
		br_write_unlock(vfsmount_lock);
		up_write(&namespace_sem);
	}
	mntput(m);
	mntput(m);
	return err;
}

void mnt_set_expiry(struct vfsmount *mnt, struct list_head *expiry_list)
{
	down_write(&namespace_sem);
	br_write_lock(vfsmount_lock);

	list_add_tail(&mnt->mnt_expire, expiry_list);

	br_write_unlock(vfsmount_lock);
	up_write(&namespace_sem);
}
EXPORT_SYMBOL(mnt_set_expiry);

void mark_mounts_for_expiry(struct list_head *mounts)
{
	struct vfsmount *mnt, *next;
	LIST_HEAD(graveyard);
	LIST_HEAD(umounts);

	if (list_empty(mounts))
		return;

	down_write(&namespace_sem);
	br_write_lock(vfsmount_lock);

	list_for_each_entry_safe(mnt, next, mounts, mnt_expire) {
		if (!xchg(&mnt->mnt_expiry_mark, 1) ||
			propagate_mount_busy(mnt, 1))
			continue;
		list_move(&mnt->mnt_expire, &graveyard);
	}
	while (!list_empty(&graveyard)) {
		mnt = list_first_entry(&graveyard, struct vfsmount, mnt_expire);
		touch_mnt_namespace(mnt->mnt_ns);
		umount_tree(mnt, 1, &umounts);
	}
	br_write_unlock(vfsmount_lock);
	up_write(&namespace_sem);

	release_mounts(&umounts);
}

EXPORT_SYMBOL_GPL(mark_mounts_for_expiry);

static int select_submounts(struct vfsmount *parent, struct list_head *graveyard)
{
	struct vfsmount *this_parent = parent;
	struct list_head *next;
	int found = 0;

repeat:
	next = this_parent->mnt_mounts.next;
resume:
	while (next != &this_parent->mnt_mounts) {
		struct list_head *tmp = next;
		struct vfsmount *mnt = list_entry(tmp, struct vfsmount, mnt_child);

		next = tmp->next;
		if (!(mnt->mnt_flags & MNT_SHRINKABLE))
			continue;
		 
		if (!list_empty(&mnt->mnt_mounts)) {
			this_parent = mnt;
			goto repeat;
		}

		if (!propagate_mount_busy(mnt, 1)) {
			list_move_tail(&mnt->mnt_expire, graveyard);
			found++;
		}
	}
	 
	if (this_parent != parent) {
		next = this_parent->mnt_child.next;
		this_parent = this_parent->mnt_parent;
		goto resume;
	}
	return found;
}

static void shrink_submounts(struct vfsmount *mnt, struct list_head *umounts)
{
	LIST_HEAD(graveyard);
	struct vfsmount *m;

	while (select_submounts(mnt, &graveyard)) {
		while (!list_empty(&graveyard)) {
			m = list_first_entry(&graveyard, struct vfsmount,
						mnt_expire);
			touch_mnt_namespace(m->mnt_ns);
			umount_tree(m, 1, umounts);
		}
	}
}

static long exact_copy_from_user(void *to, const void __user * from,
				 unsigned long n)
{
	char *t = to;
	const char __user *f = from;
	char c;

	if (!access_ok(VERIFY_READ, from, n))
		return n;

	while (n) {
		if (__get_user(c, f)) {
			memset(t, 0, n);
			break;
		}
		*t++ = c;
		f++;
		n--;
	}
	return n;
}

int copy_mount_options(const void __user * data, unsigned long *where)
{
	int i;
	unsigned long page;
	unsigned long size;

	*where = 0;
	if (!data)
		return 0;

	if (!(page = __get_free_page(GFP_KERNEL)))
		return -ENOMEM;

	size = TASK_SIZE - (unsigned long)data;
	if (size > PAGE_SIZE)
		size = PAGE_SIZE;

	i = size - exact_copy_from_user((void *)page, data, size);
	if (!i) {
		free_page(page);
		return -EFAULT;
	}
	if (i != PAGE_SIZE)
		memset((char *)page + i, 0, PAGE_SIZE - i);
	*where = page;
	return 0;
}

int copy_mount_string(const void __user *data, char **where)
{
	char *tmp;

	if (!data) {
		*where = NULL;
		return 0;
	}

	tmp = strndup_user(data, PAGE_SIZE);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	*where = tmp;
	return 0;
}

long do_mount(char *dev_name, char *dir_name, char *type_page,
		  unsigned long flags, void *data_page)
{
	struct path path;
	int retval = 0;
	int mnt_flags = 0;
#if defined(MY_ABC_HERE)
	extern int gSynoInstallFlag;
	if ( 0 == gSynoInstallFlag &&
			NULL != dev_name &&
			strstr(dev_name, SYNO_USB_FLASH_DEVICE_PATH) &&
			gSynoHasDynModule) {
		return -EINVAL;
	}
#endif

	if ((flags & MS_MGC_MSK) == MS_MGC_VAL)
		flags &= ~MS_MGC_MSK;

	if (!dir_name || !*dir_name || !memchr(dir_name, 0, PAGE_SIZE))
		return -EINVAL;

	if (data_page)
		((char *)data_page)[PAGE_SIZE - 1] = 0;

	retval = kern_path(dir_name, LOOKUP_FOLLOW, &path);
	if (retval)
		return retval;

	retval = security_sb_mount(dev_name, &path,
				   type_page, flags, data_page);
	if (retval)
		goto dput_out;

	if (!(flags & MS_NOATIME))
		mnt_flags |= MNT_RELATIME;

	if (flags & MS_NOSUID)
		mnt_flags |= MNT_NOSUID;
	if (flags & MS_NODEV)
		mnt_flags |= MNT_NODEV;
	if (flags & MS_NOEXEC)
		mnt_flags |= MNT_NOEXEC;
	if (flags & MS_NOATIME)
		mnt_flags |= MNT_NOATIME;
	if (flags & MS_NODIRATIME)
		mnt_flags |= MNT_NODIRATIME;
	if (flags & MS_STRICTATIME)
		mnt_flags &= ~(MNT_RELATIME | MNT_NOATIME);
	if (flags & MS_RDONLY)
		mnt_flags |= MNT_READONLY;

	flags &= ~(MS_NOSUID | MS_NOEXEC | MS_NODEV | MS_ACTIVE | MS_BORN |
		   MS_NOATIME | MS_NODIRATIME | MS_RELATIME| MS_KERNMOUNT |
		   MS_STRICTATIME);

	if (flags & MS_REMOUNT)
		retval = do_remount(&path, flags & ~MS_REMOUNT, mnt_flags,
				    data_page);
	else if (flags & MS_BIND)
		retval = do_loopback(&path, dev_name, flags & MS_REC);
	else if (flags & (MS_SHARED | MS_PRIVATE | MS_SLAVE | MS_UNBINDABLE))
		retval = do_change_type(&path, flags);
	else if (flags & MS_MOVE)
		retval = do_move_mount(&path, dev_name);
	else
		retval = do_new_mount(&path, type_page, flags, mnt_flags,
				      dev_name, data_page);

dput_out:
	path_put(&path);
	return retval;
}

static struct mnt_namespace *alloc_mnt_ns(void)
{
	struct mnt_namespace *new_ns;

	new_ns = kmalloc(sizeof(struct mnt_namespace), GFP_KERNEL);
	if (!new_ns)
		return ERR_PTR(-ENOMEM);
	atomic_set(&new_ns->count, 1);
	new_ns->root = NULL;
	INIT_LIST_HEAD(&new_ns->list);
	init_waitqueue_head(&new_ns->poll);
	new_ns->event = 0;
	return new_ns;
}

void mnt_make_longterm(struct vfsmount *mnt)
{
	__mnt_make_longterm(mnt);
}

void mnt_make_shortterm(struct vfsmount *mnt)
{
#ifdef CONFIG_SMP
	if (atomic_add_unless(&mnt->mnt_longterm, -1, 1))
		return;
	br_write_lock(vfsmount_lock);
	atomic_dec(&mnt->mnt_longterm);
	br_write_unlock(vfsmount_lock);
#endif
}

static struct mnt_namespace *dup_mnt_ns(struct mnt_namespace *mnt_ns,
		struct fs_struct *fs)
{
	struct mnt_namespace *new_ns;
	struct vfsmount *rootmnt = NULL, *pwdmnt = NULL;
	struct vfsmount *p, *q;

	new_ns = alloc_mnt_ns();
	if (IS_ERR(new_ns))
		return new_ns;

	down_write(&namespace_sem);
	 
	new_ns->root = copy_tree(mnt_ns->root, mnt_ns->root->mnt_root,
					CL_COPY_ALL | CL_EXPIRE);
	if (!new_ns->root) {
		up_write(&namespace_sem);
		kfree(new_ns);
		return ERR_PTR(-ENOMEM);
	}
	br_write_lock(vfsmount_lock);
	list_add_tail(&new_ns->list, &new_ns->root->mnt_list);
	br_write_unlock(vfsmount_lock);

	p = mnt_ns->root;
	q = new_ns->root;
	while (p) {
		q->mnt_ns = new_ns;
		__mnt_make_longterm(q);
		if (fs) {
			if (p == fs->root.mnt) {
				fs->root.mnt = mntget(q);
				__mnt_make_longterm(q);
				mnt_make_shortterm(p);
				rootmnt = p;
			}
			if (p == fs->pwd.mnt) {
				fs->pwd.mnt = mntget(q);
				__mnt_make_longterm(q);
				mnt_make_shortterm(p);
				pwdmnt = p;
			}
		}
		p = next_mnt(p, mnt_ns->root);
		q = next_mnt(q, new_ns->root);
	}
	up_write(&namespace_sem);

	if (rootmnt)
		mntput(rootmnt);
	if (pwdmnt)
		mntput(pwdmnt);

	return new_ns;
}

struct mnt_namespace *copy_mnt_ns(unsigned long flags, struct mnt_namespace *ns,
		struct fs_struct *new_fs)
{
	struct mnt_namespace *new_ns;

	BUG_ON(!ns);
	get_mnt_ns(ns);

	if (!(flags & CLONE_NEWNS))
		return ns;

	new_ns = dup_mnt_ns(ns, new_fs);

	put_mnt_ns(ns);
	return new_ns;
}

struct mnt_namespace *create_mnt_ns(struct vfsmount *mnt)
{
	struct mnt_namespace *new_ns;

	new_ns = alloc_mnt_ns();
	if (!IS_ERR(new_ns)) {
		mnt->mnt_ns = new_ns;
		__mnt_make_longterm(mnt);
		new_ns->root = mnt;
		list_add(&new_ns->list, &new_ns->root->mnt_list);
	} else {
		mntput(mnt);
	}
	return new_ns;
}
EXPORT_SYMBOL(create_mnt_ns);

struct dentry *mount_subtree(struct vfsmount *mnt, const char *name)
{
	struct mnt_namespace *ns;
	struct super_block *s;
	struct path path;
	int err;

	ns = create_mnt_ns(mnt);
	if (IS_ERR(ns))
		return ERR_CAST(ns);

	err = vfs_path_lookup(mnt->mnt_root, mnt,
			name, LOOKUP_FOLLOW|LOOKUP_AUTOMOUNT, &path);

	put_mnt_ns(ns);

	if (err)
		return ERR_PTR(err);

	s = path.mnt->mnt_sb;
	atomic_inc(&s->s_active);
	mntput(path.mnt);
	 
	down_write(&s->s_umount);
	 
	return path.dentry;
}
EXPORT_SYMBOL(mount_subtree);

SYSCALL_DEFINE5(mount, char __user *, dev_name, char __user *, dir_name,
		char __user *, type, unsigned long, flags, void __user *, data)
{
	int ret;
	char *kernel_type;
	char *kernel_dir;
	char *kernel_dev;
	unsigned long data_page;

	ret = copy_mount_string(type, &kernel_type);
	if (ret < 0)
		goto out_type;

	kernel_dir = getname(dir_name);
	if (IS_ERR(kernel_dir)) {
		ret = PTR_ERR(kernel_dir);
		goto out_dir;
	}

	ret = copy_mount_string(dev_name, &kernel_dev);
	if (ret < 0)
		goto out_dev;

	ret = copy_mount_options(data, &data_page);
	if (ret < 0)
		goto out_data;

	ret = do_mount(kernel_dev, kernel_dir, kernel_type, flags,
		(void *) data_page);

	free_page(data_page);
out_data:
	kfree(kernel_dev);
out_dev:
	putname(kernel_dir);
out_dir:
	kfree(kernel_type);
out_type:
	return ret;
}

SYSCALL_DEFINE2(pivot_root, const char __user *, new_root,
		const char __user *, put_old)
{
	struct vfsmount *tmp;
	struct path new, old, parent_path, root_parent, root;
	int error;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	error = user_path_dir(new_root, &new);
	if (error)
		goto out0;

	error = user_path_dir(put_old, &old);
	if (error)
		goto out1;

	error = security_sb_pivotroot(&old, &new);
	if (error)
		goto out2;

	get_fs_root(current->fs, &root);
	error = lock_mount(&old);
	if (error)
		goto out3;

	error = -EINVAL;
	if (IS_MNT_SHARED(old.mnt) ||
		IS_MNT_SHARED(new.mnt->mnt_parent) ||
		IS_MNT_SHARED(root.mnt->mnt_parent))
		goto out4;
	if (!check_mnt(root.mnt) || !check_mnt(new.mnt))
		goto out4;
	error = -ENOENT;
	if (d_unlinked(new.dentry))
		goto out4;
	if (d_unlinked(old.dentry))
		goto out4;
	error = -EBUSY;
	if (new.mnt == root.mnt ||
	    old.mnt == root.mnt)
		goto out4;  
	error = -EINVAL;
	if (root.mnt->mnt_root != root.dentry)
		goto out4;  
	if (root.mnt->mnt_parent == root.mnt)
		goto out4;  
	if (new.mnt->mnt_root != new.dentry)
		goto out4;  
	if (new.mnt->mnt_parent == new.mnt)
		goto out4;  
	 
	tmp = old.mnt;
	if (tmp != new.mnt) {
		for (;;) {
			if (tmp->mnt_parent == tmp)
				goto out4;  
			if (tmp->mnt_parent == new.mnt)
				break;
			tmp = tmp->mnt_parent;
		}
		if (!is_subdir(tmp->mnt_mountpoint, new.dentry))
			goto out4;
	} else if (!is_subdir(old.dentry, new.dentry))
		goto out4;
	br_write_lock(vfsmount_lock);
	detach_mnt(new.mnt, &parent_path);
	detach_mnt(root.mnt, &root_parent);
	 
	attach_mnt(root.mnt, &old);
	 
	attach_mnt(new.mnt, &root_parent);
	touch_mnt_namespace(current->nsproxy->mnt_ns);
	br_write_unlock(vfsmount_lock);
	chroot_fs_refs(&root, &new);
	error = 0;
out4:
	unlock_mount(&old);
	if (!error) {
		path_put(&root_parent);
		path_put(&parent_path);
	}
out3:
	path_put(&root);
out2:
	path_put(&old);
out1:
	path_put(&new);
out0:
	return error;
}

static void __init init_mount_tree(void)
{
	struct vfsmount *mnt;
	struct mnt_namespace *ns;
	struct path root;

	mnt = do_kern_mount("rootfs", 0, "rootfs", NULL);
	if (IS_ERR(mnt))
		panic("Can't create rootfs");

	ns = create_mnt_ns(mnt);
	if (IS_ERR(ns))
		panic("Can't allocate initial namespace");

	init_task.nsproxy->mnt_ns = ns;
	get_mnt_ns(ns);

	root.mnt = ns->root;
	root.dentry = ns->root->mnt_root;

	set_fs_pwd(current->fs, &root);
	set_fs_root(current->fs, &root);
}

void __init mnt_init(void)
{
	unsigned u;
	int err;

	init_rwsem(&namespace_sem);

	mnt_cache = kmem_cache_create("mnt_cache", sizeof(struct vfsmount),
			0, SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);

	mount_hashtable = (struct list_head *)__get_free_page(GFP_ATOMIC);

	if (!mount_hashtable)
		panic("Failed to allocate mount hash table\n");

	printk(KERN_INFO "Mount-cache hash table entries: %lu\n", HASH_SIZE);

	for (u = 0; u < HASH_SIZE; u++)
		INIT_LIST_HEAD(&mount_hashtable[u]);

	br_lock_init(vfsmount_lock);

	err = sysfs_init();
	if (err)
		printk(KERN_WARNING "%s: sysfs_init error: %d\n",
			__func__, err);
	fs_kobj = kobject_create_and_add("fs", NULL);
	if (!fs_kobj)
		printk(KERN_WARNING "%s: kobj create error\n", __func__);
	init_rootfs();
	init_mount_tree();
}

void put_mnt_ns(struct mnt_namespace *ns)
{
	LIST_HEAD(umount_list);

	if (!atomic_dec_and_test(&ns->count))
		return;
	down_write(&namespace_sem);
	br_write_lock(vfsmount_lock);
	umount_tree(ns->root, 0, &umount_list);
	br_write_unlock(vfsmount_lock);
	up_write(&namespace_sem);
	release_mounts(&umount_list);
	kfree(ns);
}
EXPORT_SYMBOL(put_mnt_ns);

struct vfsmount *kern_mount_data(struct file_system_type *type, void *data)
{
	struct vfsmount *mnt;
	mnt = vfs_kern_mount(type, MS_KERNMOUNT, type->name, data);
	if (!IS_ERR(mnt)) {
		 
		mnt_make_longterm(mnt);
	}
	return mnt;
}
EXPORT_SYMBOL_GPL(kern_mount_data);

void kern_unmount(struct vfsmount *mnt)
{
	 
	if (!IS_ERR_OR_NULL(mnt)) {
		mnt_make_shortterm(mnt);
		mntput(mnt);
	}
}
EXPORT_SYMBOL(kern_unmount);

int (*funcSYNOSendErrorFsBtrfsEvent)(const u8*) = NULL;
EXPORT_SYMBOL(funcSYNOSendErrorFsBtrfsEvent);

bool our_mnt(struct vfsmount *mnt)
{
	return check_mnt(mnt);
}
