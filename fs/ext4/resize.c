#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define EXT4FS_DEBUG

#include <linux/errno.h>
#include <linux/slab.h>

#include "ext4_jbd2.h"

#define outside(b, first, last)	((b) < (first) || (b) >= (last))
#define inside(b, first, last)	((b) >= (first) && (b) < (last))

static int verify_group_input(struct super_block *sb,
			      struct ext4_new_group_data *input)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_super_block *es = sbi->s_es;
	ext4_fsblk_t start = ext4_blocks_count(es);
	ext4_fsblk_t end = start + input->blocks_count;
	ext4_group_t group = input->group;
	ext4_fsblk_t itend = input->inode_table + sbi->s_itb_per_group;
	unsigned overhead = ext4_bg_has_super(sb, group) ?
		(1 + ext4_bg_num_gdb(sb, group) +
		 le16_to_cpu(es->s_reserved_gdt_blocks)) : 0;
	ext4_fsblk_t metaend = start + overhead;
	struct buffer_head *bh = NULL;
	ext4_grpblk_t free_blocks_count, offset;
	int err = -EINVAL;

	input->free_blocks_count = free_blocks_count =
		input->blocks_count - 2 - overhead - sbi->s_itb_per_group;

	if (test_opt(sb, DEBUG))
		printk(KERN_DEBUG "EXT4-fs: adding %s group %u: %u blocks "
		       "(%d free, %u reserved)\n",
		       ext4_bg_has_super(sb, input->group) ? "normal" :
		       "no-super", input->group, input->blocks_count,
		       free_blocks_count, input->reserved_blocks);

	ext4_get_group_no_and_offset(sb, start, NULL, &offset);
	if (group != sbi->s_groups_count)
		ext4_warning(sb, __func__,
			     "Cannot add at group %u (only %u groups)",
			     input->group, sbi->s_groups_count);
	else if (offset != 0)
			ext4_warning(sb, __func__, "Last group not full");
	else if (input->reserved_blocks > input->blocks_count / 5)
		ext4_warning(sb, __func__, "Reserved blocks too high (%u)",
			     input->reserved_blocks);
	else if (free_blocks_count < 0)
		ext4_warning(sb, __func__, "Bad blocks count %u",
			     input->blocks_count);
	else if (!(bh = sb_bread(sb, end - 1)))
		ext4_warning(sb, __func__,
			     "Cannot read last block (%llu)",
			     end - 1);
	else if (outside(input->block_bitmap, start, end))
		ext4_warning(sb, __func__,
			     "Block bitmap not in group (block %llu)",
			     (unsigned long long)input->block_bitmap);
	else if (outside(input->inode_bitmap, start, end))
		ext4_warning(sb, __func__,
			     "Inode bitmap not in group (block %llu)",
			     (unsigned long long)input->inode_bitmap);
	else if (outside(input->inode_table, start, end) ||
		 outside(itend - 1, start, end))
		ext4_warning(sb, __func__,
			     "Inode table not in group (blocks %llu-%llu)",
			     (unsigned long long)input->inode_table, itend - 1);
	else if (input->inode_bitmap == input->block_bitmap)
		ext4_warning(sb, __func__,
			     "Block bitmap same as inode bitmap (%llu)",
			     (unsigned long long)input->block_bitmap);
	else if (inside(input->block_bitmap, input->inode_table, itend))
		ext4_warning(sb, __func__,
			     "Block bitmap (%llu) in inode table (%llu-%llu)",
			     (unsigned long long)input->block_bitmap,
			     (unsigned long long)input->inode_table, itend - 1);
	else if (inside(input->inode_bitmap, input->inode_table, itend))
		ext4_warning(sb, __func__,
			     "Inode bitmap (%llu) in inode table (%llu-%llu)",
			     (unsigned long long)input->inode_bitmap,
			     (unsigned long long)input->inode_table, itend - 1);
	else if (inside(input->block_bitmap, start, metaend))
		ext4_warning(sb, __func__,
			     "Block bitmap (%llu) in GDT table"
			     " (%llu-%llu)",
			     (unsigned long long)input->block_bitmap,
			     start, metaend - 1);
	else if (inside(input->inode_bitmap, start, metaend))
		ext4_warning(sb, __func__,
			     "Inode bitmap (%llu) in GDT table"
			     " (%llu-%llu)",
			     (unsigned long long)input->inode_bitmap,
			     start, metaend - 1);
	else if (inside(input->inode_table, start, metaend) ||
		 inside(itend - 1, start, metaend))
		ext4_warning(sb, __func__,
			     "Inode table (%llu-%llu) overlaps"
			     "GDT table (%llu-%llu)",
			     (unsigned long long)input->inode_table,
			     itend - 1, start, metaend - 1);
	else
		err = 0;
	brelse(bh);

	return err;
}

