#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <asm/page.h>

#include "al_patches_main.h"

static ssize_t patch_009_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *page)
{
	return snprintf(page, PAGE_SIZE, "net: phy: enable phy delay without condition for atheros 803x\n");
}

static struct kobj_attribute patch_009_attr =
	__ATTR(patch_009, S_IRUGO, patch_009_show, NULL);

static int __init al_patch_009(void)
{
	al_patches_add(&patch_009_attr.attr);

	return 0;

}

__initcall(al_patch_009);
