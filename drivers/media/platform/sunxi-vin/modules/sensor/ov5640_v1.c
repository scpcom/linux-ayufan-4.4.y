/*
 * A V4L2 driver for ov5640 YUV cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Yang Feng <yangfeng@allwinnertech.com>
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

MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for ov5640 sensors");
MODULE_LICENSE("GPL");

#define AF_WIN_NEW_COORD

#define MCLK              (24*1000*1000)
int MCLK_DIV = 1;
#ifdef CONFIG_ARCH_SUN9IW1P1
int A80_VERSION;
#endif

#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR 0x5640

#define SENSOR_NAME "ov5640"

#ifdef _FLASH_FUNC_
#include "../modules/flash/flash.h"
unsigned int to_flash;
static unsigned int flash_auto_level = 0x1c;
#endif
#define CONTINUEOUS_AF

#define DENOISE_LV_AUTO
#define SHARPNESS 0x18

#ifdef AUTO_FPS

#endif

#ifndef DENOISE_LV_AUTO
#define DENOISE_LV 0x8
#endif

#define AE_CW 1

unsigned int night_mode;
unsigned int Nfrms = 1;
unsigned int cap_manual_gain = 0x10;
#define CAP_GAIN_CAL 0
#define CAP_MULTI_FRAMES
#ifdef CAP_MULTI_FRAMES
#define MAX_FRM_CAP 4
#else
#define MAX_FRM_CAP 1
#endif

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The ov5640 sits on i2c with ID 0x78
 */
#define I2C_ADDR 0x78

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};



