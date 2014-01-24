/*
 *  drivers/spi2/busses/comcerto_spi.c
 *
 *  Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/version.h>
#if !defined (AUTOCONF_INCLUDED)
#if 0
	#include <linux/config.h>
#endif
#endif
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <mach/irqs.h>
#include <linux/delay.h>

#include <linux/platform_device.h>
#include "comcerto_spi.h"

/**/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/slab.h>
/**/

/**
 * do_write_read_transfer8 -
 *
 *
 */
static int do_write_read_transfer8(struct comcerto_spi *spi, u8 *wbuf, unsigned int *wlen, u8 *rbuf, unsigned int *rlen, u32 ser_reg)
{
	unsigned int len_now;
	int rc = 0;
	unsigned int wtmp = *wlen, rtmp = *rlen;
	u32 dr = spi->membase + COMCERTO_SPI_DR;
	u32 txflr = spi->membase + COMCERTO_SPI_TXFLR;
	u32 rxflr = spi->membase + COMCERTO_SPI_RXFLR;
	int ser_done = 0;

//	printk(KERN_INFO "do_write_read_transfer(%#lx, %#lx, %d, %#lx, %d)\n", (unsigned long)spi,
//                                                                      (unsigned long)wbuf, *wlen,
//                                                                      (unsigned long)rbuf, *rlen);

	while (wtmp || rtmp) {
		len_now = 8 - __raw_readl(txflr);
		if (len_now > wtmp)
			len_now = wtmp;

		wtmp -= len_now;

		/* warm-up write fifo to avoid underruns */
		while (len_now--)
			__raw_writew(cpu_to_le16((u16) *wbuf++), dr);

		if (!ser_done){
			__raw_writel(ser_reg, spi->membase + COMCERTO_SPI_SER);
			ser_done = 1;
		}

		len_now = __raw_readl(rxflr);
		if (len_now > rtmp)
			len_now = rtmp;

		rtmp -= len_now;

		while (len_now--) {
			*rbuf = (u8) (le16_to_cpu(__raw_readw(dr)) & 0xff);
			rbuf++;
		}
	}

	*rlen -= rtmp;
	*wlen -= wtmp;

	return rc;
}

/**
 * do_write_read_transfer16 -
 *
 */
static int do_write_read_transfer16(struct comcerto_spi *spi, u16 *wbuf, unsigned int *wlen, u16 *rbuf, unsigned int *rlen, u32 ser_reg)
{
	unsigned int len_now;
	int rc = 0;
	unsigned int wtmp = *wlen, rtmp = *rlen;
	unsigned int wpadding, rpadding;
	u32 dr = spi->membase + COMCERTO_SPI_DR;
	u32 txflr = spi->membase + COMCERTO_SPI_TXFLR;
	u32 rxflr = spi->membase + COMCERTO_SPI_RXFLR;
	int ser_done = 0;

//	printk(KERN_INFO "do_write_read_transfer(%#lx, %#lx, %d, %#lx, %d)\n", (unsigned long)spi
//                                                                      (unsigned long)wbuf, *wlen,
//                                                                      (unsigned long)rbuf, *rlen);

	if (wtmp > rtmp) {
		wpadding  = 0;
		rpadding = wtmp - rtmp;
	} else {
		wpadding = rtmp - wtmp;
		rpadding = 0;
	}

	while (wtmp || rtmp) {
		len_now = 8 - __raw_readl(txflr);

		if (wtmp) {
			if (len_now > wtmp)
				len_now = wtmp;

			wtmp -= len_now;

			while (len_now--)
				__raw_writew(cpu_to_le16(*wbuf++), dr);

		} else if (wpadding) {
			if (len_now > wpadding)
				len_now = wpadding;

			wpadding -= len_now;

			while (len_now--)
				__raw_writew(0, dr);
		}

		if (!ser_done){
			__raw_writel(ser_reg, spi->membase + COMCERTO_SPI_SER);
			ser_done = 1;
		}

		len_now = __raw_readl(rxflr);
		if (rtmp) {
			if (len_now > rtmp)
				len_now = rtmp;

			rtmp -= len_now;

			while (len_now--) {
				*rbuf = le16_to_cpu(__raw_readw(dr));
				rbuf++;
			}
		} else if (rpadding) {
			if (len_now > rpadding)
				len_now = rpadding;

			rpadding -= len_now;

			while (len_now--)
				__raw_readw(dr);
		}
	}

	*rlen -= rtmp;
	*wlen -= wtmp;

	return rc;
}


