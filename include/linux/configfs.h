#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _CONFIGFS_H_
#define _CONFIGFS_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/err.h>

#include <asm/atomic.h>

#ifdef MY_ABC_HERE
#define CONFIGFS_ITEM_NAME_LEN	256
#else
#define CONFIGFS_ITEM_NAME_LEN	20
#endif

struct module;

struct configfs_item_operations;
struct configfs_group_operations;
struct configfs_attribute;
struct configfs_subsystem;

struct config_item {
	char			*ci_name;
	char			ci_namebuf[CONFIGFS_ITEM_NAME_LEN];
	struct kref		ci_kref;
	struct list_head	ci_entry;
	struct config_item	*ci_parent;
	struct config_group	*ci_group;
	struct config_item_type	*ci_type;
	struct dentry		*ci_dentry;
};

extern int config_item_set_name(struct config_item *, const char *, ...);

static inline char *config_item_name(struct config_item * item)
{
	return item->ci_name;
}

extern void config_item_init(struct config_item *);
extern void config_item_init_type_name(struct config_item *item,
				       const char *name,
				       struct config_item_type *type);

extern struct config_item * config_item_get(struct config_item *);
extern void config_item_put(struct config_item *);

struct config_item_type {
	struct module				*ct_owner;
	struct configfs_item_operations		*ct_item_ops;
	struct configfs_group_operations	*ct_group_ops;
	struct configfs_attribute		**ct_attrs;
};

struct config_group {
	struct config_item		cg_item;
	struct list_head		cg_children;
	struct configfs_subsystem 	*cg_subsys;
	struct config_group		**default_groups;
};

extern void config_group_init(struct config_group *group);
extern void config_group_init_type_name(struct config_group *group,
					const char *name,
					struct config_item_type *type);

static inline struct config_group *to_config_group(struct config_item *item)
{
	return item ? container_of(item,struct config_group,cg_item) : NULL;
}

static inline struct config_group *config_group_get(struct config_group *group)
{
	return group ? to_config_group(config_item_get(&group->cg_item)) : NULL;
}

static inline void config_group_put(struct config_group *group)
{
	config_item_put(&group->cg_item);
}

extern struct config_item *config_group_find_item(struct config_group *,
						  const char *);

struct configfs_attribute {
	const char		*ca_name;
	struct module 		*ca_owner;
	mode_t			ca_mode;
};

#define CONFIGFS_ATTR_STRUCT(_item)					\
struct _item##_attribute {						\
	struct configfs_attribute attr;					\
	ssize_t (*show)(struct _item *, char *);			\
	ssize_t (*store)(struct _item *, const char *, size_t);		\
}

#define __CONFIGFS_ATTR(_name, _mode, _show, _store)			\
{									\
	.attr	= {							\
			.ca_name = __stringify(_name),			\
			.ca_mode = _mode,				\
			.ca_owner = THIS_MODULE,			\
	},								\
	.show	= _show,						\
	.store	= _store,						\
}
 
#define __CONFIGFS_ATTR_RO(_name, _show)				\
{									\
	.attr	= {							\
			.ca_name = __stringify(_name),			\
			.ca_mode = 0444,				\
			.ca_owner = THIS_MODULE,			\
	},								\
	.show	= _show,						\
}

#define CONFIGFS_ATTR_OPS(_item)					\
static ssize_t _item##_attr_show(struct config_item *item,		\
				 struct configfs_attribute *attr,	\
				 char *page)				\
{									\
	struct _item *_item = to_##_item(item);				\
	struct _item##_attribute *_item##_attr =			\
		container_of(attr, struct _item##_attribute, attr);	\
	ssize_t ret = 0;						\
									\
	if (_item##_attr->show)						\
		ret = _item##_attr->show(_item, page);			\
	return ret;							\
}									\
static ssize_t _item##_attr_store(struct config_item *item,		\
				  struct configfs_attribute *attr,	\
				  const char *page, size_t count)	\
{									\
	struct _item *_item = to_##_item(item);				\
	struct _item##_attribute *_item##_attr =			\
		container_of(attr, struct _item##_attribute, attr);	\
	ssize_t ret = -EINVAL;						\
									\
	if (_item##_attr->store)					\
		ret = _item##_attr->store(_item, page, count);		\
	return ret;							\
}

struct configfs_item_operations {
	void (*release)(struct config_item *);
	ssize_t	(*show_attribute)(struct config_item *, struct configfs_attribute *,char *);
	ssize_t	(*store_attribute)(struct config_item *,struct configfs_attribute *,const char *, size_t);
	int (*allow_link)(struct config_item *src, struct config_item *target);
#ifdef MY_ABC_HERE
	int (*check_link)(struct config_item *src, struct config_item *target);
#endif
	int (*drop_link)(struct config_item *src, struct config_item *target);
};

struct configfs_group_operations {
	struct config_item *(*make_item)(struct config_group *group, const char *name);
	struct config_group *(*make_group)(struct config_group *group, const char *name);
	int (*commit_item)(struct config_item *item);
	void (*disconnect_notify)(struct config_group *group, struct config_item *item);
	void (*drop_item)(struct config_group *group, struct config_item *item);
};

struct configfs_subsystem {
	struct config_group	su_group;
	struct mutex		su_mutex;
};

static inline struct configfs_subsystem *to_configfs_subsystem(struct config_group *group)
{
	return group ?
		container_of(group, struct configfs_subsystem, su_group) :
		NULL;
}

int configfs_register_subsystem(struct configfs_subsystem *subsys);
void configfs_unregister_subsystem(struct configfs_subsystem *subsys);

int configfs_depend_item(struct configfs_subsystem *subsys, struct config_item *target);
void configfs_undepend_item(struct config_item *target);

#endif  
