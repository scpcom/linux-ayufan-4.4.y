/*
 * arch/arm/plat-oxnas/filter_misc.c
 *
 * Copyright (C) 2010 Oxford Semiconductor Ltd
 *
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
 
#include <linux/fs.h>
#include <linux/slab.h>
#include <mach/fast_open_filter.h>

#define INITIAL_NUM_MAP_ENTRIES	16
#define SECTOR_SIZE 			512

#ifndef CONFIG_OXNAS_FAST_OPEN_FILTER
 
void fast_close_filter(struct file* file)
{
	/* if space_reserved, need to do a space unreserve */
	struct dentry *dentry;
	struct inode	*inode;

	dentry = file->f_path.dentry;
	inode = dentry->d_inode;
	
	if(inode->space_reserve) {
		loff_t offset = 0;
		loff_t length = 0;
		inode->space_reserve = 0;
		
		offset = i_size_read(inode);
		
		/* retrieve prealloc file length */
		length = get_prealloc_length(inode);
				
		length -= offset;
		
		if ( (length > 0) && (offset >= 0) ) {
			file->f_op->unpreallocate(file, offset, length);
		}
	}
}

#endif //CONFIG_OXNAS_FAST_OPEN_FILTER

/* this function returns the total length of the file 
 * including the prealocated length
 * this retrieves a filemap of the file to do it
 */
loff_t get_prealloc_length(struct inode *inode) 
{
	loff_t retval = 0;
	int alloced_extents = INITIAL_NUM_MAP_ENTRIES;
	int used_extents = 0;
	int i = 0;
	getbmapx_t *map;
	map = kzalloc(sizeof(getbmapx_t) * (alloced_extents + 1), GFP_KERNEL);
	if (!map) {
		retval = -ENOMEM;
		goto out;
	}
	
	/* Query for the file map */
	map[0].bmv_length = -1;
	map[0].bmv_count = alloced_extents + 1;
	
	retval = inode->i_op->getbmapx(inode, map);
	if (retval < 0) {
		goto free_map;
	}
	
	used_extents = map[0].bmv_entries;
	if (used_extents > alloced_extents) {
		retval = -EINVAL;
		goto free_map;
	} else if (used_extents == alloced_extents) {
		/* More entries than we initially provided are required to discribe the
		   entire file */
		used_extents = inode->i_op->get_extents(inode, 0);

		if (used_extents < 0) {
			retval = -EINVAL;
			goto free_map;
		}

		if (used_extents > alloced_extents) {
			/* Guess was wrong so use the correct value */
			kfree(map);
			alloced_extents = used_extents;
			map = kzalloc(sizeof(getbmapx_t) * (alloced_extents + 1), GFP_KERNEL);
			if (!map) {
				retval = -ENOMEM;
				goto out;
			}

			map[0].bmv_length = -1;
			map[0].bmv_count = alloced_extents + 1;

//printk("alloc_filemap() getbmapx[2] - bmv_offset %lld, bmv_block 0x%llx, "
//	   "bmv_length %lld, bmv_count %ld, bmv_entries %ld, bmv_iflags 0x%lx, "
//	   "bmv_oflags 0x%lx\n", map_info->map[0].bmv_offset,
//	   map_info->map[0].bmv_block, map_info->map[0].bmv_length,
//	   map_info->map[0].bmv_count, map_info->map[0].bmv_entries,
//	   map_info->map[0].bmv_iflags, map_info->map[0].bmv_oflags);

			retval = inode->i_op->getbmapx(inode, map);
			if (retval < 0) {
				goto free_map;
			}

//printk("alloc_filemap() after getbmapx[2] - bmv_offset %lld, bmv_block 0x%llx, "
//	   "bmv_length %lld, bmv_count %ld, bmv_entries %ld, bmv_iflags 0x%lx, "
//	   "bmv_oflags 0x%lx\n", map_info->map[0].bmv_offset,
//	   map_info->map[0].bmv_block, map_info->map[0].bmv_length,
//	   map_info->map[0].bmv_count, map_info->map[0].bmv_entries,
//	   map_info->map[0].bmv_iflags, map_info->map[0].bmv_oflags);

			if (map[0].bmv_entries > alloced_extents) {
				retval = -EINVAL;
				goto free_map;
			}
//printk(KERN_INFO "alloc_filemap() Reallocated -> used_extents %d, alloced_extents %d\n",
//	map_info->used_extents, map_info->alloced_extents);
		}
	}

	/* Calculate the file length by totalling up all the extent sizes */
	retval = 0;
	for (i=1; i <= used_extents; i++) {
//printk("alloc_filemap() extent %d, bmv_offset %lld, bmv_length %lld, "
//	   "bmv_block 0x%llx, bmv_iflags 0x%lx, bmv_oflags 0x%lx\n", i,
//	   map_info->map[i].bmv_offset, map_info->map[i].bmv_length,
//	   map_info->map[i].bmv_block, map_info->map[i].bmv_iflags,
//	   map_info->map[i].bmv_oflags);

		retval += map[i].bmv_length;
	}
	retval *= SECTOR_SIZE;
	
free_map:
	kfree(map);
out:
	return retval;
}

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES

loff_t get_writer_prealloc_length(struct inode *inode)
{
	oxnas_filemap_info_t *map_info = &inode->writer_filemap_info;
	return  map_info->length;
}

#endif //CONFIG_OXNAS_FAST_READS_AND_WRITES
