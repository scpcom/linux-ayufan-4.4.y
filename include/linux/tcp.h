#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _LINUX_TCP_H
#define _LINUX_TCP_H

#include <linux/skbuff.h>
#include <linux/dmaengine.h>
#include <net/sock.h>
#include <net/inet_connection_sock.h>
#include <net/inet_timewait_sock.h>
#include <uapi/linux/tcp.h>

static inline struct tcphdr *tcp_hdr(const struct sk_buff *skb)
{
	return (struct tcphdr *)skb_transport_header(skb);
}

static inline unsigned int tcp_hdrlen(const struct sk_buff *skb)
{
	return tcp_hdr(skb)->doff * 4;
}

static inline struct tcphdr *inner_tcp_hdr(const struct sk_buff *skb)
{
	return (struct tcphdr *)skb_inner_transport_header(skb);
}

static inline unsigned int inner_tcp_hdrlen(const struct sk_buff *skb)
{
	return inner_tcp_hdr(skb)->doff * 4;
}

static inline unsigned int tcp_optlen(const struct sk_buff *skb)
{
	return (tcp_hdr(skb)->doff - 5) * 4;
}

#define TCP_FASTOPEN_COOKIE_MIN	4	 
#define TCP_FASTOPEN_COOKIE_MAX	16	 
#define TCP_FASTOPEN_COOKIE_SIZE 8	 

struct tcp_fastopen_cookie {
	s8	len;
	u8	val[TCP_FASTOPEN_COOKIE_MAX];
};

struct tcp_sack_block_wire {
	__be32	start_seq;
	__be32	end_seq;
};

struct tcp_sack_block {
	u32	start_seq;
	u32	end_seq;
};

#define TCP_SACK_SEEN     (1 << 0)    
#define TCP_FACK_ENABLED  (1 << 1)    
#define TCP_DSACK_SEEN    (1 << 2)    

struct tcp_options_received {
 
	long	ts_recent_stamp; 
	u32	ts_recent;	 
	u32	rcv_tsval;	 
	u32	rcv_tsecr;	 
	u16 	saw_tstamp : 1,	 
		tstamp_ok : 1,	 
		dsack : 1,	 
		wscale_ok : 1,	 
		sack_ok : 4,	 
		snd_wscale : 4,	 
		rcv_wscale : 4;	 
	u8	num_sacks;	 
	u16	user_mss;	 
	u16	mss_clamp;	 
};

static inline void tcp_clear_options(struct tcp_options_received *rx_opt)
{
	rx_opt->tstamp_ok = rx_opt->sack_ok = 0;
	rx_opt->wscale_ok = rx_opt->snd_wscale = 0;
}

#define TCP_NUM_SACKS 4

struct tcp_cookie_values;
struct tcp_request_sock_ops;

struct tcp_request_sock {
	struct inet_request_sock 	req;
#ifdef CONFIG_TCP_MD5SIG
	 
	const struct tcp_request_sock_ops *af_specific;
#endif
	struct sock			*listener;  
	u32				rcv_isn;
	u32				snt_isn;
	u32				snt_synack;  
	u32				rcv_nxt;  
};

static inline struct tcp_request_sock *tcp_rsk(const struct request_sock *req)
{
	return (struct tcp_request_sock *)req;
}

struct tcp_sock {
	 
	struct inet_connection_sock	inet_conn;
	u16	tcp_header_len;	 
	u16	xmit_size_goal_segs;  

	__be32	pred_flags;

 	u32	rcv_nxt;	 
	u32	copied_seq;	 
	u32	rcv_wup;	 
 	u32	snd_nxt;	 

 	u32	snd_una;	 
 	u32	snd_sml;	 
	u32	rcv_tstamp;	 
	u32	lsndtime;	 

	u32	tsoffset;	 

	struct list_head tsq_node;  
	unsigned long	tsq_flags;

	struct {
		struct sk_buff_head	prequeue;
		struct task_struct	*task;
		struct iovec		*iov;
		int			memory;
		int			len;
#ifdef CONFIG_NET_DMA
		 
		struct dma_chan		*dma_chan;
		int			wakeup;
		struct dma_pinned_list	*pinned_list;
#if defined(MY_DEF_HERE)
		bool			pinned;
#endif  
		dma_cookie_t		dma_cookie;
#endif
	} ucopy;

	u32	snd_wl1;	 
	u32	snd_wnd;	 
	u32	max_window;	 
	u32	mss_cache;	 

	u32	window_clamp;	 
	u32	rcv_ssthresh;	 

	u16	advmss;		 
	u8	unused;
	u8	nonagle     : 4, 
		thin_lto    : 1, 
		thin_dupack : 1, 
		repair      : 1,
		frto        : 1; 
	u8	repair_queue;
	u8	do_early_retrans:1, 
		syn_data:1,	 
		syn_fastopen:1,	 
		syn_data_acked:1; 
	u32	tlp_high_seq;	 

