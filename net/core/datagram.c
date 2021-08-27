#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/poll.h>
#include <linux/highmem.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <net/protocol.h>
#include <linux/skbuff.h>

#include <net/checksum.h>
#include <net/sock.h>
#include <net/tcp_states.h>
#include <trace/events/skb.h>

static inline int connection_based(struct sock *sk)
{
	return sk->sk_type == SOCK_SEQPACKET || sk->sk_type == SOCK_STREAM;
}

static int receiver_wake_function(wait_queue_t *wait, unsigned mode, int sync,
				  void *key)
{
	unsigned long bits = (unsigned long)key;

	if (bits && !(bits & (POLLIN | POLLERR)))
		return 0;
	return autoremove_wake_function(wait, mode, sync, key);
}
 
static int wait_for_packet(struct sock *sk, int *err, long *timeo_p)
{
	int error;
	DEFINE_WAIT_FUNC(wait, receiver_wake_function);

	prepare_to_wait_exclusive(sk_sleep(sk), &wait, TASK_INTERRUPTIBLE);

	error = sock_error(sk);
	if (error)
		goto out_err;

	if (!skb_queue_empty(&sk->sk_receive_queue))
		goto out;

	if (sk->sk_shutdown & RCV_SHUTDOWN)
		goto out_noerr;

	error = -ENOTCONN;
	if (connection_based(sk) &&
	    !(sk->sk_state == TCP_ESTABLISHED || sk->sk_state == TCP_LISTEN))
		goto out_err;

	if (signal_pending(current))
		goto interrupted;

	error = 0;
	*timeo_p = schedule_timeout(*timeo_p);
out:
	finish_wait(sk_sleep(sk), &wait);
	return error;
interrupted:
	error = sock_intr_errno(*timeo_p);
out_err:
	*err = error;
	goto out;
out_noerr:
	*err = 0;
	error = 1;
	goto out;
}

#if defined(MY_ABC_HERE) && defined(CONFIG_COMCERTO_IMPROVED_SPLICE)
 
#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
int skb_copy_datagram_to_kernel_iovec_soft(const struct sk_buff *skb, int offset,
				      struct iovec *to, int len)
#else
int skb_copy_datagram_to_kernel_iovec(const struct sk_buff *skb, int offset,
				      struct iovec *to, int len)
#endif
{
	int i, fraglen, end = 0;
	struct sk_buff *next = skb_shinfo(skb)->frag_list;

	if (!len)
		return 0;

next_skb:
	fraglen = skb_headlen(skb);
	i = -1;

	while (1) {
		int start = end;

		if ((end += fraglen) > offset) {
			int copy = end - offset;
			int o = offset - start;

			if (copy > len)
				copy = len;
			if (i == -1)
				memcpy_tokerneliovec(to, skb->data + o, copy);
			else {
				skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
				struct page *page = skb_frag_page(frag);
				void *p = kmap(page) + frag->page_offset + o;
				memcpy_tokerneliovec(to, p, copy);
				kunmap(page);
			}

			if (!(len -= copy))
				return 0;
			offset += copy;
		}
		if (++i >= skb_shinfo(skb)->nr_frags)
			break;
		fraglen = skb_shinfo(skb)->frags[i].size;
	}
	if (next) {
		skb = next;
		BUG_ON(skb_shinfo(skb)->frag_list);
		next = skb->next;
		goto next_skb;
	}

	return -EFAULT;
}

#if defined(CONFIG_COMCERTO_SPLICE_USE_MDMA)
#include <mach/hardware.h>
#include <mach/dma.h>
int skb_copy_datagram_to_kernel_iovec(const struct sk_buff *skb, int offset,
				      struct iovec *to, int len)
{
	int i, ret, fraglen, copy, o, end = 0;
	struct sk_buff *next = skb_shinfo(skb)->frag_list;

	struct comcerto_dma_sg *sg;
	unsigned int size;
	int total_len, input_len;
	if (!len)
		return 0;

	total_len = len;
	
	size = sizeof(struct comcerto_dma_sg);
	sg = kmalloc(size, GFP_ATOMIC);
	if (!sg)
		return skb_copy_datagram_to_kernel_iovec_soft (skb, offset, to, len);

	comcerto_dma_sg_init(sg);

next_skb:
	fraglen = skb_headlen(skb);
	i = -1;

