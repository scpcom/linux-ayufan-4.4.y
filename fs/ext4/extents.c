 
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/jbd2.h>
#include <linux/highuid.h>
#include <linux/pagemap.h>
#include <linux/quotaops.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/falloc.h>
#include <asm/uaccess.h>
#include <linux/fiemap.h>
#include "ext4_jbd2.h"
#include "ext4_extents.h"

ext4_fsblk_t ext_pblock(struct ext4_extent *ex)
{
	ext4_fsblk_t block;

	block = le32_to_cpu(ex->ee_start_lo);
	block |= ((ext4_fsblk_t) le16_to_cpu(ex->ee_start_hi) << 31) << 1;
	return block;
}

ext4_fsblk_t idx_pblock(struct ext4_extent_idx *ix)
{
	ext4_fsblk_t block;

	block = le32_to_cpu(ix->ei_leaf_lo);
	block |= ((ext4_fsblk_t) le16_to_cpu(ix->ei_leaf_hi) << 31) << 1;
	return block;
}

void ext4_ext_store_pblock(struct ext4_extent *ex, ext4_fsblk_t pb)
{
	ex->ee_start_lo = cpu_to_le32((unsigned long) (pb & 0xffffffff));
	ex->ee_start_hi = cpu_to_le16((unsigned long) ((pb >> 31) >> 1) & 0xffff);
}

static void ext4_idx_store_pblock(struct ext4_extent_idx *ix, ext4_fsblk_t pb)
{
	ix->ei_leaf_lo = cpu_to_le32((unsigned long) (pb & 0xffffffff));
	ix->ei_leaf_hi = cpu_to_le16((unsigned long) ((pb >> 31) >> 1) & 0xffff);
}

static int ext4_ext_truncate_extend_restart(handle_t *handle,
					    struct inode *inode,
					    int needed)
{
	int err;

	if (!ext4_handle_valid(handle))
		return 0;
	if (handle->h_buffer_credits > needed)
		return 0;
	err = ext4_journal_extend(handle, needed);
	if (err <= 0)
		return err;
	err = ext4_truncate_restart_trans(handle, inode, needed);
	if (err == 0)
		err = -EAGAIN;

	return err;
}

static int ext4_ext_get_access(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *path)
{
	if (path->p_bh) {
		 
		return ext4_journal_get_write_access(handle, path->p_bh);
	}
	 
	return 0;
}

static int ext4_ext_dirty(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *path)
{
	int err;
	if (path->p_bh) {
		 
		err = ext4_handle_dirty_metadata(handle, inode, path->p_bh);
	} else {
		 
		err = ext4_mark_inode_dirty(handle, inode);
	}
	return err;
}

static ext4_fsblk_t ext4_ext_find_goal(struct inode *inode,
			      struct ext4_ext_path *path,
			      ext4_lblk_t block)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	ext4_fsblk_t bg_start;
	ext4_fsblk_t last_block;
	ext4_grpblk_t colour;
	ext4_group_t block_group;
	int flex_size = ext4_flex_bg_size(EXT4_SB(inode->i_sb));
	int depth;

	if (path) {
		struct ext4_extent *ex;
		depth = path->p_depth;

		ex = path[depth].p_ext;
		if (ex)
			return ext_pblock(ex)+(block-le32_to_cpu(ex->ee_block));

		if (path[depth].p_bh)
			return path[depth].p_bh->b_blocknr;
	}

	block_group = ei->i_block_group;
	if (flex_size >= EXT4_FLEX_SIZE_DIR_ALLOC_SCHEME) {
		 
		block_group &= ~(flex_size-1);
		if (S_ISREG(inode->i_mode))
			block_group++;
	}
	bg_start = (block_group * EXT4_BLOCKS_PER_GROUP(inode->i_sb)) +
		le32_to_cpu(EXT4_SB(inode->i_sb)->s_es->s_first_data_block);
	last_block = ext4_blocks_count(EXT4_SB(inode->i_sb)->s_es) - 1;

	if (test_opt(inode->i_sb, DELALLOC))
		return bg_start;

	if (bg_start + EXT4_BLOCKS_PER_GROUP(inode->i_sb) <= last_block)
		colour = (current->pid % 16) *
			(EXT4_BLOCKS_PER_GROUP(inode->i_sb) / 16);
	else
		colour = (current->pid % 16) * ((last_block - bg_start) / 16);
	return bg_start + colour + block;
}

static ext4_fsblk_t
ext4_ext_new_meta_block(handle_t *handle, struct inode *inode,
			struct ext4_ext_path *path,
			struct ext4_extent *ex, int *err)
{
	ext4_fsblk_t goal, newblock;

	goal = ext4_ext_find_goal(inode, path, le32_to_cpu(ex->ee_block));
	newblock = ext4_new_meta_blocks(handle, inode, goal, NULL, err);
	return newblock;
}

static inline int ext4_ext_space_block(struct inode *inode, int check)
{
	int size;

	size = (inode->i_sb->s_blocksize - sizeof(struct ext4_extent_header))
			/ sizeof(struct ext4_extent);
	if (!check) {
#ifdef AGGRESSIVE_TEST
		if (size > 6)
			size = 6;
#endif
	}
	return size;
}

static inline int ext4_ext_space_block_idx(struct inode *inode, int check)
{
	int size;

	size = (inode->i_sb->s_blocksize - sizeof(struct ext4_extent_header))
			/ sizeof(struct ext4_extent_idx);
	if (!check) {
#ifdef AGGRESSIVE_TEST
		if (size > 5)
			size = 5;
#endif
	}
	return size;
}

static inline int ext4_ext_space_root(struct inode *inode, int check)
{
	int size;

	size = sizeof(EXT4_I(inode)->i_data);
	size -= sizeof(struct ext4_extent_header);
	size /= sizeof(struct ext4_extent);
	if (!check) {
#ifdef AGGRESSIVE_TEST
		if (size > 3)
			size = 3;
#endif
	}
	return size;
}

static inline int ext4_ext_space_root_idx(struct inode *inode, int check)
{
	int size;

	size = sizeof(EXT4_I(inode)->i_data);
	size -= sizeof(struct ext4_extent_header);
	size /= sizeof(struct ext4_extent_idx);
	if (!check) {
#ifdef AGGRESSIVE_TEST
		if (size > 4)
			size = 4;
#endif
	}
	return size;
}

int ext4_ext_calc_metadata_amount(struct inode *inode, sector_t lblock)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	int idxs, num = 0;

	idxs = ((inode->i_sb->s_blocksize - sizeof(struct ext4_extent_header))
		/ sizeof(struct ext4_extent_idx));

	if (ei->i_da_metadata_calc_len &&
	    ei->i_da_metadata_calc_last_lblock+1 == lblock) {
		if ((ei->i_da_metadata_calc_len % idxs) == 0)
			num++;
		if ((ei->i_da_metadata_calc_len % (idxs*idxs)) == 0)
			num++;
		if ((ei->i_da_metadata_calc_len % (idxs*idxs*idxs)) == 0) {
			num++;
			ei->i_da_metadata_calc_len = 0;
		} else
			ei->i_da_metadata_calc_len++;
		ei->i_da_metadata_calc_last_lblock++;
		return num;
	}

	ei->i_da_metadata_calc_len = 1;
	ei->i_da_metadata_calc_last_lblock = lblock;
	return ext_depth(inode) + 1;
}

static int
ext4_ext_max_entries(struct inode *inode, int depth)
{
	int max;

	if (depth == ext_depth(inode)) {
		if (depth == 0)
			max = ext4_ext_space_root(inode, 1);
		else
			max = ext4_ext_space_root_idx(inode, 1);
	} else {
		if (depth == 0)
			max = ext4_ext_space_block(inode, 1);
		else
			max = ext4_ext_space_block_idx(inode, 1);
	}

	return max;
}

static int ext4_valid_extent(struct inode *inode, struct ext4_extent *ext)
{
	ext4_fsblk_t block = ext_pblock(ext);
	int len = ext4_ext_get_actual_len(ext);

	return ext4_data_block_valid(EXT4_SB(inode->i_sb), block, len);
}

static int ext4_valid_extent_idx(struct inode *inode,
				struct ext4_extent_idx *ext_idx)
{
	ext4_fsblk_t block = idx_pblock(ext_idx);

	return ext4_data_block_valid(EXT4_SB(inode->i_sb), block, 1);
}

static int ext4_valid_extent_entries(struct inode *inode,
				struct ext4_extent_header *eh,
				int depth)
{
	struct ext4_extent *ext;
	struct ext4_extent_idx *ext_idx;
	unsigned short entries;
	if (eh->eh_entries == 0)
		return 1;

	entries = le16_to_cpu(eh->eh_entries);

	if (depth == 0) {
		 
		ext = EXT_FIRST_EXTENT(eh);
		while (entries) {
			if (!ext4_valid_extent(inode, ext))
				return 0;
			ext++;
			entries--;
		}
	} else {
		ext_idx = EXT_FIRST_INDEX(eh);
		while (entries) {
			if (!ext4_valid_extent_idx(inode, ext_idx))
				return 0;
			ext_idx++;
			entries--;
		}
	}
	return 1;
}

static int __ext4_ext_check(const char *function, struct inode *inode,
					struct ext4_extent_header *eh,
					int depth)
{
	const char *error_msg;
	int max = 0;

	if (unlikely(eh->eh_magic != EXT4_EXT_MAGIC)) {
		error_msg = "invalid magic";
		goto corrupted;
	}
	if (unlikely(le16_to_cpu(eh->eh_depth) != depth)) {
		error_msg = "unexpected eh_depth";
		goto corrupted;
	}
	if (unlikely(eh->eh_max == 0)) {
		error_msg = "invalid eh_max";
		goto corrupted;
	}
	max = ext4_ext_max_entries(inode, depth);
	if (unlikely(le16_to_cpu(eh->eh_max) > max)) {
		error_msg = "too large eh_max";
		goto corrupted;
	}
	if (unlikely(le16_to_cpu(eh->eh_entries) > le16_to_cpu(eh->eh_max))) {
		error_msg = "invalid eh_entries";
		goto corrupted;
	}
	if (!ext4_valid_extent_entries(inode, eh, depth)) {
		error_msg = "invalid extent entries";
		goto corrupted;
	}
	return 0;

corrupted:
	ext4_error(inode->i_sb, function,
			"bad header/extent in inode #%lu: %s - magic %x, "
			"entries %u, max %u(%u), depth %u(%u)",
			inode->i_ino, error_msg, le16_to_cpu(eh->eh_magic),
			le16_to_cpu(eh->eh_entries), le16_to_cpu(eh->eh_max),
			max, le16_to_cpu(eh->eh_depth), depth);

	return -EIO;
}

#define ext4_ext_check(inode, eh, depth)	\
	__ext4_ext_check(__func__, inode, eh, depth)

int ext4_ext_check_inode(struct inode *inode)
{
	return ext4_ext_check(inode, ext_inode_hdr(inode), ext_depth(inode));
}

#ifdef EXT_DEBUG
static void ext4_ext_show_path(struct inode *inode, struct ext4_ext_path *path)
{
	int k, l = path->p_depth;

	ext_debug("path:");
	for (k = 0; k <= l; k++, path++) {
		if (path->p_idx) {
		  ext_debug("  %d->%llu", le32_to_cpu(path->p_idx->ei_block),
			    idx_pblock(path->p_idx));
		} else if (path->p_ext) {
			ext_debug("  %d:[%d]%d:%llu ",
				  le32_to_cpu(path->p_ext->ee_block),
				  ext4_ext_is_uninitialized(path->p_ext),
				  ext4_ext_get_actual_len(path->p_ext),
				  ext_pblock(path->p_ext));
		} else
			ext_debug("  []");
	}
	ext_debug("\n");
}

static void ext4_ext_show_leaf(struct inode *inode, struct ext4_ext_path *path)
{
	int depth = ext_depth(inode);
	struct ext4_extent_header *eh;
	struct ext4_extent *ex;
	int i;

	if (!path)
		return;

	eh = path[depth].p_hdr;
	ex = EXT_FIRST_EXTENT(eh);

	ext_debug("Displaying leaf extents for inode %lu\n", inode->i_ino);

	for (i = 0; i < le16_to_cpu(eh->eh_entries); i++, ex++) {
		ext_debug("%d:[%d]%d:%llu ", le32_to_cpu(ex->ee_block),
			  ext4_ext_is_uninitialized(ex),
			  ext4_ext_get_actual_len(ex), ext_pblock(ex));
	}
	ext_debug("\n");
}
#else
#define ext4_ext_show_path(inode, path)
#define ext4_ext_show_leaf(inode, path)
#endif

void ext4_ext_drop_refs(struct ext4_ext_path *path)
{
	int depth = path->p_depth;
	int i;

	for (i = 0; i <= depth; i++, path++)
		if (path->p_bh) {
			brelse(path->p_bh);
			path->p_bh = NULL;
		}
}

static void
ext4_ext_binsearch_idx(struct inode *inode,
			struct ext4_ext_path *path, ext4_lblk_t block)
{
	struct ext4_extent_header *eh = path->p_hdr;
	struct ext4_extent_idx *r, *l, *m;

	ext_debug("binsearch for %u(idx):  ", block);

	l = EXT_FIRST_INDEX(eh) + 1;
	r = EXT_LAST_INDEX(eh);
	while (l <= r) {
		m = l + (r - l) / 2;
		if (block < le32_to_cpu(m->ei_block))
			r = m - 1;
		else
			l = m + 1;
		ext_debug("%p(%u):%p(%u):%p(%u) ", l, le32_to_cpu(l->ei_block),
				m, le32_to_cpu(m->ei_block),
				r, le32_to_cpu(r->ei_block));
	}

