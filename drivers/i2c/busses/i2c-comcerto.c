/*
 *  drivers/i2c/busses/i2c-comcerto.c
 *
 *  Copyright (C) 2008 Mindspeed Technologies, Inc.
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

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <mach/i2c.h>
#include <mach/irqs.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/reset.h>

MODULE_AUTHOR("Mindspeed Technologies, Inc.");
MODULE_DESCRIPTION("Comcerto I2C bus driver");
MODULE_LICENSE("GPL");

#define SPEED_HIGH_KHZ		3400
#define SPEED_FULL_KHZ		400
#define SPEED_NORMAL_KHZ	100

static int force_poll = 0;
static struct clk *clk_i2c;
module_param(force_poll, bool, S_IRUGO);
MODULE_PARM_DESC(force_poll, "Force polling mode: 0=interrupt mode, polling mode otherwise");

static int speed = 0;
module_param(speed, int, S_IRUGO);
MODULE_PARM_DESC(speed, "I2C speed: 0=standard, 1=fast, 2=high speed");

struct comcerto_i2c
{
	struct i2c_adapter	*adapter;
	struct device		*dev;
	unsigned long		membase;
	struct resource		*io;
	int			irq;
	u32			speed_khz;

	wait_queue_head_t	wait;
	struct i2c_msg		*msg;
	int			msg_state;
	int			msg_status;	/* < 0: error, == 0: success, > 0: message in progress */
	int			msg_len;
	int			msg_retries;
};

#define REG_ADDR(i2c, offset)		((i2c)->membase + (offset))
#define RD_REG(i2c, offset)		__raw_readb(REG_ADDR(i2c, offset))
#define WR_REG(i2c, offset, byte)	__raw_writeb(byte, REG_ADDR(i2c, offset))
#define RD_DATA(i2c)			RD_REG(i2c, COMCERTO_I2C_DATA)
#define WR_DATA(i2c, byte)		WR_REG(i2c, COMCERTO_I2C_DATA, byte)
#define RD_CNTR(i2c)			RD_REG(i2c, COMCERTO_I2C_CNTR)
#define WR_CNTR(i2c, byte)		WR_REG(i2c, COMCERTO_I2C_CNTR, byte)
#define RD_STAT(i2c)			RD_REG(i2c, COMCERTO_I2C_STAT)
#define WR_CCRFS(i2c, byte)		WR_REG(i2c, COMCERTO_I2C_CCRFS, byte)
#define WR_CCRH(i2c, byte)		WR_REG(i2c, COMCERTO_I2C_CCRH, byte)
#define WR_RESET(i2c, byte)		WR_REG(i2c, COMCERTO_I2C_RESET, byte)

enum
{
	TR_IDLE = 0,
	TR_START_ACK,
	TR_ADDR_ACK,
	TR_DATA_ACK,
	RX_DATA_NACK,
};

static u8 comcerto_i2c_calculate_dividers(struct comcerto_i2c *i2c)
{
	int m, n, hz, speed_hz;
	int saved_n, saved_m, saved_hz;
	u8 dividers;
	unsigned int i2c_clk;

	/* Get the i2c clock rate */
	i2c_clk = clk_get_rate(clk_i2c);

	speed_hz = i2c->speed_khz * 1000;
	saved_hz = saved_n = saved_m = 0;

	for (m = 0; m < 16; m++) {
		for (n = 0; n < 8; n++) {
			hz = i2c_clk / ((1 << n) * (m + 1) * 10);
			if (!saved_hz || abs(speed_hz - hz) < abs(speed_hz - saved_hz)) {
				saved_n = n;
				saved_m = m;
				saved_hz = hz;
			}
		}
	}

	dividers = (saved_m << 3) | saved_n;
	dev_dbg(i2c->dev, "%s: speed=%dkHz, M=%d, N=%d, dividers=0x%02x\n", __FUNCTION__,
		saved_hz/1000, saved_m, saved_n, dividers);
	printk("%s: speed=%dkHz, M=%d, N=%d, dividers=0x%02x\n", __FUNCTION__, saved_hz/1000, saved_m, saved_n, dividers);

	return dividers;
}

/*
 * Returns the timeout (in jiffies) for the given message.
 */
