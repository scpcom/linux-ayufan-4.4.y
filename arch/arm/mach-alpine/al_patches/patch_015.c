#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_015_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "al eth: LM: hard reset to serdes only in case of link training failures\n");
}

static struct kobj_attribute patch_015_attr =
	__ATTR(patch_015, S_IRUGO, patch_015_show, NULL);

static int __init al_patch_015(void)
{
	al_patches_add(&patch_015_attr.attr);

	return 0;

}

__initcall(al_patch_015);