	path->p_idx = l - 1;
	ext_debug("  -> %d->%lld ", le32_to_cpu(path->p_idx->ei_block),
		  idx_pblock(path->p_idx));

#ifdef CHECK_BINSEARCH
	{
		struct ext4_extent_idx *chix, *ix;
		int k;

		chix = ix = EXT_FIRST_INDEX(eh);
		for (k = 0; k < le16_to_cpu(eh->eh_entries); k++, ix++) {
		  if (k != 0 &&
		      le32_to_cpu(ix->ei_block) <= le32_to_cpu(ix[-1].ei_block)) {
				printk(KERN_DEBUG "k=%d, ix=0x%p, "
				       "first=0x%p\n", k,
				       ix, EXT_FIRST_INDEX(eh));
				printk(KERN_DEBUG "%u <= %u\n",
				       le32_to_cpu(ix->ei_block),
				       le32_to_cpu(ix[-1].ei_block));
			}
			BUG_ON(k && le32_to_cpu(ix->ei_block)
					   <= le32_to_cpu(ix[-1].ei_block));
			if (block < le32_to_cpu(ix->ei_block))
				break;
			chix = ix;
		}
		BUG_ON(chix != path->p_idx);
	}
#endif

}

static void
ext4_ext_binsearch(struct inode *inode,
		struct ext4_ext_path *path, ext4_lblk_t block)
{
	struct ext4_extent_header *eh = path->p_hdr;
	struct ext4_extent *r, *l, *m;

	if (eh->eh_entries == 0) {
		 
		return;
	}

	ext_debug("binsearch for %u:  ", block);

	l = EXT_FIRST_EXTENT(eh) + 1;
	r = EXT_LAST_EXTENT(eh);

	while (l <= r) {
		m = l + (r - l) / 2;
		if (block < le32_to_cpu(m->ee_block))
			r = m - 1;
		else
			l = m + 1;
		ext_debug("%p(%u):%p(%u):%p(%u) ", l, le32_to_cpu(l->ee_block),
				m, le32_to_cpu(m->ee_block),
				r, le32_to_cpu(r->ee_block));
	}

	path->p_ext = l - 1;
	ext_debug("  -> %d:%llu:[%d]%d ",
			le32_to_cpu(path->p_ext->ee_block),
			ext_pblock(path->p_ext),
			ext4_ext_is_uninitialized(path->p_ext),
			ext4_ext_get_actual_len(path->p_ext));

#ifdef CHECK_BINSEARCH
	{
		struct ext4_extent *chex, *ex;
		int k;

		chex = ex = EXT_FIRST_EXTENT(eh);
		for (k = 0; k < le16_to_cpu(eh->eh_entries); k++, ex++) {
			BUG_ON(k && le32_to_cpu(ex->ee_block)
					  <= le32_to_cpu(ex[-1].ee_block));
			if (block < le32_to_cpu(ex->ee_block))
				break;
			chex = ex;
		}
		BUG_ON(chex != path->p_ext);
	}
#endif

}

int ext4_ext_tree_init(handle_t *handle, struct inode *inode)
{
	struct ext4_extent_header *eh;

	eh = ext_inode_hdr(inode);
	eh->eh_depth = 0;
	eh->eh_entries = 0;
	eh->eh_magic = EXT4_EXT_MAGIC;
	eh->eh_max = cpu_to_le16(ext4_ext_space_root(inode, 0));
	ext4_mark_inode_dirty(handle, inode);
	ext4_ext_invalidate_cache(inode);
	return 0;
}

struct ext4_ext_path *
ext4_ext_find_extent(struct inode *inode, ext4_lblk_t block,
					struct ext4_ext_path *path)
{
	struct ext4_extent_header *eh;
	struct buffer_head *bh;
	short int depth, i, ppos = 0, alloc = 0;

	eh = ext_inode_hdr(inode);
	depth = ext_depth(inode);

	if (!path) {
		path = kzalloc(sizeof(struct ext4_ext_path) * (depth + 2),
				GFP_NOFS);
		if (!path)
			return ERR_PTR(-ENOMEM);
		alloc = 1;
	}
	path[0].p_hdr = eh;
	path[0].p_bh = NULL;

	i = depth;
	 
	while (i) {
		int need_to_validate = 0;

		ext_debug("depth %d: num %d, max %d\n",
			  ppos, le16_to_cpu(eh->eh_entries), le16_to_cpu(eh->eh_max));

		ext4_ext_binsearch_idx(inode, path + ppos, block);
		path[ppos].p_block = idx_pblock(path[ppos].p_idx);
		path[ppos].p_depth = i;
		path[ppos].p_ext = NULL;

		bh = sb_getblk(inode->i_sb, path[ppos].p_block);
		if (unlikely(!bh))
			goto err;
		if (!bh_uptodate_or_lock(bh)) {
			if (bh_submit_read(bh) < 0) {
				put_bh(bh);
				goto err;
			}
			 
			need_to_validate = 1;
		}
		eh = ext_block_hdr(bh);
		ppos++;
		BUG_ON(ppos > depth);
		path[ppos].p_bh = bh;
		path[ppos].p_hdr = eh;
		i--;

		if (need_to_validate && ext4_ext_check(inode, eh, i))
			goto err;
	}

	path[ppos].p_depth = i;
	path[ppos].p_ext = NULL;
	path[ppos].p_idx = NULL;

	ext4_ext_binsearch(inode, path + ppos, block);
	 
	if (path[ppos].p_ext)
		path[ppos].p_block = ext_pblock(path[ppos].p_ext);

	ext4_ext_show_path(inode, path);

	return path;

err:
	ext4_ext_drop_refs(path);
	if (alloc)
		kfree(path);
	return ERR_PTR(-EIO);
}

int ext4_ext_insert_index(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *curp,
				int logical, ext4_fsblk_t ptr)
{
	struct ext4_extent_idx *ix;
	int len, err;

	err = ext4_ext_get_access(handle, inode, curp);
	if (err)
		return err;

	BUG_ON(logical == le32_to_cpu(curp->p_idx->ei_block));
	len = EXT_MAX_INDEX(curp->p_hdr) - curp->p_idx;
	if (logical > le32_to_cpu(curp->p_idx->ei_block)) {
		 
		if (curp->p_idx != EXT_LAST_INDEX(curp->p_hdr)) {
			len = (len - 1) * sizeof(struct ext4_extent_idx);
			len = len < 0 ? 0 : len;
			ext_debug("insert new index %d after: %llu. "
					"move %d from 0x%p to 0x%p\n",
					logical, ptr, len,
					(curp->p_idx + 1), (curp->p_idx + 2));
			memmove(curp->p_idx + 2, curp->p_idx + 1, len);
		}
		ix = curp->p_idx + 1;
	} else {
		 
		len = len * sizeof(struct ext4_extent_idx);
		len = len < 0 ? 0 : len;
		ext_debug("insert new index %d before: %llu. "
				"move %d from 0x%p to 0x%p\n",
				logical, ptr, len,
				curp->p_idx, (curp->p_idx + 1));
		memmove(curp->p_idx + 1, curp->p_idx, len);
		ix = curp->p_idx;
	}

	ix->ei_block = cpu_to_le32(logical);
	ext4_idx_store_pblock(ix, ptr);
	le16_add_cpu(&curp->p_hdr->eh_entries, 1);

	BUG_ON(le16_to_cpu(curp->p_hdr->eh_entries)
			     > le16_to_cpu(curp->p_hdr->eh_max));
	BUG_ON(ix > EXT_LAST_INDEX(curp->p_hdr));

	err = ext4_ext_dirty(handle, inode, curp);
	ext4_std_error(inode->i_sb, err);

	return err;
}

static int ext4_ext_split(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *path,
				struct ext4_extent *newext, int at)
{
	struct buffer_head *bh = NULL;
	int depth = ext_depth(inode);
	struct ext4_extent_header *neh;
	struct ext4_extent_idx *fidx;
	struct ext4_extent *ex;
	int i = at, k, m, a;
	ext4_fsblk_t newblock, oldblock;
	__le32 border;
	ext4_fsblk_t *ablocks = NULL;  
	int err = 0;

	BUG_ON(path[depth].p_ext > EXT_MAX_EXTENT(path[depth].p_hdr));
	if (path[depth].p_ext != EXT_MAX_EXTENT(path[depth].p_hdr)) {
		border = path[depth].p_ext[1].ee_block;
		ext_debug("leaf will be split."
				" next leaf starts at %d\n",
				  le32_to_cpu(border));
	} else {
		border = newext->ee_block;
		ext_debug("leaf will be added."
				" next leaf starts at %d\n",
				le32_to_cpu(border));
	}

	ablocks = kzalloc(sizeof(ext4_fsblk_t) * depth, GFP_NOFS);
	if (!ablocks)
		return -ENOMEM;

	ext_debug("allocate %d blocks for indexes/leaf\n", depth - at);
	for (a = 0; a < depth - at; a++) {
		newblock = ext4_ext_new_meta_block(handle, inode, path,
						   newext, &err);
		if (newblock == 0)
			goto cleanup;
		ablocks[a] = newblock;
	}

	newblock = ablocks[--a];
	BUG_ON(newblock == 0);
	bh = sb_getblk(inode->i_sb, newblock);
	if (!bh) {
		err = -EIO;
		goto cleanup;
	}
	lock_buffer(bh);

	err = ext4_journal_get_create_access(handle, bh);
	if (err)
		goto cleanup;

	neh = ext_block_hdr(bh);
	neh->eh_entries = 0;
	neh->eh_max = cpu_to_le16(ext4_ext_space_block(inode, 0));
	neh->eh_magic = EXT4_EXT_MAGIC;
	neh->eh_depth = 0;
	ex = EXT_FIRST_EXTENT(neh);

	BUG_ON(path[depth].p_hdr->eh_entries != path[depth].p_hdr->eh_max);
	 
	m = 0;
	path[depth].p_ext++;
	while (path[depth].p_ext <=
			EXT_MAX_EXTENT(path[depth].p_hdr)) {
		ext_debug("move %d:%llu:[%d]%d in new leaf %llu\n",
				le32_to_cpu(path[depth].p_ext->ee_block),
				ext_pblock(path[depth].p_ext),
				ext4_ext_is_uninitialized(path[depth].p_ext),
				ext4_ext_get_actual_len(path[depth].p_ext),
				newblock);
		 
		path[depth].p_ext++;
		m++;
	}
	if (m) {
		memmove(ex, path[depth].p_ext-m, sizeof(struct ext4_extent)*m);
		le16_add_cpu(&neh->eh_entries, m);
	}

	set_buffer_uptodate(bh);
	unlock_buffer(bh);

	err = ext4_handle_dirty_metadata(handle, inode, bh);
	if (err)
		goto cleanup;
	brelse(bh);
	bh = NULL;

	if (m) {
		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			goto cleanup;
		le16_add_cpu(&path[depth].p_hdr->eh_entries, -m);
		err = ext4_ext_dirty(handle, inode, path + depth);
		if (err)
			goto cleanup;

	}

	k = depth - at - 1;
	BUG_ON(k < 0);
	if (k)
		ext_debug("create %d intermediate indices\n", k);
	 
	i = depth - 1;
	while (k--) {
		oldblock = newblock;
		newblock = ablocks[--a];
		bh = sb_getblk(inode->i_sb, newblock);
		if (!bh) {
			err = -EIO;
			goto cleanup;
		}
		lock_buffer(bh);

		err = ext4_journal_get_create_access(handle, bh);
		if (err)
			goto cleanup;

		neh = ext_block_hdr(bh);
		neh->eh_entries = cpu_to_le16(1);
		neh->eh_magic = EXT4_EXT_MAGIC;
		neh->eh_max = cpu_to_le16(ext4_ext_space_block_idx(inode, 0));
		neh->eh_depth = cpu_to_le16(depth - i);
		fidx = EXT_FIRST_INDEX(neh);
		fidx->ei_block = border;
		ext4_idx_store_pblock(fidx, oldblock);

		ext_debug("int.index at %d (block %llu): %u -> %llu\n",
				i, newblock, le32_to_cpu(border), oldblock);
		 
		m = 0;
		path[i].p_idx++;

		ext_debug("cur 0x%p, last 0x%p\n", path[i].p_idx,
				EXT_MAX_INDEX(path[i].p_hdr));
		BUG_ON(EXT_MAX_INDEX(path[i].p_hdr) !=
				EXT_LAST_INDEX(path[i].p_hdr));
		while (path[i].p_idx <= EXT_MAX_INDEX(path[i].p_hdr)) {
			ext_debug("%d: move %d:%llu in new index %llu\n", i,
					le32_to_cpu(path[i].p_idx->ei_block),
					idx_pblock(path[i].p_idx),
					newblock);
			 
			path[i].p_idx++;
			m++;
		}
		if (m) {
			memmove(++fidx, path[i].p_idx - m,
				sizeof(struct ext4_extent_idx) * m);
			le16_add_cpu(&neh->eh_entries, m);
		}
		set_buffer_uptodate(bh);
		unlock_buffer(bh);

		err = ext4_handle_dirty_metadata(handle, inode, bh);
		if (err)
			goto cleanup;
		brelse(bh);
		bh = NULL;

		if (m) {
			err = ext4_ext_get_access(handle, inode, path + i);
			if (err)
				goto cleanup;
			le16_add_cpu(&path[i].p_hdr->eh_entries, -m);
			err = ext4_ext_dirty(handle, inode, path + i);
			if (err)
				goto cleanup;
		}

		i--;
	}

	err = ext4_ext_insert_index(handle, inode, path + at,
				    le32_to_cpu(border), newblock);

cleanup:
	if (bh) {
		if (buffer_locked(bh))
			unlock_buffer(bh);
		brelse(bh);
	}

	if (err) {
		 
		for (i = 0; i < depth; i++) {
			if (!ablocks[i])
				continue;
			ext4_free_blocks(handle, inode, ablocks[i], 1, 1);
		}
	}
	kfree(ablocks);

	return err;
}

static int ext4_ext_grow_indepth(handle_t *handle, struct inode *inode,
					struct ext4_ext_path *path,
					struct ext4_extent *newext)
{
	struct ext4_ext_path *curp = path;
	struct ext4_extent_header *neh;
	struct ext4_extent_idx *fidx;
	struct buffer_head *bh;
	ext4_fsblk_t newblock;
	int err = 0;

	newblock = ext4_ext_new_meta_block(handle, inode, path, newext, &err);
	if (newblock == 0)
		return err;

