#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef __LINUX_DCACHE_H
#define __LINUX_DCACHE_H

#include <asm/atomic.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/spinlock.h>
#include <linux/cache.h>
#include <linux/rcupdate.h>

struct nameidata;
struct path;
struct vfsmount;

#define IS_ROOT(x) ((x) == (x)->d_parent)

struct qstr {
	unsigned int hash;
	unsigned int len;
	const unsigned char *name;
};

struct dentry_stat_t {
	int nr_dentry;
	int nr_unused;
	int age_limit;           
	int want_pages;          
	int dummy[2];
};
extern struct dentry_stat_t dentry_stat;

#define init_name_hash()		0

static inline unsigned long
partial_name_hash(unsigned long c, unsigned long prevhash)
{
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

static inline unsigned long end_name_hash(unsigned long hash)
{
	return (unsigned int) hash;
}

static inline unsigned int
full_name_hash(const unsigned char *name, unsigned int len)
{
	unsigned long hash = init_name_hash();
	while (len--)
		hash = partial_name_hash(*name++, hash);
	return end_name_hash(hash);
}

#ifdef CONFIG_64BIT
#define DNAME_INLINE_LEN_MIN 32  
#else
#define DNAME_INLINE_LEN_MIN 40  
#endif

struct dentry {
	atomic_t d_count;
	unsigned int d_flags;		 
	spinlock_t d_lock;		 
	int d_mounted;
	struct inode *d_inode;		 
	 
	struct hlist_node d_hash;	 
	struct dentry *d_parent;	 
	struct qstr d_name;

	struct list_head d_lru;		 
	 
	union {
		struct list_head d_child;	 
	 	struct rcu_head d_rcu;
	} d_u;
	struct list_head d_subdirs;	 
	struct list_head d_alias;	 
	unsigned long d_time;		 
	const struct dentry_operations *d_op;
	struct super_block *d_sb;	 
	void *d_fsdata;			 

	unsigned char d_iname[DNAME_INLINE_LEN_MIN];	 
};

enum dentry_d_lock_class
{
	DENTRY_D_LOCK_NORMAL,  
	DENTRY_D_LOCK_NESTED
};

struct dentry_operations {
	int (*d_revalidate)(struct dentry *, struct nameidata *);
	int (*d_hash) (struct dentry *, struct qstr *);
	int (*d_compare) (struct dentry *, struct qstr *, struct qstr *);
	int (*d_delete)(struct dentry *);
	void (*d_release)(struct dentry *);
	void (*d_iput)(struct dentry *, struct inode *);
	char *(*d_dname)(struct dentry *, char *, int);
};

#define DCACHE_AUTOFS_PENDING 0x0001     
#define DCACHE_NFSFS_RENAMED  0x0002     
#define	DCACHE_DISCONNECTED 0x0004
      
#define DCACHE_REFERENCED	0x0008   
#define DCACHE_UNHASHED		0x0010	

#define DCACHE_INOTIFY_PARENT_WATCHED	0x0020  

#define DCACHE_COOKIE		0x0040	 

#define DCACHE_FSNOTIFY_PARENT_WATCHED	0x0080  

#ifdef MY_ABC_HERE
#define DCACHE_CASELESS_COMPARE 0x0100  
#endif

extern spinlock_t dcache_lock;
extern seqlock_t rename_lock;

static inline void __d_drop(struct dentry *dentry)
{
	if (!(dentry->d_flags & DCACHE_UNHASHED)) {
		dentry->d_flags |= DCACHE_UNHASHED;
		hlist_del_rcu(&dentry->d_hash);
	}
}

static inline void d_drop(struct dentry *dentry)
{
	spin_lock(&dcache_lock);
	spin_lock(&dentry->d_lock);
 	__d_drop(dentry);
	spin_unlock(&dentry->d_lock);
	spin_unlock(&dcache_lock);
}

static inline int dname_external(struct dentry *dentry)
{
	return dentry->d_name.name != dentry->d_iname;
}

extern void d_instantiate(struct dentry *, struct inode *);
extern struct dentry * d_instantiate_unique(struct dentry *, struct inode *);
extern struct dentry * d_materialise_unique(struct dentry *, struct inode *);
extern void d_delete(struct dentry *);

extern struct dentry * d_alloc(struct dentry *, const struct qstr *);
extern struct dentry * d_splice_alias(struct inode *, struct dentry *);
extern struct dentry * d_add_ci(struct dentry *, struct inode *, struct qstr *);
extern struct dentry * d_obtain_alias(struct inode *);
extern void shrink_dcache_sb(struct super_block *);
extern void shrink_dcache_parent(struct dentry *);
extern void shrink_dcache_for_umount(struct super_block *);
extern int d_invalidate(struct dentry *);

extern struct dentry * d_alloc_root(struct inode *);

extern void d_genocide(struct dentry *);

extern struct dentry *d_find_alias(struct inode *);
extern void d_prune_aliases(struct inode *);

extern int have_submounts(struct dentry *);

extern void d_rehash(struct dentry *);

static inline void d_add(struct dentry *entry, struct inode *inode)
{
	d_instantiate(entry, inode);
	d_rehash(entry);
}

static inline struct dentry *d_add_unique(struct dentry *entry, struct inode *inode)
{
	struct dentry *res;

	res = d_instantiate_unique(entry, inode);
	d_rehash(res != NULL ? res : entry);
	return res;
}

extern void d_move(struct dentry *, struct dentry *);
extern struct dentry *d_ancestor(struct dentry *, struct dentry *);

extern struct dentry * d_lookup(struct dentry *, struct qstr *);
extern struct dentry * __d_lookup(struct dentry *, struct qstr *);
extern struct dentry * d_hash_and_lookup(struct dentry *, struct qstr *);

extern int d_validate(struct dentry *, struct dentry *);

extern char *dynamic_dname(struct dentry *, char *, int, const char *, ...);

extern char *__d_path(const struct path *path, struct path *root, char *, int);
extern char *d_path(const struct path *, char *, int);
extern char *dentry_path(struct dentry *, char *, int);

static inline struct dentry *dget(struct dentry *dentry)
{
	if (dentry) {
		BUG_ON(!atomic_read(&dentry->d_count));
		atomic_inc(&dentry->d_count);
	}
	return dentry;
}

extern struct dentry * dget_locked(struct dentry *);

static inline int d_unhashed(struct dentry *dentry)
{
	return (dentry->d_flags & DCACHE_UNHASHED);
}

static inline int d_unlinked(struct dentry *dentry)
{
	return d_unhashed(dentry) && !IS_ROOT(dentry);
}

static inline struct dentry *dget_parent(struct dentry *dentry)
{
	struct dentry *ret;

	spin_lock(&dentry->d_lock);
	ret = dget(dentry->d_parent);
	spin_unlock(&dentry->d_lock);
	return ret;
}

extern void dput(struct dentry *);

static inline int d_mountpoint(struct dentry *dentry)
{
	return dentry->d_mounted;
}

extern struct vfsmount *lookup_mnt(struct path *);
extern struct dentry *lookup_create(struct nameidata *nd, int is_dir);

extern int sysctl_vfs_cache_pressure;

#endif	 
