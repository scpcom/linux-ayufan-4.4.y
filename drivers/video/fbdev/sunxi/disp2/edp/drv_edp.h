/* drv_edp.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * edp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DRV_EDP_H
#define _DRV_EDP_H

#include "drv_edp_common.h"
#include "de_edp.h"
#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
#include <linux/switch.h>
#endif
#if defined(CONFIG_EXTCON)
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#endif

#define EDP_NUM_MAX 2
#define EDP_POWER_STR_LEN 32

enum edp_hpd_status {
	EDP_STATUE_CLOSE = 0,
	EDP_STATUE_OPEN = 1,
};

/**
 * save some info here for every edp module
 */
struct drv_edp_info_t {
	struct device *dev;
	uintptr_t base_addr;
	struct clk *clk;
	struct clk *clk_parent;
	bool suspend;
	bool used;
	u32 enable;
	struct mutex mlock;
	struct edp_para para;
	u32 edp_io_power_used;
	char edp_io_power[EDP_POWER_STR_LEN];
	struct disp_video_timings timings;
};

extern s32 disp_set_edp_func(struct disp_tv_func *func);
extern unsigned int disp_boot_para_parse(const char *name);

#endif /*End of file*/
