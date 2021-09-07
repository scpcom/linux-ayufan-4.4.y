#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <linux/limits.h>
#include <linux/ioctl.h>

#ifdef MY_ABC_HERE
#include <linux/net.h>
#endif  

#undef NR_OPEN
#define INR_OPEN 1024		 

#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)

#define SEEK_SET	0	 
#define SEEK_CUR	1	 
#define SEEK_END	2	 
#define SEEK_MAX	SEEK_END

struct files_stat_struct {
	int nr_files;		 
	int nr_free_files;	 
	int max_files;		 
};

struct inodes_stat_t {
	int nr_inodes;
	int nr_unused;
	int dummy[5];		 
};

#define NR_FILE  8192	 

#ifdef CONFIG_FS_SYNO_ACL 
 
#define MAY_EXEC				(0x0001)
#define MAY_WRITE				(0x0002)
#define MAY_READ				(0x0004)
#define MAY_APPEND				(0x0008)
#define MAY_ACCESS 				(0x0010) 
#define MAY_OPEN 				(0x0020) 
#define MAY_READ_EXT_ATTR		(0x0040)
#define MAY_READ_PERMISSION		(0x0080)
#define MAY_READ_ATTR			(0x0100)
#define MAY_WRITE_ATTR			(0x0200)
#define MAY_WRITE_EXT_ATTR		(0x0400)
#define MAY_WRITE_PERMISSION	(0x0800)
#define MAY_DEL					(0x1000)
#define MAY_DEL_CHILD			(0x2000)
#define MAY_GET_OWNER_SHIP		(0x4000)
#else  
#define MAY_EXEC 1
#define MAY_WRITE 2
#define MAY_READ 4
#define MAY_APPEND 8
#define MAY_ACCESS 16
#define MAY_OPEN 32
#endif  
 
#define FMODE_READ		((__force fmode_t)1)
 
#define FMODE_WRITE		((__force fmode_t)2)
 
#define FMODE_LSEEK		((__force fmode_t)4)
 
#define FMODE_PREAD		((__force fmode_t)8)
 
#define FMODE_PWRITE		((__force fmode_t)16)
 
#define FMODE_EXEC		((__force fmode_t)32)
 
#define FMODE_NDELAY		((__force fmode_t)64)
 
#define FMODE_EXCL		((__force fmode_t)128)
 
#define FMODE_WRITE_IOCTL	((__force fmode_t)256)

#define FMODE_NOCMTIME		((__force fmode_t)2048)

#define FMODE_RANDOM		((__force fmode_t)4096)

#define RW_MASK		1
#define RWA_MASK	2
#define READ 0
#define WRITE 1
#define READA 2		 
#define SWRITE 3	 
#define READ_SYNC	(READ | (1 << BIO_RW_SYNCIO) | (1 << BIO_RW_UNPLUG))
#define READ_META	(READ | (1 << BIO_RW_META))
#define WRITE_SYNC_PLUG	(WRITE | (1 << BIO_RW_SYNCIO) | (1 << BIO_RW_NOIDLE))
#define WRITE_SYNC	(WRITE_SYNC_PLUG | (1 << BIO_RW_UNPLUG))
#define WRITE_ODIRECT	(WRITE | (1 << BIO_RW_SYNCIO) | (1 << BIO_RW_UNPLUG))
#define SWRITE_SYNC_PLUG	\
			(SWRITE | (1 << BIO_RW_SYNCIO) | (1 << BIO_RW_NOIDLE))
#define SWRITE_SYNC	(SWRITE_SYNC_PLUG | (1 << BIO_RW_UNPLUG))
#define WRITE_BARRIER	(WRITE | (1 << BIO_RW_BARRIER))

#define DISCARD_NOBARRIER (WRITE | (1 << BIO_RW_DISCARD))
#define DISCARD_BARRIER (DISCARD_NOBARRIER | (1 << BIO_RW_BARRIER))

#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

#define FS_REQUIRES_DEV 1 
#define FS_BINARY_MOUNTDATA 2
#define FS_HAS_SUBTYPE 4
#define FS_REVAL_DOT	16384	 
#define FS_RENAME_DOES_D_MOVE	32768	 

#define MS_RDONLY	 1	 
#define MS_NOSUID	 2	 
#define MS_NODEV	 4	 
#define MS_NOEXEC	 8	 
#define MS_SYNCHRONOUS	16	 
#define MS_REMOUNT	32	 
#define MS_MANDLOCK	64	 
#define MS_DIRSYNC	128	 
#define MS_NOATIME	1024	 
#define MS_NODIRATIME	2048	 
#define MS_BIND		4096
#define MS_MOVE		8192
#define MS_REC		16384
#define MS_VERBOSE	32768	 
#define MS_SILENT	32768
#define MS_POSIXACL	(1<<16)	 
#define MS_UNBINDABLE	(1<<17)	 
#define MS_PRIVATE	(1<<18)	 
#define MS_SLAVE	(1<<19)	 
#define MS_SHARED	(1<<20)	 
#define MS_RELATIME	(1<<21)	 
#define MS_KERNMOUNT	(1<<22)  
#define MS_I_VERSION	(1<<23)  
#define MS_STRICTATIME	(1<<24)  
#ifdef CONFIG_FS_SYNO_ACL  
#define MS_SYNOACL	(1<<27)	 
#endif
#ifdef SYNO_FORCE_UNMOUNT
#define MS_UNMOUNT_WAIT	(1<<28)	 
#endif
#ifdef MY_ABC_HERE
#define MS_CORRUPT	(1<<29)	 
#endif
#define MS_ACTIVE	(1<<30)
#define MS_NOUSER	(1<<31)

#define MS_RMT_MASK	(MS_RDONLY|MS_SYNCHRONOUS|MS_MANDLOCK|MS_I_VERSION)

#define MS_MGC_VAL 0xC0ED0000
#define MS_MGC_MSK 0xffff0000

#define S_SYNC		1	 
#define S_NOATIME	2	 
#define S_APPEND	4	 
#define S_IMMUTABLE	8	 
#define S_DEAD		16	 
#define S_NOQUOTA	32	 
#define S_DIRSYNC	64	 
#define S_NOCMTIME	128	 
#define S_SWAPFILE	256	 
#define S_PRIVATE	512	 

#define S_SYNO_FORCE_UMOUNT	1024 

#define __IS_FLG(inode,flg) ((inode)->i_sb->s_flags & (flg))

#define IS_RDONLY(inode) ((inode)->i_sb->s_flags & MS_RDONLY)
#define IS_SYNC(inode)		(__IS_FLG(inode, MS_SYNCHRONOUS) || \
					((inode)->i_flags & S_SYNC))
#define IS_DIRSYNC(inode)	(__IS_FLG(inode, MS_SYNCHRONOUS|MS_DIRSYNC) || \
					((inode)->i_flags & (S_SYNC|S_DIRSYNC)))
#define IS_MANDLOCK(inode)	__IS_FLG(inode, MS_MANDLOCK)
#define IS_NOATIME(inode)   __IS_FLG(inode, MS_RDONLY|MS_NOATIME)
#define IS_I_VERSION(inode)   __IS_FLG(inode, MS_I_VERSION)

#define IS_NOQUOTA(inode)	((inode)->i_flags & S_NOQUOTA)
#define IS_APPEND(inode)	((inode)->i_flags & S_APPEND)
#define IS_IMMUTABLE(inode)	((inode)->i_flags & S_IMMUTABLE)
#ifdef SYNO_FORCE_UNMOUNT
#define IS_UMOUNTED_FILE(inode)	((inode)->i_flags & S_SYNO_FORCE_UMOUNT)
#endif
#define IS_POSIXACL(inode)	__IS_FLG(inode, MS_POSIXACL)

#ifdef CONFIG_FS_SYNO_ACL
#define IS_SMB_READONLY(inode)	((inode)->i_archive_bit & S2_SMB_READONLY)
#define IS_SYNOACL_SUPERUSER() (0 == current_fsuid())

#define IS_INODE_SYNOACL(inode)	((inode)->i_archive_bit & S2_SYNO_ACL_SUPPORT)
#define IS_FS_SYNOACL(inode)	__IS_FLG(inode, MS_SYNOACL)
#define IS_SYNOACL(inode)	(IS_INODE_SYNOACL(inode) && IS_FS_SYNOACL(inode))
#define IS_SYNOACL_DENTRY(dentry)	(IS_INODE_SYNOACL(dentry->d_inode) && IS_FS_SYNOACL(dentry->d_inode))

#define IS_SYNOACL_INHERIT(inode)	((inode)->i_archive_bit & S2_SYNO_ACL_INHERIT)
#define IS_SYNOACL_OWNER_IS_GROUP(inode)	((inode)->i_archive_bit & S2_SYNO_ACL_IS_OWNER_GROUP)
#define IS_SYNOACL_EXIST(inode)	((inode)->i_archive_bit & S2_SYNO_ACL_EXIST)
#define HAS_SYNOACL(inode) ((IS_SYNOACL_EXIST(inode) || IS_SYNOACL_INHERIT(inode)))
#define is_synoacl_owner(inode)	IS_SYNOACL_OWNER_IS_GROUP(inode)?in_group_p(inode->i_gid):(inode->i_uid == current_fsuid())
#define is_synoacl_owner_or_capable(inode) (is_synoacl_owner(inode) || capable(CAP_FOWNER))

#endif  
#define IS_DEADDIR(inode)	((inode)->i_flags & S_DEAD)
#define IS_NOCMTIME(inode)	((inode)->i_flags & S_NOCMTIME)
#define IS_SWAPFILE(inode)	((inode)->i_flags & S_SWAPFILE)
#define IS_PRIVATE(inode)	((inode)->i_flags & S_PRIVATE)

#define BLKROSET   _IO(0x12,93)	 
#define BLKROGET   _IO(0x12,94)	 
#define BLKRRPART  _IO(0x12,95)	 
#define BLKGETSIZE _IO(0x12,96)	 
#define BLKFLSBUF  _IO(0x12,97)	 
#define BLKRASET   _IO(0x12,98)	 
#define BLKRAGET   _IO(0x12,99)	 
#define BLKFRASET  _IO(0x12,100) 
#define BLKFRAGET  _IO(0x12,101) 
#define BLKSECTSET _IO(0x12,102) 
#define BLKSECTGET _IO(0x12,103) 
#define BLKSSZGET  _IO(0x12,104) 
#if 0
#define BLKPG      _IO(0x12,105) 

#define BLKELVGET  _IOR(0x12,106,size_t) 
#define BLKELVSET  _IOW(0x12,107,size_t) 
 
#endif
 
