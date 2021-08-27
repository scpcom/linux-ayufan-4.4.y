/*
 * GPL LICENSE SUMMARY 
 *
 *  Copyright(c) 2012 Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *  The full GNU General Public License is included in this distribution
 *  in the file called LICENSE.GPL.
 *
 *  Contact Information:
 *    Intel Corporation
 *    2200 Mission College Blvd.
 *    Santa Clara, CA  97052
 *
 */

/* UDMA  Driver main stack */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>

#include <linux/spinlock.h>

#include <linux/interrupt.h>

#include <linux/udma_api.h>
#include <linux/delay.h>

#include "udma_hw.h"
#include "udma_main.h"

#ifdef DEBUG
#include <linux/proc_fs.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <net/sock.h>
#include <net/checksum.h>
#include <linux/if_ether.h>	/* For the statistics structure. */
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/percpu.h>
#include <net/net_namespace.h>
#include <linux/u64_stats_sync.h>

#endif

#define DISABLE_RX_INTERRUPT_IN_HARDIRQ 1
#define DISABLE_TX_INTERRUPT_IN_HARDIRQ 1

static udma_result_t udma_start_rx_transfer(struct udma_device *umdev);
static udma_result_t udma_start_tx_transfer(struct udma_device *umdev);
static int udma_ring_clean_irq(struct udma_device *umdev, struct udma_ring *ring, const int budget, bool dir);
static inline u16 __get_using_desc(struct udma_ring *ring);
#ifdef DEBUG
#define UDMA_WARN_ON_RETURN(condition,ret) ({			\
		int __ret_warn_on = !!(condition);				\
		if (__ret_warn_on) return ret;					\
	})
#else 
#define UDMA_WARN_ON_RETURN(condition,ret) do{}while(0)
#endif

static inline u16 __get_using_desc(struct udma_ring *ring)
{
	u16 i = 0;
	i = ring->to_be_use;
	/* Get to_be_used updated */
	while((i!=ring->tail) && DESC_IS_DONE(INDEX_TO_DESC(i,ring)))
		i = DESC_INDEX_INC(i);
	return i;
}

#ifdef DEBUG
static inline void udma_dump_a_ring(struct udma_device *umdev, struct udma_ring *ring, bool direction);	
/*
 * Following code are for debug purpose
*/
u32 g_order[2];
#define ORDER_INC(o) ((o+1)%0xfffffff0)
void udma_dump_frame(void * buffer, size_t size, int direction)
{
	unsigned char *p = NULL;
	int i = 0;
	unsigned int size_1 = size/8;
	unsigned int size_2 = size%8;
	p = (unsigned char *)buffer;
	udma_dbg("\n==========================%s=============================\n",(direction == UDMA_TX)?"Tx":"Rx");
	udma_dbg(" Packet size %d bytes, size1 %d size2 %d\n",size,size_1,size_2);	
	if (size_1) {
		for (i = 0;i< size_1;i++) {
			if (!(unsigned char *)(p+i*8))
				return;
			udma_dbg("[%d]	 %2x  %2x  %2x  %2x  %2x  %2x  %2x  %2x ",i*8+1,\
				*(unsigned char *)(p+i*8),*(unsigned char *)(p+i*8+1),\
				*(unsigned char *)(p+i*8+2),*(unsigned char *)(p+i*8+3), \
				*(unsigned char *)(p+i*8+4),*(unsigned char *)(p+i*8+5), \
				*(unsigned char *)(p+i*8+6),*(unsigned char *)(p+i*8+7));
		}
		p+=(size_1*8);
	}
	if (size_2) {
		for (i = 0;i< size_2;i++) {
			udma_dbg("[%d]	 %2x",8*size_1+i+1,*(unsigned char *)(p+i));
		}
	}	
	udma_dbg("\n");
	udma_dbg("End Packet size %d bytes\n",size);	
	
	return ;
}

static void udma_dump_current_desc(struct udma_device *umdev, bool direction) 
{
	struct udma_ring *ring = NULL;
	int i= 0;
	if (UDMA_TX == direction)
		ring = &umdev->tx_ring;
	else
		ring = &umdev->rx_ring;
	udma_dump_a_ring(umdev, ring, direction);
	return;
}

#endif
/**
 * udma_dump - Print registers, tx-ring and rx-ring
 **/
#ifdef DEBUG 
#define udma_dump_a_ring_progress(ring)	do \
	{\
	udma_info("%8s clean[%2d] use[%2d] tail[%2d] new tail [%2d]\n",__FUNCTION__,ring->to_be_clean, ring->to_be_use, ring->tail,ring->new_tail); \
	} while (0)

