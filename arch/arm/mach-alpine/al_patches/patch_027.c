#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_027_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "Kernel spin lock: disable WFE/SEV in spin lock\n");
}

static struct kobj_attribute patch_027_attr =
	__ATTR(patch_027, S_IRUGO, patch_027_show, NULL);

static int __init al_patch_027(void)
{
	al_patches_add(&patch_027_attr.attr);

	return 0;

}

__initcall(al_patch_027);
