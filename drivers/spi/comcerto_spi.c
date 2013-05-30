/*
 * Mindspeed SPI controller driver (master mode only)
 *
 * Author: Satendra Pratap
 *	satendra.pratap@mindspeed.com
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>

#include <linux/spi/comcerto_spi.h>

#define SPI_VERSION             "0.0.1"

#define spi_err(dev, fmt, args...)      dev_err(dev, "%s: " fmt, __FUNCTION__, ##args)
#define spi_dbg(dev, fmt, args...)      dev_dbg(dev, "%s: " fmt, __FUNCTION__, ##args)
#define spi_warn(dev, fmt, args...)     dev_warn(dev, "%s: " fmt, __FUNCTION__, ##args)
         
struct comcerto_spi {
	/* bitbang has to be first */
	struct spi_bitbang bitbang;
    struct platform_device *dev;
	struct completion done;
	struct resource mem; /* phys mem */
	void __iomem	*regs;	/* virt. address of the control registers */

	u32		irq;

	u8 *rx_ptr;		/* pointer in the Tx buffer */
	const u8 *tx_ptr;	/* pointer in the Rx buffer */
	int remaining_bytes;	/* the number of bytes left to transfer */
	u8 bits_per_word;

    int (*do_read8)(void __iomem *, u8 *buf, unsigned int *len);
    int (*do_write8)(void __iomem *, u8 *buf, unsigned int *len);

    int (*do_read16)(void __iomem *, u16 *buf, unsigned int *len);
    int (*do_write16)(void __iomem *, u16 *buf, unsigned int *len);

    int (*do_write_read)(void __iomem *, u8 fs, u8 *wbuf, unsigned int *wlen, u8 *rbuf, unsigned int *rlen);
};

static int do_write_read_transfer(void __iomem *reg_base, u8 fs, u8 *wbuf, unsigned int *wlen, u8 *rbuf, unsigned int *rlen)
{
    u32 sr, dr;
    unsigned int wlen_now = 0, rlen_now = 0;
    int rc = 0;

    while (wlen_now < *wlen) {
        sr = __raw_readl(reg_base + COMCERTO_SPI_SR);

        if (sr & TFNF) {
            if (wlen_now < *wlen) {
                __raw_writew(cpu_to_le16((u16) *wbuf), reg_base + COMCERTO_SPI_DR);
                wbuf++;
                wlen_now++;
            }
        }
    }

    while (rlen_now < *rlen) {
        sr = __raw_readl(reg_base + COMCERTO_SPI_SR);

        if (sr & (RFF | DCOL)) {
            rc = -1;
            goto out;
        }

        if (sr & RFNE) {
            dr = __raw_readw(reg_base + COMCERTO_SPI_DR);
            if (rlen_now < *rlen) {
                *rbuf = (u8) (le16_to_cpu(dr) & 0xff);
                rbuf++;
                rlen_now++;
            } else {
                rc = -1;
                goto out;
            }
        }
    }

out:
    *rlen = rlen_now;
    *wlen = wlen_now;

    return rc;
}

/**
 *  * do_write_only_transfer8 -
 *   *
 *    *
 *     */
static int do_write_only_transfer8(void __iomem *reg_base, u8 *buf, unsigned int *len)
{
    unsigned int len_now;
    int rc = 0;
    unsigned int tmp = *len;
    u32 dr = (u32)reg_base + COMCERTO_SPI_DR;
    u32 txflr = (u32)reg_base + COMCERTO_SPI_TXFLR;

    while (tmp)
    {
        len_now = 8 - __raw_readl(txflr);
        if (len_now > tmp)
            len_now = tmp;

        tmp -= len_now;

        /* warm-up
         * write
         * fifo
         * to
         * avoid
         * underruns
         * */
        while (len_now--)
            __raw_writew(cpu_to_le16((u16) *buf++), dr);
    }

    *len -= tmp;

    return rc;
}

/**
 *  * do_write_only_transfer -
 *   *
 *    *
 *     */

