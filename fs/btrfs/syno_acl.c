/*
 * linux/fs/btrfs/syno_acl.c
 *
 * Copyright (C) 2001-2013 Synology Inc.
 */

#include <linux/fs.h>
#include <linux/string.h>
#include <linux/xattr.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "ctree.h"
#include "btrfs_inode.h"
#include "xattr.h"

#ifdef CONFIG_BTRFS_FS_SYNO_ACL
#include "syno_acl.h"
#endif

static inline int btrfs_sae_from_disk(struct syno_acl_entry *sae, btrfs_syno_acl_entry_t *bsae)
{
	unsigned short tag = le16_to_cpu(bsae->e_tag);

	//ID: user/group/everyone
	if (SYNO_ACL_XATTR_TAG_ID_GROUP & tag) {
		sae->e_tag = SYNO_ACL_GROUP;
		sae->e_id = le32_to_cpu(bsae->e_id);
	} else if (SYNO_ACL_XATTR_TAG_ID_EVERYONE & tag) {
		sae->e_tag = SYNO_ACL_EVERYONE;
		sae->e_id = SYNO_ACL_UNDEFINED_ID;
	} else if (SYNO_ACL_XATTR_TAG_ID_USER & tag) {
		sae->e_tag = SYNO_ACL_USER;
		sae->e_id = le32_to_cpu(bsae->e_id);
	} else if (SYNO_ACL_XATTR_TAG_ID_OWNER & tag) {
		sae->e_tag = SYNO_ACL_OWNER;
		sae->e_id = SYNO_ACL_UNDEFINED_ID;
	} else {
		return -EINVAL;
	}

	//Allow/Deny
	if (SYNO_ACL_XATTR_TAG_IS_DENY & tag) {
		sae->e_allow = SYNO_ACL_DENY;
	} else if (SYNO_ACL_XATTR_TAG_IS_ALLOW & tag){
		sae->e_allow = SYNO_ACL_ALLOW;
	} else {
		return -EINVAL;
	}

	sae->e_perm = le32_to_cpu(bsae->e_perm);
	sae->e_inherit = le16_to_cpu(bsae->e_inherit);
	sae->e_level = 0;
	return 0;
}

static inline int btrfs_sae_to_disk(const struct syno_acl_entry *sae, btrfs_syno_acl_entry_t *bsae)
{
	unsigned short tag = 0;

	//ID: user/group/everyone
	switch(sae->e_tag){
	case SYNO_ACL_GROUP:
		tag |= SYNO_ACL_XATTR_TAG_ID_GROUP;
		break;
	case SYNO_ACL_EVERYONE:
		tag |= SYNO_ACL_XATTR_TAG_ID_EVERYONE;
		break;
	case SYNO_ACL_USER:
		tag |= SYNO_ACL_XATTR_TAG_ID_USER;
		break;
	case SYNO_ACL_OWNER:
		tag |= SYNO_ACL_XATTR_TAG_ID_OWNER;
		break;
	default:
		return -EINVAL;
	}

	//Allow/Deny
	switch(sae->e_allow){
	case SYNO_ACL_DENY:
		tag |= SYNO_ACL_XATTR_TAG_IS_DENY;
		break;
	case SYNO_ACL_ALLOW:
		tag |= SYNO_ACL_XATTR_TAG_IS_ALLOW;
		break;
	default:
		return -EINVAL;
	}

	bsae->e_tag = cpu_to_le16(tag);
	bsae->e_inherit = cpu_to_le16(sae->e_inherit);
	bsae->e_perm = cpu_to_le32(sae->e_perm);
	bsae->e_id = cpu_to_le32(sae->e_id);
	return 0;
}

/*
 * Convert from filesystem to in-memory representation.
 */
static struct syno_acl * btrfs_syno_acl_from_disk(const void *value, size_t size)
{
	int i, count;
	struct syno_acl *acl;

	if (!value)
		return NULL;
	if (size < sizeof(btrfs_syno_acl_header_t)) {
		return ERR_PTR(-EINVAL);
	}
	if ((size - sizeof(btrfs_syno_acl_header_t)) % sizeof(btrfs_syno_acl_entry_t)) {
		return ERR_PTR(-EINVAL);
	}
	count = (size - sizeof(btrfs_syno_acl_header_t)) / sizeof(btrfs_syno_acl_entry_t);
	if (count == 0)
		return NULL;

	if (((btrfs_syno_acl_header_t *)value)->a_version != cpu_to_le16(BTRFS_SYNO_ACL_VERSION)) {
		return ERR_PTR(-EINVAL);
	}
	acl = syno_acl_alloc(count, GFP_NOFS);
	if (!acl)
		return ERR_PTR(-ENOMEM);

	value = (char *)value + sizeof(btrfs_syno_acl_header_t);
	for (i = 0; i < count; i++) {
		if (btrfs_sae_from_disk(&(acl->a_entries[i]), (btrfs_syno_acl_entry_t *)value))
			goto fail;
		value = (char *)value + sizeof(btrfs_syno_acl_entry_t);
	}
	return acl;

fail:
	syno_acl_release(acl);
	return ERR_PTR(-EINVAL);
}

