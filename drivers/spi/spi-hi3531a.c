/* linux/drivers/spi/spi-hi3531a.c
 *
 * HI3516A SPI Controller driver
 *
 * Copyright (c) 2014 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * History:
 *	3-February-2015 create this file
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-hisilicon.h>

#include <mach/hardware.h>

/* ********************** spi cs ******************************* */
#define HI3531A_SPI_NUM_CS	4

static int hi3531a_spi_cfg_cs(s16 bus_num, u8 csn)
{
	unsigned int val;

	hi_msg("\n");
	switch (bus_num) {
	case 0:
		if (csn < HI3531A_SPI_NUM_CS) {
			val = readl(__io_address(MISC_REG_BASE + REG_SSP_CS));
			val &= ~SSP_CS_SEL_MASK;
			val |= SSP_CS_SEL(csn);
			writel(val, __io_address(MISC_REG_BASE + REG_SSP_CS));
		} else {
			dev_err(NULL, "%s, %s, %d line: error\n",
					__FILE__, __func__, __LINE__);
			return -1;
		}
		break;

	default:
		dev_err(NULL, "%s, %s, %d line: error\n",
				__FILE__, __func__, __LINE__);
		return -1;

	}
	return 0;
}

static int hi3531a_spi_hw_init_cfg(s16 bus_num)
{
	unsigned int value;

	hi_msg("\n");
	switch (bus_num) {
	case 0:
		value = readl(__io_address(CRG_REG_BASE + REG_SSP_CRG));
		value &= ~SSP_SOFT_RESET_REQ;
		value |= SSP_CLK_ENABLE;
		writel(value, __io_address(CRG_REG_BASE + REG_SSP_CRG));
		break;

	default:
		dev_err(NULL, "%s, %s, %d line: error\n",
				__FILE__, __func__, __LINE__);
		return -1;
	}
	return 0;
}

static int hi3531a_spi_hw_exit_cfg(s16 bus_num)
{
	unsigned int value;

	hi_msg("\n");
	switch (bus_num) {
	case 0:
		value = readl(__io_address(CRG_REG_BASE + REG_SSP_CRG));
		value |= SSP_SOFT_RESET_REQ;
		value &= ~SSP_CLK_ENABLE;
		writel(value, __io_address(CRG_REG_BASE + REG_SSP_CRG));
		break;

	default:
		dev_err(NULL, "%s, %s, %d line: error\n",
				__FILE__, __func__, __LINE__);
		return -1;
	}
	return 0;
}

int hi_spi_set_platdata(struct hi_spi_platform_data *spd,
		struct platform_device *pdev)
{
	hi_msg("\n");

	if ((spd == NULL) || (pdev == NULL)) {
		dev_err(NULL, "%s (spd || pdev) == NULL\n", __func__);
		return -1;
	}

	switch (pdev->id) {
	case 0:
		spd->num_cs = HI3531A_SPI_NUM_CS;
		break;

	default:
		dev_err(NULL, "%s bus num error\n", __func__);
		return -1;
	}
	spd->clk_rate = get_bus_clk() / 4;
	spd->cfg_cs = hi3531a_spi_cfg_cs;
	spd->hw_init_cfg = hi3531a_spi_hw_init_cfg;
	spd->hw_exit_cfg = hi3531a_spi_hw_exit_cfg;

	pdev->dev.platform_data = spd;

	return 0;
}
EXPORT_SYMBOL(hi_spi_set_platdata);
