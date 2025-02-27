/*
 * A V4L2 driver for imx317 Raw cameras.
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

#define SENSOR_FRAME_RATE 30

/*
 * The IMX317 i2c address
 */
#define I2C_ADDR 0x20

#define SENSOR_NUM 0x2
#define SENSOR_NAME "imx278_mipi"
#define SENSOR_NAME_2 "imx278_mipi_2"

/*
 * The default register settings
 */
static struct regval_list sensor_default_regs[] = {
	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x3042, 0x01},
	{0x5CE8, 0x00},
	{0x5CE9, 0x91},
	{0x5CEA, 0x00},
	{0x5CEB, 0x2A},
	{0x5F1B, 0x01},
	{0x5C2C, 0x01},
	{0x5C2D, 0xFF},
	{0x5C2E, 0x00},
	{0x5C2F, 0x00},
	{0x5F0D, 0x6E},
	{0x5F0E, 0x7C},
	{0x5F0F, 0x14},
	{0x6100, 0x30},
	{0x6101, 0x12},
	{0x6102, 0x14},
	{0x6104, 0x91},
	{0x6105, 0x30},
	{0x6106, 0x11},
	{0x6107, 0x12},
	{0x6505, 0xC4},
	{0x6507, 0x25},
	{0x6508, 0xE1},
	{0x6509, 0xD7},
	{0x650A, 0x20},
	{0x7382, 0x01},
	{0x7383, 0x13},
	{0x7788, 0x04},
	{0x9006, 0x0C},

	{0xB000, 0x78},
	{0xB001, 0xB6},
	{0xB002, 0x78},
	{0xB003, 0xB7},
	{0xB004, 0x98},
	{0xB005, 0x00},
	{0xB006, 0x98},
	{0xB007, 0x01},
	{0xB008, 0x98},
	{0xB009, 0x02},
	{0xB00A, 0xA9},
	{0xB00B, 0x0F},
	{0xB00C, 0xA9},
	{0xB00D, 0x12},
	{0xB00E, 0xA9},
	{0xB00F, 0x15},
	{0xB010, 0xA9},
	{0xB011, 0x16},
	{0xB012, 0xA9},
	{0xB013, 0x17},
	{0xB014, 0xA9},
	{0xB015, 0x18},
	{0xB016, 0xA9},
	{0xB017, 0x19},

	{0x3020, 0x02},

	{0x73A0, 0x21},
	{0x73A1, 0x03},
	{0x7709, 0x00},
	{0x7786, 0x09},
	{0x778A, 0x00},
	{0x778B, 0x60},
	{0x951E, 0x3D},
	{0x951F, 0x05},
	{0x9521, 0x12},
	{0x9522, 0x02},
	{0x9524, 0x00},
	{0x9525, 0x31},
	{0x9526, 0x00},
	{0x9527, 0x04},
	{0x952A, 0x00},
	{0x952B, 0x7A},
	{0x952C, 0x00},
	{0x952D, 0x0B},

	{0x9606, 0x00},
	{0x9607, 0x90},

	{0x9618, 0x00},
	{0x9619, 0x52},
	{0x961E, 0x00},
	{0x961F, 0x1E},

	{0x9620, 0x00},
	{0x9621, 0x18},
	{0x9622, 0x00},
	{0x9623, 0x18},
	{0x9706, 0x00},
	{0x9707, 0x00},

	{0x9718, 0x00},
	{0x9719, 0x32},
	{0x971A, 0x00},
	{0x971B, 0x32},
	{0x971C, 0x00},
	{0x971D, 0x32},

	{0x971E, 0x00},
	{0x971F, 0x1E},
	{0x9720, 0x00},
	{0x9721, 0x20},
	{0x9722, 0x00},
	{0x9723, 0x20},

	{0x7837, 0x00},
	{0x7965, 0x0F},
	{0x7966, 0x0A},
	{0x7967, 0x07},
	{0x7968, 0x0F},
	{0x7969, 0x60},
	{0x796B, 0x32},
	{0xA85B, 0x10},
	{0x924E, 0x42},

	{0x9250, 0x78},
	{0x9251, 0x3C},
	{0x9252, 0x14},
	{0x9332, 0x02},
	{0x9333, 0x02},
	{0x9335, 0x02},
	{0x9336, 0x02},
	{0x9357, 0x04},
	{0x9359, 0x04},
	{0x935A, 0x04},

	{0x9809, 0x02},
	{0x980A, 0x02},
	{0x980B, 0x02},
	{0x980D, 0x00},
	{0x980E, 0x00},
	{0x980F, 0x06},
	{0x9812, 0x00},
	{0x9813, 0x00},
	{0x9814, 0x00},
	{0x981B, 0x1E},
	{0x981C, 0x23},
	{0x981D, 0x23},
	{0x981E, 0x28},
	{0x981F, 0x55},
	{0x9820, 0x55},
	{0x9822, 0x1B},
	{0x9823, 0x1B},
	{0x9824, 0x0A},
	{0x9825, 0x00},
	{0x9826, 0x00},
	{0x9827, 0x69},
	{0x9828, 0xA0},
	{0x9829, 0xA0},
	{0x982A, 0x00},
	{0x982B, 0x80},
	{0x982C, 0x00},
	{0x982D, 0x8C},
	{0x982E, 0x00},
	{0x982F, 0x8C},
	{0x9830, 0x04},
	{0x9831, 0x80},
	{0x9832, 0x05},
	{0x9833, 0x00},
	{0x9834, 0x05},
	{0x9835, 0x00},
	{0x9836, 0x00},
	{0x9837, 0x80},
	{0x9838, 0x00},
	{0x9839, 0x80},
	{0x983A, 0x00},
	{0x983B, 0x80},
	{0x983C, 0x0E},
	{0x983D, 0x01},
	{0x983E, 0x01},
	{0x983F, 0x0E},
	{0x9840, 0x06},
	{0x9845, 0x0E},
	{0x9846, 0x00},


	{0x9848, 0x0E},
	{0x9849, 0x06},
	{0x984A, 0x06},
	{0x9871, 0x14},
	{0x9872, 0x0E},

	{0x9877, 0x7F},
	{0x9878, 0x1E},
	{0x9879, 0x09},
	{0x987B, 0x0E},
	{0x987C, 0x0E},
	{0x988A, 0x13},
	{0x988B, 0x13},
	{0x9893, 0x13},
	{0x9894, 0x13},
	{0x9898, 0x75},
	{0x9899, 0x2D},
	{0x989A, 0x26},
	{0x989E, 0x96},
	{0x989F, 0x1E},
	{0x98A0, 0x0D},
	{0x98A1, 0x43},
	{0x98A2, 0x0E},
	{0x98A3, 0x03},
	{0x98AB, 0x66},
	{0x98AC, 0x66},
	{0x98B1, 0x4D},
	{0x98B2, 0x4D},
	{0x98B4, 0x0D},
	{0x98B5, 0x0D},
	{0x98BC, 0x7A},
	{0x98BD, 0x66},
	{0x98BE, 0x78},
	{0x98C2, 0x66},
	{0x98C3, 0x66},
	{0x98C4, 0x62},
	{0x98C6, 0x14},
	{0x98CE, 0x7A},
	{0x98CF, 0x78},
	{0x98D0, 0x78},
	{0x98D4, 0x66},
	{0x98D5, 0x62},
	{0x98D6, 0x62},
	{0x9921, 0x0A},
	{0x9922, 0x01},
	{0x9923, 0x01},
	{0x9928, 0xA0},
	{0x9929, 0xA0},
	{0x9949, 0x06},
	{0x994A, 0x06},
	{0x9999, 0x26},
	{0x999A, 0x26},
	{0x999F, 0x0D},
	{0x99A0, 0x0D},
	{0x99A2, 0x03},
	{0x99A3, 0x03},
	{0x99BD, 0x78},
	{0x99BE, 0x78},
	{0x99C3, 0x62},
	{0x99C4, 0x62},
	{0x99CF, 0x78},
	{0x99D0, 0x78},
	{0x99D5, 0x62},
	{0x99D6, 0x62},
	{0xA900, 0x00},
	{0xA901, 0x00},
	{0xA90B, 0x00},
	{0x9342, 0x04},
	{0x934D, 0x04},
	{0x934F, 0x04},
	{0x9350, 0x04},

	{0x3011, 0xff},
	{0xAF00, 0x01},
	{0xAF01, 0x00},
	{0xAF02, 0x00},
	{0xAF03, 0xDB},
	{0xAF04, 0x01},
	{0xAF05, 0x00},
	{0xAF06, 0x01},
	{0xAF07, 0xD2},
	{0xAF08, 0x02},
	{0xAF09, 0x3D},
	{0xAF0A, 0x02},
	{0xAF0B, 0x83},

	{0x0B05, 0x01},
	{0x0B06, 0x01}
};

