 
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/pagemap.h>
#include <linux/splice.h>
#include <linux/memcontrol.h>
#include <linux/mm_inline.h>
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/buffer_head.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/uio.h>
#include <linux/security.h>
#ifdef CONFIG_SYNO_QORIQ
#include <linux/socket.h>
#include <linux/net.h>
#ifdef CONFIG_SEND_PAGES
#include <net/tcp.h>
#include <linux/netdevice.h>
#endif  
#endif
#ifdef CONFIG_ARCH_FEROCEON
#include <net/sock.h>
#include "read_write.h"

#define WRITE_SKT_TO_FILE
#define WRITE_FROM_SOCK_TIMEOUT  8000

#ifdef COLLECT_WRITE_SOCK_TO_FILE_STAT
extern struct write_sock_to_file_stat write_from_sock;
#endif  
#endif  

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_SEND_PAGES
extern int is_sock_file(struct file *f);
#endif  

#define MAX_RECV_PAGES 8
struct receive_page {
     struct page *page;
     loff_t pos;
     size_t count;
     void *fsdata;
};
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

static const struct pipe_buf_operations page_cache_pipe_buf_ops = {
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

		if (pipe->nrbufs < PIPE_BUFFERS) {
			int newbuf = (pipe->curbuf + pipe->nrbufs) & (PIPE_BUFFERS - 1);
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
			if (pipe->nrbufs < PIPE_BUFFERS)
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

	if (do_wakeup) {
		smp_mb();
		if (waitqueue_active(&pipe->wait))
			wake_up_interruptible(&pipe->wait);
		kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
	}

	while (page_nr < spd_pages)
		spd->spd_release(spd, page_nr++);

	return ret;
}

static void spd_release_page(struct splice_pipe_desc *spd, unsigned int i)
{
	page_cache_release(spd->pages[i]);
}

static int
__generic_file_splice_read(struct file *in, loff_t *ppos,
			   struct pipe_inode_info *pipe, size_t len,
			   unsigned int flags)
{
	struct address_space *mapping = in->f_mapping;
	unsigned int loff, nr_pages, req_pages;
	struct page *pages[PIPE_BUFFERS];
	struct partial_page partial[PIPE_BUFFERS];
	struct page *page;
	pgoff_t index, end_index;
	loff_t isize;
	int error, page_nr;
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.flags = flags,
		.ops = &page_cache_pipe_buf_ops,
		.spd_release = spd_release_page,
	};

	index = *ppos >> PAGE_CACHE_SHIFT;
	loff = *ppos & ~PAGE_CACHE_MASK;
	req_pages = (len + loff + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	nr_pages = min(req_pages, (unsigned)PIPE_BUFFERS);

	spd.nr_pages = find_get_pages_contig(mapping, index, nr_pages, pages);
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
						mapping_gfp_mask(mapping));
			if (unlikely(error)) {
				page_cache_release(page);
				if (error == -EEXIST)
					continue;
				break;
			}
			 
			unlock_page(page);
		}

		pages[spd.nr_pages++] = page;
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
		page = pages[page_nr];

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_DELAY_ASYNC_READAHEAD
		if (!in->f_ra.delay_readahead && PageReadahead(page))
#else
		if (PageReadahead(page))
#endif
#else
		if (PageReadahead(page))
