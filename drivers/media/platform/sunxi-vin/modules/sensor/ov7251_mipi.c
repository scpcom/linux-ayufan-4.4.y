/*
 * A V4L2 driver for ov7251_mipi cameras.
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng ZeQun <zequnzheng@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for ov7251 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x7750

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov7251_mipi sits on i2c with ID 0xC0
 */
#define I2C_ADDR 0xC0

#define SENSOR_NAME "ov7251_mipi"

#define USE_30FPS 0

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {

};

#if USE_30FPS
/* 640x480 Raw10 30fps 24MHz */
static struct regval_list sensor_VGA_regs[] = {
	{0X0103, 0X01},
	{0X0100, 0X00},
	{0X3005, 0X0A},
	{0X3012, 0Xc0},
	{0X3013, 0Xd2},
	{0X3014, 0X04},
	{0X3016, 0X10},
	{0X3017, 0X00},
	{0X3018, 0X00},
	{0X301a, 0X00},
	{0X301b, 0X00},
	{0X301c, 0X00},
	{0X3023, 0X05},
	{0X3037, 0Xf0},
	{0X3098, 0X04},
	{0X3099, 0X28},
	{0X309a, 0X05},
	{0X309b, 0X04},
	{0X30b0, 0X0a},
	{0X30b1, 0X01},
	{0X30b3, 0X4b},
	{0X30b4, 0X03},
	{0X30b5, 0X05},
	{0X3106, 0Xda},
	{0X3500, 0X00},
	{0X3501, 0X1f},
	{0X3502, 0X80},
	{0X3503, 0X07},
	{0X3509, 0X10},
	{0X350b, 0X10},
	{0X3600, 0X1c},
	{0X3602, 0X62},
	{0X3620, 0Xb7},
	{0X3622, 0X04},
	{0X3626, 0X21},
	{0X3627, 0X30},
	{0X3630, 0X44},
	{0X3631, 0X35},
	{0X3634, 0X60},
	{0X3636, 0X00},
	{0X3662, 0X01},
	{0X3663, 0X70},
	{0X3664, 0Xf0},
	{0X3666, 0X0a},
	{0X3669, 0X1a},
	{0X366a, 0X00},
	{0X366b, 0X50},
	{0X3673, 0X01},
	{0X3674, 0Xef},
	{0X3675, 0X03},
	{0X3705, 0Xc1},
	{0X3709, 0X40},
	{0X373c, 0X08},
	{0X3742, 0X00},
	{0X3757, 0Xb3},
	{0X3788, 0X00},
	{0X37a8, 0X01},
	{0X37a9, 0Xc0},
	{0X3800, 0X00},
	{0X3801, 0X04},
	{0X3802, 0X00},
	{0X3803, 0X04},
	{0X3804, 0X02},
	{0X3805, 0X8b},
	{0X3806, 0X01},
	{0X3807, 0Xeb},
	{0X3808, 0X02},
	{0X3809, 0X80},
	{0X380a, 0X01},
	{0X380b, 0Xe0},
	{0X380c, 0X03},
	{0X380d, 0Xa0},
	{0X380e, 0X06},
	{0X380f, 0Xb8},
	{0X3810, 0X00},
	{0X3811, 0X04},
	{0X3812, 0X00},
	{0X3813, 0X05},
	{0X3814, 0X11},
	{0X3815, 0X11},
	{0X3820, 0X40},
	{0X3821, 0X00},
	{0X382f, 0X0e},
	{0X3832, 0X00},
	{0X3833, 0X05},
	{0X3834, 0X00},
	{0X3835, 0X0c},
	{0X3837, 0X00},
	{0X3b80, 0X00},
	{0X3b81, 0Xff},
	{0X3b82, 0X10},
	{0X3b83, 0X00},
	{0X3b84, 0X08},
	{0X3b85, 0X00},
	{0X3b86, 0X01},
	{0X3b87, 0X00},
	{0X3b88, 0X00},
	{0X3b89, 0X00},
	{0X3b8a, 0X00},
	{0X3b8b, 0X00},
	{0X3b8c, 0X00},
	{0X3b8d, 0X00},
	{0X3b8e, 0X09},
	{0X3b8f, 0Xc4},
	{0X3b94, 0X05},
	{0X3b95, 0Xf2},
	{0X3b96, 0Xc0},
	{0X3c00, 0X89},
	{0X3c01, 0X63},
	{0X3c02, 0X01},
	{0X3c03, 0X00},
	{0X3c04, 0X00},
	{0X3c05, 0X03},
	{0X3c06, 0X00},
	{0X3c07, 0X06},
	{0X3c0c, 0X01},
	{0X3c0d, 0Xd0},
	{0X3c0e, 0X02},
	{0X3c0f, 0X0a},
	{0X4001, 0X42},
	{0X4004, 0X04},
	{0X4005, 0X00},
	{0X404e, 0X01},
	{0X4300, 0Xff},
	{0X4301, 0X00},
	{0X4501, 0X48},
	{0X4600, 0X00},
	{0X4601, 0X4e},
	{0X4801, 0X0f},
	{0X4806, 0X0f},
	{0X4819, 0Xaa},
	{0X4823, 0X3e},
	{0X4837, 0X19},
	{0X4a0d, 0X00},
	{0X4a47, 0X7f},
	{0X4a49, 0Xf0},
	{0X4a4b, 0X30},
	{0X5000, 0X87},
	{0X5001, 0X80},
	{0X0100, 0X01},
};
#endif

