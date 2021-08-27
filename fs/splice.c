#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
#include <linux/memcontrol.h>
#include <linux/mm_inline.h>
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/buffer_head.h>
#ifdef MY_DEF_HERE
#include <linux/export.h>
#endif
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/uio.h>
#include <linux/security.h>
#include <linux/gfp.h>
#include <linux/socket.h>
#if defined(MY_ABC_HERE)
#include <linux/time.h>
#endif
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#include <net/sock.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/genalloc.h>

struct common_mempool;
static struct common_mempool  * rcv_pool = NULL;
static struct common_mempool  * kvec_pool = NULL;
#endif

#if defined(MY_ABC_HERE)
#include <net/sock.h>
#include <linux/net.h>
#include <linux/genalloc.h>
#endif

static int page_cache_pipe_buf_steal(struct pipe_inode_info *pipe,
				     struct pipe_buffer *buf)
{
	struct page *page = buf->page;
	struct address_space *mapping;

	lock_page(page);

	mapping = page_mapping(page);
	if (mapping) {
		WARN_ON(!PageUptodate(page));

		wait_on_page_writeback(page);

		if (page_has_private(page) &&
		    !try_to_release_page(page, GFP_KERNEL))
			goto out_unlock;

		if (remove_mapping(mapping, page)) {
			buf->flags |= PIPE_BUF_FLAG_LRU;
			return 0;
		}
	}

out_unlock:
	unlock_page(page);
	return 1;
}

static void page_cache_pipe_buf_release(struct pipe_inode_info *pipe,
					struct pipe_buffer *buf)
{
	page_cache_release(buf->page);
	buf->flags &= ~PIPE_BUF_FLAG_LRU;
}

static int page_cache_pipe_buf_confirm(struct pipe_inode_info *pipe,
				       struct pipe_buffer *buf)
{
	struct page *page = buf->page;
	int err;

	if (!PageUptodate(page)) {
		lock_page(page);

		if (!page->mapping) {
			err = -ENODATA;
			goto error;
		}

		if (!PageUptodate(page)) {
			err = -EIO;
			goto error;
		}

		unlock_page(page);
	}

	return 0;
error:
	unlock_page(page);
	return err;
}

const struct pipe_buf_operations page_cache_pipe_buf_ops = {
	.can_merge = 0,
	.map = generic_pipe_buf_map,
	.unmap = generic_pipe_buf_unmap,
	.confirm = page_cache_pipe_buf_confirm,
	.release = page_cache_pipe_buf_release,
	.steal = page_cache_pipe_buf_steal,
	.get = generic_pipe_buf_get,
};

static int user_page_pipe_buf_steal(struct pipe_inode_info *pipe,
				    struct pipe_buffer *buf)
{
	if (!(buf->flags & PIPE_BUF_FLAG_GIFT))
		return 1;

	buf->flags |= PIPE_BUF_FLAG_LRU;
	return generic_pipe_buf_steal(pipe, buf);
}

static const struct pipe_buf_operations user_page_pipe_buf_ops = {
	.can_merge = 0,
	.map = generic_pipe_buf_map,
	.unmap = generic_pipe_buf_unmap,
	.confirm = generic_pipe_buf_confirm,
	.release = page_cache_pipe_buf_release,
	.steal = user_page_pipe_buf_steal,
	.get = generic_pipe_buf_get,
};

static void wakeup_pipe_readers(struct pipe_inode_info *pipe)
{
	smp_mb();
	if (waitqueue_active(&pipe->wait))
		wake_up_interruptible(&pipe->wait);
	kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
}

ssize_t splice_to_pipe(struct pipe_inode_info *pipe,
		       struct splice_pipe_desc *spd)
{
	unsigned int spd_pages = spd->nr_pages;
	int ret, do_wakeup, page_nr;

	ret = 0;
	do_wakeup = 0;
	page_nr = 0;

	pipe_lock(pipe);

	for (;;) {
		if (!pipe->readers) {
			send_sig(SIGPIPE, current, 0);
			if (!ret)
				ret = -EPIPE;
			break;
		}

		if (pipe->nrbufs < pipe->buffers) {
			int newbuf = (pipe->curbuf + pipe->nrbufs) & (pipe->buffers - 1);
			struct pipe_buffer *buf = pipe->bufs + newbuf;

			buf->page = spd->pages[page_nr];
			buf->offset = spd->partial[page_nr].offset;
			buf->len = spd->partial[page_nr].len;
			buf->private = spd->partial[page_nr].private;
			buf->ops = spd->ops;
			if (spd->flags & SPLICE_F_GIFT)
				buf->flags |= PIPE_BUF_FLAG_GIFT;

			pipe->nrbufs++;
			page_nr++;
			ret += buf->len;

			if (pipe->inode)
				do_wakeup = 1;

			if (!--spd->nr_pages)
				break;
			if (pipe->nrbufs < pipe->buffers)
				continue;

			break;
		}

		if (spd->flags & SPLICE_F_NONBLOCK) {
			if (!ret)
				ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			if (!ret)
				ret = -ERESTARTSYS;
			break;
		}

		if (do_wakeup) {
			smp_mb();
			if (waitqueue_active(&pipe->wait))
				wake_up_interruptible_sync(&pipe->wait);
			kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
			do_wakeup = 0;
		}

		pipe->waiting_writers++;
		pipe_wait(pipe);
		pipe->waiting_writers--;
	}

	pipe_unlock(pipe);

	if (do_wakeup)
		wakeup_pipe_readers(pipe);

	while (page_nr < spd_pages)
		spd->spd_release(spd, page_nr++);

	return ret;
}

void spd_release_page(struct splice_pipe_desc *spd, unsigned int i)
{
	page_cache_release(spd->pages[i]);
}

int splice_grow_spd(const struct pipe_inode_info *pipe, struct splice_pipe_desc *spd)
{
	unsigned int buffers = ACCESS_ONCE(pipe->buffers);

	spd->nr_pages_max = buffers;
	if (buffers <= PIPE_DEF_BUFFERS)
		return 0;

	spd->pages = kmalloc(buffers * sizeof(struct page *), GFP_KERNEL);
	spd->partial = kmalloc(buffers * sizeof(struct partial_page), GFP_KERNEL);

	if (spd->pages && spd->partial)
		return 0;

	kfree(spd->pages);
	kfree(spd->partial);
	return -ENOMEM;
}

void splice_shrink_spd(struct splice_pipe_desc *spd)
{
	if (spd->nr_pages_max <= PIPE_DEF_BUFFERS)
		return;

	kfree(spd->pages);
	kfree(spd->partial);
}

static int
__generic_file_splice_read(struct file *in, loff_t *ppos,
			   struct pipe_inode_info *pipe, size_t len,
			   unsigned int flags)
{
	struct address_space *mapping = in->f_mapping;
	unsigned int loff, nr_pages, req_pages;
	struct page *pages[PIPE_DEF_BUFFERS];
	struct partial_page partial[PIPE_DEF_BUFFERS];
	struct page *page;
	pgoff_t index, end_index;
	loff_t isize;
	int error, page_nr;
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages_max = PIPE_DEF_BUFFERS,
		.flags = flags,
		.ops = &page_cache_pipe_buf_ops,
		.spd_release = spd_release_page,
	};

	if (splice_grow_spd(pipe, &spd))
		return -ENOMEM;

	index = *ppos >> PAGE_CACHE_SHIFT;
	loff = *ppos & ~PAGE_CACHE_MASK;
	req_pages = (len + loff + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	nr_pages = min(req_pages, spd.nr_pages_max);

#if defined(MY_ABC_HERE) && defined(CONFIG_COMCERTO_SPLICE_READ_NOCONTIG)
	spd.nr_pages = find_get_pages(mapping, index, nr_pages, spd.pages);
#else
	spd.nr_pages = find_get_pages_contig(mapping, index, nr_pages, spd.pages);
#endif
	index += spd.nr_pages;

	if (spd.nr_pages < nr_pages)
		page_cache_sync_readahead(mapping, &in->f_ra, in,
				index, req_pages - spd.nr_pages);

	error = 0;
	while (spd.nr_pages < nr_pages) {
		 
		page = find_get_page(mapping, index);
		if (!page) {
			 
			page = page_cache_alloc_cold(mapping);
			if (!page)
				break;

			error = add_to_page_cache_lru(page, mapping, index,
						GFP_KERNEL);
			if (unlikely(error)) {
				page_cache_release(page);
				if (error == -EEXIST)
					continue;
				break;
			}
			 
			unlock_page(page);
		}

		spd.pages[spd.nr_pages++] = page;
		index++;
	}

	index = *ppos >> PAGE_CACHE_SHIFT;
	nr_pages = spd.nr_pages;
	spd.nr_pages = 0;
	for (page_nr = 0; page_nr < nr_pages; page_nr++) {
		unsigned int this_len;

		if (!len)
			break;

		this_len = min_t(unsigned long, len, PAGE_CACHE_SIZE - loff);
		page = spd.pages[page_nr];

		if (PageReadahead(page))
			page_cache_async_readahead(mapping, &in->f_ra, in,
					page, index, req_pages - page_nr);

		if (!PageUptodate(page)) {
			lock_page(page);

			if (!page->mapping) {
				unlock_page(page);
				page = find_or_create_page(mapping, index,
						mapping_gfp_mask(mapping));

				if (!page) {
					error = -ENOMEM;
					break;
				}
				page_cache_release(spd.pages[page_nr]);
				spd.pages[page_nr] = page;
			}
			 
			if (PageUptodate(page)) {
				unlock_page(page);
				goto fill_it;
			}

			error = mapping->a_ops->readpage(in, page);
			if (unlikely(error)) {
				 
				if (error == AOP_TRUNCATED_PAGE)
					error = 0;

				break;
			}
		}
fill_it:
		 
		isize = i_size_read(mapping->host);
		end_index = (isize - 1) >> PAGE_CACHE_SHIFT;
		if (unlikely(!isize || index > end_index))
			break;

		if (end_index == index) {
			unsigned int plen;

			plen = ((isize - 1) & ~PAGE_CACHE_MASK) + 1;
			if (plen <= loff)
				break;

			this_len = min(this_len, plen - loff);
			len = this_len;
		}

		spd.partial[page_nr].offset = loff;
		spd.partial[page_nr].len = this_len;
		len -= this_len;
		loff = 0;
		spd.nr_pages++;
		index++;
	}

	while (page_nr < nr_pages)
		page_cache_release(spd.pages[page_nr++]);
	in->f_ra.prev_pos = (loff_t)index << PAGE_CACHE_SHIFT;

	if (spd.nr_pages)
		error = splice_to_pipe(pipe, &spd);

	splice_shrink_spd(&spd);
	return error;
}

