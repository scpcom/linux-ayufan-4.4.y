/****************************************************
  This is the driver for the Hisilicon Xor controllers.

  Copyright (c) 2009-2011 by HiC
  All rights reserved.

 *****************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/crc32.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <linux/raid/xor.h>

#include "xor.h"
#include "xor_reg.h"

static unsigned int xor_enabled __read_mostly;

unsigned int wait_flag;

#define XOR_RESOURCE_NAME "xor-engine"

#define BIT31	0x80000000

void *xor_base_ioaddr;
EXPORT_SYMBOL(xor_base_ioaddr);

typedef XOR_DESC xor_desc_t;

struct xor_chain {
	struct device		*dev;
	unsigned int		idx;
	unsigned int		pending;
	xor_desc_t		*desc;		/* N descriptors    */
	dma_addr_t		base;		/* phy address      */
	struct xor_channel	*owner;		/* owned by channel */
	unsigned int		busy;		/* busy by cpu	    */
	unsigned int		cookie_used;	/* last used	    */
	unsigned int		cookie_done;	/* last completed   */
	wait_queue_head_t       intr_wait;
	spinlock_t		lock;
};

struct xor_channel {
	unsigned int		idx;
	unsigned int		busy;
	struct xor_chain	*chain;		/* busy on chain */
	unsigned int		pad;
};

struct xor_channel *xor_channels;
struct xor_chain *xor_chains;

void dump_xor_reg(void)
{
	int i;

	for (i = 0; i < 64; i++) {
		int offset = i * 4;
		pr_info("\tReg No. %d (offset 0x%x): 0x%08x\n", i,
				offset, readl(xor_base_ioaddr + offset));
	}
}
EXPORT_SYMBOL(dump_xor_reg);

static inline unsigned int xor_is_active(void)
{
	return readl(xor_base_ioaddr + XOR_ACTIVE_REG);
}

/*
 * set dma operation mode for channel i
 */
static inline void xor_mode_dma(void)
{
	unsigned int mode = readl(xor_base_ioaddr + XOR_CONFIG_REG);

	mode &= ~XEXCR_OPERATION_MODE_MASK;
	mode |= XEXCR_OPERATION_MODE_DMA;
	writel(mode, xor_base_ioaddr + XOR_CONFIG_REG);
}

/*
 * set xor operation mode for channel i
 */
static inline void xor_mode_xor(void)
{
	unsigned int mode = readl(xor_base_ioaddr + XOR_CONFIG_REG);

	mode &= ~XEXCR_OPERATION_MODE_MASK;
	mode |= XEXCR_OPERATION_MODE_XOR;
	writel(mode, xor_base_ioaddr + XOR_CONFIG_REG);
}

/*
 * run dma operation on channel
 */
static inline void xor_dma(unsigned int base)
{
	writel(base, xor_base_ioaddr + XOR_NEXTDESC_REG);
	writel(XEXACTR_XESTART_MASK, xor_base_ioaddr + XOR_ACTIVE_REG);
}

void xor_wait(void)
{
	unsigned int tmp;
	unsigned int times = 10000;

	while (times--) {
		tmp = readl(xor_base_ioaddr + XOR_INTCAUSE_REG);
		if (tmp & END_OF_CHAIN)
			break;
	}

	writel(tmp, xor_base_ioaddr + XOR_INTCAUSE_REG);

	if (!times)
		pr_err("%s: wait time out...\n", __func__);
	return;
}

/*
 * Allocate chain
 */
static inline unsigned int xor_try_chain(struct xor_chain *chain)
{
	unsigned long flags = 0;

	local_irq_save(flags);

	if (chain->busy) {
		local_irq_restore(flags);
		return 1;
	}

	chain->busy = 1;

	local_irq_restore(flags);

	return 0;
}

/*******************************************************************************
 * xor_state_get - get xor channel state.
 *
 * DESCRIPTION:
 *       XOR channel activity state can be active, idle, paused.
 *       This function retrunes the channel activity state.
 *
 * INPUT:
 *	  NULL

 * OUTPUT:
 *       None.
 *
 * RETURN:
 *       XOR_CHANNEL_IDLE    - If the engine is idle.
 *       XOR_CHANNEL_ACTIVE  - If the engine is busy.
 *       XOR_CHANNEL_PAUSED  - If the engine is paused.
 *       XOR_UNDEFINED_STATE  - If the engine state is undefind or there is no
 *                             such engine
 *
 ******************************************************************************/
