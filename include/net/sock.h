#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _SOCK_H
#define _SOCK_H

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/list_nulls.h>
#include <linux/timer.h>
#include <linux/cache.h>
#include <linux/module.h>
#include <linux/lockdep.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>	 
#include <linux/mm.h>
#include <linux/security.h>

#include <linux/filter.h>
#include <linux/rculist_nulls.h>
#include <linux/poll.h>

#include <asm/atomic.h>
#include <net/dst.h>
#include <net/checksum.h>

#define SOCK_DEBUGGING
#ifdef SOCK_DEBUGGING
#define SOCK_DEBUG(sk, msg...) do { if ((sk) && sock_flag((sk), SOCK_DBG)) \
					printk(KERN_DEBUG msg); } while (0)
#else
 
static void inline int __attribute__ ((format (printf, 2, 3)))
SOCK_DEBUG(struct sock *sk, const char *msg, ...)
{
}
#endif

typedef struct {
	spinlock_t		slock;
	int			owned;
	wait_queue_head_t	wq;
	 
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
} socket_lock_t;

struct sock;
struct proto;
struct net;

struct sock_common {
	 
	union {
		struct hlist_node	skc_node;
		struct hlist_nulls_node skc_nulls_node;
	};
	atomic_t		skc_refcnt;

	unsigned int		skc_hash;
	unsigned short		skc_family;
	volatile unsigned char	skc_state;
	unsigned char		skc_reuse;
	int			skc_bound_dev_if;
	struct hlist_node	skc_bind_node;
	struct proto		*skc_prot;
#ifdef CONFIG_NET_NS
	struct net	 	*skc_net;
#endif
};

struct sock {
	 
	struct sock_common	__sk_common;
#define sk_node			__sk_common.skc_node
#define sk_nulls_node		__sk_common.skc_nulls_node
#define sk_refcnt		__sk_common.skc_refcnt

#define sk_copy_start		__sk_common.skc_hash
#define sk_hash			__sk_common.skc_hash
#define sk_family		__sk_common.skc_family
#define sk_state		__sk_common.skc_state
#define sk_reuse		__sk_common.skc_reuse
#define sk_bound_dev_if		__sk_common.skc_bound_dev_if
#define sk_bind_node		__sk_common.skc_bind_node
#define sk_prot			__sk_common.skc_prot
#define sk_net			__sk_common.skc_net
	kmemcheck_bitfield_begin(flags);
	unsigned int		sk_shutdown  : 2,
				sk_no_check  : 2,
				sk_userlocks : 4,
				sk_protocol  : 8,
				sk_type      : 16;
	kmemcheck_bitfield_end(flags);
	int			sk_rcvbuf;
	socket_lock_t		sk_lock;
	 
	struct {
		struct sk_buff *head;
		struct sk_buff *tail;
	} sk_backlog;
	wait_queue_head_t	*sk_sleep;
	struct dst_entry	*sk_dst_cache;
#ifdef CONFIG_XFRM
	struct xfrm_policy	*sk_policy[2];
#endif
	rwlock_t		sk_dst_lock;
#ifdef MY_DEF_HERE
#ifdef CONFIG_GFAR_HW_TCP_RECEIVE_OFFLOAD
	void			*tcp_hw_channel;
	u32			init_seq;
#endif
#endif
	atomic_t		sk_rmem_alloc;
	atomic_t		sk_wmem_alloc;
	atomic_t		sk_omem_alloc;
	int			sk_sndbuf;
	struct sk_buff_head	sk_receive_queue;
	struct sk_buff_head	sk_write_queue;
#ifdef CONFIG_NET_DMA
	struct sk_buff_head	sk_async_wait_queue;
#endif
#ifdef MY_DEF_HERE
#ifdef CONFIG_TCP_FAST_ACK
	struct sk_buff_head	sk_ack_queue;
#endif
#endif
	int			sk_wmem_queued;
	int			sk_forward_alloc;
	gfp_t			sk_allocation;
	int			sk_route_caps;
	int			sk_gso_type;
	unsigned int		sk_gso_max_size;
	int			sk_rcvlowat;
	unsigned long 		sk_flags;
	unsigned long	        sk_lingertime;
	struct sk_buff_head	sk_error_queue;
	struct proto		*sk_prot_creator;
	rwlock_t		sk_callback_lock;
	int			sk_err,
				sk_err_soft;
	atomic_t		sk_drops;
	unsigned short		sk_ack_backlog;
	unsigned short		sk_max_ack_backlog;
	__u32			sk_priority;
	struct ucred		sk_peercred;
	long			sk_rcvtimeo;
	long			sk_sndtimeo;
	struct sk_filter      	*sk_filter;
	void			*sk_protinfo;
	struct timer_list	sk_timer;
	ktime_t			sk_stamp;
	struct socket		*sk_socket;
	void			*sk_user_data;
	struct page		*sk_sndmsg_page;
	struct sk_buff		*sk_send_head;
	__u32			sk_sndmsg_off;
	int			sk_write_pending;
#ifdef CONFIG_SECURITY
	void			*sk_security;
#endif
	__u32			sk_mark;
	 
	void			(*sk_state_change)(struct sock *sk);
	void			(*sk_data_ready)(struct sock *sk, int bytes);
	void			(*sk_write_space)(struct sock *sk);
	void			(*sk_error_report)(struct sock *sk);
  	int			(*sk_backlog_rcv)(struct sock *sk,
						  struct sk_buff *skb);  
	void                    (*sk_destruct)(struct sock *sk);
};

