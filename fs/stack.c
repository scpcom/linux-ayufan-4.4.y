#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/fs_stack.h>

void fsstack_copy_inode_size(struct inode *dst, const struct inode *src)
{
	i_size_write(dst, i_size_read((struct inode *)src));
	dst->i_blocks = src->i_blocks;
}
EXPORT_SYMBOL_GPL(fsstack_copy_inode_size);

void fsstack_copy_attr_all(struct inode *dest, const struct inode *src,
				int (*get_nlinks)(struct inode *))
{
	dest->i_mode = src->i_mode;
	dest->i_uid = src->i_uid;
	dest->i_gid = src->i_gid;
	dest->i_rdev = src->i_rdev;
	dest->i_atime = src->i_atime;
	dest->i_mtime = src->i_mtime;
	dest->i_ctime = src->i_ctime;
	dest->i_blkbits = src->i_blkbits;
	dest->i_flags = src->i_flags;

#ifdef MY_ABC_HERE
	 
	dest->i_archive_bit = src->i_archive_bit;
#endif
#ifdef MY_ABC_HERE
	dest->i_create_time = src->i_create_time;
#endif
	 
	if (!get_nlinks)
		dest->i_nlink = src->i_nlink;
	else
		dest->i_nlink = (*get_nlinks)(dest);
}
EXPORT_SYMBOL_GPL(fsstack_copy_attr_all);