#define BLKBSZGET  _IOR(0x12,112,size_t)
#define BLKBSZSET  _IOW(0x12,113,size_t)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)	 
#define BLKTRACESETUP _IOWR(0x12,115,struct blk_user_trace_setup)
#define BLKTRACESTART _IO(0x12,116)
#define BLKTRACESTOP _IO(0x12,117)
#define BLKTRACETEARDOWN _IO(0x12,118)
#define BLKDISCARD _IO(0x12,119)
#define BLKIOMIN _IO(0x12,120)
#define BLKIOOPT _IO(0x12,121)
#define BLKALIGNOFF _IO(0x12,122)
#define BLKPBSZGET _IO(0x12,123)

#define BMAP_IOCTL 1		 
#define FIBMAP	   _IO(0x00,1)	 
#define FIGETBSZ   _IO(0x00,2)	 
#define FIFREEZE	_IOWR('X', 119, int)	 
#define FITHAW		_IOWR('X', 120, int)	 
#ifdef MY_ABC_HERE
#define FIGETVERSION	_IOWR('x', 121, unsigned int)	 
#define FISETVERSION	_IOWR('x', 122, unsigned int)	 
#define FIINCVERSION	_IO('x', 123)	 
#endif

#define	FS_IOC_GETFLAGS			_IOR('f', 1, long)
#define	FS_IOC_SETFLAGS			_IOW('f', 2, long)
#define	FS_IOC_GETVERSION		_IOR('v', 1, long)
#define	FS_IOC_SETVERSION		_IOW('v', 2, long)
#define FS_IOC_FIEMAP			_IOWR('f', 11, struct fiemap)
#define FS_IOC32_GETFLAGS		_IOR('f', 1, int)
#define FS_IOC32_SETFLAGS		_IOW('f', 2, int)
#define FS_IOC32_GETVERSION		_IOR('v', 1, int)
#define FS_IOC32_SETVERSION		_IOW('v', 2, int)

#define	FS_SECRM_FL			0x00000001  
#define	FS_UNRM_FL			0x00000002  
#define	FS_COMPR_FL			0x00000004  
#define FS_SYNC_FL			0x00000008  
#define FS_IMMUTABLE_FL			0x00000010  
#define FS_APPEND_FL			0x00000020  
#define FS_NODUMP_FL			0x00000040  
#define FS_NOATIME_FL			0x00000080  
 
#define FS_DIRTY_FL			0x00000100
#define FS_COMPRBLK_FL			0x00000200  
#define FS_NOCOMP_FL			0x00000400  
#define FS_ECOMPR_FL			0x00000800  
 
#define FS_BTREE_FL			0x00001000  
#define FS_INDEX_FL			0x00001000  
#define FS_IMAGIC_FL			0x00002000  
#define FS_JOURNAL_DATA_FL		0x00004000  
#define FS_NOTAIL_FL			0x00008000  
#define FS_DIRSYNC_FL			0x00010000  
#define FS_TOPDIR_FL			0x00020000  
#define FS_EXTENT_FL			0x00080000  
#define FS_DIRECTIO_FL			0x00100000  
#define FS_RESERVED_FL			0x80000000  

#define FS_FL_USER_VISIBLE		0x0003DFFF  
#define FS_FL_USER_MODIFIABLE		0x000380FF  

#define SYNC_FILE_RANGE_WAIT_BEFORE	1
#define SYNC_FILE_RANGE_WRITE		2
#define SYNC_FILE_RANGE_WAIT_AFTER	4

#ifdef __KERNEL__

#include <linux/linkage.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/dcache.h>
#include <linux/path.h>
#include <linux/stat.h>
#include <linux/cache.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/radix-tree.h>
#include <linux/prio_tree.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/mutex.h>
#include <linux/capability.h>
#include <linux/semaphore.h>
#include <linux/fiemap.h>

#include <asm/atomic.h>
#include <asm/byteorder.h>

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
#include <linux/time.h>
#endif  

#ifdef MY_ABC_HERE
enum bypass_synoacl_type {
	BYPASS_SYNOACL_SYNOUTIME,
	BYPASS_SYNOACL_SYNOARCHIVE_OVERWRITE,
	BYPASS_SYNOACL_SYNOARCHIVE_OVERWRITE_ACL,
	BYPASS_SYNOACL_SYNOACL_XATTR,
	BYPASS_SYNOACL_MAX
};
#endif  
struct export_operations;
struct hd_geometry;
struct iovec;
struct nameidata;
struct kiocb;
struct pipe_inode_info;
struct poll_table_struct;
struct kstatfs;
struct vm_area_struct;
struct vfsmount;
struct cred;

extern void __init inode_init(void);
extern void __init inode_init_early(void);
extern void __init files_init(unsigned long);

extern struct files_stat_struct files_stat;
extern int get_max_files(void);
extern int sysctl_nr_open;
extern struct inodes_stat_t inodes_stat;
extern int leases_enable, lease_break_time;
#ifdef CONFIG_DNOTIFY
extern int dir_notify_enable;
#endif

struct buffer_head;
typedef int (get_block_t)(struct inode *inode, sector_t iblock,
			struct buffer_head *bh_result, int create);
typedef void (dio_iodone_t)(struct kiocb *iocb, loff_t offset,
			ssize_t bytes, void *private);

#define ATTR_MODE	(1 << 0)
#define ATTR_UID	(1 << 1)
#define ATTR_GID	(1 << 2)
#define ATTR_SIZE	(1 << 3)
#define ATTR_ATIME	(1 << 4)
#define ATTR_MTIME	(1 << 5)
#define ATTR_CTIME	(1 << 6)
#define ATTR_ATIME_SET	(1 << 7)
#define ATTR_MTIME_SET	(1 << 8)
#define ATTR_FORCE	(1 << 9)  
#define ATTR_ATTR_FLAG	(1 << 10)
#define ATTR_KILL_SUID	(1 << 11)
#define ATTR_KILL_SGID	(1 << 12)
#define ATTR_FILE	(1 << 13)
#define ATTR_KILL_PRIV	(1 << 14)
#define ATTR_OPEN	(1 << 15)  
#define ATTR_TIMES_SET	(1 << 16)

struct iattr {
	unsigned int	ia_valid;
	umode_t		ia_mode;
	uid_t		ia_uid;
	gid_t		ia_gid;
	loff_t		ia_size;
	struct timespec	ia_atime;
	struct timespec	ia_mtime;
	struct timespec	ia_ctime;

	struct file	*ia_file;
};

#include <linux/quota.h>

enum positive_aop_returns {
	AOP_WRITEPAGE_ACTIVATE	= 0x80000,
	AOP_TRUNCATED_PAGE	= 0x80001,
};

#define AOP_FLAG_UNINTERRUPTIBLE	0x0001  
#define AOP_FLAG_CONT_EXPAND		0x0002  
#define AOP_FLAG_NOFS			0x0004  
#ifdef MY_ABC_HERE
#define AOP_FLAG_RECVFILE		0x0008
#endif

struct page;
struct address_space;
struct writeback_control;

struct iov_iter {
	const struct iovec *iov;
	unsigned long nr_segs;
	size_t iov_offset;
	size_t count;
};

size_t iov_iter_copy_from_user_atomic(struct page *page,
		struct iov_iter *i, unsigned long offset, size_t bytes);
size_t iov_iter_copy_from_user(struct page *page,
		struct iov_iter *i, unsigned long offset, size_t bytes);
void iov_iter_advance(struct iov_iter *i, size_t bytes);
int iov_iter_fault_in_readable(struct iov_iter *i, size_t bytes);
size_t iov_iter_single_seg_count(struct iov_iter *i);

static inline void iov_iter_init(struct iov_iter *i,
			const struct iovec *iov, unsigned long nr_segs,
			size_t count, size_t written)
{
	i->iov = iov;
	i->nr_segs = nr_segs;
	i->iov_offset = 0;
	i->count = count + written;

	iov_iter_advance(i, written);
}

static inline size_t iov_iter_count(struct iov_iter *i)
{
	return i->count;
}

typedef struct {
	size_t written;
	size_t count;
	union {
		char __user *buf;
		void *data;
	} arg;
	int error;
} read_descriptor_t;