static inline struct sock *__sk_head(const struct hlist_head *head)
{
	return hlist_entry(head->first, struct sock, sk_node);
}

static inline struct sock *sk_head(const struct hlist_head *head)
{
	return hlist_empty(head) ? NULL : __sk_head(head);
}

static inline struct sock *__sk_nulls_head(const struct hlist_nulls_head *head)
{
	return hlist_nulls_entry(head->first, struct sock, sk_nulls_node);
}

static inline struct sock *sk_nulls_head(const struct hlist_nulls_head *head)
{
	return hlist_nulls_empty(head) ? NULL : __sk_nulls_head(head);
}

static inline struct sock *sk_next(const struct sock *sk)
{
	return sk->sk_node.next ?
		hlist_entry(sk->sk_node.next, struct sock, sk_node) : NULL;
}

static inline struct sock *sk_nulls_next(const struct sock *sk)
{
	return (!is_a_nulls(sk->sk_nulls_node.next)) ?
		hlist_nulls_entry(sk->sk_nulls_node.next,
				  struct sock, sk_nulls_node) :
		NULL;
}

static inline int sk_unhashed(const struct sock *sk)
{
	return hlist_unhashed(&sk->sk_node);
}

static inline int sk_hashed(const struct sock *sk)
{
	return !sk_unhashed(sk);
}

static __inline__ void sk_node_init(struct hlist_node *node)
{
	node->pprev = NULL;
}

static __inline__ void sk_nulls_node_init(struct hlist_nulls_node *node)
{
	node->pprev = NULL;
}

static __inline__ void __sk_del_node(struct sock *sk)
{
	__hlist_del(&sk->sk_node);
}

static __inline__ int __sk_del_node_init(struct sock *sk)
{
	if (sk_hashed(sk)) {
		__sk_del_node(sk);
		sk_node_init(&sk->sk_node);
		return 1;
	}
	return 0;
}

static inline void sock_hold(struct sock *sk)
{
	atomic_inc(&sk->sk_refcnt);
}

static inline void __sock_put(struct sock *sk)
{
	atomic_dec(&sk->sk_refcnt);
}

static __inline__ int sk_del_node_init(struct sock *sk)
{
	int rc = __sk_del_node_init(sk);

	if (rc) {
		 
		WARN_ON(atomic_read(&sk->sk_refcnt) == 1);
		__sock_put(sk);
	}
	return rc;
}

static __inline__ int __sk_nulls_del_node_init_rcu(struct sock *sk)
{
	if (sk_hashed(sk)) {
		hlist_nulls_del_init_rcu(&sk->sk_nulls_node);
		return 1;
	}
	return 0;
}

static __inline__ int sk_nulls_del_node_init_rcu(struct sock *sk)
{
	int rc = __sk_nulls_del_node_init_rcu(sk);

	if (rc) {
		 
		WARN_ON(atomic_read(&sk->sk_refcnt) == 1);
		__sock_put(sk);
	}
	return rc;
}

static __inline__ void __sk_add_node(struct sock *sk, struct hlist_head *list)
{
	hlist_add_head(&sk->sk_node, list);
}

static __inline__ void sk_add_node(struct sock *sk, struct hlist_head *list)
{
	sock_hold(sk);
	__sk_add_node(sk, list);
}

static __inline__ void __sk_nulls_add_node_rcu(struct sock *sk, struct hlist_nulls_head *list)
{
	hlist_nulls_add_head_rcu(&sk->sk_nulls_node, list);
}

static __inline__ void sk_nulls_add_node_rcu(struct sock *sk, struct hlist_nulls_head *list)
{
	sock_hold(sk);
	__sk_nulls_add_node_rcu(sk, list);
}

static __inline__ void __sk_del_bind_node(struct sock *sk)
{
	__hlist_del(&sk->sk_bind_node);
}

static __inline__ void sk_add_bind_node(struct sock *sk,
					struct hlist_head *list)
{
	hlist_add_head(&sk->sk_bind_node, list);
}

#define sk_for_each(__sk, node, list) \
	hlist_for_each_entry(__sk, node, list, sk_node)
#define sk_nulls_for_each(__sk, node, list) \
	hlist_nulls_for_each_entry(__sk, node, list, sk_nulls_node)
#define sk_nulls_for_each_rcu(__sk, node, list) \
	hlist_nulls_for_each_entry_rcu(__sk, node, list, sk_nulls_node)
#define sk_for_each_from(__sk, node) \
	if (__sk && ({ node = &(__sk)->sk_node; 1; })) \
		hlist_for_each_entry_from(__sk, node, sk_node)
#define sk_nulls_for_each_from(__sk, node) \
	if (__sk && ({ node = &(__sk)->sk_nulls_node; 1; })) \
		hlist_nulls_for_each_entry_from(__sk, node, sk_nulls_node)
#define sk_for_each_continue(__sk, node) \
	if (__sk && ({ node = &(__sk)->sk_node; 1; })) \
		hlist_for_each_entry_continue(__sk, node, sk_node)
#define sk_for_each_safe(__sk, node, tmp, list) \
	hlist_for_each_entry_safe(__sk, node, tmp, list, sk_node)
