#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_011_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL DT: update alpine DB device tree\n");
}

static struct kobj_attribute patch_011_attr =
	__ATTR(patch_011, S_IRUGO, patch_011_show, NULL);

static int __init al_patch_011(void)
{
	al_patches_add(&patch_011_attr.attr);

	return 0;

}

__initcall(al_patch_011);