/* 640x480 Raw10 10fps 24MHz */
static struct regval_list sensor_VGA_10fps_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x3005, 0x0A},
	{0x3012, 0xc0},
	{0x3013, 0xd2},
	{0x3014, 0x04},
	{0x3016, 0x10},
	{0x3017, 0x00},
	{0x3018, 0x00},
	{0x301a, 0x00},
	{0x301b, 0x00},
	{0x301c, 0x00},
	{0x3023, 0x05},
	{0x3037, 0xf0},
	{0x3098, 0x04},
	{0x3099, 0x28},
	{0x309a, 0x05},
	{0x309b, 0x04},
	{0x30b0, 0x0a},
	{0x30b1, 0x01},
	{0x30b3, 0x64},
	{0x30b4, 0x03},
	{0x30b5, 0x05},
	{0x3106, 0xda},
	{0x3500, 0x00},
	{0x3501, 0x1f},
	{0x3502, 0x80},
	{0x3503, 0x07},
	{0x3509, 0x10},
	{0x350b, 0x10},
	{0x3600, 0x1c},
	{0x3602, 0x62},
	{0x3620, 0xb7},
	{0x3622, 0x04},
	{0x3626, 0x21},
	{0x3627, 0x30},
	{0x3630, 0x44},
	{0x3631, 0x35},
	{0x3634, 0x60},
	{0x3636, 0x00},
	{0x3662, 0x01},
	{0x3663, 0x70},
	{0x3664, 0xf0},
	{0x3666, 0x0a},
	{0x3669, 0x1a},
	{0x366a, 0x00},
	{0x366b, 0x50},
	{0x3673, 0x01},
	{0x3674, 0xef},
	{0x3675, 0x03},
	{0x3705, 0xc1},
	{0x3709, 0x40},
	{0x373c, 0x08},
	{0x3742, 0x00},
	{0x3757, 0xb3},
	{0x3788, 0x00},
	{0x37a8, 0x01},
	{0x37a9, 0xc0},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x02},
	{0x3805, 0x8b},
	{0x3806, 0x01},
	{0x3807, 0xeb},
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x03},
	{0x380d, 0xc0},
	{0x380e, 0x13},
	{0x380f, 0x8B},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x05},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x40},
	{0x3821, 0x00},
	{0x382f, 0x0e},
	{0x3832, 0x00},
	{0x3833, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x0c},
	{0x3837, 0x00},
	{0x3b80, 0x00},
	{0x3b81, 0xff},
	{0x3b82, 0x10},
	{0x3b83, 0x00},
	{0x3b84, 0x08},
	{0x3b85, 0x00},
	{0x3b86, 0x01},
	{0x3b87, 0x00},
	{0x3b88, 0x00},
	{0x3b89, 0x00},
	{0x3b8a, 0x00},
	{0x3b8b, 0x00},
	{0x3b8c, 0x00},
	{0x3b8d, 0x00},
	{0x3b8e, 0x09},
	{0x3b8f, 0xc4},
	{0x3b94, 0x05},
	{0x3b95, 0xf2},
	{0x3b96, 0xc0},
	{0x3c00, 0x89},
	{0x3c01, 0x63},
	{0x3c02, 0x01},
	{0x3c03, 0x00},
	{0x3c04, 0x00},
	{0x3c05, 0x03},
	{0x3c06, 0x00},
	{0x3c07, 0x06},
	{0x3c0c, 0x01},
	{0x3c0d, 0xd0},
	{0x3c0e, 0x02},
	{0x3c0f, 0x0a},
	{0x4001, 0x42},
	{0x4004, 0x04},
	{0x4005, 0x00},
	{0x404e, 0x01},
	{0x4300, 0xff},
	{0x4301, 0x00},
	{0x4311, 0x75},
	{0x4312, 0x30},
	{0x4501, 0x48},
	{0x4600, 0x00},
	{0x4601, 0x4e},
	{0x4801, 0x0f},
	{0x4806, 0x0f},
	{0x4819, 0xaa},
	{0x4823, 0x3e},
	{0x4837, 0x19},
	{0x4a0d, 0x00},
	{0x4a47, 0x7f},
	{0x4a49, 0xf0},
	{0x4a4b, 0x30},
	{0x5000, 0x87},
	{0x5001, 0x80},
	{0x0100, 0x01},
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */
static struct regval_list sensor_fmt_raw[] = {

};

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);

	return 0;
}

