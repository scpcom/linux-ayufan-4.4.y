#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_013_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL ETH: fix LED configuration\n");
}

static struct kobj_attribute patch_013_attr =
	__ATTR(patch_013, S_IRUGO, patch_013_show, NULL);

static int __init al_patch_013(void)
{
	al_patches_add(&patch_013_attr.attr);

	return 0;

}

__initcall(al_patch_013);