#ifdef CONFIG_SYNO_PLX_PORTING
typedef int (*read_actor_t)(read_descriptor_t *, struct page **,
#else
typedef int (*read_actor_t)(read_descriptor_t *, struct page *,
#endif
		unsigned long, unsigned long);

struct address_space_operations {
	int (*writepage)(struct page *page, struct writeback_control *wbc);
	int (*readpage)(struct file *, struct page *);
	void (*sync_page)(struct page *);

	int (*writepages)(struct address_space *, struct writeback_control *);

	int (*set_page_dirty)(struct page *page);

	int (*readpages)(struct file *filp, struct address_space *mapping,
			struct list_head *pages, unsigned nr_pages);

	int (*write_begin)(struct file *, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned flags,
				struct page **pagep, void **fsdata);
	int (*write_end)(struct file *, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned copied,
				struct page *page, void *fsdata);

#ifdef MY_ABC_HERE
	 
	int (*prepare_write)(struct file *, struct page *, unsigned, unsigned);
	int (*commit_write)(struct file *, struct page *, unsigned, unsigned);
#endif
	 
	sector_t (*bmap)(struct address_space *, sector_t);
	void (*invalidatepage) (struct page *, unsigned long);
	int (*releasepage) (struct page *, gfp_t);
	ssize_t (*direct_IO)(int, struct kiocb *, const struct iovec *iov,
			loff_t offset, unsigned long nr_segs);
	int (*get_xip_mem)(struct address_space *, pgoff_t, int,
						void **, unsigned long *);
	 
	int (*migratepage) (struct address_space *,
			struct page *, struct page *);
	int (*launder_page) (struct page *);
	int (*is_partially_uptodate) (struct page *, read_descriptor_t *,
					unsigned long);
	int (*error_remove_page)(struct address_space *, struct page *);
};

int pagecache_write_begin(struct file *, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned flags,
				struct page **pagep, void **fsdata);

int pagecache_write_end(struct file *, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned copied,
				struct page *page, void *fsdata);

struct backing_dev_info;
struct address_space {
	struct inode		*host;		 
	struct radix_tree_root	page_tree;	 
	spinlock_t		tree_lock;	 
	unsigned int		i_mmap_writable; 
	struct prio_tree_root	i_mmap;		 
	struct list_head	i_mmap_nonlinear; 
	spinlock_t		i_mmap_lock;	 
	unsigned int		truncate_count;	 
	unsigned long		nrpages;	 
	pgoff_t			writeback_index; 
	const struct address_space_operations *a_ops;	 
	unsigned long		flags;		 
	struct backing_dev_info *backing_dev_info;  
	spinlock_t		private_lock;	 
	struct list_head	private_list;	 
	struct address_space	*assoc_mapping;	 
} __attribute__((aligned(sizeof(long))));
	 
struct block_device {
	dev_t			bd_dev;   
	struct inode *		bd_inode;	 
	struct super_block *	bd_super;
	int			bd_openers;
	struct mutex		bd_mutex;	 
	struct list_head	bd_inodes;
	void *			bd_holder;
	int			bd_holders;
#ifdef CONFIG_SYSFS
	struct list_head	bd_holder_list;
#endif
	struct block_device *	bd_contains;
	unsigned		bd_block_size;
	struct hd_struct *	bd_part;
	 
	unsigned		bd_part_count;
	int			bd_invalidated;
	struct gendisk *	bd_disk;
	struct list_head	bd_list;
	 
	unsigned long		bd_private;

	int			bd_fsfreeze_count;
	 
	struct mutex		bd_fsfreeze_mutex;
};

#define PAGECACHE_TAG_DIRTY	0
#define PAGECACHE_TAG_WRITEBACK	1

int mapping_tagged(struct address_space *mapping, int tag);

static inline int mapping_mapped(struct address_space *mapping)
{
	return	!prio_tree_empty(&mapping->i_mmap) ||
		!list_empty(&mapping->i_mmap_nonlinear);
}

static inline int mapping_writably_mapped(struct address_space *mapping)
{
	return mapping->i_mmap_writable != 0;
}

#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
#include <linux/seqlock.h>
#define __NEED_I_SIZE_ORDERED
#define i_size_ordered_init(inode) seqcount_init(&inode->i_size_seqcount)
#else
#define i_size_ordered_init(inode) do { } while (0)
#endif

struct posix_acl;
#ifdef CONFIG_FS_SYNO_ACL
struct syno_acl;
#endif
#define ACL_NOT_CACHED ((void *)(-1))

#ifdef CONFIG_SYNO_PLX_PORTING
#include <mach/filemap_info.h>
#endif
struct inode {
	struct hlist_node	i_hash;
	struct list_head	i_list;		 
	struct list_head	i_sb_list;
	struct list_head	i_dentry;
	unsigned long		i_ino;
	atomic_t		i_count;
	unsigned int		i_nlink;
	uid_t			i_uid;
	gid_t			i_gid;
	dev_t			i_rdev;
	u64			i_version;
	loff_t			i_size;
#ifdef __NEED_I_SIZE_ORDERED
	seqcount_t		i_size_seqcount;
#endif
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
#ifdef MY_ABC_HERE
	struct timespec		i_create_time;
#endif
#ifdef MY_ABC_HERE
	__u32			i_archive_bit;
#endif
#ifdef MY_ABC_HERE
	__u32			i_archive_version;
#endif
	blkcnt_t		i_blocks;
	unsigned int		i_blkbits;
	unsigned short          i_bytes;
	umode_t			i_mode;
	spinlock_t		i_lock;	 
	struct mutex		i_mutex;
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
	struct mutex		i_syno_mutex;	 
#endif
	struct rw_semaphore	i_alloc_sem;
	const struct inode_operations	*i_op;
	const struct file_operations	*i_fop;	 
	struct super_block	*i_sb;
	struct file_lock	*i_flock;
	struct address_space	*i_mapping;
	struct address_space	i_data;
#ifdef CONFIG_QUOTA
	struct dquot		*i_dquot[MAXQUOTAS];
#endif
	struct list_head	i_devices;
	union {
		struct pipe_inode_info	*i_pipe;
		struct block_device	*i_bdev;
		struct cdev		*i_cdev;
	};

	__u32			i_generation;

#ifdef CONFIG_FSNOTIFY
	__u32			i_fsnotify_mask;  
	struct hlist_head	i_fsnotify_mark_entries;  
#endif

#ifdef CONFIG_INOTIFY
	struct list_head	inotify_watches;  
	struct mutex		inotify_mutex;	 
#endif

	unsigned long		i_state;
	unsigned long		dirtied_when;	 

	unsigned int		i_flags;

	atomic_t		i_writecount;
#ifdef CONFIG_SECURITY
	void			*i_security;
#endif
#ifdef CONFIG_FS_SYNO_ACL
	struct syno_acl	*i_syno_acl;
#elif defined(CONFIG_FS_POSIX_ACL)
	struct posix_acl	*i_acl;
	struct posix_acl	*i_default_acl;
#endif
	void			*i_private;  
#ifdef CONFIG_SYNO_PLX_PORTING
	int 			space_reserve:1;

#ifdef CONFIG_OXNAS_FAST_OPEN_FILTER
	spinlock_t fast_lock;
	int        normal_open_count;
#endif  

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	int			          fast_open_count;
	struct list_head      fast_files;
	rwlock_t              fast_files_lock;
	oxnas_filemap_info_t  filemap_info;
	wait_queue_head_t     fallback_wait_queue;
	int                   fast_reads_in_progress_count;
	wait_queue_head_t     filemap_wait_queue;
	int                   fallback_in_progress:1,
#ifdef CONFIG_OXNAS_FAST_WRITES
                          writer_filemap_locked:1,
                          writer_filemap_dirty:1,
#endif  
						  filemap_locked:1,
						  filemap_update_pending:1;
#ifdef CONFIG_OXNAS_FAST_WRITES
	int                   fast_writes_in_progress_count;
	int 				  write_error;
	oxnas_filemap_info_t  writer_filemap_info;
	void 				 *writer_file_context;
	void                 *filemap_locker_uid;
	loff_t 				  i_tent_size;
#endif  
#endif  

#ifdef CONFIG_OXNAS_BACKUP
	int	backup_open_count;
#endif  
#endif 
};

enum inode_i_mutex_lock_class
{
	I_MUTEX_NORMAL,
	I_MUTEX_PARENT,
	I_MUTEX_CHILD,
	I_MUTEX_XATTR,
	I_MUTEX_QUOTA
};

static inline loff_t i_size_read(const struct inode *inode)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	loff_t i_size;
	unsigned int seq;

	do {
		seq = read_seqcount_begin(&inode->i_size_seqcount);
		i_size = inode->i_size;
	} while (read_seqcount_retry(&inode->i_size_seqcount, seq));
	return i_size;
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	loff_t i_size;

	preempt_disable();
	i_size = inode->i_size;
	preempt_enable();
	return i_size;
#else
	return inode->i_size;
#endif
}

static inline void i_size_write(struct inode *inode, loff_t i_size)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	write_seqcount_begin(&inode->i_size_seqcount);
	inode->i_size = i_size;
	write_seqcount_end(&inode->i_size_seqcount);
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	preempt_disable();
	inode->i_size = i_size;
	preempt_enable();
#else
	inode->i_size = i_size;
#endif
}

static inline unsigned iminor(const struct inode *inode)
{
	return MINOR(inode->i_rdev);
}

static inline unsigned imajor(const struct inode *inode)
{
	return MAJOR(inode->i_rdev);
}

extern struct block_device *I_BDEV(struct inode *inode);

struct fown_struct {
	rwlock_t lock;           
	struct pid *pid;	 
	enum pid_type pid_type;	 
	uid_t uid, euid;	 
	int signum;		 
};

struct file_ra_state {
	pgoff_t start;			 
	unsigned int size;		 
	unsigned int async_size;	 

#ifdef MY_DEF_HERE
	unsigned int delay_readahead;	 
#endif
	unsigned int ra_pages;		 
	unsigned int mmap_miss;		 
	loff_t prev_pos;		 
};

static inline int ra_has_index(struct file_ra_state *ra, pgoff_t index)
{
	return (index >= ra->start &&
		index <  ra->start + ra->size);
}

#define FILE_MNT_WRITE_TAKEN	1
#define FILE_MNT_WRITE_RELEASED	2

struct file {
	 
	union {
		struct list_head	fu_list;
		struct rcu_head 	fu_rcuhead;
	} f_u;
	struct path		f_path;
#define f_dentry	f_path.dentry
#define f_vfsmnt	f_path.mnt
	const struct file_operations	*f_op;
	spinlock_t		f_lock;   
	atomic_long_t		f_count;
#ifdef SYNO_FORCE_UNMOUNT
	unsigned int		f_synostate;
	spinlock_t			f_synostate_lock;
#endif
	unsigned int 		f_flags;
	fmode_t			f_mode;
	loff_t			f_pos;
	struct fown_struct	f_owner;
	const struct cred	*f_cred;
	struct file_ra_state	f_ra;

	u64			f_version;
#ifdef CONFIG_SECURITY
	void			*f_security;
#endif
	 
	void			*private_data;

#ifdef CONFIG_EPOLL
	 
	struct list_head	f_ep_links;
#endif  
	struct address_space	*f_mapping;
#ifdef CONFIG_DEBUG_WRITECOUNT
	unsigned long f_mnt_write_state;
#endif

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	struct inode     *inode;
	struct list_head  fast_head;
	void             *fast_context;
#ifdef CONFIG_OXNAS_FAST_WRITES
	void             *fast_write_context;
#endif  
#else  
#ifdef CONFIG_OXNAS_BACKUP
	struct inode     *inode;
#endif  
#endif  

#ifdef CONFIG_OXNAS_BACKUP
	void             *backup_context;
#endif  

};
#ifdef SYNO_FORCE_UNMOUNT
static inline int blSynostate(int flag, struct file *f)
{
	spin_lock(&f->f_synostate_lock);
	if (flag == f->f_synostate) {
		spin_unlock(&f->f_synostate_lock);
		return 1;
	}
	spin_unlock(&f->f_synostate_lock);
	return 0;
}
#endif
extern spinlock_t files_lock;
#define file_list_lock() spin_lock(&files_lock);
#define file_list_unlock() spin_unlock(&files_lock);

#define get_file(x)	atomic_long_inc(&(x)->f_count)
#define file_count(x)	atomic_long_read(&(x)->f_count)

#ifdef CONFIG_DEBUG_WRITECOUNT
static inline void file_take_write(struct file *f)
{
	WARN_ON(f->f_mnt_write_state != 0);
	f->f_mnt_write_state = FILE_MNT_WRITE_TAKEN;
}
static inline void file_release_write(struct file *f)
{
	f->f_mnt_write_state |= FILE_MNT_WRITE_RELEASED;
}
static inline void file_reset_write(struct file *f)
{
	f->f_mnt_write_state = 0;
}
static inline void file_check_state(struct file *f)
{
	 
	WARN_ON(f->f_mnt_write_state == FILE_MNT_WRITE_TAKEN);
	WARN_ON(f->f_mnt_write_state == FILE_MNT_WRITE_RELEASED);
}
static inline int file_check_writeable(struct file *f)
{
	if (f->f_mnt_write_state == FILE_MNT_WRITE_TAKEN)
		return 0;
	printk(KERN_WARNING "writeable file with no "
			    "mnt_want_write()\n");
	WARN_ON(1);
	return -EINVAL;
}
#else  
static inline void file_take_write(struct file *filp) {}
static inline void file_release_write(struct file *filp) {}
static inline void file_reset_write(struct file *filp) {}
static inline void file_check_state(struct file *filp) {}
static inline int file_check_writeable(struct file *filp)
{
	return 0;
}
#endif  