static int do_write_only_transfer16(void __iomem *reg_base, u16 *buf, unsigned int *len)
{
    unsigned int len_now;
    int rc = 0;
    unsigned int tmp = *len;
    u32 dr = (u32)reg_base + COMCERTO_SPI_DR;
    u32 txflr = (u32)reg_base + COMCERTO_SPI_TXFLR;

    while (tmp)
    {
        len_now = 8 - __raw_readl(txflr);
        if (len_now > tmp)
            len_now = tmp;

        tmp -= len_now;

        /* warm-up
         * write
         * fifo
         * to
         * avoid
         * underruns
         * */
        while (len_now--)
            __raw_writew(cpu_to_le16(*buf++), dr);
    }

    *len -= tmp;

    return rc;
}


/**
 *  * do_read_only_transfer -
 *   *
 *    *
 *     */
static int do_read_only_transfer8(void __iomem *reg_base, u8 *buf, unsigned int *len)
{
    unsigned int len_now;
    int rc = 0;
    unsigned int tmp = *len;
    u32 dr = (u32)reg_base + COMCERTO_SPI_DR;
    u32 rxflr = (u32)reg_base + COMCERTO_SPI_RXFLR;

    /* start the serial clock */
    __raw_writew(0, dr);

    while (tmp)
    {
        len_now = __raw_readl(rxflr);
        if (len_now > tmp)
            len_now = tmp;

        tmp -= len_now;

        while (len_now--) {
            *buf = (u8) (le16_to_cpu(__raw_readw(dr)) & 0xff);
            buf++;
        }
    }

    *len -= tmp;

    return rc;
}

/**
 *  * do_read_only_transfer -
 *   *
 *    *
 *     */
static int do_read_only_transfer16(void __iomem *reg_base, u16 *buf, unsigned int *len)
{
    unsigned int len_now;
    int rc = 0;
    unsigned int tmp = *len;
    u32 dr = (u32)reg_base + COMCERTO_SPI_DR;
    u32 rxflr = (u32)reg_base + COMCERTO_SPI_RXFLR;

    /* start the serial clock */
    __raw_writew(0, dr);

    while (tmp)
    {
        len_now = __raw_readl(rxflr);
        if (len_now > tmp)
            len_now = tmp;

        tmp -= len_now;

        while (len_now--) {
            *buf = le16_to_cpu(__raw_readw(dr));
            buf++;
        }
    }

    *len -= tmp;

    return rc;
}