/*
 * Convert from in-memory to filesystem representation.
 */
static void * btrfs_syno_acl_to_disk(const struct syno_acl *acl, size_t *size)
{
	int i;
	btrfs_syno_acl_header_t *b_acl;
	char *e;

	*size = sizeof(btrfs_syno_acl_header_t) + acl->a_count * sizeof(btrfs_syno_acl_entry_t);
	b_acl = kmalloc(*size, GFP_NOFS);
	if (!b_acl)
		return ERR_PTR(-ENOMEM);

	b_acl->a_version = cpu_to_le16(BTRFS_SYNO_ACL_VERSION);
	e = (char *)b_acl + sizeof(btrfs_syno_acl_header_t);

	for (i = 0; i < acl->a_count; i++) {
		if (0 > btrfs_sae_to_disk(&(acl->a_entries[i]), (btrfs_syno_acl_entry_t *)e))
			goto fail;
		e += sizeof(btrfs_syno_acl_entry_t);
	}
	return (char *)b_acl;

fail:
	kfree(b_acl);
	return ERR_PTR(-EINVAL);
}


/*
 * Needs to be called with fs_mutex held
 */
static int __btrfs_set_syno_acl(struct btrfs_trans_handle *trans,
			 struct inode *inode, struct syno_acl *acl)
{
	int ret;
	size_t size = 0;
	char *value = NULL;

	if (acl) {
		ret = syno_acl_valid(acl);
		if (ret < 0)
			return ret;
	}

	if (acl) {
		value = btrfs_syno_acl_to_disk(acl, &size);
		if (IS_ERR(value))
			return PTR_ERR(acl);
	}

	ret = __btrfs_setxattr(trans, inode, SYNO_ACL_XATTR_ACCESS, value, size, 0);
	kfree(value);
	if (!ret)
		set_cached_syno_acl(inode, acl);

	return ret;
}

/*
 * Inode operation syno_acl_set().
 */
int btrfs_set_syno_acl(struct inode *inode, struct syno_acl *acl)
{
	int ret;

	if (!inode || !acl) {
		return -EINVAL;
	}

	ret = __btrfs_set_syno_acl(NULL, inode, acl);

	return ret;
}

/*
 * Inode operation syno_acl_get().
 */
struct syno_acl *btrfs_get_syno_acl(struct inode *inode)
{
	int size;
	char *value = NULL;
	struct syno_acl *acl;

	acl = get_cached_syno_acl(inode);
	if (acl != ACL_NOT_CACHED)
		return acl;

	size = __btrfs_getxattr(inode, SYNO_ACL_XATTR_ACCESS, "", 0);
	if (size > 0) {
		value = kzalloc(size, GFP_NOFS);
		if (!value)
			return ERR_PTR(-ENOMEM);
		size = __btrfs_getxattr(inode, SYNO_ACL_XATTR_ACCESS, value, size);
	}
	if (size > 0) {
		acl = btrfs_syno_acl_from_disk(value, size);
	} else if (size == -ENOENT || size == -ENODATA || size == 0) {
		/* FIXME, who returns -ENOENT?  I think nobody */
		acl = NULL;
	} else {
		acl = ERR_PTR(-EIO);
	}
	kfree(value);

	if (!IS_ERR(acl))
		set_cached_syno_acl(inode, acl);

	return acl;
}

static int btrfs_xattr_syno_acl_get(struct dentry *dentry, const char *name,
		void *value, size_t size, int type)
{
	struct syno_acl *acl;
	int ret = 0;

	acl = btrfs_get_syno_acl(dentry->d_inode);

	if (IS_ERR(acl))
		return PTR_ERR(acl);
	if (acl == NULL)
		return -ENODATA;
	ret = syno_acl_to_xattr(acl, value, size);
	syno_acl_release(acl);

	return ret;
}

static int btrfs_xattr_syno_acl_set(struct dentry *dentry, const char *name,
		const void *value, size_t size, int flags, int type)
{
	int ret;
	struct syno_acl *acl = NULL;

	if (value) {
		acl = syno_acl_from_xattr(value, size);
		if (IS_ERR(acl))
			return PTR_ERR(acl);

		if (acl) {
			ret = syno_acl_valid(acl);
			if (ret)
				goto out;
		}
	}

	ret = __btrfs_set_syno_acl(NULL, dentry->d_inode, acl);
out:
	syno_acl_release(acl);

	return ret;
}

const struct xattr_handler btrfs_xattr_synoacl_access_handler = {
	.prefix	= SYNO_ACL_XATTR_ACCESS,
	.get	= btrfs_xattr_syno_acl_get,
	.set	= btrfs_xattr_syno_acl_set,
};

const struct xattr_handler btrfs_xattr_synoacl_noperm_access_handler = {
	.prefix = SYNO_ACL_XATTR_ACCESS_NOPERM,
	.get	= btrfs_xattr_syno_acl_get,
	.set	= btrfs_xattr_syno_acl_set,
};
