#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 *  linux/fs/stat.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */
 
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/highuid.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>

#ifdef CONFIG_FS_SYNO_ACL
#include "synoacl_int.h"
#endif

#ifdef SYNO_DEBUG_FLAG
#include <linux/synolib.h>
extern int SynoDebugFlag;
extern int syno_hibernation_log_level;
#endif

void generic_fillattr(struct inode *inode, struct kstat *stat)
{
	stat->dev = inode->i_sb->s_dev;
	stat->ino = inode->i_ino;
	stat->mode = inode->i_mode;
	stat->nlink = inode->i_nlink;
	stat->uid = inode->i_uid;
	stat->gid = inode->i_gid;
	stat->rdev = inode->i_rdev;
	stat->size = i_size_read(inode);
	stat->atime = inode->i_atime;
	stat->mtime = inode->i_mtime;
	stat->ctime = inode->i_ctime;
	stat->blksize = (1 << inode->i_blkbits);
	stat->blocks = inode->i_blocks;
}

EXPORT_SYMBOL(generic_fillattr);

int vfs_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	int retval;

	retval = security_inode_getattr(mnt, dentry);
	if (retval)
		return retval;

#ifdef CONFIG_FS_SYNO_ACL
	if (IS_SYNOACL(dentry)) {
		if (inode->i_op->getattr) {
			if (0 != (retval = inode->i_op->getattr(mnt, dentry, stat)))
				return retval;
		} else {
			generic_fillattr(inode, stat);
		}

		synoacl_op_to_mode(dentry, stat);

		return 0;
	}
#endif //CONFIG_FS_SYNO_ACL

	if (inode->i_op->getattr)
		return inode->i_op->getattr(mnt, dentry, stat);

	generic_fillattr(inode, stat);

	return 0;
}

EXPORT_SYMBOL(vfs_getattr);

int vfs_fstat(unsigned int fd, struct kstat *stat)
{
	int fput_needed;
	struct file *f = fget_raw_light(fd, &fput_needed);
	int error = -EBADF;

	if (f) {
		error = vfs_getattr(f->f_path.mnt, f->f_path.dentry, stat);
		fput_light(f, fput_needed);
	}
	return error;
}
EXPORT_SYMBOL(vfs_fstat);

int vfs_fstatat(int dfd, const char __user *filename, struct kstat *stat,
		int flag)
{
	struct path path;
	int error = -EINVAL;
	int lookup_flags = 0;

	if ((flag & ~(AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT |
		      AT_EMPTY_PATH)) != 0)
		goto out;

	if (!(flag & AT_SYMLINK_NOFOLLOW))
		lookup_flags |= LOOKUP_FOLLOW;
	if (flag & AT_EMPTY_PATH)
		lookup_flags |= LOOKUP_EMPTY;

	error = user_path_at(dfd, filename, lookup_flags, &path);
	if (error)
		goto out;

	error = vfs_getattr(path.mnt, path.dentry, stat);
	path_put(&path);
out:
	return error;
}
EXPORT_SYMBOL(vfs_fstatat);

int vfs_stat(const char __user *name, struct kstat *stat)
{
	return vfs_fstatat(AT_FDCWD, name, stat, 0);
}
EXPORT_SYMBOL(vfs_stat);

int vfs_lstat(const char __user *name, struct kstat *stat)
{
	return vfs_fstatat(AT_FDCWD, name, stat, AT_SYMLINK_NOFOLLOW);
}
EXPORT_SYMBOL(vfs_lstat);

#ifdef SYNO_STAT

int __always_inline syno_vfs_getattr(struct path *path, struct kstat *stat, int stat_flags)
{
	int error = 0;

	error = vfs_getattr(path->mnt, path->dentry, stat);
	if ((!error) && stat_flags) {
		struct inode *inode = path->dentry->d_inode;
		int is_support = 1;

		if (inode->i_op->syno_getattr) {
			error = inode->i_op->syno_getattr(path->dentry, stat, stat_flags);
			if (-EOPNOTSUPP == error) {
				is_support = 0;
				error = 0;
			}
		} else 
			is_support = 0;

		if (!is_support) {
#ifdef SYNO_CREATE_TIME
			stat->SynoCreateTime = inode->i_CreateTime;
#endif
#ifdef SYNO_ARCHIVE_BIT
			stat->SynoMode = inode->i_mode2;
#endif
#ifdef SYNO_ARCHIVE_VERSION
			stat->syno_archive_version = inode->i_archive_version;
#endif
		}
	}
	return error;
}

