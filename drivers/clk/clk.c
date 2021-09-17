#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/clk-private.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/sched.h>

static DEFINE_SPINLOCK(enable_lock);
static DEFINE_MUTEX(prepare_lock);

static struct task_struct *prepare_owner;
static struct task_struct *enable_owner;

static int prepare_refcnt;
static int enable_refcnt;

static HLIST_HEAD(clk_root_list);
static HLIST_HEAD(clk_orphan_list);
static LIST_HEAD(clk_notifier_list);

static void clk_prepare_lock(void)
{
	if (!mutex_trylock(&prepare_lock)) {
		if (prepare_owner == current) {
			prepare_refcnt++;
			return;
		}
		mutex_lock(&prepare_lock);
	}
	WARN_ON_ONCE(prepare_owner != NULL);
	WARN_ON_ONCE(prepare_refcnt != 0);
	prepare_owner = current;
	prepare_refcnt = 1;
}

static void clk_prepare_unlock(void)
{
	WARN_ON_ONCE(prepare_owner != current);
	WARN_ON_ONCE(prepare_refcnt == 0);

	if (--prepare_refcnt)
		return;
	prepare_owner = NULL;
	mutex_unlock(&prepare_lock);
}

static unsigned long clk_enable_lock(void)
{
	unsigned long flags;

	if (!spin_trylock_irqsave(&enable_lock, flags)) {
		if (enable_owner == current) {
			enable_refcnt++;
			return flags;
		}
		spin_lock_irqsave(&enable_lock, flags);
	}
	WARN_ON_ONCE(enable_owner != NULL);
	WARN_ON_ONCE(enable_refcnt != 0);
	enable_owner = current;
	enable_refcnt = 1;
	return flags;
}

static void clk_enable_unlock(unsigned long flags)
{
	WARN_ON_ONCE(enable_owner != current);
	WARN_ON_ONCE(enable_refcnt == 0);

	if (--enable_refcnt)
		return;
	enable_owner = NULL;
	spin_unlock_irqrestore(&enable_lock, flags);
}

#ifdef CONFIG_COMMON_CLK_DEBUG
#include <linux/debugfs.h>

static struct dentry *rootdir;
static struct dentry *orphandir;
static int inited = 0;

#if defined (MY_DEF_HERE)
static void clk_summary_show_one(struct seq_file *s, struct clk *c, int level)
{
	const char *parent_name;
	char enabled, rate_name[20];

	if (!c)
		return;

	if (c->ops->get_parent) {
		u8 index = c->ops->get_parent(c->hw);
		if (index < c->num_parents)
			parent_name = (strcmp(c->parent_names[index],
					      __clk_get_parent(c)->name) ?
					      c->parent_names[index] : "S");
		else
			parent_name = "INVALID";
	} else {
		parent_name = "X";
	}

	if (c->ops->is_enabled)
		enabled = (c->ops->is_enabled(c->hw) ? '1' : '0');
	else
		enabled = 'X';

	if (c->ops->recalc_rate) {
		unsigned long parent_rate = 0, rate;

		if (c->parent)
			parent_rate = c->parent->rate;

		rate = c->ops->recalc_rate(c->hw, parent_rate);
		if (rate != c->rate)
			sprintf(rate_name, "%lu", rate);
		else
			sprintf(rate_name, "%s", "S");
	} else {
		sprintf(rate_name, "%s", "X");
	}

	seq_printf(s, "%*s%-*s %-11d %-12d %c / %s / %-12s %-10lu",
		   level * 3 + 1, "",
		   30 - level * 3, c->name,
		   c->enable_count, c->prepare_count, enabled, parent_name,
		   rate_name, c->rate);
	seq_printf(s, "\n");
}
#else  
static void clk_summary_show_one(struct seq_file *s, struct clk *c, int level)
{
	if (!c)
		return;

	seq_printf(s, "%*s%-*s %-11d %-12d %-10lu",
		   level * 3 + 1, "",
		   30 - level * 3, c->name,
		   c->enable_count, c->prepare_count, c->rate);
	seq_printf(s, "\n");
}
#endif  

static void clk_summary_show_subtree(struct seq_file *s, struct clk *c,
				     int level)
{
	struct clk *child;

	if (!c)
		return;

	clk_summary_show_one(s, c, level);

	hlist_for_each_entry(child, &c->children, child_node)
		clk_summary_show_subtree(s, child, level + 1);
}

static int clk_summary_show(struct seq_file *s, void *data)
{
	struct clk *c;

#if defined (MY_DEF_HERE)
	seq_printf(s, "                                                         -----HW Status-----\n");
	seq_printf(s, "   clock                        enable_cnt  prepare_cnt  enabled/parent/rate  rate\n");
	seq_printf(s, "------------------------------------------------------------------------------------------\n");
#else  
	seq_printf(s, "   clock                        enable_cnt  prepare_cnt  rate\n");
	seq_printf(s, "---------------------------------------------------------------------\n");

#endif  

	clk_prepare_lock();

	hlist_for_each_entry(c, &clk_root_list, child_node)
		clk_summary_show_subtree(s, c, 0);

	hlist_for_each_entry(c, &clk_orphan_list, child_node)
		clk_summary_show_subtree(s, c, 0);

	clk_prepare_unlock();

	return 0;
}

static int clk_summary_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_summary_show, inode->i_private);
}

