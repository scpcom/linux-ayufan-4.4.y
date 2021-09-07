 
#include "xfs.h"
#include "xfs_fs.h"
#include "xfs_bit.h"
#include "xfs_log.h"
#include "xfs_inum.h"
#include "xfs_trans.h"
#include "xfs_sb.h"
#include "xfs_ag.h"
#include "xfs_dir2.h"
#include "xfs_alloc.h"
#include "xfs_dmapi.h"
#include "xfs_quota.h"
#include "xfs_mount.h"
#include "xfs_bmap_btree.h"
#include "xfs_alloc_btree.h"
#include "xfs_ialloc_btree.h"
#include "xfs_dir2_sf.h"
#include "xfs_attr_sf.h"
#include "xfs_dinode.h"
#include "xfs_inode.h"
#include "xfs_bmap.h"
#include "xfs_btree.h"
#include "xfs_ialloc.h"
#include "xfs_rtalloc.h"
#include "xfs_error.h"
#include "xfs_itable.h"
#include "xfs_rw.h"
#include "xfs_attr.h"
#include "xfs_inode_item.h"
#include "xfs_buf_item.h"
#include "xfs_utils.h"
#include "xfs_iomap.h"
#include "xfs_vnodeops.h"

#include <linux/capability.h>
#include <linux/writeback.h>

#if defined(XFS_RW_TRACE)
void
xfs_rw_enter_trace(
	int			tag,
	xfs_inode_t		*ip,
	void			*data,
	size_t			segs,
	loff_t			offset,
	int			ioflags)
{
	if (ip->i_rwtrace == NULL)
		return;
	ktrace_enter(ip->i_rwtrace,
		(void *)(unsigned long)tag,
		(void *)ip,
		(void *)((unsigned long)((ip->i_d.di_size >> 32) & 0xffffffff)),
		(void *)((unsigned long)(ip->i_d.di_size & 0xffffffff)),
		(void *)data,
		(void *)((unsigned long)segs),
		(void *)((unsigned long)((offset >> 32) & 0xffffffff)),
		(void *)((unsigned long)(offset & 0xffffffff)),
		(void *)((unsigned long)ioflags),
		(void *)((unsigned long)((ip->i_new_size >> 32) & 0xffffffff)),
		(void *)((unsigned long)(ip->i_new_size & 0xffffffff)),
		(void *)((unsigned long)current_pid()),
		(void *)NULL,
		(void *)NULL,
		(void *)NULL,
		(void *)NULL);
}

void
xfs_inval_cached_trace(
	xfs_inode_t	*ip,
	xfs_off_t	offset,
	xfs_off_t	len,
	xfs_off_t	first,
	xfs_off_t	last)
{

	if (ip->i_rwtrace == NULL)
		return;
	ktrace_enter(ip->i_rwtrace,
		(void *)(__psint_t)XFS_INVAL_CACHED,
		(void *)ip,
		(void *)((unsigned long)((offset >> 32) & 0xffffffff)),
		(void *)((unsigned long)(offset & 0xffffffff)),
		(void *)((unsigned long)((len >> 32) & 0xffffffff)),
		(void *)((unsigned long)(len & 0xffffffff)),
		(void *)((unsigned long)((first >> 32) & 0xffffffff)),
		(void *)((unsigned long)(first & 0xffffffff)),
		(void *)((unsigned long)((last >> 32) & 0xffffffff)),
		(void *)((unsigned long)(last & 0xffffffff)),
		(void *)((unsigned long)current_pid()),
		(void *)NULL,
		(void *)NULL,
		(void *)NULL,
		(void *)NULL,
		(void *)NULL);
}
#endif

STATIC int
xfs_iozero(
	struct xfs_inode	*ip,	 
	loff_t			pos,	 
	size_t			count)	 
{
	struct page		*page;
	struct address_space	*mapping;
	int			status;

	mapping = VFS_I(ip)->i_mapping;
	do {
		unsigned offset, bytes;
		void *fsdata;

		offset = (pos & (PAGE_CACHE_SIZE -1));  
		bytes = PAGE_CACHE_SIZE - offset;
		if (bytes > count)
			bytes = count;

		status = pagecache_write_begin(NULL, mapping, pos, bytes,
					AOP_FLAG_UNINTERRUPTIBLE,
					&page, &fsdata);
		if (status)
			break;

		zero_user(page, offset, bytes);

		status = pagecache_write_end(NULL, mapping, pos, bytes, bytes,
					page, fsdata);
		WARN_ON(status <= 0);  
		pos += bytes;
		count -= bytes;
		status = 0;
	} while (count);

	return (-status);
}

