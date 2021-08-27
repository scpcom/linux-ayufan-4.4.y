#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/dmaengine.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <net/tcp.h>  
#include <asm/io.h>
#include <asm/uaccess.h>

static int num_pages_spanned(struct iovec *iov)
{
	return
	((PAGE_ALIGN((unsigned long)iov->iov_base + iov->iov_len) -
	((unsigned long)iov->iov_base & PAGE_MASK)) >> PAGE_SHIFT);
}
#ifdef MY_DEF_HERE
#define NETDMA_MAX_NR_IOVECS	20
#ifdef CONFIG_SYNO_NETDMA_BUF_NOT_ENOUGH_FIX
#define NETDMA_MAX_NR_PAGES	20
#else
#define NETDMA_MAX_NR_PAGES	20
#endif
#endif

#ifdef MY_DEF_HERE
int dma_pin_iovec_pages(struct tcp_sock *tp, struct iovec *iov, size_t len)
#else
struct dma_pinned_list *dma_pin_iovec_pages(struct iovec *iov, size_t len)
#endif
{
	struct dma_pinned_list *local_list;
	struct page **pages;
	int i;
	int ret;
	int nr_iovecs = 0;
	int iovec_len_used = 0;
	int iovec_pages_used = 0;

#ifdef MY_DEF_HERE
	if (!tp->ucopy.pinned_list) {
		 
		local_list = kmalloc(sizeof(*local_list)
		+ (NETDMA_MAX_NR_IOVECS * sizeof (struct dma_page_list))
		+ (NETDMA_MAX_NR_PAGES * sizeof (struct page*)), GFP_KERNEL);

		BUG_ON(!local_list);
		 
		local_list->sgts = kmalloc(2 * sizeof(struct sg_table), GFP_KERNEL);
		sg_alloc_table(local_list->sgts, NETDMA_MAX_NR_PAGES, GFP_KERNEL);
		sg_alloc_table(local_list->sgts + 1, NETDMA_MAX_NR_IOVECS, GFP_KERNEL);

		tp->ucopy.pinned_list = local_list;
	}

	local_list = tp->ucopy.pinned_list;
#else
	 
	if (segment_eq(get_fs(), KERNEL_DS))
		return NULL;
#endif

	do {
		iovec_len_used += iov[nr_iovecs].iov_len;
		iovec_pages_used += num_pages_spanned(&iov[nr_iovecs]);
		nr_iovecs++;
	} while (iovec_len_used < len);

#ifdef MY_DEF_HERE
	if (iovec_pages_used > NETDMA_MAX_NR_PAGES || nr_iovecs > NETDMA_MAX_NR_IOVECS) {
		return -1;
	}

	BUG_ON(nr_iovecs > NETDMA_MAX_NR_IOVECS);
	BUG_ON(iovec_pages_used > NETDMA_MAX_NR_PAGES);

	local_list->nr_pages = iovec_pages_used;
	if (segment_eq(get_fs(), KERNEL_DS))
               local_list->kernel = 1;
        else
               local_list->kernel = 0;
#else
	 
	local_list = kmalloc(sizeof(*local_list)
		+ (nr_iovecs * sizeof (struct dma_page_list))
		+ (iovec_pages_used * sizeof (struct page*)), GFP_KERNEL);
	if (!local_list)
		goto out;
#endif

	pages = (struct page **) &local_list->page_list[nr_iovecs];

	local_list->nr_iovecs = 0;

#ifdef MY_DEF_HERE
	if (local_list->kernel) {
		for (i = 0; i < nr_iovecs; i++) {
			struct dma_page_list *page_list = &local_list->page_list[i];
			page_list->base_address = iov[i].iov_base;
			page_list->nr_pages = num_pages_spanned(&iov[i]);
			BUG_ON(num_pages_spanned(&iov[i]) != 1);
			*pages = kmap_to_page(iov[i].iov_base);
			page_list->pages = pages;
			pages++;
			local_list->nr_iovecs = i + 1;
		}
		pr_debug("%s %d: added kernel %d pages (%d vecs)\n", __func__, __LINE__,
				local_list->nr_pages, local_list->nr_iovecs);

	} else {
#endif
		for (i = 0; i < nr_iovecs; i++) {
			struct dma_page_list *page_list = &local_list->page_list[i];

			len -= iov[i].iov_len;

			if (!access_ok(VERIFY_WRITE, iov[i].iov_base, iov[i].iov_len))
				goto unpin;

			page_list->nr_pages = num_pages_spanned(&iov[i]);
			page_list->base_address = iov[i].iov_base;

			page_list->pages = pages;
			pages += page_list->nr_pages;

			down_read(&current->mm->mmap_sem);
			ret = get_user_pages(
					current,
					current->mm,
					(unsigned long) iov[i].iov_base,
					page_list->nr_pages,
					1,	 
					0,	 
					page_list->pages,
					NULL);
			up_read(&current->mm->mmap_sem);

			if (ret != page_list->nr_pages)
				goto unpin;

			local_list->nr_iovecs = i + 1;
		}

#ifdef MY_DEF_HERE
		pr_debug("%s %d: added user %d pages (%d vecs)\n", __func__, __LINE__,
				local_list->nr_pages, local_list->nr_iovecs);
	}
	return 0;
#else
	return local_list;
#endif

unpin:
	dma_unpin_iovec_pages(local_list);
#ifdef MY_DEF_HERE
	return -1;
#else
out:
	return NULL;
#endif
}
#ifdef MY_DEF_HERE
void dma_free_iovec_data(struct tcp_sock *tp)
{
        struct dma_pinned_list *local_list = tp->ucopy.pinned_list;

        if (local_list) {
                sg_free_table(local_list->sgts);
                sg_free_table(local_list->sgts + 1);
                kfree(local_list->sgts);
                kfree(tp->ucopy.pinned_list);
        }
}
#endif

