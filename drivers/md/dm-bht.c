 /*
 * Copyright (C) 2011 The Chromium OS Authors <chromium-os-dev chromium org>
 *
 * Device-Mapper block hash tree interface.
 * See Documentation/device-mapper/dm-bht.txt for details.
 *
 * This file is released under the GPLv2.
 */

#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/cpumask.h>
#include <linux/device-mapper.h>
#include <linux/dm-bht.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm_types.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DM_MSG_PREFIX "dm bht"


/*
 * Utilities
 */

static u8 from_hex(u8 ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	if ((ch >= 'A') && (ch <= 'F'))
		return ch - 'A' + 10;
	return -1;
}

/**
 * dm_bht_bin_to_hex - converts a binary stream to human-readable hex
 * @binary:	a byte array of length @binary_len
 * @hex:	a byte array of length @binary_len * 2 + 1
 */
static void dm_bht_bin_to_hex(u8 *binary, u8 *hex, unsigned int binary_len)
{
	while (binary_len-- > 0) {
		sprintf((char *)hex, "%02hhx", (int)*binary);
		hex += 2;
		binary++;
	}
}

/**
 * dm_bht_hex_to_bin - converts a hex stream to binary
 * @binary:	a byte array of length @binary_len
 * @hex:	a byte array of length @binary_len * 2 + 1
 */
static void dm_bht_hex_to_bin(u8 *binary, const u8 *hex,
			      unsigned int binary_len)
{
	while (binary_len-- > 0) {
		*binary = from_hex(*(hex++));
		*binary *= 16;
		*binary += from_hex(*(hex++));
		binary++;
	}
}

static void dm_bht_log_mismatch(struct dm_bht *bht, u8 *given, u8 *computed)
{
	u8 given_hex[DM_BHT_MAX_DIGEST_SIZE * 2 + 1];
	u8 computed_hex[DM_BHT_MAX_DIGEST_SIZE * 2 + 1];

	dm_bht_bin_to_hex(given, given_hex, bht->digest_size);
	dm_bht_bin_to_hex(computed, computed_hex, bht->digest_size);
	DMERR_LIMIT("%s != %s", given_hex, computed_hex);
}

/**
 * dm_bht_compute_hash: hashes a page of data
 */
static int dm_bht_compute_hash(struct dm_bht *bht, struct page *pg,
			       unsigned int offset, u8 *digest)
{
	struct hash_desc *hash_desc = &bht->hash_desc[smp_processor_id()];
	struct scatterlist sg;

	sg_init_table(&sg, 1);
	sg_set_page(&sg, pg, bht->block_size, offset);
	/* Note, this is synchronous. */
	if (crypto_hash_init(hash_desc)) {
		DMCRIT("failed to reinitialize crypto hash (proc:%d)",
			smp_processor_id());
		return -EINVAL;
	}
	if (crypto_hash_update(hash_desc, &sg, bht->block_size)) {
		DMCRIT("crypto_hash_update failed");
		return -EINVAL;
	}
	sg_set_buf(&sg, bht->salt, sizeof(bht->salt));
	if (crypto_hash_update(hash_desc, &sg, sizeof(bht->salt))) {
		DMCRIT("crypto_hash_update failed");
		return -EINVAL;
	}
	if (crypto_hash_final(hash_desc, digest)) {
		DMCRIT("crypto_hash_final failed");
		return -EINVAL;
	}

	return 0;
}

/*
 * Implementation functions
 */

static int dm_bht_initialize_entries(struct dm_bht *bht)
{
	/* last represents the index of the last digest store in the tree.
	 * By walking the tree with that index, it is possible to compute the
	 * total number of entries at each level.
	 *
	 * Since each entry will contain up to |node_count| nodes of the tree,
	 * it is possible that the last index may not be at the end of a given
	 * entry->nodes.  In that case, it is assumed the value is padded.
	 *
	 * Note, we treat both the tree root (1 hash) and the tree leaves
	 * independently from the bht data structures.  Logically, the root is
	 * depth=-1 and the block layer level is depth=bht->depth
	 */
	unsigned int last = bht->block_count;
	int depth;

	/* check that the largest level->count can't result in an int overflow
	 * on allocation or sector calculation.
	 */
	if (((last >> bht->node_count_shift) + 1) >
	    UINT_MAX / max((unsigned int)sizeof(struct dm_bht_entry),
			   (unsigned int)to_sector(bht->block_size))) {
		DMCRIT("required entries %u is too large", last + 1);
		return -EINVAL;
	}

	/* Track the current sector location for each level so we don't have to
	 * compute it during traversals.
	 */
	bht->sectors = 0;
	for (depth = 0; depth < bht->depth; ++depth) {
		struct dm_bht_level *level = &bht->levels[depth];

		level->count = dm_bht_index_at_level(bht, depth, last) + 1;
		level->entries = (struct dm_bht_entry *)
				 kcalloc(level->count,
					 sizeof(struct dm_bht_entry),
					 GFP_KERNEL);
		if (!level->entries) {
			DMERR("failed to allocate entries for depth %d", depth);
			return -ENOMEM;
		}
		level->sector = bht->sectors;
		bht->sectors += level->count * to_sector(bht->block_size);
	}

	return 0;
}

