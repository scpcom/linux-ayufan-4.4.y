#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/aio.h>
#include <linux/capability.h>
#include <linux/kernel_stat.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/hash.h>
#include <linux/writeback.h>
#include <linux/backing-dev.h>
#include <linux/pagevec.h>
#include <linux/blkdev.h>

#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_FSL_DMA_MEMCPY
#include <linux/rmap.h>
#endif
#endif

#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/cpuset.h>
#include <linux/hardirq.h>  
#include <linux/memcontrol.h>
#include <linux/mm_inline.h>  
#include "internal.h"

#include <linux/buffer_head.h>  

#include <asm/mman.h>

#ifdef MY_ABC_HERE
#include <linux/tcp.h>
#include <net/tcp.h>
#endif  

void __remove_from_page_cache(struct page *page)
{
	struct address_space *mapping = page->mapping;

	radix_tree_delete(&mapping->page_tree, page->index);
	page->mapping = NULL;
	mapping->nrpages--;
	__dec_zone_page_state(page, NR_FILE_PAGES);
	if (PageSwapBacked(page))
		__dec_zone_page_state(page, NR_SHMEM);
	BUG_ON(page_mapped(page));

	if (PageDirty(page) && mapping_cap_account_dirty(mapping)) {
		dec_zone_page_state(page, NR_FILE_DIRTY);
		dec_bdi_stat(mapping->backing_dev_info, BDI_RECLAIMABLE);
	}
}

void remove_from_page_cache(struct page *page)
{
	struct address_space *mapping = page->mapping;

	BUG_ON(!PageLocked(page));

	spin_lock_irq(&mapping->tree_lock);
	__remove_from_page_cache(page);
	spin_unlock_irq(&mapping->tree_lock);
	mem_cgroup_uncharge_cache_page(page);
}

static int sync_page(void *word)
{
	struct address_space *mapping;
	struct page *page;

	page = container_of((unsigned long *)word, struct page, flags);

	smp_mb();
	mapping = page_mapping(page);
	if (mapping && mapping->a_ops && mapping->a_ops->sync_page)
		mapping->a_ops->sync_page(page);
	io_schedule();
	return 0;
}

static int sync_page_killable(void *word)
{
	sync_page(word);
	return fatal_signal_pending(current) ? -EINTR : 0;
}

int __filemap_fdatawrite_range(struct address_space *mapping, loff_t start,
				loff_t end, int sync_mode)
{
	int ret;
	struct writeback_control wbc = {
		.sync_mode = sync_mode,
		.nr_to_write = LONG_MAX,
		.range_start = start,
		.range_end = end,
	};

	if (!mapping_cap_writeback_dirty(mapping))
		return 0;

	ret = do_writepages(mapping, &wbc);
	return ret;
}

static inline int __filemap_fdatawrite(struct address_space *mapping,
	int sync_mode)
{
	return __filemap_fdatawrite_range(mapping, 0, LLONG_MAX, sync_mode);
}

int filemap_fdatawrite(struct address_space *mapping)
{
	return __filemap_fdatawrite(mapping, WB_SYNC_ALL);
}
EXPORT_SYMBOL(filemap_fdatawrite);

int filemap_fdatawrite_range(struct address_space *mapping, loff_t start,
				loff_t end)
{
	return __filemap_fdatawrite_range(mapping, start, end, WB_SYNC_ALL);
}
EXPORT_SYMBOL(filemap_fdatawrite_range);

int filemap_flush(struct address_space *mapping)
{
	return __filemap_fdatawrite(mapping, WB_SYNC_NONE);
}
EXPORT_SYMBOL(filemap_flush);

int wait_on_page_writeback_range(struct address_space *mapping,
				pgoff_t start, pgoff_t end)
{
	struct pagevec pvec;
	int nr_pages;
	int ret = 0;
	pgoff_t index;

	if (end < start)
		return 0;

	pagevec_init(&pvec, 0);
	index = start;
	while ((index <= end) &&
			(nr_pages = pagevec_lookup_tag(&pvec, mapping, &index,
			PAGECACHE_TAG_WRITEBACK,
			min(end - index, (pgoff_t)PAGEVEC_SIZE-1) + 1)) != 0) {
		unsigned i;

		for (i = 0; i < nr_pages; i++) {
			struct page *page = pvec.pages[i];

			if (page->index > end)
				continue;

			wait_on_page_writeback(page);
			if (PageError(page))
				ret = -EIO;
		}
		pagevec_release(&pvec);
		cond_resched();
	}

	if (test_and_clear_bit(AS_ENOSPC, &mapping->flags))
		ret = -ENOSPC;
	if (test_and_clear_bit(AS_EIO, &mapping->flags))
		ret = -EIO;

	return ret;
}

int filemap_fdatawait_range(struct address_space *mapping, loff_t start,
			    loff_t end)
{
	return wait_on_page_writeback_range(mapping, start >> PAGE_CACHE_SHIFT,
					    end >> PAGE_CACHE_SHIFT);
}
EXPORT_SYMBOL(filemap_fdatawait_range);

int filemap_fdatawait(struct address_space *mapping)
{
	loff_t i_size = i_size_read(mapping->host);

	if (i_size == 0)
		return 0;

	return wait_on_page_writeback_range(mapping, 0,
				(i_size - 1) >> PAGE_CACHE_SHIFT);
}
EXPORT_SYMBOL(filemap_fdatawait);

int filemap_write_and_wait(struct address_space *mapping)
{
	int err = 0;

	if (mapping->nrpages) {
		err = filemap_fdatawrite(mapping);
		 
		if (err != -EIO) {
			int err2 = filemap_fdatawait(mapping);
			if (!err)
				err = err2;
		}
	}
	return err;
}
EXPORT_SYMBOL(filemap_write_and_wait);

int filemap_write_and_wait_range(struct address_space *mapping,
				 loff_t lstart, loff_t lend)
{
	int err = 0;

	if (mapping->nrpages) {
		err = __filemap_fdatawrite_range(mapping, lstart, lend,
						 WB_SYNC_ALL);
		 
		if (err != -EIO) {
			int err2 = wait_on_page_writeback_range(mapping,
						lstart >> PAGE_CACHE_SHIFT,
						lend >> PAGE_CACHE_SHIFT);
			if (!err)
				err = err2;
		}
	}
	return err;
}
EXPORT_SYMBOL(filemap_write_and_wait_range);

int add_to_page_cache_locked(struct page *page, struct address_space *mapping,
		pgoff_t offset, gfp_t gfp_mask)
{
	int error;

	VM_BUG_ON(!PageLocked(page));

	error = mem_cgroup_cache_charge(page, current->mm,
					gfp_mask & GFP_RECLAIM_MASK);
	if (error)
		goto out;

	error = radix_tree_preload(gfp_mask & ~__GFP_HIGHMEM);
	if (error == 0) {
		page_cache_get(page);
		page->mapping = mapping;
		page->index = offset;

		spin_lock_irq(&mapping->tree_lock);
		error = radix_tree_insert(&mapping->page_tree, offset, page);
		if (likely(!error)) {
			mapping->nrpages++;
			__inc_zone_page_state(page, NR_FILE_PAGES);
			if (PageSwapBacked(page))
				__inc_zone_page_state(page, NR_SHMEM);
			spin_unlock_irq(&mapping->tree_lock);
		} else {
			page->mapping = NULL;
			spin_unlock_irq(&mapping->tree_lock);
			mem_cgroup_uncharge_cache_page(page);
			page_cache_release(page);
		}
		radix_tree_preload_end();
	} else
		mem_cgroup_uncharge_cache_page(page);
out:
	return error;
}
EXPORT_SYMBOL(add_to_page_cache_locked);