	while (1) {
		int start = end;

		if ((end += fraglen) > offset) {
current_frag:
			copy = end - offset;
			o = offset - start;

			if (copy > len)
				copy = len;
			
			if (i == -1) {
				ret = comcerto_dma_sg_add_input(sg, skb->data + o, copy, 0);
			} else {
				skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
				struct page *page = skb_frag_page(frag);
				void *p = page_address(page) + frag->page_offset + o;
				ret = comcerto_dma_sg_add_input(sg, p, copy, 0);
			}
			if (likely(ret == 0)) {
				len -= copy;
				offset += copy;
			}

			if ((len == 0) || unlikely(ret))
			{
				input_len = total_len - len;
				len = input_len;
				 
				while (len > 0) {
					if (to->iov_len) {
						int copy = min_t(unsigned int, to->iov_len, len);

						ret = comcerto_dma_sg_add_output(sg, to->iov_base, copy, 1);
						if (unlikely(ret)) {
							 
							comcerto_dma_sg_cleanup(sg, input_len);
							kfree(sg);
							return -EFAULT;
						}
						len -= copy;
						to->iov_base += copy;
						to->iov_len -= copy;
					}
					if (to->iov_len == 0)
						to++;
				}

				comcerto_dma_get();
				comcerto_dma_sg_setup(sg, input_len);
				comcerto_dma_start();
				comcerto_dma_wait();
				comcerto_dma_put();
				comcerto_dma_sg_cleanup(sg, input_len);

				total_len = total_len - input_len;
				if (total_len) { 
					comcerto_dma_sg_init(sg);
					goto current_frag;
				} else {  
					kfree(sg);
					return 0;
				}

			}
		}
		if (++i >= skb_shinfo(skb)->nr_frags)
			break;
		fraglen = skb_shinfo(skb)->frags[i].size;
	}
	if (next) {
		skb = next;
		BUG_ON(skb_shinfo(skb)->frag_list);
		next = skb->next;
		goto next_skb;
	}

	comcerto_dma_sg_cleanup(sg, total_len - len);
	kfree(sg);
	return -EFAULT;
}
#endif
#endif

#ifdef CONFIG_SYNO_ARMADA_V2
 
int skb_copy_datagram_to_kernel_iovec(const struct sk_buff *skb, int offset,
				      struct iovec *to, int len)
{
	int i, fraglen, end = 0;
	struct sk_buff *next = skb_shinfo(skb)->frag_list;

	if (!len)
		return 0;

next_skb:
	fraglen = skb_headlen(skb);
	i = -1;

	while (1) {
		int start = end;

		if ((end += fraglen) > offset) {
			int copy = end - offset;
			int o = offset - start;

			if (copy > len)
				copy = len;
			if (i == -1)
				memcpy_tokerneliovec(to, skb->data + o, copy);
			else {
				skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
				struct page *page = frag->page.p;
				void *p = kmap(page) + frag->page_offset + o;
				memcpy_tokerneliovec(to, p, copy);
				kunmap(page);
			}

			if (!(len -= copy))
				return 0;
			offset += copy;
		}
		if (++i >= skb_shinfo(skb)->nr_frags)
			break;
		fraglen = skb_shinfo(skb)->frags[i].size;
	}
	if (next) {
		skb = next;
		BUG_ON(skb_shinfo(skb)->frag_list);
		next = skb->next;
		goto next_skb;
	}

	return -EFAULT;
}

#endif
 
struct sk_buff *__skb_recv_datagram(struct sock *sk, unsigned flags,
				    int *peeked, int *err)
{
	struct sk_buff *skb;
	long timeo;
	 
	int error = sock_error(sk);

	if (error)
		goto no_packet;

	timeo = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);

	do {
		 
		unsigned long cpu_flags;

		spin_lock_irqsave(&sk->sk_receive_queue.lock, cpu_flags);
		skb = skb_peek(&sk->sk_receive_queue);
		if (skb) {
			*peeked = skb->peeked;
			if (flags & MSG_PEEK) {
				skb->peeked = 1;
				atomic_inc(&skb->users);
			} else
				__skb_unlink(skb, &sk->sk_receive_queue);
		}
		spin_unlock_irqrestore(&sk->sk_receive_queue.lock, cpu_flags);

		if (skb)
			return skb;

		error = -EAGAIN;
		if (!timeo)
			goto no_packet;

	} while (!wait_for_packet(sk, err, &timeo));

	return NULL;

no_packet:
	*err = error;
	return NULL;
}
EXPORT_SYMBOL(__skb_recv_datagram);