ssize_t			 
xfs_read(
	xfs_inode_t		*ip,
	struct kiocb		*iocb,
	const struct iovec	*iovp,
	unsigned int		segs,
	loff_t			*offset,
	int			ioflags)
{
	struct file		*file = iocb->ki_filp;
	struct inode		*inode = file->f_mapping->host;
	xfs_mount_t		*mp = ip->i_mount;
	size_t			size = 0;
	ssize_t			ret = 0;
	xfs_fsize_t		n;
	unsigned long		seg;

	XFS_STATS_INC(xs_read_calls);

	for (seg = 0; seg < segs; seg++) {
		const struct iovec *iv = &iovp[seg];

		size += iv->iov_len;
		if (unlikely((ssize_t)(size|iv->iov_len) < 0))
			return XFS_ERROR(-EINVAL);
	}
	 
	if (unlikely(ioflags & IO_ISDIRECT)) {
		xfs_buftarg_t	*target =
			XFS_IS_REALTIME_INODE(ip) ?
				mp->m_rtdev_targp : mp->m_ddev_targp;
		if ((*offset & target->bt_smask) ||
		    (size & target->bt_smask)) {
			if (*offset == ip->i_size) {
				return (0);
			}
			return -XFS_ERROR(EINVAL);
		}
	}

	n = XFS_MAXIOFFSET(mp) - *offset;
	if ((n <= 0) || (size == 0))
		return 0;

	if (n < size)
		size = n;

	if (XFS_FORCED_SHUTDOWN(mp))
		return -EIO;

	if (unlikely(ioflags & IO_ISDIRECT))
		mutex_lock(&inode->i_mutex);
	xfs_ilock(ip, XFS_IOLOCK_SHARED);

	if (DM_EVENT_ENABLED(ip, DM_EVENT_READ) && !(ioflags & IO_INVIS)) {
		int dmflags = FILP_DELAY_FLAG(file) | DM_SEM_FLAG_RD(ioflags);
		int iolock = XFS_IOLOCK_SHARED;

		ret = -XFS_SEND_DATA(mp, DM_EVENT_READ, ip, *offset, size,
					dmflags, &iolock);
		if (ret) {
			xfs_iunlock(ip, XFS_IOLOCK_SHARED);
			if (unlikely(ioflags & IO_ISDIRECT))
				mutex_unlock(&inode->i_mutex);
			return ret;
		}
	}

	if (unlikely(ioflags & IO_ISDIRECT)) {
		if (inode->i_mapping->nrpages)
			ret = -xfs_flushinval_pages(ip, (*offset & PAGE_CACHE_MASK),
						    -1, FI_REMAPF_LOCKED);
		mutex_unlock(&inode->i_mutex);
		if (ret) {
			xfs_iunlock(ip, XFS_IOLOCK_SHARED);
			return ret;
		}
	}

	xfs_rw_enter_trace(XFS_READ_ENTER, ip,
				(void *)iovp, segs, *offset, ioflags);

	iocb->ki_pos = *offset;
	ret = generic_file_aio_read(iocb, iovp, segs, *offset);
	if (ret == -EIOCBQUEUED && !(ioflags & IO_ISAIO))
		ret = wait_on_sync_kiocb(iocb);
	if (ret > 0)
		XFS_STATS_ADD(xs_read_bytes, ret);

	xfs_iunlock(ip, XFS_IOLOCK_SHARED);
	return ret;
}

