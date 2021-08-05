#ifndef _LINUX_SIMPLE_XATTR_H
#define _LINUX_SIMPLE_XATTR_H

#ifdef  __KERNEL__

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/xattr.h>

/*
 * initialize the simple_xattrs structure
 */
static inline void simple_xattrs_init(struct simple_xattrs *xattrs)
{
	INIT_LIST_HEAD(&xattrs->head);
	spin_lock_init(&xattrs->lock);
}

/*
 * free all the xattrs
 */
static inline void simple_xattrs_free(struct simple_xattrs *xattrs)
{
	struct simple_xattr *xattr, *node;

	list_for_each_entry_safe(xattr, node, &xattrs->head, list) {
		kfree(xattr->name);
		kfree(xattr);
	}
}

struct simple_xattr *simple_xattr_alloc(const void *value, size_t size);
int simple_xattr_get(struct simple_xattrs *xattrs, const char *name,
		     void *buffer, size_t size);
int simple_xattr_set(struct simple_xattrs *xattrs, const char *name,
		     const void *value, size_t size, int flags);
int simple_xattr_remove(struct simple_xattrs *xattrs, const char *name);
ssize_t simple_xattr_list(struct simple_xattrs *xattrs, char *buffer,
			  size_t size);
void simple_xattr_list_add(struct simple_xattrs *xattrs,
			   struct simple_xattr *new_xattr);
#endif  /*  __KERNEL__  */

#endif	/* _LINUX_SIMPLE_XATTR_H */
