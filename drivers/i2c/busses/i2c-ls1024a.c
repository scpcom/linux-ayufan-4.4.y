/*
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct ls1024a_i2c_dev {
	struct device		*dev;
	struct i2c_adapter	adapter;
	void __iomem		*membase;
	int			irq;
	struct clk		*clk;
	wait_queue_head_t	wait;
	struct i2c_msg		*msg;
	int			msg_state;
	int			msg_status;	/* < 0: error, == 0: success, > 0: message in progress */
	int			msg_len;
	int			msg_retries;
};

#define SPEED_NORMAL_KHZ	100

#define COMCERTO_I2C_ADDR		(0x00*4)
#define COMCERTO_I2C_DATA		(0x01*4)
#define COMCERTO_I2C_CNTR		(0x02*4)
#define COMCERTO_I2C_STAT		(0x03*4)
#define COMCERTO_I2C_CCRFS		(0x03*4)
#define COMCERTO_I2C_XADDR		(0x04*4)
#define COMCERTO_I2C_CCRH		(0x05*4)
#define COMCERTO_I2C_RESET		(0x07*4)

/* CNTR - Control register bits */
#define CNTR_IEN			(1<<7)
#define CNTR_ENAB			(1<<6)
#define CNTR_STA			(1<<5)
#define CNTR_STP			(1<<4)
#define CNTR_IFLG			(1<<3)
#define CNTR_AAK			(1<<2)

/* STAT - Status codes */
#define STAT_BUS_ERROR			0x00	/* Bus error in master mode only */
#define STAT_START			0x08	/* Start condition transmitted */
#define STAT_START_REPEATED		0x10	/* Repeated Start condition transmited */
#define STAT_ADDR_WR_ACK		0x18	/* Address + Write bit transmitted, ACK received */
#define STAT_ADDR_WR_NACK		0x20	/* Address + Write bit transmitted, NACK received */
#define STAT_DATA_WR_ACK		0x28	/* Data byte transmitted in master mode , ACK received */
#define STAT_DATA_WR_NACK		0x30	/* Data byte transmitted in master mode , NACK received */
#define STAT_ARBIT_LOST			0x38	/* Arbitration lost in address or data byte */
#define STAT_ADDR_RD_ACK		0x40	/* Address + Read bit transmitted, ACK received  */
#define STAT_ADDR_RD_NACK		0x48	/* Address + Read bit transmitted, NACK received  */
#define STAT_DATA_RD_ACK		0x50	/* Data byte received in master mode, ACK transmitted  */
#define STAT_DATA_RD_NACK		0x58	/* Data byte received in master mode, NACK transmitted*/
#define STAT_ARBIT_LOST_ADDR		0x68	/* Arbitration lost in address  */
#define STAT_GENERAL_CALL		0x70	/* General Call, ACK transmitted */
#define STAT_NO_RELEVANT_INFO		0xF8	/* No relevant status information, IFLF=0 */

#define REG_ADDR(i2c, offset)		((i2c)->membase + (offset))
#define RD_REG(i2c, offset)		readb(REG_ADDR(i2c, offset))
#define WR_REG(i2c, offset, byte)	writeb(byte, REG_ADDR(i2c, offset))
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