/**
 * do_write_only_transfer8 -
 *
 *
 */
static int do_write_only_transfer8(struct comcerto_spi *spi, u8 *buf, unsigned int *len, u32 ser_reg)
{
	unsigned int len_now;
	int rc = 0;
	unsigned int tmp = *len;
	u32 dr = spi->membase + COMCERTO_SPI_DR;
	u32 txflr = spi->membase + COMCERTO_SPI_TXFLR;
	int ser_done = 0;

//	printk(KERN_INFO "do_write_only_transfer8(%#lx, %#lx, %d)\n", (unsigned long)spi, (unsigned long)buf, *len);

	while (tmp) {
		len_now = 8 - __raw_readl(txflr);
		if (len_now > tmp)
			len_now = tmp;

		tmp -= len_now;

		while (len_now--)
			__raw_writew(cpu_to_le16((u16) *buf++), dr);

		if (!ser_done)
		{
			__raw_writel(ser_reg, spi->membase + COMCERTO_SPI_SER);
			ser_done = 1;
		}
	}

	*len -= tmp;

//      printk(KERN_INFO "exit do_write_only_transfer(%d, %d)\n", *len, rc);

	return rc;
}

/**
 * do_write_only_transfer -
 *
 *
 */
static int do_write_only_transfer16(struct comcerto_spi *spi, u16 *buf, unsigned int *len, u32 ser_reg)
{
	unsigned int len_now;
	int rc = 0;
	unsigned int tmp = *len;
	u32 dr = spi->membase + COMCERTO_SPI_DR;
	u32 txflr = spi->membase + COMCERTO_SPI_TXFLR;
	int ser_done = 0;

//      printk(KERN_INFO "do_write_only_transfer(%#lx, %#lx, %d)\n", (unsigned long)spi, (unsigned long)buf, *len);

	while (tmp) {
		len_now = 8 - __raw_readl(txflr);
		if (len_now > tmp)
			len_now = tmp;

		tmp -= len_now;

		while (len_now--)
			__raw_writew(cpu_to_le16(*buf++), dr);

		if (!ser_done)
		{
			__raw_writel(ser_reg, spi->membase + COMCERTO_SPI_SER);
			ser_done = 1;
		}
	}

	*len -= tmp;

//      printk(KERN_INFO "exit do_write_only_transfer(%d, %d)\n", *len, rc);

	return rc;
}


/**
 * do_read_only_transfer -
 *
 *
 */
