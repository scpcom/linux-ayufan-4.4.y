#ifndef _PE_H_
#define _PE_H_

#include "hal.h"

#if defined(COMCERTO_2000_CLASS)
#include "pfe/class.h"
#elif defined(COMCERTO_2000_TMU)
#include "pfe/tmu.h"
#elif defined(COMCERTO_2000_UTIL)
#include "pfe/util.h"
#endif

enum {
	CLASS0_ID = 0,
	CLASS1_ID,
	CLASS2_ID,
	CLASS3_ID,
	CLASS4_ID,
	CLASS5_ID,
	TMU0_ID,
	TMU1_ID,
	TMU2_ID,
	TMU3_ID,
	UTIL_ID,
	MAX_PE
};

enum {
	PHY0_PORT = 0x0,
	PHY1_PORT,
	PHY2_PORT,
	HIF_PORT,
	HIF_NOCPY_PORT
};

enum {
	TMU0_PORT = 0x0,
	TMU1_PORT,
	TMU2_PORT,
	TMU3_PORT
};


#define DDR_BASE_ADDR		0x00020000
#define DDR_END			0x86000000 /* This includes ACP and IRAM areas */
#define IRAM_BASE_ADDR		0x83000000

#define IS_DDR(addr, len)	(((unsigned long)(addr) >= DDR_BASE_ADDR) && (((unsigned long)(addr) + (len)) <= DDR_END))

typedef struct {
	u8	start_data_off;		/* packet data start offset, relative to start of this tx pre-header */
	u8	start_buf_off;		/* this tx pre-header start offset, relative to start of DDR buffer */
	u16	pkt_length;		/* total packet length */
	u8	act_phyno;		/* action / phy number */
	u8	queueno;		/* queueno */
	u16	unused;
} class_tx_hdr_t;

typedef struct {
	u8	start_data_off;		/* packet data start offset, relative to start of this tx pre-header */
	u8	start_buf_off;		/* this tx pre-header start offset, relative to start of DDR buffer */
	u16	pkt_length;		/* total packet length */
	u8	act_phyno;		/* action / phy number */
	u8	queueno;		/* queueno */
	u16	src_mac_msb;		/* indicates src_mac 47:32 */
	u32	src_mac_lsb;		/* indicates src_mac 31:0 */
	u32	vlanid;			/* vlanid */
} class_tx_hdr_mc_t;

typedef struct {
        u32     next_ptr;       /* ptr to the start of the first DDR buffer */
        u16     length;         /* total packet length */
        u16     phyno;          /* input physical port number */
        u32     status;         /* gemac status bits bits[32:63]*/
        u32     status2;        /* gemac status bits bits[0:31] */
} class_rx_hdr_t;
/*class_rx_hdr status bits */
#define STATUS_CUMULATIVE_ERR		(1<<16)
#define STATUS_LENGTH_ERR		(1<<17)
#define STATUS_CRC_ERR			(1<<18)
#define STATUS_TOO_SHORT_ERR		(1<<19)
#define STATUS_TOO_LONG_ERR		(1<<20)
#define STATUS_CODE_ERR			(1<<21)
#define STATUS_MC_HASH_MATCH		(1<<22)
#define STATUS_CUMULATIVE_ARC_HIT	(1<<23)
#define STATUS_UNICAST_HASH_MATCH	(1<<24)
#define STATUS_IP_CHECKSUM_CORRECT	(1<<25)
#define STATUS_TCP_CHECKSUM_CORRECT	(1<<26)
#define STATUS_UDP_CHECKSUM_CORRECT	(1<<27)
#define STATUS_OVERFLOW_ERR		(1<<28)

#define PHYNO_UTIL_INBOUND		(1<<15) /**< Control bit (in PHYNO field) used to inform CLASS PE that packet comes from UtilPE. */
#define PHYNO_UTIL_OFFSET		11		/**< Offset in PHYNO field where the original phyno will be stored for packets coming from UtilPE. */

/** Structure passed from UtilPE to Class, stored at the end of the LMEM buffer. Defined and used by software only.
 *
 */
typedef struct {
	u8 packet_type;
	u8 offset;
	u16 mtd_flags;
	union {
		u16 half[6];
		u8 byte[12];

		struct ipsec_info_out {
				u16 flags;
				u8 sa_adj_len;
				u8 proto;
				U16 sa_handle[2]; // SA_MAX_OP value should be used here instead of 2
				S8 sa_op;
				u8 l2hdr_len;
				u8 adj_dmem;
				u8 reserved[1];
		} ipsec;

		struct relay_info_out {
				u16 l4offset;
				u16 socket_id;
				BOOL update;
				u8 reserved;
				u32 payload_diff;
		} relay;
		struct frag_info_out {
				u16 l3offset;
		} frag;
	};
} lmem_trailer_t;

