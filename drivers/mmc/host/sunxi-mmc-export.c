/*
* Sunxi SD/MMC host driver
*
* Copyright (C) 2015 AllWinnertech Ltd.
* Author: lixiang <lixiang@allwinnertech>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/


#include <linux/clk.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>

#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>

#include "sunxi-mmc-export.h"

#define SDXC_REG_STAS	(0x3C)	/* SMC Status Register */

#define mmc_readl(host, reg) \
	readl((host)->reg_base + SDXC_##reg)

#define SDXC_CARD_DATA_BUSY		BIT(9)

#define SUNXI_MAX_CONTROL	4

static struct sunxi_mmc_host_base *sunxi_hosts[SUNXI_MAX_CONTROL] = { NULL };

void sunxi_mmc_rescan_card(unsigned id)
{

	if (id > SUNXI_MAX_CONTROL) {
		pr_err("%d id over max id", id);
		return;
	}

	if (sunxi_hosts[id] == NULL) {
		pr_err("sunxi_hosts[%d] should not be null", id);
		return;
	}

	if (sunxi_hosts[id] == NULL) {
		dev_err(mmc_dev(sunxi_hosts[id]->mmc),
			"%s:can't find the host\n", __func__);
		return;
	}
	mmc_detect_change(sunxi_hosts[id]->mmc, 0);
}
EXPORT_SYMBOL_GPL(sunxi_mmc_rescan_card);

void sunxi_mmc_reg_ex_res_inter(struct sunxi_mmc_host_base *host, u32 phy_id)
{
	if (phy_id > SUNXI_MAX_CONTROL) {
		pr_err("%d id over max id", phy_id);
		return;
	}
	sunxi_hosts[phy_id] = host;
}
EXPORT_SYMBOL_GPL(sunxi_mmc_reg_ex_res_inter);

int sunxi_mmc_check_r1_ready(struct mmc_host *mmc, unsigned ms)
{
	struct sunxi_mmc_host_base *host = mmc_priv(mmc);
	unsigned long expire = jiffies + msecs_to_jiffies(ms);

	do {
		if (!(mmc_readl(host, REG_STAS) & SDXC_CARD_DATA_BUSY))
			break;
	} while (time_before(jiffies, expire));

	if ((mmc_readl(host, REG_STAS) & SDXC_CARD_DATA_BUSY)) {
		dev_err(mmc_dev(mmc), "wait r1 rdy %d ms timeout\n", ms);
		return -1;
	} else
		return 0;
}
EXPORT_SYMBOL_GPL(sunxi_mmc_check_r1_ready);

MODULE_DESCRIPTION("Sunxi SD/MMC host driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lixiang <lixiang@allwinnertech>");
