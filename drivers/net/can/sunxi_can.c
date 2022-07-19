/*
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 */

#include <linux/netdevice.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/can/led.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include "sunxi_can_asm.h"

#define DRV_NAME "sun8i-can"

struct sunxican_priv {
	struct can_priv can;
	void __iomem *base;
	void __iomem *iomem_t;
	spinlock_t cmdreg_lock;	/* lock for concurrent cmd register writes */
	bool is_suspend;
	struct pinctrl *can_pinctrl;
	spinlock_t lock;
	int num;
};

static const struct can_bittiming_const sunxican_bittiming_const = {
	.name = DRV_NAME,
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 1,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 64,
	.brp_inc = 1,
};

static void sunxi_can_write_cmdreg(struct sunxican_priv *priv, u8 val)
{
	unsigned long flags;

	spin_lock_irqsave(&priv->cmdreg_lock, flags);
	can_asm_write_cmdreg(val, priv->base, priv->iomem_t);
	spin_unlock_irqrestore(&priv->cmdreg_lock, flags);
}

static int set_normal_mode(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int retry = SUNXI_MODE_MAX_RETRIES;
	u32 mod_reg_val = 0;

	do {
		mod_reg_val = readl(priv->base + SUNXI_REG_MSEL_ADDR);
		mod_reg_val &= ~SUNXI_MSEL_RESET_MODE;
		writel(mod_reg_val, priv->base + SUNXI_REG_MSEL_ADDR);
	} while (retry-- && (mod_reg_val & SUNXI_MSEL_RESET_MODE));

	if (readl(priv->base + SUNXI_REG_MSEL_ADDR) & SUNXI_MSEL_RESET_MODE) {
		netdev_err(dev,
			   "setting controller into normal mode failed!\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int set_reset_mode(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int retry = SUNXI_MODE_MAX_RETRIES;
	u32 mod_reg_val = 0;

	do {
		mod_reg_val = readl(priv->base + SUNXI_REG_MSEL_ADDR);
		mod_reg_val |= SUNXI_MSEL_RESET_MODE;
		writel(mod_reg_val, priv->base + SUNXI_REG_MSEL_ADDR);
	} while (retry-- && !(mod_reg_val & SUNXI_MSEL_RESET_MODE));

	if (!(readl(priv->base + SUNXI_REG_MSEL_ADDR) &
	      SUNXI_MSEL_RESET_MODE)) {
		netdev_err(dev, "setting controller into reset mode failed!\n");
		return -ETIMEDOUT;
	}

	return 0;
}

/* bittiming is called in reset_mode only */
static int sunxican_set_bittiming(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct can_bittiming *bt = &priv->can.bittiming;
	u32 cfg;

	cfg = ((bt->brp - 1) & 0x3FF) |
	     (((bt->sjw - 1) & 0x3) << 14) |
	     (((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) << 16) |
	     (((bt->phase_seg2 - 1) & 0x7) << 20);

	can_asm_set_bittiming(priv->base, priv->can.ctrlmode, &cfg, priv->iomem_t);

	netdev_dbg(dev, "setting BITTIMING=0x%08x\n", cfg);

	return 0;
}

static int sunxican_get_berr_counter(const struct net_device *dev,
				     struct can_berr_counter *bec)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	can_asm_fun1(priv->num);
	can_asm_fun2(priv->num);
	can_asm_clean_transfer_err(priv->base, &bec->txerr, &bec->rxerr);
	return 0;
}

static int sunxi_can_start(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int err;

	/* we need to enter the reset mode */
	err = set_reset_mode(dev);
	if (err) {
		netdev_err(dev, "could not enter reset mode\n");
		return err;
	}

	can_asm_start(priv->base, priv->can.ctrlmode, priv->num);

	err = sunxican_set_bittiming(dev);
	if (err)
		return err;

	/* we are ready to enter the normal mode */
	err = set_normal_mode(dev);
	if (err) {
		netdev_err(dev, "could not enter normal mode\n");
		return err;
	}

	priv->can.state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

static int sunxi_can_stop(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int err;

	priv->can.state = CAN_STATE_STOPPED;
	/* we need to enter reset mode */
	err = set_reset_mode(dev);
	if (err) {
		netdev_err(dev, "could not enter reset mode\n");
		return err;
	}

	/* disable all interrupts */
	writel(0, priv->base + SUNXI_REG_INTEN_ADDR);

	return 0;
}

static int sunxican_set_mode(struct net_device *dev, enum can_mode mode)
{
	int err;

	switch (mode) {
	case CAN_MODE_START:
		err = sunxi_can_start(dev);
		if (err) {
			netdev_err(dev, "starting CAN controller failed!\n");
			return err;
		}
		if (netif_queue_stopped(dev))
			netif_wake_queue(dev);
		break;

	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

/* transmit a CAN message
 * message layout in the sk_buff should be like this:
 * xx xx xx xx         ff         ll 00 11 22 33 44 55 66 77
 * [ can_id ] [flags] [len] [can data (up to 8 bytes]
 */
static netdev_tx_t sunxican_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	u8 dlc;
	u32 dreg, msg_flag_n;
	canid_t id;

	if (can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;

	netif_stop_queue(dev);


	id = cf->can_id;
	dlc = cf->can_dlc;
	msg_flag_n = dlc;

	can_asm_start_xmit(priv->base, priv->iomem_t, id, &msg_flag_n, &dreg, cf->data);

	can_put_echo_skb(skb, dev, 0);

	if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK)
		sunxi_can_write_cmdreg(priv, SUNXI_CMD_SELF_RCV_REQ);
	else
		sunxi_can_write_cmdreg(priv, SUNXI_CMD_TRANS_REQ);

	return NETDEV_TX_OK;
}

static void sunxi_can_rx(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u8 fi = 0;
	u32 dreg = 0;
	canid_t id = 0;
	int i;

	/* create zero'ed CAN frame buffer */
	skb = alloc_can_skb(dev, &cf);
	if (!skb)
		return;

	can_asm_rx(priv->base, &fi, &dreg, &id);

	cf->can_dlc = get_can_dlc(fi & 0x0F);

	/* remote frame ? */
	if (fi & SUNXI_MSG_RTR_FLAG)
		id |= CAN_RTR_FLAG;
	else
		for (i = 0; i < cf->can_dlc; i++)
			cf->data[i] = readl(priv->base + dreg + i * 4);

	cf->can_id = id;

	sunxi_can_write_cmdreg(priv, SUNXI_CMD_RELEASE_RBUF);

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;
	netif_rx(skb);

	can_led_event(dev, CAN_LED_EVENT_RX);
}

static int sunxi_can_err(struct net_device *dev, u8 isrc, u8 status)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	enum can_state state = priv->can.state;
	enum can_state rx_state, tx_state;
	u16 rxerr, txerr;
	u32 ecc, alc;

	/* we don't skip if alloc fails because we want the stats anyhow */
	skb = alloc_can_err_skb(dev, &cf);

	can_asm_clean_transfer_err(priv->base, &txerr, &rxerr);

	if (skb) {
		cf->data[6] = txerr;
		cf->data[7] = rxerr;
	}

	if (isrc & SUNXI_INT_DATA_OR) {
		/* data overrun interrupt */
		netdev_dbg(dev, "data overrun interrupt\n");
		if (likely(skb)) {
			cf->can_id |= CAN_ERR_CRTL;
			cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		}
		stats->rx_over_errors++;
		stats->rx_errors++;

		/* reset the CAN IP by entering reset mode
		 * ignoring timeout error
		 */
		set_reset_mode(dev);
		set_normal_mode(dev);

		/* clear bit */
		sunxi_can_write_cmdreg(priv, SUNXI_CMD_CLEAR_OR_FLAG);
	}
	if (isrc & SUNXI_INT_ERR_WRN) {
		/* error warning interrupt */
		netdev_dbg(dev, "error warning interrupt\n");

		if (status & SUNXI_STA_BUS_OFF)
			state = CAN_STATE_BUS_OFF;
		else if (status & SUNXI_STA_ERR_STA)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_ACTIVE;
	}
	if (isrc & SUNXI_INT_BUS_ERR) {
		/* bus error interrupt */
		netdev_dbg(dev, "bus error interrupt\n");
		priv->can.can_stats.bus_error++;
		stats->rx_errors++;

		if (likely(skb)) {
			ecc = readl(priv->base + SUNXI_REG_STA_ADDR);

			cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

			switch (ecc & SUNXI_STA_MASK_ERR) {
			case SUNXI_STA_BIT_ERR:
				cf->data[2] |= CAN_ERR_PROT_BIT;
				break;
			case SUNXI_STA_FORM_ERR:
				cf->data[2] |= CAN_ERR_PROT_FORM;
				break;
			case SUNXI_STA_STUFF_ERR:
				cf->data[2] |= CAN_ERR_PROT_STUFF;
				break;
			default:
				cf->data[3] = (ecc & SUNXI_STA_ERR_SEG_CODE)
					       >> 16;
				break;
			}
			/* error occurred during transmission? */
			if ((ecc & SUNXI_STA_ERR_DIR) == 0)
				cf->data[2] |= CAN_ERR_PROT_TX;
		}
	}
	if (isrc & SUNXI_INT_ERR_PASSIVE) {
		/* error passive interrupt */
		netdev_dbg(dev, "error passive interrupt\n");
		if (state == CAN_STATE_ERROR_PASSIVE)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_PASSIVE;
	}
	if (isrc & SUNXI_INT_ARB_LOST) {
		/* arbitration lost interrupt */
		netdev_dbg(dev, "arbitration lost interrupt\n");
		alc = readl(priv->base + SUNXI_REG_STA_ADDR);
		priv->can.can_stats.arbitration_lost++;
		stats->tx_errors++;
		if (likely(skb)) {
			cf->can_id |= CAN_ERR_LOSTARB;
			cf->data[0] = (alc >> 8) & 0x1f;
		}
	}

	if (state != priv->can.state) {
		tx_state = txerr >= rxerr ? state : 0;
		rx_state = txerr <= rxerr ? state : 0;

		if (likely(skb))
			can_change_state(dev, cf, tx_state, rx_state);
		else
			priv->can.state = state;
		if (state == CAN_STATE_BUS_OFF)
			can_bus_off(dev);
	}

	if (likely(skb)) {
		stats->rx_packets++;
		stats->rx_bytes += cf->can_dlc;
		netif_rx(skb);
	} else {
		return -ENOMEM;
	}

	return 0;
}

static irqreturn_t sunxi_can_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	u8 isrc, status;
	int n = 0;

	while ((isrc = readl(priv->base + SUNXI_REG_INT_ADDR)) &&
	       (n < SUNXI_CAN_MAX_IRQ)) {
		n++;
		status = readl(priv->base + SUNXI_REG_STA_ADDR);

		if (isrc & SUNXI_INT_WAKEUP)
			netdev_warn(dev, "wakeup interrupt\n");

		if (isrc & SUNXI_INT_TBUF_VLD) {
			/* transmission complete interrupt */
			stats->tx_bytes +=
			    readl(priv->base +
				  SUNXI_REG_RBUF_RBACK_START_ADDR) & 0xf;
			stats->tx_packets++;
			can_get_echo_skb(dev, 0);
			netif_wake_queue(dev);
			can_led_event(dev, CAN_LED_EVENT_TX);
		}
		if ((isrc & SUNXI_INT_RBUF_VLD) &&
		    !(isrc & SUNXI_INT_DATA_OR)) {
			/* receive interrupt - don't read if overrun occurred */
			while (status & SUNXI_STA_RBUF_RDY) {
				/* RX buffer is not empty */
				sunxi_can_rx(dev);
				status = readl(priv->base + SUNXI_REG_STA_ADDR);
			}
		}
		if (isrc &
		    (SUNXI_INT_DATA_OR | SUNXI_INT_ERR_WRN | SUNXI_INT_BUS_ERR |
		     SUNXI_INT_ERR_PASSIVE | SUNXI_INT_ARB_LOST)) {
			/* error interrupt */
			if (sunxi_can_err(dev, isrc, status))
				netdev_err(dev, "can't allocate buffer - clearing pending interrupts\n");
		}
		/* clear interrupts */
		writel(isrc, priv->base + SUNXI_REG_INT_ADDR);
		readl(priv->base + SUNXI_REG_INT_ADDR);
	}
	if (n >= SUNXI_CAN_MAX_IRQ)
		netdev_dbg(dev, "%d messages handled in ISR", n);

	return (n) ? IRQ_HANDLED : IRQ_NONE;
}

static int sunxican_open(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int err;

	/* common open */
	err = open_candev(dev);
	if (err)
		return err;

	/* register interrupt handler */
	err = request_irq(dev->irq, sunxi_can_interrupt, 0, dev->name, dev);
	if (err) {
		netdev_err(dev, "request_irq err: %d\n", err);
		goto exit_irq;
	}

	can_asm_fun0(priv->num);
	can_asm_fun1(priv->num);
	can_asm_fun2(priv->num);

	err = sunxi_can_start(dev);
	if (err) {
		netdev_err(dev, "could not start CAN peripheral\n");
		goto exit_can_start;
	}

	can_led_event(dev, CAN_LED_EVENT_OPEN);
	netif_start_queue(dev);

	return 0;

exit_can_start:
	free_irq(dev->irq, dev);
exit_irq:
	close_candev(dev);
	return err;
}

static int sunxican_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	sunxi_can_stop(dev);
	free_irq(dev->irq, dev);
	close_candev(dev);
	can_led_event(dev, CAN_LED_EVENT_STOP);

	return 0;
}

static const struct net_device_ops sunxican_netdev_ops = {
	.ndo_open = sunxican_open,
	.ndo_stop = sunxican_close,
	.ndo_start_xmit = sunxican_start_xmit,
};

static const struct of_device_id sunxican_of_match[] = {
	{.compatible = "allwinner,sun8i-can"},
	{},
};

MODULE_DEVICE_TABLE(of, sunxican_of_match);

static int can_request_gpio(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct sunxican_priv *priv = netdev_priv(dev);
	struct pinctrl_state *pctrl_state = NULL;
	int ret = 0;

	priv->can_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(priv->can_pinctrl)) {
		netdev_err(dev, "request pinctrl handle fail!\n");
		return -EINVAL;
	}

	pctrl_state = pinctrl_lookup_state(priv->can_pinctrl,
			PINCTRL_STATE_DEFAULT);
	if (IS_ERR(pctrl_state)) {
		netdev_err(dev, "pinctrl_lookup_state fail! return %p\n",
				pctrl_state);
		return -EINVAL;
	}

	ret = pinctrl_select_state(priv->can_pinctrl, pctrl_state);
	if (ret < 0)
		netdev_err(dev, "pinctrl_select_state fail! return %d\n", ret);

	return ret;
}