static struct buffer_head *bclean(handle_t *handle, struct super_block *sb,
				  ext4_fsblk_t blk)
{
	struct buffer_head *bh;
	int err;

	bh = sb_getblk(sb, blk);
	if (!bh)
		return ERR_PTR(-EIO);
	if ((err = ext4_journal_get_write_access(handle, bh))) {
		brelse(bh);
		bh = ERR_PTR(err);
	} else {
		lock_buffer(bh);
		memset(bh->b_data, 0, sb->s_blocksize);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
	}

	return bh;
}

static int extend_or_restart_transaction(handle_t *handle, int thresh,
					 struct buffer_head *bh)
{
	int err;

	if (ext4_handle_has_enough_credits(handle, thresh))
		return 0;

	err = ext4_journal_extend(handle, EXT4_MAX_TRANS_DATA);
	if (err < 0)
		return err;
	if (err) {
		if ((err = ext4_journal_restart(handle, EXT4_MAX_TRANS_DATA)))
			return err;
		if ((err = ext4_journal_get_write_access(handle, bh)))
			return err;
	}

	return 0;
}

static int setup_new_group_blocks(struct super_block *sb,
				  struct ext4_new_group_data *input)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	ext4_fsblk_t start = ext4_group_first_block_no(sb, input->group);
	int reserved_gdb = ext4_bg_has_super(sb, input->group) ?
		le16_to_cpu(sbi->s_es->s_reserved_gdt_blocks) : 0;
	unsigned long gdblocks = ext4_bg_num_gdb(sb, input->group);
	struct buffer_head *bh;
	handle_t *handle;
	ext4_fsblk_t block;
	ext4_grpblk_t bit;
	int i;
	int err = 0, err2;

	handle = ext4_journal_start_sb(sb, EXT4_MAX_TRANS_DATA);

	if (IS_ERR(handle))
		return PTR_ERR(handle);

	mutex_lock(&sbi->s_resize_lock);
	if (input->group != sbi->s_groups_count) {
		err = -EBUSY;
		goto exit_journal;
	}

	if (IS_ERR(bh = bclean(handle, sb, input->block_bitmap))) {
		err = PTR_ERR(bh);
		goto exit_journal;
	}

	if (ext4_bg_has_super(sb, input->group)) {
		ext4_debug("mark backup superblock %#04llx (+0)\n", start);
		ext4_set_bit(0, bh->b_data);
	}

	for (i = 0, bit = 1, block = start + 1;
	     i < gdblocks; i++, block++, bit++) {
		struct buffer_head *gdb;

		ext4_debug("update backup group %#04llx (+%d)\n", block, bit);

		if ((err = extend_or_restart_transaction(handle, 1, bh)))
			goto exit_bh;

		gdb = sb_getblk(sb, block);
		if (!gdb) {
			err = -EIO;
			goto exit_bh;
		}
		if ((err = ext4_journal_get_write_access(handle, gdb))) {
			brelse(gdb);
			goto exit_bh;
		}
		lock_buffer(gdb);
		memcpy(gdb->b_data, sbi->s_group_desc[i]->b_data, gdb->b_size);
		set_buffer_uptodate(gdb);
		unlock_buffer(gdb);
		ext4_handle_dirty_metadata(handle, NULL, gdb);
		ext4_set_bit(bit, bh->b_data);
		brelse(gdb);
	}

	for (i = 0, bit = gdblocks + 1, block = start + bit;
	     i < reserved_gdb; i++, block++, bit++) {
		struct buffer_head *gdb;

		ext4_debug("clear reserved block %#04llx (+%d)\n", block, bit);

		if ((err = extend_or_restart_transaction(handle, 1, bh)))
			goto exit_bh;

		if (IS_ERR(gdb = bclean(handle, sb, block))) {
			err = PTR_ERR(gdb);
			goto exit_bh;
		}
		ext4_handle_dirty_metadata(handle, NULL, gdb);
		ext4_set_bit(bit, bh->b_data);
		brelse(gdb);
	}
	ext4_debug("mark block bitmap %#04llx (+%llu)\n", input->block_bitmap,
		   input->block_bitmap - start);
	ext4_set_bit(input->block_bitmap - start, bh->b_data);
	ext4_debug("mark inode bitmap %#04llx (+%llu)\n", input->inode_bitmap,
		   input->inode_bitmap - start);
	ext4_set_bit(input->inode_bitmap - start, bh->b_data);

	for (i = 0, block = input->inode_table, bit = block - start;
	     i < sbi->s_itb_per_group; i++, bit++, block++) {
		struct buffer_head *it;

		ext4_debug("clear inode block %#04llx (+%d)\n", block, bit);

		if ((err = extend_or_restart_transaction(handle, 1, bh)))
			goto exit_bh;

		if (IS_ERR(it = bclean(handle, sb, block))) {
			err = PTR_ERR(it);
			goto exit_bh;
		}
		ext4_handle_dirty_metadata(handle, NULL, it);
		brelse(it);
		ext4_set_bit(bit, bh->b_data);
	}

	if ((err = extend_or_restart_transaction(handle, 2, bh)))
		goto exit_bh;

	mark_bitmap_end(input->blocks_count, sb->s_blocksize * 8, bh->b_data);
	ext4_handle_dirty_metadata(handle, NULL, bh);
	brelse(bh);
	 
	ext4_debug("clear inode bitmap %#04llx (+%llu)\n",
		   input->inode_bitmap, input->inode_bitmap - start);
	if (IS_ERR(bh = bclean(handle, sb, input->inode_bitmap))) {
		err = PTR_ERR(bh);
		goto exit_journal;
	}

	mark_bitmap_end(EXT4_INODES_PER_GROUP(sb), sb->s_blocksize * 8,
			bh->b_data);
	ext4_handle_dirty_metadata(handle, NULL, bh);
