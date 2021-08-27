#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_023_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "ARM: 7503/1: mm: only flush both pmd entries for classic MMU LPAE does not use two pmd entries for a pte, so the additional tlb flushing is not required.\n");
}

static struct kobj_attribute patch_023_attr =
	__ATTR(patch_023, S_IRUGO, patch_023_show, NULL);

static int __init al_patch_023(void)
{
	al_patches_add(&patch_023_attr.attr);

	return 0;

}

__initcall(al_patch_023);
