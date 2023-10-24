#ifndef _PFE_HIF_H_
#define _PFE_HIF_H_

#include <linux/netdevice.h>

#define HIF_NAPI_STATS

#define HIF_CLIENT_QUEUES_MAX	16
#define HIF_RX_POLL_WEIGHT	64

enum {
	NAPI_SCHED_COUNT = 0,
	NAPI_POLL_COUNT,
	NAPI_PACKET_COUNT,
	NAPI_DESC_COUNT,
	NAPI_FULL_BUDGET_COUNT,
	NAPI_CLIENT_FULL_COUNT,
	NAPI_MAX_COUNT
};


#if defined(CONFIG_COMCERTO_64K_PAGES)
#define PAGE_RATIO	16	/* This is (PAGE_SIZE / pfe_pkt_size), MUST be 4, 8 or 16 */
#else
#define PAGE_RATIO	1
#endif

/* XXX  HIF_TX_DESC_NT value should be always greter than 4,
 *      Otherwise HIF_TX_POLL_MARK will become zero.
 */
#if defined(CONFIG_PLATFORM_PCI)
#define HIF_RX_DESC_NT		4
#define HIF_TX_DESC_NT		4
#else
#if defined(CONFIG_COMCERTO_64K_PAGES)
    #if PAGE_RATIO > 1
    #define HIF_RX_DESC_NT		(PAGE_RATIO * 16)	/* Must be multiple of PAGE_RATIO */
    #else /* PAGE_RATIO = 1 */
    #define HIF_RX_DESC_NT		64
    #endif
#else
#define HIF_RX_DESC_NT		256
#endif
#define HIF_TX_DESC_NT		1024
#endif

#define HIF_FIRST_BUFFER	(1 << 0)
#define HIF_LAST_BUFFER		(1 << 1)
#define HIF_DONT_DMA_MAP	(1 << 2)
#define HIF_DATA_VALID		(1 << 3)

enum {
	PFE_CL_GEM0 = 0,
	PFE_CL_GEM1,
	PFE_CL_GEM2,
	PFE_CL_VWD,
	PFE_CL_EVENT,
	PFE_CL_RTP_CUT,
	PFE_CL_PCAP0,
	PFE_CL_PCAP1,
	PFE_CL_PCAP3,
	HIF_CLIENTS_MAX
};

/*structure to store client queue info */
struct hif_rx_queue {
	struct rx_queue_desc *base;
	u32	size;
	u32	write_idx;
};

struct hif_tx_queue {
	struct tx_queue_desc *base;
	u32	size;
	u32	ack_idx;
};

/*Structure to store the client info */
struct hif_client {
	int	rx_qn;
	struct hif_rx_queue 	rx_q[HIF_CLIENT_QUEUES_MAX];
	int	tx_qn;
	struct hif_tx_queue	tx_q[HIF_CLIENT_QUEUES_MAX];
};

/*HIF hardware buffer descriptor */
struct hif_desc {
	volatile u32 ctrl;
	volatile u32 status;
	volatile u32 data;
	volatile u32 next;
};

struct __hif_desc {
	u32 ctrl;
	u32 status;
	u32 data;
};

struct hif_desc_sw {
	dma_addr_t data;
	u16 len;
	u8 client_id;
	u8 q_no;
};

struct hif_hdr {
	u8 client_id;
	u8 qNo;
	u16 client_ctrl;
	u16 client_ctrl1;
};

struct __hif_hdr {
	union {
		struct hif_hdr hdr;
		u32 word[2];
	};
};

struct hif_lro_hdr {
	u16 data_offset;
	u16 mss;
};

struct hif_ipsec_hdr {
	u16	sa_handle[2];
}__attribute__((packed));

struct hif_tso_hdr {
	struct	hif_hdr pkt_hdr;
	u16	ip_off;
	u16	ip_id;
	u16	ip_len;
	u16	tcp_off;
	u32	tcp_seq;
} __attribute__((packed));


/*  HIF_CTRL_TX... defines */
#define HIF_CTRL_TX_IPSEC_OUT	(1 << 7)
#define HIF_CTRL_TX_OWN_MAC		(1 << 6)
#define HIF_CTRL_TX_TSO_END		(1 << 5)
#define HIF_CTRL_TX_TSO6		(1 << 4)
#define HIF_CTRL_TX_TSO			(1 << 3)
#define HIF_CTRL_TX_CHECKSUM	(1 << 2)