static int ov7251_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > ((ov7251_sensor_vts - 16) << 4))
		exp_val = (ov7251_sensor_vts - 16) << 4;
	if (exp_val < 16)
		exp_val = 16;

	exphigh = (unsigned char)((exp_val >> 16) & 0x0F);
	expmid = (unsigned char)((exp_val >> 8) & 0xFF);
	explow = (unsigned char)(exp_val & 0xFF);

	sensor_write(sd, 0x3500, exphigh);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3502, explow);

	sensor_dbg("sensor_s_exp info->exp %d\n", exp_val);
	info->exp = exp_val;

	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);

	return 0;
}

static int last_dgain;
static int sensor_s_gain(struct v4l2_subdev *sd, unsigned int gain_val)
{
	data_type gainlow;
	struct sensor_info *info = to_state(sd);
	unsigned int gain_dig;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	if (gain_val < 32) {
		gainlow = 0x10;
		gain_dig = gain_val << 6;
	} else if (gain_val < 64) {
		gainlow = 0x20;
		gain_dig = gain_val << 5;
	} else if (gain_val < 128) {
		gainlow = 0x42;
		gain_dig = gain_val << 4;
	} else {
		gainlow = 0x8A;
		gain_dig = gain_val << 3;
	}

	/* sensor_write(sd, 0x350a, gainhigh); */
	sensor_write(sd, 0x350b, gainlow);

	sensor_write(sd, 0x3400, last_dgain >> 8);
	sensor_write(sd, 0x3401, last_dgain & 0xff);

	sensor_write(sd, 0x3402, last_dgain >> 8);
	sensor_write(sd, 0x3403, last_dgain & 0xff);

	sensor_write(sd, 0x3404, last_dgain >> 8);
	sensor_write(sd, 0x3405, last_dgain & 0xff);

	last_dgain = gain_dig;

	sensor_dbg("sensor_s_gain info->gain %d %d\n", gain_val, last_dgain);
	info->gain = gain_val;

	return 0;
}