ssize_t generic_file_splice_read(struct file *in, loff_t *ppos,
				 struct pipe_inode_info *pipe, size_t len,
				 unsigned int flags)
{
	loff_t isize, left;
	int ret;

	isize = i_size_read(in->f_mapping->host);
	if (unlikely(*ppos >= isize))
		return 0;

	left = isize - *ppos;
	if (unlikely(left < len))
		len = left;

	ret = __generic_file_splice_read(in, ppos, pipe, len, flags);
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	if (ret > 0)
		*ppos += ret;
#else
	if (ret > 0) {
 		*ppos += ret;
		file_accessed(in);
	}
#endif

	return ret;
}
EXPORT_SYMBOL(generic_file_splice_read);

static const struct pipe_buf_operations default_pipe_buf_ops = {
	.can_merge = 0,
	.map = generic_pipe_buf_map,
	.unmap = generic_pipe_buf_unmap,
	.confirm = generic_pipe_buf_confirm,
	.release = generic_pipe_buf_release,
	.steal = generic_pipe_buf_steal,
	.get = generic_pipe_buf_get,
};

static ssize_t kernel_readv(struct file *file, const struct iovec *vec,
			    unsigned long vlen, loff_t offset)
{
	mm_segment_t old_fs;
	loff_t pos = offset;
	ssize_t res;

	old_fs = get_fs();
	set_fs(get_ds());
	 
	res = vfs_readv(file, (const struct iovec __user *)vec, vlen, &pos);
	set_fs(old_fs);

	return res;
}

static ssize_t kernel_write(struct file *file, const char *buf, size_t count,
			    loff_t pos)
{
	mm_segment_t old_fs;
	ssize_t res;

	old_fs = get_fs();
	set_fs(get_ds());
	 
	res = vfs_write(file, (const char __user *)buf, count, &pos);
	set_fs(old_fs);

	return res;
}

ssize_t default_file_splice_read(struct file *in, loff_t *ppos,
				 struct pipe_inode_info *pipe, size_t len,
				 unsigned int flags)
{
	unsigned int nr_pages;
	unsigned int nr_freed;
	size_t offset;
	struct page *pages[PIPE_DEF_BUFFERS];
	struct partial_page partial[PIPE_DEF_BUFFERS];
	struct iovec *vec, __vec[PIPE_DEF_BUFFERS];
	ssize_t res;
	size_t this_len;
	int error;
	int i;
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages_max = PIPE_DEF_BUFFERS,
		.flags = flags,
		.ops = &default_pipe_buf_ops,
		.spd_release = spd_release_page,
	};

	if (splice_grow_spd(pipe, &spd))
		return -ENOMEM;

	res = -ENOMEM;
	vec = __vec;
	if (spd.nr_pages_max > PIPE_DEF_BUFFERS) {
		vec = kmalloc(spd.nr_pages_max * sizeof(struct iovec), GFP_KERNEL);
		if (!vec)
			goto shrink_ret;
	}

	offset = *ppos & ~PAGE_CACHE_MASK;
	nr_pages = (len + offset + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;

	for (i = 0; i < nr_pages && i < spd.nr_pages_max && len; i++) {
		struct page *page;

		page = alloc_page(GFP_USER);
		error = -ENOMEM;
		if (!page)
			goto err;

		this_len = min_t(size_t, len, PAGE_CACHE_SIZE - offset);
		vec[i].iov_base = (void __user *) page_address(page);
		vec[i].iov_len = this_len;
		spd.pages[i] = page;
		spd.nr_pages++;
		len -= this_len;
		offset = 0;
	}

	res = kernel_readv(in, vec, spd.nr_pages, *ppos);
	if (res < 0) {
		error = res;
		goto err;
	}

	error = 0;
	if (!res)
		goto err;

	nr_freed = 0;
	for (i = 0; i < spd.nr_pages; i++) {
		this_len = min_t(size_t, vec[i].iov_len, res);
		spd.partial[i].offset = 0;
		spd.partial[i].len = this_len;
		if (!this_len) {
			__free_page(spd.pages[i]);
			spd.pages[i] = NULL;
			nr_freed++;
		}
		res -= this_len;
	}
	spd.nr_pages -= nr_freed;

	res = splice_to_pipe(pipe, &spd);
	if (res > 0)
		*ppos += res;

shrink_ret:
	if (vec != __vec)
		kfree(vec);
	splice_shrink_spd(&spd);
	return res;

err:
	for (i = 0; i < spd.nr_pages; i++)
		__free_page(spd.pages[i]);

	res = error;
	goto shrink_ret;
}
EXPORT_SYMBOL(default_file_splice_read);

static int pipe_to_sendpage(struct pipe_inode_info *pipe,
			    struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *file = sd->u.file;
	loff_t pos = sd->pos;
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	int ret, more;

	ret = buf->ops->confirm(pipe, buf);
	if (!ret) {
		more = (sd->flags & SPLICE_F_MORE) ? MSG_MORE : 0;
		if (sd->len < sd->total_len && pipe->nrbufs > 1)
			more |= MSG_SENDPAGE_NOTLAST;

		ret = file->f_op->sendpage(file, buf->page, buf->offset,
					   sd->len, &pos, more);
	}

	return ret;
#else
	int more;

	if (!likely(file->f_op && file->f_op->sendpage))
		return -EINVAL;

	more = (sd->flags & SPLICE_F_MORE) ? MSG_MORE : 0;

	if (sd->len < sd->total_len && pipe->nrbufs > 1)
		more |= MSG_SENDPAGE_NOTLAST;

	return file->f_op->sendpage(file, buf->page, buf->offset,
				    sd->len, &pos, more);
#endif
}

int pipe_to_file(struct pipe_inode_info *pipe, struct pipe_buffer *buf,
		 struct splice_desc *sd)
{
	struct file *file = sd->u.file;
	struct address_space *mapping = file->f_mapping;
	unsigned int offset, this_len;
	struct page *page;
	void *fsdata;
	int ret;

	offset = sd->pos & ~PAGE_CACHE_MASK;

	this_len = sd->len;
	if (this_len + offset > PAGE_CACHE_SIZE)
		this_len = PAGE_CACHE_SIZE - offset;

	ret = pagecache_write_begin(file, mapping, sd->pos, this_len,
				AOP_FLAG_UNINTERRUPTIBLE, &page, &fsdata);
	if (unlikely(ret))
		goto out;

	if (buf->page != page) {
		 
		char *src = buf->ops->map(pipe, buf, 1);
		char *dst = kmap_atomic(page, KM_USER1);

		memcpy(dst + offset, src + buf->offset, this_len);
		flush_dcache_page(page);
		kunmap_atomic(dst, KM_USER1);
		buf->ops->unmap(pipe, buf, src);
	}
	ret = pagecache_write_end(file, mapping, sd->pos, this_len, this_len,
				page, fsdata);
out:
	return ret;
}
EXPORT_SYMBOL(pipe_to_file);

static void wakeup_pipe_writers(struct pipe_inode_info *pipe)
{
	smp_mb();
	if (waitqueue_active(&pipe->wait))
		wake_up_interruptible(&pipe->wait);
	kill_fasync(&pipe->fasync_writers, SIGIO, POLL_OUT);
}

int splice_from_pipe_feed(struct pipe_inode_info *pipe, struct splice_desc *sd,
			  splice_actor *actor)
{
	int ret;

	while (pipe->nrbufs) {
		struct pipe_buffer *buf = pipe->bufs + pipe->curbuf;
		const struct pipe_buf_operations *ops = buf->ops;

		sd->len = buf->len;
		if (sd->len > sd->total_len)
			sd->len = sd->total_len;

		ret = buf->ops->confirm(pipe, buf);
		if (unlikely(ret)) {
			if (ret == -ENODATA)
				ret = 0;
			return ret;
		}

		ret = actor(pipe, buf, sd);
		if (ret <= 0)
			return ret;

		buf->offset += ret;
		buf->len -= ret;

		sd->num_spliced += ret;
		sd->len -= ret;
		sd->pos += ret;
		sd->total_len -= ret;

		if (!buf->len) {
			buf->ops = NULL;
			ops->release(pipe, buf);
			pipe->curbuf = (pipe->curbuf + 1) & (pipe->buffers - 1);
			pipe->nrbufs--;
			if (pipe->inode)
				sd->need_wakeup = true;
		}

		if (!sd->total_len)
			return 0;
	}

	return 1;
}
EXPORT_SYMBOL(splice_from_pipe_feed);

#if defined(MY_ABC_HERE) && defined(CONFIG_COMCERTO_IMPROVED_SPLICE)
#if !defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
#define MSPD_SPLICE_NUM_DMA		100
#else
#define MSPD_SPLICE_NUM_DMA		MDMA_OUTBOUND_BUF_DESC
#endif

#if defined(CONFIG_COMCERTO_SPLICE_PROF)
unsigned int enable_splice_prof = 0;
#endif