static int comcerto_i2c_calculate_timeout(struct comcerto_i2c *i2c, struct i2c_msg *msg)
{
	int timeout;

	/* if no timeout was specified, calculate it */
	if (i2c->adapter->timeout <= 0) {
		if (i2c->irq >= 0) {
			/* for the interrupt mode calculate timeout for 'full' message */
			timeout = ((int)msg->len) * 10;	/* convert approx. to bits */
			timeout /= i2c->speed_khz;	/* convert to bits per ms (note of kHz scale) */
			timeout += timeout >> 1;	/* add 50% */
			timeout = timeout*HZ / 1000;	/* convert to jiffies */
			if (timeout < HZ / 5)		/* at least 200ms */
				timeout = HZ / 5;
		}
		else
			timeout = HZ;			/* 1 second for the polling mode */
	}
	else
		timeout = i2c->adapter->timeout;

	return timeout;
}

/*
 * Initialize I2C core. Zero CNTR and DATA, try RESET. Short busy wait and check core status.
 * After that set dividers for choosen speed.
 */
static void comcerto_i2c_reset(struct comcerto_i2c *i2c)
{
	u8 status, dividers;

	dev_dbg(i2c->dev, "%s\n", __FUNCTION__);

	WR_CNTR(i2c, 0);
	WR_DATA(i2c, 0);
	WR_RESET(i2c, 1);

	udelay(10);

	status = RD_STAT(i2c);
	if (status != STAT_NO_RELEVANT_INFO)
		dev_warn(i2c->dev, "%s: unexpected status after reset: 0x%02x\n", __FUNCTION__, status);

	/* dividers should be placed in CCRH for high-sped mode and in CCRFS for standard/full modes */
	dividers = comcerto_i2c_calculate_dividers(i2c);
	if (i2c->speed_khz == SPEED_HIGH_KHZ)
		WR_CCRH(i2c, dividers);
	else
		WR_CCRFS(i2c, dividers);
}

static inline void comcerto_i2c_message_complete(struct comcerto_i2c *i2c, int status)
{
	i2c->msg_status = status;
	WR_CNTR(i2c, CNTR_STP);
}

static inline int comcerto_i2c_message_in_progress(struct comcerto_i2c *i2c)
{
	return i2c->msg_status > 0;
}

/*
 * Wait event. This function sleeps in polling mode, in interrupt
 * mode it enables IRQ from I2C core and exits immediately.
 */
static int comcerto_i2c_wait(struct comcerto_i2c *i2c, u8 cntr)
{
	cntr &= ~(CNTR_IFLG | CNTR_IEN);	/* clear both IFLG and IEN */

	if (i2c->irq < 0) {
		ulong jiffies_mark = jiffies + comcerto_i2c_calculate_timeout(i2c, i2c->msg);

		WR_CNTR(i2c, cntr);
		while ((RD_CNTR(i2c) & CNTR_IFLG) == 0) {
			if (need_resched())
				schedule();

			if (time_after(jiffies, jiffies_mark)) {
				dev_warn(i2c->dev, "%s: polling transfer timeout\n", __FUNCTION__);
				comcerto_i2c_message_complete(i2c, -ETIME);
				comcerto_i2c_reset(i2c);
				break;
			}
		}
	}
	else {
		/* enable interrupt */
		WR_CNTR(i2c, cntr | CNTR_IEN);
	}

	return 0;
}

static void comcerto_i2c_state_idle(struct comcerto_i2c *i2c, u8 *cntr)
{
	if (unlikely(i2c->msg->flags & I2C_M_NOSTART)) {
		i2c->msg_state = TR_ADDR_ACK;
	}
	else {
		*cntr = CNTR_STP|CNTR_STA;	/* SPT|STA to auto recover from bus error state transparently at the start of the transfer */
		i2c->msg_state = TR_START_ACK;
	}
}