#define UTIL_TX_TRAILER_SIZE	sizeof(lmem_trailer_t)
#define UTIL_TX_TRAILER(mtd)	((lmem_trailer_t *)(((u32)(mtd)->rx_dmem_end + 3) & ~0x3))

typedef struct {
	u32 pkt_ptr;
	u8  phyno;
	u8  queueno;
	u16 len;
} tmu_tx_hdr_t;

struct hif_pkt_hdr {		
	u8	client_id;
	u8	qNo;
	u32	client_ctrl_le;
} __attribute__((packed));


/* These match LE definition */
#define HIF_CTRL_TX_IPSEC_OUT	__cpu_to_le32(1 << 7)
#define HIF_CTRL_TX_WIFI_OWNMAC	__cpu_to_le32(1 << 6)
#define HIF_CTRL_TX_TSO_END		__cpu_to_le32(1 << 5)
#define HIF_CTRL_TX_TSO6		__cpu_to_le32(1 << 4)
#define HIF_CTRL_TX_TSO			__cpu_to_le32(1 << 3)
#define HIF_CTRL_TX_CHECKSUM	__cpu_to_le32(1 << 2)

#define HIF_CTRL_RX_OFFSET_MASK	__cpu_to_le32(0xf << 24)
#define HIF_CTRL_RX_PE_ID_MASK	__cpu_to_le32(0xf << 16)
#define HIF_CTRL_RX_VAP_ID_MASK	__cpu_to_le32(0xf << 8)
#define HIF_CTRL_RX_IPSEC_IN	__cpu_to_le32(1 << 4)
#define HIF_CTRL_RX_WIFI_EXPT	__cpu_to_le32(1 << 3)
#define HIF_CTRL_RX_CHECKSUMMED	__cpu_to_le32(1 << 2)
#define HIF_CTRL_RX_CONTINUED	__cpu_to_le32(1 << 1)
#define HIF_CTRL_VAPID_OFST		8

struct hif_lro_hdr {
	u16 data_offset;
	u16 mss;
};

struct hif_ipsec_hdr {
	u16 sa_handle[2];
} __attribute__((packed));

struct hif_tso_hdr {
	u16 ip_off;
	u16 ip_id;
	u16 ip_len;
	u16 tcp_off;
	u32 tcp_seq;
} __attribute__((packed));

struct pe_sync_mailbox
{
	u32 stop;
	u32 stopped;
};

struct pe_msg_mailbox
{
	u32 dst;
	u32 src;
	u32 len;
	u32 request;
};


/** Basic busy loop delay function
*
* @param cycles		Number of cycles to delay (actual cpu cycles should be close to 3 x cycles)
*
*/
static inline void delay(u32 cycles)
{
	volatile int i;

	for (i = 0; i < cycles; i++);
}


/** Read PE id
*
* @return	PE id (0 - 5 for CLASS-PE's, 6 - 9 for TMU-PE's, 10 for UTIL-PE)
*
*/
static inline u32 esi_get_mpid(void)
{
	u32 mpid;

	asm ("rcsr %0, Configuration, MPID" : "=d" (mpid));

	return mpid;
}