static inline void udma_dump_a_ring(struct udma_device *umdev, struct udma_ring *ring, bool direction)
{
	int i;
	udma_info("\n ============Ring=======Stat================================= \n");
	udma_info("\n 		RingNo 	     		 dma 	0x%x  \n",  (u32)ring->ring_dma);
	udma_info("\n 		size 	    0x%x	 \n", ring->ring_dma_size);

	udma_info("\n 		cnt 	    %d 		 desc 0x%p     \n", ring->ring_entries, ring->desc);

	udma_info("\n 		clean[%d]	use[%d]	 tail[%d] newtail[%d]\n", ring->to_be_clean, ring->to_be_use, ring->tail,ring->new_tail);

	i = 0;
	while ( i< ring->ring_entries) {
		udma_info("\n  No. %d  \n", i);
		udma_desc_dump(INDEX_TO_DESC(i,ring));
		i++;
	}
		
	udma_info("\n =========================================================== \n");

}

static void udma_dump_all_rings(u8 port)
{
	struct udma_device *umdev = umdev = udma_devs[port];
	struct udma_ring *tx= &umdev->tx_ring;
	struct udma_ring *rx= &umdev->rx_ring;
	udma_info("\n================================================================\n");
	udma_info("\n");	
	udma_info("\n 		UDMA data structure dump 	\n");
	udma_info("\n");	
	udma_info("\n 		desc size 0x%x \n", sizeof(struct udma_desc));
	
	udma_dump_a_ring(umdev, tx, UDMA_TX);
	udma_dump_a_ring(umdev, rx, UDMA_RX);

	udma_info("\n================================================================\n");

	udma_info("\n");	
	return ;
}
#else

#define udma_dump_all_rings(port) \
do { \
} while(0)

#endif

static bool udma_rx_is_stopped(struct udma_device *umdev)
{
	struct udma_hw *hw = umdev->hw;
	struct udma_desc *udma_desc = NULL;
	struct udma_ring *rx = &umdev->rx_ring;
	u32 desc;
	int index;
	bool ret = false;

	if (hw->ops->rx_is_active(hw))
		return false;
	desc = hw->ops->get_next_rx_desc(hw);
	if (unlikely(desc == 0)) {
		return true;
	}
	desc = hw->ops->get_curr_rx_desc(hw);

	index = DESC_DMA_TO_DESC_INDEX(desc,rx);
	udma_desc = INDEX_TO_DESC(rx->tail,rx);
	if ((index == rx->tail) && DESC_IS_DONE(udma_desc)) {
			ret = true;
	}
	return ret;
}

static bool udma_tx_is_stopped(struct udma_device *umdev)
{
	struct udma_hw *hw = umdev->hw;
	struct udma_desc *udma_desc = NULL;
	struct udma_ring *tx = &umdev->tx_ring;
	u32 desc;
	int index;
	bool ret = false;

	if (hw->ops->tx_is_active(hw))
		return false;

	desc = hw->ops->get_next_tx_desc(hw);
	if (unlikely(desc == 0)) {
		return true;
	}
	desc = hw->ops->get_curr_tx_desc(hw);
	index = DESC_DMA_TO_DESC_INDEX(desc,tx);
	udma_desc = INDEX_TO_DESC(tx->tail,tx);
	if ((index == tx->tail) && DESC_IS_DONE(udma_desc)) {
		ret = true;
	}
	return ret;
}

/* Pop a descriptor in the ring and reclaim resource  */
static void udma_ring_pop(struct udma_device *umdev, struct udma_ring *ring, const unsigned int i, bool direction)
{
	struct udma_hw *hw = umdev->hw;
	struct udma_desc *desc = NULL;
	udma_buffer_desc_t *user_desc = NULL;

	udma_dbg("Try to Pop -%s-ring  desc [%d] \n",(direction == UDMA_TX)?"Tx":"Rx",i);

	desc = INDEX_TO_DESC(i,ring);
	user_desc = ring->buffer_desc[i];

	/* Make sure do not reclaim a packet repeatly */
	if (user_desc == NULL) {
		return;	
	} else if (user_desc->state != UDMA_BUFFER_INITIAL) {
		return;
	}
	
	if (DESC_IS_DONE(desc)) {
		if (direction == UDMA_TX) {
			user_desc->state = UDMA_BUFFER_TX_DONE;
		} else { 
			user_desc->state = UDMA_BUFFER_RX_DONE;
		}
		user_desc->data_size = user_desc->len - DESC_DATA_BUFFER_LEN(desc);
	} else {
		user_desc->state = UDMA_BUFFER_RETURNED;
		user_desc->data_size = 0;
	}
	
	dma_unmap_single(&hw->pdev->dev, (dma_addr_t)desc->priv,
			  user_desc->len,(direction== UDMA_TX)?DMA_TO_DEVICE:DMA_FROM_DEVICE);

	/* Clean the descriptor buffer and desc. */
	hw->ops->clear_desc(desc);

	/* Reclaim the resource */
	if (direction == UDMA_TX)  
		umdev->tx_complete(umdev->hw->port,user_desc);
	else if (direction == UDMA_RX) 
		umdev->rx_complete(umdev->hw->port,user_desc);
	ring->buffer_desc[i] = NULL;
	return ;
}