static struct regval_list sensor_13M20_regs[] = {
	{0x0100, 0x00},

	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},
	{0x0301, 0x05},
	{0x0303, 0x02},
	{0x0305, 0x0C},
	{0x0306, 0x01},
	{0x0307, 0x90},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x04},
	{0x030F, 0xB0},
	{0x0310, 0x00},
	{0x3041, 0x01},

	{0x0342, 0x13},
	{0x0343, 0x70},
	{0x0340, 0x0C},
	{0x0341, 0x94},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x10},
	{0x0349, 0x6F},
	{0x034A, 0x0C},
	{0x034B, 0x2F},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x11},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x10},
	{0x040D, 0x70},
	{0x040E, 0x0C},
	{0x040F, 0x30},
	{0x3038, 0x00},
	{0x303A, 0x00},
	{0x303B, 0x10},
	{0x300D, 0x00},
	{0x034C, 0x10},
	{0x034D, 0x70},
	{0x034E, 0x0C},
	{0x034F, 0x30},

	{0x3029, 0x00},
	{0x3A00, 0x00},
	{0x3A01, 0x00},
	{0x3A02, 0x05},
	{0x3A03, 0x05},
	{0x3A04, 0x05},
	{0x3A05, 0xF8},
	{0x3A06, 0x40},
	{0x3A07, 0xFE},
	{0x3A08, 0x10},
	{0x3A09, 0x14},
	{0x3A0A, 0xFE},
	{0x3A0B, 0x44},
	{0x0202, 0x0C},
	{0x0203, 0x82},

	{0x0808, 0x02},
	{0x080A, 0x00},
	{0x080B, 0x77},
	{0x080C, 0x00},
	{0x080D, 0x37},
	{0x080E, 0x00},
	{0x080F, 0x67},
	{0x0810, 0x00},
	{0x0811, 0x37},
	{0x0812, 0x00},
	{0x0813, 0x37},
	{0x0814, 0x00},
	{0x0815, 0x37},
	{0x0816, 0x00},
	{0x0817, 0xDF},
	{0x0818, 0x00},
	{0x0819, 0x2F},

	{0x0100, 0x01},

};

