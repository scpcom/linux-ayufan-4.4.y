/*
 * Copyright (c) 2012, Mindspeed Technologies.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/designware.h>

#include <linux/delay.h>

#include "spi-c2000-dma.h"

#define	SPDBG_MIN	0

struct spi_dma {
        struct spi_dma_slave      dmas_tx;
        struct spi_dma_slave      dmas_rx;
};

extern void dwspi_enable(struct designware_spi *dwspi, int on);

static inline void dw_writew(struct designware_spi *dws, u32 offset, u16 val)
{
        __raw_writew(val, dws->regs + offset);
}

static inline u16 dw_readw(struct designware_spi *dws, u32 offset)
{
        return __raw_readw(dws->regs + offset);
}

static int spi_dma_init(struct designware_spi *dws)
{
        struct spi_dma *dw_dma = dws->dma_priv;
        struct spi_dma_slave *rxs, *txs;
        dma_cap_mask_t mask;

        dma_cap_zero(mask);
        dma_cap_set(DMA_SLAVE, mask);

        /* 1. Init rx channel */
        dws->rxchan = dma_request_channel(mask, NULL, dws);
        if (!dws->rxchan){
		printk("%s:%d: dma_request_channel failed.\n", \
				__func__, __LINE__);
                goto err_exit;
	}

        rxs = &dw_dma->dmas_rx;
	rxs->dma_dev = dws->rxchan->device->dev;
        dws->rxchan->private = rxs;
        rxs->cfg_hi = (DMA_CFG_SRC_PER & DMA_CFG_SRC_PER_MASK) << DMA_CFG_SRC_PER_SHIFT \
                        |(DMA_CFG_DEST_PER & DMA_CFG_DEST_PER_MASK) << DMA_CFG_DEST_PER_SHIFT \
                        |(DMA_CFG_PROTCTL & DMA_CFG_PROTCTL_MASK) << DMA_CFG_PROTCTL_SHIFT;

        rxs->cfg_lo = (DMA_CFG_CH_SUSP & DMA_CFG_CH_SUSP_MASK) << DMA_CFG_CH_SUSP_SHIFT \
                        |(DMA_CFG_FIFO_EMPTY & DMA_CFG_FIFO_EMPTY_MASK) << DMA_CFG_FIFO_EMPTY_SHIFT \
                        |(DMA_CFG_HS_SEL_SRC & DMA_CFG_HS_SEL_SRC_MASK) << DMA_CFG_HS_SEL_SRC_SHIFT \
                        |(DMA_CFG_HS_SEL_DST & DMA_CFG_HS_SEL_DST_MASK) << DMA_CFG_HS_SEL_DST_SHIFT;

        rxs->src_master = (DMA_CTL_SMS & DMA_CTL_SMS_MASK);
        rxs->dst_master = (DMA_CTL_DMS & DMA_CTL_DMS_MASK);

        /* 2. Init tx channel */
        dws->txchan = dma_request_channel(mask, NULL, dws);
        if (!dws->txchan){
		printk("%s:%d: dma_request_channel failed.\n", \
				__func__, __LINE__);
                goto free_rxchan;
	}

        txs = &dw_dma->dmas_tx;
	txs->dma_dev = dws->txchan->device->dev;
        dws->txchan->private = txs;

        txs->cfg_hi = (DMA_CFG_SRC_PER_WR & DMA_CFG_SRC_PER_MASK) << DMA_CFG_SRC_PER_SHIFT \
                        |(DMA_CFG_DEST_PER_WR & DMA_CFG_DEST_PER_MASK) << DMA_CFG_DEST_PER_SHIFT \
                        |(DMA_CFG_PROTCTL & DMA_CFG_PROTCTL_MASK) << DMA_CFG_PROTCTL_SHIFT;

        txs->cfg_lo = (DMA_CFG_CH_SUSP & DMA_CFG_CH_SUSP_MASK) << DMA_CFG_CH_SUSP_SHIFT \
                        |(DMA_CFG_FIFO_EMPTY & DMA_CFG_FIFO_EMPTY_MASK) << DMA_CFG_FIFO_EMPTY_SHIFT \
                        |(DMA_CFG_HS_SEL_SRC_WR & DMA_CFG_HS_SEL_SRC_MASK) << DMA_CFG_HS_SEL_SRC_SHIFT \
                        |(DMA_CFG_HS_SEL_DST_WR & DMA_CFG_HS_SEL_DST_MASK) << DMA_CFG_HS_SEL_DST_SHIFT;

        txs->src_master = (DMA_CTL_SMS_WR & DMA_CTL_SMS_MASK);
        txs->dst_master = (DMA_CTL_DMS_WR & DMA_CTL_DMS_MASK);

        dws->dma_inited = 1;
	printk("%s:%d: dma_ineted set to 1.\n", __func__, __LINE__);

        return 0;

free_rxchan:
        dma_release_channel(dws->rxchan);