int comcerto_splice_from_pipe_feed(struct pipe_inode_info *pipe, struct splice_desc *sd)
{
	struct page **mspd_splice_pages;
	void **mspd_splice_fsdata;
	struct pipe_buffer *buf;
	const struct pipe_buf_operations *ops;
	int ret, ret2 = 0, remaining;
	unsigned int curbuf, nrbufs, len, nrbufs_len, done;
	loff_t pos, offset;
	struct file *file = sd->u.file;
	struct address_space *mapping = file->f_mapping;
	struct page **page;
	void **fsdata;
	unsigned int size;
#if !defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
	unsigned int buf_len, buf_offset;
	char *src, *dst;
#else
	struct comcerto_dma_sg *sg;
#endif

	size = (sizeof(struct page *) + sizeof(void *)) * MSPD_SPLICE_NUM_DMA;

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
	size = ALIGN(size, 8) + sizeof(struct comcerto_dma_sg);
#endif

	mspd_splice_pages = kmalloc(size, GFP_KERNEL);
	if (!mspd_splice_pages)
		return -ENOMEM;

	mspd_splice_fsdata = (void **)(mspd_splice_pages + MSPD_SPLICE_NUM_DMA);

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
	sg = (struct comcerto_dma_sg *)(mspd_splice_fsdata + MSPD_SPLICE_NUM_DMA);
	sg = PTR_ALIGN(sg, 8);
#endif

start:
#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
	comcerto_dma_sg_init(sg);
#endif

	nrbufs_len = 0;
	nrbufs = pipe->nrbufs;
	curbuf = pipe->curbuf;
	while (nrbufs) {
		buf = pipe->bufs + curbuf;

		ret = buf->ops->confirm(pipe, buf);
		if (unlikely(ret)) {
			printk(KERN_WARNING "%s: buf->ops->confirm() failed(%d)\n", __func__, ret);
			if (ret == -ENODATA)
				ret = 0;
			goto err;
		}

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
		 
		ret = comcerto_dma_sg_add_input(sg, page_address(buf->page) + buf->offset, buf->len, 0);
		if (unlikely(ret)) {
			printk(KERN_WARNING "%s: out of input bdescs\n", __func__);
			break;  
		}
#endif

		nrbufs_len += buf->len;

		if (nrbufs_len > sd->total_len) {
			nrbufs_len = sd->total_len;
			break;
		}

		if (nrbufs_len > (MSPD_SPLICE_NUM_DMA - 2)*PAGE_CACHE_SIZE) {
			nrbufs_len = (MSPD_SPLICE_NUM_DMA - 2)*PAGE_CACHE_SIZE;
			break;
		}
		curbuf = (curbuf + 1) & (pipe->buffers - 1);
		nrbufs--;
	}

	if (unlikely(nrbufs_len == 0)) {
		printk(KERN_WARNING "%s: nrbufs_len == 0\n", __func__);
		ret = 0;
		goto err;
	}

	page = &mspd_splice_pages[0];
	fsdata = &mspd_splice_fsdata[0];

	pos = sd->pos;
	offset = pos & ~PAGE_CACHE_MASK;
	len = nrbufs_len;

	if (likely(len + offset > PAGE_CACHE_SIZE))
		len = PAGE_CACHE_SIZE - offset;

	ret = pagecache_write_begin(file, mapping, pos, len,
			AOP_FLAG_UNINTERRUPTIBLE, page, fsdata);
	if (unlikely(ret))
		goto err;		 

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
	comcerto_dma_sg_add_output(sg, page_address(*page) + offset, len, 1);  
#endif

	pos += len;
	remaining = nrbufs_len - len;
	page++;
	fsdata++;

	while (remaining > PAGE_CACHE_SIZE) {
		ret = pagecache_write_begin(file, mapping, pos, PAGE_CACHE_SIZE,
				AOP_FLAG_UNINTERRUPTIBLE, page, fsdata);

		if (unlikely(ret))
			goto write_begin_done;

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
		ret = comcerto_dma_sg_add_output(sg, page_address(*page), PAGE_CACHE_SIZE, 1);
		if (unlikely(ret)) {
			pagecache_write_end(file, mapping, pos, PAGE_CACHE_SIZE, 0, *page, *fsdata);
			goto write_begin_done;
		}
#endif
		pos += PAGE_CACHE_SIZE;
		remaining -= PAGE_CACHE_SIZE;
		page++;
		fsdata++;
	}

	if (remaining) {
		ret = pagecache_write_begin(file, mapping, pos, remaining,
						AOP_FLAG_UNINTERRUPTIBLE, page, fsdata);

		if (unlikely(ret))
			goto write_begin_done;

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
		ret = comcerto_dma_sg_add_output(sg, page_address(*page), remaining, 1);
		if (unlikely(ret)) {
			pagecache_write_end(file, mapping, pos, remaining, 0, *page, *fsdata);
			goto write_begin_done;
		}
#endif
		remaining = 0;
	}

write_begin_done:
	 
	if (unlikely(remaining))
		nrbufs_len = nrbufs_len - remaining;

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)

	comcerto_dma_get();

	comcerto_dma_sg_setup(sg, nrbufs_len);

	comcerto_dma_start();
	comcerto_dma_wait();
	comcerto_dma_put();

	comcerto_dma_sg_cleanup(sg, nrbufs_len);
#else
	remaining = nrbufs_len;
	curbuf = pipe->curbuf;
	buf = pipe->bufs + curbuf;
	buf_len = buf->len;
	buf_offset = buf->offset;
	src = buf->ops->map(pipe, buf, 1);
	pos = sd->pos;
	offset = pos & ~PAGE_CACHE_MASK;
	page = &mspd_splice_pages[0];
	dst = kmap_atomic(*page, KM_USER1);

	while (remaining) {
		len = remaining;
		if (len + offset > PAGE_CACHE_SIZE)
			len = PAGE_CACHE_SIZE - offset;
		if (len > buf_len)
			len = buf_len;

		memcpy(dst + offset, src + buf_offset, len);

		buf_len -= len;
		buf_offset += len;
		remaining -= len;
		pos += len;
		offset = pos & ~PAGE_CACHE_MASK;

		if (!offset) {
			 
			flush_dcache_page(*page);
			kunmap_atomic(dst, KM_USER1);
			if (remaining) {
				page++;
				dst = kmap_atomic(*page, KM_USER1);
			}
		}

		if (!buf_len) {
			buf->ops->unmap(pipe, buf, src);
			if (remaining) {
				curbuf = (curbuf + 1) & (pipe->buffers - 1);
				buf = pipe->bufs + curbuf;
				buf_len = buf->len;
				buf_offset = buf->offset;
				src = buf->ops->map(pipe, buf, 1);
			}
		}
	}

	if (offset) {
		flush_dcache_page(*page);
		kunmap_atomic(dst, KM_USER1);
	}

	if (buf_len)
		buf->ops->unmap(pipe, buf, src);
#endif

	page = &mspd_splice_pages[0];
	fsdata = &mspd_splice_fsdata[0];
	offset = sd->pos & ~PAGE_CACHE_MASK;
	pos = sd->pos;
	remaining = nrbufs_len;
	len = nrbufs_len;
	done = 0;

	if (likely(len + offset > PAGE_CACHE_SIZE))
		len = PAGE_CACHE_SIZE - offset;

	ret = pagecache_write_end(file, mapping, pos, len, len,
			*page, *fsdata);

	if (unlikely(ret != len)) {
		printk(KERN_ERR "Failed on write_end, continuing with other buffers\n");

		ret2 = ret;
		nrbufs_len = (ret > 0) ? ret: 0;
	}

	pos += len;
	done += len;
	remaining -= len;

	page++;
	fsdata++;

	while (remaining > PAGE_CACHE_SIZE) {
		ret = pagecache_write_end(file, mapping, pos, PAGE_CACHE_SIZE, PAGE_CACHE_SIZE,
				*page, *fsdata);

		if (unlikely((ret != PAGE_CACHE_SIZE) && !ret2)) {
			printk(KERN_ERR "Failed on write_end, continuing with other buffers\n");

			nrbufs_len = done;

			if (ret >= 0)
				nrbufs_len += ret;

			ret2 = nrbufs_len;
		}

		pos += PAGE_CACHE_SIZE;
		done += PAGE_CACHE_SIZE;
		remaining -= PAGE_CACHE_SIZE;

		page++;
		fsdata++;
	}

	if (remaining) {
		ret = pagecache_write_end(file, mapping, pos, remaining, remaining,
					*page, *fsdata);

		if (unlikely((ret != remaining) && !ret2)) {
			printk(KERN_ERR "Failed on write_end, continuing with other buffers\n");

			nrbufs_len = done;

			if (ret >= 0)
				nrbufs_len += ret;

			ret2 = nrbufs_len;
		}
	}

	sd->num_spliced += nrbufs_len;
	sd->len -= nrbufs_len;
	sd->pos += nrbufs_len;
	sd->total_len -= nrbufs_len;

	remaining = nrbufs_len;
	buf = pipe->bufs + pipe->curbuf;

	while (remaining && (remaining >= buf->len)) {
		ops = buf->ops;

		remaining -= buf->len;
		buf->len = 0;
		buf->ops = NULL;
		ops->release(pipe, buf);
		pipe->nrbufs--;
		pipe->curbuf = (pipe->curbuf + 1) & (pipe->buffers - 1);
		buf = pipe->bufs + pipe->curbuf;
	}

	if (remaining) {
		buf->len -= remaining;
		buf->offset += remaining;
	}

	if (pipe->inode)
		sd->need_wakeup = true;

	if (!sd->total_len) {
		kfree(mspd_splice_pages);
		return 0;
	}

	if (ret2) {
		if (ret2 > 0)
			ret = 0;
		else
			ret = ret2;

		goto err;
	}

	if (pipe->nrbufs)
		goto start;

	ret = 1;

err:
	kfree(mspd_splice_pages);

	return ret;
}
EXPORT_SYMBOL(comcerto_splice_from_pipe_feed);
#endif