#ifdef CONFIG_SYNO_PLX_PORTING
ssize_t
xfs_sendfile(
	xfs_inode_t		*ip,
	struct file		*filp,
	loff_t			*ppos,
	int				 ioflags,
	size_t			 count,
	read_actor_t	 actor,
	void			*target,
	int              incoherent)
{
	xfs_mount_t	*mp = ip->i_mount;
	ssize_t		 ret;

	XFS_STATS_INC(xs_read_calls);
	if (XFS_FORCED_SHUTDOWN(ip->i_mount))
		return -EIO;

	xfs_ilock(ip, XFS_IOLOCK_SHARED);

	if (DM_EVENT_ENABLED(ip, DM_EVENT_READ) && (!(ioflags & IO_INVIS))) {
		int iolock = XFS_IOLOCK_SHARED;
		int error;

		error = XFS_SEND_DATA(mp, DM_EVENT_READ, ip, *ppos, count,
				      FILP_DELAY_FLAG(filp), &iolock);
		if (error) {
			xfs_iunlock(ip, XFS_IOLOCK_SHARED);
			return -error;
		}
	}
	xfs_rw_enter_trace(XFS_SENDFILE_ENTER, ip,
		target, count, *offset, ioflags);
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	if(incoherent) {
		xfs_iunlock(ip, XFS_IOLOCK_SHARED);
		ret = generic_file_incoherent_sendfile(filp, ppos, count, actor, target);
		xfs_ilock(ip, XFS_IOLOCK_SHARED);
	} else {
		ret = generic_file_sendfile(filp, ppos, count, actor, target);
	}
#else  
		ret = generic_file_sendfile(filp, ppos, count, actor, target);
#endif  

	if (ret > 0)
		XFS_STATS_ADD(xs_read_bytes, ret);

	xfs_iunlock(ip, XFS_IOLOCK_SHARED);
	return ret;
}
#endif

ssize_t
xfs_splice_read(
	xfs_inode_t		*ip,
	struct file		*infilp,
	loff_t			*ppos,
	struct pipe_inode_info	*pipe,
	size_t			count,
	int			flags,
	int			ioflags)
{
	xfs_mount_t		*mp = ip->i_mount;
	ssize_t			ret;

	XFS_STATS_INC(xs_read_calls);
	if (XFS_FORCED_SHUTDOWN(ip->i_mount))
		return -EIO;

	xfs_ilock(ip, XFS_IOLOCK_SHARED);

	if (DM_EVENT_ENABLED(ip, DM_EVENT_READ) && !(ioflags & IO_INVIS)) {
		int iolock = XFS_IOLOCK_SHARED;
		int error;

		error = XFS_SEND_DATA(mp, DM_EVENT_READ, ip, *ppos, count,
					FILP_DELAY_FLAG(infilp), &iolock);
		if (error) {
			xfs_iunlock(ip, XFS_IOLOCK_SHARED);
			return -error;
		}
	}
	xfs_rw_enter_trace(XFS_SPLICE_READ_ENTER, ip,
			   pipe, count, *ppos, ioflags);
	ret = generic_file_splice_read(infilp, ppos, pipe, count, flags);
	if (ret > 0)
		XFS_STATS_ADD(xs_read_bytes, ret);

	xfs_iunlock(ip, XFS_IOLOCK_SHARED);
	return ret;
}

ssize_t
xfs_splice_write(
	xfs_inode_t		*ip,
	struct pipe_inode_info	*pipe,
	struct file		*outfilp,
	loff_t			*ppos,
	size_t			count,
	int			flags,
	int			ioflags)
{
	xfs_mount_t		*mp = ip->i_mount;
	ssize_t			ret;
	struct inode		*inode = outfilp->f_mapping->host;
	xfs_fsize_t		isize, new_size;

	XFS_STATS_INC(xs_write_calls);
	if (XFS_FORCED_SHUTDOWN(ip->i_mount))
		return -EIO;

	xfs_ilock(ip, XFS_IOLOCK_EXCL);

	if (DM_EVENT_ENABLED(ip, DM_EVENT_WRITE) && !(ioflags & IO_INVIS)) {
		int iolock = XFS_IOLOCK_EXCL;
		int error;

		error = XFS_SEND_DATA(mp, DM_EVENT_WRITE, ip, *ppos, count,
					FILP_DELAY_FLAG(outfilp), &iolock);
		if (error) {
			xfs_iunlock(ip, XFS_IOLOCK_EXCL);
			return -error;
		}
	}

	new_size = *ppos + count;

	xfs_ilock(ip, XFS_ILOCK_EXCL);
	if (new_size > ip->i_size)
		ip->i_new_size = new_size;
	xfs_iunlock(ip, XFS_ILOCK_EXCL);

	xfs_rw_enter_trace(XFS_SPLICE_WRITE_ENTER, ip,
			   pipe, count, *ppos, ioflags);
	ret = generic_file_splice_write(pipe, outfilp, ppos, count, flags);
	if (ret > 0)
		XFS_STATS_ADD(xs_write_bytes, ret);

	isize = i_size_read(inode);
	if (unlikely(ret < 0 && ret != -EFAULT && *ppos > isize))
		*ppos = isize;

	if (*ppos > ip->i_size) {
		xfs_ilock(ip, XFS_ILOCK_EXCL);
		if (*ppos > ip->i_size)
			ip->i_size = *ppos;
		xfs_iunlock(ip, XFS_ILOCK_EXCL);
	}

	if (ip->i_new_size) {
		xfs_ilock(ip, XFS_ILOCK_EXCL);
		ip->i_new_size = 0;
		if (ip->i_d.di_size > ip->i_size)
			ip->i_d.di_size = ip->i_size;
		xfs_iunlock(ip, XFS_ILOCK_EXCL);
	}
	xfs_iunlock(ip, XFS_IOLOCK_EXCL);
	return ret;
}

