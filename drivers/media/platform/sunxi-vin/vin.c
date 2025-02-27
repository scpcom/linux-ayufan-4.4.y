
/*
 * vin.c for all v4l2 subdev manage
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/reset.h>

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/regulator/consumer.h>

#include "vin-cci/cci_helper.h"
#include "utility/config.h"
#include "modules/sensor/camera_cfg.h"
#include "utility/vin_io.h"
#include "modules/sensor/sensor_helper.h"
#include "vin.h"
#include "modules/sensor-list/sensor_list.h"

#define VIN_MODULE_NAME "sunxi-vin-media"

static char ccm0[I2C_NAME_SIZE] = "";
uint i2c0_addr = 0xff;
static char ccm1[I2C_NAME_SIZE] = "";
uint i2c1_addr = 0xff;

char act_name[I2C_NAME_SIZE] = "";
uint act_slave = 0xff;
uint use_sensor_list = 0xff;
uint ptn_on_cnt;
extern uint ptn_frame_cnt;

module_param_string(ccm0, ccm0, sizeof(ccm0), S_IRUGO | S_IWUSR);
module_param_string(ccm1, ccm1, sizeof(ccm1), S_IRUGO | S_IWUSR);
module_param(i2c0_addr, uint, S_IRUGO | S_IWUSR);
module_param(i2c1_addr, uint, S_IRUGO | S_IWUSR);

module_param_string(act_name, act_name, sizeof(act_name), S_IRUGO | S_IWUSR);
module_param(act_slave, uint, S_IRUGO | S_IWUSR);
module_param(use_sensor_list, uint, S_IRUGO | S_IWUSR);

static void vin_md_prepare_pipeline(struct vin_pipeline *p,
				  struct media_entity *me)
{
	struct v4l2_subdev *sd;
	int i;

	for (i = 0; i < VIN_IND_ACTUATOR; i++)
		p->sd[i] = NULL;

	while (1) {
		struct media_pad *pad = NULL;

		/* Find remote source pad */
		for (i = 0; i < me->num_pads; i++) {
			struct media_pad *spad = &me->pads[i];

			if (!(spad->flags & MEDIA_PAD_FL_SINK))
				continue;
			pad = media_entity_remote_pad(spad);
			if (pad)
				break;
		}

		if (pad == NULL)
			break;

		sd = media_entity_to_v4l2_subdev(pad->entity);
		vin_log(VIN_LOG_MD, "%s entity is %s, group id is 0x%x\n",
			__func__, pad->entity->name, sd->grp_id);

		switch (sd->grp_id) {
		case VIN_GRP_ID_SENSOR:
			p->sd[VIN_IND_SENSOR] = sd;
			break;
		case VIN_GRP_ID_MIPI:
			p->sd[VIN_IND_MIPI] = sd;
			break;
		case VIN_GRP_ID_CSI:
			p->sd[VIN_IND_CSI] = sd;
			break;
		case VIN_GRP_ID_TDM_RX:
			p->sd[VIN_IND_TDM_RX] = sd;
			break;
		case VIN_GRP_ID_ISP:
			p->sd[VIN_IND_ISP] = sd;
			break;
		case VIN_GRP_ID_SCALER:
			p->sd[VIN_IND_SCALER] = sd;
			break;
		case VIN_GRP_ID_CAPTURE:
			p->sd[VIN_IND_CAPTURE] = sd;
			break;
		default:
			break;
		}
		me = &sd->entity;
		if (me->num_pads == 1)
			break;
	}
}

static int vin_mclk_pin_release(struct vin_md *vind)
{
#ifndef FPGA_VER
	int i;

	for (i = 0; i < VIN_MAX_CCI; i++) {
		if (!IS_ERR_OR_NULL(vind->mclk[i].pin))
			devm_pinctrl_put(vind->mclk[i].pin);
	}
#endif
	return 0;
}

static int vin_md_get_clocks(struct vin_md *vind)
{
	struct device_node *np = vind->pdev->dev.of_node;
	unsigned int core_clk;
	struct device *dev;
	char clk_name[20];
	int i;

	dev = &vind->pdev->dev;
	if (IS_ERR_OR_NULL(dev)) {
		vin_err("dev is NULL!\n");
		return -1;
	}
	/*get csi top clk*/
	vind->clk[VIN_TOP_CLK].clock = devm_clk_get(dev, "csi_top");
	if (IS_ERR(vind->clk[VIN_TOP_CLK].clock)) {
		vin_err("get csi top clk fail\n");
		return PTR_ERR(vind->clk[VIN_TOP_CLK].clock);
	}
	vind->clk[VIN_TOP_CLK_SRC].clock = devm_clk_get(dev, "csi_top_src");
	if (IS_ERR(vind->clk[VIN_TOP_CLK_SRC].clock)) {
		vin_err("get csi top clk src fail\n");
		return PTR_ERR(vind->clk[VIN_TOP_CLK_SRC].clock);
	}
	/*get mclk*/
	for (i = 0; i < VIN_MAX_CCI; i++) {
		sprintf(clk_name, "csi_mclk%d", i);
		vind->mclk[i].mclk = devm_clk_get(dev, clk_name);
		if (IS_ERR(vind->mclk[i].mclk)) {
			vin_err("get mclk%d clk fail\n", i);
			return PTR_ERR(vind->mclk[i].mclk);
		}
		sprintf(clk_name, "csi_mclk%d_24m", i);
		vind->mclk[i].clk_24m = devm_clk_get(dev, clk_name);
		if (IS_ERR(vind->mclk[i].clk_24m)) {
			vin_err("get mclk%d clk src 24m fail\n", i);
			return PTR_ERR(vind->mclk[i].clk_24m);
		}
		sprintf(clk_name, "csi_mclk%d_pll", i);
		vind->mclk[i].clk_pll = devm_clk_get(dev, clk_name);
		if (IS_ERR(vind->mclk[i].clk_pll)) {
			vin_err("get mclk%d clk src pll fail\n", i);
			return PTR_ERR(vind->mclk[i].clk_pll);
		}
	}
	/*get isp clk*/
	vind->isp_clk[VIN_ISP_CLK].clock = devm_clk_get(dev, "csi_isp");
	if (IS_ERR(vind->isp_clk[VIN_ISP_CLK].clock)) {
		vind->isp_clk[VIN_ISP_CLK].clock = NULL;
		vin_warn("get csi isp clk fail\n");
	}
	vind->isp_clk[VIN_ISP_CLK_SRC].clock = devm_clk_get(dev, "csi_isp_src");
	if (IS_ERR(vind->isp_clk[VIN_ISP_CLK_SRC].clock)) {
		vind->isp_clk[VIN_ISP_CLK].clock = NULL;
		vin_warn("get csi isp src clk fail\n");
	}
	/*get mipi clk*/
	vind->mipi_clk[VIN_MIPI_CLK].clock = devm_clk_get(dev, "csi_mipi");
	if (IS_ERR(vind->mipi_clk[VIN_MIPI_CLK].clock)) {
		vind->mipi_clk[VIN_MIPI_CLK].clock = NULL;
		vin_warn("get csi mipi clk fail\n");
	}
	vind->mipi_clk[VIN_MIPI_CLK_SRC].clock = devm_clk_get(dev, "csi_mipi_src");
	if (IS_ERR(vind->mipi_clk[VIN_MIPI_CLK_SRC].clock)) {
		vind->mipi_clk[VIN_MIPI_CLK_SRC].clock = NULL;
		vin_warn("get csi mipi src clk fail\n");
	}
	/*get bus clk*/
	vind->bus_clk[VIN_CSI_BUS_CLK] = devm_clk_get(dev, "csi_bus");
	if (IS_ERR_OR_NULL(vind->bus_clk[VIN_CSI_BUS_CLK])) {
		vin_err("Get csi bus clk failed!\n");
		return PTR_ERR(vind->bus_clk[VIN_CSI_BUS_CLK]);
	}
	vind->bus_clk[VIN_CSI_MBUS_CLK] = devm_clk_get(dev, "csi_mbus");
	if (IS_ERR_OR_NULL(vind->bus_clk[VIN_CSI_MBUS_CLK])) {
		vin_err("Get csi mbus clk failed!\n");
		return PTR_ERR(vind->bus_clk[VIN_CSI_MBUS_CLK]);
	}
	vind->bus_clk[VIN_ISP_MBUS_CLK] = devm_clk_get(dev, "csi_isp_mbus");
	if (IS_ERR(vind->bus_clk[VIN_ISP_MBUS_CLK])) {
		vind->bus_clk[VIN_ISP_MBUS_CLK] = NULL;
		vin_warn("get csi isp clk fail\n");
	}
	/*get csi/isp reset*/
	vind->clk_reset[VIN_CSI_RET] = devm_reset_control_get(dev, "csi_ret");
	if (IS_ERR(vind->clk_reset[VIN_CSI_RET])) {
		vin_err("Get csi reset control fail\n");
		return PTR_ERR(vind->clk_reset[VIN_CSI_RET]);
	}
	vind->clk_reset[VIN_ISP_RET] = devm_reset_control_get(dev, "isp_ret");
	if (IS_ERR(vind->clk_reset[VIN_ISP_RET])) {
		vin_warn("Get isp reset control fail\n");
		vind->clk_reset[VIN_ISP_RET] = NULL;
	}
	/*get csi clk rate*/
	if (clk_set_parent(vind->clk[VIN_TOP_CLK].clock, vind->clk[VIN_TOP_CLK_SRC].clock)) {
		vin_err("vin top clock set parent failed\n");
		return -1;
	}
	if (of_property_read_u32(np, "csi_top", &core_clk)) {
		vin_err("vin failed to get core clk\n");
		vind->clk[VIN_TOP_CLK].frequency = VIN_CLK_RATE;
	} else {
		vin_log(VIN_LOG_MD, "vin get core clk = %d\n", core_clk);
		vind->clk[VIN_TOP_CLK].frequency = core_clk;
	}
	/*get isp clk rate*/
	if (vind->isp_clk[VIN_ISP_CLK].clock && vind->isp_clk[VIN_ISP_CLK_SRC].clock) {
		if (clk_set_parent(vind->isp_clk[VIN_ISP_CLK].clock,
					vind->isp_clk[VIN_ISP_CLK_SRC].clock)) {
			vin_err("isp clock set parent failed\n");
			return -1;
		}

		if (of_property_read_u32(np, "csi_isp", &core_clk)) {
			vin_err("vin failed to get isp clk rate\n");
			vind->isp_clk[VIN_ISP_CLK].frequency = ISP_CLK_RATE;
		} else {
			vin_log(VIN_LOG_MD, "vin get isp clk rate = %d\n", core_clk);
			vind->isp_clk[VIN_ISP_CLK].frequency = core_clk;
		}
	}
	return 0;
}

