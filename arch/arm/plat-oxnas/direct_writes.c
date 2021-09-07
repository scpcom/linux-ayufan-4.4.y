/*
 * arch/arm/plat-oxnas/direct_writes.c
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

#include <net/tcp.h>
#include <mach/direct_writes.h>
#include <mach/oxnas_errors.h>
#include <mach/oxnas_direct_disk.h>
#include <mach/incoherent_sendfile.h>
#include <mach/sata_helper.h>

#define SATA_ACQUIRE_TIMEOUT_JIFFIES (30*HZ)

//#define NO_DISK_WRITE 		1
#undef NO_DISK_WRITE

#define OXNAS_WRITER_TIMEOUT 	CONFIG_OXNAS_WRITE_TIMEOUT

/* These have to be a factor of FS Block Size */
#define NET_SAMBA_RX_CHUNK_SIZE (CONFIG_OXNAS_WRITE_ACCUMULATION * 1024)

#define META_DATA_UPDATE_SIZE 		(CONFIG_OXNAS_WRITE_METADATA_UPDATE_SIZE * 1024 * 1024)
#define HOLE_PREALLOC_SIZE (CONFIG_OXNAS_WRITE_HOLE_PREALLOC_SIZE * 1024 * 1024)

static int oxnas_do_disk_flush( struct file *fp, loff_t offset, loff_t count);

enum {
	PARTIAL_MEM_FREE = 0,
	PARTIAL_MEM_TO_WRITE,
	PARTIAL_MEM_ON_GOING
};

/* this function returns the status of the current extent
 * Needs to be modified if the FS is changed
 */
int oxnas_get_extent_status(
	getbmapx_t *map,
	int         cur_map_entry)
{
	getbmapx_t     *map_entry = &map[cur_map_entry];

	if (map_entry->bmv_oflags & GETBMAPX_OF_PREALLOC) {
#ifdef DEBUG
		printk(KERN_INFO "extent status - PREALLOC - map entry - %d - flags - %ld\n", cur_map_entry, map_entry->bmv_oflags);
#endif
		return GETBMAPX_OF_PREALLOC;
	}
#ifdef DEBUG
	printk(KERN_INFO "extent status - NORMAL - map entry - %d - flags - %ld\n", cur_map_entry, map_entry->bmv_oflags);
#endif
	return 0;
}

static inline void oxnas_set_filemap_dirty(struct inode *inode)
{
	inode->writer_filemap_dirty = 1;
	smp_wmb();
}

static inline void oxnas_clear_filemap_dirty(struct inode *inode)
{
	inode->writer_filemap_dirty = 0;
	smp_wmb();
}

static inline loff_t oxnas_get_filesize(struct inode *inode)
{
	loff_t file_size;

	mutex_lock(&inode->i_mutex);
	file_size = i_size_read(inode);
	mutex_unlock(&inode->i_mutex);

	return file_size;
}

/* the below two functions are similar to i_size_read and i_size_write 
 * functions present in fs.h. Check these functions for more
 * explanation
 */
loff_t i_tent_size_read(const struct inode *inode)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	loff_t i_size;
	unsigned int seq;

	do {
		seq = read_seqcount_begin(&inode->i_size_seqcount);
		i_size = inode->i_tent_size;
	} while (read_seqcount_retry(&inode->i_size_seqcount, seq));
	return i_size;
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	loff_t i_size;

	preempt_disable();
	i_size = inode->i_tent_size;
	preempt_enable();
	return i_size;
#else
	return inode->i_tent_size;
#endif

	return 0;
}

void i_tent_size_write(struct inode *inode, loff_t i_size)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	write_seqcount_begin(&inode->i_size_seqcount);
	inode->i_tent_size = i_size;
	write_seqcount_end(&inode->i_size_seqcount);
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	preempt_disable();
	inode->i_tent_size = i_size;
	preempt_enable();
#else
	inode->i_tent_size = i_size;
#endif
}

static inline void update_list_index(oxnas_direct_disk_context_t *context)
{
	if (++context->list_idx == NUM_SATA_LISTS) {
	   context->list_idx = 0;
	}
}

/* function returns the number of sectors read successfully */
static int oxnas_read_sector_from_disk(
	oxnas_file_context_t        *file_context,
	oxnas_direct_disk_context_t *context,
	sector_t                     lba,
	int                          num_of_sectors,
	dma_addr_t 					buffer_pa)
{
#ifdef CONFIG_SATA_OX810
	int                     port = context->port;
#elif defined(CONFIG_SATA_OX820_DIRECT)
	direct_access_context_t *sata_context;
#endif // CONFIG_SATA_OX810

#ifdef CONFIG_ODRB_USE_PRDS
	prd_table_entry_t *prd = NULL;

	// Catch trying to read more data from disk than can fit in a single PRD
	BUG_ON(num_of_sectors > (PRD_MAX_LEN >> SECTOR_SHIFT));

	if (atomic_read(&context->cur_sg_status[context->list_idx]) == 0) {
		BUG_ON(odrb_alloc_prd_array(&context->prd_list[context->list_idx], 1, 1));
	}
#ifdef DEBUG	
	else {
		printk(KERN_INFO "read using same list thats currently under sata\n");
	}
#endif
	atomic_set(&context->cur_sg_status[context->list_idx], 1);

	prd = context->prd_list[context->list_idx]->prds;

	// Fill the PRD entry with the DMAable address and length
	prd->adr = buffer_pa;
	prd->flags_len = (num_of_sectors << SECTOR_SHIFT) | PRD_EOF_MASK;
#else // CONFIG_ODRB_USE_PRDS
	{
	odrb_sg_entry_t *sg = NULL;

	if (atomic_read(&context->cur_sg_status[context->list_idx]) == 0) {
		BUG_ON(odrb_alloc_sg_list(&context->sg_list[context->list_idx], 1, 1));
#ifdef DEBUG
		printk(KERN_INFO "alloc -1 context - %p, lists - %p , %p, fill idx -%d\n", 
						context, context->sg_list[0], context->sg_list[1], context->list_idx);
#endif						
	}
#ifdef DEBUG	 
	else {
		printk(KERN_INFO "read - using same list thats currently under sata\n");
	}
#endif
	atomic_set(&context->cur_sg_status[context->list_idx], 1);

	sg = context->sg_list[context->list_idx]->sg_entries;

	// Fill the SG entry with the DMAable address and length
	sg->addr_ = buffer_pa;
	sg->length_ = num_of_sectors << SECTOR_SHIFT;
	sg->next_ = 0;
	}
#endif // CONFIG_ODRB_USE_PRDS

	wait_sata_complete(context);

	/* Busy wait for ownership of the SATA core from the Linux SATA stack */
#ifdef CONFIG_SATA_OX810
	while (!acquire_sata_core_may_sleep(fast_writes_isr, (unsigned long)context,
		SATA_ACQUIRE_TIMEOUT_JIFFIES)) {
		printk(KERN_WARNING "oxnas_read_sector_from_disk() Sata acquire timeout\n");
	}
#elif defined(CONFIG_SATA_OX820_DIRECT)
	sata_context = context->inode->writer_filemap_info.direct_access_context;

	while ((*sata_context->acquire)(sata_context, SATA_ACQUIRE_TIMEOUT_JIFFIES, context, 0)) {
		printk(KERN_WARNING "oxnas_read_sector_from_disk() Sata acquire timeout\n");
	}
#endif // CONFIG_SATA_XXX

	if (unlikely((file_context->partial_block) &&
		(file_context->partial_block->status == PARTIAL_MEM_ON_GOING))) {
		dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
		kfree (file_context->partial_block);
		file_context->partial_block = NULL;
	}

	/* Setup SG DMA transfer */
#ifdef CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
	odrb_dma_sata_prd(OXNAS_DMA_FROM_DEVICE, num_of_sectors,
		context->prd_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
	(*sata_context->prepare_command)(sata_context, 0, lba,
		num_of_sectors, context->prd_list[context->list_idx]->phys,
		fast_writes_isr, context);
#endif // CONFIG_SATA_XXX
#else // CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
	odrb_dma_sata_sq(OXNAS_DMA_FROM_DEVICE, num_of_sectors,
		context->sg_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
	(*sata_context->prepare_command)(sata_context, 0, lba,
		num_of_sectors, context->sg_list[context->list_idx]->phys,
		fast_writes_isr, context);
#endif // CONFIG_SATA_XXX
#endif // CONFIG_ODRB_USE_PRDS

	atomic_set(&context->cur_transfer_idx, context->list_idx);
	atomic_set(&context->free_sg, 1);
	atomic_set(&context->cur_sg_status[context->list_idx], 1);

	update_list_index(context);

	set_need_to_wait(context);

	/* Issue SATA command to write contiguous sectors to disk */
#ifdef CONFIG_SATA_OX810
	direct_sata_transfer(C_READ_DMA_EXT, port, lba, num_of_sectors);
#elif defined(CONFIG_SATA_OX820_DIRECT)
	(*sata_context->execute_command)();
#endif // CONFIG_SATA_XXX

	// Wait for ISR to signal SATA transfer finished */
	wait_sata_complete(context);

	return num_of_sectors;
}

static void oxnas_direct_disk_complete_partial_write(
	oxnas_file_context_t        *file_context,
	oxnas_direct_disk_context_t *context)
{
#ifdef CONFIG_SATA_OX810
	int                     port = context->port;
#elif defined(CONFIG_SATA_OX820_DIRECT)
	direct_access_context_t *sata_context;
#endif // CONFIG_SATA_OX810
	unsigned char *			buf_ptr = NULL;
#ifdef CONFIG_ODRB_USE_PRDS
	prd_table_entry_t *prd = NULL;
#else // CONFIG_ODRB_USE_PRDS
	odrb_sg_entry_t *prev_sg = NULL;
	odrb_sg_entry_t *sg = NULL;
#endif // CONFIG_ODRB_USE_PRDS

	if (file_context->partial_block) {
		if(!context->buffer) {
			context->buffer = dma_alloc_coherent(0, sizeof(char) << context->fs_blocklog, &context->buffer_pa, GFP_KERNEL);
			BUG_ON(!context->buffer);
		}
		/* read */
		if (file_context->partial_block->unwritten == GETBMAPX_OF_PREALLOC) {
			if (file_context->prev_partial_write_loc == file_context->partial_block->lba) {
				goto read_from_disk;
			}
			// Wait for our last SATA transfer to complete
			wait_sata_complete(context);
			memset(context->buffer, 0, sizeof(char) << context->fs_blocklog);
		} else {
read_from_disk:
			BUG_ON(!oxnas_read_sector_from_disk(file_context, context,
				file_context->partial_block->lba,
				(1 << (context->fs_blocklog - SECTOR_SHIFT)), context->buffer_pa));
		}

#ifdef CONFIG_ODRB_USE_PRDS
		if (atomic_read(&context->cur_sg_status[context->list_idx]) == 0) {
			BUG_ON(odrb_alloc_prd_array(&context->prd_list[context->list_idx], 1, 1));
		}
		atomic_set(&context->cur_sg_status[context->list_idx], 1);

		prd = context->prd_list[context->list_idx]->prds;

		if (file_context->partial_block->bytes_into_block) {
			prd->adr = context->buffer_pa;
			prd++->flags_len = file_context->partial_block->bytes_into_block;
		}

		prd->adr = file_context->partial_block->buffer_pa;
		prd->flags_len = file_context->partial_block->length;

		if ((1 << context->fs_blocklog) > (file_context->partial_block->length + file_context->partial_block->bytes_into_block)) {
			buf_ptr = (unsigned char *)context->buffer_pa;
			buf_ptr += file_context->partial_block->length + file_context->partial_block->bytes_into_block;
			++prd;
			prd->adr = (dma_addr_t)buf_ptr;
			prd->flags_len = (1 << context->fs_blocklog) - (file_context->partial_block->length + file_context->partial_block->bytes_into_block);
		}
		prd->flags_len |= PRD_EOF_MASK;
#else // CONFIG_ODRB_USE_PRDS

		if (atomic_read(&context->cur_sg_status[context->list_idx]) == 0) {		
			BUG_ON(odrb_alloc_sg_list(&context->sg_list[context->list_idx], 1, 1));
		}
		atomic_set(&context->cur_sg_status[context->list_idx], 1);

		sg = context->sg_list[context->list_idx]->sg_entries;

		if (file_context->partial_block->bytes_into_block) {
			sg->addr_ = context->buffer_pa;
			sg->length_ = file_context->partial_block->bytes_into_block;

			prev_sg = sg;
			prev_sg->next_ = (dma_addr_t)descriptors_virt_to_phys(((u32)++sg));
		}

		sg->addr_ = file_context->partial_block->buffer_pa;
		sg->length_ = file_context->partial_block->length;

		if ((1 << context->fs_blocklog) > (file_context->partial_block->length + file_context->partial_block->bytes_into_block)) {
			prev_sg = sg;
			prev_sg->next_ = (dma_addr_t)descriptors_virt_to_phys(((u32)++sg));

			buf_ptr = (unsigned char *)context->buffer_pa;
			buf_ptr += file_context->partial_block->length + file_context->partial_block->bytes_into_block;
			sg->addr_ = (dma_addr_t)buf_ptr;

			sg->length_ = (1 << context->fs_blocklog) - (file_context->partial_block->length + file_context->partial_block->bytes_into_block);
		}
		sg->next_ = 0;
#endif // CONFIG_ODRB_USE_PRDS

		/* write */
		/* Busy wait for ownership of the SATA core from the Linux SATA stack */
#ifdef CONFIG_SATA_OX810
		while (!acquire_sata_core_may_sleep(fast_writes_isr, (unsigned long)context,
			SATA_ACQUIRE_TIMEOUT_JIFFIES)) {
			printk(KERN_WARNING "oxnas_direct_disk_complete_partial_write() Sata acquire timeout \n");
		}
#elif defined(CONFIG_SATA_OX820_DIRECT)
		sata_context = context->inode->writer_filemap_info.direct_access_context;

		while ((*sata_context->acquire)(sata_context, SATA_ACQUIRE_TIMEOUT_JIFFIES, context, 0)) {
			printk(KERN_WARNING "oxnas_direct_disk_complete_partial_write() Sata acquire timeout\n");
		}
#endif // CONFIG_SATA_XXX

		/* Setup SG DMA transfer */
#ifdef CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
		odrb_dma_sata_prd(OXNAS_DMA_TO_DEVICE,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)),
			context->prd_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
		(*sata_context->prepare_command)(sata_context, 1,
			file_context->partial_block->lba,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)),
			context->prd_list[context->list_idx]->phys, fast_writes_isr, context);
#endif // CONFIG_SATA_XXX
#else // CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
		odrb_dma_sata_sq(OXNAS_DMA_TO_DEVICE,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)),
			context->sg_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
		(*sata_context->prepare_command)(sata_context, 1,
			file_context->partial_block->lba,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)),
			context->sg_list[context->list_idx]->phys, fast_writes_isr, context);