static void comcerto_i2c_state_start_ack(struct comcerto_i2c *i2c, u8 *cntr)
{
	u8 status, addr;

	*cntr = 0;	/* zero IFLG, IEN (for the interrupt mode it will be enabled in wait function) */

	status = RD_STAT(i2c);

	if (status == STAT_START || status == STAT_START_REPEATED) {
		i2c->msg_state = TR_ADDR_ACK;

		addr = i2c->msg->addr << 1;
		if (i2c->msg->flags & I2C_M_RD)
			addr |= 1;
		if (i2c->msg->flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;		/* invert RW bit if it's requested */

		WR_DATA(i2c, addr);		/* write address and read/write bit */
	} else {
		dev_warn(i2c->dev, "%s: unexpected state (%#x) on start phase, %s\n",
			__FUNCTION__, status, i2c->msg_retries > 1 ? "retrying":"aborting");

		if (--i2c->msg_retries < 0)
			comcerto_i2c_message_complete(i2c, -1);
		else
			comcerto_i2c_state_idle(i2c, cntr);
	}
}

static void comcerto_i2c_rx(struct comcerto_i2c *i2c)
{
	u8 status, cntr = 0;

restart:
	switch (i2c->msg_state) {
	case TR_IDLE:
		comcerto_i2c_state_idle(i2c, &cntr);
		if (unlikely(i2c->msg->flags & I2C_M_NOSTART))
			goto restart;	/* needed to avoid event loss in interrupt mode */
		break;

	case TR_START_ACK:
		comcerto_i2c_state_start_ack(i2c, &cntr);
		break;

	case TR_ADDR_ACK:
		if (unlikely(i2c->msg->flags & I2C_M_NOSTART)) {
			/* we can enter this state if skip start/addr flag is set, so fake good ack */
			status = STAT_ADDR_RD_ACK;
		}
		else {
			status = RD_STAT(i2c);
			/* check whether we should ignore NACK */
			if (status == STAT_DATA_RD_NACK && (i2c->msg->flags & I2C_M_IGNORE_NAK))
				status = STAT_DATA_RD_ACK;
		}

		if (likely(status == STAT_ADDR_RD_ACK)) {
			/* start reception phase - wait until data is ready and loop in RX_DATA_ACK state
			 * until we read all the data, sending ACK after each byte (but the last)
			 */
			i2c->msg_len = 0;
			if (i2c->msg->len > 1) {
				i2c->msg_state = TR_DATA_ACK;
				cntr = CNTR_AAK;
			}
			else if (i2c->msg->len == 1) {
				i2c->msg_state = RX_DATA_NACK;
			}
			else {	/* nothing to receive, send STOP and signal success */
				comcerto_i2c_message_complete(i2c, 0);
			}
		}
		else {
			dev_warn(i2c->dev, "%s: unexpected state (%#x) on address phase, %s\n",
				__FUNCTION__, status, i2c->msg_retries > 1 ? "retrying":"aborting");

			if (--i2c->msg_retries < 0)
				comcerto_i2c_message_complete(i2c, -1);
			else
				comcerto_i2c_state_idle(i2c, &cntr);
		}
		break;

	case TR_DATA_ACK:
		status = RD_STAT(i2c);

		if (likely(status == STAT_DATA_RD_ACK)) {
			i2c->msg->buf[i2c->msg_len++] = RD_DATA(i2c);
			if (likely(i2c->msg->len - i2c->msg_len > 1)) {
				cntr = CNTR_AAK;
			}
			else {
				i2c->msg_state = RX_DATA_NACK;
				/* NACK should be transmitted on the last byte */
			}
		}
		else {
			dev_warn(i2c->dev, "%s: unexpected state (%#x) on read phase\n", __FUNCTION__, status);
			comcerto_i2c_message_complete(i2c, -1);
		}
		break;

	case RX_DATA_NACK:
		status = RD_STAT(i2c);
		if (likely(status == STAT_DATA_RD_NACK)) {
			i2c->msg->buf[i2c->msg_len++] = RD_DATA(i2c);
			comcerto_i2c_message_complete(i2c, 0);
		}
		else {
			dev_warn(i2c->dev, "%s: unexpected state (%#x) on finishing read phase\n", __FUNCTION__, status);
			comcerto_i2c_message_complete(i2c, -1);
		}
	}

	/* no wait if we completed message */
	if (comcerto_i2c_message_in_progress(i2c))
		comcerto_i2c_wait(i2c, cntr);
}

static void comcerto_i2c_tx(struct comcerto_i2c *i2c)
{
	u8 status, cntr = 0;

restart:
	switch (i2c->msg_state) {
	case TR_IDLE:
		comcerto_i2c_state_idle(i2c, &cntr);
		if (unlikely(i2c->msg->flags & I2C_M_NOSTART))
			goto restart;	/* needed to avoid event loss in interrupt mode */
		break;

	case TR_START_ACK:
		comcerto_i2c_state_start_ack(i2c, &cntr);
		break;

	case TR_ADDR_ACK:
		if (unlikely(i2c->msg->flags & I2C_M_NOSTART)) {
			/* we can enter this state if skip start/addr flag is set, so fake good ack */
			status = STAT_ADDR_WR_ACK;
		}
		else {
			status = RD_STAT(i2c);
			if (status == STAT_DATA_WR_NACK && (i2c->msg->flags & I2C_M_IGNORE_NAK))
				status = STAT_DATA_WR_ACK;
		}

		if (likely(status == STAT_ADDR_WR_ACK)) {
			/* start reception phase - wait until data is ready and loop in TX_DATA_ACK state
			 * until we read all the data, sending ACK after each byte (but the last)
			 */
			i2c->msg_state = TR_DATA_ACK;
			i2c->msg_len = 0;
			if (likely(i2c->msg->len != 0)) {
				WR_DATA(i2c, i2c->msg->buf[i2c->msg_len++]);
				//printk("comcerto_i2c_tx: i2c->msg->buf[i2c->msg_len - 1]=%d\n", i2c->msg->buf[i2c->msg_len - 1]);
			}
			else {
				/* nothing to transmit, send STOP and signal success */
				comcerto_i2c_message_complete(i2c, 0);
			}
		}
		else {
			dev_warn(i2c->dev, "%s: unexpected state (%#x) on address phase, %s\n",
				__FUNCTION__, status, i2c->msg_retries > 1 ? "retrying":"aborting");

			if (--i2c->msg_retries < 0)
				comcerto_i2c_message_complete(i2c, -1);
			else
				comcerto_i2c_state_idle(i2c, &cntr);
		}
		break;

	case TR_DATA_ACK:
		status = RD_STAT(i2c);
		if (status == STAT_DATA_WR_NACK && (i2c->msg->flags & I2C_M_IGNORE_NAK))
			status = STAT_DATA_WR_ACK;

		if (likely(status == STAT_DATA_WR_ACK)) {
			if (i2c->msg->len > i2c->msg_len)
				WR_DATA(i2c, i2c->msg->buf[i2c->msg_len++]);
			else
				comcerto_i2c_message_complete(i2c, 0);
		}
		else {
			dev_warn(i2c->dev, "%s: unexpected state (%#x) on read data phase\n", __FUNCTION__, status);
			comcerto_i2c_message_complete(i2c, -1);
		}
		break;
	}

	if (comcerto_i2c_message_in_progress(i2c))
		comcerto_i2c_wait(i2c, cntr);
}

static irqreturn_t comcerto_i2c_interrupt(int irq, void *dev_id)
{
	struct comcerto_i2c *i2c = dev_id;

	if (!(RD_CNTR(i2c) & CNTR_IFLG))
		goto none;

	/* IRQ enable/disable logic is hidden in state handlers, all we need is to wake
	 * process when message completed.
	 */
	if (i2c->msg->flags & I2C_M_RD)
		comcerto_i2c_rx(i2c);
	else
		comcerto_i2c_tx(i2c);

	if (!comcerto_i2c_message_in_progress(i2c)) {
		WR_CNTR(i2c, RD_CNTR(i2c) & ~CNTR_IEN);	/* disable interrupt unconditionally */
		wake_up(&i2c->wait);
	}

	return IRQ_HANDLED;
none:
	return IRQ_NONE;
}

static void comcerto_i2c_message_process(struct comcerto_i2c *i2c, struct i2c_msg *msg)
{
	i2c->msg = msg;
	i2c->msg_state = TR_IDLE;
	i2c->msg_status = 1;
	i2c->msg_retries = i2c->adapter->retries;

polling_mode:
	if (msg->flags & I2C_M_RD)
		comcerto_i2c_rx(i2c);
	else
		comcerto_i2c_tx(i2c);

	if (i2c->irq < 0) {
		/*if (i2c->msg != NULL)*/
			goto polling_mode;
	}
	else {
		int timeout, res;
		ulong flags;

		timeout = comcerto_i2c_calculate_timeout(i2c, msg);

		res = wait_event_timeout(i2c->wait, i2c->msg_status <= 0, timeout);

		local_irq_save(flags);

		/* check if we timed out and set respective error codes */
		if (res == 0) {
			if (comcerto_i2c_message_in_progress(i2c)) {
				dev_warn(i2c->dev, "%s: interrupt transfer timeout\n", __FUNCTION__);
				comcerto_i2c_message_complete(i2c, -ETIME);
				comcerto_i2c_reset(i2c);
			}
		}

		local_irq_restore(flags);
	}
}

/*
 * Generic master transfer entrypoint.
 * Returns the number of processed messages or error value
 */
static int comcerto_i2c_master_xfer(struct i2c_adapter *adapter, struct i2c_msg msgs[], int num)
{
	struct comcerto_i2c *i2c = i2c_get_adapdata(adapter);
	int i;

	dev_dbg(i2c->dev, "%s: %d messages to process\n", __FUNCTION__, num);

	for (i = 0; i < num; i++) {
		dev_dbg(i2c->dev, "%s: message #%d: addr=%#x, flags=%#x, len=%u\n", __FUNCTION__,
			i, msgs[i].addr, msgs[i].flags, msgs[i].len);

		comcerto_i2c_message_process(i2c, &msgs[i]);

		if (i2c->msg_status < 0) {
			dev_warn(i2c->dev, "%s: transfer failed on message #%d (addr=%#x, flags=%#x, len=%u)\n",
				__FUNCTION__, i, msgs[i].addr, msgs[i].flags, msgs[i].len);
			break;
		}
	}

	if (i2c->msg_status == -1)
		i2c->msg_status = -EIO;

	if (i2c->msg_status == 0)
		i2c->msg_status = num;

	return i2c->msg_status;
}

static u32 comcerto_i2c_functionality(struct i2c_adapter *adap)
{
	return (I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL);
}

static struct i2c_algorithm comcerto_i2c_algo = {
	.master_xfer	= comcerto_i2c_master_xfer,
	.functionality	= comcerto_i2c_functionality,
};

static struct i2c_adapter comcerto_i2c_adapter = {
	.name		= "comcerto_i2c",
	.owner		= THIS_MODULE,
	.algo		= &comcerto_i2c_algo,
	.timeout	= 0,	/* <= zero means that we calculate timeout in run-time, can be changed with ioctl call */
	.retries	= 0,	/* no retries by default - let the user decide what's the best, can be changed with ioctl call */
};

static int comcerto_i2c_probe(struct platform_device *pdev)
{
	struct comcerto_i2c *i2c;
	struct resource *irq;
	int res = -1;

	dev_dbg(&pdev->dev, "%s\n", __FUNCTION__);
	
	/* Put the I2C device Out-Of-Reset*/
	c2000_block_reset(COMPONENT_AXI_I2C,0);

	/* Clock divider configuration, get the i2c clock*/
	clk_i2c = clk_get(NULL, "spi_i2c");
	if (IS_ERR(clk_i2c)){
		pr_err("%s: Unable to obtain i2c clock: %ld\n", __func__, PTR_ERR(clk_i2c));
		return PTR_ERR(clk_i2c);
	}

	/*Enable the i2c clock */
	res = clk_enable(clk_i2c); 
	if (res){
		pr_err("%s: i2c clock failed to enable:\n", __func__);
		goto err0;
	}

	i2c = kzalloc(sizeof(*i2c), GFP_KERNEL);
	if (i2c == NULL) {
		dev_err(&pdev->dev, "%s: failed allocate memory\n", __FUNCTION__);
		res = -ENOMEM;
		goto err0;
	}

	i2c->adapter = &comcerto_i2c_adapter;
	i2c->adapter->dev.parent = &pdev->dev;
	i2c->dev = &pdev->dev;

	init_waitqueue_head(&i2c->wait);

	platform_set_drvdata(pdev, i2c);
	i2c_set_adapdata(&comcerto_i2c_adapter, i2c);

	i2c->io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (i2c->io == NULL) {
		dev_err(&pdev->dev, "%s: no IO region specified\n", __FUNCTION__);
		res = -ENOENT;
		goto err1;
	}

	if (!request_mem_region(i2c->io->start, i2c->io->end - i2c->io->start + 1, "I2C")) {
		dev_err(i2c->dev, "%s: failed to request memory region\n", __FUNCTION__);
		goto err1;
	}

	i2c->membase = APB_VADDR(i2c->io->start);

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq == NULL && !force_poll) {
		dev_warn(i2c->dev, "%s: no IRQ specified in resources, polling mode forced\n", __FUNCTION__);
		force_poll = 1;
		i2c->irq = -1;
	}

	if (speed == 0) {
		i2c->speed_khz = SPEED_NORMAL_KHZ;
	}
	else if (speed == 1) {
		i2c->speed_khz = SPEED_FULL_KHZ;
	}
	else if (speed == 2) {
		i2c->speed_khz = SPEED_HIGH_KHZ;
	}
	else {
		dev_err(i2c->dev, "%s: invalid 'speed' module option provided (%d, must be 0,1,2 for normal/full/high modes)\n",
			__FUNCTION__, speed);
		goto err2;
	}


	comcerto_i2c_reset(i2c);

	if (!force_poll) {
		i2c->irq = irq->start;

		res = request_irq(i2c->irq, comcerto_i2c_interrupt, IRQF_SHARED, "I2C", i2c);
		if (res < 0) {
			dev_warn(i2c->dev, "%s: failed to request IRQ%d, polling mode forced\n", __FUNCTION__, i2c->irq);
			force_poll = 1;
			i2c->irq = -1;
		}
	}
	else
		i2c->irq = -1;

	if (i2c_add_numbered_adapter(&comcerto_i2c_adapter) != 0) {
		dev_err(i2c->dev, "%s: failed to add I2C adapter\n", __FUNCTION__);
		goto err3;
	}

	dev_dbg(&pdev->dev, "%s: I2C adapter registered\n", __FUNCTION__);

	return 0;

err3:
	if (i2c->irq >= 0)
		free_irq(i2c->irq, i2c);

err2:
	release_mem_region(i2c->io->start, i2c->io->end - i2c->io->end + 1);

err1:
	kfree(i2c);

err0:
	return res;
}

