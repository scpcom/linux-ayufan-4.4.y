#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef MV_XOR_H
#define MV_XOR_H

#include <linux/types.h>
#include <linux/io.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>

#define USE_TIMER
#define MV_XOR_SLOT_SIZE		64
#define MV_XOR_THRESHOLD		1

#define XOR_OPERATION_MODE_XOR		0
#define XOR_OPERATION_MODE_MEMCPY	2
#define XOR_OPERATION_MODE_MEMSET	4
#ifdef MY_DEF_HERE
#define XOR_DESCRIPTOR_SWAP            BIT(14)
#endif

#define XOR_CURR_DESC(chan)	(chan->mmr_base + 0x210 + (chan->idx * 4))
#define XOR_NEXT_DESC(chan)	(chan->mmr_base + 0x200 + (chan->idx * 4))
#define XOR_BYTE_COUNT(chan)	(chan->mmr_base + 0x220 + (chan->idx * 4))
#define XOR_DEST_POINTER(chan)	(chan->mmr_base + 0x2B0 + (chan->idx * 4))
#define XOR_BLOCK_SIZE(chan)	(chan->mmr_base + 0x2C0 + (chan->idx * 4))
#define XOR_INIT_VALUE_LOW(chan)	(chan->mmr_base + 0x2E0)
#define XOR_INIT_VALUE_HIGH(chan)	(chan->mmr_base + 0x2E4)

#define XOR_CONFIG(chan)	(chan->mmr_base + 0x10 + (chan->idx * 4))
#define XOR_ACTIVATION(chan)	(chan->mmr_base + 0x20 + (chan->idx * 4))
#define XOR_INTR_CAUSE(chan)	(chan->mmr_base + 0x30)
#define XOR_INTR_MASK(chan)	(chan->mmr_base + 0x40)
#define XOR_ERROR_CAUSE(chan)	(chan->mmr_base + 0x50)
#define XOR_ERROR_ADDR(chan)	(chan->mmr_base + 0x60)
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#if defined(MY_DEF_HERE)
#define XOR_OUTSTANDING_RDEADS(chan)	(chan->mmr_base + 0x80 + (chan->idx * 4))
#endif
#define XOR_INTR_MASK_VALUE	0x3F7
#else
#define XOR_INTR_MASK_VALUE	0x3F5
#endif

#define WINDOW_BASE(w)		(0x250 + ((w) << 2))
#define WINDOW_SIZE(w)		(0x270 + ((w) << 2))
#define WINDOW_REMAP_HIGH(w)	(0x290 + ((w) << 2))
#define WINDOW_BAR_ENABLE(chan)	(0x240 + ((chan) << 2))

struct mv_xor_shared_private {
	void __iomem	*xor_base;
	void __iomem	*xor_high_base;
};

struct mv_xor_device {
	struct platform_device		*pdev;
	int				id;
	dma_addr_t			dma_desc_pool;
	void				*dma_desc_pool_virt;
	struct dma_device		common;
	struct mv_xor_shared_private	*shared;
};

struct mv_xor_chan {
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#else
	int			pending;
#endif
	dma_cookie_t		completed_cookie;
	spinlock_t		lock;  
	void __iomem		*mmr_base;
	unsigned int		idx;
	enum dma_transaction_type	current_type;
	struct list_head	chain;
	struct list_head	completed_slots;
	struct mv_xor_device	*device;
	struct dma_chan		common;
	struct mv_xor_desc_slot	*last_used;
	struct list_head	all_slots;
	int			slots_allocated;
	struct tasklet_struct	irq_tasklet;
#ifdef USE_TIMER
	unsigned long		cleanup_time;
	u32			current_on_last_cleanup;
	dma_cookie_t		is_complete_cookie;
#endif
};

struct mv_xor_desc_slot {
	struct list_head	slot_node;
	struct list_head	chain_node;
	struct list_head	completed_node;
	enum dma_transaction_type	type;
	void			*hw_desc;
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#else
	struct mv_xor_desc_slot	*group_head;
	u16			slot_cnt;
#endif
	u16			slots_per_op;
	u16			idx;
	u16			unmap_src_cnt;
	u32			value;
	size_t			unmap_len;
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#else
	struct list_head	tx_list;
#endif
	struct dma_async_tx_descriptor	async_tx;
	union {
		u32		*xor_check_result;
		u32		*crc32_result;
	};
#ifdef USE_TIMER
	unsigned long		arrival_time;
	struct timer_list	timeout;
#endif
};

#ifdef MY_DEF_HERE
 
#if defined(__LITTLE_ENDIAN)
struct mv_xor_desc {
	u32 status;		 
	u32 crc32_result;	 
	u32 desc_command;	 
	u32 phy_next_desc;	 
	u32 byte_count;		 
	u32 phy_dest_addr;	 
	u32 phy_src_addr[8];	 
	u32 reserved0;
	u32 reserved1;
};
#define mv_phy_src_idx(src_idx) (src_idx)
#else  
struct mv_xor_desc {
	u32 crc32_result;	 
	u32 status;		 
	u32 phy_next_desc;	 
	u32 desc_command;	 
	u32 phy_dest_addr;	 
	u32 byte_count;		 
	u32 phy_src_addr[8];	 
	u32 reserved1;
	u32 reserved0;
};
#define mv_phy_src_idx(src_idx) (src_idx ^ 1)
#endif 
#else !MY_DEF_HERE
 
struct mv_xor_desc {
	u32 status;		 
	u32 crc32_result;	 
	u32 desc_command;	 
	u32 phy_next_desc;	 
	u32 byte_count;		 
	u32 phy_dest_addr;	 
	u32 phy_src_addr[8];	 
	u32 reserved0;
	u32 reserved1;
};
#endif  

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
 
struct mv_xor_save_regs {
	int xor_config;
	int interrupt_mask;
};
#endif

#define to_mv_sw_desc(addr_hw_desc)		\
	container_of(addr_hw_desc, struct mv_xor_desc_slot, hw_desc)

#define mv_hw_desc_slot_idx(hw_desc, idx)	\
	((void *)(((unsigned long)hw_desc) + ((idx) << 5)))

#define MV_XOR_MIN_BYTE_COUNT	(128)
#define XOR_MAX_BYTE_COUNT	((16 * 1024 * 1024) - 1)
#define MV_XOR_MAX_BYTE_COUNT	XOR_MAX_BYTE_COUNT

#endif