int splice_from_pipe_next(struct pipe_inode_info *pipe, struct splice_desc *sd)
{
	while (!pipe->nrbufs) {
		if (!pipe->writers)
			return 0;

		if (!pipe->waiting_writers && sd->num_spliced)
			return 0;

		if (sd->flags & SPLICE_F_NONBLOCK)
			return -EAGAIN;

		if (signal_pending(current))
			return -ERESTARTSYS;

		if (sd->need_wakeup) {
			wakeup_pipe_writers(pipe);
			sd->need_wakeup = false;
		}

		pipe_wait(pipe);
	}

	return 1;
}
EXPORT_SYMBOL(splice_from_pipe_next);

void splice_from_pipe_begin(struct splice_desc *sd)
{
	sd->num_spliced = 0;
	sd->need_wakeup = false;
}
EXPORT_SYMBOL(splice_from_pipe_begin);

void splice_from_pipe_end(struct pipe_inode_info *pipe, struct splice_desc *sd)
{
	if (sd->need_wakeup)
		wakeup_pipe_writers(pipe);
}
EXPORT_SYMBOL(splice_from_pipe_end);

ssize_t __splice_from_pipe(struct pipe_inode_info *pipe, struct splice_desc *sd,
			   splice_actor *actor)
{
	int ret;

	splice_from_pipe_begin(sd);
	do {
		ret = splice_from_pipe_next(pipe, sd);
		if (ret > 0)
			ret = splice_from_pipe_feed(pipe, sd, actor);
	} while (ret > 0);
	splice_from_pipe_end(pipe, sd);

	return sd->num_spliced ? sd->num_spliced : ret;
}
EXPORT_SYMBOL(__splice_from_pipe);

ssize_t splice_from_pipe(struct pipe_inode_info *pipe, struct file *out,
			 loff_t *ppos, size_t len, unsigned int flags,
			 splice_actor *actor)
{
	ssize_t ret;
	struct splice_desc sd = {
		.total_len = len,
		.flags = flags,
		.pos = *ppos,
		.u.file = out,
	};

	pipe_lock(pipe);
	ret = __splice_from_pipe(pipe, &sd, actor);
	pipe_unlock(pipe);

	return ret;
}

ssize_t
generic_file_splice_write(struct pipe_inode_info *pipe, struct file *out,
			  loff_t *ppos, size_t len, unsigned int flags)
{
	struct address_space *mapping = out->f_mapping;
	struct inode *inode = mapping->host;
	struct splice_desc sd = {
		.total_len = len,
		.flags = flags,
		.pos = *ppos,
		.u.file = out,
	};
	ssize_t ret;

	sb_start_write(inode->i_sb);
	pipe_lock(pipe);

	splice_from_pipe_begin(&sd);
	do {
		ret = splice_from_pipe_next(pipe, &sd);
		if (ret <= 0)
			break;

		mutex_lock_nested(&inode->i_mutex, I_MUTEX_CHILD);
		ret = file_remove_suid(out);

#if defined(MY_DEF_HERE)
		if (!ret)
			ret = splice_from_pipe_feed(pipe, &sd, pipe_to_file);
#else
		if (!ret) {
#if defined(MY_DEF_HERE)
#else
			ret = file_update_time(out);
#endif  
			if (!ret)
				ret = splice_from_pipe_feed(pipe, &sd,
							    pipe_to_file);
		}
#endif  
		mutex_unlock(&inode->i_mutex);
	} while (ret > 0);
	splice_from_pipe_end(pipe, &sd);

	pipe_unlock(pipe);

	if (sd.num_spliced)
		ret = sd.num_spliced;

	if (ret > 0) {
		int err;

		err = generic_write_sync(out, *ppos, ret);
		if (err)
			ret = err;
		else
			*ppos += ret;
		balance_dirty_pages_ratelimited(mapping);
	}
	sb_end_write(inode->i_sb);
	return ret;
}

EXPORT_SYMBOL(generic_file_splice_write);

#if defined(MY_ABC_HERE) && defined(CONFIG_COMCERTO_SPLICE_PROF)
unsigned int splicew_time_counter[256];
unsigned int splicew_reqtime_counter[256];
unsigned int splicew_data_counter[256];
static struct timeval last_splicew;
unsigned int init_splicew_prof = 0;
#endif

