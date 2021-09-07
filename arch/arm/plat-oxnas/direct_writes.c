 
#include <net/tcp.h>
#include <mach/direct_writes.h>
#include <mach/oxnas_errors.h>
#include <mach/oxnas_direct_disk.h>
#include <mach/incoherent_sendfile.h>
#include <mach/sata_helper.h>

#define SATA_ACQUIRE_TIMEOUT_JIFFIES (30*HZ)

#undef NO_DISK_WRITE

#define OXNAS_WRITER_TIMEOUT 	CONFIG_OXNAS_WRITE_TIMEOUT

#define NET_SAMBA_RX_CHUNK_SIZE (CONFIG_OXNAS_WRITE_ACCUMULATION * 1024)

#define META_DATA_UPDATE_SIZE 		(CONFIG_OXNAS_WRITE_METADATA_UPDATE_SIZE * 1024 * 1024)
#define HOLE_PREALLOC_SIZE (CONFIG_OXNAS_WRITE_HOLE_PREALLOC_SIZE * 1024 * 1024)

static int oxnas_do_disk_flush( struct file *fp, loff_t offset, loff_t count);

enum {
	PARTIAL_MEM_FREE = 0,
	PARTIAL_MEM_TO_WRITE,
	PARTIAL_MEM_ON_GOING
};

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
#endif  

#ifdef CONFIG_ODRB_USE_PRDS
	prd_table_entry_t *prd = NULL;

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

	prd->adr = buffer_pa;
	prd->flags_len = (num_of_sectors << SECTOR_SHIFT) | PRD_EOF_MASK;
#else  
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

	sg->addr_ = buffer_pa;
	sg->length_ = num_of_sectors << SECTOR_SHIFT;
	sg->next_ = 0;
	}
#endif  

	wait_sata_complete(context);

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
#endif  

	if (unlikely((file_context->partial_block) &&
		(file_context->partial_block->status == PARTIAL_MEM_ON_GOING))) {
		dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
		kfree (file_context->partial_block);
		file_context->partial_block = NULL;
	}

#ifdef CONFIG_ODRB_USE_PRDS
#ifdef CONFIG_SATA_OX810
	odrb_dma_sata_prd(OXNAS_DMA_FROM_DEVICE, num_of_sectors,
		context->prd_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
	(*sata_context->prepare_command)(sata_context, 0, lba,
		num_of_sectors, context->prd_list[context->list_idx]->phys,
		fast_writes_isr, context);
#endif  
#else  
#ifdef CONFIG_SATA_OX810
	odrb_dma_sata_sq(OXNAS_DMA_FROM_DEVICE, num_of_sectors,
		context->sg_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
	(*sata_context->prepare_command)(sata_context, 0, lba,
		num_of_sectors, context->sg_list[context->list_idx]->phys,
		fast_writes_isr, context);
#endif  
#endif  

	atomic_set(&context->cur_transfer_idx, context->list_idx);
	atomic_set(&context->free_sg, 1);
	atomic_set(&context->cur_sg_status[context->list_idx], 1);

	update_list_index(context);

	set_need_to_wait(context);

#ifdef CONFIG_SATA_OX810
	direct_sata_transfer(C_READ_DMA_EXT, port, lba, num_of_sectors);
#elif defined(CONFIG_SATA_OX820_DIRECT)
	(*sata_context->execute_command)();
#endif  

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
#endif  
	unsigned char *			buf_ptr = NULL;
#ifdef CONFIG_ODRB_USE_PRDS
	prd_table_entry_t *prd = NULL;
#else  
	odrb_sg_entry_t *prev_sg = NULL;
	odrb_sg_entry_t *sg = NULL;
#endif  

	if (file_context->partial_block) {
		if(!context->buffer) {
			context->buffer = dma_alloc_coherent(0, sizeof(char) << context->fs_blocklog, &context->buffer_pa, GFP_KERNEL);
			BUG_ON(!context->buffer);
		}
		 
		if (file_context->partial_block->unwritten == GETBMAPX_OF_PREALLOC) {
			if (file_context->prev_partial_write_loc == file_context->partial_block->lba) {
				goto read_from_disk;
			}
			 
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
#else  

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
#endif  

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
#endif  

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
#endif  
#else  
#ifdef CONFIG_SATA_OX810
		odrb_dma_sata_sq(OXNAS_DMA_TO_DEVICE,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)),
			context->sg_list[context->list_idx]->phys, 1);
#elif defined(CONFIG_SATA_OX820_DIRECT)
		(*sata_context->prepare_command)(sata_context, 1,
			file_context->partial_block->lba,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)),
			context->sg_list[context->list_idx]->phys, fast_writes_isr, context);
