/*
 * A V4L2 driver for imx386 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
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
MODULE_DESCRIPTION("A low-level driver for IMX278 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x0278

/*
 * Our nominal (default) frame rate.
 */

/*
 * settle time set 0xb0
 */

#define SENSOR_FRAME_RATE 30

/*
 * The imx386 i2c address
 */
#define I2C_ADDR 0x20

#define SENSOR_NUM 0x2
#define SENSOR_NAME "imx386_mipi"
#define SENSOR_NAME_2 "imx386_mipi_2"


#define  PIC_OFFSET_H  (16*12)  //16
#define  PIC_OFFSET_V  (16*12*3/4)
#define  VIDEO_OFFSET_H  (16*8)  //10
#define  VIDEO_OFFSET_V  (16*8*9/16)


/*
 * The default register settings
 */

static struct regval_list Imx386_default_regs[] = {
	{0x0100, 0x00},
	{0x0103, 0x01},
	{0xFFFF, 0x10},
	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x3A7D, 0x00},
	{0x3A7E, 0x02},
	{0x3A7F, 0x05},
	{0x3100, 0x00},
	{0x3101, 0x40},
	{0x3102, 0x00},
	{0x3103, 0x10},
	{0x3104, 0x01},
	{0x3105, 0xE8},
	{0x3106, 0x01},
	{0x3107, 0xF0},
	{0x3150, 0x04},
	{0x3151, 0x03},
	{0x3152, 0x02},
	{0x3153, 0x01},
	{0x5A86, 0x00},
	{0x5A87, 0x82},
	{0x5D1A, 0x00},
	{0x5D95, 0x02},
	{0x5E1B, 0x00},
	{0x5F5A, 0x00},
	{0x5F5B, 0x04},
	{0x682C, 0x31},
	{0x6831, 0x31},
	{0x6835, 0x0E},
	{0x6836, 0x31},
	{0x6838, 0x30},
	{0x683A, 0x06},
	{0x683B, 0x33},
	{0x683D, 0x30},
	{0x6842, 0x31},
	{0x6844, 0x31},
	{0x6847, 0x31},
	{0x6849, 0x31},
	{0x684D, 0x0E},
	{0x684E, 0x32},
	{0x6850, 0x31},
	{0x6852, 0x06},
	{0x6853, 0x33},
	{0x6855, 0x31},
	{0x685A, 0x32},
	{0x685C, 0x33},
	{0x685F, 0x31},
	{0x6861, 0x33},
	{0x6865, 0x0D},
	{0x6866, 0x33},
	{0x6868, 0x31},
	{0x686B, 0x34},
	{0x686D, 0x31},
	{0x6872, 0x32},
	{0x6877, 0x33},
	{0x7FF0, 0x01},
	{0x7FF4, 0x08},
	{0x7FF5, 0x3C},
	{0x7FFA, 0x01},
	{0x7FFD, 0x00},
	{0x831E, 0x00},
	{0x831F, 0x00},
	{0x9301, 0xBD},
	{0x9B94, 0x03},
	{0x9B95, 0x00},
	{0x9B96, 0x08},
	{0x9B97, 0x00},
	{0x9B98, 0x0A},
	{0x9B99, 0x00},
	{0x9BA7, 0x18},
	{0x9BA8, 0x18},
	{0x9D04, 0x08},
	{0x9D50, 0x8C},
	{0x9D51, 0x64},
	{0x9D52, 0x50},
	{0x9E31, 0x04},
	{0x9E32, 0x04},
	{0x9E33, 0x04},
	{0x9E34, 0x04},
	{0xA200, 0x00},
	{0xA201, 0x0A},
	{0xA202, 0x00},
	{0xA203, 0x0A},
	{0xA204, 0x00},
	{0xA205, 0x0A},
	{0xA206, 0x01},
	{0xA207, 0xC0},
	{0xA208, 0x00},
	{0xA209, 0xC0},
	{0xA20C, 0x00},
	{0xA20D, 0x0A},
	{0xA20E, 0x00},
	{0xA20F, 0x0A},
	{0xA210, 0x00},
	{0xA211, 0x0A},
	{0xA212, 0x01},
	{0xA213, 0xC0},
	{0xA214, 0x00},
	{0xA215, 0xC0},
	{0xA300, 0x00},
	{0xA301, 0x0A},
	{0xA302, 0x00},
	{0xA303, 0x0A},
	{0xA304, 0x00},
	{0xA305, 0x0A},
	{0xA306, 0x01},
	{0xA307, 0xC0},
	{0xA308, 0x00},
	{0xA309, 0xC0},
	{0xA30C, 0x00},
	{0xA30D, 0x0A},
	{0xA30E, 0x00},
	{0xA30F, 0x0A},
	{0xA310, 0x00},
	{0xA311, 0x0A},
	{0xA312, 0x01},
	{0xA313, 0xC0},
	{0xA314, 0x00},
	{0xA315, 0xC0},
	{0xBC19, 0x01},
	{0xBC1C, 0x0A},
	{0x0101, 0x00},
	{0x0101, 0x03},
	{0x300b, 0x01},
};