XOR_STATE xor_state_get(void)
{
	unsigned int state;

	/* read the current state */
	state = readl(xor_base_ioaddr + XOR_ACTIVE_REG);
	state &= XEXACTR_XESTATUS_MASK;

	/* return the state */
	switch (state) {
	case XEXACTR_XESTATUS_IDLE:
		return XOR_IDLE;
	case XEXACTR_XESTATUS_ACTIVE:
		return XOR_ACTIVE;
	case XEXACTR_XESTATUS_PAUSED:
		return XOR_PAUSED;
	}
	return XOR_UNDEFINED_STATE;
}

void xor_state_set(unsigned int state)
{
	unsigned int tmp;

	tmp = readl(xor_base_ioaddr + XOR_ACTIVE_REG);
	tmp |= state;
	writel(tmp, xor_base_ioaddr + XOR_ACTIVE_REG);
}
/*******************************************************************************
 * xor_command_set - set command of xor channel
 *
 * DESCRIPTION:
 *	XOR channel can be started, idle, paused and restarted.
 *	Paused can be set only if channel is active.
 *	Start can be set only if channel is idle or paused.
 *	Restart can be set only if channel is paused.
 *	Stop can be set only if channel is active.
 *
 * INPUT:
 *       chan     - The channel number
 *       command  - The command type (start, stop, restart, pause)
 *
 * OUTPUT:
 *       None.
 *****************************************************************************/
int xor_command_set(XOR_COMMAND command)
{
	XOR_STATE    state;

	/* get the current state */
	state = xor_state_get();

	/* command is start and current state is idle */
	if ((command == XOR_START) && (state == XOR_IDLE)) {
		xor_state_set(XEXACTR_XESTART_MASK);
		return 0;
	/* command is stop and current state is active*/
	} else if ((command == XOR_STOP) && (state == XOR_ACTIVE)) {
		xor_state_set(XEXACTR_XESTOP_MASK);
		return 0;
	/* command is paused and current state is active */
	} else if ((command == XOR_PAUSE) && (state == XOR_ACTIVE)) {
		xor_state_set(XEXACTR_XEPAUSE_MASK);
		return 0;
	/* command is restart and current state is paused*/
	} else if ((command == XOR_RESTART) && (state == XOR_PAUSED)) {
		xor_state_set(XEXACTR_XERESTART_MASK);
		return 0;
	/* command is stop and current state is idle*/
	} else if ((command == XOR_STOP) && (state == XOR_IDLE))
		return 0;

	/* illegal command */
	pr_err("%s: ERR. Illegal command\n", __func__);

	return -1;
}

/*******************************************************************************
 * xor_ctrl_set - set xor channel control registers
 *
 * DESCRIPTION:
 *
 * INPUT:
 *
 * OUTPUT:
 *       None.
 *
 * NOTE:
 *    This function does not modify the OperationMode field of control register.
 *
 ******************************************************************************/
int xor_ctrl_set(void)
{
	unsigned int tmp;
	/* nothing to set for now, maybe set sommething later*/

	tmp = readl(xor_base_ioaddr + XOR_CONFIG_REG);
	tmp |= 0xff0000;
	writel(tmp, xor_base_ioaddr + XOR_CONFIG_REG);

	return 0;
}

void xor_engine_init(void)
{
	/* unrest XOR bus */
	writel(0xa, (void *)IO_ADDRESS(0x120400e0));

	/* unlock regs to write */
	writel(0x0, xor_base_ioaddr + XOR_ACCESSPRO_REG);

	/* Abort any XOR activity & set default configuration */
	xor_command_set(XOR_STOP);
	xor_ctrl_set();
}