/**
 * dm_bht_create - prepares @bht for us
 * @bht:	pointer to a dm_bht_create()d bht
 * @depth:	tree depth without the root; including block hashes
 * @block_count:the number of block hashes / tree leaves
 * @alg_name:	crypto hash algorithm name
 *
 * Returns 0 on success.
 *
 * Callers can offset into devices by storing the data in the io callbacks.
 */
int dm_bht_create(struct dm_bht *bht, unsigned int block_count,
		  unsigned int block_size, const char *alg_name)
{
	int cpu, status;

	bht->block_size = block_size;
	/* Verify that PAGE_SIZE >= block_size >= SECTOR_SIZE. */
	if ((block_size > PAGE_SIZE) ||
	    (PAGE_SIZE % block_size) ||
	    (to_sector(block_size) == 0))
		return -EINVAL;

	/* Setup the hash first. Its length determines much of the bht layout */
	for (cpu = 0; cpu < nr_cpu_ids; ++cpu) {
		bht->hash_desc[cpu].tfm = crypto_alloc_hash(alg_name, 0, 0);
		if (IS_ERR(bht->hash_desc[cpu].tfm)) {
			DMERR("failed to allocate crypto hash '%s'", alg_name);
			status = -ENOMEM;
			bht->hash_desc[cpu].tfm = NULL;
			goto bad_arg;
		}
	}
	bht->digest_size = crypto_hash_digestsize(bht->hash_desc[0].tfm);
	/* We expect to be able to pack >=2 hashes into a block */
	if (block_size / bht->digest_size < 2) {
		DMERR("too few hashes fit in a block");
		status = -EINVAL;
		goto bad_arg;
	}

	if (bht->digest_size > DM_BHT_MAX_DIGEST_SIZE) {
		DMERR("DM_BHT_MAX_DIGEST_SIZE too small for chosen digest");
		status = -EINVAL;
		goto bad_arg;
	}

	/* Configure the tree */
	bht->block_count = block_count;
	if (block_count == 0) {
		DMERR("block_count must be non-zero");
		status = -EINVAL;
		goto bad_arg;
	}

	/* Each dm_bht_entry->nodes is one block.  The node code tracks
	 * how many nodes fit into one entry where a node is a single
	 * hash (message digest).
	 */
	bht->node_count_shift = fls(block_size / bht->digest_size) - 1;
	/* Round down to the nearest power of two.  This makes indexing
	 * into the tree much less painful.
	 */
	bht->node_count = 1 << bht->node_count_shift;

	/* This is unlikely to happen, but with 64k pages, who knows. */
	if (bht->node_count > UINT_MAX / bht->digest_size) {
		DMERR("node_count * hash_len exceeds UINT_MAX!");
		status = -EINVAL;
		goto bad_arg;
	}

	bht->depth = DIV_ROUND_UP(fls(block_count - 1), bht->node_count_shift);

	/* Ensure that we can safely shift by this value. */
	if (bht->depth * bht->node_count_shift >= sizeof(unsigned int) * 8) {
		DMERR("specified depth and node_count_shift is too large");
		status = -EINVAL;
		goto bad_arg;
	}

	/* Allocate levels. Each level of the tree may have an arbitrary number
	 * of dm_bht_entry structs.  Each entry contains node_count nodes.
	 * Each node in the tree is a cryptographic digest of either node_count
	 * nodes on the subsequent level or of a specific block on disk.
	 */
	bht->levels = (struct dm_bht_level *)
			kcalloc(bht->depth,
				sizeof(struct dm_bht_level), GFP_KERNEL);
	if (!bht->levels) {
		DMERR("failed to allocate tree levels");
		status = -ENOMEM;
		goto bad_level_alloc;
	}

	bht->read_cb = NULL;

	status = dm_bht_initialize_entries(bht);
	if (status)
		goto bad_entries_alloc;

	/* We compute depth such that there is only be 1 block at level 0. */
	BUG_ON(bht->levels[0].count != 1);

	return 0;

bad_entries_alloc:
	while (bht->depth-- > 0)
		kfree(bht->levels[bht->depth].entries);
	kfree(bht->levels);
bad_level_alloc:
bad_arg:
	for (cpu = 0; cpu < nr_cpu_ids; ++cpu)
		if (bht->hash_desc[cpu].tfm)
			crypto_free_hash(bht->hash_desc[cpu].tfm);
	return status;
}
EXPORT_SYMBOL(dm_bht_create);

