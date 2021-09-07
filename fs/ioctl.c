#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/capability.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/security.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/writeback.h>
#include <linux/buffer_head.h>
#include <linux/falloc.h>

#include <asm/ioctls.h>

#define FIEMAP_MAX_EXTENTS	(UINT_MAX / sizeof(struct fiemap_extent))

static long vfs_ioctl(struct file *filp, unsigned int cmd,
		      unsigned long arg)
{
	int error = -ENOTTY;

	if (!filp->f_op)
		goto out;

	if (filp->f_op->unlocked_ioctl) {
		error = filp->f_op->unlocked_ioctl(filp, cmd, arg);
		if (error == -ENOIOCTLCMD)
			error = -EINVAL;
		goto out;
	} else if (filp->f_op->ioctl) {
		lock_kernel();
		error = filp->f_op->ioctl(filp->f_path.dentry->d_inode,
					  filp, cmd, arg);
		unlock_kernel();
	}

 out:
	return error;
}

static int ioctl_fibmap(struct file *filp, int __user *p)
{
	struct address_space *mapping = filp->f_mapping;
	int res, block;

	if (!mapping->a_ops->bmap)
		return -EINVAL;
	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	res = get_user(block, p);
	if (res)
		return res;
	res = mapping->a_ops->bmap(mapping, block);
	return put_user(res, p);
}

#define SET_UNKNOWN_FLAGS	(FIEMAP_EXTENT_DELALLOC)
#define SET_NO_UNMOUNTED_IO_FLAGS	(FIEMAP_EXTENT_DATA_ENCRYPTED)
#define SET_NOT_ALIGNED_FLAGS	(FIEMAP_EXTENT_DATA_TAIL|FIEMAP_EXTENT_DATA_INLINE)
int fiemap_fill_next_extent(struct fiemap_extent_info *fieinfo, u64 logical,
			    u64 phys, u64 len, u32 flags)
{
	struct fiemap_extent extent;
	struct fiemap_extent *dest = fieinfo->fi_extents_start;

	if (fieinfo->fi_extents_max == 0) {
		fieinfo->fi_extents_mapped++;
		return (flags & FIEMAP_EXTENT_LAST) ? 1 : 0;
	}

	if (fieinfo->fi_extents_mapped >= fieinfo->fi_extents_max)
		return 1;

	if (flags & SET_UNKNOWN_FLAGS)
		flags |= FIEMAP_EXTENT_UNKNOWN;
	if (flags & SET_NO_UNMOUNTED_IO_FLAGS)
		flags |= FIEMAP_EXTENT_ENCODED;
	if (flags & SET_NOT_ALIGNED_FLAGS)
		flags |= FIEMAP_EXTENT_NOT_ALIGNED;

	memset(&extent, 0, sizeof(extent));
	extent.fe_logical = logical;
	extent.fe_physical = phys;
	extent.fe_length = len;
	extent.fe_flags = flags;

	dest += fieinfo->fi_extents_mapped;

#ifdef CONFIG_SYNO_PLX_PORTING
	if(fieinfo->fi_flags & FIEMAP_KERNEL_READ) {
		memcpy(dest, &extent, sizeof(extent));
	} else {
#endif
	if (copy_to_user(dest, &extent, sizeof(extent)))
		return -EFAULT;
#ifdef CONFIG_SYNO_PLX_PORTING
	}
#endif

	fieinfo->fi_extents_mapped++;
	if (fieinfo->fi_extents_mapped == fieinfo->fi_extents_max)
		return 1;
	return (flags & FIEMAP_EXTENT_LAST) ? 1 : 0;
}
EXPORT_SYMBOL(fiemap_fill_next_extent);

int fiemap_check_flags(struct fiemap_extent_info *fieinfo, u32 fs_flags)
{
	u32 incompat_flags;

	incompat_flags = fieinfo->fi_flags & ~(FIEMAP_FLAGS_COMPAT & fs_flags);
	if (incompat_flags) {
		fieinfo->fi_flags = incompat_flags;
		return -EBADR;
	}
	return 0;
}
EXPORT_SYMBOL(fiemap_check_flags);