struct sk_buff *skb_recv_datagram(struct sock *sk, unsigned flags,
				  int noblock, int *err)
{
	int peeked;

	return __skb_recv_datagram(sk, flags | (noblock ? MSG_DONTWAIT : 0),
				   &peeked, err);
}
EXPORT_SYMBOL(skb_recv_datagram);

void skb_free_datagram(struct sock *sk, struct sk_buff *skb)
{
	consume_skb(skb);
	sk_mem_reclaim_partial(sk);
}
EXPORT_SYMBOL(skb_free_datagram);

void skb_free_datagram_locked(struct sock *sk, struct sk_buff *skb)
{
	bool slow;

	if (likely(atomic_read(&skb->users) == 1))
		smp_rmb();
	else if (likely(!atomic_dec_and_test(&skb->users)))
		return;

	slow = lock_sock_fast(sk);
	skb_orphan(skb);
	sk_mem_reclaim_partial(sk);
	unlock_sock_fast(sk, slow);

	trace_kfree_skb(skb, skb_free_datagram_locked);
	__kfree_skb(skb);
}
EXPORT_SYMBOL(skb_free_datagram_locked);

int skb_kill_datagram(struct sock *sk, struct sk_buff *skb, unsigned int flags)
{
	int err = 0;

	if (flags & MSG_PEEK) {
		err = -ENOENT;
		spin_lock_bh(&sk->sk_receive_queue.lock);
		if (skb == skb_peek(&sk->sk_receive_queue)) {
			__skb_unlink(skb, &sk->sk_receive_queue);
			atomic_dec(&skb->users);
			err = 0;
		}
		spin_unlock_bh(&sk->sk_receive_queue.lock);
	}

	kfree_skb(skb);
	atomic_inc(&sk->sk_drops);
	sk_mem_reclaim_partial(sk);

	return err;
}
EXPORT_SYMBOL(skb_kill_datagram);

int skb_copy_datagram_iovec(const struct sk_buff *skb, int offset,
			    struct iovec *to, int len)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
	struct sk_buff *frag_iter;

	trace_skb_copy_datagram_iovec(skb, len);

	if (copy > 0) {
		if (copy > len)
			copy = len;
		if (memcpy_toiovec(to, skb->data + offset, copy))
			goto fault;
		if ((len -= copy) == 0)
			return 0;
		offset += copy;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		if ((copy = end - offset) > 0) {
			int err;
			u8  *vaddr;
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;
			vaddr = kmap(page);
			err = memcpy_toiovec(to, vaddr + frag->page_offset +
					     offset - start, copy);
			kunmap(page);
			if (err)
				goto fault;
			if (!(len -= copy))
				return 0;
			offset += copy;
		}
		start = end;
	}

	skb_walk_frags(skb, frag_iter) {
		int end;

		WARN_ON(start > offset + len);

		end = start + frag_iter->len;
		if ((copy = end - offset) > 0) {
			if (copy > len)
				copy = len;
			if (skb_copy_datagram_iovec(frag_iter,
						    offset - start,
						    to, copy))
				goto fault;
			if ((len -= copy) == 0)
				return 0;
			offset += copy;
		}
		start = end;
	}
	if (!len)
		return 0;

fault:
	return -EFAULT;
}
EXPORT_SYMBOL(skb_copy_datagram_iovec);

#ifdef MY_ABC_HERE
 
