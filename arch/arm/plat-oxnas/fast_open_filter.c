/*
 * arch/arm/plat-oxnas/fast_open_filter.c
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
#include <linux/list.h>
#include <linux/net.h>
#include <linux/spinlock.h>
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
#include <mach/fast_open_filter.h>
#include <mach/incoherent_sendfile.h>
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES
#ifdef CONFIG_OXNAS_FAST_WRITES
#include <mach/direct_writes.h>
#endif // CONFIG_OXNAS_FAST_WRITES

int fast_open_filter(
	struct file  *file,
	struct inode *inode)
{
	int ret = 0;
	int supports_fast_mode = 0;

//printk("fast_open_filter() File %p, inode %p\n", file, inode);
	BUG_ON(file->inode);

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	supports_fast_mode = file->f_op->incoherent_sendfile &&
		inode->i_op->get_extents && inode->i_op->getbmapx;
#ifdef CONFIG_OXNAS_FAST_WRITES
	supports_fast_mode = supports_fast_mode && inode->i_fop->resetpreallocate;
#endif // CONFIG_OXNAS_FAST_WRITES
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES

	if ((file->f_flags & O_FAST) && !supports_fast_mode) {
//printk("fast_open_filter() File %p, inode %p FAST mode is not supported\n", file, inode);
		/* The filesystem on which the file resides does not support FAST mode
		 * so force to NORMAL mode
		 */
		file->f_flags &= ~O_FAST;
#ifdef CONFIG_OXNAS_BACKUP
	} else {
#else // CONFIG_OXNAS_BACKUP
	} else if (supports_fast_mode) {
#endif // CONFIG_OXNAS_BACKUP
		spin_lock(&inode->fast_lock);

		/* If fallback from fast to normal mode is in progress, wait for it to
		 * complete */
		if (unlikely(inode->fallback_in_progress)) {
//printk("fast_open_filter() File %p, inode %p, waiting for fallback to complete\n", file, inode);
			wait_fallback_complete(inode);
		}

#ifdef CONFIG_OXNAS_BACKUP
		if (file->f_flags & O_BKP) {
			/* Must not allow backup open if already open in normal or fast mode
			 * and must only allow single open in backup mode */
			if ((inode->backup_open_count > 0) ||
				(inode->fast_open_count > 0) ||
				(inode->normal_open_count > 0)) {
//printk("fast_open_filter() File %p, inode %p cannot open in BACKUP mode (backup %d, normal %d, fast %d)\n", file, inode, inode->backup_open_count, inode->normal_open_count, inode->fast_open_count);
				ret = -1;
			} else {
				++inode->backup_open_count;
			}
		} else {
			/* Must not allow normal or fast mode open if already open in
			 * backup mode */
			if (inode->backup_open_count > 0) {
				ret = -1;
			} else {
#endif // CONFIG_OXNAS_BACKUP
				if (file->f_flags & O_FAST) {
//printk("fast_open_filter() File %p, inode %p FAST open request (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
					if (inode->normal_open_count > 0) {
//printk("fast_open_filter() File %p, inode %p already open for NORMAL read so force back to NORMAL (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
						file->f_flags &= ~O_FAST;
						++inode->normal_open_count;
					} else {
						/* Remember that we haven't yet allocated any context for this
						 * new fast reader
						 */
						file->fast_context = NULL;
#ifdef CONFIG_OXNAS_FAST_WRITES
						file->fast_write_context = NULL;
#endif // CONFIG_OXNAS_FAST_WRITES

						/* Record this fast open file with the inode */
						INIT_LIST_HEAD(&file->fast_head);
						write_lock(&inode->fast_files_lock);
						list_add_tail(&file->fast_head, &inode->fast_files);
						write_unlock(&inode->fast_files_lock);
						++inode->fast_open_count;
//printk("fast_open_filter() File %p, inode %p sucessfully opened for FAST read (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
					}
				} else {
//printk("fast_open_filter() File %p, inode %p NORMAL open request (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
					if (inode->fast_open_count > 0) {
						/* Force the FAST mode users to fallback to normal mode so that
						 * we can then allow the new normal open to proceed
						 */
//printk("fast_open_filter() File %p, inode %p already open for FAST read, so denying NORMAL open (normal %d, fast %d)", file, inode, inode->normal_open_count, inode->fast_open_count);
						fast_fallback(inode);
					}
		
					++inode->normal_open_count;
//printk("fast_open_filter() File %p, inode %p sucessfully opened for NORMAL read (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
				}
#ifdef CONFIG_OXNAS_BACKUP
			}
		}
#endif //  CONFIG_OXNAS_BACKUP

		spin_unlock(&inode->fast_lock);

		if (!ret) {
			/* Successfully opened a file on a filesystem capable of FAST mode */
			file->inode = inode;
		}
	}

	return ret;
}

