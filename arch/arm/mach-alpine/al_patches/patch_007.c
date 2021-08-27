#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_007_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL config: Added CONFIG_NLS_CODEPAGE_437 to support legacy formatted USB disks\n");
}

static struct kobj_attribute patch_007_attr =
	__ATTR(patch_007, S_IRUGO, patch_007_show, NULL);

static int __init al_patch_007(void)
{
	al_patches_add(&patch_007_attr.attr);

	return 0;

}

__initcall(al_patch_007);