exit_bh:
	brelse(bh);

exit_journal:
	mutex_unlock(&sbi->s_resize_lock);
	if ((err2 = ext4_journal_stop(handle)) && !err)
		err = err2;

	return err;
}

static unsigned ext4_list_backups(struct super_block *sb, unsigned *three,
				  unsigned *five, unsigned *seven)
{
	unsigned *min = three;
	int mult = 3;
	unsigned ret;

	if (!EXT4_HAS_RO_COMPAT_FEATURE(sb,
					EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
		ret = *min;
		*min += 1;
		return ret;
	}

	if (*five < *min) {
		min = five;
		mult = 5;
	}
	if (*seven < *min) {
		min = seven;
		mult = 7;
	}

	ret = *min;
	*min *= mult;

	return ret;
}

static int verify_reserved_gdb(struct super_block *sb,
			       struct buffer_head *primary)
{
	const ext4_fsblk_t blk = primary->b_blocknr;
	const ext4_group_t end = EXT4_SB(sb)->s_groups_count;
	unsigned three = 1;
	unsigned five = 5;
	unsigned seven = 7;
	unsigned grp;
	__le32 *p = (__le32 *)primary->b_data;
	int gdbackups = 0;

	while ((grp = ext4_list_backups(sb, &three, &five, &seven)) < end) {
		if (le32_to_cpu(*p++) !=
		    grp * EXT4_BLOCKS_PER_GROUP(sb) + blk){
			ext4_warning(sb, __func__,
				     "reserved GDT %llu"
				     " missing grp %d (%llu)",
				     blk, grp,
				     grp *
				     (ext4_fsblk_t)EXT4_BLOCKS_PER_GROUP(sb) +
				     blk);
			return -EINVAL;
		}
		if (++gdbackups > EXT4_ADDR_PER_BLOCK(sb))
			return -EFBIG;
	}

	return gdbackups;
}

static int add_new_gdb(handle_t *handle, struct inode *inode,
		       struct ext4_new_group_data *input,
		       struct buffer_head **primary)
{
	struct super_block *sb = inode->i_sb;
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;
	unsigned long gdb_num = input->group / EXT4_DESC_PER_BLOCK(sb);
	ext4_fsblk_t gdblock = EXT4_SB(sb)->s_sbh->b_blocknr + 1 + gdb_num;
	struct buffer_head **o_group_desc, **n_group_desc;
	struct buffer_head *dind;
	int gdbackups;
	struct ext4_iloc iloc;
	__le32 *data;
	int err;

	if (test_opt(sb, DEBUG))
		printk(KERN_DEBUG
		       "EXT4-fs: ext4_add_new_gdb: adding group block %lu\n",
		       gdb_num);

	if (EXT4_SB(sb)->s_sbh->b_blocknr !=
	    le32_to_cpu(EXT4_SB(sb)->s_es->s_first_data_block)) {
		ext4_warning(sb, __func__,
			"won't resize using backup superblock at %llu",
			(unsigned long long)EXT4_SB(sb)->s_sbh->b_blocknr);
		return -EPERM;
	}

	*primary = sb_bread(sb, gdblock);
	if (!*primary)
		return -EIO;

	if ((gdbackups = verify_reserved_gdb(sb, *primary)) < 0) {
		err = gdbackups;
		goto exit_bh;
	}

	data = EXT4_I(inode)->i_data + EXT4_DIND_BLOCK;
	dind = sb_bread(sb, le32_to_cpu(*data));
	if (!dind) {
		err = -EIO;
		goto exit_bh;
	}

	data = (__le32 *)dind->b_data;
	if (le32_to_cpu(data[gdb_num % EXT4_ADDR_PER_BLOCK(sb)]) != gdblock) {
		ext4_warning(sb, __func__,
			     "new group %u GDT block %llu not reserved",
			     input->group, gdblock);
		err = -EINVAL;
		goto exit_dind;
	}

	if ((err = ext4_journal_get_write_access(handle, EXT4_SB(sb)->s_sbh)))
		goto exit_dind;

	if ((err = ext4_journal_get_write_access(handle, *primary)))
		goto exit_sbh;

	if ((err = ext4_journal_get_write_access(handle, dind)))
		goto exit_primary;

	if ((err = ext4_reserve_inode_write(handle, inode, &iloc)))
		goto exit_dindj;

	n_group_desc = kmalloc((gdb_num + 1) * sizeof(struct buffer_head *),
			GFP_NOFS);
	if (!n_group_desc) {
		err = -ENOMEM;
		ext4_warning(sb, __func__,
			      "not enough memory for %lu groups", gdb_num + 1);
		goto exit_inode;
	}

	data[gdb_num % EXT4_ADDR_PER_BLOCK(sb)] = 0;
	ext4_handle_dirty_metadata(handle, NULL, dind);
	brelse(dind);
	inode->i_blocks -= (gdbackups + 1) * sb->s_blocksize >> 9;
	ext4_mark_iloc_dirty(handle, inode, &iloc);
	memset((*primary)->b_data, 0, sb->s_blocksize);
	ext4_handle_dirty_metadata(handle, NULL, *primary);

	o_group_desc = EXT4_SB(sb)->s_group_desc;
	memcpy(n_group_desc, o_group_desc,
	       EXT4_SB(sb)->s_gdb_count * sizeof(struct buffer_head *));
	n_group_desc[gdb_num] = *primary;
	EXT4_SB(sb)->s_group_desc = n_group_desc;
	EXT4_SB(sb)->s_gdb_count++;
	kfree(o_group_desc);

	le16_add_cpu(&es->s_reserved_gdt_blocks, -1);
	ext4_handle_dirty_metadata(handle, NULL, EXT4_SB(sb)->s_sbh);

	return 0;

exit_inode:
	 
	brelse(iloc.bh);
exit_dindj:
	 
exit_primary:
	 
exit_sbh:
	 
exit_dind:
	brelse(dind);
exit_bh:
	brelse(*primary);

	ext4_debug("leaving with error %d\n", err);
	return err;
}

static int reserve_backup_gdb(handle_t *handle, struct inode *inode,
			      struct ext4_new_group_data *input)
{
	struct super_block *sb = inode->i_sb;
	int reserved_gdb =le16_to_cpu(EXT4_SB(sb)->s_es->s_reserved_gdt_blocks);
	struct buffer_head **primary;
	struct buffer_head *dind;
	struct ext4_iloc iloc;
	ext4_fsblk_t blk;
	__le32 *data, *end;
	int gdbackups = 0;
	int res, i;
	int err;

	primary = kmalloc(reserved_gdb * sizeof(*primary), GFP_NOFS);
	if (!primary)
		return -ENOMEM;

	data = EXT4_I(inode)->i_data + EXT4_DIND_BLOCK;
	dind = sb_bread(sb, le32_to_cpu(*data));
	if (!dind) {
		err = -EIO;
		goto exit_free;
	}

	blk = EXT4_SB(sb)->s_sbh->b_blocknr + 1 + EXT4_SB(sb)->s_gdb_count;
	data = (__le32 *)dind->b_data + (EXT4_SB(sb)->s_gdb_count %
					 EXT4_ADDR_PER_BLOCK(sb));
	end = (__le32 *)dind->b_data + EXT4_ADDR_PER_BLOCK(sb);

	for (res = 0; res < reserved_gdb; res++, blk++) {
		if (le32_to_cpu(*data) != blk) {
			ext4_warning(sb, __func__,
				     "reserved block %llu"
				     " not at offset %ld",
				     blk,
				     (long)(data - (__le32 *)dind->b_data));
			err = -EINVAL;
			goto exit_bh;
		}
		primary[res] = sb_bread(sb, blk);
		if (!primary[res]) {
			err = -EIO;
			goto exit_bh;
		}
		if ((gdbackups = verify_reserved_gdb(sb, primary[res])) < 0) {
			brelse(primary[res]);
			err = gdbackups;
			goto exit_bh;
		}
		if (++data >= end)
			data = (__le32 *)dind->b_data;
	}

	for (i = 0; i < reserved_gdb; i++) {
		if ((err = ext4_journal_get_write_access(handle, primary[i]))) {
			 
			goto exit_bh;
		}
	}

	if ((err = ext4_reserve_inode_write(handle, inode, &iloc)))
		goto exit_bh;

	blk = input->group * EXT4_BLOCKS_PER_GROUP(sb);
	for (i = 0; i < reserved_gdb; i++) {
		int err2;
		data = (__le32 *)primary[i]->b_data;
		 
		data[gdbackups] = cpu_to_le32(blk + primary[i]->b_blocknr);
		err2 = ext4_handle_dirty_metadata(handle, NULL, primary[i]);
		if (!err)
			err = err2;
	}
	inode->i_blocks += reserved_gdb * sb->s_blocksize >> 9;
	ext4_mark_iloc_dirty(handle, inode, &iloc);

exit_bh:
	while (--res >= 0)
		brelse(primary[res]);
	brelse(dind);

exit_free:
	kfree(primary);

	return err;
}

static void update_backups(struct super_block *sb,
			   int blk_off, char *data, int size)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	const ext4_group_t last = sbi->s_groups_count;
	const int bpg = EXT4_BLOCKS_PER_GROUP(sb);
	unsigned three = 1;
	unsigned five = 5;
	unsigned seven = 7;
	ext4_group_t group;
	int rest = sb->s_blocksize - size;
	handle_t *handle;
	int err = 0, err2;

	handle = ext4_journal_start_sb(sb, EXT4_MAX_TRANS_DATA);
	if (IS_ERR(handle)) {
		group = 1;
		err = PTR_ERR(handle);
		goto exit_err;
	}

	while ((group = ext4_list_backups(sb, &three, &five, &seven)) < last) {
		struct buffer_head *bh;

		if (ext4_handle_valid(handle) &&
		    handle->h_buffer_credits == 0 &&
		    ext4_journal_extend(handle, EXT4_MAX_TRANS_DATA) &&
		    (err = ext4_journal_restart(handle, EXT4_MAX_TRANS_DATA)))
			break;

		bh = sb_getblk(sb, group * bpg + blk_off);
		if (!bh) {
			err = -EIO;
			break;
		}
		ext4_debug("update metadata backup %#04lx\n",
			  (unsigned long)bh->b_blocknr);
		if ((err = ext4_journal_get_write_access(handle, bh)))
			break;
		lock_buffer(bh);
		memcpy(bh->b_data, data, size);
		if (rest)
			memset(bh->b_data + size, 0, rest);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		ext4_handle_dirty_metadata(handle, NULL, bh);
		brelse(bh);
	}
	if ((err2 = ext4_journal_stop(handle)) && !err)
		err = err2;

exit_err:
	if (err) {
		ext4_warning(sb, __func__,
			     "can't update backup for group %u (err %d), "
			     "forcing fsck on next reboot", group, err);
		sbi->s_mount_state &= ~EXT4_VALID_FS;
		sbi->s_es->s_state &= cpu_to_le16(~EXT4_VALID_FS);
		mark_buffer_dirty(sbi->s_sbh);
	}
}