	bh = sb_getblk(inode->i_sb, newblock);
	if (!bh) {
		err = -EIO;
		ext4_std_error(inode->i_sb, err);
		return err;
	}
	lock_buffer(bh);

	err = ext4_journal_get_create_access(handle, bh);
	if (err) {
		unlock_buffer(bh);
		goto out;
	}

	memmove(bh->b_data, curp->p_hdr, sizeof(EXT4_I(inode)->i_data));

	neh = ext_block_hdr(bh);
	 
	if (ext_depth(inode))
		neh->eh_max = cpu_to_le16(ext4_ext_space_block_idx(inode, 0));
	else
		neh->eh_max = cpu_to_le16(ext4_ext_space_block(inode, 0));
	neh->eh_magic = EXT4_EXT_MAGIC;
	set_buffer_uptodate(bh);
	unlock_buffer(bh);

	err = ext4_handle_dirty_metadata(handle, inode, bh);
	if (err)
		goto out;

	err = ext4_ext_get_access(handle, inode, curp);
	if (err)
		goto out;

	curp->p_hdr->eh_magic = EXT4_EXT_MAGIC;
	curp->p_hdr->eh_max = cpu_to_le16(ext4_ext_space_root_idx(inode, 0));
	curp->p_hdr->eh_entries = cpu_to_le16(1);
	curp->p_idx = EXT_FIRST_INDEX(curp->p_hdr);

	if (path[0].p_hdr->eh_depth)
		curp->p_idx->ei_block =
			EXT_FIRST_INDEX(path[0].p_hdr)->ei_block;
	else
		curp->p_idx->ei_block =
			EXT_FIRST_EXTENT(path[0].p_hdr)->ee_block;
	ext4_idx_store_pblock(curp->p_idx, newblock);

	neh = ext_inode_hdr(inode);
	fidx = EXT_FIRST_INDEX(neh);
	ext_debug("new root: num %d(%d), lblock %d, ptr %llu\n",
		  le16_to_cpu(neh->eh_entries), le16_to_cpu(neh->eh_max),
		  le32_to_cpu(fidx->ei_block), idx_pblock(fidx));

	neh->eh_depth = cpu_to_le16(path->p_depth + 1);
	err = ext4_ext_dirty(handle, inode, curp);
out:
	brelse(bh);

	return err;
}

static int ext4_ext_create_new_leaf(handle_t *handle, struct inode *inode,
					struct ext4_ext_path *path,
					struct ext4_extent *newext)
{
	struct ext4_ext_path *curp;
	int depth, i, err = 0;

repeat:
	i = depth = ext_depth(inode);

	curp = path + depth;
	while (i > 0 && !EXT_HAS_FREE_INDEX(curp)) {
		i--;
		curp--;
	}

	if (EXT_HAS_FREE_INDEX(curp)) {
		 
		err = ext4_ext_split(handle, inode, path, newext, i);
		if (err)
			goto out;

		ext4_ext_drop_refs(path);
		path = ext4_ext_find_extent(inode,
				    (ext4_lblk_t)le32_to_cpu(newext->ee_block),
				    path);
		if (IS_ERR(path))
			err = PTR_ERR(path);
	} else {
		 
		err = ext4_ext_grow_indepth(handle, inode, path, newext);
		if (err)
			goto out;

		ext4_ext_drop_refs(path);
		path = ext4_ext_find_extent(inode,
				   (ext4_lblk_t)le32_to_cpu(newext->ee_block),
				    path);
		if (IS_ERR(path)) {
			err = PTR_ERR(path);
			goto out;
		}

		depth = ext_depth(inode);
		if (path[depth].p_hdr->eh_entries == path[depth].p_hdr->eh_max) {
			 
			goto repeat;
		}
	}

out:
	return err;
}

int
ext4_ext_search_left(struct inode *inode, struct ext4_ext_path *path,
			ext4_lblk_t *logical, ext4_fsblk_t *phys)
{
	struct ext4_extent_idx *ix;
	struct ext4_extent *ex;
	int depth, ee_len;

	BUG_ON(path == NULL);
	depth = path->p_depth;
	*phys = 0;

	if (depth == 0 && path->p_ext == NULL)
		return 0;

	ex = path[depth].p_ext;
	ee_len = ext4_ext_get_actual_len(ex);
	if (*logical < le32_to_cpu(ex->ee_block)) {
		BUG_ON(EXT_FIRST_EXTENT(path[depth].p_hdr) != ex);
		while (--depth >= 0) {
			ix = path[depth].p_idx;
			BUG_ON(ix != EXT_FIRST_INDEX(path[depth].p_hdr));
		}
		return 0;
	}

	BUG_ON(*logical < (le32_to_cpu(ex->ee_block) + ee_len));

	*logical = le32_to_cpu(ex->ee_block) + ee_len - 1;
	*phys = ext_pblock(ex) + ee_len - 1;
	return 0;
}

int
ext4_ext_search_right(struct inode *inode, struct ext4_ext_path *path,
			ext4_lblk_t *logical, ext4_fsblk_t *phys)
{
	struct buffer_head *bh = NULL;
	struct ext4_extent_header *eh;
	struct ext4_extent_idx *ix;
	struct ext4_extent *ex;
	ext4_fsblk_t block;
	int depth;	 
	int ee_len;

	BUG_ON(path == NULL);
	depth = path->p_depth;
	*phys = 0;

	if (depth == 0 && path->p_ext == NULL)
		return 0;

	ex = path[depth].p_ext;
	ee_len = ext4_ext_get_actual_len(ex);
	if (*logical < le32_to_cpu(ex->ee_block)) {
		BUG_ON(EXT_FIRST_EXTENT(path[depth].p_hdr) != ex);
		while (--depth >= 0) {
			ix = path[depth].p_idx;
			BUG_ON(ix != EXT_FIRST_INDEX(path[depth].p_hdr));
		}
		*logical = le32_to_cpu(ex->ee_block);
		*phys = ext_pblock(ex);
		return 0;
	}

	BUG_ON(*logical < (le32_to_cpu(ex->ee_block) + ee_len));

	if (ex != EXT_LAST_EXTENT(path[depth].p_hdr)) {
		 
		ex++;
		*logical = le32_to_cpu(ex->ee_block);
		*phys = ext_pblock(ex);
		return 0;
	}

	while (--depth >= 0) {
		ix = path[depth].p_idx;
		if (ix != EXT_LAST_INDEX(path[depth].p_hdr))
			goto got_index;
	}

	return 0;

got_index:
	 
	ix++;
	block = idx_pblock(ix);
	while (++depth < path->p_depth) {
		bh = sb_bread(inode->i_sb, block);
		if (bh == NULL)
			return -EIO;
		eh = ext_block_hdr(bh);
		 
		if (ext4_ext_check(inode, eh, path->p_depth - depth)) {
			put_bh(bh);
			return -EIO;
		}
		ix = EXT_FIRST_INDEX(eh);
		block = idx_pblock(ix);
		put_bh(bh);
	}

	bh = sb_bread(inode->i_sb, block);
	if (bh == NULL)
		return -EIO;
	eh = ext_block_hdr(bh);
	if (ext4_ext_check(inode, eh, path->p_depth - depth)) {
		put_bh(bh);
		return -EIO;
	}
	ex = EXT_FIRST_EXTENT(eh);
	*logical = le32_to_cpu(ex->ee_block);
	*phys = ext_pblock(ex);
	put_bh(bh);
	return 0;
}

static ext4_lblk_t
ext4_ext_next_allocated_block(struct ext4_ext_path *path)
{
	int depth;

	BUG_ON(path == NULL);
	depth = path->p_depth;

	if (depth == 0 && path->p_ext == NULL)
		return EXT_MAX_BLOCK;

	while (depth >= 0) {
		if (depth == path->p_depth) {
			 
			if (path[depth].p_ext !=
					EXT_LAST_EXTENT(path[depth].p_hdr))
			  return le32_to_cpu(path[depth].p_ext[1].ee_block);
		} else {
			 
			if (path[depth].p_idx !=
					EXT_LAST_INDEX(path[depth].p_hdr))
			  return le32_to_cpu(path[depth].p_idx[1].ei_block);
		}
		depth--;
	}

	return EXT_MAX_BLOCK;
}

static ext4_lblk_t ext4_ext_next_leaf_block(struct inode *inode,
					struct ext4_ext_path *path)
{
	int depth;

	BUG_ON(path == NULL);
	depth = path->p_depth;

	if (depth == 0)
		return EXT_MAX_BLOCK;

	depth--;

	while (depth >= 0) {
		if (path[depth].p_idx !=
				EXT_LAST_INDEX(path[depth].p_hdr))
			return (ext4_lblk_t)
				le32_to_cpu(path[depth].p_idx[1].ei_block);
		depth--;
	}

	return EXT_MAX_BLOCK;
}

static int ext4_ext_correct_indexes(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *path)
{
	struct ext4_extent_header *eh;
	int depth = ext_depth(inode);
	struct ext4_extent *ex;
	__le32 border;
	int k, err = 0;

	eh = path[depth].p_hdr;
	ex = path[depth].p_ext;
	BUG_ON(ex == NULL);
	BUG_ON(eh == NULL);

	if (depth == 0) {
		 
		return 0;
	}

	if (ex != EXT_FIRST_EXTENT(eh)) {
		 
		return 0;
	}

	k = depth - 1;
	border = path[depth].p_ext->ee_block;
	err = ext4_ext_get_access(handle, inode, path + k);
	if (err)
		return err;
	path[k].p_idx->ei_block = border;
	err = ext4_ext_dirty(handle, inode, path + k);
	if (err)
		return err;

	while (k--) {
		 
		if (path[k+1].p_idx != EXT_FIRST_INDEX(path[k+1].p_hdr))
			break;
		err = ext4_ext_get_access(handle, inode, path + k);
		if (err)
			break;
		path[k].p_idx->ei_block = border;
		err = ext4_ext_dirty(handle, inode, path + k);
		if (err)
			break;
	}

	return err;
}

int
ext4_can_extents_be_merged(struct inode *inode, struct ext4_extent *ex1,
				struct ext4_extent *ex2)
{
	unsigned short ext1_ee_len, ext2_ee_len, max_len;

	if (ext4_ext_is_uninitialized(ex1) ^ ext4_ext_is_uninitialized(ex2))
		return 0;

	if (ext4_ext_is_uninitialized(ex1))
		max_len = EXT_UNINIT_MAX_LEN;
	else
		max_len = EXT_INIT_MAX_LEN;

	ext1_ee_len = ext4_ext_get_actual_len(ex1);
	ext2_ee_len = ext4_ext_get_actual_len(ex2);

	if (le32_to_cpu(ex1->ee_block) + ext1_ee_len !=
			le32_to_cpu(ex2->ee_block))
		return 0;

	if (ext1_ee_len + ext2_ee_len > max_len)
		return 0;
#ifdef AGGRESSIVE_TEST
	if (ext1_ee_len >= 4)
		return 0;
#endif

	if (ext_pblock(ex1) + ext1_ee_len == ext_pblock(ex2))
		return 1;
	return 0;
}

int ext4_ext_try_to_merge(struct inode *inode,
			  struct ext4_ext_path *path,
			  struct ext4_extent *ex)
{
	struct ext4_extent_header *eh;
	unsigned int depth, len;
	int merge_done = 0;
	int uninitialized = 0;

	depth = ext_depth(inode);
	BUG_ON(path[depth].p_hdr == NULL);
	eh = path[depth].p_hdr;

	while (ex < EXT_LAST_EXTENT(eh)) {
		if (!ext4_can_extents_be_merged(inode, ex, ex + 1))
			break;
		 
		if (ext4_ext_is_uninitialized(ex))
			uninitialized = 1;
		ex->ee_len = cpu_to_le16(ext4_ext_get_actual_len(ex)
				+ ext4_ext_get_actual_len(ex + 1));
		if (uninitialized)
			ext4_ext_mark_uninitialized(ex);

		if (ex + 1 < EXT_LAST_EXTENT(eh)) {
			len = (EXT_LAST_EXTENT(eh) - ex - 1)
				* sizeof(struct ext4_extent);
			memmove(ex + 1, ex + 2, len);
		}
		le16_add_cpu(&eh->eh_entries, -1);
		merge_done = 1;
		WARN_ON(eh->eh_entries == 0);
		if (!eh->eh_entries)
			ext4_error(inode->i_sb, "ext4_ext_try_to_merge",
			   "inode#%lu, eh->eh_entries = 0!", inode->i_ino);
	}

	return merge_done;
}

unsigned int ext4_ext_check_overlap(struct inode *inode,
				    struct ext4_extent *newext,
				    struct ext4_ext_path *path)
{
	ext4_lblk_t b1, b2;
	unsigned int depth, len1;
	unsigned int ret = 0;

	b1 = le32_to_cpu(newext->ee_block);
	len1 = ext4_ext_get_actual_len(newext);
	depth = ext_depth(inode);
	if (!path[depth].p_ext)
		goto out;
	b2 = le32_to_cpu(path[depth].p_ext->ee_block);

	if (b2 < b1) {
		b2 = ext4_ext_next_allocated_block(path);
		if (b2 == EXT_MAX_BLOCK)
			goto out;
	}

	if (b1 + len1 < b1) {
		len1 = EXT_MAX_BLOCK - b1;
		newext->ee_len = cpu_to_le16(len1);
		ret = 1;
	}

	if (b1 + len1 > b2) {
		newext->ee_len = cpu_to_le16(b2 - b1);
		ret = 1;
	}
out:
	return ret;
}

int ext4_ext_insert_extent(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *path,
				struct ext4_extent *newext, int flag)
{
	struct ext4_extent_header *eh;
	struct ext4_extent *ex, *fex;
	struct ext4_extent *nearex;  
	struct ext4_ext_path *npath = NULL;
	int depth, len, err;
	ext4_lblk_t next;
	unsigned uninitialized = 0;

	BUG_ON(ext4_ext_get_actual_len(newext) == 0);
	depth = ext_depth(inode);
	ex = path[depth].p_ext;
	BUG_ON(path[depth].p_hdr == NULL);