#endif // CONFIG_SATA_XXX
#endif // CONFIG_ODRB_USE_PRDS

		atomic_set(&context->cur_transfer_idx, context->list_idx);
		atomic_set(&context->free_sg, 1);
		atomic_set(&context->cur_sg_status[context->list_idx], 1);

		update_list_index(context);

		set_need_to_wait(context);

		/* Issue SATA command to write contiguous sectors to disk */
#ifdef CONFIG_SATA_OX810
		direct_sata_transfer(C_WRITE_DMA_EXT, port,
			file_context->partial_block->lba,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)));
#elif defined(CONFIG_SATA_OX820_DIRECT)
		(*sata_context->execute_command)();
#endif // CONFIG_SATA_XXX

		/* since the transfer is queued, can set the filemap dirty flag
		 */
		if (file_context->partial_block->unwritten == GETBMAPX_OF_PREALLOC) {
			context->prealloc_write = 1; 
		}
		file_context->prev_partial_write_loc = file_context->partial_block->lba;

		// Wait for ISR to signal SATA transfer finished
		wait_sata_complete(context);

		/* have to reset the preallocate flag if present */
		oxnas_reset_extent_preallocate_flag(file_context->partial_block->fp,
				file_context->partial_block->start_offset_into_file - file_context->partial_block->bytes_into_block,
				1 << context->fs_blocklog,
				file_context->partial_block->unwritten,
				file_context->disable_accumulation);

		/* have to update the file size if required */
		oxnas_set_filesize(file_context->partial_block->fp,
			(file_context->partial_block->start_offset_into_file
				+ file_context->partial_block->length));

#ifdef DEBUG			
		printk(KERN_INFO "partial write complete Inode %p, file %p, name %s: \n",
							 			file_context->partial_block->fp->inode, 
							 			file_context->partial_block->fp, 
							 			file_context->partial_block->fp->f_path.dentry->d_name.name);
#endif

		dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
		kfree(file_context->partial_block);
		file_context->partial_block = NULL;
	}
}

/* This function gets called with the locks held 
 * It flushes any pending accumulated stuff to disk
 */
static int complete_accumulated_write(oxnas_file_context_t * file_context)
{
	struct file 				*acc_fp 		= file_context->acc_fp;
	oxnas_direct_disk_context_t *disk_context 	= (oxnas_direct_disk_context_t *) acc_fp->fast_write_context;
	oxnas_net_rx_context_t      *net_rx_context = &disk_context->net_rx_context;
	int 						write_result 	= 0;
	
	if (net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length) {
		loff_t temp_offset = 0;
		loff_t temp_count = 0;
			 
#ifdef NO_DISK_WRITE
		release_netdma_net_frags_by_index(net_rx_context, net_rx_context->fill_frag_list_idx);
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length = 0;
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset = 0;
		update_context_indices(net_rx_context);
		write_result = 0;
		
#else //NO_DISK_WRITE
		temp_count = net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length;
		temp_offset = net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset;
		
#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX
		printk (KERN_INFO "\n Acc write - Filename %s, inode - %p[%lu], file - %p offset - %lld count - %lld - idx - %d\n",
                            acc_fp->f_path.dentry->d_name.name, acc_fp->inode, acc_fp->inode->i_ino, acc_fp, temp_offset, temp_count,
                            net_rx_context->fill_frag_list_idx);
#else
		 printk (KERN_INFO "\n Acc write - Filename %s, inode - %p, file - %p offset - %lld count - %lld - idx - %d\n",
                           acc_fp->f_path.dentry->d_name.name, acc_fp->inode, acc_fp, temp_offset, temp_count,
							net_rx_context->fill_frag_list_idx);
#endif
#endif
		
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length = 0;
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset = 0;
				
		/* call the function to do the dirty work */
		write_result = oxnas_do_disk_flush( acc_fp, temp_offset, temp_count);
								 
#endif //NO_DISK_WRITE
	}
	
	file_context->acc_fp = 0;
	smp_wmb();
	
	return write_result;
}

static void update_context_indices(oxnas_net_rx_context_t *context)
{
	// Update context fill and release indices after completing a SATA transfer
	if (++context->release_frag_list_idx > 1) {
	   context->release_frag_list_idx = 0;
	}
	if (++context->fill_frag_list_idx > 1) {
	   context->fill_frag_list_idx = 0;
	}
}

static void oxnas_direct_disk_complete(oxnas_direct_disk_context_t *context)
{
	// Wait for our last SATA transfer to complete
	wait_sata_complete(context);

	// Update fill and release indices
	update_context_indices(&context->net_rx_context);
}