#define sk_for_each_bound(__sk, node, list) \
	hlist_for_each_entry(__sk, node, list, sk_bind_node)

enum sock_flags {
	SOCK_DEAD,
	SOCK_DONE,
	SOCK_URGINLINE,
	SOCK_KEEPOPEN,
	SOCK_LINGER,
	SOCK_DESTROY,
	SOCK_BROADCAST,
	SOCK_TIMESTAMP,
	SOCK_ZAPPED,
	SOCK_USE_WRITE_QUEUE,  
	SOCK_DBG,  
	SOCK_RCVTSTAMP,  
	SOCK_RCVTSTAMPNS,  
	SOCK_LOCALROUTE,  
	SOCK_QUEUE_SHRUNK,  
	SOCK_TIMESTAMPING_TX_HARDWARE,   
	SOCK_TIMESTAMPING_TX_SOFTWARE,   
	SOCK_TIMESTAMPING_RX_HARDWARE,   
	SOCK_TIMESTAMPING_RX_SOFTWARE,   
	SOCK_TIMESTAMPING_SOFTWARE,      
	SOCK_TIMESTAMPING_RAW_HARDWARE,  
	SOCK_TIMESTAMPING_SYS_HARDWARE,  
#ifdef CONFIG_SYNO_PLX_PORTING
	SOCK_ZCC,
#endif
};

static inline void sock_copy_flags(struct sock *nsk, struct sock *osk)
{
	nsk->sk_flags = osk->sk_flags;
}

static inline void sock_set_flag(struct sock *sk, enum sock_flags flag)
{
	__set_bit(flag, &sk->sk_flags);
}

static inline void sock_reset_flag(struct sock *sk, enum sock_flags flag)
{
	__clear_bit(flag, &sk->sk_flags);
}

static inline int sock_flag(struct sock *sk, enum sock_flags flag)
{
	return test_bit(flag, &sk->sk_flags);
}

static inline void sk_acceptq_removed(struct sock *sk)
{
	sk->sk_ack_backlog--;
}

static inline void sk_acceptq_added(struct sock *sk)
{
	sk->sk_ack_backlog++;
}

static inline int sk_acceptq_is_full(struct sock *sk)
{
	return sk->sk_ack_backlog > sk->sk_max_ack_backlog;
}

static inline int sk_stream_min_wspace(struct sock *sk)
{
	return sk->sk_wmem_queued >> 1;
}

static inline int sk_stream_wspace(struct sock *sk)
{
	return sk->sk_sndbuf - sk->sk_wmem_queued;
}

extern void sk_stream_write_space(struct sock *sk);

static inline int sk_stream_memory_free(struct sock *sk)
{
	return sk->sk_wmem_queued < sk->sk_sndbuf;
}

static inline void sk_add_backlog(struct sock *sk, struct sk_buff *skb)
{
	if (!sk->sk_backlog.tail) {
		sk->sk_backlog.head = sk->sk_backlog.tail = skb;
	} else {
		sk->sk_backlog.tail->next = skb;
		sk->sk_backlog.tail = skb;
	}
	skb->next = NULL;
}

static inline int sk_backlog_rcv(struct sock *sk, struct sk_buff *skb)
{
	return sk->sk_backlog_rcv(sk, skb);
}

#define sk_wait_event(__sk, __timeo, __condition)			\
	({	int __rc;						\
		release_sock(__sk);					\
		__rc = __condition;					\
		if (!__rc) {						\
			*(__timeo) = schedule_timeout(*(__timeo));	\
		}							\
		lock_sock(__sk);					\
		__rc = __condition;					\
		__rc;							\
	})

extern int sk_stream_wait_connect(struct sock *sk, long *timeo_p);
extern int sk_stream_wait_memory(struct sock *sk, long *timeo_p);
extern void sk_stream_wait_close(struct sock *sk, long timeo_p);
extern int sk_stream_error(struct sock *sk, int flags, int err);
extern void sk_stream_kill_queues(struct sock *sk);

extern int sk_wait_data(struct sock *sk, long *timeo);

struct request_sock_ops;
struct timewait_sock_ops;
struct inet_hashinfo;
struct raw_hashinfo;

struct proto {
	void			(*close)(struct sock *sk, 
					long timeout);
	int			(*connect)(struct sock *sk,
				        struct sockaddr *uaddr, 
					int addr_len);
	int			(*disconnect)(struct sock *sk, int flags);

	struct sock *		(*accept) (struct sock *sk, int flags, int *err);

	int			(*ioctl)(struct sock *sk, int cmd,
					 unsigned long arg);
	int			(*init)(struct sock *sk);
	void			(*destroy)(struct sock *sk);
	void			(*shutdown)(struct sock *sk, int how);
	int			(*setsockopt)(struct sock *sk, int level, 
					int optname, char __user *optval,
					unsigned int optlen);
	int			(*getsockopt)(struct sock *sk, int level, 
					int optname, char __user *optval, 
					int __user *option);  	 
#ifdef CONFIG_COMPAT
	int			(*compat_setsockopt)(struct sock *sk,
					int level,
					int optname, char __user *optval,
					unsigned int optlen);
	int			(*compat_getsockopt)(struct sock *sk,
					int level,
					int optname, char __user *optval,
					int __user *option);
#endif
	int			(*sendmsg)(struct kiocb *iocb, struct sock *sk,
					   struct msghdr *msg, size_t len);
	int			(*recvmsg)(struct kiocb *iocb, struct sock *sk,
					   struct msghdr *msg,
					size_t len, int noblock, int flags, 
					int *addr_len);
	int			(*sendpage)(struct sock *sk, struct page *page,
					int offset, size_t size, int flags);
#ifdef CONFIG_SYNO_PLX_PORTING
	int			(*sendpages)(struct sock *sk, struct page **page,
					int offset, size_t size, int flags);
#endif
	int			(*bind)(struct sock *sk, 
					struct sockaddr *uaddr, int addr_len);