	if (ex && (flag != EXT4_GET_BLOCKS_DIO_CREATE_EXT)
		&& ext4_can_extents_be_merged(inode, ex, newext)) {
		ext_debug("append [%d]%d block to %d:[%d]%d (from %llu)\n",
				ext4_ext_is_uninitialized(newext),
				ext4_ext_get_actual_len(newext),
				le32_to_cpu(ex->ee_block),
				ext4_ext_is_uninitialized(ex),
				ext4_ext_get_actual_len(ex), ext_pblock(ex));
		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			return err;

		if (ext4_ext_is_uninitialized(ex))
			uninitialized = 1;
		ex->ee_len = cpu_to_le16(ext4_ext_get_actual_len(ex)
					+ ext4_ext_get_actual_len(newext));
		if (uninitialized)
			ext4_ext_mark_uninitialized(ex);
		eh = path[depth].p_hdr;
		nearex = ex;
		goto merge;
	}

repeat:
	depth = ext_depth(inode);
	eh = path[depth].p_hdr;
	if (le16_to_cpu(eh->eh_entries) < le16_to_cpu(eh->eh_max))
		goto has_space;

	fex = EXT_LAST_EXTENT(eh);
	next = ext4_ext_next_leaf_block(inode, path);
	if (le32_to_cpu(newext->ee_block) > le32_to_cpu(fex->ee_block)
	    && next != EXT_MAX_BLOCK) {
		ext_debug("next leaf block - %d\n", next);
		BUG_ON(npath != NULL);
		npath = ext4_ext_find_extent(inode, next, NULL);
		if (IS_ERR(npath))
			return PTR_ERR(npath);
		BUG_ON(npath->p_depth != path->p_depth);
		eh = npath[depth].p_hdr;
		if (le16_to_cpu(eh->eh_entries) < le16_to_cpu(eh->eh_max)) {
			ext_debug("next leaf isnt full(%d)\n",
				  le16_to_cpu(eh->eh_entries));
			path = npath;
			goto repeat;
		}
		ext_debug("next leaf has no free space(%d,%d)\n",
			  le16_to_cpu(eh->eh_entries), le16_to_cpu(eh->eh_max));
	}

	err = ext4_ext_create_new_leaf(handle, inode, path, newext);
	if (err)
		goto cleanup;
	depth = ext_depth(inode);
	eh = path[depth].p_hdr;

has_space:
	nearex = path[depth].p_ext;

	err = ext4_ext_get_access(handle, inode, path + depth);
	if (err)
		goto cleanup;

	if (!nearex) {
		 
		ext_debug("first extent in the leaf: %d:%llu:[%d]%d\n",
				le32_to_cpu(newext->ee_block),
				ext_pblock(newext),
				ext4_ext_is_uninitialized(newext),
				ext4_ext_get_actual_len(newext));
		path[depth].p_ext = EXT_FIRST_EXTENT(eh);
	} else if (le32_to_cpu(newext->ee_block)
			   > le32_to_cpu(nearex->ee_block)) {
 
		if (nearex != EXT_LAST_EXTENT(eh)) {
			len = EXT_MAX_EXTENT(eh) - nearex;
			len = (len - 1) * sizeof(struct ext4_extent);
			len = len < 0 ? 0 : len;
			ext_debug("insert %d:%llu:[%d]%d after: nearest 0x%p, "
					"move %d from 0x%p to 0x%p\n",
					le32_to_cpu(newext->ee_block),
					ext_pblock(newext),
					ext4_ext_is_uninitialized(newext),
					ext4_ext_get_actual_len(newext),
					nearex, len, nearex + 1, nearex + 2);
			memmove(nearex + 2, nearex + 1, len);
		}
		path[depth].p_ext = nearex + 1;
	} else {
		BUG_ON(newext->ee_block == nearex->ee_block);
		len = (EXT_MAX_EXTENT(eh) - nearex) * sizeof(struct ext4_extent);
		len = len < 0 ? 0 : len;
		ext_debug("insert %d:%llu:[%d]%d before: nearest 0x%p, "
				"move %d from 0x%p to 0x%p\n",
				le32_to_cpu(newext->ee_block),
				ext_pblock(newext),
				ext4_ext_is_uninitialized(newext),
				ext4_ext_get_actual_len(newext),
				nearex, len, nearex + 1, nearex + 2);
		memmove(nearex + 1, nearex, len);
		path[depth].p_ext = nearex;
	}

	le16_add_cpu(&eh->eh_entries, 1);
	nearex = path[depth].p_ext;
	nearex->ee_block = newext->ee_block;
	ext4_ext_store_pblock(nearex, ext_pblock(newext));
	nearex->ee_len = newext->ee_len;

merge:
	 
	if (flag != EXT4_GET_BLOCKS_DIO_CREATE_EXT)
		ext4_ext_try_to_merge(inode, path, nearex);

	err = ext4_ext_correct_indexes(handle, inode, path);
	if (err)
		goto cleanup;

	err = ext4_ext_dirty(handle, inode, path + depth);

cleanup:
	if (npath) {
		ext4_ext_drop_refs(npath);
		kfree(npath);
	}
	ext4_ext_invalidate_cache(inode);
	return err;
}

int ext4_ext_walk_space(struct inode *inode, ext4_lblk_t block,
			ext4_lblk_t num, ext_prepare_callback func,
			void *cbdata)
{
	struct ext4_ext_path *path = NULL;
	struct ext4_ext_cache cbex;
	struct ext4_extent *ex;
	ext4_lblk_t next, start = 0, end = 0;
	ext4_lblk_t last = block + num;
	int depth, exists, err = 0;

	BUG_ON(func == NULL);
	BUG_ON(inode == NULL);

	while (block < last && block != EXT_MAX_BLOCK) {
		num = last - block;
		 
		down_read(&EXT4_I(inode)->i_data_sem);
		path = ext4_ext_find_extent(inode, block, path);
		up_read(&EXT4_I(inode)->i_data_sem);
		if (IS_ERR(path)) {
			err = PTR_ERR(path);
			path = NULL;
			break;
		}

		depth = ext_depth(inode);
		BUG_ON(path[depth].p_hdr == NULL);
		ex = path[depth].p_ext;
		next = ext4_ext_next_allocated_block(path);

		exists = 0;
		if (!ex) {
			 
			start = block;
			end = block + num;
		} else if (le32_to_cpu(ex->ee_block) > block) {
			 
			start = block;
			end = le32_to_cpu(ex->ee_block);
			if (block + num < end)
				end = block + num;
		} else if (block >= le32_to_cpu(ex->ee_block)
					+ ext4_ext_get_actual_len(ex)) {
			 
			start = block;
			end = block + num;
			if (end >= next)
				end = next;
		} else if (block >= le32_to_cpu(ex->ee_block)) {
			 
			start = block;
			end = le32_to_cpu(ex->ee_block)
				+ ext4_ext_get_actual_len(ex);
			if (block + num < end)
				end = block + num;
			exists = 1;
		} else {
			BUG();
		}
		BUG_ON(end <= start);

		if (!exists) {
			cbex.ec_block = start;
			cbex.ec_len = end - start;
			cbex.ec_start = 0;
			cbex.ec_type = EXT4_EXT_CACHE_GAP;
		} else {
			cbex.ec_block = le32_to_cpu(ex->ee_block);
			cbex.ec_len = ext4_ext_get_actual_len(ex);
			cbex.ec_start = ext_pblock(ex);
			cbex.ec_type = EXT4_EXT_CACHE_EXTENT;
		}

		BUG_ON(cbex.ec_len == 0);
		err = func(inode, path, &cbex, ex, cbdata);
		ext4_ext_drop_refs(path);

		if (err < 0)
			break;

		if (err == EXT_REPEAT)
			continue;
		else if (err == EXT_BREAK) {
			err = 0;
			break;
		}

		if (ext_depth(inode) != depth) {
			 
			kfree(path);
			path = NULL;
		}

		block = cbex.ec_block + cbex.ec_len;
	}

	if (path) {
		ext4_ext_drop_refs(path);
		kfree(path);
	}

	return err;
}

static void
ext4_ext_put_in_cache(struct inode *inode, ext4_lblk_t block,
			__u32 len, ext4_fsblk_t start, int type)
{
	struct ext4_ext_cache *cex;
	BUG_ON(len == 0);
	spin_lock(&EXT4_I(inode)->i_block_reservation_lock);
	cex = &EXT4_I(inode)->i_cached_extent;
	cex->ec_type = type;
	cex->ec_block = block;
	cex->ec_len = len;
	cex->ec_start = start;
	spin_unlock(&EXT4_I(inode)->i_block_reservation_lock);
}

static void
ext4_ext_put_gap_in_cache(struct inode *inode, struct ext4_ext_path *path,
				ext4_lblk_t block)
{
	int depth = ext_depth(inode);
	unsigned long len;
	ext4_lblk_t lblock;
	struct ext4_extent *ex;

	ex = path[depth].p_ext;
	if (ex == NULL) {
		 
		lblock = 0;
		len = EXT_MAX_BLOCK;
		ext_debug("cache gap(whole file):");
	} else if (block < le32_to_cpu(ex->ee_block)) {
		lblock = block;
		len = le32_to_cpu(ex->ee_block) - block;
		ext_debug("cache gap(before): %u [%u:%u]",
				block,
				le32_to_cpu(ex->ee_block),
				 ext4_ext_get_actual_len(ex));
	} else if (block >= le32_to_cpu(ex->ee_block)
			+ ext4_ext_get_actual_len(ex)) {
		ext4_lblk_t next;
		lblock = le32_to_cpu(ex->ee_block)
			+ ext4_ext_get_actual_len(ex);

		next = ext4_ext_next_allocated_block(path);
		ext_debug("cache gap(after): [%u:%u] %u",
				le32_to_cpu(ex->ee_block),
				ext4_ext_get_actual_len(ex),
				block);
		BUG_ON(next == lblock);
		len = next - lblock;
	} else {
		lblock = len = 0;
		BUG();
	}

	ext_debug(" -> %u:%lu\n", lblock, len);
	ext4_ext_put_in_cache(inode, lblock, len, 0, EXT4_EXT_CACHE_GAP);
}

static int
ext4_ext_in_cache(struct inode *inode, ext4_lblk_t block,
			struct ext4_extent *ex)
{
	struct ext4_ext_cache *cex;
	int ret = EXT4_EXT_CACHE_NO;

	spin_lock(&EXT4_I(inode)->i_block_reservation_lock);
	cex = &EXT4_I(inode)->i_cached_extent;

	if (cex->ec_type == EXT4_EXT_CACHE_NO)
		goto errout;

	BUG_ON(cex->ec_type != EXT4_EXT_CACHE_GAP &&
			cex->ec_type != EXT4_EXT_CACHE_EXTENT);
	if (block >= cex->ec_block && block < cex->ec_block + cex->ec_len) {
		ex->ee_block = cpu_to_le32(cex->ec_block);
		ext4_ext_store_pblock(ex, cex->ec_start);
		ex->ee_len = cpu_to_le16(cex->ec_len);
		ext_debug("%u cached by %u:%u:%llu\n",
				block,
				cex->ec_block, cex->ec_len, cex->ec_start);
		ret = cex->ec_type;
	}
errout:
	spin_unlock(&EXT4_I(inode)->i_block_reservation_lock);
	return ret;
}

static int ext4_ext_rm_idx(handle_t *handle, struct inode *inode,
			struct ext4_ext_path *path)
{
	struct buffer_head *bh;
	int err;
	ext4_fsblk_t leaf;

	path--;
	leaf = idx_pblock(path->p_idx);
	BUG_ON(path->p_hdr->eh_entries == 0);
	err = ext4_ext_get_access(handle, inode, path);
	if (err)
		return err;
	le16_add_cpu(&path->p_hdr->eh_entries, -1);
	err = ext4_ext_dirty(handle, inode, path);
	if (err)
		return err;
	ext_debug("index is empty, remove it, free block %llu\n", leaf);
	bh = sb_find_get_block(inode->i_sb, leaf);
	ext4_forget(handle, 1, inode, bh, leaf);
	ext4_free_blocks(handle, inode, leaf, 1, 1);
	return err;
}

int ext4_ext_calc_credits_for_single_extent(struct inode *inode, int nrblocks,
						struct ext4_ext_path *path)
{
	if (path) {
		int depth = ext_depth(inode);
		int ret = 0;

		if (le16_to_cpu(path[depth].p_hdr->eh_entries)
				< le16_to_cpu(path[depth].p_hdr->eh_max)) {

			ret = 2 + EXT4_META_TRANS_BLOCKS(inode->i_sb);
			return ret;
		}
	}

	return ext4_chunk_trans_blocks(inode, nrblocks);
}

int ext4_ext_index_trans_blocks(struct inode *inode, int nrblocks, int chunk)
{
	int index;
	int depth = ext_depth(inode);

	if (chunk)
		index = depth * 2;
	else
		index = depth * 3;

	return index;
}

static int ext4_remove_blocks(handle_t *handle, struct inode *inode,
				struct ext4_extent *ex,
				ext4_lblk_t from, ext4_lblk_t to)
{
	struct buffer_head *bh;
	unsigned short ee_len =  ext4_ext_get_actual_len(ex);
	int i, metadata = 0;

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
		metadata = 1;
#ifdef EXTENTS_STATS
	{
		struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
		spin_lock(&sbi->s_ext_stats_lock);
		sbi->s_ext_blocks += ee_len;
		sbi->s_ext_extents++;
		if (ee_len < sbi->s_ext_min)
			sbi->s_ext_min = ee_len;
		if (ee_len > sbi->s_ext_max)
			sbi->s_ext_max = ee_len;
		if (ext_depth(inode) > sbi->s_depth_max)
			sbi->s_depth_max = ext_depth(inode);
		spin_unlock(&sbi->s_ext_stats_lock);
	}
#endif
	if (from >= le32_to_cpu(ex->ee_block)
	    && to == le32_to_cpu(ex->ee_block) + ee_len - 1) {
		 
		ext4_lblk_t num;
		ext4_fsblk_t start;

		num = le32_to_cpu(ex->ee_block) + ee_len - from;
		start = ext_pblock(ex) + ee_len - num;
		ext_debug("free last %u blocks starting %llu\n", num, start);
		for (i = 0; i < num; i++) {
			bh = sb_find_get_block(inode->i_sb, start + i);
			ext4_forget(handle, metadata, inode, bh, start + i);
		}
		ext4_free_blocks(handle, inode, start, num, metadata);
	} else if (from == le32_to_cpu(ex->ee_block)
		   && to <= le32_to_cpu(ex->ee_block) + ee_len - 1) {
		printk(KERN_INFO "strange request: removal %u-%u from %u:%u\n",
			from, to, le32_to_cpu(ex->ee_block), ee_len);
	} else {
		printk(KERN_INFO "strange request: removal(2) "
				"%u-%u from %u:%u\n",
				from, to, le32_to_cpu(ex->ee_block), ee_len);
	}
	return 0;
}

