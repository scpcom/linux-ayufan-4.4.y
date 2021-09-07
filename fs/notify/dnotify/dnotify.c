 
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/dnotify.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/fdtable.h>
#include <linux/fsnotify_backend.h>

int dir_notify_enable __read_mostly = 1;

static struct kmem_cache *dnotify_struct_cache __read_mostly;
static struct kmem_cache *dnotify_mark_entry_cache __read_mostly;
static struct fsnotify_group *dnotify_group __read_mostly;
static DEFINE_MUTEX(dnotify_mark_mutex);

struct dnotify_mark_entry {
	struct fsnotify_mark_entry fsn_entry;
	struct dnotify_struct *dn;
};

static void dnotify_recalc_inode_mask(struct fsnotify_mark_entry *entry)
{
	__u32 new_mask, old_mask;
	struct dnotify_struct *dn;
	struct dnotify_mark_entry *dnentry  = container_of(entry,
							   struct dnotify_mark_entry,
							   fsn_entry);

	assert_spin_locked(&entry->lock);

	old_mask = entry->mask;
	new_mask = 0;
	for (dn = dnentry->dn; dn != NULL; dn = dn->dn_next)
		new_mask |= (dn->dn_mask & ~FS_DN_MULTISHOT);
	entry->mask = new_mask;

	if (old_mask == new_mask)
		return;

	if (entry->inode)
		fsnotify_recalc_inode_mask(entry->inode);
}

static int dnotify_handle_event(struct fsnotify_group *group,
				struct fsnotify_event *event)
{
	struct fsnotify_mark_entry *entry = NULL;
	struct dnotify_mark_entry *dnentry;
	struct inode *to_tell;
	struct dnotify_struct *dn;
	struct dnotify_struct **prev;
	struct fown_struct *fown;
	__u32 test_mask = event->mask & ~FS_EVENT_ON_CHILD;

	to_tell = event->to_tell;

	spin_lock(&to_tell->i_lock);
	entry = fsnotify_find_mark_entry(group, to_tell);
	spin_unlock(&to_tell->i_lock);

	if (unlikely(!entry))
		return 0;
	dnentry = container_of(entry, struct dnotify_mark_entry, fsn_entry);

	spin_lock(&entry->lock);
	prev = &dnentry->dn;
	while ((dn = *prev) != NULL) {
		if ((dn->dn_mask & test_mask) == 0) {
			prev = &dn->dn_next;
			continue;
		}
		fown = &dn->dn_filp->f_owner;
		send_sigio(fown, dn->dn_fd, POLL_MSG);
		if (dn->dn_mask & FS_DN_MULTISHOT)
			prev = &dn->dn_next;
		else {
			*prev = dn->dn_next;
			kmem_cache_free(dnotify_struct_cache, dn);
			dnotify_recalc_inode_mask(entry);
		}
	}

	spin_unlock(&entry->lock);
	fsnotify_put_mark(entry);

	return 0;
}

static bool dnotify_should_send_event(struct fsnotify_group *group,
				      struct inode *inode, __u32 mask)
{
	struct fsnotify_mark_entry *entry;
	bool send;

	if (!S_ISDIR(inode->i_mode))
		return false;

	spin_lock(&inode->i_lock);
	entry = fsnotify_find_mark_entry(group, inode);
	spin_unlock(&inode->i_lock);

	if (!entry)
		return false;

	mask = (mask & ~FS_EVENT_ON_CHILD);
	send = (mask & entry->mask);

	fsnotify_put_mark(entry);  

	return send;
}

static void dnotify_free_mark(struct fsnotify_mark_entry *entry)
{
	struct dnotify_mark_entry *dnentry = container_of(entry,
							  struct dnotify_mark_entry,
							  fsn_entry);

	BUG_ON(dnentry->dn);

	kmem_cache_free(dnotify_mark_entry_cache, dnentry);
}

static struct fsnotify_ops dnotify_fsnotify_ops = {
	.handle_event = dnotify_handle_event,
	.should_send_event = dnotify_should_send_event,
	.free_group_priv = NULL,
	.freeing_mark = NULL,
	.free_event_priv = NULL,
};

void dnotify_flush(struct file *filp, fl_owner_t id)
{
	struct fsnotify_mark_entry *entry;
	struct dnotify_mark_entry *dnentry;
	struct dnotify_struct *dn;
	struct dnotify_struct **prev;
	struct inode *inode;

#ifdef SYNO_FORCE_UNMOUNT
	if (blSynostate(O_UNMOUNT_DONE, filp)) {
		return;
	}
	if (id == NULL) {
		return;
	}
#endif
	inode = filp->f_path.dentry->d_inode;
	if (!S_ISDIR(inode->i_mode))
		return;

	spin_lock(&inode->i_lock);
	entry = fsnotify_find_mark_entry(dnotify_group, inode);
	spin_unlock(&inode->i_lock);
	if (!entry)
		return;
	dnentry = container_of(entry, struct dnotify_mark_entry, fsn_entry);

	mutex_lock(&dnotify_mark_mutex);

	spin_lock(&entry->lock);
	prev = &dnentry->dn;
	while ((dn = *prev) != NULL) {
		if ((dn->dn_owner == id) && (dn->dn_filp == filp)) {
			*prev = dn->dn_next;
			kmem_cache_free(dnotify_struct_cache, dn);
			dnotify_recalc_inode_mask(entry);
			break;
		}
		prev = &dn->dn_next;
	}

	spin_unlock(&entry->lock);

	if (dnentry->dn == NULL)
		fsnotify_destroy_mark_by_entry(entry);

	fsnotify_recalc_group_mask(dnotify_group);

	mutex_unlock(&dnotify_mark_mutex);

	fsnotify_put_mark(entry);
}

