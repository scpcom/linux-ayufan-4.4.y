 
#include <linux/module.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/capability.h>
#include <linux/fsnotify.h>
#include <linux/fcntl.h>
#include <linux/quotaops.h>
#include <linux/security.h>

#ifdef CONFIG_FS_SYNO_ACL
#include "synoacl_int.h"
#endif

int inode_change_ok(const struct inode *inode, struct iattr *attr)
{
	int retval = -EPERM;
	unsigned int ia_valid = attr->ia_valid;

	if (ia_valid & ATTR_FORCE)
		goto fine;

	if ((ia_valid & ATTR_UID) &&
	    (current_fsuid() != inode->i_uid ||
	     attr->ia_uid != inode->i_uid) && !capable(CAP_CHOWN))
		goto error;

	if ((ia_valid & ATTR_GID) &&
	    (current_fsuid() != inode->i_uid ||
	    (!in_group_p(attr->ia_gid) && attr->ia_gid != inode->i_gid)) &&
	    !capable(CAP_CHOWN))
		goto error;

	if (ia_valid & ATTR_MODE) {
		if (!is_owner_or_cap(inode))
			goto error;
		 
		if (!in_group_p((ia_valid & ATTR_GID) ? attr->ia_gid :
				inode->i_gid) && !capable(CAP_FSETID))
			attr->ia_mode &= ~S_ISGID;
	}

	if (ia_valid & (ATTR_MTIME_SET | ATTR_ATIME_SET | ATTR_TIMES_SET)) {
		if (!is_owner_or_cap(inode))
			goto error;
	}
fine:
	retval = 0;
error:
	return retval;
}
EXPORT_SYMBOL(inode_change_ok);

int inode_newsize_ok(const struct inode *inode, loff_t offset)
{
	if (inode->i_size < offset) {
		unsigned long limit;

		limit = current->signal->rlim[RLIMIT_FSIZE].rlim_cur;
		if (limit != RLIM_INFINITY && offset > limit)
			goto out_sig;
		if (offset > inode->i_sb->s_maxbytes)
			goto out_big;
	} else {
		 
		if (IS_SWAPFILE(inode))
			return -ETXTBSY;
	}

	return 0;
out_sig:
	send_sig(SIGXFSZ, current, 0);
out_big:
	return -EFBIG;
}
EXPORT_SYMBOL(inode_newsize_ok);

int inode_setattr(struct inode * inode, struct iattr * attr)
{
	unsigned int ia_valid = attr->ia_valid;

	if (ia_valid & ATTR_SIZE &&
	    attr->ia_size != i_size_read(inode)) {
		int error = vmtruncate(inode, attr->ia_size);
		if (error)
			return error;
	}

	if (ia_valid & ATTR_UID)
		inode->i_uid = attr->ia_uid;
	if (ia_valid & ATTR_GID)
		inode->i_gid = attr->ia_gid;
	if (ia_valid & ATTR_ATIME)
		inode->i_atime = timespec_trunc(attr->ia_atime,
						inode->i_sb->s_time_gran);
	if (ia_valid & ATTR_MTIME)
		inode->i_mtime = timespec_trunc(attr->ia_mtime,
						inode->i_sb->s_time_gran);
	if (ia_valid & ATTR_CTIME)
		inode->i_ctime = timespec_trunc(attr->ia_ctime,
						inode->i_sb->s_time_gran);
	if (ia_valid & ATTR_MODE) {
		umode_t mode = attr->ia_mode;

		if (!in_group_p(inode->i_gid) && !capable(CAP_FSETID))
			mode &= ~S_ISGID;
		inode->i_mode = mode;
	}
	mark_inode_dirty(inode);

	return 0;
}
EXPORT_SYMBOL(inode_setattr);

int notify_change(struct dentry * dentry, struct iattr * attr)
{
	struct inode *inode = dentry->d_inode;
	mode_t mode = inode->i_mode;
	int error;
	struct timespec now;
	unsigned int ia_valid = attr->ia_valid;

	if (ia_valid & (ATTR_MODE | ATTR_UID | ATTR_GID | ATTR_TIMES_SET)) {
		if (IS_IMMUTABLE(inode) || IS_APPEND(inode))
			return -EPERM;
	}
#ifdef SYNO_FORCE_UNMOUNT
	if (IS_UMOUNTED_FILE(inode)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("%s(%d) force umount hit.\n", __func__, __LINE__);
#endif
		return -EPERM;
	}
#endif

	now = current_fs_time(inode->i_sb);

	attr->ia_ctime = now;
	if (!(ia_valid & ATTR_ATIME_SET))
		attr->ia_atime = now;
	if (!(ia_valid & ATTR_MTIME_SET))
		attr->ia_mtime = now;
	if (ia_valid & ATTR_KILL_PRIV) {
		attr->ia_valid &= ~ATTR_KILL_PRIV;
		ia_valid &= ~ATTR_KILL_PRIV;
		error = security_inode_need_killpriv(dentry);
		if (error > 0)
			error = security_inode_killpriv(dentry);
		if (error)
			return error;
	}

	if ((ia_valid & (ATTR_KILL_SUID|ATTR_KILL_SGID)) &&
	    (ia_valid & ATTR_MODE))
		BUG();

	if (ia_valid & ATTR_KILL_SUID) {
		if (mode & S_ISUID) {
			ia_valid = attr->ia_valid |= ATTR_MODE;
			attr->ia_mode = (inode->i_mode & ~S_ISUID);
		}
	}
	if (ia_valid & ATTR_KILL_SGID) {
		if ((mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP)) {
			if (!(ia_valid & ATTR_MODE)) {
				ia_valid = attr->ia_valid |= ATTR_MODE;
				attr->ia_mode = inode->i_mode;
			}
			attr->ia_mode &= ~S_ISGID;
		}
	}
	if (!(attr->ia_valid & ~(ATTR_KILL_SUID | ATTR_KILL_SGID)))
		return 0;

	error = security_inode_setattr(dentry, attr);
	if (error)
		return error;

	if (ia_valid & ATTR_SIZE)
		down_write(&dentry->d_inode->i_alloc_sem);

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(inode)) {
		error = synoacl_op_inode_chg_ok(dentry, attr);
		if (error) {
		       return error;
		}
	}
#endif
	if (inode->i_op && inode->i_op->setattr) {
		error = inode->i_op->setattr(dentry, attr);
	} else {
		error = inode_change_ok(inode, attr);
		if (!error) {
			if ((ia_valid & ATTR_UID && attr->ia_uid != inode->i_uid) ||
			    (ia_valid & ATTR_GID && attr->ia_gid != inode->i_gid))
				error = vfs_dq_transfer(inode, attr) ?
					-EDQUOT : 0;
			if (!error)
				error = inode_setattr(inode, attr);
		}
	}

	if (ia_valid & ATTR_SIZE)
		up_write(&dentry->d_inode->i_alloc_sem);

	if (!error)
#ifdef CONFIG_FS_SYNO_ACL
	{
		if (IS_SYNOACL(inode)) {
			synoacl_op_setattr_post(dentry, attr);
		}
#endif
		fsnotify_change(dentry, ia_valid);
#ifdef CONFIG_FS_SYNO_ACL
	}
#endif

	return error;
}

EXPORT_SYMBOL(notify_change);
