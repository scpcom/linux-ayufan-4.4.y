#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/quotaops.h>
#include <linux/fsnotify.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/namei.h>
#include <linux/backing-dev.h>
#include <linux/capability.h>
#include <linux/securebits.h>
#include <linux/security.h>
#include <linux/mount.h>
#include <linux/vfs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/personality.h>
#include <linux/pagemap.h>
#include <linux/syscalls.h>
#include <linux/rcupdate.h>
#include <linux/audit.h>
#include <linux/falloc.h>
#include <linux/fs_struct.h>

#ifdef CONFIG_FS_SYNO_ACL
#include "synoacl_int.h"
#endif

int vfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	int retval = -ENODEV;

	if (dentry) {
		retval = -ENOSYS;
		if (dentry->d_sb->s_op->statfs) {
			memset(buf, 0, sizeof(*buf));
			retval = security_sb_statfs(dentry);
			if (retval)
				return retval;
			retval = dentry->d_sb->s_op->statfs(dentry, buf);
			if (retval == 0 && buf->f_frsize == 0)
				buf->f_frsize = buf->f_bsize;
		}
	}
	return retval;
}

EXPORT_SYMBOL(vfs_statfs);

static int vfs_statfs_native(struct dentry *dentry, struct statfs *buf)
{
	struct kstatfs st;
	int retval;

	retval = vfs_statfs(dentry, &st);
	if (retval)
		return retval;

	if (sizeof(*buf) == sizeof(st))
		memcpy(buf, &st, sizeof(st));
	else {
		if (sizeof buf->f_blocks == 4) {
			if ((st.f_blocks | st.f_bfree | st.f_bavail |
			     st.f_bsize | st.f_frsize) &
			    0xffffffff00000000ULL)
				return -EOVERFLOW;
			 
			if (st.f_files != -1 &&
			    (st.f_files & 0xffffffff00000000ULL))
				return -EOVERFLOW;
			if (st.f_ffree != -1 &&
			    (st.f_ffree & 0xffffffff00000000ULL))
				return -EOVERFLOW;
		}

		buf->f_type = st.f_type;
		buf->f_bsize = st.f_bsize;
		buf->f_blocks = st.f_blocks;
		buf->f_bfree = st.f_bfree;
		buf->f_bavail = st.f_bavail;
		buf->f_files = st.f_files;
		buf->f_ffree = st.f_ffree;
		buf->f_fsid = st.f_fsid;
		buf->f_namelen = st.f_namelen;
		buf->f_frsize = st.f_frsize;
		memset(buf->f_spare, 0, sizeof(buf->f_spare));
	}
	return 0;
}

static int vfs_statfs64(struct dentry *dentry, struct statfs64 *buf)
{
	struct kstatfs st;
	int retval;

	retval = vfs_statfs(dentry, &st);
	if (retval)
		return retval;

	if (sizeof(*buf) == sizeof(st))
		memcpy(buf, &st, sizeof(st));
	else {
		buf->f_type = st.f_type;
		buf->f_bsize = st.f_bsize;
		buf->f_blocks = st.f_blocks;
		buf->f_bfree = st.f_bfree;
		buf->f_bavail = st.f_bavail;
		buf->f_files = st.f_files;
		buf->f_ffree = st.f_ffree;
		buf->f_fsid = st.f_fsid;
		buf->f_namelen = st.f_namelen;
		buf->f_frsize = st.f_frsize;
		memset(buf->f_spare, 0, sizeof(buf->f_spare));
	}
	return 0;
}

SYSCALL_DEFINE2(statfs, const char __user *, pathname, struct statfs __user *, buf)
{
	struct path path;
	int error;

	error = user_path(pathname, &path);
	if (!error) {
		struct statfs tmp;
		error = vfs_statfs_native(path.dentry, &tmp);
		if (!error && copy_to_user(buf, &tmp, sizeof(tmp)))
			error = -EFAULT;
		path_put(&path);
	}
	return error;
}