static void vin_md_put_clocks(struct vin_md *vind)
{
#ifndef FPGA_VER
	int i;

	for (i = 0; i < VIN_MAX_CLK; i++) {
		if (vind->clk[i].clock)
			clk_put(vind->clk[i].clock);
	}

	for (i = 0; i < VIN_MAX_CCI; i++) {
		if (vind->mclk[i].mclk)
			clk_put(vind->mclk[i].mclk);
		if (vind->mclk[i].clk_24m)
			clk_put(vind->mclk[i].clk_24m);
		if (vind->mclk[i].clk_pll)
			clk_put(vind->mclk[i].clk_pll);
	}

	if (vind->isp_clk[VIN_ISP_CLK].clock)
		clk_put(vind->isp_clk[VIN_ISP_CLK].clock);
	if (vind->isp_clk[VIN_ISP_CLK_SRC].clock)
		clk_put(vind->isp_clk[VIN_ISP_CLK_SRC].clock);

	for (i = 0; i < VIN_MIPI_MAX_CLK; i++) {
		if (vind->mipi_clk[i].clock)
			clk_put(vind->mipi_clk[i].clock);
	}
	if (vind->bus_clk[VIN_CSI_BUS_CLK])
		clk_put(vind->bus_clk[VIN_ISP_CLK]);
	if (vind->bus_clk[VIN_CSI_MBUS_CLK])
		clk_put(vind->bus_clk[VIN_ISP_CLK]);
	if (vind->bus_clk[VIN_ISP_MBUS_CLK])
		clk_put(vind->bus_clk[VIN_ISP_CLK]);
#endif
}

static int __vin_set_top_clk_rate(struct vin_md *vind, unsigned int rate)
{
	if (rate >= 300000000)
		vind->clk[VIN_TOP_CLK_SRC].frequency = rate;
	else if (rate >= 150000000)
		vind->clk[VIN_TOP_CLK_SRC].frequency = rate * 2;
	else if (rate >= 75000000)
		vind->clk[VIN_TOP_CLK_SRC].frequency = rate * 4;
	else
		vind->clk[VIN_TOP_CLK_SRC].frequency = VIN_CLK_RATE;

#ifndef CONFIG_ARCH_SUN50IW3P1
	if (clk_set_rate(vind->clk[VIN_TOP_CLK_SRC].clock,
	    vind->clk[VIN_TOP_CLK_SRC].frequency)) {
		vin_err("set vin top clock source rate error\n");
		return -1;
	}
#endif

	if (clk_set_rate(vind->clk[VIN_TOP_CLK].clock, rate)) {
		vin_err("set vin top clock rate error\n");
		return -1;
	}
	vin_log(VIN_LOG_POWER, "vin top clk get rate = %ld\n",
		clk_get_rate(vind->clk[VIN_TOP_CLK].clock));

	return 0;
}

static int __vin_set_isp_clk_rate(struct vin_md *vind, unsigned int rate)
{
	if (rate >= 300000000)
		vind->isp_clk[VIN_ISP_CLK_SRC].frequency = rate;
	else if (rate >= 150000000)
		vind->isp_clk[VIN_ISP_CLK_SRC].frequency = rate * 2;
	else if (rate >= 75000000)
		vind->isp_clk[VIN_ISP_CLK_SRC].frequency = rate * 4;
	else
		vind->isp_clk[VIN_ISP_CLK_SRC].frequency = ISP_CLK_RATE;

#if defined CONFIG_ARCH_SUN8IW16P1
	if (clk_set_rate(vind->isp_clk[VIN_ISP_CLK_SRC].clock,
	    vind->isp_clk[VIN_ISP_CLK_SRC].frequency)) {
		vin_err("set vin isp clock source rate error\n");
		return -1;
	}
#endif
	if (clk_set_rate(vind->isp_clk[VIN_ISP_CLK].clock, rate)) {
		vin_err("set vin isp clock rate error\n");
		return -1;
	}
	vin_log(VIN_LOG_POWER, "vin isp clk get rate = %ld\n",
		clk_get_rate(vind->isp_clk[VIN_ISP_CLK].clock));

	return 0;
}

static int vin_md_clk_enable(struct vin_md *vind)
{
#ifndef FPGA_VER
	int ret;

	if (vind->clk[VIN_TOP_CLK].clock) {
		__vin_set_top_clk_rate(vind, vind->clk[VIN_TOP_CLK].frequency);
		ret = reset_control_deassert(vind->clk_reset[VIN_CSI_RET]);
		if (ret) {
			vin_err("reset deassert fail\n");
			return ret;
		}
		ret = clk_prepare_enable(vind->bus_clk[VIN_CSI_BUS_CLK]);
		if (ret) {
			vin_err("csi bus clk prepare enable fail\n");
			goto assert_reset_csi;
		}
		ret = clk_prepare_enable(vind->bus_clk[VIN_CSI_MBUS_CLK]);
		if (ret) {
			vin_err("csi mbus clk prepare enable fail\n");
			goto enable_csi_bus;
		}

		ret = clk_prepare_enable(vind->clk[VIN_TOP_CLK].clock);
		if (ret) {
			vin_err("csi clk prepare enable fail\n");
			goto enable_csi_mbus;
		}
	}

	if (vind->isp_clk[VIN_ISP_CLK].clock) {
		if (vind->isp_clk[VIN_ISP_CLK_SRC].clock)
			__vin_set_isp_clk_rate(vind, vind->isp_clk[VIN_ISP_CLK].frequency);
		ret = reset_control_deassert(vind->clk_reset[VIN_ISP_RET]);
		if (ret) {
			goto enable_csi_clk;
		}
		ret = clk_prepare_enable(vind->bus_clk[VIN_ISP_MBUS_CLK]);
		if (ret) {
			vin_err("csi mbus clk prepare enable fail\n");
			goto assert_reset_isp;
		}
		ret = clk_prepare_enable(vind->isp_clk[VIN_ISP_CLK].clock);
		if (ret)
			goto enable_isp_mbus;

	}

	if (vind->mipi_clk[VIN_MIPI_CLK].clock)
		clk_prepare_enable(vind->mipi_clk[VIN_MIPI_CLK].clock);

	return 0;
enable_isp_mbus:
	clk_disable_unprepare(vind->bus_clk[VIN_ISP_MBUS_CLK]);
assert_reset_isp:
	reset_control_assert(vind->clk_reset[VIN_ISP_RET]);
enable_csi_clk:
	clk_disable_unprepare(vind->clk[VIN_TOP_CLK].clock);
enable_csi_mbus:
	clk_disable_unprepare(vind->bus_clk[VIN_CSI_MBUS_CLK]);
enable_csi_bus:
	clk_disable_unprepare(vind->bus_clk[VIN_CSI_BUS_CLK]);
assert_reset_csi:
	reset_control_assert(vind->clk_reset[VIN_CSI_RET]);

	return ret;
#else
	void __iomem *clk_base;
	void __iomem *gpio_base;
	int ret;

	vin_log(VIN_LOG_MD, "directly write pin and clk config @ FPGA\n");
	if (vind->clk[VIN_TOP_CLK].clock) {
		ret = reset_control_deassert(vind->clk_reset[VIN_CSI_RET]);
		if (ret) {
			vin_err("reset deassert fail\n");
			return ret;
		}
		ret = clk_prepare_enable(vind->bus_clk[VIN_CSI_BUS_CLK]);
		if (ret) {
			vin_err("csi bus clk prepare enable fail\n");
			goto assert_reset_csi;
		}
		ret = clk_prepare_enable(vind->bus_clk[VIN_CSI_MBUS_CLK]);
		if (ret) {
			vin_err("csi mbus clk prepare enable fail\n");
			goto enable_csi_bus;
		}

		ret = clk_prepare_enable(vind->clk[VIN_TOP_CLK].clock);
		if (ret) {
			vin_err("csi clk prepare enable fail\n");
			goto enable_csi_mbus;
		}
	}

	if (vind->isp_clk[VIN_ISP_CLK].clock) {
		ret = reset_control_deassert(vind->clk_reset[VIN_ISP_RET]);
		if (ret) {
			goto enable_csi_clk;
		}
		ret = clk_prepare_enable(vind->bus_clk[VIN_ISP_MBUS_CLK]);
		if (ret) {
			vin_err("csi mbus clk prepare enable fail\n");
			goto assert_reset_isp;
		}
		ret = clk_prepare_enable(vind->isp_clk[VIN_ISP_CLK].clock);
		if (ret)
			goto enable_isp_mbus;

	}

	if (vind->mipi_clk[VIN_MIPI_CLK].clock)
		clk_prepare_enable(vind->mipi_clk[VIN_MIPI_CLK].clock);




	gpio_base = ioremap(0x02001000, 0x1000);
	if (!gpio_base) {
		vin_print("csi clk ioremap failed\n");
		return -EIO;
	}
	writel(0x00010001, (gpio_base + 0xc1c)); /*CSI RET GATING*/
	return 0;

enable_isp_mbus:
	clk_disable_unprepare(vind->bus_clk[VIN_ISP_MBUS_CLK]);
assert_reset_isp:
	reset_control_assert(vind->clk_reset[VIN_ISP_RET]);
enable_csi_clk:
	clk_disable_unprepare(vind->clk[VIN_TOP_CLK].clock);
enable_csi_mbus:
	clk_disable_unprepare(vind->bus_clk[VIN_CSI_MBUS_CLK]);
enable_csi_bus:
	clk_disable_unprepare(vind->bus_clk[VIN_CSI_BUS_CLK]);
assert_reset_csi:
	reset_control_assert(vind->clk_reset[VIN_CSI_RET]);

	return ret;
#endif
}

