 
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/srcu.h>
#include <linux/rculist.h>
#include <linux/wait.h>
#ifdef CONFIG_AUFS_FS
#include <linux/module.h>
#endif  

#include <linux/fsnotify_backend.h>
#include "fsnotify.h"

#include <asm/atomic.h>

static DEFINE_MUTEX(fsnotify_grp_mutex);
 
struct srcu_struct fsnotify_grp_srcu;
 
LIST_HEAD(fsnotify_groups);
 
__u32 fsnotify_mask;

void fsnotify_recalc_global_mask(void)
{
	struct fsnotify_group *group;
	__u32 mask = 0;
	int idx;

	idx = srcu_read_lock(&fsnotify_grp_srcu);
	list_for_each_entry_rcu(group, &fsnotify_groups, group_list)
		mask |= group->mask;
	srcu_read_unlock(&fsnotify_grp_srcu, idx);
	fsnotify_mask = mask;
}

void fsnotify_recalc_group_mask(struct fsnotify_group *group)
{
	__u32 mask = 0;
	__u32 old_mask = group->mask;
	struct fsnotify_mark_entry *entry;

	spin_lock(&group->mark_lock);
	list_for_each_entry(entry, &group->mark_entries, g_list)
		mask |= entry->mask;
	spin_unlock(&group->mark_lock);

	group->mask = mask;

	if (old_mask != mask)
		fsnotify_recalc_global_mask();
}

static void fsnotify_get_group(struct fsnotify_group *group)
{
	atomic_inc(&group->refcnt);
}

void fsnotify_final_destroy_group(struct fsnotify_group *group)
{
	 
	fsnotify_flush_notify(group);

	if (group->ops->free_group_priv)
		group->ops->free_group_priv(group);

	kfree(group);
}

static void fsnotify_destroy_group(struct fsnotify_group *group)
{
	 
	fsnotify_clear_marks_by_group(group);

	if (atomic_dec_and_test(&group->num_marks))
		fsnotify_final_destroy_group(group);
}

static void __fsnotify_evict_group(struct fsnotify_group *group)
{
	BUG_ON(!mutex_is_locked(&fsnotify_grp_mutex));

	if (group->on_group_list)
		list_del_rcu(&group->group_list);
	group->on_group_list = 0;
}

void fsnotify_evict_group(struct fsnotify_group *group)
{
	mutex_lock(&fsnotify_grp_mutex);
	__fsnotify_evict_group(group);
	mutex_unlock(&fsnotify_grp_mutex);
}

void fsnotify_put_group(struct fsnotify_group *group)
{
	if (!atomic_dec_and_mutex_lock(&group->refcnt, &fsnotify_grp_mutex))
		return;

	__fsnotify_evict_group(group);

	mutex_unlock(&fsnotify_grp_mutex);
	synchronize_srcu(&fsnotify_grp_srcu);

	fsnotify_recalc_global_mask();
	fsnotify_destroy_group(group);
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(fsnotify_put_group);
#endif  

static struct fsnotify_group *fsnotify_find_group(unsigned int group_num, __u32 mask,
						  const struct fsnotify_ops *ops)
{
	struct fsnotify_group *group_iter;
	struct fsnotify_group *group = NULL;

	BUG_ON(!mutex_is_locked(&fsnotify_grp_mutex));

	list_for_each_entry_rcu(group_iter, &fsnotify_groups, group_list) {
		if (group_iter->group_num == group_num) {
			if ((group_iter->mask == mask) &&
			    (group_iter->ops == ops)) {
				fsnotify_get_group(group_iter);
				group = group_iter;
			} else
				group = ERR_PTR(-EEXIST);
		}
	}
	return group;
}

struct fsnotify_group *fsnotify_obtain_group(unsigned int group_num, __u32 mask,
					     const struct fsnotify_ops *ops)
{
	struct fsnotify_group *group, *tgroup;

	group = kmalloc(sizeof(struct fsnotify_group), GFP_KERNEL);
	if (!group)
		return ERR_PTR(-ENOMEM);

	atomic_set(&group->refcnt, 1);

	group->on_group_list = 0;
	group->group_num = group_num;
	group->mask = mask;

	mutex_init(&group->notification_mutex);
	INIT_LIST_HEAD(&group->notification_list);
	init_waitqueue_head(&group->notification_waitq);
	group->q_len = 0;
	group->max_events = UINT_MAX;

	spin_lock_init(&group->mark_lock);
	atomic_set(&group->num_marks, 0);
	INIT_LIST_HEAD(&group->mark_entries);

	group->ops = ops;

	mutex_lock(&fsnotify_grp_mutex);
	tgroup = fsnotify_find_group(group_num, mask, ops);
	if (tgroup) {
		 
		mutex_unlock(&fsnotify_grp_mutex);
		 
		fsnotify_put_group(group);
		return tgroup;
	}

	list_add_rcu(&group->group_list, &fsnotify_groups);
	group->on_group_list = 1;
	 
	atomic_inc(&group->num_marks);

	mutex_unlock(&fsnotify_grp_mutex);

	if (mask)
		fsnotify_recalc_global_mask();

	return group;
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(fsnotify_obtain_group);
#endif  