STATIC int				 
xfs_zero_last_block(
	xfs_inode_t	*ip,
	xfs_fsize_t	offset,
	xfs_fsize_t	isize)
{
	xfs_fileoff_t	last_fsb;
	xfs_mount_t	*mp = ip->i_mount;
	int		nimaps;
	int		zero_offset;
	int		zero_len;
	int		error = 0;
	xfs_bmbt_irec_t	imap;

	ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL));

	zero_offset = XFS_B_FSB_OFFSET(mp, isize);
	if (zero_offset == 0) {
		 
		return 0;
	}

	last_fsb = XFS_B_TO_FSBT(mp, isize);
	nimaps = 1;
	error = xfs_bmapi(NULL, ip, last_fsb, 1, 0, NULL, 0, &imap,
			  &nimaps, NULL, NULL);
	if (error) {
		return error;
	}
	ASSERT(nimaps > 0);
	 
	if (imap.br_startblock == HOLESTARTBLOCK) {
		return 0;
	}
	 
	xfs_iunlock(ip, XFS_ILOCK_EXCL);

	zero_len = mp->m_sb.sb_blocksize - zero_offset;
	if (isize + zero_len > offset)
		zero_len = offset - isize;
	error = xfs_iozero(ip, isize, zero_len);

	xfs_ilock(ip, XFS_ILOCK_EXCL);
	ASSERT(error >= 0);
	return error;
}

int					 
xfs_zero_eof(
	xfs_inode_t	*ip,
	xfs_off_t	offset,		 
	xfs_fsize_t	isize)		 
{
	xfs_mount_t	*mp = ip->i_mount;
	xfs_fileoff_t	start_zero_fsb;
	xfs_fileoff_t	end_zero_fsb;
	xfs_fileoff_t	zero_count_fsb;
	xfs_fileoff_t	last_fsb;
	xfs_fileoff_t	zero_off;
	xfs_fsize_t	zero_len;
	int		nimaps;
	int		error = 0;
	xfs_bmbt_irec_t	imap;

	ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL|XFS_IOLOCK_EXCL));
	ASSERT(offset > isize);

	error = xfs_zero_last_block(ip, offset, isize);
	if (error) {
		ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL|XFS_IOLOCK_EXCL));
		return error;
	}

	last_fsb = isize ? XFS_B_TO_FSBT(mp, isize - 1) : (xfs_fileoff_t)-1;
	start_zero_fsb = XFS_B_TO_FSB(mp, (xfs_ufsize_t)isize);
	end_zero_fsb = XFS_B_TO_FSBT(mp, offset - 1);
	ASSERT((xfs_sfiloff_t)last_fsb < (xfs_sfiloff_t)start_zero_fsb);
	if (last_fsb == end_zero_fsb) {
		 
		return 0;
	}

	ASSERT(start_zero_fsb <= end_zero_fsb);
	while (start_zero_fsb <= end_zero_fsb) {
		nimaps = 1;
		zero_count_fsb = end_zero_fsb - start_zero_fsb + 1;
		error = xfs_bmapi(NULL, ip, start_zero_fsb, zero_count_fsb,
				  0, NULL, 0, &imap, &nimaps, NULL, NULL);
		if (error) {
			ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL|XFS_IOLOCK_EXCL));
			return error;
		}
		ASSERT(nimaps > 0);

		if (imap.br_state == XFS_EXT_UNWRITTEN ||
		    imap.br_startblock == HOLESTARTBLOCK) {
			 
			start_zero_fsb = imap.br_startoff + imap.br_blockcount;
			ASSERT(start_zero_fsb <= (end_zero_fsb + 1));
			continue;
		}

		xfs_iunlock(ip, XFS_ILOCK_EXCL);

		zero_off = XFS_FSB_TO_B(mp, start_zero_fsb);
		zero_len = XFS_FSB_TO_B(mp, imap.br_blockcount);

		if ((zero_off + zero_len) > offset)
			zero_len = offset - zero_off;

		error = xfs_iozero(ip, zero_off, zero_len);
		if (error) {
			goto out_lock;
		}

		start_zero_fsb = imap.br_startoff + imap.br_blockcount;
		ASSERT(start_zero_fsb <= (end_zero_fsb + 1));

		xfs_ilock(ip, XFS_ILOCK_EXCL);
	}

	return 0;