/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_vga_regs[] = {
	/* VGA(YUV) 30fps */
	{0x3103,  0x11},
	{0x3008,  0x82},
	{REG_DLY, 0x1e},
	{0x3008,  0x42},
	{0x3103,  0x03},
	{0x3017,  0xff},
	{0x3018,  0xff},
	{0x3034,  0x1a},
	{0x3035,  0x11},
	{0x3036,  0x46},
	{0x3037,  0x13},
	{0x3108,  0x01},
	{0x3630,  0x36},
	{0x3631,  0x0e},
	{0x3632,  0xe2},
	{0x3633,  0x12},
	{0x3621,  0xe0},
	{0x3704,  0xa0},
	{0x3703,  0x5a},
	{0x3715,  0x78},
	{0x3717,  0x01},
	{0x370b,  0x60},
	{0x3705,  0x1a},
	{0x3905,  0x02},
	{0x3906,  0x10},
	{0x3901,  0x0a},
	{0x3731,  0x12},
	{0x3600,  0x08},
	{0x3601,  0x33},
	{0x302d,  0x60},
	{0x3620,  0x52},
	{0x371b,  0x20},
	{0x471c,  0x50},
	{0x3a13,  0x43},
	{0x3a18,  0x00},
	{0x3a19,  0xf8},
	{0x3635,  0x13},
	{0x3636,  0x03},
	{0x3634,  0x40},
	{0x3622,  0x01},
	{0x3c01,  0x34},
	{0x3c04,  0x28},
	{0x3c05,  0x98},
	{0x3c06,  0x00},
	{0x3c07,  0x08},
	{0x3c08,  0x00},
	{0x3c09,  0x1c},
	{0x3c0a,  0x9c},
	{0x3c0b,  0x40},
	{0x3820,  0x41},
	{0x3821,  0x07},
	{0x3814,  0x31},
	{0x3815,  0x31},
	{0x3800,  0x00},
	{0x3801,  0x00},
	{0x3802,  0x00},
	{0x3803,  0x04},
	{0x3804,  0x0a},
	{0x3805,  0x3f},
	{0x3806,  0x07},
	{0x3807,  0x9b},
	{0x3808,  0x02},
	{0x3809,  0x80},
	{0x380a,  0x01},
	{0x380b,  0xe0},
	{0x380c,  0x07},
	{0x380d,  0x68},
	{0x380e,  0x03},
	{0x380f,  0xd8},
	{0x3810,  0x00},
	{0x3811,  0x10},
	{0x3812,  0x00},
	{0x3813,  0x06},
	{0x3618,  0x00},
	{0x3612,  0x29},
	{0x3708,  0x64},
	{0x3709,  0x52},
	{0x370c,  0x03},
	{0x3a02,  0x03},
	{0x3a03,  0xd8},
	{0x3a08,  0x01},
	{0x3a09,  0x27},
	{0x3a0a,  0x00},
	{0x3a0b,  0xf6},
	{0x3a0e,  0x03},
	{0x3a0d,  0x04},
	{0x3a14,  0x03},
	{0x3a15,  0xd8},
	{0x4001,  0x02},
	{0x4004,  0x02},
	{0x3000,  0x00},
	{0x3002,  0x1c},
	{0x3004,  0xff},
	{0x3006,  0xc3},
	{0x300e,  0x58},
	{0x302e,  0x00},
	{0x4300,  0x30},
	{0x501f,  0x00},
	{0x4713,  0x03},
	{0x4407,  0x04},
	{0x440e,  0x00},
	{0x460b,  0x35},
	{0x460c,  0x22},
	{0x4837,  0x22},
	{0x3824,  0x02},
	{REG_DLY, 0x05},
	{0x5000,  0xa7},
	{0x5001,  0xa3},
	{0x5180,  0xff},
	{0x5181,  0xf2},
	{0x5182,  0x00},
	{0x5183,  0x14},
	{0x5184,  0x25},
	{0x5185,  0x24},
	{0x5186,  0x09},
	{0x5187,  0x09},
	{0x5188,  0x09},
	{0x5189,  0x75},
	{0x518a,  0x54},
	{0x518b,  0xe0},
	{0x518c,  0xb2},
	{0x518d,  0x42},
	{0x518e,  0x3d},
	{0x518f,  0x56},
	{0x5190,  0x46},
	{0x5191,  0xf8},
	{0x5192,  0x04},
	{0x5193,  0x70},
	{0x5194,  0xf0},
	{0x5195,  0xf0},
	{0x5196,  0x03},
	{0x5197,  0x01},
	{0x5198,  0x04},
	{0x5199,  0x12},
	{0x519a,  0x04},
	{0x519b,  0x00},
	{0x519c,  0x06},
	{0x519d,  0x82},
	{0x519e,  0x38},
	{0x5381,  0x1e},
	{0x5382,  0x5b},
	{0x5383,  0x08},
	{0x5384,  0x0a},
	{0x5385,  0x7e},
	{0x5386,  0x88},
	{0x5387,  0x7c},
	{0x5388,  0x6c},
	{0x5389,  0x10},
	{0x538a,  0x01},
	{0x538b,  0x98},
	{0x5300,  0x08},
	{0x5301,  0x30},
	{0x5302,  0x10},
	{0x5303,  0x00},
	{0x5304,  0x08},
	{0x5305,  0x30},
	{0x5306,  0x08},
	{0x5307,  0x16},
	{0x5309,  0x08},
	{0x530a,  0x30},
	{0x530b,  0x04},
	{0x530c,  0x06},
	{0x5480,  0x01},
	{0x5481,  0x08},
	{0x5482,  0x14},
	{0x5483,  0x28},
	{0x5484,  0x51},
	{0x5485,  0x65},
	{0x5486,  0x71},
	{0x5487,  0x7d},
	{0x5488,  0x87},
	{0x5489,  0x91},
	{0x548a,  0x9a},
	{0x548b,  0xaa},
	{0x548c,  0xb8},
	{0x548d,  0xcd},
	{0x548e,  0xdd},
	{0x548f,  0xea},
	{0x5490,  0x1d},
	{0x5580,  0x02},
	{0x5583,  0x40},
	{0x5584,  0x10},
	{0x5589,  0x10},
	{0x558a,  0x00},
	{0x558b,  0xf8},
	{0x5800,  0x23},
	{0x5801,  0x14},
	{0x5802,  0x0f},
	{0x5803,  0x0f},
	{0x5804,  0x12},
	{0x5805,  0x26},
	{0x5806,  0x0c},
	{0x5807,  0x08},
	{0x5808,  0x05},
	{0x5809,  0x05},
	{0x580a,  0x08},
	{0x580b,  0x0d},
	{0x580c,  0x08},
	{0x580d,  0x03},
	{0x580e,  0x00},
	{0x580f,  0x00},
	{0x5810,  0x03},
	{0x5811,  0x09},
	{0x5812,  0x07},
	{0x5813,  0x03},
	{0x5814,  0x00},
	{0x5815,  0x01},
	{0x5816,  0x03},
	{0x5817,  0x08},
	{0x5818,  0x0d},
	{0x5819,  0x08},
	{0x581a,  0x05},
	{0x581b,  0x06},
	{0x581c,  0x08},
	{0x581d,  0x0e},
	{0x581e,  0x29},
	{0x581f,  0x17},
	{0x5820,  0x11},
	{0x5821,  0x11},
	{0x5822,  0x15},
	{0x5823,  0x28},
	{0x5824,  0x46},
	{0x5825,  0x26},
	{0x5826,  0x08},
	{0x5827,  0x26},
	{0x5828,  0x64},
	{0x5829,  0x26},
	{0x582a,  0x24},
	{0x582b,  0x22},
	{0x582c,  0x24},
	{0x582d,  0x24},
	{0x582e,  0x06},
	{0x582f,  0x22},
	{0x5830,  0x40},
	{0x5831,  0x42},
	{0x5832,  0x24},
	{0x5833,  0x26},
	{0x5834,  0x24},
	{0x5835,  0x22},
	{0x5836,  0x22},
	{0x5837,  0x26},
	{0x5838,  0x44},
	{0x5839,  0x24},
	{0x583a,  0x26},
	{0x583b,  0x28},
	{0x583c,  0x42},
	{0x583d,  0xce},
	{0x5025,  0x00},
	{0x3a0f,  0x30},
	{0x3a10,  0x28},
	{0x3a1b,  0x30},
	{0x3a1e,  0x26},
	{0x3a11,  0x60},
	{0x3a1f,  0x14},
	{0x3008,  0x02},
};

