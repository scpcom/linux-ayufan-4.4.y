/*
 * offload engine driver for the Marvell XOR engine
 * Copyright (C) 2007, 2008, Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/memory.h>
#include <plat/mv_memcpy.h>
#include <linux/proc_fs.h>

#include "mv_memcpy.h"

static struct proc_dir_entry *mv_memcpy_proc;

struct {
	unsigned int avg_busy_loops;
	unsigned int avg_busy_cnt;
	unsigned int issue_pen;
	unsigned int prep;
	unsigned int tx_status;
	unsigned int break_chain;
} mv_memcpy_stats;

#define MV_MEMCPY_BANK_SIZE 8

#if 0
#ifdef CONFIG_PM
static struct mv_memcpy_save_regs saved_regs;
#endif
#endif

static void mv_memcpy_set_mode(struct mv_memcpy_chan *chan,
			       enum dma_transaction_type type)
{
	u32 op_mode;
	u32 config = __raw_readl(MEMCPY_CONFIG(chan));

	switch (type) {
	case DMA_MEMCPY:
		op_mode = MEMCPY_OPERATION_MODE_MEMCPY;
		break;
	default:
		BUG();
		return;
	}

	config &= ~0x7;
	config |= op_mode;
	__raw_writel(config, MEMCPY_CONFIG(chan));
}

static void mv_memcpy_set_next_desc(struct mv_memcpy_chan *chan,
				    u32 next_desc_addr)
{
	__raw_writel(next_desc_addr, MEMCPY_NEXT_DESC(chan));
}

#define to_mv_memcpy_chans(chan)		\
	container_of(chan, struct mv_memcpy_chans, common)

static void mv_memcpy_chan_activate(struct mv_memcpy_chan *chan)
{
	__raw_writel(1, MEMCPY_ACTIVATION(chan));
}

static char mv_memcpy_chan_is_busy(struct mv_memcpy_chan *chan)
{
	u32 state = __raw_readl(MEMCPY_ACTIVATION(chan));

	state = (state >> 4) & 0x3;
	return (state == 1) ? 1 : 0;
}

static void mv_memcpy_free_bank(struct dma_chan *chan,
				int channel_num,
				int bank_num)
{
	struct mv_memcpy_chans *mv_chans = to_mv_memcpy_chans(chan);
	struct mv_memcpy_chan *mv_chan = &mv_chans->chan[channel_num];
	struct device *dev = mv_chans->common.device->dev;
	struct dma_async_tx_descriptor *txd;
	unsigned int buff_index;
	int j;

	for (j = 0; j < mv_chans->coalescing; j++) {
		buff_index = (bank_num * MV_MEMCPY_BANK_SIZE) + j;
		txd = &mv_chan->buff_info[buff_index].async_tx;
		/* Unmap the dst buffer, if requested */
		if (!(txd->flags & DMA_COMPL_SKIP_DEST_UNMAP)) {
			if (txd->flags & DMA_COMPL_DEST_UNMAP_SINGLE)
				dma_unmap_single(dev,
						 mv_chan->buff_info[buff_index].dst_addr,
						 mv_chan->buff_info[buff_index].len,
						 DMA_FROM_DEVICE);

			else
				dma_unmap_page(dev,
					       mv_chan->buff_info[buff_index].dst_addr,
					       mv_chan->buff_info[buff_index].len,
					       DMA_FROM_DEVICE);

		}

		/* Unmap the src buffer, if requested */
		if (!(txd->flags & DMA_COMPL_SKIP_SRC_UNMAP)) {
			if (txd->flags & DMA_COMPL_SRC_UNMAP_SINGLE)
				dma_unmap_single(dev,
						 mv_chan->buff_info[buff_index].src_addr,
						 mv_chan->buff_info[buff_index].len,
						 DMA_TO_DEVICE);
			else
				dma_unmap_page(dev,
					       mv_chan->buff_info[buff_index].src_addr,
					       mv_chan->buff_info[buff_index].len,
					       DMA_TO_DEVICE);
		}
	}
	return;
}