	int			(*backlog_rcv) (struct sock *sk, 
						struct sk_buff *skb);

	void			(*hash)(struct sock *sk);
	void			(*unhash)(struct sock *sk);
	int			(*get_port)(struct sock *sk, unsigned short snum);

#ifdef CONFIG_PROC_FS
	unsigned int		inuse_idx;
#endif

	void			(*enter_memory_pressure)(struct sock *sk);
	atomic_t		*memory_allocated;	 
	struct percpu_counter	*sockets_allocated;	 
	 
	int			*memory_pressure;
	int			*sysctl_mem;
	int			*sysctl_wmem;
	int			*sysctl_rmem;
	int			max_header;

	struct kmem_cache	*slab;
	unsigned int		obj_size;
	int			slab_flags;

	struct percpu_counter	*orphan_count;

	struct request_sock_ops	*rsk_prot;
	struct timewait_sock_ops *twsk_prot;

	union {
		struct inet_hashinfo	*hashinfo;
		struct udp_table	*udp_table;
		struct raw_hashinfo	*raw_hash;
	} h;

	struct module		*owner;

	char			name[32];

	struct list_head	node;
#ifdef SOCK_REFCNT_DEBUG
	atomic_t		socks;
#endif
};

extern int proto_register(struct proto *prot, int alloc_slab);
extern void proto_unregister(struct proto *prot);

#ifdef SOCK_REFCNT_DEBUG
static inline void sk_refcnt_debug_inc(struct sock *sk)
{
	atomic_inc(&sk->sk_prot->socks);
}

static inline void sk_refcnt_debug_dec(struct sock *sk)
{
	atomic_dec(&sk->sk_prot->socks);
	printk(KERN_DEBUG "%s socket %p released, %d are still alive\n",
	       sk->sk_prot->name, sk, atomic_read(&sk->sk_prot->socks));
}

static inline void sk_refcnt_debug_release(const struct sock *sk)
{
	if (atomic_read(&sk->sk_refcnt) != 1)
		printk(KERN_DEBUG "Destruction of the %s socket %p delayed, refcnt=%d\n",
		       sk->sk_prot->name, sk, atomic_read(&sk->sk_refcnt));
}
#else  
#define sk_refcnt_debug_inc(sk) do { } while (0)
#define sk_refcnt_debug_dec(sk) do { } while (0)
#define sk_refcnt_debug_release(sk) do { } while (0)
#endif  

#ifdef CONFIG_PROC_FS
 
extern void sock_prot_inuse_add(struct net *net, struct proto *prot, int inc);
extern int sock_prot_inuse_get(struct net *net, struct proto *proto);
#else
static void inline sock_prot_inuse_add(struct net *net, struct proto *prot,
		int inc)
{
}
#endif

static inline void __sk_prot_rehash(struct sock *sk)
{
	sk->sk_prot->unhash(sk);
	sk->sk_prot->hash(sk);
}

#define SOCK_DESTROY_TIME (10*HZ)

#define PROT_SOCK	1024

#define SHUTDOWN_MASK	3
#define RCV_SHUTDOWN	1
#define SEND_SHUTDOWN	2

#define SOCK_SNDBUF_LOCK	1
#define SOCK_RCVBUF_LOCK	2
#define SOCK_BINDADDR_LOCK	4
#define SOCK_BINDPORT_LOCK	8

struct sock_iocb {
	struct list_head	list;

	int			flags;
	int			size;
	struct socket		*sock;
	struct sock		*sk;
	struct scm_cookie	*scm;
	struct msghdr		*msg, async_msg;
	struct kiocb		*kiocb;
};

static inline struct sock_iocb *kiocb_to_siocb(struct kiocb *iocb)
{
	return (struct sock_iocb *)iocb->private;
}

static inline struct kiocb *siocb_to_kiocb(struct sock_iocb *si)
{
	return si->kiocb;
}

struct socket_alloc {
	struct socket socket;
	struct inode vfs_inode;
};

static inline struct socket *SOCKET_I(struct inode *inode)
{
	return &container_of(inode, struct socket_alloc, vfs_inode)->socket;
}

static inline struct inode *SOCK_INODE(struct socket *socket)
{
	return &container_of(socket, struct socket_alloc, socket)->vfs_inode;
}

extern int __sk_mem_schedule(struct sock *sk, int size, int kind);
extern void __sk_mem_reclaim(struct sock *sk);

#define SK_MEM_QUANTUM ((int)PAGE_SIZE)
#define SK_MEM_QUANTUM_SHIFT ilog2(SK_MEM_QUANTUM)
#define SK_MEM_SEND	0
#define SK_MEM_RECV	1

static inline int sk_mem_pages(int amt)
{
	return (amt + SK_MEM_QUANTUM - 1) >> SK_MEM_QUANTUM_SHIFT;
}