SYSCALL_DEFINE3(statfs64, const char __user *, pathname, size_t, sz, struct statfs64 __user *, buf)
{
	struct path path;
	long error;

	if (sz != sizeof(*buf))
		return -EINVAL;
	error = user_path(pathname, &path);
	if (!error) {
		struct statfs64 tmp;
		error = vfs_statfs64(path.dentry, &tmp);
		if (!error && copy_to_user(buf, &tmp, sizeof(tmp)))
			error = -EFAULT;
		path_put(&path);
	}
	return error;
}

SYSCALL_DEFINE2(fstatfs, unsigned int, fd, struct statfs __user *, buf)
{
	struct file * file;
	struct statfs tmp;
	int error;

	error = -EBADF;
	file = fget(fd);
	if (!file)
		goto out;
	error = vfs_statfs_native(file->f_path.dentry, &tmp);
	if (!error && copy_to_user(buf, &tmp, sizeof(tmp)))
		error = -EFAULT;
	fput(file);
out:
	return error;
}

SYSCALL_DEFINE3(fstatfs64, unsigned int, fd, size_t, sz, struct statfs64 __user *, buf)
{
	struct file * file;
	struct statfs64 tmp;
	int error;

	if (sz != sizeof(*buf))
		return -EINVAL;

	error = -EBADF;
	file = fget(fd);
	if (!file)
		goto out;
	error = vfs_statfs64(file->f_path.dentry, &tmp);
	if (!error && copy_to_user(buf, &tmp, sizeof(tmp)))
		error = -EFAULT;
	fput(file);
out:
	return error;
}

int do_truncate(struct dentry *dentry, loff_t length, unsigned int time_attrs,
	struct file *filp)
{
	int ret;
	struct iattr newattrs;

	if (length < 0)
		return -EINVAL;

	newattrs.ia_size = length;
	newattrs.ia_valid = ATTR_SIZE | time_attrs;
	if (filp) {
		newattrs.ia_file = filp;
		newattrs.ia_valid |= ATTR_FILE;
	}

	ret = should_remove_suid(dentry);
	if (ret)
		newattrs.ia_valid |= ret | ATTR_FORCE;

	mutex_lock(&dentry->d_inode->i_mutex);
	ret = notify_change(dentry, &newattrs);
	mutex_unlock(&dentry->d_inode->i_mutex);
	return ret;
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(do_truncate);
#endif  

#ifdef CONFIG_OXNAS_FAST_WRITES
	extern void flush_writes(struct inode *inode);
	extern void writer_remap_file(struct inode *inode);
#else  

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	extern void incoherent_sendfile_remap_file(struct inode *inode);
#endif  

#endif  

static long do_sys_truncate(const char __user *pathname, loff_t length)
{
	struct path path;
	struct inode *inode;
	int error;

	error = -EINVAL;
	if (length < 0)	 
		goto out;

	error = user_path(pathname, &path);
	if (error)
		goto out;
	inode = path.dentry->d_inode;

	error = -EISDIR;
	if (S_ISDIR(inode->i_mode))
		goto dput_and_out;

	error = -EINVAL;
	if (!S_ISREG(inode->i_mode))
		goto dput_and_out;

	error = mnt_want_write(path.mnt);
	if (error)
		goto dput_and_out;
#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(inode)) {
		error = synoacl_op_perm(path.dentry, MAY_WRITE);
	} else
#endif
	error = inode_permission(inode, MAY_WRITE);
	if (error)
		goto mnt_drop_write_and_out;

	error = -EPERM;
	if (IS_APPEND(inode))
		goto mnt_drop_write_and_out;

	error = get_write_access(inode);
	if (error)
		goto mnt_drop_write_and_out;

	error = break_lease(inode, FMODE_WRITE);
	if (error)
		goto put_write_and_out;

