#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_001_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "al alpine machine: add patch tracking mechanism\n");
}

static struct kobj_attribute patch_001_attr =
	__ATTR(patch_001, S_IRUGO, patch_001_show, NULL);

static int __init al_patch_001(void)
{
	al_patches_add(&patch_001_attr.attr);

	return 0;

}

__initcall(al_patch_001);