static int do_read_only_transfer8(struct comcerto_spi *spi, u8 *buf, unsigned int *len)
{
	unsigned int len_now;
	int rc = 0;
	unsigned int tmp = *len;
	u32 dr = spi->membase + COMCERTO_SPI_DR;
	u32 rxflr = spi->membase + COMCERTO_SPI_RXFLR;

//	printk(KERN_INFO "do_read_only_transfer8(%#lx, %#lx, %d)\n", (unsigned long)spi, (unsigned long)buf, *len);

	/* start the serial clock */
	__raw_writew(0, dr);

	while (tmp) {
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
 * do_read_only_transfer -
 *
 *
 */
static int do_read_only_transfer16(struct comcerto_spi *spi, u16 *buf, unsigned int *len)
{
	unsigned int len_now;
	int rc = 0;
	unsigned int tmp = *len;
	u32 dr = spi->membase + COMCERTO_SPI_DR;
	u32 rxflr = spi->membase + COMCERTO_SPI_RXFLR;

//      printk(KERN_INFO "do_read_only_transfer(%#lx, %#lx, %d)\n", (unsigned long)spi, (unsigned long)buf, *len);

	/* start the serial clock */
	__raw_writew(0, dr);

	while (tmp) {
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


/**
 * comcerto_spi_do_transfer -
 *
 *
 */
static int comcerto_spi_do_transfer(struct spi_adapter *adapter, struct spi_transfer *transfer, struct spi_client_config *config)
{
	struct comcerto_spi *spi = (struct comcerto_spi *)adapter->data;
	u32 ctrlr0, ctrlr1, baudr, ser;
	int rc;

//      printk(KERN_INFO "comcerto_spi_do_transfer(%#lx, %#lx, %#lx)\n", (unsigned long) adapter, (unsigned long) transfer, (unsigned long) config);

	/* make sure last transaction is finished */
	while (__raw_readl(spi->membase + COMCERTO_SPI_SR) & BUSY) ;

	if (config->ba_delay)
		udelay(config->ba_delay);

	ctrlr0 = ((config->sc_polarity & 0x1) << 7) | ((config->sc_phase & 0x1) << 6) | (((transfer->fs - 1) & 0xf) << 0);

	baudr = spi->clock_rate / config->sc_rate;

	ser = config->cs_msk & adapter->caps.cs_msk;

	__raw_writel(0, spi->membase + COMCERTO_SPI_SSIENR);

	switch (transfer->mode & 0x0f) {
	default:
		rc = -1;
		break;

	case SPI_TRANSFER_MODE_WRITE_ONLY:
		ctrlr0 |= (0x0001 << 8);

		__raw_writel(ctrlr0, spi->membase + COMCERTO_SPI_CTRLR0);
		__raw_writel(baudr, spi->membase + COMCERTO_SPI_BAUDR);
		//__raw_writel(ser, spi->membase + COMCERTO_SPI_SER);
		__raw_writel(8, spi->membase + COMCERTO_SPI_RXFTLR);
		__raw_writel(0, spi->membase + COMCERTO_SPI_TXFTLR);
		__raw_writel(0, spi->membase + COMCERTO_SPI_IMR);
		__raw_writel(1, spi->membase + COMCERTO_SPI_SSIENR);

		if (transfer->fs <= 8)
			rc = do_write_only_transfer8(spi, transfer->wbuf, &transfer->wlen, ser);
		else
			rc = do_write_only_transfer16(spi, (u16 *) transfer->wbuf, &transfer->wlen, ser);

		break;

	case SPI_TRANSFER_MODE_READ_ONLY:
		ctrlr0 |= (0x0002 << 8);
		ctrlr1 = transfer->rlen - 1;

		__raw_writel(ctrlr0, spi->membase + COMCERTO_SPI_CTRLR0);
		__raw_writel(ctrlr1, spi->membase + COMCERTO_SPI_CTRLR1);
		__raw_writel(baudr, spi->membase + COMCERTO_SPI_BAUDR);
		__raw_writel(ser, spi->membase + COMCERTO_SPI_SER);
		__raw_writel(8, spi->membase + COMCERTO_SPI_RXFTLR);
		__raw_writel(0, spi->membase + COMCERTO_SPI_TXFTLR);
		__raw_writel(0, spi->membase + COMCERTO_SPI_IMR);
		__raw_writel(1, spi->membase + COMCERTO_SPI_SSIENR);

		if (transfer->fs <= 8)
			rc = do_read_only_transfer8(spi, transfer->rbuf, &transfer->rlen);
		else
			rc = do_read_only_transfer16(spi, (u16 *) transfer->rbuf, &transfer->rlen);
	
		break;

	case SPI_TRANSFER_MODE_WRITE_READ:
		ctrlr0 |= (0x0000 << 8);

		__raw_writel(ctrlr0, spi->membase + COMCERTO_SPI_CTRLR0);
		__raw_writel(baudr, spi->membase + COMCERTO_SPI_BAUDR);
	//	__raw_writel(ser, spi->membase + COMCERTO_SPI_SER);
		__raw_writel(8, spi->membase + COMCERTO_SPI_RXFTLR);
		__raw_writel(0, spi->membase + COMCERTO_SPI_TXFTLR);
		__raw_writel(0, spi->membase + COMCERTO_SPI_IMR);
		__raw_writel(1, spi->membase + COMCERTO_SPI_SSIENR);

		if (transfer->fs <= 8)
		rc = do_write_read_transfer8(spi, transfer->wbuf, &transfer->wlen, transfer->rbuf, &transfer->rlen, ser);
		else
		rc = do_write_read_transfer16(spi, (u16 *) transfer->wbuf, &transfer->wlen, (u16 *) transfer->rbuf, &transfer->rlen, ser);

		break;
	}

	if (config->ba_delay) {
		udelay(config->ba_delay);
	        /* make sure this transaction is finished */
        	while (__raw_readl(spi->membase + COMCERTO_SPI_SR) & BUSY) ;
	}

	__raw_writel(0, spi->membase + COMCERTO_SPI_SER);

	return rc;
}

#if 0
/**
 * comcerto_spi_irq_handler -
 *
 *
 */
irqreturn_t comcerto_spi_irq_handler(int irq, void *dev_id, struct pt_regs * regs)
{
	struct comcerto_spi *spi = (struct comcerto_spi *)dev_id;
	struct spi_adapter *adapter = spi->adapter;
	u32 callback_status = 0;
	u32 status;
	u32 imr;
	irqreturn_t ret = IRQ_NONE;

	printk(KERN_INFO "comcerto_spi_irq_handler(%d, %#lx, %#lx)\n", irq, (unsigned long)dev_id, (unsigned long)regs);

	status = readl(spi->membase + COMCERTO_SPI_ISR);
	if (!status)
		goto out;

	printk(KERN_INFO "status %x\n", status);

	ret = IRQ_HANDLED;

	if (status & TXEIS) {
		callback_status |= SPI_WRITE_DONE;

		printk(KERN_INFO "%x %x\n", readl(spi->membase + COMCERTO_SPI_SR), readl(spi->membase + COMCERTO_SPI_TXFLR));

		/* disable fifo empty interrupt */
		imr = readl(spi->membase + COMCERTO_SPI_IMR) & ~(TXEIM);
		writel(imr, spi->membase + COMCERTO_SPI_IMR);
	}

	if (status & TXOIS) {
		callback_status |= SPI_WRITE_ERROR;
	}

	if (status & RXUIS) {
		callback_status |= SPI_READ_ERROR;
	}

	if (status & RXOIS) {
		callback_status |= SPI_READ_ERROR;
	}

	if (status & RXFIS) {
		callback_status |= SPI_DATA_AVAILABLE;
	}

	spi_callback(adapter, callback_status);

	/* clear all interrupts */
	readl(spi->membase + COMCERTO_SPI_ICR);

      out:
	return ret;
}
#endif

/**
 * comcerto_spi_hw_init -
 *
 *
 */
static void comcerto_spi_hw_init(struct comcerto_spi *spi)
{
//	printk(KERN_INFO "comcerto_spi_hw_init(%#lx)\n", (unsigned long)spi);

#ifndef CONFIG_ARCH_M83XXX
	/* enable SPI bus: not needed for c2k */
	//comcerto_gpio_ctrl(0x3 << 4, 0x3 << 4);
#endif

	/* disable SPI operation */
	writel(0, spi->membase + COMCERTO_SPI_SSIENR);

	/* mask all SPI irq's */
	writel(0, spi->membase + COMCERTO_SPI_IMR);
}

/**
 * comcerto_spi_hw_reset -
 *
 *
 */
static void comcerto_spi_hw_reset(struct comcerto_spi *spi)
{
	/* disable SPI operation */
	writel(0, spi->membase + COMCERTO_SPI_SSIENR);

	/* mask all SPI irq's */
	writel(0, spi->membase + COMCERTO_SPI_IMR);

#ifndef CONFIG_ARCH_M83XXX
	/* disable SPI bus: not needed for c2k */
	//comcerto_gpio_ctrl(0x0 << 4, 0x3 << 4);
#endif
}
#if 0
struct spi_adapter comcerto_spi_adapter = {
	.name = "comcerto_spi",
	.do_transfer = comcerto_spi_do_transfer,
};
#endif

#define        SPI2_DRV_NAME_LEN       15
char spi2_drv_name[SPI2_DRV_NAME_LEN] = "comcerto_spi";

/**
* comcerto_spi_probe -
 *
 *
 */
static int __init comcerto_spi_probe(struct platform_device *pdev)
{
	struct comcerto_spi *spi;
	struct spi_adapter *ladapter;
	unsigned long base, len;

//	printk(KERN_INFO "comcerto_spi_probe(%#lx)\n", (unsigned long) pdev);
	printk(KERN_INFO "%s: comcerto_spi_probe(%#lx)\
			\npdev->resource[0].start=0x%x\
			\npdev->resource[0].end=0x%x\
			\npdev->name=%s\
			\npdev->id=%d\n", __func__, (unsigned long) pdev, \
			pdev->resource[0].start, pdev->resource[0].end, pdev->name, pdev->id);


	spi = kmalloc(sizeof(struct comcerto_spi), GFP_KERNEL);
	if (spi == NULL) {
		printk(KERN_INFO "comcerto_spi: error allocating memory");
		goto err0;
	}

	ladapter = kmalloc(sizeof(struct spi_adapter), GFP_KERNEL);
	if (ladapter == NULL) {
		printk(KERN_INFO "%s: Error allocating memory: ladapter=0x%x", __func__,\
				(unsigned int)ladapter);
		goto err1;
	}

	base = pdev->resource[0].start;
	len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(base, len, COMCERTO_SPI_DRIVER_NAME)) {
		printk(KERN_INFO "comcerto_spi: error requesting memory region %#lx - %#lx", base, base + len);
		goto err2;
	}

	/* io-remaped in arch/arm/mm.c */
	if(pdev->id == 0)
		spi->membase = APB_VADDR(pdev->resource[0].start);
	else
		if(pdev->id == 1)
			spi->membase = AXI_VADDR(pdev->resource[0].start);
		else
		{
			printk (KERN_INFO "%s: No support for pdev->id = %d\n",\
					__func__, pdev->id);
			goto err3;
		}

	printk (KERN_INFO "%s: pdev->id=%d spi->membase=0x%x\n", __func__,\
			pdev->id, (unsigned int)spi->membase);
	//spi->membase = ioremap(pdev->resource[0].start, len);
	spi->irq = pdev->resource[1].start;

	//pratapc pwr_mgmt_clk_restore(COMPONENT_SPI);

	comcerto_spi_hw_init(spi);
#if 0
	if (request_irq(spi->irq, comcerto_spi_irq_handler, SA_SHIRQ, COMCERTO_SPI_DRIVER_NAME, spi)) {
		printk(KERN_INFO "comcerto_spi: error requesting irq %d\n", IRQ_SPI);
		goto err2;
	}
#endif
	spi->adapter = ladapter;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	ladapter->dev.parent = &pdev->dev;
#endif
	ladapter->name = spi2_drv_name;
	ladapter->do_transfer = comcerto_spi_do_transfer;
	ladapter->data = spi;
	ladapter->caps.max_sc_rate = COMCERTO_DEFAULTAXICLK / 2;
	ladapter->caps.min_sc_rate = COMCERTO_DEFAULTAXICLK / 0xffff;
	ladapter->caps.max_fs = 16;
	ladapter->caps.min_fs = 4;
	ladapter->caps.max_nframe = 0xffff;
	ladapter->caps.min_nframe = 1;
	ladapter->caps.cs_msk = 0xf;
	ladapter->bus_num = pdev->id;

	printk ("%s: ladapter->bus_num=0x%x\n", __func__,\
			ladapter->bus_num);

	if (spi_add_adapter(ladapter)) {
		printk(KERN_INFO "%s:error adding adapter\n", __func__);
		goto err3;
	}

	spi->clock_rate = COMCERTO_DEFAULTAXICLK;
	platform_set_drvdata(pdev, spi);

	return 0;

err3:
#if 0
	free_irq(spi->irq, spi);

      err2:
#endif
	release_mem_region(base, len);
err2:
	kfree(ladapter);

err1:
	kfree(spi);

      err0:
	return -1;
}

/**
 * comcerto_spi_remove -
 *
 *
 */
static int comcerto_spi_remove(struct platform_device *pdev)
{
	struct comcerto_spi *spi = platform_get_drvdata(pdev);
	unsigned long base, len;

	platform_set_drvdata(pdev, NULL);

	spi_del_adapter(spi->adapter);

	comcerto_spi_hw_reset(spi);

	//pratapc pwr_mgmt_clk_down(COMPONENT_SPI);

//      free_irq(spi->irq, spi);

	base = pdev->resource[0].start;
	len = pdev->resource[0].end - pdev->resource[0].start + 1;

	release_mem_region(base, len);

	kfree(spi->adapter);
	kfree(spi);

	return 0;
}

/* FIXME:  we are not supporting Power management in SPI-2 driver */
#if 0
#ifdef CONFIG_PM
static int spi_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret;
	struct spi_client *client;
	struct list_head *item, *_n;
	struct comcerto_spi *spi = platform_get_drvdata(pdev);
	state.event = PM_EVENT_SUSPEND;
	list_for_each_safe(item, _n, &spi->adapter->clients) {
		client = list_entry(item, struct spi_client, list);
		if(client->driver->suspend != NULL)
			ret = client->driver->suspend(client,state);
		}

	if(ret == 0)
		pwr_mgmt_clk_down(COMPONENT_SPI);

	return 0;
}

static int spi_resume(struct platform_device *pdev)
{
	struct spi_client *client;
	struct list_head *item, *_n;
	struct comcerto_spi *spi = platform_get_drvdata(pdev);
	
	pwr_mgmt_clk_restore(COMPONENT_SPI);

	list_for_each_safe(item, _n, &spi->adapter->clients) {
		client = list_entry(item, struct spi_client, list);
		if(client->driver->resume != NULL)
			client->driver->resume(client);
		}

	return 0;
}
#endif
#endif

static struct platform_driver comcerto_spi_driver = {
	.probe = comcerto_spi_probe,
	.remove = comcerto_spi_remove,
#if 0
#ifdef CONFIG_PM
	.suspend	= spi_suspend,
	.resume		= spi_resume,
#endif
#endif
	.driver = {
		.name = "comcerto_spi",
	},
};

/**
 * comcerto_spi_init -
 *
 *
 */
static int __init comcerto_spi_init(void)
{
//	printk(KERN_INFO "comcerto_spi_init()\n");

	return platform_driver_register(&comcerto_spi_driver);
}

/**
 * comcerto_spi_exit -
 *
 *
 */
static void __exit comcerto_spi_exit(void)
{
	platform_driver_unregister(&comcerto_spi_driver);
}

MODULE_AUTHOR("Mindspeed Technologies, Inc.");
MODULE_DESCRIPTION("Comcerto SPI bus driver");
MODULE_LICENSE("GPL");

module_init(comcerto_spi_init);
module_exit(comcerto_spi_exit);