#if defined(MY_ABC_HERE) && defined(CONFIG_COMCERTO_IMPROVED_SPLICE)
ssize_t
comcerto_file_splice_write(struct pipe_inode_info *pipe, struct file *out,
			  loff_t *ppos, size_t len, unsigned int flags)
{
	struct address_space *mapping = out->f_mapping;
	struct inode *inode = mapping->host;
	struct splice_desc sd = {
		.total_len = len,
		.flags = flags,
		.pos = *ppos,
		.u.file = out,
	};
	ssize_t ret;
#if defined(CONFIG_COMCERTO_SPLICE_PROF)
	struct timeval now;
	int diff_time_ms;
#endif

#if defined(MY_ABC_HERE)
	sb_start_write(inode->i_sb);
#endif

	pipe_lock(pipe);

#if defined(CONFIG_COMCERTO_SPLICE_PROF)
	if (enable_splice_prof) {
		do_gettimeofday(&now);
		if (init_splicew_prof) {
			diff_time_ms = ((now.tv_sec - last_splicew.tv_sec) * 1000) + ((now.tv_usec - last_splicew.tv_usec) / 1000);
			if (diff_time_ms < 1000) {
				splicew_time_counter[diff_time_ms >> 3]++;
			}
			else {
				splicew_time_counter[255]++;
			}
		}
		last_splicew = now;
		if (len < (1 <<21))
			splicew_data_counter[(len >> 13) & 0xFF]++;
		else
			splicew_data_counter[255]++;
	}
#endif

	splice_from_pipe_begin(&sd);
	do {
		ret = splice_from_pipe_next(pipe, &sd);
		if (ret <= 0)
			break;

		mutex_lock_nested(&inode->i_mutex, I_MUTEX_CHILD);
		ret = file_remove_suid(out);
		if (!ret) {
			file_update_time(out);
			ret = comcerto_splice_from_pipe_feed(pipe, &sd);
		}
		mutex_unlock(&inode->i_mutex);
	} while (ret > 0);
	splice_from_pipe_end(pipe, &sd);

	pipe_unlock(pipe);

	if (sd.num_spliced)
		ret = sd.num_spliced;

	if (ret > 0) {
		unsigned long nr_pages;
		int err;

		nr_pages = (ret + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;

		err = generic_write_sync(out, *ppos, ret);
		if (err)
			ret = err;
		else
			*ppos += ret;
#if defined(MY_ABC_HERE)
		balance_dirty_pages_ratelimited(mapping);
#else
		balance_dirty_pages_ratelimited_nr(mapping, nr_pages);
#endif

	}

#if defined(CONFIG_COMCERTO_SPLICE_PROF)
	if (enable_splice_prof) {
		do_gettimeofday(&now);
		
		diff_time_ms = ((now.tv_sec - last_splicew.tv_sec) * 1000) + ((now.tv_usec - last_splicew.tv_usec) / 1000);
		if (diff_time_ms < 1000) { 
			splicew_reqtime_counter[diff_time_ms >> 3]++;
		}
		else
			splicew_reqtime_counter[255]++;

		if(!init_splicew_prof)
			init_splicew_prof = 1;

		last_splicew = now;
	}
#endif

#if defined(MY_ABC_HERE)
	sb_end_write(inode->i_sb);
#endif
	return ret;
}

EXPORT_SYMBOL(comcerto_file_splice_write);
#endif

static int write_pipe_buf(struct pipe_inode_info *pipe, struct pipe_buffer *buf,
			  struct splice_desc *sd)
{
	int ret;
	void *data;

	data = buf->ops->map(pipe, buf, 0);
	ret = kernel_write(sd->u.file, data + buf->offset, sd->len, sd->pos);
	buf->ops->unmap(pipe, buf, data);

	return ret;
}

static ssize_t default_file_splice_write(struct pipe_inode_info *pipe,
					 struct file *out, loff_t *ppos,
					 size_t len, unsigned int flags)
{
	ssize_t ret;

	ret = splice_from_pipe(pipe, out, ppos, len, flags, write_pipe_buf);
	if (ret > 0)
		*ppos += ret;

	return ret;
}

ssize_t generic_splice_sendpage(struct pipe_inode_info *pipe, struct file *out,
				loff_t *ppos, size_t len, unsigned int flags)
{
	return splice_from_pipe(pipe, out, ppos, len, flags, pipe_to_sendpage);
}

EXPORT_SYMBOL(generic_splice_sendpage);

#ifdef CONFIG_AUFS_FS
long do_splice_from(struct pipe_inode_info *pipe, struct file *out,
		    loff_t *ppos, size_t len, unsigned int flags)
#else  
static long do_splice_from(struct pipe_inode_info *pipe, struct file *out,
			   loff_t *ppos, size_t len, unsigned int flags)
#endif  
{
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *,
				loff_t *, size_t, unsigned int);
	int ret;

	if (unlikely(!(out->f_mode & FMODE_WRITE)))
		return -EBADF;

	if (unlikely(out->f_flags & O_APPEND))
		return -EINVAL;

	ret = rw_verify_area(WRITE, out, ppos, len);
	if (unlikely(ret < 0))
		return ret;

#if defined(MY_DEF_HERE)
	splice_write = out->f_op->splice_write;
	if (!splice_write)
#else
	if (out->f_op && out->f_op->splice_write)
		splice_write = out->f_op->splice_write;
	else
#endif
		splice_write = default_file_splice_write;

	return splice_write(pipe, out, ppos, len, flags);
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(do_splice_from);
#endif  

#ifdef CONFIG_AUFS_FS
long do_splice_to(struct file *in, loff_t *ppos,
		  struct pipe_inode_info *pipe, size_t len,
		  unsigned int flags)
#else  
static long do_splice_to(struct file *in, loff_t *ppos,
			 struct pipe_inode_info *pipe, size_t len,
			 unsigned int flags)
#endif  
{
	ssize_t (*splice_read)(struct file *, loff_t *,
			       struct pipe_inode_info *, size_t, unsigned int);
	int ret;

	if (unlikely(!(in->f_mode & FMODE_READ)))
		return -EBADF;

	ret = rw_verify_area(READ, in, ppos, len);
	if (unlikely(ret < 0))
		return ret;

#if defined(MY_DEF_HERE)
	splice_read = in->f_op->splice_read;
	if (!splice_read)
#else
	if (in->f_op && in->f_op->splice_read)
		splice_read = in->f_op->splice_read;
	else
#endif
		splice_read = default_file_splice_read;

	return splice_read(in, ppos, pipe, len, flags);
}
#ifdef CONFIG_AUFS_FS
EXPORT_SYMBOL(do_splice_to);
#endif  

ssize_t splice_direct_to_actor(struct file *in, struct splice_desc *sd,
			       splice_direct_actor *actor)
{
	struct pipe_inode_info *pipe;
	long ret, bytes;
	umode_t i_mode;
	size_t len;
	int i, flags;

	i_mode = in->f_path.dentry->d_inode->i_mode;
	if (unlikely(!S_ISREG(i_mode) && !S_ISBLK(i_mode)))
		return -EINVAL;

	pipe = current->splice_pipe;
	if (unlikely(!pipe)) {
		pipe = alloc_pipe_info(NULL);
		if (!pipe)
			return -ENOMEM;

		pipe->readers = 1;

		current->splice_pipe = pipe;
	}

	ret = 0;
	bytes = 0;
	len = sd->total_len;
	flags = sd->flags;

	sd->flags &= ~SPLICE_F_NONBLOCK;

	while (len) {
		size_t read_len;
		loff_t pos = sd->pos, prev_pos = pos;

		ret = do_splice_to(in, &pos, pipe, len, flags);
		if (unlikely(ret <= 0))
			goto out_release;

		read_len = ret;
		sd->total_len = read_len;

		ret = actor(pipe, sd);
		if (unlikely(ret <= 0)) {
			sd->pos = prev_pos;
			goto out_release;
		}

		bytes += ret;
		len -= ret;
		sd->pos = pos;

		if (ret < read_len) {
			sd->pos = prev_pos + ret;
			goto out_release;
		}
	}

done:
	pipe->nrbufs = pipe->curbuf = 0;
	file_accessed(in);
	return bytes;

out_release:
	 
	for (i = 0; i < pipe->buffers; i++) {
		struct pipe_buffer *buf = pipe->bufs + i;

		if (buf->ops) {
			buf->ops->release(pipe, buf);
			buf->ops = NULL;
		}
	}

	if (!bytes)
		bytes = ret;

	goto done;
}
EXPORT_SYMBOL(splice_direct_to_actor);

static int direct_splice_actor(struct pipe_inode_info *pipe,
			       struct splice_desc *sd)
{
	struct file *file = sd->u.file;

	return do_splice_from(pipe, file, &file->f_pos, sd->total_len,
			      sd->flags);
}

long do_splice_direct(struct file *in, loff_t *ppos, struct file *out,
		      size_t len, unsigned int flags)
{
	struct splice_desc sd = {
		.len		= len,
		.total_len	= len,
		.flags		= flags,
		.pos		= *ppos,
		.u.file		= out,
	};
	long ret;

	ret = splice_direct_to_actor(in, &sd, direct_splice_actor);
	if (ret > 0)
		*ppos = sd.pos;

	return ret;
}

static int splice_pipe_to_pipe(struct pipe_inode_info *ipipe,
			       struct pipe_inode_info *opipe,
			       size_t len, unsigned int flags);

static long do_splice(struct file *in, loff_t __user *off_in,
		      struct file *out, loff_t __user *off_out,
		      size_t len, unsigned int flags)
{
	struct pipe_inode_info *ipipe;
	struct pipe_inode_info *opipe;
	loff_t offset, *off;
	long ret;

	ipipe = get_pipe_info(in);
	opipe = get_pipe_info(out);

	if (ipipe && opipe) {
		if (off_in || off_out)
			return -ESPIPE;

		if (!(in->f_mode & FMODE_READ))
			return -EBADF;

		if (!(out->f_mode & FMODE_WRITE))
			return -EBADF;

		if (ipipe == opipe)
			return -EINVAL;

		return splice_pipe_to_pipe(ipipe, opipe, len, flags);
	}

	if (ipipe) {
		if (off_in)
			return -ESPIPE;
		if (off_out) {
			if (!(out->f_mode & FMODE_PWRITE))
				return -EINVAL;
			if (copy_from_user(&offset, off_out, sizeof(loff_t)))
				return -EFAULT;
			off = &offset;
		} else
			off = &out->f_pos;

		ret = do_splice_from(ipipe, out, off, len, flags);

		if (off_out && copy_to_user(off_out, off, sizeof(loff_t)))
			ret = -EFAULT;

		return ret;
	}

	if (opipe) {
		if (off_out)
			return -ESPIPE;
		if (off_in) {
			if (!(in->f_mode & FMODE_PREAD))
				return -EINVAL;
			if (copy_from_user(&offset, off_in, sizeof(loff_t)))
				return -EFAULT;
			off = &offset;
		} else
			off = &in->f_pos;

		ret = do_splice_to(in, off, opipe, len, flags);

		if (off_in && copy_to_user(off_in, off, sizeof(loff_t)))
			ret = -EFAULT;

		return ret;
	}

	return -EINVAL;
}

#ifdef MY_DEF_HERE
#include <net/sock.h>
struct RECV_FILE_CONTROL_BLOCK
{
    struct page *rv_page;
    loff_t rv_pos;
    size_t  rv_count;
    void *rv_fsdata;
};

static ssize_t do_splice_from_socket(struct file *file, struct socket *sock,
				     loff_t __user *ppos,size_t count)
{
	struct address_space *mapping = file->f_mapping;
	struct inode	*inode = mapping->host;
	loff_t pos;
	int count_tmp;
	int err = 0;
	int cPagePtr = 0;
	int cPagesAllocated = 0;
	struct RECV_FILE_CONTROL_BLOCK rv_cb[MAX_PAGES_PER_RECVFILE + 1];
	struct kvec iov[MAX_PAGES_PER_RECVFILE + 1];
	struct msghdr msg;
	long rcvtimeo;
	int ret;

	if(copy_from_user(&pos, ppos, sizeof(loff_t)))
		return -EFAULT;

	if(count > MAX_PAGES_PER_RECVFILE * PAGE_SIZE){
		printk("%s: count(%d) exceed maximum\n",__func__,count);
		return -EINVAL;
	}

	mutex_lock(&inode->i_mutex);

	current->backing_dev_info = mapping->backing_dev_info;

	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (err != 0 || count == 0)
		goto done;

	file_remove_suid(file);
	file_update_time(file);

	count_tmp = count;
	do {
		unsigned long bytes;	 
		unsigned long offset;	 
		struct page *pageP;
		void *fsdata;

		offset = (pos & (PAGE_CACHE_SIZE - 1));
		bytes = PAGE_CACHE_SIZE - offset;
		if (bytes > count_tmp)
			bytes = count_tmp;

		ret =  mapping->a_ops->write_begin(file, mapping, pos, bytes,
						   AOP_FLAG_UNINTERRUPTIBLE,
						   &pageP,&fsdata);

		if (unlikely(ret)) {
			err = ret;
			for(cPagePtr = 0; cPagePtr < cPagesAllocated; cPagePtr++) {
				kunmap(rv_cb[cPagePtr].rv_page);
				ret = mapping->a_ops->write_end(file, mapping,
								rv_cb[cPagePtr].rv_pos,
								rv_cb[cPagePtr].rv_count,
								rv_cb[cPagePtr].rv_count,
								rv_cb[cPagePtr].rv_page,
								rv_cb[cPagePtr].rv_fsdata);
			}
			goto done;
		}
		rv_cb[cPagesAllocated].rv_page = pageP;
		rv_cb[cPagesAllocated].rv_pos = pos;
		rv_cb[cPagesAllocated].rv_count = bytes;
		rv_cb[cPagesAllocated].rv_fsdata = fsdata;
		iov[cPagesAllocated].iov_base = kmap(pageP) + offset;
		iov[cPagesAllocated].iov_len = bytes;
		cPagesAllocated++;
		count_tmp -= bytes;
		pos += bytes;
	} while (count_tmp);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = (struct iovec *)&iov[0];
	msg.msg_iovlen = cPagesAllocated ;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = MSG_KERNSPACE;
	rcvtimeo = sock->sk->sk_rcvtimeo;
	sock->sk->sk_rcvtimeo = 8 * HZ;

	ret = kernel_recvmsg(sock, &msg, &iov[0], cPagesAllocated, count,
			     MSG_WAITALL | MSG_NOCATCHSIGNAL);

	sock->sk->sk_rcvtimeo = rcvtimeo;

	if(unlikely(ret < 0)) {
		err = ret;
		for(cPagePtr = 0; cPagePtr < cPagesAllocated; cPagePtr++){
			kunmap(rv_cb[cPagePtr].rv_page);
			ret = mapping->a_ops->write_end(file, mapping,
							rv_cb[cPagePtr].rv_pos,
							rv_cb[cPagePtr].rv_count,
							rv_cb[cPagePtr].rv_count,
							rv_cb[cPagePtr].rv_page,
							rv_cb[cPagePtr].rv_fsdata);
		}
		goto done;
	} else {
		err = 0;
		pos = pos - count + ret;
		count = ret;
	}

	for (cPagePtr=0;cPagePtr < cPagesAllocated;cPagePtr++) {
		 
		kunmap(rv_cb[cPagePtr].rv_page);
		ret = mapping->a_ops->write_end(file, mapping,
						rv_cb[cPagePtr].rv_pos,
						rv_cb[cPagePtr].rv_count,
						rv_cb[cPagePtr].rv_count,
						rv_cb[cPagePtr].rv_page,
						rv_cb[cPagePtr].rv_fsdata);

		if (unlikely(ret < 0))
			printk("%s: write_end fail,ret = %d\n",__func__,ret);
		 
	}
	balance_dirty_pages_ratelimited(mapping);
	copy_to_user(ppos,&pos,sizeof(loff_t));

done:
    current->backing_dev_info = NULL;
    mutex_unlock(&inode->i_mutex);

    if(err)
        return err;
    else
        return count;
}
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
 
typedef struct common_mempool common_mempool_t;
void* common_mempool_alloc(common_mempool_t* pool);
void common_mempool_free(common_mempool_t* pool, void* mem);
common_mempool_t* common_mempool_get(void* mem);
common_mempool_t*  common_mempool_create(uint32_t number_of_entries, uint32_t entry_size);
void  common_mempool_destroy(common_mempool_t* pool);
int32_t common_mempool_get_number_of_free_entries(common_mempool_t* pool);
int32_t common_mempool_get_number_of_entries(common_mempool_t* pool);
int32_t common_mempool_get_entry_size(common_mempool_t* pool);

#define COMMON_MPOOL_HDR_FLAGS_ALLOCATED 0x00000001
#define COMMON_MPOOL_HDR_MAGIC           0xa5a5a508
#define COMMON_MPOOL_FTR_MAGIC           0xa5a5a509
#define COMMON_MPOOL_ALIGN4(size) ((size)+4) & 0xFFFFFFFC;
#define COMMON_MPOOL_CHECK_ALIGNED4(ptr) ((((uint32_t)(ptr)) & 0x00000003) == 0)

typedef struct common_mpool_hdr
{
  struct common_mpool_hdr* next;
  common_mempool_t*        pool;
  uint32_t flags;
  uint32_t magic;
} common_mpool_hdr_t;

typedef struct
{
	uint32_t magic;
	common_mempool_t* pool;
} common_mpool_ftr_t;

struct common_mempool
{
	common_mpool_hdr_t*  head;
	common_mpool_hdr_t*  tail;
	uint32_t		number_of_free_entries;
	spinlock_t		lock;
	uint32_t                 data_size;  
	uint32_t                 pool_entry_size;  
	 
	uint32_t                 number_of_entries;
	uint32_t                 entry_size;
	uint8_t*                 mem;
};

bool common_mempool_check_internal(common_mempool_t * pool,
					void * ptr,
					common_mpool_hdr_t * hdr,
					common_mpool_ftr_t * ftr)
{
	if (!ptr) {
		printk(KERN_ERR "illegal ptr NULL");
		return false;
	}

	if (!COMMON_MPOOL_CHECK_ALIGNED4(ptr)) {
		printk(KERN_ERR "ptr not aligned %p",ptr);
		return false;
	}

	if (hdr->magic != COMMON_MPOOL_HDR_MAGIC) {
		printk(KERN_ERR "illegal hdr magic %x for ptr %p",hdr->magic,ptr);
		return false;
	}

	if (ftr->magic != COMMON_MPOOL_FTR_MAGIC) {
		printk(KERN_ERR "illegal ftr magic %x for ptr %p",ftr->magic,ptr);
		return false;
	}

	if (hdr->pool != pool || ftr->pool != pool) {
		printk(KERN_ERR "inconsistent size hdr->pool: %p ftr->pool: %p for ptr %p",hdr->pool,ftr->pool,ptr);
		return false;
	}

	if (!(hdr->flags & COMMON_MPOOL_HDR_FLAGS_ALLOCATED)) {
		printk(KERN_ERR "ptr %p was not allocated",ptr);
		return false;
	}
	return true;
}

void* common_mempool_alloc(common_mempool_t* pool)
{
	common_mpool_hdr_t* hdr;

	if (!pool || !pool->head || pool->number_of_free_entries == 0) {
		return NULL;
	}
	spin_lock_bh(&pool->lock);
	hdr = pool->head;
	pool->head = pool->head->next;

	if (!pool->head) {
		pool->tail = NULL;
	}

	hdr->flags = COMMON_MPOOL_HDR_FLAGS_ALLOCATED;
	pool->number_of_free_entries--;
	spin_unlock_bh(&pool->lock);
	return ((uint8_t*)hdr+sizeof(common_mpool_hdr_t));
}

void common_mempool_free(common_mempool_t* pool, void* ptr)
{
	common_mpool_hdr_t* hdr;
	common_mpool_ftr_t* ftr;

	if (!pool || !ptr) {
		return;
	}
	if (!COMMON_MPOOL_CHECK_ALIGNED4(ptr)) {
		printk(KERN_ERR "ptr not aligned %p",ptr);
		return;
	}
	spin_lock_bh(&pool->lock);
	hdr = (common_mpool_hdr_t*)((uint8_t*)ptr-sizeof(common_mpool_hdr_t));
	ftr = (common_mpool_ftr_t*)((uint8_t*)ptr+pool->data_size);

	if (!common_mempool_check_internal(pool,ptr,hdr,ftr)) {
		printk(KERN_ERR "invalid ptr %p",ptr);
		spin_unlock_bh(&pool->lock);
		return;
	}

	hdr->flags ^= COMMON_MPOOL_HDR_FLAGS_ALLOCATED;
	hdr->next = NULL;

	if (!pool->head) {
		pool->head = pool->tail = hdr;
	} else {
		pool->tail->next = hdr;
		pool->tail = hdr;
	}

	pool->number_of_free_entries++;
	spin_unlock_bh(&pool->lock);
}

common_mempool_t*  common_mempool_create(uint32_t number_of_entries,
						uint32_t entry_size)
{
	uint32_t i;
	uint32_t aligned_entry_size = COMMON_MPOOL_ALIGN4(entry_size);
	uint32_t pool_entry_size = COMMON_MPOOL_ALIGN4(sizeof(common_mpool_hdr_t)+aligned_entry_size+sizeof(common_mpool_ftr_t));
	common_mpool_hdr_t* hdr;
	common_mpool_hdr_t* next_hdr;
	common_mpool_ftr_t* ftr;
	common_mempool_t* pool;

	pool = kmalloc((sizeof(common_mempool_t) + pool_entry_size*number_of_entries), GFP_ATOMIC);

	if (!pool) {
		return NULL;
	}

	pool->entry_size = entry_size;
	pool->number_of_entries = number_of_entries;
	pool->data_size  = aligned_entry_size;
	pool->pool_entry_size = pool_entry_size;
	pool->number_of_free_entries = number_of_entries;
	pool->mem = (uint8_t*)(pool+1);
	pool->head = (common_mpool_hdr_t*)pool->mem;
	spin_lock_init(&pool->lock);

	for (i=0;i<number_of_entries;i++) {
		hdr = (common_mpool_hdr_t*)&pool->mem[pool_entry_size*i];
		ftr = (common_mpool_ftr_t*)((uint8_t*)hdr+sizeof(common_mpool_hdr_t)+aligned_entry_size);
		hdr->magic = COMMON_MPOOL_HDR_MAGIC;
		hdr->pool = pool;
		hdr->flags = 0;
		ftr->magic = COMMON_MPOOL_FTR_MAGIC;
		ftr->pool = pool;

		if (i < (number_of_entries-1)) {
			next_hdr = (common_mpool_hdr_t*)&pool->mem[pool_entry_size*(i+1)];
		} else {
			pool->tail = hdr;
			next_hdr = NULL;
		}

		hdr->next = next_hdr;
	}
	return pool;
}

void  common_mempool_destroy(common_mempool_t* pool)
{
	if (!pool) {
		return;
	}

	kfree(pool);
}

int32_t common_mempool_get_number_of_free_entries(common_mempool_t* pool)
{
	if (!pool) {
		return -1;
	}

	return (int32_t)pool->number_of_free_entries;
}

int32_t common_mempool_get_number_of_entries(common_mempool_t* pool)
{
	if (!pool) {
		return -1;
	}
	return (int32_t)pool->number_of_entries;
}

int32_t common_mempool_get_entry_size(common_mempool_t* pool)
{
	if (!pool) {
		return -1;
	}
	return (int32_t)pool->entry_size;
}

common_mempool_t* common_mempool_get(void* ptr)
{
	common_mpool_hdr_t* hdr;
	common_mpool_ftr_t* ftr;

	if (!ptr) {
		return NULL;
	}
	if (!COMMON_MPOOL_CHECK_ALIGNED4(ptr)) {
		return NULL;
	}
	hdr = (common_mpool_hdr_t*)((uint8_t*)ptr-sizeof(common_mpool_hdr_t));
	ftr = (common_mpool_ftr_t*)((uint8_t*)ptr + hdr->pool->data_size);

	if (hdr->magic != COMMON_MPOOL_HDR_MAGIC) {
		printk(KERN_ERR "illegal hdr magic %x for ptr %p",hdr->magic,ptr);
		return NULL;
	}
	if (ftr->magic != COMMON_MPOOL_FTR_MAGIC) {
		printk(KERN_ERR "illegal ftr magic %x for ptr %p",ftr->magic,ptr);
		return NULL;
	}
	if (hdr->pool != ftr->pool || !hdr->pool) {
		printk(KERN_ERR "inconsistent size hdr->pool: %p ftr->pool: %p for ptr %p",hdr->pool,ftr->pool,ptr);
		return false;
	}
	return hdr->pool;
}
 
ssize_t generic_splice_from_socket(struct file *file, struct socket *sock,
				     loff_t __user *ppos, size_t count)
{
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	loff_t pos;
	int count_tmp;
	int err = 0;
	int i = 0;
	int nr_pages = 0;
	int page_cnt_est= count/PAGE_SIZE + 1;
	struct recvfile_ctl_blk *rv_cb;
	struct kvec *iov;
	struct msghdr msg;
	long rcvtimeo;
	int ret;

	if (copy_from_user(&pos, ppos, sizeof(loff_t)))
		return -EFAULT;

	if (count > MAX_PAGES_PER_RECVFILE * PAGE_SIZE) {
		printk("%s: count(%u) exceeds maxinum\n", __func__, count);
		return -EINVAL;
	}
	mutex_lock(&inode->i_mutex);

	sb_start_write(inode->i_sb);

	current->backing_dev_info = mapping->backing_dev_info;

	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (err != 0 || count == 0)
		goto done;

	file_remove_suid(file);
	file_update_time(file);

	if (unlikely(!rcv_pool || !kvec_pool))
	{
		printk(KERN_ERR "rcv_pool %p kvec_pool %p uninitialized %d\n", rcv_pool, kvec_pool);
		sb_end_write(inode->i_sb);
		return -ENOMEM;
	}

	rv_cb = (struct recvfile_ctl_blk *)common_mempool_alloc(rcv_pool);
	iov = (struct kvec *)common_mempool_alloc(kvec_pool);

	if (!rv_cb || !iov)
	{
		printk(KERN_ERR "Failed to get pool mem for %d pages (rv_cb %p iov %p)\n", page_cnt_est, rv_cb, iov);
		sb_end_write(inode->i_sb);
		return -ENOMEM;
	}

	count_tmp = count;
	do {
		unsigned long bytes;	 
		unsigned long offset;	 
		struct page *pageP;
		void *fsdata;

		offset = (pos & (PAGE_CACHE_SIZE - 1));
		bytes = PAGE_CACHE_SIZE - offset;
		if (bytes > count_tmp)
			bytes = count_tmp;
		ret = mapping->a_ops->write_begin(file, mapping, pos, bytes,
						  AOP_FLAG_UNINTERRUPTIBLE,
						  &pageP, &fsdata);

		if (unlikely(ret)) {
			err = ret;
			goto cleanup;
		}

		rv_cb[nr_pages].rv_page = pageP;
		rv_cb[nr_pages].rv_pos = pos;
		rv_cb[nr_pages].rv_count = bytes;
		rv_cb[nr_pages].rv_fsdata = fsdata;
		iov[nr_pages].iov_base = kmap(pageP) + offset;
		iov[nr_pages].iov_len = bytes;
		nr_pages++;
		count_tmp -= bytes;
		pos += bytes;
	} while (count_tmp);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = (struct iovec *)&iov[0];
	msg.msg_iovlen = nr_pages ;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = MSG_KERNSPACE;
	rcvtimeo = sock->sk->sk_rcvtimeo;
	sock->sk->sk_rcvtimeo = 8 * HZ;

	ret = kernel_recvmsg(sock, &msg, &iov[0], nr_pages, count,
			     MSG_WAITALL | MSG_NOCATCHSIGNAL);

	sock->sk->sk_rcvtimeo = rcvtimeo;
	if(ret != count)
		err = -EPIPE;
	else
		err = 0;

	if (unlikely(err < 0)) {
		goto cleanup;
	}

	for(i=0,count=0;i < nr_pages;i++) {
		kunmap(rv_cb[i].rv_page);
		ret = mapping->a_ops->write_end(file, mapping,
						rv_cb[i].rv_pos,
						rv_cb[i].rv_count,
						rv_cb[i].rv_count,
						rv_cb[i].rv_page,
						rv_cb[i].rv_fsdata);
		if (unlikely(ret < 0))
			printk("%s: write_end fail,ret = %d\n", __func__, ret);
		count += rv_cb[i].rv_count;
	}
	balance_dirty_pages_ratelimited(mapping);

	if (copy_to_user(ppos, &pos, sizeof(loff_t)))
		err = -EFAULT;
done:
	current->backing_dev_info = NULL;
	common_mempool_free(rcv_pool, (void*)rv_cb);
	common_mempool_free(kvec_pool, (void*)iov);

	mutex_unlock(&inode->i_mutex);
	sb_end_write(inode->i_sb);
	return err ? err : count;
cleanup:
	for(i = 0; i < nr_pages; i++) {
		kunmap(rv_cb[i].rv_page);
		ret = mapping->a_ops->write_end(file, mapping,
						rv_cb[i].rv_pos,
						rv_cb[i].rv_count,
						rv_cb[i].rv_count,
						rv_cb[i].rv_page,
						rv_cb[i].rv_fsdata);
	}

	goto done;
}
#endif

static int get_iovec_page_array(const struct iovec __user *iov,
				unsigned int nr_vecs, struct page **pages,
				struct partial_page *partial, int aligned,
				unsigned int pipe_buffers)
{
	int buffers = 0, error = 0;

	while (nr_vecs) {
		unsigned long off, npages;
		struct iovec entry;
		void __user *base;
		size_t len;
		int i;

		error = -EFAULT;
		if (copy_from_user(&entry, iov, sizeof(entry)))
			break;

		base = entry.iov_base;
		len = entry.iov_len;

		error = 0;
		if (unlikely(!len))
			break;
		error = -EFAULT;
		if (!access_ok(VERIFY_READ, base, len))
			break;

		off = (unsigned long) base & ~PAGE_MASK;

		error = -EINVAL;
		if (aligned && (off || len & ~PAGE_MASK))
			break;

		npages = (off + len + PAGE_SIZE - 1) >> PAGE_SHIFT;
		if (npages > pipe_buffers - buffers)
			npages = pipe_buffers - buffers;

		error = get_user_pages_fast((unsigned long)base, npages,
					0, &pages[buffers]);

		if (unlikely(error <= 0))
			break;

		for (i = 0; i < error; i++) {
			const int plen = min_t(size_t, len, PAGE_SIZE - off);

			partial[buffers].offset = off;
			partial[buffers].len = plen;

			off = 0;
			len -= plen;
			buffers++;
		}

		if (len)
			break;

		if (error < npages || buffers == pipe_buffers)
			break;

		nr_vecs--;
		iov++;
	}

	if (buffers)
		return buffers;

	return error;
}

static int pipe_to_user(struct pipe_inode_info *pipe, struct pipe_buffer *buf,
			struct splice_desc *sd)
{
	char *src;
	int ret;

	if (!fault_in_pages_writeable(sd->u.userptr, sd->len)) {
		src = buf->ops->map(pipe, buf, 1);
		ret = __copy_to_user_inatomic(sd->u.userptr, src + buf->offset,
							sd->len);
		buf->ops->unmap(pipe, buf, src);
		if (!ret) {
			ret = sd->len;
			goto out;
		}
	}

	src = buf->ops->map(pipe, buf, 0);

	ret = sd->len;
	if (copy_to_user(sd->u.userptr, src + buf->offset, sd->len))
		ret = -EFAULT;

	buf->ops->unmap(pipe, buf, src);
out:
	if (ret > 0)
		sd->u.userptr += ret;
	return ret;
}

static long vmsplice_to_user(struct file *file, const struct iovec __user *iov,
			     unsigned long nr_segs, unsigned int flags)
{
	struct pipe_inode_info *pipe;
	struct splice_desc sd;
	ssize_t size;
	int error;
	long ret;

	pipe = get_pipe_info(file);
	if (!pipe)
		return -EBADF;

	pipe_lock(pipe);

	error = ret = 0;
	while (nr_segs) {
		void __user *base;
		size_t len;

		error = get_user(base, &iov->iov_base);
		if (unlikely(error))
			break;
		error = get_user(len, &iov->iov_len);
		if (unlikely(error))
			break;

		if (unlikely(!len))
			break;
		if (unlikely(!base)) {
			error = -EFAULT;
			break;
		}

		if (unlikely(!access_ok(VERIFY_WRITE, base, len))) {
			error = -EFAULT;
			break;
		}

		sd.len = 0;
		sd.total_len = len;
		sd.flags = flags;
		sd.u.userptr = base;
		sd.pos = 0;

		size = __splice_from_pipe(pipe, &sd, pipe_to_user);
		if (size < 0) {
			if (!ret)
				ret = size;

			break;
		}

		ret += size;

		if (size < len)
			break;

		nr_segs--;
		iov++;
	}

	pipe_unlock(pipe);

	if (!ret)
		ret = error;

	return ret;
}

static long vmsplice_to_pipe(struct file *file, const struct iovec __user *iov,
			     unsigned long nr_segs, unsigned int flags)
{
	struct pipe_inode_info *pipe;
	struct page *pages[PIPE_DEF_BUFFERS];
	struct partial_page partial[PIPE_DEF_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages_max = PIPE_DEF_BUFFERS,
		.flags = flags,
		.ops = &user_page_pipe_buf_ops,
		.spd_release = spd_release_page,
	};
	long ret;

	pipe = get_pipe_info(file);
	if (!pipe)
		return -EBADF;

	if (splice_grow_spd(pipe, &spd))
		return -ENOMEM;

	spd.nr_pages = get_iovec_page_array(iov, nr_segs, spd.pages,
					    spd.partial, flags & SPLICE_F_GIFT,
					    spd.nr_pages_max);
	if (spd.nr_pages <= 0)
		ret = spd.nr_pages;
	else
		ret = splice_to_pipe(pipe, &spd);

	splice_shrink_spd(&spd);
	return ret;
}

SYSCALL_DEFINE4(vmsplice, int, fd, const struct iovec __user *, iov,
		unsigned long, nr_segs, unsigned int, flags)
{
	struct file *file;
	long error;
	int fput;

	if (unlikely(nr_segs > UIO_MAXIOV))
		return -EINVAL;
	else if (unlikely(!nr_segs))
		return 0;

	error = -EBADF;
	file = fget_light(fd, &fput);
	if (file) {
		if (file->f_mode & FMODE_WRITE)
			error = vmsplice_to_pipe(file, iov, nr_segs, flags);
		else if (file->f_mode & FMODE_READ)
			error = vmsplice_to_user(file, iov, nr_segs, flags);

		fput_light(file, fput);
	}

	return error;
}

SYSCALL_DEFINE6(splice, int, fd_in, loff_t __user *, off_in,
		int, fd_out, loff_t __user *, off_out,
		size_t, len, unsigned int, flags)
{
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	int error;
#elif defined(MY_ABC_HERE)
	int error = -EBADF;
#else
	long error;
#endif
#if defined(MY_ABC_HERE)
	struct file *in, *out = NULL;
#else
	struct file *in, *out;
#endif
	int fput_in, fput_out;
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)  || defined(MY_ABC_HERE) || defined(MY_DEF_HERE)
	struct socket *sock = NULL;
#endif

	if (unlikely(!len))
		return 0;

#if !defined(MY_ABC_HERE)
	error = -EBADF;
#endif

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	 
#ifdef MY_DEF_HERE
	sock = sockfd_lookup(fd_in, (int *)&error);
#else
	sock = sockfd_lookup(fd_in, &error);
#endif
	if (sock) {
#ifdef MY_DEF_HERE
		if(!sock->sk) {
			BUG();
			goto done;
		}
#else
		out = NULL;
		if (!sock->sk)
			goto done;
#endif
		out = fget_light(fd_out, &fput_out);

		if (out) {
			if (!(out->f_mode & FMODE_WRITE))
				goto done;
#ifdef MY_DEF_HERE
			error = do_splice_from_socket(out, sock, off_out,len);
#else
			if (!out->f_op->splice_from_socket)
				goto done;
			error = out->f_op->splice_from_socket(out, sock, off_out, len);
#endif
		}
done:
		if(out)
			fput_light(out, fput_out);
		fput(sock->file);
		return error;
	}
#endif
#if defined(MY_ABC_HERE)
	if (!(out = fget_light(fd_out, &fput_out)))
		return -EBADF;

	if (!(out->f_mode & FMODE_WRITE))
		goto out;

	if (!get_pipe_info(out) &&
		(sock = sockfd_lookup(fd_in, &error))) {
#if defined(CONFIG_COMCERTO_IMPROVED_SPLICE)
		if (sock->sk && out->f_op->splice_from_socket)
			error = out->f_op->splice_from_socket(out, sock,
								off_out, len);
#endif
		fput(sock->file);
	} else
	{
		if (!(in = fget_light(fd_in, &fput_in)))
			goto out;
		if ((in->f_mode & FMODE_READ))
			error = do_splice(in, off_in, out, off_out, len, flags);

   		fput_light(in, fput_in);
   }

out:
	fput_light(out, fput_out);
#else
	in = fget_light(fd_in, &fput_in);
	if (in) {
		if (in->f_mode & FMODE_READ) {
			out = fget_light(fd_out, &fput_out);
			if (out) {
				if (out->f_mode & FMODE_WRITE)
					error = do_splice(in, off_in,
							  out, off_out,
							  len, flags);
 				fput_light(out, fput_out);
			}
		}

			fput_light(in, fput_in);
	}

#endif
	return error;
}