out_lock:
	xfs_ilock(ip, XFS_ILOCK_EXCL);
	ASSERT(error >= 0);
	return error;
}

ssize_t				 
xfs_write(
	struct xfs_inode	*xip,
	struct kiocb		*iocb,
	const struct iovec	*iovp,
	unsigned int		nsegs,
	loff_t			*offset,
	int			ioflags)
{
	struct file		*file = iocb->ki_filp;
	struct address_space	*mapping = file->f_mapping;
	struct inode		*inode = mapping->host;
	unsigned long		segs = nsegs;
	xfs_mount_t		*mp;
	ssize_t			ret = 0, error = 0;
	xfs_fsize_t		isize, new_size;
	int			iolock;
	int			eventsent = 0;
	size_t			ocount = 0, count;
	loff_t			pos;
	int			need_i_mutex;

	XFS_STATS_INC(xs_write_calls);

	error = generic_segment_checks(iovp, &segs, &ocount, VERIFY_READ);
	if (error)
		return error;

	count = ocount;
	pos = *offset;

	if (count == 0)
		return 0;

	mp = xip->i_mount;

	xfs_wait_for_freeze(mp, SB_FREEZE_WRITE);

	if (XFS_FORCED_SHUTDOWN(mp))
		return -EIO;

relock:
	if (ioflags & IO_ISDIRECT) {
		iolock = XFS_IOLOCK_SHARED;
		need_i_mutex = 0;
	} else {
		iolock = XFS_IOLOCK_EXCL;
		need_i_mutex = 1;
		mutex_lock(&inode->i_mutex);
	}

	xfs_ilock(xip, XFS_ILOCK_EXCL|iolock);

start:
	error = -generic_write_checks(file, &pos, &count,
					S_ISBLK(inode->i_mode));
	if (error) {
		xfs_iunlock(xip, XFS_ILOCK_EXCL|iolock);
		goto out_unlock_mutex;
	}

	if ((DM_EVENT_ENABLED(xip, DM_EVENT_WRITE) &&
	    !(ioflags & IO_INVIS) && !eventsent)) {
		int		dmflags = FILP_DELAY_FLAG(file);

		if (need_i_mutex)
			dmflags |= DM_FLAGS_IMUX;

		xfs_iunlock(xip, XFS_ILOCK_EXCL);
		error = XFS_SEND_DATA(xip->i_mount, DM_EVENT_WRITE, xip,
				      pos, count, dmflags, &iolock);
		if (error) {
			goto out_unlock_internal;
		}
		xfs_ilock(xip, XFS_ILOCK_EXCL);
		eventsent = 1;

		if ((file->f_flags & O_APPEND) && pos != xip->i_size)
			goto start;
	}

	if (ioflags & IO_ISDIRECT) {
		xfs_buftarg_t	*target =
			XFS_IS_REALTIME_INODE(xip) ?
				mp->m_rtdev_targp : mp->m_ddev_targp;

		if ((pos & target->bt_smask) || (count & target->bt_smask)) {
			xfs_iunlock(xip, XFS_ILOCK_EXCL|iolock);
			return XFS_ERROR(-EINVAL);
		}

		if (!need_i_mutex && (mapping->nrpages || pos > xip->i_size)) {
			xfs_iunlock(xip, XFS_ILOCK_EXCL|iolock);
			iolock = XFS_IOLOCK_EXCL;
			need_i_mutex = 1;
			mutex_lock(&inode->i_mutex);
			xfs_ilock(xip, XFS_ILOCK_EXCL|iolock);
			goto start;
		}
	}

	new_size = pos + count;
	if (new_size > xip->i_size)
		xip->i_new_size = new_size;

	if (likely(!(ioflags & IO_INVIS)))
		file_update_time(file);

	if (pos > xip->i_size) {
		error = xfs_zero_eof(xip, pos, xip->i_size);
		if (error) {
			xfs_iunlock(xip, XFS_ILOCK_EXCL);
			goto out_unlock_internal;
		}
	}
	xfs_iunlock(xip, XFS_ILOCK_EXCL);

	if (((xip->i_d.di_mode & S_ISUID) ||
	    ((xip->i_d.di_mode & (S_ISGID | S_IXGRP)) ==
		(S_ISGID | S_IXGRP))) &&
	     !capable(CAP_FSETID)) {
		error = xfs_write_clear_setuid(xip);
		if (likely(!error))
			error = -file_remove_suid(file);
		if (unlikely(error)) {
			goto out_unlock_internal;
		}
	}

	current->backing_dev_info = mapping->backing_dev_info;

	if ((ioflags & IO_ISDIRECT)) {
		if (mapping->nrpages) {
			WARN_ON(need_i_mutex == 0);
			xfs_inval_cached_trace(xip, pos, -1,
					(pos & PAGE_CACHE_MASK), -1);
			error = xfs_flushinval_pages(xip,
					(pos & PAGE_CACHE_MASK),
					-1, FI_REMAPF_LOCKED);
			if (error)
				goto out_unlock_internal;
		}

		if (need_i_mutex) {
			 
			xfs_ilock_demote(xip, XFS_IOLOCK_EXCL);
			mutex_unlock(&inode->i_mutex);

			iolock = XFS_IOLOCK_SHARED;
			need_i_mutex = 0;
		}

 		xfs_rw_enter_trace(XFS_DIOWR_ENTER, xip, (void *)iovp, segs,
				*offset, ioflags);
		ret = generic_file_direct_write(iocb, iovp,
				&segs, pos, offset, count, ocount);

		if (ret >= 0 && ret != count) {
			XFS_STATS_ADD(xs_write_bytes, ret);

			pos += ret;
			count -= ret;

			ioflags &= ~IO_ISDIRECT;
			xfs_iunlock(xip, iolock);
			goto relock;
		}
	} else {
		int enospc = 0;
		ssize_t ret2 = 0;

write_retry:
		xfs_rw_enter_trace(XFS_WRITE_ENTER, xip, (void *)iovp, segs,
				*offset, ioflags);
		ret2 = generic_file_buffered_write(iocb, iovp, segs,
				pos, offset, count, ret);
		 
		if (ret2 == -ENOSPC && !enospc) {
			error = xfs_flush_pages(xip, 0, -1, 0, FI_NONE);
			if (error)
				goto out_unlock_internal;
			enospc = 1;
			goto write_retry;
		}
		ret = ret2;
	}

	current->backing_dev_info = NULL;

	if (ret == -EIOCBQUEUED && !(ioflags & IO_ISAIO))
		ret = wait_on_sync_kiocb(iocb);

	isize = i_size_read(inode);
	if (unlikely(ret < 0 && ret != -EFAULT && *offset > isize))
		*offset = isize;

	if (*offset > xip->i_size) {
		xfs_ilock(xip, XFS_ILOCK_EXCL);
		if (*offset > xip->i_size)
			xip->i_size = *offset;
		xfs_iunlock(xip, XFS_ILOCK_EXCL);
	}

	if (ret == -ENOSPC &&
	    DM_EVENT_ENABLED(xip, DM_EVENT_NOSPACE) && !(ioflags & IO_INVIS)) {
		xfs_iunlock(xip, iolock);
		if (need_i_mutex)
			mutex_unlock(&inode->i_mutex);
		error = XFS_SEND_NAMESP(xip->i_mount, DM_EVENT_NOSPACE, xip,
				DM_RIGHT_NULL, xip, DM_RIGHT_NULL, NULL, NULL,
				0, 0, 0);  
		if (need_i_mutex)
			mutex_lock(&inode->i_mutex);
		xfs_ilock(xip, iolock);
		if (error)
			goto out_unlock_internal;
		goto start;
	}

	error = -ret;
	if (ret <= 0)
		goto out_unlock_internal;

	XFS_STATS_ADD(xs_write_bytes, ret);

	if ((file->f_flags & O_SYNC) || IS_SYNC(inode)) {
		loff_t end = pos + ret - 1;
		int error2;

		xfs_iunlock(xip, iolock);
		if (need_i_mutex)
			mutex_unlock(&inode->i_mutex);

		error2 = filemap_write_and_wait_range(mapping, pos, end);
		if (!error)
			error = error2;
		if (need_i_mutex)
			mutex_lock(&inode->i_mutex);
		xfs_ilock(xip, iolock);

		error2 = xfs_fsync(xip);
		if (!error)
			error = error2;
	}

 out_unlock_internal:
	if (xip->i_new_size) {
		xfs_ilock(xip, XFS_ILOCK_EXCL);
		xip->i_new_size = 0;
		 
		if (xip->i_d.di_size > xip->i_size)
			xip->i_d.di_size = xip->i_size;
		xfs_iunlock(xip, XFS_ILOCK_EXCL);
	}
	xfs_iunlock(xip, iolock);
 out_unlock_mutex:
	if (need_i_mutex)
		mutex_unlock(&inode->i_mutex);
	return -error;
}