/**
 * udma_intr - Interrupt Handler
 * @irq: interrupt number
 * @data: pointer to a private device structure
 **/
static irqreturn_t udma_intr(int irq, void *data)
{
	struct udma_device *umdev = (struct udma_device *)data;
	struct udma_hw *hw = umdev->hw;
	struct udma_ring *rx =&umdev->rx_ring;
	struct udma_ring *tx = &umdev->tx_ring;
	u32 status = hw->ops->get_irq_status(hw);

	if (!(UDMA_HW_VALID_INTR_STATE(hw->port) & status)) {
		return IRQ_NONE;
	}

	/* Tx interrupt */
	if (status & UDMA_HW_VALID_TX_INTR_STATE(hw->port)) {			
		//udma_dbg("Tx udma_intr status 0x%x \n",status);	
		/*Read the interrupt again before clearing it */
		status = hw->ops->get_irq_status(hw);
		hw->ops->clear_tx_irq(hw);
		spin_lock(&tx->lock);
		tx->to_be_use	= __get_using_desc(tx);
		spin_unlock(&tx->lock);

#ifdef DISABLE_TX_INTERRUPT_IN_HARDIRQ
		hw->ops->disable_tx_irq(hw);
#endif		
	} 
		
	if (status & UDMA_HW_VALID_RX_INTR_STATE(hw->port)) { 
		udma_dbg("Rx udma_intr status 0x%x \n",status);	
		/*Read the interrupt again before clearing it */
		hw->ops->clear_rx_irq(hw);
		
		spin_lock(&rx->lock);
		rx->to_be_use =	__get_using_desc(rx);
		spin_unlock(&rx->lock);
#ifdef DISABLE_RX_INTERRUPT_IN_HARDIRQ
		hw->ops->disable_rx_irq(hw);
#endif
	}
	/* Schedule the recycle tasklet */ 
	tasklet_hi_schedule(&umdev->clean_tasklet);

	return IRQ_HANDLED;
}

/* 
 * udma_ring_clean_unused: Clean the unused descriptors in a ring 
*/
static int udma_ring_clean_unused(struct udma_device *umdev, struct udma_ring *ring, const int budget, bool dir)
{

	struct udma_desc *desc = NULL;
	int i = 0;
	int cleaned = 0;

	i = 0;
	desc = INDEX_TO_DESC(i,ring);
	while ((i< ring->ring_entries) && cleaned <= budget) {
		udma_ring_pop(umdev,ring,i, dir);
		cleaned ++;
		i = DESC_INDEX_INC(i);
	}
	return cleaned;
}

/* 
 * udma_ring_clean_irq: Clean the finished descriptors in a ring 
*/
static int udma_ring_clean_irq(struct udma_device *umdev, struct udma_ring *ring, const int budget,bool direction)
{
	int i,cleaned = 0;
	struct udma_desc *desc = NULL;
	int using = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&ring->lock, flags);	
	ring->to_be_use = __get_using_desc(ring);
	using = ring->to_be_use;
	spin_unlock_irqrestore(&ring->lock, flags);	
		
	i = ring->to_be_clean;
	desc = INDEX_TO_DESC(i,ring);

	while ((i != using) && (cleaned <= budget)) {
		if (DESC_IS_IN_PROGRESS(desc) || \
			(DESC_IS_INITIAL(desc) && !DESC_IS_NULL(desc))) {
			break;
		}
		if (DESC_IS_DONE(desc))
			udma_ring_pop(umdev,ring,i,direction);
		cleaned ++;
		i = DESC_INDEX_INC(i);
		desc = INDEX_TO_DESC(i,ring);
	}
	ring->to_be_clean = i;
	return cleaned;

}

static int udma_clean_a_ring(struct udma_device *umdev, udma_clean_op_t ops, const int budget, bool dir)
{
	struct udma_ring *ring = NULL;
	u32 cleaned = 0;
	if (UDMA_TX == dir)
		ring = &umdev->tx_ring;
	else 
		ring = &umdev->rx_ring;

	cleaned += ops(umdev,ring,budget,dir);
	return cleaned;
}
static int udma_clean_finished_works(struct udma_device *umdev,bool dir, const int budget)
{
		return udma_clean_a_ring(umdev,umdev->clean_irq,budget, dir);		
}
static int udma_clean_unfinished_works(struct udma_device *umdev,bool dir, const int budget)
{
		return udma_clean_a_ring(umdev,umdev->clean_unused,budget,dir);
}