int ext4_group_add(struct super_block *sb, struct ext4_new_group_data *input)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_super_block *es = sbi->s_es;
	int reserved_gdb = ext4_bg_has_super(sb, input->group) ?
		le16_to_cpu(es->s_reserved_gdt_blocks) : 0;
	struct buffer_head *primary = NULL;
	struct ext4_group_desc *gdp;
	struct inode *inode = NULL;
	handle_t *handle;
	int gdb_off, gdb_num;
	int err, err2;

	gdb_num = input->group / EXT4_DESC_PER_BLOCK(sb);
	gdb_off = input->group % EXT4_DESC_PER_BLOCK(sb);

	if (gdb_off == 0 && !EXT4_HAS_RO_COMPAT_FEATURE(sb,
					EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
		ext4_warning(sb, __func__,
			     "Can't resize non-sparse filesystem further");
		return -EPERM;
	}

	if (ext4_blocks_count(es) + input->blocks_count <
	    ext4_blocks_count(es)) {
		ext4_warning(sb, __func__, "blocks_count overflow");
		return -EINVAL;
	}

	if (le32_to_cpu(es->s_inodes_count) + EXT4_INODES_PER_GROUP(sb) <
	    le32_to_cpu(es->s_inodes_count)) {
		ext4_warning(sb, __func__, "inodes_count overflow");
		return -EINVAL;
	}

	if (reserved_gdb || gdb_off == 0) {
		if (!EXT4_HAS_COMPAT_FEATURE(sb,
					     EXT4_FEATURE_COMPAT_RESIZE_INODE)
		    || !le16_to_cpu(es->s_reserved_gdt_blocks)) {
			ext4_warning(sb, __func__,
				     "No reserved GDT blocks, can't resize");
			return -EPERM;
		}
		inode = ext4_iget(sb, EXT4_RESIZE_INO);
		if (IS_ERR(inode)) {
			ext4_warning(sb, __func__,
				     "Error opening resize inode");
			return PTR_ERR(inode);
		}
	}

	if ((err = verify_group_input(sb, input)))
		goto exit_put;

	if ((err = setup_new_group_blocks(sb, input)))
		goto exit_put;

	handle = ext4_journal_start_sb(sb,
				       ext4_bg_has_super(sb, input->group) ?
				       3 + reserved_gdb : 4);
	if (IS_ERR(handle)) {
		err = PTR_ERR(handle);
		goto exit_put;
	}

	mutex_lock(&sbi->s_resize_lock);
	if (input->group != sbi->s_groups_count) {
		ext4_warning(sb, __func__,
			     "multiple resizers run on filesystem!");
		err = -EBUSY;
		goto exit_journal;
	}

	if ((err = ext4_journal_get_write_access(handle, sbi->s_sbh)))
		goto exit_journal;

	if (gdb_off) {
		primary = sbi->s_group_desc[gdb_num];
		if ((err = ext4_journal_get_write_access(handle, primary)))
			goto exit_journal;

		if (reserved_gdb && ext4_bg_num_gdb(sb, input->group) &&
		    (err = reserve_backup_gdb(handle, inode, input)))
			goto exit_journal;
	} else if ((err = add_new_gdb(handle, inode, input, &primary)))
		goto exit_journal;

	gdp = (struct ext4_group_desc *)((char *)primary->b_data +
					 gdb_off * EXT4_DESC_SIZE(sb));

	memset(gdp, 0, EXT4_DESC_SIZE(sb));
	ext4_block_bitmap_set(sb, gdp, input->block_bitmap);  
	ext4_inode_bitmap_set(sb, gdp, input->inode_bitmap);  
	ext4_inode_table_set(sb, gdp, input->inode_table);  
	ext4_free_blks_set(sb, gdp, input->free_blocks_count);
	ext4_free_inodes_set(sb, gdp, EXT4_INODES_PER_GROUP(sb));
	gdp->bg_flags = cpu_to_le16(EXT4_BG_INODE_ZEROED);
	gdp->bg_checksum = ext4_group_desc_csum(sbi, input->group, gdp);

	err = ext4_mb_add_groupinfo(sb, input->group, gdp);
	if (err)
		goto exit_journal;

	ext4_blocks_count_set(es, ext4_blocks_count(es) +
		input->blocks_count);
	le32_add_cpu(&es->s_inodes_count, EXT4_INODES_PER_GROUP(sb));

	smp_wmb();

	sbi->s_groups_count++;

	ext4_handle_dirty_metadata(handle, NULL, primary);

#ifndef MY_ABC_HERE
	ext4_r_blocks_count_set(es, ext4_r_blocks_count(es) +
		input->reserved_blocks);
#endif

	percpu_counter_add(&sbi->s_freeblocks_counter,
			   input->free_blocks_count);
	percpu_counter_add(&sbi->s_freeinodes_counter,
			   EXT4_INODES_PER_GROUP(sb));

	if (EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_FLEX_BG) &&
	    sbi->s_log_groups_per_flex) {
		ext4_group_t flex_group;
		flex_group = ext4_flex_group(sbi, input->group);
		atomic_add(input->free_blocks_count,
			   &sbi->s_flex_groups[flex_group].free_blocks);
		atomic_add(EXT4_INODES_PER_GROUP(sb),
			   &sbi->s_flex_groups[flex_group].free_inodes);
	}

	ext4_handle_dirty_metadata(handle, NULL, sbi->s_sbh);
	sb->s_dirt = 1;

