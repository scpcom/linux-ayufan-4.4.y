/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/device.h>
#include <linux/stat.h>
#include <linux/sysfs.h>

#include "al_patches_main.h"

#define AL_MAX_NUM_OF_PATCHES	500

static struct attribute	*al_attrs[AL_MAX_NUM_OF_PATCHES];
static uint32_t		current_attr = 0;
static struct kset	*al_kset;

void al_patches_add(struct attribute *attr)
{
	if (current_attr == (AL_MAX_NUM_OF_PATCHES - 1)) {
		pr_warn("%s: too many patches\n", __func__);
		return;
	}

	al_attrs[current_attr] = attr;
	current_attr++;
}

static struct attribute_group al_attr_group = {
	.attrs = al_attrs,
};

static int __init al_patches(void)
{
	int rc = 0;

	al_kset = kset_create_and_add("al_patches", NULL, NULL);
	if (!al_kset)
		return -ENOMEM;

	if (current_attr > 0) {
		rc = sysfs_create_group(&al_kset->kobj, &al_attr_group);
		if (rc)
			kset_unregister(al_kset);
	}

	return rc;
}

late_initcall(al_patches);