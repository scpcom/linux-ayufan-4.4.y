/*
 * designware_spi.c
 *
 * Synopsys DesignWare AMBA SPI controller driver (master mode only)
 *
 * Author: Baruch Siach, Tk Open Systems
 *	baruch-NswTu9S1W3P6gbPvEgmw2w@...org
 *
 * Base on the Xilinx SPI controller driver by MontaVista
 *
 * 2002-2007 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 *
 * 2008, 2009 (c) Provigent Ltd.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/designware.h>

#include <linux/clk.h>
#include <linux/err.h>
#include <mach/reset.h>

#define DESIGNWARE_SPI_NAME "comcerto_spi"

/* Register definitions as per "DesignWare DW_apb_ssi Databook", Capture 6. */

#define DWSPI_CTRLR0        0x00
#define DWSPI_CTRLR1        0x04
#define DWSPI_SSIENR        0x08
#define DWSPI_MWCR          0x0C
#define DWSPI_SER           0x10
#define DWSPI_BAUDR         0x14
#define DWSPI_TXFTLR        0x18
#define DWSPI_RXFTLR        0x1C
#define DWSPI_TXFLR         0x20
#define DWSPI_RXFLR         0x24
#define DWSPI_SR            0x28
#define DWSPI_IMR           0x2C
#define DWSPI_ISR           0x30
#define DWSPI_RISR          0x34
#define DWSPI_TXOICR        0x38
#define DWSPI_RXOICR        0x3C
#define DWSPI_RXUICR        0x40
#define DWSPI_ICR           0x44
#define DWSPI_DMACR         0x4C
#define DWSPI_DMATDLR       0x50
#define DWSPI_DMARDLR       0x54
#define DWSPI_IDR           0x58
#define DWSPI_SSI_COMP_VERSION 0x5C
#define DWSPI_DR            0x60

#define DWSPI_CTRLR0_DFS_MASK	0x000f
#define DWSPI_CTRLR0_SCPOL	0x0080
#define DWSPI_CTRLR0_SCPH	0x0040
#define DWSPI_CTRLR0_TMOD_MASK	0x0300

#define DWSPI_SR_BUSY_MASK	0x01
#define DWSPI_SR_TFNF_MASK	0x02
#define DWSPI_SR_TFE_MASK	0x04
#define DWSPI_SR_RFNE_MASK	0x08
#define DWSPI_SR_RFF_MASK	0x10

#define DWSPI_ISR_TXEIS_MASK	0x01
#define DWSPI_ISR_RXFIS_MASK	0x10

#define DWSPI_IMR_TXEIM_MASK	0x01
#define DWSPI_IMR_RXFIM_MASK	0x10

static DEFINE_SPINLOCK(transfer_lock);

struct designware_spi {
	struct device *dev;
	struct workqueue_struct *workqueue;
	struct work_struct   work;

	struct tasklet_struct pump_transfers;

	struct mutex         lock; /* lock this struct except from queue */
	struct list_head     queue; /* spi_message queue */
	spinlock_t           qlock; /* lock the queue */

	void __iomem	*regs;	/* virt. address of the control registers */
	unsigned int ssi_clk;	/* clock in Hz */
	unsigned int tx_fifo_depth; /* bytes in TX FIFO */
	unsigned int rx_fifo_depth; /* bytes in RX FIFO */

	u32		irq;

	u8 bits_per_word;	/* current data frame size */
	struct spi_transfer *tx_t; /* current tx transfer */
	struct spi_transfer *rx_t; /* current rx transfer */
	struct spi_transfer *rxtx_t; /* current rx/tx transfer */
	const u8 *tx_ptr;       /* current tx buffer */
	u8 *rx_ptr;             /* current rx buffer */
	int remaining_tx_bytes;	/* bytes left to tx in the current transfer */
	int remaining_rx_bytes;	/* bytes left to rx in the current transfer */
	int status;             /* status of the current spi_transfer */

	struct completion done; /* signal the end of tx for current sequence */

	struct spi_device *spi; /* current spi slave device */
	struct clk	*clk_spi;
	struct list_head *transfers_list; /* head of the current sequence */
	unsigned int tx_count, rx_count; /* bytes in the current sequence */
};