static void udma_tasklet_clean(unsigned long param)
{
	struct udma_device * umdev = (struct udma_device *)param;
#if defined(DISABLE_RX_INTERRUPT_IN_HARDIRQ) || defined( DISABLE_RX_INTERRUPT_IN_HARDIRQ)
	struct udma_hw *hw = umdev->hw;
#endif
	unsigned long flags;
	PRINT_FUNC

	udma_clean_finished_works(umdev,UDMA_TX,UDMA_TASKLET_THRESHOLD);	
	
	spin_lock_irqsave(&umdev->tx_ring.lock, flags);
	if (udma_tx_is_stopped(umdev)) {
		udma_start_tx_transfer(umdev);		
	}
	spin_unlock_irqrestore(&umdev->tx_ring.lock, flags);

	udma_clean_finished_works(umdev,UDMA_RX,UDMA_TASKLET_THRESHOLD);
	spin_lock_irqsave(&umdev->rx_ring.lock, flags);
	if (udma_rx_is_stopped(umdev)) {	
		udma_start_rx_transfer(umdev);
	}
	spin_unlock_irqrestore(&umdev->rx_ring.lock, flags);
	
#ifdef DISABLE_TX_INTERRUPT_IN_HARDIRQ
	hw->ops->enable_tx_irq(hw);
#endif
#ifdef DISABLE_RX_INTERRUPT_IN_HARDIRQ
	hw->ops->enable_rx_irq(hw);
#endif
}

/****************************************************************************
*
*
****************************************************************************/

/**  udma_send_packet - A buffer is used to send
 * @port - udma port number, could be 0 or 1
 * @buffer_desc - parameter to describe a coming buffer
 * 
 * return UDMA_OK, UDMA driver took care of the buffer
 * return UDMA_TX_BUSY, UDMA driver is busy and refuse to take care of the buffer
 * return UDMA_ERR, if error
*/

static udma_result_t udma_start_rx_transfer(struct udma_device *umdev)
{
	struct udma_hw *hw = umdev->hw;
	struct udma_ring *rx = &umdev->rx_ring;
	struct udma_desc *udma_desc_new_tail;
	struct udma_desc *udma_desc_to_be_use;

	PRINT_FUNC

	if (rx->tail == rx->new_tail) {
		return UDMA_EMPTY;
	}
	
	rx->to_be_use = NEXT_RX(rx->tail);
	udma_desc_new_tail = &rx->desc[rx->new_tail];
	udma_desc_to_be_use = &rx->desc[rx->to_be_use];
	SET_END_DESC_FLAG(udma_desc_new_tail);

	rx->tail = rx->new_tail;
	hw->ops->start_rx_transfer(hw, DESC_INDEX_TO_DESC_DMA(rx->to_be_use,rx));

	return UDMA_OK;
	
}
static udma_result_t udma_start_tx_transfer(struct udma_device *umdev)
{
	struct udma_hw *hw = umdev->hw;
	struct udma_desc *udma_desc_new_tail;
	struct udma_desc *udma_desc_to_be_use;
	struct udma_ring *tx = &umdev->tx_ring;

	if (tx->tail == tx->new_tail) {
		return UDMA_EMPTY;
	}
	tx->to_be_use = NEXT_TX(tx->tail);
	udma_desc_new_tail = &tx->desc[tx->new_tail];
	udma_desc_to_be_use = &tx->desc[tx->to_be_use];
	SET_END_DESC_FLAG(udma_desc_new_tail);
	hw->ops->enable_desc_tx_irq(udma_desc_new_tail);
	tx->tail = tx->new_tail;
	hw->ops->start_tx_transfer(hw, DESC_INDEX_TO_DESC_DMA(tx->to_be_use,tx));

	return UDMA_OK;

}
udma_result_t udma_send_packet(unsigned char port, udma_buffer_desc_t *buffer_desc)
{
	struct udma_device *umdev = NULL;
	struct udma_hw *hw = NULL;
	struct udma_ring *tx = NULL;
	unsigned long flags;
	struct udma_desc *udma_desc;
	dma_addr_t dma;
	int ret = UDMA_OK;

	UDMA_WARN_ON_RETURN((port >= UDMA_PORT_NUM_TOTAL),UDMA_INVALID_PARAM);
	UDMA_WARN_ON_RETURN((!buffer_desc)||(!buffer_desc->buf),UDMA_INVALID_PARAM);
	UDMA_WARN_ON_RETURN((buffer_desc->state != UDMA_BUFFER_INITIAL)||(!buffer_desc->private),UDMA_INVALID_PARAM);
	UDMA_WARN_ON_RETURN((buffer_desc->len < UDMA_MIN_XMIT_SIZE) ||(buffer_desc->len > UDMA_MAX_XMIT_SIZE),UDMA_INVALID_PARAM);

	umdev = udma_devs[port];	
	UDMA_WARN_ON_RETURN((!umdev->tx_complete) || (!umdev->rx_complete),UDMA_UNINITIALIZED);
	hw = umdev->hw;
	
	tx = &umdev->tx_ring;
	
	if (is_ring_full(tx)) {
		tasklet_hi_schedule(&umdev->clean_tasklet);
		return UDMA_FULL;
	}
	dma = dma_map_single(&hw->pdev->dev, buffer_desc->buf, buffer_desc->len, DMA_TO_DEVICE);
	if (dma_mapping_error(&hw->pdev->dev, dma)) {
		dev_err(&umdev->hw->pdev->dev, "DMA map failed\n");
		return UDMA_ERR;
	}
	
	/* Fill in the buffer info to the descriptor */
	spin_lock_irqsave(&tx->lock, flags);
	tx->new_tail = NEXT_TX(tx->new_tail);
	tx->buffer_desc[tx->new_tail] = buffer_desc;	
	udma_desc = &tx->desc[tx->new_tail];
	udma_desc->priv = dma;
	udma_desc->union_field.size = buffer_desc->len & 0x00FFFFFF;
	hw->ops->update_tx_desc(hw, udma_desc, dma, buffer_desc->pos);

	if (udma_tx_is_stopped(umdev)) {
		udma_start_tx_transfer(umdev);		
	}
	spin_unlock_irqrestore(&tx->lock, flags);
	return ret;
}

