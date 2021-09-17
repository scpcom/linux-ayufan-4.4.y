#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/mtt/mtt.h>

#include <linux/hardirq.h>

#ifdef CONFIG_KPTRACE
#include <linux/mtt/kptrace.h>
#endif

LIST_HEAD(mtt_comp_list);

static struct mutex comp_mutex;

struct mtt_component_obj **mtt_comp_cpu;
struct mtt_component_obj **mtt_comp_kmux;

struct mtt_component_attribute {
	struct attribute attr;
	 ssize_t(*show) (struct mtt_component_obj *mtt_component,
			 struct mtt_component_attribute *attr, char *buf);
	 ssize_t(*store) (struct mtt_component_obj *mtt_component,
			  struct mtt_component_attribute *attr,
			  const char *buf, size_t count);
};
#define to_mtt_component_attr(x)\
	container_of(x, struct mtt_component_attribute, attr)

static struct mtt_component_obj *create_mtt_component_obj(int comp_id,
							const char *name,
							int early);

static ssize_t mtt_component_attr_show(struct kobject *kobj,
				       struct attribute *attr, char *buf)
{
	struct mtt_component_attribute *attribute;
	struct mtt_component_obj *mtt_component;

	attribute = to_mtt_component_attr(attr);
	mtt_component = to_mtt_component_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(mtt_component, attribute, buf);
}

static ssize_t mtt_component_attr_store(struct kobject *kobj,
					struct attribute *attr, const char *buf,
					size_t len)
{
	struct mtt_component_attribute *attribute;
	struct mtt_component_obj *mtt_component;

	attribute = to_mtt_component_attr(attr);
	mtt_component = to_mtt_component_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(mtt_component, attribute, buf, len);
}

static const struct sysfs_ops mtt_component_sysfs_ops = {
	.show = mtt_component_attr_show,
	.store = mtt_component_attr_store,
};

static void mtt_component_release(struct kobject *kobj)
{
	struct mtt_component_obj *mtt_component;
	mtt_component = to_mtt_component_obj(kobj);
	kfree(mtt_component);
}

struct mtt_component_obj *mtt_component_alloc(uint32_t comp_id,
					      const char *comp_name,
					      int early)
{
	struct mtt_component_obj *co;

	BUG_ON(in_interrupt());

	mutex_lock(&comp_mutex);

	if (comp_id == MTT_COMP_ID_ANY) {
		uint32_t last_kernel_compid;
		uint32_t invl_kernel_compid;

		if (mtt_cur_out_drv) {
			last_kernel_compid = mtt_cur_out_drv->last_ch_ker;
			invl_kernel_compid = mtt_cur_out_drv->invl_ch_ker;
		} else {
			return NULL;
		}

		comp_id = last_kernel_compid;
		if (last_kernel_compid < invl_kernel_compid) {
			last_kernel_compid++;
			mtt_cur_out_drv->last_ch_ker = last_kernel_compid;
		}
	}

	if ((comp_id & MTT_COMPID_ST) == 0) {
		list_for_each_entry(co, &mtt_comp_list, list) {
			if (co->id == comp_id) {
				printk(KERN_DEBUG
				       "mtt: comp. 0x%x open multiple times.",
				       comp_id);
				mutex_unlock(&comp_mutex);
				return co;
			}
		}
	}

	co = create_mtt_component_obj(comp_id, comp_name, early);

	if (co) {
		 
		if (mtt_cur_out_drv && (mtt_cur_out_drv->comp_alloc_func))
			co->private =
			    mtt_cur_out_drv->comp_alloc_func(comp_id,
							     mtt_cur_out_drv->
							     private);

		if ((comp_id & MTT_COMPID_ST) == 0)
			 
			mtt_get_cname_tx(comp_id);
	}

	mutex_unlock(&comp_mutex);
	return co;
}

