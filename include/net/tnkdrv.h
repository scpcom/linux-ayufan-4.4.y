/*
 *  COPYRIGHT NOTICE
 *
 *  Copyright 2011 Emutex Ltd. All rights reserved.
 *
 *  tnk info structure
 */

#ifndef TNKDRV_H
#define TNKDRV_H

#include <linux/fs.h>

typedef int (*tnk_read_actor_t)(read_descriptor_t *, struct sk_buff *,
				unsigned int, size_t);

#ifndef CONFIG_TNK
#error "You must enable the CONFIG_TNK kernel configuration option"
#endif

#define SWITCH_DUAL_TXBUF       1 /* bit 0 */
#define SWITCH_RETRANS_TWO_PKT  1 /* bit 1 */
#define SWITCH_DUPACK_NUM       1 /* bit 2 */
#define SWITCH_RETRANS_MAX_CNT  1 /* bit 3 */
#define SWITCH_SEND_ACK         1 /* bit 4 */
#define SWITCH_REMOVE_TTX_ERR   1 /* bit 5 */
#define SWITCH_TOE_LOOPBACK     0 /* bit 6 */
#define SWITCH_RCVBUF_ADDR      1 /* bit 7 */
#define SWITCH_MULTI_INTR       1 /* bit 8 */
#define SWITCH_RTT_CACULATE     1 /* bit 9 */
#define SWITCH_IP_HEADER_ID     1 /* bit 10 */
#define SWITCH_ZERO_PROBE       1 /* bit 11 */
#define SWITCH_CORK             1 /* bit 12 */
#define SWITCH_NAGLE            1 /* bit 13 */
#define SWITCH_KEEPALIVE        1 /* bit 14 */
#define SWITCH_SEND_FIN         0 /* bit 15 */
#define SWITCH_THIN_STREAM      1 /* bit 16 */
#define SWITCH_MULTI_INTR_RETRANS       1 /* bit 17 */
#define SWITCH_RECV_LRO		1 /* bit 18 */

#if SWITCH_RECV_LRO
#define SWITCH_HW_UPDATE_WND		1 /* bit 19 */
#else
#define SWITCH_HW_UPDATE_WND		0 /* bit 19 */
#endif

#define SWITCH_FOUR_OFO		1 /* bit 20 */
#define SWITCH_TOE_ERR_PKT	1 /* bit 21 */
#define SWITCH_CV_FIFO		1 /* bit 22 */
#define SWITCH_PROBE_SEQ	1 /* bit 23 */
#define SWITCH_TTX_FLUSH	1 /* bit 24 */

#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
#define SWITCH_VLAN		1
#else
#define SWITCH_VLAN		0
#endif

#if SWITCH_MULTI_INTR
#define TOE_MULTI_INTR_NUM      CONFIG_STMMAC_MULTI_IRQ_NUM
#endif

/* Forward declarations required below */
struct tnkentry;
struct net_device;
struct sk_buff;
struct sock;

struct tnkcb {
	unsigned magic;
	int type;
	dma_addr_t dma;
	unsigned flag;
};
#define TNK_MAGIC 0x6ac169f3

enum tnkhw_cb_flags {
	TNKHW_CB_FLAG_NONE = 0,
	TNKHW_CB_FLAG_PREMATURE_EOP = (1 << 0),
	TNKHW_CB_FLAG_BAD_ETHERTYPE = (1 << 1),
	TNKHW_CB_FLAG_BAD_DST_MACADDR = (1 << 2),
	TNKHW_CB_FLAG_TCP_CSUM_ERR = (1 << 3),
	TNKHW_CB_FLAG_IPHDR_CSUM_ERR = (1 << 4),
	TNKHW_CB_FLAG_NON_TCP_PROT = (1 << 5),
	TNKHW_CB_FLAG_DST_IP_MISMATCH = (1 << 6),
	TNKHW_CB_FLAG_CONNECTION_ERR = (1 << 7),
	TNKHW_CB_FLAG_RX_SEQNUM_ERR = (1 << 8),
	TNKHW_CB_FLAG_PKT_DROPPED = (1 << 9),
	TNKHW_CB_FLAG_RESERVED = (1 << 10),
	TNKHW_CB_FLAG_FULL_PKT = (1 << 11),
	TNKHW_CB_FLAG_TOE_ERR = (1 << 12),
	TNKHW_CB_FLAG_URG = (1 << 13),
	TNKHW_CB_FLAG_ACK = (1 << 14),
	TNKHW_CB_FLAG_PSH = (1 << 15),
	TNKHW_CB_FLAG_RST = (1 << 16),
	TNKHW_CB_FLAG_SYN = (1 << 17),
	TNKHW_CB_FLAG_FIN = (1 << 18),
};