#define	MAX_NON_LFS	((1UL<<31) - 1)

#if BITS_PER_LONG==32
#define MAX_LFS_FILESIZE	(((u64)PAGE_CACHE_SIZE << (BITS_PER_LONG-1))-1) 
#elif BITS_PER_LONG==64
#define MAX_LFS_FILESIZE 	0x7fffffffffffffffUL
#endif

#define FL_POSIX	1
#define FL_FLOCK	2
#define FL_ACCESS	8	 
#define FL_EXISTS	16	 
#define FL_LEASE	32	 
#define FL_CLOSE	64	 
#define FL_SLEEP	128	 

#define FILE_LOCK_DEFERRED 1

typedef struct files_struct *fl_owner_t;

struct file_lock_operations {
	void (*fl_copy_lock)(struct file_lock *, struct file_lock *);
	void (*fl_release_private)(struct file_lock *);
};

struct lock_manager_operations {
	int (*fl_compare_owner)(struct file_lock *, struct file_lock *);
	void (*fl_notify)(struct file_lock *);	 
	int (*fl_grant)(struct file_lock *, struct file_lock *, int);
	void (*fl_copy_lock)(struct file_lock *, struct file_lock *);
	void (*fl_release_private)(struct file_lock *);
	void (*fl_break)(struct file_lock *);
	int (*fl_mylease)(struct file_lock *, struct file_lock *);
	int (*fl_change)(struct file_lock **, int);
};

struct lock_manager {
	struct list_head list;
};

void locks_start_grace(struct lock_manager *);
void locks_end_grace(struct lock_manager *);
int locks_in_grace(void);

#include <linux/nfs_fs_i.h>

struct file_lock {
	struct file_lock *fl_next;	 
	struct list_head fl_link;	 
	struct list_head fl_block;	 
	fl_owner_t fl_owner;
	unsigned char fl_flags;
	unsigned char fl_type;
	unsigned int fl_pid;
	struct pid *fl_nspid;
	wait_queue_head_t fl_wait;
	struct file *fl_file;
	loff_t fl_start;
	loff_t fl_end;

	struct fasync_struct *	fl_fasync;  
	unsigned long fl_break_time;	 

	const struct file_lock_operations *fl_ops;	 
	const struct lock_manager_operations *fl_lmops;	 
	union {
		struct nfs_lock_info	nfs_fl;
		struct nfs4_lock_info	nfs4_fl;
		struct {
			struct list_head link;	 
			int state;		 
		} afs;
	} fl_u;
};

#ifndef OFFSET_MAX
#define INT_LIMIT(x)	(~((x)1 << (sizeof(x)*8 - 1)))
#define OFFSET_MAX	INT_LIMIT(loff_t)
#define OFFT_OFFSET_MAX	INT_LIMIT(off_t)
#endif

#include <linux/fcntl.h>

extern void send_sigio(struct fown_struct *fown, int fd, int band);

extern int do_sync_mapping_range(struct address_space *mapping, loff_t offset,
			loff_t endbyte, unsigned int flags);

#ifdef CONFIG_FILE_LOCKING
extern int fcntl_getlk(struct file *, struct flock __user *);
extern int fcntl_setlk(unsigned int, struct file *, unsigned int,
			struct flock __user *);

#if BITS_PER_LONG == 32
extern int fcntl_getlk64(struct file *, struct flock64 __user *);
extern int fcntl_setlk64(unsigned int, struct file *, unsigned int,
			struct flock64 __user *);
#endif

extern int fcntl_setlease(unsigned int fd, struct file *filp, long arg);
extern int fcntl_getlease(struct file *filp);

extern void locks_init_lock(struct file_lock *);
extern void locks_copy_lock(struct file_lock *, struct file_lock *);
extern void __locks_copy_lock(struct file_lock *, const struct file_lock *);
extern void locks_remove_posix(struct file *, fl_owner_t);
extern void locks_remove_flock(struct file *);
extern void locks_release_private(struct file_lock *);
extern void posix_test_lock(struct file *, struct file_lock *);
extern int posix_lock_file(struct file *, struct file_lock *, struct file_lock *);
extern int posix_lock_file_wait(struct file *, struct file_lock *);
extern int posix_unblock_lock(struct file *, struct file_lock *);
extern int vfs_test_lock(struct file *, struct file_lock *);
extern int vfs_lock_file(struct file *, unsigned int, struct file_lock *, struct file_lock *);
extern int vfs_cancel_lock(struct file *filp, struct file_lock *fl);
extern int flock_lock_file_wait(struct file *filp, struct file_lock *fl);
extern int __break_lease(struct inode *inode, unsigned int flags);
extern void lease_get_mtime(struct inode *, struct timespec *time);
extern int generic_setlease(struct file *, long, struct file_lock **);
extern int vfs_setlease(struct file *, long, struct file_lock **);
extern int lease_modify(struct file_lock **, int);
extern int lock_may_read(struct inode *, loff_t start, unsigned long count);
extern int lock_may_write(struct inode *, loff_t start, unsigned long count);
#else  
static inline int fcntl_getlk(struct file *file, struct flock __user *user)
{
	return -EINVAL;
}

static inline int fcntl_setlk(unsigned int fd, struct file *file,
			      unsigned int cmd, struct flock __user *user)
{
	return -EACCES;
}

#if BITS_PER_LONG == 32
static inline int fcntl_getlk64(struct file *file, struct flock64 __user *user)
{
	return -EINVAL;
}

static inline int fcntl_setlk64(unsigned int fd, struct file *file,
				unsigned int cmd, struct flock64 __user *user)
{
	return -EACCES;
}
#endif
static inline int fcntl_setlease(unsigned int fd, struct file *filp, long arg)
{
	return 0;
}

static inline int fcntl_getlease(struct file *filp)
{
	return 0;
}

static inline void locks_init_lock(struct file_lock *fl)
{
	return;
}

static inline void __locks_copy_lock(struct file_lock *new, struct file_lock *fl)
{
	return;
}

static inline void locks_copy_lock(struct file_lock *new, struct file_lock *fl)
{
	return;
}

static inline void locks_remove_posix(struct file *filp, fl_owner_t owner)
{
	return;
}

static inline void locks_remove_flock(struct file *filp)
{
	return;
}

static inline void posix_test_lock(struct file *filp, struct file_lock *fl)
{
	return;
}

static inline int posix_lock_file(struct file *filp, struct file_lock *fl,
				  struct file_lock *conflock)
{
	return -ENOLCK;
}

static inline int posix_lock_file_wait(struct file *filp, struct file_lock *fl)
{
	return -ENOLCK;
}

static inline int posix_unblock_lock(struct file *filp,
				     struct file_lock *waiter)
{
	return -ENOENT;
}

static inline int vfs_test_lock(struct file *filp, struct file_lock *fl)
{
	return 0;
}

static inline int vfs_lock_file(struct file *filp, unsigned int cmd,
				struct file_lock *fl, struct file_lock *conf)
{
	return -ENOLCK;
}

static inline int vfs_cancel_lock(struct file *filp, struct file_lock *fl)
{
	return 0;
}

static inline int flock_lock_file_wait(struct file *filp,
				       struct file_lock *request)
{
	return -ENOLCK;
}

static inline int __break_lease(struct inode *inode, unsigned int mode)
{
	return 0;
}

static inline void lease_get_mtime(struct inode *inode, struct timespec *time)
{
	return;
}

static inline int generic_setlease(struct file *filp, long arg,
				    struct file_lock **flp)
{
	return -EINVAL;
}

static inline int vfs_setlease(struct file *filp, long arg,
			       struct file_lock **lease)
{
	return -EINVAL;
}

static inline int lease_modify(struct file_lock **before, int arg)
{
	return -EINVAL;
}

static inline int lock_may_read(struct inode *inode, loff_t start,
				unsigned long len)
{
	return 1;
}

static inline int lock_may_write(struct inode *inode, loff_t start,
				 unsigned long len)
{
	return 1;
}

#endif  

struct fasync_struct {
	int	magic;
	int	fa_fd;
	struct	fasync_struct	*fa_next;  
	struct	file 		*fa_file;
};

#define FASYNC_MAGIC 0x4601

extern int fasync_helper(int, struct file *, int, struct fasync_struct **);
 
extern void kill_fasync(struct fasync_struct **, int, int);
 
extern void __kill_fasync(struct fasync_struct *, int, int);

extern int __f_setown(struct file *filp, struct pid *, enum pid_type, int force);
extern int f_setown(struct file *filp, unsigned long arg, int force);
extern void f_delown(struct file *filp);
extern pid_t f_getown(struct file *filp);
extern int send_sigurg(struct fown_struct *fown);

#define MNT_FORCE	0x00000001	 
#define MNT_DETACH	0x00000002	 
#define MNT_EXPIRE	0x00000004	 

extern struct list_head super_blocks;
extern spinlock_t sb_lock;

#define sb_entry(list)  list_entry((list), struct super_block, s_list)
#define S_BIAS (1<<30)
struct super_block {
	struct list_head	s_list;		 
	dev_t			s_dev;		 
	unsigned long		s_blocksize;
	unsigned char		s_blocksize_bits;
	unsigned char		s_dirt;
	loff_t			s_maxbytes;	 
	struct file_system_type	*s_type;
	const struct super_operations	*s_op;
	const struct dquot_operations	*dq_op;
	const struct quotactl_ops	*s_qcop;
	const struct export_operations *s_export_op;
	unsigned long		s_flags;
	unsigned long		s_magic;
	struct dentry		*s_root;
	struct rw_semaphore	s_umount;
	struct mutex		s_lock;
	int			s_count;
	int			s_need_sync;
	atomic_t		s_active;
#ifdef CONFIG_SECURITY
	void                    *s_security;
#endif
	struct xattr_handler	**s_xattr;

	struct list_head	s_inodes;	 
	struct hlist_head	s_anon;		 
	struct list_head	s_files;
	 
	struct list_head	s_dentry_lru;	 
	int			s_nr_dentry_unused;	 

	struct block_device	*s_bdev;
	struct backing_dev_info *s_bdi;
	struct mtd_info		*s_mtd;
	struct list_head	s_instances;
	struct quota_info	s_dquot;	 

	int			s_frozen;
	wait_queue_head_t	s_wait_unfrozen;

	char s_id[32];				 

	void 			*s_fs_info;	 
	fmode_t			s_mode;

