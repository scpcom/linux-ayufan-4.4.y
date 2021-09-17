#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <media/rc-core.h>
#include <linux/pinctrl/consumer.h>

struct st_rc_device {
	struct device			*dev;
	int				irq;
	int				irq_wake;
	struct clk			*sys_clock;
	void				*base;	 
	void				*rx_base; 
	struct pinctrl			*pinctrl;
	struct pinctrl_state		*pins_default;
	struct pinctrl_state		*pins_sleep;
	struct rc_dev			*rdev;
	bool				overclocking;
	int				sample_mult;
	int				sample_div;
	bool				rxuhfmode;
	struct	reset_control		*rstc;
};

#define IRB_SAMPLE_RATE_COMM	0x64	 
#define IRB_CLOCK_SEL		0x70	 
#define IRB_CLOCK_SEL_STATUS	0x74	 
 
#define IRB_RX_ON               0x40	 
#define IRB_RX_SYS              0X44	 
#define IRB_RX_INT_EN           0x48	 
#define IRB_RX_INT_STATUS       0x4c	 
#define IRB_RX_EN               0x50	 
#define IRB_MAX_SYM_PERIOD      0x54	 
#define IRB_RX_INT_CLEAR        0x58	 
#define IRB_RX_STATUS           0x6c	 
#define IRB_RX_NOISE_SUPPR      0x5c	 
#define IRB_RX_POLARITY_INV     0x68	 

#define IRB_RX_INTS		0x0f
#define IRB_RX_OVERRUN_INT	0x04
  
#define MAX_SYMB_TIME		0x5000
#define IRB_SAMPLE_FREQ		10000000
#define	IRB_FIFO_NOT_EMPTY	0xff00
#define IRB_OVERFLOW		0x4
#define IRB_TIMEOUT		0xffff
#define IR_ST_NAME "st-rc"

#define IR_INT_ENABLE(base) do {\
writel_relaxed(IRB_RX_INTS, (base) + IRB_RX_INT_EN); \
writel_relaxed(0x01, (base) + IRB_RX_EN); \
} while (0)

#define IR_INT_DISABLE(base) do {\
writel_relaxed(0x00, (base) + IRB_RX_EN); \
writel_relaxed(0x00, (base) + IRB_RX_INT_EN); \
} while (0)

static void st_rc_send_lirc_timeout(struct rc_dev *rdev)
{
	DEFINE_IR_RAW_EVENT(ev);
	ev.timeout = true;
	ir_raw_event_store(rdev, &ev);
}

static irqreturn_t st_rc_rx_interrupt(int irq, void *data)
{
	unsigned int symbol, mark = 0;
	struct st_rc_device *dev = data;
	int last_symbol = 0;
	u32 status;

	DEFINE_IR_RAW_EVENT(ev);

	if (dev->irq_wake)
		pm_wakeup_event(dev->dev, 0);

	status  = readl(dev->rx_base + IRB_RX_STATUS);

	while (status & (IRB_FIFO_NOT_EMPTY | IRB_OVERFLOW)) {
		u32 int_status = readl(dev->rx_base + IRB_RX_INT_STATUS);

		if (unlikely(int_status & IRB_RX_OVERRUN_INT)) {
			 
			ir_raw_event_reset(dev->rdev);
			dev_info(dev->dev, "IR RX overrun\n");
			writel(IRB_RX_OVERRUN_INT,
					dev->rx_base + IRB_RX_INT_CLEAR);
			continue;
		}

		symbol = readl(dev->rx_base + IRB_RX_SYS);
		mark = readl(dev->rx_base + IRB_RX_ON);

		if (symbol == IRB_TIMEOUT)
			last_symbol = 1;

		if ((mark > 2) && (symbol > 1)) {
			symbol -= mark;
			if (dev->overclocking) {  
				symbol *= dev->sample_mult;
				symbol /= dev->sample_div;
				mark *= dev->sample_mult;
				mark /= dev->sample_div;
			}

			ev.duration = US_TO_NS(mark);
			ev.pulse = true;
			ir_raw_event_store(dev->rdev, &ev);

			if (!last_symbol) {
				ev.duration = US_TO_NS(symbol);
				ev.pulse = false;
				ir_raw_event_store(dev->rdev, &ev);
			} else  {
				st_rc_send_lirc_timeout(dev->rdev);
			}

		}
		last_symbol = 0;
		status  = readl(dev->rx_base + IRB_RX_STATUS);
	}

	writel(IRB_RX_INTS, dev->rx_base + IRB_RX_INT_CLEAR);

	ir_raw_event_handle(dev->rdev);
	return IRQ_HANDLED;
}

