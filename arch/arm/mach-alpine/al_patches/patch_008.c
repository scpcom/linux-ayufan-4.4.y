#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_008_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL eth [API Change]: Link management\n");
}

static struct kobj_attribute patch_008_attr =
	__ATTR(patch_008, S_IRUGO, patch_008_show, NULL);

static int __init al_patch_008(void)
{
	al_patches_add(&patch_008_attr.attr);

	return 0;

}

__initcall(al_patch_008);