static struct regval_list sensor_4k30_regs[] = {
	{0x0100, 0x00},

	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},
	{0x0301, 0x05},
	{0x0303, 0x02},
	{0x0305, 0x0C},
	{0x0306, 0x01},
	{0x0307, 0xCC},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x01},
	{0x030F, 0xA0},
	{0x0310, 0x01},
	{0x3041, 0x01},

	{0x0342, 0x13},
	{0x0343, 0x70},
	{0x0340, 0x09},
	{0x0341, 0xA2},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x01},
	{0x0347, 0x78},
	{0x0348, 0x10},
	{0x0349, 0x6F},
	{0x034A, 0x0A},
	{0x034B, 0xB7},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x11},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x10},
	{0x040D, 0x70},
	{0x040E, 0x09},
	{0x040F, 0x40},
	{0x3038, 0x00},
	{0x303A, 0x00},
	{0x303B, 0x10},
	{0x300D, 0x00},
	{0x034C, 0x10},
	{0x034D, 0x70},
	{0x034E, 0x09},
	{0x034F, 0x40},

	{0x3029, 0x00},
	{0x3A00, 0x00},
	{0x3A01, 0x00},
	{0x3A02, 0x05},
	{0x3A03, 0x05},
	{0x3A04, 0x05},
	{0x3A05, 0xF8},
	{0x3A06, 0x40},
	{0x3A07, 0xFE},
	{0x3A08, 0x10},
	{0x3A09, 0x14},
	{0x3A0A, 0xFE},
	{0x3A0B, 0x44},
	{0x0202, 0x09},
	{0x0203, 0x92},

	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},

	{0x0808, 0x02},
	{0x080A, 0x00},
	{0x080B, 0x77},
	{0x080C, 0x00},
	{0x080D, 0x37},
	{0x080E, 0x00},
	{0x080F, 0x67},
	{0x0810, 0x00},
	{0x0811, 0x37},
	{0x0812, 0x00},
	{0x0813, 0x37},
	{0x0814, 0x00},
	{0x0815, 0x37},
	{0x0816, 0x00},
	{0x0817, 0xDF},
	{0x0818, 0x00},
	{0x0819, 0x2F},

	{0x0100, 0x01},
};