/**  udma_give_free_buffer - Give a free buffer to UDMA driver, the buffer will be used for packet receive
 * @port - udma port number, could be 0 or 1
 * @buffer_desc - parameter to describe a coming buffer
 * 
 * Note the upper layer should calls this functioin periodly to give enough free buffers to UDMA driver 
 *
 * It's requried to allocate a single buffer for a packet. 
 * Thus the coming free buffer size should be large enough, i.e larger than 1536 bytes. Otherwise, the buffer would be refused.
 *
 * return 0, UDMA driver took care of the buffer
 * return negative for failure
*/
udma_result_t udma_give_free_buffer(unsigned char port, udma_buffer_desc_t *buffer_desc)
{
	struct udma_device *umdev = NULL;
	struct udma_hw *hw = NULL;
	struct udma_ring *rx = NULL;
	unsigned long flags;
	struct udma_desc *udma_desc;
	dma_addr_t dma;
	int ret = UDMA_OK;

	PRINT_FUNC
	UDMA_WARN_ON_RETURN((port >= UDMA_PORT_NUM_TOTAL),UDMA_INVALID_PARAM);

	UDMA_WARN_ON_RETURN((!buffer_desc)||(!buffer_desc->buf),UDMA_INVALID_PARAM);

	UDMA_WARN_ON_RETURN((buffer_desc->state != UDMA_BUFFER_INITIAL)||(!buffer_desc->private),UDMA_INVALID_PARAM);

	UDMA_WARN_ON_RETURN((buffer_desc->len < UDMA_MIN_RX_BUFFER_SIZE)||(buffer_desc->len > UDMA_MAX_RX_BUFFER_SIZE),UDMA_INVALID_PARAM);

	umdev = udma_devs[port];	
	UDMA_WARN_ON_RETURN((!umdev->tx_complete) || (!umdev->rx_complete),UDMA_UNINITIALIZED);
	hw = umdev->hw;
	
	rx = &umdev->rx_ring;

	//udma_dump_a_ring_progress(rx);
	if (is_ring_full(rx)) {
		tasklet_hi_schedule(&umdev->clean_tasklet);
		return UDMA_FULL;
	}
	dma = dma_map_single(&hw->pdev->dev, buffer_desc->buf, buffer_desc->len, DMA_FROM_DEVICE);
	if (dma_mapping_error(&hw->pdev->dev, dma)) {
		dev_err(&umdev->hw->pdev->dev, "DMA map failed\n");
		return UDMA_ERR;
	}
	
	/* Fill in the buffer info to the descriptor */
	spin_lock_irqsave(&rx->lock, flags);
	rx->new_tail = NEXT_RX(rx->new_tail);
	rx->buffer_desc[rx->new_tail] = buffer_desc;
	udma_desc = &rx->desc[rx->new_tail];
	udma_desc->priv = dma;
	udma_desc->union_field.size = buffer_desc->len & 0x00FFFFFF;
	hw->ops->update_rx_desc(hw, udma_desc, dma, buffer_desc->pos);
	hw->ops->enable_desc_rx_irq(udma_desc);

	//wmb();
	if (udma_rx_is_stopped(umdev)) {
		udma_start_rx_transfer(umdev);		
	}
	spin_unlock_irqrestore(&rx->lock, flags);
	return ret;
}

