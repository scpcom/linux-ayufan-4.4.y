/*
 * SDIO driver for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <generated/uapi/linux/version.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/spinlock.h>
#include <net/mac80211_xr.h>

#include "platform.h"
#include "xradio.h"
#include "sbus.h"

/* sdio vendor id and device id*/
#define SDIO_VENDOR_ID_XRADIO 0x0020
#define SDIO_DEVICE_ID_XRADIO 0x2281
static const struct sdio_device_id xradio_sdio_ids[] = {
	{SDIO_DEVICE(SDIO_VENDOR_ID_XRADIO, SDIO_DEVICE_ID_XRADIO)},
	/*{ SDIO_DEVICE(SDIO_ANY_ID, SDIO_ANY_ID) },*/
	{ /* end: all zeroes */ },
};

/* sbus_ops implemetation */
static int sdio_data_read(struct sbus_priv *self, unsigned int addr,
			  void *dst, int count)
{
	return sdio_memcpy_fromio(self->func, dst, addr, count);
}

static int sdio_data_write(struct sbus_priv *self, unsigned int addr,
			   const void *src, int count)
{
	return sdio_memcpy_toio(self->func, addr, (void *)src, count);
}

static void sdio_lock(struct sbus_priv *self)
{
	sdio_claim_host(self->func);
}

static void sdio_unlock(struct sbus_priv *self)
{
	sdio_release_host(self->func);
}

static size_t sdio_align_len(struct sbus_priv *self, size_t size)
{
	return sdio_align_size(self->func, size);
}

static int sdio_set_blk_size(struct sbus_priv *self, size_t size)
{
	sbus_printk(XRADIO_DBG_NIY, "set blocksize=%zu\n", size);
	return sdio_set_block_size(self->func, size);
}

static size_t sdio_cur_blk_size(struct sbus_priv *self)
{
	return (size_t)self->func->cur_blksize;
}

#ifndef CONFIG_XRADIO_USE_GPIO_IRQ
static void sdio_irq_handler(struct sdio_func *func)
{
	struct sbus_priv *self = sdio_get_drvdata(func);
	unsigned long flags;
	sbus_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	SYS_BUG(!self);
	spin_lock_irqsave(&self->lock, flags);
	if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	spin_unlock_irqrestore(&self->lock, flags);
}
#endif

static int sdio_irq_subscribe(struct sbus_priv *self,
				     sbus_irq_handler handler,
				     void *priv)
{
	int ret = 0;
	unsigned long flags;


	if (!handler)
		return -EINVAL;
	sbus_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	spin_lock_irqsave(&self->lock, flags);
	self->irq_priv = priv;
	self->irq_handler = handler;
	spin_unlock_irqrestore(&self->lock, flags);

	sdio_claim_host(self->func);
#ifndef CONFIG_XRADIO_USE_GPIO_IRQ
	ret = sdio_claim_irq(self->func, sdio_irq_handler);
	if (ret) {
		sbus_printk(XRADIO_DBG_ERROR, "%s:sdio_claim_irq failed(%d).\n",
				__func__, ret);

	} else {
		sbus_printk(XRADIO_DBG_NIY, "%s:sdio_claim_irq success.\n", __func__);
	}
#else
	ret = xradio_request_gpio_irq(&(self->func->dev), self);
	if (!ret) {
		/* Hack to access Fuction-0 */
		u8 cccr;
		int func_num = self->func->num;
		sbus_printk(XRADIO_DBG_NIY, "%s:xradio_request_gpio_irq success.\n",
					__func__);

		self->func->num = 0;
		cccr = sdio_readb(self->func, SDIO_CCCR_IENx, &ret);
		cccr |= BIT(0);         /* Master interrupt enable ... */
		cccr |= BIT(func_num);  /* ... for our function */
		sdio_writeb(self->func, cccr, SDIO_CCCR_IENx, &ret);
		if (ret) {
			xradio_free_gpio_irq(&(self->func->dev), self);
			if (MCI_CHECK_READY(self->func->card->host, 1000) != 0)
				sbus_printk(XRADIO_DBG_ERROR,
					    "%s:MCI_CHECK_READY timeout\n", __func__);
		}
		/* Restore the WLAN function number */
		self->func->num = func_num;
	} else {
		sbus_printk(XRADIO_DBG_ERROR, "%s:xradio_request_gpio_irq failed(%d).\n",
				__func__, ret);
	}
#endif
	sdio_release_host(self->func);

	return ret;
}

static int sdio_irq_unsubscribe(struct sbus_priv *self)
{
	int ret = 0;
	unsigned long flags;

	sbus_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	if (!self->irq_handler) {
		sbus_printk(XRADIO_DBG_ERROR, "%s:irq_handler is NULL!\n", __func__);
		return 0;
	}

#ifndef CONFIG_XRADIO_USE_GPIO_IRQ
	sdio_claim_host(self->func);
	ret = sdio_release_irq(self->func);
	sdio_release_host(self->func);
#else
	xradio_free_gpio_irq(&(self->func->dev), self);
#endif /*CONFIG_XRADIO_USE_GPIO_IRQ*/

	spin_lock_irqsave(&self->lock, flags);
	self->irq_priv = NULL;
	self->irq_handler = NULL;
	spin_unlock_irqrestore(&self->lock, flags);

	return ret;
}