/************************ DMA engine API functions ****************************/

/**
 * mv_memcpy_status - poll the status of an MEMCPY transaction
 * @chan: MEMCPY channel handle
 * @cookie: MEMCPY transaction identifier
 * @txstate: MEMCPY transactions state holder (or NULL)
 */
static enum dma_status mv_memcpy_tx_status(struct dma_chan *chan,
					   dma_cookie_t cookie,
					   struct dma_tx_state *txstate)
{
	struct mv_memcpy_chans *mv_chans = to_mv_memcpy_chans(chan);
	struct mv_memcpy_chan *mv_chan;
	unsigned long irq_flags;
	struct mv_memcpy_desc *desc;
	int i;
	int last_index;

	spin_lock_irqsave(&mv_chans->lock, irq_flags);
	mv_memcpy_stats.tx_status++;

	for (i = 0; i < mv_chans->num_channels; i++) {
		mv_chan = &mv_chans->chan[i];

		/* let the channels finish pending descriptors */
		while (mv_memcpy_chan_is_busy(&mv_chans->chan[i]))
			;

		/* free bank */
		if (mv_chan->active_bank != -1)
			mv_memcpy_free_bank(chan, i, mv_chan->active_bank);

		/* if we have some descriptors pending in the current bank chain flush them */
		if (mv_chan->next_index) {
			mv_memcpy_stats.break_chain++;
			last_index = (mv_chan->next_bank * MV_MEMCPY_BANK_SIZE) + mv_chan->next_index - 1;

			/* we're triggering so cut the chain */
			desc = &mv_chan->dma_desc_pool_virt[last_index];
			desc->phy_next_desc = 0x0;

			mv_chan->active_bank = mv_chan->next_bank;

			/* set the descriptor pointer */
			mv_memcpy_set_next_desc(mv_chan, mv_chan->dma_desc_pool +
						((mv_chan->active_bank * MV_MEMCPY_BANK_SIZE) *
						 sizeof(struct mv_memcpy_desc)));

			/* trigger channel */
			wmb();
			mv_memcpy_chan_activate(mv_chan);
		}
	}

	for (i = 0; i < mv_chans->num_channels; i++) {
		mv_chan = &mv_chans->chan[i];

		/* if we have some descriptors pending in the current bank chain flush them */
		if (mv_chan->next_index) {
			/* let the channels finish pending descriptors */
			while (mv_memcpy_chan_is_busy(mv_chan))
				;

			/* free bank */
			mv_memcpy_free_bank(chan, i, mv_chan->active_bank);

			/* nothing to clean on next round */
			mv_chan->active_bank = -1;
			/* switch bank */
			mv_chan->next_bank = 1 - mv_chan->next_bank;
			mv_chan->next_index = 0;
		}
	}

	spin_unlock_irqrestore(&mv_chans->lock, irq_flags);
	return DMA_SUCCESS;
}

static dma_cookie_t
mv_memcpy_tx_submit(struct dma_aysnc_tx_descriptor *tx)
{
	return 1;
}

/* returns the number of allocated descriptors */
static int mv_memcpy_alloc_chan_resources(struct dma_chan *chan)
{
	struct mv_memcpy_chans *mv_chans = to_mv_memcpy_chans(chan);

	return mv_chans->num_channels * mv_chans->coalescing;
}