// copy from vfs_fstat
int syno_vfs_fstat(unsigned int fd, struct kstat *stat, int stat_flags)
{
	int fput_needed;
	struct file *f = fget_raw_light(fd, &fput_needed);
	int error = -EBADF;

	if (f) {
		error = syno_vfs_getattr(&(f->f_path), stat, stat_flags);
		fput_light(f, fput_needed);
	}
	return error;
}
EXPORT_SYMBOL(syno_vfs_fstat);

int syno_vfs_stat(const char __user *name, struct kstat *stat, int flags, int stat_flags)
{
	struct path path;
	int error;

	error = user_path_at(AT_FDCWD, name, flags, &path);
	if (error)
		goto out;

	error = syno_vfs_getattr(&path, stat, stat_flags);
	path_put(&path);
out:
	return error;
}
EXPORT_SYMBOL(syno_vfs_stat);
#endif

#ifdef __ARCH_WANT_OLD_STAT

/*
 * For backward compatibility?  Maybe this should be moved
 * into arch/i386 instead?
 */
static int cp_old_stat(struct kstat *stat, struct __old_kernel_stat __user * statbuf)
{
	static int warncount = 5;
	struct __old_kernel_stat tmp;
	
	if (warncount > 0) {
		warncount--;
		printk(KERN_WARNING "VFS: Warning: %s using old stat() call. Recompile your binary.\n",
			current->comm);
	} else if (warncount < 0) {
		/* it's laughable, but... */
		warncount = 0;
	}

	memset(&tmp, 0, sizeof(struct __old_kernel_stat));
	tmp.st_dev = old_encode_dev(stat->dev);
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, stat->uid);
	SET_GID(tmp.st_gid, stat->gid);
	tmp.st_rdev = old_encode_dev(stat->rdev);