static inline int sk_has_account(struct sock *sk)
{
	 
	return !!sk->sk_prot->memory_allocated;
}

static inline int sk_wmem_schedule(struct sock *sk, int size)
{
	if (!sk_has_account(sk))
		return 1;
	return size <= sk->sk_forward_alloc ||
		__sk_mem_schedule(sk, size, SK_MEM_SEND);
}

static inline int sk_rmem_schedule(struct sock *sk, int size)
{
	if (!sk_has_account(sk))
		return 1;
	return size <= sk->sk_forward_alloc ||
		__sk_mem_schedule(sk, size, SK_MEM_RECV);
}

static inline void sk_mem_reclaim(struct sock *sk)
{
	if (!sk_has_account(sk))
		return;
	if (sk->sk_forward_alloc >= SK_MEM_QUANTUM)
		__sk_mem_reclaim(sk);
}

static inline void sk_mem_reclaim_partial(struct sock *sk)
{
	if (!sk_has_account(sk))
		return;
	if (sk->sk_forward_alloc > SK_MEM_QUANTUM)
		__sk_mem_reclaim(sk);
}

static inline void sk_mem_charge(struct sock *sk, int size)
{
	if (!sk_has_account(sk))
		return;
	sk->sk_forward_alloc -= size;
}

static inline void sk_mem_uncharge(struct sock *sk, int size)
{
	if (!sk_has_account(sk))
		return;
	sk->sk_forward_alloc += size;
}

static inline void sk_wmem_free_skb(struct sock *sk, struct sk_buff *skb)
{
	sock_set_flag(sk, SOCK_QUEUE_SHRUNK);
	sk->sk_wmem_queued -= skb->truesize;
	sk_mem_uncharge(sk, skb->truesize);
	__kfree_skb(skb);
}

#define sock_owned_by_user(sk)	((sk)->sk_lock.owned)

#ifdef CONFIG_SYNO_PLX_PORTING
#define sock_lock_init_class_and_name(sk, sname, skey, name, key) 	\
do {									\
	sk->sk_lock.owned = 0;						\
	init_waitqueue_head(&sk->sk_lock.wq);				\
	spin_lock_init(&(sk)->sk_lock.slock);				\
	debug_check_no_locks_freed((void *)&(sk)->sk_lock,		\
			sizeof((sk)->sk_lock));				\
	lockdep_set_class_and_name(&(sk)->sk_lock.slock.lock,		\
		       	(skey), (sname));				\
	lockdep_init_map(&(sk)->sk_lock.dep_map, (name), (key), 0);	\
} while (0)
#else
#define sock_lock_init_class_and_name(sk, sname, skey, name, key) 	\
do {									\
	sk->sk_lock.owned = 0;						\
	init_waitqueue_head(&sk->sk_lock.wq);				\
	spin_lock_init(&(sk)->sk_lock.slock);				\
	debug_check_no_locks_freed((void *)&(sk)->sk_lock,		\
			sizeof((sk)->sk_lock));				\
	lockdep_set_class_and_name(&(sk)->sk_lock.slock,		\
		       	(skey), (sname));				\
	lockdep_init_map(&(sk)->sk_lock.dep_map, (name), (key), 0);	\
} while (0)
#endif

extern void lock_sock_nested(struct sock *sk, int subclass);

static inline void lock_sock(struct sock *sk)
{
	lock_sock_nested(sk, 0);
}

extern void release_sock(struct sock *sk);

#ifdef CONFIG_SYNO_PLX_PORTING
#define bh_lock_wsock(_sk) do { \
	if (unlikely(!spin_trylock(&(_sk)->sk_lock.slock))) { \
		unsigned long end = jiffies + HZ; \
		do { \
			if (time_after(jiffies, end)) { \
				printk(KERN_WARNING "bh_lock_wsock() A second has elapsed " \
					"while waiting for spinlock\n"); \
				end = jiffies + HZ; \
			} \
		} while (unlikely(!spin_trylock(&(_sk)->sk_lock.slock))); \
	} \
} while(0)
 
#define bh_lock_wsock_nested(_sk) bh_lock_wsock(_sk)
 
#define bh_unlock_wsock(_sk) spin_unlock(&(_sk)->sk_lock.slock)
 
#define wspin_lock_bh(_lock) do { \
	if (unlikely(!spin_trylock_bh((_lock)))) { \
		unsigned long end = jiffies + HZ; \
		do { \
			if (time_after(jiffies, end)) { \
				printk(KERN_WARNING "wspin_lock_bh() A second has elapsed " \
					"while waiting for spinlock\n"); \
				end = jiffies + HZ; \
			} \
		} while (unlikely(!spin_trylock_bh((_lock)))); \
	} \
} while(0)
 
#define wspin_unlock_bh(_lock) spin_unlock_bh((_lock))
 
#define wspin_unlock(_lock) spin_unlock((_lock))
 
#define wspin_trylock(_lock) spin_trylock((_lock))
 
#else
#define bh_lock_sock(__sk)	spin_lock(&((__sk)->sk_lock.slock))
#define bh_lock_sock_nested(__sk) \
				spin_lock_nested(&((__sk)->sk_lock.slock), \
				SINGLE_DEPTH_NESTING)
#define bh_unlock_sock(__sk)	spin_unlock(&((__sk)->sk_lock.slock))
#endif

extern struct sock		*sk_alloc(struct net *net, int family,
					  gfp_t priority,
					  struct proto *prot);