/**  udma_register_handler - register the Tx/Rx callback
 *
 * @port - udma port number, could be 0 or 1
 * @tx_handle - Tx callback. Once a buffer is send out, UDMA driver fills in UDMA_BUFFRE_TX_DONE to buffer descriptor information, and calls tx_handle
 * @rx_handle - Rx callback. Once a buffer is received, UDMA driver fills in UDMA_BUFFRE_RX_DONE and udma_packt_pos_t to buffer descriptor information, 
 			   and calls rx_handle
 *
 * Note that the context in which the tx_callback/rx_callback is called is that of a softIRQ
 *
 * A prototype of a udma_handle is:
 * 		int udma_handle(int port, udma_buffer_desc_t *buffer_desc);
 * 
 * At UDMA driver exit, 
 * 	it will mark the buffer descriptor as "UDMA_BUFFER_NULL", and send all of the unfinished Tx buffers to upper layer for clean by Tx_handle callback
 * 	it will mark the buffer descriptor as "UDMA_BUFFER_NULL", and send all of the received Rx free buffers to upper layer for clean by Rx_handle callback

 * return UDMA_OK, success
 * return UDMA_ERR, failure
*/
udma_result_t udma_register_handler(unsigned char port, udma_handle_t tx_handle, udma_handle_t rx_handle)
{
	struct udma_device *umdev = NULL;
	struct udma_hw *hw = NULL;
	int err = 0;
	PRINT_FUNC

	UDMA_WARN_ON_RETURN((port >= UDMA_PORT_NUM_TOTAL),UDMA_INVALID_PARAM);
	UDMA_WARN_ON_RETURN((!tx_handle || !rx_handle),UDMA_INVALID_PARAM);
	
	umdev = udma_devs[port];
	if (umdev == NULL) {
		udma_err("UDMA Driver is not installed\n");
		return UDMA_UNINITIALIZED;
	}
	hw = umdev->hw;
	if ((umdev->tx_complete)||(umdev->rx_complete)) {
		udma_err("err: UDMA device %d is already being used by others \n", port);
		return UDMA_BUSY;
	}
	umdev->tx_complete = tx_handle;
	umdev->rx_complete = rx_handle;
	
	hw->ops->hw_init(hw);

	err = request_irq(hw->pdev->irq, udma_intr, IRQF_SHARED,hw->port?UDMA1_NAME:UDMA0_NAME,umdev);
	if (err) {
		udma_err("Error to register UDMA interrupt for port %d, exit\n", hw->port);
		hw->ops->hw_exit(hw);		
		umdev->tx_complete = NULL;
		umdev->rx_complete = NULL;
		return UDMA_ERR;
	}
	hw->ops->enable_rx_irq(hw);
	hw->ops->enable_tx_irq(hw);
	umdev->status = UDMA_DEV_IS_OPEN;

	return UDMA_OK;
}

/**  udma_flush - Stop the UDMA and flush the pending requests in a UDMA port and recycling all of the buffers
 * 
 * UDMA driver will flush all of the pending Tx/Rx packets, updating the buffer status and return them to upper layer.
 * The buffers may be already handled or still in a initial status
 * The upper layer is expected to recycle the return buffers
 * 
 * @port - udma port number, could be 0 or 1
 * 
 * This function is expected to be called by upper layer when exit
 *
*/
void udma_flush(unsigned char port)
{
	struct udma_device *umdev = NULL;
	struct udma_hw *hw = NULL;
	int budget = 2 * UDMA_RING_ENTRY_NUM;
#define  MAX_UDMA_STOP_DELAY      (200)
	int delay;
	
	if (WARN_ON(port >= UDMA_PORT_NUM_TOTAL)) {
		udma_err("%s invalid parameters \n",__FUNCTION__);
		return;
	}
	udma_dbg("%s line %d\n", __FUNCTION__, __LINE__);
	
	umdev = udma_devs[port];
	hw = umdev->hw;

	/* Stop Tx and clean */
	hw->ops->stop_tx_transfer(hw);
	delay = MAX_UDMA_STOP_DELAY;
	while (!hw->ops->tx_is_stopped(hw) && delay--) {
		mdelay(1);
	} 
	
	hw->ops->clear_tx_irq(hw);

	udma_clean_finished_works(umdev,UDMA_TX,budget);
	udma_clean_unfinished_works(umdev, UDMA_TX,budget);

	/* Stop Rx and clean */
	hw->ops->stop_rx_transfer(hw);
	delay = MAX_UDMA_STOP_DELAY;
	while ((!hw->ops->rx_is_stopped(hw)) && (delay--)){
		mdelay(1);
	}
	hw->ops->clear_rx_irq(hw);

	udma_clean_finished_works(umdev,UDMA_RX,budget);
	udma_clean_unfinished_works(umdev, UDMA_RX,budget);

	free_irq(hw->pdev->irq, umdev);

	hw->ops->hw_exit(hw);
	umdev->tx_complete = NULL;
	umdev->rx_complete = NULL;
	umdev->status = UDMA_DEV_IS_CLOSED;

	return;
}

