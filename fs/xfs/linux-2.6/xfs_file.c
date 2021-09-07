 
#include "xfs.h"
#include "xfs_bit.h"
#include "xfs_log.h"
#include "xfs_inum.h"
#include "xfs_sb.h"
#include "xfs_ag.h"
#include "xfs_dir2.h"
#include "xfs_trans.h"
#include "xfs_dmapi.h"
#include "xfs_mount.h"
#include "xfs_bmap_btree.h"
#include "xfs_alloc_btree.h"
#include "xfs_ialloc_btree.h"
#include "xfs_alloc.h"
#include "xfs_btree.h"
#include "xfs_attr_sf.h"
#include "xfs_dir2_sf.h"
#include "xfs_dinode.h"
#include "xfs_inode.h"
#include "xfs_error.h"
#include "xfs_rw.h"
#include "xfs_vnodeops.h"
#include "xfs_da_btree.h"
#include "xfs_ioctl.h"
#ifdef CONFIG_SYNO_PLX_PORTING
#include "xfs_iomap.h"
#endif

#include <linux/dcache.h>

static const struct vm_operations_struct xfs_file_vm_ops;

STATIC ssize_t
xfs_file_aio_read(
	struct kiocb		*iocb,
	const struct iovec	*iov,
	unsigned long		nr_segs,
	loff_t			pos)
{
	struct file		*file = iocb->ki_filp;
	int			ioflags = IO_ISAIO;

	BUG_ON(iocb->ki_pos != pos);
	if (unlikely(file->f_flags & O_DIRECT))
		ioflags |= IO_ISDIRECT;
	if (file->f_mode & FMODE_NOCMTIME)
		ioflags |= IO_INVIS;
	return xfs_read(XFS_I(file->f_path.dentry->d_inode), iocb, iov,
				nr_segs, &iocb->ki_pos, ioflags);
}

STATIC ssize_t
xfs_file_aio_write(
	struct kiocb		*iocb,
	const struct iovec	*iov,
	unsigned long		nr_segs,
	loff_t			pos)
{
	struct file		*file = iocb->ki_filp;
	int			ioflags = IO_ISAIO;

	BUG_ON(iocb->ki_pos != pos);
	if (unlikely(file->f_flags & O_DIRECT))
		ioflags |= IO_ISDIRECT;
	if (file->f_mode & FMODE_NOCMTIME)
		ioflags |= IO_INVIS;
	return xfs_write(XFS_I(file->f_mapping->host), iocb, iov, nr_segs,
				&iocb->ki_pos, ioflags);
}

#ifdef CONFIG_SYNO_PLX_PORTING
STATIC ssize_t
xfs_file_sendfile(
	struct file *filp,
	loff_t		*pos,
	size_t		 count,
	read_actor_t actor,
	void		*target)
{
	return xfs_sendfile(XFS_I(filp->f_path.dentry->d_inode), filp, pos, 0, count, actor, target, 0);
}

STATIC ssize_t
xfs_file_sendfile_incoherent(
	struct file	 *filp,
	loff_t		 *pos,
	size_t		  count,
	read_actor_t  actor,
	void		 *target)
{
	return xfs_sendfile(XFS_I(filp->f_path.dentry->d_inode), filp, pos, 0, count, actor, target, 1);
}
#endif

STATIC ssize_t
xfs_file_splice_read(
	struct file		*infilp,
	loff_t			*ppos,
	struct pipe_inode_info	*pipe,
	size_t			len,
	unsigned int		flags)
{
	int			ioflags = 0;

	if (infilp->f_mode & FMODE_NOCMTIME)
		ioflags |= IO_INVIS;

	return xfs_splice_read(XFS_I(infilp->f_path.dentry->d_inode),
				   infilp, ppos, pipe, len, flags, ioflags);
}

STATIC ssize_t
xfs_file_splice_write(
	struct pipe_inode_info	*pipe,
	struct file		*outfilp,
	loff_t			*ppos,
	size_t			len,
	unsigned int		flags)
{
	int			ioflags = 0;

	if (outfilp->f_mode & FMODE_NOCMTIME)
		ioflags |= IO_INVIS;

	return xfs_splice_write(XFS_I(outfilp->f_path.dentry->d_inode),
				    pipe, outfilp, ppos, len, flags, ioflags);
}

STATIC int
xfs_file_open(
	struct inode	*inode,
	struct file	*file)
{
	if (!(file->f_flags & O_LARGEFILE) && i_size_read(inode) > MAX_NON_LFS)
		return -EFBIG;
	if (XFS_FORCED_SHUTDOWN(XFS_M(inode->i_sb)))
		return -EIO;
	return 0;
}

STATIC int
xfs_dir_open(
	struct inode	*inode,
	struct file	*file)
{
	struct xfs_inode *ip = XFS_I(inode);
	int		mode;
	int		error;

	error = xfs_file_open(inode, file);
	if (error)
		return error;

	mode = xfs_ilock_map_shared(ip);
	if (ip->i_d.di_nextents > 0)
		xfs_da_reada_buf(NULL, ip, 0, XFS_DATA_FORK);
	xfs_iunlock(ip, mode);
	return 0;
}