static u8 comcerto_i2c_calculate_dividers(struct ls1024a_i2c_dev *i2c)
{
	int m, n, hz, speed_hz;
	int saved_n, saved_m, saved_hz;
	u8 dividers;
	unsigned int i2c_clk;

	/* Get the i2c clock rate */
	i2c_clk = clk_get_rate(i2c->clk);

	speed_hz = SPEED_NORMAL_KHZ * 1000;
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
static int comcerto_i2c_calculate_timeout(struct ls1024a_i2c_dev *i2c, struct i2c_msg *msg)
{
	int timeout;

	/* if no timeout was specified, calculate it */
	if (i2c->adapter.timeout <= 0) {
		if (i2c->irq >= 0) {
			/* for the interrupt mode calculate timeout for 'full' message */
			timeout = ((int)msg->len) * 10;	/* convert approx. to bits */
			timeout /= SPEED_NORMAL_KHZ;	/* convert to bits per ms (note of kHz scale) */
			timeout += timeout >> 1;	/* add 50% */
			timeout = timeout*HZ / 1000;	/* convert to jiffies */
			if (timeout < HZ / 5)		/* at least 200ms */
				timeout = HZ / 5;
		}
		else
			timeout = HZ;			/* 1 second for the polling mode */
	}
	else
		timeout = i2c->adapter.timeout;

	return timeout;
}

/*
 * Initialize I2C core. Zero CNTR and DATA, try RESET. Short busy wait and check core status.
 * After that set dividers for choosen speed.
 */
static void comcerto_i2c_reset(struct ls1024a_i2c_dev *i2c)
{
	u8 status, dividers;

	dev_dbg(i2c->dev, "%s\n", __FUNCTION__);

	WR_CNTR(i2c, 0);
	WR_DATA(i2c, 0);
	WR_RESET(i2c, 1);

	udelay(10);

	status = RD_STAT(i2c);
	if (status != STAT_NO_RELEVANT_INFO)
		dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected status after reset: 0x%02x\n", __FUNCTION__, status);

	/* dividers should be placed in CCRH for high-sped mode and in CCRFS for standard/full modes */
	dividers = comcerto_i2c_calculate_dividers(i2c);
	WR_CCRFS(i2c, dividers);
}

static inline void comcerto_i2c_message_complete(struct ls1024a_i2c_dev *i2c, int status)
{
	WR_CNTR(i2c, CNTR_STP);

	i2c->msg_status = status;
}

static inline int comcerto_i2c_message_in_progress(struct ls1024a_i2c_dev *i2c)
{
	return i2c->msg_status > 0;
}

/*
 * Wait event. This function sleeps in polling mode, in interrupt
 * mode it enables IRQ from I2C core and exits immediately.
 */
static int comcerto_i2c_wait(struct ls1024a_i2c_dev *i2c, u8 cntr)
{
	cntr &= ~(CNTR_IFLG | CNTR_IEN);	/* clear both IFLG and IEN */

	if (i2c->irq < 0) {
		ulong jiffies_mark = jiffies + comcerto_i2c_calculate_timeout(i2c, i2c->msg);

		WR_CNTR(i2c, cntr);
		while ((RD_CNTR(i2c) & CNTR_IFLG) == 0) {
			if (need_resched())
				schedule();

			if (time_after(jiffies, jiffies_mark)) {
				dev_printk(KERN_DEBUG, i2c->dev, "%s: polling transfer timeout\n", __FUNCTION__);
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

static void comcerto_i2c_state_idle(struct ls1024a_i2c_dev *i2c, u8 *cntr)
{
	if (unlikely(i2c->msg->flags & I2C_M_NOSTART)) {
		i2c->msg_state = TR_ADDR_ACK;
	}
	else {
		*cntr = CNTR_STP|CNTR_STA;	/* SPT|STA to auto recover from bus error state transparently at the start of the transfer */
		i2c->msg_state = TR_START_ACK;
	}
}

static void comcerto_i2c_state_start_ack(struct ls1024a_i2c_dev *i2c, u8 *cntr)
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
		dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected state (%#x) on start phase, %s\n",
			__FUNCTION__, status, i2c->msg_retries > 1 ? "retrying":"aborting");

		if (--i2c->msg_retries < 0)
			comcerto_i2c_message_complete(i2c, -1);
		else
			comcerto_i2c_state_idle(i2c, cntr);
	}
}

static void comcerto_i2c_rx(struct ls1024a_i2c_dev *i2c)
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
			dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected state (%#x) on address phase, %s\n",
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
			dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected state (%#x) on read phase\n", __FUNCTION__, status);
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
			dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected state (%#x) on finishing read phase\n", __FUNCTION__, status);
			comcerto_i2c_message_complete(i2c, -1);
		}
	}

	/* no wait if we completed message */
	if (comcerto_i2c_message_in_progress(i2c))
		comcerto_i2c_wait(i2c, cntr);
}

static void comcerto_i2c_tx(struct ls1024a_i2c_dev *i2c)
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
			dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected state (%#x) on address phase, %s\n",
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
			dev_printk(KERN_DEBUG, i2c->dev, "%s: unexpected state (%#x) on read data phase\n", __FUNCTION__, status);
			comcerto_i2c_message_complete(i2c, -1);
		}
		break;
	}

	if (comcerto_i2c_message_in_progress(i2c))
		comcerto_i2c_wait(i2c, cntr);
}

static irqreturn_t comcerto_i2c_interrupt(int irq, void *dev_id)
{
	struct ls1024a_i2c_dev *i2c = dev_id;

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

static void comcerto_i2c_message_process(struct ls1024a_i2c_dev *i2c, struct i2c_msg *msg)
{
	i2c->msg = msg;
	i2c->msg_state = TR_IDLE;
	i2c->msg_status = 1;
	i2c->msg_retries = i2c->adapter.retries;

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
				dev_printk(KERN_DEBUG, i2c->dev, "%s: interrupt transfer timeout\n", __FUNCTION__);
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
	struct ls1024a_i2c_dev *i2c = i2c_get_adapdata(adapter);
	int i;

	dev_dbg(i2c->dev, "%s: %d messages to process\n", __FUNCTION__, num);

	for (i = 0; i < num; i++) {
		dev_dbg(i2c->dev, "%s: message #%d: addr=%#x, flags=%#x, len=%u\n", __FUNCTION__,
			i, msgs[i].addr, msgs[i].flags, msgs[i].len);

		comcerto_i2c_message_process(i2c, &msgs[i]);

		if (i2c->msg_status < 0) {
			dev_printk(KERN_DEBUG, i2c->dev, "%s: transfer failed on message #%d (addr=%#x, flags=%#x, len=%u)\n",
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

static struct i2c_algorithm ls1024a_i2c_algo = {
	.master_xfer	= comcerto_i2c_master_xfer,
	.functionality	= comcerto_i2c_functionality,
};

static int ls1024a_i2c_probe(struct platform_device *pdev)
{
	struct ls1024a_i2c_dev *priv;
	struct resource *res;
	int err = 0;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->membase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->membase))
		return PTR_ERR(priv->membase);

	priv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	clk_prepare_enable(priv->clk);

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq <= 0) {
		dev_err(&pdev->dev, "invalid irq\n");
		return priv->irq;
	}

	err = devm_request_irq(&pdev->dev, priv->irq, comcerto_i2c_interrupt, 0,
			       pdev->name, priv);
	if (err) {
		dev_err(&pdev->dev, "IRQ request failed\n");
		return err;
	}

	init_waitqueue_head(&priv->wait);
	priv->adapter.dev.parent = &pdev->dev;
	priv->adapter.algo = &ls1024a_i2c_algo;
	priv->adapter.dev.of_node = pdev->dev.of_node;
	priv->dev = &pdev->dev;

	snprintf(priv->adapter.name, sizeof(priv->adapter.name), "ls1024a-i2c");
	i2c_set_adapdata(&priv->adapter, priv);

	comcerto_i2c_reset(priv);

	err = i2c_add_adapter(&priv->adapter);
	if (err) {
		dev_err(&pdev->dev, "failed to add I2C adapter\n");
		return err;
	}

	platform_set_drvdata(pdev, priv);
	dev_dbg(&pdev->dev, "I2C bus:%d added\n", priv->adapter.nr);

	return 0;
}

static int ls1024a_i2c_remove(struct platform_device *pdev)
{
	struct ls1024a_i2c_dev *priv;

	priv = platform_get_drvdata(pdev);
	synchronize_irq(priv->irq);
	i2c_del_adapter(&priv->adapter);

	return 0;
}

static const struct of_device_id ls1024a_i2c_of_match[] = {
	{ .compatible = "fsl,ls1024a-i2c", },
	{ /* sentinel */ },
};

static struct platform_driver ls1024a_i2c_driver = {
	.probe = ls1024a_i2c_probe,
	.remove = ls1024a_i2c_remove,
	.driver = {
		.name = "ls1024a-i2c",
		.of_match_table = ls1024a_i2c_of_match,
	},
};

module_platform_driver(ls1024a_i2c_driver);

MODULE_DESCRIPTION("Freescale LS1024A I2C Bus Controller Driver");
MODULE_LICENSE("GPL v2");