	error = locks_verify_truncate(inode, NULL, length);
	if (!error)
		error = security_path_truncate(&path, length, 0);
	if (!error) {
		vfs_dq_init(inode);

#ifdef CONFIG_OXNAS_FAST_WRITES
		 
		flush_writes(inode);
#endif  

		error = do_truncate(path.dentry, length, 0, NULL);

#ifdef CONFIG_OXNAS_FAST_WRITES
		 
		if (likely(!error) ) {
			writer_remap_file(inode);
		}
#else  

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		if (likely(!error) && inode->filemap_info.map) {
printk("do_sys_truncate() Inode %p re-mapping filemap %p\n", inode, inode->filemap_info.map);
			incoherent_sendfile_remap_file(inode);
		}
#endif  
#endif  
	}

put_write_and_out:
	put_write_access(inode);
mnt_drop_write_and_out:
	mnt_drop_write(path.mnt);
dput_and_out:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE2(truncate, const char __user *, path, long, length)
{
	return do_sys_truncate(path, length);
}

static long do_sys_ftruncate(unsigned int fd, loff_t length, int small)
{
	struct inode * inode;
	struct dentry *dentry;
	struct file * file;
	int error;

	error = -EINVAL;
	if (length < 0)
		goto out;
	error = -EBADF;
	file = fget(fd);
	if (!file)
		goto out;

	if (file->f_flags & O_LARGEFILE)
		small = 0;

	dentry = file->f_path.dentry;
	inode = dentry->d_inode;
	error = -EINVAL;
	if (!S_ISREG(inode->i_mode) || !(file->f_mode & FMODE_WRITE))
		goto out_putf;

	error = -EINVAL;
	 
	if (small && length > MAX_NON_LFS)
		goto out_putf;

	error = -EPERM;
	if (IS_APPEND(inode))
		goto out_putf;

	error = locks_verify_truncate(inode, file, length);
	if (!error)
		error = security_path_truncate(&file->f_path, length,
					       ATTR_MTIME|ATTR_CTIME);

#ifndef CONFIG_SYNO_PLX_PORTING
	if (!error)
		error = do_truncate(dentry, length, ATTR_MTIME|ATTR_CTIME, file);
#else
	if (!error) {
#ifdef CONFIG_OXNAS_FAST_WRITES                                                                       
		                                                          
		flush_writes(inode);                                                                   
#endif  
		error = do_truncate(dentry, length, ATTR_MTIME|ATTR_CTIME, file);

#ifdef CONFIG_OXNAS_FAST_WRITES
		 
		if (likely(!error) && file->inode ) {
			writer_remap_file(file->inode);
		}
#else  
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		if (likely(!error) && file->inode && file->inode->filemap_info.map) {
printk("do_sys_ftruncate() File %p, inode %p re-mapping filemap\n", file, file->inode);
			incoherent_sendfile_remap_file(file->inode);
		}
#endif  
#endif  
	}
#endif

out_putf:
	fput(file);
out:
	return error;
}

SYSCALL_DEFINE2(ftruncate, unsigned int, fd, unsigned long, length)
{
	long ret = do_sys_ftruncate(fd, length, 1);
	 
	asmlinkage_protect(2, ret, fd, length);
	return ret;
}

#if BITS_PER_LONG == 32
SYSCALL_DEFINE(truncate64)(const char __user * path, loff_t length)
{
	return do_sys_truncate(path, length);
}
#ifdef CONFIG_HAVE_SYSCALL_WRAPPERS
asmlinkage long SyS_truncate64(long path, loff_t length)
{
	return SYSC_truncate64((const char __user *) path, length);
}
SYSCALL_ALIAS(sys_truncate64, SyS_truncate64);
#endif

SYSCALL_DEFINE(ftruncate64)(unsigned int fd, loff_t length)
{
	long ret = do_sys_ftruncate(fd, length, 0);
	 
	asmlinkage_protect(2, ret, fd, length);
	return ret;
}
#ifdef CONFIG_HAVE_SYSCALL_WRAPPERS
asmlinkage long SyS_ftruncate64(long fd, loff_t length)
{
	return SYSC_ftruncate64((unsigned int) fd, length);
}
SYSCALL_ALIAS(sys_ftruncate64, SyS_ftruncate64);
#endif
#endif  

int do_fallocate(struct file *file, int mode, loff_t offset, loff_t len)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	long ret;

	if (offset < 0 || len <= 0)
		return -EINVAL;