static void dwspi_init_hw(struct designware_spi *dwspi)
{
	u16 ctrlr0;

	/* Disable the SPI master */
	writel(0, dwspi->regs + DWSPI_SSIENR);
	/* Disable all the interrupts just in case */
	writel(0, dwspi->regs + DWSPI_IMR);
	/* Set TX empty IRQ threshold */

//	writew(dwspi->tx_fifo_depth / 2, dwspi->regs + DWSPI_TXFTLR);
	printk(KERN_INFO "%s: TX FIFO level %d , RX FIFO level %d\n",__func__, readw(dwspi->regs + DWSPI_TXFTLR), readw(dwspi->regs + DWSPI_RXFTLR));

    /* Set transmit & receive mode */
	ctrlr0 = readw(dwspi->regs + DWSPI_CTRLR0);
    ctrlr0 &= ~DWSPI_CTRLR0_TMOD_MASK;
	writew(ctrlr0, dwspi->regs + DWSPI_CTRLR0);
}

static void dwspi_baudcfg(struct designware_spi *dwspi, u32 speed_hz)
{
	u16 div = (speed_hz) ? dwspi->ssi_clk/speed_hz : 0xffff;

	writew(div, dwspi->regs + DWSPI_BAUDR);
}

static void dwspi_enable(struct designware_spi *dwspi, int on)
{
	writel(on ? 1 : 0, dwspi->regs + DWSPI_SSIENR);
}

static void designware_spi_chipselect(struct spi_device *spi, int on)
{
	struct designware_spi *dwspi = spi_master_get_devdata(spi->master);
#if 0
	long gpio = (long) spi->controller_data;
	unsigned active = spi->mode & SPI_CS_HIGH;
#endif
	/*
	 * Note, the SPI controller must have been enabled at this point, i.e.
	 * SSIENR == 1
	 */

	if (on) {
	#if 0

		/* Turn the actual chip select on for GPIO chip selects */
		if (gpio >= 0)
			gpio_set_value(gpio, active);
	#endif
		/* Activate slave on the SPI controller */
		writel(1 << spi->chip_select, dwspi->regs + DWSPI_SER);
	} else {
		/* Deselect the slave on the SPI bus */
		writel(0, dwspi->regs + DWSPI_SER);
	#if 0
		if (gpio >= 0)
			gpio_set_value(gpio, !active);
	#endif
	}
}

static int designware_spi_setup_transfer(struct spi_device *spi,
		struct spi_transfer *t)
{
	u8 bits_per_word;
	u32 hz;
	struct designware_spi *dwspi = spi_master_get_devdata(spi->master);
	u16 ctrlr0 = readw(dwspi->regs + DWSPI_CTRLR0);

	bits_per_word = (t) ? t->bits_per_word : spi->bits_per_word;
	hz = (t) ? t->speed_hz : spi->max_speed_hz;

	if (bits_per_word < 4 || bits_per_word > 16) {
		dev_err(&spi->dev, "%s, unsupported bits_per_word=%d\n",
			__func__, bits_per_word);
		return -EINVAL;
	} else {
		ctrlr0 &= ~DWSPI_CTRLR0_DFS_MASK;
		ctrlr0 |= bits_per_word - 1;

		dwspi->bits_per_word = bits_per_word;
	}

	/* Set the SPI clock phase and polarity */
	if (spi->mode & SPI_CPHA)
		ctrlr0 |= DWSPI_CTRLR0_SCPH;
	else
		ctrlr0 &= ~DWSPI_CTRLR0_SCPH;
	if (spi->mode & SPI_CPOL)
		ctrlr0 |= DWSPI_CTRLR0_SCPOL;
	else
		ctrlr0 &= ~DWSPI_CTRLR0_SCPOL;

	writew(ctrlr0, dwspi->regs + DWSPI_CTRLR0);

	/* set speed */
	dwspi_baudcfg(dwspi, hz);

	return 0;
}

/* the spi->mode bits currently understood by this driver: */
#define MODEBITS (SPI_CPOL | SPI_CPHA)