	struct mutex s_vfs_rename_mutex;	 

	u32		   s_time_gran;

	char *s_subtype;

	char *s_options;
#ifdef MY_ABC_HERE
	 
	struct mutex s_archive_mutex;   
	u32		s_archive_version;
#endif
#ifdef MY_ABC_HERE
	 
	long relatime_period;
#endif  
};

extern struct timespec current_fs_time(struct super_block *sb);

enum {
	SB_UNFROZEN = 0,
	SB_FREEZE_WRITE	= 1,
	SB_FREEZE_TRANS = 2,
};

#define vfs_check_frozen(sb, level) \
	wait_event((sb)->s_wait_unfrozen, ((sb)->s_frozen < (level)))

#define get_fs_excl() atomic_inc(&current->fs_excl)
#define put_fs_excl() atomic_dec(&current->fs_excl)
#define has_fs_excl() atomic_read(&current->fs_excl)

#define is_owner_or_cap(inode)	\
	((current_fsuid() == (inode)->i_uid) || capable(CAP_FOWNER))

extern void lock_super(struct super_block *);
extern void unlock_super(struct super_block *);

extern int vfs_create(struct inode *, struct dentry *, int, struct nameidata *);
extern int vfs_mkdir(struct inode *, struct dentry *, int);
extern int vfs_mknod(struct inode *, struct dentry *, int, dev_t);
extern int vfs_symlink(struct inode *, struct dentry *, const char *);
extern int vfs_link(struct dentry *, struct inode *, struct dentry *);
extern int vfs_rmdir(struct inode *, struct dentry *);
extern int vfs_unlink(struct inode *, struct dentry *);
extern int vfs_rename(struct inode *, struct dentry *, struct inode *, struct dentry *);

extern void dentry_unhash(struct dentry *dentry);

extern int file_permission(struct file *, int);

struct fiemap_extent_info {
	unsigned int fi_flags;		 
	unsigned int fi_extents_mapped;	 
	unsigned int fi_extents_max;	 
	struct fiemap_extent *fi_extents_start;  
};
int fiemap_fill_next_extent(struct fiemap_extent_info *info, u64 logical,
			    u64 phys, u64 len, u32 flags);
int fiemap_check_flags(struct fiemap_extent_info *fieinfo, u32 fs_flags);

#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

#ifdef CONFIG_SYNO_PLX_PORTING
#define OSYNC_METADATA (1<<0)
#define OSYNC_DATA (1<<1)
#define OSYNC_INODE (1<<2)
int generic_osync_inode(struct inode *, struct address_space *, int);
#endif

typedef int (*filldir_t)(void *, const char *, int, loff_t, u64, unsigned);
struct block_device_operations;

#define HAVE_COMPAT_IOCTL 1
#define HAVE_UNLOCKED_IOCTL 1

struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
#ifdef CONFIG_SYNO_PLX_PORTING
	ssize_t (*aio_direct_netrx_write) (struct kiocb *, void* callback, void *sock);
#endif
	int (*readdir) (struct file *, void *, filldir_t);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, struct dentry *, int datasync);
	int (*aio_fsync) (struct kiocb *, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
#ifdef CONFIG_SYNO_PLX_PORTING
	ssize_t (*sendfile) (struct file *, loff_t *, size_t, read_actor_t, void *);
	ssize_t (*incoherent_sendfile) (struct file *, loff_t *, size_t, read_actor_t, void *);
#endif
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
#ifdef CONFIG_SYNO_PLX_PORTING
	ssize_t (*sendpages) (struct file *, struct page **, int, size_t, loff_t *, int);
#endif
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long, struct file_lock **);
#ifdef CONFIG_SYNO_PLX_PORTING
	int (*preallocate)(struct file *, loff_t, loff_t);
	int (*unpreallocate)(struct file *, loff_t, loff_t);
	int (*resetpreallocate)(struct file *, loff_t, loff_t);
#endif
};

#ifdef CONFIG_SYNO_PLX_PORTING
struct getbmapx;
#endif

struct inode_operations {
#ifdef MY_ABC_HERE
	int (*syno_getattr)(struct dentry *, struct kstat *, int flags);
#endif  
	int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
	struct dentry * (*lookup) (struct inode *,struct dentry *, struct nameidata *);
#ifdef MY_ABC_HERE
	struct dentry * (*lookup_case) (struct inode *,struct dentry *, struct nameidata *);
#endif
	int (*link) (struct dentry *,struct inode *,struct dentry *);
	int (*unlink) (struct inode *,struct dentry *);
	int (*symlink) (struct inode *,struct dentry *,const char *);
	int (*mkdir) (struct inode *,struct dentry *,int);
	int (*rmdir) (struct inode *,struct dentry *);
	int (*mknod) (struct inode *,struct dentry *,int,dev_t);
	int (*rename) (struct inode *, struct dentry *,
			struct inode *, struct dentry *);
	int (*readlink) (struct dentry *, char __user *,int);
	void * (*follow_link) (struct dentry *, struct nameidata *);
	void (*put_link) (struct dentry *, struct nameidata *, void *);
	void (*truncate) (struct inode *);
	int (*permission) (struct inode *, int);
	int (*check_acl)(struct inode *, int);
#ifdef CONFIG_FS_SYNO_ACL
	struct syno_acl * (*syno_acl_get)(struct inode *);
	int (*syno_acl_set)(struct inode *, struct syno_acl *);
	int (*syno_acl_xattr_get)(struct dentry *, int, void *, size_t);
	int (*syno_permission)(struct dentry *, int);
	int (*syno_exec_permission)(struct dentry *);
	int (*syno_acl_access)(struct dentry *, int, int);
	int (*syno_inode_change_ok)(struct dentry *, struct iattr *);
	int (*syno_arbit_chg_ok)(struct dentry *, unsigned int cmd, int tag, int mask);
	int (*syno_setattr_post)(struct dentry *, struct iattr *);
	int (*syno_acl_init)(struct dentry *, struct inode *);
	void (*syno_acl_to_mode)(struct dentry *, struct kstat *);
	int (*syno_bypass_is_synoacl)(struct dentry *, int cmd, int reterr);
#endif  
#ifdef MY_ABC_HERE
	int (*syno_get_archive_bit)(struct dentry *, unsigned int *);
	int (*syno_set_archive_bit)(struct dentry *, unsigned int);
#endif
#ifdef MY_ABC_HERE
	int (*syno_set_crtime)(struct dentry *, struct timespec *);
#endif
#ifdef MY_ABC_HERE
	int (*syno_get_archive_ver)(struct dentry *, u32 *);
	int (*syno_set_archive_ver)(struct dentry *, u32);
#endif
	int (*setattr) (struct dentry *, struct iattr *);
	int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
	int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
	ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
	ssize_t (*listxattr) (struct dentry *, char *, size_t);
	int (*removexattr) (struct dentry *, const char *);
	void (*truncate_range)(struct inode *, loff_t, loff_t);
	long (*fallocate)(struct inode *inode, int mode, loff_t offset,
			  loff_t len);
	int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start,
		      u64 len);
#ifdef CONFIG_SYNO_PLX_PORTING
	int (*get_extents)(struct inode *inode, loff_t size);
	int (*getbmapx)(struct inode *inode, struct getbmapx *bmx);
	int (*setsize)(struct inode *, loff_t);
#endif
};

struct seq_file;

ssize_t rw_copy_check_uvector(int type, const struct iovec __user * uvector,
				unsigned long nr_segs, unsigned long fast_segs,
				struct iovec *fast_pointer,
				struct iovec **ret_pointer);

extern ssize_t vfs_read(struct file *, char __user *, size_t, loff_t *);
extern ssize_t vfs_write(struct file *, const char __user *, size_t, loff_t *);
extern ssize_t vfs_readv(struct file *, const struct iovec __user *,
		unsigned long, loff_t *);
extern ssize_t vfs_writev(struct file *, const struct iovec __user *,
		unsigned long, loff_t *);

struct super_operations {
#ifdef MY_ABC_HERE
	int (*syno_get_sb_archive_ver)(struct super_block *sb, u32 *version);
	int (*syno_set_sb_archive_ver)(struct super_block *sb, u32 version);
#endif
   	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);

   	void (*dirty_inode) (struct inode *);
	int (*write_inode) (struct inode *, int);
	void (*drop_inode) (struct inode *);
	void (*delete_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	void (*write_super) (struct super_block *);
	int (*sync_fs)(struct super_block *sb, int wait);
	int (*freeze_fs) (struct super_block *);
	int (*unfreeze_fs) (struct super_block *);
	int (*statfs) (struct dentry *, struct kstatfs *);
	int (*remount_fs) (struct super_block *, int *, char *);
	void (*clear_inode) (struct inode *);
	void (*umount_begin) (struct super_block *);

	int (*show_options)(struct seq_file *, struct vfsmount *);
	int (*show_stats)(struct seq_file *, struct vfsmount *);
#ifdef CONFIG_QUOTA
	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
#endif
	int (*bdev_try_to_free_page)(struct super_block*, struct page*, gfp_t);
};

#define I_DIRTY_SYNC		1
#define I_DIRTY_DATASYNC	2
#define I_DIRTY_PAGES		4
#define I_NEW			8
#define I_WILL_FREE		16
#define I_FREEING		32
#define I_CLEAR			64
#define __I_LOCK		7
#define I_LOCK			(1 << __I_LOCK)
#define __I_SYNC		8
#define I_SYNC			(1 << __I_SYNC)

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)

extern void __mark_inode_dirty(struct inode *, int);
static inline void mark_inode_dirty(struct inode *inode)
{
	__mark_inode_dirty(inode, I_DIRTY);
}

static inline void mark_inode_dirty_sync(struct inode *inode)
{
	__mark_inode_dirty(inode, I_DIRTY_SYNC);
}

static inline void inc_nlink(struct inode *inode)
{
	inode->i_nlink++;
}

static inline void inode_inc_link_count(struct inode *inode)
{
	inc_nlink(inode);
	mark_inode_dirty(inode);
}

static inline void drop_nlink(struct inode *inode)
{
	inode->i_nlink--;
}

static inline void clear_nlink(struct inode *inode)
{
	inode->i_nlink = 0;
}

static inline void inode_dec_link_count(struct inode *inode)
{
	drop_nlink(inode);
	mark_inode_dirty(inode);
}

static inline void inode_inc_iversion(struct inode *inode)
{
       spin_lock(&inode->i_lock);
       inode->i_version++;
       spin_unlock(&inode->i_lock);
}

extern void touch_atime(struct vfsmount *mnt, struct dentry *dentry);
static inline void file_accessed(struct file *file)
{
	if (!(file->f_flags & O_NOATIME))
		touch_atime(file->f_path.mnt, file->f_path.dentry);
}