static int fiemap_check_ranges(struct super_block *sb,
			       u64 start, u64 len, u64 *new_len)
{
	u64 maxbytes = (u64) sb->s_maxbytes;

	*new_len = len;

	if (len == 0)
		return -EINVAL;

	if (start > maxbytes)
		return -EFBIG;

	if (len > maxbytes || (maxbytes - len) < start)
		*new_len = maxbytes - start;

	return 0;
}

static int ioctl_fiemap(struct file *filp, unsigned long arg)
{
	struct fiemap fiemap;
	struct fiemap_extent_info fieinfo = { 0, };
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	u64 len;
	int error;

	if (!inode->i_op->fiemap)
		return -EOPNOTSUPP;

	if (copy_from_user(&fiemap, (struct fiemap __user *)arg,
			   sizeof(struct fiemap)))
		return -EFAULT;

	if (fiemap.fm_extent_count > FIEMAP_MAX_EXTENTS)
		return -EINVAL;

	error = fiemap_check_ranges(sb, fiemap.fm_start, fiemap.fm_length,
				    &len);
	if (error)
		return error;

	fieinfo.fi_flags = fiemap.fm_flags;
	fieinfo.fi_extents_max = fiemap.fm_extent_count;
	fieinfo.fi_extents_start = (struct fiemap_extent *)(arg + sizeof(fiemap));

	if (fiemap.fm_extent_count != 0 &&
	    !access_ok(VERIFY_WRITE, fieinfo.fi_extents_start,
		       fieinfo.fi_extents_max * sizeof(struct fiemap_extent)))
		return -EFAULT;

	if (fieinfo.fi_flags & FIEMAP_FLAG_SYNC)
		filemap_write_and_wait(inode->i_mapping);

	error = inode->i_op->fiemap(inode, &fieinfo, fiemap.fm_start, len);
	fiemap.fm_flags = fieinfo.fi_flags;
	fiemap.fm_mapped_extents = fieinfo.fi_extents_mapped;
	if (copy_to_user((char *)arg, &fiemap, sizeof(fiemap)))
		error = -EFAULT;

	return error;
}

#ifdef CONFIG_BLOCK

#define blk_to_logical(inode, blk) (blk << (inode)->i_blkbits)
#define logical_to_blk(inode, offset) (offset >> (inode)->i_blkbits);

int __generic_block_fiemap(struct inode *inode,
			   struct fiemap_extent_info *fieinfo, u64 start,
			   u64 len, get_block_t *get_block)
{
	struct buffer_head tmp;
	unsigned long long start_blk;
	long long length = 0, map_len = 0;
	u64 logical = 0, phys = 0, size = 0;
	u32 flags = FIEMAP_EXTENT_MERGED;
	int ret = 0, past_eof = 0, whole_file = 0;

	if ((ret = fiemap_check_flags(fieinfo, FIEMAP_FLAG_SYNC)))
		return ret;

	start_blk = logical_to_blk(inode, start);

	length = (long long)min_t(u64, len, i_size_read(inode));
	if (length < len)
		whole_file = 1;

	map_len = length;

	do {
		 
		memset(&tmp, 0, sizeof(struct buffer_head));
		tmp.b_size = map_len;

		ret = get_block(inode, start_blk, &tmp, 0);
		if (ret)
			break;

		if (!buffer_mapped(&tmp)) {
			length -= blk_to_logical(inode, 1);
			start_blk++;

			if (!past_eof &&
			    blk_to_logical(inode, start_blk) >=
			    blk_to_logical(inode, 0)+i_size_read(inode))
				past_eof = 1;

			if (past_eof && size) {
				flags = FIEMAP_EXTENT_MERGED|FIEMAP_EXTENT_LAST;
				ret = fiemap_fill_next_extent(fieinfo, logical,
							      phys, size,
							      flags);
				break;
			}

			if (length <= 0 || past_eof)
				break;
		} else {
			 
			if (length <= 0 && !whole_file) {
				ret = fiemap_fill_next_extent(fieinfo, logical,
							      phys, size,
							      flags);
				break;
			}

			if (size) {
				ret = fiemap_fill_next_extent(fieinfo, logical,
							      phys, size,
							      flags);
				if (ret)
					break;
			}

			logical = blk_to_logical(inode, start_blk);
			phys = blk_to_logical(inode, tmp.b_blocknr);
			size = tmp.b_size;
			flags = FIEMAP_EXTENT_MERGED;

			length -= tmp.b_size;
			start_blk += logical_to_blk(inode, size);

			if (!past_eof &&
			    logical+size >=
			    blk_to_logical(inode, 0)+i_size_read(inode))
				past_eof = 1;
		}
		cond_resched();
	} while (1);

	if (ret == 1)
		ret = 0;

	return ret;
}
EXPORT_SYMBOL(__generic_block_fiemap);