static void can_release_gpio(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct sunxican_priv *priv = netdev_priv(dev);

	if (!IS_ERR_OR_NULL(priv->can_pinctrl))
		devm_pinctrl_put(priv->can_pinctrl);
	priv->can_pinctrl = NULL;
}

static int sunxican_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct sunxican_priv *priv = netdev_priv(dev);
	iounmap(priv->base);
	can_release_gpio(pdev);
	unregister_netdev(dev);
	free_candev(dev);
	return 0;
}

static int sunxican_probe(struct platform_device *pdev)
{
	void __iomem *addr;
	int err, irq;
	struct net_device *dev;
	struct sunxican_priv *priv;
	struct device_node *node;
	int len, num;
	struct property *pp;

	dev_info(&pdev->dev, "can driver start probe\n");

	node = pdev->dev.of_node;
	pp = of_find_property(node, "id", &len);
	num = of_read_number(pp->value, len / 4);

	irq = can_asm_probe(node, num);
	if (irq < 0) {
		err = -ENODEV;
		dev_err(&pdev->dev, "could not get irq\n");
		goto exit;
	}

	addr = can_asm_fun5(num);
	if (IS_ERR(addr)) {
		err = -EBUSY;
		dev_err(&pdev->dev,
			"could not get addr for CAN device\n");
		goto exit;
	}

	dev = alloc_candev(sizeof(struct sunxican_priv), 1);
	if (!dev) {
		dev_err(&pdev->dev,
			"could not allocate memory for CAN device\n");
		err = -ENOMEM;
		goto exit;
	}

	dev->netdev_ops = &sunxican_netdev_ops;
	dev->irq = irq;
	dev->flags |= IFF_ECHO;

	priv = netdev_priv(dev);
	priv->can.clock.freq = 24000000;
	priv->can.bittiming_const = &sunxican_bittiming_const;
	priv->can.do_set_mode = sunxican_set_mode;
	priv->can.do_get_berr_counter = sunxican_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_BERR_REPORTING |
				       CAN_CTRLMODE_LISTENONLY |
				       CAN_CTRLMODE_LOOPBACK |
				       CAN_CTRLMODE_3_SAMPLES;
	priv->base = addr;
	priv->num = num;
	spin_lock_init(&priv->cmdreg_lock);
	spin_lock_init(&priv->lock);

	platform_set_drvdata(pdev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	can_request_gpio(pdev);
	err = register_candev(dev);
	if (err) {
		dev_err(&pdev->dev, "registering %s failed (err=%d)\n",
			DRV_NAME, err);
		goto exit_free;
	}
	devm_can_led_init(dev);

	dev_info(&pdev->dev, "can driver probe ok ...\n");
	return 0;

exit_free:
	free_candev(dev);
exit:
	return err;
}

#ifdef CONFIG_PM
static int can_select_gpio_state(struct pinctrl *pctrl, char *state)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, state);
	if (IS_ERR(pctrl_state)) {
		pr_err("can pinctrl_lookup_state(%s) failed!\n", state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		pr_err("can pinctrl_select_state(%s) failed!\n", state);

	return ret;
}

static int can_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxican_priv *priv = netdev_priv(ndev);

	if (!ndev || !netif_running(ndev))
		return 0;

	spin_lock(&priv->lock);
	priv->is_suspend = true;
	spin_unlock(&priv->lock);

	can_select_gpio_state(priv->can_pinctrl, PINCTRL_STATE_SLEEP);
	sunxican_close(ndev);

	return 0;
}

static int can_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sunxican_priv *priv = netdev_priv(ndev);

	if (!ndev || !netif_running(ndev))
		return 0;

	spin_lock(&priv->lock);
	priv->is_suspend = false;
	spin_unlock(&priv->lock);

	can_select_gpio_state(priv->can_pinctrl, PINCTRL_STATE_DEFAULT);
	sunxican_open(ndev);

	return 0;
}

static const struct dev_pm_ops can_pm_ops = {
	.suspend = can_suspend,
	.resume = can_resume,
};
#else
static const struct dev_pm_ops can_pm_ops;
#endif /* CONFIG_PM */
static struct platform_driver sunxi_can_driver = {
	.driver = {
		.name = DRV_NAME,
		.pm = &can_pm_ops,
		.of_match_table = sunxican_of_match,
	},
	.probe = sunxican_probe,
	.remove = sunxican_remove,
};

module_platform_driver(sunxi_can_driver);

MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("CAN driver for Allwinner SoCs");