static const struct file_operations clk_summary_fops = {
	.open		= clk_summary_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void clk_dump_one(struct seq_file *s, struct clk *c, int level)
{
	if (!c)
		return;

	seq_printf(s, "\"%s\": { ", c->name);
	seq_printf(s, "\"enable_count\": %d,", c->enable_count);
	seq_printf(s, "\"prepare_count\": %d,", c->prepare_count);
	seq_printf(s, "\"rate\": %lu", c->rate);
}

static void clk_dump_subtree(struct seq_file *s, struct clk *c, int level)
{
	struct clk *child;

	if (!c)
		return;

	clk_dump_one(s, c, level);

	hlist_for_each_entry(child, &c->children, child_node) {
		seq_printf(s, ",");
		clk_dump_subtree(s, child, level + 1);
	}

	seq_printf(s, "}");
}

static int clk_dump(struct seq_file *s, void *data)
{
	struct clk *c;
	bool first_node = true;

	seq_printf(s, "{");

	clk_prepare_lock();

	hlist_for_each_entry(c, &clk_root_list, child_node) {
		if (!first_node)
			seq_printf(s, ",");
		first_node = false;
		clk_dump_subtree(s, c, 0);
	}

	hlist_for_each_entry(c, &clk_orphan_list, child_node) {
		seq_printf(s, ",");
		clk_dump_subtree(s, c, 0);
	}

	clk_prepare_unlock();

	seq_printf(s, "}");
	return 0;
}

static int clk_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, clk_dump, inode->i_private);
}