static int ipipe_prep(struct pipe_inode_info *pipe, unsigned int flags)
{
#if defined(MY_ABC_HERE)
	int ret = 0;
#else
	int ret;
#endif

	if (pipe->nrbufs)
		return 0;

#if !defined(MY_ABC_HERE)
	ret = 0;
#endif
	pipe_lock(pipe);

	while (!pipe->nrbufs) {
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}
		if (!pipe->writers)
			break;
		if (!pipe->waiting_writers) {
			if (flags & SPLICE_F_NONBLOCK) {
				ret = -EAGAIN;
				break;
			}
		}
		pipe_wait(pipe);
	}

	pipe_unlock(pipe);
	return ret;
}

static int opipe_prep(struct pipe_inode_info *pipe, unsigned int flags)
{
#if defined(MY_ABC_HERE)
	int ret = 0;
#else
	int ret;
#endif

	if (pipe->nrbufs < pipe->buffers)
		return 0;

#if !defined(MY_ABC_HERE)
	ret = 0;
#endif
	pipe_lock(pipe);

	while (pipe->nrbufs >= pipe->buffers) {
		if (!pipe->readers) {
			send_sig(SIGPIPE, current, 0);
			ret = -EPIPE;
			break;
		}
		if (flags & SPLICE_F_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}
		pipe->waiting_writers++;
		pipe_wait(pipe);
		pipe->waiting_writers--;
	}

	pipe_unlock(pipe);
	return ret;
}