static void vin_md_clk_disable(struct vin_md *vind)
{
#ifndef FPGA_VER
	if (vind->clk[VIN_TOP_CLK].clock) {
		clk_disable_unprepare(vind->clk[VIN_TOP_CLK].clock);
		clk_disable_unprepare(vind->bus_clk[VIN_CSI_MBUS_CLK]);
		clk_disable_unprepare(vind->bus_clk[VIN_CSI_BUS_CLK]);
		reset_control_assert(vind->clk_reset[VIN_CSI_RET]);
	}

	if (vind->isp_clk[VIN_ISP_CLK].clock) {
		clk_disable_unprepare(vind->isp_clk[VIN_ISP_CLK].clock);
		clk_disable_unprepare(vind->bus_clk[VIN_ISP_MBUS_CLK]);
		reset_control_assert(vind->clk_reset[VIN_ISP_RET]);

	}

	if (vind->mipi_clk[VIN_MIPI_CLK].clock)
		clk_disable_unprepare(vind->mipi_clk[VIN_MIPI_CLK].clock);
#endif
}

#if !defined NO_SUPPROT_CCU_PLATDORM
static void vin_ccu_clk_gating_en(unsigned int en)
{
	if (en) {
		csic_ccu_clk_gating_enable();
		csic_ccu_mcsi_clk_mode(1);
		csic_ccu_mcsi_post_clk_enable(0);
		csic_ccu_mcsi_post_clk_enable(1);
	} else {
		csic_ccu_mcsi_post_clk_disable(1);
		csic_ccu_mcsi_post_clk_disable(0);
		csic_ccu_mcsi_clk_mode(0);
		csic_ccu_clk_gating_disable();
	}
}

static void vin_subdev_ccu_en(struct v4l2_subdev *sd, unsigned int en)
{
	__maybe_unused struct mipi_dev *mipi = NULL;
	__maybe_unused struct csi_dev *csi = NULL;
	__maybe_unused struct isp_dev *isp = NULL;
	struct scaler_dev *scaler = NULL;
	struct vin_core *vinc = NULL;
	void *dev = v4l2_get_subdevdata(sd);

	if (dev == NULL) {
		vin_err("%s subdev is NULL, cannot set ccu\n", sd->name);
		return;
	}

	switch (sd->grp_id) {
#if !defined (CONFIG_ARCH_SUN50IW10P1)
	case VIN_GRP_ID_MIPI:
		mipi = (struct mipi_dev *)dev;
#if defined (CONFIG_ARCH_SUN8IW16P1)
		csic_ccu_mcsi_combo_clk_en(mipi->id, en);
#else
		csic_ccu_mcsi_mipi_clk_en(mipi->id, en);
#endif
		break;
#endif
#ifndef SUPPORT_ISP_TDM
	case VIN_GRP_ID_CSI:
		csi = (struct csi_dev *)dev;
		csic_ccu_mcsi_parser_clk_en(csi->id, en);
		break;
	case VIN_GRP_ID_ISP:
		isp = (struct isp_dev *)dev;
		csic_ccu_misp_isp_clk_en(isp->id, en);
		break;
#endif
	case VIN_GRP_ID_SCALER:
		scaler = (struct scaler_dev *)dev;
		csic_ccu_vipp_clk_en(scaler->id, en);
		break;
	case VIN_GRP_ID_CAPTURE:
		vinc = (struct vin_core *)dev;
		csic_ccu_bk_clk_en(vinc->vipp_sel, en);
		break;
	default:
		break;
	}
}
#endif

static void vin_md_set_power(struct vin_md *vind, int on)
{
	__maybe_unused int i;

	if (on && (vind->use_count)++ > 0)
		return;
	else if (!on && (vind->use_count == 0 || --(vind->use_count) > 0))
		return;

	if (on) {
		vin_md_clk_enable(vind);
		usleep_range(100, 120);
#if !defined NO_SUPPROT_CCU_PLATDORM
		vin_ccu_clk_gating_en(1);
		csic_isp_bridge_enable(vind->id);
#endif
#if defined (CONFIG_ARCH_SUN50IW10P1)
		csic_ccu_mcsi_combo_clk_en(0, 1);
#endif
#ifdef SUPPORT_ISP_TDM
		for (i = 0; i < VIN_MAX_CSI; i++)
			csic_ccu_mcsi_parser_clk_en(i, 1);
		for (i = 0; i < VIN_MAX_ISP; i++)
			csic_ccu_misp_isp_clk_en(i, 1);
#endif
		csic_top_enable(vind->id);
		csic_top_version_read_en(vind->id, 1);
		csic_feature_list_get(vind->id, &vind->csic_fl);
		csic_version_get(vind->id, &vind->csic_ver);
		csic_top_version_read_en(vind->id, 0);
		csic_mbus_req_mex_set(vind->id, 0xf);
#ifdef CONFIG_MULTI_FRAME
		csic_mulp_mode_en(vind->id, 1);
		csic_mulp_dma_cs(vind->id, CSIC_MULF_DMA0_CS);
		csic_mulp_int_enable(vind->id, MULF_DONE | MULF_ERR);
#endif

#if defined (CONFIG_ARCH_SUN50IW10P1)
		cmb_phy_top_enable();
#endif
	} else {
#if defined (CONFIG_ARCH_SUN50IW10P1)
		cmb_phy_top_disable();
#endif

#ifdef CONFIG_MULTI_FRAME
		csic_mulp_int_disable(vind->id, MULF_ALL);
		csic_mulp_mode_en(vind->id, 0);
#endif
		csic_top_disable(vind->id);
#if !defined NO_SUPPROT_CCU_PLATDORM
		csic_isp_bridge_disable(vind->id);
		vin_ccu_clk_gating_en(0);
#endif
		vin_md_clk_disable(vind);
	}
}

static void vin_set_cci_power(struct vin_md *vind, int on)
{
#if defined (CONFIG_ARCH_SUN50IW9P1)
	int i;

	if (on) {
		vin_md_set_power(vind, on);
		for (i = 0; i < VIN_MAX_CSI; i++)
			csic_ccu_mcsi_parser_clk_en(i, on);
	} else {
		for (i = 0; i < VIN_MAX_CSI; i++)
			csic_ccu_mcsi_parser_clk_en(i, on);
		vin_md_set_power(vind, on);
	}
#endif
}

static int vin_gpio_request(struct vin_md *vind)
{
#ifndef FPGA_VER
	unsigned int i, num;
	struct sensor_list *sl = NULL;
	int *gpio = NULL;

	for (num = 0; num < VIN_MAX_DEV; num++) {
		sl = &vind->modules[num].sensors;

		for (i = 0; i < MAX_GPIO_NUM; i++) {
			gpio = &sl->gpio[i];
			if (gpio != NULL && *gpio >= 0) {
				if (gpio_request(*gpio, NULL) < 0) {
					vin_log(VIN_LOG_MD, "gpio%d request failed!\n", *gpio);
					continue;
				}
				vin_log(VIN_LOG_MD, "gpio%d request success!\n", *gpio);
			}

		}
	}
#endif
	return 0;
}