static int
ext4_ext_rm_leaf(handle_t *handle, struct inode *inode,
		struct ext4_ext_path *path, ext4_lblk_t start)
{
	int err = 0, correct_index = 0;
	int depth = ext_depth(inode), credits;
	struct ext4_extent_header *eh;
	ext4_lblk_t a, b, block;
	unsigned num;
	ext4_lblk_t ex_ee_block;
	unsigned short ex_ee_len;
	unsigned uninitialized = 0;
	struct ext4_extent *ex;

#ifdef SYNO_FAST_RW_FIX
	ext_debug("truncate inode [%lu] since %u in leaf\n", inode->i_ino, start);
#else
	ext_debug("truncate since %u in leaf\n", start);
#endif
	if (!path[depth].p_hdr)
		path[depth].p_hdr = ext_block_hdr(path[depth].p_bh);
	eh = path[depth].p_hdr;
	BUG_ON(eh == NULL);

	ex = EXT_LAST_EXTENT(eh);

	ex_ee_block = le32_to_cpu(ex->ee_block);
	ex_ee_len = ext4_ext_get_actual_len(ex);

	while (ex >= EXT_FIRST_EXTENT(eh) &&
			ex_ee_block + ex_ee_len > start) {

		if (ext4_ext_is_uninitialized(ex))
			uninitialized = 1;
		else
			uninitialized = 0;

		ext_debug("remove ext %u:[%d]%d\n", ex_ee_block,
			 uninitialized, ex_ee_len);
		path[depth].p_ext = ex;

		a = ex_ee_block > start ? ex_ee_block : start;
		b = ex_ee_block + ex_ee_len - 1 < EXT_MAX_BLOCK ?
			ex_ee_block + ex_ee_len - 1 : EXT_MAX_BLOCK;

		ext_debug("  border %u:%u\n", a, b);

		if (a != ex_ee_block && b != ex_ee_block + ex_ee_len - 1) {
			block = 0;
			num = 0;
			BUG();
		} else if (a != ex_ee_block) {
			 
			block = ex_ee_block;
			num = a - block;
		} else if (b != ex_ee_block + ex_ee_len - 1) {
			 
			block = a;
			num = b - a;
			 
			BUG();
		} else {
			 
			block = ex_ee_block;
			num = 0;
			BUG_ON(a != ex_ee_block);
			BUG_ON(b != ex_ee_block + ex_ee_len - 1);
		}

		credits = 7 + 2*(ex_ee_len/EXT4_BLOCKS_PER_GROUP(inode->i_sb));
		if (ex == EXT_FIRST_EXTENT(eh)) {
			correct_index = 1;
			credits += (ext_depth(inode)) + 1;
		}
		credits += EXT4_MAXQUOTAS_TRANS_BLOCKS(inode->i_sb);

		err = ext4_ext_truncate_extend_restart(handle, inode, credits);
		if (err)
			goto out;

		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			goto out;

		err = ext4_remove_blocks(handle, inode, ex, a, b);
		if (err)
			goto out;

		if (num == 0) {
			 
			ext4_ext_store_pblock(ex, 0);
			le16_add_cpu(&eh->eh_entries, -1);
		}

		ex->ee_block = cpu_to_le32(block);
		ex->ee_len = cpu_to_le16(num);
		 
		if (uninitialized && num)
			ext4_ext_mark_uninitialized(ex);

		err = ext4_ext_dirty(handle, inode, path + depth);
		if (err)
			goto out;

		ext_debug("new extent: %u:%u:%llu\n", block, num,
				ext_pblock(ex));
		ex--;
		ex_ee_block = le32_to_cpu(ex->ee_block);
		ex_ee_len = ext4_ext_get_actual_len(ex);
	}

	if (correct_index && eh->eh_entries)
		err = ext4_ext_correct_indexes(handle, inode, path);

	if (err == 0 && eh->eh_entries == 0 && path[depth].p_bh != NULL)
		err = ext4_ext_rm_idx(handle, inode, path + depth);

out:
	return err;
}

static int
ext4_ext_more_to_rm(struct ext4_ext_path *path)
{
	BUG_ON(path->p_idx == NULL);

	if (path->p_idx < EXT_FIRST_INDEX(path->p_hdr))
		return 0;

	if (le16_to_cpu(path->p_hdr->eh_entries) == path->p_block)
		return 0;
	return 1;
}

static int ext4_ext_remove_space(struct inode *inode, ext4_lblk_t start)
{
	struct super_block *sb = inode->i_sb;
	int depth = ext_depth(inode);
	struct ext4_ext_path *path;
	handle_t *handle;
	int i, err;

#ifdef SYNO_FAST_RW_FIX
	ext_debug("truncate inode [%lu] since %u\n", inode->i_ino, start);
#else
	ext_debug("truncate since %u\n", start);
#endif

	handle = ext4_journal_start(inode, depth + 1);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

again:
	ext4_ext_invalidate_cache(inode);

	depth = ext_depth(inode);
	path = kzalloc(sizeof(struct ext4_ext_path) * (depth + 1), GFP_NOFS);
	if (path == NULL) {
		ext4_journal_stop(handle);
		return -ENOMEM;
	}
	path[0].p_depth = depth;
	path[0].p_hdr = ext_inode_hdr(inode);
	if (ext4_ext_check(inode, path[0].p_hdr, depth)) {
		err = -EIO;
		goto out;
	}
	i = err = 0;

	while (i >= 0 && err == 0) {
		if (i == depth) {
			 
			err = ext4_ext_rm_leaf(handle, inode, path, start);
			 
			brelse(path[i].p_bh);
			path[i].p_bh = NULL;
			i--;
			continue;
		}

		if (!path[i].p_hdr) {
			ext_debug("initialize header\n");
			path[i].p_hdr = ext_block_hdr(path[i].p_bh);
		}

		if (!path[i].p_idx) {
			 
			path[i].p_idx = EXT_LAST_INDEX(path[i].p_hdr);
			path[i].p_block = le16_to_cpu(path[i].p_hdr->eh_entries)+1;
			ext_debug("init index ptr: hdr 0x%p, num %d\n",
				  path[i].p_hdr,
				  le16_to_cpu(path[i].p_hdr->eh_entries));
		} else {
			 
			path[i].p_idx--;
		}

		ext_debug("level %d - index, first 0x%p, cur 0x%p\n",
				i, EXT_FIRST_INDEX(path[i].p_hdr),
				path[i].p_idx);
		if (ext4_ext_more_to_rm(path + i)) {
			struct buffer_head *bh;
			 
			ext_debug("move to level %d (block %llu)\n",
				  i + 1, idx_pblock(path[i].p_idx));
			memset(path + i + 1, 0, sizeof(*path));
			bh = sb_bread(sb, idx_pblock(path[i].p_idx));
			if (!bh) {
				 
				err = -EIO;
				break;
			}
			if (WARN_ON(i + 1 > depth)) {
				err = -EIO;
				break;
			}
			if (ext4_ext_check(inode, ext_block_hdr(bh),
							depth - i - 1)) {
				err = -EIO;
				break;
			}
			path[i + 1].p_bh = bh;

			path[i].p_block = le16_to_cpu(path[i].p_hdr->eh_entries);
			i++;
		} else {
			 
			if (path[i].p_hdr->eh_entries == 0 && i > 0) {
				 
				err = ext4_ext_rm_idx(handle, inode, path + i);
			}
			 
			brelse(path[i].p_bh);
			path[i].p_bh = NULL;
			i--;
			ext_debug("return to level %d\n", i);
		}
	}

	if (path->p_hdr->eh_entries == 0) {
		 
		err = ext4_ext_get_access(handle, inode, path);
		if (err == 0) {
			ext_inode_hdr(inode)->eh_depth = 0;
			ext_inode_hdr(inode)->eh_max =
				cpu_to_le16(ext4_ext_space_root(inode, 0));
			err = ext4_ext_dirty(handle, inode, path);
		}
	}
out:
	ext4_ext_drop_refs(path);
	kfree(path);
	if (err == -EAGAIN)
		goto again;
	ext4_journal_stop(handle);

	return err;
}

void ext4_ext_init(struct super_block *sb)
{
	 
	if (EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_EXTENTS)) {
#if defined(AGGRESSIVE_TEST) || defined(CHECK_BINSEARCH) || defined(EXTENTS_STATS)
		printk(KERN_INFO "EXT4-fs: file extents enabled");
#ifdef AGGRESSIVE_TEST
		printk(", aggressive tests");
#endif
#ifdef CHECK_BINSEARCH
		printk(", check binsearch");
#endif
#ifdef EXTENTS_STATS
		printk(", stats");
#endif
		printk("\n");
#endif
#ifdef EXTENTS_STATS
		spin_lock_init(&EXT4_SB(sb)->s_ext_stats_lock);
		EXT4_SB(sb)->s_ext_min = 1 << 30;
		EXT4_SB(sb)->s_ext_max = 0;
#endif
	}
}

void ext4_ext_release(struct super_block *sb)
{
	if (!EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_EXTENTS))
		return;

#ifdef EXTENTS_STATS
	if (EXT4_SB(sb)->s_ext_blocks && EXT4_SB(sb)->s_ext_extents) {
		struct ext4_sb_info *sbi = EXT4_SB(sb);
		printk(KERN_ERR "EXT4-fs: %lu blocks in %lu extents (%lu ave)\n",
			sbi->s_ext_blocks, sbi->s_ext_extents,
			sbi->s_ext_blocks / sbi->s_ext_extents);
		printk(KERN_ERR "EXT4-fs: extents: %lu min, %lu max, max depth %lu\n",
			sbi->s_ext_min, sbi->s_ext_max, sbi->s_depth_max);
	}
#endif
}

static void bi_complete(struct bio *bio, int error)
{
	complete((struct completion *)bio->bi_private);
}

static int ext4_ext_zeroout(struct inode *inode, struct ext4_extent *ex)
{
	int ret;
	struct bio *bio;
	int blkbits, blocksize;
	sector_t ee_pblock;
	struct completion event;
	unsigned int ee_len, len, done, offset;

	blkbits   = inode->i_blkbits;
	blocksize = inode->i_sb->s_blocksize;
	ee_len    = ext4_ext_get_actual_len(ex);
	ee_pblock = ext_pblock(ex);

	ee_pblock = ee_pblock << (blkbits - 9);

	while (ee_len > 0) {

		if (ee_len > BIO_MAX_PAGES)
			len = BIO_MAX_PAGES;
		else
			len = ee_len;

		bio = bio_alloc(GFP_NOIO, len);
		if (!bio)
			return -ENOMEM;

		bio->bi_sector = ee_pblock;
		bio->bi_bdev   = inode->i_sb->s_bdev;

		done = 0;
		offset = 0;
		while (done < len) {
			ret = bio_add_page(bio, ZERO_PAGE(0),
							blocksize, offset);
			if (ret != blocksize) {
				 
				break;
			}
			done++;
			offset += blocksize;
			if (offset >= PAGE_CACHE_SIZE)
				offset = 0;
		}

		init_completion(&event);
		bio->bi_private = &event;
		bio->bi_end_io = bi_complete;
		submit_bio(WRITE, bio);
		wait_for_completion(&event);

		if (!test_bit(BIO_UPTODATE, &bio->bi_flags)) {
			bio_put(bio);
			return -EIO;
		}
		bio_put(bio);
		ee_len    -= done;
		ee_pblock += done  << (blkbits - 9);
	}
	return 0;
}

#define EXT4_EXT_ZERO_LEN 7
 
static int ext4_ext_convert_to_initialized(handle_t *handle,
						struct inode *inode,
						struct ext4_ext_path *path,
						ext4_lblk_t iblock,
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
						unsigned int max_blocks,
						int zeroout_flag)
#else
						unsigned int max_blocks)