static int sdio_pm(struct sbus_priv *self, bool suspend)
{
	int ret = 0;
	if (suspend) {
		/* Notify SDIO that XRADIO will remain powered during suspend */
		ret = sdio_set_host_pm_flags(self->func, MMC_PM_KEEP_POWER);
		if (ret)
			sbus_printk(XRADIO_DBG_ERROR,
				    "Error setting SDIO pm flags: %i\n", ret);
	}

	return ret;
}

static int sdio_reset(struct sbus_priv *self)
{
	return 0;
}

static struct sbus_ops sdio_sbus_ops = {
	.sbus_data_read     = sdio_data_read,
	.sbus_data_write    = sdio_data_write,
	.lock               = sdio_lock,
	.unlock             = sdio_unlock,
	.align_size         = sdio_align_len,
	.set_block_size     = sdio_set_blk_size,
	.get_block_size     = sdio_cur_blk_size,
	.irq_subscribe      = sdio_irq_subscribe,
	.irq_unsubscribe    = sdio_irq_unsubscribe,
	.power_mgmt         = sdio_pm,
	.reset              = sdio_reset,
};
static struct sbus_priv sdio_self;

#if (defined(CONFIG_XRADIO_DEBUGFS))
u32 dbg_sdio_clk;
static int sdio_set_clk(struct sdio_func *func, u32 clk)
{
	if (func) {
		/* set min to 1M */
		if (func->card->host->ops->set_ios && clk >= 1000000) {
			sdio_claim_host(func);
			func->card->host->ios.clock = (clk < 50000000) ? clk : 50000000;
			func->card->host->ops->set_ios(func->card->host,
			     &func->card->host->ios);
			sdio_release_host(func);
			sbus_printk(XRADIO_DBG_ALWY, "%s:change mmc clk=%d\n", __func__,
				    func->card->host->ios.clock);
		} else {
			sbus_printk(XRADIO_DBG_ALWY, "%s:fail change mmc clk=%d\n",
				    __func__, clk);
		}
	}
	return 0;
	sbus_printk(XRADIO_DBG_TRC, "%s\n", __func__);
}
#endif

/* Probe Function to be called by SDIO stack when device is discovered */
static int sdio_probe(struct sdio_func *func,
		      const struct sdio_device_id *id)
{
	sbus_printk(XRADIO_DBG_ALWY, "XRadio Device:sdio clk=%d\n",
		    func->card->host->ios.clock);
	sbus_printk(XRADIO_DBG_NIY, "sdio func->class=%x\n", func->class);
	sbus_printk(XRADIO_DBG_NIY, "sdio_vendor: 0x%04x\n", func->vendor);
	sbus_printk(XRADIO_DBG_NIY, "sdio_device: 0x%04x\n", func->device);
	sbus_printk(XRADIO_DBG_NIY, "Function#: 0x%04x\n",   func->num);
	sbus_printk(XRADIO_DBG_NIY, "max_blksize:%u\n",   func->max_blksize);
	sbus_printk(XRADIO_DBG_NIY, "cur_blksize:%u\n",   func->cur_blksize);

#if (defined(CONFIG_XRADIO_DEBUGFS))
	if (dbg_sdio_clk)
		sdio_set_clk(func, dbg_sdio_clk);
#endif

#if 0				/*for odly and sdly debug.*/
	{
		u32 sdio_param = 0;
		sdio_param = readl(__io_address(0x01c20088));
		sdio_param &= ~(0xf << 8);
		sdio_param |= 3 << 8;
		sdio_param &= ~(0xf << 20);
		sdio_param |= s_dly << 20;
		writel(sdio_param, __io_address(0x01c20088));
		sbus_printk(XRADIO_DBG_ALWY, "%s: 0x01c20088=0x%08x\n",
			    __func__, sdio_param);
	}
#endif

	sdio_self.func = func;
	sdio_self.func->card->quirks |= MMC_QUIRK_BROKEN_BYTE_MODE_512;
	sdio_set_drvdata(func, &sdio_self);
	sdio_claim_host(func);
	sdio_enable_func(func);
	sdio_release_host(func);

	sdio_self.load_state = SDIO_LOAD;
	wake_up(&sdio_self.init_wq);

	return 0;
}

/* Disconnect Function to be called by SDIO stack when
 * device is disconnected */
static void sdio_remove(struct sdio_func *func)
{
	struct sbus_priv *self = sdio_get_drvdata(func);
	sbus_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	sdio_claim_host(func);
	sdio_disable_func(func);
	sdio_release_host(func);
	sdio_set_drvdata(func, NULL);
	if (self) {
		self->func = NULL;
		self->load_state = SDIO_UNLOAD;
		wake_up(&sdio_self.init_wq);
	}
}

