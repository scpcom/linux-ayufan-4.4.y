 
#ifndef _LINUX_INOTIFY_H
#define _LINUX_INOTIFY_H

#if !defined(SYNO_PPC_853X) && !defined(SYNO_PPC_854X) && !defined(SYNOPLAT_F_I686)
 
#include <linux/fcntl.h>
#endif
#include <linux/types.h>

struct inotify_event {
	__s32		wd;		 
	__u32		mask;		 
	__u32		cookie;		 
	__u32		len;		 
	char		name[0];	 
};

#define IN_ACCESS		0x00000001	 
#define IN_MODIFY		0x00000002	 
#define IN_ATTRIB		0x00000004	 
#define IN_CLOSE_WRITE		0x00000008	 
#define IN_CLOSE_NOWRITE	0x00000010	 
#define IN_OPEN			0x00000020	 
#define IN_MOVED_FROM		0x00000040	 
#define IN_MOVED_TO		0x00000080	 
#define IN_CREATE		0x00000100	 
#define IN_DELETE		0x00000200	 
#define IN_DELETE_SELF		0x00000400	 
#define IN_MOVE_SELF		0x00000800	 

#define IN_UNMOUNT		0x00002000	 
#define IN_Q_OVERFLOW		0x00004000	 
#define IN_IGNORED		0x00008000	 

#define IN_CLOSE		(IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)  
#define IN_MOVE			(IN_MOVED_FROM | IN_MOVED_TO)  

#define IN_ONLYDIR		0x01000000	 
#define IN_DONT_FOLLOW		0x02000000	 
#define IN_MASK_ADD		0x20000000	 
#define IN_ISDIR		0x40000000	 
#define IN_ONESHOT		0x80000000	 

#define IN_ALL_EVENTS	(IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | \
			 IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | \
			 IN_MOVED_TO | IN_DELETE | IN_CREATE | IN_DELETE_SELF | \
			 IN_MOVE_SELF)

#define IN_CLOEXEC O_CLOEXEC
#define IN_NONBLOCK O_NONBLOCK

#ifdef __KERNEL__

#include <linux/dcache.h>
#include <linux/fs.h>

struct inotify_watch {
	struct list_head	h_list;	 
	struct list_head	i_list;	 
	atomic_t		count;	 
	struct inotify_handle	*ih;	 
	struct inode		*inode;	 
	__s32			wd;	 
	__u32			mask;	 
};

struct inotify_operations {
	void (*handle_event)(struct inotify_watch *, u32, u32, u32,
			     const char *, struct inode *);
	void (*destroy_watch)(struct inotify_watch *);
};

#ifdef CONFIG_INOTIFY

extern void inotify_d_instantiate(struct dentry *, struct inode *);
extern void inotify_d_move(struct dentry *);
extern void inotify_inode_queue_event(struct inode *, __u32, __u32,
				      const char *, struct inode *);
extern void inotify_dentry_parent_queue_event(struct dentry *, __u32, __u32,
					      const char *);
extern void inotify_unmount_inodes(struct list_head *);
extern void inotify_inode_is_dead(struct inode *);
extern u32 inotify_get_cookie(void);

extern struct inotify_handle *inotify_init(const struct inotify_operations *);
extern void inotify_init_watch(struct inotify_watch *);
extern void inotify_destroy(struct inotify_handle *);
extern __s32 inotify_find_watch(struct inotify_handle *, struct inode *,
				struct inotify_watch **);
extern __s32 inotify_find_update_watch(struct inotify_handle *, struct inode *,
				       u32);
extern __s32 inotify_add_watch(struct inotify_handle *, struct inotify_watch *,
			       struct inode *, __u32);
extern __s32 inotify_clone_watch(struct inotify_watch *, struct inotify_watch *);
extern void inotify_evict_watch(struct inotify_watch *);
extern int inotify_rm_watch(struct inotify_handle *, struct inotify_watch *);
extern int inotify_rm_wd(struct inotify_handle *, __u32);
extern void inotify_remove_watch_locked(struct inotify_handle *,
					struct inotify_watch *);
extern void get_inotify_watch(struct inotify_watch *);
extern void put_inotify_watch(struct inotify_watch *);
extern int pin_inotify_watch(struct inotify_watch *);
extern void unpin_inotify_watch(struct inotify_watch *);

#else

static inline void inotify_d_instantiate(struct dentry *dentry,
					struct inode *inode)
{
}

static inline void inotify_d_move(struct dentry *dentry)
{
}

static inline void inotify_inode_queue_event(struct inode *inode,
					     __u32 mask, __u32 cookie,
					     const char *filename,
					     struct inode *n_inode)
{
}

static inline void inotify_dentry_parent_queue_event(struct dentry *dentry,
						     __u32 mask, __u32 cookie,
						     const char *filename)
{
}

static inline void inotify_unmount_inodes(struct list_head *list)
{
}

static inline void inotify_inode_is_dead(struct inode *inode)
{
}

static inline u32 inotify_get_cookie(void)
{
	return 0;
}

static inline struct inotify_handle *inotify_init(const struct inotify_operations *ops)
{
	return ERR_PTR(-EOPNOTSUPP);
}

static inline void inotify_init_watch(struct inotify_watch *watch)
{
}

static inline void inotify_destroy(struct inotify_handle *ih)
{
}

static inline __s32 inotify_find_watch(struct inotify_handle *ih, struct inode *inode,
				       struct inotify_watch **watchp)
{
	return -EOPNOTSUPP;
}

static inline __s32 inotify_find_update_watch(struct inotify_handle *ih,
					      struct inode *inode, u32 mask)
{
	return -EOPNOTSUPP;
}

static inline __s32 inotify_add_watch(struct inotify_handle *ih,
				      struct inotify_watch *watch,
				      struct inode *inode, __u32 mask)
{
	return -EOPNOTSUPP;
}

static inline int inotify_rm_watch(struct inotify_handle *ih,
				   struct inotify_watch *watch)
{
	return -EOPNOTSUPP;
}

static inline int inotify_rm_wd(struct inotify_handle *ih, __u32 wd)
{
	return -EOPNOTSUPP;
}

static inline void inotify_remove_watch_locked(struct inotify_handle *ih,
					       struct inotify_watch *watch)
{
}

static inline void get_inotify_watch(struct inotify_watch *watch)
{
}

static inline void put_inotify_watch(struct inotify_watch *watch)
{
}

extern inline int pin_inotify_watch(struct inotify_watch *watch)
{
	return 0;
}

extern inline void unpin_inotify_watch(struct inotify_watch *watch)
{
}

#endif	 

#endif	 

#endif	 