#if 0
static struct regval_list sensor_1080p60_regs[] = {

	{0x0100, 0x00},
	{0xFFFF, 0x01},
	{0x0112, 0x0A},
	{0x0113, 0x0A},

	{0x0301, 0x06},
	{0x0303, 0x02},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x6E},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x01},
	{0x030F, 0x72},
	{0x0310, 0x01},

	{0x0342, 0x08},
	{0x0343, 0xD0},

	{0x0340, 0x08},
	{0x0341, 0x76},

	{0x0344, 0x00},
	{0x0345, 0x60},
	{0x0346, 0x01},
	{0x0347, 0xAC},
	{0x0348, 0x0F},
	{0x0349, 0x5F},
	{0x034A, 0x0A},
	{0x034B, 0x1B},

	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x22},
	{0x300D, 0x00},
	{0x302E, 0x00},

	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x040C, 0x07},
	{0x040D, 0x80},
	{0x040E, 0x04},
	{0x040F, 0x38},

	{0x034C, 0x07},
	{0x034D, 0x80},
	{0x034E, 0x04},
	{0x034F, 0x38},

	{0x0114, 0x03},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x0902, 0x01},
	{0x3030, 0x00},
	{0x3031, 0x01},
	{0x3032, 0x00},
	{0x3047, 0x01},
	{0x3049, 0x00},
	{0x30E6, 0x00},
	{0x30E7, 0x00},
	{0x4E25, 0x80},
	{0x663A, 0x01},
	{0x9311, 0x3F},
	{0xA0CD, 0x0A},
	{0xA0CE, 0x0A},
	{0xA0CF, 0x0A},

	{0x0202, 0x08},
	{0x0203, 0x6C},

};
#endif
static struct regval_list reg_mipi_2lane_1080P60_10bit[] = {
	{0x0100, 0x00},
	{0xFFFF, 0x10},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0301, 0x06},
	{0x0303, 0x02},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x38}, //38 //6e
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x01}, //0x01   0x02
	{0x030F, 0x72}, //0x72   0xe4
	{0x0310, 0x01},
	{0x0342, 0x08},
	{0x0343, 0xD0},
	{0x0340, 0x04}, //8   //4
	{0x0341, 0x80}, //76  //50
	{0x0344, 0x00},
	{0x0345, 0x60},
	{0x0346, 0x01},
	{0x0347, 0xAC},
	{0x0348, 0x0F},
	{0x0349, 0x5F},
	{0x034A, 0x0A},
	{0x034B, 0x1B},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x22},
	{0x300D, 0x00},
	{0x302E, 0x00},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x040C, 0x07},
	{0x040D, 0x80},
	{0x040E, 0x04},
	{0x040F, 0x38},
	{0x034C, 0x07},
	{0x034D, 0x80},
	{0x034E, 0x04},
	{0x034F, 0x38},
	{0x0114, 0x01},  //0x03 lane num   0x01
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},

	{0x0902, 0x02},
	{0x3030, 0x00},
	{0x3031, 0x01},
	{0x3032, 0x00},
	{0x3047, 0x01},
	{0x3049, 0x00},
	{0x30E6, 0x00},
	{0x30E7, 0x00},
	{0x4E25, 0x80},
	{0x663A, 0x01},
	{0x9311, 0x3F},
	{0xA0CD, 0x0A},
	{0xA0CE, 0x0A},
	{0xA0CF, 0x0A},
	{0x0202, 0x04}, //8
	{0x0203, 0x36}, //6c
	{0x020e, 0x04},  //digital gain
	{0x020f, 0x00},
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

