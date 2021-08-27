#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_004_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "al_eth [Bug Fix]: add nulity check in case of wrong i2c bus id\n");
}

static struct kobj_attribute patch_004_attr =
	__ATTR(patch_004, S_IRUGO, patch_004_show, NULL);

static int __init al_patch_004(void)
{
	al_patches_add(&patch_004_attr.attr);

	return 0;

}

__initcall(al_patch_004);