static int designware_spi_setup(struct spi_device *spi)
{
	struct designware_spi *dwspi;
	int retval;

	dwspi = spi_master_get_devdata(spi->master);

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	if (spi->mode & ~MODEBITS) {
		dev_err(&spi->dev, "%s, SP unsupported mode bits %x\n",
			__func__, spi->mode & ~MODEBITS);
		return -EINVAL;
	}

	if (spi->chip_select > spi->master->num_chipselect) {
		dev_err(&spi->dev,
				"setup: invalid chipselect %u (%u defined)\n",
				spi->chip_select, spi->master->num_chipselect);
		return -EINVAL;
	}

	retval = designware_spi_setup_transfer(spi, NULL);
	if (retval < 0)
		return retval;

	dev_dbg(&spi->dev, "%s, mode %d, %u bits/w, %u nsec/bit\n",
		__func__, spi->mode & MODEBITS, spi->bits_per_word, 0);

	return 0;
}

static void designware_spi_do_tx(struct designware_spi *dwspi)
{
	u8 sr;
	int bytes_to_tx = dwspi->remaining_tx_bytes;
	u8 valid_tx_fifo_bytes = readb(dwspi->regs + DWSPI_TXFLR);
	u8 valid_rx_fifo_bytes = readb(dwspi->regs + DWSPI_RXFLR);
	int tx_limit = min(dwspi->tx_fifo_depth - valid_tx_fifo_bytes,\
			dwspi->rx_fifo_depth - valid_rx_fifo_bytes);

	/* Fill the Tx FIFO with as many bytes as possible */
	sr = readb(dwspi->regs + DWSPI_SR);
	while ((sr & DWSPI_SR_TFNF_MASK) && dwspi->remaining_tx_bytes > 0) {
		if (dwspi->bits_per_word <= 8) {
			u8 dr = (dwspi->tx_ptr) ? *dwspi->tx_ptr++ : 0;

			writeb(dr, dwspi->regs + DWSPI_DR);
			dwspi->remaining_tx_bytes--;
		} else {
			u16 dr = (dwspi->tx_ptr) ? *(u16 *) dwspi->tx_ptr : 0;

			dwspi->tx_ptr += 2;
			writew(dr, dwspi->regs + DWSPI_DR);
			dwspi->remaining_tx_bytes -= 2;
		}
		if (--tx_limit <= 0)
			break;
		sr = readb(dwspi->regs + DWSPI_SR);
	}

	dwspi->tx_count += bytes_to_tx - dwspi->remaining_tx_bytes;
}


