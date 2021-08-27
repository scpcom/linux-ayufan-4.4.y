#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_003_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "al nand [Bug Fix]: check oob instead of all page to recognise empty page\n");
}

static struct kobj_attribute patch_003_attr =
	__ATTR(patch_003, S_IRUGO, patch_003_show, NULL);

static int __init al_patch_003(void)
{
	al_patches_add(&patch_003_attr.attr);

	return 0;

}

__initcall(al_patch_003);