void fast_close_filter(struct file* file)
{
//printk("fast_close_filter() File %p, inode %p, f_count = %ld\n", file, file->inode, atomic_long_read(&file->f_count));
	if (file->inode) {
		struct inode *inode = file->inode;
		int unprealloc = 0;
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		int free_filemap = 0;
		incoherent_sendfile_context_t *context = 0;
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES
#ifdef CONFIG_OXNAS_FAST_WRITES
		int reset_prealloc = 0;
#endif // CONFIG_OXNAS_FAST_WRITES

		spin_lock(&inode->fast_lock);

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		/* If fallback from fast to normal mode is in progress, wait for it to
		 * complete */
		if (unlikely(inode->fallback_in_progress)) {
//printk("fast_close_filter() File %p, inode %p, waiting for fallback to complete\n", file, inode);
			wait_fallback_complete(inode);
		}

		if (file->f_flags & O_FAST) {
//printk("fast_close_filter() File %p, inode %p FAST close request (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);

			if (!(--inode->fast_open_count)) {
				free_filemap = 1;
#ifdef CONFIG_OXNAS_FAST_WRITES
				reset_prealloc = 1;
#endif // CONFIG_OXNAS_FAST_WRITES
//printk("fast_close_filter() File %p, inode %p FAST open count now zero (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
				if(inode->space_reserve) {
					inode->space_reserve = 0;
					unprealloc = 1;
				}
				i_tent_size_write(inode, 0);
				inode->write_error = 0;
			}

			/* Was a fast read context reserved for this file? */
			if (file->fast_context) {
				/* Want context deallocated once inode fast lock dropped */
				context = file->fast_context;
			}

			/* Remove the reference from the inode to this fast file */
			write_lock(&inode->fast_files_lock);
			list_del(&file->fast_head);
			write_unlock(&inode->fast_files_lock);
		} else {
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES
#ifdef CONFIG_OXNAS_BACKUP
			if (file->f_flags & O_BKP) {
				//printk("fast_close_filter() File %p, inode %p BACKUP close request (backup %d, normal %d, fast %d)\n", file, inode, inode->backup_open_count, inode->normal_open_count, inode->fast_open_count);
				BUG_ON(--inode->backup_open_count < 0);
			} else {
#endif // CONFIG_OXNAS_BACKUP
//printk("fast_close_filter() File %p, inode %p NORMAL close request (normal %d, fast %d)\n", file, inode, inode->normal_open_count, inode->fast_open_count);
				BUG_ON(--inode->normal_open_count < 0);
				if(inode->normal_open_count == 0) {
					if(inode->space_reserve) {
						inode->space_reserve = 0;
						unprealloc = 1;
					}
				}
#ifdef CONFIG_OXNAS_BACKUP
			}
#endif // CONFIG_OXNAS_BACKUP
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		}
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES

		spin_unlock(&inode->fast_lock);

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		if (context) {
//printk("fast_close_filter() File %p, inode %p freeing context %p\n", file, inode, context);
			incoherent_sendfile_free_context(context);
		}

#ifdef CONFIG_OXNAS_FAST_WRITES
		if (file->fast_write_context) {
			complete_fast_write(file);
			if (reset_prealloc) {
				/* last file - reset prealloc */
				writer_reset_prealloc(file);
			}
		}
#endif // CONFIG_OXNAS_FAST_WRITES

		if(unprealloc) {
			loff_t offset = 0;
			loff_t length = 0;
			
			offset = i_size_read(inode);
#ifdef CONFIG_OXNAS_FAST_WRITES			
			if(free_filemap) { /* file map present and can be used */
				length = get_writer_prealloc_length(inode);
			} else
#endif // CONFIG_OXNAS_FAST_WRITES 
			{ /* filemap not present - have to be read */
				/* retrieve prealloc file length */
				length = get_prealloc_length(inode);
			}
			
			length -= offset;
			
			if ( (length > 0) && (offset >= 0) ) {
				file->f_op->unpreallocate(file, offset, length);
			}
		}

		if (free_filemap) {
//printk("fast_close_filter() File %p, inode %p freeing filemap\n", file, inode);
			incoherent_sendfile_check_and_free_filemap(inode);
#ifdef CONFIG_OXNAS_FAST_WRITES
			fast_write_check_and_free_filemap(inode);
#endif // CONFIG_OXNAS_FAST_WRITES
		}

		file->inode = NULL;
		file->fast_context = NULL;
#ifdef CONFIG_OXNAS_FAST_WRITES
		file->fast_write_context = NULL;
#endif // CONFIG_OXNAS_FAST_WRITES
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES
	}
}