static int splice_pipe_to_pipe(struct pipe_inode_info *ipipe,
			       struct pipe_inode_info *opipe,
			       size_t len, unsigned int flags)
{
	struct pipe_buffer *ibuf, *obuf;
	int ret = 0, nbuf;
	bool input_wakeup = false;

retry:
	ret = ipipe_prep(ipipe, flags);
	if (ret)
		return ret;

	ret = opipe_prep(opipe, flags);
	if (ret)
		return ret;

	pipe_double_lock(ipipe, opipe);

	do {
		if (!opipe->readers) {
			send_sig(SIGPIPE, current, 0);
			if (!ret)
				ret = -EPIPE;
			break;
		}

		if (!ipipe->nrbufs && !ipipe->writers)
			break;

		if (!ipipe->nrbufs || opipe->nrbufs >= opipe->buffers) {
			 
			if (ret)
				break;

			if (flags & SPLICE_F_NONBLOCK) {
				ret = -EAGAIN;
				break;
			}

			pipe_unlock(ipipe);
			pipe_unlock(opipe);
			goto retry;
		}

		ibuf = ipipe->bufs + ipipe->curbuf;
		nbuf = (opipe->curbuf + opipe->nrbufs) & (opipe->buffers - 1);
		obuf = opipe->bufs + nbuf;

		if (len >= ibuf->len) {
			 
			*obuf = *ibuf;
			ibuf->ops = NULL;
			opipe->nrbufs++;
			ipipe->curbuf = (ipipe->curbuf + 1) & (ipipe->buffers - 1);
			ipipe->nrbufs--;
			input_wakeup = true;
		} else {
			 
			ibuf->ops->get(ipipe, ibuf);
			*obuf = *ibuf;

			obuf->flags &= ~PIPE_BUF_FLAG_GIFT;

			obuf->len = len;
			opipe->nrbufs++;
			ibuf->offset += obuf->len;
			ibuf->len -= obuf->len;
		}
		ret += obuf->len;
		len -= obuf->len;
	} while (len);

	pipe_unlock(ipipe);
	pipe_unlock(opipe);

	if (ret > 0)
		wakeup_pipe_readers(opipe);

	if (input_wakeup)
		wakeup_pipe_writers(ipipe);

	return ret;
}

