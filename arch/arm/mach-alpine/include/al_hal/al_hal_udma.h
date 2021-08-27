 
#ifndef __AL_HAL_UDMA_H__
#define __AL_HAL_UDMA_H__

#include "al_hal_common.h"
#include "al_hal_udma_regs.h"

#ifdef __cplusplus
extern "C" {
#endif
 
#define DMA_MAX_Q 	4
#define AL_UDMA_MIN_Q_SIZE 	4
#define AL_UDMA_MAX_Q_SIZE 	(1 << 16)  

#define AL_UDMA_DEFAULT_MAX_ACTN_DESCS	16

#define DMA_RING_ID_MASK	0x3
 
union al_udma_desc {
	 
	struct {
		uint32_t len_ctrl;
		uint32_t meta_ctrl;
		uint64_t buf_ptr;
	} tx;
	 
	struct {
		uint32_t len_ctrl;
		uint32_t meta_ctrl;
		uint32_t meta1;
		uint32_t meta2;
	} tx_meta;
	 
	struct {
		uint32_t len_ctrl;
		uint32_t buf2_ptr_lo;
		uint64_t buf1_ptr;
	} rx;
} __packed_a16;

#define AL_M2S_DESC_CONCAT		AL_BIT(31)	 
#define AL_M2S_DESC_DMB			AL_BIT(30)
						 
#define AL_M2S_DESC_NO_SNOOP_H		AL_BIT(29)
#define AL_M2S_DESC_INT_EN		AL_BIT(28)	 
#define AL_M2S_DESC_LAST		AL_BIT(27)
#define AL_M2S_DESC_FIRST		AL_BIT(26)
#define AL_M2S_DESC_RING_ID_SHIFT	24
#define AL_M2S_DESC_RING_ID_MASK 	(0x3 << AL_M2S_DESC_RING_ID_SHIFT)
#define AL_M2S_DESC_META_DATA		AL_BIT(23)
#define AL_M2S_DESC_DUMMY		AL_BIT(22)  
#define AL_M2S_DESC_LEN_ADJ_SHIFT	20
#define AL_M2S_DESC_LEN_ADJ_MASK	(0x7 << AL_M2S_DESC_LEN_ADJ_SHIFT)
#define AL_M2S_DESC_LEN_SHIFT		0
#define AL_M2S_DESC_LEN_MASK		(0xffff << AL_M2S_DESC_LEN_SHIFT)

#define AL_UDMA_DESC_VMID_SHIFT		48

union al_udma_cdesc {
	 
	struct {
		uint32_t ctrl_meta;
	} al_desc_comp_tx;
	 
	struct {
		 
		uint32_t ctrl_meta;
	} al_desc_comp_rx;
} __packed_a4;

#define AL_UDMA_CDESC_ERROR		AL_BIT(31)
#define AL_UDMA_CDESC_LAST		AL_BIT(27)
#define AL_UDMA_CDESC_FIRST		AL_BIT(26)

struct al_buf {
	al_phys_addr_t addr;  
	uint32_t len;  
};

struct al_block {
	struct al_buf *bufs;  
	uint32_t num;  

	uint16_t vmid;
};

enum al_udma_type {
	UDMA_TX,
	UDMA_RX
};

enum al_udma_state {
	UDMA_DISABLE = 0,
	UDMA_IDLE,
	UDMA_NORMAL,
	UDMA_ABORT,
	UDMA_RESET
};

extern const char *const al_udma_states_name[];

struct al_udma_q_params {
	uint32_t size;		 
	union al_udma_desc *desc_base;  
	al_phys_addr_t desc_phy_base;	 
	uint8_t *cdesc_base;	 
				 
	al_phys_addr_t cdesc_phy_base;	 
	uint32_t cdesc_size;	 

	uint16_t dev_id;  
	uint8_t rev_id;  
};

struct al_udma_params {
	union udma_regs __iomem *udma_reg;  
	enum al_udma_type type;	 
	uint8_t num_of_queues;  
	char *name;  
};

struct al_udma;

enum al_udma_queue_status {
	AL_QUEUE_NOT_INITIALIZED = 0,
	AL_QUEUE_DISABLED,
	AL_QUEUE_ENABLED,
	AL_QUEUE_ABORTED
};

struct __cache_aligned al_udma_q {
	uint16_t size_mask;		 
	union udma_q_regs __iomem *q_regs;  
	union al_udma_desc *desc_base_ptr;  
	uint16_t next_desc_idx;  

	uint32_t desc_ring_id;	 

	uint8_t *cdesc_base_ptr; 
				 
	uint32_t cdesc_size;	 
	uint16_t next_cdesc_idx;  
	uint8_t *end_cdesc_ptr;	 
	uint16_t comp_head_idx;  
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	volatile union al_udma_cdesc *comp_head_ptr;  
#else
	union al_udma_cdesc *comp_head_ptr;  
#endif

	uint32_t pkt_crnt_descs;  
	uint32_t comp_ring_id;	 

	al_phys_addr_t desc_phy_base;  
	al_phys_addr_t cdesc_phy_base;  

	uint32_t flags;  
	uint32_t size;		 
	enum al_udma_queue_status status;
	struct al_udma *udma;	 
	uint32_t qid;		 

	uint16_t dev_id;  
	uint8_t rev_id;  
};

struct al_udma {
	char *name;
	enum al_udma_type type;	 
	enum al_udma_state state;
	uint8_t num_of_queues;  
	union udma_regs __iomem *udma_regs;  
	struct al_udma_q udma_q[DMA_MAX_Q];	 
};

int al_udma_init(struct al_udma *udma, struct al_udma_params *udma_params);

int al_udma_q_init(struct al_udma *udma, uint32_t qid,
		   struct al_udma_q_params *q_params);

int al_udma_q_reset(struct al_udma_q *udma_q);

int al_udma_q_handle_get(struct al_udma *udma, uint32_t qid,
		      struct al_udma_q **q_handle);

int al_udma_state_set(struct al_udma *udma, enum al_udma_state state);

enum al_udma_state al_udma_state_get(struct al_udma *udma);

static INLINE uint32_t al_udma_available_get(struct al_udma_q *udma_q)
{
	uint16_t tmp = udma_q->next_cdesc_idx - (udma_q->next_desc_idx + 1);
	tmp &= udma_q->size_mask;

	return (uint32_t) tmp;
}

static INLINE al_bool al_udma_is_empty(struct al_udma_q *udma_q)
{
	if (((udma_q->next_cdesc_idx - udma_q->next_desc_idx) &
	     udma_q->size_mask) == 0)
		return AL_TRUE;

	return AL_FALSE;
}

static INLINE union al_udma_desc *al_udma_desc_get(struct al_udma_q *udma_q)
{
	union al_udma_desc *desc;

	al_assert(udma_q);

	desc = udma_q->desc_base_ptr + udma_q->next_desc_idx;

	udma_q->next_desc_idx++;
	 
	udma_q->next_desc_idx &= udma_q->size_mask;

	return desc;
}

static INLINE uint32_t al_udma_ring_id_get(struct al_udma_q *udma_q)
{
	uint32_t ring_id;

	al_assert(udma_q);

	ring_id = udma_q->desc_ring_id;

	if (unlikely(udma_q->next_desc_idx) == 0)
		udma_q->desc_ring_id = (udma_q->desc_ring_id + 1) &
			DMA_RING_ID_MASK;
	return ring_id;
}

static INLINE int al_udma_desc_action_add(struct al_udma_q *udma_q,
					  uint32_t num)
{
	al_assert(udma_q);
	al_assert((num > 0) && (num <= udma_q->size));

	al_local_data_memory_barrier();

	al_reg_write32_relaxed(&udma_q->q_regs->rings.drtp_inc, num);

	return 0;
}

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
#define cdesc_is_first(flags) ((flags) & AL_UDMA_CDESC_FIRST)
#define cdesc_is_last(flags) ((flags) & AL_UDMA_CDESC_LAST)
#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
static INLINE union al_udma_cdesc *al_cdesc_next(
	struct al_udma_q		*udma_q,
	volatile union al_udma_cdesc	*cdesc,
	uint32_t			offset)
#else
static INLINE union al_udma_cdesc *al_cdesc_next(struct al_udma_q *udma_q,
						 union al_udma_cdesc *cdesc,
						 uint32_t offset)
#endif
{
	uint8_t *tmp = (uint8_t *) cdesc + offset * udma_q->cdesc_size;
	al_assert(udma_q);
	al_assert(cdesc);

	if (unlikely(((uint8_t *) tmp > udma_q->end_cdesc_ptr)))
		return (union al_udma_cdesc *)
			(udma_q->cdesc_base_ptr +
			(tmp - udma_q->end_cdesc_ptr - udma_q->cdesc_size));

	return (union al_udma_cdesc *) tmp;
}

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
static INLINE al_bool al_udma_new_cdesc(struct al_udma_q *udma_q,
								uint32_t flags)
{
	if (((flags & AL_M2S_DESC_RING_ID_MASK) >> AL_M2S_DESC_RING_ID_SHIFT)
	    == udma_q->comp_ring_id)
		return AL_TRUE;
	return AL_FALSE;
}

static INLINE union al_udma_cdesc *al_cdesc_next_update(
	struct al_udma_q		*udma_q,
	volatile union al_udma_cdesc	*cdesc)
{
	 
	if (unlikely(((uint8_t *) cdesc == udma_q->end_cdesc_ptr))) {
		udma_q->comp_ring_id =
		    (udma_q->comp_ring_id + 1) & DMA_RING_ID_MASK;
		return (union al_udma_cdesc *) udma_q->cdesc_base_ptr;
	}
	return (union al_udma_cdesc *) ((uint8_t *) cdesc + udma_q->cdesc_size);
}
#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
uint32_t al_udma_cdesc_packet_get(
	struct al_udma_q		*udma_q,
	volatile union al_udma_cdesc	**desc);

#define al_udma_cdesc_idx_to_ptr(udma_q, idx)				\
	((volatile union al_udma_cdesc *) ((udma_q)->cdesc_base_ptr +		\
				(idx) * (udma_q)->cdesc_size))
#else
uint32_t al_udma_cdesc_packet_get(struct al_udma_q *udma_q,
				     union al_udma_cdesc **desc);

#define al_udma_cdesc_idx_to_ptr(udma_q, idx)				\
	((union al_udma_cdesc *) ((udma_q)->cdesc_base_ptr +		\
				(idx) * (udma_q)->cdesc_size))

#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
static INLINE uint32_t al_udma_cdesc_get_all(
	struct al_udma_q		*udma_q,
	volatile union al_udma_cdesc	**cdesc)
#else
static INLINE uint32_t al_udma_cdesc_get_all(struct al_udma_q *udma_q,
					     union al_udma_cdesc **cdesc)
#endif
{
	uint16_t count = 0;

	al_assert(udma_q);
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
#else
	al_assert(cdesc);
#endif

	udma_q->comp_head_idx = (uint16_t)
				(al_reg_read32(&udma_q->q_regs->rings.crhp) &
						0xFFFF);

	count = (udma_q->comp_head_idx - udma_q->next_cdesc_idx) &
		udma_q->size_mask;

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	if (cdesc)
#endif
		*cdesc = al_udma_cdesc_idx_to_ptr(udma_q, udma_q->next_cdesc_idx);

	return (uint32_t)count;
}

static INLINE int al_udma_cdesc_ack(struct al_udma_q *udma_q, uint32_t num)
{
	al_assert(udma_q);

	udma_q->next_cdesc_idx += num;
	udma_q->next_cdesc_idx &= udma_q->size_mask;

	return 0;
}

#ifdef __cplusplus
}
#endif
 
#endif  
 