int skb_copy_datagram_iovec1(const struct sk_buff *skb, int offset,
			    struct iovec *to, int len)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
	struct sk_buff *frag_iter;

	trace_skb_copy_datagram_iovec(skb, len);

	if (copy > 0) {
		if (copy > len)
			copy = len;
#if 1
		memcpy_tokerneliovec(to, skb->data + offset, copy);
#else
		if (memcpy_toiovec(to, skb->data + offset, copy))
			goto fault;
#endif
		if ((len -= copy) == 0)
			return 0;
		offset += copy;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		if ((copy = end - offset) > 0) {
#if 0
			int err;
#endif
			u8  *vaddr;
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;
			vaddr = kmap(page);
#if 1
			memcpy_tokerneliovec(to, vaddr + frag->page_offset +
#else
			err = memcpy_toiovec(to, vaddr + frag->page_offset +
#endif
					     offset - start, copy);
			kunmap(page);
#if 0
			if (err)
				goto fault;
#endif
			if (!(len -= copy))
				return 0;
			offset += copy;
		}
		start = end;
	}

	skb_walk_frags(skb, frag_iter) {
		int end;

		WARN_ON(start > offset + len);

		end = start + frag_iter->len;
		if ((copy = end - offset) > 0) {
			if (copy > len)
				copy = len;
#if 1
			if (skb_copy_datagram_iovec1(frag_iter,
#else
			if (skb_copy_datagram_iovec(frag_iter,
#endif
						    offset - start,
						    to, copy))
				goto fault;
			if ((len -= copy) == 0)
				return 0;
			offset += copy;
		}
		start = end;
	}
	if (!len)
		return 0;

fault:
	return -EFAULT;
}
#endif

int skb_copy_datagram_const_iovec(const struct sk_buff *skb, int offset,
				  const struct iovec *to, int to_offset,
				  int len)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
	struct sk_buff *frag_iter;

	if (copy > 0) {
		if (copy > len)
			copy = len;
		if (memcpy_toiovecend(to, skb->data + offset, to_offset, copy))
			goto fault;
		if ((len -= copy) == 0)
			return 0;
		offset += copy;
		to_offset += copy;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		if ((copy = end - offset) > 0) {
			int err;
			u8  *vaddr;
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;
			vaddr = kmap(page);
			err = memcpy_toiovecend(to, vaddr + frag->page_offset +
						offset - start, to_offset, copy);
			kunmap(page);
			if (err)
				goto fault;
			if (!(len -= copy))
				return 0;
			offset += copy;
			to_offset += copy;
		}
		start = end;
	}

	skb_walk_frags(skb, frag_iter) {
		int end;

		WARN_ON(start > offset + len);

		end = start + frag_iter->len;
		if ((copy = end - offset) > 0) {
			if (copy > len)
				copy = len;
			if (skb_copy_datagram_const_iovec(frag_iter,
							  offset - start,
							  to, to_offset,
							  copy))
				goto fault;
			if ((len -= copy) == 0)
				return 0;
			offset += copy;
			to_offset += copy;
		}
		start = end;
	}
	if (!len)
		return 0;

fault:
	return -EFAULT;
}
EXPORT_SYMBOL(skb_copy_datagram_const_iovec);

int skb_copy_datagram_from_iovec(struct sk_buff *skb, int offset,
				 const struct iovec *from, int from_offset,
				 int len)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
	struct sk_buff *frag_iter;

	if (copy > 0) {
		if (copy > len)
			copy = len;
		if (memcpy_fromiovecend(skb->data + offset, from, from_offset,
					copy))
			goto fault;
		if ((len -= copy) == 0)
			return 0;
		offset += copy;
		from_offset += copy;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		if ((copy = end - offset) > 0) {
			int err;
			u8  *vaddr;
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;
			vaddr = kmap(page);
			err = memcpy_fromiovecend(vaddr + frag->page_offset +
						  offset - start,
						  from, from_offset, copy);
			kunmap(page);
			if (err)
				goto fault;

			if (!(len -= copy))
				return 0;
			offset += copy;
			from_offset += copy;
		}
		start = end;
	}

	skb_walk_frags(skb, frag_iter) {
		int end;

		WARN_ON(start > offset + len);

		end = start + frag_iter->len;
		if ((copy = end - offset) > 0) {
			if (copy > len)
				copy = len;
			if (skb_copy_datagram_from_iovec(frag_iter,
							 offset - start,
							 from,
							 from_offset,
							 copy))
				goto fault;
			if ((len -= copy) == 0)
				return 0;
			offset += copy;
			from_offset += copy;
		}
		start = end;
	}
	if (!len)
		return 0;

fault:
	return -EFAULT;
}
EXPORT_SYMBOL(skb_copy_datagram_from_iovec);