#endif
{
	struct ext4_extent *ex, newex, orig_ex;
	struct ext4_extent *ex1 = NULL;
	struct ext4_extent *ex2 = NULL;
	struct ext4_extent *ex3 = NULL;
	struct ext4_extent_header *eh;
	ext4_lblk_t ee_block, eof_block;
	unsigned int allocated, ee_len, depth;
	ext4_fsblk_t newblock;
	int err = 0;
	int ret = 0;
	int may_zeroout;

	ext_debug("ext4_ext_convert_to_initialized: inode %lu, logical"
		"block %llu, max_blocks %u\n", inode->i_ino,
		(unsigned long long)iblock, max_blocks);

	eof_block = (inode->i_size + inode->i_sb->s_blocksize - 1) >>
		inode->i_sb->s_blocksize_bits;
	if (eof_block < iblock + max_blocks)
		eof_block = iblock + max_blocks;

	depth = ext_depth(inode);
	eh = path[depth].p_hdr;
	ex = path[depth].p_ext;
	ee_block = le32_to_cpu(ex->ee_block);
	ee_len = ext4_ext_get_actual_len(ex);
	allocated = ee_len - (iblock - ee_block);
	newblock = iblock - ee_block + ext_pblock(ex);

	ex2 = ex;
	orig_ex.ee_block = ex->ee_block;
	orig_ex.ee_len   = cpu_to_le16(ee_len);
	ext4_ext_store_pblock(&orig_ex, ext_pblock(ex));
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
 	if(!zeroout_flag) {
 		 
 		if(!ext4_ext_is_uninitialized(ex)) {
  
 			return (allocated > max_blocks) ? max_blocks : allocated;
 		}
 	}
#endif

	may_zeroout = ee_block + ee_len <= eof_block;

	err = ext4_ext_get_access(handle, inode, path + depth);
	if (err)
		goto out;
	 
	if (ee_len <= 2*EXT4_EXT_ZERO_LEN && may_zeroout) {
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
		if(zeroout_flag) {
#endif
		err =  ext4_ext_zeroout(inode, &orig_ex);
		if (err)
			goto fix_extent_len;
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
		}
#endif
		 
		ex->ee_block = orig_ex.ee_block;
		ex->ee_len   = orig_ex.ee_len;
		ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
		ext4_ext_dirty(handle, inode, path + depth);
		 
		return allocated;
	}

	if (iblock > ee_block) {
		ex1 = ex;
		ex1->ee_len = cpu_to_le16(iblock - ee_block);
		ext4_ext_mark_uninitialized(ex1);
		ext4_ext_dirty(handle, inode, path + depth);
		ex2 = &newex;
	}
	 
	if (!ex1 && allocated > max_blocks)
		ex2->ee_len = cpu_to_le16(max_blocks);
	 
	if (allocated > max_blocks) {
		unsigned int newdepth;
		 
		if (allocated <= EXT4_EXT_ZERO_LEN && may_zeroout) {
			 
			ex->ee_block = orig_ex.ee_block;
			ex->ee_len   = cpu_to_le16(ee_len - allocated);
			ext4_ext_mark_uninitialized(ex);
			ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
			ext4_ext_dirty(handle, inode, path + depth);

			ex3 = &newex;
			ex3->ee_block = cpu_to_le32(iblock);
			ext4_ext_store_pblock(ex3, newblock);
			ex3->ee_len = cpu_to_le16(allocated);
			err = ext4_ext_insert_extent(handle, inode, path,
							ex3, 0);
			if (err == -ENOSPC) {
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
				if(zeroout_flag) {
#endif
				err =  ext4_ext_zeroout(inode, &orig_ex);
				if (err)
					goto fix_extent_len;
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
				}
#endif
				ex->ee_block = orig_ex.ee_block;
				ex->ee_len   = orig_ex.ee_len;
				ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
				ext4_ext_dirty(handle, inode, path + depth);
				 
				return allocated;

			} else if (err)
				goto fix_extent_len;

#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
			if(zeroout_flag) {
#endif
			 
			err =  ext4_ext_zeroout(inode, ex3);
			if (err) {
				 
				depth = ext_depth(inode);
				ext4_ext_drop_refs(path);
				path = ext4_ext_find_extent(inode,
								iblock, path);
				if (IS_ERR(path)) {
					err = PTR_ERR(path);
					return err;
				}
				 
				ex = path[depth].p_ext;
				err = ext4_ext_get_access(handle, inode,
								path + depth);
				if (err)
					return err;
				ext4_ext_mark_uninitialized(ex);
				ext4_ext_dirty(handle, inode, path + depth);
				return err;
			}
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
			}
#endif

			return allocated;
		}
		ex3 = &newex;
		ex3->ee_block = cpu_to_le32(iblock + max_blocks);
		ext4_ext_store_pblock(ex3, newblock + max_blocks);
		ex3->ee_len = cpu_to_le16(allocated - max_blocks);
		ext4_ext_mark_uninitialized(ex3);
		err = ext4_ext_insert_extent(handle, inode, path, ex3, 0);
		if (err == -ENOSPC && may_zeroout) {
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
 		if(zeroout_flag) {
#endif
			err =  ext4_ext_zeroout(inode, &orig_ex);
			if (err)
				goto fix_extent_len;
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
		}
#endif
			 
			ex->ee_block = orig_ex.ee_block;
			ex->ee_len   = orig_ex.ee_len;
			ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
			ext4_ext_dirty(handle, inode, path + depth);
			 
			return allocated;

		} else if (err)
			goto fix_extent_len;
		 
		newdepth = ext_depth(inode);
		 
		ee_len -= ext4_ext_get_actual_len(ex3);
		orig_ex.ee_len = cpu_to_le16(ee_len);
		may_zeroout = ee_block + ee_len <= eof_block;

		depth = newdepth;
		ext4_ext_drop_refs(path);
		path = ext4_ext_find_extent(inode, iblock, path);
		if (IS_ERR(path)) {
			err = PTR_ERR(path);
			goto out;
		}
		eh = path[depth].p_hdr;
		ex = path[depth].p_ext;
		if (ex2 != &newex)
			ex2 = ex;

		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			goto out;

		allocated = max_blocks;

		if (le16_to_cpu(orig_ex.ee_len) <= EXT4_EXT_ZERO_LEN &&
			iblock != ee_block && may_zeroout) {
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
 		if(zeroout_flag) {
#endif
			err =  ext4_ext_zeroout(inode, &orig_ex);
			if (err)
				goto fix_extent_len;
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
		}
#endif
			 
			ex->ee_block = orig_ex.ee_block;
			ex->ee_len   = orig_ex.ee_len;
			ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
			ext4_ext_dirty(handle, inode, path + depth);
			 
			return allocated;
		}
	}
	 
	if (ex1 && ex1 != ex) {
		ex1 = ex;
		ex1->ee_len = cpu_to_le16(iblock - ee_block);
		ext4_ext_mark_uninitialized(ex1);
		ex2 = &newex;
	}
	 
	ex2->ee_block = cpu_to_le32(iblock);
	ext4_ext_store_pblock(ex2, newblock);
	ex2->ee_len = cpu_to_le16(allocated);
	if (ex2 != ex)
		goto insert;
	 
	if (ex2 > EXT_FIRST_EXTENT(eh)) {
		 
		ret = ext4_ext_try_to_merge(inode, path, ex2 - 1);
		if (ret) {
			err = ext4_ext_correct_indexes(handle, inode, path);
			if (err)
				goto out;
			depth = ext_depth(inode);
			ex2--;
		}
	}
	 
	if (!ex3) {
		ret = ext4_ext_try_to_merge(inode, path, ex2);
		if (ret) {
			err = ext4_ext_correct_indexes(handle, inode, path);
			if (err)
				goto out;
		}
	}
	 
	err = ext4_ext_dirty(handle, inode, path + depth);
	goto out;
insert:
	err = ext4_ext_insert_extent(handle, inode, path, &newex, 0);
	if (err == -ENOSPC && may_zeroout) {
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
		if(zeroout_flag) {
#endif
		err =  ext4_ext_zeroout(inode, &orig_ex);
		if (err)
			goto fix_extent_len;
#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
		}
#endif
		 
		ex->ee_block = orig_ex.ee_block;
		ex->ee_len   = orig_ex.ee_len;
		ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
		ext4_ext_dirty(handle, inode, path + depth);
		 
		return allocated;
	} else if (err)
		goto fix_extent_len;
out:
	ext4_ext_show_leaf(inode, path);
	return err ? err : allocated;

fix_extent_len:
	ex->ee_block = orig_ex.ee_block;
	ex->ee_len   = orig_ex.ee_len;
	ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
	ext4_ext_mark_uninitialized(ex);
	ext4_ext_dirty(handle, inode, path + depth);
	return err;
}

static int ext4_split_unwritten_extents(handle_t *handle,
					struct inode *inode,
					struct ext4_ext_path *path,
					ext4_lblk_t iblock,
					unsigned int max_blocks,
					int flags)
{
	struct ext4_extent *ex, newex, orig_ex;
	struct ext4_extent *ex1 = NULL;
	struct ext4_extent *ex2 = NULL;
	struct ext4_extent *ex3 = NULL;
	struct ext4_extent_header *eh;
	ext4_lblk_t ee_block, eof_block;
	unsigned int allocated, ee_len, depth;
	ext4_fsblk_t newblock;
	int err = 0;
	int may_zeroout;

	ext_debug("ext4_split_unwritten_extents: inode %lu, logical"
		"block %llu, max_blocks %u\n", inode->i_ino,
		(unsigned long long)iblock, max_blocks);

	eof_block = (inode->i_size + inode->i_sb->s_blocksize - 1) >>
		inode->i_sb->s_blocksize_bits;
	if (eof_block < iblock + max_blocks)
		eof_block = iblock + max_blocks;

	depth = ext_depth(inode);
	eh = path[depth].p_hdr;
	ex = path[depth].p_ext;
	ee_block = le32_to_cpu(ex->ee_block);
	ee_len = ext4_ext_get_actual_len(ex);
	allocated = ee_len - (iblock - ee_block);
	newblock = iblock - ee_block + ext_pblock(ex);

	ex2 = ex;
	orig_ex.ee_block = ex->ee_block;
	orig_ex.ee_len   = cpu_to_le16(ee_len);
	ext4_ext_store_pblock(&orig_ex, ext_pblock(ex));

	may_zeroout = ee_block + ee_len <= eof_block;

	if ((iblock == ee_block) && (allocated <= max_blocks))
		return allocated;

	err = ext4_ext_get_access(handle, inode, path + depth);
	if (err)
		goto out;
	 
	if (iblock > ee_block) {
		ex1 = ex;
		ex1->ee_len = cpu_to_le16(iblock - ee_block);
		ext4_ext_mark_uninitialized(ex1);
		ex2 = &newex;
	}
	 
	if (!ex1 && allocated > max_blocks)
		ex2->ee_len = cpu_to_le16(max_blocks);
	 
	if (allocated > max_blocks) {
		unsigned int newdepth;
		ex3 = &newex;
		ex3->ee_block = cpu_to_le32(iblock + max_blocks);
		ext4_ext_store_pblock(ex3, newblock + max_blocks);
		ex3->ee_len = cpu_to_le16(allocated - max_blocks);
		ext4_ext_mark_uninitialized(ex3);
		err = ext4_ext_insert_extent(handle, inode, path, ex3, flags);
		if (err == -ENOSPC && may_zeroout) {
			err =  ext4_ext_zeroout(inode, &orig_ex);
			if (err)
				goto fix_extent_len;
			 
			ex->ee_block = orig_ex.ee_block;
			ex->ee_len   = orig_ex.ee_len;
			ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
			ext4_ext_dirty(handle, inode, path + depth);
			 
			return allocated;

		} else if (err)
			goto fix_extent_len;
		 
		newdepth = ext_depth(inode);
		 
		ee_len -= ext4_ext_get_actual_len(ex3);
		orig_ex.ee_len = cpu_to_le16(ee_len);
		may_zeroout = ee_block + ee_len <= eof_block;

		depth = newdepth;
		ext4_ext_drop_refs(path);
		path = ext4_ext_find_extent(inode, iblock, path);
		if (IS_ERR(path)) {
			err = PTR_ERR(path);
			goto out;
		}
		eh = path[depth].p_hdr;
		ex = path[depth].p_ext;
		if (ex2 != &newex)
			ex2 = ex;

		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			goto out;

		allocated = max_blocks;
	}
	 
	if (ex1 && ex1 != ex) {
		ex1 = ex;
		ex1->ee_len = cpu_to_le16(iblock - ee_block);
		ext4_ext_mark_uninitialized(ex1);
		ex2 = &newex;
	}
	 
	ex2->ee_block = cpu_to_le32(iblock);
	ext4_ext_store_pblock(ex2, newblock);
	ex2->ee_len = cpu_to_le16(allocated);
	ext4_ext_mark_uninitialized(ex2);
	if (ex2 != ex)
		goto insert;
	 
	err = ext4_ext_dirty(handle, inode, path + depth);
	ext_debug("out here\n");
	goto out;
insert:
	err = ext4_ext_insert_extent(handle, inode, path, &newex, flags);
	if (err == -ENOSPC && may_zeroout) {
		err =  ext4_ext_zeroout(inode, &orig_ex);
		if (err)
			goto fix_extent_len;
		 
		ex->ee_block = orig_ex.ee_block;
		ex->ee_len   = orig_ex.ee_len;
		ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
		ext4_ext_dirty(handle, inode, path + depth);
		 
		return allocated;
	} else if (err)
		goto fix_extent_len;
out:
	ext4_ext_show_leaf(inode, path);
	return err ? err : allocated;

fix_extent_len:
	ex->ee_block = orig_ex.ee_block;
	ex->ee_len   = orig_ex.ee_len;
	ext4_ext_store_pblock(ex, ext_pblock(&orig_ex));
	ext4_ext_mark_uninitialized(ex);
	ext4_ext_dirty(handle, inode, path + depth);
	return err;
}
static int ext4_convert_unwritten_extents_dio(handle_t *handle,
					      struct inode *inode,
					      struct ext4_ext_path *path)
{
	struct ext4_extent *ex;
	struct ext4_extent_header *eh;
	int depth;
	int err = 0;
	int ret = 0;

	depth = ext_depth(inode);
	eh = path[depth].p_hdr;
	ex = path[depth].p_ext;

	err = ext4_ext_get_access(handle, inode, path + depth);
	if (err)
		goto out;
	 
	ext4_ext_mark_initialized(ex);

	if (ex > EXT_FIRST_EXTENT(eh)) {
		 
		ret = ext4_ext_try_to_merge(inode, path, ex - 1);
		if (ret) {
			err = ext4_ext_correct_indexes(handle, inode, path);
			if (err)
				goto out;
			depth = ext_depth(inode);
			ex--;
		}
	}
	 
	ret = ext4_ext_try_to_merge(inode, path, ex);
	if (ret) {
		err = ext4_ext_correct_indexes(handle, inode, path);
		if (err)
			goto out;
		depth = ext_depth(inode);
	}
	 
	err = ext4_ext_dirty(handle, inode, path + depth);
out:
	ext4_ext_show_leaf(inode, path);
	return err;
}

static void unmap_underlying_metadata_blocks(struct block_device *bdev,
			sector_t block, int count)
{
	int i;
	for (i = 0; i < count; i++)
                unmap_underlying_metadata(bdev, block + i);
}