#ifdef AUTO_FPS
static struct regval_list sensor_auto_fps_mode[] = {

};


static struct regval_list sensor_fix_fps_mode[] = {

};
#endif

static struct regval_list sensor_oe_disable_regs[] = {

};

static struct regval_list sensor_oe_enable_regs[] = {

};

static struct regval_list sensor_sw_stby_on_regs[] = {

};

static struct regval_list sensor_sw_stby_off_regs[] = {

};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0x4300, 0x30},
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0x4300, 0x31},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0x4300, 0x33},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x4300, 0x32},
};

static struct regval_list sensor_fmt_raw[] = {
	{0x4300, 0x00},
};

static data_type current_lum = 0xff;
static data_type sensor_get_lum(struct v4l2_subdev *sd)
{
	sensor_read(sd, 0x56a1, &current_lum);
	sensor_dbg("check luminance=0x%x\n", current_lum);
	return current_lum;
}

/* stuff about exposure when capturing image and video*/
data_type ogain, oexposurelow, oexposuremid, oexposurehigh;
unsigned int preview_exp_line, preview_fps;
unsigned long preview_pclk;

#ifdef _FLASH_FUNC_
void check_to_flash(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);

	if (info->flash_mode == V4L2_FLASH_LED_MODE_FLASH) {
		to_flash = 1;
	} else if (info->flash_mode == V4L2_FLASH_LED_MODE_AUTO) {
		sensor_get_lum(sd);
		if (current_lum < flash_auto_level)
			to_flash = 1;
		else
			to_flash = 0;
	} else {
		to_flash = 0;
	}

	sensor_dbg("to_flash=%d\n", to_flash);
}
#endif

static int sensor_s_release_af(struct v4l2_subdev *sd)
{

	sensor_print("sensor_s_release_af\n");
	//sensor_write(sd, 0x3022, 0x08);
	return 0;
}

static int sensor_s_af_zone(struct v4l2_subdev *sd,
			    struct v4l2_win_coordinate *win_c)
{
#if 0
	struct sensor_info *info = to_state(sd);
	int ret;

	int x1, y1, x2, y2;
	unsigned int xc, yc;
	unsigned int prv_x, prv_y;

	sensor_print("sensor_s_af_zone\n");

	if (info->width == 0 || info->height == 0) {
		sensor_err("current width or height is zero!\n");
		return -EINVAL;
	}

	prv_x = (int)info->width;
	prv_y = (int)info->height;