#ifdef CONFIG_SYNO_PLX_PORTING
extern ssize_t generic_file_direct_netrx_write(
	struct kiocb *iocb,
	void         *callback,
	void         *sock,
	loff_t        pos,
	loff_t       *ppos,
	u32           count,
	ssize_t       written);

ssize_t xfs_direct_netrx_write(
	struct kiocb *iocb,
	void         *callback,
	void         *sock)
{
	struct xfs_inode     *xip = XFS_I(iocb->ki_filp->f_mapping->host);
	struct file          *file = iocb->ki_filp;
	loff_t               *offset = &iocb->ki_pos;
	size_t                count = iocb->ki_left;
	loff_t                pos = *offset;
	struct address_space *mapping = file->f_mapping;
	struct inode         *inode = mapping->host;
	xfs_mount_t          *mp = xip->i_mount;
	int                   iolock = XFS_IOLOCK_EXCL;
	int                   eventsent = 0;
	int                   need_i_mutex = 1;
	ssize_t               ret = 0, error = 0;
	xfs_fsize_t           isize, new_size;

	XFS_STATS_INC(xs_write_calls);

	xfs_wait_for_freeze(mp, SB_FREEZE_WRITE);

	if (XFS_FORCED_SHUTDOWN(mp))
		return -EIO;

	mutex_lock(&inode->i_mutex);

	xfs_ilock(xip, XFS_ILOCK_EXCL|iolock);

start:
	error = -generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (error) {
		xfs_iunlock(xip, XFS_ILOCK_EXCL|iolock);
		goto out_unlock_mutex;
	}

	if ((DM_EVENT_ENABLED(xip, DM_EVENT_WRITE) && !eventsent)) {
		int dmflags = FILP_DELAY_FLAG(file);

		if (need_i_mutex)
			dmflags |= DM_FLAGS_IMUX;

		xfs_iunlock(xip, XFS_ILOCK_EXCL);
		error = XFS_SEND_DATA(xip->i_mount, DM_EVENT_WRITE, xip,
				      pos, count, dmflags, &iolock);
		if (error) {
			goto out_unlock_internal;
		}
		xfs_ilock(xip, XFS_ILOCK_EXCL);
		eventsent = 1;

		if ((file->f_flags & O_APPEND) && pos != xip->i_size)
			goto start;
	}

	new_size = pos + count;
	if (new_size > xip->i_size)
		xip->i_new_size = new_size;

	xfs_ichgtime(xip, XFS_ICHGTIME_MOD | XFS_ICHGTIME_CHG);

	if (pos > xip->i_size) {
		error = xfs_zero_eof(xip, pos, xip->i_size);
		if (error) {
			xfs_iunlock(xip, XFS_ILOCK_EXCL);
			goto out_unlock_internal;
		}
	}
	xfs_iunlock(xip, XFS_ILOCK_EXCL);

	if (((xip->i_d.di_mode & S_ISUID) ||
	    ((xip->i_d.di_mode & (S_ISGID | S_IXGRP)) ==
		(S_ISGID | S_IXGRP))) &&
	     !capable(CAP_FSETID)) {
		error = xfs_write_clear_setuid(xip);
		if (likely(!error))
			error = -file_remove_suid(file);
		if (unlikely(error)) {
			goto out_unlock_internal;
		}
	}

	current->backing_dev_info = mapping->backing_dev_info;

	{
		int enospc = 0;
		ssize_t ret2 = 0;

write_retry:
		xfs_rw_enter_trace(XFS_WRITE_ENTER, xip, (void *)iovp, segs, *offset,
			ioflags);

		ret2 = generic_file_direct_netrx_write(iocb, callback, sock, pos,
			offset, count, ret);

		if (ret2 == -ENOSPC && !enospc) {
			error = xfs_flush_pages(xip, 0, -1, 0, FI_NONE);
			if (error)
				goto out_unlock_internal;
			enospc = 1;
			goto write_retry;
		}
		ret = ret2;
	}

	current->backing_dev_info = NULL;

	isize = i_size_read(inode);
	if (unlikely(ret < 0 && ret != -EFAULT && *offset > isize))
		*offset = isize;

	if (*offset > xip->i_size) {
		xfs_ilock(xip, XFS_ILOCK_EXCL);
		if (*offset > xip->i_size)
			xip->i_size = *offset;
		xfs_iunlock(xip, XFS_ILOCK_EXCL);
	}

	if (ret == -ENOSPC && DM_EVENT_ENABLED(xip, DM_EVENT_NOSPACE)) {
		xfs_iunlock(xip, iolock);
		if (need_i_mutex)
			mutex_unlock(&inode->i_mutex);
		error = XFS_SEND_NAMESP(xip->i_mount, DM_EVENT_NOSPACE, xip,
				DM_RIGHT_NULL, xip, DM_RIGHT_NULL, NULL, NULL,
				0, 0, 0);  
		if (need_i_mutex)
			mutex_lock(&inode->i_mutex);
		xfs_ilock(xip, iolock);
		if (error)
			goto out_unlock_internal;
		goto start;
	}

	error = -ret;
	if (ret <= 0)
		goto out_unlock_internal;

	XFS_STATS_ADD(xs_write_bytes, ret);

	if ((file->f_flags & O_SYNC) || IS_SYNC(inode)) {
		int error2;

		xfs_iunlock(xip, iolock);
		if (need_i_mutex)
			mutex_unlock(&inode->i_mutex);
#ifdef CONFIG_SYNO_PLX_PORTING
		error2 = filemap_write_and_wait_range(mapping, pos,
												pos + ret - 1);
#else
		error2 = sync_page_range(inode, mapping, pos, ret);
#endif

		if (!error)
			error = error2;
		if (need_i_mutex)
			mutex_lock(&inode->i_mutex);
		xfs_ilock(xip, iolock);
#ifdef CONFIG_SYNO_PLX_PORTING
		error2 = xfs_fsync(xip);
#else
		error2 = xfs_write_sync_logforce(mp, xip);
#endif
		if (!error)
			error = error2;
	}

 out_unlock_internal:
	if (xip->i_new_size) {
		xfs_ilock(xip, XFS_ILOCK_EXCL);
		xip->i_new_size = 0;
		 
		if (xip->i_d.di_size > xip->i_size)
			xip->i_d.di_size = xip->i_size;
		xfs_iunlock(xip, XFS_ILOCK_EXCL);
	}
	xfs_iunlock(xip, iolock);
 out_unlock_mutex:
	if (need_i_mutex)
		mutex_unlock(&inode->i_mutex);

	return -error;
}
#endif