void mtt_component_fixup_co_private(void)
{
	struct list_head *p;
	struct mtt_component_obj *co;

	MTT_BUG_ON(mtt_cur_out_drv == NULL);

	if (!mtt_cur_out_drv->comp_alloc_func)
		return;

	mutex_lock(&comp_mutex);

	list_for_each(p, &mtt_comp_list) {
		co = list_entry(p, struct mtt_component_obj, list);
		co->private = mtt_cur_out_drv->comp_alloc_func(co->id,
					mtt_cur_out_drv->private);
	}

	mutex_unlock(&comp_mutex);
}

static ssize_t id_show(struct mtt_component_obj *mtt_component_obj,
		       struct mtt_component_attribute *attr, char *buf)
{
	return sprintf(buf, "id = %x\n", mtt_component_obj->id);
}

static struct mtt_component_attribute id_attribute =
__ATTR(id, S_IRUGO, id_show, NULL);

static ssize_t level_show(struct mtt_component_obj *co,
			  struct mtt_component_attribute *attr, char *buf)
{
	return sprintf(buf, "level filter = %x\n", co->filter);
}

static ssize_t level_store(struct mtt_component_obj *co,
			   struct mtt_component_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned long new_filter;
	int ret = kstrtoul(buf, 16, &new_filter);

	if (!ret) {
		co->filter = new_filter;
		co->active_filter = new_filter;
	}

	return count;
}

static struct mtt_component_attribute level_attribute =
__ATTR(filter, S_IRUGO | S_IWUSR, level_show, level_store);

static struct attribute *mtt_component_default_attrs[] = {
	&id_attribute.attr,
	&level_attribute.attr,
	NULL,
};

static struct kobj_type mtt_component_ktype = {
	.sysfs_ops = &mtt_component_sysfs_ops,
	.release = mtt_component_release,
	.default_attrs = mtt_component_default_attrs,
};

static struct kset *component_kset;

static struct mtt_component_obj *create_mtt_component_obj(int comp_id,
							  const char *name,
							  int early)
{
	struct mtt_component_obj *mtt_component;
#ifdef MY_DEF_HERE
#else  
	int retval;
#endif  

	mtt_component = kzalloc(sizeof(*mtt_component), GFP_KERNEL);
	if (!mtt_component)
		return NULL;

	mtt_component->id = comp_id;

	mtt_component->filter = MTT_LEVEL_ALL;
	mtt_component->active_filter = mtt_sys_config.filter;

	if (!early) {
#ifdef MY_DEF_HERE
		char sname[16];
		const char *pname;

		if (name)
			pname = name;
		else {
			 
			sprintf(sname, "comp_%05d", comp_id & 0xffff);
			pname = sname;
		}

		mtt_component->kobj.kset = component_kset;

		if (kset_find_obj(component_kset, pname) == NULL) {

			int retval;

			retval = kobject_init_and_add(&mtt_component->kobj,
				&mtt_component_ktype, NULL, "%s", pname);

			if (retval) {
				kobject_put(&mtt_component->kobj);
				kfree(mtt_component);
				return NULL;
			}
		} else
			return mtt_component;
#else  
		mtt_component->kobj.kset = component_kset;
		 
		if (name)
			retval = kobject_init_and_add(&mtt_component->kobj,
					      &mtt_component_ktype,
					      NULL, "%s", name);
		else
			retval = kobject_init_and_add(&mtt_component->kobj,
					      &mtt_component_ktype,
					      NULL, "comp_%05d",
					      comp_id & 0xffff);
		if (retval) {
			kobject_put(&mtt_component->kobj);
			kfree(mtt_component);
			return NULL;
		}
#endif  

		kobject_uevent(&mtt_component->kobj, KOBJ_ADD);
	}

	list_add_tail(&mtt_component->list, &mtt_comp_list);

	return mtt_component;
}

void mtt_component_delete(struct mtt_component_obj *co)
{
	mutex_lock(&comp_mutex);

	if (co->kobj.state_initialized) {
		kobject_uevent(&co->kobj, KOBJ_REMOVE);
		kobject_put(&co->kobj);
	}

	list_del(&co->list);

	mutex_unlock(&comp_mutex);
}

