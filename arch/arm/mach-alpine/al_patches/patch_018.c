#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_018_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL RMN: Implement and enable AL RMN 1010\n");
}

static struct kobj_attribute patch_018_attr =
	__ATTR(patch_018, S_IRUGO, patch_018_show, NULL);

static int __init al_patch_018(void)
{
	al_patches_add(&patch_018_attr.attr);

	return 0;

}

__initcall(al_patch_018);