	x1 = win_c->x1;
	y1 = win_c->y1;
	x2 = win_c->x2;
	y2 = win_c->y2;

#ifdef AF_WIN_NEW_COORD
	xc = prv_x * ((unsigned int)(2000 + x1 + x2) / 2) / 2000;
	yc = (prv_y * ((unsigned int)(2000 + y1 + y2) / 2) / 2000);
#else
	xc = (x1 + x2) / 2;
	yc = (y1 + y2) / 2;
#endif

	sensor_dbg("af zone input xc=%d,yc=%d\n", xc, yc);

	if (x1 > x2 || y1 > y2 || xc > info->width || yc > info->height) {
		sensor_dbg("invalid af win![%d,%d][%d,%d] prv[%d/%d]\n", x1,
			    y1, x2, y2, prv_x, prv_y);
		return -EINVAL;
	}

	if (info->focus_status == 1)
		return 0;

	xc = (xc * 80 * 2 / info->width + 1) / 2;
	if ((info->width == HD720_WIDTH && info->height == HD720_HEIGHT) ||
	    (info->width == HD1080_WIDTH && info->height == HD1080_HEIGHT)) {
		yc = (yc * 45 * 2 / info->height + 1) / 2;
	} else {
		yc = (yc * 60 * 2 / info->height + 1) / 2;
	}

	sensor_dbg("af zone after xc=%d,yc=%d\n", xc, yc);

	ret = sensor_write(sd, 0x3024, xc);
	if (ret < 0) {
		sensor_err("sensor_s_af_zone_xc error!\n");
		return ret;
	}

	ret = sensor_write(sd, 0x3025, yc);
	if (ret < 0) {
		sensor_err("sensor_s_af_zone_yc error!\n");
		return ret;
	}

	ret = sensor_write(sd, 0x3023, 0x01);

	ret |= sensor_write(sd, 0x3022, 0x81);
	if (ret < 0) {
		sensor_err("sensor_s_af_zone error!\n");
		return ret;
	}

	sensor_s_relaunch_af_zone(sd);
#endif
	return 0;
}

/* **********************begin of ******************************* */

int ov5640_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
#if 0
	int exp_val, gain_val, frame_length, shutter;
	unsigned char explow = 0, expmid = 0, exphigh = 0;
	unsigned char gainlow = 0, gainhigh = 0;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	sensor_print("exp_val %d, gain_val %d\n", exp_val, gain_val);

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x3);

	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)((0x0000ff & exp_val));
	shutter = exp_val / 16;


	if (shutter > ov5640_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = ov5640_sensor_vts;


	sensor_write(sd, 0x3208, 0x00);

	sensor_write(sd, 0x3503, 0x07);

	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));

	sensor_write(sd, 0x350b, gainlow);
	sensor_write(sd, 0x350a, gainhigh);

	sensor_write(sd, 0x3502, explow);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3500, exphigh);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);
	info->exp = exp_val;
	info->gain = gain_val;
#endif
	return 0;
}

/* ********************************end of ******************************* */

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;
#ifdef _FLASH_FUNC_
	struct modules_config *modules = sd_to_modules(sd);
#endif
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
#ifdef _FLASH_FUNC_
		io_set_flash_ctrl(modules->modules.flash.sd,
				  SW_CTRL_FLASH_OFF);