int add_to_page_cache_lru(struct page *page, struct address_space *mapping,
				pgoff_t offset, gfp_t gfp_mask)
{
	int ret;

	if (mapping_cap_swap_backed(mapping))
		SetPageSwapBacked(page);

	ret = add_to_page_cache(page, mapping, offset, gfp_mask);
	if (ret == 0) {
		if (page_is_file_cache(page))
			lru_cache_add_file(page);
		else
			lru_cache_add_active_anon(page);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(add_to_page_cache_lru);

#ifdef CONFIG_NUMA
struct page *__page_cache_alloc(gfp_t gfp)
{
	if (cpuset_do_page_mem_spread()) {
		int n = cpuset_mem_spread_node();
		return alloc_pages_exact_node(n, gfp, 0);
	}
	return alloc_pages(gfp, 0);
}
EXPORT_SYMBOL(__page_cache_alloc);
#endif

static int __sleep_on_page_lock(void *word)
{
	io_schedule();
	return 0;
}

static wait_queue_head_t *page_waitqueue(struct page *page)
{
	const struct zone *zone = page_zone(page);

	return &zone->wait_table[hash_ptr(page, zone->wait_table_bits)];
}

static inline void wake_up_page(struct page *page, int bit)
{
	__wake_up_bit(page_waitqueue(page), &page->flags, bit);
}

void wait_on_page_bit(struct page *page, int bit_nr)
{
	DEFINE_WAIT_BIT(wait, &page->flags, bit_nr);

	if (test_bit(bit_nr, &page->flags))
		__wait_on_bit(page_waitqueue(page), &wait, sync_page,
							TASK_UNINTERRUPTIBLE);
}
EXPORT_SYMBOL(wait_on_page_bit);

void add_page_wait_queue(struct page *page, wait_queue_t *waiter)
{
	wait_queue_head_t *q = page_waitqueue(page);
	unsigned long flags;

	spin_lock_irqsave(&q->lock, flags);
	__add_wait_queue(q, waiter);
	spin_unlock_irqrestore(&q->lock, flags);
}
EXPORT_SYMBOL_GPL(add_page_wait_queue);

void unlock_page(struct page *page)
{
	VM_BUG_ON(!PageLocked(page));
	clear_bit_unlock(PG_locked, &page->flags);
	smp_mb__after_clear_bit();
	wake_up_page(page, PG_locked);
}
EXPORT_SYMBOL(unlock_page);

void end_page_writeback(struct page *page)
{
	if (TestClearPageReclaim(page))
		rotate_reclaimable_page(page);

	if (!test_clear_page_writeback(page))
		BUG();

	smp_mb__after_clear_bit();

#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_FSL_DMA_MEMCPY
	clear_page_constant(page);
#endif
#endif

	wake_up_page(page, PG_writeback);
}
EXPORT_SYMBOL(end_page_writeback);

#ifdef MY_DEF_HERE
#ifdef CONFIG_OPTIMIZE_FSL_DMA_MEMCPY
void clear_page_constant(struct page *page)
{
	if (PageConstant(page)) {
		ClearPageConstant(page);
		SetPageUptodate(page);
	}
}
EXPORT_SYMBOL(clear_page_constant);
#endif
#endif

void __lock_page(struct page *page)
{
	DEFINE_WAIT_BIT(wait, &page->flags, PG_locked);

	__wait_on_bit_lock(page_waitqueue(page), &wait, sync_page,
							TASK_UNINTERRUPTIBLE);
}
EXPORT_SYMBOL(__lock_page);

int __lock_page_killable(struct page *page)
{
	DEFINE_WAIT_BIT(wait, &page->flags, PG_locked);

	return __wait_on_bit_lock(page_waitqueue(page), &wait,
					sync_page_killable, TASK_KILLABLE);
}
EXPORT_SYMBOL_GPL(__lock_page_killable);

void __lock_page_nosync(struct page *page)
{
	DEFINE_WAIT_BIT(wait, &page->flags, PG_locked);
	__wait_on_bit_lock(page_waitqueue(page), &wait, __sleep_on_page_lock,
							TASK_UNINTERRUPTIBLE);
}

struct page *find_get_page(struct address_space *mapping, pgoff_t offset)
{
	void **pagep;
	struct page *page;

	rcu_read_lock();
repeat:
	page = NULL;
	pagep = radix_tree_lookup_slot(&mapping->page_tree, offset);
	if (pagep) {
		page = radix_tree_deref_slot(pagep);
		if (unlikely(!page || page == RADIX_TREE_RETRY))
			goto repeat;

		if (!page_cache_get_speculative(page))
			goto repeat;

		if (unlikely(page != *pagep)) {
			page_cache_release(page);
			goto repeat;
		}
	}
	rcu_read_unlock();

	return page;
}
EXPORT_SYMBOL(find_get_page);

struct page *find_lock_page(struct address_space *mapping, pgoff_t offset)
{
	struct page *page;

repeat:
	page = find_get_page(mapping, offset);
	if (page) {
		lock_page(page);
		 
		if (unlikely(page->mapping != mapping)) {
			unlock_page(page);
			page_cache_release(page);
			goto repeat;
		}
		VM_BUG_ON(page->index != offset);
	}
	return page;
}
EXPORT_SYMBOL(find_lock_page);

struct page *find_or_create_page(struct address_space *mapping,
		pgoff_t index, gfp_t gfp_mask)
{
	struct page *page;
	int err;
repeat:
	page = find_lock_page(mapping, index);
	if (!page) {
		page = __page_cache_alloc(gfp_mask);
		if (!page)
			return NULL;
		 
		err = add_to_page_cache_lru(page, mapping, index,
			(gfp_mask & GFP_RECLAIM_MASK));
		if (unlikely(err)) {
			page_cache_release(page);
			page = NULL;
			if (err == -EEXIST)
				goto repeat;
		}
	}
	return page;
}
EXPORT_SYMBOL(find_or_create_page);

unsigned find_get_pages(struct address_space *mapping, pgoff_t start,
			    unsigned int nr_pages, struct page **pages)
{
	unsigned int i;
	unsigned int ret;
	unsigned int nr_found;

	rcu_read_lock();
restart:
	nr_found = radix_tree_gang_lookup_slot(&mapping->page_tree,
				(void ***)pages, start, nr_pages);
	ret = 0;
	for (i = 0; i < nr_found; i++) {
		struct page *page;
repeat:
		page = radix_tree_deref_slot((void **)pages[i]);
		if (unlikely(!page))
			continue;
		 
		if (unlikely(page == RADIX_TREE_RETRY))
			goto restart;

		if (!page_cache_get_speculative(page))
			goto repeat;

		if (unlikely(page != *((void **)pages[i]))) {
			page_cache_release(page);
			goto repeat;
		}

		pages[ret] = page;
		ret++;
	}
	rcu_read_unlock();
	return ret;
}

unsigned find_get_pages_contig(struct address_space *mapping, pgoff_t index,
			       unsigned int nr_pages, struct page **pages)
{
	unsigned int i;
	unsigned int ret;
	unsigned int nr_found;

	rcu_read_lock();
restart:
	nr_found = radix_tree_gang_lookup_slot(&mapping->page_tree,
				(void ***)pages, index, nr_pages);
	ret = 0;
	for (i = 0; i < nr_found; i++) {
		struct page *page;
repeat:
		page = radix_tree_deref_slot((void **)pages[i]);
		if (unlikely(!page))
			continue;
		 
		if (unlikely(page == RADIX_TREE_RETRY))
			goto restart;

		if (page->mapping == NULL || page->index != index)
			break;

		if (!page_cache_get_speculative(page))
			goto repeat;

		if (unlikely(page != *((void **)pages[i]))) {
			page_cache_release(page);
			goto repeat;
		}

		pages[ret] = page;
		ret++;
		index++;
	}
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL(find_get_pages_contig);

unsigned find_get_pages_tag(struct address_space *mapping, pgoff_t *index,
			int tag, unsigned int nr_pages, struct page **pages)
{
	unsigned int i;
	unsigned int ret;
	unsigned int nr_found;

	rcu_read_lock();
restart:
	nr_found = radix_tree_gang_lookup_tag_slot(&mapping->page_tree,
				(void ***)pages, *index, nr_pages, tag);
	ret = 0;
	for (i = 0; i < nr_found; i++) {
		struct page *page;
repeat:
		page = radix_tree_deref_slot((void **)pages[i]);
		if (unlikely(!page))
			continue;
		 
		if (unlikely(page == RADIX_TREE_RETRY))
			goto restart;

		if (!page_cache_get_speculative(page))
			goto repeat;

		if (unlikely(page != *((void **)pages[i]))) {
			page_cache_release(page);
			goto repeat;
		}

		pages[ret] = page;
		ret++;
	}
	rcu_read_unlock();

	if (ret)
		*index = pages[ret - 1]->index + 1;

	return ret;
}
EXPORT_SYMBOL(find_get_pages_tag);

struct page *
grab_cache_page_nowait(struct address_space *mapping, pgoff_t index)
{
	struct page *page = find_get_page(mapping, index);

	if (page) {
		if (trylock_page(page))
			return page;
		page_cache_release(page);
		return NULL;
	}
	page = __page_cache_alloc(mapping_gfp_mask(mapping) & ~__GFP_FS);
	if (page && add_to_page_cache_lru(page, mapping, index, GFP_NOFS)) {
		page_cache_release(page);
		page = NULL;
	}
	return page;
}
EXPORT_SYMBOL(grab_cache_page_nowait);

static void shrink_readahead_size_eio(struct file *filp,
					struct file_ra_state *ra)
{
	ra->ra_pages /= 4;
}

static void do_generic_file_read(struct file *filp, loff_t *ppos,
		read_descriptor_t *desc, read_actor_t actor)
{
	struct address_space *mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	struct file_ra_state *ra = &filp->f_ra;
	pgoff_t index;
	pgoff_t last_index;
	pgoff_t prev_index;
	unsigned long offset;       
	unsigned int prev_offset;
	int error;

#ifdef CONFIG_SYNO_PLX_PORTING
     
#define MAX_QUEUED_PAGES (65536/PAGE_CACHE_SIZE)
	 
	struct page* page_table[MAX_QUEUED_PAGES];
	pgoff_t start_index;
	unsigned long loop_offset;
	unsigned long transfer_count;
	unsigned long start_desc_count;
	unsigned long index_count;
	unsigned long desc_remaining;     
#endif

	index = *ppos >> PAGE_CACHE_SHIFT;
	prev_index = ra->prev_pos >> PAGE_CACHE_SHIFT;
	prev_offset = ra->prev_pos & (PAGE_CACHE_SIZE-1);
	last_index = (*ppos + desc->count + PAGE_CACHE_SIZE-1) >> PAGE_CACHE_SHIFT;
	offset = *ppos & ~PAGE_CACHE_MASK;

#ifdef CONFIG_SYNO_PLX_PORTING
     
	start_index = index;
	index_count = 0;
	transfer_count = 0;
	desc_remaining = desc->count;
	loop_offset = offset;
#endif

	for (;;) {
		struct page *page;
		pgoff_t end_index;
		loff_t isize;
		unsigned long nr, ret;

		cond_resched();
find_page:
		page = find_get_page(mapping, index);
		if (!page) {
			page_cache_sync_readahead(mapping,
					ra, filp,
					index, last_index - index);
			page = find_get_page(mapping, index);
			if (unlikely(page == NULL))
				goto no_cached_page;
		}
		if (PageReadahead(page)) {
			page_cache_async_readahead(mapping,
					ra, filp, page,
					index, last_index - index);
		}
		if (!PageUptodate(page)) {
			if (inode->i_blkbits == PAGE_CACHE_SHIFT ||
					!mapping->a_ops->is_partially_uptodate)
				goto page_not_up_to_date;
			if (!trylock_page(page))
				goto page_not_up_to_date;
			if (!mapping->a_ops->is_partially_uptodate(page,
								desc, offset))
				goto page_not_up_to_date_locked;
			unlock_page(page);
		}
page_ok:
		 
		isize = i_size_read(inode);
		end_index = (isize - 1) >> PAGE_CACHE_SHIFT;
		if (unlikely(!isize || index > end_index)) {
			page_cache_release(page);
			goto out;
		}

		nr = PAGE_CACHE_SIZE;
		if (index == end_index) {
			nr = ((isize - 1) & ~PAGE_CACHE_MASK) + 1;
#ifdef CONFIG_SYNO_PLX_PORTING
			if (nr <= loop_offset) {
#else
			if (nr <= offset) {
#endif
				page_cache_release(page);
				goto out;
			}
		}
#ifdef CONFIG_SYNO_PLX_PORTING
		nr = nr - loop_offset;
#else
		nr = nr - offset;
#endif

		if (mapping_writably_mapped(mapping))
			flush_dcache_page(page);

#ifdef CONFIG_SYNO_PLX_PORTING
		if (prev_index != index || loop_offset != prev_offset)
#else
		if (prev_index != index || offset != prev_offset)
#endif
			mark_page_accessed(page);
		prev_index = index;

#ifndef CONFIG_SYNO_PLX_PORTING
		ret = actor(desc, page, offset, nr);
		offset += ret;
		index += offset >> PAGE_CACHE_SHIFT;
		offset &= ~PAGE_CACHE_MASK;
		prev_offset = offset;

		page_cache_release(page);
		if (ret == nr && desc->count)
			continue;
#else
        page_table[index_count] = page;

        index_count++;

        transfer_count += nr;

        if (transfer_count >= desc->count) {            
            loop_offset += desc_remaining;
            index += loop_offset >> PAGE_CACHE_SHIFT;            
            loop_offset &= ~PAGE_CACHE_MASK;
            desc_remaining = 0;
        } else {            
            loop_offset = 0;
            index++;
            desc_remaining -= nr;
        }

        prev_offset = loop_offset;
         
        if ((transfer_count < desc->count) && (index <= end_index) && (index_count < MAX_QUEUED_PAGES)) {
            continue;
        }
        
    start_desc_count = desc->count;

    ret = actor(desc, page_table, offset, transfer_count);

		offset += ret;
    index = start_index + (offset >> PAGE_CACHE_SHIFT);

		offset &= ~PAGE_CACHE_MASK;
     
    while (index_count) {
        index_count--;
        page_cache_release(page_table[index_count]);
    }

    if (ret == transfer_count && desc->count) {
         
        index_count = 0;
        start_index = index;
        transfer_count = 0;
        loop_offset = offset;
        desc_remaining = desc->count;
			continue;
    }
#endif
		goto out;

page_not_up_to_date:
		 
		error = lock_page_killable(page);
		if (unlikely(error))
			goto readpage_error;

page_not_up_to_date_locked:
		 
		if (!page->mapping) {
			unlock_page(page);
			page_cache_release(page);
			continue;
		}

		if (PageUptodate(page)) {
			unlock_page(page);
			goto page_ok;
		}

readpage:
		 
		error = mapping->a_ops->readpage(filp, page);

		if (unlikely(error)) {
			if (error == AOP_TRUNCATED_PAGE) {
				page_cache_release(page);
				goto find_page;
			}
			goto readpage_error;
		}

		if (!PageUptodate(page)) {
			error = lock_page_killable(page);
			if (unlikely(error))
				goto readpage_error;
			if (!PageUptodate(page)) {
				if (page->mapping == NULL) {
					 
					unlock_page(page);
					page_cache_release(page);
					goto find_page;
				}
				unlock_page(page);
				shrink_readahead_size_eio(filp, ra);
				error = -EIO;
				goto readpage_error;
			}
			unlock_page(page);
		}

		goto page_ok;

readpage_error:
		 
		desc->error = error;
		page_cache_release(page);
		goto out;

no_cached_page:
		 
		page = page_cache_alloc_cold(mapping);
		if (!page) {
			desc->error = -ENOMEM;
			goto out;
		}
		error = add_to_page_cache_lru(page, mapping,
						index, GFP_KERNEL);
		if (error) {
			page_cache_release(page);
			if (error == -EEXIST)
				goto find_page;
			desc->error = error;
			goto out;
		}
		goto readpage;
	}

out:
	ra->prev_pos = prev_index;
	ra->prev_pos <<= PAGE_CACHE_SHIFT;
	ra->prev_pos |= prev_offset;

	*ppos = ((loff_t)index << PAGE_CACHE_SHIFT) + offset;
	file_accessed(filp);
}

#ifdef CONFIG_SYNO_PLX_PORTING
int file_read_actor(read_descriptor_t *desc, struct page **page,
			unsigned long offset, unsigned long size)
#else
int file_read_actor(read_descriptor_t *desc, struct page *page,
			unsigned long offset, unsigned long size)
#endif
{
	char *kaddr;
	unsigned long left, count = desc->count;
#ifdef CONFIG_SYNO_PLX_PORTING
    unsigned char* dst;
    unsigned long ret_size;
#endif

	if (size > count)
		size = count;

#ifdef CONFIG_SYNO_PLX_PORTING
    ret_size = size;
    dst = desc->arg.buf;
#endif

#ifndef CONFIG_SYNO_PLX_PORTING
	if (!fault_in_pages_writeable(desc->arg.buf, size)) {
		kaddr = kmap_atomic(page, KM_USER0);
		left = __copy_to_user_inatomic(desc->arg.buf,
						kaddr + offset, size);
		kunmap_atomic(kaddr, KM_USER0);
		if (left == 0)
			goto success;
	}

	kaddr = kmap(page);
	left = __copy_to_user(desc->arg.buf, kaddr + offset, size);
	kunmap(page);

	if (left) {
		size -= left;
		desc->error = -EFAULT;
	}
success:
	desc->count = count - size;
	desc->written += size;
	desc->arg.buf += size;
	return size;
#else
    while (size) {            
        unsigned long psize = PAGE_CACHE_SIZE - offset;
        struct page *page_it;

        psize = PAGE_CACHE_SIZE - offset;
        if (size <= psize)
            psize = size;

        if (fault_in_pages_writeable(dst, psize))
            break;

        page_it = *page;

        kaddr = kmap_atomic(page_it, KM_USER0);
        left = __copy_to_user_inatomic(dst, kaddr + offset, psize);
        kunmap_atomic(kaddr, KM_USE R0);
        if (left != 0)
            break;

        size -= psize;
        page++;
        offset += psize;
        dst += psize;
        offset &= (PAGE_CACHE_SIZE - 1);
	}

    if (size == 0)
        goto success;

    while (size) {
        unsigned long psize;
        struct page *page_it;

        psize = PAGE_CACHE_SIZE - offset;
        if (size <= psize)
            psize = size;

        page_it = *page;

        kaddr = kmap(page_it);

        left = __copy_to_user(dst, kaddr + offset, psize);
        kunmap(page_it);

	if (left) {
		size -= left;
		desc->error = -EFAULT;
            break;
        }

        page++;
        offset += psize;
        dst += psize;            
        offset &= (PAGE_CACHE_SIZE - 1);
        size -= psize;
	}

success:
	desc->count = count - ret_size;
	desc->written += ret_size;
	desc->arg.buf += ret_size;
	return ret_size;
#endif
}

int generic_segment_checks(const struct iovec *iov,
			unsigned long *nr_segs, size_t *count, int access_flags)
{
	unsigned long   seg;
	size_t cnt = 0;
	for (seg = 0; seg < *nr_segs; seg++) {
		const struct iovec *iv = &iov[seg];

		cnt += iv->iov_len;
		if (unlikely((ssize_t)(cnt|iv->iov_len) < 0))
			return -EINVAL;
		if (access_ok(access_flags, iv->iov_base, iv->iov_len))
			continue;
		if (seg == 0)
			return -EFAULT;
		*nr_segs = seg;
		cnt -= iv->iov_len;	 
		break;
	}
	*count = cnt;
	return 0;
}
EXPORT_SYMBOL(generic_segment_checks);

ssize_t
generic_file_aio_read(struct kiocb *iocb, const struct iovec *iov,
		unsigned long nr_segs, loff_t pos)
{
	struct file *filp = iocb->ki_filp;
	ssize_t retval;
	unsigned long seg;
	size_t count;
	loff_t *ppos = &iocb->ki_pos;

#ifdef SYNO_FORCE_UNMOUNT
	if (!blSynostate(O_UNMOUNT_OK, filp)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("%s: force unmount hit\n", __FUNCTION__);
#endif
		return -EIO;
	}
#endif
	count = 0;
	retval = generic_segment_checks(iov, &nr_segs, &count, VERIFY_WRITE);
	if (retval)
		return retval;

	if (filp->f_flags & O_DIRECT) {
		loff_t size;
		struct address_space *mapping;
		struct inode *inode;

		mapping = filp->f_mapping;
		inode = mapping->host;
		if (!count)
			goto out;  
		size = i_size_read(inode);
		if (pos < size) {
			retval = filemap_write_and_wait_range(mapping, pos,
					pos + iov_length(iov, nr_segs) - 1);
			if (!retval) {
				retval = mapping->a_ops->direct_IO(READ, iocb,
							iov, pos, nr_segs);
			}
			if (retval > 0)
				*ppos = pos + retval;
			if (retval) {
				file_accessed(filp);
				goto out;
			}
		}
	}

	for (seg = 0; seg < nr_segs; seg++) {
		read_descriptor_t desc;

		desc.written = 0;
		desc.arg.buf = iov[seg].iov_base;
		desc.count = iov[seg].iov_len;
		if (desc.count == 0)
			continue;
		desc.error = 0;
		do_generic_file_read(filp, ppos, &desc, file_read_actor);
		retval += desc.written;
		if (desc.error) {
			retval = retval ?: desc.error;
			break;
		}
		if (desc.count > 0)
			break;
	}
out:
	return retval;
}
EXPORT_SYMBOL(generic_file_aio_read);

#ifdef CONFIG_SYNO_PLX_PORTING
int file_send_actor(read_descriptor_t * desc, struct page **page, unsigned long offset, unsigned long size)
{
    ssize_t written;
	unsigned long count = desc->count;
	struct file *file = desc->arg.data;

	if (size > count)
		size = count;

	written = file->f_op->sendpages(file, page, offset,
				       size, &file->f_pos, size<count);
	if (written < 0) {
		desc->error = written;
		written = 0;
	}
	desc->count = count - written;
	desc->written += written;
	return written;
}

ssize_t generic_file_sendfile(struct file *in_file, loff_t *ppos,
			 size_t count, read_actor_t actor, void *target)
{
	read_descriptor_t desc;

	if (!count)
		return 0;

	desc.written = 0;
	desc.count = count;
	desc.arg.data = target;
	desc.error = 0;

	do_generic_file_read(in_file, ppos, &desc, actor);
	if (desc.written)
		return desc.written;
	return desc.error;
}
EXPORT_SYMBOL(generic_file_sendfile);
#endif

static ssize_t
do_readahead(struct address_space *mapping, struct file *filp,
	     pgoff_t index, unsigned long nr)
{
	if (!mapping || !mapping->a_ops || !mapping->a_ops->readpage)
		return -EINVAL;

	force_page_cache_readahead(mapping, filp, index, nr);
	return 0;
}

SYSCALL_DEFINE(readahead)(int fd, loff_t offset, size_t count)
{
	ssize_t ret;
	struct file *file;

	ret = -EBADF;
	file = fget(fd);
	if (file) {
		if (file->f_mode & FMODE_READ) {
			struct address_space *mapping = file->f_mapping;
			pgoff_t start = offset >> PAGE_CACHE_SHIFT;
			pgoff_t end = (offset + count - 1) >> PAGE_CACHE_SHIFT;
			unsigned long len = end - start + 1;
			ret = do_readahead(mapping, file, start, len);
		}
		fput(file);
	}
	return ret;
}
#ifdef CONFIG_HAVE_SYSCALL_WRAPPERS
asmlinkage long SyS_readahead(long fd, loff_t offset, long count)
{
	return SYSC_readahead((int) fd, offset, (size_t) count);
}
SYSCALL_ALIAS(sys_readahead, SyS_readahead);
#endif

#ifdef CONFIG_MMU
 
static int page_cache_read(struct file *file, pgoff_t offset)
{
	struct address_space *mapping = file->f_mapping;
	struct page *page; 
	int ret;

	do {
		page = page_cache_alloc_cold(mapping);
		if (!page)
			return -ENOMEM;

		ret = add_to_page_cache_lru(page, mapping, offset, GFP_KERNEL);
		if (ret == 0)
			ret = mapping->a_ops->readpage(file, page);
		else if (ret == -EEXIST)
			ret = 0;  

		page_cache_release(page);

	} while (ret == AOP_TRUNCATED_PAGE);
		
	return ret;
}

#define MMAP_LOTSAMISS  (100)

static void do_sync_mmap_readahead(struct vm_area_struct *vma,
				   struct file_ra_state *ra,
				   struct file *file,
				   pgoff_t offset)
{
	unsigned long ra_pages;
	struct address_space *mapping = file->f_mapping;

	if (VM_RandomReadHint(vma))
		return;

	if (VM_SequentialReadHint(vma) ||
			offset - 1 == (ra->prev_pos >> PAGE_CACHE_SHIFT)) {
		page_cache_sync_readahead(mapping, ra, file, offset,
					  ra->ra_pages);
		return;
	}

	if (ra->mmap_miss < INT_MAX)
		ra->mmap_miss++;

	if (ra->mmap_miss > MMAP_LOTSAMISS)
		return;

	ra_pages = max_sane_readahead(ra->ra_pages);
	if (ra_pages) {
		ra->start = max_t(long, 0, offset - ra_pages/2);
		ra->size = ra_pages;
		ra->async_size = 0;
		ra_submit(ra, mapping, file);
	}
}

static void do_async_mmap_readahead(struct vm_area_struct *vma,
				    struct file_ra_state *ra,
				    struct file *file,
				    struct page *page,
				    pgoff_t offset)
{
	struct address_space *mapping = file->f_mapping;

	if (VM_RandomReadHint(vma))
		return;
	if (ra->mmap_miss > 0)
		ra->mmap_miss--;
	if (PageReadahead(page))
		page_cache_async_readahead(mapping, ra, file,
					   page, offset, ra->ra_pages);
}

int filemap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	int error;
	struct file *file = vma->vm_file;
	struct address_space *mapping = file->f_mapping;
	struct file_ra_state *ra = &file->f_ra;
	struct inode *inode = mapping->host;
	pgoff_t offset = vmf->pgoff;
	struct page *page;
	pgoff_t size;
	int ret = 0;

#ifdef SYNO_FORCE_UNMOUNT
	if (!blSynostate(O_UNMOUNT_OK, file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("%s: force unmount hit\n", __FUNCTION__);
#endif
		return -EFAULT;
	}
#endif
	size = (i_size_read(inode) + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	if (offset >= size)
		return VM_FAULT_SIGBUS;

	page = find_get_page(mapping, offset);
	if (likely(page)) {
		 
		do_async_mmap_readahead(vma, ra, file, page, offset);
		lock_page(page);

		if (unlikely(page->mapping != mapping)) {
			unlock_page(page);
			put_page(page);
			goto no_cached_page;
		}
	} else {
		 
		do_sync_mmap_readahead(vma, ra, file, offset);
		count_vm_event(PGMAJFAULT);
		ret = VM_FAULT_MAJOR;
retry_find:
		page = find_lock_page(mapping, offset);
		if (!page)
			goto no_cached_page;
	}

	if (unlikely(!PageUptodate(page)))
		goto page_not_uptodate;

	size = (i_size_read(inode) + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	if (unlikely(offset >= size)) {
		unlock_page(page);
		page_cache_release(page);
		return VM_FAULT_SIGBUS;
	}

	ra->prev_pos = (loff_t)offset << PAGE_CACHE_SHIFT;
	vmf->page = page;
	return ret | VM_FAULT_LOCKED;

no_cached_page:
	 
	error = page_cache_read(file, offset);

	if (error >= 0)
		goto retry_find;

	if (error == -ENOMEM)
		return VM_FAULT_OOM;
	return VM_FAULT_SIGBUS;

page_not_uptodate:
	 
	ClearPageError(page);
	error = mapping->a_ops->readpage(file, page);
	if (!error) {
		wait_on_page_locked(page);
		if (!PageUptodate(page))
			error = -EIO;
	}
	page_cache_release(page);

	if (!error || error == AOP_TRUNCATED_PAGE)
		goto retry_find;

	shrink_readahead_size_eio(file, ra);
	return VM_FAULT_SIGBUS;
}
EXPORT_SYMBOL(filemap_fault);

const struct vm_operations_struct generic_file_vm_ops = {
	.fault		= filemap_fault,
};

int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
	struct address_space *mapping = file->f_mapping;

	if (!mapping->a_ops->readpage)
		return -ENOEXEC;
	file_accessed(file);
	vma->vm_ops = &generic_file_vm_ops;
	vma->vm_flags |= VM_CAN_NONLINEAR;
	return 0;
}

int generic_file_readonly_mmap(struct file *file, struct vm_area_struct *vma)
{
	if ((vma->vm_flags & VM_SHARED) && (vma->vm_flags & VM_MAYWRITE))
		return -EINVAL;
	return generic_file_mmap(file, vma);
}
#else
int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
	return -ENOSYS;
}
int generic_file_readonly_mmap(struct file * file, struct vm_area_struct * vma)
{
	return -ENOSYS;
}
#endif  

EXPORT_SYMBOL(generic_file_mmap);
EXPORT_SYMBOL(generic_file_readonly_mmap);

static struct page *__read_cache_page(struct address_space *mapping,
				pgoff_t index,
				int (*filler)(void *,struct page*),
				void *data,
				gfp_t gfp)
{
	struct page *page;
	int err;
repeat:
	page = find_get_page(mapping, index);
	if (!page) {
		page = __page_cache_alloc(gfp | __GFP_COLD);
		if (!page)
			return ERR_PTR(-ENOMEM);
		err = add_to_page_cache_lru(page, mapping, index, GFP_KERNEL);
		if (unlikely(err)) {
			page_cache_release(page);
			if (err == -EEXIST)
				goto repeat;
			 
			return ERR_PTR(err);
		}
		err = filler(data, page);
		if (err < 0) {
			page_cache_release(page);
			page = ERR_PTR(err);
		}
	}
	return page;
}

static struct page *do_read_cache_page(struct address_space *mapping,
				pgoff_t index,
				int (*filler)(void *,struct page*),
				void *data,
				gfp_t gfp)

{
	struct page *page;
	int err;

retry:
	page = __read_cache_page(mapping, index, filler, data, gfp);
	if (IS_ERR(page))
		return page;
	if (PageUptodate(page))
		goto out;

	lock_page(page);
	if (!page->mapping) {
		unlock_page(page);
		page_cache_release(page);
		goto retry;
	}
	if (PageUptodate(page)) {
		unlock_page(page);
		goto out;
	}
	err = filler(data, page);
	if (err < 0) {
		page_cache_release(page);
		return ERR_PTR(err);
	}
out:
	mark_page_accessed(page);
	return page;
}

struct page *read_cache_page_async(struct address_space *mapping,
				pgoff_t index,
				int (*filler)(void *,struct page*),
				void *data)
{
	return do_read_cache_page(mapping, index, filler, data, mapping_gfp_mask(mapping));
}
EXPORT_SYMBOL(read_cache_page_async);

static struct page *wait_on_page_read(struct page *page)
{
	if (!IS_ERR(page)) {
		wait_on_page_locked(page);
		if (!PageUptodate(page)) {
			page_cache_release(page);
			page = ERR_PTR(-EIO);
		}
	}
	return page;
}

struct page *read_cache_page_gfp(struct address_space *mapping,
				pgoff_t index,
				gfp_t gfp)
{
	filler_t *filler = (filler_t *)mapping->a_ops->readpage;

	return wait_on_page_read(do_read_cache_page(mapping, index, filler, NULL, gfp));
}
EXPORT_SYMBOL(read_cache_page_gfp);

struct page *read_cache_page(struct address_space *mapping,
				pgoff_t index,
				int (*filler)(void *,struct page*),
				void *data)
{
	return wait_on_page_read(read_cache_page_async(mapping, index, filler, data));
}
EXPORT_SYMBOL(read_cache_page);

int should_remove_suid(struct dentry *dentry)
{
	mode_t mode = dentry->d_inode->i_mode;
	int kill = 0;

	if (unlikely(mode & S_ISUID))
		kill = ATTR_KILL_SUID;

	if (unlikely((mode & S_ISGID) && (mode & S_IXGRP)))
		kill |= ATTR_KILL_SGID;

	if (unlikely(kill && !capable(CAP_FSETID) && S_ISREG(mode)))
		return kill;

	return 0;
}
EXPORT_SYMBOL(should_remove_suid);

static int __remove_suid(struct dentry *dentry, int kill)
{
	struct iattr newattrs;

	newattrs.ia_valid = ATTR_FORCE | kill;
	return notify_change(dentry, &newattrs);
}

int file_remove_suid(struct file *file)
{
	struct dentry *dentry = file->f_path.dentry;
	int killsuid = should_remove_suid(dentry);
	int killpriv = security_inode_need_killpriv(dentry);
	int error = 0;

	if (killpriv < 0)
		return killpriv;
	if (killpriv)
		error = security_inode_killpriv(dentry);
	if (!error && killsuid)
		error = __remove_suid(dentry, killsuid);

	return error;
}
EXPORT_SYMBOL(file_remove_suid);

static size_t __iovec_copy_from_user_inatomic(char *vaddr,
			const struct iovec *iov, size_t base, size_t bytes)
{
	size_t copied = 0, left = 0;

	while (bytes) {
		char __user *buf = iov->iov_base + base;
		int copy = min(bytes, iov->iov_len - base);

		base = 0;
		left = __copy_from_user_inatomic(vaddr, buf, copy);
		copied += copy;
		bytes -= copy;
		vaddr += copy;
		iov++;

		if (unlikely(left))
			break;
	}
	return copied - left;
}

size_t iov_iter_copy_from_user_atomic(struct page *page,
		struct iov_iter *i, unsigned long offset, size_t bytes)
{
	char *kaddr;
	size_t copied;

	BUG_ON(!in_atomic());
	kaddr = kmap_atomic(page, KM_USER0);
	if (likely(i->nr_segs == 1)) {
		int left;
		char __user *buf = i->iov->iov_base + i->iov_offset;
		left = __copy_from_user_inatomic(kaddr + offset, buf, bytes);
		copied = bytes - left;
	} else {
		copied = __iovec_copy_from_user_inatomic(kaddr + offset,
						i->iov, i->iov_offset, bytes);
	}
	kunmap_atomic(kaddr, KM_USER0);

	return copied;
}
EXPORT_SYMBOL(iov_iter_copy_from_user_atomic);

size_t iov_iter_copy_from_user(struct page *page,
		struct iov_iter *i, unsigned long offset, size_t bytes)
{
	char *kaddr;
	size_t copied;

	kaddr = kmap(page);
	if (likely(i->nr_segs == 1)) {
		int left;
		char __user *buf = i->iov->iov_base + i->iov_offset;
		left = __copy_from_user(kaddr + offset, buf, bytes);
		copied = bytes - left;
	} else {
		copied = __iovec_copy_from_user_inatomic(kaddr + offset,
						i->iov, i->iov_offset, bytes);
	}
	kunmap(page);
	return copied;
}
EXPORT_SYMBOL(iov_iter_copy_from_user);

void iov_iter_advance(struct iov_iter *i, size_t bytes)
{
	BUG_ON(i->count < bytes);

	if (likely(i->nr_segs == 1)) {
		i->iov_offset += bytes;
		i->count -= bytes;
	} else {
		const struct iovec *iov = i->iov;
		size_t base = i->iov_offset;

		while (bytes || unlikely(i->count && !iov->iov_len)) {
			int copy;

			copy = min(bytes, iov->iov_len - base);
			BUG_ON(!i->count || i->count < copy);
			i->count -= copy;
			bytes -= copy;
			base += copy;
			if (iov->iov_len == base) {
				iov++;
				base = 0;
			}
		}
		i->iov = iov;
		i->iov_offset = base;
	}
}
EXPORT_SYMBOL(iov_iter_advance);

int iov_iter_fault_in_readable(struct iov_iter *i, size_t bytes)
{
	char __user *buf = i->iov->iov_base + i->iov_offset;
	bytes = min(bytes, i->iov->iov_len - i->iov_offset);
	return fault_in_pages_readable(buf, bytes);
}
EXPORT_SYMBOL(iov_iter_fault_in_readable);

size_t iov_iter_single_seg_count(struct iov_iter *i)
{
	const struct iovec *iov = i->iov;
	if (i->nr_segs == 1)
		return i->count;
	else
		return min(i->count, iov->iov_len - i->iov_offset);
}
EXPORT_SYMBOL(iov_iter_single_seg_count);

inline int generic_write_checks(struct file *file, loff_t *pos, size_t *count, int isblk)
{
	struct inode *inode = file->f_mapping->host;
	unsigned long limit = current->signal->rlim[RLIMIT_FSIZE].rlim_cur;

        if (unlikely(*pos < 0))
                return -EINVAL;

#ifdef SYNO_FORCE_UNMOUNT
	if (!blSynostate(O_UNMOUNT_OK, file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("%s: force unmount hit\n", __FUNCTION__);
#endif
		return -EIO;
	}
#endif
	if (!isblk) {
		 
		if (file->f_flags & O_APPEND)
                        *pos = i_size_read(inode);

		if (limit != RLIM_INFINITY) {
			if (*pos >= limit) {
				send_sig(SIGXFSZ, current, 0);
				return -EFBIG;
			}
			if (*count > limit - (typeof(limit))*pos) {
				*count = limit - (typeof(limit))*pos;
			}
		}
	}

	if (unlikely(*pos + *count > MAX_NON_LFS &&
				!(file->f_flags & O_LARGEFILE))) {
		if (*pos >= MAX_NON_LFS) {
			return -EFBIG;
		}
		if (*count > MAX_NON_LFS - (unsigned long)*pos) {
			*count = MAX_NON_LFS - (unsigned long)*pos;
		}
	}

	if (likely(!isblk)) {
		if (unlikely(*pos >= inode->i_sb->s_maxbytes)) {
			if (*count || *pos > inode->i_sb->s_maxbytes) {
				return -EFBIG;
			}
			 
		}

		if (unlikely(*pos + *count > inode->i_sb->s_maxbytes))
			*count = inode->i_sb->s_maxbytes - *pos;
	} else {
#ifdef CONFIG_BLOCK
		loff_t isize;
		if (bdev_read_only(I_BDEV(inode)))
			return -EPERM;
		isize = i_size_read(inode);
		if (*pos >= isize) {
			if (*count || *pos > isize)
				return -ENOSPC;
		}

		if (*pos + *count > isize)
			*count = isize - *pos;
#else
		return -EPERM;
#endif
	}
	return 0;
}
EXPORT_SYMBOL(generic_write_checks);

int pagecache_write_begin(struct file *file, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned flags,
				struct page **pagep, void **fsdata)
{
	const struct address_space_operations *aops = mapping->a_ops;

	return aops->write_begin(file, mapping, pos, len, flags,
							pagep, fsdata);
}
EXPORT_SYMBOL(pagecache_write_begin);

int pagecache_write_end(struct file *file, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned copied,
				struct page *page, void *fsdata)
{
	const struct address_space_operations *aops = mapping->a_ops;

	mark_page_accessed(page);
	return aops->write_end(file, mapping, pos, len, copied, page, fsdata);
}
EXPORT_SYMBOL(pagecache_write_end);

#ifdef MY_ABC_HERE
#ifdef SYNO_OLD_RECVFILE
int
do_recvfile(struct file *file, struct socket *sock, loff_t * ppos,
			size_t count, size_t * rbytes, size_t * wbytes)
{
	struct address_space *mapping = file->f_mapping;
	struct inode   *inode = mapping->host;
	loff_t          pos;
	struct page    *page;
	long            status = 0;
	ssize_t         err = 0;
	unsigned        bytes;

	unsigned        bytes_received = 0;
	struct kvec     iov[MAX_PAGES_PER_RECVFILE + 1];
	struct page    *rgPageList[MAX_PAGES_PER_RECVFILE + 1];

	int         rgblPageNeedCommit[MAX_PAGES_PER_RECVFILE+1];
	loff_t      pos2;
	int         count2;
	int             cPagesAllocated;

	unsigned long rgOffset[MAX_PAGES_PER_RECVFILE+1];
	unsigned rgBytes[MAX_PAGES_PER_RECVFILE+1];
	int             blTruncate = 0;

	long            rcvtimeo;
	struct msghdr   msg;
	int             ret;
	size_t          cBytesToReceive = 0;
	int             crgPagePtr = 0;

	*rbytes = 0;
	*wbytes = 0;
	pos = *ppos;

	vfs_check_frozen(inode->i_sb, SB_FREEZE_WRITE);

	current->backing_dev_info = mapping->backing_dev_info;

	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (err != 0 || count == 0) {
		goto done1;
	}

	file_remove_suid(file);
	file_update_time(file);

	cPagesAllocated = 0;
	pos2 = pos;
	count2 = count;

	do {
		pgoff_t index, offset;

		offset = (pos2 & (PAGE_CACHE_SIZE -1));  
		index = pos2 >> PAGE_CACHE_SHIFT;
		bytes = PAGE_CACHE_SIZE - offset;
		if (bytes > count2)
			bytes = count2;

		page = grab_cache_page_write_begin(mapping, index,  mapping_gfp_mask(mapping));
		if (!page) {
			err = -ENOMEM;
			crgPagePtr = 0;
			while( crgPagePtr < cPagesAllocated ) {
				unlock_page(rgPageList[crgPagePtr]);
				mark_page_accessed(page);
				page_cache_release(rgPageList[crgPagePtr]);
				crgPagePtr++;
			}
			goto done1;
		}
		rgPageList[cPagesAllocated] = page;
		rgblPageNeedCommit[cPagesAllocated] = 0;
		cPagesAllocated++;

		count2 -= bytes;
		pos2 += bytes;
	} while (count2);

	do {
		unsigned long index, offset;
		char *kaddr;

		offset = (pos & (PAGE_CACHE_SIZE -1));  
		index = pos >> PAGE_CACHE_SHIFT;
		bytes = PAGE_CACHE_SIZE - offset;
		if (bytes > count)
		   bytes = count;
		page = rgPageList[crgPagePtr];
		kaddr = kmap(page);

		if (mapping->a_ops->prepare_write) {
			status = mapping->a_ops->prepare_write(file, page, offset, offset+bytes);
		}else{
			printk("%s(%d) prepare_write does not implement\n", __FUNCTION__, __LINE__);
			status = -EINVAL;
		}

		if (status) {
			 
			crgPagePtr++;
			while( crgPagePtr < cPagesAllocated ) {
			   kmap(rgPageList[crgPagePtr]);
			   crgPagePtr++;
			}
			if (pos + bytes > inode->i_size)
			   blTruncate = 1;
			err = status;
			goto done;
		}
		rgblPageNeedCommit[crgPagePtr] = 1;
		rgOffset[crgPagePtr] = offset;
		rgBytes[crgPagePtr] = bytes;
		iov[crgPagePtr].iov_base = kaddr+offset;
		iov[crgPagePtr].iov_len = bytes;
		crgPagePtr++;

		if(crgPagePtr > MAX_PAGES_PER_RECVFILE+1)
			panic("allocate %d pages in do_recvfile()\n",crgPagePtr);
		count -= bytes;
		pos += bytes;
		cBytesToReceive += bytes;
	} while (count);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = (struct iovec *) &iov[0];
	msg.msg_iovlen = crgPagePtr ;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = MSG_KERNSPACE;
	rcvtimeo = sock->sk->sk_rcvtimeo;
	sock->sk->sk_rcvtimeo = 64 * HZ;

	ret = kernel_recvmsg(sock, &msg, &iov[0], crgPagePtr, cBytesToReceive, MSG_WAITALL | MSG_NOCATCHSIGNAL);

	sock->sk->sk_rcvtimeo = rcvtimeo;

	if (ret >= 0) {
		bytes_received = ret;
		if (ret != cBytesToReceive) {
			err = -EPIPE;
		}
		 
	} else {
		err = ret;
	}

	if (err) {
#ifdef SYNO_DEBUG_BUILD
		printk("do_recvfile: bytes_received %d , count = %d, err = %d\n",
				bytes_received, count, err);
#endif
	}

	*rbytes = bytes_received;
	*wbytes = bytes_received;

done:
	*ppos = pos;

	crgPagePtr = 0;
	while (crgPagePtr < cPagesAllocated) {
		page = rgPageList[crgPagePtr];
		flush_dcache_page(page);
		if(rgblPageNeedCommit[crgPagePtr]){
			if (mapping->a_ops->commit_write) {
				status = mapping->a_ops->commit_write(file, page,
									 rgOffset[crgPagePtr], rgOffset[crgPagePtr] + rgBytes[crgPagePtr]);
			}else{
				printk("%s(%d) commit_write does not implement\n", __FUNCTION__, __LINE__);
				status = -EINVAL;
			}
		}
		if((status)&&(!err))
			err = status;
		kunmap(page);
		 
		unlock_page(page);
		mark_page_accessed(page);
		page_cache_release(page);
		cond_resched();
		crgPagePtr++;
	}

	if (!err) {
		balance_dirty_pages_ratelimited_nr(mapping, cPagesAllocated);
	}

done1:
	current->backing_dev_info = NULL;

	if (blTruncate)
		vmtruncate(inode, inode->i_size);

	if (err) {
		return err;
	} else {
		return bytes_received;
	}
}
#else
int
do_recvfile(struct file *file, struct socket *sock, loff_t * ppos,
			size_t count, size_t * rbytes, size_t * wbytes)
{
	struct address_space *mapping = file->f_mapping;
	struct inode   *inode = mapping->host;
	loff_t          pos;
	struct page    *page;
	ssize_t         err = 0;
	unsigned        bytes;

	unsigned        bytes_received = 0;
	struct kvec     iov[MAX_PAGES_PER_RECVFILE + 1];
	struct page    *rgPageList[MAX_PAGES_PER_RECVFILE + 1];

	int             cPagesAllocated;
	int             failed_write_flag = 0;

	loff_t          rgPos[MAX_PAGES_PER_RECVFILE + 1];
	unsigned        rgBytes[MAX_PAGES_PER_RECVFILE + 1];
	void           *fsdata[MAX_PAGES_PER_RECVFILE + 1];
	long            write_begin_ret = 0, write_end_ret = 0, kernel_recvmsg_ret = 0;

	long            rcvtimeo;
	struct msghdr   msg;
	size_t          cBytesToReceive = 0;
	int             crgPagePtr = 0;
	int             flags = AOP_FLAG_UNINTERRUPTIBLE|AOP_FLAG_RECVFILE;

	*rbytes = 0;
	*wbytes = 0;
	pos = *ppos;

	vfs_check_frozen(inode->i_sb, SB_FREEZE_WRITE);

	current->backing_dev_info = mapping->backing_dev_info;

	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (err != 0 || count == 0) {
		goto done1;
	}

	if (!mapping->a_ops->write_begin || !mapping->a_ops->write_end) {
		printk("write_begin() or write_end() is not implemented\n");
		goto done1;
	}
	file_remove_suid(file);
	file_update_time(file);

	cPagesAllocated = 0;
	do {
		pgoff_t         offset;
		char           *kaddr;

		offset = (pos & (PAGE_CACHE_SIZE - 1));	 
		bytes = min_t(unsigned int, PAGE_CACHE_SIZE - offset, count);

		page = NULL;
		write_begin_ret =
			mapping->a_ops->write_begin(
					file, mapping, pos, bytes, flags,
					&page, &fsdata[cPagesAllocated]);
		if (write_begin_ret) {
			err = write_begin_ret;
			goto done;
		}

		kaddr = kmap(page);
		rgPageList[cPagesAllocated] = page;
		rgPos[cPagesAllocated] = pos;
		rgBytes[cPagesAllocated] = bytes;
		iov[cPagesAllocated].iov_base = kaddr + offset;
		iov[cPagesAllocated].iov_len = bytes;
		cPagesAllocated++;

		if (cPagesAllocated > MAX_PAGES_PER_RECVFILE + 1) {
			panic("allocate %d pages in do_recvfile()\n",
					cPagesAllocated);
		}

		count -= bytes;
		pos += bytes;
		cBytesToReceive += bytes;
	} while (count);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = (struct iovec *) &iov[0];
	msg.msg_iovlen = cPagesAllocated;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = MSG_KERNSPACE;
	rcvtimeo = sock->sk->sk_rcvtimeo;
	sock->sk->sk_rcvtimeo = 64 * HZ;

	kernel_recvmsg_ret = kernel_recvmsg(
			sock, &msg, &iov[0], cPagesAllocated, cBytesToReceive,
			MSG_WAITALL | MSG_NOCATCHSIGNAL);

	sock->sk->sk_rcvtimeo = rcvtimeo;

	if (kernel_recvmsg_ret >= 0) {
		bytes_received = kernel_recvmsg_ret;
		if (kernel_recvmsg_ret != cBytesToReceive) {
			err = -EPIPE;
		}
		 
	} else {
		err = kernel_recvmsg_ret;
	}
#ifdef SYNO_DEBUG_BUILD
	if (err) {
		printk("do_recvfile: bytes_received %d , count = %d, err = %d\n",
				bytes_received, count, err);
	}
#endif

done:
	*ppos = pos;

	crgPagePtr = 0;

		while (crgPagePtr < cPagesAllocated) {
			page = rgPageList[crgPagePtr];

			kunmap(page);
			write_end_ret = mapping->a_ops->write_end(
					file, mapping, rgPos[crgPagePtr],
					rgBytes[crgPagePtr], rgBytes[crgPagePtr],
					page, fsdata[crgPagePtr]);
			 
			if (0 > write_end_ret) {
				if (!failed_write_flag) {
					failed_write_flag = 1;
					if (crgPagePtr) {
						bytes_received = rgBytes[0] + PAGE_CACHE_SIZE*(crgPagePtr-1);
						if (bytes_received > kernel_recvmsg_ret)
							bytes_received = kernel_recvmsg_ret;
					} else {
						bytes_received = 0;
					}
				}
				if (!err) {
					err = write_end_ret;
				}
			}
#if defined(MY_DEF_HERE)
#else
			cond_resched();
#endif
			crgPagePtr++;
		}

	if (0 < bytes_received) {
		*rbytes = kernel_recvmsg_ret;
		*wbytes = bytes_received;
	} else {
		*rbytes = kernel_recvmsg_ret;
		*wbytes = 0;
	}

	if (!err) {
		balance_dirty_pages_ratelimited_nr(mapping, cPagesAllocated);
	}

done1:
	current->backing_dev_info = NULL;
	if (err) {
		return err;
	} else {
		return bytes_received;
	}
}

#endif
#endif

ssize_t
generic_file_direct_write(struct kiocb *iocb, const struct iovec *iov,
		unsigned long *nr_segs, loff_t pos, loff_t *ppos,
		size_t count, size_t ocount)
{
	struct file	*file = iocb->ki_filp;
	struct address_space *mapping = file->f_mapping;
	struct inode	*inode = mapping->host;
	ssize_t		written;
	size_t		write_len;
	pgoff_t		end;

	if (count != ocount)
		*nr_segs = iov_shorten((struct iovec *)iov, *nr_segs, count);

	write_len = iov_length(iov, *nr_segs);
	end = (pos + write_len - 1) >> PAGE_CACHE_SHIFT;

	written = filemap_write_and_wait_range(mapping, pos, pos + write_len - 1);
	if (written)
		goto out;

	if (mapping->nrpages) {
		written = invalidate_inode_pages2_range(mapping,
					pos >> PAGE_CACHE_SHIFT, end);
		 
		if (written) {
			if (written == -EBUSY)
				return 0;
			goto out;
		}
	}

	written = mapping->a_ops->direct_IO(WRITE, iocb, iov, pos, *nr_segs);

	if (mapping->nrpages) {
		invalidate_inode_pages2_range(mapping,
					      pos >> PAGE_CACHE_SHIFT, end);
	}

	if (written > 0) {
		loff_t end = pos + written;
		if (end > i_size_read(inode) && !S_ISBLK(inode->i_mode)) {
			i_size_write(inode,  end);
			mark_inode_dirty(inode);
		}
		*ppos = end;
	}
out:
	return written;
}
EXPORT_SYMBOL(generic_file_direct_write);

struct page *grab_cache_page_write_begin(struct address_space *mapping,
					pgoff_t index, unsigned flags)
{
	int status;
	struct page *page;
	gfp_t gfp_notmask = 0;
	if (flags & AOP_FLAG_NOFS)
		gfp_notmask = __GFP_FS;
repeat:
	page = find_lock_page(mapping, index);
	if (likely(page))
		return page;

	page = __page_cache_alloc(mapping_gfp_mask(mapping) & ~gfp_notmask);
	if (!page)
		return NULL;
	status = add_to_page_cache_lru(page, mapping, index,
						GFP_KERNEL & ~gfp_notmask);
	if (unlikely(status)) {
		page_cache_release(page);
		if (status == -EEXIST)
			goto repeat;
		return NULL;
	}
	return page;
}
EXPORT_SYMBOL(grab_cache_page_write_begin);

static ssize_t generic_perform_write(struct file *file,
				struct iov_iter *i, loff_t pos)
{
	struct address_space *mapping = file->f_mapping;
	const struct address_space_operations *a_ops = mapping->a_ops;
	long status = 0;
	ssize_t written = 0;
	unsigned int flags = 0;

	if (segment_eq(get_fs(), KERNEL_DS))
		flags |= AOP_FLAG_UNINTERRUPTIBLE;

	do {
		struct page *page;
		pgoff_t index;		 
		unsigned long offset;	 
		unsigned long bytes;	 
		size_t copied;		 
		void *fsdata;

		offset = (pos & (PAGE_CACHE_SIZE - 1));
		index = pos >> PAGE_CACHE_SHIFT;
		bytes = min_t(unsigned long, PAGE_CACHE_SIZE - offset,
						iov_iter_count(i));

again:

		if (unlikely(iov_iter_fault_in_readable(i, bytes))) {
			status = -EFAULT;
			break;
		}

#ifdef SYNO_FORCE_UNMOUNT
		if (!blSynostate(O_UNMOUNT_OK, file)) {
			status = -EIO;
			break;
		}
#endif
		status = a_ops->write_begin(file, mapping, pos, bytes, flags,
						&page, &fsdata);
		if (unlikely(status))
			break;

		if (mapping_writably_mapped(mapping))
			flush_dcache_page(page);

		pagefault_disable();
		copied = iov_iter_copy_from_user_atomic(page, i, offset, bytes);
		pagefault_enable();
		flush_dcache_page(page);

		mark_page_accessed(page);
		status = a_ops->write_end(file, mapping, pos, bytes, copied,
						page, fsdata);
		if (unlikely(status < 0))
			break;
		copied = status;

		cond_resched();

		iov_iter_advance(i, copied);
		if (unlikely(copied == 0)) {
			 
			bytes = min_t(unsigned long, PAGE_CACHE_SIZE - offset,
						iov_iter_single_seg_count(i));
			goto again;
		}
		pos += copied;
		written += copied;

		balance_dirty_pages_ratelimited(mapping);

	} while (iov_iter_count(i));

	return written ? written : status;
}

ssize_t
generic_file_buffered_write(struct kiocb *iocb, const struct iovec *iov,
		unsigned long nr_segs, loff_t pos, loff_t *ppos,
		size_t count, ssize_t written)
{
	struct file *file = iocb->ki_filp;
	struct address_space *mapping = file->f_mapping;
	ssize_t status;
	struct iov_iter i;

	iov_iter_init(&i, iov, nr_segs, count, written);
	status = generic_perform_write(file, &i, pos);

	if (likely(status >= 0)) {
		written += status;
		*ppos = pos + status;
  	}
	
	if (unlikely(file->f_flags & O_DIRECT) && written)
		status = filemap_write_and_wait_range(mapping,
					pos, pos + written - 1);

	return written ? written : status;
}
EXPORT_SYMBOL(generic_file_buffered_write);

#ifdef CONFIG_SYNO_PLX_PORTING
#ifndef CONFIG_SYNO_PLX_PORTING
 typedef struct oxnas_net_get_bytes_args {
 	struct sock    *sk;
 	char           *ptr;
 	size_t          len;
 	size_t          preadvance;
 	struct sk_buff *cached_skb;
 	u32             cached_offset;
 	int             cleanup;
 } oxnas_net_get_bytes_args_t;
#endif
 
 typedef int (*oxnas_net_get_bytes_t)(oxnas_net_get_bytes_args_t *args);
 
 extern void release_sock(struct sock *sk);
 extern void lock_sock_nested(struct sock *sk, int subclass);
 
 #define NET_BYTES_CLEANUP_LIMIT	(62*1024)
 static ssize_t generic_perform_direct_netrx_write(
 	struct file *file,
 	void        *callback,
 	void        *sock,
 	u32          length,
 	loff_t       pos)
 {
 	struct address_space                  *mapping = file->f_mapping;
 	const struct address_space_operations *a_ops = mapping->a_ops;
 	struct sock                           *sk = (struct sock*)sock;
 	long                                   status = 0;
 	ssize_t                                written = 0;
 	int                                    pages_dirtied = 0;
 	size_t								   net_bytes_since_cleanup = 0;
 
 	oxnas_net_get_bytes_args_t get_bytes_args = {
 		.sk = sk,
 		.ptr = NULL,
 		.len = 0,
 		.preadvance = 0,
 		.cached_skb = NULL,
 		.cached_offset = 0,
 		.cleanup = 0
 	};
 
 	while(1) {
 		struct page *page;
 		u32          offset;
 		u32          bytes;
 		void        *fsdata;
 		u32          page_remaining;
 		size_t       copied = 0;
 		int          net_status = 0;
 		int          exhausted_net_bytes = 0;
 
 		offset = (pos & (PAGE_CACHE_SIZE - 1));
 
 		bytes = min_t(u32, PAGE_CACHE_SIZE - offset, length);
 
 		status = a_ops->write_begin(file, mapping, pos, bytes,
 			AOP_FLAG_UNINTERRUPTIBLE, &page, &fsdata);
 
 		lock_sock_nested(sk, 0);
 
 		if (unlikely(status)) {
  
 			break;
 		}
 
 		page_remaining = bytes;
  
 		while (!net_status && page_remaining) {
 			size_t bytes_from_netrx;
 
 			get_bytes_args.len = page_remaining;
 			while (1) {
  
 				net_status = ((oxnas_net_get_bytes_t)callback)(&get_bytes_args);
 
 				if (likely(net_status != -EAGAIN)) {
 					bytes_from_netrx = get_bytes_args.len;
 					break;
 				}
  
 			}
  
 			if (unlikely(!bytes_from_netrx)) {
 				exhausted_net_bytes = 1;
 				break;
 			}
 
 			memcpy(page_address(page) + offset, get_bytes_args.ptr, bytes_from_netrx);
 
 			net_bytes_since_cleanup += bytes_from_netrx;
 			if (net_bytes_since_cleanup >= NET_BYTES_CLEANUP_LIMIT) {
 				get_bytes_args.cleanup = 1;
 				get_bytes_args.len = net_bytes_since_cleanup;
 				((oxnas_net_get_bytes_t)callback)(&get_bytes_args);
 				get_bytes_args.cleanup = 0;
 				net_bytes_since_cleanup = 0;
 			}
 
 			page_remaining -= bytes_from_netrx;
 			offset += bytes_from_netrx;
 			copied += bytes_from_netrx;
 		}
  
 		release_sock(sk);
 
 		flush_dcache_page(page);
 
 		status = a_ops->write_end(file, mapping, pos, bytes, copied, page, fsdata);
 		if (unlikely(status < 0)) {
 			lock_sock_nested(sk, 0);
  
 			break;
 		}
 		copied = status;
  
 		++pages_dirtied;
 
 		pos += copied;
 		written += copied;
 
 		status = net_status;
 		length -= copied;
  
 		if (!length || status || exhausted_net_bytes) {
 			lock_sock_nested(sk, 0);
  
 			break;
 		}
 
 #if 0
 		 
 		cond_resched();
 #endif
 	}
 
 	if (net_bytes_since_cleanup) {
 		get_bytes_args.cleanup = 1;
 		get_bytes_args.len = net_bytes_since_cleanup;
 		((oxnas_net_get_bytes_t)callback)(&get_bytes_args);
 	}
 
 	release_sock(sk);
 
 	balance_dirty_pages_ratelimited_nr(mapping, pages_dirtied);
 
 	return written ? written : status;
 }
 
 ssize_t generic_file_direct_netrx_write(
 	struct kiocb *iocb,
 	void         *callback,
 	void         *sock,
 	loff_t        pos,
 	loff_t       *ppos,
 	u32           count,
 	ssize_t       written)
 {
 	struct file                           *file = iocb->ki_filp;
 	struct address_space                  *mapping = file->f_mapping;
 	const struct address_space_operations *a_ops = mapping->a_ops;
 	struct inode                          *inode = mapping->host;
 	ssize_t                                status;
 
 	status = generic_perform_direct_netrx_write(file, callback, sock, count, pos);
 	if (likely(status >= 0)) {
 		written += status;
 		*ppos = pos + status;
 
 		if (unlikely((file->f_flags & O_SYNC) || IS_SYNC(inode))) {
 			if (!a_ops->writepage || !is_sync_kiocb(iocb))
 				status = generic_osync_inode(inode, mapping,
 						OSYNC_METADATA|OSYNC_DATA);
 		}
   	}
 
 	return written ? written : status;
 }
 EXPORT_SYMBOL(generic_file_direct_netrx_write);
#endif

ssize_t __generic_file_aio_write(struct kiocb *iocb, const struct iovec *iov,
				 unsigned long nr_segs, loff_t *ppos)
{
	struct file *file = iocb->ki_filp;
	struct address_space * mapping = file->f_mapping;
	size_t ocount;		 
	size_t count;		 
	struct inode 	*inode = mapping->host;
	loff_t		pos;
	ssize_t		written;
	ssize_t		err;

	ocount = 0;
	err = generic_segment_checks(iov, &nr_segs, &ocount, VERIFY_READ);
	if (err)
		return err;

	count = ocount;
	pos = *ppos;

	vfs_check_frozen(inode->i_sb, SB_FREEZE_WRITE);

	current->backing_dev_info = mapping->backing_dev_info;
	written = 0;

	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (err)
		goto out;

	if (count == 0)
		goto out;

	err = file_remove_suid(file);
	if (err)
		goto out;

	file_update_time(file);

	if (unlikely(file->f_flags & O_DIRECT)) {
		loff_t endbyte;
		ssize_t written_buffered;

		written = generic_file_direct_write(iocb, iov, &nr_segs, pos,
							ppos, count, ocount);
		if (written < 0 || written == count)
			goto out;
		 
		pos += written;
		count -= written;
		written_buffered = generic_file_buffered_write(iocb, iov,
						nr_segs, pos, ppos, count,
						written);
		 
		if (written_buffered < 0) {
			err = written_buffered;
			goto out;
		}

		endbyte = pos + written_buffered - written - 1;
		err = do_sync_mapping_range(file->f_mapping, pos, endbyte,
					    SYNC_FILE_RANGE_WAIT_BEFORE|
					    SYNC_FILE_RANGE_WRITE|
					    SYNC_FILE_RANGE_WAIT_AFTER);
		if (err == 0) {
			written = written_buffered;
			invalidate_mapping_pages(mapping,
						 pos >> PAGE_CACHE_SHIFT,
						 endbyte >> PAGE_CACHE_SHIFT);
		} else {
			 
		}
	} else {
		written = generic_file_buffered_write(iocb, iov, nr_segs,
				pos, ppos, count, written);
	}
out:
	current->backing_dev_info = NULL;
	return written ? written : err;
}
EXPORT_SYMBOL(__generic_file_aio_write);

ssize_t generic_file_aio_write(struct kiocb *iocb, const struct iovec *iov,
		unsigned long nr_segs, loff_t pos)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file->f_mapping->host;
	ssize_t ret;

	BUG_ON(iocb->ki_pos != pos);

	mutex_lock(&inode->i_mutex);
	ret = __generic_file_aio_write(iocb, iov, nr_segs, &iocb->ki_pos);
	mutex_unlock(&inode->i_mutex);

	if (ret > 0 || ret == -EIOCBQUEUED) {
		ssize_t err;

		err = generic_write_sync(file, pos, ret);
		if (err < 0 && ret > 0)
			ret = err;
	}
	return ret;
}
EXPORT_SYMBOL(generic_file_aio_write);

int try_to_release_page(struct page *page, gfp_t gfp_mask)
{
	struct address_space * const mapping = page->mapping;

	BUG_ON(!PageLocked(page));
	if (PageWriteback(page))
		return 0;

	if (mapping && mapping->a_ops->releasepage)
		return mapping->a_ops->releasepage(page, gfp_mask);
	return try_to_free_buffers(page);
}

EXPORT_SYMBOL(try_to_release_page);