static int designware_spi_tx_rx(struct designware_spi *dwspi)
{
	int i;
	u8 data;
	int len_start = 0;
	int total_len = 0;
	int tx_total_len = 0;
	int rx_total_len = 0;
	int tx_to_fill = 0;
	int list_first_node = 1;
	u8 sr;
	u8 dr;

	u8 *tx_ptr1 = NULL;
	u8 *tx_ptr2 = NULL;
	u8 *rx_ptr1 = NULL;
	u8 *rx_ptr2 = NULL;
	int tx_len1 = 0;
	int rx_len1 = 0;
	int tx_len2 = 0;
	int rx_len2 = 0;
	int rx_level = 0;
	int tx_level = 0;

	unsigned long flags;

	u8 valid_tx_fifo_bytes = readb(dwspi->regs + DWSPI_TXFLR);
	u8 valid_rx_fifo_bytes = readb(dwspi->regs + DWSPI_RXFLR);
	u16 tx_fifo_threshold = readw(dwspi->regs + DWSPI_TXFTLR);
	u16 rx_fifo_threshold = readw(dwspi->regs + DWSPI_RXFTLR);

//	printk(KERN_INFO "%s\n",__func__);

	if(valid_tx_fifo_bytes > 0 || valid_rx_fifo_bytes > 0)
		printk(KERN_ERR "%s: FIFO not empty: valid_tx_fifo_bytes %d valid_rx_fifo_bytes %d\n",valid_tx_fifo_bytes,valid_rx_fifo_bytes);


	list_for_each_entry_from(dwspi->rxtx_t, dwspi->transfers_list,
			transfer_list) {

		if(list_first_node)
		{
			if(dwspi->rxtx_t->tx_buf)
				tx_ptr1 = dwspi->rxtx_t->tx_buf;
			if(dwspi->rxtx_t->rx_buf)
				rx_ptr1 = dwspi->rxtx_t->rx_buf;
			total_len += dwspi->rxtx_t->len;
			tx_len1 = dwspi->rxtx_t->len;
			rx_len1 = dwspi->rxtx_t->len;

			list_first_node = 0;
//			printk(KERN_INFO "%s: len1 %d\n",__func__, dwspi->rxtx_t->len);
		}
		else
		{
			if(dwspi->rxtx_t->tx_buf)
				tx_ptr2 = dwspi->rxtx_t->tx_buf;
			if(dwspi->rxtx_t->rx_buf)
				rx_ptr2 = dwspi->rxtx_t->rx_buf;
			total_len += dwspi->rxtx_t->len;
			tx_len2 = dwspi->rxtx_t->len;
			rx_len2 = dwspi->rxtx_t->len;

//			printk(KERN_INFO "%s: len2 %d\n",__func__, dwspi->rxtx_t->len);
		}

	}

	tx_total_len =  rx_total_len = total_len;
	dwspi->tx_count = total_len;
	dwspi->status = 0;
//	printk(KERN_INFO "%s: total len %d\n",__func__, total_len);

//	sr = readb(dwspi->regs + DWSPI_SR);

	if(total_len > 8)
		len_start = 8;
	else
		len_start = total_len;

	spin_lock_irqsave(&transfer_lock, flags);

		//Fill Tx FIFO
		for(i = 0 ; i < len_start ; i++)
		{
			if(tx_len1)
			{
				dr = (tx_ptr1) ? *tx_ptr1++ : 0;
				tx_len1--;
			}
			else
			{
				dr = (tx_ptr2) ? *tx_ptr2++ : 0;
				tx_len2--;
			}
			writeb(dr, dwspi->regs + DWSPI_DR);
			tx_total_len--;
		}

//		printk(KERN_WARNING "%s: Tx FIFO now %d\n",__func__, readl(dwspi->regs + DWSPI_TXFLR));

		//Enable chip-select to start the transfer
		designware_spi_chipselect(dwspi->spi, 1);

		while(rx_total_len)
		{

//			printk(KERN_WARNING "rx_total_len = %d\n", rx_total_len);
			//If data in RX FIFO read it
			rx_level = readl(dwspi->regs + DWSPI_RXFLR);
			while(rx_level)
			{
				data = readb(dwspi->regs + DWSPI_DR);

				if(rx_len1)
				{
					if(rx_ptr1)
						*rx_ptr1++ = data;
					rx_len1--;
				}
				else
				{
					if(rx_ptr2)
						*rx_ptr2++ = data;
					rx_len2--;
				}

				rx_total_len--;
				tx_level = readl(dwspi->regs + DWSPI_TXFLR);
				rx_level = readl(dwspi->regs + DWSPI_RXFLR);
			
				//Tx FIFO going to get empty , break from here	
				if(tx_level < 4)
				{
				//	printk(KERN_WARNING "FIFO %d %d\n",tx_level,rx_level);
					break;
				}

			}

			//Check for TX FIFO level and if not full then make it full
			tx_level = readl(dwspi->regs + DWSPI_TXFLR);
			tx_to_fill = min((8 - tx_level), tx_total_len);

			for( i = 0 ; i < tx_to_fill; i++)
			{
				if(tx_len1)
				{
					dr = (tx_ptr1) ? *tx_ptr1++ : 0;
					tx_len1--;
				}
				else
				{
					if(tx_len2)
					{
						dr = (tx_ptr2) ? *tx_ptr2++ : 0;
						tx_len2--;
					}
					else
					{
						dr = 0;
					}
				}
			
				writeb(dr, dwspi->regs + DWSPI_DR);
				tx_total_len--;
			}


		}

		spin_unlock_irqrestore(&transfer_lock, flags);

		return 0;


#if 0
	sr = readb(dwspi->regs + DWSPI_SR);



	while ((sr & DWSPI_SR_TFNF_MASK) && dwspi->remaining_tx_bytes > 0) {

		u8 dr = (dwspi->tx_ptr) ? *dwspi->tx_ptr++ : 0;
		writeb(dr, dwspi->regs + DWSPI_DR);
		dwspi->remaining_tx_bytes--;
	}

	designware_spi_chipselect(dwspi->spi, 1);

	if (dwspi->remaining_tx_bytes > 0)

	{

		rx_level = readl(dwspi->regs + DWSPI_RXFLR);
		data = readb(dwspi->regs + DWSPI_DR);
		dwspi->remaining_rx_bytes--;
		if (dwspi->rx_ptr)
			*dwspi->rx_ptr++ = data;
	}

	
#endif
}