#endif
		sensor_s_release_af(sd);
		ret =
		    sensor_write_array(sd, sensor_sw_stby_on_regs,
				       ARRAY_SIZE(sensor_sw_stby_on_regs));
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		sensor_print("disalbe oe!\n");
		ret =
		    sensor_write_array(sd, sensor_oe_disable_regs,
				       ARRAY_SIZE(sensor_oe_disable_regs));
		if (ret < 0)
			sensor_err("disalbe oe falied!\n");

		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK / MCLK_DIV);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		sensor_print("enable oe!\n");
		ret =
		    sensor_write_array(sd, sensor_oe_enable_regs,
				       ARRAY_SIZE(sensor_oe_enable_regs));
		if (ret < 0)
			sensor_err("enable oe falied!\n");
		ret =
		    sensor_write_array(sd, sensor_sw_stby_off_regs,
				       ARRAY_SIZE(sensor_sw_stby_off_regs));
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(10000, 12000);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK / MCLK_DIV);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
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
	unsigned int SENSOR_ID = 0;
	data_type val;

	sensor_read(sd, 0x300a, &val);
	SENSOR_ID |= (val << 8);
	sensor_read(sd, 0x300b, &val);
	SENSOR_ID |= (val);
	sensor_print("V4L2_IDENT_SENSOR = %x\n", SENSOR_ID);

	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return -ENODEV;

	return 0;

}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init 0x%x\n", val);

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	ogain = 0x28;
	oexposurelow = 0x00;
	oexposuremid = 0x3d;
	oexposurehigh = 0x00;
	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 0;
	info->height = 0;
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp_bias = 0;
	info->autoexp = 1;
	info->autowb = 1;
	info->wb = V4L2_WHITE_BALANCE_AUTO;
	info->clrfx = V4L2_COLORFX_NONE;
	info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;


	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	ret =
		sensor_write_array(sd, sensor_default_regs,
				   ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	info->preview_first_flag = 1;
	night_mode = 0;
	Nfrms = MAX_FRM_CAP;

	return 0;
}

static int sensor_g_exif(struct v4l2_subdev *sd,
			 struct sensor_exif_attribute *exif)
{
	int ret = 0;

	exif->fnumber = 220;
	exif->focal_length = 180;
	exif->brightness = 125;
	exif->flash_fire = 0;
	exif->iso_speed = 200;
	exif->exposure_time_num = 1;
	exif->exposure_time_den = 15;
	return ret;
}
static void sensor_s_af_win(struct v4l2_subdev *sd,
			    struct v4l2_win_setting *af_win)
{
	sensor_s_af_zone(sd, &af_win->coor);
}
static void sensor_s_ae_win(struct v4l2_subdev *sd,
			    struct v4l2_win_setting *ae_win)
{

}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	case GET_SENSOR_EXIF:
		sensor_g_exif(sd, (struct sensor_exif_attribute *)arg);
		break;
	case SET_AUTO_FOCUS_WIN:
		sensor_s_af_win(sd, (struct v4l2_win_setting *)arg);
		break;
	case SET_AUTO_EXPOSURE_WIN:
		sensor_s_ae_win(sd, (struct v4l2_win_setting *)arg);
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
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
		.desc = "YUYV 4:2:2",
		.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
		.regs = sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp = 2,
	}, {
		.desc = "YVYU 4:2:2",
		.mbus_code = MEDIA_BUS_FMT_YVYU8_2X8,
		.regs = sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp = 2,
	}, {
		.desc = "UYVY 4:2:2",
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.regs = sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp = 2,
	}, {
		.desc = "VYUY 4:2:2",
		.mbus_code = MEDIA_BUS_FMT_VYUY8_2X8,
		.regs = sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp = 2,
	}, {
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8,
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
	/* VGA */
	{
	 .width = VGA_WIDTH,
	 .height = VGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 640,
	 .vts = 480,
	 .pclk = 9216 * 1000,
	 .fps_fixed = 1,
	 .bin_factor = 1,
	 .intg_min = 1,
	 .intg_max = 480 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 10 << 4,
	 .regs = sensor_vga_regs,
	 .regs_size = ARRAY_SIZE(sensor_vga_regs),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_get_mbus_config(struct v4l2_subdev *sd,
				unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *****************************begin of ******************************* */
static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

#ifdef _FLASH_FUNC_
	struct modules_config *modules = sd_to_modules(sd);
#endif
	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	ov5640_sensor_vts = wsize->vts;
	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}



/* ----------------------------------------------------------------------- */

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
	info->af_first_flag = 1;
	info->auto_focus = 0;
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	sensor_print("sensor_remove ov5640 sd = %p!\n", sd);
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
#ifdef CONFIG_ARCH_SUN9IW1P1
	A80_VERSION = sunxi_get_soc_ver();
	if (A80_VERSION >= SUN9IW1P1_REV_B)
		MCLK_DIV = 1;
	else
		MCLK_DIV = 2;

	sensor_print("A80_VERSION = %d , SUN9IW1P1_REV_B = %d, MCLK_DIV = %d\n",
	       A80_VERSION, SUN9IW1P1_REV_B, MCLK_DIV);
#else
	MCLK_DIV = 1;
#endif
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