static void writer_work_handler(struct work_struct	*work)
{
	oxnas_file_context_t *file_context =
		container_of(work, struct oxnas_file_context, write_completion_work.work);
	struct file  *fp = file_context->fp;
	struct file  *acc_fp = file_context->acc_fp;
	struct inode *inode = NULL;
	struct file *locker_fp;
	
	smp_rmb();

	if ( (fp == NULL) && (acc_fp == NULL) ) {
		return;
	}

	if(fp)
		inode = fp->inode;
	else if(acc_fp)
		inode = acc_fp->inode;
	else {
		printk(KERN_INFO "work_handler - no valid file pointer - returning \n");
		return;
	}		

	locker_fp = fp ?: acc_fp;

	spin_lock(&inode->fast_lock);
	if(unlikely((inode->fallback_in_progress) ||
		        (inode->fast_writes_in_progress_count))) {
		spin_unlock(&inode->fast_lock);
		return;
	} else {
		// Increment in-progress write count so that any fast fallback
		// shall wait for us to finish
		++inode->fast_writes_in_progress_count;	
	}
	spin_unlock(&inode->fast_lock);

	/* grab the exclusive write access */
	if (down_trylock(&inode->writer_filemap_info.sem)) {
		/* some one else is using it - they should take care of things */
		goto spin_lock_out;
	}

	if(acc_fp) {
		/* complete any pending writes */
		if( complete_accumulated_write(file_context) < 0) {
			/* the error here will be taken care by next actual write rather
			 * than from the thread
			 */
			printk(KERN_INFO "FAST WRITE COMPLETE - accumulated write of some other file pointer failed \n");
		}
		file_context->acc_fp = 0;
	}

	if (fp) {
		oxnas_file_context_t *file_context =
			(oxnas_file_context_t *)inode->writer_file_context;
		oxnas_direct_disk_context_t *disk_context =
			(oxnas_direct_disk_context_t *)fp->fast_write_context;
										
		cancel_delayed_work(&file_context->write_completion_work);
			
		/* Wait for the last SATA transfer to finish before freeing the network
	   		fragment storage */
		oxnas_direct_disk_complete(disk_context);

		/* set file size and reset prealloc flag */
		if (file_context->write_end_offset) {
			long long temp_end =
				(file_context->write_end_offset >> disk_context->fs_blocklog)
					<< disk_context->fs_blocklog;
#ifdef DEBUG
			printk (KERN_INFO "writer_timeout - file - %p, start offset - %lld, end offset - %lld\n", fp, file_context->write_start_offset, file_context->write_end_offset);
#endif

			if (temp_end > file_context->write_start_offset) {
				oxnas_reset_extent_preallocate_flag(fp,
					file_context->write_start_offset,
					temp_end - file_context->write_start_offset,
					file_context->write_extent_flag,
					file_context->disable_accumulation);
													
				file_context->write_start_offset = temp_end;
			}

			oxnas_set_filesize(fp, file_context->write_end_offset);
		}

		/* free up any unused frag cache */
		release_netdma_net_frags(&disk_context->net_rx_context);
		release_netdma_net_frags_by_index(&disk_context->net_rx_context,
			disk_context->net_rx_context.fill_frag_list_idx);
	}

	/* release the exclusive write access */
	up(&inode->writer_filemap_info.sem);

spin_lock_out:	
	spin_lock(&inode->fast_lock);
	--inode->fast_writes_in_progress_count;
	if (unlikely(inode->fallback_in_progress) &&
		!inode->fast_writes_in_progress_count && 
		!inode->fast_reads_in_progress_count) {
		/* last in line - initiate fall back
		 */
printk(KERN_INFO "fast write - Initiating fallback from timeout\n");
		do_fast_fallback(inode);
	}
	spin_unlock(&inode->fast_lock);
}

static int init_write_inode_context(
	struct inode * inode)
{
	int result = 0;
	oxnas_file_context_t * file_context;
	if (!inode->writer_file_context) {
		file_context = kzalloc(sizeof(oxnas_file_context_t), GFP_KERNEL);
		if (unlikely(!file_context)) {
			result = -ENOMEM;
			printk(KERN_INFO "Alloc memory for map_info failed\n");
			goto out;
		}
		memset(file_context, 0, sizeof(oxnas_file_context_t));

		INIT_DELAYED_WORK(&file_context->write_completion_work, writer_work_handler);

		inode->writer_file_context = file_context;
		smp_wmb();
	}
out:
	return result;
}

/* Called without inode.fast lock */
static int do_initial_inode_prep(
	struct inode *inode,
	struct file  *file)
{
	int retval = 0;

	// First fast write on inode so in case there is data hanging around
	// in the pagecache from previous normal writes we need to flush and
	// invalidate the pagecache for this file
	retval = vfs_fsync(file, file->f_dentry, 0);
	if (unlikely(retval)) {
		printk(KERN_WARNING "fast write - fsync() Inode %p, file %p fsync failed, error %d\n", inode, file, retval);
	} else {
		/* Find partition/disk info */
		retval = init_filemapinfo(inode, 1);
		if (unlikely(retval)) {
			printk(KERN_WARNING "fast write -init_filemapinfo() Inode %p, file %p Failed to initialise filemapinfo, error %d\n", inode, file, retval);
		} else {
			retval = init_write_inode_context(inode);
			if (unlikely(retval)) {
				printk(KERN_WARNING "fast write - init_write_file_context() Inode %p, file %p Failed to allocate filemap, error %d\n", inode, file, retval);
			} 
		}
	}
	return retval;
}

static void writer_complete_fallback(struct inode * inode)
{
	spin_lock(&inode->fast_lock);
	if (!inode->fast_writes_in_progress_count && 
		!inode->fast_reads_in_progress_count) {
		do_fast_fallback(inode);
	} else { /* wait for a fallback to compete */
		wait_fallback_complete(inode);
	}
	spin_unlock(&inode->fast_lock);
}

static int init_write_file_context(
	struct inode *inode,
	struct file  *fp)
{
	int                          result = 0;
	int                          i = 0;
	oxnas_direct_disk_context_t *disk_context;
	oxnas_net_rx_context_t      *net_context;
	struct kmem_cache           *frag_cache;

#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX
	printk(KERN_INFO "Fast Write - Init inode:%p[%lu]- File - %p - write context - %p\n", inode, inode->i_ino, fp, fp->fast_write_context);
#else
	printk(KERN_INFO "Fast Write - Init - File - %p - write context - %p\n", fp, fp->fast_write_context);
#endif
#endif

	/* Output file context initialisation */
	if (unlikely(fp->fast_write_context) ) {
		printk(KERN_INFO "Setting file context more than once for the same file- ERROR\n");
		result = -EINVAL;
		goto out;
	}

	/* create the context structure and assign to variable */
	disk_context = kzalloc(sizeof(oxnas_direct_disk_context_t), GFP_KERNEL);
	if (unlikely(!disk_context)) {
		result = -ENOMEM;
		printk(KERN_INFO "Alloc memory for map_info failed\n");
		goto out;
	}

	memset(disk_context, 0, sizeof(oxnas_direct_disk_context_t));

	atomic_set(&disk_context->free_sg, 0);
	atomic_set(&disk_context->cur_transfer_idx, -1);

	/* Semaphore to ensure only a single SATA access can be active for any
	   writers on this disk context and hence the associated inode */
	sema_init(&disk_context->sata_active_sem, 0);

	disk_context->fs_blocklog = oxnas_get_fs_blocksize(fp);
	disk_context->list_idx = 0;

	disk_context->buffer = NULL;

	/* initialize net fragment related stuff */
	net_context = &disk_context->net_rx_context;

	/*
	 * -2 to account for possible first and last partial FSB's
	 * -1 to account for 0 offset
	 */
#ifdef CONFIG_ODRB_USE_PRDS
	net_context->max_frag_cnt = CONFIG_ODRB_WRITER_PRD_ARRAY_SIZE - 3; 
#else // CONFIG_ODRB_USE_PRDS
	net_context->max_frag_cnt = CONFIG_ODRB_NUM_WRITER_SG_ENTRIES - 3; 
#endif // CONFIG_ODRB_USE_PRDS
																	  
	/* Initialise the zero-copy context and sg index variables*/
	for (i=0; i < NUM_NET_RX_FRAG_LISTS; i++) {
		INIT_LIST_HEAD(&net_context->page_info[i]);
		net_context->frag_cnt[i] = 0;
		net_context->data_ref[i].length = 0;
	}
	
	for (i=0; i < NUM_SATA_LISTS; i++) {
		atomic_set(&disk_context->cur_sg_status[i], 0);
	}

	/* Construct unique name for the fragment cache for this file handle */
	snprintf(disk_context->frag_cache_name, 24 * sizeof(char), "FRG%p%p", fp, fp);

	frag_cache = kmem_cache_create(disk_context->frag_cache_name,
		sizeof(frag_list_entry_t), 0, 0, frag_list_entry_ctor);

	if (unlikely(!frag_cache)) {
		result = -ENOMEM;
		printk(KERN_ERR "Frag Cache creation failed \n");
		goto disk_context_out;
	}

	net_context->frag_cache = frag_cache;

	for (i=0; i < NUM_SATA_LISTS; i++) {
#ifdef CONFIG_ODRB_USE_PRDS
		disk_context->prd_list[i] = NULL;
#else // CONFIG_ODRB_USE_PRDS
		disk_context->sg_list[i] = NULL;
#endif // CONFIG_ODRB_USE_PRDS
	}

	/* Don't want any fragments releasing until 1st SATA write has completed */
	net_context->release_frag_list_idx = -2;

	disk_context->inode = inode;
	fp->fast_write_context = disk_context;
	
	smp_wmb();

out:
	return result;

disk_context_out:
	if (disk_context)
		kfree(disk_context);
	goto out;

	/* Control never reaches here */
	return 0;
}

/* Return 0 = not dirty 1 - dirty */ 
static int oxnas_check_filemap_dirty(struct inode *inode)
{
	smp_rmb();
	return inode->writer_filemap_dirty;
}

static int oxnas_direct_disk_write(
	oxnas_file_context_t        *file_context,
	oxnas_direct_disk_context_t *context,
	oxnas_filemap_offset_t      *filemap_offset,
	ssize_t                      length,
	loff_t                       start_offset,
	int                          bytes_into_block,
	struct file                 *fp)
{
	oxnas_net_rx_context_t *net_rx_context = &context->net_rx_context;
#ifdef CONFIG_SATA_OX810
	int                     port = context->port;
	sector_t                part_offset = fp->inode->writer_filemap_info.part_offset;
#endif // CONFIG_SATA_OX810
	frag_list_entry_t      *frag;
	unsigned int            frag_offset = 0;
	ssize_t                 remaining = length;
	oxnas_partial_block_t *new_partial = NULL;

#ifdef DEBUG
	printk(KERN_INFO "direct disk write 2 called inode - %p, file - %p, start offset - %lld, length -%u, bytesinto block - %d\n", fp->inode, fp, start_offset, length, bytes_into_block);
#endif

	/* Get the first network Rx data fragment */
	frag = (frag_list_entry_t*)container_of(
		net_rx_context->page_info[net_rx_context->fill_frag_list_idx].next,
		frag_list_entry_t, head);

