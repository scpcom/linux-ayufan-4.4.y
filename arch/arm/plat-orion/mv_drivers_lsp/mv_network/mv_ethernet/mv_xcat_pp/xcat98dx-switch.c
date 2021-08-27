/******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

*******************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mbus.h>
#include "xcat98dx-switch.h"
#include "xcat98dx-eprom.h"
#include "xcat98dx-core.h"
#include "xcat98dx-regs.h"

#define WINDOW_BASE(w)			(0x030c + ((w) << 3))
#define WINDOW_SIZE(w)			(0x0310 + ((w) << 3))
#define WINDOW_REMAP_HIGH(w)		(0x033c + ((w) << 2))
#define WINDOW_BAR_ENABLE		(0x034c)

void mvPpBridgeTeachNewAddr(MV_U8 *mac)
{
	MV_U32 macL, macH, timeout, val;

	macL =  (mac[4] << 24) | (mac[5] << 16);
	macH =  (mac[0] << 24) | (mac[1] << 16) |
		(mac[2] << 8)  | (mac[3] << 0);

	/* Config CPU MAC address on PP switch */
	mvPpWriteReg(MV_PP_DEV0, 0x06000040, macL);
	mvPpWriteReg(MV_PP_DEV0, 0x06000044, macH);
	mvPpWriteReg(MV_PP_DEV0, 0x06000048, 0x00FC0001);
	mvPpWriteReg(MV_PP_DEV0, 0x0600004C, 0x00440000);
	mvPpWriteReg(MV_PP_DEV0, 0x06000050, 0x00000000);
	mvPpWriteReg(MV_PP_DEV0, 0x06000050, 0x00000001);

	/* wait to operation done */
	timeout = 0;
	do {
		val = mvPpReadReg(MV_PP_DEV0, 0x06000050);
		if ((val & 1) == 0)
			break;
		timeout++;
	} while (timeout < 10);

	if (timeout >= 10)
		printk(KERN_ERR "%s: timeout in teach MAC address\n", __func__);
}

/*
 * Config PP switch memory windows to enable DRAM access.
 */
static void pp_conf_mbus_windows(const struct mbus_dram_target_info *dram)
{
	void __iomem *base = (void __iomem *)KIRKWOOD_PP_VIRT_BASE;
	u32 win_enable;
	int i;

	for (i = 0; i < 6; i++) {
		writel(0, base + WINDOW_BASE(i));
		writel(0, base + WINDOW_SIZE(i));
		if (i < 4)
			writel(0, base + WINDOW_REMAP_HIGH(i));
	}

	win_enable = 0x3f;

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->base & 0xffff0000) |
			(cs->mbus_attr << 8) |
			dram->mbus_dram_target_id, base + WINDOW_BASE(i));
		writel((cs->size - 1) & 0xffff0000, base + WINDOW_SIZE(i));

		win_enable &= ~(1 << i);
	}

	writel(win_enable, base + WINDOW_BAR_ENABLE);
}

/*
 * Config KW GbE (ethernet) port
 */
static int xcat_config_gbe(mv_eth_priv *priv)
{
	int ret_val = 0;

	ret_val = mvEthForceLinkUp(priv->hal_priv);
	if (ret_val)
		printk(KERN_ERR "%s: mvEthForceLinkUp failed\n", __func__);

	ret_val = mvEthSpeedDuplexSet(priv->hal_priv,
			MV_ETH_SPEED_1000, MV_ETH_DUPLEX_FULL);
	if (ret_val)
		printk(KERN_ERR "%s: mvEthSpeedDuplexSet failed\n", __func__);

	ret_val = mvEthHeaderModeSet(priv->hal_priv,
			MV_ETH_DISABLE_HEADER_MODE);
	if (ret_val)
		printk(KERN_ERR "%s: mvEthHeaderModeSet failed\n", __func__);

	ret_val = mvEthDSAModeSet(priv->hal_priv, MV_ETH_DSA_TAG_EXTENDED);
	if (ret_val)
		printk(KERN_ERR "%s: mvEthDSAModeSet failed\n", __func__);

	return ret_val;
}

static void xcat_config_pp(mv_eth_priv *priv)
{
	const struct mbus_dram_target_info *dram;

	/*
	 * (Re-)program MBUS remapping windows if we are asked to.
	 */
	dram = mv_mbus_dram_info();
	if (dram)
		pp_conf_mbus_windows(dram);

	/*
	 * Now PP switch registers can be accessed for configuration.
	 */
	mvPpCpuPortConfig(MV_PP_DEV0);
	mvPpCpuPortCascadeModeSet(MV_PP_DEV0);
	mvPpCpuPortTwoBytePrependDisable();

	mvPpConfigNetworkPorts();
}

/*
 * Setup MPPs to internally connect Gbe1 to PP cpu port (63)
 */
static void xcat_config_gbe_switch_mpp(void)
{
	unsigned int val, mpp_base = DEV_BUS_VIRT_BASE;
	unsigned int gpio_high_base = GPIO_HIGH_VIRT_BASE;

	/* Setup MPP [20-23] */
	val = readl(mpp_base + 0x8);
	writel(val | 0x33330000, mpp_base + 0x8);

	/* Setup MPP [24-27] and [30-31] */
	writel(0x33003333, mpp_base + 0xC);

	/* Setup MPP [30-33] */
	val = readl(mpp_base + 0x10);
	writel(val | 0x33, mpp_base + 0x10);

	/* Setup GPIO_OE[46] = 1 */
	writel(0x4000, gpio_high_base + 0x4);
}

/*
 * Config internal [KW-GbE] to [switch CPU port] interconnect.
 */
int xcat_config_internal_mii(mv_eth_priv *priv)
{
	int ret_val = 0;

	xcat_config_gbe_switch_mpp();

	ret_val = xcat_config_gbe(priv);
	if (ret_val)
		printk(KERN_ERR "%s: xcat_config_gbe failed\n", __func__);

	xcat_config_pp(priv);

	return ret_val;
}