/* Return 1 when done, 0 otherwise */
static int designware_spi_fill_tx_fifo(struct designware_spi *dwspi)
{
	unsigned cs_change = 0;
	unsigned int ser;

	ser = readw(dwspi->regs + DWSPI_SER);

	list_for_each_entry_from(dwspi->tx_t, dwspi->transfers_list,
		transfer_list) {
		if (dwspi->remaining_tx_bytes == 0) {
			/* Initialize new spi_transfer */
			dwspi->tx_ptr = dwspi->tx_t->tx_buf;
			dwspi->remaining_tx_bytes = dwspi->tx_t->len;
			dwspi->status = 0;

			/* can't change speed or bits in the middle of a
			 * message. must disable the controller for this.
			 */
			if (dwspi->tx_t->speed_hz
					|| dwspi->tx_t->bits_per_word) {
				dwspi->status = -ENOPROTOOPT;
				break;
			}

			if (!dwspi->tx_t->tx_buf && !dwspi->tx_t->rx_buf
					&& dwspi->tx_t->len) {
				dwspi->status = -EINVAL;
				break;
			}

			if (cs_change)
				break;
		}

		designware_spi_do_tx(dwspi);

		/* Don't advance dwspi->tx_t, we'll get back to this
		 * spi_transfer later
		 */
		if (dwspi->remaining_tx_bytes > 0)
		{
			return 0;
		}

		cs_change = dwspi->tx_t->cs_change;
	}

	complete(&dwspi->done);
	return 1;
}

static void designware_spi_do_rx(struct designware_spi *dwspi)
{
	u8 sr;
	int bytes_to_rx = dwspi->remaining_rx_bytes;

	sr = readb(dwspi->regs + DWSPI_SR);

	if (sr & DWSPI_SR_RFF_MASK) {
	#if 0
		dev_err(dwspi->dev, "%s: RX FIFO overflow\n", __func__);
		dwspi->status = -EIO;
	#endif
	}

	/* Read as long as RX FIFO is not empty */
	while ((sr & DWSPI_SR_RFNE_MASK) != 0
			&& dwspi->remaining_rx_bytes > 0) {
		int rx_level = readl(dwspi->regs + DWSPI_RXFLR);

		while (rx_level-- && dwspi->remaining_rx_bytes > 0) {
			if (dwspi->bits_per_word <= 8) {
				u8 data;

				data = readb(dwspi->regs + DWSPI_DR);
				dwspi->remaining_rx_bytes--;
				if (dwspi->rx_ptr)
					*dwspi->rx_ptr++ = data;
			} else {
				u16 data;

				data = readw(dwspi->regs + DWSPI_DR);
				dwspi->remaining_rx_bytes -= 2;
				if (dwspi->rx_ptr) {
					*(u16 *) dwspi->rx_ptr = data;
					dwspi->rx_ptr += 2;
				}
			}
		}
		sr = readb(dwspi->regs + DWSPI_SR);
	}

	dwspi->rx_count += (bytes_to_rx - dwspi->remaining_rx_bytes);
}

/* return 1 if cs_change is true, 0 otherwise */
static int designware_spi_read_rx_fifo(struct designware_spi *dwspi)
{
	unsigned cs_change = 0;
	unsigned int ser;

	ser = readw(dwspi->regs + DWSPI_SER);

	list_for_each_entry_from(dwspi->rx_t, dwspi->transfers_list,
			transfer_list) {
		if (dwspi->remaining_rx_bytes == 0) {
			dwspi->rx_ptr = dwspi->rx_t->rx_buf;
			dwspi->remaining_rx_bytes = dwspi->rx_t->len;

			if (cs_change)
				return 1;
		}

		designware_spi_do_rx(dwspi);

		/* The rx buffer is filling up with more bytes. Don't advance
		 * dwspi->rx_t, as we have more bytes to read in this
		 * spi_transfer.
		 */
		if (dwspi->remaining_rx_bytes > 0)
			return 0;

		cs_change = dwspi->rx_t->cs_change;
	}

	return 0;
}

