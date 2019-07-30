/*
 * SARADC joystick driver for Linux(Hardkernel ODROIDGO-Advance)
 */

/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

/*
 * ADC generic resistive touchscreen (GRTS)
 * This is a generic input driver that connects to an ADC
 * given the channels in device tree, and reports events to the input
 * subsystem.
 *
 * Copyright (C) 2019 Hardkernel Co.,LTD
 *
 */
/*----------------------------------------------------------------------------*/
#include <linux/err.h>
#include <linux/iio/consumer.h>
#include <linux/iio/types.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>

/*----------------------------------------------------------------------------*/
#define DRIVER_NAME		"adc-joystick"

/* unit (mv) */
#define	DEFAULT_ABS_MAX		1800
#define	DEFAULT_ABS_MIN		0
#define	DEFAULT_THRESHOLD	2

/* unit (ms) */
#define	DEFAULT_POLL_INTERVAL	500

/*----------------------------------------------------------------------------*/
struct adc_joystick_state {
	/* saradc read channel : joy_x, joy_y */
	struct iio_channel	*channel[2];

	/* report threshold (mV) */
	int threshold;
	/* polled device read interval (ms) */
	int interval;

	/* report value (mV) */
	int old_x, old_y, x, y;
	/* input device init value (mV) */
	int abs_x_max, abs_x_min, abs_y_max, abs_y_min;
};

/*----------------------------------------------------------------------------*/
static void adc_joystick_poll(struct input_polled_dev *dev)
{
	struct adc_joystick_state *st = dev->private;

	if ((iio_read_channel_processed(st->channel[0], &st->x)) ||
	    (iio_read_channel_processed(st->channel[1], &st->y))) {
		pr_err("%s : read saradc channels!\n", __func__);
		return;
	}

	if ((abs(st->x - st->old_x) > st->threshold) ||
	    (abs(st->y - st->old_y) > st->threshold)) {
		/* report joystick event */
		input_report_abs(dev->input, ABS_X, st->x);
		input_report_abs(dev->input, ABS_Y, st->y);
		input_sync(dev->input);
		st->old_x = st->x;	st->old_y = st->y;
	}
}

/*----------------------------------------------------------------------------*/
static void adc_joystick_dt_parse(struct platform_device *pdev,
				struct adc_joystick_state *st)
{
	struct device *dev = &pdev->dev;

	if (device_property_read_u32(dev, "poll-interval", &st->interval))
		st->interval = DEFAULT_POLL_INTERVAL;

	if (device_property_read_u32(dev, "report-x-max", &st->abs_x_max))
		st->abs_x_max = DEFAULT_ABS_MAX;

	if (device_property_read_u32(dev, "report-y-max", &st->abs_y_max))
		st->abs_y_max = DEFAULT_ABS_MAX;

	if (device_property_read_u32(dev, "report-x-min", &st->abs_x_min))
		st->abs_x_min = DEFAULT_ABS_MIN;

	if (device_property_read_u32(dev, "report-y-min", &st->abs_y_min))
		st->abs_y_min = DEFAULT_ABS_MIN;

	if (device_property_read_u32(dev, "report-threshold", &st->threshold))
		st->threshold = DEFAULT_THRESHOLD;

	st->old_x = st->old_y = st->x = st->y = 0;
}

/*----------------------------------------------------------------------------*/
static int adc_joystick_register(struct platform_device *pdev,
				struct adc_joystick_state *st)
{
	struct device *dev = &pdev->dev;
	struct input_polled_dev *poll_dev;
	struct input_dev *input;
	int error;

	poll_dev = devm_input_allocate_polled_device(dev);
	if (!poll_dev) {
		dev_err(dev, "failed to allocate polled input device\n");
		return -ENOMEM;
	}

	poll_dev->poll = adc_joystick_poll;
	poll_dev->poll_interval = st->interval;
	poll_dev->private = st;

	input = poll_dev->input;

	input->phys = "adc-joystick/js0";
	input->name = pdev->name;

	input->evbit[0]   = BIT_MASK(EV_ABS);
	input->id.bustype = BUS_HOST;
	input->id.vendor  = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	input_set_abs_params(input, ABS_X, st->abs_x_min, st->abs_x_max, 0, 0);
	input_set_abs_params(input, ABS_Y, st->abs_y_min, st->abs_y_max, 0, 0);

	error = input_register_polled_device(poll_dev);
	if (error) {
		dev_err(dev, "Unable to register input device: %d\n", error);
		return error;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static int adc_joystick_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct adc_joystick_state *st;
	enum iio_chan_type type;
	int error, i;

	st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
	if (!st)
		return -ENOMEM;

	for (i = 0; i < 2; i++) {
		if (i)
			st->channel[i] = devm_iio_channel_get(dev, "joy_y");
		else
			st->channel[i] = devm_iio_channel_get(dev, "joy_x");

		if (IS_ERR(st->channel[i])) {
			dev_err(dev, "iio channel[%d] get error\n", i);
			return -EINVAL;
		}
		if (!st->channel[i]->indio_dev)
			return -ENXIO;

		if (iio_get_channel_type(st->channel[i], &type))
			return -EINVAL;

		if (type != IIO_VOLTAGE) {
			dev_err(dev, "Incompatible channel %d type %d\n", i, type);
			return -EINVAL;
		}
	}
	platform_set_drvdata(pdev, st);

	adc_joystick_dt_parse(pdev, st);

	if ((error = adc_joystick_register(pdev, st)))
		return error;

	dev_info(dev, "poll-interval    = %d\n", st->interval);
	dev_info(dev, "report-x-max     = %d\n", st->abs_x_max);
	dev_info(dev, "report-y-max     = %d\n", st->abs_y_max);
	dev_info(dev, "report-x-min     = %d\n", st->abs_x_min);
	dev_info(dev, "report-y-min     = %d\n", st->abs_y_min);
	dev_info(dev, "report-threshold = %d\n", st->threshold);
	dev_info(dev, "%s success\n", __func__);
	return 0;
}

/*----------------------------------------------------------------------------*/
static const struct of_device_id adc_joystick_of_match[] = {
	{ .compatible = "adc-joystick", },
	{},
};

MODULE_DEVICE_TABLE(of, adc_joystick_of_match);

/*----------------------------------------------------------------------------*/
static struct platform_driver adc_joystick_driver = {
	.probe = adc_joystick_probe,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(adc_joystick_of_match),
	},
};

/*----------------------------------------------------------------------------*/
static int __init adc_joystick_init(void)
{
	return platform_driver_register(&adc_joystick_driver);
}

/*----------------------------------------------------------------------------*/
static void __exit adc_joystick_exit(void)
{
	platform_driver_unregister(&adc_joystick_driver);
}

/*----------------------------------------------------------------------------*/
late_initcall(adc_joystick_init);
module_exit(adc_joystick_exit);

/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Hardkernel Co.,LTD");
MODULE_DESCRIPTION("SARADC Joystick Driver for ODROIDGO-Advance");
MODULE_LICENSE("GPL v2");

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