static void st_rc_hardware_init(struct st_rc_device *dev)
{
	int baseclock, freqdiff;
	unsigned int rx_max_symbol_per = MAX_SYMB_TIME;
	unsigned int rx_sampling_freq_div;

	if (dev->rstc)
		reset_control_deassert(dev->rstc);

	clk_prepare_enable(dev->sys_clock);
	baseclock = clk_get_rate(dev->sys_clock);

	writel(1, dev->rx_base + IRB_RX_POLARITY_INV);

	rx_sampling_freq_div = baseclock / IRB_SAMPLE_FREQ;
	writel(rx_sampling_freq_div, dev->base + IRB_SAMPLE_RATE_COMM);

	freqdiff = baseclock - (rx_sampling_freq_div * IRB_SAMPLE_FREQ);
	if (freqdiff) {  
		dev->overclocking = true;
		dev->sample_mult = 1000;
		dev->sample_div = baseclock / (10000 * rx_sampling_freq_div);
		rx_max_symbol_per = (rx_max_symbol_per * 1000)/dev->sample_div;
	}

	writel(rx_max_symbol_per, dev->rx_base + IRB_MAX_SYM_PERIOD);

	IR_INT_DISABLE(dev->rx_base);
}

static int st_rc_remove(struct platform_device *pdev)
{
	struct st_rc_device *rc_dev = platform_get_drvdata(pdev);

	clk_disable_unprepare(rc_dev->sys_clock);
	rc_unregister_device(rc_dev->rdev);
	return 0;
}

static int st_rc_open(struct rc_dev *rdev)
{
	struct st_rc_device *dev = rdev->priv;
	unsigned long flags;

	local_irq_save(flags);
	 
	IR_INT_ENABLE(dev->rx_base);

	local_irq_restore(flags);

	return 0;
}

static void st_rc_close(struct rc_dev *rdev)
{
	struct st_rc_device *dev = rdev->priv;
	 
	IR_INT_DISABLE(dev->rx_base);
}

static int st_rc_probe(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct rc_dev *rdev;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct st_rc_device *rc_dev;
	struct device_node *np = pdev->dev.of_node;
	const char *rx_mode;

	rc_dev = devm_kzalloc(dev, sizeof(struct st_rc_device), GFP_KERNEL);

	if (!rc_dev)
		return -ENOMEM;

	rdev = rc_allocate_device();

	if (!rdev)
		return -ENOMEM;

	if (np && !of_property_read_string(np, "rx-mode", &rx_mode)) {

		if (!strcmp(rx_mode, "uhf")) {
			rc_dev->rxuhfmode = true;
		} else if (!strcmp(rx_mode, "infrared")) {
			rc_dev->rxuhfmode = false;
		} else {
			dev_err(dev, "Unsupported rx mode [%s]\n", rx_mode);
			goto err;
		}

	} else {
		goto err;
	}

	rc_dev->sys_clock = devm_clk_get(dev, NULL);
	if (IS_ERR(rc_dev->sys_clock)) {
		dev_err(dev, "System clock not found\n");
		ret = PTR_ERR(rc_dev->sys_clock);
		goto err;
	}

	rc_dev->irq = platform_get_irq(pdev, 0);
	if (rc_dev->irq < 0) {
		ret = rc_dev->irq;
		goto err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	rc_dev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(rc_dev->base)) {
		ret = PTR_ERR(rc_dev->base);
		goto err;
	}

	if (rc_dev->rxuhfmode)
		rc_dev->rx_base = rc_dev->base + 0x40;
	else
		rc_dev->rx_base = rc_dev->base;

	rc_dev->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(rc_dev->pinctrl))
		return PTR_ERR(rc_dev->pinctrl);

	rc_dev->pins_default = pinctrl_lookup_state(rc_dev->pinctrl,
						 PINCTRL_STATE_DEFAULT);
	if (IS_ERR(rc_dev->pins_default))
		dev_err(&pdev->dev, "could not get default pinstate\n");

	rc_dev->pins_sleep = pinctrl_lookup_state(rc_dev->pinctrl,
					       PINCTRL_STATE_SLEEP);
	if (IS_ERR(rc_dev->pins_sleep))
		dev_dbg(&pdev->dev, "could not get sleep pinstate\n");

	rc_dev->rstc = reset_control_get(dev, NULL);
	if (IS_ERR(rc_dev->rstc))
		rc_dev->rstc = NULL;

	rc_dev->dev = dev;
	platform_set_drvdata(pdev, rc_dev);
	st_rc_hardware_init(rc_dev);

	rdev->driver_type = RC_DRIVER_IR_RAW;
	rdev->allowed_protos = RC_BIT_ALL;
	 
	rdev->rx_resolution = 100;
	rdev->timeout = US_TO_NS(MAX_SYMB_TIME);
	rdev->priv = rc_dev;
	rdev->open = st_rc_open;
	rdev->close = st_rc_close;
	rdev->driver_name = IR_ST_NAME;
	rdev->map_name = RC_MAP_LIRC;
	rdev->input_name = "ST Remote Control Receiver";

	device_set_wakeup_capable(dev, true);

	ret = rc_register_device(rdev);
	if (ret < 0)
		goto clkerr;

	rc_dev->rdev = rdev;
	if (devm_request_irq(dev, rc_dev->irq, st_rc_rx_interrupt,
			IRQF_NO_SUSPEND, IR_ST_NAME, rc_dev) < 0) {
		dev_err(dev, "IRQ %d register failed\n", rc_dev->irq);
		ret = -EINVAL;
		goto rcerr;
	}

	st_rc_send_lirc_timeout(rdev);

	dev_info(dev, "setup in %s mode\n", rc_dev->rxuhfmode ? "UHF" : "IR");

	return ret;