/*  HIF_CTRL_RX... defines */
#define HIF_CTRL_RX_IPSEC_IN	(1 << 4)
#define HIF_CTRL_RX_WIFI_EXPT	(1 << 3)
#define HIF_CTRL_RX_CHECKSUMMED	(1 << 2)
#define HIF_CTRL_RX_CONTINUED	(1 << 1)
#define HIF_CTRL_VAPID_OFST		8

struct pfe_hif {
	/* To store registered clients in hif layer */
	struct hif_client client[HIF_CLIENTS_MAX];
	struct hif_shm *shm;
	int	irq;

	void	*descr_baseaddr_v;
	unsigned long	descr_baseaddr_p;

	struct hif_desc *RxBase;
	u32	RxRingSize;
	u32	RxtocleanIndex;
	void	*rx_buf_addr[HIF_RX_DESC_NT];
	unsigned int qno;
	unsigned int client_id;
	unsigned int client_ctrl;
	unsigned int started;
	void *page;
	unsigned int page_off;

	struct hif_desc *TxBase;
	u32	TxRingSize;
	u32	Txtosend;
	u32	Txtoclean;
	u32	TxAvail;
	struct hif_desc_sw tx_sw_queue[HIF_TX_DESC_NT];
	struct hif_tso_hdr *tso_hdr_v;
	dma_addr_t tso_hdr_p;

	spinlock_t tx_lock;
	spinlock_t lock;
	struct net_device	dummy_dev;
	struct napi_struct	napi;
	struct device *dev;

#ifdef HIF_NAPI_STATS
	unsigned int napi_counters[NAPI_MAX_COUNT];
#endif
};

void __hif_xmit_pkt(struct pfe_hif *hif, unsigned int client_id, unsigned int q_no, void *data, u32 len, unsigned int flags);
int hif_xmit_pkt(struct pfe_hif *hif, unsigned int client_id, unsigned int q_no, void *data, unsigned int len);
void __hif_tx_done_process(struct pfe_hif *hif, int count);
void hif_process_client_req(struct pfe_hif *hif, int req, int data1, int data2);
int pfe_hif_init(struct pfe *pfe);
void pfe_hif_exit(struct pfe *pfe);

static inline void hif_tx_done_process(struct pfe_hif *hif, int count)
{
	spin_lock_bh(&hif->tx_lock);
	__hif_tx_done_process(hif, count);
	spin_unlock_bh(&hif->tx_lock);
}

static inline void hif_tx_lock(struct pfe_hif *hif)
{
	spin_lock_bh(&hif->tx_lock);
}

static inline void hif_tx_unlock(struct pfe_hif *hif)
{
	spin_unlock_bh(&hif->tx_lock);
}

static inline int __hif_tx_avail(struct pfe_hif *hif)
{
	return hif->TxAvail;
}

static inline void __memcpy8(void *dst, void *src)
{
	asm volatile (	"ldm %1, {r9, r10}\n\t"
			"stm %0, {r9, r10}\n\t"
			:
			: "r" (dst), "r" (src)
			: "r9", "r10", "memory"
		);
}

static inline void __memcpy12(void *dst, void *src)
{
	asm volatile (	"ldm %1, {r8, r9, r10}\n\t"
			"stm %0, {r8, r9, r10}\n\t"
			:
			: "r" (dst), "r" (src)
			: "r8", "r9", "r10", "memory"
		);
}

static inline void __memcpy16(void *dst, void *src)
{
	asm volatile (	"ldm %1, {r7, r8, r9, r10}\n\t"
			"stm %0, {r7, r8, r9, r10}\n\t"
			:
			: "r"(dst), "r"(src)
			: "r7", "r8", "r9", "r10", "memory"
		);
}

static inline void __memcpy(void *dst, void *src, unsigned int len)
{
	void *end = src + len;

	dst = (void *)((unsigned long)dst & ~0x3);
	src = (void *)((unsigned long)src & ~0x3);

	while (src < end) {
		asm volatile (	"ldm %1!, {r3, r4, r5, r6, r7, r8, r9, r10}\n\t"
				"stm %0!, {r3, r4, r5, r6, r7, r8, r9, r10}\n\t"
				: "+r"(dst), "+r"(src)
				:
				: "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "memory"
			);
	}
}

#endif /* _PFE_HIF_H_ */
