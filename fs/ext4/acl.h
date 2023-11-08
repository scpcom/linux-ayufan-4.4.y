/*
  File: fs/ext4/acl.h

  (C) 2001 Andreas Gruenbacher, <a.gruenbacher@computer.org>
*/

#include <linux/posix_acl_xattr.h>

#define EXT4_ACL_VERSION	0x0001

typedef struct {
	__le16		e_tag;
	__le16		e_perm;
	__le32		e_id;
} ext4_acl_entry;

typedef struct {
	__le16		e_tag;
	__le16		e_perm;
} ext4_acl_entry_short;

typedef struct {
	__le32		a_version;
} ext4_acl_header;

static inline size_t ext4_acl_size(int count)
{
	if (count <= 4) {
		return sizeof(ext4_acl_header) +
		       count * sizeof(ext4_acl_entry_short);
	} else {
		return sizeof(ext4_acl_header) +
		       4 * sizeof(ext4_acl_entry_short) +
		       (count - 4) * sizeof(ext4_acl_entry);
	}
}

static inline int ext4_acl_count(size_t size)
{
	ssize_t s;
	size -= sizeof(ext4_acl_header);
	s = size - 4 * sizeof(ext4_acl_entry_short);
	if (s < 0) {
		if (size % sizeof(ext4_acl_entry_short))
			return -1;
		return size / sizeof(ext4_acl_entry_short);
	} else {
		if (s % sizeof(ext4_acl_entry))
			return -1;
		return s / sizeof(ext4_acl_entry) + 4;
	}
}

#ifdef CONFIG_EXT4_FS_POSIX_ACL

/* acl.c */
#ifdef CONFIG_EXT4_FS_RICHACL
struct posix_acl *ext4_get_posix_acl(struct inode *inode, int type);
struct posix_acl *ext4_get_generic_acl(struct inode *inode, int type);
extern int ext4_set_generic_acl(handle_t *handle, struct inode *inode, int type, struct posix_acl *acl);
extern int ext4_set_acl(handle_t *handle, struct inode *inode, int type, struct posix_acl *acl);
#else
struct posix_acl *ext4_get_acl(struct inode *inode, int type);
#endif
extern int ext4_acl_chmod(struct inode *);
extern int ext4_init_acl(handle_t *, struct inode *, struct inode *);

#else  /* CONFIG_EXT4_FS_POSIX_ACL */
#include <linux/sched.h>
#ifdef CONFIG_EXT4_FS_RICHACL
#define ext4_get_posix_acl NULL
#else
#define ext4_get_acl NULL
#endif

static inline int
ext4_acl_chmod(struct inode *inode)
{
	return 0;
}

static inline int
ext4_init_acl(handle_t *handle, struct inode *inode, struct inode *dir)
{
	return 0;
}
#endif  /* CONFIG_EXT4_FS_POSIX_ACL */

