#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_022_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "mm: allow arch code to control the user page table ceiling\n");
}

static struct kobj_attribute patch_022_attr =
	__ATTR(patch_022, S_IRUGO, patch_022_show, NULL);

static int __init al_patch_022(void)
{
	al_patches_add(&patch_022_attr.attr);

	return 0;

}

__initcall(al_patch_022);