#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_SPLICE_NET_DMA_SUPPORT)
struct dma_pinned_list *dma_pin_kernel_iovec_pages(struct iovec *iov, size_t len)
{
	struct dma_pinned_list *local_list;
	struct page **pages;
	int i, j;
	int nr_iovecs = 0;
	int iovec_len_used = 0;
	int iovec_pages_used = 0;

	do {
		iovec_len_used += iov[nr_iovecs].iov_len;
		iovec_pages_used += num_pages_spanned(&iov[nr_iovecs]);
		nr_iovecs++;
	} while (iovec_len_used < len);

	local_list = kmalloc(sizeof(*local_list)
		+ (nr_iovecs * sizeof (struct dma_page_list))
		+ (iovec_pages_used * sizeof (struct page*)), GFP_KERNEL);
	if (!local_list)
		goto out;

	pages = (struct page **) &local_list->page_list[nr_iovecs];

	local_list->nr_iovecs = 0;

	for (i = 0; i < nr_iovecs; i++) {
		struct dma_page_list *page_list = &local_list->page_list[i];
		int offset;

		len -= iov[i].iov_len;

		if (!access_ok(VERIFY_WRITE, iov[i].iov_base, iov[i].iov_len))
			goto unpin;

		page_list->nr_pages = num_pages_spanned(&iov[i]);
		page_list->base_address = iov[i].iov_base;

		page_list->pages = pages;
		pages += page_list->nr_pages;

		for (offset=0, j=0; j < page_list->nr_pages; j++, offset+=PAGE_SIZE) {
			page_list->pages[j] = phys_to_page(__pa((unsigned int)page_list->base_address) + offset);
		}
		local_list->nr_iovecs = i + 1;
	}

	return local_list;

unpin:
	kfree(local_list);
out:
	return NULL;
}

void dma_unpin_kernel_iovec_pages(struct dma_pinned_list *pinned_list)
{
	if (!pinned_list)
		return;

	kfree(pinned_list);
}
#endif

void dma_unpin_iovec_pages(struct dma_pinned_list *pinned_list)
{
	int i, j;

	if (!pinned_list)
		return;

#ifdef MY_DEF_HERE
	if (!pinned_list->kernel) {
#endif
		for (i = 0; i < pinned_list->nr_iovecs; i++) {
			struct dma_page_list *page_list = &pinned_list->page_list[i];
			for (j = 0; j < page_list->nr_pages; j++) {
				set_page_dirty_lock(page_list->pages[j]);
				page_cache_release(page_list->pages[j]);
			}
		}

#ifdef MY_DEF_HERE
	}
#else
	kfree(pinned_list);
#endif
}

