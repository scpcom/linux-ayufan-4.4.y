#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_020_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "ARM: 7687/1: atomics: don't use exclusives for atomic64 read/set with LPAE\n");
}

static struct kobj_attribute patch_020_attr =
	__ATTR(patch_020, S_IRUGO, patch_020_show, NULL);

static int __init al_patch_020(void)
{
	al_patches_add(&patch_020_attr.attr);

	return 0;

}

__initcall(al_patch_020);