static int skb_copy_and_csum_datagram(const struct sk_buff *skb, int offset,
				      u8 __user *to, int len,
				      __wsum *csump)
{
	int start = skb_headlen(skb);
	int i, copy = start - offset;
	struct sk_buff *frag_iter;
	int pos = 0;

	if (copy > 0) {
		int err = 0;
		if (copy > len)
			copy = len;
		*csump = csum_and_copy_to_user(skb->data + offset, to, copy,
					       *csump, &err);
		if (err)
			goto fault;
		if ((len -= copy) == 0)
			return 0;
		offset += copy;
		to += copy;
		pos = copy;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		int end;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		WARN_ON(start > offset + len);

		end = start + skb_frag_size(frag);
		if ((copy = end - offset) > 0) {
			__wsum csum2;
			int err = 0;
			u8  *vaddr;
			struct page *page = skb_frag_page(frag);

			if (copy > len)
				copy = len;
			vaddr = kmap(page);
			csum2 = csum_and_copy_to_user(vaddr +
							frag->page_offset +
							offset - start,
						      to, copy, 0, &err);
			kunmap(page);
			if (err)
				goto fault;
			*csump = csum_block_add(*csump, csum2, pos);
			if (!(len -= copy))
				return 0;
			offset += copy;
			to += copy;
			pos += copy;
		}
		start = end;
	}

	skb_walk_frags(skb, frag_iter) {
		int end;

		WARN_ON(start > offset + len);

		end = start + frag_iter->len;
		if ((copy = end - offset) > 0) {
			__wsum csum2 = 0;
			if (copy > len)
				copy = len;
			if (skb_copy_and_csum_datagram(frag_iter,
						       offset - start,
						       to, copy,
						       &csum2))
				goto fault;
			*csump = csum_block_add(*csump, csum2, pos);
			if ((len -= copy) == 0)
				return 0;
			offset += copy;
			to += copy;
			pos += copy;
		}
		start = end;
	}
	if (!len)
		return 0;

fault:
	return -EFAULT;
}

__sum16 __skb_checksum_complete_head(struct sk_buff *skb, int len)
{
	__sum16 sum;

	sum = csum_fold(skb_checksum(skb, 0, len, skb->csum));
	if (likely(!sum)) {
		if (unlikely(skb->ip_summed == CHECKSUM_COMPLETE))
			netdev_rx_csum_fault(skb->dev);
		if (!skb_shared(skb))
			skb->ip_summed = CHECKSUM_UNNECESSARY;
	}
	return sum;
}
EXPORT_SYMBOL(__skb_checksum_complete_head);

__sum16 __skb_checksum_complete(struct sk_buff *skb)
{
	return __skb_checksum_complete_head(skb, skb->len);
}
EXPORT_SYMBOL(__skb_checksum_complete);

int skb_copy_and_csum_datagram_iovec(struct sk_buff *skb,
				     int hlen, struct iovec *iov)
{
	__wsum csum;
	int chunk = skb->len - hlen;

	if (!chunk)
		return 0;

	while (!iov->iov_len)
		iov++;

	if (iov->iov_len < chunk) {
		if (__skb_checksum_complete(skb))
			goto csum_error;
		if (skb_copy_datagram_iovec(skb, hlen, iov, chunk))
			goto fault;
	} else {
		csum = csum_partial(skb->data, hlen, skb->csum);
		if (skb_copy_and_csum_datagram(skb, hlen, iov->iov_base,
					       chunk, &csum))
			goto fault;
		if (csum_fold(csum))
			goto csum_error;
		if (unlikely(skb->ip_summed == CHECKSUM_COMPLETE))
			netdev_rx_csum_fault(skb->dev);
		iov->iov_len -= chunk;
		iov->iov_base += chunk;
	}
	return 0;
csum_error:
	return -EINVAL;
fault:
	return -EFAULT;
}
EXPORT_SYMBOL(skb_copy_and_csum_datagram_iovec);

unsigned int datagram_poll(struct file *file, struct socket *sock,
			   poll_table *wait)
{
	struct sock *sk = sock->sk;
	unsigned int mask;

	sock_poll_wait(file, sk_sleep(sk), wait);
	mask = 0;

	if (sk->sk_err || !skb_queue_empty(&sk->sk_error_queue))
		mask |= POLLERR;
	if (sk->sk_shutdown & RCV_SHUTDOWN)
		mask |= POLLRDHUP | POLLIN | POLLRDNORM;
	if (sk->sk_shutdown == SHUTDOWN_MASK)
		mask |= POLLHUP;

	if (!skb_queue_empty(&sk->sk_receive_queue))
		mask |= POLLIN | POLLRDNORM;

	if (connection_based(sk)) {
		if (sk->sk_state == TCP_CLOSE)
			mask |= POLLHUP;
		 
		if (sk->sk_state == TCP_SYN_SENT)
			return mask;
	}

	if (sock_writeable(sk))
		mask |= POLLOUT | POLLWRNORM | POLLWRBAND;
	else
		set_bit(SOCK_ASYNC_NOSPACE, &sk->sk_socket->flags);

	return mask;
}
EXPORT_SYMBOL(datagram_poll);