static int
ext4_ext_handle_uninitialized_extents(handle_t *handle, struct inode *inode,
			ext4_lblk_t iblock, unsigned int max_blocks,
			struct ext4_ext_path *path, int flags,
			unsigned int allocated, struct buffer_head *bh_result,
			ext4_fsblk_t newblock)
{
	int ret = 0;
	int err = 0;
	ext4_io_end_t *io = EXT4_I(inode)->cur_aio_dio;

	ext_debug("ext4_ext_handle_uninitialized_extents: inode %lu, logical"
		  "block %llu, max_blocks %u, flags %d, allocated %u",
		  inode->i_ino, (unsigned long long)iblock, max_blocks,
		  flags, allocated);
	ext4_ext_show_leaf(inode, path);

	if (flags == EXT4_GET_BLOCKS_DIO_CREATE_EXT) {
		ret = ext4_split_unwritten_extents(handle,
						inode, path, iblock,
						max_blocks, flags);
		 
		if (io)
			io->flag = DIO_AIO_UNWRITTEN;
		else
			ext4_set_inode_state(inode, EXT4_STATE_DIO_UNWRITTEN);
		goto out;
	}
	 
	if (flags == EXT4_GET_BLOCKS_DIO_CONVERT_EXT) {
		ret = ext4_convert_unwritten_extents_dio(handle, inode,
							path);
		if (ret >= 0)
			ext4_update_inode_fsync_trans(handle, inode, 1);
		goto out2;
	}
	 
	if (flags & EXT4_GET_BLOCKS_UNINIT_EXT)
		goto map_out;

	if ((flags & EXT4_GET_BLOCKS_CREATE) == 0) {
		 
		set_buffer_unwritten(bh_result);
		goto out1;
	}

#if defined(CONFIG_SYNO_PLX_PORTING) && !defined(SYNO_FAST_RW_FIX)
	ret = ext4_ext_convert_to_initialized(handle, inode,
						path, iblock,
						max_blocks, 1);
#else
	ret = ext4_ext_convert_to_initialized(handle, inode,
						path, iblock,
						max_blocks);
#endif
	if (ret >= 0)
		ext4_update_inode_fsync_trans(handle, inode, 1);
out:
	if (ret <= 0) {
		err = ret;
		goto out2;
	} else
		allocated = ret;
	set_buffer_new(bh_result);
	 
	if (allocated > max_blocks) {
		unmap_underlying_metadata_blocks(inode->i_sb->s_bdev,
					newblock + max_blocks,
					allocated - max_blocks);
		allocated = max_blocks;
	}

	if (flags & EXT4_GET_BLOCKS_DELALLOC_RESERVE)
		ext4_da_update_reserve_space(inode, allocated, 0);

map_out:
	set_buffer_mapped(bh_result);
out1:
	if (allocated > max_blocks)
		allocated = max_blocks;
	ext4_ext_show_leaf(inode, path);
	bh_result->b_bdev = inode->i_sb->s_bdev;
	bh_result->b_blocknr = newblock;
out2:
	if (path) {
		ext4_ext_drop_refs(path);
		kfree(path);
	}
	return err ? err : allocated;
}
 
int ext4_ext_get_blocks(handle_t *handle, struct inode *inode,
			ext4_lblk_t iblock,
			unsigned int max_blocks, struct buffer_head *bh_result,
			int flags)
{
	struct ext4_ext_path *path = NULL;
	struct ext4_extent_header *eh;
	struct ext4_extent newex, *ex, *last_ex;
	ext4_fsblk_t newblock;
	int i, err = 0, depth, ret, cache_type;
	unsigned int allocated = 0;
	struct ext4_allocation_request ar;
	ext4_io_end_t *io = EXT4_I(inode)->cur_aio_dio;

	__clear_bit(BH_New, &bh_result->b_state);
	ext_debug("blocks %u/%u requested for inode %lu\n",
			iblock, max_blocks, inode->i_ino);

	cache_type = ext4_ext_in_cache(inode, iblock, &newex);
	if (cache_type) {
		if (cache_type == EXT4_EXT_CACHE_GAP) {
			if ((flags & EXT4_GET_BLOCKS_CREATE) == 0) {
				 
				goto out2;
			}
			 
		} else if (cache_type == EXT4_EXT_CACHE_EXTENT) {
			 
			newblock = iblock
				   - le32_to_cpu(newex.ee_block)
				   + ext_pblock(&newex);
			 
			allocated = ext4_ext_get_actual_len(&newex) -
					(iblock - le32_to_cpu(newex.ee_block));
			goto out;
		} else {
			BUG();
		}
	}

	path = ext4_ext_find_extent(inode, iblock, NULL);
	if (IS_ERR(path)) {
		err = PTR_ERR(path);
		path = NULL;
		goto out2;
	}

	depth = ext_depth(inode);

	if (path[depth].p_ext == NULL && depth != 0) {
		ext4_error(inode->i_sb, __func__, "bad extent address "
			   "inode: %lu, iblock: %lu, depth: %d",
			   inode->i_ino, (unsigned long) iblock, depth);
		err = -EIO;
		goto out2;
	}
	eh = path[depth].p_hdr;

	ex = path[depth].p_ext;
	if (ex) {
		ext4_lblk_t ee_block = le32_to_cpu(ex->ee_block);
		ext4_fsblk_t ee_start = ext_pblock(ex);
		unsigned short ee_len;

		ee_len = ext4_ext_get_actual_len(ex);
		 
		if (iblock >= ee_block && iblock < ee_block + ee_len) {
			newblock = iblock - ee_block + ee_start;
			 
			allocated = ee_len - (iblock - ee_block);
			ext_debug("%u fit into %u:%d -> %llu\n", iblock,
					ee_block, ee_len, newblock);

			if (!ext4_ext_is_uninitialized(ex)) {
				ext4_ext_put_in_cache(inode, ee_block,
							ee_len, ee_start,
							EXT4_EXT_CACHE_EXTENT);
				goto out;
			}
			ret = ext4_ext_handle_uninitialized_extents(handle,
					inode, iblock, max_blocks, path,
					flags, allocated, bh_result, newblock);
			return ret;
		}
	}

	if ((flags & EXT4_GET_BLOCKS_CREATE) == 0) {
		 
		ext4_ext_put_gap_in_cache(inode, path, iblock);
		goto out2;
	}
	 
	ar.lleft = iblock;
	err = ext4_ext_search_left(inode, path, &ar.lleft, &ar.pleft);
	if (err)
		goto out2;
	ar.lright = iblock;
	err = ext4_ext_search_right(inode, path, &ar.lright, &ar.pright);
	if (err)
		goto out2;

	if (max_blocks > EXT_INIT_MAX_LEN &&
	    !(flags & EXT4_GET_BLOCKS_UNINIT_EXT))
		max_blocks = EXT_INIT_MAX_LEN;
	else if (max_blocks > EXT_UNINIT_MAX_LEN &&
		 (flags & EXT4_GET_BLOCKS_UNINIT_EXT))
		max_blocks = EXT_UNINIT_MAX_LEN;

	newex.ee_block = cpu_to_le32(iblock);
	newex.ee_len = cpu_to_le16(max_blocks);
	err = ext4_ext_check_overlap(inode, &newex, path);
	if (err)
		allocated = ext4_ext_get_actual_len(&newex);
	else
		allocated = max_blocks;

	ar.inode = inode;
	ar.goal = ext4_ext_find_goal(inode, path, iblock);
	ar.logical = iblock;
	ar.len = allocated;
	if (S_ISREG(inode->i_mode))
		ar.flags = EXT4_MB_HINT_DATA;
	else
		 
		ar.flags = 0;
	newblock = ext4_mb_new_blocks(handle, &ar, &err);
	if (!newblock)
		goto out2;
#ifdef SYNO_FAST_RW_FIX
	ext_debug("inode %llu allocate new block: goal %llu, found %llu/%u\n",
		  inode->i_ino, ar.goal, newblock, allocated);
#else
	ext_debug("allocate new block: goal %llu, found %llu/%u\n",
		ar.goal, newblock, allocated);
#endif

	ext4_ext_store_pblock(&newex, newblock);
	newex.ee_len = cpu_to_le16(ar.len);
	 
	if (flags & EXT4_GET_BLOCKS_UNINIT_EXT){
		ext4_ext_mark_uninitialized(&newex);
		 
		if (flags == EXT4_GET_BLOCKS_DIO_CREATE_EXT) {
			if (io)
				io->flag = DIO_AIO_UNWRITTEN;
			else
				ext4_set_inode_state(inode,
						     EXT4_STATE_DIO_UNWRITTEN);
		}
	}

	if (unlikely(ext4_test_inode_flag(inode, EXT4_INODE_EOFBLOCKS))) {
		if (unlikely(!eh->eh_entries)) {
			ext4_error(inode->i_sb, __func__,
				   "inode#%lu, eh->eh_entries = 0 and "
				   "EOFBLOCKS_FL set", inode->i_ino);
			err = -EIO;
			goto out2;
		}
		last_ex = EXT_LAST_EXTENT(eh);
		 
		for (i = depth-1; i >= 0; i--) {
			if (path[i].p_idx != EXT_LAST_INDEX(path[i].p_hdr))
				break;
		}
		if ((i < 0) &&
		    (iblock + ar.len > le32_to_cpu(last_ex->ee_block) +
		     ext4_ext_get_actual_len(last_ex)))
			ext4_clear_inode_flag(inode, EXT4_INODE_EOFBLOCKS);
	}
	err = ext4_ext_insert_extent(handle, inode, path, &newex, flags);
	if (err) {
		 
		ext4_discard_preallocations(inode);
		ext4_free_blocks(handle, inode, ext_pblock(&newex),
					ext4_ext_get_actual_len(&newex), 0);
		goto out2;
	}

	newblock = ext_pblock(&newex);
	allocated = ext4_ext_get_actual_len(&newex);
	if (allocated > max_blocks)
		allocated = max_blocks;
	set_buffer_new(bh_result);

	if (flags & EXT4_GET_BLOCKS_DELALLOC_RESERVE)
		ext4_da_update_reserve_space(inode, allocated, 1);

	if ((flags & EXT4_GET_BLOCKS_UNINIT_EXT) == 0) {
		ext4_ext_put_in_cache(inode, iblock, allocated, newblock,
						EXT4_EXT_CACHE_EXTENT);
		ext4_update_inode_fsync_trans(handle, inode, 1);
	} else
		ext4_update_inode_fsync_trans(handle, inode, 0);
out:
	if (allocated > max_blocks)
		allocated = max_blocks;
	ext4_ext_show_leaf(inode, path);
	set_buffer_mapped(bh_result);
	bh_result->b_bdev = inode->i_sb->s_bdev;
	bh_result->b_blocknr = newblock;
out2:
	if (path) {
		ext4_ext_drop_refs(path);
		kfree(path);
	}
	return err ? err : allocated;
}

void ext4_ext_truncate(struct inode *inode)
{
	struct address_space *mapping = inode->i_mapping;
	struct super_block *sb = inode->i_sb;
	ext4_lblk_t last_block;
	handle_t *handle;
	int err = 0;

	err = ext4_writepage_trans_blocks(inode);
	handle = ext4_journal_start(inode, err);
	if (IS_ERR(handle))
		return;

	if (inode->i_size & (sb->s_blocksize - 1))
		ext4_block_truncate_page(handle, mapping, inode->i_size);

	if (ext4_orphan_add(handle, inode))
		goto out_stop;

	down_write(&EXT4_I(inode)->i_data_sem);
	ext4_ext_invalidate_cache(inode);

	ext4_discard_preallocations(inode);

	EXT4_I(inode)->i_disksize = inode->i_size;
	ext4_mark_inode_dirty(handle, inode);

	last_block = (inode->i_size + sb->s_blocksize - 1)
			>> EXT4_BLOCK_SIZE_BITS(sb);
	err = ext4_ext_remove_space(inode, last_block);

	if (IS_SYNC(inode))
		ext4_handle_sync(handle);

out_stop:
	up_write(&EXT4_I(inode)->i_data_sem);
	 
	if (inode->i_nlink)
		ext4_orphan_del(handle, inode);

	inode->i_mtime = inode->i_ctime = ext4_current_time(inode);
	ext4_mark_inode_dirty(handle, inode);
	ext4_journal_stop(handle);
}

static void ext4_falloc_update_inode(struct inode *inode,
				int mode, loff_t new_size, int update_ctime)
{
	struct timespec now;

	if (update_ctime) {
		now = current_fs_time(inode->i_sb);
		if (!timespec_equal(&inode->i_ctime, &now))
			inode->i_ctime = now;
	}
	 
	if (!(mode & FALLOC_FL_KEEP_SIZE)) {
		if (new_size > i_size_read(inode))
			i_size_write(inode, new_size);
		if (new_size > EXT4_I(inode)->i_disksize)
			ext4_update_i_disksize(inode, new_size);
	} else {
		 
		if (new_size > i_size_read(inode))
			ext4_set_inode_flag(inode, EXT4_INODE_EOFBLOCKS);
	}

}

#ifdef CONFIG_SYNO_PLX_PORTING
int ext4_preallocate(
	struct file	*filp,
	loff_t		 offset,
	loff_t		 len)
{
#ifdef SYNO_FAST_RW_FIX
	 
	return ext4_fallocate(filp->f_path.dentry->d_inode, 0, offset, len);
#else
 
	return ext4_fallocate(filp->f_path.dentry->d_inode, FALLOC_FL_KEEP_SIZE, offset, len);
#endif
}

int ext4_unpreallocate(
	struct file	*filp,
	loff_t		 offset,
	loff_t		 len)
{
#ifdef SYNO_FAST_RW_FIX
     
#else
 
#endif
	ext4_truncate(filp->f_path.dentry->d_inode);
	return 0;
}

