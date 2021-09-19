#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __BTRFS_QGROUP__
#define __BTRFS_QGROUP__

enum btrfs_qgroup_operation_type {
	BTRFS_QGROUP_OPER_ADD_EXCL,
	BTRFS_QGROUP_OPER_ADD_SHARED,
	BTRFS_QGROUP_OPER_SUB_EXCL,
	BTRFS_QGROUP_OPER_SUB_SHARED,
};

struct btrfs_qgroup_operation {
	u64 ref_root;
	u64 bytenr;
	u64 num_bytes;
	u64 seq;
	enum btrfs_qgroup_operation_type type;
	struct seq_list elem;
	struct rb_node n;
	struct list_head list;
};

int btrfs_quota_enable(struct btrfs_trans_handle *trans,
		       struct btrfs_fs_info *fs_info);
int btrfs_quota_disable(struct btrfs_trans_handle *trans,
			struct btrfs_fs_info *fs_info);
int btrfs_qgroup_rescan(struct btrfs_fs_info *fs_info);
void btrfs_qgroup_rescan_resume(struct btrfs_fs_info *fs_info);
int btrfs_qgroup_wait_for_completion(struct btrfs_fs_info *fs_info);
int btrfs_add_qgroup_relation(struct btrfs_trans_handle *trans,
			      struct btrfs_fs_info *fs_info, u64 src, u64 dst);
int btrfs_del_qgroup_relation(struct btrfs_trans_handle *trans,
			      struct btrfs_fs_info *fs_info, u64 src, u64 dst);
int btrfs_create_qgroup(struct btrfs_trans_handle *trans,
			struct btrfs_fs_info *fs_info, u64 qgroupid,
			char *name);
int btrfs_remove_qgroup(struct btrfs_trans_handle *trans,
			      struct btrfs_fs_info *fs_info, u64 qgroupid);
int btrfs_limit_qgroup(struct btrfs_trans_handle *trans,
		       struct btrfs_fs_info *fs_info, u64 qgroupid,
		       struct btrfs_qgroup_limit *limit);
int btrfs_read_qgroup_config(struct btrfs_fs_info *fs_info);
void btrfs_free_qgroup_config(struct btrfs_fs_info *fs_info);
struct btrfs_delayed_extent_op;
int btrfs_qgroup_record_ref(struct btrfs_trans_handle *trans,
			    struct btrfs_fs_info *fs_info, u64 ref_root,
			    u64 bytenr, u64 num_bytes,
			    enum btrfs_qgroup_operation_type type,
			    int mod_seq);
int btrfs_delayed_qgroup_accounting(struct btrfs_trans_handle *trans,
				    struct btrfs_fs_info *fs_info);
void btrfs_remove_qgroup_operation(struct btrfs_trans_handle *trans,
				   struct btrfs_fs_info *fs_info,
				   struct btrfs_qgroup_operation *oper);
int btrfs_run_qgroups(struct btrfs_trans_handle *trans,
		      struct btrfs_fs_info *fs_info);
int btrfs_qgroup_inherit(struct btrfs_trans_handle *trans,
			 struct btrfs_fs_info *fs_info, u64 srcid, u64 objectid,
			 struct btrfs_qgroup_inherit *inherit);
int btrfs_qgroup_reserve(struct btrfs_root *root, u64 num_bytes);
void btrfs_qgroup_free(struct btrfs_root *root, u64 num_bytes);

#ifdef MY_ABC_HERE
void btrfs_qgroup_query(struct btrfs_fs_info *fs_info, u64 qgroupid,
                        struct btrfs_ioctl_qgroup_query_args *qqa);
#endif  
void assert_qgroups_uptodate(struct btrfs_trans_handle *trans);

#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS
int btrfs_verify_qgroup_counts(struct btrfs_fs_info *fs_info, u64 qgroupid,
			       u64 rfer, u64 excl);
#endif

#endif  