int sync_inode(struct inode *inode, struct writeback_control *wbc);

struct file_system_type {
	const char *name;
	int fs_flags;
	int (*get_sb) (struct file_system_type *, int,
		       const char *, void *, struct vfsmount *);
	void (*kill_sb) (struct super_block *);
	struct module *owner;
	struct file_system_type * next;
	struct list_head fs_supers;

	struct lock_class_key s_lock_key;
	struct lock_class_key s_umount_key;

	struct lock_class_key i_lock_key;
	struct lock_class_key i_mutex_key;
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
	struct lock_class_key i_syno_mutex_key;
#endif
	struct lock_class_key i_mutex_dir_key;
	struct lock_class_key i_alloc_sem_key;
};

extern int get_sb_ns(struct file_system_type *fs_type, int flags, void *data,
	int (*fill_super)(struct super_block *, void *, int),
	struct vfsmount *mnt);
extern int get_sb_bdev(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data,
	int (*fill_super)(struct super_block *, void *, int),
	struct vfsmount *mnt);
extern int get_sb_single(struct file_system_type *fs_type,
	int flags, void *data,
	int (*fill_super)(struct super_block *, void *, int),
	struct vfsmount *mnt);
extern int get_sb_nodev(struct file_system_type *fs_type,
	int flags, void *data,
	int (*fill_super)(struct super_block *, void *, int),
	struct vfsmount *mnt);
void generic_shutdown_super(struct super_block *sb);
void kill_block_super(struct super_block *sb);
void kill_anon_super(struct super_block *sb);
void kill_litter_super(struct super_block *sb);
void deactivate_super(struct super_block *sb);
#ifdef MY_ABC_HERE
void deactivate_read_locked_super(struct super_block *s);
#endif
void deactivate_locked_super(struct super_block *sb);
int set_anon_super(struct super_block *s, void *data);
struct super_block *sget(struct file_system_type *type,
			int (*test)(struct super_block *,void *),
			int (*set)(struct super_block *,void *),
			void *data);
extern int get_sb_pseudo(struct file_system_type *, char *,
	const struct super_operations *ops, unsigned long,
	struct vfsmount *mnt);
extern void simple_set_mnt(struct vfsmount *mnt, struct super_block *sb);
int __put_super_and_need_restart(struct super_block *sb);
void put_super(struct super_block *sb);

#define fops_get(fops) \
	(((fops) && try_module_get((fops)->owner) ? (fops) : NULL))
#define fops_put(fops) \
	do { if (fops) module_put((fops)->owner); } while(0)

extern int register_filesystem(struct file_system_type *);
extern int unregister_filesystem(struct file_system_type *);
extern struct vfsmount *kern_mount_data(struct file_system_type *, void *data);
#define kern_mount(type) kern_mount_data(type, NULL)
extern int may_umount_tree(struct vfsmount *);
extern int may_umount(struct vfsmount *);
extern long do_mount(char *, char *, char *, unsigned long, void *);
extern struct vfsmount *collect_mounts(struct path *);
extern void drop_collected_mounts(struct vfsmount *);

extern int vfs_statfs(struct dentry *, struct kstatfs *);

extern int current_umask(void);

extern struct kobject *fs_kobj;

extern int rw_verify_area(int, struct file *, loff_t *, size_t);

#define FLOCK_VERIFY_READ  1
#define FLOCK_VERIFY_WRITE 2

#ifdef CONFIG_FILE_LOCKING
extern int locks_mandatory_locked(struct inode *);
extern int locks_mandatory_area(int, struct inode *, struct file *, loff_t, size_t);

static inline int __mandatory_lock(struct inode *ino)
{
	return (ino->i_mode & (S_ISGID | S_IXGRP)) == S_ISGID;
}

static inline int mandatory_lock(struct inode *ino)
{
	return IS_MANDLOCK(ino) && __mandatory_lock(ino);
}

static inline int locks_verify_locked(struct inode *inode)
{
	if (mandatory_lock(inode))
		return locks_mandatory_locked(inode);
	return 0;
}

static inline int locks_verify_truncate(struct inode *inode,
				    struct file *filp,
				    loff_t size)
{
	if (inode->i_flock && mandatory_lock(inode))
		return locks_mandatory_area(
			FLOCK_VERIFY_WRITE, inode, filp,
			size < inode->i_size ? size : inode->i_size,
			(size < inode->i_size ? inode->i_size - size
			 : size - inode->i_size)
		);
	return 0;
}

static inline int break_lease(struct inode *inode, unsigned int mode)
{
	if (inode->i_flock)
		return __break_lease(inode, mode);
	return 0;
}
#else  
static inline int locks_mandatory_locked(struct inode *inode)
{
	return 0;
}

static inline int locks_mandatory_area(int rw, struct inode *inode,
				       struct file *filp, loff_t offset,
				       size_t count)
{
	return 0;
}

static inline int __mandatory_lock(struct inode *inode)
{
	return 0;
}

static inline int mandatory_lock(struct inode *inode)
{
	return 0;
}

static inline int locks_verify_locked(struct inode *inode)
{
	return 0;
}

static inline int locks_verify_truncate(struct inode *inode, struct file *filp,
					size_t size)
{
	return 0;
}

static inline int break_lease(struct inode *inode, unsigned int mode)
{
	return 0;
}

#endif  

extern int do_truncate(struct dentry *, loff_t start, unsigned int time_attrs,
		       struct file *filp);
extern int do_fallocate(struct file *file, int mode, loff_t offset,
			loff_t len);
extern long do_sys_open(int dfd, const char __user *filename, int flags,
			int mode);
extern struct file *filp_open(const char *, int, int);
extern struct file * dentry_open(struct dentry *, struct vfsmount *, int,
				 const struct cred *);
extern int filp_close(struct file *, fl_owner_t id);
extern char * getname(const char __user *);

extern int ioctl_preallocate(struct file *filp, void __user *argp);

extern void __init vfs_caches_init_early(void);
extern void __init vfs_caches_init(unsigned long);

extern struct kmem_cache *names_cachep;

#define __getname_gfp(gfp)	kmem_cache_alloc(names_cachep, (gfp))
#define __getname()		__getname_gfp(GFP_KERNEL)
#define __putname(name)		kmem_cache_free(names_cachep, (void *)(name))
#ifndef CONFIG_AUDITSYSCALL
#define putname(name)   __putname(name)
#else
extern void putname(const char *name);
#endif

#ifdef CONFIG_BLOCK
extern int register_blkdev(unsigned int, const char *);
extern void unregister_blkdev(unsigned int, const char *);
extern struct block_device *bdget(dev_t);
extern struct block_device *bdgrab(struct block_device *bdev);
extern void bd_set_size(struct block_device *, loff_t size);
extern void bd_forget(struct inode *inode);
extern void bdput(struct block_device *);
extern struct block_device *open_by_devnum(dev_t, fmode_t);
extern void invalidate_bdev(struct block_device *);
extern int sync_blockdev(struct block_device *bdev);
extern struct super_block *freeze_bdev(struct block_device *);
extern void emergency_thaw_all(void);
extern int thaw_bdev(struct block_device *bdev, struct super_block *sb);
extern int fsync_bdev(struct block_device *);
#else
static inline void bd_forget(struct inode *inode) {}
static inline int sync_blockdev(struct block_device *bdev) { return 0; }
static inline void invalidate_bdev(struct block_device *bdev) {}

static inline struct super_block *freeze_bdev(struct block_device *sb)
{
	return NULL;
}

static inline int thaw_bdev(struct block_device *bdev, struct super_block *sb)
{
	return 0;
}
#endif
extern int sync_filesystem(struct super_block *);
extern const struct file_operations def_blk_fops;
extern const struct file_operations def_chr_fops;
extern const struct file_operations bad_sock_fops;
extern const struct file_operations def_fifo_fops;
#ifdef CONFIG_BLOCK
extern int ioctl_by_bdev(struct block_device *, unsigned, unsigned long);
extern int blkdev_ioctl(struct block_device *, fmode_t, unsigned, unsigned long);
extern long compat_blkdev_ioctl(struct file *, unsigned, unsigned long);
extern int blkdev_get(struct block_device *, fmode_t);
extern int blkdev_put(struct block_device *, fmode_t);
extern int bd_claim(struct block_device *, void *);
extern void bd_release(struct block_device *);
#ifdef CONFIG_SYSFS
extern int bd_claim_by_disk(struct block_device *, void *, struct gendisk *);
extern void bd_release_from_disk(struct block_device *, struct gendisk *);
#else
#define bd_claim_by_disk(bdev, holder, disk)	bd_claim(bdev, holder)
#define bd_release_from_disk(bdev, disk)	bd_release(bdev)
#endif
#endif

#define CHRDEV_MAJOR_HASH_SIZE	255
extern int alloc_chrdev_region(dev_t *, unsigned, unsigned, const char *);
extern int register_chrdev_region(dev_t, unsigned, const char *);
extern int __register_chrdev(unsigned int major, unsigned int baseminor,
			     unsigned int count, const char *name,
			     const struct file_operations *fops);
extern void __unregister_chrdev(unsigned int major, unsigned int baseminor,
				unsigned int count, const char *name);
extern void unregister_chrdev_region(dev_t, unsigned);
extern void chrdev_show(struct seq_file *,off_t);

static inline int register_chrdev(unsigned int major, const char *name,
				  const struct file_operations *fops)
{
	return __register_chrdev(major, 0, 256, name, fops);
}

static inline void unregister_chrdev(unsigned int major, const char *name)
{
	__unregister_chrdev(major, 0, 256, name);
}

#define BDEVNAME_SIZE	32	 
#define BDEVT_SIZE	10	 

#ifdef CONFIG_BLOCK
#define BLKDEV_MAJOR_HASH_SIZE	255
extern const char *__bdevname(dev_t, char *buffer);
extern const char *bdevname(struct block_device *bdev, char *buffer);
extern struct block_device *lookup_bdev(const char *);
extern struct block_device *open_bdev_exclusive(const char *, fmode_t, void *);
extern void close_bdev_exclusive(struct block_device *, fmode_t);
extern void blkdev_show(struct seq_file *,off_t);

#else
#define BLKDEV_MAJOR_HASH_SIZE	0
#endif

extern void init_special_inode(struct inode *, umode_t, dev_t);

extern void make_bad_inode(struct inode *);
extern int is_bad_inode(struct inode *);

extern const struct file_operations read_pipefifo_fops;
extern const struct file_operations write_pipefifo_fops;
extern const struct file_operations rdwr_pipefifo_fops;

extern int fs_may_remount_ro(struct super_block *);

#ifdef CONFIG_BLOCK
 
#define bio_rw(bio)		((bio)->bi_rw & (RW_MASK | RWA_MASK))