static int comcerto_i2c_remove(struct platform_device *pdev)
{
	struct comcerto_i2c *i2c = platform_get_drvdata(pdev);

	dev_dbg(i2c->dev, "%s\n", __FUNCTION__);

	platform_set_drvdata(pdev, NULL);

	i2c_del_adapter(i2c->adapter);

	if (i2c->irq >= 0)
		free_irq(i2c->irq, i2c);

	release_mem_region(i2c->io->start, i2c->io->end - i2c->io->start + 1);

	kfree(i2c);

	/* Disable the Clock */
	clk_disable(clk_i2c);
	clk_put(clk_i2c);

	/* Put the I2C device in reset state*/
	c2000_block_reset(COMPONENT_AXI_I2C,1);

	return 0;
}

#ifdef CONFIG_PM
static int comcerto_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* No need to suspend client drivers here. Because clients are
	 * children, client drivers get suspended before adapter driver
	*/
	
	/* So do the Clock disable here , This clock is depends upon Legacy SPI*/
	clk_disable(clk_i2c);
	return 0;
}
	
static int comcerto_i2c_resume(struct platform_device *pdev)
{
	int ret;
	/* No need to suspend client drivers here. Because clients are
	 * children, client drivers get suspended before adapter driver
	*/

	/* So do the Clock Enable here , This clock is depends upon Legacy SPI*/
	ret = clk_enable(clk_i2c);
	if (ret)
	{
		pr_err("%s: I2C clock enable failed \n",__func__);
		return ret;
	}
	return 0;
}
#endif

static struct platform_driver comcerto_i2c_driver = {
	.driver = {
		.name	= "comcerto_i2c",
		.owner	= THIS_MODULE,
	},
	.probe	= comcerto_i2c_probe,
	.remove	= comcerto_i2c_remove,
#ifdef CONFIG_PM
	.suspend = comcerto_i2c_suspend,
	.resume  = comcerto_i2c_resume,
#endif
};

static int __init comcerto_i2c_init(void)
{
	if (platform_driver_register(&comcerto_i2c_driver)) {
		return -1;
	}

	return 0;
}

static void __exit comcerto_i2c_exit(void)
{
	platform_driver_unregister(&comcerto_i2c_driver);
}

module_init(comcerto_i2c_init);
module_exit(comcerto_i2c_exit);

