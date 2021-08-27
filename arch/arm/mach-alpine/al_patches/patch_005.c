#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_005_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL DT: change mdc-mdio default frequency to 1 MHz\n");
}

static struct kobj_attribute patch_005_attr =
	__ATTR(patch_005, S_IRUGO, patch_005_show, NULL);

static int __init al_patch_005(void)
{
	al_patches_add(&patch_005_attr.attr);

	return 0;

}

__initcall(al_patch_005);
