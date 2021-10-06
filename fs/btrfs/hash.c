#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Copyright (C) 2014 Filipe David Borba Manana <fdmanana@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <crypto/hash.h>
#include <linux/err.h>
#include "hash.h"
#ifdef MY_ABC_HERE
#include "ctree.h"
#include <linux/fs.h>
#endif /* MY_ABC_HERE */

static struct crypto_shash *tfm;

int __init btrfs_hash_init(void)
{
	tfm = crypto_alloc_shash("crc32c", 0, 0);

	return PTR_ERR_OR_ZERO(tfm);
}

const char* btrfs_crc32c_impl(void)
{
	return crypto_tfm_alg_driver_name(crypto_shash_tfm(tfm));
}

void btrfs_hash_exit(void)
{
	crypto_free_shash(tfm);
}

u32 btrfs_crc32c(u32 crc, const void *address, unsigned int length)
{
	SHASH_DESC_ON_STACK(shash, tfm);
	u32 *ctx = (u32 *)shash_desc_ctx(shash);
	int err;

	shash->tfm = tfm;
	shash->flags = 0;
	*ctx = crc;

	err = crypto_shash_update(shash, address, length);
	BUG_ON(err);

	return *ctx;
}

#ifdef MY_ABC_HERE
int btrfs_upper_name_hash(const char *name, int len, u32 *hash)
{
	/*
	 * hash_buf need to add 1 byte for syno_utf8_toupper,
	 * because it will append 0 to last byte.
	 */
	char hash_buf[BTRFS_NAME_LEN+1];
	unsigned int upperlen;

	if (len > BTRFS_NAME_LEN) {
		return -ENAMETOOLONG;
	}

	upperlen = syno_utf8_toupper(hash_buf, name, BTRFS_NAME_LEN, len, NULL);
	*hash = btrfs_crc32c((u32)~1, hash_buf, upperlen);
	return 0;
}
#endif /* MY_ABC_HERE */