/* interate through the list of spi_transfer elements.
 * stop at the end of the list or when t->cs_change is true.
 */
static void designware_spi_do_transfers(struct designware_spi *dwspi)
{
	int tx_done, cs_change;
	int val;

	init_completion(&dwspi->done);

	/* transfer kickoff */
	tx_done = designware_spi_fill_tx_fifo(dwspi);
	designware_spi_chipselect(dwspi->spi, 1);

	if (!tx_done) {
		/* Enable the transmit empty interrupt, which we use to
		 * determine progress on the transmission in case we're
		 * not done yet.
		 */
		writeb(DWSPI_IMR_TXEIM_MASK, dwspi->regs + DWSPI_IMR);

		/* wait for tx completion */
		wait_for_completion(&dwspi->done);
	}

	/* This delay should be good enough for 100KHz spi transfers. Slower
	 * transfers may need a longer delay.
	 */
	udelay(10);

	/* get remaining rx bytes */
    do {
		cs_change = designware_spi_read_rx_fifo(dwspi);
	} while (readb(dwspi->regs + DWSPI_SR) &
			(DWSPI_SR_BUSY_MASK | DWSPI_SR_RFNE_MASK));

	/* transaction is done */
	designware_spi_chipselect(dwspi->spi, 0);

	if (dwspi->status < 0)
		return;

	if (!cs_change && (dwspi->remaining_rx_bytes > 0 ||
			dwspi->remaining_tx_bytes > 0)) {
	#if 0
		dev_err(dwspi->dev, "%s: remaining_rx_bytes = %d, "
				"remaining_tx_bytes = %d\n",
				__func__,  dwspi->remaining_rx_bytes,
				dwspi->remaining_tx_bytes);
	#endif
		dwspi->status = -EIO;
    }

	if (dwspi->rx_count != dwspi->tx_count) {
	#if 0
		dev_err(dwspi->dev, "%s: rx_count == %d, tx_count == %d\n",
				__func__, dwspi->rx_count, dwspi->tx_count);
	#endif
		dwspi->status = -EIO;
    }
}

static int designware_spi_transfer(struct spi_device *spi,
		struct spi_message *mesg)
{
	struct designware_spi *dwspi = spi_master_get_devdata(spi->master);

	mesg->actual_length = 0;
	mesg->status = -EINPROGRESS;

	/* we can't block here, so we use a spinlock
	 * here instead of the global mutex
	 */
	spin_lock(&dwspi->qlock);
	list_add_tail(&mesg->queue, &dwspi->queue);
	queue_work(dwspi->workqueue, &dwspi->work);
	spin_unlock(&dwspi->qlock);

	return 0;
}

static void designware_work(struct work_struct *work)
{
	struct designware_spi *dwspi = container_of(work,
			struct designware_spi, work);

	mutex_lock(&dwspi->lock);
	spin_lock(&dwspi->qlock);

	while (!list_empty(&dwspi->queue)) {
		struct spi_message *m;

		m = container_of(dwspi->queue.next, struct spi_message, queue);
		list_del_init(&m->queue);
		spin_unlock(&dwspi->qlock);

		dwspi->spi = m->spi;
		dwspi->tx_t = dwspi->rx_t = dwspi->rxtx_t =
			list_first_entry(&m->transfers, struct spi_transfer,
					transfer_list);

		/*
		 * Interate through groups of spi_transfer structs
		 * that are separated by cs_change being true
		 */
		dwspi->transfers_list = &m->transfers;
#if 0
		do {
			dwspi->remaining_tx_bytes =
				dwspi->remaining_rx_bytes = 0;
			dwspi->tx_count = dwspi->rx_count = 0;
			designware_spi_setup_transfer(m->spi, NULL);
			dwspi_enable(dwspi, 1);
			designware_spi_do_transfers(dwspi);
			dwspi_enable(dwspi, 0);
			if (dwspi->status < 0)
				break;
			m->actual_length +=
				dwspi->tx_count; /* same as rx_count */
		} while (&dwspi->tx_t->transfer_list != &m->transfers);
#else
			designware_spi_setup_transfer(m->spi, NULL);
			dwspi_enable(dwspi, 1);
			designware_spi_tx_rx(dwspi);
			dwspi_enable(dwspi, 0);
			m->actual_length = dwspi->tx_count; /* same as rx_count */

#endif
		m->status = dwspi->status;
		m->complete(m->context);

		spin_lock(&dwspi->qlock);
	}
	spin_unlock(&dwspi->qlock);
	mutex_unlock(&dwspi->lock);
}