#if BITS_PER_LONG == 32
	if (stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
#endif	
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_ctime = stat->ctime.tv_sec;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(stat, const char __user *, filename,
		struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

#ifdef SYNO_DEBUG_FLAG
	if(syno_hibernation_log_level > 0) {
		syno_do_hibernation_filename_log(filename);
	}
#endif

	error = vfs_stat(filename, &stat);

	if (error)
		return error;

	return cp_old_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(lstat, const char __user *, filename,
		struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;

	return cp_old_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(fstat, unsigned int, fd, struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_old_stat(&stat, statbuf);

	return error;
}
#endif /* __ARCH_WANT_OLD_STAT */

static int cp_new_stat(struct kstat *stat, struct stat __user *statbuf)
{
	struct stat tmp;

#if BITS_PER_LONG == 32
	if (!old_valid_dev(stat->dev) || !old_valid_dev(stat->rdev))
		return -EOVERFLOW;
#else
	if (!new_valid_dev(stat->dev) || !new_valid_dev(stat->rdev))
		return -EOVERFLOW;
#endif

	memset(&tmp, 0, sizeof(tmp));
#if BITS_PER_LONG == 32
	tmp.st_dev = old_encode_dev(stat->dev);
#else
	tmp.st_dev = new_encode_dev(stat->dev);
#endif
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, stat->uid);
	SET_GID(tmp.st_gid, stat->gid);
#if BITS_PER_LONG == 32
	tmp.st_rdev = old_encode_dev(stat->rdev);
#else
	tmp.st_rdev = new_encode_dev(stat->rdev);
#endif
#if BITS_PER_LONG == 32
	if (stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
#endif	
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_ctime = stat->ctime.tv_sec;
#ifdef STAT_HAVE_NSEC
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
#endif
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(newstat, const char __user *, filename,
		struct stat __user *, statbuf)
{
	struct kstat stat;
#ifdef SYNO_DEBUG_FLAG
	int error;

	if(syno_hibernation_log_level > 0) {
		syno_do_hibernation_filename_log(filename);
	}
	error = vfs_stat(filename, &stat);
#else
	int error = vfs_stat(filename, &stat);
#endif
	if (error)
		return error;
	return cp_new_stat(&stat, statbuf);
}

#ifdef SYNO_ARCHIVE_SET_ALL_SYSCALL
SYSCALL_DEFINE2(SYNOArchiveOverwrite, unsigned int, fd, unsigned int, flags)
{
	int ret = -EBADF;
	struct file *file;
	int fput_needed;
	extern long __SYNOArchiveOverwrite(struct dentry *dentry, unsigned int flags);

	file = fget_light(fd, &fput_needed);
	if (!file) {
		return ret;
	}

	ret = mnt_want_write(file->f_vfsmnt);
	if (ret)
		goto fput_out;

	ret = __SYNOArchiveOverwrite(file->f_path.dentry, flags);
	mnt_drop_write(file->f_vfsmnt);
fput_out:
	fput_light(file, fput_needed);
	return ret;
}
#endif

#ifdef SYNO_ECRYPTFS_FILENAME_SYSCALL
#include "../fs/ecryptfs/ecryptfs_kernel.h"
int (*fecryptfs_decode_and_decrypt_filename)(char **plaintext_name,
                                        size_t *plaintext_name_size,
                                        struct dentry *ecryptfs_dir_dentry,
                                        const char *name, size_t name_size) = NULL;
EXPORT_SYMBOL(fecryptfs_decode_and_decrypt_filename);

asmlinkage long sys_SYNOEcryptName(char __user * src, char __user * dst)
{
	int                               err = -1;
	struct qstr                      *lower_path = NULL;
	struct path                       path;
	struct ecryptfs_dentry_info      *crypt_dentry = NULL;

	if (NULL == src || NULL == dst) {
		return -EINVAL;
	}

	err = user_path_at(AT_FDCWD, src, LOOKUP_FOLLOW, &path);

	if (err) {
		return -ENOENT;
	}
	if (!path.dentry->d_inode->i_sb->s_type || 
		strcmp(path.dentry->d_inode->i_sb->s_type->name, "ecryptfs")) {
		err = -EINVAL;
		goto OUT_RELEASE;
	}
	crypt_dentry = ecryptfs_dentry_to_private(path.dentry);
	if (!crypt_dentry) {
		err = -EINVAL;
		goto OUT_RELEASE;
	}
	lower_path = &crypt_dentry->lower_path.dentry->d_name;
	err = copy_to_user(dst, lower_path->name, lower_path->len + 1);

OUT_RELEASE:
	path_put(&path);

	return err;
}

asmlinkage long sys_SYNODecryptName(char __user * root, char __user * src, char __user * dst)
{
	int                           err;
	size_t                        plaintext_name_size = 0;
	char                         *plaintext_name = NULL;
	char                         *token = NULL;
	char                         *szTarget = NULL;
	char                         *root_name = NULL;
	char                         *src_name = NULL;
	char                         *src_walk = NULL;
	struct path					 path;

	if (NULL == src || NULL == root || NULL == dst) {
		return -EINVAL;
	}
	if (!fecryptfs_decode_and_decrypt_filename) {
		return -EPERM;
	}

	src_name = getname(src);
	if (IS_ERR(src_name)) {
		err = PTR_ERR(src_name);
		goto OUT_RELEASE;
	}
	// strsep() will move src_walk, so we should keep the head for free mem
	src_walk = src_name;
	root_name = getname(root);
	if (IS_ERR(root_name)) {
		err = PTR_ERR(root_name);
		goto OUT_RELEASE;
	}
	szTarget = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!szTarget) {
		err = -ENOMEM;
		goto OUT_RELEASE;
	}
	strncpy(szTarget, root_name, PATH_MAX-1);
	szTarget[PATH_MAX-1] = '\0';

	token = strsep(&src_walk, "/");
	if (*token == '\0') {
		token = strsep(&src_walk, "/");
	}
	while (token) {
		err = kern_path(szTarget, LOOKUP_FOLLOW, &path);
		if (err) {
			goto OUT_RELEASE;
		}
		path.dentry->d_flags |= DCACHE_ECRYPTFS_DECRYPT;
		err = fecryptfs_decode_and_decrypt_filename(
			&plaintext_name, &plaintext_name_size, path.dentry, token, strlen(token));
		path.dentry->d_flags &= ~DCACHE_ECRYPTFS_DECRYPT;
		if (err) {
			path_put(&path);
			goto OUT_RELEASE;
		}
		if (PATH_MAX < strlen(szTarget) + plaintext_name_size + 1) {
			path_put(&path);
			goto OUT_RELEASE;
		}
		strcat(szTarget, "/");
		strcat(szTarget, plaintext_name);

		kfree(plaintext_name);
		plaintext_name = NULL;
		path_put(&path);

		token = strsep(&src_walk, "/");
	}
	err = copy_to_user(dst, szTarget, strlen(szTarget)+1);

OUT_RELEASE:
	if (plaintext_name) {
		kfree(plaintext_name);
	}
	if (szTarget) {
		kfree(szTarget);
	}
	if (!IS_ERR(src_name)) {
		putname(src_name);
	}
	if (!IS_ERR(root_name)) {
		putname(root_name);
	}

	return err;
}
#endif

SYSCALL_DEFINE2(newlstat, const char __user *, filename,
		struct stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;

	return cp_new_stat(&stat, statbuf);
}

#if !defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_SYS_NEWFSTATAT)
SYSCALL_DEFINE4(newfstatat, int, dfd, const char __user *, filename,
		struct stat __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_new_stat(&stat, statbuf);
}
#endif

SYSCALL_DEFINE2(newfstat, unsigned int, fd, struct stat __user *, statbuf)
{
	struct kstat stat;
#ifdef SYNO_DEBUG_FLAG
	int error;

	if(syno_hibernation_log_level > 0) {
		syno_do_hibernation_fd_log(fd);
	}
	error = vfs_fstat(fd, &stat);
#else
	int error = vfs_fstat(fd, &stat);
#endif
	if (!error)
		error = cp_new_stat(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE4(readlinkat, int, dfd, const char __user *, pathname,
		char __user *, buf, int, bufsiz)
{
	struct path path;
	int error;
	int empty = 0;

	if (bufsiz <= 0)
		return -EINVAL;

	error = user_path_at_empty(dfd, pathname, LOOKUP_EMPTY, &path, &empty);
	if (!error) {
		struct inode *inode = path.dentry->d_inode;

		error = empty ? -ENOENT : -EINVAL;
		if (inode->i_op->readlink) {
			error = security_inode_readlink(path.dentry);
			if (!error) {
				touch_atime(path.mnt, path.dentry);
				error = inode->i_op->readlink(path.dentry,
							      buf, bufsiz);
			}
		}
		path_put(&path);
	}
	return error;
}

SYSCALL_DEFINE3(readlink, const char __user *, path, char __user *, buf,
		int, bufsiz)
{
	return sys_readlinkat(AT_FDCWD, path, buf, bufsiz);
}

#ifdef SYNO_UNICODE_STAT
/* This stat is used by caseless protocol.
 * The filename will be convert to real filename and return to user space.
 * In caller, the length of filename must equal or be larger than SYNO_SMB_PSTRING_LEN.
*/
int __SYNOCaselessStat(char __user * filename, int nofollowLink, struct kstat *stat, int *lastComponent, int flags)
{
	struct path path;
	int error;
	int f;
	char *real_filename = NULL;
	int real_filename_len = 0;

	real_filename = kmalloc(SYNO_SMB_PSTRING_LEN, GFP_KERNEL);
	if (!real_filename) {
		return -ENOMEM;
	}

#ifdef MY_ABC_HERE
	if(!nofollowLink && syno_hibernation_log_level > 0) {
		syno_do_hibernation_filename_log(filename);
	}
#endif

#ifdef MY_ABC_HERE
	if (SynoDebugFlag) {
		printk("%s(%d) orig name:[%s] len:[%u]\n", __FUNCTION__, __LINE__, filename, (unsigned int)strlen(filename));
	}
#endif
	if (nofollowLink) {
		f = LOOKUP_CASELESS_COMPARE;
	} else {
		f = LOOKUP_FOLLOW|LOOKUP_CASELESS_COMPARE;
	}
	error = syno_user_path_at(AT_FDCWD, filename, f, &path, &real_filename, &real_filename_len, lastComponent);
	if (!error) {
		error = syno_vfs_getattr(&path, stat, flags);
		path_put(&path);
		if (real_filename_len) {
#ifdef SYNO_DEBUG_FLAG
			if (SynoDebugFlag) {
				printk("%s(%d) convert name:[%s]\n",__FUNCTION__,__LINE__,filename);
			}
#endif
			error = copy_to_user(filename, real_filename, real_filename_len) ? -EFAULT : error;
		}
	}
#ifdef SYNO_DEBUG_FLAG
	if (error && SynoDebugFlag) {
		printk("%s(%d) convert name:[%s], error:[%d]\n",__FUNCTION__,__LINE__,filename, error);
	}
#endif

	kfree(real_filename);
	return error;
}
EXPORT_SYMBOL(__SYNOCaselessStat);

#endif

/* ---------- LFS-64 ----------- */
#ifdef __ARCH_WANT_STAT64

static long cp_new_stat64(struct kstat *stat, struct stat64 __user *statbuf)
{
	struct stat64 tmp;

	memset(&tmp, 0, sizeof(struct stat64));
#ifdef CONFIG_MIPS
	/* mips has weird padding, so we don't get 64 bits there */
	if (!new_valid_dev(stat->dev) || !new_valid_dev(stat->rdev))
		return -EOVERFLOW;
	tmp.st_dev = new_encode_dev(stat->dev);
	tmp.st_rdev = new_encode_dev(stat->rdev);
#else
	tmp.st_dev = huge_encode_dev(stat->dev);
	tmp.st_rdev = huge_encode_dev(stat->rdev);
#endif
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
#ifdef STAT64_HAS_BROKEN_ST_INO
	tmp.__st_ino = stat->ino;
#endif
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	tmp.st_uid = stat->uid;
	tmp.st_gid = stat->gid;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime = stat->ctime.tv_sec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
	tmp.st_size = stat->size;
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(stat64, const char __user *, filename,
		struct stat64 __user *, statbuf)
{
	struct kstat stat;

#ifdef SYNO_DEBUG_FLAG
	if(syno_hibernation_log_level > 0) {
		syno_do_hibernation_filename_log(filename);
	}
#endif

	int error = vfs_stat(filename, &stat);
	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE2(lstat64, const char __user *, filename,
		struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_lstat(filename, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE2(fstat64, unsigned long, fd, struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE4(fstatat64, int, dfd, const char __user *, filename,
		struct stat64 __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_new_stat64(&stat, statbuf);
}

#ifdef SYNO_STAT
struct SYNOSTAT64 {
	struct stat64 st;
	struct SYNOSTAT_EXTRA ext;
};

static int SYNOStat64CopyToUser(struct kstat *pKst, unsigned int flags, struct SYNOSTAT64 __user * pSt64)
{
	int error = -EFAULT;

	if (pSt64) {
		if (flags & SYNOST_STAT) {
			error = cp_new_stat64(pKst, &pSt64->st);
		}
#ifdef SYNO_ARCHIVE_BIT
		if (flags & SYNOST_ARBIT) {
			if (__put_user(pKst->SynoMode, &pSt64->ext.archBit)){
				goto Out;
			}
		}
#endif /* SYNO_ARCHIVE_BIT */
#ifdef SYNO_CREATE_TIME
		if (flags & SYNOST_CREATIME) {
			if (copy_to_user(&pSt64->ext.creatTime, &pKst->SynoCreateTime, sizeof(pSt64->ext.creatTime))){
				goto Out;
			}
		}
#endif /* SYNO_CREATE_TIME */
#ifdef SYNO_ARCHIVE_VERSION
		if (flags & SYNOST_BKPVER) {
			if (__put_user(pKst->syno_archive_version, &pSt64->ext.bkpVer)){
				goto Out;
			}
		}
#endif /* SYNO_ARCHIVE_VERSION */
	} else {
		error = -EINVAL;
		goto Out;
	}

	error = 0;
Out:
	return error;
}

static int do_SYNOStat64(char __user * filename, int nofollowLink, int flags, struct SYNOSTAT64 __user * pSt64)
{
	long error = -EINVAL;
	int lastComponent = 0;
	struct kstat kst;

	if (flags & SYNOST_IS_CASELESS) {
#ifdef SYNO_UNICODE_STAT
		error = __SYNOCaselessStat(filename, nofollowLink, &kst, &lastComponent, flags);
		if (-ENOENT == error) {
			if (pSt64){
				if (__put_user(lastComponent, &pSt64->ext.lastComponent)){
					goto Out;
				}
			}
		}
#else
		error = -EOPNOTSUPP;
#endif
	} else {
		if (nofollowLink) {
			error = syno_vfs_stat(filename, &kst, 0, flags);
		} else {
#ifdef SYNO_DEBUG_FLAG
			if(syno_hibernation_log_level > 0) {
				syno_do_hibernation_filename_log(filename);
			}
#endif
			error = syno_vfs_stat(filename, &kst, LOOKUP_FOLLOW, flags);
		}
	}

	if (error) {
		goto Out;
	}

	error = SYNOStat64CopyToUser(&kst, flags, pSt64);
Out:
	return error;
}

static int do_SYNOFStat64(unsigned int fd, int flags, struct SYNOSTAT64 __user * pSt)
{
	int error;
	struct kstat kst;

	error = syno_vfs_fstat(fd, &kst, flags);
	if (!error) {
		error = SYNOStat64CopyToUser(&kst, flags, pSt);
	}
	return error;
}

SYSCALL_DEFINE3(SYNOFStat64, unsigned int, fd, unsigned int, flags, struct SYNOSTAT64 __user *, pSt)
{
	return do_SYNOFStat64(fd, flags, pSt);
}

SYSCALL_DEFINE3(SYNOStat64, char __user *, filename, unsigned int, flags, struct SYNOSTAT64 __user *, pSt)
{
	return do_SYNOStat64(filename, 0, flags, pSt);
}

SYSCALL_DEFINE3(SYNOLStat64, char __user *, filename, unsigned int, flags, struct SYNOSTAT64 __user *, pSt)
{
	return do_SYNOStat64(filename, 1, flags, pSt);
}
#endif /* SYNO_STAT */

#ifdef SYNO_UNICODE_STAT
asmlinkage long sys_SYNOCaselessStat64(char __user * filename, struct stat64 __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, 0, &stat, &lastComponent, 0);
	if (!error) {
		error = cp_new_stat64(&stat, statbuf);
	}

	return error;
}

asmlinkage long sys_SYNOCaselessLStat64(char __user * filename, struct stat64 __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, 1, &stat, &lastComponent, 0);
	if (!error) {
		error = cp_new_stat64(&stat, statbuf);
	}

	return error;
}
#endif /* SYNO_UNICODE_STAT */
#else /* __ARCH_WANT_STAT64 */

#ifdef SYNO_STAT
static int SYNOStatCopyToUser(struct kstat *pKst, unsigned int flags, struct SYNOSTAT __user * pSt)
{
	int error = -EFAULT;

	if (pSt) {
		if (flags & SYNOST_STAT) {
			if(0 != (error = cp_new_stat(pKst, &pSt->st))){
				goto Out;
			}
		}
#ifdef SYNO_ARCHIVE_BIT
		if (flags & SYNOST_ARBIT) {
			if (__put_user(pKst->SynoMode, &pSt->ext.archBit)){
				goto Out;
			}
		}
#endif /* SYNO_ARCHIVE_BIT */
#ifdef SYNO_CREATE_TIME
		if (flags & SYNOST_CREATIME) {
			if (copy_to_user(&pSt->ext.creatTime, &pKst->SynoCreateTime, sizeof(pSt->ext.creatTime))){
				goto Out;
			}
		}
#endif
#ifdef SYNO_ARCHIVE_VERSION
		if (flags & SYNOST_BKPVER) {
			if (__put_user(pKst->syno_archive_version, &pSt->ext.bkpVer)){
				goto Out;
			}
		}
#endif /* SYNO_ARCHIVE_VERSION */
	} else {
		error = -EINVAL;
		goto Out;
	}

	error = 0;
Out:
	return error;
}
static int do_SYNOStat(char __user * filename, int nofollowLink, int flags, struct SYNOSTAT __user * pSt)
{
	long error = -EINVAL;
	int lastComponent = 0;
	struct kstat kst;

	if (flags & SYNOST_IS_CASELESS) {
#ifdef SYNO_UNICODE_STAT
		error = __SYNOCaselessStat(filename, nofollowLink, &kst, &lastComponent, flags);
		if (-ENOENT == error) {
			if (pSt) {
				if (__put_user(lastComponent, &pSt->ext.lastComponent)){
					goto Out;
				}
			}
		}
#else
		error = -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
	} else {
		if (nofollowLink) {
			error = syno_vfs_stat(filename, &kst, 0, flags);
		} else {
#ifdef SYNO_DEBUG_FLAG
			if(syno_hibernation_log_level > 0) {
				syno_do_hibernation_filename_log(filename);
			}
#endif /* MY_ABC_HERE */
			error = syno_vfs_stat(filename, &kst, LOOKUP_FOLLOW, flags);
		}
	}

	if (error) {
		goto Out;
	}

	error = SYNOStatCopyToUser(&kst, flags, pSt);
Out:
	return error;
}

static int do_SYNOFStat(unsigned int fd, int flags, struct SYNOSTAT __user * pSt)
{
	int error;
	struct kstat kst;

	error = syno_vfs_fstat(fd, &kst, flags);
	if (!error) {
		error = SYNOStatCopyToUser(&kst, flags, pSt);
	}
	return error;
}

SYSCALL_DEFINE3(SYNOFStat, unsigned int, fd, unsigned int, flags, struct SYNOSTAT __user *, pSt)
{
	return do_SYNOFStat(fd, flags, pSt);
}

SYSCALL_DEFINE3(SYNOStat, char __user *, filename, unsigned int, flags, struct SYNOSTAT __user *, pSt)
{
	return do_SYNOStat(filename, 0, flags, pSt);
}

SYSCALL_DEFINE3(SYNOLStat, char __user *, filename, unsigned int, flags, struct SYNOSTAT __user *, pSt)
{
	return do_SYNOStat(filename, 1, flags, pSt);
}
#endif /* SYNO_STAT */

#ifdef SYNO_UNICODE_STAT
asmlinkage long sys_SYNOCaselessStat(char __user * filename, struct stat __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, 0, &stat, &lastComponent, 0);
	if (!error) {
		error = cp_new_stat(&stat, statbuf);
	}

	return error;
}

asmlinkage long sys_SYNOCaselessLStat(char __user * filename, struct stat __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, 1, &stat, &lastComponent, 0);
	if (!error) {
		error = cp_new_stat(&stat, statbuf);
	}

	return error;
}
#endif /* SYNO_UNICODE_STAT */
#endif /* __ARCH_WANT_STAT64 */

/* Caller is here responsible for sufficient locking (ie. inode->i_lock) */
void __inode_add_bytes(struct inode *inode, loff_t bytes)
{
	inode->i_blocks += bytes >> 9;
	bytes &= 511;
	inode->i_bytes += bytes;
	if (inode->i_bytes >= 512) {
		inode->i_blocks++;
		inode->i_bytes -= 512;
	}
}

void inode_add_bytes(struct inode *inode, loff_t bytes)
{
	spin_lock(&inode->i_lock);
	__inode_add_bytes(inode, bytes);
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(inode_add_bytes);

void inode_sub_bytes(struct inode *inode, loff_t bytes)
{
	spin_lock(&inode->i_lock);
	inode->i_blocks -= bytes >> 9;
	bytes &= 511;
	if (inode->i_bytes < bytes) {
		inode->i_blocks--;
		inode->i_bytes += 512;
	}
	inode->i_bytes -= bytes;
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(inode_sub_bytes);

loff_t inode_get_bytes(struct inode *inode)
{
	loff_t ret;

	spin_lock(&inode->i_lock);
	ret = (((loff_t)inode->i_blocks) << 9) + inode->i_bytes;
	spin_unlock(&inode->i_lock);
	return ret;
}

EXPORT_SYMBOL(inode_get_bytes);

void inode_set_bytes(struct inode *inode, loff_t bytes)
{
	/* Caller is here responsible for sufficient locking
	 * (ie. inode->i_lock) */
	inode->i_blocks = bytes >> 9;
	inode->i_bytes = bytes & 511;
}

EXPORT_SYMBOL(inode_set_bytes);