int
xfs_bdstrat_cb(struct xfs_buf *bp)
{
	if (XFS_FORCED_SHUTDOWN(bp->b_mount)) {
		xfs_buftrace("XFS__BDSTRAT IOERROR", bp);
		 
		if (XFS_BUF_IODONE_FUNC(bp) == NULL &&
		    (XFS_BUF_ISREAD(bp)) == 0)
			return (xfs_bioerror_relse(bp));
		else
			return (xfs_bioerror(bp));
	}

	xfs_buf_iorequest(bp);
	return 0;
}

void
xfsbdstrat(
	struct xfs_mount	*mp,
	struct xfs_buf		*bp)
{
	ASSERT(mp);
	if (!XFS_FORCED_SHUTDOWN(mp)) {
		xfs_buf_iorequest(bp);
		return;
	}

	xfs_buftrace("XFSBDSTRAT IOERROR", bp);
	xfs_bioerror_relse(bp);
}

int
xfs_dev_is_read_only(
	xfs_mount_t		*mp,
	char			*message)
{
	if (xfs_readonly_buftarg(mp->m_ddev_targp) ||
	    xfs_readonly_buftarg(mp->m_logdev_targp) ||
	    (mp->m_rtdev_targp && xfs_readonly_buftarg(mp->m_rtdev_targp))) {
		cmn_err(CE_NOTE,
			"XFS: %s required on read-only device.", message);
		cmn_err(CE_NOTE,
			"XFS: write access unavailable, cannot proceed.");
		return EROFS;
	}
	return 0;
}