#ifndef CONFIG_XOR_POLL
static irqreturn_t xor_irq_handler(int irq, void *arg)
{
	unsigned int status, err_status;

	status = readl(xor_base_ioaddr + XOR_INTCAUSE_REG);

	err_status = readl(xor_base_ioaddr + XOR_ERRCAUSE_REG);

	/* clear all int status */
	writel(status, xor_base_ioaddr + XOR_INTCAUSE_REG);

	/* disable all intrrupt */
	/* write(0, xor_base_ioaddr + XOR_INTENABLE_REG); */

	if (status & ACCESS_FORBID) {
		pr_err("ERROR: Access Forbiden Address: 0x%x.\n",
				readl(xor_base_ioaddr + XOR_ERRADDR_REG));

		wait_flag = 2;
	}

	if (status & WRITE_FORBID) {
		pr_err("ERROR: Try to Access Write-protected Address: 0x%x.\n",
				readl(xor_base_ioaddr + XOR_ERRADDR_REG));

		wait_flag = 2;
	}

	if (status & OWN_BIT_ERROR) {
		pr_err("ERROR: Desc is owned by CPU.\n");

		wait_flag = 2;
	}

	if (status & AXI_BUS_ERROR) {
		if (err_status == READ_ERROR_ADDR)
			pr_err("ERROR: READ AXI Bus Address: 0x%x.\n",
				readl(xor_base_ioaddr + XOR_ERRADDR_REG));

		if (err_status == WRITE_ERROR_ADDR)
			pr_err("ERROR: WRITE AXI Bus Address: 0x%x.\n",
				readl(xor_base_ioaddr + XOR_ERRADDR_REG));

		wait_flag = 2;
	}

	if ((status & END_OF_DESC) || (status & END_OF_CHAIN))
		wait_flag = 1;

	wake_up(&xor_chains->intr_wait);

	return IRQ_HANDLED;
}
#endif /* !CONFIG_XOR_POLL */

static int xor_dvr_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	unsigned int va;
	xor_desc_t *desc;
	unsigned int temp;

	/* get baseaddr for xor engine */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		pr_err("%s: ERROR: memory allocation failed", __func__);
		pr_err("cannot get the I/O addr 0x%x\n",
					(unsigned int)res->start);
		return -EBUSY;
	}

	xor_base_ioaddr =
		ioremap_nocache(res->start, res->end - res->start + 1);

	if (!xor_base_ioaddr) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out_release_region;
	}

	/* init xor engine */
	xor_engine_init();

	/* channels */
	va = (u32)kmalloc((sizeof(struct xor_channel)), GFP_KERNEL);
	xor_channels = (struct xor_channel *)va;
	memset(xor_channels, 0, sizeof(struct xor_channel));

	xor_channels->busy = 0;
	xor_channels->chain = NULL;

	/* chains */
	va = (u32)kmalloc(sizeof(struct xor_chain), GFP_KERNEL);
	xor_chains = (struct xor_chain *)va;
	memset(xor_chains, 0, sizeof(struct xor_chain));

	xor_chains->dev = &(pdev->dev);

	desc = (xor_desc_t *)kmalloc(sizeof(xor_desc_t), GFP_KERNEL);

	xor_chains->desc = desc;
	xor_chains->base = virt_to_phys((void *)desc);
	if (xor_chains->desc == NULL) {
		pr_err("%s:ERROR allocating the XOR desc\n", __func__);
		return -ENOMEM;
	}

	init_waitqueue_head(&xor_chains->intr_wait);
	spin_lock_init(&xor_chains->lock);
#ifndef CONFIG_XOR_POLL
	ret = request_irq(CONFIG_XOR_IRQNUM, xor_irq_handler,
			0, XOR_RESOURCE_NAME, pdev);
	if (unlikely(ret < 0)) {
		pr_err("%s: ERROR: allocating the IRQ %d (error: %d)\n",
				__func__, CONFIG_XOR_IRQNUM, ret);
		goto out_free_irq;
	}
#endif

	/* enable all intr for xor */
	writel(0xffffffff, xor_base_ioaddr + XOR_INTENABLE_REG);

	/* set AXI go through ACP */
	temp = readl((void *)IO_ADDRESS(0x1212000c));
	temp |= 0x1 << 3;
	writel(temp, (void *)IO_ADDRESS(0x1212000c));

	xor_enabled = 1;

	return 0;