	if (mode && !(mode & FALLOC_FL_KEEP_SIZE))
		return -EOPNOTSUPP;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;
	 
	ret = security_file_permission(file, MAY_WRITE);
	if (ret)
		return ret;

	if (S_ISFIFO(inode->i_mode))
		return -ESPIPE;

	if (!S_ISREG(inode->i_mode) && !S_ISDIR(inode->i_mode))
		return -ENODEV;

	if (((offset + len) > inode->i_sb->s_maxbytes) || ((offset + len) < 0))
		return -EFBIG;

	if (!inode->i_op->fallocate)
		return -EOPNOTSUPP;

	return inode->i_op->fallocate(inode, mode, offset, len);
}

SYSCALL_DEFINE(fallocate)(int fd, int mode, loff_t offset, loff_t len)
{
	struct file *file;
	int error = -EBADF;

	file = fget(fd);
	if (file) {
		error = do_fallocate(file, mode, offset, len);
		fput(file);
	}

	return error;
}

#ifdef CONFIG_HAVE_SYSCALL_WRAPPERS
asmlinkage long SyS_fallocate(long fd, long mode, loff_t offset, loff_t len)
{
	return SYSC_fallocate((int)fd, (int)mode, offset, len);
}
SYSCALL_ALIAS(sys_fallocate, SyS_fallocate);
#endif

SYSCALL_DEFINE3(faccessat, int, dfd, const char __user *, filename, int, mode)
{
	const struct cred *old_cred;
	struct cred *override_cred;
	struct path path;
	struct inode *inode;
	int res;

	if (mode & ~S_IRWXO)	 
		return -EINVAL;

	override_cred = prepare_creds();
	if (!override_cred)
		return -ENOMEM;

	override_cred->fsuid = override_cred->uid;
	override_cred->fsgid = override_cred->gid;

	if (!issecure(SECURE_NO_SETUID_FIXUP)) {
		 
		if (override_cred->uid)
			cap_clear(override_cred->cap_effective);
		else
			override_cred->cap_effective =
				override_cred->cap_permitted;
	}

	old_cred = override_creds(override_cred);

	res = user_path_at(dfd, filename, LOOKUP_FOLLOW, &path);
	if (res)
		goto out;

	inode = path.dentry->d_inode;

	if ((mode & MAY_EXEC) && S_ISREG(inode->i_mode)) {
		 
		res = -EACCES;
		if (path.mnt->mnt_flags & MNT_NOEXEC)
			goto out_path_release;
	}
#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(inode)) {
		res = synoacl_op_access(path.dentry, mode, 0);
	} else
#endif
	res = inode_permission(inode, mode | MAY_ACCESS);
	 
	if (res || !(mode & S_IWOTH) || special_file(inode->i_mode))
		goto out_path_release;
	 
	if (__mnt_is_readonly(path.mnt))
		res = -EROFS;

out_path_release:
	path_put(&path);
out:
	revert_creds(old_cred);
	put_cred(override_cred);
	return res;
}

SYSCALL_DEFINE2(access, const char __user *, filename, int, mode)
{
	return sys_faccessat(AT_FDCWD, filename, mode);
}

SYSCALL_DEFINE1(chdir, const char __user *, filename)
{
	struct path path;
	int error;
#ifdef CONFIG_FS_SYNO_ACL
	struct inode *inode;
#endif

	error = user_path_dir(filename, &path);
	if (error)
		goto out;

#ifdef CONFIG_FS_SYNO_ACL
	inode = path.dentry->d_inode;
	if (IS_SYNOACL(inode)) {
		error = synoacl_op_perm(path.dentry, MAY_EXEC | MAY_ACCESS);
	} else {
		error = inode_permission(inode, MAY_EXEC | MAY_ACCESS);
	}
#else
	error = inode_permission(path.dentry->d_inode, MAY_EXEC | MAY_ACCESS);
#endif
	if (error)
		goto dput_and_out;

	set_fs_pwd(current->fs, &path);

dput_and_out:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE1(fchdir, unsigned int, fd)
{
	struct file *file;
	struct inode *inode;
	int error;

	error = -EBADF;
	file = fget(fd);
	if (!file)
		goto out;

	inode = file->f_path.dentry->d_inode;

	error = -ENOTDIR;
	if (!S_ISDIR(inode->i_mode))
		goto out_putf;

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(inode)) {
		error = synoacl_op_perm(file->f_path.dentry, MAY_EXEC | MAY_ACCESS);
	} else
#endif
	error = inode_permission(inode, MAY_EXEC | MAY_ACCESS);
	if (!error)
		set_fs_pwd(current->fs, &file->f_path);

out_putf:
	fput(file);
out:
	return error;
}