EXPORT_SYMBOL_GPL(udma_send_packet);
EXPORT_SYMBOL_GPL(udma_give_free_buffer);
EXPORT_SYMBOL_GPL(udma_register_handler);
EXPORT_SYMBOL_GPL(udma_flush);

/****************************************************************************
*
* The UDMA SW stack setup/initialization routine
*
****************************************************************************/

/**
 * @udma_alloc_ring_dma - allocate DMA memory for a ring structure
 **/
static int  udma_alloc_ring_dma(struct udma_device *udma_dev, 
					struct udma_ring *ring)
{
	struct pci_dev *pdev = udma_dev->hw->pdev;
	struct udma_desc* desc = NULL;
	int i = 0;
	BUG_ON(!udma_dev);
	BUG_ON(!ring);

#define UDMA_DESC_ALIGN  4
	ring->ring_dma_size = sizeof(struct udma_desc)*UDMA_RING_ENTRY_NUM;
	ring->ring_entries = UDMA_RING_ENTRY_NUM;
	ring->desc = dma_alloc_coherent(&pdev->dev, ring->ring_dma_size, &ring->ring_dma,
					GFP_KERNEL);
	if (!ring->desc)
		return -ENOMEM;

	desc = ring->desc;
	udma_dbg(" dma 0x%x\n",ring->ring_dma);

	/* Link the descripors one by one */
	while (i < UDMA_RING_ENTRY_NUM) {
		desc[i].next_desc = DESC_INDEX_TO_DESC_DMA((i+1)%UDMA_RING_ENTRY_NUM, ring);
		udma_dev->hw->ops->clear_desc(&desc[i]);
		i++;
	}
	return 0;
}

static int  udma_free_ring_dma(struct udma_device *udma_dev, struct udma_ring *ring)
{
	struct pci_dev *pdev = udma_dev->hw->pdev;
	BUG_ON(!udma_dev);
	BUG_ON(!ring);

	dma_free_coherent(&pdev->dev, ring->ring_dma_size, ring->desc, ring->ring_dma);
	ring->desc = NULL;
	ring->ring_dma	=	0;
	ring->ring_dma_size	=	0;

	return 0 ;
}

/**
 * udma_alloc_rings - Allocate memory for all rings
 * @udma_dev: board private structure to initialize
 */
static int  udma_init_rings(struct udma_device *udma_dev)
{

	memset(&udma_dev->rx_ring, 0, sizeof(struct udma_ring));
	memset(&udma_dev->tx_ring, 0, sizeof(struct udma_ring));
	spin_lock_init(&udma_dev->rx_ring.lock);
	spin_lock_init(&udma_dev->tx_ring.lock);
	udma_alloc_ring_dma(udma_dev, &udma_dev->rx_ring);
	udma_alloc_ring_dma(udma_dev, &udma_dev->tx_ring);

	return 0;
}
static int  udma_destroy_rings(struct udma_device *udma_dev)
{
	
	udma_free_ring_dma(udma_dev, &udma_dev->rx_ring);
	udma_free_ring_dma(udma_dev, &udma_dev->tx_ring);

	memset(&udma_dev->rx_ring, 0, sizeof(struct udma_ring));
	memset(&udma_dev->tx_ring, 0, sizeof(struct udma_ring));
	return 0;
}
static int udma_dev_init(struct udma_device *umdev)
{

	/* Init tasklet */
	tasklet_init(&umdev->clean_tasklet, udma_tasklet_clean,(unsigned long)umdev);

	umdev->status 	=	UDMA_DEV_IS_CLOSED;

	umdev->clean_irq = udma_ring_clean_irq;
	umdev->clean_unused = udma_ring_clean_unused;

	udma_init_rings(umdev);
	PRINT_FUNC
	return 0;
}

/*
 * Driver exit routine
 * 
*/
static void udma_dev_exit(struct udma_device *umdev)
{
	BUG_ON(!umdev);
	umdev->clean_irq = NULL;
	umdev->clean_unused = NULL;
	udma_destroy_rings(umdev);

	return ;
}