static void designware_pump_transfers(unsigned long data)
{
	struct designware_spi *dwspi = (struct designware_spi *) data;

	designware_spi_read_rx_fifo(dwspi);
	if (!designware_spi_fill_tx_fifo(dwspi))
		/* reenable the interrupt */
		writeb(DWSPI_IMR_TXEIM_MASK, dwspi->regs + DWSPI_IMR);
}

static irqreturn_t designware_spi_irq(int irq, void *dev_id)
{
	struct designware_spi *dwspi = dev_id;

	tasklet_schedule(&dwspi->pump_transfers);
	/* disable the interrupt for now */
	writeb(0, dwspi->regs + DWSPI_IMR);

	return IRQ_HANDLED;
}

static void designware_spi_cleanup(struct spi_device *spi)
{
}

static int __init designware_spi_probe(struct platform_device *dev)
{
	int ret = 0;
	struct spi_master *master;
	struct designware_spi *dwspi;
	//struct designware_platform_data *pdata;
	struct spi_controller_pdata *pdata;
	struct resource *r;
	struct clk *clk_spi;

	pdata = dev->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&dev->dev, "no device data specified\n");
		return -EINVAL;
	}

        if(memcmp(pdata->clk_name, "DUS", 3))
                c2000_block_reset(COMPONENT_AXI_LEGACY_SPI, 0);
        else
                c2000_block_reset(COMPONENT_AXI_FAST_SPI, 0);

	clk_spi = clk_get(NULL,pdata->clk_name);
	if (IS_ERR(clk_spi)) {
		ret = PTR_ERR(clk_spi);
		pr_err("%s:Unable to obtain spi clock: %d\n",\
				__func__, ret);
		goto err_clk;
	}

	clk_enable(clk_spi);

	printk ("%s:Initializing SPI Controller : Using dma=%d CLK(%s)=%ld Hz\n", __func__, \
			pdata->use_dma, pdata->clk_name, clk_get_rate(clk_spi));

	/* Get resources(memory, IRQ) associated with the device */
	master = spi_alloc_master(&dev->dev, sizeof(struct designware_spi));
	if (master == NULL) {
		ret = -ENOMEM;
		goto err_nomem;
	}

	master->mode_bits = SPI_CPOL | SPI_CPHA;
	master->bus_num = pdata->bus_num;
	master->num_chipselect = pdata->num_chipselects;
	master->setup = designware_spi_setup;
	master->transfer = designware_spi_transfer;
	master->cleanup = designware_spi_cleanup;
	platform_set_drvdata(dev, master);

	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		ret = -ENODEV;
		goto put_master;
	}

	dwspi = spi_master_get_devdata(master);
	dwspi->clk_spi = clk_spi;
	dwspi->ssi_clk = clk_get_rate(dwspi->clk_spi);
	//dwspi->ssi_clk = pdata->max_freq;
	dwspi->tx_fifo_depth = TX_FIFO_DEPTH;
	dwspi->rx_fifo_depth = RX_FIFO_DEPTH;
#if 0
	dwspi->tx_fifo_depth = pdata->tx_fifo_depth;
	dwspi->rx_fifo_depth = pdata->rx_fifo_depth;