#define esi_get_csr(bank, csr) \
({ \
	u32 res; \
	asm ("rcsr %0, " #bank ", " #csr : "=d" (res)); \
	res; \
})

#define esi_get_isa0() esi_get_csr(Configuration, ISA0)
#define esi_get_isa1() esi_get_csr(Configuration, ISA1)
#define esi_get_isa2() esi_get_csr(Configuration, ISA2)
#define esi_get_isa3() esi_get_csr(Configuration, ISA3)
#define esi_get_epc() esi_get_csr(Thread, EPC)
#define esi_get_ecas() esi_get_csr(Thread, ECAS)
#define esi_get_eid() esi_get_csr(Thread, EID)
#define esi_get_ed() esi_get_csr(Thread, ED)

static inline void esi_pe_stop(void)
{
	PESTATUS_SETSTATE('STOP');
	while (1)
	{
		asm("stop");
	}
}


/** Same 64bit alignment memory copy using efet.
* Either the source or destination address must be in DMEM, the other address can be in LMEM or DDR.
* Both the source and destination must have the same 64bit alignment, length should be more than four bytes
* or dst/src must be 32bit aligned. Otherwise use efet_memcpy_any()
* Uses efet synchronous interface to copy the data.
*
* @param dst	Destination address to write to (must have the same 64bit alignment as src)
* @param src	Source address to read from (must have the same 64bit alignment as dst)
* @param len	Number of bytes to copy
*
*/
void efet_memcpy(void *dst, void *src, unsigned int len);

/** Same 64bit alignment memory copy using efet.
* Either the source or destination address must be in DMEM, the other address can be in LMEM or DDR.
* Both the source and destination must have the same 64bit alignment, there is no restriction on length.
* For UTIL-PE revA0, this function will still fail to handle small/unaligned writes.
* Uses efet synchronous interface to copy the data.
*
* @param dst	Destination address to write to (must have the same 64bit alignment as src)
* @param src	Source address to read from (must have the same 64bit alignment as dst)
* @param len	Number of bytes to copy
*
*/
void efet_memcpy_any(void *dst, void *src, unsigned int len);

/** Same 64bit alignment memory copy using efet.
* Either the source or destination address must be in DMEM, the other address can be in LMEM or DDR.
* Both the source and destination must have the same 64bit alignment, length should be more than four bytes
* or dst/src must be 32bit aligned.
* Uses efet asynchronous interface to copy the data.
*
* @param dst	Destination address to write to (must have the same 64bit alignment as src)
* @param src	Source address to read from (must have the same 64bit alignment as dst)
* @param len	Number of bytes to copy
*
*/
void efet_memcpy_nowait(void *dst, void *src, unsigned int len);

/** Unaligned memory copy using efet.
* Either the source or destination address must be in DMEM, the other address can be in LMEM or DDR.
* There is not restriction on source and destination, nor on length.
*
* @param dst		Destination address to write to
* @param src		Source address to read from
* @param len		Number of bytes to copy
* @param dmem_buf	temp dmem buffer to use, must be 64bit aligned
* @param dmem_len	length of dmem buffer, must be 64bit aligned and at least 16 bytes
*
*/
void efet_memcpy_unaligned(void *dst, void *src, unsigned int len, void *dmem_buf, unsigned int dmem_len);

/** Aligned memory copy of 4 bytes to register address.
* Register address must be 32 bit aligned.
*
* @param val		value to be copied.       
* @param reg_addr	Register address (must be 16bit aligned)
*
*/
void __efet_writel(u32 val, void *addr);

#ifdef REVA_WA
#define efet_writel(val, addr)	__efet_writel((u32)(val), (void *) (addr))
#else
#define efet_writel(val, addr)	writel((u32)(val), (void *) (addr))
#endif


/** 32bit aligned memory copy.
* Source and destination addresses must be 32bit aligned, there is no restriction on the length.
*
* @param dst		Destination address (must be 32bit aligned)
* @param src		Source address (must be 32bit aligned)
* @param len		Number of bytes to copy
*
*/
void memcpy_aligned32(void *dst, void *src, unsigned int len);

/** Aligned memory copy.
* Source and destination addresses must have the same alignment
* relative to 32bit boundaries (but otherwsie may have any alignment),
* there is no restriction on the length.
*
* @param dst		Destination address
* @param src		Source address (must have same 32bit alignment as dst)
* @param len		Number of bytes to copy
*
*/
void memcpy_aligned(void *dst, void *src, unsigned int len);

/** Unaligned memory copy.
* Implements unaligned memory copy. We first align the destination
* to a 32bit boundary (using byte copies) then the src, and finally use a loop
* of read, shift, write
*
* @param dst		Destination address
* @param src		Source address (must have same 32bit alignment as dst)
* @param len		Number of bytes to copy
*
*/
void memcpy_unaligned(void *dst, void *src, unsigned int len);

/** Generic memory set.
* Implements a generic memory set. Not very optimal (uses byte writes for the entire range)
*
*
* @param dst		Destination address
* @param val		Value to set memory to
* @param len		Number of bytes to set
*
*/
void memset(void *dst, u8 val, unsigned int len);

/** Generic memory copy.
* Implements generic memory copy. If source and destination have the same
* alignment memcpy_aligned() is used, otherwise memcpy_unaligned()
*
* @param dst		Destination address
* @param src		Source address
* @param len		Number of bytes to copy
*
*/
void memcpy(void *dst, void *src, unsigned int len);

/** Aligned memory copy in DDR memory.
 * Implements aligned memory copy between two DDR buffers using efet_memcpy64 and DMEM
 * Both the source and destination must have the same 64bit alignment, there is no restriction on length.
 * If start or end are not 64bit aligned, data in destination buffer before start/after end will be corrupted.
 *
 * @param dst 		DDR Destination address
 * @param src		DDR Source address
 * @param len		Number of bytes to copy
 * @param dmem_buf	temp dmem buffer to use, must be 64bit aligned
 * @param dmem_len	length of dmem buffer, must be 64bit aligned and at least 16 bytes
 */
void memcpy_ddr_to_ddr(void *dst, void *src, unsigned int len, void *dmem_buf, unsigned int dmem_len);

/** Unaligned memory copy in DDR memory.
 * Implements generic memory copy between two DDR buffers using efet_memcpy and DMEM
 * There is no restriction on the source, destination and length alignments.
 *
 * @param dst 		DDR Destination address
 * @param src		DDR Source address
 * @param len		Number of bytes to copy
 * @param dmem_buf	temp dmem buffer to use, must be 64bit aligned
 * @param dmem_len	length of dmem buffer, must be 64bit aligned and at least 16 bytes
 */
void memcpy_ddr_to_ddr_unaligned(void *dst, void *src, unsigned int len, void *dmem_buf, unsigned int dmem_len);



void send_to_output_port(u8 port, u8 queue, void *src_addr, void *ddr_addr, u16 hdr_len, u16 len);
void send_to_host_port(void *dmem_addr, void *ddr_addr, u16 len, u8 input_port, u32 ctrl, u16 hdr_len, u16 lro);

/** Posts a packet descriptor directly to TMU
 *
 * @param port		TMU port
 * @param queue		TMU queue
 * @param addr 		DDR class_tx_hdr + packet data address
 * @param len		DDR packet len
 */
static inline void send_to_tmu(u8 port, u8 queue, void *addr, u16 len)
{
	tmu_tx_hdr_t tmu __attribute__ ((aligned (8)));

	tmu.pkt_ptr = (u32)addr;
	tmu.len = len;
	tmu.queueno = queue;
	tmu.phyno = port;

	/* write buffer pointer to TMU INQ */
	efet_memcpy((void *)TMU_PHY_INQ_PKTPTR, &tmu, 0x8);

	PESTATUS_INCR_TX();
}

/** Retrieves hif queue
 *
 * @param queue		TMU queue
 * @param lro		lro flag
 */
static inline u8 get_hif_queue(u8 queue, u16 lro)
{
#if defined(COMCERTO_2000_CLASS)
	if (lro)
		return 2;
	else
#endif
		return queue & 0x1;
}

static inline u16 hif_hdr_add(void *dmem_addr, u8 client_id, u8 client_queue, u32 ctrl)
{
	u16 hdr_len = sizeof(struct hif_pkt_hdr);
	struct hif_pkt_hdr *hif_hdr = dmem_addr - hdr_len;
	
	hif_hdr->client_id = client_id;
	hif_hdr->qNo = client_queue;

	WRITE_UNALIGNED_INT(hif_hdr->client_ctrl_le, ctrl);

	return hdr_len;
}

/*GPI may corrupt the packet if txhdr+pktlen is < GPI_TX_MIN_LENGTH */
#define GPI_TX_MIN_LENGTH	73

static inline u16 tx_hdr_add(void *dmem_addr, void *ddr_addr, u16 len, u8 port, u8 queue, u8 action)
{
	class_tx_hdr_t *tx_hdr;
	u32 hdr_len;

#if defined(GPI_TX_MIN_LENGTH)  // workaround needed
	if (port < TMU3_PORT && sizeof(class_tx_hdr_t) + len < GPI_TX_MIN_LENGTH)
		tx_hdr = dmem_addr - (GPI_TX_MIN_LENGTH - len);
	else
#endif
		tx_hdr = dmem_addr - sizeof(class_tx_hdr_t);
	tx_hdr = (class_tx_hdr_t *)ALIGN64(tx_hdr);
	hdr_len = (void *)dmem_addr - (void *)tx_hdr;

	tx_hdr->start_data_off = dmem_addr - (void *)tx_hdr;
	tx_hdr->start_buf_off = ((u32)ddr_addr - hdr_len) & 0xff;
	tx_hdr->pkt_length = len;
	tx_hdr->act_phyno = (action & 0xf0) | (port & 0xf);
	tx_hdr->queueno = queue;

	return hdr_len;
}

#define GPI_TX_MIN_PAYLOAD 60
static inline u16 tx_gpi_wa(u8 port, void *dmem_addr, u16 *len)
{
	u16 pad_len = 0;

	if (port == TMU3_PORT && *len < GPI_TX_MIN_PAYLOAD)
	{
		pad_len = GPI_TX_MIN_PAYLOAD - *len;
		memset(dmem_addr + *len, 0, pad_len);
		*len = GPI_TX_MIN_PAYLOAD;
	}

	return pad_len;
}



/* GPI may not work properly for the packets size < GPI_TX_MIN_PAYLOAD
 * Following WA pads the tx packet length */
#define TX_GPI_WA(mtd) do {							\
		mtd->rx_dmem_end += tx_gpi_wa(mtd->output_port, mtd->data + mtd->offset, &mtd->length);	\
} while (0)										\



#endif /* _PE_H_ */
