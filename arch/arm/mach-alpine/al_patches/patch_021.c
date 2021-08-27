#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_021_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "arm: set the page table freeing ceiling to TASK_SIZE\n");
}

static struct kobj_attribute patch_021_attr =
	__ATTR(patch_021, S_IRUGO, patch_021_show, NULL);

static int __init al_patch_021(void)
{
	al_patches_add(&patch_021_attr.attr);

	return 0;

}

__initcall(al_patch_021);
