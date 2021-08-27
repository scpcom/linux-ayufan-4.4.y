#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_012_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "Makefile: Version 2.5.2\n");
}

static struct kobj_attribute patch_012_attr =
	__ATTR(patch_012, S_IRUGO, patch_012_show, NULL);

static int __init al_patch_012(void)
{
	al_patches_add(&patch_012_attr.attr);

	return 0;

}

__initcall(al_patch_012);