static int imx386_sensor_vts;
static int imx386_sensor_hts;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	exp_val = (exp_val+8)>>4;

	exphigh = (unsigned char) ((0xff00&exp_val)>>8);
	explow	= (unsigned char) ((0x00ff&exp_val));

	sensor_write(sd, 0x0203, explow);
	sensor_write(sd, 0x0202, exphigh);
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

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	data_type gainlow = 0;
	data_type gainhigh = 0;
	long gaindigi = 0;
	int gainana = 0;

	if (gain_val < 16) {
		gainana = 0;
		gaindigi = 256;
	} else if (gain_val <= 256) {
		gainana = 512 - 8192/gain_val;
		gaindigi = 256;
	} else {
		gainana = 480;
		gaindigi = gain_val;
	}

	gainlow = (unsigned char)(gainana&0xff);
	gainhigh = (unsigned char)((gainana>>8)&0xff);

	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);

	sensor_write(sd, 0x020F, (unsigned char)(gaindigi&0xff));
	sensor_write(sd, 0x020E, (unsigned char)(gaindigi>>8));

	info->gain = gain_val;
	return 0;
}


static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int shutter, frame_length;
	static int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	shutter = exp_val>>4;
	if (shutter  > imx386_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = imx386_sensor_vts;

	sensor_write(sd, 0x0104, 0x01);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x0104, 0x00);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

/*
 *set && get sensor flip
 */

static int sensor_get_fmt_mbus_core(struct v4l2_subdev *sd, int *code)
{
	struct sensor_info *info = to_state(sd);
	data_type get_value;

	sensor_read(sd, 0x0101, &get_value);
	switch (get_value) {
	case 0x00:
		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
		break;
	case 0x01:
		*code = MEDIA_BUS_FMT_SRGGB10_1X10;
		break;
	case 0x02:
		*code = MEDIA_BUS_FMT_SGBRG10_1X10;
		break;
	case 0x03:
		*code = MEDIA_BUS_FMT_SBGGR10_1X10;
		break;
	default:
		*code = info->fmt->mbus_code;
	}
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	sensor_dbg("into set sensor hfilp the value:%d\n", enable);
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x0101, &get_value);
	if (enable)
		set_value = get_value | 0x01;
	else
		set_value = get_value & 0xfe;

	sensor_write(sd, 0x0101, set_value);
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int enable)
{
	data_type get_value;
	data_type set_value;

	sensor_dbg("into set sensor hfilp the value:%d\n", enable);
	if (!(enable == 0 || enable == 1))
		return -1;

	sensor_read(sd, 0x0101, &get_value);
	if (enable)
		set_value = get_value | 0x02;
	else
		set_value = get_value & 0xfd;

	sensor_write(sd, 0x0101, set_value);
	return 0;

}

/* long exp mode eg:
 *
 *   coarse_int_time = 60290; frame_length= 65535;
 *   times =8; VTPXCLK = 480Mhz; imx386_hts = 4976
 *
 *	EXP time = 60290 * 8 * 4976 / (120[Mhz] * 4 ) = 5 s
 */
