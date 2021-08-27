#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __TREE_LOG_
#define __TREE_LOG_

#define BTRFS_NO_LOG_SYNC 256

struct btrfs_log_ctx {
	int log_ret;
#ifdef MY_ABC_HERE
#else
	int log_transid;
#endif
	struct list_head list;
};

static inline void btrfs_init_log_ctx(struct btrfs_log_ctx *ctx)
{
	ctx->log_ret = 0;
#ifdef MY_ABC_HERE
#else
	ctx->log_transid = 0;
#endif
	INIT_LIST_HEAD(&ctx->list);
}

int btrfs_sync_log(struct btrfs_trans_handle *trans,
		   struct btrfs_root *root, struct btrfs_log_ctx *ctx);
int btrfs_free_log(struct btrfs_trans_handle *trans, struct btrfs_root *root);
int btrfs_free_log_root_tree(struct btrfs_trans_handle *trans,
			     struct btrfs_fs_info *fs_info);
int btrfs_recover_log_trees(struct btrfs_root *tree_root);
int btrfs_log_dentry_safe(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root, struct dentry *dentry,
			  struct btrfs_log_ctx *ctx);
int btrfs_del_dir_entries_in_log(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 const char *name, int name_len,
				 struct inode *dir, u64 index);
int btrfs_del_inode_ref_in_log(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root,
			       const char *name, int name_len,
			       struct inode *inode, u64 dirid);
void btrfs_end_log_trans(struct btrfs_root *root);
int btrfs_pin_log_trans(struct btrfs_root *root);
void btrfs_record_unlink_dir(struct btrfs_trans_handle *trans,
			     struct inode *dir, struct inode *inode,
			     int for_rename);
int btrfs_log_new_name(struct btrfs_trans_handle *trans,
			struct inode *inode, struct inode *old_dir,
			struct dentry *parent);
#endif