	u32	srtt;		 
	u32	mdev;		 
	u32	mdev_max;	 
	u32	rttvar;		 
	u32	rtt_seq;	 

	u32	packets_out;	 
	u32	retrans_out;	 

	u16	urg_data;	 
	u8	ecn_flags;	 
	u8	reordering;	 
	u32	snd_up;		 

	u8	keepalive_probes;  
 
	struct tcp_options_received rx_opt;

 	u32	snd_ssthresh;	 
 	u32	snd_cwnd;	 
	u32	snd_cwnd_cnt;	 
	u32	snd_cwnd_clamp;  
	u32	snd_cwnd_used;
	u32	snd_cwnd_stamp;
	u32	prior_cwnd;	 
	u32	prr_delivered;	 
	u32	prr_out;	 

 	u32	rcv_wnd;	 
	u32	write_seq;	 
	u32	pushed_seq;	 
	u32	lost_out;	 
	u32	sacked_out;	 
	u32	fackets_out;	 
	u32	tso_deferred;

	struct sk_buff* lost_skb_hint;
	struct sk_buff *scoreboard_skb_hint;
	struct sk_buff *retransmit_skb_hint;

	struct sk_buff_head	out_of_order_queue;  

	struct tcp_sack_block duplicate_sack[1];  
	struct tcp_sack_block selective_acks[4];  

	struct tcp_sack_block recv_sack_cache[4];

	struct sk_buff *highest_sack;    

	int     lost_cnt_hint;
	u32     retransmit_high;	 

	u32	lost_retrans_low;	 

	u32	prior_ssthresh;  
	u32	high_seq;	 

	u32	retrans_stamp;	 
	u32	undo_marker;	 
	int	undo_retrans;	 
	u32	total_retrans;	 

	u32	urg_seq;	 
	unsigned int		keepalive_time;	   
	unsigned int		keepalive_intvl;   

	int			linger2;

	struct {
		u32	rtt;
		u32	seq;
		u32	time;
	} rcv_rtt_est;

	struct {
		int	space;
		u32	seq;
		u32	time;
	} rcvq_space;

	struct {
		u32		  probe_seq_start;
		u32		  probe_seq_end;
	} mtu_probe;
	u32	mtu_info;  

#ifdef CONFIG_TCP_MD5SIG
 
	const struct tcp_sock_af_ops	*af_specific;

	struct tcp_md5sig_info	__rcu *md5sig_info;
#endif

	struct tcp_fastopen_request *fastopen_req;
	 
	struct request_sock *fastopen_rsk;
};

enum tsq_flags {
	TSQ_THROTTLED,
	TSQ_QUEUED,
	TCP_TSQ_DEFERRED,	    
	TCP_WRITE_TIMER_DEFERRED,   
	TCP_DELACK_TIMER_DEFERRED,  
	TCP_MTU_REDUCED_DEFERRED,   
};

static inline struct tcp_sock *tcp_sk(const struct sock *sk)
{
	return (struct tcp_sock *)sk;
}

struct tcp_timewait_sock {
	struct inet_timewait_sock tw_sk;
	u32			  tw_rcv_nxt;
	u32			  tw_snd_nxt;
	u32			  tw_rcv_wnd;
	u32			  tw_ts_offset;
	u32			  tw_ts_recent;
	long			  tw_ts_recent_stamp;
#ifdef CONFIG_TCP_MD5SIG
	struct tcp_md5sig_key	  *tw_md5_key;
#endif
};

static inline struct tcp_timewait_sock *tcp_twsk(const struct sock *sk)
{
	return (struct tcp_timewait_sock *)sk;
}

static inline bool tcp_passive_fastopen(const struct sock *sk)
{
	return (sk->sk_state == TCP_SYN_RECV &&
		tcp_sk(sk)->fastopen_rsk != NULL);
}

static inline bool fastopen_cookie_present(struct tcp_fastopen_cookie *foc)
{
	return foc->len != -1;
}

extern void tcp_sock_destruct(struct sock *sk);

static inline int fastopen_init_queue(struct sock *sk, int backlog)
{
	struct request_sock_queue *queue =
	    &inet_csk(sk)->icsk_accept_queue;

	if (queue->fastopenq == NULL) {
		queue->fastopenq = kzalloc(
		    sizeof(struct fastopen_queue),
		    sk->sk_allocation);
		if (queue->fastopenq == NULL)
			return -ENOMEM;

		sk->sk_destruct = tcp_sock_destruct;
		spin_lock_init(&queue->fastopenq->lock);
	}
	queue->fastopenq->max_qlen = backlog;
	return 0;
}

#endif	 