static void vin_gpio_release(struct vin_md *vind)
{
#ifndef FPGA_VER
	unsigned int i, num;
	struct sensor_list *sl = NULL;
	int *gpio = NULL;

	for (num = 0; num < VIN_MAX_DEV; num++) {
		sl = &vind->modules[num].sensors;

		for (i = 0; i < MAX_GPIO_NUM; i++) {
			gpio = &sl->gpio[i];
			if (gpio != NULL && *gpio >= 0)
					gpio_free(*gpio);
		}
	}
#endif
}

static void __vin_pattern_config(struct vin_md *vind, struct vin_core *vinc, int on)
{
#ifdef SUPPORT_PTN
	int port_sel = 2;

	if (vinc->ptn_cfg.ptn_en && on) {
		if (vinc->csi_sel == 1)
			port_sel = 2;
		else
			port_sel = vinc->csi_sel + 2;
		csic_ptn_control(vind->id, vinc->ptn_cfg.ptn_mode, vinc->ptn_cfg.ptn_dw, port_sel);
		csic_ptn_length(vind->id, vinc->ptn_cfg.ptn_buf.size);
		csic_ptn_addr(vind->id, (unsigned long)vinc->ptn_cfg.ptn_buf.dma_addr);
		csic_ptn_size(vind->id, vinc->ptn_cfg.ptn_w, vinc->ptn_cfg.ptn_h);
	} else {
		csic_ptn_generation_en(vind->id, 0);
	}
#endif
}

static void __vin_pattern_onoff(struct vin_md *vind, struct vin_core *vinc, int on)
{
#ifdef SUPPORT_PTN
	if (vinc->ptn_cfg.ptn_en) {
		if (vinc->ptn_cfg.ptn_type > 0) {
			ptn_on_cnt++;
			if (ptn_on_cnt%vinc->ptn_cfg.ptn_type == 0) {
				ptn_on_cnt = 0;
				ptn_frame_cnt = 0;
				csic_ptn_generation_en(vind->id, on);
			}
		} else {
			csic_ptn_generation_en(vind->id, on);
		}
	}
#endif
}

static int __vin_subdev_set_power(struct v4l2_subdev *sd, unsigned int idx, int on)
{
	int *use_count;
	int ret;

	if (sd == NULL)
		return -ENXIO;

	use_count = &sd->entity.use_count;
	if (on && (*use_count)++ > 0)
		return 0;
	else if (!on && (*use_count == 0 || --(*use_count) > 0))
		return 0;

#if defined SENSOR_POER_BEFORE_VIN
	if (idx == VIN_IND_SENSOR)
		return 0;
#endif

#if !defined NO_SUPPROT_CCU_PLATDORM
	if (on)
		vin_subdev_ccu_en(sd, on);
#endif
	ret = v4l2_subdev_call(sd, core, s_power, on);
#if !defined NO_SUPPROT_CCU_PLATDORM
	if (!on)
		vin_subdev_ccu_en(sd, on);
#endif

#if 0
	if (ret == 0 || ret == -ENOIOCTLCMD) {
		if (on || sd->grp_id != VIN_GRP_ID_SENSOR)
			ret = v4l2_subdev_call(sd, core, init, on);
	}
#endif
	return ret != -ENOIOCTLCMD ? ret : 0;
}

static int vin_pipeline_s_power(struct vin_pipeline *p, bool on)
{
	static const u8 seq[2][VIN_IND_MAX] = {
		{ VIN_IND_ISP, VIN_IND_SENSOR, VIN_IND_CSI, VIN_IND_MIPI,
			VIN_IND_SCALER, VIN_IND_CAPTURE  },
		{ VIN_IND_CAPTURE, VIN_IND_MIPI, VIN_IND_CSI, VIN_IND_SENSOR,
			VIN_IND_ISP, VIN_IND_SCALER},
	};
	int i, ret = 0;

	for (i = 0; i < VIN_IND_TDM_RX; i++) {
		unsigned int idx = seq[on][i];
		if (!p->sd[idx] || !p->sd[idx]->entity.graph_obj.mdev)
			continue;
		ret = __vin_subdev_set_power(p->sd[idx], idx, on);
		if (ret < 0 && ret != -ENXIO)
			goto error;
	}
	return 0;
error:
	for (; i >= 0; i--) {
		unsigned int idx = seq[on][i];
		if (!p->sd[idx] || !p->sd[idx]->entity.graph_obj.mdev)
			continue;
		__vin_subdev_set_power(p->sd[idx], idx, !on);
	}
	return ret;
}

static int __vin_pipeline_open(struct vin_pipeline *p,
				struct media_entity *me, bool prepare)
{
	struct vin_md *vind;
	int ret;

	if (WARN_ON(p == NULL || me == NULL))
		return -EINVAL;

	if (prepare)
		vin_md_prepare_pipeline(p, me);

	vind = entity_to_vin_mdev(me);
	if (vind)
		vin_md_set_power(vind, 1);

	ret = vin_pipeline_s_power(p, 1);
	if (!ret)
		return 0;
	return ret;
}

static int __vin_pipeline_close(struct vin_pipeline *p)
{
	struct v4l2_subdev *sd = p ? p->sd[VIN_IND_SENSOR] : NULL;
	struct vin_md *vind;
	int ret = 0;

	if (WARN_ON(sd == NULL))
		return -EINVAL;

	if (p->sd[VIN_IND_SENSOR])
		ret = vin_pipeline_s_power(p, 0);

	vind = entity_to_vin_mdev(&sd->entity);
	if (vind)
		vin_md_set_power(vind, 0);

	return ret == -ENXIO ? 0 : ret;
}

static int __vin_subdev_set_stream(struct v4l2_subdev *sd, unsigned int idx, int on)
{
	__maybe_unused struct isp_dev *isp = NULL;
	int *stream_count;
	int ret;

	if (sd == NULL)
		return -ENODEV;

	stream_count = &sd->entity.stream_count;
	if (on && (*stream_count)++ > 0)
		return 0;
	else if (!on && (*stream_count == 0 || --(*stream_count) > 0))
		return 0;

#if defined ISP0_BRIDGE_VALID
	if (!on && idx == VIN_IND_ISP) {
		isp = v4l2_get_subdevdata(sd);
		if (isp->id == 0)
			csic_isp_bridge_disable(0);
	}
#endif
	ret = v4l2_subdev_call(sd, video, s_stream, on);
#if defined ISP0_BRIDGE_VALID
	if (on && idx == VIN_IND_ISP) {
		isp = v4l2_get_subdevdata(sd);
		if (isp->id == 0)
			csic_isp_bridge_enable(0);
	}
#endif

	return ret != -ENOIOCTLCMD ? ret : 0;
}

static int __vin_pipeline_s_stream(struct vin_pipeline *p, int on_idx)
{
	static const u8 seq[3][VIN_IND_MAX] = {
		{ VIN_IND_CAPTURE, VIN_IND_ISP, VIN_IND_SENSOR, VIN_IND_CSI, VIN_IND_MIPI,
			VIN_IND_SCALER, VIN_IND_TDM_RX },
		{ VIN_IND_TDM_RX, VIN_IND_SENSOR, VIN_IND_MIPI, VIN_IND_ISP,
			VIN_IND_SCALER, VIN_IND_CAPTURE, VIN_IND_CSI},
		{ VIN_IND_TDM_RX, VIN_IND_MIPI, VIN_IND_SENSOR, VIN_IND_ISP,
			VIN_IND_SCALER, VIN_IND_CAPTURE, VIN_IND_CSI},
	};
	struct v4l2_mbus_config mcfg;
	struct vin_core *vinc = NULL;
	struct vin_md *vind = NULL;
	int i, on, ret = 0;

	if (p == NULL) {
		vin_err("pipeline is NULL, cannot s_stream\n");
		return -ENODEV;
	}

	if (WARN_ON(p->sd[VIN_IND_SENSOR] == NULL))
		return -ENODEV;

	vind = entity_to_vin_mdev(&p->sd[VIN_IND_SENSOR]->entity);
	if (vind == NULL) {
		vin_err("vin media is NULL, cannot s_stream\n");
		return -ENODEV;
	}

	vinc = v4l2_get_subdevdata(p->sd[VIN_IND_CAPTURE]);
	if (vinc == NULL) {
		vin_err("vin video is NULL, cannot s_stream\n");
		return -ENODEV;
	}

	if (on_idx) {
		v4l2_subdev_call(p->sd[VIN_IND_SENSOR], pad, get_mbus_config, 0, &mcfg);
#if defined CONFIG_ARCH_SUN8IW16P1
		ret = sensor_get_clk(p->sd[VIN_IND_SENSOR], &mcfg, &vind->clk[VIN_TOP_CLK].frequency,
			&vind->isp_clk[VIN_ISP_CLK].frequency);
		if (!ret) {
			__vin_set_top_clk_rate(vind, vind->clk[VIN_TOP_CLK].frequency);
			if (vind->isp_clk[VIN_ISP_CLK_SRC].clock)
				__vin_set_isp_clk_rate(vind, vind->isp_clk[VIN_ISP_CLK].frequency);
		}
#endif
		/*vin change top clk rate*/
		if (vinc->vin_clk && (vinc->vin_clk != vind->clk[VIN_TOP_CLK].frequency)) {
			__vin_set_top_clk_rate(vind, vinc->vin_clk);
			vind->clk[VIN_TOP_CLK].frequency = vinc->vin_clk;
		}

#if defined (CONFIG_ARCH_SUN50IW9P1)
		csic_dma_input_select(vind->id, vinc->vipp_sel, vinc->csi_sel, vinc->isp_tx_ch);
#else
		for (i = 0; i < vinc->total_rx_ch; i++)
			csic_isp_input_select(vind->id, vinc->isp_sel, i, vinc->csi_sel, i);
		csic_vipp_input_select(vind->id, vinc->vipp_sel, vinc->isp_sel, vinc->isp_tx_ch);
#endif
	}

	on = on_idx ? 1 : 0;

	__vin_pattern_config(vind, vinc, on);

	for (i = 0; i < VIN_IND_ACTUATOR; i++) {
		unsigned int idx = seq[on_idx][i];
		if (!p->sd[idx] || !p->sd[idx]->entity.graph_obj.mdev)
			continue;
		if (vinc->ptn_cfg.ptn_en && (idx <= VIN_IND_MIPI))
			continue;
		ret = __vin_subdev_set_stream(p->sd[idx], idx, on);
		if (ret < 0 && ret != -ENODEV) {
			vin_err("%s error!\n", __func__);
			goto error;
		}
		usleep_range(100, 120);
	}

	__vin_pattern_onoff(vind, vinc, on);

	return 0;
error:
	for (; i >= 0; i--) {
		unsigned int idx = seq[on_idx][i];
		if (!p->sd[idx] || !p->sd[idx]->entity.graph_obj.mdev)
			continue;
		if (vinc->ptn_cfg.ptn_en && (idx <= VIN_IND_MIPI))
			continue;
		__vin_subdev_set_stream(p->sd[idx], idx, !on);
	}
	return ret;
}