extern void			sk_free(struct sock *sk);
extern void			sk_release_kernel(struct sock *sk);
extern struct sock		*sk_clone(const struct sock *sk,
					  const gfp_t priority);

extern struct sk_buff		*sock_wmalloc(struct sock *sk,
					      unsigned long size, int force,
					      gfp_t priority);
extern struct sk_buff		*sock_rmalloc(struct sock *sk,
					      unsigned long size, int force,
					      gfp_t priority);
extern void			sock_wfree(struct sk_buff *skb);
extern void			sock_rfree(struct sk_buff *skb);

extern int			sock_setsockopt(struct socket *sock, int level,
						int op, char __user *optval,
						unsigned int optlen);

extern int			sock_getsockopt(struct socket *sock, int level,
						int op, char __user *optval, 
						int __user *optlen);
extern struct sk_buff 		*sock_alloc_send_skb(struct sock *sk,
						     unsigned long size,
						     int noblock,
						     int *errcode);
extern struct sk_buff 		*sock_alloc_send_pskb(struct sock *sk,
						      unsigned long header_len,
						      unsigned long data_len,
						      int noblock,
						      int *errcode);
extern void *sock_kmalloc(struct sock *sk, int size,
			  gfp_t priority);
extern void sock_kfree_s(struct sock *sk, void *mem, int size);
extern void sk_send_sigurg(struct sock *sk);

extern int                      sock_no_bind(struct socket *, 
					     struct sockaddr *, int);
extern int                      sock_no_connect(struct socket *,
						struct sockaddr *, int, int);
extern int                      sock_no_socketpair(struct socket *,
						   struct socket *);
extern int                      sock_no_accept(struct socket *,
					       struct socket *, int);
extern int                      sock_no_getname(struct socket *,
						struct sockaddr *, int *, int);
extern unsigned int             sock_no_poll(struct file *, struct socket *,
					     struct poll_table_struct *);
extern int                      sock_no_ioctl(struct socket *, unsigned int,
					      unsigned long);
extern int			sock_no_listen(struct socket *, int);
extern int                      sock_no_shutdown(struct socket *, int);
extern int			sock_no_getsockopt(struct socket *, int , int,
						   char __user *, int __user *);
extern int			sock_no_setsockopt(struct socket *, int, int,
						   char __user *, unsigned int);
extern int                      sock_no_sendmsg(struct kiocb *, struct socket *,
						struct msghdr *, size_t);
extern int                      sock_no_recvmsg(struct kiocb *, struct socket *,
						struct msghdr *, size_t, int);
extern int			sock_no_mmap(struct file *file,
					     struct socket *sock,
					     struct vm_area_struct *vma);
extern ssize_t			sock_no_sendpage(struct socket *sock,
						struct page *page,
						int offset, size_t size, 
						int flags);

extern int sock_common_getsockopt(struct socket *sock, int level, int optname,
				  char __user *optval, int __user *optlen);
extern int sock_common_recvmsg(struct kiocb *iocb, struct socket *sock,
			       struct msghdr *msg, size_t size, int flags);
extern int sock_common_setsockopt(struct socket *sock, int level, int optname,
				  char __user *optval, unsigned int optlen);
extern int compat_sock_common_getsockopt(struct socket *sock, int level,
		int optname, char __user *optval, int __user *optlen);
extern int compat_sock_common_setsockopt(struct socket *sock, int level,
		int optname, char __user *optval, unsigned int optlen);

extern void sk_common_release(struct sock *sk);

extern void sock_init_data(struct socket *sock, struct sock *sk);

static inline void sk_filter_release(struct sk_filter *fp)
{
	if (atomic_dec_and_test(&fp->refcnt))
		kfree(fp);
}

static inline void sk_filter_uncharge(struct sock *sk, struct sk_filter *fp)
{
	unsigned int size = sk_filter_len(fp);

	atomic_sub(size, &sk->sk_omem_alloc);
	sk_filter_release(fp);
}

static inline void sk_filter_charge(struct sock *sk, struct sk_filter *fp)
{
	atomic_inc(&fp->refcnt);
	atomic_add(sk_filter_len(fp), &sk->sk_omem_alloc);
}

static inline void sock_put(struct sock *sk)
{
	if (atomic_dec_and_test(&sk->sk_refcnt))
		sk_free(sk);
}

extern int sk_receive_skb(struct sock *sk, struct sk_buff *skb,
			  const int nested);

static inline void sk_set_socket(struct sock *sk, struct socket *sock)
{
	sk->sk_socket = sock;
}

static inline void sock_orphan(struct sock *sk)
{
	write_lock_bh(&sk->sk_callback_lock);
	sock_set_flag(sk, SOCK_DEAD);
	sk_set_socket(sk, NULL);
	sk->sk_sleep  = NULL;
	write_unlock_bh(&sk->sk_callback_lock);
}

static inline void sock_graft(struct sock *sk, struct socket *parent)
{
	write_lock_bh(&sk->sk_callback_lock);
	sk->sk_sleep = &parent->wait;
	parent->sk = sk;
	sk_set_socket(sk, parent);
	security_sock_graft(sk, parent);
	write_unlock_bh(&sk->sk_callback_lock);
}

extern int sock_i_uid(struct sock *sk);
extern unsigned long sock_i_ino(struct sock *sk);