static struct dma_async_tx_descriptor *
mv_memcpy_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
			  size_t len, unsigned long flags)
{
	struct mv_memcpy_chans *mv_chans = to_mv_memcpy_chans(chan);
	struct mv_memcpy_chan *mv_chan = &mv_chans->chan[mv_chans->next_chan];
	struct mv_memcpy_desc *desc;
	u32 next_desc;
	struct mv_memcpy_desc tmp_desc;
	unsigned long irq_flags;
	unsigned int trigger_chan = 0;
	unsigned int current_desc;
	int last_bank;

	mv_memcpy_stats.prep++;

	spin_lock_irqsave(&mv_chans->lock, irq_flags);

	last_bank = mv_chan->active_bank;

	current_desc = (mv_chan->next_bank * MV_MEMCPY_BANK_SIZE) + mv_chan->next_index;

	/* fill the descriptor */
	desc = &mv_chan->dma_desc_pool_virt[current_desc];
	next_desc = mv_chan->dma_desc_pool +
		((mv_chan->next_bank * MV_MEMCPY_BANK_SIZE + mv_chan->next_index + 1)
		 * sizeof(struct mv_memcpy_desc));

	tmp_desc.status = (0x1 << 31);
	tmp_desc.desc_command = 0x1 | (0x1 << 31);
	tmp_desc.byte_count = len;
	tmp_desc.phy_dest_addr = dest;
	tmp_desc.phy_src_addr[0] = src;
	tmp_desc.phy_next_desc = next_desc;

	/* incrementing next index and bank to use */
	mv_chan->next_index = (mv_chan->next_index + 1) % mv_chans->coalescing;
	if (!mv_chan->next_index) {
		/* saving the bank to trigger */
		mv_chan->active_bank = mv_chan->next_bank;
		mv_chan->next_bank = 1 - mv_chan->next_bank;
		trigger_chan = 1;
		/* we're triggering so cut the chain */
		tmp_desc.phy_next_desc = 0x0;
	}

	/* write desc */
	memcpy(desc, &tmp_desc, sizeof(struct mv_memcpy_desc));

	/* update sw chain */
	mv_chan->buff_info[current_desc].src_addr = src;
	mv_chan->buff_info[current_desc].dst_addr = dest;
	mv_chan->buff_info[current_desc].len = len;
	mv_chan->buff_info[current_desc].async_tx.flags = flags;
	mv_chan->buff_info[current_desc].async_tx.tx_submit = mv_memcpy_tx_submit;
	mv_chan->buff_info[current_desc].async_tx.phys = (dma_addr_t) desc;
	mv_chan->buff_info[current_desc].async_tx.chan = chan;

	/* trigger if needed */
	if (trigger_chan) {
		/* Check if engine is idle. */
		while (mv_memcpy_chan_is_busy(mv_chan))
			mv_memcpy_stats.avg_busy_loops++;

		/* Set the descriptor pointer */
		mv_memcpy_set_next_desc(mv_chan, mv_chan->dma_desc_pool +
					((mv_chan->active_bank * MV_MEMCPY_BANK_SIZE) *
					 sizeof(struct mv_memcpy_desc)));

		/* trigger channel */
		wmb();
		mv_memcpy_chan_activate(mv_chan);

		/* free bank */
		if (mv_chan->active_bank != -1)
			mv_memcpy_free_bank(chan, mv_chans->next_chan, last_bank);

		/* set next channel */
		mv_chans->next_chan = (mv_chans->next_chan + 1) % mv_chans->num_channels;
	}

	spin_unlock_irqrestore(&mv_chans->lock, irq_flags);

	return &(mv_chan->buff_info[current_desc].async_tx);

}

static void mv_memcpy_free_chan_resources(struct dma_chan *chan)
{
	return;
}

static void mv_memcpy_issue_pending(struct dma_chan *chan)
{
	mv_memcpy_stats.issue_pen++;

	return;
}

static int __devexit mv_memcpy_remove(struct platform_device *dev)
{
	return 0;
}

static void
mv_memcpy_conf_mbus_windows(void __iomem *base,
			    struct mbus_dram_target_info *dram)
{
	u32 win_enable = 0;
	int i;