#ifdef MY_DEF_HERE
 
int dma_memcpy_fill_sg_from_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, struct scatterlist *dst_sg,
	unsigned int offset, size_t len)
{
	int iov_byte_offset;
	int copy;
	int iovec_idx;
	int page_idx;
	int sg_nents = 0;

	pr_debug("%s %d:  nr iovecs %d. len 0x%x\n",
					__func__, __LINE__,
					 pinned_list->nr_iovecs, len);

	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;

		pr_debug("%s %d: iov idx %d. len 0x%x\n",
					__func__, __LINE__,
					 iovec_idx, iov[iovec_idx].iov_len);

		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			pr_debug("%s %d: add dst buf page %p. len 0x%x offset 0x%x\n",
					__func__, __LINE__,
					page_list->pages[page_idx], copy, iov_byte_offset);
			sg_set_page(dst_sg, page_list->pages[page_idx],
					copy, iov_byte_offset);

			dst_sg = sg_next(dst_sg);
			sg_nents++;

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			page_idx++;
			if (!len)
				return sg_nents;

			offset += copy;
			iov_byte_offset = 0;
		}
		iovec_idx++;
	}

	BUG();
	return -EFAULT;
}
#endif
 
dma_cookie_t dma_memcpy_to_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, unsigned char *kdata, size_t len)
{
	int iov_byte_offset;
	int copy;
	dma_cookie_t dma_cookie = 0;
	int iovec_idx;
	int page_idx;

	if (!chan)
		return memcpy_toiovec(iov, kdata, len);

	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;

		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			dma_cookie = dma_async_memcpy_buf_to_pg(chan,
					page_list->pages[page_idx],
					iov_byte_offset,
					kdata,
					copy);
			 
			if (unlikely(dma_cookie < 0)) {
				dma_async_issue_pending(chan);
				continue;
			}

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			if (!len)
				return dma_cookie;

			kdata += copy;
			iov_byte_offset = 0;
			page_idx++;
		}
		iovec_idx++;
	}

	BUG();
	return -EFAULT;
}

dma_cookie_t dma_memcpy_pg_to_iovec(struct dma_chan *chan, struct iovec *iov,
	struct dma_pinned_list *pinned_list, struct page *page,
	unsigned int offset, size_t len)
{
	int iov_byte_offset;
	int copy;
	dma_cookie_t dma_cookie = 0;
	int iovec_idx;
	int page_idx;
	int err;

	if (!chan || !pinned_list) {
		u8 *vaddr = kmap(page);
		err = memcpy_toiovec(iov, vaddr + offset, len);
		kunmap(page);
		return err;
	}

	iovec_idx = 0;
	while (iovec_idx < pinned_list->nr_iovecs) {
		struct dma_page_list *page_list;

		while (!iov[iovec_idx].iov_len)
			iovec_idx++;

		page_list = &pinned_list->page_list[iovec_idx];

		iov_byte_offset = ((unsigned long)iov[iovec_idx].iov_base & ~PAGE_MASK);
		page_idx = (((unsigned long)iov[iovec_idx].iov_base & PAGE_MASK)
			 - ((unsigned long)page_list->base_address & PAGE_MASK)) >> PAGE_SHIFT;

		while (iov[iovec_idx].iov_len) {
			copy = min_t(int, PAGE_SIZE - iov_byte_offset, len);
			copy = min_t(int, copy, iov[iovec_idx].iov_len);

			dma_cookie = dma_async_memcpy_pg_to_pg(chan,
					page_list->pages[page_idx],
					iov_byte_offset,
					page,
					offset,
					copy);
			 
			if (unlikely(dma_cookie < 0)) {
				dma_async_issue_pending(chan);
				continue;
			}

			len -= copy;
			iov[iovec_idx].iov_len -= copy;
			iov[iovec_idx].iov_base += copy;

			if (!len)
				return dma_cookie;

			offset += copy;
			iov_byte_offset = 0;
			page_idx++;
		}
		iovec_idx++;
	}

	BUG();
	return -EFAULT;
}