SYSCALL_DEFINE1(chroot, const char __user *, filename)
{
	struct path path;
	int error;
#ifdef CONFIG_FS_SYNO_ACL
	struct inode *inode;
#endif
	error = user_path_dir(filename, &path);
	if (error)
		goto out;

#ifdef CONFIG_FS_SYNO_ACL
	inode = path.dentry->d_inode;
	if (IS_SYNOACL(inode)) {
		error = synoacl_op_perm(path.dentry, MAY_EXEC | MAY_ACCESS);
	} else {
		error = inode_permission(inode, MAY_EXEC | MAY_ACCESS);
	}
#else
	error = inode_permission(path.dentry->d_inode, MAY_EXEC | MAY_ACCESS);
#endif
	if (error)
		goto dput_and_out;

	error = -EPERM;
	if (!capable(CAP_SYS_CHROOT))
		goto dput_and_out;

	set_fs_root(current->fs, &path);
	error = 0;
dput_and_out:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE2(fchmod, unsigned int, fd, mode_t, mode)
{
	struct inode * inode;
	struct dentry * dentry;
	struct file * file;
	int err = -EBADF;
	struct iattr newattrs;

	file = fget(fd);
	if (!file)
		goto out;

	dentry = file->f_path.dentry;
	inode = dentry->d_inode;

	audit_inode(NULL, dentry);

	err = mnt_want_write_file(file);
	if (err)
		goto out_putf;
	mutex_lock(&inode->i_mutex);
	if (mode == (mode_t) -1)
		mode = inode->i_mode;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	err = notify_change(dentry, &newattrs);
	mutex_unlock(&inode->i_mutex);
	mnt_drop_write(file->f_path.mnt);
out_putf:
	fput(file);
out:
	return err;
}

SYSCALL_DEFINE3(fchmodat, int, dfd, const char __user *, filename, mode_t, mode)
{
	struct path path;
	struct inode *inode;
	int error;
	struct iattr newattrs;

	error = user_path_at(dfd, filename, LOOKUP_FOLLOW, &path);
	if (error)
		goto out;
	inode = path.dentry->d_inode;

	error = mnt_want_write(path.mnt);
	if (error)
		goto dput_and_out;
	mutex_lock(&inode->i_mutex);
	if (mode == (mode_t) -1)
		mode = inode->i_mode;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	error = notify_change(path.dentry, &newattrs);
	mutex_unlock(&inode->i_mutex);
	mnt_drop_write(path.mnt);
dput_and_out:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE2(chmod, const char __user *, filename, mode_t, mode)
{
	return sys_fchmodat(AT_FDCWD, filename, mode);
}

static int chown_common(struct dentry * dentry, uid_t user, gid_t group)
{
	struct inode *inode = dentry->d_inode;
	int error;
	struct iattr newattrs;

	newattrs.ia_valid =  ATTR_CTIME;
	if (user != (uid_t) -1) {
		newattrs.ia_valid |= ATTR_UID;
		newattrs.ia_uid = user;
	}
	if (group != (gid_t) -1) {
		newattrs.ia_valid |= ATTR_GID;
		newattrs.ia_gid = group;
	}
	if (!S_ISDIR(inode->i_mode))
		newattrs.ia_valid |=
			ATTR_KILL_SUID | ATTR_KILL_SGID | ATTR_KILL_PRIV;
	mutex_lock(&inode->i_mutex);
	error = notify_change(dentry, &newattrs);
	mutex_unlock(&inode->i_mutex);

	return error;
}

#ifdef	MY_ABC_HERE
extern long __SYNOArchiveSet(struct dentry *, unsigned int cmd);

asmlinkage long sys_SYNOArchiveBit(const char * filename, int cmd)
{
	struct path path;
	long error;

	if (SYNO_FCNTL_BASE > cmd || SYNO_FCNTL_LAST < cmd) {
		printk(KERN_WARNING "Archive bit cmd:%x not implement.\n", cmd);
		return -EINVAL;
	}

	error = user_path_at(AT_FDCWD, filename, LOOKUP_FOLLOW, &path);
	if (error)
		return error;

	error = __SYNOArchiveSet(path.dentry, cmd);
	path_put(&path);
	return error;
}
#endif  

SYSCALL_DEFINE3(chown, const char __user *, filename, uid_t, user, gid_t, group)
{
	struct path path;
	int error;

	error = user_path(filename, &path);
	if (error)
		goto out;
	error = mnt_want_write(path.mnt);
	if (error)
		goto out_release;
	error = chown_common(path.dentry, user, group);
	mnt_drop_write(path.mnt);
out_release:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE5(fchownat, int, dfd, const char __user *, filename, uid_t, user,
		gid_t, group, int, flag)
{
	struct path path;
	int error = -EINVAL;
	int follow;

	if ((flag & ~AT_SYMLINK_NOFOLLOW) != 0)
		goto out;

	follow = (flag & AT_SYMLINK_NOFOLLOW) ? 0 : LOOKUP_FOLLOW;
	error = user_path_at(dfd, filename, follow, &path);
	if (error)
		goto out;
	error = mnt_want_write(path.mnt);
	if (error)
		goto out_release;
	error = chown_common(path.dentry, user, group);
	mnt_drop_write(path.mnt);
out_release:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE3(lchown, const char __user *, filename, uid_t, user, gid_t, group)
{
	struct path path;
	int error;

	error = user_lpath(filename, &path);
	if (error)
		goto out;
	error = mnt_want_write(path.mnt);
	if (error)
		goto out_release;
	error = chown_common(path.dentry, user, group);
	mnt_drop_write(path.mnt);
out_release:
	path_put(&path);
out:
	return error;
}

SYSCALL_DEFINE3(fchown, unsigned int, fd, uid_t, user, gid_t, group)
{
	struct file * file;
	int error = -EBADF;
	struct dentry * dentry;

	file = fget(fd);
	if (!file)
		goto out;

	error = mnt_want_write_file(file);
	if (error)
		goto out_fput;
	dentry = file->f_path.dentry;
	audit_inode(NULL, dentry);
	error = chown_common(dentry, user, group);
	mnt_drop_write(file->f_path.mnt);
out_fput:
	fput(file);
out:
	return error;
}

static inline int __get_file_write_access(struct inode *inode,
					  struct vfsmount *mnt)
{
	int error;
	error = get_write_access(inode);
	if (error)
		return error;
	 
	if (!special_file(inode->i_mode)) {
		 
		error = mnt_want_write(mnt);
		if (error)
			put_write_access(inode);
	}
	return error;
}

#ifdef CONFIG_SYNO_PLX_PORTING
#include <mach/fast_open_filter.h>
#endif

static struct file *__dentry_open(struct dentry *dentry, struct vfsmount *mnt,
					int flags, struct file *f,
					int (*open)(struct inode *, struct file *),
					const struct cred *cred)
{
	struct inode *inode;
	int error;

	f->f_flags = flags;
	f->f_mode = (__force fmode_t)((flags+1) & O_ACCMODE) | FMODE_LSEEK |
				FMODE_PREAD | FMODE_PWRITE;
	inode = dentry->d_inode;
	if (f->f_mode & FMODE_WRITE) {
		error = __get_file_write_access(inode, mnt);
		if (error)
			goto cleanup_file;
		if (!special_file(inode->i_mode))
			file_take_write(f);
	}

	f->f_mapping = inode->i_mapping;
	f->f_path.dentry = dentry;
	f->f_path.mnt = mnt;
	f->f_pos = 0;
	f->f_op = fops_get(inode->i_fop);
	file_move(f, &inode->i_sb->s_files);

	error = security_dentry_open(f, cred);
	if (error)
		goto cleanup_all;

	if (!open && f->f_op)
		open = f->f_op->open;
	if (open) {
		error = open(inode, f);
		if (error)
			goto cleanup_all;
	}

	f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	file_ra_state_init(&f->f_ra, f->f_mapping->host->i_mapping);

	if (f->f_flags & O_DIRECT) {
		if (!f->f_mapping->a_ops ||
		    ((!f->f_mapping->a_ops->direct_IO) &&
		    (!f->f_mapping->a_ops->get_xip_mem))) {
			fput(f);
			f = ERR_PTR(-EINVAL);
		}
	}

#ifdef CONFIG_SYNO_PLX_PORTING
	if (fast_open_filter(f, inode)) {
		fput(f);
		f = ERR_PTR(-EINVAL);
	}
#endif

	return f;

cleanup_all:
	fops_put(f->f_op);
	if (f->f_mode & FMODE_WRITE) {
		put_write_access(inode);
		if (!special_file(inode->i_mode)) {
			 
			file_reset_write(f);
			mnt_drop_write(mnt);
		}
	}
	file_kill(f);
	f->f_path.dentry = NULL;
	f->f_path.mnt = NULL;
cleanup_file:
	put_filp(f);
	dput(dentry);
	mntput(mnt);
	return ERR_PTR(error);
}

struct file *lookup_instantiate_filp(struct nameidata *nd, struct dentry *dentry,
		int (*open)(struct inode *, struct file *))
{
	const struct cred *cred = current_cred();

	if (IS_ERR(nd->intent.open.file))
		goto out;
	if (IS_ERR(dentry))
		goto out_err;
	nd->intent.open.file = __dentry_open(dget(dentry), mntget(nd->path.mnt),
					     nd->intent.open.flags - 1,
					     nd->intent.open.file,
					     open, cred);
out:
	return nd->intent.open.file;
out_err:
	release_open_intent(nd);
	nd->intent.open.file = (struct file *)dentry;
	goto out;
}
EXPORT_SYMBOL_GPL(lookup_instantiate_filp);

struct file *nameidata_to_filp(struct nameidata *nd, int flags)
{
	const struct cred *cred = current_cred();
	struct file *filp;

	filp = nd->intent.open.file;
	 
	if (filp->f_path.dentry == NULL)
		filp = __dentry_open(nd->path.dentry, nd->path.mnt, flags, filp,
				     NULL, cred);
	else
		path_put(&nd->path);
	return filp;
}

struct file *dentry_open(struct dentry *dentry, struct vfsmount *mnt, int flags,
			 const struct cred *cred)
{
	int error;
	struct file *f;

	validate_creds(cred);

	if (!mnt) {
		printk(KERN_WARNING "%s called with NULL vfsmount\n", __func__);
		dump_stack();
		return ERR_PTR(-EINVAL);
	}

	error = -ENFILE;
	f = get_empty_filp();
	if (f == NULL) {
		dput(dentry);
		mntput(mnt);
		return ERR_PTR(error);
	}

	return __dentry_open(dentry, mnt, flags, f, NULL, cred);
}
EXPORT_SYMBOL(dentry_open);

static void __put_unused_fd(struct files_struct *files, unsigned int fd)
{
	struct fdtable *fdt = files_fdtable(files);
	__FD_CLR(fd, fdt->open_fds);
	if (fd < files->next_fd)
		files->next_fd = fd;
}

void put_unused_fd(unsigned int fd)
{
	struct files_struct *files = current->files;
	spin_lock(&files->file_lock);
	__put_unused_fd(files, fd);
	spin_unlock(&files->file_lock);
}

EXPORT_SYMBOL(put_unused_fd);

void fd_install(unsigned int fd, struct file *file)
{
	struct files_struct *files = current->files;
	struct fdtable *fdt;
	spin_lock(&files->file_lock);
	fdt = files_fdtable(files);
	BUG_ON(fdt->fd[fd] != NULL);
	rcu_assign_pointer(fdt->fd[fd], file);
	spin_unlock(&files->file_lock);
}

EXPORT_SYMBOL(fd_install);

long do_sys_open(int dfd, const char __user *filename, int flags, int mode)
{
	char *tmp = getname(filename);
	int fd = PTR_ERR(tmp);

	if (!IS_ERR(tmp)) {
		fd = get_unused_fd_flags(flags);
		if (fd >= 0) {
			struct file *f = do_filp_open(dfd, tmp, flags, mode, 0);
			if (IS_ERR(f)) {
				put_unused_fd(fd);
				fd = PTR_ERR(f);
			} else {
				fsnotify_open(f->f_path.dentry);
				fd_install(fd, f);
			}
		}
		putname(tmp);
	}
	return fd;
}

#ifdef MY_ABC_HERE
#include <linux/synolib.h>
extern int syno_hibernation_log_sec;
#endif
SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, int, mode)
{
	long ret;

	if (force_o_largefile())
		flags |= O_LARGEFILE;

#ifdef MY_ABC_HERE
	if(syno_hibernation_log_sec > 0) {
		syno_do_hibernation_log(filename);
	}
#endif

	ret = do_sys_open(AT_FDCWD, filename, flags, mode);
	 
	asmlinkage_protect(3, ret, filename, flags, mode);
	return ret;
}

SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename, int, flags,
		int, mode)
{
	long ret;

	if (force_o_largefile())
		flags |= O_LARGEFILE;

	ret = do_sys_open(dfd, filename, flags, mode);
	 
	asmlinkage_protect(4, ret, dfd, filename, flags, mode);
	return ret;
}

#ifndef __alpha__

SYSCALL_DEFINE2(creat, const char __user *, pathname, int, mode)
{
	return sys_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

#endif

int filp_close(struct file *filp, fl_owner_t id)
{
	int retval = 0;

	if (!file_count(filp)) {
		printk(KERN_ERR "VFS: Close: file count is 0\n");
		return 0;
	}

#ifdef SYNO_FORCE_UNMOUNT
	if (blSynostate(O_UNMOUNT_DONE, filp)) {
		fput(filp);
		return retval;
	}
#endif
	if (filp->f_op && filp->f_op->flush)
		retval = filp->f_op->flush(filp, id);

	dnotify_flush(filp, id);
	locks_remove_posix(filp, id);
	fput(filp);
	return retval;
}

EXPORT_SYMBOL(filp_close);

SYSCALL_DEFINE1(close, unsigned int, fd)
{
	struct file * filp;
	struct files_struct *files = current->files;
	struct fdtable *fdt;
	int retval;

	spin_lock(&files->file_lock);
	fdt = files_fdtable(files);
	if (fd >= fdt->max_fds)
		goto out_unlock;
	filp = fdt->fd[fd];
	if (!filp)
		goto out_unlock;
	rcu_assign_pointer(fdt->fd[fd], NULL);
	FD_CLR(fd, fdt->close_on_exec);
	__put_unused_fd(files, fd);
	spin_unlock(&files->file_lock);
	retval = filp_close(filp, files);

	if (unlikely(retval == -ERESTARTSYS ||
		     retval == -ERESTARTNOINTR ||
		     retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK))
		retval = -EINTR;

	return retval;

out_unlock:
	spin_unlock(&files->file_lock);
	return -EBADF;
}
EXPORT_SYMBOL(sys_close);

SYSCALL_DEFINE0(vhangup)
{
	if (capable(CAP_SYS_TTY_CONFIG)) {
		tty_vhangup_self();
		return 0;
	}
	return -EPERM;
}

int generic_file_open(struct inode * inode, struct file * filp)
{
	if (!(filp->f_flags & O_LARGEFILE) && i_size_read(inode) > MAX_NON_LFS)
		return -EOVERFLOW;
	return 0;
}

EXPORT_SYMBOL(generic_file_open);

int nonseekable_open(struct inode *inode, struct file *filp)
{
	filp->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

EXPORT_SYMBOL(nonseekable_open);