static int link_pipe(struct pipe_inode_info *ipipe,
		     struct pipe_inode_info *opipe,
		     size_t len, unsigned int flags)
{
	struct pipe_buffer *ibuf, *obuf;
	int ret = 0, i = 0, nbuf;

	pipe_double_lock(ipipe, opipe);

	do {
		if (!opipe->readers) {
			send_sig(SIGPIPE, current, 0);
			if (!ret)
				ret = -EPIPE;
			break;
		}

		if (i >= ipipe->nrbufs || opipe->nrbufs >= opipe->buffers)
			break;

		ibuf = ipipe->bufs + ((ipipe->curbuf + i) & (ipipe->buffers-1));
		nbuf = (opipe->curbuf + opipe->nrbufs) & (opipe->buffers - 1);

		ibuf->ops->get(ipipe, ibuf);

		obuf = opipe->bufs + nbuf;
		*obuf = *ibuf;

		obuf->flags &= ~PIPE_BUF_FLAG_GIFT;

		if (obuf->len > len)
			obuf->len = len;

		opipe->nrbufs++;
		ret += obuf->len;
		len -= obuf->len;
		i++;
	} while (len);

	if (!ret && ipipe->waiting_writers && (flags & SPLICE_F_NONBLOCK))
		ret = -EAGAIN;

	pipe_unlock(ipipe);
	pipe_unlock(opipe);

	if (ret > 0)
		wakeup_pipe_readers(opipe);

	return ret;
}

static long do_tee(struct file *in, struct file *out, size_t len,
		   unsigned int flags)
{
	struct pipe_inode_info *ipipe = get_pipe_info(in);
	struct pipe_inode_info *opipe = get_pipe_info(out);
	int ret = -EINVAL;

	if (ipipe && opipe && ipipe != opipe) {
		 
		ret = ipipe_prep(ipipe, flags);
		if (!ret) {
			ret = opipe_prep(opipe, flags);
			if (!ret)
				ret = link_pipe(ipipe, opipe, len, flags);
		}
	}

	return ret;
}

SYSCALL_DEFINE4(tee, int, fdin, int, fdout, size_t, len, unsigned int, flags)
{
	struct file *in;
	int error, fput_in;

	if (unlikely(!len))
		return 0;

	error = -EBADF;
	in = fget_light(fdin, &fput_in);
	if (in) {
		if (in->f_mode & FMODE_READ) {
			int fput_out;
			struct file *out = fget_light(fdout, &fput_out);

			if (out) {
				if (out->f_mode & FMODE_WRITE)
					error = do_tee(in, out, len, flags);
				fput_light(out, fput_out);
			}
		}
 		fput_light(in, fput_in);
 	}

	return error;
}

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
static int __init init_splice_pools(void)
{
	unsigned int rcv_pool_size= sizeof(struct recvfile_ctl_blk) * MAX_PAGES_PER_RECVFILE;
	unsigned int kve_pool_size= sizeof(struct kvec) * MAX_PAGES_PER_RECVFILE;

	rcv_pool =  common_mempool_create((8 * num_possible_cpus()), rcv_pool_size);
	kvec_pool = common_mempool_create((8 * num_possible_cpus()), kve_pool_size);
	if (!rcv_pool || !kvec_pool)
	{
		return -ENOMEM;
	}
 
}

fs_initcall(init_splice_pools);
#endif