static const struct file_operations clk_dump_fops = {
	.open		= clk_dump_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int clk_debug_create_one(struct clk *clk, struct dentry *pdentry)
{
	struct dentry *d;
	int ret = -ENOMEM;

	if (!clk || !pdentry) {
		ret = -EINVAL;
		goto out;
	}

	d = debugfs_create_dir(clk->name, pdentry);
	if (!d)
		goto out;

	clk->dentry = d;

	d = debugfs_create_u32("clk_rate", S_IRUGO, clk->dentry,
			(u32 *)&clk->rate);
	if (!d)
		goto err_out;

	d = debugfs_create_x32("clk_flags", S_IRUGO, clk->dentry,
			(u32 *)&clk->flags);
	if (!d)
		goto err_out;

	d = debugfs_create_u32("clk_prepare_count", S_IRUGO, clk->dentry,
			(u32 *)&clk->prepare_count);
	if (!d)
		goto err_out;

	d = debugfs_create_u32("clk_enable_count", S_IRUGO, clk->dentry,
			(u32 *)&clk->enable_count);
	if (!d)
		goto err_out;

	d = debugfs_create_u32("clk_notifier_count", S_IRUGO, clk->dentry,
			(u32 *)&clk->notifier_count);
	if (!d)
		goto err_out;

	ret = 0;
	goto out;

err_out:
	debugfs_remove(clk->dentry);
out:
	return ret;
}

static int clk_debug_create_subtree(struct clk *clk, struct dentry *pdentry)
{
	struct clk *child;
	int ret = -EINVAL;;

	if (!clk || !pdentry)
		goto out;

	ret = clk_debug_create_one(clk, pdentry);

	if (ret)
		goto out;

	hlist_for_each_entry(child, &clk->children, child_node)
		clk_debug_create_subtree(child, clk->dentry);

	ret = 0;
out:
	return ret;
}

static int clk_debug_register(struct clk *clk)
{
	struct clk *parent;
	struct dentry *pdentry;
	int ret = 0;

	if (!inited)
		goto out;

	parent = clk->parent;

	if (!parent)
		if (clk->flags & CLK_IS_ROOT)
			pdentry = rootdir;
		else
			pdentry = orphandir;
	else
		if (parent->dentry)
			pdentry = parent->dentry;
		else
			goto out;

	ret = clk_debug_create_subtree(clk, pdentry);

out:
	return ret;
}

static void clk_debug_reparent(struct clk *clk, struct clk *new_parent)
{
	struct dentry *d;
	struct dentry *new_parent_d;

	if (!inited)
		return;

	if (new_parent)
		new_parent_d = new_parent->dentry;
	else
		new_parent_d = orphandir;

	d = debugfs_rename(clk->dentry->d_parent, clk->dentry,
			new_parent_d, clk->name);
	if (d)
		clk->dentry = d;
	else
		pr_debug("%s: failed to rename debugfs entry for %s\n",
				__func__, clk->name);
}

static int __init clk_debug_init(void)
{
	struct clk *clk;
	struct dentry *d;

	rootdir = debugfs_create_dir("clk", NULL);

	if (!rootdir)
		return -ENOMEM;

	d = debugfs_create_file("clk_summary", S_IRUGO, rootdir, NULL,
				&clk_summary_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("clk_dump", S_IRUGO, rootdir, NULL,
				&clk_dump_fops);
	if (!d)
		return -ENOMEM;

	orphandir = debugfs_create_dir("orphans", rootdir);

	if (!orphandir)
		return -ENOMEM;

	clk_prepare_lock();

	hlist_for_each_entry(clk, &clk_root_list, child_node)
		clk_debug_create_subtree(clk, rootdir);

	hlist_for_each_entry(clk, &clk_orphan_list, child_node)
		clk_debug_create_subtree(clk, orphandir);

	inited = 1;

	clk_prepare_unlock();

	return 0;
}
late_initcall(clk_debug_init);
#else
static inline int clk_debug_register(struct clk *clk) { return 0; }
static inline void clk_debug_reparent(struct clk *clk, struct clk *new_parent)
{
}
#endif

static void clk_unprepare_unused_subtree(struct clk *clk)
{
	struct clk *child;

	if (!clk)
		return;

	hlist_for_each_entry(child, &clk->children, child_node)
		clk_unprepare_unused_subtree(child);

	if (clk->prepare_count)
		return;

	if (clk->flags & CLK_IGNORE_UNUSED)
		return;

	if (__clk_is_prepared(clk)) {
		if (clk->ops->unprepare_unused)
			clk->ops->unprepare_unused(clk->hw);
		else if (clk->ops->unprepare)
			clk->ops->unprepare(clk->hw);
	}
}
EXPORT_SYMBOL_GPL(__clk_get_flags);

static void clk_disable_unused_subtree(struct clk *clk)
{
	struct clk *child;
	unsigned long flags;

	if (!clk)
		goto out;

	hlist_for_each_entry(child, &clk->children, child_node)
		clk_disable_unused_subtree(child);

	flags = clk_enable_lock();

	if (clk->enable_count)
		goto unlock_out;

	if (clk->flags & CLK_IGNORE_UNUSED)
		goto unlock_out;

	if (__clk_is_enabled(clk)) {
		if (clk->ops->disable_unused)
			clk->ops->disable_unused(clk->hw);
		else if (clk->ops->disable)
			clk->ops->disable(clk->hw);
	}

unlock_out:
	clk_enable_unlock(flags);

out:
	return;
}

static bool clk_ignore_unused;
static int __init clk_ignore_unused_setup(char *__unused)
{
	clk_ignore_unused = true;
	return 1;
}
__setup("clk_ignore_unused", clk_ignore_unused_setup);

static int clk_disable_unused(void)
{
	struct clk *clk;

	if (clk_ignore_unused) {
		pr_warn("clk: Not disabling unused clocks\n");
		return 0;
	}

	clk_prepare_lock();

	hlist_for_each_entry(clk, &clk_root_list, child_node)
		clk_disable_unused_subtree(clk);

	hlist_for_each_entry(clk, &clk_orphan_list, child_node)
		clk_disable_unused_subtree(clk);

	hlist_for_each_entry(clk, &clk_root_list, child_node)
		clk_unprepare_unused_subtree(clk);

	hlist_for_each_entry(clk, &clk_orphan_list, child_node)
		clk_unprepare_unused_subtree(clk);

	clk_prepare_unlock();

	return 0;
}
late_initcall(clk_disable_unused);

const char *__clk_get_name(struct clk *clk)
{
	return !clk ? NULL : clk->name;
}
EXPORT_SYMBOL_GPL(__clk_get_name);

struct clk_hw *__clk_get_hw(struct clk *clk)
{
	return !clk ? NULL : clk->hw;
}

u8 __clk_get_num_parents(struct clk *clk)
{
	return !clk ? 0 : clk->num_parents;
}

struct clk *__clk_get_parent(struct clk *clk)
{
	return !clk ? NULL : clk->parent;
}

unsigned int __clk_get_enable_count(struct clk *clk)
{
	return !clk ? 0 : clk->enable_count;
}

unsigned int __clk_get_prepare_count(struct clk *clk)
{
	return !clk ? 0 : clk->prepare_count;
}

unsigned long __clk_get_rate(struct clk *clk)
{
	unsigned long ret;

	if (!clk) {
		ret = 0;
		goto out;
	}

	ret = clk->rate;

	if (clk->flags & CLK_IS_ROOT)
		goto out;

	if (!clk->parent)
		ret = 0;

out:
	return ret;
}

unsigned long __clk_get_flags(struct clk *clk)
{
	return !clk ? 0 : clk->flags;
}

bool __clk_is_prepared(struct clk *clk)
{
	int ret;

	if (!clk)
		return false;

	if (!clk->ops->is_prepared) {
		ret = clk->prepare_count ? 1 : 0;
		goto out;
	}

	ret = clk->ops->is_prepared(clk->hw);
out:
	return !!ret;
}

bool __clk_is_enabled(struct clk *clk)
{
	int ret;

	if (!clk)
		return false;

	if (!clk->ops->is_enabled) {
		ret = clk->enable_count ? 1 : 0;
		goto out;
	}

	ret = clk->ops->is_enabled(clk->hw);
out:
	return !!ret;
}

static struct clk *__clk_lookup_subtree(const char *name, struct clk *clk)
{
	struct clk *child;
	struct clk *ret;

	if (!strcmp(clk->name, name))
		return clk;

	hlist_for_each_entry(child, &clk->children, child_node) {
		ret = __clk_lookup_subtree(name, child);
		if (ret)
			return ret;
	}

	return NULL;
}

struct clk *__clk_lookup(const char *name)
{
	struct clk *root_clk;
	struct clk *ret;

	if (!name)
		return NULL;

	hlist_for_each_entry(root_clk, &clk_root_list, child_node) {
		ret = __clk_lookup_subtree(name, root_clk);
		if (ret)
			return ret;
	}

	hlist_for_each_entry(root_clk, &clk_orphan_list, child_node) {
		ret = __clk_lookup_subtree(name, root_clk);
		if (ret)
			return ret;
	}

	return NULL;
}

void __clk_unprepare(struct clk *clk)
{
	if (!clk)
		return;

	if (WARN_ON(clk->prepare_count == 0))
		return;

	if (--clk->prepare_count > 0)
		return;

	WARN_ON(clk->enable_count > 0);

	if (clk->ops->unprepare)
		clk->ops->unprepare(clk->hw);

	__clk_unprepare(clk->parent);
}

void clk_unprepare(struct clk *clk)
{
	clk_prepare_lock();
	__clk_unprepare(clk);
	clk_prepare_unlock();
}
EXPORT_SYMBOL_GPL(clk_unprepare);

int __clk_prepare(struct clk *clk)
{
	int ret = 0;

	if (!clk)
		return 0;

	if (clk->prepare_count == 0) {
		ret = __clk_prepare(clk->parent);
		if (ret)
			return ret;

		if (clk->ops->prepare) {
			ret = clk->ops->prepare(clk->hw);
			if (ret) {
				__clk_unprepare(clk->parent);
				return ret;
			}
		}
	}

	clk->prepare_count++;

	return 0;
}

int clk_prepare(struct clk *clk)
{
	int ret;

	clk_prepare_lock();
	ret = __clk_prepare(clk);
	clk_prepare_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(clk_prepare);

static void __clk_disable(struct clk *clk)
{
	if (!clk)
		return;

	if (WARN_ON(IS_ERR(clk)))
		return;

	if (WARN_ON(clk->enable_count == 0))
		return;

	if (--clk->enable_count > 0)
		return;

	if (clk->ops->disable)
		clk->ops->disable(clk->hw);

	__clk_disable(clk->parent);
}

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	flags = clk_enable_lock();
	__clk_disable(clk);
	clk_enable_unlock(flags);
}
EXPORT_SYMBOL_GPL(clk_disable);

static int __clk_enable(struct clk *clk)
{
	int ret = 0;

	if (!clk)
		return 0;

	if (WARN_ON(clk->prepare_count == 0))
		return -ESHUTDOWN;

	if (clk->enable_count == 0) {
		ret = __clk_enable(clk->parent);

		if (ret)
			return ret;

		if (clk->ops->enable) {
			ret = clk->ops->enable(clk->hw);
			if (ret) {
				__clk_disable(clk->parent);
				return ret;
			}
		}
	}

	clk->enable_count++;
	return 0;
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret;

	flags = clk_enable_lock();
	ret = __clk_enable(clk);
	clk_enable_unlock(flags);

	return ret;
}
EXPORT_SYMBOL_GPL(clk_enable);

unsigned long __clk_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned long parent_rate = 0;

	if (!clk)
		return 0;

	if (!clk->ops->round_rate) {
		if (clk->flags & CLK_SET_RATE_PARENT)
			return __clk_round_rate(clk->parent, rate);
		else
			return clk->rate;
	}

	if (clk->parent)
		parent_rate = clk->parent->rate;

	return clk->ops->round_rate(clk->hw, rate, &parent_rate);
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned long ret;

	clk_prepare_lock();
	ret = __clk_round_rate(clk, rate);
	clk_prepare_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(clk_round_rate);

static int __clk_notify(struct clk *clk, unsigned long msg,
		unsigned long old_rate, unsigned long new_rate)
{
	struct clk_notifier *cn;
	struct clk_notifier_data cnd;
	int ret = NOTIFY_DONE;

	cnd.clk = clk;
	cnd.old_rate = old_rate;
	cnd.new_rate = new_rate;

	list_for_each_entry(cn, &clk_notifier_list, node) {
		if (cn->clk == clk) {
			ret = srcu_notifier_call_chain(&cn->notifier_head, msg,
					&cnd);
			break;
		}
	}

	return ret;
}

static void __clk_recalc_rates(struct clk *clk, unsigned long msg)
{
	unsigned long old_rate;
	unsigned long parent_rate = 0;
	struct clk *child;

	old_rate = clk->rate;

	if (clk->parent)
		parent_rate = clk->parent->rate;

	if (clk->ops->recalc_rate)
		clk->rate = clk->ops->recalc_rate(clk->hw, parent_rate);
	else
		clk->rate = parent_rate;

	if (clk->notifier_count && msg)
		__clk_notify(clk, msg, old_rate, clk->rate);

	hlist_for_each_entry(child, &clk->children, child_node)
		__clk_recalc_rates(child, msg);
}

unsigned long clk_get_rate(struct clk *clk)
{
	unsigned long rate;

	clk_prepare_lock();

	if (clk && (clk->flags & CLK_GET_RATE_NOCACHE))
		__clk_recalc_rates(clk, 0);

	rate = __clk_get_rate(clk);
	clk_prepare_unlock();

	return rate;
}
EXPORT_SYMBOL_GPL(clk_get_rate);

static int __clk_speculate_rates(struct clk *clk, unsigned long parent_rate)
{
	struct clk *child;
	unsigned long new_rate;
	int ret = NOTIFY_DONE;

	if (clk->ops->recalc_rate)
		new_rate = clk->ops->recalc_rate(clk->hw, parent_rate);
	else
		new_rate = parent_rate;

	if (clk->notifier_count)
		ret = __clk_notify(clk, PRE_RATE_CHANGE, clk->rate, new_rate);

	if (ret & NOTIFY_STOP_MASK)
		goto out;

	hlist_for_each_entry(child, &clk->children, child_node) {
		ret = __clk_speculate_rates(child, new_rate);
		if (ret & NOTIFY_STOP_MASK)
			break;
	}

out:
	return ret;
}

static void clk_calc_subtree(struct clk *clk, unsigned long new_rate)
{
	struct clk *child;

	clk->new_rate = new_rate;

	hlist_for_each_entry(child, &clk->children, child_node) {
		if (child->ops->recalc_rate)
			child->new_rate = child->ops->recalc_rate(child->hw, new_rate);
		else
			child->new_rate = new_rate;
		clk_calc_subtree(child, child->new_rate);
	}
}

static struct clk *clk_calc_new_rates(struct clk *clk, unsigned long rate)
{
	struct clk *top = clk;
	unsigned long best_parent_rate = 0;
	unsigned long new_rate;

	if (IS_ERR_OR_NULL(clk))
		return NULL;

	if (clk->parent)
		best_parent_rate = clk->parent->rate;

	if (!(clk->flags & CLK_SET_RATE_PARENT)) {
		if (!clk->ops->round_rate) {
			clk->new_rate = clk->rate;
			return NULL;
		}
		new_rate = clk->ops->round_rate(clk->hw, rate, &best_parent_rate);
		goto out;
	}

	if (!clk->parent) {
		pr_debug("%s: %s has NULL parent\n", __func__, clk->name);
		return NULL;
	}

	if (!clk->ops->round_rate) {
		top = clk_calc_new_rates(clk->parent, rate);
		new_rate = clk->parent->new_rate;

		goto out;
	}

	new_rate = clk->ops->round_rate(clk->hw, rate, &best_parent_rate);

	if (best_parent_rate != clk->parent->rate) {
		top = clk_calc_new_rates(clk->parent, best_parent_rate);

		goto out;
	}

out:
	clk_calc_subtree(clk, new_rate);

	return top;
}

static struct clk *clk_propagate_rate_change(struct clk *clk, unsigned long event)
{
	struct clk *child, *fail_clk = NULL;
	int ret = NOTIFY_DONE;

	if (clk->rate == clk->new_rate)
		return NULL;

	if (clk->notifier_count) {
		ret = __clk_notify(clk, event, clk->rate, clk->new_rate);
		if (ret & NOTIFY_STOP_MASK)
			fail_clk = clk;
	}

	hlist_for_each_entry(child, &clk->children, child_node) {
		clk = clk_propagate_rate_change(child, event);
		if (clk)
			fail_clk = clk;
	}

	return fail_clk;
}

static void clk_change_rate(struct clk *clk)
{
	struct clk *child;
	unsigned long old_rate;
	unsigned long best_parent_rate = 0;

	old_rate = clk->rate;

	if (clk->parent)
		best_parent_rate = clk->parent->rate;

	if (clk->ops->set_rate)
		clk->ops->set_rate(clk->hw, clk->new_rate, best_parent_rate);

	if (clk->ops->recalc_rate)
		clk->rate = clk->ops->recalc_rate(clk->hw, best_parent_rate);
	else
		clk->rate = best_parent_rate;

	if (clk->notifier_count && old_rate != clk->rate)
		__clk_notify(clk, POST_RATE_CHANGE, old_rate, clk->rate);

	hlist_for_each_entry(child, &clk->children, child_node)
		clk_change_rate(child);
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *top, *fail_clk;
	int ret = 0;

	clk_prepare_lock();
#if defined (MY_DEF_HERE)
#else  

	if (rate == clk->rate)
		goto out;

#endif  
	if ((clk->flags & CLK_SET_RATE_GATE) && clk->prepare_count) {
		ret = -EBUSY;
		goto out;
	}

	top = clk_calc_new_rates(clk, rate);
	if (!top) {
		ret = -EINVAL;
		goto out;
	}

	fail_clk = clk_propagate_rate_change(top, PRE_RATE_CHANGE);
	if (fail_clk) {
		pr_warn("%s: failed to set %s rate\n", __func__,
				fail_clk->name);
		clk_propagate_rate_change(top, ABORT_RATE_CHANGE);
		ret = -EBUSY;
		goto out;
	}

	clk_change_rate(top);

out:
	clk_prepare_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(clk_set_rate);

struct clk *clk_get_parent(struct clk *clk)
{
	struct clk *parent;

	clk_prepare_lock();
	parent = __clk_get_parent(clk);
	clk_prepare_unlock();

	return parent;
}
EXPORT_SYMBOL_GPL(clk_get_parent);

static struct clk *__clk_init_parent(struct clk *clk)
{
	struct clk *ret = NULL;
	u8 index;

	if (!clk->num_parents)
		goto out;

	if (clk->num_parents == 1) {
		if (IS_ERR_OR_NULL(clk->parent))
			ret = clk->parent = __clk_lookup(clk->parent_names[0]);
		ret = clk->parent;
		goto out;
	}

	if (!clk->ops->get_parent) {
		WARN(!clk->ops->get_parent,
			"%s: multi-parent clocks must implement .get_parent\n",
			__func__);
		goto out;
	};

	index = clk->ops->get_parent(clk->hw);
#if defined (MY_DEF_HERE)
	if (index >= clk->num_parents)
		goto out;
#endif  

	if (!clk->parents)
		clk->parents =
			kzalloc((sizeof(struct clk*) * clk->num_parents),
					GFP_KERNEL);

	if (!clk->parents)
		ret = __clk_lookup(clk->parent_names[index]);
	else if (!clk->parents[index])
		ret = clk->parents[index] =
			__clk_lookup(clk->parent_names[index]);
	else
		ret = clk->parents[index];

out:
	return ret;
}

static void clk_reparent(struct clk *clk, struct clk *new_parent)
{
	hlist_del(&clk->child_node);

	if (new_parent)
		hlist_add_head(&clk->child_node, &new_parent->children);
	else
		hlist_add_head(&clk->child_node, &clk_orphan_list);

	clk->parent = new_parent;
}

void __clk_reparent(struct clk *clk, struct clk *new_parent)
{
	clk_reparent(clk, new_parent);
	clk_debug_reparent(clk, new_parent);
	__clk_recalc_rates(clk, POST_RATE_CHANGE);
}

static u8 clk_fetch_parent_index(struct clk *clk, struct clk *parent)
{
	u8 i;

	if (!clk->parents)
		clk->parents = kzalloc((sizeof(struct clk*) * clk->num_parents),
								GFP_KERNEL);

	for (i = 0; i < clk->num_parents; i++) {
		if (clk->parents && clk->parents[i] == parent)
			break;
		else if (!strcmp(clk->parent_names[i], parent->name)) {
			if (clk->parents)
				clk->parents[i] = __clk_lookup(parent->name);
			break;
		}
	}

	return i;
}

static int __clk_set_parent(struct clk *clk, struct clk *parent, u8 p_index)
{
	unsigned long flags;
	int ret = 0;
	struct clk *old_parent = clk->parent;
	bool migrated_enable = false;

	if (clk->prepare_count)
		__clk_prepare(parent);

	flags = clk_enable_lock();

	if (clk->enable_count) {
		__clk_enable(parent);
		migrated_enable = true;
	}

	clk_reparent(clk, parent);

	clk_enable_unlock(flags);

	if (parent && clk->ops->set_parent)
		ret = clk->ops->set_parent(clk->hw, p_index);

	if (ret) {
		 
		flags = clk_enable_lock();

		clk_reparent(clk, old_parent);

		if (migrated_enable && clk->enable_count) {
			__clk_disable(parent);
		} else if (migrated_enable && (clk->enable_count == 0)) {
			__clk_disable(old_parent);
		} else if (!migrated_enable && clk->enable_count) {
			__clk_disable(parent);
			__clk_enable(old_parent);
		}

		clk_enable_unlock(flags);

		if (clk->prepare_count)
			__clk_unprepare(parent);

		return ret;
	}

	if (migrated_enable) {
		flags = clk_enable_lock();
		__clk_disable(old_parent);
		clk_enable_unlock(flags);
	}

	if (clk->prepare_count)
		__clk_unprepare(old_parent);

	clk_debug_reparent(clk, parent);
	return 0;
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;
	u8 p_index = 0;
	unsigned long p_rate = 0;

	if (!clk || !clk->ops)
		return -EINVAL;

	if ((clk->num_parents > 1) && (!clk->ops->set_parent))
		return -ENOSYS;

	clk_prepare_lock();
#if defined (MY_DEF_HERE)
#else  

	if (clk->parent == parent)
		goto out;

#endif  
	 
	if ((clk->flags & CLK_SET_PARENT_GATE) && clk->prepare_count) {
		ret = -EBUSY;
		goto out;
	}

	if (parent) {
		p_index = clk_fetch_parent_index(clk, parent);
		p_rate = parent->rate;
		if (p_index == clk->num_parents) {
			pr_debug("%s: clk %s can not be parent of clk %s\n",
					__func__, parent->name, clk->name);
			ret = -EINVAL;
			goto out;
		}
	}

	if (clk->notifier_count)
		ret = __clk_speculate_rates(clk, p_rate);

	if (ret & NOTIFY_STOP_MASK)
		goto out;

	ret = __clk_set_parent(clk, parent, p_index);

	if (ret)
		__clk_recalc_rates(clk, ABORT_RATE_CHANGE);
	else
		__clk_recalc_rates(clk, POST_RATE_CHANGE);

out:
	clk_prepare_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(clk_set_parent);

int __clk_init(struct device *dev, struct clk *clk)
{
	int i, ret = 0;
	struct clk *orphan;
	struct hlist_node *tmp2;

	if (!clk)
		return -EINVAL;

	clk_prepare_lock();

	if (__clk_lookup(clk->name)) {
		pr_debug("%s: clk %s already initialized\n",
				__func__, clk->name);
		ret = -EEXIST;
		goto out;
	}

	if (clk->ops->set_rate &&
			!(clk->ops->round_rate && clk->ops->recalc_rate)) {
		pr_warning("%s: %s must implement .round_rate & .recalc_rate\n",
				__func__, clk->name);
		ret = -EINVAL;
		goto out;
	}

	if (clk->ops->set_parent && !clk->ops->get_parent) {
		pr_warning("%s: %s must implement .get_parent & .set_parent\n",
				__func__, clk->name);
		ret = -EINVAL;
		goto out;
	}

	for (i = 0; i < clk->num_parents; i++)
		WARN(!clk->parent_names[i],
				"%s: invalid NULL in %s's .parent_names\n",
				__func__, clk->name);

	if (clk->num_parents > 1 && !clk->parents) {
		clk->parents = kzalloc((sizeof(struct clk*) * clk->num_parents),
				GFP_KERNEL);
		 
		if (clk->parents)
			for (i = 0; i < clk->num_parents; i++)
				clk->parents[i] =
					__clk_lookup(clk->parent_names[i]);
	}

	clk->parent = __clk_init_parent(clk);

	if (clk->parent)
		hlist_add_head(&clk->child_node,
				&clk->parent->children);
	else if (clk->flags & CLK_IS_ROOT)
		hlist_add_head(&clk->child_node, &clk_root_list);
	else
		hlist_add_head(&clk->child_node, &clk_orphan_list);

	if (clk->ops->recalc_rate)
		clk->rate = clk->ops->recalc_rate(clk->hw,
				__clk_get_rate(clk->parent));
	else if (clk->parent)
		clk->rate = clk->parent->rate;
	else
		clk->rate = 0;

	hlist_for_each_entry_safe(orphan, tmp2, &clk_orphan_list, child_node) {
		if (orphan->ops->get_parent) {
			i = orphan->ops->get_parent(orphan->hw);
#if defined (MY_DEF_HERE)
			if (i < orphan->num_parents)
#endif  
			if (!strcmp(clk->name, orphan->parent_names[i]))
				__clk_reparent(orphan, clk);
			continue;
		}

		for (i = 0; i < orphan->num_parents; i++)
			if (!strcmp(clk->name, orphan->parent_names[i])) {
				__clk_reparent(orphan, clk);
				break;
			}
	 }

	if (clk->ops->init)
		clk->ops->init(clk->hw);

	clk_debug_register(clk);

out:
	clk_prepare_unlock();

	return ret;
}

struct clk *__clk_register(struct device *dev, struct clk_hw *hw)
{
	int ret;
	struct clk *clk;

	clk = hw->clk;
	clk->name = hw->init->name;
	clk->ops = hw->init->ops;
	clk->hw = hw;
	clk->flags = hw->init->flags;
	clk->parent_names = hw->init->parent_names;
	clk->num_parents = hw->init->num_parents;

	ret = __clk_init(dev, clk);
	if (ret)
		return ERR_PTR(ret);

	return clk;
}
EXPORT_SYMBOL_GPL(__clk_register);

static int _clk_register(struct device *dev, struct clk_hw *hw, struct clk *clk)
{
	int i, ret;

	clk->name = kstrdup(hw->init->name, GFP_KERNEL);
	if (!clk->name) {
		pr_err("%s: could not allocate clk->name\n", __func__);
		ret = -ENOMEM;
		goto fail_name;
	}
	clk->ops = hw->init->ops;
	clk->hw = hw;
	clk->flags = hw->init->flags;
	clk->num_parents = hw->init->num_parents;
	hw->clk = clk;

	clk->parent_names = kzalloc((sizeof(char*) * clk->num_parents),
			GFP_KERNEL);

	if (!clk->parent_names) {
		pr_err("%s: could not allocate clk->parent_names\n", __func__);
		ret = -ENOMEM;
		goto fail_parent_names;
	}

	for (i = 0; i < clk->num_parents; i++) {
		clk->parent_names[i] = kstrdup(hw->init->parent_names[i],
						GFP_KERNEL);
		if (!clk->parent_names[i]) {
			pr_err("%s: could not copy parent_names\n", __func__);
			ret = -ENOMEM;
			goto fail_parent_names_copy;
		}
	}

	ret = __clk_init(dev, clk);
	if (!ret)
		return 0;

fail_parent_names_copy:
	while (--i >= 0)
		kfree(clk->parent_names[i]);
	kfree(clk->parent_names);
fail_parent_names:
	kfree(clk->name);
fail_name:
	return ret;
}

struct clk *clk_register(struct device *dev, struct clk_hw *hw)
{
	int ret;
	struct clk *clk;

	clk = kzalloc(sizeof(*clk), GFP_KERNEL);
	if (!clk) {
		pr_err("%s: could not allocate clk\n", __func__);
		ret = -ENOMEM;
		goto fail_out;
	}

	ret = _clk_register(dev, hw, clk);
	if (!ret)
		return clk;

	kfree(clk);
fail_out:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(clk_register);

void clk_unregister(struct clk *clk) {}
EXPORT_SYMBOL_GPL(clk_unregister);

static void devm_clk_release(struct device *dev, void *res)
{
	clk_unregister(res);
}

struct clk *devm_clk_register(struct device *dev, struct clk_hw *hw)
{
	struct clk *clk;
	int ret;

	clk = devres_alloc(devm_clk_release, sizeof(*clk), GFP_KERNEL);
	if (!clk)
		return ERR_PTR(-ENOMEM);

	ret = _clk_register(dev, hw, clk);
	if (!ret) {
		devres_add(dev, clk);
	} else {
		devres_free(clk);
		clk = ERR_PTR(ret);
	}

	return clk;
}
EXPORT_SYMBOL_GPL(devm_clk_register);

static int devm_clk_match(struct device *dev, void *res, void *data)
{
	struct clk *c = res;
	if (WARN_ON(!c))
		return 0;
	return c == data;
}

void devm_clk_unregister(struct device *dev, struct clk *clk)
{
	WARN_ON(devres_release(dev, devm_clk_release, devm_clk_match, clk));
}
EXPORT_SYMBOL_GPL(devm_clk_unregister);

int clk_notifier_register(struct clk *clk, struct notifier_block *nb)
{
	struct clk_notifier *cn;
	int ret = -ENOMEM;

	if (!clk || !nb)
		return -EINVAL;

	clk_prepare_lock();

	list_for_each_entry(cn, &clk_notifier_list, node)
		if (cn->clk == clk)
			break;

	if (cn->clk != clk) {
		cn = kzalloc(sizeof(struct clk_notifier), GFP_KERNEL);
		if (!cn)
			goto out;

		cn->clk = clk;
		srcu_init_notifier_head(&cn->notifier_head);

		list_add(&cn->node, &clk_notifier_list);
	}

	ret = srcu_notifier_chain_register(&cn->notifier_head, nb);

	clk->notifier_count++;

out:
	clk_prepare_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(clk_notifier_register);

int clk_notifier_unregister(struct clk *clk, struct notifier_block *nb)
{
	struct clk_notifier *cn = NULL;
	int ret = -EINVAL;

	if (!clk || !nb)
		return -EINVAL;

	clk_prepare_lock();

	list_for_each_entry(cn, &clk_notifier_list, node)
		if (cn->clk == clk)
			break;

	if (cn->clk == clk) {
		ret = srcu_notifier_chain_unregister(&cn->notifier_head, nb);

		clk->notifier_count--;

		if (!cn->notifier_head.head) {
			srcu_cleanup_notifier_head(&cn->notifier_head);
			list_del(&cn->node);
			kfree(cn);
		}

	} else {
		ret = -ENOENT;
	}

	clk_prepare_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(clk_notifier_unregister);

#ifdef CONFIG_OF
 
struct of_clk_provider {
	struct list_head link;

	struct device_node *node;
	struct clk *(*get)(struct of_phandle_args *clkspec, void *data);
	void *data;
};

extern struct of_device_id __clk_of_table[];

static const struct of_device_id __clk_of_table_sentinel
	__used __section(__clk_of_table_end);

static LIST_HEAD(of_clk_providers);
static DEFINE_MUTEX(of_clk_lock);

struct clk *of_clk_src_simple_get(struct of_phandle_args *clkspec,
				     void *data)
{
	return data;
}
EXPORT_SYMBOL_GPL(of_clk_src_simple_get);

struct clk *of_clk_src_onecell_get(struct of_phandle_args *clkspec, void *data)
{
	struct clk_onecell_data *clk_data = data;
	unsigned int idx = clkspec->args[0];

	if (idx >= clk_data->clk_num) {
		pr_err("%s: invalid clock index %d\n", __func__, idx);
		return ERR_PTR(-EINVAL);
	}

	return clk_data->clks[idx];
}
EXPORT_SYMBOL_GPL(of_clk_src_onecell_get);

int of_clk_add_provider(struct device_node *np,
			struct clk *(*clk_src_get)(struct of_phandle_args *clkspec,
						   void *data),
			void *data)
{
	struct of_clk_provider *cp;

	cp = kzalloc(sizeof(struct of_clk_provider), GFP_KERNEL);
	if (!cp)
		return -ENOMEM;

	cp->node = of_node_get(np);
	cp->data = data;
	cp->get = clk_src_get;

	mutex_lock(&of_clk_lock);
	list_add(&cp->link, &of_clk_providers);
	mutex_unlock(&of_clk_lock);
	pr_debug("Added clock from %s\n", np->full_name);

	return 0;
}
EXPORT_SYMBOL_GPL(of_clk_add_provider);

void of_clk_del_provider(struct device_node *np)
{
	struct of_clk_provider *cp;

	mutex_lock(&of_clk_lock);
	list_for_each_entry(cp, &of_clk_providers, link) {
		if (cp->node == np) {
			list_del(&cp->link);
			of_node_put(cp->node);
			kfree(cp);
			break;
		}
	}
	mutex_unlock(&of_clk_lock);
}
EXPORT_SYMBOL_GPL(of_clk_del_provider);

struct clk *of_clk_get_from_provider(struct of_phandle_args *clkspec)
{
	struct of_clk_provider *provider;
	struct clk *clk = ERR_PTR(-ENOENT);

	mutex_lock(&of_clk_lock);
	list_for_each_entry(provider, &of_clk_providers, link) {
		if (provider->node == clkspec->np)
			clk = provider->get(clkspec, provider->data);
		if (!IS_ERR(clk))
			break;
	}
	mutex_unlock(&of_clk_lock);

	return clk;
}

const char *of_clk_get_parent_name(struct device_node *np, int index)
{
	struct of_phandle_args clkspec;
	const char *clk_name;
	int rc;

	if (index < 0)
		return NULL;

	rc = of_parse_phandle_with_args(np, "clocks", "#clock-cells", index,
					&clkspec);
	if (rc)
		return NULL;

	if (of_property_read_string_index(clkspec.np, "clock-output-names",
					  clkspec.args_count ? clkspec.args[0] : 0,
					  &clk_name) < 0)
		clk_name = clkspec.np->name;

	of_node_put(clkspec.np);
	return clk_name;
}
EXPORT_SYMBOL_GPL(of_clk_get_parent_name);

void __init of_clk_init(const struct of_device_id *matches)
{
	struct device_node *np;

	if (!matches)
		matches = __clk_of_table;

	for_each_matching_node(np, matches) {
		const struct of_device_id *match = of_match_node(matches, np);
		of_clk_init_cb_t clk_init_cb = match->data;
		clk_init_cb(np);
	}
}
#endif
