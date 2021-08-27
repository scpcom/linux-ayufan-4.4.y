#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_025_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "printk console: fixing potentially endless loop\n");
}

static struct kobj_attribute patch_025_attr =
	__ATTR(patch_025, S_IRUGO, patch_025_show, NULL);

static int __init al_patch_025(void)
{
	al_patches_add(&patch_025_attr.attr);

	return 0;

}

__initcall(al_patch_025);
