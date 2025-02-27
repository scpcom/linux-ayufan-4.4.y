/*
 * A V4L2 driver for sc4353 Raw cameras.
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
MODULE_DESCRIPTION("A low-level driver for SC4353_MIPI sensors");
MODULE_LICENSE("GPL");

#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR 0xcd01

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

#define HDR_RATIO 16

/*
 * The SC4353_MIPI i2c address
 */
#define I2C_ADDR 0x60

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sc4353_mipi"
#define SENSOR_NAME_2 "sc4353_mipi_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_2560_1440_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0xd4}, //1113
	{0x36f9, 0xd4},
	{0x3632, 0x18},  //[5:4]pump driving ability
	{0x3650, 0x06},  //mipi driving ability
	{0x3e01, 0x62},
	{0x330b, 0x10},
	{0x3e09, 0x20},
	{0x3637, 0x70},
	{0x3e25, 0x03},
	{0x3e26, 0x20}, //for 20->3f fine gain
	{0x3620, 0xa8}, //cmp pd option
	{0x3638, 0x22},
	{0x3306, 0x70},
	{0x330a, 0x00},
	{0x330b, 0xf8},
	{0x3309, 0x88},
	{0x331f, 0x79},
	{0x330f, 0x04},
	{0x3310, 0x20},//0201B
	{0x3314, 0x94},
	{0x330e, 0x38},//[10,40]
	{0x3367, 0x10},
	{0x3347, 0x05}, //1113
	{0x3342, 0x01},
	{0x57a4, 0xa0},
	{0x5781, 0x28},
	{0x5782, 0x20},
	{0x5783, 0x20},
	{0x5784, 0x20},
	{0x5785, 0x40},
	{0x5786, 0x18},
	{0x5787, 0x18},
	{0x5788, 0x10},
	{0x3622, 0xf6},
	{0x3630, 0x13},
	{0x3904, 0x0c},
	{0x3908, 0x41},
	{0x330a, 0x01},
	{0x330b, 0x00},  //1113
	{0x363a, 0x90}, //1113
	{0x3306, 0x60},
	{0x3631, 0x88},
	{0x3614, 0x80}, //smear
	{0x3213, 0x06}, //away from pump
	{0x3633, 0x33},  // logic
	{0x3622, 0xf6},  //
	{0x3301, 0x10},  // [1,12]
	{0x3630, 0xc0},
	//0x3622  auto logic read 0x3680 for auto value
	{0x360f, 0x05}, //0x3622 0x360f[0] enable  0x3622 in 0x3680, 0x360f[2]fine gain op 1~20--3f 0~10--1f
	{0x367a, 0x08},//gain0{3e08[4:2],3e09[4:2]}	gain=0x0720
	{0x367b, 0x38},//gain1	gain=1f20
	{0x3671, 0xf6},//gain<gain0	[0320,033f)
	{0x3672, 0xf6},//gain0=<gain<gain1	[0720,1f20)
	{0x3673, 0x16},//gain>=gain1	[1f20,+]
	//0x3633  auto logic read 0x3683 for auto value
	{0x366e, 0x04},//0x366e[3]fine gain op 1~20--3f(0x3e09[4:2]) 0~10--1f(0x3e09[3:1])
	{0x3670, 0x08},//[3] 0x3633 auto en
	{0x369c, 0x38},//gain0  0x1f20
	{0x369d, 0x3f},//gain1  0x1f3c
	{0x3690, 0x33},//sel0	gain<0x1f20				prnu	//20190307
	{0x3691, 0x34},//sel1	0x1f20<=gain<0x1f3c		save power	//20190307
	{0x3692, 0x44},//sel2	gain>=0x1f3c			rts
	//0x3301  auto logic read 0x3373 for auto value
	{0x3364, 0x1d},//0x3364[4] comprst 0x3301 auto enable, 0x3364[3]fine gain op 1~20--3f(0x3e09[4:2]) 0~10--1f(0x3e09[3:1])
	{0x33b8, 0x10}, //[0320,0720)
	{0x33b9, 0x18}, //[0720,1f30)
	{0x33ba, 0x70}, //[1f20,+)
	{0x33b6, 0x07}, //0720
	{0x33b7, 0x2f}, //1f20
	//0x3630  auto logic read 0x3681 for auto value
	{0x3670, 0x0a},//0x3670[1] 0x3630 auto enable , 0x3681 reaout
	{0x367c, 0x38},//auto 0x3630 gain0
	{0x367d, 0x3f},//auto 0x3630 gain1
	{0x3674, 0xc0},//auto 0x3630 sel0	[0x0320,0x1f20)
	{0x3675, 0xc8},//auto 0x3630 sel1	[0x1f20,0x1f3c)
	{0x3676, 0xaf},//auto 0x3630 sel2	[0x1f3c,+)
	{0x330b, 0x08},//[ns,of][100,11a] -- 0x3306=0x60
	{0x301f, 0x01},
	{0x5781, 0x04},
	{0x5782, 0x04},
	{0x5783, 0x02},
	{0x5784, 0x02},
	{0x5786, 0x20},
	{0x5789, 0x10},
	{0x363b, 0x09},//hvdd
	{0x3625, 0x0a},//[3]smear
	//HITEMP  20190416
	{0x3904, 0x10},
	{0x3902, 0xc5},//mirror & flip, blc trig
	{0x3933, 0x0a},//BLC_MAX
	{0x3934, 0x0d},
	{0x3942, 0x02},//BLC_KNEE
	{0x3943, 0x12},
	{0x3940, 0x65},//BLC_SLOPE
	{0x3941, 0x18},//BLC_MAX_STEP
	{0x395e, 0xa0},
	{0x3960, 0x9d},// AlphaDiv0
	{0x3961, 0x9d},// AlphaDiv1
	{0x3966, 0x4e},// AlphaDiv2
	{0x3962, 0x89},//TEMP_ON
	{0x3963, 0x80},//
	{0x3980, 0x60},//KH
	{0x3981, 0x30},
	{0x3982, 0x15},
	{0x3983, 0x10},
	{0x3984, 0x0d},
	{0x3985, 0x20},
	{0x3986, 0x30},
	{0x3987, 0x60},
	{0x3988, 0x04},//POSH
	{0x3989, 0x0c},
	{0x398a, 0x14},
	{0x398b, 0x24},
	{0x398c, 0x50},
	{0x398d, 0x32},
	{0x398e, 0x1e},
	{0x398f, 0x0a},
	{0x3990, 0xc0},//KV
	{0x3991, 0x50},
	{0x3992, 0x22},
	{0x3993, 0x0c},
	{0x3994, 0x10},
	{0x3995, 0x38},
	{0x3996, 0x80},
	{0x3997, 0xff},
	{0x3998, 0x08},//POSV
	{0x3999, 0x16},
	{0x399a, 0x28},
	{0x399b, 0x40},
	{0x399c, 0x50},
	{0x399d, 0x28},
	{0x399e, 0x18},
	{0x399f, 0x0c},
	//init
	{0x3e01, 0xbb},
	{0x3e02, 0x00},
	{0x3301, 0x10},  // [1,12]
	{0x3632, 0x18},  //1123
	{0x3631, 0x88},
	{0x3636, 0x25},
	{0x36e9, 0x54}, //1113
	{0x36f9, 0x54},
	{0x0100, 0x01}, //sclk 121.5M count_clck 364.5M
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

