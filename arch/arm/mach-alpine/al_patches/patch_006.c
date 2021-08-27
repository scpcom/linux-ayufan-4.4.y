#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_006_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL DMA [Bug Fix]: fix a rare situation of data corruption in RAID 5/6 recovery.\n");
}

static struct kobj_attribute patch_006_attr =
	__ATTR(patch_006, S_IRUGO, patch_006_show, NULL);

static int __init al_patch_006(void)
{
	al_patches_add(&patch_006_attr.attr);

	return 0;

}

__initcall(al_patch_006);