static struct regval_list sensor_1080p60_regs[] = {
	{0x0100, 0x00},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},

	{0x0301, 0x05},
	{0x0303, 0x02},
	{0x0305, 0x0C},
	{0x0306, 0x01},
	{0x0307, 0xCC},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x01},
	{0x030F, 0x30},
	{0x0310, 0x01},
	{0x3041, 0x01},

	{0x0342, 0x13},
	{0x0343, 0x70},
	{0x0340, 0x04},
	{0x0341, 0xD0},

	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x01},
	{0x0347, 0x78},
	{0x0348, 0x10},
	{0x0349, 0x6F},
	{0x034A, 0x0A},
	{0x034B, 0xB7},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x12},
	{0x0401, 0x01},
	{0x0404, 0x00},
	{0x0405, 0x20},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x10},
	{0x040D, 0x70},
	{0x040E, 0x04},
	{0x040F, 0xA0},
	{0x3038, 0x00},
	{0x303A, 0x00},
	{0x303B, 0x10},
	{0x300D, 0x00},
	{0x034C, 0x08},
	{0x034D, 0x30},
	{0x034E, 0x04},
	{0x034F, 0xA0},

	{0x3029, 0x00},
	{0x3A00, 0x00},
	{0x3A01, 0x00},
	{0x3A02, 0x05},
	{0x3A03, 0x05},
	{0x3A04, 0x05},
	{0x3A05, 0xF8},
	{0x3A06, 0x40},
	{0x3A07, 0xFE},
	{0x3A08, 0x10},
	{0x3A09, 0x14},
	{0x3A0A, 0xFE},
	{0x3A0B, 0x44},

	{0x0202, 0x04},
	{0x0203, 0x00},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},

	{0x0808, 0x02},
	{0x080A, 0x00},
	{0x080B, 0x67},
	{0x080C, 0x00},
	{0x080D, 0x2F},
	{0x080E, 0x00},
	{0x080F, 0x57},
	{0x0810, 0x00},
	{0x0811, 0x2F},
	{0x0812, 0x00},
	{0x0813, 0x27},
	{0x0814, 0x00},
	{0x0815, 0x2F},
	{0x0816, 0x00},
	{0x0817, 0xBF},
	{0x0818, 0x00},
	{0x0819, 0x27},

	{0x0100, 0x01},
};



static struct regval_list sensor_1080p30_regs[] = {
	{0x0100, 0x00},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},

	{0x0301, 0x05},
	{0x0303, 0x02},
	{0x0305, 0x0C},
	{0x0306, 0x01},
	{0x0307, 0xCC},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x01},
	{0x030F, 0x30},
	{0x0310, 0x01},
	{0x3041, 0x01},

	{0x0342, 0x13},
	{0x0343, 0x70},
	{0x0340, 0x09},
	{0x0341, 0xA0},

	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x01},
	{0x0347, 0x78},
	{0x0348, 0x10},
	{0x0349, 0x6F},
	{0x034A, 0x0A},
	{0x034B, 0xB7},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x12},
	{0x0401, 0x01},
	{0x0404, 0x00},
	{0x0405, 0x20},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x10},
	{0x040D, 0x70},
	{0x040E, 0x04},
	{0x040F, 0xA0},
	{0x3038, 0x00},
	{0x303A, 0x00},
	{0x303B, 0x10},
	{0x300D, 0x00},

	{0x034C, 0x07},
	{0x034D, 0x80},
	{0x034E, 0x04},
	{0x034F, 0x38},

	{0x3029, 0x00},
	{0x3A00, 0x00},
	{0x3A01, 0x00},
	{0x3A02, 0x05},
	{0x3A03, 0x05},
	{0x3A04, 0x05},
	{0x3A05, 0xF8},
	{0x3A06, 0x40},
	{0x3A07, 0xFE},
	{0x3A08, 0x10},
	{0x3A09, 0x14},
	{0x3A0A, 0xFE},
	{0x3A0B, 0x44},

	{0x0202, 0x04},
	{0x0203, 0x00},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},

	{0x0808, 0x02},
	{0x080A, 0x00},
	{0x080B, 0x67},
	{0x080C, 0x00},
	{0x080D, 0x2F},
	{0x080E, 0x00},
	{0x080F, 0x57},
	{0x0810, 0x00},
	{0x0811, 0x2F},
	{0x0812, 0x00},
	{0x0813, 0x27},
	{0x0814, 0x00},
	{0x0815, 0x2F},
	{0x0816, 0x00},
	{0x0817, 0xBF},
	{0x0818, 0x00},
	{0x0819, 0x27},

	{0x0100, 0x01},
};