static inline struct dst_entry *
__sk_dst_get(struct sock *sk)
{
	return sk->sk_dst_cache;
}

static inline struct dst_entry *
sk_dst_get(struct sock *sk)
{
	struct dst_entry *dst;

	read_lock(&sk->sk_dst_lock);
	dst = sk->sk_dst_cache;
	if (dst)
		dst_hold(dst);
	read_unlock(&sk->sk_dst_lock);
	return dst;
}

static inline void
__sk_dst_set(struct sock *sk, struct dst_entry *dst)
{
	struct dst_entry *old_dst;

	old_dst = sk->sk_dst_cache;
	sk->sk_dst_cache = dst;
	dst_release(old_dst);
}

static inline void
sk_dst_set(struct sock *sk, struct dst_entry *dst)
{
	write_lock(&sk->sk_dst_lock);
	__sk_dst_set(sk, dst);
	write_unlock(&sk->sk_dst_lock);
}

static inline void
__sk_dst_reset(struct sock *sk)
{
	struct dst_entry *old_dst;

	old_dst = sk->sk_dst_cache;
	sk->sk_dst_cache = NULL;
	dst_release(old_dst);
}

static inline void
sk_dst_reset(struct sock *sk)
{
	write_lock(&sk->sk_dst_lock);
	__sk_dst_reset(sk);
	write_unlock(&sk->sk_dst_lock);
}

extern struct dst_entry *__sk_dst_check(struct sock *sk, u32 cookie);

extern struct dst_entry *sk_dst_check(struct sock *sk, u32 cookie);

static inline int sk_can_gso(const struct sock *sk)
{
	return net_gso_ok(sk->sk_route_caps, sk->sk_gso_type);
}

extern void sk_setup_caps(struct sock *sk, struct dst_entry *dst);

static inline int skb_copy_to_page(struct sock *sk, char __user *from,
				   struct sk_buff *skb, struct page *page,
				   int off, int copy)
{
	if (skb->ip_summed == CHECKSUM_NONE) {
		int err = 0;
		__wsum csum = csum_and_copy_from_user(from,
						     page_address(page) + off,
							    copy, 0, &err);
		if (err)
			return err;
		skb->csum = csum_block_add(skb->csum, csum, skb->len);
	} else if (copy_from_user(page_address(page) + off, from, copy))
		return -EFAULT;

	skb->len	     += copy;
	skb->data_len	     += copy;
	skb->truesize	     += copy;
	sk->sk_wmem_queued   += copy;
	sk_mem_charge(sk, copy);
	return 0;
}

static inline int sk_wmem_alloc_get(const struct sock *sk)
{
	return atomic_read(&sk->sk_wmem_alloc) - 1;
}

static inline int sk_rmem_alloc_get(const struct sock *sk)
{
	return atomic_read(&sk->sk_rmem_alloc);
}

static inline int sk_has_allocations(const struct sock *sk)
{
	return sk_wmem_alloc_get(sk) || sk_rmem_alloc_get(sk);
}

static inline int sk_has_sleeper(struct sock *sk)
{
	 
	smp_mb__after_lock();
	return sk->sk_sleep && waitqueue_active(sk->sk_sleep);
}

static inline void sock_poll_wait(struct file *filp,
		wait_queue_head_t *wait_address, poll_table *p)
{
	if (p && wait_address) {
		poll_wait(filp, wait_address, p);
		 
		smp_mb();
	}
}

static inline void skb_set_owner_w(struct sk_buff *skb, struct sock *sk)
{
	skb_orphan(skb);
	skb->sk = sk;
	skb->destructor = sock_wfree;
	 
	atomic_add(skb->truesize, &sk->sk_wmem_alloc);
}

static inline void skb_set_owner_r(struct sk_buff *skb, struct sock *sk)
{
	skb_orphan(skb);
	skb->sk = sk;
	skb->destructor = sock_rfree;
	atomic_add(skb->truesize, &sk->sk_rmem_alloc);
	sk_mem_charge(sk, skb->truesize);
}

extern void sk_reset_timer(struct sock *sk, struct timer_list* timer,
			   unsigned long expires);

extern void sk_stop_timer(struct sock *sk, struct timer_list* timer);

extern int sock_queue_rcv_skb(struct sock *sk, struct sk_buff *skb);

static inline int sock_queue_err_skb(struct sock *sk, struct sk_buff *skb)
{
	 
	if (atomic_read(&sk->sk_rmem_alloc) + skb->truesize >=
	    (unsigned)sk->sk_rcvbuf)
		return -ENOMEM;
	skb_set_owner_r(skb, sk);
	skb_queue_tail(&sk->sk_error_queue, skb);
	if (!sock_flag(sk, SOCK_DEAD))
		sk->sk_data_ready(sk, skb->len);
	return 0;
}

static inline int sock_error(struct sock *sk)
{
	int err;
	if (likely(!sk->sk_err))
		return 0;
	err = xchg(&sk->sk_err, 0);
	return -err;
}

static inline unsigned long sock_wspace(struct sock *sk)
{
	int amt = 0;

	if (!(sk->sk_shutdown & SEND_SHUTDOWN)) {
		amt = sk->sk_sndbuf - atomic_read(&sk->sk_wmem_alloc);
		if (amt < 0) 
			amt = 0;
	}
	return amt;
}