STATIC int
xfs_file_release(
	struct inode	*inode,
	struct file	*filp)
{
	return -xfs_release(XFS_I(inode));
}

STATIC int
xfs_file_fsync(
	struct file		*file,
	struct dentry		*dentry,
	int			datasync)
{
	struct xfs_inode	*ip = XFS_I(dentry->d_inode);

	xfs_iflags_clear(ip, XFS_ITRUNCATED);
	return -xfs_fsync(ip);
}

STATIC int
xfs_file_readdir(
	struct file	*filp,
	void		*dirent,
	filldir_t	filldir)
{
	struct inode	*inode = filp->f_path.dentry->d_inode;
	xfs_inode_t	*ip = XFS_I(inode);
	int		error;
	size_t		bufsize;

	bufsize = (size_t)min_t(loff_t, PAGE_SIZE, ip->i_d.di_size);

	error = xfs_readdir(ip, dirent, bufsize,
				(xfs_off_t *)&filp->f_pos, filldir);
	if (error)
		return -error;
	return 0;
}

STATIC int
xfs_file_mmap(
	struct file	*filp,
	struct vm_area_struct *vma)
{
	vma->vm_ops = &xfs_file_vm_ops;
	vma->vm_flags |= VM_CAN_NONLINEAR;

	file_accessed(filp);
	return 0;
}

STATIC int
xfs_vm_page_mkwrite(
	struct vm_area_struct	*vma,
	struct vm_fault		*vmf)
{
	return block_page_mkwrite(vma, vmf, xfs_get_blocks);
}

#ifdef CONFIG_SYNO_PLX_PORTING
STATIC ssize_t
xfs_file_aio_direct_netrx_write(
	struct kiocb *iocb,
	void         *callback,
	void         *sock)
{
	return xfs_direct_netrx_write(iocb, callback, sock);
}

STATIC int
xfs_preallocate(
	struct file	*filp,
	loff_t		 start,
	loff_t		 length)
{
	struct inode	 *inode = filp->f_path.dentry->d_inode;
	struct xfs_inode *ip = XFS_I(inode);
	int               ioflags = 0;
	xfs_flock64_t     info;
 
	info.l_whence = 0;
	info.l_start  = start;
	info.l_len    = length;
	info.l_type   = 0;
	info.l_sysid  = 0;
	info.l_pid    = 0;

	if (filp->f_mode & FMODE_NOCMTIME) {
		ioflags |= IO_INVIS;
	}

	return xfs_ioc_space(ip, inode, filp, ioflags, XFS_IOC_RESVSP64, &info);
}

STATIC int
xfs_unpreallocate(
	struct file	*filp,
	loff_t		 start,
	loff_t		 length)
{
	struct inode	 *inode = filp->f_path.dentry->d_inode;
	struct xfs_inode *ip = XFS_I(inode);
	int               ioflags = 0;
	xfs_flock64_t     info;
 
	info.l_whence = 0;
	info.l_start  = start;
	info.l_len    = length;
	info.l_type   = 0;
	info.l_sysid  = 0;
	info.l_pid    = 0;

	if (filp->f_mode & FMODE_NOCMTIME) {
		ioflags |= IO_INVIS;
	}

	return xfs_ioc_space(ip, inode, filp, ioflags, XFS_IOC_UNRESVSP64, &info);
}

STATIC int
xfs_reset_preallocate(
	struct file	*filp,
	loff_t       start,
	loff_t      length)
{
	struct inode     *inode = filp->f_path.dentry->d_inode;
	struct xfs_inode *ip = XFS_I(inode);

	return xfs_iomap_write_unwritten(ip, start, length);
}
#endif

const struct file_operations xfs_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.aio_read	= xfs_file_aio_read,
	.aio_write	= xfs_file_aio_write,
#ifdef CONFIG_SYNO_PLX_PORTING
	.aio_direct_netrx_write = xfs_file_aio_direct_netrx_write,
	.sendfile		= xfs_file_sendfile,
	.incoherent_sendfile	= xfs_file_sendfile_incoherent,
#endif
	.splice_read	= xfs_file_splice_read,
	.splice_write	= xfs_file_splice_write,
	.unlocked_ioctl	= xfs_file_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= xfs_file_compat_ioctl,
#endif
	.mmap		= xfs_file_mmap,
	.open		= xfs_file_open,
	.release	= xfs_file_release,
	.fsync		= xfs_file_fsync,
#ifdef HAVE_FOP_OPEN_EXEC
	.open_exec	= xfs_file_open_exec,
#endif
#ifdef CONFIG_SYNO_PLX_PORTING
	.preallocate = xfs_preallocate,
	.unpreallocate = xfs_unpreallocate,
	.resetpreallocate = xfs_reset_preallocate,
#endif
};

const struct file_operations xfs_dir_file_operations = {
	.open		= xfs_dir_open,
	.read		= generic_read_dir,
	.readdir	= xfs_file_readdir,
	.llseek		= generic_file_llseek,
	.unlocked_ioctl	= xfs_file_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= xfs_file_compat_ioctl,
#endif
	.fsync		= xfs_file_fsync,
};

static const struct vm_operations_struct xfs_file_vm_ops = {
	.fault		= filemap_fault,
	.page_mkwrite	= xfs_vm_page_mkwrite,
};