int generic_block_fiemap(struct inode *inode,
			 struct fiemap_extent_info *fieinfo, u64 start,
			 u64 len, get_block_t *get_block)
{
	int ret;
	mutex_lock(&inode->i_mutex);
	ret = __generic_block_fiemap(inode, fieinfo, start, len, get_block);
	mutex_unlock(&inode->i_mutex);
	return ret;
}
EXPORT_SYMBOL(generic_block_fiemap);

#endif   

int ioctl_preallocate(struct file *filp, void __user *argp)
{
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct space_resv sr;

	if (copy_from_user(&sr, argp, sizeof(sr)))
		return -EFAULT;

	switch (sr.l_whence) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		sr.l_start += filp->f_pos;
		break;
	case SEEK_END:
		sr.l_start += i_size_read(inode);
		break;
	default:
		return -EINVAL;
	}

	return do_fallocate(filp, FALLOC_FL_KEEP_SIZE, sr.l_start, sr.l_len);
}

static int file_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct inode *inode = filp->f_path.dentry->d_inode;
	int __user *p = (int __user *)arg;

	switch (cmd) {
	case FIBMAP:
		return ioctl_fibmap(filp, p);
	case FIONREAD:
		return put_user(i_size_read(inode) - filp->f_pos, p);
	case FS_IOC_RESVSP:
	case FS_IOC_RESVSP64:
		return ioctl_preallocate(filp, p);
	}

	return vfs_ioctl(filp, cmd, arg);
}

#ifdef MY_ABC_HERE
static int archive_check_capable(struct inode *inode)
{
	if ((!S_ISDIR(inode->i_mode)) && (!S_ISREG(inode->i_mode)))
		return -EPERM;
	
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!inode->i_sb->s_op->syno_set_sb_archive_ver)
		return -EINVAL;
	if (!inode->i_sb->s_op->syno_get_sb_archive_ver)
		return -EINVAL;

	return 0;
}

static int ioctl_get_version(struct inode *inode, unsigned int *p_ver)
{
	int error;
	struct super_block *sb = inode->i_sb;

	error = archive_check_capable(inode);
	if (error)
		return error;

	error = sb->s_op->syno_get_sb_archive_ver(sb, p_ver);
	return error;
}

static int ioctl_set_version(struct inode *inode, unsigned int version)
{
	int error;
	struct super_block *sb = inode->i_sb;

	error = archive_check_capable(inode);
	if (error)
		return error;
	if ((UINT_MAX - 1) <= version) {
		return -EPERM;
	}
	mutex_lock(&sb->s_archive_mutex);
	error = sb->s_op->syno_set_sb_archive_ver(sb, version);
	mutex_unlock(&sb->s_archive_mutex);
	return error;
}

static int ioctl_inc_version(struct inode *inode)
{
	unsigned int ver;
	int error;
	struct super_block *sb = inode->i_sb;

	error = archive_check_capable(inode);
	if (error)
		return error;

	mutex_lock(&sb->s_archive_mutex);
	error = sb->s_op->syno_get_sb_archive_ver(sb, &ver);
	if (error)
		goto unlock;

	if ((UINT_MAX - 1) <= (ver + 1)) {
		error = -EPERM;
		goto unlock;
	}
	error = sb->s_op->syno_set_sb_archive_ver(sb, ver + 1);
unlock:
	mutex_unlock(&sb->s_archive_mutex);
	return error;
}
#endif

static int ioctl_fionbio(struct file *filp, int __user *argp)
{
	unsigned int flag;
	int on, error;

	error = get_user(on, argp);
	if (error)
		return error;
	flag = O_NONBLOCK;
#ifdef __sparc__
	 
	if (O_NONBLOCK != O_NDELAY)
		flag |= O_NDELAY;
#endif
	spin_lock(&filp->f_lock);
	if (on)
		filp->f_flags |= flag;
	else
		filp->f_flags &= ~flag;
	spin_unlock(&filp->f_lock);
	return error;
}

