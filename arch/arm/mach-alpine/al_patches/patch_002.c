#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_002_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "al eth: add alpine A0 support.\n");
}

static struct kobj_attribute patch_002_attr =
	__ATTR(patch_002, S_IRUGO, patch_002_show, NULL);

static int __init al_patch_002(void)
{
	al_patches_add(&patch_002_attr.attr);

	return 0;

}

__initcall(al_patch_002);
