#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/dmaengine.h>
#include <linux/socket.h>
#include <linux/export.h>
#include <net/tcp.h>
#include <net/netdma.h>

#ifdef MY_DEF_HERE
#define NET_DMA_DEFAULT_COPYBREAK  8192  
#else
#define NET_DMA_DEFAULT_COPYBREAK 4096
#endif

int sysctl_tcp_dma_copybreak = NET_DMA_DEFAULT_COPYBREAK;
EXPORT_SYMBOL(sysctl_tcp_dma_copybreak);

int dma_skb_copy_datagram_iovec(struct dma_chan *chan,
			struct sk_buff *skb, int offset, struct iovec *to,
			size_t len, struct dma_pinned_list *pinned_list)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
#ifdef MY_DEF_HERE
 
#else
	struct sk_buff *frag_iter;
#endif
	dma_cookie_t cookie = 0;
#ifdef MY_DEF_HERE
	struct sg_table	*dst_sgt;
	struct sg_table	*src_sgt;
	struct scatterlist *dst_sg;
	int	dst_sg_len;
	struct scatterlist *src_sg;
	int		src_sg_len = skb_shinfo(skb)->nr_frags;
	size_t	dst_len = len;
	size_t	dst_offset = offset;
	int ret;
#ifdef MY_DEF_HERE
	int retry_limit = 10;
#endif

	pr_debug("%s %d copy %d len %d nr_iovecs %d skb frags %d\n",
			__func__, __LINE__, copy, len, pinned_list->nr_iovecs,
			src_sg_len);

	dst_sgt = pinned_list->sgts;

	if (copy > 0)
		src_sg_len += 1;

	src_sgt = pinned_list->sgts + 1;

	dst_sg = dst_sgt->sgl;
	src_sg = src_sgt->sgl;
	src_sg_len = 0;
#endif

	if (copy > 0) {
		if (copy > len)
			copy = len;
#ifdef MY_DEF_HERE
		sg_set_buf(src_sg, skb->data + offset, copy);
		pr_debug("%s %d: add src buf page %p. addr %p len 0x%x\n", __func__,
				__LINE__, virt_to_page(skb->data),
				skb->data, copy);

		len -= copy;
		src_sg_len++;
		if (len == 0)
			goto fill_dst_sg;
		offset += copy;
		src_sg = sg_next(src_sg);
#else
		cookie = dma_memcpy_to_iovec(chan, to, pinned_list,
					    skb->data + offset, copy);
		if (cookie < 0)
			goto fault;
		len -= copy;
		if (len == 0)
			goto end;
		offset += copy;
#endif
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		copy = end - offset;
		if (copy > 0) {
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;

#ifdef MY_DEF_HERE
			sg_set_page(src_sg, page, copy, frag->page_offset +
					offset - start);
			pr_debug("%s %d: add src buf [%d] page %p. len 0x%x\n", __func__,
				__LINE__, i, page, copy);
			src_sg = sg_next(src_sg);
			src_sg_len++;
#else
			cookie = dma_memcpy_pg_to_iovec(chan, to, pinned_list, page,
					frag->page_offset + offset - start, copy);
			if (cookie < 0)
				goto fault;
#endif
			len -= copy;
			if (len == 0)
#ifdef MY_DEF_HERE
				break;
#else
				goto end;
#endif
			offset += copy;
		}
		start = end;
	}

#ifdef MY_DEF_HERE
fill_dst_sg:

	if (src_sg_len > 0) {
		dst_sg_len = dma_memcpy_fill_sg_from_iovec(chan, to, pinned_list, dst_sg, dst_offset, dst_len);
		BUG_ON(dst_sg_len <= 0);

#ifdef MY_DEF_HERE
retry:
#endif
		cookie = dma_async_memcpy_sg_to_sg(chan,
						dst_sgt->sgl,
						dst_sg_len,
						src_sgt->sgl,
						src_sg_len);
#ifdef MY_DEF_HERE
		if (cookie == -ENOMEM) {
			if (--retry_limit > 0) {
				udelay(50);
				goto retry;
			} else {
				printk(KERN_ERR "Cannot retrieve DMA buffer!\n");
			}
		}
#endif
	}
#else
	skb_walk_frags(skb, frag_iter) {
		int end;

		WARN_ON(start > offset + len);

		end = start + frag_iter->len;
		copy = end - offset;
		if (copy > 0) {
			if (copy > len)
				copy = len;
			cookie = dma_skb_copy_datagram_iovec(chan, frag_iter,
							     offset - start,
							     to, copy,
							     pinned_list);
			if (cookie < 0)
				goto fault;
			len -= copy;
			if (len == 0)
				goto end;
			offset += copy;
		}
		start = end;
	}
#endif

#ifdef MY_DEF_HERE
 
#else
end:
#endif
	if (!len) {
		skb->dma_cookie = cookie;
		return cookie;
	}

#ifdef MY_DEF_HERE
 
#else
fault:
#endif
	return -EFAULT;
}