static __u32 convert_arg(unsigned long arg)
{
	__u32 new_mask = FS_EVENT_ON_CHILD;

	if (arg & DN_MULTISHOT)
		new_mask |= FS_DN_MULTISHOT;
	if (arg & DN_DELETE)
		new_mask |= (FS_DELETE | FS_MOVED_FROM);
	if (arg & DN_MODIFY)
		new_mask |= FS_MODIFY;
	if (arg & DN_ACCESS)
		new_mask |= FS_ACCESS;
	if (arg & DN_ATTRIB)
		new_mask |= FS_ATTRIB;
	if (arg & DN_RENAME)
		new_mask |= FS_DN_RENAME;
	if (arg & DN_CREATE)
		new_mask |= (FS_CREATE | FS_MOVED_TO);

	return new_mask;
}

static int attach_dn(struct dnotify_struct *dn, struct dnotify_mark_entry *dnentry,
		     fl_owner_t id, int fd, struct file *filp, __u32 mask)
{
	struct dnotify_struct *odn;

	odn = dnentry->dn;
	while (odn != NULL) {
		 
		if ((odn->dn_owner == id) && (odn->dn_filp == filp)) {
			odn->dn_fd = fd;
			odn->dn_mask |= mask;
			return -EEXIST;
		}
		odn = odn->dn_next;
	}

	dn->dn_mask = mask;
	dn->dn_fd = fd;
	dn->dn_filp = filp;
	dn->dn_owner = id;
	dn->dn_next = dnentry->dn;
	dnentry->dn = dn;

	return 0;
}

int fcntl_dirnotify(int fd, struct file *filp, unsigned long arg)
{
	struct dnotify_mark_entry *new_dnentry, *dnentry;
	struct fsnotify_mark_entry *new_entry, *entry;
	struct dnotify_struct *dn;
	struct inode *inode;
	fl_owner_t id = current->files;
	struct file *f;
	int destroy = 0, error = 0;
	__u32 mask;

	new_entry = NULL;
	dn = NULL;

	if (!dir_notify_enable) {
		error = -EINVAL;
		goto out_err;
	}

	if ((arg & ~DN_MULTISHOT) == 0) {
		dnotify_flush(filp, id);
		error = 0;
		goto out_err;
	}

	inode = filp->f_path.dentry->d_inode;
	if (!S_ISDIR(inode->i_mode)) {
		error = -ENOTDIR;
		goto out_err;
	}

	dn = kmem_cache_alloc(dnotify_struct_cache, GFP_KERNEL);
	if (!dn) {
		error = -ENOMEM;
		goto out_err;
	}

	new_dnentry = kmem_cache_alloc(dnotify_mark_entry_cache, GFP_KERNEL);
	if (!new_dnentry) {
		error = -ENOMEM;
		goto out_err;
	}

	mask = convert_arg(arg);

	new_entry = &new_dnentry->fsn_entry;
	fsnotify_init_mark(new_entry, dnotify_free_mark);
	new_entry->mask = mask;
	new_dnentry->dn = NULL;

	mutex_lock(&dnotify_mark_mutex);

	spin_lock(&inode->i_lock);
	entry = fsnotify_find_mark_entry(dnotify_group, inode);
	spin_unlock(&inode->i_lock);
	if (entry) {
		dnentry = container_of(entry, struct dnotify_mark_entry, fsn_entry);
		spin_lock(&entry->lock);
	} else {
		fsnotify_add_mark(new_entry, dnotify_group, inode);
		spin_lock(&new_entry->lock);
		entry = new_entry;
		dnentry = new_dnentry;
		 
		new_entry = NULL;
	}

	rcu_read_lock();
	f = fcheck(fd);
	rcu_read_unlock();

	if (f != filp) {
		 
		if (dnentry == new_dnentry)
			destroy = 1;
		goto out;
	}

	error = __f_setown(filp, task_pid(current), PIDTYPE_PID, 0);
	if (error) {
		 
		if (dnentry == new_dnentry)
			destroy = 1;
		goto out;
	}

	error = attach_dn(dn, dnentry, id, fd, filp, mask);
	 
	if (!error)
		dn = NULL;
	 
	else if (error == -EEXIST)
		error = 0;

	dnotify_recalc_inode_mask(entry);
out:
	spin_unlock(&entry->lock);

	if (destroy)
		fsnotify_destroy_mark_by_entry(entry);

	fsnotify_recalc_group_mask(dnotify_group);

	mutex_unlock(&dnotify_mark_mutex);
	fsnotify_put_mark(entry);
out_err:
	if (new_entry)
		fsnotify_put_mark(new_entry);
	if (dn)
		kmem_cache_free(dnotify_struct_cache, dn);
	return error;
}

static int __init dnotify_init(void)
{
	dnotify_struct_cache = KMEM_CACHE(dnotify_struct, SLAB_PANIC);
	dnotify_mark_entry_cache = KMEM_CACHE(dnotify_mark_entry, SLAB_PANIC);

	dnotify_group = fsnotify_obtain_group(DNOTIFY_GROUP_NUM,
					      0, &dnotify_fsnotify_ops);
	if (IS_ERR(dnotify_group))
		panic("unable to allocate fsnotify group for dnotify\n");
	return 0;
}

module_init(dnotify_init)