#define bio_data_dir(bio)	((bio)->bi_rw & 1)

extern void check_disk_size_change(struct gendisk *disk,
				   struct block_device *bdev);
extern int revalidate_disk(struct gendisk *);
extern int check_disk_change(struct block_device *);
extern int __invalidate_device(struct block_device *);
extern int invalidate_partition(struct gendisk *, int);
#endif
extern int invalidate_inodes(struct super_block *);
unsigned long invalidate_mapping_pages(struct address_space *mapping,
					pgoff_t start, pgoff_t end);

static inline unsigned long __deprecated
invalidate_inode_pages(struct address_space *mapping)
{
	return invalidate_mapping_pages(mapping, 0, ~0UL);
}

static inline void invalidate_remote_inode(struct inode *inode)
{
	if (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
	    S_ISLNK(inode->i_mode))
		invalidate_mapping_pages(inode->i_mapping, 0, -1);
}
extern int invalidate_inode_pages2(struct address_space *mapping);
extern int invalidate_inode_pages2_range(struct address_space *mapping,
					 pgoff_t start, pgoff_t end);
extern int write_inode_now(struct inode *, int);
extern int filemap_fdatawrite(struct address_space *);
extern int filemap_flush(struct address_space *);
extern int filemap_fdatawait(struct address_space *);
extern int filemap_fdatawait_range(struct address_space *, loff_t lstart,
				   loff_t lend);
extern int filemap_write_and_wait(struct address_space *mapping);
extern int filemap_write_and_wait_range(struct address_space *mapping,
				        loff_t lstart, loff_t lend);
extern int wait_on_page_writeback_range(struct address_space *mapping,
				pgoff_t start, pgoff_t end);
extern int __filemap_fdatawrite_range(struct address_space *mapping,
				loff_t start, loff_t end, int sync_mode);
extern int filemap_fdatawrite_range(struct address_space *mapping,
				loff_t start, loff_t end);

extern int vfs_fsync_range(struct file *file, struct dentry *dentry,
			   loff_t start, loff_t end, int datasync);
extern int vfs_fsync(struct file *file, struct dentry *dentry, int datasync);
extern int generic_write_sync(struct file *file, loff_t pos, loff_t count);
extern void sync_supers(void);
extern void emergency_sync(void);
extern void emergency_remount(void);
#ifdef CONFIG_BLOCK
extern sector_t bmap(struct inode *, sector_t);
#endif
extern int notify_change(struct dentry *, struct iattr *);
extern int inode_permission(struct inode *, int);
extern int generic_permission(struct inode *, int,
		int (*check_acl)(struct inode *, int));

static inline bool execute_ok(struct inode *inode)
{
	return (inode->i_mode & S_IXUGO) || S_ISDIR(inode->i_mode);
}

extern int get_write_access(struct inode *);
extern int deny_write_access(struct file *);
static inline void put_write_access(struct inode * inode)
{
	atomic_dec(&inode->i_writecount);
}
static inline void allow_write_access(struct file *file)
{
	if (file)
		atomic_inc(&file->f_path.dentry->d_inode->i_writecount);
}
extern int do_pipe_flags(int *, int);
extern struct file *create_read_pipe(struct file *f, int flags);
extern struct file *create_write_pipe(int flags);
extern void free_write_pipe(struct file *);

extern struct file *do_filp_open(int dfd, const char *pathname,
		int open_flag, int mode, int acc_mode);
extern int may_open(struct path *, int, int);

extern int kernel_read(struct file *, loff_t, char *, unsigned long);
extern struct file * open_exec(const char *);
 
extern int is_subdir(struct dentry *, struct dentry *);
extern ino_t find_inode_number(struct dentry *, struct qstr *);

#include <linux/err.h>

extern loff_t default_llseek(struct file *file, loff_t offset, int origin);

extern loff_t vfs_llseek(struct file *file, loff_t offset, int origin);

extern int inode_init_always(struct super_block *, struct inode *);
extern void inode_init_once(struct inode *);
extern void inode_add_to_lists(struct super_block *, struct inode *);
extern void iput(struct inode *);
extern struct inode * igrab(struct inode *);
extern ino_t iunique(struct super_block *, ino_t);
extern int inode_needs_sync(struct inode *inode);
extern void generic_delete_inode(struct inode *inode);
extern void generic_drop_inode(struct inode *inode);
extern int generic_detach_inode(struct inode *inode);

extern struct inode *ilookup5_nowait(struct super_block *sb,
		unsigned long hashval, int (*test)(struct inode *, void *),
		void *data);
extern struct inode *ilookup5(struct super_block *sb, unsigned long hashval,
		int (*test)(struct inode *, void *), void *data);
extern struct inode *ilookup(struct super_block *sb, unsigned long ino);

extern struct inode * iget5_locked(struct super_block *, unsigned long, int (*test)(struct inode *, void *), int (*set)(struct inode *, void *), void *);
extern struct inode * iget_locked(struct super_block *, unsigned long);
extern int insert_inode_locked4(struct inode *, unsigned long, int (*test)(struct inode *, void *), void *);
extern int insert_inode_locked(struct inode *);
extern void unlock_new_inode(struct inode *);

extern void __iget(struct inode * inode);
extern void iget_failed(struct inode *);
extern void clear_inode(struct inode *);
extern void destroy_inode(struct inode *);
extern void __destroy_inode(struct inode *);
extern struct inode *new_inode(struct super_block *);
extern int should_remove_suid(struct dentry *);
extern int file_remove_suid(struct file *);

extern void __insert_inode_hash(struct inode *, unsigned long hashval);
extern void remove_inode_hash(struct inode *);
static inline void insert_inode_hash(struct inode *inode) {
	__insert_inode_hash(inode, inode->i_ino);
}

extern struct file * get_empty_filp(void);
extern void file_move(struct file *f, struct list_head *list);
extern void file_kill(struct file *f);
#ifdef CONFIG_BLOCK
struct bio;
extern void submit_bio(int, struct bio *);
extern int bdev_read_only(struct block_device *);
#endif
extern int set_blocksize(struct block_device *, int);
extern int sb_set_blocksize(struct super_block *, int);
extern int sb_min_blocksize(struct super_block *, int);

extern int generic_file_mmap(struct file *, struct vm_area_struct *);
extern int generic_file_readonly_mmap(struct file *, struct vm_area_struct *);
#ifdef CONFIG_SYNO_PLX_PORTING
extern int file_read_actor(read_descriptor_t * desc, struct page **page, unsigned long offset, unsigned long size);
extern int file_send_actor(read_descriptor_t * desc, struct page **page, unsigned long offset, unsigned long size);
#else
extern int file_read_actor(read_descriptor_t * desc, struct page *page, unsigned long offset, unsigned long size);
#endif
int generic_write_checks(struct file *file, loff_t *pos, size_t *count, int isblk);
#ifdef MY_ABC_HERE
#define MAX_PAGES_PER_RECVFILE 16
extern int do_recvfile(struct file *file, struct socket *sock, loff_t *ppos, size_t count, size_t *rbytes , size_t *wbytes);
#endif
extern ssize_t generic_file_aio_read(struct kiocb *, const struct iovec *, unsigned long, loff_t);
extern ssize_t __generic_file_aio_write(struct kiocb *, const struct iovec *, unsigned long,
		loff_t *);
extern ssize_t generic_file_aio_write(struct kiocb *, const struct iovec *, unsigned long, loff_t);
extern ssize_t generic_file_direct_write(struct kiocb *, const struct iovec *,
		unsigned long *, loff_t, loff_t *, size_t, size_t);
extern ssize_t generic_file_buffered_write(struct kiocb *, const struct iovec *,
		unsigned long, loff_t, loff_t *, size_t, ssize_t);
extern ssize_t do_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);
extern ssize_t do_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos);
#ifdef CONFIG_SYNO_PLX_PORTING
extern ssize_t generic_file_sendfile(struct file *, loff_t *, size_t, read_actor_t, void *);
extern ssize_t generic_file_incoherent_sendfile(struct file *, loff_t *, size_t, read_actor_t, void *);
#endif
extern int generic_segment_checks(const struct iovec *iov,
		unsigned long *nr_segs, size_t *count, int access_flags);

extern ssize_t blkdev_aio_write(struct kiocb *iocb, const struct iovec *iov,
				unsigned long nr_segs, loff_t pos);
extern int block_fsync(struct file *filp, struct dentry *dentry, int datasync);

extern ssize_t generic_file_splice_read(struct file *, loff_t *,
		struct pipe_inode_info *, size_t, unsigned int);
extern ssize_t default_file_splice_read(struct file *, loff_t *,
		struct pipe_inode_info *, size_t, unsigned int);
extern ssize_t generic_file_splice_write(struct pipe_inode_info *,
		struct file *, loff_t *, size_t, unsigned int);
extern ssize_t generic_splice_sendpage(struct pipe_inode_info *pipe,
		struct file *out, loff_t *, size_t len, unsigned int flags);
extern long do_splice_direct(struct file *in, loff_t *ppos, struct file *out,
		size_t len, unsigned int flags);

extern void
file_ra_state_init(struct file_ra_state *ra, struct address_space *mapping);
extern loff_t no_llseek(struct file *file, loff_t offset, int origin);
extern loff_t generic_file_llseek(struct file *file, loff_t offset, int origin);
extern loff_t generic_file_llseek_unlocked(struct file *file, loff_t offset,
			int origin);
extern int generic_file_open(struct inode * inode, struct file * filp);
extern int nonseekable_open(struct inode * inode, struct file * filp);

#ifdef CONFIG_FS_XIP
extern ssize_t xip_file_read(struct file *filp, char __user *buf, size_t len,
			     loff_t *ppos);
extern int xip_file_mmap(struct file * file, struct vm_area_struct * vma);
extern ssize_t xip_file_write(struct file *filp, const char __user *buf,
			      size_t len, loff_t *ppos);
extern int xip_truncate_page(struct address_space *mapping, loff_t from);
#else
static inline int xip_truncate_page(struct address_space *mapping, loff_t from)
{
	return 0;
}
#endif

#ifdef CONFIG_BLOCK
ssize_t __blockdev_direct_IO(int rw, struct kiocb *iocb, struct inode *inode,
	struct block_device *bdev, const struct iovec *iov, loff_t offset,
	unsigned long nr_segs, get_block_t get_block, dio_iodone_t end_io,
	int lock_type);

enum {
	DIO_LOCKING = 1,  
	DIO_NO_LOCKING,   
	DIO_OWN_LOCKING,  
};

static inline ssize_t blockdev_direct_IO(int rw, struct kiocb *iocb,
	struct inode *inode, struct block_device *bdev, const struct iovec *iov,
	loff_t offset, unsigned long nr_segs, get_block_t get_block,
	dio_iodone_t end_io)
{
	return __blockdev_direct_IO(rw, iocb, inode, bdev, iov, offset,
				nr_segs, get_block, end_io, DIO_LOCKING);
}