int ext4_resetpreallocate(
	struct file	*filp,
	loff_t		 offset,
	loff_t		 len)
{
	handle_t *handle = ext4_journal_current_handle();
	struct inode * inode = filp->f_path.dentry->d_inode;
	struct ext4_ext_path *path = NULL;
	int ret, started = 0;
	int reset_credits;
	unsigned int blkbits = inode->i_sb->s_blocksize_bits;

	ext4_lblk_t startblock = offset >> blkbits;  
	int num_of_blocks = len >> blkbits;;  

	if(num_of_blocks == 0)  
		num_of_blocks = 1;

	if (!handle) {
		 
		reset_credits = ext4_chunk_trans_blocks(inode, num_of_blocks);
		handle = ext4_journal_start(inode, reset_credits);
		if (IS_ERR(handle)) {
			ret = PTR_ERR(handle);
			goto out;
		}
		started = 1;
	}

	down_write((&EXT4_I(inode)->i_data_sem));

	do {
		path = ext4_ext_find_extent(inode, startblock, path);
		if (IS_ERR(path)) {
			ret = PTR_ERR(path);
			path = NULL;
			goto out2;
		}

#ifdef SYNO_FAST_RW_FIX
		ret = ext4_ext_convert_to_initialized(handle, inode,
												path, startblock,
												num_of_blocks);
#else
		ret = ext4_ext_convert_to_initialized(handle, inode,
												path, startblock,
												num_of_blocks, 0);
#endif
 
		if (ret < 0)
			break;
		startblock += ret;
		num_of_blocks -= ret;
	} while(num_of_blocks > 0);

	if(path) {
		ext4_ext_drop_refs(path);
		kfree(path);
	}

	up_write((&EXT4_I(inode)->i_data_sem));

out2:
	if (started)
		ext4_journal_stop(handle);
out:
	return ret;
}
#endif

long ext4_fallocate(struct inode *inode, int mode, loff_t offset, loff_t len)
{
	handle_t *handle;
	ext4_lblk_t block;
	loff_t new_size;
	unsigned int max_blocks;
	int ret = 0;
	int ret2 = 0;
	int retries = 0;
	struct buffer_head map_bh;
	unsigned int credits, blkbits = inode->i_blkbits;

	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS)))
		return -EOPNOTSUPP;

	if (S_ISDIR(inode->i_mode))
		return -ENODEV;

	block = offset >> blkbits;
	 
	max_blocks = (EXT4_BLOCK_ALIGN(len + offset, blkbits) >> blkbits)
							- block;
	 
	credits = ext4_chunk_trans_blocks(inode, max_blocks);
	mutex_lock(&inode->i_mutex);
	ret = inode_newsize_ok(inode, (len + offset));
	if (ret) {
		mutex_unlock(&inode->i_mutex);
		return ret;
	}
retry:
	while (ret >= 0 && ret < max_blocks) {
		block = block + ret;
		max_blocks = max_blocks - ret;
		handle = ext4_journal_start(inode, credits);
		if (IS_ERR(handle)) {
			ret = PTR_ERR(handle);
			break;
		}
		map_bh.b_state = 0;
		ret = ext4_get_blocks(handle, inode, block,
				      max_blocks, &map_bh,
				      EXT4_GET_BLOCKS_CREATE_UNINIT_EXT);
		if (ret <= 0) {
#ifdef EXT4FS_DEBUG
			WARN_ON(ret <= 0);
			printk(KERN_ERR "%s: ext4_ext_get_blocks "
				    "returned error inode#%lu, block=%u, "
				    "max_blocks=%u", __func__,
				    inode->i_ino, block, max_blocks);
#endif
			ext4_mark_inode_dirty(handle, inode);
			ret2 = ext4_journal_stop(handle);
			break;
		}
		if ((block + ret) >= (EXT4_BLOCK_ALIGN(offset + len,
						blkbits) >> blkbits))
			new_size = offset + len;
		else
			new_size = (block + ret) << blkbits;

		ext4_falloc_update_inode(inode, mode, new_size,
						buffer_new(&map_bh));
		ext4_mark_inode_dirty(handle, inode);
		ret2 = ext4_journal_stop(handle);
		if (ret2)
			break;
	}
	if (ret == -ENOSPC &&
			ext4_should_retry_alloc(inode->i_sb, &retries)) {
		ret = 0;
		goto retry;
	}
	mutex_unlock(&inode->i_mutex);
	return ret > 0 ? ret2 : ret;
}

int ext4_convert_unwritten_extents(struct inode *inode, loff_t offset,
				    ssize_t len)
{
	handle_t *handle;
	ext4_lblk_t block;
	unsigned int max_blocks;
	int ret = 0;
	int ret2 = 0;
	struct buffer_head map_bh;
	unsigned int credits, blkbits = inode->i_blkbits;

	block = offset >> blkbits;
	 
	max_blocks = (EXT4_BLOCK_ALIGN(len + offset, blkbits) >> blkbits)
							- block;
	 
	credits = ext4_chunk_trans_blocks(inode, max_blocks);
	while (ret >= 0 && ret < max_blocks) {
		block = block + ret;
		max_blocks = max_blocks - ret;
		handle = ext4_journal_start(inode, credits);
		if (IS_ERR(handle)) {
			ret = PTR_ERR(handle);
			break;
		}
		map_bh.b_state = 0;
		ret = ext4_get_blocks(handle, inode, block,
				      max_blocks, &map_bh,
				      EXT4_GET_BLOCKS_DIO_CONVERT_EXT);
		if (ret <= 0) {
			WARN_ON(ret <= 0);
			printk(KERN_ERR "%s: ext4_ext_get_blocks "
				    "returned error inode#%lu, block=%u, "
				    "max_blocks=%u", __func__,
				    inode->i_ino, block, max_blocks);
		}
		ext4_mark_inode_dirty(handle, inode);
		ret2 = ext4_journal_stop(handle);
		if (ret <= 0 || ret2 )
			break;
	}
	return ret > 0 ? ret2 : ret;
}
 
static int ext4_ext_fiemap_cb(struct inode *inode, struct ext4_ext_path *path,
		       struct ext4_ext_cache *newex, struct ext4_extent *ex,
		       void *data)
{
	struct fiemap_extent_info *fieinfo = data;
	unsigned char blksize_bits = inode->i_sb->s_blocksize_bits;
	__u64	logical;
	__u64	physical;
	__u64	length;
	__u32	flags = 0;
	int	error;

	logical =  (__u64)newex->ec_block << blksize_bits;

	if (newex->ec_type == EXT4_EXT_CACHE_GAP) {
		pgoff_t offset;
		struct page *page;
		struct buffer_head *bh = NULL;

		offset = logical >> PAGE_SHIFT;
		page = find_get_page(inode->i_mapping, offset);
		if (!page || !page_has_buffers(page))
			return EXT_CONTINUE;

		bh = page_buffers(page);

		if (!bh)
			return EXT_CONTINUE;

		if (buffer_delay(bh)) {
			flags |= FIEMAP_EXTENT_DELALLOC;
			page_cache_release(page);
		} else {
			page_cache_release(page);
			return EXT_CONTINUE;
		}
	}

	physical = (__u64)newex->ec_start << blksize_bits;
	length =   (__u64)newex->ec_len << blksize_bits;

	if (ex && ext4_ext_is_uninitialized(ex))
		flags |= FIEMAP_EXTENT_UNWRITTEN;

	if (ext4_ext_next_allocated_block(path) == EXT_MAX_BLOCK ||
	    newex->ec_block + newex->ec_len - 1 == EXT_MAX_BLOCK) {
		loff_t size = i_size_read(inode);
		loff_t bs = EXT4_BLOCK_SIZE(inode->i_sb);

		flags |= FIEMAP_EXTENT_LAST;
		if ((flags & FIEMAP_EXTENT_DELALLOC) &&
		    logical+length > size)
			length = (size - logical + bs - 1) & ~(bs-1);
	}

	error = fiemap_fill_next_extent(fieinfo, logical, physical,
					length, flags);
	if (error < 0)
		return error;
	if (error == 1)
		return EXT_BREAK;

	return EXT_CONTINUE;
}

#ifdef CONFIG_SYNO_PLX_PORTING
#define EXT4_FIEMAP_FLAGS	(FIEMAP_FLAG_SYNC|FIEMAP_FLAG_XATTR|FIEMAP_KERNEL_READ)
#else
#define EXT4_FIEMAP_FLAGS	(FIEMAP_FLAG_SYNC|FIEMAP_FLAG_XATTR)
#endif

static int ext4_xattr_fiemap(struct inode *inode,
				struct fiemap_extent_info *fieinfo)
{
	__u64 physical = 0;
	__u64 length;
	__u32 flags = FIEMAP_EXTENT_LAST;
	int blockbits = inode->i_sb->s_blocksize_bits;
	int error = 0;

	if (ext4_test_inode_state(inode, EXT4_STATE_XATTR)) {
		struct ext4_iloc iloc;
		int offset;	 

		error = ext4_get_inode_loc(inode, &iloc);
		if (error)
			return error;
		physical = iloc.bh->b_blocknr << blockbits;
		offset = EXT4_GOOD_OLD_INODE_SIZE +
				EXT4_I(inode)->i_extra_isize;
		physical += offset;
		length = EXT4_SB(inode->i_sb)->s_inode_size - offset;
		flags |= FIEMAP_EXTENT_DATA_INLINE;
		brelse(iloc.bh);
	} else {  
		physical = EXT4_I(inode)->i_file_acl << blockbits;
		length = inode->i_sb->s_blocksize;
	}

	if (physical)
		error = fiemap_fill_next_extent(fieinfo, 0, physical,
						length, flags);
	return (error < 0 ? error : 0);
}

int ext4_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		__u64 start, __u64 len)
{
	ext4_lblk_t start_blk;
	int error = 0;

	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS)))
		return generic_block_fiemap(inode, fieinfo, start, len,
			ext4_get_block);

	if (fiemap_check_flags(fieinfo, EXT4_FIEMAP_FLAGS))
		return -EBADR;

	if (fieinfo->fi_flags & FIEMAP_FLAG_XATTR) {
		error = ext4_xattr_fiemap(inode, fieinfo);
	} else {
		ext4_lblk_t len_blks;
		__u64 last_blk;

		start_blk = start >> inode->i_sb->s_blocksize_bits;
		last_blk = (start + len - 1) >> inode->i_sb->s_blocksize_bits;
		if (last_blk >= EXT_MAX_BLOCK)
			last_blk = EXT_MAX_BLOCK-1;
		len_blks = ((ext4_lblk_t) last_blk) - start_blk + 1;

		error = ext4_ext_walk_space(inode, start_blk, len_blks,
					  ext4_ext_fiemap_cb, fieinfo);
	}

	return error;
}

#ifdef CONFIG_SYNO_PLX_PORTING
#define BBSHIFT							9
#define BBSIZE							(1<<BBSHIFT)
#define BTOBB(bytes)					(((__u64)(bytes) + BBSIZE - 1) >> BBSHIFT)
#define BMV_OF_LAST  					0x4
#define GETBMAPX_BLOCK_HOLE             (-1LL)

int ext4_getbmapx(struct inode *inode, struct getbmapx *bmx)
{
	int			  count = 0;
	int			  error = 0;
	int 		  bmv_count;
	struct fiemap_extent_info fileinfo;
	struct getbmapx * map;
	struct fiemap_extent * fie_map;
	__u64 start = 0;
	__u64 len = FIEMAP_MAX_OFFSET;  
	__u64 cum_len = 0;  

	if (bmx->bmv_count < 2)
		return -EINVAL;

	filemap_write_and_wait(inode->i_mapping);

	bmv_count = bmx->bmv_count - 1;
	 
	fileinfo.fi_extents_max = bmx->bmv_count - 1;
	fileinfo.fi_flags = FIEMAP_KERNEL_READ;
	fileinfo.fi_extents_mapped = 0;

	fileinfo.fi_extents_start = kzalloc(sizeof(struct fiemap_extent) * (fileinfo.fi_extents_max), GFP_KERNEL);
	if (!fileinfo.fi_extents_start) {
#ifdef SYNO_FAST_RW_FIX
	printk (KERN_INFO "ext4_getbmapx - no memory, filemap_extent size:[%lu] extents_max:[%lu]\n", sizeof(struct fiemap_extent), fileinfo.fi_extents_max);
#else
	printk (KERN_INFO "ext4_getbmapx - no memory\n");
#endif
		return -ENOMEM;
	}

	error = ext4_fiemap(inode, &fileinfo, start, len);

	map = bmx;
	fie_map = fileinfo.fi_extents_start;
	map ++;  
	 
	while ( (count < fileinfo.fi_extents_mapped) && (bmx->bmv_entries < bmv_count) ){
		 
		if(cum_len < BTOBB (fie_map->fe_logical)) {
			printk(KERN_INFO "ext4 : found hole in filemap \n");
			map->bmv_offset = cum_len;
			map->bmv_length = (BTOBB (fie_map->fe_logical)) - cum_len;
			map->bmv_block = GETBMAPX_BLOCK_HOLE;
			cum_len += map->bmv_length;
			bmx->bmv_entries ++;
			map ++;
			continue;
		}

		map->bmv_offset = BTOBB (fie_map->fe_logical);
		map->bmv_block = BTOBB (fie_map->fe_physical);
		map->bmv_length = BTOBB (fie_map->fe_length);

		if(fie_map->fe_flags & FIEMAP_EXTENT_UNWRITTEN)
			map->bmv_oflags = GETBMAPX_OF_PREALLOC;

		if(fie_map->fe_flags & FIEMAP_EXTENT_LAST)
			map->bmv_oflags |= BMV_OF_LAST;

		cum_len += map->bmv_length;
		bmx->bmv_entries ++;
		map ++;

		fie_map ++;
		count ++;
	}

	kfree(fileinfo.fi_extents_start);
	fileinfo.fi_extents_start = NULL;
	return error;
}

int ext4_get_extents(struct inode *inode, loff_t size)
{
	 
	int			  retval = 0;
	struct fiemap_extent_info fileinfo;
	__u64 start = 0;
	__u64 len = FIEMAP_MAX_OFFSET;  

	fileinfo.fi_extents_max = 0;  
	fileinfo.fi_flags = FIEMAP_KERNEL_READ;
	fileinfo.fi_extents_mapped = 0;
	fileinfo.fi_extents_start = NULL;

	retval = ext4_fiemap(inode, &fileinfo, start, len);

	if(retval)
		return -EINVAL;

	retval = fileinfo.fi_extents_mapped;
	return retval;
}
#endif