struct udma_hw *udma_alloc_hw(size_t size)
{
	struct udma_device *umdev = NULL;
	struct udma_hw *hw = NULL;
	
	hw = kzalloc(sizeof(struct udma_device) + size, GFP_KERNEL);
	if (NULL == hw) {
			udma_err("Cannot allocate memory\n");
			return ERR_PTR(-ENOMEM);
	}
	
	umdev = (struct udma_device *)udma_hw_priv(hw);
	umdev->hw = hw;

	return hw;
}
EXPORT_SYMBOL_GPL(udma_alloc_hw);

#ifdef DEBUG
/* For debug purpose */
static int udma_open(struct inode *inode, struct file *filp)
{
        return 0;
}
static int udma_close(struct inode *inode, struct file *filp)
{
        return 0;
}

static long udma_unlocked_ioctl(struct file *filp, unsigned int arg, unsigned long cmd)
{
	int i = 0;
	struct udma_device *umdev = NULL;
	struct udma_hw *hw = NULL;
	struct udma_ring *ring = NULL;
	PRINT_FUNC

	if (arg - UDMA_PORT_MAGIC_BASE >= UDMA_PORT_NUM_TOTAL)
		return -EIO;
	umdev = udma_devs[arg - UDMA_PORT_MAGIC_BASE];
	hw = umdev->hw;
	switch (cmd) {
    case UDMA_DUMP_TX_CURR_RING: 
		ring = &umdev->tx_ring;
		udma_dump_a_ring(umdev, ring, UDMA_TX);
         break;

    case UDMA_DUMP_RX_CURR_RING: 
		ring = &umdev->rx_ring;		
		udma_dump_a_ring(umdev, ring, UDMA_RX);

         break;

    case UDMA_DUMP_TX_RING0:
		ring = &umdev->tx_ring;		
		udma_dump_a_ring(umdev, ring, UDMA_TX);
        break;
 	case UDMA_DUMP_TX_RING1:

		 break;

	case UDMA_DUMP_RX_RING0:		 
		ring = &umdev->rx_ring;		
		udma_dump_a_ring(umdev, ring, UDMA_RX);
         break;
    case UDMA_DUMP_RX_RING1:
		break;
	case UDMA_DUMP_CURR_TX_REGS:
		udma_regs_dump(hw,UDMA_TX);
		break;
	case UDMA_DUMP_CURR_RX_REGS:
		udma_regs_dump(hw,UDMA_RX);
		break;
	case UDMA_DUMP_DBG_SPACE:
		//udma_dump_frame();
		break;
	case UDMA_DUMP_SND_PKT:
		//udma_dump_frame();
		break;
    default:
         printk(KERN_ERR "UDMA driver receive Wrong IOCTL command = 0x%x \n",cmd);
         return -EFAULT;
    }

	return 0;
}
static struct file_operations udma_fops = {
	.owner   		= THIS_MODULE,
 	.unlocked_ioctl 	= udma_unlocked_ioctl,
	.open 			= udma_open,
	.release 		= udma_close,
};
#endif

int __devinit udma_setup_sw(void *dev)
{
	struct udma_device *umdev = (struct udma_device *)dev;
#ifdef DEBUG	
	struct proc_dir_entry	*proc;
#endif
	int err;

	if (WARN_ON(NULL == umdev)) return -EINVAL;

	err = udma_dev_init(umdev);
	if (0 != err) {
		udma_err("UDMA SW layer setup failure\n");
		return -ENODEV;
	}
	udma_devs[umdev->hw->port] = umdev;
	udma_dump_all_rings(umdev->hw->port);

#ifdef DEBUG

	if (!umdev->hw->port) {
		proc = create_proc_entry( UDMA_PROC_FS, S_IRUSR|S_IWUSR | S_IRGRP |S_IWGRP |S_IROTH |S_IWOTH, NULL);
		if ( NULL != proc ) 
			proc->proc_fops = &udma_fops;
		else {
			printk("ERROR - DEVICE_NAME initialization- Can not create proc entry\n");
			return -EIO;
		}
	}
#endif	
	return 0;
}

/**
 * udma_unregister - Called by hw layer as removal routine
 **/
void __devexit udma_free_sw(void *dev)
{
	struct udma_device *umdev = (struct udma_device *)dev;

	if (WARN_ON(umdev == NULL)) return ;

	udma_dev_exit(umdev);

	udma_devs[umdev->hw->port]  = NULL;

#ifdef DEBUG
	if (!umdev->hw->port) 
		remove_proc_entry(UDMA_PROC_FS, NULL);	
#endif
	return ;
}
EXPORT_SYMBOL_GPL(udma_setup_sw);
EXPORT_SYMBOL_GPL(udma_free_sw);

/* udma_main.c */
