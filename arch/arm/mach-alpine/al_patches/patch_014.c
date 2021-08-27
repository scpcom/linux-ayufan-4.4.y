#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_014_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL ETH: Link Management: Add support for using retimer for the 10G-serial ports.\n");
}

static struct kobj_attribute patch_014_attr =
	__ATTR(patch_014, S_IRUGO, patch_014_show, NULL);

static int __init al_patch_014(void)
{
	al_patches_add(&patch_014_attr.attr);

	return 0;

}

__initcall(al_patch_014);