#endif
			page_cache_async_readahead(mapping, &in->f_ra, in,
					page, index, req_pages - page_nr);

		if (!PageUptodate(page)) {
			 
			if (flags & SPLICE_F_NONBLOCK) {
				if (!trylock_page(page)) {
					error = -EAGAIN;
					break;
				}
			} else
				lock_page(page);

			if (!page->mapping) {
				unlock_page(page);
				page = find_or_create_page(mapping, index,
						mapping_gfp_mask(mapping));

				if (!page) {
					error = -ENOMEM;
					break;
				}
				page_cache_release(pages[page_nr]);
				pages[page_nr] = page;
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

		partial[page_nr].offset = loff;
		partial[page_nr].len = this_len;
		len -= this_len;
		loff = 0;
		spd.nr_pages++;
		index++;
	}

	while (page_nr < nr_pages)
		page_cache_release(pages[page_nr++]);
	in->f_ra.prev_pos = (loff_t)index << PAGE_CACHE_SHIFT;

	if (spd.nr_pages)
		return splice_to_pipe(pipe, &spd);

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
	if (ret > 0) {
		*ppos += ret;
		file_accessed(in);
	}

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
	struct page *pages[PIPE_BUFFERS];
	struct partial_page partial[PIPE_BUFFERS];
	struct iovec vec[PIPE_BUFFERS];
	pgoff_t index;
	ssize_t res;
	size_t this_len;
	int error;
	int i;
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.flags = flags,
		.ops = &default_pipe_buf_ops,
		.spd_release = spd_release_page,
	};

	index = *ppos >> PAGE_CACHE_SHIFT;
	offset = *ppos & ~PAGE_CACHE_MASK;
	nr_pages = (len + offset + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;

	for (i = 0; i < nr_pages && i < PIPE_BUFFERS && len; i++) {
		struct page *page;

		page = alloc_page(GFP_USER);
		error = -ENOMEM;
		if (!page)
			goto err;

		this_len = min_t(size_t, len, PAGE_CACHE_SIZE - offset);
		vec[i].iov_base = (void __user *) page_address(page);
		vec[i].iov_len = this_len;
		pages[i] = page;
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
		partial[i].offset = 0;
		partial[i].len = this_len;
		if (!this_len) {
			__free_page(pages[i]);
			pages[i] = NULL;
			nr_freed++;
		}
		res -= this_len;
	}
	spd.nr_pages -= nr_freed;

	res = splice_to_pipe(pipe, &spd);
	if (res > 0)
		*ppos += res;

	return res;

err:
	for (i = 0; i < spd.nr_pages; i++)
		__free_page(pages[i]);

	return error;
}
EXPORT_SYMBOL(default_file_splice_read);

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_SEND_PAGES
static int pipe_to_sendpages(struct pipe_inode_info *pipe,
			    struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *file = sd->u.file;
	int ret, more;
	int page_index = 0;
	unsigned int tlen, len, offset;
	unsigned int curbuf = pipe->curbuf;
	struct page *pages[PIPE_BUFFERS];
	int nrbuf = pipe->nrbufs;
	int flags;
	struct socket *sock = file->private_data;

	sd->len = sd->total_len;
	tlen = 0;
	offset = buf->offset;

	while (nrbuf) {
		buf = pipe->bufs + curbuf;

		ret = buf->ops->confirm(pipe, buf);
		if (ret)
			break;

		pages[page_index] = buf->page;
		page_index++;
		len = (buf->len < sd->len) ? buf->len : sd->len;
		buf->offset += len;
		buf->len -= len;

		sd->num_spliced += len;
		sd->len -= len;
		sd->pos += len;
		sd->total_len -= len;
		tlen += len;

		if (!buf->len) {
			curbuf = (curbuf + 1) & (PIPE_BUFFERS - 1);
			nrbuf--;
		}
		if (!sd->total_len)
			break;
	}

	more = (sd->flags & SPLICE_F_MORE) || sd->len < sd->total_len;
	flags = !(file->f_flags & O_NONBLOCK) ? 0 : MSG_DONTWAIT;
	if (more)
		flags |= MSG_MORE;

	len = tcp_sendpages(sock, pages, offset, tlen, flags);

	if (!ret)
		ret = len;

	while (page_index) {
		page_index--;
		buf = pipe->bufs + pipe->curbuf;
		if (!buf->len) {
			buf->ops->release(pipe, buf);
			buf->ops = NULL;
			pipe->curbuf = (pipe->curbuf + 1) & (PIPE_BUFFERS - 1);
			pipe->nrbufs--;
			if (pipe->inode)
				sd->need_wakeup = true;
		}
	}

	return ret;
}
#endif 
#endif  