/**
 * dm_bht_read_completed
 * @entry:	pointer to the entry that's been loaded
 * @status:	I/O status. Non-zero is failure.
 * MUST always be called after a read_cb completes.
 */
void dm_bht_read_completed(struct dm_bht_entry *entry, int status)
{
	if (status) {
		/* TODO(wad) add retry support */
		DMCRIT("an I/O error occurred while reading entry");
		atomic_set(&entry->state, DM_BHT_ENTRY_ERROR_IO);
		/* entry->nodes will be freed later */
		return;
	}
	BUG_ON(atomic_read(&entry->state) != DM_BHT_ENTRY_PENDING);
	atomic_set(&entry->state, DM_BHT_ENTRY_READY);
}
EXPORT_SYMBOL(dm_bht_read_completed);

/**
 * dm_bht_verify_block - checks that all nodes in the path for @block are valid
 * @bht:	pointer to a dm_bht_create()d bht
 * @block:	specific block data is expected from
 * @pg:		page holding the block data
 * @offset:	offset into the page
 *
 * Returns 0 on success, DM_BHT_ENTRY_ERROR_MISMATCH on error.
 */
int dm_bht_verify_block(struct dm_bht *bht, unsigned int block,
			struct page *pg, unsigned int offset)
{
	int state, depth = bht->depth;
	u8 digest[DM_BHT_MAX_DIGEST_SIZE];
	struct dm_bht_entry *entry;
	void *node;

	do {
		/* Need to check that the hash of the current block is accurate
		 * in its parent.
		 */
		entry = dm_bht_get_entry(bht, depth - 1, block);
		state = atomic_read(&entry->state);
		/* This call is only safe if all nodes along the path
		 * are already populated (i.e. READY) via dm_bht_populate.
		 */
		BUG_ON(state < DM_BHT_ENTRY_READY);
		node = dm_bht_get_node(bht, entry, depth, block);

		if (dm_bht_compute_hash(bht, pg, offset, digest) ||
		    memcmp(digest, node, bht->digest_size))
			goto mismatch;

		/* Keep the containing block of hashes to be verified in the
		 * next pass.
		 */
		pg = virt_to_page(entry->nodes);
		offset = offset_in_page(entry->nodes);
	} while (--depth > 0 && state != DM_BHT_ENTRY_VERIFIED);

	if (depth == 0 && state != DM_BHT_ENTRY_VERIFIED) {
		if (dm_bht_compute_hash(bht, pg, offset, digest) ||
		    memcmp(digest, bht->root_digest, bht->digest_size))
			goto mismatch;
		atomic_set(&entry->state, DM_BHT_ENTRY_VERIFIED);
	}

	/* Mark path to leaf as verified. */
	for (depth++; depth < bht->depth; depth++) {
		entry = dm_bht_get_entry(bht, depth, block);
		/* At this point, entry can only be in VERIFIED or READY state.
		 * So it is safe to use atomic_set instead of atomic_cmpxchg.
		 */
		atomic_set(&entry->state, DM_BHT_ENTRY_VERIFIED);
	}

	return 0;

mismatch:
	DMERR_LIMIT("verify_path: failed to verify hash (d=%d,bi=%u)",
		    depth, block);
	dm_bht_log_mismatch(bht, node, digest);
	return DM_BHT_ENTRY_ERROR_MISMATCH;
}
EXPORT_SYMBOL(dm_bht_verify_block);

/**
 * dm_bht_is_populated - check that entries from disk needed to verify a given
 *                       block are all ready
 * @bht:	pointer to a dm_bht_create()d bht
 * @block:	specific block data is expected from
 *
 * Callers may wish to call dm_bht_is_populated() when checking an io
 * for which entries were already pending.
 */
bool dm_bht_is_populated(struct dm_bht *bht, unsigned int block)
{
	int depth;

	for (depth = bht->depth - 1; depth >= 0; depth--) {
		struct dm_bht_entry *entry = dm_bht_get_entry(bht, depth,
							      block);
		if (atomic_read(&entry->state) < DM_BHT_ENTRY_READY)
			return false;
	}

	return true;
}
EXPORT_SYMBOL(dm_bht_is_populated);

/**
 * dm_bht_populate - reads entries from disk needed to verify a given block
 * @bht:	pointer to a dm_bht_create()d bht
 * @ctx:        context used for all read_cb calls on this request
 * @block:	specific block data is expected from
 *
 * Returns negative value on error. Returns 0 on success.
 */