static const struct vin_pipeline_ops vin_pipe_ops = {
	.open		= __vin_pipeline_open,
	.close		= __vin_pipeline_close,
	.set_stream	= __vin_pipeline_s_stream,
};

static struct v4l2_subdev *__vin_subdev_register(struct vin_md *vind,
			char *name, u8 addr, enum module_type type, int bus_sel)
{
	struct v4l2_device *v4l2_dev = &vind->v4l2_dev;
	struct v4l2_subdev *sd = NULL;

	if (type == VIN_MODULE_TYPE_CCI) {
		sd = cci_bus_match(name, bus_sel, addr);
		if (IS_ERR_OR_NULL(sd)) {
			vin_err("registering %s, No such device!\n", name);
			return NULL;
		} else {
			if (v4l2_device_register_subdev(v4l2_dev, sd)) {
				struct cci_driver *cd = v4l2_get_subdevdata(sd);

				cci_bus_match_cancel(cd);
				vin_log(VIN_LOG_MD, "%s register failed!\n", name);
				return NULL;
			}
			vin_log(VIN_LOG_MD, "%s register OK!\n", name);
		}
	} else if (type == VIN_MODULE_TYPE_I2C) {
		struct i2c_adapter *adapter = i2c_get_adapter(bus_sel);

		if (adapter == NULL) {
			vin_err("%s request i2c%d adapter failed!\n", name, bus_sel);
			return NULL;
		}
		sd = v4l2_i2c_new_subdev(v4l2_dev, adapter, name, addr, NULL);
		if (IS_ERR_OR_NULL(sd)) {
			i2c_put_adapter(adapter);
			vin_err("registering %s, No such device!\n", name);
			return NULL;
		} else {
			vin_log(VIN_LOG_MD, "%s register OK!\n", name);
		}
	} else if (type == VIN_MODULE_TYPE_SPI) {
#if defined(CONFIG_SPI)
		struct spi_master *master = spi_busnum_to_master(bus_sel);
		/*if use struct spi_board_info info diretly, maybe leadto stack overflow!*/
		struct spi_board_info *info = NULL;

		if (master == NULL) {
			vin_err("%s request spi%d master failed!\n", name, bus_sel);
			return NULL;
		}

		info = kzalloc(sizeof(struct spi_board_info), GFP_KERNEL);
		if (info == NULL)
			return NULL;
		strlcpy(info->modalias, name, sizeof(info->modalias));
		info->bus_num = bus_sel;
		info->chip_select = 0;
		info->max_speed_hz = 12000000;
		info->mode = 0x0b; /*0x08 (little end) | 0x03*/
		sd = v4l2_spi_new_subdev(v4l2_dev, master, info);
		kfree(info);
		if (IS_ERR_OR_NULL(sd)) {
			spi_master_put(master);
			vin_err("registering %s, No such device!\n", name);
			return NULL;
		} else {
			vin_log(VIN_LOG_MD, "%s register OK!\n", name);
		}
#endif
	} else if (type == VIN_MODULE_TYPE_GPIO) {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return NULL;
	} else {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return NULL;
	}

	return sd;
}

static int __vin_subdev_unregister(struct v4l2_subdev *sd,
				enum module_type type)
{
	if (IS_ERR_OR_NULL(sd)) {
		vin_log(VIN_LOG_MD, "%s sd = NULL!\n", __func__);
		return -1;
	}

	if (type == VIN_MODULE_TYPE_CCI) {
		struct cci_driver *cci_driv = v4l2_get_subdevdata(sd);

		if (IS_ERR_OR_NULL(cci_driv))
			return -ENODEV;
		vin_log(VIN_LOG_MD, "vin sd %s unregister!\n", sd->name);
		v4l2_device_unregister_subdev(sd);
		cci_bus_match_cancel(cci_driv);
	} else if (type == VIN_MODULE_TYPE_I2C) {
		struct i2c_adapter *adapter;
		struct i2c_client *client = v4l2_get_subdevdata(sd);

		if (!client)
			return -ENODEV;
		vin_log(VIN_LOG_MD, "vin sd %s unregister!\n", sd->name);
		v4l2_device_unregister_subdev(sd);
		adapter = client->adapter;
		i2c_unregister_device(client);
		if (adapter)
			i2c_put_adapter(adapter);
	} else if (type == VIN_MODULE_TYPE_SPI) {
#if defined(CONFIG_SPI)
		struct spi_master *master;
		struct spi_device *spi = v4l2_get_subdevdata(sd);

		if (!spi)
			return -ENODEV;
		vin_log(VIN_LOG_MD, "vin sd %s unregister!\n", sd->name);
		v4l2_device_unregister_subdev(sd);
		master = spi->master;
		spi_unregister_device(spi);
		if (master)
			spi_master_put(master);
#endif
	} else if (type == VIN_MODULE_TYPE_GPIO) {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return -EFAULT;
	} else {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return -EFAULT;
	}

	return 0;
}

static int __vin_handle_sensor_info(struct sensor_instance *inst)
{
	if (inst->cam_type == SENSOR_RAW) {
		inst->is_bayer_raw = 1;
		inst->is_isp_used = 1;
	} else if (inst->cam_type == SENSOR_YUV) {
		inst->is_bayer_raw = 0;
		inst->is_isp_used = 0;
	} else {
		inst->is_bayer_raw = 0;
		inst->is_isp_used = 0;
	}
	return 0;
}

static struct v4l2_subdev *__vin_register_module(struct vin_md *vind,
			struct modules_config *module, int i)
{
	struct sensor_instance *inst = &module->sensors.inst[i];
	struct vin_module_info *modules = &module->modules;

	if (!strcmp(inst->cam_name, "")) {
		vin_err("Sensor name is NULL!\n");
		modules->sensor[i].sd = NULL;
		return modules->sensor[i].sd;
	}

	/*camera sensor register. */
	modules->sensor[i].sd = __vin_subdev_register(vind, inst->cam_name,
						inst->cam_addr >> 1,
						modules->sensor[i].type,
						module->sensors.sensor_bus_sel);
	if (module->sensors.use_sensor_list == 1)
		module->act_used = inst->act_used;

	if (!module->act_used || !modules->sensor[i].sd) {
		modules->act[i].sd = NULL;
		return modules->sensor[i].sd;
	}
	/*camera act register. */
	modules->act[i].sd = __vin_subdev_register(vind, inst->act_name,
						inst->act_addr >> 1,
						modules->act[i].type,
						module->sensors.act_bus_sel);
	return modules->sensor[i].sd;
}

static void __vin_unregister_module(struct modules_config *module, int i)
{
	struct vin_module_info *modules = &module->modules;

	/*camera subdev unregister */
	__vin_subdev_unregister(modules->sensor[i].sd,
		modules->sensor[i].type);
	__vin_subdev_unregister(modules->act[i].sd,
		modules->act[i].type);
	vin_log(VIN_LOG_MD, "%s!\n", __func__);
	modules->sensor[i].sd = NULL;
	modules->act[i].sd = NULL;
}