static inline ssize_t blockdev_direct_IO_no_locking(int rw, struct kiocb *iocb,
	struct inode *inode, struct block_device *bdev, const struct iovec *iov,
	loff_t offset, unsigned long nr_segs, get_block_t get_block,
	dio_iodone_t end_io)
{
	return __blockdev_direct_IO(rw, iocb, inode, bdev, iov, offset,
				nr_segs, get_block, end_io, DIO_NO_LOCKING);
}

static inline ssize_t blockdev_direct_IO_own_locking(int rw, struct kiocb *iocb,
	struct inode *inode, struct block_device *bdev, const struct iovec *iov,
	loff_t offset, unsigned long nr_segs, get_block_t get_block,
	dio_iodone_t end_io)
{
	return __blockdev_direct_IO(rw, iocb, inode, bdev, iov, offset,
				nr_segs, get_block, end_io, DIO_OWN_LOCKING);
}
#endif

extern const struct file_operations generic_ro_fops;

#define special_file(m) (S_ISCHR(m)||S_ISBLK(m)||S_ISFIFO(m)||S_ISSOCK(m))

extern int vfs_readlink(struct dentry *, char __user *, int, const char *);
extern int vfs_follow_link(struct nameidata *, const char *);
extern int page_readlink(struct dentry *, char __user *, int);
extern void *page_follow_link_light(struct dentry *, struct nameidata *);
extern void page_put_link(struct dentry *, struct nameidata *, void *);
extern int __page_symlink(struct inode *inode, const char *symname, int len,
		int nofs);
extern int page_symlink(struct inode *inode, const char *symname, int len);
extern const struct inode_operations page_symlink_inode_operations;
extern int generic_readlink(struct dentry *, char __user *, int);
extern void generic_fillattr(struct inode *, struct kstat *);
extern int vfs_getattr(struct vfsmount *, struct dentry *, struct kstat *);
void __inode_add_bytes(struct inode *inode, loff_t bytes);
void inode_add_bytes(struct inode *inode, loff_t bytes);
void inode_sub_bytes(struct inode *inode, loff_t bytes);
loff_t inode_get_bytes(struct inode *inode);
void inode_set_bytes(struct inode *inode, loff_t bytes);

extern int vfs_readdir(struct file *, filldir_t, void *);

extern int vfs_stat(char __user *, struct kstat *);
extern int vfs_lstat(char __user *, struct kstat *);
extern int vfs_fstat(unsigned int, struct kstat *);
extern int vfs_fstatat(int , char __user *, struct kstat *, int);

extern int do_vfs_ioctl(struct file *filp, unsigned int fd, unsigned int cmd,
		    unsigned long arg);
extern int __generic_block_fiemap(struct inode *inode,
				  struct fiemap_extent_info *fieinfo, u64 start,
				  u64 len, get_block_t *get_block);
extern int generic_block_fiemap(struct inode *inode,
				struct fiemap_extent_info *fieinfo, u64 start,
				u64 len, get_block_t *get_block);

extern void get_filesystem(struct file_system_type *fs);
extern void put_filesystem(struct file_system_type *fs);
extern struct file_system_type *get_fs_type(const char *name);
extern struct super_block *get_super(struct block_device *);
extern struct super_block *get_active_super(struct block_device *bdev);
extern struct super_block *user_get_super(dev_t);
extern void drop_super(struct super_block *sb);

extern int dcache_dir_open(struct inode *, struct file *);
extern int dcache_dir_close(struct inode *, struct file *);
extern loff_t dcache_dir_lseek(struct file *, loff_t, int);
extern int dcache_readdir(struct file *, void *, filldir_t);
extern int simple_getattr(struct vfsmount *, struct dentry *, struct kstat *);
extern int simple_statfs(struct dentry *, struct kstatfs *);
extern int simple_link(struct dentry *, struct inode *, struct dentry *);
extern int simple_unlink(struct inode *, struct dentry *);
extern int simple_rmdir(struct inode *, struct dentry *);
extern int simple_rename(struct inode *, struct dentry *, struct inode *, struct dentry *);
extern int simple_sync_file(struct file *, struct dentry *, int);
extern int simple_empty(struct dentry *);
extern int simple_readpage(struct file *file, struct page *page);
extern int simple_prepare_write(struct file *file, struct page *page,
			unsigned offset, unsigned to);
extern int simple_write_begin(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata);
extern int simple_write_end(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata);

extern struct dentry *simple_lookup(struct inode *, struct dentry *, struct nameidata *);
extern ssize_t generic_read_dir(struct file *, char __user *, size_t, loff_t *);
extern const struct file_operations simple_dir_operations;
extern const struct inode_operations simple_dir_inode_operations;
struct tree_descr { char *name; const struct file_operations *ops; int mode; };
struct dentry *d_alloc_name(struct dentry *, const char *);
extern int simple_fill_super(struct super_block *, int, struct tree_descr *);
extern int simple_pin_fs(struct file_system_type *, struct vfsmount **mount, int *count);
extern void simple_release_fs(struct vfsmount **mount, int *count);

extern ssize_t simple_read_from_buffer(void __user *to, size_t count,
			loff_t *ppos, const void *from, size_t available);

extern int simple_fsync(struct file *, struct dentry *, int);

#ifdef CONFIG_MIGRATION
extern int buffer_migrate_page(struct address_space *,
				struct page *, struct page *);
#else
#define buffer_migrate_page NULL
#endif

extern int inode_change_ok(const struct inode *, struct iattr *);
extern int inode_newsize_ok(const struct inode *, loff_t offset);
extern int __must_check inode_setattr(struct inode *, struct iattr *);

extern void file_update_time(struct file *file);

extern int generic_show_options(struct seq_file *m, struct vfsmount *mnt);
extern void save_mount_options(struct super_block *sb, char *options);
extern void replace_mount_options(struct super_block *sb, char *options);

static inline ino_t parent_ino(struct dentry *dentry)
{
	ino_t res;

	spin_lock(&dentry->d_lock);
	res = dentry->d_parent->d_inode->i_ino;
	spin_unlock(&dentry->d_lock);
	return res;
}

struct simple_transaction_argresp {
	ssize_t size;
	char data[0];
};

#define SIMPLE_TRANSACTION_LIMIT (PAGE_SIZE - sizeof(struct simple_transaction_argresp))

char *simple_transaction_get(struct file *file, const char __user *buf,
				size_t size);
ssize_t simple_transaction_read(struct file *file, char __user *buf,
				size_t size, loff_t *pos);
int simple_transaction_release(struct inode *inode, struct file *file);

void simple_transaction_set(struct file *file, size_t n);

#define DEFINE_SIMPLE_ATTRIBUTE(__fops, __get, __set, __fmt)		\
static int __fops ## _open(struct inode *inode, struct file *file)	\
{									\
	__simple_attr_check_format(__fmt, 0ull);			\
	return simple_attr_open(inode, file, __get, __set, __fmt);	\
}									\
static const struct file_operations __fops = {				\
	.owner	 = THIS_MODULE,						\
	.open	 = __fops ## _open,					\
	.release = simple_attr_release,					\
	.read	 = simple_attr_read,					\
	.write	 = simple_attr_write,					\
};

static inline void __attribute__((format(printf, 1, 2)))
__simple_attr_check_format(const char *fmt, ...)
{
	 
}

int simple_attr_open(struct inode *inode, struct file *file,
		     int (*get)(void *, u64 *), int (*set)(void *, u64),
		     const char *fmt);
int simple_attr_release(struct inode *inode, struct file *file);
ssize_t simple_attr_read(struct file *file, char __user *buf,
			 size_t len, loff_t *ppos);
ssize_t simple_attr_write(struct file *file, const char __user *buf,
			  size_t len, loff_t *ppos);

struct ctl_table;
int proc_nr_files(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos);

int __init get_filesystem_list(char *buf);

#ifdef MY_ABC_HERE
#define UTF16_UPCASE_TABLE_SIZE 	0x10000		 
#define UNICODE_UTF16_BUFSIZE		4096		 
#define UNICODE_UTF8_BUFSIZE		8192

int SYNOUnicodeUTF8Strcmp(const u_int8_t *utf8str1,const u_int8_t *utf8str2,int clenUtf8Str1, int clenUtf8Str2, u_int16_t *upcasetable);
int SYNOUnicodeUTF8toUpper(u_int8_t *to,const u_int8_t *from, int maxlen, int clenfrom, u_int16_t *upcasetable);
#endif  

#ifdef MY_ABC_HERE
static inline int syno_op_set_crtime(struct dentry *dentry, struct timespec *time)
{
	int error = 0;
	struct inode *inode = dentry->d_inode;

	mutex_lock(&inode->i_mutex);

	if (inode->i_op->syno_set_crtime) {
		error = inode->i_op->syno_set_crtime(dentry, time);
	} else {
		inode->i_create_time = timespec_trunc(*time, inode->i_sb->s_time_gran);
		inode->i_ctime = CURRENT_TIME;
		mark_inode_dirty(inode);
	}

	mutex_unlock(&inode->i_mutex);
	return error;
}
#endif

#ifdef MY_ABC_HERE
static inline int syno_op_get_archive_bit(struct dentry *dentry, unsigned int *pArbit)
{
	int err = 0;
	struct inode *inode = dentry->d_inode;

	if (inode->i_op->syno_get_archive_bit) {
		err = inode->i_op->syno_get_archive_bit(dentry, pArbit);
		if (-ENODATA == err) {
			err = 0;
			*pArbit = 0;
		}
	} else {
		*pArbit = inode->i_archive_bit;
	}

	return err;
}

static inline int syno_op_set_archive_bit_nolock(struct dentry *dentry, unsigned int arbit)
{
	int err = 0;
	struct inode *inode = dentry->d_inode;

	if (inode->i_op->syno_set_archive_bit) {
		err = inode->i_op->syno_set_archive_bit(dentry, arbit);
	} else {
		inode->i_archive_bit = arbit;
		inode->i_ctime = CURRENT_TIME;
		mark_inode_dirty_sync(inode);
	}

	return err;
}

static inline int syno_op_set_archive_bit(struct dentry *dentry, unsigned int arbit)
{
	int err = 0;
	struct inode *inode = dentry->d_inode;

	mutex_lock(&inode->i_syno_mutex);
	err = syno_op_set_archive_bit_nolock(dentry, arbit);
	mutex_unlock(&inode->i_syno_mutex);
	return err;
}

#endif  

#ifdef MY_ABC_HERE
#define SYNO_EXT4_MOUNT_PATH_LEN 128
#endif

#endif  
#endif  