int dm_bht_populate(struct dm_bht *bht, void *ctx, unsigned int block)
{
	int depth, state;

	BUG_ON(block >= bht->block_count);

	for (depth = bht->depth - 1; depth >= 0; --depth) {
		unsigned int index = dm_bht_index_at_level(bht, depth, block);
		struct dm_bht_level *level = &bht->levels[depth];
		struct dm_bht_entry *entry = dm_bht_get_entry(bht, depth,
							      block);
		state = atomic_cmpxchg(&entry->state,
				       DM_BHT_ENTRY_UNALLOCATED,
				       DM_BHT_ENTRY_PENDING);
		if (state == DM_BHT_ENTRY_VERIFIED)
			break;
		if (state <= DM_BHT_ENTRY_ERROR)
			goto error_state;
		if (state != DM_BHT_ENTRY_UNALLOCATED)
			continue;

		/* Current entry is claimed for allocation and loading */
		entry->nodes = kmalloc(bht->block_size, GFP_NOIO);
		if (!entry->nodes)
			goto nomem;

		bht->read_cb(ctx,
			     level->sector + to_sector(index * bht->block_size),
			     entry->nodes, to_sector(bht->block_size), entry);
	}

	return 0;

error_state:
	DMCRIT("block %u at depth %d is in an error state", block, depth);
	return -EPERM;

nomem:
	DMCRIT("failed to allocate memory for entry->nodes");
	return -ENOMEM;
}
EXPORT_SYMBOL(dm_bht_populate);

/**
 * dm_bht_destroy - cleans up all memory used by @bht
 * @bht:	pointer to a dm_bht_create()d bht
 */
void dm_bht_destroy(struct dm_bht *bht)
{
	int depth, cpu;

	for (depth = 0; depth < bht->depth; depth++) {
		struct dm_bht_entry *entry = bht->levels[depth].entries;
		struct dm_bht_entry *entry_end = entry +
						 bht->levels[depth].count;
		for (; entry < entry_end; ++entry)
			kfree(entry->nodes);
		kfree(bht->levels[depth].entries);
	}
	kfree(bht->levels);
	for (cpu = 0; cpu < nr_cpu_ids; ++cpu)
		if (bht->hash_desc[cpu].tfm)
			crypto_free_hash(bht->hash_desc[cpu].tfm);
}
EXPORT_SYMBOL(dm_bht_destroy);

/*
 * Accessors
 */

/**
 * dm_bht_set_root_hexdigest - sets an unverified root digest hash from hex
 * @bht:	pointer to a dm_bht_create()d bht
 * @hexdigest:	array of u8s containing the new digest in binary
 * Returns non-zero on error.  hexdigest should be NUL terminated.
 */
int dm_bht_set_root_hexdigest(struct dm_bht *bht, const u8 *hexdigest)
{
	/* Make sure we have at least the bytes expected */
	if (strnlen((char *)hexdigest, bht->digest_size * 2) !=
	    bht->digest_size * 2) {
		DMERR("root digest length does not match hash algorithm");
		return -1;
	}
	dm_bht_hex_to_bin(bht->root_digest, hexdigest, bht->digest_size);
	return 0;
}
EXPORT_SYMBOL(dm_bht_set_root_hexdigest);

/**
 * dm_bht_root_hexdigest - returns root digest in hex
 * @bht:	pointer to a dm_bht_create()d bht
 * @hexdigest:	u8 array of size @available
 * @available:	must be bht->digest_size * 2 + 1
 */
int dm_bht_root_hexdigest(struct dm_bht *bht, u8 *hexdigest, int available)
{
	if (available < 0 ||
	    ((unsigned int) available) < bht->digest_size * 2 + 1) {
		DMERR("hexdigest has too few bytes available");
		return -EINVAL;
	}
	dm_bht_bin_to_hex(bht->root_digest, hexdigest, bht->digest_size);
	return 0;
}
EXPORT_SYMBOL(dm_bht_root_hexdigest);

/**
 * dm_bht_set_salt - sets the salt used, in hex
 * @bht:      pointer to a dm_bht_create()d bht
 * @hexsalt:  salt string, as hex; will be zero-padded or truncated to
 *            DM_BHT_SALT_SIZE * 2 hex digits.
 */
void dm_bht_set_salt(struct dm_bht *bht, const char *hexsalt)
{
	size_t saltlen = min(strlen(hexsalt) / 2, sizeof(bht->salt));

	memset(bht->salt, 0, sizeof(bht->salt));
	dm_bht_hex_to_bin(bht->salt, (const u8 *)hexsalt, saltlen);
}
EXPORT_SYMBOL(dm_bht_set_salt);

/**
 * dm_bht_salt - returns the salt used, in hex
 * @bht:      pointer to a dm_bht_create()d bht
 * @hexsalt:  buffer to put salt into, of length DM_BHT_SALT_SIZE * 2 + 1.
 */
int dm_bht_salt(struct dm_bht *bht, char *hexsalt)
{
	dm_bht_bin_to_hex(bht->salt, (u8 *)hexsalt, sizeof(bht->salt));
	return 0;
}
EXPORT_SYMBOL(dm_bht_salt);

