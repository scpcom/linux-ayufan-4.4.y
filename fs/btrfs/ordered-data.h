#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __BTRFS_ORDERED_DATA__
#define __BTRFS_ORDERED_DATA__
#if defined(MY_DEF_HERE)
#include "ctree.h"
#endif  

struct btrfs_ordered_inode_tree {
	spinlock_t lock;
	struct rb_root tree;
	struct rb_node *last;
};

struct btrfs_ordered_sum {
	 
	u64 bytenr;

	int len;
	struct list_head list;
	 
	u32 sums[];
};

#define BTRFS_ORDERED_IO_DONE 0  

#define BTRFS_ORDERED_COMPLETE 1  

#define BTRFS_ORDERED_NOCOW 2  

#define BTRFS_ORDERED_COMPRESSED 3  

#define BTRFS_ORDERED_PREALLOC 4  

#define BTRFS_ORDERED_DIRECT 5  

#define BTRFS_ORDERED_IOERR 6  

#define BTRFS_ORDERED_UPDATED_ISIZE 7  
#define BTRFS_ORDERED_LOGGED_CSUM 8  
#define BTRFS_ORDERED_TRUNCATED 9  

#define BTRFS_ORDERED_LOGGED 10  
#define BTRFS_ORDERED_PENDING 11  
struct btrfs_ordered_extent {
	 
	u64 file_offset;

	u64 start;

	u64 len;

	u64 disk_len;

	u64 bytes_left;

	u64 outstanding_isize;

	u64 truncated_len;

	unsigned long flags;

	int compress_type;

	atomic_t refs;

	struct inode *inode;

	struct list_head list;

	struct list_head log_list;

	struct list_head trans_list;

	wait_queue_head_t wait;

	struct rb_node rb_node;

	struct list_head root_extent_list;

	struct btrfs_work work;

	struct completion completion;
	struct btrfs_work flush_work;
	struct list_head work_list;
};

static inline int btrfs_ordered_sum_size(struct btrfs_root *root,
					 unsigned long bytes)
{
	int num_sectors = (int)DIV_ROUND_UP(bytes, root->sectorsize);
	return sizeof(struct btrfs_ordered_sum) + num_sectors * sizeof(u32);
}

static inline void
btrfs_ordered_inode_tree_init(struct btrfs_ordered_inode_tree *t)
{
	spin_lock_init(&t->lock);
	t->tree = RB_ROOT;
	t->last = NULL;
}

void btrfs_put_ordered_extent(struct btrfs_ordered_extent *entry);
void btrfs_remove_ordered_extent(struct inode *inode,
				struct btrfs_ordered_extent *entry);
int btrfs_dec_test_ordered_pending(struct inode *inode,
				   struct btrfs_ordered_extent **cached,
				   u64 file_offset, u64 io_size, int uptodate);
int btrfs_dec_test_first_ordered_pending(struct inode *inode,
				   struct btrfs_ordered_extent **cached,
				   u64 *file_offset, u64 io_size,
				   int uptodate);
int btrfs_add_ordered_extent(struct inode *inode, u64 file_offset,
			     u64 start, u64 len, u64 disk_len, int type);
int btrfs_add_ordered_extent_dio(struct inode *inode, u64 file_offset,
				 u64 start, u64 len, u64 disk_len, int type);
int btrfs_add_ordered_extent_compress(struct inode *inode, u64 file_offset,
				      u64 start, u64 len, u64 disk_len,
				      int type, int compress_type);
void btrfs_add_ordered_sum(struct inode *inode,
			   struct btrfs_ordered_extent *entry,
			   struct btrfs_ordered_sum *sum);
struct btrfs_ordered_extent *btrfs_lookup_ordered_extent(struct inode *inode,
							 u64 file_offset);
void btrfs_start_ordered_extent(struct inode *inode,
				struct btrfs_ordered_extent *entry, int wait);
int btrfs_wait_ordered_range(struct inode *inode, u64 start, u64 len);
struct btrfs_ordered_extent *
btrfs_lookup_first_ordered_extent(struct inode * inode, u64 file_offset);
struct btrfs_ordered_extent *btrfs_lookup_ordered_range(struct inode *inode,
							u64 file_offset,
							u64 len);
bool btrfs_have_ordered_extents_in_range(struct inode *inode,
					 u64 file_offset,
					 u64 len);
int btrfs_ordered_update_i_size(struct inode *inode, u64 offset,
				struct btrfs_ordered_extent *ordered);
int btrfs_find_ordered_sum(struct inode *inode, u64 offset, u64 disk_bytenr,
			   u32 *sum, int len);
int btrfs_wait_ordered_extents(struct btrfs_root *root, int nr);
void btrfs_wait_ordered_roots(struct btrfs_fs_info *fs_info, int nr);
void btrfs_get_logged_extents(struct inode *inode,
			      struct list_head *logged_list,
			      const loff_t start,
			      const loff_t end);
void btrfs_put_logged_extents(struct list_head *logged_list);
void btrfs_submit_logged_extents(struct list_head *logged_list,
				 struct btrfs_root *log);
void btrfs_wait_logged_extents(struct btrfs_trans_handle *trans,
			       struct btrfs_root *log, u64 transid);
void btrfs_free_logged_extents(struct btrfs_root *log, u64 transid);
int __init ordered_data_init(void);
void ordered_data_exit(void);
#endif
