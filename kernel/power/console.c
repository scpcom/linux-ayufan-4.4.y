#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/console.h>
#include <linux/vt_kern.h>
#include <linux/kbd_kern.h>
#include <linux/vt.h>
#include <linux/module.h>
#include "power.h"

#define SUSPEND_CONSOLE	(MAX_NR_CONSOLES-1)

static int orig_fgconsole, orig_kmsg;

static DEFINE_MUTEX(vt_switch_mutex);

struct pm_vt_switch {
	struct list_head head;
	struct device *dev;
	bool required;
};

static LIST_HEAD(pm_vt_switch_list);

void pm_vt_switch_required(struct device *dev, bool required)
{
	struct pm_vt_switch *entry, *tmp;

	mutex_lock(&vt_switch_mutex);
	list_for_each_entry(tmp, &pm_vt_switch_list, head) {
		if (tmp->dev == dev) {
			 
			tmp->required = required;
			goto out;
		}
	}

	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		goto out;

	entry->required = required;
	entry->dev = dev;

	list_add(&entry->head, &pm_vt_switch_list);
out:
	mutex_unlock(&vt_switch_mutex);
}
EXPORT_SYMBOL(pm_vt_switch_required);

void pm_vt_switch_unregister(struct device *dev)
{
	struct pm_vt_switch *tmp;

	mutex_lock(&vt_switch_mutex);
	list_for_each_entry(tmp, &pm_vt_switch_list, head) {
		if (tmp->dev == dev) {
			list_del(&tmp->head);
#if defined (MY_DEF_HERE)
			kfree(tmp);
#endif  
			break;
		}
	}
	mutex_unlock(&vt_switch_mutex);
}
EXPORT_SYMBOL(pm_vt_switch_unregister);

static bool pm_vt_switch(void)
{
	struct pm_vt_switch *entry;
	bool ret = true;

	mutex_lock(&vt_switch_mutex);
	if (list_empty(&pm_vt_switch_list))
		goto out;

	if (!console_suspend_enabled)
		goto out;

	list_for_each_entry(entry, &pm_vt_switch_list, head) {
		if (entry->required)
			goto out;
	}

	ret = false;
out:
	mutex_unlock(&vt_switch_mutex);
	return ret;
}

int pm_prepare_console(void)
{
	if (!pm_vt_switch())
		return 0;

	orig_fgconsole = vt_move_to_console(SUSPEND_CONSOLE, 1);
	if (orig_fgconsole < 0)
		return 1;

	orig_kmsg = vt_kmsg_redirect(SUSPEND_CONSOLE);
	return 0;
}

void pm_restore_console(void)
{
	if (!pm_vt_switch())
		return;

	if (orig_fgconsole >= 0) {
		vt_move_to_console(orig_fgconsole, 0);
		vt_kmsg_redirect(orig_kmsg);
	}
}