static int ioctl_fioasync(unsigned int fd, struct file *filp,
			  int __user *argp)
{
	unsigned int flag;
	int on, error;

	error = get_user(on, argp);
	if (error)
		return error;
	flag = on ? FASYNC : 0;

	if ((flag ^ filp->f_flags) & FASYNC) {
		if (filp->f_op && filp->f_op->fasync)
			 
			error = filp->f_op->fasync(fd, filp, on);
		else
			error = -ENOTTY;
	}
	return error < 0 ? error : 0;
}

static int ioctl_fsfreeze(struct file *filp)
{
	struct super_block *sb = filp->f_path.dentry->d_inode->i_sb;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (sb->s_op->freeze_fs == NULL)
		return -EOPNOTSUPP;

	if (sb->s_bdev == NULL)
		return -EINVAL;

	sb = freeze_bdev(sb->s_bdev);
	if (IS_ERR(sb))
		return PTR_ERR(sb);
	return 0;
}

static int ioctl_fsthaw(struct file *filp)
{
	struct super_block *sb = filp->f_path.dentry->d_inode->i_sb;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (sb->s_bdev == NULL)
		return -EINVAL;

	return thaw_bdev(sb->s_bdev, sb);
}

int do_vfs_ioctl(struct file *filp, unsigned int fd, unsigned int cmd,
	     unsigned long arg)
{
	int error = 0;
	int __user *argp = (int __user *)arg;
#ifdef MY_ABC_HERE
	unsigned int ver = 0;
#endif

	switch (cmd) {
	case FIOCLEX:
		set_close_on_exec(fd, 1);
		break;

	case FIONCLEX:
		set_close_on_exec(fd, 0);
		break;

	case FIONBIO:
		error = ioctl_fionbio(filp, argp);
		break;

	case FIOASYNC:
		error = ioctl_fioasync(fd, filp, argp);
		break;

	case FIOQSIZE:
		if (S_ISDIR(filp->f_path.dentry->d_inode->i_mode) ||
		    S_ISREG(filp->f_path.dentry->d_inode->i_mode) ||
		    S_ISLNK(filp->f_path.dentry->d_inode->i_mode)) {
			loff_t res =
				inode_get_bytes(filp->f_path.dentry->d_inode);
			error = copy_to_user((loff_t __user *)arg, &res,
					     sizeof(res)) ? -EFAULT : 0;
		} else
			error = -ENOTTY;
		break;

	case FIFREEZE:
		error = ioctl_fsfreeze(filp);
		break;

	case FITHAW:
		error = ioctl_fsthaw(filp);
		break;

	case FS_IOC_FIEMAP:
		return ioctl_fiemap(filp, arg);

	case FIGETBSZ:
	{
		struct inode *inode = filp->f_path.dentry->d_inode;
		int __user *p = (int __user *)arg;
		return put_user(inode->i_sb->s_blocksize, p);
	}

#ifdef MY_ABC_HERE
	case FIGETVERSION:
		error = ioctl_get_version(filp->f_path.dentry->d_inode, &ver);
		if (!error) {
			error = put_user(ver, (unsigned int __user *)arg) ? -EFAULT : 0;
		}
		break;
	case FISETVERSION:
		if ((error = get_user(ver, (unsigned int __user *)arg)) != 0)
			break;
		error = ioctl_set_version(filp->f_path.dentry->d_inode, ver);
		break;
	case FIINCVERSION:
		error = ioctl_inc_version(filp->f_path.dentry->d_inode);
		break;
#endif

	default:
		if (S_ISREG(filp->f_path.dentry->d_inode->i_mode))
			error = file_ioctl(filp, cmd, arg);
		else
			error = vfs_ioctl(filp, cmd, arg);
		break;
	}
	return error;
}

SYSCALL_DEFINE3(ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg)
{
	struct file *filp;
	int error = -EBADF;
	int fput_needed;

	filp = fget_light(fd, &fput_needed);
	if (!filp)
		goto out;

	error = security_file_ioctl(filp, cmd, arg);
	if (error)
		goto out_fput;

	error = do_vfs_ioctl(filp, fd, cmd, arg);
 out_fput:
	fput_light(filp, fput_needed);
 out:
	return error;
}