static struct regval_list sensor_720p120_regs[] = {
	{0x0100, 0x00},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},

	{0x0301, 0x05},
	{0x0303, 0x02},
	{0x0305, 0x0C},
	{0x0306, 0x02},
	{0x0307, 0x48},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x0C},
	{0x030E, 0x01},
	{0x030F, 0xA0},
	{0x0310, 0x01},
	{0x3041, 0x01},

	{0x0342, 0x14},
	{0x0343, 0x00},
	{0x0340, 0x02},
	{0x0341, 0xF0},

	{0x0344, 0x00},
	{0x0345, 0xB0},
	{0x0346, 0x01},
	{0x0347, 0xE0},
	{0x0348, 0x0F},
	{0x0349, 0xBF},
	{0x034A, 0x0A},
	{0x034B, 0x4F},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x01},
	{0x0901, 0x13},
	{0x0401, 0x01},
	{0x0404, 0x00},
	{0x0405, 0x30},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x0F},
	{0x040D, 0x10},
	{0x040E, 0x02},
	{0x040F, 0xD0},
	{0x3038, 0x00},
	{0x303A, 0x00},
	{0x303B, 0x10},
	{0x300D, 0x01},
	{0x034C, 0x05},
	{0x034D, 0x00},
	{0x034E, 0x02},
	{0x034F, 0xD0},

	{0x300D, 0x01},
	{0x3029, 0x01},
	{0x3A00, 0x01},

	{0x3A01, 0x01},
	{0x3A02, 0x05},
	{0x3A03, 0x05},
	{0x3A04, 0x05},
	{0x3A05, 0xF8},
	{0x3A06, 0x40},
	{0x3A07, 0xFE},
	{0x3A08, 0x10},
	{0x3A09, 0x14},
	{0x3A0A, 0xFE},
	{0x3A0B, 0x44},

	{0x0202, 0x02},
	{0x0203, 0x00},

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

static int imx278_sensor_vts;
static int imx278_sensor_hts;

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, exphigh;
	struct sensor_info *info = to_state(sd);

	exp_val = (exp_val+8)>>4;

	exphigh = (unsigned char)((0xff00&exp_val)>>8);
	explow  = (unsigned char)((0x00ff&exp_val));

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

	if (gain_val <= 128) {
		gainana = 512 - 8192/gain_val;
		gaindigi = 256;
	} else {
		gainana = 448;
		gaindigi = gain_val * 2;
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

static int sensor_s_wb(struct v4l2_subdev *sd, int rgain, int bgain)
{
	if (rgain > 751)
		rgain = 751;
	if (bgain > 751)
		bgain = 751;

	sensor_write(sd, 0x0B90, rgain>>8);
	sensor_write(sd, 0x0B91, rgain&0xFF);
	sensor_write(sd, 0x0B92, bgain>>8);
	sensor_write(sd, 0x0B93, bgain&0xFF);

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, shutter, frame_length, r_gain, b_gain;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;
	r_gain = exp_gain->r_gain;
	b_gain = exp_gain->b_gain;

	shutter = exp_val>>4;
	if (shutter > imx278_sensor_vts - 4)
		frame_length = shutter;
	else
		frame_length = imx278_sensor_vts;

	sensor_write(sd, 0x0104, 0x01);
	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);
	sensor_s_wb(sd, r_gain, b_gain);
	sensor_write(sd, 0x0104, 0x00);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}