static int fps_change_flag;

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	unsigned int coarse_int_time, frame_length = 0;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;
	unsigned int times_reg, times, imx386_hts = 0, FRM_LINES = 0;

	sensor_dbg("------start set fps = %d--------!!!!\n", fps->fps);
	if (fps->fps == 0)
		fps->fps = 30;

	/****************************/
	/*Early enter long exp mode; in case of quit before*/
	if (fps->fps < 0) {
		times = 2;
		times_reg = 0x1;
		imx386_hts = wsize->hts;

		/* test : when fps = 10  && delay 150ms, will not quit before */
		coarse_int_time = wsize->pclk/imx386_hts/times/10;
		FRM_LINES = coarse_int_time;
		coarse_int_time -= 10;

		sensor_write(sd, 0x0100, 0x00);
		sensor_write(sd, 0x0104, 0x01);

		sensor_write(sd, 0x3004, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (imx386_hts >> 8));
		sensor_write(sd, 0x0343, (imx386_hts & 0xff));

		sensor_write(sd, 0x0100, 0x01);
		sensor_write(sd, 0x0104, 0x00);

		usleep_range(150000, 200000);
	}

	/****************************/

	if (fps->fps < 0) {
	sensor_dbg("------enter long exp--------!!!!\n");
		fps->fps = -fps->fps;

		if (fps->fps >= 1 && fps->fps <= 5) {
			times = 8;
			times_reg = 0x3;
			imx386_hts = wsize->hts;
		} else if (fps->fps <= 10) {
			times = 16;
			times_reg = 0x4;
			imx386_hts = wsize->hts;
		} else if (fps->fps <= 20) {
			times = 16;
			times_reg = 0x4;
			imx386_hts = wsize->hts * 2;
		} else {
			times = 16;
			times_reg = 0x4;
			imx386_hts = wsize->hts * 4;
		}

		coarse_int_time = wsize->pclk/imx386_hts/times * fps->fps;
		FRM_LINES = coarse_int_time;
		/* 0 <=  coarse_int_time  <= FRM_LINES - 10 */
		coarse_int_time -= 20;
		fps_change_flag = 1;
	} else {
		sensor_dbg("------enter normal exp--------!!!!\n");
		coarse_int_time = wsize->pclk/wsize->hts/fps->fps;
		if (coarse_int_time	> imx386_sensor_vts - 4)
			frame_length = coarse_int_time + 4;
		else
			frame_length = imx386_sensor_vts;
		fps_change_flag = 0;
	}

	/*sensor reg standby*/
	sensor_write(sd, 0x0100, 0x00);
	/*grouped hold function*/
	sensor_write(sd, 0x0104, 0x01);

	if (fps_change_flag == 1) {
		/* open long exp mode*/
		sensor_write(sd, 0x3004, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (imx386_hts >> 8));
		sensor_write(sd, 0x0343, (imx386_hts & 0xff));
	} else {
		/* close long exp mode */
		sensor_write(sd, 0x3004, 0x00);

		sensor_write(sd, 0x0340, (frame_length >> 8));
		sensor_write(sd, 0x0341, (frame_length & 0xff));

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0342, (wsize->hts >> 8));
		sensor_write(sd, 0x0343, (wsize->hts & 0xff));
	}
	/*must release*/
	sensor_write(sd, 0x0104, 0x00);
	sensor_write(sd, 0x0100, 0x01);

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
		ret = sensor_write(sd, 0x0100, rdval&0xfe);
	else
		ret = sensor_write(sd, 0x0100, rdval|0x01);
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		/*Sensors and PMU use the same IIC,
		so IIC cannot be locked when setting up PMU*/
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(1000, 1200);

		cci_lock(sd);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(3000, 3200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		cci_unlock(sd);

		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);

		cci_lock(sd);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		vin_gpio_set_status(sd, POWER_EN, 0);
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
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;

	sensor_read(sd, 0x0016, &rdval);
	sensor_dbg("0x0016 0x%x\n", rdval);

	sensor_read(sd, 0x0017, &rdval);
	sensor_dbg("0x0017 0x%x\n", rdval);

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
	info->width = 4000;
	info->height = 3000;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

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
	case VIDIOC_VIN_SENSOR_SET_FPS:
		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case VIDIOC_VIN_GET_SENSOR_CODE:
		sensor_get_fmt_mbus_core(sd, (int *)arg);
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
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
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

	{
	.width         = 1920,
	.height        = 1080,
	.hoffset       = 0,
	.voffset       = 0,
	.hts           = 2256,
	.vts           = 1104,
	.pclk          = 147*1000*1000,
	.mipi_bps      = 740*1000*1000,
	.fps_fixed     = 60,
	.bin_factor    = 1,
	.intg_min      = 16,
	.intg_max      = (1104-4)<<4,
	.gain_min      = 16,
	.gain_max      = (128<<4),
	.regs          = reg_mipi_2lane_1080P60_10bit,
	.regs_size     = ARRAY_SIZE(reg_mipi_2lane_1080P60_10bit),
	.set_size      = NULL,
	.top_clk       = 336*1000*1000,
	.isp_clk       = 326*1000*1000,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_get_mbus_config(struct v4l2_subdev *sd,
				unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

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
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->val);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->val);

	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	struct sensor_exp_gain exp_gain;

	ret = sensor_write_array(sd, Imx386_default_regs,
					 ARRAY_SIZE(Imx386_default_regs));

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
	imx386_sensor_vts = wsize->vts;
	imx386_sensor_hts = wsize->hts;

	/* To revert the gain and exp to last time*/
	if (info->exp && info->gain) {
		if ((info->exp > imx386_sensor_vts*16) || (info->gain > 16)) {
			exp_gain.exp_val = imx386_sensor_vts*16;
			exp_gain.gain_val = info->gain * info->exp / exp_gain.exp_val;
		} else {
			exp_gain.exp_val = info->exp;
			exp_gain.gain_val = info->gain;
		}
	} else {
		exp_gain.exp_val = 16*1000;
		exp_gain.gain_val = 16*68;
	}
	sensor_s_exp_gain(sd, &exp_gain);

	sensor_write(sd, 0x0100, 0x01); /*sensor mipi stream on*/

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable)
		return 0;

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.try_ctrl = sensor_try_ctrl,
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
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};


static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 4);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 16,
			      2816 * 16, 1, 1 * 16);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
			      65536 * 16, 1, 1);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_VFLIP, 0, 1, 1, 0);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_dev_id;

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

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
	info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x32;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

module_init(init_sensor);
module_exit(exit_sensor);