#ifndef CONFIG_XOR_POLL
out_free_irq:
#endif
	free_irq(CONFIG_XOR_IRQNUM, pdev);

out_release_region:
	release_mem_region(res->start, resource_size(res));

	return ret;
}

int xor_dvr_remove(struct platform_device *pdev)
{
	struct xor_chain *chain;

	xor_enabled = 0;

	chain = xor_chains;

	dma_free_coherent(chain->dev,
			sizeof(struct xor_chain), chain->desc, chain->base);

	kfree(xor_chains);
	kfree(xor_channels);

	iounmap(xor_base_ioaddr);

	return 0;
}

static struct resource xor_resources[] = {
	[0] = {
		.start = CONFIG_XOR_IOADDR,
		.end = CONFIG_XOR_IOADDR + CONFIG_XOR_IOSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = CONFIG_XOR_IRQNUM,
		.end = CONFIG_XOR_IRQNUM,
		.flags = IORESOURCE_IRQ,
	}
};

static void xor_platform_device_release(struct device *dev)
{
}

static u64 xor_dmamask = DMA_BIT_MASK(32);

static struct platform_device xor_platform_device = {
	.name = XOR_RESOURCE_NAME,
	.id = 0,
	.dev = {
		.dma_mask = &xor_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.release = xor_platform_device_release,
	},
	.num_resources = ARRAY_SIZE(xor_resources),
	.resource = xor_resources,
};

static struct platform_driver xor_driver = {
	.probe = xor_dvr_probe,
	.remove = xor_dvr_remove,
	.driver = {
		.name = XOR_RESOURCE_NAME,
		.owner = THIS_MODULE,
	},
};

/**
 * xor_init_module - Entry point for the driver
 * Description: This function is the entry point for the driver.
 */
static int __init xor_init_module(void)
{
	if (platform_device_register(&xor_platform_device)) {
		pr_err("No XOR devices registered!\n");
		return -ENODEV;
	}

	if (platform_driver_register(&xor_driver)) {
		pr_err("No XOR driver registered!\n");
		return -ENODEV;
	}

	return 0;
}

/**
 * xor_cleanup_module - Cleanup routine for the driver
 * Description: This function is the cleanup routine for the driver.
 */
static void __exit xor_cleanup_module(void)
{
	platform_driver_unregister(&xor_driver);

	platform_device_unregister(&xor_platform_device);
}

module_init(xor_init_module);
module_exit(xor_cleanup_module);

int xor_memxor(unsigned int src_count, unsigned int bytes, void *dest,
		void **srcs)
{
	unsigned int *p;
	int i;

	if (!xor_enabled)
		goto out;
	spin_lock(&xor_chains->lock);

	BUG_ON(src_count < 1);

	xor_chains->pending = 1;
	xor_chains->desc->phydestadd = virt_to_phys(dest);
	xor_chains->desc->bytecnt = bytes;
	xor_chains->desc->phynextdescptr = 0;
	/* add dest to the count */
	xor_chains->desc->desccommand = (1 << (src_count + 1)) - 1;
	xor_chains->desc->status = BIT31;
	p = &xor_chains->desc->srcadd0;

	for (i = 0; i < src_count; i++)
		p[i] = virt_to_phys(srcs[i]);

	p[i] = virt_to_phys(dest);

	xor_mode_xor();
	xor_dma(xor_chains->base);

	/* could be useful before busywait, because we already have it clean */
#ifdef CONFIG_XOR_POLL
	xor_wait();
#else
	wait_event(xor_chains->intr_wait, wait_flag != 0);

	if (wait_flag == 2)
		goto out2;
#endif

	xor_mode_dma();

	wait_flag = 0;

	spin_unlock(&xor_chains->lock);
	return 0;

#ifndef CONFIG_XOR_POLL
out2:
#endif
	xor_chains->busy = 0;
	spin_unlock(&xor_chains->lock);
out:
	wait_flag = 0;

	return 1;
}
EXPORT_SYMBOL(xor_memxor);

MODULE_DESCRIPTION("xor controller driver");
MODULE_AUTHOR("Li Gaopeng <gpeng.li@huawei.com>");
MODULE_LICENSE("GPL");