exit_journal:
	mutex_unlock(&sbi->s_resize_lock);
	if ((err2 = ext4_journal_stop(handle)) && !err)
		err = err2;
	if (!err) {
		update_backups(sb, sbi->s_sbh->b_blocknr, (char *)es,
			       sizeof(struct ext4_super_block));
		update_backups(sb, primary->b_blocknr, primary->b_data,
			       primary->b_size);
	}
exit_put:
	iput(inode);
	return err;
}  

int ext4_group_extend(struct super_block *sb, struct ext4_super_block *es,
		      ext4_fsblk_t n_blocks_count)
{
	ext4_fsblk_t o_blocks_count;
	ext4_group_t o_groups_count;
	ext4_grpblk_t last;
	ext4_grpblk_t add;
	struct buffer_head *bh;
	handle_t *handle;
	int err;
	ext4_group_t group;

	o_blocks_count = ext4_blocks_count(es);
	o_groups_count = EXT4_SB(sb)->s_groups_count;

	if (test_opt(sb, DEBUG))
		printk(KERN_DEBUG "EXT4-fs: extending last group from %llu uto %llu blocks\n",
		       o_blocks_count, n_blocks_count);

	if (n_blocks_count == 0 || n_blocks_count == o_blocks_count)
		return 0;

	if (n_blocks_count > (sector_t)(~0ULL) >> (sb->s_blocksize_bits - 9)) {
		printk(KERN_ERR "EXT4-fs: filesystem on %s:"
			" too large to resize to %llu blocks safely\n",
			sb->s_id, n_blocks_count);
		if (sizeof(sector_t) < 8)
			ext4_warning(sb, __func__, "CONFIG_LBDAF not enabled");
		return -EINVAL;
	}

	if (n_blocks_count < o_blocks_count) {
		ext4_warning(sb, __func__,
			     "can't shrink FS - resize aborted");
		return -EBUSY;
	}

	ext4_get_group_no_and_offset(sb, o_blocks_count, &group, &last);

	if (last == 0) {
		ext4_warning(sb, __func__,
			     "need to use ext2online to resize further");
		return -EPERM;
	}

	add = EXT4_BLOCKS_PER_GROUP(sb) - last;

	if (o_blocks_count + add < o_blocks_count) {
		ext4_warning(sb, __func__, "blocks_count overflow");
		return -EINVAL;
	}

	if (o_blocks_count + add > n_blocks_count)
		add = n_blocks_count - o_blocks_count;

	if (o_blocks_count + add < n_blocks_count)
		ext4_warning(sb, __func__,
			     "will only finish group (%llu"
			     " blocks, %u new)",
			     o_blocks_count + add, add);

	bh = sb_bread(sb, o_blocks_count + add - 1);
	if (!bh) {
		ext4_warning(sb, __func__,
			     "can't read last block, resize aborted");
		return -ENOSPC;
	}
	brelse(bh);

	handle = ext4_journal_start_sb(sb, 3);
	if (IS_ERR(handle)) {
		err = PTR_ERR(handle);
		ext4_warning(sb, __func__, "error %d on journal start", err);
		goto exit_put;
	}

	mutex_lock(&EXT4_SB(sb)->s_resize_lock);
	if (o_blocks_count != ext4_blocks_count(es)) {
		ext4_warning(sb, __func__,
			     "multiple resizers run on filesystem!");
		mutex_unlock(&EXT4_SB(sb)->s_resize_lock);
		ext4_journal_stop(handle);
		err = -EBUSY;
		goto exit_put;
	}

	if ((err = ext4_journal_get_write_access(handle,
						 EXT4_SB(sb)->s_sbh))) {
		ext4_warning(sb, __func__,
			     "error %d on journal write access", err);
		mutex_unlock(&EXT4_SB(sb)->s_resize_lock);
		ext4_journal_stop(handle);
		goto exit_put;
	}
	ext4_blocks_count_set(es, o_blocks_count + add);
	ext4_handle_dirty_metadata(handle, NULL, EXT4_SB(sb)->s_sbh);
	sb->s_dirt = 1;
	mutex_unlock(&EXT4_SB(sb)->s_resize_lock);
	ext4_debug("freeing blocks %llu through %llu\n", o_blocks_count,
		   o_blocks_count + add);
	 
	ext4_add_groupblocks(handle, sb, o_blocks_count, add);
	ext4_debug("freed blocks %llu through %llu\n", o_blocks_count,
		   o_blocks_count + add);
	if ((err = ext4_journal_stop(handle)))
		goto exit_put;

	if (test_opt(sb, DEBUG))
		printk(KERN_DEBUG "EXT4-fs: extended group to %llu blocks\n",
		       ext4_blocks_count(es));
	update_backups(sb, EXT4_SB(sb)->s_sbh->b_blocknr, (char *)es,
		       sizeof(struct ext4_super_block));
exit_put:
	return err;
}  