static inline void sk_wake_async(struct sock *sk, int how, int band)
{
	if (sk->sk_socket && sk->sk_socket->fasync_list)
		sock_wake_async(sk->sk_socket, how, band);
}

#define SOCK_MIN_SNDBUF 2048
#define SOCK_MIN_RCVBUF 256

static inline void sk_stream_moderate_sndbuf(struct sock *sk)
{
	if (!(sk->sk_userlocks & SOCK_SNDBUF_LOCK)) {
		sk->sk_sndbuf = min(sk->sk_sndbuf, sk->sk_wmem_queued >> 1);
		sk->sk_sndbuf = max(sk->sk_sndbuf, SOCK_MIN_SNDBUF);
	}
}

struct sk_buff *sk_stream_alloc_skb(struct sock *sk, int size, gfp_t gfp);

static inline struct page *sk_stream_alloc_page(struct sock *sk)
{
	struct page *page = NULL;

	page = alloc_pages(sk->sk_allocation, 0);
	if (!page) {
		sk->sk_prot->enter_memory_pressure(sk);
		sk_stream_moderate_sndbuf(sk);
	}
	return page;
}

static inline int sock_writeable(const struct sock *sk) 
{
	return atomic_read(&sk->sk_wmem_alloc) < (sk->sk_sndbuf >> 1);
}

static inline gfp_t gfp_any(void)
{
	return in_softirq() ? GFP_ATOMIC : GFP_KERNEL;
}

static inline long sock_rcvtimeo(const struct sock *sk, int noblock)
{
	return noblock ? 0 : sk->sk_rcvtimeo;
}

static inline long sock_sndtimeo(const struct sock *sk, int noblock)
{
	return noblock ? 0 : sk->sk_sndtimeo;
}

static inline int sock_rcvlowat(const struct sock *sk, int waitall, int len)
{
	return (waitall ? len : min_t(int, sk->sk_rcvlowat, len)) ? : 1;
}

static inline int sock_intr_errno(long timeo)
{
	return timeo == MAX_SCHEDULE_TIMEOUT ? -ERESTARTSYS : -EINTR;
}

extern void __sock_recv_timestamp(struct msghdr *msg, struct sock *sk,
	struct sk_buff *skb);

static __inline__ void
sock_recv_timestamp(struct msghdr *msg, struct sock *sk, struct sk_buff *skb)
{
	ktime_t kt = skb->tstamp;
	struct skb_shared_hwtstamps *hwtstamps = skb_hwtstamps(skb);

	if (sock_flag(sk, SOCK_RCVTSTAMP) ||
	    sock_flag(sk, SOCK_TIMESTAMPING_RX_SOFTWARE) ||
	    (kt.tv64 && sock_flag(sk, SOCK_TIMESTAMPING_SOFTWARE)) ||
	    (hwtstamps->hwtstamp.tv64 &&
	     sock_flag(sk, SOCK_TIMESTAMPING_RAW_HARDWARE)) ||
	    (hwtstamps->syststamp.tv64 &&
	     sock_flag(sk, SOCK_TIMESTAMPING_SYS_HARDWARE)))
		__sock_recv_timestamp(msg, sk, skb);
	else
		sk->sk_stamp = kt;
}

extern int sock_tx_timestamp(struct msghdr *msg,
			     struct sock *sk,
			     union skb_shared_tx *shtx);

#ifdef CONFIG_NET_DMA
static inline void sk_eat_skb(struct sock *sk, struct sk_buff *skb, int copied_early)
{
	__skb_unlink(skb, &sk->sk_receive_queue);
	if (!copied_early)
		__kfree_skb(skb);
	else
		__skb_queue_tail(&sk->sk_async_wait_queue, skb);
}
#else
static inline void sk_eat_skb(struct sock *sk, struct sk_buff *skb, int copied_early)
{
	__skb_unlink(skb, &sk->sk_receive_queue);
	__kfree_skb(skb);
}
#endif

static inline
struct net *sock_net(const struct sock *sk)
{
#ifdef CONFIG_NET_NS
	return sk->sk_net;
#else
	return &init_net;
#endif
}

static inline
void sock_net_set(struct sock *sk, struct net *net)
{
#ifdef CONFIG_NET_NS
	sk->sk_net = net;
#endif
}

static inline void sk_change_net(struct sock *sk, struct net *net)
{
	put_net(sock_net(sk));
	sock_net_set(sk, hold_net(net));
}

static inline struct sock *skb_steal_sock(struct sk_buff *skb)
{
	if (unlikely(skb->sk)) {
		struct sock *sk = skb->sk;

		skb->destructor = NULL;
		skb->sk = NULL;
		return sk;
	}
	return NULL;
}

extern void sock_enable_timestamp(struct sock *sk, int flag);
extern int sock_get_timestamp(struct sock *, struct timeval __user *);
extern int sock_get_timestampns(struct sock *, struct timespec __user *);

extern int net_msg_warn;
#define NETDEBUG(fmt, args...) \
	do { if (net_msg_warn) printk(fmt,##args); } while (0)

#define LIMIT_NETDEBUG(fmt, args...) \
	do { if (net_msg_warn && net_ratelimit()) printk(fmt,##args); } while(0)

extern __u32 sysctl_wmem_max;
extern __u32 sysctl_rmem_max;

extern void sk_init(void);

extern int sysctl_optmem_max;

extern __u32 sysctl_wmem_default;
extern __u32 sysctl_rmem_default;

#endif	 