int comcerto_spi_do_transfer(struct comcerto_spi *xspi, struct spi_client_conf *cconf, struct comcerto_transfer *transfer)
{
    u32 ctrlr0, ctrlr1, baudr = 0, ser = 0;
    int rc;
    void __iomem *reg_base = xspi->regs;

    baudr = transfer->baudr;
    ser = transfer->ser;
    /* make sure last transaction is finished */
    while (__raw_readl(reg_base + COMCERTO_SPI_SR) & BUSY) ;

    ctrlr0 = transfer->ctrlr0;

    __raw_writel(0, reg_base + COMCERTO_SPI_SSIENR);

    switch (transfer->mode & 0x0f) {
        default:
            rc = -1;
            break;

        case SPI_TRANSFER_MODE_WRITE_ONLY:
            ctrlr0 |= (0x0001 << 8);

            __raw_writel(ctrlr0, reg_base + COMCERTO_SPI_CTRLR0);
            __raw_writel(baudr, reg_base + COMCERTO_SPI_BAUDR);
            __raw_writel(ser, reg_base + COMCERTO_SPI_SER);
            __raw_writel(8, reg_base + COMCERTO_SPI_RXFTLR);
            __raw_writel(0, reg_base + COMCERTO_SPI_TXFTLR);
            __raw_writel(0, reg_base + COMCERTO_SPI_IMR);
            __raw_writel(1, reg_base + COMCERTO_SPI_SSIENR);

            if (transfer->fs <= 8)
                rc = xspi->do_write8(reg_base, transfer->wbuf, &transfer->wlen);
            else
                rc = xspi->do_write16(reg_base, (u16 *) transfer->wbuf, &transfer->wlen);

            break;

        case SPI_TRANSFER_MODE_READ_ONLY:
            ctrlr0 |= (0x0002 << 8);
            ctrlr1 = transfer->rlen - 1;

            __raw_writel(ctrlr0, reg_base + COMCERTO_SPI_CTRLR0);

            __raw_writel(ctrlr1, reg_base + COMCERTO_SPI_CTRLR1);
            __raw_writel(baudr, reg_base + COMCERTO_SPI_BAUDR);
            __raw_writel(ser, reg_base + COMCERTO_SPI_SER);
            __raw_writel(8, reg_base + COMCERTO_SPI_RXFTLR);
            __raw_writel(0, reg_base + COMCERTO_SPI_TXFTLR);
            __raw_writel(0, reg_base + COMCERTO_SPI_IMR);
            __raw_writel(1, reg_base + COMCERTO_SPI_SSIENR);

            if (transfer->fs <= 8)
                rc = xspi->do_read8(reg_base, transfer->rbuf, &transfer->rlen);
            else
                rc = xspi->do_read16(reg_base, (u16 *) transfer->rbuf, &transfer->rlen);

            break;

        case SPI_TRANSFER_MODE_WRITE_READ:
            ctrlr0 |= (0x0000 << 8);

            __raw_writel(ctrlr0, reg_base + COMCERTO_SPI_CTRLR0);
            __raw_writel(baudr, reg_base + COMCERTO_SPI_BAUDR);
            __raw_writel(ser, reg_base + COMCERTO_SPI_SER);
            __raw_writel(8, reg_base + COMCERTO_SPI_RXFTLR);
            __raw_writel(0, reg_base + COMCERTO_SPI_TXFTLR);
            __raw_writel(0, reg_base + COMCERTO_SPI_IMR);
            __raw_writel(1, reg_base + COMCERTO_SPI_SSIENR);

            rc = xspi->do_write_read(reg_base, transfer->fs, transfer->wbuf, &transfer->wlen, transfer->rbuf, &transfer->rlen);

            break;
    }

    return rc;
}

static int comcerto_spi_setup(struct spi_device *spi)
{
    int rc = 0;

    spi_dbg(&spi->dev, "bits per word %u, max speed %uHz, mode %#x\n",
            spi->bits_per_word, spi->max_speed_hz, spi->mode);

    if (!spi->bits_per_word)
        spi->bits_per_word = 8;

    if (spi->bits_per_word < SPI_FRAME_SIZE_MIN || spi->bits_per_word > SPI_FRAME_SIZE_MAX) {
        spi_err(&spi->dev, "bits per word (frame size) %u out of range %u..%u\n",
                spi->max_speed_hz, SPI_FRAME_SIZE_MIN, SPI_FRAME_SIZE_MAX);
        rc = -EINVAL;
        goto err;
    }

    if (spi->max_speed_hz < SPI_SPEED_MIN) {
        spi_err(&spi->dev, "such low speed %u isn't supported, min is %u\n",
                spi->max_speed_hz, SPI_SPEED_MIN);
        rc = -EINVAL;
        goto err;
    }

    if (spi->max_speed_hz > SPI_SPEED_MAX) {
        spi_warn(&spi->dev, "decreasing speed %u to max supported %u\n",
                spi->max_speed_hz, SPI_SPEED_MAX);
        spi->max_speed_hz = SPI_SPEED_MAX;
    }

    if (spi->chip_select > SPI_CHIP_SELECT_MAX) {
        spi_err(&spi->dev, "chip select %u out of range 0..%u\n",
                spi->max_speed_hz, SPI_CHIP_SELECT_MAX);
        rc = -EINVAL;
        goto err;
    }

    if (spi->mode & SPI_CS_HIGH) {
        spi_err(&spi->dev, "chip select active high isn't supported\n");
        rc = -EINVAL;
        goto err;
    }

    if (spi->mode & SPI_LSB_FIRST) {
        spi_err(&spi->dev, "LSB first mode isn't supported\n");
        rc = -EINVAL;
        goto err;
    }

err:
    return rc;
	return 0;
}