#if 0
static int sdio_suspend(struct device *dev)
{
	int ret = 0;
	sbus_printk(XRADIO_DBG_NIY, "%s\n", __func__);
#ifdef CONFIG_XRADIO_ETF
	ret = xradio_etf_suspend();
	if (ret) {
		sbus_printk(XRADIO_DBG_ERROR,
			"xradio_etf_suspend failed(%d)\n", ret);
	}
#endif
	return ret;
}
#endif

/*
 * wifi device need to use sdio while working, so sdio should not suspend before wifi device.
 * However, sdio may suspend before wifi device, which is controlled by system.so we need to
 * keep sdio working.The better way is register sdio device as the father device of wifi device,
 * so it will suspend wifi device first
 */
extern struct xradio_common *g_hw_priv;
static int sdio_suspend(struct device *dev)
{
	int ret = 0;
	sbus_printk(XRADIO_DBG_NIY, "%s\n", __func__);

	if (g_hw_priv && (g_hw_priv->exit_sync == true)) {
		sbus_printk(XRADIO_DBG_WARN, "Don't suspend because xradio is exiting\n");
		return -EBUSY;
	}

#ifdef CONFIG_XRADIO_ETF
	ret = xradio_etf_suspend();
	if (!ret) {
		struct sdio_func *func = dev_to_sdio_func(dev);
		ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
		if (ret) {
			xradio_etf_resume();
			sbus_printk(XRADIO_DBG_ERROR, "set MMC_PM_KEEP_POWER error\n");
		}
	} else {
		sbus_printk(XRADIO_DBG_ERROR, "xradio_etf_suspend failed\n");
	}
#else
	struct sdio_func *func = dev_to_sdio_func(dev);
	ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	if (ret) {
		sbus_printk(XRADIO_DBG_ERROR, "set MMC_PM_KEEP_POWER error\n");
	}
#endif

	return ret;
}

static int sdio_resume(struct device *dev)
{
	sbus_printk(XRADIO_DBG_NIY, "%s\n", __func__);
#ifdef CONFIG_XRADIO_ETF
	xradio_etf_resume();
#endif
	return 0;
}

static const struct dev_pm_ops sdio_pm_ops = {
	.suspend = sdio_suspend,
	.resume = sdio_resume,
};

static struct sdio_driver sdio_driver = {
	.name     = "xradio_wlan",
	.id_table = xradio_sdio_ids,
	.probe    = sdio_probe,
	.remove   = sdio_remove,
	.drv = {
		.pm = &sdio_pm_ops,
	}
};

/* Init Module function -> Called by insmod */
struct device *sbus_sdio_init(struct sbus_ops **sdio_ops,
			      struct sbus_priv **sdio_priv)
{
	int ret = 0;
	struct device *sdio_dev = NULL;
	sbus_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	/*initialize sbus_priv.*/
	if (sdio_self.load_state == SDIO_UNLOAD) {
		spin_lock_init(&sdio_self.lock);
		init_waitqueue_head(&sdio_self.init_wq);

		/*module power up.*/
		xradio_wlan_power(1);
		/*detect sdio card.*/
		xradio_sdio_detect(1);

		/*setup sdio driver.*/
		ret = sdio_register_driver(&sdio_driver);
		if (ret) {
			sbus_printk(XRADIO_DBG_ERROR, "sdio_register_driver failed!\n");
			return NULL;
		}

		if (wait_event_timeout(sdio_self.init_wq,
			sdio_self.load_state == SDIO_LOAD, 2*HZ) <= 0) {
			sdio_unregister_driver(&sdio_driver);
			sdio_self.load_state = SDIO_UNLOAD;

			xradio_wlan_power(0);	/*power down.*/
			xradio_sdio_detect(0);
			sbus_printk(XRADIO_DBG_ERROR, "sdio probe timeout!\n");
			return NULL;
		}
	}

	/*register sbus.*/
	sdio_dev   = &(sdio_self.func->dev);
	*sdio_ops  = &sdio_sbus_ops;
	*sdio_priv = &sdio_self;

	return sdio_dev;
}

/* SDIO Driver Unloading */
void sbus_sdio_deinit(void)
{
	sbus_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	if (sdio_self.load_state != SDIO_UNLOAD) {
		xradio_wlan_power(0);	/*power down.*/
		xradio_sdio_detect(0);
		if (wait_event_interruptible_timeout(sdio_self.init_wq,
			sdio_self.load_state == SDIO_UNLOAD, 5*HZ) <= 0) {
			sbus_printk(XRADIO_DBG_ERROR, "%s remove timeout!\n", __func__);
		}
		sdio_unregister_driver(&sdio_driver);
		memset(&sdio_self, 0, sizeof(sdio_self));
		msleep(5);
	} else {
		sbus_printk(XRADIO_DBG_ERROR, "%s sdio did not init!\n", __func__);
	}
}
