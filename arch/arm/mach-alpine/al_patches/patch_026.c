#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_026_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "AL thermal [bug fix]: updated temperature readout coefficients\n");
}

static struct kobj_attribute patch_026_attr =
	__ATTR(patch_026, S_IRUGO, patch_026_show, NULL);

static int __init al_patch_026(void)
{
	al_patches_add(&patch_026_attr.attr);

	return 0;

}

__initcall(al_patch_026);