static int vin_md_link_notify(struct media_link *link, u32 flags,
				unsigned int notification)
{
	if (notification == MEDIA_DEV_NOTIFY_POST_LINK_CH)
		vin_log(VIN_LOG_MD, "%s: source %s, sink %s, flag %d\n", __func__,
			link->source->entity->name,
			link->sink->entity->name, flags);
	return 0;
}

const struct media_device_ops media_device_ops = {
		.link_notify = vin_md_link_notify,
};

static ssize_t vin_md_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct vin_md *vind = platform_get_drvdata(pdev);

	if (vind->user_subdev_api)
		return strlcpy(buf, "Sub-device API (sub-dev)\n", PAGE_SIZE);

	return strlcpy(buf, "V4L2 video node only API (vid-dev)\n", PAGE_SIZE);
}

static ssize_t vin_md_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct vin_md *vind = platform_get_drvdata(pdev);
	bool subdev_api;
	int i;

	if (!strcmp(buf, "vid-dev\n"))
		subdev_api = false;
	else if (!strcmp(buf, "sub-dev\n"))
		subdev_api = true;
	else
		return count;

	vind->user_subdev_api = subdev_api;
	for (i = 0; i < VIN_MAX_DEV; i++)
		if (vind->vinc[i])
			vind->vinc[i]->vid_cap.user_subdev_api = subdev_api;
	return count;
}

static DEVICE_ATTR(subdev_api, S_IWUSR | S_IRUGO,
		   vin_md_sysfs_show, vin_md_sysfs_store);

static int vin_md_register_core_entity(struct vin_md *vind,
					struct vin_core *vinc)
{
	struct v4l2_subdev *sd;
	int ret;

	if (WARN_ON(vinc->id >= VIN_MAX_DEV))
		return -EBUSY;

	sd = &vinc->vid_cap.subdev;
	v4l2_set_subdev_hostdata(sd, (void *)&vin_pipe_ops);

	ret = v4l2_device_register_subdev(&vind->v4l2_dev, sd);
	if (!ret) {
		vind->vinc[vinc->id] = vinc;
		vinc->vid_cap.user_subdev_api = vind->user_subdev_api;
	} else {
		vin_err("Failed to register vin_cap.%d (%d)\n",
			 vinc->id, ret);
	}
	return ret;
}

static int vin_md_register_entities(struct vin_md *vind,
						struct device_node *parent)
{
	int i, j, ret;

	vin_log(VIN_LOG_MD, "%s\n", __func__);

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct modules_config *module = NULL;
		struct sensor_list *sensors = NULL;

		module = &vind->modules[i];
		sensors = &vind->modules[i].sensors;

		sensors->valid_idx = NO_VALID_SENSOR;
		for (j = 0; j < sensors->detect_num; j++) {
			if (sensors->use_sensor_list == 1)
				__vin_handle_sensor_info(&sensors->inst[j]);

			if (__vin_register_module(vind, module, j)) {
				sensors->valid_idx = j;
				break;
			}
		}
		vin_log(VIN_LOG_MD, "list%d valid sensor index %d\n",
			i, sensors->valid_idx);

		if (sensors->valid_idx == NO_VALID_SENSOR || !module->flash_used)
			continue;

		/*flash subdev register */
		module->modules.flash.sd = sunxi_flash_get_subdev(
						module->modules.flash.id);

		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
					    module->modules.flash.sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "flash%d register fail!\n",
					module->modules.flash.id);
	}

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct modules_config *module = NULL;

		/*video device register */
		vind->vinc[i] = sunxi_vin_core_get_dev(i);
		if (vind->vinc[i] == NULL) {
			vin_print("vinc%d is null\n", i);
			continue;

		}

		vind->vinc[i]->v4l2_dev = &vind->v4l2_dev;

		module = &vind->modules[vind->vinc[i]->rear_sensor];

		if (module->sensors.valid_idx == NO_VALID_SENSOR) {
			vind->vinc[i] = NULL;
			continue;
		}
		vin_md_register_core_entity(vind, vind->vinc[i]);
	}

	for (i = 0; i < VIN_MAX_CSI; i++) {
		/*Register CSI subdev */
		vind->csi[i].id = i;
		vind->csi[i].sd = sunxi_csi_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->csi[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "csi%d register fail!\n", i);
	}

#ifdef SUPPORT_ISP_TDM
	for (i = 0; i < VIN_MAX_TDM; i++) {
		/*Register TDM subdev */
		vind->tdm[i].id = i;
		for (j = 0; j < TDM_RX_NUM; j++) {
			vind->tdm[i].tdm_rx[j].sd = sunxi_tdm_get_subdev(i, j);
			ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->tdm[i].tdm_rx[j].sd);
			if (ret < 0)
				vin_log(VIN_LOG_MD, "the tdx%d of tdx_rx%d register fail!\n", i, j);
		}
	}
#endif
	for (i = 0; i < VIN_MAX_MIPI; i++) {
		/*Register MIPI subdev */
		vind->mipi[i].id = i;
		vind->mipi[i].sd = sunxi_mipi_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->mipi[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "mipi%d register fail!\n", i);
	}

	for (i = 0; i < VIN_MAX_ISP; i++) {
		/*Register ISP subdev */
		vind->isp[i].id = i;
		vind->isp[i].sd = sunxi_isp_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->isp[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "isp%d register fail!\n", i);
		/*Register STATISTIC BUF subdev */
		vind->stat[i].id = i;
		vind->stat[i].sd = sunxi_stat_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->stat[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "stat%d register fail!\n", i);
	}

	for (i = 0; i < VIN_MAX_SCALER; i++) {
		/*Register SCALER subdev */
		vind->scaler[i].id = i;
		vind->scaler[i].sd = sunxi_scaler_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->scaler[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "scaler%d register fail!\n", i);
	}

	return 0;
}

static void vin_md_unregister_entities(struct vin_md *vind)
{
	int i;
	__maybe_unused int j;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct vin_module_info *modules = NULL;
		struct sensor_list *sensors = NULL;

		sensors = &vind->modules[i].sensors;
		if (sensors->valid_idx != NO_VALID_SENSOR) {
			__vin_unregister_module(&vind->modules[i],
						sensors->valid_idx);

			modules = &vind->modules[i].modules;
			v4l2_device_unregister_subdev(modules->flash.sd);
			modules->flash.sd = NULL;
		}

		if (vind->vinc[i] == NULL)
			continue;
		v4l2_device_unregister_subdev(&vind->vinc[i]->vid_cap.subdev);
		vind->vinc[i]->pipeline_ops = NULL;
		vind->vinc[i] = NULL;
	}

	for (i = 0; i < VIN_MAX_CSI; i++) {
		v4l2_device_unregister_subdev(vind->csi[i].sd);
		vind->cci[i].sd = NULL;
	}

#ifdef SUPPORT_ISP_TDM
	for (i = 0; i < VIN_MAX_TDM; i++) {
		for (j = 0; j < TDM_RX_NUM; j++) {
			v4l2_device_unregister_subdev(vind->tdm[i].tdm_rx[j].sd);
			vind->tdm[i].tdm_rx[j].sd = NULL;
		}
	}
#endif

	for (i = 0; i < VIN_MAX_MIPI; i++) {
		v4l2_device_unregister_subdev(vind->mipi[i].sd);
		vind->mipi[i].sd = NULL;
	}

	for (i = 0; i < VIN_MAX_ISP; i++) {
		v4l2_device_unregister_subdev(vind->isp[i].sd);
		vind->isp[i].sd = NULL;
		v4l2_device_unregister_subdev(vind->stat[i].sd);
		vind->stat[i].sd = NULL;
	}

	for (i = 0; i < VIN_MAX_SCALER; i++) {
		v4l2_device_unregister_subdev(vind->scaler[i].sd);
		vind->scaler[i].sd = NULL;
	}

	vin_log(VIN_LOG_MD, "%s\n", __func__);
}

static int sensor_link_to_mipi_csi(struct modules_config *module,
					struct v4l2_subdev *to)
{
	struct v4l2_subdev *sensor = NULL;
	struct media_entity *source, *sink;
	int ret = 0;

	if (module->sensors.valid_idx == NO_VALID_SENSOR) {
		vin_warn("Pipe line %s sensor subdev is NULL!\n",
			module->sensors.sensor_pos);
		return -1;
	}

	sensor = module->modules.sensor[module->sensors.valid_idx].sd;
	source = &sensor->entity;
	sink = &to->entity;
	ret = media_create_pad_link(source, SENSOR_PAD_SOURCE, sink, 0, 0);

	vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
		source->name, '-', sink->name);
	return ret;
}
static int vin_create_media_links(struct vin_md *vind)
{
	struct v4l2_subdev *mipi, *csi, *isp, *stat, *scaler, *cap_sd;
	struct media_entity *source, *sink;
	struct modules_config *module;
	int i, j, ret = 0;
	__maybe_unused struct v4l2_subdev *tdm_rx;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct vin_core *vinc = NULL;

		vinc = vind->vinc[i];

		if (vinc == NULL)
			continue;

		/*MIPI*/
		if (vinc->mipi_sel == 0xff)
			mipi = NULL;
		else
			mipi = vind->mipi[vinc->mipi_sel].sd;
		/*CSI*/
		if (vinc->csi_sel == 0xff)
			csi = NULL;
		else
			csi = vind->csi[vinc->csi_sel].sd;

		if (mipi != NULL) {
			/*link MIPI sensor*/
			module = &vind->modules[vinc->rear_sensor];
			sensor_link_to_mipi_csi(module, mipi);
			if (vinc->rear_sensor != vinc->front_sensor) {
				module = &vind->modules[vinc->front_sensor];
				sensor_link_to_mipi_csi(module, mipi);
			}

			if (csi == NULL) {
				vin_err("MIPI Pipe line csi subdev is NULL, "
					"DevID is %d\n", i);
				continue;
			}
			source = &mipi->entity;
			sink = &csi->entity;
			ret = media_create_pad_link(source, MIPI_PAD_SOURCE,
						       sink, CSI_PAD_SINK,
						       MEDIA_LNK_FL_ENABLED);
		} else {
			/*link Bt.601 sensor*/
			if (csi == NULL) {
				vin_err("Bt.601 Pipeline csi subdev is NULL, "
					"DevID is %d\n", i);
				continue;
			}
			module = &vind->modules[vinc->rear_sensor];
			sensor_link_to_mipi_csi(module, csi);
			if (vinc->rear_sensor != vinc->front_sensor) {
				module = &vind->modules[vinc->front_sensor];
				sensor_link_to_mipi_csi(module, csi);
			}
		}

#ifdef SUPPORT_ISP_TDM
		/*tdm*/
		if (vinc->tdm_rx_sel == 0xff)
			tdm_rx = NULL;
		else
			tdm_rx = vind->tdm[vinc->tdm_rx_sel/2].tdm_rx[vinc->tdm_rx_sel].sd;
		/*isp*/
		if (vinc->isp_sel == 0xff)
			isp = NULL;
		else
			isp = vind->isp[vinc->isp_sel].sd;

		if (tdm_rx != NULL) {
			source = &csi->entity;
			sink = &tdm_rx->entity;
			ret = media_create_pad_link(source, SCALER_PAD_SOURCE,
						sink, VIN_SD_PAD_SINK,
						MEDIA_LNK_FL_ENABLED);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '=', sink->name);

			source = &tdm_rx->entity;
			sink = &isp->entity;
			ret = media_create_pad_link(source, SCALER_PAD_SOURCE,
						sink, VIN_SD_PAD_SINK,
						MEDIA_LNK_FL_ENABLED);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '=', sink->name);
		} else {
			source = &csi->entity;
			sink = &isp->entity;
			ret = media_create_pad_link(source, SCALER_PAD_SOURCE,
						sink, VIN_SD_PAD_SINK,
						MEDIA_LNK_FL_ENABLED);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '=', sink->name);
		}
#endif
		cap_sd = &vinc->vid_cap.subdev;

		/* SCALER */
		scaler = vind->scaler[i].sd;
		if (scaler == NULL)
			continue;
		/*Link Vin Core*/
		source = &scaler->entity;
		sink = &cap_sd->entity;
		ret = media_create_pad_link(source, SCALER_PAD_SOURCE,
					       sink, VIN_SD_PAD_SINK,
					       MEDIA_LNK_FL_ENABLED);
		if (ret)
			break;

		/* Notify vin core subdev entity */
		ret = media_entity_call(sink, link_setup, &sink->pads[0],
					&source->pads[SCALER_PAD_SOURCE],
					MEDIA_LNK_FL_ENABLED);
		if (ret)
			break;

		vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
			source->name, '=', sink->name);

		source = &cap_sd->entity;
		sink = &vinc->vid_cap.vdev.entity;
		ret = media_create_pad_link(source, VIN_SD_PAD_SOURCE,
					       sink, 0, MEDIA_LNK_FL_ENABLED);
		if (ret)
			break;
		vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
			source->name, '=', sink->name);
	}

#ifndef SUPPORT_ISP_TDM
	for (i = 0; i < VIN_MAX_CSI; i++) {
		csi = vind->csi[i].sd;
		if (csi == NULL)
			continue;
		source = &csi->entity;
		for (j = 0; j < VIN_MAX_ISP; j++) {
			isp = vind->isp[j].sd;
			if (isp == NULL)
				continue;
			sink = &isp->entity;
			ret = media_create_pad_link(source, CSI_PAD_SOURCE,
						       sink, ISP_PAD_SINK, 0);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '-', sink->name);
		}
	}
#endif

	for (i = 0; i < VIN_MAX_ISP; i++) {
		isp = vind->isp[i].sd;
		if (isp == NULL)
			continue;
		source = &isp->entity;
		stat = vind->stat[i].sd;
		sink = &stat->entity;
		ret = media_create_pad_link(source, ISP_PAD_SOURCE_ST,
					       sink, 0,
					       MEDIA_LNK_FL_IMMUTABLE |
					       MEDIA_LNK_FL_ENABLED);
		vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
			source->name, '=', sink->name);

		for (j = 0; j < VIN_MAX_SCALER; j++) {
			scaler = vind->scaler[j].sd;
			if (scaler == NULL)
				continue;
			sink = &scaler->entity;
			ret = media_create_pad_link(source, ISP_PAD_SOURCE,
						sink, SCALER_PAD_SINK, 0);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '-', sink->name);
		}
	}
	return ret;
}

static int vin_setup_default_links(struct vin_md *vind)
{
	struct v4l2_subdev *isp, *scaler;
	int i, ret = 0;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct vin_core *vinc = NULL;
		struct media_link *link = NULL;
		struct vin_pipeline *p = NULL;

		vinc = vind->vinc[i];
		if (vinc == NULL)
			continue;

		/*ISP*/
		if (vinc->isp_sel == 0xff)
			isp = NULL;
		else
			isp = vind->isp[vinc->isp_sel].sd;

		/*SCALER*/
		if (vinc->vipp_sel == 0xff)
			scaler = NULL;
		else
			scaler = vind->scaler[vinc->vipp_sel].sd;
		if (isp && scaler)
			link = media_entity_find_link(&isp->entity.pads[ISP_PAD_SOURCE],
						      &scaler->entity.pads[SCALER_PAD_SINK]);

		if (link) {
			vin_log(VIN_LOG_MD, "link: source %s sink %s\n",
				link->source->entity->name,
				link->sink->entity->name);
			ret = media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
			if (ret)
				vin_err("media_entity_setup_link error\n");
		} else {
			vin_err("media_entity_find_link null\n");
			continue;
		}

		p = &vinc->vid_cap.pipe;
		vin_md_prepare_pipeline(p, &vinc->vid_cap.vdev.entity);
	}

	return ret;
}

#if IS_ENABLED(CONFIG_MULTI_FRAME)
static irqreturn_t vin_top_isr(int irq, void *priv)
{
	struct vin_md *vind = (struct vin_md *)priv;
	struct cisc_mulp_int_status status;
	unsigned long flags;

	csic_mulp_int_get_status(vind->id, &status);

	spin_lock_irqsave(&vind->slock, flags);

	if (status.mulf_done) {
		csic_mulp_int_clear_status(vind->id, MULF_DONE);
		vin_print("MULF_DONE come\n");
	}

	if (status.mulf_err) {
		csic_mulp_int_clear_status(vind->id, MULF_ERR);
		vin_print("MULF_ERR come\n");
	}

	spin_unlock_irqrestore(&vind->slock, flags);

	return IRQ_HANDLED;
}

static int vind_irq_request(struct vin_md *vind, int i)
{
	int ret;
	struct device_node *np = vind->pdev->dev.of_node;
	/*get irq resource */
	vind->irq = irq_of_parse_and_map(np, i);
	if (vind->irq <= 0) {
		vin_warn("Failing to get CSI TOP IRQ resource!\n");
		return -ENXIO;
	}

	ret = request_irq(vind->irq, vin_top_isr, IRQF_SHARED,
			vind->pdev->name, vind);

	if (ret) {
		vin_err("Failing to install CSI TOP irq (%d)!\n", ret);
		return -ENXIO;
	}
	return 0;
}
#endif

static int vin_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct v4l2_device *v4l2_dev;
	struct vin_md *vind;
	enum module_type sensor_type, act_type;
	int ret, i, num;

	vind = devm_kzalloc(dev, sizeof(*vind), GFP_KERNEL);
	if (!vind)
		return -ENOMEM;

	spin_lock_init(&vind->slock);

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("vin media failed to get device id\n");
		pdev->id = 0;
	}

	vind->id = pdev->id;
	vind->pdev = pdev;

	vind->base = of_iomap(np, 0);
	if (!vind->base) {
		vind->is_empty = 1;
		vind->base = kzalloc(0x400, GFP_KERNEL);
		if (!vind->base) {
			ret = -EIO;
			goto freedev;
		}
	}
	csic_top_set_base_addr(vind->id, (unsigned long)vind->base);

	vind->ccu_base = of_iomap(np, 1);
	if (!vind->ccu_base)
		vin_warn("vin failed to get ccu base register!\n");
	 else
		csic_ccu_set_base_addr((unsigned long)vind->ccu_base);

