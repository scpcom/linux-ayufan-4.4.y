/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 * Author: kedong@google.com (Ke Dong)
 *
 * This file is released under the GPLv2.
 *
 * Implements a BRUNO platform specific error handler.
 */
#include <linux/err.h>
#include <linux/genhd.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/device-mapper.h>
#include <asm/page.h>

#include "dm-verity.h"

#define DM_MSG_PREFIX "verity-bruno"

static int error_handler(struct notifier_block *nb, unsigned long transient,
			 void *opaque_err)
{
	struct dm_verity_error_state *err =
		(struct dm_verity_error_state *) opaque_err;
        u32 failure_count = 0;
	err->behavior = DM_VERITY_ERROR_BEHAVIOR_PANIC;
	if (transient)
		return 0;

	// TODO(jnewlin): Need to increment the error count for failing back
	// to the previous image.
	return 0;
}

static struct notifier_block bruno_nb = {
	.notifier_call = &error_handler,
	.next = NULL,
	.priority = 1,
};

static int __init dm_verity_bruno_init(void)
{
	int r;

	r = dm_verity_register_error_notifier(&bruno_nb);
	if (r < 0)
		DMERR("failed to register handler: %d", r);
	else
		DMINFO("registered");
	return r;
}

static void __exit dm_verity_bruno_exit(void)
{
	dm_verity_unregister_error_notifier(&bruno_nb);
}

module_init(dm_verity_bruno_init);
module_exit(dm_verity_bruno_exit);

MODULE_AUTHOR("Ke Dong <kedong@google.com>");
MODULE_DESCRIPTION("bruno-specific error handler for dm-verity");
MODULE_LICENSE("GPL");
