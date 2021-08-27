#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_019_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "Makefile: Change version to 2.5.3\n");
}

static struct kobj_attribute patch_019_attr =
	__ATTR(patch_019, S_IRUGO, patch_019_show, NULL);

static int __init al_patch_019(void)
{
	al_patches_add(&patch_019_attr.attr);

	return 0;

}

__initcall(al_patch_019);
