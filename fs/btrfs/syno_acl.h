/*
 *  File: fs/btrfs/syno_acl.h
 *
 */
#include <linux/syno_acl.h>

#define BTRFS_SYNO_ACL_VERSION 0x0002

struct btrfs_syno_acl_entry{
	__le16		e_tag;
	__le16		e_inherit;
	__le32		e_perm;
	__le32		e_id;
} __attribute__ ((__packed__));

typedef struct btrfs_syno_acl_entry btrfs_syno_acl_entry_t;

typedef struct {
	__le16		a_version;
} btrfs_syno_acl_header_t;

struct syno_acl *btrfs_get_syno_acl(struct inode *inode);
int btrfs_set_syno_acl(struct inode *inode, struct syno_acl *acl);

