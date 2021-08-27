 
#include "al_dma.h"

struct dma_async_tx_descriptor *al_dma_prep_interrupt_lock(
	struct dma_chan *c,
	unsigned long flags)
{
	struct al_dma_chan *chan = to_al_dma_chan(c);
	struct dma_async_tx_descriptor *txd = NULL;
	int idx;
	int32_t rc;
	struct al_dma_sw_desc *desc;
	struct al_raid_transaction *xaction;

	dev_dbg(
		chan->device->common.dev,
		"%s: chan->idx = %d, flags = %08x\n",
		__func__,
		chan->idx,
		(unsigned int)flags);

	if (likely(al_dma_get_sw_desc_lock(chan, 1) == 0))
		idx = chan->head;
	else {
		dev_dbg(
			chan->device->common.dev,
			"%s: al_dma_get_sw_desc_lock failed!\n",
			__func__);

		return NULL;
	}

	chan->sw_desc_num_locked = 1;

	desc = al_dma_get_ring_ent(chan, idx);

	desc->umap_ent_cnt = 0;

	txd = &desc->txd;

	desc->txd.flags = flags;

	xaction = &desc->hal_xaction;
	memset(xaction, 0, sizeof(struct al_raid_transaction));
	xaction->op = AL_RAID_OP_NOP;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	xaction->flags |= AL_SSM_INTERRUPT;
#else
	xaction->flags |= AL_RAID_INTERRUPT;
#endif
	if (flags & DMA_PREP_FENCE)
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
		xaction->flags |= AL_SSM_BARRIER;
#else
		xaction->flags |= AL_RAID_BARRIER;
#endif

	if (flags & (~(DMA_PREP_INTERRUPT | DMA_PREP_FENCE)))
		dev_err(
			chan->device->common.dev,
			"%s: flags = %08x\n",
			__func__,
			(unsigned int)flags);

	xaction->num_of_srcs = 0;
	xaction->total_src_bufs = 0;

	xaction->num_of_dsts = 0;
	xaction->total_dst_bufs = 0;

	dev_dbg(
		chan->device->common.dev,
		"%s: xaction->flags = %08x\n",
		__func__,
		xaction->flags);

	rc = al_raid_dma_prepare(chan->hal_raid, chan->idx,
				&desc->hal_xaction);
	if (unlikely(rc)) {
		dev_err(chan->device->common.dev, 
			"%s: al_raid_dma_prepare failed!\n", __func__);
		spin_unlock_bh(&chan->prep_lock);
		return NULL;
	}

	chan->tx_desc_produced += desc->hal_xaction.tx_descs_count;

	AL_DMA_STATS_UPDATE(
		chan,
		chan->stats_prep.int_num,
		1,
		chan->stats_prep.int_num,  
		0);

	al_dma_tx_submit_sw_cond_unlock(chan, txd);

	return txd;
}