static int frame_cnt;
static int last_exp;
static int last_gain;
static unsigned int exp_duration;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	data_type duration_mid, duration_low;

	sensor_write(sd, 0x3208, 0x00);
	sensor_s_exp(sd, last_exp);
	sensor_s_gain(sd, last_gain);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);

	last_exp = exp_gain->exp_val;
	last_gain = exp_gain->gain_val;

	/* STROBE 20us */
	exp_duration = (info->exp >> 4) * 960 / 48 / 20;

	/* sync MCU */
	if (frame_cnt < 5) {
		sensor_print("%s frame_cnt %d\n", __func__, frame_cnt);
		frame_cnt++;
		exp_duration = 2500;
	}

	duration_mid = (unsigned char)((exp_duration >> 8) & 0xFF);
	duration_low = (unsigned char)(exp_duration & 0xFF);
	sensor_write(sd, 0x3b8e, duration_mid);
	sensor_write(sd, 0x3b8f, duration_low);

	/* vsync 20ns */
	/* exp_duration = exp_duration * 12; */

	duration_mid = (unsigned char)((exp_duration >> 8) & 0xFF);
	duration_low = (unsigned char)(exp_duration & 0xFF);
	sensor_write(sd, 0x4311, duration_mid);
	sensor_write(sd, 0x4312, duration_low);

	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);

	if (ret != 0)
		return ret;

	if (on_off == STBY_ON)
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
	else
		ret = sensor_write(sd, 0x0100, rdval | 0x01);

	return ret;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;

	ret = 0;
	switch (on) {
	case STBY_ON:
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);

		cci_lock(sd);
		/* inactive mclk after stadby in */
		vin_set_mclk(sd, OFF);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		cci_lock(sd);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);

		cci_unlock(sd);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(10000, 12000);

		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");

		cci_lock(sd);

		vin_gpio_set_status(sd, RESET, 1);

		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		usleep_range(30000, 31000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);

		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(30000, 31000);

		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(30000, 31000);

		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		cci_lock(sd);

		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);

		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);

		vin_gpio_set_status(sd, RESET, 0);

		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	sensor_print("%s val %d\n", __func__, val);

	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;

	sensor_read(sd, 0x300A, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR >> 8))
		return -ENODEV;

	sensor_read(sd, 0x300B, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR & 0xff))
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = VGA_WIDTH;
	info->height = VGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->exp = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = SENSOR_FRAME_RATE; /* 30 fps */
	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		ret = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
#if USE_30FPS
	{
		.width      = VGA_WIDTH,
		.height     = VGA_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 928,
		.vts        = 1720,
		.pclk       = 48 * 1000 * 1000,
		.mipi_bps   = 93 * 1000 * 1000,
		.fps_fixed  = 30,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (1720) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 16 << 4,
		.regs       = sensor_VGA_regs,
		.regs_size  = ARRAY_SIZE(sensor_VGA_regs),
		.set_size   = NULL,
	},
#endif
	{
		.width      = VGA_WIDTH,
		.height     = VGA_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 960,
		.vts        = 5003,
		.pclk       = 48 * 1000 * 1000,
		.mipi_bps   = 93 * 1000 * 1000,
		.fps_fixed  = 10,
		.bin_factor = 1,
		.intg_min   = 1 << 4,
		.intg_max   = (5003) << 4,
		.gain_min   = 1 << 4,
		.gain_max   = 256 << 4,
		.regs       = sensor_VGA_10fps_regs,
		.regs_size  = ARRAY_SIZE(sensor_VGA_10fps_regs),
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_get_mbus_config(struct v4l2_subdev *sd,
				unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_1_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
		container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
		container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	ov7251_sensor_vts = wsize->vts;
	info->exp = 0;
	info->gain = 0;

	frame_cnt = 0;

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable) {
		/* stream off */
		sensor_write(sd, 0x0100, 0x00);
		return 0;
	}

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_mbus_config = sensor_get_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static const struct v4l2_ctrl_config sensor_custom_ctrls[] = {
	{
		.ops = &sensor_ctrl_ops,
		.id = V4L2_CID_FRAME_RATE,
		.name = "frame rate",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 15,
		.max = 120,
		.step = 1,
		.def = 120,
	},
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16, 256 * 16, 1, 16);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 3 * 16, 65536 * 16, 1, 3 * 16);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);

	mutex_init(&info->lock);
#ifdef CONFIG_SAME_I2C
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = SENSOR_NAME,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
