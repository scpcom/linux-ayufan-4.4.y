#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_017_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "xHCI kernel driver bug fix: coherent DMA mask is not set in LPAE/64-bit environment\n");
}

static struct kobj_attribute patch_017_attr =
	__ATTR(patch_017, S_IRUGO, patch_017_show, NULL);

static int __init al_patch_017(void)
{
	al_patches_add(&patch_017_attr.attr);

	return 0;

}

__initcall(al_patch_017);
