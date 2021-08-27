#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/jbd2.h>
#include <linux/namei.h>
#include "ext4.h"
#include "xattr.h"

static void *ext4_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	struct ext4_inode_info *ei = EXT4_I(dentry->d_inode);
	nd_set_link(nd, (char *) ei->i_data);
	return NULL;
}

const struct inode_operations ext4_symlink_inode_operations = {
#ifdef MY_ABC_HERE
	.syno_getattr	= syno_ext4_getattr,
#endif
#ifdef MY_ABC_HERE
	.syno_get_archive_ver = syno_ext4_get_archive_ver,
	.syno_set_archive_ver = syno_ext4_set_archive_ver,
#endif
	.readlink	= generic_readlink,
	.follow_link	= page_follow_link_light,
	.put_link	= page_put_link,
	.setattr	= ext4_setattr,
#ifdef CONFIG_EXT4_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= ext4_listxattr,
	.removexattr	= generic_removexattr,
#endif
};

const struct inode_operations ext4_fast_symlink_inode_operations = {
#ifdef MY_ABC_HERE
	.syno_getattr	= syno_ext4_getattr,
#endif
#ifdef MY_ABC_HERE
	.syno_get_archive_ver = syno_ext4_get_archive_ver,
	.syno_set_archive_ver = syno_ext4_set_archive_ver,
#endif
	.readlink	= generic_readlink,
	.follow_link	= ext4_follow_link,
	.setattr	= ext4_setattr,
#ifdef CONFIG_EXT4_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= ext4_listxattr,
	.removexattr	= generic_removexattr,
#endif
};