static void spi_set_control (struct comcerto_spi *xspi, struct spi_client_conf *cconf, struct comcerto_transfer *ct)
{
    u32 ctrlr0;
    struct spi_master *master;
    struct comcerto_spi_platform *pdata = (struct comcerto_spi_platform *)xspi->dev->dev.platform_data;
    struct spi_client_conf *scc = (struct spi_client_conf *)pdata->devices->platform_data;

    master = xspi->bitbang.master;

    cconf->cs_msk = scc->cs_msk;
    cconf->sc_polarity = scc->sc_polarity;
    cconf->sc_phase = scc->sc_phase;
    cconf->sc_rate = scc->sc_rate;
    cconf->cs_delay = scc->cs_delay;

    ctrlr0 = ((cconf->sc_polarity & 0x1) << 7) | ((cconf->sc_phase & 0x1) << 6) | (((ct->fs - 1) & 0xf) << 0);

    ct->ctrlr0 = ctrlr0;
    ct->mode = master->mode_bits;

    /* FIXME: this will be fixed after clock PLL implementation done.*/
    ct->baudr = pdata->clock_rate / cconf->sc_rate;
    ct->ser = cconf->cs_msk & pdata->cs_msk;
}

int comcerto_spi_transfer(struct spi_device *spi,
                        struct spi_message *mesg)
{
    struct spi_transfer     *t = NULL;
    struct comcerto_transfer     ct;
    struct spi_client_conf cconf;
    int ret = 0;
    struct comcerto_spi *xspi = spi_master_get_devdata(spi->master);

    ct.fs = 8;

    spi_set_control(xspi, &cconf, &ct);

    list_for_each_entry (t, &mesg->transfers, transfer_list) {
        ct.wbuf = (u8 *)t->tx_buf;
        ct.rbuf = (u8 *)t->rx_buf;
        ct.wlen = t->len;
        ct.rlen = t->len;

        ret = comcerto_spi_do_transfer(xspi, &cconf, &ct);

        if (ret == -1)
            return -1;
    }

    return 0;
}

static void comcerto_spi_hw_init(struct comcerto_spi *xspi)
{
    void __iomem *reg_base = xspi->regs;

    /* enable SPI bus : FIXME */
    //comcerto_gpio_ctrl(0x1 << 9, 0x1 << 9);


    /* disable SPI operation */
    writel(0, reg_base + COMCERTO_SPI_SSIENR);

    /* mask all SPI irq's */
    writel(0, reg_base + COMCERTO_SPI_IMR);
}

struct spi_master *comcerto_spi_init(struct platform_device *pdev, struct resource *mem,
	u32 irq, s16 bus_num, int num_cs)
{
    struct device *dev = &pdev->dev;
	struct spi_master *master;
	struct comcerto_spi *xspi;

	master = spi_alloc_master(dev, sizeof(struct comcerto_spi));
	if (!master)
		return NULL;

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA;

	xspi = spi_master_get_devdata(master);
    //memcpy(&(xspi->dev), pdev, sizeof(struct platform_device));
    xspi->dev = pdev;
	xspi->bitbang.master = spi_master_get(master);
	xspi->bitbang.master->setup = comcerto_spi_setup;
	xspi->bitbang.master->transfer = comcerto_spi_transfer;
	xspi->bitbang.master->cleanup = NULL;

	init_completion(&xspi->done);

	if (!request_mem_region(mem->start, resource_size(mem),
		comcerto_SPI_NAME))
		goto put_master;

	xspi->regs = ioremap(mem->start, resource_size(mem));
	if (xspi->regs == NULL) {
		dev_warn(dev, "ioremap failure\n");
		goto map_failed;
	}

	master->bus_num = bus_num;
	master->num_chipselect = num_cs;
	master->dev.of_node = dev->of_node;

	xspi->mem = *mem;
	xspi->irq = irq;

	xspi->do_read8 = do_read_only_transfer8;
	xspi->do_write8 = do_write_only_transfer8;

	xspi->do_read16 = do_read_only_transfer16;
	xspi->do_write16 = do_write_only_transfer16;

    xspi->do_write_read = do_write_read_transfer;

	/* SPI controller initializations */
	comcerto_spi_hw_init(xspi);

	/* Register for SPI Interrupt : FIXME*/
    /*
	ret = request_irq(xspi->irq, comcerto_spi_irq, 0, comcerto_SPI_NAME, xspi);
	if (ret)
		goto unmap_io;
    

	ret = spi_bitbang_start(&xspi->bitbang);
	if (ret) {
		dev_err(dev, "spi_bitbang_start FAILED\n");
		goto free_irq;
	}
    */
	dev_info(dev, "at 0x%08llX mapped to 0x%p, irq=%d\n",
		(unsigned long long)mem->start, xspi->regs, xspi->irq);
	return master;

/* FIXME */
//free_irq:
	//free_irq(xspi->irq, xspi);
	//iounmap(xspi->regs);
map_failed:
	release_mem_region(mem->start, resource_size(mem));
put_master:
	spi_master_put(master);
	return NULL;
}
EXPORT_SYMBOL(comcerto_spi_init);