	for (i = 0; i < 8; i++) {
		writel(0, base + WINDOW_BASE(i));
		writel(0, base + WINDOW_SIZE(i));
		if (i < 4)
			writel(0, base + WINDOW_REMAP_HIGH(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->base & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       dram->mbus_dram_target_id, base + WINDOW_BASE(i));
		writel((cs->size - 1) & 0xffff0000, base + WINDOW_SIZE(i));

		win_enable |= (1 << i);
		win_enable |= 3 << (16 + (2 * i));
	}

	writel(win_enable, base + WINDOW_BAR_ENABLE(0));
	writel(win_enable, base + WINDOW_BAR_ENABLE(1));
}

#if 0

#ifdef CONFIG_PM
static int mv_memcpy_suspend(struct platform_device *dev, pm_message_t state)
{
}

static int mv_memcpy_resume(struct platform_device *dev)
{
}

#endif /* CONFIG_PM*/
#endif

int mv_memcpy_proc_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *zero, void *ptr)
{
	int count = 0;
	char tmp_buffer[1000] = { 0 };

	if (offset > 0)
		return 0;

	count += sprintf(tmp_buffer + count, "AVG busy loops            - %d\n", mv_memcpy_stats.avg_busy_loops);
	count += sprintf(tmp_buffer + count, "chain broke               - %d\n", mv_memcpy_stats.break_chain);

	count += sprintf(tmp_buffer + count, "mv_memcpy_issue_pending   - %d\n", mv_memcpy_stats.issue_pen);
	count += sprintf(tmp_buffer + count, "mv_memcpy_prep_dma_memcpy - %d\n", mv_memcpy_stats.prep);
	count += sprintf(tmp_buffer + count, "mv_memcpy_tx_status       - %d\n", mv_memcpy_stats.tx_status);

	*(tmp_buffer + count) = '\0';
	sprintf(buffer, "%s", tmp_buffer);
	return count;
}