#endif
	dwspi->dev = &dev->dev;
	spin_lock_init(&dwspi->qlock);
	mutex_init(&dwspi->lock);
	INIT_LIST_HEAD(&dwspi->queue);
	INIT_WORK(&dwspi->work, designware_work);
	dwspi->workqueue =
		create_singlethread_workqueue(dev_name(master->dev.parent));
	
	if (dwspi->workqueue == NULL) {
		ret = -EBUSY;
		goto put_master;
	}
	tasklet_init(&dwspi->pump_transfers, designware_pump_transfers,
			(unsigned long) dwspi);

	if (!request_mem_region(r->start,
			r->end - r->start + 1, DESIGNWARE_SPI_NAME)) {
		ret = -ENXIO;
		goto destroy_wq;
	}

	dwspi->regs = ioremap(r->start, r->end - r->start + 1);
	if (dwspi->regs == NULL) {
		ret = -ENOMEM;
		goto destroy_wq;
	}

	dwspi->irq = platform_get_irq(dev, 0);
	if (dwspi->irq < 0) {
		ret = -ENXIO;
		goto unmap_io;
	}

	/* SPI controller initializations */
	dwspi_init_hw(dwspi);

	/* Register for SPI Interrupt */
	ret = request_irq(dwspi->irq, designware_spi_irq, 0,
			DESIGNWARE_SPI_NAME, dwspi);
	if (ret != 0)
		goto unmap_io;

	ret = spi_register_master(master);
    if (ret < 0)
		goto free_irq;

	dev_info(&dev->dev, "at 0x%08X mapped to 0x%08X, irq=%d\n",
			r->start, (u32)dwspi->regs, dwspi->irq);

	return ret;

free_irq:
	free_irq(dwspi->irq, dwspi);
unmap_io:
	iounmap(dwspi->regs);
destroy_wq:
	destroy_workqueue(dwspi->workqueue);
put_master:
	spi_master_put(master);
err_nomem:
	clk_disable(clk_spi);
	clk_put(clk_spi);
err_clk:
        if(memcmp(pdata->clk_name, "DUS", 3))
                c2000_block_reset(COMPONENT_AXI_LEGACY_SPI, 1);
        else
                c2000_block_reset(COMPONENT_AXI_FAST_SPI, 1);

	return ret;
}

static int __devexit designware_spi_remove(struct platform_device *dev)
{
	struct designware_spi *dwspi;
	struct spi_master *master;
	struct spi_controller_pdata *pdata;

	master = platform_get_drvdata(dev);
	dwspi = spi_master_get_devdata(master);

	free_irq(dwspi->irq, dwspi);
	iounmap(dwspi->regs);
	destroy_workqueue(dwspi->workqueue);
	tasklet_kill(&dwspi->pump_transfers);
	platform_set_drvdata(dev, 0);
	spi_master_put(master);
	clk_disable(dwspi->clk_spi);
	clk_put(dwspi->clk_spi);

        pdata = dev->dev.platform_data;
        if(!pdata)
        {
                return -EINVAL;
        }

        if(memcmp(pdata->clk_name, "DUS", 3))
                c2000_block_reset(COMPONENT_AXI_LEGACY_SPI, 1);
        else
                c2000_block_reset(COMPONENT_AXI_FAST_SPI, 1);

	return 0;
}

#if CONFIG_PM
static int designware_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct designware_spi *dwspi;
	struct spi_master *master;

	master = platform_get_drvdata(pdev);
	dwspi = spi_master_get_devdata(master);

        /* Disable the SPI master */
        writel(0, dwspi->regs + DWSPI_SSIENR);

        clk_disable(dwspi->clk_spi);

        return 0;

}

static int designware_spi_resume(struct platform_device *pdev)
{
	struct designware_spi *dwspi;
	struct spi_master *master;

	master = platform_get_drvdata(pdev);
	dwspi = spi_master_get_devdata(master);

        clk_enable(dwspi->clk_spi);
	dwspi_init_hw(dwspi);

        return 0;
}
#endif

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:" DESIGNWARE_SPI_NAME);

static struct platform_driver designware_spi_driver = {
	.probe  = designware_spi_probe,
	.remove	= __devexit_p(designware_spi_remove),
#if CONFIG_PM
        .suspend        = designware_spi_suspend,
        .resume         = designware_spi_resume,
#endif
	.driver = {
		.name = DESIGNWARE_SPI_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init designware_spi_init(void)
{
	return platform_driver_register(&designware_spi_driver);
}
module_init(designware_spi_init);

static void __exit designware_spi_exit(void)
{
	platform_driver_unregister(&designware_spi_driver);
}
module_exit(designware_spi_exit);

MODULE_AUTHOR("Baruch Siach <baruch-NswTu9S1W3P6gbPvEgmw2w@...org>");
MODULE_DESCRIPTION("Synopsys DesignWare SPI driver");
MODULE_LICENSE("GPL");