void mtt_component_snap_filter(uint32_t filter)
{
	struct list_head *p;
	struct mtt_component_obj *co;

	list_for_each(p, &mtt_comp_list) {
		co = list_entry(p, struct mtt_component_obj, list);

		if ((co->filter == MTT_LEVEL_ALL) || (filter == MTT_LEVEL_NONE))
			co->active_filter = filter;
	}
}

int mtt_component_info(struct mtt_cmd_comp_info *info  )
{
	struct list_head *p;
	struct mtt_component_obj *co;

	MTT_BUG_ON(!info);

	list_for_each(p, &mtt_comp_list) {
		co = list_entry(p, struct mtt_component_obj, list);
		if (co && (info->id == co->id)) {
			 
			info->filter = co->filter;
			strncpy(info->name, co->kobj.name, sizeof(info->name));
			return 0;
		}
	}
	return -EINVAL;
}

int __init create_components_tree(struct device *parent)
{
	int core;
	int err = 0;

	char name_kmux[] = "mtt_kmux0";
	char name_cpus[] = "mtt_cpus0";

	mutex_init(&comp_mutex);

	component_kset =
	    kset_create_and_add("components", NULL, &(parent->kobj));
	if (!component_kset)
		return -ENOMEM;

	mtt_comp_kmux = kmalloc(num_possible_cpus()
				* sizeof(struct mtt_component_obj *),
				GFP_KERNEL);

	mtt_comp_cpu = kmalloc(num_possible_cpus()
			       * sizeof(struct mtt_component_obj *),
			       GFP_KERNEL);

	if (!mtt_comp_kmux || !mtt_comp_cpu)
		return -ENOMEM;

	for (core = 0; (core < num_possible_cpus()) && !err; core++) {
		name_kmux[sizeof(name_kmux)-2] = 0x30 + core;
		name_cpus[sizeof(name_cpus)-2] = 0x30 + core;
		mtt_comp_kmux[core] = mtt_component_alloc(MTT_COMPID_LIN_KMUX
						+ MTT_COMPID_MAOFFSET * core,
						name_kmux, 0);
		mtt_comp_cpu[core] = mtt_component_alloc(MTT_COMPID_LIN_DATA
						+ MTT_COMPID_MAOFFSET * core,
						name_cpus, 0);
		if (!mtt_comp_kmux[core] || !mtt_comp_cpu[core])
			err = 1;
	}

	if (err) {
		 
		printk(KERN_ERR "mtt: releasing component tree (ENOMEM)");
		remove_components_tree();
		return -ENOMEM;
	}

#ifdef CONFIG_KPTRACE
	mtt_kptrace_comp_alloc();
#endif

	return 0;
}

const char *mtt_components_get_name(uint32_t id)
{
	struct list_head *p;
	struct mtt_component_obj *co;

	list_for_each(p, &mtt_comp_list) {
		co = list_entry(p, struct mtt_component_obj, list);
		if (co->id == id)
			return co->kobj.name;
	}

	return "pUnk";
}

struct mtt_component_obj *mtt_component_find(uint32_t id)
{
	struct list_head *p;
	struct mtt_component_obj *co;

	list_for_each(p, &mtt_comp_list) {
		co = list_entry(p, struct mtt_component_obj, list);
		if (co->id == id)
			return co;
	}
	return NULL;
}

void remove_components_tree(void)
{
	struct list_head *p, *tmp;
	struct mtt_component_obj *co;

	list_for_each_prev_safe(p, tmp, &mtt_comp_list) {
		co = list_entry(p, struct mtt_component_obj, list);
		mtt_printk(KERN_DEBUG "mtt: removing %s\n", co->kobj.name);
		mtt_component_delete(co);
	}

	kfree(mtt_comp_kmux);
	kfree(mtt_comp_cpu);

	kset_unregister(component_kset);
}