static int __devinit mv_memcpy_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int i = 0;
	int j;
	int chan_id;
	struct mv_memcpy_device *mv_dev;
	struct mv_memcpy_chans *mv_chans;
	struct mv_memcpy_chan *mv_chan;
	struct dma_device *dma_dev;
	struct mv_memcpy_platform_data *plat_data = pdev->dev.platform_data;
	struct resource *res;

	dev_printk(KERN_NOTICE, &pdev->dev, "Marvell MEMCPY driver\n");

	/* allocate device */
	mv_dev = devm_kzalloc(&pdev->dev, sizeof(*mv_dev), GFP_KERNEL);
	if (!mv_dev)
		return -ENOMEM;

	/* set number of engines allocated to this driver */
	mv_dev->num_engines = pdev->num_resources;

	/*
	 * allocate channel
	 * this is kind of a fake channel
	 * it holds all the engines channels selected and appear to the
	 * dma driver as one channel only, the driver handle the internal switching
	 */
	mv_chans = devm_kzalloc(&pdev->dev, sizeof(*mv_chans), GFP_KERNEL);
	if (!mv_chans)
		return -ENOMEM;

	mv_chans->device = mv_dev;
	mv_chans->num_channels = mv_dev->num_engines * 2;
	/* start work from channels #0 */
	mv_chans->next_chan = 0;
	mv_chans->coalescing = plat_data->coalescing;

	dev_printk(KERN_NOTICE, &pdev->dev, "initiating %d engines\n", mv_dev->num_engines);

	for (i = 0; i < mv_dev->num_engines; i++) {
		/* get engine resources */
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			return -ENODEV;

		mv_dev->engine_base[i] = devm_ioremap(&pdev->dev, res->start,
						      resource_size(res));
		if (!mv_dev->engine_base[i])
			return -EBUSY;

		/*
		 * (Re-)program MBUS remapping windows if we are asked to.
		 */
		if (plat_data != NULL && plat_data->dram != NULL)
			mv_memcpy_conf_mbus_windows(mv_dev->engine_base[i], plat_data->dram);

		/* initialize both channels on selected engines */
		for (j = 0; j < 2; j++) {
			/* allocate coherent memory for hardware descriptors
			 * note: writecombine gives slightly better performance, but
			 * requires that we explicitly flush the writes
			 */
			chan_id = (i * 2) + j;

			mv_chan = &mv_chans->chan[chan_id];

			if (!mv_chan)
				return -ENOMEM;

			mv_chan->dma_desc_pool_virt = dma_alloc_writecombine(&pdev->dev,
									     plat_data->pool_size,
									     &mv_chan->dma_desc_pool,
									     GFP_KERNEL);
			if (!mv_chan->dma_desc_pool_virt)
				return -ENOMEM;

			/* internal channel id */
			mv_chan->idx = j;
			mv_chan->base = mv_dev->engine_base[i];

			if (!mv_chan->base) {
				ret = -ENOMEM;
				goto err_free_dma;
			}

			/* set channels as memcpy */
			mv_memcpy_set_mode(mv_chan, DMA_MEMCPY);

			/* initialize indexes*/
			mv_chan->next_bank = 0;
			mv_chan->active_bank = -1;
			mv_chan->next_index = 0;
		}
	}

	/* general initializations */
	dma_dev = &mv_dev->common;

	/* only memcpy capability */
	dma_cap_set(DMA_MEMCPY, dma_dev->cap_mask);

	/* save driver data */
	mv_dev->pdev = pdev;
	platform_set_drvdata(pdev, mv_dev);

	INIT_LIST_HEAD(&dma_dev->channels);

	/* set base routines */
	dma_dev->device_alloc_chan_resources = mv_memcpy_alloc_chan_resources;
	dma_dev->device_free_chan_resources = mv_memcpy_free_chan_resources;
	dma_dev->device_tx_status = mv_memcpy_tx_status;
	dma_dev->device_issue_pending = mv_memcpy_issue_pending;
	dma_dev->dev = &pdev->dev;
	dma_dev->device_prep_dma_memcpy = mv_memcpy_prep_dma_memcpy;

	spin_lock_init(&mv_chans->lock);
	mv_chans->common.device = dma_dev;

	list_add_tail(&mv_chans->common.device_node, &dma_dev->channels);

	dev_printk(KERN_INFO, &pdev->dev, "Marvell MEMCPY v2");
	dma_async_device_register(dma_dev);

	/* proc entry for statistics */
	mv_memcpy_proc = create_proc_entry("mv_memcpy_stats", 0666, NULL);
	mv_memcpy_proc->read_proc = mv_memcpy_proc_read;
	mv_memcpy_proc->write_proc = NULL;
	mv_memcpy_proc->nlink = 1;

	goto out;

err_free_dma:
	for (i = 0; i < mv_chans->num_channels; i++)
		dma_free_coherent(&mv_dev->pdev->dev, plat_data->pool_size,
				  mv_chans->chan[i].dma_desc_pool_virt, mv_chans->chan[i].dma_desc_pool);
out:
	return ret;
}

static struct platform_driver mv_memcpy_driver = {
	.probe		= mv_memcpy_probe,
	.remove		= __devexit_p(mv_memcpy_remove),
#if 0
#ifdef CONFIG_PM
	.suspend	= mv_memcpy_suspend,
	.resume		= mv_memcpy_resume,
#endif
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MV_MEMCPY_NAME,
	},
};
/*----------------------------------------------------------------------------*/
/* Module Init / Exit                                                         */
/*----------------------------------------------------------------------------*/

static __init int mv_memcpy_init(void)
{
	return platform_driver_register(&mv_memcpy_driver);
}

static void __exit mv_memcpy_exit(void)
{
	platform_driver_unregister(&mv_memcpy_driver);
}

module_init(mv_memcpy_init);
module_exit(mv_memcpy_exit);

MODULE_AUTHOR("Lior Amsalem <alior@marvell.com>");
MODULE_DESCRIPTION("DMA engine driver for Marvell's memcpy engine");
MODULE_LICENSE("GPL");
