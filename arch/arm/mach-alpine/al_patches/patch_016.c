#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_016_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "Fix overlap of FIXMAP area with DMA consistent memory area\n");
}

static struct kobj_attribute patch_016_attr =
	__ATTR(patch_016, S_IRUGO, patch_016_show, NULL);

static int __init al_patch_016(void)
{
	al_patches_add(&patch_016_attr.attr);

	return 0;

}

__initcall(al_patch_016);