	do {
		getbmapx_t     *map_entry;
		long long       map_start;
		long long       map_len;
		long long       map_offset;
		sector_t        lba;
		long long       contig_len;
		unsigned int    remaining_contig_len;
		unsigned long   total_len = 0;
		int             partial_write_wait = 0;
		int             prefilled_bytes = 0;
		int             wait_sata_inactive = 0;
#ifdef CONFIG_ODRB_USE_PRDS
		prd_table_entry_t *prd = NULL;
#else // CONFIG_ODRB_USE_PRDS
		odrb_sg_entry_t *sg = NULL;
#endif // CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX820_DIRECT
		direct_access_context_t *sata_context;
#endif // CONFIG_SATA_OX810

		map_entry = &context->map[filemap_offset->cur_map_entry];
		map_offset = filemap_offset->cur_map_entry_offset;

		map_start  = map_entry->bmv_offset;
		map_len    = map_entry->bmv_length;

		/* Find the next block to write */
		lba = map_entry->bmv_block + map_offset;
#ifdef CONFIG_SATA_OX810
		lba += part_offset;
#endif // CONFIG_SATA_OX810

		BUG_ON (file_context->partial_block);

		/* only writes of greater than a sector need to be prefilled.
		 * writes of a lesser size will get deferred and hence
		 * no need to prefill
		 */
		if (bytes_into_block &&
			((bytes_into_block + remaining) >= (1 << context->fs_blocklog))) {
			/* partial start sector */
			/* have to do a read modify and write read */
			if(!context->buffer) {
				context->buffer = dma_alloc_coherent(0, sizeof(char) << context->fs_blocklog, &context->buffer_pa, GFP_KERNEL);
				BUG_ON(!context->buffer);
			}

			if (oxnas_get_extent_status(context->map, filemap_offset->cur_map_entry) == GETBMAPX_OF_PREALLOC) {
				if (file_context->prev_partial_write_loc == lba) {
					/* same lba again - have to read from disk */
					goto read_from_disk;
				} else {
					/* setting this flag will mark the filemap to be dirty on exit */
					context->prealloc_write = 1;
				}
				// Wait for our last SATA transfer to complete
				wait_sata_complete(context);

				if (unlikely((file_context->partial_block) &&
					(file_context->partial_block->status == PARTIAL_MEM_ON_GOING))) {
					dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
					kfree (file_context->partial_block);
					file_context->partial_block = NULL;
				}
				memset(context->buffer, 0, sizeof(char) * (1 << context->fs_blocklog));
			} else {
read_from_disk:
				BUG_ON(!oxnas_read_sector_from_disk(file_context, context, lba,
					(1 << (context->fs_blocklog - SECTOR_SHIFT)), context->buffer_pa));
			}
			
#ifdef CONFIG_ODRB_USE_PRDS
			if (atomic_read(&context->cur_sg_status[context->list_idx]) == 0) {
				BUG_ON(odrb_alloc_prd_array(&context->prd_list[context->list_idx], 1, 1));
			}
			
			atomic_set(&context->cur_sg_status[context->list_idx], 1);
			
			prd = context->prd_list[context->list_idx]->prds;

			prd->adr = context->buffer_pa;
			prd->flags_len = bytes_into_block;
#else // CONFIG_ODRB_USE_PRDS
			if (atomic_read(&context->cur_sg_status[context->list_idx]) == 0) {
				BUG_ON(odrb_alloc_sg_list(&context->sg_list[context->list_idx], 1, 1));
#ifdef DEBUG		
				printk(KERN_INFO "alloc -6 context - %p, lists - %p , %p, fill idx -%d\n", 
						context, context->sg_list[0], context->sg_list[1], context->list_idx);
#endif						
			}
			
			atomic_set(&context->cur_sg_status[context->list_idx], 1);
			
			sg = context->sg_list[context->list_idx]->sg_entries;

			sg->addr_ = context->buffer_pa;
			sg->length_ = bytes_into_block;
#endif // CONFIG_ODRB_USE_PRDS

			/* write will be done as part of the normal write */
			/* just have to set the right size  here */
			prefilled_bytes = bytes_into_block;
			bytes_into_block = 0;
		}

		/* Calculate remaining contiguous disk bytes at offset into mapping */
		contig_len = (map_len - map_offset) << SECTOR_SHIFT;
		
		BUG_ON(!contig_len);

		if (contig_len > remaining + prefilled_bytes) {
			contig_len = remaining + prefilled_bytes;
			filemap_offset->cur_map_entry_offset += (contig_len >> SECTOR_SHIFT);
		} else {
			++filemap_offset->cur_map_entry;
			filemap_offset->cur_map_entry_offset = 0;
		}

		/* calculate start offset */
		/* added to update file size in case of partial write */
/*		start_offset = start_offset + (contig_len - prefilled_bytes) - (contig_len - ((contig_len >> context->fs_blocklog) << context->fs_blocklog));
		start_offset = start_offset + contig_len - prefilled_bytes - contig_len + ((contig_len >> context->fs_blocklog) << context->fs_blocklog));
 */
		start_offset = start_offset - prefilled_bytes + ((contig_len >> context->fs_blocklog) << context->fs_blocklog);
		
		/* check for last sector to be partial */
		if (contig_len - ((contig_len >> context->fs_blocklog) << context->fs_blocklog)) {
			new_partial = kmalloc(sizeof(oxnas_partial_block_t), GFP_KERNEL);
			BUG_ON (!new_partial);
			new_partial->buffer = dma_alloc_coherent(0, (sizeof(char) << context->fs_blocklog), &new_partial->buffer_pa, GFP_KERNEL);
			BUG_ON (!new_partial->buffer);
			new_partial->length = contig_len -
				((contig_len >> context->fs_blocklog) << context->fs_blocklog);
			new_partial->bytes_into_block = bytes_into_block;
			new_partial->status = PARTIAL_MEM_TO_WRITE;
			new_partial->fp = fp;
			new_partial->start_offset_into_file = start_offset;
			new_partial->unwritten =
				oxnas_get_extent_status(context->map, filemap_offset->cur_map_entry);

			/* temp soln - copy the partial sector buffer from fragments to buffer */
			/* perm soln - to do later - add additional reference count to the respective pages */
			/* copy will be done down the function */

			contig_len -= new_partial->length; /* reduce the length to write now */
			remaining -= new_partial->length; /* reduce the length to write now */

			/* lba has to be calculated only after knowing the right length thats written now */			
			new_partial->lba = lba + (contig_len >> SECTOR_SHIFT);
#ifdef DEBUG
			printk(KERN_INFO "creating new partial to handle later lba - %lld, length - %d, bytes_into_block - %d, offset into file - %lld\n",
								new_partial->lba, new_partial->length, new_partial->bytes_into_block, new_partial->start_offset_into_file);
#endif
		}

		/* Fill SG DMA descriptors with enough network Rx data to fill the
		   contiguous disk sectors */
		remaining_contig_len = contig_len - prefilled_bytes;
		
		while (remaining_contig_len) {
			unsigned int frag_remaining_len;
			unsigned int len;
			
//			printk(KERN_INFO "frag_len - %d, frag_offset - %d, reamining contig - %d\n",
//										frag->bio_vec.bv_len,
//										frag_offset,
//										remaining_contig_len);					 

			frag_remaining_len = frag->bio_vec.bv_len - frag_offset;
			len = (frag_remaining_len > remaining_contig_len) ?
					remaining_contig_len : frag_remaining_len;
					
			BUG_ON(!len);

#ifdef CONFIG_ODRB_USE_PRDS
			if ((atomic_read(&context->cur_sg_status[context->list_idx]) == 0) || !prd){		
				BUG_ON(odrb_alloc_prd_array(&context->prd_list[context->list_idx], 1, 1));
				atomic_set(&context->cur_sg_status[context->list_idx], 1);

				prd = context->prd_list[context->list_idx]->prds;
			} else {
				++prd;
			}

			// Fill the PRD entry with the DMAable address and length
			prd->adr = virt_to_dma(0, page_address(frag->bio_vec.bv_page) +
				frag->bio_vec.bv_offset + frag_offset);
			prd->flags_len = len;
#else // CONFIG_ODRB_USE_PRDS
			if ((atomic_read(&context->cur_sg_status[context->list_idx]) == 0) || !sg) {
				BUG_ON(odrb_alloc_sg_list(&context->sg_list[context->list_idx], 1, 1));
				atomic_set(&context->cur_sg_status[context->list_idx], 1);

				sg = context->sg_list[context->list_idx]->sg_entries;
			} else {
				// Acquire the next SG element and link the just completed SG
				// element's next_ field to it's physical address
				odrb_sg_entry_t *prev_sg = sg;
				prev_sg->next_ = (dma_addr_t)descriptors_virt_to_phys(((u32)++sg));
			}

			// Fill the SG entry with the DMAable address and length
			sg->addr_ = virt_to_dma(0, page_address(frag->bio_vec.bv_page) +
				frag->bio_vec.bv_offset + frag_offset);
			sg->length_ = len;
#endif // CONFIG_ODRB_USE_PRDS

			total_len += len;
			frag_offset += len;
			remaining_contig_len -= len;

			if (len == frag_remaining_len) {
				if (frag->head.next) {
					frag = (frag_list_entry_t*)container_of(frag->head.next,
						frag_list_entry_t, head);
					frag_offset = 0;
				} else {
					BUG_ON(remaining_contig_len);
				}
			}
		}
		
		if (new_partial) {
			/* need to copy the required number of bytes to new buffer */
			int length = new_partial->length;
			int total_len = 0;
			memset(new_partial->buffer, 0, sizeof(char) << context->fs_blocklog);
			do {
				unsigned int frag_remaining_len;
				unsigned int len;

				frag_remaining_len = frag->bio_vec.bv_len - frag_offset;

				len = (frag_remaining_len > length) ? length : frag_remaining_len;

				BUG_ON(total_len >= (1 << context->fs_blocklog));

				memcpy(&new_partial->buffer[total_len],
					page_address(frag->bio_vec.bv_page) +
						frag->bio_vec.bv_offset + frag_offset, len);

				frag_offset += len;
				total_len += len;
				length -= len;

				if (len == frag_remaining_len) {
					if (frag->head.next) {
						frag = (frag_list_entry_t*)container_of(frag->head.next,
							frag_list_entry_t, head);
						frag_offset = 0;
					} else {
						BUG_ON(length);
					}
				}

			} while (length);

#ifdef DEBUG
			length = new_partial->length;
			total_len = 0;
			printk("new partial - lba - %ld length - %d\n", (long)new_partial->lba, new_partial->length);
			do {
				printk(KERN_INFO "%2x %2x %2x %2x  %2x %2x %2x %2x -- %2x %2x %2x %2x  %2x %2x %2x %2x \n",
									new_partial->buffer[total_len ],
									new_partial->buffer[total_len + 1],
									new_partial->buffer[total_len + 2],
									new_partial->buffer[total_len + 3],
									new_partial->buffer[total_len + 4],
									new_partial->buffer[total_len + 5],
									new_partial->buffer[total_len + 6],
									new_partial->buffer[total_len + 7],
									new_partial->buffer[total_len + 8],
									new_partial->buffer[total_len + 9],
									new_partial->buffer[total_len + 10],
									new_partial->buffer[total_len + 11],
									new_partial->buffer[total_len + 12],
									new_partial->buffer[total_len + 13],
									new_partial->buffer[total_len + 14],
									new_partial->buffer[total_len + 15]);

				total_len += 16;
			} while(total_len < length);
#endif
		}
		
		// Terminate the SG list
#ifdef CONFIG_ODRB_USE_PRDS
		if (prd) prd->flags_len |= PRD_EOF_MASK;
#else // CONFIG_ODRB_USE_PRDS
		if (sg) sg->next_ = 0;
#endif // CONFIG_ODRB_USE_PRDS

		// Wait for our last SATA transfer to complete
		wait_sata_complete(context);

		if (contig_len) {
			/* Busy wait for ownership of the SATA core from the Linux SATA stack */
#ifdef CONFIG_SATA_OX810
			while (!acquire_sata_core_may_sleep(fast_writes_isr,
				(unsigned long)context, SATA_ACQUIRE_TIMEOUT_JIFFIES)) {
				printk(KERN_WARNING "oxnas_direct_disk_write() Sata acquire timeout \n");
			}
#elif defined(CONFIG_SATA_OX820_DIRECT)
			sata_context = context->inode->writer_filemap_info.direct_access_context;

			while ((*sata_context->acquire)(sata_context, SATA_ACQUIRE_TIMEOUT_JIFFIES, context, 0)) {
				printk(KERN_WARNING "oxnas_direct_disk_write() Sata acquire timeout\n");
			}
#endif // CONFIG_SATA_XXX

			/* Setup SG DMA transfer */
#ifdef CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
			odrb_dma_sata_prd(OXNAS_DMA_TO_DEVICE,
				(total_len + prefilled_bytes) >> SECTOR_SHIFT,
				context->prd_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
			(*sata_context->prepare_command)(sata_context, 1, lba,
				(total_len + prefilled_bytes) >> SECTOR_SHIFT,
				context->prd_list[context->list_idx]->phys, fast_writes_isr,
				context);
#endif // CONFIG_SATA_XXX
#else // CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
			BUG_ON(!context->sg_list[context->list_idx]);
			odrb_dma_sata_sq(OXNAS_DMA_TO_DEVICE,
				(total_len + prefilled_bytes) >> SECTOR_SHIFT,
				context->sg_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
			(*sata_context->prepare_command)(sata_context, 1, lba,
				(total_len + prefilled_bytes) >> SECTOR_SHIFT,
				context->sg_list[context->list_idx]->phys, fast_writes_isr,
				context);
#endif // CONFIG_SATA_XXX
#endif // CONFIG_ODRB_USE_PRDS

			file_context->prev_partial_write_loc = 0; /*reset - for safety */
		}

		/* How much still to be written to SATA after transfer completes? */
		remaining -= (contig_len - prefilled_bytes);

		if (file_context->partial_block) {
			switch(file_context->partial_block->status) {
				case PARTIAL_MEM_TO_WRITE:
					/* this would be setup now to write */
					file_context->partial_block->status = PARTIAL_MEM_ON_GOING;
					break;

				case PARTIAL_MEM_ON_GOING:
					/* completed - free memory */
				default:
					dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
					kfree(file_context->partial_block);
					file_context->partial_block = NULL;
					break;
			}
		}

		/* Need to wait for SATA to complete as we shall be re-using the same
		   DMA SG entry and fragment lists for subsequent transfer to next
		   contiguous region of disk. No need to lock access to flag as we know
		   there's no SATA transfer in progress and therefore the ISR cannot
		   get in */
		wait_sata_inactive = remaining;

		if (new_partial) {
			if (file_context->partial_block) {
				partial_write_wait = 1;
				wait_sata_inactive = 1;
			} else {
				file_context->partial_block = new_partial;
			}
		}

		if (contig_len) {
			int cur_extent_flag = 0;

			atomic_set(&context->cur_transfer_idx, context->list_idx);
			atomic_set(&context->free_sg, 1);
			atomic_set(&context->cur_sg_status[context->list_idx], 1);

			update_list_index(context);

			set_need_to_wait(context);

			/* Issue SATA command to write contiguous sectors to disk */
#ifdef CONFIG_SATA_OX810
			direct_sata_transfer(C_WRITE_DMA_EXT, port, lba, contig_len >> SECTOR_SHIFT);
#elif defined(CONFIG_SATA_OX820_DIRECT)
			(*sata_context->execute_command)();
#endif // CONFIG_SATA_XXX

			cur_extent_flag =
				oxnas_get_extent_status(context->map, filemap_offset->cur_map_entry);

			/* Check to see whether a extent boundary is crossed and extent flags are different */
			if (file_context->write_extent_flag != cur_extent_flag) {
				loff_t temp_end =
					((file_context->write_end_offset - remaining) >>
						context->fs_blocklog) << context->fs_blocklog;

				if (temp_end > file_context->write_start_offset) {
					/* if this is not true its less than a FSB write - flag reset from partial write */
					oxnas_reset_extent_preallocate_flag(fp,
						file_context->write_start_offset,
						temp_end - file_context->write_start_offset,
						file_context->write_extent_flag,
						file_context->disable_accumulation);
				}

				oxnas_set_filesize(fp, temp_end );

				file_context->write_start_offset = temp_end + 1;
				file_context->write_extent_flag =
					oxnas_get_extent_status(context->map,filemap_offset->cur_map_entry);
			}

			if (cur_extent_flag == GETBMAPX_OF_PREALLOC) {
				context->prealloc_write = 1;
			}

			if (wait_sata_inactive) {
				// Wait for ISR to signal SATA transfer finished */
				wait_sata_complete(context);

				if (partial_write_wait) {
					dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
					kfree(file_context->partial_block);
					file_context->partial_block = new_partial;
				}
			}
		}
#ifdef DEBUG
		printk(KERN_INFO "Remaining - %u\n", remaining);
#endif
	} while (remaining);

	/* Update fill and release indices */
	update_context_indices(net_rx_context);

	BUG_ON(remaining);
	
#ifdef DEBUG
	printk(KERN_INFO "fast write leaving - inode %p file - %p\n", fp->inode, fp);
#endif
	
	return length;
}

/* this function is called to do the actual dirty work
 * the network pages are already reference counted and 
 * have to be written to disk after completing all the 
 * necessary checks
 * This function assumes that the caller holds the 
 * necessary locks
 */
static int oxnas_do_disk_flush(
	struct file   *fp,
	loff_t         offset,
	loff_t         count)
{
	oxnas_file_context_t 		*file_context = NULL;
	oxnas_direct_disk_context_t *disk_context = NULL;
	oxnas_net_rx_context_t      *net_rx_context = NULL;
	getbmapx_t     				*map_entry;
	oxnas_filemap_offset_t 		 filemap_offset;
	loff_t 						size_written = offset;
	loff_t		   				 cur_offset = 0;
	loff_t 						 temp_end = 0;
	int 						 bytes_into_block = 0;
	int                          write_result = 0;
	int 						 map_read_flag = 0;
	int 						 hole_once = 1; /* will be reset by the hole logic */
	int 						 preallocate_remap_once = 1;
	int 						 reread_filemap_now = 0;
	struct inode 				*inode = fp->inode;
	
	BUG_ON (!fp->fast_write_context);
	BUG_ON (!inode->writer_file_context);

#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX
 	printk("fast - write Inode %p, file %p, name %s\n", inode, inode->i_ino, fp, fp->f_path.dentry->d_name.name);
#else
	printk("fast - write Inode %p, file %p, name %s\n", inode, fp, fp->f_path.dentry->d_name.name);
#endif
#endif
	
	file_context = (oxnas_file_context_t *) inode->writer_file_context;
	disk_context = (oxnas_direct_disk_context_t *) fp->fast_write_context;
	net_rx_context = &disk_context->net_rx_context;

	/* the loop back for filemap reread has to come back till here
	 * as the offset calculation has to be redone as well with the
	 * new map
	 */
file_map_read:
	/* offset in terms of num sectors */
	cur_offset = (offset >> disk_context->fs_blocklog)
					<< (disk_context->fs_blocklog - SECTOR_SHIFT);

	/* bytes into the calculated FSB */
	bytes_into_block = offset - (cur_offset << SECTOR_SHIFT);

	cancel_delayed_work(&file_context->write_completion_work);

	if (unlikely( (!inode->writer_filemap_info.map) || reread_filemap_now ) ) {
//		printk(KERN_INFO "Reading filemap for inode - %p\n", inode);
		write_result = alloc_filemap(inode, 1);
		if (unlikely(write_result)) {
			printk(KERN_WARNING "fast write - alloc_filemap() failed - fallback init -Inode %p, file %p Failed to allocate filemap, error %d\n", inode, fp, write_result);
			/* have to fall back to normal writes */
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			spin_unlock(&inode->fast_lock);

			up(&inode->writer_filemap_info.sem);
			writer_complete_fallback(inode);

			/* when returning from here, there would have been no part of current write
			 * thats actually read and written to disk. hence its safe to fallback to 
			 * normal writes
			 */
			return OXNAS_FALLBACK; /* use normal writes */
		}
		oxnas_clear_filemap_dirty(inode);
		reread_filemap_now = 0;
	}
	
	memset(&filemap_offset, 0, sizeof(oxnas_filemap_offset_t));

	disk_context->map = &inode->writer_filemap_info.map[1];
#ifdef CONFIG_SATA_OX810
	disk_context->port = inode->writer_filemap_info.port;
#endif // CONFIG_SATA_OX810

	/* position the current map info correctly */
	/* The calculation below is based on sectors - hence need not worry about bytes_into_block */
	do {
		map_entry = &disk_context->map[filemap_offset.cur_map_entry];
//		printk(KERN_INFO "cur offset - %lld, cur map entry length - %lld\n", cur_offset, map_entry->bmv_length);
		if (cur_offset >= map_entry->bmv_length) {
			filemap_offset.cur_map_entry ++;
			/* preallocate handling */
			if (preallocate_remap_once) {
				/* check whether to remap */
				if (filemap_offset.cur_map_entry ==
					inode->writer_filemap_info.map->bmv_entries ) {
					preallocate_remap_once = 0;
#ifdef DEBUG
					printk(KERN_INFO "Fast write - prealloc check - reading filemap again \n");
#endif
					reread_filemap_now = 1;
					goto file_map_read;
				}
			}
			cur_offset -= map_entry->bmv_length;
		} else {
			filemap_offset.cur_map_entry_offset = cur_offset;
			cur_offset = 0;
		}
	} while(cur_offset);
	
	/* Check for holes in current write here */
	if (hole_once) {
		/* This is the first time - hence check for holes */
		int    cur_write_start_entry = filemap_offset.cur_map_entry;
		loff_t cur_total_bytes = bytes_into_block + count;
		loff_t cur_total_sectors = filemap_offset.cur_map_entry_offset +
									(cur_total_bytes >> SECTOR_SHIFT) +
									((cur_total_bytes % SECTOR_SIZE) ? 1 : 0);

		do {
			map_entry = &disk_context->map[cur_write_start_entry];
			if (map_entry->bmv_block == GETBMAPX_BLOCK_HOLE) {
				if (hole_once) {
					/* preallocate */
					/* case 1 - if entire hole less than x MB allocate it
				 	* case 2 - if entire hole greater than x MB allocate x MB (HOLE_PREALLOC_SIZE)
				 	* case 3 - if either of above fail, try allocate bytes to write
				 	*/
					loff_t alloc_start = map_entry->bmv_offset << SECTOR_SHIFT;
					loff_t alloc_len = 0;

					if ((map_entry->bmv_length << SECTOR_SHIFT) < HOLE_PREALLOC_SIZE) {
#ifdef DEBUG
						printk (KERN_INFO "Writing into hole - allocating BMV LENGTH \n");
#endif
						alloc_len = map_entry->bmv_length << SECTOR_SHIFT;
					} else {
#ifdef DEBUG
						printk (KERN_INFO "Writing into hole - allocating HOLE_PREALLOC_SIZE \n");
#endif
						alloc_len = HOLE_PREALLOC_SIZE;
					}

					write_result = fp->f_op->preallocate(fp, alloc_start, alloc_len);
					if (unlikely(write_result < 0)) {
						/* error on preallocate */
						/* try allocating only the required amount if its less than what we tried */
						if (cur_total_sectors < map_entry->bmv_length) {
#ifdef DEBUG
							printk (KERN_INFO "Writing into hole - MIN ALLOC \n");
#endif
							alloc_len = cur_total_sectors << SECTOR_SHIFT;
							write_result = fp->f_op->preallocate(fp, alloc_start,
								alloc_len);
							if (unlikely(write_result < 0)) {
								printk(KERN_ERR "ERROR - PREALLOCATING INTO HOLE - %d\n", write_result);
								goto out;
							}
						}
					}
//					printk(KERN_INFO "Fast writes - Hole found - PREALLOCATING alloc start - %lld length - %lld\n", alloc_start, alloc_len);
				}
				map_read_flag = 1;
				hole_once = 0;
			}
			cur_total_sectors -= map_entry->bmv_length;
			cur_write_start_entry ++;

			/* preallocate handling */
			if ((preallocate_remap_once) && (cur_total_sectors > 0)) {
				/* check whether to remap */
				if (cur_write_start_entry ==
						inode->writer_filemap_info.map->bmv_entries ) {
					preallocate_remap_once = 0;
					/* since we already preallocated into a hole and now have a incomplete filemap
					 * and there cant be two holes next to each other in any FS
					 * just need to reread the map it once
					 */
					if (map_read_flag == 1) map_read_flag = 0; 
#ifdef DEBUG
					printk(KERN_INFO "Fast writes - peralloc - hole - reading filemap again \n");
#endif
					reread_filemap_now = 1;
					goto file_map_read;
				}
			}
		} while(cur_total_sectors > 0);
		hole_once = 0;
	}
	
	if (map_read_flag) {
		map_read_flag = 0;
		/* need to reread the file map */
		reread_filemap_now = 1;
		goto file_map_read;
	}
	/* End of Holes handling */
	
	file_context->fp = fp; /* we are the last to have modified this stuff */

	/* calculation to reset prealloc flag */
	if (file_context->write_end_offset == 0) {
		/* first call - initialize */
		file_context->write_start_offset = offset;
		file_context->write_end_offset = offset + count;
		file_context->write_extent_flag =
			oxnas_get_extent_status(disk_context->map,filemap_offset.cur_map_entry);
		
		if(filemap_offset.cur_map_entry + 1 == inode->writer_filemap_info.map[0].bmv_entries) {
			file_context->last_extent_flag = true;
		} else {
			file_context->last_extent_flag = false;
		}
		
#ifdef DEBUG
		printk(KERN_INFO "offset values changed from 0 - start - %lld, end -%lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
	} else {
		if (file_context->write_end_offset == offset) {
			if (file_context->write_extent_flag !=
					oxnas_get_extent_status(disk_context->map,
						filemap_offset.cur_map_entry)) {
				/* reset old extent */
				long long temp_end =
					(file_context->write_end_offset >> disk_context->fs_blocklog) <<
						disk_context->fs_blocklog;
#ifdef DEBUG
				printk (KERN_INFO "write loop - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
				/* if this is not true it's less than a FSB write - flag reset
				   from partial write */
				if (temp_end > file_context->write_start_offset) {
					oxnas_reset_extent_preallocate_flag(fp,
						file_context->write_start_offset,
						temp_end - file_context->write_start_offset,
						file_context->write_extent_flag,
						file_context->disable_accumulation);
				}

				oxnas_set_filesize(fp, temp_end );

				file_context->write_start_offset = offset;
				file_context->write_end_offset = offset + count;
				file_context->write_extent_flag =
					oxnas_get_extent_status(disk_context->map,
						filemap_offset.cur_map_entry);
			} else {
#ifdef DEBUG
				printk(KERN_INFO "increasing end offset new end -%lld\n", file_context->write_end_offset);
#endif

				if (file_context->write_end_offset - file_context->write_start_offset >=
						META_DATA_UPDATE_SIZE) {
#ifdef DEBUG
					printk (KERN_INFO "META_DATA_UPDATE_SIZE - start offset - %lld, length - %d\n", file_context->write_start_offset, META_DATA_UPDATE_SIZE);
#endif
					oxnas_reset_extent_preallocate_flag(fp,
						file_context->write_start_offset, META_DATA_UPDATE_SIZE,
						file_context->write_extent_flag,
						file_context->disable_accumulation);

					oxnas_set_filesize(fp, file_context->write_start_offset + META_DATA_UPDATE_SIZE);

					file_context->write_start_offset =
						file_context->write_start_offset + META_DATA_UPDATE_SIZE;
#ifdef DEBUG
					printk(KERN_INFO "crossed metadata update size movins start offset - new start - %lld\n", file_context->write_start_offset);	
#endif
				}
				
				file_context->write_end_offset += count;
			}

		} else {
			/* reset old extent */
			/* discontinuity size wil be updated by the partial writer */
			long long temp_end =
				(file_context->write_end_offset >> disk_context->fs_blocklog) <<
					disk_context->fs_blocklog;
#ifdef DEBUG
			printk (KERN_INFO "write loop - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
			/* if this is not true its less than a FSB write - flag reset from
			   partial write */
			if (temp_end > file_context->write_start_offset) {
				oxnas_reset_extent_preallocate_flag(fp,
					file_context->write_start_offset,
					temp_end - file_context->write_start_offset,
					file_context->write_extent_flag,
					file_context->disable_accumulation);
			}
			oxnas_set_filesize(fp, temp_end );

			/* reread filemap if needed
			 * 1 - filemap dirty
			 * 2 - bytes_into_sector
			 * 3 - partial last block present
			 */
			if(oxnas_check_filemap_dirty(fp->inode)) {
				loff_t 	temp = (bytes_into_block + count) >> disk_context->fs_blocklog;
				temp = bytes_into_block + count - (temp << disk_context->fs_blocklog);
				/* can be optimised a bit more
				 * now the filemap is reread for prealloced and already written extents
				 * can be modified to reread only if we have a prealloc region and the
				 * filemap is dirty
				 */
				if (bytes_into_block || temp) {
					/* setting the offsets to zero will make sure we fall through
					 * a new write case on comeback and avoid unneeded checks
					 */
					file_context->prev_partial_write_loc = 0;
					file_context->write_start_offset = 0;
					file_context->write_end_offset = 0;
					reread_filemap_now = 1;
					goto file_map_read;
				}
			}

			file_context->write_start_offset = offset;
			file_context->write_end_offset = offset + count;
			file_context->write_extent_flag =
				oxnas_get_extent_status(disk_context->map,
					filemap_offset.cur_map_entry); 
			
			if(filemap_offset.cur_map_entry + 1 == inode->writer_filemap_info.map[0].bmv_entries) {
				file_context->last_extent_flag = true;
			} else {
				file_context->last_extent_flag = false;
			}
			

#ifdef DEBUG
			printk(KERN_INFO "Discontinuous locations new  - start - %lld, end -%lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
		}
	}
	/* end of prealloc flag calculation */
	
	i_tent_size_write(inode, file_context->write_end_offset);
	
	disk_context->prealloc_write = 0;
	
//	printk(KERN_INFO "before actual writes - fc - %p, dc - %p, cur_file_map - %d, count - %lld, size_written - %lld, bytes in block - %d, file - %p\n",
//										file_context, disk_context,
//											filemap_offset.cur_map_entry, count,	size_written,
//											bytes_into_block, fp);

	/* Write all received network data directly to disk */
	write_result = oxnas_direct_disk_write(file_context, disk_context,
											&filemap_offset, count,	size_written,
											bytes_into_block, fp);
											
											
	/* Release pages holding network data */
	release_netdma_net_frags(net_rx_context);
	
/* dont wait for write completion and cleanup here as there may be more to write when write is called again
 * or can do the cleanup on complete/ close of file
 */
 	if (disk_context->prealloc_write) {
 		oxnas_set_filemap_dirty(inode);
 		/* if the end of write is less than file size
 		 * have written to a hole - immediately reset 
 		 * prealloc flag - no need to set size
 		 */
 		 if ( (file_context->write_end_offset <= oxnas_get_filesize(inode)) && 
 		 					(!file_context->last_extent_flag) ) {
 		 	temp_end = (file_context->write_end_offset >> disk_context->fs_blocklog)
				<< disk_context->fs_blocklog;
#ifdef DEBUG
			printk (KERN_INFO "writing inside file size - cur file size - %lld \n", oxnas_get_filesize(inode));
#endif 
 		 	goto prealloc_reset_now;
 		 }
 		 
 		 if (file_context->disable_accumulation) {
 		 	/* hold on - we have a prealloc and writing after the current file size
 		 	 * but still have to update the meta data as we dont accumulate 
 		 	 * any more as there may be possible readers who need immediate updates
 		 	 */
			temp_end = (file_context->write_end_offset >> disk_context->fs_blocklog)
							<< disk_context->fs_blocklog;
#ifdef DEBUG
			printk (KERN_INFO "accumulation disabled - calling metadata update \n");
#endif 
			if(temp_end > oxnas_get_filesize(inode)) {
				oxnas_set_filesize(fp, temp_end);
			}
 		 	goto prealloc_reset_now; 
 		 }
 	} else {
		/* not writing to a prealloc extent - invalidate reader immediately */
		{
 			temp_end = (file_context->write_end_offset >> disk_context->fs_blocklog)
				<< disk_context->fs_blocklog;

prealloc_reset_now:
#ifdef DEBUG
			printk (KERN_INFO "end of write - reset prealloc /invalidate - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
			/* if this is not true its less than a FSB write - flag reset from
			   partial write */
			if (temp_end > file_context->write_start_offset) {
				oxnas_reset_extent_preallocate_flag(fp,
					file_context->write_start_offset,
					temp_end - file_context->write_start_offset,
					file_context->write_extent_flag,
					file_context->disable_accumulation);
			}
			oxnas_set_filesize(fp, temp_end);
			
			file_context->write_start_offset = offset;
			file_context->write_end_offset = offset + count;
			file_context->write_extent_flag =
				oxnas_get_extent_status(disk_context->map,
					filemap_offset.cur_map_entry);
			
			i_tent_size_write(inode, file_context->write_end_offset);
		}
 	}
 	
 	/* always flush the partial to disk with normal write
 	 * This is done to ease the synchronisation with the
 	 * readers. Because of this assumption, the actual
 	 * writes code is simplified not to handle partial
 	 * writes
 	 */
 	disk_context->prealloc_write = 0;
 	oxnas_direct_disk_complete_partial_write(file_context, disk_context);

 	if(disk_context->prealloc_write) {
 		/* we ahve written a partial to a prealloc region */
 		oxnas_set_filemap_dirty(inode);
 	}

	schedule_delayed_work(&file_context->write_completion_work,
		msecs_to_jiffies(OXNAS_WRITER_TIMEOUT));
		
#ifdef DEBUG	
	printk("Exiting fastwrite Inode %p, file %p, name %s \n", inode, fp, fp->f_path.dentry->d_name.name);
#endif

out:
	if (write_result <= 0)
		return write_result;
	else if(check_write_error(disk_context))
		return -EIO;
	else
		return count;
}

int oxnas_do_direct_disk_write(
	struct socket *socket,
	struct file   *fp,
	loff_t         offset,
	loff_t         count)
{
	loff_t						remaining_to_receive = count;
	loff_t						 loop_to_receive = 0;
	oxnas_direct_disk_context_t *disk_context = NULL;
	oxnas_net_rx_context_t      *net_rx_context = NULL;
	oxnas_file_context_t 		*file_context = NULL;
	read_descriptor_t            desc;
	ssize_t                      received_from_net;
	int                          read_result = 0;
	int                          write_result = 0;
	int 						write_now = 0;
	int 						loop_back_to_read_more = 0;
	struct inode 				*inode = fp->inode;

#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX
	printk("ENTERING fastwrite Inode %p[%lu], file %p, name %s \n", inode, inode->i_ino, fp, fp->f_path.dentry->d_name.name);
#else
	printk("ENTERING fastwrite Inode %p, file %p, name %s \n", inode, fp, fp->f_path.dentry->d_name.name);
#endif
#endif
	
	spin_lock(&inode->fast_lock);
	if (unlikely(inode->fallback_in_progress)) {
		wait_fallback_complete(inode);
		spin_unlock(&inode->fast_lock);
		/* At this point, fallback has completed and the 
		 * current write has to be completed by net to cache
		 * writes - return custom error message
		 */
		 return OXNAS_FALLBACK;
	} else {
		// Increment in-progress write count so that any fast fallback
		// request issued while we have dropped the lock to do context
		// allocation will not be able to proceed
		++inode->fast_writes_in_progress_count;	
	}
	spin_unlock(&inode->fast_lock);

	smp_rmb();
	
	if (!fp->fast_write_context) { /* first write - need to initialise */
#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX
		printk("Init fastwrite Inode %p[%lu], file %p, name %s\n", inode, inode->i_ino, fp, fp->f_path.dentry->d_name.name);
#else
		printk("Init fastwrite Inode %p, file %p, name %s\n", inode, fp, fp->f_path.dentry->d_name.name);
#endif
#endif
		read_result = init_write_file_context(inode, fp);
		if (unlikely(read_result)) {
			/* have to fall back to normal writes here */
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			--inode->fast_writes_in_progress_count;
			spin_unlock(&inode->fast_lock);

			writer_complete_fallback(inode);

			return OXNAS_FALLBACK; /* use normal writes */
		}
	}

#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX	
 	printk("fast - write Inode %p[%lu], file %p, name %s\n", inode, inode->i_ino, fp, fp->f_path.dentry->d_name.name);
#else
	printk("fast - write Inode %p, file %p, name %s\n", inode, fp, fp->f_path.dentry->d_name.name);
#endif
#endif

	/* grab the exclusive write access */
	while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
		printk("oxnas_do_direct_disk_write() A second has elapsed while waiting, inode %p\n", inode);
	}

	/* check whether the inode elements are initialised */
	if (unlikely(!inode->writer_file_context)) {
		/* have to initialise inode */
		read_result = do_initial_inode_prep(inode, fp);
		if (unlikely(read_result)) {
			/* have to fall back to normal writes */
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			spin_unlock(&inode->fast_lock);

			up(&inode->writer_filemap_info.sem);
			writer_complete_fallback(inode);

			return OXNAS_FALLBACK; /* use normal writes */
		}
	}
	/* end of inode init stuff */
	file_context = (oxnas_file_context_t *)inode->writer_file_context;
	disk_context = (oxnas_direct_disk_context_t *) fp->fast_write_context;
	net_rx_context = &disk_context->net_rx_context;
	
	/* in memory accumulation belongs to different fp - complete it first */
	if( (file_context->acc_fp) && (file_context->acc_fp != fp) ) {
//		printk(KERN_INFO "FAST WRITES - more than one fp writing to same inode - if you see this let me know\n");
		if( complete_accumulated_write(file_context) < 0) {
			/* need not propogate the error to client from here */
			printk(KERN_INFO "accumulated write of some other file pointer failed \n");
		}
		file_context->acc_fp = 0;
	}
	
read_more_data:
	write_result = read_result = 0;
	if (net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length == 0) {
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length = remaining_to_receive;
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset = offset;
//		printk (KERN_INFO "New accumulation - file - %p offset - %lld count - %lld - idx - %d\n", fp, offset, remaining_to_receive, net_rx_context->fill_frag_list_idx);
		
	} else if(offset == net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length 
					+ net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset) { 
		
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length += count;
		/* start offset remains the same */
//		printk (KERN_INFO "Continuing accumulation - file - %p offset - %lld count - %lld - idx - %d\n", fp, offset, count, net_rx_context->fill_frag_list_idx);
					
	} else { /* discontinuous */
		write_now = 1;
		loop_back_to_read_more = 1;
//		printk (KERN_INFO "discontinuous writes on accumulation - file - %p offset - %lld count - %lld - idx - %d\n", fp, offset, count, net_rx_context->fill_frag_list_idx);
		goto do_actual_write;
	}
	
	/* reference count from network */
	desc.arg.data = net_rx_context;
	/* process the data and writing to disk */

	/* Initialise the descriptor that will track progress of acquiring
	   network page references */
	desc.written = 0;
	desc.error = 0;

	loop_to_receive = remaining_to_receive;

	/* acquire data from network */
	received_from_net = 0;
	while (loop_to_receive > 0) {
		size_t bytes_read = 0;

		desc.count = loop_to_receive;

		read_result = oxnas_net_read_sock(socket->sk, &desc, oxnas_net_rx_actor,
			1, &bytes_read);

		received_from_net    += bytes_read;
		loop_to_receive      -= bytes_read;
		remaining_to_receive -= bytes_read;

		if (read_result < 0) {
			printk (KERN_INFO "Fast writes - Read error - inode - %p, file - %p, read_result %d, bytes_read = %d\n",
				inode, fp, read_result, bytes_read);
			break;
		}
		
		if (unlikely(net_rx_context->frag_cnt[net_rx_context->fill_frag_list_idx] >= net_rx_context->max_frag_cnt)) {
#ifdef DEBUG
			printk(KERN_INFO "reached num of frags to fill - breaking fill loop - inode - %p, file - %p max frags - %d no. reached - %d\n", 
									inode, fp, net_rx_context->max_frag_cnt, net_rx_context->frag_cnt[net_rx_context->fill_frag_list_idx]);
#endif
			write_result = -1;
			break;
		}
	}
		
	if(read_result < 0) {
		/* read error - adjust how much to write and write now and return error */
//		printk(KERN_INFO "Read_result less than ZERO \n");
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length -= remaining_to_receive;
		write_now = 1;
	}
		
	if(write_result < 0) {
		/* we have reached the max no. frags - adjust amount we write, write and loop back */
//		printk(KERN_INFO "Write result less than ZERO \n");
		offset += received_from_net;
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length -= remaining_to_receive;
		loop_back_to_read_more = 1;
		write_now = 1;
	}
		
do_actual_write:
	if( file_context->disable_accumulation || write_now ||
			 ( net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length >= NET_SAMBA_RX_CHUNK_SIZE ) ){
			 
//		printk(KERN_INFO "Actual writes - write now - %d length to write - %lld\n", write_now, net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length);
		
		file_context->acc_fp = fp;
		write_result = complete_accumulated_write(file_context);
							 
		write_now = 0;
	} else { /* NOT ACCUMULATED ENOUGH */
		/* release locks and return here */
		
//		printk(KERN_INFO "Delaying write for later \n");
		read_result = count;
		write_result = count; /* prevent from returning any errors */
		file_context->acc_fp = fp; /* this fp is the accumulator */
	}
	
	if(loop_back_to_read_more && (read_result >= 0)) {
//		printk(KERN_INFO "loop back \n");
		loop_back_to_read_more = 0;
		goto read_more_data;
	}

 	/* release the exclusive write access */
	up(&inode->writer_filemap_info.sem);

	spin_lock(&inode->fast_lock);
	// Decrement the write in progress count
	--inode->fast_writes_in_progress_count;
	if (unlikely(inode->fallback_in_progress) &&
		!inode->fast_writes_in_progress_count &&
		!inode->fast_reads_in_progress_count) {
		/* last in line - initiate fall back
		 */
printk(KERN_INFO "Initiating fallback from fast write\n");
		do_fast_fallback(inode);
	}
	spin_unlock(&inode->fast_lock);
	
#ifdef DEBUG	
	printk("Exiting fastwrite Inode %p, file %p, name %s \n", inode, fp, fp->f_path.dentry->d_name.name);
#endif

	if ((read_result < 0) || (write_result < 0)) {
		return read_result ? : write_result;
	} else {
		return count;
	}
}

static void fast_write_free_filemap(struct inode * inode)
{
	oxnas_file_context_t *file_context =
		(oxnas_file_context_t *)inode->writer_file_context;

	smp_rmb();		
	if (file_context) {
		cancel_delayed_work(&file_context->write_completion_work);
	}

	kfree(inode->writer_filemap_info.map);
	inode->writer_filemap_info.map = NULL;
#ifdef CONFIG_SATA_OX820_DIRECT
	if (inode->writer_filemap_info.direct_access_context) {
		free_direct_sata_context(inode->writer_filemap_info.direct_access_context);
		inode->writer_filemap_info.direct_access_context = NULL;
	}
#endif // defined(CONFIG_SATA_OX820_DIRECT)

	kfree(file_context);
	inode->writer_file_context = NULL;
	smp_wmb();
}

/* this function frees the fast write file context
 * and file map 
 */
void fast_write_check_and_free_filemap(struct inode * inode)
{
	/* grab the exclusive write access */
	if (!down_trylock(&inode->writer_filemap_info.sem)) {
		fast_write_free_filemap(inode);
		up(&inode->writer_filemap_info.sem);
	}
}

void __write_flush_filemap(struct inode *inode)
{
	struct file *file;

	if (!inode->writer_filemap_info.map || !inode->writer_file_context) {
		return;
	}

	/* grab the exclusive write access */
	while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
		printk("__write_flush_filemap() A second has elapsed while waiting, inode %p\n", inode);
	}

	file = ((oxnas_file_context_t*)(inode->writer_file_context))->fp;

	WARN_ON(!file);

	if (file) {
		writer_reset_prealloc(file);
	}

	/* release the exclusive write access */
	up(&inode->writer_filemap_info.sem);
}

void write_flush_pending(
	struct inode *inode,
	int           disable_accumulation)
{
	smp_rmb();
	if(inode->writer_file_context) {
		oxnas_file_context_t 		*file_context =
							(oxnas_file_context_t *)inode->writer_file_context;
							
//		printk(KERN_INFO "write file context present - flushing - inode - %p\n", inode);
		
		if(file_context->acc_fp) {
//			printk(KERN_INFO "write acc pointer present \n");

			/* grab the exclusive write access */
			while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
				printk("write_flush_pending() A second has elapsed while waiting, inode %p\n", inode);
			}

			if(file_context->acc_fp) {
#ifdef DEBUG
				printk(KERN_INFO "Writing from write flush pending\n");
#endif
				if( complete_accumulated_write(file_context) < 0) {
					/* need not bother of write error here as its the readers who
					 * have asked to flush the writes - no client
					 */
					printk(KERN_INFO "FAST WRITE COMPLETE - accumulated write of failed \n");
				}
				file_context->acc_fp = 0;
			}
			
			/* release the exclusive write access */
			up(&inode->writer_filemap_info.sem);
		}
		if(disable_accumulation)
			file_context->disable_accumulation = disable_accumulation;
		smp_wmb();
//		printk (KERN_INFO "after flushing - inode size - %lld\n", i_size_read(inode));
	}
}

/* this function is a wrapper to the actual flush work. This increments the 
 * write count and as well takes care of fallback as needed
 */
void flush_writes(struct inode *inode)
{
	smp_rmb();
	if (inode->writer_file_context) {
		spin_lock(&inode->fast_lock);
		if (unlikely(inode->fallback_in_progress)) {
			wait_fallback_complete(inode);
			spin_unlock(&inode->fast_lock);
			/* At this point, fallback has completed and the 
			 * current write has to be completed by net to cache
			 * writes - return custom error message
			 */
			 return;
		} else {
			// Increment in-progress write count so that any fast fallback
			// request issued while we have dropped the lock to do context
			// allocation will not be able to proceed
			++inode->fast_writes_in_progress_count;	
		}
		spin_unlock(&inode->fast_lock);
	
		write_flush_pending(inode, 0);
		write_flush_filemap(inode);
		
		spin_lock(&inode->fast_lock);
		// Decrement the write in progress count
		--inode->fast_writes_in_progress_count;
		if (unlikely(inode->fallback_in_progress) &&
			!inode->fast_writes_in_progress_count &&
			!inode->fast_reads_in_progress_count) {
			/* last in line - initiate fall back
			 */
printk(KERN_INFO "Initiating fallback from flush writes\n");
			do_fast_fallback(inode);
		}
		spin_unlock(&inode->fast_lock);
	}
}

/* this function is invoked from file truncate to 
 * reread the writer file map and invoke the 
 * readers to reread the filemap as well
 */
void writer_remap_file(struct inode *inode)
{
	int fallback = 0;
	int result = 0;
	smp_rmb();
	if(inode->writer_filemap_info.map) {
		spin_lock(&inode->fast_lock);
		if (unlikely(inode->fallback_in_progress)) {
			wait_fallback_complete(inode);
			spin_unlock(&inode->fast_lock);
			/* At this point, fallback has completed and the 
			 * current write has to be completed by net to cache
			 * writes - return custom error message
			 */
			 return;
		} else {
			// Increment in-progress write count so that any fast fallback
			// request issued while we have dropped the lock to do context
			// allocation will not be able to proceed
			++inode->fast_writes_in_progress_count;	
		}
		spin_unlock(&inode->fast_lock);

		/* grab the exclusive write access */
		while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
			printk("writer_remap_file() A second has elapsed while waiting, inode %p\n", inode);
		}

		/* remap the file here */
		result = alloc_filemap(inode, 1);
		if (unlikely(result)) {
			printk(KERN_WARNING "writer_remap_file - alloc_filemap() failed - fallback init -Inode %p, Failed to allocate filemap, error %d\n", inode, result);
			/* have to fall back to normal writes */
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			// Decrement the write in progress count
			--inode->fast_writes_in_progress_count;
			if( (!inode->fast_writes_in_progress_count) 
					&& (!inode->fast_reads_in_progress_count) ) {
				fallback = 1;
			}
			spin_unlock(&inode->fast_lock);

			up(&inode->writer_filemap_info.sem);

			if(fallback) { /* implies we have to do the cleanup */
				writer_complete_fallback(inode);
			}

			/* when returning from here, there would have been no part of current write
			 * thats actually read and written to disk. hence its safe to fallback to 
			 * normal writes
			 */
			return; /* use normal writes */
		}
		oxnas_clear_filemap_dirty(inode);

		/* release the exclusive write access */
		up(&inode->writer_filemap_info.sem);

		spin_lock(&inode->fast_lock);
		// Decrement the write in progress count
		--inode->fast_writes_in_progress_count;
		if (unlikely(inode->fallback_in_progress) &&
			!inode->fast_writes_in_progress_count &&
			!inode->fast_reads_in_progress_count) {
			/* last in line - initiate fall back
			 */
	printk(KERN_INFO "Initiating fallback from writer_remap_file\n");
			do_fast_fallback(inode);
			fallback = 1;
		}
		spin_unlock(&inode->fast_lock);
	}
	
	/* invoke reader to remap file */
	if((!fallback) && inode->filemap_info.map) {
printk("writer_remap_file() inode %p re-mapping reader filemap\n", inode);
		incoherent_sendfile_remap_file(inode);
	}
}

void writer_reset_prealloc(struct file * file)
{
	struct inode 				*inode = file->inode;
	oxnas_direct_disk_context_t *disk_context =
		(oxnas_direct_disk_context_t *)file->fast_write_context;
	oxnas_file_context_t 		*file_context =
		(oxnas_file_context_t *)inode->writer_file_context;

	if (file_context->write_end_offset) {
		long long temp_end =
			(file_context->write_end_offset >> disk_context->fs_blocklog) <<
				disk_context->fs_blocklog;
#ifdef DEBUG
		printk (KERN_INFO "complete_direct_disk_write - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif

		if (temp_end > file_context->write_start_offset) {
			oxnas_reset_extent_preallocate_flag(file,
				file_context->write_start_offset,
				temp_end - file_context->write_start_offset,
				file_context->write_extent_flag,
				file_context->disable_accumulation);
				
			file_context->write_start_offset = temp_end + 1;
		}
		oxnas_set_filesize(file, file_context->write_end_offset);
	}
}

int complete_fast_write(struct file	*fp)
{
	oxnas_file_context_t        *file_context = NULL;
	oxnas_direct_disk_context_t *disk_context = NULL;
	oxnas_net_rx_context_t      *net_rx_context = NULL;
	struct inode                *inode;
	int                         schedule_queue = 1;
	int 						retval = 0;

	smp_rmb();
	
	if (fp->fast_write_context == NULL) {
		printk(KERN_INFO "Fast Write - complete - no context return\n");
		return 0;
	}

	inode = fp->inode;
	
	/* grab the exclusive write access */
	while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
		printk("complete_fast_write() A second has elapsed while waiting, inode %p\n", inode);
	}

	file_context = (oxnas_file_context_t *)inode->writer_file_context;
	disk_context = (oxnas_direct_disk_context_t *)fp->fast_write_context;
//printk("complete_fast_write() Disk context %p\n", disk_context);
	net_rx_context = & (disk_context->net_rx_context);

	if ((file_context->fp == NULL) || (file_context->fp == fp)) schedule_queue = 0;
	
	/* complete any pending writes */
	if(file_context->acc_fp == fp ) {
//		printk(KERN_INFO "writing from close/ comlpete file \n");
		retval = complete_accumulated_write(file_context);
		if(retval < 0) {
			printk(KERN_INFO "FAST WRITE COMPLETE - accumulated write failed \n");
		}
		file_context->acc_fp = 0;
	}
	
	cancel_delayed_work(&file_context->write_completion_work);
	flush_scheduled_work();
	
//	printk(KERN_INFO "After completing disk flush\n");

	/* Wait for the last SATA transfer to finish before freeing the network
	   fragment storage */
	oxnas_direct_disk_complete(disk_context);

	/* Free network fragments used in final SATA transfer */
	release_netdma_net_frags(&(disk_context->net_rx_context));

	release_netdma_net_frags_by_index(&disk_context->net_rx_context,
		disk_context->net_rx_context.fill_frag_list_idx);

	if (net_rx_context->frag_cache) {
		/* Free the network fragment storage */
		kmem_cache_destroy(net_rx_context->frag_cache);
	}

	if (disk_context->buffer) {
		/* free the read buffer */
		dma_free_coherent(0, sizeof(char) << disk_context->fs_blocklog, disk_context->buffer, disk_context->buffer_pa);
		disk_context->buffer = NULL;
	}

	/* free the file context */
	kfree(disk_context);

#ifdef DEBUG	
	printk(KERN_INFO "Fast Write - Close\n");
#endif

	if (schedule_queue) {
		schedule_delayed_work(&file_context->write_completion_work,
			msecs_to_jiffies(OXNAS_WRITER_TIMEOUT));
	}

	/* release the exclusive write access */
	up(&inode->writer_filemap_info.sem);

	if (retval < 0)
		return retval;
	else
		return 0;
}
