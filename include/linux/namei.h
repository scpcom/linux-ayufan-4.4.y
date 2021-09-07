#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_NAMEI_H
#define _LINUX_NAMEI_H

#include <linux/dcache.h>
#include <linux/linkage.h>
#include <linux/path.h>

struct vfsmount;

struct open_intent {
	int	flags;
	int	create_mode;
	struct file *file;
};

enum { MAX_NESTED_LINKS = 8 };

struct nameidata {
	struct path	path;
	struct qstr	last;
	struct path	root;
	unsigned int	flags;
	int		last_type;
	unsigned	depth;
	char *saved_names[MAX_NESTED_LINKS + 1];
#ifdef MY_ABC_HERE
	unsigned char *real_filename;
	unsigned int real_filename_len;
#endif

	union {
		struct open_intent open;
	} intent;
};

enum {LAST_NORM, LAST_ROOT, LAST_DOT, LAST_DOTDOT, LAST_BIND};

#define LOOKUP_FOLLOW		 1
#define LOOKUP_DIRECTORY	 2
#define LOOKUP_CONTINUE		 4
#define LOOKUP_PARENT		16
#define LOOKUP_REVAL		64
 
#define LOOKUP_OPEN		0x0100
#define LOOKUP_CREATE		0x0200
#define LOOKUP_EXCL		0x0400
#define LOOKUP_RENAME_TARGET	0x0800

#ifdef MY_ABC_HERE
 
#define LOOKUP_TO_LASTCOMPONENT 0x1000
#define LOOKUP_CASELESS_COMPARE 0x2000
#endif

extern int user_path_at(int, const char __user *, unsigned, struct path *);
#ifdef MY_ABC_HERE
extern int syno_user_path_at(int, const char __user *, unsigned, struct path *, char **, int *);
#endif

#define user_path(name, path) user_path_at(AT_FDCWD, name, LOOKUP_FOLLOW, path)
#define user_lpath(name, path) user_path_at(AT_FDCWD, name, 0, path)
#define user_path_dir(name, path) \
	user_path_at(AT_FDCWD, name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, path)

extern int kern_path(const char *, unsigned, struct path *);

extern int path_lookup(const char *, unsigned, struct nameidata *);
extern int vfs_path_lookup(struct dentry *, struct vfsmount *,
			   const char *, unsigned int, struct nameidata *);

extern struct file *lookup_instantiate_filp(struct nameidata *nd, struct dentry *dentry,
		int (*open)(struct inode *, struct file *));
extern struct file *nameidata_to_filp(struct nameidata *nd, int flags);
extern void release_open_intent(struct nameidata *);

#ifdef CONFIG_AUFS_FS
extern struct dentry *lookup_hash(struct nameidata *nd);
extern int __lookup_one_len(const char *name, struct qstr *this,
			    struct dentry *base, int len);
#endif  
extern struct dentry *lookup_one_len(const char *, struct dentry *, int);
extern struct dentry *lookup_one_noperm(const char *, struct dentry *);

extern int follow_down(struct path *);
extern int follow_up(struct path *);

extern struct dentry *lock_rename(struct dentry *, struct dentry *);
extern void unlock_rename(struct dentry *, struct dentry *);

static inline void nd_set_link(struct nameidata *nd, char *path)
{
	nd->saved_names[nd->depth] = path;
}

static inline char *nd_get_link(struct nameidata *nd)
{
	return nd->saved_names[nd->depth];
}

static inline void nd_terminate_link(void *name, size_t len, size_t maxlen)
{
	((char *) name)[min(len, maxlen)] = '\0';
}

#endif  
