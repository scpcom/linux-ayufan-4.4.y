#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_024_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "ARM: 7953/1: mm: ensure TLB invalidation is complete before enabling MMU\n");
}

static struct kobj_attribute patch_024_attr =
	__ATTR(patch_024, S_IRUGO, patch_024_show, NULL);

static int __init al_patch_024(void)
{
	al_patches_add(&patch_024_attr.attr);

	return 0;

}

__initcall(al_patch_024);