rcerr:
	rc_unregister_device(rdev);
	rdev = NULL;
clkerr:
	clk_disable_unprepare(rc_dev->sys_clock);
err:
	rc_free_device(rdev);
	dev_err(dev, "Unable to register device (%d)\n", ret);
	return ret;
}

#ifdef MY_DEF_HERE
#ifdef CONFIG_PM_SLEEP
static int st_rc_suspend(struct device *dev)
{
	struct st_rc_device *rc_dev = dev_get_drvdata(dev);
	struct rc_dev *rdev = rc_dev->rdev;

	if (device_may_wakeup(dev)) {
		 
		if (!rdev->users) {
			dev_err(dev,
				"IR wake-up not available though requested");
			return -EINVAL;
		}

		if (!enable_irq_wake(rc_dev->irq))
			rc_dev->irq_wake = 1;
		else
			return -EINVAL;
	} else {

		if (!IS_ERR(rc_dev->pins_sleep))
			pinctrl_select_state(rc_dev->pinctrl,
					rc_dev->pins_sleep);

		IR_INT_DISABLE(rc_dev->rx_base);
		clk_disable_unprepare(rc_dev->sys_clock);
		if (rc_dev->rstc)
			reset_control_assert(rc_dev->rstc);
	}

	return 0;
}

static int st_rc_resume(struct device *dev)
{
	struct st_rc_device *rc_dev = dev_get_drvdata(dev);
	struct rc_dev *rdev = rc_dev->rdev;

	if (rc_dev->irq_wake) {
		disable_irq_wake(rc_dev->irq);
		rc_dev->irq_wake = 0;
	} else {
		if (!IS_ERR(rc_dev->pins_default))
			pinctrl_select_state(rc_dev->pinctrl,
					rc_dev->pins_default);
	}

	st_rc_hardware_init(rc_dev);
	if (rdev->users)
		IR_INT_ENABLE(rc_dev->rx_base);

	return 0;
}

static SIMPLE_DEV_PM_OPS(st_rc_pm_ops, st_rc_suspend, st_rc_resume);
#define DEV_PM_OPS	(&st_rc_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif
#else  
#ifdef CONFIG_PM
static int st_rc_suspend(struct device *dev)
{
	struct st_rc_device *rc_dev = dev_get_drvdata(dev);
	struct rc_dev *rdev = rc_dev->rdev;

	if (device_may_wakeup(dev)) {
		 
		if (!rdev->users) {
			dev_err(dev, "IR wake-up not available though requested");
			return -EINVAL;
		}

		if (!enable_irq_wake(rc_dev->irq))
			rc_dev->irq_wake = 1;
		else
			return -EINVAL;
	} else {

		if (!IS_ERR(rc_dev->pins_sleep))
			pinctrl_select_state(rc_dev->pinctrl,
					rc_dev->pins_sleep);

		IR_INT_DISABLE(rc_dev->rx_base);
		clk_disable_unprepare(rc_dev->sys_clock);
		if (rc_dev->rstc)
			reset_control_assert(rc_dev->rstc);
	}

	return 0;
}

static int st_rc_resume(struct device *dev)
{
	struct st_rc_device *rc_dev = dev_get_drvdata(dev);
	struct rc_dev *rdev = rc_dev->rdev;

	if (rc_dev->irq_wake) {
		disable_irq_wake(rc_dev->irq);
		rc_dev->irq_wake = 0;
	} else {
		if (!IS_ERR(rc_dev->pins_default))
			pinctrl_select_state(rc_dev->pinctrl,
					rc_dev->pins_default);
	}

	st_rc_hardware_init(rc_dev);
	if (rdev->users)
		IR_INT_ENABLE(rc_dev->rx_base);

	return 0;
}

static SIMPLE_DEV_PM_OPS(st_rc_pm_ops, st_rc_suspend, st_rc_resume);
#endif
#endif  

static void st_rc_shutdown(struct platform_device *pdev)
{
	struct st_rc_device *rc_dev = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev)) {
		enable_irq_wake(rc_dev->irq);
		IR_INT_ENABLE(rc_dev->rx_base);
	}
}

#ifdef CONFIG_OF
static struct of_device_id st_rc_match[] = {
	{ .compatible = "st,comms-irb", },
	{},
};

MODULE_DEVICE_TABLE(of, st_rc_match);
#endif

static struct platform_driver st_rc_driver = {
	.driver = {
		.name = IR_ST_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(st_rc_match),
#ifdef MY_DEF_HERE
		.pm     = DEV_PM_OPS,
#else  
#ifdef CONFIG_PM
		.pm     = &st_rc_pm_ops,
#endif
#endif  
	},
	.probe = st_rc_probe,
	.shutdown = st_rc_shutdown,
	.remove = st_rc_remove,
};

module_platform_driver(st_rc_driver);

MODULE_DESCRIPTION("RC Transceiver driver for STMicroelectronics platforms");
MODULE_AUTHOR("STMicroelectronics (R&D) Ltd");
MODULE_LICENSE("GPL");