err_exit:
	printk("%s:%d: dma_ineted NOT set to 1.\n", __func__, __LINE__);
        return -1;

}

static void spi_dma_exit(struct designware_spi *dws)
{
        dma_release_channel(dws->txchan);
        dma_release_channel(dws->rxchan);
}

/*
 * Completion callback for rx/tx channel.
 */
static void dw_spi_dma_done(void *arg)
{
        struct designware_spi *dwspi = arg;

        complete(&dwspi->done);
}

static void set_tmode_ndf(struct designware_spi *dws)
{
	if((dws->tx_dma && dws->tx_len) && (dws->rx_dma && dws->rx_len)){
		u32 ctrlr0 = readw(dws->regs + DWSPI_CTRLR0);
		u32 tm = 0x0300;

#define	SPI_DMA_MAX_LEN	4095U

		if (dws->rx_len > SPI_DMA_MAX_LEN){
			printk ("%s:%d: DMA read len(%d bytes) not supported.\n", \
					__func__, __LINE__, dws->rx_len);
			return;
		}

		ctrlr0 &= ~DWSPI_CTRLR0_TMOD_MASK;
		ctrlr0 |= tm;

		writew(ctrlr0, dws->regs + DWSPI_CTRLR0);
		dw_writew(dws, DWSPI_CTRLR1, dws->rx_len-1);

		return;
	}

	if((dws->tx_dma && dws->tx_len) && !(dws->rx_dma && dws->rx_len)){
		u32 ctrlr0 = readw(dws->regs + DWSPI_CTRLR0);
		u32 tm = 0x0100;

		if (dws->tx_len > SPI_DMA_MAX_LEN){
			printk ("%s:%d: DMA write len(%d bytes) not supported.\n", \
					__func__, __LINE__, dws->tx_len);
			return;
		}

		ctrlr0 &= ~DWSPI_CTRLR0_TMOD_MASK;
		ctrlr0 |= tm;

		writew(ctrlr0, dws->regs + DWSPI_CTRLR0);
		dw_writew(dws, DWSPI_CTRLR1, dws->tx_len-1);

		return;
	}

	if(!(dws->tx_dma && dws->tx_len) && (dws->rx_dma && dws->rx_len)){
		u32 ctrlr0 = readw(dws->regs + DWSPI_CTRLR0);
		u32 tm = 0x0200;

		if (dws->tx_len > SPI_DMA_MAX_LEN){
			printk ("%s:%d: DMA read len(%d bytes) not supported.\n", \
					__func__, __LINE__, dws->tx_len);
			return;
		}

		ctrlr0 &= ~DWSPI_CTRLR0_TMOD_MASK;
		ctrlr0 |= tm;

		writew(ctrlr0, dws->regs + DWSPI_CTRLR0);
		dw_writew(dws, DWSPI_CTRLR1, dws->rx_len-1);

		return;
	}

	return;
}

