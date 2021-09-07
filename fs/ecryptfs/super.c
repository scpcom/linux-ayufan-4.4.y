#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/key.h>
#include <linux/seq_file.h>
#include <linux/smp_lock.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include "ecryptfs_kernel.h"

struct kmem_cache *ecryptfs_inode_info_cache;

static struct inode *ecryptfs_alloc_inode(struct super_block *sb)
{
	struct ecryptfs_inode_info *inode_info;
	struct inode *inode = NULL;

	inode_info = kmem_cache_alloc(ecryptfs_inode_info_cache, GFP_KERNEL);
	if (unlikely(!inode_info))
		goto out;
	ecryptfs_init_crypt_stat(&inode_info->crypt_stat);
	mutex_init(&inode_info->lower_file_mutex);
#ifdef MY_ABC_HERE
	atomic_set(&inode_info->lower_file_count, 0);
#endif  
	inode_info->lower_file = NULL;
	inode = &inode_info->vfs_inode;
out:
	return inode;
}

static void ecryptfs_destroy_inode(struct inode *inode)
{
	struct ecryptfs_inode_info *inode_info;

	inode_info = ecryptfs_inode_to_private(inode);
#ifdef MY_ABC_HERE
	BUG_ON(inode_info->lower_file);
#else
	if (inode_info->lower_file) {
		struct dentry *lower_dentry =
			inode_info->lower_file->f_dentry;

		BUG_ON(!lower_dentry);
		if (lower_dentry->d_inode) {
			fput(inode_info->lower_file);
			inode_info->lower_file = NULL;
		}
	}
#endif  
	ecryptfs_destroy_crypt_stat(&inode_info->crypt_stat);
	kmem_cache_free(ecryptfs_inode_info_cache, inode_info);
}

void ecryptfs_init_inode(struct inode *inode, struct inode *lower_inode)
{
	ecryptfs_set_inode_lower(inode, lower_inode);
	inode->i_ino = lower_inode->i_ino;
	inode->i_version++;
	inode->i_op = &ecryptfs_main_iops;
	inode->i_fop = &ecryptfs_main_fops;
	inode->i_mapping->a_ops = &ecryptfs_aops;
}

static void ecryptfs_put_super(struct super_block *sb)
{
	struct ecryptfs_sb_info *sb_info = ecryptfs_superblock_to_private(sb);

	lock_kernel();

	ecryptfs_destroy_mount_crypt_stat(&sb_info->mount_crypt_stat);
	kmem_cache_free(ecryptfs_sb_info_cache, sb_info);
	ecryptfs_set_superblock_private(sb, NULL);

	unlock_kernel();
}

static int ecryptfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	return vfs_statfs(ecryptfs_dentry_to_lower(dentry), buf);
}

static void ecryptfs_clear_inode(struct inode *inode)
{
	iput(ecryptfs_inode_to_lower(inode));
}

static int ecryptfs_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	struct super_block *sb = mnt->mnt_sb;
	struct ecryptfs_mount_crypt_stat *mount_crypt_stat =
		&ecryptfs_superblock_to_private(sb)->mount_crypt_stat;
	struct ecryptfs_global_auth_tok *walker;

	mutex_lock(&mount_crypt_stat->global_auth_tok_list_mutex);
	list_for_each_entry(walker,
			    &mount_crypt_stat->global_auth_tok_list,
			    mount_crypt_stat_list) {
		if (walker->flags & ECRYPTFS_AUTH_TOK_FNEK)
			seq_printf(m, ",ecryptfs_fnek_sig=%s", walker->sig);
		else
			seq_printf(m, ",ecryptfs_sig=%s", walker->sig);
	}
	mutex_unlock(&mount_crypt_stat->global_auth_tok_list_mutex);

	seq_printf(m, ",ecryptfs_cipher=%s",
		mount_crypt_stat->global_default_cipher_name);

	if (mount_crypt_stat->global_default_cipher_key_size)
		seq_printf(m, ",ecryptfs_key_bytes=%zd",
			   mount_crypt_stat->global_default_cipher_key_size);
	if (mount_crypt_stat->flags & ECRYPTFS_PLAINTEXT_PASSTHROUGH_ENABLED)
		seq_printf(m, ",ecryptfs_passthrough");
	if (mount_crypt_stat->flags & ECRYPTFS_XATTR_METADATA_ENABLED)
		seq_printf(m, ",ecryptfs_xattr_metadata");
	if (mount_crypt_stat->flags & ECRYPTFS_ENCRYPTED_VIEW_ENABLED)
		seq_printf(m, ",ecryptfs_encrypted_view");
	if (mount_crypt_stat->flags & ECRYPTFS_UNLINK_SIGS)
		seq_printf(m, ",ecryptfs_unlink_sigs");
#ifdef MY_ABC_HERE
	if (mount_crypt_stat->flags & ECRYPTFS_SYNO_ERROR_REPORT)
		seq_printf(m, ",syno_error_report");
#endif

	return 0;
}

#ifdef MY_ABC_HERE
static int ecryptfs_get_sb_archive_ver(struct super_block *sb, u32 *archive_ver)
{
	struct super_block *lower_sb = ecryptfs_superblock_to_lower(sb);
	if (!lower_sb->s_op->syno_get_sb_archive_ver)
		return -EINVAL;
	return lower_sb->s_op->syno_get_sb_archive_ver(lower_sb, archive_ver);
}

static int ecryptfs_set_sb_archive_ver(struct super_block *sb, u32 archive_ver)
{
	struct super_block *lower_sb = ecryptfs_superblock_to_lower(sb);
	if (!lower_sb->s_op->syno_set_sb_archive_ver)
		return -EINVAL;
	return lower_sb->s_op->syno_set_sb_archive_ver(lower_sb, archive_ver);
}
#endif  

const struct super_operations ecryptfs_sops = {
#ifdef MY_ABC_HERE
	.syno_get_sb_archive_ver = ecryptfs_get_sb_archive_ver,
	.syno_set_sb_archive_ver = ecryptfs_set_sb_archive_ver,
#endif
	.alloc_inode = ecryptfs_alloc_inode,
	.destroy_inode = ecryptfs_destroy_inode,
	.drop_inode = generic_delete_inode,
	.put_super = ecryptfs_put_super,
	.statfs = ecryptfs_statfs,
	.remount_fs = NULL,
	.clear_inode = ecryptfs_clear_inode,
	.show_options = ecryptfs_show_options
};