/* long exp mode eg:
 *
 *   coarse_int_time = 60290; frame_length= 65535;
 *   times =8; VTPXCLK = 480Mhz; imx278_hts = 4976
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
	unsigned int times_reg, times, imx278_hts = 0, FRM_LINES = 0;

	if (fps->fps == 0)
		fps->fps = 30;

	/****************************/
	/*Early enter long exp mode; in case of quit before*/
	if (fps->fps < 0) {
		times = 2;
		times_reg = 0x1;
		imx278_hts = wsize->hts;

		/* test : when fps = 10  && delay 150ms, will not quit before */
		coarse_int_time = wsize->pclk/imx278_hts/times/10;
		FRM_LINES = coarse_int_time;
		coarse_int_time -= 10;

		sensor_write(sd, 0x0100, 0x00);
		sensor_write(sd, 0x0104, 0x01);

		sensor_write(sd, 0x3002, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (imx278_hts >> 8));
		sensor_write(sd, 0x0343, (imx278_hts & 0xff));

		sensor_write(sd, 0x0100, 0x01);
		sensor_write(sd, 0x0104, 0x00);

		usleep_range(150000, 200000);
	}

	/****************************/

	if (fps->fps < 0) {
		fps->fps = -fps->fps;

		if (fps->fps >= 1 && fps->fps <= 5) {
			times = 8;
			times_reg = 0x3;
			imx278_hts = wsize->hts;
		} else if (fps->fps <= 10) {
			times = 16;
			times_reg = 0x4;
			imx278_hts = wsize->hts;
		} else if (fps->fps <= 20) {
			times = 16;
			times_reg = 0x4;
			imx278_hts = wsize->hts * 2;
		} else {
			times = 16;
			times_reg = 0x4;
			imx278_hts = wsize->hts * 4;
		}

		coarse_int_time = wsize->pclk/imx278_hts/times * fps->fps;
		FRM_LINES = coarse_int_time;
		/* 0 <=  coarse_int_time  <= FRM_LINES - 10 */
		coarse_int_time -= 20;
		fps_change_flag = 1;
	} else {
		coarse_int_time = wsize->pclk/wsize->hts/fps->fps;
		if (coarse_int_time	> imx278_sensor_vts - 4)
			frame_length = coarse_int_time + 4;
		else
			frame_length = imx278_sensor_vts;
		fps_change_flag = 0;
	}

	/*sensor reg standby*/
	sensor_write(sd, 0x0100, 0x00);
	/*grouped hold function*/
	sensor_write(sd, 0x0104, 0x01);

	if (fps_change_flag == 1) {
		/* open long exp mode*/
		sensor_write(sd, 0x3002, times_reg);

		sensor_write(sd, 0x0202, (coarse_int_time >> 8));
		sensor_write(sd, 0x0203, (coarse_int_time & 0xff));

		sensor_write(sd, 0x0340, (FRM_LINES >> 8));
		sensor_write(sd, 0x0341, (FRM_LINES & 0xff));

		sensor_write(sd, 0x0342, (imx278_hts >> 8));
		sensor_write(sd, 0x0343, (imx278_hts & 0xff));
	} else {
		/* close long exp mode */
		sensor_write(sd, 0x3002, 0x00);

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
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
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
		sensor_dbg("PWR_OFF! null\n");
/*		cci_lock(sd);
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
		cci_unlock(sd);*/
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
	if ((rdval&0x0f) != (V4L2_IDENT_SENSOR>>8))
		return -ENODEV;

	sensor_dbg("sensor id = 0x%x", rdval);
	sensor_read(sd, 0x0017, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR&0xff))
		return -ENODEV;
	sensor_dbg("%x\n", rdval);

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
	info->width = 4208;
	info->height = 3120;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30; /* 30fps */

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
	   .width      = 4208,
	   .height     = 3120,
	   .hoffset    = 0,
	   .voffset    = 0,
	   .hts        = 4976,
	   .vts        = 3220,
	   .pclk       = 320*1000*1000,
	   .mipi_bps   = 800*1000*1000,
	   .fps_fixed  = 20,
	   .bin_factor = 1,
	   .intg_min   = 16,
	   .intg_max   = (3220-4)<<4,
	   .gain_min   = 16,
	   .gain_max   = (128<<4),
	   .regs       = sensor_13M20_regs,
	   .regs_size  = ARRAY_SIZE(sensor_13M20_regs),
	   .set_size   = NULL,
	 },

	 {
	   .width      = 4160,
	   .height     = 3120,
	   .hoffset    = (4208-4160)/2,
	   .voffset    = 0,
	   .hts        = 4976,
	   .vts        = 3220,
	   .pclk       = 320*1000*1000,
	   .mipi_bps   = 800*1000*1000,
	   .fps_fixed  = 20,
	   .bin_factor = 1,
	   .intg_min   = 16,
	   .intg_max   = (3220-4)<<4,
	   .gain_min   = 16,
	   .gain_max   = (128<<4),
	   .regs	   = sensor_13M20_regs,
	   .regs_size  = ARRAY_SIZE(sensor_13M20_regs),
	   .set_size   = NULL,
	 },

	 {
	   .width      = 4208,
	   .height     = 2368,
	   .hoffset    = 0,
	   .voffset    = 0,
	   .hts        = 4976,
	   .vts        = 2466,
	   .pclk       = 368*1000*1000,
	   .mipi_bps   = 832*1000*1000,
	   .fps_fixed  = 30,
	   .bin_factor = 1,
	   .intg_min   = 16,
	   .intg_max   = (2466-4)<<4,
	   .gain_min   = 16,
	   .gain_max   = (128<<4),
	   .regs	   = sensor_4k30_regs,
	   .regs_size  = ARRAY_SIZE(sensor_4k30_regs),
	   .set_size   = NULL,
	 },

	/* info->time_hs=0x48 */
	 {
	   .width      = 2048,
	   .height     = 1152,
	   .hoffset    = 0,
	   .voffset    = 0,
	   .hts        = 4976,
	   .vts        = 2425,
	   .pclk       = 724*1000*1000,
	   .mipi_bps   = 832*1000*1000,
	   .fps_fixed  = 60,
	   .bin_factor = 1,
	   .intg_min   = 16,
	   .intg_max   = (2466-4)<<4,
	   .gain_min   = 16,
	   .gain_max   = (128<<4),
	   .regs       = sensor_1080p60_regs,
	   .regs_size  = ARRAY_SIZE(sensor_1080p60_regs),
	   .set_size   = NULL,
	 },


	   /* info->time_hs=0x48 */
	 {
	   .width	  = 1920,
	   .height	  = 1080,
	   .hoffset	  = 0,
	   .voffset	  = 0,
	   .hts		  = 4976,
	   .vts		  = 2425,
	   .pclk 	  = 724*1000*1000,
	   .mipi_bps   = 832*1000*1000,
	   .fps_fixed  = 30,
	   .bin_factor = 1,
	   .intg_min   = 16,
	   .intg_max   = (2466-4)<<4,
	   .gain_min   = 16,
	   .gain_max   = (128<<4),
	   .regs 	  = sensor_1080p30_regs,
	   .regs_size  = ARRAY_SIZE(sensor_1080p30_regs),
	   .set_size   = NULL,
	 },

	/* info->time_hs=0x30 */
	 {
	   .width      = 1280,
	   .height     = 720,
	   .hoffset    = 0,
	   .voffset    = 0,
	   .hts        = 4976,
	   .vts        = 752,
	   .pclk       = 449*1000*1000,
	   .mipi_bps   = 832*1000*1000,
	   .fps_fixed  = 120,
	   .bin_factor = 1,
	   .intg_min   = 16,
	   .intg_max   = (752-4)<<4,
	   .gain_min   = 16,
	   .gain_max   = (128<<4),
	   .regs       = sensor_720p120_regs,
	   .regs_size  = ARRAY_SIZE(sensor_720p120_regs),
	   .set_size   = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_get_mbus_config(struct v4l2_subdev *sd,
				unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;

	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info = container_of(ctrl->handler, struct sensor_info, handler);
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
	struct sensor_info *info = container_of(ctrl->handler, struct sensor_info, handler);
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
	imx278_sensor_vts = wsize->vts;
	imx278_sensor_hts = wsize->hts;

	sensor_s_exp(sd, 16*400);
	sensor_s_gain(sd, 16*100);

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
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
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
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x30;
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