#define TNK_TYPE_DATA   0
#define TNK_TYPE_FIN    1
#define TNK_TYPE_RST    2
#define TNK_TYPE_FREE   3
#define TNK_TYPE_LRO_DATA	4

#define TNK_SOCK_NOT_SET_TOE	0
#define TNK_SOCK_SET_TOE_OFF	1
#define TNK_SOCK_SET_TOE_ON	2

/* Socket is created but not connected or accelerated */
#define TNKINFO_STATE_OPEN        0
/* Socket is created and has a connection but not accelerated */
#define TNKINFO_STATE_PREPARED    1
/* Socket is created and has a connection but not accelerated */
#define TNKINFO_STATE_ESTABLISHED 2
/* Socket is connected and acceleration handover has begun */
#define TNKINFO_STATE_ACTIVATING  3
 /* Socket is connected and accelerated */
#define TNKINFO_STATE_ACTIVE      4
/* Socket is connected but no longer accelerated */
#define TNKINFO_STATE_STOPPING    5
/* Socket is connected but no longer accelerated */
#define TNKINFO_STATE_STOPPED     6
#define TNKINFO_STATE_CLOSED      7

#define TNK_DESTROY_CLOSE	0
#define TNK_DESTROY_SHUTDOWN	1

enum tnk_update_path {
	UPDATE_PATH_UNKNOWN = 0,
	UPDATE_PATH_SEND,
	UPDATE_PATH_RECV
};

struct tnkinfo {
	int state;
	int bytecount;
	unsigned int snd_initial_seq;
	/* used by kernel path to decide whether allowed to use toe */
	int enable;
	struct tnkentry *entry;
	struct net_device *dev;
	uint8_t rmac[6];
	int gmac_id;
	struct timer_list idle_timer;
	int finflag;
	int rstflag;
	int reetrant;
	int howto_destroy;
	struct tnk_tcp_close_work *close_work;
	int update_path;
	/* used by user settings , some socket setting use special
	   function, but toe can't afford it , so it's not allowed
	   to use toe, for example,when traffic control is set,toe
	   can not be used, the initial value is zero .
	 */
	int not_capable;
#if SWITCH_MULTI_INTR
	unsigned int cur_cpu;
#endif
};

struct tnkfuncs {
	int (*tcp_prepare) (struct sock *sk, struct sk_buff *skb);
	int (*tcp_open) (struct sock *sk);
	int (*tcp_close) (struct sock *sk, int graceful);
	int (*tcp_reset) (struct sock *sk);
	int (*tcp_sendpages) (struct sock *s, struct page *page,
			int offset, size_t size, int flags);
	int (*tcp_sendmsg) (struct sock *s, struct iovec *iov, int iovlen,
			int flags);
	int (*tcp_receive) (struct sock *sk, uint32_t *seq, long *timeo,
			int target, struct iovec *to, int *has_copied,
			size_t *len, int fin_flag, int sk_flags);
	int (*tcp_read_sock) (struct sock *sk, read_descriptor_t *desc,
			tnk_read_actor_t recv_actor, int *has_copied);
	int (*tcp_update) (struct sock *s, int len);
	int (*tcp_wait_data) (struct sock *sk, long *timeo);
	int (*tcp_check_fin) (struct sock *sk);
	int (*tcp_check_connect_state) (struct sock *sk);
	void (*tcp_disable_rcv) (struct sock *sk);
	int (*tcp_recv_queue_data_size) (struct sock *sk);
	void (*tcp_sync_mss) (struct sock *sk);
	void (*config_tcp_retries2) (int tcp_retries2);
#if SWITCH_DUPACK_NUM
	void (*config_tcp_dupack_cnt) (int tcp_reordering);
#endif
	void (*set_thin_linear_timeouts) (int thin_linear_timeouts);
	void (*tcp_set_thin_linear_timeouts) (struct sock *sk);
	void (*tcp_set_nagle_cork) (struct sock *sk);
#if SWITCH_KEEPALIVE
	int (*tcp_check_tx_queue) (struct sock *sk);
	int (*tcp_keepalive) (struct sock *sk);
#endif
#if SWITCH_SEND_FIN
	int (*tcp_send_fin) (struct sock *sk);
#endif
};

void tcp_register_tnk_funcs(struct tnkfuncs *f);

#endif