#endif  
#endif  

		atomic_set(&context->cur_transfer_idx, context->list_idx);
		atomic_set(&context->free_sg, 1);
		atomic_set(&context->cur_sg_status[context->list_idx], 1);

		update_list_index(context);

		set_need_to_wait(context);

#ifdef CONFIG_SATA_OX810
		direct_sata_transfer(C_WRITE_DMA_EXT, port,
			file_context->partial_block->lba,
			(1 << (context->fs_blocklog - SECTOR_SHIFT)));
#elif defined(CONFIG_SATA_OX820_DIRECT)
		(*sata_context->execute_command)();
#endif  

		if (file_context->partial_block->unwritten == GETBMAPX_OF_PREALLOC) {
			context->prealloc_write = 1; 
		}
		file_context->prev_partial_write_loc = file_context->partial_block->lba;

		wait_sata_complete(context);

		oxnas_reset_extent_preallocate_flag(file_context->partial_block->fp,
				file_context->partial_block->start_offset_into_file - file_context->partial_block->bytes_into_block,
				1 << context->fs_blocklog,
				file_context->partial_block->unwritten,
				file_context->disable_accumulation);

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
		
#else  
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
				
		write_result = oxnas_do_disk_flush( acc_fp, temp_offset, temp_count);
								 
#endif  
	}
	
	file_context->acc_fp = 0;
	smp_wmb();
	
	return write_result;
}

static void update_context_indices(oxnas_net_rx_context_t *context)
{
	 
	if (++context->release_frag_list_idx > 1) {
	   context->release_frag_list_idx = 0;
	}
	if (++context->fill_frag_list_idx > 1) {
	   context->fill_frag_list_idx = 0;
	}
}