#if defined CONFIG_ARCH_SUN50IW10P1
	vind->cmb_top_base = of_iomap(np, 2);
	if (!vind->cmb_top_base)
		vin_warn("vin failed to get cmb top base register!\n");
	 else
		cmb_csi_set_top_base_addr((unsigned long)vind->cmb_top_base);
#endif

#ifdef CONFIG_MULTI_FRAME
	vind_irq_request(vind, 0);
#endif

#if 1
	for (num = 0; num < VIN_MAX_DEV; num++) {
#ifdef CONFIG_FLASH_MODULE
		vind->modules[num].modules.flash.type = VIN_MODULE_TYPE_GPIO;
#endif
		vind->modules[num].sensors.inst[0].cam_addr = i2c0_addr;
		strcpy(vind->modules[num].sensors.inst[0].cam_name, ccm0);
		vind->modules[num].sensors.inst[0].act_addr = act_slave;
		strcpy(vind->modules[num].sensors.inst[0].act_name, act_name);
		vind->modules[num].sensors.use_sensor_list = use_sensor_list;
		for (i = 0; i < MAX_GPIO_NUM; i++)
			vind->modules[num].sensors.gpio[i] = -1;
	}
#endif

	parse_modules_from_device_tree(vind);

	for (num = 0; num < VIN_MAX_DEV; num++) {
		sensor_type = vind->modules[num].sensors.sensor_bus_type;
		act_type = vind->modules[num].sensors.act_bus_type;

#if defined(CONFIG_CCI_MODULE) || defined(CONFIG_CCI)
		sensor_type = VIN_MODULE_TYPE_CCI;
		act_type = VIN_MODULE_TYPE_CCI;
#endif
		for (i = 0; i < MAX_DETECT_NUM; i++) {
			vind->modules[num].modules.sensor[i].type = sensor_type;
			vind->modules[num].modules.act[i].type = act_type;
		}
	}

	vin_gpio_request(vind);

	strlcpy(vind->media_dev.model, "Allwinner Vin",
		sizeof(vind->media_dev.model));

	vind->media_dev.ops = &media_device_ops;
	vind->media_dev.dev = dev;

	v4l2_dev = &vind->v4l2_dev;
	v4l2_dev->mdev = &vind->media_dev;
	strlcpy(v4l2_dev->name, "sunxi-vin", sizeof(v4l2_dev->name));

	ret = v4l2_device_register(dev, &vind->v4l2_dev);
	if (ret < 0) {
		vin_err("Failed to register v4l2_device: %d\n", ret);
		goto unmap;
	}
	media_device_init(&vind->media_dev);
	ret = media_device_register(&vind->media_dev);
	if (ret < 0) {
		vin_err("Failed to register media device: %d\n",
			 ret);
		goto err_md;
	}

	platform_set_drvdata(pdev, vind);

	ret = vin_md_get_clocks(vind);
	if (ret)
		goto err_clk;

	vind->user_subdev_api = 0;

#ifdef CONFIG_PM
	pm_runtime_enable(&pdev->dev);
#endif

	vin_md_clk_enable(vind);
	vin_set_cci_power(vind, 1);

	if (dev->of_node) {
		ret = vin_md_register_entities(vind, dev->of_node);
	} else {
		vin_err("Device tree of_node is NULL!\n");
		ret = -ENOSYS;
		goto err_clk;
	}

	vin_set_cci_power(vind, 0);
	vin_md_clk_disable(vind);

	mutex_lock(&vind->media_dev.graph_mutex);
	ret = vin_create_media_links(vind);
	mutex_unlock(&vind->media_dev.graph_mutex);
	if (ret) {
		vin_err("vin_create_media_links error\n");
		goto err_clk;
	}

	/*
	 * when use media_entity_setup_link we should
	 * pay attention to graph_mutex dead lock.
	 */
	ret = vin_setup_default_links(vind);
	if (ret) {
		vin_err("vin_setup_default_links error\n");
		goto err_clk;
	}

	ret = v4l2_device_register_subdev_nodes(&vind->v4l2_dev);
	if (ret)
		goto err_clk;

	ret = device_create_file(&pdev->dev, &dev_attr_subdev_api);
	if (ret)
		goto err_clk;


	vin_log(VIN_LOG_MD, "%s ok!\n", __func__);
	return 0;

err_clk:
	vin_md_put_clocks(vind);
	vin_md_unregister_entities(vind);
	media_device_unregister(&vind->media_dev);
err_md:
	v4l2_device_unregister(&vind->v4l2_dev);
unmap:
	if (!vind->is_empty)
		iounmap(vind->base);
	else
		kfree(vind->base);
freedev:
	devm_kfree(dev, vind);
	return ret;
}

static int vin_remove(struct platform_device *pdev)
{
	struct vin_md *vind = (struct vin_md *)dev_get_drvdata(&pdev->dev);

	device_remove_file(&pdev->dev, &dev_attr_subdev_api);
	vin_md_put_clocks(vind);
	vin_mclk_pin_release(vind);
	vin_gpio_release(vind);
	vin_md_unregister_entities(vind);
	v4l2_device_unregister(&vind->v4l2_dev);
	media_device_unregister(&vind->media_dev);
	media_device_cleanup(&vind->media_dev);
#ifdef CONFIG_PM
	pm_runtime_disable(&pdev->dev);
#endif
	if (vind->base) {
		if (!vind->is_empty)
			iounmap(vind->base);
		else
			kfree(vind->base);
	}

	devm_kfree(&pdev->dev, vind);
	vin_log(VIN_LOG_MD, "%s ok!\n", __func__);
	return 0;
}

static void vin_shutdown(struct platform_device *pdev)
{
	vin_log(VIN_LOG_MD, "%s!\n", __func__);
}

#ifdef CONFIG_PM

int vin_runtime_suspend(struct device *d)
{
	return 0;
}
int vin_runtime_resume(struct device *d)
{
	return 0;
}

int vin_runtime_idle(struct device *d)
{
	return 0;
}

#endif

int vin_suspend(struct device *d)
{
	return 0;
}

int vin_resume(struct device *d)
{
	return 0;
}

static const struct dev_pm_ops vin_runtime_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vin_suspend, vin_resume)
	SET_RUNTIME_PM_OPS(vin_runtime_suspend, vin_runtime_resume,
				vin_runtime_idle)
};

static const struct of_device_id sunxi_vin_match[] = {
	{.compatible = "allwinner,sunxi-vin-media",},
	{},
};

static struct platform_driver vin_driver = {
	.probe = vin_probe,
	.remove = vin_remove,
	.shutdown = vin_shutdown,
	.driver = {
		   .name = VIN_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_vin_match,
		   .pm = &vin_runtime_pm_ops,
	}
};
static int __init vin_init(void)
{
	int ret;

	vin_log(VIN_LOG_MD, "Welcome to Video Input driver\n");
	ret = sunxi_csi_platform_register();
	if (ret) {
		vin_err("Sunxi csi driver register failed\n");
		return ret;
	}

#ifdef SUPPORT_ISP_TDM
	ret = sunxi_tdm_platform_register();
	if (ret) {
		vin_err("Sunxi tdm driver register failed\n");
		return ret;
	}
#endif

	ret = sunxi_isp_platform_register();
	if (ret) {
		vin_err("Sunxi isp driver register failed\n");
		return ret;
	}

	ret = sunxi_mipi_platform_register();
	if (ret) {
		vin_err("Sunxi mipi driver register failed\n");
		return ret;
	}

	ret = sunxi_flash_platform_register();
	if (ret) {
		vin_err("Sunxi flash driver register failed\n");
		return ret;
	}

	ret = sunxi_scaler_platform_register();
	if (ret) {
		vin_err("Sunxi scaler driver register failed\n");
		return ret;
	}

	ret = sunxi_vin_core_register_driver();
	if (ret) {
		vin_err("Sunxi vin register driver failed!\n");
		return ret;
	}

	ret = platform_driver_register(&vin_driver);
	if (ret) {
		vin_err("Sunxi vin register driver failed!\n");
		return ret;
	}

	ret = sunxi_vin_debug_register_driver();
	if (ret) {
		vin_err("Sunxi vin debug register driver failed!\n");
		return ret;
	}

	vin_log(VIN_LOG_MD, "vin init end\n");
	return ret;
}

static void __exit vin_exit(void)
{
	vin_log(VIN_LOG_MD, "vin_exit\n");
	platform_driver_unregister(&vin_driver);
	sunxi_vin_debug_unregister_driver();
	sunxi_vin_core_unregister_driver();
	sunxi_csi_platform_unregister();
#ifdef SUPPORT_ISP_TDM
	sunxi_tdm_platform_unregister();
#endif
	sunxi_isp_platform_unregister();
	sunxi_mipi_platform_unregister();
	sunxi_flash_platform_unregister();
	sunxi_scaler_platform_unregister();
	vin_log(VIN_LOG_MD, "vin_exit end\n");
}

late_initcall(vin_init);
//module_init(vin_init);
module_exit(vin_exit);

MODULE_AUTHOR("yangfeng");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video Input Module for Allwinner");

