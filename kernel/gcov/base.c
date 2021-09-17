#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define pr_fmt(fmt)	"gcov: " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include "gcov.h"

#if defined (MY_DEF_HERE)
#else  
static struct gcov_info *gcov_info_head;
#endif  
static int gcov_events_enabled;
static DEFINE_MUTEX(gcov_lock);

void __gcov_init(struct gcov_info *info)
{
	static unsigned int gcov_version;

	mutex_lock(&gcov_lock);
	if (gcov_version == 0) {
#if defined (MY_DEF_HERE)
		gcov_version = gcov_info_version(info);
#else  
		gcov_version = info->version;
#endif  
		 
		pr_info("version magic: 0x%x\n", gcov_version);
	}
	 
#if defined (MY_DEF_HERE)
	gcov_info_link(info);
#else  
	info->next = gcov_info_head;
	gcov_info_head = info;
#endif  
	if (gcov_events_enabled)
		gcov_event(GCOV_ADD, info);
	mutex_unlock(&gcov_lock);
}
EXPORT_SYMBOL(__gcov_init);

void __gcov_flush(void)
{
	 
}
EXPORT_SYMBOL(__gcov_flush);

void __gcov_merge_add(gcov_type *counters, unsigned int n_counters)
{
	 
}
EXPORT_SYMBOL(__gcov_merge_add);

void __gcov_merge_single(gcov_type *counters, unsigned int n_counters)
{
	 
}
EXPORT_SYMBOL(__gcov_merge_single);

void __gcov_merge_delta(gcov_type *counters, unsigned int n_counters)
{
	 
}
EXPORT_SYMBOL(__gcov_merge_delta);
#if defined (MY_DEF_HERE)

void __gcov_merge_ior(gcov_type *counters, unsigned int n_counters)
{
	 
}
EXPORT_SYMBOL(__gcov_merge_ior);
#endif  

void gcov_enable_events(void)
{
#if defined (MY_DEF_HERE)
	struct gcov_info *info = NULL;
#else  
	struct gcov_info *info;
#endif  

	mutex_lock(&gcov_lock);
	gcov_events_enabled = 1;
	 
#if defined (MY_DEF_HERE)
	while ((info = gcov_info_next(info)))
		gcov_event(GCOV_ADD, info);
#else  
	for (info = gcov_info_head; info; info = info->next)
		gcov_event(GCOV_ADD, info);
#endif  
	mutex_unlock(&gcov_lock);
}

#ifdef CONFIG_MODULES
static inline int within(void *addr, void *start, unsigned long size)
{
	return ((addr >= start) && (addr < start + size));
}

static int gcov_module_notifier(struct notifier_block *nb, unsigned long event,
				void *data)
{
	struct module *mod = data;
#if defined (MY_DEF_HERE)
	struct gcov_info *info = NULL;
	struct gcov_info *prev = NULL;
#else  
	struct gcov_info *info;
	struct gcov_info *prev;
#endif  

	if (event != MODULE_STATE_GOING)
		return NOTIFY_OK;
	mutex_lock(&gcov_lock);
#if defined (MY_DEF_HERE)
	 
	while ((info = gcov_info_next(info))) {
		if (within(info, mod->module_core, mod->core_size)) {
			gcov_info_unlink(prev, info);
			if (gcov_events_enabled)
				gcov_event(GCOV_REMOVE, info);
		} else
			prev = info;
	}
#else  
	prev = NULL;
	 
	for (info = gcov_info_head; info; info = info->next) {
		if (within(info, mod->module_core, mod->core_size)) {
			if (prev)
				prev->next = info->next;
			else
				gcov_info_head = info->next;
			if (gcov_events_enabled)
				gcov_event(GCOV_REMOVE, info);
		} else
			prev = info;
	}
#endif  
	mutex_unlock(&gcov_lock);

	return NOTIFY_OK;
}

static struct notifier_block gcov_nb = {
	.notifier_call	= gcov_module_notifier,
};

static int __init gcov_init(void)
{
	return register_module_notifier(&gcov_nb);
}
device_initcall(gcov_init);
#endif  