static int spi_dma_transfer(struct designware_spi *dws, int cs_change)
{
	struct dma_async_tx_descriptor *txdesc = NULL, *rxdesc = NULL;
	struct dma_chan *txchan, *rxchan;
	struct dma_slave_config txconf, rxconf;
	int ret=0;

#define BUSY            (1<<0)
	/* 1. setup DMA related registers */
	if (cs_change) {

		while(readb(dws->regs + DWSPI_SR) & BUSY){
			printk ("%s:%d:SSI busy waiting.\n", __func__, __LINE__);
		}
		dwspi_enable(dws, 0);

		set_tmode_ndf(dws);

		writew(0, dws->regs + DWSPI_TXFTLR);
		writew(0, dws->regs + DWSPI_RXFTLR);
		writew(0, dws->regs + DWSPI_TXFLR);
		writew(0, dws->regs + DWSPI_RXFLR);
		writew(0, dws->regs + DWSPI_IMR);

		dw_writew(dws, DWSPI_DMACR, 0x3);
		if(dws->tx_dma)
			dw_writew(dws, DWSPI_DMATDLR, 0x4);
		else
			dw_writew(dws, DWSPI_DMATDLR, 0x10);

		dw_writew(dws, DWSPI_DMARDLR, 0xF);

		dwspi_enable(dws, 1);
		writel(0x1 << dws->spi->chip_select, \
				dws->regs + DWSPI_SER);
	}

	if(dws->tx_dma && dws->tx_len){
#if SPDBG_MIN
		printk("%s:%d: Prepare the TX dma transfer. Length=%d\n", \
				__func__, __LINE__, dws->tx_len);

#endif
		txchan = dws->txchan;
		/* 2. Prepare the TX dma transfer */
		txconf.direction = DMA_MEM_TO_DEV;
		txconf.dst_addr = dws->dma_addr;
		txconf.src_addr = dws->tx_dma;
		txconf.dst_maxburst = DMA_CTL_DEST_MSIZE_WR;
		txconf.src_maxburst = DMA_CTL_SRC_MSIZE;
		txconf.src_addr_width = 0;
		txconf.dst_addr_width = 0;
		txconf.device_fc = false;

		txchan->device->device_control(txchan, DMA_SLAVE_CONFIG, (unsigned long) &txconf);
		if(ret){
			printk ("%s: Error in device_control for tx:%d\n", __func__, ret);
			return ret;
		}

		memset(&dws->tx_sgl, 0, sizeof(dws->tx_sgl));
		dws->tx_sgl.dma_address = dws->tx_dma;
		dws->tx_sgl.length = dws->tx_len;

		txdesc = txchan->device->device_prep_slave_sg(txchan,
				&dws->tx_sgl,
				1,
				DMA_MEM_TO_DEV,
//				DMA_PREP_INTERRUPT | DMA_COMPL_SRC_UNMAP_SINGLE);
				DMA_PREP_INTERRUPT /*| DMA_COMPL_SKIP_DEST_UNMAP*/, 
#ifdef LINUX_3_2_DMA_DRIVER_PORTING_CHANGES
	/*
	 * The below changes done for 3.2 kernel. 
	 * It needs to be check it is required here or not
	 */
				); // FIXME: DMA_COMPL_SKIP_DEST_UNMAP 
#else
				NULL ); // FIXME: DMA_COMPL_SKIP_DEST_UNMAP and NULL as context. NULL parameter we added.
#endif
		if(!txdesc){
			printk ("%s: txdesc: Error in device_prep_slave_sg\n", __func__);
			return -1;
		}

                if(dws->rx_dma && dws->rx_len){
                        txdesc->callback = NULL;
                        txdesc->callback_param = dws;
                }else{
                        txdesc->callback = dw_spi_dma_done;
                        txdesc->callback_param = dws;
                }
	}

	if(dws->rx_dma && dws->rx_len){
#if SPDBG_MIN
		printk ("%s:%d: Prepare the RX dma transfer. Length=%d\n", \
				__func__, __LINE__, dws->rx_len);
#endif

		rxchan = dws->rxchan;
		/* 3. Prepare the RX dma transfer */
		rxconf.direction = DMA_DEV_TO_MEM;
		rxconf.src_addr = dws->dma_addr;
		rxconf.dst_addr = dws->rx_dma;
		rxconf.src_maxburst = DMA_CTL_SRC_MSIZE;
		rxconf.dst_maxburst = DMA_CTL_DEST_MSIZE;
		rxconf.dst_addr_width = 0;
		rxconf.src_addr_width = 0;
		rxconf.device_fc = true;

		rxchan->device->device_control(rxchan, DMA_SLAVE_CONFIG, (unsigned long) &rxconf);
		if(ret){
			printk ("%s: Error in device_control for rx:%d\n", __func__, ret);
			return ret;
		}

		memset(&dws->rx_sgl, 0, sizeof(dws->rx_sgl));
		dws->rx_sgl.dma_address = dws->rx_dma;
		dws->rx_sgl.length = dws->rx_len;

		rxdesc = rxchan->device->device_prep_slave_sg(rxchan,
				&dws->rx_sgl,
				1,
				DMA_DEV_TO_MEM,
				//DMA_PREP_INTERRUPT | DMA_COMPL_DEST_UNMAP_SINGLE);
				DMA_PREP_INTERRUPT /*| DMA_COMPL_SKIP_DEST_UNMAP*/,
#ifdef LINUX_3_2_DMA_DRIVER_PORTING_CHANGES
	/*
	 * The below changes done for 3.2 kernel. 
	 * It needs to be check it is required here or not
	 */
				); // FIXME: DMA_COMPL_SKIP_DEST_UNMAP 
#else
				NULL ); // FIXME: DMA_COMPL_SKIP_DEST_UNMAP and NULL as context. NULL parameter we added.
#endif
		if(!rxdesc){
			printk ("%s: rxdesc: Error in device_prep_slave_sg\n", __func__);
			return -1;
		}

		rxdesc->callback = dw_spi_dma_done;
		rxdesc->callback_param = dws;
	}

	if((dws->tx_dma && dws->tx_len)){
		txdesc->tx_submit(txdesc);
		while (readl(dws->regs + DWSPI_SR) & BUSY);
	}

	if(dws->rx_dma && dws->rx_len){
		rxdesc->tx_submit(rxdesc);
	}

	return 0;
}

static struct dw_spi_dma_ops spi_dma_ops = {
        .dma_init       = spi_dma_init,
        .dma_exit       = spi_dma_exit,
        .dma_transfer   = spi_dma_transfer,
};

int dw_spi_dma_init(struct designware_spi *dws)
{
	printk("%s:%d: initializing spi dma....", __func__, __LINE__);

        dws->dma_priv = kzalloc(sizeof(struct spi_dma), GFP_KERNEL);
        if (!dws->dma_priv)
                return -ENOMEM;

        dws->dma_ops = &spi_dma_ops;

	printk("done\n");

        return 0;
}