static int pipe_to_sendpage(struct pipe_inode_info *pipe,
			    struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *file = sd->u.file;
	loff_t pos = sd->pos;
	int ret, more;

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_SEND_PAGES
	struct socket *sock = file->private_data;

	if (is_sock_file(file) &&
		sock->ops->sendpage == tcp_sendpage){
		struct sock *sk = sock->sk;
		if ((sk->sk_route_caps & NETIF_F_SG) &&
			(sk->sk_route_caps & NETIF_F_ALL_CSUM))
			return pipe_to_sendpages(pipe, buf, sd);
	}
#endif
#endif
	ret = buf->ops->confirm(pipe, buf);
	if (!ret) {
		more = (sd->flags & SPLICE_F_MORE) || sd->len < sd->total_len;
#ifdef CONFIG_ARCH_FEROCEON
		if (file->f_op && file->f_op->sendpage)
#endif
			ret = file->f_op->sendpage(file, buf->page, buf->offset,
						   sd->len, &pos, more);
#ifdef CONFIG_ARCH_FEROCEON
		else
			ret = -EINVAL;
#endif
	}

	return ret;
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

	ret = buf->ops->confirm(pipe, buf);
	if (unlikely(ret))
		return ret;

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

		ret = actor(pipe, buf, sd);
#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_SEND_PAGES
		if (!sd->total_len)
			return 0;
		if (!pipe->nrbufs)
			break;
#endif  
#endif
		if (ret <= 0) {
			if (ret == -ENODATA)
				ret = 0;
			return ret;
		}
		buf->offset += ret;
		buf->len -= ret;

		sd->num_spliced += ret;
		sd->len -= ret;
		sd->pos += ret;
		sd->total_len -= ret;

		if (!buf->len) {
			buf->ops = NULL;
			ops->release(pipe, buf);
			pipe->curbuf = (pipe->curbuf + 1) & (PIPE_BUFFERS - 1);
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

	pipe_lock(pipe);

	splice_from_pipe_begin(&sd);
	do {
		ret = splice_from_pipe_next(pipe, &sd);
		if (ret <= 0)
			break;

		mutex_lock_nested(&inode->i_mutex, I_MUTEX_CHILD);
		ret = file_remove_suid(out);
		if (!ret) {
			file_update_time(out);
			ret = splice_from_pipe_feed(pipe, &sd, pipe_to_file);
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
		balance_dirty_pages_ratelimited_nr(mapping, nr_pages);
	}

	return ret;
}

EXPORT_SYMBOL(generic_file_splice_write);

static int write_pipe_buf(struct pipe_inode_info *pipe, struct pipe_buffer *buf,
			  struct splice_desc *sd)
{
	int ret;
	void *data;

	ret = buf->ops->confirm(pipe, buf);
	if (ret)
		return ret;

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

#ifdef CONFIG_ARCH_FEROCEON
	if (out->f_op && out->f_op->splice_write)
		splice_write = out->f_op->splice_write;
	else
#else
		splice_write = out->f_op->splice_write;
	if (!splice_write)
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

#ifdef SYNO_FORCE_UNMOUNT
	if (!blSynostate(O_UNMOUNT_OK, in)) {
		return -EBADF;
	}
#endif
	if (unlikely(!(in->f_mode & FMODE_READ)))
		return -EBADF;

	ret = rw_verify_area(READ, in, ppos, len);
	if (unlikely(ret < 0))
		return ret;

#ifdef CONFIG_ARCH_FEROCEON
	if (in->f_op && in->f_op->splice_read)
		splice_read = in->f_op->splice_read;
	else
#else
		splice_read = in->f_op->splice_read;
	if (!splice_read)
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
#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_DELAY_ASYNC_READAHEAD
	int nr_pages, index;
	struct page *pages[PIPE_BUFFERS];
	struct address_space *mapping = in->f_mapping;
#endif
#endif

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

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_DELAY_ASYNC_READAHEAD
		 
		if (in->f_op->splice_read == generic_file_splice_read)
			in->f_ra.delay_readahead = 1;
#endif  
#endif

		ret = do_splice_to(in, &pos, pipe, len, flags);
		if (unlikely(ret <= 0))
			goto out_release;

		read_len = ret;
		sd->total_len = read_len;

		ret = actor(pipe, sd);

#ifdef CONFIG_SYNO_QORIQ
#ifdef CONFIG_DELAY_ASYNC_READAHEAD
		 
		if (in->f_ra.delay_readahead) {
			index = sd->pos >> PAGE_CACHE_SHIFT;
			nr_pages = (read_len + (sd->pos & ~PAGE_CACHE_MASK)
					+ PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
			nr_pages = find_get_pages_contig(mapping, index, nr_pages, pages);
			for (i = 0; i < nr_pages; i++) {
				if (PageReadahead(pages[i])) {
					page_cache_async_readahead(mapping, &in->f_ra, in,
						pages[i], index, nr_pages - i);
				}
				index++;
				page_cache_release(pages[i]);
			}
			in->f_ra.delay_readahead = 0;
		}
#endif  
#endif

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
	 
	for (i = 0; i < PIPE_BUFFERS; i++) {
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

	return do_splice_from(pipe, file, &sd->pos, sd->total_len, sd->flags);
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
 
static inline struct pipe_inode_info *pipe_info(struct inode *inode)
{
	if (S_ISFIFO(inode->i_mode))
		return inode->i_pipe;

	return NULL;
}

static long do_splice(struct file *in, loff_t __user *off_in,
		      struct file *out, loff_t __user *off_out,
		      size_t len, unsigned int flags)
{
	struct pipe_inode_info *ipipe;
	struct pipe_inode_info *opipe;
	loff_t offset, *off;
	long ret;

	ipipe = pipe_info(in->f_path.dentry->d_inode);
	opipe = pipe_info(out->f_path.dentry->d_inode);

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
#ifdef CONFIG_ARCH_FEROCEON
			if (!out->f_op || !out->f_op->llseek ||
			    out->f_op->llseek == no_llseek)
#else
			if (out->f_op->llseek == no_llseek)
#endif
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
#ifdef CONFIG_ARCH_FEROCEON
			if (!in->f_op || !in->f_op->llseek ||
			    in->f_op->llseek == no_llseek)
#else
			if (in->f_op->llseek == no_llseek)
#endif
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

static int get_iovec_page_array(const struct iovec __user *iov,
				unsigned int nr_vecs, struct page **pages,
				struct partial_page *partial, int aligned)
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
		if (npages > PIPE_BUFFERS - buffers)
			npages = PIPE_BUFFERS - buffers;

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

		if (error < npages || buffers == PIPE_BUFFERS)
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

	ret = buf->ops->confirm(pipe, buf);
	if (unlikely(ret))
		return ret;

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

	pipe = pipe_info(file->f_path.dentry->d_inode);
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

#ifdef CONFIG_SYNO_QORIQ
static int socket_to_file(struct socket *sock, struct file *file,
		loff_t pos, size_t count)
{
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	struct receive_page pages[MAX_RECV_PAGES];
	struct kvec iov[MAX_RECV_PAGES];
	struct msghdr msg;
	unsigned long total_remains;
	unsigned long total_send = 0;
	int ret;
	int err = 0;
	int index = 0;
	int total_page = 0;
	int send_page = 0;

	mutex_lock(&inode->i_mutex);

	vfs_check_frozen(inode->i_sb, SB_FREEZE_WRITE);
	current->backing_dev_info = mapping->backing_dev_info;

	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));

	if (err != 0 || count == 0) {
		current->backing_dev_info = NULL;
		mutex_unlock(&inode->i_mutex);
		return -EFAULT;
	}

	file_remove_suid(file);
	file_update_time(file);

	total_remains = count;
	do {
		send_page = 0;
		do {
			unsigned long bytes;
			unsigned long offset;
			struct page *page;
			void *fsdata;

			offset = (pos & (PAGE_CACHE_SIZE - 1));
			bytes = min_t(unsigned long, PAGE_CACHE_SIZE - offset,
					total_remains);

			ret =  mapping->a_ops->write_begin(file, mapping, pos,
				bytes, AOP_FLAG_UNINTERRUPTIBLE, &page, &fsdata);

			if (unlikely(ret)) {
				err = -EFAULT;
				break;
			}

			pages[send_page].page = page;
			pages[send_page].pos = pos;
			pages[send_page].count = bytes;
			pages[send_page].fsdata = fsdata;
			iov[send_page].iov_base = kmap(page) + offset;
			iov[send_page].iov_len = bytes;
			send_page++;
			total_remains -= bytes;
			pos += bytes;
		} while (total_remains && send_page != MAX_RECV_PAGES);

		total_page += send_page;
		 
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = (struct iovec *)&iov[0];
		msg.msg_iovlen = send_page;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = MSG_WAITALL;
		total_send += kernel_recvmsg(sock, &msg, &iov[0], send_page,
			(count - total_remains - total_send), MSG_WAITALL);

		if ((total_send + total_remains) != count)
			err = -EPIPE;
		else
			err = total_send;

		for (index = 0; index < send_page; index++) {
			mark_page_accessed(pages[index].page);
			kunmap(pages[index].page);
			ret = mapping->a_ops->write_end(file, mapping,
			pages[index].pos, pages[index].count, pages[index].count,
			pages[index].page, pages[index].fsdata);
			if (unlikely(ret < 0)) {
				err = ret;
				break;
			}
		}

		if (err < 0)
			break;

	} while (total_remains);

	if (err > 0)
		balance_dirty_pages_ratelimited_nr(mapping, total_page);

	current->backing_dev_info = NULL;
	mutex_unlock(&inode->i_mutex);

	return err;
}
#endif  

static long vmsplice_to_pipe(struct file *file, const struct iovec __user *iov,
			     unsigned long nr_segs, unsigned int flags)
{
	struct pipe_inode_info *pipe;
	struct page *pages[PIPE_BUFFERS];
	struct partial_page partial[PIPE_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.flags = flags,
		.ops = &user_page_pipe_buf_ops,
		.spd_release = spd_release_page,
	};

	pipe = pipe_info(file->f_path.dentry->d_inode);
	if (!pipe)
		return -EBADF;

	spd.nr_pages = get_iovec_page_array(iov, nr_segs, pages, partial,
					    flags & SPLICE_F_GIFT);
	if (spd.nr_pages <= 0)
		return spd.nr_pages;

	return splice_to_pipe(pipe, &spd);
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
#ifdef CONFIG_ARCH_FEROCEON
	long error, timeout;
	struct file *in, *out = NULL;
	struct socket *socket_struct = NULL;
#else
	long error;
	struct file *in, *out;
#endif
	int fput_in, fput_out;
#ifdef CONFIG_SYNO_QORIQ
	struct socket *sock = NULL;
#endif

	if (unlikely(!len))
		return 0;

	error = -EBADF;

#ifdef CONFIG_ARCH_FEROCEON
	 
	out = fget_light(fd_out, &fput_out);
	if (out) {
		if (!(out->f_mode & FMODE_WRITE)) {
			fput_light(out, fput_out);
			dprintk("FMODE_WRITE flag is not set in out->f_mode\n");
			INC_WRITE_FROM_SOCK_ERR_CNT;
			return error;
		}
	} else {
		dprintk("Failed to get \"out\" file structire\n");
		INC_WRITE_FROM_SOCK_ERR_CNT;
		return error;
	}
#else
#ifdef CONFIG_SYNO_QORIQ
	sock = sockfd_lookup(fd_in, &error);
	if (sock) {
		loff_t pos;
		out = NULL;

		if (!sock->sk)
			return error;

		if (copy_from_user(&pos, off_out, sizeof(loff_t)))
			return -EFAULT;

		out = fget_light(fd_out, &fput_out);
		if (out) {
			if (out->f_mode & FMODE_WRITE)
				error = socket_to_file(sock, out, pos, len);
			fput_light(out, fput_out);
		}

		fput(sock->file);
		return error;
	}
#endif

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
#endif  

#ifdef CONFIG_ARCH_FEROCEON
#ifdef WRITE_SKT_TO_FILE
     
	socket_struct = sockfd_lookup(fd_in, (int*)&error);
	 
	if (socket_struct) {
		 
		if(len > WRITE_RECEIVE_SIZE || !socket_struct->sk) {
			if (socket_struct->sk != 0)
				dprintk("Bad length (%d)\n", len);
			else
				dprintk("Bad socket (socket_struct->sk == 0)\n");
			INC_WRITE_FROM_SOCK_ERR_CNT;
        	error = -EINVAL;
		} else {
			timeout = socket_struct->sk->sk_rcvtimeo;
			 
			socket_struct->sk->sk_rcvtimeo = msecs_to_jiffies(WRITE_FROM_SOCK_TIMEOUT);
			error = write_from_socket_to_file(socket_struct, out, off_out, len);
			socket_struct->sk->sk_rcvtimeo = timeout;
		} 
		fput(socket_struct->file);
	} else {
#endif
		in = fget_light(fd_in, &fput_in);
		if (in) {
			if (in->f_mode & FMODE_READ) {
				error = do_splice(in, off_in, out, off_out, len, flags);
			}
#endif  
			fput_light(in, fput_in);
		}
#ifdef CONFIG_ARCH_FEROCEON
#ifdef WRITE_SKT_TO_FILE
	}
#endif
	fput_light(out, fput_out);
#endif  
	return error;
}

static int ipipe_prep(struct pipe_inode_info *pipe, unsigned int flags)
{
	int ret;

	if (pipe->nrbufs)
		return 0;

	ret = 0;
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
	int ret;

	if (pipe->nrbufs < PIPE_BUFFERS)
		return 0;

	ret = 0;
	pipe_lock(pipe);

	while (pipe->nrbufs >= PIPE_BUFFERS) {
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

		if (!ipipe->nrbufs || opipe->nrbufs >= PIPE_BUFFERS) {
			 
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
		nbuf = (opipe->curbuf + opipe->nrbufs) % PIPE_BUFFERS;
		obuf = opipe->bufs + nbuf;

		if (len >= ibuf->len) {
			 
			*obuf = *ibuf;
			ibuf->ops = NULL;
			opipe->nrbufs++;
			ipipe->curbuf = (ipipe->curbuf + 1) % PIPE_BUFFERS;
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

	if (ret > 0) {
		smp_mb();
		if (waitqueue_active(&opipe->wait))
			wake_up_interruptible(&opipe->wait);
		kill_fasync(&opipe->fasync_readers, SIGIO, POLL_IN);
	}
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

		if (i >= ipipe->nrbufs || opipe->nrbufs >= PIPE_BUFFERS)
			break;

		ibuf = ipipe->bufs + ((ipipe->curbuf + i) & (PIPE_BUFFERS - 1));
		nbuf = (opipe->curbuf + opipe->nrbufs) & (PIPE_BUFFERS - 1);

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

	if (ret > 0) {
		smp_mb();
		if (waitqueue_active(&opipe->wait))
			wake_up_interruptible(&opipe->wait);
		kill_fasync(&opipe->fasync_readers, SIGIO, POLL_IN);
	}

	return ret;
}

static long do_tee(struct file *in, struct file *out, size_t len,
		   unsigned int flags)
{
	struct pipe_inode_info *ipipe = pipe_info(in->f_path.dentry->d_inode);
	struct pipe_inode_info *opipe = pipe_info(out->f_path.dentry->d_inode);
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