static int sc4353_sensor_vts;
static int sc4353_sensor_svr;
static int shutter_delay = 1;
static int shutter_delay_cnt;
static int fps_change_flag;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);
	//struct sensor_win_size *wsize = info->current_wins;
	//if(exp_val > wsize->vts * 2 - 4)
		//exp_val = wsize->vts * 2 - 4;

	exphigh = (unsigned char) (0xf & (exp_val>>15));
	expmid = (unsigned char) (0xff & (exp_val>>7));
	explow = (unsigned char) (0xf0 & (exp_val<<1));

	sensor_write(sd, 0x3e02, explow);
	sensor_write(sd, 0x3e01, expmid);
	sensor_write(sd, 0x3e00, exphigh);
	sensor_dbg("sensor_set_exp = %d line Done!\n", exp_val);
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
	data_type rdval;
	data_type gainlow = 0;
	data_type gainhigh = 0;
	data_type gaindiglow = 0x80;
	data_type gaindighigh = 0x00;
	int gainana = gain_val << 1;

	sensor_read(sd, 0x3e03, &rdval);
	sensor_dbg("0x3e03 = 0x%x\n", rdval);
	if (gainana < 0x40) {	//32-64
		gainhigh = 0x03;
		gainlow = gainana;
	} else if (gainana < 2 * 0x40) {//64-128
		gainhigh = 0x07;
		gainlow = gainana >> 1;
	} else if (gainana < 4 * 0x40) {  //128-256
		gainhigh = 0x0f;
		gainlow = gainana >> 2;
	} else if (gainana < 8 * 0x40) { //256-512
		gainhigh = 0x1f;
		gainlow = gainana >> 3;
	} else {
		gainhigh = 0x1f;
		gainlow = 0x3f;
		if (gainana < 16 * 0x40) {  //512-1024
			gaindiglow = gainana >> 2;
			gaindighigh = 0x00;
		} else if (gainana < 32 * 0x40) {	//1024-2048
			gaindiglow = gainana >> 3;
			gaindighigh = 0x01;
		} else if (gainana < 64 * 0x40) { //2048-4096
			gaindiglow = gainana >> 4;
			gaindighigh = 0x03;
		} else if (gainana < 128 * 0x40) { //4096-8192
			gaindiglow = gainana >> 5;
			gaindighigh = 0x07;
		} else if (gainana < 256 * 0x40) { //8192-12864
			gaindiglow = gainana >> 6;
			gaindighigh = 0x0f;
		} else {
			gaindiglow = 0xfc;
			gaindighigh = 0x0f;
		}
	}
	sensor_write(sd, 0x3e09, (unsigned char)gainlow);
	sensor_write(sd, 0x3e08, (unsigned char)gainhigh);
	sensor_write(sd, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(sd, 0x3e06, (unsigned char)gaindighigh);
	sensor_dbg("sensor_set_anagain = %d, 0x%x, 0x%x Done!\n", gain_val, gainhigh, gainlow);
	sensor_dbg("digital_gain = 0x%x, 0x%x Done!\n", gaindighigh, gaindiglow);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;
	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;
	if (fps_change_flag) {
		if (shutter_delay_cnt == shutter_delay) {
			//sensor_write(sd, 0x320f, sc4353_sensor_vts / (sc4353_sensor_svr + 1) & 0xFF);
			//sensor_write(sd, 0x320e, sc4353_sensor_vts / (sc4353_sensor_svr + 1) >> 8 & 0xFF);
			//sensor_write(sd, 0x302d, 0);
			shutter_delay_cnt = 0;
			fps_change_flag = 0;
		} else
			shutter_delay_cnt++;
	}
	sensor_write(sd, 0x3812, 0x00);//group_hold
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_write(sd, 0x3812, 0x30);
	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	data_type rdval1, rdval2, rdval3;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	sc4353_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	fps_change_flag = 1;
	//sensor_write(sd, 0x302d, 1);

	//sensor_read(sd, 0x30f8, &rdval1);
	//sensor_read(sd, 0x30f9, &rdval2);
	//sensor_read(sd, 0x30fa, &rdval3);

	sensor_dbg("sc4353_sensor_svr: %d, vts: %d.\n", sc4353_sensor_svr, (rdval1 | (rdval2<<8) | (rdval3<<16)));
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
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
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
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
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

	sensor_read(sd, 0x3107, &rdval);
	sensor_print("0x3107 = 0x%x\n", rdval);
	if (rdval != 0xcd)
		return -ENODEV;
	sensor_read(sd, 0x3108, &rdval);
	sensor_print("0x3108 = 0x%x\n", rdval);
	if (rdval != 0x01)
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
	info->width = 2560;
	info->height = 1440;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

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
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
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
	 .width = 2560,
	 .height = 1440,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2700,
	 .vts = 1500,
	 .pclk = 122 * 1000 * 1000,
	 .mipi_bps = 607.5 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 0,
	 .intg_max = (1500 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 496 << 4,
	 .regs = sensor_2560_1440_regs,
	 .regs_size = ARRAY_SIZE(sensor_2560_1440_regs),
	 .set_size = NULL,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_get_mbus_config(struct v4l2_subdev *sd,
				unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
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
	data_type rdval_l, rdval_h;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;
	struct sensor_exp_gain exp_gain;

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
	sc4353_sensor_vts = wsize->vts;
//	sensor_read(sd, 0x300E, &rdval_l);
//	sensor_read(sd, 0x300F, &rdval_h);
//	sc4353_sensor_svr = (rdval_h << 8) | rdval_l;

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

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

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
			      65536 * 16, 1, 1);
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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
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
