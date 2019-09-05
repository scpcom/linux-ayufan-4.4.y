/*
 * comcerto_xor.h
 *
 *  Created on: May 11, 2012
 *      Author: bwang
 */

#ifndef COMCERTO_XOR_H_
#define COMCERTO_XOR_H_

#include<linux/types.h>
#include<linux/io.h>
#include<linux/dmaengine.h>
#include<linux/interrupt.h>

#define POOL_NUMBER 4
#define COMCERTO_XOR_INBOUND_DESC_SIZE         64
#define COMCERTO_XOR_OUTBOUND_DESC_SIZE        32
#define COMCERTO_XOR_MAX_SRC                   6
#define COMCERTO_XOR_MAX_DEST                  2
#ifdef CONFIG_COMCERTO_XOR_EXTENDED_TEST
#define COMCERTO_XOR_THRESHOLD                 100
#endif
#ifndef CONFIG_COMCERTO_XOR_EXTENDED_TEST
#define COMCERTO_XOR_THRESHOLD                 1
#endif

#define M2IO_CONTROL(chan)               (chan->mmr_base)
#define M2IO_HEAD(chan)                  (chan->mmr_base + 0x4)
#define M2IO_BURST(chan)                 (chan->mmr_base + 0x8)
#define M2IO_FLEN(chan)                  (chan->mmr_base + 0xC)
#define M2IO_IRQ_ENABLE(chan)            (chan->mmr_base + 0x10)
#define M2IO_IRQ_STATUS(chan)            (chan->mmr_base + 0x14)
#define M2IO_RESET(chan)                 (chan->mmr_base + 0x20)

#define IO2M_CONTROL(chan)               (chan->mmr_base + 0x80)
#define IO2M_HEAD(chan)                  (chan->mmr_base + 0x84)
#define IO2M_BURST(chan)                 (chan->mmr_base + 0x88)
#define IO2M_FLEN(chan)                  (chan->mmr_base + 0x8C)
#define IO2M_IRQ_ENABLE(chan)            (chan->mmr_base + 0x90)
#define IO2M_IRQ_STATUS(chan)            (chan->mmr_base + 0x94)
#define IO2M_RESET(chan)                 (chan->mmr_base + 0xA0)

//	Mem to IO Control
#define M2IO_START	(1 << 0)
#define M2IO_FLENEN	(1 << 1)
#define M2IO_FCOM	(1 << 2)
#define M2IO_DONOSTOP	(1 << 3)
#define M2IO_DONOSTA	(1 << 4)

//	IO to Mem: DMA Control
#define IO2M_IRQFRDYN	(1 << 0)
#define IO2M_IRQFLST	(1 << 1)
#define IO2M_IRQFDON	(1 << 2)
#define IO2M_IRQFLSH	(1 << 3)
#define IO2M_IRQFLEN	(1 << 4)
#define IO2M_IRQFTHLD	(1 << 5)

// IRQ Status Register
#define IRQ_IRQFRDYN	(1 << 0)
#define IRQ_IRQFLST	(1 << 1)
#define IRQ_IRQFDON	(1 << 2)
#define IRQ_IRQFLSH	(1 << 3)
#define IRQ_IRQFLEN	(1 << 4)
#define IRQ_IRQFTHLD	(1 << 5)
#define IRQ_IRQFCTRL	(1 << 6)

// Inbound Frame and Buffer Descriptor Programming

// BControl
#define BLAST	(1 << 16)
#define BFIX	(1 << 17)

// Block Size
#define XOR_BLOCK_SIZE_256	0
#define XOR_BLOCK_SIZE_512	1
#define XOR_BLOCK_SIZE_1024	2
#define XOR_BLOCK_SIZE_2048	3
#define XOR_BLOCK_SIZE_4096	4

struct comcerto_xor_device {
	dma_addr_t               dma_desc_pool[POOL_NUMBER];
	void                     *dma_desc_pool_virt[POOL_NUMBER];
	struct dma_device        device;
};

struct comcerto_xor_desc_slot {
	struct list_head               slot_node;
	struct list_head               chain_node;
	struct list_head               completed_node;
	enum dma_transaction_type      type;
	void                           *hw_desc_inbound;
	void                           *hw_desc_outbound;
	dma_addr_t                     hw_desc_outbound_dma;
	int                            busy;
	u16                            src_cnt;
	size_t                         len;
	struct dma_async_tx_descriptor async_tx;
	u32                            *xor_check_result;
};

struct comcerto_xor_chan {
	int                         pending;
	int                         slot_allocated;
	dma_cookie_t                completed_cookie;
	spinlock_t                  lock;
	void __iomem                *mmr_base;
	struct list_head            all_slots;
	struct list_head            chain;
	struct list_head            completed_slots;
	struct comcerto_xor_device        *device;
	struct dma_chan		        chan;
	struct comcerto_xor_desc_slot     *last_used;
	struct comcerto_xor_desc_slot     *to_be_started;
	struct tasklet_struct       irq_tasklet;
};

struct comcerto_xor_inbound_desc {
	u32  next_desc;
	u32  fcontrol;
	u32  fstatus0;
	u32  fstatus1;
	u32  buff_info[12]; // 6 Buffer Descriptors
};

struct comcerto_xor_outbound_desc {
	u32  next_desc;
	u32  fcontrol;
	u32  fstatus0;
	u32  fstatus1;
	u32  buff_info[4]; // 2 Buffer Descriptors
};


#endif /* COMCERTO_XOR_H_ */