void comcerto_spi_deinit(struct spi_master *master)
{
	struct comcerto_spi *xspi;

	xspi = spi_master_get_devdata(master);

	//FIXME spi_bitbang_stop(&xspi->bitbang);
	//free_irq(xspi->irq, xspi);
	iounmap(xspi->regs);

	release_mem_region(xspi->mem.start, resource_size(&xspi->mem));
	spi_master_put(xspi->bitbang.master);

     spi_unregister_master(master);
}
EXPORT_SYMBOL(comcerto_spi_deinit);

static int __devinit comcerto_spi_probe(struct platform_device *dev)
{
	struct comcerto_spi_platform *pdata;
	struct resource *r;
	int irq, num_cs = 0;
	struct spi_master *master;
    int rc = -EINVAL;
    int i;

	pdata = (struct comcerto_spi_platform *)dev->dev.platform_data;

	if (pdata) {
		num_cs = pdata->num_chipselect;
	}

	if (!num_cs) {
		dev_err(&dev->dev, "Missing slave select configuration data\n");
		return -EINVAL;
	}


	r = platform_get_resource(dev, IORESOURCE_MEM, dev->num_resources);
	if (!r)
		return -ENODEV;

	irq = platform_get_irq(dev, 0);
	if (irq < 0)
		return -ENXIO;

	master = comcerto_spi_init(dev, r, irq, dev->id, num_cs);

	if (!master)
		return -ENODEV;

	if (pdata) {
		for (i = 0; i < pdata->num_devices; i++)
			spi_new_device(master, pdata->devices + i);
	}

	platform_set_drvdata(dev, master);

    rc = spi_register_master(master);

    if (rc != 0) {
        spi_err(&dev->dev, "error registering SPI master\n");
        goto err1;
    }

	return 0;

err1:
    spi_master_put(master);

    return rc;
}

static int __devexit comcerto_spi_remove(struct platform_device *dev)
{
	comcerto_spi_deinit(platform_get_drvdata(dev));

	platform_set_drvdata(dev, 0);

	return 0;
}

static struct platform_driver comcerto_spi_driver = {
	.probe = comcerto_spi_probe,
	.remove = __devexit_p(comcerto_spi_remove),
	.driver = {
		.name = comcerto_SPI_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init comcerto_spi_pltfm_init(void)
{
    printk(KERN_INFO "c2k-spi: %s: loaded version %s\n", __FUNCTION__, SPI_VERSION);

	if(platform_driver_register(&comcerto_spi_driver)) {
        printk(KERN_ERR "c2k-spi: %s: error registering driver\n", __FUNCTION__);
        goto err0;
    }

    return 0;

err0:
    return -1;
}
module_init(comcerto_spi_pltfm_init);

static void __exit comcerto_spi_pltfm_exit(void)
{
	platform_driver_unregister(&comcerto_spi_driver);
}
module_exit(comcerto_spi_pltfm_exit);

MODULE_AUTHOR("Satendra Pratap");
MODULE_DESCRIPTION("Mindspeed SPI driver");
MODULE_LICENSE("GPL");