static void oxnas_direct_disk_complete(oxnas_direct_disk_context_t *context)
{
	 
	wait_sata_complete(context);

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
		 
		++inode->fast_writes_in_progress_count;	
	}
	spin_unlock(&inode->fast_lock);

	if (down_trylock(&inode->writer_filemap_info.sem)) {
		 
		goto spin_lock_out;
	}

	if(acc_fp) {
		 
		if( complete_accumulated_write(file_context) < 0) {
			 
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
			
		oxnas_direct_disk_complete(disk_context);

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

		release_netdma_net_frags(&disk_context->net_rx_context);
		release_netdma_net_frags_by_index(&disk_context->net_rx_context,
			disk_context->net_rx_context.fill_frag_list_idx);
	}

	up(&inode->writer_filemap_info.sem);

spin_lock_out:	
	spin_lock(&inode->fast_lock);
	--inode->fast_writes_in_progress_count;
	if (unlikely(inode->fallback_in_progress) &&
		!inode->fast_writes_in_progress_count && 
		!inode->fast_reads_in_progress_count) {
		 
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

static int do_initial_inode_prep(
	struct inode *inode,
	struct file  *file)
{
	int retval = 0;

	retval = vfs_fsync(file, file->f_dentry, 0);
	if (unlikely(retval)) {
		printk(KERN_WARNING "fast write - fsync() Inode %p, file %p fsync failed, error %d\n", inode, file, retval);
	} else {
		 
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
	} else {  
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

	if (unlikely(fp->fast_write_context) ) {
		printk(KERN_INFO "Setting file context more than once for the same file- ERROR\n");
		result = -EINVAL;
		goto out;
	}

	disk_context = kzalloc(sizeof(oxnas_direct_disk_context_t), GFP_KERNEL);
	if (unlikely(!disk_context)) {
		result = -ENOMEM;
		printk(KERN_INFO "Alloc memory for map_info failed\n");
		goto out;
	}

	memset(disk_context, 0, sizeof(oxnas_direct_disk_context_t));

	atomic_set(&disk_context->free_sg, 0);
	atomic_set(&disk_context->cur_transfer_idx, -1);

	sema_init(&disk_context->sata_active_sem, 0);

	disk_context->fs_blocklog = oxnas_get_fs_blocksize(fp);
	disk_context->list_idx = 0;

	disk_context->buffer = NULL;

	net_context = &disk_context->net_rx_context;

#ifdef CONFIG_ODRB_USE_PRDS
	net_context->max_frag_cnt = CONFIG_ODRB_WRITER_PRD_ARRAY_SIZE - 3; 
#else  
	net_context->max_frag_cnt = CONFIG_ODRB_NUM_WRITER_SG_ENTRIES - 3; 
#endif  
																	  
	for (i=0; i < NUM_NET_RX_FRAG_LISTS; i++) {
		INIT_LIST_HEAD(&net_context->page_info[i]);
		net_context->frag_cnt[i] = 0;
		net_context->data_ref[i].length = 0;
	}
	
	for (i=0; i < NUM_SATA_LISTS; i++) {
		atomic_set(&disk_context->cur_sg_status[i], 0);
	}

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
#else  
		disk_context->sg_list[i] = NULL;
#endif  
	}

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

	return 0;
}

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
#endif  
	frag_list_entry_t      *frag;
	unsigned int            frag_offset = 0;
	ssize_t                 remaining = length;
	oxnas_partial_block_t *new_partial = NULL;

#ifdef DEBUG
	printk(KERN_INFO "direct disk write 2 called inode - %p, file - %p, start offset - %lld, length -%u, bytesinto block - %d\n", fp->inode, fp, start_offset, length, bytes_into_block);
#endif

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
#else  
		odrb_sg_entry_t *sg = NULL;
#endif  
#ifdef CONFIG_SATA_OX820_DIRECT
		direct_access_context_t *sata_context;
#endif  

		map_entry = &context->map[filemap_offset->cur_map_entry];
		map_offset = filemap_offset->cur_map_entry_offset;

		map_start  = map_entry->bmv_offset;
		map_len    = map_entry->bmv_length;

		lba = map_entry->bmv_block + map_offset;
#ifdef CONFIG_SATA_OX810
		lba += part_offset;
#endif  

		BUG_ON (file_context->partial_block);

		if (bytes_into_block &&
			((bytes_into_block + remaining) >= (1 << context->fs_blocklog))) {
			 
			if(!context->buffer) {
				context->buffer = dma_alloc_coherent(0, sizeof(char) << context->fs_blocklog, &context->buffer_pa, GFP_KERNEL);
				BUG_ON(!context->buffer);
			}

			if (oxnas_get_extent_status(context->map, filemap_offset->cur_map_entry) == GETBMAPX_OF_PREALLOC) {
				if (file_context->prev_partial_write_loc == lba) {
					 
					goto read_from_disk;
				} else {
					 
					context->prealloc_write = 1;
				}
				 
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
#else  
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
#endif  

			prefilled_bytes = bytes_into_block;
			bytes_into_block = 0;
		}

		contig_len = (map_len - map_offset) << SECTOR_SHIFT;
		
		BUG_ON(!contig_len);

		if (contig_len > remaining + prefilled_bytes) {
			contig_len = remaining + prefilled_bytes;
			filemap_offset->cur_map_entry_offset += (contig_len >> SECTOR_SHIFT);
		} else {
			++filemap_offset->cur_map_entry;
			filemap_offset->cur_map_entry_offset = 0;
		}

		start_offset = start_offset - prefilled_bytes + ((contig_len >> context->fs_blocklog) << context->fs_blocklog);
		
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

			contig_len -= new_partial->length;  
			remaining -= new_partial->length;  

			new_partial->lba = lba + (contig_len >> SECTOR_SHIFT);
#ifdef DEBUG
			printk(KERN_INFO "creating new partial to handle later lba - %lld, length - %d, bytes_into_block - %d, offset into file - %lld\n",
								new_partial->lba, new_partial->length, new_partial->bytes_into_block, new_partial->start_offset_into_file);
#endif
		}

		remaining_contig_len = contig_len - prefilled_bytes;
		
		while (remaining_contig_len) {
			unsigned int frag_remaining_len;
			unsigned int len;
			
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

			prd->adr = virt_to_dma(0, page_address(frag->bio_vec.bv_page) +
				frag->bio_vec.bv_offset + frag_offset);
			prd->flags_len = len;
#else  
			if ((atomic_read(&context->cur_sg_status[context->list_idx]) == 0) || !sg) {
				BUG_ON(odrb_alloc_sg_list(&context->sg_list[context->list_idx], 1, 1));
				atomic_set(&context->cur_sg_status[context->list_idx], 1);

				sg = context->sg_list[context->list_idx]->sg_entries;
			} else {
				 
				odrb_sg_entry_t *prev_sg = sg;
				prev_sg->next_ = (dma_addr_t)descriptors_virt_to_phys(((u32)++sg));
			}

			sg->addr_ = virt_to_dma(0, page_address(frag->bio_vec.bv_page) +
				frag->bio_vec.bv_offset + frag_offset);
			sg->length_ = len;
#endif  

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
		
#ifdef CONFIG_ODRB_USE_PRDS
		if (prd) prd->flags_len |= PRD_EOF_MASK;
#else  
		if (sg) sg->next_ = 0;
#endif  

		wait_sata_complete(context);

		if (contig_len) {
			 
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
#endif  

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
#endif  
#else  
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
#endif  
#endif  

			file_context->prev_partial_write_loc = 0;  
		}

		remaining -= (contig_len - prefilled_bytes);

		if (file_context->partial_block) {
			switch(file_context->partial_block->status) {
				case PARTIAL_MEM_TO_WRITE:
					 
					file_context->partial_block->status = PARTIAL_MEM_ON_GOING;
					break;

				case PARTIAL_MEM_ON_GOING:
					 
				default:
					dma_free_coherent(0, sizeof(char) << context->fs_blocklog, file_context->partial_block->buffer, file_context->partial_block->buffer_pa);
					kfree(file_context->partial_block);
					file_context->partial_block = NULL;
					break;
			}
		}

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

#ifdef CONFIG_SATA_OX810
			direct_sata_transfer(C_WRITE_DMA_EXT, port, lba, contig_len >> SECTOR_SHIFT);
#elif defined(CONFIG_SATA_OX820_DIRECT)
			(*sata_context->execute_command)();
#endif  

			cur_extent_flag =
				oxnas_get_extent_status(context->map, filemap_offset->cur_map_entry);

			if (file_context->write_extent_flag != cur_extent_flag) {
				loff_t temp_end =
					((file_context->write_end_offset - remaining) >>
						context->fs_blocklog) << context->fs_blocklog;

				if (temp_end > file_context->write_start_offset) {
					 
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

	update_context_indices(net_rx_context);

	BUG_ON(remaining);
	
#ifdef DEBUG
	printk(KERN_INFO "fast write leaving - inode %p file - %p\n", fp->inode, fp);
#endif
	
	return length;
}

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
	int 						 hole_once = 1;  
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

file_map_read:
	 
	cur_offset = (offset >> disk_context->fs_blocklog)
					<< (disk_context->fs_blocklog - SECTOR_SHIFT);

	bytes_into_block = offset - (cur_offset << SECTOR_SHIFT);

	cancel_delayed_work(&file_context->write_completion_work);

	if (unlikely( (!inode->writer_filemap_info.map) || reread_filemap_now ) ) {
 
		write_result = alloc_filemap(inode, 1);
		if (unlikely(write_result)) {
			printk(KERN_WARNING "fast write - alloc_filemap() failed - fallback init -Inode %p, file %p Failed to allocate filemap, error %d\n", inode, fp, write_result);
			 
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			spin_unlock(&inode->fast_lock);

			up(&inode->writer_filemap_info.sem);
			writer_complete_fallback(inode);

			return OXNAS_FALLBACK;  
		}
		oxnas_clear_filemap_dirty(inode);
		reread_filemap_now = 0;
	}
	
	memset(&filemap_offset, 0, sizeof(oxnas_filemap_offset_t));

	disk_context->map = &inode->writer_filemap_info.map[1];
#ifdef CONFIG_SATA_OX810
	disk_context->port = inode->writer_filemap_info.port;
#endif  

	do {
		map_entry = &disk_context->map[filemap_offset.cur_map_entry];
 
		if (cur_offset >= map_entry->bmv_length) {
			filemap_offset.cur_map_entry ++;
			 
			if (preallocate_remap_once) {
				 
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
	
	if (hole_once) {
		 
		int    cur_write_start_entry = filemap_offset.cur_map_entry;
		loff_t cur_total_bytes = bytes_into_block + count;
		loff_t cur_total_sectors = filemap_offset.cur_map_entry_offset +
									(cur_total_bytes >> SECTOR_SHIFT) +
									((cur_total_bytes % SECTOR_SIZE) ? 1 : 0);

		do {
			map_entry = &disk_context->map[cur_write_start_entry];
			if (map_entry->bmv_block == GETBMAPX_BLOCK_HOLE) {
				if (hole_once) {
					 
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
 
				}
				map_read_flag = 1;
				hole_once = 0;
			}
			cur_total_sectors -= map_entry->bmv_length;
			cur_write_start_entry ++;

			if ((preallocate_remap_once) && (cur_total_sectors > 0)) {
				 
				if (cur_write_start_entry ==
						inode->writer_filemap_info.map->bmv_entries ) {
					preallocate_remap_once = 0;
					 
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
		 
		reread_filemap_now = 1;
		goto file_map_read;
	}
	 
	file_context->fp = fp;  

	if (file_context->write_end_offset == 0) {
		 
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
				 
				long long temp_end =
					(file_context->write_end_offset >> disk_context->fs_blocklog) <<
						disk_context->fs_blocklog;
#ifdef DEBUG
				printk (KERN_INFO "write loop - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
				 
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
			 
			long long temp_end =
				(file_context->write_end_offset >> disk_context->fs_blocklog) <<
					disk_context->fs_blocklog;
#ifdef DEBUG
			printk (KERN_INFO "write loop - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
			 
			if (temp_end > file_context->write_start_offset) {
				oxnas_reset_extent_preallocate_flag(fp,
					file_context->write_start_offset,
					temp_end - file_context->write_start_offset,
					file_context->write_extent_flag,
					file_context->disable_accumulation);
			}
			oxnas_set_filesize(fp, temp_end );

			if(oxnas_check_filemap_dirty(fp->inode)) {
				loff_t 	temp = (bytes_into_block + count) >> disk_context->fs_blocklog;
				temp = bytes_into_block + count - (temp << disk_context->fs_blocklog);
				 
				if (bytes_into_block || temp) {
					 
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
	 
	i_tent_size_write(inode, file_context->write_end_offset);
	
	disk_context->prealloc_write = 0;
	
	write_result = oxnas_direct_disk_write(file_context, disk_context,
											&filemap_offset, count,	size_written,
											bytes_into_block, fp);
											
	release_netdma_net_frags(net_rx_context);
	
 	if (disk_context->prealloc_write) {
 		oxnas_set_filemap_dirty(inode);
 		 
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
		 
		{
 			temp_end = (file_context->write_end_offset >> disk_context->fs_blocklog)
				<< disk_context->fs_blocklog;

prealloc_reset_now:
#ifdef DEBUG
			printk (KERN_INFO "end of write - reset prealloc /invalidate - start offset - %lld, end offset - %lld\n", file_context->write_start_offset, file_context->write_end_offset);
#endif
			 
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
 	
 	disk_context->prealloc_write = 0;
 	oxnas_direct_disk_complete_partial_write(file_context, disk_context);

 	if(disk_context->prealloc_write) {
 		 
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
		 
		 return OXNAS_FALLBACK;
	} else {
		 
		++inode->fast_writes_in_progress_count;	
	}
	spin_unlock(&inode->fast_lock);

	smp_rmb();
	
	if (!fp->fast_write_context) {  
#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX
		printk("Init fastwrite Inode %p[%lu], file %p, name %s\n", inode, inode->i_ino, fp, fp->f_path.dentry->d_name.name);
#else
		printk("Init fastwrite Inode %p, file %p, name %s\n", inode, fp, fp->f_path.dentry->d_name.name);
#endif
#endif
		read_result = init_write_file_context(inode, fp);
		if (unlikely(read_result)) {
			 
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			--inode->fast_writes_in_progress_count;
			spin_unlock(&inode->fast_lock);

			writer_complete_fallback(inode);

			return OXNAS_FALLBACK;  
		}
	}

#ifdef DEBUG
#ifdef SYNO_FAST_RW_FIX	
 	printk("fast - write Inode %p[%lu], file %p, name %s\n", inode, inode->i_ino, fp, fp->f_path.dentry->d_name.name);
#else
	printk("fast - write Inode %p, file %p, name %s\n", inode, fp, fp->f_path.dentry->d_name.name);
#endif
#endif

	while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
		printk("oxnas_do_direct_disk_write() A second has elapsed while waiting, inode %p\n", inode);
	}

	if (unlikely(!inode->writer_file_context)) {
		 
		read_result = do_initial_inode_prep(inode, fp);
		if (unlikely(read_result)) {
			 
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			spin_unlock(&inode->fast_lock);

			up(&inode->writer_filemap_info.sem);
			writer_complete_fallback(inode);

			return OXNAS_FALLBACK;  
		}
	}
	 
	file_context = (oxnas_file_context_t *)inode->writer_file_context;
	disk_context = (oxnas_direct_disk_context_t *) fp->fast_write_context;
	net_rx_context = &disk_context->net_rx_context;
	
	if( (file_context->acc_fp) && (file_context->acc_fp != fp) ) {
 
		if( complete_accumulated_write(file_context) < 0) {
			 
			printk(KERN_INFO "accumulated write of some other file pointer failed \n");
		}
		file_context->acc_fp = 0;
	}
	
read_more_data:
	write_result = read_result = 0;
	if (net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length == 0) {
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length = remaining_to_receive;
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset = offset;
 
	} else if(offset == net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length 
					+ net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].start_offset) { 
		
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length += count;
		 
	} else {  
		write_now = 1;
		loop_back_to_read_more = 1;
 
		goto do_actual_write;
	}
	
	desc.arg.data = net_rx_context;
	 
	desc.written = 0;
	desc.error = 0;

	loop_to_receive = remaining_to_receive;

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
		 
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length -= remaining_to_receive;
		write_now = 1;
	}
		
	if(write_result < 0) {
		 
		offset += received_from_net;
		net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length -= remaining_to_receive;
		loop_back_to_read_more = 1;
		write_now = 1;
	}
		
do_actual_write:
	if( file_context->disable_accumulation || write_now ||
			 ( net_rx_context->data_ref[net_rx_context->fill_frag_list_idx].length >= NET_SAMBA_RX_CHUNK_SIZE ) ){
			 
		file_context->acc_fp = fp;
		write_result = complete_accumulated_write(file_context);
							 
		write_now = 0;
	} else {  
		 
		read_result = count;
		write_result = count;  
		file_context->acc_fp = fp;  
	}
	
	if(loop_back_to_read_more && (read_result >= 0)) {
 
		loop_back_to_read_more = 0;
		goto read_more_data;
	}

	up(&inode->writer_filemap_info.sem);

	spin_lock(&inode->fast_lock);
	 
	--inode->fast_writes_in_progress_count;
	if (unlikely(inode->fallback_in_progress) &&
		!inode->fast_writes_in_progress_count &&
		!inode->fast_reads_in_progress_count) {
		 
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
#endif  

	kfree(file_context);
	inode->writer_file_context = NULL;
	smp_wmb();
}

void fast_write_check_and_free_filemap(struct inode * inode)
{
	 
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

	while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
		printk("__write_flush_filemap() A second has elapsed while waiting, inode %p\n", inode);
	}

	file = ((oxnas_file_context_t*)(inode->writer_file_context))->fp;

	WARN_ON(!file);

	if (file) {
		writer_reset_prealloc(file);
	}

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
							
		if(file_context->acc_fp) {
 
			while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
				printk("write_flush_pending() A second has elapsed while waiting, inode %p\n", inode);
			}

			if(file_context->acc_fp) {
#ifdef DEBUG
				printk(KERN_INFO "Writing from write flush pending\n");
#endif
				if( complete_accumulated_write(file_context) < 0) {
					 
					printk(KERN_INFO "FAST WRITE COMPLETE - accumulated write of failed \n");
				}
				file_context->acc_fp = 0;
			}
			
			up(&inode->writer_filemap_info.sem);
		}
		if(disable_accumulation)
			file_context->disable_accumulation = disable_accumulation;
		smp_wmb();
 
	}
}

void flush_writes(struct inode *inode)
{
	smp_rmb();
	if (inode->writer_file_context) {
		spin_lock(&inode->fast_lock);
		if (unlikely(inode->fallback_in_progress)) {
			wait_fallback_complete(inode);
			spin_unlock(&inode->fast_lock);
			 
			 return;
		} else {
			 
			++inode->fast_writes_in_progress_count;	
		}
		spin_unlock(&inode->fast_lock);
	
		write_flush_pending(inode, 0);
		write_flush_filemap(inode);
		
		spin_lock(&inode->fast_lock);
		 
		--inode->fast_writes_in_progress_count;
		if (unlikely(inode->fallback_in_progress) &&
			!inode->fast_writes_in_progress_count &&
			!inode->fast_reads_in_progress_count) {
			 
printk(KERN_INFO "Initiating fallback from flush writes\n");
			do_fast_fallback(inode);
		}
		spin_unlock(&inode->fast_lock);
	}
}

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
			 
			 return;
		} else {
			 
			++inode->fast_writes_in_progress_count;	
		}
		spin_unlock(&inode->fast_lock);

		while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
			printk("writer_remap_file() A second has elapsed while waiting, inode %p\n", inode);
		}

		result = alloc_filemap(inode, 1);
		if (unlikely(result)) {
			printk(KERN_WARNING "writer_remap_file - alloc_filemap() failed - fallback init -Inode %p, Failed to allocate filemap, error %d\n", inode, result);
			 
			spin_lock(&inode->fast_lock);
			inode->fallback_in_progress = 1;
			 
			--inode->fast_writes_in_progress_count;
			if( (!inode->fast_writes_in_progress_count) 
					&& (!inode->fast_reads_in_progress_count) ) {
				fallback = 1;
			}
			spin_unlock(&inode->fast_lock);

			up(&inode->writer_filemap_info.sem);

			if(fallback) {  
				writer_complete_fallback(inode);
			}

			return;  
		}
		oxnas_clear_filemap_dirty(inode);

		up(&inode->writer_filemap_info.sem);

		spin_lock(&inode->fast_lock);
		 
		--inode->fast_writes_in_progress_count;
		if (unlikely(inode->fallback_in_progress) &&
			!inode->fast_writes_in_progress_count &&
			!inode->fast_reads_in_progress_count) {
			 
	printk(KERN_INFO "Initiating fallback from writer_remap_file\n");
			do_fast_fallback(inode);
			fallback = 1;
		}
		spin_unlock(&inode->fast_lock);
	}
	
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
	
	while (down_timeout(&inode->writer_filemap_info.sem, HZ)) {
		printk("complete_fast_write() A second has elapsed while waiting, inode %p\n", inode);
	}

	file_context = (oxnas_file_context_t *)inode->writer_file_context;
	disk_context = (oxnas_direct_disk_context_t *)fp->fast_write_context;
 
	net_rx_context = & (disk_context->net_rx_context);

	if ((file_context->fp == NULL) || (file_context->fp == fp)) schedule_queue = 0;
	
	if(file_context->acc_fp == fp ) {
 
		retval = complete_accumulated_write(file_context);
		if(retval < 0) {
			printk(KERN_INFO "FAST WRITE COMPLETE - accumulated write failed \n");
		}
		file_context->acc_fp = 0;
	}
	
	cancel_delayed_work(&file_context->write_completion_work);
	flush_scheduled_work();
	
	oxnas_direct_disk_complete(disk_context);

	release_netdma_net_frags(&(disk_context->net_rx_context));

	release_netdma_net_frags_by_index(&disk_context->net_rx_context,
		disk_context->net_rx_context.fill_frag_list_idx);

	if (net_rx_context->frag_cache) {
		 
		kmem_cache_destroy(net_rx_context->frag_cache);
	}

	if (disk_context->buffer) {
		 
		dma_free_coherent(0, sizeof(char) << disk_context->fs_blocklog, disk_context->buffer, disk_context->buffer_pa);
		disk_context->buffer = NULL;
	}

	kfree(disk_context);

#ifdef DEBUG	
	printk(KERN_INFO "Fast Write - Close\n");
#endif

	if (schedule_queue) {
		schedule_delayed_work(&file_context->write_completion_work,
			msecs_to_jiffies(OXNAS_WRITER_TIMEOUT));
	}

	up(&inode->writer_filemap_info.sem);

	if (retval < 0)
		return retval;
	else
		return 0;
}
