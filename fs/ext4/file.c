#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/jbd2.h>
#include <linux/mount.h>
#include <linux/path.h>
#include "ext4.h"
#include "ext4_jbd2.h"
#include "xattr.h"
#include "acl.h"

static int ext4_release_file(struct inode *inode, struct file *filp)
{
	if (ext4_test_inode_state(inode, EXT4_STATE_DA_ALLOC_CLOSE)) {
		ext4_alloc_da_blocks(inode);
		ext4_clear_inode_state(inode, EXT4_STATE_DA_ALLOC_CLOSE);
	}
	 
	if ((filp->f_mode & FMODE_WRITE) &&
			(atomic_read(&inode->i_writecount) == 1) &&
		        !EXT4_I(inode)->i_reserved_data_blocks)
	{
		down_write(&EXT4_I(inode)->i_data_sem);
		ext4_discard_preallocations(inode);
		up_write(&EXT4_I(inode)->i_data_sem);
	}
	if (is_dx(inode) && filp->private_data)
		ext4_htree_free_dir_info(filp->private_data);

	return 0;
}

static ssize_t
ext4_file_write(struct kiocb *iocb, const struct iovec *iov,
		unsigned long nr_segs, loff_t pos)
{
	struct inode *inode = iocb->ki_filp->f_path.dentry->d_inode;

	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))) {
		struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
		size_t length = iov_length(iov, nr_segs);

		if (pos > sbi->s_bitmap_maxbytes)
			return -EFBIG;

		if (pos + length > sbi->s_bitmap_maxbytes) {
			nr_segs = iov_shorten((struct iovec *)iov, nr_segs,
					      sbi->s_bitmap_maxbytes - pos);
		}
	}

	return generic_file_aio_write(iocb, iov, nr_segs, pos);
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
 
 static ssize_t ext4_direct_netrx_write(
 	struct kiocb *iocb,
 	void         *callback,
 	void         *sock)
 {
 	struct file  *file = iocb->ki_filp;
 	struct inode *inode = file->f_path.dentry->d_inode;
 	loff_t       *offset = &iocb->ki_pos;
 	size_t        length = iocb->ki_left;
 	loff_t        pos = *offset;
 	ssize_t       ret;
 	int           err;
 
 	if (!(EXT4_I(inode)->i_flags & EXT4_EXTENTS_FL)) {
 		struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
 
 		if (pos > sbi->s_bitmap_maxbytes)
 			return -EFBIG;
 
 		if (pos + length > sbi->s_bitmap_maxbytes) {
 			length = sbi->s_bitmap_maxbytes - pos;
 		}
 	}
 
 	ret = generic_file_direct_netrx_write(iocb, callback, sock, pos, offset, length, 0);
 	 
 	if (ret <= 0)
 		return ret;
 
 	if (file->f_flags & O_SYNC) {
 		 
 		if (!ext4_should_journal_data(inode))
 			return ret;
 
 		goto force_commit;
 	}
 
 	if (!IS_SYNC(inode))
 		return ret;
 
 force_commit:
 	err = ext4_force_commit(inode->i_sb);
 	if (err)
 		return err;
 	return ret;
 }
 
#endif

static const struct vm_operations_struct ext4_file_vm_ops = {
	.fault		= filemap_fault,
	.page_mkwrite   = ext4_page_mkwrite,
};

static int ext4_file_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct address_space *mapping = file->f_mapping;

	if (!mapping->a_ops->readpage)
		return -ENOEXEC;
	file_accessed(file);
	vma->vm_ops = &ext4_file_vm_ops;
	vma->vm_flags |= VM_CAN_NONLINEAR;
	return 0;
}

static int ext4_file_open(struct inode * inode, struct file * filp)
{
	struct super_block *sb = inode->i_sb;
	struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
	struct vfsmount *mnt = filp->f_path.mnt;
	struct path path;
	char buf[64], *cp;

	if (unlikely(!(sbi->s_mount_flags & EXT4_MF_MNTDIR_SAMPLED) &&
		     !(sb->s_flags & MS_RDONLY))) {
		sbi->s_mount_flags |= EXT4_MF_MNTDIR_SAMPLED;
		 
		memset(buf, 0, sizeof(buf));
		path.mnt = mnt->mnt_parent;
		path.dentry = mnt->mnt_mountpoint;
		path_get(&path);
		cp = d_path(&path, buf, sizeof(buf));
		path_put(&path);
		if (!IS_ERR(cp)) {
			memcpy(sbi->s_es->s_last_mounted, cp,
			       sizeof(sbi->s_es->s_last_mounted));
			sb->s_dirt = 1;
		}
	}
	return generic_file_open(inode, filp);
}

const struct file_operations ext4_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= ext4_file_write,
#ifdef CONFIG_SYNO_PLX_PORTING
	.aio_direct_netrx_write = ext4_direct_netrx_write,
#endif
	.unlocked_ioctl = ext4_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ext4_compat_ioctl,
#endif
	.mmap		= ext4_file_mmap,
	.open		= ext4_file_open,
	.release	= ext4_release_file,
	.fsync		= ext4_sync_file,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
#ifdef CONFIG_SYNO_PLX_PORTING
	.sendfile	   = generic_file_sendfile,
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	.incoherent_sendfile = generic_file_incoherent_sendfile,
#else
	.incoherent_sendfile = generic_file_sendfile,
#endif
	.preallocate   = ext4_preallocate,
	.unpreallocate = ext4_unpreallocate,
	.resetpreallocate = ext4_resetpreallocate,
#endif
};

const struct inode_operations ext4_file_inode_operations = {
#ifdef MY_ABC_HERE
	.syno_get_archive_ver = syno_ext4_get_archive_ver,
	.syno_set_archive_ver = syno_ext4_set_archive_ver,
#endif
	.truncate	= ext4_truncate,
	.setattr	= ext4_setattr,
	.getattr	= ext4_getattr,
#ifdef CONFIG_EXT4_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= ext4_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.check_acl	= ext4_check_acl,
	.fallocate	= ext4_fallocate,
	.fiemap		= ext4_fiemap,
#ifdef CONFIG_SYNO_PLX_PORTING
	.get_extents = ext4_get_extents,
	.getbmapx 	= ext4_getbmapx,
#endif
};
